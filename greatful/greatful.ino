#include <LiquidCrystal.h>
//#include <pcmRF.h>
//#include <pcmConfig.h>
//#include <TMRpcm.h>

//functions
void record();

// MicroSD pins
const int SD_ChipSelect = 22; //ChipSelectPin
const int SD_MOSI = 23;
const int SD_SCK = 24;
const int SD_MISO = 25;

// MAX9814 pins
//#define MAX_OUT A0

//TMRpcm audio;
//int audioFile = 0;
//unsigned long i = 0;
//bool recmode = 0; // to check whether it's in recording mode or not

// LCD pins
const int rs = 26, en = 27, d4 = 30, d5 = 31, d6 = 28, d7 = 29;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("hello, world!");

}

void loop() {
  // put your main code here, to run repeatedly:
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  lcd.print(millis() / 1000);

}
