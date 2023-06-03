#if !defined(MODERNBUS_CROSSLINK_H)
#define MODERNBUS_CROSSLINK_H
#include <Arduino.h>
#include <linkedlist.h>

#include "modernbus_provider.h"

class CrossLinkManager;
class CrossLinkStream;
using CrossLinkProvider = SerialProvider<CrossLinkStream>;

class CrossLinkStream{
    friend class CrossLinkManager;
    public:
        ~CrossLinkStream();
        size_t write(uint8_t value);
        uint8_t read();
        size_t available();
        int baudRate();
        bool operator==(const CrossLinkStream& rStream);
    private:
        CrossLinkStream(TinyLinkedList<uint8_t> &buffer, CrossLinkManager &manager);
        TinyLinkedList<uint8_t> &_buffer;
        CrossLinkManager &_manager;
};

class CrossLinkManager{
    friend class CrossLinkStream;
    public:
        CrossLinkManager();
        CrossLinkStream first;
        CrossLinkStream second;
    private:
        TinyLinkedList<uint8_t> bufferA{};
        TinyLinkedList<uint8_t> bufferB{};
        void _informWrite(CrossLinkStream &stream);
};


#endif // MODERNBUS_CROSSLINK_H
