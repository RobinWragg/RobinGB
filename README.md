# RobinGB
A Game Boy emulator in active development. It is written in C89 to allow it to compile on many microcontroller-oriented cross-compilers. RobinGB is a very efficient emulator, enabling it to run on very small devices with low clock speeds and less than 256KB of RAM.

With Super Mario Land as a benchmark, RobinGB currently runs at full speed with approximately 20% headroom on a 240MHz MCU such as the ESP32 (single-threaded) or an overclocked Teensy 3.6. Further optimisation is planned, but the current focus is on adding support for more games.

Although RobinGB is designed for performance rather than emulation accuracy, no compromises have had to be made so far and it passes all of Blargg's CPU instruction tests, apart from the interrupt tests. There's no audio output yet, but development on that will start once Pokemon Red & Blue are playable.

The emulator can currently only run games that use no MBC chip or the MBC1 chip without external RAM (the vast majority of Game Boy games). Work is currently being done to support MBC1 games that contain RAM, including battery savestates. Support for MBC2 and MBC3 is planned.

This code is entirely platform-agnostic, so to use this emulator, you'll need to implement your own function for file access. All the functions you need to call are in the 37-line header, RobinGB.h.
