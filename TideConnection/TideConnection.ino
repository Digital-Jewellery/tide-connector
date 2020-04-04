#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "Robert and Nantia";
const char* password = "Thriller123";

ESP8266WebServer server(80);

const int led1 = 12; //green
const int led2 = 15; //red
const int led3 = 13; //blue

const int tideMax = 315;
const int tideMin = -223;

double lastTideValue = 0;

const int highTideMaxColorRed = 0;
const int highTideMaxColorGreen = 0;
const int highTideMaxColorBlue = 255;

const int highTideMinColorRed = 0;
const int highTideMinColorGreen = 255;
const int highTideMinColorBlue = 0;

const int lowTideMaxColorRed = 255;
const int lowTideMaxColorGreen = 0;
const int lowTideMaxColorBlue = 0;

const int lowTideMinColorRed = 0;
const int lowTideMinColorGreen = 255;
const int lowTideMinColorBlue = 0;

int previousTideValue = 0;
int currentTideValue = 315;
int currentRed = 255;
int currentGreen = 0;
int currentBlue = 0;

unsigned long lastUpdate = 0;
int tideDirection = 0;
int updateInterval = 1000 * 60 * 10;
// Every time this is call this device will try to access another to turn it LED on or off
void getTideLevel() {
  if (WiFi.status() == WL_CONNECTED) {

    WiFiClient client;

    HTTPClient http;

    Serial.printf("[HTTP} !!!!!!!!!");

     //The device trying to access another on the link http://192.168.1.112//LED?ledState=1
    if (http.begin(client, "http://environment.data.gov.uk/flood-monitoring/id/measures/E71039-level-tidal_level-Mean-15_min-mAOD/readings?_sorted&_limit=1")) {  // HTTP

        Serial.printf("A");
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
          Serial.printf("B");
          Serial.println();
         // String payload2 = http.getString();
         // Serial.println(payload2);


        const size_t bufferSize = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(8) + 370;
         DynamicJsonBuffer jsonBuffer(bufferSize);
          JsonObject& root = jsonBuffer.parseObject(http.getString());

          if(!root.success()) {
            Serial.println("parseObject() failed");
          } else {
            String currentValueTideString = root["items"][0]["value"];
            double newCurrentValueTide = currentValueTideString.toDouble()*100;
            //applyCurrentTide(currentValueTide);
            previousTideValue = currentTideValue;
            currentTideValue = newCurrentValueTide;
            lastUpdate = millis(); // replacement for the delay function
            Serial.print("currentTideValue: ");
            Serial.print(currentTideValue);
            Serial.println();
          }
    
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
             Serial.printf("C");
      }

      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect\n");
    }

  }
}

void applyCurrentTide(int tideValue) {
  Serial.print("CurrentTide: ");
  Serial.print(tideValue);
  Serial.println();

  int red;
  int green;
  int blue;

  if(tideValue >= 0) {

    //Red
    red = map(tideValue, 0, tideMax, highTideMinColorRed, highTideMaxColorRed);
      Serial.print("map: ");
      Serial.print(tideValue);
      Serial.print(" ");
      Serial.print(" 0, ");
      Serial.print(tideMax);
      Serial.print(" , ");
      Serial.print(highTideMinColorRed);
      Serial.print(" , ");
      Serial.print(highTideMaxColorRed);
      Serial.println();
  
    //Green
    green = map(tideValue, 0, tideMax, highTideMinColorGreen, highTideMaxColorGreen);
      Serial.print("map: ");
      Serial.print(tideValue);
      Serial.print(" 0, ");
      Serial.print(tideMax);
      Serial.print(" , ");
      Serial.print(highTideMinColorGreen);
      Serial.print(" , ");
      Serial.print(highTideMaxColorGreen);
      
      Serial.println();
  
    //Blue
    blue = map(tideValue, 0, tideMax, highTideMinColorBlue, highTideMaxColorBlue);
      Serial.print("map: ");
      Serial.print(tideValue);
      Serial.print(" ");
      Serial.print(" 0, ");
      Serial.print(tideMax);
      Serial.print(" , ");
      Serial.print(highTideMinColorBlue);
      Serial.print(" , ");
      Serial.print(highTideMaxColorBlue);
      Serial.println();

  } else {
    
    //Red
    red = map(tideValue, tideMin, 0, lowTideMaxColorRed, lowTideMinColorRed);
      Serial.print("map: ");
      Serial.print(tideValue);
      Serial.print(" ");
      Serial.print(" 0, ");
      Serial.print(tideMin);
      Serial.print(" , ");
      Serial.print(lowTideMaxColorRed);
      Serial.print(" , ");
      Serial.print(lowTideMinColorRed);
      Serial.println();
  
    //Green
    green = map(tideValue, tideMin, 0, lowTideMaxColorGreen, lowTideMinColorGreen);
      Serial.print("map: ");
      Serial.print(tideValue);
      Serial.print(" 0, ");
      Serial.print(tideMin);
      Serial.print(" , ");
      Serial.print(lowTideMaxColorGreen);
      Serial.print(" , ");
      Serial.print(lowTideMinColorGreen);
      
      Serial.println();
  
    //Blue
    blue = map(tideValue, tideMin, 0, lowTideMaxColorBlue, lowTideMinColorBlue);
      Serial.print("map: ");
      Serial.print(tideValue);
      Serial.print(" 0, ");
      Serial.print(tideMin);
      Serial.print(" , ");
      Serial.print(lowTideMaxColorBlue);
      Serial.print(" , ");
      Serial.print(lowTideMinColorBlue);
      Serial.println();
 
  }
  
  analogWrite(led1, green); // green
  analogWrite(led2, red); //red
  analogWrite(led3, blue); // blue
  
  
  Serial.print("r:");
  Serial.print(red);
  Serial.print(" g:");
  Serial.print(green);
  Serial.print(" b:");
  Serial.print(blue);
  Serial.println();

}

void tideTest()  {

  for(int i = tideMin; i < tideMax; i+=5) {
   applyCurrentTide(i);
   delay(100);
 }

 for(int i = tideMax; i > tideMin; i-=5) {
   applyCurrentTide(i);
   delay(100);
 }
}

void setup() {
  
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);

  randomSeed(analogRead(0));

  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  //If you connect with success to the internet the device will print its IP address with the Serial Monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }


  Serial.println("HTTP server started");
  startTideConnector();
}

void loop() {
  server.handleClient();
  MDNS.update();

  updateTideConnector();
  //Run the online version
  //getTideLevel();

  //Remove coment to run test
  //tideTest(); 
  
 //delay(10* 60 * 1000);
}

void startTideConnector() {
  getTideLevel();
  int colours[3];
  getTideColours(currentTideValue, colours);
  currentRed = colours[0];
  currentGreen = colours[1];
  currentBlue = colours[2];
  writeColourToLED(colours[0],colours[1], colours[2]);
}

void updateTideConnector() {
   if(millis()-lastUpdate >= updateInterval) {
    getTideLevel();
    applyCurrentTide2(previousTideValue, currentTideValue);
  }
}

int getTideValue() {
  if(lastUpdate == 0) {
    currentTideValue =  random(tideMin, tideMax+1);
    tideDirection = random(0,2);
  } else {
    int variance = random(5, 15);
    previousTideValue = currentTideValue;
    if(tideDirection == 0) {
      if(currentTideValue - variance < tideMin) {
        currentTideValue = tideMin;
        tideDirection = 1;
      } else 
        currentTideValue -= variance; 
    } else if(tideDirection == 1) {
      if(currentTideValue + variance > tideMax) {
        currentTideValue = tideMax;
        tideDirection = 0;
      } else 
        currentTideValue += variance; 
    }
  }
  lastUpdate = millis();
  Serial.print("Current Tide Value");
  Serial.println(currentTideValue);
  return currentTideValue;
}

void applyCurrentTide2(int previousTideValue, int currentTideValue) {

  int previousColours[3];
  getTideColours(previousTideValue, previousColours);

  int currentColours[3];
  getTideColours(currentTideValue, currentColours);
  
  if((previousTideValue < 0 && currentTideValue >= 0) || (previousTideValue >= 0 && currentTideValue < 0)) {
    dimTransition(previousColours[0], previousColours[1], previousColours[2], currentColours[0], currentColours[1], currentColours[2]);
  } else {
    transition(previousColours[0], previousColours[1], previousColours[2], currentColours[0], currentColours[1], currentColours[2]);
  }
}

void getTideColours (int tideValue, int *colours) {

  if(tideValue >= 0) {

    //Red
    colours[0] = map(tideValue, 0, tideMax, highTideMinColorRed, highTideMaxColorRed);
  
    //Green
    colours[1] = map(tideValue, 0, tideMax, highTideMinColorGreen, highTideMaxColorGreen);
  
    //Blue
    colours[2] = map(tideValue, 0, tideMax, highTideMinColorBlue, highTideMaxColorBlue);
 

  } else {

    //Red
    colours[0] = map(tideValue, tideMin, 0, lowTideMaxColorRed, lowTideMinColorRed);
  
    //Green
    colours[1] = map(tideValue, tideMin, 0, lowTideMaxColorGreen, lowTideMinColorGreen);
  
    //Blue
    colours[2] = map(tideValue, tideMin, 0, lowTideMaxColorBlue, lowTideMinColorBlue);
 
  }

}

void transition(int cRed, int cGreen, int cBlue, int tRed, int tGreen, int tBlue) {

  int redVariance = tRed - cRed;
  int greenVariance = tGreen - cGreen;
  int blueVariance = tBlue - cBlue;
  int maxVariance = -1;

  if( abs(redVariance) >=  abs(greenVariance) &&  abs(redVariance) >=  abs(blueVariance)) {
    maxVariance = abs(redVariance);
  } else if ( abs(greenVariance) >=  abs(redVariance) &&  abs(greenVariance) >=  abs(blueVariance)) {
    maxVariance = abs(greenVariance);
  } else if ( abs(blueVariance) >=  abs(redVariance) &&  abs(blueVariance) >=  abs(greenVariance)) {
    maxVariance = abs(blueVariance);
  }

  int nRed = cRed;
  int nGreen = cGreen;
  int nBlue = cBlue;

  while(cRed != tRed || cGreen != tGreen || cBlue != tBlue) {

    if(cRed != tRed){ if(redVariance > 0) cRed++; else if(redVariance < 0) cRed--; }
    if(cGreen != tGreen){ if(greenVariance > 0) cGreen++; else if(greenVariance < 0) cGreen--; }
    if(cBlue != tBlue){ if(blueVariance > 0) cBlue++; else if(blueVariance < 0) cBlue--; }

/*
    Serial.print(cRed);
     Serial.print("-");
    Serial.print(tRed);
    Serial.print(" ");
    Serial.print(cGreen);
    Serial.print("-");
    Serial.print(tGreen);
    Serial.print(" ");
    Serial.print(cBlue);
    Serial.print("-");
    Serial.println(tBlue);
    */
    Serial.print(3000/maxVariance);
    Serial.print(" ");

    
    writeColourToLED(cRed, cGreen, cBlue);
    delay(3000/maxVariance);
  }
  Serial.println();
}

void dimTransition(int cRed, int cGreen, int cBlue, int tRed, int tGreen, int tBlue){
  transition(cRed, cGreen, cBlue, 0, 0, 0);
  transition(0, 0, 0, tRed, tGreen, tBlue);
}

void writeColourToLED(int red, int green, int blue) {
  analogWrite(led2, red);
  analogWrite(led1, green);
  analogWrite(led3, blue);
}
