
/*************** DECLARATIONS**************************/
//*************************** BLE Transmission *********************
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
int BLESampleTime = 1000;
int BLESampleReset = 0;
int BLELastRunTime = 0;
unsigned long btTimeCurrent1 = 0;
unsigned long btTimePrev1 = 0;
unsigned long btTimeCurrent2 = 0;
unsigned long btTimePrev2 = 0;

#define DEMO_DURATION 3000
typedef void (*Demo)(void);

#define PIN_SDA 21
#define PIN_SCL 22

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

bool bluetoothDelay = HIGH;

//******************************** Resistances *************************
double resistance1 = 0;
double deltaR1 = 0;
double resistance2 = 0;
double deltaR2 = 0;
double resistance3 = 0;
double deltaR3 = 0;
double resistance4 = 0;
double deltaR4 = 0;
double resistance5 = 0;
double deltaR5 = 0;

double temperature = 0;
double humidity = 0;
double ppm = 0;

double resistanceInitial1;
double resistanceInitial2;
double resistanceInitial3;
double resistanceInitial4;
double resistanceInitial5;

double Rref = 1000;          // Reference resistor's value in ohms (you can give this value in kiloohms or megaohms - the resistance of the tested resistor will be given in the same units)

bool takedR = true;

//*************************** TEMPERATURE/RH SENSOR **********************
#include <Wire.h>
#include "ClosedCube_HDC1080.h"
ClosedCube_HDC1080 hdc1080;

//************************************ LED ******************************
const int ledPinGreen = 5;
const int ledPinRed = 17;
const int ledPinBlue = 18;
const int batLEDGreen = 27;
const int batLEDRed = 26;
const int batLEDBlue = 25;

//*************************** Buzzer ***********************************
const int buzzer = 12;
int freq = 2000;
int channel = 0;
int resolution = 8;
int buzzDelay = 0;

//*************************************** Display **********************************
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1327.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 128 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1327 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET, 800000);

//*********************************** RTC ************************************
#include <RTClib.h>
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//*********************************** ADC ******************************
#include <Adafruit_ADS1015.h>
Adafruit_ADS1115 ads(0x48);

const int samplingRate = 50;
unsigned long currentTime = 0;
unsigned long currentTimeMicro = 0;
unsigned long prevTime = 0;
unsigned long prevdR = 0;
int counter = 0;

//Read Arrays
int16_t sensorADCRead[samplingRate]; //ads.readADC_SingleEnded(0);
int16_t VinADCRead[samplingRate]; //ads.readADC_SingleEnded(1);
int16_t batADCRead[samplingRate]; //ads.readADC_SingleEnded(2);

//For Data
int16_t sensorADCRead1 = 0;
int16_t VinADCRead1 = 0;
int16_t batADCRead1 = 0;

//Sum
long sensorSum = 0;
long VinSum = 0;
long batSum = 0;
  
//Average values (final)
float sensorADC = 0.0;
float VinADC = 0.0;
float batADC = 0.0;

float volt = 0;
int batLevel = 0;


/*****************************************************************************/

void setup() {

  Serial.begin(500000);

  ads.begin();
  ads.setSPS(ADS1115_DR_860SPS);
  ads.setGain(GAIN_ONE);

  //RTC
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
  }
  //rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));
      //Note: The time on the RTC will be 30s behind due to uploading/compiling time
  
  //Initialize display
  display.begin(0x3D, true);


  // LED TEST
  pinMode (ledPinRed, OUTPUT);
  pinMode (ledPinGreen, OUTPUT);
  pinMode (ledPinBlue, OUTPUT);
  pinMode (batLEDGreen, OUTPUT);
  pinMode (batLEDRed, OUTPUT);
  pinMode (batLEDBlue, OUTPUT);
  
  // TEMP/RH SENSOR
  hdc1080.begin(0x40);

  // BUZZER
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(12, channel);

  //BLE Transmission//
  
  // Create the BLE Device
  BLEDevice::init("Gas Sensor 1");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID,BLECharacteristic::PROPERTY_READ|BLECharacteristic::PROPERTY_WRITE|BLECharacteristic::PROPERTY_NOTIFY|BLECharacteristic::PROPERTY_INDICATE);

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");

  BLESampleReset = BLESampleTime + 1;
}

void loop() {
  currentTime = millis();
  currentTimeMicro = micros();
  //Read Sensor and Battery
  processData;

  //Temp/Humidity
  temperature = hdc1080.readTemperature();
  humidity = hdc1080.readHumidity();

  //Display
  //displayData();
  serialPrint();    //serial display for debuugging
  
    // LED
  flashLed();
  //delay(10);

  
  if (BLESampleReset >= BLESampleTime) {
    transmitPacket();
    BLELastRunTime = millis();
    BLESampleReset = 0;
  }
  else {
    BLESampleReset = millis() - BLELastRunTime;
    //Serial.println((String)BLESampleReset);
  }
}


void processData(){

  if (takedR == true) {
      resistanceInitial1 = resistance1;
      takedR = false;
  }
  
  sensorADCRead1 = ads.readADC_SingleEnded(0);
  VinADCRead1 = ads.readADC_SingleEnded(1);
  batADCRead1 = ads.readADC_SingleEnded(2);

  if (currentTimeMicro - prevTime >= 1250) {
    sensorSum = sensorSum - sensorADCRead[counter];
    sensorADCRead[counter] = sensorADCRead1;
    sensorSum = sensorSum + sensorADCRead[counter];

    VinSum = VinSum - VinADCRead[counter];
    VinADCRead[counter] = VinADCRead1;
    VinSum = VinSum + VinADCRead[counter];

    batSum = batSum - batADCRead[counter];
    batADCRead[counter] = batADCRead1;
    batSum = batSum + batADCRead[counter];
    
    counter = counter + 1;
    prevTime = currentTime;
    if (counter >= samplingRate) {
      counter = 0;
    }
  }
  
  //Average & convert to voltages
  sensorADC = (sensorSum/samplingRate) * 0.125 / 1000;
  VinADC = (VinSum/samplingRate) * 0.125 / 1000;
  batADC = (batSum/samplingRate) * 0.125 / 1000;

  //Round to 2 decimal places
  //sensorADC = roundf(sensorADC*100)/100;
  //VinADC = roundf(VinADC*100)/100;

  //Resistance conversion for sensor
  resistance1 = Rref * (1/((VinADC/sensorADC)-1)); //Ensure right schematic for this function
  if (resistance1 <= 0) {
    resistance1 = 0;
  }

  //Battery voltage conversion & level output
  //volt = roundf(batADC * 2 * 100)/100;
  volt = batADC*2;
  if (volt > 3.2 && volt < 4.2) {
    batLevel = (volt - 3.2) * 100;
  }
  else if(volt <= 3.2) {
    batLevel = 1;
  }
  else if(volt >= 4.2) {
    batLevel = 100;
  }
  
  if (currentTime - prevdR >= 1000){
      deltaR1 = resistance1 - resistanceInitial1;
      prevdR = currentTime;
      takedR = true;
  }
}

// DISPLAYs
void displayData() {
    
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1327_WHITE);
  display.setCursor(0,0);
  display.println((String)ppm + " PPM");
  display.println((String)temperature + " C");
  display.println((String)humidity + " %");
  display.println("R1(ohm): " + (String)resistance1 /*+ " " + (String)resistance2 + " " + (String)resistance3 + " " + (String)resistance4 + " " +  (String)resistance5*/);
  display.println("R1 rd: " + (String)sensorADC + " Vin: " + (String)VinADC);
  display.println("dR1(ohm/s): " + (String)deltaR1 /*+ " " + (String)deltaR2 + " " + (String)deltaR3 + " " + (String)deltaR4 + " " + (String)deltaR5*/);

  display.println((String)batADC + " " + (String)volt + " " + (String)batLevel);

  //RTC
  DateTime now = rtc.now();
  display.println((String)daysOfTheWeek[now.dayOfTheWeek()] + "-" + (String)now.day() + "/" + (String)now.month() + "/" + (String)now.year());
  display.println((String)now.hour() + ":"+ (String)now.minute() + ":" + (String)now.second());

  display.println(analogRead(34));
  display.println("s: " + (String)sensorADCRead1 + " v: " + (String)VinADCRead1 + " b: " + (String)batADCRead1);
  display.println("Sum s: " + (String)sensorSum);
  display.println("millis: " + (String)currentTime);
  display.println("prev: " + (String)prevTime);

  display.display();
}

void serialPrint() {
  //Serial.println((String)ppm + " PPM");
  //Serial.println((String)temperature + " C");
  //Serial.println((String)humidity + " %");
  Serial.println(/*"R1(ohm): " + */(String)resistance1 /*+ " " + (String)resistance2 + " " + (String)resistance3 + " " + (String)resistance4 + " " +  (String)resistance5*/);
  //Serial.println("R1 rd: " + (String)sensorADC + " Vin: " + (String)VinADC);
  //Serial.println("dR1(ohm/s): " + (String)deltaR1 /*+ " " + (String)deltaR2 + " " + (String)deltaR3 + " " + (String)deltaR4 + " " + (String)deltaR5*/);

  //Serial.println((String)batADC + " " + (String)volt + " " + (String)batLevel);

  //RTC
  //DateTime now = rtc.now();
  //Serial.println((String)daysOfTheWeek[now.dayOfTheWeek()] + "-" + (String)now.day() + "/" + (String)now.month() + "/" + (String)now.year());
  //Serial.println((String)now.hour() + ":"+ (String)now.minute() + ":" + (String)now.second());

  //Serial.println(analogRead(34));
  //Serial.println("s: " + (String)sensorADCRead1 + " v: " + (String)VinADCRead1 + " b: " + (String)batADCRead1);
  //Serial.println("Sum s: " + (String)sensorSum);
  //Serial.println("millis: " + (String)currentTime);
  //Serial.println("prev: " + (String)prevTime);
}

void flashLed() {
  int ledFlash;
  int changeState = 0;
  //digitalWrite (buzzer, LOW);
  if (resistance1 > 2000)
  {
    ledFlash = ledPinRed;
    ledcWrite(channel, 125); //(0-230)
    ledcWriteTone(channel, 500);
    //delay(100);
    ledcWrite(channel, 0); //(0-230)
    changeState = 1;
  }
  else if (resistance1 > 1250 )
  {
    ledFlash = ledPinBlue;
    ledcWrite(channel, 15); //(0-230)
    ledcWriteTone(channel, 100);
    //delay(100);
    ledcWrite(channel, 0); //(0-230)
    changeState = 1;
  }
  else {
    ledFlash = ledPinGreen;
    ledcWrite(channel, 0); //(0-230)
    ledcWriteTone(channel, 0);
    changeState = 1;
  }
  if (changeState == 1)
  {
    digitalWrite (ledPinRed,LOW);
    digitalWrite (ledPinBlue,LOW);
    digitalWrite (ledPinGreen,LOW);
    digitalWrite (ledFlash, HIGH);
  }

  if (analogRead(34)>2000 && batLevel >= 95) {
    digitalWrite(batLEDGreen, HIGH);
  } else {digitalWrite(batLEDGreen, LOW);}

  if (batLevel <= 15) {
    digitalWrite(batLEDRed, HIGH);
  } else {digitalWrite(batLEDRed, LOW);}
  
}

void transmitPacket() {
    // BLE Notification Asynchronous

  if (deviceConnected) {
      btTimeCurrent1 = millis();
      if (btTimeCurrent1 - btTimePrev1 >= 30) {
          String dataPacket = "";
          dataPacket = dataPacket + "A" + (String)btTimeCurrent1 + "/";
          dataPacket = dataPacket + "B" + (String)temperature + "/";
          dataPacket = dataPacket + "C" + (String)humidity + "/";
          dataPacket = dataPacket + "D" + (String)resistance1 + "/";
          dataPacket = dataPacket + "E" + (String)deltaR1 + "/";
          dataPacket = dataPacket + "F" + (String)ppm;
          //Serial.println(dataPacket);
          
          // Define 
          String str = dataPacket; 
          
          // Length (with one extra character for the null terminator)
          int str_len = str.length() + 1; 
          
          // Prepare the character array (the buffer) 
          char char_array[str_len];
          
          // Copy it over 
          str.toCharArray(char_array, str_len);
          
          pCharacteristic->setValue(char_array);
          pCharacteristic->notify();
          //delay(30); // bluetooth stack will go into congestion if too many packets are sent, in 6 hours test i was able to go as low as 3ms
          btTimePrev1 = btTimeCurrent1;
      }
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
      btTimeCurrent2 = millis();
      if (bluetoothDelay == HIGH) {
          btTimePrev2 = btTimeCurrent2;
          bluetoothDelay = LOW;
      }
      if (btTimeCurrent2 - btTimePrev2 >= 500) {
          //delay(500); // give the bluetooth stack the chance to get things ready
          pServer->startAdvertising(); // restart advertising
          Serial.println("start advertising");
          oldDeviceConnected = deviceConnected;
          bluetoothDelay = HIGH;
      }
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
      // do stuff here on connecting
      oldDeviceConnected = deviceConnected;
  }
}
