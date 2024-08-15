/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023 Montage Technology. All rights reserved.
 */

#ifndef __TSSE_DEV_H__
#define __TSSE_DEV_H__
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/pci-ats.h>
#include <linux/serial_core.h>
#include <linux/firmware.h>
#include "tsse_ipc.h"

#define TSSE_PCI_MAX_BARS 4
#define TSSE_FW_VERSION_LEN 32

struct tsse_bar {
	void __iomem *virt_addr;
	resource_size_t addr;
	resource_size_t size;
};
struct tsse_dev_pci {
	struct pci_dev *pci_dev;
	struct tsse_bar bars[TSSE_PCI_MAX_BARS];
	u8 revid;
};
enum tsse_dev_status_bit {
	TSSE_DEV_STATUS_STARTING = 0,
	TSSE_DEV_STATUS_STARTED = 1

};
struct tsse_qpairs_bank {
	struct tsse_dev *tsse_dev;
	void __iomem *reg_base;

	u32 num_qparis;
	u32 irq_vec;
};
struct tsse_dev {
	struct module *owner;
	struct dentry *debugfs_dir;
	unsigned long status;
	struct list_head list;
	struct tsse_dev_pci tsse_pci_dev;
	struct tsse_qpairs_bank qpairs_bank;
	atomic_t ref_count;
	bool is_vf;
	int id;
	u32 num_irqs;
	u32 num_vfs;
	struct uart_port *port;
	struct tsse_ipc *ipc;
	void *adi;
	void *mbx_hw;
	const struct firmware *fw;
	char fw_version[TSSE_FW_VERSION_LEN];
	bool fw_version_exist;
};
#define TSSEDEV_TO_DEV(tssedev) (&((tssedev)->tsse_pci_dev.pci_dev->dev))
#define TSSE_DEV_BARS(tssedev) ((tssedev)->tsse_pci_dev.bars)

#include "tsse_log.h"

struct list_head *tsse_devmgr_get_head(void);

int tsse_dev_get(struct tsse_dev *tsse_dev);
void tsse_dev_put(struct tsse_dev *tsse_dev);
int tsse_devmgr_add_dev(struct tsse_dev *tsse_dev);
void tsse_devmgr_rm_dev(struct tsse_dev *tdev);
int tsse_prepare_restart_dev(struct tsse_dev *tdev);
int tsse_start_dev(struct tsse_dev *tdev);

static inline struct tsse_dev *pci_to_tsse_dev(struct pci_dev *pci_dev)
{
	return (struct tsse_dev *)pci_get_drvdata(pci_dev);
}

static inline int tsse_get_cur_node(void)
{
	int cpu, node;

	cpu = get_cpu();
	node = topology_physical_package_id(cpu);
	put_cpu();

	return node;
}

static inline int tsse_dev_started(struct tsse_dev *tdev)
{
	return test_bit(TSSE_DEV_STATUS_STARTED, &tdev->status);
}
static inline int tsse_dev_in_use(struct tsse_dev *tdev)
{
	return atomic_read(&tdev->ref_count) != 0;
}
#endif
