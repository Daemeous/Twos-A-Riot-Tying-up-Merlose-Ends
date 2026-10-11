# Two's a Riot: Tying Up Merlose Ends

![Two's a Riot: Tying Up Merlose Ends](docs/title.png)

*A Vagrant Story mod: Callo Merlose joins Ashley as a companion.*

Callo follows Ashley through Leá Monde as a fully AI-controlled party member. She fights, heals
and casts spells on her own, while you choose her equipment and which of Ashley's spells she may
use.

## Features

- **She fights beside you.** Callo appears next to Ashley in every room, attacks enemies, heals
  and casts spells. Enemies fight back against her. She can be defeated, and comes back in the
  next room.
- **She grows with Ashley.** She copies his base stats with STR and INT swapped, so a
  strength-built Ashley gets a caster Callo, and the other way round.
- **MERLOSE equipment menu.** A new entry under ITEMS → OPTIONS uses the normal equip screens
  with her stats and her model. Each item can only be worn by one of them, her gear grows like
  Ashley's, and it's all kept in your save. She starts with her own bow, *VKP Standard Issue*.
- **Choose her spells.** In the MAGIC menu, **Square** gives the highlighted spell to Callo or
  takes it back (up to 8).
- **Friendly targeting.** Ashley's attacks and attack spells can't hit her; his healing,
  support spells and items can.
- **Quality of life.** Rooms clear without having to beat her, the STATUS screen shows her
  stats, small HP/MP/RISK bars for her sit under Ashley's, and she catches up if she's left
  behind.
- **Extras.** Permanent stat-up items always give their maximum (+4).

**In testing** (next release): Callo sits out cutscenes and the prologue, and the bonus wheel
after a boss can be chosen instead of spun (**Down/Up** to pick, **Circle** to take it).

## Compatibility

- **Game:** Vagrant Story, USA (SLUS-01040). You need your own copy; nothing from the game is in
  this repository.
- **[Zenith](https://vszenith.wordpress.com/)** (1.94) and the **Vagrant Story Randomizer**
  (ChaoticBrave, 0.9.2.0): both supported. Apply the mod last: your disc → Zenith → randomizer
  → this mod.
- **Saves:** existing saves work; Callo's settings live in save data the game doesn't use.
- **Hardware:** runs on a standard 2 MB PlayStation; developed in DuckStation.

## Getting it

The mod is built from source against your own disc image. The step-by-step guide is in
**[CALLO_MOD.md](https://github.com/Daemeous/Twos-A-Riot-Tying-up-Merlose-Ends/blob/callo-preview/CALLO_MOD.md)**
on the `callo-preview` branch (Linux, or WSL on Windows).

## How it's made

The mod is built on **[rood-reverse](https://github.com/ser-pounce/rood-reverse)**, the
decompilation of Vagrant Story, which turns the game's code back into readable C. Two editions
come from the same source:

- **Patch edition** (the one to use): a few small patches to the original game files plus two
  new files on the disc, so it can go on top of Zenith or the randomizer.
- **Decomp edition** (experimental, in development): Callo compiled straight into the rebuilt
  game code. This needs the in-progress decompilation from
  [rood-reverse PR #186](https://github.com/ser-pounce/rood-reverse/pull/186), and doesn't mix
  with mods that patch the original files.

Developed with [Claude Code](https://claude.com/claude-code).

## Credits

- **ser-pounce** and the rood-reverse contributors, for the decompilation this is built on, and
  **noprompt** for PR #186.
- The **Zenith** author, and **ChaoticBrave** for the Vagrant Story Randomizer.
- Vagrant Story © Square Enix. This is a fan project, not affiliated with or endorsed by Square
  Enix.

Code in this repository is released under [CC0](LICENSE), like rood-reverse.
