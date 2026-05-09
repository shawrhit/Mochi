#pragma once

#include <WString.h>

class BleNotifier {
 public:
  BleNotifier();

  void begin();
  bool isConnected() const;
  bool hasNewNotification() const;
  String takeLatestNotification();

  void setConnected(bool connected);
  void setLatest(const String& value);

 private:
  bool connected_;
  bool hasNew_;
  String latest_;
};
