# modbusServer

A flexible Modbus (TCP & RTU) server for C++ 11 and above. Designed for embedded systems, in particular [Teensy's](https://www.pjrc.com/teensy/) and Arduino devices.

## Flexibility

Modbus logic is isolated to its own files, managing receiving and replying with the Modbus data is left to the user, though standard implementations are included for Arduino Serial RTU and Teensy TCP.

## Target

Designed to be used as a PlatformIO library though should be usable anywhere.

## Dependencies

- RTU requires the [fastCRC](https://github.com/FrankBoesing/FastCRC) lib, CRC logic is isolated to the ReceiveRTUStream function in registers.h so can easily be replaced with other implementations or disabled.

- TCP Depends on a external TCP stack. The StdTeensyModbusTCP.h usable implementation for teensy relies on [QNEthernet.h](https://github.com/ssilverman/QNEthernet) for its TCP stack and should be usable for any system compatible with that library.

## Examples

Generic examples are located in the Examples folder, and use the Std*.h implementations to show how to setup, use modbus registers, and run the server functions. They are written in the generic C++ main() form but have Arduino Function annotations for those only familiar with the Arduino framework.

## Endianness and Byte Swapping

The Modbus standard specifies BIG Endian for its data. To add flexibility for nonstandard types (eg. floats) there is an option to receive data as little endian (control frames are always BIG endian). However currently this lib always sends its data bytes in the Endianness of the hardware its running on (tends to be LITTLE). This is done to prevent unnecessary double byte swaps, as most clients support byte swapping to achieve cross Endianness support.

## Testing

Lightly tested written using the unity test suite, coverage may be expanded later. Manually tested extensively on Teensy 4.1.
