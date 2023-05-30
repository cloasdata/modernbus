#include <Arduino.h>

#include "fixture.hpp"
#include "mock.hpp"
#include "../src/modernbus_server.h"
#include "../src/modernbus_provider.h"



using provideType = ProviderRS485<HardwareSerial>;

Scheduler serverScheduler{};

void GivenStreamWhenNewInstanceThenNoError(){
    ProviderRS485<HardwareSerial> provider{Serial, 0};
    
    ModbusServer<ProviderRS485<HardwareSerial>> server {&serverScheduler, &provider, 1};
    assert (server.slaveAddress() == 1);
}

void GivenServer_WhenStartedandRun_RaiseNoException(){
    ProviderRS485<HardwareSerial> provider{Serial, 0};
    ModbusServer<ProviderRS485<HardwareSerial>> server {&serverScheduler, &provider, 1};
    
    server.start();
    for (int i = 0; i < 100; i++){
        serverScheduler.execute();
    }
    

    assert (server.slaveAddress() == 1);
}

void GivenServerWhenResponseAddedThenNoError(){
    ProviderRS485<HardwareSerial> provider{Serial, 0};
    ModbusServer<ProviderRS485<HardwareSerial>> server {&serverScheduler, &provider, 1};

    server.responseTo(03, 0, [](ModbusResponse<provideType> *response){});
    
    assert (server.slaveAddress() == 1);
}

void GivenServer_WhenStarted_ThenNoError(){
    MockStream mock{};
    SerialProvider<MockStream> provider{mock};
    ModbusServer<SerialProvider<MockStream>> server {&serverScheduler, &provider, 1};
    mock.append(ReadRequest04, sizeof(ReadRequest04));
    mock.begin();

    server.responseTo(04, 0x0001, [](ModbusResponse<SerialProvider<MockStream>> *response){});
    server.start();
    while (mock.available()){
        serverScheduler.execute();
    }
    assert (server.errorCount() == 0);
}

void GivenServer_WhenRun_ThenCallback(){
    MockStream mock{};
    SerialProvider<MockStream> provider{mock};
    ModbusServer<SerialProvider<MockStream>> server {&serverScheduler, &provider, 1};
    mock.append(ReadRequest04, sizeof(ReadRequest04));
    mock.begin();

    server.responseTo(04, 0x0001, [](ModbusResponse<SerialProvider<MockStream>> *response){
        uint8_t payload[2] = {0x00, 0xFF};
        response->send(payload, 2);
    });

    server.start();
    
    while (!server.getParser().isComplete() && !server.getParser().isError()){
        serverScheduler.execute();
    }
    assert(mock.writeBuffer()[4] == 0xFF);
}

void Given03Handler_WhenRequest04_ThenExceptionResponse(){
    MockStream mock{};
    SerialProvider<MockStream> provider{mock};
    ModbusServer<SerialProvider<MockStream>> server {&serverScheduler, &provider, 1};
    mock.append(ReadRequest04, sizeof(ReadRequest04));
    mock.begin();

    // 03 fc is not requested
    server.responseTo(03, 0x0131, [](ModbusResponse<SerialProvider<MockStream>> *response){
        uint8_t payload[2] = {0x00, 0xFF};
        response->send(payload, 2);
        ESP.restart();
    });

    server.start();
    while (!server.getParser().isComplete() && !server.getParser().isError()){
        serverScheduler.execute();
    }

    assert(mock.writeBuffer()[1] == 04 + 128); // exception response
}

void Given130AddressHandler_WhenRequest130_ThenExceptionResponse(){
    MockStream mock{};
    SerialProvider<MockStream> provider{mock};
    ModbusServer<SerialProvider<MockStream>> server {&serverScheduler, &provider, 1};
    mock.append(ReadRequest04, sizeof(ReadRequest04));
    mock.begin();

    // address 131 not available
    server.responseTo(04, 0x0130, [](ModbusResponse<SerialProvider<MockStream>> *response){
        uint8_t payload[2] = {0x00, 0xFF};
        response->send(payload, 2);
    });

    server.start();
    while (!server.getParser().isComplete() && !server.getParser().isError()){
        serverScheduler.execute();
    }
    assert(mock.writeBuffer()[1] == 04 + 128); // exception response
    assert(mock.writeBuffer()[2] == 2);
}

void GivenServerVerboseParser_WhenRequestCRCBad_ThenExceptionResponse(){
    MockStream mock{};
    SerialProvider<MockStream> provider{mock};
    ModbusServer<SerialProvider<MockStream>> server {&serverScheduler, &provider, 1};
    mock.append(BadCRCRequest04, sizeof(BadCRCRequest04));
    mock.begin();
    server.verboseParser(true); // parser errors are ignored by default

    server.responseTo(04, 0x0001, [](ModbusResponse<SerialProvider<MockStream>> *response){
        uint8_t payload[2] = {0x00 ,0xFF};
        response->send(payload, 2);
    });

    server.start();
    while (!server.getParser().isComplete() && !server.getParser().isError()){
        serverScheduler.execute();
    }

    assert(mock.writeBuffer()[1] == 04 + 128); // exception response
    assert(mock.writeBuffer()[2] == 21); // Parser internal CRC error
}

void GivenUserErrorHandler_WhenRequest04_ThenExceptionResponse(){
    MockStream mock{};
    SerialProvider<MockStream> provider{mock};
    ModbusServer<SerialProvider<MockStream>> server {&serverScheduler, &provider, 1};
    mock.append(ReadRequest04, sizeof(ReadRequest04));
    mock.begin();

    server.setOnError([](ModbusExceptionResponse<SerialProvider<MockStream>> *response){
        assert(response->functionCode() == 04);
        response->sendException();
    });
    
    //request is 04
    server.responseTo(03, 0x0131, [](ModbusResponse<SerialProvider<MockStream>> *response){
        uint8_t payload[2] = {0x00, 0xFF};
        response->send(payload, 2);
    });

    server.start();
    while (!server.getParser().isComplete() && !server.getParser().isError()){
        serverScheduler.execute();
    }
    assert(mock.writeBuffer()[1] == 04 + 128); 
    assert(mock.writeBuffer()[2] == 1); 
}

void runServerTest(){
    printf("\n\n -- Testing Modernbus Server -- \n\n");
    uint16_t heapConsumed = ESP.getFreeHeap();
    unsigned long runningTime = millis();

    GivenStreamWhenNewInstanceThenNoError();
    printf(".");
    GivenServer_WhenStartedandRun_RaiseNoException();
    printf(".");
    GivenServerWhenResponseAddedThenNoError();
    printf(".");
    GivenServer_WhenStarted_ThenNoError();
    printf(".");
    GivenServer_WhenRun_ThenCallback();
    printf(".");
    Given03Handler_WhenRequest04_ThenExceptionResponse();
    printf(".");
    Given130AddressHandler_WhenRequest130_ThenExceptionResponse();
    printf(".");
    GivenServerVerboseParser_WhenRequestCRCBad_ThenExceptionResponse();
    printf(".");
    GivenUserErrorHandler_WhenRequest04_ThenExceptionResponse();
    printf(".");

    printf("\n");
    runningTime = millis() - runningTime;
    printf("HEAP Consumed: %u\n", heapConsumed - ESP.getFreeHeap());
    printf("\nTime: %lu\n", runningTime);
    
    printf("-- Modernbus Server Tested --");
}
