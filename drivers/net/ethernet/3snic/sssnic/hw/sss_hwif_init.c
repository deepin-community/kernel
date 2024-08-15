// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include <linux/types.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/module.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_csr.h"
#include "sss_common.h"
#include "sss_hwdev.h"
#include "sss_hwif_init.h"
#include "sss_hwif_api.h"

#define SSS_WAIT_CHIP_READY_TIMEOUT				10000

#define SSS_WAIT_DB_READY_TIMEOUT				60000

#define SSS_MAX_MSIX_ENTRY 2048

#define SSS_AF0_FUNC_GLOBAL_ID_SHIFT	0
#define SSS_AF0_PF_ID_SHIFT			12
#define SSS_AF0_PCI_INTF_ID_SHIFT		17
#define SSS_AF0_VF_IN_PF_SHIFT			20
#define SSS_AF0_FUNC_TYPE_SHIFT			28

#define SSS_AF0_FUNC_GLOBAL_ID_MASK		0xFFF
#define SSS_AF0_PF_ID_MASK			0x1F
#define SSS_AF0_PCI_INTF_ID_MASK		0x7
#define SSS_AF0_VF_IN_PF_MASK			0xFF
#define SSS_AF0_FUNC_TYPE_MASK			0x1

#define SSS_GET_AF0(val, member)				\
	(((val) >> SSS_AF0_##member##_SHIFT) & SSS_AF0_##member##_MASK)

#define SSS_AF2_CEQ_PER_FUNC_SHIFT		0
#define SSS_AF2_DMA_ATTR_PER_FUNC_SHIFT	9
#define SSS_AF2_IRQ_PER_FUNC_SHIFT		16

#define SSS_AF2_CEQ_PER_FUNC_MASK		0x1FF
#define SSS_AF2_DMA_ATTR_PER_FUNC_MASK	0x7
#define SSS_AF2_IRQ_PER_FUNC_MASK		0x7FF

#define SSS_GET_AF2(val, member)				\
	(((val) >> SSS_AF2_##member##_SHIFT) & SSS_AF2_##member##_MASK)

#define SSS_AF3_GLOBAL_VF_ID_OF_NXT_PF_SHIFT	0
#define SSS_AF3_GLOBAL_VF_ID_OF_PF_SHIFT	16

#define SSS_AF3_GLOBAL_VF_ID_OF_NXT_PF_MASK	0xFFF
#define SSS_AF3_GLOBAL_VF_ID_OF_PF_MASK	0xFFF

#define SSS_GET_AF3(val, member)				\
	(((val) >> SSS_AF3_##member##_SHIFT) & SSS_AF3_##member##_MASK)

#define SSS_AF5_OUTBOUND_CTRL_SHIFT		0
#define SSS_AF5_OUTBOUND_CTRL_MASK		0x1

#define SSS_GET_AF5(val, member)				\
	(((val) >> SSS_AF5_##member##_SHIFT) & SSS_AF5_##member##_MASK)

#define SSS_SET_AF5(val, member)				\
	(((val) & SSS_AF5_##member##_MASK) << SSS_AF5_##member##_SHIFT)

#define SSS_CLEAR_AF5(val, member)				\
	((val) & (~(SSS_AF5_##member##_MASK << SSS_AF5_##member##_SHIFT)))

#define SSS_MPF_ELECTION_ID_SHIFT		0

#define SSS_MPF_ELECTION_ID_MASK		0x1F

#define SSS_SET_MPF(val, member)			\
	(((val) & SSS_MPF_ELECTION_##member##_MASK) <<	\
		SSS_MPF_ELECTION_##member##_SHIFT)

#define SSS_GET_MPF(val, member)			\
	(((val) >> SSS_MPF_ELECTION_##member##_SHIFT) &	\
		SSS_MPF_ELECTION_##member##_MASK)

#define SSS_CLEAR_MPF(val, member)			\
	((val) & (~(SSS_MPF_ELECTION_##member##_MASK <<	\
		SSS_MPF_ELECTION_##member##_SHIFT)))

static enum sss_process_ret sss_check_pcie_link_handle(void *data)
{
	u32 status;

	status = sss_chip_get_pcie_link_status(data);
	if (status == SSS_PCIE_LINK_DOWN)
		return SSS_PROCESS_ERR;
	else if (status == SSS_PCIE_LINK_UP)
		return SSS_PROCESS_OK;

	return SSS_PROCESS_DOING;
}

static int sss_wait_pcie_link_up(struct sss_hwdev *hwdev)
{
	int ret;

	ret = sss_check_handler_timeout(hwdev, sss_check_pcie_link_handle,
					SSS_WAIT_CHIP_READY_TIMEOUT, USEC_PER_MSEC);
	if (ret == -ETIMEDOUT)
		sdk_err(hwdev->dev_hdl, "Wait for chip ready timeout\n");

	return ret;
}

static int sss_chip_get_func_attr0(struct sss_hwif *hwif)
{
	u32 attr = sss_chip_read_reg(hwif, SSS_CSR_HW_ATTR0_ADDR);

	if (attr == SSS_PCIE_LINK_DOWN)
		return -EFAULT;

	SSS_SET_HWIF_GLOBAL_ID(hwif, SSS_GET_AF0(attr, FUNC_GLOBAL_ID));
	SSS_SET_HWIF_PF_ID(hwif, SSS_GET_AF0(attr, PF_ID));
	SSS_SET_HWIF_PCI_INTF_ID(hwif, SSS_GET_AF0(attr, PCI_INTF_ID));
	SSS_SET_HWIF_FUNC_TYPE(hwif, SSS_GET_AF0(attr, FUNC_TYPE));

	return 0;
}

static int sss_chip_get_func_attr1(struct sss_hwif *hwif)
{
	u32 attr = sss_chip_read_reg(hwif, SSS_CSR_HW_ATTR1_ADDR);

	if (attr == SSS_PCIE_LINK_DOWN)
		return -EFAULT;

	SSS_SET_HWIF_PPF_ID(hwif, SSS_GET_AF1(attr, PPF_ID));
	SSS_SET_HWIF_AEQ_NUM(hwif, BIT(SSS_GET_AF1(attr, AEQ_PER_FUNC)));

	return 0;
}

static int sss_chip_get_func_attr2(struct sss_hwif *hwif)
{
	u32 attr = sss_chip_read_reg(hwif, SSS_CSR_HW_ATTR2_ADDR);

	if (attr == SSS_PCIE_LINK_DOWN)
		return -EFAULT;

	SSS_SET_HWIF_CEQ_NUM(hwif, (u8)SSS_GET_AF2(attr, CEQ_PER_FUNC));
	SSS_SET_HWIF_IRQ_NUM(hwif, SSS_GET_AF2(attr, IRQ_PER_FUNC));
	if (SSS_GET_HWIF_IRQ_NUM(hwif) > SSS_MAX_MSIX_ENTRY)
		SSS_SET_HWIF_IRQ_NUM(hwif, SSS_MAX_MSIX_ENTRY);
	SSS_SET_HWIF_DMA_ATTR_NUM(hwif, BIT(SSS_GET_AF2(attr, DMA_ATTR_PER_FUNC)));

	return 0;
}

static int sss_chip_get_func_attr3(struct sss_hwif *hwif)
{
	u32 attr = sss_chip_read_reg(hwif, SSS_CSR_HW_ATTR3_ADDR);

	if (attr == SSS_PCIE_LINK_DOWN)
		return -EFAULT;

	SSS_SET_HWIF_GLOBAL_VF_OFFSET(hwif, SSS_GET_AF3(attr, GLOBAL_VF_ID_OF_PF));

	return 0;
}

static int sss_chip_get_func_attr6(struct sss_hwif *hwif)
{
	u32 attr = sss_chip_read_reg(hwif, SSS_CSR_HW_ATTR6_ADDR);

	if (attr == SSS_PCIE_LINK_DOWN)
		return -EFAULT;

	SSS_SET_HWIF_SQ_NUM(hwif, SSS_GET_AF6(attr, FUNC_MAX_SQ));
	SSS_SET_HWIF_MSIX_EN(hwif, SSS_GET_AF6(attr, MSIX_FLEX_EN));

	return 0;
}

static int sss_hwif_init_func_attr(struct sss_hwif *hwif)
{
	int ret;

	ret = sss_chip_get_func_attr0(hwif);
	if (ret != 0)
		return ret;

	ret = sss_chip_get_func_attr1(hwif);
	if (ret != 0)
		return ret;

	ret = sss_chip_get_func_attr2(hwif);
	if (ret != 0)
		return ret;

	ret = sss_chip_get_func_attr3(hwif);
	if (ret != 0)
		return ret;

	ret = sss_chip_get_func_attr6(hwif);
	if (ret != 0)
		return ret;

	return 0;
}

static void sss_chip_init_ppf(struct sss_hwif *hwif)
{
	u32 val;

	val = sss_chip_read_reg(hwif, SSS_CSR_PPF_ELECT_ADDR);
	val = SSS_CLEAR_PPF(val, ID);
	val |= SSS_SET_PPF(SSS_GET_HWIF_GLOBAL_ID(hwif), ID);

	sss_chip_write_reg(hwif, SSS_CSR_PPF_ELECT_ADDR, val);

	/* Check PPF */
	val = sss_chip_read_reg(hwif, SSS_CSR_PPF_ELECT_ADDR);
	SSS_SET_HWIF_PPF_ID(hwif, SSS_GET_PPF(val, ID));
	if (SSS_GET_HWIF_PPF_ID(hwif) == SSS_GET_HWIF_GLOBAL_ID(hwif))
		SSS_SET_HWIF_FUNC_TYPE(hwif, SSS_FUNC_TYPE_PPF);
}

static void sss_chip_get_mpf(struct sss_hwif *hwif)
{
	u32 mpf;

	mpf = sss_chip_read_reg(hwif, SSS_CSR_GLOBAL_MPF_ELECT_ADDR);
	SSS_SET_HWIF_MPF_ID(hwif, SSS_GET_MPF(mpf, ID));
}

static void sss_chip_init_mpf(struct sss_hwif *hwif)
{
	u32 val;

	val = sss_chip_read_reg(hwif, SSS_CSR_GLOBAL_MPF_ELECT_ADDR);
	val = SSS_CLEAR_MPF(val, ID);
	val |= SSS_SET_MPF(SSS_GET_HWIF_GLOBAL_ID(hwif), ID);

	sss_chip_write_reg(hwif, SSS_CSR_GLOBAL_MPF_ELECT_ADDR, val);
}

static int sss_hwif_alloc_db_pool(struct sss_hwif *hwif)
{
	struct sss_db_pool *pool = &hwif->db_pool;
	u32 bit_size;

	bit_size = (hwif->db_dwqe_len > SSS_DB_DWQE_SIZE) ? SSS_DB_MAX_AREAS :
		   ((u32)(hwif->db_dwqe_len / SSS_DB_PAGE_SIZE));
	pool->bitmap = bitmap_zalloc(bit_size, GFP_KERNEL);
	if (!pool->bitmap) {
		pr_err("Fail to allocate db area.\n");
		return -ENOMEM;
	}
	pool->bit_size = bit_size;
	spin_lock_init(&pool->id_lock);

	return 0;
}

static void sss_hwif_free_db_pool(struct sss_db_pool *pool)
{
	kfree(pool->bitmap);
}

static void sss_chip_disable_all_msix(struct sss_hwdev *hwdev)
{
	u16 i;
	u16 irq_num = SSS_GET_HWIF_IRQ_NUM(hwdev->hwif);

	for (i = 0; i < irq_num; i++)
		sss_chip_set_msix_state(hwdev, i, SSS_MSIX_DISABLE);
}

static enum sss_process_ret sss_chip_check_db_ready(void *data)
{
	int outbound_status;
	int db_status;
	struct sss_hwif *hwif = data;
	u32 db_attr = sss_chip_read_reg(hwif, SSS_CSR_HW_ATTR4_ADDR);
	u32 outband_attr = sss_chip_read_reg(hwif, SSS_CSR_HW_ATTR5_ADDR);

	db_status = SSS_GET_AF4(db_attr, DOORBELL_CTRL);
	outbound_status = SSS_GET_AF5(outband_attr, OUTBOUND_CTRL);

	if (db_status == DB_ENABLE && outbound_status == OUTBOUND_ENABLE)
		return SSS_PROCESS_OK;

	return SSS_PROCESS_DOING;
}

static int sss_wait_db_ready(struct sss_hwif *hwif)
{
	return sss_check_handler_timeout(hwif, sss_chip_check_db_ready,
					 SSS_WAIT_DB_READY_TIMEOUT, USEC_PER_MSEC);
}

static void sss_hwif_init_bar_base(struct sss_pci_adapter *adapter)
{
	struct sss_hwif *hwif = SSS_TO_HWIF(adapter->hwdev);

	hwif->db_dwqe_len = adapter->db_dwqe_len;
	hwif->db_base_vaddr = adapter->db_reg_bar;
	hwif->db_base_paddr = adapter->db_base_paddr;

	hwif->mgmt_reg_base = adapter->mgmt_reg_bar;
	hwif->cfg_reg_base = (adapter->mgmt_reg_bar) ?
			     adapter->cfg_reg_bar :
			     ((u8 *)adapter->cfg_reg_bar + SSS_VF_CFG_REG_OFFSET);
}

static int sss_hwif_wait_chip_ready(struct sss_hwdev *hwdev)
{
	int ret;
	u32 db_attr;
	u32 outband_attr;

	ret = sss_wait_pcie_link_up(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Pcie is not link up\n");
		return ret;
	}

	ret = sss_wait_db_ready(hwdev->hwif);
	if (ret != 0) {
		db_attr = sss_chip_read_reg(hwdev->hwif, SSS_CSR_HW_ATTR4_ADDR);
		outband_attr = sss_chip_read_reg(hwdev->hwif, SSS_CSR_HW_ATTR5_ADDR);
		sdk_err(hwdev->dev_hdl, "Hw doorbell is disabled, db 0x%x outbound 0x%x\n",
			db_attr, outband_attr);
		return ret;
	}

	return 0;
}

static void sss_hwif_init_pf(struct sss_hwdev *hwdev)
{
	struct sss_hwif *hwif = hwdev->hwif;

	if (!SSS_IS_VF(hwdev)) {
		sss_chip_init_ppf(hwif);

		if (SSS_IS_PPF(hwdev))
			sss_chip_init_mpf(hwif);
		sss_chip_get_mpf(hwif);
	}

	sss_chip_disable_all_msix(hwdev);

	sss_chip_set_pf_status(hwif, SSS_PF_STATUS_INIT);

	sdk_info(hwdev->dev_hdl,
		 "Global_func_id: %u, func_type: %d, host_id: %u, ppf: %u, mpf: %u\n",
		 SSS_GET_HWIF_GLOBAL_ID(hwif), SSS_GET_HWIF_FUNC_TYPE(hwif),
		 SSS_GET_HWIF_PCI_INTF_ID(hwif), SSS_GET_HWIF_PPF_ID(hwif),
		 SSS_GET_HWIF_MPF_ID(hwif));
}

int sss_hwif_init(struct sss_pci_adapter *adapter)
{
	struct sss_hwdev *hwdev = adapter->hwdev;
	struct sss_hwif *hwif = NULL;
	int ret;

	hwif = kzalloc(sizeof(*hwif), GFP_KERNEL);
	if (!hwif)
		return -ENOMEM;

	hwif->pdev = hwdev->pcidev_hdl;
	hwdev->hwif = hwif;

	sss_hwif_init_bar_base(adapter);

	ret = sss_hwif_alloc_db_pool(hwif);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init db pool.\n");
		goto alloc_db_pool_err;
	}

	ret = sss_hwif_wait_chip_ready(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Chip is not ready\n");
		goto wait_chip_ready_err;
	}

	ret = sss_hwif_init_func_attr(hwif);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail init hwif attr\n");
		goto wait_chip_ready_err;
	}

	sss_hwif_init_pf(hwdev);

	return 0;

wait_chip_ready_err:
	sss_dump_chip_err_info(hwdev);
	sss_hwif_free_db_pool(&hwif->db_pool);
alloc_db_pool_err:
	kfree(hwif);
	hwdev->hwif = NULL;

	return ret;
}

void sss_hwif_deinit(struct sss_hwdev *hwdev)
{
	sss_hwif_free_db_pool(&hwdev->hwif->db_pool);
	kfree(hwdev->hwif);
	hwdev->hwif = NULL;
}
