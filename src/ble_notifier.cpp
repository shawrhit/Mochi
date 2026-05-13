#include "ble_notifier.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <esp_gap_ble_api.h>

namespace {
BleNotifier* gNotifier = nullptr;

constexpr const char* kDeviceName = "Mochi";
constexpr const char* kServiceUuid = "c3ad1001-8f37-46a1-9a3a-f4f10eec9001";
constexpr const char* kTextCharUuid = "c3ad1002-8f37-46a1-9a3a-f4f10eec9001";

class NotifierServerCallbacks : public BLEServerCallbacks {
 public:
  void onConnect(BLEServer* pServer) override {
    (void)pServer;
    if (gNotifier != nullptr) {
      gNotifier->setConnected(true);
    }
  }

  void onDisconnect(BLEServer* pServer) override {
    (void)pServer;
    if (gNotifier != nullptr) {
      gNotifier->setConnected(false);
    }
    BLEDevice::startAdvertising();
  }
};

class NotifierTextCallbacks : public BLECharacteristicCallbacks {
 public:
  void onWrite(BLECharacteristic* pChar) override {
    if (gNotifier == nullptr || pChar == nullptr) {
      return;
    }

    const std::string value = pChar->getValue();
    if (value.empty()) {
      return;
    }

    gNotifier->setLatest(String(value.c_str()));
  }
};

void clearBondedDevices_() {
  int count = esp_ble_get_bond_device_num();
  if (count <= 0) {
    return;
  }

  esp_ble_bond_dev_t* list = static_cast<esp_ble_bond_dev_t*>(malloc(sizeof(esp_ble_bond_dev_t) * count));
  if (!list) {
    return;
  }

  if (esp_ble_get_bond_device_list(&count, list) == ESP_OK) {
    for (int i = 0; i < count; ++i) {
      esp_ble_remove_bond_device(list[i].bd_addr);
    }
  }

  free(list);
}
}  // namespace

BleNotifier::BleNotifier() : connected_(false), hasNew_(false), latest_("") {}

void BleNotifier::begin() {
  gNotifier = this;
  clearBondedDevices_();
  BLEDevice::init(kDeviceName);

  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new NotifierServerCallbacks());

  BLEService* service = server->createService(kServiceUuid);
  BLECharacteristic* textChar = service->createCharacteristic(
      kTextCharUuid, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  textChar->setValue("ready");
  textChar->setCallbacks(new NotifierTextCallbacks());

  service->start();
  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(kServiceUuid);
  advertising->start();
}

bool BleNotifier::isConnected() const { return connected_; }

bool BleNotifier::hasNewNotification() const { return hasNew_; }

String BleNotifier::takeLatestNotification() {
  hasNew_ = false;
  return latest_;
}

void BleNotifier::setConnected(bool connected) { connected_ = connected; }

void BleNotifier::setLatest(const String& value) {
  latest_ = value;
  hasNew_ = true;
}
