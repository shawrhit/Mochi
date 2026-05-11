#pragma once

#include <WString.h>

class WifiPortal {
 public:
  WifiPortal();

  void begin();
  void handle();

  bool isWifiConnected() const;
  bool isApMode() const;
  String localIp() const;
  String city() const;
  void markSkipWifi();
  bool consumeSkipWifi();

 private:
  bool apMode_;
  unsigned long apStartTime_;

  void setupPortalRoutes();
  void startPortalAp();
  bool tryConnectSaved();
  String readCity_() const;
};
