"""Apply a PPF3 patch (e.g. Zenith) to a built disc image, then fix sector checksums.

The patch carries EDC/ECC for its own version of each sector it touches. Where our mod also
changed that sector (BATTLE.PRG's last sector holds both Zenith's code and our stub), those
would be stale, so EDC/ECC is recomputed for every MODE2 form 1 sector the patch touched.

usage: apply_ppf.py <patch.ppf> <image.bin>   (modifies the image in place)
"""

import struct
import sys

RAW = 2352

# --- CD-ROM EDC/ECC (ECMA-130) ---------------------------------------------------------
_ecc_f = [0] * 256
_ecc_b = [0] * 256
_edc = [0] * 256
for i in range(256):
    j = (i << 1) ^ (0x11D if i & 0x80 else 0)
    _ecc_f[i] = j & 0xFF
    _ecc_b[i ^ j & 0xFF] = i
    e = i
    for _ in range(8):
        e = (e >> 1) ^ (0xD8018001 if e & 1 else 0)
    _edc[i] = e


def _edc_of(data: bytes) -> int:
    e = 0
    for b in data:
        e = (e >> 8) ^ _edc[(e ^ b) & 0xFF]
    return e


def _ecc_block(src: bytearray, major_count, minor_count, major_mult, minor_inc, dest_off):
    size = major_count * minor_count
    for major in range(major_count):
        index = (major >> 1) * major_mult + (major & 1)
        a = b = 0
        for _ in range(minor_count):
            t = src[index]
            index += minor_inc
            if index >= size:
                index -= size
            a ^= t
            b ^= t
            a = _ecc_f[a]
        a = _ecc_b[_ecc_f[a] ^ b]
        src[dest_off + major] = a
        src[dest_off + major + major_count] = a ^ b


def fix_mode2_form1(sector: bytearray) -> None:
    """Recompute EDC and ECC of a raw 2352-byte MODE2 form 1 sector."""
    struct.pack_into("<I", sector, 2072, _edc_of(sector[16:2072]))
    # ECC covers bytes 12.. with the 4 header bytes treated as zero for mode 2
    header = sector[12:16]
    sector[12:16] = b"\0\0\0\0"
    body = sector[12:]
    _ecc_block(body, 86, 24, 2, 86, 2064)          # P parity -> sector 2076
    _ecc_block(body, 52, 43, 86, 88, 2064 + 172)   # Q parity -> sector 2248
    sector[12:] = body
    sector[12:16] = header


def records(path: str):
    d = open(path, "rb").read()
    if d[:5] != b"PPF30":
        sys.exit("only PPF3 patches are supported")
    blockcheck, undo = d[0x39], d[0x3A]
    pos, end = 0x3C + (1024 if blockcheck else 0), len(d)
    diz = d.rfind(b"@BEGIN_FILE_ID.DIZ")  # optional description block at the end
    if diz > 0:
        end = diz
    while pos < end:
        offset, length = struct.unpack_from("<QB", d, pos)
        pos += 9
        yield offset, d[pos:pos + length]
        pos += length * (2 if undo else 1)


# Files our build rebuilds from source: the patch's raw bytes for them would land on shifted
# code, so they are skipped and their changes are ported in source instead (LBA, sectors).
SKIP_FILES = {}  # e.g. "MENU/MENU3.PRG": (85049, 14) for (first sector, sector count)


def main() -> None:
    patch, image_path = sys.argv[1], sys.argv[2]
    skip = [range(lba, lba + n) for lba, n in SKIP_FILES.values()]
    touched = set()
    count = skipped = 0
    with open(image_path, "r+b") as image:
        for offset, data in records(patch):
            first, last = offset // RAW, (offset + len(data) - 1) // RAW
            if not any(first in r or last in r for r in skip):
                image.seek(offset)
                image.write(data)
                count += len(data)
                touched.update(range(first, last + 1))
                continue
            for i in range(len(data)):
                sector = (offset + i) // RAW
                if any(sector in r for r in skip):
                    skipped += 1
                    continue
                image.seek(offset + i)
                image.write(data[i:i + 1])
                count += 1
                touched.add(sector)
        if SKIP_FILES:
            print(f"skipped {skipped} bytes in rebuilt files {list(SKIP_FILES)}")
        fixed = 0
        for s in sorted(touched):
            image.seek(s * RAW)
            sector = bytearray(image.read(RAW))
            # MODE2 (header byte 15 == 2), form 1 (subheader submode bit 5 clear)
            if len(sector) == RAW and sector[15] == 2 and not sector[18] & 0x20:
                before = bytes(sector[2072:])
                fix_mode2_form1(sector)
                if bytes(sector[2072:]) != before:
                    image.seek(s * RAW + 2072)
                    image.write(sector[2072:])
                    fixed += 1
    print(f"applied {count} bytes over {len(touched)} sectors; recomputed EDC/ECC differed in {fixed}")


if __name__ == "__main__":
    main()
