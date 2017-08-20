#include "LedStrip.h"

LedStrip::LedStrip(int RGBPin, int PWMPin, int numLeds) : LedStrip(numLeds){
 strip =  Adafruit_NeoPixel(numLeds, RGBPin, NEO_GRB + NEO_KHZ800);
 analogWriteFreq(200);
}

LedStrip::LedStrip(int RGBWPin, int numLeds) : LedStrip(numLeds){
  strip = Adafruit_NeoPixel(numLeds, RGBWPin, NEO_GRBW + NEO_KHZ800);
}

LedStrip::LedStrip(int numLeds){
  amountOfLeds = numLeds;
  leds = new CRGB[numLeds];
}

void LedStrip::begin(void){
  strip.begin();
}

void LedStrip::writeWhiteLedPWMIfChanged(int value)
{
  if (whiteValue != value)
  {
    whiteValue = value;
    show();
  }
}

CRGBSet LedStrip::getLedSet(void){
  CRGBSet ledSet(leds,amountOfLeds);
  return ledSet;
}

void LedStrip::setMaxBrightness(int brightness){
  strip.setBrightness(brightness);
}
int LedStrip::getWhiteValue(void){
  return whiteValue;
}

void LedStrip::setWhiteValue(int value){
  if(value<0){
    value=0;
  }
  writeWhiteLedPWMIfChanged(value);
}

void LedStrip::show(void){
  uint8_t pwmTemp=whiteValue/4;
  int i = 0;
  for (CRGB & pixel : getLedSet()) {
    strip.setPixelColor(i++,pixel.r,pixel.g,pixel.b,pwmTemp);  
  }
  strip.show();
}
