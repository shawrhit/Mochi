#include "time_service.h"

#include <Arduino.h>
#include <time.h>

TimeService::TimeService() : synced_(false) {}

void TimeService::begin(bool networkAvailable) {
  if (!networkAvailable) {
    return;
  }

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  trySync(true);
}

void TimeService::trySync(bool networkAvailable) {
  if (!networkAvailable || synced_) {
    return;
  }

  time_t now = 0;
  for (int i = 0; i < 15; ++i) {
    time(&now);
    if (now > 1700000000) {
      synced_ = true;
      return;
    }
    delay(200);
  }
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
