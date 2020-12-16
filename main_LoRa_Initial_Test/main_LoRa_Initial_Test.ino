// Enables
int BT_enable = 1;
int lora_enable = 1;

// Display
#include "Arduino.h"
#include "heltec.h"
#include "images.h"

#define DEMO_DURATION 3000
typedef void (*Demo)(void);

// DAC for UV LED
#include <Adafruit_MCP4725.h>
Adafruit_MCP4725 dac;
double UVLEDvoltage = 3.3; // 3.3V max

String deviceName = "Gas_Sensor_1"; // NO BLANK SPACES PERMITTED

// RTC
#include <RTClib.h>
RTC_DS3231 rtc;

// External ADC
#include <Wire.h>
#include <Adafruit_ADS1015.h>
Adafruit_ADS1115 ads1(0x48);
//Adafruit_ADS1115 ads2(0x48);

// Internal ADC
const int usbSense = 36;
const int batterySense = 37;
double resistanceBattery1 = 100000;
double resistanceBattery2 = 100000;
double batteryLevel;
boolean usbPluggedIn;

// BLE Transmission
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#define PIN_SDA 21
#define PIN_SCL 22

// LoRA
#define BAND    915E6  //you can set band here directly,e.g. 868E6,915E6

// PACKET OUTPUTS
unsigned long relativeTime = 0;
unsigned long currentTime;
unsigned long previousTime;
double resistance[8];
double deltaResistance[8];
double temperature = 0;
double humidity = 0;
double ppm = 0;
String str;

// TEMPERATURE/RH SENSOR
#include "ClosedCube_HDC1080.h"
ClosedCube_HDC1080 hdc1080;

// LED
const int ledGreen = 32;
const int ledRed = 33;
const int ledBlue = 25;

// RESISTANCE MEASUREMENT
const int sensorPin1 = 39;  // Analog input pin that senses Vout
double sensorVoltage = 0;  // sensorPin default value
double Vin = 3.0;             // Input voltage
double Rref = 1500;          // Reference resistor's value in ohms (you can give this value in kiloohms or megaohms - the resistance of the tested resistor will be given in the same units)

// Buzzer
const int buzzer = 23;
int freq = 1000;
int channel = 0;
int resolution = 100;
int buzzDelay = 0;

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
  // DISPLAY
  Serial.begin(115200);
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.Heltec.Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  Heltec.display->flipScreenVertically();
  // External ADC
  ads1.setGain(GAIN_ONE);
  ads1.begin();

  // DAC for UV LED
  dac.begin(0x62);
  dac.setVoltage(UVLEDvoltage*(4095/3.3), false); // range from 0 to 4095 aka 0 - 3.3V
  
  // Internal ADC
  pinMode(usbSense, INPUT);
  pinMode(batterySense, INPUT);
  
  //RTC
  if (! rtc.begin()) {Serial.println("Couldn't find RTC");}
  rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));
  
  // LED TEST
  pinMode (ledRed, OUTPUT);
  pinMode (ledGreen, OUTPUT);
  pinMode (ledBlue, OUTPUT);
  
  // TEMP/RH SENSOR
  hdc1080.begin(0x40);

  // BUZZER
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(buzzer, channel);

  //BLE Transmission
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
  Serial.println("Waiting a client connection to notify...");
  Heltec.display->clear();
  Heltec.display->drawXbm(0, 5, uOttawa_Logo_width, uOttawa_Logo_height, uOttawa_Logo_bits);
  Heltec.display->display();
  delay(2000);
  LoRa.setSpreadingFactor(7);
  LoRa.setTxPower(17,1);
  previousTime = micros();
}

/**
 * Main Loop
 */
void loop() {
  // Update display and peripheral indicator lights/buzzer
  Heltec.display->clear();
  displayData();
  Heltec.display->display();
  //setLed(1);
  //setBuzzer(300,200);
  getBatteryLevel();
  getUSBIndicator();
  temperature = hdc1080.readTemperature();
  humidity = hdc1080.readHumidity();
  if (temperature > 120 || humidity > 90) { // Check if HDC1080 is disconnected, and restart if necessary
    hdc1080.begin(0x40);
  }
  getResistance();
  updatePPM();
  if (BT_enable == 1) {transmitBLEPacket();}
  if (lora_enable == 1) {transmitLORAPacket();}
}
