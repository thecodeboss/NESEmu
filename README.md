NESEmu
======

NESEmu (probably not a very original name) is an NES emulator written in C++ (with a few C++11 features scattered).  It was written for fun, so it is not designed to support every NES game out there.  However it does support a few important ones such as Super Mario and Legend of Zelda.  It also supports saved files for games that had them.

Compilation
===========

In order to compile, you'll need Visual Studio 2012.  The solution should be in a state as soon as you download it to simply compile without any link errors.  If you run into any problems, shoot me a message.  My email is below.

Emulating
=========

To run the emulator, you need to have compiled it, and you can then run it in Visual Studio, or standalone from the command line.  When running it, the command line should be as follows:

    NESEmu path/to/game.nes [framedump [audiodump]]

framedump: Will create a 'frames' directory in your working directory to dump every frame while you are playing (in BMP format)
audiodump: Will output an 'output.wav' file in your working directory upon exiting the application, with the entire audio contents of your playthrough.

Frame/audio dumps must be manually concatenated using software such as FFMPEG.  Be aware you may need to figure out any sync issues yourself.

Questions
=========

Any questions or ideas (I love discussion on my work), feel free to email me at admin@thecodeboss.com.

Disclaimers
===========

This application is designed purely for educational purposes, and I am not attempting to infringe on any copyright materials.  Any and all data from Nintendo game cartridges used with the application will NOT be distributed by me, but rather you must own the game cartridge and not distribute it for any reason.

This code is yours, free to use and modify, assuming you do not use it for any form of profit without my written permission.
