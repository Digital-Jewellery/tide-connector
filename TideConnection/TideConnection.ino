#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>

#include <ArduinoJson.h>
#include <WiFiManager.h>

#include <Adafruit_NeoPixel.h>

#define LED_PIN 12
#define BRIGHTNESS 50

Adafruit_NeoPixel strip(1, LED_PIN, NEO_GRBW + NEO_KHZ800);
WiFiManager wifiManager;

//const int redLED = 14;    //D4
//const int greenLED = 12;  //D6
//const int blueLED = 13;   //D7

//If cathode led invert values
const int maxLEDValue = 255;//0;           
const int minLEDValue = 0;//1023;

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
int currentTideValue = -9999;
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

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  
  //pinMode(led1, OUTPUT);
  //pinMode(led2, OUTPUT);
  //pinMode(led3, OUTPUT);

  randomSeed(analogRead(0));

  writeColourToLED(maxLEDValue, maxLEDValue, minLEDValue);

  if(!wifiManager.autoConnect("Tide Connector")) {
    writeColourToLED(maxLEDValue, minLEDValue, minLEDValue);
    delay(1000);
    Serial.println("failed to connect and hit timeout");
    ESP.reset();
    delay(1000);
  };

  writeColourToLED(255, 0, 0);
	startTideConnector();
}

void loop() {
  
  //tideTest();

  /*
  for(int i = 0; i < 50; i++) {
      Serial.print("brightness: ");
     	Serial.println(i);
  		strip.setPixelColor(0, strip.Color(0, 0, 255));
     	strip.setBrightness(i); 
  	 	strip.show();
    	delay(100);
  }
  
  for(int j = 50; j >= 0; j--) {
    	Serial.print("brightness: ");
     	Serial.println(j);
  		strip.setPixelColor(0, strip.Color(0, 0, 255));
    	strip.setBrightness(j); 
  	 	strip.show();
    	delay(100);
  }
  */
  

	updateTideConnector();
  
  //Run the online version
  //getTideLevel(tideLink);
  
	//updateTideConnector();

  //Remove coment to run test
  //tideTest(); 
  
 //delay(10* 60 * 1000);

  //tideTest2();


}

void startTideConnector() {
  Serial.println("get tide");
  int newTideValue = getTideLevel(tideLink);
    Serial.println("got tide?");
  //If there is a error, it keeps retrying
  while(newTideValue == -9999) {
    Serial.println("no");
    blink(minLEDValue, minLEDValue, minLEDValue, maxLEDValue, maxLEDValue, minLEDValue, 200);
    newTideValue = getTideLevel(tideLink);
    delay(1000);
  }

  Serial.println("yes");
  
  updateTideBoundries(newTideValue);

  int *colours = getTideColours(newTideValue);
  int color1 = colours[0];
  int color2 = colours[1];
  int color3 = colours[2];
  
	writeColourToLED(color1, color2, color3);
  
  setState(newTideValue, color1, color2, color3);
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

  const size_t bufferSize = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(8) + 1370;
  DynamicJsonDocument doc(bufferSize);
  DeserializationError error = deserializeJson(doc,http.getString());

  if(error) {
    Serial.print("deserializeJson() failed with code");
    Serial.println(error.c_str());
    http.end();
    return errorTide;
  }

  String currentValueTideString = doc["items"][0]["value"];
  Serial.print("current tide value: ");
  Serial.println(currentValueTideString);
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
  //analogWrite(redLED, red);
  //analogWrite(greenLED, green);
  //analogWrite(blueLED, blue);
  
  Serial.print("r:");
  Serial.print(red);
  Serial.print(" g:");
  Serial.print(green);
  Serial.print(" b:");
  Serial.print(blue);
  Serial.println();
		
  strip.setPixelColor(0, strip.Color(red, green, blue, 255));
  strip.show();
}

void setState(int newTideValue, int red, int green, int blue) {
  lastUpdate = millis();
  
  if(currentTideValue == -9999) {
    previousTideValue = newTideValue;
  } else {
  	previousTideValue = currentTideValue;
  }
  
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
   int *colours = getTideColours(i);
   applyCurrentTide2(i-5, i);
   delay(2000);
 }

 for(int i = tideMax; i > tideMin; i-=5) {
   int *colours = getTideColours(i);
   applyCurrentTide2(i+5, i);
   delay(2000);
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
   
    int color1 = colours[0];
    int color2 = colours[1];
    int color3 = colours[3];
     
     
    applyCurrentTide2(previousTideValue, newTideValue);

    setState(newTideValue, color1, color2, color3);


    //getTideLevel();
    //applyCurrentTide2(previousTideValue, currentTideValue);
  } /* else {
  	waving();  
  }*/
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
  
  Serial.print("previous colour");
  Serial.print(previousColours[0]);
   Serial.print(" ");
  Serial.print(previousColours[1]);
   Serial.print(" ");
	Serial.println(previousColours[2]);
  
  int p1 = previousColours[0];
  int p2 = previousColours[1];
  int p3 = previousColours[2];
  
  int *currentColours = getTideColours(currentTideValue);
  Serial.print("current colour");
  Serial.print(currentColours[0]);
   Serial.print(" ");
  Serial.print(currentColours[1]);
   Serial.print(" ");
	Serial.println(currentColours[2]);
  
  int c1 = currentColours[0];
  int c2 = currentColours[1];
  int c3 = currentColours[2];
  
  
  if((previousTideValue < 0 && currentTideValue >= 0) || (previousTideValue >= 0 && currentTideValue < 0)) {
    //dimTransition(previousColours[0], previousColours[1], previousColours[2], currentColours[0], currentColours[1], currentColours[2]);
    dimTransition(p1, p2, p3, c1, c2, c3);
  } else {
    //transition(previousColours[0], previousColours[1], previousColours[2], currentColours[0], currentColours[1], currentColours[2]);
    transition(p1, p2, p3, c1, c2, c3);
  }
  
}

void blink(int aRed, int aGreen, int aBlue, int bRed, int bGreen, int bBlue, int delayPeriod) {
  writeColourToLED(aRed, aGreen, aBlue);
  delay(delayPeriod);
  writeColourToLED(bRed, bGreen, bBlue);
  delay(delayPeriod);
}

void transition(int cRed, int cGreen, int cBlue, int tRed, int tGreen, int tBlue) {
  
  Serial.print(cRed);
  Serial.print(cGreen);
  Serial.print(cBlue);
  Serial.println(" ");
  Serial.print(tRed);
  Serial.print(tGreen);
  Serial.print(tBlue);
  Serial.println(" ");

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

      Serial.print("test 2");
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
  Serial.print("test 1");
  Serial.println();
}

void dimTransition(int cRed, int cGreen, int cBlue, int tRed, int tGreen, int tBlue){
  Serial.println("dimTransition");
  transition(cRed, cGreen, cBlue, 0, 0, 0);
  transition(0, 0, 0, tRed, tGreen, tBlue);
}


void waving() {

  
  /*
  Serial.println("going down");
  for(int i = 50; i > 0; i--) {
    Serial.println(i);
 		strip.setBrightness(i);
    strip.show();      
    delay(100);
  }
  
  Serial.println("going up");
  for(int j = 1; j < 50; j++) {
    Serial.println(j);
  	strip.setBrightness(j);
    strip.show();      
    delay(100);
  }*/
  
    /*
  Serial.println("going down");
  for(int i = 50; i > 0; i--) {
    int t1 = bellCurve(i, 3);
    Serial.println(t1);
 		strip.setBrightness(t1 * 50);
    strip.show();      
    delay(100);
  }
  
  Serial.println("going up");
  for(int j = 1; j < 50; j++) {
    int t2 = bellCurve(j, 3);
    Serial.println(t2);
  	strip.setBrightness(t2 * 50);
    strip.show();      
    delay(100);
  }*/
  

      
  Serial.println("going down");
  for(float i = 0.5; i > 0; i-=0.01) {
    Serial.println(i);
    float t1 = arch(i);
    float t12 = map(t1 * 100, 0, 100, 1, 50);
    Serial.println(t1);
    Serial.println(t12);
 		strip.setBrightness(t12);
    strip.show();      
    delay(100);
  }
  /*
  Serial.println("going up");
  for(int j = 1; j < 50; j++) {
    int t2 = bellCurve(j, 3);
    Serial.println(t2);
  	strip.setBrightness(t2 * 50);
    strip.show();      
    delay(100);
  }
  */

}

float smoothStart(int t, int n) {
  return pow(t , n);
}

float smoothStop(int t, int n) {
  
  float temp = 1 - t;
	return 1 - pow(temp, n);
}

float bellCurve(int t, int n) {
	return smoothStart(t, n) * smoothStop(t, n);
}

float arch(int t) {
	return t * (1 - t); 
}



