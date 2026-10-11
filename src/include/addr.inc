# lui/ori pairs in hand-written code that load a BATTLE.PRG address. Normally these emit
# the original constants. With SHIFTCHECK they use relocations against sym, so that the
# address follows it when BATTLE.PRG moves. ori is kept while the low half has bit 15
# clear (byte-identical, and some loops rely on ori's semantics); config/BATTLE/
# BATTLE.PRG/shift_syms.ld asserts it stays clear. Otherwise addiu is used: the register
# only ever holds the lui result at those sites.

.macro lui_addr reg, sym, addr
.ifdef SHIFTCHECK
    lui     \reg, %hi(\sym)
.else
    lui     \reg, (\addr >> 16)
.endif
.endm

.macro ori_addr rt, rs, sym, addr
.ifdef SHIFTCHECK
.if (\addr & 0x8000)
    addiu   \rt, \rs, %lo(\sym)
.else
    ori     \rt, \rs, %lo(\sym)
.endif
.else
    ori     \rt, \rs, (\addr & 0xFFFF)
.endif
.endm
