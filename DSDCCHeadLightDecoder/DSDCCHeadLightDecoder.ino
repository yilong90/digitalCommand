// DCC Headlight Decoder for DS-DCC decode
// By yaasan
// Based on Nicolas's sketch http://blog.nicolas.cx
// Inspired by Geoff Bunza and his 17 Function DCC Decoder & updated library
//
//
// Debug serial output available on the serial port at baud 115200, aka Tools -> Serial Monitor
//

#include <NmraDcc.h>
#include <avr/eeprom.h>	 //required by notifyCVRead() function if enabled below

//各種設定、宣言

#define DECODER_ADDRESS 3
#define DCC_ACK_PIN 9	//if defined enables the ACK pin functionality. Comment out to disable.

#define MOTOR_PWM_A 9
#define MOTOR_PWM_B 10


#define MAX_PWMDUTY 200
#define MID_PWMDUTY 100


//使用クラスの宣言
NmraDcc	 Dcc;
DCC_MSG	 Packet;

//Task Schedule
unsigned long gPreviousL5 = 0;

//進行方向
uint8_t gDirection = 128;

//Function State
uint8_t gState_F0 = 0;
uint8_t gState_F1 = 0;
uint8_t gState_F2 = 0;

//モータ制御関連の変数
uint32_t gSpeedRef = 1;

//CV related
uint8_t gCV1_SAddr = 3;
uint8_t gCVx_LAddr = 3;


//Internal variables and other.
#if defined(DCC_ACK_PIN)
const int DccAckPin = DCC_ACK_PIN ;
#endif


struct CVPair{
  uint16_t	CV;
  uint8_t	Value;
};
CVPair FactoryDefaultCVs [] = {
  {CV_MULTIFUNCTION_PRIMARY_ADDRESS, DECODER_ADDRESS},
  {CV_ACCESSORY_DECODER_ADDRESS_MSB, 0},		//The LSB is set CV 1 in the libraries .h file, which is the regular address location, so by setting the MSB to 0 we tell the library to use the same address as the primary address. 0 DECODER_ADDRESS
  {CV_MULTIFUNCTION_EXTENDED_ADDRESS_MSB, 0},	 //XX in the XXYY address
  {CV_MULTIFUNCTION_EXTENDED_ADDRESS_LSB, 0},	 //YY in the XXYY address
  {CV_29_CONFIG, 128 },	 //Make sure this is 0 or else it will be random based on what is in the eeprom which could caue headaches
};

void(* resetFunc) (void) = 0;  //declare reset function at address 0


uint8_t FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs) / sizeof(CVPair);


void notifyCVResetFactoryDefault()
{
	//When anything is writen to CV8 reset to defaults. 

	resetCVToDefault();	 
	Serial.println("Resetting...");
	delay(1000);  //typical CV programming sends the same command multiple times - specially since we dont ACK. so ignore them by delaying

	resetFunc();
};


void resetCVToDefault()
{
	//CVをデフォルトにリセット
	Serial.println("CVs being reset to factory defaults");
	
	for (int j=0; j < FactoryDefaultCVIndex; j++ ){
		Dcc.setCV( FactoryDefaultCVs[j].CV, FactoryDefaultCVs[j].Value);
	}
};

void notifyCVAck(void)
{
  Serial.println("notifyCVAck");
  
  analogWrite(MOTOR_PWM_B, 0);
  analogWrite(MOTOR_PWM_A, 64);
  
  delay( 6 );  
  
  analogWrite(MOTOR_PWM_A, 0);
}

extern void	   notifyCVChange( uint16_t CV, uint8_t Value){
   //CVが変更されたときのメッセージ
   Serial.print("CV "); 
   Serial.print(CV); 
   Serial.print(" Changed to "); 
   Serial.println(Value, DEC);
};


void setup()
{
	uint8_t cv_value;
	
	//シリアル通信開始
	Serial.begin(115200);

	//D9,D10 PWM キャリア周期:31kHz
	TCCR1B &= B11111000;
	TCCR1B |= B00000001;
	
	//PWMを10bit化
	//TCCR1A |= B00000011;

	
	//PWM出力ピン D9,D10を出力にセット
	pinMode(9, OUTPUT);
	pinMode(10, OUTPUT);

	
	//DCCの応答用負荷ピン
	
	#if defined(DCCACKPIN)
	//Setup ACK Pin
	pinMode(DccAckPin,OUTPUT);
	digitalWrite(DccAckPin, 0);
	#endif
  
 

  #if !defined(DECODER_DONT_DEFAULT_CV_ON_POWERUP)
	if ( Dcc.getCV(CV_MULTIFUNCTION_PRIMARY_ADDRESS) == 0xFF ){	 //if eeprom has 0xFF then assume it needs to be programmed
	  Serial.println("CV Defaulting due to blank eeprom");
	  notifyCVResetFactoryDefault();
	  
   } else{
	 Serial.println("CV Not Defaulting");
   }
  #else
	 Serial.println("CV Defaulting Always On Powerup");
	 notifyCVResetFactoryDefault();
  #endif 
  
   
   
  // Setup which External Interrupt, the Pin it's associated with that we're using, disable pullup.
  Dcc.pin(0, 2, 0);
  
  // Call the main DCC Init function to enable the DCC Receiver
  Dcc.init( MAN_ID_DIY, 100,   FLAGS_MY_ADDRESS_ONLY , 0 ); 

  //Reset task
  gPreviousL5 = millis();
  
  //Init CVs
  /*
  gCV1_SAddr = Dcc.getCV( CV_MULTIFUNCTION_PRIMARY_ADDRESS ) ;
  gCVx_LAddr = (Dcc.getCV( CV_MULTIFUNCTION_EXTENDED_ADDRESS_MSB ) << 8) + Dcc.getCV( CV_MULTIFUNCTION_EXTENDED_ADDRESS_LSB );
  
  Serial.print("CV1(ShortAddr): ");
  Serial.println(gCV1_SAddr);
  Serial.print("CV17/18(LongAddr): ");
  Serial.println(gCVx_LAddr);
 
*/  Serial.println("Ready");
 
	 
}

void loop(){
	
	// You MUST call the NmraDcc.process() method frequently from the Arduino loop() function for correct library operation
	Dcc.process();
	

	if( (millis() - gPreviousL5) >= 100)
	{
		//Headlight control
		HeadLight_Control();
		
		//Reset task
		gPreviousL5 = millis();
		
	}
}

/* Motor control Task (10Hz) */
void HeadLight_Control()
{
	
	uint16_t aPwmRef = 0;
	
	if( gState_F0 > 0)
	{
		
		if( gState_F2 > 0)
		{
			if( gSpeedRef > 1)
			{
				aPwmRef = MAX_PWMDUTY;
			}
			else
			{
				aPwmRef = MID_PWMDUTY;
			}
		}
		else if( gState_F1 > 0)
		{
			aPwmRef = MID_PWMDUTY;
		}
		else
		{
			aPwmRef = MAX_PWMDUTY;
		}
	}
	else
	{
		aPwmRef = 0;
	}
	
	
	//PWM出力
	//進行方向でPWMのABを切り替える
	if( gDirection > 0){
		analogWrite(MOTOR_PWM_B, 0);
		analogWrite(MOTOR_PWM_A, aPwmRef);
	}
	else
	{
		analogWrite(MOTOR_PWM_A, 0);
		analogWrite(MOTOR_PWM_B, aPwmRef);
	}

}


//DCC速度信号の受信によるイベント
extern void notifyDccSpeed( uint16_t Addr, uint8_t Speed, uint8_t ForwardDir, uint8_t MaxSpeed )
{

	if( gDirection != ForwardDir)
	{
		gDirection = ForwardDir;
	}
	
	gSpeedRef = Speed;
	
	// デバッグメッセージ
	Serial.print("Speed - ADR: ");
	Serial.print(Addr);
	Serial.print(", SPD: ");
	Serial.print(Speed);
	Serial.print(", DIR: ");
	Serial.print(ForwardDir);
	Serial.print(", MAX: ");
	Serial.println(MaxSpeed);

}

//ファンクション信号受信のイベント
extern void notifyDccFunc( uint16_t Addr, FN_GROUP FuncGrp, uint8_t FuncState)
{
	
	if( FuncGrp == FN_0_4)
	{
		if( gState_F0 != (FuncState & FN_BIT_00))
		{
			//Get Function 0 (FL) state
			gState_F0 = (FuncState & FN_BIT_00);
		}
		if( gState_F1 != (FuncState & FN_BIT_01))
		{
			//Get Function 1 state
			gState_F1 = (FuncState & FN_BIT_01);
		}
		if( gState_F2 != (FuncState & FN_BIT_02))
		{
			//Get Function 0 (FL) state
			gState_F2 = (FuncState & FN_BIT_02);
		}
	}

}