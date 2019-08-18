#include "HX711.h"
#include <SPI.h>
#include <WiFiNINA.h>
#include "arduino_secrets.h"
#include <EasyNTPClient.h>
#include <WiFiUdp.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>

//scales
const int LOADCELL1_DOUT_PIN = 6;
const int LOADCELL1_SCK_PIN = 5;
const int scaleFactor1 = 381410;
const float netto1 = 50.0f;
const float brutto1 = 100.0f;
HX711 scale1;
const int LOADCELL2_DOUT_PIN = 11;
const int LOADCELL2_SCK_PIN = 10;
const int scaleFactor2 = 393860;
const float netto2 = 500.0f;
const float brutto2 = 1000.0f;
HX711 scale2;
int scaleID;
float currentWeight, currentWeight1, currentWeight2;
float currentFill, currentFill1, currentFill2;

//WiFi connection
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

//time
WiFiUDP udp;
EasyNTPClient ntpClient(udp, "pool.ntp.org", (60*60*2));
long hourTimer1 = 0;
long hourTimer2 = 0;

//push
const char* logServer = "api.pushingbox.com";
String deviceId = "v6ED79EFFEB2DD3F";
String message;
bool dailyMessageSent = false;
bool fillMessageSent1 = false;
bool fillMessageSent2 = false;

//OLED
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, A4, A5, U8X8_PIN_NONE);
int digits;

void setup() {

    Serial.begin(9600);
    u8g2.begin();
    u8g2.setFont(u8g2_font_ncenB08_tr);
  
    //scales
    delay(5000);
    scale1.begin(LOADCELL1_DOUT_PIN, LOADCELL1_SCK_PIN);
    scale1.set_scale(scaleFactor1);  
    scale1.tare(); 
    scale2.begin(LOADCELL2_DOUT_PIN, LOADCELL2_SCK_PIN);
    scale2.set_scale(scaleFactor2);  
    scale2.tare();
}

void loop() {
    //connecting
    if (status != WL_CONNECTED)
        connect();

    //scales
    readWeight(1);
    calculateFill(currentWeight, 1);
    currentWeight1 = currentWeight;
    currentFill1 = currentFill;
    readWeight(2);
    calculateFill(currentWeight, 2);
    currentWeight2 = currentWeight;
    currentFill2 = currentFill;

    //printing
    printSerial(currentFill1, currentFill2, currentWeight1, currentWeight2);
    u8g2.clearBuffer();
    printOLED(currentFill1, currentWeight1, 1);
    printOLED(currentFill2, currentWeight2, 2);
    u8g2.sendBuffer();
 
    


    
    
    
    
// todo: preparing message for push in function 
    /*message = "Scale 1: ";
    if(currentFill1 < 1){
            message += String("empty");
        if (hourTimer1 == 0)
            hourTimer1 = ntpClient.getUnixTime();
  
        else{
            message += String(currentFill1);
            message += "%"; 
        } 
    }*/


//daily message at 16:00 - to do: put it in function
    if((ntpClient.getUnixTime()  % 86400L) / 3600 == 18 && dailyMessageSent == false){  
        sendNotification(message);
        printTime();  
        dailyMessageSent = true;
    }

//message: empty, 0-20% - to do: put it in function without repeating
    if(currentFill1 < 1 && fillMessageSent1 == false && ntpClient.getUnixTime() > (hourTimer1 + 3600)){ //empty & ater 1hour
        sendNotification(message);
        printTime();
        fillMessageSent1 = true;
    }
    else if(currentFill1 < 20 && currentFill1 > 1 && fillMessageSent1 == false){ //below 20%
        sendNotification(message);
        fillMessageSent1 = true;
    }
  
    if(currentFill2 < 1 && fillMessageSent2 == false && ntpClient.getUnixTime() > (hourTimer2 + 3600)){ //empty & ater 1hour
        sendNotification(message);
        printTime();
        fillMessageSent2 = true;
    }
    else if(currentFill2 < 20 && currentFill2 > 1 &&  fillMessageSent2 == false){ //below 20%
        sendNotification(message);
        fillMessageSent2 = true;
    }

//reset flags - to do: put it in function
    if((ntpClient.getUnixTime()  % 86400L) / 3600 == 17)
        dailyMessageSent = false;
    if(currentFill1 > 21){
        fillMessageSent1 == false;
        hourTimer1 = 0;
    }
    if(currentFill2 > 21){
        fillMessageSent2 == false;
        hourTimer2 = 0;
    }
}

void connect(){

  u8g2.clearBuffer();
  u8g2.drawStr(0,10,"Connecting...");
  u8g2.sendBuffer();
  byte connectionCounter = 1;
  if (WiFi.status() == WL_NO_MODULE) 
    Serial.print("Communication with WiFi module failed!\n"); 
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.print(ssid);
    Serial.print(" Attempt: ");
    Serial.println(connectionCounter);
    status = WiFi.begin(ssid, pass);
    delay(1000);
    if (connectionCounter == 10){
      u8g2.clearBuffer();
      u8g2.drawStr(0,20,"Unable to connect");
      u8g2.drawStr(0,40,"Check your WiFi");
      u8g2.sendBuffer();
      delay(5000);
      softReset();
    }
    ++connectionCounter;
  }
}

float readWeight(int scaleID){
    float temporaryWeight = 0.0f;
    while(true){
        if(scaleID == 1)
            currentWeight = scale1.get_units(3) * 1000;
        else
            currentWeight = scale2.get_units(3) * 1000;
        int equal = currentWeight - temporaryWeight;
        if(fabs(equal) < 5)
            break;
        temporaryWeight = currentWeight;
    }
    return currentWeight;
}

float calculateFill(float currentWeight, int scaleID){    
    if(scaleID == 1){
        currentFill = ((currentWeight - (brutto1 - netto1)) * 100) / netto1;
        Serial.print("Hello 1: ");
        Serial.println(currentWeight, 0);
    }
    else{
        currentFill = ((currentWeight - (brutto2 - netto2)) * 100) / netto2;
        Serial.print("Hello 2: ");
        Serial.println(currentWeight, 0);
    }
    Serial.print("Test ");
    Serial.print(scaleID);
    Serial.print(": ");
    Serial.println(currentFill, 0);
    return currentFill;
}

void printSerial(float currentFill1, float currentFill2, float currentWeight1, float currentWeight2){  
    Serial.print("Scale 1: ");
    Serial.print(currentFill1, 0);
    Serial.print("%  ");
    Serial.print(currentWeight1, 0);
    Serial.println("g");
    Serial.print("Scale 2: ");
    Serial.print(currentFill2, 0);
    Serial.print("%  ");
    Serial.print(currentWeight2, 0);
    Serial.println("g");
}

void printOLED(float currentFill, float currentWeight, int scaleID){
    char scaleString[5];
    
    u8g2.drawStr(0,scaleID*30-20,"Scale");
    strcpy(scaleString, u8x8_u8toa(scaleID, 1));
    u8g2.drawStr(30,scaleID*30-20,scaleString);
    u8g2.drawStr(36,scaleID*30-20,":");
    
    if(currentFill > 1){
        digitsCounter(currentFill);
        strcpy(scaleString, u8x8_u8toa(currentFill, digits));
        u8g2.drawStr(45,scaleID*30-20,scaleString);
        u8g2.drawStr(80,scaleID*30-20,"%");
        digitsCounter(currentWeight);
        strcpy(scaleString, u8x8_u8toa(currentWeight, digits));
        u8g2.drawStr(45,scaleID*30-10,scaleString);
        u8g2.drawStr(80,scaleID*30-10,"g");
    }
    else
        u8g2.drawStr(45,scaleID*30-20,"empty");
}

float digitsCounter(int number){
    digits = 0;
    while(number >= 1){
        number /= 10;
        ++digits;
    }
    return digits;
}

void sendNotification(String message){
  
  WiFiClient client;
    if (client.connect(logServer, 80)) {
    Serial.println("Client connected\n"); 
    String postStr = "devid=";
    postStr += String(deviceId);
    postStr += "&message_parameter=";
    postStr += String(message);
    postStr += "\r\n\r\n";
    client.print("POST /pushingbox HTTP/1.1\n");
    client.print("Host: api.pushingbox.com\n");
    client.print("Connection: close\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
  }
  client.stop();
}

void printTime(){
  Serial.print("The UTC time is ");
    Serial.print((ntpClient.getUnixTime()  % 86400L) / 3600); 
    Serial.print(':');
    if (((ntpClient.getUnixTime() % 3600) / 60) < 10) {
      
      Serial.print('0');
    }
    Serial.print((ntpClient.getUnixTime()  % 3600) / 60);
    Serial.print(':');
    if ((ntpClient.getUnixTime() % 60) < 10) {
      Serial.print('0');
    }
    Serial.println(ntpClient.getUnixTime() % 60); 
}

void softReset(){
    asm volatile ("  jmp 0");
}
  
