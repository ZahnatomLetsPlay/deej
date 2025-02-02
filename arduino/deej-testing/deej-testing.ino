#include "arduino.h"
#include <avr/wdt.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AFMotor.h>
#include <PID_v1.h>
#include <limits.h>

//Microcontroller type
//#define MCU32U4 1
#define MCUA328P 1
#define ULONG_MAX (LONG_MAX * 2UL + 1UL)

//You must Hard Code in the number of Sliders in
#define NUM_SLIDERS 5
#define SERIALSPEED 115200
#define FrequencyMS 10
#define SerialTimeout 5000 //This is two seconds
#define NUM_MOTORS 2
#define NUM_MUTES  1
#define DEBOUNCE_TIME 200

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const unsigned char icon [] PROGMEM = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xfb, 0xfe, 0x7f, 0xdf, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xfb, 0xfe, 0x7f, 0xdf, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xfb, 0xfc, 0x3f, 0xdf, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfb, 0xf9, 0x8f, 0xdf, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfb, 0xf3, 0xef, 0xdf, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xfb, 0xe7, 0xe7, 0xdf, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xfb, 0xe7, 0xf7, 0xdf, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xfb, 0xe7, 0xf7, 0xdf, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xfb, 0xe7, 0xe7, 0xdf, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xfb, 0xf3, 0xcf, 0xdf, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xfb, 0xf8, 0x0f, 0xdf, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xfb, 0xfc, 0x3f, 0xdf, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xfb, 0xfe, 0x7f, 0xdf, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xfb, 0xfe, 0x7f, 0xcf, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xfb, 0xfe, 0x7f, 0x03, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xfb, 0xfe, 0x7e, 0x79, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xfb, 0xfe, 0x7c, 0xfd, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xfb, 0xfe, 0x7d, 0xfc, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xe1, 0xfe, 0x7d, 0xfc, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xc0, 0x7e, 0x7d, 0xfc, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0x9f, 0x3e, 0x7c, 0xf9, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xbf, 0x3e, 0x7e, 0x71, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0x3f, 0xbe, 0x7f, 0x07, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0x3f, 0xbe, 0x7f, 0xdf, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xbf, 0xbe, 0x7f, 0xdf, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0x9f, 0x3e, 0x7f, 0xdf, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xcc, 0x7e, 0x7f, 0xdf, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xe0, 0xfe, 0x7f, 0xdf, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xfb, 0xfe, 0x7f, 0xdf, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xfb, 0xfe, 0x7f, 0xdf, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

bool makeLogarithmic = false;
const uint8_t analogInputs[NUM_SLIDERS] = {A10, A11, A7, A8, A9};
//Mute index corresponds to analoginput index
//If you want to leave an input without mute, enter -1
const uint8_t muteInputs[NUM_MUTES] = {50};

uint16_t analogSliderValues[NUM_SLIDERS];
uint16_t volumeValues[NUM_SLIDERS];
String groupNames[NUM_SLIDERS];
uint8_t motorMoved[NUM_MOTORS];
uint8_t sliderMuted[NUM_MUTES];
int buttonState[NUM_MUTES];
bool mute[NUM_MUTES];
uint16_t muteValues[NUM_MUTES];
unsigned long muteTimes[NUM_MUTES];
bool pause = true;

//this is what motor has what analog input
const uint8_t motorMap[NUM_MOTORS] = {A10, A11};
const uint8_t touchInputs[NUM_MOTORS] = {30, 31};
unsigned long touchTimes[NUM_MOTORS];
bool touch[NUM_MOTORS];
AF_DCMotor motors[NUM_MOTORS] = {AF_DCMotor(2), AF_DCMotor(4)};

// Constend Send
bool pushSliderValuesToPC = false;
bool receivednewvalues = false;
unsigned long lastcmd;
bool firstcmd = false;
bool lastcmdrequest = true;

String names;
String receiveline = "";
String sendline = "";

//Sets up pinmodes, motors; initializes buttons, display, etc, etc
void setup() {
  if (!Serial) {
    Serial.end();
  }

  Serial.begin(SERIALSPEED);
  Serial.println("INITBEGIN");

  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    pinMode(analogInputs[i], INPUT);

    names += String(i + 1);
    if (i < NUM_SLIDERS - 1) {
      names += "|";
    }
    if (i < NUM_MUTES) {
      if (muteInputs[i] != -1) {
        pinMode(muteInputs[i], INPUT);
        mute[i] = false;
        buttonState[i] = LOW;
        muteTimes[i] = 0;
      }
    }
    volumeValues[i] = 0;
    analogSliderValues[i] = 0;
  }
  for (int i = 0; i < NUM_MOTORS; i++) {
    touch[i] = true;
  }
  updateSliderValues();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    //Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.drawBitmap(0, 0, icon, 128, 64, WHITE);
  display.display();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  bool savelog = makeLogarithmic;
  makeLogarithmic = false;

  for (int i = 0; i < NUM_MOTORS; i++) {
    int pin = motorMap[i];
    int returnval = 0;
    for (int j = 0; j < NUM_SLIDERS; j++) {
      if (analogInputs[j] == pin) {
        returnval = analogSliderValues[j];
      }
    }
    AF_DCMotor motor = motors[i];
    moveSliderTo(0, pin, motor);
  }
  makeLogarithmic = savelog;
  lastcmd = millis();
  int movespeeds[NUM_MOTORS];
  Serial.println("INITDONE");
}

void loop() {
  checkForTouch();
  if (!pause) {
    checkForButton();
  }

  checkForCommand();

  if (!pause) {
    updateSliderValues();

    //Check for data chanel to be open
    if (pushSliderValuesToPC) {
      sendSliderValues(); // Actually send data
    }

    if (firstcmd) {
      for (int i = 0; i < NUM_MOTORS; i++) {
        moveMotor(i);
      }
    }
  }

  // printSliderValues(); // For debug
  delay(FrequencyMS);
}

// reboots the arduino
void reboot() {
#if MCU32U4
  wdt_disable();
  wdt_enable(WDTO_30MS);
  while (1) {}
#elif MCUA328P
  asm volatile ("  jmp 0");
#endif
}

//checks if a mute button is pressed and acts correspondingly
void checkForButton() {
  for (int i = 0; i < NUM_MUTES; i++) {
    if (muteInputs[i] != -1) {
      int read = digitalRead(muteInputs[i]);
      int previous = buttonState[i];
      if (read == HIGH && previous == LOW && millis() - muteTimes[i] > DEBOUNCE_TIME) {
        if (mute[i]) {
          mute[i] = false;
          volumeValues[i] = muteValues[i];
          analogSliderValues[i] = muteValues[i];
        } else {
          mute[i] = true;
          muteValues[i] = volumeValues[i];
          volumeValues[i] = 0;
        }
        moveMotor(i);
        motorMoved[i] = 1;
        sliderMuted[i] = 0;
        muteTimes[i] = millis();
      } else {
        sliderMuted[i]++;
      }
      buttonState[i] = read;
    }
  }
}

//checks if someone is touching the slider of a motorized fader so that the input by the user isn't
//being ignored
void checkForTouch() {
  for (int i = 0; i < NUM_MOTORS; i++) {
    if (digitalRead(touchInputs[i]) == 0) {
      touchTimes[i] = millis();
      touch[i] = true;
    } else {
      if (millis() - touchTimes[i] > DEBOUNCE_TIME) {
        touch[i] = false;
      }
    }
  }
}

//updates text on the display
void showOnDisplay() {
  String dsp = "";
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    int vol = toVolume(volumeValues[i]);
    if (vol != 0) {
      if (vol >= 100) {
        dsp += String(vol);
      } else if (vol >= 10 ) {
        dsp += "0" + String(vol);
      } else {
        dsp += "00" + String(vol);
      }
    } else {
      dsp += "MMM";
    }
    if ( i < NUM_SLIDERS - 1) {
      dsp += "|";
    }
  }
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(names);
  display.println(dsp);
  display.println(receiveline);
  display.println(sendline);
  display.display();
}

//checks if a slider has to be moved to a different value and does so
void moveMotor(int i) {
  checkForTouch();
  if (!touch[i]) {
    AF_DCMotor motor = motors[i];
    int pin = motorMap[i];
    motorMoved[i]++;
    for (int j = 0; j < NUM_SLIDERS; j++) {
      if (analogInputs[j] == pin) {
        int vol = toVolume(volumeValues[i]);
        int analogvol = toVolume(getAnalogValue(pin));
        uint16_t diff = abs(vol - analogvol);
        if (diff > 1) {
          //Serial.println("Moving slider #" + String(i) + " " + String(analogval) + " " + String(diff));
          checkForTouch();
          if (!touch[i]) {
            moveSliderTo(volumeValues[j], pin, motor);
          }
          motorMoved[i] = 0;
        }
        return;
      }
    }
  }
}

//currently not useful but an attempt at making linear sliders logarithmic
uint16_t getAnalogValue(int input) {
  if (makeLogarithmic) {
    //return exp(6.774677191*pow(10,-3)*analogRead(input));
    //return log10(analogRead(input));
    Serial.println(log10(analogRead(input) + 1));
    return map(log10((analogRead(A11) + 1)) * 10000, log10(1) * 10000, log10(1024) * 10000, 0, 1023);
  } else {
    return analogRead(input);
  }
}

//updates values of sliders
void updateSliderValues() {
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    if (i < NUM_MUTES && mute[i]) {
      analogSliderValues[i] = 0;
      volumeValues[i] = 0;
    } else {
      bool motor = false;
      int motor_num = -1;
      for (int j = 0; j < NUM_MOTORS; j++) {
        if (motorMap[j] == analogInputs[i]) {
          motor = true;
          motor_num = j;
          break;
        }
      }
      if (!motor) {
        analogSliderValues[i] = getAnalogValue(analogInputs[i]);
      } else {
        if (touch[motor_num]) {
          analogSliderValues[i] = getAnalogValue(analogInputs[i]);
        } else {
          if (motorMoved[i] >= 0) {
            analogSliderValues[i] = volumeValues[i];
            //motorMoved[i] = 0;
          }
        }
      }
    }
  }
  //FOR TESTING:
  //memcpy(analogSliderValues, volumeValues, sizeof(analogSliderValues));
}

//sends slider values to the computer
void sendSliderValues() {
  String sendvals = "";
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    sendvals += analogSliderValues[i];
    if (i < NUM_SLIDERS - 1) {
      sendvals += "|";
    }
  }
  sendline = sendvals;
  Serial.println(sendvals);
}

//prints slider values easier to read
void printSliderValues() {
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    Serial.print("Slider #" + String(i + 1) + ": " + String(toVolume(analogSliderValues[i])) + " mV");

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

//returns the volume level that the analog value would be on the computer
int toVolume(int val) {
  return round(((float(val) / 1023)) * 100);
}

//used for splitting strings
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void checkForCommand() {
  //Check if data is waiting

  if (Serial.available() > 0) {

    if (!firstcmd) {
      firstcmd = true;
    }

    //Get start time of command
    unsigned long timeStart = millis();

    //Get data from Serial

    String input = Serial.readStringUntil('\r');  // Read chars from serial monitor
    Serial.readStringUntil('\n');
    //If data takes to long
    if (millis() - timeStart >= SerialTimeout) {
      Serial.println("TIMEOUT");
      return;
    }
    // Check and match commands
    else {
      // Start Sending Slider Values
      if ( input.equalsIgnoreCase("deej.core.start") == true ) {
        pushSliderValuesToPC = true;
      }

      // Stop Sending Slider Values
      else if ( input.equalsIgnoreCase("deej.core.stop") == true ) {
        pushSliderValuesToPC = false;
      }

      // Send Single Slider Values
      else if ( input.equalsIgnoreCase("deej.core.values") == true ) {
        sendSliderValues();
        lastcmdrequest = true;
      }

      // Send Human Readable Slider Values
      else if ( input.equalsIgnoreCase("deej.core.values.HR") == true ) {
        printSliderValues();
      }

      // Receive Values
      else if (input.equalsIgnoreCase("deej.core.receive") == true) {
        String receive = Serial.readStringUntil('\r');
        Serial.readStringUntil('\n');
        if (receive.length() > (4 * NUM_SLIDERS + (NUM_SLIDERS - 1)) || receive.length() < (1 * NUM_SLIDERS + (NUM_SLIDERS - 1))) {
          Serial.println("INVALID DATA: " + receive);
          Serial.flush();
          return;
        }
        receiveline = receive;
        int saveVals[NUM_SLIDERS];
        memcpy(saveVals, volumeValues, NUM_SLIDERS);
        String str = getValue(receive, '|', 0);
        for (int i = 1; str != ""; i++) {
          if (i - 1 < NUM_MUTES) {
            if (sliderMuted[i - 1] > 0) {
              volumeValues[i - 1] = str.toInt();
            }
          } else {
            volumeValues[i - 1] = str.toInt();
          }
          str = getValue(receive, '|', i);
        }
        showOnDisplay();
        lastcmdrequest = false;
        Serial.println(receive);
      }

      // Receive Group Names
      else if (input.equalsIgnoreCase("deej.core.receive.groupnames")) {
        String receive = Serial.readStringUntil('\r');
        Serial.readStringUntil('\n');
        String str = getValue(receive, '|', 0);
        for (int i = 1; i <= NUM_SLIDERS; i++) {
          groupNames[i - 1] = str;
          str = getValue(receive, '|', i);
        }
        names = "";
        for (int i = 0; i < NUM_SLIDERS; i++) {
          str = groupNames[i];
          if (str != "" && str != NULL) {
            names += str;
          } else {
            names += String(i + 1);
          }
          if (i < NUM_SLIDERS - 1) {
            names += "|";
          }
        }
        showOnDisplay();
        Serial.println(receive);
      }

      //reboots the arduino
      else if ( input.equalsIgnoreCase("deej.core.reboot") == true ) {
        for (int i = 0; i < NUM_MUTES; i++) {
          if (mute[i]) {
            analogSliderValues[i] = muteValues[i];
          }
        }
        delay(1000);
        sendSliderValues();
        Serial.flush();
        reboot();
      }

      else if (input.equalsIgnoreCase("deej.core.flush")) {
        Serial.flush();
      }

      //Default Catch all
      else {
        Serial.println("INVALIDCOMMAND");
        Serial.flush();
      }
    }
    lastcmd = millis();
    if (pause) {
      pause = false;
    }
    return;
  } else {
    if ((millis() - lastcmd) > 500 && firstcmd) {
      Serial.println("STALECONNECTION");
      lastcmd = millis();
      if (!pause) {
        pause = true;
        display.clearDisplay();
        display.drawBitmap(0, 0, icon, 128, 64, WHITE);
        display.display();
      }
      for (int i = 0; i < NUM_MOTORS; i++) {
        if (!touch[i]) {
          moveSliderTo(0, motorMap[i], motors[i]);
        }
      }
      Serial.flush();
      //reboot();
    }
  }
}

//moves a slider to a different value
void moveSliderTo(int value, int slider, AF_DCMotor motor) {
  int speed = 0;
  int vol = toVolume(value);
  int analogvol = toVolume(getAnalogValue(slider));
  int error = vol - analogvol;
  if (value > 1023 || value < 0) {
    return;
  }
  unsigned long mills = millis();
  while (abs(error) > 1) {
    speed = (int)((int)value - (int)getAnalogValue(slider));
    if (speed < 0) {
      motor.run(BACKWARD);
    } else if (speed > 0) {
      motor.run(FORWARD);
    } else {
      motor.run(RELEASE);
      return;
    }
    speed = abs(speed);
    /*if (speed > 230) {
      speed = 230;
      } else if (speed < 140) {
      speed = 140;
      }*/
    if (speed > 768) {
      speed = 768;
    }
    speed = map(speed, 0, 768, 140, 255);
    motor.setSpeed(speed);
    if ((millis() - mills) > 5000) {
      break;
    }
    delay(5);
    error = vol - toVolume(getAnalogValue(slider));
  }
  motor.run(RELEASE);
  speed = 0;
  motor.setSpeed(speed);
}
