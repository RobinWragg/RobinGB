# RobinGB
A Game Boy emulator in active development. It is written in C89 to allow it to compile on many microcontroller-oriented cross-compilers. RobinGB is a very efficient emulator, enabling it to run on very small devices with low clock speeds and less than 256KB of RAM.

With Super Mario Land as a benchmark, RobinGB currently runs at full speed with approximately 20% headroom on a 240MHz MCU such as the ESP32 (single-threaded). Further optimisation is planned, but the current focus is on adding support for more games.

Although RobinGB is designed for performance rather than emulation accuracy, no compromises have had to be made so far and it passes all of Blargg's CPU instruction tests, apart from the interrupt tests. There's no audio output yet, but development on that will start once Pokemon Red & Blue are playable.

## Inexhaustive list of working games
Asteroids, Bomb Jack, Boxxle, Brainbender, Bubble Ghost, Castelian, Catrap, Cool Ball, Crystal Quest, Dr. Mario, Flipull, Heiankyo Alien, Kwirk, Loopz, Missile Command, Motocross Maniacs, Pipe Dream, Q Billion, Serpent, Shanghai, Super Mario Land, Tasmania Story, Tennis, Tesserae, Tetris, World Bowling.
