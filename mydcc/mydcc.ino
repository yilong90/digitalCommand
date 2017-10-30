#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <avr/wdt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "DSSequence.h"

//菜单
const char rMainMenu0[] PROGMEM    = " CV Read";
const char rMainMenu1[] PROGMEM    = " CV Write";
const char rMainMenu2[] PROGMEM    = " CV ReadWrite";
const char rMainMenu3[] PROGMEM    = " Nucky Signal";
const char rMainMenu4[] PROGMEM    = " Check LocAddr";
const char rMainMenu5[] PROGMEM    = " Write LocAddr";
const char rMainMenu6[] PROGMEM    = " Loc Control";
const char rMainMenu7[] PROGMEM    = " Acc Control";
const char rMainMenu8[] PROGMEM    = " Config";
const char rFactoryRes[] PROGMEM   = " Factory Reset";
const char rErrorStat[] PROGMEM    = " Error Status";
const char rManCheck[] PROGMEM     = " Manufacturer";
const char rCVWriting[] PROGMEM    = " CV Writing...";
const char rCVReading[] PROGMEM    = " CV Reading...";
const char rCVEnterStart[] PROGMEM   = " Enter to start";
const char rCVReadErr[] PROGMEM      = " Read error!";
const char rLocAdrWrite[] PROGMEM    = " LocAddr Write";
const char rLocAdrCheck[] PROGMEM    = " LocAddr Check";
const char rWriteOk[] PROGMEM     = " Write Ok!";
const char rWriteEnd[] PROGMEM    = " Write End!";
const char rWriteErr[] PROGMEM    = " Write error!";
const char rFinished[] PROGMEM    = " Finished!";
const char rFailed[] PROGMEM      = " Failed!";
const char rReturnTop[] PROGMEM      = " Return to Top";
const char rEnterReset[] PROGMEM      = " Enter to reset";

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(9, 8, 13, 12, 11, 10);

DSSequence Sequence;
unsigned long gPreviousL5 = 0; //   500ms interval
unsigned long gPreviousL1 = 0; //   100ms interval

//LCD text
char gTextBuf[16 + 1];
//byte 0-255   word 0-65535
byte gDisplayUpdate = 0;

byte convertByteToASCII(byte inByte);

void displayConfig(byte inNo, byte inArrow);
void displayCVRead(byte inNo);
void displayCVReadWrite(byte inNo);
void displayAddrCheck(byte inNo);
void displayAddrWrite(byte inNo);
void displayNuckySignal(byte inNo);

void displayFactoryReset(byte inNo);
void displayErrStatus(byte inNo);

void ExecuteCommand(unsigned char *outpReturn, unsigned char inDrawFlag);

void changedPowerStatus(const byte inPower);
void changedTargetSpeed(const word inAddr, const word inSpeed);
void changedTargetFunc(const word inAddr, const byte inFuncNo, const byte inFuncPower);
void changedTargetDir(const word inAddr, const byte inDir);
void changedTurnout(const word inAddr, const byte inDir);
void clearTextBuf(void);
void GetManufactureName();
void StrCpyFromROM(char *outpText, const char *inpROMText, byte inLen);

void setup() 
{
    lcd.begin(16, 2);
    lcd.print("DCC Controller");
    lcd.setCursor(0, 1);
    lcd.print("CVRead&Write 0.1");

    Serial.begin(9600);
	while (!Serial);

	Serial.println("My DCC Controller");
	Serial.println("2017 10 24");
	
	delay(500);

    wdt_enable(WDTO_8S); // WDT reset
	
	
	//lcd update
	gDisplayUpdate = 0;
	Draw();
	
	
	gPreviousL5 = millis();
	gPreviousL1 = millis();
	sei();    //ENABLE INTERRUPTION
}

void loop()
{
    /* Reset watchdog timer. */
    wdt_reset(); 
    if((millis() - gPreviousL1) >= 100)
    {
        gDisplayUpdate = Sequence.Interval();

        //reset task
        gPreviousL1 = millis();
	}
	
	if(gDisplayUpdate > 0)
	{
		gDisplayUpdate = 0;
		Draw();
	}
}

void Draw()
{
    lcd.clear();

    switch(Sequence.gMode)
    {
        case MODE_MAINMENU:
            for(int i = 0; i <= 1; i++)
			{
				lcd.setCursor(0, i);
				displayMainMenu(i + Sequence.gNo_MainMenu, i);
			}			
			break;
    }
}

void displayMainMenu(byte inNo, byte inArrow)
{
    char aTextBuf[16 + 1] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    switch(inNo)
	{
	case 0:
		StrCpyFromROM(aTextBuf, rMainMenu0, 8);
		break;
	case 1:
		StrCpyFromROM(aTextBuf, rMainMenu1, 9);
		break;
	case 2:
		StrCpyFromROM(aTextBuf, rMainMenu2, 13);
		break;
	case 3:
		StrCpyFromROM(aTextBuf, rMainMenu3, 13);
		break;
	case 4:
		StrCpyFromROM(aTextBuf, rMainMenu4, 14);
		break;
	case 5:
		StrCpyFromROM(aTextBuf, rMainMenu5, 14);
		break;
	case 6:
		StrCpyFromROM(aTextBuf, rMainMenu6, 12);
		break;
	case 7:
		StrCpyFromROM(aTextBuf, rMainMenu7, 12);
		break;
	case 8:
		StrCpyFromROM(aTextBuf, rManCheck, 13);
		break;
	case 9:
		StrCpyFromROM(aTextBuf, rFactoryRes, 14);
		break;
	case 10:
		StrCpyFromROM(aTextBuf, rErrorStat, 13);
		break;
	case 11:
		StrCpyFromROM(aTextBuf, rMainMenu8, 7);
		break;
	}
	
	if( inArrow == 0)
	{
		aTextBuf[0] = 0x3E;
	}
	
	lcd.print(aTextBuf);
	
}

void StrCpyFromROM(char *outpText, const char *inpROMText, byte inLen)
{
	byte i;
	
	for( i = 0; i < inLen; i++)
	{
		outpText[i] = (char)pgm_read_byte(inpROMText + i);
	}
	
	for( i = inLen; i < 16; i++)
	{
		outpText[i] = 0x00;
	}
}
