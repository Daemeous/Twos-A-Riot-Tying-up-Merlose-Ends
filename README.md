# Two's a Riot: Tying Up Merlose Ends

![Two's a Riot: Tying Up Merlose Ends](docs/title.png)

*A Vagrant Story mod: Merlose joins Ashley as a companion.*

Callo Merlose, the VKP inquisitor from the opening, follows Ashley through Leá Monde as a fully
AI-controlled party member. She fights, heals and casts spells on her own, while you choose her
equipment and which of Ashley's spells she may use.

## Oh, and by the way...

**Vagrant Story now has native Linux and Windows ports.**

*Maybe I should've led with that...*

The Windows port is based on **noprompt**'s **[rood-boi](https://github.com/noprompt/rood-boi)**,
which builds on **ser-pounce**'s **[rood-reverse](https://github.com/ser-pounce/rood-reverse)**
decompilation and the work of its contributors. Without them, none of this would have been
possible. The decompilation has opened up possibilities for Vagrant Story modding that were
previously far beyond reach, and noprompt's Linux port was the foundation for the Windows one.

I developed the Windows port, which is now merged into rood-boi, and Two's A Riot has been
rebuilt to run natively on both Windows and Linux.

Both are experimental, with bugs and rough edges still to iron out. But the game runs, and we
can start exploring what this makes possible for the future of Vagrant Story.

Please give the original projects and their contributors the credit they deserve:

- **[rood-reverse](https://github.com/ser-pounce/rood-reverse)**: the decompilation
- **[rood-boi](https://github.com/noprompt/rood-boi)**: the native ports

## Features

- **She fights beside you.** Merlose appears next to Ashley in every room, attacks enemies, heals
  and casts spells. Enemies fight back against her. She can be defeated, and comes back in the
  next room.
- **She grows with Ashley.** She copies his base stats with STR and INT swapped, so a
  strength-built Ashley gets a caster Merlose, and the other way round.
- **MERLOSE equipment menu.** A new MERLOSE entry in the ITEMS menu, next to EQUIP, uses the
  normal equip screens with her stats and her model. Each item can only be worn by one of them,
  her gear grows like Ashley's, and it's all kept in your save. She starts with her own bow,
  *VKP Standard Issue*.
- **Choose her spells.** In the MAGIC menu, **Square** gives the highlighted spell to Merlose or
  takes it back (up to 8).
- **Friendly targeting.** Ashley's attacks and single-target attack spells can't hit her, but
  area spells can catch her; his healing, support spells and items work on her.
- **Quality of life.** Rooms clear without having to beat her, the STATUS screen shows her
  stats, small HP/MP/RISK bars for her sit under Ashley's, and she catches up if she's left
  behind.
- **Cutscenes.** She steps out for cutscenes and the prologue, and is back when they end.
- **Optional extras** (on by default, switch off in `Merlose options.ini`): permanent stat-up
  items always give their maximum (+4), and the bonus wheel after a boss can be chosen instead
  of spun (**Down/Up** to pick, **Circle** to take it).

## Compatibility

- **Game:** Vagrant Story, USA (SLUS-01040). You need your own copy; nothing from the game is in
  this repository or any download.
- **Native PC (rood-boi):** Windows 10/11 and 64-bit Linux. The QoL mod's weapon slots are built
  into rood-boi (switch them on in its editor); Zenith, the Randomizer and the rest of the QoL mod
  are patches for the PlayStation version and don't apply there.
- **PlayStation version:** [Zenith](https://vszenith.wordpress.com/) (1.94), the
  **[QoL mod](https://github.com/eremtsov/rood-reverse/releases)** (eremtsov, v3.1) and the
  **Vagrant Story Randomizer** (ChaoticBrave, 0.9.2.0): all supported. Apply this mod last:
  your disc → Zenith → QoL or randomizer → this mod. With QoL's default (Cross confirms)
  version, the choosable bonus wheel is left out, since QoL changes that screen too.
- **Saves:** existing saves work; her settings live in save data the game doesn't use.
- **Hardware (PlayStation version):** runs on a standard 2 MB PlayStation; developed in
  DuckStation.

## Getting it

Nothing from the game is downloaded: everything is built or patched from your own disc image of
Vagrant Story (USA), as a `.bin` + `.cue`.

| Play it on | What you get | Download |
| --- | --- | --- |
| **Windows** | rood-boi, the native PC version, with Merlose | **[Windows builder](https://github.com/Daemeous/rood-boi/releases/latest)** (`rood-boi-builder-*.zip`) |
| **Linux** | rood-boi, the native PC version, with Merlose | **[Linux builder](https://github.com/Daemeous/rood-boi/releases/latest)** (`rood-boi-linux-builder-*.tar.gz`) |
| **An emulator** (or a real PlayStation) | the PlayStation version, patched | **[Patcher](https://github.com/Daemeous/Twos-A-Riot-Tying-up-Merlose-Ends/releases/latest)** (`Twos-A-Riot-Merlose-*.zip`, Windows) |

### Native PC version (Windows or Linux)

The builder compiles rood-boi on your PC from the source code and your disc. Your disc image has
to be unmodified (no Zenith or other patches).

- **Windows:** extract the zip, put your `.bin` + `.cue` in the folder and double-click
  **Build rood-boi.bat**. The first time, it may install WSL (Windows' built-in Linux), which
  needs a restart. Then play with **play-callo.bat** in the new `rood-boi` folder.
- **Linux:** install Podman (or Docker), extract the archive, put your `.bin` + `.cue` in the
  folder and run `./build-rood-boi.sh`. Then play with `rood-boi/play-callo.sh`.

The first build takes 20-40 minutes. Merlose and her extras are switched on or off in
`rood-boi options.ini`. Run the builder again to update. The README in each download has the
details.

### PlayStation version (emulators)

**[Download the patcher](https://github.com/Daemeous/Twos-A-Riot-Tying-up-Merlose-Ends/releases/latest)**
(Windows 10/11, nothing to install):

1. Extract the zip and copy your Vagrant Story (USA) `.bin` + `.cue` into the extracted folder.
   To leave out an extra, set it to `no` in `Merlose options.ini`.
2. Double-click **Patch Vagrant Story.bat** and answer the questions. (If Windows says "Windows
   protected your PC", click **More info → Run anyway**.)
3. Open the new `... + Merlose.cue` in your emulator. Your original image isn't changed.

For Zenith, put its `.ppf` in the same folder and answer **Y** when asked. For the QoL mod,
patch your disc with QoL first (its own `.xdelta`, following its instructions), then run the
patcher on that disc. For the Randomizer, randomize first and patch the randomized disc. The
README in the zip covers `.chd` files and troubleshooting.

## How it's made

The mod is built on **[rood-reverse](https://github.com/ser-pounce/rood-reverse)**, the
decompilation of Vagrant Story, which turns the game's code back into readable C. Two editions
come from the same source:

- **Patch edition** (the one to use): a few small patches to the original game files plus two
  new files on the disc, so it can go on top of Zenith or the randomizer.
- **Decomp edition** (experimental, in development): Merlose compiled straight into the rebuilt
  game code. This needs the in-progress decompilation from
  [rood-reverse PR #186](https://github.com/ser-pounce/rood-reverse/pull/186), and doesn't mix
  with mods that patch the original files.

The native PC version comes from the same mod source, compiled into
**[rood-boi](https://github.com/Daemeous/rood-boi/tree/callo)** (`callo` branch;
`pc/run-game.sh --vulkan --callo` builds and plays it by hand on Linux).

Developed with [Claude Code](https://claude.com/claude-code).

## Credits

- **ser-pounce** and the rood-reverse contributors, for the decompilation this is built on.
- **noprompt**, for rood-boi, the native PC port, and for PR #186.
- **Zenith**: lead author **the_E_y_Es_o_0**, with collaborators **Valendian** and **Rosto**.
- **ChaoticBrave**, for the Vagrant Story Randomizer.
- **eremtsov**, for the QoL mod.


- Vagrant Story © Square Enix. This is a fan project, not affiliated with or endorsed by Square
  Enix.

Code in this repository is released under [CC0](LICENSE), like rood-reverse.

☕ Support My Work

If you've enjoyed this mod and would like to support me or my other projects, you can buy me a coffee on Ko-fi!

Absolutely no obligation, though, everything I make is because I'm passionate about it, and your enjoyment is already a reward in itself. Please take care of you and yours first!

I want to be clear that I stood on the shoulders of giants to make this. [ser-pounce's rood-reverse](https://github.com/ser-pounce/rood-reverse) and [noprompt's rood-boi](https://github.com/noprompt/rood-boi) are the real MVPs who made this possible. If you'd like to support the people whose work made this project possible, please consider supporting them, too!

[☕ Support me on Ko-fi](https://ko-fi.com/daemeous)
