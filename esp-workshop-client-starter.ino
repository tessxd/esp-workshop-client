/**
 * Adapted from BLE Client Example
 */

#include "BLEDevice.h"
#include <driver/rtc_io.h>

// Define UUIDs:
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID  charUUID_1("beb5483e-36e1-4688-b7f5-ea07361b26a8");

// Some variables to keep track on device connected
static boolean doConnect = false;
static boolean connected = false;

// Define pointer for the BLE connection
static BLEAdvertisedDevice* myDevice;
BLERemoteCharacteristic* pRemoteChar_1;
BLEClient*  pClient;

// Sleep logic
#define SLEEP_TIME 10 * 1000000 // 10 seconds
void deepSleep() {
  esp_sleep_enable_timer_wakeup(SLEEP_TIME);
  gpio_deep_sleep_hold_dis();
  esp_deep_sleep_start();
}

// Callback function that is called whenever a client is connected or disconnected
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    deepSleep(); // go to sleep
  }
};

// Function that is run whenever the server is connected
bool connectToServer() {
  Serial.println(myDevice->getAddress().toString().c_str());

  pClient  = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(myDevice);

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    pClient->disconnect();
    return false;
  }

  connected = true;
  pRemoteChar_1 = pRemoteService->getCharacteristic(charUUID_1);
  if(pRemoteChar_1 == nullptr) {
    pClient-> disconnect();
    return false;
  }
  return true;
}

// Scan for BLE servers and find the first one that advertises the service we are looking for.
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  //Called for each advertising BLE server.
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

void setup() {
  BLEDevice::init("Your number here");
  Serial.begin(115200);

  // Turn LED off on esp for sleep
  pinMode(LED_BUILTIN, OUTPUT);
  gpio_hold_dis(GPIO_NUM_2);
  digitalWrite(LED_BUILTIN, LOW);
  gpio_hold_en(GPIO_NUM_2);

  // Scan for server
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {

  if (doConnect == true) {
    connectToServer();
  }

  if (connected) {
    // Read from sensors

    // Format readings into: "Name,sensor,sensor_reading,sensor1,sensor1_readingg"
    // Example
    String sensor_readings = "Christopher,temp,72.1";
    // Can use .concat() to append values to string
    sensor_readings.concat(",light,");
    sensor_readings.concat(0.78);
    Serial.println(sensor_readings);

    std::string str = sensor_readings.c_str();
    pClient->setMTU(512);

    pRemoteChar_1->writeValue((uint8_t*)str.c_str(), str.length());

    delay(5000);
    deepSleep();
  }
}%