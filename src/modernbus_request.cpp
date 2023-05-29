#include "modernbus_request.h"
#include "modernbus_server_response.h"

ModbusRequest::ModbusRequest(uint8_t *request, uint16_t requestSize, bool swap, uint16_t registerSize, ResponseHandler handler)
:   _requestFrame{request, requestSize},
    _swap{swap},
    _registerSize{registerSize},
    _handler{handler},
    _response{this},
    _slaveAdress{request[0]},
    _functionCode{request[1]},
    _address{(uint16_t)((request[2] << 8) | request[3])}
{
    _validateSwap();
}

ModbusRequest::~ModbusRequest(){
    while(_requestFrame.size()){
        _requestFrame.popLeft();
    }
}

uint8_t ModbusRequest::slaveAddress() const{
    return _slaveAdress;
}

uint16_t ModbusRequest::functionCode() const {
   return  _functionCode;
}

uint16_t ModbusRequest::address() const{
    return _address;
}

 ModbusRequest& ModbusRequest::setThrottle(uint16_t time_){
            _throttle = time_;
            return *this;
            }

void ModbusRequest::setExtension(void * ptr){ 
    _extensionPtr = ptr; 
}

void ModbusRequest::_validateSwap(){
    if (_swap && _registerSize < 2){
        assert(false);
    }
}

uint16_t ModbusRequest::requestSize(){
    return _requestFrame.size();
}

ServerResponse &ModbusRequest::response(){
    return _response;
}

bool ModbusRequest::swap() const{
    return _swap;
}

uint16_t ModbusRequest::registerSize() const{
    return _registerSize;
}

uint16_t ModbusRequest::throttle() const{
    return _throttle;
}

void *ModbusRequest::getExtension(){
    return _extensionPtr;
}

ResponseHandler ModbusRequest::getHandler() const{
    return _handler;
}
