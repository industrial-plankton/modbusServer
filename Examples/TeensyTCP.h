#include <vector>

#include <StdTeensyModbusTCP.h>
// Dependencies
// https://github.com/FrankBoesing/FastCRC // only used for RTU
// https://github.com/ssilverman/QNEthernet

using namespace qindesign::network;

const TCPServerInit ServerSettings = {
    .ServerPort = 502,
    .LinkTimeout = 5000,
    .ClientTimeout = 5000,
    .ShutdownTimeout = 5000,
    .staticIP = IPAddress{192, 168, 0, 11},
    .subnetMask = IPAddress{255, 255, 255, 0},
    .gateway = IPAddress{192, 168, 0, 1},
};

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

const std::array<char, 8> TXT = {'2', '.', '6', '.', '6', '6', ' ', ' '}; // 2 chars per holding register, only used to hold the version (PLC legacy)
HoldingRegister Chars(0x9005, 0x9008, std::vector<ModbusFunction>{ReadHoldingRegisters}, (uint16_t *)TXT.data());

Registers registers(std::vector<Register *>{&InputCoils, &OutputCoils, &Coils, &SystemCoils, &Integers, &Doubles, &Floats, &SystemInts, &Chars});
StdTeenyModbusTCPServer ModbusServer(ServerSettings, registers);

extern "C" int main(void)
{
    // Setup();
    {
        Serial.begin(115200);
        while (!Serial && millis() < 4000)
        {
            // Wait for Serial to initialize
        }

        stdPrint = &Serial; // Make printf work (a QNEthernet feature)
        printf("Starting...\r\n");

        ModbusServer.Initialize(ServerSettings);
    }

    for (;;)
    {
        // Loop();
        {
            ModbusServer.ConnectNewClients();
            ModbusServer.ProcessClients();

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