#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <FastLED.h>

#include <Hash.h>
#include <EEPROM.h>
#include <WebSockets.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Hash.h>
#include "SettingsServer.h"
#include "LedStrip.h"
#define USE_SERIAL Serial

extern "C" {
#include "user_interface.h"
}

// Defining LED strip
const int NUM_LEDS = 60;                 //Number of LEDs in your strip
const int LEDSTRIP_PIN = 0;         //LEDSTRIP_PIN
const int WHITELEDPWM_PIN = 0;
const int BRIGHTNESS = 50;               // Define maximum brightness as used by the ADAFRUIT library, is important

LedStrip ledStrip(LEDSTRIP_PIN, NUM_LEDS);
//LedStrip ledStrip(LEDSTRIP_PIN,LEDWHITEPWM_PIN,NUM_LEDS);

void putOnStrip(void);
//Some Variables
byte myEffect = 1;                  //what animation/effect should be displayed

byte myHue = 33;                    //I am using HSV, the initial settings display something like "warm white" color at the first start
byte mySaturation = 168;
byte myValue = 255;
unsigned int myAnimationSpeed = 100;
unsigned int myWhiteLedValue = 0;
byte rainbowHue = myHue;            //Using this so the rainbow effect doesn't overwrite the hue set on the website

int flickerTime = random(200, 400);
int flickerLed;
int flickerValue = 110 + random(-3, +3); //70 works nice, too
int flickerHue = 33;

bool eepromCommitted = true;

unsigned long currentTime = 0;
unsigned long previousTime = 0;
unsigned long lastChangeTime = 0;
unsigned long currentChangeTime = 0;
unsigned long startTimeSleepTimer = 0;
unsigned long sleepTime = 0;
bool inSleep = 0;

//#include "LEDanimations.h"
#include "LEDWebsockets.h"



void setup() {
  ledStrip.setMaxBrightness(BRIGHTNESS);
  ledStrip.begin();
  ledStrip.setWhiteValue(0);
  ledStrip.setWhiteValue(1);
  EEPROM.begin(6);  // Using simulated EEPROM on the ESP8266 flash to remember settings after restarting the ESP
  Serial.begin(115200);
  Serial.println("Ledtest example");
  //  LEDS.addLeds<WS2812B,DATAFASTLED_PIN,GRB>(leds,NUM_LEDS);  // Initialize the LEDs


  // Reading EEPROM
  myEffect = 1;                         // Only read EEPROM for the myEffect variable after you're sure the animation you are testing won't break OTA updates, make your ESP restart etc. or you'll need to use the USB interface to update the module.
  //  myEffect = EEPROM.read(0); //blocking effects had a bad effect on the website hosting, without commenting this away even restarting would not help
  myHue = EEPROM.read(1);
  mySaturation = EEPROM.read(2);
  myValue = EEPROM.read(3);

  //  LEDS.showColor(CHSV(0,255,255));
  ledStrip.getLedSet() = CHSV(0, 100, 100);
  ledStrip.show();
  delay(100);                                         //Delay needed, otherwise showcolor doesn't light up all leds or they produce errors after turning on the power supply - you will need to experiment
  ledStrip.getLedSet() = CHSV(0, 100, 100);
  ledStrip.show();
  setupWiFi();
  ledStrip.getLedSet() = CHSV(myHue, mySaturation, myValue);
  ledStrip.show();
  startSettingsServer();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  ledStrip.getLedSet() = CHSV(100, 0, 0);
  ledStrip.show();
  USE_SERIAL.println("Free " + String(ESP.getFreeHeap()));
  
}


void startSleepTimer(int value) {
  sleepTime = value * 1000;
  startTimeSleepTimer = millis();
  inSleep = 1;
}

int getSleepTimerRemainingTime(void) {
  return (sleepTime - startTimeSleepTimer) / 1000 / 60;
}

int getAnimationSpeed(void) {
  return myAnimationSpeed;
}

void disableSleepTimer(void) {
  inSleep = 0;
}

void whiteFadeToBlackBy(int amount) {
  if (ledStrip.getWhiteValue() > amount) {
    ledStrip.setWhiteValue(ledStrip.getWhiteValue() - amount);
  }
}

void loop() {
  webSocket.loop();                           // handles websockets
  settingsServerTask();
  if (myEffect != 5)
  {
    ledStrip.setWhiteValue(myWhiteLedValue);
  }

  if (inSleep == 1) {
    if (millis() - startTimeSleepTimer > sleepTime) {
      myEffect = 5;
      inSleep = 0;
    }
  }


  switch (myEffect) {                           // switches between animations
    case 1: // Solid Color
      EVERY_N_MILLISECONDS( 20 ) {
        ledStrip.getLedSet() = CHSV(myHue, mySaturation, myValue);
        ledStrip.show();
        //LEDS.show();
      }

      break;
    case 2: // Ripple effect
      //      ripple();
      break;
    case 3: // Cylon effect
      //      cylon();
      break;
    case 4: // Fire effect
      //     Fire2012();
      break;
    case 5: // Turn off all LEDs
      EVERY_N_MILLISECONDS( 50 ) {
        whiteFadeToBlackBy(8);
        ledStrip.getLedSet().fadeToBlackBy(2);
        ledStrip.show();
      }
      break;
    case 6: // loop through hues with all leds the same color. Can easily be changed to display a classic rainbow loop
      EVERY_N_MILLISECONDS( 20 ) {
        rainbowHue = rainbowHue + 1;
        ledStrip.getLedSet() = CHSV(rainbowHue, mySaturation, myValue);
        ledStrip.show();
      }
      break;
    case 7: // make a single, random LED act as a candle
      currentTime = millis();
      ledStrip.getLedSet().fadeToBlackBy(1);
      ledStrip.leds[flickerLed] = CHSV(flickerHue, 255, flickerValue);
      flickerTime = random(150, 500);
      if (currentTime - previousTime > flickerTime) {
        flickerValue = 110 + random(-10, +10); //70 works best
        flickerHue = 33; //random(33, 34);
        previousTime = currentTime;
        ledStrip.show();
      }
      break;
    default:
      ledStrip.getLedSet() = CRGB(0, 255, 0);
      ledStrip.show();
      break;
  }

  // EEPROM-commit and websocket broadcast -- they get called once if there has been a change 1 second ago and no further change since. This happens for performance reasons.
  currentChangeTime = millis();
  if (currentChangeTime - lastChangeTime > 5000 && eepromCommitted == false) {
    EEPROM.commit();
    eepromCommitted = true;
 //   String aMessage = getStatusString();
 //   webSocket.broadcastTXT(aMessage); // Tell all connected clients which HSV values are running
  }
}
