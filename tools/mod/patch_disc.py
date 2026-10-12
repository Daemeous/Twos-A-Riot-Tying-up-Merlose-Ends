"""Guided patcher for players: finds a Vagrant Story (USA) disc image in a folder, copies it,
optionally adds a Zenith .ppf found in the same folder, then applies the Merlose patch kit.

The release zip (tools/mod/make_release.py) runs this from "Patch Vagrant Story.bat" with the
embedded Python. The original image is never modified.

usage: patch_disc.py <folder> [image]   (image: a .cue/.bin dropped onto the .bat)
"""

import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from patch_image import Image  # noqa: E402

KIT = os.path.join(HERE, "kit")
SUFFIX = " + Merlose"
RAW = 2352
RAW_EXTS = (".bin", ".img", ".iso")
CHDMAN_URL = "https://www.mamedev.org/release.html"


def say(text=""):
    print(text, flush=True)


def fail(text):
    say()
    say("PROBLEM: " + text)
    sys.exit(1)


def ask(question, default=None):
    while True:
        answer = input(question + " ").strip().lower()
        if not answer and default:
            return default
        if answer[:1] in ("y", "n"):
            return answer[:1]
        say("  Please type Y or N, then press Enter.")


def cue_tracks(cue):
    """The files a .cue refers to, as full paths."""
    files = []
    for line in open(cue, encoding="latin-1"):
        line = line.strip()
        if line.upper().startswith("FILE "):
            rest = line[5:].strip()
            name = rest[1:rest.index('"', 1)] if rest.startswith('"') else rest.split()[0]
            files.append(os.path.join(os.path.dirname(cue), name))
    return files


def find_images(folder):
    """(choices, notes): choices are (label, bin path, cue path or None)."""
    names = sorted(os.listdir(folder), key=str.lower)
    choices, notes, used = [], [], set()
    for name in names:
        path = os.path.join(folder, name)
        stem, ext = os.path.splitext(name)
        if ext.lower() != ".cue" or stem.endswith(SUFFIX):
            continue
        tracks = cue_tracks(path)
        used.update(os.path.normcase(t) for t in tracks)
        if len(tracks) != 1:
            notes.append(f"{name}: has {len(tracks)} tracks; Vagrant Story (USA) has 1, skipped")
        elif not os.path.isfile(tracks[0]):
            notes.append(f"{name}: its .bin ({os.path.basename(tracks[0])}) isn't in the folder")
        else:
            choices.append((name, tracks[0], path))
    for name in names:
        path = os.path.join(folder, name)
        stem, ext = os.path.splitext(name)
        ext = ext.lower()
        if stem.endswith(SUFFIX) or os.path.normcase(path) in used or not os.path.isfile(path):
            continue
        if ext in RAW_EXTS:
            if os.path.getsize(path) % RAW == 0:
                choices.append((name, path, None))
            else:
                notes.append(f"{name}: a 2048-byte ISO, which leaves out the game's movies and "
                             f"voices. Make a .bin/.cue copy of your disc instead.")
        elif ext == ".chd":
            choices.append((name, path, "chd"))
        elif ext in (".zip", ".7z", ".rar"):
            notes.append(f"{name}: extract (unzip) it into this folder first")
        elif ext in (".ecm", ".pbp"):
            notes.append(f"{name}: {ext} images aren't supported; use a .bin/.cue")
    return choices, notes


def pick(choices):
    if len(choices) == 1:
        return choices[0]
    say("Found these disc images:")
    for i, (label, _, _) in enumerate(choices, 1):
        say(f"  {i}. {label}")
    while True:
        answer = input(f"Which one should be patched? Type 1-{len(choices)} and press Enter: ").strip()
        if answer.isdigit() and 1 <= int(answer) <= len(choices):
            return choices[int(answer) - 1]


def chd_to_bin(chd, folder):
    """Convert with chdman.exe (from MAME) if the player put it beside the .bat."""
    chdman = next((p for p in (os.path.join(folder, "chdman.exe"), os.path.join(HERE, "chdman.exe"))
                   if os.path.isfile(p)), None)
    if chdman is None:
        fail(f"{os.path.basename(chd)} is a CHD file. To use it, get chdman.exe from MAME "
             f"({CHDMAN_URL}, the Windows download; chdman.exe is inside) and put it in this "
             f"folder, then run this again.")
    stem = os.path.splitext(chd)[0]
    cue, bin_ = stem + ".cue", stem + ".bin"
    if os.path.exists(cue) or os.path.exists(bin_):
        fail(f"{os.path.basename(cue)} or .bin already exists; remove it or patch that instead.")
    say(f"Converting {os.path.basename(chd)} to .bin/.cue with chdman...")
    if subprocess.call([chdman, "extractcd", "-i", chd, "-o", cue, "-ob", bin_]) != 0:
        fail("chdman couldn't convert the CHD file.")
    return bin_, cue


def copy_with_progress(src, dst):
    total, done, step = os.path.getsize(src), 0, 0
    with open(src, "rb") as fi, open(dst, "wb") as fo:
        while True:
            chunk = fi.read(8 << 20)
            if not chunk:
                break
            fo.write(chunk)
            done += len(chunk)
            if done * 10 // total > step:
                step = done * 10 // total
                print(f"  {step * 10}%", end="", flush=True)
    say()


def ppf_present(ppf, image_path, samples=200):
    """True when the image already holds the patch's bytes (e.g. it already has Zenith)."""
    from apply_ppf import records
    checked = 0
    with open(image_path, "rb") as image:
        for offset, data in records(ppf):
            image.seek(offset)
            if image.read(len(data)) != data:
                return False
            checked += 1
            if checked >= samples:
                break
    return checked > 0


def run(script, *args):
    return subprocess.call([sys.executable, os.path.join(HERE, script), *args])


def main():
    try:
        sys.stdout.reconfigure(errors="replace")
    except AttributeError:
        pass
    folder = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else ".")
    dropped = sys.argv[2] if len(sys.argv) > 2 and sys.argv[2] else None
    say("Two's A Riot: Tying up Merlose Ends - disc patcher")
    say("=" * 50)
    say()

    if dropped:
        ext = os.path.splitext(dropped)[1].lower()
        if ext == ".cue":
            tracks = cue_tracks(dropped)
            if len(tracks) != 1 or not os.path.isfile(tracks[0]):
                fail(f"{os.path.basename(dropped)} doesn't point to one .bin file next to it.")
            choice = (os.path.basename(dropped), tracks[0], dropped)
        elif ext == ".chd":
            choice = (os.path.basename(dropped), dropped, "chd")
        else:
            choice = (os.path.basename(dropped), dropped, None)
    else:
        choices, notes = find_images(folder)
        for note in notes:
            say("Note: " + note)
        if notes:
            say()
        if not choices:
            if "\\temp\\" in folder.lower() or "\\appdata\\" in folder.lower():
                fail("It looks like this is running from inside the zip file. Extract (unzip) the "
                     "whole folder first: right-click the zip, Extract All. Then put your disc "
                     "image in that folder and run the .bat from there.")
            fail("No disc image found. Put your Vagrant Story (USA) .bin and .cue files in this "
                 f"folder:\n  {folder}\nthen run this again.")
        choice = pick(choices)

    label, src_bin, src_cue = choice
    if src_cue == "chd":
        src_bin, src_cue = chd_to_bin(src_bin, os.path.dirname(os.path.abspath(src_bin)))
    say(f"Disc image: {label}")

    # Is it Vagrant Story (USA)? (patch_image checks every byte later; this gives a clearer message)
    if os.path.getsize(src_bin) % RAW:
        fail(f"{os.path.basename(src_bin)} isn't a raw (2352-byte sector) disc image.")
    image = Image(src_bin, "rb")
    try:
        image.find("SLUS_010.40")
    except SystemExit:
        fail(f"{label} isn't Vagrant Story (USA) (SLUS-01040). Other versions aren't supported.")
    finally:
        image.f.close()

    folder_out = os.path.dirname(os.path.abspath(src_bin))
    stem = os.path.splitext(os.path.basename(src_cue or src_bin))[0]
    out_bin = os.path.join(folder_out, stem + SUFFIX + ".bin")
    out_cue = os.path.join(folder_out, stem + SUFFIX + ".cue")

    # Zenith (or another PPF) from the same folder, applied before the mod
    ppfs = sorted(p for p in os.listdir(folder) if p.lower().endswith(".ppf"))
    ppf = None
    if len(ppfs) > 1:
        say(f"Note: more than one .ppf in the folder ({', '.join(ppfs)}); none will be applied. "
            f"Keep only the one you want.")
    elif ppfs:
        candidate = os.path.join(folder, ppfs[0])
        if open(candidate, "rb").read(5) != b"PPF30":
            say(f"Note: {ppfs[0]} isn't a PPF3 patch; skipped.")
        elif ppf_present(candidate, src_bin):
            say(f"{ppfs[0]} is already applied to this disc; it won't be added again.")
        else:
            say()
            say(f"Found {ppfs[0]} (Zenith?) in the folder.")
            say("Add it to the new disc as well? Say N if this disc came from the Randomizer.")
            if ask("Add it? [Y/N]") == "y":
                ppf = candidate

    if os.path.exists(out_bin):
        say()
        if ask(f"{os.path.basename(out_bin)} already exists. Replace it? [Y/N]") != "y":
            fail("Stopped; nothing was changed.")

    free = shutil.disk_usage(folder_out).free
    if free < os.path.getsize(src_bin) + (64 << 20):
        fail(f"Not enough free disk space for the patched copy "
             f"(needs {os.path.getsize(src_bin) >> 20} MB).")

    say()
    say(f"Copying to {os.path.basename(out_bin)} (your original stays untouched)...")
    copy_with_progress(src_bin, out_bin)
    ok = True
    if ppf:
        say(f"Applying {os.path.basename(ppf)}...")
        ok = run("apply_ppf.py", ppf, out_bin) == 0
    if ok:
        say("Adding Merlose...")
        ok = run("patch_image.py", KIT, out_bin) == 0
    if not ok:
        os.remove(out_bin)
        fail("Patching failed (see the messages above), so the copy was deleted. Is this the "
             "USA version of the game, and was any other mod applied after the Randomizer?")

    with open(out_cue, "w", newline="\r\n") as f:
        f.write(f'FILE "{os.path.basename(out_bin)}" BINARY\n'
                f"  TRACK 01 MODE2/2352\n"
                f"    INDEX 01 00:00:00\n")
    say()
    say("Done! Open this file in your emulator:")
    say(f"  {out_cue}")


if __name__ == "__main__":
    main()
