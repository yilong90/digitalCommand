/*********************************************************************
 * LCD Manager for Desktop Station main
 *
 * Copyright (C) 2015 Yaasan
 *
 */

#include <Arduino.h>
#include <avr/pgmspace.h>
#include "DSGatewayLibM.h"
#include "DSLCDManager.h"
#include "DSSequence.h"
#include <string.h>

char aText[16 + 1];

const char rMenu0[] PROGMEM = " Loco   ";
const char rMenu1[] PROGMEM = " Turnout";
const char rMenu2[] PROGMEM = " CVRead ";
const char rMenu3[] PROGMEM = " CVWrite";
const char rMenu4[] PROGMEM = " MM2mode";
const char rMenu5[] PROGMEM = " DCCmode";
const char rMenu6[] PROGMEM = " Sensors";
const char rMenu7[] PROGMEM = " Config ";
const char rMenuCfg0[] PROGMEM = " RJ45";
const char rMenuCfg1[] PROGMEM = " OCLv";
const char rBlank[] PROGMEM    = "        ";
const char rReading[] PROGMEM  = "Reading.";
const char rWriting[] PROGMEM  = "Writing.";
const char rCantRead[] PROGMEM = "Read Err";
const char rFinWrite[] PROGMEM = "WriteEnd";
const char rFinWriteFail[] PROGMEM = "WriteErr";
const char rNoError[] PROGMEM    = "NO ERRORS";
const char rErrorOC[] PROGMEM    = "ERR: OC ";
const char rErrorOV[] PROGMEM    = "ERR: OV ";
const char rErrorLV[] PROGMEM    = "ERR: LV ";



DSLCDManager::DSLCDManager()
{
	valCurrent_H = 0;
	valCurrent_L = 0;
	valVoltage_H = 0;
	valVoltage_L = 0;
	valErrCurrent_H = 0;
	valErrCurrent_L = 0;
	valErrVoltage_H = 0;
	valErrVoltage_L = 0;
	valErrorFlag = 0;
	
	valPower = 0;
	valAccAddress = 0;
	valAccDirection = 0;
	
	valLocAddress = 0;
	valLocSpeed = 0;
	valLocDirection = 0;
	valLocFunction = 0;
	valLocFuncPower = 0;
	
	mode_cv_seq = 0;
	
	//Menu
	valSelectedMenuItemNo = 0;
	valConfigItemNo = 0;
	
	mode_display = MODE_DISP_HELLO;
	mode_display_temp = MODE_DISP_TEMP_NONE;
	flagRefresh = true;
	
}

void DSLCDManager::SetSensors(unsigned char inI_H, unsigned char inI_L, unsigned char inV_H, unsigned char inV_L, unsigned char inErrFlag)
{
	
	valCurrent_H = inI_H;
	valCurrent_L = inI_L;
	valVoltage_H = inV_H;
	valVoltage_L = inV_L;
	valErrorFlag = inErrFlag;
	
	flagRefresh = true;
}

void DSLCDManager::SetSensorsErr(unsigned char inI, unsigned char inV)
{
	
	unsigned char aCurrent_H = inI / 10;
	unsigned char aCurrent_L = inI - (aCurrent_H * 10);
	unsigned char aVoltage_H = inV / 10;
	unsigned char aVoltage_L = inV - (aVoltage_H * 10);
	
	valErrCurrent_H = aCurrent_H;
	valErrCurrent_L = aCurrent_L;
	valErrVoltage_H = aVoltage_H;
	valErrVoltage_L = aVoltage_L;
	
}

void DSLCDManager::Interval()
{
	word aLocProtocol;
	int i;
	
	if( flagRefresh == false)
	{
		// Refresh is not required
		return;
	}
	
	
	switch( mode_display)
	{
	case MODE_DISP_HELLO:
		//PrintString("Desktop Station", 0, 3);
		//PrintString("Welcome aboard!", 0, 2);
		break;
		
	case MODE_DISP_LOC:
		
		DisplayLocAddress();
		
		clearTextBuf();
		
		switch( mode_display_temp)
		{
		case MODE_DISP_TEMP_NONE:
			sprintf(aText, "%3d%c %s", ((word)(valLocSpeed >> 3) * 100) >> 7, 37, valLocDirection == 1 ? "FWD" : "REV");
			break;
		case MODE_DISP_TEMP_FUNC:
			sprintf(aText, "F%02d %s ", valLocFunction, (valLocFuncPower == 1) ? "ON " : "OFF");
			break;
		}
		
		if( mode_display_temp_cnt == 0)
		{
			mode_display_temp = MODE_DISP_TEMP_NONE;
		}
		else
		{
			mode_display_temp_cnt--;
		}
			
		PrintString(aText, 0, 1);
		
		break;
	
	
	case MODE_DISP_ACC:
		DisplayAccAddress();
		
		if( valAccDirection == 1)
		{
			PrintString("Str |", 0, 1);
		}
		else
		{
			PrintString("Div / ", 0, 1);
		}		
		
		break;
		
		
	case MODE_DISP_CVR:
		
		
		if( mode_cv_seq == 0)
		{
			sprintf(aText, "CVNo%4d", valCVAddr);
			PrintString(aText, 0, 0);
			sprintf(aText, "Ent:Read");
			PrintString(aText, 0, 1);
		}
		else if( mode_cv_seq == 10)
		{
			sprintf(aText, "CVNo%4d", valCVAddr);
			PrintString(aText, 0, 0);
			StrCpyFromROM(aText, rReading, 8);
			PrintString(aText, 0, 1);
			
		}
		else if( mode_cv_seq == 1)
		{
			sprintf(aText, "CV =%4d", valCVAddr);
			PrintString(aText, 0, 0);
			sprintf(aText, "Val=>%3d", valCVValue);
			PrintString(aText, 0, 1);
		}
		else if( mode_cv_seq == 11)
		{
			sprintf(aText, "CVNo%4d", valCVAddr);
			PrintString(aText, 0, 0);
			StrCpyFromROM(aText, rCantRead, 8);
			PrintString(aText, 0, 1);
		}
		
		
		break;
		
	case MODE_DISP_CVW:
		
		/* Row 1 */
		
		if( mode_cv_seq == 0)
		{
			sprintf(aText, "%cCV=%4d", 0x07, valCVAddr);
		}
		else
		{
			sprintf(aText, " CV=%4d", valCVAddr);
		}
		
		PrintString(aText, 0, 0);
		
		
		if( (mode_cv_seq == 0) || ( mode_cv_seq == 1))
		{
			/* Row 2 */
			if( mode_cv_seq == 1)
			{
				sprintf(aText, "%cVal=%3d", 0x07, valCVValue);
			}
			else
			{
				sprintf(aText, " Val=%3d", valCVValue);
			}
			
			PrintString(aText, 0, 1);
		}
		else if( mode_cv_seq == 2)
		{
			
			if( valCVAddr == 0xFFFF)
			{
				//Failed to CV Write
				StrCpyFromROM(aText, rFinWriteFail, 8);
				PrintString(aText, 0, 1);
			}
			else
			{
				//Success CV write
				StrCpyFromROM(aText, rFinWrite, 8);
				PrintString(aText, 0, 1);
			}
		}
		else if( mode_cv_seq == 20)
		{
			StrCpyFromROM(aText, rWriting, 8);
			PrintString(aText, 0, 1);
		}
		
		break;
		
		
	case MODE_DISP_MENU:
		
		i = 0;
		
		for( int j = valSelectedMenuItemNo; j <= valSelectedMenuItemNo + 1; j++)
		{
			
			switch(j)
			{
			case 0: 
				StrCpyFromROM(aText, rMenu0, 8);
				break;
			case 1: 
				StrCpyFromROM(aText, rMenu1, 8);
				break;
			case 2: 
				StrCpyFromROM(aText, rMenu2, 8);
				break;
			case 3: 
				StrCpyFromROM(aText, rMenu3, 8);
				break;
			case 4: 
				aLocProtocol = GetLocIDProtocol(valLocAddress >> 8);
				
				if( aLocProtocol == ADDR_DCC)
				{
					StrCpyFromROM(aText, rMenu4, 8);
				}
				else
				{
					StrCpyFromROM(aText, rMenu5, 8);
				}
				
				break;
			case 5: 
				StrCpyFromROM(aText, rMenu6, 8);
				break;
			case 6: 
				StrCpyFromROM(aText, rMenu7, 8);
				
				break;
			default:
				StrCpyFromROM(aText, rBlank, 8);
				break;
				
			}
			
			if( j == valSelectedMenuItemNo)
			{
				aText[0] = 0x07;
			}
			else
			{
				//nothing to do
			}			
			
			PrintString(aText, 0, i);
			i++;
		}

		
		break;
		
	case MODE_DISP_CFG:
		
		i = 0;
		
		for( int j = valConfigItemNo; j <= valConfigItemNo + 1; j++)
		{
			
			switch(j)
			{
			case 0: 
				StrCpyFromROM(aText, rMenuCfg0, 5);
				
				switch( GetConfigData(0))
				{
				case RJ45MODE_S88:
					aText[5] = ' ';
					aText[6] = 'S';
					aText[7] = '8';
					break;
				case RJ45MODE_DSJ:
					aText[5] = ' ';
					aText[6] = 'J';
					aText[7] = 'Y';
					break;
				}
				break;
			case 1: 
				//OC Level
				StrCpyFromROM(aText, rMenuCfg1, 5);
				
				aText[5] = ' ';
				aText[6] = '0' + GetConfigData(1);
				aText[7] = 'A';
				
				break;
			default:
				StrCpyFromROM(aText, rBlank, 8);
				break;
				
			}
			
			if( j == valConfigItemNo)
			{
				aText[0] = 0x07;
			}
			else
			{
				//nothing to do
			}			
			
			PrintString(aText, 0, i);
			i++;
		}		
		
		break;
		
		
		
	case MODE_DISP_INF:
		/* Sensor */
		
		if( valErrorFlag > 0)
		{
			/* Error Value Display */
			sprintf(aText, "%d.%dA %2dV", valErrCurrent_H, valErrCurrent_L, valErrVoltage_H + (valErrVoltage_L > 4 ? 1 : 0));
		}
		else
		{
			/* Non Errors */
			if( valCurrent_H == 0)
			{
				sprintf(aText, "0.%dA %2dV", valCurrent_L, valVoltage_H + (valVoltage_L > 4 ? 1 : 0));
			}
			else
			{
				sprintf(aText, "%d.%dA %2dV", valCurrent_H, valCurrent_L, valVoltage_H + (valVoltage_L > 4 ? 1 : 0));
			}
		}
			
		PrintString(aText, 0, 1);
		
		/* Error flag */
		if( valErrorFlag > 0)
		{
			if( (valErrorFlag & 0b100) > 0)
			{
				StrCpyFromROM(aText, rErrorOC, 8);
			}
			else if( (valErrorFlag & 0b010) > 0)
			{
				StrCpyFromROM(aText, rErrorLV, 8);
			}
			else if( (valErrorFlag & 0b001) > 0)
			{
				StrCpyFromROM(aText, rErrorOV, 8);
			}
			
			PrintString(aText, 0, 0);
			
		}
		else
		{
			StrCpyFromROM(aText, rNoError, 8);
			PrintString(aText, 0, 0);
		}
		break;
		
	}
	
	//Sets Refresh disabled.
	flagRefresh = false;
	
}


void DSLCDManager::StrCpyFromROM(char *outpText, const char *inpROMText, byte inLen)
{
	byte i;
	
	for( i = 0; i < inLen; i++)
	{
		outpText[i] = (char)pgm_read_byte(inpROMText + i);
	}
	
	for( i = inLen; i < 16; i++)
	{
		outpText[i] = 0x00;
	}
}

void DSLCDManager::DisplayLocAddress()
{
	word aProtocol = GetLocIDProtocol(valLocAddress >> 8);
	sprintf(aText, "LOC%4d%s", valLocAddress - aProtocol, aProtocol == ADDR_DCC ? "D" : "M");
	PrintString(aText, 0, 0);
	
}

void DSLCDManager::DisplayAccAddress()
{
	word aProtocol = GetLocIDProtocol(valAccAddress >> 8);
	sprintf(aText, "ACC%4d%s", valAccAddress - aProtocol, aProtocol == ADDR_ACC_DCC ? "D" : "M");
	PrintString(aText, 0, 0);
	
}

void DSLCDManager::RegisterLocSpeed(word inLocAddr, word inSpeed)
{
	valLocAddress = inLocAddr;
	valLocSpeed = inSpeed;
	
	flagRefresh = true;
	
}

void DSLCDManager::RegisterLoc(word inLocAddr)
{
	valLocAddress = inLocAddr;
	
	mode_display = MODE_DISP_LOC;
	flagRefresh = true;
	
}

void DSLCDManager::RegisterLocDirection(word inLocAddr, unsigned char inDir)
{
	valLocAddress = inLocAddr;
	valLocDirection = inDir;
	
	flagRefresh = true;
	
}

void DSLCDManager::RegisterLocFunction(word inLocAddr, unsigned char inFuncNo, unsigned char inFuncPower)
{
	valLocAddress = inLocAddr;
	valLocFunction = inFuncNo;
	valLocFuncPower = inFuncPower;
	
	flagRefresh = true;
	mode_display_temp = MODE_DISP_TEMP_FUNC;
	mode_display_temp_cnt = 1;
}

void DSLCDManager::RegisterPower(unsigned char inPower)
{
	valPower = inPower;
	
	flagRefresh = true;
	
}

void DSLCDManager::RegisterTurnout(word inAccAddr, unsigned char inDir)
{
	//�O������̕\���p�̓o�^�֐�
	word aLocProtocol = GetLocIDProtocol(inAccAddr >> 8);
	
	//valAccAddress = inAccAddr;
	//valAccDirection = inDir;
	
	sprintf(aText, "Ac%4d%s %s",inAccAddr - aLocProtocol,aLocProtocol == ADDR_ACC_DCC ? "D":"M", inDir==1?"|":"/");
	AddMessages(aText);

	flagRefresh = true;
	
}

void DSLCDManager::RegisterAcc(word inAccAddr, unsigned char inDir)
{
	//��������p�̓o�^�֐�
	valAccAddress = inAccAddr;
	valAccDirection = inDir;
	
	mode_display = MODE_DISP_ACC;
	flagRefresh = true;
	
}

void DSLCDManager::RegisterMenu(byte inMenuItemNo)
{
	
	valSelectedMenuItemNo = inMenuItemNo;
	mode_display = MODE_DISP_MENU;
	flagRefresh = true;
	
}

void DSLCDManager::RegisterConfig(byte inConfigItemNo)
{
	
	valConfigItemNo = inConfigItemNo;
	mode_display = MODE_DISP_CFG;
	flagRefresh = true;
	
}

void DSLCDManager::RegisterMain()
{
	
	
	mode_display = MODE_DISP_LOC;
	flagRefresh = true;
	
}

unsigned char DSLCDManager::GetMode()
{
	return mode_display;
}

void DSLCDManager::RegisterInfo()
{
	
	
	mode_display = MODE_DISP_INF;
	flagRefresh = true;
	
}


void DSLCDManager::RegisterCVWrite(byte inSeq, word inAddr, byte inValue)
{
	valCVAddr = inAddr;
	valCVValue= inValue;
	
	mode_display = MODE_DISP_CVW;
	mode_cv_seq = inSeq;
	flagRefresh = true;
	
}

void DSLCDManager::RegisterCVRead(byte inSeq, word inAddr, byte inValue)
{
	valCVAddr = inAddr;
	valCVValue= inValue;
	
	mode_display = MODE_DISP_CVR;
	mode_cv_seq = inSeq;
	flagRefresh = true;
	
}

word DSLCDManager::GetLocIDProtocol(byte address)
{
	if( address < 0x04)
	{
			return ADDR_MM2;
	}
	else if( (address >= 0x30) && (address <= 0x33))
	{
			return ADDR_ACC_MM2;
	}
	else if( (address >= 0x38) && (address <= 0x3F))
	{
			return ADDR_ACC_DCC;
	}
	else if( (address >= 0x40) && (address <= 0x70))
	{
			return ADDR_MFX;
	}
	else if( (address >= 0xC0) && (address <= 0xFF))
	{
			return ADDR_DCC;
	}
	else
	{
		return 0;
	}
}



void DSLCDManager::AddMessages(char *inText)
{
	
	
	//Messages
	PrintString(inText, 0, 1);
	
	mode_display = MODE_DISP_LOC;
	flagRefresh = true;
	
	
}

void DSLCDManager::WriteLocAddr(word inAddr)
{
	valLocAddress = inAddr;
	
}

void DSLCDManager::WriteAccAddr(word inAddr)
{
	valAccAddress = inAddr;
	
}

void DSLCDManager::clearTextBuf()
{
	for( int i = 0; i < 16; i++)
	{
		aText[i] = 0;
	}
}


