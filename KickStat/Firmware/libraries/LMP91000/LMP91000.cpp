/*
 FILENAME:    LMP91000.h
 AUTHOR:      Orlando S. Hoilett
 EMAIL:       ohoilett@purdue.edu
 VERSION:     1.0.0
 
 
 DESCRIPTION
 
 
 
 A FEW INSTRUCTIONS
 * All methods are defined and coded according to the instructions given in the
 * LMP91000 datsheet, December 2014 Revision, from Texas Instruments. All
 * references to the "datasheet" refer to this specific revision. The datasheet
 * is referenced in the code so that the user can have further consult if he/she
 * needs more information. A copy of the datasheet is included in the software
 * download.
 *
 * All references to "the device" refer to the LMP91000 Sensor AFE System:
 * Configurable AFE Potentiostat for Low-Power Chemical-Sensing Applications
 * Impedance Analyzer from Texas Instruments.
 *
 * TIA - Transimpedance Amplifier
 * TIACN - Transimpedance Amplifier Control Register (0x10)
 * REFCN - Reference Control Register (0x11)
 
 
 * UPDATES
 * Version 0.0
 * 2015/09/18:1200>
 *			Initialization of code development.
 * 2015/10/12:1010>
 *          Testing methods.
 * 2015/10/12:1041>
 *          Noticed that objects cannot be instantiated in the "setup()" method.
 *          No idea why that is.
 
 * SOURCES
 * Some code snippets were taken from
 * vicatcu. "LMP91000." Authored: Oct 27, 2014. Accessed:
 *      September 18, 2015. GitHub. <https://github.com/WickedDevice/LMP91000>
 * jorgenro1. "lmp91000." Authord: Jan 26, 2015. Acccessed:
 *      September 18, 2015. GitHub. <https://github.com/dgnest/lmp91000>
 
 * A couple of other useful links from TI's forum
 * https://e2e.ti.com/support/interface/etc_interface/f/146/t/258263
 * https://e2e.ti.com/support/amplifiers/precision_amplifiers/f/14/t/189399
 * https://e2e.ti.com/support/interface/etc_interface/f/146/t/195448
 * https://e2e.ti.com/support/amplifiers/precision_amplifiers/f/14/t/317192
 
 
 * DISCLAIMER
 Copyright (c) 2016 Linnes Lab, Purdue University, West Lafayette, IN, USA
 
 */



#include "LMP91000.h"


/************CONSTRUCTORS*****************/

//DEFAULT CONSTRUCTOR
//Initializes object
LMP91000::LMP91000()
{
}


//void setMENB(uint8_t pin)
//Sets the MENB I/O pin and initializes pin with "pinMode" function.
void LMP91000::setMENB(uint8_t pin)
{
    MENB = pin;
    pinMode(MENB, OUTPUT);
}


//uint8_t LMP91000::getMENB() const
//Returns the I/O pin for controlling the MENB (module enable).
uint8_t LMP91000::getMENB() const
{
    return MENB;
}


//void write(uint8_t reg, uint8_t data) const
//@param        reg: register to write to
//@param		data: data that will be written to register
//
//First ensures the device is enabled for I2C commands using the "enable()"
//method. Writes to the LMP91000 I2C device using the I2C protocol for Arduino
//https://www.arduino.cc/en/Reference/WireWrite
//Please consult page 20, Section 7.5.1 I2C Interface and 7.5.2 Write and Read
//Operation in the datsheet for more information.
void LMP91000::write(uint8_t reg, uint8_t data) const
{
    enable();
    Wire.beginTransmission(LMP91000_I2C_ADDRESS);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission();
}


//uint8_t read(uint8_t reg) const
//@param        reg: register to read from
//
//First ensures the device is enabled for I2C commands using the "enable()"
//method. Reads from the LMP91000 I2C device using the I2C protocol for Arduino.
//https://www.arduino.cc/en/Reference/WireRead
//
//Please consult page 20, "Section 7.5.1 I2C Interface" and "7.5.2 Write and Read
//Operation" in the datsheet for more information.
//
//The device has must be written to first before a read operation can be performed.
uint8_t LMP91000::read(uint8_t reg) const
{
    uint8_t data = 0;
    
    enable();
    
    Wire.beginTransmission(LMP91000_I2C_ADDRESS);
    Wire.write(reg);
    Wire.endTransmission(false);
    
    Wire.requestFrom(LMP91000_I2C_ADDRESS, 0x01);
    while(Wire.available()){
        data = Wire.read();
    }
    
    return data;
}


//void LMP91000::enable() const
//ENABLES the LMP91000 for I2C operations. Please consult page 3, "Section 5 Pin
//Configurations and Functions" for more information.
//
//The device is active low.
void LMP91000::enable() const
{
    digitalWrite(MENB, LOW);
}


//void LMP91000::disable() const
//DISABLES the LMP91000 for I2C operations. Please consult page 3, "Section 5
//Pin Configurations and Functions" for more information.
//
//The device is active low.
void LMP91000::disable() const
{
    digitalWrite(MENB, HIGH);
}


//boolean isReady() const
//@return       whether or not the device is ready.
//
//Reads the status register (0x00) of the LMP91000 to determine whether or not
//the device is ready to accept I2C commands.
//
//Please consult page 21, "Section 7.6.1 STATUS -- Status Register (Address
//0x00)" of the datasheet for more information.
//
//Default state is not ready.
boolean LMP91000::isReady() const
{
    return read(LMP91000_STATUS_REG)==LMP91000_READY;
}


//boolean isLocked() const
//@return       whether or not the TIACN and REFCN is locked for writing
//
////Reads the lock register (0x01) of the LMP91000 to determine whether or not
//the TIACN and REFCN are "write-enabled" or "read-only."
//
//Please consult pages 21 and 22, "Section 7.6.2 LOCK -- Protection Register
//(Address 0x01)" for more information.
//
//Deafult state is "read-only" mode.
boolean LMP91000::isLocked() const
{
    return bitRead(read(LMP91000_LOCK_REG),0)==LMP91000_WRITE_LOCK;
}

//from vicatcu
//void LMP91000::lock() const
//
//Writes to the lock register (0x01) of the LMP9100 to set the TIACN and REFCN
//registers to "read-only."
//
//Please consult pages 21 and 22, "Section 7.6.2 LOCK -- Protection Register
//(Address 0x01)" for more information.
//
//Default state is "read-only" mode.
void LMP91000::lock() const
{
    write(LMP91000_LOCK_REG, LMP91000_WRITE_LOCK);
}

//from vicatcu
//void LMP91000::unlock() const
//
//Writes to the lock register (0x01) of the LMP9100 to set the TIACN and REFCN
//registers to "write" mode.
//
//Please consult pages 21 and 22, "Section 7.6.2 LOCK -- Protection Register
//(Address 0x01)" for more information.
//
//Default state is "read-only" mode.
void LMP91000::unlock() const
{
    write(LMP91000_LOCK_REG, LMP91000_WRITE_UNLOCK);
}


//void LMP91000::setGain(uint8_t gain) const
//@param            gain: the gain to be set to
//
//param - value - gain resistor
//0 - 000 - External resistor
//1 - 001 - 2.75 kOhm
//2 - 010 - 3.5 kOhm
//3 - 011 - 7 kOhm
//4 - 100 - 14 kOhm
//5 - 101 - 35 kOhm
//6 - 110 - 120 kOhm
//7 - 111 - 350 kOhm
//
//Sets the transimpedance amplifier gain. First reads the register to ensure
//that the other bits are not affected. The 3 LSBs of "gain" parameter is
//written to the 2nd, 3rd, and 4th bit of the TIACN register.
//
//Please consult page 14 "7.3.1.1 Transimpedance Amplifier" and page 22 "Section
//7.6.3 TIACN -- TIA Control Register (Address 0x10)" of the datasheet for more
//information.
void LMP91000::setGain(uint8_t user_gain)
{
    gain = user_gain;
    
    unlock();
    uint8_t data = read(LMP91000_TIACN_REG);
    data &= ~(7 << 2); //clears bits 2-4
    data |= (user_gain << 2); //writes to bits 2-4
    write(LMP91000_TIACN_REG, data);
}


double LMP91000::getGain() const
{
    if (gain == 0) return gain;
    else return TIA_GAIN[gain];
}

//void LMP91000::setRLoad(uint8_t load) const
//@param            load: the internal load resistor to select
//
//param - value - RLoad
//0 - 00 - 10 Ohm
//1 - 01 - 33 Ohm
//2 - 10 - 50 Ohm
//3 - 11 - 100 Ohm
//
//Sets the internal RLOAD selection resistor. First reads the register to ensure
//that the other bits are not affected. The 2 LSBs of "load" parameter is
//written to the 0th and 1st bit of the TIACN register.
//
//Please consult page 14 "7.3.1.1 Transimpedance Amplifier" and page 22 "Section
//7.6.3 TIACN -- TIA Control Register (Address 0x10)" of the datasheet for more
//information.
void LMP91000::setRLoad(uint8_t load) const
{
    unlock();
    uint8_t data = read(LMP91000_TIACN_REG);
    data &= ~3; //clears 0th and 1st bits
    data |= load; //writes to 0th and 1st bits
    write(LMP91000_TIACN_REG, data);
}

//void LMP91000::setRefSource(uint8_t source) const
//@param            source: external vs. internal
//
//param - result
//0 - internal reference
//1 - external reference
//
//Sets the voltage reference source of the LMP91000 to an internal reference or
//an external reference.
//
//Please consult page 22, "Section 7.6.4 REFCN -- Reference Control Register
//(Address 0x11)" of the datasheet for more information.
void LMP91000::setRefSource(uint8_t source) const
{
    if (source == 0) setIntRefSource();
    else setExtRefSource();

}


//void LMP91000::setIntRefSource() const
//
//Unlocks the REFCN register for "write" mode. First reads the register to
//ensure that the other bits are not affected. Writes a "0" to the 7th bit of
//the REFCN register.
//
//Sets the voltage reference source to supply voltage (Vdd).
//
//Please consult page 22, "Section 7.6.4 REFCN -- Reference Control Register
//(Address 0x11)" of the datasheet for more information.
void LMP91000::setIntRefSource() const
{
    unlock(); //unlocks the REFCN register for "write" mode
    uint8_t data = read(LMP91000_REFCN_REG);
    data &= ~(1 << 7); //clears the 7th bit
    write(LMP91000_REFCN_REG, data);
}


//void LMP91000::setExtRefSource() const
//
//Unlocks the REFCN register for "write" mode. First reads the register to
//ensure that the other bits are not affected. Writes a "1" to the 7th bit of
//the REFCN register.
//
//Sets the reference source of the LMP91000 to an external reference provided at
//the Vref pin.
//
//Please consult page 22, "Section 7.6.4 REFCN -- Reference Control Register
//(Address 0x11)" of the datasheet for more information.
void LMP91000::setExtRefSource() const
{
    unlock(); //unlocks the REFCN register for "write" mode
    uint8_t data = read(LMP91000_REFCN_REG);
    data |= (1 << 7); //writes a "1" to the 7th bit
    write(LMP91000_REFCN_REG, data);
}


//void LMP91000::setIntZ(uint8_t intZ) const
//@param            intZ: the internal zero selection
//
//param - value - result
//0 - 00 - 20%
//1 - 01 - 50%
//2 - 10 - 67%
//3 - 11 - bypassed
//
//Unlocks the REFCN register for "write" mode. First reads the register to
//ensure that the other bits are not affected. Writes to the 5th and 6th bits
//of the REFCN register.
//
//Sets the internal zero of the device, particularly the transimpedance
//amplifier.
//
//Please consult page 22, "Section 7.6.4 REFCN -- Reference Control Register
//(Address 0x11)" of the datasheet for more information.
void LMP91000::setIntZ(uint8_t intZ)
{
    zero = intZ;
    
    unlock(); //unlocks the REFCN register for "write" mode
    uint8_t data = read(LMP91000_REFCN_REG);
    data &= ~(3 << 5);
    data |= (intZ << 5);
    write(LMP91000_REFCN_REG, data);
}

double LMP91000::getIntZ() const
{
    return TIA_ZERO[zero];
}


//void LMP91000::setBiasSign(uint8_t sign) const
//0 = negative
//1 = positive
void LMP91000::setBiasSign(uint8_t sign) const
{
    if (sign == 0) setNegBias();
    else setPosBias();
}

//void LMP91000::setNegBias() const
void LMP91000::setNegBias() const
{
    unlock();
    uint8_t data = read(LMP91000_REFCN_REG);
    data &= ~(1 << 4); //clear bit
    write(LMP91000_REFCN_REG, data);
}

//void LMP91000::setPosBias() const
void LMP91000::setPosBias() const
{
    unlock();
    uint8_t data = read(LMP91000_REFCN_REG);
    data |= (1 << 4);
    write(LMP91000_REFCN_REG, data);
}

//void LMP91000::setBias(uint8_t bias) const
void LMP91000::setBias(uint8_t bias) const
{
    unlock();
    uint8_t data = read(LMP91000_REFCN_REG);
    data &= ~(0x0F); //clear the first four bits so I can bit Or in the next step
    data |= bias;
    write(LMP91000_REFCN_REG, data);
}


//void LMP91000::setBias(uint8_t bias) const
//sign				0 is negative and 1 is positive
//
void LMP91000::setBias(uint8_t bias, signed char sign) const
{
	if(sign > 0) sign = 1;
	else sign = 0;
	sign = (uint8_t)sign;
	
	if(bias > 13) bias = 0;
	
	
	unlock();
	uint8_t data = read(LMP91000_REFCN_REG);
	data &= ~(0x1F); //clear the first five bits so I can bit Or in the next step
	data |= bias;
	data |= ((sign << 4) | bias);
	write(LMP91000_REFCN_REG, data);
}


//void LMP91000::setFET(uint8_t selection) const
void LMP91000::setFET(uint8_t selection) const
{
    if (selection == 0) disableFET();
    else enableFET();
}

//void LMP91000::disableFET() const
void LMP91000::disableFET() const
{
    uint8_t data = read(LMP91000_MODECN_REG);
    data &= ~(1 << 7);
    write(LMP91000_MODECN_REG, data);
}

//void LMP91000::enableFET() const
void LMP91000::enableFET() const
{
    uint8_t data = read(LMP91000_MODECN_REG);
    data |= (1 << 7);
    write(LMP91000_MODECN_REG, data);
}

//void LMP91000::setMode(uint8_t mode) const
void LMP91000::setMode(uint8_t mode) const
{
    if (mode == 0) sleep();
    else if (mode == 1) setTwoLead();
    else if (mode == 2) standby();
    else if (mode == 3) setThreeLead();
    else if (mode == 4) measureCell();
    else if (mode == 5) getTemp();
	else {}; //some error
}

//void LMP91000::sleep const
//
//Sets the 3 LSBs of the Mode Control Register (0x12) to 0.
//
//Places the LMP91000 in deep sleep state for power conservation. The LMP91000
//consumes 0.6 uA of current in deep sleep mode.
//
//Please see page 19 Section, 7.4 Device Functional Modes and page 23, Section
//7.6.5 MODECN -- Mode Control Register (Address 0x12) of the datasheet for more
//information.
void LMP91000::sleep() const
{
    uint8_t data = read(LMP91000_MODECN_REG);
    data &= ~(0x07);
    write(LMP91000_MODECN_REG, data);
}


//void LMP91000::setTwoLead() const
//Sets the first three bits of the Mode Control Register to 001. This enables
//the LMP91000 for 2-electrode potentiometric measurements.
void LMP91000::setTwoLead() const
{
    uint8_t data = read(LMP91000_MODECN_REG);
    data &= ~(0x07);
    data |= (0x01);
    write(LMP91000_MODECN_REG, data);
}


//void LMP91000::standby() const
//
//Sets the 3 LSBs of the Mode Control Register (0x12) to 010.
//
//Places the device in standby() mode which allows quick warm-up in between tests
//
//Please see page 19-20, Section 7.4 Device Functional Modes and page 23 Section
//7.6.5 MODECN -- Mode Control Register (Address 0x12) of the datasheet for
//more information.
void LMP91000::standby() const
{
    uint8_t data = read(LMP91000_MODECN_REG);
    data &= ~(0x07);
    data |= (0x02);
    write(LMP91000_MODECN_REG, data);
}

//void LMP91000::setThreeLead() const
//Sets the first three bits of the Mode Control Register to 011. This enables
//the LMP91000 for 3-electrode potentiometric measurements.
void LMP91000::setThreeLead() const
{
    uint8_t data = read(LMP91000_MODECN_REG);
    data &= ~(0x07);
    data |= (0x03);
    write(LMP91000_MODECN_REG, data);
}

//void LMP91000::measureCell() const
//
void LMP91000::measureCell() const
{
    uint8_t data = read(LMP91000_MODECN_REG);
    data &= ~(0x07); //clears the first three bits
    data |= (0x06);
    write(LMP91000_MODECN_REG, data);
}

//void LMP91000::getTemp() const
void LMP91000::getTemp() const
{
    uint8_t data = read(LMP91000_MODECN_REG);
    data |= (0x07);
    write(LMP91000_MODECN_REG, data);
}


//double LMP91000::getTemp(uint8_t sensor, double adc_ref, uint8_t adc_bits) const
//returns               temperatue in degrees Celsius
//
//Measures temperature by setting bits 0, 1, and 2 of the Mode Control Register
//to 1. This sets the transimpedance amplifier of the LMP91000 ON and sends
//the output of the internal temperature sensor to the VOUT pin of the LMP91000.
double LMP91000::getTemp(uint8_t sensor, double adc_ref, uint8_t adc_bits) const
{
    uint8_t data = read(LMP91000_MODECN_REG);
    data |= (0x07);
    write(LMP91000_MODECN_REG, data);
    
    delay(100);
    
    return (getVoltage(sensor, adc_ref, adc_bits)-TEMP_INTERCEPT)/TEMPSLOPE;
}


//uint16_t MiniStat::getOutput(uint8_t sensor) const
//
//@param            sensor: the analog in pin of the LMP91000 is connected to
//
//@return           the voltage output of the LMP91000 in bits
//
//Uses analogRead() return the output of the LMP91000.
uint16_t LMP91000::getOutput(uint8_t sensor) const
{
    return analogRead(sensor);
}


//double MiniStat::getVoltage(uint8_t sensor, double adc_ref, uint8_t adc_bits) const
//{
//    return (analogRead(sensor)*adc_ref)/(pow(10,adc_bits)-1);
//}
//
//
//double MiniStat::getCurrent(uint8_t sensor, double adc_ref, uint8_t adc_bits) const
//{
//    return (getVoltage(sensor, adc_ref, adc_bits) - (adc_ref/TIA_ZERO[zero]))/TIA_GAIN[gain-1];
//}
//
//
//
//double MiniStat::getCurrent(uint8_t sensor, double adc_ref, uint8_t adc_bits, double extGain) const
//{
//    return (getVoltage(sensor, adc_ref, adc_bits) - (adc_ref/TIA_ZERO[zero]))/extGain;
//}



//double MiniStat::getVoltage(uint16_t adcVal, double adc_ref, uint8_t adc_bits) const
//
//@param            adcVal: value returned by the analog-to-digital converter of
//                          the microcontroller used to control the LMP91000
//
//@param            adc_ref: voltage reference of the analog-to-digtal converter
//                          of the microcontroller
//
//@param            adc_bits: number of bits of the analog-to-digital converter
//                          of the microcontroller
//
//@return           the voltage output of the LMP91000
//
//This method calculates the voltage at the output of the LMP91000 by multiplying
//by the refernece voltage of the analog-to-digital converter and dividing by
//the bit resolution of the analog-to-digital converter.
double LMP91000::getVoltage(uint16_t adcVal, double adc_ref, uint8_t adc_bits) const
{
    return (adcVal*adc_ref)/(pow(2,adc_bits)-1);
}
 

//double MiniStat::getCurrent(uint16_t adcVal, double adc_ref, uint8_t adc_bits) const
//
//@param            adcVal: value returned by the analog-to-digital converter of
//                          the microcontroller used to control the LMP91000
//
//@param            adc_ref: voltage reference of the analog-to-digtal converter
//                          of the microcontroller
//
//@param            adc_bits: number of bits of the analog-to-digital converter
//                          of the microcontroller
//
//@return           the current at the working electrode
//
//This method calculates the current at the working electrode by reading in the
//voltage at the output of LMP91000 and dividing by the value of the gain resistor.
double LMP91000::getCurrent(uint16_t adcVal, double adc_ref, uint8_t adc_bits) const
{
    return (getVoltage(adcVal, adc_ref, adc_bits) - (adc_ref*TIA_ZERO[zero]))/TIA_GAIN[gain-1];
}


//double MiniStat::getCurrent(uint16_t adcVal, double adc_ref, uint8_t adc_bits,
//                              double extGain) const
//
//@param            adcVal: value returned by the analog-to-digital converter of
//                          the microcontroller used to control the LMP91000
//
//@param            adc_ref: voltage reference of the analog-to-digtal converter
//                          of the microcontroller
//
//@param            adc_bits: number of bits of the analog-to-digital converter
//                          of the microcontroller
//
//@param            extGain: value of external gain resistor
//
//@return           the current at the working electrode
//
//This method calculates the current at the working electrode by reading in the
//voltage at the output of LMP91000 and dividing by the value of the external
//gain resistor.
double LMP91000::getCurrent(uint16_t adcVal, double adc_ref, uint8_t adc_bits,
                            double extGain) const
{
    return (getVoltage(adcVal, adc_ref, adc_bits) - (adc_ref*TIA_ZERO[zero]))/extGain;
}




