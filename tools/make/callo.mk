# CALLO=1: the decomp edition of the Callo Merlose companion mod (src/BATTLE/BATTLE.PRG/mod).
# Her code is linked into BATTLE.PRG and called from the game's own code at the hooks in
# mod/hooks.h; it implies SHIFTCHECK (shift.mk), which lets BATTLE.PRG move and grow. The code
# only menus use is a separate module (ENDING/CALLOMNU.BIN), loaded on demand (mod/callo.h).
#
#   make CALLO=1 callo-image   -> build/disks/SLUS-01040.bin/.cue

CALLO_SOURCES := $(sort $(wildcard src/BATTLE/BATTLE.PRG/mod/*.c) \n	$(shell grep -rlE 'VS_CALLO|VS_SELECTABLE_ROULETTE|mod/hooks\.h' src --include=*.c))

# Their code depends on the mode, so rebuild them when it changes
$(patsubst %,$(BUILD)/%.o,$(CALLO_SOURCES)): $(SHIFT_STAMP)

# SELECTABLE_ROULETTE=1 (also part of CALLO=1): the boss bonus wheel doesn't spin; Down/Up
# choose the bonus and Circle takes it (MENUF.PRG). On its own it keeps the original layout,
# so MENUF.PRG can go on a disc with the original BATTLE.PRG (patch edition, Zenith).
ifneq ($(CALLO)$(SELECTABLE_ROULETTE),)
CPPFLAGS += -DVS_SELECTABLE_ROULETTE
endif

ifdef CALLO

CPPFLAGS += -DVS_CALLO

CALLO_DIR      := $(BUILD)/src/BATTLE/BATTLE.PRG/mod
CALLO_OBJ      := $(CALLO_DIR)/callo.c.o
CALLO_MENU     := $(BUILD)/data/ENDING/CALLOMNU.BIN
BATTLE_CONFIG  := $(BUILD)/config/BATTLE/BATTLE.PRG
BATTLE_STRICT  := $(BUILD)/data/BATTLE/BATTLE.PRG.strict.elf
CALLO_SECTORS  := $(shell sed -n 's/^\#define VS_CALLO_BATTLE_PRG_SECTORS //p' src/BATTLE/BATTLE.PRG/mod/hooks.h)
# BATTLE.PRG's sectors on the disc, and what SLUS loads of it: up to 0x800F9800
CALLO_PRG_SIZE := $(shell echo $$(( (283 + $(CALLO_SECTORS)) * 2048 )))

# Hooks the menu overlays call
CALLO_EXPORTS := _menuMode vs_mod_optionsRows vs_mod_itemNavigationDone vs_mod_equipMenuDone \
	vs_mod_loadCalloObjectData vs_mod_markSpells vs_mod_spellHighlighted

# The mod goes after BATTLE.PRG's data, before its zero-initialised variables (which are in the
# file), so that its own variables are zeros in the file too.
$(BATTLE_CONFIG)/link_callo.ld: $(BATTLE_CONFIG)/link.d tools/make/callo.mk
	sed 's|^\(\s*\)build/src/BATTLE/BATTLE.PRG/83A58.data.s.o(.data);|&\n\1$(CALLO_OBJ)(.text .rodata .rodata.* .data .sdata .bss .sbss COMMON);|' \
		$(BATTLE_CONFIG)/link.ld > $@
	grep -q 'callo.c.o' $@

$(BUILD)/data/BATTLE/BATTLE.PRG $(BATTLE_ELF): $(CALLO_OBJ) $(BATTLE_CONFIG)/link_callo.ld
$(BUILD)/data/BATTLE/BATTLE.PRG $(BATTLE_ELF): private LDSCRIPT := \
	link_callo.ld shift_funcs.txt shift_syms.txt $(CURDIR)/config/BATTLE/BATTLE.PRG/shift_syms.ld

BATTLE_EXPORTS := $(BATTLE_CONFIG)/exports_callo.txt
$(BATTLE_EXPORTS): config/BATTLE/BATTLE.PRG/exports.txt tools/make/callo.mk | $$(@D)/
	{ cat $<; printf '%s\n' $(CALLO_EXPORTS); } > $@

# Padded to its sectors: SLUS loads all of them
$(BUILD)/data/BATTLE/BATTLE.PRG: FIXUP = \
	$(VPYTHON) tools/data/BATTLE/battle_prg_fixup.py $@; \
	size=$$(stat -c %s $@); \
	if [ $$size -gt $(CALLO_PRG_SIZE) ]; then \
		echo "BATTLE.PRG is $$size bytes, room for $(CALLO_PRG_SIZE)"; exit 1; \
	fi; \
	echo "BATTLE.PRG: $$size bytes, $$(( $(CALLO_PRG_SIZE) - $$size )) to spare"; \
	truncate -s $(CALLO_PRG_SIZE) $@

# The menu overlays must keep their sectors on the disc and end below the menu module
# (0x80109800, callo.h MENU_MODULE)
$(BUILD)/data/MENU/MENU3.PRG: FIXUP = \
	size=$$(stat -c %s $@); [ $$size -le 28672 ] || { echo "MENU3.PRG too big: $$size"; exit 1; }
$(BUILD)/data/MENU/MENU0.PRG: FIXUP = \
	size=$$(stat -c %s $@); [ $$size -le 18432 ] || { echo "MENU0.PRG too big: $$size"; exit 1; }

# The menu module: linked at its load address (after its 0x20-byte header) against BATTLE.PRG
CALLO_MENU_BASE  := 0x80109800
CALLO_MENU_LIMIT := 0x8010C000
CALLO_MENU_MAGIC := 0x554E4D43
CALLO_MENU_LBA   := $(shell sed -n 's/^\#define MENU_MODULE_LBA \([0-9]*\).*/\1/p' src/BATTLE/BATTLE.PRG/mod/callo.h)
CALLO_MENU_ENTRIES := vs_menu_equipEnter vs_menu_equipLeave vs_menu_equipCallo \
	vs_menu_markSpells vs_menu_toggleSpell

$(CALLO_MENU): $(CALLO_DIR)/menu.c.o $(BUILD)/data/BATTLE/BATTLE.PRG tools/mod/link_module.sh | $$(@D)/
	$(ECHO) Linking $@
	tools/mod/link_module.sh $< $$(( $(CALLO_MENU_BASE) + 0x20 )) $(BATTLE_STRICT) \
		$(CALLO_DIR)/menu.elf $(CALLO_DIR)/menu.bin
	addr() { mipsel-linux-gnu-nm $(CALLO_DIR)/menu.elf | awk -v s=$$1 '$$3 == s {print "0x" $$1}'; }
	$(VPYTHON) tools/mod/build_mod.py pack $(CALLO_DIR)/menu.bin $@ $(CALLO_MENU_MAGIC) \
		$(CALLO_MENU_BASE) $(CALLO_MENU_LIMIT) $$(addr mod_checked_end) \
		$(foreach e,$(CALLO_MENU_ENTRIES),$$(addr $(e)))

.PHONY: callo-image
callo-image: $$(TARGETS) $(CALLO_MENU) $(BUILD)/$(DISKCONFIG)
	$(ECHO) Building disk image with Callo
	cp -r --update=none data/ $(BUILD)
	cp $(DISKCONFIG) $(BUILD)/disks/callo.xml
	$(VPYTHON) tools/mod/callo_xml.py $(BUILD)/disks/callo.xml $(CALLO_SECTORS) \
		CALLOMNU.BIN $(CALLO_MENU_LBA) $$(stat -c %s $(CALLO_MENU))
	rm -f $(BUILD)/$(DISKIMAGE) $(BUILD)/$(basename $(DISKIMAGE)).cue
	cd $(BUILD)
	../$(MKPSXISO) disks/callo.xml -o $(DISKIMAGE) -c $(basename $(DISKIMAGE)).cue $(if $(DEBUG),,> /dev/null)

endif
