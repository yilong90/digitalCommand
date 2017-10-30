//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DCC Accessory / Function Decoder
// Author: Ruud Boer - February 2015
// This sketch turns an Arduino into a DCC decoder with max 16x function outputs and max 12 servo outputs.
// Also, the DCC address can be sent over serial USB to the PC, where the program DCCSound can playback a wav or mp3
// An output can be mode=0: continuous, or mode=1: a oneshot, or mode=2: a 2 output alternating flasher
// The DCC signal is optically separated and fed to pin 2 (=Interrupt 0). Schematics: www.mynabay.com
// Many thanks to www.mynabay.com for publishing their DCC monitor and -decoder code

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IMPORTANT: to avoid servo jerk at startup, first power up the Arduino and wait a few seconds before
// switching the power to the servos. Before power-off, use a switch or a wire to make input A5 LOW. This will let all
// servos rotate to their 'offangle'. At startup this is where they will start, thus minimizing jerk.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IMPORTANT: GOTO lines 24, 68, 98 to configure some data!
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <DCC_Decoder.h>
#include <Servo.h> 
#define kDCC_INTERRUPT 0

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fill in the number of accessories and servos you want to control
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const byte maxaccessories = 4;
const byte maxservos = 1;
const byte servotimer = 60; //Servo angle change timer: 1 degree per ## ms. Lower value -> higher speed
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned long timetoupdatesetpoint = millis() + servotimer;

typedef struct {
  int               address;          // User defined. DCC address to respond to
  byte              mode;             // User defined. Mode: 1=Continuous, 2=Oneshot, 3=Flasher
  byte              outputPin;        // User defined. Arduino pin where accessory is connected to
  byte              outputPin2;       // User defined. 2nd pin for AlternatingFlasher (e.g. railway crossing)
  byte              invert;           // User defined. 0=output HIGH when DCC enabled, 1=output LOW when DCC enabled
  byte              playsound;        // User defined. 1 = play sound via serial communication to DCCSound.exe
  int               ontime;           // User defined. One shot or Flasher 'on' time in ms
  int               offtime;          // User defined. Flasher 'off' time in ms
  int               ondelay;          // User defined. Delay before the function reacts to switching on
  int               offdelay;         // User defined. Delay before the function reacts to switching off
  byte              dcc;              // Internal use. DCC state of accessory: 1=on, 0=off
  byte              dccdelayed;       // Internal use. Delayed DCC state of accessory: 1=on, 0=off
  byte              output;           // Internal use. Output state of accessory: 1=on, 0=off
  byte              output2;          // Internal use. Output2 state (for AlternatingFlasher) 1=on, 0=off
  byte              finished;         // Internal use. Memory that says the Oneshot is finished
  unsigned long     onmillis;         // Internal use.
  unsigned long     offmillis;        // Internal use.
  unsigned long     ondelaymillis;    // Internal use.
  unsigned long     offdelaymillis;   // Internal use.
} 
DCCAccessoryAddress;
DCCAccessoryAddress accessory[maxaccessories];

struct servoItem {
  byte angle;
  byte setpoint;
  byte offangle;
  byte onangle;
  Servo servo;
  byte functionnumber;
};
servoItem servos[maxservos];

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fill in the parameters for every accessory / function
// COPY - PASTE as many times as you have functions. The amount must be same as maxaccessories in line 26
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ConfigureDecoderFunctions() // The amount of accessories must be same as in line 26 above!
{
  accessory[0].address = 1;    // DCC address
  accessory[0].mode = 1;       // Continuous: HIGH until DCC switches the address off again
  accessory[0].outputPin = 13; // Arduino pin to which this function is connected
  accessory[0].ondelay = 5000; // servo is attached to thos function
  
  accessory[1].address = 1;
  accessory[1].mode = 3; // Flasher: HIGH for ontime ms, LOW for offtime ms, repeats till DCC off
  accessory[1].outputPin = 3;
  accessory[1].outputPin2 = 4;
  accessory[1].invert = 1; // Output inverted, HIGH if no DCC
  accessory[1].ontime = 400;
  accessory[1].offtime = 400;
  accessory[1].playsound = 1;  //plays a sound via DCCSound.exe
  accessory[1].offdelay = 5000;

  accessory[2].address = 2;
  accessory[2].mode = 2; // Oneshot: HIGH for ontime ms, then LOW and stays LOW.
  accessory[2].outputPin = 13;
  accessory[2].ontime = 1000;
  accessory[2].playsound = 1; //plays a sound via DCCSound.exe
  
  accessory[3].address = 3;
  accessory[3].mode = 3; // F3asher
  accessory[3].outputPin = 13;
  accessory[3].ontime = 299;
  accessory[3].offtime = 299;
  accessory[3].playsound = 1; //plays a sound via DCCSound.exe

  
  
}  // END ConfigureDecoderFunctions

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fill in the parameters for every servo
// COPY - PASTE as many times as you have servo's. The amount must be same as maxservos in line 27
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ConfigureDecoderServos()
{
  servos[0].functionnumber=0; // CONNECTION BETWEEN FUNCTION AND SERVO ()accessory[functionnumber])
  servos[0].servo.attach(5); //Arduino pin number where servo is connected to
  servos[0].angle=50; //initial angle of servo. Make this the same as offangle to avoid startup jitter.
  servos[0].offangle=50; //minimum angle. Do not use value too close to 0, servo may stutter at the extremes.
  servos[0].onangle=140; //maximum angle. Do not use value too close to 180, servo may stutter at the extremes.

  // A servo is coupled to an accessory[n]
  // It rotates to offangle if accessory[n].dcc = 0
  // It rotates to onangle if accessory[n].dcc = 1
  // If you have multiple servo's you need to couple them to different accessory functions. However ...
  // accessories may switch the same output pin (e.g. pin 13, which has the on board led attached)
	
} // END ConfigureDecoderServos

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DCC accessory packet handler 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  void BasicAccDecoderPacket_Handler(int address, boolean activate, byte data)
  {
    // Convert NMRA packet address format to human address
    address -= 1;
    address *= 4;
    address += 1;
    address += (data & 0x06) >> 1;

    boolean enable = (data & 0x01) ? 1 : 0;

    for (int i=0; i<maxaccessories; i++)
    {
      if (address == accessory[i].address)
      {
        if (enable) 
        {
          accessory[i].dcc = 1;
          accessory[i].ondelaymillis = millis() + accessory[i].ondelay;
        }
        else
        {
          accessory[i].dcc = 0;
          accessory[i].offdelaymillis = millis() + accessory[i].offdelay;
        }
      }
    }
  } //END BasicAccDecoderPacket_Handler

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Setup (run once)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() 
{ 
  DCC.SetBasicAccessoryDecoderPacketHandler(BasicAccDecoderPacket_Handler, true);
  ConfigureDecoderFunctions();
  ConfigureDecoderServos();
  DCC.SetupDecoder( 0x00, 0x00, kDCC_INTERRUPT );
  pinMode(2,INPUT_PULLUP); //Interrupt 0 with internal pull up resistor (can get rid of external 10k)

  pinMode(A0,INPUT_PULLUP); //For test purposes, make LOW to trigger function in 7th line of main loop
  pinMode(A5,INPUT_PULLUP); //If made LOW, all servos go to their min angle, to avoid jitter at starup.

  for(int i=0; i<maxaccessories; i++)
  {
    if (accessory[i].outputPin) pinMode(accessory[i].outputPin, OUTPUT);
    if (accessory[i].outputPin2) pinMode(accessory[i].outputPin2, OUTPUT);
    accessory[i].dcc = 0; //all functions are set to 0 at startup
  }
  Serial.begin(9600);
} // END setup

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Main loop (run continuous)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop()
{
  static int addr = 0;
  
  /*
  // FOR TESTING. Make pin A0 LOW to test a function
  byte test = 2; // Fill in the accessory[n] number to test
  if (!accessory[test].dcc && !digitalRead(A0))
  {
    accessory[test].dcc = 1;
    accessory[test].ondelaymillis = millis() + accessory[test].ondelay;
  }
  if (accessory[test].dcc && digitalRead(A0))
  {
    accessory[test].dcc = 0;
    accessory[test].offdelaymillis = millis() + accessory[test].offdelay;
  }
  // END TESTING VIA INPUT A0
  */
  
  DCC.loop(); // Loop DCC library
  
  if( ++addr >= maxaccessories ) addr = 0; // Next address to test
  
  if (accessory[addr].dcc)
  {
    if (!accessory[addr].dccdelayed && millis() > accessory[addr].ondelaymillis) accessory[addr].dccdelayed = 1;
  }
  else
  {
    if (accessory[addr].dccdelayed && millis() > accessory[addr].offdelaymillis) accessory[addr].dccdelayed = 0;
  }
    
  if (accessory[addr].dccdelayed)
  {
    if (!accessory[addr].finished && accessory[addr].playsound)
    {
      Serial.print ("on: ");
      Serial.print (accessory[addr].address);
      Serial.print (char(10));
    }
    switch (accessory[addr].mode)
    {
    case 1: // Continuous
      if (!accessory[addr].finished)
      {
        accessory[addr].output = 1;
        accessory[addr].finished = true;
      }
      break;
    case 2: // Oneshot
      if (!accessory[addr].output && !accessory[addr].finished)
      {
        accessory[addr].output = 1;
        accessory[addr].offmillis = millis() + accessory[addr].ontime;
        accessory[addr].finished = true;
      }
      if (accessory[addr].output && millis() > accessory[addr].offmillis)
      {
        accessory[addr].output = 0;
      }
      break;
    case 3: // Flasher (which always is an 'alternating' flasher with .outputPin2)
      if (!accessory[addr].output && millis() > accessory[addr].onmillis)
      {
        accessory[addr].output = 1;
        accessory[addr].output2 = 0;
        accessory[addr].offmillis = millis() + accessory[addr].ontime;
        accessory[addr].finished = true;
      }
      if (accessory[addr].output && millis() > accessory[addr].offmillis)
      {
        accessory[addr].output = 0;
        accessory[addr].output2 = 1;
        accessory[addr].onmillis = millis() + accessory[addr].offtime;
      }
      break;
    }
  }
  else // accessory[addr].dccdelayed == 0
  {
    if (accessory[addr].finished)
    {
      accessory[addr].output = 0;
      accessory[addr].output2 = 0;
      if (accessory[addr].playsound)
      {
        Serial.print ("off: ");
        Serial.print (accessory[addr].address);
        Serial.print (char(10));
      }
      accessory[addr].finished = false; // Reset finished flag, the function can be triggered again.
    }
  }

  // activate outputpin, based on value of output
  if (!accessory[addr].invert)
  {
    digitalWrite(accessory[addr].outputPin, accessory[addr].output);
    digitalWrite(accessory[addr].outputPin2, accessory[addr].output2);
  }
  else
  {
    digitalWrite(accessory[addr].outputPin, !accessory[addr].output);
    digitalWrite(accessory[addr].outputPin2, !accessory[addr].output2);
  }
  
  /////////////////////////////////////////////////////////////////////////////
  // Every 'servotimer' ms, modify setpoints and move servos 1 step (if needed)
  /////////////////////////////////////////////////////////////////////////////
  if (millis() > timetoupdatesetpoint)
	{
    timetoupdatesetpoint = millis() + servotimer;
    for (int n=0; n<maxservos; n++)
		{
      if (accessory[servos[n].functionnumber].dccdelayed) servos[n].setpoint=servos[n].onangle;
      else servos[n].setpoint=servos[n].offangle;

      if (servos[n].angle < servos[n].setpoint) servos[n].angle++;
      if (servos[n].angle > servos[n].setpoint) servos[n].angle--;
      servos[n].servo.write(servos[n].angle);
    }
  }
  
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Move all servos to min position and set all function outputs to 0, to eliminate startup servo jerk current draw
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  if (digitalRead(A5)==LOW) {for (int n=0; n<maxservos; n++) accessory[n].dcc = 0;}

} //END loop
