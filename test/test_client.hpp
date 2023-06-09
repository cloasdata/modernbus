#ifndef TEST_CLIENT_H
#define TEST_CLIENT_H
#include "TaskSchedulerDeclarations.h"

#include "mbparser.h"
#include "mock.hpp"
#include "fixture.hpp"
#include "../src/modernbus_client.h"
#include "../src/modernbus_server_response.h"
#include "../src/modernbus_provider.h"



/*
All the unit test in here running against a MockStream class.
This makes it possible to investigate the client write/read behaviour.
*/

using providerType = SerialProvider<MockStream>;

Scheduler clientScheduler{};

void GivenNothing_WhenInit_ThenNoError(){
    MockStream mStream{};
    providerType testProvider{mStream};
    
    ModbusClient<providerType> client {&clientScheduler,&testProvider};
    assert(!client.isRunning());
}

void GivenClient_WhenRun_ThenNoError(){
    MockStream mStream{};
    providerType testProvider{mStream};
    
    ModbusClient<providerType> client {&clientScheduler,&testProvider};
    client.start();
    assert (client.isRunning());
    for (int n=0; n<100; n++){
        clientScheduler.execute();
    }
    assert(client.errorCount() == 0);
}

void GivenClient_WhenAdd_Request_ThenNoError(){
    MockStream mStream{};
    providerType testProvider{mStream};
    mStream.append(Response04, sizeof(Response04));
    mStream.begin();

    ModbusClient<providerType> client {&clientScheduler,&testProvider};
    ModbusRequest &request = client.poll(ReadRequest04, sizeof(ReadRequest04), [](ServerResponse* response){});
    assert(request.getHandler() != nullptr);
    client.start();
    
    for (int n=0; n<1000; n++){
        clientScheduler.execute();
    }
    assert(client.requestCount() > 0);
}

void GivenClient_WhenAddRequest_ThenCorrectRequestOnProvider(){
    MockStream mStream{};
    providerType testProvider{mStream};
    mStream.append(Response04, sizeof(Response04));
    mStream.begin();

    ModbusClient<providerType> client {&clientScheduler,&testProvider};
    client.poll(ReadRequest04, sizeof(ReadRequest04), true, 2, [&mStream](ServerResponse * response){
        assert(response->functionCode() == 0x04);
        assert(mStream.bytesWritten() == 8);
    }).setThrottle(5);
    client.start();
    
    while(client.getParser().state() != ParserState::complete){
        clientScheduler.execute();
    }
    
    for (int n = 0; n < 8; n++){
        assert(mStream.writeBuffer()[n] == ReadRequest04[n]);
    }
    assert(client.completeCount() == 1);
}

void GivenClient_WhenAdd_Request_ThenCorrectResponeClient(){
    MockStream mStream{};
    providerType testProvider{mStream};
    mStream.append(Response04, sizeof(Response04));
    mStream.begin();

    ModbusClient<providerType> client {&clientScheduler,&testProvider};
    client.poll(ReadRequest04, sizeof(ReadRequest04), false, 2, [](ServerResponse* response){
        assert(response->functionCode() == 0x04);
        assert(response->slaveAddress() == 0x01);
        assert(response->byteCount() == 80);
        assert(response->request()->throttle() == 5);
        for (int i = 0; i < response->byteCount(); i++){
            assert(response->payload()[i] == Response04[i+3]);
        }       
    }).setThrottle(5);
    client.start();
    while(!client.getParser().isComplete()){
        clientScheduler.execute();
    }
    assert(client.completeCount() == 1);
}

void GivenRequest_WhenSendSingleRequest_ThenCorrectResponse(){
    MockStream mStream{};
    providerType testProvider{mStream};
    mStream.append(Response04, sizeof(Response04));
    mStream.begin();

    ModbusClient<providerType> client {&clientScheduler, &testProvider};
    client.start();
    bool called{false};
    ModbusRequest request{ReadRequest04, sizeof(ReadRequest04),false, 0, [&called](ServerResponse *response ){called=true;}};
    client.send(&request);
    while(!client.getParser().isComplete()){
        clientScheduler.execute();
    }
    assert(client.completeCount() == 1);
    assert(called == true);
}

void GivenWriteRequest05_WhenSendRequest_ThenCorrectResponse(){
    MockStream mStream{};
    providerType testProvider{mStream};
    mStream.append(WriteRequest05, sizeof(WriteRequest05)); // echo msg
    mStream.begin();

    ModbusClient<providerType> client {&clientScheduler, &testProvider};
    client.start();
    bool called{false};
    ModbusRequest request{WriteRequest05, sizeof(WriteRequest05) , false, 0, [&called](ServerResponse *response ){
        called = true;
        assert(response->slaveAddress() == 0x01);
        assert(response->functionCode() == 0x05);
        assert(response->address() == 0x00AC);
        assert(response->payload()[0] == 0xFF);
    }};
    client.send(&request);
    while(!client.getParser().isComplete()){
        clientScheduler.execute();
    }
    assert(client.completeCount() == 1);
    assert(called == true);
}

void GivenDifferentRequests_WhenRequested_ThenCorrectResponse(){
    MockStream mStream{};
    providerType testProvider{mStream};
    mStream.append(Response01, sizeof(Response01));
    mStream.append(Response04, sizeof(Response04));
    mStream.append(Response16, sizeof(Response16));
    mStream.begin();

    ModbusClient<providerType> client {&clientScheduler, &testProvider};
    uint8_t called = 0;
    client.poll(ReadRequest01, sizeof(ReadRequest01), [&called](ServerResponse *response ){called++;});
    client.poll(ReadRequest04, sizeof(ReadRequest04), [&called](ServerResponse *response ){called++;});
    client.poll(WriteRequest16, sizeof(WriteRequest16), [&called](ServerResponse *response ){called++;});
    client.start();

    while (called < 1){
        clientScheduler.execute();
    }
    assert(mStream.compare(ReadRequest01));
    mStream.nextData();
    while (called < 2){
        clientScheduler.execute();
    }
    assert(mStream.compare(ReadRequest04));
    mStream.nextData();
    while (called < 3){
        clientScheduler.execute();
    }
    assert(mStream.compare(WriteRequest16));
}

void GivenClientWithHandlers_WhenSendingSeveralSingleRequest_ReturnThemInOrder(){
    MockStream mStream{};
    providerType testProvider{mStream};
    mStream.append(Response01, sizeof(Response01));
    for (int i = 0; i<3; i++){
        // Response to 05 is a echo.
        mStream.append(WriteRequest05, sizeof(WriteRequest05));
    }
    mStream.append(Response16, sizeof(Response16));
    mStream.begin();

    ModbusClient<providerType> client {&clientScheduler, &testProvider};
    uint8_t called = 0;
    
    ModbusRequest WriteRequest{WriteRequest05, sizeof(WriteRequest05), false, 0, [&called](ServerResponse *response ){called++;}};
    
    client.poll(ReadRequest01, sizeof(ReadRequest01), [&called, &client, &WriteRequest](ServerResponse *response ){
        called++;
        for (int i = 0; i<3; i++){
            client.send(&WriteRequest);
        }
    
    });
    
    client.poll(WriteRequest16, sizeof(WriteRequest16), [&called](ServerResponse *response ){called++;});

    client.setOnError([](ServerResponse *response, ErrorCode code ){
        Serial.printf("FC: %X, ADDR: %X, EC: %d", response->functionCode(), response->address(),(int)code);
    });
    client.start();

    while (called < 3){
        clientScheduler.execute();
    }
    //Serial.printf("TO: %d, EC: %d, Code: %d\n", client.timeoutCount(), client.errorCount(), (int)client.getParser().errorCode());
    assert(client.errorCount() == 0);
}

void GivenClientWithHandler_WhenResponseWrong_ReturnErrorCount(){
    MockStream mStream{};
    providerType testProvider{mStream};
    mStream.append(BadResponseCRC03, sizeof(BadResponseCRC03));
    mStream.begin();

    bool called{false};
    ModbusClient<providerType> client {&clientScheduler,&testProvider};
    client.poll(ReadRequest04, sizeof(ReadRequest04), true, 2, [&called](ServerResponse * response){
        called = true;
    }).setThrottle(5);
    client.start();
    
    while(!client.getParser().isComplete() && !client.getParser().isError()){
        clientScheduler.execute();
    }
    
    assert(!called);
    assert(client.errorCount() == 1);
    assert(client.getParser().errorCode() == ErrorCode::CRCError);
}


void GivenClientWithErrorHandler_WhenResponseWrong_CallErrorHandler(){
    MockStream mStream{};
    providerType testProvider{mStream};
    mStream.append(BadResponseCRC03, sizeof(BadResponseCRC03));
    mStream.begin();

    bool called{false};
    ModbusClient<providerType> client {&clientScheduler,&testProvider};
    client.setOnError([&called](ServerResponse *response, ErrorCode errorCode){
        called = true;
        assert(errorCode == ErrorCode::CRCError);
    });
    
    client.poll(ReadRequest04, sizeof(ReadRequest04), true, 2, [&called](ServerResponse * response){
        assert(false);
    }).setThrottle(5);
    client.start();
    
    while(!client.getParser().isComplete() && !client.getParser().isError()){
        clientScheduler.execute();
    }
    
    assert(called);
    assert(client.errorCount() == 1);
    assert(client.completeCount() == 0);
}

void GivenClientFunctionCodeValidation_WhenResponseWrong_CallErrorHandler(){
    MockStream mStream{};
    providerType testProvider{mStream};
    mStream.append(Response01, sizeof(Response01));
    mStream.begin();

    bool called{false};
    ModbusClient<providerType> client {&clientScheduler,&testProvider};
    client.setFunctionCodeValidation(true);

    client.setOnError([&called](ServerResponse *response, ErrorCode errorCode){
        called = true;
        assert(errorCode == ErrorCode::illegalFunction);
    });
    
    client.poll(ReadRequest04, sizeof(ReadRequest04), true, 2, [&called](ServerResponse * response){
        assert(false);
    }).setThrottle(5);
    client.start();
    
    while(!client.getParser().isComplete() && !client.getParser().isError()){
        clientScheduler.execute();
    }
    
    assert(called);
    assert(client.errorCount() == 1);
    assert(client.completeCount() == 0);
}

void GivenClientWithHandlersSendingSingleRequests_WhenDestroyed_ReturnNoError(){
    /* todo */
}

void GivenClientPollingRequest_WhenResponseTakesLong_ThenTimeOutOccurs(){
    MockStream mStream{};
    providerType testProvider{mStream};
    mStream.append(WriteRequest05, sizeof(WriteRequest05)); // echo msg
    mStream.setUnavailable(3, 600);
    mStream.begin();

    bool errorHandlerCalled{false};
    ModbusClient<providerType> client {&clientScheduler, &testProvider};
    client.setOnError([&errorHandlerCalled](ServerResponse *response, ErrorCode code ){
        errorHandlerCalled = true;
    });
    client.start();
    bool called{false};
    ModbusRequest request{WriteRequest05, sizeof(WriteRequest05) , false, 0, [&called](ServerResponse *response ){
        called = true;
        assert(response->slaveAddress() == 0x01);
        assert(response->functionCode() == 0x05);
        assert(response->address() == 0x00AC);
        assert(response->payload()[0] == 0xFF);
    }};

    client.send(&request);
    
    while(!client.timeoutCount()){
        clientScheduler.execute();
    }

    assert(client.timeoutCount() == 1);
    assert(errorHandlerCalled == true);
    assert(called == false);

}


void runClientTest(){
    printf("\n\n -- Testing Modernbus Client -- \n\n");
    uint16_t heapConsumed = ESP.getFreeHeap();
    unsigned long runningTime = millis();

    GivenClient_WhenAdd_Request_ThenNoError();
    GivenNothing_WhenInit_ThenNoError();
    printf(".");
    GivenClient_WhenRun_ThenNoError();
    printf(".");
    printf(".");
    GivenClient_WhenAddRequest_ThenCorrectRequestOnProvider();
    printf(".");
    GivenClient_WhenAdd_Request_ThenCorrectResponeClient();
    printf(".");
    GivenRequest_WhenSendSingleRequest_ThenCorrectResponse();
    printf(".");
    GivenWriteRequest05_WhenSendRequest_ThenCorrectResponse();
    printf(".");
    GivenDifferentRequests_WhenRequested_ThenCorrectResponse();
    printf(".");
    GivenClientWithHandlers_WhenSendingSeveralSingleRequest_ReturnThemInOrder();
    printf(".");
    GivenClientWithHandler_WhenResponseWrong_ReturnErrorCount();
    printf(".");
    GivenClientWithErrorHandler_WhenResponseWrong_CallErrorHandler();
    printf(".");
    GivenClientFunctionCodeValidation_WhenResponseWrong_CallErrorHandler();
    printf(".");
    GivenClientPollingRequest_WhenResponseTakesLong_ThenTimeOutOccurs();
    printf(".");

    printf("\n");
    runningTime = millis() - runningTime;
    heapConsumed -= ESP.getFreeHeap();
    printf("HEAP Consumed: %u\n", heapConsumed);
    assert(heapConsumed == 0);
    printf("\nTime: %lu\n", runningTime);
    
    printf("-- Modernbus Client Tested --");
}

#endif