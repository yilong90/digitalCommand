/*********************************************************************
 * Desktop Station Gateway Sketch for Arduino UNO/Leonard
 *
 * Copyright (C) 2014 Yaasan
 *
 */

#include "DSGatewayLib.h"
#include "I2CLCDLIB.h"
#include "DSMAnalogIn.h"
#include "SPI.h"
#include "Wire.h"



#define MAX_ANALOGINTERVAL 254

#define LCDMSG_PWR 10
#define LCDMSG_DIR 20
#define LCDMSG_SPD 30
#define LCDMSG_ACC 40
#define LCDMSG_FNC 50

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
word gCurrentLocAddr = 3;
word gCurrentProtocol = ADDR_MM2;
byte gCurrentDirection = 1;
byte gLastAnalogCounter = 0;

boolean result;

void printMessage(byte inSpeed, word inAddress, byte inDir);
void ProcessAnalogInput();

void setup() {
  
  LCD.begin();
 
  Serial.begin(115200);
  while (!Serial);

  Serial.println("--------------------------------------");
  Serial.println("Desktop Station Gateway               ");
  Serial.println("--------------------------------------");
  Serial.println("100 Ready");
  
  
  pinMode(8, INPUT);
  pinMode(9, INPUT);
  digitalWrite(8, HIGH); // enable internal pullup
  digitalWrite(9, HIGH); // enable internal pullup
  
	
	/* Change Locomotive protocol */
	if( digitalRead(8) == LOW)
	{
		delay(300);
		gCurrentProtocol = ADDR_DCC;
	}

  pinMode(A4, OUTPUT);
   pinMode(A5, OUTPUT);
  
  /* Mascon I/F */
  AnalogIn.begin();

  Gateway.begin();
  
  delay(1000);
  
  printMessage( 0, gCurrentLocAddr, 1);
  /* Power On */
  Gateway.SetPower(true);
  
}

void printMessage(byte inSpeed, word inAddress, byte inDir)
{
  char s[16];
  
  LCD.lcd_clear();
  LCD.lcd_setCursor(0, 0);

  sprintf(s, "LOC%3d %s", inAddress, gCurrentProtocol == ADDR_DCC ? "D" : "M");
  LCD.lcd_printStr(s);
  LCD.lcd_setCursor(0, 1);
  sprintf(s, "SPD%3d%c%s", ((word)inSpeed * 100) >> 7, 37, inDir <= 1 ? "F" : "R");
  LCD.lcd_printStr(s);   
 
}

void loop() {

        /* Loc Select */
        if( digitalRead(8) == LOW)
        {
          delay(300);
          gCurrentLocAddr++;
          
          printMessage( 0, gCurrentLocAddr, 1);
          
        }
        
        if( digitalRead(9) == LOW)
        {
          delay(300);
          gCurrentLocAddr--;
          
          if( (gCurrentLocAddr) <= 0)
          {
            gCurrentLocAddr = 1;
          }
          else
          {
            printMessage( 0, gCurrentLocAddr, 1);
          }
          
        }        
	
	/* Analog input */
	ProcessAnalogInput();	
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
			Gateway.SetLocoSpeed(gCurrentProtocol + gCurrentLocAddr, aSpeed);

			Serial.print("LOC=");
			Serial.print(gCurrentProtocol + gCurrentLocAddr, DEC);
			Serial.print(", SPD=");
			Serial.println(aSpeed, DEC);
			
			byte aDirection = AnalogIn.GetSpeedDirection();
			Serial.print(", DIR=");
			Serial.println(aDirection, DEC);
			
			if( (gCurrentDirection != aDirection) && (gLastAnalogValue <= 64))
			{
				gCurrentDirection = aDirection;
				Gateway.SetLocoDirection(gCurrentProtocol + gCurrentLocAddr, aDirection);
			}

                        /* Print message */
			printMessage( (byte)(aSpeed >> 3), gCurrentLocAddr, gCurrentDirection);
		}
	}
}


