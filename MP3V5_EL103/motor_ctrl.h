// Motor Control for DS-DCC decode
// By yaasan
//




/*************************************************
 * 
 * Declaration of Symbols
 * 
 *************************************************/


#define MOTOR_PWM_A 11
#define MOTOR_PWM_B 3

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


typedef struct _MOTOR_PARAM
{
	byte mStartVoltage;
	byte mMaxVoltage;
	byte mAccRatio;
	byte mDecRatio;
	byte mBEMFcoefficient;
	byte mPI_P;	
	byte mPI_I;	
	byte mBEMFcutoff;
} MOTOR_PARAM;
	
	


/*************************************************
 * 
 * Declaration of Functions
 * 
 *************************************************/

extern void MOTOR_Init();
extern void MOTOR_SetCV(byte iNo, byte inData);
extern void MOTOR_Sensor();
extern void MOTOR_Ack(void);
extern void MOTOR_Main(int inSpeedCmd, int inDirection);
int MOTOR_limitSpeed(int inSpeed);
int MOTOR_GetBEMF();

