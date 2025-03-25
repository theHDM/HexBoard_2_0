/*  
 *  HexBoard v2.0
 */

#include <stdint.h>
#include "pico/time.h"
#include "src/config.h"

enum class App_state { 
  setup,               
  play_mode, menu_nav,
  hex_picker, color_picker,
  sequencer, calibrate,           
  data_mgmt, crash, low_power,
};
App_state current_state = App_state::setup;


#include "src/debug.h"
#include "src/settings.h"
#include "src/file_system.h"

const char* settingFileName = "temp222.dat";


#include "src/hardware_drivers.h"
namespace hexBoardHW::Synth {
  Instance instance;
  bool on_callback(struct repeating_timer *t)  { 
    instance.poll();  
    return true; 
  }
}
namespace hexBoardHW::Rotary {
  Instance instance(rotaryPinA, rotaryPinB, rotaryPinC);
  bool on_callback(struct repeating_timer *t) { 
    instance.poll(); 
    return true; 
  }
}
namespace hexBoardHW::Keys {
  Instance instance(muxPins, colPins, analogPins);
  bool on_callback(struct repeating_timer *t)   { 
    instance.poll();   
    return true; 
  }
}

#include "src/color.h"
#include "src/music.h"  // microtonal and MIDI math
wave_tbl               cached_waveform;

#include "src/MIDI_api.h"
#include "src/LED.h"
#include "src/OLED.h"
#include "src/button.h"

OLED_screensaver       oled_screensaver(default_contrast, screensaver_contrast);
Button_Grid            hexBoard(hexBoard_layout_hw_1_2);

#include "src/menu.h"
#include "src/GUI.h"
//#include "src/layout.h"

void start_background_processes() {
  alarm_pool_t *core1pool;
  core1pool = alarm_pool_create(1, 4);
  hexBoardHW::Synth::instance.begin();
  hexBoardHW::Synth::background_poll(core1pool, -audio_sample_interval_uS);
  hexBoardHW::Rotary::instance.begin();
  hexBoardHW::Rotary::background_poll(core1pool, rotary_poll_interval_uS);
  hexBoardHW::Keys::instance.begin();
  hexBoardHW::Keys::background_poll(core1pool, key_poll_interval_uS);
  while (1) {
    // once these objects are run, then core_1 will
    // run background processes only
  }
}

void link_settings_to_objects(hexBoard_Setting_Array& refS) {
  debug.setStatus(&refS[_debug].b);
  oled_screensaver.setDelay(&refS[_SStime].i);
}

void on_setting_change(int settingNumber) {
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
      hexBoardHW::Rotary::instance.recalibrate(
        settings[_rotInv].b, 
        settings[_rotLongP].i, 
        settings[_rotDblCk].i
      );
      break;
    case _MIDIusb: case _MIDIjack:
      // turn MIDI jacks on/off
      break;    
    case _synthBuz: case _synthJac:
      hexBoardHW::Synth::instance.set_pin(piezoPin, settings[_synthBuz].b);
      hexBoardHW::Synth::instance.set_pin(audioJackPin, settings[_synthJac].b);
      break;    
    case _MIDIpc: case _MT32pc:
      // send program change
      break;
    case _synthWav:
      pre_cache_synth_waveform(settings[_synthWav].i, cached_waveform); 
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

void apply_settings_to_objects(hexBoard_Setting_Array& refS) {
  on_setting_change(_synthBuz);
  on_setting_change(_rotInv);
  on_setting_change(_synthWav);
  //generate_layout(refS);
}

void note_on(Physical_Button& n) {
  using namespace hexBoardHW::Synth;
  // synth note-on
  if (!queue_is_empty(&open_channel_queue)) {
    queue_remove_blocking(&open_channel_queue, &(n.synthChPlaying) );
    Voice *v = &instance.voice[(n.synthChPlaying) - 1];
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
    using namespace hexBoardHW::Synth;
    instance.voice[(n.synthChPlaying) - 1].note_off();
    if (!queue_is_full(&open_channel_queue)) {
      queue_add_blocking(
        &open_channel_queue, 
        &(n.synthChPlaying)
      );
    }
    n.synthChPlaying = 0;
  }
  // MIDI note-off
}

void color_this_hex(const Hex& h, const HSV& c) {
  strip.setPixelColor(
    hexBoard.btn_by_coord.at(h)->pixel, 
    okhsv_to_neopixel_code(c)
  );
}

/*
 * GUI layers
 */

void draw_hex_grid_on_OLED() {
  for (auto& b : hexBoard.btn) {
    int atX = 107 + 2 * b.coord.x 
                              - (b.coord.x <= -10 ? 1 : 0);
    int atY =  99 + 3 * b.coord.y;
    u8g2.drawPixel(atX,atY);
    if (b.pressure) {
                            u8g2.drawPixel(atX  ,atY-1);   // off low mid hi
                            u8g2.drawPixel(atX  ,atY+1);   //      *   *  ***
    if (b.pressure >  64) { u8g2.drawPixel(atX-1,atY  );   //  *   *  *** ***
                            u8g2.drawPixel(atX+1,atY  ); } //      *   *  ***
    if (b.pressure >  96) { u8g2.drawPixel(atX-1,atY-1);   //
                            u8g2.drawPixel(atX-1,atY+1);   //
                            u8g2.drawPixel(atX+1,atY-1);   //
                            u8g2.drawPixel(atX+1,atY+1); } //          
    }
  }  
}

void draw_brightness_amt_on_OLED() {
  u8g2.setFont(u8g2_font_osr41_tn);
  u8g2.drawStr(_LEFT_MARGIN, 10, std::to_string(settings[_globlBrt].i).c_str());
  u8g2.setFont(u8g2_font_6x12_tr);
}

void draw_HUD_footer_on_OLED() {
  u8g2.setFont(u8g2_font_4x6_tr);
  GEMPagePublic* thisPg = static_cast<GEMPagePublic*>
                        (menu.getCurrentMenuPage());
  u8g2.drawStr(_LEFT_MARGIN, 122, thisPg->HUD_footer.c_str());
  u8g2.setFont(u8g2_font_6x12_tr);
}

void draw_hex_picker_on_OLED() {
  Hex selHex;
  selHex.x = settings[_anchorX].i;
  selHex.y = settings[_anchorY].i;
  if (hexBoard.in_bounds(selHex)) {
    u8g2.drawStr(_LEFT_MARGIN, 8, "Anchor hex: ");
    u8g2.drawStr(_LEFT_MARGIN + 84, 8, 
      std::to_string(hexBoard.btn_by_coord.at(selHex)->pixel).c_str());
  } else {
    u8g2.drawStr(_LEFT_MARGIN, 8, "Press anchor hex >>");
  }
}

void initialize_GUI_layers() {
  GUI.drawLayer[0] = draw_hex_grid_on_OLED;
  GUI.drawLayer[1] = draw_brightness_amt_on_OLED;
  GUI.drawLayer[2] = draw_HUD_footer_on_OLED;
  GUI.drawLayer[2] = draw_hex_picker_on_OLED;
}


/*  
 *  Handlers for UI input: key and knob
 */

void key_handler_playback(Physical_Button& b) {
  if (b.check_and_reset_just_pressed()) {
    note_on(b);
  } else if (b.check_and_reset_just_released()) {
    note_off(b);
  } else if (b.pressure) {
    // nothing
  }
}

void key_handler_hex_picker(Physical_Button& b) {
  if (b.check_and_reset_just_pressed()) {
    settings[_anchorX].i = b.coord.x;
    settings[_anchorY].i = b.coord.y;
  } else if (b.check_and_reset_just_released()) {
    note_off(b);
  } else if (b.pressure) {
    // nothing
  }
}

void key_handler_color_picker(Physical_Button& b) {
  if (b.check_and_reset_just_pressed()) {
    if (b.coord.y == -6) {
      switch (b.coord.x) {
        case -4: settings[_hue_0].d = _hueY; break;
        case -2: settings[_hue_0].d = _hueC; break;
        case  0: settings[_hue_0].d = _hueG; break;
        case  2: settings[_hue_0].d = _hueM; break;
        case  4: settings[_hue_0].d = _hueR; break;
        case  6: settings[_hue_0].d = _hueB; break;
      }
    } else if (b.coord.y == -4) {
      settings[_hue_0].d += 3.0 * b.coord.x;
    }
  } else if (b.check_and_reset_just_released()) {
    note_off(b);
  } else if (b.pressure) {
    // nothing
  }
}

void knob_handler_playback(const hexBoardHW::Rotary::Action& r) {
  switch (r) {
    case hexBoardHW::Rotary::Action::turn_CW:
    case hexBoardHW::Rotary::Action::turn_CW_with_press: {
      ++settings[_globlBrt].i;
      break;
    }
    case hexBoardHW::Rotary::Action::turn_CCW:
    case hexBoardHW::Rotary::Action::turn_CCW_with_press: {
      --settings[_globlBrt].i;
      break;
    }
    case hexBoardHW::Rotary::Action::long_press: {
      menu.setMenuPageCurrent(pgHome);
      current_state = App_state::menu_nav;
      break;
    }
    default:                          break;
  }
}

void knob_handler_color_picker(const hexBoardHW::Rotary::Action& r) {
  knob_handler_playback(r);
}
void knob_handler_hex_picker(const hexBoardHW::Rotary::Action& r) {
  knob_handler_playback(r);
}


volatile bool doNotDrawMenu = false;
void knob_handler_menu(const hexBoardHW::Rotary::Action& r) {
  // while dealing with rotary, halt OLED auto-update
  doNotDrawMenu = true;
  switch (r) {
    case hexBoardHW::Rotary::Action::turn_CW:
    case hexBoardHW::Rotary::Action::turn_CW_with_press: {
      menu.registerKeyPress(GEM_KEY_DOWN);
      break;
    }
    case hexBoardHW::Rotary::Action::turn_CCW:
    case hexBoardHW::Rotary::Action::turn_CCW_with_press: {
      menu.registerKeyPress(GEM_KEY_UP);
      break;
    }
    case hexBoardHW::Rotary::Action::click: {
      if (menu_app_state() >= 2) {
        menu.registerKeyPress(GEM_KEY_RIGHT);       
      } else {
        menu.registerKeyPress(GEM_KEY_OK);       
      }
      break;
    }
    case hexBoardHW::Rotary::Action::double_click: {
      if (menu_app_state() >= 2) {
        menu.registerKeyPress(GEM_KEY_LEFT);       
      } else {
        //
      }
      break;
    }
    case hexBoardHW::Rotary::Action::double_click_release: {
      if (menu_app_state() >= 2) {
        menu.registerKeyPress(GEM_KEY_LEFT);       
      } else {
        menu.registerKeyPress(GEM_KEY_OK);       
      }
      break;
    }
    case hexBoardHW::Rotary::Action::long_press: {
      if (menu_app_state() >= 2) {
        menu.registerKeyPress(GEM_KEY_OK);
      } else if (menu.getCurrentMenuPage() == &pgHome) {
        menu.setMenuPageCurrent(pgNoMenu);
        current_state = App_state::play_mode;
      } else {
        menu.registerKeyPress(GEM_KEY_CANCEL);
      }
      break;
    }
    default:                          break;
    doNotDrawMenu = false;
  }
}

void hardwired_switch_handler(const Hardwire_Switch& h) {
  if (h.pinID == linear_index(14,0)) {
    if (settings[_defaults].b) { // if you have not loaded existing settings
      load_factory_defaults_to(settings, 12); // replace with v1.2 firmware defaults
    }
  }
}

/*  
 *  Handlers for UI output: LED and OLED
 */

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
      if (doNotDrawMenu) break;
      menu.drawMenu();
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


/*
 *  Boot order:
 *  1) set up OLED display
 *     display verbose logging?
 *  2) set up background collection (keys, knob, audio)
 *  3) mount file system
 *  4) attempt to load last settings and layout
 *  5) if either fail
 */

void setup() {
  hexBoardHW::Keys::initialize_queue(keys_count + 1);
  hexBoardHW::Rotary::initialize_queue(32);
  load_factory_defaults_to(settings);
  link_settings_to_objects(settings);
  add_repeating_timer_ms(LED_poll_interval_mS, on_LED_frame_refresh, NULL, &polling_timer_LED);
  add_repeating_timer_ms(OLED_poll_interval_mS, on_OLED_frame_refresh, NULL, &polling_timer_OLED);
  add_repeating_timer_ms(1'000, on_debug_refresh, NULL, &polling_timer_debug);
  multicore_launch_core1(start_background_processes);
  connect_OLED_display(OLED_sdaPin, OLED_sclPin);
  connect_neoPixels(ledPin, buttons_count);
  initialize_GUI_layers();


  if (hexBoardHW::Rotary::instance.getClickState()) {
    // knob held down during boot -- offer factory reset
    // GUI a message box
  }
  
  if (mount_file_system(false)) {
    // loop until user confirms reformat.
  }
  //if (!load_settings(settings, settingFileName)) {
    // if load fails?
  //}  

  mount_tinyUSB();
  apply_settings_to_objects(settings);
  init_MIDI();
  hexBoardHW::Synth::initialize_channel_queue();
  menu_setup();
	menu.setDrawMenuCallback(after_menu_update_GUI);
  current_state = App_state::play_mode;
}
void loop() {
  hexBoardHW::Keys::Msg key_msg_out;
  if (queue_try_remove(&hexBoardHW::Keys::msg_queue, &key_msg_out)) {
    if (hexBoard.btn_at_index[key_msg_out.switch_number] == nullptr) {
      if (hexBoard.dip_at_index[key_msg_out.switch_number] == nullptr) return;
      Hardwire_Switch* h = hexBoard.dip_at_index[key_msg_out.switch_number];
      h->state = key_msg_out.level;
      hardwired_switch_handler(*h);
      return;
    }
    Physical_Button* b = hexBoard.btn_at_index[key_msg_out.switch_number];
    b->update_levels(key_msg_out.timestamp, key_msg_out.level);
    switch (current_state) {
      case App_state::play_mode:    key_handler_playback(*b);     break;
      case App_state::menu_nav:     key_handler_playback(*b);     break;
      case App_state::hex_picker:   key_handler_hex_picker(*b);   break;
      case App_state::color_picker: key_handler_color_picker(*b); break;
      default:                                                    break;
    }
  }
  hexBoardHW::Rotary::Action rotary_action_out;
  if (queue_try_remove(&hexBoardHW::Rotary::act_queue, &rotary_action_out)) {
    switch (current_state) {
      case App_state::play_mode:    knob_handler_playback(rotary_action_out);     break;
      case App_state::menu_nav:     knob_handler_menu(rotary_action_out);         break;
      case App_state::hex_picker:   knob_handler_hex_picker(rotary_action_out);   break;
      case App_state::color_picker: knob_handler_color_picker(rotary_action_out); break;
      default:                                                                    break;
    }
  }
}