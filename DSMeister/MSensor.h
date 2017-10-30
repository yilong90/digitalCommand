/* DSmeister software
 * Sensor interface
 *
 * Created: 2015/09/06
 *  Author: Yaasan
 */ 

#ifndef MSENSOR_H
#define MSENSOR_H

#include <Arduino.h>

#define		LV_LEVEL			276		// 9V
#define		OC_LEVEL			154		// 7.5A
#define		OV_LEVEL			646		// 21V

class MSensor
{
  private:
	
	unsigned char gSensor_Voltage;		/**< 1V unit(0V to 255V)   */
	unsigned char gSensor_Current;		/**< 0.1A unit(0.0A to 25.5A)   */
	int pinCurrent;
	int pinVoltage;
	int pinTemperature;
	word gOffset_Current;
	word gMonitoringCurrent;
	
  public:
	
	MSensor(int inPin_current, int inPin_voltage);
	
	unsigned char  Update(unsigned short inOCLevel);
	unsigned char GetCurrent();
	unsigned char GetVoltage();
	word GetCurrentOffset();
	void CalcCurrentOffset();
	void MonitorCurrent(void);
};

#endif
