#if !defined(MODERNBUS_REQUEST_H)
#define MODERNBUS_REQUEST_H

#include <Arduino.h>
#include <linkedlist.h>

#include "mbparser.h"

#include "modernbus_server_response.h"

#ifdef STD_FUNCTIONAL
    #include <functional>
#endif


/*
ModbusRequest holds all essential data required for a typical request.
Requests are send from a client class to a server class. 
*/
class ModbusRequest{
    template<typename> friend class ModbusClient;
    
    #ifndef STD_FUNCTIONAL
        template <typename U>
        friend void _parserComplete(ResponseParser *parser);
        template <typename U>
        friend void _handleError(ResponseParser *parser);
    #endif

    public:
        ModbusRequest() = delete;
        ModbusRequest(const ModbusRequest& r) = delete;
        ModbusRequest(uint8_t* request, uint16_t requestSize, bool swap, uint16_t registerSize, ResponseHandler handler);
        ~ModbusRequest();
        // Getter

        uint8_t slaveAddress()const;
        uint16_t functionCode()const;
        uint16_t address()const;
        uint16_t requestSize();
        ServerResponse& response();
        bool swap()const;
        uint16_t registerSize()const;
        uint16_t throttle() const;
        void* getExtension();
        ResponseHandler getHandler() const;
        uint16_t deviceDelay() const;

        //Setter

        ModbusRequest& setThrottle(uint16_t time);
        void setExtension(void *ptr);
        ModbusRequest& setTimeout(uint32_t time);
        ModbusRequest& setDeviceDelay(uint16_t millis_);

    private:
        TinyLinkedList<uint8_t> _requestFrame{};
        bool _swap = false;
        uint16_t _registerSize{0};
        ResponseHandler _handler;
        ServerResponse _response;
        uint16_t _throttle{0};
        uint32_t _timeOut{500};
        uint32_t _requestSent{};
        void* _extensionPtr {nullptr};
        void _validateSwap();
        void _determineQuantity();

        const uint8_t _slaveAddress;
        const uint8_t _functionCode;
        const uint16_t _address;
        uint16_t _deviceDelay{30};
        uint16_t _registerQuantity{1};
};



#endif // MODERNBUS_REQUEST_H
