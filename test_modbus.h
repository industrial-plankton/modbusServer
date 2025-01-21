#include "unity.h"
#include <registers.h>

namespace ModbusServer
{
    uint8_t dataBuffer[32] = {0};

    void setup()
    {
    }

    void tearDown()
    {
    }

    void test_RequestByteTranslation()
    {
        uint16_t LocalValues[3] = {0};
#ifdef __AVR__
        ModbusFunction ModbusFunctions[1] = {ModbusFunction::WriteMultipleHoldingRegisters};
        HoldingRegister LocalHoldingRegister(0, 3, vector<ModbusFunction>(ModbusFunctions), LocalValues, false);
        Register *RegistersArray[1] = {&LocalHoldingRegister};
        vector<Register *> asVec(RegistersArray);
        Registers regs(asVec);
#else
        HoldingRegister LocalHoldingRegister(0, 3, std::vector<ModbusFunction>{ModbusFunction::WriteMultipleHoldingRegisters}, LocalValues, false, false);
        Registers regs(std::vector<Register *>{&LocalHoldingRegister});
#endif

        uint16_t RequestValues[2] = {2, 3};
        ModbusRequestPDU expectedPDU = {
            .FunctionCode = ModbusFunction::WriteMultipleHoldingRegisters,
            .Address = 1,
            .NumberOfRegisters = 2,
            .RegisterValue = 0,
            .DataByteCount = 4};

#ifdef __AVR__
        expectedPDU.Values.setStorage(dataBuffer, expectedPDU.DataByteCount);
        TEST_ASSERT_EQUAL(4, expectedPDU.DataByteCount);
#else
        expectedPDU.Values.resize(expectedPDU.DataByteCount);
#endif

        memcpy(expectedPDU.Values.data(), RequestValues, expectedPDU.DataByteCount);
        TEST_ASSERT_EQUAL(2, expectedPDU.Values[0]);
        TEST_ASSERT_EQUAL(3, expectedPDU.Values[2]);

        uint8_t buffer[256] = {0};
        getRequestBytes(expectedPDU, buffer);

        const ModbusRequestPDU processedRequestPDU = ParseRequestPDU(buffer);

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
        TEST_ASSERT_EQUAL(expectedPDU.Values[0], processedRequestPDU.Values[0]);
        TEST_ASSERT_EQUAL(expectedPDU.Values[1], processedRequestPDU.Values[1]);
        TEST_ASSERT_EQUAL(expectedPDU.Values[2], processedRequestPDU.Values[2]);
        TEST_ASSERT_EQUAL(expectedPDU.Values[3], processedRequestPDU.Values[3]);
    }

    void test_Server_WriteMultipleHoldingRegisters()
    {
        uint16_t LocalValues[3] = {0};
#ifdef __AVR__
        ModbusFunction ModbusFunctions[1] = {ModbusFunction::WriteMultipleHoldingRegisters};
        HoldingRegister LocalHoldingRegister(0, 3, vector<ModbusFunction>(ModbusFunctions, 1), LocalValues, false);
        Register *RegistersArray[1] = {&LocalHoldingRegister};
        vector<Register *> asVec(RegistersArray, 1);
        TEST_ASSERT_EQUAL(1, asVec.size());
        Registers regs(asVec);
#else
        HoldingRegister LocalHoldingRegister(0, 3, std::vector<ModbusFunction>{ModbusFunction::WriteMultipleHoldingRegisters}, LocalValues, false, false);
        Registers regs(std::vector<Register *>{&LocalHoldingRegister});
#endif
        uint16_t RequestValues[2] = {2, 3};
        ModbusRequestPDU reqPDU = {.FunctionCode = ModbusFunction::WriteMultipleHoldingRegisters,
                                   .Address = 1,
                                   .NumberOfRegisters = 2,
                                   .RegisterValue = 0,
                                   .DataByteCount = 4};

#ifdef __AVR__
        reqPDU.Values.setStorage(dataBuffer, reqPDU.DataByteCount);
#else
        reqPDU.Values.resize(reqPDU.DataByteCount);
#endif

        memcpy(reqPDU.Values.data(), RequestValues, reqPDU.DataByteCount);

        uint8_t buffer[256] = {0};
        getRequestBytes(reqPDU, buffer);

        const ModbusRequestPDU processedRequestPDU = ParseRequestPDU(buffer);
        TEST_ASSERT_EQUAL(2, processedRequestPDU.Values[0]);
        TEST_ASSERT_EQUAL(3, processedRequestPDU.Values[2]);
        TEST_ASSERT_EQUAL(ModbusFunction::WriteMultipleHoldingRegisters, processedRequestPDU.FunctionCode);

        const ModbusResponsePDU response = regs.ProcessRequest(processedRequestPDU);
        TEST_ASSERT_EQUAL(ModbusError::NoError, response.Error); // returning Illegal function
        TEST_ASSERT_EQUAL(0, LocalValues[0]);
        TEST_ASSERT_EQUAL(2, LocalValues[1]);
        TEST_ASSERT_EQUAL(3, LocalValues[2]);

        const uint8_t responseByteCount = ModbusResponsePDUtoStream(response, buffer);
        TEST_ASSERT_EQUAL(5, responseByteCount);
    }

    void test_Server_ReadMultipleHoldingRegisters()
    {
        uint16_t LocalValues[3] = {0, 2, 3};
#ifdef __AVR__
        ModbusFunction ModbusFunctions[1] = {ModbusFunction::ReadHoldingRegisters};
        HoldingRegister LocalHoldingRegister(0, 3, vector<ModbusFunction>(ModbusFunctions, 1), LocalValues);
        Register *RegistersArray[1] = {&LocalHoldingRegister};
        vector<Register *> asVec(RegistersArray, 1);
        Registers regs(asVec);
#else
        HoldingRegister LocalHoldingRegister(0, 3, std::vector<ModbusFunction>{ModbusFunction::ReadHoldingRegisters}, LocalValues, false, false);
        Registers regs(std::vector<Register *>{&LocalHoldingRegister});
#endif

        ModbusRequestPDU reqPDU = {.FunctionCode = ModbusFunction::ReadHoldingRegisters,
                                   .Address = 1,
                                   .NumberOfRegisters = 2,
                                   .RegisterValue = 0,
                                   .DataByteCount = 0,
                                   .Values = {}};

        uint8_t buffer[256] = {0};
        getRequestBytes(reqPDU, buffer);
        const ModbusRequestPDU processedRequestPDU = ParseRequestPDU(buffer);
        const ModbusResponsePDU response = regs.ProcessRequest(processedRequestPDU);
        const uint16_t *valuesAsInt = reinterpret_cast<const uint16_t *>(response.RegisterValue.data());
        TEST_ASSERT_EQUAL(4, response.DataByteCount);
        TEST_ASSERT_EQUAL(2, valuesAsInt[0]);
        TEST_ASSERT_EQUAL(3, valuesAsInt[1]);
        TEST_ASSERT_EQUAL(2 + response.DataByteCount, ModbusResponsePDUtoStream(response, buffer));
    }

    void test_Server_WriteFloats()
    {
        float LocalValues[3] = {0};
#ifdef __AVR__
        ModbusFunction ModbusFunctions[1] = {ModbusFunction::WriteMultipleHoldingRegisters};
        HoldingRegister LocalHoldingRegister(0, 3, vector<ModbusFunction>(ModbusFunctions, 1), reinterpret_cast<uint16_t *>(LocalValues), false);
        Register *RegistersArray[1] = {&LocalHoldingRegister};
        vector<Register *> asVec(RegistersArray, 1);
        Registers regs(asVec);
#else
        HoldingRegister LocalHoldingRegister(0, 3, std::vector<ModbusFunction>{ModbusFunction::WriteMultipleHoldingRegisters}, reinterpret_cast<uint16_t *>(LocalValues), false, false);
        Registers regs(std::vector<Register *>{&LocalHoldingRegister});
#endif

        float RequestValues[1] = {2.5};
        ModbusRequestPDU reqPDU = {.FunctionCode = ModbusFunction::WriteMultipleHoldingRegisters,
                                   .Address = 2,
                                   .NumberOfRegisters = 2,
                                   .RegisterValue = 0,
                                   .DataByteCount = 4,
                                   .Values = {}};

#ifdef __AVR__
        reqPDU.Values.setStorage(dataBuffer, reqPDU.DataByteCount);
#else
        reqPDU.Values.resize(reqPDU.DataByteCount);
#endif

        memcpy(reqPDU.Values.data(), RequestValues, reqPDU.DataByteCount);

        uint8_t buffer[256] = {0};
        getRequestBytes(reqPDU, buffer);

        const ModbusRequestPDU processedRequestPDU = ParseRequestPDU(buffer);
        const ModbusResponsePDU response = regs.ProcessRequest(processedRequestPDU);
        TEST_ASSERT_EQUAL(0, LocalValues[0]);
        TEST_ASSERT_EQUAL(2.5, LocalValues[1]);
        TEST_ASSERT_EQUAL(0, LocalValues[2]);

        const uint8_t responseByteCount = ModbusResponsePDUtoStream(response, buffer);
        TEST_ASSERT_EQUAL(5, responseByteCount);
    }

    void test_Server_ReadCoils()
    {
        uint8_t C[2000] = {0};
#ifdef __AVR__
        ModbusFunction ModbusFunctions[3] = {ModbusFunction::ReadCoils, ModbusFunction::WriteSingleCoil, ModbusFunction::WriteMultipleCoils};
        CoilRegister Coils(0x4000, 0x47CF, vector<ModbusFunction>(ModbusFunctions, 3), C);
        Register *RegistersArray[1] = {&Coils};
        vector<Register *> asVec(RegistersArray, 1);
        Registers regs(asVec);
#else
        CoilRegister Coils(0x4000, 0x47CF, std::vector<ModbusFunction>{ModbusFunction::ReadCoils, ModbusFunction::WriteSingleCoil, ModbusFunction::WriteMultipleCoils}, C);
        Registers regs(std::vector<Register *>{&Coils});
#endif
        C[178] = 255;

        ModbusRequestPDU reqPDU = {.FunctionCode = ModbusFunction::ReadCoils,
                                   .Address = 16500,
                                   .NumberOfRegisters = 88,
                                   .RegisterValue = 0,
                                   .DataByteCount = 0,
                                   .Values = {}};

        uint8_t buffer[256] = {0};
        getRequestBytes(reqPDU, buffer);
        const ModbusRequestPDU processedRequestPDU = ParseRequestPDU(buffer);
        const ModbusResponsePDU response = regs.ProcessRequest(processedRequestPDU);

        array<bool, 90> Values = {};
        for (size_t i = 0; i < response.DataByteCount && i * 8 < 90; i++)
        {
            auto asBooleans = DecompressBooleans(response.RegisterValue[i]);
            memcpy(Values.data() + 8 * i, asBooleans.data(), 8);
        }

        TEST_ASSERT_EQUAL(11, response.DataByteCount);
        TEST_ASSERT_TRUE(Values[178 + 0x4000 - 16500]);
    }

    void test_Server_WriteCoil()
    {
        bool LocalValues[3] = {false, true, false};
#ifdef __AVR__
        ModbusFunction ModbusFunctions[1] = {ModbusFunction::WriteSingleCoil};
        CoilRegister Coils(17000, 17003, vector<ModbusFunction>(ModbusFunctions, 1), reinterpret_cast<uint8_t *>(LocalValues));
        Register *RegistersArray[1] = {&Coils};
        vector<Register *> asVec(RegistersArray, 1);
        Registers regs(asVec);
#else
        CoilRegister LocalHoldingRegister(17000, 17003, std::vector<ModbusFunction>{ModbusFunction::WriteSingleCoil}, reinterpret_cast<uint8_t *>(LocalValues));
        Registers regs(std::vector<Register *>{&LocalHoldingRegister});
#endif
        ModbusRequestPDU reqPDU = {.FunctionCode = ModbusFunction::WriteSingleCoil,
                                   .Address = 17002,
                                   .NumberOfRegisters = 1,
                                   .RegisterValue = 65280,
                                   //    .RegisterValue = 255,
                                   .DataByteCount = 0,
                                   .Values = {}};

        uint8_t buffer[256] = {0};
        getRequestBytes(reqPDU, buffer);

        const ModbusRequestPDU processedRequestPDU = ParseRequestPDU(buffer);
        const ModbusResponsePDU response = regs.ProcessRequest(processedRequestPDU);

        ModbusRequestPDU reqPDU2 = {.FunctionCode = ModbusFunction::WriteSingleCoil,
                                    .Address = 17001,
                                    .NumberOfRegisters = 1,
                                    .RegisterValue = 0,
                                    .DataByteCount = 0,
                                    .Values = {}};

        getRequestBytes(reqPDU2, buffer);

        const ModbusRequestPDU processedRequestPDU2 = ParseRequestPDU(buffer);
        const ModbusResponsePDU response2 = regs.ProcessRequest(processedRequestPDU2);
        TEST_ASSERT_EQUAL(false, LocalValues[0]);
        TEST_ASSERT_EQUAL(false, LocalValues[1]);
        TEST_ASSERT_EQUAL(true, LocalValues[2]);

        const uint8_t responseByteCount = ModbusResponsePDUtoStream(response2, buffer);
        TEST_ASSERT_EQUAL(5, responseByteCount);
    }

    void test_Server_WriteMultipleCoils()
    {

        bool LocalValues[3] = {false, true, false};
#ifdef __AVR__
        ModbusFunction ModbusFunctions[1] = {ModbusFunction::WriteMultipleCoils};
        CoilRegister Coils(17000, 17003, vector<ModbusFunction>(ModbusFunctions, 1), reinterpret_cast<uint8_t *>(LocalValues));
        Register *RegistersArray[1] = {&Coils};
        vector<Register *> asVec(RegistersArray, 1);
        Registers regs(asVec);
#else
        CoilRegister LocalHoldingRegister(17000, 17003, std::vector<ModbusFunction>{ModbusFunction::WriteMultipleCoils}, reinterpret_cast<uint8_t *>(LocalValues));
        Registers regs(std::vector<Register *>{&LocalHoldingRegister});
#endif
        ModbusRequestPDU reqPDU = {.FunctionCode = ModbusFunction::WriteMultipleCoils,
                                   .Address = 17000,
                                   .NumberOfRegisters = 3,
                                   .RegisterValue = 0,
                                   .DataByteCount = 1,
                                   .Values = {}};
#ifdef __AVR__
        reqPDU.Values.setStorage(dataBuffer, 3);
#else
        reqPDU.Values.resize(3);
#endif
        uint8_t values[3] = {1, 0, 1};
        reqPDU.Values[0] = CompressBooleans(values, 3);

        uint8_t buffer[256] = {0};
        getRequestBytes(reqPDU, buffer);

        const ModbusRequestPDU processedRequestPDU = ParseRequestPDU(buffer);
        const ModbusResponsePDU response = regs.ProcessRequest(processedRequestPDU);
        TEST_ASSERT_EQUAL(true, LocalValues[0]);
        TEST_ASSERT_EQUAL(false, LocalValues[1]);
        TEST_ASSERT_EQUAL(true, LocalValues[2]);
    }

    void test_LittleEndian()
    {
        TEST_ASSERT_EQUAL(Little, EndiannessTest()); // This will fail if the System is Big Endian
    }

    void test_DecompressBooleans()
    {
        uint8_t ValuesCompressed[3] = {0b00000010u, 0b01000000u, 0b00000000u};
        array<bool, 3 * 8> Values = {};
        for (size_t i = 0; i < 3; i++)
        {
            array<bool, 8> asBooleans = DecompressBooleans(ValuesCompressed[i]);
            memcpy(Values.data() + 8 * i, asBooleans.data(), 8);
        }

        TEST_ASSERT_EQUAL(0, Values[0]);
        TEST_ASSERT_EQUAL(1, Values[1]);
        TEST_ASSERT_EQUAL(0, Values[2]);
        TEST_ASSERT_EQUAL(0, Values[13]);
        TEST_ASSERT_EQUAL(1, Values[14]);
        TEST_ASSERT_EQUAL(0, Values[15]);
    };

    void test(void)
    {
        setup();
        RUN_TEST(test_RequestByteTranslation);
        RUN_TEST(test_Server_WriteMultipleHoldingRegisters);
        RUN_TEST(test_Server_ReadMultipleHoldingRegisters);
        RUN_TEST(test_Server_WriteFloats);
        RUN_TEST(test_LittleEndian);
        RUN_TEST(test_DecompressBooleans);
        RUN_TEST(test_Server_ReadCoils);
        RUN_TEST(test_Server_WriteCoil);
        RUN_TEST(test_Server_WriteMultipleCoils);
        tearDown();
    }
} // namespace ModbusServer
