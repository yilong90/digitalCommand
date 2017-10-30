/*********************************************************************
 * Sequence process for Desktop Station main
 *
 * Copyright (C) 2015 Yaasan
 *
 */

#include <Arduino.h>


#define SQCMD_NONE 0
#define SQCMD_SPEED 10
#define SQCMD_TURNOUT 11
#define SQCMD_FUNCTION 12
#define SQCMD_DIRECTION 13
#define SQCMD_CVWRITE 14

#define SQMODE_POWEROFF 0
#define SQMODE_DCC 10
#define SQMODE_MM2 20
#define SQMODE_CV 30

#define SQMODE_LOC 1
#define SQMODE_ACC 2
#define SQMODE_CVW 3
#define SQMODE_LOC_EDIT 4
#define SQMODE_ACC_EDIT 5


#define SQBUTTON_SEL 0b00000001
#define SQBUTTON_LOC 0b00000010
#define SQBUTTON_ACC 0b00000100
#define SQBUTTON_FN0 0b00001000
#define SQBUTTON_FN1 0b00010000
#define SQBUTTON_DIR 0b00100000
#define SQBUTTON_RUN 0b01000000

#define DSEEPROM_LOCADDR 0b00000001
#define DSEEPROM_ACCADDR 0b00000010
#define DSEEPROM_PROTCOL 0b00000100


#define GO_FWD 1
#define GO_REV 2
#define GO_STRAIGHT 1
#define GO_DIVERSE 0

#define POWER_ON 1
#define POWER_OFF 0

#define TIME_RETURN 900


typedef void (*CVFunction) (const byte inCVNo, const byte inCVValue);
typedef void (*PowerFunction) (const byte inPower);
typedef void (*SpeedFunction) (const word inAddr, const word inSpeed, const bool inUpdate);
typedef void (*DirFunction) (const word inAddr, const byte inDir, const bool inUpdate);
typedef void (*FuncFunction) (const word inAddr, const byte inFuncNo, const byte inFuncPower, const bool inUpdate);


class DSSequence {

  private:
	byte mode_seq;
	word prev_dial;
	byte prev_btns;
	byte mode_protocol;
	byte currentFnc[8];
	byte lastUsedFunc;
	byte lastFuncNo;
	byte count_SelButton;
	byte count_LocButton;
	byte cv_no;
	byte cv_value;
	word locaddress_temp;
	word accaddress_temp;
	
	byte getButtonStatus();
	word getDialStatus();
	
	bool checkButton(byte inBuf, byte inCurrent, byte inButtonBit);
	bool checkButtonLeaved(byte inBuf, byte inCurrent, byte inButtonBit);
	bool checkButtonContinuous(byte inBuf, byte inCurrent, byte inButtonBit);
	void updateLCD();
	void setProtocol();
	void setTargetFunc(byte inNo);
	void clickSelectWithRun(byte inSeqMode);
	void clickSelectWithFunc0(byte inSeqMode);
	void clickSelectWithFunc1(byte inSeqMode);
	void clickSelectWithLoc(byte inSeqMode);
	void clickSelectWithAcc(byte inSeqMode);
	void clickDir(byte inSeqMode);
	void clickRun(byte inSeqMode);
	void clickLoc(byte inSeqMode);
	void clickAcc(byte inSeqMode);
	void clickFunc0(byte inSeqMode);
	void clickFunc1(byte inSeqMode);
	void pushingLoc(byte inSeqMode);
	void pushingSelect(byte inSeqMode);
	void clickLocWithFunc0(byte inSeqMode);
	void clickLocWithFunc1(byte inSeqMode);
	void loadEEPROM();
	void saveEEPROM(byte inMode);
	
	
  public:
	word NextLocoSpeed;
	word NextLocoAddress;
	byte NextLocoDirection;
	word NextAccAddress;
	byte NextAccDirection;
	byte NextPowerStatus;
	word CurrentProtocol;
	word CurrentACCProtocol;
	
	DSSequence();
	bool Interval();
	void RegisterFunction(byte inFuncNo);
	void RegisterSpeed(word inSpeed);
	void RegisterDir();
	
	PowerFunction ChangedPowerStatus;
	SpeedFunction ChangedTargetSpeed;
	FuncFunction ChangedTargetFunc;
	DirFunction ChangedTargetDir;
	DirFunction ChangedTurnout;
	PowerFunction DebugMessage;
	PowerFunction DisplayMessage;
	CVFunction DisplayCVFunc;
	CVFunction ChangedCVFunc;
	SpeedFunction DisplayLocEditFunc;
	SpeedFunction DisplayAccEditFunc;

	

};
