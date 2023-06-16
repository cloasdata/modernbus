#ifndef MODBUS_STREAMS_H
#define MODBUS_STREAMS_H

#ifndef MIN_TX_TIME
#define MIN_TX_TIME 1
#endif

template <typename> class ModbusClient;

/*
Abstract Base class of a provide used by modbus client and server.
*/
template <typename TStream>
class ProviderBase{
    template <typename> friend class ModbusClient;
    public:
        ProviderBase(TStream &stream_)
        : _stream{stream_}
        {};
        // read one byte from stream
        virtual int read() = 0;
        // write one byte to stream
        virtual size_t write(uint8_t v) = 0;
        // check if there is something in the buffer
        virtual size_t available() = 0;
        // Estimate the total time for transmitting the given bytes.
        virtual uint8_t _calculateTXTime(uint8_t noOfBytes){return 0;};
        // inform provider transmission is about to start
        virtual void _beginTransmission(){};
        // inform provider about transmission has been done
        virtual void _endTransmission(){};
        // inform provider that we have not reached the end of the frame but rx is done
        virtual void _informNotComplete(uint16_t bytes){};

    protected:
        TStream &_stream;


};

/*
A simple serial (RS232/UART) provider type.

Can be user for example with SoftSerial or HardwareSerial 
from Arduino framework.

The stream most provide following interface:
    - int read()
    - size_t write(uint8_t v)
    - size_t available()
    - int baudRate()
*/
template <typename TSerialStream>
class SerialProvider: public ProviderBase<TSerialStream>{
    template <typename> friend class ModbusClient;
    public:
        SerialProvider(TSerialStream &stream_)
        : ProviderBase<TSerialStream>(stream_)
        {};

        int read() override {
            return this->_stream.read();
        };

        size_t write(uint8_t v) override {
            return this->_stream.write(v);
        }

        size_t available() override {
            return this->_stream.available();
        }

        uint8_t _calculateTXTime(uint8_t noOfBytes) override {
            uint16_t bitsTx = 10 * noOfBytes; // 10 bits per byte to send 0,5 + 8 + 1 + 0,5
            return (bitsTx * 1000) / this->_stream.baudRate();
        }
};

/*
A RS485 Serial provider using a tx pin.
*/
template <typename TSerialStream>
class ProviderRS485: public SerialProvider<TSerialStream>{
    template <typename> friend class ModbusClient;
     public:
        uint8_t txPin;

        ProviderRS485(TSerialStream &provider, uint8_t txPin)
        : SerialProvider<TSerialStream>(provider),
            txPin(txPin)
        {
            pinMode(txPin, OUTPUT);
        }
        
        
        void _beginTransmission() override {
            digitalWrite(txPin, HIGH);
        };

        void _endTransmission() override {
            digitalWrite(txPin, !digitalRead(txPin));
        };
};

#endif