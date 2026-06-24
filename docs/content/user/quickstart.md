---
title: 'Quickstart'
weight: 0
---

To set up nileswan, you're going to need a removable storage card, sometimes referred to as a TF card, formatted
using the FAT16 or FAT32 file system.

{{< hint type=note >}}
Cards over 32 gigabytes may be formatted using the exFAT file system by default, which is not supported.

To ensure correct storage card formatting, you may wish to follow [the formatting guide](../troubleshooting/storage-card-formatting).
{{< /hint >}}

You're also going to need a *menu program*. This is the first program launched by nileswan's boot loader, stored in `/NILESWAN/MENU.WS`. The following options are currently available:

* **[swanshell](https://docs.asie.pl/swanshell/user/installation)** - the official main menu program.

{{< hint type=caution >}}
The boot loader doesn't support loading arbitrary programs. Do not use this functionality as an autoboot method; you will be disappointed.
{{< /hint >}}

With a menu program installed, you should be able to turn on your console with the nileswan cartridge inserted. Enjoy!

{{< hint type=note >}}
Unfortunately it is not always possible to ship a nileswan with a battery preinstalled. Please see this [guide](https://www.49bitcat.com/products/nileswan/battery_swap/) on suitable coin cell batteries and how to install them.

There should be a note accompanying your nileswan if the battery could not have been included, though swanshell will also warn you if it does not detect a battery.
{{< /hint >}}

If you run into problems, make sure to check the [Troubleshooting](../troubleshooting) section.
