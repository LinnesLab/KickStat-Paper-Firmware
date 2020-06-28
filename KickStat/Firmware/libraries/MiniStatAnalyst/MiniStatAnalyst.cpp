/*
 FILENAME:	MiniStatAnalyst.cpp
 AUTHOR:	Orlando S. Hoilett
 CONTACT:	ohoilett@purdue.edu
 VERSION:	1.0.1
 
 
 AFFILIATIONS
 Linnes Lab, Weldon School of Biomedical Engineering,
 		Purdue University, West Lafayette, IN 47907

 
 DESCRIPTION
 
 
 
 UPDATES
 Version 1.0.1
 2020/03/12:1748>
			Updated descriptions and comments. Also reorganized folder structures
 			to comply wit Arduino library guidelines by using "extras" folder
 			to store supplementary info for the class.
 
 
 
 DISCLAIMER
 Linnes Lab code, firmware, and software is released under the
 MIT License (http://opensource.org/licenses/MIT).
 
 The MIT License (MIT)
 
 Copyright (c) 2019 Linnes Lab, Purdue University
 
 Permission is hereby granted, free of charge, to any person obtaining a copy of
 this software and associated documentation files (the "Software"), to deal in
 the Software without restriction, including without limitation the rights to
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



#include "MiniStatAnalyst.h"


//DEFAULT CONSTRUCTOR
//MiniStatAnalyst::MiniStatAnalyst()
//Initializes class object
//
MiniStatAnalyst::MiniStatAnalyst()
{
}


//num1 and num2			two points
//dt					difference in time between the two points of interest
//float					intentionally using float since it uses less space than double
//
//Calculates the derviate between two points
float MiniStatAnalyst::calcDerivative(int16_t num1, int16_t num2, int16_t dt)
{
	return (num2-num1)/(float)dt;
}


//uint32_t MiniStatAnalyst::calcResiduals(uint16_t samples)
//{
//	uint32_t sum = 0;
//	for (uint16_t i = 0; i < samples; i++)
//	{
//		sum += pow(num2-num1,2);
//	}
//	
//	return sum;
//}



//
//Private helper method (should not be used outside of the class) that rearranges
//the bounds
void MiniStatAnalyst::checkVBounds(int16_t &v1, int16_t &v2, const int8_t redox)
{
	//for the reduction scan, v1 must be larger than v2 since reduction goes
	//from more positive number to more negative number
	if(redox == REDUCTION)
	{
		if(v2 > v1)
		{
			int16_t temp = v1; //create temp variable to facilitate swapping
			v1 = v2;
			v2 = temp;
		}
	}
	//for the oxidation scan, v1 must be smaller than v2 since oxiation goes
	//from more negative number to more positive number
	else if(redox == OXIDATION)
	{
		if(v1 > v2)
		{
			int16_t temp = v1; //create temp variable to facilitate swapping
			v1 = v2;
			v2 = temp;
		}
	}
}


void MiniStatAnalyst::setBounds(const uint8_t &exp, int16_t &v1, int16_t &v2, int8_t &redox)
{
	
}


//This seems useful to know https://en.wikipedia.org/wiki/Linear_least_squares#Example,
//although this is really the guide this method is based on https://youtu.be/1C3olrs1CUw
//void MiniStatAnalyst::calcTangentLine(Queue<float> current_copy,
//									  Queue<float> voltage_copy,
//									  float &slope, float &intercept)
uint16_t MiniStatAnalyst::calcBaseline(int16_t v1, int16_t v2, int8_t redox,
									   const float current[], const int16_t voltage[],
									   float &slope, float &intercept, uint16_t samples)
{
	if(redox != REDUCTION || redox != OXIDATION) return NO_REDOX_TYPE;

	if(v2 == v1) return NO_BASELINE_SELECTED;
	else checkVBounds(v1, v2, redox);
	
	//Find the voltage increments for the voltammogram. This is used later to
	//find when the user-selected baseline region, v1 and v2, are found in the
	//data stored in the voltage array
	int16_t v_incr = abs(voltage[0]-voltage[1]);
	
	
	//initializes working variables
	uint16_t i = 0;
	float xy_sum = 0;
	float x_sum = 0;
	float y_sum = 0;
	float x_squared_sum = 0;
	
	
	//Searches for the index holding the start bounds of the baseline.
	//Two things must be true:
	//1. The current voltage in the array must be within one index of the start
	//	 specified start-bound
	//2. The section of the array being scanned must be the correct redox type.
	//	 For reduction, the voltages should be decreasing. For oxidation, the
	//	 voltages should be increasing.
	while(!(v1 >= voltage[i] - v_incr && v1 <= voltage[i] + v_incr
			&& (voltage[i+1]-voltage[i])/abs(voltage[i+1]-voltage[i]) == redox))
	{
		i++;
		if(i > samples) return NO_BASELINE_FOUND;
	}
	
	
	//count variable to keep track of how many values are involved in the
	//subsequent calculations. This is not the same as the array index as the
	//array will hold values other values other than the baseline
	uint16_t count = 0;
	while(!(v2 >= voltage[i] - v_incr && v2 <= voltage[i] + v_incr) && (i+1) < samples)
	{
		xy_sum += voltage[i]*current[i];
		x_sum += voltage[i];
		y_sum += current[i];
		x_squared_sum += voltage[i]*voltage[i];
		
		i++;
		count++;
		if(i > samples) return NO_BASELINE_FOUND;
	}
	
	
	//Updates variables. Since slope and intercept were passed by reference,
	//there is no need to return them using the "return" statement. The return
	//statement in this function instead returns error/success codes.
	slope = (xy_sum - (x_sum*y_sum/(float)count))/(x_squared_sum - (x_sum*x_sum/(float)count));
	intercept = y_sum/(float)count - slope*x_sum/(float)count;
	
	//if you get this far, then everything worked correctly
	return NO_ERROR;
}



//
//
//exp			stored baseline regions for voltammograms of common redox moieties
//current		array storing the currents from the input voltammogram
//voltage		array storing the voltages from the input voltammogram
//slope			variable for storing the value of the baseline's slope from
//					the equation y =  mx + b where m is the slope
//intercept		variable for storing the value of the baseline's intercept from
//					the equation y =  mx + b where b is the intercept
//samples		number of possible values to analyze before returning the
//					baseline information or returning an error if no baseline is found
//
//Calculates the tangent to the baseline preceding the redox peak. The baseline
//is calculated by using the least squared method. To calculate the baseline,
//this method determines the slope and the intercept of the line and updates
//the respective variables that were passed by reference.
//
uint16_t MiniStatAnalyst::calcBaseline(uint8_t exp, const float current[],
									   const int16_t voltage[], float &slope,
									   float &intercept, uint16_t samples)
{
	//Initializes working variables
	int8_t redox = 0;
	int16_t v1 = 0;
	int16_t v2 = 0;
	int16_t v_incr = abs(voltage[0]-voltage[1]);
	
	//uses the experiment keywords to automatically set the range for
	//the baseline based on previous knowledge of the resulting voltammograms
	//
	//MB Aptamer is a DNA Aptamer that uses methylene blue as its redox moiety.
	//FeCN is potassium ferricyanide.
	//OX is oxidation and RED is reduction.
	if(exp == MB_APTAMER_BASE)
	{
		redox = REDUCTION;
		v1 = -50;
		v2 = -200;
	}
	else if (exp == FeCN_OX_BASE)
	{
		redox = OXIDATION;
		v1 = -185;
		v2 = -30;
	}
	else if (exp == FeCN_RED_BASE)
	{
		redox = REDUCTION;
		v1 = 400;
		v2 = 300;
	}
	else return NO_BASELINE_SELECTED;
	
	
	//initializes working variables
	uint16_t i = 0;
	float xy_sum = 0;
	float x_sum = 0;
	float y_sum = 0;
	float x_squared_sum = 0;
	
	
	//Searches for the index holding the start bounds of the baseline.
	//Two things must be true:
	//1. The current voltage in the array must be within one index of the start
	//	 specified start-bound
	//2. The section of the array being scanned must be the correct redox type.
	//	 For reduction, the voltages should be decreasing. For oxidation, the
	//	 voltages should be increasing.
	while(!(v1 >= voltage[i] - v_incr && v1 <= voltage[i] + v_incr
			&& (voltage[i+1]-voltage[i])/abs(voltage[i+1]-voltage[i]) == redox))
	{
		i++;
		if(i > samples) return NO_BASELINE_FOUND;
	}
	
	
	//count variable to keep track of how many values are involved in the
	//subsequent calculations. This is not the same as the array index as the
	//array will hold values other values other than the baseline
	uint16_t count = 0;
	while(!(v2 >= voltage[i] - v_incr && v2 <= voltage[i] + v_incr) && (i+1) < samples)
	{
		xy_sum += voltage[i]*current[i];
		x_sum += voltage[i];
		y_sum += current[i];
		x_squared_sum += voltage[i]*voltage[i];
		
		i++;
		count++;
		if(i > samples) return NO_BASELINE_FOUND;
	}
	
	
	//Updates variables. Since slope and intercept were passed by reference,
	//there is no need to return them using the "return" statement. The return
	//statement in this function instead returns error/success codes.
	slope = (xy_sum - (x_sum*y_sum/(float)count))/(x_squared_sum - (x_sum*x_sum/(float)count));
	intercept = y_sum/(float)count - slope*x_sum/(float)count;
	
	//if you get this far, then everything worked correctly
	return NO_ERROR;
}



//float MiniStatAnalyst::getPeakCurrent(Queue<float> current_copy,
//									   Queue<float> voltage_copy,
//									   float slope, float intercept)
uint16_t MiniStatAnalyst::getPeakCurrent(int16_t v1, int16_t v2, int8_t redox,
										 const float current[], const int16_t voltage[],
										 const float slope, const float intercept,
										 float &peak, int16_t &v_peak, uint16_t samples)
{
	if(redox != REDUCTION || redox != OXIDATION) return NO_REDOX_TYPE;
	
	if(v2 == v1) return NO_PEAK_REGION_SELECTED;
	checkVBounds(v1, v2, redox);
	
	int16_t v_incr = abs(voltage[0]-voltage[1]);
	
	
	//Searches for the index holding the start bounds of the peak region.
	//Two things must be true:
	//1. The current voltage in the array must be within one index of the start
	//	 specified start-bound
	//2. The section of the array being scanned must be the correct redox type.
	//	 For reduction, the voltages should be decreasing. For oxidation, the
	//	 voltages should be increasing.
	uint16_t i = 0;
	while(!(v1 >= voltage[i] - v_incr && v1 <= voltage[i] + v_incr
			&& (voltage[i+1]-voltage[i])/abs(voltage[i+1]-voltage[i]) == redox))
	{
		i++;
		if(i > samples) return NO_BASELINE_FOUND;
	}


	//Set the current index as the peak, then increment the index.
	peak = current[i] - (slope*voltage[i]+intercept);
	v_peak = voltage[i];
	i++;
	
	
	//Runs until we reach the final bound or the index is incremented greater
	//than the number of samples in the range of the peak region.
	while(!(v2 >= voltage[i] - v_incr && v2 <= voltage[i] + v_incr) && (i+1) < samples)
	{
		//If the current index is contains a value greater than the absolute
		//value of the peak, then update the value of the peak.
		if(abs(peak) < abs(current[i] - (slope*voltage[i]+intercept)))
		{
			peak = current[i] - (slope*voltage[i]+intercept);
			v_peak = voltage[i];
		}
		
		i++;
		if(i > samples) return NO_BASELINE_FOUND;
	}

	
	return NO_ERROR;
}



//exp			stored peak regions for voltammograms of common redox moieties
//current		array storing the currents from the input voltammogram
//voltage		array storing the voltages from the input voltammogram
//slope			slope of the tangent to the baseline from the equation
//					y =  mx + b where m is the slope
//intercept		y-intercept of the tangent to the baseline from the equation
//					y =  mx + b where b is the y-intercept
//samples		number of possible values to analyze before returning the peak
//					or returning an error if no peak is found
//
//Obtains the peak current of the input voltammogram with respect to the tangent
//to the baseline.
//
uint16_t MiniStatAnalyst::getPeakCurrent(uint8_t exp, const float current[],
									  const int16_t voltage[], const float slope,
									  const float intercept, float &peak,
									  int16_t &v_peak, uint16_t samples)
{
	//initializes working variables
	int8_t redox = 0;
	int16_t v1 = 0;
	int16_t v2 = 0;
	int16_t v_incr = abs(voltage[0]-voltage[1]);

	
	//uses the experiment keywords to automatically set the range for
	//the redox peak based on previous knowledge of the resulting voltammograms
	//voltages are in milliVolts
	//
	//MB Aptamer is a DNA Aptamer that uses methylene blue as its redox moiety.
	//FeCN is potassium ferricyanide.
	//OX is oxidation and RED is reduction.
	if(exp == MB_APTAMER_PEAK)
	{
		redox = REDUCTION;
		v1 = -315;
		v2 = -375;
	}
	else if (exp == FeCN_OX_PEAK)
	{
		redox = OXIDATION;
		v1 = 140;
		v2 = 300;
	}
	else if (exp == FeCN_RED_PEAK)
	{
		redox = REDUCTION;
		v1 = 170;
		v2 = 30;
	}
	else return NO_BASELINE_SELECTED;
	
	
	//Searches for the index holding the start bounds of the peak region.
	//Two things must be true:
	//1. The current voltage in the array must be within one index of the start
	//	 specified start-bound
	//2. The section of the array being scanned must be the correct redox type.
	//	 For reduction, the voltages should be decreasing. For oxidation, the
	//	 voltages should be increasing.
	uint16_t i = 0;
	while(!(v1 >= voltage[i] - v_incr && v1 <= voltage[i] + v_incr
			&& (voltage[i+1]-voltage[i])/abs(voltage[i+1]-voltage[i]) == redox))
	{
		i++;
		if(i > samples) return NO_BASELINE_FOUND;
	}
	
	
	//Set the current index as the peak, then increment the index.
	peak = current[i] - (slope*voltage[i]+intercept);
	v_peak = voltage[i];
	i++;
	
	
	//Runs until we reach the final bound or the index is incremented greater
	//than the number of samples in the range of the peak region.
	while(!(v2 >= voltage[i] - v_incr && v2 <= voltage[i] + v_incr) && (i+1) < samples)
	{
		//If the current index is contains a value greater than the absolute
		//value of the peak, then update the value of the peak.
		if(abs(peak) < abs(current[i] - (slope*voltage[i]+intercept)))
		{
			peak = current[i] - (slope*voltage[i]+intercept);
			v_peak = voltage[i];
		}
		
		i++;
		if(i > samples) return NO_BASELINE_FOUND;
	}
	
	
	return NO_ERROR;
}


//Γ=(N_A Q)/nFA
float MiniStatAnalyst::calcProbeDensity(float charge, uint8_t num_electrons, float area)
{
	return (AVOGADRO*charge)/(num_electrons*FARADAY*area);
}


//Q = i*V/v
float MiniStatAnalyst::calcCharge(float current, int16_t voltage, uint16_t scan_rate)
{
	return current*voltage/scan_rate;
}


//float MiniStatAnalyst::calcArea(float charge)
//@param			charge: 
//Calculate the area of the electrode given using the technique
//
float MiniStatAnalyst::calcArea(float charge)
{
	return charge/CHARGE_TRANSFER_CONSTANT;
}



float MiniStatAnalyst::calcPhase(float freq, float dt, uint8_t scale)
{
	return 360*freq*dt/pow(10,scale);
}



float MiniStatAnalyst::calcImg(float z, float theta)
{
	return z*sin(theta*PI/180);
}



////do i call it imp, z, mag
////do i call it phase, theta, angle
float MiniStatAnalyst::calcReal(float z, float theta)
{
	return z*cos(theta*PI/180);
}


//float MiniStatAnalyst::getMax()
//data				array containing data to be analyzed
//samples			number of elements in input array
//
//Determines maximum value of input data array.
float MiniStatAnalyst::getMax(const float data[], uint16_t samples)
{
	//assumes first value is the max, then checks if any
	//subsequent value in the array is bigger than the first
	float max = data[0];
	
	for(uint16_t i = 1; i < samples; i++)
	{
		if(data[i] > max) max = data[i];
	}
	
	return max;
}


//float MiniStatAnalyst::getMin()
//data				array containing data to be analyzed
//samples			number of elements in input array
//
//Determines minimum value of input data array.
float MiniStatAnalyst::getMin(const float data[], uint16_t samples)
{
	//assumes first value is the minimum, then checks if any subsequent value
	//in the array is smaller than the first
	float min = data[0];
	
	for(uint16_t i = 1; i < samples; i++)
	{
		if(data[i] < min) min = data[i];
	}
	
	return min;
}


//float MiniStatAnalyst::getPeaktoPeak()
//data				array containing data to be analyzed
//samples			number of elements in input array
//
//Calculates peak-to-peak value of AC signal in the "data" array.
float MiniStatAnalyst::getPeaktoPeak(const float data[], uint16_t samples)
{
	return getMax(data,samples) - getMin(data,samples);
}


//float MiniStatAnalyst::getPeaktoPeak()
//data				array containing data to be analyzed
//samples			number of elements in input array
//max				maximum value of input data array
//min				minimum value of input data array
//
//Calculates peak-to-peak value of AC signal in the "data" array.
float MiniStatAnalyst::getPeaktoPeak(const float data[], uint16_t samples,
									 float &max, float &min)
{
	//assumes first value is the min/max, then checks if any subsequent value
	//in the array is smaller/bigger than the first
	max = data[0];
	min = data[0];
	
	for(uint16_t i = 1; i < samples; i++)
	{
		if(data[i] > max) max = data[i];
		if(data[i] < min) min = data[i];
	}
	
	return max-min;
}


//float MiniStatAnalyst::getAverage(const float data[], uint16_t samples)
//data					array containing data to average
//samples				number of samples in input array
//
//Returns average value of input array
float MiniStatAnalyst::getAverage(const float data[], uint16_t samples)
{
	float sum = 0;
	
	for(uint16_t i = 0; i < samples; i++)
	{
		sum += data[i];
	}
	
	return sum/samples;
}


double MiniStatAnalyst::getZeroCrossing(const float data[], const unsigned long time[],
										const float avg, const uint16_t samples)
{
	for(uint16_t i = 0; i < samples-1; i++)
	{
		if( data[i+1]-avg == 0 ) return time[i+1];
		else if( data[i]-avg == 0 ) return time[i];
		else if( (data[i+1]-avg)/(data[i]-avg) < 0)
		{
			SerialUSB.println();
			SerialUSB.println();
			SerialUSB.print(i);
			SerialUSB.println();
			SerialUSB.println();
			
			double slope = (data[i+1] - data[i]) / (time[i+1] - time[i]);
			double intercept = (data[i]-avg)-slope*time[i];
			return  (0-intercept)/slope;
		}
	}
	
	return 0;
}



//void MiniStatAnalyst::getWaveformStatistics(const float data[], uint16_t samples,
//											 float &max, float &min, float &pkpk,
//											 float &avg)
//{
//	float sum = 0;
//	
//	for(uint16_t i = 0; i < samples; i++)
//	{
//		sum += data[i];
//	}
//	
//	avg = sum/samples;
//	pkpk = max-min;
//}



//float MiniStatAnalyst::calcCapacitance(float imgImp, float freq)
//{
//	return 1/(2*PI*freq*abs(imgImp));
//}



/*
 HELPFUL LINKS
 
 polar and rectangular coordinates
 https://www.intmath.com/complex-numbers/convert-polar-rectangular-interactive.php
 https://www.mathsisfun.com/polar-cartesian-coordinates.html
 
 EIS
 https://www.gamry.com/application-notes/EIS/basics-of-electrochemical-impedance-spectroscopy/
 
 Calculate phase angle from time delay
 http://www.sengpielaudio.com/calculator-timedelayphase.htm
 
 faster ADC on SAMD21
 https://forum.arduino.cc/index.php?topic=338640.0
 https://forum.arduino.cc/index.php?topic=341345.0
 Google Search "analog read low level samd21"
 
 
 */






////This seems useful to know https://en.wikipedia.org/wiki/Linear_least_squares#Example,
////although this is really the guide this method is based on https://youtu.be/1C3olrs1CUw
////void MiniStatAnalyst::calcTangentLine(Queue<float> current_copy,
////									  Queue<float> voltage_copy,
////									  float &slope, float &intercept)
//void MiniStatAnalyst::calcTangentLine(const float current, const float voltage,
//									  float &slope, float &intercept)
//{
//	float samples = 0;
//	float volt = 0;
//	float amp = 0;
//
//	float xy_sum = 0;
//	float x_sum = 0;
//	float y_sum = 0;
//	float x_squared_sum = 0;
//
//	while(abs(volt) < 200 && !voltage_copy.isEmpty() && !current_copy.isEmpty())
//	{
//		volt = voltage_copy.front();
//		amp = current_copy.front();
//
//		xy_sum += volt*amp;
//		x_sum += volt;
//		y_sum += amp;
//		x_squared_sum += volt*volt;
//
//		voltage_copy.dequeue();
//		current_copy.dequeue();
//
//		samples++;
//
//		SerialUSB.print("I'm in the calcTangentLine Method");
//		SerialUSB.println(volt);
//	}
//
//
//	slope = (xy_sum - (x_sum*y_sum/samples))/(x_squared_sum - (x_sum*x_sum/samples));
//	intercept = y_sum/samples - slope*x_sum/samples;
//}
//
//
////float MiniStatAnalyst::getPeakCurrent(Queue<float> current_copy,
////									   Queue<float> voltage_copy,
////									   float slope, float intercept)
//float MiniStatAnalyst::getPeakCurrent(float current, float voltage,
//									   float slope, float intercept)
//{
//	float volt = voltage_copy.front();
//	float amp = current_copy.front();
//
//	float peak = amp - slope*volt-intercept;
//	voltage_copy.dequeue();
//	current_copy.dequeue();
//
//
//	while(abs(volt) < 400 && !voltage_copy.isEmpty() && !current_copy.isEmpty())
//	{
//		volt = voltage_copy.front();
//		amp = current_copy.front();
//
//		if(abs(peak) < abs(amp - slope*volt-intercept))
//		{
//			peak = amp - slope*volt-intercept;
//
//			SerialUSB.print("volt: ");
//			SerialUSB.println(volt);
//		}
//
//		voltage_copy.dequeue();
//		current_copy.dequeue();
//	}
//
//
//	return peak;
//}
