#pragma once
#include <stdint.h>
#include <functional>
#include <vector>
#include <Wire.h>
#include <U8g2lib.h>
#include "pico/time.h"
#include <string>

// Create an instance of the U8g2 graphics library.
U8G2_SH1107_SEEED_128X128_F_HW_I2C u8g2(U8G2_R2);

const uint8_t _OLED_HEIGHT = 128;
const uint8_t _OLED_WIDTH  = 128;
const uint8_t _LG_FONT_WIDTH  = 6;
const uint8_t _LG_FONT_HEIGHT = 8;
const uint8_t _SM_FONT_HEIGHT = 6;
const uint8_t _LEFT_MARGIN = 6;
const uint8_t _RIGHT_MARGIN = 8;

void drawStringWrap(u8g2_uint_t x, u8g2_uint_t y, std::string str, bool altWrap = false) {
  std::vector<std::string> linesOfText(0);
  uint8_t yCursor = y;
  int8_t yLineBreak = 2 + u8g2.getAscent() - u8g2.getDescent();
  size_t strCursor = 0;
  bool endOfString = false;
  while ((yCursor <= _OLED_HEIGHT - yLineBreak) && (yCursor > 0)) {
    if (endOfString) break;
    std::string thisLine;
    thisLine.clear();
    bool endOfLine = false;
    while (!endOfString && !endOfLine) {
      if (str.substr(strCursor,1) == "\n") {
        endOfLine = true;
      } else {
        thisLine += str[strCursor];
        endOfLine = (u8g2.getStrWidth(thisLine.c_str()) > (_OLED_WIDTH - _RIGHT_MARGIN - x));
      }
      endOfString = (++strCursor >= str.size());
    }
    linesOfText.push_back(thisLine);
    yCursor += yLineBreak * (altWrap ? -1 : 1); 
  }
  for (size_t i = 0; i < linesOfText.size(); ++i) {
    int j = i;
    if (altWrap) j -= (linesOfText.size() - 1);
    u8g2.drawStr(x, y + (j * yLineBreak), linesOfText[i].c_str());
  }
}

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

const size_t maximum_GUI_layers = 32;

struct GUI_Object {
  size_t sz;
  uint32_t last_updated;
  uint32_t context;
  std::vector<std::function<void(std::string)>> drawLayer;
  std::vector<std::string> strParam;
  GUI_Object(size_t n) : 
    sz(n),
    drawLayer(n), 
    strParam(n),
    context(0),
    last_updated(0) {}  
  void add_context(uint32_t c) {
    context |= c;
    last_updated = timer_hw->timerawl;
  }
  void remove_context(uint32_t c) {
    context &= ~c;
    last_updated = timer_hw->timerawl;
  }
  void set_context(uint32_t c, std::string s) {
    context = c;
    for (int i = 0; i < sz; ++i) {
      if (context & (1u << i)) {
        strParam[i] = s;
      }
    }
    last_updated = timer_hw->timerawl;
  }
  void set_handler(uint32_t c, std::function<void(std::string)> f) {
    for (int i = 0; i < sz; ++i) {
      if (c & (1u << i)) {
        drawLayer[i] = f;
      }
    }
    last_updated = timer_hw->timerawl;
  }
  void draw() {
    for (size_t i = 0; i < sz; ++i) {
      if (context & (1u << i)) {
        drawLayer[i](strParam[i]);
      }
    }
  }
};
GUI_Object GUI(maximum_GUI_layers);