/*********************************************************************
 * Sequence for Desktop Station main
 *
 * Copyright (C) 2015 Yaasan
 *
 */

#include <Arduino.h>
#include <EEPROM.h>
#include "DSSequence.h"

#define PRESS_THRESHOLD		10

DSSequence::DSSequence()
{
	
	/* Peripherals */
	pinMode(A3, INPUT);//Enter
	pinMode(A2, INPUT);//Up
	pinMode(2, INPUT);//Down
	pinMode(3, INPUT);//Escape
	pinMode(4, INPUT);//STOP
	pinMode(5, INPUT);//DIR
	
	digitalWrite(A3, HIGH); // enable internal pullup
	digitalWrite(A2, HIGH); // enable internal pullup
	digitalWrite(2, HIGH); // enable internal pullup
	digitalWrite(3, HIGH); // enable internal pullup
	digitalWrite(4, HIGH); // enable internal pullup
	digitalWrite(5, HIGH); // enable internal pullup
	
	
	pinMode(A6, INPUT); // Input Speed ref
	digitalWrite(A6, LOW); // disable internal pullup
	analogReference(DEFAULT); // set analog reference 5V(VCC) 

	//Modes
	mode_protocol = 1;//DCC
	mode_seq = SQMODE_LOC;
	mode_rj45 = RJ45MODE_S88;
	mode_oclv = 8;//8Amax
	
	mode_depth = SQCMD_DEPTH_LOC;
	mode_depth_last = 0;
	
	//Function
	for( int i = 0; i < 29; i++)
	{
		currentFnc[i] = 0;
	}
	currentFuncNo = 0;
	
	//Menu
	currentMenuNo = 0;
	
	//Configuration
	currentCfgNo = 0;
	
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
	count_AdrButton = 0;
	
	//Load EEPROM
	loadEEPROM();
	
	NextLocoAddress = eepLocAddr;
	NextAccAddress = eepAccAddr;
	
	//CV values
	cv_no = 1;
	cv_value = 0;
	
	//Update protocol data
	setProtocol();
	
}

byte DSSequence::GetRJ45Mode()
{
	return mode_rj45;
	
}

byte DSSequence::GetOCLVMode()
{
	return mode_oclv;
	
}



void DSSequence::loadEEPROM()
{
	
	if( EEPROM.read(0) == 0xBB)
	{
		
		eepLocAddr = (EEPROM.read(3) << 8) + EEPROM.read(2);
		eepAccAddr = (EEPROM.read(5) << 8) + EEPROM.read(4);
		mode_protocol = EEPROM.read(6);
		mode_rj45 = EEPROM.read(7);
		mode_oclv = EEPROM.read(8);
		
		if( mode_protocol > 1)
		{
			mode_protocol = 0;
		}

		if( mode_rj45 > 1)
		{
			mode_rj45 = 0;
		}
		
		/* おかしな値の排除 */
		if( (mode_oclv == 0) || (mode_oclv > 8))
		{
			mode_oclv = 8;
		}
		
		
		
	}
	else
	{
		eepLocAddr = 3;
		eepAccAddr = 1;
	}
	
}

void DSSequence::saveEEPROM(byte inMode)
{
	
	if( EEPROM.read(0) == 0xBB)
	{
		
		if( inMode == DSEEPROM_LOCADDR)
		{
			EEPROM.write(2, lowByte(NextLocoAddress));
			EEPROM.write(3, highByte(NextLocoAddress));
		}
		
		if( inMode ==  DSEEPROM_ACCADDR)
		{
			EEPROM.write(4, lowByte(NextAccAddress));
			EEPROM.write(5, highByte(NextAccAddress));
		}
		
		if( inMode ==  DSEEPROM_PROTCOL)
		{
			EEPROM.write(6, mode_protocol);
		}
		
		if( inMode == DSEEPROM_RJ45SEL)
		{
			EEPROM.write(7, mode_rj45);
		}
		if( inMode == DSEEPROM_OCLVSEL)
		{
			EEPROM.write(8, mode_oclv);
		}

		
		
		
	}
	else
	{
		//初期化
		EEPROM.write(0, 0xBB);
		
		//アドレス書き換え
		EEPROM.write(1, 0);
		EEPROM.write(2, lowByte(NextLocoAddress));
		EEPROM.write(3, highByte(NextLocoAddress));
		EEPROM.write(4, lowByte(NextAccAddress));
		EEPROM.write(5, highByte(NextAccAddress));
		EEPROM.write(6, mode_protocol);
		EEPROM.write(7, mode_rj45);
		
	}
	
}

byte DSSequence::getButtonStatus()
{
	
	byte aButton_val = 0;
	
	//Input Buttons
	bitWrite(aButton_val, 1, digitalRead(A3));//ENTER
	bitWrite(aButton_val, 2, digitalRead(3));//ESC
	bitWrite(aButton_val, 3, digitalRead(A2));//UP
	bitWrite(aButton_val, 4, digitalRead(2));//DOWN
	bitWrite(aButton_val, 5, (digitalRead(5) == 1) ? 0 : 1);//Dir
	bitWrite(aButton_val, 6, digitalRead(4));//Run/Stop
	
	return ~aButton_val;
	
}

word DSSequence::getDialStatus()
{
	//Mask lower 2bits (1024 to 256)
	return analogRead(A6) & 0b1111111100;
}

byte DSSequence::GetModeDepth()
{
	return mode_depth;
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
			
		}
		aChange = true;
	}
	else
	{
		//Nothing to do
	}
	
	//Sequence main
	
	switch( mode_depth)
	{
	case SQCMD_DEPTH_LOC:
		interval_Loc(aButton_val);
		
		break;
		
	case SQCMD_DEPTH_MENU:
		interval_Menu(aButton_val);
		
		break;
		
	case SQCMD_DEPTH_ACC:
		interval_Acc(aButton_val);
		
		break;
		
	case SQCMD_DEPTH_CVR:
		interval_ReadCV(aButton_val);
		
		break;
		
	case SQCMD_DEPTH_CVW:
		interval_WriteCV(aButton_val);
		
		break;
	
	case SQCMD_DEPTH_INFO:
		interval_Info(aButton_val);
		break;
		
	case SQCMD_DEPTH_CFG:
		interval_Config(aButton_val);
		break;
		
		
		
	}
	
	//Always run event
	interval_always(aButton_val);
	
	
	
	//Store previous values
	prev_dial = aDial_val;
	prev_btns = aButton_val;
	
	return aChange;
}

/****************************************************
 
 Always run event
 
 ****************************************************/

void DSSequence::interval_always(byte inButtonStatus)
{
	
	//Check button changes
	if( inButtonStatus != prev_btns)
	{
		if( checkButton(prev_btns, inButtonStatus, SQBUTTON_RUN) == true )
		{
			
			clickRun();
			
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_DIR) == true )
		{
			
			clickDir();
			
		}
	}	
	
}

void DSSequence::interval_Loc(byte inButtonStatus)
{
	
	//Check button changes
	if( inButtonStatus != prev_btns)
	{
		
		
		if( (checkButtonContinuous(prev_btns, inButtonStatus, SQBUTTON_ENT) == true) && (checkButton(prev_btns, inButtonStatus, SQBUTTON_UP) == true) )
		{
			//Clear counter
			count_SelButton = 255;
			
			currentFuncNo++;
			
			//F28超のとき
			if( currentFuncNo > 28)
			{
				currentFuncNo = 0;
			}
			
			
			/* 表示処理 */
			ChangedTargetFunc(CurrentProtocol + NextLocoAddress, currentFuncNo, currentFnc[currentFuncNo], false);
			
		}
		else if( (checkButtonContinuous(prev_btns, inButtonStatus, SQBUTTON_ENT) == true) && (checkButton(prev_btns, inButtonStatus, SQBUTTON_DN) == true) )
		{
			//Clear counter
			count_SelButton = 255;
			
			//F0のとき
			if( currentFuncNo == 0 )
			{
				currentFuncNo = 28;
			}
			else
			{
				currentFuncNo--;
			}
			
			
			/* 表示処理 */
			ChangedTargetFunc(CurrentProtocol + NextLocoAddress, currentFuncNo, currentFnc[currentFuncNo], false);
			
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_ENT) == true)
		{
			count_SelButton = 0;
			
		}
		else if( checkButtonLeaved(prev_btns, inButtonStatus, SQBUTTON_ENT) == true)
		{
			
			if( count_SelButton <= PRESS_THRESHOLD)
			{
			
				/*ファンクション操作*/
				if( currentFnc[currentFuncNo] == 1)
				{
					currentFnc[currentFuncNo] = 0;
				}
				else
				{
					currentFnc[currentFuncNo] = 1;
				}
			
				//表示
				RegisterFunction(currentFuncNo);
				
				
				//EEPROM操作
				if( eepLocAddr != NextLocoAddress)
				{
					eepLocAddr = NextLocoAddress;
					saveEEPROM(DSEEPROM_LOCADDR);
				}
			}
			
			//Clear counter
			count_SelButton = 0;
		}
		else if( (checkButtonLeaved(prev_btns, inButtonStatus, SQBUTTON_UP) == true) && (checkButtonContinuous(prev_btns, inButtonStatus, SQBUTTON_ENT) == false))
		{
			
			if( count_AdrButton <= PRESS_THRESHOLD)
			{
				locaddress_temp = NextLocoAddress;
				plusLocAddress(&locaddress_temp, mode_protocol, 1);
				NextLocoAddress = locaddress_temp;
				ChangedTargetSpeed(CurrentProtocol + NextLocoAddress, NextLocoSpeed, false);
			}
			
			//Clear counter
			count_AdrButton = 0;
		}
		else if( (checkButtonLeaved(prev_btns, inButtonStatus, SQBUTTON_DN) == true) && (checkButtonContinuous(prev_btns, inButtonStatus, SQBUTTON_ENT) == false))
		{
			
			
			if( count_AdrButton <= PRESS_THRESHOLD)
			{
				locaddress_temp = NextLocoAddress;
				minusLocAddress(&locaddress_temp, mode_protocol, 1);
				NextLocoAddress = locaddress_temp;
				ChangedTargetSpeed(CurrentProtocol + NextLocoAddress, NextLocoSpeed, false);
			}
			
			//Clear counter
			count_AdrButton = 0;
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_UP) == true )
		{
			//Clear counter
			count_AdrButton = 0;
			
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_DN) == true )
		{
			//Clear counter
			count_AdrButton = 0;
			
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_ESC) == true )
		{
			//何もしない
			NextMode(SQCMD_DEPTH_MENU);
			
			//MENUに入る
			ChangedMenu(currentMenuNo);
		}
	}
	else
	{
		//変化なしの時（連続押しの場合）
		
		if( (checkButtonContinuous(prev_btns, inButtonStatus, SQBUTTON_UP) == true ) && (checkButtonContinuous(prev_btns, inButtonStatus, SQBUTTON_ENT) == false))
		{
			count_AdrButton++;
			
			locaddress_temp = NextLocoAddress;
			
			if( count_AdrButton < PRESS_THRESHOLD)
			{
				//何もしない
				return;
			}
			else if( count_AdrButton < 50)
			{
				plusLocAddress(&locaddress_temp, mode_protocol, 1);
			}
			else if( count_AdrButton <150)
			{
				plusLocAddress(&locaddress_temp, mode_protocol, 10);
			}
			else if( count_AdrButton < 250)
			{
				plusLocAddress(&locaddress_temp, mode_protocol, 100);
			}
			else
			{
				count_AdrButton = 255;
				plusLocAddress(&locaddress_temp, mode_protocol, 200);
			}
			
			
			NextLocoAddress = locaddress_temp;
			
			ChangedTargetSpeed(CurrentProtocol + NextLocoAddress, NextLocoSpeed, false);
			
		}
		else if( (checkButtonContinuous(prev_btns, inButtonStatus, SQBUTTON_DN) == true ) && (checkButtonContinuous(prev_btns, inButtonStatus, SQBUTTON_ENT) == false))
		{
			count_AdrButton++;
			
			locaddress_temp = NextLocoAddress;
			
			if( count_AdrButton < PRESS_THRESHOLD)
			{
				//何もしない
				return;
			}
			else if( count_AdrButton < 50)
			{
				minusLocAddress(&locaddress_temp, mode_protocol, 1);
			}
			else if( count_AdrButton < 150)
			{
				minusLocAddress(&locaddress_temp, mode_protocol, 10);
			}
			else if( count_AdrButton < 250)
			{
				minusLocAddress(&locaddress_temp, mode_protocol, 100);
			}
			else
			{
				count_AdrButton = 255;
				minusLocAddress(&locaddress_temp, mode_protocol, 200);
			}
			
			NextLocoAddress = locaddress_temp;
			
			ChangedTargetSpeed(CurrentProtocol + NextLocoAddress, NextLocoSpeed, false);
			
		}
		else if( checkButtonContinuous(prev_btns, inButtonStatus, SQBUTTON_ENT) == true )
		{
			if( count_SelButton < 255)
			{
				count_SelButton++;
				
			}
			
		}
		
	}
	
}

void DSSequence::minusLocAddress(word *iopLocAddr, byte inProtocol, word inMinus)
{
	if( ((int)*iopLocAddr - (int)inMinus) <= 0)
	{
		if( inProtocol == 0)
		{
			//MM2
			*iopLocAddr = 255;
		}
		else
		{
			//DCC
			*iopLocAddr = 9999;
		}
	}
	else
	{
		*iopLocAddr = *iopLocAddr - inMinus;
	}
}

void DSSequence::plusLocAddress(word *iopLocAddr, byte inProtocol, word inPlus)
{
	
	if( inProtocol == 0)
	{
		if( (*iopLocAddr + inPlus) >= 255)
		{
			//MM2
			*iopLocAddr = 1;
		}
		else
		{
			*iopLocAddr = *iopLocAddr + inPlus;
		}
	}
	else
	{
		if( (*iopLocAddr + inPlus) >= 9999)
		{
			//DCC
			*iopLocAddr = 1;
		}
		else
		{
			*iopLocAddr = *iopLocAddr + inPlus;
		}
	}
}

void DSSequence::minusAccAddress(word *iopAccAddr, byte inProtocol, word inMinus)
{
	if( ((int)*iopAccAddr - (int)inMinus) <= 0)
	{
		if( inProtocol == 0)
		{
			//MM2
			*iopAccAddr = 320;
		}
		else
		{
			//DCC
			*iopAccAddr = 2044;
		}
	}
	else
	{
		*iopAccAddr = *iopAccAddr - inMinus;
	}
}

void DSSequence::plusAccAddress(word *iopAccAddr, byte inProtocol, word inPlus)
{
	
	if( inProtocol == 0)
	{
		if( (*iopAccAddr + inPlus) >= 320)
		{
			//MM2
			*iopAccAddr = 1;
		}
		else
		{
			*iopAccAddr = *iopAccAddr + inPlus;
		}
	}
	else
	{
		if( (*iopAccAddr + inPlus) >= 2044)
		{
			//DCC
			*iopAccAddr = 1;
		}
		else
		{
			*iopAccAddr = *iopAccAddr + inPlus;
		}
	}
}


void DSSequence::interval_Acc(byte inButtonStatus)
{
	
	//Check button changes
	if( inButtonStatus != prev_btns)
	{
		if( checkButton(prev_btns, inButtonStatus, SQBUTTON_UP) == true )
		{
			//Clear counter
			count_AdrButton = 0;
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_DN) == true )
		{
			//Clear counter
			count_AdrButton = 0;
			
		}
		else if( checkButtonLeaved(prev_btns, inButtonStatus, SQBUTTON_UP) == true)
		{
			
			if( count_AdrButton <= PRESS_THRESHOLD)
			{
				accaddress_temp = NextAccAddress;
				
				plusAccAddress(&accaddress_temp, mode_protocol, 1);
				NextAccAddress = accaddress_temp;
				
				ChangedAcc(CurrentACCProtocol + NextAccAddress, NextAccDirection, false);
				
			}
			//Clear counter
			count_AdrButton = 0;
		}
		else if( checkButtonLeaved(prev_btns, inButtonStatus, SQBUTTON_DN) == true)
		{
			
			if( count_AdrButton <= PRESS_THRESHOLD)
			{
				accaddress_temp = NextAccAddress;
				
				minusAccAddress(&accaddress_temp, mode_protocol, 1);
				NextAccAddress = accaddress_temp;
				
				ChangedAcc(CurrentACCProtocol + NextAccAddress, NextAccDirection, false);
			}
			//Clear counter
			count_AdrButton = 0;
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_ENT) == true )
		{
			//操作
			if( NextAccDirection == 0 )
			{
				NextAccDirection = 1;
			}
			else
			{
				NextAccDirection = 0;
			}
			
			ChangedAcc(CurrentACCProtocol + NextAccAddress, NextAccDirection, true);
			
			//EEPROM操作
			if( eepAccAddr != NextAccAddress)
			{
				eepAccAddr = NextAccAddress;
				saveEEPROM(DSEEPROM_ACCADDR);
			}
			
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_ESC) == true )
		{
			NextMode(SQCMD_DEPTH_MENU);
			
			//MENUに入る
			ChangedMenu(currentMenuNo);
		}
	}
	else
	{
		//変化なしの時（連続押しの場合）
		
		if( checkButtonContinuous(prev_btns, inButtonStatus, SQBUTTON_UP) == true )
		{
			count_AdrButton++;
			
			accaddress_temp = NextAccAddress;
			
			if( count_AdrButton < PRESS_THRESHOLD)
			{
				//何もしない
				return;
			}
			else if( count_AdrButton < 50)
			{
				plusAccAddress(&accaddress_temp, mode_protocol, 1);
			}
			else if( count_AdrButton <150)
			{
				plusAccAddress(&accaddress_temp, mode_protocol, 10);
			}
			else if( count_AdrButton < 250)
			{
				plusAccAddress(&accaddress_temp, mode_protocol, 20);
			}
			else
			{
				count_AdrButton = 255;
				plusAccAddress(&accaddress_temp, mode_protocol, 50);
			}
			
			
			NextAccAddress = accaddress_temp;
			
			ChangedAcc(CurrentACCProtocol + NextAccAddress, NextAccDirection, false);
			
		}
		else if( checkButtonContinuous(prev_btns, inButtonStatus, SQBUTTON_DN) == true )
		{
			count_AdrButton++;
			
			accaddress_temp = NextAccAddress;
			
			if( count_AdrButton < PRESS_THRESHOLD)
			{
				//何もしない
				return;
			}
			else if( count_AdrButton < 50)
			{
				minusAccAddress(&accaddress_temp, mode_protocol, 1);
			}
			else if( count_AdrButton < 150)
			{
				minusAccAddress(&accaddress_temp, mode_protocol, 10);
			}
			else if( count_AdrButton < 250)
			{
				minusAccAddress(&accaddress_temp, mode_protocol, 20);
			}
			else
			{
				count_AdrButton = 255;
				minusAccAddress(&accaddress_temp, mode_protocol, 50);
			}
			
			NextAccAddress = accaddress_temp;
			
			ChangedAcc(CurrentACCProtocol + NextAccAddress, NextAccDirection, false);
			
		}
		else if( checkButtonContinuous(prev_btns, inButtonStatus, SQBUTTON_ENT) == true )
		{
			if( count_SelButton < 255)
			{
				count_SelButton++;
				
			}
			
		}
		
	}
	
	
}

void DSSequence::interval_Menu(byte inButtonStatus)
{
	
	//Check button changes
	if( inButtonStatus != prev_btns)
	{
		if( checkButton(prev_btns, inButtonStatus, SQBUTTON_DN) == true )
		{
			//Clear counter
			count_SelButton = 255;
			
			if( currentMenuNo >= 6)
			{
				currentMenuNo = 6;
			}
			else
			{
				currentMenuNo = currentMenuNo + 1;
			}
			
			ChangedMenu(currentMenuNo);
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_UP) == true )
		{
			//Clear counter
			count_SelButton = 255;
			
			
			if( currentMenuNo <= 1)
			{
				currentMenuNo = 0;
			}
			else
			{
				currentMenuNo = currentMenuNo - 1;
			}
			
			ChangedMenu(currentMenuNo);
			
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_ENT) == true )
		{
			//切り替え
			switch(currentMenuNo)
			{
			
			case 1:
				//Accessory
				NextMode(SQCMD_DEPTH_ACC);
				
				//Refresh
				DisplayClear();
				ChangedAcc(CurrentACCProtocol + NextAccAddress, NextAccDirection, false);
				
				break;
			case 2:
				//CV Read
				NextMode(SQCMD_DEPTH_CVR);
				
				//Refresh
				DisplayClear();
				
				//Read CV Func
				currentCVSeq = 0;
				ReadCVFunc(currentCVSeq, cv_no, cv_value);
				break;
			case 3:
				//CV Write
				NextMode(SQCMD_DEPTH_CVW);
				
				//Refresh
				DisplayClear();
				
				//Read CV Func
				currentCVSeq = 0;
				WriteCVFunc(currentCVSeq, cv_no, cv_value);
				
				break;
			case 4:
				if( mode_protocol == 0)
				{
					//DCC Mode
					mode_protocol = 1;
				}
				else
				{
					//MM2 Mode
					mode_protocol = 0;
					
				}
				
				setProtocol();
				NextMode(SQCMD_DEPTH_LOC);
				
				//Refresh
				DisplayClear();
				ChangedMain();
				
				/* EEPROM書き換え */
				saveEEPROM(DSEEPROM_PROTCOL);
				delay(10);
				
				break;
			case 5:
				//Info
				
				NextMode(SQCMD_DEPTH_INFO);
				
				//Refresh
				DisplayClear();
				ChangedInfo();
				
				break;
			case 6:
				//Config
				NextMode(SQCMD_DEPTH_CFG);
				
				//Refresh
				DisplayClear();
				ChangedConfiguration(currentCfgNo);
				
				break;
			case 0:
			default:
				//Locomotive
				NextMode(SQCMD_DEPTH_LOC);
				
				//Refresh
				DisplayClear();
				ChangedMain();
				break;
				
			}
			
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_ESC) == true )
		{
			/* 何もしない */
			
		}

	}
	
}


void DSSequence::interval_Config(byte inButtonStatus)
{
	
	//Check button changes
	if( inButtonStatus != prev_btns)
	{
		if( checkButton(prev_btns, inButtonStatus, SQBUTTON_DN) == true )
		{
			//Clear counter
			count_SelButton = 255;
			
			if( currentCfgNo >= 1)
			{
				currentCfgNo = 1;
			}
			else
			{
				currentCfgNo = currentCfgNo + 1;
			}
			
			ChangedConfiguration(currentCfgNo);
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_UP) == true )
		{
			//Clear counter
			count_SelButton = 255;
			
			
			if( currentCfgNo <= 0)
			{
				currentCfgNo = 0;
			}
			else
			{
				currentCfgNo = currentCfgNo - 1;
			}
			
			ChangedConfiguration(currentCfgNo);
			
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_ENT) == true )
		{
			//切り替え
			switch(currentCfgNo)
			{
			case 0:
				//RJ45 
				
				
				if( mode_rj45 == 1)
				{
					//S88
					mode_rj45 = 0;
				}
				else if (mode_rj45 == 0)
				{
					//PSX
					mode_rj45 = 1;
				}
				else
				{
					mode_rj45 = 0;
				}
				
				
				/* 表示切替 */
				ChangedConfiguration(currentCfgNo);
				
				/* EEPROM書き換え */
				saveEEPROM(DSEEPROM_RJ45SEL);
				delay(10);
				
				break;
			case 1:
				//OCLv 
				
				mode_oclv++;
				
				if( mode_oclv > 8)
				{
					//S88
					mode_oclv = 1;
				}
				
				/* 表示切替 */
				ChangedConfiguration(currentCfgNo);
				
				/* EEPROM書き換え */
				saveEEPROM(DSEEPROM_OCLVSEL);
				delay(10);
				
				break;
			default:
				//何もしない
				
				break;
				
			}
			
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_ESC) == true )
		{
			/* メニューに戻る */
			DisplayClear();
			
			ChangedMenu(currentMenuNo);
			mode_depth = SQCMD_DEPTH_MENU;
			
		}

	}
	
}



void DSSequence::interval_ReadCV(byte inButtonStatus)
{
	
	//Check button changes
	if( inButtonStatus != prev_btns)
	{
		if( checkButton(prev_btns, inButtonStatus, SQBUTTON_UP) == true )
		{
			//Clear counter
			count_SelButton = 255;
			
			if( currentCVSeq == 1)
			{
				return;
			}
			
			if( cv_no >= 1024)
			{
				cv_no = 1;
			}
			else
			{
				cv_no = cv_no + 1;
			}
			
			ReadCVFunc(currentCVSeq, cv_no, cv_value);
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_DN) == true )
		{
			//Clear counter
			count_SelButton = 255;
			
			if( currentCVSeq == 1)
			{
				return;
			}
			
			
			if( cv_no <= 1)
			{
				cv_no = 1024;
			}
			else
			{
				cv_no = cv_no - 1;
			}
			
			ReadCVFunc(currentCVSeq, cv_no, cv_value);
			
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_ENT) == true )
		{
			//切り替え
			switch(currentCVSeq)
			{
			
			case 0:
				//Address to Read(CVR)
				currentCVSeq++;
				
				//Read CV Func
				ReadCVFunc(currentCVSeq, cv_no, cv_value);
				break;
			case 1:
				//Go to Menu
				mode_depth = SQCMD_DEPTH_MENU;
				
				//Refresh
				DisplayClear();
				ChangedMenu(currentMenuNo);
				
				break;
			}
			
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_ESC) == true )
		{
			
			if( currentCVSeq == 0)
			{
				DisplayClear();
				
				
				ChangedMenu(currentMenuNo);
				mode_depth = SQCMD_DEPTH_MENU;
			}
			else
			{
				currentCVSeq--;
				//Read CV Func
				ReadCVFunc(currentCVSeq, cv_no, cv_value);
			}
		}

	}
	
}



void DSSequence::interval_WriteCV(byte inButtonStatus)
{
	
	//Check button changes
	if( inButtonStatus != prev_btns)
	{
		if( checkButton(prev_btns, inButtonStatus, SQBUTTON_UP) == true )
		{
			//Clear counter
			count_SelButton = 255;
			
			if( currentCVSeq == 0)
			{
				if( cv_no >= 1024)
				{
					cv_no = 1;
				}
				else
				{
					cv_no = cv_no + 1;
				}
			}
			else if (currentCVSeq == 1)
			{
				if( cv_value >= 255)
				{
					cv_value = 0;
				}
				else
				{
					cv_value = cv_value + 1;
				}
			}
			
			
			WriteCVFunc(currentCVSeq, cv_no, cv_value);
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_DN) == true )
		{
			//Clear counter
			count_SelButton = 255;
			
			if( currentCVSeq == 0)
			{
				if( cv_no <= 1)
				{
					cv_no = 1024;
				}
				else
				{
					cv_no = cv_no - 1;
				}
			}
			else if (currentCVSeq == 1)
			{
				
				if( cv_value <= 0)
				{
					cv_value = 255;
				}
				else
				{
					cv_value = cv_value - 1;
				}
			}
			
			
			WriteCVFunc(currentCVSeq, cv_no, cv_value);
			
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_ENT) == true )
		{
			//切り替え
			switch(currentCVSeq)
			{
			
			case 0:
			case 1:
				//Address to Read(CVR)
				currentCVSeq++;
				
				//Write CV Func
				WriteCVFunc(currentCVSeq, cv_no, cv_value);
				break;
			case 2:
				//Go to Menu
				mode_depth = SQCMD_DEPTH_MENU;
				
				//Refresh
				DisplayClear();
				ChangedMenu(currentMenuNo);
				
				break;
			}
			
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_ESC) == true )
		{
			
			if( currentCVSeq == 0)
			{
				DisplayClear();
				
				
				ChangedMenu(currentMenuNo);
				mode_depth = SQCMD_DEPTH_MENU;
			}
			else
			{
				currentCVSeq--;
				//Read CV Func
				WriteCVFunc(currentCVSeq, cv_no, cv_value);
			}
		}

	}
	
}


void DSSequence::interval_Info(byte inButtonStatus)
{
	
	//Check button changes
	if( inButtonStatus != prev_btns)
	{
		if( checkButton(prev_btns, inButtonStatus, SQBUTTON_UP) == true )
		{
			//何もしない
			
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_DN) == true )
		{
			//何もしない
			
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_ENT) == true )
		{
			//何もしない
		}
		else if( checkButton(prev_btns, inButtonStatus, SQBUTTON_ESC) == true )
		{
			DisplayClear();
			
			
			ChangedMenu(currentMenuNo);
			mode_depth = SQCMD_DEPTH_MENU;
		}

	}
	
}

void DSSequence::setProtocol()
{
	switch(mode_protocol)
	{
	case 0:
		//MM2
		CurrentProtocol = 0x0000;
		CurrentACCProtocol = 0x2FFF;
		
		//リミット処理
		if( NextLocoAddress > 255)
		{
			NextLocoAddress = 255;
		}
		if( NextAccAddress > 320)
		{
			NextAccAddress = 320;
		}
		
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

void DSSequence::RegisterDir()
{
	//Call click dir
	clickDir();
}

void DSSequence::RegisterSpeed(word inSpeed)
{
	
	ChangedTargetSpeed(CurrentProtocol + NextLocoAddress, inSpeed, true);
	
}

void DSSequence::RegisterFunction(byte inFuncNo)
{
	ChangedTargetFunc(CurrentProtocol + NextLocoAddress, inFuncNo, currentFnc[inFuncNo], true);
}


void DSSequence::NextMode(byte inMode)
{
	mode_depth_last = mode_depth;
	mode_depth = inMode;
}

void DSSequence::clickDir()
{
	
	NextLocoDirection = (NextLocoDirection == GO_FWD) ? GO_REV : GO_FWD;
	
	ChangedTargetDir(CurrentProtocol + NextLocoAddress, NextLocoDirection, true);
	
	
}

void DSSequence::clickRun()
{
	
	switch(NextPowerStatus)
	{
	case POWER_OFF:
		
		//エラーチェック
		
		
		
		
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
	delay(TIME_RETURN >> 1);

}



