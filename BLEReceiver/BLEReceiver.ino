#include "heltec.h"
#include "images.h"
#include <HTTPClient.h>

// Credentials
String deviceName = "Gateway_1";

// Enables
int BT_enable = 0;
int lora_enable = 0;

// LoRa
#include <stdio.h>
#include <string.h>
#define BAND    915E6  //you can set band here directly,e.g. 868E6,915E6

// LoRA Packet Formatting
String packet;
int newConnect = 0;
unsigned long relativeTime = 0;
double resistance[8];
double deltaResistance[8];
double temperature = 0;
double humidity = 0;
double ppm = 0;

// BLE Transmission
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// BLE Transmission
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      BLEDevice::startAdvertising();
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void setup() {
  // Initialize device
  Serial.begin(115200);
  Heltec.begin(true, true , true, true , BAND); // display, lora, serial, paboost, band
  Heltec.display->flipScreenVertically();

  // Start receieving transmission and update UI screen
  Heltec.display->clear();
  Heltec.display->drawString(0, 30, "Scanning for LoRA+BLE");
  Heltec.display->display();

  const char* json = deviceName.c_str();
  BLEDevice::init(json);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID,BLECharacteristic::PROPERTY_READ|BLECharacteristic::PROPERTY_WRITE|BLECharacteristic::PROPERTY_NOTIFY|BLECharacteristic::PROPERTY_INDICATE);
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Heltec.display->clear();
  Heltec.display->drawXbm(0, 5, uOttawa_Logo_width, uOttawa_Logo_height, uOttawa_Logo_bits);
  Heltec.display->display();
  delay(2000);
  // Start receieving transmission and update UI screen
  LoRa.setSpreadingFactor(7);
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "Success!");
  Heltec.display->drawString(0, 30, "Scanning for LoRA...");
  Heltec.display->display();
}

/**
 * Returns a substring of a mainstring based on index number and the seperator
 * E.x A1/B2/C3    If using a "/" seperator, the 2nd term would be "B2".
 */
String getValue(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;
    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

/**
 * Displays variables to Heltec OLED Screen
 */
void displayData() {
  int x = 0; int y = 0;
  Heltec.display->drawXbm(x, y, Bluetooth_on_width, Bluetooth_on_height, Bluetooth_on_bits);
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(x+15, y, deviceName);
  Heltec.display->drawString(x, y + 10, "T=" + double2string(temperature,1) + "°C");
  Heltec.display->drawString(x+65, y + 10, "RH=" + double2string(humidity,0) + "%");
  Heltec.display->drawString(x, y + 20, "R1:" + double2string(resistance[0],2));
  Heltec.display->drawString(x, y + 30, "R2:" + double2string(resistance[1],2));
  Heltec.display->drawString(x, y + 40, "R3:" + double2string(resistance[2],2));
  Heltec.display->drawString(x, y + 50, "R4:" + double2string(resistance[3],2));
  Heltec.display->drawString(x+70, y + 20, "[" + double2string(deltaResistance[0],2) + "]");
  Heltec.display->drawString(x+70, y + 30, "[" + double2string(deltaResistance[1],2) + "]");
  Heltec.display->drawString(x+70, y + 40, "[" + double2string(deltaResistance[2],2) + "]");
  Heltec.display->drawString(x+70, y + 50, "[" + double2string(deltaResistance[3],2) + "]");
}

/**
 * Main Loop
 */
String previousRaw;
void loop() {
  delay(100);
  int packetSize = Heltec.LoRa.parsePacket();
  if (packetSize) {
    while (Heltec.LoRa.available()) {
      Heltec.display->clear();
      displayData();
      Heltec.display->display();
      String packetRaw = Heltec.LoRa.readString();
      if (previousRaw != packetRaw) {
        previousRaw = packetRaw;
        updateVariables(packetRaw);
        transmitBLEPacket();
      }
    }
  }
}

/**
 * LORA
 * Retrieve LoRa transmission and store values to local variables
 * Decodes the LoRa transmission packet using delimited "/"
 */
void updateVariables(String packet) {
      String tempVal = getValue(packet,'/',0);      
      tempVal.remove(0,1);
      deviceName = tempVal;
      
      tempVal = getValue(packet,'/',1);
      tempVal.remove(0,1);
      relativeTime = tempVal.toInt();

      tempVal = getValue(packet,'/',2);
      tempVal.remove(0,1);
      temperature = tempVal.toDouble();

      tempVal = getValue(packet,'/',3);
      tempVal.remove(0,1);
      humidity = tempVal.toDouble();

      tempVal = getValue(packet,'/',4);
      tempVal.remove(0,1);
      ppm = tempVal.toDouble();

      tempVal = getValue(packet,'/',5);
      tempVal.remove(0,1);
      resistance[0] = tempVal.toDouble();
      
      tempVal = getValue(packet,'/',6);
      tempVal.remove(0,1);
      deltaResistance[0] = tempVal.toDouble();

      tempVal = getValue(packet,'/',7);
      tempVal.remove(0,1);
      resistance[1] = tempVal.toDouble();
      
      tempVal = getValue(packet,'/',8);
      tempVal.remove(0,1);
      deltaResistance[1] = tempVal.toDouble();

      tempVal = getValue(packet,'/',9);
      tempVal.remove(0,1);
      resistance[2] = tempVal.toDouble();
      
      tempVal = getValue(packet,'/',10);
      tempVal.remove(0,1);
      deltaResistance[2] = tempVal.toDouble();

      tempVal = getValue(packet,'/',11);
      tempVal.remove(0,1);
      resistance[3] = tempVal.toDouble();
      
      tempVal = getValue(packet,'/',12);
      tempVal.remove(0,1);
      deltaResistance[3] = tempVal.toDouble();
}

/**
 * DEPRACATED - DO NOT USE.  LORA will not function with Bluetooth simultaneously
 * Transmits data as broadcast on Bluetooth Low Energy Channel
 */
void transmitBLEPacket() {
  if (deviceConnected) {
      long relativeTime = millis();
      String dataPacket = "";
      dataPacket = dataPacket + "A" + (String)deviceName + "/";
      dataPacket = dataPacket + "B" + (String)relativeTime + "/";
      dataPacket = dataPacket + "C" + (String)temperature + "/";
      dataPacket = dataPacket + "D" + (String)humidity + "/";
      dataPacket = dataPacket + "E" + (String)ppm + "/";
      dataPacket = dataPacket + "F" + (String)resistance[0] + "/";
      dataPacket = dataPacket + "G" + (String)deltaResistance[0] + "/";
      dataPacket = dataPacket + "H" + (String)resistance[1] + "/";
      dataPacket = dataPacket + "I" + (String)deltaResistance[1] + "/";
      dataPacket = dataPacket + "J" + (String)resistance[2] + "/";
      dataPacket = dataPacket + "K" + (String)deltaResistance[2] + "/";
      dataPacket = dataPacket + "L" + (String)resistance[3] + "/";
      dataPacket = dataPacket + "M" + (String)deltaResistance[3];
      String str = dataPacket;
      int str_len = str.length() + 1; 
      char char_array[str_len];
      str.toCharArray(char_array, str_len);
      pCharacteristic->setValue(char_array);
      pCharacteristic->notify();
      delay(30); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
  }
  if (!deviceConnected && oldDeviceConnected) { // disconnecting
      delay(100); // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // restart advertising
      Serial.println("start advertising");
      oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) { // connecting
      oldDeviceConnected = deviceConnected;
  }
}

/**
 * Convert a double to a string with a specified number of decimal places
 */
String double2string(double n, int ndec) {
    int nMultiplier = 1;
    if (n < 0) {
      nMultiplier = -1;
      n = -n;
    }
    if (ndec == 0) {
      int nn = round(n)*nMultiplier;
      return String(nn);
    }
    String r = "";
    int v = n;
    r += v;     // whole number part
    r += '.';   // decimal point
    int i;
    for (i=0;i<ndec;i++) {
        // iterate through each decimal digit for 0..ndec 
        n -= v;
        n *= 10; 
        v = n;
        r += v;
    }
    if (nMultiplier == -1) {return "-"+r;}
    else {return r;}
}
