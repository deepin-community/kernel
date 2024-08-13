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
#include "sss_hwdev.h"
#include "sss_hwif_api.h"
#include "sss_hwif_export.h"

#define SSS_GET_REG_FLAG(reg)		((reg) & (~(SSS_CSR_FLAG_MASK)))
#define SSS_GET_REG_ADDR(reg)		((reg) & (SSS_CSR_FLAG_MASK))

#define SSS_PAGE_SIZE_HW(pg_size)	((u8)ilog2((u32)((pg_size) >> 12)))

#define SSS_CLEAR_SLAVE_HOST_STATUS(host_id, val)	((val) & (~(1U << (host_id))))
#define SSS_SET_SLAVE_HOST_STATUS(host_id, enable)	(((u8)(enable) & 1U) << (host_id))

#define SSS_MULT_HOST_SLAVE_STATUS_ADDR				(SSS_MGMT_FLAG + 0xDF30)

u32 sss_chip_read_reg(struct sss_hwif *hwif, u32 reg)
{
	if (SSS_GET_REG_FLAG(reg) == SSS_MGMT_FLAG)
		return be32_to_cpu(readl(hwif->mgmt_reg_base +
					 SSS_GET_REG_ADDR(reg)));
	else
		return be32_to_cpu(readl(hwif->cfg_reg_base +
					 SSS_GET_REG_ADDR(reg)));
}

void sss_chip_write_reg(struct sss_hwif *hwif, u32 reg, u32 val)
{
	if (SSS_GET_REG_FLAG(reg) == SSS_MGMT_FLAG)
		writel(cpu_to_be32(val),
		       hwif->mgmt_reg_base + SSS_GET_REG_ADDR(reg));
	else
		writel(cpu_to_be32(val),
		       hwif->cfg_reg_base + SSS_GET_REG_ADDR(reg));
}

bool sss_chip_get_present_state(void *hwdev)
{
	u32 val;

	val = sss_chip_read_reg(SSS_TO_HWIF(hwdev), SSS_CSR_HW_ATTR1_ADDR);
	if (val == SSS_PCIE_LINK_DOWN) {
		sdk_warn(SSS_TO_DEV(hwdev), "Card is not present\n");
		return false;
	}

	return true;
}

u32 sss_chip_get_pcie_link_status(void *hwdev)
{
	u32 val;

	if (!hwdev)
		return SSS_PCIE_LINK_DOWN;

	val = sss_chip_read_reg(SSS_TO_HWIF(hwdev), SSS_CSR_HW_ATTR1_ADDR);
	if (val == SSS_PCIE_LINK_DOWN)
		return val;

	return !SSS_GET_AF1(val, MGMT_INIT_STATUS);
}

void sss_chip_set_pf_status(struct sss_hwif *hwif,
			    enum sss_pf_status status)
{
	u32 val;

	if (SSS_GET_HWIF_FUNC_TYPE(hwif) == SSS_FUNC_TYPE_VF)
		return;

	val = sss_chip_read_reg(hwif, SSS_CSR_HW_ATTR6_ADDR);
	val = SSS_CLEAR_AF6(val, PF_STATUS);
	val |= SSS_SET_AF6(status, PF_STATUS);

	sss_chip_write_reg(hwif, SSS_CSR_HW_ATTR6_ADDR, val);
}

enum sss_pf_status sss_chip_get_pf_status(struct sss_hwif *hwif)
{
	u32 val = sss_chip_read_reg(hwif, SSS_CSR_HW_ATTR6_ADDR);

	return SSS_GET_AF6(val, PF_STATUS);
}

void sss_chip_enable_doorbell(struct sss_hwif *hwif)
{
	u32 addr;
	u32 val;

	addr = SSS_CSR_HW_ATTR4_ADDR;
	val = sss_chip_read_reg(hwif, addr);

	val = SSS_CLEAR_AF4(val, DOORBELL_CTRL);
	val |= SSS_SET_AF4(DB_ENABLE, DOORBELL_CTRL);

	sss_chip_write_reg(hwif, addr, val);
}

void sss_chip_disable_doorbell(struct sss_hwif *hwif)
{
	u32 addr;
	u32 val;

	addr = SSS_CSR_HW_ATTR4_ADDR;
	val = sss_chip_read_reg(hwif, addr);

	val = SSS_CLEAR_AF4(val, DOORBELL_CTRL);
	val |= SSS_SET_AF4(DB_DISABLE, DOORBELL_CTRL);

	sss_chip_write_reg(hwif, addr, val);
}

void sss_free_db_id(struct sss_hwif *hwif, u32 id)
{
	struct sss_db_pool *pool = &hwif->db_pool;

	if (id >= pool->bit_size)
		return;

	spin_lock(&pool->id_lock);
	clear_bit((int)id, pool->bitmap);
	spin_unlock(&pool->id_lock);
}

int sss_alloc_db_id(struct sss_hwif *hwif, u32 *id)
{
	struct sss_db_pool *pool = &hwif->db_pool;
	u32 pg_id;

	spin_lock(&pool->id_lock);
	pg_id = (u32)find_first_zero_bit(pool->bitmap, pool->bit_size);
	if (pg_id == pool->bit_size) {
		spin_unlock(&pool->id_lock);
		return -ENOMEM;
	}
	set_bit(pg_id, pool->bitmap);
	spin_unlock(&pool->id_lock);

	*id = pg_id;

	return 0;
}

void sss_dump_chip_err_info(struct sss_hwdev *hwdev)
{
	u32 value;

	if (sss_get_func_type(hwdev) == SSS_FUNC_TYPE_VF)
		return;

	value = sss_chip_read_reg(hwdev->hwif, SSS_CHIP_BASE_INFO_ADDR);
	sdk_warn(hwdev->dev_hdl, "Chip base info: 0x%08x\n", value);

	value = sss_chip_read_reg(hwdev->hwif, SSS_MGMT_HEALTH_STATUS_ADDR);
	sdk_warn(hwdev->dev_hdl, "Mgmt CPU health status: 0x%08x\n", value);

	value = sss_chip_read_reg(hwdev->hwif, SSS_CHIP_ERR_STATUS0_ADDR);
	sdk_warn(hwdev->dev_hdl, "Chip fatal error status0: 0x%08x\n", value);
	value = sss_chip_read_reg(hwdev->hwif, SSS_CHIP_ERR_STATUS1_ADDR);
	sdk_warn(hwdev->dev_hdl, "Chip fatal error status1: 0x%08x\n", value);

	value = sss_chip_read_reg(hwdev->hwif, SSS_ERR_INFO0_ADDR);
	sdk_warn(hwdev->dev_hdl, "Chip exception info0: 0x%08x\n", value);
	value = sss_chip_read_reg(hwdev->hwif, SSS_ERR_INFO1_ADDR);
	sdk_warn(hwdev->dev_hdl, "Chip exception info1: 0x%08x\n", value);
	value = sss_chip_read_reg(hwdev->hwif, SSS_ERR_INFO2_ADDR);
	sdk_warn(hwdev->dev_hdl, "Chip exception info2: 0x%08x\n", value);
}

u8 sss_chip_get_host_ppf_id(struct sss_hwdev *hwdev, u8 host_id)
{
	u32 addr;
	u32 val;

	if (!hwdev)
		return 0;

	addr = SSS_CSR_FUNC_PPF_ELECT(host_id);
	val = sss_chip_read_reg(hwdev->hwif, addr);

	return SSS_GET_PPF_ELECT_PORT(val, ID);
}

static void sss_init_eq_msix_cfg(void *hwdev,
				 struct sss_cmd_msix_config *cmd_msix,
				 struct sss_irq_cfg *info)
{
	cmd_msix->opcode = SSS_MGMT_MSG_SET_CMD;
	cmd_msix->func_id = sss_get_global_func_id(hwdev);
	cmd_msix->msix_index = (u16)info->msix_id;
	cmd_msix->lli_credit_cnt = info->lli_credit;
	cmd_msix->lli_timer_cnt = info->lli_timer;
	cmd_msix->pending_cnt = info->pending;
	cmd_msix->coalesce_timer_cnt = info->coalesc_timer;
	cmd_msix->resend_timer_cnt = info->resend_timer;
}

int sss_chip_set_eq_msix_attr(void *hwdev,
			      struct sss_irq_cfg *intr_info, u16 ch)
{
	int ret;
	struct sss_cmd_msix_config cmd_msix = {0};
	u16 out_len = sizeof(cmd_msix);

	sss_init_eq_msix_cfg(hwdev, &cmd_msix, intr_info);

	ret = sss_sync_send_msg_ch(hwdev, SSS_COMM_MGMT_CMD_CFG_MSIX_CTRL_REG,
				   &cmd_msix, sizeof(cmd_msix), &cmd_msix, &out_len, ch);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_msix)) {
		sdk_err(SSS_TO_DEV(hwdev),
			"Fail to set eq msix cfg, ret: %d, status: 0x%x, out_len: 0x%x, ch: 0x%x\n",
			ret, cmd_msix.head.state, out_len, ch);
		return -EINVAL;
	}

	return 0;
}

int sss_chip_set_wq_page_size(void *hwdev, u16 func_id, u32 page_size)
{
	int ret;
	struct sss_cmd_wq_page_size cmd_page = {0};
	u16 out_len = sizeof(cmd_page);

	cmd_page.opcode = SSS_MGMT_MSG_SET_CMD;
	cmd_page.func_id = func_id;
	cmd_page.page_size = SSS_PAGE_SIZE_HW(page_size);

	ret = sss_sync_send_msg(hwdev, SSS_COMM_MGMT_CMD_CFG_PAGESIZE,
				&cmd_page, sizeof(cmd_page), &cmd_page, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_page)) {
		sdk_err(SSS_TO_DEV(hwdev),
			"Fail to set wq page size, ret: %d, status: 0x%x, out_len: 0x%0x\n",
			ret, cmd_page.head.state, out_len);
		return -EFAULT;
	}

	return 0;
}

int sss_chip_set_ceq_attr(struct sss_hwdev *hwdev, u16 qid,
			  u32 attr0, u32 attr1)
{
	int ret;
	struct sss_cmd_ceq_ctrl_reg cmd_ceq = {0};
	u16 out_len = sizeof(cmd_ceq);

	cmd_ceq.func_id = sss_get_global_func_id(hwdev);
	cmd_ceq.qid = qid;
	cmd_ceq.ctrl0 = attr0;
	cmd_ceq.ctrl1 = attr1;

	ret = sss_sync_send_msg(hwdev, SSS_COMM_MGMT_CMD_SET_CEQ_CTRL_REG,
				&cmd_ceq, sizeof(cmd_ceq), &cmd_ceq, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_ceq)) {
		sdk_err(hwdev->dev_hdl,
			"Fail to set ceq %u ctrl, ret: %d status: 0x%x, out_len: 0x%x\n",
			qid, ret, cmd_ceq.head.state, out_len);
		return -EFAULT;
	}

	return 0;
}

void sss_chip_set_slave_host_status(void *dev, u8 host_id, bool enable)
{
	u32 val;
	struct sss_hwdev *hwdev = dev;

	if (SSS_GET_FUNC_TYPE(hwdev) != SSS_FUNC_TYPE_PPF)
		return;

	val = sss_chip_read_reg(hwdev->hwif, SSS_MULT_HOST_SLAVE_STATUS_ADDR);
	val = SSS_CLEAR_SLAVE_HOST_STATUS(host_id, val);
	val |= SSS_SET_SLAVE_HOST_STATUS(host_id, !!enable);

	sss_chip_write_reg(hwdev->hwif, SSS_MULT_HOST_SLAVE_STATUS_ADDR, val);

	sdk_info(hwdev->dev_hdl, "Set slave host %d status %d, reg value: 0x%x\n",
		 host_id, enable, val);
}
