//#define UseDHCPinSketch
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE };   // MAC address.
prog_char website[] PROGMEM = "volkszaehler";
boolean sendEnabled = false;
unsigned long TimerEthSend;

EthernetClient client;

void EthInit() {
  Debug.println( F( "Initialising the Ethernet controller"));
  if (Ethernet.begin(mac) == 0) {
    Debug.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for(;;)
      ;
  }
  // give the Ethernet shield a second to initialize:
  delay(1000);
  Debug.print("My IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Debug.print(Ethernet.localIP()[thisByte], DEC);
    Debug.print("."); 
  }
  Debug.println();

}

void ethDoEvents() {
  static boolean lastConnected = false;                 // state of the connection last time through the main loop

  // if there's incoming data from the net connection.
  // send it out the Debug port.  This is for debugging
  // purposes only:
  if (client.available()) {
    char c = client.read();
    Debug.print(c);
  }
  // if there's no net connection, but there was one last time
  // through the loop, then stop the client:
  if (!client.connected() && lastConnected) {
    Debug.println();
    Debug.println("disconnecting.");
    client.stop();
  }
  // Maybe I will get a new IP so lets do this when no client is connected
  if(!client.connected()) {
    Ethernet.maintain();
  }
  // if you're not connected, and ten seconds have passed since
  // your last connection, then connect again and send data:
  if(sendEnabled) {
    sendEnabled = false;
    char DataBuffer[700];
    PString Data( DataBuffer, sizeof( DataBuffer));
    Debug.print( F( "Free Memory:")); 
    Debug.println( freeMemory());
    Debug.println("connecting...");
    step0.blinkBegin( 1000, 0, 1);
    byte server[] = { 
      192, 168, 54, 20   };
    //  if (client.connect(website, 80)) {
    if (client.connect(server, 80)) {
      Debug.println("connected");
      // send the HTTP PUT request:
      Data.println( F( "{"));
      Data.println( F( "\"From\":\"A1\","));
      Data.print( F( "\"Log\":\""));
      Data.print( Log);
      Data.print( F( "\""));
      Log.begin();
      optoAppendJsonSnipet(Data);
      Data.print( F( "}"));                            //Debug.print( F( "}"));
      client.println( F("POST /VitoR.php HTTP/1.0"));
      client.print  ( F("Host: "));
      client.println( website);
      //    client.println( F("Connection: close"));
      //    client.println( F("Content-Type: text/csv"));
      client.print  ( F("Content-Length: "));
      client.println( Data.length());
      client.println();
      client.println( Data);
      Debug.println(Data);
      Debug.print( F( "Data.Length = "));    
      Debug.println( Data.length());
    } 
    else {
      Debug.println( F( "Connection Failed"));
    }
  }
  // store the state of the connection for next time through
  // the loop:
  lastConnected = client.connected();
}
void ethSend() {
  sendEnabled = true;
  TimerEthSend = millis();
}

boolean ethReadyToSend() {
  return ( !client.connected() && (millis() - TimerEthSend > 10000));
}

