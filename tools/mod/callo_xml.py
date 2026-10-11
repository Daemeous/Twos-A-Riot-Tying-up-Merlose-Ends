"""Disc layout for the Callo decomp edition: BATTLE.PRG starts `sectors` earlier, in the padding
before it (so it can grow without moving anything after it), and the menu module goes in the
ENDING padding (build_mod.xml).

    callo_xml.py <disc xml> <sectors> <name> <lba> <size> ...
"""
import re
import sys

import build_mod


def main(path, sectors, *files):
    sectors = int(sectors)
    text = open(path, encoding="utf-8").read()
    battle = re.search(r'(<dir name="BATTLE"[^>]*>\s*<dummy sectors=")(\d+)(")', text)
    if battle is None:
        sys.exit("BATTLE directory padding not found")
    padding = int(battle.group(2))
    if padding < sectors:
        sys.exit(f"only {padding} sectors of padding before BATTLE.PRG")
    text = text[: battle.start(2)] + str(padding - sectors) + text[battle.end(2) :]
    open(path, "w", encoding="utf-8").write(text)
    print(f"disc layout: BATTLE.PRG starts {sectors} sectors earlier")
    build_mod.xml(path, *files)


if __name__ == "__main__":
    main(*sys.argv[1:])
