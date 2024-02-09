#ifndef H_ModbusDataStructures_IP
#define H_ModbusDataStructures_IP

#include <byteManipulation.h>
#include <stdint.h> // uintX_t
#include <string.h> // memcpy()

// #ifdef ModbusRTU
#include <FastCRC.h> // For RTU capability https://github.com/FrankBoesing/FastCRC, PlatformIO: frankboesing/FastCRC @ ^1.41
// #endif

#ifdef __AVR__
#include <Array.h>    //https://github.com/janelia-arduino/Array
#include <Vector.h>   //https://github.com/janelia-arduino/Vector
#define array Array   // Make code invariant
#define vector Vector // Make code invariant
uint8_t requestBuffer[64] = {0};
uint8_t responseBuffer[64] = {0};
#else
#include <vector>
#include <array>
using std::array;
using std::vector;
#endif

uint8_t CompressBooleans(uint8_t *b, int8_t limit = 8);
bool CRC16Check(const uint8_t *data, uint8_t byteCount);

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
    vector<uint8_t> Values;
};

ModbusRequestPDU ParseRequestPDU(uint8_t *data)
{
    ModbusRequestPDU req = {
        .FunctionCode = static_cast<ModbusFunction>(data[0]),
        .Address = CombineBytes(data[1], data[2]),
        .NumberOfRegisters = CombineBytes(data[3], data[4]),
        .RegisterValue = CombineBytes(data[3], data[4]),
        .DataByteCount = data[5]};

#ifdef __AVR__
    req.Values.setStorage(requestBuffer, req.DataByteCount);
#else
    req.Values.resize(req.DataByteCount);
#endif
    memcpy(req.Values.data(), data + 6, req.DataByteCount);
    return req;
}

void getRequestBytes(ModbusRequestPDU PDU, uint8_t *bytesBuffer) // Primarily for testing, not normally used //TODO consider implementing WriteCoils Byte compression // TODO return vector<uint8_t>
{
    bytesBuffer[0] = PDU.FunctionCode;
    SplitBytes(PDU.Address, Big, bytesBuffer + 1);

    switch (PDU.FunctionCode)
    {
    case ModbusFunction::ReadCoils:
    case ModbusFunction::ReadDiscreteInputs:
    case ModbusFunction::ReadHoldingRegisters:
    case ModbusFunction::ReadInputRegisters:
        SplitBytes(PDU.NumberOfRegisters, Big, bytesBuffer + 3);
        break;
    case ModbusFunction::WriteSingleCoil:
    case ModbusFunction::WriteSingleHoldingRegister:
        SplitBytes(PDU.RegisterValue, Big, bytesBuffer + 3);
        break;
    case ModbusFunction::WriteMultipleCoils:
    case ModbusFunction::WriteMultipleHoldingRegisters:
        SplitBytes(PDU.NumberOfRegisters, Big, bytesBuffer + 3);
        bytesBuffer[5] = PDU.DataByteCount;
        memcpy(bytesBuffer + 6, PDU.Values.data(), PDU.DataByteCount);
        break;
    }
}

uint8_t getRequestByteLength(ModbusRequestPDU PDU)
{
    return 5 + PDU.DataByteCount + (PDU.DataByteCount > 0 ? 1 : 0);
}

struct ModbusResponsePDU
{
    ModbusFunction FunctionCode = ModbusFunction::ReadCoils;
    uint16_t Address = 0; // Unchanged from request PDU
    uint8_t DataByteCount = 0;
    uint16_t NumberOfRegistersChanged = 0; // Unchanged from request PDU
    vector<uint8_t> RegisterValue;
    ModbusError Error = NoError;
};

ModbusResponsePDU CreateErroredResponse(ModbusError Error)
{
    ModbusResponsePDU resp;
    resp.FunctionCode = ModbusFunction::ReadCoils;
    resp.Address = 0;
    resp.DataByteCount = 0;
    resp.NumberOfRegistersChanged = 0;
    resp.Error = Error;
    return resp;
}

ModbusResponsePDU ParseResponsePDU(const uint8_t *data)
{
    if (data[0] & 0b10000000)
    {
        return CreateErroredResponse(static_cast<ModbusError>(data[1]));
    };

    ModbusFunction code = static_cast<ModbusFunction>(data[0]);
    ModbusResponsePDU resp;
    resp.FunctionCode = code;
    resp.Address = 0;
    resp.DataByteCount = 0;
    resp.NumberOfRegistersChanged = 0;
    resp.Error = NoError;

    switch (code)
    {
    case ModbusFunction::ReadCoils:
    case ModbusFunction::ReadDiscreteInputs:
    case ModbusFunction::ReadHoldingRegisters:
    case ModbusFunction::ReadInputRegisters:
    {
        resp.DataByteCount = data[1];
#ifdef __AVR__
        resp.RegisterValue.setStorage(responseBuffer, resp.DataByteCount);
#else
        resp.RegisterValue.resize(resp.DataByteCount);
#endif
        memcpy(resp.RegisterValue.data(), data + 2, resp.DataByteCount);
    }
    break;

    case ModbusFunction::WriteSingleCoil:
    case ModbusFunction::WriteSingleHoldingRegister:
        resp.Address = CombineBytes(data[1], data[2]);
        resp.NumberOfRegistersChanged = 1;
#ifdef __AVR__
        resp.RegisterValue.setStorage(responseBuffer, resp.DataByteCount);
#else
        resp.RegisterValue.resize(resp.DataByteCount);
#endif
        memcpy(resp.RegisterValue.data(), data + 3, 2);
        break;

    case ModbusFunction::WriteMultipleCoils:
    case ModbusFunction::WriteMultipleHoldingRegisters:
        resp.Address = CombineBytes(data[1], data[2]);
        resp.NumberOfRegistersChanged = CombineBytes(data[3], data[4]);
        break;

    default:
        resp.Error = ModbusError::IllegalFunction;
        break;
    }
    return resp;
}

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
        memcpy(DataBuffer + 2,
               responseData.RegisterValue.data(),
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

// #ifdef ModbusRTU
bool CRC16Check(const uint8_t *data, uint8_t byteCount)
{
    FastCRC16 CRC16;
    return (CRC16.modbus(data, byteCount - 2) == CombineBytes(data[byteCount - 1], data[byteCount - 2]));
}
// #endif

uint8_t CompressBooleans(uint8_t *boolArray, int8_t limit = 8)
{
    uint8_t c = 0;
    for (int i = 0; i < limit && i < 8; i++)
    {
        c |= (boolArray[i] << i);
    }
    return c;
}

array<bool, 8> DecompressBooleans(const uint8_t b)
{
    array<bool, 8> c;
    for (int i = 0; i < 8; i++)
        c[i] = static_cast<bool>(b & (1 << i));
    return c;
}

#endif