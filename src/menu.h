#pragma once
#include "OLED.h" // OLED graphics object
#include "library.h"
#include <GEM_u8g2.h>   // library of code to create menu objects on the B&W display
#include "config/enable-advanced-mode.h"
#include "settings.h"

// pre-allocate an array for menu items that are
// generated to modify settings.
GEM_u8g2 menu(u8g2);

GEMSelect dd_periods(4, (SelectOptionInt[]){
  {"Octave" , 0},
  {"Tritave", 1},
  {"Fifth"  , 2},
  {"Custom" ,-1}
});

GEMSelect dropdown_dir(6, (SelectOptionInt[]){
  {"Down-Left",dir_sw},
  {"Left",     dir_w},
  {"Up-Left",  dir_nw},
  {"Up-Right", dir_ne},
  {"Right",    dir_e},
  {"DownRight",dir_se}
});

GEMSelect dropdown_lg_sm_ratio(11, (SelectOptionInt[]){
  {"5:4 ",      0x54},
  {"4:3",       0x43},
  {"3:2  Soft", 0x32},
  {"5:3",       0x53},
  {"2:1 Basic", 0x21},
  {"5:2",       0x52},
  {"3:1  Hard", 0x31},
  {"4:1",       0x41},
  {"5:1",       0x51},
  {"customize",    0},
  {"  decimal",   -1}
});

GEMSelect dropdown_speed(7, (SelectOptionInt[]){
  {"TooSlow",1},
  {"Turtle", 2},
  {"Slow",   4}, 
  {"Medium", 8},
  {"Fast",   16},
  {"Cheetah",32},
  {"Instant",127}
});

GEMSelect dropdown_anim(9, (SelectOptionInt[]){
  {"None",   _animType_none},
  {"Star",   _animType_star},
  {"Splash", _animType_splash},
  {"Orbit",  _animType_orbit},
  {"Octave", _animType_octave},
  {"By Note",_animType_by_note},
  {"Beams",  _animType_beams},
  {"Rv.Spls",_animType_splash_reverse},
  {"Rv.Star",_animType_star_reverse}
});

GEMSelect dropdown_bright(7, (SelectOptionInt[]){
  {"Off",    _globlBrt_off},
  {"Dimmer", _globlBrt_dimmer},
  {"Dim",    _globlBrt_dim},
  {"Low",    _globlBrt_low},
  {"Mid",    _globlBrt_mid},
  {"High",   _globlBrt_high},
  {"THE SUN",_globlBrt_max}
});

GEMSelect dropdown_synth_mode(4, (SelectOptionInt[]){
  {"  Off",   _synthTyp_off},
  {"  Mono",  _synthTyp_mono},
  {"Arpeggio",_synthTyp_arpeggio},
  {"  Poly",  _synthTyp_poly}
});

GEMSelect dropdown_wave(7, (SelectOptionInt[]){
  {" Hybrid", _synthWav_hybrid},
  {" Square", _synthWav_square},
  {"  Saw",   _synthWav_saw},
  {"Triangle",_synthWav_triangle},
  {" Sine",   _synthWav_sine},
  {"Strings", _synthWav_strings},
  {"Clarinet",_synthWav_clarinet}
});

GEMSelect dropdown_adsr(6,(SelectOptionInt[]){
  {"  None",   _synthEnv_none},
  {"  Hit",    _synthEnv_hit},
  {" Pluck",   _synthEnv_pluck},
  {" Strum",   _synthEnv_strum},
  {"  Slow",   _synthEnv_slow},
  {"Reverse",  _synthEnv_reverse}
});

GEMSelect dropdown_palette(3, (SelectOptionInt[]){
  {"Rainbow",_palette_rainbow},
  {"Tiered", _palette_tiered},
  {"Alt",    _palette_alternate}
});

GEMSelect dropdown_fps(4, (SelectOptionInt[]){
  {"24",24},{"30",30},{"60",60},{"70",70}
});

GEMSelect dropdown_MIDImode(4, (SelectOptionInt[]){
  {"MIDI Mode: Normal", _MIDImode_standard},
  {"MIDI Mode: MPE", _MIDImode_MPE},
  {"MIDI Mode: MTS", _MIDImode_tuning_table},
  {"MIDI Mode: 2.0", _MIDImode_2_point_oh}
});

GEMSelect dropdown_instruments(2, (SelectOptionInt[]){
  {"General MIDI PC:", _GM_instruments},
  {"Roland MT-32 PC:", _MT32_instruments}
});

GEMSelect dropdown_MPE(3, (SelectOptionInt[]){
  {"1-zone master ch 1 ", 0},
  {"1-zone master ch 16", 1},
  {"2-zone (dual banks)", 2},
});  

GEMSelect dropdown_MPE_left(14, (SelectOptionInt[]){
  {" 2 voices, ch 1-3  ", 3},
  {" 3 voices, ch 1-4  ", 4},
  {" 4 voices, ch 1-5  ", 5},
  {" 5 voices, ch 1-6  ", 6},
  {" 6 voices, ch 1-7  ", 7},
  {" 7 voices, ch 1-8  ", 8},
  {" 8 voices, ch 1-9  ", 9},
  {" 9 voices, ch 1-10 ",10},
  {"10 voices, ch 1-11 ",11},
  {"11 voices, ch 1-12 ",12},
  {"12 voices, ch 1-13 ",13},
  {"13 voices, ch 1-14 ",14},
  {"14 voices, ch 1-15 ",15},
  {"15 voices, ch 1-16 ",16}
});
GEMSelect dropdown_MPE_right(14, (SelectOptionInt[]){
  {" 2 voices, ch 14-16",14},
  {" 3 voices, ch 13-16",13},
  {" 4 voices, ch 12-16",12},
  {" 5 voices, ch 11-16",11},
  {" 6 voices, ch 10-16",10},
  {" 7 voices, ch 9-16 ", 9},
  {" 8 voices, ch 8-16 ", 8},
  {" 9 voices, ch 7-16 ", 7},
  {"10 voices, ch 6-16 ", 6},
  {"11 voices, ch 5-16 ", 5},
  {"12 voices, ch 4-16 ", 4},
  {"13 voices, ch 3-16 ", 3},
  {"14 voices, ch 2-16 ", 2},
  {"15 voices, ch 1-16 ", 1}
});
GEMSelect dropdown_MPE_PB(5, (SelectOptionInt[]){
  {"Pitch bend +/-  2",2 },
  {"Pitch bend +/- 12",12},
  {"Pitch bend +/- 24",24},
  {"Pitch bend +/- 48",48},
  {"Pitch bend +/- 96",96}
});

GEMSpinner spin_1_32    ((GEMSpinnerBoundariesInt){1,   1,  32});
GEMSpinner spin_1_16    ((GEMSpinnerBoundariesInt){1,   1,  16});
GEMSpinner spin_0_62    ((GEMSpinnerBoundariesInt){1,   0,  62});
GEMSpinner spin_0_127   ((GEMSpinnerBoundariesInt){1,   0, 127});
GEMSpinner spin_n127_127((GEMSpinnerBoundariesInt){1,-127, 127});
GEMSpinner spin_0_255   ((GEMSpinnerBoundariesInt){1,   0, 255});
GEMSpinner spin_1_255   ((GEMSpinnerBoundariesInt){1,   1, 255});
GEMSpinner spin_100_1k  ((GEMSpinnerBoundariesInt){10,100,1000});
GEMSpinner spin_500_2k  ((GEMSpinnerBoundariesInt){10,500,2000});

GEMItem *menuItem[_settingSize];

/*
 *  allow the GEMPage object to store the GUI context
 *  and make custom constructors to streamline design.
 *  OLED is 128 pixels wide.
 *  Menu items are displayed in monospace font.
 *  Maximum string size is 19 characters.
 *  On each page you set characters aside for 
 *  selectors (spinners, checkboxes, value entry)
 *  This can be different between pages -- e.g.
 *  7 characters for some pages, 8 for others.
 *  The default layout can show 11 menu entries,
 *  plus a "back" entry at the top (enter zero).
 *  If you want to display the HUD panel below
 *  then you can limit how many menu items are 
 *  shown to e.g. 7 rows.
 */

struct GEMPagePublic : public GEMPage {
	std::string header_text;
  GEMAppearance derived_appearance;
  void initialize_appearance(byte vOffset_, byte itemsPerScr_, byte hOffset_) {
    derived_appearance = {GEM_POINTER_ROW, itemsPerScr_, 
      (byte)(_LG_FONT_HEIGHT + 2), 
      (byte)(4 + _SM_FONT_HEIGHT + (vOffset_ * (_LG_FONT_HEIGHT + 3)
      )),
      (byte)(_OLED_WIDTH - _RIGHT_MARGIN - (hOffset_ * _LG_FONT_WIDTH))
    };
    _appearance = &derived_appearance;
  }
  GEMPagePublic(const char* title_, 
                std::string header_text_, byte headerRows_, 
                byte itemsPerScreen_, byte valueMargin_)
  : GEMPage(title_), header_text(header_text_) {
    initialize_appearance(headerRows_, itemsPerScreen_, valueMargin_);
  }
  GEMPagePublic(const char* title_, GEMPage& parentPage_,  
	              byte itemsPerScreen_, byte valueMargin_)
  : GEMPage(title_, parentPage_), header_text("") {
    initialize_appearance(0, itemsPerScreen_, valueMargin_);
  }
  GEMPagePublic(const char* title_, void (*on_exit_)(),
                std::string header_text_, byte headerRows_, 
	              byte itemsPerScreen_, byte valueMargin_)
  : GEMPage(title_, on_exit_), header_text(header_text_) {
    initialize_appearance(headerRows_, itemsPerScreen_, valueMargin_);
  }
};

GEMPagePublic pgNoMenu("", "", 0, 1, 0);

GEMPagePublic pgFileSystemError("Error",
  "LittleFS file systemcould not mount.\nAttempt formatting\nflash partition?", 
  5, 2, 0);
  
GEMPagePublic pgSafeMode("Safe mode", "", 0, 7 ,0);

GEMPagePublic pgHome("HexBoard v2.0", "", 0, 7, 0);

// the menu items created based on settings
// will pass thru this callback function
// first. here we'll ONLY put in menu-related
// callback items (i.e. showing/hiding entries).
// then in the main program we'll
// flesh out other code-related activity.
// + positive integer callbacks mean it came from the
// associated setting menu item.
// - negative integer callbacks are to trigger
// special routines.
enum {
  _trigger_format_flash = -2048, //- 0b 1000 0000 000*  * = yes/no
  _trigger_save_layout  = -1280, //- 0b  101 0000 ****  * = preset #
  _trigger_load_layout  = -1024, //- 0b  100 0000 ****  * = preset #
  _trigger_save_setting = -768,  //- 0b  011 0000 ****  * = preset #
  _trigger_load_setting = -512,  //- 0b  010 0000 ****  * = preset #
  _trigger_hardware_test = -256,
};

extern void menu_handler(int m);

// a "sidebar" page is one that could have multiple points of entry.
// using this macro ensures the "go back" button goes to the previous page.
#define __SIDEBAR(P) P.setParentMenuPage(*(menu.getCurrentMenuPage()));menu.setMenuPageCurrent(P)

void onChg(GEMCallbackData callbackData) {
  // these are only menu-related navigation actions
	int m = callbackData.valInt;
  menu_handler(m);
  menu.drawMenu();
}

#define __LIB_LIST(L,P,A,E) menuItem[A]=new GEMItem(L,settings[A].i,*new GEMSelect(sizeof(E)/sizeof(E[0]),static_cast<SelectOptionInt*>(static_cast<void*>(E))),onChg,A);P.addMenuItem(*menuItem[A])
#define __DROPDOWN(L,P,A,S) menuItem[A]=new GEMItem(L,settings[A].i,S,onChg,A);P.addMenuItem(*menuItem[A])
#define __CHECKBOX(L,P,A) menuItem[A]=new GEMItem(L,settings[A].b,onChg,A);P.addMenuItem(*menuItem[A])
#define __EDIT_FLT(L,P,A) menuItem[A]=new GEMItem(L,settings[A].d,onChg,A);P.addMenuItem(*menuItem[A])
#define __SEND_INT(L,P,C) P.addMenuItem(*new GEMItem(L,onChg,C))
#define __NAVIGATE(L,P,D) P.addMenuItem(*new GEMItem(L,D))

void build_menu() {

  // pgFileSystemError
  __SEND_INT("Yes", pgFileSystemError, _trigger_format_flash - 1);
  __SEND_INT("No",  pgFileSystemError, _trigger_format_flash);

  // pgSafeMode
  




  // sidebar pages
/*
  __EDIT_FLT("Amount in c",         pgSideBarCents, _equaveC);
  __SEND_INT( "           Confirm", pgSideBarCents, _on_change_equave); // this needs work
  fill_MIDI_pitch_names(string_array_MIDI_pitch_names, list_of_MIDI_pitch_names, 0, 7);
  fill_coarse_pitch_values(string_array_coarse_pitch, list_of_coarse_pitch, 4, 3);
  fill_fine_pitch_values(string_array_fine_pitch, list_of_fine_pitch, 7, 0);
  __LIB_LIST("Pitch",    pgSideBarKey,   _anchorN,  list_of_MIDI_pitch_names);
  __LIB_LIST("Coarse",   pgSideBarKey,   _anchorC,  list_of_coarse_pitch);
  __LIB_LIST("Fine",     pgSideBarKey,   _anchorF,  list_of_fine_pitch);
  __SEND_INT("           Confirm", pgSideBarKey, _on_change_key); // this needs work



  // safe mode
  __NAVIGATE("Hardware Test", pgSafeMode, pgHardwareTest);
  __NAVIGATE("Show me a popup", pgSafeMode, pgShowMsg);
    __SEND_INT("OK", pgShowMsg, -16);
  __NAVIGATE("Show me a choice", pgSafeMode, pgShowMsg2);
    __SEND_INT("OK", pgShowMsg2, -17);
    __NAVIGATE("Cancel", pgShowMsg2, pgSafeMode);
  //__NAVIGATE("Format Flash", pgSafeMode, pgFormatFlash);

  // menu tree
  //__NAVIGATE("Tuning", pgHome, pgTuning);
  __NAVIGATE("Layout", pgHome, pgLayout);
  //__NAVIGATE("Scales", pgHome, pgScales);
  //__NAVIGATE("Color Options", pgHome, pgColors);
  //__NAVIGATE("Synth", pgHome, pgSynth);
  //__NAVIGATE("MIDI Options", pgHome, pgMIDI);
  //__NAVIGATE("Control Wheel", pgHome, pgControl);
  //__NAVIGATE("Advanced", pgHome, pgAdvanced);
  //	__NAVIGATE("Select from preset",  pgLayout,   pgL_preset);
  //  __NAVIGATE("Make new isomorphic", pgLayout,   pgL_Iso);
  //  __NAVIGATE("Make new easy-scale", pgLayout,   pgL_EZ);
  //  __NAVIGATE("Make new microtonal", pgLayout,   pgL_Micro);
  //    __DROPDOWN(" Period is:",          pgL_Micro,  _equaveD,  dd_periods);
  //    __SEND_INT(label_anchor.c_str(),   pgL_Micro,  _goto_select_key);

*/
}

struct GEMItemPublic : public GEMItem {
  GEMItemPublic(const GEMItem& g) : GEMItem(g) {}
  // expose the underlying numerical type linked to the menu item
  byte getLinkedType() { return linkedType; }
};
int menu_app_state() {
  if (!menu.readyForKey()) return 0;
  if (!menu.isEditMode()) return 1;
  GEMItemPublic pubItem = *(menu.getCurrentMenuPage()->getCurrentMenuItem());
  return 2 + (pubItem.getLinkedType() == GEM_VAL_DOUBLE);
}

void after_menu_update_GUI() {
	GEMPagePublic* thisPg = static_cast<GEMPagePublic*>(menu.getCurrentMenuPage());
  drawStringWrap(_LEFT_MARGIN, 4 + _SM_FONT_HEIGHT, thisPg->header_text.c_str());
	GUI.draw();
}

void menu_setup() {
  menu.setSplashDelay(0);
  build_menu();
  menu.init();
  menu.setMenuPageCurrent(pgNoMenu);
  menu.invertKeysDuringEdit(true);
	menu.setDrawMenuCallback(after_menu_update_GUI);
}