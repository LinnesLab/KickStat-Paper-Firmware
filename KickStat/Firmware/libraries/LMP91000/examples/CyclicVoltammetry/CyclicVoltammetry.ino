#include <LMP91000.h>
#include <Wire.h>

LMP91000 pstat = LMP91000();

int rate = 200;
int settling_time = 50;

void setup()
{
  Wire.begin();
  Serial.begin(9600);

  delay(50);
  pstat.standby();
  delay(50);
  pstat.disableFET();
  pstat.setGain(1);
  pstat.setRLoad(0);
  pstat.setExtRefSource();
  pstat.setIntZ(1);
  pstat.setThreeLead();
  delay(2000); //warm-up time for the gas sensor

  for (int j = 0; j < 3; j++)
  {
    pstat.setNegBias();
    for (int i = 1; i < 11; i++)
    {
      pstat.setBias(i);
      //delay(50);
      delay(settling_time);
      Serial.print(i*-1);
      Serial.print(",");
      delay(1);
      Serial.println(analogRead(A0));
      delay(rate);
    }
    for (int i = 10; i >= 0; i--)
    {
      pstat.setBias(i);
      //delay(50);
      delay(settling_time);
      Serial.print(i*-1);
      Serial.print(",");
      delay(1);
      Serial.println(analogRead(A0));
      delay(rate);
    }
    pstat.setPosBias();
    for (int i = 1; i < 11; i++)
    {
      pstat.setBias(i);
      //delay(50);
      delay(settling_time);
      Serial.print(i*1);
      Serial.print(",");
      delay(1);
      Serial.println(analogRead(A0));
      delay(rate);
    }
    for (int i = 10; i >= 0; i--)
    {
      pstat.setBias(i);
      //delay(50);
      delay(settling_time);
      Serial.print(i*1);
      Serial.print(",");
      delay(1);
      Serial.println(analogRead(A0));
      delay(rate);
    }
  }
  
  pstat.setBias(0);

}

void loop() {
  // put your main code here, to run repeatedly:

}
