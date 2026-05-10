#pragma once

#include <WString.h>

class WeatherService {
 public:
  WeatherService();

  void begin(const String& city);
  void setCity(const String& city);
  void update(bool networkAvailable);
  String weatherString() const;

 private:
  bool fetchGeocode_();
  bool fetchCurrent_();
  const char* codeToText_(int code) const;

  String city_;
  String display_;
  float latitude_;
  float longitude_;
  bool hasLocation_;
  unsigned long lastGeoMs_;
  unsigned long lastFetchMs_;
};
