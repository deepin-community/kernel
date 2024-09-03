/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_PCI_H
#define _ASM_SW64_PCI_H

#ifdef __KERNEL__

#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>

/* Offset for PCIe controller registers */
enum SUNWAY_PIU_IOR0 {
	PIUCONFIG0 =		0x0UL,
	EPDMABAR =		0x80UL,
	IOMMUSEGITEM0 =		0x100UL,
	IOMMUEXCPT_CTRL =	0x2100UL,
	MSIADDR =		0x2180UL,
	MSICONFIG0 =		0x2200UL,
	INTACONFIG =		0xa200UL,
	INTBCONFIG =		0xa280UL,
	INTCCONFIG =		0xa300UL,
	INTDCONFIG =		0xa380UL,
	AERERRINTCONFIG =	0xa400UL,
	AERERRMSICONFIG =	0xa480UL,
	PMEINTCONFIG =		0xa500UL,
	PMEMSICONFIG =		0xa580UL,
	HPINTCONFIG =		0xa600UL,
	HPMSICONFIG =		0xa680UL,
	HP_CTRL =		0xac80UL, /* verison >= junzhang */
	HP_WATCHOUT =		0xae00UL, /* version >= junzhang */
	DTBASEADDR =		0xb000UL,
	DTLB_FLUSHALL =		0xb080UL,
	DTLB_FLUSHDEV =		0xb100UL,
	PTLB_FLUSHALL =		0xb180UL,
	PTLB_FLUSHDEV =		0xb200UL,
	PTLB_FLUSHVADDR =	0xb280UL,
	PCACHE_FLUSHALL =	0xb300UL,
	PCACHE_FLUSHDEV =	0xb380UL,
	PCACHE_FLUSHPADDR =	0xb400UL,
	TIMEOUT_CONFIG =	0xb480UL,
	IOMMUEXCPT_STATUS =	0xb500UL,
	IOMMUPAGE_PADDR1 =	0xb580UL,
	IOMMUPAGE_PADDR2 =	0xb600UL,
	IOMMUPAGE_PADDR3 =	0xb680UL,
	PTLB_ACCESS =		0xb700UL,
	PTLB_ITEM_TAG =		0xb780UL,
	PTLB_ITEM_DATA =	0xb800UL,
	PCACHE_ACCESS =		0xb880UL,
	PCACHE_ITEM_TAG =	0xb900UL,
	PCACHE_ITEM_DATA0 =	0xb980UL,
};

enum SUNWAY_PIU_IOR1 {
	PIUCONFIG1 =		0x0UL,
	NEWLTSSMSTATE0 =	0x300UL, /* verison >= junzhang */
	ERRENABLE =		0x880UL,
	RCDEBUGINF1 =		0xc80UL,
	DCACONTROL =		0x1a00UL,
	DEVICEID0 =		0x1a80UL,
};

enum SUNWAY_RC {
	RC_VENDOR_ID =		0x0UL,
	RC_COMMAND =		0x80UL,
	RC_REVISION_ID =	0x100UL,
	RC_PRIMARY_BUS =	0x300UL,
	RC_MSI_CONTROL =	0xa00UL,
	RC_EXP_DEVCAP =		0xe80UL,
	RC_EXP_DEVCTL =		0xf00UL,
	RC_SLOT_CTRL =		0x1100UL,
	RC_LINK_STAT =		0x1000UL,
	RC_CONTROL =		0X1180UL,
	RC_STATUS =		0X1200UL,
	RC_EXP_DEVCTL2 =	0x1300UL,
	RC_PORT_LINK_CTL =	0xe200UL,
	RC_ORDER_RULE_CTL =	0x11680UL,
	RC_MISC_CONTROL_1 =	0x11780UL,
	RC_PHY_INT_REG =	0x80000UL,
	RC_PHY_EXT_GEN1 =	0x82400UL,
	RC_PHY_EXT_GEN2 =	0x82480UL,
};

struct pci_dev;
struct pci_bus;
struct resource;
struct sunway_iommu;
struct page;

struct piu_saved {
	unsigned long piuconfig0;
	unsigned long piuconfig1;
	unsigned long epdmabar;
	unsigned long msiaddr;
	unsigned long msiconfig[256];
	unsigned long iommuexcpt_ctrl;
	unsigned long dtbaseaddr;
	unsigned long hpintconfig;
	unsigned long pmeintconfig;
	unsigned long aererrintconfig;
	unsigned long intaconfig;
	unsigned long intbconfig;
	unsigned long intcconfig;
	unsigned long intdconfig;
};

/* A controller.  Used to manage multiple PCI busses.  */
struct pci_controller {
	struct pci_controller *next;
	struct pci_bus *bus;
	struct resource *io_space;
	struct resource *mem_space;
	struct resource *pre_mem_space;
	struct resource *busn_space;
	unsigned long sparse_mem_base;
	unsigned long dense_mem_base;
	unsigned long sparse_io_base;
	unsigned long dense_io_base;

	/* This one's for the kernel only.  It's in KSEG somewhere.  */
	void __iomem *ep_config_space_base;
	void __iomem *rc_config_space_base;
	void __iomem *piu_ior0_base;
	void __iomem *piu_ior1_base;

	unsigned long index;
	unsigned long node;
	DECLARE_BITMAP(piu_msiconfig, 256);
	int int_irq;
	int service_irq;
	/* For compatibility with current (as of July 2003) pciutils
	 * and XFree86. Eventually will be removed.
	 */
	unsigned int need_domain_info;
	bool iommu_enable;
	bool linkup;
	struct sunway_iommu *pci_iommu;
	bool hotplug_enable;
	int first_busno;
	int last_busno;
	int self_busno;
	void *sysdata;
};

/* Override the logic in pci_scan_bus for skipping already-configured
 * bus numbers.
 */

#define pcibios_assign_all_busses() (pci_has_flag(PCI_REASSIGN_ALL_BUS))

#define PCIBIOS_MIN_IO		0x1000
#define PCIBIOS_MIN_MEM		0

extern void __init sw64_init_pci(void);
extern void __init sw64_device_interrupt(unsigned long vector);
extern void setup_intx_irqs(struct pci_controller *hose);
extern void __init sw64_init_irq(void);
extern void __init sw64_init_arch(void);
extern int sw64_map_irq(const struct pci_dev *dev, u8 slot, u8 pin);
extern struct pci_controller *hose_head;
extern bool sunway_legacy_pci;

#ifdef CONFIG_PCI_SW64
extern void __init setup_chip_pci_ops(void);
#else
#define setup_chip_pci_ops()	do { } while (0)
#endif

extern struct pci_controller *
pci_bus_to_pci_controller(const struct pci_bus *bus);
extern struct pci_controller *bus_num_to_pci_controller(unsigned long bus_num);

extern int sunway_pci_map_irq(const struct pci_dev *dev, u8 slot, u8 pin);
extern void sunway_pci_root_bridge_scan_finish(struct pci_host_bridge *bridge);

extern void __iomem *sunway_pci_map_bus(struct pci_bus *bus,
		unsigned int devfn, int where);
extern int sunway_pci_config_write(struct pci_bus *bus, unsigned int devfn,
		int where, int size, u32 val);
extern int sunway_pci_config_read(struct pci_bus *bus, unsigned int devfn,
		int where, int size, u32 *val);

extern void pci_mark_rc_linkup(struct pci_controller *hose);
extern void pci_clear_rc_linkup(struct pci_controller *hose);
extern int pci_get_rc_linkup(const struct pci_controller *hose);

#ifdef CONFIG_PCI_DOMAINS
static inline int pci_proc_domain(struct pci_bus *bus)
{
	struct pci_controller *hose = pci_bus_to_pci_controller(bus);

	return hose->need_domain_info;
}
#endif

#ifdef CONFIG_NUMA
static inline int __pcibus_to_node(const struct pci_bus *bus)
{
	struct pci_controller *hose;

	hose = pci_bus_to_pci_controller(bus);
	if (!node_online(hose->node))
		return next_node_in(hose->node, node_online_map);
	else
		return hose->node;
}
#define pcibus_to_node(bus)     __pcibus_to_node(bus)
#endif

#endif /* __KERNEL__ */

/* Values for the `which' argument to sys_pciconfig_iobase. */
#define IOBASE_HOSE		0
#define IOBASE_SPARSE_MEM	1
#define IOBASE_DENSE_MEM	2
#define IOBASE_SPARSE_IO	3
#define IOBASE_DENSE_IO		4
#define IOBASE_ROOT_BUS		5
#define IOBASE_FROM_HOSE	0x10000

extern int pci_legacy_read(struct pci_bus *bus, loff_t port, u32 *val,
			   size_t count);
extern int pci_legacy_write(struct pci_bus *bus, loff_t port, u32 val,
			    size_t count);
extern int pci_mmap_legacy_page_range(struct pci_bus *bus,
				      struct vm_area_struct *vma,
				      enum pci_mmap_state mmap_state);
extern void pci_adjust_legacy_attr(struct pci_bus *bus,
				   enum pci_mmap_state mmap_type);
#define HAVE_PCI_LEGACY	1

extern int pci_create_resource_files(struct pci_dev *dev);
extern void pci_remove_resource_files(struct pci_dev *dev);
extern void __init reserve_mem_for_pci(void);
extern int chip_pcie_configure(struct pci_controller *hose);

#define PCI_INTX_ENABLE			((1UL) << 62)
#define PCI_INTX_DISABLE		~((1UL) << 62)
#define PCI_INTX_VALID			(1UL << 63)

#define PCI_VENDOR_ID_JN		0x5656
#define PCI_DEVICE_ID_SW64_ROOT_BRIDGE	0x3231
#define PCI_DEVICE_ID_JN_PCIESW		0x1000
#define PCI_DEVICE_ID_JN_PCIEUSIP	0x1200
#define PCI_DEVICE_ID_JN_PCIE2PCI	0x1314

#define NR_IRQ_VECTORS NR_IRQS

#define LAST_DEVICE_VECTOR		31

#define PCITODMA_OFFSET			0x0	/*0 offset*/

#define MAX_NR_RCS_PER_NODE		12

#endif /* _ASM_SW64_PCI_H */
