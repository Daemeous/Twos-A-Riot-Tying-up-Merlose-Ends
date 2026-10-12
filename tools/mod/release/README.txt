Two's A Riot: Tying up Merlose Ends  -  version @VERSION@
=========================================================

A Vagrant Story mod: Callo Merlose joins Ashley as an AI companion.
Project page: https://github.com/Daemeous/Twos-A-Riot-Tying-up-Merlose-Ends


WHAT YOU NEED
-------------
* Your own copy of Vagrant Story (USA), SLUS-01040, as a disc image.
  The game is NOT included. Best is a .bin + .cue pair; a single .bin,
  .img or .chd (see below) also works. Other versions (Europe, Japan)
  are not supported.
* Windows 10 or 11. Nothing else to install.


HOW TO PATCH
------------
1. Extract this zip: right-click it, choose "Extract All...".
   (Don't run anything from inside the zip window.)
2. Copy your Vagrant Story .bin and .cue into the extracted folder,
   next to "Patch Vagrant Story.bat".
   (Optional: open "Merlose options.ini" in Notepad to switch extras
   off, see OPTIONS below.)
3. Double-click "Patch Vagrant Story.bat".
   If Windows says "Windows protected your PC", click "More info",
   then "Run anyway". (It's a plain text file; you can open it in
   Notepad to see what it does.)
4. Answer the questions in the black window. It makes a NEW copy named
   "... + Merlose.bin" / "... + Merlose.cue". Your original disc image
   is never changed. You need about 750 MB of free space.
5. Open the new "+ Merlose.cue" file in your emulator (DuckStation,
   ePSXe, ...).

Tip: you can also drag your .cue (or .bin) onto the .bat file.


OPTIONS
-------
"Merlose options.ini" lets you leave out the extras. Change "yes" to
"no", save, and run the .bat again on your ORIGINAL disc image:
* max_stat_items: permanent stat-up items always give +4.
* choosable_bonus_wheel: pick the bonus wheel reward after a boss
  (Down/Up, then Circle) instead of spinning for it.
Merlose herself is always included.


WANT ZENITH TOO?
----------------
Download "Vagrant Story: Zenith" (https://vszenith.wordpress.com/) and
put its .ppf file in this folder before running the .bat. The patcher
will ask whether to add it. If your disc already has Zenith, just patch
it as it is; the patcher notices and won't add it twice.


USING THE QoL MOD?
------------------
The QoL mod (weapon slots, Cross to confirm, camera controls) is a
separate mod: https://github.com/eremtsov/rood-reverse/releases
1. Patch your disc with the QoL mod first, following its own
   instructions (it uses an .xdelta file and a tool such as xdelta UI
   or Delta Patcher). For Zenith + QoL, use QoL's "for Zenith 1.94"
   patch on a Zenith disc.
2. Put that QoL disc here and run "Patch Vagrant Story.bat" on it.
   If it offers to add a .ppf, say N.
With QoL's default (Cross confirms) version, the choosable bonus wheel
is left out automatically, because QoL changes that screen too. With
QoL's "classic buttons" version, everything is included.


USING THE RANDOMIZER?
---------------------
Randomize first, then run this patcher on the randomized disc.
Merlose always has to go on last. Say N if the patcher offers to add a
.ppf to a randomized disc (pick Zenith in the Randomizer instead).


CHD FILES
---------
The patcher can convert a .chd for you if chdman.exe is in this folder.
chdman comes with MAME: https://www.mamedev.org/release.html (get the
Windows download, open it with 7-Zip, copy chdman.exe out).


IF SOMETHING GOES WRONG
-----------------------
* "No disc image found": the .bin/.cue must be in the same folder as
  the .bat (not in a subfolder, not still zipped).
* "doesn't match what the mod expects": the disc isn't the USA version,
  or another mod was applied after this one or changed the same parts
  of the game. Other mods always go on first (Zenith, then QoL or the
  Randomizer), Merlose last. Start again from a clean disc.
* Use in-game saves (memory card), not emulator save states made
  without the mod.
* Problems or ideas: open an issue on the project page.


WHAT'S IN THIS FOLDER
---------------------
Patch Vagrant Story.bat   starts the patcher
Merlose options.ini       which extras to include
files\                    the patcher (Python scripts) and the mod itself
files\python\             an official Python (python.org), so you don't
                          need to install one
