# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the linux kernel.
#

extra-y		:= vmlinux.lds
asflags-y	:= $(KBUILD_CFLAGS)
ccflags-y	:= -Wno-sign-compare

ifdef CONFIG_FTRACE
CFLAGS_REMOVE_ftrace.o = -pg
CFLAGS_REMOVE_insn.o = -pg
CFLAGS_REMOVE_printk.o = -pg
endif

obj-y    := entry.o fpu.o traps.o process.o sys_sw64.o irq.o \
	    irq_sw64.o signal.o setup.o ptrace.o time.o \
	    systbls.o dup_print.o chip_setup.o \
	    insn.o early_init.o topology.o cacheinfo.o \
	    vdso.o vdso/ hmcall.o stacktrace.o idle.o reset.o \
	    head.o termios.o

obj-$(CONFIG_SUBARCH_C3B) += tc.o
obj-$(CONFIG_ACPI) += acpi.o
obj-$(CONFIG_SMP)	+= smp.o
obj-$(CONFIG_MODULES)	+= module.o
obj-$(CONFIG_PM)	+= pm.o
obj-$(CONFIG_SUSPEND)	+= suspend_asm.o suspend.o
obj-$(CONFIG_PERF_EVENTS) += perf_event.o
obj-$(CONFIG_HIBERNATION) += hibernate_asm.o hibernate.o
obj-$(CONFIG_AUDIT)     += audit.o
obj-$(CONFIG_RELOCATABLE)   += relocate.o
obj-$(CONFIG_DEBUG_FS)	+= segvdbg.o unaligned.o
obj-$(CONFIG_JUMP_LABEL) += jump_label.o
obj-$(CONFIG_DEBUG_MATCH)	+= match.o

ifndef CONFIG_PCI
obj-y += pci-noop.o
endif

# Core logic support
obj-$(CONFIG_SW64_CPUAUTOPLUG) += cpuautoplug.o

obj-$(CONFIG_CRASH_DUMP)    += crash_dump.o
obj-$(CONFIG_KEXEC)		+= machine_kexec.o relocate_kernel.o
obj-$(CONFIG_FUNCTION_TRACER)     += ftrace.o entry-ftrace.o
obj-$(CONFIG_KPROBES)           += kprobes/
obj-$(CONFIG_UPROBES)           += uprobes.o
obj-$(CONFIG_EARLY_PRINTK)      += early_printk.o
obj-$(CONFIG_KGDB)     += kgdb.o
obj-$(CONFIG_HAVE_PERF_REGS)   += perf_regs.o
