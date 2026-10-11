"""Position-independent replacement for BATTLE.PRG.xdelta.

The original fixup sets the final padding byte of the statusStrings table to 0x80.
Locating it by its surrounding bytes keeps the fixup correct when code is added
to BATTLE.PRG and the table moves.
"""

import sys

# The 32 bytes preceding the patched byte (end of statusStrings).
CONTEXT = bytes.fromhex(
    "a1e7eb1b28362c36378f0e2435372ba1e71b28362c36378f2024372835a1e700"
)

def main(path: str) -> None:
    data = bytearray(open(path, "rb").read())
    pos = data.find(CONTEXT)
    if pos < 0 or data.find(CONTEXT, pos + 1) >= 0:
        sys.exit(f"{path}: statusStrings fixup context not found exactly once")
    target = pos + len(CONTEXT)
    if data[target] not in (0x00, 0x80):
        sys.exit(f"{path}: unexpected byte {data[target]:#x} at {target:#x}")
    data[target] = 0x80
    open(path, "wb").write(data)

if __name__ == "__main__":
    main(sys.argv[1])
