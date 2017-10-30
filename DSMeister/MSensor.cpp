/* DSmeister software
 * Sensor interface
 *
 * Created: 2015/09/06
 *  Author: Yaasan
 */ 

#include "MSensor.h"



MSensor::MSensor(int inPin_current, int inPin_voltage)
{
	
	pinCurrent = inPin_current;
	pinVoltage = inPin_voltage;
	
	
	pinMode(pinCurrent, INPUT);
	pinMode(pinVoltage, INPUT);
	
	gOffset_Current = 0;
	
	gSensor_Voltage = 0;		/**< 1V unit(0V to 255V)   */
	gSensor_Current = 0;		/**< 0.1A unit(0.0A to 25.5A)   */
	
	gMonitoringCurrent = 512;
	
	//Initial update
	Update(OC_LEVEL);
}

unsigned char MSensor::GetCurrent()
{
	return gSensor_Current;
}


unsigned char MSensor::GetVoltage()
{
	return gSensor_Voltage;
}

word MSensor::GetCurrentOffset()
{
	if( gOffset_Current == 0)
	{
		return 512;
	}
	else
	{
		return gOffset_Current;
	}
}

void MSensor::CalcCurrentOffset()
{
	unsigned short aCurrentSum = 0;
	
	if( gOffset_Current == 0)
	{
		/* 直流中間に電圧が印加されてないと、実際の動作時のADCのオフセット誤差が異なる */
		if( analogRead(pinVoltage) >= LV_LEVEL)
		{
			
			for( int i = 0; i < 16; i++)
			{
				aCurrentSum = aCurrentSum + analogRead(pinCurrent);
				delay(1);
			}
			
			//平均値を算出
			gOffset_Current = aCurrentSum >> 4;
		}
	}	
}

void MSensor::MonitorCurrent(void)
{
	word aCurrent = analogRead(pinCurrent);
	
	/* LPF ２倍周期(100msなら200ms LPF) */
	gMonitoringCurrent = ((gMonitoringCurrent * 3) + aCurrent) >> 2;
}

unsigned char  MSensor::Update(unsigned short inOCLevel)
{

	unsigned char aErrorFlag = 0;
	
	
	//電圧換算
	// 1.2k / 8k  y=((x*8*5)/1.2/1024)
	unsigned short aVolt = analogRead(pinVoltage);
	
	/* OV check */
	if( aVolt > OV_LEVEL)
	{
		aErrorFlag |= 0b01;
	}
	
	/* LV check */
	if( aVolt < LV_LEVEL)
	{
		aErrorFlag |= 0b10;
	}
		

	//Limit
	if( aVolt > 922)
	{
		//30Vで打ち止め
		aVolt = 922;
	}
	
	aVolt = aVolt >> 1;//512に分解能を落とす
	
	aVolt = ((aVolt * 126) / 3) >> 6;
	
	if( aVolt > 255)
	{
		//UCに入るサイズにリミット(25.5V)
		aVolt = 255;
	}
	
	gSensor_Voltage = (unsigned char)aVolt;
	
	//電流換算 1d = 0.025A
	
	unsigned short aCurrent = 0;
	
	aCurrent = gMonitoringCurrent;
	
	//絶対値化
	int aOffset_current = GetCurrentOffset();
	
	if( aCurrent >= aOffset_current)
	{
		aCurrent = aCurrent - aOffset_current;
		
	}
	else
	{
		aCurrent = aOffset_current - aCurrent;
	}
	
	
	//Limit
	if( aCurrent > 409)
	{
		//20Aで打ち止め
		aCurrent = 409;
	}
	
	// 2.08V = 12.5A,  426.7d
	// y = (x-512)*125/426.66666
	aCurrent = (aCurrent * 125) / 427;
	// (** DSmainR5の場合はこちらを上の代わりに使用 **)aCurrent = (aCurrent * 125) >> 8;
	
	if( aCurrent > 255)
	{
		//UCに入るサイズにリミット(25.5A)
		aCurrent = 255;
	}
	
	/* OC check */
	if( aCurrent > inOCLevel)
	{
		aErrorFlag |= 0b100;
	}
	
	gSensor_Current = (unsigned char)aCurrent;
	
	
	return aErrorFlag;
	
}