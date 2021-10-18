## SimpleNB

This is an Arduino library for communicating with NB-IoT/CAT-M1 modules that using AT command interface, such as such as SIMCOM SIM70XX series, Quectel BG96, uBlox SARA-R410M.

This library is based on the works of [TinyGSM](https://github.com/vshymanskyy/TinyGSM) which try to be all-things-AT-commands. What make this library different from the TinyGSM is that this library stripped out the supports of all the legacy GSM modules and unrelated WiFi modules, focus only on NB-IoT/CAT-M1 modules to make the library easier to setup, and add new new features and functionalities related to NB-IoT/CAT-M1 technologies to the library.

If you like **SimpleNB** - give it a star, or fork it and contribute!

- [Supported modules](#supported-modules)
- [Features](#features)
- [Getting Started](#getting-started)
  - [First Steps](#first-steps)
  - [Writing your own code](#writing-your-own-code)
  - [If you have any issues](#if-you-have-any-issues)
- [How does it work?](#how-does-it-work)
- [API Reference](#api-reference)
- [Troubleshooting](#troubleshooting)
  - [Adequate power source](#adequte-power-source)
  - [Ensure correct Serial connection](#ensure-correct-serial-connection)
  - [Baud rates](#baud-rates)
    - [SoftwareSerial problems](#softwareserial-problems)
    - [HardwareSerial](#hardwareserial)
  - [Factory Reset](#factory-reset)
  - [APN Configuration](#apn-configuration)
  - [Failed connection or no data received](#failed-connection-or-no-data-received)
  - [Diagnostics sketch](#diagnostics-sketch)
  - [HTTP implementation problems - "but it works with PostMan"](#http-implementation-problems---but-it-works-with-postman)
  - [Which version of the SIM7000 code to use](#which-version-of-the-sim7000-code-to-use)
- [License](#license)

## Supported modules

- SIMCom SIM7000X CAT-M1/NB-IoT Module
- SIMCom SIM7020X CAT-M1/NB-IoT Module
- SIMCom SIM7070/SIM7080/SIM7090 CAT-M1/NB-IoT Module
- u-blox LTE-M/NB-IoT Modems (SARA-R4xx, SARA-N4xx, _but NOT SARA-N2xx_)
- Sequans Monarch LTE Cat M1/NB1 (VZM20Q)
- Quectel BG96

With the exception of SIM7070/7080/7090 and SIM7020, the rest of supported modules are inherited from TinyGSM without further test.

## Supported Features

Some of the modules does not support some of the features due to hardware design (e.g. GNSS receiver), some functionality are available but not implemtented (those marked as `TBI`), here is a summary of features and functionalities supported by each modules:

| Functionality  | BG96 | Sara 4 | uBlox | Sequans | SIM7000 | SIM7020 | SIM70x0 | Xbee |
|----------------|:----:|:------:|:-----:|:-------:|:-------:|:-------:|:-------:|:----:|
| TCP            |   x  |    x   |   x   |    x    |    x    |    x    |    x    |   x  |
| SSL            |  TBI |    x   |   x   |    x    |    x    |   TBI   |    x    |   x  |
| GNSS           |   x  |    x   |   x   |         |    x    |         |    x    |      |
| GSM LBS        |      |    x   |   x   |         |    x    |         |    x    |      |
| SMS            |   x  |    x   |   x   |    x    |    x    |    x    |    x    |   x  |
| Voice Call     |   x  |        |   x   |    x    |         |         |         |      |
| Network Time   |   x  |    x   |   x   |    x    |    x    |    x    |    x    |      |
| NTP            |   x  |        |       |         |    x    |    x    |    x    |      |
| Battery Status |   x  |    x   |   x   |         |    x    |    x    |    x    |   x  |
| Temperature    |   x  |    x   |       |    x    |         |         |         |   x  |
| GPRS           |   x  |    x   |   x   |    x    |    x    |    x    |    x    |   x  |

**Data connections**
- TCP (HTTP, MQTT, Blynk, ...)
    - ALL modules support TCP connections
    - Most modules support multiple simultaneous connections, however please noted that some network only allows less or even one connection, so please check with your network operator for number of network connections it supported:
        - Quectel BG96 - 12
        - Sequans Monarch - 6
        - SIM7000 - 8 possible without SSL, only 2 with SSL
        - SIM7020 - Currently only 1 supported
        - SIM 7070/7080/7090 - 12
        - u-blox SARA R4/N4 - 7
        - Digi XBee - _only 1 connection supported!_
- SSL/TLS
    - Supported on:
        - SIM7000, u-Blox, and Sequans Monarch
    - Not yet supported on:
        - Quectel, SIM7020
    - Like TCP, most modules support simultaneous connections
    - TCP and SSL connections can usually be mixed up to the total number of possible connections

**SMS**
- Only _sending_ SMS is supported, not receiving
    - Supported on all modules

**Voice Calls**
- Supported on:
    - Quectel modems, u-blox
- Not yet supported on:
    - SIM7000
- Not possible on:
    -  u-blox SARA R4/N4
- Functions:
    - Dial, hangup
    - DTMF sending

**Location**
- GPS/GNSS
    - SIM7000, SIM7070/SIM7080/SIM7090, BG96, u-blox
    - NOTE:  u-blox chips do _NOT_ have embedded GPS - this functionality only works if a secondary GPS is connected to primary cellular chip over I2C
- GSM LBS location service
    - SIM7000, SIM7070/SIM7080/SIM7090, u-blox

## Credits

- TinyGSM Primary Authors/Contributors:
    - [vshymanskyy](https://github.com/vshymanskyy)
    - [SRGDamia1](https://github.com/SRGDamia1/)
- SIM7000:
    - [captFuture](https://github.com/captFuture/)
    - [FStefanni](https://github.com/FStefanni/)
- Sequans Monarch:
    - [nootropicdesign](https://github.com/nootropicdesign/)

## Getting Started

### First Steps

  1. Check your balance if you had an prepaid plan. Ensure that the PIN code on the SIM card is disable, the APN, Username and Password are correctly provisioned, contact your local carier if you are unsure.
  2. Ensure the SIM card is correctly inserted into the NB-IOT/CAT-M1 board that you used.
  3. Ensure that antenna is firmly attached.
  4. Ensure that you have a stable power supply to the module of at least **500mA**.
  5. Check if serial connection is working (Hardware Serial is recommended). Use [AT Debug](tools/AT_Debug/AT_Debug.ino) tool to check if the module can response to an `AT` command.
  6. Try out the examples included in this Library.

### Writing your own code

Prior using the library, there is some boiler-plat configuration required. The general flow of your code should be:
- Define the module that you are using (choose one and only one)
    - ie, `#define SIMPLE_NB_MODEM_SIM7080`
- Included SimpleNB library
    - `#include <SimpleNBClient.h>`
- Create a SimpleNB modem instance, and pass-in the Serial Communication port for AT-Command interface as parameter
    - `SimpleNB modem(SerialAT);`
- Create one or more SimpleNB client instances
    - For a single connection, use
        - `SimpleNBClient client(modem);`
        or
        `SimpleNBClientSecure client(modem);` (on supported modules)
    - For multiple connections (on supported modules) use:
        - `SimpleNBClient clientX(modem, 0);`, `SimpleNBClient clientY(modem, 1);`, etc
          or
        - `SimpleNBClientSecure clientX(modem, 0);`, `SimpleNBClientSecure clientY(modem, 1);`, etc
- Begin your serial communication and set all your pins as required to power your module and bring it to full functionality.
    - The examples attempt to guess the module's baud rate.  In working code, consider use a set baud.
- Initialize the modem
    - `modem.init()` or
    - `modem.restart()`, restart generally takes much longer than init but ensures the module doesn't have lingering connections
- Unlock your SIM, if necessary (optional):
    - `modem.simUnlock(GSM_PIN)`
- Wait for network registration to be successful
    - `modem.waitForRegistration(600000L)`
- Check the Signal Quality to make sure it has reached to a stable stage with
    - `modem.getSignalQuality()`
- To establish a data connection (GPRS or EPS), If using cellular, establish the GPRS or EPS data connection _after_ your are successfully registered on the network
    - `modem.gprsConnect(apn, gprsUser, gprsPass)` (or simply `modem.gprsConnect(apn)`). The same command is used for both GPRS or EPS connection
    - For SIM7070/7080/7090, use the new API `modem.activateDataNetwork()` to establish a data connection
    - If using a **Digi** brand cellular XBee, you must specify your GPRS/EPS connection information _before_ waiting for the network.  This is true ONLY for _Digi cellular XBees_!
- Connect the TCP or SSL client
    `client.connect(server, port)`
- Send out your data as TCP packet, or compose your HTTP request as per HTTP protocol.

### If you have any issues

  1. Read the whole README (you're looking at it!), particularly the troubleshooting section below.
  2. Take a look at the example similar to your application.
  2. Try running the Diagnostics sketch

## How does it work?

SimpleNB behind the scene wrap AT commands and parse the feedback fom the AT Commands to get the important parameters, and provide an easy-to-use API. In most of the case, the syntax of the APIs are resemble of typical Arduino Client interface.

This library is "blocking" in all of its communication. Depending on the function, your code may be blocked for a long time waiting for the module responses. Some functions (e.g. `modem.waitForRegistration()`, `modem.retart()` or `modem.getGPS()`) may block your code for several seconds or even 1 to 2 minute, others may need you to loop through until the return value confirms a value indicating a success (e.g. `modem.checkSignalQuality()`).

This libary *does not* support any sort of "hardware" or pin level controls for the modules.
If you need to turn your module on or reset it using some sort of High/Low/High pin sequence, you must write those functions yourself.

## API Reference

For TCP data streams, this library follows the standard [Arduino Client](https://www.arduino.cc/en/Reference/ClientConstructor) API interface. The [AllFunctions](examples/AllFunctions/AllFunctions.ino) example provides a glance of almost all the function APIs available in the library.

## Troubleshooting

### Adequate power source

Most modules require _**as much as 500mA**_ to properly connect to the network, many of the Arduino boards may not able to power those NB-IoT/CAT-M1 modules. Improving the power supply actually solves stability problems in **many** cases!

### Ensure correct Serial connection
Make sure you connect the Serial connection correctly between the host MCU and the NB-IOT/CAT-M1 module
```
MCU      NB-IOT/CAT-M1
Tx  ---> Rx
Rx  ---> Tx
```

### Baud rates

Most modules support "auto-bauding" feature where the module will attempt to adjust it's baud rate to match what it is receiving. In most of the cases, if `HardwareSerial` is used for interfacing with the module, 115200bps would work perfectly, if `SoftwareSerial` is used, it is recommended not use data rate higher than 9600bps. In some cases, if you experiences missing data or receiving gabbage from the modules, there is an auto bauding function (`SimpleNBAutoBaud(SerialAT, ACK_AUTOBAUD_MIN, ACK_AUTOBAUD_MAX);`) which allows you to provide a range of baud rate to test the best suitable baud rate that can be used. While very useful when initially connecting to a module and doing tests, these should **NOT** be used in any sort of production code. Once you've established communication with the module, set the baud rate using the `setBaud(#)` function and stick with that baud rate.

#### SoftwareSerial problems

When using `SoftwareSerial` (on Uno, Nano, etc), the speed **115200** may not work.
Try selecting **57600**, **38400**, or even lower - the one that works best for you.
In some cases **9600** is unstable, but using **38400** helps, etc.
Be sure to set correct TX/RX pins in the sketch. Please note that not every Arduino pin can serve as TX or RX pin.
**Read more about SoftSerial options and configuration [here](https://www.pjrc.com/teensy/td_libs_AltSoftSerial.html) and [here](https://www.arduino.cc/en/Reference/SoftwareSerial).**

#### HardwareSerial and SAMD21-based boards

When using `HardwareSerial`, you may need to specify additional parameters to the `.begin()` call depend on your MCU's Arduino Core implementation. When using SAMD21-based boards, you may need to use a sercom uart port instead of `Serial1`. Check your MCU's Arduino implementation documentation.

### Factory Reset

Sometimes (especially if you played with AT commands), your module configuration may become invalid. This may result in problems such as:

 * Can't connect to the network
 * Can't connect to the server
 * Sent/received data contains invalid bytes
 * etc.

To return module to **Factory Defaults**, use [FactoryReset](https://github.com/techstudio-design/SimpleNB/blob/master/tools/FactoryReset/FactoryReset.ino) tool to reset it.

### APN Configuration
In rare cases (for the legacy network (i.e. 2G/3G)), you may need to set an initial APN to connect to the mobile network.
Try using the `gprsConnect(APN)` function to set an initial APN if you are unable to register on the network. You may need set the APN again after registering (In most cases, you should set the APN after registration).

### Failed connection or no data received

The first connection with a new SIM card, a new module, or at a new location/tower may take a *LONG* time, especially if the signal quality isn't excellent. If it is your first connection, you may need to adjust your wait times (timeout) timing.

If you are able to open a TCP connection but have the connection close before receiving data, try adding a keep-alive header to your request. Some modules (ie, the SIM7000 in SSL mode) will immediately throw away any un-read data when the remote server closes the connection - sometimes without even giving a notification that data arrived in the first place.

When using MQTT, to keep a continuous connection you may need set the correct keep-alive interval (PINGREQ/PINGRESP).

### Diagnostics sketch

Use [Diagnostics](https://github.com/techstudio-design/SimpleNB/blob/master/tools/Diagnostics/Diagnostics.ino) tool to help diagnose SIM Card and Connection issues.

If the diagnostics fail, uncomment this line (or add this line) to output some debugging comments from the library:
```cpp
#define SIMPLE_NB_DEBUG Serial
```

If you are unable to see any obvious errors in the library debugging, use [StreamDebugger](https://github.com/vshymanskyy/StreamDebugger) to copy the entire AT command sequence to the main serial port.
In the diagnostics example, simply uncomment the line:
```cpp
#define DUMP_AT_COMMANDS
```
or add this snippit to your code:
```cpp
#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, Serial);
  SimpleNB modem(debugger);
#else
  SimpleNB modem(SerialAT);
#endif
```

### HTTP implementation problems - "but it works with PostMan"

This library opens a TCP (or SSL) connection to a server. In the [OSI model](https://en.wikipedia.org/wiki/OSI_model), that's [layer 4](http://www.tcpipguide.com/free/t_TransportLayerLayer4.htm) (or 5 for SSL). HTTP (GET/POST), MQTT, and most of the other functions you probably want to use are implemented at [layer 7](http://www.tcpipguide.com/free/t_ApplicationLayerLayer7.htm). This means that you need to either manually code the top layer or use another library (like [HTTPClient](https://github.com/arduino-libraries/ArduinoHttpClient) or [PubSubClient](https://pubsubclient.knolleary.net/)) to do it for you.

Tools like [PostMan](https://www.postman.com/) implemented the HTTP/HTTPS and communicate with servers at layer 7. If you are able to successfully send a request to your server using Postman, it means that your server works correctly, and the problem is with your client implementation.

If you are successfully connecting to a server, but getting responses of "bad request" (or no response), the issue is probably your HTTP formatting (such as missing mandated HTTP header(s), incorrect HTTP protocol (HTTP 1.0 versus HTTP1.1), or incorrect HTTP body). Here are some tips for implementing your own HTTP request manually:
- Look at the examples related to HTTP implmentation
- Make sure you are including all required headers
- Instead of sednging data to your server, send it to the service like `httpbin.org` to test your implementation, as it will echo back what you sent so that you could see what's going on
- Use `client.print("...")`, or `client.write(buf, #)`, or even `client.write(String("..."))`, not `client.write("...")` to help prevent text being sent out one character at a time (typewriter style)
- Make sure there is one blank line that separates the HTTP headers from HTTP body (i.e. the content of any POST request or HTTP form data) as part of HTTP requirement, and this is really easy to miss. Add two lines to the last header `client.print("....\r\n\r\n")` or put in an extra `client.println()`


### Which version of the SIM7000 code to use

There are two versions of the SIM7000 code, one using `SIMPLE_NB_MODEM_SIM7000` and another with `SIMPLE_NB_MODEM_SIM7000SSL`. The `SIMPLE_NB_MODEM_SIM7000` version *does not support SSL* but supports up to 8 simultaneous connections. The `SIMPLE_NB_MODEM_SIM7000SSL` version supports both SSL *and unsecured connections* with up to 2 simultaneous connections.

So why are there two versions?

The "SSL" version uses the SIM7000's "application" commands while the other uses the "TCP-IP toolkit". Depending on your region/firmware, one or the other may not work for you. Try both and use whichever is more stable. If you do not need SSL, I recommend starting with `SIMPLE_NB_MODEM_SIM7000`.

## License
This project is released under The GNU Lesser General Public License (LGPL-3.0)
