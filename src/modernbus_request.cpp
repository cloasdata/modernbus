#include "modernbus_request.h"
#include "modernbus_server_response.h"

ModbusRequest::ModbusRequest(uint8_t *request, uint16_t requestSize, bool swap, uint16_t registerSize, ResponseHandler handler)
:   _requestFrame{request, requestSize},
    _swap{swap},
    _registerSize{registerSize},
    _handler{handler},
    _response{this},
    _slaveAddress{request[0]},
    _functionCode{request[1]},
    _address{(uint16_t)((request[2] << 8) | request[3])}
{
    _validateSwap();
    _determineQuantity();
}

ModbusRequest::~ModbusRequest(){
    while(_requestFrame.size()){
        _requestFrame.popLeft();
    }
}

uint8_t ModbusRequest::slaveAddress() const{
    return _slaveAddress;
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

ModbusRequest& ModbusRequest::setTimeout(uint32_t time)
{
    _timeOut = time;
    return *this;
}

 ModbusRequest& ModbusRequest::setDeviceDelay(uint16_t millis_)
{
    _deviceDelay = millis_;
    return *this;
}

void ModbusRequest::_validateSwap(){
    if (_swap && _registerSize < 2){
        assert(false);
    }
}

void ModbusRequest::_determineQuantity()
{
    if (_functionCode < 5 || _functionCode > 6){
        _registerQuantity = _requestFrame.get(4) << 8 | _requestFrame.get(5);
    } else {
        _registerQuantity = 1;
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

uint16_t ModbusRequest::deviceDelay() const
{
    return _deviceDelay;
}
