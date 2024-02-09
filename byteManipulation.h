#ifndef H_ByteManipulation_IP
#define H_ByteManipulation_IP

#include <stdint.h> // uintX_t
#include <string.h> // memcpy()

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

constexpr Endianness EndiannessTest()
{
#ifdef __AVR__
    return Little;
#else
    uint16_t TestValue = 1;
    return static_cast<Endianness>(reinterpret_cast<uint8_t *>(&TestValue)[0] == TestValue);
#endif
}

uint16_t byteSwap(uint16_t wrongEndianessInteger)
{
    converter In = {.asInt = wrongEndianessInteger};
    converter out = {.asByte = {In.asByte[1], In.asByte[0]}};
    return out.asInt;
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
    memcpy(Bytes, &integer, 2);
    return Bytes;
}
#endif