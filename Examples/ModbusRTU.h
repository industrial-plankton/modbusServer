#include <vector>

#include <StdArdunioModbusRTU.h>
// Dependencies
// https://github.com/FrankBoesing/FastCRC

// Define Modbus registers and types
std::array<bool, 272> X;
CoilRegister InputCoils(0, 0x010F, std::vector<ModbusFunction>{ReadDiscreteInputs}, (uint8_t *)X.data());
std::array<bool, 272> Y;
CoilRegister OutputCoils(0x2000, 0x210F, std::vector<ModbusFunction>{ReadCoils, WriteSingleCoil, WriteMultipleCoils}, (uint8_t *)Y.data());
std::array<bool, 2000> C;
CoilRegister Coils(0x4000, 0x47CF, std::vector<ModbusFunction>{ReadCoils, WriteSingleCoil, WriteMultipleCoils}, (uint8_t *)C.data());
std::array<bool, 1000> SC;
CoilRegister SystemCoils(0xF000, 0xF3E7, std::vector<ModbusFunction>{ReadDiscreteInputs, ReadCoils, WriteSingleCoil, WriteMultipleCoils}, (uint8_t *)SC.data());

std::array<int16_t, 4500> DS;
HoldingRegister Integers(0, 0x1193, std::vector<ModbusFunction>{ReadHoldingRegisters, WriteSingleHoldingRegister, WriteMultipleHoldingRegisters}, (uint16_t *)DS.data());
std::array<int32_t, 1000> DD;
HoldingRegister Doubles(0x4000, 0x47CE, std::vector<ModbusFunction>{ReadHoldingRegisters, WriteSingleHoldingRegister, WriteMultipleHoldingRegisters}, (uint16_t *)DD.data());
std::array<float, 500> DF;
HoldingRegister Floats(0x7000, 0x73E6, std::vector<ModbusFunction>{ReadHoldingRegisters, WriteSingleHoldingRegister, WriteMultipleHoldingRegisters}, (uint16_t *)DF.data(), false);
std::array<int16_t, 1000> SD;
HoldingRegister SystemInts(0xF000, 0xF3E7, std::vector<ModbusFunction>{ReadInputRegisters, ReadHoldingRegisters, WriteSingleHoldingRegister, WriteMultipleHoldingRegisters}, (uint16_t *)SD.data());

const std::array<char, 8> TXT; // 2 chars per holding register
HoldingRegister Chars(0x9005, 0x9008, std::vector<ModbusFunction>{ReadHoldingRegisters}, (uint16_t *)TXT.data());

Registers registers(std::vector<Register *>{&InputCoils, &OutputCoils, &Coils, &SystemCoils, &Integers, &Doubles, &Floats, &SystemInts, &Chars});

extern "C" int main(void)
{
    // Setup();
    {
        // Beware baud rate mismatches / error rates due to clock differences
        Serial1.begin(38400, SERIAL_8E1); // pass serial format (8 wide, Even parity, 1 Stop bit) if you can (SerialUSB doesn't support it as it auto negotiates)
        // Serial1.transmitterEnable(pinNumber); on teensy's this can be used to easily control RS485/422 transmission enable
    }

    for (;;)
    {
        // Loop();
        {
            Process(registers, Serial1);

            // Use and set values
            Y[1] = true;

            const auto intput = X[20];

            DS.at(10)++;

            // Alias modbus registers
            auto &ThisCoilsName = C.at(9);
            auto &ThisIntegersName = DS.at(5);
            auto &ThisFloatsName = DF.at(5);

            // Use the Alias'
            ThisIntegersName = 10;
            ThisFloatsName = 10;
            ThisFloatsName++;
            ThisCoilsName = static_cast<float>(ThisIntegersName) < ThisFloatsName;
        }

        yield();
    }
}