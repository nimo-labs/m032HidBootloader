![Build status](https://github.com/nimo-labs/m032HidBootloader/actions/workflows/build_master.yml/badge.svg)
![Build status](https://github.com/nimo-labs/m032HidBootloader/actions/workflows/build_dev.yml/badge.svg?branch=dev)
# M032 Hid Bootloader - NIMOLIB based USB HID bootloader for Nuvoton M032 devices

A USB HID based bootloader for the Nuvoton M032 family of devices, currently the following devices are supported:
- M032LG6AE
- ATSAMD21E17A

## Building

The bootloader requres the following prerequisites:
- arm-none-eabi-gcc compiler
- [umake](https://github.com/nimo-labs/umake) build system

To build...
- Install prerequisites
- Create a symlink from umakefile to the microcontroller you wish to build. E.g. `ls -s umakefile-m032 umakefile` Technically, `buildall.sh` will build all of the umakefiles within the current directory, however it is tailored to github's build system so may need tweaking to use locally.
- Run umake in the repository root directory to download the libraries and generate the Makefile
- Run make in the repository root directory to build the binary
- The final output is build/m032Hidbootloader.bin

## Usage

Currently the bootloader assumes that the application code starts in Flash at 0x3000, this may be modified in the future as different devices, usecases etc. become known. Ultimately it's likely that it will become user configurable with a default rather than hard coded.
There is a [companion desktop app](https://github.com/nimo-labs/hid_bootloader_console_client) that can be found.

