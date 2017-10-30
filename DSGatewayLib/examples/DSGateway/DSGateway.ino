/*********************************************************************
 * Desktop Station Gateway Sketch for Arduino UNO/Leonard
 *
 * Copyright (C) 2014 Yaasan
 *
 */

#include <avr/wdt.h>
#include "TrackReporterS88_DS.h"
#include "DSGatewayLib.h"
#include "SPI.h"


#define MAX_S88DECODER 1
#define RELPYERROR_300 "300 Command error"
#define RELPYERROR_301 "301 Syntax error"
#define RELPYERROR_302 "302 receive timeout"
#define RELPYERROR_303 "303 Unknown error"
#define RELPYERROR_NONE ""

TrackReporterS88_DS reporter(MAX_S88DECODER);
DSGatewayLib Gateway;


String request;

String function;

word arguments[8];

word numOfArguments;

boolean result;

void setup() {
  
  
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
  
  
  Gateway.begin();
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
	return Gateway.SetLocoDirection(arguments[0], arguments[1]);
	
  } else if (function == "setLocoFunction") {
	return Gateway.SetLocoFunction(arguments[0], arguments[1], arguments[2]);
	
  } else if (function == "setTurnout") {
	return Gateway.SetTurnout(arguments[0], (byte)arguments[1]);
	
  } else if (function == "setPower") {
	return Gateway.SetPower(arguments[0]);
	
  } else if (function == "setLocoSpeed") {
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
	
	
	/* Reply to Desktop Station */
	if( aReplyText != "")
	{
		Serial.println(aReplyText);
	}

}

