
/*************** DECLARATIONS**************************/
// BLE Transmission
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
int BLESampleTime = 1000;
int BLESampleReset = 0;
int BLELastRunTime = 0;

#define DEMO_DURATION 3000
typedef void (*Demo)(void);

#define PIN_SDA 21
#define PIN_SCL 22

//Resistances
unsigned long relativeTime = 0;
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
double StartTime;
int dRSampleTime = 1000;
int dRSampleTimeReset = 0;

// TEMPERATURE/RH SENSOR
#include <Wire.h>
#include "ClosedCube_HDC1080.h"
ClosedCube_HDC1080 hdc1080;

// LED
const int ledPinGreen = 5;
const int ledPinRed = 17;
const int ledPinBlue = 18;
const int batLEDGreen = 27;
const int batLEDRed = 26;
const int batLEDBlue = 25;

// RESISTANCE MEASUREMENT
const int sensorPin1 = 39;  // Analog input pin that senses Vout
const int voltagePinA = 0;// Vin
const int voltagePinB = 2;// Vin
double sensorValue1 = 0;       // sensorPin default value

double Vin = 3.3;             // Input voltage
//double Vin = 1.985;
double Vout1 = 0;            // Vout default value
double Vout2 = 0;
double Vout3 = 0;
double Vout4 = 0;
double Vout5 = 0;

double Rref = 997;          // Reference resistor's value in ohms (you can give this value in kiloohms or megaohms - the resistance of the tested resistor will be given in the same units)
double R1 = 0;               // Tested resistors default value
double R2 = 0;
double R3 = 0;
double R4 = 0;
double R5 = 0;

// Buzzer
const int buzzer = 12;
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

//Display
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1327.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 128 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1327 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//RTC
#include <RTClib.h>
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//Battery Detector
const double convFactor = 1.78;
#include <driver/adc.h>

double volt = 0;
double readArray[1000];
int batLevel = 0;
double rms = 0;

unsigned long currentMillis = 0;
unsigned long prevMillis = 0;
int counter = 0;
double vs[101];
/*****************************************************************************/

void setup() {

  adc1_config_width(ADC_WIDTH_12Bit);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_11db);
  adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_11db);
  adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_11db);
  

  initVoltsArray();
  for(int i = 0; i < 1000; i++){
    //readArray[i] = analogRead(36);
    readArray[i] = adc1_get_voltage(ADC1_CHANNEL_0);
    //readArray[i] = 2000;
  }

  //RTC
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
  }

  //rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));
  
  Serial.begin(115200);

  // SSD1327_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display.begin(0x3D, true);
  //if(!display.begin(0x3D, true)) { // Address 0x3D for 128x64
    //Serial.println("SSD1327 allocation failed");
    //for(;;); // Don't proceed, loop forever
  //}

  // LED TEST
  pinMode (ledPinRed, OUTPUT);
  pinMode (ledPinGreen, OUTPUT);
  pinMode (ledPinBlue, OUTPUT);
  pinMode (batLEDGreen, OUTPUT);
  pinMode (batLEDRed, OUTPUT);
  pinMode (batLEDBlue, OUTPUT);
  
  // TEMP/RH SENSOR
  hdc1080.begin(0x40);

  //RESISTANCE MEASUREMENT
  pinMode (voltagePinA, OUTPUT);
  pinMode (voltagePinB, OUTPUT);
  digitalWrite (voltagePinA, HIGH);
  digitalWrite (voltagePinB, LOW);

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
  
  //Battery Detector
  getVoltage();

  //Temp/Humidity
  temperature = hdc1080.readTemperature();
  humidity = hdc1080.readHumidity();

  // RESISTANCE
  getResistance();

  // UPDATE PPM
  updatePPM();

  //Display
  displayData();
  
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
  display.println("dR1(ohm/s): " + (String)deltaR1 /*+ " " + (String)deltaR2 + " " + (String)deltaR3 + " " + (String)deltaR4 + " " + (String)deltaR5*/);

  display.println((String)rms + " " + (String)volt + " " + (String)batLevel);

  //RTC
  DateTime now = rtc.now();
  display.println((String)daysOfTheWeek[now.dayOfTheWeek()] + "-" + (String)now.day() + "/" + (String)now.month() + "/" + (String)now.year());
  display.println((String)now.hour() + ":"+ (String)now.minute() + ":" + (String)now.second());

  display.println(analogRead(34));

  display.display();
}

void getVoltage(){

  currentMillis = millis();
  if(currentMillis - prevMillis >= 1) {
    //readArray[counter] = analogRead(36);
    readArray[counter] = adc1_get_voltage(ADC1_CHANNEL_0);
    prevMillis = currentMillis;
    counter = counter + 1;
   if(counter >= 1000){
      counter = 0;
    }
  }

  double sum = 0;
  for (int i = 0; i<1000; i++){
    sum = sum + pow(readArray[i],2);
  }
  rms = sqrt(sum/1000);

  volt = rms*convFactor/1000;
  volt = roundf(volt*100)/100;
  //batLevel = getChargeLevel(volt);
  if (volt > 3.2 && volt < 4.2) {
    batLevel = (volt - 3.2)*100;
  }
  else if (volt < 3.2) {
    batLevel = 1;
  }
  else if (volt > 4.2) {
    batLevel = 100;
  }

  
}

void updatePPM() {
  double alpha = 1;
  double beta = 2;
  double gamma = 2;

  ppm = alpha * resistance1 + beta * humidity + gamma * temperature;
}

// Measure Resistance
void getResistance() {
  //digitalWrite (voltagePin1, HIGH);
  int samples = 43;
  int i;
  R1 = R2 = R3 = R4 = R5 = 0;
  if (dRSampleTimeReset == 0)
  {
    dRSampleTimeReset = 1;
    StartTime = millis();
    resistanceInitial1 = resistance1;
  }
  for (i = 0; i < samples; i++)
  {
    //sensorValue1 = adc1_get_voltage(ADC1_CHANNEL_7);  // Read Vout on analog input pin A0 (Arduino can sense from 0-1023, 1023 is 5V)
    sensorValue1 = adc1_get_raw(ADC1_CHANNEL_7);
   
    
    Vout1 = (Vin * sensorValue1) / 4096;    // Convert Vout to volts
    //Serial.println(sensorValue1);

    R1 = R1 + Rref * ((Vin / Vout1) - 1);  // Formula to calculate tested resistor's value
    
    //delay(1);
    delayMicroseconds(100);
  }
  double timeElapsed = millis() - StartTime;
  if (timeElapsed >= dRSampleTime)
  {
    //deltaR1 = ((1.3525*R1/samples-19.118) - resistanceInitial1)/(timeElapsed/1000);
    deltaR1 = (R1/samples - resistanceInitial1)/(timeElapsed/1000);
    
    dRSampleTimeReset = 0;
  }
  
  //resistance1 = 1.3525*(R1/samples)-19.118;
  resistance1 = R1/samples;
  Serial.println(resistance1);
  
  if (resistance1 < 0) {
    resistance1 = 0.0;
    deltaR1 = 0.0;
  }
  
  //digitalWrite (voltagePin1, LOW);

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
    delay(100);
    ledcWrite(channel, 0); //(0-230)
    changeState = 1;
  }
  else if (resistance1 > 1250 )
  {
    ledFlash = ledPinBlue;
    ledcWrite(channel, 15); //(0-230)
    ledcWriteTone(channel, 100);
    delay(100);
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

  //if (analogRead(34) > 2000 && batLevel <= 15) {
    //digitalWrite(batLEDBlue, HIGH);
    //digitalWrite(batLEDRed, HIGH);
  //} else {digitalWrite(batLEDBlue, LOW); digitalWrite(batLEDRed, LOW);}

  //if (analogRead(34) > 2000 && batLevel < 95 && batLevel > 15) {
    //digitalWrite(batLEDBlue, HIGH);
  //} else {digitalWrite(batLEDBlue, LOW);}
  
}

void transmitPacket() {
    // BLE Notification Asynchronous

  if (deviceConnected) {
      relativeTime = millis();
      String dataPacket = "";
      dataPacket = dataPacket + "A" + (String)relativeTime + "/";
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
      delay(30); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
      delay(500); // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // restart advertising
      Serial.println("start advertising");
      oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
      // do stuff here on connecting
      oldDeviceConnected = deviceConnected;
  }
}

void initVoltsArray(){
  //Taken from Pangodream_18650_CL.cpp due to private fn
    vs[0] = 3.200; 
    vs[1] = 3.250; vs[2] = 3.300; vs[3] = 3.350; vs[4] = 3.400; vs[5] = 3.450;
    vs[6] = 3.500; vs[7] = 3.550; vs[8] = 3.600; vs[9] = 3.650; vs[10] = 3.700;
    vs[11] = 3.703; vs[12] = 3.706; vs[13] = 3.710; vs[14] = 3.713; vs[15] = 3.716;
    vs[16] = 3.719; vs[17] = 3.723; vs[18] = 3.726; vs[19] = 3.729; vs[20] = 3.732;
    vs[21] = 3.735; vs[22] = 3.739; vs[23] = 3.742; vs[24] = 3.745; vs[25] = 3.748;
    vs[26] = 3.752; vs[27] = 3.755; vs[28] = 3.758; vs[29] = 3.761; vs[30] = 3.765;
    vs[31] = 3.768; vs[32] = 3.771; vs[33] = 3.774; vs[34] = 3.777; vs[35] = 3.781;
    vs[36] = 3.784; vs[37] = 3.787; vs[38] = 3.790; vs[39] = 3.794; vs[40] = 3.797;
    vs[41] = 3.800; vs[42] = 3.805; vs[43] = 3.811; vs[44] = 3.816; vs[45] = 3.821;
    vs[46] = 3.826; vs[47] = 3.832; vs[48] = 3.837; vs[49] = 3.842; vs[50] = 3.847;
    vs[51] = 3.853; vs[52] = 3.858; vs[53] = 3.863; vs[54] = 3.868; vs[55] = 3.874;
    vs[56] = 3.879; vs[57] = 3.884; vs[58] = 3.889; vs[59] = 3.895; vs[60] = 3.900;
    vs[61] = 3.906; vs[62] = 3.911; vs[63] = 3.917; vs[64] = 3.922; vs[65] = 3.928;
    vs[66] = 3.933; vs[67] = 3.939; vs[68] = 3.944; vs[69] = 3.950; vs[70] = 3.956;
    vs[71] = 3.961; vs[72] = 3.967; vs[73] = 3.972; vs[74] = 3.978; vs[75] = 3.983;
    vs[76] = 3.989; vs[77] = 3.994; vs[78] = 4.000; vs[79] = 4.008; vs[80] = 4.015;
    vs[81] = 4.023; vs[82] = 4.031; vs[83] = 4.038; vs[84] = 4.046; vs[85] = 4.054;
    vs[86] = 4.062; vs[87] = 4.069; vs[88] = 4.077; vs[89] = 4.085; vs[90] = 4.092;
    vs[91] = 4.100; vs[92] = 4.111; vs[93] = 4.122; vs[94] = 4.133; vs[95] = 4.144;
    vs[96] = 4.156; vs[97] = 4.167; vs[98] = 4.178; vs[99] = 4.189; vs[100] = 4.200;
}

int getChargeLevel(double volts){
  //Taken from Pangodream_18650_CL.cpp due to private fn
  int idx = 50;
  int prev = 0;
  int half = 0;
  if (volts >= 4.2){
    return 100;
  }
  if (volts <= 3.2){
    return 0;
  }
  while(true){
    half = abs(idx - prev) / 2;
    prev = idx;
    if(volts >= vs[idx]){
      idx = idx + half;
    }else{
      idx = idx - half;
    }
    if (prev == idx){
      break;
    }
  }
  return idx;
}
