#include "wifi_portal.h"

#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

namespace {
Preferences prefs;
WebServer server(80);

constexpr char kPrefsNs[] = "wifi";
constexpr char kPrefsSsid[] = "5G Lab-2.4G";
constexpr char kPrefsPass[] = "penance@007";
constexpr char kPortalSsid[] = "shaws.systems";
constexpr char kPortalPass[] = "shawsetup";

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
      <button type="submit">Save & Connect</button>
      <div class="foot">AP: shaws.systems | pass: shawsetup</div>
    </form>
  </div>
</body>
</html>
)HTML";
}  // namespace

WifiPortal::WifiPortal() : apMode_(false) {}

bool WifiPortal::tryConnectSaved() {
  prefs.begin(kPrefsNs, false);
  const String ssid = prefs.getString(kPrefsSsid, "");
  const String pass = prefs.getString(kPrefsPass, "");
  prefs.end();

  if (ssid.isEmpty()) {
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 12000) {
    delay(250);
  }

  return WiFi.status() == WL_CONNECTED;
}

void WifiPortal::setupPortalRoutes() {
  server.on("/", HTTP_GET, []() { server.send(200, "text/html", kPortalPage); });

  server.on("/save", HTTP_POST, []() {
    const String ssid = server.arg("ssid");
    const String pass = server.arg("pass");

    if (ssid.isEmpty()) {
      server.send(400, "text/plain", "SSID is required");
      return;
    }

    prefs.begin(kPrefsNs, false);
    prefs.putString(kPrefsSsid, ssid);
    prefs.putString(kPrefsPass, pass);
    prefs.end();

    server.send(200, "text/plain", "Saved. Rebooting to connect...");
    delay(500);
    ESP.restart();
  });

  server.begin();
}

void WifiPortal::startPortalAp() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(kPortalSsid, kPortalPass);
  setupPortalRoutes();
  apMode_ = true;
}

void WifiPortal::begin() {
  if (tryConnectSaved()) {
    apMode_ = false;
    return;
  }

  startPortalAp();
}

void WifiPortal::handle() {
  if (apMode_) {
    server.handleClient();
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
