# SPDX-License-Identifier: GPL-2.0
#
# arch/sw_64/boot/Makefile
#
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Based on arch/arm64/boot/Makefile.
#

OBJCOPYFLAGS_vmlinux.bin := -O binary

targets	:= vmlinux vmlinux.bin vmlinux.bin.gz

quiet_cmd_strip = STRIP  $@
      cmd_strip = $(STRIP) -o $@ $<

# Compressed kernel image
$(obj)/vmlinux.bin.gz: $(obj)/vmlinux.bin FORCE
	$(call if_changed,gzip)
	@echo '  Kernel $@ is ready'

$(obj)/vmlinux: vmlinux FORCE
	$(call if_changed,strip)

$(obj)/vmlinux.bin: $(obj)/vmlinux FORCE
	$(call if_changed,objcopy)
