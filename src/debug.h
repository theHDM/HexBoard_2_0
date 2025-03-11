#pragma once
#include <Arduino.h>
#include <string>
#include "pico/time.h"

struct hexBoard_Debug_Object {
  volatile uint8_t ownership;

  bool *_ptrIsOn;
  std::string msg;
  hexBoard_Debug_Object()  : _ptrIsOn(nullptr) {}
  bool isOn() {
    if (_ptrIsOn == nullptr) return false;
    return *_ptrIsOn;
  }
  bool isOff() {
    if (_ptrIsOn == nullptr) return false;
    return !(*_ptrIsOn);
  }
  void add(const std::string& s, bool core1 = false) {    
    if (isOff()) return;
    while (ownership == (core1 ? 1 : 2)) {}
    ownership = (core1 ? 2 : 1);
    msg += s; 
    ownership = 0;
  }
  template <typename T> void add_num(T i, bool core1 = false) { 
    if (isOff()) return;
    while (ownership == (core1 ? 1 : 2)) {}
    ownership = (core1 ? 2 : 1);
    msg += std::to_string(i); 
    msg += " ";
    ownership = 0;
  }
  void timestamp(bool core1 = false) { 
    if (isOff()) return;
    while (ownership == (core1 ? 1 : 2)) {}
    ownership = (core1 ? 2 : 1);
    msg += "@ time ";
    msg += std::to_string(timer_hw->timerawl/1000000.f);
    msg += ": ";
    ownership = 0;
  }

  // only run by primary core
  void setStatus(bool *_ptr) {
    while (ownership == 2) {}
    ownership = 1;
    _ptrIsOn = _ptr;
    ownership = 0;
  }
  void clear() {
    while (ownership == 2) {}
    ownership = 1;
    msg.clear();
    ownership = 0;
  }
  void send() {
    if (isOn()) {
      while (ownership == 2) {}
      ownership = 1;
      Serial.flush();
      Serial.print(msg.c_str()); 
      ownership = 0;
      clear();
    }
  }		
};

hexBoard_Debug_Object  debug;