/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_SW64_INIT_H
#define _ASM_SW64_SW64_INIT_H

#include <linux/cpu.h>
#include <linux/pci.h>

#include <asm/sw64io.h>

struct sw64_early_init_ops {
	void (*setup_core_map)(struct cpumask *cpumask);
	unsigned long (*get_node_mem)(int nodeid);
	void (*get_smp_info)(void);
};

struct sw64_pci_init_ops {
	int (*map_irq)(const struct pci_dev *dev, u8 slot, u8 pin);
	unsigned long (*get_rc_enable)(unsigned long node);
	void (*hose_init)(struct pci_controller *hose);
	void (*set_rc_piu)(unsigned long node, unsigned long index);
	int (*check_pci_linkup)(unsigned long node, unsigned long index);
	void (*set_intx)(unsigned long node, unsigned long index,
			unsigned long int_conf);
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
#endif

DECLARE_PER_CPU(unsigned long, hard_node_id);

#endif /* _ASM_SW64_SW64_INIT_H */
