#include "Arduino.h"
#include "ESP8266.h"

#define wifi Serial
#define SVR_CHAN 1
#define BCN_CHAN 2
#define CLI_CHAN 3
#define BUFFER_SIZE 255
#define BEACON_PORT 34807

enum connectMode {
  CONNECT_MODE_NONE = 0,
  CONNECT_MODE_SERVER,
  CONNECT_MODE_CLIENT
};

int  _mode;
long _baudrate;
char _ipaddress[15];
char _broadcast[15];
int  _port;
char _device[48];
char _ssid[48];
char _password[24];
bool _beacon;
long _beaconInterval;
long _previousMillis;
int _replyChan;
DataCallback _dcb;
ConnectCallback _ccb;
char _wb[BUFFER_SIZE];
int _wctr = 0;
bool _connected;
int _connectMode;
int _debugLevel = 0;

ESP8266::ESP8266(int mode, long baudrate, int debugLevel)
{
  _mode = mode;
  _baudrate = baudrate;
  _debugLevel = debugLevel;
  _port = 8000;
  _replyChan = 0;
  memset(_ipaddress, 0, 15);
  memset(_broadcast, 0, 15);
  memset(_device, 0, 48);
  memset(_ssid, 0, 48);
  memset(_password, 0, 24);
  _beacon = false;
  _beaconInterval = (10000L);
  _previousMillis = 0;
  _connected = false;
}

int ESP8266::initializeWifi(DataCallback dcb, ConnectCallback ccb)
{

  if (dcb) {
    _dcb = dcb;
  }

  if (ccb) {
    _ccb = ccb;
  }

  wifi.begin(_baudrate);
  wifi.setTimeout(5000);

  //delay(500);
  clearResults();

  // check for presence of wifi module
  wifi.println(F("AT"));
  delay(500);
  if(!searchResults("OK", 1000, _debugLevel)) {
    return WIFI_ERR_AT;
  }

  //delay(500);

  // reset WiFi module
  wifi.println(F("AT+RST"));
  delay(500);
  if(!searchResults("ready", 5000, _debugLevel)) {
    return WIFI_ERR_RESET;
  }

  delay(500);

  // set the connectivity mode 1=sta, 2=ap, 3=sta+ap
  wifi.print(F("AT+CWMODE="));
  wifi.println(_mode);
  //delay(500);

  clearResults();

  return WIFI_ERR_NONE;
}

int ESP8266::connectWifi(char *ssid, char *password)
{

  strcpy(_ssid, ssid);
  strcpy(_password, password);

  // set the access point value and connect
  wifi.print(F("AT+CWJAP=\""));
  wifi.print(ssid);
  wifi.print(F("\",\""));
  wifi.print(password);
  wifi.println(F("\""));
  delay(100);
  if(!searchResults("OK", 30000, _debugLevel)) {
    return WIFI_ERR_CONNECT;
  }

  // enable multi-connection mode
  if (!setLinkMode(1)) {
    return WIFI_ERR_LINK;
  }

  // get the IP assigned by DHCP
  getIP();

  // get the broadcast address (assumes class c w/ .255)
  getBroadcast();

  return WIFI_ERR_NONE;
}

bool ESP8266::disconnectWifi()
{

}

bool ESP8266::enableBeacon(char *device)
{
  // you can only beacon if you're a server
  if (_connectMode != CONNECT_MODE_SERVER)
    return false;

  bool ret;
  strcpy(_device, device);
  ret = startUDPChannel(BCN_CHAN, _broadcast, BEACON_PORT);
  _beacon = ret;
  return ret;
}

bool ESP8266::disableBeacon()
{
  _beacon = false;
}



bool ESP8266::send(char *data)
{
  int chan;
  if (_connectMode == CONNECT_MODE_SERVER) {
    chan = _replyChan;
  } else {
    chan = CLI_CHAN;
  }
  return sendData(chan, data);
}

void ESP8266::run()
{
  int v;
  char _data[255];
  unsigned long currentMillis = millis();

  if (currentMillis - _previousMillis < _beaconInterval) {

    // process wifi messages
    while(wifi.available() > 0) {
      v = wifi.read();
      if (v == 10) {
        _wb[_wctr] = 0;
        _wctr = 0;
        processWifiMessage();
      } else if (v == 13) {
        // gndn
      } else {
        _wb[_wctr] = v;
        _wctr++;
      }
    }


  } else {
    _previousMillis = currentMillis;
    if (_beacon == false) return;

    // create message text
    char *line1 = "{\"event\": \"beacon\", \"ip\": \"";
    char *line3 = "\", \"port\": ";
    char *line5 = ", \"device\": \"";
    char *line7 = "\"}\r\n";

    // convert port to a string
    char p[6];
    itoa(_port, p, 10);

    // get lenthg of message text
    memset(_data, 0, 255);
    strcat(_data, line1);
    strcat(_data, _ipaddress);
    strcat(_data, line3);
    strcat(_data, p);
    strcat(_data, line5);
    strcat(_data, _device);
    strcat(_data, line7);

    sendData(BCN_CHAN, _data);
  }
}

bool ESP8266::startServer(int port, long timeout)
{

  // cache the port number for the beacon
  _port = port;

  wifi.print(F("AT+CIPSERVER="));
  wifi.print(SVR_CHAN);
  wifi.print(F(","));
  wifi.println(_port);
  if(!searchResults("OK", 500, _debugLevel)){
    return false;
  }

  // send AT command
  wifi.print(F("AT+CIPSTO="));
  wifi.println(timeout);
  if(!searchResults("OK", 500, _debugLevel)) {
    return false;
  }

  _connectMode = CONNECT_MODE_SERVER;
  return true;
}

bool ESP8266::startClient(char *ip, int port, long timeout)
{
  wifi.print(F("AT+CIPSTART="));
  wifi.print(CLI_CHAN);
  wifi.print(F(",\"TCP\",\""));
  wifi.print(ip);
  wifi.print("\",");
  wifi.println(port);
  delay(100);
  if(!searchResults("OK", timeout, _debugLevel)) {
    return false;
  }

  _connectMode = CONNECT_MODE_CLIENT;
  return true;
}

char *ESP8266::ip()
{
  return _ipaddress;
}

int ESP8266::scan(char *out, int max)
{
  int timeout = 10000;
  int count = 0;
  int c = 0;

  if (_debugLevel > 0) {
    char num[6];
    itoa(max, num, 10);
    debug("maximum lenthg of buffer: ");
    debug(num);
  }
  wifi.println(F("AT+CWLAP"));
  long _startMillis = millis();
  do {
    c = wifi.read();
    if ((c >= 0) && (count < max)) {
      // add data to list
      out[count] = c;
      count++;
    }
  } while(millis() - _startMillis < timeout);

  return count;
}

// *****************************************************************************
// PRIVATE FUNCTIONS BELOW THIS POINT
// *****************************************************************************

// process a complete message from the WiFi side
// and send the actual data payload to the serial port
void ESP8266::processWifiMessage() {
  int packet_len;
  int channel;
  char *pb;

  // if the message is simply "Link", then we have a live connection
  if(strncmp(_wb, "Link", 5) == 0) {

    // reduce the beacon frequency by increasing the interval
    _beaconInterval = 30000; //false;

    // flag the connection as active
    _connected = true;

    // if a connection callback is set, call it
    if (_ccb) _ccb();
  } else

  // message packet received from the server
  if(strncmp(_wb, "+IPD,", 5)==0) {

    // get the channel and length of the packet
    sscanf(_wb+5, "%d,%d", &channel, &packet_len);

    // cache the channel ID - this is used to reply
    _replyChan = channel;

    // if the packet contained data, move the pointer past the header
    if (packet_len > 0) {
      pb = _wb+5;
      while(*pb!=':') pb++;
      pb++;

      // execute the callback passing a pointer to the message
      if (_dcb) {
        _dcb(pb);
      }

      // DANGER WILL ROBINSON - there is no ring buffer or other safety net here.
      // the application should either use the data immediately or make a copy of it!
      // do NOT block in the callback or bad things may happen

    }
  } else {
    // other messages might wind up here - some useful, some not.
    // you might look here for things like 'Unlink' or errors
  }
}

bool ESP8266::sendData(int chan, char *data) {

  // start send
  wifi.print(F("AT+CIPSEND="));
  wifi.print(chan);
  wifi.print(",");
  wifi.println(strlen(data));

  // send the data
  wifi.println(data);

  delay(50);

  // to debug only
  searchResults("OK", 500, _debugLevel);

  return true;
}

bool ESP8266::setLinkMode(int mode) {
  wifi.print(F("AT+CIPMUX="));
  wifi.println(mode);
  delay(500);
  if(!searchResults("OK", 1000, _debugLevel)){
    return false;
  }
  return true;
}

bool ESP8266::startUDPChannel(int chan, char *address, int port) {
  wifi.print(F("AT+CIPSTART="));
  wifi.print(chan);
  wifi.print(F(",\"UDP\",\""));
  wifi.print(address);
  wifi.print("\",");
  wifi.println(port);
  if(!searchResults("OK", 500, _debugLevel)) {
    return false;
  }
  return true;
}

// private convenience functions
bool ESP8266::getIP() {

  char c;
  char buf[15];
  int dots, ptr = 0;
  bool ret = false;

  memset(buf, 0, 15);

  wifi.println(F("AT+CIFSR"));
  delay(500);
  while (wifi.available() > 0) {
    c = wifi.read();

    // increment the dot counter if we have a "."
    if ((int)c == 46) {
      dots++;
    }
    if ((int)c == 10) {
      // end of a line.
      if ((dots == 3) && (ret == false)) {
        buf[ptr] = 0;
        strcpy(_ipaddress, buf);
        ret = true;
      } else {
        memset(buf, 0, 15);
        dots = 0;
        ptr = 0;
      }
    } else
    if ((int)c == 13) {
      // ignore it
    } else {
      buf[ptr] = c;
      ptr++;
    }
  }

  return ret;
}


bool ESP8266::getBroadcast() {

  int i, c, dots = 0;

  if (strlen(_ipaddress) < 7) {
    return false;
  }

  memset(_broadcast, 0, 15);
  for (i = 0; i < strlen(_ipaddress); i++) {
    c = _ipaddress[i];
    _broadcast[i] = c;
    if (c == 46) dots++;
    if (dots == 3) break;
  }
  i++;
  _broadcast[i++] = 50;
  _broadcast[i++] = 53;
  _broadcast[i++] = 53;

  return true;
}

void ESP8266::debug(char *msg) {
  if (_dcb && (_debugLevel > 0)) {
    _dcb(msg);
  }
}

bool ESP8266::searchResults(char *target, long timeout, int dbg)
{
  int c;
  int index = 0;
  int targetLength = strlen(target);
  int count = 0;
  char _data[255];

  memset(_data, 0, 255);

  long _startMillis = millis();
  do {
    c = wifi.read();

    if (c >= 0) {

      if (dbg > 0) {
        if (count >= 254) {
          debug(_data);
          memset(_data, 0, 255);
          count = 0;
        }
        _data[count] = c;
        count++;
      }

      if (c != target[index])
        index = 0;

      if (c == target[index]){
        if(++index >= targetLength){
          if (dbg > 1)
            debug(_data);
          return true;
        }
      }
    }
  } while(millis() - _startMillis < timeout);

  if (dbg > 0) {
    if (_data[0] == 0) {
      debug("Failed: No data");
    } else {
      debug("Failed");
      debug(_data);
    }
  }
  return false;
}

void ESP8266::clearResults() {
  char c;
  while(wifi.available() > 0) {
    c = wifi.read();
  }
}
