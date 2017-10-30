/*********************************************************************
 * Desktop Station Gateway Sketch for Arduino UNO/Leonard
 *
 * Copyright (C) 2014 Yaasan
 *
 */

#include <avr/wdt.h>
#include <SPI.h>
#include "DSGatewayLib.h"

#define CHANGETIME 5000

DSGatewayLib Gateway;

void setup()
{
	
	Gateway.begin();
	
	delay(5000);
	Gateway.SetPower(true);
}

void loop()
{
	int aSpeed;
	word aLocAddr = ADDR_MM2 + 3;
	
	// Go forward
	Gateway.SetLocoDirection( aLocAddr, 1);
	
	for( int i = 0; i < 10; i++)
	{
		aSpeed =i * 73;
		Gateway.SetLocoSpeed( aLocAddr, aSpeed);
		delay(CHANGETIME);
	}
	
	for( int i = 0; i < 10; i++)
	{
		aSpeed = (10 - i) * 73;
		Gateway.SetLocoSpeed( aLocAddr, aSpeed);
		delay(CHANGETIME);
	}
	
	// Go reverse
	Gateway.SetLocoDirection( aLocAddr, 2);
	
	for( int i = 0; i < 10; i++)
	{
		aSpeed =i * 73;
		Gateway.SetLocoSpeed( aLocAddr, aSpeed);
		delay(CHANGETIME);
	}
	
	for( int i = 0; i < 10; i++)
	{
		aSpeed = (10 - i) * 73;
		Gateway.SetLocoSpeed( aLocAddr, aSpeed);
		delay(CHANGETIME);
	}
	
}

