#pragma once
#include <stdint.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "pico/time.h"

// Create an instance of the U8g2 graphics library.
U8G2_SH1107_SEEED_128X128_F_HW_I2C u8g2(U8G2_R2);

const uint8_t _OLED_HEIGHT = 128;
const uint8_t _OLED_WIDTH  = 128;
const uint8_t _LG_FONT_WIDTH  = 6;
const uint8_t _LG_FONT_HEIGHT = 8;
const uint8_t _SM_FONT_HEIGHT = 6;
const uint8_t _LEFT_MARGIN = 6;
const uint8_t _RIGHT_MARGIN = 8;

void connect_OLED_display(uint8_t SDA, uint8_t SCL) {
  // the microprocessor is the RP2040 / RP2350 series
  // and we use Earle Philhower's pico library to interface
  // with hardware. the u8g2 library to control the OLED screen
  // has a function to set the pins but it is not compatible
  // with Earle's library. we therefore set the u8g2 pins
  // using the Wire initializers as shown below.
  Wire.setSDA(SDA);
  Wire.setSCL(SCL);
  u8g2.begin();
  u8g2.setBusClock(1000000);
  u8g2.setContrast(255);
  u8g2.setFont(u8g2_font_6x12_tr);
}

struct OLED_screensaver {
  bool screensaver_mode;
  uint8_t contrast_on;
  uint8_t contrast_off;
  uint32_t switch_time;
  int *_ptr_delay;
  OLED_screensaver(uint8_t contrast_on_, uint8_t contrast_off_)
  : switch_time(0), screensaver_mode(true), _ptr_delay(nullptr)
  , contrast_on(contrast_on_), contrast_off(contrast_off_) {}
  void jiggle() {
    if (_ptr_delay == nullptr) return;
    switch_time = timer_hw->timerawl;
    switch_time += ((unsigned long long int)(*_ptr_delay) << 20);
    if (!screensaver_mode) return;
    screensaver_mode = false;
    u8g2.setContrast(contrast_on);
  }
  void setDelay(int *_ptr) {
    _ptr_delay = _ptr;
    jiggle();
  }
  void poll() {
    if (screensaver_mode) return;
    if (timer_hw->timerawl >= switch_time) {
      screensaver_mode = true;
      u8g2.setContrast(contrast_off);
    }
  }
};