#include "wifi_portal.h"

#include <Arduino.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

namespace {
Preferences prefs;
WebServer server(80);
DNSServer dnsServer;
    
constexpr char kPrefsNs[] = "wifi";
constexpr char kPrefsSsid[] = "ssid";
constexpr char kPrefsPass[] = "pass";
constexpr char kPrefsCity[] = "city";
constexpr char kPrefsSkip[] = "skip_wifi";
constexpr char kPortalSsid[] = "shaws.systems";
constexpr char kPortalPass[] = "shawsetup";
constexpr char kFallbackSsid[] = "5G Lab-2.4G";
constexpr char kFallbackPass[] = "penance@007";
constexpr int kPortalChannel = 1;
constexpr unsigned long kApRetryIntervalMs = 300000;

const char kPortalPage[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width,initial-scale=1" />
<title>shaws.systems setup</title>
<style>
  :root { --bg:#f4f6fb; --ink:#0f172a; --muted:#334155; --line:#cbd5e1; --card:#ffffff; }
  body { margin:0; font-family:ui-sans-serif,system-ui,-apple-system,Segoe UI,sans-serif; color:var(--ink); background:radial-gradient(circle at 20% 20%,#dbeafe,transparent 45%),radial-gradient(circle at 80% 30%,#cffafe,transparent 40%),var(--bg); }
  .wrap { min-height:100vh; display:grid; place-items:center; padding:20px; }
  .card { width:min(450px,95vw); background:var(--card); border:1px solid var(--line); border-radius:20px; box-shadow:0 20px 35px rgba(15,23,42,.08); padding:22px; }
  h1 { margin:0 0 8px; font-size:1.35rem; }
  p { margin:0 0 18px; color:var(--muted); }
  label { display:block; font-size:.9rem; margin:.75rem 0 .35rem; color:#1e293b; }
  input { width:100%; box-sizing:border-box; border:1px solid #cbd5e1; border-radius:12px; padding:.75rem .8rem; font-size:1rem; }
  button { margin-top:14px; width:100%; border:0; border-radius:12px; padding:.85rem 1rem; font-weight:700; color:#fff; background:linear-gradient(135deg,#0284c7,#0ea5e9); }
  .foot { margin-top:12px; font-size:.82rem; color:#64748b; text-align:center; }
</style>
</head>
<body>
  <div class="wrap">
    <form class="card" method="POST" action="/save">
      <h1>Connect shaws.systems</h1>
      <p>Enter your WiFi credentials and the device will restart into station mode.</p>
      <label>WiFi SSID</label>
      <input name="ssid" required maxlength="64" />
      <label>WiFi Password</label>
      <input name="pass" type="password" maxlength="64" />
      <label>City</label>
      <input name="city" maxlength="64" placeholder="Shillong" />
      <button type="submit">Save & Connect</button>
      <button type="submit" formaction="/skip">Skip WiFi For Now</button>
      <div class="foot">AP: shaws.systems | pass: shawsetup</div>
    </form>
  </div>
</body>
</html>
)HTML";
}  // namespace

WifiPortal::WifiPortal() : apMode_(false), apStartTime_(0), lastRetryMs_(0) {}

bool WifiPortal::tryConnectSaved() {
  prefs.begin(kPrefsNs, false);
  String ssid = "";
  String pass = "";
  if (prefs.isKey(kPrefsSsid)) {
    ssid = prefs.getString(kPrefsSsid, "");
  }
  if (prefs.isKey(kPrefsPass)) {
    pass = prefs.getString(kPrefsPass, "");
  }
  prefs.end();

  if (ssid.isEmpty()) {
    ssid = kFallbackSsid;
    pass = kFallbackPass;
    Serial.println("WiFi: no saved SSID, trying fallback");
  }

  Serial.print("WiFi: trying SSID ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  // ESP32-C3 workaround: keep TX power at 8.5 dBm (DO NOT change).
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.begin(ssid.c_str(), pass.c_str());

  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 12000) {
    delay(250);
  }

  const bool ok = WiFi.status() == WL_CONNECTED;
  Serial.println(ok ? "WiFi: connected" : "WiFi: connect failed");
  return ok;
}

void WifiPortal::setupPortalRoutes() {
  server.on("/", HTTP_GET, []() { server.send(200, "text/html", kPortalPage); });

  server.on("/save", HTTP_POST, []() {
    const String ssid = server.arg("ssid");
    const String pass = server.arg("pass");
    const String city = server.arg("city");

    if (ssid.isEmpty()) {
      server.send(400, "text/plain", "SSID is required");
      return;
    }

    prefs.begin(kPrefsNs, false);
    prefs.putString(kPrefsSsid, ssid);
    prefs.putString(kPrefsPass, pass);
    if (!city.isEmpty()) {
      prefs.putString(kPrefsCity, city);
    }
    prefs.putBool(kPrefsSkip, false);
    prefs.end();

    server.send(200, "text/plain", "Saved. Rebooting to connect...");
    delay(500);
    ESP.restart();
  });

  server.on("/skip", HTTP_POST, []() {
    prefs.begin(kPrefsNs, false);
    prefs.putBool(kPrefsSkip, true);
    prefs.end();

    server.send(200, "text/plain", "WiFi skipped. Rebooting offline...");
    delay(500);
    ESP.restart();
  });

  server.onNotFound([]() {
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
    server.send(302, "text/plain", "");
  });

  server.begin();
}

void WifiPortal::startPortalAp() {
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  // ESP32-C3 workaround: keep TX power at 8.5 dBm (DO NOT change).
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  const bool apOk = WiFi.softAP(kPortalSsid, kPortalPass, kPortalChannel, false, 4);
  dnsServer.start(53, "*", WiFi.softAPIP());
  Serial.print("WiFi AP: ");
  Serial.print(kPortalSsid);
  Serial.print(" pass: ");
  Serial.print(kPortalPass);
  Serial.print(" ch: ");
  Serial.print(kPortalChannel);
  Serial.print(" ip: ");
  Serial.println(WiFi.softAPIP());
  Serial.println(apOk ? "WiFi AP: started" : "WiFi AP: failed to start");
  setupPortalRoutes();
  apMode_ = true;
  apStartTime_ = millis();
  lastRetryMs_ = apStartTime_;
}

void WifiPortal::begin() {
  if (consumeSkipWifi()) {
    WiFi.mode(WIFI_OFF);
    apMode_ = false;
    return;
  }

  if (tryConnectSaved()) {
    apMode_ = false;
    return;
  }

  startPortalAp();
}

void WifiPortal::handle() {
  if (apMode_) {
    dnsServer.processNextRequest();
    server.handleClient();
    const unsigned long now = millis();
    if (WiFi.softAPgetStationNum() > 0) {
      lastRetryMs_ = now;
      return;
    }

    if (now - lastRetryMs_ >= kApRetryIntervalMs) {
      Serial.println("WiFi: AP retry window. Trying STA connect...");
      lastRetryMs_ = now;
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_OFF);
      delay(200);
      if (tryConnectSaved()) {
        apMode_ = false;
        Serial.println("WiFi: STA connected. AP stopped.");
      } else {
        Serial.println("WiFi: STA retry failed. Restarting AP.");
        startPortalAp();
      }
    }
  }
}

bool WifiPortal::isWifiConnected() const { return WiFi.status() == WL_CONNECTED; }

bool WifiPortal::isApMode() const { return apMode_; }

String WifiPortal::localIp() const {
  if (apMode_) {
    return WiFi.softAPIP().toString();
  }
  return WiFi.localIP().toString();
}

String WifiPortal::city() const { return readCity_(); }

void WifiPortal::markSkipWifi() {
  prefs.begin(kPrefsNs, false);
  prefs.putBool(kPrefsSkip, true);
  prefs.end();
}

bool WifiPortal::consumeSkipWifi() {
  prefs.begin(kPrefsNs, false);
  const bool skip = prefs.getBool(kPrefsSkip, false);
  if (skip) {
    prefs.putBool(kPrefsSkip, false);
  }
  prefs.end();
  return skip;
}

String WifiPortal::readCity_() const {
  prefs.begin(kPrefsNs, false);
  String city = "Shillong";
  if (prefs.isKey(kPrefsCity)) {
    city = prefs.getString(kPrefsCity, "Shillong");
  }
  prefs.end();
  return city;
}
