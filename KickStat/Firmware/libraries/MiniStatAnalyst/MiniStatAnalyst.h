/*
 FILENAME:	MiniStatAnalyst.h
 AUTHOR:	Orlando S. Hoilett
 EMAIL:		ohoilett@purdue.edu
 
 
 Please see .cpp file for extended descriptions, instructions, and version updates
 
 
 DISCLAIMER
 Linnes Lab code, firmware, and software is released under the MIT License
 (http://opensource.org/licenses/MIT).
 
 The MIT License (MIT)
 
 Copyright (c) 2019 Linnes Lab, Purdue University, West Lafayette, IN, USA
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 of the Software, and to permit persons to whom the Software is furnished to do
 so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 
 */


#ifndef MiniStatAnalyst_h
#define MiniStatAnalyst_h


//Standard Arduino libraries
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>


const float AVOGADRO = 6.022*pow(10,23);
const float FARADAY = 96485.33289;
const float CHARGE_TRANSFER_CONSTANT = 400; //400uC/cm2

const int8_t REDUCTION = -1;
const int8_t OXIDATION = 1;


//ERROR CODES
const uint16_t NO_ERROR = 0x00;

const uint16_t NO_REDOX_TYPE = 0x01;
const uint16_t NO_BASELINE_SELECTED = 0x02;
const uint16_t NO_BASELINE_FOUND = 0x03;
const uint16_t NO_PEAK_REGION_SELECTED = 0x04;


//Experiment Codes
const uint8_t MB_APTAMER_BASE = 0;
const uint8_t FeCN_OX_BASE = 1;
const uint8_t FeCN_RED_BASE = 2;

const uint8_t MB_APTAMER_PEAK = 3;
const uint8_t FeCN_OX_PEAK = 4;
const uint8_t FeCN_RED_PEAK = 5;



class MiniStatAnalyst
{
	
private:
	
	void checkVBounds(int16_t &v1, int16_t &v2, const int8_t redox);
	void setBounds(const uint8_t &exp, int16_t &v1, int16_t &v2, int8_t &redox);
	
	
public:
	
	MiniStatAnalyst(); //DEFAULT CONSTRUCTOR
	
	float calcDerivative(int16_t num1, int16_t num2, int16_t dt);
	
	//uint32_t calcResiduals(uint16_t samples);
	
	//void calcStDev();
	
	uint16_t calcBaseline(int16_t v1, int16_t v2, int8_t redox,
						  const float current[], const int16_t voltage[],
						  float &slope, float &intercept, uint16_t samples);
	uint16_t calcBaseline(uint8_t exp, const float current[],
						  const int16_t voltage[], float &slope,
						  float &intercept, uint16_t samples);
	
	uint16_t getPeakCurrent(int16_t v1, int16_t v2, int8_t redox,
							const float current[], const int16_t voltage[],
							const float slope, const float intercept,
							float &peak, int16_t &v_peak, uint16_t samples);	
	uint16_t getPeakCurrent(uint8_t exp, const float current[],
							const int16_t voltage[], const float slope,
							const float intercept, float &peak,
							int16_t &v_peak, uint16_t samples);
	
	
	float calcProbeDensity(float charge, uint8_t num_electrons, float area);
	float calcCharge(float current, int16_t voltage, uint16_t scan_rate);
	float calcArea(float charge);
	
	
	float calcPhase(float freq, float dt, uint8_t scale);
	float calcImg(float z, float theta);
	float calcReal(float z, float theta);
	
	float getMax(const float data[], uint16_t samples);
	float getMin(const float data[], uint16_t samples);
	float getPeaktoPeak(const float data[], uint16_t samples);
	float getPeaktoPeak(const float data[], uint16_t samples, float &max, float &min);
	
	float getAverage(const float data[], uint16_t samples);
	double getZeroCrossing(const float data[], const unsigned long time[],
						   const float avg, const uint16_t samples);
	
};


#endif /* MiniStatAnalyst_h */



/*
 
 
 Get all the data
 
 
 Find the baseline area
 - should be fitted with a line (so maybe find baseline by looking at first and
 second derivatives)
 
 
 Isolate the baseline area
 - either create a new array, or figure out which indices correspond to the
 baseline
 
 
 Find trendline
 - least squares method
 
 
 */



