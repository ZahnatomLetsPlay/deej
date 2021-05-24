#include "arduino.h"
#include <avr/wdt.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//Microcontroller type
//#define MCU32U4 1
#define MCUA328P 1

//You must Hard Code in the number of Sliders in
#define NUM_SLIDERS 3
#define SERIALSPEED 115200
#define FrequencyMS 10
#define SerialTimeout 5000 //This is two seconds

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const uint8_t analogInputs[NUM_SLIDERS] = {A0, A1, A2};

uint16_t analogSliderValues[NUM_SLIDERS];
uint16_t volumeValues[NUM_SLIDERS];

// Constend Send
bool pushSliderValuesToPC = false;
bool receivednewvalues = false;

String outboundCommands = "";

void setup() { 
  Serial.begin(SERIALSPEED);
  Serial.println("INITBEGIN");
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    //Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.display();
  delay(500);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    pinMode(analogInputs[i], INPUT);
  }

  Serial.println("INITDONE");
  //Serial.println("");
}

void loop() {
  checkForCommand();
  
  updateSliderValues();

  showOnDisplay();

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

void showOnDisplay() {
  display.clearDisplay();
  display.setCursor(0,10);
  String dsp = "";
  for(uint8_t i = 0; i< NUM_SLIDERS; i++){
    int vol = (int)(((float)volumeValues[i])/((float)1023)*((float)100));
    if(vol != 0){
      dsp += vol;
    } else {
      dsp += "M";
    }
    if( i < NUM_SLIDERS - 1){
      dsp += "|";
    }
  }
  display.println(dsp);
  display.display();
}

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
        /*if(Serial.available() > (4*NUM_SLIDERS+(NUM_SLIDERS-1))+4 || Serial.available() == 0 || Serial.available() < (1*NUM_SLIDERS+(NUM_SLIDERS-1))+4){
          String receive = Serial.readStringUntil('\r');
          Serial.readStringUntil('\n');
          Serial.println("INVALID DATA: " + receive);
          return;
        }*/
        String receive = Serial.readStringUntil('\r');
        Serial.readStringUntil('\n');

        if(receive.length() > (4*NUM_SLIDERS+(NUM_SLIDERS-1)) || receive.length() < (1*NUM_SLIDERS+(NUM_SLIDERS-1))){
          Serial.println("INVALID DATA: " + receive);
          return;
        }
        
        Serial.println(receive);
        char split[receive.length()];
        receive.toCharArray(split, receive.length()+1);
        char* piece = strtok(split, "|");
        for(int i = 0; piece!= NULL; i++){
          String value = String(piece);
          volumeValues[i] = value.toInt();
          piece = strtok(NULL, "|");
        }
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
