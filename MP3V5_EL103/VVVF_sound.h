//160918 fujigaya2
//VVVF Sound

/*************************************************
 * 
 * Declaration of Symbols
 * 
 *************************************************/


#define VVVF_SOUND_PIN 9

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



/*************************************************
 * 
 * Declaration of Functions
 * 
 *************************************************/

 extern void VVVF_Cont(int inPWMFreq, uint8_t inF2Flag);
 extern void VVVF_Setup();
 void VVVF_SetCV(uint8_t iNo, uint8_t inData);
 
 void  VVVF_Init();
 int VVVF_Freq( int inPWMFreq);
