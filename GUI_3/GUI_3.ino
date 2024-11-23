// Parts used (links are for reference, other similar products *may* suffice):
// - Arduino mega 2560 (https://www.amazon.com/ELEGOO-ATmega2560-ATMEGA16U2-Projects-Compliant/dp/B01H4ZLZLQ/ref=sr_1_4?crid=2YIGZ15X03LN6&keywords=arduino+mega+2560&qid=1677782417&sprefix=arduino+mega+2560%2Caps%2C114&sr=8-4)
// - elegoo 2.8" tft lcd screen (https://www.amazon.com/Elegoo-EL-SM-004-Inches-Technical-Arduino/dp/B01EUVJYME)
// - rotary encoder module (https://www.amazon.com/Maxmoral-Encoder-Degrees-Compatible-Development/dp/B07M631J1Q/ref=sr_1_16?crid=ON1XLW72P0QA&keywords=rotary+encoder+module&qid=1677782439&sprefix=rotary+encoder+modul%2Caps%2C116&sr=8-16)

// each page in this code is made from a combination of these types of functions:
// button function - these are interactive, and are drawn differently based on conditions
// setup function - draws the entire page when it is first called
// display - draws something to the tft that usually cannot be interacted with

// I wrote my own code for handling the buttons and button selection. The main functions for rotary encoders and the turning of rotary encoders 
// are at the bottom of the file (RETurned, REPressed)


#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <RotaryEncoder.h>
#include <SPI.h>
#include <SD.h>

                     // colors {WHITE,  BLACK,  GRAY,   GREEN,  RED,    BLUE,  MAGENTA, PURPLE, CYAN,   YELLOW, ORANGE}
const uint16_t allColors[11] = {0xFFFF, 0x0000, 0x9CF3, 0x07E0, 0xF800, 0x001F, 0xE99F, 0x899F, 0x37FD, 0xFFC0, 0xFD07};
uint16_t FgColor;
uint16_t BgColor; 
uint16_t TmpFg;
uint16_t TmpBg;

#define PIN_IN1 A13
#define PIN_IN2 A14
#define PIN_IN3 A15

#define PIN_L    50
#define PIN_M    51
#define PIN_H    52
#define PIN_CTRL 53

const uint32_t MAX32 = 4294967000;

// rotary encoder
int lastBState = 1;  // rotary encoder button last state
RotaryEncoder *encoder = nullptr;
void checkPosition()
{
  encoder->tick();
}

// tft
MCUFRIEND_kbv tft;

// SD Card
Sd2Card card;
SdVolume volume;

// Rotary encoder button
void REPressed();
void RETurned();

// Page setup Functions
void homePageSetup();
void newPageSetup();
void recentPageSetup();
void runPageSetup();
// void optionsPageSetup();
// void infoPageSetup();

// Home buttons
void homeBTreatmentNew(bool setup = false);
void homeBTreatmentRecent(bool setup = false);
void homeBMoreOptions(bool setup = false);
void homeBMoreInfo(bool setup = false);

// New buttons
void newBMode0();
void newBMode1();
void newBMode2();
void newBTimeHours();
void newBTimeMins();
void newBContinue();
void newBReturn();

// Recent buttons
void recentBTreatment(uint16_t num=0, bool setup = false);
void recentBLeft();
void recentBReturn();
void recentBRight();
void saveCurTreatment();
void updatewPageItems();

// Options Buttons
void optionsBFg();
void optionsBBg();
void optionsBApply();
void optionsBReturn();

// Run buttons
void runBPlayPause();
void runBRestart();
void runBEnd();
void runControl();

// Button Array intialization
void (*homeButtons[4])() = {homeBTreatmentNew, homeBTreatmentRecent, homeBMoreOptions, homeBMoreInfo};
void (*newButtons[7])() = {newBMode0, newBMode1, newBMode2, newBTimeHours, newBTimeMins, newBContinue, newBReturn};
void (*recentButtons[3])() = {recentBLeft, recentBReturn, recentBRight};
void (*optionsButtons[4])() = {optionsBFg, optionsBBg, optionsBApply, optionsBReturn};
void (*runButtons[3])() = {runBPlayPause, runBRestart, runBEnd};

void printTime(uint8_t num, uint16_t x, uint16_t y, uint8_t textSize, int tColor, bool erase = false);
void drawTitle(String title, int y, int doLine = 0, uint16_t tColor = FgColor);

// Global variables for tracking the current page
enum Page {NONE, HOME, NEW, RECENT, OPTIONS, INFO, RUN};
Page currentPage = HOME;

// Global variables for tracking the current & previous buttons
int currentButton = 0;
int previousButton = -1;

// RECENT page global variables
uint16_t recentTotNumItems;
int recentTotNumPages;
String recentPageItems[6] = {"", "", "", "", "", ""};
uint32_t recentCurPageNum = 0;
int8_t recentCPNI = 6;  // current page number of items

// NEW page variables
int erase = false;

// RUN page variables
bool isRunning = false;
uint32_t startTime;

File recentTreatments;
File options;

struct Treatment {
  uint32_t hours;
  uint32_t minutes;
  uint32_t tot_tseconds;
  uint32_t cur_tseconds;
  uint8_t strength;
};

struct Treatment activeTreatment;

void setup() {
  // initialize Serial
  Serial.begin(9600);
  while(!Serial);
  
  // make sure system is set correctly
  digitalWrite(PIN_L, LOW);
  digitalWrite(PIN_M, LOW);
  digitalWrite(PIN_H, LOW); 
  digitalWrite(PIN_CTRL, HIGH);
  
  // Initialize micro SD card
  if (!SD.begin(10)) {
    Serial.println("initialization of SD card failed!");
    delay(500);
    exit(0);
  }  

  // use FOUR3 mode when PIN_IN1, PIN_IN2 signals are always HIGH in latch position.
  encoder = new RotaryEncoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::FOUR3);
  // register interrupt routine
  attachInterrupt(digitalPinToInterrupt(PIN_IN1), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_IN2), checkPosition, CHANGE);

  // open recent treatments file and measure file length, or create new file 
  recentTreatments = SD.open("rt.txt");
  if (recentTreatments){  // file exists
    while (recentTreatments.available()){
      if (recentTreatments.read() == '\n') recentTotNumItems++;
    }
  } else {  // file doesnt exist
    recentTreatments.close();
    recentTreatments = SD.open("rt.txt", FILE_WRITE);
    if (!recentTreatments){
      Serial.println("Error: unable to open file rt.txt");
      delay(500);
      exit(0);      
    }
    recentTreatments.close();    
  }
  recentTreatments.close();
  
  activeTreatment.tot_tseconds = 0;
  activeTreatment.cur_tseconds = activeTreatment.tot_tseconds;
  activeTreatment.strength = 0;

  // open options file and grab options, or create new file 
  grabOptions();
  
  // initialize tft
  tft.reset();
  uint16_t identifier = tft.readID();
  Serial.print("ID = 0x");
  Serial.println(identifier, HEX);
  if (identifier == 0xEFEF) identifier = 0x9486;
  tft.begin(identifier);
  homePageSetup();  
}

void loop() {
  // check for rotary encoder pressed
  int curBState = digitalRead(PIN_IN3);
  if (lastBState == 0 && curBState == 1) {
    REPressed();
    lastBState = curBState;
    return;
  }

  // check for rotary encoder turned
  static int pos = 0;
  encoder->tick();
  int newPos = (int)encoder->getPosition();
  if (pos != newPos) {
    RETurned();
    pos = newPos;
  }
  lastBState = curBState;

  // timer control for RUN page
  if (isRunning) {  // fix: make it erase only the digit that changes instead of both each time. Might require a new printTime function which only erases/prints one digit
    if (millis() - startTime >= 1000){
      if (activeTreatment.cur_tseconds <= 0) {
        runControl(false);
        activeTreatment.cur_tseconds = 0;
        runBPlayPause();        
        return;
      }
      activeTreatment.cur_tseconds--;
      uint32_t s = activeTreatment.cur_tseconds;
      printTime(s % 60, 157, 120, 4, 0, true);              // seconds
      if ((s+1) % 60 == 0){
        printTime((s % 3600) / 60, 96, 120, 4, 0, true);      // minutes
        if ((s+1) % 3600 == 0){
          printTime(s / 3600, 35, 120, 4, 0, true);             // hours
        }
      }
      startTime = millis();
    }
  }
}

// open options options.txt and grab options, or create new file
void grabOptions() {
  options = SD.open("options.txt");
  if (options){  // file exists
    int n0 = 0;
    int n1 = 0;
    char tempChar;
    uint8_t FgSel = 0;
    uint8_t BgSel = 0;
    while (options.available()){
      tempChar=options.read();
      if (tempChar == '\n') {
        n0++;
        n1 = 0;
        continue;
      } else if (int(tempChar) == 13) continue;
      switch (n0) {
        case 0:  // Grab forground color
          if (n1 > 3) {
            FgSel = FgSel*10 + (int)(tempChar-'0');
          }
          break;
        case 1:  // Grab background color
          if (n1 > 3) {
            BgSel = BgSel*10 + (int)(tempChar-'0');
          }
          break;
      }
      n1++;
    }
    FgColor = allColors[FgSel];
    BgColor = allColors[BgSel];    
  } else {  // file does not exist
    options.close();
    options = SD.open("options.txt", FILE_WRITE);
    if (!options){
      Serial.println("Error: unable to open file options.txt");
      delay(500);
      exit(0);      
    }
    options.println("Fg: 0");
    options.println("Bg: 1");
    FgColor = allColors[0];
    FgColor = allColors[1];
    options.close();    
  }
  options.close();
}

// draw centered text, doline is used to draw a line under text for page titles
void drawTitle(String title, int y, int doLine = 0, uint16_t tColor = FgColor){
  tft.setTextSize(2);
  tft.setTextColor(tColor);
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
  int centerX = (tft.width() - w) / 2;
  tft.setCursor(centerX, y);
  tft.println(title);
  if (doLine == 1) {
    tft.drawLine(0, 17, tft.width(), 17, FgColor);
  }
}

// save current treatment to rt.txt
void saveCurTreatment() { 
  // Create string containing formatted treatment data
  String newItem;
  switch (activeTreatment.strength) {
    case 0:
      newItem += "LOW  - ";
      break;
    case 1:
      newItem += "MID  - ";
      break;
    case 2:
      newItem += "HIGH - ";
      break;      
  }
  newItem += "0" + String(activeTreatment.hours) + ":";
  switch (activeTreatment.minutes) {
    case 0:
      newItem += "00";
      break;        
    case 1 ... 9:
      newItem += "0" + String(activeTreatment.minutes);
      break;
    default:
      newItem += String(activeTreatment.minutes);
      break;
  }
  // store string in file    
  recentTreatments = SD.open("rt.txt", FILE_WRITE);
  if (recentTreatments) {
    recentTreatments.println(newItem);
    recentTreatments.close();
  }
  else {
    Serial.println("Error: unable to open file [1]");
    delay(1000);
    exit(0);
  }
  recentTotNumItems++;
}

// update the current page items to recentPageItems from rt.txt
void updatePageItems(int pNum){ 
  recentCPNI = 0;
  uint32_t fLen;
  uint32_t sub = 0;
  char x;
  // open file and loop through all of the items in the current page, storing each item in the array
  recentTreatments = SD.open("rt.txt");
  if (recentTreatments){  
    fLen = recentTreatments.size();
    // Serial.print("fLen: ");
    // Serial.println(fLen);
    if (pNum*13*6 > fLen){
      Serial.println("Error: page out of bounds");       
      delay(500);
      return;
    }
    for (int k=0; k<6; k++){
      recentPageItems[k] = "";
    }
    while(recentCPNI<6) {
      sub = 14+(84*pNum)+(14*recentCPNI);      
      if (sub>fLen) break;
      recentTreatments.seek(fLen-sub);
      for (int k = 0; k<13; k++){
        x = recentTreatments.read();
        recentPageItems[recentCPNI] += x;
      }
      recentCPNI++;
    }
    recentTreatments.close();
  } else {
    Serial.println("Error: unable to open file [2]");
    delay(500);
    exit(0);    
  }
}

// takes in a 1 or 2 digit number and text format info and prints to tft
void printTime(uint8_t num, uint16_t x, uint16_t y, uint8_t textSize, int tColor, bool erase = false){ 
  tft.setCursor(x, y);
  tft.setTextSize(textSize);
  if (erase) {
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds("00", 0, 0, &x1, &y1, &w, &h);
    if (tColor == 0){
      tft.fillRect(x-3, y-3, w+3, h+3, BgColor);
    } else {
      tft.fillRect(x-3, y-3, w+3, h+3, FgColor);
    }
  }
  if (tColor == 0){
    tft.setTextColor(FgColor);
  } else {
    tft.setTextColor(BgColor);
  }
  char* t = malloc(2*sizeof(int));
  if (num<10){
    sprintf(t, "%i%i", 0, num);
  } else {
    sprintf(t, "%i", num);
  }
  tft.println(t);
  free(t);
}

// turn EM device on or off
void runControl(bool run){
  if (run == false){
    if (isRunning){
      digitalWrite(PIN_L, LOW);
      digitalWrite(PIN_M, LOW);
      digitalWrite(PIN_H, LOW);      
      digitalWrite(PIN_CTRL, HIGH);
      isRunning = false;
      if (currentPage == RUN) runBPlayPause();
    }
    return;
  } else {
    if (!isRunning){
      digitalWrite((PIN_L + activeTreatment.strength), HIGH);
      digitalWrite(PIN_CTRL, LOW);
      startTime = millis();
      isRunning = true;
    }
    return;
  }
}

// used in NEW page for selecting time
void timeSelect(int unit) {
  int maxNum;
  int curNum;
  if (unit == 0){  // minutes
    maxNum = 59;
    curNum = activeTreatment.minutes;
  } else {  // hours
    maxNum = 7;
    curNum = activeTreatment.hours;
  }
  int tsLastBState = 1;
  // get number
  while(1==1){
    int tsCurBState = digitalRead(PIN_IN3);
    if (tsLastBState == 0 && tsCurBState == 1) {  // end selection
      if (unit == 0){
        activeTreatment.minutes = curNum;
      } else if (unit == 1) {
        activeTreatment.hours = curNum;
      }
      activeTreatment.tot_tseconds = activeTreatment.minutes * 60 + activeTreatment.hours * 3600;
      break;
    }
    static int tsPos = 0;
    encoder->tick();
    int tsNewPos = (int)encoder->getPosition();
    if (tsPos != tsNewPos) {
      int tsDir = (int)encoder->getDirection();
      curNum += tsDir;      
      if (tsDir == 1) {  // cw turn
        curNum = curNum % (maxNum+1);
      } else if (curNum<0) {  // ccw turn
        curNum = maxNum;
      }
      tsPos = tsNewPos;
      if (unit == 0){
        printTime(curNum, 142, 172, 3, 1, true);       
      } else if (unit == 1) {
        printTime(curNum, 65, 172, 3, 1, true); 
      }
    }
    tsLastBState = tsCurBState;
  }
}

// used in OPTIONS page for selecting color
uint16_t colorSelect(uint8_t mode) {  // 0 -> Fg, 1 -> Bg
  int k;
  int curNum;
  for (k=0; k<11; k++) {
    if (mode == 0){
      if (FgColor == allColors[k]) curNum = k;
    } else {
      if (BgColor == allColors[k]) curNum = k;
    }
  }
  int csLastBState = 1;
  // get number
  while(1==1){
    int csCurBState = digitalRead(PIN_IN3);
    if (csLastBState == 0 && csCurBState == 1) {  // end selection
      Serial.println("end");
      break;
    }
    static int csPos = 0;
    encoder->tick();
    int csNewPos = (int)encoder->getPosition();
    if (csPos != csNewPos) {  // RE was turned
      int csDir = (int)encoder->getDirection();
      curNum += csDir;      
      if (csDir == 1) {  // cw turn
        curNum = curNum % (11);
        if ((mode == 0 && allColors[curNum] == BgColor) ||\
         (mode == 1 && allColors[curNum == FgColor])) { 
          curNum++;
          curNum = curNum % (11);
          if (mode == 0) {
            TmpFg = allColors[curNum];
            optionsBFg();
          } else {
            TmpBg = allColors[curNum];
            optionsBBg();
          }
          csPos = csNewPos;
          continue;
        }
      } else if (curNum<0) curNum = 10;  // ccw turn
      if (((mode == 0 && allColors[curNum] == BgColor) ||\
        (mode == 1 && allColors[curNum == FgColor])) && csDir==-1) { 
        curNum--;
        if (curNum<0) curNum = 10;
        if (mode == 0) {
          TmpFg = allColors[curNum];
          optionsBFg();
        } else {
          TmpBg = allColors[curNum];
          optionsBBg();
        }
        csPos = csNewPos;
        continue;
      }
      Serial.println(curNum);
      if (mode == 0) {
        TmpFg = allColors[curNum];
        optionsBFg();
      } else {
        TmpBg = allColors[curNum];
        optionsBBg();
      }
      csPos = csNewPos;
    }
    csLastBState = csCurBState;
  }
}

//**********************************************************************
// HOME PAGE COMPONENTS
//
void homeBTreatmentNew(bool setup = false){  // b0
  if (setup) {
    tft.setTextSize(2);
    tft.setTextColor(FgColor);
    tft.setCursor(47, 82-1);
    tft.println("New");
  }
  if (previousButton != currentButton){
    tft.fillRect(8, 82-9, 30, 30, BgColor);
  }  
  if (currentButton == 0) {
    tft.fillRect(8, 82-9, 30, 30, FgColor);
    tft.setTextSize(3);
    tft.setTextColor(BgColor);
    tft.setCursor(16, 82-5);
    tft.println(">");
  }
}

void homeBTreatmentRecent(bool setup = false){  // b1
  if (setup) {
    tft.setTextSize(2);
    tft.setTextColor(FgColor);
    tft.setCursor(47, 123-1);
    tft.println("Recent");
  }
  if (previousButton != currentButton){
    tft.fillRect(8, 123-9, 30, 30, BgColor);
  }  
  if (currentButton == 1) {
    tft.fillRect(8, 123-9, 30, 30, FgColor);
    tft.setTextSize(3);
    tft.setTextColor(BgColor);
    tft.setCursor(16, 123-5);
    tft.println(">");
  }
}

void homeBMoreOptions(bool setup = false){  // b2
  if (setup) {
    tft.setTextSize(2);
    tft.setTextColor(FgColor);
    tft.setCursor(47, 218-1);
    tft.println("Options");
  }
  if (previousButton != currentButton){
    tft.fillRect(8, 218-9, 30, 30, BgColor);
  }  
  if (currentButton == 2) {
    tft.fillRect(8, 218-9, 30, 30, FgColor);
    tft.setTextSize(3);
    tft.setTextColor(BgColor);
    tft.setCursor(16, 218-5);
    tft.println(">");
  }
}

void homeBMoreInfo(bool setup = false){  // b3
  if (setup) {
    tft.setTextSize(2);
    tft.setTextColor(FgColor);
    tft.setCursor(47, 259-1);
    tft.println("Info");
  }
  if (previousButton != currentButton){
    tft.fillRect(8, 259-9, 30, 30, BgColor);
  }  
  if (currentButton == 3) {
    tft.fillRect(8, 259-9, 30, 30, FgColor);
    tft.setTextSize(3);
    tft.setTextColor(BgColor);
    tft.setCursor(16, 259-5);
    tft.println(">");
  }
}

void homePageSetup(){  // Setup
  currentPage = HOME;
  previousButton = 0;
  currentButton = 0;
  tft.fillScreen(BgColor);
  drawTitle("Home", 0, 1);

  tft.setTextSize(3);
  tft.setTextColor(FgColor);
  tft.setCursor(4, 34);
  tft.println("Treatment");  
  tft.setCursor(4, 170);
  tft.println("More");  

  homeBTreatmentNew(true);
  homeBTreatmentRecent(true);
  homeBMoreInfo(true);
  homeBMoreOptions(true);
}

//*****************************************************************
// NEW TREATMENT PAGE COMPONENTS
//
void newBMode0(){  // b0
  if (previousButton != currentButton || erase){
    tft.fillRect(25, 70, 60, 40, BgColor);
  }
  if (currentButton != 0){
    tft.drawRect(25, 70, 60, 40, FgColor);
    if (activeTreatment.strength == 0) {
      tft.fillTriangle(26, 70, 26, 109, 41, 90, FgColor);
    }
    tft.setTextSize(3);
    tft.setCursor(48, 79);
    tft.setTextColor(FgColor);
    tft.println("L");
  } else {
    tft.fillRect(25, 70, 60, 40, FgColor);
    if (activeTreatment.strength == 0) {
      tft.fillTriangle(25, 70, 25, 109, 40, 90, BgColor);
    }
    tft.setTextSize(3);
    tft.setCursor(48, 79);
    tft.setTextColor(BgColor);
    tft.println("L");
  }
}

void newBMode1(){  // b1
  if (previousButton != currentButton || erase){
    tft.fillRect(90, 70, 60, 40, BgColor);
  }
  if (currentButton != 1){
    tft.drawRect(90, 70, 60, 40, FgColor);
    if (activeTreatment.strength == 1) {
      tft.fillTriangle(91, 70, 91, 109, 106, 90, FgColor);
    }
    tft.setTextSize(3);
    tft.setCursor(113, 79);
    tft.setTextColor(FgColor);
    tft.println("M");
  } else {
    tft.fillRect(90, 70, 60, 40, FgColor);
    if (activeTreatment.strength == 1) {
      tft.fillTriangle(90, 70, 90, 109, 105, 90, BgColor);
    }
    tft.setTextSize(3);
    tft.setCursor(113, 79);
    tft.setTextColor(BgColor);
    tft.println("M");
  }
}

void newBMode2(){  // b2
  if (previousButton != currentButton || erase){
    tft.fillRect(155, 70, 60, 40, BgColor);
  }
  if (currentButton != 2){
    tft.drawRect(155, 70, 60, 40, FgColor);
    if (activeTreatment.strength == 2) {
      tft.fillTriangle(156, 70, 156, 109, 171, 90, FgColor);
    }
    tft.setTextSize(3);
    tft.setCursor(178, 79);
    tft.setTextColor(FgColor);
    tft.println("H");
  } else {
    tft.fillRect(155, 70, 60, 40, FgColor);
    if (activeTreatment.strength == 2) {
      tft.fillTriangle(155, 70, 155, 109, 170, 90, BgColor);
    }
    tft.setTextSize(3);
    tft.setCursor(178, 79);
    tft.setTextColor(BgColor);
    tft.println("H");
  }
}

void newBTimeHours(){  // b3
  if (previousButton != currentButton){
    tft.fillRect(52, 163, 60, 40, BgColor);
  }
  if (currentButton != 3){
    tft.drawRect(52, 163, 60, 40, FgColor);
    printTime(activeTreatment.hours, 65, 172, 3, 0);
  } else {
    tft.fillRect(52, 163, 60, 40, FgColor);
    printTime(activeTreatment.hours, 65, 172, 3, 1);
  }
}

void newBTimeMins(){  // b4
  if (previousButton != currentButton){
    tft.fillRect(129, 163, 60, 40, BgColor);
  }
  if (currentButton != 4){
    tft.drawRect(129, 163, 60, 40, FgColor);
    printTime(activeTreatment.minutes, 142, 172, 3, 0);
  } else {
    tft.fillRect(129, 163, 60, 40, FgColor);
    printTime(activeTreatment.minutes, 142, 172, 3, 1);
  }
}

void newBContinue(){  // b5
  if (previousButton != currentButton){
    tft.fillRect(60, 235, 120, 40, BgColor);
  }
  if (currentButton != 5){
    tft.drawRect(60, 235, 120, 40, FgColor);
    tft.setCursor(72, 247);
    tft.setTextColor(FgColor);
    tft.setTextSize(2);
    tft.println("Continue");
  } else {
    tft.fillRect(60, 235, 120, 40, FgColor);
    tft.setCursor(72, 247);
    tft.setTextColor(BgColor);
    tft.setTextSize(2);
    tft.println("Continue");
  }
}

void newBReturn(){  // b6
  if (previousButton != currentButton){
    tft.fillRect(90, 284, 60, 20, BgColor);
  }
  int16_t x1, y1;
  uint16_t w, h;
  int centerX;
  if (currentButton != 6){
    tft.drawRect(90, 284, 60, 20, FgColor);
    tft.setTextColor(FgColor);
    tft.setTextSize(1);
    tft.getTextBounds("Return", 0, 0, &x1, &y1, &w, &h);
    centerX = (tft.width() - w) / 2;
    tft.setCursor(centerX, 290);
    tft.println("Return");
  } else{
    tft.fillRect(90, 284, 60, 20, FgColor);
    tft.setTextColor(BgColor);
    tft.setTextSize(1);
    tft.getTextBounds("Return", 0, 0, &x1, &y1, &w, &h);
    centerX = (tft.width() - w) / 2;
    tft.setCursor(centerX, 290);
    tft.println("Return");
  }
}

void newPageSetup(){  // Setup
  currentPage = NEW;
  previousButton = 0;
  currentButton = 0;
  tft.fillScreen(BgColor);
  // reset activeTreatment
  activeTreatment.minutes = 0;
  activeTreatment.hours = 0;
  activeTreatment.tot_tseconds = 0;
  activeTreatment.strength = 1;
  drawTitle("New Treatment", 0, 1);
  drawTitle("Power", 49);
  newBMode0();
  newBMode1();
  newBMode2();
  drawTitle("Time", 142);
  newBTimeHours();
  tft.setTextSize(5);
  tft.setTextColor(FgColor);
  tft.setCursor(tft.width()/2 - 12, 165);
  tft.println(":");
  newBTimeMins();
  newBContinue();
  newBReturn();
}

//*****************************************************************
// RECENT TREATMENTS PAGE COMPONENTS
//
void recentBTreatment(uint16_t num=0, bool setup=false){  // b0 - b5
  int y = 35 + 40*num;
  if (setup) { 
    drawTitle(recentPageItems[num], y);
  }
  if (previousButton != currentButton){
    tft.fillRect(8, y-9, 30, 30, BgColor);
  }  
  if (currentButton == num) {
    tft.fillRect(8, y-9, 30, 30, FgColor);
    tft.setTextSize(3);
    tft.setTextColor(BgColor);
    tft.setCursor(16, y-5);
    tft.println(">");
  }
}

void recentBLeft(){  // b6
  if (previousButton != currentButton){
    tft.fillRect(20, 269, 40, 30, BgColor);
  }
  if (recentCurPageNum <= 0) return;
  if (currentButton != recentCPNI){
    tft.drawRect(20, 269, 40, 30, FgColor);
    tft.fillTriangle(52, 273, 52, 294, 25, 283, FgColor);
  } else {
    tft.fillRect(20, 269, 40, 30, FgColor);
    tft.fillTriangle(52, 273, 52, 294, 25, 283, BgColor);
  }
}

void recentBReturn(){  // b7
  if (previousButton != currentButton){  // b0 - b5
    tft.fillRect(79, 269, 80, 30, BgColor);
  }
  int16_t x1, y1;
  uint16_t w, h;
  int centerX;
  if (currentButton != recentCPNI+1){
    tft.drawRect(79, 269, 80, 30, FgColor);
    tft.setTextSize(2);
    tft.setTextColor(FgColor);
    tft.getTextBounds("Return", 0, 0, &x1, &y1, &w, &h);
    centerX = (tft.width() - w) / 2;
    tft.setCursor(centerX, tft.height()-43);
    tft.println("Return");
  } else {
    tft.fillRect(79, 269, 80, 30, FgColor);
    tft.setTextSize(2);
    tft.setTextColor(BgColor);
    tft.getTextBounds("Return", 0, 0, &x1, &y1, &w, &h);
    centerX = (tft.width() - w) / 2;
    tft.setCursor(centerX, tft.height()-43);
    tft.println("Return");
  }
}

void recentBRight(){  // b8
  if (previousButton != currentButton){
    tft.fillRect(178, 269, 40, 30, BgColor);
  }
  if (currentButton != recentCPNI+2){
    tft.drawRect(178, 269, 40, 30, FgColor);
    tft.fillTriangle(185, 273, 185, 294, 212, 283, FgColor);
  } else {
    tft.fillRect(178, 269, 40, 30, FgColor);
    tft.fillTriangle(185, 273, 185, 294, 212, 283, BgColor);
  }
}

void recentPageSetup(){  
  currentPage = RECENT;
  // calculate total number of pages
  recentTotNumPages = recentTotNumItems / 6;
  if (recentTotNumItems % 6 != 0) recentTotNumPages++;  
  previousButton = 0;
  currentButton = 0;
  recentCurPageNum = 0;
  tft.fillScreen(BgColor);
  drawTitle("Recent Treatments", 0, 1);
  recentBReturn();
  updatePageItems(recentCurPageNum);
  if (recentCPNI == 6) {  
    recentBRight();
  }
  for (int i=0; i<recentCPNI; i++){ 
    recentBTreatment(i, true);
  }
}

//*********************************************************************
// OPTIONS PAGE COMPONENTS
//
void optionsBFg(){  // b0  
  if (previousButton != currentButton) {
      tft.drawRect(80, 27, 74, 24, BgColor);    
  }
  tft.fillRect(82, 29, 70, 20, TmpFg);
  tft.drawRect(82, 29, 70, 20, FgColor);
  if (currentButton == 0) {
    tft.drawRect(80, 27, 74, 24, FgColor);
  }  
}

void optionsBBg(){  // b1
  if (previousButton != currentButton) {
    tft.drawRect(159, 27, 74, 24, BgColor);   
  }
  tft.fillRect(161, 29, 70, 20, TmpBg);
  tft.drawRect(161, 29, 70, 20, FgColor);
  if (currentButton == 1) {  
    tft.drawRect(159, 27, 74, 24, FgColor);
  }
}

void optionsBApply(){  // b2
  if (previousButton != currentButton) {
    tft.fillRect(80, 226, 79, 34, BgColor);   
  }
  if (currentButton != 2) { 
    tft.drawRect(80, 226, 79, 34, FgColor); 
    drawTitle("Apply", 235);
  } else {
    tft.fillRect(80, 226, 79, 34, FgColor);
    drawTitle("Apply", 235, 0, BgColor);
  }
}

void optionsBReturn(){  // b3
  if (previousButton != currentButton) {
    tft.fillRect(70, 270, 99, 34, BgColor);   
  }
  if (currentButton != 3) {
    tft.drawRect(70, 270, 99, 34, FgColor);
    drawTitle("Return", 280);
  } else {
    tft.fillRect(70, 270, 99, 34, FgColor);
    drawTitle("Return", 280, 0, BgColor);
  }
}

void optionsPageSetup(){
  currentPage = OPTIONS;
  previousButton = 0;
  currentButton = 0;
  TmpFg = FgColor;
  TmpBg = BgColor;
  tft.fillScreen(BgColor);
  drawTitle("Options", 0, 1);
  tft.setTextSize(2);
  tft.setTextColor(FgColor);
  tft.setCursor(1, 30);
  tft.println("Colors");  
  optionsBFg(); 
  optionsBBg();
  optionsBApply();
  optionsBReturn();
}

//*********************************************************************
// INFO PAGE COMPONENTS
//
void infoBReturn() {
  int16_t x1, y1;
  uint16_t w, h;
  int centerX;
  tft.fillRect(79, 269, 80, 30, FgColor);
  tft.setTextSize(2);
  tft.setTextColor(BgColor);
  tft.getTextBounds("Return", 0, 0, &x1, &y1, &w, &h);
  centerX = (tft.width() - w) / 2;
  tft.setCursor(centerX, tft.height()-43);
  tft.println("Return");
}

void infoPageSetup(){
  currentPage = INFO;
  previousButton = 0;
  currentButton = 0;
  tft.fillScreen(BgColor);
  
  // get cid
  tft.setCursor(10, 70);
  Sd2Card card;
  cid_t cid;
  if(!card.init()) {
    Serial.println("error initializing card"); 
    exit(0);
  }
  card.readCID(&cid);

  // get file size
  uint32_t fSize = 0;
  recentTreatments = SD.open("rt.txt");
  fSize = recentTreatments.size();
  recentTreatments.close();

  // display 
  drawTitle("Info", 0, 1);
  tft.setTextSize(2);
  tft.setTextColor(FgColor);  
  // card ID
  tft.setCursor(1, 30);
  tft.print("CID: [");
  tft.print(cid.mid);
  tft.print(cid.oid[0]);
  tft.print(cid.oid[1]);
  tft.print(cid.pnm[0]);
  tft.print(cid.pnm[1]);
  tft.print(cid.pnm[2]);
  tft.print(cid.pnm[3]);
  tft.print(cid.pnm[4]);
  tft.print(cid.prv_m);
  tft.print(cid.prv_n);
  tft.print(cid.psn);
  tft.print(cid.mdt_year_high);
  tft.print(cid.reserved);
  tft.print(cid.mdt_month);
  tft.print(cid.mdt_year_low);
  tft.print(cid.always1);
  tft.print(cid.crc);
  tft.print("]");
  // card storage info 
  tft.setCursor(1, 100);
  tft.print((double)(fSize*1.0d / MAX32*1.0d)*100);
  tft.println("% full");
  // company info
  tft.setCursor(1, 150);
  tft.println("Product name v1.0");
  tft.println("Company Name");
  tft.println("copyright information");
  infoBReturn();
}

//*********************************************************************
// RUN PAGE COMPONENTS
//
void runBPlayPause(){  // b0
  if (previousButton != currentButton || erase){
    tft.fillRect(20, 192, 50, 45, BgColor);
  }
  if (currentButton != 0){
    tft.drawRect(20, 192, 50, 45, FgColor);
    if (!isRunning){
      tft.fillTriangle(32, 199, 32, 229, 62, 214, FgColor);
      int triXCenter = 50 + (90-50)/2;
    } else{
      tft.fillRect(45 - 13, 199, 8, 30, FgColor);
      tft.fillRect(45 + 4, 199, 8, 30, FgColor);
    }
  } else {
    tft.fillRect(20, 192, 50, 45, FgColor);
    if (!isRunning){
      tft.fillTriangle(32, 199, 32, 229, 62, 214, BgColor);
      int triXCenter = 50 + (90-50)/2;
    } else{
      tft.fillRect(45 - 13, 199, 8, 30, BgColor);
      tft.fillRect(45 + 4, 199, 8, 30, BgColor);
    }
  }
}

void runBRestart(){  // b1
  if (previousButton != currentButton){
    tft.fillRect(80, 192, 140, 45, BgColor);
  }
  if (currentButton != 1){
    tft.drawRect(80, 192, 140, 45, FgColor);
    tft.setTextSize(3);
    tft.setTextColor(FgColor);
    tft.setCursor(80 + 8, 195 + 10);
    tft.println("Restart");
  } else {
    tft.fillRect(80, 192, 140, 45, FgColor);
    tft.setTextSize(3);
    tft.setTextColor(BgColor);
    tft.setCursor(80 + 8, 195 + 10);
    tft.println("Restart");
  }
}

void runBEnd(){  // b2
  if (previousButton != currentButton){
    tft.fillRect(80, 257, 80, 45, BgColor);
  }
  int16_t x1, y1;
  uint16_t w, h;
  int centerX;
  if (currentButton != 2){
    tft.drawRect(80, 257, 80, 45, FgColor);
    tft.setTextSize(3);
    tft.setTextColor(FgColor);
    tft.getTextBounds("End", 0, 0, &x1, &y1, &w, &h);
    centerX = (tft.width() - w) / 2;
    tft.setCursor(centerX, 270);
    tft.println("End");
  } else {
    tft.fillRect(80, 257, 80, 45, FgColor);
    tft.setTextSize(3);
    tft.setTextColor(BgColor);
    tft.getTextBounds("End", 0, 0, &x1, &y1, &w, &h);
    centerX = (tft.width() - w) / 2;
    tft.setCursor(centerX, 270);
    tft.println("End");
  }
}

void runPageSetup(){  // Setup
  currentPage = RUN;
  activeTreatment.cur_tseconds = activeTreatment.tot_tseconds;
  previousButton = 0;
  currentButton = 0;
  tft.fillScreen(BgColor);
  drawTitle("Treatment", 0, 1);
  // Mode
  tft.setTextSize(5); 
  tft.setTextColor(FgColor);
  char* st = malloc(10*sizeof(char));
  switch (activeTreatment.strength){
    case 0:
      sprintf(st, "LOW");
      break;
    case 1:
      sprintf(st, "MID");
      break;
    case 2:
      sprintf(st, "HIGH");
      break;
  }
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(st, 0, 0, &x1, &y1, &w, &h);
  int centerX = (tft.width() - w) / 2;
  tft.setCursor(centerX, 40);
  tft.println(st);
  free(st);
  // Colons
  tft.setCursor(75,117);
  tft.println(":");
  tft.setCursor(136,117);
  tft.println(":");
  // Dynamic Output and Buttons
  uint32_t s = activeTreatment.tot_tseconds;
  printTime(s / 3600, 35, 120, 4, 0, true);               // hours
  printTime((s % 3600) / 60, 96, 120, 4, 0, true);        // minutes
  printTime(s % 60, 157, 120, 4, 0, true);                // seconds
  runBPlayPause();
  runBRestart();
  runBEnd();
}
//*******************************************************************

// This is logic for when the rotary encoder is turned. It is split by direction -> page.
// All this function does is draw and redraw buttons and make sure currentButton is within
// the correct bounds
void RETurned(){  // rotary encoder turned (Button drawing and updating current and previous button)
  int dir = (int)encoder->getDirection();
  currentButton += dir;
  if (dir == 1) {  // clockwise turn
    switch (currentPage) {
      case HOME:
        currentButton = currentButton % 4;
        homeButtons[currentButton]();
        homeButtons[previousButton]();
        break;
      case NEW:
        currentButton = currentButton % 7;
        newButtons[currentButton]();
        newButtons[previousButton]();
        break;
      case RECENT:
        if (recentCurPageNum == 0 && currentButton == recentCPNI) {  // if on first page and on left button, skip
          currentButton++;
        }
        if (recentCurPageNum == recentTotNumPages-1 && previousButton == 8) { // keep current button within bounds on last page
          currentButton = 0;
          recentBTreatment(currentButton);
          break;
        } else if (recentCurPageNum == recentTotNumPages-1 && currentButton == recentCPNI+2){  // if on last bage and on right button, skip
          currentButton++;
        }
        currentButton = currentButton % (3 + recentCPNI);
        if (currentButton < recentCPNI) {
          recentBTreatment(currentButton);
        } else {
          recentButtons[currentButton - recentCPNI]();
        }
        if (previousButton < recentCPNI) {
          recentBTreatment(previousButton);
        } else {
          recentButtons[previousButton - recentCPNI]();
        } 
        break;
      case OPTIONS:
        currentButton = currentButton % 4;
        optionsButtons[currentButton]();
        optionsButtons[previousButton]();
        break;     
      case RUN:
        currentButton = currentButton % 3;
        runButtons[currentButton]();
        runButtons[previousButton]();
        break;
    }
  } else {  // counter-clockwise turn
    switch (currentPage) {
      case HOME:
        if (currentButton < 0) {
          currentButton = 3;
        }
        homeButtons[currentButton]();
        homeButtons[previousButton]();
        break;
      case NEW:
        if (currentButton < 0) {
          currentButton = 6;
        }
        newButtons[currentButton]();
        newButtons[previousButton]();
        break;
      case RECENT:  // issue
        if (recentCurPageNum == 0 && currentButton == recentCPNI) {  // if on first page and on left button, skip
          currentButton--;
        }
        if (currentButton < 0) {
          currentButton = (3 + recentCPNI) - 1;
        }
        if (recentCurPageNum == recentTotNumPages-1 && previousButton == 8) {  // keep current button within bounds on last page
          currentButton = recentCPNI+1;
          recentButtons[currentButton - recentCPNI]();
          break;
        } else if (recentCurPageNum == recentTotNumPages-1 && currentButton == recentCPNI+2) {  // if on last bage and on right button, skip
          currentButton--;
        }
        if (currentButton < recentCPNI) {
          recentBTreatment(currentButton);
        } else {
          recentButtons[currentButton - recentCPNI]();
        }
        if (previousButton < recentCPNI) {
          recentBTreatment(previousButton);
        } else {
          recentButtons[previousButton - recentCPNI]();
        } 
        break;
      case OPTIONS:
        if (currentButton < 0) {
          currentButton = 3;
        }
        optionsButtons[currentButton]();
        optionsButtons[previousButton]();
        break;
      case RUN:
        if (currentButton < 0) {
          currentButton = 2;
        }
        runButtons[currentButton]();
        runButtons[previousButton]();
        break;
    }
  }
  Serial.println(currentButton);
  previousButton = currentButton;
}

// This is what happens when a button is pressed. it is split by page -> current button.
void REPressed(){  // rotary encoder pressed (Button actions)
  int i, j, k;
  uint32_t temp1;
  uint32_t temp2;
  String fIn = "";
  switch (currentPage) {
    case HOME:
      switch (currentButton) {
        case 0:  // New Treatment
          newPageSetup();
          break;
        case 1:  // Recent Treatments
          recentPageSetup();
          break;
        case 2:  // Options
          optionsPageSetup();          
          break;
        case 3:  // Info
          infoPageSetup();          
          break;
      }
      break;
    case NEW:
      switch (currentButton) {
        case 0:  // mode 0
          j = activeTreatment.strength-0;
          activeTreatment.strength = 0;
          newBMode0();   
          erase = true;
          newButtons[j]();  
          erase = false; 
          break;
        case 1:  // mode 1
          j = activeTreatment.strength-0;
          activeTreatment.strength = 1;
          newBMode1();   
          erase = true;
          newButtons[j]();  
          erase = false;       
          break;
        case 2:  // mode 2
          j = activeTreatment.strength-0;
          activeTreatment.strength = 2;
          newBMode2();
          erase = true;
          newButtons[j]();  
          erase = false; 
          break;
        case 3:  // hour select
          timeSelect(1);
          break;
        case 4:  // minute select
          timeSelect(0);
          break;
        case 5:  // continue
          if (activeTreatment.tot_tseconds == 0){
            activeTreatment.tot_tseconds = 1200;
            activeTreatment.minutes = 20;
          }
          saveCurTreatment();
          runPageSetup();
          break;
        case 6:  // return
          homePageSetup();
          break;
      }
      break;
    case RECENT:
      if (currentButton<recentCPNI) {  // recent treatments
        switch (int(recentPageItems[currentButton].charAt(0))) {
          case int('L'):
            activeTreatment.strength = 0;
            break;
          case int('M'):
            activeTreatment.strength = 1;
            break;
          case int('H'):
            activeTreatment.strength = 2;
            break;
        }
        temp1 = (int(recentPageItems[currentButton][7]-'0')*10 + (int(recentPageItems[currentButton][8]-'0')));
        temp2 = (int(recentPageItems[currentButton][10]-'0')*10 + (int(recentPageItems[currentButton][11]-'0')));
        activeTreatment.hours = temp1;
        activeTreatment.minutes = temp2;
        activeTreatment.tot_tseconds = temp1*3600 + temp2*60;
        runPageSetup();
      } else if (currentButton==recentCPNI && recentCurPageNum > 0) {  // left button
        if (recentCurPageNum == 1) tft.fillRect(20, 269, 40, 30, BgColor);
        if (recentCurPageNum == recentTotNumPages-1) {
          recentBRight();
          currentButton = 6;
          
        }
        tft.fillRect(0, 35, tft.width(), 230, BgColor);
        recentCurPageNum--;
        updatePageItems(recentCurPageNum);
        for (i=0; i<recentCPNI; i++){ 
          recentBTreatment(i, true);
        }
      } else if (currentButton==recentCPNI+1) {  // return button
        homePageSetup();
      } else if (currentButton==recentCPNI+2) {      
        recentCurPageNum++;
        if (recentCurPageNum == recentTotNumPages-1) {  // right button
          tft.fillRect(178, 269, 40, 30, BgColor);
        } else if (recentCurPageNum == recentTotNumPages){
          recentCurPageNum--;
          break;
        }
        if (recentCurPageNum == 1) {
          recentBLeft();
        } 
        updatePageItems(recentCurPageNum);
        tft.fillRect(0, 35, tft.width(), 230, BgColor);        
        for (i=0; i<recentCPNI; i++){ 
          recentBTreatment(i, true);
        }
      }
      break;
    case OPTIONS:
      switch (currentButton) {
        case 0:  // Fg Color
          // trigger color picker
          colorSelect(0);
          break;
        case 1:  // Bg Color
          // trigger color picker
          colorSelect(1);
          break;
        case 2:  // Apply Changes
          // set colors and save to file
          FgColor = TmpFg;
          BgColor = TmpBg;
          for (k=0;k<11;k++){
            if (FgColor == allColors[k]) fIn += "Fg: " + String(k) + "\n";
            if (BgColor == allColors[k]) fIn += "Bg: " + String(k);            
          }
          SD.remove("options.txt");
          options = SD.open("options.txt", FILE_WRITE);
          options.println(fIn);
          options.close();
          optionsPageSetup();
          break;  
        case 3:  // Return
          Serial.println("Hello!");
          homePageSetup();
          break;      
      }
      break;
    case INFO:
      homePageSetup();
      break;
    case RUN:
      switch (currentButton) {
        case 0:  // play & pause
          if (isRunning){
            runControl(false);
            runBPlayPause();
          } else {
            runControl(true);
            runBPlayPause();
          }
          break;
        case 1:  // restart
          runControl(false);
          activeTreatment.cur_tseconds = activeTreatment.tot_tseconds;
          printTime(activeTreatment.tot_tseconds / 3600, 35, 120, 4, 0, true);             // hours
          printTime((activeTreatment.tot_tseconds % 3600) / 60, 96, 120, 4, 0, true);      // minutes
          printTime(activeTreatment.tot_tseconds % 60, 157, 120, 4, 0, true);              // seconds
          erase = true;
          runBPlayPause();
          erase = false;
          break;
        case 2:  // return
          runControl(false);
          homePageSetup();        
          break;
      }
      break;
  }
}

