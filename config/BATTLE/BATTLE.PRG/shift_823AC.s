# SHIFTCHECK replacement for the generated 823AC.s: the same blob, except that its table
# of pointers into itself (D_800EB588) is relocated instead of stored as raw addresses.

.include "macro.inc"

.section .data, "wa"

dlabel __823AC_databin
.incbin "build/assets/BATTLE/BATTLE.PRG/823AC.databin.bin", 0, 0x9DC
dlabel D_800EB588
    .word __823AC_databin + 0x000
    .word __823AC_databin + 0x8D4
    .word __823AC_databin + 0x138
    .word __823AC_databin + 0x27C
    .word __823AC_databin + 0x3C8
    .word __823AC_databin + 0x53C
    .word __823AC_databin + 0x680
    .word __823AC_databin + 0x7C0
    .word __823AC_databin + 0x018
.incbin "build/assets/BATTLE/BATTLE.PRG/823AC.databin.bin", 0xA00
enddlabel __823AC_databin
