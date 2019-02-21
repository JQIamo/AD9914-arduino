#include <SPI.h>
#include "AD9914.h"
#include <ADF4350.h>

#define MAX_PARAM_NUM 11

//#include "SetListArduino.h"

//Define pin mappings: 

#define CSPIN 1                 // DDS chip select pin. Digital input (active low). Bringing this pin low enables detection of serial clock edges.
#define OSKPIN 17              //not actually connected               // DDS Output Shift Keying. Digital input.
#define PS0PIN 7               // DDS PROFILE[0] pin. Profile Select Pins. Digital input. Use these pins to select one of eight profiles for the DDS.
#define PS1PIN 6               // DDS PROFILE[1] pin. Profile Select Pins. Digital input. Use these pins to select one of eight profiles for the DDS.
#define PS2PIN 5               // DDS PROFILE[2] pin. Profile Select Pins. Digital input. Use these pins to select one of eight profiles for the DDS.
#define IO_UPDATEPIN  23        // DDS I/O_UPDATE pin. Digital input. A high on this pin transfers the contents of the buffers to the internal registers.
#define RESETPIN 10      // DDS MASTER_RESET pin. Digital input. Clears all memory elements and sets registers to default values.

#define SYNCIO_PIN 22
#define DROVER 2
#define DRHOLD 9
#define DRCTL 8

#define PLL_CS_PIN 14

#define SETLIST_TRIGGER 33 //we don't actually want to trigger this device, so specify an unused pin




//Declare the DDS object:
AD9914 DDS(CSPIN, RESETPIN, IO_UPDATEPIN, PS0PIN, PS1PIN, PS2PIN, OSKPIN);

//Declare the PLL object:
ADF4350 PLL(PLL_CS_PIN);

//Declare the setlist arduino object:
//SetListArduino SetListImage(SETLIST_TRIGGER);

double amplitudeCorrdB(unsigned long freq){ //freq is in Hz
  if (freq > 1500000000) {
    return 0.0;
  } else {
    //return (-20.25 + 0.013*(freq/1000000.0)); //linear version
    //return (5.613e-12 * pow(freq/1000000.0,4) - 1.341e-8 * pow(freq/1000000.0,3) + 6.154e-6 * pow(freq/1000000.0,2) + 0.016*freq/1000000.0 - 21.035); //4th order polynomial version, smoothes output after AOM driver board
    return (-3.288e-12 * pow(freq/1000000.0,4) + 1.262e-8 * pow(freq/1000000.0,3) - 1.51e-5 * pow(freq/1000000.0,2) + 7.847e-3*freq/1000000.0 - 4.717); //4th order polynomial version, smoothes output after directional coupler and amplifier to about +28 dBm

  }
}

int maxFreqs;
unsigned long freq[8];
unsigned long delayMicros;


void setup() {
  delay(100);
  SPI.begin();
  SPI.setClockDivider(4);
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  
  Serial.begin(115200);

  unsigned long  clockFreq = 3500000000;
  
  PLL.initialize(clockFreq/1000000,10); //max PLL setting may be 3 GHz...
  
  delay(10);
  
  DDS.initialize(clockFreq);
  //DDS.enableProfileMode();
  //DDS.enableOSK();
  delay(100);

  boolean AutoClearAccumulator = false;
  boolean DRGoverOutput = false;
  boolean noDwellHigh = true;
  boolean noDwellLow = true;
  double lowerFreq = 1e6;
  double upperFreq = 10e6;

  double RR = 24.0/clockFreq; //set to minimum value of 6.857 us, change this if clock changes
  double decrementSS = upperFreq-lowerFreq;
  double fRamp = 100e3; //ramp rep. rate or comb tooth spacing
  double incrementSS = fRamp*(upperFreq - lowerFreq)*RR;
  

  
  DDS.configureRamp(AutoClearAccumulator, DRGoverOutput, noDwellHigh, noDwellLow);
  DDS.setDRlowerLimit(lowerFreq);
  DDS.setDRupperLimit(upperFreq);
  DDS.setDRincrementStepSize(incrementSS);
  DDS.setDRdecrementStepSize(decrementSS);
  DDS.setDRrampRate(RR, RR); //set to minimum value
  DDS.enableDR();


}

void loop() {
  // put your main code here, to run repeatedly:
//  SetListImage.readSerial();
  
// for (int i = 0 ; i < maxFreqs ; i++) {
//   DDS.selectProfile(i);
//   //delay(1);
//   delayMicroseconds(delayMicros);
// } 
 
 //DDS.selectProfile(0);
 //DDS.setAmpdB(-3,0);
 //Serial.println(amplitudeCorrdB(25000000));


 //delay(5000);

}
