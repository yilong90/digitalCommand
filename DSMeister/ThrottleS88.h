/*********************************************************************
 * Throttle S88
 *
 * Copyright (C) 2016 Desktop Station
 * 
 */

//170128 fujigaya2 dsjoy

#define MAX_THROTTLE   8

// ===================================================================
// === Throttle over S88==============================================
// ===================================================================

//typedef void (*PowerFunction) (const uint8_t inPower);
//typedef void (*SpeedFunction) (const uint16_t inAddr, const uint16_t inSpeed);
//typedef void (*DirFunction) (const uint16_t inAddr, const uint8_t inDir);
//typedef void (*FuncFunction) (const uint16_t inAddr, const uint32_t inFuncData);

typedef void (*PowerFunction) (const uint8_t inPower);
typedef void (*SpeedFunction) (const uint16_t inAddr, const uint16_t inSpeed, const bool inUpdate);
typedef void (*DirFunction) (const uint16_t inAddr, const uint8_t inDir, const bool inUpdate);
typedef void (*FuncFunction) (const uint16_t inAddr, const uint8_t inFuncNo, const uint8_t inFuncPower, const bool inUpdate);
 
class ThrottleS88 {

  private:

void setSpeed(uint8_t inSlotNo, uint16_t inData);
void setDirection(uint8_t inSlotNo, uint16_t inData);
void setFunctions(uint8_t inSlotNo, uint8_t inNo, uint32_t inData);

	
  /**
   * The number of contacts available.
   */
  int mSize;

  /**
   * Loco Address Buffer
   */
	uint16_t mLocAddr[MAX_THROTTLE];
	uint16_t mSpeed[MAX_THROTTLE];
	uint32_t mFunctions[MAX_THROTTLE];
	uint8_t mDirection[MAX_THROTTLE];
	uint8_t mPowerStatus[MAX_THROTTLE];
	
	
	

  public:


	ThrottleS88();
	void SetData(uint8_t inSlotNo, uint16_t inData);

	PowerFunction ChangedPowerStatus;
	SpeedFunction ChangedTargetSpeed;
	FuncFunction ChangedTargetFunc;
	DirFunction ChangedTargetDir;
	DirFunction ChangedTurnout;
	uint16_t *CurrentProtocol;
	uint16_t *CurrentACCProtocol;

};
