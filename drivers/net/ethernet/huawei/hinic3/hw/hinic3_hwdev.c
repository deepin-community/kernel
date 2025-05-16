// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [COMM]" fmt

#include <linux/time.h>
#include <linux/timex.h>
#include <linux/rtc.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/completion.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>

#include "ossl_knl.h"
#include "hinic3_mt.h"
#include "hinic3_crm.h"
#include "hinic3_hw.h"
#include "hinic3_common.h"
#include "hinic3_csr.h"
#include "hinic3_hwif.h"
#include "hinic3_eqs.h"
#include "hinic3_api_cmd.h"
#include "hinic3_mgmt.h"
#include "hinic3_mbox.h"
#include "hinic3_cmdq.h"
#include "hinic3_hw_cfg.h"
#include "hinic3_multi_host_mgmt.h"
#include "hinic3_hw_comm.h"
#include "hinic3_cqm.h"
#include "hinic3_prof_adap.h"
#include "hinic3_devlink.h"
#include "hinic3_hwdev.h"

static unsigned int wq_page_order = HINIC3_MAX_WQ_PAGE_SIZE_ORDER;
module_param(wq_page_order, uint, 0444);
MODULE_PARM_DESC(wq_page_order, "Set wq page size order, wq page size is 4K * (2 ^ wq_page_order) - default is 8");

enum hinic3_pcie_nosnoop {
	HINIC3_PCIE_SNOOP = 0,
	HINIC3_PCIE_NO_SNOOP = 1,
};

enum hinic3_pcie_tph {
	HINIC3_PCIE_TPH_DISABLE = 0,
	HINIC3_PCIE_TPH_ENABLE = 1,
};

#define HINIC3_DMA_ATTR_INDIR_IDX_SHIFT				0

#define HINIC3_DMA_ATTR_INDIR_IDX_MASK				0x3FF

#define HINIC3_DMA_ATTR_INDIR_IDX_SET(val, member)			\
		(((u32)(val) & HINIC3_DMA_ATTR_INDIR_##member##_MASK) << \
			HINIC3_DMA_ATTR_INDIR_##member##_SHIFT)

#define HINIC3_DMA_ATTR_INDIR_IDX_CLEAR(val, member)		\
		((val) & (~(HINIC3_DMA_ATTR_INDIR_##member##_MASK	\
			<< HINIC3_DMA_ATTR_INDIR_##member##_SHIFT)))

#define HINIC3_DMA_ATTR_ENTRY_ST_SHIFT				0
#define HINIC3_DMA_ATTR_ENTRY_AT_SHIFT				8
#define HINIC3_DMA_ATTR_ENTRY_PH_SHIFT				10
#define HINIC3_DMA_ATTR_ENTRY_NO_SNOOPING_SHIFT			12
#define HINIC3_DMA_ATTR_ENTRY_TPH_EN_SHIFT			13

#define HINIC3_DMA_ATTR_ENTRY_ST_MASK				0xFF
#define HINIC3_DMA_ATTR_ENTRY_AT_MASK				0x3
#define HINIC3_DMA_ATTR_ENTRY_PH_MASK				0x3
#define HINIC3_DMA_ATTR_ENTRY_NO_SNOOPING_MASK			0x1
#define HINIC3_DMA_ATTR_ENTRY_TPH_EN_MASK			0x1

#define HINIC3_DMA_ATTR_ENTRY_SET(val, member)			\
		(((u32)(val) & HINIC3_DMA_ATTR_ENTRY_##member##_MASK) << \
			HINIC3_DMA_ATTR_ENTRY_##member##_SHIFT)

#define HINIC3_DMA_ATTR_ENTRY_CLEAR(val, member)		\
		((val) & (~(HINIC3_DMA_ATTR_ENTRY_##member##_MASK	\
			<< HINIC3_DMA_ATTR_ENTRY_##member##_SHIFT)))

#define HINIC3_PCIE_ST_DISABLE			0
#define HINIC3_PCIE_AT_DISABLE			0
#define HINIC3_PCIE_PH_DISABLE			0

#define PCIE_MSIX_ATTR_ENTRY			0

#define HINIC3_CHIP_PRESENT			1
#define HINIC3_CHIP_ABSENT			0

#define HINIC3_DEAULT_EQ_MSIX_PENDING_LIMIT	0
#define HINIC3_DEAULT_EQ_MSIX_COALESC_TIMER_CFG	0xFF
#define HINIC3_DEAULT_EQ_MSIX_RESEND_TIMER_CFG	7

#define HINIC3_HWDEV_WQ_NAME			"hinic3_hardware"
#define HINIC3_WQ_MAX_REQ			10

#define SLAVE_HOST_STATUS_CLEAR(host_id, val)	((val) & (~(1U << (host_id))))
#define SLAVE_HOST_STATUS_SET(host_id, enable)	(((u8)(enable) & 1U) << (host_id))
#define SLAVE_HOST_STATUS_GET(host_id, val)	(!!((val) & (1U << (host_id))))

#ifdef HAVE_HOT_REPLACE_FUNC
	extern int get_partition_id(void);
#else
	static int get_partition_id(void) {return 0; }
#endif

void hinic3_set_slave_host_enable(void *hwdev, u8 host_id, bool enable)
{
	u32 reg_val;
	struct hinic3_hwdev *dev = (struct hinic3_hwdev *)hwdev;

	if (HINIC3_FUNC_TYPE(dev) != TYPE_PPF)
		return;

	reg_val = hinic3_hwif_read_reg(dev->hwif, HINIC3_MULT_HOST_SLAVE_STATUS_ADDR);

	reg_val = SLAVE_HOST_STATUS_CLEAR(host_id, reg_val);
	reg_val |= SLAVE_HOST_STATUS_SET(host_id, enable);
	hinic3_hwif_write_reg(dev->hwif, HINIC3_MULT_HOST_SLAVE_STATUS_ADDR, reg_val);

	sdk_info(dev->dev_hdl, "Set slave host %d status %d, reg value: 0x%x\n",
		 host_id, enable, reg_val);
}

int hinic3_get_slave_host_enable(void *hwdev, u8 host_id, u8 *slave_en)
{
	struct hinic3_hwdev *dev = hwdev;

	u32 reg_val;

	if (HINIC3_FUNC_TYPE(dev) != TYPE_PPF) {
		sdk_warn(dev->dev_hdl, "hwdev should be ppf\n");
		return -EINVAL;
	}

	reg_val = hinic3_hwif_read_reg(dev->hwif, HINIC3_MULT_HOST_SLAVE_STATUS_ADDR);
	*slave_en = SLAVE_HOST_STATUS_GET(host_id, reg_val);

	return 0;
}
EXPORT_SYMBOL(hinic3_get_slave_host_enable);

int hinic3_get_slave_bitmap(void *hwdev, u8 *slave_host_bitmap)
{
	struct hinic3_hwdev *dev = hwdev;
	struct service_cap *cap = NULL;

	if (!dev || !slave_host_bitmap)
		return -EINVAL;

	cap = &dev->cfg_mgmt->svc_cap;

	if (HINIC3_FUNC_TYPE(dev) != TYPE_PPF) {
		sdk_warn(dev->dev_hdl, "hwdev should be ppf\n");
		return -EINVAL;
	}

	*slave_host_bitmap = cap->host_valid_bitmap & (~(1U << cap->master_host_id));

	return 0;
}
EXPORT_SYMBOL(hinic3_get_slave_bitmap);

void hinic3_set_func_host_mode(struct hinic3_hwdev *hwdev,
			       enum hinic3_func_mode mode)
{
	switch (mode) {
	case FUNC_MOD_MULTI_BM_MASTER:
		sdk_info(hwdev->dev_hdl, "Detect multi-host BM master host\n");
		hwdev->func_mode = FUNC_MOD_MULTI_BM_MASTER;
		break;
	case FUNC_MOD_MULTI_BM_SLAVE:
		sdk_info(hwdev->dev_hdl, "Detect multi-host BM slave host\n");
		hwdev->func_mode = FUNC_MOD_MULTI_BM_SLAVE;
		break;
	case FUNC_MOD_MULTI_VM_MASTER:
		sdk_info(hwdev->dev_hdl, "Detect multi-host VM master host\n");
		hwdev->func_mode = FUNC_MOD_MULTI_VM_MASTER;
		break;
	case FUNC_MOD_MULTI_VM_SLAVE:
		sdk_info(hwdev->dev_hdl, "Detect multi-host VM slave host\n");
		hwdev->func_mode = FUNC_MOD_MULTI_VM_SLAVE;
		break;
	default:
		hwdev->func_mode = FUNC_MOD_NORMAL_HOST;
		break;
	}
}

static void hinic3_init_host_mode_pre(struct hinic3_hwdev *hwdev)
{
	struct service_cap *cap = &hwdev->cfg_mgmt->svc_cap;
	u8 host_id = hwdev->hwif->attr.pci_intf_idx;

	switch (cap->srv_multi_host_mode) {
	case HINIC3_SDI_MODE_BM:
		if (host_id == cap->master_host_id)
			hinic3_set_func_host_mode(hwdev,
						  FUNC_MOD_MULTI_BM_MASTER);
		else
			hinic3_set_func_host_mode(hwdev,
						  FUNC_MOD_MULTI_BM_SLAVE);
		break;
	case HINIC3_SDI_MODE_VM:
		if (host_id == cap->master_host_id)
			hinic3_set_func_host_mode(hwdev,
						  FUNC_MOD_MULTI_VM_MASTER);
		else
			hinic3_set_func_host_mode(hwdev,
						  FUNC_MOD_MULTI_VM_SLAVE);
		break;
	default:
		hinic3_set_func_host_mode(hwdev, FUNC_MOD_NORMAL_HOST);
		break;
	}
}

static void hinic3_init_hot_plug_status(struct hinic3_hwdev *hwdev)
{
	struct service_cap *cap = &hwdev->cfg_mgmt->svc_cap;

	if (cap->hot_plug_disable)
		hwdev->hot_plug_mode = HOT_PLUG_DISABLE;
	else
		hwdev->hot_plug_mode = HOT_PLUG_ENABLE;
}

static void hinic3_init_os_hot_replace(struct hinic3_hwdev *hwdev)
{
	struct service_cap *cap = &hwdev->cfg_mgmt->svc_cap;

	if (cap->os_hot_replace) {
		hwdev->hot_replace_mode = HOT_REPLACE_ENABLE;
	} else {
		hwdev->hot_replace_mode = HOT_REPLACE_DISABLE;
	}
}

static u8 hinic3_nic_sw_aeqe_handler(void *hwdev, u8 event, u8 *data)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!dev)
		return 0;

	sdk_err(dev->dev_hdl, "Received nic ucode aeq event type: 0x%x, data: 0x%llx\n",
		event, *((u64 *)data));

	if (event < HINIC3_NIC_FATAL_ERROR_MAX)
		atomic_inc(&dev->hw_stats.nic_ucode_event_stats[event]);

	return 0;
}

static void hinic3_init_heartbeat_detect(struct hinic3_hwdev *hwdev);
static void hinic3_destroy_heartbeat_detect(struct hinic3_hwdev *hwdev);

typedef void (*mgmt_event_cb)(void *handle, void *buf_in, u16 in_size,
			      void *buf_out, u16 *out_size);

struct mgmt_event_handle {
	u16 cmd;
	mgmt_event_cb proc;
};

static int pf_handle_vf_comm_mbox(void *pri_handle,
				  u16 vf_id, u16 cmd, void *buf_in,
				  u16 in_size, void *buf_out, u16 *out_size)
{
	struct hinic3_hwdev *hwdev = pri_handle;

	if (!hwdev)
		return -EINVAL;

	sdk_warn(hwdev->dev_hdl, "Unsupported vf mbox event %u to process\n",
		 cmd);

	return 0;
}

static int vf_handle_pf_comm_mbox(void *pri_handle,
				  u16 cmd, void *buf_in,
				  u16 in_size, void *buf_out, u16 *out_size)
{
	struct hinic3_hwdev *hwdev = pri_handle;

	if (!hwdev)
		return -EINVAL;

	sdk_warn(hwdev->dev_hdl, "Unsupported pf mbox event %u to process\n",
		 cmd);
	return 0;
}

static void chip_fault_show(struct hinic3_hwdev *hwdev,
			    struct hinic3_fault_event *event)
{
	char fault_level[FAULT_LEVEL_MAX][FAULT_SHOW_STR_LEN + 1] = {
		"fatal", "reset", "host", "flr", "general", "suggestion"};
	char level_str[FAULT_SHOW_STR_LEN + 1];
	u8 level;
	int ret;

	memset(level_str, 0, FAULT_SHOW_STR_LEN + 1);
	level = event->event.chip.err_level;
	if (level < FAULT_LEVEL_MAX) {
		ret = strscpy(level_str, fault_level[level],
			      FAULT_SHOW_STR_LEN);
		if (ret < 0)
			return;
	} else {
		ret = strscpy(level_str, "Unknown", FAULT_SHOW_STR_LEN);
		if (ret < 0)
			return;
	}

	if (level == FAULT_LEVEL_SERIOUS_FLR)
		dev_err(hwdev->dev_hdl, "err_level: %u [%s], flr func_id: %u\n",
			level, level_str, event->event.chip.func_id);

	dev_err(hwdev->dev_hdl,
		"Module_id: 0x%x, err_type: 0x%x, err_level: %u[%s], err_csr_addr: 0x%08x, err_csr_value: 0x%08x\n",
		event->event.chip.node_id,
		event->event.chip.err_type, level, level_str,
		event->event.chip.err_csr_addr,
		event->event.chip.err_csr_value);
}

static void fault_report_show(struct hinic3_hwdev *hwdev,
			      struct hinic3_fault_event *event)
{
	char fault_type[FAULT_TYPE_MAX][FAULT_SHOW_STR_LEN + 1] = {
		"chip", "ucode", "mem rd timeout", "mem wr timeout",
		"reg rd timeout", "reg wr timeout", "phy fault", "tsensor fault"};
	char type_str[FAULT_SHOW_STR_LEN + 1] = {0};
	struct fault_event_stats *fault = NULL;
	int ret;

	sdk_err(hwdev->dev_hdl, "Fault event report received, func_id: %u\n",
		hinic3_global_func_id(hwdev));

	fault = &hwdev->hw_stats.fault_event_stats;

	if (event->type < FAULT_TYPE_MAX) {
		ret = strscpy(type_str, fault_type[event->type], sizeof(type_str));
		if (ret < 0)
			return;
		atomic_inc(&fault->fault_type_stat[event->type]);
	} else {
		ret = strscpy(type_str, "Unknown", sizeof(type_str));
		if (ret < 0)
			return;
	}

	sdk_err(hwdev->dev_hdl, "Fault type: %u [%s]\n", event->type, type_str);
	/* 0, 1, 2 and 3 word Represents array event->event.val index */
	sdk_err(hwdev->dev_hdl, "Fault val[0]: 0x%08x, val[1]: 0x%08x, val[2]: 0x%08x, val[3]: 0x%08x\n",
		event->event.val[0x0], event->event.val[0x1],
		event->event.val[0x2], event->event.val[0x3]);

	hinic3_show_chip_err_info(hwdev);

	switch (event->type) {
	case FAULT_TYPE_CHIP:
		chip_fault_show(hwdev, event);
		break;
	case FAULT_TYPE_UCODE:
		sdk_err(hwdev->dev_hdl, "Cause_id: %u, core_id: %u, c_id: %u, epc: 0x%08x\n",
			event->event.ucode.cause_id, event->event.ucode.core_id,
			event->event.ucode.c_id, event->event.ucode.epc);
		break;
	case FAULT_TYPE_MEM_RD_TIMEOUT:
	case FAULT_TYPE_MEM_WR_TIMEOUT:
		sdk_err(hwdev->dev_hdl, "Err_csr_ctrl: 0x%08x, err_csr_data: 0x%08x, ctrl_tab: 0x%08x, mem_index: 0x%08x\n",
			event->event.mem_timeout.err_csr_ctrl,
			event->event.mem_timeout.err_csr_data,
			event->event.mem_timeout.ctrl_tab, event->event.mem_timeout.mem_index);
		break;
	case FAULT_TYPE_REG_RD_TIMEOUT:
	case FAULT_TYPE_REG_WR_TIMEOUT:
		sdk_err(hwdev->dev_hdl, "Err_csr: 0x%08x\n", event->event.reg_timeout.err_csr);
		break;
	case FAULT_TYPE_PHY_FAULT:
		sdk_err(hwdev->dev_hdl, "Op_type: %u, port_id: %u, dev_ad: %u, csr_addr: 0x%08x, op_data: 0x%08x\n",
			event->event.phy_fault.op_type, event->event.phy_fault.port_id,
			event->event.phy_fault.dev_ad, event->event.phy_fault.csr_addr,
			event->event.phy_fault.op_data);
		break;
	default:
		break;
	}
}

static void fault_event_handler(void *dev, void *buf_in, u16 in_size,
				void *buf_out, u16 *out_size)
{
	struct hinic3_cmd_fault_event *fault_event = NULL;
	struct hinic3_fault_event *fault = NULL;
	struct hinic3_event_info event_info;
	struct hinic3_hwdev *hwdev = dev;
	u8 fault_src = HINIC3_FAULT_SRC_TYPE_MAX;
	u8 fault_level;

	if (in_size != sizeof(*fault_event)) {
		sdk_err(hwdev->dev_hdl, "Invalid fault event report, length: %u, should be %ld\n",
			in_size, sizeof(*fault_event));
		return;
	}

	fault_event = buf_in;
	fault_report_show(hwdev, &fault_event->event);

	if (fault_event->event.type == FAULT_TYPE_CHIP)
		fault_level = fault_event->event.event.chip.err_level;
	else
		fault_level = FAULT_LEVEL_FATAL;

	if (hwdev->event_callback) {
		event_info.service = EVENT_SRV_COMM;
		event_info.type = EVENT_COMM_FAULT;
		fault = (void *)event_info.event_data;
		memcpy(fault, &fault_event->event, sizeof(struct hinic3_fault_event));
		fault->fault_level = fault_level;
		hwdev->event_callback(hwdev->event_pri_handle, &event_info);
	}

	if (fault_event->event.type <= FAULT_TYPE_REG_WR_TIMEOUT)
		fault_src = fault_event->event.type;
	else if (fault_event->event.type == FAULT_TYPE_PHY_FAULT)
		fault_src = HINIC3_FAULT_SRC_HW_PHY_FAULT;

	hisdk3_fault_post_process(hwdev, fault_src, fault_level);
}

static void ffm_event_record(struct hinic3_hwdev *dev, struct dbgtool_k_glb_info *dbgtool_info,
			     struct ffm_intr_info *intr)
{
	struct rtc_time rctm;
	struct timeval txc;
	u32 ffm_idx;
	u32 last_err_csr_addr;
	u32 last_err_csr_value;

	ffm_idx = dbgtool_info->ffm->ffm_num;
	last_err_csr_addr = dbgtool_info->ffm->last_err_csr_addr;
	last_err_csr_value = dbgtool_info->ffm->last_err_csr_value;
	if (ffm_idx < FFM_RECORD_NUM_MAX) {
		if (intr->err_csr_addr == last_err_csr_addr &&
		    intr->err_csr_value == last_err_csr_value) {
			dbgtool_info->ffm->ffm[ffm_idx - 1].times++;
			sdk_err(dev->dev_hdl, "Receive intr same, ffm_idx: %u\n", ffm_idx - 1);
			return;
		}
		sdk_err(dev->dev_hdl, "Receive intr, ffm_idx: %u\n", ffm_idx);

		dbgtool_info->ffm->ffm[ffm_idx].intr_info.node_id = intr->node_id;
		dbgtool_info->ffm->ffm[ffm_idx].intr_info.err_level = intr->err_level;
		dbgtool_info->ffm->ffm[ffm_idx].intr_info.err_type = intr->err_type;
		dbgtool_info->ffm->ffm[ffm_idx].intr_info.err_csr_addr = intr->err_csr_addr;
		dbgtool_info->ffm->ffm[ffm_idx].intr_info.err_csr_value = intr->err_csr_value;
		dbgtool_info->ffm->last_err_csr_addr = intr->err_csr_addr;
		dbgtool_info->ffm->last_err_csr_value = intr->err_csr_value;
		dbgtool_info->ffm->ffm[ffm_idx].times = 1;

		/* Obtain the current UTC time */
		 do_gettimeofday(&txc);

		/* Calculate the time in date value to tm, i.e. GMT + 8, mutiplied by 60 * 60 */
		 rtc_time_to_tm((unsigned long)txc.tv_sec + 60 * 60 * 8, &rctm);

		/* tm_year starts from 1900; 0->1900, 1->1901, and so on */
		dbgtool_info->ffm->ffm[ffm_idx].year = (u16)(rctm.tm_year + 1900);
		/* tm_mon starts from 0, 0 indicates January, and so on */
		dbgtool_info->ffm->ffm[ffm_idx].mon = (u8)rctm.tm_mon + 1;
		dbgtool_info->ffm->ffm[ffm_idx].mday = (u8)rctm.tm_mday;
		dbgtool_info->ffm->ffm[ffm_idx].hour = (u8)rctm.tm_hour;
		dbgtool_info->ffm->ffm[ffm_idx].min = (u8)rctm.tm_min;
		dbgtool_info->ffm->ffm[ffm_idx].sec = (u8)rctm.tm_sec;

		dbgtool_info->ffm->ffm_num++;
	}
}

static void ffm_event_msg_handler(void *hwdev, void *buf_in, u16 in_size,
				  void *buf_out, u16 *out_size)
{
	struct dbgtool_k_glb_info *dbgtool_info = NULL;
	struct hinic3_hwdev *dev = hwdev;
	struct card_node *card_info = NULL;
	struct ffm_intr_info *intr = NULL;

	if (in_size != sizeof(*intr)) {
		sdk_err(dev->dev_hdl, "Invalid fault event report, length: %u, should be %ld.\n",
			in_size, sizeof(*intr));
		return;
	}

	intr = buf_in;

	sdk_err(dev->dev_hdl, "node_id: 0x%x, err_type: 0x%x, err_level: %u, err_csr_addr: 0x%08x, err_csr_value: 0x%08x\n",
		intr->node_id, intr->err_type, intr->err_level,
		intr->err_csr_addr, intr->err_csr_value);

	hinic3_show_chip_err_info(hwdev);

	card_info = dev->chip_node;
	dbgtool_info = card_info->dbgtool_info;

	*out_size = sizeof(*intr);

	if (!dbgtool_info)
		return;

	if (!dbgtool_info->ffm)
		return;

	ffm_event_record(dev, dbgtool_info, intr);
}

#define X_CSR_INDEX 30

static void sw_watchdog_timeout_info_show(struct hinic3_hwdev *hwdev,
					  void *buf_in, u16 in_size,
					  void *buf_out, u16 *out_size)
{
	struct comm_info_sw_watchdog *watchdog_info = buf_in;
	u32 stack_len, i, j, tmp;
	u32 *dump_addr = NULL;
	u64 *reg = NULL;

	if (in_size != sizeof(*watchdog_info)) {
		sdk_err(hwdev->dev_hdl, "Invalid mgmt watchdog report, length: %d, should be %ld\n",
			in_size, sizeof(*watchdog_info));
		return;
	}

	sdk_err(hwdev->dev_hdl, "Mgmt deadloop time: 0x%x 0x%x, task id: 0x%x, sp: 0x%llx\n",
		watchdog_info->curr_time_h, watchdog_info->curr_time_l,
		watchdog_info->task_id, watchdog_info->sp);
	sdk_err(hwdev->dev_hdl,
		"Stack current used: 0x%x, peak used: 0x%x, overflow flag: 0x%x, top: 0x%llx, bottom: 0x%llx\n",
		watchdog_info->curr_used, watchdog_info->peak_used,
		watchdog_info->is_overflow, watchdog_info->stack_top, watchdog_info->stack_bottom);

	sdk_err(hwdev->dev_hdl, "Mgmt pc: 0x%llx, elr: 0x%llx, spsr: 0x%llx, far: 0x%llx, esr: 0x%llx, xzr: 0x%llx\n",
		watchdog_info->pc, watchdog_info->elr, watchdog_info->spsr, watchdog_info->far,
		watchdog_info->esr, watchdog_info->xzr);

	sdk_err(hwdev->dev_hdl, "Mgmt register info\n");
	reg = &watchdog_info->x30;
	for (i = 0; i <= X_CSR_INDEX; i++)
		sdk_err(hwdev->dev_hdl, "x%02u:0x%llx\n",
			X_CSR_INDEX - i, reg[i]);

	if (watchdog_info->stack_actlen <= DATA_LEN_1K) {
		stack_len = watchdog_info->stack_actlen;
	} else {
		sdk_err(hwdev->dev_hdl, "Oops stack length: 0x%x is wrong\n",
			watchdog_info->stack_actlen);
		stack_len = DATA_LEN_1K;
	}

	sdk_err(hwdev->dev_hdl, "Mgmt dump stack, 16 bytes per line(start from sp)\n");
	for (i = 0; i < (stack_len / DUMP_16B_PER_LINE); i++) {
		dump_addr = (u32 *)(watchdog_info->stack_data + (u32)(i * DUMP_16B_PER_LINE));
		sdk_err(hwdev->dev_hdl, "0x%08x 0x%08x 0x%08x 0x%08x\n",
			*dump_addr, *(dump_addr + 0x1), *(dump_addr + 0x2), *(dump_addr + 0x3));
	}

	tmp = (stack_len % DUMP_16B_PER_LINE) / DUMP_4_VAR_PER_LINE;
	for (j = 0; j < tmp; j++) {
		dump_addr = (u32 *)(watchdog_info->stack_data +
				    (u32)(i * DUMP_16B_PER_LINE + j * DUMP_4_VAR_PER_LINE));
		sdk_err(hwdev->dev_hdl, "0x%08x ", *dump_addr);
	}

	*out_size = sizeof(*watchdog_info);
	watchdog_info = buf_out;
	watchdog_info->head.status = 0;
}

static void mgmt_watchdog_timeout_event_handler(void *hwdev, void *buf_in, u16 in_size,
						void *buf_out, u16 *out_size)
{
	struct hinic3_event_info event_info = { 0 };
	struct hinic3_hwdev *dev = hwdev;

	sw_watchdog_timeout_info_show(dev, buf_in, in_size, buf_out, out_size);

	if (dev->event_callback) {
		event_info.type = EVENT_COMM_MGMT_WATCHDOG;
		dev->event_callback(dev->event_pri_handle, &event_info);
	}
}

static void show_exc_info(struct hinic3_hwdev *hwdev, struct tag_exc_info *exc_info)
{
	u32 i;

	/* key information */
	sdk_err(hwdev->dev_hdl, "==================== Exception Info Begin ====================\n");
	sdk_err(hwdev->dev_hdl, "Exception CpuTick       : 0x%08x 0x%08x\n",
		exc_info->cpu_tick.cnt_hi, exc_info->cpu_tick.cnt_lo);
	sdk_err(hwdev->dev_hdl, "Exception Cause         : %u\n", exc_info->exc_cause);
	sdk_err(hwdev->dev_hdl, "Os Version              : %s\n", exc_info->os_ver);
	sdk_err(hwdev->dev_hdl, "App Version             : %s\n", exc_info->app_ver);
	sdk_err(hwdev->dev_hdl, "CPU Type                : 0x%08x\n", exc_info->cpu_type);
	sdk_err(hwdev->dev_hdl, "CPU ID                  : 0x%08x\n", exc_info->cpu_id);
	sdk_err(hwdev->dev_hdl, "Thread Type             : 0x%08x\n", exc_info->thread_type);
	sdk_err(hwdev->dev_hdl, "Thread ID               : 0x%08x\n", exc_info->thread_id);
	sdk_err(hwdev->dev_hdl, "Byte Order              : 0x%08x\n", exc_info->byte_order);
	sdk_err(hwdev->dev_hdl, "Nest Count              : 0x%08x\n", exc_info->nest_cnt);
	sdk_err(hwdev->dev_hdl, "Fatal Error Num         : 0x%08x\n", exc_info->fatal_errno);
	sdk_err(hwdev->dev_hdl, "Current SP              : 0x%016llx\n", exc_info->uw_sp);
	sdk_err(hwdev->dev_hdl, "Stack Bottom            : 0x%016llx\n", exc_info->stack_bottom);

	/* register field */
	sdk_err(hwdev->dev_hdl, "Register contents when exception occur.\n");
	sdk_err(hwdev->dev_hdl, "%-14s: 0x%016llx \t %-14s: 0x%016llx\n", "TTBR0",
		exc_info->reg_info.ttbr0, "TTBR1", exc_info->reg_info.ttbr1);
	sdk_err(hwdev->dev_hdl, "%-14s: 0x%016llx \t %-14s: 0x%016llx\n", "TCR",
		exc_info->reg_info.tcr, "MAIR", exc_info->reg_info.mair);
	sdk_err(hwdev->dev_hdl, "%-14s: 0x%016llx \t %-14s: 0x%016llx\n", "SCTLR",
		exc_info->reg_info.sctlr, "VBAR", exc_info->reg_info.vbar);
	sdk_err(hwdev->dev_hdl, "%-14s: 0x%016llx \t %-14s: 0x%016llx\n", "CURRENTE1",
		exc_info->reg_info.current_el, "SP", exc_info->reg_info.sp);
	sdk_err(hwdev->dev_hdl, "%-14s: 0x%016llx \t %-14s: 0x%016llx\n", "ELR",
		exc_info->reg_info.elr, "SPSR", exc_info->reg_info.spsr);
	sdk_err(hwdev->dev_hdl, "%-14s: 0x%016llx \t %-14s: 0x%016llx\n", "FAR",
		exc_info->reg_info.far_r, "ESR", exc_info->reg_info.esr);
	sdk_err(hwdev->dev_hdl, "%-14s: 0x%016llx\n", "XZR", exc_info->reg_info.xzr);

	for (i = 0; i < XREGS_NUM - 1; i += 0x2)
		sdk_err(hwdev->dev_hdl, "XREGS[%02u]%-5s: 0x%016llx \t XREGS[%02u]%-5s: 0x%016llx",
			i, " ", exc_info->reg_info.xregs[i],
			(u32)(i + 0x1U), " ", exc_info->reg_info.xregs[(u32)(i + 0x1U)]);

	sdk_err(hwdev->dev_hdl, "XREGS[%02u]%-5s: 0x%016llx \t ", XREGS_NUM - 1, " ",
		exc_info->reg_info.xregs[XREGS_NUM - 1]);
}

#define FOUR_REG_LEN 16

static void mgmt_lastword_report_event_handler(void *hwdev, void *buf_in, u16 in_size,
					       void *buf_out, u16 *out_size)
{
	struct tag_comm_info_up_lastword *lastword_info = buf_in;
	struct tag_exc_info *exc_info = &lastword_info->stack_info;
	u32 stack_len = lastword_info->stack_actlen;
	struct hinic3_hwdev *dev = hwdev;
	u32 *curr_reg = NULL;
	u32 reg_i, cnt;

	if (in_size != sizeof(*lastword_info)) {
		sdk_err(dev->dev_hdl, "Invalid mgmt lastword, length: %u, should be %ld\n",
			in_size, sizeof(*lastword_info));
		return;
	}

	show_exc_info(dev, exc_info);

	/* call stack dump */
	sdk_err(dev->dev_hdl, "Dump stack when exceptioin occurs, 16Bytes per line.\n");

	cnt = stack_len / FOUR_REG_LEN;
	for (reg_i = 0; reg_i < cnt; reg_i++) {
		curr_reg = (u32 *)(lastword_info->stack_data + ((u64)(u32)(reg_i * FOUR_REG_LEN)));
		sdk_err(dev->dev_hdl, "0x%08x 0x%08x 0x%08x 0x%08x\n",
			*curr_reg, *(curr_reg + 0x1), *(curr_reg + 0x2), *(curr_reg + 0x3));
	}

	sdk_err(dev->dev_hdl, "==================== Exception Info End ====================\n");
}

const struct mgmt_event_handle mgmt_event_proc[] = {
	{
		.cmd	= COMM_MGMT_CMD_FAULT_REPORT,
		.proc	= fault_event_handler,
	},

	{
		.cmd	= COMM_MGMT_CMD_FFM_SET,
		.proc	= ffm_event_msg_handler,
	},

	{
		.cmd	= COMM_MGMT_CMD_WATCHDOG_INFO,
		.proc	= mgmt_watchdog_timeout_event_handler,
	},

	{
		.cmd	= COMM_MGMT_CMD_LASTWORD_GET,
		.proc	= mgmt_lastword_report_event_handler,
	},
};

static void pf_handle_mgmt_comm_event(void *handle, u16 cmd,
				      void *buf_in, u16 in_size, void *buf_out,
				      u16 *out_size)
{
	struct hinic3_hwdev *hwdev = handle;
	u32 i, event_num = ARRAY_LEN(mgmt_event_proc);

	if (!hwdev)
		return;

	for (i = 0; i < event_num; i++) {
		if (cmd == mgmt_event_proc[i].cmd) {
			if (mgmt_event_proc[i].proc)
				mgmt_event_proc[i].proc(handle, buf_in, in_size,
							buf_out, out_size);

			return;
		}
	}

	sdk_warn(hwdev->dev_hdl, "Unsupported mgmt cpu event %u to process\n",
		 cmd);
	*out_size = sizeof(struct mgmt_msg_head);
	((struct mgmt_msg_head *)buf_out)->status = HINIC3_MGMT_CMD_UNSUPPORTED;
}

static void hinic3_set_chip_present(struct hinic3_hwdev *hwdev)
{
	hwdev->chip_present_flag = HINIC3_CHIP_PRESENT;
}

static void hinic3_set_chip_absent(struct hinic3_hwdev *hwdev)
{
	sdk_err(hwdev->dev_hdl, "Card not present\n");
	hwdev->chip_present_flag = HINIC3_CHIP_ABSENT;
}

int hinic3_get_chip_present_flag(const void *hwdev)
{
	if (!hwdev)
		return 0;

	return ((struct hinic3_hwdev *)hwdev)->chip_present_flag;
}
EXPORT_SYMBOL(hinic3_get_chip_present_flag);

void hinic3_force_complete_all(void *dev)
{
	struct hinic3_recv_msg *recv_resp_msg = NULL;
	struct hinic3_hwdev *hwdev = dev;
	struct hinic3_mbox *func_to_func = NULL;

	spin_lock_bh(&hwdev->channel_lock);
	if (hinic3_func_type(hwdev) != TYPE_VF &&
	    test_bit(HINIC3_HWDEV_MGMT_INITED, &hwdev->func_state)) {
		recv_resp_msg = &hwdev->pf_to_mgmt->recv_resp_msg_from_mgmt;
		spin_lock_bh(&hwdev->pf_to_mgmt->sync_event_lock);
		if (hwdev->pf_to_mgmt->event_flag == SEND_EVENT_START) {
			complete(&recv_resp_msg->recv_done);
			hwdev->pf_to_mgmt->event_flag = SEND_EVENT_TIMEOUT;
		}
		spin_unlock_bh(&hwdev->pf_to_mgmt->sync_event_lock);
	}

	if (test_bit(HINIC3_HWDEV_MBOX_INITED, &hwdev->func_state)) {
		func_to_func = hwdev->func_to_func;
		spin_lock(&func_to_func->mbox_lock);
		if (func_to_func->event_flag == EVENT_START)
			func_to_func->event_flag = EVENT_TIMEOUT;
		spin_unlock(&func_to_func->mbox_lock);
	}

	if (test_bit(HINIC3_HWDEV_CMDQ_INITED, &hwdev->func_state))
		hinic3_cmdq_flush_sync_cmd(hwdev);

	spin_unlock_bh(&hwdev->channel_lock);
}
EXPORT_SYMBOL(hinic3_force_complete_all);

void hinic3_detect_hw_present(void *hwdev)
{
	if (!get_card_present_state((struct hinic3_hwdev *)hwdev)) {
		hinic3_set_chip_absent(hwdev);
		hinic3_force_complete_all(hwdev);
	}
}

/**
 * dma_attr_table_init - initialize the default dma attributes
 * @hwdev: the pointer to hw device
 **/
static int dma_attr_table_init(struct hinic3_hwdev *hwdev)
{
	u32 addr, val, dst_attr;

	/* Use indirect access should set entry_idx first */
	addr = HINIC3_CSR_DMA_ATTR_INDIR_IDX_ADDR;
	val = hinic3_hwif_read_reg(hwdev->hwif, addr);
	val = HINIC3_DMA_ATTR_INDIR_IDX_CLEAR(val, IDX);

	val |= HINIC3_DMA_ATTR_INDIR_IDX_SET(PCIE_MSIX_ATTR_ENTRY, IDX);

	hinic3_hwif_write_reg(hwdev->hwif, addr, val);

	wmb(); /* write index before config */

	addr = HINIC3_CSR_DMA_ATTR_TBL_ADDR;
	val = hinic3_hwif_read_reg(hwdev->hwif, addr);

	dst_attr = HINIC3_DMA_ATTR_ENTRY_SET(HINIC3_PCIE_ST_DISABLE, ST)	|
		HINIC3_DMA_ATTR_ENTRY_SET(HINIC3_PCIE_AT_DISABLE, AT)		|
		HINIC3_DMA_ATTR_ENTRY_SET(HINIC3_PCIE_PH_DISABLE, PH)		|
		HINIC3_DMA_ATTR_ENTRY_SET(HINIC3_PCIE_SNOOP, NO_SNOOPING)	|
		HINIC3_DMA_ATTR_ENTRY_SET(HINIC3_PCIE_TPH_DISABLE, TPH_EN);

	if (val == dst_attr)
		return 0;

	return hinic3_set_dma_attr_tbl(hwdev, PCIE_MSIX_ATTR_ENTRY, HINIC3_PCIE_ST_DISABLE,
				       HINIC3_PCIE_AT_DISABLE, HINIC3_PCIE_PH_DISABLE,
				       HINIC3_PCIE_SNOOP, HINIC3_PCIE_TPH_DISABLE);
}

static int init_aeqs_msix_attr(struct hinic3_hwdev *hwdev)
{
	struct hinic3_aeqs *aeqs = hwdev->aeqs;
	struct interrupt_info info = {0};
	struct hinic3_eq *eq = NULL;
	int q_id;
	int err;

	info.lli_set = 0;
	info.interrupt_coalesc_set = 1;
	info.pending_limt = HINIC3_DEAULT_EQ_MSIX_PENDING_LIMIT;
	info.coalesc_timer_cfg = HINIC3_DEAULT_EQ_MSIX_COALESC_TIMER_CFG;
	info.resend_timer_cfg = HINIC3_DEAULT_EQ_MSIX_RESEND_TIMER_CFG;

	for (q_id = aeqs->num_aeqs - 1; q_id >= 0; q_id--) {
		eq = &aeqs->aeq[q_id];
		info.msix_index = eq->eq_irq.msix_entry_idx;
		err = hinic3_set_interrupt_cfg_direct(hwdev, &info,
						      HINIC3_CHANNEL_COMM);
		if (err != 0) {
			sdk_err(hwdev->dev_hdl, "Set msix attr for aeq %d failed\n",
				q_id);
			return -EFAULT;
		}
	}

	return 0;
}

static int init_ceqs_msix_attr(struct hinic3_hwdev *hwdev)
{
	struct hinic3_ceqs *ceqs = hwdev->ceqs;
	struct interrupt_info info = {0};
	struct hinic3_eq *eq = NULL;
	u16 q_id;
	int err;

	info.lli_set = 0;
	info.interrupt_coalesc_set = 1;
	info.pending_limt = HINIC3_DEAULT_EQ_MSIX_PENDING_LIMIT;
	info.coalesc_timer_cfg = HINIC3_DEAULT_EQ_MSIX_COALESC_TIMER_CFG;
	info.resend_timer_cfg = HINIC3_DEAULT_EQ_MSIX_RESEND_TIMER_CFG;

	for (q_id = 0; q_id < ceqs->num_ceqs; q_id++) {
		eq = &ceqs->ceq[q_id];
		info.msix_index = eq->eq_irq.msix_entry_idx;
		err = hinic3_set_interrupt_cfg(hwdev, info,
					       HINIC3_CHANNEL_COMM);
		if (err != 0) {
			sdk_err(hwdev->dev_hdl, "Set msix attr for ceq %u failed\n",
				q_id);
			return -EFAULT;
		}
	}

	return 0;
}

static int hinic3_comm_clp_to_mgmt_init(struct hinic3_hwdev *hwdev)
{
	int err;

	if (hinic3_func_type(hwdev) == TYPE_VF || !COMM_SUPPORT_CLP(hwdev))
		return 0;

	err = hinic3_clp_pf_to_mgmt_init(hwdev);
	if (err != 0)
		return err;

	return 0;
}

static void hinic3_comm_clp_to_mgmt_free(struct hinic3_hwdev *hwdev)
{
	if (hinic3_func_type(hwdev) == TYPE_VF || !COMM_SUPPORT_CLP(hwdev))
		return;

	hinic3_clp_pf_to_mgmt_free(hwdev);
}

static int hinic3_comm_aeqs_init(struct hinic3_hwdev *hwdev)
{
	struct irq_info aeq_irqs[HINIC3_MAX_AEQS] = {{0} };
	u16 num_aeqs, resp_num_irq = 0, i;
	int err;

	num_aeqs = HINIC3_HWIF_NUM_AEQS(hwdev->hwif);
	if (num_aeqs > HINIC3_MAX_AEQS) {
		sdk_warn(hwdev->dev_hdl, "Adjust aeq num to %d\n",
			 HINIC3_MAX_AEQS);
		num_aeqs = HINIC3_MAX_AEQS;
	}
	err = hinic3_alloc_irqs(hwdev, SERVICE_T_INTF, num_aeqs, aeq_irqs,
				&resp_num_irq);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to alloc aeq irqs, num_aeqs: %u\n",
			num_aeqs);
		return err;
	}

	if (resp_num_irq < num_aeqs) {
		sdk_warn(hwdev->dev_hdl, "Adjust aeq num to %u\n",
			 resp_num_irq);
		num_aeqs = resp_num_irq;
	}

	err = hinic3_aeqs_init(hwdev, num_aeqs, aeq_irqs);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to init aeqs\n");
		goto aeqs_init_err;
	}

	return 0;

aeqs_init_err:
	for (i = 0; i < num_aeqs; i++)
		hinic3_free_irq(hwdev, SERVICE_T_INTF, aeq_irqs[i].irq_id);

	return err;
}

static void hinic3_comm_aeqs_free(struct hinic3_hwdev *hwdev)
{
	struct irq_info aeq_irqs[HINIC3_MAX_AEQS] = {{0} };
	u16 num_irqs, i;

	hinic3_get_aeq_irqs(hwdev, (struct irq_info *)aeq_irqs, &num_irqs);

	hinic3_aeqs_free(hwdev);

	for (i = 0; i < num_irqs; i++)
		hinic3_free_irq(hwdev, SERVICE_T_INTF, aeq_irqs[i].irq_id);
}

static int hinic3_comm_ceqs_init(struct hinic3_hwdev *hwdev)
{
	struct irq_info ceq_irqs[HINIC3_MAX_CEQS] = {{0} };
	u16 num_ceqs, resp_num_irq = 0, i;
	int err;

	num_ceqs = HINIC3_HWIF_NUM_CEQS(hwdev->hwif);
	if (num_ceqs > HINIC3_MAX_CEQS) {
		sdk_warn(hwdev->dev_hdl, "Adjust ceq num to %d\n",
			 HINIC3_MAX_CEQS);
		num_ceqs = HINIC3_MAX_CEQS;
	}

	err = hinic3_alloc_irqs(hwdev, SERVICE_T_INTF, num_ceqs, ceq_irqs,
				&resp_num_irq);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to alloc ceq irqs, num_ceqs: %u\n",
			num_ceqs);
		return err;
	}

	if (resp_num_irq < num_ceqs) {
		sdk_warn(hwdev->dev_hdl, "Adjust ceq num to %u\n",
			 resp_num_irq);
		num_ceqs = resp_num_irq;
	}

	err = hinic3_ceqs_init(hwdev, num_ceqs, ceq_irqs);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl,
			"Failed to init ceqs, err:%d\n", err);
		goto ceqs_init_err;
	}

	return 0;

ceqs_init_err:
	for (i = 0; i < num_ceqs; i++)
		hinic3_free_irq(hwdev, SERVICE_T_INTF, ceq_irqs[i].irq_id);

	return err;
}

static void hinic3_comm_ceqs_free(struct hinic3_hwdev *hwdev)
{
	struct irq_info ceq_irqs[HINIC3_MAX_CEQS] = {{0} };
	u16 num_irqs;
	int i;

	hinic3_get_ceq_irqs(hwdev, (struct irq_info *)ceq_irqs, &num_irqs);

	hinic3_ceqs_free(hwdev);

	for (i = 0; i < num_irqs; i++)
		hinic3_free_irq(hwdev, SERVICE_T_INTF, ceq_irqs[i].irq_id);
}

static int hinic3_comm_func_to_func_init(struct hinic3_hwdev *hwdev)
{
	int err;

	err = hinic3_func_to_func_init(hwdev);
	if (err != 0)
		return err;

	hinic3_aeq_register_hw_cb(hwdev, hwdev, HINIC3_MBX_FROM_FUNC,
				  hinic3_mbox_func_aeqe_handler);
	hinic3_aeq_register_hw_cb(hwdev, hwdev, HINIC3_MSG_FROM_MGMT_CPU,
				  hinic3_mgmt_msg_aeqe_handler);

	if (!HINIC3_IS_VF(hwdev)) {
		hinic3_register_pf_mbox_cb(hwdev, HINIC3_MOD_COMM, hwdev, pf_handle_vf_comm_mbox);
		hinic3_register_pf_mbox_cb(hwdev, HINIC3_MOD_SW_FUNC,
					   hwdev,
					   hinic3_sw_func_pf_mbox_handler);
	} else {
		hinic3_register_vf_mbox_cb(hwdev, HINIC3_MOD_COMM, hwdev, vf_handle_pf_comm_mbox);
	}

	set_bit(HINIC3_HWDEV_MBOX_INITED, &hwdev->func_state);

	return 0;
}

static void hinic3_comm_func_to_func_free(struct hinic3_hwdev *hwdev)
{
	spin_lock_bh(&hwdev->channel_lock);
	clear_bit(HINIC3_HWDEV_MBOX_INITED, &hwdev->func_state);
	spin_unlock_bh(&hwdev->channel_lock);

	hinic3_aeq_unregister_hw_cb(hwdev, HINIC3_MBX_FROM_FUNC);

	if (!HINIC3_IS_VF(hwdev)) {
		hinic3_unregister_pf_mbox_cb(hwdev, HINIC3_MOD_COMM);
	} else {
		hinic3_unregister_vf_mbox_cb(hwdev, HINIC3_MOD_COMM);

		hinic3_aeq_unregister_hw_cb(hwdev, HINIC3_MSG_FROM_MGMT_CPU);
	}

	hinic3_func_to_func_free(hwdev);
}

static int hinic3_comm_pf_to_mgmt_init(struct hinic3_hwdev *hwdev)
{
	int err;

	if (hinic3_func_type(hwdev) == TYPE_VF)
		return 0;

	err = hinic3_pf_to_mgmt_init(hwdev);
	if (err != 0)
		return err;

	hinic3_register_mgmt_msg_cb(hwdev, HINIC3_MOD_COMM, hwdev,
				    pf_handle_mgmt_comm_event);

	set_bit(HINIC3_HWDEV_MGMT_INITED, &hwdev->func_state);

	return 0;
}

static void hinic3_comm_pf_to_mgmt_free(struct hinic3_hwdev *hwdev)
{
	if (hinic3_func_type(hwdev) == TYPE_VF)
		return;

	spin_lock_bh(&hwdev->channel_lock);
	clear_bit(HINIC3_HWDEV_MGMT_INITED, &hwdev->func_state);
	spin_unlock_bh(&hwdev->channel_lock);

	hinic3_unregister_mgmt_msg_cb(hwdev, HINIC3_MOD_COMM);

	hinic3_aeq_unregister_hw_cb(hwdev, HINIC3_MSG_FROM_MGMT_CPU);

	hinic3_pf_to_mgmt_free(hwdev);
}

static int hinic3_comm_cmdqs_init(struct hinic3_hwdev *hwdev)
{
	int err;

	err = hinic3_cmdqs_init(hwdev);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to init cmd queues\n");
		return err;
	}

	hinic3_ceq_register_cb(hwdev, hwdev, HINIC3_CMDQ, hinic3_cmdq_ceq_handler);

	err = hinic3_set_cmdq_depth(hwdev, HINIC3_CMDQ_DEPTH);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to set cmdq depth\n");
		goto set_cmdq_depth_err;
	}

	set_bit(HINIC3_HWDEV_CMDQ_INITED, &hwdev->func_state);

	return 0;

set_cmdq_depth_err:
	hinic3_cmdqs_free(hwdev);

	return err;
}

static void hinic3_comm_cmdqs_free(struct hinic3_hwdev *hwdev)
{
	spin_lock_bh(&hwdev->channel_lock);
	clear_bit(HINIC3_HWDEV_CMDQ_INITED, &hwdev->func_state);
	spin_unlock_bh(&hwdev->channel_lock);

	hinic3_ceq_unregister_cb(hwdev, HINIC3_CMDQ);
	hinic3_cmdqs_free(hwdev);
}

static void hinic3_sync_mgmt_func_state(struct hinic3_hwdev *hwdev)
{
	hinic3_set_pf_status(hwdev->hwif, HINIC3_PF_STATUS_ACTIVE_FLAG);
}

static void hinic3_unsync_mgmt_func_state(struct hinic3_hwdev *hwdev)
{
	hinic3_set_pf_status(hwdev->hwif, HINIC3_PF_STATUS_INIT);
}

static int init_basic_attributes(struct hinic3_hwdev *hwdev)
{
	u64 drv_features[COMM_MAX_FEATURE_QWORD] = {HINIC3_DRV_FEATURE_QW0, 0, 0, 0};
	int err, i;

	if (hinic3_func_type(hwdev) == TYPE_PPF)
		drv_features[0] |= COMM_F_CHANNEL_DETECT;

	err = hinic3_get_board_info(hwdev, &hwdev->board_info,
				    HINIC3_CHANNEL_COMM);
	if (err != 0)
		return err;

	err = hinic3_get_comm_features(hwdev, hwdev->features,
				       COMM_MAX_FEATURE_QWORD);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Get comm features failed\n");
		return err;
	}

	sdk_info(hwdev->dev_hdl, "Comm hw features: 0x%llx, drv features: 0x%llx\n",
		 hwdev->features[0], drv_features[0]);

	for (i = 0; i < COMM_MAX_FEATURE_QWORD; i++)
		hwdev->features[i] &= drv_features[i];

	err = hinic3_get_global_attr(hwdev, &hwdev->glb_attr);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to get global attribute\n");
		return err;
	}

	sdk_info(hwdev->dev_hdl,
		 "global attribute: max_host: 0x%x, max_pf: 0x%x, vf_id_start: 0x%x, mgmt node id: 0x%x, cmdq_num: 0x%x\n",
		 hwdev->glb_attr.max_host_num, hwdev->glb_attr.max_pf_num,
		 hwdev->glb_attr.vf_id_start,
		 hwdev->glb_attr.mgmt_host_node_id,
		 hwdev->glb_attr.cmdq_num);

	return 0;
}

static int init_basic_mgmt_channel(struct hinic3_hwdev *hwdev)
{
	int err;

	err = hinic3_comm_aeqs_init(hwdev);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to init async event queues\n");
		return err;
	}

	err = hinic3_comm_func_to_func_init(hwdev);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to init mailbox\n");
		goto func_to_func_init_err;
	}

	err = init_aeqs_msix_attr(hwdev);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to init aeqs msix attr\n");
		goto aeqs_msix_attr_init_err;
	}

	return 0;

aeqs_msix_attr_init_err:
	hinic3_comm_func_to_func_free(hwdev);

func_to_func_init_err:
	hinic3_comm_aeqs_free(hwdev);

	return err;
}

static void free_base_mgmt_channel(struct hinic3_hwdev *hwdev)
{
	hinic3_comm_func_to_func_free(hwdev);
	hinic3_comm_aeqs_free(hwdev);
}

static int init_pf_mgmt_channel(struct hinic3_hwdev *hwdev)
{
	int err;

	err = hinic3_comm_clp_to_mgmt_init(hwdev);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to init clp\n");
		return err;
	}

	err = hinic3_comm_pf_to_mgmt_init(hwdev);
	if (err != 0) {
		hinic3_comm_clp_to_mgmt_free(hwdev);
		sdk_err(hwdev->dev_hdl, "Failed to init pf to mgmt\n");
		return err;
	}

	return 0;
}

static void free_pf_mgmt_channel(struct hinic3_hwdev *hwdev)
{
	hinic3_comm_clp_to_mgmt_free(hwdev);
	hinic3_comm_pf_to_mgmt_free(hwdev);
}

static int init_mgmt_channel_post(struct hinic3_hwdev *hwdev)
{
	int err;

	/* mbox host channel resources will be freed in
	 * hinic3_func_to_func_free
	 */
	if (HINIC3_IS_PPF(hwdev)) {
		err = hinic3_mbox_init_host_msg_channel(hwdev);
		if (err != 0) {
			sdk_err(hwdev->dev_hdl, "Failed to init mbox host channel\n");
			return err;
		}
	}

	err = init_pf_mgmt_channel(hwdev);
	if (err != 0)
		return err;

	return 0;
}

static void free_mgmt_msg_channel_post(struct hinic3_hwdev *hwdev)
{
	free_pf_mgmt_channel(hwdev);
}

static int init_cmdqs_channel(struct hinic3_hwdev *hwdev)
{
	int err;

	err = dma_attr_table_init(hwdev);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to init dma attr table\n");
		goto dma_attr_init_err;
	}

	err = hinic3_comm_ceqs_init(hwdev);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to init completion event queues\n");
		goto ceqs_init_err;
	}

	err = init_ceqs_msix_attr(hwdev);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to init ceqs msix attr\n");
		goto init_ceq_msix_err;
	}

	/* set default wq page_size */
	if (wq_page_order > HINIC3_MAX_WQ_PAGE_SIZE_ORDER) {
		sdk_info(hwdev->dev_hdl, "wq_page_order exceed limit[0, %d], reset to %d\n",
			 HINIC3_MAX_WQ_PAGE_SIZE_ORDER,
			 HINIC3_MAX_WQ_PAGE_SIZE_ORDER);
		wq_page_order = HINIC3_MAX_WQ_PAGE_SIZE_ORDER;
	}
	hwdev->wq_page_size = HINIC3_HW_WQ_PAGE_SIZE * (1U << wq_page_order);
	sdk_info(hwdev->dev_hdl, "WQ page size: 0x%x\n", hwdev->wq_page_size);
	err = hinic3_set_wq_page_size(hwdev, hinic3_global_func_id(hwdev),
				      hwdev->wq_page_size, HINIC3_CHANNEL_COMM);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to set wq page size\n");
		goto init_wq_pg_size_err;
	}

	err = hinic3_comm_cmdqs_init(hwdev);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to init cmd queues\n");
		goto cmdq_init_err;
	}

	return 0;

cmdq_init_err:
	if (HINIC3_FUNC_TYPE(hwdev) != TYPE_VF)
		hinic3_set_wq_page_size(hwdev, hinic3_global_func_id(hwdev),
					HINIC3_HW_WQ_PAGE_SIZE,
					HINIC3_CHANNEL_COMM);
init_wq_pg_size_err:
init_ceq_msix_err:
	hinic3_comm_ceqs_free(hwdev);

ceqs_init_err:
dma_attr_init_err:

	return err;
}

static void hinic3_free_cmdqs_channel(struct hinic3_hwdev *hwdev)
{
	hinic3_comm_cmdqs_free(hwdev);

	if (HINIC3_FUNC_TYPE(hwdev) != TYPE_VF)
		hinic3_set_wq_page_size(hwdev, hinic3_global_func_id(hwdev),
					HINIC3_HW_WQ_PAGE_SIZE, HINIC3_CHANNEL_COMM);

	hinic3_comm_ceqs_free(hwdev);
}

static int hinic3_init_comm_ch(struct hinic3_hwdev *hwdev)
{
	int err;

	err = init_basic_mgmt_channel(hwdev);
	if (err != 0)
		return err;

	err = hinic3_func_reset(hwdev, hinic3_global_func_id(hwdev),
				HINIC3_COMM_RES, HINIC3_CHANNEL_COMM);
	if (err != 0)
		goto func_reset_err;

	err = init_basic_attributes(hwdev);
	if (err != 0)
		goto init_basic_attr_err;

	err = init_mgmt_channel_post(hwdev);
	if (err != 0)
		goto init_mgmt_channel_post_err;

	err = hinic3_set_func_svc_used_state(hwdev, SVC_T_COMM, 1, HINIC3_CHANNEL_COMM);
	if (err != 0)
		goto set_used_state_err;

	err = init_cmdqs_channel(hwdev);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to init cmdq channel\n");
		goto init_cmdqs_channel_err;
	}

	hinic3_sync_mgmt_func_state(hwdev);

	if (HISDK3_F_CHANNEL_LOCK_EN(hwdev)) {
		hinic3_mbox_enable_channel_lock(hwdev, true);
		hinic3_cmdq_enable_channel_lock(hwdev, true);
	}

	err = hinic3_aeq_register_swe_cb(hwdev, hwdev, HINIC3_STATELESS_EVENT,
					 hinic3_nic_sw_aeqe_handler);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl,
			"Failed to register sw aeqe handler\n");
		goto register_ucode_aeqe_err;
	}

	return 0;

register_ucode_aeqe_err:
	hinic3_unsync_mgmt_func_state(hwdev);
	hinic3_free_cmdqs_channel(hwdev);
init_cmdqs_channel_err:
	hinic3_set_func_svc_used_state(hwdev, SVC_T_COMM, 0, HINIC3_CHANNEL_COMM);
set_used_state_err:
	free_mgmt_msg_channel_post(hwdev);
init_mgmt_channel_post_err:
init_basic_attr_err:
func_reset_err:
	free_base_mgmt_channel(hwdev);

	return err;
}

static void hinic3_uninit_comm_ch(struct hinic3_hwdev *hwdev)
{
	hinic3_aeq_unregister_swe_cb(hwdev, HINIC3_STATELESS_EVENT);

	hinic3_unsync_mgmt_func_state(hwdev);

	hinic3_free_cmdqs_channel(hwdev);

	hinic3_set_func_svc_used_state(hwdev, SVC_T_COMM, 0, HINIC3_CHANNEL_COMM);

	free_mgmt_msg_channel_post(hwdev);

	free_base_mgmt_channel(hwdev);
}

static void hinic3_auto_sync_time_work(struct work_struct *work)
{
	struct delayed_work *delay = to_delayed_work(work);
	struct hinic3_hwdev *hwdev = container_of(delay, struct hinic3_hwdev, sync_time_task);
	int err;

	err = hinic3_sync_time(hwdev, ossl_get_real_time());
	if (err != 0)
		sdk_err(hwdev->dev_hdl, "Synchronize UTC time to firmware failed, errno:%d.\n",
			err);

	queue_delayed_work(hwdev->workq, &hwdev->sync_time_task,
			   msecs_to_jiffies(HINIC3_SYNFW_TIME_PERIOD));
}

static void hinic3_auto_channel_detect_work(struct work_struct *work)
{
	struct delayed_work *delay = to_delayed_work(work);
	struct hinic3_hwdev *hwdev = container_of(delay, struct hinic3_hwdev, channel_detect_task);
	struct card_node *chip_node = NULL;

	hinic3_comm_channel_detect(hwdev);

	chip_node = hwdev->chip_node;
	if (!atomic_read(&chip_node->channel_busy_cnt))
		queue_delayed_work(hwdev->workq, &hwdev->channel_detect_task,
				   msecs_to_jiffies(HINIC3_CHANNEL_DETECT_PERIOD));
}

static int hinic3_init_ppf_work(struct hinic3_hwdev *hwdev)
{
	if (hinic3_func_type(hwdev) != TYPE_PPF)
		return 0;

	INIT_DELAYED_WORK(&hwdev->sync_time_task, hinic3_auto_sync_time_work);
	queue_delayed_work(hwdev->workq, &hwdev->sync_time_task,
			   msecs_to_jiffies(HINIC3_SYNFW_TIME_PERIOD));

	if (COMM_SUPPORT_CHANNEL_DETECT(hwdev)) {
		INIT_DELAYED_WORK(&hwdev->channel_detect_task,
				  hinic3_auto_channel_detect_work);
		queue_delayed_work(hwdev->workq, &hwdev->channel_detect_task,
				   msecs_to_jiffies(HINIC3_CHANNEL_DETECT_PERIOD));
	}

	return 0;
}

static void hinic3_free_ppf_work(struct hinic3_hwdev *hwdev)
{
	if (hinic3_func_type(hwdev) != TYPE_PPF)
		return;

	if (COMM_SUPPORT_CHANNEL_DETECT(hwdev)) {
		hwdev->features[0] &= ~(COMM_F_CHANNEL_DETECT);
		cancel_delayed_work_sync(&hwdev->channel_detect_task);
	}

	cancel_delayed_work_sync(&hwdev->sync_time_task);
}

static int init_hwdew(struct hinic3_init_para *para)
{
	struct hinic3_hwdev *hwdev;

	hwdev = kzalloc(sizeof(*hwdev), GFP_KERNEL);
	if (!hwdev)
		return -ENOMEM;

	*para->hwdev = hwdev;
	hwdev->adapter_hdl = para->adapter_hdl;
	hwdev->pcidev_hdl = para->pcidev_hdl;
	hwdev->dev_hdl = para->dev_hdl;
	hwdev->chip_node = para->chip_node;
	hwdev->poll = para->poll;
	hwdev->probe_fault_level = para->probe_fault_level;
	hwdev->func_state = 0;
	sema_init(&hwdev->ppf_sem, 1);

	hwdev->chip_fault_stats = vzalloc(HINIC3_CHIP_FAULT_SIZE);
	if (!hwdev->chip_fault_stats)
		goto alloc_chip_fault_stats_err;

	hwdev->stateful_ref_cnt = 0;
	memset(hwdev->features, 0, sizeof(hwdev->features));

	spin_lock_init(&hwdev->channel_lock);
	mutex_init(&hwdev->stateful_mutex);

	return 0;

alloc_chip_fault_stats_err:
	sema_deinit(&hwdev->ppf_sem);
	para->probe_fault_level = hwdev->probe_fault_level;
	kfree(hwdev);
	*para->hwdev = NULL;
	return  -EFAULT;
}

int hinic3_init_hwdev(struct hinic3_init_para *para)
{
	struct hinic3_hwdev *hwdev = NULL;
	int err;

	err = init_hwdew(para);
	if (err != 0)
		return err;

	hwdev = *para->hwdev;
	err = hinic3_init_hwif(hwdev, para->cfg_reg_base, para->intr_reg_base, para->mgmt_reg_base,
			       para->db_base_phy, para->db_base, para->db_dwqe_len);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to init hwif\n");
		goto init_hwif_err;
	}

	hinic3_set_chip_present(hwdev);

	hisdk3_init_profile_adapter(hwdev);

	hwdev->workq = alloc_workqueue(HINIC3_HWDEV_WQ_NAME, WQ_MEM_RECLAIM, HINIC3_WQ_MAX_REQ);
	if (!hwdev->workq) {
		sdk_err(hwdev->dev_hdl, "Failed to alloc hardware workq\n");
		goto alloc_workq_err;
	}

	hinic3_init_heartbeat_detect(hwdev);

	err = init_cfg_mgmt(hwdev);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to init config mgmt\n");
		goto init_cfg_mgmt_err;
	}

	err = hinic3_init_comm_ch(hwdev);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to init communication channel\n");
		goto init_comm_ch_err;
	}

#ifdef HAVE_DEVLINK_FLASH_UPDATE_PARAMS
	err = hinic3_init_devlink(hwdev);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to init devlink\n");
		goto init_devlink_err;
	}
#endif

	err = init_capability(hwdev);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to init capability\n");
		goto init_cap_err;
	}

	hinic3_init_host_mode_pre(hwdev);

	hinic3_init_hot_plug_status(hwdev);

	hinic3_init_os_hot_replace(hwdev);

	err = hinic3_multi_host_mgmt_init(hwdev);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to init function mode\n");
		goto init_multi_host_fail;
	}

	// hot_replace_mode is enable, run ppf function only when partition_id is 0
	// or run ppf function directly
	if (hwdev->hot_replace_mode == HOT_REPLACE_ENABLE) {
		if (get_partition_id() == 0) {
			err = hinic3_init_ppf_work(hwdev);
			if (err != 0)
				goto init_ppf_work_fail;
		}
	} else {
		err = hinic3_init_ppf_work(hwdev);
		if (err != 0)
			goto init_ppf_work_fail;
	}

	err = hinic3_set_comm_features(hwdev, hwdev->features, COMM_MAX_FEATURE_QWORD);
	if (err != 0) {
		sdk_err(hwdev->dev_hdl, "Failed to set comm features\n");
		goto set_feature_err;
	}

	return 0;

set_feature_err:
	hinic3_free_ppf_work(hwdev);

init_ppf_work_fail:
	hinic3_multi_host_mgmt_free(hwdev);

init_multi_host_fail:
	free_capability(hwdev);

init_cap_err:
#ifdef HAVE_DEVLINK_FLASH_UPDATE_PARAMS
	hinic3_uninit_devlink(hwdev);

init_devlink_err:
#endif
	hinic3_uninit_comm_ch(hwdev);

init_comm_ch_err:
	free_cfg_mgmt(hwdev);

init_cfg_mgmt_err:
	hinic3_destroy_heartbeat_detect(hwdev);
	destroy_workqueue(hwdev->workq);

alloc_workq_err:
	hisdk3_deinit_profile_adapter(hwdev);

	hinic3_free_hwif(hwdev);

init_hwif_err:
	spin_lock_deinit(&hwdev->channel_lock);
	vfree(hwdev->chip_fault_stats);
	para->probe_fault_level = hwdev->probe_fault_level;
	kfree(hwdev);
	*para->hwdev = NULL;

	return -EFAULT;
}

void hinic3_free_hwdev(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;
	u64 drv_features[COMM_MAX_FEATURE_QWORD];

	memset(drv_features, 0, sizeof(drv_features));
	hinic3_set_comm_features(hwdev, drv_features, COMM_MAX_FEATURE_QWORD);

	hinic3_free_ppf_work(dev);

	hinic3_multi_host_mgmt_free(dev);

	hinic3_func_rx_tx_flush(hwdev, HINIC3_CHANNEL_COMM, true);

	free_capability(dev);

#ifdef HAVE_DEVLINK_FLASH_UPDATE_PARAMS
	hinic3_uninit_devlink(dev);
#endif

	hinic3_uninit_comm_ch(dev);

	free_cfg_mgmt(dev);
	hinic3_destroy_heartbeat_detect(hwdev);
	destroy_workqueue(dev->workq);

	hisdk3_deinit_profile_adapter(hwdev);
	hinic3_free_hwif(dev);

	spin_lock_deinit(&dev->channel_lock);
	vfree(dev->chip_fault_stats);

	kfree(dev);
}

void *hinic3_get_pcidev_hdl(void *hwdev)
{
	struct hinic3_hwdev *dev = (struct hinic3_hwdev *)hwdev;

	if (!hwdev)
		return NULL;

	return dev->pcidev_hdl;
}

int hinic3_register_service_adapter(void *hwdev, void *service_adapter,
				    enum hinic3_service_type type)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!hwdev || !service_adapter || type >= SERVICE_T_MAX)
		return -EINVAL;

	if (dev->service_adapter[type])
		return -EINVAL;

	dev->service_adapter[type] = service_adapter;

	return 0;
}
EXPORT_SYMBOL(hinic3_register_service_adapter);

void hinic3_unregister_service_adapter(void *hwdev,
				       enum hinic3_service_type type)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!hwdev || type >= SERVICE_T_MAX)
		return;

	dev->service_adapter[type] = NULL;
}
EXPORT_SYMBOL(hinic3_unregister_service_adapter);

void *hinic3_get_service_adapter(void *hwdev, enum hinic3_service_type type)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!hwdev || type >= SERVICE_T_MAX)
		return NULL;

	return dev->service_adapter[type];
}
EXPORT_SYMBOL(hinic3_get_service_adapter);

int hinic3_dbg_get_hw_stats(const void *hwdev, u8 *hw_stats, const u32 *out_size)
{
	struct hinic3_hw_stats *tmp_hw_stats = (struct hinic3_hw_stats *)hw_stats;
	struct card_node *chip_node = NULL;

	if (!hwdev)
		return -EINVAL;

	if (*out_size != sizeof(struct hinic3_hw_stats) || !hw_stats) {
		pr_err("Unexpect out buf size from user :%u, expect: %lu\n",
		       *out_size, sizeof(struct hinic3_hw_stats));
		return -EFAULT;
	}

	memcpy(hw_stats,
	       &((struct hinic3_hwdev *)hwdev)->hw_stats, sizeof(struct hinic3_hw_stats));

	chip_node = ((struct hinic3_hwdev *)hwdev)->chip_node;

	atomic_set(&tmp_hw_stats->nic_ucode_event_stats[HINIC3_CHANNEL_BUSY],
		   atomic_read(&chip_node->channel_busy_cnt));

	return 0;
}

u16 hinic3_dbg_clear_hw_stats(void *hwdev)
{
	struct card_node *chip_node = NULL;
	struct hinic3_hwdev *dev = hwdev;

	memset((void *)&dev->hw_stats, 0, sizeof(struct hinic3_hw_stats));
	memset((void *)dev->chip_fault_stats, 0, HINIC3_CHIP_FAULT_SIZE);

	chip_node = dev->chip_node;
	if (COMM_SUPPORT_CHANNEL_DETECT(dev) && atomic_read(&chip_node->channel_busy_cnt)) {
		atomic_set(&chip_node->channel_busy_cnt, 0);
		dev->aeq_busy_cnt = 0;
		queue_delayed_work(dev->workq, &dev->channel_detect_task,
				   msecs_to_jiffies(HINIC3_CHANNEL_DETECT_PERIOD));
	}

	return sizeof(struct hinic3_hw_stats);
}

void hinic3_get_chip_fault_stats(const void *hwdev, u8 *chip_fault_stats,
				 u32 offset)
{
	if (offset >= HINIC3_CHIP_FAULT_SIZE) {
		pr_err("Invalid chip offset value: %d\n", offset);
		return;
	}

	if (offset + MAX_DRV_BUF_SIZE <= HINIC3_CHIP_FAULT_SIZE)
		memcpy(chip_fault_stats,
		       ((struct hinic3_hwdev *)hwdev)->chip_fault_stats
		       + offset, MAX_DRV_BUF_SIZE);
	else
		memcpy(chip_fault_stats,
		       ((struct hinic3_hwdev *)hwdev)->chip_fault_stats
		       + offset, HINIC3_CHIP_FAULT_SIZE - offset);
}

void hinic3_event_register(void *dev, void *pri_handle,
			   hinic3_event_handler callback)
{
	struct hinic3_hwdev *hwdev = dev;

	if (!dev) {
		pr_err("Hwdev pointer is NULL for register event\n");
		return;
	}

	hwdev->event_callback = callback;
	hwdev->event_pri_handle = pri_handle;
}

void hinic3_event_unregister(void *dev)
{
	struct hinic3_hwdev *hwdev = dev;

	if (!dev) {
		pr_err("Hwdev pointer is NULL for register event\n");
		return;
	}

	hwdev->event_callback = NULL;
	hwdev->event_pri_handle = NULL;
}

void hinic3_event_callback(void *hwdev, struct hinic3_event_info *event)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!hwdev) {
		pr_err("Hwdev pointer is NULL for event callback\n");
		return;
	}

	if (!dev->event_callback) {
		sdk_info(dev->dev_hdl, "Event callback function not register\n");
		return;
	}

	dev->event_callback(dev->event_pri_handle, event);
}
EXPORT_SYMBOL(hinic3_event_callback);

void hinic3_set_pcie_order_cfg(void *handle)
{
}

void hinic3_disable_mgmt_msg_report(void *hwdev)
{
	struct hinic3_hwdev *hw_dev = (struct hinic3_hwdev *)hwdev;

	hinic3_set_pf_status(hw_dev->hwif, HINIC3_PF_STATUS_INIT);
}

void hinic3_record_pcie_error(void *hwdev)
{
	struct hinic3_hwdev *dev = (struct hinic3_hwdev *)hwdev;

	if (!hwdev)
		return;

	atomic_inc(&dev->hw_stats.fault_event_stats.pcie_fault_stats);
}

bool hinic3_need_init_stateful_default(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;
	u16 chip_svc_type = dev->cfg_mgmt->svc_cap.svc_type;

	/* Current virtio net have to init cqm in PPF. */
	if (hinic3_func_type(hwdev) == TYPE_PPF && (chip_svc_type & CFG_SERVICE_MASK_VIRTIO) != 0)
		return true;

	/* vroce have to init cqm */
	if (IS_MASTER_HOST(dev) &&
	    (hinic3_func_type(hwdev) != TYPE_PPF) &&
	    ((chip_svc_type & CFG_SERVICE_MASK_ROCE) != 0))
		return true;

	/* SDI5.1 vm mode nano os PF0 as ppf needs to do stateful init else mailbox will fail */
	if (hinic3_func_type(hwdev) == TYPE_PPF && hinic3_is_vm_slave_host(hwdev))
		return true;

	/* Other service type will init cqm when uld call. */
	return false;
}

static inline void stateful_uninit(struct hinic3_hwdev *hwdev)
{
	u32 stateful_en;

	cqm_uninit(hwdev);

	stateful_en = IS_FT_TYPE(hwdev) | IS_RDMA_TYPE(hwdev);
	if (stateful_en)
		hinic3_ppf_ext_db_deinit(hwdev);
}

int hinic3_stateful_init(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;
	int stateful_en;
	int err;

	if (!dev)
		return -EINVAL;

	if (!hinic3_get_stateful_enable(dev))
		return 0;

	mutex_lock(&dev->stateful_mutex);
	if (dev->stateful_ref_cnt++) {
		mutex_unlock(&dev->stateful_mutex);
		return 0;
	}

	stateful_en = (int)(IS_FT_TYPE(dev) | IS_RDMA_TYPE(dev));
	if (stateful_en != 0 && HINIC3_IS_PPF(dev)) {
		err = hinic3_ppf_ext_db_init(dev);
		if (err != 0)
			goto out;
	}

	err = cqm_init(dev);
	if (err != 0) {
		sdk_err(dev->dev_hdl, "Failed to init cqm, err: %d\n", err);
		goto init_cqm_err;
	}

	mutex_unlock(&dev->stateful_mutex);
	sdk_info(dev->dev_hdl, "Initialize stateful resource success\n");

	return 0;

init_cqm_err:
	if (stateful_en != 0)
		hinic3_ppf_ext_db_deinit(dev);

out:
	dev->stateful_ref_cnt--;
	mutex_unlock(&dev->stateful_mutex);

	return err;
}
EXPORT_SYMBOL(hinic3_stateful_init);

void hinic3_stateful_deinit(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!dev || !hinic3_get_stateful_enable(dev))
		return;

	mutex_lock(&dev->stateful_mutex);
	if (!dev->stateful_ref_cnt || --dev->stateful_ref_cnt) {
		mutex_unlock(&dev->stateful_mutex);
		return;
	}

	stateful_uninit(hwdev);
	mutex_unlock(&dev->stateful_mutex);

	sdk_info(dev->dev_hdl, "Clear stateful resource success\n");
}
EXPORT_SYMBOL(hinic3_stateful_deinit);

void hinic3_free_stateful(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!dev || !hinic3_get_stateful_enable(dev) || !dev->stateful_ref_cnt)
		return;

	if (!hinic3_need_init_stateful_default(hwdev) || dev->stateful_ref_cnt > 1)
		sdk_info(dev->dev_hdl, "Current stateful resource ref is incorrect, ref_cnt:%u\n",
			 dev->stateful_ref_cnt);

	stateful_uninit(hwdev);

	sdk_info(dev->dev_hdl, "Clear stateful resource success\n");
}

int hinic3_get_card_present_state(void *hwdev, bool *card_present_state)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!hwdev || !card_present_state)
		return -EINVAL;

	*card_present_state = get_card_present_state(dev);

	return 0;
}
EXPORT_SYMBOL(hinic3_get_card_present_state);

void hinic3_link_event_stats(void *dev, u8 link)
{
	struct hinic3_hwdev *hwdev = dev;

	if (link)
		atomic_inc(&hwdev->hw_stats.link_event_stats.link_up_stats);
	else
		atomic_inc(&hwdev->hw_stats.link_event_stats.link_down_stats);
}
EXPORT_SYMBOL(hinic3_link_event_stats);

int hinic3_get_link_event_stats(void *dev, int *link_state)
{
	struct hinic3_hwdev *hwdev = dev;

	if (!hwdev || !link_state)
		return -EINVAL;

	*link_state = hwdev->hw_stats.link_event_stats.link_down_stats.counter;

	return 0;
}
EXPORT_SYMBOL(hinic3_get_link_event_stats);

u8 hinic3_max_pf_num(void *hwdev)
{
	if (!hwdev)
		return 0;

	return HINIC3_MAX_PF_NUM((struct hinic3_hwdev *)hwdev);
}
EXPORT_SYMBOL(hinic3_max_pf_num);

void hinic3_fault_event_report(void *hwdev, u16 src, u16 level)
{
	if (!hwdev)
		return;

	sdk_info(((struct hinic3_hwdev *)hwdev)->dev_hdl, "Fault event report, src: %u, level: %u\n",
		 src, level);

	hisdk3_fault_post_process(hwdev, src, level);
}
EXPORT_SYMBOL(hinic3_fault_event_report);

int hinic3_is_slave_func(const void *hwdev, bool *is_slave_func)
{
	if (!hwdev)
		return -EINVAL;

	*is_slave_func = IS_SLAVE_HOST((struct hinic3_hwdev *)hwdev);
	return 0;
}
EXPORT_SYMBOL(hinic3_is_slave_func);

int hinic3_is_master_func(const void *hwdev, bool *is_master_func)
{
	if (!hwdev)
		return -EINVAL;

	*is_master_func = IS_MASTER_HOST((struct hinic3_hwdev *)hwdev);
	return 0;
}
EXPORT_SYMBOL(hinic3_is_master_func);

void hinic3_probe_success(void *hwdev)
{
	if (!hwdev)
		return;

	hisdk3_probe_success(hwdev);
}

#define HINIC3_CHANNEL_BUSY_TIMEOUT	25

static void hinic3_update_channel_status(struct hinic3_hwdev *hwdev)
{
	struct card_node *chip_node = hwdev->chip_node;

	if (!chip_node)
		return;

	if (hinic3_func_type(hwdev) != TYPE_PPF || !COMM_SUPPORT_CHANNEL_DETECT(hwdev) ||
	    atomic_read(&chip_node->channel_busy_cnt))
		return;

	if (test_bit(HINIC3_HWDEV_MBOX_INITED, &hwdev->func_state)) {
		if (hwdev->last_recv_aeq_cnt != hwdev->cur_recv_aeq_cnt) {
			hwdev->aeq_busy_cnt = 0;
			hwdev->last_recv_aeq_cnt = hwdev->cur_recv_aeq_cnt;
		} else {
			hwdev->aeq_busy_cnt++;
		}

		if (hwdev->aeq_busy_cnt > HINIC3_CHANNEL_BUSY_TIMEOUT) {
			atomic_inc(&chip_node->channel_busy_cnt);
			sdk_err(hwdev->dev_hdl, "Detect channel busy\n");
		}
	}
}

static void hinic3_heartbeat_lost_handler(struct work_struct *work)
{
	struct hinic3_event_info event_info = { 0 };
	struct hinic3_hwdev *hwdev = container_of(work, struct hinic3_hwdev,
						  heartbeat_lost_work);
	u16 src, level;

	atomic_inc(&hwdev->hw_stats.heart_lost_stats);

	if (hwdev->event_callback) {
		event_info.service = EVENT_SRV_COMM;
		event_info.type =
			hwdev->pcie_link_down ? EVENT_COMM_PCIE_LINK_DOWN :
			EVENT_COMM_HEART_LOST;
		hwdev->event_callback(hwdev->event_pri_handle, &event_info);
	}

	if (hwdev->pcie_link_down) {
		src = HINIC3_FAULT_SRC_PCIE_LINK_DOWN;
		level = FAULT_LEVEL_HOST;
		sdk_err(hwdev->dev_hdl, "Detect pcie is link down\n");
	} else {
		src = HINIC3_FAULT_SRC_HOST_HEARTBEAT_LOST;
		level = FAULT_LEVEL_FATAL;
		sdk_err(hwdev->dev_hdl, "Heart lost report received, func_id: %d\n",
			hinic3_global_func_id(hwdev));
	}

	hinic3_show_chip_err_info(hwdev);

	hisdk3_fault_post_process(hwdev, src, level);
}

#define DETECT_PCIE_LINK_DOWN_RETRY		2
#define HINIC3_HEARTBEAT_START_EXPIRE		5000
#define HINIC3_HEARTBEAT_PERIOD			1000

static bool hinic3_is_hw_abnormal(struct hinic3_hwdev *hwdev)
{
	u32 status;

	if (!hinic3_get_chip_present_flag(hwdev))
		return false;

	status = hinic3_get_heartbeat_status(hwdev);
	if (status == HINIC3_PCIE_LINK_DOWN) {
		sdk_warn(hwdev->dev_hdl, "Detect BAR register read failed\n");
		hwdev->rd_bar_err_cnt++;
		if (hwdev->rd_bar_err_cnt >= DETECT_PCIE_LINK_DOWN_RETRY) {
			hinic3_set_chip_absent(hwdev);
			hinic3_force_complete_all(hwdev);
			hwdev->pcie_link_down = true;
			return true;
		}

		return false;
	}

	if (status) {
		hwdev->heartbeat_lost = true;
		return true;
	}

	hwdev->rd_bar_err_cnt = 0;

	return false;
}

#ifdef HAVE_TIMER_SETUP
static void hinic3_heartbeat_timer_handler(struct timer_list *t)
#else
static void hinic3_heartbeat_timer_handler(unsigned long data)
#endif
{
#ifdef HAVE_TIMER_SETUP
	struct hinic3_hwdev *hwdev = from_timer(hwdev, t, heartbeat_timer);
#else
	struct hinic3_hwdev *hwdev = (struct hinic3_hwdev *)data;
#endif

	if (hinic3_is_hw_abnormal(hwdev)) {
		stop_timer(&hwdev->heartbeat_timer);
		queue_work(hwdev->workq, &hwdev->heartbeat_lost_work);
	} else {
		mod_timer(&hwdev->heartbeat_timer,
			  jiffies + msecs_to_jiffies(HINIC3_HEARTBEAT_PERIOD));
	}

	hinic3_update_channel_status(hwdev);
}

static void hinic3_init_heartbeat_detect(struct hinic3_hwdev *hwdev)
{
#ifdef HAVE_TIMER_SETUP
	timer_setup(&hwdev->heartbeat_timer, hinic3_heartbeat_timer_handler, 0);
#else
	initialize_timer(hwdev->adapter_hdl, &hwdev->heartbeat_timer);
	hwdev->heartbeat_timer.data = (u64)hwdev;
	hwdev->heartbeat_timer.function = hinic3_heartbeat_timer_handler;
#endif

	hwdev->heartbeat_timer.expires =
		jiffies + msecs_to_jiffies(HINIC3_HEARTBEAT_START_EXPIRE);

	add_to_timer(&hwdev->heartbeat_timer, HINIC3_HEARTBEAT_PERIOD);

	INIT_WORK(&hwdev->heartbeat_lost_work, hinic3_heartbeat_lost_handler);
}

static void hinic3_destroy_heartbeat_detect(struct hinic3_hwdev *hwdev)
{
	destroy_work(&hwdev->heartbeat_lost_work);
	stop_timer(&hwdev->heartbeat_timer);
	delete_timer(&hwdev->heartbeat_timer);
}

void hinic3_set_api_stop(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!hwdev)
		return;

	dev->chip_present_flag = HINIC3_CHIP_ABSENT;
	sdk_info(dev->dev_hdl, "Set card absent\n");
	hinic3_force_complete_all(dev);
	sdk_info(dev->dev_hdl, "All messages interacting with the chip will stop\n");
}
