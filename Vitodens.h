/*
  A Library to connect my Viodens300 to Arduino
  Created by Eddy Steier, February 08, 2013.
  est.git@online.de
 */
 
#ifndef Vitodens_h
#define Vitodens_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

extern "C" {
// callback function types
    typedef void (*onMsgFunction)(uint8_t LogLevel, const char* Msg);
    typedef void (*onValueReadCallbackFunction)(uint8_t AdrBit1, uint8_t AdrBit2, uint8_t Length, uint8_t* Data);
}

class Vitodens
{
public:
	Vitodens(HardwareSerial *pVito);
//	Vitodens(HardwareSerial *pVito,Stream *pDebug);
  void attach(onMsgFunction newFunction);
  void attach(onValueReadCallbackFunction newFunction);
	void doEvents();
	void beginReadValue(uint8_t AdrBit1, uint8_t AdrBit2, uint8_t LenBit );
  void beginWriteValue(uint8_t AdrBit1, uint8_t AdrBit2, uint8_t DataBit );
	bool awaitingCommand();
	bool dataAvailable();
  
private:
	void Msg(uint8_t LogLevel, const char* msg);
//    void Msg(uint8_t LogLevel, const __FlashStringHelper* msg);
	void Msg(uint8_t LogLevel, const uint8_t& theNumber);
	void Msg(uint8_t LogLevel, const uint8_t& theNumber, const int& Type);
	enum pStatusEnm{
	  None,
	  WaitForConnectionOffer,
	  NotInitialized,
	  Initialized,
		SendDatagram,
	  WaitForAnswer
	} ;
	pStatusEnm pStatus;
	HardwareSerial *pVitoS;
//	Stream *pDebug;
	uint8_t pByteNum;
	uint32_t pWaitForAnswerSince;
  uint8_t Data[16];

  onMsgFunction currentOnMsg;
  onValueReadCallbackFunction currentonValueReadCallback;
};
#endif

