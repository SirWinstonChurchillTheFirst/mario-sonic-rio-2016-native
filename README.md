# Mario & Sonic at the Rio 2016 Olympic Games (Wii U): native build, work in progress

An attempt to run the Wii U game as a native Linux program by static recompilation: the game's PowerPC code is
translated to C++ ahead of time and runs on a reimplementation of the Wii U's system libraries. No emulator.

> **No game files are included.** You need your own dump of the game (Europe, title `00050000101E5400`, v0),
> unpacked to `code/`, `content/`, `meta/`. The translated C++ and the program built from it contain the game's
> code: do not share them. Not affiliated with or endorsed by Nintendo, SEGA or the IOC.

## Status (19 November 2026): does not reach gameplay

| Step | State |
| --- | --- |
| Translate `unison.rpx` (84,281 functions, 4.87 million instructions) | works |
| Build with the runtime | works (Linux; Windows not tried) |
| Start-up, Rio 2016 logo and copyright screen | shown correctly |
| Autosave notice, accepted with A | works (text needs a system font, see below) |
| "Now loading..." after the title | reached, then the game stops with a null access (guest address 0x4, at `0x0215B4FC`) |
| Menus, events, gameplay | **not reached** |
| Sound | a device is opened; nothing has been listened to. Reverb, chorus and delay effects are not computed |
| Speed | not measured: tested only on a software Vulkan renderer (about 11 frames/s) |

Known gaps: the game asks for the console's Mii data files (`FFLResMiddle.dat`, `FFLResHigh.dat`), which are not
part of a game dump; `GX2ResetGPU` does nothing; `GX2CopySurfaceEx` copies whole surfaces only; online services
answer "offline".

## What is here

This repository holds only what was added for this game, on top of
[Smash 4 PC](https://github.com/josiahg2502-glitch/Smash4PC) at `f145e7f` (MIT), which is built on
[Tekken Tag 2 Ultimate Edition](https://github.com/Petexy/Tekken-Tag-2-Ultimate-Edition)'s cafe-recomp and
Wii U runtime by Petexy (MIT). All credit for the recompiler and the runtime goes to them.

- `port/upstream-local.patch`: changes to that runtime and recompiler (`port/patch_*.py` are the scripts that made
  them, kept as a record of each change and why):
  - integer arguments beyond the eighth are read from the caller's stack (PowerPC EABI);
  - `GX2SetEventCallback` callbacks are really called (vsync, flip, end-of-pipe interrupt);
  - a user timestamp is written low word first: this game's callback reads it so, and it reported a GPU timeout
    twice a second with the other order (not checked against Tekken or Smash);
  - `FSGetPosFile`, `FSIsEof`, `FSGetVolumeState`, the async read/write/position calls, `MEMFreeToFrmHeap`,
    `GX2GetCurrentDisplayList`, `GX2CopySurfaceEx` (whole surface);
  - relocation kinds 109, 251, 252, 253 in the RPX loader (needed by another title, untested at run time);
  - the Windows-only netplay files are left out on Linux.
- `port/game_main.cpp`: a game-neutral entry program. `port/os_extra.cpp`: system functions other titles need.
  `port/netplay_none.cpp`: stand-in for the left-out netplay part.
- `port/setup.sh GAME_FOLDER NAME`: the build steps in order. They were done by hand; the script as a whole has
  not been run yet. `port/run.sh NAME GAME_FOLDER` starts the result. `port/iter.sh` is the build-and-run loop
  used while porting.
- `tools/rpxinfo.py`: prints facts about an RPX (code size, function symbols, imports).

The system font is not part of a game dump either: copy your console's `CafeStd.ttf` to `build-NAME/fonts/`;
until then `setup.sh` puts DejaVu Sans there, and texts look different from the console's.

## License

The files here are under the MIT license, as the projects they build on. Third-party code keeps its own license.
