void displayData() {
  int x = 0;
  int y = 0;
  Heltec.display->drawXbm(x, y, Wifi_on_width, Wifi_on_height, Wifi_on_bits);
  if (BT_enable == 1) {Heltec.display->drawXbm(x+18, y, Bluetooth_on_width, Bluetooth_on_height, Bluetooth_on_bits);}
  else {Heltec.display->drawXbm(x+18, y, Bluetooth_off_width, Bluetooth_off_height, Bluetooth_off_bits);}
  if (lora_enable == 1) {Heltec.display->drawXbm(x+30, y, Lora_on_width, Lora_on_height, Lora_on_bits);}
  else {Heltec.display->drawXbm(x+30, y, Lora_off_width, Lora_off_height, Lora_off_bits);}
  if (usbPluggedIn) {Heltec.display->drawXbm(x+100, y, Battery_charging_width, Battery_charging_height, Battery_charging_bits);}
  else if (batteryLevel > 1.59){Heltec.display->drawXbm(x+100, y, Battery_100_width, Battery_100_height, Battery_100_bits);}
  else if (batteryLevel > 1.45){Heltec.display->drawXbm(x+100, y, Battery_66_width, Battery_66_height, Battery_66_bits);}
  else if (batteryLevel > 1.3){Heltec.display->drawXbm(x+100, y, Battery_33_width, Battery_33_height, Battery_33_bits);}
  else if (batteryLevel > 0){Heltec.display->drawXbm(x+100, y, Battery_0_width, Battery_0_height, Battery_0_bits);}
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawLine(62, 22,62, 80);
  Heltec.display->drawString(x+60, y, (String)batteryLevel + " V");
  Heltec.display->drawString(x, y + 10, "T=" + double2string(temperature,1) + "°C");
  Heltec.display->drawString(x+65, y + 10, "RH=" + double2string(humidity,0) + "%");
  Heltec.display->drawString(x, y + 20, "R1:" + double2string(resistance[0],0));
  Heltec.display->drawString(x, y + 30, "R2:" + double2string(resistance[1],0));
  Heltec.display->drawString(x, y + 40, "R3:" + double2string(resistance[2],0));
  Heltec.display->drawString(x, y + 50, "R4:" + double2string(resistance[3],0));
  Heltec.display->drawString(x+40, y + 20, "[" + double2string(abs(deltaResistance[0]),1) + "]");
  Heltec.display->drawString(x+40, y + 30, "[" + double2string(abs(deltaResistance[1]),1) + "]");
  Heltec.display->drawString(x+40, y + 40, "[" + double2string(abs(deltaResistance[2]),1) + "]");
  Heltec.display->drawString(x+40, y + 50, "[" + double2string(abs(deltaResistance[3]),1) + "]");
//  Heltec.display->drawString(x+65, y + 20, "R5:" + double2string(resistance[4],0));
//  Heltec.display->drawString(x+65, y + 30, "R6:" + double2string(resistance[5],0));
//  Heltec.display->drawString(x+65, y + 40, "R7:" + double2string(resistance[6],0));
//  Heltec.display->drawString(x+65, y + 50, "R8:" + double2string(resistance[7],0));
//  Heltec.display->drawString(x+105, y + 20, "[" + double2string(abs(deltaResistance[4]),1) + "]");
//  Heltec.display->drawString(x+105, y + 30, "[" + double2string(abs(deltaResistance[5]),1) + "]");
//  Heltec.display->drawString(x+105, y + 40, "[" + double2string(abs(deltaResistance[6]),1) + "]");
//  Heltec.display->drawString(x+105, y + 50, "[" + double2string(abs(deltaResistance[7]),1) + "]");
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

void setBuzzer(int intensity, int toneLevel) {
  ledcWrite(channel, intensity); //(0-230)
  ledcWriteTone(channel, toneLevel);
}

void getResistance() {
//    resistance[0] = random(2623, 2631)/10.0; 
//    resistance[1] = random(4334, 4365)/10.0; 
//    resistance[2] = random(8773, 8782)/10.0; 
//    resistance[3] = random(28072, 28082)/10.0; 
//    resistance[4] = random(28072, 28082)/10.0; 
//    resistance[5] = random(8773, 8782)/10.0; 
//    resistance[6] = random(4334, 4365)/10.0; 
//    resistance[7] = random(244, 246)/10.0; 
//
//    deltaResistance[0] = random(2, 5)/10.0; 
//    deltaResistance[1] = random(2, 8)/10.0;  
//    deltaResistance[2] = random(2, 8)/10.0; 
//    deltaResistance[3] = random(2, 7)/10.0; 
//    deltaResistance[4] = random(2, 6)/10.0;  
//    deltaResistance[5] = random(2, 5)/10.0; 
//    deltaResistance[6] = random(2, 23)/10.0;  
//    deltaResistance[7] = random(2, 4)/10.0; 
    
  int i = 0;
  sensorVoltage = 0;
  currentTime = micros();
  //Serial.println(micros());
  double timeElapsed = ((double)(currentTime - previousTime))/1000000;
  //Serial.println(timeElapsed/1000000);
  for (i = 0; i < 4; i++) {
    double resistanceMeasurement;
    
    sensorVoltage = ads1.readADC_SingleEnded(i)*0.000125;
    resistanceMeasurement = Rref * ((Vin / sensorVoltage)-1);
    deltaResistance[i] = (resistance[i] - resistanceMeasurement)/timeElapsed;
    resistance[i]  = resistanceMeasurement;
    
//    sensorVoltage = ads2.readADC_SingleEnded(i)*0.000125;
//    
//    resistanceMeasurement = Rref * ((Vin / sensorVoltage)-1);
//    //Serial.println((String)resistanceMeasurement + ":" + (String)resistance[i+4] + ":" + currentTime + ":" + previousTime+":" +(String)timeElapsed);
//    deltaResistance[i+4] = (resistance[i+4] - resistanceMeasurement)/timeElapsed;
//    resistance[i+4] = resistanceMeasurement;
  }   
  previousTime = currentTime;
  Serial.print((String)deltaResistance[0] + " - " + (String)deltaResistance[1] + " - " + (String)deltaResistance[2] + " - " + (String)deltaResistance[3] + " - ");
  //Serial.println((String)deltaResistance[4] + " - " + (String)deltaResistance[5] + " - " + (String)deltaResistance[6] + " - " + (String)deltaResistance[7]);                                                                                                   
  //Serial.println(deltaResistance[0]);
}

void transmitBLEPacket() {
  if (deviceConnected) {
      relativeTime = millis();
      String dataPacket = "";
      dataPacket = dataPacket + "A" + (String)deviceName + "/";
      dataPacket = dataPacket + "B" + (String)relativeTime + "/";
      dataPacket = dataPacket + "C" + (String)temperature + "/";
      dataPacket = dataPacket + "D" + (String)humidity + "/";
      dataPacket = dataPacket + "E" + (String)resistance[0] + "/";
      dataPacket = dataPacket + "F" + (String)deltaResistance[0] + "/";
      dataPacket = dataPacket + "G" + (String)resistance[1] + "/";
      dataPacket = dataPacket + "H" + (String)deltaResistance[1] + "/";
      dataPacket = dataPacket + "I" + (String)resistance[2] + "/";
      dataPacket = dataPacket + "J" + (String)deltaResistance[2] + "/";
      dataPacket = dataPacket + "K" + (String)resistance[3] + "/";
      dataPacket = dataPacket + "L" + (String)deltaResistance[3] + "/";
//      dataPacket = dataPacket + "M" + (String)resistance[4] + "/";
//      dataPacket = dataPacket + "N" + (String)deltaResistance[4] + "/";
//      dataPacket = dataPacket + "O" + (String)resistance[5] + "/";
//      dataPacket = dataPacket + "P" + (String)deltaResistance[5] + "/";
//      dataPacket = dataPacket + "Q" + (String)resistance[6] + "/";
//      dataPacket = dataPacket + "R" + (String)deltaResistance[6] + "/";
//      dataPacket = dataPacket + "S" + (String)resistance[7] + "/";
//      dataPacket = dataPacket + "T" + (String)deltaResistance[7] + "/";
      dataPacket = dataPacket + "U" + (String)ppm;
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
      delay(100); // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // restart advertising
      Serial.println("start advertising");
      oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) { // connecting
      oldDeviceConnected = deviceConnected;
  }
}

void transmitLORAPacket() {
  // send packet
  Heltec.LoRa.beginPacket();
      relativeTime = millis();
      String dataPacket2 = "";
      dataPacket2 = dataPacket2 + "A" + (String)deviceName + "/";
      dataPacket2 = dataPacket2 + "B" + (String)relativeTime + "/";
      dataPacket2 = dataPacket2 + "C" + (String)temperature + "/";
      dataPacket2 = dataPacket2 + "D" + (String)humidity + "/";
      dataPacket2 = dataPacket2 + "E" + (String)resistance[0] + "/";
      dataPacket2 = dataPacket2 + "F" + (String)deltaResistance[0] + "/";
      dataPacket2 = dataPacket2 + "G" + (String)resistance[1] + "/";
      dataPacket2 = dataPacket2 + "H" + (String)deltaResistance[1] + "/";
      dataPacket2 = dataPacket2 + "I" + (String)resistance[2] + "/";
      dataPacket2 = dataPacket2 + "J" + (String)deltaResistance[2] + "/";
      dataPacket2 = dataPacket2 + "K" + (String)resistance[3] + "/";
      dataPacket2 = dataPacket2 + "L" + (String)deltaResistance[3] + "/";
//      dataPacket2 = dataPacket2 + "M" + (String)resistance[4] + "/";
//      dataPacket2 = dataPacket2 + "N" + (String)deltaResistance[4] + "/";
//      dataPacket2 = dataPacket2 + "O" + (String)resistance[5] + "/";
//      dataPacket2 = dataPacket2 + "P" + (String)deltaResistance[5] + "/";
//      dataPacket2 = dataPacket2 + "Q" + (String)resistance[6] + "/";
//      dataPacket2 = dataPacket2 + "R" + (String)deltaResistance[6] + "/";
//      dataPacket2 = dataPacket2 + "S" + (String)resistance[7] + "/";
//      dataPacket2 = dataPacket2 + "T" + (String)deltaResistance[7] + "/";
      dataPacket2 = dataPacket2 + "U" + (String)ppm;
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
  if (usbIn > 3) {
    usbPluggedIn = true;
  }
  else {
    usbPluggedIn = false;
  }
  //Serial.println("Battery:" + (String)usbIn);
}

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
