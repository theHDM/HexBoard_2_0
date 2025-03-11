#pragma once
/* 
 *  This is the background code that collects the
 *  pinout level for each of the key switches on the
 *  HexBoard. This code is run on core1 in the background 
 *  and passes key-press messages to core0 for processing.
 */

#include <stdint.h>
#include <functional>
#include <array>    // structure for managing info for each key

#include <Wire.h>   // needed to set pin states
#include "pico/util/queue.h"
#include "pico/time.h"
#include "config.h" // import hardware config constants

struct Key_Msg {
  uint32_t timestamp;
  uint8_t  switch_number;
  uint8_t  level;
};
queue_t key_press_queue;

class hexBoard_Key_Object {
protected:
  bool            active;         // is the object ready to run in the background
  const uint8_t * mux;            // reference to existing constant
  const uint8_t * col;            // reference to existing constant
  const bool    * analog;         // reference to existing constant
  uint8_t         m_ctr;          // mux counter
  uint8_t         m_val;          // mux value
  bool            send_pressure;  // are we sending key messages on pressure change
  std::array<uint8_t,  keys_count> pressure;
  std::array<uint16_t, keys_count> high;
  std::array<uint16_t, keys_count> low;
  std::array<uint16_t, keys_count> invert_range;
  int8_t ownership; // -1 = no one, 0 = core0, 1 = core1
  void calibrate(uint8_t _k, uint16_t _hi, uint16_t _lo) {
    high[_k] = _hi;
    low[_k] = _lo;
    invert_range[_k] = (127u << 9);
    if (_hi - _lo > 1) {
      invert_range[_k] /= (_hi - _lo);
    }
  }

public:
  hexBoard_Key_Object(const uint8_t *arrM, const uint8_t *arrC, const bool *arrA)
  : mux(arrM), col(arrC), analog(arrA), m_ctr(0), m_val(0)
  , active(false), send_pressure(false) {
    for (size_t i = 0; i < mux_pins_count; ++i) {
      pinMode(*(mux + i), OUTPUT);
      digitalWrite(*(mux + i), 0);
    }
    for (size_t i = 0; i < col_pins_count; ++i) {
      if (*(analog + i)) {
        pinMode(*(col + i), INPUT);
      } else {
        pinMode(*(col + i), INPUT_PULLUP); 
      }
      for (size_t j = 0; j < mux_channels_count; ++j) {
        uint8_t k = linear_index(j,i);
        pressure[k] = 0;
        if (*(analog + i)) {
          calibrate(k,
            default_analog_calibration_up,
            default_analog_calibration_down
          );
        } else { 
          calibrate(k, 1, 0);
        }
      }
    }
  }

  void start() {active = true; }
  void stop()  {active = false;}

  // wrapper to safely calibrate keys from core0
  void recalibrate(uint8_t atMux, uint8_t atCol, uint16_t newHigh, uint16_t newLow) {
    while (ownership == 1) {}
    ownership = 0;
    calibrate(linear_index(atMux, atCol), newHigh, newLow);
    ownership = -1;
  }

  void poll() {
    if (!active) return;
    uint8_t  index;
    uint16_t pin_read;
    uint8_t  level;
    while (ownership == 0) {}
    ownership = 1;
    for (size_t i = 0; i < col_pins_count; ++i) {
      index = linear_index(m_val, i);
      pin_read = *(analog + i) 
               ? analogRead(*(col + i))
               : digitalRead(*(col + i));
      if (pin_read >= high[index]) {
        level = 0;
      } else if (pin_read <= low[index]) {
        level = 127;
      } else if (send_pressure) {
        level = (invert_range[index] * (high[index] - pin_read)) >> 9;
      } else {
        level = 64;
      }
      if (level != pressure[index]) {
        Key_Msg key_msg_in;
        key_msg_in.timestamp = timer_hw->timerawl;
        key_msg_in.switch_number = index;
        key_msg_in.level = level;
        queue_add_blocking(&key_press_queue, &key_msg_in);
        pressure[index] = level;
      }
    }
    ownership = -1;
    // this algorithm cycles through the multiplexer
    // by changing one bit at a time and still
    // making sure all permutations are reached
    if (++m_ctr == mux_channels_count) {m_ctr = 0;}
    size_t b = mux_pins_count - 1;
    for (size_t i = 0; i < b; ++i) {
      if ((m_ctr >> i) & 1) { b = i; }
    }
    m_val ^= (1 << b);
    digitalWrite(*(mux + b), (m_val >> b) & 1);
  }
  
  void begin() {
    start();
  }
};