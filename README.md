# Mario & Sonic at the Rio 2016 Olympic Games (Wii U): native build, work in progress

An attempt to run the Wii U game as a native Linux program by static recompilation: the game's PowerPC code is
translated to C++ ahead of time and runs on a reimplementation of the Wii U's system libraries. No emulator.

> **No game files are included.** You need your own dump of the game (Europe, title `00050000101E5400`, v0),
> unpacked to `code/`, `content/`, `meta/`. The translated C++ and the program built from it contain the game's
> code: do not share them. Not affiliated with or endorsed by Nintendo, SEGA or the IOC.

## Status (22 November 2026, evening): playable, events run at 60 frames/s on a Batocera console

| Step | State |
| --- | --- |
| Logo, save notice, map, "Now loading...", the hub town (Copacabana) with your Mii walking, dialogs | works (PC and console) |
| Discipline and character select | works |
| An event | works: a football duel played through on the PC and twice on the console (220 s and 280 s), no crash |
| Speed | console (Ryzen 3 4350GE, Vega 6): 54-60 frames/s in events, 30 in the hub town; CPU max 76 C, GPU max 67 C |
| Sound | works |

**The crash is fixed.** The game's renderer (`hh::rsdx`) hands out memory from lock-free lists built on
`lwarx`/`stwcx.`. The runtime emulated that pair as a bare compare-and-swap on the value, which has the ABA problem:
another thread could take the head entry and the next one and put the head back in between, the swap still
succeeded and an entry in use was handed out again (null shaders, wild pointers, an assertion in
`RsdxAtomicPopSList`). `port/patch_reservation.py` gives every `stwcx.` a reservation: a version counter per
32-byte line, bumped by every `stwcx.`, which makes a `stwcx.` fail when another thread stored there since its
`lwarx`, as on the hardware.

**Miis.** The game needs the Wii U's Mii resources (system title `0005001B-10056000`: `FFLResMiddle.dat`,
`FFLResHigh.dat`). They are **not** part of the game and **not** included here: visible Miis need the Mii resource
title from your own Wii U. Point `CAFE_SYS_DIR` at a Cemu-style `mlc01/sys` folder that holds
`title/0005001b/10056000/content/` (the console launcher uses `/userdata/saves/wiiu/sys` when it exists). The account
Mii comes from `CAFE_ACCOUNT_DAT` (a Cemu `account.dat`) or is a generated default Mii. The Mii databases
(`FFL_ODB.dat`, `FFL_HDB.dat`) are made empty in the port's own save folder.

What made it run past the loading screen (all in `port/`): `patch_sys_dir.py`, `patch_mlc_usr.py` (Mii Maker's shared
save and its title id), `patch_default_mii.py`, `patch_ffl_odb.py` (an empty Mii database as FFL itself writes one; the
game treats FFL's "database was just created" result as a failure).

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
