/*********************************************************************
 * Desktop Station Gateway Library for Arduino
 *
 * Copyright (C) 2014 Yaasan
 *
 */

#ifndef DSGATEWAYLIB_H
#define DSGATEWAYLIB_H


#include <Arduino.h>
#include <SPI.h>

#define DSGATEWAY_VERSION 0x01
#define DEBUG false
#define SIZE_PACKET 8
#define TIME_REPLY 200
#define SPI_SSPIN 10
#define TIME_OUT 200

#define ADDR_MM2     0x0000 // MM2 locomotive
#define ADDR_SX1     0x0800 // Selectrix (old) locomotive
#define ADDR_MFX     0x4000 // MFX locomotive
#define ADDR_SX2     0x8000 // Selectrix (new) locomotive
#define ADDR_DCC     0xC000 // DCC locomotive
#define ADDR_ACC_SX1 0x2000 // Selectrix (old) magnetic accessory
#define ADDR_ACC_MM2 0x2FFF // MM2 magnetic accessory
#define ADDR_ACC_DCC 0x37FF // DCC magnetic accessory

#define SPEEDSTEP_DCC28 0
#define SPEEDSTEP_DCC14 1
#define SPEEDSTEP_DCC127 2
#define SPEEDSTEP_MM14 0
#define SPEEDSTEP_MM28 1

#define CVMODE_DIRECT		0	// DIR
#define CVMODE_OPERATION	1	// OPS


#define CMD_PWR_OFF 0b00000000
#define CMD_PWR_ON  0b11110000

#define CMD_WAIT 	0b11010000
#define CMD_OK		0b10000000
#define CMD_CRCERR	0b10010000
#define CMD_CMDERR	0b10100000
#define CMD_UNKERR	0b11000000
#define CMD_DCC_IDLE	0b11000000
#define CMD_SPEED		0b00010000
#define CMD_ACCESSORY	0b00100000
#define CMD_FUNCTION	0b00110000
#define CMD_CVWRITE		0b01000000
#define CMD_DIRECTION	0b01010000
#define CMD_CVREAD		0b01100000
#define CMD_EXTENTION	0b01110000


/* 特殊・拡張機能用 */
#define EXCMD_NONE		0


/* Defined functions */
class DSGatewayLib
{
  private:
	void sendPacket(unsigned char *inPackets);
	boolean sendMessage(unsigned char *inPackets);
	boolean recvCVMessage(unsigned char *inPackets, byte *outCVvalue, byte *outCVResult);
	boolean exchangeMessage(unsigned char *inPackets, word timeout);
	void clearMessage(unsigned char *inPackets);
	unsigned char generateCRC(unsigned char *inPackets, unsigned char inLen);
	byte convertAcc_MMDCC(word address, byte inSwitch);
	boolean poweron;
	
  public:
	DSGatewayLib();
	~DSGatewayLib();
	void begin();
	
	boolean IsPower();
	
	boolean SetPower(boolean power);
	boolean SetPowerEx(boolean power);
	boolean SetLocoSpeed(word address, int inSpeed);
	boolean SetLocoSpeedEx(word address, int inSpeed, int inProtcol);
	boolean SetLocoFunction(word address, unsigned char inFunction, unsigned char inPower);
	boolean SetLocoDirection(word address, unsigned char inDirection);
	boolean SetTurnout(word address, byte inSwitch);
	boolean WriteConfig(word address, word number, byte value);
	boolean WriteConfigEx(word inAddress, word inNumber, byte inValue, byte inMode);
	boolean ReadConfig(word address, word number, byte *value);
	word GetLocIDProtocol(byte address);

};

#endif
