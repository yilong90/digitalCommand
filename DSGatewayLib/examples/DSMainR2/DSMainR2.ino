/*********************************************************************
 * Desktop Station Gateway Sketch for Arduino UNO/Leonard
 *
 * Copyright (C) 2014 Yaasan
 *
 */

#include <avr/wdt.h>
#include "TrackReporterS88_DS.h"
#include "DSGatewayLib.h"
#include "DSMAnalogIn.h"
#include "I2CLCDLIB.h"
#include "SPI.h"
#include "Wire.h" 


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

TrackReporterS88_DS reporter(MAX_S88DECODER);
DSGatewayLib Gateway;
I2CLCDLib LCD;
DSMAnalogIn AnalogIn;

String request;
String function;
word arguments[8];
word numOfArguments;

//Analog input paramters
word gLastAnalogValue = 0xFF;
byte gLastDir = 0xFF;
word gCurrentLocAddr = ADDR_MM2 + 3;
byte gCurrentDirection = 1;
byte gLastAnalogCounter = 0;

boolean result;

void printMessage(byte inNo, word inAddress, word inValue1, byte inValue2, byte inValue3);
void ProcessAnalogInput();
void ReplySpeedPacket(word inLocaddress, word inSpeed);
void ReplyDirPacket(word inLocaddress, byte inDir);
word LPF(word inValue, word inGain, word *iopBuf);

void setup() {
  
  LCD.begin();
  
  LCD.lcd_setCursor(0, 0);
  LCD.lcd_printStr("WELCOME");
  LCD.lcd_setCursor(1, 1);
  LCD.lcd_printStr("ABOARD!"); 
  //delay(1000);
  
  Serial.begin(115200);
  while (!Serial);
  
  wdt_enable(WDTO_8S); // WDT reset
  
  /* Clear data of the S88 decoder */
  reporter.refresh();


  Serial.println("--------------------------------------");
  Serial.println("Desktop Station Gateway               ");
  Serial.println("--------------------------------------");
  Serial.println("100 Ready");
  
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  
  /* Mascon I/F */
  pinMode(6, INPUT);
  pinMode(A1, INPUT);
  digitalWrite(A1, LOW); // disable internal pullup
  analogReference(DEFAULT); // set analog reference 5V(VCC) 

  Gateway.begin();
}

void printMessage(byte inNo, word inAddress, word inValue1, byte inValue2, byte inValue3)
{
  char s[16];
  word aACCProtocol;
  
  LCD.lcd_clear();
  LCD.lcd_setCursor(0, 0);
  
  
  if( inNo != 10)
  {
    word aProtocol = Gateway.GetLocIDProtocol(inAddress >> 8);
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
      LCD.lcd_setCursor(0, 1);
      sprintf(s, "ACC%3d %s", inAddress - aACCProtocol, inValue1 == 0 ? "R" : "G");
      LCD.lcd_printStr(s);    
   break;
   
     case 50:
      LCD.lcd_setCursor(0, 1);
      sprintf(s, "FNC%2d %s", inValue1 + 1, inValue2 == 0 ? "OF" : "ON" );
      LCD.lcd_printStr(s);
   break;
    
  }
}

int receiveRequest() {
	char buffer[128];
	int i = 0;
	char aByte;
	int aResult = -1;
	
	unsigned long time = millis();
	
	/* Check serial buffer */
	if (Serial.available() > 0) {
		
		while (1) {
		
			if (Serial.available() > 0) {
			
				aByte = Serial.read();
				
				// Write to text buf.
				buffer[i] = aByte;
				
				// check last identification of text end.
				if (aByte == '\n') {
					aResult = i;
					break;
				}
				
				// increment
				i = i + 1;
				
				// buf over check
				if( i > 127)
				{
					break;
				}
			}
			else
			{
				/* Check timeout. */
				if( millis() > time + 4000)
				{
					aResult = -1;
					break;
				}
			}
		}
	}
	else
	{
		/* Not received. */
		aResult = 0;
	}
	
	if( aResult > 0)
	{
	
		buffer[i] = '\0';
		
		request = String(buffer);
	}
	else
	{
		request = "";
	}
	
	return aResult;
}

word stringToWord(String s) {
  word result = 0;
  
  for (int i = 0; i < s.length(); i++) {
	result = 10 * result + (s.charAt(i) - '0');
  }
  
  return result;
}

boolean parse() {
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

	if (function == "setLocoDirection") {
		printMessage(LCDMSG_DIR, arguments[0], arguments[1],0,0);
		gCurrentLocAddr = arguments[0];// set current loc address
		return Gateway.SetLocoDirection(arguments[0], arguments[1]);
	
	} else if (function == "setLocoFunction") {
		printMessage(LCDMSG_FNC, arguments[0], arguments[1], arguments[2], 0);
		gCurrentLocAddr = arguments[0];// set current loc address
		return Gateway.SetLocoFunction(arguments[0], arguments[1], arguments[2]);
	
	} else if (function == "setTurnout") {
		printMessage(LCDMSG_ACC, arguments[0], arguments[1], 0,0);
		return Gateway.SetTurnout(arguments[0], (byte)arguments[1]);
	
  } else if (function == "setPower") {
		printMessage(LCDMSG_PWR, 0,arguments[0],0,0);
		return Gateway.SetPower(arguments[0]);
	
  } else if (function == "setLocoSpeed") {
		printMessage(LCDMSG_SPD, arguments[0], arguments[1],0,0);
		gCurrentLocAddr = arguments[0];// set current loc address
		
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
	Serial.print("@S88,");

	word aFlags = 0;

	for( int j = 0; j < aMaxS88Num; j++)
	{
	  aFlags = (reporter.getByte((j << 1) + 1) << 8) + reporter.getByte(j << 1);
	  
	  Serial.print(aFlags, HEX);
	  Serial.print(",");
	}
		
	Serial.println("");
	
    return true;
	
  } /* getS88 */
  else if (function == "reset")
  {
  
	Serial.println("100 Ready");
	
	return true;
  } /* reset */
  else if (function == "setPing") {
      Serial.println("@DSG,001,");
	return true;
  } else if (function == "getLocoConfig") {
  
	/*aResult = ctrl.readConfig(arguments[0], arguments[1], &aValue);*/
	Serial.print("@CV,");
	Serial.print(arguments[0], DEC);
	Serial.print(",");
	Serial.print(arguments[1], DEC);
	Serial.print(",");
	Serial.print(0x00, DEC);
	Serial.println(",");
	
	return true;
  } else if (function == "setLocoConfig") {
	return Gateway.WriteConfig(arguments[0], arguments[1], arguments[2]);
	
  }
  else
  {
	return false;
  }
  
}

void loop() {
	
	String aReplyText = RELPYERROR_NONE;
	
	/* Reset watchdog timer. */
	wdt_reset(); 

	//Receive command from Desktop Station(PC via USB)
	int aReceived = receiveRequest();
	
	/* Reset watchdog timer. */
	wdt_reset();
	
	if( aReceived > 0)
	{
  		if (parse()) {
			if (dispatch()) {
				aReplyText = "200 Ok";
			} else {
				//Serial.println(function);
				aReplyText = RELPYERROR_300;
			}
		} else {
			aReplyText = RELPYERROR_301;
		}
	}
	else if( aReceived == 0)
	{
		aReplyText = "";
		
	}
	else if( aReceived < 0)
	{
		// Not received per 4sec
		// Reset serial
		Serial.flush();
		
		aReplyText = RELPYERROR_302;
        }
	else
	{
		/* Nothing to do */
		
	}
	
	
	/* Analog input */
	ProcessAnalogInput();
	
	/* Reply to Desktop Station */
	if( aReplyText != "")
	{
		Serial.println(aReplyText);
	}
	
	
}



void ProcessAnalogInput()
{
	if( Gateway.IsPower() == false)
	{
		return;
	}

	if( gLastAnalogCounter >= MAX_ANALOGINTERVAL)
	{
		gLastAnalogCounter = 0;
	}
	else
	{
		gLastAnalogCounter++;
		return;
	}

	word aSpeed;

	aSpeed = AnalogIn.GetSpeedInputPulse();

	if( (gLastAnalogValue == 0xFF) && (aSpeed == 0))
	{
	  /* Not connected an analog device */
	}
	else
	{
		/* Connected an analog device */
		if( gLastAnalogValue != aSpeed)
		{
  
 			/* Changed loc speed */
			gLastAnalogValue = aSpeed;
			
			/* Send the speed command */
			Gateway.SetLocoSpeed(gCurrentLocAddr, aSpeed);
			
			/* Reply to PC */
			ReplySpeedPacket(gCurrentLocAddr, aSpeed);
			
			byte aDirection = AnalogIn.GetSpeedDirection();
			
			if( (gCurrentDirection != aDirection) && (gLastAnalogValue <= 64))
			{
				gCurrentDirection = aDirection;
				Gateway.SetLocoDirection(gCurrentLocAddr, aDirection);
				
				/* Reply to PC */
				ReplyDirPacket(gCurrentLocAddr, gCurrentDirection);
				
			}

			printMessage(LCDMSG_SPD, gCurrentLocAddr, aSpeed, 0, 0);

		}
	}
}

void ReplySpeedPacket(word inLocaddress, word inSpeed)
{
	
	/* Locomotive speed */
	Serial.print("@SPD,");
	Serial.print(highByte(inLocaddress), HEX);
	Serial.print(",");
	Serial.print(lowByte(inLocaddress), HEX);
	Serial.print(",");
	Serial.print(highByte(inSpeed), HEX);
	Serial.print(",");
	Serial.print(lowByte(inSpeed), HEX);
	Serial.println(",");
}

void ReplyDirPacket(word inLocaddress, byte inDir)
{
	
	/* Locomotive direction */
	Serial.print("@DIR,");
	Serial.print(highByte(inLocaddress), HEX);
	Serial.print(",");
	Serial.print(lowByte(inLocaddress), HEX);
	Serial.print(",");
	Serial.print(inDir, HEX);
	Serial.println(",");
}



