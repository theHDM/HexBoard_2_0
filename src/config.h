/* config.h
 *
 * This file defines hardware-specific constants
 * If you rewire or add peripherals, you'll need to
 * update this file or provide new constants to
 * link to other modules.
 *
 * Definition created 2025-01-31 for HexBoard Hardware v1.2
 *
 * Pinout
 *  0  Serial  (USB)
 *  1  Serial1 (MIDI)
 *  2  Multiplexer, bit 2 (0100)
 *  3  Multiplexer, bit 3 (1000)
 *  4  Multiplexer, bit 0 (0001)
 *  5  Multiplexer, bit 1 (0010)
 *  6  Key switch, column 0
 *  7  Key switch, column 1
 *  8  Key switch, column 2
 *  9  Key switch, column 3
 * 10  Key switch, column 4
 * 11  Key switch, column 5
 * 12  Key switch, column 6
 * 13  Key switch, column 7
 * 14  Key switch, column 8
 * 15  Key switch, column 9
 * 16  OLED display, I2C data pin
 * 17  OLED display, I2C clock pin
 * 18    -- open --
 * 19    -- open --
 * 20  Rotary knob, left
 * 21  Rotary knob, right
 * 22  Adafruit NeoPixel LED strip
 * 23  Piezoelectric buzzer
 * 24  Rotary knob, switch
 * 25  Audio out
 * 26    -- open --
 * 27    -- open --
 * 28    -- open --
 * 29    -- open --
 */

#pragma once
#include <stdint.h>
#include <map>

const uint8_t GPIO_pin_count = 32; // maximum size of certain object arrays

// If you rewire the HexBoard then change these pin values
const uint8_t muxPins[] = {4,5,2,3}; // 1bit 2bit 4bit 8bit
const uint8_t colPins[] = {6,7,8,9,10,11,12,13,14,15};

// 1 if analog (firmware 2.0), 0 if digital
const    bool analogPins[]  = {0,0,0,0,0,0,0,0,0,0};

const size_t mux_pins_count = sizeof(muxPins)/sizeof(muxPins[0]);  // should equal 4
const size_t col_pins_count = sizeof(colPins)/sizeof(colPins[0]);  // should equal 10
constexpr size_t mux_channels_count = 1 << mux_pins_count;         // should equal 16
constexpr size_t keys_count = mux_channels_count * col_pins_count; // should equal 160

constexpr size_t linear_index(uint8_t argM, uint8_t argC) {  // should return value 0 thru 159
  return (argC << mux_pins_count) + argM;
}

const uint8_t rotaryPinA = 20;
const uint8_t rotaryPinB = 21;
const uint8_t rotaryPinC = 24;
const uint8_t piezoPin = 23;
const uint8_t audioJackPin = 25;
const uint8_t synthPins[] = {piezoPin, audioJackPin};
const uint8_t ledPin = 22;
const uint8_t OLED_sdaPin = 16;
const uint8_t OLED_sclPin = 17;

const uint32_t highest_MIDI_note_Hz = 13290;
const uint32_t target_sample_rate_Hz = 2 * highest_MIDI_note_Hz;
constexpr int32_t audio_sample_interval_uS = 31250 / (target_sample_rate_Hz >> 5);
const int32_t key_poll_interval_uS = 96;         // ideal is 1/16th microsecond so the whole thing is under 1 millisecond.
const int32_t rotary_poll_interval_uS = 768; // tested at 512 microseconds and it was too short

const uint8_t LED_frame_rate_Hz = 60;
const uint8_t OLED_frame_rate_Hz = 24;
constexpr int32_t LED_poll_interval_mS = 1'000 / LED_frame_rate_Hz;
constexpr int32_t OLED_poll_interval_mS = 1'000 / OLED_frame_rate_Hz;

// TO-DO: test on hardware v2
const uint16_t default_analog_calibration_up = 480;
const uint16_t default_analog_calibration_down = 280;

const uint8_t  default_contrast = 64; // range: 0-127
const uint8_t  screensaver_contrast = 1; // range: 0-127

const uint8_t synth_polyphony_limit = 16;
const uint8_t audio_bits = 9;
constexpr uint16_t neutral_level = (1u << (audio_bits - 1)) - 1;

const size_t buttons_count = 140;  // based on the size of the NeoPixel installed
constexpr size_t hardwire_count = keys_count - buttons_count;

// physical coordinates & pin-out locations of each button
// ordered by NeoStrip pixel number (0 thru 139 on this version)
const int hexBoard_layout_hw_1_2[buttons_count][4] = {
  // x   y    mux   col  pixel
  {-10,  0, 0b0000, 0}, //   0 **
  { -8, -6, 0b0000, 1}, //   1
  { -6, -6, 0b0000, 2}, //   2
  { -4, -6, 0b0000, 3}, //   3
  { -2, -6, 0b0000, 4}, //   4
  {  0, -6, 0b0000, 5}, //   5
  {  2, -6, 0b0000, 6}, //   6
  {  4, -6, 0b0000, 7}, //   7
  {  6, -6, 0b0000, 8}, //   8
  {  8, -6, 0b0000, 9}, //   9
  { -9, -5, 0b0001, 0}, //  10
  { -7, -5, 0b0001, 1}, //  11
  { -5, -5, 0b0001, 2}, //  12
  { -3, -5, 0b0001, 3}, //  13
  { -1, -5, 0b0001, 4}, //  14
  {  1, -5, 0b0001, 5}, //  15
  {  3, -5, 0b0001, 6}, //  16
  {  5, -5, 0b0001, 7}, //  17
  {  7, -5, 0b0001, 8}, //  18
  {  9, -5, 0b0001, 9}, //  19
  {-11,  1, 0b0010, 0}, //  20 **
  { -8, -4, 0b0010, 1}, //  21
  { -6, -4, 0b0010, 2}, //  22
  { -4, -4, 0b0010, 3}, //  23
  { -2, -4, 0b0010, 4}, //  24
  {  0, -4, 0b0010, 5}, //  25
  {  2, -4, 0b0010, 6}, //  26
  {  4, -4, 0b0010, 7}, //  27
  {  6, -4, 0b0010, 8}, //  28
  {  8, -4, 0b0010, 9}, //  29
  { -9, -3, 0b0011, 0}, //  30
  { -7, -3, 0b0011, 1}, //  31
  { -5, -3, 0b0011, 2}, //  32
  { -3, -3, 0b0011, 3}, //  33
  { -1, -3, 0b0011, 4}, //  34
  {  1, -3, 0b0011, 5}, //  35
  {  3, -3, 0b0011, 6}, //  36
  {  5, -3, 0b0011, 7}, //  37
  {  7, -3, 0b0011, 8}, //  38
  {  9, -3, 0b0011, 9}, //  39
  {-10,  2, 0b0100, 0}, //  40 **
  { -8, -2, 0b0100, 1}, //  41
  { -6, -2, 0b0100, 2}, //  42
  { -4, -2, 0b0100, 3}, //  43
  { -2, -2, 0b0100, 4}, //  44
  {  0, -2, 0b0100, 5}, //  45
  {  2, -2, 0b0100, 6}, //  46
  {  4, -2, 0b0100, 7}, //  47
  {  6, -2, 0b0100, 8}, //  48
  {  8, -2, 0b0100, 9}, //  49
  { -9, -1, 0b0101, 0}, //  50
  { -7, -1, 0b0101, 1}, //  51
  { -5, -1, 0b0101, 2}, //  52
  { -3, -1, 0b0101, 3}, //  53
  { -1, -1, 0b0101, 4}, //  54
  {  1, -1, 0b0101, 5}, //  55
  {  3, -1, 0b0101, 6}, //  56
  {  5, -1, 0b0101, 7}, //  57
  {  7, -1, 0b0101, 8}, //  58
  {  9, -1, 0b0101, 9}, //  59
  {-11,  3, 0b0110, 0}, //  60 **
  { -8,  0, 0b0110, 1}, //  61
  { -6,  0, 0b0110, 2}, //  62
  { -4,  0, 0b0110, 3}, //  63
  { -2,  0, 0b0110, 4}, //  64
  {  0,  0, 0b0110, 5}, //  65
  {  2,  0, 0b0110, 6}, //  66
  {  4,  0, 0b0110, 7}, //  67
  {  6,  0, 0b0110, 8}, //  68
  {  8,  0, 0b0110, 9}, //  69
  { -9,  1, 0b0111, 0}, //  70
  { -7,  1, 0b0111, 1}, //  71
  { -5,  1, 0b0111, 2}, //  72
  { -3,  1, 0b0111, 3}, //  73
  { -1,  1, 0b0111, 4}, //  74
  {  1,  1, 0b0111, 5}, //  75
  {  3,  1, 0b0111, 6}, //  76
  {  5,  1, 0b0111, 7}, //  77
  {  7,  1, 0b0111, 8}, //  78
  {  9,  1, 0b0111, 9}, //  79
  {-10,  4, 0b1000, 0}, //  80 **
  { -8,  2, 0b1000, 1}, //  81
  { -6,  2, 0b1000, 2}, //  82
  { -4,  2, 0b1000, 3}, //  83
  { -2,  2, 0b1000, 4}, //  84
  {  0,  2, 0b1000, 5}, //  85
  {  2,  2, 0b1000, 6}, //  86
  {  4,  2, 0b1000, 7}, //  87
  {  6,  2, 0b1000, 8}, //  88
  {  8,  2, 0b1000, 9}, //  89
  { -9,  3, 0b1001, 0}, //  90
  { -7,  3, 0b1001, 1}, //  91
  { -5,  3, 0b1001, 2}, //  92
  { -3,  3, 0b1001, 3}, //  93
  { -1,  3, 0b1001, 4}, //  94
  {  1,  3, 0b1001, 5}, //  95
  {  3,  3, 0b1001, 6}, //  96
  {  5,  3, 0b1001, 7}, //  97
  {  7,  3, 0b1001, 8}, //  98
  {  9,  3, 0b1001, 9}, //  99
  {-11,  5, 0b1010, 0}, // 100 **
  { -8,  4, 0b1010, 1}, // 101
  { -6,  4, 0b1010, 2}, // 102
  { -4,  4, 0b1010, 3}, // 103
  { -2,  4, 0b1010, 4}, // 104
  {  0,  4, 0b1010, 5}, // 105
  {  2,  4, 0b1010, 6}, // 106
  {  4,  4, 0b1010, 7}, // 107
  {  6,  4, 0b1010, 8}, // 108
  {  8,  4, 0b1010, 9}, // 109
  { -9,  5, 0b1011, 0}, // 110
  { -7,  5, 0b1011, 1}, // 111
  { -5,  5, 0b1011, 2}, // 112
  { -3,  5, 0b1011, 3}, // 113
  { -1,  5, 0b1011, 4}, // 114
  {  1,  5, 0b1011, 5}, // 115
  {  3,  5, 0b1011, 6}, // 116
  {  5,  5, 0b1011, 7}, // 117
  {  7,  5, 0b1011, 8}, // 118
  {  9,  5, 0b1011, 9}, // 119
  {-10,  6, 0b1100, 0}, // 120 **
  { -8,  6, 0b1100, 1}, // 121
  { -6,  6, 0b1100, 2}, // 122
  { -4,  6, 0b1100, 3}, // 123
  { -2,  6, 0b1100, 4}, // 124
  {  0,  6, 0b1100, 5}, // 125
  {  2,  6, 0b1100, 6}, // 126
  {  4,  6, 0b1100, 7}, // 127
  {  6,  6, 0b1100, 8}, // 128
  {  8,  6, 0b1100, 9}, // 129
  { -9,  7, 0b1101, 0}, // 130
  { -7,  7, 0b1101, 1}, // 131
  { -5,  7, 0b1101, 2}, // 132
  { -3,  7, 0b1101, 3}, // 133
  { -1,  7, 0b1101, 4}, // 134
  {  1,  7, 0b1101, 5}, // 135
  {  3,  7, 0b1101, 6}, // 136
  {  5,  7, 0b1101, 7}, // 137
  {  7,  7, 0b1101, 8}, // 138
  {  9,  7, 0b1101, 9}  // 139
};