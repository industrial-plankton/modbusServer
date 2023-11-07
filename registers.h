#ifndef H_ModBusRegisters_IP
#define H_ModBusRegisters_IP

#ifdef __AVR__
#include <Array.h>    //https://github.com/janelia-arduino/Array
#include <Vector.h>   //https://github.com/janelia-arduino/Vector
#define array Array   // Make code invariant
#define vector Vector // Make code invariant
#else
#include <vector>
#include <array>
using std::array;
using std::vector;
#endif

#include <FastCRC.h> // For RTU capability https://github.com/FrankBoesing/FastCRC, PlatformIO: frankboesing/FastCRC @ ^1.41
#include <ModbusDataStructures.h>

class Register
{
protected:
    const uint16_t FirstAddress;

private:
    const uint16_t LastAddress;
    const vector<ModbusFunction> FunctionList;

public:
    Register(uint16_t FirstAddress, uint16_t LastAddress, vector<ModbusFunction> FunctionList)
        : FirstAddress{FirstAddress}, LastAddress{LastAddress}, FunctionList{FunctionList} {};
    ~Register(){};

    virtual uint8_t *getDataLocation(const uint16_t Address) const = 0;
    virtual uint8_t getResponseByteCount(const uint8_t RegistersCount) const = 0;
    virtual void Write(const uint16_t Address, const uint8_t RegistersCount, const uint8_t *dataBuffer) = 0;
    virtual void WriteSingle(const uint16_t Address, const uint16_t value) = 0;

    bool AddressInRange(const uint16_t address) const
    {
        return (FirstAddress <= address) && (address <= LastAddress);
    }
    bool ValidFunctionCode(const ModbusFunction FunctionCode) const
    {
        for (auto func : FunctionList)
        {
            if (func == FunctionCode)
            {
                return true;
            }
        }
        return false;
    }
};

class CoilRegister : public Register
{
private:
    uint8_t *data;

public:
    CoilRegister(uint16_t FirstAddress, uint16_t LastAddress, vector<ModbusFunction> FunctionList, uint8_t *data)
        : Register(FirstAddress, LastAddress, FunctionList), data{data} {};
    ~CoilRegister(){};

    uint8_t *getDataLocation(const uint16_t Address) const override
    {
        return data + (Address - FirstAddress);
    }
    uint8_t getResponseByteCount(const uint8_t RegistersCount) const override
    {
        return RegistersCount * sizeof(data[0]);
    }
    void Write(const uint16_t Address, const uint8_t RegistersCount, const uint8_t *dataBuffer) override
    {
        memcpy(data + (Address - FirstAddress),
               dataBuffer,
               getResponseByteCount(RegistersCount));
    }
    void WriteSingle(const uint16_t Address, const uint16_t value) override
    {
        data[Address - FirstAddress] = static_cast<uint8_t>(value);
    }
};

class HoldingRegister : public Register
{
private:
    uint16_t *data;
    const bool ReceiveBigEndian;

public:
    HoldingRegister(uint16_t FirstAddress, uint16_t LastAddress, vector<ModbusFunction> FunctionList, uint16_t *data, bool ReceiveBigEndian)
        : Register(FirstAddress, LastAddress, FunctionList), data{data}, ReceiveBigEndian{ReceiveBigEndian} {};
    HoldingRegister(uint16_t FirstAddress, uint16_t LastAddress, vector<ModbusFunction> FunctionList, uint16_t *data)
        : HoldingRegister(FirstAddress, LastAddress, FunctionList, data, true){};
    ~HoldingRegister(){};

    uint8_t *getDataLocation(const uint16_t Address) const override
    {
        return reinterpret_cast<uint8_t *>(data + (Address - FirstAddress));
    }
    uint8_t getResponseByteCount(const uint8_t RegistersCount) const override
    {
        return RegistersCount * sizeof(data[0]);
    }
    void Write(const uint16_t Address, const uint8_t RegistersCount, const uint8_t *dataBuffer) override
    {
        if (ReceiveBigEndian)
        {
            uint16_t *dataBuffer16 = (uint16_t *)dataBuffer;
            for (size_t i = 0; i < getResponseByteCount(RegistersCount) / 2; i++)
            {
                dataBuffer16[i] = byteSwap(dataBuffer16[i]);
            }
        }
        memcpy(data + (Address - FirstAddress),
               dataBuffer,
               getResponseByteCount(RegistersCount));
    }
    void WriteSingle(const uint16_t Address, const uint16_t value) override
    {
        data[Address - FirstAddress] = ReceiveBigEndian ? byteSwap(value) : value;
    }
};

class Registers
{
private:
    const vector<Register *> RegisterList;
    Register *getRegister(const ModbusRequestPDU PDU) const
    {
        for (Register *reg : RegisterList)
        {
            if (reg->ValidFunctionCode(PDU.FunctionCode) && reg->AddressInRange(PDU.Address))
            {
                return reg;
            }
        }

        return nullptr;
    }
    bool ValidFunctionCode(const ModbusFunction FunctionCode) const
    {
        for (Register *reg : RegisterList)
        {
            if (reg->ValidFunctionCode(FunctionCode))
            {
                return true;
            }
        }
        return false;
    }

    bool ValidAddress(const ModbusFunction FunctionCode, const uint16_t address) const
    {
        for (Register *reg : RegisterList)
        {
            if (reg->ValidFunctionCode(FunctionCode) && reg->AddressInRange(address))
            {
                return true;
            }
        }
        return false;
    }

    ModbusResponsePDU getErrorCode(const ModbusRequestPDU PDU) const
    {
        ModbusResponsePDU response = {.FunctionCode = PDU.FunctionCode};
        if (!ValidFunctionCode(PDU.FunctionCode))
        {
            // printf("IllegalFunction error on address: %u, and func code: %u\n", PDU.Address, (uint8_t)PDU.FunctionCode);
            response.Error = ModbusError::IllegalFunction;
        }
        else if (!ValidAddress(PDU.FunctionCode, PDU.Address))
        {
            // printf("IllegalDataAddress error on address: %u, and func code: %u\n", PDU.Address, (uint8_t)PDU.FunctionCode);
            response.Error = ModbusError::IllegalDataAddress;
        }
        else
        {
            // printf("SlaveDeviceFailure error on address: %u, and func code: %u\n", PDU.Address, (uint8_t)PDU.FunctionCode);
            response.Error = ModbusError::SlaveDeviceFailure;
        }
        return response;
    }

public:
    explicit Registers(vector<Register *> RegisterList) : RegisterList{RegisterList} {};
    ~Registers(){};
    ModbusResponsePDU ProcessRequest(ModbusRequestPDU PDU)
    {
        Register *reg = getRegister(PDU);
        if (reg == nullptr)
        {
            // printf("No valid register found for address: %u, and func code: %u\n", PDU.Address, (uint8_t)PDU.FunctionCode);
            return getErrorCode(PDU);
        }

        ModbusResponsePDU response = {.FunctionCode = PDU.FunctionCode};
        switch (PDU.FunctionCode)
        {
        case ModbusFunction::ReadCoils:
        case ModbusFunction::ReadDiscreteInputs:
        case ModbusFunction::ReadHoldingRegisters:
        case ModbusFunction::ReadInputRegisters:
            response.RegisterValue = reg->getDataLocation(PDU.Address);
            response.DataByteCount = reg->getResponseByteCount(PDU.NumberOfRegisters);
            break;
        case ModbusFunction::WriteSingleCoil:
        case ModbusFunction::WriteSingleHoldingRegister:
            reg->WriteSingle(PDU.Address, PDU.RegisterValue);
            break;
        case ModbusFunction::WriteMultipleCoils:
        case ModbusFunction::WriteMultipleHoldingRegisters:
            reg->Write(PDU.Address, PDU.NumberOfRegisters, PDU.Values);
            break;
        default:
            // printf("IllegalFunction address: %u, and func code: %u", PDU.Address, (uint8_t)PDU.FunctionCode);
            response.Error = ModbusError::IllegalFunction;
        }
        return response;
    }
    uint8_t ProcessStream(uint8_t *ModbusFrame)
    {
        return ModbusResponsePDUtoStream(this->ProcessRequest(ParseRequestPDU(ModbusFrame)), ModbusFrame);
    }
};

template <size_t BufferSize>
size_t ReceiveTCPStream(Registers &registers, array<uint8_t, BufferSize> &ModbusFrame, const uint16_t byteCount)
{
    if (byteCount <= 7 || byteCount > BufferSize)
    {
        return 0;
    }

    const MBAPHead header = MBAPfromBytes(ModbusFrame.data());
    if (header.ProtocolID != 0 || header.Length + 6 > byteCount)
    {
        return 0;
    }

    const auto size = registers.ProcessStream(ModbusFrame.data() + 7);
    ModbusFrame[5] = size + 1;
    return 7 + size;
}

template <size_t BufferSize>
size_t ReceiveRTUStream(Registers &registers, array<uint8_t, BufferSize> &ModbusFrame, const uint8_t byteCount)
{
    if (byteCount <= 7 || byteCount > BufferSize)
    {
        return 0;
    }

    FastCRC16 CRC16;
    if (CRC16.modbus(ModbusFrame.data(), byteCount - 2) !=
        CombineBytes(ModbusFrame.data()[byteCount - 1], ModbusFrame.data()[byteCount - 2]))
    {
        return 0;
    }

    const auto size = registers.ProcessStream(ModbusFrame.data() + 1) + 1;
    ((uint16_t *)(ModbusFrame.data() + size))[0] = CRC16.modbus(ModbusFrame.data(), size);

    return size + 2;
}

#endif