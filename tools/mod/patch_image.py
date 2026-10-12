"""Apply the Callo mod's patch kit (build/mod/kit, from tools/mod/build_mod.sh) to a finished
Vagrant Story (USA) disc image.

Run it last, after any other mods: Zenith (any PPF tool) and the randomizer replace whole files,
which would undo changes made before them. Every byte the kit changes is checked first: if
another mod changed one of them, nothing is written. Running it again on a patched image does
nothing.

Optional features (the kit's "features") are all applied unless --features lists the ones to
use (--features= for none). A feature whose bytes another mod already changed (e.g. the QoL
mod's button swap on the bonus wheel screen) is left out, with a note, rather than failing.

usage: patch_image.py <kit dir> <image.bin> [--features=a,b]   (modifies the image in place)
"""

import hashlib
import json
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from apply_ppf import fix_mode2_form1  # noqa: E402

RAW = 2352
DATA = 24          # MODE2 form 1 user data offset
SECTOR = 2048
SUBMODE_DATA = 0x08
SUBMODE_LAST = 0x89  # data + end of record + end of file
SUBMODE_FORM2 = 0x20


class Image:
    def __init__(self, path, mode="r+b"):
        self.f = open(path, mode)
        self.dirty = {}

    def sector(self, lba):
        if lba not in self.dirty:
            self.f.seek(lba * RAW)
            raw = bytearray(self.f.read(RAW))
            if len(raw) != RAW:
                sys.exit(f"sector {lba} is past the end of the image")
            self.dirty[lba] = raw
        return self.dirty[lba]

    def read(self, lba, size):
        return b"".join(bytes(self.sector(lba + i)[DATA:DATA + SECTOR])
                        for i in range(-(-size // SECTOR)))[:size]

    def write(self, lba, offset, data):
        for i, b in enumerate(data):
            s = self.sector(lba + (offset + i) // SECTOR)
            s[DATA + (offset + i) % SECTOR] = b

    def find(self, path):
        """(lba, size) of a file, from the ISO 9660 directories."""
        pvd = self.read(16, SECTOR)
        if pvd[1:6] != b"CD001":
            sys.exit("not a PlayStation disc image (no ISO 9660 volume)")
        lba, size = struct.unpack_from("<I", pvd, 156 + 2)[0], struct.unpack_from("<I", pvd, 156 + 10)[0]
        for part in path.upper().split("/"):
            data, pos, found = self.read(lba, size), 0, None
            while pos < len(data):
                length = data[pos]
                if length == 0:
                    pos = (pos // SECTOR + 1) * SECTOR
                    continue
                name = data[pos + 33:pos + 33 + data[pos + 32]].decode("latin-1").split(";")[0]
                if name == part:
                    found = struct.unpack_from("<I", data, pos + 2)[0], struct.unpack_from("<I", data, pos + 10)[0]
                    break
                pos += length
            if found is None:
                sys.exit(f"{path} not found on the disc")
            lba, size = found
        return lba, size

    def save(self, form1_sectors):
        for lba, raw in self.dirty.items():
            if lba in form1_sectors:
                fix_mode2_form1(raw)
            self.f.seek(lba * RAW)
            self.f.write(raw)
        self.f.close()


def check(image, files):
    """(changes, already, problems) for one group of per-file runs."""
    changes, already, problems = [], 0, []
    for path, runs in files.items():
        lba, size = image.find(path)
        room = -(-size // SECTOR) * SECTOR  # the file's last sector may hold appended code
        for run in runs:
            new = bytes.fromhex(run["new"])
            if run["offset"] + len(new) > room:
                problems.append(f"{path}: change at {run['offset']:#x} is past the file's sectors")
                continue
            current = image.read(lba, run["offset"] + len(new))[run["offset"]:]
            # Release kits carry a hash of the original bytes instead of the game's own bytes
            if "original" in run:
                matches = current == bytes.fromhex(run["original"])
            else:
                matches = hashlib.sha1(current).hexdigest() == run["original_sha1"]
            if current == new:
                already += 1
            elif matches:
                changes.append((lba, run["offset"], new, path))
            else:
                problems.append(f"{path} at {run['offset']:#x}: found {current[:16].hex()}..., "
                                f"not the original bytes (changed by another mod?)")
    return changes, already, problems


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--features=")]
    if len(args) != 2:
        sys.exit(__doc__)
    kit_dir, image_path = args
    kit = json.load(open(os.path.join(kit_dir, "patches.json")))
    features = kit.get("features", {})
    wanted = list(features)
    for a in sys.argv[1:]:
        if a.startswith("--features="):
            wanted = [f for f in a.split("=", 1)[1].split(",") if f]
    unknown = [f for f in wanted if f not in features]
    if unknown:
        sys.exit(f"unknown feature(s) {', '.join(unknown)}; the kit has {', '.join(features)}")
    image = Image(image_path)
    touched = set()

    # Check everything first
    changes, already, problems = check(image, kit["files"])
    notes = []
    for name in wanted:
        f_changes, f_already, f_problems = check(image, features[name])
        label = name.replace("_", " ")
        if f_problems:
            notes.append(f"Left out the {label}: another mod already changes that part of the "
                         f"game (e.g. the QoL mod's button swap).")
            continue
        notes.append(f"Including the {label}.")
        changes += f_changes
        already += f_already
    added = []
    for entry in kit["added"]:
        data = open(os.path.join(kit_dir, entry["file"]), "rb").read()
        sectors = -(-len(data) // SECTOR)
        data += bytes(sectors * SECTOR - len(data))
        present = True
        for i in range(sectors):
            raw = image.sector(entry["lba"] + i)
            current = bytes(raw[DATA:DATA + SECTOR])
            if current == data[i * SECTOR:(i + 1) * SECTOR] and raw[18] & SUBMODE_DATA:
                continue
            present = False
            if raw[18] != SUBMODE_FORM2 or any(current):
                problems.append(f"{entry['file']}: sector {entry['lba'] + i} isn't the game's empty "
                                f"padding (used by another mod, or an older version of this one?)")
        if present:
            already += 1
        else:
            added.append((entry, data, sectors))

    if problems:
        print("Not patched: the image doesn't match what the mod expects.")
        for p in problems:
            print("  " + p)
        sys.exit(1)
    for note in notes:
        print(note)

    for lba, offset, new, path in changes:
        image.write(lba, offset, new)
        touched.update(lba + (offset + i) // SECTOR for i in range(len(new)))
    for entry, data, sectors in added:
        for i in range(sectors):
            lba = entry["lba"] + i
            raw = image.sector(lba)
            raw[DATA:DATA + SECTOR] = data[i * SECTOR:(i + 1) * SECTOR]
            submode = SUBMODE_LAST if i == sectors - 1 else SUBMODE_DATA
            raw[16:24] = bytes([0, 0, submode, 0, 0, 0, submode, 0])
            touched.add(lba)
    image.save(touched)
    print(f"Patched: {len(changes)} changes to the game's files, {len(added)} files added "
          f"({already} were already there).")


if __name__ == "__main__":
    main()
