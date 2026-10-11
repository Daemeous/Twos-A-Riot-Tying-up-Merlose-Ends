#!/bin/bash
# Link a mod module (one object) to run at a fixed address, against a linked BATTLE.PRG:
#   link_module.sh <object> <base> <BATTLE.PRG elf with all its symbols> <out.elf> <out.bin>
# func_/D_XXXXXXXX names BATTLE.PRG doesn't define (other overlays: MAINMENU, MENU3) are taken
# to be at the address in their name. The output section has `mod_checked_end` after the code
# and constants (see build_mod.py pack).
set -euo pipefail
obj=$1 base=$2 syms=$3 out=$4 bin=$5
nm=mipsel-linux-gnu-nm
defs=${out%.elf}_defs.ld
script=${out%.elf}.ld

# Each name it uses: BATTLE.PRG's address for it (given as an absolute definition: linking
# against the ELF with -R reports bogus R_MIPS_26 overflows), else the one in its name
$nm --defined-only "$syms" > "${out%.elf}_syms.txt"
$nm -u "$obj" | awk '{print $2}' | sort -u | while read -r name; do
    addr=$(awk -v s="$name" '$3 == s {print $1; exit}' "${out%.elf}_syms.txt")
    if [[ -n $addr ]]; then
        echo "$name = 0x$addr;"
    elif [[ $name =~ ^(func|D)_([0-9A-F]{8})$ ]]; then
        echo "$name = 0x${BASH_REMATCH[2]};"
    else
        echo "link_module.sh: $name is not defined" >&2
        exit 1
    fi
done > "$defs"
cat > "$script" <<LD
INCLUDE $defs
SECTIONS
{
    . = $base;
    .mod : { *(.text) *(.rodata*) mod_checked_end = .; *(.data) *(.sdata) *(.bss) *(.sbss) *(COMMON) }
    /DISCARD/ : { *(*) }
}
LD
mipsel-linux-gnu-ld -nostdlib --build-id=none --no-warn-rwx-segments -T "$script" \
    -R "$syms" -e 0 -o "$out" "$obj"
mipsel-linux-gnu-objcopy -O binary -j .mod "$out" "$bin"
