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
    });

    // add a holding register address 0
    server.responseTo(0x03, 0x00, [](ModbusResponse<ProviderType> *response){
        // we may only allow to read 4 bytes from address 00
        if (response->byteCount() == 4){
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