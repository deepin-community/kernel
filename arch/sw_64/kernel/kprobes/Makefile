# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_KPROBES)           += kprobes.o decode-insn.o
obj-$(CONFIG_KPROBES_ON_FTRACE) += kprobes-ftrace.o
obj-$(CONFIG_RETHOOK)		+= rethook.o rethook_trampoline.o
CFLAGS_REMOVE_rethook.o = $(CC_FLAGS_FTRACE)
CFLAGS_REMOVE_rethook_trampoline.o = $(CC_FLAGS_FTRACE)
