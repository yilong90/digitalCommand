// Motor Control for DS-DCC decode
// By yaasan
//

#include <Arduino.h>
#include <string.h>
#include "motor_ctrl.h"
#include "motor_func.h"


/*************************************************
 * 
 * Declaration of Symbols
 * 
 *************************************************/

#define COUNT_ZEROMAX 10


/*************************************************
 * 
 * Declaration of Classes
 * 
 *************************************************/


/*************************************************
 * 
 * Declaration of Variables
 * 
 *************************************************/


//モータ制御関連の変数
long gMotorLPF_buf = 0;
long gBEMFLPF_buf = 0;

long gMotorPI_buf = 0;
long gMotorSpeed = 0;
byte gDirection = 128;
int gPrevSpeed = 0;
int gPrevSpeedRef = 0;
int gPWMRef = 0;
byte gZeroCount = 0;

MOTOR_PARAM gParam;

/*************************************************
 * 
 * Declaration of Functions
 * 
 *************************************************/

void MOTOR_Init();
void MOTOR_SetCV(byte iNo, byte inData);
void MOTOR_Sensor();
void MOTOR_Ack(void);
void MOTOR_Main(int inSpeedCmd, int inDirection);



/*************************************************
 * 
 * Functions
 * 
 *************************************************/

void MOTOR_Init()
{
	gParam.mStartVoltage = 32;
	gParam.mBEMFcoefficient = 38;
	gParam.mPI_P = 32;
	gParam.mPI_I = 96;
 	gParam.mBEMFcutoff = 32;
	gParam.mAccRatio = 4 * 16;
	gParam.mDecRatio = 4 * 16;
	
	//D3,D11 PWM キャリア周期:31kHz
	TCCR2B &= B11111000;
	TCCR2B |= B00000001;

	//PWM出力ピン D3,D11を出力にセット
	pinMode(MOTOR_PWM_B, OUTPUT);
	pinMode(MOTOR_PWM_A, OUTPUT);

	
	
}

void MOTOR_SetCV(byte iNo, byte inData)
{
	switch(iNo)
	{
	case 2:
		gParam.mStartVoltage = inData;
		break;
	case 3:
		
		if( inData > 16)
		{
//			gParam.mAccRatio = 255;
//			gParam.mAccRatio = 16;                    // Adjust CV3 Para set by MECY 2017/3/26
			gParam.mAccRatio = 160;                   // Readjust CV3 Para set by MECY 2017/4/11
		}
		else
		{
//			gParam.mAccRatio = inData * 16;
//			gParam.mAccRatio = inData ;               // Adjust CV3 Para set by MECY 2017/3/26
			gParam.mAccRatio = inData * 10 ;          // Readjust CV3 Para set by MECY 2017/4/11
		}
		
		break;
	case 4:
		if( inData > 16)
		{
//			gParam.mDecRatio = 255;
//			gParam.mDecRatio = 16;                    // Adjust CV4 Para set by MECY 2017/3/26
			gParam.mDecRatio = 160;                   // Readjust CV4 Para set by MECY 2017/4/11
		}
		else
		{
//			gParam.mDecRatio = inData * 16;
//			gParam.mDecRatio = inData ;               // Adjust CV4 Para set by MECY 2017/3/26
			gParam.mDecRatio = inData * 10;           // Readjust CV4 Para set by MECY 2017/4/11
		}
		
		break;
	case 5:
		gParam.mMaxVoltage = inData;
		break;
	case 54:
    	gParam.mBEMFcoefficient = inData;
    	break;
	case 55:
		gParam.mPI_P = inData;
		break;
	case 56:
		gParam.mPI_I = inData;
		break;
  	case 57:
//    	gParam.mBEMFcutoff = inData;

    	if(0 < inData < 20)
		{
			gParam.mBEMFcutoff = 20;                  // Adjust CV57 Para set by MECY 2017/4/16
		}
		else
		{
			gParam.mBEMFcutoff = inData;              // Adjust CV57 Para set by MECY 2017/4/16
		}
    	break;

	}
	
	
}

void MOTOR_Ack(void)
{
  
  analogWrite(MOTOR_PWM_B, 0);
  analogWrite(MOTOR_PWM_A, 250);
  
  delay( 8 );  
  
  analogWrite(MOTOR_PWM_A, 0);
}


void MOTOR_Sensor()
{
	int aBEMF_result, aSpeed_calculated;
	
	//BEMF detection
	aBEMF_result = MOTOR_GetBEMF();
	
	
	aSpeed_calculated = aBEMF_result;
	
	if(aSpeed_calculated > 255)
	{
		aSpeed_calculated = 255;
	}
	
	//Speed conversion
	gMotorSpeed = MOTOR_LPF(aSpeed_calculated, 8, &gBEMFLPF_buf);
	
	
}


/* Motor control Task (10Hz) */
void MOTOR_Main(int inSpeedCmd, int inDirection)
{
	
	int aSpeedRef = (double)inSpeedCmd * ((double)gParam.mMaxVoltage  - (double)gParam.mStartVoltage) / (double)gParam.mMaxVoltage ;
	int aPWMRef = 0;
	long aSpeedRef_offseted = 0;
	
	
	//8bit LPF
	if( gPrevSpeedRef <= aSpeedRef)
	{
		aSpeedRef = MOTOR_LPF(aSpeedRef, gParam.mAccRatio, &gMotorLPF_buf);
	}
	else
	{
		aSpeedRef = MOTOR_LPF(aSpeedRef, gParam.mDecRatio, &gMotorLPF_buf);
	}
	
	//バックアップ
	gPrevSpeedRef = inSpeedCmd;
	
	//PI Control
	if( gParam.mBEMFcutoff > 0)
	{
		
		//Speed detection
		int aSpeedDet = (gMotorSpeed * (127 - (int)gParam.mBEMFcoefficient)) / 127;     // 127 step value  /Adjust MECY 2016/12/24
		
		if( aSpeedDet < gParam.mBEMFcutoff)
		{
			//ある閾値以下は速度ゼロと見なす
			aSpeedDet = 0;
		}
		else
		{
			//検出速度のオフセットをする
			aSpeedDet = aSpeedDet - gParam.mBEMFcutoff;
		}
		
		//Command zero check
		if( aSpeedDet == 0)
		{
			gZeroCount++;
			
			if( gZeroCount > COUNT_ZEROMAX)
			{
				gZeroCount = 0;
				gMotorPI_buf = 0;
			}
		}
		else
		{
			gZeroCount = 0;
		}
			
		//BEMF時 PI制御
		if((aSpeedRef == 0) && ( aSpeedDet == 0))
		{
			aSpeedRef_offseted = MOTOR_PI(aSpeedRef - aSpeedDet, gParam.mPI_P, gParam.mPI_I, &gMotorPI_buf) ;
		}
		else if((aSpeedRef >= 1) && ( aSpeedDet >= 0))
		{
			aSpeedRef_offseted = MOTOR_PI(aSpeedRef - aSpeedDet, gParam.mPI_P, gParam.mPI_I, &gMotorPI_buf) + gParam.mStartVoltage * 2 ; //CV2 response  /MECY Adjust 2016/12/24
		}
	}
	else
	{
		//PI無効
		if( aSpeedRef == 0)
		{
			aSpeedRef_offseted = aSpeedRef;
		}
		else if( aSpeedRef >= 1)
		{
			aSpeedRef_offseted = aSpeedRef + gParam.mStartVoltage ; //2016/06/19 Modify Nagoden 
		}
	}

	//前回速度として保存
	gPrevSpeed = aSpeedRef_offseted ;

	//Limiter (8bit PWM Output)
	aPWMRef = MOTOR_limitSpeed((uint16_t)(aSpeedRef_offseted));
	
	//PWM出力
	if( aPWMRef == 0)
	{
		analogWrite(MOTOR_PWM_A, 0);
		analogWrite(MOTOR_PWM_B, 0);
	}
	else
	{
		//PWM出力
		//進行方向でPWMのABを切り替える
		if( inDirection > 0)
		{
  		  analogWrite(MOTOR_PWM_B, 0);
	  	  analogWrite(MOTOR_PWM_A, aPWMRef);
	    }
	    else
	    {
	  	  analogWrite(MOTOR_PWM_A, 0);
	  	  analogWrite(MOTOR_PWM_B, aPWMRef);
	    }
	}

	gDirection = inDirection;
	gPWMRef = aPWMRef;
	
	/*
	Serial.print("SCMD= ");
	Serial.print(inSpeedCmd);

	Serial.print(" ,SREF= ");
	Serial.print(aSpeedRef);
	
	Serial.print(" ,BEMF= ");
	Serial.print(aSpeedDet);

	Serial.print(" ,PI_IBUF= ");
	Serial.print(gMotorPI_buf);

	Serial.print(" ,PWM=");
	Serial.println(aPWMRef);
	*/
	
}

int MOTOR_GetBEMF()
{
	uint32_t aAve = 0;
	int i;
	
	//Stop PWM
	analogWrite(MOTOR_PWM_B, 0);
	analogWrite(MOTOR_PWM_A, 0);
	
	delayMicroseconds(300);
	
	for( i = 0; i < 8; i++)
	{
		aAve = aAve + analogRead(A0);
		delayMicroseconds(10);
	}
	
	//進行方向でPWMのABを切り替える
	if( gDirection > 0){
		analogWrite(MOTOR_PWM_B, 0);
		analogWrite(MOTOR_PWM_A, gPWMRef);
	}
	else
	{
		analogWrite(MOTOR_PWM_A, 0);
		analogWrite(MOTOR_PWM_B, gPWMRef);
	}
	
	return (aAve >> 3);
	
	
}



int MOTOR_limitSpeed(int inSpeed)
{
	uint16_t aSpeedref = 0;                                   //MECY Adjust 2016/06/04
	
	if( inSpeed >= gParam.mMaxVoltage)
	{
		aSpeedref = gParam.mMaxVoltage;                            //MECY Adjust 2016/06/04
	}
	else if( inSpeed <= 0)
	{
		aSpeedref = 0;                                        //MECY Adjust 2016/06/04
	}
	else if( gParam.mBEMFcutoff == 0 )                        //MECY Adjust 2016/06/04
	{
		aSpeedref = inSpeed;
	}
	else if( gParam.mBEMFcutoff > 0 )
	{
		aSpeedref = inSpeed;
	}
	
	return aSpeedref;
}

