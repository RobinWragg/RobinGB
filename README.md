# RobinGB
A simple, efficient, cross-platform Game Boy emulator library. It's written in C89 to allow it to compile on many microcontroller-oriented cross-compilers. It's also C++ friendly. RobinGB is a performant emulator, enabling it to run on very small devices with low clock speeds and less than 256KB of RAM for most games. I hope for it to find a place in hackerspace and DIY IoT projects.

With Super Mario Land as a benchmark, RobinGB currently runs at full speed on a Teensy 3.6 (ARM Cortex-M4) at 240MHz, or at about 80% speed on one core of an ESP32.

Currently, RobinGB can only run games that use no MBC chip, or the MBC1 chip without external RAM, which is the vast majority of early Game Boy games. The notable exception is the Pokemon series, which uses the MBC3 chip or greater.

## Basic Usage

1. Do `#include "RobinGB.h"` and add the .c files to your build system.
2. Call `robingb_init(...)` to set the path to your .gb file.
3. Call `robingb_update_screen(...)` 60 times per second to run the emulation and get the pixel data for each frame.
4. Call `robingb_press_button(...)` and `robingb_release_button(...)` to convey the player's input to the emulator.

Full details, including audio, saving/loading, and alternative functions for rendering are all explained in RobinGB.h.