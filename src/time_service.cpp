#include "time_service.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <sys/time.h>
#include <time.h>

namespace {
unsigned long parseEpochFromJson(const String& body) {
  const int pos = body.indexOf("\"unixtime\":");
  if (pos < 0) {
    return 0;
  }

  const int start = pos + 11;
  const int end = body.indexOf(',', start);
  return body.substring(start, end < 0 ? body.length() : end).toInt();
}
}  // namespace

TimeService::TimeService() : synced_(false), tzSpec_("UTC0"), lastAttemptMs_(0), attemptCount_(0) {}

void TimeService::setTimezone(const char* tzSpec) {
  if (tzSpec != nullptr) {
    tzSpec_ = tzSpec;
  }
}

void TimeService::begin(bool networkAvailable) {
  if (!networkAvailable) {
    Serial.println("Time: network unavailable, skipping NTP setup");
    return;
  }

  setenv("TZ", tzSpec_, 1);
  tzset();
  Serial.print("Time: TZ set to ");
  Serial.println(tzSpec_);
  Serial.println("Time: HTTP-only mode (no NTP)");
  lastAttemptMs_ = 0;
  attemptCount_ = 0;
  trySync(true);
}

void TimeService::trySync(bool networkAvailable) {
  if (!networkAvailable || synced_) {
    return;
  }

  const unsigned long nowMs = millis();
  if (nowMs - lastAttemptMs_ < 5000) {
    return;
  }
  lastAttemptMs_ = nowMs;
  ++attemptCount_;

  Serial.print("Time: HTTP sync attempt ");
  Serial.println(attemptCount_);
  Serial.print("Time: IP ");
  Serial.print(WiFi.localIP());
  Serial.print(" gw ");
  Serial.print(WiFi.gatewayIP());
  Serial.print(" dns ");
  Serial.println(WiFi.dnsIP());

  IPAddress resolved;
  if (WiFi.hostByName("worldtimeapi.org", resolved)) {
    Serial.print("Time: DNS worldtimeapi.org -> ");
    Serial.println(resolved);
  } else {
    Serial.println("Time: DNS worldtimeapi.org failed");
  }

  const char* url = "https://time.now/developer/api/timezone/Asia/Kolkata";
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(8000);
  if (!http.begin(client, url)) {
    Serial.print("Time: HTTP begin failed ");
    Serial.println(url);
    Serial.println("Time: HTTP sync failed");
    return;
  }

  const int status = http.GET();
  if (status > 0) {
    const String body = http.getString();
    const unsigned long epoch = parseEpochFromJson(body);
    if (epoch > 1700000000) {
      struct timeval tv;
      tv.tv_sec = epoch;
      tv.tv_usec = 0;
      settimeofday(&tv, nullptr);
      synced_ = true;
      Serial.print("Time: HTTP synced epoch ");
      Serial.println(epoch);
      http.end();
      return;
    }
    Serial.println("Time: HTTP response missing time field");
  } else {
    Serial.print("Time: HTTP error ");
    Serial.print(status);
    Serial.print(" for ");
    Serial.println(url);
    Serial.println(http.errorToString(status));
  }
  http.end();

  Serial.println("Time: HTTP sync failed");
}

bool TimeService::isSynced() const { return synced_; }

String TimeService::timeString() const {
  time_t now;
  time(&now);

  struct tm ti;
  localtime_r(&now, &ti);

  char buf[10];
  strftime(buf, sizeof(buf), "%H:%M:%S", &ti);
  return String(buf);
}

String TimeService::dateString() const {
  time_t now;
  time(&now);

  struct tm ti;
  localtime_r(&now, &ti);

  char buf[24];
  strftime(buf, sizeof(buf), "%d %b %Y", &ti);
  return String(buf);
}

String TimeService::dayString() const {
  time_t now;
  time(&now);

  struct tm ti;
  localtime_r(&now, &ti);

  char buf[8];
  strftime(buf, sizeof(buf), "%a", &ti);
  return String(buf);
}

String TimeService::dateShortString() const {
  time_t now;
  time(&now);

  struct tm ti;
  localtime_r(&now, &ti);

  char buf[12];
  strftime(buf, sizeof(buf), "%d %b", &ti);
  return String(buf);
}

String TimeService::yearString() const {
  time_t now;
  time(&now);

  struct tm ti;
  localtime_r(&now, &ti);

  char buf[6];
  strftime(buf, sizeof(buf), "%Y", &ti);
  return String(buf);
}

int TimeService::hour24() const {
  time_t now;
  time(&now);

  struct tm ti;
  localtime_r(&now, &ti);
  return ti.tm_hour;
}
