/*********************************************************************
 * LCD management process for Desktop Station main
 *
 * Copyright (C) 2015 Yaasan
 *
 */
 
#ifndef DSLCDMANAGER_H
#define DSLCDMANAGER_H

#include <Arduino.h>

#define MODE_DISP_HELLO			0
#define MODE_DISP_LOC			1
#define MODE_DISP_ACC			6
#define MODE_DISP_MENU			7
#define MODE_DISP_CVR			2
#define MODE_DISP_CVW			3
#define MODE_DISP_INF			4
#define MODE_DISP_CFG			5
#define MODE_DISP_ERR			99

#define MODE_DISP_TEMP_NONE		0
#define MODE_DISP_TEMP_FUNC		10


typedef void (*PrintFunction) (const char *s, const int x, const int y);
typedef void (*DrawFunction) (byte x, byte y, byte num, byte inNot);
typedef void (*VoidFunction) ();
typedef byte (*GetFunction) (byte inNo);

class DSLCDManager
{

	private:
		
	
	//一時保存関連
	unsigned char valCurrent_H;
	unsigned char valCurrent_L;
	unsigned char valVoltage_H;
	unsigned char valVoltage_L;
	unsigned char valErrCurrent_H;
	unsigned char valErrCurrent_L;
	unsigned char valErrVoltage_H;
	unsigned char valErrVoltage_L;

	unsigned char valErrorFlag;
	bool flagRefresh;
	unsigned char valPower;
	word valAccAddress;
	unsigned char valAccDirection;
	word valLocAddress;
	word valLocSpeed;
	unsigned char valLocDirection;
	byte valLocFunction;
	byte valLocFuncPower;
	unsigned char valSelectedMenuItemNo;
	unsigned char valConfigItemNo;
	word valCVAddr;
	byte valCVValue;
	byte mode_cv_seq;
	
	word valS88Data[8];
	
	//表示モード
	unsigned char mode_display;
	byte mode_display_temp;
	byte mode_display_temp_cnt;
	
	void clearTextBuf();
		
		
	public:
	
	DSLCDManager();
	void SetSensors(unsigned char inI_H, unsigned char inI_L, unsigned char inV_H, unsigned char inV_L, unsigned char inErrFlag);
	void SetSensorsErr(unsigned char inI, unsigned char inV);
	void Interval();
	void DisplayLocAddress();
	void DisplayAccAddress();
	void DisplayIcon();
	unsigned char GetMode();
	
	void RegisterLocSpeed(word inLocAddr, word inSpeed);
	void RegisterLoc(word inLocAddr);
	void RegisterLocDirection(word inLocAddr, unsigned char inDir);
	void RegisterLocFunction(word inLocAddr, unsigned char inFuncNo, unsigned char inFuncPower);
	void RegisterPower(unsigned char inPower);
	void RegisterTurnout(word inAccAddr, unsigned char inDir);
	void RegisterMenu(byte inMenuItemNo);
	void RegisterConfig(byte inConfigItemNo);
	void RegisterAcc(word inAccAddr, unsigned char inDir);
	void RegisterMain();
	void RegisterInfo();
	word GetLocIDProtocol(byte address);
	void AddMessages(char *inText);
	void WriteLocAddr(word inAddr);
	void WriteAccAddr(word inAddr);
	void RegisterCVWrite(byte inSeq, word inAddr, byte inValue);
	void RegisterCVRead(byte inSeq, word inAddr, byte inValue);
	void ClearRow(byte inY);
	void StrCpyFromROM(char *outpText, const char *inpROMText, byte inLen);
	
	VoidFunction ClearDisplay;
	PrintFunction PrintString;
	GetFunction GetConfigData;
	

};

#endif
