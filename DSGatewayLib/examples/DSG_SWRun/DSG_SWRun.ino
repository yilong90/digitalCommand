/*********************************************************************
 * Desktop Station Gateway Sketch for Arduino UNO/Leonard
 *
 * Copyright (C) 2014 Yaasan
 *
 */

#include <avr/wdt.h>
#include <SPI.h>
#include "DSGatewayLib.h"

DSGatewayLib Gateway;

word gLocAddr = ADDR_DCC + 200;

void setup()
{
	pinMode(4,INPUT) ;    //スイッチに接続ピンをデジタル入力に設定
        digitalWrite(4, HIGH);       // プルアップ抵抗を有効に

	Gateway.begin();
	
	delay(1000);
	Gateway.SetPower(true);
        delay(1000);
	
	// Go forward
	Gateway.SetLocoDirection( gLocAddr, 1);
}

void loop()
{

     if (digitalRead(4) == HIGH) {     //スイッチの状態を調べる
          //HIGHのとき、止める。スイッチは押されていない。
          Gateway.SetLocoSpeed( gLocAddr, 0);
          
     } else {
          //LOWのとき、動かす。スイッチは押されている。
          Gateway.SetLocoSpeed( gLocAddr, 103);
	  delay(1000);
     }  
	

	
}

