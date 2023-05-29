#include "TaskSchedulerDeclarations.h"

#include "modernbus_client.h"
#include "modernbus_server_response.h"
#include "modernbus_provider.h"
#include "mbparser.h"

#include "mock.hpp"
#include "fixture.hpp"


/*
All the unit test in here running against a MockStream class.
This makes it possible to investigate the client write/read behaviour.
*/

using providerType = SerialProvider<MockStream>;

//MockStream mStream{LongResponse04, 85};
//providerType testProvider{mStream};

Scheduler testScheduler{};


void setupTest(){
    
}



void GivenNothing_WhenInit_ThenNoError(){
    MockStream mStream{};
    providerType testProvider{mStream};
    
    ModbusClient<providerType> client {&testScheduler,&testProvider};
    assert(!client.isRunning());
}

void GivenClient_WhenRun_ThenNoError(){
    MockStream mStream{};
    providerType testProvider{mStream};
    
    ModbusClient<providerType> client {&testScheduler,&testProvider};
    client.start();
    assert (client.isRunning());
    for (int n=0; n<100; n++){
        testScheduler.execute();
    }
    assert(client.errorCount() == 0);
}

void GivenClient_WhenAdd_Request_ThenNoError(){
    MockStream mStream{};
    providerType testProvider{mStream};
    mStream.append(Response04, sizeof(Response04));
    mStream.begin();

    ModbusClient<providerType> client {&testScheduler,&testProvider};
    ModbusRequest &request = client.poll(ReadRequest04, sizeof(ReadRequest04), [](ServerResponse* response){});
    assert(request.getHandler() != nullptr);
    client.start();
    
    for (int n=0; n<1000; n++){
        testScheduler.execute();
    }
    assert(client.requestCount() > 0);
}

void GivenClient_WhenAddRequest_ThenCorrectRequestOnProvider(){
    MockStream mStream{};
    providerType testProvider{mStream};
    mStream.append(Response04, sizeof(Response04));
    mStream.begin();

    ModbusClient<providerType> client {&testScheduler,&testProvider};
    client.poll(ReadRequest04, sizeof(ReadRequest04), true, 2, [&mStream](ServerResponse * response){
        assert(response->functionCode() == 0x04);
        assert(mStream.bytesWritten() == 8);
    }).setThrottle(5);
    client.start();
    
    while(client.getParser().state() != ParserState::complete){
        testScheduler.execute();
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

    ModbusClient<providerType> client {&testScheduler,&testProvider};
    client.poll(ReadRequest04, sizeof(ReadRequest04), false, 2, [](ServerResponse* response){
        assert(response->functionCode() == 0x04);
        assert(response->slaveAdress() == 0x01);
        assert(response->byteCount() == 80);
        assert(response->request()->throttle() == 5);
        for (int i = 0; i < response->byteCount(); i++){
            assert(response->payload()[i] == Response04[i+3]);
        }       
    }).setThrottle(5);
    client.start();
    while(!client.getParser().isComplete()){
        testScheduler.execute();
    }
    assert(client.completeCount() == 1);
}

void GivenRequest_WhenSendSingleRequest_ThenCorrectResponse(){
    MockStream mStream{};
    providerType testProvider{mStream};
    mStream.append(Response04, sizeof(Response04));
    mStream.begin();

    ModbusClient<providerType> client {&testScheduler, &testProvider};
    client.start();
    bool called{false};
    ModbusRequest request{ReadRequest04, sizeof(ReadRequest04),false, 0, [&called](ServerResponse *response ){called=true;}};
    client.send(&request);
    while(!client.getParser().isComplete()){
        testScheduler.execute();
    }
    assert(client.completeCount() == 1);
    assert(called == true);
}

void GivenWriteRequest05_WhenSendRequest_ThenCorrectResponse(){
    MockStream mStream{};
    providerType testProvider{mStream};
    mStream.append(WriteRequest05, sizeof(WriteRequest05)); // echo msg
    mStream.begin();

    ModbusClient<providerType> client {&testScheduler, &testProvider};
    client.start();
    bool called{false};
    ModbusRequest request{WriteRequest05, sizeof(WriteRequest05) , false, 0, [&called](ServerResponse *response ){
        called = true;
        assert(response->slaveAdress() == 0x01);
        assert(response->functionCode() == 0x05);
        assert(response->address() == 0x00AC);
        assert(response->payload()[0] == 0xFF);
    }};
    client.send(&request);
    while(!client.getParser().isComplete()){
        testScheduler.execute();
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

    ModbusClient<providerType> client {&testScheduler, &testProvider};
    uint8_t called = 0;
    client.poll(ReadRequest01, sizeof(ReadRequest01), [&called](ServerResponse *response ){called++;});
    client.poll(ReadRequest04, sizeof(ReadRequest04), [&called](ServerResponse *response ){called++;});
    client.poll(WriteRequest16, sizeof(WriteRequest16), [&called](ServerResponse *response ){called++;});
    client.start();

    while (called < 1){
        testScheduler.execute();
    }
    assert(mStream.compare(ReadRequest01));
    mStream.nextData();
    while (called < 2){
        testScheduler.execute();
    }
    assert(mStream.compare(ReadRequest04));
    mStream.nextData();
    while (called < 3){
        testScheduler.execute();
    }
    assert(mStream.compare(WriteRequest16));
}

void GivenClientWithHandlers_WhenSendingSeveralSingleRequest_ReturnThemInOrder(){
    MockStream mStream{};
    providerType testProvider{mStream};
    mStream.append(Response01, sizeof(Response01));
    for (int i = 0; i<10; i++){
        mStream.append(Response04, sizeof(Response04));
    }
    mStream.append(Response16, sizeof(Response16));
    mStream.begin();

    ModbusClient<providerType> client {&testScheduler, &testProvider};
    uint8_t called = 0;
    
    ModbusRequest request{WriteRequest05, sizeof(WriteRequest05), false, 0, [&called](ServerResponse *response ){called++;}};
    client.poll(ReadRequest01, sizeof(ReadRequest01), [&called, &client, &request](ServerResponse *response ){
        called++;
        for (int i = 0; i<10; i++){
            client.send(&request);
        }
    });
    
    client.poll(WriteRequest16, sizeof(WriteRequest16), [&called](ServerResponse *response ){called++;});
    client.start();

    while (called < 12){
        testScheduler.execute();
    }
    assert(client.errorCount() == 0);
}

void GivenClientWithHandler_WhenResponseWrong_ReturnErrorCount(){
    MockStream mStream{};
    providerType testProvider{mStream};
    mStream.append(BadResponseCRC03, sizeof(BadResponseCRC03));
    mStream.begin();

    bool called{false};
    ModbusClient<providerType> client {&testScheduler,&testProvider};
    client.poll(ReadRequest04, sizeof(ReadRequest04), true, 2, [&called](ServerResponse * response){
        called = true;
    }).setThrottle(5);
    client.start();
    
    while(!client.getParser().isComplete() && !client.getParser().isError()){
        testScheduler.execute();
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
    ModbusClient<providerType> client {&testScheduler,&testProvider};
    client.setOnError([&called](ServerResponse *response, ErrorCode errorCode){
        called = true;
        assert(errorCode == ErrorCode::CRCError);
    });
    
    client.poll(ReadRequest04, sizeof(ReadRequest04), true, 2, [&called](ServerResponse * response){
        assert(false);
    }).setThrottle(5);
    client.start();
    
    while(!client.getParser().isComplete() && !client.getParser().isError()){
        testScheduler.execute();
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
    ModbusClient<providerType> client {&testScheduler,&testProvider};
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
        testScheduler.execute();
    }
    
    assert(called);
    assert(client.errorCount() == 1);
    assert(client.completeCount() == 0);
}

void GivenClientWithHandlersSendingSingleRequests_WhenDestroyed_ReturnNoError(){
    /* todo */
}


void runClientTest(){
    setupTest();
    printf("\n\n -- TEST STARTING -- \n\n");
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

    printf("\n");
    runningTime = millis() - runningTime;
    heapConsumed -= ESP.getFreeHeap();
    printf("HEAP Consumed: %u\n", heapConsumed);
    assert(heapConsumed == 0);
    printf("\nTime: %lu\n", runningTime);
    
    printf("TEST DONE. BYE");
    ESP.restart();

}