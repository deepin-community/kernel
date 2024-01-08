/* SPDX-License-Identifier: GPL-2.0 */
/*
 * This file contains declarations and inline functions for interfacing
 * with the PCI initialization routines.
 */
#ifndef _SW64_KERNEL_PCI_IMPL_H
#define _SW64_KERNEL_PCI_IMPL_H

#include <asm/sw64io.h>

struct pci_dev;
struct pci_controller;

/* The hose list.  */
extern struct pci_controller *hose_head, **hose_tail;

extern void common_init_pci(void);
extern struct pci_controller *alloc_pci_controller(void);
extern struct resource *alloc_resource(void);

extern unsigned long size_for_memory(unsigned long max);

extern const struct dma_map_ops sw64_dma_direct_ops;

extern struct cma *sw64_kvm_cma;
extern struct gen_pool *sw64_kvm_pool;
#endif /* _SW64_KERNEL_PCI_IMPL_H */
