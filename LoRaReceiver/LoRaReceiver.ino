#include "EEPROM.h"
#include "heltec.h"
#include "images.h"
#include <HTTPClient.h>

// Credentials
String deviceName = "Gateway_1";
String WIFI_SSID = "199Russell";
String WIFI_PASSWORD = "Mce7576s2te";
String GOOGLE_SHEETS_ENABLE = "1"; // 1 = enabled, 0 = disabled
String FIREBASE_ENABLE = "0"; // 1 = enabled,  0 = disabled
String GOOGLE_SCRIPT_ID = "AKfycbwuRsl_vnO6jB8m-IcXgFQaCaF-7UTydW8CPirmV1G2nzcwfbId";
String FIREBASE_HOST = "graphene-pcb-01.firebaseio.com";
String FIREBASE_AUTH = "xBwc6woVX1si03ByKGmgjbozxS9w7raAIZhWtD4G";

// Enables
int BT_enable = 0;
int lora_enable = 0;

// LoRa
#include <stdio.h>
#include <string.h>
#define BAND    915E6  //you can set band here directly,e.g. 868E6,915E6

// Bluetooth Classic
#include "BluetoothSerial.h"
BluetoothSerial SerialBT;
char incomingBluetoothPacket;

// Firebase
#include <WiFi.h>
String locale;
#include <FirebaseESP32.h>
FirebaseData firebaseData;

// Time
#include "time.h"
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

// LoRA Packet Formatting
String packet;
int newConnect = 0;
unsigned long relativeTime = 0;
double resistance[8];
double deltaResistance[8];
double temperature = 0;
double humidity = 0;
double ppm = 0;
String dateStamp;
String timeStamp;
#define ARRAYSIZE 10
String sendCodeStack[ARRAYSIZE] = {""};

// BLE Transmission
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
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
// Google Sheets Packet Outgoing

/**
 * Second main loop running on dual core processor Core 0;
 * This enables asynchronous communication with Google sheets
 */
TaskHandle_t Task1;
void codeForTask1( void * parameter ) {
  for(;;) {
          //Serial.print("This Task run on Core: ");
          //Serial.println(xPortGetCoreID());
          sendToSheets();
  }
}

void setup() {
  // Initialize device
  Serial.begin(115200);
  Heltec.begin(true, true , true, true , BAND ); // display, lora, serial, paboost, band
  Heltec.display->flipScreenVertically();
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "Connecting to Wifi...");
  Heltec.display->display();
  
  // Begin Wifi Reconnection using default WIFI password/user
  WiFi.begin(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());
  for (int i = 0; i < 8; i++) {
    WiFi.begin(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());
    delay(600);
  }
  
  //Begin Wifi Reconnection using STORED MEMORY WIFI password/user
  if (WiFi.status() != WL_CONNECTED) {  
    if (!EEPROM.begin(1000)) {
    Serial.println("Failed to initialise EEPROM.  Restarting..");
    delay(400);
    ESP.restart();
  }
    String ssid_sto = EEPROM.readString(0);
    String password_sto = EEPROM.readString(100);
    ssid_sto.trim();
    password_sto.trim();
    for (int i = 0; i < 8; i++) {
      WiFi.begin(ssid_sto.c_str(), password_sto.c_str());
      delay(600);
    }
  }
  
  // Prompt user to reenter Wifi credentials using bluetooth
  if (WiFi.status() != WL_CONNECTED) {reconnectWifi();}
  
  // Begin firebase connection
  locale = WiFi.localIP().toString();
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  if(Firebase.setInt(firebaseData, "/Status", 1)) {Serial.println("Set int data success");} 
  else {
    Serial.print("Error in setInt, ");
    Serial.println(firebaseData.errorReason());
  }

  // Get most recent time from internet
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
  
  // Start receieving transmission and update UI screen
  LoRa.setSpreadingFactor(7);
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "Success!");
  Heltec.display->drawString(0, 10, "Connected to: ");
  Heltec.display->drawString(0, 20, locale);
  Heltec.display->drawString(0, 30, "Scanning for LoRA...");
  Heltec.display->display();
  xTaskCreatePinnedToCore(codeForTask1,"Task_1",20000,NULL,1,&Task1,0);
}

/**
 * Bluetooth feature that prompts user to change the Wifi username and password if invalid
 * If connection is successful, the Wifi username and password will be replaced by the new one, in EEPROM
 */
void reconnectWifi() {
  if (!EEPROM.begin(1000)) {
    Serial.println("Failed to initialise EEPROM.  Restarting..");
    delay(400);
    ESP.restart();
  }
  String ssid = EEPROM.readString(0);
  String password = EEPROM.readString(100);
  ssid.trim();
  password.trim();
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "Reconnect WiFi via app");
  Heltec.display->drawString(0, 10, "Available as Bluetooth Serial");
  Heltec.display->drawString(0, 20, "on the google play app store");
  Heltec.display->display();
  String message = "";
  SerialBT.begin(deviceName);
  while (WiFi.status() != WL_CONNECTED) {
    if (SerialBT.available()){
        message = SerialBT.readString();
        message.replace(" ", "");
        Serial.println(message);
        if (message.length() > 4 && message.indexOf("=") > 0) {
          String command = message.substring(0,message.indexOf("="));
          command.toLowerCase();
          Serial.println(command);
          if (command == "ssid") {
            EEPROM.writeString(0,message.substring(message.indexOf("=")+1,message.length()));
            delay(100);
            EEPROM.commit();
            ssid = EEPROM.readString(0);
            ssid.trim();

          }
          else if (command == "pw") {
            EEPROM.writeString(100,message.substring(message.indexOf("=")+1,message.length()));
            delay(100);
            EEPROM.commit();
            password = EEPROM.readString(100);
            password.trim();
          }
          else if (command == "connect") {
            int tries = 0;
            int maxTries = 15;
            while (WiFi.status() != WL_CONNECTED && tries < maxTries) {
              Heltec.display->clear();
              Heltec.display->drawString(0, 0, "Reconnect Attempts: " + (String)tries);
              Heltec.display->display();
              ssid = EEPROM.readString(0);
              ssid.trim();
              password = EEPROM.readString(100);
              password.trim();
              WiFi.begin(ssid.c_str(), password.c_str());
              tries = tries + 1;
              delay(600);
            }
          }
        }
        if (WiFi.status() != WL_CONNECTED) {
          Heltec.display->clear();
          Heltec.display->drawString(0, 0, "Connected to Bluetooth!");
          Heltec.display->drawString(0, 10, "Connect to wifi using");
          Heltec.display->drawString(0, 20, "the following commands:");
          Heltec.display->drawString(0, 30, "[1] ssid=" + ssid);
          Heltec.display->drawString(0, 40, "[2] pw=" + password);
          Heltec.display->drawString(0, 50, "[3] connect=1");
          Heltec.display->display();
        }
        else {
          EEPROM.writeString(0,ssid);
          EEPROM.writeString(100,password);
          Heltec.display->clear();
          Heltec.display->drawString(0, 0, "Connected to WiFi!");
          Heltec.display->display();
        }
   }
  } 
    delay(200);
    Serial.println(WiFi.status());
    SerialBT.flush();
    SerialBT.end();
}

/**
 * Returns a substring of a mainstring based on index number and the seperator
 * E.x A1/B2/C3    If using a "/" seperator, the 2nd term would be "B2".
 */
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

/**
 * FIREBASE
 * Sends local variables to Firebase
 */
void updateFirebase() {
  printLocalTime();
  if (newConnect == 0) {
    Firebase.setString(firebaseData, "/Status/" + deviceName, timeStamp);
  }
  newConnect = 1;
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
}

/**
 * Displays variables to Heltec OLED Screen
 */
void displayData() {
  int x = 0; int y = 0;
  Heltec.display->drawXbm(x, y, Wifi_on_width, Wifi_on_height, Wifi_on_bits);
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(x+15, y, "IP:"+locale);
  Heltec.display->drawString(x, y + 10, "T=" + double2string(temperature,1) + "°C");
  Heltec.display->drawString(x+65, y + 10, "RH=" + double2string(humidity,0) + "%");
  Heltec.display->drawString(x, y + 20, "R1:" + double2string(resistance[0],2));
  Heltec.display->drawString(x, y + 30, "R2:" + double2string(resistance[1],2));
  Heltec.display->drawString(x, y + 40, "R3:" + double2string(resistance[2],2));
  Heltec.display->drawString(x, y + 50, "R4:" + double2string(resistance[3],2));
  Heltec.display->drawString(x+70, y + 20, "[" + double2string(deltaResistance[0],2) + "]");
  Heltec.display->drawString(x+70, y + 30, "[" + double2string(deltaResistance[1],2) + "]");
  Heltec.display->drawString(x+70, y + 40, "[" + double2string(deltaResistance[2],2) + "]");
  Heltec.display->drawString(x+70, y + 50, "[" + double2string(deltaResistance[3],2) + "]");
}

/**
 * Main Loop
 */
String previousRaw;
void loop() {
  delay(100);
  int packetSize = Heltec.LoRa.parsePacket();
  if (packetSize) {
    while (Heltec.LoRa.available()) {
      Heltec.display->clear();
      displayData();
      Heltec.display->display();
      
      String packetRaw = Heltec.LoRa.readString();
      if (previousRaw != packetRaw) {
        previousRaw = packetRaw;
        updateVariables(packetRaw);
      }

      // Uncomment this code if you wish to upload to Firebase
      //updateVariables();
      //updateFirebase();
    }
  }
}

/**
 * FIREBASE
 * Retrieves real time clock data from the internet and stores as a string
 */
void printLocalTime() {
  time_t rawtime;
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) {
   //Serial.println("Failed to obtain time");
   return;
  }
  char timeStringBuff[50]; //50 chars should be enough
  strftime(timeStringBuff, sizeof(timeStringBuff), "%m- %d- %Y", &timeinfo);
  //Serial.println(timeStringBuff);
  dateStamp = (String)timeStringBuff;
  strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S", &timeinfo);
  timeStamp = (String)timeStringBuff;
}

/**
 * LORA
 * Retrieve LoRa transmission and store values to local variables
 * Decodes the LoRa transmission packet using delimited "/"
 */
void updateVariables(String packet) {
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
      ppm = tempVal.toDouble();

      tempVal = getValue(packet,'/',5);
      tempVal.remove(0,1);
      resistance[0] = tempVal.toDouble();
      
      tempVal = getValue(packet,'/',6);
      tempVal.remove(0,1);
      deltaResistance[0] = tempVal.toDouble();

      tempVal = getValue(packet,'/',7);
      tempVal.remove(0,1);
      resistance[1] = tempVal.toDouble();
      
      tempVal = getValue(packet,'/',8);
      tempVal.remove(0,1);
      deltaResistance[1] = tempVal.toDouble();

      tempVal = getValue(packet,'/',9);
      tempVal.remove(0,1);
      resistance[2] = tempVal.toDouble();
      
      tempVal = getValue(packet,'/',10);
      tempVal.remove(0,1);
      deltaResistance[2] = tempVal.toDouble();

      tempVal = getValue(packet,'/',11);
      tempVal.remove(0,1);
      resistance[3] = tempVal.toDouble();
      
      tempVal = getValue(packet,'/',12);
      tempVal.remove(0,1);
      deltaResistance[3] = tempVal.toDouble();

      String sendCodeNewStack = "";
      printLocalTime();
      dateStamp.replace(" ","");
      timeStamp.replace(" ","");
      sendCodeNewStack = sendCodeNewStack + "&" + "DateCode=" + dateStamp;
      sendCodeNewStack = sendCodeNewStack + "&" + "TimeCode=" + timeStamp;
      sendCodeNewStack = sendCodeNewStack + "&" + "Temperature=" + temperature;
      sendCodeNewStack = sendCodeNewStack + "&" + "Humidity=" + humidity;
      sendCodeNewStack = sendCodeNewStack + "&" + "R1=" + resistance[0];
      sendCodeNewStack = sendCodeNewStack + "&" + "dR1=" + deltaResistance[0];
      sendCodeNewStack = sendCodeNewStack + "&" + "R2=" + resistance[1];
      sendCodeNewStack = sendCodeNewStack + "&" + "dR2=" + deltaResistance[1];
      sendCodeNewStack = sendCodeNewStack + "&" + "R3=" + resistance[2];
      sendCodeNewStack = sendCodeNewStack + "&" + "dR3=" + deltaResistance[2];
      sendCodeNewStack = sendCodeNewStack + "&" + "R4=" + resistance[3];
      sendCodeNewStack = sendCodeNewStack + "&" + "dR4=" + deltaResistance[3];
      //Serial.println("1");
      // Shift stack
      String sendCodeStackOld[ARRAYSIZE];
      for (int i = 0; i < ARRAYSIZE; i++) {
        sendCodeStackOld[i] = sendCodeStack[i];
      }
      for (int i = 0; i < ARRAYSIZE-1; i++) {
        sendCodeStack[i+1] = sendCodeStackOld[i];
      }
      sendCodeStack[0] = sendCodeNewStack;
      for (int i = 0; i < ARRAYSIZE; i++) {
        //Serial.println("i:" + (String)i + "---" + sendCodeStack[i]);
      }
      //Serial.println("-------------------------");
}

/**
 * LORA
 * Send data to google sheet with specified Google Script ID using HTML.get request
 * @sheetName is the name of the sheet within the Google docs to write to
 * @sendCode is the variable names and values encoded using Google script formatting
 */
void sendToSheets() {
  // Get stack count
  String sendCode = "";
  int stackCount = 0;
  for (int i = 0; i < ARRAYSIZE; i++) {
    //Serial.println("i=" + (String)i);
    if (sendCodeStack[i] != "") {stackCount = stackCount + 1;} 
  }
  // Prepare sendCode
  for (int i = 0; i < stackCount; i++) {
    //Serial.println("BEFORE:"+sendCodeStack[i]);
    sendCodeStack[i].replace("&","&row"+(String)i);
    //Serial.println("AFTER:"+(String)i+"--"+sendCodeStack[i]);
    sendCode = sendCode + sendCodeStack[i];
  }
  // Reset Stack to blank characters
  for (int i = 0; i < ARRAYSIZE; i++) {
    sendCodeStack[i] = "";
  }

  if (sendCode != "") {
    String sheetName = "Data"; // Name of the sheet inside the Google sheets workbook
    HTTPClient http;
    String url="https://script.google.com/macros/s/"+GOOGLE_SCRIPT_ID+"/exec?id=" + sheetName +sendCode;
    Serial.println(url);
    http.begin(url); //Specify the URL and certificate
    int httpCode = http.GET();  
    http.end();
  }
  else {delay(500);}
}

/**
 * Convert a double to a string with a specified number of decimal places
 */
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
