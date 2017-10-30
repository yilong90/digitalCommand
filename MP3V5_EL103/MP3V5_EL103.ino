//
// MP3 Sound Decoder V5 EL1 Rev.exp.02
// Copyright(C)'2016- Nagoden / Desktop Station / Fujigaya2 / MECY 
//

#include <Arduino.h>
#include <string.h>
#include <avr/pgmspace.h> 
#include <avr/eeprom.h>
#include "motor_func.h"
#include "motor_ctrl.h"
#include "VVVF_sound.h"

#include <SoftwareSerial.h>
#include "NmraDcc_x.h"
//#include "TimerOne_x.h"
#include "DFPlayer_Mini_Mp3_x.h"

/*************************************************
 * 
 * Declaration of Symbols
 * 
 *************************************************/

//#define DEBUG			//リリースのときはコメントアウトすること
#define MAN_VER_NUMBER  03   /* Release Ver CV07 */
#define MAN_ID_NUMBER 108  /* Manufacture ID */


#define DECODER_ADDRESS 3

#define PIN_LIGHT	A1
#define PIN_HEADF1	A2
#define PIN_HEADF2	A3
#define PIN_HEADD1	A4
#define PIN_HEADD2	A5

#define PIN_AMPUP	5	// アンプボリュームUP
#define PIN_AMPDOWN	6	// アンプボリュームDOWN


#define PIN_BUSY 7

#define MOTOR_LIM_MAX 255

#define CV_VSTART 2
#define CV_ACCRATIO 3
#define CV_DECCRATIO  4
#define CV_VMAX 5
#define CV_DECODER_MODE  30       //ファンクションモード
#define CV_SOUNDSWITCH 47//fujigaya2
#define CV_SOUNDSTATE 48//fujigaya2
#define CV_SOUNDNOTCH 49//yaasan
#define CV_SOUNDVOLUME 50//fujigaya2
#define CV_SOUNDRailInt 51//fujigaya2

#define CV_BEMFcoefficient  54
#define CV_PI_P 55
#define CV_PI_I 56
#define CV_BEMFCUTOFF 57
#define CV_MP3_Vol 58             //MP3 Volume
#define CV_AMP_Vol 59             //AMP Volume
#define CV_MP3_F0 110             //F00 Function
#define CV_MP3_F1 111             //F01 Function
#define CV_MP3_F2 112             //F02 Function
#define CV_FUNCSTATE 200

#define WAIT_VOLMP3		20        //MP3 Volumeの通信ウェイト

/*************************************************
 * 
 * Declaration of Classes
 * 
 *************************************************/

NmraDcc	 Dcc;
DCC_MSG	 Packet;

/*************************************************
 * 
 * Declaration of Variables
 * 
 *************************************************/


//Task Schedule
unsigned long gPreviousL1 = 0;
unsigned long gPreviousL2 = 0;
unsigned long gPreviousL3 = 0;
unsigned long gPreviousL4 = 0;

unsigned long gPreviousL5 = 0; //161117 fujigaya2
unsigned long gPreviousL6 = 0; //170419 fujigaya2
unsigned long gIntervalRail = 1000;

//Mp3Set用
//boolean mp3_init_volume_set = true;

//モータ制御関連の変数

uint16_t gSpeedCmd = 0;
uint16_t gSpeedCmd_vvvf = 0;
uint16_t gSpeedCmd_rail = 0;//fujigaya2
uint8_t gPwmDirv = 128;
uint8_t gCV29_Vstart = 0;
uint8_t gCV29Direction = 0;

// CV

uint8_t gCV1_SAddr = 3;
uint8_t gCVx_LAddr = 3;
uint8_t gCV2_Vstart = 20;
uint8_t gCV3_AccRatio = 16;
uint8_t gCV4_DecRatio = 16;
uint8_t gCV5_VMAX = 255;

uint8_t gCV47_SoundSwitch = 2;//２：吊り掛け音
uint8_t gCV48_SoundState = 1;//0:レール継ぎ目音なし、1：レール継ぎ目音あり
uint8_t gCV49_SoundNotch = 1;//0:ノッチオフあり,1:ノッチオフなし
uint8_t gCV50_SoundVolume = 7;//ボリューム　0-8
uint8_t gCV51_SoundRailInt = 50;///レール継ぎ目音用最高速　0-100
//uint8_t gCV54_BEMFcoefficient = 38; // 0-127 step / MECY 2016/12/24
uint8_t gCV54_BEMFcoefficient = 72; // 0-127 step / Reajust by MECY 2017/04/16
uint8_t gCV55_PI_P = 32;
uint8_t gCV56_PI_I = 96;
uint8_t gCV57_BEMF_CutOff = 0;  // 0でBEMF Off 1でBEMF On

//MP3 Control
uint8_t gCV58_MP3_Vol = 26;
uint8_t gCVMP3Table[29] = {0};


//ファンクションの変数
uint8_t gFuncBits[29] = {0};

//アンプボリューム調整
uint8_t gAmpVol_Up;
uint8_t gAmpVol_Down;

//ブレーキ音用
int speed_state = 1;

struct CVPair{
  uint16_t	CV;
  uint8_t	Value;
};

CVPair FactoryDefaultCVs [] = {
  {CV_MULTIFUNCTION_PRIMARY_ADDRESS, DECODER_ADDRESS},
  {CV_ACCESSORY_DECODER_ADDRESS_MSB, 0},		//The LSB is set CV 1 in the libraries .h file, which is the regular address location, so by setting the MSB to 0 we tell the library to use the same address as the primary address. 0 DECODER_ADDRESS
  {CV_MULTIFUNCTION_EXTENDED_ADDRESS_MSB, 0},	 //XX in the XXYY address
  {CV_MULTIFUNCTION_EXTENDED_ADDRESS_LSB, 0},	 //YY in the XXYY address
  {CV_29_CONFIG, 2},	 //Make sure this is 0 or else it will be random based on what is in the eeprom which could caue headaches
//  {CV_29_CONFIG, 128},   //Make sure this is 0 or else it will be random based on what is in the eeprom which could caue headaches
  {CV_VSTART, 32},
  {CV_ACCRATIO, 4},
  {CV_DECCRATIO, 4},
  {CV_VMAX, 200},
  {CV_DECODER_MODE,0},
  {CV_SOUNDSWITCH, 2},
  {CV_SOUNDSTATE, 1},
	{CV_SOUNDNOTCH, 2},
  {CV_SOUNDVOLUME, 7},
  {CV_SOUNDRailInt,100},
  {CV_BEMFcoefficient, 72},
  {CV_PI_P, 32},
  {CV_PI_I, 96},
  {CV_BEMFCUTOFF,0},
  {CV_MP3_Vol,26}, //5V PAM must be 28 or less
	{CV_AMP_Vol, 23},
//  {CV_MP3_F0,0},
//  {CV_MP3_F1,0},
  {CV_MP3_F2 + 0, 0},
  {CV_MP3_F2 + 1, 0},
  {CV_MP3_F2 + 2, 0},
  {CV_MP3_F2 + 3, 0},
  {CV_MP3_F2 + 4, 0},
  {CV_MP3_F2 + 5, 0},
  {CV_MP3_F2 + 6, 0},
  {CV_MP3_F2 + 7, 0},
  {CV_MP3_F2 + 8, 0},
  {CV_MP3_F2 + 9, 0},
  {CV_MP3_F2 + 10, 1},
  {CV_MP3_F2 + 11, 1},
  {CV_MP3_F2 + 12, 1},
  {CV_MP3_F2 + 13, 1},
  {CV_MP3_F2 + 14, 1},
  {CV_MP3_F2 + 15, 1},
  {CV_MP3_F2 + 16, 1},
  {CV_MP3_F2 + 17, 1},
  {CV_MP3_F2 + 18, 1},
  {CV_MP3_F2 + 19, 1},
  {CV_MP3_F2 + 20, 1},
  {CV_MP3_F2 + 21, 1},
  {CV_MP3_F2 + 22, 1},
  {CV_MP3_F2 + 23, 1},
  {CV_MP3_F2 + 24, 1},
  {CV_MP3_F2 + 25, 1},
  {CV_MP3_F2 + 26, 1},
	{200 + 0, 0}, // Function status
	{200 + 1, 0},
	{200 + 2, 0},
	{200 + 3, 0},
	{200 + 4, 0},
	{200 + 5, 0},
	{200 + 6, 0},
	{200 + 7, 0},
	{200 + 8, 0},
	{200 + 9, 0},
	{200 + 10, 0},
	{200 + 11, 0},
	{200 + 12, 0},
	{200 + 13, 0},
	{200 + 14, 0},
	{200 + 15, 0},
	{200 + 16, 0},
	{200 + 17, 0},
	{200 + 18, 0},
	{200 + 19, 0},
	{200 + 20, 0},
	{200 + 21, 0},
	{200 + 22, 0},
	{200 + 23, 0},
	{200 + 24, 0},
	{200 + 25, 0},
	{200 + 26, 0},
	{200 + 27, 0},
	{200 + 28, 0}
};

/*************************************************
 * 
 * Declaration of Functions
 * 
 *************************************************/

void(* resetFunc) (void) = 0;  //declare reset function at address 0

uint8_t FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs) / sizeof(CVPair);

void ControlRoomLight(void);
void ControlHeadLight(void);

void Play_VolDown(uint8_t inVolDown);
void Play_VolUp(uint8_t inVolUp);


/*************************************************
 * 
 * Functions
 * 
 *************************************************/

void notifyCVResetFactoryDefault()
{
	//When anything is writen to CV8 reset to defaults. 

	resetCVToDefault();	 
#ifdef DEBUG
	Serial.println("Resetting...");
#endif
	delay(1000);  //typical CV programming sends the same command multiple times - specially since we dont ACK. so ignore them by delaying
	resetFunc();
}


void resetCVToDefault()
{
	//CVをデフォルトにリセット
#ifdef DEBUG
	Serial.println("CVs being reset to factory defaults");
#endif
	
	for (int j=0; j < FactoryDefaultCVIndex; j++ ){
		Dcc.setCV( FactoryDefaultCVs[j].CV, FactoryDefaultCVs[j].Value);
	}
}



extern void	   notifyCVChange( uint16_t CV, uint8_t Value)
{
   //CVが変更されたときのメッセージ
#ifdef DEBUG
   Serial.print("CV "); 
   Serial.print(CV); 
   Serial.print(" Changed to "); 
   Serial.println(Value, DEC);
#endif
}

void notifyCVAck(void)
{
#ifdef DEBUG
  Serial.println("notifyCVAck");
#endif
  
	MOTOR_Ack();
}


void setup()
{
	
	// ソフトウエアシリアル通信レートセット:
	Serial.begin(9600);
	mp3_set_serial (Serial);	//set softwareSerial for DFPlayer-mini mp3 module 

	pinMode(PIN_BUSY, INPUT);  //Mp3 Busy pin 設定
	digitalWrite(PIN_BUSY, HIGH); //Internal pull up enabled

	//VVVFハード設定
	VVVF_Setup();


	//ファンクションの割り当てピン初期化
	pinMode(PIN_LIGHT, OUTPUT);
	digitalWrite(PIN_LIGHT, 0);
	
	pinMode(PIN_HEADF1, OUTPUT);
	digitalWrite(PIN_HEADF1, 0);
	
	pinMode(PIN_HEADF2, OUTPUT);
	digitalWrite(PIN_HEADF2, 0);	
	
	pinMode(PIN_HEADD1, OUTPUT);
	digitalWrite(PIN_HEADD1, 0);
	
	pinMode(PIN_HEADD2, OUTPUT);
	digitalWrite(PIN_HEADD2, 0);
	
	//PAM8408アンプ用ボリューム
	pinMode(PIN_AMPUP, OUTPUT);
	pinMode(PIN_AMPDOWN, OUTPUT);
	digitalWrite(PIN_AMPUP, 1);
	digitalWrite(PIN_AMPDOWN, 1);
	
	if ( Dcc.getCV(CV_MULTIFUNCTION_PRIMARY_ADDRESS) == 0xFF )
	{
		//if eeprom has 0xFF then assume it needs to be programmed
#ifdef DEBUG
	  Serial.println("CV Defaulting due to blank eeprom");
#endif
		
		
	  notifyCVResetFactoryDefault();
	  
   } else{
#ifdef DEBUG
	 Serial.println("CV Not Defaulting");
#endif
   }
  
	// Setup which External Interrupt, the Pin it's associated with that we're using, disable pullup.
	Dcc.pin(0, 2, 0);

	// Call the main DCC Init function to enable the DCC Receiver
	Dcc.init( MAN_ID_NUMBER, MAN_VER_NUMBER,   FLAGS_MY_ADDRESS_ONLY , 0 ); 

	//Reset task
	gPreviousL1 = millis();
	gPreviousL2 = millis();
	gPreviousL3 = millis();
	gPreviousL4 = millis();
	gPreviousL5 = millis();

	//Init CVs
	gCV1_SAddr = Dcc.getCV( CV_MULTIFUNCTION_PRIMARY_ADDRESS ) ;
	gCV2_Vstart = Dcc.getCV( CV_VSTART ) ;
	gCV3_AccRatio = Dcc.getCV( CV_ACCRATIO ) ;
	gCV4_DecRatio = Dcc.getCV( CV_DECCRATIO ) ;
	gCV5_VMAX = Dcc.getCV( CV_VMAX ) ;
	gCVx_LAddr = (Dcc.getCV( CV_MULTIFUNCTION_EXTENDED_ADDRESS_MSB ) << 8) + Dcc.getCV( CV_MULTIFUNCTION_EXTENDED_ADDRESS_LSB );
	
	gCV29_Vstart = Dcc.getCV( CV_29_CONFIG ) ;
	gCV47_SoundSwitch = Dcc.getCV(CV_SOUNDSWITCH);
	gCV48_SoundState = Dcc.getCV(CV_SOUNDSTATE);
	gCV49_SoundNotch = Dcc.getCV(CV_SOUNDNOTCH);
	gCV50_SoundVolume = Dcc.getCV(CV_SOUNDVOLUME);
	gCV51_SoundRailInt = Dcc.getCV(CV_SOUNDRailInt);

	gCV54_BEMFcoefficient =  Dcc.getCV( CV_BEMFcoefficient ) ;
	gCV55_PI_P = Dcc.getCV( CV_PI_P ) ;
	gCV56_PI_I = Dcc.getCV( CV_PI_I ) ;
	gCV57_BEMF_CutOff = Dcc.getCV( CV_BEMFCUTOFF ) ;
	gCV58_MP3_Vol = Dcc.getCV( CV_MP3_Vol ) ;
	
	/* アンプボリューム調整 */
	uint8_t aAmpVol = Dcc.getCV( CV_AMP_Vol);
	
	if( aAmpVol > 21)
	{
		gAmpVol_Up = aAmpVol - 21;
		gAmpVol_Down = 0;
		
	}
	else if( aAmpVol < 21)
	{
		gAmpVol_Up = 0;
		gAmpVol_Down = 21 - aAmpVol;
	}
	else
	{
		//何もしない(Default, 12dB)
		gAmpVol_Up = 0;
		gAmpVol_Down = 0;
	}
	
	
	/* MP3ファンクションテーブル初期化・読み取り */
	gCVMP3Table[0] = 0;
	gCVMP3Table[1] = 0;
	
	for( int i = 0; i < 27; i++)
	{
		gCVMP3Table[i + 2] = Dcc.getCV( CV_MP3_F2 + i ) ;
		
		/* 異常値は0に固定 */
		if( gCVMP3Table[i + 2] > 2)
		{
			gCVMP3Table[i + 2] = 0;
		}
		
	}
	
	//ファンクション状態をロード
	for( int i = 0; i < 29; i++)
	{
		gFuncBits[i] = Dcc.getCV( CV_FUNCSTATE + i ) ;
		
		/* 異常値は0に固定 */
		if( gFuncBits[i] > 1)
		{
			gFuncBits[i] = 0;
		}
	}

	//cv29 Direction Check
	if ( (gCV29_Vstart & 0x01) > 0)
	{
		gCV29Direction = 1;//REVをFWDにする
	}
	else
	{
		gCV29Direction = 0;//FWDをFWDにする
	}

	MOTOR_Init();
	MOTOR_SetCV(2, gCV2_Vstart);
	MOTOR_SetCV(3, gCV3_AccRatio);
	MOTOR_SetCV(4, gCV4_DecRatio);
	MOTOR_SetCV(5, gCV5_VMAX);
	MOTOR_SetCV(54, gCV54_BEMFcoefficient);        //Nagoden Adjust1 2016/06/02
	VVVF_SetCV(3, gCV3_AccRatio);
	VVVF_SetCV(4, gCV4_DecRatio);
	VVVF_SetCV(47, gCV47_SoundSwitch);        // 160917 fujigaya2
	VVVF_SetCV(48, gCV48_SoundState);        // 160917 fujigaya2
	VVVF_SetCV(49, gCV49_SoundNotch);        //161010 Yaasan
	VVVF_SetCV(50, gCV50_SoundVolume);       //161108 fujigaya2
	MOTOR_SetCV(55, gCV55_PI_P);				// MECY Adjust1 2016/04/09
	MOTOR_SetCV(56, gCV56_PI_I);				// MECY Adjust1 2016/04/09
	MOTOR_SetCV(57, gCV57_BEMF_CutOff);
	
	//VVVFをCVで初期化
	VVVF_Init();
	
   
#ifdef DEBUG
	Serial.print("CV1(ShortAddr): ");
	Serial.println(gCV1_SAddr);
	Serial.print("CV17/18(LongAddr): ");
	Serial.println(gCVx_LAddr);
	Serial.print("CV2(Vstart): ");
	Serial.println(gCV2_Vstart);
	Serial.print("CV3(AccRatio): ");
	Serial.println(gCV3_AccRatio);
	Serial.print("CV4(DecRatio): ");
	Serial.println(gCV4_DecRatio);
	Serial.print("CV5(VoltageMAX): ");
	Serial.println(gCV5_VMAX);

	Serial.print(F("CV47(SoundSwitch): "));
	Serial.println(gCV47_SoundSwitch);
	Serial.print(F("CV48(SoundState): "));
	Serial.println(gCV48_SoundState);
  
	Serial.print("CV54 (BEMF coefficient): ");
	Serial.println(gCV54_BEMFcoefficient);
	Serial.print("CV55 (PI P Gain): ");
	Serial.println(gCV55_PI_P);
	Serial.print("CV56 (PI I gain): ");
	Serial.println(gCV56_PI_I);
	Serial.print("CV57 (BEMF Cut Off): ");
	Serial.println(gCV57_BEMF_CutOff);
	Serial.print("CV58 (MP3 Volume): ");
	Serial.println(gCV58_MP3_Vol);

	Serial.println("Ready");
#endif

}

void loop()
{
	
	// You MUST call the NmraDcc.process() method frequently from the Arduino loop() function for correct library operation
	Dcc.process();
	
	if( (millis() - gPreviousL1) >= 10)
	{
		//Speed detection
		MOTOR_Sensor();
		
		//Reset task
		gPreviousL1 = millis();

	}	

	if( (millis() - gPreviousL2) >= 33)
	{
		//VVVF control
		VVVF_Cont(gSpeedCmd_vvvf, gFuncBits[3]);
    //Interval_Rail [ms]
    //255で正規化されているはずなのでとりあえず最高速100km/h固定として、
    //int kmph = gSpeedCmd_vvvf * 100 / 255;
    //int mps = kmph * 1000 / 3600;
    //int t= 25 / mps;
		gSpeedCmd_rail = (gSpeedCmd_vvvf * gCV51_SoundRailInt) / 100L;  
		if(gSpeedCmd_rail != 0)
		{
			gIntervalRail =  (255L * 900L) / (unsigned long)gSpeedCmd_rail - 200;
		}
		//Reset task
		gPreviousL2 = millis();

	} 
	
	if( (millis() - gPreviousL3) >= 100)
	{
		//Motor drive control
		MOTOR_Main(gSpeedCmd, gPwmDirv);
		
		//Reset task
		gPreviousL3 = millis();   
	}
	
	
	if( (millis() - gPreviousL4) >= 250)
	{	
		ControlRoomLight();
		ControlHeadLight();
		
		//アンプボリューム調整
		if(gAmpVol_Up > 0)
		{
			Play_VolUp();
			
			gAmpVol_Up--;
		}
		
		
		if( gAmpVol_Down > 0)
		{
			Play_VolDown();
			
			gAmpVol_Down--;
		}
		
		
		//Reset task
		gPreviousL4 = millis();
	}

  if( (millis() - gPreviousL5) >= gIntervalRail)
  { 
    //Reset task
    gPreviousL5 = millis();
    //Rail Sound
    if((gCV48_SoundState & 0x01) != 0)
    {
      //if(mp3_init_volume_set == true)
      //{
      //  //Set initial volume
      //  PlayMP3_SetVolume();
      //  mp3_init_volume_set = false;
      //}
      if(gSpeedCmd_rail != 0)
      {
        Play_RailSound(gSpeedCmd_rail);
      }
    }
  }

  if( (millis() - gPreviousL6) >= 100)
  {
    if((gCV48_SoundState & 0x02) != 0)//170805 fujigaya2
    {
      //gSpeedCmd_vvvfは255最大
      if ( gSpeedCmd_vvvf >= 25) // ブレーキ音発生イベント検出
      {
        speed_state = 1;
      }
      if ( gSpeedCmd_vvvf <= 24 && speed_state == 1)
      {
        speed_state = 2;
        mp3_set_volume (gCV58_MP3_Vol);
        delay (WAIT_VOLMP3);
        mp3_play (9); //　ブレーキ音 
      }
    }
    //Reset task
    gPreviousL6 = millis();
  }
  
}



//DCC速度信号の受信によるイベント
extern void notifyDccSpeed( uint16_t Addr, DCC_ADDR_TYPE AddrType, uint8_t Speed, DCC_DIRECTION Dir, DCC_SPEED_STEPS SpeedSteps )
{
	
	
	uint16_t aSpeedRef = 0;
	
	//速度値の正規化(255を100%とする処理)
	if( Speed >= 1)
	{
		aSpeedRef = ((Speed - 1) * 255) / SpeedSteps;
	}
	else
	{
		//緊急停止信号受信時の処理 //Nagoden comment 2016/06/11
#ifdef DEBUG
		Serial.println("***** Emagency STOP **** ");
#endif
		aSpeedRef = 0;
	}

  //VVVF用
  gSpeedCmd_vvvf = aSpeedRef; 
	//リミッタ //Nagoden Adjust1 2016/06/11
	//if(aSpeedRef > gCV5_VMAX)
	//{
		//aSpeedRef =gCV5_VMAX;
    aSpeedRef = aSpeedRef * gCV5_VMAX / 255; 
	//}
  
	
	gSpeedCmd = aSpeedRef;
  gPwmDirv = Dir;
  if ( gCV29Direction > 0)
  {
     if ( Dir == DCC_DIR_FWD)
     {
          gPwmDirv = 128;
     }
    else
    {
          gPwmDirv = 0;     
    }     
  }
  else
  {
     if ( Dir == DCC_DIR_FWD)
     {
          gPwmDirv = 0;
     }
    else
    {
         gPwmDirv =128;     
    }     
  }

#ifdef DEBUG
	// デバッグメッセージ
	Serial.print("Speed - ADR: ");
	Serial.print(Addr);
	Serial.print(", SPD: ");
	Serial.print(Speed);
	Serial.print(", DIR: ");
	Serial.print(ForwardDir);
	Serial.print(", MAX: ");
	Serial.println(MaxSpeed);
#endif

}

//ファンクション信号受信のイベント
extern void notifyDccFunc( uint16_t Addr, DCC_ADDR_TYPE AddrType, FN_GROUP FuncGrp, uint8_t FuncState)
{
	switch(FuncGrp)
	{
    case FN_0_4:
//      Function F00
         if  ((FuncState & FN_BIT_00) == 16)
         {
            if (gFuncBits[0] == 0)
            {
              gFuncBits[0] = 1;
         		Dcc.setCV(CV_FUNCSTATE + 0, 1);
            }
         }    
         else
         {
            if (gFuncBits[0] == 1) 
            {
              gFuncBits[0] = 0;
       		Dcc.setCV(CV_FUNCSTATE + 0, 0);

            }
         }
             
//      Function F01
         if  ((FuncState & FN_BIT_01) == 1){
            if (gFuncBits[1] == 0)
            {
              gFuncBits[1] = 1;
         		Dcc.setCV(CV_FUNCSTATE + 1, 1);
            }
         }    
         else
         {
            if (gFuncBits[1] == 1) 
            {
              gFuncBits[1] = 0;
       		Dcc.setCV(CV_FUNCSTATE + 1, 0);

            }
         }
//      Function F02
         if  ((FuncState & FN_BIT_02) == 2)
		 {
         	PlayMP3_P(2);
         }    
         else 
		{
		 	PlayMP3_N(2);
         }
//      Function F03
         if  ((FuncState & FN_BIT_03) == 4)  
         {
		 	/* CV49=2の時はMP3を再生しない(手動ノッチオフ) */
		 	if( gCV49_SoundNotch != 2)
		 	{
         		PlayMP3_P(3);
		 	}
		 	else
		 	{
		 		gFuncBits[3] = 1;
		 	}         
         }    
         else
         {
		 	/* CV49=2の時はMP3を再生しない(手動ノッチオフ) */
		 	if( gCV49_SoundNotch != 2)
		 	{
		 		PlayMP3_N(3);
		 	}
			else
			{
		 		gFuncBits[3] = 0;
			}         
         }
//      Function F04
         if ((FuncState & FN_BIT_04) == 8)
         {
         	PlayMP3_P(4);
         }    
         else
         {
         	PlayMP3_N(4);
         }
      break;
      
    case FN_5_8:
//      Function F05    
         if  ((FuncState & FN_BIT_05) == 1)
         {
           	PlayMP3_P(5);
         }    
         else
         {
           	PlayMP3_N(5);
         }
//      Function F06    
         if  ((FuncState & FN_BIT_06) == 2) 
         {
           	PlayMP3_P(6);
         }    
         else 
         {
           	PlayMP3_N(6);
         }
//      Function F07    
         if  ((FuncState & FN_BIT_07) == 4) 
         {
           	PlayMP3_P(7);
         }    
         else 
         {
           	PlayMP3_N(7);
         }
//      Function F08    
         if  ((FuncState & FN_BIT_08) == 8)
         {
           	PlayMP3_P(8);
         }    
         else
         {
           	PlayMP3_N(8);
         }

      break;
    case FN_9_12:
//      Function F09    
         if  ((FuncState & FN_BIT_09) == 1) 
         {
           	PlayMP3_P(9);
         }    
         else
         {
           	PlayMP3_N(9);

         }
//      Function F10    
         if  ((FuncState & FN_BIT_10) == 2) 
         {
           	PlayMP3_P(10);
         }    
         else 
         {
           	PlayMP3_N(10);

         }
//      Function F11    
         if  ((FuncState & FN_BIT_11) == 4) 
         {
            	PlayMP3_P(11);
         }    
         else 
         {
            	PlayMP3_N(11);
         }

//      Function F12    
         if  ((FuncState & FN_BIT_12) == 8) 
         {
           	PlayMP3_P(12);
         }    
         else 
         {
           	PlayMP3_N(12);
         }

      break;

    case FN_13_20:
//      Function F13
         if  ((FuncState & FN_BIT_13) == 1) 
         {
           	PlayMP3_P(13);
         }    
         else 
         {
           	PlayMP3_N(13);

         }
//      Function F14    
         if  ((FuncState & FN_BIT_14) == 2) 
         {
           	PlayMP3_P(14);
         }    
         else 
		{
           	PlayMP3_N(14);

         }
//      Function F15    
         if  ((FuncState & FN_BIT_15) == 4) 
         {
           	PlayMP3_P(15);
         }    
         else 
         {
           	PlayMP3_N(15);
         }

//      Function F16    
         if  ((FuncState & FN_BIT_16) == 8) 
         {
           	PlayMP3_P(16);
         }    
         else 
         {
           	PlayMP3_N(16);
         }
//      Function F17    
         if  ((FuncState & FN_BIT_17) == 16) 
         {
           	PlayMP3_P(17);
         }    
         else 
         {
           	PlayMP3_N(17);
         }
//      Function F18    
         if  ((FuncState & FN_BIT_18) == 32) 
         {
           	PlayMP3_P(18);
         }    
         else 
         {
           	PlayMP3_N(18);
         }
//      Function F19    
         if  ((FuncState & FN_BIT_19) == 64) 
         {
           	PlayMP3_P(19);
         }    
         else 
         {
           	PlayMP3_N(19);
         }

 //      Function F20    
        if  ((FuncState & FN_BIT_20) == 128) 
        {
           	PlayMP3_P(20);
         }    
         else 
         {
           	PlayMP3_N(20);
         }
      break;
      
    case FN_21_28:
//      Function F21    

         if  ((FuncState & FN_BIT_21) == 1) 
         {
           	PlayMP3_P(21);
         }    
         else 
         {
           	PlayMP3_N(21);
         }
//      Function F22    
         if  ((FuncState & FN_BIT_22) == 2) 
         {
           	PlayMP3_P(22);
         }    
         else 
         {
           	PlayMP3_N(22);
         }
//      Function F23    
         if  ((FuncState & FN_BIT_23) == 4) 
         {
           	PlayMP3_P(23);
         }    
         else 
         {
           	PlayMP3_N(23);

         }
//      Function F24    
         if  ((FuncState & FN_BIT_24) == 8) 
         {
           	PlayMP3_P(24);
         }    
         else 
         {
           	PlayMP3_N(24);
         }
//      Function F25    
         if  ((FuncState & FN_BIT_25) == 16) 
         {
           	PlayMP3_P(25);
         }    
         else 
         {
           	PlayMP3_N(25);
         }
//      Function F26    
         if  ((FuncState & FN_BIT_26) == 32) 
         {
           	PlayMP3_P(26);
         }    
         else 
         {
           	PlayMP3_N(26);
         }
//      Function F27    
         if  ((FuncState & FN_BIT_27) == 64) 
         {
           	PlayMP3_P(27);
         }    
         else 
         {
           	PlayMP3_N(27);
         }
 //      Function F28    
         if  ((FuncState & FN_BIT_28) == 128) 
         {
           	PlayMP3_P(28);
         }    
         else
         {
           	PlayMP3_N(28);
         }
     break;
	}
  
}

void exec_function (int function, int pin, int FuncState)
{
	//digitalWrite (pin, FuncState);
}


void PlayMP3_P(uint8_t inMusicNo)
{
	PlayMP3_PosiEdge( inMusicNo, &gFuncBits[inMusicNo], gCVMP3Table[inMusicNo]);
	Dcc.setCV(CV_FUNCSTATE + inMusicNo, 1);
}

void PlayMP3_N(uint8_t inMusicNo)
{
	PlayMP3_NegaEdge( inMusicNo, &gFuncBits[inMusicNo], gCVMP3Table[inMusicNo]);
	Dcc.setCV(CV_FUNCSTATE + inMusicNo, 0);
}


void PlayMP3_PosiEdge(uint8_t inMusicNo, uint8_t *iopFnBit, uint8_t inModeCV)
{

	/* 0: ON/OFFのエッジで再生 */
	/* 1: ONレベルで再生(リピート無し) */
	/* 2: ONレベルで再生(リピート有り) */
	
	if ( *iopFnBit == 0)
	{
		*iopFnBit = 1;
		mp3_set_volume (gCV58_MP3_Vol);
		delay (WAIT_VOLMP3);
		mp3_play (inMusicNo);
		
		if (inModeCV == 2)
		{
			delay (WAIT_VOLMP3);
			mp3_single_loop (true);
		}
	}
	else
	{
		/* 何もしない */
	}
}

void PlayMP3_NegaEdge(uint8_t inMusicNo, uint8_t *iopFnBit, uint8_t inModeCV)
{

	if ( *iopFnBit == 1)
	{
		*iopFnBit = 0;
		
		if ((inModeCV == 2) || (inModeCV == 1))
		{
			/* 1: ONレベルで再生(リピート無し) */
			/* 2: ONレベルで再生(リピート有り) */
			mp3_single_loop (false);
			delay (WAIT_VOLMP3);
			mp3_stop () ;
			delay (WAIT_VOLMP3);
		}
		else if (inModeCV == 0)
		{
			/* 0: ON/OFFのエッジで再生 */
			mp3_set_volume (gCV58_MP3_Vol);
			delay (WAIT_VOLMP3);
			mp3_play (inMusicNo) ;
			delay (WAIT_VOLMP3);
		}
		else
		{
			mp3_stop() ;
		}
	}
	else
	{
		//何もしない
	}
}


void ControlRoomLight(void)
{
	
	if(gFuncBits[1] == 0)
	{
		digitalWrite(PIN_LIGHT, 0);
		
	}
	else
	{
		digitalWrite(PIN_LIGHT, 1);
	}
	
}

void ControlHeadLight(void)
{
	
	if(gFuncBits[0] == 0)
	{
		digitalWrite(PIN_HEADF1, 0);
		digitalWrite(PIN_HEADF2, 0);	
		digitalWrite(PIN_HEADD1, 0);
		digitalWrite(PIN_HEADD2, 0);
		
		return;
	}
	
	
	
	if( gPwmDirv == 0)
	{
		//FWD
		digitalWrite(PIN_HEADF1, 1);
		digitalWrite(PIN_HEADF2, 0);	
		digitalWrite(PIN_HEADD1, 0);
		digitalWrite(PIN_HEADD2, 1);	
	
	}
	else
	{
		//REV
		digitalWrite(PIN_HEADF1, 0);
		digitalWrite(PIN_HEADF2, 1);	
		digitalWrite(PIN_HEADD1, 1);
		digitalWrite(PIN_HEADD2, 0);	
	}
}


//void PlayMP3_SetVolume()
//{
//  mp3_set_volume (gCV58_MP3_Vol);
//}


void PlayMP3_Simple(uint8_t num)
{
	//Busyが立っている＝ほかの音がなっているときは鳴らさない。
	if(digitalRead(PIN_BUSY) == HIGH)
	{
		//MP3 Volume Set
		mp3_set_volume (gCV58_MP3_Vol);
		delay(WAIT_VOLMP3);
		mp3_play (num);
	}
}

void Play_RailSound(uint16_t t_spd)
{
	//255で正規化されているはずなのでとりあえず最高速100km/h固定として、
	int t_switch = t_spd / 26;//0 - 9
	PlayMP3_Simple(101 + t_switch);// 101 - 110
}

void Play_VolUp(void)
{
	digitalWrite(PIN_AMPUP, 0);
	delay(33);
	digitalWrite(PIN_AMPUP, 1);
}

void Play_VolDown(void)
{
	digitalWrite(PIN_AMPDOWN, 0);
	delay(33);
	digitalWrite(PIN_AMPDOWN, 1);
}
