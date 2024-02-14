#ifndef H_StdModbusRTU_IP
#define H_StdModbusRTU_IP
#include <Arduino.h>
#define ModbusRTU
#include <registers.h>

// #define ModbusSerialPort Serial // Map the serial port used for modbus
const uint8_t ModbusAddress = 1;

// void Initialize(long baud)
// {
// Beware baud rate mismatches / error rates due to clock differences
// ModbusSerialPort.begin(baud); //, SERIAL_8E1); pass serial format (8 wide, Even parity, 1 Stop bit) if you can (SerialUSB doesn't support it)
// ModbusSerial.transmitterEnable(pinNumber); on teensy's this can be used to easily control RS485/422 transmission enable
// }

void Process(Registers &registers, Stream &ModbusSerial)
{
    array<uint8_t, 128> ModbusFrame = {0}; // Should limit size to the same as the serial ring buffer
    uint16_t bufferIndex = 0;

    while (ModbusSerial.available() > 0)
    {
        if (bufferIndex <= ModbusFrame.size())
        {
            ModbusFrame[bufferIndex] = ModbusSerial.read();
            bufferIndex++;
        }
        else
        {
            ModbusSerial.read();
        }

        if (!ModbusSerial.available())
        {
            delayMicroseconds(750); // inter character time out
        }
    }

    const auto broadcast = ModbusFrame[0] == 0;
    if (ModbusAddress == ModbusFrame[0] || broadcast) // Match ID
    {
        const auto responseSize = ReceiveRTUStream(registers, ModbusFrame, bufferIndex);
        if (responseSize > 0 && !broadcast)
        {
            ModbusSerial.write(ModbusFrame.data(), responseSize);
            // ModbusSerial.flush(); // Finish transmitting all bytes, not strictly necessary and .flush() sometimes has a different function in Arduino
        }
    }
}

#endif