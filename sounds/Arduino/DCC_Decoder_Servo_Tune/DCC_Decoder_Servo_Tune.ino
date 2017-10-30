//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Ruud Boer December 2014
// DCC Servo decoder hardware initialization version 3
//
// PURPOSE: Tune the angles of 4 setpopints of servos.
//          Values are shown on Serial Monitor to be (manually) transferred to DCC_Decoder_Servo.
//
// CONNECT THE SERVO TO PIN 8
//
// REMARK 1: CW and CCW = (Counter)ClockWise when looking at the BOTTOM of the servo.
//           (Because this is how they are mounted under the table at RudysModelRailway).
//
// REMARK 2: Timing for Servo, Setpoint and Print can be changed in lines 39.40,41
//
// REMARK 3: To avoid servo movement which can give high current draw at startup,
//           first start the Arduino and after led blinked 5 times switch the servo power on.
//           Before power down, move the servo to setpoint 1 = CCWidle.
//
// OPERATION:
//   A0: Select setpoint 0 Minangle switch
//   A1: Select setpoint 1 Minangle idle
//   A2: Select setpoint 2 Maxamgle idle
//   A3: Select setpoint 3 Maxangle switch
//   A4: Change angle of current Setpoint decrease angle
//   A5: Change angle of current Setpoint increase angle
//    0: Setpoint = 90 degrees
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Declarations and initialization
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Servo.h>

byte led = 13;
byte angle = 90;
byte sp=1; //servo setpoint, initial set at the 2nd setpoint which is 90 degrees 
byte setpoint[5] = {84,90,90,96,90};
Servo servo; //Servo functions included via 'include servo.h'
byte servotimer = 40; //Servo update timer. Lower value = higher servo speed. 10ms = 100 deg/s
byte setpointtimer = 250; //Setpoint update timer. 250ms = 4 deg/s
int printtimer = 2000; //Serial print timer
unsigned long timeforservo = millis() + servotimer;
unsigned long timetoupdatesetpoint = millis() + setpointtimer;
unsigned long timetoprint = millis() + printtimer;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void blink() {
digitalWrite(led,HIGH);
delay (50);
digitalWrite(led,LOW);
delay (200);
}

void printData() {
Serial.print("0 CCW switch = ");
Serial.println(setpoint[0]);
Serial.print("1 CCW idle   = ");
Serial.println(setpoint[1]);
Serial.print("2 CW switch  = ");
Serial.println(setpoint[2]);
Serial.print("3 CW idle    = ");
Serial.println(setpoint[3]);
Serial.println();
}

void printAngle() {
Serial.print("Setpoint ");
Serial.print(sp);
Serial.print(" ");
Serial.println(setpoint[sp]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Setup (run once)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() 
{

pinMode(A0,INPUT_PULLUP);
pinMode(A1,INPUT_PULLUP);
pinMode(A2,INPUT_PULLUP);
pinMode(A3,INPUT_PULLUP);
pinMode(A4,INPUT_PULLUP);
pinMode(A5,INPUT_PULLUP);
pinMode(0,INPUT_PULLUP);
pinMode(led,OUTPUT);
digitalWrite(led,LOW);

servo.attach(8); //servo is connected to pin 8

Serial.begin(9600);

for (byte n=0; n<5; n++) blink();

} // end setup

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Main loop (run continuous)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() 
{

////////////////////////////////////////////////////////////////
// Setpoint selection
if (digitalRead(A0)==LOW) sp=0; //go to min angle
if (digitalRead(A1)==LOW) sp=1; //go to min idle angle
if (digitalRead(A2)==LOW) sp=2; //go to max idle angle
if (digitalRead(A3)==LOW) sp=3; //go to max angle
if (digitalRead(0)==LOW) sp=4; //go to 90 degrees angle

////////////////////////////////////////////////////////////////
// Time to move servos 1 degree
if (millis() > timeforservo) {
	timeforservo = millis() + servotimer;
	if (angle < setpoint[sp]) angle++;
	if (angle > setpoint[sp]) angle--;
	servo.write(angle);
}

////////////////////////////////////////////////////////////////
// Time to print data
if (millis() > timetoprint) {
	timetoprint = millis() + printtimer;
	printData();
}

////////////////////////////////////////////////////////////////
// Time to update setpoit
if (millis() > timetoupdatesetpoint) {
	timetoupdatesetpoint = millis() + setpointtimer;
	if (digitalRead(A4)==LOW) {setpoint[sp]--; printAngle();} //adjust to min, CCW
	if (digitalRead(A5)==LOW) {setpoint[sp]++; printAngle();} //adjust to max, CW
}

} // end loop

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// END
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
