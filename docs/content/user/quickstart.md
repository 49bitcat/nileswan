---
title: 'Quickstart'
weight: 0
---

To set up nileswan, you're going to need a removable storage card, sometimes referred to as a TF card, formatted
using the FAT16 or FAT32 file system.

{{< hint type=note >}}
nileswan supports cards of up to 2 terabytes in size. However, cards over 32 gigabytes may be formatted using the exFAT file system by default, which is not supported.

If you run into issues, follow the [Storage card formatting](../troubleshooting/storage-card-formatting) guide.
{{< /hint >}}

You're also going to need a *menu program*. This is the first program launched by nileswan's boot loader, stored in `/NILESWAN/MENU.WS`. The following options are currently available:

* **[swanshell](https://docs.asie.pl/swanshell/user/installation)** - the official main menu program.

{{< hint type=caution >}}
The boot loader doesn't support loading arbitrary programs. Do not use this functionality as an autoboot method; you will be disappointed.
{{< /hint >}}

With a menu program installed, you should be able to turn on your console with the nileswan cartridge inserted. Enjoy!

If you run into problems, make sure to check the [Troubleshooting](../troubleshooting) section.
