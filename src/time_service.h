#pragma once

#include <WString.h>

class TimeService {
 public:
  TimeService();

  void begin(bool networkAvailable);
  void trySync(bool networkAvailable);
  bool isSynced() const;

  String timeString() const;
  String dateString() const;

 private:
  bool synced_;
};
