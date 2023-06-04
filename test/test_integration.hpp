#include <TaskSchedulerDeclarations.h>

#include "fixture.hpp"
#include "../src/modernbus_client.h"
#include "../src/modernbus_server.h"
#include "../src/modernbus_crosslink.h"

CrossLinkManager manager{};
CrossLinkStream clientStream{manager.first};
CrossLinkProvider clientProvider{clientStream};
CrossLinkStream serverStream{manager.second};
CrossLinkProvider serverProvider{serverStream};
Scheduler scheduler{};

using ResponseT = ModbusResponse<CrossLinkProvider>;


void GivenClientAndServer_WhenBothUsingCrosslink_ThenNoError(){
    ModbusClient<CrossLinkProvider> client{&scheduler, &clientProvider};
    ModbusServer<CrossLinkProvider> server{&scheduler, &serverProvider, 0x01};
    client.start();
    server.start();
    

    for (int i = 0; i<100; i++){
        scheduler.execute();
    }
    assert(client.isRunning());
    assert(server.isRunning());

}

void GivenRequest_WhenBothUsingCrosslink_ThenNoError(){
    ModbusClient<CrossLinkProvider> client{&scheduler, &clientProvider};
    ModbusServer<CrossLinkProvider> server{&scheduler, &serverProvider, 0x01};
    
    client.poll(ReadRequest04, sizeof(ReadRequest04),[](ServerResponse *response){
        assert(response->payload()[0] == 0x00);
        assert(response->payload()[79] == 0x4f);
    });   
    
    server.responseTo(0x04, 0x01, [](ResponseT *respone){
        respone->send(Payload04, sizeof(Payload04));
    });
    
    client.start();
    server.start();
    

    while(client.completeCount() < 1){
        scheduler.execute();
    }
    assert(client.isRunning());
    assert(server.isRunning());

}

void GivenWriteRequest_WhenBothUsingCrosslink_ThenNoError(){
    ModbusClient<CrossLinkProvider> client{&scheduler, &clientProvider};
    ModbusServer<CrossLinkProvider> server{&scheduler, &serverProvider, 0x01};
    
    client.poll(WriteRequest05, sizeof(WriteRequest05),[](ServerResponse *response){
        assert(response->payload()[0] == WriteRequest05[4]);
    });   
    
    server.responseTo(0x05, 0x00AC, [](ResponseT *respone){
        respone->sendEcho();
    });
    
    client.start();
    server.start();
    

    while(client.completeCount() < 10){
        scheduler.execute();
    }
    assert(client.isRunning());
    assert(server.isRunning());

}

void GivenRequest04_WhenWithMap_ThenNoError(){
    ModbusClient<CrossLinkProvider> client{&scheduler, &clientProvider};
    ModbusServer<CrossLinkProvider> server{&scheduler, &serverProvider, 0x01};
    
    client.poll(ReadRequest04, sizeof(ReadRequest04),[](ServerResponse *response){
        assert(response->payload()[1]);
    });   
    
    server.responseTo(0x04, 0x0001).with(Payload04, sizeof(Payload04), 1);
    
    client.start();
    server.start();
    

    while(client.completeCount() < 10){
        scheduler.execute();
    }
    assert(client.isRunning());
    assert(server.isRunning());

}

void runIntegrationTests(){
    Serial.print("\n-- Testing integration of Modernbus --\n");
    GivenClientAndServer_WhenBothUsingCrosslink_ThenNoError();
    Serial.print(".");
    GivenRequest_WhenBothUsingCrosslink_ThenNoError();
    Serial.print(".");
    GivenWriteRequest_WhenBothUsingCrosslink_ThenNoError();
    Serial.print(".");
    GivenRequest04_WhenWithMap_ThenNoError();
    Serial.print(".");
    Serial.print("\n-- Integration Test Done --\n");
}
