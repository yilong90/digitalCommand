/*********************************************************************
 * Sequence for Desktop Station main
 *
 * Copyright (C) 2015 Yaasan
 *
 */

#include <Arduino.h>
#include <EEPROM.h>
#include <avr/wdt.h>
#include "DSSequence.h"

//#define DEBUG 

#define		BTN_NUM5	0b0000000000000001 //SW09
#define		BTN_NUM1	0b0000000000000010 //SW02
#define		BTN_NUM9	0b0000000000000100 //SW13
#define		BTN_NUM4	0b0000000000001000 //SW05
#define		BTN_CLR		0b0000000000010000 //SW14
#define		BTN_NUM0	0b0000000000100000 //SW01
#define		BTN_NUM2	0b0000000001000000 //SW03
#define		BTN_NUM8	0b0000000010000000 //SW12
#define		BTN_ESC		0b0000000100000000 //SW06
#define		BTN_UP		0b0000001000000000 //SW15
#define		BTN_NUM6	0b0000010000000000 //SW06
#define		BTN_NUM3	0b0000100000000000 //SW04
#define		BTN_NUM7	0b0001000000000000 //SW11
#define		BTN_ENT		0b0010000000000000 //SW07
#define		BTN_DN		0b0100000000000000 //SW08


#define		MAX_MAINMENU	11


DSSequence::DSSequence()
{
	
	for( int i = 0; i < 128; i++)
	{
		gAcc_FuncBuf[i] = 0;
	}
	
	gMode = MODE_MAINMENU;
	gNo_CVReadMenu = 0;
	gNo_CVWriteMenu = 0;
	gNo_CVReadWriteMenu = 0;
	gNo_AddrChkMenu = 0;
	gNo_AddrWriteMenu = 0;
	gNo_AccCtrlMenu = 0;
	gNo_LocCtrlMenu = 0;
	gNo_FactoryResetMenu = 0;
	gNo_ManCtrlMenu = 0;
	gNo_ConfigMenu = 0;
	gNo_NSignalMenu = 0;
	
	
	gAcc_Addr = 1;
	gAcc_Dir = 0;
	gLoc_Addr = 3;
	gLoc_Func = 0;
	gLoc_Dir = 0;
	gLoc_Spd = 0;
	
	gCfgReadSpeed = 0;
	gCfgReadPower = 0;
	gCfgThreshold = 8;
	gCfgRetry = 2;
	
	CVNo = 1;
	CVValue = 0;
	Address = 0;
	
	gLastManID = 0;
	
	SignalAddr[0] = 0;
	SignalAddr[1] = 0;
	SignalAddr[2] = 0;
	SignalAddr[3] = 0;
	SignalAddr[4] = 0;
	SignalAddr[5] = 0;
	
	loadEEPROM();
	
	gDScmd.mCmd = 0;
	
	count = 0;
}

PackedDScmd DSSequence::GetCommad()
{
	return gDScmd;
}


void DSSequence::loadEEPROM()
{
	
	if( EEPROM.read(0) == 0xBB)
	{
		
		gCfgReadSpeed = EEPROM.read(1);
		gCfgThreshold = EEPROM.read(2);
		gCfgRetry = EEPROM.read(3);
		
		if( gCfgReadSpeed > 1)
		{
			gCfgReadSpeed = 0;
		}
		
		if( gCfgThreshold > 64)
		{
			gCfgThreshold = 8;
		}
		
		if( gCfgRetry > 5)
		{
			gCfgRetry = 5;
		}
	}	
	
}

void DSSequence::saveEEPROM(byte inMode)
{
	
	if( EEPROM.read(0) == 0xBB)
	{
		
		switch(inMode)
		{
		case 0:
			EEPROM.write(1, gCfgReadSpeed);
			break;
			
		case 1:
			EEPROM.write(2, gCfgThreshold);
			break;
			
		case 2:
			EEPROM.write(3, gCfgRetry);
			break;
		}
	}
	else
	{
		//初期化
		EEPROM.write(0, 0xBB);
		
		//設定
		EEPROM.write(1, gCfgReadSpeed);
		EEPROM.write(2, gCfgThreshold);
		EEPROM.write(3, gCfgRetry);

		
	}
	
}

word DSSequence::getButtonStatus()
{
	
	word aButton_val = 0;
	
	byte aTempBtnA = 0;
	byte aTempBtnB = 0;
	byte aTempBtnC = 0;
	
	unsigned short aButtonA = analogRead(PORT_BTNA);
	unsigned short aButtonB = analogRead(PORT_BTNB);
	unsigned short aButtonC = analogRead(PORT_BTNC);
	
	//Get button status(返回值 1，2，4，8，16)
	aTempBtnA = GetButtonsFromAD(aButtonA);
	aTempBtnB = GetButtonsFromAD(aButtonB);
	aTempBtnC = GetButtonsFromAD(aButtonC);
	
	//Check and process the buttons
	aButton_val = ProcessCheck(aTempBtnA, aTempBtnB, aTempBtnC);
	
	return aButton_val;
	
}

byte DSSequence::Interval()
{
	bool aChange = false;
	word aButton_val = 0;
	
	gDisplayFlag = 0;
	
	//Input Buttons （确认按了哪个键）
	aButton_val = getButtonStatus();
	
	//Sequence main
	switch(gMode)
	{
		case MODE_MAINMENU:
			interval_Main(aButton_val);
			break;
    }
	//Store previous values
	gButtonBuf = aButton_val;
	
	return gDisplayFlag;
}


void DSSequence::interval_Main(word inButtonStatus)
{
	//Check button changes
	if( inButtonStatus != gButtonBuf)
	{
		
		if( checkButton(gButtonBuf, inButtonStatus, BTN_ENT) == true )
		{
			//工作状态转换, gNo_MainMode（0） 与 gMode （1）差2（有MODE_NONE），
			gMode = gNo_MainMenu + 2;
			
#ifdef DEBUG
			Serial.print(12345);
			Serial.println(gMode, DEC);
#endif
			
			
// 			if ( (gMode == MODE_LOCCTRL) || (gMode == MODE_ACCCTRL))
// 			{
				
// #ifdef DEBUG
// 				Serial.println("Power On (CTRL mode)");
// #endif
				
// 				//パワーオン
// 				ChangedPowerStatus(1);
				
// 			}
			
			
			//表示処理
			gDisplayFlag |= 1;
		}
		else if( checkButton(gButtonBuf, inButtonStatus, BTN_UP) == true )
		{
			//不能往上
			// if( gNo_MainMenu <= 0)
			// {
			// 	gNo_MainMenu = 0;
			// }
			// else
			// {
			// 	gNo_MainMenu--;
			// }
			count--;
			gNo_MainMenu = count % MAX_MAINMENU;
			//表示処理
			gDisplayFlag |= 1;
			
		}
		else if( checkButton(gButtonBuf, inButtonStatus, BTN_DN) == true )
		{
			
			// if( gNo_MainMenu >= MAX_MAINMENU)
			// {
			// 	gNo_MainMenu = MAX_MAINMENU;
			// }
			// else
			// {
			// 	gNo_MainMenu++;
			// }
			count++;
			gNo_MainMenu = count % MAX_MAINMENU;
			//表示処理
			gDisplayFlag |= 1;
			
		}
	}	
}



byte DSSequence::GetButtonsFromAD(unsigned short inADData)
{
  /*
  R1  2.2kΩ
  R2  470Ω
  R3  1kΩ
  R4  2.2kΩ
  R5  6.8kΩ
  
  Threshold value:
  SW1: 82
  SW2: 290
  SW3: 525
  SW4: 747
  SW5: 942
  
  
  */
  
  byte aRet = 0;
  
  if( inADData <= 82)
  {
    aRet = 1 << 0;
  }
  else if( inADData <= 290)
  {
    aRet = 1 << 1;
  }
  else if( inADData <= 525)
  {
    aRet = 1 << 2;
  }
  else if( inADData <= 747)
  {
    aRet = 1 << 3;
  }
  else if( inADData <= 942)
  {
    aRet = 1 << 4;
  }
  else
  {
    //Not pressed
  }
  
  
  return aRet;
}


bool DSSequence::checkButton(word inBuf, word inCurrent, word inButtonBit)
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

bool DSSequence::editNums(word inButtonStatus, word *iopNum, word inMax)
{
	word aNum = *iopNum;
	bool aRet = false;
	
	if( checkButton(gButtonBuf, inButtonStatus, BTN_NUM1) == true )
	{
		aNum = aNum * 10 + 1;
		aRet = true;
	}
	else if( checkButton(gButtonBuf, inButtonStatus, BTN_NUM2) == true )
	{
		aNum = aNum * 10 + 2;
		aRet = true;
	}
	else if( checkButton(gButtonBuf, inButtonStatus, BTN_NUM3) == true )
	{
		aNum = aNum * 10 + 3;
		aRet = true;
	}
	else if( checkButton(gButtonBuf, inButtonStatus, BTN_NUM4) == true )
	{
		aNum = aNum * 10 + 4;
		aRet = true;
	}
	else if( checkButton(gButtonBuf, inButtonStatus, BTN_NUM5) == true )
	{
		aNum = aNum * 10 + 5;
		aRet = true;
	}
	else if( checkButton(gButtonBuf, inButtonStatus, BTN_NUM6) == true )
	{
		aNum = aNum * 10 + 6;
		aRet = true;
	}
	else if( checkButton(gButtonBuf, inButtonStatus, BTN_NUM7) == true )
	{
		aNum = aNum * 10 + 7;
		aRet = true;
	}
	else if( checkButton(gButtonBuf, inButtonStatus, BTN_NUM8) == true )
	{
		aNum = aNum * 10 + 8;
		aRet = true;
	}
	else if( checkButton(gButtonBuf, inButtonStatus, BTN_NUM9) == true )
	{
		aNum = aNum * 10 + 9;
		aRet = true;
	}
	else if( checkButton(gButtonBuf, inButtonStatus, BTN_NUM0) == true )
	{
		aNum = aNum * 10 + 0;
		aRet = true;
	}
	else if( checkButton(gButtonBuf, inButtonStatus, BTN_CLR) == true )
	{
		aNum = 0;
		aRet = true;
	}	
	
	//Check max value
	if( aNum >= inMax)
	{
		aNum = inMax;
		aRet = true;
	}
	
	
	*iopNum = aNum;
	
	return aRet;
}

word DSSequence::ProcessCheck( byte inTempA, byte inTempB, byte inTempC)
{
  byte aChanged = 0;
  word aButtonData = 0;

// PCB Revision 4
  switch(inTempA)
  {
  case 1:
    //SW9, 5
    aButtonData |= BTN_ESC;//
    break;
  case 2:
    //SW2, 1
    aButtonData |= BTN_NUM2;//
    break;
  case 4:
    //SW13, 9
    aButtonData |= BTN_ENT;//
    break;
  case 8:
    //SW5, 4
    aButtonData |= BTN_NUM7;//
    break;
  case 16:
    //SW14, CLR
    aButtonData |= BTN_NUM8;//
    break;
  }
    
  switch(inTempB)
  {
  case 1:
    //SW1, 0
    aButtonData |= BTN_DN;//
    break;
  case 2:
    //SW3, 2
    aButtonData |= BTN_NUM3;//
    break;
  case 4:
    //SW12, 8
    aButtonData |= BTN_CLR;//
    break;
  case 8:
    //SW6, ESC
    aButtonData |= BTN_NUM6;//
    break;
  case 16:
    //SW15, UP
    aButtonData |= BTN_NUM0;//
    break;
  }

    
  switch(inTempC)
  {
  case 1:
    //SW10, 6
    aButtonData |= BTN_UP;//
    break;
  case 2:
    //SW4, 3
    aButtonData |= BTN_NUM4;//
    break;
  case 4:
    //SW11, 7
    aButtonData |= BTN_NUM1;//
    break;
  case 8:
    //SW7, ENTER
    aButtonData |= BTN_NUM5;//OK
    break;
  case 16:
    //SW8, DOWN
    aButtonData |= BTN_NUM9;//OK
    break;
  }
  
  return aButtonData;
  
}



