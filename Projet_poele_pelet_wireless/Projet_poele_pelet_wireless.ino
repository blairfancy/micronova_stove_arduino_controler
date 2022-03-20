/*
  Multiple Serial test

  Receives from the main serial port, sends to the others.
  Receives from serial port 1, sends to the main serial (Serial 0).

  This example works only with boards with more than one serial like Arduino Mega, Due, Zero etc.

  The circuit:
  - any serial device attached to Serial port 1
  - Serial Monitor open on Serial port 0

  created 30 Dec 2008
  modified 20 May 2012
  by Tom Igoe & Jed Roach
  modified 27 Nov 2015
  by Arturo Guadalupi

  This example code is in the public domain.
*/


#include <WiFiNINA.h>
#include <WiFiUdp.h>

bool modeAuto = false;
bool remoteConnected = false;
IPAddress remoteIp;
int remotePortNum= 0;
int wifi_status = WL_IDLE_STATUS;
#include "arduino_secrets.h" 
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)

unsigned int localPort = 2390;      // local port to listen on

WiFiUDP Udp;

char packetBuffer[10]; //buffer to hold incoming packet
char ReplyBuffer[128];
char commandToSend[5];

void setup() {

  Serial.begin(19200);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.print("Stating programme by initializing Wifi");
  
  
  //--------------------------------------------------------------------------------
  //Setup WIFI

  WiFi.setHostname(HOST_NAME);
  
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to Wifi network:
  while (wifi_status != WL_CONNECTED) {
    Serial.print("Wifi status: ");
    Serial.println(wifi_status);
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED off 
    
    WiFi.disconnect();
    // wait 3 seconds for connection:
    delay(3000);
    
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    wifi_status = WiFi.begin(ssid, pass);
    
    // wait 10 seconds for connection:
    delay(10000);
  }
  
  Serial.println("Connected to wifi");
  printWifiStatus();

  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  Udp.begin(localPort);
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on
  //--------------------------------------------------------------------------------  


  
  // initialize serial ports for stove comunication:
  Serial1.begin(19200);
  Serial1.setTimeout(500);
}

void loop() {

  int command[5];
  byte msgRec[7];
  char *ptr = NULL;
  int value;
  bool res;
  //----------------------------------------------------------------------
  //WIFI and server management

  wifi_status = WiFi.status();

  if ( wifi_status== WL_CONNECTED) {
    // if there's data available, read a packet
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED ON
    int packetSize = Udp.parsePacket();
    if (packetSize) {

      Serial.print("Received packet of size ");
      Serial.println(packetSize);
      Serial.print("From ");
      remoteIp = Udp.remoteIP();
      Serial.print(remoteIp);
      Serial.print(", port ");
      remotePortNum = Udp.remotePort();
      Serial.println(remotePortNum);
      remoteConnected = true;
      
      // read the packet into packetBufffer
      int len = Udp.read(packetBuffer, 12);
      if (len > 0) {
        Serial.println(len);
        packetBuffer[len-1] = '\0';
      }
      Serial.println(packetBuffer);      
      if (strcmp(packetBuffer,"auto")==0) {
        Serial.println("Automatic mode activated");
        modeAuto=true;
      } else if (strcmp(packetBuffer,"off") == 0) {
        Serial.println("Automatic mode deactivated");
        modeAuto=false;
      } else {
        // unpack command and send it if valid
         byte index = 0;
         ptr = strtok(packetBuffer, ";");  // takes a list of delimiters
         while(ptr != NULL)
         {
              command[index] = (int)strtol(ptr, NULL, 16);
              index++;
              ptr = strtok(NULL, ";");  // takes a list of delimiters
         }
        Serial.print(int(command[0]));
        Serial.print(" ");
        Serial.println(int(command[1]));

        //send request to stove
        if (int(command[0]) & 0x80) {
          //Write
          int dataSize = 2;
          if (int(command[0]) & 0xA5) {
            dataSize = 1;
          }
          res = sendStoveCommand(int(command[0]),int(command[1]),int(command[2]),dataSize);
          if (res) {
            sprintf(ReplyBuffer,"Command sent successfully\n");         
          } else {
            sprintf(ReplyBuffer,"Send Command Failled\n");
          } 
          sendUDPpacket(remoteIp,remotePortNum,ReplyBuffer);
          
        } else {
          //Read
          int dataSize = 2;
          if (int(command[0]) & 0x25) {
            dataSize = 1;
          }
          value = sendGetStoveData(int(command[0]),int(command[1]),dataSize);
          Serial.print("Send reply to the requestor ");
          Serial.println(value);
          sprintf(ReplyBuffer,"%d\n",value);
          sendUDPpacket(remoteIp,remotePortNum,ReplyBuffer);   
        }
      }
      
    }
  } else {
    // attempt to connect to Wifi network:
    while (wifi_status != WL_CONNECTED) {
      Serial.print("Wifi status: ");
      Serial.println(wifi_status);
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(ssid);
      digitalWrite(LED_BUILTIN, LOW);   // turn the LED off
      
      WiFi.disconnect();
      // wait 3 seconds for connection:
      delay(3000);
      // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
      wifi_status = WiFi.begin(ssid, pass);
      
      // wait 10 seconds for connection:
      delay(10000);
    }
    Serial.println("Connected to wifi");
    printWifiStatus();
  
    Serial.println("\nStarting connection to server...");
    // if you get a connection, report back via serial:
    Udp.begin(localPort);
  }  
  //-----------------------------------------------------
  
}

//--------------------------------------------------------
// Send command to the stove
//
//  input:
//          commandType: 0xA1 or 0x80
//          commandAddress :  see stove protocol (i.e 0x62 = temperature set point)
//          value :   value to be set (i.e 24 for 24°c)
//          datasize : size of data in byte 
//
//  return:  True if succeed
//--------------------------------------------------------
bool sendStoveCommand(int commandType, int commandAddress, int value, int datasize)
{
    int chksum1 = (commandType + commandAddress + (value & 0xFF)) & 0xFF;
    int chksum2 = (commandType + commandAddress + 1 + (value >> 8)) & 0xFF;
    bool LSBvalid = false;
    byte msgRec[7];
    int readBytecounter =0;    
    int recCS,recVal;
    
    //Empty reception buffer
    Serial1.read();

    //send LSB
    int sizebuf = sprintf(commandToSend,"%c%c%c%c",commandType,commandAddress,value & 0xFF,chksum1);
    Serial1.write(commandToSend,sizebuf);

    //get reply
    readBytecounter = Serial1.readBytes(msgRec, 6);
    if (readBytecounter == 2) {
      //probable stove not responding or echo removal circuit connected
      Serial.println("Received 2 bytes");
      recVal=int(msgRec[1]);
      recCS=int(msgRec[0]);
    } else if (readBytecounter == 6) {
      //probable TX and RX message in buffer
      //Check if TX message match first 4 bytes of repply (echo)
      if ( (int(msgRec[0]) == commandType) & (int(msgRec[1]) == commandAddress) & (int(msgRec[2]) == value & 0xFF) & (int(msgRec[3]) == chksum1) ) {
        Serial.println("Received echo and match with command");
        //Save stove reply
        recVal=int(msgRec[5]);
        recCS=int(msgRec[4]);  
      } else {
        Serial.println("Received echo but not match with command");
        return false;
      }
    } else {
      Serial.println("Wrong numbers of bytes received");
      Serial.println(readBytecounter);
      return false;
    }

    Serial.println("LSB received");
    Serial.println(recVal); 
    Serial.println(recCS); 
    if (recCS == chksum1) {
      //command validated return value to udp
      Serial.println("LSB checksum ok");
      value = recVal;
      LSBvalid = true;
    }

    if (datasize == 1) {
       if (LSBvalid) {
          return true; 
       } else {
          return false;
       }
    }

    //Empty reception buffer
    Serial1.read();

    //send MSB
    sizebuf = sprintf(commandToSend,"%c%c%c%c",commandType,commandAddress+1,value >> 8,chksum2);
    Serial1.write(commandToSend,sizebuf);

    //get reply
    readBytecounter = Serial1.readBytes(msgRec, 6);

    if (readBytecounter == 2) {
      //probable stove not responding or echo removal circuit connected
      Serial.println("Received 2 bytes");
      recVal=int(msgRec[1]);
      recCS=int(msgRec[0]);
    } else if (readBytecounter == 6) {
      //probable TX and RX message in buffer
      //Check if TX message match first 4 bytes of repply (echo)
      if ( (int(msgRec[0]) == commandType) & (int(msgRec[1]) == commandAddress+1) & (int(msgRec[2]) == value >> 8) & (int(msgRec[3]) == chksum2) ) {
        Serial.println("Received echo and match with command");
        //Save stove reply
        recVal=int(msgRec[5]);
        recCS=int(msgRec[4]);  
      } else {
        Serial.println("Received echo but not match with command");
        return false;
      }
    } else {
      Serial.println("Wrong numbers of bytes received");
      Serial.println(readBytecounter);
      return false;
    }
    
    if (recCS == chksum2) {
      //command validated return value to udp
      if (LSBvalid) return true;
    }

  return false;
}




//--------------------------------------------------------
// Send command to the stove
//
//  input:
//          commandType: 0x21 or 0x0 or 0x25
//          commandAddress :  see stove protocol (i.e 0x62 = temperature set point)
//          datasize : size of data in byte 
//
//  return:  value requested or
//           0x10000: when wrong number of bytes received
//           0x20000: when Echo not match to Command
//           0x30000: when LSB checksum onvalid
//           0x40000: when MSB checksum onvalid
//           0x50000: Unknown issue
//--------------------------------------------------------
int sendGetStoveData(int commandType, int commandAddress, int datasize)
{
    bool LSBvalid = false;
    int chksum;
    int value = 0;
    byte msgRec[7];
    int readBytecounter =0; 
    int recCS,recVal;

    
    //Empty reception buffer
    Serial1.read();
    
    //send LSB
    int sizebuf = sprintf(commandToSend,"%c%c",int(commandType),int(commandAddress));
    Serial1.write(commandToSend,sizebuf);
    Serial.println(commandToSend);
    //get reply
    readBytecounter = Serial1.readBytes(msgRec, 4);
    if (readBytecounter == 2) {
      //probable stove not responding or echo removal circuit connected
      Serial.println("Received 2 bytes");
      recVal=int(msgRec[1]);
      recCS=int(msgRec[0]);
    } else if (readBytecounter == 4) {
      //probable TX and RX message in buffer
      //Check if TX message match first 4 bytes of repply (echo)
      if ( (int(msgRec[0]) == commandType) & (int(msgRec[1]) == commandAddress) ) {
        Serial.println("Received echo and match with command");
        //Save stove reply
        recVal=int(msgRec[3]);
        recCS=int(msgRec[2]);  
      } else {
        Serial.println("Received echo but not match with command");
        return 0x20000;
      }
    } else {
      Serial.println("Wrong numbers of bytes received");
      Serial.println(readBytecounter);
      return 0x10000;
    }
    
    chksum = (commandType + commandAddress + recVal) & 0xFF;
    Serial.println("LSB received");
    Serial.println(recVal); 
    Serial.println(recCS); 
    if (recCS == chksum) {
      //command validated return value to udp
      Serial.println("LSB checksum ok");
      value = recVal;
      LSBvalid = true;
    } else {
      return 0x30000;
    }

    if (datasize == 1) {
          return value; 
    }

    //Empty reception buffer
    Serial1.read();

    //send MSB
    sizebuf = sprintf(commandToSend,"%c%c",int(commandType),int(commandAddress+1));
    Serial1.write(commandToSend,sizebuf);

    //get reply
    readBytecounter = Serial1.readBytes(msgRec, 4);
    if (readBytecounter == 2) {
      //probable stove not responding or echo removal circuit connected
      Serial.println("Received 2 bytes");
      recVal=int(msgRec[1]);
      recCS=int(msgRec[0]);
    } else if (readBytecounter == 4) {
      //probable TX and RX message in buffer
      //Check if TX message match first 4 bytes of repply (echo)
      if ( (int(msgRec[0]) == commandType) & (int(msgRec[1]) == commandAddress+1) ) {
        Serial.println("Received echo and match with command");
        //Save stove reply
        recVal=int(msgRec[3]);
        recCS=int(msgRec[2]);  
      } else {
        Serial.println("Received echo but not match with command");
        return 0x20000;
      }
    } else {
      Serial.println("Wrong numbers of bytes received");
      Serial.println(readBytecounter);
      return 0x10000;
    }

    chksum = (commandType + commandAddress + 1 + recVal) & 0xFF;
    Serial.println("MSB received");      
    Serial.println(recVal); 
    Serial.println(recCS); 
    if (recCS == chksum) {
      //command validated return value to udp
      Serial.println("MSB checksum ok");
      if (LSBvalid) {
        value += int(recVal << 8 );
        return value;
      }
    } else {
      return 0x40000;
    }
    
  return 0x50000;
}



void sendUDPpacket(IPAddress& address, int UDPport, char* message)
{
  Udp.beginPacket(address, UDPport); //NTP requests are to port
  Udp.write(message);
  Udp.endPacket(); 
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
