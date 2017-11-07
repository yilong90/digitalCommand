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
// IMA　ADPCM typedef

typedef struct {  /* Decoder work area of an IMA-ADPCM stream */
  int16_t val;  /* Previous sample value (-32768..32768) */
  uint8_t idx;  /* Current index of step size (0..88) */
} IMADEC;



/*************************************************
 * 
 * Declaration of Functions
 * 
 *************************************************/

 //extern void VVVF_Cont(int inPWMFreq);
 // extern void VVVF_Cont(int inPWMFreq, uint8_t inF2Flag);
 extern void VVVF_Cont(int inPWMFreq, uint8_t inF2Flag, uint8_t inF3Flag);   //Change by MECY 2017/10/07
 extern void VVVF_Setup();
 void VVVF_SetCV(uint8_t iNo, uint8_t inData);
 
 //void  VVVF_Init(int cv47_Value,int cv48_Value);
 //int VVVF_Freq( int inPWMFreq);
 //byte readSoundRom(uint8_t inNo, uint16_t inOffset);
 uint8_t readSoundRom(uint8_t inNo, uint16_t inOffset);    //Change by MECY 2017/01/07
 //uint8_t ima_decode(    /* Returns the sample value (-32768 to 32767) */
 uint16_t ima_decode(    /* Returns the sample value (-32768 to 32767) */
  IMADEC* ad,     /* Work area for the ADPCM stream */
  uint8_t dat,      /* ADPCM data, b3:sign (0:+, 1:-), b2-b0:magnify (0..7) */
  uint8_t hl      /* High or low data read */
);
