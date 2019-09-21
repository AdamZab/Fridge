#include "HX711.h" 

const int NUMBER_OF_SCALES = 2;
const char LOADCELL_SDA_PIN[NUMBER_OF_SCALES] = {3, 5};
const char LOADCELL_SCL_PIN[NUMBER_OF_SCALES] = {2, 4};
HX711 scale[NUMBER_OF_SCALES];
#define NUMBER_OF_SCALE_READINGS 100
#define KNOWN_WEIGHT_IN_KG 0.145

 
void setup() {
    Serial.begin(9600);
    for(int scaleID = 0; scaleID < NUMBER_OF_SCALES; ++scaleID){
        scale[scaleID].begin(LOADCELL_SDA_PIN[scaleID], LOADCELL_SCL_PIN[scaleID]);  
        scale[scaleID].tare();    
    } 
}

void loop() {
    for(int scaleID = 0; scaleID < NUMBER_OF_SCALES; ++scaleID){
        float currentWeight = scale[scaleID].get_units(NUMBER_OF_SCALE_READINGS);
        float scaleFactor=(currentWeight/KNOWN_WEIGHT_IN_KG);
        String serialPortMessage = "Scale factor for scale ";
        serialPortMessage += String(scaleID);
        serialPortMessage += ": ";
        serialPortMessage += String(scaleFactor);
        Serial.println(serialPortMessage);
    }
}
