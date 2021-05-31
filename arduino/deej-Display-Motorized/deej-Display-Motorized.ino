#include "arduino.h"
#include <avr/wdt.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AFMotor.h>

//Microcontroller type
//#define MCU32U4 1
#define MCUA328P 1

//You must Hard Code in the number of Sliders in
#define NUM_SLIDERS 2
#define SERIALSPEED 115200
#define FrequencyMS 10
#define SerialTimeout 5000 //This is two seconds
#define NUM_MOTORS 2

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const unsigned char icon [] PROGMEM = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xfb, 0xfe, 0x7f, 0xdf, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xfb, 0xfe, 0x7f, 0xdf, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xfb, 0xfc, 0x3f, 0xdf, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfb, 0xf9, 0x8f, 0xdf, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfb, 0xf3, 0xef, 0xdf, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xfb, 0xe7, 0xe7, 0xdf, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xfb, 0xe7, 0xf7, 0xdf, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xfb, 0xe7, 0xf7, 0xdf, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xfb, 0xe7, 0xe7, 0xdf, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xfb, 0xf3, 0xcf, 0xdf, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xfb, 0xf8, 0x0f, 0xdf, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xfb, 0xfc, 0x3f, 0xdf, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xfb, 0xfe, 0x7f, 0xdf, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xfb, 0xfe, 0x7f, 0xcf, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xfb, 0xfe, 0x7f, 0x03, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xfb, 0xfe, 0x7e, 0x79, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xfb, 0xfe, 0x7c, 0xfd, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xfb, 0xfe, 0x7d, 0xfc, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xe1, 0xfe, 0x7d, 0xfc, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xc0, 0x7e, 0x7d, 0xfc, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0x9f, 0x3e, 0x7c, 0xf9, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xbf, 0x3e, 0x7e, 0x71, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0x3f, 0xbe, 0x7f, 0x07, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0x3f, 0xbe, 0x7f, 0xdf, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xbf, 0xbe, 0x7f, 0xdf, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0x9f, 0x3e, 0x7f, 0xdf, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xcc, 0x7e, 0x7f, 0xdf, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xe0, 0xfe, 0x7f, 0xdf, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xfb, 0xfe, 0x7f, 0xdf, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xfb, 0xfe, 0x7f, 0xdf, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const uint8_t analogInputs[NUM_SLIDERS] = {A10, A11};//, A7, A8, A9};

uint16_t analogSliderValues[NUM_SLIDERS];
uint16_t volumeValues[NUM_SLIDERS];
String groupNames[NUM_SLIDERS];
uint8_t motorMoved[NUM_MOTORS];
//bool firstReceive = true;

//this is what motor has what analog input
const uint8_t motorMap[NUM_MOTORS] = {A11, A10};
const uint8_t touchInputs[NUM_MOTORS] = {30,31};
unsigned long touchTimes[NUM_MOTORS];
bool touch[NUM_MOTORS];
AF_DCMotor motors[NUM_MOTORS] = {AF_DCMotor(4), AF_DCMotor(2)};

// Constend Send
bool pushSliderValuesToPC = false;
bool receivednewvalues = false;

String names;

void setup() { 

  if(!Serial){
    Serial.end();
  }
  
  Serial.begin(SERIALSPEED);
  Serial.println("INITBEGIN");
  
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    pinMode(analogInputs[i], INPUT);

    names += "X";
    if(i<NUM_SLIDERS-1){
      names += "|";
    }

    volumeValues[i] = 0; 
    analogSliderValues[i] = 0;
    
  }
  for(int i = 0; i<NUM_MOTORS; i++){
    touch[i] = true;
  }
  updateSliderValues();
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    //Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.drawBitmap(0, 0, icon, 128, 64, WHITE);
  display.display();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  delay(1000);
  showOnDisplay();

  for(int i = 0; i<NUM_MOTORS; i++){
    //motorMoved[i] = 50;
    int pin = motorMap[i];
    int returnval = 0;
    for(int j = 0; j<NUM_SLIDERS; j++){
      if(analogInputs[j] == pin){
        returnval = analogSliderValues[j];
      }
    }
    AF_DCMotor motor = motors[i];
    motor.setSpeed(200);
    moveSliderTo(512, pin, motor);
    //delay(100);
    moveSliderTo(0, pin, motor);
    delay(100);
    moveSliderTo(1023, pin, motor);
    delay(100);
    moveSliderTo(0, pin, motor);
    /*moveSliderTo(returnval, pin, motor);
    delay(100);*/
    //Serial.println(String(returnval) + " " + String(analogRead(pin)));
  }
  /*for(;;){
    for(int p = 0; p<=1023; p+=100){
      for(int i = 0; i<NUM_MOTORS; i++){
        moveSliderTo(p, motorMap[i], motors[i]);
        delay(1000);
      }
    }
  }*/
  //firstReceive = false;
  //pushSliderValuesToPC=true;
  Serial.println("INITDONE");
  //Serial.println("");
}

void loop() {
  checkForTouch();
  
  checkForCommand();
  
  updateSliderValues();

  //Check for data chanel to be open
  if(pushSliderValuesToPC) {
    sendSliderValues(); // Actually send data
  } 
  
  // printSliderValues(); // For debug
  delay(FrequencyMS);
}

void reboot() {
#if MCU32U4
  wdt_disable();
  wdt_enable(WDTO_30MS);
  while (1) {}
#elif MCUA328P
  asm volatile ("  jmp 0");  
#endif
}

void checkForTouch(){
  for(int i = 0; i<NUM_MOTORS; i++){
    if(digitalRead(touchInputs[i]) == 0){
      touchTimes[i] = millis();
      touch[i] = true;
    } else {
      if(millis() - touchTimes[i] > 500){
        touch[i] = false;
      }
    }
  }
}

void showOnDisplay() {
  String dsp = "";
  for(uint8_t i = 0; i< NUM_SLIDERS; i++){
    float vol = ((float)volumeValues[i])/(1023.0)*100.0;
    if(vol != 0){
      dsp += round(vol);
    } else {
      dsp += "M";
    }
    if( i < NUM_SLIDERS - 1){
      dsp += "|";
    }
  }
  display.clearDisplay();
  display.setCursor(0,0);
  display.println(names);
  display.println(dsp);
  display.display();
}

void moveMotor(int i){
  checkForTouch();
  if(!touch[1]){
    AF_DCMotor motor = motors[i];    
    int pin = motorMap[i];
    for(int j = 0; j<NUM_SLIDERS; j++){
      if(analogInputs[j] == pin){
          moveSliderTo(volumeValues[j], pin, motor);
          break;
      }
    }
    motorMoved[i]++;
  }
}

void updateSliderValues() {
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    bool motor = false;
    int motor_num = -1;
    for(int j = 0; j<NUM_MOTORS; j++){
      if(motorMap[j] == analogInputs[i]){
        motor = true;
        motor_num = j;
        break;
      }
    }
    if(!motor){
     analogSliderValues[i] = analogRead(analogInputs[i]);
    } else {
      if(touch[motor_num]){
        analogSliderValues[i] = analogRead(analogInputs[i]);
      } else {
        if(motorMoved[i] >= 2){
          analogSliderValues[i] = volumeValues[i];
          motorMoved[i] = 0;
        }
      }
    }
  }
  //FOR TESTING:
  //memcpy(analogSliderValues, volumeValues, sizeof(analogSliderValues));
}

void sendSliderValues() {
  String sendvals = "";
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    sendvals += analogSliderValues[i];
    if (i < NUM_SLIDERS - 1) {
      sendvals += "|";
    }
  }
  
  Serial.println(sendvals);
}

void printSliderValues() {
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    Serial.print("Slider #"+ String(i + 1) + ": " + String(analogSliderValues[i]) + " mV");

    if (i < NUM_SLIDERS - 1) {
      Serial.print(" | ");
    } else {
      Serial.println();
    }
  }
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    Serial.print(volumeValues[i]);

    if (i < NUM_SLIDERS - 1) {
      Serial.print(" | ");
    } else {
      Serial.println(" END ");
    }
  }
}

String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void checkForCommand() {
  //Check if data is waiting
  
  if (Serial.available() > 0) {
    
    //Get start time of command
    unsigned long timeStart = millis();

    //Get data from Serial
    
    String input = Serial.readStringUntil('\r');  // Read chars from serial monitor
    Serial.readStringUntil('\n');
    //If data takes to long
    if(millis()-timeStart >= SerialTimeout) {
      Serial.println("TIMEOUT");
      return;
    }
    // Check and match commands
    else {
      // Start Sending Slider Values
      if ( input.equalsIgnoreCase("deej.core.start") == true ) {
        pushSliderValuesToPC = true;
        return;
      }

      // Stop Sending Slider Values
      else if ( input.equalsIgnoreCase("deej.core.stop") == true ) {
        pushSliderValuesToPC = false;
        return;
      }
      
      // Send Single Slider Values
      else if ( input.equalsIgnoreCase("deej.core.values") == true ) {
        sendSliderValues();
        return;
      }

      // Send Human Readable Slider Values 
      else if ( input.equalsIgnoreCase("deej.core.values.HR") == true ) {
        printSliderValues();
        return;
      }

      // Receive Values
      else if(input.equalsIgnoreCase("deej.core.receive") == true){
        String receive = Serial.readStringUntil('\r');
        Serial.readStringUntil('\n');
        if(receive.length() > (4*NUM_SLIDERS+(NUM_SLIDERS-1)) || receive.length() < (1*NUM_SLIDERS+(NUM_SLIDERS-1))){
          Serial.println("INVALID DATA: " + receive);
          return;
        }
        int saveVals[NUM_SLIDERS];
        memcpy(saveVals, volumeValues, NUM_SLIDERS);
        String str = getValue(receive, '|', 0);
        for(int i = 1; str != ""; i++){
          volumeValues[i-1] = str.toInt();
          str = getValue(receive, '|', i);
        }
        showOnDisplay();
        for(int i = 0; i<NUM_MOTORS; i++){
          for(int j = 0; j<NUM_SLIDERS; j++){
            if(analogInputs[j] == motorMap[i]){
              if(abs(volumeValues[j]-analogSliderValues[j]) > 1){
                moveMotor(i);
                //motorMoved[i] = 100;
                break;
              }
            }
          }
        }
        /*if(!firstReceive){
          firstReceive = true;
        }*/
        Serial.println(receive);
        return;
      }

      // Receive Group Names
      else if(input.equalsIgnoreCase("deej.core.receive.groupnames")){
        String receive = Serial.readStringUntil('\r');
        Serial.readStringUntil('\n');
        String str = getValue(receive, '|', 0);
        for(int i = 1; i <= NUM_SLIDERS; i++){
          groupNames[i-1] = str;
          str = getValue(receive, '|', i);
        }
        names = "";
        for(int i = 0; i<NUM_SLIDERS; i++){
          str = groupNames[i];
          if(str != ""){
            names += str;
          } else {
            names += "X";
          }
          if(i<NUM_SLIDERS-1){
            names += "|";
          }
        }
        Serial.println(receive);
        return;
      }
      
      else if ( input.equalsIgnoreCase("deej.core.reboot") == true ) {
        reboot();
        return;
      }

      //Default Catch all
      else {
        Serial.println("INVALIDCOMMANDS: " + input);
        return;
      }
    }
  }
}

void moveSliderTo(int value, int slider, AF_DCMotor motor){
  int current = analogRead(slider);
  //Serial.println("Start " + String(slider) + " " + String(current) + " " + String(value));
  int dir = 0;
  if(value > 1023 || value < 0 || abs(current-value)<=2){
    return;
  }
  if(value > current){
    motor.run(FORWARD);
    dir = 1;
  } else if(value < current){
    motor.run(BACKWARD);
    dir = 2;
  } else {
    return;
  }
  while(abs(current-value) > 30){
    current = analogRead(slider);
    if(dir == 1 && value < current){
      break;
    } else if(dir == 2 && value > current){
      break;
    }
  }
  motor.run(RELEASE);
  motor.setSpeed(125);
  delay(10);
  current = analogRead(slider);
  if(value > current){
    motor.run(FORWARD);
    dir = 1;
  } else if(value < current){
    motor.run(BACKWARD);
    dir = 2;
  } else {
    motor.setSpeed(200);
    return;
  }
  while(abs(current-value) > 1){
    current = analogRead(slider);
    if(dir == 1 && value < current){
      break;
    } else if(dir == 2 && value > current){
      break;
    }
    if(abs(value-current) > 31){
      motor.run(RELEASE);
      delay(10);
      motor.setSpeed(200);
      moveSliderTo(value, slider, motor);
      break;
    }
  }
  motor.run(RELEASE);
  motor.setSpeed(200);
  //Serial.println("End " + String(slider) + " " + String(current) + " " + String(value));
}