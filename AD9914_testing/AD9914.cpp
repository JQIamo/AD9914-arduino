/*
   AD9914.cpp - AD9914 DDS communication library
   Created by Ben Reschovsky, 2016
   JQI - Strontium - UMD
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "Arduino.h"
#include "SPI.h"
#include "AD9914.h"
#include <math.h>


/* CONSTRUCTOR */

// Constructor function; initializes communication pinouts
AD9914::AD9914(byte ssPin, byte resetPin, byte updatePin, byte ps0, byte ps1, byte ps2, byte powerDownPin, byte DRholdPin) //Master_reset pin?
{
  RESOLUTION  = 4294967296.0;
  _ssPin = ssPin;
  _resetPin = resetPin;
  _updatePin = updatePin;
  _ps0 = ps0;
  _ps1 = ps1;
  _ps2 = ps2;
  _powerDownPin = powerDownPin;
  _DRholdPin = DRholdPin;


  // sets up the pinmodes for output
  pinMode(_ssPin, OUTPUT);
  pinMode(_resetPin, OUTPUT);
  pinMode(_updatePin, OUTPUT);
//  pinMode(_ps0, OUTPUT); //temporarily comment out these lines since P205 jumper on eval board will have ot be enables to buffer DROVR signal
//  pinMode(_ps1, OUTPUT);
//  pinMode(_ps2, OUTPUT);
  pinMode(_powerDownPin, OUTPUT);
//  pinMode(_DRholdPin, OUTPUT);

  // defaults for pin logic levels
  digitalWrite(_ssPin, HIGH);
  digitalWrite(_resetPin, LOW);
  digitalWrite(_updatePin, LOW);
//  digitalWrite(_ps0, LOW);
//  digitalWrite(_ps1, LOW);
//  digitalWrite(_ps2, LOW);
  digitalWrite(_powerDownPin, LOW);
 // digitalWrite(_DRholdPin, LOW);
}

/* PUBLIC CLASS FUNCTIONS */


// initialize(refClk) - initializes DDS with reference clock frequency refClk (assumes you are using an external clock, not the internal PLL)
void AD9914::initialize(unsigned long refClk) {
  _refIn = refClk;
  _refClk = refClk;

  AD9914::reset();

  delay(100);

  _profileModeOn = false; //profile mode is disabled by default
  _OSKon = false; //OSK is disabled by default
  _disable = false; //power down pin low by default

  //Digital Ramp settings all false by default:
  _AutoclearDRAccumulatorOn = false;
  _AutoclearPhaseOn = false;
  _DRGoverOutputOn = false;
  _DRon = false;
  _DRnoDwellHighOn = false;
  _DRnoDwellLowOn = false;

  _activeProfile = 0;

  //initialize ramp values:
 _DRlowerLimit = 0.0;
 _DRupperLimit = 0.0;
 _DRincrementStepSize = 0.0;
 _DRdecrementStepSize = 0.0;
 _DRpositiveSlopeRate = 0.0;
 _DRnegativeSlopeRate = 0.0;
 _AutoclearDRAccumulatorOn = false;
 _AutoclearPhaseOn = false;
 _DRGoverOutputOn = false; 
 _DRon = false;
 _DRnoDwellHighOn = false;
 _DRnoDwellLowOn = false;
 

  //Disable the PLL - I think this is disabled by default, so comment it out for now
  //byte registerInfo[] = {0x02, 4};
  //byte data[] = {0x00, 0x00, 0x00, 0x00};
  //AD9914::writeRegister(registerInfo, data);


  AD9914::dacCalibrate(); //Calibrate DAC

}

// reset() - takes no arguments; resets DDS
void AD9914::reset() {
  digitalWrite(_resetPin, HIGH);
  delay(1);
  digitalWrite(_resetPin, LOW);
}

// update() - sends a logic pulse to IO UPDATE pin on DDS; updates frequency output to
//      newly set frequency (FTW0)
void AD9914::update() {
  digitalWrite(_updatePin, HIGH);
  //delay(1); //no delay needed
  digitalWrite(_updatePin, LOW);
}

// setFreq(freq) -- writes freq to DDS board, in FTW0
void AD9914::setFreq(unsigned long freq, byte profile) {

  if (profile > 7) {
    return; //invalid profile, return without doing anything
  }

  // set _freq and _ftw variables
  _freq[profile] = freq;
  _ftw[profile] = round(freq * RESOLUTION / _refClk) ;

  // divide up ftw into four bytes
  byte ftw[] = { lowByte(_ftw[profile] >> 24), lowByte(_ftw[profile] >> 16), lowByte(_ftw[profile] >> 8), lowByte(_ftw[profile])};
  // register info -- writing four bytes to register 0x04,

  byte registerInfo[] = {0x0B, 4};

  //    else if (profile == 0) {  //select the right register for the commanded profile number
  //      registerInfo[0]=0x0B;
  //    } else if (profile == 1) {
  //      registerInfo[0]=0x0D;
  //    } else if (profile == 2) {
  //      registerInfo[0]=0x0F;
  //    } else if (profile == 3) {
  //      registerInfo[0]=0x11;
  //    } else if (profile == 4) {
  //      registerInfo[0]=0x13;
  //    } else if (profile == 5) {
  //      registerInfo[0]=0x15;
  //    } else if (profile == 6) {
  //      registerInfo[0]=0x17;
  //    } else if (profile == 7) {
  //      registerInfo[0]=0x19;
  //    }
  registerInfo[0] += 2 * profile; //select the right register for the commanded profile number



  //byte CFR1[] = { 0x00, 0x00, 0x00, 0x00 };
  //byte CFR1Info[] = {0x00, 4};

  // actually writes to register
  //AD9914::writeRegister(CFR1Info, CFR1);
  AD9914::writeRegister(registerInfo, ftw);

  // issues update command
  AD9914::update();
}
void AD9914::setFreq(unsigned long freq) {
  AD9914::setFreq(freq, 0);
}

void AD9914::setAmp(double scaledAmp, byte profile) {
  if (profile > 7) {
    return; //invalid profile, return without doing anything
  }

  _scaledAmp[profile] = scaledAmp;
  _asf[profile] = round(scaledAmp * 4096);
  _scaledAmpdB[profile] = 20.0 * log10(_asf[profile] / 4096.0);

  if (_asf[profile] >= 4096) {
    _asf[profile] = 4095; //write max value
  } else if (scaledAmp < 0) {
    _asf[profile] = 0; //write min value
  }

  AD9914::writeAmp(_asf[profile], profile);

}

void AD9914::setAmp(double scaledAmp) {
  AD9914::setAmp(scaledAmp, 0);
}

void AD9914::setAmpdB(double scaledAmpdB, byte profile) {
  if (profile > 7) {
    return; //invalid profile, return without doing anything
  }

  if (scaledAmpdB > 0) {
    return; //only valid for attenuation, so dB should be less than 0, return without doing anything
  }

  _scaledAmpdB[profile] = scaledAmpdB;
  _asf[profile] = round(pow(10, scaledAmpdB / 20.0) * 4096.0);
  _scaledAmp[profile] = _asf[profile] / 4096.0;

  if (_asf[profile] >= 4096) {
    _asf[profile] = 4095; //write max value
  }

  AD9914::writeAmp(_asf[profile], profile);

}

void AD9914::setAmpdB(double scaledAmpdB) {
  AD9914::setAmpdB(scaledAmpdB, 0);
}

//Gets current amplitude
double AD9914::getAmp(byte profile) {
  return _scaledAmp[profile];
}
double AD9914::getAmp() {
  return _scaledAmp[0];
}

// Gets current amplitude in dB
double AD9914::getAmpdB(byte profile) {
  return _scaledAmpdB[profile];
}
double AD9914::getAmpdB() {
  return _scaledAmpdB[0];
}

//Gets current amplitude tuning word
unsigned long AD9914::getASF(byte profile) {
  return _asf[profile];
}
unsigned long AD9914::getASF() {
  return _asf[0];
}



// getFreq() - returns current frequency
unsigned long AD9914::getFreq(byte profile) {
  return _freq[profile];
}

// getFreq() - returns frequency from profile 0
unsigned long AD9914::getFreq() {
  return _freq[0];
}

// getFTW() -- returns current FTW
unsigned long AD9914::getFTW(byte profile) {
  return _ftw[profile];
}

unsigned long AD9914::getFTW() {
  return _ftw[0];
}

// Function setFTW -- accepts 32-bit frequency tuning word ftw;
//      updates instance variables for FTW and Frequency, and writes ftw to DDS.
void AD9914::setFTW(unsigned long ftw, byte profile) {

  if (profile > 7) {
    return; //invalid profile, return without doing anything
  }

  // set freqency and ftw variables
  _ftw[profile] = ftw;
  _freq[profile] = ftw * _refClk / RESOLUTION;

  // divide up ftw into four bytes
  byte data[] = { lowByte(ftw >> 24), lowByte(ftw >> 16), lowByte(ftw >> 8), lowByte(ftw)};
  // register info -- writing four bytes to register 0x04,

  byte registerInfo[] = {0x0B, 4};
  registerInfo[0] += 2 * profile; //select the right register for the commanded profile number


  //byte CFR1[] = { 0x00, 0x00, 0x00, 0x00 };
  //byte CFR1Info[] = {0x00, 4};

  //AD9914::writeRegister(CFR1Info, CFR1);
  AD9914::writeRegister(registerInfo, data);
  AD9914::update();

}

void AD9914::setFTW(unsigned long ftw) {
  AD9914::setFTW(ftw, 0);
}

//Enable the profile select mode
void AD9914::enableProfileMode() {
  //write 0x01, byte 23 high
  _profileModeOn = true;
//  byte registerInfo[] = {0x01, 4};
//  byte data[] = {0x00, B10000000, B00001001, 0x00}; //I think this 0x09 should be 0x00?
//  if (_DRGoverOutputOn == true) {
//    data[2] = B00101001;
//  }
//  if (_DRon == true) {
//    data[1] = data[1] | B00001000; //turn on 2nd byte, 5th bit
//  }
//  if (_DRnoDwellHighOn == true) {
//    data[1] = data[1] | B00000100; //turn on 2nd byt, 6th bit
//  }
//  if (_DRnoDwellLowOn == true) {
//    data[1] = data[1] | B00000010; //turn on 2nd byt, 7th bit
//  }
//  AD9914::writeRegister(registerInfo, data);
//  AD9914::update();
  AD9914::updateRegister2();
}

//Disable the profile select mode
void AD9914::disableProfileMode() {
  //write 0x01, byte 23 low
  _profileModeOn = false;
//  byte registerInfo[] = {0x01, 4};
//  byte data[] = {0x00, 0x00, 0x09, 0x00}; //I think this 0x09 should be 0x00
//  if (_DRGoverOutputOn == true) {
//    data[2] = B00101001;
//  }
//  if (_DRon == true) {
//    data[1] = data[1] | B00001000; //turn on 2nd byte, 5th bit
//  }
//  if (_DRnoDwellHighOn == true) {
//    data[1] = data[1] | B00000100; //turn on 2nd byt, 6th bit
//  }
//  if (_DRnoDwellLowOn == true) {
//    data[1] = data[1] | B00000010; //turn on 2nd byt, 7th bit
//  }
//  AD9914::writeRegister(registerInfo, data);
//  AD9914::update();
  AD9914::updateRegister2();
}

//enable OSK
void AD9914::enableOSK() {
  //write 0x00, byte 8 high
  _OSKon = true;

  AD9914::updateRegister1();
}

//disable OSK
void AD9914::disableOSK() {
  //write 0x00, byte 8 low
  _OSKon = false;

  AD9914::updateRegister1();
}

//return boolean indicating if profile select mode is activated
boolean AD9914::getProfileSelectMode() {
  return _profileModeOn;
}

//return boolean indicating if OSK mode is activated
boolean AD9914::getOSKMode() {
  return _OSKon;
}

//void AD9914::enableSyncClck() {
// //write 0x01, byte 11 high
//  byte registerInfo[] = {0x01, 4};
//  byte data[] = {0x00, 0x80, 0x09, 0x00};
//  AD9914::writeRegister(registerInfo, data);
//  AD9914::update();
//}
//
//void AD9914::disableSyncClck() {
//  //write 0x01, byte 11 low
//  byte registerInfo[] = {0x01, 4};
//  byte data[] = {0x00, 0x80, 0x01, 0x00};
//  AD9914::writeRegister(registerInfo, data);
//  AD9914::update();
//}

void AD9914::selectProfile(byte profile) {
  //Possible improvement: write PS pin states all at once using register masks
  _activeProfile = profile;

  if (profile > 7) {
    return; //not a valid profile number, return without doing anything
  }

  if ((B00000001 & profile) > 0) { //rightmost bit is 1
    digitalWrite(_ps0, HIGH);
  } else {
    digitalWrite(_ps0, LOW);
  }
  if ((B00000010 & profile) > 0) { //next bit is 1
    digitalWrite(_ps1, HIGH);
  } else {
    digitalWrite(_ps1, LOW);
  }
  if ((B00000100 & profile) > 0) { //next bit is 1
    digitalWrite(_ps2, HIGH);
  } else {
    digitalWrite(_ps2, LOW);
  }

}

byte AD9914::getProfile() {
  return _activeProfile;
}

//sets digital ramp lower limit
void AD9914::setDRlowerLimit(double DRlowerLimit) { //should this take a double instead of unsigned long?

  // set DRlowerLimit variable
  unsigned long LLftw = round(DRlowerLimit * RESOLUTION / _refClk) ;
  double freqStep = _refClk / RESOLUTION;
  _DRlowerLimit = LLftw * freqStep;


  // divide up ftw into four bytes
  byte ftw[] = { lowByte(LLftw >> 24), lowByte(LLftw >> 16), lowByte(LLftw >> 8), lowByte(LLftw)};
  // register info -- writing four bytes to register 0x04,

  byte registerInfo[] = {0x04, 4};


  // actually writes to register
  AD9914::writeRegister(registerInfo, ftw);

  // issues update command
  AD9914::update();

}

//get digital ramp lower limit
double AD9914::getDRlowerLimit() {
  return _DRlowerLimit;
}

//sets digital ramp upper limit
void AD9914::setDRupperLimit(double DRupperLimit) {
  // set DRupperLimit variable
  unsigned long ULftw = round(DRupperLimit * RESOLUTION / _refClk) ;
  double freqStep = _refClk / RESOLUTION;
  _DRupperLimit = ULftw * freqStep;
 
  // divide up ftw into four bytes
  byte ftw[] = { lowByte(ULftw >> 24), lowByte(ULftw >> 16), lowByte(ULftw >> 8), lowByte(ULftw)};
  // register info -- writing four bytes to register 0x05,

  byte registerInfo[] = {0x05, 4};


  // actually writes to register
  AD9914::writeRegister(registerInfo, ftw);

  // issues update command
  AD9914::update();
}

//get digital ramp upper limit
double AD9914::getDRupperLimit() {
  return _DRupperLimit;
}

//sets digital ramp increment step size
void AD9914::setDRincrementStepSize(double DRincrementSS){
   // set DRincrementStepSize variable
  unsigned long incSSftw = round(DRincrementSS * RESOLUTION / _refClk) ;
  double freqStep = _refClk / RESOLUTION;
  _DRincrementStepSize = incSSftw * freqStep;

  // divide up ftw into four bytes
  byte ftw[] = { lowByte(incSSftw >> 24), lowByte(incSSftw >> 16), lowByte(incSSftw >> 8), lowByte(incSSftw)};
  // register info -- writing four bytes to register 0x06,

  byte registerInfo[] = {0x06, 4};

  // actually writes to register
  AD9914::writeRegister(registerInfo, ftw);

  // issues update command
  AD9914::update();
}

//get digital ramp increment step size
double AD9914::getDRincrementStepSize(){
  return _DRincrementStepSize;
}

//sets digital ramp decrement step size
void AD9914::setDRdecrementStepSize(double DRdecrementSS){
   // set DRdecrementStepSize variable
  unsigned long decSSftw = round(DRdecrementSS * RESOLUTION / _refClk) ;
  double freqStep = _refClk / RESOLUTION;
  _DRdecrementStepSize = decSSftw * freqStep;

  // divide up ftw into four bytes
  byte ftw[] = { lowByte(decSSftw >> 24), lowByte(decSSftw >> 16), lowByte(decSSftw >> 8), lowByte(decSSftw)};
  // register info -- writing four bytes to register 0x07,

  byte registerInfo[] = {0x07, 4};


  // actually writes to register
  AD9914::writeRegister(registerInfo, ftw);

  // issues update command
  AD9914::update();
}

//get digital ramp decrement step size
double AD9914::getDRdecrementStepSize(){
  return _DRdecrementStepSize;
}

//sets digital ramp rate (rising and falling)
void AD9914::setDRrampRate(double positiveRR, double negativeRR){ //units are seconds
  unsigned short risingRR = round(positiveRR * _refClk/24.0);
  if (risingRR <= 0){ 
    risingRR = 1; //set to minimum value
  }

  unsigned short fallingRR = round(negativeRR * _refClk/24.0);
  if (fallingRR <=0){
    fallingRR = 1; //set to minimum value
  }

  //set global variables
  _DRpositiveSlopeRate = 24.0* risingRR / _refClk;
  _DRnegativeSlopeRate = 24.0* fallingRR / _refClk;

  // divide up ftw into four bytes
  byte ftw[] = { highByte(risingRR), lowByte(risingRR), highByte(fallingRR), lowByte(fallingRR)};
  // register info -- writing four bytes to register 0x08,

  byte registerInfo[] = {0x08, 4};


  // actually writes to register
  AD9914::writeRegister(registerInfo, ftw);

  // issues update command
  AD9914::update();
  
}

//get digital ramp rising rate
double AD9914::getDRpositiveSlopeRate(){
  return _DRpositiveSlopeRate;
}

//get digital ramp falling rate
double AD9914::getDRnegativeSlopeRate(){
  return _DRnegativeSlopeRate;
}

//configure digital ramp
void AD9914::configureRamp(boolean AutoClearAccumulator, boolean AutoclearPhase, boolean DRGoverOutput, boolean noDwellHigh, boolean noDwellLow){
  //set variables
  _AutoclearDRAccumulatorOn = AutoClearAccumulator;
  _AutoclearPhaseOn = AutoclearPhase;
  _DRGoverOutputOn = DRGoverOutput;
  _DRnoDwellHighOn = noDwellHigh;
  _DRnoDwellLowOn = noDwellLow;

  AD9914::updateRegister1();
  AD9914::updateRegister2();
  
  
  AD9914::update();
}

//enable digital ramp
void AD9914::enableDR(){
  //set variable
  _DRon = true;
  
  AD9914::updateRegister2();
}

//disable digital ramp
void AD9914::disableDR(){
  //set variable
  _DRon = false;
  
  AD9914::updateRegister2();
}

//get digital ramp mode status
boolean AD9914::getDRmode(){
  return _DRon;
}

//get digital ramp settings:
boolean AD9914::getAutoclearAccumulatorOn(){
  return _AutoclearDRAccumulatorOn;
}

boolean AD9914::getAutoclearPhaseOn(){
  return _AutoclearPhaseOn;
}

boolean AD9914::getDRGoverOutputOn(){
  return _DRGoverOutputOn;
}

boolean AD9914::getDRnoDwellHighOn(){
  return _DRnoDwellHighOn;
}

boolean AD9914::getDRnoDwellLowOn(){
  return _DRnoDwellLowOn;
}


/* PRIVATE CLASS FUNCTIONS */


// Writes SPI to particular register.
//      registerInfo is a 2-element array which contains [register, number of bytes]
void AD9914::writeRegister(byte registerInfo[], byte data[]) {

  digitalWrite(_ssPin, LOW);

  // Writes the register value
  SPI.transfer(registerInfo[0]);

  // Writes the data
  for (int i = 0; i < registerInfo[1]; i++) {
    SPI.transfer(data[i]);
  }

  digitalWrite(_ssPin, HIGH);

}

void AD9914::dacCalibrate() {
  //Calibrate DAC (0x03, bit 24 -> high then low)
  byte registerInfo[] = {0x03, 4};
  byte data[] = {0x01, 0x05, 0x21, 0x20}; //write bit high
  AD9914::writeRegister(registerInfo, data);
  AD9914::update();
 // delay(1);
  data[0] = 0x00; //write bit low
  AD9914::writeRegister(registerInfo, data);
  AD9914::update();
}

void AD9914::writeAmp(long ampScaleFactor, byte profile) {
  byte registerInfo[] = {0x0C, 4};

  registerInfo[0] += 2 * profile; //select the right register for the commanded profile number

  // divide up ASF into two bytes, pad with 0s for the phase offset
  byte atw[] = {lowByte(ampScaleFactor >> 8), lowByte(ampScaleFactor), 0x00, 0x00};

  // actually writes to register
  AD9914::writeRegister(registerInfo, atw);

  AD9914::update();

}

void AD9914::updateRegister1() {

  byte registerInfo[] = {0x00, 4};
  byte data[] = {0x00, 0x01, 0x00, 0x08}; //default values, 
  //if we want to use cosine (instead of sine) output, 2nd byte should be 0x00
  //if we just want to power down DAC, then we need to toggle between 0x00 and 0x40 in last byte
  
  

  if (_AutoclearDRAccumulatorOn == true) {
    data[2] = data[2] | B01000000; //turn on 3rd byte, 2nd bit
  }
  if (_AutoclearPhaseOn == true) {
    data[2] = data[2] | B00100000;
  }

  if (_OSKon == true) {
    data[2] = data[2] | B00000001; //turn on 3rd byte, last bit
  }
 
  AD9914::writeRegister(registerInfo, data);
  AD9914::update();
}


void AD9914::updateRegister2() {

  byte registerInfo[] = {0x01, 4};
  byte data[] = {0x00, 0x00, 0x09, 0x00}; //default values
  if (_DRGoverOutputOn == true) {
    data[2] = B00101001;
  }
  if (_DRon == true) {
    data[1] = data[1] | B00001000; //turn on 2nd byte, 5th bit
  }
  if (_DRnoDwellHighOn == true) {
    data[1] = data[1] | B00000100; //turn on 2nd byte, 6th bit
  }
  if (_DRnoDwellLowOn == true) {
    data[1] = data[1] | B00000010; //turn on 2nd byte, 7th bit
  }
  if (_profileModeOn == true) {
    data[1] = data[1] | B10000000; //turn on 2nd byte, 1st bit
  }

  AD9914::writeRegister(registerInfo, data);
  AD9914::update();
}

void AD9914::enableDDS(){
  _disable == false;
  digitalWrite(_powerDownPin, LOW);
  AD9914::dacCalibrate();
}

void AD9914::disableDDS(){
  _disable == true;
  digitalWrite(_powerDownPin, HIGH); 
}

unsigned long AD9914::getClock(){
  return _refClk;
}
 
