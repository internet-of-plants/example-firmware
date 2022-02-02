#include "server.hpp"

#ifndef IOP_SERVER_DISABLED
#include "driver/server.hpp"
#include "driver/wifi.hpp"
#include "driver/thread.hpp"

#include "configuration.hpp"
#include "driver/network.hpp"
#include "api.hpp"
#include "driver/device.hpp"
#include "configuration.hpp"
#include "loop.hpp"

auto pageHTMLStart() -> iop::StaticString {
  return IOP_STATIC_STRING(
    "<!DOCTYPE HTML>\r\n"
    "<html><body>\r\n"
    "  <h1><center>Hello, I'm your plantomator</center></h1>\r\n"
    "  <h4><center>If, in the future, you want to reset the "
    "configurations set here, just press the factory reset button "
    "for at least 15 seconds</center></h4>"
    "<form style='margin: 0 auto; width: 500px;' action='/submit' "
    "method='POST'>\r\n");
}

auto wifiOverwriteHTML() -> iop::StaticString {
  return IOP_STATIC_STRING(
    "<h3>"
    "  <center>It seems you already have your wifi credentials set, if you "
    "want to rewrite it, please set the checkbox below and fill the "
    "fields. Otherwise they will be ignored</center>"
    "</h3>\r\n"
    "<div>"
    "  <input type='checkbox' name='wifi'>"
    "  <label for='wifi'>Overwrite wifi credentials</label>"
    "</div>"
    "<div class=\"wifi\" style=\"display: none\">"
    "  <div><strong>Network name:</strong></div>"
    "  <input name='ssid' type='text' style='width:100%' />"
    "</div>\r\n"
    "<div class=\"wifi\" style=\"display: none\">"
    "  <div><strong>Password:</strong></div>"
    "  <input name='password' type='password' style='width:100%' />"
    "</div>\r\n");
}

auto wifiHTML() -> iop::StaticString {
  return IOP_STATIC_STRING(
    "<h3><center>"
    "Please provide your Wifi credentials, so we can connect to it."
    "</center></h3>\r\n"
    "<div><input type='hidden' value='true' name='wifi'></div>"
    "<div>"
    "  <div><strong>Network name:</strong></div>"
    "  <input name='ssid' type='text' style='width:100%' />"
    "</div>\r\n"
    "<div><div><strong>Password:</strong></div>"
    "<input name='password' type='password' style='width:100%' /></div>\r\n");
}

auto iopOverwriteHTML() -> iop::StaticString {
  return IOP_STATIC_STRING(
    "<h3><center>It seems you already have your Iop credentials set, if you "
    "want to rewrite it, please set the checkbox below and fill the "
    "fields. Otherwise they will be ignored</center></h3>\r\n"
    "<div>"
    "  <input type='checkbox' name='iop'>"
    "  <label for='iop'>Overwrite Iop credentials</label>"
    "</div>"
    "<div class=\"iop\" style=\"display: 'none'\">"
    "  <div><strong>Email:</strong></div>"
    "  <input name='iopEmail' type='text' style='width:100%' />"
    "</div>\r\n"
    "<div class=\"iop\" style=\"display: 'none'\">"
    "  <div><strong>Password:</strong></div>"
    "  <input name='iopPassword' type='password' style='width:100%' />"
    "</div>\r\n");
}

auto iopHTML() -> iop::StaticString {
  return IOP_STATIC_STRING(
    "<h3><center>Please provide your Iop credentials, so we can get an "
    "authentication token to use</center></h3>\r\n"
    "<div>"
    "  <input type='hidden' value='true' name='iop'></div>"
    "<div>"
    "  <div><strong>Email:</strong></div>"
    "  <input name='iopEmail' type='text' style='width:100%' />"
    "</div>\r\n"
    "<div>"
    "  <div><strong>Password:</strong></div>"
    "  <input name='iopPassword' type='password' style='width:100%' />"
    "</div>\r\n");
}

auto script() -> iop::StaticString {
  return IOP_STATIC_STRING(
    "<script type='application/javascript'>"
    "document.querySelector(\"input[name='wifi']\").addEventListener('change', ev => {"
    "  for (const el of document.getElementsByClassName('wifi')) {"
    "    if (ev.currentTarget.checked) {"
    "      el.style.display = 'block';"
    "    } else {"
    "      el.style.display = 'none';"
    "    }"
    "  }"
    "});"
    "document.querySelector(\"input[name='iop']\").addEventListener('change', ev => {"
    "  for (const el of document.getElementsByClassName('iop')) {"
    "    if (ev.currentTarget.checked) {"
    "      el.style.display = 'block';"
    "    } else {"
    "      el.style.display = 'none';"
    "    }"
    "  }"
    "});"
    "</script>");
}

auto pageHTMLEnd() -> iop::StaticString {
  return IOP_STATIC_STRING(
    "<br>\r\n"
    "<input type='submit' value='Submit' />\r\n"
    "</form></body></html>");
}

// We use this globals to share messages from the callbacks
static std::optional<std::pair<std::string, std::string>> credentialsWifi;
static std::optional<std::pair<std::string, std::string>> credentialsIop;

static driver::HttpServer server;
static driver::CaptivePortal dnsServer;

void CredentialsServer::setup() const noexcept {
  IOP_TRACE();
  server.on(IOP_STATIC_STRING("/favicon.ico"), [](driver::HttpConnection &conn, iop::Log const &logger) { conn.send(404, IOP_STATIC_STRING("text/plain"), IOP_STATIC_STRING("")); (void) logger; });
  server.on(IOP_STATIC_STRING("/submit"), [](driver::HttpConnection &conn, iop::Log const &logger) {
    IOP_TRACE();
    logger.debug(IOP_STATIC_STRING("Received credentials form"));

    const auto wifi = conn.arg(IOP_STATIC_STRING("wifi"));
    const auto ssid = conn.arg(IOP_STATIC_STRING("ssid"));
    const auto psk = conn.arg(IOP_STATIC_STRING("password"));
    if (wifi && ssid && psk) {
      logger.debug(IOP_STATIC_STRING("SSID: "), *ssid);
      credentialsWifi = std::make_pair(*ssid, *psk);
    }

    const auto iop = conn.arg(IOP_STATIC_STRING("iop"));
    const auto email = conn.arg(IOP_STATIC_STRING("iopEmail"));
    const auto password = conn.arg(IOP_STATIC_STRING("iopPassword"));
    if (iop && email && password) {
      logger.debug(IOP_STATIC_STRING("Email: "), *email);

      credentialsIop = std::make_pair(*email, *password);
    }

    conn.sendHeader(IOP_STATIC_STRING("Location"), IOP_STATIC_STRING("/"));
    conn.send(302, IOP_STATIC_STRING("text/plain"), IOP_STATIC_STRING(""));
  });

  server.onNotFound([](driver::HttpConnection &conn, iop::Log const &logger) {
    IOP_TRACE();
    logger.info(IOP_STATIC_STRING("Serving captive portal"));

    const auto mustConnect = !iop::Network::isConnected();
    const auto needsIopAuth = !eventLoop.storage().token();

    auto len = pageHTMLStart().length() + pageHTMLEnd().length() + script().length();
    len += mustConnect ? wifiHTML().length() : wifiOverwriteHTML().length();
    len += needsIopAuth ? iopHTML().length() : iopOverwriteHTML().length();

    conn.setContentLength(len);
    conn.send(200, IOP_STATIC_STRING("text/html"), pageHTMLStart());

    if (mustConnect) conn.sendData(wifiHTML());
    else conn.sendData(wifiOverwriteHTML());

    if (needsIopAuth) conn.sendData(iopHTML());
    else conn.sendData(iopOverwriteHTML());

    conn.sendData(script());
    conn.sendData(pageHTMLEnd());
    logger.debug(IOP_STATIC_STRING("Served HTML"));
  });
}

void CredentialsServer::start() noexcept {
  IOP_TRACE();
  if (!this->isServerOpen) {
    this->isServerOpen = true;
    this->logger.info(IOP_STATIC_STRING("Setting our own wifi access point"));

    // TODO: how to mock it in a reasonable way?
    iop::data.wifi.setMode(driver::WiFiMode::AP_STA);
    driver::thisThread.sleep(1);
    iop::data.wifi.setupAP();

    {
      const auto hash = iop::hashString(iop::to_view(driver::device.macAddress()));
      const auto ssid = std::string("iop-") + std::to_string(hash);

      iop::data.wifi.connectAP(ssid, IOP_STATIC_STRING("le$memester#passwordz").toString());
    }
    
    const auto ip = iop::data.wifi.APIP();
    
    server.begin();

    // Makes it a captive portal (redirects all wifi trafic to it)
    dnsServer.start();

    this->logger.info(IOP_STATIC_STRING("Opened captive portal: "), iop::to_view(ip));
  }
}

void CredentialsServer::close() noexcept {
  IOP_TRACE();
  if (this->isServerOpen) {
    this->logger.debug(IOP_STATIC_STRING("Closing captive portal"));
    this->isServerOpen = false;
    dnsServer.close();
    server.close();

    iop::data.wifi.setMode(driver::WiFiMode::STA);
    driver::thisThread.sleep(1);
  }
}

auto CredentialsServer::serve(const Api &api) noexcept
    -> std::optional<AuthToken> {
  IOP_TRACE();
  this->start();

  // The user provided those informations through the web form
  // But we shouldn't act on it inside the server's callback, as callback
  // should be rather simple, so we use globals. UNWRAP moves them out on use.

  const auto isConnected = iop::Network::isConnected();

  if (credentialsWifi) {
    eventLoop.connect(credentialsWifi->first, credentialsWifi->second);
  }
  
  if (isConnected && credentialsIop) {
    auto tok = eventLoop.authenticate(credentialsIop->first, credentialsIop->second, api);
    if (tok)
      return tok;

    // WiFi Credentials stored in storage

    // Wifi connection errors are hard to debug
    // (see countless open issues about it at esp8266/Arduino's github)
    // One example citing others:
    // https://github.com/esp8266/Arduino/issues/7432
    //
    // So we never delete a storage stored wifi credentials (outside of factory
    // reset), we have a timer to avoid constantly retrying a bad credential.
  }


  // Give processing time to the servers
  this->logger.trace(IOP_STATIC_STRING("Serve captive portal"));
  dnsServer.handleClient();
  server.handleClient();
  return std::nullopt;
}
#else
auto CredentialsServer::serve(const Api &api) noexcept
    -> std::optional<AuthToken> {
  IOP_TRACE();
  (void)this;
  (void)api;
  return std::optional<AuthToken>();
}
void CredentialsServer::setup() const noexcept {
  (void)*this;
  IOP_TRACE();
}
void CredentialsServer::close() noexcept {
  (void)*this;
  IOP_TRACE();
}
void CredentialsServer::start() noexcept {
  (void)*this;
  IOP_TRACE();
}
#endif
