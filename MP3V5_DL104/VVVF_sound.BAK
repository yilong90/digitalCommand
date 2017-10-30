//160918 fujigaya2

//#include "Timertwo.h"
#include <avr/pgmspace.h>
#include <TimerOne.h>
#include "kasoku.h"
#include "VVVF_sound.h"

#include <Arduino.h>

#define  COUNT_CONSTANTSPEED  30

#include "D205_16000_WAV6L.h"//accel
#include "D205_16000_WAV6U.h"//accel
#include "D205_16000_WAV6UU.h"//accel
#include "D205_16000_WAV3.h"//idle

//VVVF
int uplimit; //VVVF配列の上限
int kasoku[15][8];//VVVF配列
//#define CARRER_FREQ 22050
#define CARRER_FREQ 16000
int current_period1 = (double)1000000 / CARRER_FREQ; //Diesel周期
int current_period2 = (double)1000000 / CARRER_FREQ; //Diesel周期

volatile unsigned int pwm_state = 0;
unsigned int pwm_add = 0;
int pwm_mask = 0xff;
int pwm_shift = 8;
int current_duty_a = 0;


int gAccRatio = 20;
int gDecRatio = 20;
int gNotchSpdCount = 0;

uint8_t SoundSwitch = 0;
uint8_t SoundState = 1;
uint8_t SoundNotch = 0;
uint8_t SoundVolume = 10;
uint8_t gSoundLoopTiming = 8 ; // Variable set of Sound loop time by MECY 2017/05/14

int SoundWave_coeff = 1;


int SoundVolume_correct = 1;

float PrevSpeed = 0;

boolean vvvf_first_flag = true;//VVVF_Initを一回呼び出すためのFlag
boolean accel_flag = false;
boolean accel_flag_prev = false;

//IMA WAV関連転記
//Sound played flag
volatile uint8_t gSound_played = 0;
uint8_t gSound_playnext = 0;
int gSound_Length = 0;
//uint8_t SoundState = 0;      // 0: idle  1:Sound再生待ち
//IMA-ADPCM add aya
int gSound_offset = 0;      // aya add  RIFF フォーマットをそのまま使っているのでdata chunkまでoffsetを履かせる
volatile uint16_t sample;   // volatile を使う意味があるかな？
uint16_t ndata;             // 11/9 block数をカウント
byte lastSample;            //
uint16_t nBlockAlign = 252;
IMADEC imad;
uint16_t nspeed;


//割り込みのひな形
void vvvf_int1();

void VVVF_Setup()
{
  //PWM出力ピン D9を出力にセット
  pinMode(VVVF_SOUND_PIN, OUTPUT);
  //Timer1使用
  Timer1.initialize();
  //初期設定
  Timer1.pwm(VVVF_SOUND_PIN, 0, 0);
  Timer1.attachInterrupt(vvvf_int1, current_period1); //interruptの設定

  //IMAADPCM初期設定
  gSound_played = 1;
  gSound_playnext = 1;
  sample = 64;      // dataまでのオフセット
  ndata = 0;        // IMA ADPCM の block を初期化
  nBlockAlign = (uint16_t)(readSoundRom(gSound_played, 33) << 8 | readSoundRom(gSound_played, 32)) - 4;//周波数によってブロックサイズが違う。
  //SoundState = 0;   // 再生終了
  gSound_Length = (uint16_t)(readSoundRom(gSound_played, 57) << 8 | readSoundRom(gSound_played, 56)) - 13 * 20; // RIFF No56,No57
  nspeed = 500;//idleの時もAccelのときと時間を一緒にしておく。3500と3000
  imad.val = (int16_t)(readSoundRom(gSound_played, 61) << 8 | readSoundRom(gSound_played, 60));   // Get 1st sample value of the block
  imad.idx = readSoundRom(gSound_played, 62);
}

void VVVF_SetCV(uint8_t iNo, uint8_t inData)
{
  switch (iNo)
  {
    case 3:

      if ( inData > 16)
      {
        gAccRatio = 255;
      }
      else
      {
        gAccRatio = inData * 16;
      }

      if ( gAccRatio <= 0)
      {
        gAccRatio = 1;
      }

      break;
    case 4:
      if ( inData > 16)
      {
        gDecRatio = 255;
      }
      else
      {
        gDecRatio = inData * 16;
      }

      if ( gDecRatio <= 0)
      {
        gDecRatio = 1;
      }
      break;
    case 47:
      SoundSwitch = inData;
      break;
    case 48:
      SoundState = inData;
      break;
    case 49:
      SoundNotch = inData;
      break;
    case 50:
      SoundVolume = inData;
      break;
    case 52:
      if ( inData > 40 )
      {
        gSoundLoopTiming = 40;
      }
      else
      {
        gSoundLoopTiming = inData;
      }
      if ( gSoundLoopTiming <= 0)
      {
        gSoundLoopTiming = 1;
      }
      break;
  }
}

void VVVF_Cont(int inPWMFreq)
{
  uint16_t aPWMRef = 0;

  //LPF routin
  float instruct_speed = (float)(inPWMFreq * 4);//change from 256 to 1023
  float speed_step = 0;

  if (inPWMFreq == 0)
  {
    accel_flag = false;
    PrevSpeed = 0;
  }
  else if ( PrevSpeed < instruct_speed )
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
  else if ( PrevSpeed > instruct_speed )
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


    if ( gNotchSpdCount > 0)
    {
      gNotchSpdCount--;
      accel_flag = true;
    }
    else
    {
      accel_flag = false;
    }

  }

  //Accel_flagがTrueの時は高回転、falseでアイドリングとするだけ
  if (accel_flag == true)
  {
    //accel
    //ディーセル回転数チックなもの
    if(nspeed < 1200)
    {
      gSound_playnext = 2;//accel
//      nspeed += 15;
      nspeed += gSoundLoopTiming ;  // Variable set of Sound loop time by MECY 2017/05/14
    }
    else if(nspeed < 1800)
    {
      gSound_playnext = 3;//accel
//      nspeed += 15;
      nspeed += gSoundLoopTiming ;  // Variable set of Sound loop time by MECY 2017/05/14
    }
    else if(nspeed < 2400)
    {
      gSound_playnext = 4;//accel
//      nspeed += 15;
      nspeed += gSoundLoopTiming ;  // Variable set of Sound loop time by MECY 2017/05/14
    }
  }
  else
  {
    //idle
    //ディーセル回転数チックなもの
    if(nspeed > 1800)
    {
      gSound_playnext = 4;//accel
//      nspeed -= 30;
      nspeed -= gSoundLoopTiming * 2 ;  // Variable set of Sound loop time by MECY 2017/05/14
    }
    else if(nspeed > 1200)
    {
      gSound_playnext = 3;//accel
//      nspeed -= 30;
      nspeed -= gSoundLoopTiming * 2 ;  // Variable set of Sound loop time by MECY 2017/05/14
    }
//    else if(nspeed > 500)
   else if((nspeed > 500) && (PrevSpeed > 4))   //無負荷走行の時はエンジン音はLow　by MECY 2017/05/10
    {
      gSound_playnext = 2;//accel
//      nspeed -= 30;
      nspeed -= gSoundLoopTiming * 2 ;  // Variable set of Sound loop time by MECY 2017/05/14
    }
//    else
    else if((nspeed < 500) || (PrevSpeed < 4))  //停止している時だけidle音を鳴らす　by MECY 2017/05/10
    {
      gSound_playnext = 1;//idle
      nspeed = 500;
    }
  }
}


void vvvf_int1()
{
  //波形生成用interrupt
  static uint8_t foo;
  static uint8_t hilow = LOW;   // add aya:最初は下位
  //if(ndata == 252) {    // 11/9 1 block 再生終わったら、val,idxを再セットする //8000Hz
  if (ndata == nBlockAlign)    // 11/9 1 block 再生終わったら、val,idxを再セットする //22050Hz
  {
    ndata = 0;
    imad.val = (int16_t)(readSoundRom(gSound_played, sample + 1) << 8 | readSoundRom(gSound_played, sample)); // Get 1st sample value of the block
    imad.idx = readSoundRom(gSound_played, sample + 2);
    sample += 4;
  }

  //3回続けて０なら。（このやり方が良いかどうかかなり不明！）
  //if ((readSoundRom(gSound_played, sample) || readSoundRom(gSound_played, sample+1) || readSoundRom(gSound_played, sample+2)) == 0)
  //if (sample == gSound_Length)
  if (sample >= gSound_Length - nspeed)
  { //    リピート
    gSound_played = gSound_playnext;
    sample = 64;
    ndata = 0;
    imad.val = (int16_t)(readSoundRom(gSound_played, 61) << 8 | readSoundRom(gSound_played, 60));   // Get 1st sample value of the block
    imad.idx = readSoundRom(gSound_played, 62);
    //gSound_Length = (uint16_t)(readSoundRom(gSound_played, 57) << 8 | readSoundRom(gSound_played, 56)) - 300; // RIFF No56,No57
  }
//  uint8_t t = ima_decode(&imad, readSoundRom(gSound_played, sample), hilow);
//  uint16_t t = ima_decode(&imad, readSoundRom(gSound_played, sample), hilow) << 1;  // Sound extension by MECY 2017/01/14

//CV47 = 10の時 DEISEL SOUND ON  Add by MECY 2017/02/12
//CV47 = 11の時 MORE BIG DEISEL SOUND ON  Add by MECY 2017/04/22
  if(SoundSwitch == 10 )
  {
	uint16_t t = ima_decode(&imad, readSoundRom(gSound_played, sample), hilow) << 1;  // Sound extension by MECY 2017/01/14
	Timer1.setPwmDuty(VVVF_SOUND_PIN,t);
  }
  else if(SoundSwitch == 11 )
  {
	uint16_t t = ima_decode(&imad, readSoundRom(gSound_played, sample), hilow) << 2;  // Sound extension by MECY 2017/04/24
	Timer1.setPwmDuty(VVVF_SOUND_PIN,t);
  }
  //  if(speakerPin == 11) {
  //    //OCR2A = readSoundRom(gSound_played, sample);
  //    OCR2A = ima_decode(&imad,readSoundRom(gSound_played, sample), hilow); // aya add:IMA ADPCM DECODE処理
  //  } else {
  //    //OCR2B = readSoundRom(gSound_played, sample);
  //    foo = ima_decode(&imad,readSoundRom(gSound_played, sample), hilow); // aya add:IMA ADPCM DECODE処理
  /*
    char buffer[20];
    sprintf( buffer,"[foo %02x]",foo);
    Serial.print(buffer);
  */
  //OCR2B = foo;
  //      if (sample == gSound_Length - 1) {          // 最後の音をlastSampleに代入
  //        lastSample = foo;
  //      }
  //}

  if (hilow == HIGH) {   // add aya : IMA ADPCM は1byteに4bit*2のデータがあるからね。
    hilow = LOW;
    sample++;     // 再生アドレスをインクリメント
    ndata++;      // IMA ADPMC 1BLOCKカウントをインクリメント
  } else {
    hilow = HIGH;
  }
}

//------------------------------------------------------------------------------
//  ROMに配置されている変数から値を取り出す処理
//------------------------------------------------------------------------------
byte readSoundRom(uint8_t inNo, uint16_t inOffset)
{
  switch (inNo) {
    case 1:
      return pgm_read_byte(&D205_16000_WAV3_data[inOffset]);
      //return pgm_read_byte(&D205_IDLE2_16000_WAV_data[inOffset]);
      break;
    case 2:
      return pgm_read_byte(&D205_16000_WAV6L_data[inOffset]);
      break;
    case 3:
      return pgm_read_byte(&D205_16000_WAV6U_data[inOffset]);
      break;
    case 4:
      return pgm_read_byte(&D205_16000_WAV6UU_data[inOffset]);
      break;
    default:
      return 128;//無音
      return 0;
      break;
  }
}

//------------------------------------------------------------------------------------
// ChaN さんのIMA ADPCM decoder を流用

/*-------------------------------------------------*/
/* IMA ADPCM Decoder                               */

//uint8_t ima_decode(    /* Returns the sample value (-32768 to 32767) */
uint16_t ima_decode(    /* Returns the sample value (-32768 to 32767) / Sound extension by MECY 2017/01/14 */
  IMADEC* ad,     /* Work area for the ADPCM stream */
  uint8_t dat,      /* ADPCM data, b3:sign (0:+, 1:-), b2-b0:magnify (0..7) */
  uint8_t hl      /* High or low data read */
)
{
  /*
    char buffer[20];
    sprintf( buffer,"%d,",sample);
    Serial.print(buffer);
  */
  static const uint16_t step[89] __attribute__((__progmem__)) = {
    //  static const uint16_t step[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66,
    73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449,
    494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272,
    2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493,
    10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
  };

  int32_t dif;
  uint16_t stp;
  uint8_t idx;

  if (hl == HIGH)   // hl のフラグで、HIGHだったら、上位4bitを下位に移動
    dat = dat >> 4;
  dat = dat & 0x0f;
  /*
    sprintf( buffer,"%d,",dat);
    Serial.print(buffer);
  */
  /* Get current step size */
  idx = ad->idx;            // scale
  stp = pgm_read_word(&step[idx]);
  //  stp = step[idx];

  dif = stp >> 3;   // dif = stp / 8 と等価
  if (dat & 4) dif += stp;
  stp >>= 1;
  if (dat & 2) dif += stp;
  stp >>= 1;
  if (dat & 1) dif += stp;
  if (dat & 8) dif = 1 - dif;
  /* Add the difference to the previous sample */
  dif += ad->val;

  if (dif >  32767) dif =  32767;
  if (dif < -32768) dif = -32768;
  ad->val = (int16_t)dif;

  /* Update step size by magnify */
  dat = (dat & 7) - 4;

  if (dat > 3) {  /* Decrease step size by 1 if magnify is 0..3 */
    if (idx) idx--;
  } else {    /* Increase step size by 2..8 if magnify is 4..7 */
    idx += (dat + 1) << 1;
    if (idx > 88) idx = 88;
  }
  ad->idx = idx;

  //sprintf( buffer,"%#4x:%d",dif,dif);  要素が２つあると、ちゃんと変換できない
  //Serial.println(buffer);
  /*
    sprintf( buffer,"%d,",ad->val);
    Serial.print(buffer);
    sprintf( buffer,"%d,",ad->val/256+128);
    Serial.print(buffer);
    sprintf( buffer,"%d,",idx);
    Serial.println(buffer);
  */
  return ad->val / 256 + 128; /* PCM化した16bitのデータを8bitに間引いて、128のオフセットを履かせています*/
}
//------------------------------------------------------------------------------------

