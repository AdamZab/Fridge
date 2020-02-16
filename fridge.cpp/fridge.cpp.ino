//Libraries
#include "HX711.h"
#include <WiFiNINA.h>
#include "arduino_secrets.h"
#include <EasyNTPClient.h>
#include <WiFiUdp.h>
#include <Arduino.h>
#include <U8g2lib.h>

#define MINIMUM_FILL 1
#define MAXIMUM_FILL 20
#define NUMBER_OF_SCALE_READINGS 5
#define POLAND_LOCAL_TIME 7200
#define SECONDS_IN_ONE_DAY 86400
#define SECONDS_IN_ONE_HOUR 3600
#define SECONDS_IN_ONE_MINUTE 60
#define TIME_OF_DAILY_MESSAGE 16
#define CONNECTION_PORT 80
#define OLED_SCL_PIN A4
#define OLED_SDA_PIN A5
#define SERIAL_PORT_BAUD 9600

//Scales
const int NUMBER_OF_SCALES = 5;
const char LOADCELL_SDA_PIN[NUMBER_OF_SCALES] = {3, 5, 7, 9, 11};
const char LOADCELL_SCL_PIN[NUMBER_OF_SCALES] = {2, 4, 6, 8, 12};
const long SCALE_FACTOR[NUMBER_OF_SCALES] = {381410, 393860, 0, 0 ,0};
const float NETTO[NUMBER_OF_SCALES] = {500.0f, 500.0f, 0.0f, 0.0f, 0.0f};
const float BRUTTO[NUMBER_OF_SCALES] = {560.0f, 560.0f, 0.0f, 0.0f, 0.0f};

const String SCALE_NAME[NUMBER_OF_SCALES] = {"Woda 1", "Woda 2", "Test 3", "Test 4", "Test 5"};
HX711 scale[NUMBER_OF_SCALES];

//WiFi connection
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

//Time
WiFiUDP UDP;
EasyNTPClient ntpClient(UDP, "pool.ntp.org", POLAND_LOCAL_TIME);

//Push

const char* log_server = "api.pushingbox.com";
String DEVICE_ID = "v6ED79EFFEB2DD3F";

//Oled

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, OLED_SCL_PIN, OLED_SDA_PIN, U8X8_PIN_NONE);


//Flags
bool fill_push_message_sent_flags[NUMBER_OF_SCALES];
bool one_hour_push_message_sent_flags[NUMBER_OF_SCALES];
bool daily_push_message_sent = false;
bool check_if_send_push_message = false;
long long one_hour_timers[NUMBER_OF_SCALES];

void setup() {
    Serial.begin(SERIAL_PORT_BAUD);
    u8g2.begin();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.clearBuffer();
    connect_with_reset();
    for(int scale_id = 0; scale_id < NUMBER_OF_SCALES; ++scale_id){
        scale[scale_id].begin(LOADCELL_SDA_PIN[scale_id], LOADCELL_SCL_PIN[scale_id]);
        scale[scale_id].set_scale(SCALE_FACTOR[scale_id]);  
        scale[scale_id].tare();
        reset_flags_and_timers(scale_id);    
    }  
}

void loop() {
    String push_message = "";
    ::check_if_send_push_message = false; 
    for(int scale_id = 0; scale_id < NUMBER_OF_SCALES; ++scale_id){
        next_oled_page_for_every_two_scales(scale_id);
        float current_weight, current_fill;
        current_weight = read_weight(current_weight, scale_id);
        current_fill = calculate_fill(current_weight, current_fill, scale_id);
        check_connection();
        print_serial_port_message(current_fill, current_weight, scale_id);
        print_oled_message(current_fill, current_weight, scale_id);
        push_message = prepare_push_message(push_message, current_fill, scale_id);
        start_one_hour_timer_if_empty(current_fill,  scale_id);
        check_if_send_empty_push_message(current_fill, scale_id);
        check_if_send_fill_push_message(current_fill, scale_id);
    }
    check_if_send_daily_push_message();
    if(check_if_send_push_message == true)
        send_push_message(push_message);
}

void connect_with_reset(){
    int connection_attempt = 1;
    int first_line_x_axis = 0;
    int first_line_y_axis = 10;
    int second_line_x_axis = 0;
    int second_line_y_axis = 20;
    u8g2.clearBuffer();
    u8g2.drawStr(first_line_x_axis,first_line_y_axis,"Connecting...");
    u8g2.sendBuffer();
    if (WiFi.status() == WL_NO_MODULE) 
        Serial.println("Communication with WiFi module failed!\n");
    while (WiFi.status() != WL_CONNECTED) {
        if (connection_attempt == 11){
            u8g2.clearBuffer();
            u8g2.drawStr(first_line_x_axis,first_line_y_axis,"Connection failed");
            u8g2.drawStr(second_line_x_axis,second_line_y_axis,"Restarting...");
            u8g2.sendBuffer();
            delay(5000);
            soft_reset();
        }
        connect(connection_attempt);
        ++connection_attempt;
    }
    u8g2.clearBuffer();
    u8g2.drawStr(first_line_x_axis,first_line_y_axis,"Connected!!!");
    u8g2.sendBuffer();
}
 
void connect(int arg_connection_attempt){
    String attempt_message = "Attempting to connect to WPA SSID: ";
    attempt_message += ssid;
    attempt_message += " Attempt: ";
    attempt_message += arg_connection_attempt;
    Serial.println(attempt_message);
    status = WiFi.begin(ssid, pass);
    delay(1000);
}

void reset_flags_and_timers(int arg_scale_id){
    ::fill_push_message_sent_flags[arg_scale_id] = false;
    ::one_hour_push_message_sent_flags[arg_scale_id] = false;
    ::daily_push_message_sent = false;
    ::one_hour_timers[arg_scale_id] = 0;
}

void next_oled_page_for_every_two_scales(int arg_scale_id){
    if(arg_scale_id % 2 == 0){
        delay(3000);
        u8g2.clearBuffer();
    }
}

float read_weight(float arg_current_weight, int arg_scale_id){
    float temporaryWeight = 0.0f;
    while(true){
        arg_current_weight = scale[arg_scale_id].get_units(NUMBER_OF_SCALE_READINGS) * 1000;
        int equal = arg_current_weight - temporaryWeight;
        if(fabs(equal) < 5)
            break;
        temporaryWeight = arg_current_weight;
    }
    return arg_current_weight;
}

float calculate_fill(float arg_current_weight, float arg_current_fill, int arg_scale_id){ 
    arg_current_fill = ((arg_current_weight - (BRUTTO[arg_scale_id] - NETTO[arg_scale_id])) * 100) / NETTO[arg_scale_id];
    return arg_current_fill;
}

void print_serial_port_message(float arg_current_fill, float arg_current_weight, int arg_scale_id){
    String serial_port_message = String(SCALE_NAME[arg_scale_id]);
    serial_port_message += ": ";
    serial_port_message += String(arg_current_fill);
    serial_port_message += "%";
    serial_port_message += String(arg_current_weight);
    serial_port_message += "g";
    Serial.println(serial_port_message);
}

void print_oled_message(float arg_current_fill, float arg_current_weight, int arg_scale_id){
    int first_line_x_axis = 0;
    int first_line_y_axis = (arg_scale_id % 2)*30+10;
    int second_line_x_axis = 45;
    int second_line_y_axis = (arg_scale_id % 2)*30+20;
    String oled_message_first_line = String(SCALE_NAME[arg_scale_id]);
    oled_message_first_line += ": ";
    String oled_message_second_line = "";
    if(arg_current_fill > MINIMUM_FILL){
        oled_message_first_line += String(arg_current_fill);
        oled_message_first_line += "%";
        oled_message_second_line += String(arg_current_weight);
        oled_message_second_line += " g";
    }
    else
        oled_message_first_line += "empty";
    u8g2.setCursor(first_line_x_axis, first_line_y_axis);
    u8g2.print(oled_message_first_line);
    u8g2.setCursor(second_line_x_axis, second_line_y_axis);
    u8g2.print(oled_message_second_line);
    if(arg_scale_id % 2 == 1 || arg_scale_id == NUMBER_OF_SCALES - 1)
            u8g2.sendBuffer();
}

String prepare_push_message(String arg_push_message, float arg_current_fill, int arg_scale_id){
    arg_push_message += String(SCALE_NAME[arg_scale_id]);
    arg_push_message += ": ";
    if(arg_current_fill < MINIMUM_FILL)
        arg_push_message += "empty";
    else{
        arg_push_message += String(arg_current_fill);
        arg_push_message += "%"; 
    } 
    arg_push_message += "\n";
    return arg_push_message;
}

void check_if_send_daily_push_message(){
    if((ntpClient.getUnixTime()  % SECONDS_IN_ONE_DAY) / SECONDS_IN_ONE_HOUR == 7)
        ::daily_push_message_sent = false;
    if((ntpClient.getUnixTime()  % SECONDS_IN_ONE_DAY) / SECONDS_IN_ONE_HOUR == TIME_OF_DAILY_MESSAGE && daily_push_message_sent == false){
        ::daily_push_message_sent = true;
        ::check_if_send_push_message = true;
    }
}  

void start_one_hour_timer_if_empty(float arg_current_fill, int arg_scale_id){
    if(arg_current_fill <= MINIMUM_FILL &&  one_hour_timers[arg_scale_id] == 0){
        ::one_hour_timers[arg_scale_id] = ntpClient.getUnixTime() + SECONDS_IN_ONE_HOUR;
        ::fill_push_message_sent_flags[arg_scale_id] = false;
    }
    else if(arg_current_fill > MINIMUM_FILL){
        ::one_hour_timers[arg_scale_id] = 0;
        ::one_hour_push_message_sent_flags[arg_scale_id] = false;
    }
}

void check_if_send_empty_push_message(float arg_current_fill, int arg_scale_id){
    if(arg_current_fill <= MINIMUM_FILL && one_hour_push_message_sent_flags[arg_scale_id] == false && one_hour_timers[arg_scale_id] <= ntpClient.getUnixTime()){
        ::one_hour_push_message_sent_flags[arg_scale_id] = true;
        ::check_if_send_push_message = true;
    }
}   
 
bool check_if_send_fill_push_message(float arg_current_fill, int arg_scale_id){
    if(arg_current_fill > MINIMUM_FILL && arg_current_fill < MAXIMUM_FILL && fill_push_message_sent_flags[arg_scale_id] == false){
        ::fill_push_message_sent_flags[arg_scale_id] = true;
        ::check_if_send_push_message = true;
    }
} 

void check_connection(){
    if(WiFi.status() != WL_CONNECTED){
        int connection_attempt = 1;
        int first_line_x_axis = 105;
        int first_line_y_axis = 25;
        int second_line_x_axis = 100;
        int second_line_y_axis = 35;
        u8g2.drawStr(first_line_x_axis, first_line_y_axis,"NO");
        u8g2.drawStr(second_line_x_axis, second_line_y_axis,"WiFi");
        connect(connection_attempt);
    }
}

void send_push_message(String arg_push_message){
    WiFiClient client;
    if (client.connect(log_server, CONNECTION_PORT)) {
    Serial.println("Client connected"); 
    String postStr = "devid=";
    postStr += String(DEVICE_ID);
    postStr += "&message_parameter=";
    postStr += String(arg_push_message);
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

void soft_reset(){
    asm volatile ("  jmp 0");
}
