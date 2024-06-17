# SPDX-License-Identifier: GPL-2.0
#
# Makefile for Kernel-based Virtual Machine module
#

ccflags-y += -I $(srctree)/$(src)

include $(srctree)/virt/kvm/Makefile.kvm

obj-$(CONFIG_KVM) += kvm.o

kvm-y += sw64.o emulate.o mmio.o kvm_timer.o handle_exit.o perf.o

kvm-$(CONFIG_SUBARCH_C3B) += kvm_core3.o entry_core3.o
kvm-$(CONFIG_SUBARCH_C4) += kvm_core4.o mmu.o entry_core4.o
