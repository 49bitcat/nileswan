---
title: 'Firmware restore'
weight: 30
---

The IPL1 (second stage bootloader) or FPGA bitstream data may be corrupted as the result of a failed regular update.
This can manifest as a blank screen or as a crash on the initial splash screen (before the menu program is loaded).

nileswan provides a way to boot from the original firmware flashed at manufacturing. One can force the use of the factory IPL1 and FPGA bitstream by proceeding as follows:

1. Hold the physical button at the top of the cartridge.
2. While holding the physical button, turn on the console.
3. Keep holding the physical button until the console's splash screen appears.

From here, you should be able to use the menu software to perform [a standard firmware update](../../updating).

## Factory reflash

Alternatively, if one so chooses, one can also perform a full factory reflash. This updates both the regular and manufacturing copies of the
firmware.

{{< hint type=warning >}}
If the factory reflash fails, the cartridge may be bricked in a way which requires external SPI flashing.

You are performing this procedure at your own risk!
{{< /hint >}}

1. Ensure your console's battery is fully charged.
2. Download the latest firmware reflash image version from [here](https://github.com/49bitcat/nileswan/releases/latest/download/nileswan-fw-reflash.ws).
3. Launch the `.ws` file.
4. After reviewing the messages, press `A` (or `Circle` on PCv2) to install.
5. Wait for the installation to complete. **Do not turn off the console** until the updater tells you it is safe to do so.
