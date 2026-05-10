#include "weather_service.h"

#include <Arduino.h>
#include <HTTPClient.h>

namespace {
constexpr unsigned long kGeoRefreshMs = 6UL * 60UL * 60UL * 1000UL;
constexpr unsigned long kWeatherRefreshMs = 10UL * 60UL * 1000UL;
}  // namespace

WeatherService::WeatherService()
    : city_("Shillong"),
      display_("--C --"),
      latitude_(0.0f),
      longitude_(0.0f),
      hasLocation_(false),
      lastGeoMs_(0),
      lastFetchMs_(0) {}

void WeatherService::begin(const String& city) {
  setCity(city);
}

void WeatherService::setCity(const String& city) {
  if (city.isEmpty() || city == city_) {
    return;
  }
  city_ = city;
  hasLocation_ = false;
}

String WeatherService::weatherString() const { return display_; }

void WeatherService::update(bool networkAvailable) {
  if (!networkAvailable) {
    return;
  }

  const unsigned long now = millis();

  if (!hasLocation_ || now - lastGeoMs_ > kGeoRefreshMs) {
    if (!fetchGeocode_()) {
      return;
    }
  }

  if (now - lastFetchMs_ > kWeatherRefreshMs || lastFetchMs_ == 0) {
    fetchCurrent_();
  }
}

bool WeatherService::fetchGeocode_() {
  HTTPClient http;
  String url = "http://geocoding-api.open-meteo.com/v1/search?count=1&language=en&format=json&name=";
  url += city_;

  if (!http.begin(url)) {
    return false;
  }

  const int status = http.GET();
  if (status <= 0) {
    http.end();
    return false;
  }

  const String body = http.getString();
  http.end();

  const int latPos = body.indexOf("\"latitude\":");
  const int lonPos = body.indexOf("\"longitude\":");
  if (latPos < 0 || lonPos < 0) {
    return false;
  }

  const int latStart = latPos + 11;
  const int latEnd = body.indexOf(',', latStart);
  const int lonStart = lonPos + 12;
  const int lonEnd = body.indexOf(',', lonStart);

  if (latEnd < 0 || lonEnd < 0) {
    return false;
  }

  latitude_ = body.substring(latStart, latEnd).toFloat();
  longitude_ = body.substring(lonStart, lonEnd).toFloat();
  hasLocation_ = true;
  lastGeoMs_ = millis();
  return true;
}

bool WeatherService::fetchCurrent_() {
  if (!hasLocation_) {
    return false;
  }

  HTTPClient http;
  String url = "http://api.open-meteo.com/v1/forecast?current_weather=true&temperature_unit=celsius&timezone=Asia%2FKolkata";
  url += "&latitude=" + String(latitude_, 4);
  url += "&longitude=" + String(longitude_, 4);

  if (!http.begin(url)) {
    return false;
  }

  const int status = http.GET();
  if (status <= 0) {
    http.end();
    return false;
  }

  const String body = http.getString();
  http.end();

  const int cwPos = body.indexOf("\"current_weather\"");
  if (cwPos < 0) {
    return false;
  }
  const int cwStart = body.indexOf('{', cwPos);
  const int cwEnd = body.indexOf('}', cwStart);
  if (cwStart < 0 || cwEnd < 0) {
    return false;
  }

  const String cw = body.substring(cwStart + 1, cwEnd);
  const int tempPos = cw.indexOf("\"temperature\":");
  const int codePos = cw.indexOf("\"weathercode\":");
  if (tempPos < 0 || codePos < 0) {
    return false;
  }

  const int tempStart = tempPos + 14;
  const int tempEnd = cw.indexOf(',', tempStart);
  const int codeStart = codePos + 14;
  const int codeEnd = cw.indexOf(',', codeStart);
  const float temp = cw.substring(tempStart, tempEnd < 0 ? cw.length() : tempEnd).toFloat();
  const int code = cw.substring(codeStart, codeEnd < 0 ? cw.length() : codeEnd).toInt();

  display_ = String(static_cast<int>(roundf(temp))) + "C " + codeToText_(code);
  lastFetchMs_ = millis();
  return true;
}

const char* WeatherService::codeToText_(int code) const {
  switch (code) {
    case 0:
      return "Clear";
    case 1:
    case 2:
      return "Partly";
    case 3:
      return "Cloudy";
    case 45:
    case 48:
      return "Fog";
    case 51:
    case 53:
    case 55:
      return "Drizzle";
    case 61:
    case 63:
    case 65:
      return "Rain";
    case 71:
    case 73:
    case 75:
      return "Snow";
    case 80:
    case 81:
    case 82:
      return "Showers";
    case 95:
      return "Storm";
    default:
      return "Wx";
  }
}
