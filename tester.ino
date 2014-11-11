
#include <AltSoftSerial.h>
#include <EEPROM.h>
#include "EEPROMAnything.h"
#include "ESP8266.h"

// CHANGE THIS TO 2 TO SEE MOST OF THE OUTPUT FROM THE LIBRARY
#define DEBUG_LEVEL 0

// DO NOT try to use the default SoftSerial it WILL NOT work
AltSoftSerial softSerial;

// instantiate the wifi object with STA mode and server on port 80 and the baud rate
// for communicating with the module
ESP8266 wifi(WIFI_MODE_STA, 38400, DEBUG_LEVEL);

#define BUFFER_SIZE 255
#define rs232 softSerial  // alias the software port as 'rs232'

// serial data buffer
char sb[BUFFER_SIZE];
int sctr = 0;
bool connected = false;

int dataCallback(char *data) {
  rs232.println(data);
}

void connectCallback() {
  connected = true;
}

void setup() {

  int ret;
  
  // Start the AltSoftSerial port at 9600 - can theoretically go as high as 56K
  rs232.begin(9600);
  rs232.println("Starting up WiFi to RS232 Bridge");
  
  // THE FOLLOWING CODE READS VALUES FOR ssid, password, etc FROM THE EEPROM
  // ON AN Arudino Pro Mini. IF YOU DON'T WANT TO MESS WITH THIS, REPLACE IT
  // WITH SIMPLE STRING LITERALS
  
  // read the SSID from eeprom (up to 48 bytes: 0 to 47)
  char ssid[48];
  memset(ssid, 0, 48);
  eepromReadString(0, 48, ssid);
  
  // read the password from the eeprom (up to 24 bytes: 48 to 71)
  char password[24];
  memset(password, 0, 24);
  eepromReadString(48, 24, password);
  
  // read the port number from the eeprom (2 bytes: 72-73)
  int port = eepromReadInt(72);

  // read the device name from the eeprom (up to 48 bytes: 74 to 121)
  char device[48];
  memset(device, 0, 48);
  eepromReadString(74, 48, device);

  // check for empty ssid and password
  if ((ssid[0] <= 0) || (password[0] <= 0)) {
    rs232.println(F("The ID and PASSWORD must be set before you can use this device."));
    rs232.println(F("\tAT+SSID=yourssid"));
    rs232.println(F("\tAT+PASSWORD=yourpassword"));
    return;
  }

  // if the port value is zero or -1, use the default
  if (port <= 0) {
    port = 8000;
  }

  // check for empty device name
  if (device[0] <= 0) {
    rs232.println(F("The device ID has not been set. Defaulting to 'wifi-serial'."));
    rs232.println(F("\tAT+DEVICE=device-name"));
    strcpy(device, "wifi-serial");
  }
  
  // start up the wifi connection
  // returns true if the module responds / resets properly
  ret = wifi.initializeWifi(&dataCallback, &connectCallback);
  if (ret != WIFI_ERR_NONE) {   
    rs232.println(F("Wifi initialization failed."));
    rs232.println(ret);
    return;
  }
  
  rs232.println(F("Wifi initialized"));
  
  // connect the module to the provided SSID
  ret = wifi.connectWifi(ssid, password);
  if (ret != WIFI_ERR_NONE) {
    rs232.println(F("Unable to associated with access point."));
    rs232.println(ret);
    return;
  }
  
  rs232.println(F("Wifi connected"));
  rs232.println(wifi.ip());
  
  // start the server
  bool svr = wifi.startServer(80);
  if (svr) {
    // enables a beacon that can be used for discovery - UDP port 34807
    wifi.enableBeacon(device);
  }

  /*
  // demo of client mode - connects to 4040 on 10.0.1.3 and sends a message
  
  if (wifi.startClient("10.0.1.3", 4040, 1000)) {
    rs232.println("client connected");
    wifi.send("bazinga!"); 
  }
*/

}

void loop() {
  
  int v;
  
  // make the wifi library run
  wifi.run();

  // process serial messages
  while(rs232.available() > 0) {
    v = rs232.read();
    sb[sctr] = v;
    sctr++;
    if (v == 10) {
      sb[sctr] = 0;
      processSerialMessage(sctr);
    }
  }
}

// process a complete message from the Serial side
// and send the payload to the WiFi connection
void processSerialMessage(int len) {
  
  // look for command messages - start with "AT"
  // really should require a pin high to put in programming
  // mode
  if ((strncmp(sb, "AT", 2) == 0) && (connected == false)) {
    if (processSerialCommand()) {
      rs232.println("OK"); 
    }
  } else {
    // data message - no command - just pass it on to the WiFi
    wifi.send(sb);
  }
  
  // reset the character counter
  sctr = 0;
}


// process a serial command
bool processSerialCommand() {
  
  // command message - AT+something or AT+something=XXX
  if (strncmp(sb+2, "+", 1) == 0) {
    
    // eliminate any \r and \n at the end
    for (int i = 0; i < strlen(sb); i++) {
      if ((int)sb[i] == 13) {
        sb[i] = 0;
      }
      if ((int)sb[i] == 10) {
        sb[i] = 0;
      } 
    }
    
    char command[32];
    memset(command, 0, 32);
    char value[48];
    memset(value, 0, 48);
    
    char *split = strchr(sb+3, '=');
    if (split) {
      int index = (split - (sb+3));
      strncpy(command, sb+3, index);
      strcpy(value, split+1);
    } else {
      strcpy(command, sb+3);
    }

    rs232.print("Command: ");
    rs232.println(command);
    
    rs232.print("Value: ");
    rs232.println(value);

    // set SSID
    if (strncmp(command, "SSID", 4) == 0) {
      //strcpy(ssid, value);
      eepromWriteString(0, 48, value);
      return true;
    }

    if (strncmp(command, "PASSWORD", 8) == 0) {
      //strcpy(password, value);
      eepromWriteString(48, 24, value);
      return true;
    }
    
    if (strncmp(command, "PORT", 4) == 0) {
      int port = atoi(value);
      eepromWriteInt(72, port);
      return true;
    }
    
    // device name
    if (strncmp(command, "DEVICE", 4) == 0) {
      eepromWriteString(74, 48, value);
      return true;
    }
    
    return false;
  }
  
  return true;
}



