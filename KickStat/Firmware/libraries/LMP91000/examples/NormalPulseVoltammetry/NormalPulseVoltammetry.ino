#include <LMP91000.h>
#include <Wire.h>

LMP91000 pStat = LMP91000();

int16_t opVolt = 3300; //milliVolts if working with a 3.3V device
uint8_t resolution = 10; //10-bits

void setup()
{
  Wire.begin();
  Serial.begin(115200);

  pStat.standby();
  delay(1000); //warm-up time for the gas sensor
  
  runNPV(1,200,50,0,500,4000,6);
}


void loop()
{
}


void runNPV(uint8_t user_gain, uint32_t pulse_period, uint32_t pulse_width,
            int16_t startV, int16_t endV, uint32_t quietTime, uint8_t range)
{
  pStat.disableFET();
  pStat.setGain(user_gain);
  pStat.setRLoad(0);
  pStat.setIntRefSource();
  pStat.setIntZ(1);
  pStat.setThreeLead();
  pStat.setBias(0);
  pStat.setPosBias();
  delay(quietTime);

  //Print column headers
  String current = "";
  if(range == 12) current = "Current(pA)";
  else if(range == 9) current = "Current(nA)";
  else if(range == 6) current = "Current(uA)";
  else if(range == 3) current = "Current(mA)";
  else current = "SOME ERROR";
  
  Serial.println("Voltage(mV)," + current);
 
  uint32_t off_time = pulse_period - pulse_width;

  startV = determineLMP91000Bias(startV);
  startV =  opVolt*TIA_BIAS[abs(startV)]*startV/abs(startV);
  endV = determineLMP91000Bias(endV);
  endV =  opVolt*TIA_BIAS[abs(endV)]*endV/abs(endV);


  //this divides the voltage span for the scan
  //by voltage resolution of the LMP91000
  //referenced to the chip's operating voltage
  uint8_t pulses = abs((endV-startV)/(opVolt*0.02));
  int16_t incrV = (endV-startV)/pulses;

  startV = determineLMP91000Bias(startV);
  unsigned long startTime = 0;
  signed char sign = 0;
  int16_t nextV = 0;
  
  for (uint8_t i = 0; i < pulses; i++)
  {
    //baseline voltage
    startTime = millis();
    if(startV < 0) sign = 0;
    else sign = 1;
    pStat.setBias(abs(startV), sign);
    while(millis() - startTime < off_time);


    //find next voltage potential
    if(startV != 0) nextV = (opVolt*TIA_BIAS[abs(startV)]*startV/abs(startV)) + i*incrV;
    else nextV +=  i*incrV;
    nextV = determineLMP91000Bias(nextV);


    //start next pulse
    startTime = millis();
    if(nextV < 0) sign = 0;
    else sign = 1;
    pStat.setBias(abs(nextV), sign);
    while(millis() - startTime < pulse_width);


    //print output current
    Serial.print((uint16_t)(opVolt*TIA_BIAS[abs(nextV)]*(nextV/abs(nextV))));
    Serial.print(",");
    Serial.println(pow(10,range)*pStat.getCurrent(pStat.getOutput(A0), opVolt/1000.0, resolution));
  }

  //end at 0V
  pStat.setBias(0);
}


signed char determineLMP91000Bias(int16_t voltage)
{
  signed char polarity = 0;
  if(voltage < 0) polarity = -1;
  else polarity = 1;
  
  int16_t v1 = 0;
  int16_t v2 = 0;
  
  voltage = abs(voltage);

  if(voltage == 0) return 0;
  
  for(int i = 0; i < NUM_TIA_BIAS-1; i++)
  {
    v1 = opVolt*TIA_BIAS[i];
    v2 = opVolt*TIA_BIAS[i+1];

    if(voltage == v1) return polarity*i;
    else if(voltage > v1 && voltage < v2)
    {
      if(abs(voltage-v1) < abs(voltage-v2)) return polarity*i;
      else return polarity*(i+1);
    }
  }
  return 0;
}
