//#define DebugOnNano
#define Debug Serial

#include <SPI.h>
#include <Ethernet.h>
#include <ByteBuffer.h>
#include <LEDTimer.h>
#include <MemoryFree.h>
#include <PString.h>

char logBuffer[100];
PString Log( logBuffer, sizeof( logBuffer));


LEDTimer step0( 13);

void setup() {
  //  delay(5000);
  step0.blink( 1000, 500, 5);
  Debug.begin(57600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Log.print( F( "setup,"));  
  Debug.print( F( "setup,"));
  Debug.print( freeMemory());

  EthInit();
  OptoInit();
}

void loop() {
  step0.doEvents();
  optoDoEvents();
  if(ethReadyToSend()) {
    if (optoReadInaktive()) {
      Debug.println( F("loop:optoReadInaktive"));
      optoReadBegin();
    }
    else if (optoReadEnded()) {
      Debug.println( F("loop:optoReadEnded"));
      ethSend();
    }
  }
  ethDoEvents();
}







