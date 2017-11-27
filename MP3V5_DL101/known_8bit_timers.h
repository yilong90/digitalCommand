#ifndef known_8bit_timers_header_
#define known_8bit_timers_header_



//  Uno, Duemilanove, LilyPad, etc
//
#if defined (__AVR_ATmega168__) || defined (__AVR_ATmega328P__)
  #define TIMER2_A_PIN   11
  #define TIMER2_B_PIN   3
//  #define TIMER1_ICP_PIN 8
//  #define TIMER1_CLK_PIN 5
#endif

#endif
