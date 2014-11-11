/*
    Convenience functions for us with ESP8266 module.
    
    Presumes the 8266 is attached to the hardware Serial port on the Arduino
    
*/

#include <Arduino.h>  // for type definitions

#ifndef ESP8266_h
#define ESP8266_h

#include <AltSoftSerial.h>

typedef int (*DataCallback)(char *);
typedef void (*ConnectCallback)(void);

enum wifiModes {
  WIFI_MODE_STA = 1,
  WIFI_MODE_AP,
  WIFI_MODE_APSTA
};

enum wifiErrors {
  WIFI_ERR_NONE = 0,
  WIFI_ERR_AT,
  WIFI_ERR_RESET,
  WIFI_ERR_CONNECT,
  WIFI_ERR_LINK  
};

class ESP8266
{
  public:
    // constructor - set link mode and server port
    ESP8266(int mode = 1, long baudrate = 9600, int debugLevel = 0);
    
    // init / connect / disconnect access point
    int initializeWifi(DataCallback dcb, ConnectCallback ccb);
    int connectWifi(char *ssid, char *password);
    bool disconnectWifi();
    
    // server
    bool startServer(int port = 8000, long timeout = 300);
    
    // client
    bool startClient(char *ip, int port, long timeout = 300);
    
    // discovery beacon
    bool enableBeacon(char *device);
    bool disableBeacon();
    
    // send data across the link
    bool send(char *data);
    
    // process wifi messages - MUST be called from main app's loop
    void run();

    // informational
    char *ip();
    int scan(char *out, int max);
  private:
    void clearResults();
    bool sendData(int chan, char *data);
    bool setLinkMode(int mode);
    bool startUDPChannel(int chan, char *address, int port);
    void processWifiMessage();
    bool getIP();
    bool getBroadcast();
    void debug(char *msg);
    bool searchResults(char *target, long timeout, int dbg = 0);
};

#endif


