// SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#include "osdep.h"
#include "status.h"
#include "hmc.h"
#include "defs.h"
#include "type.h"
#include "protos.h"
#include "virtchnl.h"
#include "ws.h"

/**
 * zxdh_find_vf_dev - get vf struct pointer
 * @dev: shared device pointer
 * @vf_id: virtual function id
 */
struct zxdh_vfdev *zxdh_find_vf_dev(struct zxdh_sc_dev *dev, u16 vf_id)
{
	struct zxdh_vfdev *vf_dev = NULL;
	u16 iw_vf_idx;
	unsigned long flags;

	spin_lock_irqsave(&dev->vf_dev_lock, flags);
	for (iw_vf_idx = 0; iw_vf_idx < dev->num_vfs; iw_vf_idx++) {
		if (dev->vf_dev[iw_vf_idx] &&
		    dev->vf_dev[iw_vf_idx]->vf_id == vf_id) {
			vf_dev = dev->vf_dev[iw_vf_idx];
			refcount_inc(&vf_dev->refcnt);
			break;
		}
	}
	spin_unlock_irqrestore(&dev->vf_dev_lock, flags);

	return vf_dev;
}

/**
 * zxdh_remove_vf_dev - remove vf_dev
 * @dev: shared device pointer
 * @vf_dev: vf dev to be removed
 */
void zxdh_remove_vf_dev(struct zxdh_sc_dev *dev, struct zxdh_vfdev *vf_dev)
{
	u16 iw_vf_idx = 0;
	unsigned long flags;
	
	if(vf_dev) {
		iw_vf_idx = vf_dev->iw_vf_idx;
		zxdh_put_vfdev(dev, vf_dev);
	} else {
		pr_err("[zxdh_rdma] %s vf_dev is NULL!\n",__func__);
		return;
	}
	spin_lock_irqsave(&dev->vf_dev_lock, flags);
	dev->vf_dev[iw_vf_idx] = NULL;
	spin_unlock_irqrestore(&dev->vf_dev_lock, flags);
}

/**
 * zxdh_put_vfdev - put vfdev and free memory
 * @dev: pointer to RDMA dev structure
 * @vf_dev: pointer to RDMA vf dev structure
 */
void zxdh_put_vfdev(struct zxdh_sc_dev *dev, struct zxdh_vfdev *vf_dev)
{
	if (refcount_dec_and_test(&vf_dev->refcnt)) {
		struct zxdh_virt_mem virt_mem;

		if (vf_dev->hmc_info.sd_table.sd_entry) {
			virt_mem.va = vf_dev->hmc_info.sd_table.sd_entry;
			virt_mem.size = sizeof(struct zxdh_hmc_sd_entry) *
					(vf_dev->hmc_info.hmc_entry_total);
			kfree(virt_mem.va);
		}

		virt_mem.va = vf_dev;
		virt_mem.size = sizeof(*vf_dev);
		kfree(virt_mem.va);
	}
}
