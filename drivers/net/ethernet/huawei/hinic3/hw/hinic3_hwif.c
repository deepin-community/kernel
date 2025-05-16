// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [COMM]" fmt

#include <linux/types.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/module.h>

#include "ossl_knl.h"
#include "hinic3_csr.h"
#include "hinic3_crm.h"
#include "hinic3_hw.h"
#include "hinic3_common.h"
#include "hinic3_hwdev.h"
#include "hinic3_hwif.h"

#ifndef CONFIG_MODULE_PROF
#define WAIT_HWIF_READY_TIMEOUT				10000
#else
#define WAIT_HWIF_READY_TIMEOUT				30000
#endif

#define HINIC3_WAIT_DOORBELL_AND_OUTBOUND_TIMEOUT	60000

#define MAX_MSIX_ENTRY 2048

#define DB_IDX(db, db_base)	\
	((u32)(((ulong)(db) - (ulong)(db_base)) /	\
	       HINIC3_DB_PAGE_SIZE))

#define HINIC3_AF0_FUNC_GLOBAL_IDX_SHIFT	0
#define HINIC3_AF0_P2P_IDX_SHIFT		12
#define HINIC3_AF0_PCI_INTF_IDX_SHIFT		17
#define HINIC3_AF0_VF_IN_PF_SHIFT		20
#define HINIC3_AF0_FUNC_TYPE_SHIFT		28

#define HINIC3_AF0_FUNC_GLOBAL_IDX_MASK		0xFFF
#define HINIC3_AF0_P2P_IDX_MASK			0x1F
#define HINIC3_AF0_PCI_INTF_IDX_MASK		0x7
#define HINIC3_AF0_VF_IN_PF_MASK		0xFF
#define HINIC3_AF0_FUNC_TYPE_MASK		0x1

#define HINIC3_AF0_GET(val, member)				\
	(((val) >> HINIC3_AF0_##member##_SHIFT) & HINIC3_AF0_##member##_MASK)

#define HINIC3_AF1_PPF_IDX_SHIFT		0
#define HINIC3_AF1_AEQS_PER_FUNC_SHIFT		8
#define HINIC3_AF1_MGMT_INIT_STATUS_SHIFT	30
#define HINIC3_AF1_PF_INIT_STATUS_SHIFT		31

#define HINIC3_AF1_PPF_IDX_MASK			0x3F
#define HINIC3_AF1_AEQS_PER_FUNC_MASK		0x3
#define HINIC3_AF1_MGMT_INIT_STATUS_MASK	0x1
#define HINIC3_AF1_PF_INIT_STATUS_MASK		0x1

#define HINIC3_AF1_GET(val, member)				\
	(((val) >> HINIC3_AF1_##member##_SHIFT) & HINIC3_AF1_##member##_MASK)

#define HINIC3_AF2_CEQS_PER_FUNC_SHIFT		0
#define HINIC3_AF2_DMA_ATTR_PER_FUNC_SHIFT	9
#define HINIC3_AF2_IRQS_PER_FUNC_SHIFT		16

#define HINIC3_AF2_CEQS_PER_FUNC_MASK		0x1FF
#define HINIC3_AF2_DMA_ATTR_PER_FUNC_MASK	0x7
#define HINIC3_AF2_IRQS_PER_FUNC_MASK		0x7FF

#define HINIC3_AF2_GET(val, member)				\
	(((val) >> HINIC3_AF2_##member##_SHIFT) & HINIC3_AF2_##member##_MASK)

#define HINIC3_AF3_GLOBAL_VF_ID_OF_NXT_PF_SHIFT	0
#define HINIC3_AF3_GLOBAL_VF_ID_OF_PF_SHIFT	16

#define HINIC3_AF3_GLOBAL_VF_ID_OF_NXT_PF_MASK	0xFFF
#define HINIC3_AF3_GLOBAL_VF_ID_OF_PF_MASK	0xFFF

#define HINIC3_AF3_GET(val, member)				\
	(((val) >> HINIC3_AF3_##member##_SHIFT) & HINIC3_AF3_##member##_MASK)

#define HINIC3_AF4_DOORBELL_CTRL_SHIFT		0
#define HINIC3_AF4_DOORBELL_CTRL_MASK		0x1

#define HINIC3_AF4_GET(val, member)				\
	(((val) >> HINIC3_AF4_##member##_SHIFT) & HINIC3_AF4_##member##_MASK)

#define HINIC3_AF4_SET(val, member)				\
	(((val) & HINIC3_AF4_##member##_MASK) << HINIC3_AF4_##member##_SHIFT)

#define HINIC3_AF4_CLEAR(val, member)				\
	((val) & (~(HINIC3_AF4_##member##_MASK << HINIC3_AF4_##member##_SHIFT)))

#define HINIC3_AF5_OUTBOUND_CTRL_SHIFT		0
#define HINIC3_AF5_OUTBOUND_CTRL_MASK		0x1

#define HINIC3_AF5_GET(val, member)				\
	(((val) >> HINIC3_AF5_##member##_SHIFT) & HINIC3_AF5_##member##_MASK)

#define HINIC3_AF5_SET(val, member)				\
	(((val) & HINIC3_AF5_##member##_MASK) << HINIC3_AF5_##member##_SHIFT)

#define HINIC3_AF5_CLEAR(val, member)				\
	((val) & (~(HINIC3_AF5_##member##_MASK << HINIC3_AF5_##member##_SHIFT)))

#define HINIC3_AF6_PF_STATUS_SHIFT		0
#define HINIC3_AF6_PF_STATUS_MASK		0xFFFF

#define HINIC3_AF6_FUNC_MAX_SQ_SHIFT	23
#define HINIC3_AF6_FUNC_MAX_SQ_MASK		0x1FF

#define HINIC3_AF6_MSIX_FLEX_EN_SHIFT	22
#define HINIC3_AF6_MSIX_FLEX_EN_MASK	0x1

#define HINIC3_AF6_SET(val, member)				\
	((((u32)(val)) & HINIC3_AF6_##member##_MASK) <<		\
	 HINIC3_AF6_##member##_SHIFT)

#define HINIC3_AF6_GET(val, member)				\
	(((u32)(val) >> HINIC3_AF6_##member##_SHIFT) & HINIC3_AF6_##member##_MASK)

#define HINIC3_AF6_CLEAR(val, member)				\
	((u32)(val) & (~(HINIC3_AF6_##member##_MASK <<		\
	 HINIC3_AF6_##member##_SHIFT)))

#define HINIC3_PPF_ELECT_PORT_IDX_SHIFT		0

#define HINIC3_PPF_ELECT_PORT_IDX_MASK		0x3F

#define HINIC3_PPF_ELECT_PORT_GET(val, member)			\
	(((val) >> HINIC3_PPF_ELECT_PORT_##member##_SHIFT) &	\
	 HINIC3_PPF_ELECT_PORT_##member##_MASK)

#define HINIC3_PPF_ELECTION_IDX_SHIFT		0

#define HINIC3_PPF_ELECTION_IDX_MASK		0x3F

#define HINIC3_PPF_ELECTION_SET(val, member)			\
	(((val) & HINIC3_PPF_ELECTION_##member##_MASK) <<	\
	 HINIC3_PPF_ELECTION_##member##_SHIFT)

#define HINIC3_PPF_ELECTION_GET(val, member)			\
	(((val) >> HINIC3_PPF_ELECTION_##member##_SHIFT) &	\
	 HINIC3_PPF_ELECTION_##member##_MASK)

#define HINIC3_PPF_ELECTION_CLEAR(val, member)			\
	((val) & (~(HINIC3_PPF_ELECTION_##member##_MASK <<	\
		  HINIC3_PPF_ELECTION_##member##_SHIFT)))

#define HINIC3_MPF_ELECTION_IDX_SHIFT		0

#define HINIC3_MPF_ELECTION_IDX_MASK		0x1F

#define HINIC3_MPF_ELECTION_SET(val, member)			\
	(((val) & HINIC3_MPF_ELECTION_##member##_MASK) <<	\
	 HINIC3_MPF_ELECTION_##member##_SHIFT)

#define HINIC3_MPF_ELECTION_GET(val, member)			\
	(((val) >> HINIC3_MPF_ELECTION_##member##_SHIFT) &	\
	 HINIC3_MPF_ELECTION_##member##_MASK)

#define HINIC3_MPF_ELECTION_CLEAR(val, member)			\
	((val) & (~(HINIC3_MPF_ELECTION_##member##_MASK <<	\
	 HINIC3_MPF_ELECTION_##member##_SHIFT)))

#define HINIC3_GET_REG_FLAG(reg)	((reg) & (~(HINIC3_REGS_FLAG_MAKS)))

#define HINIC3_GET_REG_ADDR(reg)	((reg) & (HINIC3_REGS_FLAG_MAKS))

u32 hinic3_hwif_read_reg(struct hinic3_hwif *hwif, u32 reg)
{
	if (HINIC3_GET_REG_FLAG(reg) == HINIC3_MGMT_REGS_FLAG)
		return be32_to_cpu(readl(hwif->mgmt_regs_base +
					 HINIC3_GET_REG_ADDR(reg)));
	else
		return be32_to_cpu(readl(hwif->cfg_regs_base +
					 HINIC3_GET_REG_ADDR(reg)));
}

void hinic3_hwif_write_reg(struct hinic3_hwif *hwif, u32 reg, u32 val)
{
	if (HINIC3_GET_REG_FLAG(reg) == HINIC3_MGMT_REGS_FLAG)
		writel(cpu_to_be32(val),
		       hwif->mgmt_regs_base + HINIC3_GET_REG_ADDR(reg));
	else
		writel(cpu_to_be32(val),
		       hwif->cfg_regs_base + HINIC3_GET_REG_ADDR(reg));
}

bool get_card_present_state(struct hinic3_hwdev *hwdev)
{
	u32 attr1;

	attr1 = hinic3_hwif_read_reg(hwdev->hwif, HINIC3_CSR_FUNC_ATTR1_ADDR);
	if (attr1 == HINIC3_PCIE_LINK_DOWN) {
		sdk_warn(hwdev->dev_hdl, "Card is not present\n");
		return false;
	}

	return true;
}

/**
 * hinic3_get_heartbeat_status - get heart beat status
 * @hwdev: the pointer to hw device
 * Return: 0 - normal, 1 - heart lost, 0xFFFFFFFF - Pcie link down
 **/
u32 hinic3_get_heartbeat_status(void *hwdev)
{
	u32 attr1;

	if (!hwdev)
		return HINIC3_PCIE_LINK_DOWN;

	attr1 = hinic3_hwif_read_reg(((struct hinic3_hwdev *)hwdev)->hwif,
				     HINIC3_CSR_FUNC_ATTR1_ADDR);
	if (attr1 == HINIC3_PCIE_LINK_DOWN)
		return attr1;

	return !HINIC3_AF1_GET(attr1, MGMT_INIT_STATUS);
}
EXPORT_SYMBOL(hinic3_get_heartbeat_status);

#define MIGRATE_HOST_STATUS_CLEAR(host_id, val)	((val) & (~(1U << (host_id))))
#define MIGRATE_HOST_STATUS_SET(host_id, enable)	(((u8)(enable) & 1U) << (host_id))
#define MIGRATE_HOST_STATUS_GET(host_id, val)	(!!((val) & (1U << (host_id))))

int hinic3_set_host_migrate_enable(void *hwdev, u8 host_id, bool enable)
{
	struct hinic3_hwdev *dev = hwdev;

	u32 reg_val;

	if (!dev || host_id > SPU_HOST_ID)
		return -EINVAL;

	if (HINIC3_FUNC_TYPE(dev) != TYPE_PPF) {
		sdk_warn(dev->dev_hdl, "hwdev should be ppf\n");
		return -EINVAL;
	}

	reg_val = hinic3_hwif_read_reg(dev->hwif, HINIC3_MULT_MIGRATE_HOST_STATUS_ADDR);
	reg_val = MIGRATE_HOST_STATUS_CLEAR(host_id, reg_val);
	reg_val |= MIGRATE_HOST_STATUS_SET(host_id, enable);

	hinic3_hwif_write_reg(dev->hwif, HINIC3_MULT_MIGRATE_HOST_STATUS_ADDR, reg_val);

	sdk_info(dev->dev_hdl, "Set migrate host %d status %d, reg value: 0x%x\n",
		 host_id, enable, reg_val);

	return 0;
}
EXPORT_SYMBOL(hinic3_set_host_migrate_enable);

int hinic3_get_host_migrate_enable(void *hwdev, u8 host_id, u8 *migrate_en)
{
	struct hinic3_hwdev *dev = hwdev;

	u32 reg_val;

	if (!dev || !migrate_en || host_id > SPU_HOST_ID)
		return -EINVAL;

	if (HINIC3_FUNC_TYPE(dev) != TYPE_PPF) {
		sdk_warn(dev->dev_hdl, "hwdev should be ppf\n");
		return -EINVAL;
	}

	reg_val = hinic3_hwif_read_reg(dev->hwif, HINIC3_MULT_MIGRATE_HOST_STATUS_ADDR);
	*migrate_en = MIGRATE_HOST_STATUS_GET(host_id, reg_val);

	return 0;
}
EXPORT_SYMBOL(hinic3_get_host_migrate_enable);

static enum hinic3_wait_return check_hwif_ready_handler(void *priv_data)
{
	u32 status;

	status = hinic3_get_heartbeat_status(priv_data);
	if (status == HINIC3_PCIE_LINK_DOWN)
		return WAIT_PROCESS_ERR;
	else if (!status)
		return WAIT_PROCESS_CPL;

	return WAIT_PROCESS_WAITING;
}

static int wait_hwif_ready(struct hinic3_hwdev *hwdev)
{
	int ret;

	ret = hinic3_wait_for_timeout(hwdev, check_hwif_ready_handler,
				      WAIT_HWIF_READY_TIMEOUT, USEC_PER_MSEC);
	if (ret == -ETIMEDOUT) {
		hwdev->probe_fault_level = FAULT_LEVEL_FATAL;
		sdk_err(hwdev->dev_hdl, "Wait for hwif timeout\n");
	}

	return ret;
}

/**
 * set_hwif_attr - set the attributes as members in hwif
 * @hwif: the hardware interface of a pci function device
 * @attr0: the first attribute that was read from the hw
 * @attr1: the second attribute that was read from the hw
 * @attr2: the third attribute that was read from the hw
 * @attr3: the fourth attribute that was read from the hw
 **/
static void set_hwif_attr(struct hinic3_hwif *hwif, u32 attr0, u32 attr1,
			  u32 attr2, u32 attr3, u32 attr6)
{
	hwif->attr.func_global_idx = HINIC3_AF0_GET(attr0, FUNC_GLOBAL_IDX);
	hwif->attr.port_to_port_idx = HINIC3_AF0_GET(attr0, P2P_IDX);
	hwif->attr.pci_intf_idx = HINIC3_AF0_GET(attr0, PCI_INTF_IDX);
	hwif->attr.vf_in_pf = HINIC3_AF0_GET(attr0, VF_IN_PF);
	hwif->attr.func_type = HINIC3_AF0_GET(attr0, FUNC_TYPE);

	hwif->attr.ppf_idx = HINIC3_AF1_GET(attr1, PPF_IDX);
	hwif->attr.num_aeqs = BIT(HINIC3_AF1_GET(attr1, AEQS_PER_FUNC));
	hwif->attr.num_ceqs = (u8)HINIC3_AF2_GET(attr2, CEQS_PER_FUNC);
	hwif->attr.num_irqs = HINIC3_AF2_GET(attr2, IRQS_PER_FUNC);
	if (hwif->attr.num_irqs > MAX_MSIX_ENTRY)
		hwif->attr.num_irqs = MAX_MSIX_ENTRY;

	hwif->attr.num_dma_attr = BIT(HINIC3_AF2_GET(attr2, DMA_ATTR_PER_FUNC));

	hwif->attr.global_vf_id_of_pf = HINIC3_AF3_GET(attr3,
						       GLOBAL_VF_ID_OF_PF);

	hwif->attr.num_sq = HINIC3_AF6_GET(attr6, FUNC_MAX_SQ);
	hwif->attr.msix_flex_en = HINIC3_AF6_GET(attr6, MSIX_FLEX_EN);
}

/**
 * get_hwif_attr - read and set the attributes as members in hwif
 * @hwif: the hardware interface of a pci function device
 **/
static int get_hwif_attr(struct hinic3_hwif *hwif)
{
	u32 addr, attr0, attr1, attr2, attr3, attr6;

	addr   = HINIC3_CSR_FUNC_ATTR0_ADDR;
	attr0  = hinic3_hwif_read_reg(hwif, addr);
	if (attr0 == HINIC3_PCIE_LINK_DOWN)
		return -EFAULT;

	addr   = HINIC3_CSR_FUNC_ATTR1_ADDR;
	attr1  = hinic3_hwif_read_reg(hwif, addr);
	if (attr1 == HINIC3_PCIE_LINK_DOWN)
		return -EFAULT;

	addr   = HINIC3_CSR_FUNC_ATTR2_ADDR;
	attr2  = hinic3_hwif_read_reg(hwif, addr);
	if (attr2 == HINIC3_PCIE_LINK_DOWN)
		return -EFAULT;

	addr   = HINIC3_CSR_FUNC_ATTR3_ADDR;
	attr3  = hinic3_hwif_read_reg(hwif, addr);
	if (attr3 == HINIC3_PCIE_LINK_DOWN)
		return -EFAULT;

	addr   = HINIC3_CSR_FUNC_ATTR6_ADDR;
	attr6  = hinic3_hwif_read_reg(hwif, addr);
	if (attr6 == HINIC3_PCIE_LINK_DOWN)
		return -EFAULT;

	set_hwif_attr(hwif, attr0, attr1, attr2, attr3, attr6);

	return 0;
}

void hinic3_set_pf_status(struct hinic3_hwif *hwif,
			  enum hinic3_pf_status status)
{
	u32 attr6 = hinic3_hwif_read_reg(hwif, HINIC3_CSR_FUNC_ATTR6_ADDR);

	attr6 = HINIC3_AF6_CLEAR(attr6, PF_STATUS);
	attr6 |= HINIC3_AF6_SET(status, PF_STATUS);

	if (hwif->attr.func_type == TYPE_VF)
		return;

	hinic3_hwif_write_reg(hwif, HINIC3_CSR_FUNC_ATTR6_ADDR, attr6);
}

enum hinic3_pf_status hinic3_get_pf_status(struct hinic3_hwif *hwif)
{
	u32 attr6 = hinic3_hwif_read_reg(hwif, HINIC3_CSR_FUNC_ATTR6_ADDR);

	return HINIC3_AF6_GET(attr6, PF_STATUS);
}

static enum hinic3_doorbell_ctrl hinic3_get_doorbell_ctrl_status(struct hinic3_hwif *hwif)
{
	u32 attr4 = hinic3_hwif_read_reg(hwif, HINIC3_CSR_FUNC_ATTR4_ADDR);

	return HINIC3_AF4_GET(attr4, DOORBELL_CTRL);
}

static enum hinic3_outbound_ctrl hinic3_get_outbound_ctrl_status(struct hinic3_hwif *hwif)
{
	u32 attr5 = hinic3_hwif_read_reg(hwif, HINIC3_CSR_FUNC_ATTR5_ADDR);

	return HINIC3_AF5_GET(attr5, OUTBOUND_CTRL);
}

void hinic3_enable_doorbell(struct hinic3_hwif *hwif)
{
	u32 addr, attr4;

	addr = HINIC3_CSR_FUNC_ATTR4_ADDR;
	attr4 = hinic3_hwif_read_reg(hwif, addr);

	attr4 = HINIC3_AF4_CLEAR(attr4, DOORBELL_CTRL);
	attr4 |= HINIC3_AF4_SET(ENABLE_DOORBELL, DOORBELL_CTRL);

	hinic3_hwif_write_reg(hwif, addr, attr4);
}

void hinic3_disable_doorbell(struct hinic3_hwif *hwif)
{
	u32 addr, attr4;

	addr = HINIC3_CSR_FUNC_ATTR4_ADDR;
	attr4 = hinic3_hwif_read_reg(hwif, addr);

	attr4 = HINIC3_AF4_CLEAR(attr4, DOORBELL_CTRL);
	attr4 |= HINIC3_AF4_SET(DISABLE_DOORBELL, DOORBELL_CTRL);

	hinic3_hwif_write_reg(hwif, addr, attr4);
}

/**
 * set_ppf - try to set hwif as ppf and set the type of hwif in this case
 * @hwif: the hardware interface of a pci function device
 **/
static void set_ppf(struct hinic3_hwif *hwif)
{
	struct hinic3_func_attr *attr = &hwif->attr;
	u32 addr, val, ppf_election;

	/* Read Modify Write */
	addr  = HINIC3_CSR_PPF_ELECTION_ADDR;

	val = hinic3_hwif_read_reg(hwif, addr);
	val = HINIC3_PPF_ELECTION_CLEAR(val, IDX);

	ppf_election =  HINIC3_PPF_ELECTION_SET(attr->func_global_idx, IDX);
	val |= ppf_election;

	hinic3_hwif_write_reg(hwif, addr, val);

	/* Check PPF */
	val = hinic3_hwif_read_reg(hwif, addr);

	attr->ppf_idx = HINIC3_PPF_ELECTION_GET(val, IDX);
	if (attr->ppf_idx == attr->func_global_idx)
		attr->func_type = TYPE_PPF;
}

/**
 * get_mpf - get the mpf index into the hwif
 * @hwif: the hardware interface of a pci function device
 **/
static void get_mpf(struct hinic3_hwif *hwif)
{
	struct hinic3_func_attr *attr = &hwif->attr;
	u32 mpf_election, addr;

	addr = HINIC3_CSR_GLOBAL_MPF_ELECTION_ADDR;

	mpf_election = hinic3_hwif_read_reg(hwif, addr);
	attr->mpf_idx = HINIC3_MPF_ELECTION_GET(mpf_election, IDX);
}

/**
 * set_mpf - try to set hwif as mpf and set the mpf idx in hwif
 * @hwif: the hardware interface of a pci function device
 **/
static void set_mpf(struct hinic3_hwif *hwif)
{
	struct hinic3_func_attr *attr = &hwif->attr;
	u32 addr, val, mpf_election;

	/* Read Modify Write */
	addr  = HINIC3_CSR_GLOBAL_MPF_ELECTION_ADDR;

	val = hinic3_hwif_read_reg(hwif, addr);

	val = HINIC3_MPF_ELECTION_CLEAR(val, IDX);
	mpf_election = HINIC3_MPF_ELECTION_SET(attr->func_global_idx, IDX);

	val |= mpf_election;
	hinic3_hwif_write_reg(hwif, addr, val);
}

static int init_hwif(struct hinic3_hwdev *hwdev, void *cfg_reg_base, void *intr_reg_base,
		     void *mgmt_regs_base)
{
	struct hinic3_hwif *hwif = NULL;

	hwif = kzalloc(sizeof(*hwif), GFP_KERNEL);
	if (!hwif)
		return -ENOMEM;

	hwdev->hwif = hwif;
	hwif->pdev = hwdev->pcidev_hdl;

	/* if function is VF, mgmt_regs_base will be NULL */
	hwif->cfg_regs_base = mgmt_regs_base ? cfg_reg_base :
		(u8 *)cfg_reg_base + HINIC3_VF_CFG_REG_OFFSET;

	hwif->intr_regs_base = intr_reg_base;
	hwif->mgmt_regs_base = mgmt_regs_base;

	return 0;
}

static int init_db_area_idx(struct hinic3_hwif *hwif, u64 db_base_phy, u8 *db_base,
			    u64 db_dwqe_len)
{
	struct hinic3_free_db_area *free_db_area = &hwif->free_db_area;
	u32 db_max_areas;

	hwif->db_base_phy = db_base_phy;
	hwif->db_base = db_base;
	hwif->db_dwqe_len = db_dwqe_len;

	db_max_areas = (db_dwqe_len > HINIC3_DB_DWQE_SIZE) ?
		      HINIC3_DB_MAX_AREAS :
		      (u32)(db_dwqe_len / HINIC3_DB_PAGE_SIZE);
	free_db_area->db_bitmap_array = bitmap_zalloc(db_max_areas, GFP_KERNEL);
	if (!free_db_area->db_bitmap_array) {
		pr_err("Failed to allocate db area.\n");
		return -ENOMEM;
	}
	free_db_area->db_max_areas = db_max_areas;
	spin_lock_init(&free_db_area->idx_lock);
	return 0;
}

static void free_db_area(struct hinic3_free_db_area *free_db_area)
{
	spin_lock_deinit(&free_db_area->idx_lock);
	kfree(free_db_area->db_bitmap_array);
	free_db_area->db_bitmap_array = NULL;
}

static int get_db_idx(struct hinic3_hwif *hwif, u32 *idx)
{
	struct hinic3_free_db_area *free_db_area = &hwif->free_db_area;
	u32 pg_idx;

	spin_lock(&free_db_area->idx_lock);
	pg_idx = (u32)find_first_zero_bit(free_db_area->db_bitmap_array,
					  free_db_area->db_max_areas);
	if (pg_idx == free_db_area->db_max_areas) {
		spin_unlock(&free_db_area->idx_lock);
		return -ENOMEM;
	}
	set_bit(pg_idx, free_db_area->db_bitmap_array);
	spin_unlock(&free_db_area->idx_lock);

	*idx = pg_idx;

	return 0;
}

static void free_db_idx(struct hinic3_hwif *hwif, u32 idx)
{
	struct hinic3_free_db_area *free_db_area = &hwif->free_db_area;

	if (idx >= free_db_area->db_max_areas)
		return;

	spin_lock(&free_db_area->idx_lock);
	clear_bit((int)idx, free_db_area->db_bitmap_array);

	spin_unlock(&free_db_area->idx_lock);
}

void hinic3_free_db_addr(void *hwdev, const void __iomem *db_base,
			 void __iomem *dwqe_base)
{
	struct hinic3_hwif *hwif = NULL;
	u32 idx;

	if (!hwdev || !db_base)
		return;

	hwif = ((struct hinic3_hwdev *)hwdev)->hwif;
	idx = DB_IDX(db_base, hwif->db_base);

	free_db_idx(hwif, idx);
}
EXPORT_SYMBOL(hinic3_free_db_addr);

int hinic3_alloc_db_addr(void *hwdev, void __iomem **db_base,
			 void __iomem **dwqe_base)
{
	struct hinic3_hwif *hwif = NULL;
	u32 idx = 0;
	int err;

	if (!hwdev || !db_base)
		return -EINVAL;

	hwif = ((struct hinic3_hwdev *)hwdev)->hwif;

	err = get_db_idx(hwif, &idx);
	if (err)
		return -EFAULT;

	*db_base = hwif->db_base + idx * HINIC3_DB_PAGE_SIZE;

	if (!dwqe_base)
		return 0;

	*dwqe_base = (u8 *)*db_base + HINIC3_DWQE_OFFSET;

	return 0;
}
EXPORT_SYMBOL(hinic3_alloc_db_addr);

void hinic3_free_db_phy_addr(void *hwdev, u64 db_base, u64 dwqe_base)
{
	struct hinic3_hwif *hwif = NULL;
	u32 idx;

	if (!hwdev)
		return;

	hwif = ((struct hinic3_hwdev *)hwdev)->hwif;
	idx = DB_IDX(db_base, hwif->db_base_phy);

	free_db_idx(hwif, idx);
}
EXPORT_SYMBOL(hinic3_free_db_phy_addr);

int hinic3_alloc_db_phy_addr(void *hwdev, u64 *db_base, u64 *dwqe_base)
{
	struct hinic3_hwif *hwif = NULL;
	u32 idx;
	int err;

	if (!hwdev || !db_base || !dwqe_base)
		return -EINVAL;

	hwif = ((struct hinic3_hwdev *)hwdev)->hwif;

	err = get_db_idx(hwif, &idx);
	if (err)
		return -EFAULT;

	*db_base = hwif->db_base_phy + idx * HINIC3_DB_PAGE_SIZE;
	*dwqe_base = *db_base + HINIC3_DWQE_OFFSET;

	return 0;
}
EXPORT_SYMBOL(hinic3_alloc_db_phy_addr);

void hinic3_set_msix_auto_mask_state(void *hwdev, u16 msix_idx,
				     enum hinic3_msix_auto_mask flag)
{
	struct hinic3_hwif *hwif = NULL;
	u32 mask_bits;
	u32 addr;

	if (!hwdev)
		return;

	hwif = ((struct hinic3_hwdev *)hwdev)->hwif;

	if (flag)
		mask_bits = HINIC3_MSI_CLR_INDIR_SET(1, AUTO_MSK_SET);
	else
		mask_bits = HINIC3_MSI_CLR_INDIR_SET(1, AUTO_MSK_CLR);

	mask_bits = mask_bits |
		    HINIC3_MSI_CLR_INDIR_SET(msix_idx, SIMPLE_INDIR_IDX);

	addr = HINIC3_CSR_FUNC_MSI_CLR_WR_ADDR;
	hinic3_hwif_write_reg(hwif, addr, mask_bits);
}
EXPORT_SYMBOL(hinic3_set_msix_auto_mask_state);

void hinic3_set_msix_state(void *hwdev, u16 msix_idx,
			   enum hinic3_msix_state flag)
{
	struct hinic3_hwif *hwif = NULL;
	u32 mask_bits;
	u32 addr;
	u8 int_msk = 1;

	if (!hwdev)
		return;

	hwif = ((struct hinic3_hwdev *)hwdev)->hwif;

	if (flag)
		mask_bits = HINIC3_MSI_CLR_INDIR_SET(int_msk, INT_MSK_SET);
	else
		mask_bits = HINIC3_MSI_CLR_INDIR_SET(int_msk, INT_MSK_CLR);
	mask_bits = mask_bits |
		    HINIC3_MSI_CLR_INDIR_SET(msix_idx, SIMPLE_INDIR_IDX);

	addr = HINIC3_CSR_FUNC_MSI_CLR_WR_ADDR;
	hinic3_hwif_write_reg(hwif, addr, mask_bits);
}
EXPORT_SYMBOL(hinic3_set_msix_state);

static void disable_all_msix(struct hinic3_hwdev *hwdev)
{
	u16 num_irqs = hwdev->hwif->attr.num_irqs;
	u16 i;

	for (i = 0; i < num_irqs; i++)
		hinic3_set_msix_state(hwdev, i, HINIC3_MSIX_DISABLE);
}

static void enable_all_msix(struct hinic3_hwdev *hwdev)
{
	u16 num_irqs = hwdev->hwif->attr.num_irqs;
	u16 i;

	for (i = 0; i < num_irqs; i++)
		hinic3_set_msix_state(hwdev, i, HINIC3_MSIX_ENABLE);
}

static enum hinic3_wait_return check_db_outbound_enable_handler(void *priv_data)
{
	struct hinic3_hwif *hwif = priv_data;
	enum hinic3_doorbell_ctrl db_ctrl;
	enum hinic3_outbound_ctrl outbound_ctrl;

	db_ctrl = hinic3_get_doorbell_ctrl_status(hwif);
	outbound_ctrl = hinic3_get_outbound_ctrl_status(hwif);
	if (outbound_ctrl == ENABLE_OUTBOUND && db_ctrl == ENABLE_DOORBELL)
		return WAIT_PROCESS_CPL;

	return WAIT_PROCESS_WAITING;
}

static int wait_until_doorbell_and_outbound_enabled(struct hinic3_hwif *hwif)
{
	return hinic3_wait_for_timeout(hwif, check_db_outbound_enable_handler,
		HINIC3_WAIT_DOORBELL_AND_OUTBOUND_TIMEOUT, USEC_PER_MSEC);
}

static void select_ppf_mpf(struct hinic3_hwdev *hwdev)
{
	struct hinic3_hwif *hwif = hwdev->hwif;

	if (!HINIC3_IS_VF(hwdev)) {
		set_ppf(hwif);

		if (HINIC3_IS_PPF(hwdev))
			set_mpf(hwif);

		get_mpf(hwif);
	}
}

/**
 * hinic3_init_hwif - initialize the hw interface
 * @hwif: the hardware interface of a pci function device
 * @pdev: the pci device that will be part of the hwif struct
 * Return: 0 - success, negative - failure
 **/
int hinic3_init_hwif(struct hinic3_hwdev *hwdev, void *cfg_reg_base,
		     void *intr_reg_base, void *mgmt_regs_base, u64 db_base_phy,
		     void *db_base, u64 db_dwqe_len)
{
	struct hinic3_hwif *hwif = NULL;
	u32 attr1, attr4, attr5;
	int err;

	err = init_hwif(hwdev, cfg_reg_base, intr_reg_base, mgmt_regs_base);
	if (err)
		return err;

	hwif = hwdev->hwif;

	err = init_db_area_idx(hwif, db_base_phy, db_base, db_dwqe_len);
	if (err) {
		sdk_err(hwdev->dev_hdl, "Failed to init db area.\n");
		goto init_db_area_err;
	}

	err = wait_hwif_ready(hwdev);
	if (err) {
		attr1 = hinic3_hwif_read_reg(hwif, HINIC3_CSR_FUNC_ATTR1_ADDR);
		sdk_err(hwdev->dev_hdl, "Chip status is not ready, attr1:0x%x\n", attr1);
		goto hwif_ready_err;
	}

	err = get_hwif_attr(hwif);
	if (err) {
		sdk_err(hwdev->dev_hdl, "Get hwif attr failed\n");
		goto hwif_ready_err;
	}

	err = wait_until_doorbell_and_outbound_enabled(hwif);
	if (err) {
		attr4 = hinic3_hwif_read_reg(hwif, HINIC3_CSR_FUNC_ATTR4_ADDR);
		attr5 = hinic3_hwif_read_reg(hwif, HINIC3_CSR_FUNC_ATTR5_ADDR);
		sdk_err(hwdev->dev_hdl, "Hw doorbell/outbound is disabled, attr4 0x%x attr5 0x%x\n",
			attr4, attr5);
		goto hwif_ready_err;
	}

	select_ppf_mpf(hwdev);

	disable_all_msix(hwdev);
	/* disable mgmt cpu report any event */
	hinic3_set_pf_status(hwdev->hwif, HINIC3_PF_STATUS_INIT);

	sdk_info(hwdev->dev_hdl, "global_func_idx: %u, func_type: %d, host_id: %u, ppf: %u, mpf: %u\n",
		 hwif->attr.func_global_idx, hwif->attr.func_type, hwif->attr.pci_intf_idx,
		 hwif->attr.ppf_idx, hwif->attr.mpf_idx);

	return 0;

hwif_ready_err:
	hinic3_show_chip_err_info(hwdev);
	free_db_area(&hwif->free_db_area);
init_db_area_err:
	kfree(hwif);

	return err;
}

/**
 * hinic3_free_hwif - free the hw interface
 * @hwif: the hardware interface of a pci function device
 * @pdev: the pci device that will be part of the hwif struct
 **/
void hinic3_free_hwif(struct hinic3_hwdev *hwdev)
{
	spin_lock_deinit(&hwdev->hwif->free_db_area.idx_lock);
	free_db_area(&hwdev->hwif->free_db_area);
	enable_all_msix(hwdev);
	kfree(hwdev->hwif);
	hwdev->hwif = NULL;
}

u16 hinic3_global_func_id(void *hwdev)
{
	struct hinic3_hwif *hwif = NULL;

	if (!hwdev)
		return 0;

	hwif = ((struct hinic3_hwdev *)hwdev)->hwif;

	return hwif->attr.func_global_idx;
}
EXPORT_SYMBOL(hinic3_global_func_id);

/**
 * get function id from register,used by sriov hot migration process
 * @hwdev: the pointer to hw device
 */
u16 hinic3_global_func_id_hw(void *hwdev)
{
	u32 addr, attr0;
	struct hinic3_hwdev *dev;

	dev = (struct hinic3_hwdev *)hwdev;
	addr = HINIC3_CSR_FUNC_ATTR0_ADDR;
	attr0 = hinic3_hwif_read_reg(dev->hwif, addr);

	return HINIC3_AF0_GET(attr0, FUNC_GLOBAL_IDX);
}

/**
 * get function id, used by sriov hot migratition process.
 * @hwdev: the pointer to hw device
 * @func_id: function id
 */
int hinic3_global_func_id_get(void *hwdev, u16 *func_id)
{
	struct hinic3_hwdev *dev = (struct hinic3_hwdev *)hwdev;

	if (!hwdev || !func_id)
		return -EINVAL;

	/* only vf get func_id from chip reg for sriov migrate */
	if (!HINIC3_IS_VF(dev)) {
		*func_id = hinic3_global_func_id(hwdev);
		return 0;
	}

	*func_id = hinic3_global_func_id_hw(dev);
	return 0;
}

u16 hinic3_intr_num(void *hwdev)
{
	struct hinic3_hwif *hwif = NULL;

	if (!hwdev)
		return 0;

	hwif = ((struct hinic3_hwdev *)hwdev)->hwif;

	return hwif->attr.num_irqs;
}
EXPORT_SYMBOL(hinic3_intr_num);

u8 hinic3_pf_id_of_vf(void *hwdev)
{
	struct hinic3_hwif *hwif = NULL;

	if (!hwdev)
		return 0;

	hwif = ((struct hinic3_hwdev *)hwdev)->hwif;

	return hwif->attr.port_to_port_idx;
}
EXPORT_SYMBOL(hinic3_pf_id_of_vf);

u8 hinic3_pcie_itf_id(void *hwdev)
{
	struct hinic3_hwif *hwif = NULL;

	if (!hwdev)
		return 0;

	hwif = ((struct hinic3_hwdev *)hwdev)->hwif;

	return hwif->attr.pci_intf_idx;
}
EXPORT_SYMBOL(hinic3_pcie_itf_id);

u8 hinic3_vf_in_pf(void *hwdev)
{
	struct hinic3_hwif *hwif = NULL;

	if (!hwdev)
		return 0;

	hwif = ((struct hinic3_hwdev *)hwdev)->hwif;

	return hwif->attr.vf_in_pf;
}
EXPORT_SYMBOL(hinic3_vf_in_pf);

enum func_type hinic3_func_type(void *hwdev)
{
	struct hinic3_hwif *hwif = NULL;

	if (!hwdev)
		return 0;

	hwif = ((struct hinic3_hwdev *)hwdev)->hwif;

	return hwif->attr.func_type;
}
EXPORT_SYMBOL(hinic3_func_type);

u8 hinic3_ceq_num(void *hwdev)
{
	struct hinic3_hwif *hwif = NULL;

	if (!hwdev)
		return 0;

	hwif = ((struct hinic3_hwdev *)hwdev)->hwif;

	return hwif->attr.num_ceqs;
}
EXPORT_SYMBOL(hinic3_ceq_num);

u16 hinic3_glb_pf_vf_offset(void *hwdev)
{
	struct hinic3_hwif *hwif = NULL;

	if (!hwdev)
		return 0;

	hwif = ((struct hinic3_hwdev *)hwdev)->hwif;

	return hwif->attr.global_vf_id_of_pf;
}
EXPORT_SYMBOL(hinic3_glb_pf_vf_offset);

u8 hinic3_ppf_idx(void *hwdev)
{
	struct hinic3_hwif *hwif = NULL;

	if (!hwdev)
		return 0;

	hwif = ((struct hinic3_hwdev *)hwdev)->hwif;

	return hwif->attr.ppf_idx;
}
EXPORT_SYMBOL(hinic3_ppf_idx);

u8 hinic3_host_ppf_idx(struct hinic3_hwdev *hwdev, u8 host_id)
{
	u32 ppf_elect_port_addr;
	u32 val;

	if (!hwdev)
		return 0;

	ppf_elect_port_addr = HINIC3_CSR_FUNC_PPF_ELECT(host_id);
	val = hinic3_hwif_read_reg(hwdev->hwif, ppf_elect_port_addr);

	return HINIC3_PPF_ELECT_PORT_GET(val, IDX);
}

u32 hinic3_get_self_test_result(void *hwdev)
{
	struct hinic3_hwif *hwif = ((struct hinic3_hwdev *)hwdev)->hwif;

	return hinic3_hwif_read_reg(hwif, HINIC3_MGMT_HEALTH_STATUS_ADDR);
}

void hinic3_show_chip_err_info(struct hinic3_hwdev *hwdev)
{
	struct hinic3_hwif *hwif = hwdev->hwif;
	u32 value;

	if (hinic3_func_type(hwdev) == TYPE_VF)
		return;

	value = hinic3_hwif_read_reg(hwif, HINIC3_CHIP_BASE_INFO_ADDR);
	sdk_warn(hwdev->dev_hdl, "Chip base info: 0x%08x\n", value);

	value = hinic3_hwif_read_reg(hwif, HINIC3_MGMT_HEALTH_STATUS_ADDR);
	sdk_warn(hwdev->dev_hdl, "Mgmt CPU health status: 0x%08x\n", value);

	value = hinic3_hwif_read_reg(hwif, HINIC3_CHIP_ERR_STATUS0_ADDR);
	sdk_warn(hwdev->dev_hdl, "Chip fatal error status0: 0x%08x\n", value);
	value = hinic3_hwif_read_reg(hwif, HINIC3_CHIP_ERR_STATUS1_ADDR);
	sdk_warn(hwdev->dev_hdl, "Chip fatal error status1: 0x%08x\n", value);

	value = hinic3_hwif_read_reg(hwif, HINIC3_ERR_INFO0_ADDR);
	sdk_warn(hwdev->dev_hdl, "Chip exception info0: 0x%08x\n", value);
	value = hinic3_hwif_read_reg(hwif, HINIC3_ERR_INFO1_ADDR);
	sdk_warn(hwdev->dev_hdl, "Chip exception info1: 0x%08x\n", value);
	value = hinic3_hwif_read_reg(hwif, HINIC3_ERR_INFO2_ADDR);
	sdk_warn(hwdev->dev_hdl, "Chip exception info2: 0x%08x\n", value);
}

