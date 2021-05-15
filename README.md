![Build status](https://github.com/nimo-labs/m032HidBootloader/actions/workflows/build.yml/badge.svg)
# M032 Hid Bootloader - NIMOLIB based USB HID bootloader for Nuvoton M032 devices

A USB HID based bootloader for the Nuvoton M032 family of devices, currently the following devices are supported:
- M032LG6AE

## Building

The bootloader requres the following prerequisites:
- arm-none-eabi-gcc compiler
- [umake](https://github.com/nimo-labs/umake) build system

- Install prerequisites
- Run umake in the repository root directory to download the libraries and generate the Makefile
- Run make in the repository root directory to build the binary
- The final output is build/m032Hidbootloader.bin

