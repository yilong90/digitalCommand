/*********************************************************************
 * Desktop Station Main Analog Input
 *
 * Copyright (C) 2014 Yaasan
 *
 */


#ifndef ANALOGINLib_H
#define ANALOGINLib_H

#include "Arduino.h"

class DSMAnalogIn
{
	private:
		//Analog input paramters
		word gLPFBuf;
	
	public:
                DSMAnalogIn();
		void begin();
		word GetSpeedInputPulse();
		byte GetSpeedDirection();
		word GetSpeedInputAnalog();
		word LPF(word inValue, word inGain, word *iopBuf);
	
};

#endif
