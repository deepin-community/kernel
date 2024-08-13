// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_hwdev.h"
#include "sss_csr.h"
#include "sss_hwif_api.h"
#include "sss_hw_svc_cap.h"

#define SSS_DEFAULT_RX_BUF_SIZE_LEVEL	((u16)0xB)

enum sss_rx_buf_size {
	SSS_RX_BUF_SIZE_32B = 0x20,
	SSS_RX_BUF_SIZE_64B = 0x40,
	SSS_RX_BUF_SIZE_96B = 0x60,
	SSS_RX_BUF_SIZE_128B = 0x80,
	SSS_RX_BUF_SIZE_192B = 0xC0,
	SSS_RX_BUF_SIZE_256B = 0x100,
	SSS_RX_BUF_SIZE_384B = 0x180,
	SSS_RX_BUF_SIZE_512B = 0x200,
	SSS_RX_BUF_SIZE_768B = 0x300,
	SSS_RX_BUF_SIZE_1K = 0x400,
	SSS_RX_BUF_SIZE_1_5K = 0x600,
	SSS_RX_BUF_SIZE_2K = 0x800,
	SSS_RX_BUF_SIZE_3K = 0xC00,
	SSS_RX_BUF_SIZE_4K = 0x1000,
	SSS_RX_BUF_SIZE_8K = 0x2000,
	SSS_RX_BUF_SIZE_16K = 0x4000,
};

const int sss_rx_buf_size_level[] = {
	SSS_RX_BUF_SIZE_32B,
	SSS_RX_BUF_SIZE_64B,
	SSS_RX_BUF_SIZE_96B,
	SSS_RX_BUF_SIZE_128B,
	SSS_RX_BUF_SIZE_192B,
	SSS_RX_BUF_SIZE_256B,
	SSS_RX_BUF_SIZE_384B,
	SSS_RX_BUF_SIZE_512B,
	SSS_RX_BUF_SIZE_768B,
	SSS_RX_BUF_SIZE_1K,
	SSS_RX_BUF_SIZE_1_5K,
	SSS_RX_BUF_SIZE_2K,
	SSS_RX_BUF_SIZE_3K,
	SSS_RX_BUF_SIZE_4K,
	SSS_RX_BUF_SIZE_8K,
	SSS_RX_BUF_SIZE_16K,
};

static u16 sss_get_rx_buf_size_level(int buf_size)
{
	u16 i;
	u16 cnt = ARRAY_LEN(sss_rx_buf_size_level);

	for (i = 0; i < cnt; i++) {
		if (sss_rx_buf_size_level[i] == buf_size)
			return i;
	}

	return SSS_DEFAULT_RX_BUF_SIZE_LEVEL; /* default 2K */
}

static int sss_chip_get_interrupt_cfg(void *hwdev,
				      struct sss_irq_cfg *intr_cfg, u16 channel)
{
	int ret;
	struct sss_cmd_msix_config cmd_msix = {0};
	u16 out_len = sizeof(cmd_msix);

	cmd_msix.opcode = SSS_MGMT_MSG_GET_CMD;
	cmd_msix.func_id = sss_get_global_func_id(hwdev);
	cmd_msix.msix_index = intr_cfg->msix_id;

	ret = sss_sync_send_msg_ch(hwdev, SSS_COMM_MGMT_CMD_CFG_MSIX_CTRL_REG,
				   &cmd_msix, sizeof(cmd_msix), &cmd_msix, &out_len, channel);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_msix)) {
		sdk_err(SSS_TO_DEV(hwdev),
			"Fail to get intr config, ret: %d, status: 0x%x, out_len: 0x%x, channel: 0x%x\n",
			ret, cmd_msix.head.state, out_len, channel);
		return -EINVAL;
	}

	intr_cfg->lli_credit = cmd_msix.lli_credit_cnt;
	intr_cfg->lli_timer = cmd_msix.lli_timer_cnt;
	intr_cfg->pending = cmd_msix.pending_cnt;
	intr_cfg->coalesc_timer = cmd_msix.coalesce_timer_cnt;
	intr_cfg->resend_timer = cmd_msix.resend_timer_cnt;

	return 0;
}

int sss_chip_set_msix_attr(void *hwdev,
			   struct sss_irq_cfg intr_cfg, u16 channel)
{
	int ret;
	struct sss_irq_cfg temp_cfg = {0};

	if (!hwdev)
		return -EINVAL;

	temp_cfg.msix_id = intr_cfg.msix_id;

	ret = sss_chip_get_interrupt_cfg(hwdev, &temp_cfg, channel);
	if (ret != 0)
		return -EINVAL;

	if (intr_cfg.lli_set == 0) {
		intr_cfg.lli_credit = temp_cfg.lli_credit;
		intr_cfg.lli_timer = temp_cfg.lli_timer;
	}

	if (intr_cfg.coalesc_intr_set == 0) {
		intr_cfg.pending = temp_cfg.pending;
		intr_cfg.coalesc_timer = temp_cfg.coalesc_timer;
		intr_cfg.resend_timer = temp_cfg.resend_timer;
	}

	return sss_chip_set_eq_msix_attr(hwdev, &intr_cfg, channel);
}
EXPORT_SYMBOL(sss_chip_set_msix_attr);

void sss_chip_clear_msix_resend_bit(void *hwdev, u16 msix_id, bool clear_en)
{
	u32 val;

	if (!hwdev)
		return;

	val = SSS_SET_MSI_CLR_INDIR(msix_id, SIMPLE_INDIR_ID) |
	      SSS_SET_MSI_CLR_INDIR(!!clear_en, RESEND_TIMER_CLR);

	sss_chip_write_reg(SSS_TO_HWIF(hwdev), SSS_CSR_FUNC_MSI_CLR_WR_ADDR, val);
}
EXPORT_SYMBOL(sss_chip_clear_msix_resend_bit);

int sss_chip_reset_function(void *hwdev, u16 func_id, u64 flag, u16 channel)
{
	int ret = 0;
	struct sss_cmd_func_reset cmd_reset = {0};
	u16 out_len = sizeof(cmd_reset);

	if (!hwdev)
		return -EINVAL;

	cmd_reset.func_id = func_id;
	cmd_reset.reset_flag = flag;
	sdk_info(SSS_TO_DEV(hwdev), "Func reset, flag: 0x%llx, channel:0x%x\n", flag, channel);

	ret = sss_sync_send_msg_ch(hwdev, SSS_COMM_MGMT_CMD_FUNC_RESET,
				   &cmd_reset, sizeof(cmd_reset), &cmd_reset, &out_len, channel);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_reset)) {
		sdk_err(SSS_TO_DEV(hwdev),
			"Fail to reset func, flag 0x%llx, ret: %d, status: 0x%x, out_len: 0x%x\n",
			flag, ret, cmd_reset.head.state, out_len);
		return -EIO;
	}

	return 0;
}
EXPORT_SYMBOL(sss_chip_reset_function);

int sss_chip_set_root_ctx(void *hwdev,
			  u32 rq_depth, u32 sq_depth, int rx_size, u16 channel)
{
	int ret;
	struct sss_cmd_root_ctxt cmd_root = {0};
	u16 out_len = sizeof(cmd_root);

	if (!hwdev)
		return -EINVAL;

	cmd_root.func_id = sss_get_global_func_id(hwdev);
	if (rq_depth != 0 || sq_depth != 0 || rx_size != 0) {
		cmd_root.rx_buf_sz = sss_get_rx_buf_size_level(rx_size);
		cmd_root.rq_depth = (u16)ilog2(rq_depth);
		cmd_root.sq_depth = (u16)ilog2(sq_depth);
		cmd_root.lro_en = 1;
	}

	ret = sss_sync_send_msg_ch(hwdev, SSS_COMM_MGMT_CMD_SET_VAT,
				   &cmd_root, sizeof(cmd_root), &cmd_root, &out_len, channel);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_root)) {
		sdk_err(SSS_TO_DEV(hwdev),
			"Fail to set root ctx, ret: %d, status: 0x%x, out_len: 0x%x, channel: 0x%x\n",
			ret, cmd_root.head.state, out_len, channel);
		return -EFAULT;
	}

	return 0;
}
EXPORT_SYMBOL(sss_chip_set_root_ctx);

int sss_chip_clean_root_ctx(void *hwdev, u16 channel)
{
	return sss_chip_set_root_ctx(hwdev, 0, 0, 0, channel);
}
EXPORT_SYMBOL(sss_chip_clean_root_ctx);

static int sss_get_fw_ver(struct sss_hwdev *hwdev,
			  enum sss_fw_ver_type fw_type, u8 *buf, u8 buf_size, u16 channel)
{
	int ret;
	struct sss_cmd_get_fw_version cmd_version = {0};
	u16 out_len = sizeof(cmd_version);

	if (!hwdev || !buf)
		return -EINVAL;

	cmd_version.fw_type = fw_type;
	ret = sss_sync_send_msg_ch(hwdev, SSS_COMM_MGMT_CMD_GET_FW_VERSION,
				   &cmd_version, sizeof(cmd_version), &cmd_version,
				   &out_len, channel);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_version)) {
		sdk_err(hwdev->dev_hdl,
			"Fail to get fw version, ret: %d, status: 0x%x, out_len: 0x%x, channel: 0x%x\n",
			ret, cmd_version.head.state, out_len, channel);
		return -EIO;
	}

	ret = snprintf(buf, buf_size, "%s", cmd_version.ver);
	if (ret < 0)
		return -EINVAL;

	return 0;
}

int sss_get_mgmt_version(void *hwdev, u8 *buf, u8 buf_size, u16 channel)
{
	return sss_get_fw_ver(hwdev, SSS_FW_VER_TYPE_MPU, buf,
			      buf_size, channel);
}
EXPORT_SYMBOL(sss_get_mgmt_version);

int sss_chip_set_func_used_state(void *hwdev,
				 u16 service_type, bool state, u16 channel)
{
	int ret;
	struct sss_cmd_func_svc_used_state cmd_state = {0};
	u16 out_len = sizeof(cmd_state);

	if (!hwdev)
		return -EINVAL;

	cmd_state.func_id = sss_get_global_func_id(hwdev);
	cmd_state.svc_type = service_type;
	cmd_state.used_state = !!state;

	ret = sss_sync_send_msg_ch(hwdev,
				   SSS_COMM_MGMT_CMD_SET_FUNC_SVC_USED_STATE,
				   &cmd_state, sizeof(cmd_state), &cmd_state, &out_len, channel);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_state)) {
		sdk_err(SSS_TO_DEV(hwdev),
			"Fail to set func used state, ret: %d, status: 0x%x, out_len: 0x%x, channel: 0x%x\n\n",
			ret, cmd_state.head.state, out_len, channel);
		return -EIO;
	}

	return 0;
}
EXPORT_SYMBOL(sss_chip_set_func_used_state);

bool sss_get_nic_capability(void *hwdev, struct sss_nic_service_cap *capability)
{
	struct sss_hwdev *dev = hwdev;

	if (!capability || !hwdev)
		return false;

	if (SSS_IS_NIC_TYPE(dev)) {
		memcpy(capability, SSS_TO_NIC_CAP(hwdev), sizeof(*capability));
		return true;
	} else {
		return false;
	}
}
EXPORT_SYMBOL(sss_get_nic_capability);

bool sss_support_nic(void *hwdev)
{
	return (hwdev && SSS_IS_NIC_TYPE((struct sss_hwdev *)hwdev));
}
EXPORT_SYMBOL(sss_support_nic);

bool sss_support_ppa(void *hwdev, struct sss_ppa_service_cap *cap)
{
	struct sss_hwdev *dev = hwdev;

	if (!hwdev)
		return false;

	if (!SSS_IS_PPA_TYPE(dev))
		return false;

	if (cap)
		memcpy(cap, &dev->mgmt_info->svc_cap.ppa_cap, sizeof(*cap));

	return true;
}
EXPORT_SYMBOL(sss_support_ppa);

u16 sss_get_max_sq_num(void *hwdev)
{
	if (!hwdev) {
		pr_err("Get max sq num: hwdev is NULL\n");
		return 0;
	}

	return SSS_TO_MAX_SQ_NUM(hwdev);
}
EXPORT_SYMBOL(sss_get_max_sq_num);

u8 sss_get_phy_port_id(void *hwdev)
{
	if (!hwdev) {
		pr_err("Get phy port id: hwdev is NULL\n");
		return 0;
	}

	return SSS_TO_PHY_PORT_ID(hwdev);
}
EXPORT_SYMBOL(sss_get_phy_port_id);

u16 sss_get_max_vf_num(void *hwdev)
{
	if (!hwdev) {
		pr_err("Get max vf num: hwdev is NULL\n");
		return 0;
	}

	return SSS_TO_MAX_VF_NUM(hwdev);
}
EXPORT_SYMBOL(sss_get_max_vf_num);

u16 sss_nic_intr_num(void *hwdev)
{
	struct sss_hwif *hwif = NULL;

	if (!hwdev)
		return 0;

	hwif = ((struct sss_hwdev *)hwdev)->hwif;

	return hwif->attr.irq_num;
}
EXPORT_SYMBOL(sss_nic_intr_num);

int sss_get_cos_valid_bitmap(void *hwdev, u8 *func_cos_bitmap, u8 *port_cos_bitmap)
{
	if (!hwdev) {
		pr_err("Get cos valid bitmap: hwdev is NULL\n");
		return -EINVAL;
	}

	*func_cos_bitmap = SSS_TO_FUNC_COS_BITMAP(hwdev);
	*port_cos_bitmap = SSS_TO_PORT_COS_BITMAP(hwdev);

	return 0;
}
EXPORT_SYMBOL(sss_get_cos_valid_bitmap);

u16 sss_alloc_irq(void *hwdev, enum sss_service_type service_type,
		  struct sss_irq_desc *alloc_array, u16 alloc_num)
{
	int i;
	int j;
	u16 need_num = alloc_num;
	u16 act_num = 0;
	struct sss_irq_info *irq_info = NULL;
	struct sss_irq *irq = NULL;

	if (!hwdev || !alloc_array)
		return 0;

	irq_info = SSS_TO_IRQ_INFO(hwdev);
	irq = irq_info->irq;

	mutex_lock(&irq_info->irq_mutex);
	if (irq_info->free_num == 0) {
		sdk_err(SSS_TO_DEV(hwdev), "Fail to alloc irq, free_num is zero\n");
		mutex_unlock(&irq_info->irq_mutex);
		return 0;
	}

	if (alloc_num > irq_info->free_num) {
		sdk_warn(SSS_TO_DEV(hwdev), "Adjust need_num to %u\n", irq_info->free_num);
		need_num = irq_info->free_num;
	}

	for (i = 0; i < need_num; i++) {
		for (j = 0; j < irq_info->total_num; j++) {
			if (irq[j].busy != SSS_CFG_FREE)
				continue;

			if (irq_info->free_num == 0) {
				sdk_err(SSS_TO_DEV(hwdev), "Fail to alloc irq, free_num is zero\n");
				mutex_unlock(&irq_info->irq_mutex);
				memset(alloc_array, 0, sizeof(*alloc_array) * alloc_num);
				return 0;
			}

			irq[j].type = service_type;
			irq[j].busy = SSS_CFG_BUSY;

			alloc_array[i].irq_id = irq[j].desc.irq_id;
			alloc_array[i].msix_id = irq[j].desc.msix_id;
			irq_info->free_num--;
			act_num++;

			break;
		}
	}

	mutex_unlock(&irq_info->irq_mutex);
	return act_num;
}
EXPORT_SYMBOL(sss_alloc_irq);

void sss_free_irq(void *hwdev, enum sss_service_type service_type, u32 irq_id)
{
	int i;
	struct sss_irq_info *irq_info = NULL;
	struct sss_irq *irq = NULL;

	if (!hwdev)
		return;

	irq_info = SSS_TO_IRQ_INFO(hwdev);
	irq = irq_info->irq;

	mutex_lock(&irq_info->irq_mutex);

	for (i = 0; i < irq_info->total_num; i++) {
		if (irq_id != irq[i].desc.irq_id ||
		    service_type != irq[i].type)
			continue;

		if (irq[i].busy == SSS_CFG_FREE)
			continue;

		irq[i].busy = SSS_CFG_FREE;
		irq_info->free_num++;
		if (irq_info->free_num > irq_info->total_num) {
			sdk_err(SSS_TO_DEV(hwdev), "Free_num out of range :[0, %u]\n",
				irq_info->total_num);
			mutex_unlock(&irq_info->irq_mutex);
			return;
		}
		break;
	}

	if (i >= irq_info->total_num)
		sdk_warn(SSS_TO_DEV(hwdev), "Irq %u don`t need to free\n", irq_id);

	mutex_unlock(&irq_info->irq_mutex);
}
EXPORT_SYMBOL(sss_free_irq);

void sss_register_dev_event(void *hwdev, void *data, sss_event_handler_t callback)
{
	struct sss_hwdev *dev = hwdev;

	if (!hwdev) {
		pr_err("Register event: hwdev is NULL\n");
		return;
	}

	dev->event_handler = callback;
	dev->event_handler_data = data;
}
EXPORT_SYMBOL(sss_register_dev_event);

void sss_unregister_dev_event(void *hwdev)
{
	struct sss_hwdev *dev = hwdev;

	if (!hwdev) {
		pr_err("Unregister event: hwdev is NULL\n");
		return;
	}

	dev->event_handler = NULL;
	dev->event_handler_data = NULL;
}
EXPORT_SYMBOL(sss_unregister_dev_event);

int sss_get_dev_present_flag(const void *hwdev)
{
	return hwdev && !!((struct sss_hwdev *)hwdev)->chip_present_flag;
}
EXPORT_SYMBOL(sss_get_dev_present_flag);

u8 sss_get_max_pf_num(void *hwdev)
{
	if (!hwdev)
		return 0;

	return SSS_MAX_PF_NUM((struct sss_hwdev *)hwdev);
}
EXPORT_SYMBOL(sss_get_max_pf_num);

int sss_get_chip_present_state(void *hwdev, bool *present_state)
{
	if (!hwdev || !present_state)
		return -EINVAL;

	*present_state = sss_chip_get_present_state(hwdev);

	return 0;
}
EXPORT_SYMBOL(sss_get_chip_present_state);

void sss_fault_event_report(void *hwdev, u16 src, u16 level)
{
	if (!hwdev)
		return;

	sdk_info(SSS_TO_DEV(hwdev),
		 "Fault event report, src: %u, level: %u\n", src, level);
}
EXPORT_SYMBOL(sss_fault_event_report);

int sss_register_service_adapter(void *hwdev, enum sss_service_type service_type,
				 void *service_adapter)
{
	struct sss_hwdev *dev = hwdev;

	if (!hwdev || !service_adapter || service_type >= SSS_SERVICE_TYPE_MAX)
		return -EINVAL;

	if (dev->service_adapter[service_type])
		return -EINVAL;

	dev->service_adapter[service_type] = service_adapter;

	return 0;
}
EXPORT_SYMBOL(sss_register_service_adapter);

void sss_unregister_service_adapter(void *hwdev,
				    enum sss_service_type service_type)
{
	struct sss_hwdev *dev = hwdev;

	if (!hwdev || service_type >= SSS_SERVICE_TYPE_MAX)
		return;

	dev->service_adapter[service_type] = NULL;
}
EXPORT_SYMBOL(sss_unregister_service_adapter);

void *sss_get_service_adapter(void *hwdev, enum sss_service_type service_type)
{
	struct sss_hwdev *dev = hwdev;

	if (!hwdev || service_type >= SSS_SERVICE_TYPE_MAX)
		return NULL;

	return dev->service_adapter[service_type];
}
EXPORT_SYMBOL(sss_get_service_adapter);

void sss_do_event_callback(void *hwdev, struct sss_event_info *event)
{
	struct sss_hwdev *dev = hwdev;

	if (!hwdev) {
		pr_err("Event callback: hwdev is NULL\n");
		return;
	}

	if (!dev->event_handler) {
		sdk_info(dev->dev_hdl, "Event callback: handler is NULL\n");
		return;
	}

	dev->event_handler(dev->event_handler_data, event);
}
EXPORT_SYMBOL(sss_do_event_callback);

void sss_update_link_stats(void *hwdev, bool link_state)
{
	struct sss_hwdev *dev = hwdev;

	if (!hwdev)
		return;

	if (link_state)
		atomic_inc(&dev->hw_stats.link_event_stats.link_up_stats);
	else
		atomic_inc(&dev->hw_stats.link_event_stats.link_down_stats);
}
EXPORT_SYMBOL(sss_update_link_stats);
