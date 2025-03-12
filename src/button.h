#pragma once
#include <stdint.h>
#include <map>
#include <array>
#include <variant>
#include "settings.h"
#include "hexagon.h"
#include "music.h"
#include <FS.h>

struct Hardwire_Switch {
  // hardware defined, static
  size_t  pinID;
  // volatile
  uint8_t state;
};

const size_t bytes_per_btn = 32; // just in case

struct Physical_Button {
  // user-defined
  uint32_t color;   // rgb hex value
  int8_t   equave;  // not used for JI lattice
  int8_t   degree;  // for 1-dimension
  uint8_t  channel; // what channel assigned (if not MPE mode)   [1..16]
  uint8_t  note;    // assigned MIDI note (if MTS mode) [0..127]
  double   pitch;   // pitch, 69 = A440, every 1.0 is 100.0 cents
  uint8_t  param;   // control parameter corresponding to this hex

  // hardware defined, static
  size_t   pixel; // associated pixel
  size_t   pinID; // linear index of muxPin/colPin
  Hex      coord; // physical location

  // cached
  double    frequency; // equivalent of pitch in Hz
  uint32_t  LEDcodeBase = 0; // calculate it once and store value, to make LED playback snappier 
  uint32_t  LEDcodeAnim = 0; // calculate it once and store value, to make LED playback snappier 
  uint32_t  LEDcodePlay = 0; // calculate it once and store value, to make LED playback snappier
  uint32_t  LEDcodeRest = 0; // calculate it once and store value, to make LED playback snappier
  uint32_t  LEDcodeOff  = 0; // calculate it once and store value, to make LED playback snappier
  uint32_t  LEDcodeDim  = 0; // calculate it once and store value, to make LED playback snappier

  // volatile
  uint32_t  timeLastUpdate = 0; // store time that key level was last updated
  uint32_t  timePressBegan = 0; // store time that full press occurred
  uint32_t  timeHeldSince  = 0;
  uint8_t   pressure       = 0; // press level currently
  uint8_t   velocity       = 0; // proxy for velocity
  bool      just_pressed   = false;
  bool      just_released  = false;
  uint8_t   midiChPlaying  = 0;         // what midi channel is there currrently a note-on
  uint8_t   midiNote = 0;               // nearest MIDI pitch, 0 to 128
  int16_t   midiBend = 0;               // pitch bend for MPE purposes
  uint8_t   synthChPlaying = 0;         // what synth channel is there currrently a note-on

  // void serialize()

  // member functions
  void update_levels(uint32_t& timestamp, uint8_t& new_level) {
    if (pressure == new_level) return;
    timeLastUpdate = timestamp;
    if (new_level == 0) {
      just_released = true;
      velocity = 0;
      timeHeldSince = 0;
    } else if (new_level >= 127) {
      just_pressed = true;
      velocity = 127;
      // velocity = function of timeLastUpdate - timePressBegan;
      // need a velocity curve, eventually. this is ignored in v1.2.
      timePressBegan = 0;
      timeHeldSince = timeLastUpdate;
    } else if (timePressBegan == 0) {
      timePressBegan = timeLastUpdate;
    }
    pressure = new_level;
  }

  bool check_and_reset_just_pressed() {
    bool result = just_pressed;
    just_pressed = false;
    return result;
  }
  
  bool check_and_reset_just_released() {
    bool result = just_released;
    just_released = false;
    return result;
  }

};

struct Button_Grid {
  std::array<Physical_Button, buttons_count> btn;
  std::array<Hardwire_Switch, hardwire_count> dip;
  std::array<Physical_Button*, keys_count>  btn_at_index;
  std::array<Hardwire_Switch*, keys_count>  dip_at_index;
  std::map<Hex, Physical_Button*>           btn_by_coord;

  Button_Grid(const int def[buttons_count][4]) {
    for (auto& ptr : btn_at_index) {ptr = nullptr;}
    for (auto& ptr : dip_at_index) {ptr = nullptr;}

    for (size_t pxl = 0; pxl < buttons_count; ++pxl) {
      Hex x(def[pxl][0],def[pxl][1]);
      size_t i = linear_index(def[pxl][2], def[pxl][3]);
      btn[pxl].pixel = pxl;
      btn[pxl].pinID = i;
      btn[pxl].coord = x;
      btn_at_index[i] = &(btn[pxl]);
      btn_by_coord[x] = &(btn[pxl]);
    }
    size_t h = 0;
    for (size_t k = 0; k < keys_count; ++k) {
      if (btn_at_index[k] == nullptr) {
        dip[h].pinID = k;
        dip_at_index[k] = &(dip[h]);
        if (++h == hardwire_count) break;
      }
    }
  }

  bool in_bounds(const Hex& coord) {
    return (btn_by_coord.find(coord) != btn_by_coord.end());
  }
};


void save_layout(hexBoard_Setting_Array& refS, File& F) {
  for (size_t s = 0; s < _settingSize; ++s) {
    // serialize
    for (size_t b = 0; b < bytes_per_setting; ++b) {
      F.write((refS[s].w)[b]);
    }
  }
}

void load_layout(hexBoard_Setting_Array& refS, File& F) {
  for (size_t s = 0; s < _settingSize; ++s) {
    for (size_t b = 0; b < bytes_per_setting; ++b) {
      if (F.available()) (refS[s].w)[b] = F.read();
    }
    // unserialize
  }
}