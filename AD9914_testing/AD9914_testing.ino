#include <SPI.h>
#include "AD9914.h"
#include <ADF4350.h>
#include <SerialCommand.h>


//Define pin mappings: 

#define CSPIN 1                 // DDS chip select pin. Digital input (active low). Bringing this pin low enables detection of serial clock edges.
#define OSKPIN 17              //not actually connected               // DDS Output Shift Keying. Digital input.
#define PS0PIN 7               // DDS PROFILE[0] pin. Profile Select Pins. Digital input. Use these pins to select one of eight profiles for the DDS.
#define PS1PIN 6               // DDS PROFILE[1] pin. Profile Select Pins. Digital input. Use these pins to select one of eight profiles for the DDS.
#define PS2PIN 5               // DDS PROFILE[2] pin. Profile Select Pins. Digital input. Use these pins to select one of eight profiles for the DDS.
#define IO_UPDATEPIN  23        // DDS I/O_UPDATE pin. Digital input. A high on this pin transfers the contents of the buffers to the internal registers.
#define RESETPIN 10      // DDS MASTER_RESET pin. Digital input. Clears all memory elements and sets registers to default values.

#define SYNCIO_PIN 22
#define DROVERPIN 2        // DDS writes this line high when the end of a ramp is reached
#define DRHOLDPIN 9        // Pauses sweep when high
#define DRCTLPIN 8         // Controls sweep direction (up/down on rising/falling edge)

#define OSKPIN  17          //Output shift keying (amplitude control), not sure of functionality
#define EXTPDCTLPIN 18      //High Level triggers power-down mode 

#define PLL_CS_PIN 14



//Declare the DDS object:
AD9914 DDS(CSPIN, RESETPIN, IO_UPDATEPIN, PS0PIN, PS1PIN, PS2PIN, EXTPDCTLPIN, DRHOLDPIN);

//Declare the PLL object:
ADF4350 PLL(PLL_CS_PIN);

SerialCommand sCmd;

int maxFreqs;
unsigned long freq[8];
unsigned long delayMicros;

void serialTest(){
  char *arg;
  arg = sCmd.next();
  double freqStep = 3.5e9/4294967296.0;
  

  //int number = atoi(arg);  //convert char string to an integer
  double number = atof(arg); //convert char string to a double
  char lineToDisplay[50];
  snprintf(lineToDisplay,50, "%9.15f MHz", 1227133513*freqStep);
  Serial.println(lineToDisplay);
}

void setDRupperLim() {     //units: MHz
  char *arg;
  arg = sCmd.next();

  double upperFreq = atof(arg);
  DDS.setDRupperLimit(upperFreq*1e6);
  double actualFreq = DDS.getDRupperLimit()/1e6;

  char lineToDisplay[50];
  snprintf(lineToDisplay,50, "DR Upper Limit: %9.9f MHz", actualFreq);
  Serial.println(lineToDisplay); 
}

void setDRlowerLim() {   //units: MHz
  char *arg;
  arg = sCmd.next();

  double lowerFreq = atof(arg);
  DDS.setDRlowerLimit(lowerFreq*1e6);
  double actualFreq = DDS.getDRlowerLimit()/1e6;

  char lineToDisplay[50];
  snprintf(lineToDisplay,50, "DR Lower Limit: %9.9f MHz", actualFreq);
  Serial.println(lineToDisplay); 
}

void setDRrisingStepSize() { //units: MHz
  char *arg;
  arg = sCmd.next();

  double incrementSS = atof(arg);
  DDS.setDRincrementStepSize(incrementSS*1e6);
  double actualSS = DDS.getDRincrementStepSize()/1e6;

  char lineToDisplay[50];
  snprintf(lineToDisplay,50, "DR Rising Step Size: %9.9f MHz", actualSS);
  Serial.println(lineToDisplay); 
}

void setDRfallingStepSize() { //units: MHz
  char *arg;
  arg = sCmd.next();

  double decrementSS = atof(arg);
  DDS.setDRdecrementStepSize(decrementSS*1e6);
  double actualSS = DDS.getDRdecrementStepSize()/1e6;

  char lineToDisplay[50];
  snprintf(lineToDisplay,50, "DR Falling Step Size: %9.9f MHz", actualSS);
  Serial.println(lineToDisplay); 
}

void setDRrisingRate(){   //units: us
  char *arg;
  arg = sCmd.next();
  
  double risingRR = atof(arg);
  double fallingRR = DDS.getDRnegativeSlopeRate();
  DDS.setDRrampRate(risingRR/1e6, fallingRR);
  
  double actualRR = DDS.getDRpositiveSlopeRate()*1e6; //us

  char lineToDisplay[50];
  snprintf(lineToDisplay,50, "DR Rising Ramp Rate: %9.9f us", actualRR);
  Serial.println(lineToDisplay); 
}

void setDRfallingRate(){   //units: us
  char *arg;
  arg = sCmd.next();
  
  double fallingRR = atof(arg);
  double risingRR = DDS.getDRpositiveSlopeRate();
  DDS.setDRrampRate(risingRR, fallingRR/1e6);
  
  double actualRR = DDS.getDRnegativeSlopeRate()*1e6; //us

  char lineToDisplay[50];
  snprintf(lineToDisplay,50, "DR Falling Ramp Rate: %9.9f us", actualRR);
  Serial.println(lineToDisplay); 
}

void powerDown(){
  DDS.disableDDS();
  Serial.println("DDS Off");
}

void powerUp(){
  DDS.enableDDS();
  Serial.println("DDS On");
}

void setAmplitude(){
  char *arg;
  arg = sCmd.next();
  double amp = atof(arg);
  
  DDS.enableOSK();

  for (int i = 0 ; i <= 7 ; i++) { //for now, set the amplitude of all profiles since the state of PS0 - PS1 is not being controlled
    DDS.setAmp(amp, i);
  }

 // DDS.setAmp(amp);
  char lineToDisplay[50];
  snprintf(lineToDisplay,50, "Amplitude: %f", amp);
  Serial.println(lineToDisplay);
  
}

void configureDR(){
  char *arg;

  arg = sCmd.next();
  boolean rampOn = (atoi(arg) == 1);

  arg = sCmd.next();
  boolean autoClearAccum = (atoi(arg) == 1);

  arg = sCmd.next();
  boolean autoClearPhase = (atoi(arg) == 1);

  arg = sCmd.next();
  boolean DRover = (atoi(arg) == 1);

  arg = sCmd.next();
  boolean noDwellHigh = (atoi(arg) == 1);

  arg = sCmd.next();
  boolean noDwellLow = (atoi(arg) == 1);

  
  Serial.print("Digital Ramp Enable: ");
  Serial.println(rampOn);
  Serial.print("Autoclear Accumulator: ");
  Serial.println(autoClearAccum);
  Serial.print("Autoclear Phase: ");
  Serial.println(autoClearPhase);
  Serial.print("DR Over Output: ");
  Serial.println(DRover);
  Serial.print("No Dwell High: ");
  Serial.println(noDwellHigh);
  Serial.print("No Dwell Low: ");
  Serial.println(noDwellLow);
 

  
  DDS.configureRamp(autoClearAccum, autoClearPhase, DRover, noDwellHigh, noDwellLow);
  if (rampOn){
    DDS.enableDR();
  }
  else{
    DDS.disableDR();
  }
}

void calibrateDAC(){
  DDS.dacCalibrate();
  Serial.println("DAC Calibrated");
}

void setClk(){  //units: MHz
  char *arg;

  arg = sCmd.next();
  double clockF = atof(arg);
  unsigned long clockFreq = round(clockF);

  PLL.initialize(clockFreq,10); //does PLL take double or integer?

  delay(100);

  DDS.initialize(clockFreq*1000000);

  char lineToDisplay[50];
  snprintf(lineToDisplay,50, "DDS Reset, Clock Frequency: %f MHz", clockF);
  Serial.println(lineToDisplay);


  
}

void printDRSettings(){
  Serial.println("-----------------------------------------------");
  Serial.print("Digital Ramp Enabled: ");
  Serial.println(DDS.getDRmode());

  Serial.print("Autoclear Accumulator: ");
  Serial.println(DDS.getAutoclearPhaseOn());
  Serial.print("Autoclear Phase: ");
  Serial.println(DDS.getAutoclearPhaseOn());
  Serial.print("DR Over Output: ");
  Serial.println(DDS.getDRGoverOutputOn());
  Serial.print("No Dwell High: ");
  Serial.println(DDS.getDRnoDwellHighOn());
  Serial.print("No Dwell Low: ");
  Serial.println(DDS.getDRnoDwellLowOn());

  char lineToDisplay[50];
  snprintf(lineToDisplay,50, "(UL) DR Upper Limit: %9.9f MHz", DDS.getDRupperLimit()/1e6);
  Serial.println(lineToDisplay); 
  snprintf(lineToDisplay,50, "(LL) DR Lower Limit: %9.9f MHz", DDS.getDRlowerLimit()/1e6);
  Serial.println(lineToDisplay); 
  snprintf(lineToDisplay,50, "(RSS) DR Rising Step Size: %9.9f MHz", DDS.getDRincrementStepSize()/1e6);
  Serial.println(lineToDisplay); 
  snprintf(lineToDisplay,50, "(FSS) DR Falling Step Size: %9.9f MHz", DDS.getDRdecrementStepSize()/1e6);
  Serial.println(lineToDisplay); 
  snprintf(lineToDisplay,50, "(RRR) DR Rising Ramp Rate: %9.9f us", DDS.getDRpositiveSlopeRate()*1e6);
  Serial.println(lineToDisplay); 
  snprintf(lineToDisplay,50, "(FRR) DR Falling Ramp Rate: %9.9f us", DDS.getDRnegativeSlopeRate()*1e6);
  Serial.println(lineToDisplay); 
  snprintf(lineToDisplay,50, "DDS Clock Frequency: %9.9f MHz", DDS.getClock()/1.0e6);
  Serial.println(lineToDisplay); 
  Serial.println("-----------------------------------------------");
}

void toggleRamp(){
  if (DDS.getDRmode() == true){
    DDS.disableDR();
    Serial.println("Ramp Off");
  } else { 
    DDS.enableDR();
    Serial.println("Ramp On");
    
  }
}

void unrecognizedCmd(const char *command) {
  Serial.println("UNRECOGNIZED COMMAND");
}

void setup() {
  delay(100);
  SPI.begin();
  SPI.setClockDivider(4);
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  
  Serial.begin(115200);
  sCmd.addCommand("t", serialTest);
  sCmd.addCommand("UL", setDRupperLim);
  sCmd.addCommand("LL", setDRlowerLim);
  sCmd.addCommand("RSS", setDRrisingStepSize);
  sCmd.addCommand("FSS", setDRfallingStepSize);
  sCmd.addCommand("RRR", setDRrisingRate);
  sCmd.addCommand("FRR", setDRfallingRate);
  
  sCmd.addCommand("off", powerDown);
  sCmd.addCommand("on", powerUp);
  sCmd.addCommand("amp", setAmplitude);
  sCmd.addCommand("DR", configureDR);
  sCmd.addCommand("cal", calibrateDAC);
  sCmd.addCommand("DR?", printDRSettings);
  sCmd.addCommand("clock", setClk);
  sCmd.addCommand("R", toggleRamp);
  
  
  sCmd.setDefaultHandler(unrecognizedCmd);
  

  unsigned long  clockFreq = 3600000000; //PLL only tunable to the nearest 10 MHz 3.75 best (3.66 also good and a bunch of points spaced by 30 MHz)
  //unsigned long  clockFreq = 3840000000;
  
  PLL.initialize(clockFreq/1000000,10); //max PLL setting about 4.25 GHz
   
  PLL.setRfPower(3); //nominally 3 dBm, actually about -0.7 dBm, set to 3 for a little more power
  //PLL.auxEnable(true); //turn on auxiliary output
  //PLL.setAuxPower(3); 
  delay(10);
  
  DDS.initialize(clockFreq);
  //DDS.enableProfileMode();
  //DDS.enableOSK();
  delay(100);

  boolean AutoClearAccumulator = true;
  boolean AutoClearPhase = true;
  boolean DRGoverOutput = true;
  boolean noDwellHigh = false;
  boolean noDwellLow = false;
  double lowerFreq = 10e6;
  double upperFreq = 1200e6;

  
  //settings for larger combs:
  double RR = 24.0/clockFreq; //set to minimum value of 6.857 us, change this if clock changes
  double decrementSS = upperFreq-lowerFreq;
  double fRamp = 10e6; //ramp rep. rate or comb tooth spacing
  double incrementSS = fRamp*(upperFreq - lowerFreq - fRamp )*RR; //subtract fRamp from total span to make sure we never quite get to the end of the ramp, which seems to cause problems
  double FRR = RR;

  

  //settings for spectroscopy: (use with 3.692 GHz clock, (SRS) or 3.75 GHz clock (PLL) )
  //lowerFreq = 461.71e6;
  //upperFreq = 461.75e6;
  //RR = 24.0/clockFreq; //set to minimum value of 6.857 us, change this if clock changes
  //decrementSS = upperFreq-lowerFreq;
  //incrementSS = 0.000001e6;
  //FRR = 0.22e-6;
  
  
  DDS.configureRamp(AutoClearAccumulator, AutoClearPhase, DRGoverOutput, noDwellHigh, noDwellLow);
  DDS.setDRlowerLimit(lowerFreq);
  DDS.setDRupperLimit(upperFreq);
  DDS.setDRincrementStepSize(incrementSS);
  DDS.setDRdecrementStepSize(decrementSS);
  DDS.setDRrampRate(RR, FRR); //set to minimum value
  DDS.enableDR();

  double amp = 0.4 ; //0.22 for potassium spectroscopy, 0.4 for cavity stuff
  DDS.enableOSK();
  for (int i = 0 ; i <= 7 ; i++) { //for now, set the amplitude of all profiles since the state of PS0 - PS1 is not being controlled
    DDS.setAmp(amp, i);
  }
  
 // DDS.enableOSK();
 //DDS.setAmp(.55);


}

void loop() {
  // put your main code here, to run repeatedly:
  sCmd.readSerial();
  
  
// for (int i = 0 ; i < maxFreqs ; i++) {
//   DDS.selectProfile(i);
//   //delay(1);
//   delayMicroseconds(delayMicros);
// } 
 
 //DDS.selectProfile(0);
 //DDS.setAmpdB(-3,0);
  //Serial.println("test");


 delay(100);
// DDS.dacCalibrate(); //this doesn't appear to be helpful

}
