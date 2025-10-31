---
title: 'Recovery menu'
weight: 10
---

nileswan features a recovery menu.

1. Start the console while holding the **X3 + B** button combination (or **Esc + Up** on PCv2) to boot into safe mode. 

![](/docs/nileswan/img/ipl1_safe.png)

2. Use the up and down buttons to navigate to the **launch recovery** option, then press A (or **Circle** on PCv2).

![](/docs/nileswan/img/recovery_main.png)

### Internal EEPROM recovery

Some console issues are caused by internal EEPROM corruption:

- a freeze on start can indicate corrupt splash screen data,
- LCD display issues can indicate corrupt LCD timing data on SC consoles with a TFT panel.

nileswan provides tools which may help in repairing a console with corrupt internal EEPROM data.

#### Disable custom splash

To disable custom splash on boot, one can use the following recovery feature:

1. Hold the button at the top of the cartridge.
2. While holding the button, turn on the console.

This forces the console's IPL to skip any splash screen data in the internal EEPROM.

A permanent solution is offered by the `Internal EEPROM recovery -> Disable custom splash` option in the cartridge recovery program.

#### Restore TFT panel data

To fix TFT panel display issues on SC consoles, one can use the `Internal EEPROM recovery -> Restore TFT panel data` option in the cartridge recovery program.

### Storage card management

This menu provides features related to managing the removable storage card, as well as evaluating its performance.

### Setup MCU boot flags

This is a manufacturing option used to configure the boot sequence of the microcontroller used on nileswan.

### Cartridge self-test

The cartridge comes with a set of self-test features used during manufacturing. These may allow you to diagnose certain hardware and firmware faults yourself. You can also run them in sequence with the `Run manufacturing tests` option.
