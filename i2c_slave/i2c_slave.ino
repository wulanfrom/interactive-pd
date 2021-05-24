#include <Wire.h>

// slave address
#define SLAVE_ADDR 9
#define ANSWERSIZE 5
String ans = "hello";

void setup() {
  // know that we want to run in slave mode
  Wire.begin(SLAVE_ADDR);

  // fuction to run when data requested from master
  Wire.onRequest(requestEvent);

  // function to run when data is received from master
  Wire.onReceive(receiveEvent);

  // setup serial monitor
  Serial.begin(9600);
  Serial.println("I2C demosntration");
}

void loop() {
  // put your main code here, to run repeatedly:

}
