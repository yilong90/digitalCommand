/*********************************************************************
 * Desktop Station Main Analog Input
 *
 * Copyright (C) 2014 Yaasan
 *
 */

#include "DSMAnalogIn.h"

DSMAnalogIn::DSMAnalogIn()
{
	gLPFBuf = 0;
}

void DSMAnalogIn::begin()
{
	/* Mascon I/F */
	pinMode(6, INPUT);
	pinMode(A1, INPUT);
	digitalWrite(A1, LOW); // disable internal pullup
	analogReference(DEFAULT); // set analog reference 5V(VCC) 
}


word DSMAnalogIn::GetSpeedInputPulse()
{
	unsigned long aSpeed;

	word aLowTime = pulseIn(A1, LOW, 500);
	word aHighTime = pulseIn(A1, HIGH, 500);

	if( (aHighTime == 0) && (aLowTime == 0))
	{
		/* Not PWM but Analog */
		aSpeed = (unsigned long)analogRead(A1);
	}
	else if(aHighTime == 0)
	{
		aSpeed = 0;
	}
	else if ( aLowTime == 0)
	{
		aSpeed = 1023;
	}
	else
	{ 
		aSpeed = 1023 * aHighTime / (aLowTime + aHighTime);
	}

	return LPF((word)aSpeed, 63, &gLPFBuf) & 0xFFF0;

}

byte DSMAnalogIn::GetSpeedDirection()
{

	word aLowTime = pulseIn(6, LOW, 500);
	word aHighTime = pulseIn(6, HIGH, 500);
	
	if((aHighTime == 0) && (aLowTime == 0))
	{
		/* FWD */
		return 2;
	}
	else
	{ 
		/* REV */
		return 1;
	}

}

word DSMAnalogIn::GetSpeedInputAnalog()
{
	return LPF((word)analogRead(A1), 63, &gLPFBuf) & 0xFFF0;
}

// inValue 0-255
// inGain 1-64
// 
word DSMAnalogIn::LPF(word inValue, word inGain, word *iopBuf) 
{ 
	word aOut; 

	aOut = (((*iopBuf) * inGain) + (inValue * (64 - inGain))) >> 6;
	*iopBuf = aOut;

	return aOut; 
}

