#ifndef modbus_client_h
#define modbus_client_h

//#undef STD_FUNCTIONAL

#ifdef STD_FUNCTIONAL
    #include <functional>
#endif

#include <Arduino.h>

#include <TaskSchedulerDeclarations.h>

#include "modernbus_request.h"
#include "modernbus_provider.h"
#include "mbparser.h"
#include "linkedlist.h"
#include "modernbus_util.h"
#include "modernbus_server_response.h"

// protos
class ServerResponse;
class ModbusRequest;
template <typename T> class ModbusClient;
void removeWhitespace(char *s);
uint16_t crc16_update(uint16_t crc, uint8_t a);

#ifndef STD_FUNCTIONAL
    // parser callback function protos
    template <typename U>
    void _parserComplete(ResponseParser *parser);

    template <typename U>
    void _parserError(ResponseParser *parser);
#endif


/*
Implements a simple modbus client which is reading holding register,
from a RTU slave.
*/
template <typename T>
class ModbusClient{
    
    #ifndef STD_FUNCTIONAL
        friend void _parserComplete<T>(ResponseParser *parser);
        friend void _parserError<T>(ResponseParser *parser);
    #endif
    
    public:   
        ModbusClient(Scheduler *scheduler, T *provider)
            :   _provider{provider},
                _scheduler{scheduler}
        {   
            _scheduler->addTask(_mainTask);
            _parser.setSlaveAddress(0);
            // Set callbacks
            #ifdef STD_FUNCTIONAL
                _parser.setOnCompleteCB([this](ResponseParser *parser){_parserComplete();});
                _parser.setOnErrorCB([this](ResponseParser *parser){_parserError();});
            #else
                _parser.setExtension(this);
                _parser.setOnCompleteCB(_parserComplete<T>);
                _parser.setOnErrorCB(_parserError<T>);
            #endif
        }

        ~ModbusClient(){
            _free();
            
            };

        /*
        Appends a request instance to the client.
        Client requests data periodical in order of submission.
        */
        void append(ModbusRequest *request){
            _requests.append(request);
        };

        /*
        Removes the appended request. Will return a empty request if not found.
        */
        ModbusRequest* remove(ModbusRequest *request){
            int16_t idx = _requests.index(request);
            return _requests.remove(idx);
        };

        /*
        Periodical poll the provided request.
        The server takes a copy of the provided request, so that user does not need to manage the array further.
        User can modify the request, by receiving the returned reference.

        The passed request array is not validated for fullfilling any modbus standard.
        So user could also implement its very own frame.

        Supported Functioncodes are:
        01, 02, 03, 04, 05, 06, 15, 16
        */ 
        ModbusRequest& poll(uint8_t* request, uint16_t requestSize, bool swap, uint16_t registerSize, ResponseHandler handler){
            // base method
            auto *mbRequest{new ModbusRequest{request, requestSize, swap, registerSize, handler}};
            append(mbRequest);
            return *mbRequest;
        }
        
        ModbusRequest& poll(uint8_t* request, uint16_t requestSize, ResponseHandler handler){
            return poll(request, requestSize, false, 0, handler);
        }
        

        
        
        /*
        Queues a single request and sends the request as soon as possible
        */
        void send(ModbusRequest *request){
            _singleRequestQueue.append(request);
        }
        
        /*
        Starts the client.
        Must be called at least once

        Client waits until requests are added.
        Assumes that Stream provider is running. 
        */
        void start()
        {   
        if (!_isRunning){
            _isRunning = true;
            _mainTask.set(
                TASK_IMMEDIATE, 
                TASK_FOREVER, 
                [this](){this->_dispatchRequest();}
                );
            _mainTask.enable();
        }
        };

        /*
        Stops the client.
        
        Keeps state
        Can be restarted with call of start
        */
        void stop()
        {
            if (_isRunning){
                _isRunning = false;
                _mainTask.disable();
            }
        };

        /*
        Frees all requests and stops client
        */
        void reset()
        {
            stop();
            _free();
        };

        // setter
        /*
        Sets minimal dealy between requests
        */
        void setMinDelay(uint16_t millis_){_minDelay=millis_;};
        
        /*
        Sets maximum data to receive. Avoids heap corruption etc.
        Default 96 bytes.
        */
        void setDataLimit(size_t limit){_parser.setByteCountLimit(limit);};

        /*
        Default is no error handler.
        If set each error will be handled with this handler.
        The handler is called when a valid request is made.
        It wont be called as long there are now requests.
        */
        void setOnError(ErrorHandler handler){
            _onError = handler;
        }

        /*
        Sets function code validation.
        When true then the server response is checked against the client request
        If functioncode does not match it will call the error handler if available.
        */
       void setFunctionCodeValidation(bool v){
            _needsValidation = v;
       }

        // properties

        ResponseParser& getParser(){return _parser;};
        bool isRunning(){return _isRunning;};
        uint32_t errorCount(){return _errorCount;};
        uint32_t requestCount(){return _requestCount;};
        uint32_t completeCount(){return _completeCount;};
        size_t dataLimit(){return _parser.byteCountLimit();};
 
    private:
        T *_provider;
        Scheduler *_scheduler;
        Task _mainTask;
        ResponseParser _parser;

        ModbusRequest *_currentRequest = nullptr;
        
        TinyLinkedList<ModbusRequest*> _requests;
        ModbusRequest *_singleRequest = nullptr;
        TinyLinkedList<ModbusRequest*> _singleRequestQueue;

        ErrorHandler _onError = nullptr;
        
        uint32_t _requestCount{0};
        uint32_t _completeCount{0};
        uint32_t _errorCount{0};
        ErrorCode _lastError{ErrorCode::noError};
        bool _isRunning = false;
        unsigned long _lastTX{0};

        uint16_t _deviceDelay{200};
        uint16_t _minDelay{40};

        bool _needsValidation;

        //mem
        void _free()
        {   
            while(_requests.size()){
                ModbusRequest *r = _requests.popLeft();
                delete r;
            }
        };

        // Modbus implementation

        void _beginTransmission()
        {   
            while(_provider->available()) _provider->read(); // clean buffer
            _lastTX = millis();
            _provider->_beginTransmission();

            _mainTask.setCallback(
                [this](){ _transmitRequest();}
                );
            _mainTask.delay(_provider->_calculateTXTime(1)); // delay by one byte
        };

        void _transmitRequest()
        {   
            while(_currentRequest->_requestFrame.iter()){
                uint8_t token = _currentRequest->_requestFrame.iter.next();
                _provider->write(token);
            }
            _requestCount++;
            _mainTask.setCallback(
                [this](){ _endTransmission(); }
                );

            // delay until all bytes send
            _mainTask.delay(_provider->_calculateTXTime(9)); // request frame have 8 bytes + 1 silent

        };

        void _endTransmission(){
            _provider->_endTransmission();
            // calc delay
            uint16_t framesize = _calcFrameSize(_currentRequest->requestSize(), 2);
            uint16_t delayBy = _provider->_calculateTXTime(framesize);
            delayBy += _deviceDelay;

            _mainTask.setCallback(
                [this](){ _retrieveResponse(); }
                );
            _mainTask.delay(delayBy);
            _lastTX = millis();
        };


        void _retrieveResponse()
        {   
            _parser.setSlaveAddress(_currentRequest->slaveAddress());
            while (!_parser.isComplete() && !_parser.isError() && _provider->available()){
                uint8_t token = _provider->read();
                _parser.parse(token);
                Serial.printf("Client Retrieve: %X, State: %d\n",token, (int)_parser.state());
            }

            if (!_parser.isComplete()){
                // Inform provider provider
                _provider->_informNotComplete(_parser.dataToReceive());
            }
            // next request. We do not wait for anything else.
            _mainTask.setCallback(
                [this](){ _dispatchRequest(); }
                );
                //_mainTask.forceNextIteration();
        };
        
        // Request handling

        void _dispatchRequest()
        {
            // phase in a single request. Once request is done delete the request
            if (_singleRequestQueue.size()){
                _currentRequest = _singleRequestQueue.pop();
                _doRequest();
            } else if (_requests.size()){ 
                _currentRequest = _requests.iter.loopNext();
                _doRequest();
                return;
            } else {
                _waitUntilRequest();
            }
        };


        void _doRequest()
        {
            _setupParser();
            _mainTask.setCallback([this]()
                                { _beginTransmission(); });
            //_mainTask.forceNextIteration();
            _mainTask.delay(_currentRequest->throttle());
        };
        
        
        void _waitUntilRequest()
        {
            _mainTask.setCallback([this](){_dispatchRequest();});
            _mainTask.delay(100);
        };

        // parser

        void _setupParser()
        {
            _parser.setSlaveAddress(_currentRequest->slaveAddress());
            _parser.setSwap(_currentRequest->swap());
            _parser.setRegisterSize(_currentRequest->registerSize());
            _parser.reset();
        };

        #ifdef STD_FUNCTIONAL
            // parser callback function
            void _parserComplete()
            {   
                if (!_isFCOkay()){

                    _parserError(ErrorCode::illegalFunction);
                    return;
                }
                _completeCount++;
                // this makes sure that user will only receive valid data
                ServerResponse &response = _currentRequest->response();
                response._slaveAdress = _parser.slaveAddress();
                response._functionCode = _parser.functionCode();
                response._byteCount = _parser.byteCount();
                response._address = _parser.address();
                response._payload = _parser.data();
                _currentRequest->_handler(&response);
                _parser.free();
            };

            void _parserError(ErrorCode error=ErrorCode::noError)
            {
                _errorCount++;
                _lastError = error != ErrorCode::noError ? error : _parser.errorCode();
                if ( _onError != nullptr){
                    _onError(&_currentRequest->response(), _lastError);
                }
                _parser.free();   
            };
        #endif

        bool _isFCOkay(){
            if (_needsValidation){
                return _currentRequest->functionCode() == _parser.functionCode();
            }
            return true;
        }

        uint16_t _calcFrameSize(uint16_t noOfRegister, uint16_t registerSize)
        {
            return 2 + noOfRegister * registerSize + 2;
        };

};

#ifndef STD_FUNCTIONAL
    template <typename U>
    void _parserComplete(ResponseParser *parser)
    {
        ModbusClient<U>* client {static_cast<ModbusClient<U>*>(parser->getExtension())};
        if (not client){
            return;
        }
        client->_completeCount++;
        client->_currentRequest->_sizePayload = parser->byteCount();
        // this makes sure that user will only receive valid data
        client->_currentRequest->_payload = parser->payload();
        client->_currentRequest->payload();
        client->_currentRequest->_handler(client->_currentRequest);
        parser->free();
    };

    template <typename U>
    void _parserError(ResponseParser *parser)
    {   
        ModbusClient<U>* client {static_cast<ModbusClient<U>*>(parser->getExtension())};
        client->_errorCount++;
        client->_lastError = parser->errorCode();
        if ( client->_onError != nullptr){
            client->_onError(client->_currentRequest, client->_lastError);
        }
        parser->free();   
    };
#endif

#endif