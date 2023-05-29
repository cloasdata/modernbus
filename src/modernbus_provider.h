#ifndef MODBUS_STREAMS_H
#define MODBUS_STREAMS_H


template <typename> class ModbusClient;

template <typename TStream>
class ProviderBase{
    template <typename> friend class ModbusClient;
    public:
        ProviderBase(TStream &stream_)
        : _stream{stream_}
        {};
        virtual int read() = 0;
        virtual size_t write(uint8_t v) = 0;
        virtual size_t available() = 0;
        virtual size_t _calculateTXTime(size_t noOfBytes){return 0;};
        //inform provider transmission is about to start
        virtual void _beginTransmission(){};
        //inform provider about transmission has ben done
        virtual void _endTransmission(){};
        // inform provider that we have not reached the end of the frame but rx is done
        virtual void _informNotComplete(uint16_t bytes){};

    protected:
        TStream &_stream;


};

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
};

template <typename TSerialStream>
class ProviderRS485: public SerialProvider<TSerialStream>{
    template <typename> friend class ModbusClient;
    public:
        ProviderRS485(TSerialStream &provider, int16_t txPin)
        : SerialProvider<TSerialStream>(provider),
            _txPin(txPin)
        {
            pinMode(_txPin, OUTPUT);
        }
        
        int16_t _txPin {0};
        void _beginTransmission() override {
            digitalWrite(_txPin, HIGH);
        };

        void _endTransmission() override {
            digitalWrite(_txPin, LOW);
        };
};

#endif