# ESP32 Sensor Array
ESP32 Sensor Array is a network of ESP32 microcontrollers that relay high-speed sensor data to the internet or through Bluetooth to your phone

# Features
* 3km+ transmission distance between transcievers
* Auto-upload sensor data to Google Sheets (WiFi mode)
* Broadcast sensor data to your phone (BLE mode)
\
\
Demo: https://bit.ly/38d89ue
Phone App: https://bit.ly/3moTwZK

# BLEReciever.ino
Specify a device name
```C++
// Credentials
String deviceName = "Gateway_1";
```
Set LoRa band to 915MHz (North American open band)
```C++
// LoRa
#include <stdio.h>
#include <string.h>
#define BAND    915E6  //you can set band here directly,e.g. 868E6,915E6
```

Set the Service UUID and Characteristic UUID.  If using ArduNet, do not change the UUIDs or else the phone will not recognize the device.
```C++
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
```
Initialize device and serial output
```C++
Serial.begin(115200);
Heltec.begin(true, true , true, true , BAND); // display, lora, serial, paboost, band
```
Start Bluetooth Transmission
```C++
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
  ```
  Function that recieves incoming LoRa signal, and converts it to a String (byte -> String conversion)
  ```C++
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
```
Remove identifier from the packet, and set the correct value to the variable.  For example, an input would be A1000.23.  The "A" is removed and the value 1000.23 is set as the new value
```C++
void updateVariables(String packet) {
      String tempVal = getValue(packet,'/',0);      
      tempVal.remove(0,1);
      deviceName = tempVal;
}
```
Prepare BLE packet by concatenating all packets into a single string, encoding it into bytes and sending it using BLE transmission
```C++
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
  ```
  
