#define VitoS Serial1

enum pStatusEnm{
  Inaktive,
  Start,
  None,
  WaitForConnectionOffer,
  NotInitialized,
  Initialized,
  WaitForAnswer,
  EndReading
} 
pStatus=Inaktive;

#define AddressCount 20
#define AddressCountByteSum 58
//If like to move into Progmem use PROGMEM prog_uint8_t and implement something to read it back
uint8_t Address_01[] = { 0x25, 0x00, 0x16}; // aktuelle Betriebsart A1M1
uint8_t Address_02[] = { 0x55, 0x5A, 0x02}; // Kesselsolltemperatur (/ 10)
uint8_t Address_03[] = { 0xA3, 0x93, 0x02}; // Kesseltemperatur (/ 100)
uint8_t Address_04[] = { 0x08, 0x10, 0x02}; // Kesseltemperatur Tiefpass (/ 10)
uint8_t Address_05[] = { 0x25, 0x44, 0x02}; // Vorlaufsolltemperatur A1M1 (/ 10)
uint8_t Address_06[] = { 0x08, 0x08, 0x02}; // Rücklauftemperatur (17A) RLTS (/ 10)
uint8_t Address_07[] = { 0x08, 0x12, 0x02}; // Speichertemperatur Tiefpass STS1 (/ 10)
uint8_t Address_08[] = { 0x55, 0x25, 0x02}; // Aussentemperatur Tiefpass (/ 10)
uint8_t Address_09[] = { 0x55, 0x27, 0x02}; // Aussentemperatur gedämpft (/ 10)
uint8_t Address_10[] = { 0x0A, 0x10, 0x01}; // Umschaltventil (0..3)
uint8_t Address_11[] = { 0x08, 0x8A, 0x04}; // Brennerstarts
uint8_t Address_12[] = { 0x08, 0x86, 0x04}; // Betriebsstunden Stufe 1 (Sek)
uint8_t Address_13[] = { 0x23, 0x23, 0x01}; // Betriebsart A1M1
uint8_t Address_14[] = { 0x0A, 0x82, 0x01}; // Sammelstörung (0..1)
uint8_t Address_15[] = { 0x23, 0x02, 0x01}; // Sparbetrieb A1M1 (0..1)
uint8_t Address_16[] = { 0x23, 0x03, 0x01}; // Partybetrieb A1M1 (0..1)
uint8_t Address_17[] = { 0x65, 0x13, 0x01}; // Speicherladepumpe (0..1)
uint8_t Address_18[] = { 0x76, 0x60, 0x02}; // Interne Pumpe (0..1 / 0..100)
uint8_t Address_19[] = { 0x76, 0x63, 0x02}; // Heizkreispumpe A1M1 (0..1 / 0..100)
uint8_t Address_20[] = { 0xA3, 0x8F, 0x02}; // Anlagen Ist-Leistung (0..200 / 0..1)

const uint8_t *addressTable[AddressCount] = { 
  Address_01, Address_02, Address_03, Address_04, Address_05, Address_06, Address_07, Address_08, Address_09, Address_10, 
  Address_11, Address_12, Address_13, Address_14, Address_15, Address_16, Address_17, Address_18, Address_19, Address_20};
uint8_t lastValue[AddressCountByteSum];  //61 = Summe des 3. Feldes aus Adresse_??                                 
boolean lastValueChanged[AddressCount];  
boolean anyValueChanged = false;
int8_t run = -1;
LEDTimer led04Gesendet( 1);
LEDTimer ledWaitForAnswer( 6);
ByteBuffer inputBuffer; //Opto: to hold incoming data

void optoPrintLastValuesToDebug () {
  for ( uint8_t i = 0; i < AddressCountByteSum; i++) {
    Debug.print(toHex(lastValue[i]));
  }
  Debug.print( F(" - "));
  for ( uint8_t i = 0; i < AddressCount; i++) {
    Debug.print(lastValueChanged[i]);
  }
  Debug.print( F(" - "));
  for ( uint8_t i = 0; i < AddressCount; i++) {
    if (lastValueChanged[i])
      Debug.print( F("+"));
    else
      Debug.print( F("-"));
  }
  Debug.println();
} 

void optoReadBegin () {
  Debug.write ("optoReadBegin");
  pStatus = Start;
}

boolean optoReadInaktive () {
  return ( pStatus == Inaktive);
}

boolean optoReadEnded () {
  return ( pStatus == EndReading);
}

void optoAppendJsonSnipet ( PString& Data) {
  optoPrintLastValuesToDebug();
  for ( uint8_t i = 0; i < AddressCount; i++) {
    Debug.print ( F("lastValueChanged[")); 
    Debug.print (i);
    Debug.print ( F("]="));
    Debug.print (lastValueChanged[i]);
    if ( lastValueChanged[i]) {
      Debug.println ( F("-true"));
      Data.println( F( ","));                      //Debug.print( F( ","));
      Data.print( F( "\""));
      Data.print( toHex( addressTable[i][0]));     //Debug.print( toHex( addressTable[i][0]));
      Data.print( toHex( addressTable[i][1]));     //Debug.print( toHex( addressTable[i][1]));
      Data.print( F( "\":\""));                    //Debug.print( F( "\":\""));
      uint8_t lVS = lastValueStart( i);
      for ( uint8_t b = lVS; b < lVS + addressTable[i][2]; b++) {
        Data.print( toHex( lastValue[b]));         //Debug.print( toHex( lastValue[b]));
      }
      Data.print( F( "\""));                       //Debug.print( F( "\""));
      lastValueChanged[i] = false;
    } 
    else {
      Debug.println ( F("-false"));
    }
  }
  pStatus = Inaktive;
}    
void OptoInit() {
  Debug.println ( F("OptoInit"));
  inputBuffer.init(32);
  VitoS.begin(4800, SERIAL_8E2);
  ledWaitForAnswer.onBlinkEnd( OnLedWaitForAnswerEnd);
}

void optoDoEvents(){
  ledWaitForAnswer.doEvents();
  led04Gesendet.doEvents();
  // -- Do Work --
  static uint8_t ByteNum = 0;
  //String LogCurrentDS;
  //boolean BigLog = false;
  switch ( pStatus) {
  case Inaktive:
    break;
  case Start:
    Debug.println();
    Debug.print(F("optoStart bei millis="));
    Debug.println(millis());
    optoPrintLastValuesToDebug();
    pStatus = None;
    run = -1;
    //kein Break - Status None soll gleich mit bedient werden - ist aber extra Status als Einstigspunkt bei Fehlern
  case None:
    Debug.print( F( "Status=None,Write0x04,"));
    inputBuffer.clear();
    pStatus = WaitForConnectionOffer;
    led04Gesendet.blinkBegin( 1000);
    optoWrite( 0x04);
    ledWaitForAnswer.blinkBegin( 60000);
    break;
  case WaitForConnectionOffer:
    if ( inputBuffer.getSize()) {
      uint8_t B = inputBuffer.get();
      if ( B == 0x05) {
        Debug.print( F( "Read0x05,Write0x16_0x00_0x00,"));
        pStatus = NotInitialized;
        optoWrite( ( uint8_t[]){ 
          0x16, 0x00, 0x00                                                }
        , 3);
        ledWaitForAnswer.blinkBegin( 60000);
        break;
      }
      else {
        Debug.print( F( "Read!0x05=")); 
        Debug.println( B, HEX);
      }
    }
    break;
  case NotInitialized:
    if ( inputBuffer.getSize()) {
      uint8_t B = inputBuffer.get();
      if ( B == 0x06) {
        Debug.print( F( "Read0x06,"));
        pStatus = Initialized;
        //        led06Empfangen.blinkBegin( 1000);
        ledWaitForAnswer.blinkBegin( 60000);
        break;
      }
      else {
        Debug.print( F( "Read!0x06=")); 
        Debug.println( B, HEX);
      }
    }
    break;
  case Initialized:
    if ( Log.length() < 60) {
      if ( ++run >= AddressCount) {
        Debug.print(F("EndReading bei millis="));
        Debug.println(millis());
        pStatus = EndReading;
        ledWaitForAnswer.blinkDisable();
      }
      else {
        //Debug.println();
        Debug.print( F( "Run=")); 
        Debug.print( String( run));
        Debug.print( F( ",")); 
        //      Debug.print( F( "h")); Debug.print( String( millis()));
        //      if ( BigLog) 
        //        Debug.print( LogCurrentDS;
        //      else
        //        Debug.print( F( "v";
        //      LogCurrentDS = "";
        //      BigLog = false;
  
        // Gerätekennung abfragen
        //    optoWrite( ( uint8_t[]){ 0x41, 0x05, 0x00, 0x01, 0x00, 0xF8, 0x02, 0x00}, 8);
        //    Senden    :    41 05 00 01 00 F8 02 00
        //    Empfangen : 06 41 07 01 01 00 F8 02 20 B8 DB
        //    Auswertung: 0x20B8 = V333MW1
        optoWrite( ( uint8_t[]){ 
          0x41, 0x05, 0x00, 0x01, addressTable[run][0], addressTable[run][1], addressTable[run][2], 
          ( 6 + addressTable[run][0] + addressTable[run][1] + addressTable[run][2]) % 256                                    }
        , 8);
        pStatus = WaitForAnswer;
        ByteNum = 0;
      }
      ledWaitForAnswer.blinkBegin( 60000);
    }
    break;
  case WaitForAnswer:
    //ToDo: Ma gucken was dahinter kommt. Ich erwarte: 
    //    41 Telegramm-Start-Byte
    //    05 Länge der Nutzdaten
    //    00 00 = Anfrage, 01 = Antwort, 03 = Fehler
    //    01 01 = ReadData, 02 = WriteData, 07 = Function Call
    //    XX XX 2 uint8_t Adresse der Daten oder Prozedur
    //    XX Anzahl der Bytes, die in der Antwort erwartet werden
    //    XX Prüfsumme = Summe der Werte ab dem 2 Byte (ohne 41)      
    if ( inputBuffer.getSize() > 0) {
      static uint8_t dataLength;
      static boolean someByteChanged = false;
      uint8_t B = inputBuffer.get();
      switch ( ByteNum) {
      case 0:
        if ( B == 0x06) {
          Debug.print( F( ">0x06,"));
        }
        else {
          Debug.print( F( ">!0x06==")); 
          Debug.print( B, HEX); //          LogCurrentDS+="P"; LogCurrentDS += String( B, HEX); BigLog = true;
          Debug.print( F( ","));
          pStatus = None;
        }
        break; 
      case 1: 
        if ( B == 0x41) {
          Debug.print( F( ">0x41,"));
        }
        else {
          Debug.print( F( ">!0x41==")); 
          Debug.print( B, HEX); //          LogCurrentDS+="Q"; LogCurrentDS += String( B, HEX); BigLog = true;
          Debug.print( F( ","));
          pStatus = None;
        }
        break;
      case 2:
        dataLength = B;
        Debug.print( F( "dataLength=")); 
        Debug.print( B, HEX);
        Debug.print( F( ","));
        break;
      case 3:
        static uint8_t StateByte1;
        StateByte1 = B;
        Debug.print( F( "StateByte1=")); 
        Debug.print( B, HEX);
        Debug.print( F( ","));
        break;
      case 4:
        Debug.print( F( "StateByte2="));         
        Debug.print( B, HEX);        
        Debug.print( F( ","));
        static uint8_t StateByte2;
        StateByte2 = B;
        break;
      case 5:
        Debug.print( F( "addressByte1="));         
        Debug.print( B, HEX);
        static uint8_t addressByte1;
        addressByte1 = B;
        if ( addressByte1 != addressTable[run][0]){     //Das ist nicht die Adresse nach der ich gefragt habe
          Debug.print( F( "!=erwartet(")); 
          Debug.print( addressTable[run][0], HEX);
          Debug.print( F( ")")); 
          pStatus = None;
        }
        Debug.print( F( ","));
        break;
      case 6:
        Debug.print( F( "addressByte2=")); 
        Debug.print( B, HEX);
        static uint8_t addressByte2;
        addressByte2 = B;
        if ( addressByte2 != addressTable[run][1]){     //Das ist nicht die Adresse nach der ich gefragt habe
          Debug.print( F( "!=erwartet(")); 
          Debug.print( addressTable[run][1], HEX);
          Debug.print( F( ")")); 
          pStatus = None;
        }
        Debug.print( F( ","));
        break;
      case 7:
        Debug.print( F( "Bytes=")); 
        Debug.print( B, HEX);
        if ( B != addressTable[run][2]){     //Bitte klären warum die erwartete Anzahl Bytes nicht stimmt
          Debug.print( F( "!=erwartet(")); 
          Debug.print( addressTable[run][2], HEX);
          Debug.println( F( ")")); 
          pStatus = None;
        }
        Debug.print( F( ","));
        someByteChanged = false;
        break;
      default:
        if ( ByteNum < dataLength + 3) {
          Debug.print( F( "Byte")); 
          Debug.print( ByteNum); 
          Debug.print( F( "="));
          Debug.print( B, HEX);
          uint8_t lVByte = lastValueStart( run) + ByteNum - 8;
          if ( lastValue[lVByte] == B) {
            Debug.print( F( "Same,"));
          } 
          else {
            Debug.print( F( "Changed("));
            Debug.print( lastValue[lVByte], HEX);
            Debug.print( F( "),"));
            lastValue[lVByte] = B;
            someByteChanged = true;
          }
        }
        else {
          Debug.print( F( "CheckSum=")); 
          Debug.print( B, HEX);
          //        if ( Prüfsumme falsch)
          //        BIGLog = true;
          //        lastValueChanged[run] = false;
          if ( someByteChanged) {
            Debug.print( F( ",LastValueChanged and anyValueChanged = true")); 
            lastValueChanged[run] = true;
            anyValueChanged = true;
          }
          Debug.println();
          optoPrintLastValuesToDebug();
          pStatus = Initialized;
        }
      }
      ByteNum ++;
    }
  }  
}

void optoWrite(uint8_t Byte) {
  VitoS.write(Byte);
}
void optoWrite(uint8_t *buffer, size_t size) {
  VitoS.write(buffer, size);
}

// SerialEvent occurs whenever a new data comes in the
// hardware serial RX.  This routine is run between each
// time loop() runs, so using delay inside loop can delay
// response.  Multiple bytes of data may be available.
void serialEvent1() {
  while (VitoS.available()) {
    //    cli();
    inputBuffer.put(VitoS.read()); 
    //    sei();
  }
}

void OnLedWaitForAnswerEnd() {
  Debug.print( F( "i"));
  pStatus = None;
}

uint8_t lastValueStart(byte addr){
  uint8_t x = 0;
  //Debug.print( F( "z(")); Debug.print( addr);
  for( uint8_t i=0; i < addr ; i++) {
    x += addressTable[i][2];
    //Debug.print( F( "_")); Debug.print( x);
  }
  //Debug.print( F( ")"));
  return x;
}



