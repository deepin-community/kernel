// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include <linux/types.h>
#include <linux/spinlock.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_hwdev.h"
#include "sss_hwdev_api.h"
#include "sss_hwdev_mgmt_channel.h"
#include "sss_hwif_mbx.h"
#include "sss_hwif_mbx_init.h"
#include "sss_hwif_aeq.h"
#include "sss_hwif_export.h"
#include "sss_hwif_api.h"
#include "sss_hwif_adm_init.h"
#include "sss_hwif_mgmt_init.h"
#include "sss_hwif_ctrlq_init.h"
#include "sss_csr.h"

#define SSS_DRV_FEATURE_DEF		\
	(SSS_COMM_F_ADM | SSS_COMM_F_CLP | SSS_COMM_F_MBX_SEGMENT | \
	SSS_COMM_F_CTRLQ_NUM | SSS_COMM_F_VIRTIO_VQ_SIZE)

#define SSS_COMM_SUPPORT_CLP(hwdev)				\
	((hwdev)->features[0] & SSS_COMM_F_CLP)

#define SSS_DMA_ATTR_INDIR_ID_SHIFT				0
#define SSS_DMA_ATTR_INDIR_ID_MASK				0x3FF

#define SSS_SET_DMA_ATTR_INDIR_ID(val, member)			\
		(((u32)(val) & SSS_DMA_ATTR_INDIR_##member##_MASK) << \
			SSS_DMA_ATTR_INDIR_##member##_SHIFT)

#define SSS_CLEAR_DMA_ATTR_INDIR_ID(val, member)		\
		((val) & (~(SSS_DMA_ATTR_INDIR_##member##_MASK	\
			<< SSS_DMA_ATTR_INDIR_##member##_SHIFT)))

#define SSS_DMA_ATTR_ENTRY_ST_SHIFT				0
#define SSS_DMA_ATTR_ENTRY_AT_SHIFT				8
#define SSS_DMA_ATTR_ENTRY_PH_SHIFT				10
#define SSS_DMA_ATTR_ENTRY_NO_SNOOPING_SHIFT	12
#define SSS_DMA_ATTR_ENTRY_TPH_EN_SHIFT			13

#define SSS_DMA_ATTR_ENTRY_ST_MASK				0xFF
#define SSS_DMA_ATTR_ENTRY_AT_MASK				0x3
#define SSS_DMA_ATTR_ENTRY_PH_MASK				0x3
#define SSS_DMA_ATTR_ENTRY_NO_SNOOPING_MASK		0x1
#define SSS_DMA_ATTR_ENTRY_TPH_EN_MASK			0x1

#define SSS_SET_DMA_ATTR_ENTRY(val, member)			\
		(((u32)(val) & SSS_DMA_ATTR_ENTRY_##member##_MASK) << \
			SSS_DMA_ATTR_ENTRY_##member##_SHIFT)

#define SSS_PCIE_ST_DISABLE			0
#define SSS_PCIE_AT_DISABLE			0
#define SSS_PCIE_PH_DISABLE			0

#define SSS_PCIE_MSIX_ATTR_ENTRY	0

#define SSS_PCIE_SNOOP				0
#define SSS_PCIE_NO_SNOOP			1

#define SSS_PCIE_TPH_DISABLE		0
#define SSS_PCIE_TPH_ENABLE			1

#define SSS_FAULT_LEVEL_STR_FATAL			"fatal"
#define SSS_FAULT_LEVEL_STR_RESET			"reset"
#define SSS_FAULT_LEVEL_STR_HOST			"host"
#define SSS_FAULT_LEVEL_STR_FLR				"flr"
#define SSS_FAULT_LEVEL_STR_GENERAL			"general"
#define SSS_FAULT_LEVEL_STR_SUGGESTION		"suggestion"
#define SSS_FAULT_LEVEL_STR_UNKNOWN			"Unknown"

#define SSS_FAULT_TYPE_STR_CHIP				"chip"
#define SSS_FAULT_TYPE_STR_NPU				"ucode"
#define SSS_FAULT_TYPE_STR_MEM_RD			"mem rd timeout"
#define SSS_FAULT_TYPE_STR_MEM_WR			"mem wr timeout"
#define SSS_FAULT_TYPE_STR_REG_RD			"reg rd timeout"
#define SSS_FAULT_TYPE_STR_REG_WR			"reg wr timeout"
#define SSS_FAULT_TYPE_STR_PHY				"phy fault"
#define SSS_FAULT_TYPE_STR_TSENSOR			"tsensor fault"
#define SSS_FAULT_TYPE_STR_UNKNOWN			"Unknown"

#define SSS_COMM_RESET_TYPE \
	((1 << SSS_RESET_TYPE_COMM) | (1 << SSS_RESET_TYPE_COMM_CMD_CH) | \
	(1 << SSS_RESET_TYPE_FLUSH_BIT) | (1 << SSS_RESET_TYPE_MQM) | \
	(1 << SSS_RESET_TYPE_SMF) | (1 << SSS_RESET_TYPE_PF_BW_CFG))

#define SSS_FOUR_REG_LEN		16

#define SSS_X_CSR_INDEX			30
#define SSS_DUMP_16B_PER_LINE	16
#define SSS_DUMP_4_VAR_PER_LINE	4

typedef void (*sss_print_err_handler_t)(struct sss_hwdev *hwdev,
					struct sss_fault_event *fault_event);

typedef void (*sss_mgmt_event_handler_t)(void *data, void *in_buf, u16 in_size,
		void *out_buf, u16 *out_size);

struct sss_mgmt_event {
	u16 event_type;
	sss_mgmt_event_handler_t handler;
};

static void sss_fault_event_handler(void *data, void *in_buf, u16 in_size,
				    void *out_buf, u16 *out_size);

static void sss_show_watchdog_mgmt_register_info(struct sss_hwdev *hwdev,
						 struct sss_watchdog_info *watchdog_info)
{
	u32 i;
	u64 *reg = NULL;

	sdk_err(hwdev->dev_hdl, "Mgmt deadloop time: 0x%x 0x%x, task id: 0x%x, sp: 0x%llx\n",
		watchdog_info->cur_time_h, watchdog_info->cur_time_l,
		watchdog_info->task_id, watchdog_info->sp);

	sdk_err(hwdev->dev_hdl,
		"Stack current used: 0x%x, peak used: 0x%x, overflow flag: 0x%x, top: 0x%llx, bottom: 0x%llx\n",
		watchdog_info->cur_used, watchdog_info->peak_used,
		watchdog_info->is_overflow, watchdog_info->stack_top, watchdog_info->stack_bottom);

	sdk_err(hwdev->dev_hdl, "Mgmt pc: 0x%llx, elr: 0x%llx, spsr: 0x%llx, far: 0x%llx, esr: 0x%llx, xzr: 0x%llx\n",
		watchdog_info->pc, watchdog_info->elr, watchdog_info->spsr, watchdog_info->far,
		watchdog_info->esr, watchdog_info->xzr);

	sdk_err(hwdev->dev_hdl, "Mgmt register info\n");

	reg = &watchdog_info->x30;
	for (i = 0; i <= SSS_X_CSR_INDEX; i++)
		sdk_err(hwdev->dev_hdl, "x%02u:0x%llx\n",
			SSS_X_CSR_INDEX - i, reg[i]);
}

static void sss_show_watchdog_stack_info(struct sss_hwdev *hwdev,
					 struct sss_watchdog_info *watchdog_info)
{
	u32 i;
	u32 j;
	u32 tmp;
	u32 stack_len;
	u32 *dump_addr = NULL;

	if (watchdog_info->stack_actlen <= SSS_STACK_DATA_LEN) {
		stack_len = watchdog_info->stack_actlen;
	} else {
		sdk_err(hwdev->dev_hdl, "Oops stack length: 0x%x is wrong\n",
			watchdog_info->stack_actlen);
		stack_len = SSS_STACK_DATA_LEN;
	}

	sdk_err(hwdev->dev_hdl, "Mgmt dump stack, 16 bytes per line(start from sp)\n");
	for (i = 0; i < (stack_len / SSS_DUMP_16B_PER_LINE); i++) {
		dump_addr = (u32 *)(watchdog_info->stack_data + (u32)(i * SSS_DUMP_16B_PER_LINE));
		sdk_err(hwdev->dev_hdl, "0x%08x 0x%08x 0x%08x 0x%08x\n",
			*dump_addr, *(dump_addr + 0x1), *(dump_addr + 0x2), *(dump_addr + 0x3));
	}

	tmp = (stack_len % SSS_DUMP_16B_PER_LINE) / SSS_DUMP_4_VAR_PER_LINE;
	for (j = 0; j < tmp; j++) {
		dump_addr = (u32 *)(watchdog_info->stack_data +
				    (u32)(i * SSS_DUMP_16B_PER_LINE + j * SSS_DUMP_4_VAR_PER_LINE));
		sdk_err(hwdev->dev_hdl, "0x%08x ", *dump_addr);
	}
}

static void sss_show_watchdog_timeout_info(struct sss_hwdev *hwdev,
					   void *buf_in, u16 in_size, void *buf_out, u16 *out_size)
{
	struct sss_watchdog_info *watchdog_info = buf_in;

	if (in_size != sizeof(*watchdog_info)) {
		sdk_err(hwdev->dev_hdl, "Invalid mgmt watchdog report, length: %d, should be %ld\n",
			in_size, sizeof(*watchdog_info));
		return;
	}

	sss_show_watchdog_mgmt_register_info(hwdev, watchdog_info);
	sss_show_watchdog_stack_info(hwdev, watchdog_info);

	*out_size = sizeof(*watchdog_info);
	watchdog_info = buf_out;
	watchdog_info->head.state = 0;
}

static void sss_watchdog_timeout_event_handler(void *hwdev,
					       void *buf_in, u16 in_size,
					       void *buf_out, u16 *out_size)
{
	struct sss_event_info event_info = {0};
	struct sss_hwdev *dev = hwdev;

	sss_show_watchdog_timeout_info(dev, buf_in, in_size, buf_out, out_size);

	if (dev->event_handler) {
		event_info.type = SSS_EVENT_MGMT_WATCHDOG;
		dev->event_handler(dev->event_handler_data, &event_info);
	}
}

static void sss_show_exc_info(struct sss_hwdev *hwdev, struct sss_exc_info *exc_info)
{
	u32 i;

	/* key information */
	sdk_err(hwdev->dev_hdl, "==================== Exception Info Begin ====================\n");
	sdk_err(hwdev->dev_hdl, "Exception CpuTick       : 0x%08x 0x%08x\n",
		exc_info->cpu_tick.tick_cnt_h, exc_info->cpu_tick.tick_cnt_l);
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

	for (i = 0; i < SSS_XREGS_NUM - 1; i += 0x2)
		sdk_err(hwdev->dev_hdl, "XREGS[%02u]%-5s: 0x%016llx \t XREGS[%02u]%-5s: 0x%016llx",
			i, " ", exc_info->reg_info.xregs[i],
			(u32)(i + 0x1U), " ", exc_info->reg_info.xregs[(u32)(i + 0x1U)]);

	sdk_err(hwdev->dev_hdl, "XREGS[%02u]%-5s: 0x%016llx \t ", SSS_XREGS_NUM - 1, " ",
		exc_info->reg_info.xregs[SSS_XREGS_NUM - 1]);
}

static void sss_lastword_report_event_handler(void *hwdev,
					      void *buf_in, u16 in_size,
					      void *buf_out, u16 *out_size)
{
	struct sss_lastword_info *lastword_info = buf_in;
	struct sss_exc_info *exc_info = &lastword_info->stack_info;
	u32 stack_len = lastword_info->stack_actlen;
	struct sss_hwdev *dev = hwdev;
	u32 *curr_reg = NULL;
	u32 reg_i;
	u32 cnt;

	if (in_size != sizeof(*lastword_info)) {
		sdk_err(dev->dev_hdl, "Invalid mgmt lastword, length: %u, should be %ld\n",
			in_size, sizeof(*lastword_info));
		return;
	}

	sss_show_exc_info(dev, exc_info);

	/* call stack dump */
	sdk_err(dev->dev_hdl, "Dump stack when exceptioin occurs, 16Bytes per line.\n");

	cnt = stack_len / SSS_FOUR_REG_LEN;
	for (reg_i = 0; reg_i < cnt; reg_i++) {
		curr_reg = (u32 *)(lastword_info->stack_data +
				((u64)(u32)(reg_i * SSS_FOUR_REG_LEN)));
		sdk_err(dev->dev_hdl, "0x%08x 0x%08x 0x%08x 0x%08x\n",
			*curr_reg, *(curr_reg + 0x1), *(curr_reg + 0x2), *(curr_reg + 0x3));
	}

	sdk_err(dev->dev_hdl, "==================== Exception Info End ====================\n");
}

const struct sss_mgmt_event g_mgmt_event_handler[] = {
	{
		.event_type = SSS_COMM_MGMT_CMD_FAULT_REPORT,
		.handler = sss_fault_event_handler,
	},

	{
		.event_type	= SSS_COMM_MGMT_CMD_WATCHDOG_INFO,
		.handler = sss_watchdog_timeout_event_handler,
	},

	{
		.event_type	= SSS_COMM_MGMT_CMD_LASTWORD_GET,
		.handler = sss_lastword_report_event_handler,
	},
};

static void sss_print_chip_fault(struct sss_hwdev *hwdev,
				 struct sss_fault_event *fault_event)
{
	u8 err_level;
	char *level_str = NULL;
	char *fault_level[SSS_FAULT_LEVEL_MAX] = {
		SSS_FAULT_LEVEL_STR_FATAL, SSS_FAULT_LEVEL_STR_RESET,
		SSS_FAULT_LEVEL_STR_HOST, SSS_FAULT_LEVEL_STR_FLR,
		SSS_FAULT_LEVEL_STR_GENERAL, SSS_FAULT_LEVEL_STR_SUGGESTION
	};

	err_level = fault_event->info.chip.err_level;
	if (err_level < SSS_FAULT_LEVEL_MAX)
		level_str = fault_level[err_level];
	else
		level_str = SSS_FAULT_LEVEL_STR_UNKNOWN;

	if (err_level == SSS_FAULT_LEVEL_SERIOUS_FLR)
		sdk_err(hwdev->dev_hdl, "Err_level: %u [%s], func_id: %u\n",
			err_level, level_str, fault_event->info.chip.func_id);

	sdk_err(hwdev->dev_hdl, "Node_id: 0x%x, err_type: 0x%x, err_level: %u[%s], err_csr_addr: 0x%08x, err_csr_value: 0x%08x\n",
		fault_event->info.chip.node_id, fault_event->info.chip.err_type,
		err_level, level_str,
		fault_event->info.chip.err_csr_addr, fault_event->info.chip.err_csr_value);
}

static void sss_print_ucode_err(struct sss_hwdev *hwdev,
				struct sss_fault_event *fault_event)
{
	sdk_err(hwdev->dev_hdl, "Cause_id: %u, core_id: %u, c_id: %u, epc: 0x%08x\n",
		fault_event->info.ucode.cause_id, fault_event->info.ucode.core_id,
		fault_event->info.ucode.c_id, fault_event->info.ucode.epc);
}

static void sss_print_mem_rw_err(struct sss_hwdev *hwdev,
				 struct sss_fault_event *fault_event)
{
	sdk_err(hwdev->dev_hdl, "Err_csr_ctrl: 0x%08x, err_csr_data: 0x%08x, ctrl_tab: 0x%08x, mem_id: 0x%08x\n",
		fault_event->info.mem_timeout.err_csr_ctrl,
		fault_event->info.mem_timeout.err_csr_data,
		fault_event->info.mem_timeout.ctrl_tab, fault_event->info.mem_timeout.mem_id);
}

static void sss_print_reg_rw_err(struct sss_hwdev *hwdev,
				 struct sss_fault_event *fault_event)
{
	sdk_err(hwdev->dev_hdl, "Err_csr: 0x%08x\n", fault_event->info.reg_timeout.err_csr);
}

static void sss_print_phy_err(struct sss_hwdev *hwdev,
			      struct sss_fault_event *fault_event)
{
	sdk_err(hwdev->dev_hdl, "Op_type: %u, port_id: %u, dev_ad: %u, csr_addr: 0x%08x, op_data: 0x%08x\n",
		fault_event->info.phy_fault.op_type, fault_event->info.phy_fault.port_id,
		fault_event->info.phy_fault.dev_ad, fault_event->info.phy_fault.csr_addr,
		fault_event->info.phy_fault.op_data);
}

static void sss_print_fault_info(struct sss_hwdev *hwdev,
				 struct sss_fault_event *fault_event)
{
	struct sss_fault_event_stats *event_stats = &hwdev->hw_stats.fault_event_stats;
	char *type = NULL;
	char *fault_type[SSS_FAULT_TYPE_MAX] = {
		SSS_FAULT_TYPE_STR_CHIP, SSS_FAULT_TYPE_STR_NPU,
		SSS_FAULT_TYPE_STR_MEM_RD, SSS_FAULT_TYPE_STR_MEM_WR,
		SSS_FAULT_TYPE_STR_REG_RD, SSS_FAULT_TYPE_STR_REG_WR,
		SSS_FAULT_TYPE_STR_PHY, SSS_FAULT_TYPE_STR_TSENSOR
	};
	sss_print_err_handler_t print_handler[] = {
		sss_print_chip_fault, sss_print_ucode_err,
		sss_print_mem_rw_err, sss_print_mem_rw_err,
		sss_print_reg_rw_err, sss_print_reg_rw_err,
		sss_print_phy_err
	};

	if (fault_event->type < SSS_FAULT_TYPE_MAX) {
		type = fault_type[fault_event->type];
		atomic_inc(&event_stats->fault_type_stat[fault_event->type]);
	} else {
		type = SSS_FAULT_TYPE_STR_UNKNOWN;
	}

	sdk_err(hwdev->dev_hdl, "Fault event report received, func_id: %u\n",
		sss_get_global_func_id(hwdev));
	sdk_err(hwdev->dev_hdl, "Fault type: %u [%s]\n", fault_event->type, type);
	sdk_err(hwdev->dev_hdl, "Fault val[0]: 0x%08x, val[1]: 0x%08x, val[2]: 0x%08x, val[3]: 0x%08x\n",
		fault_event->info.val[0x0], fault_event->info.val[0x1],
		fault_event->info.val[0x2], fault_event->info.val[0x3]);

	sss_dump_chip_err_info(hwdev);

	if (fault_event->type >= ARRAY_LEN(print_handler))
		return;

	print_handler[fault_event->type](hwdev, fault_event);
}

static void sss_fault_event_handler(void *data, void *in_buf, u16 in_size,
				    void *out_buf, u16 *out_size)
{
	struct sss_hwdev *hwdev = data;
	struct sss_cmd_fault_event *cmd_event = in_buf;
	struct sss_event_info info;
	struct sss_fault_event *fault_event = (void *)info.event_data;

	if (in_size != sizeof(*cmd_event)) {
		sdk_err(hwdev->dev_hdl, "Invalid size: %u.\n", in_size);
		return;
	}

	sss_print_fault_info(hwdev, &cmd_event->fault_event);

	if (hwdev->event_handler) {
		info.type = SSS_EVENT_FAULT;
		info.service = SSS_EVENT_SRV_COMM;
		memcpy(info.event_data, &cmd_event->fault_event, sizeof(cmd_event->fault_event));
		fault_event->fault_level = (cmd_event->fault_event.type == SSS_FAULT_TYPE_CHIP) ?
					   cmd_event->fault_event.info.chip.err_level :
					   SSS_FAULT_LEVEL_FATAL;
		hwdev->event_handler(hwdev->event_handler_data, &info);
	}
}

static void sss_pf_handle_mgmt_event(void *data, u16 event_type,
				     void *in_buf, u16 in_size, void *out_buf, u16 *out_size)
{
	u32 i;
	u32 num = ARRAY_LEN(g_mgmt_event_handler);

	for (i = 0; i < num; i++) {
		if (event_type == g_mgmt_event_handler[i].event_type &&
		    g_mgmt_event_handler[i].handler) {
			g_mgmt_event_handler[i].handler(data, in_buf, in_size,
							out_buf, out_size);
			return;
		}
	}

	*out_size = sizeof(struct sss_mgmt_msg_head);
	((struct sss_mgmt_msg_head *)out_buf)->state = SSS_MGMT_CMD_UNSUPPORTED;
	sdk_warn(SSS_TO_DEV(data), "Unsupported mgmt event %u.\n", event_type);
}

static int sss_hwdev_init_mbx(struct sss_hwdev *hwdev)
{
	int ret;

	ret = sss_hwif_init_mbx(hwdev);
	if (ret != 0)
		return ret;

	sss_aeq_register_hw_cb(hwdev, hwdev, SSS_MBX_FROM_FUNC, sss_recv_mbx_aeq_handler);
	sss_aeq_register_hw_cb(hwdev, hwdev, SSS_MSG_FROM_MGMT, sss_mgmt_msg_aeqe_handler);

	set_bit(SSS_HW_MBX_INIT_OK, &hwdev->func_state);

	return 0;
}

static void sss_hwdev_deinit_mbx(struct sss_hwdev *hwdev)
{
	spin_lock_bh(&hwdev->channel_lock);
	clear_bit(SSS_HW_MBX_INIT_OK, &hwdev->func_state);
	spin_unlock_bh(&hwdev->channel_lock);

	sss_aeq_unregister_hw_cb(hwdev, SSS_MBX_FROM_FUNC);

	if (!SSS_IS_VF(hwdev)) {
		sss_unregister_pf_mbx_handler(hwdev, SSS_MOD_TYPE_COMM);
	} else {
		sss_unregister_vf_mbx_handler(hwdev, SSS_MOD_TYPE_COMM);

		sss_aeq_unregister_hw_cb(hwdev, SSS_MSG_FROM_MGMT);
	}

	sss_hwif_deinit_mbx(hwdev);
}

static int sss_chip_get_global_attr(struct sss_hwdev *hwdev)
{
	int ret = 0;
	struct sss_cmd_get_glb_attr attr_cmd = {0};
	u16 out_len = sizeof(attr_cmd);

	ret = sss_sync_send_msg(hwdev, SSS_COMM_MGMT_CMD_GET_GLOBAL_ATTR,
				&attr_cmd, sizeof(attr_cmd), &attr_cmd, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &attr_cmd)) {
		sdk_err(((struct sss_hwdev *)hwdev)->dev_hdl,
			"Fail to get global attr, ret: %d, status: 0x%x, out_len: 0x%x\n",
			ret, attr_cmd.head.state, out_len);
		return -EIO;
	}

	memcpy(&hwdev->glb_attr, &attr_cmd.attr, sizeof(hwdev->glb_attr));

	return 0;
}

static int sss_chip_get_feature(struct sss_hwdev *hwdev)
{
	int i;
	int ret;
	u64 feature[SSS_MAX_FEATURE_QWORD] = {SSS_DRV_FEATURE_DEF, 0, 0, 0};

	ret = sss_chip_do_nego_feature(hwdev, SSS_MGMT_MSG_GET_CMD,
				       hwdev->features, SSS_MAX_FEATURE_QWORD);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to get comm feature\n");
		return ret;
	}

	if (sss_get_func_type(hwdev) == SSS_FUNC_TYPE_PPF)
		feature[0] |= SSS_COMM_F_CHANNEL_DETECT;

	for (i = 0; i < SSS_MAX_FEATURE_QWORD; i++)
		hwdev->features[i] &= feature[i];

	return 0;
}

static int sss_get_global_info(struct sss_hwdev *hwdev)
{
	int ret;

	ret = sss_chip_get_board_info(hwdev, &hwdev->board_info);
	if (ret != 0)
		return ret;

	ret = sss_chip_get_feature(hwdev);
	if (ret != 0)
		return ret;

	ret = sss_chip_get_global_attr(hwdev);
	if (ret != 0)
		return ret;

	return 0;
}

static void sss_hwdev_deinit_adm(struct sss_hwdev *hwdev)
{
	if (sss_get_func_type(hwdev) == SSS_FUNC_TYPE_VF)
		return;

	spin_lock_bh(&hwdev->channel_lock);
	clear_bit(SSS_HW_ADM_INIT_OK, &hwdev->func_state);
	spin_unlock_bh(&hwdev->channel_lock);

	sss_unregister_mgmt_msg_handler(hwdev, SSS_MOD_TYPE_COMM);

	sss_aeq_unregister_hw_cb(hwdev, SSS_MSG_FROM_MGMT);

	sss_hwif_deinit_adm(hwdev);
}

static int sss_hwdev_init_adm(struct sss_hwdev *hwdev)
{
	int ret;

	if (sss_get_func_type(hwdev) == SSS_FUNC_TYPE_VF)
		return 0;

	ret = sss_hwif_init_adm(hwdev);
	if (ret != 0)
		return ret;

	sss_register_mgmt_msg_handler(hwdev, SSS_MOD_TYPE_COMM, hwdev,
				      sss_pf_handle_mgmt_event);

	set_bit(SSS_HW_ADM_INIT_OK, &hwdev->func_state);

	return 0;
}

static int sss_chip_set_dma_attr_table(struct sss_hwdev *hwdev)
{
	int ret;
	struct sss_cmd_dma_attr_config attr = {0};
	u16 out_len = sizeof(attr);

	attr.ph = SSS_PCIE_PH_DISABLE;
	attr.at = SSS_PCIE_AT_DISABLE;
	attr.st = SSS_PCIE_ST_DISABLE;
	attr.no_snooping = SSS_PCIE_SNOOP;
	attr.tph_en = SSS_PCIE_TPH_DISABLE;
	attr.func_id = sss_get_global_func_id(hwdev);
	attr.entry_id = SSS_PCIE_MSIX_ATTR_ENTRY;

	ret = sss_sync_send_msg(hwdev, SSS_COMM_MGMT_CMD_SET_DMA_ATTR, &attr, sizeof(attr),
				&attr, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &attr)) {
		sdk_err(hwdev->dev_hdl,
			"Fail to set dma attr, ret: %d, status: 0x%x, out_len: 0x%x\n",
			ret, attr.head.state, out_len);
		return -EIO;
	}

	return 0;
}

static int sss_chip_init_dma_attr(struct sss_hwdev *hwdev)
{
	u32 set;
	u32 get;
	u32 dst;

	set = sss_chip_read_reg(hwdev->hwif, SSS_CSR_DMA_ATTR_INDIR_ID_ADDR);
	set = SSS_CLEAR_DMA_ATTR_INDIR_ID(set, ID);
	set |= SSS_SET_DMA_ATTR_INDIR_ID(SSS_PCIE_MSIX_ATTR_ENTRY, ID);

	sss_chip_write_reg(hwdev->hwif, SSS_CSR_DMA_ATTR_INDIR_ID_ADDR, set);

	/* make sure reset dma attr */
	wmb();

	dst = SSS_SET_DMA_ATTR_ENTRY(SSS_PCIE_TPH_DISABLE, TPH_EN) |
	      SSS_SET_DMA_ATTR_ENTRY(SSS_PCIE_SNOOP, NO_SNOOPING) |
	      SSS_SET_DMA_ATTR_ENTRY(SSS_PCIE_ST_DISABLE, ST) |
	      SSS_SET_DMA_ATTR_ENTRY(SSS_PCIE_AT_DISABLE, AT) |
	      SSS_SET_DMA_ATTR_ENTRY(SSS_PCIE_PH_DISABLE, PH);
	get = sss_chip_read_reg(hwdev->hwif, SSS_CSR_DMA_ATTR_TBL_ADDR);

	if (get == dst)
		return 0;

	return sss_chip_set_dma_attr_table(hwdev);
}

static void sss_chip_set_pf_state(struct sss_hwdev *hwdev)
{
	sss_chip_set_pf_status(hwdev->hwif, SSS_PF_STATUS_ACTIVE_FLAG);
}

static void sss_chip_reset_pf_state(struct sss_hwdev *hwdev)
{
	sss_chip_set_pf_status(hwdev->hwif, SSS_PF_STATUS_INIT);
}

static int sss_init_basic_mgmt_channel(struct sss_hwdev *hwdev)
{
	int ret;

	ret = sss_hwif_init_aeq(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init comm aeqs\n");
		return ret;
	}

	ret = sss_hwdev_init_mbx(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init mbx\n");
		goto init_mbx_err;
	}

	ret = sss_init_aeq_msix_attr(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init aeqs msix attr\n");
		goto init_aeq_msix_attr_err;
	}

	return 0;

init_aeq_msix_attr_err:
	sss_hwdev_deinit_mbx(hwdev);

init_mbx_err:
	sss_hwif_deinit_aeq(hwdev);

	return ret;
}

static void sss_free_base_mgmt_channel(struct sss_hwdev *hwdev)
{
	sss_hwdev_deinit_mbx(hwdev);
	sss_hwif_deinit_aeq(hwdev);
}

int sss_init_mgmt_channel(struct sss_hwdev *hwdev)
{
	int ret;

	/* init aeq, mbx */
	ret = sss_init_basic_mgmt_channel(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init basic mgmt channel\n");
		return ret;
	}

	ret = sss_chip_reset_function(hwdev, sss_get_global_func_id(hwdev),
				      SSS_COMM_RESET_TYPE, SSS_CHANNEL_COMM);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to reset func\n");
		goto out;
	}

	ret = sss_get_global_info(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init hwdev attr\n");
		goto out;
	}

	ret = sss_hwdev_init_adm(hwdev);
	if (ret != 0)
		goto out;

	ret = sss_chip_set_func_used_state(hwdev, SSS_SVC_TYPE_COM,
					   true, SSS_CHANNEL_COMM);
	if (ret != 0)
		goto set_use_state_err;

	ret = sss_chip_init_dma_attr(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init dma attr table\n");
		goto init_dma_attr_err;
	}

	ret = sss_init_ctrlq_channel(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init ctrlq channel\n");
		goto init_ctrlq_channel_err;
	}

	sss_chip_set_pf_state(hwdev);

	ret = sss_aeq_register_swe_cb(hwdev, hwdev, SSS_STL_EVENT, sss_sw_aeqe_handler);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl,
			"Fail to register sw aeqe handler\n");
		goto register_ucode_aeqe_err;
	}

	return 0;

register_ucode_aeqe_err:
	sss_chip_reset_pf_state(hwdev);
	sss_deinit_ctrlq_channel(hwdev);

init_ctrlq_channel_err:
init_dma_attr_err:
	sss_chip_set_func_used_state(hwdev, SSS_SVC_TYPE_COM,
				     false, SSS_CHANNEL_COMM);

set_use_state_err:
	sss_hwdev_deinit_adm(hwdev);

out:
	sss_free_base_mgmt_channel(hwdev);

	return ret;
}

void sss_deinit_mgmt_channel(struct sss_hwdev *hwdev)
{
	sss_aeq_unregister_swe_cb(hwdev, SSS_STL_EVENT);

	sss_chip_reset_pf_state(hwdev);

	sss_deinit_ctrlq_channel(hwdev);

	sss_chip_set_func_used_state(hwdev, SSS_SVC_TYPE_COM,
				     false, SSS_CHANNEL_COMM);

	sss_hwdev_deinit_adm(hwdev);

	sss_free_base_mgmt_channel(hwdev);
}
