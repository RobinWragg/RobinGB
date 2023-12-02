# RobinGB
A simple, efficient, cross-platform Game Boy emulator library. It's written in C89 to allow it to compile on many microcontroller-oriented cross-compilers. It's also C++ friendly. RobinGB is a performant emulator, enabling it to run on very small devices with low clock speeds and less than 256KB of RAM. I hope for it to find a place in hackerspace and DIY IoT projects.

With Super Mario Land as a benchmark, RobinGB currently runs at full speed on a Teensy 3.6 (ARM Cortex-M4) at 240MHz, or at about 80% speed on one core of an ESP32.

Currently, RobinGB can only run games that use no MBC chip, or the MBC1 chip without external RAM, which is the vast majority of early Game Boy games. The notable exception is the Pokemon series, which uses the MBC3 chip or greater.

This code is entirely platform-agnostic, so to use this emulator, you'll need to implement your own function for file access. All the functions you need to call are in the small header, RobinGB.h.
