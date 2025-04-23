#pragma once
#include "color_conversion.h"
#include <Adafruit_NeoPixel.h>  // library of code to interact with the LED array
Adafruit_NeoPixel strip;

void connect_neoPixels(uint8_t pin, size_t numLEDs) {
  strip.updateType(NEO_GRB + NEO_KHZ800);
  strip.updateLength(numLEDs);
  strip.setPin(pin);
  strip.begin();
  strip.clear();
  strip.show();
}

struct Pixel_Data {
  uint32_t LEDcode = 0;

  uint32_t baseRGBcolor = 0;
  uint32_t cachedLEDcodeBase = 0; // calculate it once and store value, to make LED playback snappier 
  uint32_t cachedLEDcodeAnim = 0; // calculate it once and store value, to make LED playback snappier 
  uint32_t cachedLEDcodePlay = 0; // calculate it once and store value, to make LED playback snappier
  uint32_t cachedLEDcodeRest = 0; // calculate it once and store value, to make LED playback snappier
  uint32_t cachedLEDcodeOff  = 0; // calculate it once and store value, to make LED playback snappier
  uint32_t cachedLEDcodeDim  = 0; // calculate it once and store value, to make LED playback snappier  
};

