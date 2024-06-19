// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023 Montage Technology. All rights reserved.
 */

#include <linux/types.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/iommu.h>
#include <linux/delay.h>
#include "tsse_dev.h"
#include "tsse_irq.h"
static DEFINE_MUTEX(tsse_dev_table_lock);
static LIST_HEAD(tsse_dev_table);

static DEFINE_MUTEX(algs_lock);

static inline void tsse_list_del(struct list_head *entry)
{
	WRITE_ONCE(entry->next->prev, entry->prev);
	WRITE_ONCE(entry->prev->next, entry->next);
}
static inline void tsse_list_add(struct list_head *new, struct list_head *prev,
				 struct list_head *next)
{
	WRITE_ONCE(new->next, next);
	WRITE_ONCE(new->prev, prev);
	mb();  /* Make sure new node updates first */
	WRITE_ONCE(next->prev, new);
	WRITE_ONCE(prev->next, new);
}

static int tsse_dev_pf_get(struct tsse_dev *vf_tsse_dev)
{
	int ret = 0;
	struct tsse_dev *pf_tsse_dev = NULL;
	struct pci_dev *pf_pci_dev = NULL;

	pf_pci_dev = vf_tsse_dev->tsse_pci_dev.pci_dev->physfn;

	if (!pf_pci_dev)
		return 0;

	pf_tsse_dev = pci_to_tsse_dev(pf_pci_dev);
	if (pf_tsse_dev) {
		if (atomic_add_return(1, &pf_tsse_dev->ref_count) == 1) {
			if (!try_module_get(pf_tsse_dev->owner))
				ret = -EFAULT;
		}
	}
	return ret;
}

static void tsse_dev_pf_put(struct tsse_dev *vf_tsse_dev)
{
	struct tsse_dev *pf_tsse_dev = NULL;
	struct pci_dev *pf_pci_dev = NULL;

	pf_pci_dev = vf_tsse_dev->tsse_pci_dev.pci_dev->physfn;

	if (!pf_pci_dev)
		return;

	pf_tsse_dev = pci_to_tsse_dev(pf_pci_dev);
	if (pf_tsse_dev) {
		if (atomic_sub_return(1, &pf_tsse_dev->ref_count) == 0)
			module_put(pf_tsse_dev->owner);
	}
}

int tsse_dev_get(struct tsse_dev *tdev)
{
	int ref_count = atomic_add_return(1, &tdev->ref_count);

	if (!tsse_dev_started(tdev)) {
		atomic_sub(1, &tdev->ref_count);
		return -EAGAIN;
	}

	if (ref_count == 1) {
		if (!try_module_get(tdev->owner))
			return -EFAULT;
		if (tdev->is_vf)
			return tsse_dev_pf_get(tdev);
	}
	return 0;
}
void tsse_dev_put(struct tsse_dev *tdev)
{
	if (atomic_sub_return(1, &tdev->ref_count) == 0) {
		module_put(tdev->owner);
		if (tdev->is_vf)
			tsse_dev_pf_put(tdev);
	}
}

static int tsse_stop_dev(struct tsse_dev *tdev, bool busy_exit)
{
	int times, max_retry = 150;

	clear_bit(TSSE_DEV_STATUS_STARTING, &tdev->status);
	clear_bit(TSSE_DEV_STATUS_STARTED, &tdev->status);

	for (times = 0; times < max_retry; times++) {
		if (!tsse_dev_in_use(tdev))
			break;
		msleep(100);
	}

	if (times >= max_retry) {
		tsse_dev_err(tdev, "Failed to stop busy device\n");
		if (busy_exit)
			return -EBUSY;
	}
	if (tdev->qpairs_bank.num_qparis != 0) {
		mutex_lock(&tsse_dev_table_lock);
		tsse_list_del(&tdev->list);
		mutex_unlock(&tsse_dev_table_lock);
		tsse_dev_info(tdev, "removed from active dev table list\n");
	}

	tsse_dev_info(tdev, "device stopped\n");

	return 0;
}

int tsse_start_dev(struct tsse_dev *tdev)
{
	struct tsse_dev *tmp_dev;
	struct list_head *prev_node = &tsse_dev_table;
	int ret = 0;

	if (tdev->qpairs_bank.num_qparis == 0) {
		set_bit(TSSE_DEV_STATUS_STARTED, &tdev->status);
		tsse_dev_info(tdev, "device started\n");
		return 0;
	}

	set_bit(TSSE_DEV_STATUS_STARTING, &tdev->status);

	mutex_lock(&tsse_dev_table_lock);

	list_for_each_entry(tmp_dev, &tsse_dev_table, list) {
		if (tmp_dev == tdev) {
			ret = -EEXIST;
			tsse_dev_err(tdev,
				     "The device cannot be added repeatedly\n");
			goto clear_status;
		}
	}

	set_bit(TSSE_DEV_STATUS_STARTED, &tdev->status);
	tsse_list_add(&tdev->list, prev_node, prev_node->next);

	tsse_dev_info(tdev, "device started\n");
	mutex_unlock(&tsse_dev_table_lock);

	return 0;
clear_status:
	mutex_unlock(&tsse_dev_table_lock);
	clear_bit(TSSE_DEV_STATUS_STARTING, &tdev->status);
	clear_bit(TSSE_DEV_STATUS_STARTED, &tdev->status);
	return ret;
}

int tsse_prepare_restart_dev(struct tsse_dev *tdev)
{
	return tsse_stop_dev(tdev, false);
}

void tsse_devmgr_rm_dev(struct tsse_dev *tdev)
{
	tsse_stop_dev(tdev, false);
	tsse_dev_free_irq_vectors(tdev);
	msleep(300);
}

int tsse_devmgr_add_dev(struct tsse_dev *tdev)
{
	int ret;

	ret = tsse_dev_alloc_irq_vectors(tdev);
	if (ret == 0) {
		atomic_set(&tdev->ref_count, 0);
		tdev->status = 0;
		ret = tsse_start_dev(tdev);

		if (ret != 0)
			tsse_dev_free_irq_vectors(tdev);
	}
	return ret;
}

struct list_head *tsse_devmgr_get_head(void)
{
	return &tsse_dev_table;
}
