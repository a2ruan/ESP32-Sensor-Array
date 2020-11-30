/*************** DECLARATIONS**************************/
// Enables
int BT_enable = 0;
int lora_enable = 1;

//Display
#include "Arduino.h"
#include "heltec.h"
#include "images.h"

#define DEMO_DURATION 3000
typedef void (*Demo)(void);

//RTC
#include <RTClib.h>
RTC_DS3231 rtc;

// External ADC
#include <Wire.h>
#include <Adafruit_ADS1015.h>
Adafruit_ADS1115 ads(0x48);

// Internal ADC
const int usbSense = 36;
const int batterySense = 37;
double resistanceBattery1 = 120000;
double resistanceBattery2 = 120000;
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
#define BAND    433E6  //you can set band here directly,e.g. 868E6,915E6

// PACKET OUTPUTS
unsigned long relativeTime = 0;
double resistance[5];
double deltaResistance[5];
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
int S[4];
double sensorVoltage = 0;  // sensorPin default value

double Vin = 3.312;             // Input voltage
double Vout1 = 0;            // Vout default value
double Rref = 983;          // Reference resistor's value in ohms (you can give this value in kiloohms or megaohms - the resistance of the tested resistor will be given in the same units)

// Buzzer
const int buzzer = 13;
int freq = 2000;
int channel = 0;
int resolution = 8;
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
/*****************************************************************************/
void setup() {
  
  resistance[0] = 0, resistance[1] = 0, resistance[2]=0, resistance[3] = 0, resistance[4]=0;
  deltaResistance[0] = 0, deltaResistance[1] = 0, deltaResistance[2]=0, deltaResistance[3] = 0, deltaResistance[4]=0;
  S[0] = 0, S[1] = 1, S[2]=2, S[3] = 3;
  
  // DISPLAY
  Serial.begin(115200);
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.Heltec.Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  Heltec.display->flipScreenVertically();
  
  // External ADC
  ads.setGain(GAIN_ONE);
  ads.begin();

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

  //RESISTANCE MEASUREMENT
  pinMode (S[0], OUTPUT);
  pinMode (S[1], OUTPUT);
  pinMode (S[2], OUTPUT);
  pinMode (S[3], OUTPUT);
  digitalWrite (S[0], LOW);
  digitalWrite (S[1], LOW);
  digitalWrite (S[2], LOW);
  digitalWrite (S[3], LOW);

  // BUZZER
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(buzzer, channel);

  //BLE Transmission
  BLEDevice::init("Gas Sensor 1");
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
}

void loop() {
  Heltec.display->clear();
  displayData();
  Heltec.display->display();
  
  //DateTime now = rtc.now();
  //Serial.println((String)now.hour() + ":"+ (String)now.minute() + ":" + (String)now.second());

  //setLed(3);
  setBuzzer(200,100);

  getBatteryLevel();
  getUSBIndicator();
  temperature = hdc1080.readTemperature();
  humidity = hdc1080.readHumidity();
  getResistance();
  updatePPM();

  if (BT_enable == 1) {transmitPacket();}
  if (lora_enable == 1) {transmitLORAPacket();}
  delay(30);
}
