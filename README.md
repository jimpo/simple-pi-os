# Simple Pi OS

This is a partial operating system for a Rasberry Pi A+, written for CS 140e.

## Running the OS

### Dependencies
- [CMake](https://cmake.org/)
- [FUSE](https://github.com/libfuse/libfuse)
- [GNU Arm Embedded Toolchain](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm)

On Ubuntu/Debian:
```bash
$ sudo add-apt-repository ppa:team-gcc-arm-embedded/ppa
$ sudo apt-get update
$ apt-get install gcc fuse libfuse-dev make cmake
```

### Installation

- Copy the files in `firmware/` to the root directory of the Pi's SD card ([source](https://github.com/raspberrypi/firmware)).
- Build the project
```bash
$ mkdir build
$ cd build
$ cmake ..
$ make
```
- Build and copy `pi-side/bootloader/bootloader.bin` to the root directory of the Pi's SD card.
- Run `make run-shell` to interact with the device.

## Features

The operating system kernel is loaded into the Pi's memory from the bootloader over the UART. The kernel implements virtual memory, reading of files from disk, execution of programs read from the SD card.

## References

[BCM ARM Peripherals](https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf)
