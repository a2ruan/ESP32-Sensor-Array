void displayData() {
  int x = 0;
  int y = 0;
  Heltec.display->drawXbm(x, y, Wifi_on_width, Wifi_on_height, Wifi_on_bits);

  if (BT_enable == 1) {Heltec.display->drawXbm(x+18, y, Bluetooth_on_width, Bluetooth_on_height, Bluetooth_on_bits);}
  else {Heltec.display->drawXbm(x+18, y, Bluetooth_off_width, Bluetooth_off_height, Bluetooth_off_bits);}

  if (lora_enable == 1) {Heltec.display->drawXbm(x+30, y, Lora_on_width, Lora_on_height, Lora_on_bits);}
  else {Heltec.display->drawXbm(x+30, y, Lora_off_width, Lora_off_height, Lora_off_bits);}

  if (usbPluggedIn) {Heltec.display->drawXbm(x+100, y, Battery_charging_width, Battery_charging_height, Battery_charging_bits);}
  else if (batteryLevel > 3.59){Heltec.display->drawXbm(x+100, y, Battery_100_width, Battery_100_height, Battery_100_bits);}
  else if (batteryLevel > 3.45){Heltec.display->drawXbm(x+100, y, Battery_66_width, Battery_66_height, Battery_66_bits);}
  else if (batteryLevel > 3.3){Heltec.display->drawXbm(x+100, y, Battery_33_width, Battery_33_height, Battery_33_bits);}
  else if (batteryLevel > 0){Heltec.display->drawXbm(x+100, y, Battery_0_width, Battery_0_height, Battery_0_bits);}
  

//  Heltec.display->drawXbm(x, y, Wifi_off_width, Wifi_off_height, Wifi_off_bits);
//  Heltec.display->drawXbm(x+18, y, Bluetooth_off_width, Bluetooth_off_height, Bluetooth_off_bits);
//  Heltec.display->drawXbm(x+30, y, Lora_off_width, Lora_off_height, Lora_off_bits);
//  Heltec.display->drawXbm(x+100, y, Battery_0_width, Battery_0_height, Battery_0_bits);
  
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(x, y + 10, (String)batteryLevel + " V");
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(x, y + 20, "TEMP: " + (String)temperature + "°C");
  Heltec.display->drawString(x, y + 30, "RH: " + (String)humidity + "%");
  Heltec.display->drawString(x, y + 40, "R: " + (String)resistance[0] + " ohm ");
  Heltec.display->drawString(x, y + 50, "dR: " + (String)deltaResistance[0] + " ohm/s ");
}

void updatePPM() {
  double alpha = 1;
  double beta = 2;
  double gamma = 2;
  ppm = alpha * resistance[0] + beta * humidity + gamma * temperature;
}

void setLed(int color) {
  digitalWrite (ledRed, LOW);
  digitalWrite (ledGreen, LOW);
  digitalWrite (ledBlue, LOW);

  if (color == 1) {digitalWrite (ledRed, HIGH);}
  else if (color == 2) {digitalWrite (ledGreen, HIGH);}
  else if (color == 3) {digitalWrite (ledBlue, HIGH);}
}

void setLed2(int color) {
  digitalWrite (ledRed2, LOW);
  digitalWrite (ledGreen2, LOW);
  digitalWrite (ledBlue2, LOW);

  if (color == 1) {digitalWrite (ledRed2, HIGH);}
  else if (color == 2) {digitalWrite (ledGreen2, HIGH);}
  else if (color == 3) {digitalWrite (ledBlue2, HIGH);}
}

void setBuzzer(int intensity, int toneLevel) {
  ledcWrite(channel, intensity); //(0-230)
  ledcWriteTone(channel, toneLevel);
}


void getResistance() {
  //sensorVoltage = analogRead(sensorPin1);
  //int16_t tempVoltage = adc.readADC_SingleEnded(1);
  int i;
  sensorVoltage = 0;
  if (counterSelect > 7) {counterSelect = 0;}
  for (i = 0; i < 5; i++) {
    if (counterSelect < 4) {sensorVoltage = sensorVoltage + ads1.readADC_SingleEnded(counterSelect)*0.000125;}
    if (counterSelect > 3) {sensorVoltage = sensorVoltage + ads2.readADC_SingleEnded(counterSelect-4)*0.000125;}
  }
  sensorVoltage = sensorVoltage / 5;
  counterSelect = counterSelect + 1;
  //sensorVoltage = 0;
  resistance[0] = Rref * ((Vin / sensorVoltage)-1);  // Formula to calculate tested resistor's value
  //delayMicroseconds(100);
  if (counterSelect == 1) {                                                                                                    
  Serial.println((String)counterSelect + ":" + (String)resistance[0]);}
}

void transmitPacket() {
  if (deviceConnected) {
      relativeTime = millis();
      String dataPacket = "";
      dataPacket = dataPacket + "A" + (String)relativeTime + "/";
      dataPacket = dataPacket + "B" + (String)temperature + "/";
      dataPacket = dataPacket + "C" + (String)humidity + "/";
      dataPacket = dataPacket + "D" + (String)resistance[0] + "/";
      dataPacket = dataPacket + "E" + (String)deltaResistance[0] + "/";
      dataPacket = dataPacket + "F" + (String)batteryLevel;
      //Serial.println(dataPacket);
      str = dataPacket;
      int str_len = str.length() + 1; 
      char char_array[str_len];
      str.toCharArray(char_array, str_len);
      pCharacteristic->setValue(char_array);
      pCharacteristic->notify();
      delay(30); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
  }
  if (!deviceConnected && oldDeviceConnected) { // disconnecting
      delay(500); // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // restart advertising
      Serial.println("start advertising");
      oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) { // connecting
      // do stuff here on connecting
      oldDeviceConnected = deviceConnected;
  }
}

void transmitLORAPacket() {
  // send packet
  Heltec.LoRa.beginPacket();
      relativeTime = millis();
      String dataPacket2 = "";
      dataPacket2 = dataPacket2 + "A" + (String)relativeTime + "/";
      dataPacket2 = dataPacket2 + "B" + (String)temperature + "/";
      dataPacket2 = dataPacket2 + "C" + (String)humidity + "/";
      dataPacket2 = dataPacket2 + "D" + (String)resistance[0] + "/";
      dataPacket2 = dataPacket2 + "E" + (String)deltaResistance[0] + "/";
      dataPacket2 = dataPacket2 + "F" + (String)ppm;
      //Serial.println(dataPacket);
      str = dataPacket2;
  Heltec.LoRa.print(str);
  Heltec.LoRa.endPacket();
}

void getBatteryLevel() {
  double batteryOut = analogRead(batterySense)*(3.3/4095);
  double batteryIn = batteryOut*(resistanceBattery1+resistanceBattery2)/resistanceBattery2;
  batteryLevel = batteryIn;
  //Serial.println("Battery:" + (String)batteryIn);
}

void getUSBIndicator() {
  double usbOut = analogRead(usbSense)*(3.3/4095);
  double usbIn = usbOut*(resistanceBattery1+resistanceBattery2)/resistanceBattery2;
  //batteryLevel = usbIn;
  if (usbIn > 4) {
    usbPluggedIn = true;
  }
  else {
    usbPluggedIn = false;
  }
  //Serial.println("Battery:" + (String)usbIn);
}
