/*
  A Library to connect my Viodens300 to Arduino
  Created by Eddy Steier, February 08, 2013.
  est.git@online.de
 */
 
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "Vitodens.h"

Vitodens::Vitodens(HardwareSerial *Vito,Stream *Debug){
	Debug->println ( ("OptoInit"));
	pDebug     = Debug;
	pVitoS     = Vito;
	pStatus    = None;
	pVitoS->begin(4800, SERIAL_8E2);
}

void Vitodens::doEvents(){
uint32_t zp = millis();
	if (zp - pWaitForAnswerSince > 60000) {
		pDebug->println("pWaitForAnswerSince > 60000");
		pStatus = None;
//		currentOnMsg(1,"Test\nZeile2");//Das funktionert - aber Zahlen?
	}
  switch ( pStatus) {
  case None:
    pDebug->println( ( "Status=None,Write0x04,"));
    pVitoS->write( 0x04);
    pStatus = WaitForConnectionOffer;
    pWaitForAnswerSince=zp;
    break;
  case WaitForConnectionOffer:
	//																																Data[0]=0x07;
	//																																Data[1]=0x01;
	//																																currentonValueReadCallback(0x55 , 0x5A, 2,Data);
    if ( pVitoS->available()) {
      uint8_t B = pVitoS->read();
      if ( B == 0x05) {
        pDebug->print( ( "Read0x05,Write0x16_0x00_0x00,"));
        pStatus = NotInitialized;
        pVitoS->write( ( uint8_t[]){ 0x16, 0x00, 0x00}, 3);
        pWaitForAnswerSince=zp;
        break;
      }
      else {
        pDebug->print( ( "Read!0x05=")); 
        pDebug->println( B, HEX);
      }
    }
    break;
  case NotInitialized:
    if ( pVitoS->available()) {
      uint8_t B = pVitoS->read();
      if ( B == 0x06) {
        pDebug->print( ( "Read0x06,"));
        pStatus = Initialized;
        pWaitForAnswerSince=zp;
        break;
      }
      else {
        pDebug->print( ( "Read!0x06=")); 
        pDebug->println( B, HEX);
      }
    }
    break;
  case Initialized: //Einfach abwarten bis BeginReading kommt
		break;
	case BeginReading:
		pDebug->println();
		pDebug->print( ( "Status=BeginReading,Write")); 
		pDebug->print( pAdrBit1,HEX);
		pDebug->print( ( " ")); 
		pDebug->print( pAdrBit2,HEX);
		pDebug->print( ( " ")); 
		pDebug->print( pLenBit);
		//      //pDebug->print( ( "h")); //pDebug->print( String( zp));
		//      if ( BigLog) 
		//        //pDebug->print( LogCurrentDS;
		//      else
		//        //pDebug->print( F( "v";
		//      LogCurrentDS = "";
		//      BigLog = false;

		// Gerätekennung abfragen
		//    pVitoS->write( ( uint8_t[]){ 0x41, 0x05, 0x00, 0x01, 0x00, 0xF8, 0x02, 0x00}, 8);
		//    Senden    :    41 05 00 01 00 F8 02 00
		//    Empfangen : 06 41 07 01 01 00 F8 02 20 B8 DB
		//    Auswertung: 0x20B8 = V333MW1
		pVitoS->write( ( uint8_t[]){ 0x41, 0x05, 0x00, 0x01, pAdrBit1, pAdrBit2, pLenBit, ( 6 + pAdrBit1 + pAdrBit2 + pLenBit) % 256}, 8);
		pStatus = WaitForAnswer;
		pByteNum = 0;
    
    pWaitForAnswerSince=zp;
    break;
  case WaitForAnswer:
    //Ma gucken was dahinter kommt. Ich erwarte: 
    //    41 Telegramm-Start-Byte
    //    05 Länge der Nutzdaten
    //    00 00 = Anfrage, 01 = Antwort, 03 = Fehler
    //    01 01 = ReadData, 02 = WriteData, 07 = Function Call
    //    XX XX 2 uint8_t Adresse der Daten oder Prozedur
    //    XX Anzahl der Bytes, die in der Antwort erwartet werden
    //    XX Prüfsumme = Summe der Werte ab dem 2 Byte (ohne 41)      
    if ( pVitoS->available() > 0) {
      static uint8_t dataLength;
      static boolean someByteChanged = false;
      uint8_t B = pVitoS->read();
      switch ( pByteNum) {
      case 0:
        if ( B == 0x06) {
          pDebug->print( ( ">0x06,"));
        }
        else {
          pDebug->print( ( ">!0x06==")); 
          pDebug->print( B, HEX); //          LogCurrentDS+="P"; LogCurrentDS += String( B, HEX); BigLog = true;
          pDebug->print( ( ","));
          pStatus = None;
        }
        break; 
      case 1: 
        if ( B == 0x41) {
          pDebug->print( ( ">0x41,"));
        }
        else {
          pDebug->print( ( ">!0x41==")); 
          pDebug->print( B, HEX); //          LogCurrentDS+="Q"; LogCurrentDS += String( B, HEX); BigLog = true;
          pDebug->print( ( ","));
          pStatus = None;
        }
        break;
      case 2:
        dataLength = B;
        pDebug->print( ( "dataLength=")); 
        pDebug->print( B, HEX);
        pDebug->print( ( ","));
        break;
      case 3:
        static uint8_t StateByte1;
        StateByte1 = B;
        pDebug->print( ( "StateByte1=")); 
        pDebug->print( B, HEX);
        pDebug->print( ( ","));
        break;
      case 4:
        pDebug->print( ( "StateByte2="));         
        pDebug->print( B, HEX);        
        pDebug->print( ( ","));
        static uint8_t StateByte2;
        StateByte2 = B;
        break;
      case 5:
        pDebug->print( ( "addressByte1="));         
        pDebug->print( B, HEX);
        static uint8_t addressByte1;
        addressByte1 = B;
        if ( addressByte1 != pAdrBit1){     //Das ist nicht die Adresse nach der ich gefragt habe
          pDebug->print( ( "!=erwartet(")); 
          pDebug->print( pAdrBit1, HEX);
          pDebug->print( ( ")")); 
          pStatus = None;
        }
        pDebug->print( ( ","));
        break;
      case 6:
        pDebug->print( ( "addressByte2=")); 
        pDebug->print( B, HEX);
        static uint8_t addressByte2;
        addressByte2 = B;
        if ( addressByte2 != pAdrBit2){     //Das ist nicht die Adresse nach der ich gefragt habe
          pDebug->print( ( "!=erwartet(")); 
          pDebug->print( pAdrBit2, HEX);
          pDebug->print( ( ")")); 
          pStatus = None;
        }
        pDebug->print( ( ","));
        break;
      case 7:
        pDebug->print( ( "Bytes=")); 
        pDebug->print( B, HEX);
        if ( B != pLenBit){     //Bitte klären warum die erwartete Anzahl Bytes nicht stimmt
          pDebug->print( ( "!=erwartet(")); 
          pDebug->print( pLenBit, HEX);
          pDebug->println( ( ")")); 
          pStatus = None;
        }
        pDebug->print( ( ","));
        someByteChanged = false;
        break;
      default:
        if ( pByteNum < dataLength + 3) {
          pDebug->print( ( ",Byte")); 
          pDebug->print( pByteNum); 
          pDebug->print( ( "="));
          pDebug->print( B, HEX);
          uint8_t lVByte = pByteNum - 8;
          Data[lVByte] = B;
        }
        else {
          pDebug->print( ( ",CheckSum=")); 
          pDebug->print( B, HEX);
          //        if ( Prüfsumme falsch)
          //        BIGLog = true;
					currentonValueReadCallback(pAdrBit1,pAdrBit2,dataLength,Data);
          //pDebug->println();
          //optoPrintLastValuesToDebug();
          pStatus = Initialized;
        }
      }
      pByteNum ++;
    }
  }  
}

void Vitodens::beginReadValue(uint8_t AdrBit1, uint8_t AdrBit2, uint8_t LenBit ) {
	pAdrBit1   = AdrBit1;
	pAdrBit2   = AdrBit2;
	pLenBit    = LenBit;
	pStatus    = BeginReading;
}

bool Vitodens::awaitingCommand(){
	return pStatus == Initialized;
}



void Vitodens::attach(onMsgFunction newFunction)
{
  currentOnMsg = newFunction;
}
void Vitodens::attach(onValueReadCallbackFunction newFunction)
{
  currentonValueReadCallback = newFunction;
}
