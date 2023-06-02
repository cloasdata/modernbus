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

void GivenReadRequest04_WhenRun_ThenCallback(){
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
    assert(mock.writeBuffer()[0] == 0x01);
    assert(mock.writeBuffer()[1] == 0x04);
    assert(mock.writeBuffer()[2] == 0x02);
    assert(mock.writeBuffer()[3] == 0x00);
    assert(mock.writeBuffer()[4] == 0xFF);
    assert(mock.writeBuffer()[5]);
    assert(mock.writeBuffer()[6]);
}

void GivenWriteRequest05_WhenCallback_ThenEchoResponse(){
    MockStream mock{};
    SerialProvider<MockStream> provider{mock};
    ModbusServer<SerialProvider<MockStream>> server {&serverScheduler, &provider, 1};
    mock.append(WriteRequest05, sizeof(WriteRequest05));
    mock.begin();

    bool stuff_done{false};
    server.responseTo(05, 0x00AC, [&stuff_done](ModbusResponse<SerialProvider<MockStream>> *response){
        stuff_done = true;
        response->sendEcho();
    });

    server.start();
    
    while (!server.getParser().isComplete() && !server.getParser().isError()){
        serverScheduler.execute();
    }
    assert(stuff_done);
    for (int i = 0; i<8; i++){
        assert(mock.writeBuffer()[i] == WriteRequest05[i]);
    }
}

void GivenWriteRequest15_WhenCallback_ThenCorrectResponse(){
    MockStream mock{};
    SerialProvider<MockStream> provider{mock};
    ModbusServer<SerialProvider<MockStream>> server {&serverScheduler, &provider, 1};
    mock.append(WriteRequest15, sizeof(WriteRequest15));
    mock.begin();

    bool stuff_done{false};
    server.responseTo(15, 0x0013, [&stuff_done](ModbusResponse<SerialProvider<MockStream>> *response){
        stuff_done = true;
        response->sendEcho();
    });

    server.start();
    
    while (!server.getParser().isComplete() && !server.getParser().isError()){
        serverScheduler.execute();
    }
    assert(stuff_done);
    assert(mock.writeBuffer()[0] == WriteRequest15[0]);
    assert(mock.writeBuffer()[1] == WriteRequest15[1]);
    assert(mock.writeBuffer()[2] == WriteRequest15[2]);
    assert(mock.writeBuffer()[3] == WriteRequest15[3]);
    assert(mock.writeBuffer()[4] == WriteRequest15[4]);
    assert(mock.writeBuffer()[5] == WriteRequest15[5]);
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

void GivenReadRequest04_WhenAddressIsLarger_ThenGoodResponse(){
    MockStream mock{};
    SerialProvider<MockStream> provider{mock};
    ModbusServer<SerialProvider<MockStream>> server {&serverScheduler, &provider, 1};
    mock.append(ReadRequest04, sizeof(ReadRequest04));
    mock.begin();
    
    // server is responding from 0000. RequestStarts at 1. this means byte count should be 79,
    // first payload byte is one off
    server.responseTo(04, 0x0000).with(Payload04, sizeof(Payload04), 1);

    server.start();
    
    while (!server.getParser().isComplete() && !server.getParser().isError()){
        serverScheduler.execute();
    }
    assert(mock.writeBuffer()[0] == 0x01);
    assert(mock.writeBuffer()[1] == 0x04);
    assert(mock.writeBuffer()[2] == 0x27); // bytecount
    assert(mock.writeBuffer()[3] == 0x01); // first payload byte is one off
    assert(mock.writeBuffer()[40] == 0x26);
}

void GivenReadRequest04_WhenRegisterRequestedToLarge_ThenException(){
    MockStream mock{};
    SerialProvider<MockStream> provider{mock};
    ModbusServer<SerialProvider<MockStream>> server {&serverScheduler, &provider, 1};
    mock.append(ReadRequest04, sizeof(ReadRequest04));
    mock.begin();
    
    server.responseTo(04, 0x0000).with(Payload04, 4, 1);

    server.start();
    
    while (!server.getParser().isComplete() && !server.getParser().isError()){
        serverScheduler.execute();
    }
    assert(mock.writeBuffer()[0] == 0x01);
    assert(mock.writeBuffer()[1] == 4 + 128);
    assert(mock.writeBuffer()[2] == 3);
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
    GivenReadRequest04_WhenRun_ThenCallback();
    printf(".");
    GivenWriteRequest05_WhenCallback_ThenEchoResponse();
    printf(".");
    GivenWriteRequest15_WhenCallback_ThenCorrectResponse();
    printf(".");
    Given03Handler_WhenRequest04_ThenExceptionResponse();
    printf(".");
    Given130AddressHandler_WhenRequest130_ThenExceptionResponse();
    printf(".");
    GivenUserErrorHandler_WhenRequest04_ThenExceptionResponse();
    printf(".");
    GivenReadRequest04_WhenAddressIsLarger_ThenGoodResponse();
    printf(".");
    GivenReadRequest04_WhenRegisterRequestedToLarge_ThenException();
    printf(".");

    printf("\n");
    runningTime = millis() - runningTime;
    printf("HEAP Consumed: %u\n", heapConsumed - ESP.getFreeHeap());
    printf("\nTime: %lu\n", runningTime);
    
    printf("-- Modernbus Server Tested --");
}
