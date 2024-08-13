/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_ADAPTER_H
#define SSS_ADAPTER_H

#include <linux/types.h>
#include <linux/pci.h>
#include <linux/list.h>
#include <linux/atomic.h>
#include <linux/spinlock.h>

#include "sss_hw_common.h"
#include "sss_hw_uld_driver.h"
#include "sss_hw_svc_cap.h"
#include "sss_sriov_info.h"

#define SSS_MAX_FUNC 4096

struct sss_card_node {
	struct list_head node;
	struct list_head func_list;
	char chip_name[IFNAMSIZ];
	u8 bus_id;
	u8 resvd[7];
	u16 func_num;
	atomic_t channel_timeout_cnt;
	void *func_handle_array[SSS_MAX_FUNC];
	void *dbgtool_info;
};

/* Structure pcidev private */
struct sss_pci_adapter {
	struct pci_dev *pcidev;
	void *hwdev;

	struct sss_hal_dev hal_dev;

	/* Record the upper driver object address,
	 * such as nic_dev and toe_dev, fc_dev
	 */
	void *uld_dev[SSS_SERVICE_TYPE_MAX];

	/* Record the upper driver object name */
	char uld_dev_name[SSS_SERVICE_TYPE_MAX][IFNAMSIZ];

	/* Manage all function device linked by list */
	struct list_head node;

	void __iomem *cfg_reg_bar;
	void __iomem *intr_reg_bar;
	void __iomem *mgmt_reg_bar;
	void __iomem *db_reg_bar;
	u64 db_dwqe_len;
	u64 db_base_paddr;

	struct sss_card_node *chip_node;

	int init_state;

	struct sss_sriov_info sriov_info;

	atomic_t ref_cnt;

	atomic_t uld_ref_cnt[SSS_SERVICE_TYPE_MAX];
	spinlock_t uld_lock; /* protect uld probe and remove */

	/* set when uld driver processing event */
	unsigned long uld_run_state;

	unsigned long uld_attach_state;

	/* lock for attach/detach uld */
	struct mutex uld_attach_mutex;

	spinlock_t dettach_uld_lock; /* spin lock for uld_attach_state access */
};
#endif
