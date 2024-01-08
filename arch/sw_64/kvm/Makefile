# SPDX-License-Identifier: GPL-2.0
#
# Makefile for Kernel-based Virtual Machine module
#

ccflags-y += -I $(srctree)/$(src)

include $(srctree)/virt/kvm/Makefile.kvm

obj-$(CONFIG_KVM) += kvm.o

kvm-y += sw64.o
kvm-y += entry.o
kvm-y += emulate.o
kvm-y += mmio.o
kvm-y += kvm_timer.o
kvm-y += handle_exit.o
kvm-y += perf.o
kvm-$(CONFIG_SUBARCH_C3B) += kvm_core3.o kvm_cma.o
kvm-$(CONFIG_SUBARCH_C4) += kvm_core4.o mmu.o
