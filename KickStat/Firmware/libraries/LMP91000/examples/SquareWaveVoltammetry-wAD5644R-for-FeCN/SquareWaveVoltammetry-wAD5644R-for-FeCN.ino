/*
 * FILENAME:  SquareWaveVoltammetry-wDueDAC.ino
 * AUTHOR:    Orlando Hoilett
 * VERSION:   V.1.0.0
 * 
 * 
 * DESCRIPTION
 * 
 * 
 * 
 * UPDATES
 * 
 * 
 * 
 * DISCLAIMER
 * Linnes Lab code, firmware, and software is released under the
 * MIT License (http://opensource.org/licenses/MIT).
 * 
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Linnes Lab, Purdue University
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 * 
 * 
 */


#include <Wire.h>
#include <SPI.h>
#include <LMP91000.h>
#include <AD56X4.h>
#include <MemoryFree.h>;
#include "pgmStrToRAM.h"; //SerialUSB.println(freeMemory(), DEC);  // print how much RAM is available.


LMP91000 pStat = LMP91000();

const int AD56X4_SS_pin = 10;

const uint16_t opVolt = 3280;
const uint8_t adcBits = 10;
const double v_tolerance = 0.009;


//const uint16_t dacMin = 0; //0V
//const uint16_t dacMax = opVolt; //Vdd
//const uint16_t dacSpan = opVolt; //Vdd
const uint16_t dacResolution = pow(2,16)-1; //16-bit


bool debug = false;


//analog input pins to read voltages
const uint8_t LMP = A2;
const uint8_t C1 = A0;
const uint8_t C2 = A1;
const uint8_t DACRead = A3;
const uint8_t diffAmp = A6;
const uint8_t INA = A7;


void setup()
{
  Wire.begin();
  Serial.begin(115200);


  pinMode(AD56X4_SS_pin,OUTPUT);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  SPI.begin();  

  AD56X4.reset(AD56X4_SS_pin,true);
  

  //enable the potentiostat
  delay(50);
  pStat.standby();
  delay(50);
  initLMP(5);
  delay(2000); //warm-up time for the gas sensor
  
}


void loop()
{
  Serial.println(F("Ready!"));
  
  //will hold the code here until a character is sent over the serial port
  //this ensures the experiment will only run when initiated
  while(!Serial.available());
  Serial.read();


  //prints column headings
//  if(debug) Serial.println("Time(ms),VF,Sign,Bias Index,Bias,dacVOut,DACVal,DAC Read,LMP,C1,C2,DiffAmp,INA,Time(ms),VF,Sign,Bias Index,Bias,dacVOut,DACVal,DAC Read,LMP,C1,C2,DiffAmp,INA");
//  else Serial.println("Time(ms),Volt,LMP,C1,C2,LMP,C1,C2");

//  //prints column headings
//  if(debug) Serial.println("Time(ms),VF,Sign,Bias Index,Bias,dacVOut,DACVal,DAC Read,LMP,C1,C2,DiffAmp,INA,Time(ms),VF,Sign,Bias Index,Bias,dacVOut,DACVal,DAC Read,LMP,C1,C2,DiffAmp,INA");
//  else Serial.println("Time(ms),Volt,diffAmp,INA,Time(ms),Volt,diffAmp,INA");


//  //prints column headings
//  if(debug) Serial.println("Time(ms),VF,Sign,Bias Index,Bias,dacVOut,DACVal,DAC Read,LMP,C1,C2,DiffAmp,INA,Time(ms),VF,Sign,Bias Index,Bias,dacVOut,DACVal,DAC Read,LMP,C1,C2,DiffAmp,INA");
//  else Serial.println("Time(ms),Volt,LMP,C1,C2,Time(ms),Volt,LMP,C1,C2");


  //Serial.println("Time(ms),Volt,C1,C2,INA,Time(ms),Volt,C1,C2,INA");
  Serial.println("Time(ms),Volt,C1,C2,Diff,INA,Time(ms),Volt,C1,C2,Diff,INA");


//  //prints column headings
//  if(debug) Serial.println("Time(ms),VF,Sign,Bias Index,Bias,dacVOut,DACVal,DAC Read,LMP,C1,C2,DiffAmp,INA,Time(ms),VF,Sign,Bias Index,Bias,dacVOut,DACVal,DAC Read,LMP,C1,C2,DiffAmp,INA");
//  else Serial.println("Time(ms),Volt,LMP,C1,C2,Diff,INA,Time(ms),Volt,LMP,C1,C2,Diff,INA");

  
  //lmpGain, startV(mV), endV(mV), pulseAmp(mV), stepV(mV), freq(Hz)
  runSWV(2, 500, -500, 50, 2, 120);
//  Serial.println("Backward scan");
//  runSWV(2, -500, 50, 50, 5, 0.8);
  setOutputsToZero();
}


void initLMP(uint8_t lmpGain)
{
  pStat.disableFET();
  pStat.setGain(lmpGain);
  pStat.setRLoad(0);
  //pStat.setRLoad(3);
  pStat.setExtRefSource();
  pStat.setIntZ(1);
  pStat.setThreeLead();
  pStat.setBias(0);
  pStat.setPosBias();

  setOutputsToZero();
}


void setOutputsToZero()
{
  AD56X4.setChannel(AD56X4_SS_pin, AD56X4_SETMODE_INPUT, AD56X4_CHANNEL_B, 0);
  AD56X4.updateChannel(AD56X4_SS_pin, AD56X4_CHANNEL_B);
  pStat.setBias(0);
}


void runSWV(uint8_t lmpGain, int16_t startV, int16_t endV,
            int16_t pulseAmp, int16_t stepV, double freq)
{
  initLMP(lmpGain);
  stepV = abs(stepV);
  pulseAmp = abs(pulseAmp);
  freq = (uint16_t)(1000.0 / (2*freq));


  if(startV < endV) runSWVForward(startV, endV, pulseAmp, stepV, freq);
  else runSWVBackward(startV, endV, pulseAmp, stepV, freq);

  //setOutputsToZero();
}


void runSWVForward(int16_t startV, int16_t endV, int16_t pulseAmp,
                   int16_t stepV, double freq)
{
  for (int16_t j = startV; j <= endV; j += stepV)
  {
    //positive pulse
    biasAndSample(j + pulseAmp,freq);
    Serial.print(F(","));

    //negative pulse
    biasAndSample(j - pulseAmp,freq);
    Serial.println();
  }
}



void runSWVBackward(int16_t startV, int16_t endV, int16_t pulseAmp,
                   int16_t stepV, double freq)
{
  for (int16_t j = startV; j >= endV; j -= stepV)
  {
    //positive pulse
    biasAndSample(j + pulseAmp,freq);
    Serial.print(F(","));
    
    //negative pulse
    biasAndSample(j - pulseAmp,freq);
    Serial.println();
  }
}


void biasAndSample(int16_t voltage, double rate)
{
  Serial.print(millis());
  Serial.print(F(","));
  Serial.print(voltage);
  Serial.print(F(","));
  
  setLMPBias(voltage);
  setVoltage(voltage);
  
  delay(rate);
  sampleOutputs();
  testingDiffAndINAAmps();
  //delay(rate);
}


void sampleOutputs()
{
//  Serial.print(analogRead(DACRead));
//  Serial.print(F(","));
//  Serial.print(analogRead(LMP));
//  Serial.print(F(","));
  Serial.print(analogRead(C1));
  Serial.print(F(","));
  Serial.print(analogRead(C2));


//  Serial.print(pStat.getVoltage(analogRead(DACRead),opVolt,adcBits),1);
//  Serial.print(F(","));
//  Serial.print(pStat.getCurrent(analogRead(LMP),opVolt,adcBits),6);
//  Serial.print(F(","));
//  Serial.print(pStat.getCurrent(analogRead(C1),opVolt,adcBits),6);
//  Serial.print(F(","));
//  Serial.print(pStat.getCurrent(analogRead(C2),opVolt,adcBits),6);
}


void testingDiffAndINAAmps()
{
  Serial.print(F(","));
  Serial.print(analogRead(diffAmp));
  Serial.print(F(","));
  Serial.print(analogRead(INA));


//  Serial.print(F(","));
//  Serial.print(pStat.getCurrent(analogRead(diffAmp),opVolt,adcBits),6);
//  Serial.print(F(","));
//  Serial.print(pStat.getCurrent(analogRead(INA),opVolt,adcBits),6);
}



void setVoltage(int16_t voltage)
{
  uint16_t dacVout = 1500;
  uint8_t bias_setting = 0;

  if(abs(voltage) < 15) voltage = 15*(voltage/abs(voltage));    
  
  int16_t setV = dacVout*TIA_BIAS[bias_setting];
  voltage = abs(voltage);
  
  
  while(setV > voltage*(1+v_tolerance) || setV < voltage*(1-v_tolerance))
  {
    if(bias_setting == 0) bias_setting = 1;
    
    dacVout = voltage/TIA_BIAS[bias_setting];
    
    if (dacVout > opVolt)
    {
      bias_setting++;
      dacVout = 1500;

      if(bias_setting > NUM_TIA_BIAS) bias_setting = 0;
    }
    
    setV = dacVout*TIA_BIAS[bias_setting];    
  }


  pStat.setBias(bias_setting);
  AD56X4.setChannel(AD56X4_SS_pin, AD56X4_SETMODE_INPUT, AD56X4_CHANNEL_B, convertDACVoutToDACVal(dacVout));
  AD56X4.updateChannel(AD56X4_SS_pin, AD56X4_CHANNEL_B);
  

  if(debug)
  {
    Serial.print(bias_setting);
    Serial.print(F(","));
    Serial.print(TIA_BIAS[bias_setting]);
    Serial.print(F(","));
    Serial.print(dacVout);
    Serial.print(F(","));
    //Serial.print(convertDACVoutToDACVal(dacVout));
    Serial.print(convertDACVoutToDACVal(dacVout) >> 2);
    Serial.print(F(","));
  }
}


//Convert the desired voltage
uint16_t convertDACVoutToDACVal(uint16_t dacVout)
{
  //return (dacVout-dacMin)*((double)dacResolution/dacSpan);
  return dacVout*((double)dacResolution/opVolt);
}


void setLMPBias(int16_t voltage)
{
  signed char sign = (double)voltage/abs(voltage);
  
  if(sign < 0) pStat.setNegBias();
  else if (sign > 0) pStat.setPosBias();
  else {} //do nothing

  if(debug)
  {
    Serial.print(sign);
    Serial.print(F(","));
  }
}
