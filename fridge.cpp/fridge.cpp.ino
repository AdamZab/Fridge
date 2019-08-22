#include "HX711.h"
#include <SPI.h>
#include <WiFiNINA.h>
#include "arduino_secrets.h"
#include <EasyNTPClient.h>
#include <WiFiUdp.h>
#include <Arduino.h>
#include <U8g2lib.h>


//scales
const int LOADCELL1_DOUT_PIN = 6;
const int LOADCELL1_SCK_PIN = 5;
const long scaleFactor1 = 381410;
const float netto1 = 50.0f;
const float brutto1 = 100.0f;
HX711 scale1;
const int LOADCELL2_DOUT_PIN = 11;
const int LOADCELL2_SCK_PIN = 10;
const long scaleFactor2 = 393860;
const float netto2 = 500.0f;
const float brutto2 = 1000.0f;
HX711 scale2;
int scaleID;
float currentWeight, currentFill, currentFill1, currentFill2;

//WiFi connection
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

//time
WiFiUDP udp;
EasyNTPClient ntpClient(udp, "pool.ntp.org", (60*60*2));

//push
const char* logServer = "api.pushingbox.com";
String deviceId = "v6ED79EFFEB2DD3F";
String message;

//flags
bool oneHourMessageSent1= false , oneHourMessageSent2 = false;
bool fillMessageSent1 = false, fillMessageSent2 = false;
long long oneHourTimer1 = 0, oneHourTimer2 = 0;
bool dailyMessageSent = false;


//OLED
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, A4, A5, U8X8_PIN_NONE);

void setup() {
    Serial.begin(9600);
    u8g2.begin();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.clearBuffer();
    connect(true);
  
    //scales
    scale1.begin(LOADCELL1_DOUT_PIN, LOADCELL1_SCK_PIN);
    scale1.set_scale(scaleFactor1);  
    scale1.tare();    
    scale2.begin(LOADCELL2_DOUT_PIN, LOADCELL2_SCK_PIN);
    scale2.set_scale(scaleFactor2);  
    scale2.tare();
}




void loop() {
    u8g2.clearBuffer();
    
    if (status != WL_CONNECTED){
        u8g2.drawStr(105, 25,"NO");
        u8g2.drawStr(100, 35,"WiFi");
        connect(false);
    }

    message = "";
    bool sendMessage = false;
    
    for (scaleID = 1; scaleID <= 2; ++scaleID){
        readWeight(scaleID);
        calculateFill(currentWeight, scaleID);
        printSerial(currentFill, currentWeight, scaleID);
        printOLED(currentFill, currentWeight, scaleID);
        prepareMessage(currentFill, scaleID);        
        if(scaleID == 1)
            currentFill1 = currentFill;
        else
            currentFill2 = currentFill;
    }
    u8g2.sendBuffer();
   
    if((ntpClient.getUnixTime()  % 86400L) / 3600 == 16 && dailyMessageSent == false){  
        sendMessage = true; 
        dailyMessageSent = true;
    }
    
    if(currentFill1 < 1 &&  oneHourTimer1 == 0){
        oneHourTimer1 = ntpClient.getUnixTime() + (60 * 1);
        fillMessageSent1 = false;
    }
    
    if(currentFill1 < 1 && oneHourMessageSent1 == false && oneHourTimer1 < ntpClient.getUnixTime()){
        sendMessage = true;
        oneHourMessageSent1 = true;
    }
    else if(currentFill1 > 1 && currentFill1 < 20 && fillMessageSent1 == false){
        sendMessage = true;
        fillMessageSent1 = true;
        oneHourMessageSent1 = false;
    }
    
    if(currentFill2 < 1 &&  oneHourTimer2 == 0){
        oneHourTimer2 = ntpClient.getUnixTime() + (60 * 3);
        fillMessageSent2 = false;
    }
    
    if(currentFill2 < 1 && oneHourMessageSent2 == false && oneHourTimer2 < ntpClient.getUnixTime()){
        sendMessage = true;
        oneHourMessageSent2 = true;
    }
    else if(currentFill2 > 1 && currentFill2 < 20 && fillMessageSent2 == false){
        sendMessage = true;
        fillMessageSent2 = true;
        oneHourMessageSent2 = false;
    }

    if(sendMessage == true)
        sendNotification(message);
}

void connect(bool ifSetup){
  if (ifSetup == true){
      u8g2.drawStr(0,10,"Connecting...");
      u8g2.sendBuffer();
  }
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
    if (ifSetup == false)
        break;
    else if (connectionCounter == 10){
        u8g2.clearBuffer();
        u8g2.drawStr(0,10,"Connection failed");
        u8g2.drawStr(0,20,"Restarting");
        u8g2.sendBuffer();
        delay(5000);
        softReset();
      }
    ++connectionCounter;
  }
}

float readWeight(int scaleID){
    float temporaryWeight = 0.0f;
    currentWeight = 0.0f;
    while(true){
        if(scaleID == 1)
            currentWeight = scale1.get_units(5) * 1000;
        else
            currentWeight = scale2.get_units(5) * 1000;
        int equal = currentWeight - temporaryWeight;
        if(fabs(equal) < 2)
            break;
        temporaryWeight = currentWeight;
    }
    return currentWeight;
}

float calculateFill(float currentWeight, int scaleID){ 
    currentFill = 0.0f;   
    if(scaleID == 1){
        float currentFill = ((currentWeight - (brutto1 - netto1)) * 100) / netto1;
        return currentFill;
    }
    else{
        currentFill = ((currentWeight - (brutto2 - netto2)) * 100) / netto2;
    }
    return currentFill;
}

void printSerial(float currentFill, float currentWeight, int scaleID){  
    Serial.print("Scale ");
    Serial.print(scaleID);
    Serial.print(": ");
    Serial.print(currentFill, 0);
    Serial.print("%  ");
    Serial.print(currentWeight, 0);
    Serial.println("g");
}

void printOLED(float currentFill, float currentWeight, int scaleID){
    u8g2.drawStr(0,scaleID*30-20,"Scale");
    u8g2.setCursor(30,scaleID*30-20);
    u8g2.print(scaleID, 1);
    u8g2.drawStr(36,scaleID*30-20,":");
    if(currentFill > 1){
        u8g2.setCursor(45,scaleID*30-20);
        u8g2.print(currentFill, 0);
        u8g2.drawStr(80,scaleID*30-20,"%");
    }
    else
        u8g2.drawStr(45,scaleID*30-20,"empty");
    u8g2.setCursor(45,scaleID*30-10);
    u8g2.print(currentWeight, 0);
    u8g2.drawStr(80,scaleID*30-10,"g");
}

String prepareMessage(float currentFill, int scaleID){
    message += "Scale ";
    message += String(scaleID);
    message += ": ";
    if(currentFill < 1)
        message += String("empty");
    else{
        message += String(currentFill);
        message += "%"; 
    } 
    message += "\n";
    return message;
}

void sendNotification(String message){
    WiFiClient client;
    if (client.connect(logServer, 80)) {
    Serial.println("Client connected"); 
    Serial.println(message);
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
  
