#include "MIDIUSB.h"

// these are the arduino pins that the motors are hooked up to
enum drumPins {kickPin = 2, perc2Pin = 3, perc3Pin = 4, strumPin = 5}; 

// these are the midi notes that each pin/pins triggers on, as well as an alternate for each
enum midiNotes {kickMidi = 0, perc2Midi = 2, perc3Midi = 4, strumMidi = 7, cmajorMidi = 60, cmajor7Midi = 61, dmajorMidi = 62, dmajor7Midi = 63, emajorMidi = 64, 
                fmajorMidi = 65, fmajor7Midi = 66, gmajorMidi = 67, gmajor7Midi = 68, amajorMidi = 69, amajor7Midi = 70, dminorMidi = 50, aminorMidi = 57, 
                eminorMidi = 52, aminor7Midi = 56};

void setup() {
  Serial.begin(115200);

  // setup all output pins
  for(int i=0; i<=13; i++) {
    pinMode(i, OUTPUT);
  }
}

void loop() {
    //listen for new MIDI messages
    midiEventPacket_t rx = MidiUSB.read();
    processMidi(rx);
}

void handleNoteOn(byte channel, byte pitch, byte velocity) {
  // it is possible to use the actual midi velocity here, just be sure to
  // double to value because midi is 0-127
  // and then change digitalWrite to analogWrite
  if(velocity > 0) { // auto set any velo > 0 to trigger a HIGH 
    velocity = HIGH;
  }else {
    velocity = LOW;
  }

  switch (pitch) {
    case kickMidi:
      Serial.print("Kick: ");
      digitalWrite(kickPin, velocity);
      break;
    case perc2Midi:
      Serial.print("Perc2: ");
      digitalWrite(perc2Pin, velocity);
      break;
    case perc3Midi:
      Serial.print("Perc3: "); 
      digitalWrite(perc3Pin, velocity); 
      break; 
    case strumMidi: 
      Serial.print("Strum: "); 
      digitalWrite(strumPin, velocity); 
      break; 
    case cmajorMidi: //0001 1 
      Serial.print("C: ");
      digitalWrite(10, velocity);
      break;
    case gmajorMidi: //0010 2 
      Serial.print("G: ");
      digitalWrite(11, velocity);
      break; 
    case dmajorMidi: //0011 3 
      Serial.print("D: ");
      digitalWrite(10, velocity);
      digitalWrite(11, velocity);
      break; 
    case amajorMidi: //0100 4  
      Serial.print("A: ");
      digitalWrite(12, velocity);
      break; 
    case emajorMidi: //0101 5 
      Serial.print("E: ");
      digitalWrite(12, velocity);
      digitalWrite(10, velocity);
      break;
    case fmajorMidi: //0110 6 
      Serial.print("F: ");
      digitalWrite(12, velocity);
      digitalWrite(11, velocity);
      break; 
    case cmajor7Midi: //0111 7 
      Serial.print("CM7: ");
      digitalWrite(12, velocity);
      digitalWrite(11, velocity);
      digitalWrite(10, velocity); 
      break;  
    case gmajor7Midi: //1000 8 
      Serial.print("GM7: ");
      digitalWrite(13, velocity);
      break;  
    case dmajor7Midi: //1001 9 
      Serial.print("DM7: ");
      digitalWrite(13, velocity);
      digitalWrite(10, velocity); 
      break;  
    case amajor7Midi: //1010 10  
      Serial.print("AM7: ");
      digitalWrite(13, velocity);
      digitalWrite(11, velocity);
      break;  
    case fmajor7Midi: //1011 11  
      Serial.print("FM7: ");
      digitalWrite(13, velocity);
      digitalWrite(11, velocity);
      digitalWrite(10, velocity); 
      break;  
    case dminorMidi: //1100 12 
      Serial.print("Dm: ");
      digitalWrite(13, velocity);
      digitalWrite(12, velocity);
      break;  
    case aminorMidi: //1101 13 
      Serial.print("Am: ");
      digitalWrite(13, velocity);
      digitalWrite(12, velocity);
      digitalWrite(10, velocity);
      break;  
    case eminorMidi: //1110 14  
      Serial.print("Em: ");
      digitalWrite(13, velocity);
      digitalWrite(12, velocity);
      digitalWrite(11, velocity);
      break;  
    case aminor7Midi: //1111 15 
      Serial.print("Am7: ");
      digitalWrite(13, velocity);
      digitalWrite(12, velocity);
      digitalWrite(11, velocity); 
      digitalWrite(10, velocity);
      break;  
    default:
      // print the midi note value, handy for adding new notes
      Serial.print("Note(");
      Serial.print(pitch);
      Serial.print("): ");
      break;
  }
  if(velocity == 0) {
    Serial.println("off");
  } else {
    Serial.println("on");
  }
}

void processMidi(midiEventPacket_t rx) {
    switch (rx.header) {
    case 0x0:
        // do nothing
        break;

    // note on
    case 0x9:
        handleNoteOn(rx.byte1 & 0xF, rx.byte2, rx.byte3);
        break;
    
    // note off
    case 0x8:
        handleNoteOn(rx.byte1 & 0xF, rx.byte2, 0);
        break;

    // control change
    case 11:
        Serial.print("CC: ");
        Serial.print(rx.byte2);
        Serial.print(":");
        Serial.print(rx.byte3);
        Serial.print("\n");
      break;

    default:
      //Serial.println(rx.header);
      break;
    }
}
