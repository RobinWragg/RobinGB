# RobinGB
A Game Boy emulator in active development. It's written in C89 to allow it to compile on many microcontroller-oriented cross-compilers. It's also C++ friendly. RobinGB is a very efficient emulator, enabling it to run on very small devices with low clock speeds and less than 256KB of RAM. I hope for it to find a place in hackerspace and DIY IoT projects.

With Super Mario Land as a benchmark, RobinGB currently runs at full speed with approximately 20% headroom on a 240MHz MCU such as the ESP32 (single-threaded) or an overclocked Teensy 3.6. Further optimisation is planned, but the current focus is on developing the audio implementation and adding support for more games.

Currently, RobinGB can only run games that use no MBC chip, or the MBC1 chip without external RAM (the vast majority of early Game Boy games). Work is currently being done to support MBC1 games that contain RAM, including battery savestates. Support for MBC2 and MBC3 is planned.

This code is entirely platform-agnostic, so to use this emulator, you'll need to implement your own function for file access. All the functions you need to call are in the small header, RobinGB.h.
