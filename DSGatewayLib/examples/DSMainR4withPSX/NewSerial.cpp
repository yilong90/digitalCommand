/*********************************************************************
 * New Serial Library for Desktop Station main
 *
 * Copyright (C) 2015 Yaasan
 *
 */

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <Arduino.h>
#include "NewSerial.h"

int gSerialSize = 0;
int gReceived = 0;
String gRequest;
char gSerialBuffer[64];

//Define function
void UART_write(char inByte);
void UART_print(char *inText);
void UART_clearBuf();


NewSerial::NewSerial()
{
	
	gRequest = "";
	UART_clearBuf();
}

String NewSerial::getRequest()
{
	return gRequest;
}

bool NewSerial::received()
{
	return (gReceived == 1) ? true : false;
}

void NewSerial::clearReceive()
{
	gReceived = 0;
}

void NewSerial::Init()
{
	unsigned long baud = 115200;
	//REGISTRE UBRR0
	
	//38400 BAUD FOSC=16MHZ
	//UBRR0=25;
	
	//115200 BAUD FOSC=16MHZ
	UBRR0=(F_CPU / 4 / baud - 1) / 2;
	UCSR0A = (1 << U2X0);


	//REGISTRE USCR0C
	//COM ASYNCHRONE
	bitWrite(UCSR0C,UMSEL00,0);
	bitWrite(UCSR0C,UMSEL01,0);

	//PARITY NONE
	bitWrite(UCSR0C,UPM01,0);
	bitWrite(UCSR0C,UPM00,0);

	//8 DATA BIT
	bitWrite(UCSR0C,UCSZ02,0);
	bitWrite(UCSR0C,UCSZ01,1);
	bitWrite(UCSR0C,UCSZ00,1);

	//REGISTRE UCSR0B  
	//RECEIVE & TRANSMITT ENABLE
	bitWrite(UCSR0B,RXEN0,1);
	bitWrite(UCSR0B,TXEN0,1);

	//ENABLE RX COMPLETE INTERRUPT
	bitWrite(UCSR0B, RXCIE0,1);
	
	sei();
}

void NewSerial::write(char inByte)
{
	UART_write(inByte);
}

void UART_write(char inByte)
{
	while( !(UCSR0A & 0b00100000) );
	UDR0 = inByte;
}

void NewSerial::print(char *inText)
{
	UART_print(inText);
}

void UART_print(char *inText)
{
	while (*inText)
	{
		UART_write (*inText);
		if (*(inText++) == '\n')
		{
			UART_write('\r');
		}
	}
	
	
}



void NewSerial::println(char *inText)
{
	
	print(inText);
	write('\n');
	
	
	
}

void NewSerial::printDEC(word inword)
{
	char aText[8];
	
	sprintf(aText, "%d", inword);
	print(aText);
	
}


void NewSerial::printHEX(word inword)
{
	char aText[8];
	
	sprintf(aText, "%x", inword);
	print(aText);
	
}

void NewSerial::printBIN(word inword)
{
	char aText[32];
	memset(aText, 0, sizeof(aText));
	
	sprintf(aText, "%b", inword);
	print(aText);
	
}


void UART_clearBuf()
{
	for( int i = 0; i < 64; i++)
	{
		gSerialBuffer[i] = 0x00;
	}
	
	gSerialSize = 0;
	
}

ISR(USART_RX_vect)
{  
	char  aByte;
	uint8_t  aUSARTstate;
	
	aByte   = UDR0;
	aUSARTstate = UCSR0A;

	/* Check serial buffer */
	if (aUSARTstate & (1 << FE0))
	{
		//Break
		gSerialSize = 0;
		
		return;
	}
	else
	{
		// Write to text buf.
		gSerialBuffer[gSerialSize] = aByte;
		
		// increment
		gSerialSize = gSerialSize + 1;
		
		
		// check last identification of text end.
		if (aByte == '\n')
		{
			gSerialBuffer[gSerialSize] = '\0';
			gRequest = String(gSerialBuffer);
			gReceived = 1;
			
			//Clear buffer
			UART_clearBuf();
			
		}
		else
		{
		}
		
		
		// buf over check
		if( gSerialSize > 63)
		{
			gSerialSize = 0;
			UART_clearBuf();
		}
	}

}
