#include "driver/client.hpp"
#include "driver/network.hpp"
#include "driver/panic.hpp"
#include "driver/wifi.hpp"

#include <system_error>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <string>
#include <stdio.h>
#include <stdlib.h>

// POSIX
/* Assume that any non-Windows platform uses POSIX-style sockets instead. */
#include <sys/socket.h>
#include <arpa/inet.h>
//#include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
#include <unistd.h> /* Needed for close() */

static iop::Log clientDriverLogger(iop::LogLevel::DEBUG, IOP_STATIC_STRING("HTTP Client"));

auto shiftChars(char* ptr, const size_t start, const size_t end) noexcept {
  //clientDriverLogger.debug(std::string_view(ptr).substr(start, end), std::to_string(start), " ", std::to_string(end));
  memmove(ptr, ptr + start, end);
  memset(ptr + end, '\0', 4096 - end);
}

namespace driver {
auto rawStatus(const int code) noexcept -> RawStatus {
  IOP_TRACE();
  switch (code) {
  case 200:
    return RawStatus::OK;
  case 500:
    return RawStatus::SERVER_ERROR;
  case 403:
    return RawStatus::FORBIDDEN;

  // We generally don't use default to be able to use static-analyzers to check
  // for exaustiveness, but this is a switch on a int, so...
  default:
    return RawStatus::UNKNOWN;
  }
}

HTTPClient::HTTPClient() noexcept: headersToCollect_() {}
HTTPClient::~HTTPClient() noexcept {}

static ssize_t send(int fd, const char * msg, const size_t len) noexcept {
  if (iop::Log::isTracing())
    iop::Log::print(msg, iop::LogLevel::TRACE, iop::LogType::STARTEND);
  return write(fd, msg, len);
}

Session::Session(HTTPClient &http, std::string_view uri, std::shared_ptr<int> fd) noexcept: fd_(std::move(fd)), http(std::ref(http)), headers{}, uri_(uri) { IOP_TRACE(); }
Session::~Session() noexcept {
  if (this->fd_.use_count() == 1)
    close(*this->fd_);
}
void HTTPClient::headersToCollect(const char * headers[], size_t count) noexcept {
  std::vector<std::string> vec;
  vec.reserve(count);
  for (size_t index = 0; index < count; ++index) {
    std::string key = headers[index];
    // Headers can't be UTF8 so we cool
    std::transform(key.begin(), key.end(), key.begin(),
            [](unsigned char c){ return std::tolower(c); });
    vec.push_back(key);
  }
  this->headersToCollect_ = std::move(vec);
}
std::string Response::header(iop::StaticString key) const noexcept {
  auto keyString = key.toString();
  // Headers can't be UTF8 so we cool
  std::transform(keyString.begin(), keyString.end(), keyString.begin(),
      [](unsigned char c){ return std::tolower(c); });
  if (this->headers_.count(keyString) == 0) return "";
  return this->headers_.at(keyString);
}
void Session::addHeader(iop::StaticString key, iop::StaticString value) noexcept  {
  auto keyLower = key.toString();
  this->headers.emplace(keyLower, value.toString());
}
void Session::addHeader(iop::StaticString key, std::string_view value) noexcept  {
  auto keyLower = key.toString();
  this->headers.emplace(keyLower, std::string(value));
}
void Session::addHeader(std::string_view key, iop::StaticString value) noexcept  {
  std::string keyLower(key);
  this->headers.emplace(keyLower, value.toString());
}
void Session::addHeader(std::string_view key, std::string_view value) noexcept  {
  std::string keyLower(key);
  this->headers.emplace(keyLower, std::string(value));
}
void Session::setAuthorization(std::string auth) noexcept  {
  if (auth.length() == 0) return;
  this->headers.emplace(std::string("Authorization"), std::string("Basic ") + auth);
}

auto Session::sendRequest(const std::string method, const uint8_t *data, const size_t len) noexcept -> std::variant<Response, int> {
  if (iop::data.wifi.status() != driver::StationStatus::GOT_IP) return static_cast<int>(RawStatus::CONNECTION_FAILED);

  iop_assert(this->http, IOP_STATIC_STRING("Session has been moved out"));

  std::unordered_map<std::string, std::string> responseHeaders;
  std::vector<uint8_t> responsePayload;
  auto status = std::make_optional(1000);
  const auto fd = *this->fd_;

  responseHeaders.reserve(this->http->get().headersToCollect_.size());

  {
    const std::string_view path(this->uri_.begin() + this->uri_.find("/", this->uri_.find("://") + 3));
    clientDriverLogger.debug(IOP_STATIC_STRING("Send request to "), path);

    const auto fd = *this->fd_;
    iop_assert(fd != -1, IOP_STATIC_STRING("Invalid file descriptor"));
    if (clientDriverLogger.level() == iop::LogLevel::TRACE || iop::Log::isTracing())
      iop::Log::print(IOP_STATIC_STRING(""), iop::LogLevel::TRACE, iop::LogType::START);
    send(fd, method.c_str(), method.length());
    send(fd, " ", 1);
    send(fd, path.begin(), path.length());
    send(fd, " HTTP/1.0\r\n", 11);
    send(fd, "Content-Length: ", 16);
    const auto dataLengthStr = std::to_string(len);
    send(fd, dataLengthStr.c_str(), dataLengthStr.length());
    send(fd, "\r\n", 2);
    for (const auto& [key, value]: this->headers) {
      send(fd, key.c_str(), key.length());
      send(fd, ": ", 2);
      send(fd, value.c_str(), value.length());
      send(fd, "\r\n", 2);
    }
    send(fd, "\r\n", 2);
    send(fd, reinterpret_cast<const char*>(data), len);
    if (clientDriverLogger.level() == iop::LogLevel::TRACE || iop::Log::isTracing())
      iop::Log::print(IOP_STATIC_STRING("\n"), iop::LogLevel::TRACE, iop::LogType::END);
    clientDriverLogger.debug(IOP_STATIC_STRING("Sent data: "), std::string_view(reinterpret_cast<const char*>(data), len));
    
    auto buffer = std::unique_ptr<char[]>(new (std::nothrow) char[4096]);
    iop_assert(buffer, IOP_STATIC_STRING("OOM"));
    memset(buffer.get(), '\0', 4096);
    
    ssize_t signedSize = 0;
    size_t size = 0;
    auto firstLine = true;
    auto isPayload = false;
    
    std::string_view buff;
    while (true) {
      if (!isPayload) {
        //clientDriverLogger.debug(IOP_STATIC_STRING("Try read: "), buff);
      }

      if (size < 4095 &&
          (signedSize = read(fd, buffer.get() + size, 4095 - size)) < 0) {
        clientDriverLogger.error(IOP_STATIC_STRING("Error reading from socket ("), std::to_string(signedSize), IOP_STATIC_STRING("): "), std::to_string(errno), IOP_STATIC_STRING(" - "), strerror(errno)); 
        return static_cast<int>(RawStatus::CONNECTION_FAILED);
      }
      
      size += static_cast<size_t>(signedSize);

      clientDriverLogger.debug(IOP_STATIC_STRING("Len: "), std::to_string(size));
      if (firstLine && size == 0) {
        clientDriverLogger.warn(IOP_STATIC_STRING("Empty request: "), std::to_string(fd), IOP_STATIC_STRING(" "), std::to_string(size));
        return Response(status.value_or(500));
      }

      if (size == 0) {
        clientDriverLogger.debug(IOP_STATIC_STRING("EOF: "), std::to_string(isPayload));
        break;
      }

      buff = std::string_view(buffer.get(), size);

      if (!firstLine && !isPayload && buff.find("\r\n") == 0) {
          // TODO: move this logic to inside Response::await to be lazy-evaluated
          clientDriverLogger.debug(IOP_STATIC_STRING("Found Payload"));
          isPayload = true;

          clientDriverLogger.debug(std::to_string(size));
          size -= 2;
          shiftChars(buffer.get(), 2, size);
          buff = std::string_view(buffer.get(), size);
      }
      
      if (!isPayload) {
        clientDriverLogger.debug(IOP_STATIC_STRING("Buffer ["), std::to_string(size), IOP_STATIC_STRING("]: "), buff.substr(0, buff.find("\r\n") == buff.npos ? (size > 30 ? 30 : size) : buff.find("\r\n")));
      }
      clientDriverLogger.debug(IOP_STATIC_STRING("Is Payload: "), std::to_string(isPayload));

      if (!isPayload && buff.find("\n") == buff.npos) {
        iop_panic(IOP_STATIC_STRING("Header line used more than 4kb, currently we don't support this: ").toString() + buffer.get());
      }

      if (firstLine && size < 10) { // len("HTTP/1.1 ") = 9
        clientDriverLogger.error(IOP_STATIC_STRING("Error reading first line: "), std::to_string(size));
        return static_cast<int>(RawStatus::READ_FAILED);
      }

      if (firstLine && size > 0) {
        clientDriverLogger.debug(IOP_STATIC_STRING("Found first line"));

        const std::string_view statusStr(buffer.get() + 9); // len("HTTP/1.1 ") = 9
        const auto codeEnd = statusStr.find(" ");
        if (codeEnd == statusStr.npos) {
          clientDriverLogger.error(IOP_STATIC_STRING("Bad server: "), statusStr, IOP_STATIC_STRING(" -- "), buff);
          return static_cast<int>(RawStatus::READ_FAILED);
        }
        status = atoi(std::string(statusStr.begin(), 0, codeEnd).c_str());
        clientDriverLogger.debug(IOP_STATIC_STRING("Status: "), std::to_string(status.value_or(500)));
        firstLine = false;

        const auto newlineIndex = buff.find("\n") + 1;
        size -= newlineIndex;
        shiftChars(buffer.get(), newlineIndex, size);
        buff = std::string_view(buffer.get(), size);
      }
      if (!status) {
        clientDriverLogger.error(IOP_STATIC_STRING("No status"));
        return static_cast<int>(RawStatus::READ_FAILED);
      }
      if (!isPayload) {
        //clientDriverLogger.debug(IOP_STATIC_STRING("Buffer: "), buff.substr(0, buff.find("\r\n")));
      //if (!buff.contains(IOP_STATIC_STRING("\r\n"))) continue;
        //clientDriverLogger.debug(IOP_STATIC_STRING("Headers + Payload: "), buff.substr(0, buff.find("\r\n") == buff.npos ? size : buff.find("\r\n")));
      }

      while (size > 0 && !isPayload) {
        // TODO: if a line is split into two reads (because of buff len) we are screwed
        if (buff.find("\r\n") == 0) {
          size -= 2;
          shiftChars(buffer.get(), 2, size);
          buff = std::string_view(buffer.get(), size);
          
          clientDriverLogger.debug(IOP_STATIC_STRING("Found Payload ("), std::to_string(buff.find("\r\n") == buff.npos ? size + 2 : buff.find("\r\n") + 2), IOP_STATIC_STRING(")"));
          isPayload = true;
        } else if (buff.find("\r\n") != buff.npos) {
          clientDriverLogger.debug(IOP_STATIC_STRING("Found headers (buffer length: "), std::to_string(size), IOP_STATIC_STRING(")"));
          iop_assert(this->http, IOP_STATIC_STRING("Session has been moved out"));
          for (const auto &key: this->http->get().headersToCollect_) {
            if (size < key.length() + 2) continue; // "\r\n"
            std::string headerKey(buff.substr(0, key.length()));
            // Headers can't be UTF8 so we cool
            std::transform(headerKey.begin(), headerKey.end(), headerKey.begin(),
              [](unsigned char c){ return std::tolower(c); });

            clientDriverLogger.debug(headerKey, IOP_STATIC_STRING(" == "), key);
            if (headerKey != key.c_str())
              continue;

            auto valueView = buff.substr(key.length());
            if (*valueView.begin() == ':') valueView = valueView.substr(1);
            while (*valueView.begin() == ' ') valueView = valueView.substr(1);

            std::string value(valueView, 0, valueView.find("\r\n"));
            clientDriverLogger.debug(IOP_STATIC_STRING("Found header "), key, IOP_STATIC_STRING(" = "), value, IOP_STATIC_STRING("\n"));
            responseHeaders.emplace(key.c_str(), value);
          }
          clientDriverLogger.debug(IOP_STATIC_STRING("Buffer: "), buff.substr(0, buff.find("\r\n") == buff.npos ? (size > 30 ? 30 : size) : buff.find("\r\n")));
          clientDriverLogger.debug(IOP_STATIC_STRING("Skipping header ("), std::to_string(buff.find("\r\n") == buff.npos ? size : buff.find("\r\n") + 2), IOP_STATIC_STRING(")"));
          const auto startIndex = buff.find("\r\n") + 2;
          size -= startIndex;
          shiftChars(buffer.get(), startIndex, size);
          buff = std::string_view(buffer.get(), size);
        } else {
          clientDriverLogger.debug(IOP_STATIC_STRING("Newline not found"));
          break;
        }
      }

      // TODO: move this logic to inside Response::await to be lazy-evaluated
      if (isPayload && size > 0) {
        clientDriverLogger.debug(IOP_STATIC_STRING("Appending payload: "), std::to_string(size), IOP_STATIC_STRING(" "), std::to_string(responsePayload.size()));
        responsePayload.insert(responsePayload.end(), buffer.get(), buffer.get() + size);
        memset(buffer.get(), '\0', 4096);
        buff = "";
        size = 0;
      }
    }
  }

  clientDriverLogger.debug(IOP_STATIC_STRING("Close client: "), std::to_string(fd));
  clientDriverLogger.debug(IOP_STATIC_STRING("Status: "), std::to_string(status.value_or(500)));
  iop_assert(status, IOP_STATIC_STRING("Status not available"));

  return Response(responseHeaders, Payload(responsePayload), *status);
}

auto HTTPClient::begin(std::string_view uri) noexcept -> std::optional<Session> {    
  struct sockaddr_in serv_addr;
  int32_t fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    clientDriverLogger.error(IOP_STATIC_STRING("Unable to open socket"));
    return std::nullopt;
  }

  iop_assert(uri.find("http://") == 0, IOP_STATIC_STRING("Protocol must be http (no SSL)"));
  uri = uri.substr(7);

  const auto portIndex = uri.find(IOP_STATIC_STRING(":").toString());
  uint16_t port = 443;
  if (portIndex != uri.npos) {
    auto end = uri.substr(portIndex + 1).find("/");
    if (end == uri.npos) end = uri.length();
    port = static_cast<uint16_t>(strtoul(std::string(uri.begin(), portIndex + 1, end).c_str(), nullptr, 10));
    if (port == 0) {
      clientDriverLogger.error(IOP_STATIC_STRING("Unable to parse port, broken server: "), uri);
     return std::nullopt;
    }
  }
  clientDriverLogger.debug(IOP_STATIC_STRING("Port: "), std::to_string(port));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  auto end = uri.find(":");
  if (end == uri.npos) end = uri.find("/");
  if (end == uri.npos) end = uri.length();
  
  const auto host = std::string(uri.begin(), 0, end);
  // Convert IPv4 and IPv6 addresses from text to binary form
  if(inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0) {
    clientDriverLogger.error(IOP_STATIC_STRING("Address not supported: "), host);
    return std::nullopt;
  }

  int32_t connection = connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  if (connection < 0) {
    clientDriverLogger.error(IOP_STATIC_STRING("Unable to connect: "), std::to_string(connection));
    return std::nullopt;
  }
  clientDriverLogger.debug(IOP_STATIC_STRING("Began connection: "), uri);
  return Session(*this, uri, std::make_shared<int>(fd));
}
}