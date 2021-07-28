#include "arduino.h"
#include <avr/wdt.h>

#include <SPI.h>
#include <Wire.h>
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
#define SerialTimeout 5000 //This is five seconds

bool makeLogarithmic = false;
const uint8_t analogInputs[NUM_SLIDERS] = {A10, A11, A7, A8, A9};

uint16_t analogSliderValues[NUM_SLIDERS];
uint16_t volumeValues[NUM_SLIDERS];
String groupNames[NUM_SLIDERS];
bool pause = true;

// Constend Send
bool pushSliderValuesToPC = false;
bool receivednewvalues = false;
unsigned long lastcmd;
bool firstcmd = false;
bool lastcmdrequest = true;

String names;

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
    volumeValues[i] = 0;
    analogSliderValues[i] = 0;
  }

  updateSliderValues();

  bool savelog = makeLogarithmic;
  makeLogarithmic = false;

  makeLogarithmic = savelog;
  lastcmd = millis();
  Serial.println("INITDONE");
}

void loop() {
  checkForCommand();

  if (!pause) {
    updateSliderValues();

    //Check for data chanel to be open
    if (pushSliderValuesToPC) {
      sendSliderValues(); // Actually send data
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
    analogSliderValues[i] = getAnalogValue(analogInputs[i]);
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
        int saveVals[NUM_SLIDERS];
        memcpy(saveVals, volumeValues, NUM_SLIDERS);
        String str = getValue(receive, '|', 0);
        for (int i = 1; str != ""; i++) {
          volumeValues[i - 1] = str.toInt();
          str = getValue(receive, '|', i);
        }
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
        Serial.println(receive);
      }

      //reboots the arduino
      else if ( input.equalsIgnoreCase("deej.core.reboot") == true ) {
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
      }
      Serial.flush();
      //reboot();
    }
  }
}
