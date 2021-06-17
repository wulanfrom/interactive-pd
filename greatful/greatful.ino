// lcd screen
#include <LiquidCrystal.h>

// microsd
#include <SPI.h>
#include <SD.h>
#include <TMRpcm.h>
TMRpcm audio;

// constants
// all the logs in a month, after 31 inputs, you have to take out the SDCard and clean it
int pastLogs[31]; // [0,1,2,3,4] corresponds to "PAST0.wav", "PAST1.wav", ...
int cur = 1;
// INITIALIZE THE FILES IN THE MICROSD with "PAST0.wav"
// "This is an example recording, don't forget to state your date of recording in the beginning!
// The recording is 1 minute long."

// functions
uint16_t calculateTime(int hours, int minutes, int seconds);
void display (uint16_t sec);
void initFiles(File dir, int numTabs);
String newFileName(int cur);
//void initializeLogs(File dir, int numTabs);

// MicroSD pins
const int SD_ChipSelectPin = 53; //ChipSelectPin
const int SD_MOSI = 51;
const int SD_SCK = 52;
const int SD_MISO = 50;
char* directory[][50] = {"Files on Card"};
int fileCounter = 0;

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
const int MODE_HOST_ASK = 3; // host asks you to talk
const int MODE_SHOW_SPEAK = 4; //user talks for 5 minutes
const int MODE_ASK_FUTURE = 5; // hosts asks the user
const int MODE_LEAVE_FUTURE = 6; //user leaves message to the future
const int MODE_TALK_OUTRO = 7; //show outro plays
const int MODE_EXPLORE_PAST = 8; // go through file names
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

// Photoresistor
#define photoPin A2
int lightCal;
int lightVal;

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
  File root = SD.open("/");
  initFiles(root,0);
    
  // TMRPCM
  audio.speakerPin = SPEAKER; // set speaker output to SPEAKER PIN
  audio.setVolume(5); // volume level: 0 - 7
  audio.quality(1); //set 1 for 2x oversampling, 0 for normal

  // photoResistor
  pinMode(photoPin, INPUT);
  lightCal = analogRead(photoPin);
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

   // PHOTO RESISTOR Management

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
//      lightVal = analogRead(photoPin);
//      Serial.print("LightVal: ");
//      Serial.println(lightVal);
      // Go to Explore Mode
      if (skipBtnPressed) {
        Serial.println("SKIP button pressed");
        currentMode = MODE_EXPLORE_PAST;
      }
      break;
    }
    case MODE_SHOW_START: {
      //Serial.println("MODE_SHOW_START");
      lcd.setCursor(0,0);
      lcd.print("MODE_SHOW_START");

      // PLAY AUDIO MECHANISM 
//      audio.play("/Audio/INTRO.wav");
//      audio.play("/Audio/INTRO1.wav");
      audio.play("INTRO1.wav");
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
//      audio.play("FUTURE.wav");
//      if (cur == 1){
//        audio.play("PAST0.wav"); // initialize microSD with 0
//      }/
//      else {
//        audio.play(futureFileNames(cur));
          playFile(cur); // play file
//      }
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
          currentMode = MODE_HOST_ASK;
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
            currentMode = MODE_HOST_ASK;
            delay(200);
            break;
          }        
        }
      break;
    }
    case MODE_HOST_ASK: {
      lcd.setCursor(0,0);
      lcd.print("MODE_HOST_ASK");
      // PLAY AUDIO THAT ASKS WHAT DO U WANNA SAY TO UR FUTURE SELF
//      audio.play("/Audio/TODAY1.wav"); // Host asks audio
      audio.play("TODAY1.wav");
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

        // when the asking audio finishes, then go to mode_show_future
        if (!audioIsPlaying) {
          Serial.println("audio is finished, time to switch to LEAVE FUTURE");
//          timeLeft = calculateTime(0, LEAVE_MESSAGE_MINUTES, 0); //set leaving message time to 1 minute
          timeLeft = calculateTime(0, 0, 30);
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
//      audio.play("/Audio/LEAVE1.wav");
      audio.play("LEAVE1.wav");
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
//          timeLeft = calculateTime(0, LEAVE_MESSAGE_MINUTES, 0); //set leaving message time to 1 minute
          timeLeft = calculateTime(0, 0, 30);
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
//          audio.stopRecording("FUTURE.wav"); break;
            stopRecordFile(cur); break;
          break;
        }
        case 1: {
          Serial.println("Recording..");
//          audio.startRecording("FUTURE.wav", 16000, MAX_OUT);
//            audio.startRecording(futureFileNames(cur), 16000, MAX_OUT);
              recordFile(cur); break;
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
//      audio.play("/Audio/OUTRO1.wav");
      audio.play("OUTRO1.wav");
//      audio.play("/Audio/OUTROHOST.wav");
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
          cur++; // add to the amount of files available
          currentMode = MODE_IDLE;
          delay(200);
          break;
        }         
      }
      break;
    }

    // Going through files
    case MODE_EXPLORE_PAST: {
//      Serial.print("Cur: ");
//      Serial.println(cur);
      int curVal = counter % cur; 
      lcd.setCursor(0, 0);
      String str = "PAST ENTRY " + String(curVal) + "          ";
      lcd.print(str);
//        lcd.print(str);
//      if (cur == 1) {
//        lcd.setCursor(0,0);
//        lcd.print("PAST ENTRY 1");
//      }
//      else {
//        if (counter >= 0) {
//          curVal = counter % cur; 
//        }
//        else {
//          curVal = (counter - 1 + cur) % 12;
//        }
//        lcd.setCursor(0, 0);
////        lcd.print("                          ");
//        String str = "PAST ENTRY " + String(curVal) + "          ";
//        lcd.print(str);
//      }

      // if the start button is pressed
      if (startBtnPressed) {
//        audio.play("PAST2.wav");
//        audio.play(futureFileNames(curVal + 1));
        playFile(curVal + 1);
        while (!startBtnPressed || !skipBtnPressed) {
          audioIsPlaying = audio.isPlaying(); 
          
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
//            currentMode = MODE_IDLE;
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
            Serial.println("audio finished playing");
            delay(200);
            break;
          }         
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

void initFiles(File dir, int numTabs)
{
  int counter = 0;
  while (true)
  {
    File entry =  dir.openNextFile();
    if (! entry)
    {
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++)
    {
//      Serial.print('\t');
    }
//    Serial.println(entry.name());
    String filename = entry.name();
    String beginning = filename.substring(0, 6);
//    Serial.print("beginning: ");
//    Serial.println(beginning);
    if (beginning == "PAST" and counter < 31) {
//      pastLogs[counter] = filename;
      counter++;
    }
    entry.close();
  }
}

// populate the current logs with existing ones in the SD card
void initializeLogs(File dir, int numTabs) { 
  while (true)
  {
    File entry =  dir.openNextFile();
    if (! entry)
    {
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++)
    {
      Serial.print('\t');
    }
    Serial.println(entry.name());
    entry.close();
  }
}

char* futureFileNames(int num) {
  char str[10] = "";
  switch (num) {
    case 1: {
      str[10] = "PAST0.wav";
      break;
    }
    case 2: {
      str[10] = "PAST1.wav";
      break;
    }
    case 3: {
      str[10] = "PAST2.wav";
      break;
    }
    case 4: {
      str[10] = "PAST3.wav";
      break;
    }
    case 5: {
      str[10] = "PAST4.wav";
      break;
    }
    case 6: {
      str[10] = "PAST5.wav";
      break;
    }
    case 7: {
      str[10] = "PAST6.wav";
      break;
    }
    case 8: {
      str[10] = "PAST7.wav";
      break;
    }
    case 9: {
      str[10] = "PAST8.wav";
      break;
    }
    case 10: {
      str[10] = "PAST9.wav";
      break;
    }
    case 11: {
      str[10] = "PAST10.wav";
      break;
    } 
    case 12: {
      str[10] = "PAST11.wav";
      break;
    }
    case 13: {
      str[10] = "PAST12.wav";
      break;
    }case 14: {
      str[10] = "PAST13.wav";
      break;
    }
    case 15: {
      str[10] = "PAST14.wav";
      break;
    }case 16: {
      str[10] = "PAST15.wav";
      break;
    }case 17: {
      str[10] = "PAST16.wav";
      break;
    }case 18: {
      str[10] = "PAST17.wav";
      break;
    }case 19: {
      str[10] = "PAST18.wav";
      break;
    }case 20: {
      str[10] = "PAST19.wav";
      break;
    }case 21: {
      str[10] = "PAST20.wav";
      break;
    }case 22: {
      str[10] = "PAST21.wav";
      break;
    }case 23: {
      str[10] = "PAST22.wav";
      break;
    }case 24: {
      str[10] = "PAST23.wav";
      break;
    }case 25: {
      str[10] = "PAST24.wav";
      break;
    }case 26: {
      str[10] = "PAST25.wav";
      break;
    }case 27: {
      str[10] = "PAST26.wav";
      break;
    }case 28: {
      str[10] = "PAST27.wav";
      break;
    }case 29: {
      str[10] = "PAST28.wav";
      break;
    }case 30: {
      str[10] = "PAST29.wav";
      break;
    }case 31: {
      str[10] = "PAST30.wav";
      break;
    }
  }
  return str;
}

void playFile(int num) {
  switch (num) {
    case 1: {
      audio.play("PAST0.wav");
      break;
    }
    case 2: {
      audio.play("PAST1.wav");
      break;
    }
    case 3: {
      audio.play("PAST2.wav");
      break;
    }
    case 4: {
      audio.play("PAST3.wav");
      break;
    }
    case 5: {
      audio.play("PAST4.wav");
      break;
    }
    case 6: {
      audio.play("PAST5.wav");
      break;
    }
    case 7: {
      audio.play("PAST6.wav");
      break;
    }
    case 8: {
      audio.play("PAST7.wav");
      break;
    }
    case 9: {
      audio.play("PAST8.wav");
      break;
    }
    case 10: {
      audio.play("PAST9.wav");
      break;
    }
    case 11: {
      audio.play("PAST10.wav");
      break;
    } 
    case 12: {
      audio.play("PAST11.wav");
      break;
    }
    case 13: {
      audio.play("PAST12.wav");
      break;
    }case 14: {
      audio.play("PAST13.wav");
      break;
    }
    case 15: {
      audio.play("PAST14.wav");
      break;
    }case 16: {
      audio.play("PAST15.wav");
      break;
    }case 17: {
      audio.play("PAST16.wav");
      break;
    }case 18: {
      audio.play("PAST17.wav");
      break;
    }case 19: {
      audio.play("PAST18.wav");
      break;
    }case 20: {
      audio.play("PAST19.wav");
      break;
    }case 21: {
      audio.play("PAST20.wav");
      break;
    }case 22: {
      audio.play("PAST21.wav");
      break;
    }case 23: {
      audio.play("PAST22.wav");
      break;
    }case 24: {
      audio.play("PAST23.wav");
      break;
    }case 25: {
      audio.play("PAST24.wav");
      break;
    }case 26: {
      audio.play("PAST25.wav");
      break;
    }case 27: {
      audio.play("PAST26.wav");
      break;
    }case 28: {
      audio.play("PAST27.wav");
      break;
    }case 29: {
      audio.play("PAST28.wav");
      break;
    }case 30: {
      audio.play("PAST29.wav");
      break;
    }case 31: {
      audio.play("PAST30.wav");
      break;
    }
  }
}

void recordFile(int num) {
  switch (num) {
    case 1: {
//      audio.play("PAST0.wav");
      audio.startRecording("PAST1.wav", 16000, MAX_OUT);
      break;
    }
    case 2: {
//      audio.play("PAST1.wav");
      audio.startRecording("PAST2.wav", 16000, MAX_OUT);
      break;
    }
    case 3: {
//      audio.play("PAST2.wav");
      audio.startRecording("PAST3.wav", 16000, MAX_OUT);
      break;
    }
    case 4: {
//      audio.play("PAST3.wav");
      audio.startRecording("PAST4.wav", 16000, MAX_OUT);
      break;
    }
    case 5: {
      audio.startRecording("PAST5.wav", 16000, MAX_OUT);
//      audio.play("PAST4.wav");
      break;
    }
    case 6: {
      audio.startRecording("PAST6.wav", 16000, MAX_OUT);
      audio.play("PAST5.wav");
      break;
    }
    case 7: {
      audio.startRecording("PAST7.wav", 16000, MAX_OUT);
//      audio.play("PAST6.wav");
      break;
    }
    case 8: {
//      audio.play("PAST7.wav");
      audio.startRecording("PAST8.wav", 16000, MAX_OUT);
      break;
    }
    case 9: {
      audio.startRecording("PAST9.wav", 16000, MAX_OUT);
//      audio.play("PAST8.wav");
      break;
    }
    case 10: {
      audio.startRecording("PAST10.wav", 16000, MAX_OUT);
//      audio.play("PAST9.wav");
      break;
    }
    case 11: {
      audio.startRecording("PAST11.wav", 16000, MAX_OUT);
//      audio.play("PAST10.wav");
      break;
    } 
    case 12: {
      audio.startRecording("PAST12.wav", 16000, MAX_OUT);
//      audio.play("PAST11.wav");
      break;
    }
    case 13: {
      audio.startRecording("PAST13.wav", 16000, MAX_OUT);
//      audio.play("PAST12.wav");
      break;
    }case 14: {
      audio.startRecording("PAST14.wav", 16000, MAX_OUT);
//      audio.play("PAST13.wav");
      break;
    }
    case 15: {
      audio.startRecording("PAST15.wav", 16000, MAX_OUT);
//      audio.play("PAST14.wav");
      break;
    }case 16: {
      audio.startRecording("PAST16.wav", 16000, MAX_OUT);      
//      audio.play("PAST15.wav");
      break;
    }case 17: {
      audio.startRecording("PAST17.wav", 16000, MAX_OUT);
//      audio.play("PAST16.wav");
      break;
    }case 18: {
      audio.startRecording("PAST18.wav", 16000, MAX_OUT);
//      audio.play("PAST17.wav");
      break;
    }case 19: {
      audio.startRecording("PAST19.wav", 16000, MAX_OUT);
//      audio.play("PAST18.wav");
      break;
    }case 20: {
      audio.startRecording("PAST20.wav", 16000, MAX_OUT);
//      audio.play("PAST19.wav");
      break;
    }case 21: {
      audio.startRecording("PAST21.wav", 16000, MAX_OUT);
//      audio.play("PAST20.wav");
      break;
    }case 22: {
      audio.startRecording("PAST22.wav", 16000, MAX_OUT);
//      audio.play("PAST21.wav");
      break;
    }case 23: {
      audio.startRecording("PAST23.wav", 16000, MAX_OUT);
//      audio.play("PAST22.wav");
      break;
    }case 24: {
      audio.startRecording("PAST24.wav", 16000, MAX_OUT);
//      audio.play("PAST23.wav");
      break;
    }case 25: {
      audio.startRecording("PAST25.wav", 16000, MAX_OUT);
//      audio.play("PAST24.wav");
      break;
    }case 26: {
      audio.startRecording("PAST26.wav", 16000, MAX_OUT);
//      audio.play("PAST25.wav");
      break;
    }case 27: {
      audio.startRecording("PAST27.wav", 16000, MAX_OUT);
//      audio.play("PAST26.wav");
      break;
    }case 28: {
      audio.startRecording("PAST28.wav", 16000, MAX_OUT);
//      audio.play("PAST27.wav");
      break;
    }case 29: {
      audio.startRecording("PAST29.wav", 16000, MAX_OUT);
//      audio.play("PAST28.wav");
      break;
    }case 30: {
      audio.startRecording("PAST30.wav", 16000, MAX_OUT);
//      audio.play("PAST29.wav");
      break;
    }case 31: {
      audio.startRecording("PAST31.wav", 16000, MAX_OUT);
//      audio.play("PAST30.wav");
      break;
    }
  }
}

void stopRecordFile(int num) {
  switch (num) {
    case 1: {
      audio.stopRecording("PAST1.wav");
      break;
    }
    case 2: {
      audio.stopRecording("PAST2.wav");
      break;
    }
    case 3: {
      audio.stopRecording("PAST3.wav");
      break;
    }
    case 4: {
      audio.stopRecording("PAST4.wav");
      break;
    }
    case 5: {
      audio.stopRecording("PAST5.wav");
      break;
    }
    case 6: {
      audio.stopRecording("PAST6.wav");
      break;
    }
    case 7: {
      audio.stopRecording("PAST7.wav");
      break;
    }
    case 8: {
      audio.stopRecording("PAST8.wav");
      break;
    }
    case 9: {
      audio.stopRecording("PAST9.wav");
      break;
    }
    case 10: {
      audio.stopRecording("PAST10.wav");
      break;
    }
    case 11: {
      audio.stopRecording("PAST11.wav");
      break;
    }
    case 12: {
      audio.stopRecording("PAST12.wav");
      break;
    }
    case 13: {
      audio.stopRecording("PAST13.wav");
      break;
    }
    case 14: {
      audio.stopRecording("PAST14.wav");
      break;
    }
    case 15: {
      audio.stopRecording("PAST15.wav");
      break;
    }
    case 16: {
      audio.stopRecording("PAST16.wav");
      break;
    }
    case 17: {
      audio.stopRecording("PAST17.wav");
      break;
    }
    case 18: {
      audio.stopRecording("PAST18.wav");
      break;
    }
    case 19: {
      audio.stopRecording("PAST19.wav");
      break;
    }
    case 20: {
      audio.stopRecording("PAST20.wav");
      break;
    }
    case 21: {
      audio.stopRecording("PAST21.wav");
      break;
    }
    case 22: {
      audio.stopRecording("PAST22.wav");
      break;
    }
    case 23: {
      audio.stopRecording("PAST23.wav");
      break;
    }
    case 24: {
      audio.stopRecording("PAST24.wav");
      break;
    }
    case 25: {
      audio.stopRecording("PAST25.wav");
      break;
    }
    case 26: {
      audio.stopRecording("PAST27.wav");
      break;
    }
    case 27: {
      audio.stopRecording("PAST27.wav");
      break;
    }
    case 28: {
      audio.stopRecording("PAST28.wav");
      break;
    }
    case 29: {
      audio.stopRecording("PAST29.wav");
      break;
    }
    case 30: {
      audio.stopRecording("PAST30.wav");
      break;
    }
    case 31: {
      audio.stopRecording("PAST31.wav");
      break;
    }
  }
}
