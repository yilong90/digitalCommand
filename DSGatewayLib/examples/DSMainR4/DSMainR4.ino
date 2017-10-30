/*********************************************************************
 * Desktop Station main R4 Sketch for Arduino UNO/Leonard
 *
 * Copyright (C) 2015 Yaasan
 *
 */

#include <avr/interrupt.h>
#include <avr/io.h>
#include <EEPROM.h>
#include "TrackReporterS88_DS.h"
#include "DSGatewayLib.h"
#include "I2CLCDLIB.h"
#include "NewSerial.h"
#include "SPI.h"
#include "Wire.h"
#include "DSSequence.h"

#define MAX_S88DECODER 1
#define RELPYERROR_300 "300 Command error"
#define RELPYERROR_301 "301 Syntax error"
#define RELPYERROR_302 "302 receive timeout"
#define RELPYERROR_303 "303 Unknown error"
#define RELPYERROR_NONE ""

#define MAX_ANALOGINTERVAL 254

#define LCDMSG_PWR 10
#define LCDMSG_DIR 20
#define LCDMSG_SPD 30
#define LCDMSG_ACC 40
#define LCDMSG_FNC 50
#define LCDMSG_MODE_MM2 60
#define LCDMSG_MODE_DCC 61
#define LCDMSG_MODE_CVW 62
#define LCDMSG_MODE_LOC 63
#define LCDMSG_MODE_ACC 64
#define LCDMSG_MODE_CVWRITING 65
#define LCDMSG_MODE_CV 70
#define LCDMSG_MODE_LOCEDIT 80
#define LCDMSG_MODE_ACCEDIT 81

TrackReporterS88_DS reporter(MAX_S88DECODER);
DSGatewayLib Gateway;
I2CLCDLib LCD;
DSSequence Sequence;
NewSerial SerialDS;

String function;
word arguments[8];
word numOfArguments;

boolean result;

//Task Schedule
unsigned long gPreviousL5 = 0;


void printMessage(byte inNo, word inAddress, word inValue1, byte inValue2, byte inValue3);
void ReplySpeedPacket(word inLocaddress, word inSpeed);
void ReplyDirPacket(word inLocaddress, byte inDir);
void ReplyFuncPacket(word inLocaddress, byte inNo, byte inPower);
void ReplyAccPacket(word inAccaddress, byte inDir);
void ReplyPowerPacket(byte inPower);

void setup() {
  
  LCD.begin();
  
  LCD.lcd_setCursor(0, 0);
  LCD.lcd_printStr("WELCOME");
  LCD.lcd_setCursor(1, 1);
  LCD.lcd_printStr("ABOARD!"); 
  

  /* Clear data of the S88 decoder */
  reporter.refresh();

  SerialDS.Init();
  SerialDS.println("--------------------------------------");
  SerialDS.println("Desktop Station main R4               ");
  SerialDS.println("--------------------------------------");
  SerialDS.println("100 Ready");
  
  pinMode(8, INPUT);
  pinMode(9, INPUT);
  digitalWrite(8, HIGH); // disable internal pullup
  digitalWrite(9, HIGH); // disable internal pullup
  
	//Reset task
	gPreviousL5 = millis();
	
	//Register Event functions
	Sequence.ChangedTargetSpeed = changedTargetSpeed;
	Sequence.ChangedTargetFunc = changedTargetFunc;
	Sequence.ChangedTargetDir = changedTargetDir;
	Sequence.ChangedTurnout = changedTurnout;
	Sequence.ChangedPowerStatus = changedPowerStatus;
	Sequence.DebugMessage = debugMessage;
	Sequence.DisplayMessage = displayMessage;
	Sequence.DisplayCVFunc = displayCVFunc;
	Sequence.ChangedCVFunc = changedCVFunc;
	Sequence.DisplayLocEditFunc = displayLocEditFunc;
	Sequence.DisplayAccEditFunc = displayAccEditFunc;


  Gateway.begin();
  
  sei();    //ENABLE INTERRUPTION
	

}

void printMessage(byte inNo, word inAddress, word inValue1, byte inValue2, byte inValue3)
{
	char s[16];
	word aACCProtocol;
	word aProtocol;

	LCD.lcd_clear();
	LCD.lcd_setCursor(0, 0);


	if( (inNo == 20) || (inNo == 30) || (inNo == 50))
	{
		aProtocol = Gateway.GetLocIDProtocol(inAddress >> 8);
		sprintf(s, "LOC%4d%s", inAddress - aProtocol, aProtocol == ADDR_DCC ? "D" : "M");
		LCD.lcd_printStr(s);
	}

	switch(inNo)
	{
	case 10:
		if( inValue1 == 0)
		{
		LCD.lcd_printStr("PWR OFF");
		}
		else
		{
		LCD.lcd_printStr("PWR ON");
		}
		LCD.lcd_setCursor(0, 1);
		LCD.lcd_printStr("");
		break;
		
	case 20:
		LCD.lcd_setCursor(0, 1);
		
		if( inValue1 == 1)
		{
		LCD.lcd_printStr("DIR FWD");
		}
		else
		{
		LCD.lcd_printStr("DIR REV");
		}      
		break;
		
	case 30:
		LCD.lcd_setCursor(0, 1);
		sprintf(s, "SPD%3d%c", ((word)(inValue1 >> 3) * 100) >> 7, 37);
		LCD.lcd_printStr(s);    
		break; 
		
	case 40:
		aACCProtocol = Gateway.GetLocIDProtocol(inAddress >> 8);
		LCD.lcd_setCursor(0, 0);
		sprintf(s, "ACC %3d%s", inAddress - aACCProtocol, aACCProtocol == ADDR_ACC_DCC ? "D" : "M");
		LCD.lcd_printStr(s);
		LCD.lcd_setCursor(0, 1);
		sprintf(s, "%s", inValue1 == 1 ? "Straight" : "Diverse");
		LCD.lcd_printStr(s);	
		break;

	case 50:
		LCD.lcd_setCursor(0, 1);
		sprintf(s, "FNC%2d %s", inValue1, inValue2 == 0 ? "OF" : "ON" );
		LCD.lcd_printStr(s);
		break;
	case 60:
		LCD.lcd_setCursor(0, 0);
		LCD.lcd_printStr("MM2 Mode ");
		break;    
	case 61:
		LCD.lcd_setCursor(0, 0);
		LCD.lcd_printStr("DCC Mode ");
		break;  
	case 62:
		LCD.lcd_setCursor(0, 0);
		LCD.lcd_printStr("CV Mode");
		LCD.lcd_setCursor(0, 1);
		LCD.lcd_printStr("Write");
		break;
	case 63:
		LCD.lcd_setCursor(0, 0);
		LCD.lcd_printStr("Loco    ");
		LCD.lcd_setCursor(0, 1);
		LCD.lcd_printStr("Control ");
		break;    
	case 64:
		LCD.lcd_setCursor(0, 0);
		LCD.lcd_printStr("Turnout ");
		LCD.lcd_setCursor(0, 1);
		LCD.lcd_printStr("Control ");
		break;
	case LCDMSG_MODE_CVWRITING:
		LCD.lcd_setCursor(0, 0);
		LCD.lcd_printStr("Writing");
		LCD.lcd_setCursor(0, 1);
		LCD.lcd_printStr("CV...");
		break;
	case LCDMSG_MODE_CV:
		LCD.lcd_setCursor(0, 0);
		sprintf(s, "CVNo%3d", inValue1);
		LCD.lcd_printStr(s);
		LCD.lcd_setCursor(0, 1);
		sprintf(s, "Val %3d", inValue2);
		LCD.lcd_printStr(s);
		break;
	case LCDMSG_MODE_LOCEDIT:
		LCD.lcd_setCursor(0, 0);
		LCD.lcd_printStr("LocAddr");
		LCD.lcd_setCursor(0, 1);
		
		aProtocol = Gateway.GetLocIDProtocol(inAddress >> 8);
		sprintf(s, "%s %4d", aProtocol == ADDR_DCC ? "DCC" : "MM2", inAddress - aProtocol);
		LCD.lcd_printStr(s);
		break;
		
	case LCDMSG_MODE_ACCEDIT:
		LCD.lcd_setCursor(0, 0);
		LCD.lcd_printStr("AccAddr");
		LCD.lcd_setCursor(0, 1);
		
		aProtocol = Gateway.GetLocIDProtocol(inAddress >> 8);
		sprintf(s, "%s %4d", aProtocol == ADDR_ACC_DCC ? "DCC" : "MM2", inAddress - aProtocol);
		LCD.lcd_printStr(s);
		break;		
		
  	
  }
}



word stringToWord(String s) {
  word result = 0;
  
  for (int i = 0; i < s.length(); i++) {
	result = 10 * result + (s.charAt(i) - '0');
  }
  
  return result;
}

boolean parse() {

String request = SerialDS.getRequest();

char aTemp[64];
request.toCharArray(aTemp, 30);

  int lpar = request.indexOf('(');
  if (lpar == -1) {
	return false;
  }
  
  function = String(request.substring(0, lpar));
  function.trim();
  
  int offset = lpar + 1;
  int comma = request.indexOf(',', offset);
  numOfArguments = 0;
  while (comma != -1) {
	String tmp = request.substring(offset, comma);
	tmp.trim();
	arguments[numOfArguments++] = stringToWord(tmp);
	offset = comma + 1;
	comma = request.indexOf(',', offset);
  }

  int rpar = request.indexOf(')', offset);
  while (rpar == -1) {
	return false;
  }
  
  if (rpar > offset) {
	String tmp = request.substring(offset, rpar);
	tmp.trim();
	arguments[numOfArguments++] = stringToWord(tmp);
  }
  
  return true;
}

boolean dispatch() {
  byte aValue;
  byte aValueHigh;
  boolean aResult;

	if (function == "setLocoDirection")
	{
		printMessage(LCDMSG_DIR, arguments[0], arguments[1], 0, 0);
		
		//Apply to Sequence status
		if((Sequence.CurrentProtocol + Sequence.NextLocoAddress) == arguments[0])
		{
			Sequence.NextLocoDirection = arguments[1];
		}
		
		return Gateway.SetLocoDirection(arguments[0], arguments[1]);
	
	}
	else if (function == "setLocoFunction")
	{
		printMessage(LCDMSG_FNC, arguments[0], arguments[1], arguments[2], 0);
		return Gateway.SetLocoFunction(arguments[0], arguments[1], arguments[2]);
	
	}
	else if (function == "setTurnout")
	{
		printMessage(LCDMSG_ACC, arguments[0], arguments[1], 0,0);
		return Gateway.SetTurnout(arguments[0], (byte)arguments[1]);
	
	}
	else if (function == "setPower")
	{
		printMessage(LCDMSG_PWR, 0,arguments[0],0,0);
		
		//Apply to Sequence status
		Sequence.NextPowerStatus = arguments[0];
		
		
		return Gateway.SetPower(arguments[0]);
	
	}
	else if (function == "setLocoSpeed")
	{
		
		printMessage(LCDMSG_SPD, arguments[0], arguments[1],0,0);
		
		//Apply to Sequence status
		if((Sequence.CurrentProtocol + Sequence.NextLocoAddress) == arguments[0])
		{
			Sequence.NextLocoSpeed = arguments[1];
		}
		
		if( numOfArguments > 2)
		{
			return Gateway.SetLocoSpeedEx(arguments[0], arguments[1], arguments[2]);
		}
		else
		{
			return Gateway.SetLocoSpeed(arguments[0], arguments[1]);
		}
	}
	else if (function == "getS88")
	{
		int aMaxS88Num = MAX_S88DECODER;
		
		if( arguments[0] > 0)
		{
			aMaxS88Num = arguments[0];
		}

		reporter.refresh(aMaxS88Num);

		//Send a S88 sensor reply 
		SerialDS.print("@S88,");

		word aFlags = 0;

		for( int j = 0; j < aMaxS88Num; j++)
		{
		  aFlags = (reporter.getByte((j << 1) + 1) << 8) + reporter.getByte(j << 1);
		  
		  SerialDS.printHEX((word)aFlags);
		  SerialDS.print(",");
		}
			
		SerialDS.println("");
		
		return true;
	} /* getS88 */
	else if (function == "reset")
	{
		SerialDS.println("100 Ready");
		return true;
	}
	/* reset */
	else if (function == "setPing")
	{
		SerialDS.println("@DSG,001,");
		return true;
	}
	else if (function == "getLocoConfig")
	{
		/*aResult = ctrl.readConfig(arguments[0], arguments[1], &aValue);*/
		SerialDS.print("@CV,");
		SerialDS.printDEC(arguments[0]);
		SerialDS.print(",");
		SerialDS.printDEC(arguments[1]);
		SerialDS.print(",");
		SerialDS.printDEC(0x00);
		SerialDS.println(",");
		
		return true;
	}
	else if (function == "setLocoConfig")
	{
		return Gateway.WriteConfig(arguments[0], arguments[1], arguments[2]);
	}
	else
	{
		return false;
	}
}

void loop()
{
	
	
	if( (millis() - gPreviousL5) >= 50)
	{
		//Sequence
		Sequence.Interval();
		
		//Reset task
		gPreviousL5 = millis();
		
	}
	
	/* LCD control */
	
	char *aReplyText = RELPYERROR_NONE;
	
	
	if( SerialDS.received())
	{
		
  		if (parse()) {
			if (dispatch()) {
				aReplyText = "200 Ok";
			} else {
				//SerialDS.println(function);
				aReplyText = RELPYERROR_300;
			}
		} else {
			aReplyText = RELPYERROR_301;
		}
		
		/* Reply to Desktop Station */
		SerialDS.println(aReplyText);
		
		SerialDS.clearReceive();
	}
	else
	{
		/* Nothing to do */
		
	}
	
}



void ReplySpeedPacket(word inLocaddress, word inSpeed)
{
	
	/* Locomotive speed */
	SerialDS.print("@SPD,");
	SerialDS.printHEX(highByte(inLocaddress));
	SerialDS.print(",");
	SerialDS.printHEX(lowByte(inLocaddress));
	SerialDS.print(",");
	SerialDS.printHEX(highByte(inSpeed));
	SerialDS.print(",");
	SerialDS.printHEX(lowByte(inSpeed));
	SerialDS.println(",");
}

void ReplyDirPacket(word inLocaddress, byte inDir)
{
	
	/* Locomotive direction */
	SerialDS.print("@DIR,");
	SerialDS.printHEX(highByte(inLocaddress));
	SerialDS.print(",");
	SerialDS.printHEX(lowByte(inLocaddress));
	SerialDS.print(",");
	SerialDS.printHEX(inDir);
	SerialDS.println(",");
}

void ReplyFuncPacket(word inLocaddress, byte inNo, byte inPower)
{
	/* Locomotive functions */
	SerialDS.print("@FNC,");
	SerialDS.printHEX(highByte(inLocaddress));
	SerialDS.print(",");
	SerialDS.printHEX(lowByte(inLocaddress));
	SerialDS.print(",");
	SerialDS.printHEX(inNo);
	SerialDS.print(",");
	SerialDS.printHEX(inPower);
	SerialDS.println(",");
}

void ReplyAccPacket(word inAccaddress, byte inDir)
{
	/* Accessories */
	SerialDS.print("@ACC,");
	SerialDS.printHEX(highByte(inAccaddress));
	SerialDS.print(",");
	SerialDS.printHEX(lowByte(inAccaddress));
	SerialDS.print(",");
	SerialDS.printHEX(inDir);
	SerialDS.println(",");
}

void ReplyPowerPacket(byte inPower)
{
	/* Power */
	SerialDS.print("@PWR,");
	SerialDS.printHEX(inPower);
	SerialDS.println(",");
}


void changedTargetSpeed(const word inAddr, const word inSpeed, const bool inUpdate)
{
	byte aProtocol = 2;
	
	if( inAddr <= 255)
	{
		aProtocol = 0;
	}
	
	if( inUpdate == true)
	{
		Gateway.SetLocoSpeedEx(inAddr, inSpeed, aProtocol);
		ReplySpeedPacket(inAddr, inSpeed);
	}
	
	printMessage(LCDMSG_SPD, inAddr, inSpeed, 0, 0);
}

void changedTargetFunc(const word inAddr, const byte inFuncNo, const byte inFuncPower, const bool inUpdate)
{
	if( inUpdate == true)
	{
		Gateway.SetLocoFunction(inAddr, inFuncNo, inFuncPower);
		ReplyFuncPacket(inAddr, inFuncNo, inFuncPower);
	}
	
	printMessage(LCDMSG_FNC, inAddr, inFuncNo, inFuncPower, 0);

}

void changedTargetDir(const word inAddr, const byte inDir, const bool inUpdate)
{
	if( inUpdate == true)
	{
		Gateway.SetLocoDirection(inAddr, inDir);
		ReplyDirPacket(inAddr, inDir);
	}
	
	printMessage(LCDMSG_DIR, inAddr, inDir, 0, 0);
}

void changedTurnout(const word inAddr, const byte inDir, const bool inUpdate)
{
	if( inUpdate == true)
	{
		Gateway.SetTurnout(inAddr, inDir);
		ReplyAccPacket(inAddr, inDir);
	}
	printMessage(LCDMSG_ACC, inAddr, inDir,0,0);
	
}

void changedPowerStatus(const byte inPower)
{
	printMessage( LCDMSG_PWR, 0, inPower, 0, 0);
	Gateway.SetPower(inPower);
	ReplyPowerPacket(inPower);
}

void debugMessage(const byte inPower)
{
	SerialDS.print("DebugSeq: ");
	SerialDS.printDEC(inPower);
	SerialDS.println("");

}

void displayMessage(const byte inPower)
{
	
	switch(inPower)
	{
	case 0:
		printMessage(LCDMSG_MODE_MM2, 0, 0, 0, 0);
		break;
	case 1:
		printMessage(LCDMSG_MODE_DCC, 0, 0, 0, 0);
		break;
	case 2:
		printMessage(LCDMSG_MODE_CVW, 0, 0, 0, 0);
		break;
	case 3:
		printMessage(LCDMSG_MODE_LOC, 0, 0, 0, 0);
		break;
	case 4:
		printMessage(LCDMSG_MODE_ACC, 0, 0, 0, 0);
		break;
	}
	
}

void displayCVFunc(const byte inCVNo, const byte inCVValue)
{
	printMessage(LCDMSG_MODE_CV, 0, inCVNo, inCVValue, 0);
	
}

void changedCVFunc(const byte inCVNo, const byte inCVValue)
{
	//CV writing message
	printMessage(LCDMSG_MODE_CVWRITING, 0, 0, 0, 0);
	
	//CV write
	Gateway.WriteConfig(ADDR_DCC, inCVNo, inCVValue);
	
	//wait for CV write
	delay(5000);
	
	//Return to CV Edit
	printMessage(LCDMSG_MODE_CV, 0, inCVNo, inCVValue, 0);
	
}

void displayLocEditFunc(const word inAddr, const word inSpeed, const bool inUpdate)
{
	
	//Editing message
	printMessage(LCDMSG_MODE_LOCEDIT, inAddr, inSpeed, 0, 0);
	
	
}

void displayAccEditFunc(const word inAddr, const word inSpeed, const bool inUpdate)
{
	
	//Editing message
	printMessage(LCDMSG_MODE_ACCEDIT, inAddr, inSpeed, 0, 0);
	
	
}
