#include <stdio.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h> 
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

/* Set these to your desired credentials. */

const char *host = "api.bitpanda.com";
const int httpsPort = 443; //HTTPS= 443 and HTTP = 80
const String path = "/v1/ticker?market=";
const String pairing = "_USD";

const String symbols[3] = {"BTC", "ETH", "DOGE"};

uint8_t symbolIndex = 0; 
 

char line1[17] = {};
LiquidCrystal_I2C lcd(0x27,16,2);

DynamicJsonDocument doc(255);
String jsonResponse;

float rawPrice = 0;



//=======================================================================
//                    Power on setup
//=======================================================================
void setup() {
  delay(1000);
  Serial.begin(115200);
  WiFi.mode(WIFI_OFF);        //Prevents reconnection issue (taking too long to connect)
  delay(1000);
  WiFi.mode(WIFI_STA);        //Only Station No AP, This line hides the viewing of ESP as wifi hotspot
  WiFiManager wm;
  
  bool res;
  res = wm.autoConnect("Coin Tracker","12345678");

  if(!res) {
      Serial.println("Failed to connect");
      ESP.restart();
  } 
  else {
        //if you get here you have connected to the WiFi    
      Serial.println("Kết nối thành công: )");
  }
  ArduinoOTA.setPassword((const char *)"12345679");
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
 
  lcd.init();
  lcd.backlight();
}
 
//=======================================================================
//                    Main Program Loop
//=======================================================================
void loop() {
  ArduinoOTA.handle();
  WiFiClientSecure client;    //Declare object of class WiFiClient
  client.setInsecure();
  Serial.println(host);
 

  
  Serial.print("HTTPS Connecting");
  int r=0; //retry counter
  while((!client.connect(host, httpsPort)) && (r < 30)){
      delay(100);
      Serial.print(".");
      r++;
  }
  if(r==30) {
    Serial.println("Connection failed");
  }
  else {
    Serial.println("Connected to web");
  }
 
  Serial.print("requesting URL: ");
  Serial.println(host + path + symbols[symbolIndex] + pairing);
  
  client.print(String("GET ") + path + symbols[symbolIndex] + pairing + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +               
               "Connection: close\r\n\r\n");
 
  Serial.println("request sent");
  String res = client.readString();
  Serial.println("res");
  Serial.println(res);
  String json = res.substring(res.indexOf('{'), res.indexOf("}}}"));
  // add removed symbols
  json += "}}}";
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  bool success = doc["success"]; // true

  JsonObject result = doc["result"];
  float result_Bid = result["Bid"]; // 0.5735
  float result_Ask = result["Ask"]; // 0.5749
  float result_Last = result["Last"]; // 0.574
  float result_High = result["High"]; // 0.5856
  float result_Low = result["Low"]; // 0.5512
  int result_BaseVolume = result["BaseVolume"]; // 0


              
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
 
  Serial.println("reply was:");
  Serial.println("==========");
  while(client.available()){        
    jsonResponse = client.readStringUntil('\n');  //Read Line by Line
    Serial.println(jsonResponse); //Print response
  }
  Serial.println("==========");
  Serial.println("closing connection");



  rawPrice = result_Last;
  sprintf(line1, "%s %.2f USD", symbols[symbolIndex].c_str(), rawPrice);
  Serial.println(rawPrice);
  symbolIndex = (symbolIndex + 1) % 3;    // result is remainder  of division 


  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print("Coin Tracker");
  lcd.setCursor(0,1);
  lcd.print(line1);
    
  delay(500);  //GET Data at every 2 seconds
    
}
