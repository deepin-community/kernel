/* SPDX-License-Identifier: GPL-2.0 */
/*
 * This file contains declarations and inline functions for interfacing
 * with the PCI initialization routines.
 */
#include <linux/iommu.h>
#include <linux/iova.h>
#include <linux/spinlock_types.h>
#include <linux/mutex.h>

struct sunway_iommu_bypass_id {
	unsigned int vendor;
	unsigned int device;
};

struct sunway_iommu {
	bool enabled;
	unsigned long index;
	unsigned long node;
	void __iomem *reg_base_addr;
	unsigned long *iommu_dtbr;
	spinlock_t dt_lock;		/* Device Table Lock */

	struct pci_controller *hose_pt;
	struct iommu_device iommu;	/* IOMMU core code handle */
	struct list_head list;
};

struct sunway_iommu_dev {
	struct list_head list;			/* For domain->dev_list */
	struct llist_node dev_data_list;	/* Global device list */
	u16 devid;
	int alias;
	unsigned int passthrough;
	struct sunway_iommu *iommu;
	struct pci_dev *pdev;

	spinlock_t lock;		/* Lock the page table mainly */
	struct sunway_iommu_domain *domain;	/* Domain device is bound to */
};

struct sunway_iommu_domain {
	unsigned int type;
	spinlock_t lock;
	struct mutex api_lock;
	u16 id;				/* Domain ID */
	struct list_head list;		/* For list of all SW domains */
	struct list_head dev_list;	/* List of devices in this domain */
	struct iommu_domain domain;	/* IOMMU domain handle */
	unsigned long *pt_root;		/* Page Table root */
	unsigned int dev_cnt;		/* Number of devices in this domain */
};

struct sw64dev_table_entry {
	u64 data;
};

struct sunway_iommu_group {
	struct pci_dev *dev;
	struct iommu_group *group;
};

#define MAX_DOMAIN_NUM 65536
#define IOVA_PFN(addr) ((addr) >> PAGE_SHIFT)

#define SW64_32BIT_DMA_LIMIT		(0xe0000000 - 1)

#define SW64_IOMMU_ENTRY_VALID		((1UL) << 63)
#define SW64_PTE_LAST_MASK		((1UL) << 8)	/*last stage valid*/
#define SW64_DMA_START			0x1000000
#define SW64_PTE_GRN_MASK		((0x3UL) << 4)
#define PAGE_8M_SHIFT			23
#define PAGE_512M_SHIFT			29
#define PAGE_8G_SHIFT			33
#define PTE_WRITEE			0x2UL
#define PTE_READE			0x1UL
#define SW64_IOMMU_LEVEL1_OFFSET	0x1ff
#define SW64_IOMMU_LEVEL2_OFFSET	0x3ff
#define SW64_IOMMU_LEVEL3_OFFSET	0x3ff
#define SW64_IOMMU_BYPASS		0x1
#define SW64_IOMMU_MAP_FLAG		((0x1UL) << 20)

#define PAGE_SHIFT_IOMMU	18
#define PAGE_SIZE_IOMMU		(_AC(1, UL) << PAGE_SHIFT_IOMMU)

#define PCACHE_FLUSHPADDR_MASK		0xffffffffff80UL
