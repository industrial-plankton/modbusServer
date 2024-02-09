#include <Arduino.h>
#include <StdArdunioModbusRTU.h>
// Dependencies
// https://github.com/janelia-arduino/Array
// https://github.com/janelia-arduino/Vector
// https://github.com/FrankBoesing/FastCRC

// Define Modbus registers and types
array<bool, 10> X;
ModbusFunction InputCoilsFunctions[1] = {ReadDiscreteInputs};
CoilRegister InputCoils(0, 10, vector<ModbusFunction>(InputCoilsFunctions, 1), (uint8_t *)X.data());
array<bool, 10> Y;
ModbusFunction CoilFunctions[3] = {ReadCoils, WriteSingleCoil, WriteMultipleCoils};
CoilRegister OutputCoils(0x2000, 0x2000 + 10, vector<ModbusFunction>(CoilFunctions, 3), (uint8_t *)Y.data());
array<bool, 10> C;
CoilRegister Coils(0x4000, 0x4000 + 10, vector<ModbusFunction>(CoilFunctions, 3), (uint8_t *)C.data());
array<bool, 10> SC;
ModbusFunction SystemCoilFunctions[4] = {ReadDiscreteInputs, ReadCoils, WriteSingleCoil, WriteMultipleCoils};
CoilRegister SystemCoils(0xF000, 0xF000 + 10, vector<ModbusFunction>(SystemCoilFunctions, 4), (uint8_t *)SC.data());

array<int16_t, 50> DS;
ModbusFunction HoldingFunctions[3] = {ReadHoldingRegisters, WriteSingleHoldingRegister, WriteMultipleHoldingRegisters};
HoldingRegister Integers(0, 50, vector<ModbusFunction>(HoldingFunctions, 3), (uint16_t *)DS.data());
array<int32_t, 20> DD;
HoldingRegister Doubles(0x4000, 0x4000 + 40, vector<ModbusFunction>(HoldingFunctions, 3), (uint16_t *)DD.data());
array<float, 20> DF;
HoldingRegister Floats(0x7000, 0x7000 + 40, vector<ModbusFunction>(HoldingFunctions, 3), (uint16_t *)DF.data(), false);
array<int16_t, 10> SD;
ModbusFunction SystemIntsFunctions[4] = {ReadInputRegisters, ReadHoldingRegisters, WriteSingleHoldingRegister, WriteMultipleHoldingRegisters};
HoldingRegister SystemInts(0xF000, 0xF004, vector<ModbusFunction>(SystemIntsFunctions, 4), (uint16_t *)SD.data());

const array<char, 8> TXT; // 2 chars per holding register
ModbusFunction ReadonlyCharsFunctions[4] = {ReadHoldingRegisters};
HoldingRegister Chars(0x9005, 0x9008, vector<ModbusFunction>(ReadonlyCharsFunctions, 1), (uint16_t *)TXT.data());

Register *RegistersArray[9] = {&InputCoils, &OutputCoils, &Coils, &SystemCoils, &Integers, &Doubles, &Floats, &SystemInts, &Chars};
vector<Register *> asVec(RegistersArray, 9);
Registers registers(asVec);

void setup()
{
    Serial.begin(38400, SERIAL_8E1);
}

void loop()
{
    // Reimplement StdArdunioModbusRTU.h to modify Modbus address and/or add read/write side effects
    Process(registers, Serial);

    // Use and set values
    Y[1] = true; // set "output" coil

    const auto intput = X[5]; // read "input" coil

    DS.at(10)++; // increment holding register value

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
