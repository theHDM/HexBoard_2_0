#pragma once
/* 
 *  This is the background code for direct digital synthesis
 *  of synthesizer sounds for the HexBoard.
 *  This code is run on core1 in the background, calculates
 *  one audio sample every polling period and sends to
 *  designated pins via PWM. When core1 is inactive, core0
 *  may update the object with music messages such as note-on,
 *  waveform change, mod wheel, volume, pressure, etc.
 */
#include <stdint.h>
#include <cmath>
#include <array>
#include <vector>
#include <functional>
#include <algorithm> // std::find

#include "hardware/pwm.h"       // library of code to access the processor's built in pulse wave modulation features
#include "pico/util/queue.h"
#include "pico/time.h"
#include "config.h" // import hardware config constants

enum class ADSR_Phase {
  off, attack, decay, sustain, release
};

struct Synth_Voice {
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

struct hexBoard_Synth_Object {
  bool active;
  std::array<Synth_Voice, synth_polyphony_limit> voice;
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

  hexBoard_Synth_Object(const uint8_t* pins, size_t count)
  : active(false) {
    for (size_t i = 0; i < GPIO_pin_count; ++i) {
      pin_status[i] = false;
    }
    for (size_t i = 0; i < count; ++i) {
      internal_set_pin(pins[i], true);
    }
  }
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
queue_t open_synth_channel_queue;
void reset_synth_channel_queue() {
  uint8_t discard;
  while (!queue_is_empty(&open_synth_channel_queue)) {
    queue_try_remove(&open_synth_channel_queue, &discard);
  }
  uint8_t i = 1;
  while (i <= synth_polyphony_limit) {
    bool success = queue_try_add(&open_synth_channel_queue, &i);
    i += (uint8_t)success;
  }

}
void initialize_synth_channel_queue() {
  queue_init(&open_synth_channel_queue, sizeof(uint8_t), synth_polyphony_limit);
  reset_synth_channel_queue();
}