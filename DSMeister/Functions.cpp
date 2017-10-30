#include <stdint.h>
#include <Arduino.h>
#include "Functions.h"


/*
�O��l���{�^����������Ă��Ȃ����̃`�F�b�N�����݉�����Ă��邩�̃`�F�b�N(�G�b�W���o)
*/

uint8_t CheckButtonStatus(uint16_t inCurrents, uint16_t inLasts, uint16_t inTargetBit)
{
	if( ((inCurrents & inTargetBit) > 0) && ((inLasts & inTargetBit) == 0))
	{
		return 1;
	}
	else
	{
		return 0;
	}
	
}

word stringToWord(String s)
 {
  word result = 0;
  
  for (int i = 0; i < s.length(); i++) {
	result = 10 * result + (s.charAt(i) - '0');
  }
  
  return result;
}

