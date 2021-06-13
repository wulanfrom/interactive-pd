// lcd screen
#include <LiquidCrystal.h>
// microsd
#include <SPI.h>
#include <SD.h>
#include <pcmRF.h>
#include <pcmConfig.h>
#include <TMRpcm.h>
TMRpcm audio;

// functions
uint16_t calculateTime(int hours, int minutes, int seconds);
void display (uint16_t sec);

// MicroSD pins
const int SD_ChipSelectPin = 53; //ChipSelectPin
const int SD_MOSI = 51;
const int SD_SCK = 52;
const int SD_MISO = 50;

// Speaker + Module, microsd
const int SPEAKER = 11;
int audiofile = 0; 
int audioIsPlaying = 0;
unsigned long recordTime = 1;

// MAX9814 pins
#define MAX_OUT A0

// variables will change:
int buttonState = 0;         // variable for reading the pushbutton status

// LCD pins
const int rs = 26, en = 27, d4 = 30, d5 = 31, d6 = 28, d7 = 29;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Modes (Leave message)
const int MODE_IDLE = 0; //before the start
const int MODE_SHOW_START = 1; //host talks and replays
const int MODE_REPLAY_PAST = 2; //plays the user's past recording
const int MODE_SHOW_SPEAK = 3; //user talks for 5 minutes
const int MODE_ASK_FUTURE = 4; // hosts asks the user
const int MODE_LEAVE_FUTURE = 5; //user leaves message to the future
const int MODE_TALK_OUTRO = 6; //show outro plays
int currentMode = MODE_IDLE;

// Modes (Go through Archive)

// Start Btn
#define STARTBTN 3
int startBtnState = LOW;
int startBtnPrevState = LOW;
bool startBtnPressed = false;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Skip Btn
#define SKIPBTN 2 
int skipBtnState = LOW;
int skipBtnPrevState = LOW;
bool skipBtnPressed = false;

// Rotary Encoder
#define inputCLK 5 // GB
#define inputDT 4 // GA
int counter = 0;
int curStateCLK;
int prevStateCLK;
String encdir = "";
//unsigned long lastButtonPress = 0;
//int timePart; // hours: 0, minute: 1, seconds: 2

// timer
const int TALKING_MINUTES = 1; //5 minutes, talking time
const int LEAVE_MESSAGE_MINUTES = 1;
uint16_t timeLeft = calculateTime(0, TALKING_MINUTES, 0); // convert setup time to seconds
uint32_t lastTime = 0; //tto help with counting seconds

// data selection
int dataSelection = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  // Buttons
  pinMode(STARTBTN, INPUT); //start button
  pinMode(SKIPBTN, OUTPUT); // speaker

  // Speaker
  pinMode(SPEAKER, OUTPUT); // speaker

  // Rotary encoder
  pinMode(inputCLK, INPUT);
  pinMode(inputDT, INPUT);
  prevStateCLK = digitalRead(inputCLK);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcd.setCursor(0,0);

  // MAX_OUT
  pinMode(MAX_OUT, INPUT);

  // MicroSD
  pinMode(SD_ChipSelectPin, OUTPUT);
  digitalWrite(SD_ChipSelectPin, HIGH);
  Serial.print("Initializing SD card... ");
  if (!SD.begin(SD_ChipSelectPin)) {
    Serial.println("failed to load SD Card! ");
    while (true); //stay here
  }
  Serial.println("Mounting Successfull");
  audio.CSPin = SD_ChipSelectPin;
  audio.speakerPin = SPEAKER; // set speaker output to SPEAKER PIN
  audio.setVolume(5); // volume level: 0 - 7
  audio.quality(1); //set 1 for 2x oversampling, 0 for normal
}

void loop() {
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

  // SKIP BTN management
  skipBtnPressed = false;
  skipBtnState = digitalRead(SKIPBTN);
  if (skipBtnState != skipBtnPrevState && (millis() - lastDebounceTime) > debounceDelay) {
    skipBtnPressed = skipBtnState == HIGH;
    skipBtnPrevState = skipBtnState;
  } 

  if (skipBtnPressed) {
    Serial.println("SKIP button pressed");
  }

  //Rotary encoder
  curStateCLK = digitalRead(inputCLK);
  // If the previous and the current state of the inputCLK are different then a pulse has occured
   if (curStateCLK != prevStateCLK){ 
       
     // If the inputDT state is different than the inputCLK state then 
     // the encoder is rotating counterclockwise
     if (digitalRead(inputDT) != curStateCLK) { 
       counter --;
       encdir ="CCW";
       
     } else {
       // Encoder is rotating clockwise
       counter ++;
       encdir ="CW";
     }
     Serial.print("Direction: ");
     Serial.print(encdir);
     Serial.print(" -- Value: ");
     Serial.println(counter);
   } 
   // Update previousStateCLK with the current state
   prevStateCLK = curStateCLK; 

  // MODE MANAGEMENT
  switch (currentMode) {
    case MODE_IDLE: {
      //Serial.println("MODE_IDLE");
      lcd.setCursor(0,0);
      lcd.print("MODE_IDLE");
      // change with photoresistor levels later
      if (startBtnPressed) {
        currentMode = MODE_SHOW_START;
      }
      break;
    }
    case MODE_SHOW_START: {
      //Serial.println("MODE_SHOW_START");
      lcd.setCursor(0,0);
      lcd.print("MODE_SHOW_START");

      // PLAY AUDIO MECHANISM 
      audio.play("PREVIEW.wav");
      while (!startBtnPressed || !skipBtnPressed) {
        audioIsPlaying = audio.isPlaying(); 
        //Serial.println(audioIsPlaying);
        // SKIP BTN management
        skipBtnPressed = false;
        skipBtnState = digitalRead(SKIPBTN);
        if (skipBtnState != skipBtnPrevState && (millis() - lastDebounceTime) > debounceDelay) {
          skipBtnPressed = skipBtnState == HIGH;
          skipBtnPrevState = skipBtnState;
        }
        // if skip is pressed, go to next mode
        if (skipBtnPressed) {
          Serial.println("SKIPBTN btn pressed");
          audio.disable();
          while (!skipBtnPressed);
          currentMode = MODE_REPLAY_PAST;
          delay(200);
          break;
        }

        // PLAY/PAUSE BTN management
        startBtnPressed = false;
        startBtnState = digitalRead(STARTBTN);
        if (startBtnState != startBtnPrevState && (millis() - lastDebounceTime) > debounceDelay) {
          startBtnPressed = startBtnState == HIGH;
          startBtnPrevState = startBtnState;
        } 

        // if pause is pressed, pause audio
        if (startBtnPressed) {
          Serial.println("PAUSEBTN btn pressed");
          audio.pause();
          while (!startBtnPressed);
          delay(200);
        }

        // when the intro audio finishes playing
          if (!audioIsPlaying) {
            Serial.println("audio is finished, time to switch modes");
            currentMode = MODE_REPLAY_PAST;
            delay(200);
            break;
          }        
        }
      break;
    }
    case MODE_REPLAY_PAST: {
      lcd.setCursor(0,0);
      lcd.print("MODE_REPLAY_PAST");

      // PLAY AUDIO MECHANISM 
      audio.play("PREVIEW.wav");
      while (!startBtnPressed || !skipBtnPressed) {
        audioIsPlaying = audio.isPlaying(); 
        //Serial.println(audioIsPlaying);
        // SKIP BTN management
        skipBtnPressed = false;
        skipBtnState = digitalRead(SKIPBTN);
        if (skipBtnState != skipBtnPrevState && (millis() - lastDebounceTime) > debounceDelay) {
          skipBtnPressed = skipBtnState == HIGH;
          skipBtnPrevState = skipBtnState;
        }
        // if skip is pressed, go to next mode
        if (skipBtnPressed) {
          Serial.println("SKIPBTN btn pressed");
          audio.disable();
          while (!skipBtnPressed);
          currentMode = MODE_SHOW_SPEAK;
          delay(200);
          break;
        }

        // PLAY/PAUSE BTN management
        startBtnPressed = false;
        startBtnState = digitalRead(STARTBTN);
        if (startBtnState != startBtnPrevState && (millis() - lastDebounceTime) > debounceDelay) {
          startBtnPressed = startBtnState == HIGH;
          startBtnPrevState = startBtnState;
        } 

        // if pause is pressed, pause audio
        if (startBtnPressed) {
          Serial.println("PAUSEBTN btn pressed");
          audio.pause();
          while (!startBtnPressed);
          delay(200);
        }

        // when the intro audio finishes playing
          if (!audioIsPlaying) {
            Serial.println("audio is finished, time to switch modes");
            currentMode = MODE_SHOW_SPEAK;
            delay(200);
            break;
          }        
        }
      break;
    }
    case MODE_SHOW_SPEAK: { //talk for 5 minutes;
      lcd.setCursor(0,0);
      lcd.print("MODE_SHOW_SPEAK");
      // counting down
      lastTime = millis();
      while (timeLeft > 0) {
        if (millis() - lastTime >= 1000) {
            timeLeft--;
            lastTime += 1000;
            display(timeLeft);
            Serial.print("timeLeft: ");
            Serial.println(timeLeft);
        }
      }
      // Change to 5 minute timer countdown
      // When time runs out, go to the next mode
      currentMode = MODE_ASK_FUTURE;
      break;
    }
    case MODE_ASK_FUTURE: {
      lcd.setCursor(0,0);
      lcd.print("MODE_ASK_FUTURE");
      // PLAY AUDIO THAT ASKS WHAT DO U WANNA SAY TO UR FUTURE SELF
      audio.play("PREVIEW.wav");
      while (!startBtnPressed || !skipBtnPressed) {
        audioIsPlaying = audio.isPlaying(); 
        // Serial.println(audioIsPlaying);
        // SKIP BTN management
        skipBtnPressed = false;
        skipBtnState = digitalRead(SKIPBTN);
        if (skipBtnState != skipBtnPrevState && (millis() - lastDebounceTime) > debounceDelay) {
          skipBtnPressed = skipBtnState == HIGH;
          skipBtnPrevState = skipBtnState;
        }
        // if skip is pressed, go to next mode
        if (skipBtnPressed) {
          Serial.println("SKIPBTN btn pressed");
          audio.disable();
          while (!skipBtnPressed);
          timeLeft = calculateTime(0, LEAVE_MESSAGE_MINUTES, 0); //set leaving message time to 1 minute
          currentMode = MODE_LEAVE_FUTURE;
          delay(200);
          break;
        }

        // PLAY/PAUSE BTN management
        startBtnPressed = false;
        startBtnState = digitalRead(STARTBTN);
        if (startBtnState != startBtnPrevState && (millis() - lastDebounceTime) > debounceDelay) {
          startBtnPressed = startBtnState == HIGH;
          startBtnPrevState = startBtnState;
        } 

        // if pause is pressed, pause audio
        if (startBtnPressed) {
          Serial.println("PAUSEBTN btn pressed");
          audio.pause();
          while (!startBtnPressed);
          delay(200);
        }

        // when the asking audio finishes, then go to mode_show_future
        if (!audioIsPlaying) {
          Serial.println("audio is finished, time to switch to LEAVE FUTURE");
          timeLeft = calculateTime(0, LEAVE_MESSAGE_MINUTES, 0); //set leaving message time to 1 minute
          currentMode = MODE_LEAVE_FUTURE;
          delay(200);
          break;
        }         
      }
      break;
    }
    case MODE_LEAVE_FUTURE: {
      // User leaves message for future self
      //Serial.println("MODE_SHOW_FUTURE");
      lcd.setCursor(0,0);
      lcd.print("MODE_LEAVE_FUTURE");
      
      // 1 minute timer countdown
      // counting down
      lastTime = millis();
      // Serial.println(lastTime);
      // Record
      switch (recordTime) {
        case 0: {
          Serial.println("STOP recording..");
          audio.stopRecording("FUTURERECORDING.wav"); break;
          break;
        }
        case 1: {
          Serial.println("Recording..");
          audio.startRecording("FUTURERECORDING.wav", 16000, MAX_OUT);
          break;
        }
      }
      
      while (timeLeft > 0) {
        if (millis() - lastTime >= 1000) {
            timeLeft--;
            lastTime += 1000;
            display(timeLeft);
            Serial.print("timeLeft: ");
            Serial.println(timeLeft);
        }
      }
      Serial.print("recordTime: ");
      Serial.println(recordTime);
      recordTime = 0;
      
      // When time runs out, go to the next mode
      currentMode = MODE_TALK_OUTRO;
      break;
    }
    case MODE_TALK_OUTRO: {
      // Play the host's voice
      //Serial.println("MODE_TALK_OUTRO");
      lcd.setCursor(0,0);
      lcd.print("MODE_TALK_OUTRO");
      // PLAY OUTRO AUDIO
      audio.play("PREVIEW.wav");
      while (!startBtnPressed || !skipBtnPressed) {
        audioIsPlaying = audio.isPlaying(); 
        // Serial.println(audioIsPlaying);
        // SKIP BTN management
        skipBtnPressed = false;
        skipBtnState = digitalRead(SKIPBTN);
        if (skipBtnState != skipBtnPrevState && (millis() - lastDebounceTime) > debounceDelay) {
          skipBtnPressed = skipBtnState == HIGH;
          skipBtnPrevState = skipBtnState;
        }
        // if skip is pressed, go to next mode
        if (skipBtnPressed) {
          Serial.println("SKIPBTN btn pressed");
          audio.disable();
          while (!skipBtnPressed);
          currentMode = MODE_IDLE;
          delay(200);
          break;
        }

        // PLAY/PAUSE BTN management
        startBtnPressed = false;
        startBtnState = digitalRead(STARTBTN);
        if (startBtnState != startBtnPrevState && (millis() - lastDebounceTime) > debounceDelay) {
          startBtnPressed = startBtnState == HIGH;
          startBtnPrevState = startBtnState;
        } 

        // if pause is pressed, pause audio
        if (startBtnPressed) {
          Serial.println("PAUSEBTN btn pressed");
          audio.pause();
          while (!startBtnPressed);
          delay(200);
        }

        // when the asking audio finishes, then go to mode_show_future
        if (!audioIsPlaying) {
          Serial.println("audio is finished, time to switch modes");
          lastTime = calculateTime(0, LEAVE_MESSAGE_MINUTES, 0); //set leaving message time to 1 minute
          currentMode = MODE_IDLE;
          delay(200);
          break;
        }         
      }
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
  int mi = sec/60;
  int secs = sec % 60;

//   Serial.print(hr);
//   Serial.print(" : ");
//   Serial.print(mi);
//   Serial.print(" : ");
//   Serial.println(secs);
  lcd.setCursor(0,1);
  lcd.print(hr);
  lcd.print(" : ");
  lcd.print(mi);
  lcd.print(" : ");
  lcd.print(secs);
}
