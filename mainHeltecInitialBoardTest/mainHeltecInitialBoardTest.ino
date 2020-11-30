// DISPLAY
#include "Arduino.h"
#include "heltec.h"
#include "images.h"

#define DEMO_DURATION 3000
typedef void (*Demo)(void);

#define PIN_SDA 21
#define PIN_SCL 22

double resistance = 0;
double deltaR = 0;
double temperature = 0;
double humidity = 0;
double ppm = 0;

// Calculations
double resistanceInitial;
double StartTime;
int dRSampleTime = 1000;
int dRSampleTimeReset = 0;

// TEMPERATURE/RH SENSOR
#include <Wire.h>
#include "ClosedCube_HDC1080.h"
ClosedCube_HDC1080 hdc1080;

// LED BLINK TEST
const int ledPinGreen = 5;
const int ledPinRed = 17;
const int ledPinBlue = 18;

// RESISTANCE MEASUREMENT
const int sensorPin = 36;  // Analog input pin that senses Vout
const int voltagePin = 13;  // Analog input pin that senses Vout
double sensorValue = 0;       // sensorPin default value
//double Vin = 3.30;             // Input voltage
double Vin = 1.985;
double Vout = 0;            // Vout default value
double Rref = 997;          // Reference resistor's value in ohms (you can give this value in kiloohms or megaohms - the resistance of the tested resistor will be given in the same units)
double R = 0;               // Tested resistors default value

// Buzzer
const int buzzer = 12;
int freq = 2000;
int channel = 0;
int resolution = 8;
int buzzDelay = 0;

void setup() {
  Serial.begin(115200);
  // DISPLAY
  Heltec.begin(true, false, true);
  Heltec.display->flipScreenVertically();

  // LED TEST
  pinMode (ledPinRed, OUTPUT);
  pinMode (ledPinGreen, OUTPUT);
  pinMode (ledPinBlue, OUTPUT);
  
  // TEMP/RH SENSOR
  hdc1080.begin(0x40);

  //RESISTANCE MEASUREMENT
  pinMode (voltagePin, OUTPUT);

  // BUZZER
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(12, channel);
}

// DISPLAYs
  void displayData() {
  int x = 0;
  int y = 0;
  Heltec.display->drawXbm(x+100, y+20, BB_width, BB_height, BB_bits);
  Heltec.display->drawXbm(x + 12 + 1, y, WIFI_width, WIFI_height, WIFI_bits);
  Heltec.display->drawXbm(x + 108, y, BAT_width, BAT_height, BAT_bits);
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(x, y + 10, (String)ppm + " PPM");
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(x, y + 20, "TEMP: " + (String)temperature + "Â°C");
  Heltec.display->drawString(x, y + 30, "RH: " + (String)humidity + "%");
  Heltec.display->drawString(x, y + 40, "R: " + (String)resistance + " ohm ");
  Heltec.display->drawString(x, y + 50, "dR: " + (String)deltaR + " ohm/s ");
}

void updatePPM()
{
  double alpha = 1;
  double beta = 2;
  double gamma = 2;

  ppm = alpha * resistance + beta * humidity + gamma * temperature;
}

// Measure Resistance
void getResistance() {
  digitalWrite (voltagePin, HIGH);
  //delay(100);
  int samples = 50;
  int i;
  R = 0;
  //Serial.println(millis());
  if (dRSampleTimeReset == 0)
  {
    dRSampleTimeReset = 1;
    StartTime = millis();
    resistanceInitial = resistance;
    //Serial.println(resistanceInitial);
  }
  for (i = 0; i < samples; i++)
  {
    // RESISTANCE
    sensorValue = analogRead(sensorPin);  // Read Vout on analog input pin A0 (Arduino can sense from 0-1023, 1023 is 5V)
    Vout = (Vin * sensorValue) / 4095;    // Convert Vout to volts
    R = R + Rref * (1 / ((Vin / Vout) - 1));  // Formula to calculate tested resistor's value
    delay(1);
    //Serial.print("R: ");                  
    //Serial.println(R);                    // Give calculated resistance in Serial Monitor
  }
  double timeElapsed = millis() - StartTime;
  if (timeElapsed >= dRSampleTime)
  {
    deltaR = ((1.3525*R/samples-19.118) - resistanceInitial)/(timeElapsed/1000);
    dRSampleTimeReset = 0;
  }
  resistance = 1.3525*R/samples-19.118;
  if (resistance < 0) {
    resistance = 0.0;
    deltaR = 0.0;
  }
  digitalWrite (voltagePin, LOW);
}

void flashLed() {
  int ledFlash;
  int changeState = 0;
  //digitalWrite (buzzer, LOW);
  if (resistance > 2000)
  {
    ledFlash = ledPinRed;
    ledcWrite(channel, 125); //(0-230)
    ledcWriteTone(channel, 500);
    delay(100);
    ledcWrite(channel, 0); //(0-230)
    changeState = 1;
  }
  else if (resistance > 1250 )
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
  //digitalWrite (ledFlash, HIGH);
  //delay(20);
  //digitalWrite (ledFlash, LOW);
}

void loop() {
  // DISPLAY
  Heltec.display->clear();
  displayData();
  Heltec.display->display();
  //delay(10);

  // TEMP
  //Serial.print("T=");
  //Serial.print(hdc1080.readTemperature());
  temperature = hdc1080.readTemperature();
  //Serial.print("C, RH=");
  //Serial.print(hdc1080.readHumidity());
  humidity = hdc1080.readHumidity();
  //Serial.println("%");

  // RESISTANCE
  getResistance();

  // UPDATE PPM
  updatePPM();
  
    // LED
  flashLed();
  delay(10);
}

// Hi
