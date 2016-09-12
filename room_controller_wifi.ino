#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Servo.h>
#include <Wire.h>
#include <Time.h>
#include <DS3232RTC.h>
#include <RCSwitch.h>


#define ONE_WIRE_BUS 2
#define LAMP_PIN 12
#define SMALL_AQ_PIN 13
#define TRANSMITTER_PIN 15

const int lightSensor = A0;

const String serverDomain = ""; // backend server domain
const char* routerSSID = ""; // access point name
const char* routerPassword = ""; // access point password
const char* userId = ""; // user id on user_record (backend server)
const char* userPassword = ""; // // user password on user_record (backend server)
 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Servo servo1;
RCSwitch radioTran = RCSwitch();

int lampOpen = true;
int smAquariumOpen = true;
int lightOpen = true;
int acOpen = true;
int pressAcButton = false;
int sentTemp = true;
int updateSet = false;

int restartStatus;
float celcius;
int currentHour;
int currentMin;
int currentSec;

int lastSecDiv = 0;
int currentSecDiv;
int lastMin;

int smAqOpenHour;
int smAqOpenMinute;
int smAqCloseHour;
int smAqCloseMinute;

String URL;
int httpCode;
String payload;
String lastSettingString = "'27`00`0`12`30`0`16`30`0`21`30`02`30`1`1`1`1`1'";
String stringCache;
String settingCode;


int acStatus = true;
int upperBoundInt;
float upperBoundTemp;
int tempAutoMode;
int acOpenHour;
int acOpenMinute;
int acCloseHour;
int acCloseMinute;
int acOpenAutoMode;
int acCloseAutoMode;
int httpCheck;
boolean acOpenChecked = false;
boolean acCloseChecked = false;

void updateTemp(float celciusTemp) {
  HTTPClient http;
  URL = serverDomain + "room_update_temperature?temperature=" + String(celciusTemp) + "&username=" + userId + "&password=" + userPassword;
  http.begin(URL);
  httpCode = http.GET();
  delay(1000);
  if (httpCode) {
//    Serial.printf("[HTTP] Update temperature to server... code: %d\n", httpCode);

    // if(httpCode == 200) {
    //    payload = http.getString();
    //    return payload;
    // }
  } else {
//    Serial.print("[HTTP] GET... failed, no connection or no HTTP server\n");

  }
  // return lastStatusString;
}

String updateSettings() {
  HTTPClient http;
  URL = serverDomain + "room_update_setting";
  http.begin(URL);
  httpCode = http.GET();
  delay(1000);
  if (httpCode) {
//    Serial.printf("[HTTP] Update to server... code: %d\n", httpCode);

    if (httpCode == 200) {
      payload = http.getString();
      return payload;
    }
  } else {
//    Serial.print("[HTTP] GET... failed, no connection or no HTTP server\n");

  }
  return lastSettingString;
}

int toggleAc() {
  lastSettingString.setCharAt(45, char(!acStatus));


  HTTPClient http;
  URL = serverDomain + "room_ac?username=" + userId + "&password=" + userPassword;
  http.begin(URL);
  httpCode = http.GET();
  delay(1000);
  if (httpCode) {
//    Serial.printf("[HTTP] Send open relay request to server... code: %d\n", httpCode);
    return httpCode;
  } else {
//    Serial.print("[HTTP] GET... failed, no connection or no HTTP server\n");
    return 0;
  }
}

int commandAc(int acStat) {
  lastSettingString.setCharAt(41, char(acStat));


  HTTPClient http;
  URL = serverDomain + "room_command_ac?ac_status=" + String(acStat) + "&username=" + userId + "&password=" + userPassword;
  http.begin(URL);
  httpCode = http.GET();
  delay(1000);
  if (httpCode) {
//    Serial.printf("[HTTP] Send open relay request to server... code: %d\n", httpCode);
    return httpCode;
  } else {
//    Serial.print("[HTTP] GET... failed, no connection or no HTTP server\n");
    return 0;
  }
}

int openAquarium(int aqStat) {
  
  lastSettingString.setCharAt(39, char(aqStat));


  HTTPClient http;
  URL = serverDomain + "small_aquarium?aquarium_status=" + String(aqStat) + "&username=" + userId + "&password=" + userPassword;
  http.begin(URL);
  httpCode = http.GET();
  delay(1000);
  if (httpCode) {
//    Serial.printf("[HTTP] Send open relay request to server... code: %d\n", httpCode);
    return httpCode;
  } else {
//    Serial.print("[HTTP] GET... failed, no connection or no HTTP server\n");
    return 0;
  }
}

int restartPing() {
  HTTPClient http;
  URL = serverDomain + "room_restart_ping";
  http.begin(URL);
  httpCode = http.GET();
  delay(1000);
  if (httpCode) {
//    Serial.printf("[HTTP] Send restart ping to server... code: %d\n", httpCode);
    return httpCode;
  } else {
//    Serial.print("[HTTP] GET... failed, no connection or no HTTP server\n");
    return 0;
  }
}





float getSampleTemperature(){
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

void pressTheButton(){
  servo1.write(178);
  delay(1000);
  servo1.write(90);
  delay(1000);
}


void setup(void)
{
  Serial.begin(115200);
  sensors.begin();
  servo1.attach(16);
  servo1.write(90);
  delay(1000);
  
  // Serial.print("Configuring access point...");
  WiFi.mode(WIFI_STA);

  IPAddress myIP = WiFi.softAPIP();
  // Serial.print("AP IP address: ");
  // Serial.println(myIP);
  
  // Connect to WiFi network
  WiFi.begin(routerSSID, routerPassword);
  // Serial.println("");  
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    // Serial.print(".");
  }
  // Serial.println("");
  // Serial.print("Connected to ");
  // Serial.println(routerSSID);
  // Serial.print("IP address: ");
  // Serial.println(WiFi.localIP());

  setSyncProvider(RTC.get);

  if (restartPing() == 200) {
    restartStatus = true;
  }

  pinMode(LAMP_PIN, OUTPUT);
  pinMode(SMALL_AQ_PIN, OUTPUT);
  pinMode(lightSensor, INPUT);
  radioTran.enableTransmit(TRANSMITTER_PIN);
}
 
void loop(void)
{
    if (!restartStatus) {
      if (restartPing() == 200) {
        restartStatus = true;
      }
    }
    
    currentHour = hour();
    currentMin = minute();
    currentSec = second();
//    Serial.print(currentHour);
//    Serial.print(":");
//    Serial.print(currentMin);
//    Serial.print(":");
//    Serial.println(currentSec);
    
    pressAcButton = false;
    
    currentSecDiv = (currentSec + 10) / 10;



    if (currentSecDiv - lastSecDiv != 0) {
      updateSet = false;
      lastSecDiv = currentSecDiv;
    }

    if (currentMin % 2 == 0) {
      if (currentMin != lastMin) {
        sentTemp = false;
        lastMin = currentMin;
      }

    }

    if (!updateSet) {
    settingCode = updateSettings();
    lastSettingString = settingCode;

    stringCache = "";
    stringCache += settingCode.charAt(1);
    stringCache += settingCode.charAt(2);
    upperBoundInt = stringCache.toInt();

    stringCache = "";
    stringCache += settingCode.charAt(4);
    stringCache += settingCode.charAt(5);
    upperBoundTemp = stringCache.toInt();
    upperBoundTemp = upperBoundInt + (((float)upperBoundTemp)*0.01);

    stringCache = "";
    stringCache += settingCode.charAt(7);
    tempAutoMode = stringCache.toInt();

    stringCache = "";
    stringCache += settingCode.charAt(9);
    stringCache += settingCode.charAt(10);
    acOpenHour = stringCache.toInt();

    stringCache = "";
    stringCache += settingCode.charAt(12);
    stringCache += settingCode.charAt(13);
    acOpenMinute = stringCache.toInt();

    stringCache = "";
    stringCache += settingCode.charAt(15);
    acOpenAutoMode = stringCache.toInt();

    stringCache = "";
    stringCache += settingCode.charAt(17);
    stringCache += settingCode.charAt(18);
    acCloseHour = stringCache.toInt();

    stringCache = "";
    stringCache += settingCode.charAt(20);
    stringCache += settingCode.charAt(21);
    acCloseMinute = stringCache.toInt();

    stringCache = "";
    stringCache += settingCode.charAt(23);
    acCloseAutoMode = stringCache.toInt();

    stringCache = "";
    stringCache += settingCode.charAt(25);
    stringCache += settingCode.charAt(26);
    smAqOpenHour = stringCache.toInt();

    stringCache = "";
    stringCache += settingCode.charAt(28);
    stringCache += settingCode.charAt(29);
    smAqOpenMinute = stringCache.toInt();

    stringCache = "";
    stringCache += settingCode.charAt(31);
    stringCache += settingCode.charAt(32);
    smAqCloseHour = stringCache.toInt();

    stringCache = "";
    stringCache += settingCode.charAt(34);
    stringCache += settingCode.charAt(35);
    smAqCloseMinute = stringCache.toInt();


    stringCache = "";
    stringCache += settingCode.charAt(37);
    lampOpen = stringCache.toInt();
    
    stringCache = "";
    stringCache += settingCode.charAt(39);
    smAquariumOpen = stringCache.toInt();

    stringCache = "";
    stringCache += settingCode.charAt(41);
    acOpen = stringCache.toInt();
//    Serial.print("AC open = ");
//    Serial.println(acOpen);
    
    stringCache = "";
    stringCache += settingCode.charAt(43);
    lightOpen = stringCache.toInt();

    stringCache = "";
    stringCache += settingCode.charAt(45);
    acStatus = stringCache.toInt();
//    Serial.print("Top AC status = ");
//    Serial.println(acStatus);

    

    if (acOpen && !acStatus){
      pressAcButton = true;
    }

    if (!acOpen && acStatus){
      pressAcButton = true;
    }
    
    if (analogRead(lightSensor) > 400){
      if (!lightOpen){
        radioTran.send("000000000001010100010001");
      }
     }
    else {
      if (lightOpen){
        radioTran.send("000000000001010100010001");
      }
    }

  }

  updateSet = true;

  if (currentHour == acOpenHour && currentMin == acOpenMinute && acOpenAutoMode && acStatus){
      if (!acOpenChecked){
        if(commandAc(!acStatus) == 200){
          acOpenChecked = true;
        }
        
      }
    }
    else{
      acOpenChecked = false;
    }

    if (currentHour == acCloseHour && currentMin == acCloseMinute && acCloseAutoMode && !acStatus){
      if (!acCloseChecked){
        if(commandAc(!acStatus) == 200){
          acCloseChecked = true;
        }
        
      }
    }
    else{
      acCloseChecked = false;
    }
  

  if (currentHour == smAqOpenHour && currentMin == smAqOpenMinute && smAquariumOpen) {
    smAquariumOpen = false;
    openAquarium(smAquariumOpen);
  }

  if (currentHour == smAqCloseHour && currentMin == smAqCloseMinute && !smAquariumOpen) {
    smAquariumOpen = true;
    openAquarium(smAquariumOpen);
  }


    if (!sentTemp) {
      celcius = getSampleTemperature();
      delay(50);
      
      updateTemp(celcius);
      if (celcius < 50){
//        Serial.print("ac status: ");
//        Serial.println(acStatus);
//        Serial.print("temp auto mode: ");
//        Serial.println(tempAutoMode);
//        Serial.print("upper bound temp: ");
//        Serial.println(upperBoundTemp);
//        Serial.print("current temp: ");
//        Serial.println(celcius);
        if (celcius > upperBoundTemp && acStatus && tempAutoMode){
//          Serial.println("Temperature exeeded commanding ac");
          commandAc(!acStatus);
        }
      }
    }

    sentTemp = true;

    if (pressAcButton){
      pressTheButton();
      toggleAc();      
    }

    
    digitalWrite(LAMP_PIN, lampOpen);
    digitalWrite(SMALL_AQ_PIN, smAquariumOpen);
     
     

    delay(10);
}
