---
title: 'MCU firmware protocol'
weight: 50
---

This page documents protocols used to communicate with the on-board STM32 MCU via SPI.

## Bootloader protocol

This is the default mode after resetting with BOOT0 pulled high.

For details on this protocol, please refer to the ST Application note AN4286.

## Native mode

This is the default mode after resetting with BOOT0 pulled low.

To initiate a transaction, send a 16-bit little endian value (two bytes) declaring the command ID (in bits 0-6) and parameter (bits 7-15, values 0-511).

Every packet except "Switch to mode" has a response. The response always consists of the response header (two bytes) and the data.

The response header's bits 1-15 are the length of the following data, in bytes; bit 0 is set to 1 if an error has occured (for example, if the command is unknown).

### 0x00 - Echo

Test command. The parameter is the number of bytes to echo, followed by the bytes to echo.

### 0x01 - MCU: Switch to mode

The parameter is the ID of the mode of communication to use going forward:

- `0x00` - command mode
- `0x01` - EEPROM emulation mode
- `0x02` - RTC S-3511A emulation mode
- `0x03` - CDC output mode (transferred SPI bytes are output via USB CDC)
- `0xFF` - standby mode (requires reset to respond to SPI again)

### 0x02 - SPI: Set maximum frequency

The parameter is the maximum SPI transfer frequency that the MCU should be configured for (clock/pin speeds):

- `0x00` - 384 KHz
- `0x01` - 6 MHz
- `0x02` - 24 MHz

The response is 1 on success, 0 on failure.

So far, speeds faster than 384 KHz have been tested with the exception of commands with large (> 2-4 bytes) input parameter buffers.

### 0x03 - MCU: Get unique ID

The response is the unique ID of the chip.

### 0x04 - MCU: Read status information (protocol 1.2+)

The response is the following structure:

- byte 0..1: status
  - bit 0: 0 = RTC using internal clock, 1 = RTC using external clock
  - bit 1: 0 = RTC not active, 1 = RTC active
  - bit 2: 1 = USB plugged in
  - bit 3: 1 = USB connected to host
- byte 2..3: capabilities
  - bit 0: EEPROM
  - bit 1: USB
  - bit 2: accelerometer
  - bit 3: RTC
  - bit 4: battery back up (battery inserted)
- byte 4..5: reserved

This structure may grow in size in the future.

### 0x08 / 0x09 - MCU: Read / Write register (protocol 1.2+)

The parameter is the register address:

- 0x000: IRQ enable
  - bit 0: TF card inserted (edge)
  - bit 1: TF card removed (edge)
  - bit 2: RTC alarm (level)
- 0x001: IRQ status
- 0x002: IRQ status (automatic acknowledgement on read)

Each register is 2 bytes in size, and is passed as the argument or response.

### 0x0F - MCU: Get protocol version

The response is the *major*, followed by the *minor* version of the MCU protocol, as 16-bit little endian unsigned integers each.

| Firmware version | MCU protocol version |
|------------------|----------------------|
| 1.0.0+ | 1.0 |
| 1.0.1+ | 1.1 |
| 1.1.0+ | 1.2 |

### 0x10 - EEPROM: Set emulation mode

Set the size of the emulated EEPROM:

- `0x00` - no EEPROM
- `0x01` - M93LC06
- `0x02` - M93LC46 compatible
- `0x03` - M93LC56 compatible
- `0x04` - M93LC66 compatible
- `0x05` - M93LC76 compatible
- `0x06` - M93LC86 compatible

The response is 1 on success, 0 on failure.

### 0x11 - EEPROM: Erase all data

The response is empty.

### 0x12 - EEPROM: Read data

The parameter is the number of words to read; the following word is the offset in bytes.

The response is the bytes read.

### 0x13 - EEPROM: Write data

The parameter is the number of words to write; the following word is the offset in bytes, then the words to write.

The response is empty.

### 0x14 - RTC: Send command

The parameter is the packet type to send to the emulated S-3511A, followed by the relevant bytes.

The response is the data returned by the emulated S-3511A.

### 0x15 - EEPROM: Get emulation mode

The response is 1 byte - the EEPROM emulation mode.

### 0x16 - MCU: Set save ID

The parameter specifies the save ID location:

- `0x01`: SRAM2 (if the save ID depends on data stored in SRAM2, such as EEPROM),
- `0x02`: RTC backup domain (if it doesn't),
- `0x03`: both.

The command is followed by four bytes of the save ID.

The save ID `0xFFFFFFFF` is reserved and means "no save is stored".

The response is 1 on success, 0 on failure.

### 0x17 - MCU: Get save ID

The parameter specifies the save ID location.

The response is four bytes of the save ID.

### 0x40 - USB: CDC: Read

The parameter is the maximum number of bytes to read. The value 0 is treated as 512 bytes.

The response is the data read from the CDC.

### 0x41 - USB: CDC: Write

The parameter is the number of bytes to write. The value 0 is treated as 512 bytes.

The response is two bytes in size and is the number of bytes successfully written.

### 0x42 - USB: HID: Write

The parameter is the length of the packet.

The data sent is two bytes - the result of a keypad scan.

The response is zero bytes in size.

### 0x43 - USB: CDC: Available

The response is the number of bytes that can be read from the CDC.

### 0x44 - USB: CDC: Clear

Clears the write and read buffers, removing any unread data.

No parameter, no response data.

### 0x50 - Accelerometer polling control

If the parameter is non-zero, the accelerometer is powered up and polled at the frequency equal to the parameter; for example, if the parameter is 100, the accelerometer will be polled at 100 Hz.

The frequency range is 1 .. 510 Hz. The parameter value 511 is reserved. 

If the parameter is zero, accelerometer polling is disabled.

A one byte response is given, 1 if the operation was successful and the accelerometer is present and 0 for the opposite.

### 0x51 - Accelerometer read

Returns three signed 16-bit words representing the measured acceleration on three axis X, Y and Z.

````

Plane representing Swan as held when playing horizontally.

Coordinate system is the local coordinate system of the accelerometer.
-Z is equivalent to the normal of the plane.

               ^ +Y
              /
     ________/________
+X  /       /        /
<----------0        /
  /        |       /
 /_________|______/
           |
           v +Z

````

The accelerometer is configured to work in a range of +/- 2 g with 10 bit fractional precision. This means that when the console is resting, the accelerometer will read a vector of magnitude 1024 the direction opposite to Earth's gravitational pull. E.g. if the console sits on the side with the EXT port a value of approximately (1024, 0, 0) will be read. See also: https://en.wikipedia.org/wiki/Accelerometer#Physical_principles and the datasheet of the used MXC400xXC series accelerometer.

### 0x7F - Reserved

Reserved to distinguish 0xFF bytes from commands.
