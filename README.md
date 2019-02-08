# RobinGB
A Game Boy emulator in development for low-power devices, written in ANSI-C for maximum portability.

With Super Mario Land as a benchmark, RobinGB runs at approximately 80% of full speed on a 240MHz MCU such as an ESP32 (single-threaded) or an overclocked Teensy 3.6. This is slow mainly due the rasteriser. Optimisation is planned, but the current focus is on adding support for more games.

Although RobinGB is designed for performance rather than emulation accuracy, no compromises have had to be made so far and it passes all of Blargg's CPU instruction tests, apart from the interrupt tests. There's no audio output yet, but development on that will start once Pokemon Red & Blue are playable.

## Inexhaustive list of working games
Asteroids, Bomb Jack, Boxxle, Brainbender, Bubble Ghost, Castelian, Catrap, Cool Ball, Crystal Quest, Dr. Mario, Flipull, Heiankyo Alien, Kwirk, Loopz, Missile Command, Motocross Maniacs, Pipe Dream, Q Billion, Serpent, Shanghai, Super Mario Land, Tasmania Story, Tennis, Tesserae, Tetris, World Bowling.
