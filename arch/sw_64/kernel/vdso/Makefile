# SPDX-License-Identifier: GPL-2.0
# Symbols present in the vdso
vdso-syms = rt_sigreturn gettimeofday

# Files to link into the vdso
obj-vdso = $(patsubst %, v%.o, $(vdso-syms))

# Build rules
targets := $(obj-vdso) vdso.so vdso.so.dbg vdso.lds vdso-syms.S
obj-vdso := $(addprefix $(obj)/, $(obj-vdso))

obj-y += vdso.o vdso-syms.o
extra-y += vdso.lds
CPPFLAGS_vdso.lds += -P -C -U$(ARCH)

# vDSO code runs in userspace and -pg doesn't help with profiling anyway.
CFLAGS_REMOVE_vdso.o = -pg
CFLAGS_REMOVE_vrt_sigreturn.o = -pg
CFLAGS_REMOVE_vgettimeofday.o = -pg

ifdef CONFIG_FEEDBACK_COLLECT
# vDSO code runs in userspace, not collecting feedback data.
CFLAGS_REMOVE_vdso.o = -ffeedback-generate
CFLAGS_REMOVE_vrt_sigreturn.o = -ffeedback-generate
CFLAGS_REMOVE_vgettimeofday.o = -ffeedback-generate
endif

# Disable gcov profiling for VDSO code
GCOV_PROFILE := n

# Force dependency
$(obj)/vdso.o: $(obj)/vdso.so

# link rule for the .so file, .lds has to be first
SYSCFLAGS_vdso.so.dbg = $(c_flags)
$(obj)/vdso.so.dbg: $(src)/vdso.lds $(obj-vdso) FORCE
	$(call if_changed,vdsold)
SYSCFLAGS_vdso.so.dbg = -shared -s -Wl,-soname=linux-vdso.so.1 \
                            $(call cc-ldoption, -Wl$(comma)--hash-style=both)

$(obj)/vdso-syms.S: $(obj)/vdso.so FORCE
	$(call if_changed,so2s)

# strip rule for the .so file
$(obj)/%.so: OBJCOPYFLAGS := -S
$(obj)/%.so: $(obj)/%.so.dbg FORCE
	$(call if_changed,objcopy)

# actual build commands
# The DSO images are built using a special linker script
# Add -lgcc so tilepro gets static muldi3 and lshrdi3 definitions.
# Make sure only to export the intended __vdso_xxx symbol offsets.
quiet_cmd_vdsold = VDSOLD  $@
      cmd_vdsold = $(CC) $(KCFLAGS) -nostdlib $(SYSCFLAGS_$(@F)) \
                           -Wl,-T,$(filter-out FORCE,$^) -o $@.tmp -lgcc && \
                   $(CROSS_COMPILE)objcopy \
                           $(patsubst %, -G __vdso_%, $(vdso-syms)) $@.tmp $@ && \
                   rm $@.tmp

# Extracts symbol offsets from the VDSO, converting them into an assembly file
# that contains the same symbols at the same offsets.
quiet_cmd_so2s = SO2S    $@
      cmd_so2s = $(NM) -D $< | $(srctree)/$(src)/so2s.sh > $@

# install commands for the unstripped file
quiet_cmd_vdso_install = INSTALL $@
      cmd_vdso_install = cp $(obj)/$@.dbg $(MODLIB)/vdso/$@

vdso.so: $(obj)/vdso.so.dbg
	@mkdir -p $(MODLIB)/vdso
	$(call cmd,vdso_install)


vdso_install: vdso.so
