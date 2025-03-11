#pragma once
#include <Adafruit_TinyUSB.h>   // library of code to get the USB port working
#include <MIDI.h>               // library of code to send and receive MIDI messages
#include "pico/time.h"
#include "debug.h"

Adafruit_USBD_MIDI usb_midi_over_Serial0;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi_over_Serial0, UMIDI);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, SMIDI);

void mount_tinyUSB() {
  uint32_t mountTime = timer_hw->timerawl;
  while (!TinyUSBDevice.mounted()) {}
  mountTime = timer_hw->timerawl - mountTime;
  Serial.begin(115200);
  debug.add("it took ");
  debug.add_num(mountTime);
  debug.add("uS to mount TinyUSB.\n");
}

// a wrapper structure
// which keeps track of MIDI-related options
// in the hexBoard app, calls the correct
// MIDI.h functions and sends the right message
// format when asked to.
struct MIDI_API_Object {
  // for now this is only built to recognize two
  // simultaneous MIDI outs. the boolean flags
  // could be changed into an array but for now
  // not bothering with that.
  bool * _ptr_UMIDI_active; // can read this on-the-fly
  bool * _ptr_SMIDI_active; // can read this on-the-fly
  int tuning_mode;   // get/set function, need to send a msg on each change

  void send(uint8_t type, uint8_t data1, uint8_t data2, uint8_t ch) {
    if (_ptr_UMIDI_active != nullptr) {
      if (*_ptr_UMIDI_active) {
        UMIDI.send(
          static_cast<midi::MidiType>(type),
          static_cast<midi::DataByte>(data1),
          static_cast<midi::DataByte>(data2),
          static_cast<midi::Channel>(ch)
        );
      }
    }
    if (_ptr_SMIDI_active != nullptr) {
      if (*_ptr_SMIDI_active) {
        SMIDI.send(
          static_cast<midi::MidiType>(type),
          static_cast<midi::DataByte>(data1),
          static_cast<midi::DataByte>(data2),
          static_cast<midi::Channel>(ch)
        );
      }
    }
  }
  void noteOff(uint8_t note, uint8_t velo, uint8_t ch)    {send(0x80,note,velo,ch);}
  void noteOn(uint8_t note, uint8_t velo, uint8_t ch)     {send(0x90,note,velo,ch);}
  void CC(uint8_t cc, uint8_t value, uint8_t ch)          {send(0xB0,cc,value,ch);}
  // CC01 void modulate();
  // CC98-101 RPN, 0 PBS, 2 Tune 3 MicroTune, 3 tuning prog 4 tuning bank
  // RPN 6 MPE zones
  // 0xC0 void PC();
  void afterTouch(uint8_t note, uint8_t pres, uint8_t ch) {send(0xA0, note, pres, ch);}
  void afterTouch(uint8_t pres, uint8_t ch)               {send(0xD0, pres, 0, ch);}
  // 0xE0 void pitchBend();
  // 0xF0 sysex

  // MPE mode:
  // noteOn, assign open channel, bend pitch
  // CC messages, send to master channel (1/16)
  
  // MTS mode and 1.0 mode:
  // noteOn, interpret straight
  // CC messages, interpret straight
  // MTS, have a tuning table dump first.
  // also allow for option to listen for
  // bulk tuning message

  // MIDI 2.0
  // different format, look at spec


};



void init_MIDI() {
  usb_midi_over_Serial0.setStringDescriptor("HexBoard MIDI");  // Initialize MIDI, and listen to all MIDI channels
  UMIDI.begin(MIDI_CHANNEL_OMNI);                 // This will also call usb_midi's begin()
  SMIDI.begin(MIDI_CHANNEL_OMNI);
}

MIDI_API_Object        MIDI_api;