// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include <linux/types.h>
#include <linux/module.h>

#include "sss_kernel.h"
#include "sss_hw_irq.h"
#include "sss_csr.h"
#include "sss_hwdev.h"
#include "sss_hwif_api.h"

int sss_alloc_db_addr(void *hwdev, void __iomem **db_base)
{
	struct sss_hwif *hwif = NULL;
	u32 id = 0;

	int ret;

	if (!hwdev || !db_base)
		return -EINVAL;

	hwif = SSS_TO_HWIF(hwdev);

	ret = sss_alloc_db_id(hwif, &id);
	if (ret != 0)
		return -EFAULT;

	*db_base = hwif->db_base_vaddr + id * SSS_DB_PAGE_SIZE;

	return 0;
}
EXPORT_SYMBOL(sss_alloc_db_addr);

void sss_free_db_addr(void *hwdev, const void __iomem *db_base)
{
	struct sss_hwif *hwif = NULL;
	u32 id;

	if (!hwdev || !db_base)
		return;

	hwif = SSS_TO_HWIF(hwdev);
	id = SSS_DB_ID(db_base, hwif->db_base_vaddr);

	sss_free_db_id(hwif, id);
}
EXPORT_SYMBOL(sss_free_db_addr);

void sss_chip_set_msix_auto_mask(void *hwdev, u16 msix_id,
				 enum sss_msix_auto_mask flag)
{
	u32 val;

	if (!hwdev)
		return;

	val = (flag == SSS_CLR_MSIX_AUTO_MASK) ?
	      SSS_SET_MSI_CLR_INDIR(1, AUTO_MSK_CLR) :
	      SSS_SET_MSI_CLR_INDIR(1, AUTO_MSK_SET);

	val |= SSS_SET_MSI_CLR_INDIR(msix_id, SIMPLE_INDIR_ID);

	sss_chip_write_reg(SSS_TO_HWIF(hwdev), SSS_CSR_FUNC_MSI_CLR_WR_ADDR, val);
}
EXPORT_SYMBOL(sss_chip_set_msix_auto_mask);

void sss_chip_set_msix_state(void *hwdev, u16 msix_id,
			     enum sss_msix_state flag)
{
	u32 val;

	if (!hwdev)
		return;

	val = (flag == SSS_MSIX_ENABLE) ? SSS_SET_MSI_CLR_INDIR(1, INT_MSK_CLR) :
	      SSS_SET_MSI_CLR_INDIR(1, INT_MSK_SET);
	val |= SSS_SET_MSI_CLR_INDIR(msix_id, SIMPLE_INDIR_ID);

	sss_chip_write_reg(SSS_TO_HWIF(hwdev), SSS_CSR_FUNC_MSI_CLR_WR_ADDR, val);
}
EXPORT_SYMBOL(sss_chip_set_msix_state);

u16 sss_get_global_func_id(void *hwdev)
{
	if (!hwdev)
		return 0;

	return SSS_GET_HWIF_GLOBAL_ID(SSS_TO_HWIF(hwdev));
}
EXPORT_SYMBOL(sss_get_global_func_id);

u8 sss_get_pf_id_of_vf(void *hwdev)
{
	if (!hwdev)
		return 0;

	return SSS_GET_HWIF_PF_ID(SSS_TO_HWIF(hwdev));
}
EXPORT_SYMBOL(sss_get_pf_id_of_vf);

u8 sss_get_pcie_itf_id(void *hwdev)
{
	if (!hwdev)
		return 0;

	return SSS_GET_HWIF_PCI_INTF_ID(SSS_TO_HWIF(hwdev));
}
EXPORT_SYMBOL(sss_get_pcie_itf_id);

enum sss_func_type sss_get_func_type(void *hwdev)
{
	if (!hwdev)
		return 0;

	return SSS_GET_FUNC_TYPE((struct sss_hwdev *)hwdev);
}
EXPORT_SYMBOL(sss_get_func_type);

enum sss_func_type sss_get_func_id(void *hwdev)
{
	if (!hwdev)
		return 0;

	return SSS_GET_FUNC_ID((struct sss_hwdev *)hwdev);
}
EXPORT_SYMBOL(sss_get_func_id);

u16 sss_get_glb_pf_vf_offset(void *hwdev)
{
	if (!hwdev)
		return 0;

	return SSS_GET_HWIF_GLOBAL_VF_OFFSET(SSS_TO_HWIF(hwdev));
}
EXPORT_SYMBOL(sss_get_glb_pf_vf_offset);

u8 sss_get_ppf_id(void *hwdev)
{
	if (!hwdev)
		return 0;

	return SSS_GET_HWIF_PPF_ID(SSS_TO_HWIF(hwdev));
}
EXPORT_SYMBOL(sss_get_ppf_id);
