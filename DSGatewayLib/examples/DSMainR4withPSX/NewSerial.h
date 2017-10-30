/*********************************************************************
 * New Serial Library for Desktop Station main
 *
 * Copyright (C) 2015 Yaasan
 *
 */

#include <Arduino.h>


class NewSerial {

  private:

  public:
	NewSerial();
	void Init();
	void write(char inByte);
	void print(char *inText);
	void println(char *inText);
	void printDEC(word inword);
	void printHEX(word inword);
	void printBIN(word inword);
	
	bool received();
	void clearReceive();
	String getRequest();

};
