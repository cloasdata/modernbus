#ifndef FIXTURE_H
#define FIXTURE_H
#include <Arduino.h>
#include <mbparser.h>
#include <modernbus_server_response.h>

uint8_t Response01[] {0x01, 0x01, 0x02, 0x0A, 0x11, 0x7F, 0x50};
uint8_t GoodResponse03[] {0x01, 0x03, 0x04, 0x0, 0x6, 0x0, 0x05, 0xDA, 0x31};
uint8_t Response04[] {0x01, 0x04, 0x50, 0x40, 0x6A, 0x9F, 0xBE, 0x40, 0xF5, 0x4F, 0xDF, 0x41, 0x3A, 0xA7, 0xF0, 0x41, 0x7A, 0xA7, 0xF0, 0x41, 0x9D, 0x53, 0xF8, 0x41, 0xBD, 0x53, 0xF8, 0x41, 0xDD, 0x53, 0xF8, 0x41, 0xFD, 0x53, 0xF8, 0x42, 0x0E, 0xA9, 0xFC, 0x42, 0x1E, 0xA9, 0xFC, 0x42, 0x2E, 0xA9, 0xFC, 0x42, 0x3E, 0xA9, 0xFC, 0x42, 0x4E, 0xA9, 0xFC, 0x42, 0x5E, 0xA9, 0xFC, 0x42, 0x6E, 0xA9, 0xFC, 0x42, 0x7E, 0xA9, 0xFC, 0x42, 0x87, 0x54, 0xFE, 0x42, 0x8F, 0x54, 0xFE, 0x42, 0x97, 0x54, 0xFE, 0x42, 0x9F, 0x54, 0xFE, 0x11, 0x94};
uint8_t Payload04[] {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f};
uint8_t Response06[] {0x11, 0x06, 0x00, 0x01, 0x00, 0x03, 0x9A, 0x9B};
uint8_t Response15[] {0x11, 0x0F, 0x00, 0x01, 0x00, 0x02, 0x87, 0x5A};
uint8_t Response16[] {0x01, 0x10, 0x00, 0x01, 0x00, 0x02, 0x10, 0x08};

uint8_t BadResponseCRC03[] {0x01, 0x03, 0x04, 0x0, 0x6,0x0, 0x05, 0xFF, 0x31};

uint8_t ExceptionResponse [] {0x01, 0x82, 0x02};

uint8_t ReadRequest01[] {0x01, 0x01, 0x00, 0x0A, 0x00, 0x0D, 0xDD, 0xCD};
uint8_t ReadRequest04[] {0x01, 0x04, 0x00, 0x01, 0x00, 0x28, 0xA1, 0xD4};

uint8_t WriteRequest05[] {0x01, 0x05, 0x00, 0xAC, 0xFF, 0x00, 0x4C, 0x1B};
uint8_t WriteRequest15[] {0x01, 0x0F, 0x00, 0x13, 0x00, 0x0A, 0x02, 0xCD, 0x01, 0x72, 0xCB};
uint8_t WriteRequest16[] {0x01, 0x10, 0x00, 0x01, 0x00, 0x02, 0x04, 0x00, 0x0A, 0x01, 0x02, 0x92, 0x30};

uint8_t BadCRCRequest04[] {0x01, 0x04, 0x01, 0x31, 0x0, 0x01E, 0x20, 0xFF};

void printParserState(ParserState state, ErrorCode error){
    Serial.printf("Parser State: %d\nParser Error: %d\n", static_cast<int>(state), static_cast<int>(error));
}


#endif