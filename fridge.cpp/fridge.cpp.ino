//Libraries
#include "HX711.h"
#include <SPI.h>
#include <WiFiNINA.h>
#include "arduino_secrets.h"
#include <EasyNTPClient.h>
#include <WiFiUdp.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <Vector.h>

//Scales
const int numberOfScales = 2;
#define LOADCELL0_DOUT_PIN 6
#define LOADCELL0_SCK_PIN 5
#define SCALE_FACTOR0 381410
HX711 scale0;
#define LOADCELL1_DOUT_PIN 11
#define LOADCELL1_SCK_PIN 10
#define SCALE_FACTOR1 393860
HX711 scale1;
const float netto[numberOfScales] = {50.0f, 500.0f};
const float brutto[numberOfScales] = {100.0f, 1000.0f};
#define MINIMUM_FILL 1
#define MAXIMUM_FILL 20

//WiFi connection
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

//Time
#define POLAND_LOCAL_TIME 7200
#define SECONDS_IN_ONE_DAY 86400
#define SECONDS_IN_ONE_HOUR 3600
#define SECONDS_IN_ONE_MINUTE 60
#define TIME_OF_DAILY_MESSAGE 16
WiFiUDP udp;
EasyNTPClient ntpClient(udp, "pool.ntp.org", POLAND_LOCAL_TIME);


//Push
#define CONNECTION_PORT 80
const char* logServer = "api.pushingbox.com";
String deviceId = "v6ED79EFFEB2DD3F";

//Oled
#define OLED_SCK_PIN A4
#define OLED_SDA_PIN A5
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, OLED_SCK_PIN, OLED_SDA_PIN, U8X8_PIN_NONE);

//SerialPort
#define SERIAL_PORT_BAUD 9600

//Flags
bool fillPushMessageSentFlags[numberOfScales];
bool oneHourPushMessageSentFlags[numberOfScales];
bool dailyPushMessageSent = false;
bool checkIfSendPushMessage = false;
long long oneHourTimers[numberOfScales];

void setup() {
    Serial.begin(SERIAL_PORT_BAUD);
    u8g2.begin();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.clearBuffer();
    connect(true);
    scales0.begin(LOADCELL0_DOUT_PIN, LOADCELL0_SCK_PIN);
    scales0.set_scale(SCALE_FACTOR0);  
    scales0.tare();    
    scale1.begin(LOADCELL1_DOUT_PIN, LOADCELL1_SCK_PIN);
    scale1.set_scale(SCALE_FACTOR1);  
    scale1.tare();
    resetFlagsAndTimers();
    
}


void loop() {
    u8g2.clearBuffer();
    
    if(status != WL_CONNECTED){
        u8g2.drawStr(105, 25,"NO");
        u8g2.drawStr(100, 35,"WiFi");
        connect(false);
    }

    printTime();

    String pushMessage = "";
    ::checkIfSendPushMessage = false;
    
    
    for(int scaleID = 0; scaleID < numberOfScales; ++scaleID){
        float currentWeight, currentFill;
        currentWeight = readWeight(currentWeight, scaleID);
        currentFill = calculateFill(currentWeight, currentFill, scaleID);
        printSerialPortMessage(currentFill, currentWeight, scaleID);
        printOledMessage(currentFill, currentWeight, scaleID);
        pushMessage = preparePushMessage(pushMessage, currentFill, scaleID);
        startOneHourTimerIfEmpty(currentFill,  scaleID);
        checkIfSendEmptyPushMessage(currentFill, scaleID);
        checkIfSendFillPushMessage(currentFill, scaleID);
    }
    checkIfSendDailyPushMessage();
    u8g2.sendBuffer();
    if(checkIfSendPushMessage == true)
        sendPushMessage(pushMessage);
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

void resetFlagsAndTimers(){
      for(int scaleID = 0; scaleID < numberOfScales; ++scaleID){
        ::fillPushMessageSentFlags[scaleID] = false;
        ::oneHourPushMessageSentFlags[scaleID] = false;
        ::dailyPushMessageSent = false;
        ::oneHourTimers[scaleID] = 0;
      }
}

float readWeight(float currentWeight, int scaleID){
    float temporaryWeight = 0.0f;
    while(true){
        if(scaleID == 0)
            currentWeight = scale0.get_units(5) * 1000;
        else
            currentWeight = scale1.get_units(5) * 1000;
        int equal = currentWeight - temporaryWeight;
        if(fabs(equal) < 2)
            break;
        temporaryWeight = currentWeight;
    }
    return currentWeight;
}

float calculateFill(float currentWeight, float currentFill, int scaleID){ 
    currentFill = ((currentWeight - (brutto[scaleID] - netto[scaleID])) * 100) / netto[scaleID];
    return currentFill;
}

void printSerialPortMessage(float currentFill, float currentWeight, int scaleID){  
    Serial.print("Scale ");
    Serial.print(scaleID);
    Serial.print(": ");
    Serial.print(currentFill, 0);
    Serial.print("%  ");
    Serial.print(currentWeight, 0);
    Serial.println("g");
}

void printOledMessage(float currentFill, float currentWeight, int scaleID){
    u8g2.drawStr(0,scaleID*30+10,"Scale");
    u8g2.setCursor(30,scaleID*30+10);
    u8g2.print(scaleID, 1);
    u8g2.drawStr(36,scaleID*30+10,":");
    if(currentFill > MINIMUM_FILL){
        u8g2.setCursor(45,scaleID*30+10);
        u8g2.print(currentFill, 0);
        u8g2.drawStr(80,scaleID*30+10,"%");
    }
    else
        u8g2.drawStr(45,scaleID*30+10,"empty");
    u8g2.setCursor(45,scaleID*30+20);
    u8g2.print(currentWeight, 0);
    u8g2.drawStr(80,scaleID*30+20,"g");
}

String preparePushMessage(String pushMessage, float currentFill, int scaleID){
    pushMessage += "Scale ";
    pushMessage += String(scaleID);
    pushMessage += ": ";
    if(currentFill < MINIMUM_FILL)
        pushMessage += String("empty");
    else{
        pushMessage += String(currentFill);
        pushMessage += "%"; 
    } 
    pushMessage += "\n";
    return pushMessage;
}

void checkIfSendDailyPushMessage(){
    if((ntpClient.getUnixTime()  % SECONDS_IN_ONE_DAY) / SECONDS_IN_ONE_HOUR == 7)
        ::dailyPushMessageSent = false;
    if((ntpClient.getUnixTime()  % SECONDS_IN_ONE_DAY) / SECONDS_IN_ONE_HOUR == TIME_OF_DAILY_MESSAGE && dailyPushMessageSent == false){
        ::dailyPushMessageSent = true;
        ::checkIfSendPushMessage = true;
    }
}  

void startOneHourTimerIfEmpty(float currentFill, int scaleID){
    if(currentFill < MINIMUM_FILL &&  oneHourTimers[scaleID] == 0){
        ::oneHourTimers[scaleID] = ntpClient.getUnixTime() + SECONDS_IN_ONE_HOUR;
        ::fillPushMessageSentFlags[scaleID] = false;
    }
}

void checkIfSendEmptyPushMessage(float currentFill, int scaleID){
    if(currentFill < MINIMUM_FILL && oneHourPushMessageSentFlags[scaleID] == false && oneHourTimers[scaleID] < ntpClient.getUnixTime()){
        ::oneHourPushMessageSentFlags[scaleID] = true;
        ::checkIfSendPushMessage = true;
    }
}   
 
bool checkIfSendFillPushMessage(float currentFill, int scaleID){
    if(currentFill > MINIMUM_FILL && currentFill < MAXIMUM_FILL && fillPushMessageSentFlags[scaleID] == false){
        ::fillPushMessageSentFlags[scaleID] = true;
        ::oneHourPushMessageSentFlags[scaleID] = false;
        ::checkIfSendPushMessage = true;
    }
} 

void sendPushMessage(String pushMessage){
    WiFiClient client;
    Serial.println("pushMessage:");
    Serial.println(pushMessage);
    if (client.connect(logServer, CONNECTION_PORT)) {
    Serial.println("Client connected"); 
    Serial.println(pushMessage);
    String postStr = "devid=";
    postStr += String(deviceId);
    postStr += "&message_parameter=";
    postStr += String(pushMessage);
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
    Serial.print((ntpClient.getUnixTime()  % SECONDS_IN_ONE_DAY) / SECONDS_IN_ONE_HOUR); 
    Serial.print(':');
    if (((ntpClient.getUnixTime() % SECONDS_IN_ONE_HOUR) / SECONDS_IN_ONE_MINUTE) < 10) {
      
      Serial.print('0');
    }
    Serial.print((ntpClient.getUnixTime()  % SECONDS_IN_ONE_HOUR) / SECONDS_IN_ONE_MINUTE);
    Serial.print(':');
    if ((ntpClient.getUnixTime() % SECONDS_IN_ONE_MINUTE) < 10) {
      Serial.print('0');
    }
    Serial.println(ntpClient.getUnixTime() % SECONDS_IN_ONE_MINUTE); 
}

void softReset(){
    asm volatile ("  jmp 0");
}
  
