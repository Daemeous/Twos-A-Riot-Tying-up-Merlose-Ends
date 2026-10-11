#!/bin/bash
# Builds the disc image with the Callo companion mod. Run from the repo root inside WSL.
#
#   src/BATTLE/BATTLE.PRG/mod/callo.c -> ENDING/CALLO.BIN, a new file placed inside the
#       padding before ENDING/NULL.DAT (no existing file moves), loaded at MOD_BASE.
#   src/BATTLE/BATTLE.PRG/mod/stub.c  -> loader appended to INITBTL.PRG (its last sector has
#       ~1.1 KB spare); INITBTL's SYSTEM.DAT load call is redirected to it.
#   func_80089114's return (BATTLE.PRG) is patched into a tail call to the mod's entry point.
#   vs_battle_exec's per-frame upkeep call (BATTLE.PRG) goes through the mod's frame hook.
#   Permanent stat-up items always give their maximum (build_mod.py maxstats).
#   Nothing goes in BATTLE.PRG's spare tail: Zenith keeps its variables there.
#   MENU3.PRG / MENU0.PRG: calls redirected to the mod's menu wrappers (no size change).
#   src/BATTLE/BATTLE.PRG/mod/menu.c  -> ENDING/CALLOMNU.BIN, the code only menus use; the mod
#       loads it into the gap above the menu overlays when needed (see mod/callo.h).
#
# Output: build/disks/SLUS-01040.bin/.cue. Other PPF mods (Zenith) can be applied on top
# with tools/mod/apply_ppf.py.
set -euo pipefail

PRG=build/data/BATTLE/BATTLE.PRG
INITBTL=build/data/BATTLE/INITBTL.PRG
MODDIR=build/src/BATTLE/BATTLE.PRG/mod
OUT=build/mod
INITBTL_VRAM=0x800F9800
MOD_BASE=0x800F6400     # above Zenith's variables (it uses RAM up to 0x800F60B0)
MOD_LIMIT=0x800F9800   # INITBTL.PRG / effect overlays load here
MOD_LBA=303280         # inside the padding between ENDING.XA (ends 303264) and NULL.DAT (319000)
MOD_MAGIC=0x4F4C4143   # "CALO"
MENU_BASE=0x80109800   # menu module: above MENU3.PRG/MENU0.PRG (callo.h MENU_MODULE)
MENU_LIMIT=0x8010C000  # the heap
MENU_LBA=$((MOD_LBA + 8))  # callo.h MENU_MODULE_LBA; CALLO.BIN gets up to 8 sectors
MENU_MAGIC=0x554E4D43  # "CMNU"

mkdir -p "$OUT" "$MODDIR"
rm -f "$PRG" "$INITBTL" "$MODDIR"/*.o
make -j8 "$PRG" "$INITBTL" "$MODDIR/callo.c.o" "$MODDIR/menu.c.o"

# Address definitions: BATTLE.PRG exports, unexported func_/D_XXXXXXXX by name, then the SLUS
# and BATTLE.PRG symbol lists (e.g. vs_scratch, which the linker script places)
symbols() {  # $1 = object, $2 = optional elf whose symbols it may also use (the mod's)
    mipsel-linux-gnu-nm build/data/BATTLE/BATTLE.PRG.elf | awk '{print $3 " = 0x" $1 ";"}'
    if [[ -n ${2:-} ]]; then
        mipsel-linux-gnu-nm --defined-only "$2" | awk '$2 ~ /[TtDdBbRr]/ {print $3 " = 0x" $1 ";"}'
    fi
    mipsel-linux-gnu-nm -u "$1" | awk '{print $2}' | while read -r s; do
        if [[ $s =~ ^func_[0-9A-F]{8}$ ]]; then
            echo "$s = 0x${s#func_};"
        elif [[ $s =~ ^D_[0-9A-F]{8}$ ]]; then
            echo "$s = 0x${s#D_};"
        else
            awk -v s="$s" '$1 == s && $2 == "=" {sub(/;.*/, "", $3); print s " = " $3 ";"; exit}' \
                config/SLUS_010.40/symbol_addrs.txt config/BATTLE/BATTLE.PRG/symbol_addrs.txt
        fi
    done
}
link() {  # $1 = object, $2 = base, $3 = entry symbol, $4 = output name, $5 = see symbols
    symbols "$1" "${5:-}" | awk '!seen[$1]++' > "$OUT/$4_syms.ld"
    # A name both defined in the object and listed would silently take the listed address
    mipsel-linux-gnu-nm --defined-only -g "$1" | awk '{print $3}' | sort > "$OUT/$4_own.txt"
    if awk '{print $1}' "$OUT/$4_syms.ld" | sort | comm -12 - "$OUT/$4_own.txt" | grep -q .; then
        echo "symbol clash in $4:"; awk '{print $1}' "$OUT/$4_syms.ld" | sort | comm -12 - "$OUT/$4_own.txt"
        exit 1
    fi
    printf 'INCLUDE %s\nSECTIONS\n{\n    . = %s;\n    .mod : { *(.text) *(.rodata*) mod_checked_end = .; *(.data) *(.sdata) *(.bss) *(.sbss) *(COMMON) }\n    /DISCARD/ : { *(*) }\n}\n' \
        "$OUT/$4_syms.ld" "$2" > "$OUT/$4.ld"
    mipsel-linux-gnu-ld -nostdlib --build-id=none --no-warn-rwx-segments -T "$OUT/$4.ld" \
        -e "$3" -o "$OUT/$4.elf" "$1"
    mipsel-linux-gnu-objcopy -O binary -j .mod "$OUT/$4.elf" "$OUT/$4.bin"
}
addr() { mipsel-linux-gnu-nm "$1" | awk -v s="$2" '$3 == s {print "0x" $1}'; }
sym() {  # address from the symbol lists
    awk -v s="$1" '$1 == s && $2 == "=" {sub(/;.*/, "", $3); print $3; exit}'         config/SLUS_010.40/symbol_addrs.txt config/BATTLE/BATTLE.PRG/symbol_addrs.txt
}

# 1. The mod: 0x20-byte header with a function table (see build_mod.py pack), code after it
link "$MODDIR/callo.c.o" "$((MOD_BASE + 0x20))" vs_mod_spawnCallo callo
ENTRY=$(addr "$OUT/callo.elf" vs_mod_spawnCallo)
python3 tools/mod/build_mod.py pack "$OUT/callo.bin" "$OUT/CALLO.BIN" \
    "$MOD_MAGIC" "$MOD_BASE" "$MOD_LIMIT" "$(addr "$OUT/callo.elf" mod_checked_end)" "$ENTRY"
MOD_SIZE=$(stat -c %s "$OUT/CALLO.BIN")
if (( MOD_SIZE > (MENU_LBA - MOD_LBA) * 2048 )); then echo "CALLO.BIN overlaps CALLOMNU.BIN on disc"; exit 1; fi

# 1b. The menu module, linked against the mod
link "$MODDIR/menu.c.o" "$((MENU_BASE + 0x20))" vs_menu_equipEnter menu "$OUT/callo.elf"
python3 tools/mod/build_mod.py pack "$OUT/menu.bin" "$OUT/CALLOMNU.BIN" \
    "$MENU_MAGIC" "$MENU_BASE" "$MENU_LIMIT" "$(addr "$OUT/menu.elf" mod_checked_end)" \
    "$(addr "$OUT/menu.elf" vs_menu_equipEnter)" "$(addr "$OUT/menu.elf" vs_menu_equipLeave)" \
    "$(addr "$OUT/menu.elf" vs_menu_equipCallo)" "$(addr "$OUT/menu.elf" vs_menu_markSpells)" \
    "$(addr "$OUT/menu.elf" vs_menu_toggleSpell)"
MENU_SIZE=$(stat -c %s "$OUT/CALLOMNU.BIN")

# 2. The loader, configured with where to find the mod, after INITBTL.PRG (16-aligned)
{
    echo "/* Generated by tools/mod/build_mod.sh */"
    echo "#define MOD_BASE  $MOD_BASE"
    echo "#define MOD_ENTRY $ENTRY"
    echo "#define MOD_MAGIC $MOD_MAGIC"
    echo "#define MOD_LBA   $MOD_LBA"
    echo "#define MOD_SIZE  $MOD_SIZE"
} > "$OUT/mod_config.h"
make -j8 "$MODDIR/stub.c.o"
LOADER_BASE=$(python3 -c "import os; print(hex(($INITBTL_VRAM + os.path.getsize('$INITBTL') + 15) & ~15))")
link "$MODDIR/stub.c.o" "$LOADER_BASE" vs_mod_loadSystemDat loader
LOADER=$(addr "$OUT/loader.elf" vs_mod_loadSystemDat)

# 3. Patches: INITBTL's SYSTEM.DAT load -> loader; BATTLE.PRG room-spawn return -> mod entry
INITBTL_SIZE=$(awk '/define VS_INITBTL_PRG_SIZE/ {print $3}' build/src/include/lbas.h)
python3 tools/mod/build_mod.py initbtl "$INITBTL" "$OUT/loader.bin" "$LOADER_BASE" "$LOADER" "$INITBTL_SIZE"
python3 tools/mod/build_mod.py maxstats "$PRG" "$(sym vs_main_getRand)"     "$(sym _increaseStrengthRand)" "$(sym _increaseIntelligenceRand)" "$(sym _increaseAgilityRand)"     "$(sym _increaseMaxHpRand)" "$(sym _increaseMaxMpRand)"
python3 tools/mod/build_mod.py battle "$PRG" "$ENTRY" "$(addr "$OUT/callo.elf" vs_mod_frame)"     "$(addr "$OUT/callo.elf" vs_mod_countTeam)" "$(addr "$OUT/callo.elf" vs_mod_hud)"     "$(addr "$OUT/callo.elf" vs_mod_targetStatus)"

# 4. Disc: CALLO.BIN and CALLOMNU.BIN inside the ENDING padding, then the image
mkdir -p build/data/ENDING build/disks
cp "$OUT/CALLO.BIN" "$OUT/CALLOMNU.BIN" build/data/ENDING/
cp disks/SLUS-01040.xml build/disks/SLUS-01040.xml
python3 tools/mod/build_mod.py xml build/disks/SLUS-01040.xml     CALLO.BIN "$MOD_LBA" "$MOD_SIZE" CALLOMNU.BIN "$MENU_LBA" "$MENU_SIZE"
touch build/disks/SLUS-01040.xml
# MENU3.PRG: calls redirected to the MERLOSE wrappers (merlose.h); same size
rm -f build/data/MENU/MENU3.PRG
make -j8 build/data/MENU/MENU3.PRG
python3 tools/mod/build_mod.py menu3 build/data/MENU/MENU3.PRG \
    "$(addr "$OUT/callo.elf" vs_mod_itemNavigation)" "$(addr "$OUT/callo.elf" vs_mod_equipMenu)" \
    "$(addr "$OUT/callo.elf" vs_mod_setMenuRows)" "$(addr "$OUT/callo.elf" vs_mod_loadObjectData)"
# MENU0.PRG (MAGIC menu): her spell choice
rm -f build/data/MENU/MENU0.PRG
make -j8 build/data/MENU/MENU0.PRG
python3 tools/mod/build_mod.py menu0 build/data/MENU/MENU0.PRG     "$(addr "$OUT/callo.elf" vs_mod_magicRows)" "$(addr "$OUT/callo.elf" vs_mod_magicCost)"
python3 tools/mod/build_mod.py sectors build/data data
rm -f build/disks/SLUS-01040.bin build/disks/SLUS-01040.cue  # mkpsxiso would wait for an overwrite prompt
make -j8 image
python3 tools/mod/build_mod.py verify build/disks/SLUS-01040.bin     "$MOD_LBA" build/data/ENDING/CALLO.BIN "$MENU_LBA" build/data/ENDING/CALLOMNU.BIN
