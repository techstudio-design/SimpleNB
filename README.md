## SimpleNB

This is an Arduino library for communicating with NB-IoT/CAT-M1 modules that using AT command interface, such as such as SIMCOM SIM70XX series, Quectel BG96, uBlox SARA-R410M.

This library is based on the works of [TinyGSM](https://github.com/vshymanskyy/TinyGSM) which try to be all-things-AT-commands. What make this library different from the TinyGSM is that this library stripped out the supports of all the legacy GSM modules and unrelated WiFi modules and focus only on NB-IoT/CAT-M1 modules, to make the library easier to setup and add new new features and functionalities related to NB-IoT/CAT-M1 technologies to the library.

> ** This library is a working-in-progress project and has not reach to a mature stage yet, therefore do not download as it may not work at this stage. Check back in the next a couple of weeks, we will remove this paragraph when it is ready for use.**

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
  - [Ensure stable data & power connection](#ensure-stable-data--power-connection)
  - [Baud rates](#baud-rates)
  - [Broken initial configuration](#broken-initial-configuration)
  - [Failed connection or no data received](#failed-connection-or-no-data-received)
  - [Diagnostics sketch](#diagnostics-sketch)
  - [Web request formatting problems - "but it works with PostMan"](#web-request-formatting-problems---but-it-works-with-postman)
  - [SoftwareSerial problems](#softwareserial-problems)
  - [ESP32 Notes](#esp32-notes)
    - [HardwareSerial](#hardwareserial)
    - [HttpClient](#httpclient)
  - [Which version of the SIM7000 code to use](#which-version-of-the-sim7000-code-to-use)
- [License](#license)

## Supported modules

- SIMCom SIM7000X CAT-M1/NB-IoT Module
- SIMCom SIM7020X CAT-M1/NB-IoT Module
- SIMCom SIM7070/SIM7080/SIM7090 CAT-M1/NB-IoT Module
- u-blox LTE-M/NB-IoT Modems (SARA-R4xx, SARA-N4xx, _but NOT SARA-N2xx_)
- Sequans Monarch LTE Cat M1/NB1 (VZM20Q)
- Quectel BG96

## Features

**Data connections**
- TCP (HTTP, MQTT, Blynk, ...)
    - ALL modules support TCP connections
    - Most modules support multiple simultaneous connections:
        - Quectel BG96 - 12
        - Sequans Monarch - 6
        - SIM7000 - 8 possible without SSL, only 2 with
        - SIM 7070/7080/7090 - 12
        - u-blox SARA R4/N4 - 7
        - Digi XBee - _only 1 connection supported!_
- UDP
    - Not yet supported on any module, though it may be some day
- SSL/TLS (HTTPS)
    - Supported on:
        - SIM7000, u-Blox, and Sequans Monarch
    - Not yet supported on:
        - Quectel modems
    - Like TCP, most modules support simultaneous connections
    - TCP and SSL connections can usually be mixed up to the total number of possible connections

**SMS**
- Only _sending_ SMS is supported, not receiving
    - Supported on all cellular modules

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
    - SIM7000, BG96, u-blox
    - NOTE:  u-blox chips do _NOT_ have embedded GPS - this functionality only works if a secondary GPS is connected to primary cellular chip over I2C
- GSM LBS location service
    - SIM7000, Quectel, u-blox

**Credits**
- TinyGSM Primary Authors/Contributors:
    - [vshymanskyy](https://github.com/vshymanskyy)
    - [SRGDamia1](https://github.com/SRGDamia1/)
- SIM7000:
    - [captFuture](https://github.com/captFuture/)
    - [FStefanni](https://github.com/FStefanni/)
- Sequans Monarch:
    - [nootropicdesign](https://github.com/nootropicdesign/)
- Other Contributors:
    - https://github.com/techstudio-design/SimpleNB/graphs/contributors

## Getting Started

#### First Steps

  1. Using your phone:
    - Disable PIN code on the SIM card
    - Check your balance
    - Check that APN, User, Pass are correct and you have internet
  2. Ensure the SIM card is correctly inserted into the module
  3. Ensure that GSM antenna is firmly attached
  4. Ensure that you have a stable power supply to the module of at least **500mA**.
  5. Check if serial connection is working (Hardware Serial is recommended)
     Send an `AT` command using [this sketch](tools/AT_Debug/AT_Debug.ino)
  6. Try out the [WebClient](https://github.com/techstudio-design/SimpleNB/blob/master/examples/WebClient/WebClient.ino) example

#### Writing your own code

The general flow of your code should be:
- Define the module that you are using (choose one and only one)
    - ie, `#define TINY_GSM_MODEM_SIM800`
- Included TinyGSM
    - `#include <SimpleNBClient.h>`
- Create a TinyGSM modem instance
    - `SimpleNB modem(SerialAT);`
- Create one or more TinyGSM client instances
    - For a single connection, use
        - `SimpleNBClient client(modem);`
        or
        `SimpleNBClientSecure client(modem);` (on supported modules)
    - For multiple connections (on supported modules) use:
        - `SimpleNBClient clientX(modem, 0);`, `SimpleNBClient clientY(modem, 1);`, etc
          or
        - `SimpleNBClientSecure clientX(modem, 0);`, `SimpleNBClientSecure clientY(modem, 1);`, etc
    - Secure and insecure clients can usually be mixed when using multiple connections.
    - The total number of connections possible varies by module
- Begin your serial communication and set all your pins as required to power your module and bring it to full functionality.
    - The examples attempt to guess the module's baud rate.  In working code, you should use a set baud.
- Wait for the module to be ready (could be as much as 6s, depending on the module)
- Initialize the modem
    - `modem.init()` or `modem.restart()`
    - restart generally takes longer than init but ensures the module doesn't have lingering connections
- Unlock your SIM, if necessary:
    - `modem.simUnlock(GSM_PIN)`
- If using **WiFi**, specify your SSID information:
    - `modem.networkConnect(wifiSSID, wifiPass)`
    - Network registration should be automatic on cellular modules
- Wait for network registration to be successful
    - `modem.waitForNetwork(600000L)`
- If using cellular, establish the GPRS or EPS data connection _after_ your are successfully registered on the network
    - `modem.gprsConnect(apn, gprsUser, gprsPass)` (or simply `modem.gprsConnect(apn)`)
    - The same command is used for both GPRS or EPS connection
    - If using a **Digi** brand cellular XBee, you must specify your GPRS/EPS connection information _before_ waiting for the network.  This is true ONLY for _Digi cellular XBees_!  _For all other cellular modules, use the GPRS connect function after network registration._
- Connect the TCP or SSL client
    `client.connect(server, port)`
- Send out your data.

#### If you have any issues

  1. Read the whole README (you're looking at it!), particularly the troubleshooting section below.
  2. Try running the Diagnostics sketch

## How does it work?

Many GSM modems, WiFi and radio modules can be controlled by sending AT commands over Serial.
TinyGSM knows which commands to send, and how to handle AT responses, and wraps that into standard Arduino Client interface.

This library is "blocking" in all of its communication.
Depending on the function, your code may be blocked for a long time waiting for the module responses.
Apart from the obvious (ie, `waitForNetwork()`) several other functions may block your code for up to several *minutes*.
The `gprsConnect()` and `client.connect()` functions commonly block the longest, especially in poorer service regions.
The module shutdown and restart may also be quite slow.

This libary *does not* support any sort of "hardware" or pin level controls for the modules.
If you need to turn your module on or reset it using some sort of High/Low/High pin sequence, you must write those functions yourself.

## API Reference

For GPRS data streams, this library provides the standard [Arduino Client](https://www.arduino.cc/en/Reference/ClientConstructor) interface.
For additional functions, please refer to [this example sketch](examples/AllFunctions/AllFunctions.ino)

## Troubleshooting

### Ensure stable data & power connection

Most modules require _**as much as 500mA**_ to properly connect to the network, many of the Arduino boards may not able to power those NB-IoT/CAT-M1 modules.
Improving the power supply actually solves stability problems in **many** cases!
- Keep your wires as short as possible
- Consider soldering them for a stable connection
- Do not put your wires next to noisy signal sources (buck converters, antennas, oscillators etc.)
- If everything else seems to be working but you are unable to connect to the network, check your power supply!
- Make sure you connect the Serial connection correctly between the host MCU and the WiFi module (i.e. MCU Tx -> WiFi Rx, MCU Rx -> WiFi Tx).

### Baud rates

Most modules support some sort of "auto-bauding" feature where the module will attempt to adjust it's baud rate to match what it is receiving.
SimpleNB also implements its own auto bauding function (`SimpleNBAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX);`).
While very useful when initially connecting to a module and doing tests, these should **NOT** be used in any sort of production code.
Once you've established communication with the module, set the baud rate using the `setBaud(#)` function and stick with that rate.

### Broken initial configuration

Sometimes (especially if you played with AT commands), your module configuration may become invalid.
This may result in problems such as:

 * Can't connect to the network
 * Can't connect to the server
 * Sent/received data contains invalid bytes
 * etc.

To return module to **Factory Defaults**, use this sketch:
  File -> Examples -> TinyGSM -> tools -> [FactoryReset](https://github.com/techstudio-design/SimpleNB/blob/master/tools/FactoryReset/FactoryReset.ino)

In some cases, you may need to set an initial APN to connect to the mobile network.
Try using the `gprsConnect(APN)` function to set an initial APN if you are unable to register on the network.
You may need set the APN again after registering.
(In most cases, you should set the APN after registration.)

### Failed connection or no data received

The first connection with a new SIM card, a new module, or at a new location/tower may take a *LONG* time - up to 15 minutes or even more, especially if the signal quality isn't excellent.
If it is your first connection, you may need to adjust your wait times and possibly go to lunch while you're waiting.

If you are able to open a TCP connection but have the connection close before receiving data, try adding a keep-alive header to your request.
Some modules (ie, the SIM7000 in SSL mode) will immediately throw away any un-read data when the remote server closes the connection - sometimes without even giving a notification that data arrived in the first place.
When using MQTT, to keep a continuous connection you may need to reduce your keep-alive interval (PINGREQ/PINGRESP).

### Diagnostics sketch

Use this sketch to help diagnose SIM card and GPRS connection issues:
  File -> Examples -> TinyGSM -> tools -> [Diagnostics](https://github.com/techstudio-design/SimpleNB/blob/master/tools/Diagnostics/Diagnostics.ino)

If the diagnostics fail, uncomment this line to output some debugging comments from the library:
```cpp
#define TINY_GSM_DEBUG SerialMon
```
In any custom code, `TINY_GSM_DEBUG` must be defined before including the TinyGSM library.

If you are unable to see any obvious errors in the library debugging, use [StreamDebugger](https://github.com/vshymanskyy/StreamDebugger) to copy the entire AT command sequence to the main serial port.
In the diagnostics example, simply uncomment the line:
```cpp
#define DUMP_AT_COMMANDS
```
In custom code, you can add this snippit:
```cpp
#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, SerialMon);
  SimpleNB modem(debugger);
#else
  SimpleNB modem(SerialAT);
#endif
```

### Web request formatting problems - "but it works with PostMan"

This library opens a TCP (or SSL) connection to a server.
In the [OSI model](https://en.wikipedia.org/wiki/OSI_model), that's [layer 4](http://www.tcpipguide.com/free/t_TransportLayerLayer4.htm) (or 5 for SSL).
HTTP (GET/POST), MQTT, and most of the other functions you probably want to use live up at [layer 7](http://www.tcpipguide.com/free/t_ApplicationLayerLayer7.htm).
This means that you need to either manually code the top layer or use another library (like [HTTPClient](https://github.com/arduino-libraries/ArduinoHttpClient) or [PubSubClient](https://pubsubclient.knolleary.net/)) to do it for you.
Tools like [PostMan](https://www.postman.com/) also show layer 7, not layer 4/5 like TinyGSM.
If you are successfully connecting to a server, but getting responses of "bad request" (or no response), the issue is probably your formatting.
Here are some tips for writing layer 7 (particularly HTTP request) manually:
- Look at the "WebClient" example
- Make sure you are including all required headers.
    - If you are testing with PostMan, make sure you un-hide and look at the "auto-generated" headers; you'll probably be surprised by how many of them there are.
- Use `client.print("...")`, or `client.write(buf, #)`, or even `client.write(String("..."))`, not `client.write("...")` to help prevent text being sent out one character at a time (typewriter style)
- Enclose the entirety of each header or line within a single string or print statement
    - use
    ```cpp
    client.print(String("GET ") + resource + " HTTP/1.1\r\n");
    ```
    instead of
    ```cpp
    client.print("GET ");
    client.print(resource);
    client.println(" HTTP/1.1")
    ```
- Make sure there is one entirely blank line between the last header and the content of any POST request.
    - Add two lines to the last header `client.print("....\r\n\r\n")` or put in an extra `client.println()`
    - This is an HTTP requirement and is really easy to miss.

### SoftwareSerial problems

When using `SoftwareSerial` (on Uno, Nano, etc), the speed **115200** may not work.
Try selecting **57600**, **38400**, or even lower - the one that works best for you.
In some cases **9600** is unstable, but using **38400** helps, etc.
Be sure to set correct TX/RX pins in the sketch. Please note that not every Arduino pin can serve as TX or RX pin.
**Read more about SoftSerial options and configuration [here](https://www.pjrc.com/teensy/td_libs_AltSoftSerial.html) and [here](https://www.arduino.cc/en/Reference/SoftwareSerial).**

### ESP32 Notes

#### HardwareSerial and SAMD21-based boards

When using `HardwareSerial`, you may need to specify additional parameters to the `.begin()` call depend on your MCU's Arduino Core implementation. When using SAMD21-based boards, you may need to use a sercom uart port instead of `Serial1`. Check your MCU's Arduino implementation documentation.

#### HttpClient
You will not be able to compile the HttpClient or HttpsClient examples with ESP32 core 1.0.2.  Upgrade to 1.0.3, downgrade to version 1.0.1 or use the WebClient example.

### Which version of the SIM7000 code to use

There are two versions of the SIM7000 code, one using `TINY_GSM_MODEM_SIM7000` and another with `TINY_GSM_MODEM_SIM7000SSL`.
The `TINY_GSM_MODEM_SIM7000` version *does not support SSL* but supports up to 8 simultaneous connections.
The `TINY_GSM_MODEM_SIM7000SSL` version supports both SSL *and unsecured connections* with up to 2 simultaneous connections.
So why are there two versions?
The "SSL" version uses the SIM7000's "application" commands while the other uses the "TCP-IP toolkit".
Depending on your region/firmware, one or the other may not work for you.
Try both and use whichever is more stable.
If you do not need SSL, I recommend starting with `TINY_GSM_MODEM_SIM7000`.

__________

## License
This project is released under
The GNU Lesser General Public License (LGPL-3.0)
