#pragma once
#include <stdint.h>
#include "color.h"
#include <string>
/*  
 *  This is a library of scale, palette, and other musical definitions
 *  Also some background stuff for the menus
 */

// this can be cast as a "SelectOptionInt" type
// to be used as a menu dropdown
struct Library_Entry {
	const char* label;
	int value;
};

enum {
  rgb_W = 0xFFFFFF,
  rgb_R = 0xFF0000,
  rgb_O = 0xFF7F00,
  rgb_Y = 0xFFFF00,
  rgb_L = 0x7FFF00,
  rgb_G = 0x00FF00,
  rgb_C = 0x00FFFF,
  rgb_A = 0x007FFF,
  rgb_B = 0x0000FF,
  rgb_P = 0x7F00FF,
  rgb_M = 0xFF00FF,
};

const uint16_t scale_definition[] = {
  0b101011010101, //  0. Major:     C  -  D  -  E  F  -  G  -  A  -  B
  0b101101011010, //  1. Minor-nat: C  -  D  Eb -  F  -  G  Ab -  Bb -
  0b101101010101, //  2. Minor-mel: C  -  D  Eb -  F  -  G  -  A  -  B
  0b101101011001, //  3. Minor-har: C  -  D  Eb -  F  -  G  Ab -  -  B
  0b101010010100, //  4. Penta-maj: C  -  D  -  E  -  -  G  -  A  -  -
  0b100101010010, //  5. Penta-min: C  -  -  Eb -  F  -  G  -  -  Bb -
  0b100111110010, //  6. Blues:     C  -  -  D# E  F  Gb G  -  -  A# -
  0b110101011010, //  7. Phrygian:  C  Db -  Eb -  F  -  G  Ab -  Bb -
  0b110011011010, //  8. Phryg Dom: C  Db -  -  E  F  -  G  Ab -  Bb -
  0b110011011001, //  9. Dbl Hrmnc: C  Db -  -  E  F  -  G  Ab -  -  B
  0b101101010110, // 10. Dorian:    C  -  D  Eb -  F  -  G  -  A  Bb -
  0b101010110101, // 11. Lydian:    C  -  D  -  E  -  F# G  -  A  -  B
  0b101010110110, // 12. LydianDom: C  -  D  -  E  -  F# G  -  A  Bb -
  0b101011010110, // 13. Mixolyd'n: C  -  D  -  E  F  -  G  -  A  Bb -
  0b110101101010, // 14. Locrian:   C  Db -  Eb -  F  Gb -  Ab -  Bb -
  0b101010101010, // 15. WholeTone: C  -  D  -  E  -  F# -  G# -  Bb -
  0b101101101101, // 16. Octatonic: C  -  D  Eb -  F  Gb -  G# A  -  B
};

Library_Entry list_of_scales[] = {
  {"Major",     0},
  {"Minor-nat", 1},
  {"Minor-mel", 2},
  {"Minor-hrm", 3},
  {"Penta-maj", 4},
  {"Penta-min", 5},
  {"Blues",     6},
  {"Phrygian",  7},
  {"Phryg Dom", 8},
  {"Dbl Hrmnc", 9},
  {"Dorian",   10},
  {"Lydian",   11},
  {"LydianDom",12},
  {"Mixolyd'n",13},
  {"Locrian",  14},
  {"WholeTone",15},
  {"Octatonic",16}
};

// to customize drop downs, you have to:
// 1) create C++ standard strings
// 2) make sure they have global scope
// 3) use .c_str() to create const char*
//    that points to those strings
std::string string_array_MIDI_pitch_names[128];
Library_Entry list_of_MIDI_pitch_names[128];
void fill_MIDI_pitch_names(std::string *refArray, Library_Entry *refLib, int leading_spaces, int trailing_spaces) {
  std::string spelling = " CC# DEb E FF# GG# A Bb B";
  int n;
  int o;
  for (int i = 0; i < 128; ++i) {
    n = i % 12;
    o = (i / 12) - 1;
    for (int j = 0; j < leading_spaces + (o >= 0); ++j) {
      refArray[i] += " ";
    }
    refArray[i] += spelling.substr(2 * n, 2);
    refArray[i] += std::to_string(o);
    for (int j = 0; j < trailing_spaces; ++j) {
      refArray[i] += " ";
    }
    refLib[i].label = refArray[i].c_str();
    refLib[i].value = i;
  }
}

std::string string_array_coarse_pitch[200];
Library_Entry list_of_coarse_pitch[200];
void fill_coarse_pitch_values(std::string *refArray, Library_Entry *refLib, int leading_spaces, int trailing_spaces) {
  int p;
  int n;
  for (int i = 0; i < 100; ++i) {
    p = 100 + i;
    n = 99 - i;
    for (int j = 0; j < leading_spaces + (i < 10); ++j) {
      refArray[p] += " ";
      refArray[n] += " ";
    }
    refArray[p] += "+" + std::to_string(i);
    refArray[n] += "-" + std::to_string(i);
    for (int j = 0; j < trailing_spaces; ++j) {
      refArray[p] += " ";
      refArray[n] += " ";
    }
    refLib[p].label = refArray[p].c_str();
    refLib[p].value = i;
    refLib[n].label = refArray[n].c_str();
    refLib[n].value = -i;
  }
  refLib[99].value = -100; // differentiate between -0 and +0
}

std::string string_array_fine_pitch[100];
Library_Entry list_of_fine_pitch[100];
void fill_fine_pitch_values(std::string *refArray, Library_Entry *refLib, int leading_spaces, int trailing_spaces) {
  for (int i = 0; i < 100; ++i) {
    for (int j = 0; j < leading_spaces; ++j) {
      refArray[i] += " ";
    }
    refArray[i] += ".";
    if (i < 10) {
      refArray[i] += "0";
    }
    refArray[i] += std::to_string(i);
    refArray[i] += "¢";
    for (int j = 0; j < trailing_spaces; ++j) {
      refArray[i] += " ";
    }
    refLib[i].label = refArray[i].c_str();
    refLib[i].value = i;
  }
}

Library_Entry list_of_MT32_instrument_patches[] = {
  {"Acoustic Piano 1",    1},  {"Acoustic Piano 2",    2}, 
  {"Acoustic Piano 3",    3},  {"Electric Piano 1",    4},
  {"Electric Piano 2",    5},  {"Electric Piano 3",    6},
  {"Electric Piano 4",    7},  {"Honkytonk",           8},
  {"Electric Organ 1",    9},  {"Electric Organ 2",   10},
  {"Electric Organ 3",   11},  {"Electric Organ 4",   12},
  {"Pipe Organ 1",       13},  {"Pipe Organ 2",       14},
  {"Pipe Organ 3",       15},  {"Accordion",          16},
  {"Harpsichord 1",      17},  {"Harpsichord 2",      18},
  {"Harpsichord 3",      19},  {"Clavinet 1",         20},
  {"Clavinet 2",         21},  {"Clavinet 3",         22},
  {"Celesta 1",          23},  {"Celesta 2",          24},
  {"Synth Brass 1",      25},  {"Synth Brass 2",      26},
  {"Synth Brass 3",      27},  {"Synth Brass 4",      28},
  {"Synth Bass 1",       29},  {"Synth Bass 2",       30},
  {"Synth Bass 3",       31},  {"Synth Bass 4",       32},
  {"Fantasy",            33},  {"Harmo Pan",          34},
  {"Chorale",            35},  {"Glasses",            36},
  {"Soundtrack",         37},  {"Atmosphere",         38},
  {"Warm Bell",          39},  {"Funny Vox",          40},
  {"Echo Bell",          41},  {"Ice Rain",           42},
  {"Oboe 2001",          43},  {"Echo Pan",           44},
  {"Doctor Solo",        45},  {"School Daze",        46},
  {"Bellsinger",         47},  {"Square Wave",        48},
  {"String Section 1",   49},  {"String Section 2",   50},
  {"String Section 3",   51},  {"Pizzicato",          52},
  {"Violin 1",           53},  {"Violin 2",           54},
  {"Cello 1",            55},  {"Cello 2",            56},
  {"Contrabass",         57},  {"Harp 1",             58},
  {"Harp 2",             59},  {"Guitar 1",           60},
  {"Guitar 2",           61},  {"Elec Gtr 1",         62},
  {"Elec Gtr 2",         63},  {"Sitar",              64},
  {"Acou Bass 1",        65},  {"Acou Bass 2",        66},
  {"Elec Bass 1",        67},  {"Elec Bass 2",        68},
  {"Slap Bass 1",        69},  {"Slap Bass 2",        70},
  {"Fretless 1",         71},  {"Fretless 2",         72},
  {"Flute 1",            73},  {"Flute 2",            74},
  {"Piccolo 1",          75},  {"Piccolo 2",          76},
  {"Recorder",           77},  {"Pan Pipes",          78},
  {"Sax 1",              79},  {"Sax 2",              80},
  {"Sax 3",              81},  {"Sax 4",              82},
  {"Clarinet 1",         83},  {"Clarinet 2",         84},
  {"Oboe",               85},  {"English Horn",       86},
  {"Bassoon",            87},  {"Harmonica",          88},
  {"Trumpet 1",          89},  {"Trumpet 2",          90},
  {"Trombone 1",         91},  {"Trombone 2",         92},
  {"French Horn 1",      93},  {"French Horn 2",      94},
  {"Tuba",               95},  {"Brass Section 1",    96},
  {"Brass Section 2",    97},  {"Vibe 1",             98},
  {"Vibe 2",             99},  {"Synth Mallet",      100},
  {"Windbell",          101},  {"Glock",             102},
  {"Tube Bell",         103},  {"Xylophone",         104},
  {"Marimba",           105},  {"Koto",              106},
  {"Sho",               107},  {"Shakuhachi",        108},
  {"Whistle 1",         109},  {"Whistle 2",         110},
  {"Bottleblow",        111},  {"Breathpipe",        112},
  {"Timpani",           113},  {"Melodic Tom",       114},
  {"Deep Snare",        115},  {"Elec Perc 1",       116},
  {"Elec Perc 2",       117},  {"Taiko",             118},
  {"Taiko Rim",         119},  {"Cymbal",            120},
  {"Castanets",         121},  {"Triangle",          122},
  {"Orchestra Hit",     123},  {"Telephone",         124},
  {"Bird Tweet",        125},  {"One Note Jam",      126},
  {"Water Bell",        127},  {"Jungle Tune",       128}
};

Library_Entry list_of_GM_instrument_patches[] = {
  {"Acoustic GrandPiano",  1},  {"BrightAcousticPiano",  2},
  {"Electric GrandPianp",  3},  {"Honky-tonk Piano",     4},
  {"Electric Piano 1",     5},  {"Electric Piano 2",     6},
  {"Harpsichord",          7},  {"Clavi",                8},
  {"Celesta",              9},  {"Glockenspiel",        10},
  {"Music Box",           11},  {"Vibraphone",          12},
  {"Marimba",             13},  {"Xylophone",           14},
  {"Tubular Bells",       15},  {"Dulcimer",            16},
  {"Drawbar Organ",       17},  {"Percussive Organ",    18},
  {"Rock Organ",          19},  {"Church Organ",        20},
  {"Reed Organ",          21},  {"Accordion",           22},
  {"Harmonica",           23},  {"Tango Accordion",     24},
  {"AcousticGuitarNylon", 25},  {"AcousticGuitarSteel", 26},
  {"ElectricGuitar Jazz", 27},  {"ElectricGuitarClean", 28},
  {"ElectricGuitarMuted", 29},  {"Overdrive Guitar",    30},
  {"Distortion Guitar",   31},  {"Guitar harmonics",    32},
  {"Acoustic Bass",       33},  {"ElectricBass Finger", 34},
  {"El Bass (pick)",      35},  {"Fretless Bass",       36},
  {"Slap Bass 1",         37},  {"Slap Bass 2",         38},
  {"Synth Bass 1",        39},  {"Synth Bass 2",        40},
  {"Violin",              41},  {"Viola",               42},
  {"Cello",               43},  {"Contrabass",          44},
  {"Tremolo Strings",     45},  {"Pizzicato Strings",   46},
  {"Orchestral Harp",     47},  {"Timpani",             48},
  {"String Ensemble 1",   49},  {"String Ensemble 2",   50},
  {"Synth Strings 1",     51},  {"Synth Strings 2",     52},
  {"Choir Aahs",          53},  {"Voice Oohs",          54},
  {"Synth Voice",         55},  {"Orchestra Hit",       56},
  {"Trumpet",             57},  {"Trombone",            58},
  {"Tuba",                59},  {"Muted Trumpet",       60},
  {"French Horn",         61},  {"Brass Section",       62},
  {"Synth Brass 1",       63},  {"Synth Brass 2",       64},
  {"Soprano Sax",         65},  {"Alto Sax",            66},
  {"Tenor Sax",           67},  {"Baritone Sax",        68},
  {"Oboe",                69},  {"English Horn",        70},
  {"Bassoon",             71},  {"Clarinet",            72},
  {"Piccolo",             73},  {"Flute",               74},
  {"Recorder",            75},  {"Pan Flute",           76},
  {"Blown Bottle",        77},  {"Shakuhachi",          78},
  {"Whistle",             79},  {"Ocarina",             80},
  {"Lead 1 (square)",     81},  {"Lead 2 (sawtooth)",   82},
  {"Lead 3 (calliope)",   83},  {"Lead 4 (chiff)",      84},
  {"Lead 5 (charang)",    85},  {"Lead 6 (voice)",      86},
  {"Lead 7 (fifths)",     87},  {"Lead 8 (bass+lead)",  88},
  {"Pad 1 (new age)",     89},  {"Pad 2 (warm)",        90},
  {"Pad 3 (poly synth)",  91},  {"Pad 4 (choir)",       92},
  {"Pad 5 (bowed)",       93},  {"Pad 6 (metallic)",    94},
  {"Pad 7 (halo)",        95},  {"Pad 8 (sweep)",       96},
  {"FX 1 (rain)",         97},  {"FX 2 (soundtrack)",   98},
  {"FX 3 (crystal)",      99},  {"FX 4 (atmosphere)",  100},
  {"FX 5 (brightness)",  101},  {"FX 6 (goblins)",     102},
  {"FX 7 (echoes)",      103},  {"FX 8 (sci-fi)",      104},
  {"Sitar",              105},  {"Banjo",              106},
  {"Shamisen",           107},  {"Koto",               108},
  {"Kalimba",            109},  {"Bag pipe",           110},
  {"Fiddle",             111},  {"Shanai",             112},
  {"Tinkle Bell",        113},  {"Agogo",              114},
  {"Steel Drums",        115},  {"Woodblock",          116},
  {"Taiko Drum",         117},  {"Melodic Tom",        118},
  {"Synth Drum",         119},  {"Reverse Cymbal",     120},
  {"Guitar Fret Noise",  121},  {"Breath Noise",       122},
  {"Seashore",           123},  {"Bird Tweet",         124},
  {"Telephone Ring",     125},  {"Helicopter",         126},
  {"Applause",           127},  {"Gunshot",            128}};