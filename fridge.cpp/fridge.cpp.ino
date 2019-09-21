//Libraries
#include "HX711.h"
#include <WiFiNINA.h>
#include "arduino_secrets.h"
#include <EasyNTPClient.h>
#include <WiFiUdp.h>
#include <Arduino.h>
#include <U8g2lib.h>

//Scales
const int NUMBER_OF_SCALES = 5;
const char LOADCELL_SDA_PIN[NUMBER_OF_SCALES] = {3, 5, 7, 9, 11};
const char LOADCELL_SCL_PIN[NUMBER_OF_SCALES] = {2, 4, 6, 8, 12};
const long SCALE_FACTOR[NUMBER_OF_SCALES] = {381410, 393860, 0, 0 ,0};
const float NETTO[NUMBER_OF_SCALES] = {50.0f, 500.0f, 0.0f, 0.0f, 0.0f};
const float BRUTTO[NUMBER_OF_SCALES] = {100.0f, 1000.0f, 0.0f, 0.0f, 0.0f};
#define MINIMUM_FILL 1
#define MAXIMUM_FILL 20
#define NUMBER_OF_SCALE_READINGS 5
const String SCALE_NAME[NUMBER_OF_SCALES] = {"Mleko", "Keczup", "Test 3", "Test 4", "Test 5"};
HX711 scale[NUMBER_OF_SCALES];

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
#define OLED_SCL_PIN A4
#define OLED_SDA_PIN A5
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, OLED_SCL_PIN, OLED_SDA_PIN, U8X8_PIN_NONE);

//SerialPort
#define SERIAL_PORT_BAUD 9600

//Flags
bool fillPushMessageSentFlags[NUMBER_OF_SCALES];
bool oneHourPushMessageSentFlags[NUMBER_OF_SCALES];
bool dailyPushMessageSent = false;
bool checkIfSendPushMessage = false;
long long oneHourTimers[NUMBER_OF_SCALES];

void setup() {
    Serial.begin(SERIAL_PORT_BAUD);
    u8g2.begin();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.clearBuffer();
    connectWithReset();
    for(int scaleID = 0; scaleID < NUMBER_OF_SCALES; ++scaleID){
        scale[scaleID].begin(LOADCELL_SDA_PIN[scaleID], LOADCELL_SCL_PIN[scaleID]);
        scale[scaleID].set_scale(SCALE_FACTOR[scaleID]);  
        scale[scaleID].tare();
        resetFlagsAndTimers(scaleID);    
    }  
}

void loop() {
    String pushMessage = "";
    ::checkIfSendPushMessage = false; 
    for(int scaleID = 0; scaleID < NUMBER_OF_SCALES; ++scaleID){
        nextOledPageForEveryTwoScales(scaleID);
        float currentWeight, currentFill;
        readWeight(currentWeight, scaleID);
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
    checkConnection();
    if(checkIfSendPushMessage == true)
        sendPushMessage(pushMessage);
}

void connectWithReset(){
    int connectionAttempt = 1;
    int firstLineXAxis = 0;
    int firstLineYAxis = 10;
    int secondLineXAxis = 0;
    int secondLineYAxis = 20;
    u8g2.clearBuffer();
    u8g2.drawStr(firstLineXAxis,firstLineYAxis,"Connecting...");
    u8g2.sendBuffer();
    if (WiFi.status() == WL_NO_MODULE) 
        Serial.println("Communication with WiFi module failed!\n");
    while (status != WL_CONNECTED) {
        if (connectionAttempt == 11){
            u8g2.clearBuffer();
            u8g2.drawStr(firstLineXAxis,firstLineYAxis,"Connection failed");
            u8g2.drawStr(secondLineXAxis,secondLineYAxis,"Restarting...");
            u8g2.sendBuffer();
            delay(5000);
            softReset();
        }
        connect(connectionAttempt);
        ++connectionAttempt;
    }
}
 
void connect(int connectionAttempt){
    String attemptMessage = "Attempting to connect to WPA SSID: ";
    attemptMessage += ssid;
    attemptMessage += " Attempt: ";
    attemptMessage += connectionAttempt;
    Serial.println(attemptMessage);
    status = WiFi.begin(ssid, pass);
    delay(1000);
}

void resetFlagsAndTimers(int scaleID){
    ::fillPushMessageSentFlags[scaleID] = false;
    ::oneHourPushMessageSentFlags[scaleID] = false;
    ::dailyPushMessageSent = false;
    ::oneHourTimers[scaleID] = 0;
}

void nextOledPageForEveryTwoScales(int scaleID){
    if(scaleID % 2 == 0){
        delay(3000);
        u8g2.clearBuffer();
    }
}

float readWeight(float currentWeight, int scaleID){
    float temporaryWeight = 0.0f;
    while(true){
        currentWeight = scale[scaleID].get_units(NUMBER_OF_SCALE_READINGS) * 1000;
        int equal = currentWeight - temporaryWeight;
        if(fabs(equal) < 5)
            break;
        temporaryWeight = currentWeight;
    }
    return currentWeight;
}

float calculateFill(float currentWeight, float currentFill, int scaleID){ 
    currentFill = ((currentWeight - (BRUTTO[scaleID] - NETTO[scaleID])) * 100) / NETTO[scaleID];
    return currentFill;
}

void printSerialPortMessage(float currentFill, float currentWeight, int scaleID){
    String serialPortMessage = String(SCALE_NAME[scaleID]);
    serialPortMessage += ": ";
    serialPortMessage += String(currentFill);
    serialPortMessage += "%";
    serialPortMessage += String(currentWeight);
    serialPortMessage += "g";
    Serial.println(serialPortMessage);
}

void printOledMessage(float currentFill, float currentWeight, int scaleID){
    int firstLineXAxis = 0;
    int firstLineYAxis = (scaleID % 2)*30+10;
    int secondLineXAxis = 45;
    int secondLineYAxis = (scaleID % 2)*30+20;
    String oledMessageFirstLine = String(SCALE_NAME[scaleID]);
    oledMessageFirstLine += ": ";
    String oledMessageSecondLine = "";
    if(currentFill > MINIMUM_FILL){
        oledMessageFirstLine += String(currentFill);
        oledMessageFirstLine += "%";
        oledMessageSecondLine += String(currentWeight);
        oledMessageSecondLine += " g";
    }
    else
        oledMessageFirstLine += "empty";
    u8g2.setCursor(firstLineXAxis, firstLineYAxis);
    u8g2.print(oledMessageFirstLine);
    u8g2.setCursor(secondLineXAxis, secondLineYAxis);
    u8g2.print(oledMessageSecondLine);
    if(scaleID % 2 == 1 || scaleID == NUMBER_OF_SCALES - 1)
            u8g2.sendBuffer();
}

String preparePushMessage(String pushMessage, float currentFill, int scaleID){
    pushMessage += String(SCALE_NAME[scaleID]);
    pushMessage += ": ";
    if(currentFill < MINIMUM_FILL)
        pushMessage += "empty";
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
    if(currentFill <= MINIMUM_FILL &&  oneHourTimers[scaleID] == 0){
        ::oneHourTimers[scaleID] = ntpClient.getUnixTime() + SECONDS_IN_ONE_HOUR;
        ::fillPushMessageSentFlags[scaleID] = false;
    }
    else if(currentFill > MINIMUM_FILL){
        ::oneHourTimers[scaleID] = 0;
        ::oneHourPushMessageSentFlags[scaleID] = false;
    }
}

void checkIfSendEmptyPushMessage(float currentFill, int scaleID){
    if(currentFill <= MINIMUM_FILL && oneHourPushMessageSentFlags[scaleID] == false && oneHourTimers[scaleID] <= ntpClient.getUnixTime()){
        ::oneHourPushMessageSentFlags[scaleID] = true;
        ::checkIfSendPushMessage = true;
    }
}   
 
bool checkIfSendFillPushMessage(float currentFill, int scaleID){
    if(currentFill > MINIMUM_FILL && currentFill < MAXIMUM_FILL && fillPushMessageSentFlags[scaleID] == false){
        ::fillPushMessageSentFlags[scaleID] = true;
        ::checkIfSendPushMessage = true;
    }
} 

void checkConnection(){
    if(status != WL_CONNECTED){
        int connectionAttempt = 1;
        int firstLineXAxis = 105;
        int firstLineYAxis = 25;
        int secondLineXAxis = 100;
        int secondLineYAxis = 35;
        u8g2.drawStr(firstLineXAxis, firstLineYAxis,"NO");
        u8g2.drawStr(secondLineXAxis, secondLineYAxis,"WiFi");
        connect(connectionAttempt);
    }
}

void sendPushMessage(String pushMessage){
    WiFiClient client;
    if (client.connect(logServer, CONNECTION_PORT)) {
    Serial.println("Client connected"); 
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

void softReset(){
    asm volatile ("  jmp 0");
}
