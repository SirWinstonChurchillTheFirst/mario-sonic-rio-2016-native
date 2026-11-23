# Mario & Sonic at the Rio 2016 Olympic Games (Wii U): native build, work in progress

An attempt to run the Wii U game as a native Linux program by static recompilation: the game's PowerPC code is
translated to C++ ahead of time and runs on a reimplementation of the Wii U's system libraries. No emulator.

> **No game files are included.** You need your own dump of the game (Europe, title `00050000101E5400`, v0),
> unpacked to `code/`, `content/`, `meta/`. The translated C++ and the program built from it contain the game's
> code: do not share them. Not affiliated with or endorsed by Nintendo, SEGA or the IOC.

## Selbst bauen / Build it yourself

One command builds the native program from **your own dump** of Mario & Sonic at the Rio 2016 Olympic Games. Nothing from the game is in this repository
or downloaded by the scripts: the game's code (unison.rpx) is translated to C++ and compiled on your computer.
*Ein Befehl baut das native Programm aus deinem eigenen Dump; nichts vom Spiel ist hier enthalten oder wird geladen.*

**You need / Du brauchst**
- your own dump of the game, **unpacked**: a folder with `code/`, `content/`, `meta/` (as Cemu or a Wii U dumper
  such as Dumpling gives it; `.wua`/`.wud` images must be unpacked first). Tested with Europe
  (00050000101E5400); other versions are translated from your files the same way and may work.
- Windows 10/11 64-bit or Linux x86-64; a CPU with AVX2/FMA (Intel Haswell, AMD Zen or newer); a GPU with a Vulkan driver.
- internet for the build tools and the pinned sources (Smash4PC `f145e7f`, ZArchive, addrlib, Vulkan-Headers from GitHub).
- about **2 GB** free in this folder (plus about 3 GB for MSYS2 on Windows) and 8 GB of RAM or more.
- time: about **8-20 minutes** for the first build on an 8-core PC; a second run only redoes what changed.

**Miis / Mii-Dateien.** Visible Miis need the Mii resource title (`0005001B-10056000`, files `FFLRes*.dat`) from **your own Wii U** — it is not part of the game, not in this repository and never downloaded. This game does not get far without it. Give the setup a Cemu-style `mlc01/sys` folder that holds `title/0005001b/10056000/content/` (`-SysDir` on Windows, `--sys` on Linux); it is written into the starter as `CAFE_SYS_DIR` and only read.
*Deutsch:* Sichtbare Miis brauchen den Mii-Systemtitel deiner **eigenen** Wii U (z. B. aus Cemus `mlc01/sys`), angegeben mit `-SysDir` bzw. `--sys`. Er ist nicht im Spiel-Dump und nicht hier enthalten.

**Windows** (PowerShell):

```powershell
git clone https://github.com/SirWinstonChurchillTheFirst/mario-sonic-rio-2016-native
cd mario-sonic-rio-2016-native
powershell -ExecutionPolicy Bypass -File .\setup-windows.ps1 -GameDir "D:\Games\Mario & Sonic at the Rio 2016 Olympic Games" -SysDir "D:\Cemu\mlc01\sys"
.\play\Start.bat
```

It installs [MSYS2](https://www.msys2.org) with winget if it is missing and the build packages into its CLANG64
environment with pacman (clang, lld, cmake, ninja, Python, git, SDL3, Vulkan loader + headers, shaderc, zlib, zstd,
xz, DejaVu fonts). Without `-GameDir` it asks for the folder. `-Jobs N` limits the parallel compile jobs; the build
runs at low priority so the PC stays usable.

**Linux** (Debian/Ubuntu, Arch, Fedora; others: install the same packages yourself):

```bash
git clone https://github.com/SirWinstonChurchillTheFirst/mario-sonic-rio-2016-native
cd mario-sonic-rio-2016-native
./setup-linux.sh "/path/to/Mario & Sonic at the Rio 2016 Olympic Games" --sys ~/.local/share/Cemu/mlc01/sys
./play/start.sh
```

Missing packages are installed with sudo (apt: clang cmake ninja-build git python3 pkg-config zlib1g-dev libzstd-dev
libssl-dev libvulkan-dev libshaderc-dev fonts-dejavu-core, and libsdl3-dev where the distribution has it; without it
SDL 3.2.26 is built from source, e.g. on Ubuntu 24.04). `--jobs N` limits the compile jobs.

**What the setup does** (`tools/native-build.sh`, the same on both systems): checks your dump; clones
[Smash4PC](https://github.com/josiahg2502-glitch/Smash4PC) at `f145e7f` and the three pinned dependencies; applies
`port/upstream-local.patch` (all runtime/recompiler changes, made by the `port/patch_*.py` scripts in order); builds the
recompiler `cafe-recomp`; translates your `code/`; compiles runtime + translated game; and puts `cafegame`, a stand-in
font and the starter into `play/`. Everything it makes stays in `build/` and `play/` (both ignored by git): they hold
code translated from the game — **keep them for yourself, do not share them.**

The Wii U system font is not part of a game dump: DejaVu Sans stands in (`play/fonts/CafeStd.ttf`); put your console's
own `CafeStd.ttf` there to replace it. Saves go to `play/save/`. The system language follows your PC
(`CAFE_LANGUAGE=de|en|fr|...` overrides it).

**Getestet / tested (23 Nov 2026):** For this game the Windows setup's translation step was run on the tested dump: it gives the same code as the build that runs on the Batocera console (only the newer recompiler's `dcbf` lines differ); the complete build of this game through the scripts was not run yet. The same scripts were run end to end from an empty folder on 23 November 2026: Lost Levels and Mario Kart 8 on Windows 11, Lost Levels on Ubuntu 26.04 and on a fresh Ubuntu 24.04 (SDL3 built from source there), Mario Party 10 on Windows 11.

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
- `setup-windows.ps1`, `setup-linux.sh`, `tools/native-build.sh`, `game.conf`: the one-command build (above).
  `port/iter.sh` is the build-and-run loop used while porting.
- `tools/rpxinfo.py`: prints facts about an RPX (code size, function symbols, imports).

## License

The files here are under the MIT license, as the projects they build on. Third-party code keeps its own license.
