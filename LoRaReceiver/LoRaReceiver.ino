#include "heltec.h"
#include "images.h"


// Enables
int BT_enable = 0;
int lora_enable = 0;

// LoRa
#include <stdio.h>
#include <string.h>
#define BAND    433E6  //you can set band here directly,e.g. 868E6,915E6

// Firebase
#include <WiFi.h>
#include <FirebaseESP32.h>
#define FIREBASE_HOST "graphene-pcb-01.firebaseio.com"
#define FIREBASE_AUTH "xBwc6woVX1si03ByKGmgjbozxS9w7raAIZhWtD4G"
#define WIFI_SSID "199Russell"
#define WIFI_PASSWORD "Mce7576s2t"
FirebaseData firebaseData;
//#define WIFI_SSID "Esp32"
//#define WIFI_PASSWORD "miniclip"

// Time
#include "time.h"
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

String packet;
String deviceName;
int newConnect = 0;
unsigned long relativeTime = 0;
double resistance[8];
double deltaResistance[8];
double temperature = 0;
double humidity = 0;
double ppm = 0;
String timeStamp;

void setup() {
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  Heltec.display->flipScreenVertically();
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
    //WiFi.reconnect();
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  if(Firebase.setInt(firebaseData, "/Status", 1)) {Serial.println("Set int data success");} 
  else {
    Serial.print("Error in setInt, ");
    Serial.println(firebaseData.errorReason());
  }
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
}

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

void updateFirebase() {
  printLocalTime();
  if (newConnect == 0) {Firebase.setString(firebaseData, "/Status/" + deviceName, timeStamp);}
  newConnect = 1;
  Serial.println("hi----");
  Firebase.setString(firebaseData, deviceName + "/" + timeStamp + "/device_Name", deviceName);
  Firebase.setDouble(firebaseData, deviceName + "/" + timeStamp +  "/relativeTime", relativeTime);
  Firebase.setDouble(firebaseData, deviceName + "/" + timeStamp +  "/Temperature", temperature);
  Firebase.setDouble(firebaseData, deviceName + "/" + timeStamp +  "/Humidity", humidity);
  Firebase.setDouble(firebaseData, deviceName + "/" + timeStamp +  "/Resistance_1", resistance[0]);
  Firebase.setDouble(firebaseData, deviceName + "/" + timeStamp +  "/delta_Resistance_1", deltaResistance[0]);
  Firebase.setDouble(firebaseData, deviceName + "/" + timeStamp +  "/Resistance_2", resistance[1]);
  Firebase.setDouble(firebaseData, deviceName + "/" + timeStamp +  "/delta_Resistance_2", deltaResistance[1]);
  Firebase.setDouble(firebaseData, deviceName + "/" + timeStamp +  "/Resistance_3", resistance[2]);
  Firebase.setDouble(firebaseData, deviceName + "/" + timeStamp +  "/delta_Resistance_3", deltaResistance[2]);
  Firebase.setDouble(firebaseData, deviceName + "/" + timeStamp +  "/Resistance_4", resistance[3]);
  Firebase.setDouble(firebaseData, deviceName + "/" + timeStamp +  "/delta_Resistance_4", deltaResistance[3]);
  Firebase.setDouble(firebaseData, deviceName + "/" + timeStamp +  "/Resistance_5", resistance[4]);
  Firebase.setDouble(firebaseData, deviceName + "/" + timeStamp +  "/delta_Resistance_5", deltaResistance[4]);
  Firebase.setDouble(firebaseData, deviceName + "/" + timeStamp +  "/Resistance_6", resistance[5]);
  Firebase.setDouble(firebaseData, deviceName + "/" + timeStamp +  "/delta_Resistance_6", deltaResistance[5]);
  Firebase.setDouble(firebaseData, deviceName + "/" + timeStamp +  "/Resistance_7", resistance[6]);
  Firebase.setDouble(firebaseData, deviceName + "/" + timeStamp +  "/delta_Resistance_7", deltaResistance[6]);
  Firebase.setDouble(firebaseData, deviceName + "/" + timeStamp +  "/Resistance_8", resistance[7]);
  Firebase.setDouble(firebaseData, deviceName + "/" + timeStamp +  "/delta_Resistance_8", deltaResistance[7]);
}

void displayData() {
  int x = 0;
  int y = 0;
  Heltec.display->drawXbm(x, y, Wifi_on_width, Wifi_on_height, Wifi_on_bits);
  if (BT_enable == 1) {Heltec.display->drawXbm(x+18, y, Bluetooth_on_width, Bluetooth_on_height, Bluetooth_on_bits);}
  else {Heltec.display->drawXbm(x+18, y, Bluetooth_off_width, Bluetooth_off_height, Bluetooth_off_bits);}
  if (lora_enable == 1) {Heltec.display->drawXbm(x+30, y, Lora_on_width, Lora_on_height, Lora_on_bits);}
  else {Heltec.display->drawXbm(x+30, y, Lora_off_width, Lora_off_height, Lora_off_bits);}
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawLine(62, 22,62, 80);
  Heltec.display->drawString(x, y + 10, "T=" + double2string(temperature,1) + "°C");
  Heltec.display->drawString(x+65, y + 10, "RH=" + double2string(humidity,0) + "%");
  Heltec.display->drawString(x, y + 20, "R1:" + double2string(resistance[0],0));
  Heltec.display->drawString(x, y + 30, "R2:" + double2string(resistance[1],0));
  Heltec.display->drawString(x, y + 40, "R3:" + double2string(resistance[2],0));
  Heltec.display->drawString(x, y + 50, "R4:" + double2string(resistance[3],0));
  Heltec.display->drawString(x+40, y + 20, "[" + double2string(deltaResistance[0],1) + "]");
  Heltec.display->drawString(x+40, y + 30, "[" + double2string(deltaResistance[1],1) + "]");
  Heltec.display->drawString(x+40, y + 40, "[" + double2string(deltaResistance[2],1) + "]");
  Heltec.display->drawString(x+40, y + 50, "[" + double2string(deltaResistance[3],1) + "]");
  Heltec.display->drawString(x+65, y + 20, "R5:" + double2string(resistance[4],0));
  Heltec.display->drawString(x+65, y + 30, "R6:" + double2string(resistance[5],0));
  Heltec.display->drawString(x+65, y + 40, "R7:" + double2string(resistance[6],0));
  Heltec.display->drawString(x+65, y + 50, "R8:" + double2string(resistance[7],0));
  Heltec.display->drawString(x+105, y + 20, "[" + double2string(deltaResistance[4],1) + "]");
  Heltec.display->drawString(x+105, y + 30, "[" + double2string(deltaResistance[5],1) + "]");
  Heltec.display->drawString(x+105, y + 40, "[" + double2string(deltaResistance[6],1) + "]");
  Heltec.display->drawString(x+105, y + 50, "[" + double2string(deltaResistance[7],1) + "]");
}

void loop() {
  // try to parse packet
  Serial.println("hi");
  int packetSize = Heltec.LoRa.parsePacket();
  if (packetSize) {
    while (Heltec.LoRa.available()) {
      Heltec.display->clear();
      displayData();
      Heltec.display->display();
      String packet = Heltec.LoRa.readString();

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
      resistance[0] = tempVal.toDouble();
      
      tempVal = getValue(packet,'/',5);
      tempVal.remove(0,1);
      deltaResistance[0] = tempVal.toDouble();

      tempVal = getValue(packet,'/',6);
      tempVal.remove(0,1);
      resistance[1] = tempVal.toDouble();
      
      tempVal = getValue(packet,'/',7);
      tempVal.remove(0,1);
      deltaResistance[1] = tempVal.toDouble();

      tempVal = getValue(packet,'/',8);
      tempVal.remove(0,1);
      resistance[2] = tempVal.toDouble();
      
      tempVal = getValue(packet,'/',9);
      tempVal.remove(0,1);
      deltaResistance[2] = tempVal.toDouble();

      tempVal = getValue(packet,'/',10);
      tempVal.remove(0,1);
      resistance[3] = tempVal.toDouble();
      
      tempVal = getValue(packet,'/',11);
      tempVal.remove(0,1);
      deltaResistance[3] = tempVal.toDouble();

      tempVal = getValue(packet,'/',12);
      tempVal.remove(0,1);
      resistance[4] = tempVal.toDouble();
      
      tempVal = getValue(packet,'/',13);
      tempVal.remove(0,1);
      deltaResistance[4] = tempVal.toDouble();

      tempVal = getValue(packet,'/',14);
      tempVal.remove(0,1);
      resistance[5] = tempVal.toDouble();
      
      tempVal = getValue(packet,'/',15);
      tempVal.remove(0,1);
      deltaResistance[5] = tempVal.toDouble();

      tempVal = getValue(packet,'/',16);
      tempVal.remove(0,1);
      resistance[6] = tempVal.toDouble();
      
      tempVal = getValue(packet,'/',17);
      tempVal.remove(0,1);
      deltaResistance[6] = tempVal.toDouble();

      tempVal = getValue(packet,'/',18);
      tempVal.remove(0,1);
      resistance[7] = tempVal.toDouble();
      
      tempVal = getValue(packet,'/',19);
      tempVal.remove(0,1);
      deltaResistance[7] = tempVal.toDouble();

      tempVal = getValue(packet,'/',20);
      tempVal.remove(0,1);
      ppm = tempVal.toDouble();

      printToScreen();
      updateFirebase();
    }
  }
  delay(300);
}
  
void printToScreen() {
  Serial.println(relativeTime);
  Serial.println(temperature);
  Serial.println(humidity);
  Serial.println(resistance[0]);
  Serial.println(deltaResistance[0]);
  Serial.println(ppm); 
}

void printLocalTime()
{
  time_t rawtime;
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
   return;
  }
  char timeStringBuff[50]; //50 chars should be enough
  strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
  Serial.println(timeStringBuff);
  timeStamp = (String)timeStringBuff;
}

String double2string(double n, int ndec) {
    if (ndec == 0) {
      int nn = round(n);
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
    return r;
}
