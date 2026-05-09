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

 private:
  bool apMode_;

  void setupPortalRoutes();
  void startPortalAp();
  bool tryConnectSaved();
};
