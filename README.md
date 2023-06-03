
[![MIT License][license-shield]][license-url] [![platform][platform-shield]][license-url]


<br />
<div align="center">
  <a href="https://github.com/cloasdata/modernbus/">
    <img src=".logo/cover.png" alt="Logo" height="150">
  </a>

<h3 align="center">mod(ern)bus</h3>

  <p align="center">
    modern c++ modbus library
    <br />
    <a href="https://github.com/cloasdata/modernbus"><strong>Explore the docs »</strong></a>
    <br />
    <br />
    <a href="https://github.com/cloasdata/modernbus/example">View Demo</a>
    ·
    <a href="https://github.com/cloasdata/modernbus/issues">Report Bug</a>
    ·
    <a href="https://github.com/cloasdata/modernbus/issues">Request Feature</a>
  </p>
</div>

<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
      <ul>
        <li><a href="#status">Motivation</a></li>
        <li><a href="#motivation">Motivation</a></li>
        <li><a href="#features">Features</a></li>
        <li><a href="#how-it-works">How It Works</a></li>
        <li><a href="#should-i-use-it">Should I Use It</a></li>
        <li><a href="#is-modbus-deprecated">Is Modbus Deprecated</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li>
        <a href="#usage">Usage</a>
        <ul>
            <li><a href="#server-slave">Server Slave</a></li>
            <li><a href="#master-client">Master Client</a></li>
        </ul>
    </li>
    <li><a href="#roadmap">Roadmap</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
  </ol>
</details>

## About The Project
**mod(ern)bus** is a modern C++ library providing asynchronous modbus client (master) and modbus server (slave) implementation on all kind of hardware communication such as RS232, RS485, and others.

## Status
At the moment I am actively developing this library for a semi industrial application. Depending on the needs there I may add additional feature or even change the API.

## Motiviation
There are indeed plenty of good and not so good modbus implementations out for embedded system and especially for the arduino header. Most of them limit themselves to one major aspect of modbus standard. For example one may only provide a serial modbus master, so the programmer always have to deal with different dependencies and different APIs if he needs to implement different devices/communication channels on one project.

Also I found most of library's are running synchronous and making the code slow. In many cases they wait (i. e. delay) until something happens or need to happen on the hardware interfaces. 
This may be okay for slower AVRs but it is completely odd when running fast ARMs or other high speed architectures.
Beneath these elementary problems, I also don't like how the main dependency of a modbus implementation is handled. For different hardware interfaces may exist different client implementation. This will not only add compile time but may also increase memory footprints or performance (when using vtable).

## Features
* Unified API for all Clients/Servers on different types of streams
* Non blocking (asynchronous) implementation using [Taskscheduler package](https://github.com/arkhipenko/TaskScheduler) (may be dropped in future for lighter footprint)
* Unified StreamProvider interface. Making user available to implement any kind of stream.
* The callback (handler) architecture is injecting the user code. So the user code is quite small, not cluttered and feels like the interface of typical web server API.
* Small footprint due to the power off C++ class templates.
* Does not have a strict binding from a particular request to particular data.
* User always can response with a exception.
* User also can response in any other context they want/need.
* Unit tested
* Integration test of your implementation is quiet simple. Just use the crosslink provider and you can connect slave/master on one machine.

## How it works
Clients are sending requests and handling responses from slaves. Server are handling requests from clients and sending responses.
All messages or frames are parsed by [mbparser](https://github.com/cloasdata/mbparser) library in the core of the client/server implementation
On a modbus network always only one master or slave is sending data. The modbus frame always starts with the slave address. So each slave can check if the frame is for him or can drop the frame immediately to not longer occupy resources.
Whenever there is a frame complete (crc check was good), then a user handler is called.
Server then typical response with stuff, when the task was done or the data is ready. Note that clients always waiting for this response and may timeout if it takes too long.
Client receiving the response is getting acknowledge in form of a echo frame or getting data as requested. But masters also can send larger data frames if required.
Here is a [website](https://www.modbustools.com/modbus.html) with more details and good examples.

Here is a modbus server (slave) in only 6 lines of code:
```c++
// A minimal modbus server delivering 20 floats on fc 04 from address 00 on HardwareSerial
#include <Arduino.h>
#include <HardwareSerial.h>
#include <TaskSchedulerDeclarations.h>
#include <modernbus_server.h>
#include <modernbus_provider.h>

Scheduler scheduler{};
SerialProvider<HardwareSerial> provider{Serial};
ModbusServer<SerialProvider<HardwareSerial>> server{&scheduler, &provider, 0x01};

uint8_t dataMap[]
    {
        0x0, 0x0, 0x80, 0x3F, 0x0, 0x0, 0x0, 0x40, 0x0, 0x0, 0x40, 0x40,
        0x0, 0x0, 0x80, 0x40, 0x0, 0x0, 0xA0, 0x40, 0x0, 0x0, 0xC0, 0x40, 
        0x0, 0x0, 0xE0, 0x40, 0x0, 0x0, 0x0, 0x41, 0x0, 0x0, 0x10, 0x41, 
        0x0, 0x0, 0x30, 0x41, 0x0, 0x0, 0x40, 0x41, 0x0, 0x0, 0x50, 0x41, 
        0x0, 0x0, 0x60, 0x41, 0x0, 0x0, 0x70, 0x41, 0x0, 0x0, 0x80, 0x41, 
        0x0, 0x0, 0x88, 0x41, 0x0, 0x0, 0x90, 0x41, 0x0, 0x0, 0x98, 0x41, 
        0x0, 0x0, 0xA0, 0x41, 0x0, 0x0, 0xB0, 0x41
    };

void setup(){
    Serial.begin(9600);
    server.responseTo(0x04, 0x0000).with(dataMap, 80, 4);
    server.start();
}

void loop(){
    scheduler.execute();
}
```

## Should I use it
I cannot tell you exactly if it is a good idea to use this package. 
But what I can say from my own experience is that modernbus may not be the choice when
* You have only limited communication to do. For example need to poll or response one single request. Then I would recommend to write you own implementation
* You you are not into lambdas and in general with callback stuff. I would recommend to learn more about C++11 feature before you use modernbus. However, you may find this as a good learning project.
* At this stage I would not recommend to use the package for any critical task.
* If you need more detailed debug information then I would recommend to use mbparser directly or again try to write you own implementation.


## Is modbus deprecated
Yes, indeed modbus is very old und lacks a lot of modern features in comparison to CAN bus for example.

But at the end modbus is very simple and easy to understand. This is one very major advantage when you are only dealing every here and then with a bus topology. So you do not have to learn a lot of stuff before hunting bugs on modbus networks.
This is even better when you are not a domain specific guy. For example you need commissioning some PLC -> valve communication and you are just the mechanical guy.

Basic implementation (not like this framework) may be written in less than 10 loc for a server responding to one function code. As features can be stripped completely modbus runs therefore also on very tiny machines.

If you are still in doubt of choosing modbus for you application, then there are many websites out comparing in detail other bus communication protocol's. 

<p align="right">(<a href="#readme-top">back to top</a>)</p>

## Getting Started
### Prerequisites
The library includes the arduino.h header. From this header it uses Serial and the typical arduino type alias like uint8_t.
To compile a c++ compiler supporting at least C++11 is required.
Features used:
Lambdas without capture
When flagged, full feature of functional lib
Generic type templates 

The preferred way to get this distribution is via platformio dependency manager.


### Installation
#### Github
```sh
git clone https://github.com/cloasdata/modernbus.git
```
#### Platformio
Add to platformio.ini or install via gui
```
    seimen/modernbus@0.3.2
```

Dependencies required (will be installed by platformio)
```
	arkhipenko/TaskScheduler@^3.7.0
	seimen/tinylinkedlist@1.2.0
	seimen/mbparser@0.1.1
```
<p align="right">(<a href="#readme-top">back to top</a>)</p>

## Usage

### Compiler Flags
Following flags are supported
```sh
-d STD_FUNCTIONAL
```
This will add functional library to modernbus. With functional included you than could use fully qualified lambdas with capture and pass methods as handlers.

More to come maybe.

### Server Slave
The server or slave provides ability to response to requests in the background. The user needs to register function code, address and a handler (provided as lambda or function).
The handler is called with a pointer to a response object.

The response object is already prefilled with data from the request.
Once you have validated the data, you can send payload, echo or exception. Also you may do you business logic within the callback.
#### Example
```c++
// A simple non blocking modbus slave
#include <Arduino.h>
#include <modernbus_server.h>
#include <modernbus_provider.h>

// first we do a typedef to shorthand the type expression
// For the example we want to run via Hardware Serial
using ProviderType = SerialProvider<HardwareSerial>;

// Machine will listen as slave 03
const uint8_t slaveAdress = 0x03;

// create some static global objects

// scheduler will drive the slave
Scheduler scheduler{};

// We wrap that hardware serial interface into a StreamProvider class
ProviderType provider{Serial};

// and finally create a server
ModbusServer<ProviderType> server{&scheduler, &provider, slaveAdress};

// global for once
union {
    float temperature;
    uint8_t byteCode[4];
} sensorT;
bool shouldUpdate{false};

void setup(){
    Serial.begin(9600);
    // add input coil. Writing the status bit as requested by the client
    server.responseTo(0x05, 0x00, [](ModbusResponse<ProviderType> *response){
       shouldUpdate=response->payload();
        response->sendEcho();
    });

    // add a holding register address 0
    server.responseTo(0x03, 0x00, [](ModbusResponse<ProviderType> *response){
        // we may only allow to read 1 register.
        if (response->quantity() == 1){
            response->send(sensorT.byteCode, 4);
        }
        else {
            // we always can do this instead of sending some payload
            response->sendException(3);
        }
    });
    server.start();
}

void loop(){
    scheduler.execute();
    if (shouldUpdate){
        sensorT.temperature = random(-10, 60);
    }
}
```
Now instead of using a response handler it is also possible to use the traditional mapping style of modbus.
```c++
server.responseTo(0x03, 0x00).with(PayloadArr, 10, 2);
```
But you may also combine both together. For example to prepare the state but leave it mapped.
```c++
server.responseTo(0x03, 0x00, [](ModbusResponse<ProviderType> *response){preparePayloadArr();}).with(PayloadArr, 10, 2);
```
This is always good if to accumulate the payload data is expensive. Than one just could send the most recent state without explicit manipulating it.


<p align="right">(<a href="#readme-top">back to top</a>)</p>

### Client Master
The client can either poll requests or send single request.
Once the a response was received the handler is called.
In the handler you could use the response to use it with you business logic. 

#### Example
```c++
#include <Arduino.h>
#include <modernbus_client.h>
#include <modernbus_provider.h>

union {
    float temperature;
    uint8_t byteCode[4];
} sensorT;

// first we do a typedef to shorthand the type expression
// For the example we want to run via Hardware Serial
using ProviderType = SerialProvider<HardwareSerial>;



// create some static global objects
Scheduler scheduler{};

// Now here we wrap that hardware serial interface into a StreamProvider class
ProviderType provider{Serial};

// create the client
ModbusClient<ProviderType> client{&scheduler, &provider};

// And this is the request we want to do
uint8_t ReadTemperature[] {0x01, 0x04, 0x01, 0x31, 0x0, 0x01E, 0x20, 0x31};

// Request Write Coil
uint8_t updateTempFrame[] {0x01, 0x05, 0x00, 0x00, 0xFF, 0xFF};
ModbusRequest updateTemperature{updateTempFrame, sizeof(updateTempFrame), false, 0x02, [](ServerResponse *response ){}};

void setup(){
    Serial.begin(9600);
    // first of all we want to poll the temperature from the slave
    client.poll(ReadTemperature, sizeof(ReadTemperature), [](ServerResponse *response){
        for (uint8_t i = 0; i <4; i++){
            sensorT.byteCode[i] = response->payload()[i];
        }
        Serial.printf("Read Temperature %2.f from slave address %d\n", sensorT.temperature, response->slaveAdress());
    });

    // start the client
    client.start();
}

size_t timeTaken{};

void loop(){
    scheduler.execute();

    // from time to time we want that slave updates it temperature
    if (millis()-timeTaken >= 1000 ){
        bool toggle{false};
        client.send(&updateTemperature);
    }
}

```

### Exception
You also can hook up in the way client and server are handling exception. That could be very useful for debugging.
```c++
    // ...

    // add a custom exception handler
    client.setOnError([](ServerResponse *response, ErrorCode error){
        Serial.print("Got Exception:\n");
        Serial.printf("Code: %d\n", (int)error);
        Serial.printf("Slave: %d\n", response->slaveAdress());
        Serial.printf("Function: %d\n", response->functionCode());
    });

    // ...
```
```c++
    // ...

    server.setOnError([](ModbusExceptionResponse<ProviderType> *response){
        Serial.print("Exception:\n");
        Serial.printf("Error: %d\n", (int)response->errorCode());
        Serial.printf("Function: %d\n", response->functionCode()); 
        // finally send the exception
        response->sendException();
    });

    // ...
```
<p align="right">(<a href="#readme-top">back to top</a>)</p>

## Roadmap
### General
* Add library to arduino library manager
* Add type constrains for template types whenever C++20 is available at a larger base
* Remove dependency task scheduler for interrupt/timer driven solution
* Unify interfaces
* Add more provider templates


<p align="right">(<a href="#readme-top">back to top</a>)</p>

## License

Distributed under the MIT License. See `LICENSE.md` for more information.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

## Contact

[homepage](https://cloasdata.de) - seimen at cloasdata.de

Project Link: [https://github.com/cloasdata/modernbus](https://github.com/cloasdata/modernbus)

<p align="right">(<a href="#readme-top">back to top</a>)</p>


[license-shield]: https://img.shields.io/github/license/cloasdata/modernbus.svg?style=for-the-badge
[license-url]: https://github.com/cloasdata/modernbus/blob/master/LICENSE.md

[platform-shield]: https://img.shields.io/badge/platform-arduino_|_C++11_|_platformio-brightgreen?style=for-the-badge
[platfrom-url]: https://www.arduino.cc/