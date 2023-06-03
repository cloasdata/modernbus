#ifndef MOCK_H
#define MOCK_H
#include <Arduino.h>
#include <linkedlist.h>
#include "../src/modernbus_provider.h"

/*
MockStream
Fakes a stream interface.
User can append several data pointers
For each data ptr a outpuf buffer of buffSize is allocated.
*/
class MockStream{
    struct DataStruct{
        uint8_t *arrPtr{nullptr};
        size_t len{0};
    };

    bool _begin = false;

    DataStruct _currentRead{};
    uint8_t* _readCursor{nullptr};
    uint8_t* _readEnd{nullptr};

    size_t _buffSize{200};
    DataStruct _currentWrite{};

    size_t _read{0};
    uint8_t* _writeCursor{nullptr};
    bool _autoReset{false};
    size_t _resetCount{0};
    TinyLinkedList<DataStruct> readData{};
    TinyLinkedList<DataStruct> writeData{};

    int32_t _readDelay{0};
    int32_t _writeDelay{0};
    
    public:
    MockStream(size_t buffSize = 200)
    :  _buffSize{buffSize}
    {
    };

    ~MockStream(){
        while (writeData.size()){
            auto el = writeData.pop();
            delete [] el.arrPtr;
        }
        while (readData.size()){
            readData.pop();
        }
        readData.~TinyLinkedList();
        writeData.~TinyLinkedList();
    }

    void begin(bool prime=false){
        _begin = true;
        assert(readData.size());
        if (prime){
            nextData();
        } else{
            nextData();
            _readCursor = _readEnd;
        }
    }

    void append(uint8_t *data, size_t len){
        readData.append(DataStruct{data, len});
        writeData.append(
            DataStruct{new uint8_t[_buffSize]}
            );
    }


    void nextData(){
        _currentRead = readData.iter.loopNext();
        _currentWrite = writeData.iter.loopNext();
        reset();
    }

    size_t write(uint8_t v){
        if (_currentWrite.len < _buffSize && _begin){
            _currentWrite.len++;
            *_writeCursor++ = v;
            return 1;
        } else {
            return 0;
        }
    }

    uint8_t read(){
        if (_readDelay){
            delay(_readDelay);
        }
        _read++;
        if (_readCursor != _readEnd){
            return *_readCursor++;
        }
        return 0xFF;
    }

    size_t available(){
        size_t res = _readEnd - _readCursor;
        if ( res == 0){
            if (_autoReset){
                nextData();
            } else {
                reset();
            }

        }
        return res;
    }

    void reset(){
        _resetCount++;
        _readCursor = _currentRead.arrPtr;
        _readEnd = _readCursor + _currentRead.len;
        _writeCursor = _currentWrite.arrPtr;
        _currentWrite.len = 0;
        _read = 0;
    }

    void setAutoReset(bool v){
        _autoReset = v;
    }

    size_t resetCount() const {
        return _resetCount;
    }

    uint8_t* writeBuffer() {
        return _currentWrite.arrPtr;
    }

    size_t bytesWritten() const {
        return _currentWrite.len;
    }

    size_t bytesRead(){
        return _read;
    }

    bool compare(uint8_t *data){
        bool res = true;
        for (size_t i =0; i < bytesWritten(); i++){
            res = res && (_currentWrite.arrPtr[i] == data[i]);
        }
        return res;
    }

    void setReadDelay(int32_t millis_){
        _readDelay = millis_;
    }

};

#endif