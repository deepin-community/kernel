/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_DEV_MGMT_H
#define HINIC3_DEV_MGMT_H
#include <linux/types.h>
#include <linux/bitops.h>

#include "hinic3_sriov.h"
#include "hinic3_lld.h"

#define HINIC3_VF_PCI_CFG_REG_BAR	0
#define HINIC3_PF_PCI_CFG_REG_BAR	1

#define HINIC3_PCI_INTR_REG_BAR		2
#define HINIC3_PCI_MGMT_REG_BAR		3 /* Only PF have mgmt bar */
#define HINIC3_PCI_DB_BAR		4

#define PRINT_ULD_DETACH_TIMEOUT_INTERVAL	1000 /* 1 second */
#define ULD_LOCK_MIN_USLEEP_TIME		900
#define ULD_LOCK_MAX_USLEEP_TIME		1000

#define BIFUR_RESOURCE_PF_SSID          0x05a1

#define HINIC3_IS_VF_DEV(pdev)	\
	((pdev)->device == HINIC3_DEV_ID_VF || \
	(pdev)->device == HINIC3_DEV_SDI_5_1_ID_VF)
#define HINIC3_IS_SPU_DEV(pdev)	\
	(((pdev)->device == HINIC3_DEV_ID_SPU) || \
	((pdev)->device == HINIC3_DEV_ID_SDI_5_0_PF) || \
	(((pdev)->device == HINIC3_DEV_ID_DPU_PF)))

enum {
	HINIC3_NOT_PROBE = 1,
	HINIC3_PROBE_START = 2,
	HINIC3_PROBE_OK = 3,
	HINIC3_IN_REMOVE = 4,
};

/* Structure pcidev private */
struct hinic3_pcidev {
	struct pci_dev *pcidev;
	void *hwdev;
	struct card_node *chip_node;
	struct hinic3_lld_dev lld_dev;
	/* Record the service object address,
	 * such as hinic3_dev and toe_dev, fc_dev
	 */
	void *uld_dev[SERVICE_T_MAX];
	/* Record the service object name */
	char uld_dev_name[SERVICE_T_MAX][IFNAMSIZ];
	/* It is a the global variable for driver to manage
	 * all function device linked list
	 */
	struct list_head node;

	bool disable_vf_load;
	bool disable_srv_load[SERVICE_T_MAX];

	void __iomem *cfg_reg_base;
	void __iomem *intr_reg_base;
	void __iomem *mgmt_reg_base;
	u64 db_dwqe_len;
	u64 db_base_phy;
	void __iomem *db_base;

	/* lock for attach/detach uld */
	struct mutex pdev_mutex;
	int lld_state;
	u32 rsvd1;

	struct hinic3_sriov_info sriov_info;

	/* setted when uld driver processing event */
	unsigned long state;
	struct pci_device_id id;

	atomic_t ref_cnt;

	atomic_t uld_ref_cnt[SERVICE_T_MAX];
	unsigned long uld_state;
	spinlock_t uld_lock;    /* uld_state lock */

	u16 probe_fault_level;
	u16	rsvd2;
	u64	rsvd4;

	struct workqueue_struct *multi_host_mgmt_workq;
	struct work_struct slave_nic_work;
	struct work_struct slave_vroce_work;

	struct workqueue_struct *migration_probe_workq;
	struct delayed_work migration_probe_dwork;
};

struct hinic_chip_info {
	u8 chip_id;   /* chip id within card */
	u8 card_type; /* hinic_multi_chip_card_type */
	u8 rsvd[10];  /* reserved 10 bytes */
};

struct list_head *get_hinic3_chip_list(void);

int alloc_chip_node(struct hinic3_pcidev *pci_adapter);

void free_chip_node(struct hinic3_pcidev *pci_adapter);

void lld_lock_chip_node(void);

void lld_unlock_chip_node(void);

void hinic3_lld_lock_init(void);

void lld_dev_cnt_init(struct hinic3_pcidev *pci_adapter);
void wait_lld_dev_unused(struct hinic3_pcidev *pci_adapter);

void *hinic3_get_hwdev_by_pcidev(struct pci_dev *pdev);

int hinic3_bar_mmap_param_valid(phys_addr_t phy_addr, unsigned long vmsize);

#endif
