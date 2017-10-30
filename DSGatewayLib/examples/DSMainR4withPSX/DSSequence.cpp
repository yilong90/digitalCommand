/*********************************************************************
 * Sequence for Desktop Station main
 *
 * Copyright (C) 2015 Yaasan
 *
 */

#include <Arduino.h>
#include <EEPROM.h>
#include "DSSequence.h"



DSSequence::DSSequence()
{
	
	/* Peripherals */
	pinMode(9, INPUT);//Select
	pinMode(8, INPUT);//Loc
	pinMode(7, INPUT);//Acc
	pinMode(A3, INPUT);//F0
	pinMode(A1, INPUT);//F1
	pinMode(6, INPUT);//DIR
	pinMode(2, INPUT);//STOP
	
	digitalWrite(9, HIGH); // enable internal pullup
	digitalWrite(8, HIGH); // enable internal pullup
	digitalWrite(7, HIGH); // enable internal pullup
	digitalWrite(6, HIGH); // enable internal pullup
	digitalWrite(2, HIGH); // enable internal pullup
	digitalWrite(A3, HIGH); // enable internal pullup
	digitalWrite(A1, HIGH); // enable internal pullup
	
	
	pinMode(A6, INPUT); // Input Speed ref
	digitalWrite(A6, LOW); // disable internal pullup
	analogReference(DEFAULT); // set analog reference 5V(VCC) 

	//Modes
	mode_protocol = 1;//DCC
	mode_seq = SQMODE_LOC;
	lastUsedFunc = SQCMD_NONE;
	
	//Previous values
	prev_dial = getDialStatus();
	prev_btns = getButtonStatus();
	
	//Control values
	NextLocoAddress = 3;
	NextLocoDirection = GO_FWD;
	NextAccAddress = 1;
	NextAccDirection = GO_STRAIGHT;
	NextPowerStatus = POWER_OFF;
	count_SelButton = 0;
	count_LocButton = 0;
	
	//Load EEPROM
	loadEEPROM();
	
	//CV values
	cv_no = 1;
	cv_value = 0;
	
	//Update protocol data
	setProtocol();
	
	//Function
	currentFnc[0] = 0;
	currentFnc[1] = 0;
	currentFnc[2] = 0;
	currentFnc[3] = 0;
	lastFuncNo = 0;
	
}

void DSSequence::loadEEPROM()
{
	
	if( EEPROM.read(0) == 0xAA)
	{
		
		NextLocoAddress = (EEPROM.read(3) << 8) + EEPROM.read(2);
		NextAccAddress = (EEPROM.read(5) << 8) + EEPROM.read(4);
		mode_protocol = EEPROM.read(6);
		
	}	
}

void DSSequence::saveEEPROM(byte inMode)
{
	if( EEPROM.read(0) == 0xAA)
	{
		
		if( inMode & DSEEPROM_LOCADDR > 0)
		{
			EEPROM.write(2, lowByte(NextLocoAddress));
			EEPROM.write(3, highByte(NextLocoAddress));
		}
		
		if( inMode & DSEEPROM_ACCADDR > 0)
		{
			EEPROM.write(4, lowByte(NextAccAddress));
			EEPROM.write(5, highByte(NextAccAddress));
		}
		
		if( inMode & DSEEPROM_PROTCOL > 0)
		{
			EEPROM.write(6, mode_protocol);
		}
		
	}
	else
	{
		//èâä˙âª
		EEPROM.write(0, 0xAA);
		
		//ÉAÉhÉåÉXèëÇ´ä∑Ç¶
		EEPROM.write(1, 0);
		EEPROM.write(2, lowByte(NextLocoAddress));
		EEPROM.write(3, highByte(NextLocoAddress));
		EEPROM.write(4, lowByte(NextAccAddress));
		EEPROM.write(5, highByte(NextAccAddress));
		EEPROM.write(6, mode_protocol);
		
	}
	
}

byte DSSequence::getButtonStatus()
{
	
	byte aButton_val = 0;
	
	//Input Buttons
	bitWrite(aButton_val, 0, digitalRead(9));//Select
	bitWrite(aButton_val, 1, digitalRead(8));//Loc
	bitWrite(aButton_val, 2, digitalRead(7));//Acc
	bitWrite(aButton_val, 3, digitalRead(A3));//F0
	bitWrite(aButton_val, 4, digitalRead(A1));//F1
	bitWrite(aButton_val, 5, (digitalRead(6) == 1) ? 0 : 1);//Dir
	bitWrite(aButton_val, 6, digitalRead(2));//Run/Stop
	
	return ~aButton_val;
	
}

word DSSequence::getDialStatus()
{
	//Mask lower 2bits (1024 to 256)
	return analogRead(A6) & 0b1111111100;
}

bool DSSequence::Interval()
{
	bool aChange = false;
	word aDial_val;
	byte aButton_val = 0;
	
	//Input Analog (lower bit is ignored)
	aDial_val = getDialStatus();
	
	//Input Buttons
	aButton_val = getButtonStatus();
	
	//Check changes
	if( aDial_val != prev_dial)
	{
		//Set speed
		NextLocoSpeed = aDial_val;
		
		if( ChangedTargetSpeed != NULL) {
			ChangedTargetSpeed(CurrentProtocol + NextLocoAddress, NextLocoSpeed, true);
			
			lastUsedFunc = SQCMD_SPEED;
		}
		aChange = true;
	}
	else
	{
		//Nothing to do
	}
	
	//Check button changes
	if( aButton_val != prev_btns)
	{
		
		if( (checkButtonContinuous(prev_btns, aButton_val, SQBUTTON_SEL) == true) && (checkButton(prev_btns, aButton_val, SQBUTTON_FN0) == true) )
		{
			//Clear counter
			count_SelButton = 255;
			
			clickSelectWithFunc0(mode_seq);
			

			
		}
		else if( (checkButtonContinuous(prev_btns, aButton_val, SQBUTTON_SEL) == true) && (checkButton(prev_btns, aButton_val, SQBUTTON_FN1) == true))
		{
			//Clear counter
			count_SelButton = 255;
			
			clickSelectWithFunc1(mode_seq);


		}
		else if( (checkButtonContinuous(prev_btns, aButton_val, SQBUTTON_SEL) == true) && ( checkButton(prev_btns, aButton_val, SQBUTTON_RUN) == true))
		{
			count_SelButton = 255;
			
			clickSelectWithRun(mode_seq);
			
		}
		else if( (checkButtonContinuous(prev_btns, aButton_val, SQBUTTON_SEL) == true) && ( checkButton(prev_btns, aButton_val, SQBUTTON_LOC) == true))
		{
			count_SelButton = 255;
			
			clickSelectWithLoc(mode_seq);
			
		}
		else if( (checkButtonContinuous(prev_btns, aButton_val, SQBUTTON_SEL) == true) && ( checkButton(prev_btns, aButton_val, SQBUTTON_ACC) == true))
		{
			count_SelButton = 255;
			
			clickSelectWithAcc(mode_seq);
			
		}
		else if( (checkButtonContinuous(prev_btns, aButton_val, SQBUTTON_LOC) == true) && ( checkButton(prev_btns, aButton_val, SQBUTTON_FN0) == true))
		{
			count_LocButton = 255;
			
			clickLocWithFunc0(mode_seq);
			
		}
		else if( (checkButtonContinuous(prev_btns, aButton_val, SQBUTTON_LOC) == true) && ( checkButton(prev_btns, aButton_val, SQBUTTON_FN1) == true))
		{
			count_LocButton = 255;
			
			clickLocWithFunc1(mode_seq);
		}
		else if( checkButton(prev_btns, aButton_val, SQBUTTON_RUN) == true)
		{
			
			clickRun(mode_seq);
			
		}
		else if( checkButton(prev_btns, aButton_val, SQBUTTON_LOC) == true)
		{
			
			count_LocButton = 0;
			
			clickLoc(mode_seq);
			
		}
		else if( checkButton(prev_btns, aButton_val, SQBUTTON_ACC) == true)
		{
			
			clickAcc(mode_seq);
		
		}
		else if( checkButton(prev_btns, aButton_val, SQBUTTON_DIR) == true)
		{
			clickDir(mode_seq);
			
		}
		else if( checkButton(prev_btns, aButton_val, SQBUTTON_FN0) == true)
		{
			clickFunc0(mode_seq);
			
			
		}
		else if( checkButton(prev_btns, aButton_val, SQBUTTON_FN1) == true)
		{
			clickFunc1(mode_seq);
			
		}
		else if( checkButton(prev_btns, aButton_val, SQBUTTON_SEL) == true)
		{
			count_SelButton = 0;
			
		}
		else if( checkButtonLeaved(prev_btns, aButton_val, SQBUTTON_SEL) == true)
		{
			count_SelButton = 0;
		}
		
	}
	else
	{
		if( checkButtonContinuous(prev_btns, aButton_val, SQBUTTON_SEL) == true)
		{
			if( count_SelButton < 255)
			{
				count_SelButton++;
				
				if( count_SelButton >= 30)
				{
					
					pushingSelect(mode_seq);
					
					count_SelButton = 255;
				}
			}
		}
		else if( checkButtonContinuous(prev_btns, aButton_val, SQBUTTON_LOC) == true)
		{
			if( count_LocButton < 255)
			{
				count_LocButton++;
				
				if( count_LocButton >= 30)
				{
					
					pushingLoc(mode_seq);
					
					count_LocButton = 255;
				}
			}
		}

	}
	
	
	//Store previous values
	prev_dial = aDial_val;
	prev_btns = aButton_val;
	
	return aChange;
}


void DSSequence::setTargetFunc(byte inNo)
{
	currentFnc[inNo] = (currentFnc[inNo] == 0) ? 1 : 0;
	
	ChangedTargetFunc(CurrentProtocol + NextLocoAddress, inNo, currentFnc[inNo], true);
	lastUsedFunc = SQCMD_FUNCTION;
	lastFuncNo = inNo;
}

void DSSequence::setProtocol()
{
	switch(mode_protocol)
	{
	case 0:
		CurrentProtocol = 0x0000;
		CurrentACCProtocol = 0x2FFF;
		break;
	case 1:
		CurrentProtocol = 0xC000;
		CurrentACCProtocol = 0x37FF;
		break;
		
	}
	
}

bool DSSequence::checkButton(byte inBuf, byte inCurrent, byte inButtonBit)
{

	if( ((inBuf & inButtonBit) == 0) && ((inCurrent & inButtonBit) > 0))
	{
		return true;
	}
	else
	{
		return false;
	}
	
}

bool DSSequence::checkButtonLeaved(byte inBuf, byte inCurrent, byte inButtonBit)
{

	if( ((inBuf & inButtonBit) > 0) && ((inCurrent & inButtonBit) == 0))
	{
		return true;
	}
	else
	{
		return false;
	}
	
}

bool DSSequence::checkButtonContinuous(byte inBuf, byte inCurrent, byte inButtonBit)
{

	if( ((inBuf & inButtonBit) > 0) && ((inCurrent & inButtonBit) > 0))
	{
		return true;
	}
	else
	{
		return false;
	}
	
}

void DSSequence::updateLCD()
{
	switch(lastUsedFunc)
	{
		case SQCMD_NONE:
		case SQCMD_SPEED:
			ChangedTargetSpeed(CurrentProtocol + NextLocoAddress, NextLocoSpeed, false);
			break;
		case SQCMD_FUNCTION:
			ChangedTargetFunc(CurrentProtocol + NextLocoAddress, lastFuncNo, currentFnc[lastFuncNo], false);
			break;
		case SQCMD_DIRECTION:
			ChangedTargetDir(CurrentProtocol + NextLocoAddress, NextLocoDirection, false);
			break;
		case SQCMD_TURNOUT:
			ChangedTurnout(CurrentACCProtocol + NextAccAddress, GO_STRAIGHT, false);
			break;
		
	}
}

void DSSequence::RegisterDir()
{
	//Call click dir
	clickDir(0);
}

void DSSequence::RegisterSpeed(word inSpeed)
{
	
	ChangedTargetSpeed(CurrentProtocol + NextLocoAddress, inSpeed, true);
	
}

void DSSequence::RegisterFunction(byte inFuncNo)
{
	setTargetFunc(inFuncNo);
	
}

void DSSequence::clickSelectWithFunc0(byte inSeqMode)
{

	switch(inSeqMode)
	{
	case SQMODE_ACC:
		
		break;
		
	case SQMODE_LOC:
		
		break;
		
	case SQMODE_LOC_EDIT:
		if( mode_protocol == 0)
		{
			if( locaddress_temp >= 255)
			{
				//MM2
				locaddress_temp = 1;
			}
			else
			{
				locaddress_temp = locaddress_temp + 100;
			}
		}
		else
		{
			if( locaddress_temp >= 9999)
			{
				//MM2
				locaddress_temp = 1;
			}
			else
			{
				locaddress_temp = locaddress_temp + 100;
			}
		}
		
		DisplayLocEditFunc(CurrentProtocol + locaddress_temp, mode_protocol, true);
	break;
	
	case SQMODE_ACC_EDIT:
		if( mode_protocol == 0)
		{
			if( accaddress_temp >= 320)
			{
				//MM2
				accaddress_temp = 1;
			}
			else
			{
				accaddress_temp = accaddress_temp + 100;
			}
		}
		else
		{
			if( accaddress_temp >= 2048)
			{
				//MM2
				accaddress_temp = 1;
			}
			else
			{
				accaddress_temp = accaddress_temp + 100;
			}
		}
		
		DisplayAccEditFunc(CurrentACCProtocol + accaddress_temp, mode_protocol, true);
	break;
		
	case SQMODE_CVW:
		if( cv_no >= 255)
		{
			cv_no = 1;
		}
		else
		{
			cv_no++;
		}
		
		DisplayCVFunc(cv_no, cv_value);
	break;
	
	
	
	}
}

void DSSequence::clickSelectWithFunc1(byte inSeqMode)
{

	switch(inSeqMode)
	{
	case SQMODE_ACC:
		
		break;
		
	case SQMODE_LOC:
		
		break;
		
	case SQMODE_LOC_EDIT:
		if( locaddress_temp <= 100)
		{
			if( mode_protocol == 0)
			{
				//MM2
				locaddress_temp = 255;
			}
			else
			{
				//DCC
				locaddress_temp = 9999;
			}
		}
		else
		{
			locaddress_temp = locaddress_temp - 100;
		}
		
		DisplayLocEditFunc(CurrentProtocol + locaddress_temp, mode_protocol, true);
	break;
	
	case SQMODE_ACC_EDIT:
		if( accaddress_temp <= 100)
		{
			if( mode_protocol == 0)
			{
				//MM2
				accaddress_temp = 320;
			}
			else
			{
				//DCC
				accaddress_temp = 2048;
			}
		}
		else
		{
			accaddress_temp = accaddress_temp - 100;
		}
		
		DisplayAccEditFunc(CurrentACCProtocol + accaddress_temp, mode_protocol, true);
	break;
		
	case SQMODE_CVW:
		if( cv_no <= 1)
		{
			cv_no = 255;
		}
		else
		{
			cv_no--;
		}
		
		DisplayCVFunc(cv_no, cv_value);
	break;
	
	
	
	}
}

void DSSequence::clickSelectWithRun(byte inSeqMode)
{
	
	switch(inSeqMode)
	{
		case SQMODE_ACC:
			break;
			
		case SQMODE_LOC:
			break;
			
		case SQMODE_LOC_EDIT:
			break;
			
		case SQMODE_ACC_EDIT:
			break;
			
		case SQMODE_CVW:
			//CV write
			ChangedCVFunc(cv_no, cv_value);
			break;
	}
	

	
}

void DSSequence::clickDir(byte inSeqMode)
{
	
	switch(inSeqMode)
	{
		case SQMODE_ACC:
			break;
			
		case SQMODE_LOC:
			break;
			
		case SQMODE_LOC_EDIT:
			break;
			
		case SQMODE_ACC_EDIT:
			break;
			
		case SQMODE_CVW:
			break;
	}
	
	NextLocoDirection = (NextLocoDirection == GO_FWD) ? GO_REV : GO_FWD;
	
	ChangedTargetDir(CurrentProtocol + NextLocoAddress, NextLocoDirection, true);
	
	
}

void DSSequence::clickRun(byte inSeqMode)
{
	
	switch(inSeqMode)
	{
		case SQMODE_ACC:
			break;
			
		case SQMODE_LOC:
			break;
			
		case SQMODE_LOC_EDIT:
			break;
			
		case SQMODE_ACC_EDIT:
			break;
			
		case SQMODE_CVW:
			break;
	}
	
	switch(NextPowerStatus)
	{
	case POWER_OFF:
		NextPowerStatus = POWER_ON;
		break;
	case POWER_ON:
		NextPowerStatus = POWER_OFF;
		break;
	}
	
	//Wait for DSCore
	delay(20);
	
	ChangedPowerStatus(NextPowerStatus);
	
	//Update protocol data
	setProtocol();
	
	//Wait for LCD message
	delay(TIME_RETURN);
	
	//Update LCD
	updateLCD();

}

void DSSequence::clickLoc(byte inSeqMode)
{
	
	switch(inSeqMode)
	{
		case SQMODE_ACC:
			break;
			
		case SQMODE_LOC:
			break;
			
		case SQMODE_LOC_EDIT:
			//Save LocAddress
			NextLocoAddress = locaddress_temp;
			saveEEPROM(DSEEPROM_LOCADDR);
			break;
			
		case SQMODE_ACC_EDIT:
			break;
			
		case SQMODE_CVW:
			break;
	}
	
	mode_seq = SQMODE_LOC;
	
	//Update protocol data
	setProtocol();
	
	lastUsedFunc = SQCMD_SPEED;
	
	//Update LCD
	updateLCD();
	
}


void DSSequence::clickAcc(byte inSeqMode)
{
	
	switch(inSeqMode)
	{
		case SQMODE_ACC:
			break;
			
		case SQMODE_LOC:
			break;
			
		case SQMODE_LOC_EDIT:
			break;
			
		case SQMODE_ACC_EDIT:
			//Save
			NextAccAddress = accaddress_temp;
			saveEEPROM(DSEEPROM_ACCADDR);
			break;
			
		case SQMODE_CVW:
			break;
	}

	
	mode_seq = SQMODE_ACC;
			
	//Update protocol data
	setProtocol();
	
	lastUsedFunc = SQCMD_TURNOUT;
	
	//Update LCD
	updateLCD();
	
}


void DSSequence::clickSelectWithLoc(byte inSeqMode)
{
	
	switch(inSeqMode)
	{
	case SQMODE_ACC:
	case SQMODE_LOC:
	case SQMODE_ACC_EDIT:
	case SQMODE_CVW:
		mode_seq = SQMODE_LOC_EDIT;
		locaddress_temp = NextLocoAddress;
		DisplayLocEditFunc(CurrentProtocol + NextLocoAddress, mode_protocol, true);
		break;
		
	case SQMODE_LOC_EDIT:
		clickLoc(inSeqMode);
		break;
	}
}

void DSSequence::clickSelectWithAcc(byte inSeqMode)
{
	
	switch(inSeqMode)
	{
	case SQMODE_ACC:
	case SQMODE_LOC:
	case SQMODE_CVW:
	case SQMODE_LOC_EDIT:
		mode_seq = SQMODE_ACC_EDIT;
		accaddress_temp = NextAccAddress;
		DisplayAccEditFunc(CurrentACCProtocol + NextAccAddress, mode_protocol, true);
		break;
		
	case SQMODE_ACC_EDIT:
		clickAcc(inSeqMode);
		break;
	}
}




void DSSequence::clickFunc0(byte inSeqMode)
{
	
	switch(inSeqMode)
	{
		case SQMODE_ACC:
			ChangedTurnout(CurrentACCProtocol + NextAccAddress, GO_STRAIGHT, true);
			lastUsedFunc = SQCMD_TURNOUT;
			break;
			
		case SQMODE_LOC:
			setTargetFunc(0);
			break;
			
		case SQMODE_LOC_EDIT:
			if( mode_protocol == 0)
			{
				if( locaddress_temp >= 255)
				{
					//MM2
					locaddress_temp = 1;
				}
				else
				{
					locaddress_temp = locaddress_temp + 1;
				}
			}
			else
			{
				if( locaddress_temp >= 9999)
				{
					//DCC
					locaddress_temp = 1;
				}
				else
				{
					locaddress_temp = locaddress_temp + 1;
				}
			}
			
			DisplayLocEditFunc(CurrentProtocol + locaddress_temp, mode_protocol, true);
		break;
		
		case SQMODE_ACC_EDIT:
			if( mode_protocol == 0)
			{
				if( accaddress_temp >= 320)
				{
					//MM2
					accaddress_temp = 1;
				}
				else
				{
					accaddress_temp = accaddress_temp + 1;
				}
			}
			else
			{
				if( accaddress_temp >= 2048)
				{
					//DCC
					accaddress_temp = 1;
				}
				else
				{
					accaddress_temp = accaddress_temp + 1;
				}
			}
			
			DisplayAccEditFunc(CurrentACCProtocol + accaddress_temp, mode_protocol, true);
		break;
		
		case SQMODE_CVW:
			if( cv_value >= 255)
			{
				cv_value = 0;
			}
			else
			{
				cv_value++;
			}
			
			DisplayCVFunc(cv_no, cv_value);
			break;
	}

}

void DSSequence::clickFunc1(byte inSeqMode)
{
	
	switch(inSeqMode)
	{
		case SQMODE_ACC:
			ChangedTurnout(CurrentACCProtocol + NextAccAddress, GO_DIVERSE, true);
			lastUsedFunc = SQCMD_TURNOUT;
			break;
			
		case SQMODE_LOC:
			setTargetFunc(1);
			break;
			
		case SQMODE_LOC_EDIT:
			if( locaddress_temp <= 1)
			{
				if( mode_protocol == 0)
				{
					//MM2
					locaddress_temp = 255;
				}
				else
				{
					//DCC
					locaddress_temp = 9999;
				}
			}
			else
			{
				locaddress_temp = locaddress_temp - 1;
			}
			
			DisplayLocEditFunc(CurrentProtocol + locaddress_temp, mode_protocol, true);
		break;
		
		case SQMODE_ACC_EDIT:
			if( accaddress_temp <= 1)
			{
				if( mode_protocol == 0)
				{
					//MM2
					accaddress_temp = 320;
				}
				else
				{
					//DCC
					accaddress_temp = 2048;
				}
			}
			else
			{
				accaddress_temp = accaddress_temp - 1;
			}
			
			DisplayAccEditFunc(CurrentACCProtocol + accaddress_temp, mode_protocol, true);
		break;
			
		case SQMODE_CVW:
			if( cv_value <= 0)
			{
				cv_value = 255;
			}
			else
			{
				cv_value--;
			}
			
			DisplayCVFunc(cv_no, cv_value);
			break;
	}
	
}

void DSSequence::pushingSelect(byte inSeqMode)
{
	
	switch(inSeqMode)
	{
	case SQMODE_ACC:
		break;
		
	case SQMODE_LOC:
		break;
		
	case SQMODE_LOC_EDIT:
	case SQMODE_ACC_EDIT:
		//Change Protocol
		
		mode_protocol++;
		
		if( mode_protocol > 1)
		{
			mode_protocol = 0;
		}
		
		DisplayMessage(mode_protocol);
		setProtocol();
		delay(TIME_RETURN);
		saveEEPROM(DSEEPROM_PROTCOL);
			
		if( inSeqMode == SQMODE_LOC_EDIT)
		{
			DisplayLocEditFunc(CurrentProtocol + locaddress_temp, mode_protocol, true);
			
		}
		else
		{
			DisplayAccEditFunc(CurrentACCProtocol + accaddress_temp, mode_protocol, true);
		}
			
		break;
		
	case SQMODE_CVW:
		break;
	}
}



void DSSequence::pushingLoc(byte inSeqMode)
{
	
	switch(inSeqMode)
	{
		case SQMODE_ACC:
			break;
			
		case SQMODE_LOC:
			break;
			
		case SQMODE_LOC_EDIT:
			break;
			
		case SQMODE_ACC_EDIT:
			break;
			
		case SQMODE_CVW:
			break;
	}
	
	
	mode_seq = SQMODE_CVW;
	
	DisplayMessage(2);
	delay(TIME_RETURN);
	DisplayCVFunc(cv_no, cv_value);
}




void DSSequence::clickLocWithFunc0(byte inSeqMode)
{
	
	switch(inSeqMode)
	{
		case SQMODE_ACC:
			break;
			
		case SQMODE_LOC:
			setTargetFunc(2);
			break;
			
		case SQMODE_LOC_EDIT:
			break;
			
		case SQMODE_ACC_EDIT:
			break;
			
		case SQMODE_CVW:
			break;
	}
}

void DSSequence::clickLocWithFunc1(byte inSeqMode)
{
	
	switch(inSeqMode)
	{
		case SQMODE_ACC:
			break;
			
		case SQMODE_LOC:
			setTargetFunc(3);
			break;
			
		case SQMODE_LOC_EDIT:
			break;
			
		case SQMODE_ACC_EDIT:
			break;
			
		case SQMODE_CVW:
			break;
	}
}

/*
void DSSequence::xxx(byte inSeqMode)
{
	
	switch(inSeqMode)
	{
		case SQMODE_ACC:
			break;
			
		case SQMODE_LOC:
			break;
			
		case SQMODE_LOC_EDIT:
			break;
			
		case SQMODE_ACC_EDIT:
			break
			
		case SQMODE_CVW:
			break;
	}
}
*/
