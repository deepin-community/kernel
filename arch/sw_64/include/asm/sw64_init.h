/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_SW64_INIT_H
#define _ASM_SW64_SW64_INIT_H

#include <linux/cpu.h>
#include <linux/pci.h>

#include <asm/sw64io.h>

struct sw64_early_init_ops {
	void (*setup_core_map)(void);
	unsigned long (*get_node_mem)(int nodeid);
	void (*get_smp_info)(void);
};

struct sw64_pci_init_ops {
	unsigned long (*get_rc_enable)(unsigned long node);
	void (*hose_init)(struct pci_controller *hose);
	void (*set_rc_piu)(struct pci_controller *hose);
	int (*check_pci_linkup)(struct pci_controller *hose);
};


struct sw64_chip_init_ops {
	struct sw64_early_init_ops	early_init;
	struct sw64_pci_init_ops	pci_init;
	void (*fixup)(void);
};

struct sw64_chip_ops {
	int (*get_cpu_num)(void);
	void (*device_interrupt)(unsigned long irq_info);
	void (*suspend)(bool wake);
	void (*fixup)(void);
};

extern void sw64_init_noop(void);
extern void setup_chip_ops(void);
extern struct sw64_chip_ops *sw64_chip;
extern struct sw64_chip_init_ops *sw64_chip_init;
#ifdef CONFIG_PM
extern struct syscore_ops io_syscore_ops;

#define PME_CLEAR	0
#define PME_WFW		1	/* wait for wake */
#define	PME_PENDING	2

extern int pme_state;

#endif

DECLARE_PER_CPU(unsigned long, hard_node_id);

#endif /* _ASM_SW64_SW64_INIT_H */
