/*  
 *  HexBoard v2.0
 */
enum class App_state {  // OLED             LED         Audio        Keys       Knob
  setup,                // splash/status    off         off          off        off
  play_mode,            // full GUI         musical     active       play       control, hold for menu
  menu_nav,             // menu             musical     active       play       menu, hold at home page to escape
  hex_picker,
  color_picker,
  sequencer,      // TBD
  calibrate,            // TBD
  data_mgmt,            // TBD
  crash,                // display error    off         off          off        TBD
  low_power,
};
App_state current_state = App_state::setup;

#include <stdint.h>
#include "pico/time.h"
#include "src/config.h"
#include "src/debug.h"
#include "src/settings.h"
#include "src/file_system.h"
volatile int successes = 0;
volatile int screenupdate = 0;
const char* settingFileName = "temp222.dat";

#include "src/color.h"
#include "src/synth.h"
hexBoard_Synth_Object  synth(synthPins, 2);
#include "src/rotary.h"
hexBoard_Rotary_Object rotary(rotaryPinA, rotaryPinB, rotaryPinC);
#include "src/keys.h"
hexBoard_Key_Object    keys(muxPins, colPins, analogPins);

#include "src/music.h"  // microtonal and MIDI math
#include "src/MIDI_api.h"
#include "src/LED.h"
#include "src/OLED.h"
#include "src/button.h"

OLED_screensaver       oled_screensaver(default_contrast, screensaver_contrast);
Button_Grid            hexBoard(hexBoard_layout_hw_1_2);

#include "src/menu.h"
#include "src/GUI.h"
//#include "src/layout.h"

bool on_callback_synth(struct repeating_timer *t)  { synth.poll();  return true; }
bool on_callback_rotary(struct repeating_timer *t) { rotary.poll(); return true; }
bool on_callback_keys(struct repeating_timer *t)   { keys.poll();   return true; }
void start_background_processes() {
  alarm_pool_t *core1pool;
  core1pool = alarm_pool_create(1, 4);
  
  synth.begin();
  struct repeating_timer timer_synth;
  // enter a negative timer value here because the poll
  // should occur X microseconds after the routine starts
  alarm_pool_add_repeating_timer_us(core1pool, 
    -audio_sample_interval_uS, on_callback_synth, 
    NULL, &timer_synth);

  rotary.begin();
  struct repeating_timer timer_rotary;
  // enter a positive timer value here because the poll
  // should occur X microseconds after the routine finishes
  alarm_pool_add_repeating_timer_us(core1pool,
    rotary_poll_interval_uS, on_callback_rotary,
    NULL, &timer_rotary);

  keys.begin();
  struct repeating_timer timer_keys;
  // enter a positive timer value here because the poll
  // should occur X microseconds after the routine finishes
  alarm_pool_add_repeating_timer_us(core1pool, 
    key_poll_interval_uS, on_callback_keys, 
    NULL, &timer_keys);

  while (1) {}
  // once these objects are run, then core_1 will
  // run background processes only
}

void link_settings_to_objects(hexBoard_Setting_Array& refS) {
  debug.setStatus(&refS[_debug].b);
  oled_screensaver.setDelay(&refS[_SStime].i);
}
void set_audio_outs_from_settings(hexBoard_Setting_Array& refS) {
  synth.set_pin(piezoPin, refS[_synthBuz].b);
  synth.set_pin(audioJackPin, refS[_synthJac].b);
}
void calibrate_rotary_from_settings(hexBoard_Setting_Array& refS) {
  rotary.recalibrate(refS[_rotInv].b, refS[_rotLongP].i, refS[_rotDblCk].i);
}
void pre_cache_synth_waveform(hexBoard_Setting_Array& refS) {
  switch (refS[_synthWav].i) {
    case _synthWav_square:
      cached_waveform = linear_waveform(1.f, Linear_Wave::square, 0);
      break;
    case _synthWav_saw:
      cached_waveform = linear_waveform(1.f, Linear_Wave::saw, 0);
      break;
    case _synthWav_triangle:
      cached_waveform = linear_waveform(1.f, Linear_Wave::triangle, 0);
      break;
    case _synthWav_sine:
      cached_waveform = additive_synthesis(1, sineAmt, sinePhase);
      break;
    case _synthWav_strings:
      cached_waveform = additive_synthesis(10, stringsAmt, stringsPhase);
      break;
    case _synthWav_clarinet:
      cached_waveform = additive_synthesis(11, clarinetAmt, clarinetPhase);
      break;
    default:
      break;
  } 
}
void apply_settings_to_objects(hexBoard_Setting_Array& refS) {
  set_audio_outs_from_settings(refS);
  calibrate_rotary_from_settings(refS);
  pre_cache_synth_waveform(refS); 
  //generate_layout(refS);
}
void menu_handler(int settingNumber) {
  switch (settingNumber) {
    case _on_generate_layout:
      //generate_layout(settings);
      menu.setMenuPageCurrent(pgHome);
      break;
    
    case _txposeS: case _txposeC:
      // recalculate pitches for everyone
      break;
    case _scaleLck:
      // set scale lock as appropriate
      break;
    case _rotInv: case _rotDblCk: case _rotLongP:
      calibrate_rotary_from_settings(settings);
      break;
    case _MIDIusb: case _MIDIjack:
      // turn MIDI jacks on/off
      break;    
    case _synthBuz: case _synthJac:
      set_audio_outs_from_settings(settings);
      break;    
    case _MIDIpc: case _MT32pc:
      // send program change
      break;
    case _synthWav:
      pre_cache_synth_waveform(settings); 
      // precalculate active waveform and cache?
      break;
    /*
    _animFPS,  //
    _palette,  //
    _animType, //
    _globlBrt, //
    _hueLoop,  //
    _tglWheel, //
    _whlMode,  //
    _mdSticky, //
    _pbSticky, //
    _vlSticky, //
    _mdSpeed,  //
    _pbSpeed,  //
    _vlSpeed,  //
    _MIDImode, //
    _MPEzoneC, //
    _MPEzoneL, //
    _MPEzoneR, //
    _MPEpb,    //
    _synthTyp, //
    */
    default: 
      break;
  }
}

void hardwired_switch_handler(const Hardwire_Switch& h) {
  if (h.pinID == linear_index(14,0)) {
    if (settings[_defaults].b) { // if you have not loaded existing settings
      load_factory_defaults_to(settings, 12); // replace with v1.2 firmware defaults
    }
  }
}

void note_on(Physical_Button& n) {
  // synth note-on
  if (!queue_is_empty(&open_synth_channel_queue)) {
    queue_remove_blocking(
      &open_synth_channel_queue, 
      &(n.synthChPlaying)
    );
    Synth_Voice *v = &synth.voice[(n.synthChPlaying) - 1];
    double adj_f = frequency_after_pitch_bend(n.frequency, 0 /*pb*/, 2 /*pb range*/);
    v->update_pitch(frequency_to_interval(adj_f, audio_sample_interval_uS));
    if    ((settings[_synthWav].i == _synthWav_hybrid) || ( false /*mod wheel > 0*/
      &&  ((settings[_synthWav].i == _synthWav_square) 
        || (settings[_synthWav].i == _synthWav_saw)
        || (settings[_synthWav].i == _synthWav_triangle)
    ))) {
      v->update_wavetable(linear_waveform(adj_f, Linear_Wave::hybrid, 0 /*mod wheel value*/));
    } else {
      v->update_wavetable(cached_waveform);
    }
    v->update_base_volume((settings[_synthVol].i * n.velocity * iso226(adj_f)) >> 15);
    switch (settings[_synthEnv].i) { // attack ms, decay ms, sustain 0-255, release ms
      case _synthEnv_hit:     v->update_envelope(  20,   50, 128,  100); break;
      case _synthEnv_pluck:   v->update_envelope(  20, 1000,  24,  100); break;
      case _synthEnv_strum:   v->update_envelope(  50, 2000, 128,  500); break;
      case _synthEnv_slow:    v->update_envelope(1000,    0, 255, 1000); break;
      case _synthEnv_reverse: v->update_envelope(2000,    0,   0,    0); break;      
      default:                v->update_envelope(   0,    0, 255,    0); break;
    }
    v->note_on();
  }
  // MIDI_api.noteOn(69,96,1);

}

void note_off(Physical_Button& n) {
  // synth note-off
  if (n.synthChPlaying) {
    synth.voice[(n.synthChPlaying) - 1].note_off();
    if (!queue_is_full(&open_synth_channel_queue)) {
      queue_add_blocking(
        &open_synth_channel_queue, 
        &(n.synthChPlaying)
      );
    }
    n.synthChPlaying = 0;
  }
  // MIDI note-off
}


void interpret_key_msg(Key_Msg& msg) {
  if (hexBoard.btn_at_index[msg.switch_number] == nullptr) {
    if (hexBoard.dip_at_index[msg.switch_number] == nullptr) return;
    Hardwire_Switch* h = hexBoard.dip_at_index[msg.switch_number];
    h->state = msg.level;
    hardwired_switch_handler(*h);
    return;
  }
  Physical_Button* b = hexBoard.btn_at_index[msg.switch_number];
  b->update_levels(msg.timestamp, msg.level);
  if (b->check_and_reset_just_pressed()) {
    switch (current_state) {
      case App_state::play_mode:
      case App_state::menu_nav: {
        note_on(*b);
        break;
      }
      case App_state::hex_picker: {
        settings[_anchorX].i = b->coord.x;
        settings[_anchorY].i = b->coord.y;
        break;
      }
      case App_state::color_picker: {
        // hard coding for now; TO-DO make this less hard-coded lol
        if (b->coord.y == -6) {
          switch (b->coord.x) {
            case -4: settings[_hue_0].d = _hueY; break;
            case -2: settings[_hue_0].d = _hueC; break;
            case  0: settings[_hue_0].d = _hueG; break;
            case  2: settings[_hue_0].d = _hueM; break;
            case  4: settings[_hue_0].d = _hueR; break;
            case  6: settings[_hue_0].d = _hueB; break;
          }
        } else if (b->coord.y == -4) {
          settings[_hue_0].d += 3.0 * b->coord.x;
        }
        break;
      }
      default: break;
    }
  } else if (b->check_and_reset_just_released()) {
    switch (current_state) {
      case App_state::play_mode:
      case App_state::menu_nav:
      case App_state::hex_picker:
      case App_state::color_picker: {
        // note-off, also allow in other modes so that
        // notes don't remain stuch on state transition
        note_off(*b);
        break;
      }
      default: break;
    }
  } else if (b->pressure) {
    switch (current_state) {
      // if you have pressure sensitive buttons
      // you can add code here to send
      // expression data to MIDI and/or synth.
      default: break;
    }
  }
}

void interpret_rotary_msg(const Rotary_Action& act) {
  // while dealing with rotary, halt OLED auto-update
  screenupdate = true;
  switch (act) {
    case Rotary_Action::turn_CW:
    case Rotary_Action::turn_CW_with_press: {
      if (current_state != App_state::menu_nav) { 
        ++settings[_globlBrt].i;
      } else {
        menu.registerKeyPress(GEM_KEY_DOWN);
      }
      break;
    }
    case Rotary_Action::turn_CCW:
    case Rotary_Action::turn_CCW_with_press: {
      if (current_state != App_state::menu_nav) {
        --settings[_globlBrt].i;
      } else {
        menu.registerKeyPress(GEM_KEY_UP);
      }
      break;
    }
    case Rotary_Action::click: {
      if (current_state != App_state::menu_nav) {
        //
      } else if (menu_app_state() >= 2) {
        menu.registerKeyPress(GEM_KEY_RIGHT);       
      } else {
        menu.registerKeyPress(GEM_KEY_OK);       
      }
      break;
    }
    case Rotary_Action::double_click: {
      if (current_state != App_state::menu_nav) {
        //
      } else if (menu_app_state() >= 2) {
        menu.registerKeyPress(GEM_KEY_LEFT);       
      } else {
        //
      }
      break;
    }
    case Rotary_Action::double_click_release: {
      if (current_state != App_state::menu_nav) {
        //
      } else if (menu_app_state() >= 2) {
        menu.registerKeyPress(GEM_KEY_LEFT);       
      } else {
        menu.registerKeyPress(GEM_KEY_OK);       
      }
      break;
    }
    case Rotary_Action::long_press: {
      if (current_state != App_state::menu_nav) {
        menu.setMenuPageCurrent(pgHome);
        current_state = App_state::menu_nav;
      } else if (menu_app_state() >= 2) {
        menu.registerKeyPress(GEM_KEY_OK);
      } else if (menu.getCurrentMenuPage() == &pgHome) {
        menu.setMenuPageCurrent(pgNoMenu);
        current_state = App_state::play_mode;
      } else {
        menu.registerKeyPress(GEM_KEY_CANCEL);
      }
      break;
    }
    case Rotary_Action::long_release: break;
    default:                          break;
  }
  screenupdate = false;
}

void color_this_hex(const Hex& h, const HSV& c) {
  strip.setPixelColor(
    hexBoard.btn_by_coord.at(h)->pixel, 
    okhsv_to_neopixel_code(c)
  );
}

struct repeating_timer polling_timer_LED;
bool on_LED_frame_refresh(repeating_timer *t) {
  switch (current_state) {
    case App_state::play_mode:
    case App_state::menu_nav: {
      for (auto& b : hexBoard.btn) {
        strip.setPixelColor(b.pixel, b.LEDcodeBase);
      }
      break;
    }
  }
  strip.show();
  return true;
}

struct repeating_timer polling_timer_OLED;
bool on_OLED_frame_refresh(repeating_timer *t) {
  switch (current_state) {
    case App_state::menu_nav:
      if (screenupdate) break;
      menu.drawMenu(); // when menu is active, call GUI update through menu refresh
      oled_screensaver.jiggle();
      break;
    case App_state::play_mode:
    case App_state::color_picker:
      u8g2.clearBuffer();
      GUI.draw();
      u8g2.sendBuffer();
      break;
    default:
      break;
  }
  return true;
}

struct repeating_timer polling_timer_debug;
bool on_debug_refresh(repeating_timer *t) {
  //debug.add_num(successes);
  //debug.add("\n");
  debug.send();
  return true;
}

void setup() {
  queue_init(&key_press_queue, sizeof(Key_Msg), keys_count + 1);
  queue_init(&rotary_action_queue,  sizeof(Rotary_Action),  32);
  load_factory_defaults_to(settings);
  link_settings_to_objects(settings);
  mount_tinyUSB();
  connect_OLED_display(OLED_sdaPin, OLED_sclPin);
  connect_neoPixels(ledPin, buttons_count);
  fileSystemExists = mount_file_system(true);

  //if (!load_settings(settings, settingFileName)) {
    // if load fails?
  //}  
  apply_settings_to_objects(settings);
  init_MIDI();
  initialize_synth_channel_queue();
  menu_setup();
  add_repeating_timer_ms(
    LED_poll_interval_mS, 
    on_LED_frame_refresh, NULL, 
    &polling_timer_LED
  );
  add_repeating_timer_ms(
    OLED_poll_interval_mS,
    on_OLED_frame_refresh, NULL, 
    &polling_timer_OLED
  );
  add_repeating_timer_ms(
    1'000,
    on_debug_refresh, NULL, 
    &polling_timer_debug
  );
  multicore_launch_core1(start_background_processes);
  current_state = App_state::play_mode;
}

Key_Msg key_msg_out;
Rotary_Action rotary_action_out;

void loop() {
  if (queue_try_remove(&key_press_queue, &key_msg_out)) {
    interpret_key_msg(key_msg_out);
    // if you are in calibration mode, that routine can send msgs to calibration_queue
  }
  if (queue_try_remove(&rotary_action_queue, &rotary_action_out)) {
    interpret_rotary_msg(rotary_action_out);
  }
}