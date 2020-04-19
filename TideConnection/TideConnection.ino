#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>

#include <ArduinoJson.h>
#include <WiFiManager.h>

WiFiManager wifiManager;

const int redLED = 14;    //D4
const int greenLED = 12;  //D6
const int blueLED = 13;   //D7

//If cathode led invert values
const int maxLEDValue = 0;           
const int minLEDValue = 1023;

const int led1 = 12; //green
const int led2 = 14; //red
const int led3 = 13; //blue

int tideMax = 315;
int tideMin = -223;

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

const int highTideColour[3] = {minLEDValue, minLEDValue, maxLEDValue};
const int zeroTideColour[3] = {minLEDValue, maxLEDValue, minLEDValue};
const int lowTideColour[3] = {maxLEDValue, maxLEDValue, minLEDValue};

int previousTideValue = 0;
int currentTideValue = 315;
int currentRed = 255;
int currentGreen = 0;
int currentBlue = 0;

unsigned long lastUpdate = 0;
int tideDirection = 0;
int updateInterval = 1000 * 60 * 10;

char *tideLink = "http://environment.data.gov.uk/flood-monitoring/id/measures/E71039-level-tidal_level-Mean-15_min-mAOD/readings?_sorted&_limit=1";
const int errorTide = -9999;

void setup() {

  Serial.begin(9600);

  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);

  randomSeed(analogRead(0));

  writeColourToLED(maxLEDValue, maxLEDValue, minLEDValue);

  if(!wifiManager.autoConnect("Tide Connector")) {
    writeColourToLED(maxLEDValue, minLEDValue, minLEDValue);
    delay(1000);
    Serial.println("failed to connect and hit timeout");
    ESP.reset();
    delay(1000);
  };

  writeColourToLED(minLEDValue, minLEDValue, minLEDValue);

  startTideConnector();
}

void loop() {

  /*
  server.handleClient();
  MDNS.update();

  updateTideConnector();
  */
  //Run the online version
  //getTideLevel();

  //Remove coment to run test
  //tideTest(); 
  
 //delay(10* 60 * 1000);

  tideTest2();


}

void startTideConnector() {
  int newTideValue = getTideLevel(tideLink);
  //If there is a error, it keeps retrying
  while(newTideValue == -9999) {
    blink(minLEDValue, minLEDValue, minLEDValue, maxLEDValue, maxLEDValue, minLEDValue, 200);
    newTideValue = getTideLevel(tideLink);
    delay(1000);
  }

  updateTideBoundries(newTideValue);

  int *colours = getTideColours(newTideValue);
  writeColourToLED(colours[0], colours[1], colours[2]);

  setState(newTideValue, colours[0], colours[1], colours[2]);
}

/**
 * Get the tide level from a given link
 * */
int getTideLevel(char * link) {

  if(!WiFi.status() == WL_CONNECTED) {
    Serial.println("Not Connect to the internet");
    return errorTide;
  }

  WiFiClient client;
  HTTPClient http;

  if(!http.begin(client, link)) {
    Serial.printf("[HTTP] Unable to connect\n");
    return errorTide;
  }

  // start connection and send HTTP header
  int httpCode = http.GET();

  // httpCode will be negative on error
  if(httpCode <= 0) {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    http.end();
    return errorTide;
  }

  const size_t bufferSize = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(8) + 370;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root = jsonBuffer.parseObject(http.getString());

  if(!root.success()) {
    Serial.println("parseObject() failed");
    http.end();
    return errorTide;
  }

  String currentValueTideString = root["items"][0]["value"];
  double newCurrentValueTide = currentValueTideString.toDouble()*100;
  http.end();
  return newCurrentValueTide;
}

int * getTideColours (int tideValue) {

  static int colours[3];

  if(tideValue >= 0) {

    //Red
    colours[0] = map(tideValue, 0, tideMax, zeroTideColour[0], highTideColour[0]);
  
    //Green
    colours[1] = map(tideValue, 0, tideMax, zeroTideColour[1], highTideColour[1]);
  
    //Blue
    colours[2] = map(tideValue, 0, tideMax, zeroTideColour[2], highTideColour[2]);
 

  } else {

    //Red
    colours[0] = map(tideValue, tideMin, 0, lowTideColour[0], zeroTideColour[0]);
  
    //Green
    colours[1] = map(tideValue, tideMin, 0, lowTideColour[1], zeroTideColour[1]);
  
    //Blue
    colours[2] = map(tideValue, tideMin, 0, lowTideColour[2], zeroTideColour[2]);
 
  }

  return colours;
}

void writeColourToLED(int red, int green, int blue) {
  analogWrite(redLED, red);
  analogWrite(greenLED, green);
  analogWrite(blueLED, blue);
}

void setState(int newTideValue, int red, int green, int blue) {
  lastUpdate = millis();
  previousTideValue = currentTideValue;
  currentTideValue = newTideValue;
  currentRed = red;
  currentGreen = green;
  currentBlue = blue;
}

void updateTideBoundries(int tideValue) {

  if(tideValue == errorTide)
    return;

  if(tideValue < tideMin)
    tideMin = tideValue;
  else if(tideValue > tideMax)
    tideMax = tideValue;

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

void tideTest2() {

  for(int i = tideMin; i < tideMax; i++) {
    Serial.println(i);
    int *colours = getTideColours(i);
    writeColourToLED(colours[0], colours[1], colours[2]);
    delay(10);
  }

  for(int i = tideMax; i >= tideMin; i--) {
    Serial.println(i);
    int *colours = getTideColours(i);
    writeColourToLED(colours[0], colours[1], colours[2]);
    delay(10);
  }

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

void updateTideConnector() {
   if(millis()-lastUpdate >= updateInterval) {
    int newTideValue = getTideLevel(tideLink);
    while(newTideValue == -9999) {
      newTideValue = getTideLevel(tideLink);
      delay(1000);
    }

    updateTideBoundries(newTideValue);

    int *colours = getTideColours(newTideValue);
   
    applyCurrentTide2(previousTideValue, newTideValue);

    setState(newTideValue, colours[0], colours[1], colours[2]);


    //getTideLevel();
    //applyCurrentTide2(previousTideValue, currentTideValue);
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

  int *previousColours = getTideColours(previousTideValue);

  int *currentColours = getTideColours(currentTideValue);
  
  if((previousTideValue < 0 && currentTideValue >= 0) || (previousTideValue >= 0 && currentTideValue < 0)) {
    dimTransition(previousColours[0], previousColours[1], previousColours[2], currentColours[0], currentColours[1], currentColours[2]);
  } else {
    transition(previousColours[0], previousColours[1], previousColours[2], currentColours[0], currentColours[1], currentColours[2]);
  }
}

void blink(int aRed, int aGreen, int aBlue, int bRed, int bGreen, int bBlue, int delayPeriod) {
  writeColourToLED(aRed, aGreen, aBlue);
  delay(delayPeriod);
  writeColourToLED(bRed, bGreen, bBlue);
  delay(delayPeriod);
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


