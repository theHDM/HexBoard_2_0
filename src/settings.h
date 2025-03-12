#pragma once
#include <stdint.h> // import common definition of uint8_t
#include <array>
#include <FS.h>

// use a running enum to identify settings by a number.
// this is useful for serializing (converting to bytes for file storage)
// and for processing menu callbacks efficiently (you can cast as integer)
// while also being very legible (settings[_settingName] returns correct array member)
// if you change the list of settings,
// make sure also to change:
// * defaults in this file
// * menu item creation in <menu.h>
// * onChg handlers in <menu.h>
// * applying settings in main

enum {
  // meta fields
  _defaults,_changed, _debug,  
  // master tuning / key center data
  _anchorX, _anchorY, _anchorN, _anchorC, _anchorF,
  _txposeS, _txposeC,
  _axisA,   _axisB,
  _equaveD, _equaveC,
  _tuneSys,
  _eqDivs,  _eqStepA, _eqStepB, _eqScale, _eqChrom,
  _lgSteps, _smSteps, _lgStepA, _smStepA, _lgStepB, _smStepB,
  _lgToSmOp,_lgToSmND,_lgToSmR, _lgToSmN, _lgToSmD, _modeLgSm,
  _JInumA,  _JIdenA,  _JInumB,  _JIdenB,  
  _scaleLck,_animFPS, _palette, _animType,_globlBrt,_hueLoop,
  _hue_0,   _sat_0,   _val_0,
  _hue_1,   _sat_1,   _val_1,   _hue_n1,  _sat_n1,  _val_n1,
  _hue_2,   _sat_2,   _val_2,   _hue_n2,  _sat_n2,  _val_n2,
  _hue_3,   _sat_3,   _val_3,   _hue_n3,  _sat_n3,  _val_n3,
  _tglWheel,_whlMode, 
  _mdSticky,_pbSticky,_vlSticky, 
  _mdSpeed, _pbSpeed, _vlSpeed,  
  _rotInv,  _rotDblCk,_rotLongP, 
  _SStime,   //
  _MIDImode,_MIDIusb, _MIDIjack,
  _MPEzoneC,_MPEzoneL,_MPEzoneR,_MPEpb,    //
  _MIDIorMT,_MIDIpc,  _MT32pc,
  _synthTyp,_synthWav,_synthEnv, //
  _synthVol,_synthBuz,_synthJac, //
  _settingSize // the largest index plus one 
};

// use regular enum, not enum class, to identify options.
// we need to be able to cast these to integers
// for the menu system to work.
enum {
  _tuneSys_normal,
  _tuneSys_equal,
  _tuneSys_lg_sm,
  _tuneSys_just
};
enum {
  _palette_rainbow,
  _palette_tiered,
  _palette_alternate
};
enum {
  _animType_none,
  _animType_star,
  _animType_splash,
  _animType_orbit,
  _animType_octave,
  _animType_by_note,
  _animType_beams,
  _animType_splash_reverse,
  _animType_star_reverse
};
enum {
  _globlBrt_max = 255,
  _globlBrt_high = 210,
  _globlBrt_mid = 180,
  _globlBrt_low = 150,
  _globlBrt_dim = 110,
  _globlBrt_dimmer = 70,
  _globlBrt_off = 0
};
enum {
  _MIDImode_standard,
  _MIDImode_MPE,
  _MIDImode_tuning_table,
  _MIDImode_2_point_oh
};
enum {
  _synthTyp_off,
  _synthTyp_mono,
  _synthTyp_arpeggio,
  _synthTyp_poly
};

enum {
  _GM_instruments,
  _MT32_instruments
};

// this is a simple kind of "variant" type.
// it holds the bit-wise representation of
// a numerical value, which you can then express
// as its associated boolean/int/float value at will
const size_t bytes_per_setting = 8;

union Setting_Value {
  bool b;
  int i;
  double d;
  std::array<uint8_t, bytes_per_setting> w;
};
using hexBoard_Setting_Array = std::array<Setting_Value, _settingSize>;

void save_settings(hexBoard_Setting_Array& refS, File& F) {
  for (size_t s = 0; s < _settingSize; ++s) {
    for (size_t b = 0; b < bytes_per_setting; ++b) {
      F.write((refS[s].w)[b]);
    }
  }
}

void load_settings(hexBoard_Setting_Array& refS, File& F) {
  for (size_t s = 0; s < _settingSize; ++s) {
    for (size_t b = 0; b < bytes_per_setting; ++b) {
      if (F.available()) (refS[s].w)[b] = F.read();
    }
  }
}

void load_factory_defaults_to(hexBoard_Setting_Array& refS, int version = 0) {
  // clear settings
  for (auto& each_setting : refS) {
    for (int each_byte = 0; each_byte < bytes_per_setting; ++each_byte) {
      each_setting.w[each_byte] = 0x00;  // reset values to zero
    }
  }
  refS[_defaults].b = true;   // if you are loading factory defaults, set to true
  refS[_changed].b  = true;   // flag as true so that it will save at first oppt'y
  refS[_debug].b    = false;   // send debug messages through Serial monitor

  refS[_anchorX].i  = -2;      // hex at {-2,0} is the root note
  refS[_anchorY].i  = 0;      // hex at {-2,0} is the root note
  refS[_anchorN].i  = 60;     // default to C4
  refS[_anchorC].i  = 0;      // default to no microtones
  refS[_anchorF].i  = 0;      // default to no microtones

  refS[_txposeS].i  = 0;      // default no transposing
  refS[_txposeC].d  = 100.0;  // shift by 100 cents each transpose
  refS[_axisA].i    = 3;      // axis A is left
  refS[_axisB].i    = 1;      // axis B is up-right
  refS[_equaveD].i  = 0;      // dropdown
  refS[_equaveC].d  = 1200.0; // 1200 cents = octave
  refS[_tuneSys].i  = 0;      // Equal divisions
  refS[_eqDivs].i   = 12;     // 12 steps
  refS[_eqStepA].i  = -2;     // one whole tone
  refS[_eqStepB].i  = 7;      // perfect fifth

  refS[_eqScale].i  = 0;      // major
  refS[_eqChrom].i  = 0;      // 

  refS[_lgSteps].i  = 5;      // five whole tones
  refS[_smSteps].i  = 2;      // two semitones
  refS[_lgStepA].i  = -1;     // one whole tone 
  refS[_smStepA].i  = 0;      // no semitones
  refS[_lgStepB].i  = 3;      // three whole tones
  refS[_smStepB].i  = 1;      // and one semitone
  refS[_lgToSmOp].i = 0x21;   // whole tone is 2x the semitone
  refS[_lgToSmND].b = true;   // 
  refS[_lgToSmR].d  = 2.0;    // whole tone is 2x the semitone
  refS[_lgToSmN].i  = 2;      // whole tone is 2x the semitone
  refS[_lgToSmD].i  = 1;      // whole tone is 2x the semitone
  refS[_modeLgSm].i = 1;      // mode 1 = major, mode 0 = lydian
  refS[_JInumA].i   = 8;      // 9/8 = JI major second
  refS[_JIdenA].i   = 9;      // 9/8 = JI major second
  refS[_JInumB].i   = 5;      // 5/4 = JI major third
  refS[_JIdenB].i   = 4;      // 5/4 = JI major third
  refS[_scaleLck].b = false;  // default to no scale lock
  refS[_animFPS].i  = 32;     // default to 32 frames per 1.048576 seconds
  refS[_palette].i  = 0;
  refS[_animType].i = 0;
  refS[_globlBrt].i = (version >= 12 ? _globlBrt_dim : _globlBrt_mid);
  refS[_hueLoop].d  = 30.0;  // seconds for 360 degrees 
  refS[_tglWheel].b = false; // bool; 0 = mod, 1 = pb
  refS[_whlMode].b  = false; // standard vs. fine tune mode
  refS[_mdSticky].b = false;
  refS[_pbSticky].b = false;
  refS[_vlSticky].b = false;
  refS[_mdSpeed].i  = 8;
  refS[_pbSpeed].i  = 8; // scale by x128
  refS[_vlSpeed].i  = 8;		
  refS[_rotInv].b   = !(version >= 12 ? true : false);
  refS[_rotDblCk].i = 500; // milliseconds
  refS[_rotLongP].i = 750; // milliseconds
  refS[_SStime].i   = 10; // seconds
  refS[_MIDImode].i = 0;
  refS[_MIDIusb].b  = true;
  refS[_MIDIjack].b = (version >= 12);
  refS[_MPEzoneC].i = 2;
  refS[_MPEzoneL].i = 9;
  refS[_MPEzoneR].i = 11;
  refS[_MPEpb].i    = 48; // 2, 12, 24, 48, or 96
  refS[_MIDIorMT].i = 0;
  refS[_MIDIpc].i   = 1; // program chg 1 - 128
  refS[_MT32pc].i   = 1;
  refS[_synthTyp].i = 0;
  refS[_synthWav].i = 0;
  refS[_synthEnv].i = 0;
  refS[_synthVol].i = 96;
  refS[_synthBuz].b = false;
  refS[_synthJac].b = true || (version >= 12);
}

hexBoard_Setting_Array settings;