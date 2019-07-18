#include "HX711.h"
#include <SPI.h>
#include <WiFiNINA.h>
#include "arduino_secrets.h"
#include <EasyNTPClient.h>
#include <WiFiUdp.h>


//scales
const int LOADCELL1_DOUT_PIN = 6;
const int LOADCELL1_SCK_PIN = 5;
const float netto1 = 500.0f;
const float brutto1 = 1205.0f;
HX711 scale1;
const int LOADCELL2_DOUT_PIN = 11;
const int LOADCELL2_SCK_PIN = 10;
const float netto2 = 1500.0f;
const float brutto2 = 1570.0f;
HX711 scale2;

//connection
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

//time
WiFiUDP udp;
EasyNTPClient ntpClient(udp, "pool.ntp.org", (60*60*2));
long timer1 = 0;
long timer2 = 0;

//push
const char* logServer = "api.pushingbox.com";
String deviceId = "v6ED79EFFEB2DD3F";
String message;
bool dailyMessageSent = false;
bool fillMessageSent1 = false;
bool fillMessageSent2 = false;

void setup() {

  Serial.begin(9600);
  connect();
   
  //scales
  scale1.begin(LOADCELL1_DOUT_PIN, LOADCELL1_SCK_PIN);
  scale1.set_scale(381410);  
  scale1.tare(); 
  scale2.begin(LOADCELL2_DOUT_PIN, LOADCELL2_SCK_PIN);
  scale2.set_scale(393860);  
  scale2.tare();

  Serial.println("Setup ended");
  delay(10000);
  
}

void loop() {

  if (status != WL_CONNECTED)
    connect();
    
  float temporaryWeight1 = 0.0f;
  float temporaryWeight2 = 0.0f;
  float currentWeight1;
  float currentWeight2;

  //scale 1
  while(true){
    currentWeight1 = scale1.get_units(20) * 1000;
    int equal = currentWeight1 - temporaryWeight1;
    if(fabs(equal) < 5)
      break;
    temporaryWeight1 = currentWeight1;
  }
  float currentFill1 = (currentWeight1 - (brutto1 - netto1) * 100) / netto1;
  Serial.println("Scale 1:");
  Serial.println(currentFill1, 2);
  message = "Scale 1: ";
  if(currentFill1 < 1){
    message += String("empty");
    if (timer1 == 0)
      timer1 = ntpClient.getUnixTime();
  }
  else{
    message += String(currentFill1);
    message += "%"; 
  } 
  
  //scale 2
  while(true){
    currentWeight2 = scale2.get_units(20) * 1000;
    int equal = currentWeight2 - temporaryWeight2;
    if(fabs(equal) < 5)
      break;
    temporaryWeight2 = currentWeight2;
  }
  float currentFill2 = ((currentWeight2 - (brutto2 - netto2)) * 100) / netto2;
  Serial.println("Scale 2:");
  Serial.println(currentFill2, 2);
  message += "\nScale 2: ";
  if(currentFill2 < 1){
    message += String("empty");
    if (timer2 == 0)
      timer2 = ntpClient.getUnixTime();
  }
  else{
    message += String(currentFill2);
    message += "%"; 
  }

  //daily message 16:00
  if((ntpClient.getUnixTime()  % 86400L) / 3600 == 16 && dailyMessageSent == false){  
    sendNotification(message);
    printTime();  
    dailyMessageSent = true;
  }

  //message: empty, 0-20%
  if(currentFill1 < 1 && fillMessageSent1 == false && ntpClient.getUnixTime() > (timer1 + 3600)){
    sendNotification(message);
    printTime();
    fillMessageSent1 = true;
  }
  else if(currentFill1 < 20 && currentFill1 > 1 && fillMessageSent1 == false){
    sendNotification(message);
    fillMessageSent1 = true;
  }
  
  if(currentFill2 < 1 && fillMessageSent2 == false && ntpClient.getUnixTime() > (timer2 + 3600)){
    sendNotification(message);
    printTime();
    fillMessageSent2 = true;
  }
  else if(currentFill2 < 20 && currentFill2 > 1 &&  fillMessageSent2 == false){
    sendNotification(message);
    fillMessageSent2 = true;
  }

  //reset flags
  if((ntpClient.getUnixTime()  % 86400L) / 3600 == 17)
    dailyMessageSent = false;
  if(currentFill1 > 21){
    fillMessageSent1 == false;
    timer1 = 0;
  }
  if(currentFill2 > 21){
    fillMessageSent2 == false;
    timer2 = 0;
  }
}



void connect(){
  if (WiFi.status() == WL_NO_MODULE) 
    Serial.print("Communication with WiFi module failed!\n"); 
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(10000);
  }
}

void sendNotification(String message){
  
  WiFiClient client;
    if (client.connect(logServer, 80)) {
    Serial.print("Client connected\n"); 
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
  
