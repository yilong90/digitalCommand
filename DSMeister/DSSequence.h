/*********************************************************************
 * Sequence process for Desktop Station main
 *
 * Copyright (C) 2015 Yaasan
 *
 */

#include <Arduino.h>

#define SQCMD_DEPTH_LOC		1
#define SQCMD_DEPTH_MENU	2
#define SQCMD_DEPTH_ACC		3
#define SQCMD_DEPTH_CVR		4
#define SQCMD_DEPTH_CVW		5
#define SQCMD_DEPTH_INFO	6
#define SQCMD_DEPTH_CFG		7


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


#define SQBUTTON_ENT 0b00000010
#define SQBUTTON_ESC 0b00000100
#define SQBUTTON_UP	 0b00001000
#define SQBUTTON_DN	 0b00010000
#define SQBUTTON_DIR 0b00100000
#define SQBUTTON_RUN 0b01000000


#define GO_FWD 1
#define GO_REV 2
#define GO_STRAIGHT 1
#define GO_DIVERSE 0

#define POWER_ON 1
#define POWER_OFF 0

#define TIME_RETURN 900


#define DSEEPROM_LOCADDR 0b00000001
#define DSEEPROM_ACCADDR 0b00000010
#define DSEEPROM_PROTCOL 0b00000100
#define DSEEPROM_RJ45SEL 0b00001000
#define DSEEPROM_OCLVSEL 0b10000001

#define RJ45MODE_S88 0
#define RJ45MODE_DSJ 1


typedef void (*CVFunction) (const byte inSeq, const word inCVAddr, const byte inCVValue);
typedef void (*VoidFunction) ();
typedef void (*PowerFunction) (const byte inPower);
typedef void (*SpeedFunction) (const word inAddr, const word inSpeed, const bool inUpdate);
typedef void (*DirFunction) (const word inAddr, const byte inDir, const bool inUpdate);
typedef void (*FuncFunction) (const word inAddr, const byte inFuncNo, const byte inFuncPower, const bool inUpdate);


class DSSequence {

  private:
	
	byte mode_depth;
	byte mode_depth_last;
	
	byte mode_seq;
	byte mode_rj45;
	byte mode_oclv;
	word prev_dial;
	byte prev_btns;
	byte mode_protocol;
	byte count_SelButton;
	byte count_LocButton;
	byte count_AdrButton;
	word cv_no;
	byte cv_value;
	byte currentCVSeq;
	
	//Function buffer
	byte currentFnc[29] = 
	{0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0};
	byte currentFuncNo;
	
	word locaddress_temp;
	word accaddress_temp;
	byte currentMenuNo;
	byte currentCfgNo;
	
	//EEPROM Param
	word eepLocAddr;
	word eepAccAddr;
	
	byte getButtonStatus();
	word getDialStatus();
	
	bool checkButton(byte inBuf, byte inCurrent, byte inButtonBit);
	bool checkButtonLeaved(byte inBuf, byte inCurrent, byte inButtonBit);
	bool checkButtonContinuous(byte inBuf, byte inCurrent, byte inButtonBit);
	void updateLCD();
	void setProtocol();
	void setTargetFunc(byte inNo);
	void clickDir();
	void clickRun();
	void loadEEPROM();
	void saveEEPROM(byte inMode);
	void interval_always(byte inButtonStatus);
	void interval_Loc(byte inButtonStatus);
	void interval_Acc(byte inButtonStatus);
	void interval_Menu(byte inButtonStatus);
	void interval_ReadCV(byte inButtonStatus);
	void interval_WriteCV(byte inButtonStatus);
	void interval_Info(byte inButtonStatus);
	void interval_Config(byte inButtonStatus);
	void minusLocAddress(word *iopLocAddr, byte inProtocol, word inMinus);
	void plusLocAddress(word *iopLocAddr, byte inProtocol, word inPlus);
	void minusAccAddress(word *iopAccAddr, byte inProtocol, word inMinus);
	void plusAccAddress(word *iopAccAddr, byte inProtocol, word inPlus);

	
	
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
	byte GetModeDepth();
	byte GetModeTop();
	byte GetRJ45Mode();
	byte GetOCLVMode();
	void NextMode(byte inMode);
	
	PowerFunction ChangedPowerStatus;
	SpeedFunction ChangedTargetSpeed;
	FuncFunction ChangedTargetFunc;
	DirFunction ChangedTargetDir;
	DirFunction ChangedTurnout;
	PowerFunction DebugMessage;
	PowerFunction DisplayMessage;
	PowerFunction ChangedMenu;
	PowerFunction ChangedConfiguration;
	VoidFunction ChangedMain;
	VoidFunction ChangedInfo;
	DirFunction ChangedAcc;
	VoidFunction DisplayClear;
	CVFunction ReadCVFunc;
	CVFunction WriteCVFunc;
	

};
