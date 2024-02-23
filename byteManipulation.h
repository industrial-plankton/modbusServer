#ifndef H_ByteManipulation_IP
#define H_ByteManipulation_IP

#include <stdint.h> // uintX_t
#include <string.h> // memcpy()

union converter
{
    uint16_t asInt;
    uint8_t asByte[2];
};

union wordConverter
{
    uint32_t as32;
    uint16_t as16[2];
};

enum Endianness
{
    Big,
    Little
};

constexpr Endianness EndiannessTest()
{
#ifdef __BYTE_ORDER__
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return Little;
#else
    return Big;
#endif
#else
#ifdef __AVR__
    return Little;
#else
    uint16_t TestValue = 1;
    return static_cast<Endianness>(reinterpret_cast<uint8_t *>(&TestValue)[0] == TestValue);
#endif
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

uint32_t CombineWord(uint16_t HighBits, uint16_t LowBits)
{
    if (EndiannessTest() == Endianness::Little)
    {
        wordConverter In = {.as16 = {LowBits, HighBits}};
        return In.as32;
    }
    else
    {
        wordConverter In = {.as16 = {HighBits, LowBits}};
        return In.as32;
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