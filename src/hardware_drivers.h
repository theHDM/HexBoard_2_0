#pragma once
#include <stdint.h>
#include <cmath>
#include <array>
#include <vector>
#include <functional>
#include <algorithm> // std::find
#include <Wire.h>   // needed to set pin states
#include "hardware/pwm.h"       // library of code to access the processor's built in pulse wave modulation features
#include "pico/util/queue.h"
#include "pico/time.h"
#include "config.h" // import hardware config constants

namespace hexBoardHW {

  namespace Synth {
    /* 
    *  This is the background code for direct digital synthesis
    *  of synthesizer sounds for the HexBoard.
    *  This code is run on core1 in the background, calculates
    *  one audio sample every polling period and sends to
    *  designated pins via PWM. When core1 is inactive, core0
    *  may update the object with music messages such as note-on,
    *  waveform change, mod wheel, volume, pressure, etc.
    */
    enum class ADSR_Phase {
      off, attack, decay, sustain, release
    };

    struct Voice {
      int8_t wavetable[256];
      uint32_t pitch_as_increment;
      uint8_t base_volume;
      uint32_t attack; // express in # of samples
      uint32_t decay;  // express in # of samples
      uint8_t sustain; // express from 0-255
      uint32_t release; // express in # of samples

      uint32_t loop_counter;
      int32_t envelope_counter; // used to apply envelope
      uint8_t envelope_level; // stored to ensure even fade at release
      ADSR_Phase phase;  
      uint32_t attack_inverse;
      uint32_t decay_inverse;
      uint32_t release_inverse;
      uint8_t  ownership;

      // define a series of setter functions for core0
      // which will block if core1 is trying to calculate
      // the next sample.
      void update_wavetable(const std::array<int8_t,256>& tbl) {
        while (ownership == 1) {}
        ownership = 0;
        for (size_t i = 0; i <  256; ++i) {
          wavetable[i] = tbl[i];
        }
        ownership = -1;
      }
      void update_pitch(uint32_t increment) {
        while (ownership == 1) {}
        ownership = 0;
        pitch_as_increment = increment;
        ownership = -1;
      }
      void update_base_volume(uint8_t volume) {
        while (ownership == 1) {}
        ownership = 0;
        base_volume = volume;
        ownership = -1;
      }
      void update_envelope(uint32_t a, uint32_t d, uint8_t s, uint32_t r) {
        while (ownership == 1) {}
        ownership = 0;
        attack = a * 1000 / audio_sample_interval_uS;
        decay = d * 1000 / audio_sample_interval_uS;
        sustain = s;
        release = r * 1000 / audio_sample_interval_uS;
        attack_inverse  = !attack ? 0 : 0xFFFFFFFF / attack;
        decay_inverse   = !decay ? 0 : ((256 - sustain) << 24) / decay;
        release_inverse = !release ? 0 : (sustain << 24) / release;
        ownership = -1;
      }
      void note_on() {
        while (ownership == 1) {}
        ownership = 0;
        envelope_counter = 0;
        phase = ADSR_Phase::attack;
        ownership = -1;
      }
      void note_off() {
        while (ownership == 1) {}
        ownership = 0;
        phase = ADSR_Phase::release;
        envelope_counter = 0;
        if (envelope_level != sustain) {
          envelope_counter = round((1.f - ((float)envelope_level / sustain)) * release);
        }
      }
      // called from core 1 only
      int32_t next_sample() {
        while (ownership == 0) {}
        ownership = 1;
        loop_counter += pitch_as_increment;
        int8_t sample = wavetable[loop_counter >> 24];
        switch (phase) {
          case ADSR_Phase::attack:
            if (envelope_counter == attack) {
              phase = ADSR_Phase::decay;
              envelope_counter = 0;
              envelope_level = 255;
            } else {
              ++envelope_counter;
              envelope_level = (envelope_counter * attack_inverse) >> 24; 
            }
            break;
          case ADSR_Phase::decay:
            if (envelope_counter >= decay) {
              phase = ADSR_Phase::sustain;
              envelope_counter = 0;
              envelope_level = sustain;
            } else {
              ++envelope_counter;
              envelope_level = 255 - ((envelope_counter * decay_inverse) >> 24); 
            }
            break;
          case ADSR_Phase::sustain: 
            envelope_level = sustain;
            break;
          case ADSR_Phase::release:
            if (envelope_counter == release) {
              phase = ADSR_Phase::off;
              envelope_level = 0;
            } else {
              ++envelope_counter;
              envelope_level = sustain - ((envelope_counter * release_inverse) >> 24); 
            }
        }
        ownership = -1;
        return (sample * base_volume * envelope_level) >> 8;
      }
    };

    const uint8_t energizeBits   = 2;
    const uint8_t deEnergizeBits = 3;
    const uint8_t rampUpCurveBits = 6;

    struct Instance {
      bool active;
      std::array<Voice, synth_polyphony_limit> voice;
      std::vector<uint8_t> pins;
      bool pin_status[GPIO_pin_count];
      pwm_config cfg;
      uint8_t ownership;
      uint16_t baseline_level;

      void internal_set_pin(uint8_t pin, bool activate) {
        auto n = std::find(pins.begin(), pins.end(), pin);
        if (n == pins.end()) pins.emplace_back(pin);
          pin_status[pin] = activate;
      }

      Instance(const uint8_t* pins, size_t count)
      : active(false) {
        for (size_t i = 0; i < GPIO_pin_count; ++i) {
          pin_status[i] = false;
        }
        if (count) {
          for (size_t i = 0; i < count; ++i) {
            internal_set_pin(pins[i], true);
          }
        }
      }
      Instance() : Instance(nullptr, 0) {}
 
      void start() {active = true;}
      void stop() {active = false;}

      void set_pin(uint8_t pin, bool activate) {
        while (ownership == 1) {}
        ownership = 0;
        internal_set_pin(pin, activate);
        ownership = -1;
      }

      void poll() {
        if (!active) return;
        int32_t mixLevels = 0;
        while (ownership == 0) {}
        bool anyVoicesOn = false;
        for (auto& v : voice) {
          if (v.phase == ADSR_Phase::off) continue;
          if (!v.base_volume) continue;

          anyVoicesOn = true;
          mixLevels += v.next_sample();
        }
        if (anyVoicesOn) {
          // waveform formula:
          // amplitude ~ sqrt(10^(dB/10))
          // dB ~ 2 * 10 * log10(level/max_level)
          // 24 bits per voice (1 << 24)
          // if there are 8 voices at max, clip possible at (8 << 24)
          // level  1   2   3   4   5   6   7   8
          // dB   -18 -12  -9  -6  -4 -2.5 -1  0
          // compression of 3:1 
          // dB   -6  -4   -3  -2  -1.4 -0.8 -0.4 0
          // level 4   5   5.7 6.4 6.8  7.3  7.7  8
          // lvl/255 = old lvl/255 ^ 1/3
          // this formula is a dirty linear estimate of ^1/3 that
          // also scales the level down to the right # of bits
          // 
          // Compression = lvl<25% ? lvl*1403/512 : [297 + 315*lvl] /512
          mixLevels /= synth_polyphony_limit;
          if (std::abs(mixLevels) < (1u << 13)) {
            mixLevels *= 1403;
          } else {
            mixLevels *= 315;
            mixLevels += 297;
          }
          mixLevels >>= 25 - audio_bits;
          mixLevels += neutral_level;
          if ((baseline_level >> rampUpCurveBits) < neutral_level) {
            // ramp up voltage smoothly from zero
            baseline_level += (1u << energizeBits);
            mixLevels *= (baseline_level >> rampUpCurveBits);
            mixLevels /= neutral_level;
          }
        } else {
          // if silent, ramp down voltage slowly to zero
          if (baseline_level) baseline_level -= (1u << deEnergizeBits);
            mixLevels = (baseline_level >> rampUpCurveBits);
        }
        ownership = 1;
        for (size_t i = 0; i < pins.size(); ++i) {
          pwm_set_gpio_level(pins[i], (pin_status[pins[i]]) ? mixLevels : 0);
        }
        ownership = -1;
      }

      void begin() {
        cfg = pwm_get_default_config();
        pwm_config_set_clkdiv(&cfg, 1.0f);
        pwm_config_set_wrap(&cfg, (1u << audio_bits) - 2);
        pwm_config_set_phase_correct(&cfg, true);
        for (size_t i = 0; i < pins.size(); ++i) {
          uint8_t p = pins[i];
          uint8_t s = pwm_gpio_to_slice_num(p);
          gpio_set_function(p, GPIO_FUNC_PWM);    // set that pin as PWM
          pwm_init(s, &cfg, true);                // configure and start!
          pwm_set_gpio_level(p, 0);               // initialize at zero to prevent whining sound
        }
        start();
      }
    };

    // core0 uses a queue to determine
    // how to assign voices to new notes
    queue_t open_channel_queue;
    void reset_channel_queue() {
      uint8_t discard;
      while (!queue_is_empty(&open_channel_queue)) {
        queue_try_remove(&open_channel_queue, &discard);
      }
      uint8_t i = 1;
      while (i <= synth_polyphony_limit) {
        bool success = queue_try_add(&open_channel_queue, &i);
        i += (uint8_t)success;
      }

    }
    void initialize_channel_queue() {
      queue_init(&open_channel_queue, sizeof(uint8_t), synth_polyphony_limit);
      reset_channel_queue();
    }
    bool on_callback(struct repeating_timer *t);
    struct repeating_timer timer;
    void background_poll(alarm_pool_t *p, int64_t d) {
      alarm_pool_add_repeating_timer_us(p, d, on_callback, NULL, &timer);
    }
  }
  namespace Keys {
    /* 
    *  This is the background code that collects the
    *  pinout level for each of the key switches on the
    *  HexBoard. This code is run on core1 in the background 
    *  and passes key-press messages to core0 for processing.
    */
    struct Msg {
      uint32_t timestamp;
      uint8_t  switch_number;
      uint8_t  level;
    };
    queue_t msg_queue;
    void initialize_queue(uint count_elements) {
      queue_init(&msg_queue, sizeof(Msg), count_elements);
    }

    class Instance {
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
      Instance(const uint8_t *arrM, const uint8_t *arrC, const bool *arrA)
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
            Msg key_msg_in;
            key_msg_in.timestamp = timer_hw->timerawl;
            key_msg_in.switch_number = index;
            key_msg_in.level = level;
            queue_add_blocking(&msg_queue, &key_msg_in);
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

    bool on_callback(struct repeating_timer *t);
    struct repeating_timer timer;
    void background_poll(alarm_pool_t *p, int64_t d) {
      alarm_pool_add_repeating_timer_us(p, d, on_callback, NULL, &timer);
    }

  }
  namespace Rotary {
    /*
    *  This is the background code that converts pinout data
    *  from the rotary knob into a queue of UI actions.
    *  This code is run on core1 in the background 
    *  and passes action messages to core0 for processing. 
    *
    *  Rotary knob code derived from:
    *      https://github.com/buxtronix/arduino/tree/master/libraries/Rotary
    *  Copyright 2011 Ben Buxton. Licenced under the GNU GPL Version 3.
    *  Contact: bb@cactii.net
    */
    enum class Action {
      no_action, turn_CW,  turn_CCW,
      turn_CW_with_press,  turn_CCW_with_press,
      click, double_click, double_click_release,
      long_press,          long_release
    };

    queue_t act_queue;
    void initialize_queue(uint count_elements) {
      queue_init(&act_queue, sizeof(Action), count_elements);
    }
    
    struct Instance {
    protected:
      bool _active;
      uint8_t _Apin;
      uint8_t _Bpin;
      uint8_t _Cpin;
      bool _invert;                    // if A and B pins were reversed
      uint32_t _longPressThreshold;    // tolerance in microseconds; -1 to ignore
      uint32_t _doubleClickThreshold;  // tolerance in microseconds;  0 to ignore
      uint32_t _debounceThreshold;     // minimum click length in microseconds

    /*
    *  the A/B pins work together to measure turns.
    *  the C pin is a simple button switch
    *
    *                          quadrature encoding
    *    _             __       pin-down sequence
    *   / \ __ pin A     v CW   -,  A,  AB, B,  -
    *  |   |__                  1/1 0/1 0/0 1/0 1/1
    *   \_/    pin B   __^ CCW  -,  B,  BA, A,  -
    *                           1/1 1/0 0/0 0/1 1/1 
    *
    *  When the mechanical rotary knob is turned,
    *  the two pins go through a set sequence of
    *  states during one physical "click", as above.
    *
    *  The neutral state of the knob is 1\1; a turn
    *  is complete when 1\1 is reached again after
    *  passing through all four valid states above,
    *  at which point action should be taken depending
    *  on the direction of the turn.
    *  
    *  The variable "state" captures all this as follows
    *    Value    Meaning
    *    0        Knob is in neutral state (state 0)
    *    1, 2, 3  CCW turn state 1, 2, 3
    *    4, 5, 6   CW turn state 1, 2, 3
    *    8, 16    Completed turn CCW, CW (state 4)
    */
      const uint8_t stateMatrix[7][4] = {
                  // From... To... (0/0) (0/1) (1/0) (1/1)
        {0,4,1,0}, // Neut (1/1)    Fail   CW    CCW  Stall
        {2,0,1,0}, // CCW  (1/0)    Next  Fail  Stall Fail 
        {2,3,1,0}, // CCW  (0/0)    Stall Next  Retry Fail
        {2,3,0,8}, // CCW  (0/1)    Retry Stall Fail  Success
        {5,4,0,0}, //  CW  (0/1)    Next  Stall Fail  Fail
        {5,4,6,0}, //  CW  (0/0)    Stall Retry Next  Fail
        {5,0,6,16} //  CW  (1/0)    Retry Fail  Stall Success
      };
      uint8_t _turnState;
      uint8_t _clickState;

      uint32_t _prevClickTime;
      uint32_t _prevHoldTime;

      bool _doubleClickRegistered;
      bool _longPressRegistered;
      bool _debouncePassed;

      // however, GEM_Menu will set interval in milliseconds.
      void calibrate(bool setInvert, int setLP, int setDC) {
        _invert = setInvert;
        _longPressThreshold = (setLP * 1000) - 1;
        _doubleClickThreshold = (setDC * 1000) - 1;
      }

      uint8_t ownership;

    public:
      Instance(uint8_t Apin, uint8_t Bpin, uint8_t Cpin)
      : _active(false), _Apin(Apin), _Bpin(Bpin), _Cpin(Cpin), _invert(false)
      , _longPressThreshold(default_long_press_timing_ms * 1000)
      , _doubleClickThreshold(default_double_click_timing_ms * 1000)
      , _debounceThreshold(default_debounce_threshold_us) 
      , _turnState(0), _clickState(0)
      , _prevClickTime(0), _prevHoldTime(0)
      , _doubleClickRegistered(false)
      , _longPressRegistered(false)
      , _debouncePassed(false) {
        pinMode(_Apin, INPUT_PULLUP);
        pinMode(_Bpin, INPUT_PULLUP);
        pinMode(_Cpin, INPUT_PULLUP);
      }
      void start() { _active = true;  }
      void stop()  { _active = false; }
      
      // wrapper to safely calibrate knob from core0
      void recalibrate(bool invert_yn, int longPress_mS, int doubleClick_mS) {
        while (ownership == 1) {}
        ownership = 0;
        calibrate(invert_yn, longPress_mS, doubleClick_mS);
        ownership = -1;
      }
      bool getClickState() {
        while (ownership == 1) {}
        ownership = 0;
        bool result = (_clickState & 1);
        ownership = -1;
        return result;
      }
      void writeAction(Action rotary_action_in) {
        queue_add_blocking(&act_queue, &rotary_action_in);
      }
      void poll() {
        if (!_active) return;
        uint8_t A = digitalRead(_Apin);
        uint8_t B = digitalRead(_Bpin);
        while (ownership == 0) {}
        ownership = 1;

        uint8_t getRotation = (_invert ? ((A << 1) | B) : ((B << 1) | A));
        _turnState = stateMatrix[_turnState & 0b00111][getRotation];
        
        uint8_t C = digitalRead(_Cpin);
        _clickState = (0b00011 & ((_clickState << 1) + (C == LOW)));

        if ((_turnState & 0b01000) >> 3) {
          writeAction(C ? Action::turn_CW  : Action::turn_CW_with_press);
        }
        if ((_turnState & 0b10000) >> 4) {
          writeAction(C ? Action::turn_CCW : Action::turn_CCW_with_press);
        }
        uint32_t right_now = timer_hw->timerawl;
        switch (_clickState) { 
          case 0b01: // click down
            _prevHoldTime = right_now;
            _debouncePassed = false;
            break;
          case 0b11: // held
            if (!_debouncePassed) {
              _debouncePassed = (right_now - _prevHoldTime >= _debounceThreshold);
            } else if (  (!_doubleClickRegistered)
                      && (right_now - _prevClickTime <= _doubleClickThreshold)) {
                  writeAction(Action::double_click);            
                  _doubleClickRegistered = true;
            } else if (  (!_longPressRegistered)
                      && (right_now - _prevHoldTime >= _longPressThreshold)) {
                writeAction(Action::long_press);
                _longPressRegistered = true;
            }
            break;
          case 0b10: // click up
            if (_debouncePassed) {
              _prevClickTime = 0;
              if (_longPressRegistered) {
                writeAction(Action::long_release);
              } else if (_doubleClickRegistered) {
                writeAction(Action::double_click_release);
              } else {
                writeAction(Action::click);
                _prevClickTime = _prevHoldTime;
              }
              _doubleClickRegistered = false;
              _longPressRegistered = false;
            }
            _prevHoldTime = 0;
            break;
          default:
            break;
        }
        ownership = -1;
      }
      
      void begin() {
        start();
      }
    };

    bool on_callback(struct repeating_timer *t);
    struct repeating_timer timer;
    void background_poll(alarm_pool_t *p, int64_t d) {
      alarm_pool_add_repeating_timer_us(p, d, on_callback, NULL, &timer);
    }

  }

}