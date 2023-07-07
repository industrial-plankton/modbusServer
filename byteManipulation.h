#ifndef H_ByteManipulation_IP
#define H_ByteManipulation_IP

#include <cstdint>
#include <cstring>

union converter
{
    uint16_t asInt;
    uint8_t asByte[2];
};

enum Endianness
{
    Big,
    Little
};

Endianness EndiannessTest()
{
    uint16_t TestValue = 1;
    return static_cast<Endianness>(reinterpret_cast<uint8_t *>(&TestValue)[0] == TestValue);

    // converter test = {.asInt = 1};
    // return static_cast<Endianness>(test.asInt == test.asByte[0]);
}

uint16_t byteSwap(uint16_t wrongEndianessInteger)
{
    converter In = {.asInt = wrongEndianessInteger};
    converter out = {.asByte = {In.asByte[1], In.asByte[0]}};
    return out.asInt;
    // return (uint16_t)((wrongEndianess & 0x00ff) << 8) | ((wrongEndianess & 0xff00) >> 8);
}

uint16_t CombineBytes(uint8_t HighBits, uint8_t LowBits)
{
    if (EndiannessTest() == Endianness::Little)
    {
        converter In = {.asByte = {LowBits, HighBits}};
        return In.asInt;
    }
    else
    {
        converter In = {.asByte = {HighBits, LowBits}};
        return In.asInt;
    }
}

uint8_t *SplitBytes(uint16_t integer, Endianness EndianDesired, uint8_t *Bytes)
{
    if (EndiannessTest() != EndianDesired)
    {
        integer = byteSwap(integer);
    }
    std::memcpy(Bytes, &integer, 2);
    return Bytes;
}
#endif