#ifndef H_ModbusDataStructures_IP
#define H_ModbusDataStructures_IP

#include <byteManipulation.h>
#include <stdint.h> // uintX_t
#include <string.h> // memcpy()

#ifdef ModbusRTU
#include <FastCRC.h> // For RTU capability https://github.com/FrankBoesing/FastCRC, PlatformIO: frankboesing/FastCRC @ ^1.41
#endif

uint8_t CompressBoolean(uint8_t b[8], uint8_t limit = 8);
bool CRC16Check(uint8_t *data, uint8_t byteCount);

enum ModbusError : uint8_t
{
    NoError,
    IllegalFunction,
    IllegalDataAddress,
    IllegalDataValue,
    SlaveDeviceFailure,
    SlaveDeviceBusy,
    CRCError = 12,
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

void getRequestBytes(ModbusRequestPDU PDU, uint8_t *bytesBuffer) // Primarily for testing, not normally used //TODO consider implementing WriteCoils Byte compression
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

uint8_t getRequestByteLength(ModbusRequestPDU PDU)
{
    return 5 + PDU.DataByteCount;
}

struct ModbusResponsePDU
{
    ModbusFunction FunctionCode = ModbusFunction::ReadCoils;
    uint16_t Address = 0; // Unchanged from request PDU
    uint8_t DataByteCount = 0;
    uint16_t NumberOfRegistersChanged = 0; // Unchanged from request PDU
    uint8_t *RegisterValue = nullptr;
    ModbusError Error = NoError;
};

ModbusResponsePDU CreateErroredResponse(ModbusError Error)
{
    return ModbusResponsePDU{.FunctionCode = ModbusFunction::ReadCoils,
                             .Address = 0,
                             .DataByteCount = 0,
                             .NumberOfRegistersChanged = 0,
                             .RegisterValue = 0,
                             .Error = Error};
}

ModbusResponsePDU ParseResponsePDU(uint8_t *data, uint8_t byteCount)
{
    if (!CRC16Check(data, byteCount))
    {
        return CreateErroredResponse(ModbusError::CRCError);
    }
    else if (data[0] & 0b10000000)
    {
        return CreateErroredResponse(static_cast<ModbusError>(data[1]));
    };

    ModbusFunction code = static_cast<ModbusFunction>(data[0]);
    switch (code)
    {
    case ModbusFunction::ReadCoils:
    case ModbusFunction::ReadDiscreteInputs:
    case ModbusFunction::ReadHoldingRegisters:
    case ModbusFunction::ReadInputRegisters:
        return ModbusResponsePDU{
            .FunctionCode = code,
            .Address = 0,
            .DataByteCount = data[1],
            .NumberOfRegistersChanged = 0,
            .RegisterValue = data + 2,
            .Error = NoError};
        break;

    case ModbusFunction::WriteSingleCoil:
    case ModbusFunction::WriteSingleHoldingRegister:
        return ModbusResponsePDU{
            .FunctionCode = code,
            .Address = CombineBytes(data[1], data[2]),
            .DataByteCount = 0,
            .NumberOfRegistersChanged = 1,
            .RegisterValue = data + 3,
            .Error = NoError};
        break;

    case ModbusFunction::WriteMultipleCoils:
    case ModbusFunction::WriteMultipleHoldingRegisters:
        return ModbusResponsePDU{
            .FunctionCode = code,
            .Address = CombineBytes(data[1], data[2]),
            .DataByteCount = 0,
            .NumberOfRegistersChanged = CombineBytes(data[3], data[4]),
            .RegisterValue = 0,
            .Error = NoError};
        break;

    default:
        return CreateErroredResponse(ModbusError::IllegalFunction);
        break;
    }
}

// DataBuffer must be the beginning of the Request PDU as this relies on reusing data that will be unchanged. Returns response length for the MBAP header
uint8_t
ModbusResponsePDUtoStream(const ModbusResponsePDU responseData, uint8_t *DataBuffer)
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
        memcpy(DataBuffer + 2,
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

bool CRC16Check(uint8_t *data, uint8_t byteCount)
{
    FastCRC16 CRC16;
    return (CRC16.modbus(data, byteCount - 2) == CombineBytes(data[byteCount - 1], data[byteCount - 2]));
}

uint8_t CompressBoolean(uint8_t b[8], uint8_t limit = 8)
{
    uint8_t c = 0;
    for (int i = 0; i < limit; i++)
        if (b[i])
            c |= (1 << i);
    return c;
}
#endif