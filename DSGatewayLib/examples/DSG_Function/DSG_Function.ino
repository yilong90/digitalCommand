/*********************************************************************
 * Desktop Station Sketch for Arduino UNO/Leonard
 *
 * Copyright (C) 2014 Yaasan
 *
 */

#include <avr/wdt.h>
#include <SPI.h>
#include "DSGatewayLib.h"

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
	
	// Headlight on
	Gateway.SetLocoFunction( aLocAddr, 0, 1);
	
	delay(5000);
	
	// Headlight off
	Gateway.SetLocoFunction( aLocAddr, 0, 0);
	
	delay(5000);	
}

