/*********************************************************************
 * Desktop Station Gateway Library for Arduino UNO/Leonard
 *
 * Copyright (C) 2014 Yaasan
 *
 */

#include <avr/wdt.h>
#include <SPI.h>
#include "DSGatewayLibM.h"

DSGatewayLib::DSGatewayLib()
{

  pinMode(SPI_SSPIN, OUTPUT);
  digitalWrite(SPI_SSPIN, HIGH);
	
	poweron = false;
	
}

void DSGatewayLib::begin()
{
  /* Open SPI */

  SPI.begin();
  SPI.setBitOrder(LSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV64); /* 500kHz */
  SPI.setDataMode(SPI_MODE0);

}

DSGatewayLib::~DSGatewayLib()
{
  
  SetPower(false);

  SPI.end();


}

boolean DSGatewayLib::IsPower()
{
	
	return poweron;
	
}

void DSGatewayLib::clearMessage(unsigned char *inPackets)
{
 	for( int i = 0; i < SIZE_PACKET; i++)
	{
		inPackets[i] = 0;
	} 
}

boolean DSGatewayLib::sendMessage(unsigned char *inPackets)
{
	byte aReceived[SIZE_PACKET] = {0,0,0,0,0,0,0,0};
	int i;

	//SS activate
	digitalWrite(SPI_SSPIN, LOW);
	delayMicroseconds(5);
	
	for( i = 0; i < SIZE_PACKET; i++)
	{
		aReceived[i] = SPI.transfer(inPackets[i]);
		delayMicroseconds(70);
	}
	
	delayMicroseconds(200);
	
	//SS deactivate
    digitalWrite(SPI_SSPIN, HIGH);
	
	/* Check for receiving */
	if( ((aReceived[1] & 0b11110000) == CMD_OK) ||((aReceived[1] & 0b11110000) == CMD_WAIT) )
	{
		return true;
	}
	else
	{
		return false;
	}
}

boolean DSGatewayLib::recvCVMessage(unsigned char *inPackets, byte *outCVvalue, byte *outCVResult)
{
	byte aReceived[SIZE_PACKET] = {0,0,0,0,0,0,0,0};
	int i;

	digitalWrite(SPI_SSPIN, LOW);
	//delayMicroseconds(50);
	
	for( i = 0; i < SIZE_PACKET; i++)
	{
		aReceived[i] = SPI.transfer(inPackets[i]);
		delayMicroseconds(5);
	}
	
    digitalWrite(SPI_SSPIN, HIGH);
	
	
	/* ’x‰„ */
	delay(20);
	
	/* Check for receiving */
	if( (aReceived[1] & 0b11110000) == CMD_CVREAD)
	{
		*outCVvalue = aReceived[2];
		*outCVResult = aReceived[3];
		
		return true;
	}
	else
	{
		return false;
	}
	
	
}

boolean DSGatewayLib::exchangeMessage(unsigned char *inPackets, word timeout)
{
	//unsigned char aTemp[SIZE_PACKET];

	bool aReturnOk = sendMessage(inPackets);
	
	/*
	unsigned long aTime = millis();

	// response;

	while ((millis() < aTime + timeout) || (!aReturnOk)) {
		aTemp[0] = CMD_WAIT | 0b0010;
		aTemp[1] = aTemp[0];
		
		delay(20);
		
		if (sendMessage(aTemp) == true) {
			return true;
		}
	}

	if (DEBUG && !aReturnOk) {
		Serial.println(F("!!! Communication Error(Timeout, Command etc)"));
	}
	*/
	
	return aReturnOk;
}


boolean DSGatewayLib::SetPower(boolean power)
{
	unsigned char aPacktes[SIZE_PACKET];

	clearMessage(aPacktes);

	if (power) {
		aPacktes[0] = CMD_PWR_ON | 0b0010;
		aPacktes[1] = aPacktes[0];//CRC
		poweron = true;

	}
	else
	{
		aPacktes[0] = CMD_PWR_OFF | 0b0010;
		aPacktes[1] = aPacktes[0];//CRC
		poweron = false;
	}

	return exchangeMessage(aPacktes, TIME_REPLY);
}

boolean DSGatewayLib::SetPowerEx(boolean power)
{
	unsigned char aPacktes[SIZE_PACKET];

	clearMessage(aPacktes);

	if (power) {
		aPacktes[0] = CMD_PWR_ON | 0b0011;
		aPacktes[1] = 0b00000011;//Extend command (DCC128 and MM2-28step)
		aPacktes[2] = generateCRC(aPacktes, 2);

	}
	else
	{
		aPacktes[0] = CMD_PWR_OFF | 0b0010;
		aPacktes[1] = aPacktes[0];//CRC
	}
	


	return exchangeMessage(aPacktes, TIME_REPLY);
}

boolean DSGatewayLib::SetLocoSpeed(word address, int inSpeed)
{
	unsigned char aPacktes[SIZE_PACKET];

	clearMessage(aPacktes);

	aPacktes[0] = CMD_SPEED | 0b0110;
	aPacktes[1] = lowByte(address);
	aPacktes[2] = highByte(address);
	aPacktes[3] = lowByte(inSpeed);
	aPacktes[4] = highByte(inSpeed);
	aPacktes[5] = generateCRC(aPacktes, 5);

	return exchangeMessage(aPacktes, TIME_REPLY);
}


boolean DSGatewayLib::SetLocoSpeedEx(word address, int inSpeed, int inProtcol)
{
    unsigned char aPacktes[SIZE_PACKET];

    clearMessage(aPacktes);

    aPacktes[0] = CMD_SPEED | 0b0111;
    aPacktes[1] = lowByte(address);
    aPacktes[2] = highByte(address);
    aPacktes[3] = lowByte(inSpeed);
    aPacktes[4] = highByte(inSpeed);
    aPacktes[5] = inProtcol;
    aPacktes[6] = generateCRC(aPacktes, 6);

    return exchangeMessage(aPacktes, TIME_REPLY);
} 

boolean DSGatewayLib::SetLocoFunction(word address, unsigned char inFunction, unsigned char inPower)
{
	unsigned char aPacktes[SIZE_PACKET];

	clearMessage(aPacktes);

	aPacktes[0] = CMD_FUNCTION | 0b0110;
	aPacktes[1] = lowByte(address);
	aPacktes[2] = highByte(address);
	aPacktes[3] = inFunction + 1; //1‚Í‚¶‚Ü‚è
	aPacktes[4] = inPower;
	aPacktes[5] = generateCRC(aPacktes, 5);

	return exchangeMessage(aPacktes, TIME_REPLY);
}


boolean DSGatewayLib::SetLocoDirection(word address, unsigned char inDirection)
{
	unsigned char aPacktes[SIZE_PACKET];

	clearMessage(aPacktes);

	aPacktes[0] = CMD_DIRECTION | 0b0101;
	aPacktes[1] = lowByte(address);
	aPacktes[2] = highByte(address);
	aPacktes[3] = inDirection - 1;// FWD 1->0, REV:2->1
	aPacktes[4] = generateCRC(aPacktes, 4);

	return exchangeMessage(aPacktes, TIME_REPLY);
}

/*
boolean DSGatewayLib::SetTurnout(word address, boolean straight)
{
	byte aSwitch = straight == true ? (byte)1 : (byte)0;
	
	return SetTurnout(address, aSwitch);
}*/

boolean DSGatewayLib::SetTurnout(word address, byte inSwitch)
{
	unsigned char aPacktes[SIZE_PACKET];

	clearMessage(aPacktes);

	aPacktes[0] = CMD_ACCESSORY | 0b0110;
	aPacktes[1] = lowByte(address);
	aPacktes[2] = highByte(address);
	aPacktes[3] = 0x00; // position
	aPacktes[4] = convertAcc_MMDCC(address, inSwitch);// 0: Straight, 1: diverging from DS 1:straight, 0: diverging
	aPacktes[5] = generateCRC(aPacktes, 4);

	return exchangeMessage(aPacktes, TIME_REPLY);
}

byte DSGatewayLib::convertAcc_MMDCC(word address, byte inSwitch)
{
	
	switch( GetLocIDProtocol(address >> 8))
	{
	case ADDR_ACC_MM2:
		/* 0:Straight, 1: diverging */
		return (inSwitch == 0) ? 1 : 0;
		break;
		
	case ADDR_ACC_DCC:
		/* 1:Straight, 0: diverging */
		return inSwitch;
		break;
		
	default:
		return inSwitch;
		break;
	}
	
}

word DSGatewayLib::GetLocIDProtocol(byte address)
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

boolean DSGatewayLib::WriteConfig(word address, word number, byte value)
{
	
	return WriteConfigEx(address, number, value, 0);
}

boolean DSGatewayLib::WriteConfigEx(word inAddress, word inNumber, byte inValue, byte inMode)
{
	unsigned char aPacktes[SIZE_PACKET];
	
	word aOffsetCVNo;
	
	if( inAddress >= ADDR_DCC)
	{
		aOffsetCVNo = inNumber;
	}
	else
	{
		aOffsetCVNo = inNumber | 0x8000;
	}

	clearMessage(aPacktes);

	
	if( inMode == CVMODE_DIRECT)
	{
		//Direct mode
		aPacktes[0] = CMD_CVWRITE | 0b0101;
		aPacktes[1] = lowByte(aOffsetCVNo);
		aPacktes[2] = highByte(aOffsetCVNo);
		aPacktes[3] = inValue; 
		aPacktes[4] = generateCRC(aPacktes, 3);
		
	}
	else
	{
		//OPS mode
		aPacktes[0] = CMD_CVWRITE | 0b0111;
		aPacktes[1] = lowByte(aOffsetCVNo);
		aPacktes[2] = highByte(aOffsetCVNo);
		aPacktes[3] = inValue; 
		aPacktes[4] = lowByte(inAddress);
		aPacktes[5] = highByte(inAddress);
		aPacktes[6] = generateCRC(aPacktes, 5);
	}

	return exchangeMessage(aPacktes, TIME_REPLY);
}

boolean DSGatewayLib::ReadConfig(word address, word number, byte *value)
{
	unsigned char aPacktes[SIZE_PACKET];
	boolean aRet;
	unsigned char aCVResult = 0;
	unsigned char aCVValue = 0;
	
	word aOffsetCVNo;
	
	if( address >= ADDR_DCC)
	{
		aOffsetCVNo = number;
	}
	else
	{
		aOffsetCVNo = number | 0x8000;
	}
	
	clearMessage(aPacktes);

	aPacktes[0] = CMD_CVREAD | 0b0100;
	aPacktes[1] = lowByte(aOffsetCVNo);
	aPacktes[2] = highByte(aOffsetCVNo);
	aPacktes[3] = generateCRC(aPacktes, 2);
	
	*value = 0;

	exchangeMessage(aPacktes, TIME_REPLY);
	
	for( int i = 0; i < 60; i++)
	{
		/* Reset watchdog timer. */
		wdt_reset(); 
		delay(100);
	
		aPacktes[0] = CMD_OK | 0b0010;
		aPacktes[1] = 0x00;
		aPacktes[2] = generateCRC(aPacktes, 1);
		
		/* CV read command(reply) */
		if( recvCVMessage(aPacktes, &aCVValue, &aCVResult) == true)
		{
			break;
		}
		else
		{
			/* ‰½‚à‚µ‚È‚¢ */
			aCVResult = 0;
		}
	}
	
	if( aCVResult == 1)
	{
		*value = aCVValue;
		return true;
	}
	else
	{
		return false;
	}
}


unsigned char DSGatewayLib::generateCRC(unsigned char *inPackets, unsigned char inLen)
{
	unsigned char aCRC = inPackets[0];

	for( int i = 1; i < inLen; i++)
	{
		aCRC = aCRC ^ inPackets[i];
	}
	
	return aCRC;

}

