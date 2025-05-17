/*
 # This is the code for the Aruino UNO linked with the
   LSS motors for the strumming and percussion subsytems 
   
 # Hardwares:
 1. Arduino UNO
 2. LSS-ADA Board 
 3. LSS-HT1, LSS-HS1 servos 
 4. Power supply:12V into ADA board 

 */

#include <SPI.h>
#include <LSS.h> 
#include <SoftwareSerial.h> // to defer LSS commands away from regular Serial, LSS.h file must be edited to accomodate 
#include "pins_arduino.h"

#define LSS_BAUD  (LSS_DefaultBaud)
// Choose the proper serial port for your platform
#define LSS_SERIAL  (Serial)  // ex: Many Arduino boards

// HT1 motors (percussion) 
LSS kickLSS = LSS(1); // kick LSS motor 
LSS perc2LSS = LSS(2); // string slap motor 
LSS perc3LSS = LSS(3); // additional percussion motor 

// HS1 motors (strum + fret) 
LSS strumLSS = LSS(4); // strum LSS motor 
 
LSS Estring3 = LSS(8); // 3 CCW 
LSS Astring3 = LSS(11); // 3 CW, 2 CCW 
LSS Astring2 = LSS(5); // 2 CW, 3 CCW
LSS Dstring2 = LSS(6); // 2 CW, 1 CCW 
LSS Gstring2 = LSS(10); // 2 CW, 3 CCW 
LSS Bstring1 = LSS(7); // 2 CW, 1 CCW 
LSS hEstring2 = LSS(9); // 1 CW, 3 CCW 

// ALL MOTORS SET TO CW (except strum), RESETTING TO 0 or other abnormal movement CAN MOVE THEM CCW!!!! 

// OF NOTE: this version of code only uses one of the two spools for fretting - the commented values beside the motors are invalid, only the fret name location is pressed with CW motion 

SoftwareSerial LSSserial(8,9); // stated before, get LSS commands out of Serial 

const int kickPin = 2;  
const int perc2Pin = 3;
const int perc3Pin = 4; 
const int strumPin = 5; // strumPin can be used to create strumming patterns if regular chord interval is too long or is required by song 
                        // no implementation of this was done for songs the team created 
                        
int INpin; // current digitalPin read 
int activeChord = 0; // outside chord range for initial status 

//previous time states for pin-to-pin & general chord HIGHs 
unsigned long twoMillis;
unsigned long threeMillis;
unsigned long fourMillis;
unsigned long fiveMillis; 

unsigned long chordMillis; 

const long interval = 50;  // 50ms between pin-to-pin input before allowed to pass 
const long chordInterval = 400; // 400ms between chords before allowed to pass 

int pinState[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // doesn't need to be 10 but it is, flag for digitalPins 
int strumState = 0; // flag for strummer/chords 

int binaryPins = 0; // binary value of PORTB pins 

void setup (void)
{
  Serial.begin (115200);
  digitalWrite(SS, HIGH);  
  SPI.begin ();
  SPI.setClockDivider(SPI_CLOCK_DIV8);

  // Initialize the LSS bus
  LSS::initBus(LSSserial, LSS_BAUD);

  // Wait for the LSS to boot
  delay(2000);
  
  // INPUT initialization not need but it's here :) 
  pinMode(kickPin, INPUT); 
  pinMode(perc2Pin, INPUT); 
  pinMode(perc3Pin, INPUT);
  pinMode(strumPin, INPUT); 
  pinMode(10, INPUT); // 2^0
  pinMode(11, INPUT); // 2^1
  pinMode(12, INPUT); // 2^2
  pinMode(13, INPUT); // 2^3

  // preset percussion motors to primary position 
  kickLSS.move(200);
  perc2LSS.move(200);
  perc3LSS.move(200);

  // preset strum and chords to primary position
  strumLSS.move(0); 
  
  Estring3.move(0); 
  Astring3.move(0); 
  Astring2.move(0); 
  Dstring2.move(0); 
  Gstring2.move(0);
  Bstring1.move(0); 
  hEstring2.move(0); 

  // cycle power to easily reset motors, otherwise digital position will be read and processes above can take loooong time

  binaryPins = 0;
}

void loop (void){           // continuosly check all pins for signal, then go to handleNoteOn or handleChordOn 
  unsigned long currentMillis = millis();
                          
  //CHECK CHORDS 
  int binaryPins = readPinsFastStable();

  if(binaryPins > 0){
    //Serial.println(binaryPins);
    handleChordOn(binaryPins);
    binaryPins = 0; 
  }
  
  if(currentMillis - chordMillis >= chordInterval && strumState == 1){  // moves strummer 400ms after fretboard was actuated 
    strumLSS.moveRelative(1800);
    //Serial.println("strum happened");
    strumState = 0; 
  }

  //CHECK PERCUSSION + STRUM 
  
  for(int i=2;i<=5;i++){
    if(digitalRead(i)==HIGH){
      switch(i){
        case 2: // kick 
          if (currentMillis - twoMillis >= interval) {
            twoMillis = currentMillis;
            pinState[i] = 0; 
          }
          break; 
        case 3: // perc2 
          if (currentMillis - threeMillis >= interval) {
            threeMillis = currentMillis;
            pinState[i] = 0; 
          }
          break; 
        case 4: // perc3 
          if (currentMillis - fourMillis >= interval) {
            fourMillis = currentMillis;
            pinState[i] = 0; 
          }
          break; 
        case 5: // strum 
          if (currentMillis - fiveMillis >= interval) {
            fiveMillis = currentMillis; 
            pinState[i] = 0; 
          }
          break;
      }
    }
    
    if(digitalRead(i)==HIGH && pinState[i]==0){ 
        INpin = i;
        handleNoteOn(INpin); 
        pinState[i] = 1; 

    }
  }  
}

int readPinsFastRaw() {
  uint8_t raw = PINB;           // Read PORTB register (pins 8–13)
  return (raw >> 2) & 0x0F;     // Extract bits 2–5 (pins 10–13), mask to 4 bits
}

int readPinsFastStable() {  // sometimes raw value was incorrect, this does two readings and won't pass unless they = each other 
  int first = readPinsFastRaw();
  delayMicroseconds(10);  // Tiny delay between reads
  int second = readPinsFastRaw();

  if (first == second) {
    return first;
  } else {
    return 0; // Unstable reading
  }
}

void handleNoteOn(int INpin){
  switch(INpin){ 
    case 2: // kick 
      kickLSS.moveRelative(900);
      //Serial.println("kick");
      break;
    case 3: // perc2
      perc2LSS.moveRelative(900);
      //Serial.println("perc2");
      break;
    case 4: // perc3
      perc3LSS.moveRelative(900);
      //Serial.println("perc3");
      break;
    case 5: // strum 
      strumLSS.moveRelative(1800); 
      //Serial.println("strum"); 
      break; 
    default:
      kickLSS.moveRelative(0);  // stop motors (not really needed) 
      perc2LSS.moveRelative(0);
      perc3LSS.moveRelative(0); 
      delay(1);               // need this otherwise will loop case command, trade-off hits must be >1 ms apart in Ableton 
      break;
  }
}

void handleChordOn(int binaryPins){
  activeChord = binaryPins; // why did i do this, idk 
  chordMillis = millis(); 
  switch(activeChord){ 
    case 1: //C  
      strumState = 1; 
      //Serial.println("C note on");
      fretChord(activeChord); 
      delay(15); // these delays are not the best and a system as used with pin-to-pin BlinkWithoutDelay would be better suited, but this doesn't harm performance to the human ear 
      break; 
    case 2: //G  
      strumState = 1; 
      //Serial.println("G note on");
      fretChord(activeChord); 
      delay(15);
      break; 
    case 3: //D  
      strumState = 1; 
      //Serial.println("D note on");
      fretChord(activeChord); 
      delay(15);
      break; 
    case 4: //A  
      strumState = 1; 
      //Serial.println("A note on");
      fretChord(activeChord); 
      delay(15);
      break; 
    /*case 5: //E  
      Serial.println("E note on");
      fretChord(activeChord); 
      delay(15);
      break;
    case 6: //F  
      Serial.println("F note on");
      fretChord(activeChord); 
      delay(15);
      break;*/
    case 7: //CM7  
      strumState = 1; 
      //Serial.println("CM7 note on");
      fretChord(activeChord); 
      delay(15);
      break;
    case 8: //GM7  
      strumState = 1; 
      //Serial.println("GM7 note on");
      fretChord(activeChord); 
      delay(15);
      break;
    /*case 9: //DM7  
      Serial.println("DM7 note on");
      fretChord(activeChord); 
      delay(15);
      break;
    case 10: //AM7   
      Serial.println("AM7 note on");
      fretChord(activeChord); 
      delay(15);
      break;
    case 11: //FM7  
      Serial.println("FM7 note on");
      fretChord(activeChord); 
      delay(15);
      break;
    case 12: //Dm     
      Serial.println("Dm note on");
      fretChord(activeChord); 
      delay(15);
      break;*/
    case 13: //Am  
      strumState = 1; 
      //Serial.println("Am note on");
      fretChord(activeChord); 
      delay(15);
      break;
    case 14: //Em  
      strumState = 1; 
      //Serial.println("Em note on");
      fretChord(activeChord); 
      delay(15);
      break;
    case 15: //Am7  
      strumState = 1; 
      //Serial.println("Am7 note on");
      fretChord(activeChord); 
      delay(15);
      break;
    default:
      //strumLSS.moveRelative(0); // stop strum 
      delay(1);               // need this otherwise will loop case command, trade-off hits must be >1 ms apart in Ableton 
      break;
  }
}

void fretChord(int activeChord){ // fretting system, positions calibrated before performance in LSS Config 
  int opened = 0; 
  int Estring3place = 1750; //CW 
  int Astring3place = 1750; 
  int Astring2place = 1750; 
  int Dstring2place = 1500; 
  int Gstring2place = 1750; 
  int Bstring1place = 1750;   
  int hEstring2place = 1750; 
  //int mute = 1500; // maybe change to -90 depending on what direction motor spins, will test  
  switch(activeChord){
    case 1: //c chord 
      Estring3.move(opened); 
      Astring3.move(Astring3place); 
      Astring2.move(opened); 
      Dstring2.move(Dstring2place); 
      Gstring2.move(opened); 
      Bstring1.move(hEstring2place); 
      hEstring2.move(opened); 
      //Serial.println("C Chord");
      break;
    case 2: //g chord 
      Estring3.move(Estring3place); 
      Astring3.move(opened); 
      Astring2.move(Astring2place); 
      Dstring2.move(opened); 
      Gstring2.move(opened); 
      Bstring1.move(opened); 
      hEstring2.move(opened);
      //Serial.println("G Chord");
      break;
    case 3: //d7 chord 
      Estring3.move(opened); 
      Astring3.move(opened); 
      Astring2.move(opened); 
      Dstring2.move(opened); 
      Gstring2.move(Gstring2place); 
      Bstring1.move(opened); 
      hEstring2.move(hEstring2place); 
      //Serial.println("D Chord");
      break;  
    case 4: //a chord 
      Estring3.move(opened); 
      Astring3.move(opened); 
      Astring2.move(opened); 
      Dstring2.move(Dstring2place); 
      Gstring2.move(Gstring2place); 
      Bstring1.move(opened); 
      hEstring2.move(opened); 
      //Serial.println("A Chord");
      break;  
   /* case 5: //e chord 
      Estring3.move(opened); 
      Astring3.move(secondNote); 
      AstrinGstring2.move(firstNote); 
      DstrinGstring2.move(secondNote); 
      Gstring2.move(opened); 
      Bstring1.move(opened); 
      hEstring2.move(opened); 
      Serial.println("E Chord");
      break; */
   /* case 6: //f chord 
      Estring3.move(mute); 
      Astring3.move(mute); 
      AstrinGstring2.move(secondNote); 
      DstrinGstring2.move(firstNote); 
      Gstring2.move(opened); 
      Bstring1.move(secondNote); 
      hEstring2.move(secondNote); 
      Serial.println("F Chord");
      break;  */
    case 7: //CM7 chord 
      Estring3.move(opened); 
      Astring3.move(Astring3place); 
      Astring2.move(opened); 
      Dstring2.move(Dstring2place); 
      Gstring2.move(opened); 
      Bstring1.move(opened); 
      hEstring2.move(opened); 
      //Serial.println("CM7 Chord");
      break; 
    case 8: //GM7 chord 
      Estring3.move(Estring3place); 
      Astring3.move(opened); 
      Astring2.move(Astring2place); 
      Dstring2.move(opened); 
      Gstring2.move(opened); 
      Bstring1.move(opened); 
      hEstring2.move(hEstring2place); 
      //Serial.println("GM7 Chord");
      break;  
    /*case 9: //DM7 chord 
      Estring3.move(mute); 
      Astring3.move(mute); 
      AstrinGstring2.move(opened); 
      DstrinGstring2.move(firstNote); 
      Gstring2.move(firstNote); 
      Bstring1.move(firstNote); 
      hEstring2.move(opened); 
      Serial.println("DM7 Chord");
      break;  */
    /*case 10: //AM7 chord 
      Estring3.move(opened); 
      Astring3.move(opened); 
      AstrinGstring2.move(firstNote); 
      DstrinGstring2.move(secondNote); 
      Gstring2.move(firstNote); 
      Bstring1.move(opened); 
      hEstring2.move(opened); 
      Serial.println("AM7 Chord");
      break;  */
   /* case 11: //FM7 chord 
      Estring3.move(mute); 
      Astring3.move(mute); 
      AstrinGstring2.move(secondNote); 
      DstrinGstring2.move(firstNote); 
      Gstring2.move(opened); 
      Bstring1.move(opened); 
      hEstring2.move(secondNote); 
      Serial.println("FM7 Chord");
      break; */
    /*case 12: //Dm chord 
      Estring3.move(mute); 
      Astring3.move(mute); 
      AstrinGstring2.move(opened); 
      DstrinGstring2.move(firstNote); 
      Gstring2.move(secondNote); 
      Bstring1.move(secondNote); 
      hEstring2.move(opened); 
      Serial.println("Dm Chord");
      break;   */
    case 13: //Am chord 
      Estring3.move(opened); 
      Astring3.move(opened); 
      Astring2.move(opened); 
      Dstring2.move(Dstring2place); 
      Gstring2.move(Gstring2place); 
      Bstring1.move(Bstring1place); 
      hEstring2.move(opened); 
      //Serial.println("Am Chord");
      break;  
    case 14: //Em chord 
      Estring3.move(opened); 
      Astring3.move(opened); 
      Astring2.move(Astring2place); 
      Dstring2.move(Dstring2place); 
      Gstring2.move(opened); 
      Bstring1.move(opened); 
      hEstring2.move(opened); 
      //Serial.println("Em Chord");
      break;  
    case 15: //Am7 chord 
      Estring3.move(opened); 
      Astring3.move(opened); 
      Astring2.move(opened); 
      Dstring2.move(Dstring2place); 
      Gstring2.move(opened); 
      Bstring1.move(Bstring1place); 
      hEstring2.move(opened); 
      //Serial.println("Am7 Chord");
      break;  
    default:
      delay(1);
      break; 
  }
}
