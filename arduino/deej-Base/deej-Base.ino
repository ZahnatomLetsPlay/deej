#include "arduino.h"
#include <avr/wdt.h>

//Microcontroller type
//#define MCU32U4 1
#define MCUA328P 1

//You must Hard Code in the number of Sliders in
#define NUM_SLIDERS 3
#define SERIALSPEED 115200
#define FrequencyMS 10
#define SerialTimeout 5000 //This is two seconds

const uint8_t analogInputs[NUM_SLIDERS] = {A0, A1, A2};

uint16_t analogSliderValues[NUM_SLIDERS];
uint16_t volumeValues[NUM_SLIDERS];
String groupNames[NUM_SLIDERS];

// Constend Send
bool pushSliderValuesToPC = false;
bool receivednewvalues = false;

void setup() { 
  Serial.begin(SERIALSPEED);
  Serial.println("INITBEGIN");
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    pinMode(analogInputs[i], INPUT);
  }

  Serial.println("INITDONE");
  //Serial.println("");
}

void loop() {
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

//bool up = true;

void updateSliderValues() {
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
     analogSliderValues[i] = analogRead(analogInputs[i]);
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
        String receive = Serial.readStringUntil('\r');
        Serial.readStringUntil('\n');

        if(receive.length() > (4*NUM_SLIDERS+(NUM_SLIDERS-1)) || receive.length() < (1*NUM_SLIDERS+(NUM_SLIDERS-1))){
          Serial.println("INVALID DATA: " + receive);
          return;
        }
        
        char split[receive.length()];
        receive.toCharArray(split, receive.length()+1);
        char* piece = strtok(split, "|");
        for(int i = 0; piece!= NULL; i++){
          String value = String(piece);
          volumeValues[i] = value.toInt();
          piece = strtok(NULL, "|");
        }
        Serial.println(receive);
      }

      // Receive Group Names
      else if(input.equalsIgnoreCase("deej.core.receive.groupnames")){
        String receive = Serial.readStringUntil('\r');
        Serial.readStringUntil('\n');
        char split[receive.length()];
        receive.toCharArray(split, receive.length()+1);
        char* piece = strtok(split, "|");
        for(int i = 0; piece != NULL; i++){
          String groupname = String(piece);
          groupNames[i] = groupname;
          piece = strtok(NULL, "|");
        }
        Serial.println(receive);
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
  }
}
