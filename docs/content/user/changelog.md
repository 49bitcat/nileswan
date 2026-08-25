---
title: 'Changelog'
weight: 100
---

## nileswan firmware 1.2.6 (25th August 2026)

- MCU firmware
  - Fixed: Improved battery power management.

## nileswan firmware 1.2.5 (21st June 2026)

- MCU firmware
  - Fixed: Issue regarding EEPROM memory management.
- Recovery
  - Added: SRAM/EEPROM data retention tests.
  - Added: Battery-backed MCU information lookup and tests.
  - Changed: Re-arranged menu hierarchy to be more user friendly.

## nileswan firmware 1.2.4 (16th June 2026)

- MCU firmware
  - General reliability improvements.

## nileswan firmware 1.2.3 (15th June 2026)

- FPGA core
  - General reliability improvements.
- MCU firmware
  - General reliability improvements.

## nileswan firmware 1.2.2 (7th June 2026)

- MCU firmware
  - General reliability improvements.
- Recovery IPL
  - Added: New quick memory test, focused on detecting address/data line faults.
  - Added: Visible warning for 24 MHz oscillator faults.
  - Fixed: Extended memory test failing all subsequent banks if one fails.
  - Fixed: ID readout in cases where the 24 MHz oscillator is non-functional.
- Updater
  - Added: Expanded SPI flash chip support.

## nileswan firmware 1.2.1 (13th May 2026*)

- Recovery
  - Changed: Tweaked manufacturing test.
- As this version does not change anything for end users, it will not be
  released as a flashable update.

## nileswan firmware 1.2.0 (10th May 2026)

- Factory FPGA core
  - Fixed: PCv2 consoles always starting in slow SPI mode.
  - Fixed: Improved startup reliability on aging "mono" WS consoles.
  - Note: These fixes requires a full reflash to be performed.
- MCU firmware
  - Fixed: Issues when querying the accelerometer while USB is plugged in.
  - Fixed: Issues when querying the onboard battery status.
- Recovery
  - Fixed: PCv2 input layout mapping.
- Updater
  - Fixed: PCv2 input layout mapping.

## nileswan firmware 1.1.1 (29th March 2026)

- MCU firmware
  - Fixed: Gamepad mode not functioning correctly on Windows machines.
  - Fixed: Issues when using the EEPROM mode while USB is plugged in.
  - Fixed: Worked around certain RTC mode reliability issues.
- Recovery
  - Added: MCU EEPROM test.

## nileswan firmware 1.1.0 (5th March 2026)

- FPGA core
  - Added: Emulation of the 0xCF I/O port mirror.
  - Added: Hardware support for faster SPI buffer access methods.
  - Fixed: Disabled I/O ports remaining readable.
  - Other minor improvements.
- Bootloader
  - Changed: Improved startup time.
- MCU firmware
  - Added: Support for detecting TF card insertion/removal.
  - Added: Support for reporting MCU status information back to console.
  - Fixed: Potential crash after disconnecting USB cable.
  - Fixed: Uncommon glitches during RTC communication.
- Recovery
  - Added: MCU status query tool.
  - Added: TF card insertion/removal test.
  - Fixed: Updated recovery program not using the updated FPGA core.

## nileswan firmware 1.0.3 (14th February 2026)

- MCU firmware
  - Updated: TinyUSB.
- Recovery
  - Added: onboard button test.
  - Added: PSRAM-SRAM stability test.
  - Added: TF card stability test.
  - Changed; Improved TF card benchmarks.
  - Fixed: Issues with TF card formatting.
- Other minor fixes and improvements.

## nileswan firmware 1.0.2 (18th November 2025)

- Bootloader:
  - Minor load time optimizations (~3-5% improvement).
- Recovery:
  - Fixed soft reset functionality.
  - Fixed storage card benchmarks not working without a forced re-initialization in some cases.
  - Fixed storage card formatting not working with some cards.
- Other minor fixes and improvements.

## nileswan firmware 1.0.1 (10th November 2025)

- MCU firmware:
  - Fixed response logic for undefined MCU commands.
  - Minor improvements to RTC and USB power management.
- Recovery:
  - Added support for printing cartridge information via USB.
  - Added support for printing the active MCU firmware's protocol version.
- Other minor fixes and improvements.

## nileswan firmware 1.0.0 (29th October 2025)

- Initial release.
