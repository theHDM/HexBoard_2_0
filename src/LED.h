#pragma once

#include <Adafruit_NeoPixel.h>  // library of code to interact with the LED array
Adafruit_NeoPixel strip;

/*
bool pixelSaverOn = 0;                         
uint64_t timeSinceLastKey = 0;  
const uint64_t pixelSaverTimeout = (1u << 31); // 2^31 microseconds ~ 36 minutes
*/

void connect_neoPixels(uint8_t pin, size_t numLEDs) {
  strip.updateType(NEO_GRB + NEO_KHZ800);
  strip.updateLength(numLEDs);
  strip.setPin(pin);
  strip.begin();
  strip.clear();
  strip.show();
}

// screensaver