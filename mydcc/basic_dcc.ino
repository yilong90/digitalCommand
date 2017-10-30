// DCC Decoder monitor
// By Nicolas http://blog.nicolas.cx
// Inspired by Geoff Bunza and his 17 Function DCC Decoder & updated library
//
// Whats updated:
// - Factory default's CV's on writing to CV8. Implemented Geoff Bunza's auto-reset on every load concept in a different way.
// - Added ability to detect if eeprom is blank on load (check for 0xFF in a specific location) and reset CV's if needed
// - Added default CV29 value to FactoryDefaultCVs array for when eeprom is blank the library doesnt base its CV29 changes on 0xFF
// - Resets arduino on CV factory default
// - Made ACK pin into define for easy enable/disable during compile
// - Handle JMRI ACC State data in  notifyDccAccState(). Account for the slight difference in JMRI data as seen in my experiments.
//
// Debug serial output available on the serial port at baud 115200, aka Tools -> Serial Monitor
//

#include <NmraDcc.h>
#include <avr/eeprom.h>  //required by notifyCVRead() function if enabled below



//Settings

// The following line when enabled (defined) prevents the decoder's settings (CV's) from being reset on every power up.
//#define 

#define DECODER_ADDRESS 24
#define DCC_ACK_PIN 3   //if defined enables the ACK pin functionality. Comment out to disable.





NmraDcc  Dcc ;
DCC_MSG  Packet ;

//Internal variables and other.
#if defined(DCC_ACK_PIN)
const int DccAckPin = DCC_ACK_PIN ;
#endif



struct CVPair{
  uint16_t  CV;
  uint8_t   Value;
};
CVPair FactoryDefaultCVs [] = {
  {CV_MULTIFUNCTION_PRIMARY_ADDRESS, DECODER_ADDRESS},
  {CV_ACCESSORY_DECODER_ADDRESS_MSB, 0},        //The LSB is set CV 1 in the libraries .h file, which is the regular address location, so by setting the MSB to 0 we tell the library to use the same address as the primary address. 0 DECODER_ADDRESS
  {CV_MULTIFUNCTION_EXTENDED_ADDRESS_MSB, 0},    //XX in the XXYY address
  {CV_MULTIFUNCTION_EXTENDED_ADDRESS_LSB, 0},    //YY in the XXYY address
  {CV_29_CONFIG, 128 },  //Make sure this is 0 or else it will be random based on what is in the eeprom which could caue headaches
};

void(* resetFunc) (void) = 0;  //declare reset function at address 0



uint8_t FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs)/sizeof(CVPair);
void notifyCVResetFactoryDefault(){
  //When anything is writen to CV8 reset to defaults. 
  
  resetCVToDefault();  
  Serial.println("Resetting...");
  delay(1000);  //typical CV programming sends the same command multiple times - specially since we dont ACK. so ignore them by delaying
  
  resetFunc();
};


void resetCVToDefault(){
  //Reset CV's to defaults. Power restart needed for changes to take effect
  Serial.println("CVs being reset to factory defaults");
  for (int j=0; j < FactoryDefaultCVIndex; j++ ){
         Dcc.setCV( FactoryDefaultCVs[j].CV, FactoryDefaultCVs[j].Value);
  }
};



extern void    notifyCVChange( uint16_t CV, uint8_t Value){
   //calld when a CV is changed
   Serial.print("CV "); 
   Serial.print(CV); 
   Serial.print(" Changed to "); 
   Serial.println(Value, DEC);
};


void setup() {
  uint8_t cv_value;
  Serial.begin(115200);
  
  
  #if defined(DCCACKPIN)
    //Setup ACK Pin
    pinMode(DccAckPin,OUTPUT);
    digitalWrite(DccAckPin, 0);
  #endif
  
 

  #if !defined(DECODER_DONT_DEFAULT_CV_ON_POWERUP)
    if ( Dcc.getCV(CV_MULTIFUNCTION_PRIMARY_ADDRESS) == 0xFF ){  //if eeprom has 0xFF then assume it needs to be programmed
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
  Dcc.init( MAN_ID_DIY, 100,   FLAGS_DCC_ACCESSORY_DECODER , 0 ); 

  
  Serial.println("Ready");
     
}

void loop(){
  // You MUST call the NmraDcc.process() method frequently from the Arduino loop() function for correct library operation
  Dcc.process();
  
}


//Turnouts - may not be properly implemented in library, problem with BoardAddress. Must turn off my-address-only check. must be in FLAGS_DCC_ACCESSORY_DECODER mode. 
extern void notifyDccAccState( uint16_t Addr, uint16_t BoardAddr, uint8_t OutputAddr, uint8_t State ){
   uint8_t OutputNum = OutputAddr >> 1;  //shift over the bits so the outputaddr is 0 to 3
   uint8_t StateProper = OutputAddr & 0b00000001;  //JMRI puts out the state as the right most bit of pDccMsg->Data[1], the state argument doesnt change in JMRI Turnout.
   
   Serial.print("AccState - ");
   Serial.print("Raw addr: ");
   Serial.print(Addr);
   Serial.print(" BoardAddr: ");
   Serial.print(BoardAddr, DEC);
   Serial.print(" OutputAddr: ");
   Serial.print(OutputAddr, DEC);
   Serial.print(" Output: ");
   Serial.print(OutputNum);
   Serial.print(" State: 0b");
   Serial.print(State, BIN);
   Serial.print(" Status: ");
   Serial.print(StateProper == 1 ? "Closed" : "Thrown");
   Serial.println();
   
   /*
  Serial.print(Addr);  //the address we sent, when in accessory mode, this is not considered "our address" but instead the BoardAddr is (which is addr / 4 discard remainder)
  Serial.print(" ");
  Serial.print(BoardAddr);  //The 511 possible addresses with 4 sub-addresses
  Serial.print(" ");
  Serial.print(OutputAddr);  //0-4 possible sub addresses
  Serial.print(" ");
  Serial.println(State);    //State. 1/0
  */
}

//Signals - must be in FLAGS_DCC_ACCESSORY_DECODER mode
extern void notifyDccSigState( uint16_t Addr, uint8_t OutputIndex, uint8_t State){
  Serial.print("SigState - ");
  Serial.print(Addr);
  Serial.print(" ");
  Serial.print(OutputIndex);
  Serial.print(" ");
  Serial.println(State);
}

//Motor control - must NOT be in FLAGS_DCC_ACCESSORY_DECODER mode
extern void notifyDccSpeed( uint16_t Addr, uint8_t Speed, uint8_t ForwardDir, uint8_t MaxSpeed ){
  Serial.print("Speed - ");
  Serial.print(Addr);
  Serial.print(" ");
  Serial.print(Speed);
  Serial.print(" ");
  Serial.print(ForwardDir);
  Serial.print(" ");
  Serial.println(MaxSpeed);
}



extern void notifyDccMsg( DCC_MSG * Msg ) {
  int i;
  if( ( Msg->Data[0] == 0 ) && ( Msg->Data[1] == 0 ) ) return;  //reset packlet
  if( ( Msg->Data[0] == 0b11111111 ) && ( Msg->Data[1] == 0 ) ) return;  //idle packet

  if(Msg->Data[0] == 100 && Msg->Data[1] == 63) return;
  
  
   Serial.print("DccMsg - ");
   Serial.print(Msg->Size);
   Serial.print(" ");
   for(i=0;i<Msg->Size;i++){
     Serial.print(Msg->Data[i], BIN);
    Serial.print(" ");
   }
   
   Serial.println();
}

/*
//If you wish to see CVRead calls uncomment this function.
extern uint8_t notifyCVRead( uint16_t CV) {
  if(CV != 29 && CV != 17 && CV != 18 && CV != 1){  //skip internally read CV's from output, the library reads CV29 and 1 very often.
     Serial.print("CV Read: ");
     Serial.println(CV);
  }
  
  return eeprom_read_byte( (uint8_t*) CV ) ;
}*/
