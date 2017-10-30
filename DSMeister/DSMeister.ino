/*********************************************************************
 * Desktop Station main R5.1 Sketch for Arduino UNO/Leonard
 *
 * Copyright (C) 2017 Yaasan
 *
 */

#include <avr/interrupt.h>
#include <avr/io.h>
#include <EEPROM.h>
#include "TrackReporterS88_DS.h"
#include "DSGatewayLibM.h"
#include "I2CLCDLib.h"
#include "NewSerial.h"
#include "SPI.h"
#include "Wire.h"
#include "MSensor.h"
#include "DSLCDManager.h"
#include "DSSequence.h"
#include "Functions.h"
#include "ThrottleS88.h"

#define MAX_S88DECODER 4

#define RELPYERROR_300 "300 Command error"
#define RELPYERROR_301 "301 Syntax error"
#define RELPYERROR_302 "302 receive timeout"
#define RELPYERROR_303 "303 Unknown error"
#define RELPYERROR_304 "304 Serial error"
#define RELPYERROR_NONE ""

#define MAX_ANALOGINTERVAL 254


#define		SENSOR_CURRENT		A0
#define		SENSOR_VOLTAGE		A7

/* Decralation classes */
TrackReporterS88_DS reporter(MAX_S88DECODER);
DSGatewayLib Gateway;
DSSequence Sequence;
NewSerial SerialDS;
MSensor Sensor(SENSOR_CURRENT, SENSOR_VOLTAGE);
DSLCDManager LCDManager;

ThrottleS88 DSJoy;

/* LCD functions */
I2CLCDLib LCD;

String function;
word arguments[4];
word numOfArguments;

boolean result;

//バッファ
unsigned int gLastJoystickData = 0;


//Task Schedule
unsigned long gPreviousL5 = 0; //   50ms interval
unsigned long gPreviousL6 = 0; //  100ms interval
unsigned long gPreviousL8 = 0; // 1000ms interval
unsigned long gPreviousL9 = 0; // 2000ms interval


//Error check
unsigned char gErrorFlag = 0;
unsigned short gOCLevel = 80; //7.5Amax (**DSmainR5では154d**)



void printSensors();
void ReplySpeedPacket(word inLocaddress, word inSpeed);
void ReplyDirPacket(word inLocaddress, byte inDir);
void ReplyFuncPacket(word inLocaddress, byte inNo, byte inPower);
void ReplyAccPacket(word inAccaddress, byte inDir);
void ReplyPowerPacket(byte inPower);
void printString(const char *s, int x, int y);
void clearDisplay();
byte GetConfigParam(byte inNo);
void updateOCLevel(void);

/***********************************************************************
* 
* Boot setup
* 
* 
************************************************************************/

void setup()
{

	/* LCD init */
	LCD.begin();
	LCD.lcd_setCursor(0, 0);
	LCD.lcd_printStr("DSmain  ");
	LCD.lcd_setCursor(1, 1);
	LCD.lcd_printStr("R5.1 FW4"); 

	/* Clear data of the S88 decoder */
	reporter.refresh();

	/* Debug message for PC */
	SerialDS.Init(SERIAL115K, SERIAL_NONPARITY);
	//SerialDS.Init(SERIAL9600, SERIAL_PARITY);
	
	SerialDS.println("--------------------------------------");
	SerialDS.println("Desktop Station main R5.1 firmware R4 ");
	SerialDS.println("--------------------------------------");
	SerialDS.println("100 Ready");

	
	/* Pin initialization */
	pinMode(8, INPUT);
	pinMode(9, INPUT);
	digitalWrite(8, HIGH); // disable internal pullup
	digitalWrite(9, HIGH); // disable internal pullup

	//Reset task
	gPreviousL5 = millis();
	gPreviousL6 = millis();
	gPreviousL8 = millis();
	
	//Register Event functions
	Sequence.ChangedTargetSpeed = changedTargetSpeed;
	Sequence.ChangedTargetFunc = changedTargetFunc;
	Sequence.ChangedTargetDir = changedTargetDir;
	Sequence.ChangedTurnout = changedTurnout;
	Sequence.ChangedPowerStatus = changedPowerStatus;
	Sequence.DebugMessage = debugMessage;
	Sequence.ChangedMain = changedMain;
	Sequence.ChangedMenu = changedMenu;
	Sequence.ChangedConfiguration = changedConfiguration;
	Sequence.ChangedAcc = changedAcc;
	Sequence.DisplayClear = clearDisplay;
	Sequence.ReadCVFunc = readCVFunc;
	Sequence.WriteCVFunc = writeCVFunc;
	Sequence.ChangedInfo = changedInfo;

	LCDManager.PrintString = printString;
	LCDManager.ClearDisplay = clearDisplay;
	LCDManager.GetConfigData = GetConfigParam;

	/* DSJoy用指令渡し関数 */
	DSJoy.ChangedPowerStatus = changedPowerStatus;
	DSJoy.ChangedTargetSpeed = changedTargetSpeed;
	DSJoy.ChangedTargetDir = changedTargetDir;
	DSJoy.ChangedTargetFunc = changedTargetFunc;
	DSJoy.ChangedTurnout = changedTurnout;
	DSJoy.CurrentProtocol = &Sequence.CurrentProtocol;
	DSJoy.CurrentACCProtocol = &Sequence.CurrentACCProtocol;
	
	/* オフセット調整（終わっていない場合) */
	Sensor.CalcCurrentOffset();
	
	/* センサ */
	//OCレベル変更
	updateOCLevel();
	
	//センサ更新
	gErrorFlag = Sensor.Update(gOCLevel);
	printSensorsSerial();

	
	Gateway.begin();
	
	
	sei();    //ENABLE INTERRUPTION
	
	//wait
	//delay(1000);
	
	//S88 or Joystick setup
	byte aRJ45Mode = Sequence.GetRJ45Mode();
	
	if( aRJ45Mode == RJ45MODE_S88)
	{
		SerialDS.println("RJ45: S88 mode");
		
	}
	else if( aRJ45Mode == RJ45MODE_DSJ)
	{
		SerialDS.println("RJ45: DSJ mode");
	}
	else
	{
		
		
	}
	
	//Clear Display
	LCDManager.RegisterLocDirection(Sequence.CurrentProtocol + Sequence.NextLocoAddress, Sequence.NextLocoDirection);
	LCDManager.RegisterMain();
	LCD.lcd_clear();
	LCDManager.Interval();
	

}

void printSensorsSerial()
{
	char aText[16];
	
	unsigned char aCurrent = Sensor.GetCurrent();
	unsigned char aVoltage = Sensor.GetVoltage();
	
	unsigned char aCurrent_H = aCurrent / 10;
	unsigned char aCurrent_L = aCurrent - (aCurrent_H * 10);
	unsigned char aVoltage_H = aVoltage / 10;
	unsigned char aVoltage_L = aVoltage - (aVoltage_H * 10);
	
	sprintf(aText, "%d.%dA %d.%dV", aCurrent_H, aCurrent_L, aVoltage_H, aVoltage_L);
	SerialDS.print("SENSORS: ");
	SerialDS.println(aText);
}

void printSensors()
{
	char aText[16];
	unsigned char aFlagDisplay = gErrorFlag;
	
	unsigned char aCurrent = Sensor.GetCurrent();
	unsigned char aVoltage = Sensor.GetVoltage();
	
	unsigned char aCurrent_H = aCurrent / 10;
	unsigned char aCurrent_L = aCurrent - (aCurrent_H * 10);
	unsigned char aVoltage_H = aVoltage / 10;
	unsigned char aVoltage_L = aVoltage - (aVoltage_H * 10);
	
	if( (millis() - gPreviousL9) >= 2000)
	{
		
		//Reset task
		gPreviousL9 = millis();
	}
	
	if( (LCDManager.GetMode() == MODE_DISP_INF) || (aFlagDisplay != 0x00))
	{
		//情報表示の時は更新する
		LCDManager.SetSensors(aCurrent_H, aCurrent_L, aVoltage_H, aVoltage_L, aFlagDisplay);
	}
	
}


boolean parse() 
{

String request = SerialDS.getRequest();

char aTemp[48];
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

boolean dispatch() 
{
	byte aValue;
	byte aValueHigh;
	boolean aResult;
	char aTempStr[16];
	word aLocProtocol;
	byte aPwr;
	boolean aRet;
	
	if ((function == "setLocoDirection") || ((function == "sLD")))
	{
		
		//Apply to Sequence status
		if((Sequence.CurrentProtocol + Sequence.NextLocoAddress) == arguments[0])
		{
			LCDManager.RegisterLocDirection(arguments[0], arguments[1]);
			Sequence.NextLocoDirection = arguments[1];
		}
		else
		{
			aLocProtocol = Gateway.GetLocIDProtocol(arguments[0] >> 8);
			sprintf(aTempStr, "L%4d%s %s",arguments[0] - aLocProtocol,aLocProtocol == ADDR_DCC ? "D":"M", arguments[1]==1?"FW":"RE");
			LCDManager.AddMessages(aTempStr);
		}
		
		return Gateway.SetLocoDirection(arguments[0], arguments[1]);
	
	}
	else if ((function == "setLocoFunction") || ((function == "sLF")))
	{
		//LCDManager.RegisterLocFunction(arguments[0], arguments[1], arguments[2]);
		aLocProtocol = Gateway.GetLocIDProtocol(arguments[0] >> 8);
		
		sprintf(aTempStr, "L%4d%s F%d%s", arguments[0] - aLocProtocol,aLocProtocol == ADDR_DCC ? "D":"M", arguments[1], arguments[2]==1?"ON ":"OFF");
		LCDManager.AddMessages(aTempStr);
		
		return Gateway.SetLocoFunction(arguments[0], arguments[1], arguments[2]);
	
	}
	else if (function == "setTurnout")
	{
		LCDManager.RegisterTurnout(arguments[0], arguments[1]);
		
		return Gateway.SetTurnout(arguments[0], (byte)arguments[1]);
		
	}
	else if (function == "sTO")
	{
		aRet = false;
		
		// ‭1100011111111110‬
		//  0011100000000001‬
		
		word aNotAddr = 0xFFFF -arguments[2];
		aPwr = 1 - arguments[3];
		
		if( (aNotAddr == arguments[0]) && (aPwr == arguments[1]))
		{
			//SerialDS.println("Check OK.");
			
			LCDManager.RegisterTurnout(arguments[0], arguments[1]);
			
			aRet = Gateway.SetTurnout(arguments[0], (byte)arguments[1]);
		}
		else
		{
			//SerialDS.println("Check Error!");
			/* エラーチェック */
			aRet = false;
		}
		
		return aRet;
		
	}
	else if (function == "sPW")
	{
		aPwr = 1 - arguments[1];
		
		if( aPwr == arguments[0])
		{
			
			LCDManager.RegisterPower(arguments[0]);
			
			//Apply to Sequence status
			Sequence.NextPowerStatus = arguments[0];
			
			aRet = Gateway.SetPower(arguments[0]);
			
			
		}
		else
		{
			aRet = false;
		}
		
		return aRet;
	
	}
	else if (function == "setPower")
	{
		LCDManager.RegisterPower(arguments[0]);
		
		//Apply to Sequence status
		Sequence.NextPowerStatus = arguments[0];
		
		if( (gErrorFlag > 0) && (arguments[0] == POWER_ON))
		{
			gErrorFlag = 0;
		}
		
		return Gateway.SetPower(arguments[0]);
	
	}
	else if ((function == "setLocoSpeed") || ((function == "sLS")))
	{
		
		
		//Apply to Sequence status
		if((Sequence.CurrentProtocol + Sequence.NextLocoAddress) == arguments[0])
		{
			LCDManager.RegisterLocSpeed(arguments[0], arguments[1]);
			Sequence.NextLocoSpeed = arguments[1];
		}
		else
		{
			aLocProtocol = Gateway.GetLocIDProtocol(arguments[0] >> 8);
			
			sprintf(aTempStr, "L%4d%s %d", arguments[0] - aLocProtocol,aLocProtocol == ADDR_DCC ? "D":"M", (arguments[1] >> 3) * 100 >> 7);
			LCDManager.AddMessages(aTempStr);
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
	else if (function == "setDCCLocoFunction")
	{
		//LCDManager.RegisterLocFunction(arguments[0] + ADDR_DCC, arguments[1], arguments[2]);
		sprintf(aTempStr, "L%4d%sF%d%s", arguments[0], "D", arguments[1], arguments[2]==1?"ON ":"OFF");
		LCDManager.AddMessages(aTempStr);
		
		return Gateway.SetLocoFunction(arguments[0] + Sequence.CurrentProtocol, arguments[1], arguments[2]);
	
	}
	else if (function == "setDCCLocoDirection")
	{
		
		//Apply to Sequence status
		if((Sequence.CurrentProtocol + Sequence.NextLocoAddress) == arguments[0] + Sequence.CurrentProtocol)
		{
			LCDManager.RegisterLocDirection(arguments[0] + Sequence.CurrentProtocol, arguments[1]);
			Sequence.NextLocoDirection = arguments[1];
		}
		else
		{
			sprintf(aTempStr, "L%4dD %s",arguments[0], arguments[1]==1?"FWD":"REV");
			LCDManager.AddMessages(aTempStr);
		}
		
		return Gateway.SetLocoDirection(arguments[0] + Sequence.CurrentProtocol, arguments[1]);
	
	}
	else if (function == "setDCCLocoSpeed")
	{
		
		
		//Apply to Sequence status
		if((Sequence.CurrentProtocol + Sequence.NextLocoAddress) == arguments[0] + Sequence.CurrentProtocol)
		{
			LCDManager.RegisterLocSpeed(arguments[0] + Sequence.CurrentProtocol, arguments[1]);
			Sequence.NextLocoSpeed = arguments[1];
		}
		else
		{
			aLocProtocol = Gateway.GetLocIDProtocol(arguments[0] >> 8);
			
			sprintf(aTempStr, "L%4d%s %d", arguments[0], "D", (arguments[1] >> 3) * 100 >> 7, 37);
			LCDManager.AddMessages(aTempStr);
		}
		
		if( numOfArguments > 2)
		{
			return Gateway.SetLocoSpeedEx(arguments[0] + Sequence.CurrentProtocol, arguments[1], arguments[2]);
		}
		else
		{
			return Gateway.SetLocoSpeed(arguments[0] + Sequence.CurrentProtocol, arguments[1]);
		}
	}
	else if (function == "setDCCLocoAddr")
	{
		Sequence.NextLocoAddress = arguments[0];
		LCDManager.RegisterLoc(Sequence.CurrentProtocol + Sequence.NextLocoAddress);
	}
	else if (function == "getS88")
	{
		int aMaxS88Num = MAX_S88DECODER;
		
		if( arguments[0] > 0)
		{
			aMaxS88Num = arguments[0];
		}
		
		//Send a S88 sensor reply 
		SerialDS.print("@S88,");
		
		if( Sequence.GetRJ45Mode() == RJ45MODE_S88)
		{

			reporter.refresh(aMaxS88Num);

			word aFlags = 0;

			for( int j = 0; j < aMaxS88Num; j++)
			{
			  aFlags = (reporter.getByte((j << 1) + 1) << 8) + reporter.getByte(j << 1);
			  
			  SerialDS.printHEX((word)aFlags);
			  SerialDS.print(",");
			}
		}
			
		SerialDS.println("");
		
		return true;
	} /* getS88 */
	else if ((function == "reset") || ((function == "sRS")))
	{
		SerialDS.println("100 Ready");
		return true;
	}
	/* reset */
	else if ((function == "setPing") || ((function == "sPI")))
	{
		SerialDS.println("@DSG,001,");
		//Sensor.Update(gOCLevel);
		printSensorsSerial();
		return true;
	}
	else if ((function == "getLocoConfig") || ((function == "gCV")))
	{
		aValue = 0;

		sprintf(aTempStr, "CVRead  Adr=%03d", arguments[1]);
		LCDManager.AddMessages(aTempStr);
		LCDManager.Interval();
		
		aResult = Gateway.ReadConfig(arguments[0], arguments[1], &aValue);
		
		//Result
		if( aResult != true)
		{
			sprintf(aTempStr, "---->Read Error", aValue);
		}
		else
		{
			sprintf(aTempStr, "---->%03d (0x%02X)", aValue, aValue);
		}
		LCDManager.AddMessages(aTempStr);

		SerialDS.print("@CV,");
		SerialDS.printDEC(arguments[0]);
		SerialDS.print(",");
		SerialDS.printDEC(arguments[1]);
		SerialDS.print(",");
		
		if( aResult == true)
		{
			SerialDS.printDEC(aValue);
		}
		else
		{
			SerialDS.printDEC(0xFFFF);
		}
		
		SerialDS.println(",");
		
		return true;
	}
	else if ((function == "setLocoConfig") || ((function == "sCV")))
	{
		sprintf(aTempStr, "CVWrite Adr=%03d", arguments[1]);
		LCDManager.AddMessages(aTempStr);
		sprintf(aTempStr, "<----%03d (0x%02x)", arguments[2], arguments[2]);
		LCDManager.AddMessages(aTempStr);
		
		if( numOfArguments > 3)
		{		
			return Gateway.WriteConfigEx(arguments[0], arguments[1], arguments[2], arguments[3]);
		}
		else
		{
			return Gateway.WriteConfig(arguments[0], arguments[1], arguments[2]);
		}
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
		
		//Monitor Current
		Sensor.MonitorCurrent();
		
		//Reset task
		gPreviousL5 = millis();
		
	}

	if( (millis() - gPreviousL6) >= 100)
	{
		if( Sequence.GetRJ45Mode() == RJ45MODE_DSJ)
		{
			// S88  
			reporter.refresh(MAX_S88DECODER);
			
			// データ処理 
			
			for( int i = 0; i < MAX_S88DECODER; i++)
			{
				uint16_t aData = (reporter.getByte(i * 2 + 1) << 8) + reporter.getByte(i * 2 + 0);
				if(i == 0)
				{
					//Serial.print("s88_raw:");
					//Serial.println(aData,BIN);
				}
				
				if( aData != 0xFFFF)
				{
					// S88データをジョイスティックプロファイルに読み替え処理 
					DSJoy.SetData(i, aData);
				}
				else
				{
					break;
				}
			}
		}
		
		//Reset task
		gPreviousL6 = millis();
	}

	if( (millis() - gPreviousL8) >= 500)
	{
		/* オフセット調整（終わっていない場合) */
		//if( Gateway.IsPower() == false)
		//{
		//	Sensor.CalcCurrentOffset();
		//}
	
		/* Sensor updates */
		unsigned char aErrorFlag = Sensor.Update(gOCLevel);
		
		if( (gErrorFlag & aErrorFlag) > 0)
		{
			
			//電源供給中の時
			if( Sequence.NextPowerStatus > 0)
			{
				Sequence.NextPowerStatus = 0;
				Gateway.SetPower(0);
				
				LCDManager.SetSensorsErr(Sensor.GetCurrent(), Sensor.GetVoltage());
				
				Sequence.NextMode(SQCMD_DEPTH_INFO);
				LCDManager.RegisterInfo();
				
				/* エラーを表示する */
				printSensorsSerial();
				
			}
			
		}
		else
		{
			
		}
		
		gErrorFlag |= aErrorFlag;
		
		printSensors();
		/* 表示 */
		
		LCDManager.Interval();

		//Reset task
		gPreviousL8 = millis();
	}
	
	
	char *aReplyText = RELPYERROR_NONE;
	
	
	if( SerialDS.received())
	{
		if( SerialDS.IsError() == 0)
		{
			if ( parse() ) 
			{
				if (dispatch())
				{
					aReplyText = "200 Ok";
				}
				else
				{
					/* コマンドエラー */
					//SerialDS.println(function);
					aReplyText = RELPYERROR_300;
				}
	  			
	  			/* 表示を更新 */
				LCDManager.Interval();
	  			
			}
			else
			{
				/* 構文エラー */
				aReplyText = RELPYERROR_301;
			}
		}
		else
		{
			/* シリアル通信エラー*/
			aReplyText = RELPYERROR_304;
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
	
	LCDManager.RegisterLocSpeed(inAddr, inSpeed);
	LCDManager.Interval();
}

void changedTargetFunc(const word inAddr, const byte inFuncNo, const byte inFuncPower, const bool inUpdate)
{
	if( inUpdate == true)
	{
		Gateway.SetLocoFunction(inAddr, inFuncNo, inFuncPower);
		ReplyFuncPacket(inAddr, inFuncNo, inFuncPower);
	}
	
	LCDManager.RegisterLocFunction(inAddr, inFuncNo, inFuncPower);
	LCDManager.Interval();

}

void changedTargetDir(const word inAddr, const byte inDir, const bool inUpdate)
{
	
	if( inUpdate == true)
	{
		Gateway.SetLocoDirection(inAddr, inDir);
		ReplyDirPacket(inAddr, inDir);
	}
	
	LCDManager.RegisterLocDirection(inAddr, inDir);
	LCDManager.Interval();
}

void changedTurnout(const word inAddr, const byte inDir, const bool inUpdate)
{
	if( inUpdate == true)
	{
		Gateway.SetTurnout(inAddr, inDir);
		ReplyAccPacket(inAddr, inDir);
	}
	
	LCDManager.RegisterTurnout(inAddr, inDir);
	LCDManager.Interval();
	
}

void changedPowerStatus(const byte inPower)
{
	LCDManager.RegisterPower(inPower);
	LCDManager.Interval();
	
	Gateway.SetPower(inPower);
	ReplyPowerPacket(inPower);
	
	if( (gErrorFlag > 0) && (inPower == POWER_ON))
	{
		gErrorFlag = 0;
	}
	
}


void changedMain()
{
	LCDManager.WriteLocAddr(Sequence.CurrentProtocol + Sequence.NextLocoAddress);
	LCDManager.WriteAccAddr(Sequence.CurrentACCProtocol + Sequence.NextAccAddress);
	
	
	LCDManager.RegisterMain();
	LCDManager.Interval();
}

void changedInfo()
{
	LCDManager.RegisterInfo();
	LCDManager.Interval();
}


void changedMenu(byte inMenuNo)
{
	LCDManager.RegisterMenu(inMenuNo);
	LCDManager.Interval();
}

void changedConfiguration(byte inMenuNo)
{
	LCDManager.RegisterConfig(inMenuNo);
	LCDManager.Interval();
	
	//OCレベル変更
	updateOCLevel();
}

void updateOCLevel(void)
{
	gOCLevel = (uint16_t)Sequence.GetOCLVMode() * 10;
	
}

void changedAcc(const word inAddr, const byte inDir, const bool inUpdate)
{
	if( inUpdate == true)
	{
		//操作
		Gateway.SetTurnout(inAddr, inDir);
	}
	
	
	LCDManager.RegisterAcc(inAddr, inDir);
	LCDManager.Interval();
}

void readCVFunc(const byte inSeq, const word inCVAddr, const byte inCVValue)
{
	byte aValue = 0;
	bool aResult;
	
	if( inSeq == 1)
	{
		LCDManager.RegisterCVRead(10, inCVAddr, 0);
		LCDManager.Interval();
		
		aValue = 0;
		aResult = Gateway.ReadConfig(ADDR_DCC, inCVAddr, &aValue);
		
		SerialDS.print("CV Read: Adr.");
		SerialDS.printDEC(inCVAddr);
		SerialDS.print(", Val.");
		SerialDS.printDEC(aValue);
		SerialDS.println("");
		
		//Result
		if( aResult != true)
		{
			LCDManager.RegisterCVRead(11, inCVAddr, aValue);
		}
		else
		{
			LCDManager.RegisterCVRead(1, inCVAddr, aValue);
		}
		
		LCDManager.Interval();
	}
	else
	{
		LCDManager.RegisterCVRead(inSeq, inCVAddr, 0);
		LCDManager.Interval();
	}

	
}

void writeCVFunc(const byte inSeq, const word inCVAddr, const byte inCVValue)
{
	unsigned long aBaseTime = 0;
	word aCurOffset;
	bool aSuccessWrite = false;
	int aCurrent;
	
	if( inSeq == 2)
	{
		LCDManager.RegisterCVWrite(20, inCVAddr, inCVValue);
		LCDManager.Interval();
		
		
		Gateway.WriteConfig(ADDR_DCC, inCVAddr, inCVValue);
		
		//電流オフセットを取得
		Sensor.CalcCurrentOffset();
		aCurOffset = Sensor.GetCurrentOffset();
		
		//5secのタイムアウト or 電流 60mA待ち
		aBaseTime = millis();
		
		while((millis() - gPreviousL5) < 5000)
		{
			aCurrent = (int)analogRead(SENSOR_CURRENT) - (int)aCurOffset;
			
			//SerialDS.printDEC(aCurrent);
			//SerialDS.print(",");
			
			if( aCurrent > 10)
			{
				//成功
				aSuccessWrite = true;
				
				break;
			}
			
			
		}
		//delay(2000);
		
		
		LCDManager.RegisterCVWrite(inSeq, aSuccessWrite == true ? inCVAddr : 0xFFFF, inCVValue);
		LCDManager.Interval();
		
	}
	else
	{
		LCDManager.RegisterCVWrite(inSeq, inCVAddr, inCVValue);
		LCDManager.Interval();
	}
	
}

void debugMessage(const byte inPower)
{
	SerialDS.print("DebugSeq: ");
	SerialDS.printDEC(inPower);
	SerialDS.println("");

}

void printString(const char *s, int x, int y)
{
	LCD.lcd_setCursor(x, y);
	LCD.lcd_printStr(s);
}

void clearDisplay()
{
	LCD.lcd_clear();           //clear the screen and set start position to top left corner

}

byte GetConfigParam(byte inNo)
{
	byte aRet = 0;
	
	switch(inNo)
	{
	case 0:
		//S88
		aRet = Sequence.GetRJ45Mode();
		break;
	case 1:
		//OC Level
		aRet = Sequence.GetOCLVMode();
		break;		
		
	default:
		aRet = 0;
		break;
	}
	
	return aRet;
	
}

