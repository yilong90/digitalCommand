/*********************************************************************
 * New Serial Library for Desktop Station main
 *
 * Copyright (C) 2015 Yaasan
 *
 */

#include <Arduino.h>

#define SERIAL9600	1
#define SERIAL115K	0

#define SERIAL_PARITY	1
#define SERIAL_NONPARITY	0


class NewSerial {

  private:

  public:
	NewSerial();
	void Init(byte inBaud, byte inEParity);
	void write(char inByte);
	void print(char *inText);
	void println(char *inText);
	void printDEC(word inword);
	void printHEX(word inword);
	void printBIN(word inword);
	
	bool received();
	void clearReceive();
	String getRequest();
	byte IsError();
	void EnableParity();
};
