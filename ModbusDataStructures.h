#ifndef H_ModbusDataStructures_IP
#define H_ModbusDataStructures_IP

#include <cstdint>
#include <byteManipulation.h>
#include <cstring>

enum ModbusError : uint8_t
{
    NoError,
    IllegalFunction,
    IllegalDataAddress,
    IllegalDataValue,
    SlaveDeviceFailure,
    SlaveDeviceBusy,
};

enum ModbusFunction : uint8_t
{
    ReadCoils = 1,
    ReadDiscreteInputs,
    ReadHoldingRegisters,
    ReadInputRegisters,
    WriteSingleCoil,
    WriteSingleHoldingRegister,
    WriteMultipleCoils = 15,
    WriteMultipleHoldingRegisters,
};

struct ModbusRequestPDU
{
    ModbusFunction FunctionCode;
    uint16_t Address;
    uint16_t NumberOfRegisters;
    uint16_t RegisterValue;
    uint8_t DataByteCount;
    uint8_t *Values;
};

ModbusRequestPDU ParseRequestPDU(uint8_t *data)
{
    return ModbusRequestPDU{
        .FunctionCode = static_cast<ModbusFunction>(data[0]),
        .Address = CombineBytes(data[1], data[2]),
        .NumberOfRegisters = CombineBytes(data[3], data[4]),
        .RegisterValue = CombineBytes(data[3], data[4]),
        .DataByteCount = data[5],
        .Values = data + 6};
}

void getRequestBytes(ModbusRequestPDU PDU, uint8_t *bytesBuffer) // Primarily for testing, not normally used
{
    bytesBuffer[0] = PDU.FunctionCode;
    SplitBytes(PDU.Address, Big, bytesBuffer + 1);
    if (PDU.FunctionCode == ModbusFunction::WriteSingleCoil || PDU.FunctionCode == ModbusFunction::WriteSingleHoldingRegister)
    {
        SplitBytes(PDU.RegisterValue, Big, bytesBuffer + 3);
    }
    else
    {
        SplitBytes(PDU.NumberOfRegisters, Big, bytesBuffer + 3);
    }
    bytesBuffer[5] = PDU.DataByteCount;
    memcpy(bytesBuffer + 6, PDU.Values, PDU.DataByteCount);
}

struct ModbusResponsePDU
{
    ModbusFunction FunctionCode;
    // uint16_t Address; // Unchanged from request PDU
    uint8_t DataByteCount;
    // uint16_t NumberOfRegistersChanged; // Unchanged from request PDU
    uint8_t *RegisterValue;
    ModbusError Error = NoError;
};

// DataBuffer must be the beginning of the Request PDU as this relies on reusing data that will be unchanged. Returns response length for the MBAP header
uint8_t ModbusResponsePDUtoStream(const ModbusResponsePDU responseData, uint8_t *DataBuffer)
{
    if (responseData.Error != NoError)
    {
        DataBuffer[0] |= 0b10000000; // flip first bit of function code
        DataBuffer[1] = responseData.Error;
        return 2;
    }

    if (responseData.FunctionCode <= 4)
    {
        DataBuffer[1] = responseData.DataByteCount;
        std::memcpy(DataBuffer + 2,
                    responseData.RegisterValue,
                    responseData.DataByteCount);
        return responseData.DataByteCount + 2;
    }

    return 5;
}

struct MBAPHead
{
    uint16_t TransactionID;
    uint16_t ProtocolID;
    uint16_t Length;
    uint8_t UnitID;
};

MBAPHead MBAPfromBytes(const uint8_t header[7])
{
    return MBAPHead{.TransactionID = CombineBytes(header[0], header[1]),
                    .ProtocolID = CombineBytes(header[2], header[3]),
                    .Length = CombineBytes(header[4], header[5]),
                    .UnitID = header[6]};
}

void getMBAPBytes(const MBAPHead MBAPHeader, uint8_t *bytes) // Primarily for testing, not normally used
{
    SplitBytes(MBAPHeader.TransactionID, Big, bytes);
    SplitBytes(MBAPHeader.ProtocolID, Big, bytes + 2);
    SplitBytes(MBAPHeader.Length, Big, bytes + 4);
    bytes[6] = MBAPHeader.UnitID;
}

#endif