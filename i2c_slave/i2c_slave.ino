#include <Wire.h>

// slave address
#define SLAVE_ADDR 9
#define ANSWERSIZE 5
String ans = "hello";

// isd1820 module
#define REC 2 //for recording
#define PLAY_E 3 // for playback-edge trigger
#define FT 5 // for feed through
#define playTime 5000 // playback time 5 seconds
#define recordTime 10000 // recording time 10 seconds, how to increase?
#define playLTime 900 // press and release playbacktime 0.9 seconds

// global trigger
int recStatus = LOW;
int replayStatus = LOW;
int mode = -1; //-1 for idle, 0 for play, 1 for record

void setup() {
  // setup serial monitor
  Serial.begin(9600);

  // sd1820 setup
  pinMode(REC,OUTPUT);// set the REC pin as output
  pinMode(PLAY_E,OUTPUT);// set the PLAY_e pin as output
  pinMode(FT,OUTPUT);// set the FT pin as output
  
  Serial.println("I2C demosntration");
  // know that we want to run in slave mode
  Wire.begin(SLAVE_ADDR);


  // function to run when data is received from master
  Wire.onReceive(receiveEvent);
  
  Serial.println("SETUP DONE");
}

void loop() {
  // put your main code here, to run repeatedly:
//  Serial.println("recording...");
//  digitalWrite(REC, HIGH);
//  delay(recordTime);
//  digitalWrite(REC, LOW); 
//  Serial.println("replaying..."); 
//  digitalWrite(PLAY_E, HIGH);
//  delay(50);
//  digitalWrite(PLAY_E, LOW);  
//  delay(playTime);
//  
//  delay(500);

  // play recording
  if (mode == 0) {
    Serial.println("replaying..."); 
    digitalWrite(PLAY_E, HIGH);
    delay(50);
    digitalWrite(PLAY_E, LOW);  
    delay(playTime);
    mode = -1; // end mode
  }
  // record voice
  else if (mode == 1) {
    Serial.println("recording...");
    digitalWrite(REC, HIGH);
    delay(recordTime);
    digitalWrite(REC, LOW); 
    mode = -1; //end mode
  }
  else {
    return;
  }
  Serial.print("mode: ");
  Serial.println(mode);
  delay(1);
}

void receiveEvent(int action) { 
  mode = Wire.read(); // 0 for play, 1 for recording
}
