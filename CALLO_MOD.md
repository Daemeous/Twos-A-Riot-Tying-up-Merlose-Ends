# Two's A Riot: Tying up Merlose Ends

*A Vagrant Story mod: Callo Merlose as a companion (preview).*

Callo Merlose joins Ashley as a fully AI-controlled party member. You choose her equipment and
which of Ashley's spells she may cast; she fights, heals and follows on her own.

This is a **preview**: most features have been playtested, some are still being tested (see
[Status](#status)). Keep backups of your saves.

## Features

- **She fights beside you.** Callo appears next to Ashley in every room, attacks enemies, heals
  and casts spells. Enemies fight back against her, including ones that arrive mid-room. She
  can be defeated, and comes back in the next room.
- **Her stats grow with Ashley's.** She copies Ashley's base stats when she appears, with STR
  and INT swapped: a strength-built Ashley gets a caster Callo, and the other way round.
- **MERLOSE equipment menu.** ITEMS → OPTIONS has a new MERLOSE entry next to EQUIP and SETUP.
  It uses the normal equip screens with her stats and her model, including the sheathe and draw
  animations. An item can only be worn by one of them at a time. Her gear is kept in your save
  and grows (classes, affinities) like Ashley's.
  - She starts with copies of Ashley's starting armour and a bow, *VKP Standard Issue*, as real
    items, if your inventory has room.
  - She can use daggers, swords, axes/maces and crossbows: weapon types she has no animations
    for aren't listed.
- **Choose her spells in the MAGIC menu.** In each school's list, **Square** gives the
  highlighted spell to Callo or takes it back (up to 8). Her spells are shown in the other text
  colour. She always knows Heal.
- **Friendly targeting.** Ashley's attacks, Break Arts and attack spells can't target her; his
  healing and support spells, and items, can.
- **Quality of life.**
  - She doesn't count as an enemy left in the room, so doors that wait for every enemy to be
    beaten still open.
  - The STATUS screen shows her stats rather than ???.
  - Small HP/MP/RISK bars for her sit under Ashley's.
  - If she's left far behind (a cutscene, scenery), she reappears beside Ashley after a few
    seconds. Opening and closing MERLOSE also brings her back to your side.
- **Extra:** permanent stat-up items (STR/INT/AGI/HP/MP-up) always give their maximum, +4.

It runs on a standard 2 MB PlayStation; no 8 MB RAM option needed.

## Compatibility

- **Game:** Vagrant Story, USA (SLUS-01040). You need your own copy: nothing from the game is in
  this repository, and the build reads your disc image.
- **Zenith:** supported and tested (Zenith 1.94).
- **Vagrant Story Randomizer** (ChaoticBrave, 0.9.2.0): supported, not yet playtested. The
  randomizer replaces whole game files with its own copies, so the mod has to go on **after**
  it (step 6). The files it ships have been checked against every change the mod makes.
- **Order:** your disc → Zenith → randomizer → this mod. Zenith and the randomizer are both
  optional.
- **Emulator:** developed in DuckStation.

## Building

You need Linux. On Windows, use WSL (Windows Subsystem for Linux) with Ubuntu.

### 1. Install the tools (once)

```sh
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
    g++ binutils-mipsel-linux-gnu git make python3 python3-venv unzip wget
```

### 2. Get the code

```sh
git clone -b callo-preview https://github.com/Daemeous/Twos-A-Riot-Tying-up-Merlose-Ends.git
cd Twos-A-Riot-Tying-up-Merlose-Ends
```

### 3. Add your disc image

Copy your **unmodified** Vagrant Story (USA) disc image, the single-track `.bin`, to
`disks/SLUS-01040.bin`. The build compiles against it. If your copy is a different format,
convert it to a BIN/CUE first.

### 4. Build the game once

```sh
make -j$(nproc)
```

The first run downloads the decompilation's tools and compilers, extracts the game's files from
your disc and checks they match: this takes a while. If it reports that your disc doesn't match,
your image isn't the USA release or isn't a clean dump.

### 5. Build the mod

```sh
bash tools/mod/build_mod.sh
```

This makes the **patch kit** in `build/mod/kit`: the mod's two files and a list of every change
it makes to the game. (It also writes a modded disc, `build/disks/SLUS-01040.bin`, without Zenith
or the randomizer.)

### 6. Patch your disc

1. Make a copy of your disc image to play, with a copy of its `.cue`. If you rename the `.bin`,
   open the `.cue` in a text editor and change the name on its `FILE` line to match: otherwise
   it still loads the original, unpatched disc.
2. Optionally apply Zenith to the copy, with any PPF tool.
3. Optionally run the randomizer on the copy.
4. Last, apply the mod, from the repository's folder:

   ```sh
   python3 tools/mod/patch_image.py build/mod/kit path/to/your-copy.bin
   ```

The patcher checks every byte it's about to change. If another mod changed one of them, it stops
without touching the disc and says where. Running it twice does nothing.

On WSL, a disc image on a Windows drive (`/mnt/c`, `/mnt/d`...) can give I/O errors: run the
same command with Windows' own Python from a Windows terminal instead. The patcher needs only
Python 3, so you can also copy `tools/mod/patch_image.py`, `tools/mod/apply_ppf.py` and the
`build/mod/kit` folder anywhere and run it there.

**Updating the mod:** patch a fresh copy (step 6 again). The patcher refuses a disc patched by a
different version of the mod.

## Status

**Playtested:**
- spawning and following
- the AI fighting beside Ashley
- enemies fighting back
- the MERLOSE menu and her saved equipment
- her starting bow and armour
- respawning after MERLOSE
- Zenith compatibility
- Ashley's attacks no longer targeting her

**Built, still being tested:**
- the equip animation
- mid-room enemies
- following after cutscenes
- room clearing
- every zone outside the Wine Cellar
- gear growth
- spell choice and her buff spells
- the HP/MP/RISK bars, which are a first version without labels or numbers
- stat-up items

**Known gaps:**
- With Zenith, there's no stat roulette after bosses; restoring it is being looked into.
- Her bars don't show numbers yet.

## How it works (for modders)

The mod is built from C source (`src/BATTLE/BATTLE.PRG/mod/`) against the decompilation and
never moves any of the game's code:

- `callo.c` / `merlose.h`: the resident part, loaded once into free RAM at `0x800F6400`.
- `menu.c`: a menu module loaded into the gap above the menu overlays while menus need it.
- The game calls into the mod through redirected calls only (`jal` instructions), so every
  original file keeps its size and other patches like Zenith still apply.
- `tools/mod/build_mod.sh` and `build_mod.py` do the patching and disc layout.
- Her equipment and spell choice are saved in unused save flags (`0x400`-`0x43F`).
