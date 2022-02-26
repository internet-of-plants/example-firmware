#include "driver/esp8266/internal_cert_store.hpp"
#include "driver/panic.hpp"

namespace driver {
  CertStore::CertStore(CertList list) noexcept: internal(new (std::nothrow) InternalCertStore(list)) {
  iop_assert(internal, IOP_STR("Unable to allocate InternalCertStore"));
}
CertStore::~CertStore() noexcept {
    delete this->internal;
}

constexpr const uint8_t hashSize = 32;
auto InternalCertStore::findHashedTA(void *ctx, void *hashed_dn, size_t len) -> const br_x509_trust_anchor * {
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

void InternalCertStore::freeHashedTA(void *ctx, const br_x509_trust_anchor *ta) {
  IOP_TRACE();
  (void)ta; // Unused

  auto *ptr = static_cast<InternalCertStore *>(ctx);
  delete ptr->x509;
  ptr->x509 = nullptr;
}

void InternalCertStore::installCertStore(br_x509_minimal_context *ctx) {
  IOP_TRACE();
  br_x509_minimal_set_dynamic(ctx, this, findHashedTA, freeHashedTA);
}
}