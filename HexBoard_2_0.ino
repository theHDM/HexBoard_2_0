/*  
 *  Hex
 Board v2.0
 */

#include "src/config.h"
#include "src/settings.h"
#include "src/debug.h"
#include "src/file_system.h"

#include "src/hexBoardHW.h"
using namespace hexBoardHW;

#include "src/hexBoardGrid.h"
Button_Grid hexBoard(hexBoard_layout_hw_1_2);

#include "src/layout.h"
App_Data music[buttons_count];

#include "src/LED.h"
Pixel_Data pixel[buttons_count];


#include "src/OLED.h"
OLED_screensaver       oled_screensaver(default_contrast, screensaver_contrast);

#include "src/direct_digital_synthesis.h"  // microtonal and MIDI math
wave_tbl cached_waveform;

#include "src/MIDI_api.h"

#include "src/menu.h"


/*
 * Menu action handler
 * 
 * After selecting an option from the OLED menu
 * this is where you define callback functions
 * 
 * A positive integer means that a setting was changed
 * (the enum value of the corresponding hexboard setting
 * is passed).
 * 
 * A negative integer means that a command was issued
 * from the menu -- e.g. update layout, load settings, etc.
 *
 */

void apply_settings_to_objects() {
  on_setting_change(_synthBuz);
  on_setting_change(_rotInv);
  on_setting_change(_synthWav);
  //generate_layout(refS);
}

void note_on(Physical_Button& b) {
  App_Data *n = static_cast<App_Data*>(b.app_data_ptr);
  if (n == nullptr) return;  
  // synth note-on
  using namespace Synth;
  if (!queue_is_empty(&open_channel_queue)) {  
    queue_remove_blocking(&open_channel_queue, &(n->synthChPlaying) );
    Voice *v = &voice[(n->synthChPlaying) - 1];
    double adj_f = frequency_after_pitch_bend(n->freq, 0 /*global pb*/, 2 /*pb range*/);
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
    v->update_base_volume((settings[_synthVol].i * b.velocity * iso226(adj_f)) >> 15);
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

  // MIDI note-on
  MIDI_api.noteOn(n->channel,n->table,n->note,b.velocity);
}

void note_off(Physical_Button& b) {
  App_Data *n = static_cast<App_Data*>(b.app_data_ptr);
  if (n == nullptr) return;  
  // synth note-off
  if (n->synthChPlaying) {
    using namespace Synth;
    voice[(n->synthChPlaying) - 1].note_off();
    if (!queue_is_full(&open_channel_queue)) {
      queue_add_blocking(
        &open_channel_queue, 
        &(n->synthChPlaying)
      );
    }
    n->synthChPlaying = 0;
  }
  
  // MIDI note-off
  MIDI_api.noteOff(n->midiChPlaying,n->table,n->note,0);
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

const uint32_t GUI_arrowPersist_uS = 500000;
volatile uint32_t GUI_timestampCW = -GUI_arrowPersist_uS;
volatile uint32_t GUI_timestampCCW = -GUI_arrowPersist_uS;
volatile uint8_t GUI_iconKnobClick = 0;

void draw_input_monitor(std::string s) {
  // draw GUI element that shows reactive 
  // pixel grid representing buttons currently
  // pressed. the string passed thru is ignored
  int atX;
  int atY;
  for (auto& b : hexBoard.btn) {
    atX = 108 + 2 * b.coord.x - (b.coord.x <= -10 ? 1 : 0);
    atY = 106 + 3 * b.coord.y;
                            u8g2.drawPixel(atX,atY);
    if (b.pressure) {       u8g2.drawPixel(atX  ,atY-1);   // off low mid hi
                            u8g2.drawPixel(atX  ,atY+1);   //      *   *  ***
    if (b.pressure >  64) { u8g2.drawPixel(atX-1,atY  );   //  *   *  *** ***
                            u8g2.drawPixel(atX+1,atY  ); } //      *   *  ***
    if (b.pressure >  96) { u8g2.drawPixel(atX-1,atY-1);   //
                            u8g2.drawPixel(atX-1,atY+1);   //
                            u8g2.drawPixel(atX+1,atY-1);   //
                            u8g2.drawPixel(atX+1,atY+1); } //          
    }
  }
  atX = 82;
  atY = 97;
  if (GUI_iconKnobClick == 1) {
    u8g2.drawBox(atX-1,atY-1,3,3);
  } else if (GUI_iconKnobClick == 2) {
    u8g2.drawPixel(atX-2,atY-1);
    u8g2.drawPixel(atX-2,atY+1);
    u8g2.drawPixel(atX,atY-2);
    u8g2.drawPixel(atX,atY);
    u8g2.drawPixel(atX,atY+2);
    u8g2.drawPixel(atX+2,atY-1);
    u8g2.drawPixel(atX+2,atY+1);
  } else if (GUI_iconKnobClick == 3) {
    u8g2.drawHLine(atX-1,atY-2,3);
    u8g2.drawVLine(atX-2,atY-1,3);
    u8g2.drawVLine(atX+2,atY-1,3);
    u8g2.drawHLine(atX-1,atY+2,3);      
  }
  if (timer_hw->timerawl - GUI_timestampCW < GUI_arrowPersist_uS) {
    u8g2.drawLine(atX+1,atY-4,atX+2,atY-5);
    u8g2.drawLine(atX+3,atY-5,atX+5,atY-3);
    u8g2.drawHLine(atX+4,atY-2,3);
    u8g2.drawVLine(atX+6,atY-4,2);
  }
  if (timer_hw->timerawl - GUI_timestampCCW < GUI_arrowPersist_uS) {
    u8g2.drawLine(atX+1,atY+4,atX+2,atY+5);
    u8g2.drawLine(atX+3,atY+5,atX+5,atY+3);
    u8g2.drawHLine(atX+4,atY+2,3);
    u8g2.drawVLine(atX+6,atY+3,2);
  }
}

void draw_GUI_sliders(std::string s) {
}

void draw_GUI_dashboard(std::string s) {
  // GUI element in "play" mode
  // show live rotary control information
  // knob press toggles what the rotary controls
  // e.g. transpose, program change, animations, etc.
}

void draw_GUI_popup(std::string s) {
  u8g2.setFont(u8g2_font_6x12_tr);
  drawStringWrap(_LEFT_MARGIN, 12, s, false);
}

void draw_GUI_verbose_log(std::string s) {
  // draw verbose text currently stored in
  // GUI instance
  u8g2.setFont(u8g2_font_4x6_tr);
  drawStringWrap(_LEFT_MARGIN, 112, s, true);
  u8g2.setFont(u8g2_font_6x12_tr);
}

void draw_GUI_footer(std::string s) {
  // display the "HUD footer" text assigned to the menu page you're on
  u8g2.setFont(u8g2_font_4x6_tr);
  drawStringWrap(_LEFT_MARGIN, 122, s, true);
  u8g2.setFont(u8g2_font_6x12_tr);
}

enum {
  _GUI_input_monitor  = 1u << 0,
  _GUI_sliders        = 1u << 1,
  _GUI_dashboard      = 1u << 2,
  _GUI_popup          = 1u << 3,
  _GUI_verbose_log    = 1u << 4,
  _GUI_footer         = 1u << 5,
};
void initialize_GUI_layers() {
  GUI.set_handler(_GUI_input_monitor, draw_input_monitor);
  GUI.set_handler(_GUI_sliders, draw_GUI_sliders);
  GUI.set_handler(_GUI_dashboard, draw_GUI_dashboard);
  GUI.set_handler(_GUI_popup, draw_GUI_popup);
  GUI.set_handler(_GUI_verbose_log, draw_GUI_verbose_log);
  GUI.set_handler(_GUI_footer, draw_GUI_footer);

  GUI.add_context(_GUI_input_monitor);
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

volatile bool doNotDrawMenu = false;
void knob_handler_menu(const Rotary::Action& r) {
  // while dealing with rotary, halt OLED auto-update
  switch (r) {
    case Rotary::Action::turn_CW:
    case Rotary::Action::turn_CW_with_press: {
      doNotDrawMenu = true;
      menu.registerKeyPress(GEM_KEY_DOWN);
      
      break;
    }
    case Rotary::Action::turn_CCW:
    case Rotary::Action::turn_CCW_with_press: {
      doNotDrawMenu = true;
      menu.registerKeyPress(GEM_KEY_UP);
      break;
    }
    case Rotary::Action::single_click_release: {
      doNotDrawMenu = true;
      if (menu_app_state() >= 2) {
        menu.registerKeyPress(GEM_KEY_RIGHT);       
      } else {
        menu.registerKeyPress(GEM_KEY_OK);       
      }
      break;
    }
    case Rotary::Action::double_click: {
      if (menu_app_state() >= 2) {
        doNotDrawMenu = true;
        menu.registerKeyPress(GEM_KEY_LEFT);       
      } else {
        //
      }
      break;
    }
    case Rotary::Action::double_click_release: {
      doNotDrawMenu = true;
      if (menu_app_state() >= 2) {
        menu.registerKeyPress(GEM_KEY_LEFT);       
      } else {
        menu.registerKeyPress(GEM_KEY_OK);       
      }
      break;
    }
    case Rotary::Action::long_press: {
      doNotDrawMenu = true;
      if (menu_app_state() >= 2) {
        menu.registerKeyPress(GEM_KEY_OK);
      } else if (menu.getCurrentMenuPage() == &pgHome) {
        menu.setMenuPageCurrent(pgNoMenu);
      } else {
        menu.registerKeyPress(GEM_KEY_CANCEL);
      }
      break;
    }
    default:                          break;
  }
  doNotDrawMenu = false;
}

void knob_handler_playback(const Rotary::Action& r) {
  switch (r) {
    case Rotary::Action::turn_CW:
    case Rotary::Action::turn_CW_with_press: {
      // based on current live setting,
      // change said setting by +1 on the fly
      ++settings[_globlBrt].i;
      break;
    }
    case Rotary::Action::turn_CCW:
    case Rotary::Action::turn_CCW_with_press: {
      // based on current live setting,
      // change said setting by -1 on the fly
      --settings[_globlBrt].i;
      break;
    }
    case Rotary::Action::single_click_release:
    case Rotary::Action::double_click_release: {
      // change live setting on the fly

      break;
    }
    case Rotary::Action::long_press: {
      // long press changes to menu mode
      menu.setMenuPageCurrent(pgHome);
      break;
    }
    default:                          break;
  }
}

void knob_handler_GUI(const Rotary::Action& r) {
  switch (r) {
    case Rotary::Action::turn_CW:
    case Rotary::Action::turn_CW_with_press: {
      GUI_timestampCW = timer_hw->timerawl;
      break;
    }
    case Rotary::Action::turn_CCW:
    case Rotary::Action::turn_CCW_with_press: {
      GUI_timestampCCW = timer_hw->timerawl;
      break;
    }
    case Rotary::Action::single_click_press: {
      GUI_iconKnobClick = 1;
      break;
    }
    case Rotary::Action::double_click: {
      GUI_iconKnobClick = 2;
      break;
    }
    case Rotary::Action::long_press: {
      GUI_iconKnobClick = 3;
      break;
    }
    default: {
      GUI_iconKnobClick = 0;
      break;
    }
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
 * startup subroutines
 */
void hardware_test_mode() {
  // hold to do some cool stuff
}

/*
  while (current_state == App_state::safe_mode) {
    // blocking queue_remove because must make a selection
    queue_remove_blocking(&Rotary::act_queue, &Rotary::action_out);
    switch (Rotary::action_out) {
      case Rotary::Action::turn_CW: 
      case Rotary::Action::turn_CW_with_press:
      case Rotary::Action::turn_CCW: 
      case Rotary::Action::turn_CCW_with_press: {
        popup_toggle = !popup_toggle;
        break;
      }
      case Rotary::Action::click: {
        if (popup_toggle) {
          current_state = App_state::hardware_test;
          hardware_test_mode();
        }
        GUI.set_context(0,"");
        current_state = App_state::startup;
        break;
      }
      default: break;
    }
  }
*/



void on_setting_change(int s) {
  switch (s) {
    case _txposeS: case _txposeC:
      // recalculate pitches for everyone
      break;
    case _scaleLck:
      // set scale lock as appropriate
      break;
    case _rotInv: case _rotDblCk: case _rotLongP:
      Rotary::recalibrate(
        settings[_rotInv].b, 
        settings[_rotLongP].i, 
        settings[_rotDblCk].i
      );
      break;
    case _MIDIusb: case _MIDIjack:
      // turn MIDI jacks on/off
      break;    
    case _synthBuz: case _synthJac:
      Synth::set_pin(piezoPin, settings[_synthBuz].b);
      Synth::set_pin(audioJackPin, settings[_synthJac].b);
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

void menu_handler(int m) {
  if (m >= 0) {
    on_setting_change(m);
    return;
  }
  if (m <= _trigger_format_flash) {
    if (m - _trigger_format_flash == -1) {
      //
    }
  }
}

struct repeating_timer polling_timer_LED;
bool on_LED_frame_refresh(repeating_timer *t) {
  for (auto& b : hexBoard.btn) {
    Pixel_Data *p = static_cast<Pixel_Data*>(b.pxl_data_ptr);
    strip.setPixelColor(b.pixel, p->LEDcode);
  }
  strip.show();
  return true;
}

struct repeating_timer polling_timer_OLED;
bool on_OLED_frame_refresh(repeating_timer *t) {
  if (!doNotDrawMenu) {
    menu.drawMenu();
    oled_screensaver.jiggle();
  }
  return true;
}

/*
  struct repeating_timer polling_timer_debug;
  bool on_debug_refresh(repeating_timer *t) {
    //debug.add_num(successes);
    //debug.add("\n");
    debug.send();
    return true;
  }
*/
  
void initialize_application() {
  for (size_t i = 0; i < buttons_count; ++i) {
    hexBoard.btn[i].app_data_ptr = static_cast<void*>(&music[i]);
    hexBoard.btn[i].pxl_data_ptr = static_cast<void*>(&pixel[i]);
  }
}


void initialize_settings() {
  load_factory_defaults_to(settings);
  debug.setStatus(&settings[_debug].b);
  oled_screensaver.setDelay(&settings[_SStime].i);
}



void setup() {
  // code to link modules and objects before setup
  initialize_application();
  initialize_settings();
  initialize_GUI_layers();
  menu_setup();
  // boot up hardware
  Boot_Flags::fs_mounted = mount_file_system(false);
  multicore_launch_core1(begin_background_processes);
  connect_OLED_display(OLED_sdaPin, OLED_sclPin);
  connect_neoPixels(ledPin, buttons_count);
  mount_tinyUSB();
  init_MIDI();
  // display handlers
  add_repeating_timer_ms(OLED_poll_interval_mS, on_OLED_frame_refresh, NULL, &polling_timer_OLED);
  add_repeating_timer_ms(LED_poll_interval_mS, on_LED_frame_refresh, NULL, &polling_timer_LED);
  // if knob held down during boot, go into safe mode
  Boot_Flags::safe_mode = Rotary::getClickState();
  if (Boot_Flags::safe_mode) {
    // stick with default settings
    // apply default settings
    // hold on loading a layout
  } else {
    menu.setMenuPageCurrent(pgFileSystemError);
    //if (!load_settings(settings, settingFileName)) {
      // if load fails?
    //}  
    // try to load latest settings
    // try to load latest layout
  }
  // when settings are updated from a file, run this:
  apply_settings_to_objects();
  menu.drawMenu();
}

void loop() {
  // key handler
  if (queue_try_remove(&Keys::msg_queue, &Keys::msg_out)) {
    if (hexBoard.btn_at_index[Keys::msg_out.switch_number] == nullptr) {
      if (hexBoard.dip_at_index[Keys::msg_out.switch_number] == nullptr) return;
      Hardwire_Switch* h = hexBoard.dip_at_index[Keys::msg_out.switch_number];
      h->state = Keys::msg_out.level;
      hardwired_switch_handler(*h);
      return;
    }
    Physical_Button* b = hexBoard.btn_at_index[Keys::msg_out.switch_number];
    b->update_levels(Keys::msg_out.timestamp, Keys::msg_out.level);
    // can change this based on current key situation
    key_handler_playback(*b);
  }
  // knob handler
  if (queue_try_remove(&Rotary::act_queue, &Rotary::action_out)) {
    knob_handler_GUI(Rotary::action_out);
    if (menu.getCurrentMenuPage() == &pgNoMenu) {
      knob_handler_playback(Rotary::action_out);
    } else {
      knob_handler_menu(Rotary::action_out);
    }
  }
  // LED and OLED displays run on a timer in the background
}
