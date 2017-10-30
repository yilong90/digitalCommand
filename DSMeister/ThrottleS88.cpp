
#include <stdint.h>
#include "ThrottleS88.h"

#include <Arduino.h>

// ===================================================================
// === Throttle over S88==============================================
// ===================================================================


ThrottleS88::ThrottleS88() 
{
	mSize = 8;
	
}

void ThrottleS88::SetData(uint8_t inSlotNo, uint16_t inData)
{
	uint16_t aParity = 0;
	uint16_t aAddr = 0;
	uint16_t aType = 0;
	
	/* パリティ計算 */
	for( int i = 0; i < 15; i++)
	{
		
		aParity = aParity ^ ((inData >> i) & 1);
		
	}
	
	/* パリティチェック */
	if( aParity != ((inData >> 15) & 1))
	{
		return;
	}
	
	//if( (inData & 0x8000) > 0)
	if( (inData & 0x4000) > 0)
	{
		/* アドレス登録またはパワーオン・オフ制御 */
		
		aAddr = inData & 0x3FFF;
		
		if( aAddr == 0)
		{
			ChangedPowerStatus(0);
			mPowerStatus[inSlotNo] = 1;
		}
		else if( aAddr == 0x3FFE)
		//else if( aAddr = 0x7FFF)
		{
			ChangedPowerStatus(1);
			mPowerStatus[inSlotNo] = 1;
		}
		else
		{
			mLocAddr[inSlotNo] = aAddr;
		}
	}
	else
	{
		aType = (inData >> 12) & 0b11;
    //Serial.print("aType:");
    //Serial.println(aType,BIN);
		
		
		switch(aType)
		{
		case 0:
			//F0-F4, DIR, SPD
			setSpeed(inSlotNo, (0b111111 & inData) << 4);
			setDirection(inSlotNo, (inData >> 6) & 1);
			setFunctions(inSlotNo, 0, (inData >> 7) & 0b11111);
			break;
		case 1:
			//F5-F16
			setFunctions(inSlotNo, 1, (inData & 0x0FFF) << 5);
			break;
		case 2:
			//F17-F28
			setFunctions(inSlotNo, 2, (inData & 0x0FFF) << 17);
			break;
		case 3:
			//Turnouts
			if( (inData & 0x07FF) > 0)
			{
				ChangedTurnout(*CurrentACCProtocol + (inData & 0x07FF), (inData >> 11) & 1, true);
			}
			break;
		}
		
	}
}

void ThrottleS88::setSpeed(uint8_t inSlotNo, uint16_t inData)
{
	
	if( mSpeed[inSlotNo] != inData)
	{
		//if( mLocAddr[inSlotNo] > 0)
		//{
			mSpeed[inSlotNo] = inData;
			ChangedTargetSpeed(*CurrentProtocol + mLocAddr[inSlotNo], mSpeed[inSlotNo], true);
		//}
	}
	else
	{
		//何もしない
	}
	
}

void ThrottleS88::setDirection(uint8_t inSlotNo, uint16_t inData)
{
	
	if( mDirection[inSlotNo] != inData)
	{
		//if( mLocAddr[inSlotNo] > 0)
		//{
			mDirection[inSlotNo] = inData;
			ChangedTargetDir(*CurrentProtocol + mLocAddr[inSlotNo], (mDirection[inSlotNo] == 0) ? 1 : 2, true);
		//}
	}
	else
	{
		//何もしない
	}
	
}

void ThrottleS88::setFunctions(uint8_t inSlotNo, uint8_t inNo, uint32_t inData)
{
	
	uint32_t aFuncDatas = mFunctions[inSlotNo];
	
	switch(inNo)
	{
	case 0:
		aFuncDatas = (aFuncDatas & 0xFFFFFFE0) | inData;
		break;
	case 1:
		aFuncDatas = (aFuncDatas & 0xFFFE001F) | inData;
		break;
	case 2:
		aFuncDatas = (aFuncDatas & 0x0001FFFF) | inData;
		break;
	}
	
	if( mFunctions[inSlotNo] != aFuncDatas)
	{
    //変更値がどこかを確認する。
    unsigned long temp_fnc = mFunctions[inSlotNo] ^ aFuncDatas;
    for(int i = 0;i<28;i++)
    {
      if (((temp_fnc >> i) & 0x0001) == 1)
      {
      	//if( mLocAddr[inSlotNo] > 0)
      	//{
        	ChangedTargetFunc(*CurrentProtocol + mLocAddr[inSlotNo], i,((aFuncDatas >> i) & 0x0001),true);
      	//}
        break;
      }
    }
		mFunctions[inSlotNo] = aFuncDatas;
		//Notify
		//ChangedTargetFunc(mLocAddr[inSlotNo], mFunctions[inSlotNo]);
	}
	else
	{
		//何もしない
	}
	
}


