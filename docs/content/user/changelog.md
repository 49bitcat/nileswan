---
title: 'Changelog'
weight: 100
---

## nileswan firmware 1.1.0 (Eventually)

- FPGA core
  - Added: Emulation of I/O port 0xCF.
  - Added: Hardware support for faster SPI buffer access methods.
- Bootloader
  - Changed: Improved startup time.
- MCU firmware
  - Added: Support for detecting TF card insertion/removal.
  - Added: Support for reporting MCU status information back to console.
  - Fixed: Potential crash after disconnecting USB cable.
- Recovery
  - Added: MCU status query tool.
  - Added: TF card insertion/removal test.

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
