#!/bin/bash
# Link a mod module (one object) to run at a fixed address, against linked code:
#   link_module.sh <object> <base> <symbol ELFs, colon-separated> <out.elf> <out.bin>
# Each name the object uses gets its address from the first ELF that defines it (as an
# absolute definition: linking against an ELF with -R reports bogus R_MIPS_26 overflows); a
# name defined with two different addresses (statics of two files) is an error. func_/D_XXXXXXXX
# names nothing defines (other overlays: MAINMENU, MENU3) are taken to be at the address in
# their name. The output section has `mod_checked_end` after the code and constants (see
# build_mod.py pack).
set -euo pipefail
obj=$1 base=$2 syms=$3 out=$4 bin=$5
nm=mipsel-linux-gnu-nm
defs=${out%.elf}_defs.ld
script=${out%.elf}.ld
table=${out%.elf}_syms.txt

IFS=: read -ra elfs <<< "$syms"
: > "$table"
for elf in "${elfs[@]}"; do
    $nm --defined-only "$elf" | awk -v n="$elf" '{print $3, $1, n}' >> "$table"
done
$nm -u "$obj" | awk '{print $2}' | sort -u | while read -r name; do
    first=$(awk -v s="$name" '$1 == s {print $3; exit}' "$table")
    if [[ -n $first ]]; then
        addrs=$(awk -v s="$name" -v f="$first" '$1 == s && $3 == f {print $2}' "$table" | sort -u)
        if [[ $(wc -l <<< "$addrs") -gt 1 ]]; then
            echo "link_module.sh: $name has several addresses in $first:" $addrs >&2
            exit 1
        fi
        echo "$name = 0x$addrs;"
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
    -e 0 -o "$out" "$obj"
mipsel-linux-gnu-objcopy -O binary -j .mod "$out" "$bin"
