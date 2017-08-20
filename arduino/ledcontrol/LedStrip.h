#include <Adafruit_NeoPixel.h>
#include <FastLED.h>
//#include "LEDanimations.h"

class LedStrip{
  public:
    LedStrip(int RGBWPin,int numLeds);
    LedStrip(int RGBPin, int PWMPin, int numLeds);
    void begin(void);
    int amountOfLeds;
    CRGB* leds;
    Adafruit_NeoPixel strip;
    void setMaxBrightness(int);
    void show(void);
    void setWhiteValue(int);
    int getWhiteValue(void);
    CRGBSet getLedSet(void);
  private:
    LedStrip(int numLeds);
    void writeWhiteLedPWMIfChanged(int);
    int whiteValue;
};





