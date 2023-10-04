#include <registers.h>
#include "unity.h"

void TestRequestByteTranslation(ModbusRequestPDU expectedPDU, ModbusRequestPDU processedRequestPDU)
{
    TEST_ASSERT_EQUAL(expectedPDU.Address, processedRequestPDU.Address);
    TEST_ASSERT_EQUAL(expectedPDU.DataByteCount, processedRequestPDU.DataByteCount);
    TEST_ASSERT_EQUAL(expectedPDU.FunctionCode, processedRequestPDU.FunctionCode);
    if (ModbusFunction::WriteSingleCoil == expectedPDU.FunctionCode || ModbusFunction::WriteSingleHoldingRegister == expectedPDU.FunctionCode)
    {
        TEST_ASSERT_EQUAL(expectedPDU.RegisterValue, processedRequestPDU.RegisterValue);
    }
    else
    {
        TEST_ASSERT_EQUAL(expectedPDU.NumberOfRegisters, processedRequestPDU.NumberOfRegisters);
    }
    TEST_ASSERT_EQUAL(expectedPDU.Values[0], processedRequestPDU.Values[0]); // Expected 537362036 Was 537362110
}

void testWriteMultipleHoldingRegisters()
{
    uint16_t LocalValues[3] = {0};
    HoldingRegister LocalHoldingRegister(0, 3, std::vector<ModbusFunction>{ModbusFunction::WriteMultipleHoldingRegisters}, LocalValues, false);
    Registers regs(std::vector<Register *>{&LocalHoldingRegister});

    uint16_t RequestValues[2] = {2, 3};
    ModbusRequestPDU reqPDU = {.FunctionCode = ModbusFunction::WriteMultipleHoldingRegisters,
                               .Address = 1,
                               .NumberOfRegisters = 2,
                               .RegisterValue = 0,
                               .DataByteCount = 4,
                               .Values = reinterpret_cast<uint8_t *>(RequestValues)};

    uint8_t buffer[256] = {0};
    getRequestBytes(reqPDU, buffer);

    const ModbusRequestPDU processedRequestPDU = ParseRequestPDU(buffer);
    TestRequestByteTranslation(reqPDU, processedRequestPDU);

    const ModbusResponsePDU response = regs.ProcessRequest(processedRequestPDU);
    TEST_ASSERT_EQUAL(0, LocalValues[0]);
    TEST_ASSERT_EQUAL(2, LocalValues[1]);
    TEST_ASSERT_EQUAL(3, LocalValues[2]);

    const uint8_t responseByteCount = ModbusResponsePDUtoStream(response, buffer);
    TEST_ASSERT_EQUAL(5, responseByteCount);
}

void testReadMultipleHoldingRegisters()
{
    uint16_t LocalValues[3] = {0, 2, 3};
    HoldingRegister LocalHoldingRegister(0, 3, std::vector<ModbusFunction>{ModbusFunction::ReadHoldingRegisters}, LocalValues);
    Registers regs(std::vector<Register *>{&LocalHoldingRegister});

    ModbusRequestPDU reqPDU = {.FunctionCode = ModbusFunction::ReadHoldingRegisters,
                               .Address = 1,
                               .NumberOfRegisters = 2,
                               .RegisterValue = 0,
                               .DataByteCount = 0,
                               .Values = nullptr};

    uint8_t buffer[256] = {0};
    getRequestBytes(reqPDU, buffer);
    const ModbusRequestPDU processedRequestPDU = ParseRequestPDU(buffer);
    const ModbusResponsePDU response = regs.ProcessRequest(processedRequestPDU);
    uint16_t *valuesAsInt = reinterpret_cast<uint16_t *>(response.RegisterValue);
    TEST_ASSERT_EQUAL(4, response.DataByteCount);
    TEST_ASSERT_EQUAL(2, valuesAsInt[0]);
    TEST_ASSERT_EQUAL(3, valuesAsInt[1]);
    TEST_ASSERT_EQUAL(2 + response.DataByteCount, ModbusResponsePDUtoStream(response, buffer));
}

void testWriteFloats()
{
    float LocalValues[3] = {0};
    HoldingRegister LocalHoldingRegister(0, 3, std::vector<ModbusFunction>{ModbusFunction::WriteMultipleHoldingRegisters}, reinterpret_cast<uint16_t *>(LocalValues), false);
    Registers regs(std::vector<Register *>{&LocalHoldingRegister});

    float RequestValues[1] = {2.5};
    ModbusRequestPDU reqPDU = {.FunctionCode = ModbusFunction::WriteMultipleHoldingRegisters,
                               .Address = 2,
                               .NumberOfRegisters = 2,
                               .RegisterValue = 0,
                               .DataByteCount = 4,
                               .Values = reinterpret_cast<uint8_t *>(RequestValues)};

    uint8_t buffer[256] = {0};
    getRequestBytes(reqPDU, buffer);

    const ModbusRequestPDU processedRequestPDU = ParseRequestPDU(buffer);
    TestRequestByteTranslation(reqPDU, processedRequestPDU);

    const ModbusResponsePDU response = regs.ProcessRequest(processedRequestPDU);
    TEST_ASSERT_EQUAL(0, LocalValues[0]);
    TEST_ASSERT_EQUAL(2.5, LocalValues[1]);
    TEST_ASSERT_EQUAL(0, LocalValues[2]);

    const uint8_t responseByteCount = ModbusResponsePDUtoStream(response, buffer);
    TEST_ASSERT_EQUAL(5, responseByteCount);
}

void test_LittleEndian()
{
    TEST_ASSERT_EQUAL(Little, EndiannessTest()); // This will fail if the System is Big Endian
}

void TestModbus(void)
{
    RUN_TEST(testWriteMultipleHoldingRegisters);
    RUN_TEST(testReadMultipleHoldingRegisters);
    RUN_TEST(testWriteFloats);
    RUN_TEST(test_LittleEndian);
}