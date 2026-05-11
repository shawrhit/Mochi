#pragma once

#include <WString.h>

class TimeService {
 public:
  TimeService();

  void begin(bool networkAvailable);
  void trySync(bool networkAvailable);
  void setTimezone(const char* tzSpec);
  bool isSynced() const;

  String timeString() const;
  String dateString() const;

 private:
  bool synced_;
  const char* tzSpec_;
  unsigned long lastAttemptMs_;
  uint8_t attemptCount_;
};
