# nileswan

Open software, WS/WSC/PCv2 compatible flash cartridge.

## Building instructions

### Requirements

- [OSS CAD Suite](https://github.com/YosysHQ/oss-cad-suite-build) (FPGA core)
  - Requires a modified version of `icepack`, see below for more information
- [Wonderful Toolchain](https://wonderful.asie.pl/wiki/doku.php?id=getting_started)
  - `target-wswan` (IPL1, recovery, updater)
  - `toolchain-gcc-arm-none-eabi` (MCU firmware)
  - `wf-superfamiconv` (IPL1)
  - `wf-zx0-salvador` (IPL1, updater)
- CMake (MCU firmware)
- NASM (IPL0)
- Nim 2.0+ (FPGA core, IPL0)
- Ninja (MCU firmware)
- Python 3.x (SPI images, updater)
  - Including external libraries: `crc`
- dd, dosfstools, mtools (emulator images)

#### icepack

For the FPGA to be ready in time, the bitstream needs to be loaded at the highest speed possible. I created a [PR](https://github.com/YosysHQ/icestorm/pull/332) for icepack which adds a setting to change this. Without it, it is not possible to compile the FPGA bitstream.

### Compiling

1. Make sure to fetch the Git submodules: `git submodule update --init --recursive`.
2. Run `make help` to read what build options are possible.
3. Run `make` (or `make ...`) to build the requested components.

## License

- `docs/`: [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/) (user and developer documentation)
- `firmware/`: GNU GPLv3+ (MCU firmware)
- `shell/`: [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/) (cartridge shell STL files)
- `software/`: GNU GPLv3+ (cartridge IPL and on-device tools)
  * `software/libnile/`: zlib AND FatFs license
