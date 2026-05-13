#pragma once

#include <WString.h>

class TimeService {
 public:
  TimeService();

  void begin(bool networkAvailable);
  void trySync(bool networkAvailable);
  void setTimezone(const char* tzSpec);
  bool isSynced() const;

  void set24Hour(bool is24Hour);
  bool is24Hour() const;

  String timeString() const;
  String ampmString() const;
  String dateString() const;
  String dayString() const;
  String dateShortString() const;
  String yearString() const;
  int hour24() const;

 private:
  bool is24Hour_;
  bool synced_;
  const char* tzSpec_;
  unsigned long lastAttemptMs_;
  uint8_t attemptCount_;
};
