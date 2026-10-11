# SHIFTCHECK=1: resolve every reference into BATTLE.PRG by name, so that BATTLE.PRG can move.
#
# Normally most binaries get the address of each BATTLE.PRG symbol from splat's
# undefined_*_auto.txt, which hard-codes the original address. Here those addresses are
# filtered out and every binary links against BATTLE.PRG.elf instead, so any reference
# that still depends on a fixed BATTLE.PRG address fails to link.

ifdef CALLO
SHIFTCHECK := 1 # the Callo decomp edition moves BATTLE.PRG (callo.mk)
endif

ifdef SHIFTCHECK

BATTLE_ELF   := $(BUILD)/data/BATTLE/BATTLE.PRG.elf
SHIFT_LDLIBS := $(BUILD)/data/SLUS_010.40.elf $(BATTLE_ELF)
SHIFT_START  := 0x80068800
SHIFT_END    := 0x800F9800
# Symbols of other overlays loaded over BATTLE.PRG's memory, which keep their addresses
SHIFT_KEEP   := vs_ending_exec

$(BINTARGETS) $(BINTARGETS:=.elf): private LDSCRIPT := link.ld shift_funcs.txt shift_syms.txt
$(BUILD)/data/BATTLE/BATTLE.PRG $(BATTLE_ELF): private LDSCRIPT += \
	$(CURDIR)/config/BATTLE/BATTLE.PRG/shift_syms.ld

$(foreach t,$(filter-out $(BUILD)/data/SLUS_010.40,$(BINTARGETS)),\
	$(eval $(t): $(SHIFT_LDLIBS))\
	$(eval $(t): private LDLIBS += $(filter-out $(t).elf,$(SHIFT_LDLIBS))))

$(BUILD)/data/SLUS_010.40: $(BATTLE_ELF)
$(BUILD)/data/SLUS_010.40: private LDLIBS += $(BATTLE_ELF)

$(BINTARGETS:%=%.elf): $$(patsubst $(BUILD)/data/%.elf,$(BUILD)/config/%/shift_funcs.txt,$$@)

$(BUILD)/config/%/shift_funcs.txt: $(BUILD)/config/%/link.d tools/make/shift.mk
	for kind in funcs syms; do
		src=$(@D)/undefined_$${kind}_auto.txt
		if [ -f $$src ]; then
			awk -v lo=$(SHIFT_START) -v hi=$(SHIFT_END) -v keep="$(SHIFT_KEEP)" '
				BEGIN { split(keep, k, " "); for (i in k) kept[k[i]] = 1 }
				{ a = toupper($$3); sub(/;$$/, "", a) }
				($$1 in kept) || !(length(a) == 10 && a >= toupper(lo) && a < toupper(hi))
			' $$src
		fi > $(@D)/shift_$$kind.txt
	done

# ld silently resolves missing symbols to 0 with --oformat=binary, so also link an ELF,
# which reports them. (Only undefined references count: the ELF link also reports bogus
# R_MIPS_26 overflows for jumps into -R libraries that the binary link handles fine.)
$(BINTARGETS): private STRICTLINK = \
	$(LD) $(filter-out --oformat=binary,$(LDFLAGS)) --noinhibit-exe -o $@.strict.elf \
		> $@.strict.log 2>&1 || true; \
	! grep "undefined reference" $@.strict.log

endif

# Hand-written BATTLE.PRG asm assembles differently with SHIFTCHECK (lui_addr/ori_addr), so
# rebuild it, and relink everything, whenever the mode changes.
SHIFT_STAMP := $(BUILD)/shiftcheck.$(if $(SHIFTCHECK),on$(SHIFTPAD)$(if $(CALLO),.callo),off)$(if $(SELECTABLE_ROULETTE),.roulette)
SHIFT_HASM  := $(patsubst %.s,$(BUILD)/%.s.o,$(wildcard src/BATTLE/BATTLE.PRG/*.s))

$(SHIFT_HASM) $(BUILD)/src/BATTLE/BATTLE.PRG/146C.c.o $(BINTARGETS) $(BINTARGETS:=.elf): $(SHIFT_STAMP)
$(SHIFT_STAMP): | $$(@D)/
	$(RM) $(BUILD)/shiftcheck.*
	touch $@

ifdef SHIFTCHECK
$(SHIFT_HASM): ASFLAGS += --defsym SHIFTCHECK=1

# SHIFTPAD=<bytes>: insert that much padding at the start of BATTLE.PRG's code
ifdef SHIFTPAD
$(BUILD)/src/BATTLE/BATTLE.PRG/146C.c.o: CPPFLAGS += -DVS_SHIFTPAD='"$(SHIFTPAD)"'
endif

# The upstream xdelta fixup only works on an unmoved BATTLE.PRG
$(BUILD)/data/BATTLE/BATTLE.PRG: FIXUP = $(VPYTHON) tools/data/BATTLE/battle_prg_fixup.py $@
endif

ifdef SHIFTCHECK
$(BUILD)/src/BATTLE/BATTLE.PRG/823AC.s.o: config/BATTLE/BATTLE.PRG/shift_823AC.s \
		$(BUILD)/src/BATTLE/BATTLE.PRG/823AC.s $(SHIFT_STAMP)
	$(ECHO) Assembling $<
	$(COMPILE.s) -no-pad-sections $(OUTPUT_OPTION) $<
else
$(BUILD)/src/BATTLE/BATTLE.PRG/823AC.s.o: $(SHIFT_STAMP)
endif

# Jump tables in the generated 83A58.data.s point at labels inside hand-written code that
# splat has no names for, so it writes their raw addresses. Name them (defined in
# shift_syms.ld) in a copy of the file.
SHIFT_JTBL_TARGETS := $(shell sed -n 's/^L\(800[0-9A-F]*\) = .*/\1/p' config/BATTLE/BATTLE.PRG/shift_syms.ld)

ifdef SHIFTCHECK
$(BUILD)/src/BATTLE/BATTLE.PRG/83A58.data.s.o: $(BUILD)/src/BATTLE/BATTLE.PRG/83A58.data.s \
		config/BATTLE/BATTLE.PRG/shift_syms.ld $(SHIFT_STAMP)
	$(ECHO) Assembling $< with named jump table targets
	sed -E $(foreach a,$(SHIFT_JTBL_TARGETS),-e 's/\.word 0x$(a)$$/.word L$(a)/') $< > $(@:.o=.shift.s)
	$(COMPILE.s) -no-pad-sections $(OUTPUT_OPTION) $(@:.o=.shift.s)
else
$(BUILD)/src/BATTLE/BATTLE.PRG/83A58.data.s.o: $(SHIFT_STAMP)
endif
