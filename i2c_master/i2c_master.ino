#include <Wire.h> //library for I2C
// I2C
#define SLAVE_ADDR 9
#define ANSWERSIZE 5 //slave answer size (5 char)

void setup() {
  // initialize I2C communications as mater
  Wire.begin();

  //Setup serial monitor
  Serial.begin(9600);
  Serial.println("I2C demonstration");

}

void loop() {
  // put your main code here, to run repeatedly:
  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write(0);
  Wire.endTransmission();

  Serial.println("receive data");
  Wire.requestFrom(SLAVE_ADDR, ANSWERSIZE);

  // add characters toa  string
  String response = "";
  while (Wire.available()) {
    char b = Wire.read();
    response += b;
  }

  // print to serial monitor
  Serial.println(response);
  
}
