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
	
	// MM2 Accessory Adr.3 Go Straight
	Gateway.SetTurnout( ADDR_ACC_MM2 + 3, true);
	
	delay(5000);
	
	// MM2 Accessory Adr.3 Turn
	Gateway.SetTurnout( ADDR_ACC_MM2 + 3, false);
	
	delay(5000);
}

