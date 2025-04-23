#include <MIDI.h>

#pragma once
#include <Adafruit_TinyUSB.h>   // library of code to get the USB port working
#include <MIDI.h>               // library of code to send and receive MIDI messages
#include "pico/time.h"
#include "pico/util/queue.h"
#include "debug.h"

Adafruit_USBD_MIDI usb_midi_over_Serial0;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi_over_Serial0, UMIDI);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, SMIDI);

void mount_tinyUSB() {
  //uint32_t mountTime = timer_hw->timerawl;
  while (!TinyUSBDevice.mounted()) {}
  //mountTime = timer_hw->timerawl - mountTime;
  //Serial.begin(115200);
  //debug.add("it took ");
  //debug.add_num(mountTime);
  //debug.add("uS to mount TinyUSB.\n");
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
  uint8_t MPE_zone_left;
  uint8_t MPE_zone_right;
  queue_t MPE_channel_queue;

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
  void sendNoteOff(uint8_t note, uint8_t velo, uint8_t ch)    {send(0x80,note,velo,ch);}
  void sendNoteOn(uint8_t note, uint8_t velo, uint8_t ch)     {send(0x90,note,velo,ch);}
  void sendAfterTouch(uint8_t note, uint8_t pres, uint8_t ch) {send(0xA0,note,pres,ch);}
  void sendCC(uint8_t cc, uint8_t value, uint8_t ch)          {send(0xB0,cc,value,ch);}
  void sendMod(uint8_t value, uint8_t ch)                     {sendCC(0x01,value,ch);}
  uint8_t MSB(uint16_t n)                                     {return (n >> 7) & 0x7F;}
  uint8_t LSB(uint16_t n)                                     {return n & 0x7F;}  
  void sendRPN(uint16_t bank, uint16_t value, uint8_t ch) {
    sendCC(0x64, LSB(bank), ch);
    sendCC(0x65, MSB(bank), ch);
    sendCC(0x06, LSB(value), ch);
    sendCC(0x26, MSB(value), ch);
    sendCC(0x64, 0x7F, ch);
    sendCC(0x65, 0x7F, ch);
  }
  void sendPitchBendRange(uint8_t semitones, uint8_t ch) {
    sendRPN(0x00, semitones << 7, ch);
  }

  // 2 Tune 1 MicroTune, 3 tuning prog 4 tuning bank

  void sendMPEzone(uint8_t sizeOfZone, uint8_t masterCh) {
    sendRPN(0x06, sizeOfZone << 7, masterCh);
  }
  void sendPC(uint8_t pc, uint8_t ch)                         {send(0xC0,pc,0x00,ch);}
  void sendAfterTouch(uint8_t pres, uint8_t ch)               {send(0xD0,pres,0,ch);}
  void sendPitchBend(int16_t value, uint8_t ch) { uint16_t pb = uint16_t(value + 8192);
                                                  send(0xE0,LSB(pb),MSB(pb),ch);}
  void sendSysEx(const uint8_t* dataArray, size_t length) {
    if (_ptr_UMIDI_active != nullptr) {
      if (*_ptr_UMIDI_active) {UMIDI.sendSysEx(length, dataArray, false);}
    }
    if (_ptr_SMIDI_active != nullptr) {
      if (*_ptr_SMIDI_active) {SMIDI.sendSysEx(length, dataArray, false);}
    }
  }

  

  void reset_MPE_queue() {
    uint8_t discard;
    while (!queue_is_empty(&MPE_channel_queue)) {
      queue_try_remove(&MPE_channel_queue, &discard);
    }
    uint8_t i = 2;
    while (i <= MPE_zone_left) {
      bool success = queue_try_add(&MPE_channel_queue, &i);
      i += (uint8_t)success;
    }
    if (MPE_zone_right) {
      i = MPE_zone_right;
      while (i <= 15) {
        bool success = queue_try_add(&MPE_channel_queue, &i);
        i += (uint8_t)success;
      }
    }
  }
  void switch_on_MPE() {
    // PB range
    // MPE zones
    reset_MPE_queue();
  }

  void set_mode(int m) {
    tuning_mode = m;
    switch (m) {

      case _MIDImode_MPE: {
        switch_on_MPE();
        break;
      }
      default:
        break;
    }    
  }
/*
  int16_t getBend(uint8_t pitchBendRange) {
    return midiBend / pitchBendRange;
  }
  void parseMidiPitch() {
    midiNote = round(midiPitch);
    midiBend = round(ldexp(midiPitch - (double)midiNote, 14));
  }
*/

  void noteOn(uint8_t channel, uint8_t table, double note, uint8_t velocity) {
    switch (tuning_mode) {
      case _MIDImode_standard:
      case _MIDImode_tuning_table: {
        sendNoteOn(table, velocity, channel);
        break;
      }
      // MTS mode and 1.0 mode:
      // noteOn, interpret straight
      // CC messages, interpret straight
      // MTS, have a tuning table dump first.
      // also allow for option to listen for
      // bulk tuning message
      case _MIDImode_MPE: {
        // get algorithm from hb 1.1

      }
      // noteOn, assign open channel, bend pitch
      // CC messages, send to master channel (1/16)

      case _MIDImode_2_point_oh: {

      }
      // different format, look at spec

      default: 
        break;
    }
  }

  void noteOff(uint8_t channel, uint8_t table, double note, uint8_t velocity) {
    switch (tuning_mode) {
      case _MIDImode_standard:
      case _MIDImode_tuning_table: {
        sendNoteOff(table, velocity, channel);
        break;
      }
      // MTS mode and 1.0 mode:
      // noteOn, interpret straight
      // CC messages, interpret straight
      // MTS, have a tuning table dump first.
      // also allow for option to listen for
      // bulk tuning message
      case _MIDImode_MPE: {
        // get algorithm from hb 1.1

      }
      // noteOn, assign open channel, bend pitch
      // CC messages, send to master channel (1/16)

      case _MIDImode_2_point_oh: {

      }
      // different format, look at spec

      default: 
        break;
    }
  }


};

MIDI_API_Object        MIDI_api;
void init_MIDI() {
  usb_midi_over_Serial0.setStringDescriptor("HexBoard MIDI");  // Initialize MIDI, and listen to all MIDI channels
  UMIDI.begin(MIDI_CHANNEL_OMNI);                 // This will also call usb_midi's begin()
  SMIDI.begin(MIDI_CHANNEL_OMNI);
  queue_init(&(MIDI_api.MPE_channel_queue), sizeof(uint8_t), 15);
}

