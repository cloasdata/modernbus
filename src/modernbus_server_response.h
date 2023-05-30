#if !defined(MODERNBUS_SERVER_RESPONSE_H)
#define MODERNBUS_SERVER_RESPONSE_H

#include <Arduino.h>
#include <mbparser.h>


class ServerResponse;
// avoid circular reference
class ModbusRequest;

// Function Pointer Types
// If flag is set we can use full support of lambdas (capture this for example)
// However, functional may be expensive
#ifdef STD_FUNCTIONAL
    #include <functional>
    using ResponseHandler = std::function<void(ServerResponse *response)>;
    using ErrorHanlder = std::function<void(ServerResponse *response, ErrorCode errorCode)>;
#endif

#ifndef STD_FUNCTIONAL
    using ResponseHandler = void(*)(ServerResponse *response);
    using ErrorHanlder = void(*)(ServerResponse *response, ErrorCode errorCode);
#endif


/*
SeverResponse is dataclass abstracting a modbus server/slave response.
*/
class ServerResponse{
    template <typename>
    friend class ModbusClient;
    friend class ModbusRequest;
    public:
        ModbusRequest* request() const {return _request;};
        uint8_t slaveAdress() const {return _slaveAdress;};
        uint8_t functionCode() const {return _functionCode;};
        uint16_t address() const {return _address;};
        uint8_t *payload() const {return _payload;};
        uint16_t byteCount() const {return _byteCount;};

    private:
        ServerResponse(ModbusRequest *request);
        ModbusRequest *_request;
        uint8_t _slaveAdress{0xFF};
        uint8_t _functionCode{0};
        uint16_t _byteCount{};
        uint16_t _address{};
        uint8_t *_payload{nullptr};


};

#endif