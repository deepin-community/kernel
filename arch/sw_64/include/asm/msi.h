/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_MSI_H
#define _ASM_SW64_MSI_H

#include <asm/sw64io.h>

#define NR_VECTORS NR_IRQS
#define NR_IRQ_VECTORS NR_IRQS

#define AUTO_ASSIGN		0

#define LAST_DEVICE_VECTOR	31

#define MSI_OFFSET		0x44

#define NUM_MSI_IRQS		256

#define PERCPU_MSI_IRQS		256

#define VT_MSIX_MSG_ADDR	(0x8000fee00000UL)
#define VT_MSIX_ADDR_DEST_ID_SHIFT	12
#define VT_MSIX_ADDR_DEST_ID_MASK	(0xff << VT_MSIX_ADDR_DEST_ID_SHIFT)
#define VT_MSIX_ADDR_DEST_ID(dest)	\
	(((dest) << VT_MSIX_ADDR_DEST_ID_SHIFT) & VT_MSIX_ADDR_DEST_ID_MASK)

enum irq_alloc_type;

#ifdef CONFIG_PCI_MSI
extern void vt_handle_pci_msi_interrupt(unsigned long type,
					unsigned long vector,
					unsigned long pci_msi1_addr);
extern void sw64_init_vt_msi_domain(struct irq_domain *parent);
extern int sw64_setup_vt_msi_irqs(struct pci_dev *pdev, int nvec, int type);
extern int vt_pcibios_device_add(struct pci_dev *dev);
extern bool find_free_cpu_vector(const struct cpumask *search_mask,
				 int *found_cpu, int *found_vector);
extern int msi_compose_msg(unsigned int irq, struct msi_msg *msg);
extern void sw64_irq_noop(struct irq_data *d);
extern struct irq_chip sw64_irq_chip;
extern void handle_pci_msi_interrupt(unsigned long type,
				     unsigned long vector,
				     unsigned long pci_msi1_addr);
extern int __init msic_acpi_init(struct irq_domain *parent,
		struct acpi_madt_sw_msic *msic);

#define MSI_ADDR_BASE_HI	0
#define MSI_ADDR_BASE_LO	0x91abc0

#define	MSI_ADDR_SHIFT		20
#define	MSI_ADDR_DEST_ID_SHIFT	10

struct sw64_msi_chip_data {
	spinlock_t cdata_lock;
	union {
		unsigned long msi_config;
		unsigned long msiaddr;
	};
	struct pci_controller *hose;
	unsigned int msi_config_index;
	unsigned int dst_cpu;
	unsigned int vector;
	unsigned int prev_cpu;
	unsigned int prev_vector;
	unsigned int multi_msi;
	bool move_in_progress;
};

static inline int rcid_to_msicid(int rcid)
{
	int msicid = 0;

	msicid |= (rcid_to_domain_id(rcid) << 7);
	msicid |= (rcid_to_thread_id(rcid) << 6);
	msicid |= (rcid_to_core_id(rcid) << 0);

	return msicid;
}

extern void arch_init_msi_domain(struct irq_domain *domain);
enum irq_alloc_type {
	IRQ_ALLOC_TYPE_MSI,
	IRQ_ALLOC_TYPE_MSIX,
	IRQ_ALLOC_TYPE_INTX,
};
struct irq_alloc_info {
	struct msi_desc         *desc;
	enum irq_alloc_type	type;
	struct pci_dev		*msi_dev;
	irq_hw_number_t         hwirq;
};
typedef struct irq_alloc_info msi_alloc_info_t;
#else /* !CONFIG_PCI_MSI */
static inline void handle_pci_msi_interrupt(unsigned long type,
			      unsigned long vector, unsigned long pci_msi1_addr)
{
	pr_warn("SW arch disable CONFIG_PCI_MSI option.\n");
}

static inline void vt_handle_pci_msi_interrupt(unsigned long type,
		unsigned long vector, unsigned long pci_msi1_addr)
{
	pr_warn("SW arch disable CONFIG_PCI_MSI option.\n");
}

static inline int __init msic_acpi_init(struct irq_domain *parent,
		struct acpi_madt_sw_msic *msic) { return 0; }
#endif /* CONFIG_PCI_MSI */
#endif /* _ASM_SW64_MSI_H */
