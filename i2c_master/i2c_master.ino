// libraries
// MASTER cable 301

#include <Wire.h> //I2C
// microsd
#include <SPI.h>
#include <SD.h>
#include <pcmRF.h>
#include <pcmConfig.h>
#include <TMRpcm.h>

// functions
uint16_t calculateTime(int hours, int minutes, int seconds);

// I2C
#define SLAVE_ADDR 9

// Start Btn
#define STARTBTN 2
int startBtnState = LOW;
int startBtnPrevState = LOW;
bool startBtnPressed = false;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Play/Pause Btn
#define PAUSEBTN 3
int pauseBtnState = LOW;
int pauseBtnPrevState = LOW;
bool pauseBtnPressed = false;

// Skip Btn
#define SKIPBTN 4
int skipBtnState = LOW;
int skipBtnPrevState = LOW;
bool skipBtnPressed = false;

// Speaker + Module, microsd
#define SPEAKER 9
#define SD_ChipSelectPin 10
int nextBtn = 3;
int pauseBtn = 4;
TMRpcm audio;
char const intro[] = "introtune.wav";
char const past[] = "samplepast2.wav";

// timer
int setupHours = 0;
int setupMinutes = 0;
int setupSeconds = 0;
int totalTime = 0;

int currentHours = 0;
int currentMinutes = 0;
int currentSeconds = 0;
uint16_t timeLeft = calculateTime(setupHours, setupMinutes, setupSeconds); // convert setup time to seconds
uint32_t lastTime = 0; //tto help with counting seconds

// rotary encoder
#define inputCLK 5
#define inputDT 6
#define inputSW 7
int counter = 0;
int curStateCLK;
int prevStateCLK;
String encdir = "";
unsigned long lastButtonPress = 0;
int timePart; // hours: 0, minute: 1, seconds: 2

const int MODE_IDLE = 0; //before the start
const int MODE_INTRO = 1; //host talks and replays
const int MODE_REPLAY = 2; //replays past hope
const int MODE_TALK_SETUP = 3; //user talks about their day
const int MODE_TALK = 4; 
const int MODE_OUTRO = 5; //user closes the show
const int MODE_FUTURE = 6; //records
int currentMode = MODE_IDLE;

void setup() {
  // initialize I2C communications as mater
  // Wire.begin();
  Serial.begin(9600);

  pinMode(STARTBTN, INPUT); //start button
  pinMode(SPEAKER, OUTPUT); // speaker

  //rotary encoder
  pinMode(inputCLK, INPUT);
  pinMode(inputDT, INPUT);
  pinMode(inputSW, INPUT_PULLUP);
  prevStateCLK = digitalRead(inputCLK);
  
  //micro sd init
  pinMode(nextBtn, INPUT);
  pinMode(pauseBtn, INPUT);
  Serial.print("Initializing SD card... ");
  if (!SD.begin(SD_ChipSelectPin)) {
    Serial.println("failed to load SD Card! ");
    while (true); //stay here
  }
  Serial.println("Mounting Successfull");
  audio.speakerPin = SPEAKER; // set speaker output to pin 9
  audio.setVolume(4); // volume level: 0 - 7
  audio.quality(1); //set 1 for 2x oversampling, 0 for normal
}

void loop() {
//  delay(50);
  
  // write a character to the slave
//  Wire.beginTransmission(SLAVE_ADDR);
//  Wire.write(val);
//  Wire.endTransmission();

  // STARTBTN management
  startBtnPressed = false;
  startBtnState = digitalRead(STARTBTN);
  if (startBtnState != startBtnPrevState && (millis() - lastDebounceTime) > debounceDelay) {
    startBtnPressed = startBtnState == HIGH;
    startBtnPrevState = startBtnState;
  } 
  
  if (startBtnPressed) {
    Serial.println("start button pressed");
  }

  // PLAY/PAUSE BTN management
  pauseBtnPressed = false;
  pauseBtnState = digitalRead(PAUSEBTN);
  if (pauseBtnState != pauseBtnPrevState && (millis() - lastDebounceTime) > debounceDelay) {
    pauseBtnPressed = pauseBtnState == HIGH;
    pauseBtnPrevState = pauseBtnState;
  } 
  
  if (pauseBtnPressed) {
    Serial.println("pause button pressed IN SETUP");
  }

  // SKIP BTN management
  skipBtnPressed = false;
  skipBtnState = digitalRead(SKIPBTN);
  if (skipBtnState != skipBtnPrevState && (millis() - lastDebounceTime) > debounceDelay) {
    skipBtnPressed = skipBtnState == HIGH;
    skipBtnPrevState = skipBtnState;
  } 


  switch (currentMode) {
    case MODE_IDLE: {
      if (startBtnPressed) {
        currentMode = MODE_INTRO;
      }
      break;
    }
    case MODE_INTRO: {
      Serial.println("INTRO_MODE ENTERED");
      // starts playing audio and listens to input
      audio.play("preview.wav");
      while (!pauseBtnPressed || !skipBtnPressed) {
        // Button input management
        // PLAY/PAUSE BTN management
        pauseBtnPressed = false;
        pauseBtnState = digitalRead(PAUSEBTN);
        if (pauseBtnState != pauseBtnPrevState && (millis() - lastDebounceTime) > debounceDelay) {
          pauseBtnPressed = pauseBtnState == HIGH;
          pauseBtnPrevState = pauseBtnState;
        } 
        
        if (pauseBtnPressed) {
          Serial.println("pause button pressed");
        }
      
        // SKIP BTN management
        skipBtnPressed = false;
        skipBtnState = digitalRead(SKIPBTN);
        if (skipBtnState != skipBtnPrevState && (millis() - lastDebounceTime) > debounceDelay) {
          skipBtnPressed = skipBtnState == HIGH;
          skipBtnPrevState = skipBtnState;
        } 

        // if pause is pressed, pause audio
        if (pauseBtnPressed) {
          Serial.println("PAUSEBTN btn pressed");
          audio.pause();
          while (!pauseBtnPressed);
          delay(200);
        }

        // if skip is pressed, go to next mode
        if (skipBtnPressed) {
          Serial.println("SKIPBTN btn pressed");
          audio.disable();
          while (!skipBtnPressed);
          currentMode = MODE_REPLAY;
          delay(200);
          break;
        }

        // when the intro audio finishes playing
        if (!audio.isPlaying()) {
          Serial.println("audio is finished, time to switch modes");
          currentMode = MODE_REPLAY;
          delay(200);
          break;
        }
      }
      break;
    }
    case MODE_REPLAY: {
      Serial.println("ENTERRING REPLAY MODE");
      audio.play("preview.wav");
      while (!pauseBtnPressed || !skipBtnPressed) {
        // Button input management
        // PLAY/PAUSE BTN management
        pauseBtnPressed = false;
        pauseBtnState = digitalRead(PAUSEBTN);
        if (pauseBtnState != pauseBtnPrevState && (millis() - lastDebounceTime) > debounceDelay) {
          pauseBtnPressed = pauseBtnState == HIGH;
          pauseBtnPrevState = pauseBtnState;
        } 
        
        if (pauseBtnPressed) {
          Serial.println("pause button pressed");
        }
      
        // SKIP BTN management
        skipBtnPressed = false;
        skipBtnState = digitalRead(SKIPBTN);
        if (skipBtnState != skipBtnPrevState && (millis() - lastDebounceTime) > debounceDelay) {
          skipBtnPressed = skipBtnState == HIGH;
          skipBtnPrevState = skipBtnState;
        } 

        // if pause is pressed, pause audio
        if (pauseBtnPressed) {
          Serial.println("PAUSEBTN btn pressed in REPLAY");
          audio.pause();
          while (!pauseBtnPressed);
          delay(200);
        }

        // if skip is pressed, go to next mode
        if (skipBtnPressed) {
          Serial.println("SKIPBTN btn pressed in REPLAY");
          audio.disable();
          while (!skipBtnPressed);
          currentMode = MODE_TALK_SETUP;
          delay(200);
          break;
        }

        // when the intro audio finishes playing
        if (!audio.isPlaying()) {
          Serial.println("audio is finished, time to switch modes in REPLAY");
          currentMode = MODE_TALK_SETUP;
          Serial.println("TALK MODE");
          delay(200);
          break;
        }
      }
      break;
      }
    case MODE_TALK_SETUP: {
//      Serial.println("TALK MODE");
      // read current state of inputCLK
      curStateCLK = digitalRead(inputCLK);
      // if prev and cur state of the inputCLK are different then a pulse has occured
      // react only to 1 state change to avoid double count
      if (curStateCLK != prevStateCLK && curStateCLK == 1) {
        // if inputDR != inputCLK, encoder is counterclockwise
        if (digitalRead(inputDT) != curStateCLK) {
          if (timePart == 0) {
            // hours
            setupHours++;
            if (setupHours > 24) {
              setupHours = 0;
            }
          }
          else if (timePart == 1) {
            // minutes
            setupMinutes++; 
            if (setupMinutes > 60) {
              setupMinutes = 0;
            }
          }
          else {
            setupSeconds++;
            if (setupSeconds > 60) {
              setupSeconds = 0;
            }
          }

        } 
        // it it's CCW
        else {
            if (timePart == 0) {
              // hours
              setupHours--;
              if (setupHours < 0) {
                setupHours = 24;
              }
            }
            else if (timePart == 1) {
              // minutes
              setupMinutes--;
              if (setupMinutes < 0) {
                setupMinutes = 59;
              } 
            }
            else {
              // seconds
              setupSeconds--;
              if (setupSeconds < 0) {
                setupSeconds = 59;
              } 
            }
          }
//        Serial.print("Direction: ");
//        Serial.print(encdir);
//        Serial.print(" --- Value: ");
//        Serial.println(counter);
        Serial.print("Hours: ");
        Serial.print(setupHours);
        Serial.print(" -- Minutes: ");
        Serial.print(setupMinutes);
        Serial.print(" -- Seconds: ");
        Serial.println(setupSeconds); 
      }
      
      // update prevStateCLK with curState
      prevStateCLK = curStateCLK;

      // read btnState
      int swState = digitalRead(inputSW);
      // if we detect LOW, the button is pressed
      if (swState == LOW) {
        // if 50ms has passed since low, means that the button is pressed and released again
        if (millis() - lastButtonPress > 50) {
          timePart++;
          timePart = timePart % 3;
          Serial.print("timePart: ");
          Serial.println(timePart);
        }

        // remember last button press event
        lastButtonPress = millis();
      }

      // Button input management
      // PLAY/PAUSE BTN management
//      pauseBtnPressed = false;
//      pauseBtnState = digitalRead(PAUSEBTN);
//      if (pauseBtnState != pauseBtnPrevState && (millis() - lastDebounceTime) > debounceDelay) {
//        pauseBtnPressed = pauseBtnState == HIGH;
//        pauseBtnPrevState = pauseBtnState;
//      } 

      // go to talking mode after finish setting up the time
      if (pauseBtnPressed) {
        timeLeft = calculateTime(setupHours, setupMinutes, setupSeconds);
        Serial.println("pause button pressed in MODE_TALK");
        Serial.println("Setup time: ");
        Serial.print("Hours: ");
        Serial.print(setupHours);
        Serial.print(" -- Minutes: ");
        Serial.print(setupMinutes);
        Serial.print(" -- Seconds: ");
        Serial.println(setupSeconds); 
        Serial.print("Total time: ");
        Serial.println(timeLeft);
        Serial.println("=================");
        currentMode = MODE_TALK;
      }
      
      // help debounce reading
      delay(1);
      break;
    }

      case MODE_TALK: {
        // counting down  
        lastTime = millis();
//        while (!pauseBtnPressed || !skipBtnPressed) {
        while (timeLeft > 0) {
          if (millis() - lastTime >= 1000) {
            timeLeft--;
            lastTime += 1000;
            display(timeLeft);
            Serial.print("timeLeft: ");
            Serial.println(timeLeft);
          }
          // give the opportunity to pause?
        }

        // when time runs out, go to outro mode
        currentMode = MODE_OUTRO;
        break;  
      }

      case MODE_OUTRO:{
        // starts playing audio and listens to input
        Serial.println("OUTRO MODE ENTERRED");
        audio.play("preview.wav");
        while (!pauseBtnPressed || !skipBtnPressed) {
          // Button input management
          // PLAY/PAUSE BTN management
          pauseBtnPressed = false;
          pauseBtnState = digitalRead(PAUSEBTN);
          if (pauseBtnState != pauseBtnPrevState && (millis() - lastDebounceTime) > debounceDelay) {
            pauseBtnPressed = pauseBtnState == HIGH;
            pauseBtnPrevState = pauseBtnState;
          } 
          
          if (pauseBtnPressed) {
            Serial.println("pause button pressed");
          }
        
          // SKIP BTN management
          skipBtnPressed = false;
          skipBtnState = digitalRead(SKIPBTN);
          if (skipBtnState != skipBtnPrevState && (millis() - lastDebounceTime) > debounceDelay) {
            skipBtnPressed = skipBtnState == HIGH;
            skipBtnPrevState = skipBtnState;
          } 
  
          // if pause is pressed, pause audio
          if (pauseBtnPressed) {
            Serial.println("PAUSEBTN btn pressed");
            audio.pause();
            while (!pauseBtnPressed);
            delay(200);
          }
  
          // if skip is pressed, go to next mode
          if (skipBtnPressed) {
            Serial.println("SKIPBTN btn pressed");
            audio.disable();
            while (!skipBtnPressed);
            currentMode = MODE_REPLAY;
            delay(200);
            break;
          }
  
          // when the intro audio finishes playing
          if (!audio.isPlaying()) {
            Serial.println("audio is finished, time to switch modes");
            currentMode = MODE_FUTURE;
            delay(200);
            break;
          }
        }
      break;
    }

    case MODE_FUTURE: {
      break;
    }
  }
}

// HELPER FUNCTIONS
uint16_t calculateTime(int hours, int minutes, int seconds) {
  uint16_t resHours = 3600*hours;
  uint16_t resMinutes = 60*minutes;
  uint16_t resSeconds = seconds;
  return resHours + resMinutes + resSeconds;
}

void display (uint16_t sec) {
  int hr = sec/3600;
//  sec = sec - hr * 3600;
  int mi = sec/60;
  int secs = sec % 60;
//  sec = sec - mi*60;
   
   // print hr:mi:sec
//   Serial.println(hr, " : ", min, " : ", sec);
   Serial.print(hr);
   Serial.print(" : ");
   Serial.print(mi);
   Serial.print(" : ");
   Serial.println(secs);
}
