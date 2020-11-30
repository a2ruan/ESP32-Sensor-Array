#include "heltec.h"

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

//#define WIFI_SSID "Esp32"
//#define WIFI_PASSWORD "miniclip"
FirebaseData firebaseData;

String packet;
unsigned long relativeTime = 0;
double resistance[5];
double deltaResistance[5];
double temperature = 0;
double humidity = 0;
double ppm = 0;

void setup() {
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  Heltec.display->flipScreenVertically();
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
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
  if(Firebase.setInt(firebaseData, "/Status", 1)) {
     Serial.println("Set int data success");
  } 
  else {
    Serial.print("Error in setInt, ");
    Serial.println(firebaseData.errorReason());
  }
}

String getValue(String data, char separator, int index)
{
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
  Firebase.setDouble(firebaseData, "Gas_Sensor_1/Resistance", resistance[0]);
  Firebase.setDouble(firebaseData, "Gas_Sensor_1/deltaResistance", deltaResistance[0]);
  Firebase.setDouble(firebaseData, "Gas_Sensor_1/Temperature", temperature);
  Firebase.setDouble(firebaseData, "Gas_Sensor_1/Humidity", humidity);
  Firebase.setDouble(firebaseData, "Gas_Sensor_1/Time", relativeTime);
  Firebase.setDouble(firebaseData, "Gas_Sensor_1/PPM", ppm);
}

void displayData() {
  int x = 0;
  int y = 0;
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(x, y + 10, (String)ppm + " PPM");
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(x, y + 20, "TEMP: " + (String)temperature + "°C");
  Heltec.display->drawString(x, y + 30, "RH: " + (String)humidity + "%");
  Heltec.display->drawString(x, y + 40, "R: " + (String)resistance[0] + " ohm ");
  Heltec.display->drawString(x, y + 50, "dR: " + (String)deltaResistance[0] + " ohm/s ");
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
      relativeTime = tempVal.toInt();

      tempVal = getValue(packet,'/',1);
      tempVal.remove(0,1);
      temperature = tempVal.toDouble();

      tempVal = getValue(packet,'/',2);
      tempVal.remove(0,1);
      humidity = tempVal.toDouble();

      tempVal = getValue(packet,'/',3);
      tempVal.remove(0,1);
      resistance[0] = tempVal.toDouble();
      
      tempVal = getValue(packet,'/',4);
      tempVal.remove(0,1);
      deltaResistance[0] = tempVal.toDouble();

      tempVal = getValue(packet,'/',5);
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
