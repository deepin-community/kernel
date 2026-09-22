// SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/debugfs.h>

#include "cmdk_mmu600.h"
#include "adk_mmu600.h"
#include "ioctl_mmu600.h"
#include "../../main.h"
/**************************************************************************
 *                         Global Value                                   *
 **************************************************************************/
static struct dentry *mmu600_dbg_root;

static long mmu600test_module_ioctl(struct file *file, unsigned int cmd,
				    unsigned long param);

static const struct file_operations zxdh_smmu_test_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = mmu600test_module_ioctl,
};

/**************************************************************************
 *                         Extern Value                                   *
 **************************************************************************/
extern u32 smmuShowPagetableInfo(struct smmu_pte_address *pstPteAddress);
extern u32 CmdkSysMmuShowPteRecord(u32 udSid, u64 uddVa,
				   struct smmu_pte_address *pstPteAddress);

/**************************************************************************
 *                         Local Function                                 *
 **************************************************************************/
static int mmu600test_ioctl_example(unsigned long arg)
{
	int ret = CMDK_OK;
	u64 addr = 0x20000000;

	if (copy_to_user((void *)arg, (void *)&addr, sizeof(u64)))
		ret = CMDK_ERROR;

	pr_info("[zxdh_rdma] [mmu600test] %s finished.\n", __func__);
	return ret;
}

static int mmu600test_show_all_info(unsigned long arg,
				    struct smmu_pte_address *pstPteAddress)
{
	struct mmu600test_para para;

	if (copy_from_user((void *)&para, (void *)arg,
			   sizeof(struct mmu600test_para))) {
		pr_info("[zxdh_rdma] [mmu600test] %s copy_from_user fail.\n", __func__);
		return CMDK_ERROR;
	}

	smmuShowPagetableInfo(pstPteAddress);
	pr_info("[zxdh_rdma] [mmu600test] %s finished.\n", __func__);
	return CMDK_OK;
}

static int mmu600test_show_pte_info(unsigned long arg,
				    struct smmu_pte_address *pstPteAddress)
{
	struct mmu600test_para para;

	if (copy_from_user((void *)&para, (void *)arg,
			   sizeof(struct mmu600test_para))) {
		pr_info("[zxdh_rdma] [mmu600test] %s copy_from_user fail.\n", __func__);
		return CMDK_ERROR;
	}

	CmdkSysMmuShowPteRecord(para.udSid, para.uddVa, pstPteAddress);
	pr_info("[zxdh_rdma] [mmu600test] %s for sid%d va(0x%llx) finished.\n", __func__,
		para.udSid, para.uddVa);
	return CMDK_OK;
}

static int mmu600test_set_pte(unsigned long arg, struct zxdh_sc_dev *dev)
{
	struct stPteRequest tMmuMmapCfg;

	if (copy_from_user((void *)&tMmuMmapCfg, (void *)arg,
			   sizeof(struct stPteRequest))) {
		pr_info("[zxdh_rdma] [mmu600test] %s copy_from_user fail.\n", __func__);
		return CMDK_ERROR;
	}

	/* dev not init */
	zxdh_smmu_set_pte(&tMmuMmapCfg, dev);
	pr_info("[zxdh_rdma] [mmu600test] %s for sid%d finished.\n", __func__,
		tMmuMmapCfg.udStreamid);
	return CMDK_OK;
}

static int mmu600test_delete_pte(unsigned long arg, struct zxdh_sc_dev *dev)
{
	struct mmu600test_para para;

	if (copy_from_user((void *)&para, (void *)arg,
			   sizeof(struct mmu600test_para))) {
		pr_info("[zxdh_rdma] [mmu600test] %s copy_from_user fail.\n", __func__);
		return CMDK_ERROR;
	}

	/* dev not init */
	pr_info("[zxdh_rdma] [mmu600test] %s for sid%d finished.\n", __func__, para.udSid);
	return CMDK_OK;
}

static int mmu600test_clean_tlb_by_ipa(unsigned long arg)
{
	struct mmu600test_para para;

	if (copy_from_user((void *)&para, (void *)arg,
			   sizeof(struct mmu600test_para))) {
		pr_info("[zxdh_rdma] [mmu600test] %s copy_from_user fail.\n", __func__);
		return CMDK_ERROR;
	}

	CmdkSysMmuCmdTlbCleanByIpa(para.udSid, para.uddVa, para.udPageLvl);
	pr_info("[zxdh_rdma] [mmu600test] %s for sid%d finished.\n", __func__, para.udSid);
	return CMDK_OK;
}

static int mmu600test_sync_tlb(unsigned long arg)
{
	struct mmu600test_para para;

	if (copy_from_user((void *)&para, (void *)arg,
			   sizeof(struct mmu600test_para))) {
		pr_info("[zxdh_rdma] [mmu600test] %s copy_from_user fail.\n", __func__);
		return CMDK_ERROR;
	}

	CmdkSysMmuCmdTlbSync();
	pr_info("[zxdh_rdma] [mmu600test] %s finished.\n", __func__);
	return CMDK_OK;
}

static long mmu600test_module_ioctl(struct file *filp, unsigned int cmd,
				    unsigned long param)
{
	int ret = CMDK_OK;
	struct zxdh_sc_dev *dev = filp->private_data;

	switch (cmd) {
	case MMU600TEST_API_CMD_EXAMPLE: {
		ret = mmu600test_ioctl_example(param);
		break;
	}
	case MMU600TEST_API_CMD_SHOW_ALL_INFO: {
		ret = mmu600test_show_all_info(param, dev->pte_address);
		break;
	}
	case MMU600TEST_API_CMD_SHOW_PTE_INFO: {
		ret = mmu600test_show_pte_info(param, dev->pte_address);
		break;
	}
	case MMU600TEST_API_CMD_SET_PTE: {
		ret = mmu600test_set_pte(param, dev);
		break;
	}
	case MMU600TEST_API_CMD_DELETE_PTE: {
		ret = mmu600test_delete_pte(param, dev);
		break;
	}
	case MMU600TEST_API_CMD_CLEAN_TLB_BY_VA: {
		ret = mmu600test_clean_tlb_by_ipa(param);
		break;
	}
	case MMU600TEST_API_CMD_SYNC_TLB: {
		ret = mmu600test_sync_tlb(param);
		break;
	}
	default: {
		pr_info("[zxdh_rdma] [mmu600test] Unknown ioctl cmd!\n");
		ret = CMDK_ERROR;
	}
	}
	return ret;
}

void zxdh_smmu_test_dbg_init(struct zxdh_sc_dev *dev)
{
#ifdef Z_CONFIG_RDMA_HOST
	struct zxdh_pci_f *rf = container_of(dev, struct zxdh_pci_f, sc_dev);
	const char *name = pci_name(rf->pcidev);
#else
	const char *name = "mmu600test";
#endif
	struct dentry *pfile __attribute__((unused));

	dev->pte_address->mmu600_dbg_dentry =
		debugfs_create_dir(name, mmu600_dbg_root);
	if (dev->pte_address->mmu600_dbg_dentry)
		pfile = debugfs_create_file("mmu600_test", 0600,
					    dev->pte_address->mmu600_dbg_dentry,
					    dev, &zxdh_smmu_test_fops);
	else
		pr_err("[zxdh_rdma] %s: debugfs entry for %s failed\n", __func__, name);
}
EXPORT_SYMBOL(zxdh_smmu_test_dbg_init);

void mmu600test_dbg_exit(struct zxdh_sc_dev *dev)
{
	if (dev->pte_address) {
		pr_err("[zxdh_rdma] %s: removing debugfs entries\n", __func__);
		debugfs_remove_recursive(dev->pte_address->mmu600_dbg_dentry);
		dev->pte_address->mmu600_dbg_dentry = NULL;
	}
}
EXPORT_SYMBOL(mmu600test_dbg_exit);
