"""Helpers for tools/mod/build_mod.sh.

pack   <code.bin> <out> <magic> <base> <limit> <checked end> <entry points...>
battle  <BATTLE.PRG> <mod entry> <frame hook> <wrappers...>   room-spawn return -> mod entry;
        upkeep call -> hook; BATTLE_CALLS -> wrappers
initbtl <INITBTL.PRG> <loader.bin> <loader base> <loader entry> <disc load size>
xml    <disc xml> (<name> <lba> <size>)...   put files inside the padding before NULL.DAT
verify <disc image> (<lba> <file>)...        check they and NULL.DAT sit where expected
menu3  <MENU3.PRG> <wrapper addresses...>  MENU3 calls -> mod wrappers (see MENU3_CALLS)
menu0  <MENU0.PRG> <magicRows> <magicCost>  MAGIC menu calls -> mod wrappers (MENU0_CALLS)
"""

import os
import re
import struct
import sys

VRAM = 0x80068800
ROOM_SPAWN_START = 0x80089114  # func_80089114, spawns the room's characters
ROOM_SPAWN_END = 0x80089888
JR_RA = 0x03E00008
UPKEEP_CALL = 0x8007A460       # vs_battle_exec: jal func_80088B6C (per-frame upkeep)
UPKEEP = 0x80088B6C
# Other BATTLE.PRG calls redirected to the mod: (original target, wrapper, expected calls)
BATTLE_CALLS = [
    (0x8007CF80, "vs_mod_countTeam", 2),  # team head count: room scripts' "enemies left"
    (0x800CA2DC, "vs_mod_hud", 1),        # HP/MP/RISK display (vs_battle_exec)
]
INITBTL_VRAM = 0x800F9800
DISK_LOAD_FILE = 0x8004493C    # vs_main_diskLoadFile
SECTOR = 2048
RAW_SECTOR = 2352
RAW_DATA = 24                  # MODE2 form 1 user data offset
ENDING_PADDING = 15736         # dummy sectors between ENDING.XA and NULL.DAT
ENDING_XA_END = 303264         # first sector of that padding
NULL_DAT_LBA = 319000


HEADER_SIZE = 0x20


def pack(code_path, out_path, magic, base, limit, checked_end, *functions):
    """Header (0x20 bytes): magic, checked size, a function table of up to 5 entry points (rest
    zero), then a checksum of the words from the end of the header up to `checked_end` (code
    and constants: the variables after them change at run time). The menu module is checked
    with it. The whole file, padded to sectors, is what gets loaded, so it must fit below
    `limit`."""
    code = open(code_path, "rb").read()
    code += bytes(-len(code) % 4)
    if len(functions) > 5:
        sys.exit("pack: at most 5 entry points")
    checked = (checked_end - base - HEADER_SIZE) & ~3
    if not 0 < checked <= len(code):
        sys.exit(f"pack: checked end {checked_end:#x} outside the code")
    entries = list(functions) + [0] * (5 - len(functions))
    checksum = sum(struct.unpack(f"<{checked // 4}I", code[:checked])) & 0xFFFFFFFF
    header = struct.pack("<8I", magic, HEADER_SIZE + checked, *entries, checksum)
    data = header + code
    data += bytes(-len(data) % SECTOR)
    name = os.path.basename(out_path)
    if base + len(data) > limit:
        sys.exit(f"{name} too big: {len(data)} bytes on disc, room for {limit - base}")
    open(out_path, "wb").write(data)
    print(f"{name}: {HEADER_SIZE + len(code)} bytes used, {len(data)} on disc, "
          f"{limit - base - len(data)} bytes of RAM to spare")


def battle(prg_path, entry, frame, *wrappers):
    """Redirect func_80089114's only `jr $ra` to a tail call of the mod's entry point, and
    vs_battle_exec's per-frame upkeep call to the mod's frame hook (which calls the upkeep)."""
    prg = bytearray(open(prg_path, "rb").read())
    returns = [a for a in range(ROOM_SPAWN_START, ROOM_SPAWN_END, 4)
               if struct.unpack_from("<I", prg, a - VRAM)[0] == JR_RA]
    if len(returns) != 1:
        sys.exit(f"expected one `jr $ra` in func_80089114, found {[hex(a) for a in returns]}")
    struct.pack_into("<I", prg, returns[0] - VRAM, 0x08000000 | ((entry >> 2) & 0x03FFFFFF))
    found = struct.unpack_from("<I", prg, UPKEEP_CALL - VRAM)[0]
    if found != 0x0C000000 | ((UPKEEP >> 2) & 0x03FFFFFF):
        sys.exit(f"BATTLE.PRG {UPKEEP_CALL:#x}: expected jal {UPKEEP:#x}, found {found:#010x}")
    struct.pack_into("<I", prg, UPKEEP_CALL - VRAM, 0x0C000000 | ((frame >> 2) & 0x03FFFFFF))
    if len(wrappers) != len(BATTLE_CALLS):
        sys.exit(f"battle: expected {len(BATTLE_CALLS)} wrapper addresses")
    for (original, name, count), wrapper in zip(BATTLE_CALLS, wrappers):
        jal = 0x0C000000 | ((original >> 2) & 0x03FFFFFF)
        sites = [i for i in range(0, len(prg), 4) if struct.unpack_from("<I", prg, i)[0] == jal]
        if len(sites) != count:
            sys.exit(f"BATTLE.PRG: expected {count} jal {original:#x} for {name}, found {len(sites)}")
        for i in sites:
            struct.pack_into("<I", prg, i, 0x0C000000 | ((wrapper >> 2) & 0x03FFFFFF))
            print(f"BATTLE.PRG: {VRAM + i:#x} jal {original:#x} -> {name} {wrapper:#x}")
    open(prg_path, "wb").write(prg)
    print(f"BATTLE.PRG: {returns[0]:#x} -> j {entry:#x}; {UPKEEP_CALL:#x} -> jal {frame:#x}")


def initbtl(path, loader_path, base, entry, load_size):
    """Append the loader to INITBTL.PRG and redirect its SYSTEM.DAT load call to it."""
    prg = bytearray(open(path, "rb").read())
    loader = open(loader_path, "rb").read()
    end = INITBTL_VRAM + len(prg)
    if not end <= base < end + 16:
        sys.exit(f"loader linked at {base:#x} but INITBTL.PRG ends at {end:#x}")
    padding = base - end
    if len(prg) + padding + len(loader) > load_size:
        sys.exit(f"loader doesn't fit in INITBTL.PRG's {load_size} loaded bytes")
    jal_load = 0x0C000000 | ((DISK_LOAD_FILE >> 2) & 0x03FFFFFF)
    calls = [i for i in range(0, len(prg), 4) if struct.unpack_from("<I", prg, i)[0] == jal_load]
    if len(calls) != 1:
        sys.exit(f"expected one vs_main_diskLoadFile call in INITBTL.PRG, found {len(calls)}")
    struct.pack_into("<I", prg, calls[0], 0x0C000000 | ((entry >> 2) & 0x03FFFFFF))
    prg += bytes(padding) + loader
    open(path, "wb").write(prg)
    print(f"INITBTL.PRG: SYSTEM.DAT load at {INITBTL_VRAM + calls[0]:#x} -> jal {entry:#x}; loader "
          f"{len(loader)} bytes at {base:#x}; {INITBTL_VRAM + load_size - base - len(loader)} bytes left")


def xml(path, *files):
    """Put files (name, lba, size, ...) inside the ENDING padding before NULL.DAT, in order."""
    text = open(path, encoding="utf-8").read()
    dummy = f'<dummy sectors="{ENDING_PADDING}" type="32"/>'
    if text.count(dummy) != 1:
        sys.exit("ENDING padding entry not found exactly once")
    indent = re.search(r"\n(\s*)" + re.escape(dummy), text).group(1)
    parts = []
    position = ENDING_XA_END
    for i in range(0, len(files), 3):
        name, lba, size = files[i], int(files[i + 1], 0), int(files[i + 2], 0)
        if lba < position:
            sys.exit(f"{name} at LBA {lba} overlaps what comes before it (ends at {position})")
        sectors = -(-size // SECTOR)
        parts.append(f'<dummy sectors="{lba - position}" type="32"/>')
        parts.append(f'<file name="{name}" type="data"/>')
        position = lba + sectors
        print(f"disc layout: {name} at LBA {lba}, {sectors} sectors")
    after = ENDING_XA_END + ENDING_PADDING - position
    if after < 0:
        sys.exit("the files don't fit in the ENDING padding")
    parts.append(f'<dummy sectors="{after}" type="32"/>')
    open(path, "w", encoding="utf-8").write(text.replace(dummy, ("\n" + indent).join(parts)))


MENU3_VRAM = 0x80102800
# MENU3 calls redirected to the mod: (original target, wrapper, call address or None for every
# call to that target, expected number of calls)
MENU3_CALLS = [
    (0x801053EC, "vs_mod_itemNavigation", 0x80108A64, 1),  # vs_menu3_exec: row = _itemNavigation(0)
    (0x80108088, "vs_mod_equipMenu", 0x80108B88, 1),       # vs_menu3_exec: row = _equipMenu(0)
    (0x801005E0, "vs_mod_setMenuRows", 0x801056C4, 1),     # ITEMS > OPTIONS rows
    (0x801090C4, "vs_mod_loadObjectData", None, 2),        # weapon/shield screens' model loads
]


def jal(target):
    return 0x0C000000 | ((target >> 2) & 0x03FFFFFF)


# MENU0.PRG (MAGIC menu): each school list's rows and its per-frame MP cost display
MENU0_CALLS = [
    (0x801005E0, "vs_mod_magicRows", 0x80102D20, 1),  # warlock list
    (0x801005E0, "vs_mod_magicRows", 0x80103670, 1),  # shaman list
    (0x801005E0, "vs_mod_magicRows", 0x801039B4, 1),  # sorcerer list
    (0x801005E0, "vs_mod_magicRows", 0x80103CF8, 1),  # enchanter list
    (0x80102884, "vs_mod_magicCost", 0x80102F0C, 1),  # warlock highlighted spell
    (0x80102884, "vs_mod_magicCost", 0x80103748, 1),  # shaman
    (0x80102884, "vs_mod_magicCost", 0x80103A8C, 1),  # sorcerer
    (0x80102884, "vs_mod_magicCost", 0x80103DD0, 1),  # enchanter
]


def menu3(path, *wrappers):
    _redirect_calls(path, "MENU3.PRG", MENU3_CALLS, wrappers)


def menu0(path, magic_rows, magic_cost):
    names = {"vs_mod_magicRows": magic_rows, "vs_mod_magicCost": magic_cost}
    _redirect_calls(path, "MENU0.PRG", MENU0_CALLS, [names[c[1]] for c in MENU0_CALLS])


def _redirect_calls(path, label, calls, wrappers):
    """Redirect calls in a menu overlay to the mod's wrappers. Only jal instructions change, so
    the file keeps its size and other patches to it (Zenith's) still apply."""
    prg = bytearray(open(path, "rb").read())
    if len(wrappers) != len(calls):
        sys.exit(f"{label}: expected {len(calls)} wrapper addresses")
    for (original, name, address, count), wrapper in zip(calls, wrappers):
        if address is None:
            sites = [MENU3_VRAM + i for i in range(0, len(prg), 4)
                     if struct.unpack_from("<I", prg, i)[0] == jal(original)]
        else:
            sites = [address] if struct.unpack_from("<I", prg, address - MENU3_VRAM)[0] == jal(original) else []
        if len(sites) != count:
            sys.exit(f"{label}: expected {count} jal {original:#x} for {name}, found {len(sites)}")
        for site in sites:
            struct.pack_into("<I", prg, site - MENU3_VRAM, jal(wrapper))
            print(f"{label}: {site:#x} jal {original:#x} -> {name} {wrapper:#x}")
    open(path, "wb").write(prg)


def read_sector(image, lba):
    image.seek(lba * RAW_SECTOR + RAW_DATA)
    return image.read(SECTOR)


def sectors(build_dir, original_dir):
    """Every rebuilt file must keep its original sector count, or every later file on the disc
    would move (zones store raw disc positions; PPF mods like Zenith patch fixed offsets)."""
    import os
    bad = []
    for root, _, files in os.walk(original_dir):
        for f in files:
            orig = os.path.join(root, f)
            built = os.path.join(build_dir, os.path.relpath(orig, original_dir))
            if os.path.exists(built):
                a, b = -(-os.path.getsize(orig) // SECTOR), -(-os.path.getsize(built) // SECTOR)
                if a != b:
                    bad.append(f"{os.path.relpath(orig, original_dir)}: {a} -> {b} sectors")
    if bad:
        sys.exit("files changed sector count (would shift the disc layout):\n  " + "\n  ".join(bad))
    print("sector counts: all files unchanged")


def verify(image_path, *files):
    """Check files (lba, path, ...) are where expected and NULL.DAT didn't move."""
    with open(image_path, "rb") as image:
        for i in range(0, len(files), 2):
            lba, file_path = int(files[i], 0), files[i + 1]
            data = open(file_path, "rb").read()
            on_disc = b"".join(read_sector(image, lba + j) for j in range(-(-len(data) // SECTOR)))
            if on_disc[:len(data)] != data:
                sys.exit(f"{file_path} is not at LBA {lba} in the image")
            print(f"verified: {os.path.basename(file_path)} at LBA {lba}")
        null_dat = read_sector(image, NULL_DAT_LBA)[:1]
    print(f"verified: NULL.DAT still at {NULL_DAT_LBA} (byte {null_dat.hex()})")


def main():
    cmd, *args = sys.argv[1:]
    num = lambda s: int(s, 0)
    if cmd == "pack":
        pack(args[0], args[1], *map(num, args[2:]))
    elif cmd == "battle":
        battle(args[0], *map(num, args[1:]))
    elif cmd == "initbtl":
        initbtl(args[0], args[1], *map(num, args[2:5]))
    elif cmd == "xml":
        xml(args[0], *args[1:])
    elif cmd == "sectors":
        sectors(args[0], args[1])
    elif cmd == "menu0":
        menu0(args[0], *map(num, args[1:3]))
    elif cmd == "menu3":
        menu3(args[0], *map(num, args[1:]))
    elif cmd == "verify":
        verify(args[0], *args[1:])
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
