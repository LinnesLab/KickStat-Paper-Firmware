/*
 * 
 * 2019-06-07-1616
 * CV works fine. Trying to combine SWV code from au9-swv-pbs-2019-02-05-0034
 * experiment. Realizing that SWV has two sets of current and deciding how to
 * deal with that for feeding the currents into the MiniStatAnalyst methods.
 * 
 * 
 * 2019-06-11-1138
 * Trying to combine my CV and SWV code
 * 
 * 
 * 2019-06-12-1346
 * Appear to have SWV and CV working correctly and was able to get data
 * with MiniStat and VSP at the same time.
 * 
 * 
 * 2019-06-13-0024
 * Added NPV and it appears to be working correctly
 * 
 * 
 * 2019-06-13-1111
 * Added EIS prototype and it appears to be going well.
 * 
 * 
 * 2019-06-16-2138
 * Doing some work with EIS. Working, just not fast enough. Now I need
 * to speed it up.
 * 
 * 2019-06-17-2113
 * Got to speed up EIS by modifying the ADC prescalers.
 * 
 * 2019-06-18-1715
 * Got to speed up EIS and verified working with limited setting (200mV bias
 * and 50mV AC). Need new method to optimize TIA bias settings for biasV+acv
 * and biasV-acv as well as correct for timing error.
 * 
 * 2019-06-18-2202
 * Got EIS working up to about 3.5 kHz with about 10 samples.
 * There is some voltage error up to 12% or so. Tested with a 1k
 * resistor, not with an electrochemical cell as yet.
 * 
 * 2019-06-19-0911
 * Got EIS working well and did my best to fix voltage error without
 * changing LMP91000 bias settings. It will only work on certain voltages
 * not all. The user will have to do
 *    (biasV+/-acv)/(TIA_bias)
 * to figure out if one bias can fit both biasV+acv and biasV-acv
 * 
 * 2019-06-24-1240
 * Got zero crossing detection working. That should give me phase detection.
 * 
 * 2019-10-01-1218
 * Changed TIA zero measurement to analog read sampling with Rev B MiniStat
 * 
 */


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


const uint16_t arr_samples = 2500; //use 1000 for EIS, can use 2500 for other experiments
uint16_t arr_cur_index = 0;
int16_t volts[arr_samples] = {0};
float amps[arr_samples] = {0};
unsigned long input_time[arr_samples] = {0};
unsigned long output_time[arr_samples] = {0};

float v1_array[arr_samples] = {0};
float v2_array[arr_samples] = {0};


const uint16_t opVolt = 3300; //3300 mV
const uint8_t adcBits = 12;
const float v_tolerance = 0.008; //0.0075 works every other technique with 1mV step except CV which needs minimum 2mV step
const uint16_t dacResolution = pow(2,10)-1;


//LMP91000 control pins
const uint8_t dac = A0;
const uint8_t MENB = 5;

//analog input pins to read voltages
const uint8_t LMP_C1 = A1;
const uint8_t LMP_C2 = A2;
const uint8_t LMP = A3;
const uint8_t anti_aliased = A4;


unsigned long lastTime = 0;
uint16_t dacVout = 1500;

bool saveQueues = false;

float RFB = 2200000; //in parallel with 15nF cap
//float RFB = 1494;
uint8_t LMPgain = 5;

uint8_t bias_setting = 0;


void setup()
{
  Wire.begin();
  SerialDebugger.begin(115200);
  while(!SerialDebugger);

  analogReadResolution(12);
  analogWriteResolution(10);
  initADC(false);

  //enable the potentiostat
  pStat.setMENB(MENB);
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


  //working variables
  float slope = 0;
  float intercept = 0;
  float peak = 0;
  int16_t v_peak = 0;


//  ////CYCLIC VOLTAMMETRY
//  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  //void runCV(uint8_t lmpGain, uint8_t cycles, int16_t startV,
//  //           int16_t endV, int16_t vertex1, int16_t vertex2,
//  //           int16_t stepV, uint16_t rate, bool setToZero)
//
//  LMPgain = 4;
//  runCV(LMPgain, 6, 0, 0, -200, 450, 1, 50, true);
//  
//  
//  SerialDebugger.println(F("Voltage,Current"));
//  for(uint16_t i = 0; i < arr_samples; i++)
//  {
//    SerialDebugger.print(volts[i]);
//    SerialDebugger.print(F(","));
//    SerialDebugger.println(amps[i]);
//  }
//
//
//  analyst.calcBaseline(FeCN_RED_BASE, amps, volts, slope, intercept, arr_samples);
//  analyst.getPeakCurrent(FeCN_RED_PEAK, amps, volts, slope, intercept, peak, v_peak, arr_samples);
//  printInfo(slope, intercept, peak, v_peak);
//  
//
//  analyst.calcBaseline(FeCN_OX_BASE, amps, volts, slope, intercept, arr_samples);
//  analyst.getPeakCurrent(FeCN_OX_PEAK, amps, volts, slope, intercept, peak, v_peak, arr_samples);
//  printInfo(slope, intercept, peak, v_peak);
//
//
//  SerialDebugger.println("Quiet time till starting SWV experiments");
//  //delay(10000);


//  ////SQUARE WAVE VOLTAMMETRY (Forward -- Oxidation)
//  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  for (uint8_t i = 0; i < 5; i++)
//  {
//    //working variables
//    slope = 0;
//    intercept = 0;
//    peak = 0;
//    v_peak = 0;
//
//
//    LMPgain = 3;
//    runSWV(LMPgain, -400, 500, 50, 1, 31.25, true);  
//  
//  
//    SerialDebugger.println(F("Voltage,Current"));
//    for(uint16_t i = 0; i < arr_samples; i++)
//    {
//      SerialDebugger.print(volts[i]);
//      SerialDebugger.print(F(","));
//      SerialDebugger.println(amps[i]);
//    }
//    
//  
//    analyst.calcBaseline(FeCN_OX_BASE, amps, volts, slope, intercept, arr_samples);
//    analyst.getPeakCurrent(FeCN_OX_PEAK, amps, volts, slope, intercept, peak, v_peak, arr_samples);
//    printInfo(slope, intercept, peak, v_peak);
//
//
//    SerialDebugger.println("Quiet time till next SWV run");
//    delay(1000);
//  }


//  SerialDebugger.println("Quiet time till starting NPV experiments");
  //delay(10000);
//  
//
  ////NORMAL PULSE VOLTAMMETRY
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //  void runNPV(uint8_t lmpGain, int16_t startV, int16_t endV,
  //            int8_t pulseAmp, uint32_t pulse_width, uint32_t pulse_period,
  //            uint32_t quietTime, uint8_t range, bool setToZero)
  for (uint8_t i = 0; i < 5; i++)
  {
    //working variables
    slope = 0;
    intercept = 0;
    peak = 0;
    v_peak = 0;
    
  
    LMPgain = 1;
    runNPV(LMPgain, -200, 500, 10, 50, 200, 500, 6, true);
  
  
    SerialDebugger.println(F("Voltage,Current"));
    for(uint16_t i = 0; i < arr_samples; i++)
    {
      SerialDebugger.print(volts[i]);
      SerialDebugger.print(F(","));
      SerialDebugger.println(amps[i]);
    }
  
    
    analyst.calcBaseline(FeCN_RED_BASE, amps, volts, slope, intercept, arr_samples);
    analyst.getPeakCurrent(FeCN_RED_PEAK, amps, volts, slope, intercept, peak, v_peak, arr_samples);
    printInfo(slope, intercept, peak, v_peak);
    
  
    analyst.calcBaseline(FeCN_OX_BASE, amps, volts, slope, intercept, arr_samples);
    analyst.getPeakCurrent(FeCN_OX_PEAK, amps, volts, slope, intercept, peak, v_peak, arr_samples);
    printInfo(slope, intercept, peak, v_peak);


    SerialDebugger.println("Quiet time till next NPV run");
    delay(1000);
  }
//  
//
//  SerialDebugger.println("Quiet time till starting Chronoamperometry experiments");
//  delay(10000);
//
//
//
//  ////CHRONOAMPEROMETRY
//  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  //void runAmp(uint8_t lmpGain, int16_t pre_stepV, uint32_t quietTime,
//  //            int16_t v1, uint32_t t1, int16_t v2, uint32_t t2,
//  //            uint16_t samples, uint8_t range, bool setToZero)
//  for(uint8_t i = 0; i < 5; i++)
//  {
//    LMPgain = 3;
//    runAmp(LMPgain, 174, 40000, -200, 40000, 200, 40000, 400, 6, true);
//  
//    SerialDebugger.println("Quiet time till next Amp run");
//    delay(10000);
//  }




//  ////CYCLIC VOLTAMMETRY
//  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  //void runCV(uint8_t lmpGain, uint8_t cycles, int16_t startV,
//  //           int16_t endV, int16_t vertex1, int16_t vertex2,
//  //           int16_t stepV, uint16_t rate, bool setToZero)
//
//  //for amptamer with good step resolution
//  slope = 0;
//  intercept = 0;
//  peak = 0;
//  v_peak = 0;
//  
//  LMPgain = 0;
//  RFB = 2200000; //in parallel with 15nF cap
//  runCV(LMPgain, 6, -40, -40, -20, -450, 1, 50, true);
//
// 
//  SerialDebugger.println(F("Voltage,Current"));
//  for(uint16_t i = 0; i < arr_samples; i++)
//  {
//    SerialDebugger.print(volts[i]);
//    SerialDebugger.print(F(","));
//    SerialDebugger.println(amps[i]);
//  }
//
//
//  analyst.calcBaseline(MB_APTAMER_BASE, amps, volts, slope, intercept, arr_samples);
//  analyst.getPeakCurrent(MB_APTAMER_PEAK, amps, volts, slope, intercept, peak, v_peak, arr_samples);
//  printInfo(slope, intercept, peak, v_peak);
//
//
//
//  //for amptamer with intentionally poor resolution
//  slope = 0;
//  intercept = 0;
//  peak = 0;
//  v_peak = 0;
//  
//  LMPgain = 0;
//  RFB = 2200000; //in parallel with 15nF cap
//  runCV(LMPgain, 6, -40, -40, -20, -465, 66, 50, true);
//
// 
//  SerialDebugger.println(F("Voltage,Current"));
//  for(uint16_t i = 0; i < arr_samples; i++)
//  {
//    SerialDebugger.print(volts[i]);
//    SerialDebugger.print(F(","));
//    SerialDebugger.println(amps[i]);
//  }
//
//  
//  analyst.calcBaseline(MB_APTAMER_BASE, amps, volts, slope, intercept, arr_samples);
//  analyst.getPeakCurrent(MB_APTAMER_PEAK, amps, volts, slope, intercept, peak, v_peak, arr_samples);
//  printInfo(slope, intercept, peak, v_peak);
//
//
//
//
//  ////SQUARE WAVE VOLTAMMETRY (Reverse -- Reduction)
//  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
////  void runSWV(uint8_t lmpGain, int16_t startV, int16_t endV,
////              int16_t pulseAmp, int16_t stepV, double freq, bool setToZero)
//
//
//  SerialDebugger.println(F("Waiting for SWV!"));
//  
//  //will hold the code here until a character is sent over the SerialDebugger port
//  //this ensures the experiment will only run when initiated
//  while(!SerialDebugger.available());
//  SerialDebugger.read();
//
//
//  for (uint8_t i = 0; i < 6; i++)
//  {
//    slope = 0;
//    intercept = 0;
//    peak = 0;
//    v_peak = 0;
//  
//    //for amptamer with intentionally poor step resolution
//    LMPgain = 0;
//    RFB = 180000; //in parallel with 10 nF cap
//    runSWV(LMPgain, -30, -500, 50, 66, 62.5, true);  
//  
//  
//    SerialDebugger.println(F("Voltage,Current"));
//    for(uint16_t i = 0; i < arr_samples; i++)
//    {
//      SerialDebugger.print(volts[i]);
//      SerialDebugger.print(F(","));
//      SerialDebugger.println(amps[i]);
//    }
//    
//  
//    analyst.calcBaseline(MB_APTAMER_BASE, amps, volts, slope, intercept, arr_samples);
//    analyst.getPeakCurrent(MB_APTAMER_PEAK, amps, volts, slope, intercept, peak, v_peak, arr_samples);
//    printInfo(slope, intercept, peak, v_peak);
//
//
//    SerialDebugger.println("Quiet time till next SWV run");
//    delay(10000);
//  }
//
//
//
//  LMPgain = 0;
//  RFB = 180000; //in parallel with 10 nF cap
//  for(uint8_t ab = 0; ab < 5; ab++)
//  {
//    slope = 0;
//    intercept = 0;
//    peak = 0;
//    v_peak = 0;
//    
//    //for amptamer with good step resolution
//    runSWV(LMPgain, -30, -500, 50, 1, 62.5, true);  
//  
//  
//    SerialDebugger.println(F("Voltage,Current"));
//    for(uint16_t i = 0; i < arr_samples; i++)
//    {
//      SerialDebugger.print(volts[i]);
//      SerialDebugger.print(F(","));
//      SerialDebugger.println(amps[i]);
//    }
//    
//  
//    analyst.calcBaseline(MB_APTAMER_BASE, amps, volts, slope, intercept, arr_samples);
//    analyst.getPeakCurrent(MB_APTAMER_PEAK, amps, volts, slope, intercept, peak, v_peak, arr_samples);
//    printInfo(slope, intercept, peak, v_peak);
//
//
//    SerialDebugger.println("Quiet time till next SWV run");
//    delay(10000);
//  }


}


//This method optimizes the ADC for faster operation. It's mostly
//intended for EIS measurements were faster sampling is necessary
//to scan at higher frequencies.
void initADC(bool fast)
{
  ADC->CTRLA.bit.ENABLE = 0; //disable the ADC while modifying
  while(ADC->STATUS.bit.SYNCBUSY == 1); //wait for synchronization
  
  //DIV32, 15-16us //below this, the ADC doesn't appear to be stable at all
  //DIV64, 20-21us
  //DIV128, 29-32us
  //DIV256, 54-55us
  //DIV512, 100us
  if(fast) ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV32 | ADC_CTRLB_RESSEL_12BIT;
  else ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV512 | ADC_CTRLB_RESSEL_12BIT;


  ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_1 |   // 1 sample   
                      ADC_AVGCTRL_ADJRES(0x00ul); // Adjusting result by 0
  ADC->SAMPCTRL.reg = 0x00;                        // Set max Sampling Time Length to half divided ADC clock pulse (5.33us)
  ADC->CTRLA.bit.ENABLE = 1; //enable ADC
  while(ADC->STATUS.bit.SYNCBUSY == 1); // Wait for synchronization
}


//Helper method to print some data to the Serial Monitor
//after running the peak detection algorithm.
void printInfo(float m, float b, float pk, int16_t vpk)
{
  SerialDebugger.print("slope: ");
  SerialDebugger.print(m,6);
  SerialDebugger.print(", ");
  SerialDebugger.print("intercept: ");
  SerialDebugger.print(b,6);
  SerialDebugger.println();
  SerialDebugger.print("Ep: ");
  SerialDebugger.print(vpk);
  SerialDebugger.print(", ");
  SerialDebugger.print("current: ");
  SerialDebugger.print(pk,2);
  SerialDebugger.println();
}


//Initializes the LMP91000 to the appropriate settings
//for operating MiniStat.
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


//Sets the electrochemical cell to 0V bias.
void setOutputsToZero()
{
  analogWrite(dac,0);
  pStat.setBias(0);
}


//void runCV()
//
//@param      lmpGain:    gain setting for LMP91000
//@param      cycles:     number of times to run the scan
//@param      startV:     voltage to start the scan
//@param      endV:       voltage to stop the scan
//@param      vertex1:    edge of the scan
//                        If vertex1 is greater than starting voltage, we start
//                        the experiment by running CV in the forward
//                        (oxidation) direction.
//@param      vertex2:    edge of the scan
//                        If vertex2 is greater than starting voltage, we start
//                        the experiment by running CV in the reverse
//                        (reduction) direction.
//@param      stepV:      how much to increment the voltage by
//@param      rate:       scanning rate
//                        in the case of CV, scanning rate is in mV/s
//@param      setToZero:  Boolean determining whether the bias potential of
//                        the electrochemical cell should be set to 0 at the
//                        end of the experiment.
//
//Runs the electrochemical technique, Cyclic Voltammetry. In this
//technique the electrochemical cell is biased at a series of
//voltages and the current at each subsequent voltage is measured.
void runCV(uint8_t lmpGain, uint8_t cycles, int16_t startV,
           int16_t endV, int16_t vertex1, int16_t vertex2,
           int16_t stepV, uint16_t rate, bool setToZero)
{
  SerialDebugger.println(F("Time(ms),Zero,LMP,Current,Cycle,Voltage,Current"));
  
  initLMP(lmpGain);
  //the method automatically handles if stepV needs to be positive or negative
  //no need for the user to specify one or the other
  //this step deals with that in case the user doesn't know
  stepV = abs(stepV);
  //figures out the delay needed to achieve a given scan rate
  //delay is dependent on desired rate and number of steps taken
  //more steps = smaller delay since we'll need to go by a bit faster
  //to sample at more steps vs. less steps, but at the same rate
  rate = (1000.0*stepV)/rate;  

  //Reset Arrays
  for(uint16_t i = 0; i < arr_samples; i++) volts[i] = 0;
  for(uint16_t i = 0; i < arr_samples; i++) amps[i] = 0;


  lastTime = millis();
  if(vertex1 > startV) runCVForward(cycles,startV,endV,vertex1,vertex2,stepV,rate);
  else runCVBackward(cycles,startV,endV,vertex1,vertex2,stepV,rate);


  //sets the bias of the electrochemical cell to 0V
  if(setToZero) setOutputsToZero();
  
  arr_cur_index = 0;
}


//@param      cycles:     number of times to run the scan
//@param      startV:     voltage to start the scan
//@param      endV:       voltage to stop the scan
//@param      vertex1:    edge of the scan
//                        If vertex1 is greater than starting voltage, we start
//                        the experiment by running CV in the forward
//                        (oxidation) direction.
//@param      vertex2:    edge of the scan
//                        If vertex2 is greater than starting voltage, we start
//                        the experiment by running CV in the reverse
//                        (reduction) direction.
//@param      stepV:      how much to increment the voltage by
//@param      rate:       scanning rate
//                        in the case of CV, scanning rate is in mV/s
//Runs CV in the forward (oxidation) direction first
void runCVForward(uint8_t cycles, int16_t startV, int16_t endV,
                  int16_t vertex1, int16_t vertex2, int16_t stepV, uint16_t rate)
{
  int16_t j = startV;
  float i_cv = 0;
  
  for(uint8_t i = 0; i < cycles; i++)
  {
    if(i==cycles-2) saveQueues = true;
    else saveQueues = false;
    
    //j starts at startV
    for (j; j <= vertex1; j += stepV)
    {
      i_cv = biasAndSample(j,rate);
      SerialDebugger.print(i+1);
      SerialDebugger.print(F(","));
      saveVoltammogram(j, i_cv, true);
      SerialDebugger.println();
    }
    j -= 2*stepV; //increment j twice to avoid biasing at the vertex twice
  
  
    //j starts right below the first vertex
    for (j; j >= vertex2; j -= stepV)
    {
      i_cv = biasAndSample(j,rate);
      SerialDebugger.print(i+1);
      SerialDebugger.print(F(","));
      saveVoltammogram(j, i_cv, true);
      SerialDebugger.println();
    }
    j += 2*stepV; //increment j twice to avoid biasing at the vertex twice
  
  
    //j starts right above the second vertex
    for (j; j <= endV; j += stepV)
    {
      i_cv = biasAndSample(j,rate);
      SerialDebugger.print(i+1);
      SerialDebugger.print(F(","));
      saveVoltammogram(j, i_cv, true);
      SerialDebugger.println();
    }
    j -= 2*stepV; //increment j twice to avoid biasing at the vertex twice
    
  }
}


//@param      cycles:     number of times to run the scan
//@param      startV:     voltage to start the scan
//@param      endV:       voltage to stop the scan
//@param      vertex1:    edge of the scan
//                        If vertex1 is greater than starting voltage, we start
//                        the experiment by running CV in the forward
//                        (oxidation) direction.
//@param      vertex2:    edge of the scan
//                        If vertex2 is greater than starting voltage, we start
//                        the experiment by running CV in the reverse
//                        (reduction) direction.
//@param      stepV:      how much to increment the voltage by
//@param      rate:       scanning rate
//                        in the case of CV, scanning rate is in mV/s
//Runs CV in the reverse (reduction) direction first
void runCVBackward(uint8_t cycles, int16_t startV, int16_t endV,
                   int16_t vertex1, int16_t vertex2, int16_t stepV, uint16_t rate)
{  
  int16_t j = startV;
  float i_cv = 0;
  
  for(uint8_t i = 0; i < cycles; i++)
  {
    if(i==cycles-2) saveQueues = true;
    else saveQueues = false;
    
    //j starts at startV
    for (j; j >= vertex1; j -= stepV)
    {
      i_cv = biasAndSample(j,rate);
      SerialDebugger.print(i+1);
      SerialDebugger.print(F(","));
      saveVoltammogram(j, i_cv, true);
      SerialDebugger.println();
    }
    j += 2*stepV; //increment j twice to avoid biasing at the vertex twice
    

    //j starts right above vertex1
    for (j; j <= vertex2; j += stepV)
    {
      i_cv = biasAndSample(j,rate);
      SerialDebugger.print(i+1);
      SerialDebugger.print(F(","));
      saveVoltammogram(j, i_cv, true);
      SerialDebugger.println();
    }
    j -= 2*stepV; //increment j twice to avoid biasing at the vertex twice
  

    //j starts right below vertex2
    for (j; j >= endV; j -= stepV)
    {
      i_cv = biasAndSample(j,rate);
      SerialDebugger.print(i+1);
      SerialDebugger.print(F(","));
      saveVoltammogram(j, i_cv, true);
      SerialDebugger.println();
    }
    j += 2*stepV; //increment j twice to avoid biasing at the vertex twice
    
  }
}


//void runSWV()
//
//@param      lmpGain:    gain setting for LMP91000
//@param      startV:     voltage to start the scan
//@param      endV:       voltage to stop the scan
//@param      pulseAmp:   amplitude of square wave
//@param      stepV:      how much to increment the voltage by
//@param      freq:       square wave frequency
//@param      setToZero:  Boolean determining whether the bias potential of
//                        the electrochemical cell should be set to 0 at the
//                        end of the experiment.
//
//Runs the electrochemical technique, Square Wave Voltammetry. In this
//technique the electrochemical cell is biased at a series of
//voltages with a superimposed squar wave on top of the bias voltage.
//The current is sampled on the forward and the reverse pulse.
//https://www.basinc.com/manuals/EC_epsilon/Techniques/Pulse/pulse#square
//
void runSWV(uint8_t lmpGain, int16_t startV, int16_t endV,
            int16_t pulseAmp, int16_t stepV, double freq, bool setToZero)
{
  SerialDebugger.println(F("Time(ms),Zero,LMP,Time(ms),Zero,LMP,Voltage,Current"));
  
  initLMP(lmpGain);
  stepV = abs(stepV);
  pulseAmp = abs(pulseAmp);
  freq = (uint16_t)(1000.0 / (2*freq)); //converts frequency to milliseconds

  //testing shows that time is usually off by 1 ms or so, therefore
  //we subtract 1 ms from the calculated rate to compensate
  freq -= 1;

  
  saveQueues = true;

  //Reset Arrays
  for(uint16_t i = 0; i < arr_samples; i++) volts[i] = 0;
  for(uint16_t i = 0; i < arr_samples; i++) amps[i] = 0;


  if(startV < endV) runSWVForward(startV, endV, pulseAmp, stepV, freq);
  else runSWVBackward(startV, endV, pulseAmp, stepV, freq);


  arr_cur_index = 0;
  if(setToZero) setOutputsToZero();
}


//void runSWVForward
//
//@param      startV:     voltage to start the scan
//@param      endV:       voltage to stop the scan
//@param      pulseAmp:   amplitude of square wave
//@param      stepV:      how much to increment the voltage by
//@param      freq:       square wave frequency
//
//Runs SWV in the forward (oxidation) direction. The bias potential is
//swept from a more negative voltage to a more positive voltage.
void runSWVForward(int16_t startV, int16_t endV, int16_t pulseAmp,
                   int16_t stepV, double freq)
{
  float i_forward = 0;
  float i_backward = 0;
  
  for (int16_t j = startV; j <= endV; j += stepV)
  {
    //positive pulse
    i_forward = biasAndSample(j + pulseAmp,freq);

    //negative pulse
    i_backward = biasAndSample(j - pulseAmp,freq);
    

    saveVoltammogram(j, i_forward-i_backward, true);
    SerialDebugger.println();
  }
}


//void runSWVBackward
//
//@param      startV:     voltage to start the scan
//@param      endV:       voltage to stop the scan
//@param      pulseAmp:   amplitude of square wave
//@param      stepV:      how much to increment the voltage by
//@param      freq:       square wave frequency
//
//Runs SWV in the backward (reduction) direction. The bias potential
//is swept from a more positivie voltage to a more negative voltage.
void runSWVBackward(int16_t startV, int16_t endV, int16_t pulseAmp,
                    int16_t stepV, double freq)
{
  float i_forward = 0;
  float i_backward = 0;
  
  for (int16_t j = startV; j >= endV; j -= stepV)
  {
    //positive pulse
    i_forward = biasAndSample(j + pulseAmp,freq);
    
    //negative pulse
    i_backward = biasAndSample(j - pulseAmp,freq);


    saveVoltammogram(j, i_forward-i_backward, true);
    SerialDebugger.println();
  }
}


//void runNPV()
//
//@param      lmpGain:      gain setting for LMP91000
//@param      startV:       voltage to start the scan
//@param      endV:         voltage to stop the scan
//@param      pulseAmp:     amplitude of square wave
//@param      pulse_period: the  of the excitation voltage
//@param      pulse_width:  the pulse-width of the excitation voltage
//@param      quietTime:    initial wait time before the first pulse
//@param      range:        the expected range of the measured current
//@param      setToZero:    Boolean determining whether the bias potential of
//                          the electrochemical cell should be set to 0 at the
//                          end of the experiment.
//
//Runs the electrochemical technique, Normal Pulse Voltammetry. In this
//technique the electrochemical cell is biased at increasing superimposed
//voltages. The current is sampled at the end of each step potential. The
//potential is returned to the startV at the end of each pulse period.
//https://www.basinc.com/manuals/EC_epsilon/Techniques/Pulse/pulse#normal
//
void runNPV(uint8_t lmpGain, int16_t startV, int16_t endV,
            int8_t pulseAmp, uint32_t pulse_width, uint32_t pulse_period,
            uint32_t quietTime, uint8_t range, bool setToZero)
{
  if(pulse_width > pulse_period)
  {
    uint32_t temp = pulse_width;
    pulse_width = pulse_period;
    pulse_period = temp;
  }

  
  //Print column headers
//  String current = "";
//  if(range == 12) current = "Current(pA)";
//  else if(range == 9) current = "Current(nA)";
//  else if(range == 6) current = "Current(uA)";
//  else if(range == 3) current = "Current(mA)";
//  else current = "SOME ERROR";
  
  //SerialDebugger.println("Time(ms),Zero(mV),LMP,i_forward,Time(ms),Zero(mV),LMP,i_forward,Voltag(mV),Current");
  SerialDebugger.println(F("Time(ms),Zero(mV),LMP,i_forward,Time(ms),Zero(mV),LMP,i_backward,Voltage(mV),Current"));

  //Reset Arrays
  for(uint16_t i = 0; i < arr_samples; i++) volts[i] = 0;
  for(uint16_t i = 0; i < arr_samples; i++) amps[i] = 0;

  
  initLMP(lmpGain);
  pulseAmp = abs(pulseAmp);
  uint32_t off_time = pulse_period - pulse_width;
  
  saveQueues = true;

  setLMPBias(startV);
  setVoltage(startV);
  SerialDebugger.println();
  
  unsigned long time1 = millis();
  while(millis() - time1 < quietTime);


  if(startV < endV) runNPVForward(startV, endV, pulseAmp, pulse_width, off_time);
  else runNPVBackward(startV, endV, pulseAmp, pulse_width, off_time);


  arr_cur_index = 0;
  if(setToZero) setOutputsToZero();
}


//void runNPVForward()
//
//@param      startV:       voltage to start the scan
//@param      endV:         voltage to stop the scan
//@param      pulseAmp:     amplitude of square wave
//@param      pulse_width:  the pulse-width of the excitation voltage
//@param      off_time:     
//
//Runs NPV in the forward (oxidation) direction. The bias potential
//is swept from a more negative voltage to a more positive voltage.
void runNPVForward(int16_t startV, int16_t endV, int8_t pulseAmp,
                   uint32_t pulse_width, uint32_t off_time)
{
  float i_forward = 0;
  float i_backward = 0;

  
  for(int16_t j = startV; j <= endV; j += pulseAmp)
  {
    i_forward = biasAndSample(j, pulse_width);
    i_backward = biasAndSample(startV, off_time);
    

    saveVoltammogram(j, i_forward-i_backward, true);
    SerialDebugger.println();
  }
}


//void runNPVBackward()
//
//@param      startV:       voltage to start the scan
//@param      endV:         voltage to stop the scan
//@param      pulseAmp:     amplitude of square wave
//@param      pulse_width:  the pulse-width of the excitation voltage
//@param      off_time:     
//
//Runs NPV in the reverse (reduction) direction. The bias potential
//is swept from a more positivie voltage to a more negative voltage.
void runNPVBackward(int16_t startV, int16_t endV, int8_t pulseAmp,
                   uint32_t pulse_width, uint32_t off_time)
{
  float i_forward = 0;
  float i_backward = 0;

  
  for(int16_t j = startV; j >= endV; j -= pulseAmp)
  {
    i_forward = biasAndSample(j, pulse_width);
    i_backward = biasAndSample(startV, off_time);
    
    
    saveVoltammogram(j, i_forward-i_backward, true);
    SerialDebugger.println();
  }
}


void runDPV(uint8_t lmpGain, int16_t startV, int16_t endV,
            int8_t pulseAmp, int8_t stepV, uint32_t pulse_period,
            uint32_t pulse_width, bool setToZero)
{
  //SerialDebugger.println("Time(ms),Zero(mV),LMP,Voltage(mV)," + current);
  SerialDebugger.println(F("Time(ms),Zero(mV),LMP,Voltage(mV),Current"));
  
  
  initLMP(lmpGain);
  stepV = abs(stepV);
  pulseAmp = abs(pulseAmp);
  uint32_t off_time = pulse_period - pulse_width;
  
  
  saveQueues = true;

  //Reset Arrays
  for(uint16_t i = 0; i < arr_samples; i++) volts[i] = 0;
  for(uint16_t i = 0; i < arr_samples; i++) amps[i] = 0;


  if(startV < endV) runDPVForward(startV, endV, pulseAmp, stepV, pulse_width, off_time);
  else runDPVBackward(startV, endV, pulseAmp, stepV, pulse_width, off_time);


  arr_cur_index = 0;
  if(setToZero) setOutputsToZero();
}


void runDPVForward(int16_t startV, int16_t endV, int8_t pulseAmp,
                   int8_t stepV, uint32_t pulse_width, uint32_t off_time)
{
  float i_forward = 0;
  float i_backward = 0;
  
  for (int16_t j = startV; j <= endV; j += stepV)
  {
    //baseline
    i_backward = biasAndSample(j, off_time);

    //pulse
    i_forward = biasAndSample(j + pulseAmp, pulse_width);
    

    saveVoltammogram(j, i_forward-i_backward, true);
    SerialDebugger.println();
  }
}


void runDPVBackward(int16_t startV, int16_t endV, int8_t pulseAmp,
                    int8_t stepV, uint32_t pulse_width, uint32_t off_time)
{
  float i_forward = 0;
  float i_backward = 0;
  
  for (int16_t j = startV; j >= endV; j -= stepV)
  {
    //baseline
    i_backward = biasAndSample(j, off_time);

    //pulse
    i_forward = biasAndSample(j + pulseAmp, pulse_width);


    saveVoltammogram(j, i_forward-i_backward, true);
    SerialDebugger.println();
  }
}


//void runAmp()
//
//@param      lmpGain:    gain setting for LMP91000
//@param      pre_stepV:  voltage to start the scan
//@param      quietTime:  initial wait time before the first pulse
//
//@param      v1:         the first step potential
//@param      t1:         how long we hold the cell at the first potential
//@param      v2:         the second step potential
//@param      t2:         how long we hold the cell at the second potential
//@param      samples:    how many times we sample the current at each potential
//@param      range:      the expected range of the measured current
//                          range = 12 is picoamperes
//                          range = 9 is nanoamperes
//                          range = 6 is microamperes
//                          range = 3 is milliamperes
//@param      setToZero:  Boolean determining whether the bias potential of
//                        the electrochemical cell should be set to 0 at the
//                        end of the experiment.
//
//Runs Amperometry technique. In Amperometry, the voltage is biased
//and held at one or maybe two potentials and the current is sampled
//while the current is held steady.
//https://www.basinc.com/manuals/EC_epsilon/Techniques/ChronoI/ca
//
void runAmp(uint8_t lmpGain, int16_t pre_stepV, uint32_t quietTime,
            int16_t v1, uint32_t t1, int16_t v2, uint32_t t2,
            uint16_t samples, uint8_t range, bool setToZero)
{
  //Print column headers
  String current = "";
  if(range == 12) current = "Current(pA)";
  else if(range == 9) current = "Current(nA)";
  else if(range == 6) current = "Current(uA)";
  else if(range == 3) current = "Current(mA)";
  else current = "SOME ERROR";


  if(samples > arr_samples/3) samples = arr_samples/3;
  initLMP(lmpGain);

  int16_t voltageArray[3] = {pre_stepV, v1, v2};
  uint32_t timeArray[3] = {quietTime, t1, t2};


  //Reset Arrays
  for(uint16_t i = 0; i < arr_samples; i++) volts[i] = 0;
  for(uint16_t i = 0; i < arr_samples; i++) output_time[i] = 0;
  for(uint16_t i = 0; i < arr_samples; i++) amps[i] = 0;


  //i = 0 is pre-step voltage
  //i = 1 is first step potential
  //i = 2 is second step potential
  for(uint8_t i = 0; i < 3; i++)
  {
    //the time between samples is determined by the
    //number of samples inputted by the user
    uint32_t fs = (double)timeArray[i]/samples;    
    unsigned long startTime = millis();


    //Print column headers
    SerialDebugger.println("Voltage(mV),Time(ms)," + current);

    
    //set bias potential
    setLMPBias(voltageArray[i]);
    setVoltage(voltageArray[i]);
    SerialDebugger.println();

    
    while(millis() - startTime < timeArray[i])
    {
      //output voltage of the transimpedance amplifier
      float v1 = pStat.getVoltage(analogRead(LMP), opVolt, adcBits);
      //float v2 = dacVout*.5; //the zero of the internal transimpedance amplifier
      float v2 = pStat.getVoltage(analogRead(LMP_C1), opVolt, adcBits); //the zero of the internal transimpedance amplifier
      float current = 0;

      
      //the current is determined by the zero of the transimpedance amplifier
      //from the output of the transimpedance amplifier, then dividing
      //by the feedback resistor
      //current = (V_OUT - V_IN-) / RFB
      //v1 and v2 are in milliVolts
      if(LMPgain == 0) current = (((v1-v2)/1000)/RFB)*pow(10,range); //scales to nA
      else current = (((v1-v2)/1000)/TIA_GAIN[LMPgain-1])*pow(10,range); //scales to nA


      
      //Sample and save data
      volts[arr_cur_index] = voltageArray[i];
      output_time[arr_cur_index] = millis();
      amps[arr_cur_index] = current;

      //Print data
      SerialDebugger.print(volts[arr_cur_index]);
      SerialDebugger.print(F(","));
      SerialDebugger.print(output_time[arr_cur_index]);
      SerialDebugger.print(F(","));
      SerialDebugger.print(amps[arr_cur_index]);
      SerialDebugger.println();


      arr_cur_index++;
      delay(fs-1); //the -1 is for adjusting for a slight offset
    }

    SerialDebugger.println();
    SerialDebugger.println();
  }


  arr_cur_index = 0;
  if(setToZero) setOutputsToZero();
}



//lmpGain:            Set the bias of the electrochemical cell.
//biasV:              The set potential of the experiment.
//acv:                The peak-to-peak voltage of the stimulating, AC voltage.
//startFreq:          The first scan frequency.
//endFreq:            The last scan freequenncy.
//ptsPerDecade:       The number of frequencies to be scanned for each
//                    decade of frequencies.
//samples:            The number of samples taken for each cycle at each frequency.
//                    Also the number of points to generate for the input sine wave.
//settlingCycles:     Number of cycles applied for each frequency before
//                    the data is recorded for future analysis.
//repeats:            How many times the entire EIS scan across all
//                    frequencies is repeated.
//setToZero:          Boolean determining whether the bias potential of
//                    the electrochemical cell should be set to 0 at the
//                    end of the experiment.
//
//Runs Electrochemical Impedance Spectroscopy (EIS) technique. This method
void runEIS(uint8_t lmpGain, int16_t biasV, int16_t acv,
            float startFreq, float endFreq, uint8_t ptsPerDecade,
            uint8_t samples, uint8_t settlingCycles,
            uint8_t repeats, bool setToZero)
{
  //SerialDebugger.println(F("Time(ms),Zero,LMP,Time(ms),Zero,LMP,Voltage,Current"));

  initADC(true); //sets the ADC to sample at highest obtainable frequency ~16us
  initLMP(lmpGain);
  acv = abs(acv);
  uint32_t dt = 0;
  
  
  saveQueues = true;
  float current = 0;
  const uint8_t maxSamples = 50;
  
  //limit number of samples
  if(samples > maxSamples) samples = maxSamples;

  //set eChem cell to bias potential
  setLMPBias(biasV);
  setVoltage(biasV, acv);


  const uint8_t timeOffset = 19;
  int16_t input_voltages[maxSamples*2] = {0};


  uint16_t num = 0; //helper variable
  for(uint8_t i = 0; i < repeats; i++)
  {
    //how to calculate points for log scale
    //https://en.wikipedia.org/wiki/Decade_(log_scale)
    //the index, j, must be a float since the frequencies can be floats
    for(float j = startFreq; j <= endFreq; j = startFreq*pow(10,(double)++num/ptsPerDecade))
    {
      SerialDebugger.println();
      SerialDebugger.println();
      SerialDebugger.print("Freq: ");
      SerialDebugger.println(j);
      delay(2000);
      dt = (uint32_t)(1000000.0 / (samples*j));
      //dt = dt-timeOffset;


      //generates sine values
      float sineValues[maxSamples] = {0};
      for(uint8_t k = 0; k < samples; k++)
      {
        //determines voltage based on numbers of points to
        //generate within 1 cycle as well as frequency
        sineValues[k] = biasV+acv*sin(2*PI*j*(k*dt/1000000.0));
        SerialDebugger.print(sineValues[k]);
        v2_array[k] = (sineValues[k]/TIA_BIAS[bias_setting])*.5;
        sineValues[k] = convertDACVoutToDACVal(sineValues[k]/TIA_BIAS[bias_setting]);
        SerialDebugger.print(",");
        SerialDebugger.print(sineValues[k]);
        SerialDebugger.println();
        //v2_array[k] = dacVout*.5;
      }


      const uint8_t cyc = settlingCycles-4;
      for(uint8_t l = 0; l < settlingCycles; l++)
      {
        if(l >= cyc) saveQueues = true;
        else saveQueues = false;
        
        for(uint16_t m = 0; m < samples; m++)
        {
          if(saveQueues && m + samples*(l-cyc) < arr_samples)
          {
            input_time[m + samples*(l-cyc)] = micros(); //mark time for voltage measurement 
            input_voltages[m] = sineValues[m];
          }
          
          analogWrite(dac,sineValues[m]); //set bias potential here


          //To-Do: deal with the offset better. It's not consistent across different frequencies
          //while(micros() - lastTime < (dt-timeOffset)); //wait for a time
          //while(micros() - lastTime < dt); //wait for a time

          
          if(saveQueues && m + samples*(l-cyc) < arr_samples)
          {
            //volts[m + samples*(l-cyc)] = micros(); //mark time for current measurement

            
            //To-Do: deal with the offset better. It's not consistent across different frequencies
            //output_time[m + samples*(l-cyc)] = micros()-dt+timeOffset; //mark time for current measurement
            output_time[m + samples*(l-cyc)] = micros(); //mark time for current measurement
            v1_array[m + samples*(l-cyc)] = analogRead(LMP); //sample
          }

          
          while(micros() - lastTime < dt); //wait for a time
          lastTime = micros(); //set timestamp
        }
      }


      for(uint16_t x = 0; x < samples*(settlingCycles-cyc-1); x++)
      {
        SerialDebugger.print(v1_array[x]);
        SerialDebugger.println();
      }


      for(uint16_t n = 0; n < samples*(settlingCycles-cyc-1); n++)
      {
        if(LMPgain == 0)
        {
          v1_array[n] = pStat.getVoltage(v1_array[n], opVolt, adcBits);
          current = (((v1_array[n] - v2_array[n%samples])/1000.0)/RFB)*pow(10,9);
        }
        else
        {
          v1_array[n] = pStat.getVoltage(v1_array[n], opVolt, adcBits);
          current = (((v1_array[n] - v2_array[n%samples])/1000.0)/TIA_GAIN[LMPgain-1])*pow(10,6);
        }

        if(saveQueues && n < arr_samples) amps[n] = current;
      }


      //SerialDebugger.println(F("Input Time,Input Voltage,Output Time,Current,LMP,Zero"));
      SerialDebugger.println(F("Input Time,Input Voltage,Output Time,Current,LMP,DAC"));
      for(uint16_t p = 0; p < samples*(settlingCycles-cyc-1); p++)
      {
        SerialDebugger.print(input_time[p]);
        SerialDebugger.print(F(", "));
        SerialDebugger.print(input_voltages[p]);
        SerialDebugger.print(F(", "));
        SerialDebugger.print(output_time[p]);
        SerialDebugger.print(F(", "));
        SerialDebugger.print(amps[p]);
        //SerialDebugger.println();
        SerialDebugger.print(F(", "));
        SerialDebugger.print(v1_array[p]);
        SerialDebugger.print(F(", "));
        SerialDebugger.print(v2_array[p%samples]);
//        SerialDebugger.print(F(", "));
//        SerialDebugger.print(freeMemory(), DEC);  // print how much RAM is available.
        SerialDebugger.println();
      }


      float avg_input = analyst.getAverage(sineValues, samples);
      float avg_output = analyst.getAverage(amps, samples);
      
      float t_zero_input = analyst.getZeroCrossing(sineValues, input_time, avg_input, samples);
      float t_zero_output = analyst.getZeroCrossing(amps, output_time, avg_output, samples);

      float phase = analyst.calcPhase(j, t_zero_output-t_zero_input, 6);
      float mag = analyst.getPeaktoPeak(amps, samples);


      SerialDebugger.print(F("t_zero_input: "));
      SerialDebugger.print(t_zero_input);
      SerialDebugger.print(F(", "));
      SerialDebugger.print(F("t_zero_output: "));
      SerialDebugger.print(t_zero_output);
      SerialDebugger.println();
      
      SerialDebugger.print(F("mag: "));
      SerialDebugger.print(mag);
      SerialDebugger.print(F(", "));
      SerialDebugger.print(F("phase: "));
      SerialDebugger.print(phase);
      SerialDebugger.println();
      
      SerialDebugger.println(freeMemory(), DEC);  // print how much RAM is available.
      
    }

  }


  initADC(false);
  if(setToZero) setOutputsToZero();

}


//inline float biasAndSample(int16_t voltage, uint32_t rate)
//@param        voltage: Set the bias of the electrochemical cell
//@param        rate:    How much to time should we wait to sample
//                       current after biasing the cell. This parameter
//                       sets the scan rate or the excitation frequency
//                       based on which electrochemical technique
//                       we're running.
//
//Sets the bias of the electrochemical cell then samples the resulting current.
inline float biasAndSample(int16_t voltage, uint32_t rate)
{
  SerialDebugger.print(millis());
  SerialDebugger.print(F(","));

  setLMPBias(voltage);
  setVoltage(voltage);
  

  //delay sampling to set scan rate
  while(millis() - lastTime < rate);


  //output voltage of the transimpedance amplifier
  float v1 = pStat.getVoltage(analogRead(LMP), opVolt, adcBits);
  //float v2 = dacVout*.5; //the zero of the internal transimpedance amplifier
  //the zero of the internal transimpedance amplifier
  float v2 = pStat.getVoltage(analogRead(LMP_C1), opVolt, adcBits);
  float current = 0;
  

  //the current is determined by the zero of the transimpedance amplifier
  //from the output of the transimpedance amplifier, then dividing
  //by the feedback resistor
  //current = (V_OUT - V_IN-) / RFB
  //v1 and v2 are in milliVolts
  if(LMPgain == 0) current = (((v1-v2)/1000)/RFB)*pow(10,9); //scales to nA
  else current = (((v1-v2)/1000)/TIA_GAIN[LMPgain-1])*pow(10,6); //scales to uA
  

  SerialDebugger.print(v1);
  SerialDebugger.print(F(","));
  SerialDebugger.print(current);
  SerialDebugger.print(F(","));

  //update timestamp for the next measurement
  lastTime = millis();
  
  return current;
}


//inline void saveVoltammogram(float voltage, float current, bool debug)
//@param        voltage: voltage or time depending on type of experiment
//                       voltage for voltammetry, time for time for
//                       time evolution experiments like chronoamperometry
//@param        current: current from the electrochemical cell
//@param        debug:   flag for determining whether or not to print to
//                       serial monitor
//
//Save voltammogram data to corresponding arrays.
inline void saveVoltammogram(float voltage, float current, bool debug)
{
  if(saveQueues && arr_cur_index < arr_samples)
  {
    volts[arr_cur_index] = (int16_t)voltage;
    amps[arr_cur_index] = current;
    arr_cur_index++;
  }

  if(debug)
  {
    SerialDebugger.print(voltage);
    SerialDebugger.print(F(","));
    SerialDebugger.print(current);
  }
}


inline void setVoltage(int16_t voltage)
{
  //Minimum DAC voltage that can be set
  //The LMP91000 accepts a minium value of 1.5V, adding the 
  //additional 20 mV for the sake of a bit of a buffer
  const uint16_t minDACVoltage = 1520;
  
  dacVout = minDACVoltage;
  bias_setting = 0;


  //voltage cannot be set to less than 15mV because the LMP91000
  //accepts a minium of 1.5V at its VREF pin and has 1% as its
  //lowest bias option 1.5V*1% = 15mV
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


//
//
//@param      voltage:    bias potential for the electrochemical cell
//@acv        acv:        amplitude of the AC signal
//
//
inline void setVoltage(int16_t voltage, int16_t acv)
{
  //Minimum DAC voltage that can be set
  //The LMP91000 accepts a minium value of 1.5V, adding the 
  //additional 20 mV for the sake of a bit of a buffer
  const uint16_t minDACVoltage = 1520;
  
  dacVout = minDACVoltage;
  bias_setting = 0;


  //voltage cannot be set to less than 15mV because the LMP91000
  //accepts a minium of 1.5V at its VREF pin and has 1% as its
  //lowest bias option 1.5V*1% = 15mV
  if(abs(voltage) < 15) voltage = 15*(voltage/abs(voltage));    


  //Variable, setV, 
  int16_t setV = dacVout*TIA_BIAS[bias_setting];
  voltage = abs(voltage);
  
  
  while( setV > voltage*(1+v_tolerance) || setV < voltage*(1-v_tolerance) )
  {
    if(bias_setting == 0) bias_setting = 1;

    
    dacVout = voltage/TIA_BIAS[bias_setting];
    
    if ( dacVout > opVolt || (voltage+acv)/TIA_BIAS[bias_setting] > opVolt )
    {
      bias_setting++;
      dacVout = minDACVoltage;

      if(bias_setting > NUM_TIA_BIAS) bias_setting = 1;
    }
    if ( (voltage-acv)/TIA_BIAS[bias_setting] < minDACVoltage )
    {
      bias_setting--;
      if(bias_setting > NUM_TIA_BIAS) bias_setting = 1;
    }
    
    setV = dacVout*TIA_BIAS[bias_setting];    
  }


  pStat.setBias(bias_setting);
  analogWrite(dac,convertDACVoutToDACVal(dacVout));

  SerialDebugger.print(dacVout*.5);
  SerialDebugger.print(F(","));
}


//inline uint16_t convertDACVoutToDACVal()
//dacVout       voltage output to set the DAC to
//
//Determines the correct value to write to the digital-to-analog
//converter (DAC) given the desired voltage, the resolution of the DAC
inline uint16_t convertDACVoutToDACVal(uint16_t dacVout)
{
  return dacVout*((float)dacResolution/opVolt);
}


//Sets the LMP91000's bias to positive or negative
inline void setLMPBias(int16_t voltage)
{
  signed char sign = (float)voltage/abs(voltage);
  
  if(sign < 0) pStat.setNegBias();
  else if (sign > 0) pStat.setPosBias();
  else {} //do nothing
}
