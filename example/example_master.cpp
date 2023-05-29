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