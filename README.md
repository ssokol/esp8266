# ESP8266 Library and Demo

Author: Steven Sokol <steven.sokol@gmail.com>

Created: November 6, 2014

## Table of Contents
1. [Overview](#section-features)
2. [Usage](#section-usage)
3. [Reference](#section-reference)
	a. [Enumerated Values](#section-reference-enums)
	b. [Constructor](#section-reference-constructor)
	c. [initializeWifi()](#section-reference-initalizewifi)
	d. [connectWifi()](#section-reference-connectwifi)
	e. [startServer()](#section-reference-startserver)
	f. [startClient()](#section-reference-startclient)
	g. [enableBeacon()](#section-reference-enablebeacon)
	h. [send()](#section-reference-send)
	i. [run()](#section-reference-run)
	j. [ip()](#section-reference-ip)
4. [Limitations](#section-limitations)
5. [License](#section-license)
	

## [Overview](id:section-features)
The project consists of the ESP8266 library and a sample application that demonstrates the use of the library to create a generic serial <=> wifi bridge.

The library handles the configuration of the connection between the module and an access point. It begins by checking for and initializing the module with a reset. If the reset succeeds, the module can then be connected to an access point. Once the connection is established, the module starts a TCP server.

The library can inform the application when a client connection occurs using a callback. Data that arrives from a connected client is also delivered to the application via a callback.

The library supports a primitive sort of discovery for situations where the module's IP address is unknown: it can broadcast a JSON-formatted UDP "beacon" containing the IP address assigned by DHCP, the port on which the server is listening and an arbitrary device identifier. By default the beacon is sent out on port 34807.

## [Usage](id:section-usage)

The library assumes that the ESP8266 is connected to the hardware Serial pins (or, in the case of a Mega, UART0).

> **WARNING:** If you are using the library to go "serial-to-serial" on an Arduino with only a single hardware serial port, do **NOT** use the default SoftwareSerial library or NewSoftSerial. Both result in maddening timing issues. Instead, use AltSoftSerial which can be found [here](https://www.pjrc.com/teensy/td_libs_AltSoftSerial.html).

To use the library, simply add a reference to the header file and create a single ESP8266 object passing in the connection mode and the baudrate for the connection to the module:

	#include "ESP8266.h"
	
	ESP8266 wifi(WIFI_MODE_STATION, 38400);
	
Next, initialize the library, passing in callbacks for data and connection events:

	void dataCallback(char *data) {
		// do something with the data
	}
	
	void connectCallback() {
		// do something in response to the connection
	}
	
	int ret = wifi.initializeWifi(&dataCallback, &connectCallback);
	if (ret != WIFI_ERR_NONE) {
		// your module was either not found or is using a different baud rate
	}

If that worked out (you received WIFI_ERR_NONE) then start the connection by passing in the SSID and password:

	int ret = wifi.connectWifi("myssid", "mypassword");
	if (ret != WIFI_ERR_NONE) {
		// connection failed for some reason. could be that the ssid or password are
		// incorrect. could be out of range. could be hardware issues (you only paid $5)
	}

If the module connects, you're ready to start a server. The server will listen on the assigned port for connections. If you're using the module purely as a client, you can skip over this.

	int myPort = 80;
	long myTimeout = 25000;
	bool ret = wifi.startServer(myPort, timeout);
	
If the function returns true, you should be able to create a TCP connection on the assigned port. When you do, your connectCallback() should be called, letting your app know that it has a connection. When data arrives, it will result in a callback to your dataCallback() function. To send data to connected client, use the send() method.

Sending data is easy (as long as you're sending text - this library does not support binary data transfer at this point). You can send data when connected in either client or server mode.

	wifi.send("hello world!");

Because the module gets its IP from DHCP, it can be difficult to use it for server applications. To work around this, the library provides a very primitive discovery and autoconfiguration method: it broadcasts UDP datagrams with a JSON-encoded blob of text that tells you the IP address, port and device name. The beacon fires every 10 seconds when no client is connected, and every 30 seconds after a connection has been made.

	wifi.enableBeacon("MyWackyDevice");
	
That's all there is to it. The call will return false if you don't have a server running.


**Client** mode allows you to establish a TCP connection to a remote server. Once connected, you can send data to the server using the send() method. Data sent to you will arrive on the data callback established when you initialized the library.

	if (wifi.connectServer("10.0.8.22", 80)) {
		wifi.send("hello, world!");
	}

> **IMPORTANT:** Your app must call the 'run()' function in your loop or the library won't work!!!

Finally, your app must call the 'run()' function in the library. This allows the library to process messages on the module.

	loop() {
		// run all wifi stuff
		wifi.run();
		
		// now do your stuff...
	}

## [Reference](id:section-reference)

### [Enumerated Values](id:section-reference-enums)

Here are some enumerations from the library and what the various values mean:

**Wifi Mode Values**

* WIFI_MODE_STA = 1 - station mode
* WIFI_MODE_AP = 2 - access point mode
* WIFI_MODE_APSTA = 3 - dual station / access point mode (untested)

**Wifi Error Values**

* WIFI_ERR_NONE = 0 - no error returned
* WIFI_ERR_AT = 1 - no response to 'AT' command (module connected?)
* WIFI_ERR_RESET = 2 - reset failed (adequate power supply?)
* WIFI_ERR_CONNECT = 3 - access point associate failed (bad ssid or password?)
* WIFI_ERR_LINK = 4 - error enabling multi-link mode

### [ESP826](id:section-reference-constructor) (constructor)

**Arguments:**

* Wireless mode
	* optional
	* WIFI_MODE_STA, WIFI_MODE_AP, WIFI_MODE_APSTA
	* defaults to STA
* baud rate
	* optional
	* any baud rate supported by the module + your Arduino
	* defaults to 9600
	* does NOT change the modules baud rate (see setBaudRate for that)
* debug level
	* optional
	* defaults to 0 (off)
	* valid values 0 - 2
	* causes debug data to be called back to your app through the dataCallback set in the constructor (could probably be done more legantly with a dedicated debug callback)
	
**Returns:** 

void (nothing)

**Example:**

	ESP8266 myWifi(WIFI_MODE_STA, 80, 115400);

### [initializeWifi()](id:section-reference-initalizewifi)

Initialize the library and set callbacks.

**Arguments:**

* data callback
	* address of a function to call when data arrives
	* must have a single char * parameter that receives the data
	* must return nothing
* connect callback
	* address of a function to call when a connection occurs
	* must have no parameters
	* must return nothing
	
**Returns:** 

An integer value that maps to the wifi errors enumeration above.

**Example:**

	int ret = myWifi.initializeWifi(&myDataCallback, &myConnectionCallback);

### [connectWifi()](id:section-reference-connectwifi)

Connect to an access point.

**Arguments:**

* ssid
	* char * or string literal
	* name of the access point with which to associate
* password
	* char * or string literal
	* password value
	* use empty string "" for unsecured networks

**Returns:**

An integer value that maps to the wifi error enumeration above.

**Example:**

	int ret = myWifi.connectWifi("accesspoint", "password");

### [startServer()](id:section-reference-startserver)

Create a TCP server to which clients may connect.

**Arguments:**

* port
	* integer port number (1 - 65534) on which to listen
	* defaults to 8000
* timeout
	* time in seconds after which inactive clients are disconnected
	* minimum useful value appear to be ~10 seconds
	* defaults to 300 (5 minutes)

**Returns:**

Boolean. True = success. False = failure.

**Example:**

	bool ret = myWifi.startServer(80, 500)


### [startClient()](id:section-reference-startclient)

Create a client TCP connection to a remote server.

**Arguments:**

* ip
	* ip address to which the client should connect
* port
	* integer port number (1 - 65534) on which to connect
* timeout
	* time in seconds to wait for the connection to succeed

**Returns:**

Boolean. True = success. False = failure.

**Example:**

	bool ret = myWifi.startServer(80, 500)

### [enableBeacon()](id:section-reference-enablebeacon)

Enable the UDP beacon which posts a JSON datagram to the broadcast address (see limitations below for caveats) every 10 seconds. JSON datagrams include the IP address and port of the server and the device name provided when the beacon is enabled.

NOTE: the beacon is currently hard-coded to use UDP port 34807. If you want/need to change this, you will need to edit the library code. At some point in the future I will update to make the beacon port configurable.

Beacon frequency is every 10 seconds when no server connections are active. Once a server connection occurrs, the frequency goes to ever 30 seconds.

The beacon makes the assumption that your broadcast address is the same as the IP the module received with the last octet set to 255. This sucks, but since there's no way to get the actual broadcast from the module, that's the best I could do. At some point it should probably be made configurable as well.

**Arguments:**

* device
	* char * or string literal identifing the device
	* use something unique if you will have muliple devices on your network
	* see the sample app for an example of how this can be stored in EEPROM

**Returns:**

Boolean. True = success - beacon enabled. False = failure.

**Example:**

	bool ret = myWifi.enableBeacon("MyWeatherStation");
	
	// sample beacon packet:
	{"event": "beacon", "ip": "10.0.1.85", "port": 80, "device": "foobar123"}

### [send()](id:section-reference-send)

Send character data from your application to device connected via the wifi module. 

**Arguments:**

* data
	* char * or string literal identifing the data to send

**Returns:**

Boolean. True = success - beacon enabled. False = failure.

**Example:**

	boot ret = myWifi.send("{timestamp: '3:14:18 9-NOV-2014', temperature: '14'}");


### [run()](id:section-reference-run)	

Causes the library to process incoming messages. This ***must*** be called from the loop() function of your application. If you don't you'll never receive any incoming data.

**Arguments:**

* none

**Returns:**

nothing

**Example:**

	loop() {
		wifi.run();
	}

### [ip()](id:section-reference-ip)	

Returns the current IP address. Only valid after connection is established.

**Arguments:**

* none

**Returns:**

char * referencing a buffer containing the IP address.

**Example:**

	// print the IP address 
	Serial2.println(wifi.ip());
	
## [Limitations](id:section-limitations)


The beacon feature presumes the broadcast address for the network consists of the initial three octets of the assigned IP address plus "255". This is obviously not going to work in cases where the network is not using that address for broadcast purposes.

No support for DNS. In client mode you can only use IPs, not addresses. (Anyone have a VERY tiny getHostByName we can toss in here?)

I'm not sure how the module deals with "glare" - incoming and outgoing data at the same time. Perhaps it internally locks / buffers when sending? I've been able to trip it up by sending from both the serial and wifi sides at the same time.

## [License](id:section-license)

Copyright (c) 2014 Steven Sokol

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.