// DCC Decoder for DS-DCC decoder
// By yaasan
//

#include <Arduino.h>
#include "motor_func.h"

/**
 * @fn
 * PI制御（モータ） 10ms周期
 * @brief モータのPI制御を行う
 * @param inValue 入力値。
 * @param inKp Pゲイン。8d=1倍。1/8～32倍
 * @param inKi Iゲイン。32d=1000ms. 31ms～8sec(8000ms)
 * @param *iopBuf 前回値バッファ
 * @return 戻り値の説明
 * @detail 詳細な説明
 */
 
int MOTOR_PI(int inValue, int inKp, int inKi, long *iopBuf)
{
	long aSignal;
	
	aSignal = ((inValue * inKp) >> 4) + ((inKi * (*iopBuf)) >> 8);    // MECY ReAdjust1 2016/05/01
		
	if( ((*iopBuf) <= 32767*2) && ((*iopBuf) >= -32768*2))            // MECY Readjust2 2016/05/01
	{
		(*iopBuf) += inValue;
	}
	
	return (int)aSignal;
}

int MOTOR_LPF(int inValue, int inT, long *iopBuf)
{
	
	(*iopBuf) = (((*iopBuf) * (256 - inT)) >> 8) + ( inT * inValue);
	
	return (*iopBuf) >> 8;
	
}



