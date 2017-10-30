#include <Wire.h>
#include "I2CLCDLib.h"


I2CLCDLib::I2CLCDLib()
{
	contrast = 35;
	contrastFlag = false;

}

void I2CLCDLib::begin()
{
	Wire.begin();
	lcd_cmd(0b00111000); // function set
	lcd_cmd(0b00111001); // function set
	lcd_cmd(0b00000100); // EntryModeSet
	lcd_cmd(0b00010100); // interval osc
	lcd_cmd(0b01110000 | (contrast & 0xF)); // contrast Low
	lcd_cmd(0b01011100 | ((contrast >> 4) & 0x3)); // contast High/icon/power
	lcd_cmd(0b01101100); // follower control
	delay(200);
	lcd_cmd(0b00111000); // function set
	lcd_cmd(0b00001100); // Display On
	lcd_cmd(0b00000001); // Clear Display
	delay(2);
}


void I2CLCDLib::lcd_cmd(byte x) {
  Wire.beginTransmission(I2Cadr);
  Wire.write(0b00000000); // CO = 0,RS = 0
  Wire.write(x);
  Wire.endTransmission();
}
 
void I2CLCDLib::lcd_contdata(byte x) 
{
  Wire.write(0b11000000); // CO = 1, RS = 1
  Wire.write(x);
}
 
void I2CLCDLib::lcd_lastdata(byte x) 
{
  Wire.write(0b01000000); // CO = 0, RS = 1
  Wire.write(x);
}
 

void I2CLCDLib::lcd_printStr(const char *s) 
{
  Wire.beginTransmission(I2Cadr);
  while (*s) {
    if (*(s + 1)) {
      lcd_contdata(*s);
    } else {
      lcd_lastdata(*s);
    }
    s++;
  }
  Wire.endTransmission();
}
 

void I2CLCDLib::lcd_setCursor(byte x, byte y)
{
  lcd_cmd(0x80 | (y * 0x40 + x));
}

void I2CLCDLib::lcd_setContrast(byte c) 
{
  lcd_cmd(0x39);
  lcd_cmd(0b01110000 | (c & 0x0f)); // contrast Low
  lcd_cmd(0b01011100 | ((c >> 4) & 0x03)); // contast High/icon/power
  lcd_cmd(0x38);
}

void I2CLCDLib::lcd_clear(void)
{
    lcd_cmd(0x01);     
    delay(2); 
}
