#pragma once
/* 
 *  Functions to create and manipulate wavetables passed to the hardware synth
 */
#include <stdint.h>
#include <cmath>
#include <array>
#include "config.h" // import hardware config constants

using wave_tbl = std::array<int8_t, 256>;

// Linear waveforms such as square, saw, and triangle waves
// can be generalized in the form A,B,C,D, as follows
//
// 255|   B ******* C       from sample 0 to A,   level is 0
//    |     *      *        from sample A to B,   level rises
// lvl|    *        *       from sample B to C,   level is 255
//    |    *         *      from sample C to D,   level falls
// 0  |**** A       D ***** from sample D to 255, level is 0
//    ---------------------  rising from A to B has a slope of AB
//    0     sample      255 falling from C to D has a slope of CD

const float f_hyb_square   =  220.f;
const float f_hyb_saw_low  =  440.f;
const float f_hyb_saw_high =  880.f;
const float f_hyb_triangle = 1760.f;

enum {
  _synthWav_hybrid,
  _synthWav_square,
  _synthWav_saw,
  _synthWav_triangle,

  _synthWav_sine,
  _synthWav_strings,
  _synthWav_clarinet
};
enum {
  _synthEnv_none,
  _synthEnv_hit,
  _synthEnv_pluck,
  _synthEnv_strum,
  _synthEnv_slow,
  _synthEnv_reverse
};

enum class Linear_Wave {square, saw, triangle, hybrid};

wave_tbl linear_waveform(double _f, Linear_Wave _shp, uint8_t _mod) {
  Linear_Wave m_shp = _shp;
  float t_pct = 0.f;
  if (_shp == Linear_Wave::hybrid) {
    if (_f < f_hyb_saw_low) {
      m_shp = Linear_Wave::square;
      if (_f > f_hyb_square) {
        t_pct =  (_f - f_hyb_square) / (f_hyb_saw_low - f_hyb_square);
      }
    } else if (_f > f_hyb_saw_high) {
      m_shp = Linear_Wave::triangle;
      if (_f < f_hyb_triangle) {
        t_pct = (f_hyb_triangle - _f) / (f_hyb_triangle - f_hyb_saw_high);
      }
    } else {
      m_shp = Linear_Wave::saw;
    }
  }
  uint8_t d_cyc = 127 - _mod;
  uint8_t a;
  uint8_t b;
  uint8_t c;
  uint8_t d;  
  switch (m_shp) {
    case Linear_Wave::square: {
      a = (d_cyc * (1.f - t_pct)) - 1;
      b = (d_cyc * (1.f + t_pct));
      c = (d_cyc << 1) - 1;
      d = c + 1;
      break;
    }
    case Linear_Wave::saw: {
      a =  0;
      b = (d_cyc << 1) - 1;
      c = (d_cyc << 1) - 1;
      d = c + 1;
      break;
    }
    case Linear_Wave::triangle: {
      a =  0;
      b =  d_cyc * (2.f - t_pct);
      c =  b;
      d = (d_cyc << 1);
      break;
    }
  }
  wave_tbl result;
  for (uint8_t i = 0; i <= a; ++i) {
    result[i] = -127;
  }
  if (a < b - 1) {
    for (uint8_t i = a + 1; i <= b - 1; ++i) {
      result[i] = (((i - a) * ((254 << 8) / (b - 1 - a))) >> 8) - 127;
    }
  }
  for (uint8_t i = b; i <= c; ++i) {
    result[i] = 127;
  }
  if (c < d - 1) {
    for (uint8_t i = c + 1; i <= d - 1; ++i) {
      result[i] = (((d - i) * ((254 << 8) / (d - 1 - c))) >> 8) - 127;
    }
  }
  return result;
}

// calculated ahead of time by core0 for instruments made of harmonics
// pass arrays containing the amount of each harmonic and the phase shift.
// amt is from 0.0 - 1.0. phase shift is in multiples of 2pi, 
// so 0.5 is negative sine, 0.25 is cosine, etc.
const float two_pi = 6.28318531;
wave_tbl additive_synthesis(size_t harmonicLimit, const float* amt, const float* phase) {
  float raw[256];
  float normalize = 1.f;
  for (size_t i = 0; i < 256; ++i) {
    raw[i] = 0.f;
    for (size_t h = 0; h < harmonicLimit; ++h) {
      raw[i] += amt[h] * std::sin(two_pi * (h + 1) * (phase[h] + i / 256.f));
    }
    if (std::abs(raw[i]) > normalize) {
      normalize = std::abs(raw[i]);
    }
  }
  normalize = 127.f / normalize;
  wave_tbl result;
  for (size_t i = 0; i < 256; ++i) {
    result[i] = round(raw[i] * normalize);
  }
  return result;
}

// harmonic wave presets
const float sineAmt[1]   = {1.f};
const float sinePhase[1] = {0.f};
const float stringsAmt[10]   = {
  0.995f, 0.94f, 0.425f, 0.48f, 0.f, 0.365f, 0.04f, 0.085f, 0.0f, 0.09f
};
const float stringsPhase[10] = {
  0.f,    0.25f, 0.f,    0.25f, 0.f, 0.25f,  0.f,   0.25f,  0.f,  0.25f
};
const float clarinetAmt[11]   = {
  1.f, 0.f, 0.333f, 0.f, 0.2f, 0.f, 0.143f, 0.f, 0.111f, 0.f, 0.909f
};
const float clarinetPhase[11] = {
  0.f, 0.f, 0.f,    0.f, 0.f,  0.f, 0.f,    0.f, 0.f,    0.f, 0.f
};

// if in the future you use a custom wave table of samples,
// this function could be useful.
//void set_cached_wavetable(const wave_tbl& inputWaveTbl) {
//  for (uint8_t i = 0; i < 256; ++i) {
//    cached_waveform[i] = inputWaveTbl[i];
//  }
//}

void pre_cache_synth_waveform(const int& selection, wave_tbl& cache) {
  switch (selection) {
    case _synthWav_square:
      cache = linear_waveform(1.f, Linear_Wave::square, 0);
      break;
    case _synthWav_saw:
      cache = linear_waveform(1.f, Linear_Wave::saw, 0);
      break;
    case _synthWav_triangle:
      cache = linear_waveform(1.f, Linear_Wave::triangle, 0);
      break;
    case _synthWav_sine:
      cache = additive_synthesis(1, sineAmt, sinePhase);
      break;
    case _synthWav_strings:
      cache = additive_synthesis(10, stringsAmt, stringsPhase);
      break;
    case _synthWav_clarinet:
      cache = additive_synthesis(11, clarinetAmt, clarinetPhase);
      break;
    default:
      break;
  } 
}

uint32_t frequency_to_interval(
  // determining the DDS step interval based on frequency
  double   frequency, 
  uint32_t interval_in_uS) {
  return lround(ldexp(frequency * interval_in_uS / 1000000.d, 32));
}

uint8_t iso226(double f) {
  // a very crude implementation of ISO 226 equal loudness curves
  //   Hz dB  Amplitude ~ sqrt(10^(dB/10))
  //  200 +0  255
  //  800 -3  191
  // 1500 +0  255
  // 3250 -6  127
  // 5000 +0  255
  if (f <      8.0) return 0;
  if (f <    200.0) return 255;
  if (f <   1500.0) return 191 + ldexp(abs(f- 800) /  700.d, 6);
  if (f <   5000.0) return 127 + ldexp(abs(f-3250) / 1750.d, 7);
  if (f < highest_MIDI_note_Hz) return 255;
  return 0;
}

double intervalToCents(double interval) {
  return 1200.0 * log2(interval);
}


double frequency_after_pitch_bend(
  // to apply global pitch bend to synth channels
  double  base_frequency, 
  int16_t global_pitch_bend, 
  uint8_t pitch_bend_range_in_semitones) {
  return base_frequency 
       * exp2(ldexp(global_pitch_bend 
       * pitch_bend_range_in_semitones / 3.d, 
        -15));
}