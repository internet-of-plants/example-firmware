#include "driver/cert_store.hpp"
#include "driver/panic.hpp"

#include "CertStoreBearSSL.h"

namespace driver {
constexpr const uint8_t hashSize = 32;
struct InternalCertStore : public BearSSL::CertStoreBase {
  BearSSL::X509List * x509;
  CertList certList;

  InternalCertStore(CertList list) noexcept: x509(nullptr), certList(list) {}
  ~InternalCertStore() noexcept {
    delete this->x509;
  }

  /// Called by libraries like `iop::Network`. Call `iop::Network::setCertList` before making a TLS connection.
  ///
  /// Panics if CertList is not set with `iop::Network::setCertList`.
  void installCertStore(br_x509_minimal_context *ctx) final {
    IOP_TRACE();
    br_x509_minimal_set_dynamic(ctx, this, findHashedTA, freeHashedTA);
  }

  // These need to be static as they are called from from BearSSL C code
  // Panics if maybeCertList is None. Used to find appropriate cert for the connection

  static auto findHashedTA(void *ctx, void *hashed_dn, size_t len) -> const br_x509_trust_anchor * {
    IOP_TRACE();
    auto *cs = static_cast<InternalCertStore *>(ctx);

    iop_assert(cs, IOP_STR("ctx is nullptr, this is unreachable because if this method is accessible, the ctx is set"));
    iop_assert(hashed_dn, IOP_STR("hashed_dn is nullptr, this is unreachable because it's a static array"));
    iop_assert(len == hashSize, IOP_STR("Invalid hash len"));

    const auto &list = cs->certList;
    for (uint16_t i = 0; i < list.count(); i++) {
      const auto cert = list.cert(i);

      if (memcmp_P(hashed_dn, cert.index, hashSize) == 0) {
        const auto size = cert.size;
        auto der = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[size]);
        iop_assert(der, IOP_STR("Cert allocation failed"));

        memcpy_P(der.get(), cert.cert, size);
        cs->x509 = new (std::nothrow) BearSSL::X509List(der.get(), size);
        iop_assert(cs->x509, IOP_STR("OOM"));
        der.reset();

        // We can const cast because it's heap allocated
        // It shouldn't be a const function. But the upstream API is just that way
        br_x509_trust_anchor *ta = (br_x509_trust_anchor*)cs->x509->getTrustAnchors();
        memcpy_P(ta->dn.data, cert.index, hashSize);
        ta->dn.len = hashSize;

        return ta;
      }
    }

    return nullptr;
  }

  static void freeHashedTA(void *ctx, const br_x509_trust_anchor *ta) {
    IOP_TRACE();
    (void)ta; // Unused

    auto *ptr = static_cast<InternalCertStore *>(ctx);
    delete ptr->x509;
    ptr->x509 = nullptr;
  }
};

CertStore::CertStore(CertList list) noexcept: internal(new (std::nothrow) InternalCertStore(list)) {
  iop_assert(internal, IOP_STR("Unable to allocate InternalCertStore"));
}
CertStore::~CertStore() noexcept {
    delete this->internal;
}
}