//#include "driver/dac.h"
#include "Arduino.h"
#include "heltec.h"

#define DAC1 25
int value1;

void setup() {

  int Value = 150; //255= 3.3V 128=1.65V
  dacWrite(DAC1, Value);
  // put your setup code here, to run once:
    Serial.begin(115200);
  // DISPLAY
  Heltec.begin(true, false, true);
  Heltec.display->flipScreenVertically();
}

void loop() {
  // put your main code here, to run repeatedly:
  Heltec.display->clear();
  displayData();
  Heltec.display->display();
  //delay(10);
  value1 = analogRead(33);
  delay(500);
}


void displayData() {
  int x = 0;
  int y = 0;
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(x, y + 10, (String)value1);
}
