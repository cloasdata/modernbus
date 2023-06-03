#include "modernbus_crosslink.h"


CrossLinkStream::CrossLinkStream(TinyLinkedList<uint8_t> &buffer, CrossLinkManager &manager)
:   _buffer{buffer},
    _manager{manager}
{};

CrossLinkStream::~CrossLinkStream(){
    while (_buffer.size()){
        _buffer.pop();
    }
}

size_t CrossLinkStream::write(uint8_t value){
    _buffer.append(value);
    _manager._informWrite(*this);
    return 1;
}


uint8_t CrossLinkStream::read(){
    auto data = _buffer.popLeft();
    return data;
}

size_t CrossLinkStream::available(){
    return _buffer.size();
}

int CrossLinkStream::baudRate()
{
    return 1152000;
}

bool CrossLinkStream::operator==(const CrossLinkStream &rStream)
{
    return &_buffer == &rStream._buffer;
}

CrossLinkManager::CrossLinkManager()
: first{bufferA, *this},
second{bufferB, *this}
{
}

void CrossLinkManager::_informWrite(CrossLinkStream &stream)
{
    if(stream == first){
        while(bufferA.size()){
            bufferB.append(bufferA.popLeft());
        }
    } else {
        while(bufferB.size()){
            bufferA.append(bufferB.popLeft());
        }
    }
}
