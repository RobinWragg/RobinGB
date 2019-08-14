# RobinGB
A Game Boy emulator in active development. It is written in C89 to allow it to compile on many microcontroller-oriented cross-compilers. RobinGB is a very efficient emulator, enabling it to run on very small devices with low clock speeds and less than 256KB of RAM.

With Super Mario Land as a benchmark, RobinGB currently runs at full speed with approximately 20% headroom on a 240MHz MCU such as the ESP32 (single-threaded). Further optimisation is planned, but the current focus is on adding support for more games.

Although RobinGB is designed for performance rather than emulation accuracy, no compromises have had to be made so far and it passes all of Blargg's CPU instruction tests, apart from the interrupt tests. There's no audio output yet, but development on that will start once Pokemon Red & Blue are playable.

The emulator can currently run games that use the RAM-less MBC1 chip (that's the vast majority of Game Boy games). Work is currently being done to support MBC1 games that contain RAM, including battery savestates. Support for MBC2 and MBC3 is planned.
