#include "arduino.h"
#include <avr/wdt.h>

//Microcontroller type
//#define MCU32U4 1
#define MCUA328P 1

//You must Hard Code in the number of Sliders in
#define NUM_SLIDERS 6
#define SERIALSPEED 9600
#define FrequencyMS 0
#define SerialTimeout 2000 //This is two seconds

const uint8_t analogInputs[NUM_SLIDERS] = {A0, 19, 20, 21, 9, 8};

uint16_t analogSliderValues[NUM_SLIDERS] = {10, 20, 30, 40, 50, 60};
uint16_t volumeValues[NUM_SLIDERS] = {1,2,3,4,5,6};

// Constend Send
bool pushSliderValuesToPC = false;
bool receivednewvalues = false;

String outboundCommands = "";

void setup() { 
  Serial.begin(SERIALSPEED);
  //Serial.print("INITBEGIN");
  pinMode(LED_BUILTIN, OUTPUT);
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    pinMode(analogInputs[i], INPUT);
  }

  //Serial.println("INITDONE");
  //Serial.println("");
}

void loop() {
  checkForCommand();
  /*if(receivednewvalues){
    checkForCommand();
    receivednewvalues = false;
  }*/
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

void updateSliderValues() {
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
     analogSliderValues[i] = analogRead(analogInputs[i]);
  }
  //FOR TESTING:
  memcpy(analogSliderValues, volumeValues, sizeof(analogSliderValues));
}

void sendSliderValues() {
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    Serial.print(analogSliderValues[i]);

    if (i < NUM_SLIDERS - 1) {
      Serial.print("|");
    }
  }
  /*if (outboundCommands != "") {
    Serial.print(":");
    Serial.print(outboundCommands);
    outboundCommands = "";
  }*/

  Serial.println();
}

void addCommand(String cmd) {
  if (outboundCommands != "") {
    outboundCommands += "|";
  }
  outboundCommands += cmd;
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
      Serial.println();
    }
  }
}

void checkForCommand() {
  //Check if data is waiting
  //if (Serial.available() > 0) {
    //Get start time of command
    unsigned long timeStart = millis();

    //Get data from Serial
    String input = Serial.readStringUntil('\n');  // Read chars from serial monitor
    //Serial.println(input);
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
      }

      // Stop Sending Slider Values
      else if ( input.equalsIgnoreCase("deej.core.stop") == true ) {
        pushSliderValuesToPC = false;
      }
      
      // Send Single Slider Values
      else if ( input.equalsIgnoreCase("deej.core.values") == true ) {
        sendSliderValues();
      }

      // Send Human Readable Slider Values 
      else if ( input.equalsIgnoreCase("deej.core.values.HR") == true ) {
        printSliderValues();
      }

      // Receive Values
      else if(input.equalsIgnoreCase("deej.core.receive") == true){
        String receive = Serial.readStringUntil("\n");
        char split[receive.length()];
        receive.toCharArray(split, receive.length());
        char* piece = strtok(split, "|");
        //Serial.print("Received: ");
        for(int i = 0; piece!= NULL; i++){
          //Serial.println(piece);
          String value = String(piece);
          volumeValues[i] = value.toInt();
          //Serial.print(value.toInt());
          //Serial.print("|");
          piece = strtok(NULL, "|");
        }
        //Serial.println();
        receivednewvalues = true;
        //Serial.println(receive);
        /*for(int i = 0; i<NUM_SLIDERS; i++){
          String value = Serial.readStringUntil("|");
          Serial.println(value);
          volumeValues[i] = value.toInt();
        }*/
      }

      else if ( input.equalsIgnoreCase("deej.core.reboot") == true ) {
        reboot();
      }

      //Default Catch all
      else {
        Serial.println("INVALIDCOMMANDS: " + input);
        return;
      }
    }
  //}
}
