/*********************************************************************
 * Sequence process for Desktop Station main
 *
 * Copyright (C) 2015 Yaasan
 *
 */

 #include <Arduino.h>
 #include <LiquidCrystal.h>
 
 
 
 #define MODE_NONE			0
 #define MODE_MAINMENU		1
 #define MODE_CVREAD			2
 #define MODE_CVWRITE		3
 #define MODE_CVREADWRITE	4
 #define MODE_NUCKYSIGNAL	5
 #define MODE_CHECKADDRESS	6
 #define MODE_WRITEADDRESS	7
 #define MODE_LOCCTRL		8
 #define MODE_ACCCTRL		9
 #define MODE_MANUFACTURE    10
 #define MODE_FACTORYRESET	11
 #define MODE_ERRSTATUS		12
 #define MODE_CONFIG			13
 
 
 #define MODE_WAITING		20
 #define MODE_ERROR			99
 
 #define		PORT_BTNA		A5
 #define		PORT_BTNB		A4
 #define		PORT_BTNC		A1
 
 
 #define DSCMD_NONE			0
 #define DSCMD_CVREAD		10
 #define DSCMD_CVWRITE		11
 #define DSCMD_SPEED			20
 #define DSCMD_DIR			30
 
 typedef void  (*ExeFunction) (unsigned char *outpReturn, unsigned char inDrawFlag);
 typedef void (*VoidFunction) ();
 typedef void (*PowerFunction) (const byte inPower);
 typedef void (*SpeedFunction) (const word inAddr, const word inSpeed);
 typedef void (*DirFunction) (const word inAddr, const byte inDir);
 typedef void (*FuncFunction) (const word inAddr, const byte inFuncNo, const byte inFuncPower);
 
 
 
 typedef struct 
 {
     unsigned char mCmd;
     word mParam1;
     word mParam2;
     word mParam3;
     word mParam4;
     unsigned char mWait;
 } PackedDScmd;
 
 
 
 class DSSequence {
 
   private:
     
     word gButtonBuf;
     byte gDisplayFlag;
     PackedDScmd gDScmd;
     byte gLastManID;
     
     bool checkButton(word inBuf, word inCurrent, word inButtonBit);
     bool checkButtonLeaved(word inBuf, word inCurrent, word inButtonBit);
     bool checkButtonContinuous(word inBuf, word inCurrent, word inButtonBit);
     word getButtonStatus();
     void loadEEPROM();
     void saveEEPROM(byte inMode);
     void interval_always(word inButtonStatus);
     void interval_Main(word inButtonStatus);
     void interval_Manufacture(word inButtonStatus);
     void interval_ReadCV(word inButtonStatus);
     void interval_WriteCV(word inButtonStatus);
     void interval_ReadWriteCV(word inButtonStatus);
     void interval_Signal(word inButtonStatus);
     void interval_CheckAddress(word inButtonStatus);
     void interval_WriteAddress(word inButtonStatus);
     void interval_Config(word inButtonStatus);
     void interval_LocCtrl(word inButtonStatus);
     void interval_AccCtrl(word inButtonStatus);
     void interval_FactoryReset(word inButtonStatus);
     void interval_ErrorStatus(word inButtonStatus);
     void StrCpyFromROM(char *outpText, const char *inpROMText, byte inLen);
     word ProcessCheck( byte inTempA, byte inTempB, byte inTempC);
     byte GetButtonsFromAD(unsigned short inADData);
     word count;     

   public:
     word Address;
     word CVNo;
     word CVValue;
     word SignalAddr[6];
 
     byte gMode;
     byte gNo_MainMenu;
     byte gNo_CVReadMenu;
     byte gNo_CVWriteMenu;
     byte gNo_CVReadWriteMenu;
     byte gNo_AddrChkMenu;
     byte gNo_AddrWriteMenu;
     byte gNo_NSignalMenu;
     byte gNo_ConfigMenu;
     byte gNo_AccCtrlMenu;
     byte gNo_LocCtrlMenu;
     byte gNo_ManCtrlMenu;
     byte gNo_FactoryResetMenu;
     
     //Locomotive
     byte gLoc_Dir;
     word gLoc_Spd;
     uint32_t gLoc_Func;
     word gLoc_Addr;
     
     word gAcc_Addr;
     byte gAcc_Dir;
     byte gAcc_FuncBuf[128];
     
     byte gCfgReadSpeed;
     byte gCfgReadPower;
     word gCfgThreshold;
     word gCfgRetry;
     
     DSSequence();
     byte Interval();
     PackedDScmd GetCommad();
     bool editNums(word inButtonStatus, word *iopNum, word inMax);
     bool editSpds(word inButtonStatus, word *iopNum);
     
     ExeFunction CVExeFunc;
     PowerFunction ChangedPowerStatus;
     SpeedFunction ChangedTargetSpeed;
     FuncFunction ChangedTargetFunc;
     DirFunction ChangedTargetDir;
     DirFunction ChangedTurnout;
 
 
     
 
 };
 