/*  
    
	E 1000000000001111, 1111000000001111
	8 1011000000001111, 1111000000001111
	7 1001000000001111, 1111000000001111
	6 0110000000001111, 1111000000001111
	5 0100000000001111, 1111000000001111
	4 0110000000001111, 1111000000001111
	3 1100000000001111, 1111000000001111
	2 1110000000001111, 1111000000001111
	1 0111000000001010, 1111000000001111
	N 0111000000001111, 1111000000001111
	1 1101100000001110, 1101000000001010
	2 1101100000001110, 1101000000001010
	3 1101000000001110, 1101000000001010
	4 1101100000001011, 1101000000001010, 
	5 1101000000001011, 1101000000001010

	       1101000000001010
	SELECT ........x.......
	START  ...........x....
	A      .......x
	B      ......x
	C      .....x
	MASK   zzzzz.......zzzz
    
    
*/

#include "DenshaMascon.h"


byte Mascon::Interval()
{
	int aSpeedRef;
	byte aRet = 0;
	
	
	aSpeedRef = gSpeedRef + (gPower * 8);
	
	//Limit
	if( aSpeedRef < 0)
	{
		aSpeedRef = 0;
	}
	else if ( aSpeedRef > 8191)
	{
		aSpeedRef = 8191;
	}
	
	if( (aSpeedRef >> 3) != (gSpeedRef >> 3))
	{
		aRet = 1;
	}
	
	gSpeedRef = aSpeedRef;
	
	return aRet;
	
}

word Mascon::GetSpeedValue()
{
	return gSpeedRef >> 3;
	
}

int Mascon::GetHandleValue()
{
	return gPower;
	
}

byte Mascon::GetButtonStatus(unsigned int inData)
{
	byte aRet = 0;
		
	if( (inData & BUTTON_E) > 0)
	{
		//Serial.print("SELECT,");
		aRet = aRet | 0b00001;
	}
	if( (inData & BUTTON_S) > 0)
	{
		//Serial.print("START,");  
		aRet = aRet | 0b00010;
	}
	if( (inData & BUTTON_A) > 0)
	{
		//Serial.print("A,");  
		aRet = aRet | 0b00100;
	}
	if( (inData & BUTTON_B) > 0)
	{
		//Serial.print("B,");  
		aRet = aRet | 0b01000;
	}
	if( (inData & BUTTON_C) > 0)
	{
		//Serial.print("C,");  
		aRet = aRet | 0b10000;
	}
	
	return aRet;

}


byte Mascon::GetHandleStatus(unsigned int inData)
{
	
	if( (inData & MASK_BTN)  == DEC_FLAG)
	{
		gDirection = 1;
	}
	else if( (inData & MASK_BTN)  == ACC_FLAG)
	{
		gDirection = 0;
	}
	
	
	if( (inData & MASK_BTN) == GEAR_EMG)
	{
		//Serial.print("EMG,");  
		gPower = -1024;
	}
	else if ( (inData & MASK_BTN) == GEAR_B8)
	{
		//Serial.print("B8,");  
		gPower = -8;
	}
	else if ( (inData & MASK_BTN) == GEAR_B7)
	{
		//Serial.print("B7,");  
		gPower = -7;
	}
	else if ( (inData & MASK_BTN) == GEAR_B6)
	{
		//Serial.print("B6,");  
		gPower = -6;
	}
	else if ( (inData & MASK_BTN) == GEAR_B5)
	{
		//Serial.print("B5,");  
		gPower = -5;
	}
	else if ( (inData & MASK_BTN) == GEAR_B4)
	{
		//Serial.print("B4,");  
		gPower = -4;
	}
	else if ( (inData & MASK_BTN) == GEAR_B3)
	{
		//Serial.print("B3,");  
		gPower = -3;
	}
	else if ( (inData & MASK_BTN) == GEAR_B2)
	{
		//Serial.print("B2,");  
		gPower = -2;
	}
	else if ( (inData & MASK_BTN) == GEAR_B1)
	{
		//Serial.print("B1,");  
		gPower = -1;
	}
	else if ( (inData & MASK_BTN) == GEAR_N)
	{
		//Serial.print("N,");  
		gPower = 0;
	}
	else if ( (inData & MASK_BTN) == GEAR_P1)
	{
		//Serial.print("P1,");  
		gPower = 1;
	}
	else if ( (inData & MASK_BTN) == GEAR_P2)
	{
		//Serial.print("P2,");  
		gPower = 2;
	}
	else if ( (inData & MASK_BTN) == GEAR_P3)
	{
		//Serial.print("P3,");  
		gPower = 3;
	}
	else if ( (inData & MASK_BTN) == GEAR_P4)
	{
		//Serial.print("P4,");  
		gPower = 4;
	}
	else if ( (inData & MASK_BTN) == GEAR_P5)
	{
		//Serial.print("P5,");  
		gPower = 5;
	}		
}
