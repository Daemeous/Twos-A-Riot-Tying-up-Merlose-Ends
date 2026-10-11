/* SHIFTCHECK: BATTLE.PRG symbols that sit inside other objects (string tables, data
 * blobs), defined relative to them so that they move with them. */
vs_battle_menuStrings = _binary_build_assets_BATTLE_BATTLE_PRG_menuStrings_vsString_bin_start + 0x0;
vs_battle_statusStrings = _binary_build_assets_BATTLE_BATTLE_PRG_statusStrings_vsString_bin_start + 0x0;
D_800EABAA = _binary_build_assets_BATTLE_BATTLE_PRG_statusStrings_vsString_bin_start + 0x342;
D_800EBDDC = _binary_build_assets_BATTLE_BATTLE_PRG_doorStrings_vsString_bin_start + 0x0;
D_800F18E8 = D_800F18E4 + 0x4;
D_800EBF58 = _binary_build_assets_BATTLE_BATTLE_PRG_keyStrings_vsStringFixed_bin_start + 0x0;
D_800EBA78 = _binary_build_assets_BATTLE_BATTLE_PRG_menuStrings_vsString_bin_start + 0x9C;
D_800EAA54 = _binary_build_assets_BATTLE_BATTLE_PRG_statusStrings_vsString_bin_start + 0x1EC;
D_800EA9B6 = _binary_build_assets_BATTLE_BATTLE_PRG_statusStrings_vsString_bin_start + 0x14E;
D_800EA984 = _binary_build_assets_BATTLE_BATTLE_PRG_statusStrings_vsString_bin_start + 0x11C;

/* Hand-written code loads these with lui/ori (see lui_addr/ori_addr in macro.inc), which
 * only works while bit 15 of the address is clear. */
ASSERT(!(D_800F5878 & 0x8000), "D_800F5878: bit 15 set, lui/ori would break")
ASSERT(!(D_800F58BC & 0x8000), "D_800F58BC: bit 15 set, lui/ori would break")
ASSERT(!(func_800E3EAC & 0x8000), "func_800E3EAC: bit 15 set, lui/ori would break")
ASSERT(!(func_800E3FF4 & 0x8000), "func_800E3FF4: bit 15 set, lui/ori would break")
ASSERT(!((func_800E3FF4 + 0xFC) & 0x8000), "func_800E3FF4+0xFC: bit 15 set, lui/ori would break")
ASSERT(!((func_800E51C8 + 0x64) & 0x8000), "func_800E51C8+0x64: bit 15 set, lui/ori would break")
ASSERT(!((func_800E511C + 0x20) & 0x8000), "func_800E511C+0x20: bit 15 set, lui/ori would break")
ASSERT(!((func_800E4B70 + 0x3C) & 0x8000), "func_800E4B70+0x3C: bit 15 set, lui/ori would break")

/* Jump-table targets inside hand-written code, which the tables in 83A58.data.s refer
 * to by name in SHIFTCHECK builds (see shift.mk). */
L800DBA4C = func_800DB93C + 0x110;
L800DBA88 = func_800DB93C + 0x14C;
L800DBB98 = func_800DB93C + 0x25C;
L800DEF30 = func_800DEEFC + 0x34;
L800DEF7C = func_800DEEFC + 0x80;
L800DEF80 = func_800DEEFC + 0x84;
L800DF478 = func_800DF458 + 0x20;
L800DF4C8 = func_800DF458 + 0x70;
L800DF4EC = func_800DF458 + 0x94;
L800DF574 = func_800DF458 + 0x11C;
L800DF598 = func_800DF458 + 0x140;
L800DF5A0 = func_800DF458 + 0x148;
L800DF5CC = func_800DF458 + 0x174;
L800DF5F0 = func_800DF458 + 0x198;
L800DF714 = func_800DF458 + 0x2BC;
L800DF720 = func_800DF458 + 0x2C8;
L800DF7F4 = func_800DF458 + 0x39C;
L800E3C3C = func_800E3C30 + 0xC;
L800E3C48 = func_800E3C30 + 0x18;
L800E40F0 = func_800E3FF4 + 0xFC;
L800E50C8 = func_800E4FE0 + 0xE8;
L800E5100 = func_800E50A0 + 0x60;
L800E5110 = func_800E50A0 + 0x70;
