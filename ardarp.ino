/*
  MIDI note player

  This sketch shows how to use the serial transmit pin (pin 1) to send MIDI note data.
  If this circuit is connected to a MIDI synth, it will play the notes
  F#-0 (0x1E) to F#-5 (0x5A) in sequence.

  The circuit:
  - digital in 1 connected to MIDI jack pin 5
  - MIDI jack pin 2 connected to ground
  - MIDI jack pin 4 connected to +5V through 220 ohm resistor
  - Attach a MIDI cable to the jack, then to a MIDI synth, and play music.

  created 13 Jun 2006
  modified 13 Aug 2012
  by Tom Igoe

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/Midi
*/
#include <MIDI.h>


// ANALOG READS
int in_pause = 0; // delay between notes
int in_gate  = 1; // ratio of in_pause being gate time [0..1]
int in_swing = 2; // in_pause distribution between two steps [0.1 .. 0.9]
int in_range = 3; // how many notes to play [1..12]
int in_scale = 4; // choose different scales:
int in_mode  = 5; // pattern
int in_note  = 6; // base note of the arpeggio.
                  // might get replaced with MIDI in in the future
                  // maybe not

// DIGITAL READS
int in_sync = 2; // get external clock signal
int in_inex = 3; // choose between internal and external clock signal

// DIGITAL OUTS
int out_gate = 7; // gate out + LED

// MIDI
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
const int channel = 1;
int midi_velocity = 100;//velocity of MIDI notes, must be between 0 and 127 //(higher velocity usually makes MIDI instruments louder)

// AUXILIARY
int scale_raw; // analog read value to identify scale type
               // actual scale locally in loop()
int mode_raw;  // analog read value to identify order type
double gate;   // fraction of the beat, that is gate time [0.1 - 0.9]
double swing;  // fraction of two beats, that is part of the first beat
               // low -> short - long
               // middle -> equal lengths
               // high -> long - short
int range;     // number different notes played
               // in unconstrained random mode instead
               // range aroung base note to pick random notes
// int sync;      // work in progress....
// int inex;     // toggle between internal and external clock
int n;
int pause;     // stores delaytime locally
int *sequence;
int *updown;
// int buttonState;
int note = 24;   // 24 is the lowest note yamaha psr300 can play
int notes = { note }; // old nondinamic approach...


void setup() {
  MIDI.begin();
  pinMode(out_gate, OUTPUT);  // declare the ledPin as an OUTPUT
  // work in progress
  // pinMode (in_inex, INPUT);
  // pinMode (in_inex, INPUT_PULLUP);

  randomSeed(187); //jäijäijäijäha
}


void loop() {
  /* READ VALUES */
  // ANALOG
  swing = analogRead(in_swing) / 2024 + 0.25;
  range = analogRead(in_range) / 60 + 1;
  note = 24 + analogRead(in_note) / 9 ; 
  scale_raw = analogRead(in_scale); //    1023/5=205
  mode_raw  = analogRead(in_mode);
  // DIGITAL
  // sync  = analogRead(in_sync);
  // inex = analogRead(in_inex);

  /* CREATE SCALE */
  int major[24] = {0, 4, 7, 12, 16, 19, 24, 28, 31, 36, 40, 43, 48, 52, 55, 60, 64, 67, 72, 76, 79, 84, 88, 91};
  int minor[24] = {0, 3, 7, 12, 15, 19, 24, 27, 31, 36, 39, 43, 48, 51, 55, 60, 63, 67, 72, 75, 79, 84, 87, 91};
  int terze[24] = {0, 3, 6,  9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57, 60, 63, 66, 69};
  int singl[24] = {0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 168, 180, 192, 204, 216, 228, 240, 252, 264, 276};
  int scale[24] = {0, 4, 7, 12, 16, 19, 24, 28, 31, 36, 40, 43, 48, 52, 55, 60, 64, 67, 72, 76, 79, 84, 88, 91}; // acutally just a dummy. could be any values
  if (scale_raw > 768) {        // MINOR
    for (int i = 0; i < 24; i++) {
      scale[i] = minor[i];
    }
  } else if (scale_raw > 512) { // MAJOR
    for (int i = 0; i < 24; i++) {
      scale[i] = major[i];
    }
  } else if (scale_raw > 256) { // TERZ
    for (int i = 0; i < 24; i++) {
      scale[i] = terze[i];
    }
  } else {                      // WHOLE OCTAVE
    for (int i = 0; i < 24; i++) { 
      scale[i] = singl[i];
    }
  }
  int notes[range];
  int notecount = 0;
  notecount = range;
  // apply to base note
  for (int i = 0; i < range; i++) {
    notes[i] = note + scale[i];
  }


  /* APPLY PATTERN */
  if (mode_raw > 800) { // FULLY RANDOM
    updown = new int[notecount];
    // RANGE value is used differently in this mode
    // random values with range +- range around base note
    int noterange = range * 2 ;
    for (int j = 0; j < notecount; j++) {
      for (int k = 0; k < notecount; k++) {
        for (int i = 0; i < notecount; i++) {
          // recursive structure ensures randomness
          // random does not seem to work well in simple for structure
          updown[i] = random(note - noterange, note + noterange);
        }
      }
    }
    sequence = updown;

  } else if (mode_raw > 600) {  // CONSTRAINED RANDOM
    // shuffles the previously obtained notes
    for (int j = 0; j < notecount; j++) {
      for (int k = 0; k < notecount; k++) {
        for (int i = 0; i < notecount; i++) {
          // recursive structure ensures randomness
          // random does not seem to work well in simple for structure
          int r = random(i); // chose one
          if (r != i) // exchange, basically bubble "unsort"
          {
            int temp = notes[i];
            notes[i] = notes[r];
            notes[r] = temp;
          }
        }
      }
    }
    sequence = notes;

  } else if (mode_raw > 400) { // UP+DOWN
    int down[notecount];
    updown = new int[notecount * 2 - 2];
    for (int i = 0; i < notecount; i++) {
      down[i] = notes[notecount - i - 1];
    }
    for (int i = 0; i < notecount; i++) {
      updown[i] = notes[i];
    }
    for (int i = 0; i < notecount - 1; i++) {
      updown[notecount + i] = down[i];
    }
    sequence = updown;
    notecount = notecount * 2 - 2;

  } else if (mode_raw > 200) { // DOWN
    int down[notecount];
    for (int i = 0; i < notecount; i++) {
      down[i] = notes[notecount - i - 1];
    }
    //   sequence = down;
    for (int i = 0; i < notecount; i++) {
      notes[i] = down[i];
    }
    sequence = notes;

  } else { // UP
    sequence = notes;
  }


  /* PLAY NOTES */
  for (int i = 0; i < notecount; i++) {
    n = sequence[i];

    gate  = analogRead(in_gate) / (double)1024;
    swing  = analogRead(in_swing) / (double)1024 * 0.8 + 0.2;

    MIDI.sendNoteOn(n, midi_velocity, channel);
    digitalWrite(out_gate, HIGH);   // turn the LED on (HIGH is the voltage level)
    if ( i % 2 == 0) {
      // even
      pause = (analogRead(in_pause) + 25) * 3 * gate * swing;
    } else {
      // odd
      pause = (analogRead(in_pause) + 25) * 3 * gate * (1 - swing);
    }
    delay(pause);

    MIDI.sendNoteOff(n, midi_velocity, channel);
    digitalWrite(out_gate, LOW);
    if ( i % 2 == 0) {
      // even
      pause = (analogRead(in_pause) + 25) * 3 * (1 - gate) * swing;
    } else {
      // odd
      pause = (analogRead(in_pause) + 25) * 3 * (1 - gate) * (1 - swing);
    }
    delay(pause);

    /* abandoned sync in
      //   buttonState = digitalRead(in_inex);
       buttonState = digitalRead(in_sync);
        if (buttonState == HIGH) {
          while (buttonState == HIGH) {
            delay(5);
            buttonState = digitalRead(in_sync);
          }
          while (buttonState == LOW) {
            delay(5);
            buttonState = digitalRead(in_sync);
          }
       }
    */

  }
}













// abandoned attempt to improve readability
int pause_time(int even, bool noteon) {
  int pause = analogRead(in_pause);
  int gate  = analogRead(in_gate) / (double)1024;
  int swing  = analogRead(in_swing) / (double)1024 * 0.8 + 0.2;
  if ( even % 2 == 1) {
    swing = (1 - swing);
  }
  if (not noteon) {
    gate = 1 - gate;
  }
  pause = pause + 25;
  pause = pause * 3;
  pause = pause * gate;
  pause = pause * swing;
  return pause;
}
