# Two's a Riot: Tying Up Merlose Ends

![Two's a Riot: Tying Up Merlose Ends](docs/title.png)

*A Vagrant Story mod: Merlose joins Ashley as a companion.*

Callo Merlose, the VKP inquisitor from the opening, follows Ashley through Leá Monde as a fully
AI-controlled party member. She fights, heals and casts spells on her own, while you choose her
equipment and which of Ashley's spells she may use.

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
- **Extras.** Permanent stat-up items always give their maximum (+4), and the bonus wheel after
  a boss can be chosen instead of spun (**Down/Up** to pick, **Circle** to take it).

## Compatibility

- **Game:** Vagrant Story, USA (SLUS-01040). You need your own copy; nothing from the game is in
  this repository.
- **[Zenith](https://vszenith.wordpress.com/)** (1.94) and the **Vagrant Story Randomizer**
  (ChaoticBrave, 0.9.2.0): both supported. Apply the mod last: your disc → Zenith → randomizer
  → this mod.
- **Saves:** existing saves work; her settings live in save data the game doesn't use.
- **Hardware:** runs on a standard 2 MB PlayStation; developed in DuckStation.

## Getting it

**[Download the latest release](https://github.com/Daemeous/Twos-A-Riot-Tying-up-Merlose-Ends/releases/latest)**
(Windows 10/11, nothing to install):

1. Extract the zip and copy your Vagrant Story (USA) `.bin` + `.cue` into the extracted folder.
2. Double-click **Patch Vagrant Story.bat** and answer the questions. (If Windows says "Windows
   protected your PC", click **More info → Run anyway**.)
3. Open the new `... + Merlose.cue` in your emulator. Your original image isn't changed.

For Zenith, put its `.ppf` in the same folder and answer **Y** when asked. For the Randomizer,
randomize first and patch the randomized disc. The README in the zip covers `.chd` files and
troubleshooting.

To build it from source instead, see
**[CALLO_MOD.md](https://github.com/Daemeous/Twos-A-Riot-Tying-up-Merlose-Ends/blob/callo-preview/CALLO_MOD.md)**
on the `callo-preview` branch (Linux, or WSL on Windows).

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

Developed with [Claude Code](https://claude.com/claude-code).

## Credits

- **ser-pounce** and the rood-reverse contributors, for the decompilation this is built on, and
  **noprompt** for PR #186.
- **Zenith**: lead author **the_E_y_Es_o_0**, with collaborators **Valendian** and **Rosto**.
- **ChaoticBrave**, for the Vagrant Story Randomizer.
- Vagrant Story © Square Enix. This is a fan project, not affiliated with or endorsed by Square
  Enix.

Code in this repository is released under [CC0](LICENSE), like rood-reverse.
