#if !defined(IS_SIMULATOR)

#include "captive_portal.hpp"

#include "Hardware/captive_portal_html.hpp"

#include <Arduino.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

namespace {

DNSServer dns;
WebServer portalServer(80);
bool portalActive = false;

const char *portalHtml() {
  return R"(<!DOCTYPE html><html><head><meta name=viewport content="width=device-width,initial-scale=1">
<title>OMOTE WiFi Setup</title><style>body{font-family:sans-serif;max-width:400px;margin:2em auto;padding:1em}
input,button{width:100%;padding:12px;margin:8px 0;box-sizing:border-box}button{background:#3366cc;color:#fff;border:0}</style></head>
<body><h1>OMOTE WiFi</h1><p>Connect your phone to this network, then enter your home WiFi details.</p>
<form method=POST action=/save>
<label>Network name (SSID)</label><input name=ssid required autocomplete=off>
<label>Password</label><input name=password type=password autocomplete=off>
<button type=submit>Save &amp; reboot</button></form></body></html>)";
}

void handleRoot() { portalServer.send(200, "text/html", portalHtml()); }

void handleSave() {
  if (!portalServer.hasArg("ssid")) {
    portalServer.send(400, "text/plain", "Missing ssid");
    return;
  }
  Preferences preferences;
  preferences.begin("wifiSettings", false);
  preferences.putString("SSID", portalServer.arg("ssid"));
  preferences.putString("password", portalServer.arg("password"));
  preferences.end();

  portalServer.send(200, "text/html", captive_portal_html::savedPage(false));
  portalServer.client().flush();
  delay(800);
  ESP.restart();
}

void handleCaptiveRedirect() {
  portalServer.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/");
  portalServer.send(302, "text/plain", "");
}

} // namespace

namespace captive_portal {

void start(const char *apName) {
  stop();
  WiFi.disconnect(true, true);
  delay(150);
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_AP);
  delay(100);
  if (!WiFi.softAP(apName, nullptr, 6, 0, 8))
    return;

  dns.start(53, "*", WiFi.softAPIP());
  portalServer.on("/", HTTP_GET, handleRoot);
  portalServer.on("/save", HTTP_POST, handleSave);
  portalServer.on("/generate_204", HTTP_GET, handleCaptiveRedirect);
  portalServer.on("/hotspot-detect.html", HTTP_GET, handleCaptiveRedirect);
  portalServer.on("/fwlink", HTTP_GET, handleCaptiveRedirect);
  portalServer.onNotFound(handleCaptiveRedirect);
  portalServer.begin();
  portalActive = true;
}

void loop() {
  if (!portalActive)
    return;
  dns.processNextRequest();
  portalServer.handleClient();
}

void stop() {
  if (!portalActive)
    return;
  portalServer.stop();
  dns.stop();
  portalActive = false;
}

bool isActive() { return portalActive; }

const char *statusText() { return "Join WiFi: OMOTE-Setup"; }

} // namespace captive_portal

#endif // !IS_SIMULATOR
