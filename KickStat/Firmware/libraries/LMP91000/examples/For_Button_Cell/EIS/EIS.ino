//Standard Arduino Libraries
#include <Wire.h>

//Custom External Libraries
#include "MemoryFree.h"
#include "pgmStrToRAM.h" //SerialDebugger.println(freeMemory(), DEC);  // print how much RAM is available.

//Custom Internal Libraries
#include "LMP91000.h"
#include "MiniStatAnalyst.h"


#if defined(ARDUINO_ARCH_SAMD)
  #define SerialDebugger SerialUSB
#else
  #define SerialDebugger Serial
#endif


LMP91000 pStat = LMP91000();
MiniStatAnalyst analyst = MiniStatAnalyst();


const uint16_t arr_samples = 750;
uint16_t arr_cur_index = 0;
float volts[arr_samples] = {};
float amps[arr_samples] = {};


const uint16_t opVolt = 3330;
const uint8_t adcBits = 12;
const float v_tolerance = 0.0075;
const uint16_t dacResolution = pow(2,12)-1; //12-bit


//analog input pins to read voltages
const uint8_t LMP = A3;
const uint8_t dac = A0;
const uint8_t MENB = 5;

unsigned long lastTime = 0;
float RFB = 2200000; //in parallel with 15nF cap
uint16_t dacVout = 1500;

bool saveQueues = false;


void setup()
{
  Wire.begin();
  SerialDebugger.begin(115200);
  while(!SerialDebugger);

  analogReadResolution(12);
  analogWriteResolution(12);


  pStat.setMENB(MENB);


  //enable the potentiostat
  delay(50);
  pStat.standby();
  delay(50);
  initLMP(0);
  delay(2000); //warm-up time for the gas sensor
}


void loop()
{
  SerialDebugger.println(F("Ready!"));
  
  //will hold the code here until a character is sent over the SerialDebugger port
  //this ensures the experiment will only run when initiated
  while(!SerialDebugger.available());
  SerialDebugger.read();


  //prints column headings
  SerialDebugger.println(F("Time(ms),Voltage,Zero,LMP,Current"));


  //lmpGain, cycles, startV(mV), endV(mV), vertex1(mV), vertex2(mV), stepV(mV), rate (mV/s)
  //runCV(4, 2, 0, 0, 450, -200, 5, 100); //for FeCN
  runCV(0, 3, 0, 0, 50, -450, 2, 50); //for probe density
  //SerialDebugger.println("Backward Scan");
  //runCV(4, 2, 0, 0, -500, 600, 2, 100);

  float slope = 0;
  float intercept = 0;

  SerialDebugger.println(F("Ready!"));
  SerialDebugger.println(freeMemory(), DEC);
  while(!SerialDebugger.available());
  SerialDebugger.read();

  SerialDebugger.println(F("Voltage,Current"));
  for(uint16_t i = 0; i < arr_samples; i++)
  {
    SerialDebugger.print(volts[i]);
    SerialDebugger.print(F(","));
    SerialDebugger.println(amps[i]);
  }

  analyst.calcTangentLine(amps, volts, slope, intercept, arr_samples);

  SerialDebugger.println(F("Ready!"));
  while(!SerialDebugger.available());
  SerialDebugger.read();
  
  float current = analyst.getPeakCurrent(amps, volts, slope, intercept, arr_samples);

  SerialDebugger.println(F("Ready!"));
  while(!SerialDebugger.available());
  SerialDebugger.read();
  
  SerialDebugger.print("slope: ");
  SerialDebugger.print(slope,5);
  SerialDebugger.print(", ");
  SerialDebugger.print("intercept: ");
  SerialDebugger.print(intercept,5);
  SerialDebugger.println();
  SerialDebugger.print("current: ");
  SerialDebugger.print(current,5);
  SerialDebugger.println();
}


void initLMP(uint8_t lmpGain)
{
  pStat.disableFET();
  pStat.setGain(lmpGain);
  pStat.setRLoad(0);
  pStat.setExtRefSource();
  pStat.setIntZ(1);
  pStat.setThreeLead();
  pStat.setBias(0);
  pStat.setPosBias();

  setOutputsToZero();
}


void setOutputsToZero()
{
  analogWrite(dac,0);
  pStat.setBias(0);
}


void runCV(uint8_t lmpGain, uint8_t cycles, int16_t startV,
           int16_t endV, int16_t vertex1, int16_t vertex2,
           int16_t stepV, uint16_t rate)
{
  initLMP(lmpGain);
  stepV = abs(stepV);
  rate = (1000.0*stepV)/rate;


  lastTime = millis();
  if(vertex1 > startV) runCVForward(cycles,startV,endV,vertex1,vertex2,stepV,rate);
  else runCVBackward(cycles,startV,endV,vertex1,vertex2,stepV,rate);
}



void runCVForward(uint8_t cycles, int16_t startV, int16_t endV,
                  int16_t vertex1, int16_t vertex2, int16_t stepV, uint16_t rate)
{
  int16_t j = startV;
  
  for(uint8_t i = 0; i < cycles; i++)
  {
    if(i==cycles-2) saveQueues = true;
    else saveQueues = false;
    
    //j starts at startV
    for (j; j <= vertex1; j += stepV)
    {
      biasAndSample(j,rate);
    }
    j -= 2*stepV;
  
  
    //j starts right below the first vertex
    for (j; j >= vertex2; j -= stepV)
    {
      biasAndSample(j,rate);
    }
    j += 2*stepV;
  
  
    //j starts right above the second vertex
    for (j; j <= endV; j += stepV)
    {
      biasAndSample(j,rate);
    }
    j -= 2*stepV;
    
  }

  setOutputsToZero();
}


void runCVBackward(uint8_t cycles, int16_t startV, int16_t endV,
                   int16_t vertex1, int16_t vertex2, int16_t stepV, uint16_t rate)
{  
  int16_t j = startV;
  
  for(uint8_t i = 0; i < cycles; i++)
  {
    if(i==cycles-2) saveQueues = true;
    else saveQueues = false;
    
    //j starts at startV
    for (j; j >= vertex1; j -= stepV)
    {
      biasAndSample(j,rate);
    }
    j += 2*stepV;
    

    //j starts right above vertex1
    for (j; j <= vertex2; j += stepV)
    {
      biasAndSample(j,rate);
    }
    j -= 2*stepV;
  

    //j starts right below vertex2
    for (j; j >= endV; j -= stepV)
    {
      biasAndSample(j,rate);
    }
    j += 2*stepV;
    
  }

  setOutputsToZero();
}



void biasAndSample(int16_t voltage, uint16_t rate)
{
  SerialDebugger.print(millis());
  SerialDebugger.print(F(","));
  SerialDebugger.print(voltage);
  SerialDebugger.print(F(","));
  
  setLMPBias(voltage);
  setVoltage(voltage);


//  //if(saveQueues && (voltage < -48)) saveQueues = true;
//  if(voltage < -48) saveQueues = true;
//  else saveQueues = false;
  
  if(saveQueues && (arr_cur_index < arr_samples) && (voltage < -48))
  {
    volts[arr_cur_index] = voltage;
    arr_cur_index++;
  }

  
  while(millis() - lastTime < rate);
//  {
//    SerialDebugger.println("Am I stuck here?");
//  }
  
  sampleOutputs();
  SerialDebugger.println();
  //arr_cur_index++;
  lastTime = millis();
}


//void sampleOutputs()
//{
//  uint32_t num = 0;
//  uint8_t num_samples = 77;
//
//  for(uint8_t i = 0; i < num_samples; i++)
//  {
//    num += analogRead(LMP);
//  }
//
//  num = (float)num/num_samples;
//  
//  SerialDebugger.print(pStat.getVoltage(num, opVolt, adcBits));
//}


void sampleOutputs()
{
  float v1 = pStat.getVoltage(analogRead(LMP), opVolt, adcBits);
  float v2 = dacVout*.5;
  float current = (((v1-v2)/1000)/RFB)*pow(10,9); //scales to nA
  
  if(saveQueues && (arr_cur_index-1 < arr_samples))
  {
    amps[arr_cur_index-1] = current;
  }
  

  SerialDebugger.print(v1);
  SerialDebugger.print(",");
  SerialDebugger.print(current,2);
  
//  float current = pStat.getCurrent(analogRead(LMP),opVolt,adcBit,RFB);
//  SerialDebugger.print(pStat.getVoltage(analogRead(LMP), opVolt, adcBits));
//  SerialDebugger.print(",");
//  SerialDebugger.print(current,8);
//  amps.enqueue(current);
}


void setVoltage(int16_t voltage)
{
  //uint16_t dacVout = 1500;
  dacVout = 1500;
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
  analogWrite(dac,convertDACVoutToDACVal(dacVout));

  SerialDebugger.print(dacVout*.5);
  SerialDebugger.print(F(","));
}



//Convert the desired voltage
uint16_t convertDACVoutToDACVal(uint16_t dacVout)
{
  //return (dacVout-dacMin)*((float)dacResolution/dacSpan);
  return dacVout*((float)dacResolution/opVolt);
}



void setLMPBias(int16_t voltage)
{
  signed char sign = (float)voltage/abs(voltage);
  
  if(sign < 0) pStat.setNegBias();
  else if (sign > 0) pStat.setPosBias();
  else {} //do nothing
}



//bias potential
//AC signal
//points per decade
//repititions
//settling cycles
//start frequency
//end frequency
void runEIS(double startF, double )
{
  
}
