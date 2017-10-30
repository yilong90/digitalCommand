/*********************************************************************
 * I2C LCD Library
 *
 *
 */

#ifndef I2CLCDLib_H
#define I2CLCDLib_H


#include <Arduino.h>
#include <Wire.h>

#define vddPin 16    // ArduinoA2
#define gndPin 17    // ArduinoA3
#define sdaPin 18    // ArduinoA4
#define sclPin 19    // ArduinoA5
#define I2Cadr 0x3e  //

 

class I2CLCDLib
{
	private:
		byte contrast;  //
		boolean contrastFlag;
	
	public:
		I2CLCDLib();
		void begin();
		void lcd_cmd(byte x);
		void lcd_contdata(byte x);
		void lcd_lastdata(byte x);
		void lcd_printStr(const char *s);
		void lcd_setCursor(byte x, byte y);
		void lcd_setContrast(byte c);
		void lcd_clear(void);
};

#endif
