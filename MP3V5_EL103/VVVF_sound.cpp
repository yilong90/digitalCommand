
#include <Arduino.h>
#include <avr/pgmspace.h> 
//#include "TimerOne_x.h"
#include "kasoku.h"
#include "VVVF_sound.h"
#include "motor_func.h"


#include "test23_loud.h"//吊りかけ音のかけら　test21 中音域
#define WAV_DATA1 test23_data
#define  WAV_LENGTH1 test23_length
#define REF_WAVE_FREQ1 (1000000 / (WAV_LENGTH1 * 62.5))

//#include "test23_loud.h"//吊りかけ音のかけら test23 低音側
#define WAV_DATA2 test23_data
#define  WAV_LENGTH2 test23_length
#define REF_WAVE_FREQ2 (1000000 / (WAV_LENGTH2 * 62.5))

//#include "test23_loud.h"//吊りかけ音のかけら test23 低音側
#define WAV_DATA3 test23_data
#define  WAV_LENGTH3 test23_length
#define REF_WAVE_FREQ3 (1000000 / (WAV_LENGTH3 * 62.5))

/*
#include "saw_art_1024.h"//吊りかけ音のかけら
#define WAV_DATA saw_art_1024_data
#define  WAV_LENGTH saw_art_1024_length
#define REF_WAVE_FREQ (1000000 / (WAV_LENGTH * 62.5))
*/

// REF_WAVE_FREQ = 1000000 / (WAV_LENGTH * 62.5)
//#define REF_WAVE_FREQ 12.618
//#define REF_WAVE_FREQ 15.625
//#define SHIFT_PWM 5
#define SHIFT_PWM 5

//#define  COUNT_CONSTANTSPEED  165 /* 33d=1sec  */
#define  COUNT_CONSTANTSPEED   33 /* 33d=1sec  */
#define  THRESHOLD_RATIO       40 /* ノッチオフの加減速しきい値 */

//VVVF
int uplimit; //VVVF配列の上限
int kasoku[15][8];//VVVF配列
//#define CARRER_FREQ 22000
#define CARRER_FREQ 16000


int current_period1 =(double)1000000 / CARRER_FREQ;//VVVF周期

volatile unsigned int pwm_state1 = 0;
volatile unsigned int pwm_state2 = 10;
volatile unsigned int pwm_state3 = 20;
unsigned int pwm_state1_ret = WAV_LENGTH1 << SHIFT_PWM;
unsigned int pwm_state2_ret = WAV_LENGTH2 << SHIFT_PWM;
unsigned int pwm_state3_ret = WAV_LENGTH3 << SHIFT_PWM;


unsigned int pwm_add1 = 0;
unsigned int pwm_add2 = 10;
unsigned int pwm_add3 = 20;
unsigned int old_pwm_add1 = 0;
unsigned int old_pwm_add2 = 10;

unsigned int pwm_shift = 8;
//unsigned int pwm_shift = 0;
int current_duty_a = 0;


int gAccRatio = 20;
int gDecRatio = 20;
int gNotchSpdCount = 0;

uint8_t SoundSwitch = 0;
uint8_t SoundState = 1;
uint8_t SoundNotch = 0;
uint8_t SoundVolume = 0;
long gVVVFLPF_buf = 0;

float PrevSpeed = 0;

boolean accel_flag = false;

//割り込みのひな形
void vvvf_int1();

void VVVF_Setup()
{
  //debug
  //pinMode(7, OUTPUT);
  
  //PWM出力ピン D9を出力にセット
  pinMode(VVVF_SOUND_PIN, OUTPUT);
  //Timer1使用
  //Timer1.initialize();
  //初期設定
  //Timer1.pwm(VVVF_SOUND_PIN,0,0);
  //Timer1.attachInterrupt(vvvf_int1,current_period1 / 2);//interruptの設定    

  //31KHzに設定（初期設定でPhaseCorrectPWMモードになっていると思う）
  //D9,D10  キャリア周期:31kHz(分周無し）
  TCCR1B &= B11111000;
  TCCR1B |= B00000001;

  //出力の設定
  if(VVVF_SOUND_PIN == 9)
  {
    TCCR1A |= _BV(COM1A1);
  }
  else //10
  {
    TCCR1A |= _BV(COM1B1);
  }
  
  //interrupt設定
  TIMSK1 = _BV(TOIE1);
}

void VVVF_SetCV(uint8_t iNo, uint8_t inData)
{
  switch(iNo)
  {
	case 3:
		
		if( inData > 16)
		{
			gAccRatio = 255;
		}
		else
		{
			gAccRatio = inData * 16;
		}
	
		if( gAccRatio <= 0)
	  	{
	  		gAccRatio = 1;
	  	}
		
		break;
	case 4:
		if( inData > 16)
		{
			gDecRatio = 255;
		}
		else
		{
			gDecRatio = inData * 16;
		}
		
		if( gDecRatio <= 0)
	  	{
	  		gDecRatio = 1;
	  	}
  		break;
  case 47:
    if(inData > 9)
    {
      inData = 9;
    }
    SoundSwitch = inData;
    break;
  case 48:
    //１：レールジョイント音、２：ブレーキ音、４：吊りかけリバーブ効果
    SoundState = inData;
    break;
  case 49:
    if(inData > 2)
    {
      inData = 2;
    }
    SoundNotch = inData;
    break;
  case 50:
    if( inData > 8)
    {
      inData = 8;//0:min(no sound) ~ 8:max
    }
    SoundVolume = inData;
    break;
  }
}

void VVVF_Cont(int inPWMFreq, uint8_t inF2Flag)
{
	uint16_t aPWMRef = 0;

	//LPF
	aPWMRef = MOTOR_LPF(inPWMFreq, 32, &gVVVFLPF_buf);
	
	//int to float
	float instruct_speed = (float)(aPWMRef * 4);//change from 256 to 1023
	float speed_step = 0;
	
  if(inPWMFreq == 0)
  {
    accel_flag = false;
    PrevSpeed = 0;
  }
  else if( (PrevSpeed - instruct_speed) < -1 )
  {
    //Acceleration
    accel_flag = true;
    speed_step = (abs(instruct_speed - PrevSpeed) / gAccRatio) + 1;
  	
    PrevSpeed += speed_step ;
    if (PrevSpeed > instruct_speed)
    {
      PrevSpeed = instruct_speed;
    }
  	
  	//ノッチオフの音を止めるタイマ
  	gNotchSpdCount = COUNT_CONSTANTSPEED;
  }
  else if( (PrevSpeed - instruct_speed) > 1 )
  {
    //Deceleration
    accel_flag = false;
    speed_step = (abs(instruct_speed - PrevSpeed) / gDecRatio) + 1;
    PrevSpeed -= speed_step;
    if (PrevSpeed < instruct_speed)
    {
      PrevSpeed = instruct_speed;
    }
  	
  	gNotchSpdCount = 0;
  }
  else
  {
  	//Constant speed
  	
  	
  	if( gNotchSpdCount > 0)
  	{
  		gNotchSpdCount--;
	    accel_flag = true;
  	}
  	else
  	{
	    accel_flag = false;
  	}
  	
  }
	
	/* 
	ノッチオフ無効(0)時はアクセルフラグを常時有効（減速でオフにさせない) 
	ノッチオフ自動(1)時はアクセルフラグを自動制御(加速終了後指定時間でオフ)
	ノッチオフ手動(2)時はアクセルフラグを手動操作
	*/
	
	if (SoundNotch == 2)
	{
		if (inF2Flag == 0)
		{
			accel_flag = true;
		}
		else
		{
			accel_flag = false;
		}
		
	}
	else
	{
		//何もしない
	}

  VVVF_Freq((int)PrevSpeed);
	/* 加減速管理 */
	if( (PrevSpeed != 0) && ((accel_flag == true) || ( SoundNotch == 0)))
	{
    if(SoundVolume > (8 - pwm_shift))
    {
      pwm_shift --;
    }
	}
	else
	{
    //pwm_shift == 10で音量0
    if(pwm_shift < 8)
    {
      pwm_shift ++;
    }
	}
}

int VVVF_Freq( int inPWMFreq)
{
  //VVVF Freq部分
  double pwm_freq1 = 440;
  double pwm_freq2 = 440;
  
  for(int i = 0;i < uplimit;i++)
  {
    if ( (kasoku[i][0] <= inPWMFreq ) and ( inPWMFreq <= kasoku [i][1]))
    {
      //音色側
      //周波数１
      pwm_freq1 = (double)kasoku[i][2]
               + (double)(kasoku[i][3] - kasoku[i][2]) / (double)(kasoku [i][1] - kasoku [i][0])
               * (double)(inPWMFreq - kasoku [i][0]);
      pwm_freq2 = (double)kasoku[i][4]
               + (double)(kasoku[i][5] - kasoku[i][4]) / (double)(kasoku [i][1] - kasoku [i][0])
               * (double)(inPWMFreq - kasoku [i][0]);
    }  
  }
  //long t1 = (65535 * pwm_freq1) / CARRER_FREQ; 
  //pwm_add1 = (unsigned int)t1;

 
  pwm_add1 = (pwm_freq1 * pow(2,SHIFT_PWM)) / REF_WAVE_FREQ2;
  pwm_add2 = (pwm_freq1 * pow(2,SHIFT_PWM)) / REF_WAVE_FREQ2;
  pwm_add3 = (pwm_freq2 * pow(2,SHIFT_PWM)) / REF_WAVE_FREQ2;

  if((SoundState & 4) != 0)
  {
    //33msリバーブ
    pwm_add3 = old_pwm_add1 * 1;
  }
  else
  {
    //通常の和音
  }
  old_pwm_add1 = pwm_add1;
  old_pwm_add2 = pwm_add2;
  

  //コーラス？
  //pwm_add2 =pwm_add1*0.999;
  //pwm_add3 =pwm_add1*0.998;
  //pwm_add2 =pwm_add1*2;
  //pwm_add3 =pwm_add1*3;

  return 0;
}

void VVVF_Init()//160917 fujigaya2 
{
  int cv47_Value = SoundSwitch;
  int cv48_Value = SoundState;
  //VVVF配列読み込み
  int kasoku_offset =  (int)pgm_read_word_near(&progmem_ref_kasoku[cv47_Value]);
  uplimit = (int)pgm_read_word_near(&progmem_ref_kasoku[cv47_Value+1]) - kasoku_offset;
  for(int i = 0;i < uplimit;i++)
  {
    for (int j = 0;j < 8;j++)
    {
      kasoku[i][j] =  (int)pgm_read_word_near(&progmem_kasoku [i + kasoku_offset][j]); 
    }
  }
}

//void vvvf_int1()
ISR(TIMER1_OVF_vect)
{
  //PORTD ^= _BV(PD7);
  //波形生成用interrupt
  static boolean flg = true;
  flg = !flg;
  if(flg)
  {
    //PORTD ^= _BV(PD7);
    return;
  }
  if (pwm_shift != 8)
  {
    //Timer1.setPwmDuty(VVVF_SOUND_PIN,((pgm_read_byte_near(&WAV_DATA1 [pwm_state1 >> SHIFT_PWM]) * 3 + pgm_read_byte_near(&WAV_DATA3 [pwm_state3 >> SHIFT_PWM])) >> pwm_shift));
    //Timer1.setPwmDuty(VVVF_SOUND_PIN,( (pgm_read_byte_near(&WAV_DATA1 [pwm_state1 >> SHIFT_PWM]))));
    //Timer1.setPwmDuty(VVVF_SOUND_PIN,100);
    unsigned int a = ((pgm_read_byte_near(&WAV_DATA1 [pwm_state1 >> SHIFT_PWM]) * 3 + pgm_read_byte_near(&WAV_DATA3 [pwm_state3 >> SHIFT_PWM])));
//   analogWrite(VVVF_SOUND_PIN,a>>2);
    
    if (VVVF_SOUND_PIN == 9)
    {
      OCR1A=a>>(pwm_shift + 2);
    }
    else//(VVVF_SOUND_PIN == 10)
    {
      OCR1B=a>>(pwm_shift + 2);
    }

    pwm_state1 += pwm_add1;
    if((pwm_state1 >= (WAV_LENGTH1 << SHIFT_PWM)))
      pwm_state1 -= (WAV_LENGTH1 << SHIFT_PWM);
    pwm_state3 += pwm_add3;
    if((pwm_state3 >= (WAV_LENGTH3 << SHIFT_PWM)))
      pwm_state3 -= (WAV_LENGTH3 << SHIFT_PWM);
  }
  else
  {
    if (VVVF_SOUND_PIN == 9)
    {
      OCR1A=0;
    }
    else//(VVVF_SOUND_PIN == 10)
    {
      OCR1B=0;
    }
  }
  //
  //PORTD ^= _BV(PD7);
}

