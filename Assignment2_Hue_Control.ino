#include <Wire.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> 
#include "arduino_secrets.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET 4
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int oldPotValue;
int potValueToSend;
boolean buttonState = false;
boolean leftButtonState = false;
boolean rightButtonState = false;
int prevButtonState = 0;
const int buttonSwitch = 2;
const int leftButton = 11;
const int rightButton = 10;
String buttonString;
int scrollIndex = 0;

float xValue;
float yValue;

String options[] = {
  "bri",
  "hue",
  "sat",
  "ct"
};

int ranges[][2]= {
  {0, 254},
  {0,60000}, 
  {0,100}, 
  {153,500}
};


int status = WL_IDLE_STATUS; // wifi radio status
char hueHubIP[] = "172.22.151.183"; //////////////// change
String hueUser = "LVgjfZjFxs5BnBB0pIhZHeG3kdXjWN2xXtH3VvXd"; ///////////change

//making wifi instance
WiFiClient wifi;
HttpClient httpClient = HttpClient(wifi, hueHubIP);

//credentials
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

void setup() {
  Serial.begin(9600);

  while(!display.begin(SSD1306_SWITCHCAPVCC, 0X3C)){ //0x3C is address for 128x32
    Serial.println("SSD1306 setup...");
    delay(100);
  }

  while( status != WL_CONNECTED){
    Serial.print("attempting to connect to WPA SSID: ");
    //displayWrite("connecting:" + String(ssid));
    Serial.println(ssid);
    status = WiFi.begin(ssid,pass);
    delay(2000);
  }

  Serial.print("you are now connected to the network. IP: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);

  pinMode(buttonSwitch, INPUT_PULLUP);
  pinMode(leftButton, INPUT_PULLUP);
  pinMode(rightButton, INPUT_PULLUP);
}

void loop() {
  boolean buttonPushed = buttonRead(buttonSwitch);
  boolean leftButtonPushed = buttonRead(leftButton);
  boolean rightButtonPushed = buttonRead(rightButton);
  int potValue = map(analogRead(A0),0,1023,ranges[scrollIndex][0],ranges[scrollIndex][1]);
  int tolerance = (ranges[scrollIndex][1] - ranges[scrollIndex][0])*0.05;
  
  if ( abs(potValue - oldPotValue) > tolerance ){
    potValueToSend = potValue;
    oldPotValue = potValue; 
    Serial.println(potValue);
  }
  //delay(500);
  
  // on - off
  if (buttonPushed){
    buttonState = !buttonState;
  }

  if (buttonState == true){
    buttonString = "true";
  }
  else if(buttonState == false){
    buttonString = "false";
  }
  
  // scrolling through options
  if (rightButtonPushed){
    rightButtonState =!rightButtonState;
  }
  if (leftButtonPushed){
    leftButtonState =!leftButtonState;
  }

  if (leftButtonPushed){
    if (scrollIndex == 0){
      scrollIndex = 3;
    }
    else{
      scrollIndex -=1;
    }
  }

  if (rightButtonPushed){
    if (scrollIndex ==3){
      scrollIndex = 0; 
    }
    else{
      scrollIndex +=1;
    }
  }

  //display
  String reading = options[scrollIndex];
  //reading += String(scrollIndex);
  reading += "\n";
  reading += String(potValueToSend);
  reading += "\n";
  reading += "switch:";
  reading += "\n";
  reading += String(buttonString);
  

  displayWrite(reading);
  
  //sending request to hub
//  Serial.print("buttonPushed:");
//  Serial.println(buttonPushed);
//  Serial.print("buttonState:");
  //Serial.println(buttonState);
  
  sendRequest(6, String(buttonString) , String(options[scrollIndex]), String(potValueToSend));
  delay(300);
}

boolean buttonRead(int thisButton) {
  boolean result = false;
  int currentState = digitalRead(thisButton);
  if (currentState != prevButtonState && currentState == LOW) {
    result = true;
    }
  delay(30);
  prevButtonState = currentState;
  return result;
}

void displayWrite(String message) {
  display.clearDisplay();
  display.setTextSize(1.5);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(message);
  display.display();
}

void sendRequest(int lightNum, String onOffState, String command, String stateVal){
  
  String request = "/api/" + hueUser + "/lights/" + lightNum + "/state/";

  String contentType = "application/json";
  
  String bulbCmd = "{\"on\":" + onOffState + ",\""  + command + "\":" + stateVal + "}";

  Serial.println(bulbCmd);

  httpClient.put(request, contentType, bulbCmd);

  int statusCode = httpClient.responseStatusCode();
  String response = httpClient.responseBody();

  Serial.print("Status code from server: ");
  Serial.println(statusCode);
  Serial.print("Server response: ");
  Serial.println(response);
  Serial.println();
}
