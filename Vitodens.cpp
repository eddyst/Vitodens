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


Vitodens::Vitodens(HardwareSerial *Vito){
	pVitoS     = Vito;
	pStatus    = None;
	pVitoS->begin(4800, SERIAL_8E2);
}

/*
Vitodens::Vitodens(HardwareSerial *Vito,Stream *Debug){
	Debug->println ( ("OptoInit"));
	pDebug     = Debug;
	pVitoS     = Vito;
	pStatus    = None;
	pVitoS->begin(4800, SERIAL_8E2);
}
*/

void Vitodens::doEvents(){
	uint32_t zp = millis();
	static 	uint8_t pAdrBit1, pAdrBit2, pLenBit;
	if (zp - pWaitForAnswerSince > 60000) {
		Msg(1, "pWaitForAnswerSince > 60000");
		pStatus = None;
		//		Msg( 1,"Test\nZeile2");//Das funktionert - aber Zahlen?
	}
	switch (pStatus) {
	case None:
		Msg(2, "Status=None,Write0x04,");
		pVitoS->write(0x04);
		pStatus = WaitForConnectionOffer;
		pWaitForAnswerSince = zp;
		//    break;
	case WaitForConnectionOffer:
		//																																Data[0]=0x07;
		//																																Data[1]=0x01;
		//																																currentonValueReadCallback(0x55 , 0x5A, 2,Data);
		if (pVitoS->available()) {
			uint8_t B = pVitoS->read();
			if (B == 0x05) {
				Msg(2, ("Read0x05,Write0x16_0x00_0x00,"));
				pStatus = NotInitialized;
				const uint8_t buff[] = { 0x16, 0x00, 0x00 };
				pVitoS->write(buff, sizeof(buff));
				pWaitForAnswerSince = zp;
				break;
			}
			else {
				Msg(1, ("Read!0x05="));
				Msg(1, B, HEX);
				pStatus = None;
			}
		}
		break;
	case NotInitialized:
		if (pVitoS->available()) {
			uint8_t B = pVitoS->read();
			if (B == 0x06) {
				Msg(2, ("Read0x06,"));
				pStatus = Initialized;
				pWaitForAnswerSince = zp;
				break;
			}
			else {
				Msg(1, ("Read!0x06="));
				Msg(1, B, HEX);
				pStatus = None;
			}
		}
		break;
	case Initialized: //Einfach abwarten bis SendDatagram kommt
		break;
	case SendDatagram:
		Msg(2, ("\nStatus=SendDatagram,Write "));
		Msg(2, Data[0], HEX);
		Msg(2, (" "));
		Data[Data[1] + 2] = 0;
		for (uint8_t i = 1; i < Data[1] + 2; i++) {
			Msg(2, Data[i], HEX);
			Msg(2, (" "));
			Data[Data[1] + 2] += Data[i];
		}
		Msg(2, ("- "));
		Msg(2, Data[Data[1] + 2], HEX);
		if (Data[Data[1] + 2] != Data[Data[1] + 2] % 256)
			Msg(2, "\n \n --------- \n Vito Prüfsumme falsch \n ----------- \n  \n");
		Msg(2, "\n");
		pAdrBit1 = Data[4];
		pAdrBit2 = Data[5];
		pLenBit = Data[6];
		pByteNum = 0;
		pVitoS->write(Data, Data[1] + 3);
		pStatus = WaitForAnswer;
		pWaitForAnswerSince = zp;
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
		if (pVitoS->available() > 0) {
			static uint8_t dataLength;
			static boolean someByteChanged = false;
			uint8_t B = pVitoS->read();
			switch (pByteNum) {
			case 0:
				if (B == 0x06) {
					Msg(2, (">0x06,"));
				}
				else {
					Msg(1, (">!0x06=="));
					Msg(1, B, HEX); //          LogCurrentDS+="P"; LogCurrentDS += String( B, HEX); BigLog = true;
					Msg(1, (","));
					pStatus = None;
				}
				break;
			case 1:
				if (B == 0x41) {
					Msg(2, (">0x41,"));
				}
				else {
					Msg(1, (">!0x41=="));
					Msg(1, B, HEX); //          LogCurrentDS+="Q"; LogCurrentDS += String( B, HEX); BigLog = true;
					Msg(1, (","));
					pStatus = None;
				}
				break;
			case 2:
				dataLength = B;
				Msg(2, ("dataLength="));
				Msg(2, B, HEX);
				Msg(2, (","));
				break;
			case 3:
				static uint8_t StateByte1;
				StateByte1 = B;
				Msg(2, ("StateByte1="));
				Msg(2, B, HEX);
				Msg(2, (","));
				break;
			case 4:
				Msg(2, ("StateByte2="));
				Msg(2, B, HEX);
				Msg(2, (","));
				static uint8_t StateByte2;
				StateByte2 = B;
				break;
			case 5:
				Msg(2, ("addressByte1="));
				Msg(2, B, HEX);
				static uint8_t addressByte1;
				addressByte1 = B;
				if (addressByte1 != pAdrBit1){     //Das ist nicht die Adresse nach der ich gefragt habe
					Msg(1, ("!=erwartet("));
					Msg(1, pAdrBit1, HEX);
					Msg(1, (")"));
					pStatus = None;
				}
				Msg(2, (","));
				break;
			case 6:
				Msg(2, ("addressByte2="));
				Msg(2, B, HEX);
				static uint8_t addressByte2;
				addressByte2 = B;
				if (addressByte2 != pAdrBit2){     //Das ist nicht die Adresse nach der ich gefragt habe
					Msg(1, ("!=erwartet("));
					Msg(1, pAdrBit2, HEX);
					Msg(1, (")"));
					pStatus = None;
				}
				Msg(2, (","));
				break;
			case 7:
				Msg(2, ("Bytes="));
				Msg(2, B, HEX);
				if (B != pLenBit){     //Bitte klären warum die erwartete Anzahl Bytes nicht stimmt
					Msg(1, ("!=erwartet("));
					Msg(1, pLenBit, HEX);
					Msg(1, (")"));
					pStatus = None;
				}
				Msg(2, (","));
				someByteChanged = false;
				break;
			default:
				if (pByteNum < dataLength + 3) {
					Msg(2, ("Byte"));
					Msg(2, pByteNum);
					Msg(2, ("="));
					Msg(2, B, HEX);
					Msg(2, (","));
					uint8_t lVByte = pByteNum - 8;
					Data[lVByte] = B;
				}
				else {
					Msg(2, ("CheckSum="));
					Msg(2, B, HEX);
					//        if ( Prüfsumme falsch)
					//        BIGLog = true;
					if (dataLength > 5)
						currentonValueReadCallback(pAdrBit1, pAdrBit2, dataLength, Data);
					pStatus = Initialized;
				}
			}
			pByteNum++;
		}
	}
}

void Vitodens::Msg(uint8_t LogLevel, const char* msg){
	currentOnMsg(LogLevel, msg);
}
/*
void Vitodens::Msg(uint8_t LogLevel, const __FlashStringHelper* msg) {
	char buffer[20]; //Size array as needed.
	int cursor = 0;
	prog_char *ptr = (prog_char *)msg;
	while ((buffer[cursor] = pgm_read_byte_near(ptr + cursor)) != '\0') ++cursor;
	currentOnMsg(LogLevel, buffer);
}
*/
void Vitodens::Msg(uint8_t LogLevel, const uint8_t& theNumber){
	Msg( LogLevel, theNumber, DEC);
}
void Vitodens::Msg(uint8_t LogLevel, const uint8_t& theNumber, const int& Type){
	char msg[4];
  int i = 0;
	switch (Type) {
  case HEX:
		msg[0] = (theNumber >> 4) + 0x30;
		if (msg[0] > 0x39) msg[0] +=7;
		msg[1] = (theNumber & 0x0f) + 0x30;
		if (msg[1] > 0x39) msg[1] +=7;
		msg[2] = '\0';	
		break;
	case DEC:
		if (theNumber > 99) 
			msg[i++] =  theNumber                             / 100 + 0x30;
		if (theNumber > 9)
		  msg[i++] = (theNumber - (theNumber / 100) * 100 ) /  10 + 0x30;
		msg[i++] = (theNumber - (theNumber /  10) *  10 )       + 0x30;
		msg[i] ='\0';	
		break;
	default:
		msg[0]='E';
		msg[1]='R';
		msg[2]='R';
		msg[3]='\0';
	}
	currentOnMsg (LogLevel, msg);
}

void Vitodens::beginReadValue(uint8_t AdrBit1, uint8_t AdrBit2, uint8_t LenBit ) {
		// Gerätekennung abfragen
		//    pVitoS->write( ( uint8_t[]){ 0x41, 0x05, 0x00, 0x01, 0x00, 0xF8, 0x02, 0x00}, 8);
		//    Senden    :    41 05 00 01 00 F8 02 00
		//    Empfangen : 06 41 07 01 01 00 F8 02 20 B8 DB
		//    Auswertung: 0x20B8 = V333MW1
	 Data[0] = 0x41;	
	 Data[1] = 0x05;
	 Data[2] = 0x00;
	 Data[3] = 0x01;
	 Data[4] = AdrBit1;
	 Data[5] = AdrBit2;
	 Data[6] = LenBit;
	pStatus    = SendDatagram;
}
void Vitodens::beginWriteValue(uint8_t AdrBit1, uint8_t AdrBit2, uint8_t DataBit ) {
	//Beispiel Vitodens 333 Betriebsart schreiben:
  //  Senden 41 06 00 02 23 23 01 XY xx
  //  Empfangen 06 41 06 01 02 23 23 01 XY xx XY:
  //  0 = Abschalten
  //  1 = nur WW
  //  2 = Heizen mit WW
  //  3 = immer Reduziert
  //  4 = immer Normal
	 Data[0] = 0x41;	
	 Data[1] = 0x06;
	 Data[2] = 0x00;
	 Data[3] = 0x02;
	 Data[4] = AdrBit1;
	 Data[5] = AdrBit2;
	 Data[6] = 0x01;
	 Data[7] = DataBit;
	pStatus    = SendDatagram;
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
