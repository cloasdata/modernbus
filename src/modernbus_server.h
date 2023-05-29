#ifndef MODERNBUS_SERVER_H
#define MODERNBUS_SERVER_H

#include <TaskSchedulerDeclarations.h>
#include <mbparser.h>
#include "linkedlist.h"
#include "modernbus_provider.h"
#include "modernbus_util.h"

template <typename>
class ModbusResponse;
template <typename>
class ModbusExceptionResponse;
template <typename>
class ModbusServer;

template <typename T>
using RequestHandler = void(*)(ModbusResponse<T> *response);

template <typename T>
using ExceptionHandler = void(*)(ModbusExceptionResponse<T> *response);

template <typename T>
class ResponseBase{
    public:
        uint8_t functionCode() const {
            return _functionCode;
        }

        uint16_t address() const {
            return _address;
        }

        void sendException(uint8_t exceptionCode){
           _sendException(exceptionCode);
        }


        ResponseBase(uint8_t slaveAddress, uint8_t functionCode, uint16_t address, ModbusServer<T>* server)
        :   _slaveAddress{slaveAddress},
            _functionCode{functionCode},
            _address{address},
            _server{server}
        
        {}

        ResponseBase(uint8_t slaveID, ModbusServer<T>* server)
        :   _slaveAddress{slaveID},
            _server{server}
        
        {}

        virtual ~ResponseBase() = default;


    protected:
        uint8_t _slaveAddress{0};
        uint8_t _functionCode{0};
        uint8_t _byteCount{0};
        uint16_t _address{0};
        uint16_t _crc{0xFFFF};
        ModbusServer<T>* _server{nullptr};
        size_t _size{5};
        
        void _sendHeader(){
            _reset();
            _server->_provider->_beginTransmission();
            _write(_slaveAddress);
            _write(_functionCode);
        }

        void _sendPayload(uint8_t* payload, size_t len){

            for (uint16_t idx=0; idx < len; idx++){
                _write(payload[idx]);
                _size++;
            }
        };

        void _sendCRC(){
            _write(lowByte(_crc), false);
            _write(highByte(_crc), false);
            
            _server->_provider->_endTransmission();
        }

        void _write(uint8_t v, bool crc=true){
            _server->_provider->write(v);
            if (crc) {
                _crc = crc16_update(_crc, v);
            }
        }

        void _reset(){
            _crc = 0xFFFF;
            _size=5;

        }

        void _sendException(uint8_t exceptionCode){
            ResponseBase<T>::_reset();
            ResponseBase<T>::_write(this->_slaveAddress);
            ResponseBase<T>::_write(this->_functionCode + 128);
            ResponseBase<T>::_write(exceptionCode);
            ResponseBase<T>::_sendCRC();
        }

};

template <typename T>
class ModbusResponse: public ResponseBase<T>{

    template <typename>
    friend class ModbusServer;
    
    public:

        /*
        Sends payload by length.
        Length is typical register lenght * requested register quantity.
        Per Register 2 x 8 bit = 16 bit by modbus default.
        Others sizes are possible.
        
        Returns true if function code allows payload (i.e. 01-04)
        */
        bool send(uint8_t* payload, size_t len){
            if (this->_functionCode <= 0x04) {
                this->_sendHeader();
                this->_write(len); // bytecount
                ResponseBase<T>::_sendPayload(payload, len);
                this->_sendCRC();
                return true;
            }
            else {
                return false;
            }
        }

        /*
        Echoes the request from client-
        Typical this is the case for functionCodes 5,6 and 15,16.
        */
        void sendEcho(){
            this->_sendHeader();
            this->_write(highByte(this->_address));
            this->_write(lowByte(this->_address));
            ResponseBase<T>::_sendPayload(_payload, 2);
            this->_sendCRC();
        }

        uint8_t byteCount() const {
            return this->_byteCount;
        }

        uint16_t quantity() const{
            return this->_quantity;
        }

        uint8_t* payload() const {
            return this->_payload;
        }
        
        RequestHandler<T> handler() const {
            return _handler;
        };

        virtual ~ModbusResponse() = default;


    private:
        ModbusResponse(const ModbusResponse&) = delete;
        ModbusResponse(uint8_t slaveID, uint8_t functionCode, uint16_t address, RequestHandler<T> handler, ModbusServer<T>* server)
        :   ResponseBase<T>::ResponseBase{slaveID, functionCode, address, server},
            _handler{handler}
        {}


        RequestHandler<T> _handler{nullptr};
        size_t _size{5};
        uint16_t _quantity{0};
        uint8_t* _payload{nullptr};


        void _update(RequestParser *parser){
            this->_byteCount = parser->byteCount();
            this->_quantity = parser->quantity();
            this->_payload = parser->data();
        }

        void _callHandler(){
            _handler(this);
        }

};

template <typename T>
class ModbusExceptionResponse: public ResponseBase<T>{
    template <typename>
    friend class ModbusServer;

    public:
        void sendException(){
            ResponseBase<T>::_sendException(static_cast<uint8_t>(this->errorCode()));
        }

        ErrorCode errorCode(){
            return _errorCode;
        }

    protected:
        ErrorCode _errorCode{ErrorCode::noError};
        ExceptionHandler<T> _handler{nullptr};

        
        ModbusExceptionResponse(uint8_t slaveID, ExceptionHandler<T> handler, ModbusServer<T>* server)
        :   ResponseBase<T>::ResponseBase{slaveID, server},
            _handler{handler}
        {}

        ModbusExceptionResponse(uint8_t slaveID, ModbusServer<T>* server)
        :   ModbusExceptionResponse{slaveID, nullptr, server}
        {}

        void _callHandler(){
            _handler(this);
        }
};


template <typename T>
class ModbusServer{
    friend class ResponseBase<T>;
    friend class ModbusResponse<T>;
    friend class ModbusExceptionResponse<T>;
    public:
        ModbusServer(Scheduler *scheduler, T *provider, uint8_t myAddress)
        :   _scheduler{scheduler},
            _provider{provider},
            _slaveAddress{myAddress},
            _exceptionResponse{myAddress, this}
        {
            _parser.setOnCompleteCB([this](RequestParser *_){_onComplete();});
            _parser.setOnErrorCB([this](RequestParser *_){_onParserError();});
            _parser.setSlaveAddress(myAddress);
            _scheduler->addTask(_mainTask);
        };

        ~ModbusServer(){_free();};

        /*
        Adds a response to the server. The server will listen and call handler when found.
        */
        ModbusResponse<T>& responseTo(uint8_t functionCode, uint16_t address, RequestHandler<T> handler){
            ModbusResponse<T>* response = new ModbusResponse<T>{
                _slaveAddress,
                functionCode,
                address,
                handler,
                this
            };
            _responses.append(response);
            return *response;
        };

        ModbusExceptionResponse<T>& setOnExceptionHandler(ExceptionHandler<T> handler){
            _exceptionResponse = ModbusExceptionResponse<T>{
                _slaveAddress,
                handler,
                this
            };
            return _exceptionResponse;
        };


        /*
        Removes the response if found. Else returns empty response.
        Transfers ownership/memory management to caller
        */
        ModbusResponse<T>* remove(ModbusResponse<T> *response){
            size_t idx{_responses.index(response)};
            return _responses.remove(idx);
        }


        void start(){
            if (!_isRunning){
                _isRunning = true;
                _mainTask.set(
                    _pollingInterval,
                    TASK_FOREVER,
                    [this](){_retrieveRequest();}
                );
                _mainTask.enable();
            }
        };

        void end(){
            if (_isRunning){
                _isRunning = false;
                _mainTask.disable();
            }
        };

        /*
        Returns server slave adress
        */
        uint8_t slaveAddress() const {
            return _slaveAddress;
            };

        /*
        The intervall in wich the provider is polled for new data
        default: 100 ms
        */
        void setInterval(uint32_t t){
            _pollingInterval = t;
            _mainTask.setInterval(_pollingInterval);
        }

        /*
        Whenever there is a error from the parser, the exception response
        will try to poll as many information as the parser still could parse.
        If not set (default), the exception response will not have such data or are invalid.
        */
        void verboseParser(bool v){
            _verboseParser = v;
        }

        size_t errorCount(){
            return _errorCount;
        }

        RequestParser& getParser(){
            return _parser;
        }

        bool isRunning(){
            return _isRunning;
        }

    protected:
        Scheduler *_scheduler;
        T* _provider;
        RequestParser _parser{};
        uint8_t _slaveAddress{};
        
        Task _mainTask{};
        uint32_t _pollingInterval{100};

        uint16_t _errorCount{};

        bool _isRunning {false};

        bool _verboseParser{false};

        TinyLinkedList<ModbusResponse<T>*> _responses{};
        ModbusExceptionResponse<T> _exceptionResponse{};
        
        /*
        This method polls the provider for data
        The polling is driven by the scheduler every intervall.
        */
        void _retrieveRequest(){
            Serial.printf("Retrieving:\n");
            // while provider could deliver more than just frame we additional check 
            while (_provider->available()){
                uint8_t msg = _provider->read();
                Serial.printf("Server: S: %d, T: %X Size: %d\n", (int)_parser.state(), msg, _provider->available());
                _parser.parse(msg);
                Serial.printf("Server: S: %d\n", (int)_parser.state());
            }
            Serial.printf("Server Complete with state: %d\n", (int)_parser.state());
            // inform provider that we have not reached the end of the frame
            if (_parser.state() != ParserState::slaveAddress && !_parser.isComplete()){
                _provider->_informNotComplete(_parser.dataToReceive());
                _parser.reset();
            }
        }

        void _onComplete(){
            // the parser does not recognize if a function code or address is wrong
            // so the server needs to throw an exception in that case
            ModbusResponse<T>* response{_findResponse()};
            if (response){
                response->_update(&_parser);
                response->_callHandler();
                // as long tx we do not need poll
                size_t txDelay = _provider->_calculateTXTime(response->_size);
                _mainTask.delay(txDelay);
            } else {
                _onServerError();
            }
        }

        ModbusResponse<T>* _findResponse(){
            ModbusResponse<T>* el{nullptr};
            _responses.iter.reset();
            while (_responses.iter()){
                el = _responses.iter.next();
                
                if (el->functionCode() == _parser.functionCode()){
                    
                    if (el->address() == _parser.address()){  
                        return el;
                    } else {
                        _exceptionResponse._errorCode = ErrorCode::illegalDataAddress;
                    }
                } else {
                    _exceptionResponse._errorCode = ErrorCode::illegalFunction;
                }

            }
            _exceptionResponse._functionCode = _parser.functionCode();
            _exceptionResponse._address = _parser.address();
            return nullptr;
        }

        void _onServerError(){
            if (_exceptionResponse._handler){
                _exceptionResponse._callHandler();
            } else {
                _exceptionResponse.sendException();
            }
        }

        /*
        handles all kind of parser errors
        */
        void _onParserError(){
            _errorCount++;
            if (_verboseParser){
                _exceptionResponse._functionCode = _parser.functionCode();
                _exceptionResponse._address = _parser.address();
                _exceptionResponse._errorCode = _parser.errorCode();
                _onServerError();
            }
        }


        void _free(){
            end();
            _responses.iter.reset();
            while(_responses.iter()){
                auto r = _responses.iter.next();
                delete r;
            }
        }   
};

#endif