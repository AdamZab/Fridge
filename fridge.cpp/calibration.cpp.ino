#include "HX711.h" 
 
const int LOADCELL1_DOUT_PIN = 6;
const int LOADCELL1_SCK_PIN = 5;
HX711 scale1;
const int LOADCELL2_DOUT_PIN = 11;
const int LOADCELL2_SCK_PIN = 10;
HX711 scale2;
 
void setup() {
  Serial.begin(9600);
  scale1.begin(LOADCELL1_DOUT_PIN, LOADCELL1_SCK_PIN);
  scale1.setScale();  
  scale1.tare(); 
  scale2.begin(LOADCELL2_DOUT_PIN, LOADCELL2_SCK_PIN);
  scale2.setScale();  
  scale2.tare();
}

void loop() {
  float currentWeight1=scale.get1_units(20);  
  float scaleFactor=(currentWeight1/0.145);  // znana waga
  Serial.println(scaleFactor1);              // wartosc uzyta w scale1.setScale();
  float currentWeight1=scale.get1_units(20);  
  float scaleFactor=(currentWeight1/0.145);  // znana waga
  Serial.println(scaleFactor1);              // wartosc uzyta w scale2.setScale();
}
