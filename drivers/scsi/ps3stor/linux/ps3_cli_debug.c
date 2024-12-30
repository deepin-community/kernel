#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/dmi.h>
#include <linux/limits.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/rtc.h>
#include <linux/errno.h>

#include "ps3_cli_debug.h"
#include "ps3_cli.h"
#include "ps3_driver_log.h"
#include "ps3_ioc_manager.h"
#include "ps3_ioc_state.h"
#include "ps3_event.h"
#include "ps3_scsi_cmd_err.h"
#include "ps3_cmd_statistics.h"
#include "ps3_scsih.h"
#include "ps3_util.h"
#include "ps3_dump.h"
#include "ps3_recovery.h"
#include "ps3_r1x_write_lock.h"
#include "ps3_irq.h"
#include "ps3_cmd_complete.h"
#include "ps3_qos.h"
#include "ps3_inject.h"
static void ps3_cli_host_and_instance_ls(int argc, char *argv[]);
static void ps3_cli_register_rw(int argc, char *argv[]);
static void ps3_cli_register_dump(int argc, char *argv[]);
static void ps3_cli_init_frame_dump(int argc, char *argv[]);
static void ps3_cli_event_subscirbe_info_dump(int argc, char *argv[]);
static void ps3_cli_dev_mgr_cli_base_dump(int argc, char *argv[]);
static void ps3_cli_dev_mgr_cli_detail_dump(int argc, char *argv[]);
static void ps3_cli_reply_fifo_dump(int argc, char *argv[]);
static void ps3_cli_cmd_dump(int argc, char *argv[]);
static void ps3_cli_event_delay_set(int argc, char *argv[]);
static void ps3_cli_crashdump_set(int argc, char *argv[]);
static void ps3_cli_force_to_stop(int argc, char *argv[]);
static void ps3_io_statis_dump_cli_cb(int argc, char *argv[]);
static void ps3_io_statis_clear_cli_cb(int argc, char *argv []);
static void ps3_debug_mem_rw_cli_cb(int argc, char *argv[]);
static void ps3_debug_mem_para_cli_cb(int argc, char *argv[]);
static void ps3_scsi_device_lookup_cb(int argc, char *argv[]);
static void ps3_hard_reset_cb(int argc, char *argv[]);
static void ps3_soc_halt_cb(int argc, char *argv[]);
static void ps3_cmd_stat_switch_store_cb(int argc, char *argv[]);
static void ps3_stat_total_show_cb(int argc, char *argv[]);
static void ps3_stat_buf_clr_cb(int argc, char *argv[]);
static void ps3_stat_interval_show_cb(int argc, char *argv[]);
static void ps3_stat_interval_store_cb(int argc, char *argv[]);
static void ps3_stat_inc_show_cb(int argc, char *argv[]);
static void ps3_cmd_stat_switch_show_cb(int argc, char *argv[]);
static void ps3_reply_fifo_reset_cb(int argc, char *argv[]);
static void ps3_cli_remove_host_force(int argc, char *argv[]);
static void ps3_hardreset_cnt_clear_cli_cb(int argc, char *argv[]);
static void ps3_hardreset_cnt_show_cli_cb(int argc, char *argv[]);
static void ps3_cli_ramfs_test_set(int argc, char *argv[]);
static void ps3_cli_err_inject_active(int argc, char *argv[]);
static void ps3_cli_err_inject_clear(int argc, char *argv[]);
static void ps3_no_wait_cli_cmd(int argc, char *argv[]);
static void ps3_cli_qos_info(int argc, char *argv[]);
static void	ps3_cli_special_log(int argc, char *argv[]);
static void ps3_cli_r1x_delay(int argc, char *argv[]);

static Bool ps3_cli_wait_flag = PS3_FALSE;
#ifdef PS3_SUPPORT_INJECT
static void ps3_show_inject_scsi_rw(int argc, char *argv[]);
static void ps3_show_inject_scsi_task(int argc, char *argv[]);
static void ps3_show_inject_mgr(int argc, char *argv[]);

static void ps3_clear_all_inject(int argc, char *argv[]);
static void ps3_clear_inject_scsi_task(int argc, char *argv[]);
static void ps3_clear_inject_mgr(int argc, char *argv[]);

static void ps3_clear_inject_scsi_rw(int argc, char *argv[]);
static void ps3_inject_scsi_task(int argc, char *argv[]);
static void ps3_inject_mgr(int argc, char *argv[]);
static void ps3_inject_scsi_rw(int argc, char *argv[]);
static void ps3_show_all_inject(int argc, char *argv[]);
static void ps3_clear_hit_cmd(int argc, char *argv[]);
#endif
#ifdef CONFIG_PM
static void ps3_suspend_cb(int argc, char *argv[]);
static void ps3_resume_cb(int argc, char *argv[]);
#endif

struct ps3_cli_debug_cmd {
	ps3_cli_func_t func;
	const char *func_name;	
	const char *help;	
};

static struct ps3_cli_debug_cmd g_ps3_cli_debug_cmd_table[] = {
	{ps3_cli_host_and_instance_ls, "ls", "ls option[host][instance][all]"},
	{ps3_cli_register_rw, "register_rw",
		"register_rw host_no xxx(host number) name/offset xxx(reg name/offset) read/write xxx(reg value)"},
	{ps3_cli_register_dump, "register_dump",
		"register_dump host_no xxx(host number)"},
	{ps3_cli_init_frame_dump, "init_frame_dump",
		"init_frame_dump host_no xxx(host number)"},
	{ps3_cli_event_subscirbe_info_dump,
		"event_subscribe_dump",
		"event_subscribe_dump host_no xxx(host number)"},
	{ps3_cli_event_delay_set,
		"event_delay_set",
		"event_delay_set host_no xxx(host number) delay xxx(seconds)"},
	{ps3_cli_crashdump_set,
		"crashdump",
		"crashdump host_no xxx(host number) wait xxx(seconds)"},
	{ps3_cli_dev_mgr_cli_base_dump, "dmbi",
		"device manager base infomation: dmbi <host_no> option[vd][pd][all]"},
	{ps3_cli_dev_mgr_cli_detail_dump, "dmdi",
		"device manager detail infomation: dmdi <host_no> <channel> <id>"},
	{ps3_cli_reply_fifo_dump, "reply_fifo_dump",
		"reply_fifo_dump host_no xxx(host number) isr_sn xxx(isr SN) \
		option[start_idx xxx default: last_reply_idx][count xxx default: 100]"},
	{ps3_cli_cmd_dump, "cmd_dump",
		"cmd_dump host_no xxx(host number) cmd_frame_id xxx(cmd_frame_id)"},
	{ps3_io_statis_dump_cli_cb, "show",
		"show io statis (detail|summary)"},
	{ps3_io_statis_clear_cli_cb, "clear",
		"clear io statis <channel> <target>"},
	{ps3_cli_force_to_stop, "force_to_stop", "force_to_stop"},
	{ps3_debug_mem_para_cli_cb, "debug_mem_para_dump",
		"debug_mem_para_dump host_no xxx(host no)"},
	{ps3_debug_mem_rw_cli_cb, "debug_mem",
		"debug_mem host_no (host no) entry_index(0--max entry count) dir(0 read/1 write) length(Bytes)"},
	{ps3_scsi_device_lookup_cb,
		"scsi_device_dump", "scsi_device_dump host_no xxx(host number)"},
	{ps3_hard_reset_cb,
		"hard_reset", "hard_reset host_no xxx(host number)"},
	{ps3_soc_halt_cb,
		"soc_halt", "soc_halt host_no xxx(host number)"},
	{ps3_cmd_stat_switch_store_cb, "cmd_stat_switch_store",
		"cmd_stat_switch_store host_no xxx(host number) value xxx mask xxx"
		"[bit0:OUTSTAND_SWITCH_OPEN, bit1:INC_SWITCH_OPEN,"
		" bit2:LOG_SWITCH_OPEN, bit3:DEV_SWITCH_OPEN]"},
	{ps3_cmd_stat_switch_show_cb,
		"cmd_stat_switch_show", "cmd_stat_switch_show host_no xxx"},
	{ps3_stat_total_show_cb, "cmd_stat_dump", "cmd_stat_dump host_no xxx"},
	{ps3_stat_inc_show_cb, "inc_stat_dump", "inc_stat_dump host_no xxx"},
	{ps3_stat_buf_clr_cb, "cmd_stat_clear", "cmd_stat_clear host_no xxx"},
	{ps3_stat_interval_show_cb, "cmd_stat_interval_show",
		"cmd_stat_interval_show host_no xxx"},
	{ps3_stat_interval_store_cb, "cmd_stat_interval_store",
		"cmd_stat_interval_store host_no xxx interval xxx(ms)"},
	{ps3_reply_fifo_reset_cb, "ps3_all_reply_fifo_reset",
		"ps3_all_reply_fifo_reset host_no xxx"},
	{ps3_cli_remove_host_force, "remove_host",
		"remove one host and free resource: <xxx>(host number)"},
	{ps3_hardreset_cnt_clear_cli_cb, "hardreset_cnt_clear",
		"hardreset_cnt_clear host_no xxx(host number)"},
	{ps3_hardreset_cnt_show_cli_cb, "hardreset_cnt_show",
		"hardreset_cnt_show host_no xxx(host number)"},
	{ps3_cli_ramfs_test_set, "ramfs_test_set",
		"set or clear filesystem type to ramfs: <xxx> (0: clear, 1: set)"},
	{ps3_cli_r1x_delay, "r1x_delay",
		"set r1x send interval delay: (x)ms"},
	{ps3_cli_err_inject_active, "err_inject",
		"err_inject err_type count"},
	{ps3_cli_err_inject_clear, "cli_err_clear",
		""},
	{ps3_no_wait_cli_cmd, "ps3_no_wait_cli_cmd",
		"clean_wait_cli_cmd flag xxx"},
    {ps3_cli_qos_info, "qos_dump",
     "qos_dump host_no xxx(host number) pd/vd xxx(id) | all"},
	{ps3_cli_special_log, "spe_log",
     "spe_log host_no xxx(host number) set xxx(0/1)"},
#ifdef PS3_SUPPORT_INJECT
	{ps3_inject_scsi_rw, "inject_rw",
	 "host_no xxx(host number) dev xxx xxx(channel id) lba xxx len xxx xxx(1 timeout_force_reply/2 error_reply)\
result xxx(scsi->result) sense xxx(resp_code-key-asc-ascq) times xxx"},
	{ps3_show_inject_scsi_rw, "show_inject_rw",
	 "host_no xxx(host number) dev xxx xxx(channel id) lba xxx len xxx xxx"},
	{ps3_clear_inject_scsi_rw, "clear_inject_rw",
	 "host_no xxx(host number) dev xxx xxx(channel id) lba xxx len xxx xxx"},
	{ps3_inject_scsi_task, "inject_scsi_task",
	 "host_no xxx(host number) dev xxx xxx(channel id) abort/reset (3 timeout/4 error_reply) times xxx"},
	{ps3_inject_mgr, "inject_mgr",
	 "host_no xxx(host number) cmd_type xxx cmd_sub_type xxx xxx (7 timeout/8 error_reply xxx (errType)) times xxx"},
	{ps3_show_inject_scsi_task, "show_inject_scsi_task",
	 "host_no xxx(host number) dev xxx xxx(channel id)"},
	{ps3_show_inject_mgr, "show_inject_mgr",
	 "host_no xxx(host number) cmd_type xxx cmd_sub_type xxx"},
	{ps3_clear_inject_scsi_task, "clear_inject_scsi_task",
	 "host_no xxx(host number) dev xxx xxx(channel id)"},
	{ps3_clear_inject_mgr, "clear_inject_mgr",
	 "host_no xxx(host number) cmd_type xxx cmd_sub_type xxx"},
	{ps3_clear_all_inject, "clear_inject_all",
	 "clear_inject_all"},
	{ps3_show_all_inject, "show_inject_all",
	 "show_inject_all"},
	{ps3_clear_hit_cmd, "ps3_clear_hit_cmd",
	 "ps3_clear_hit_cmd"},
#endif
#ifdef CONFIG_PM
	{ps3_suspend_cb, "ps3_suspend",
	 "ps3_suspend host_no xxx(host number)"},
	{ps3_resume_cb, "ps3_resume",
	 "ps3_resume host_no xxx(host number)"},
#endif
};

#define DRIVER_REGISTER_DEBUG_SIMULATOR	
#define REG_OFFSET(member)	(offsetof(Ps3Fifo_s, member))
struct ps3_reg_dump_desc {
	const char *reg_name;	
	U32 reg_offset;		
	U32 reg_cnt;		
	Bool is_readable;	
};

static struct ps3_reg_dump_desc g_reg_table[] = {
	{"ps3RequestQueue", PS3_REG_OFFSET_REQUEST_QUE, 1, PS3_TRUE},
	{"ps3Doorbell", PS3_REG_OFFSET_DOORBELL, 1, PS3_FALSE},
	{"ps3DoorbellIrqClear", PS3_REG_OFFSET_DOORBELL_IRQ_CLEAR, 1, PS3_TRUE},
	{"ps3DoorbellIrqMask", PS3_REG_OFFSET_DOORBELL_IRQ_MASK, 1, PS3_TRUE},
	{"ps3IrqControl", PS3_REG_OFFSET_IRQ_CONTROL, 1, PS3_TRUE},
	{"ps3SoftresetKey", PS3_REG_OFFSET_SOFTRESET_KEY, 1, PS3_TRUE},
	{"ps3SoftresetState", PS3_REG_OFFSET_SOFTRESET_STATE, 1, PS3_TRUE},
	{"ps3Softreset", PS3_REG_OFFSET_SOFTRESET, 1, PS3_TRUE},
	{"ps3SoftresetIrqClear", PS3_REG_OFFSET_SOFTRESET_IRQ_CLEAR, 1, PS3_TRUE},
	{"ps3SoftresetIrqMask", PS3_REG_OFFSET_SOFTRESET_IRQ_MASK, 1, PS3_TRUE},
	{"ps3SoftresetKeyShiftRegLow", PS3_REG_OFFSET_SOFTRESET_KEY_SHIFT_REG_LOW, 1, PS3_TRUE},
	{"ps3SoftresetKeyShiftRegHigh", PS3_REG_OFFSET_SOFTRESET_KEY_SHIFT_REG_HIGH, 1, PS3_TRUE},
	{"ps3SoftresetTimeCnt", PS3_REG_OFFSET_SOFTRESET_TIME_CNT, 1, PS3_TRUE},
	{"ps3SoftresetTimeOutEn", PS3_REG_OFFSET_SOFTRESET_TIME_OUT_CNT, 1, PS3_TRUE},
	{"ps3HardresetKey", PS3_REG_OFFSET_HARDRESET_KEY, 1, PS3_TRUE},
	{"ps3HardresetState", PS3_REG_OFFSET_HARDRESET_STATE, 1, PS3_TRUE},
	{"ps3Hardreset", PS3_REG_OFFSET_HARDRESET, 1, PS3_TRUE},
	{"ps3HardresetKeyShiftRegLow", PS3_REG_OFFSET_HARDRESET_KEY_SHIFT_REG_LOW, 1, PS3_TRUE},
	{"ps3HardresetKeyShiftRegHigh", PS3_REG_OFFSET_HARDRESET_KEY_SHIFT_REG_HIGH, 1, PS3_TRUE},
	{"ps3HardresetTimeCnt", PS3_REG_OFFSET_HARDRESET_TIME_CNT, 1, PS3_TRUE},
	{"ps3HardresetTimeOutEn", PS3_REG_OFFSET_HARDRESET_TIME_OUT_CNT, 1, PS3_TRUE},
	{"ps3KeyGapCfg", PS3_REG_OFFSET_KEY_GAP_CFG, 1, PS3_TRUE},
	{"ps3HardresetIrqClear", PS3_REG_OFFSET_HARDRESET_IRQ_CLEAR, 1, PS3_TRUE},
	{"ps3HardresetIrqMask", PS3_REG_OFFSET_HARDRESET_IRQ_MASK, 1, PS3_TRUE},
	{"ps3SocFwState", PS3_REG_OFFSET_SOC_FW_STATE, 1, PS3_TRUE},
	{"ps3MaxFwCmd", PS3_REG_OFFSET_MAX_FW_CMD, 1, PS3_TRUE},
	{"ps3MaxChainSize", PS3_REG_OFFSET_MAX_CHAIN_SIZE, 1, PS3_TRUE},
	{"ps3MaxVdInfoSize", PS3_REG_OFFSET_MAX_VD_INFO_SIZE, 1, PS3_TRUE},
	{"ps3MaxNvmePageSize", PS3_REG_OFFSET_MAX_NVME_PAGE_SIZE, 1, PS3_TRUE},
	{"ps3FeatureSupport", PS3_REG_OFFSET_FEATURE_SUPPORT, 1, PS3_TRUE},
	{"ps3FirmwareVersion", PS3_REG_OFFSET_FIRMWARE_VERSION, 1, PS3_TRUE},
	{"ps3MaxReplyque", PS3_REG_OFFSET_REPLY_QUE, 1, PS3_TRUE},
	{"ps3HardwareVersion", PS3_REG_OFFSET_HARDWARE_VERSION, 1, PS3_TRUE},
	{"ps3MgrQueueDepth", PS3_REG_OFFSET_MGR_QUEUE_DEPTH, 1, PS3_TRUE},
	{"ps3CmdQueueDepth", PS3_REG_OFFSET_CMD_QUEUE_DEPTH, 1, PS3_TRUE},
	{"ps3TfifoDepth", PS3_REG_OFFSET_TFIFO_DEPTH, 1, PS3_TRUE},
	{"ps3MaxSecR1xCmds", PS3_REG_OFFSET_MAX_SEC_R1X_CMDS, 1, PS3_TRUE},
	{"ps3HilAdvice2directCnt0", PS3_REG_OFFSET_HIL_ADVICE2DIRECT_CNT0, 1, PS3_TRUE},
	{"ps3HilAdvice2directCnt1", PS3_REG_OFFSET_HIL_ADVICE2DIRECT_CNT1, 1, PS3_TRUE},
	{"ps3HilAdvice2directCnt2", PS3_REG_OFFSET_HIL_ADVICE2DIRECT_CNT2, 1, PS3_TRUE},
	{"ps3HilAdvice2directCnt3", PS3_REG_OFFSET_HIL_ADVICE2DIRECT_CNT3, 1, PS3_TRUE},
	{"ps3HilAdvice2directCntAll", PS3_REG_OFFSET_HIL_ADVICE2DIRECT_CNT_ALL, 1, PS3_TRUE},
	{"ps3IrqStatusRpt", PS3_REG_OFFSET_IRQ_STATUS_RPT, 1, PS3_TRUE},
	{"ps3DumpCtrl", PS3_REG_OFFSET_DUMP_CTRL, 1, PS3_FALSE},
	{"ps3DumpCtrlIrqClear", PS3_REG_OFFSET_DUMP_CTRl_IRQ_CLEAR, 1, PS3_TRUE},
	{"ps3DumpCtrlIrqMask", PS3_REG_OFFSET_DUMP_CTRl_IRQ_MASK, 1, PS3_TRUE},
	{"ps3DumpStatus", PS3_REG_OFFSET_DUMP_STATUS, 1, PS3_TRUE},
	{"ps3DumpDataSize", PS3_REG_OFFSET_DUMP_DATA_SIZE, 1, PS3_TRUE},
	{"ps3CmdTrigger", PS3_REG_OFFSET_CMD_TRIGGER, 1, PS3_TRUE},
	{"ps3CmdTriggerIrqClear", PS3_REG_OFFSET_CMD_TRIGGER_IRQ_CLEAR, 1, PS3_TRUE},
	{"ps3CmdTriggerIrqMask", PS3_REG_OFFSET_CMD_TRIGGER_IRQ_MASK, 1, PS3_TRUE},
	{"ps3RegCmdState", PS3_REG_OFFSET_REG_CMD_STATE, 1, PS3_TRUE},
	{"ps3SoftresetCounter", PS3_REG_OFFSET_SOFTRESET_COUNTER, 1, PS3_TRUE},
	{"ps3Debug0", PS3_REG_OFFSET_DEBUG0, 1, PS3_TRUE},
	{"ps3Debug0IrqClear", PS3_REG_OFFSET_DEBUG0_IRQ_CLEAR, 1, PS3_TRUE},
	{"ps3Debug0IrqMask", PS3_REG_OFFSET_DEBUG0_IRQ_MASK, 1, PS3_TRUE},
	{"ps3Debug1", PS3_REG_OFFSET_DEBUG1, 1, PS3_TRUE},
	{"ps3Debug1IrqClear", PS3_REG_OFFSET_DEBUG1_IRQ_CLEAR, 1, PS3_TRUE},
	{"ps3Debug1IrqMask", PS3_REG_OFFSET_DEBUG1_IRQ_MASK, 1, PS3_TRUE},
	{"ps3Debug2", PS3_REG_OFFSET_DEBUG2, 1, PS3_TRUE},
	{"ps3Debug2IrqClear", PS3_REG_OFFSET_DEBUG2_IRQ_CLEAR, 1, PS3_TRUE},
	{"ps3Debug2IrqMask", PS3_REG_OFFSET_DEBUG2_IRQ_MASK, 1, PS3_TRUE},
	{"ps3Debug3", PS3_REG_OFFSET_DEBUG3, 1, PS3_TRUE},
	{"ps3Debug3IrqClear", PS3_REG_OFFSET_DEBUG3_IRQ_CLEAR, 1, PS3_TRUE},
	{"ps3Debug3IrqMask", PS3_REG_OFFSET_DEBUG3_IRQ_MASK, 1, PS3_TRUE},
	{"ps3Debug4", PS3_REG_OFFSET_DEBUG4, 1, PS3_TRUE},
	{"ps3Debug4IrqMask", PS3_REG_OFFSET_DEBUG4_IRQ_CLEAR, 1, PS3_TRUE},
	{"ps3Debug4IrqMask", PS3_REG_OFFSET_DEBUG4_IRQ_MASK, 1, PS3_TRUE},
	{"ps3Debug5", PS3_REG_OFFSET_DEBUG5, 1, PS3_TRUE},
	{"ps3Debug6", PS3_REG_OFFSET_DEBUG6, 1, PS3_TRUE},
	{"ps3Debug7", PS3_REG_OFFSET_DEBUG7, 1, PS3_TRUE},
	{"ps3Debug8", PS3_REG_OFFSET_DEBUG8, 1, PS3_TRUE},
	{"ps3Debug9", PS3_REG_OFFSET_DEBUG9, 1, PS3_TRUE},
	{"ps3Debug10", PS3_REG_OFFSET_DEBUG10, 1, PS3_TRUE},
	{"ps3Debug11", PS3_REG_OFFSET_DEBUG11, 1, PS3_TRUE},
	{"ps3Debug12", PS3_REG_OFFSET_DEBUG12, 1, PS3_TRUE},
	{"ps3SessioncmdAddr", PS3_REG_SESSION_ADDR, 1, PS3_TRUE},
	{"ps3SessioncmdAddrIrqClear", PS3_REG_SESSION_ADDR_IRQ_CLEAR, 1, PS3_TRUE},
	{"ps3SessioncmdAddrIrqMask", PS3_REG_SESSION_ADDR_IRQ_MASK, 1, PS3_TRUE},
};

static void ps3_cli_register_dump(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	char *buf = NULL;
	U16 host_no = 0;
	S32 ret = 0;

	buf = (char*)kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (buf == NULL) {
		ps3stor_cli_printf("malloc buf failed for register dump cli!\n");
		goto l_malloc_failed;
	}

	if (argc < 3) {
		ps3stor_cli_printf("Too few args for register dump cli cmd!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no for register dump cmd!\n");
		goto l_out;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d for register dump cmd!\n",
			host_no);
		goto l_out;
	}

	if (instance->reg_set == NULL) {
		ps3stor_cli_printf("ioc reg_set has not been alloc!\n");
		goto l_out;
	}

	(void)ps3_ioc_reg_dump(instance, buf);
	ps3stor_cli_printf(buf);
	ps3stor_cli_printf("\n");

l_out:
	kfree(buf);
l_malloc_failed:
	return;
}

static void ps3_cli_init_frame_dump(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	struct ps3_cmd_context *cmd_context = NULL;
	struct PS3InitReqFrame *init_frame_msg = NULL;
	U16 host_no = 0;
	S32 ret = 0;

	if (argc < 3) {
		ps3stor_cli_printf("Too few args for init_frame_dump cli cmd!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no for init_frame_dump cmd!\n");
		goto l_out;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d for init_frame_dump cmd!\n",
			host_no);
		goto l_out;
	}

	cmd_context = &instance->cmd_context;
	init_frame_msg = (struct PS3InitReqFrame *)cmd_context->init_frame_buf;

	if (init_frame_msg == NULL) {
		ps3stor_cli_printf("init frame has not been alloc!\n");
		goto l_out;
	}

	ps3stor_cli_printf("init frame:\n");
	ps3stor_cli_printf("reqHead.cmdType:\t%u\n",
		init_frame_msg->reqHead.cmdType);
	ps3stor_cli_printf("length:\t%u\n", init_frame_msg->length);
	ps3stor_cli_printf("operater:\t%u\n", init_frame_msg->operater);
	ps3stor_cli_printf("pageSize:\t%u\n", init_frame_msg->pageSize);
	ps3stor_cli_printf("pciIrqType:\t%u\n", init_frame_msg->pciIrqType);
	ps3stor_cli_printf("msixVector:\t%u\n", init_frame_msg->msixVector);
	ps3stor_cli_printf("timeStamp:\t%llu\n", init_frame_msg->timeStamp);
	ps3stor_cli_printf("reqFrameBufBaseAddr:\t0x%llx\n",
		init_frame_msg->reqFrameBufBaseAddr);
	ps3stor_cli_printf("replyFifoDescBaseAddr:\t0x%llx\n",
		init_frame_msg->replyFifoDescBaseAddr);
	ps3stor_cli_printf("respFrameBaseAddr:\t0x%llx\n",
		init_frame_msg->respFrameBaseAddr);
	ps3stor_cli_printf("eventTypeMap:\t0x%08x\n", init_frame_msg->eventTypeMap);
	ps3stor_cli_printf("reqFrameMaxNum:\t%u\n", init_frame_msg->reqFrameMaxNum);
	ps3stor_cli_printf("respFrameMaxNum:\t%u\n",
		init_frame_msg->respFrameMaxNum);
	ps3stor_cli_printf("bufSizePerRespFrame:\t%u\n",
		init_frame_msg->bufSizePerRespFrame);
	ps3stor_cli_printf("hilMode:\t%u\n",
		init_frame_msg->hilMode);
	ps3stor_cli_printf("systemInfoBufAddr:\t0x%llx\n",
		init_frame_msg->systemInfoBufAddr);
	ps3stor_cli_printf("debugMemAddr:\t0x%llx\n", init_frame_msg->debugMemArrayAddr);
	ps3stor_cli_printf("debugMemSize:\t%u\n", init_frame_msg->debugMemArrayNum);
	ps3stor_cli_printf("filterTableAddr:\t0x%llx\n", init_frame_msg->filterTableAddr);
	ps3stor_cli_printf("filterTableLen:\t%u\n", init_frame_msg->filterTableLen);

	ps3stor_cli_printf("respStatus:\t0x%08x\n", init_frame_msg->respStatus);

l_out:
	return;
}

static S32 ps3_register_offset_lookup(const char *reg_name_string,
	U32 *reg_offset)
{
	U32 idx = 0;
	struct ps3_reg_dump_desc *reg_desc = NULL;

	for (idx = 0; idx < ARRAY_SIZE(g_reg_table); idx++) {
		reg_desc = &g_reg_table[idx];

		if (reg_desc->reg_cnt != 1) {
			LOG_DEBUG("Reg %s is not a single register!\n",
				reg_desc->reg_name);
			continue;
		}

		if (strcmp(reg_name_string, reg_desc->reg_name) == 0) {
			*reg_offset = reg_desc->reg_offset;
			return PS3_SUCCESS;
		}
	}

	return -PS3_FAILED;
}

static void ps3_register_read_write(struct ps3_instance *instance,
	int argc, char *argv[])
{
	const char *reg_name_offset_string = argv[3];
	const char *reg_loc_string = argv[4];
	const char *state_ops_string = argv[5];
	U32 reg_value = 0;
	U32 reg_offset = 0;
	S32 ret = 0;
	U64 value;

	if (strcmp(reg_name_offset_string, "offset") == 0) {
		ret = kstrtouint(reg_loc_string, 0, &reg_offset);
	} else {
		ret = ps3_register_offset_lookup(reg_loc_string, &reg_offset);
	}

	if (ret != PS3_SUCCESS) {
		ps3stor_cli_printf("Reg %s not supported to read and write!\n",
			reg_loc_string);
		goto l_out;
	}

	if (strcmp(state_ops_string, "read") == 0) {
		PS3_IOC_REG_READ_OFFSET_WITCH_CHECK(instance, reg_offset, value);
		ps3stor_cli_printf("0x%llx\n", value);
	} else if (strcmp(state_ops_string, "write") == 0) {
		if (argc < 7) {
			ps3stor_cli_printf("No reg value for %s write cmd!\n",
				reg_loc_string);
			goto l_out;
		}

		ret = kstrtouint(argv[6], 0, &reg_value);
		if (ret != 0) {
			ps3stor_cli_printf("Invalid reg value for %s write cmd!\n",
				reg_loc_string);
			goto l_out;
		}

		PS3_IOC_REG_WRITE_OFFSET_WITH_CHECK(instance, reg_offset, (U64)reg_value);
		ps3stor_cli_printf("Write 0x%x to %s!\n", reg_value,
			reg_loc_string);
	} else {
		ps3stor_cli_printf("Invalid ops for %s cmd!\n", reg_loc_string);
	}

l_out:
	return;
}

static void ps3_cli_register_rw(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	U16 host_no = 0;
	S32 ret = 0;

	if (argc < 6) {
		ps3stor_cli_printf("Too few args for register cli cmd!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no for register cli cmd!\n");
		goto l_out;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d for register cli cmd!\n",
			host_no);
		goto l_out;
	}

	ps3_register_read_write(instance, argc, argv);
l_out:
	return;
}

ssize_t ps3_ioc_reg_dump(struct ps3_instance *instance, char *buf)
{
	ssize_t len = 0;
	ssize_t total_len = PAGE_SIZE;
	U32 idx = 0;
	U64 value = 0;
	struct ps3_reg_dump_desc *reg_desc = NULL;

	for (idx = 0; idx < ARRAY_SIZE(g_reg_table); idx++) {
		reg_desc = &g_reg_table[idx];

#ifndef DRIVER_REGISTER_DEBUG_SIMULATOR
		if (!reg_desc->is_readable) {
			continue;
		}
#endif
		if (reg_desc->reg_cnt > 1) {
			continue;
		}

		PS3_IOC_REG_READ_OFFSET_WITCH_CHECK(instance, reg_desc->reg_offset, value);
		len += snprintf(buf + len, total_len - len, "%s:\t0x%08llx\n",
			reg_desc->reg_name, value);
	}

	return len;
}

static void ps3_dev_mgr_print_vd(struct PS3VDEntry *p_entry, Bool is_detail)
{
	U8 i = 0;
	U8 j = 0;
	ps3stor_cli_printf("vd channel, id, virtDiskId: [%u:%u:%u], magicNum:%#x\n",
		PS3_CHANNEL(&p_entry->diskPos), PS3_TARGET(&p_entry->diskPos),
		PS3_VDID(&p_entry->diskPos), p_entry->diskPos.diskMagicNum);
	ps3stor_cli_printf("vd dev_type: PS3_DEV_TYPE_VD\n");
	ps3stor_cli_printf("vd isHidden:         %s\n",
		(p_entry->isHidden) ? "true" : "false");
	ps3stor_cli_printf("vd accessPolicy:     %s\n",
		ps3_get_vd_access_plolicy_str((VDAccessPolicy_e)p_entry->accessPolicy));
	ps3stor_cli_printf("vd diskGrpId:        %u\n", p_entry->diskGrpId);

	if (!is_detail) {
		return;
	}
	ps3stor_cli_printf("vd sectorSize:       %u\n", p_entry->sectorSize);
	ps3stor_cli_printf("vd stripeDataSize:   %u\n", p_entry->stripeDataSize);
	ps3stor_cli_printf("vd physDrvCnt:       %u\n", p_entry->physDrvCnt);
	ps3stor_cli_printf("vd stripSize:        %u\n", p_entry->stripSize);
	ps3stor_cli_printf("vd isDirectEnable:   %s\n",
		(p_entry->isDirectEnable) ? "true" : "false");
	ps3stor_cli_printf("vd isNvme:   %s\n",
		(p_entry->isNvme) ? "true" : "false");
	ps3stor_cli_printf("vd isSsd:   %s\n",
		(p_entry->isSsd) ? "true" : "false");
	ps3stor_cli_printf("vd raidLevel:        %u\n", p_entry->raidLevel);
	ps3stor_cli_printf("vd spanCount:        %u\n", p_entry->spanCount);
	ps3stor_cli_printf("vd diskState:        %u\n", p_entry->diskState);
	ps3stor_cli_printf("vd startLBA:         %llu\n", p_entry->startLBA);
	ps3stor_cli_printf("vd extentSize:       %llu\n", p_entry->extentSize);
	ps3stor_cli_printf("vd isTaskMgmtEnable: %u\n", p_entry->isTaskMgmtEnable);
	ps3stor_cli_printf("vd taskAbortTimeout: %u\n", p_entry->taskAbortTimeout);
	ps3stor_cli_printf("vd taskResetTimeout: %u\n", p_entry->taskResetTimeout);
	ps3stor_cli_printf("vd mapBlock:         %llu\n", p_entry->mapBlock);
	ps3stor_cli_printf("vd mapBlockVer:      %u\n", p_entry->mapBlockVer);
	ps3stor_cli_printf("vd maxIOSize:        %u\n", p_entry->maxIOSize);
	ps3stor_cli_printf("vd devQueDepth:      %u\n", p_entry->devQueDepth);
	ps3stor_cli_printf("vd dmaAddrAlignShift: %u\n", p_entry->dmaAddrAlignShift);
	ps3stor_cli_printf("vd dmaLenAlignShift:  %u\n", p_entry->dmaLenAlignShift);
	ps3stor_cli_printf("vd capacity(sector): %llu\n", p_entry->capacity);
	ps3stor_cli_printf("vd bdev_bdi_cap:      %u\n", p_entry->bdev_bdi_cap);
	ps3stor_cli_printf("vd umapBlkDescCnt: %u\n", p_entry->umapBlkDescCnt);
	ps3stor_cli_printf("vd umapNumblk:  %u\n", p_entry->umapNumblk);
	ps3stor_cli_printf("vd virtDiskSeq: %llu\n", p_entry->virtDiskSeq);
	ps3stor_cli_printf("vd normalQuota:%u directQuota:%u\n",
			p_entry->normalQuota, p_entry->directQuota);
	ps3stor_cli_printf("vd dev_busy_scale: %u\n", p_entry->dev_busy_scale);

	for (i = 0; i < p_entry->spanCount; i++) {
		ps3stor_cli_printf("  span[%u].spanStripeDataSize: %u\n", i,
			p_entry->span[i].spanStripeDataSize);
		ps3stor_cli_printf("  span[%u].spanState:          %u\n", i,
			p_entry->span[i].spanState);
		ps3stor_cli_printf("  span[%u].spanPdNum:          %u\n", i,
			p_entry->span[i].spanPdNum);

		for (j = 0; j < p_entry->span[i].spanPdNum; j++) {
			ps3stor_cli_printf(
				"    span[%u].extent[%u] - member pd:[%u:%u:%u]\n",
				i, j,
				p_entry->span[i].extent[j].phyDiskID.ps3Dev.softChan,
				p_entry->span[i].extent[j].phyDiskID.ps3Dev.devID,
				p_entry->span[i].extent[j].phyDiskID.ps3Dev.phyDiskID);
			ps3stor_cli_printf("    span[%u].extent[%u].state: %u\n", i, j,
				p_entry->span[i].extent[j].state);
		}
	}
}

static void ps3_dev_mgr_show_base_vd(struct ps3_instance *instance)
{
	U16 i = 0;
	U16 j = 0;
	U8 chan_id = 0;
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;
	struct ps3_channel *vd_chan = p_dev_ctx->channel_vd;
	U8 vd_table_idx = p_dev_ctx->vd_table_idx & 1;
	struct ps3_vd_table *p_table =
		&p_dev_ctx->vd_table[vd_table_idx];
	struct PS3VDEntry *p_entries =
		p_dev_ctx->vd_entries_array[vd_table_idx];
	U16 vd_idx = PS3_INVALID_DEV_ID;
	U16 virtDiskIdx = PS3_INVALID_DEV_ID;

	ps3stor_cli_printf("\n===========start print VD base infomation=========\n");
	for (i = 0; i < p_dev_ctx->vd_channel_count; i++) {
		chan_id = vd_chan[i].channel;
		for (j = 0; j < vd_chan[i].max_dev_num; j++) {
			vd_idx = p_table->vd_idxs[chan_id][j];
			virtDiskIdx = get_offset_of_vdid(PS3_VDID_OFFSET(instance), vd_idx);
			if (vd_idx != PS3_INVALID_DEV_ID) {
				ps3_dev_mgr_print_vd(&p_entries[virtDiskIdx], PS3_FALSE);
			}
		}
	}
	ps3stor_cli_printf("\n============end print VD base infomation==========\n");
}

static void ps3_dev_mgr_print_pd(struct ps3_pd_entry *p_entry, Bool is_detail, Bool is_raid)
{
	ps3stor_cli_printf("pd channel, id, pdFlatId: [%u:%u:%u], magicNum: %#x\n",
		PS3_CHANNEL(&p_entry->disk_pos), PS3_TARGET(&p_entry->disk_pos),
		PS3_PDID(&p_entry->disk_pos), p_entry->disk_pos.diskMagicNum);
	ps3stor_cli_printf("pd state: %s\n",
		getDeviceStateName((DeviceState_e)p_entry->state));
	ps3stor_cli_printf("pd dev_type: %s\n",
		namePS3DevType((enum PS3DevType)p_entry->dev_type));
	ps3stor_cli_printf("pd config_flag:         %s\n",
		getPdStateName((MicPdState_e)p_entry->config_flag, is_raid));
	ps3stor_cli_printf("pd pd_flags:            0x%02x\n", p_entry->pd_flags);
	ps3stor_cli_printf("pd support_ncq:         %d\n", p_entry->support_ncq);

	if (is_detail) {
		ps3stor_cli_printf("pd RWCT:                %u\n",
			p_entry->RWCT);
		ps3stor_cli_printf("pd scsi_interface_type: %u\n",
			p_entry->scsi_interface_type);
		ps3stor_cli_printf("pd task_abort_timeout:  %u\n",
			p_entry->task_abort_timeout);
		ps3stor_cli_printf("pd task_reset_timeout:  %u\n",
			p_entry->task_reset_timeout);
		ps3stor_cli_printf("pd max_io_size:         %u\n",
			p_entry->max_io_size);
		ps3stor_cli_printf("pd dev_queue_depth:     %u\n",
			p_entry->dev_queue_depth);
		ps3stor_cli_printf("pd sector_size:         %u\n",
			p_entry->sector_size);
		ps3stor_cli_printf("pd encl_id:             %u\n",
			p_entry->encl_id);
		ps3stor_cli_printf("pd phy_id:              %u\n",
			p_entry->phy_id);
		ps3stor_cli_printf("pd dma_addr_alignment:  %u\n",
			p_entry->dma_addr_alignment);
		ps3stor_cli_printf("pd dma_len_alignment:   %u\n",
			p_entry->dma_len_alignment);
		ps3stor_cli_printf("pd protect:             %u\n",
			p_entry->protect);
		ps3stor_cli_printf("qos pd quota:normal[%u] direct[%u]\n",
			p_entry->normal_quota, p_entry->direct_quota);
	}
}

static void ps3_dev_mgr_show_base_pd(struct ps3_instance *instance)
{
	U16 i = 0;
	U16 j = 0;
	U8 chan_id = 0;
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;
	struct ps3_channel *pd_chan = p_dev_ctx->channel_pd;
	struct ps3_pd_table *p_table = &p_dev_ctx->pd_table;
	U16 pd_idx = PS3_INVALID_DEV_ID;

	ps3stor_cli_printf("\n===========start print PD base infomation=========\n");
	for (i = 0; i < p_dev_ctx->pd_channel_count; i++) {
		chan_id = pd_chan[i].channel;
		for (j = 0; j < pd_chan[i].max_dev_num; j++) {
			pd_idx = p_table->pd_idxs[chan_id][j];
			if (pd_idx != PS3_INVALID_DEV_ID) {
				ps3_dev_mgr_print_pd(
					&p_dev_ctx->pd_entries_array[pd_idx],
					PS3_DRV_FALSE, instance->is_raid);
			}
		}
	}
	ps3stor_cli_printf("\n============end print PD base infomation==========\n");
}

static void ps3_cli_dev_mgr_cli_base_dump(int argc, char *argv[])
{
	S32 ret = 0;
	U16 host_no = 0;
	struct ps3_instance *instance = NULL;

	if (argc < 3) {
		ps3stor_cli_printf("Too few args for dmbi!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[1], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no \"%s\" for dmbi cmd!\n",
			argv[1]);
		goto l_out;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d for dmbi cmd!\n",
			host_no);
		goto l_out;
	}

	if (strcmp(argv[2], "vd") == 0) {
		ps3_dev_mgr_show_base_vd(instance);
		goto l_out;
	} else if (strcmp(argv[2], "pd") == 0) {
		ps3_dev_mgr_show_base_pd(instance);
		goto l_out;
	} else if (strcmp(argv[2], "all") == 0) {
		ps3_dev_mgr_show_base_vd(instance);
		ps3_dev_mgr_show_base_pd(instance);
		goto l_out;
	} else {
		ps3stor_cli_printf("Invalid subtype \"%s\" for dmbi cmd!\n",
			argv[2]);
	}
l_out:
	return;
}

static void ps3_dev_mgr_show_detail_info(struct ps3_instance *instance,
	U16 channel, U16 id)
{
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;
	struct ps3_pd_table *p_pd_table = &p_dev_ctx->pd_table;
	U8 vd_table_idx = p_dev_ctx->vd_table_idx & 1;
	struct ps3_vd_table *p_vd_table =
		&p_dev_ctx->vd_table[vd_table_idx];
	struct PS3VDEntry *p_vd_array =
		p_dev_ctx->vd_entries_array[vd_table_idx];
	U16 disk_idx = 0;
	U16 virtDiskIdx = PS3_INVALID_DEV_ID;

	if (PS3_IS_VD_CHANNEL(instance, channel)) {
		if (p_vd_table->vd_idxs[channel] == NULL) {
			ps3stor_cli_printf("==error==invalid channel parameter: %d\n",
				channel);
			goto l_out;
		}

		if (id >= p_dev_ctx->max_dev_in_channel[channel]) {
			ps3stor_cli_printf("==error==invalid id parameter: %d\n",
				id);
			goto l_out;
		}

		disk_idx = p_vd_table->vd_idxs[channel][id];
		virtDiskIdx = get_offset_of_vdid(PS3_VDID_OFFSET(instance), disk_idx);
		if (virtDiskIdx == PS3_INVALID_DEV_ID) {
			ps3stor_cli_printf("==warn==vd is not exist: [%u:%u]\n",
				channel, id);
			goto l_out;
		}
		ps3stor_cli_printf("\n===========start print VD detail infomation=========\n");
		ps3_dev_mgr_print_vd(&p_vd_array[virtDiskIdx], PS3_DRV_TRUE);
		ps3stor_cli_printf("\n============end print VD detail infomation==========\n");

	} else {
		if (p_pd_table->pd_idxs[channel] == NULL) {
			ps3stor_cli_printf("==error==invalid channel parameter: %d\n",
				channel);
			goto l_out;
		}

		if (id >= p_dev_ctx->max_dev_in_channel[channel]) {
			ps3stor_cli_printf("==error==invalid id parameter: %d\n",
				id);
			goto l_out;
		}

		disk_idx = p_pd_table->pd_idxs[channel][id];
		if (disk_idx == PS3_INVALID_DEV_ID) {
			ps3stor_cli_printf("==warn==pd is not exist: [%u:%u]\n",
				channel, id);
			goto l_out;
		}
		ps3stor_cli_printf("\n===========start print PD detail infomation=========\n");
		ps3_dev_mgr_print_pd(&p_dev_ctx->pd_entries_array[disk_idx],
			PS3_DRV_TRUE, instance->is_raid);
		ps3stor_cli_printf("\n============end print PD detail infomation==========\n");
	}
l_out:
	return;
}

static void ps3_cli_dev_mgr_cli_detail_dump(int argc, char *argv[])
{
	S32 ret = 0;
	U16 host_no = 0;
	U16 channel = 0;
	U16 id = 0;
	struct ps3_instance *instance = NULL;

	if (argc < 4) {
		ps3stor_cli_printf("Too few args for dmdi!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[1], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no \"%s\" for dmdi cmd!\n",
			argv[1]);
		goto l_out;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d for dmdi cmd!\n",
			host_no);
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &channel);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse chanel \"%s\" for dmdi cmd!\n",
			argv[2]);
		goto l_out;
	}

	ret = kstrtou16(argv[3], 0, &id);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse targetId \"%s\" for dmdi cmd!\n",
			argv[3]);
		goto l_out;
	}

	ps3_dev_mgr_show_detail_info(instance, channel, id);
l_out:
	return;
}

static void ps3_cli_event_subscirbe_info_dump(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	char *buf = NULL;
	U16 host_no = 0;
	S32 ret = 0;

	buf = (char*)kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (buf == NULL) {
		ps3stor_cli_printf("malloc buf failed for event subscribe info cli!\n");
		goto l_malloc_failed;
	}

	if (argc < 3) {
		ps3stor_cli_printf("Too few args for event_subscribe_dump cli cmd!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no for event_subscribe_dump cmd!\n");
		goto l_out;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d for event_subscribe_dump cmd!\n",
			host_no);
		goto l_out;
	}

	(void)ps3_event_subscribe_info_get(instance, buf, PAGE_SIZE);
	ps3stor_cli_printf(buf);

l_out:
	kfree(buf);
l_malloc_failed:
	return;
}

static void ps3_cli_event_delay_set(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	U16 host_no = 0;
	U32 delay_seconds = 0;
	S32 ret = 0;

	if (argc < 3) {
		ps3stor_cli_printf("Too few args for event_subscribe_dump cli cmd!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no for event_subscribe_dump cmd!\n");
		goto l_out;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d for event_subscribe_dump cmd!\n",
			host_no);
		goto l_out;
	}

	if (strcmp(argv[3], "delay") == 0) {
		ret = kstrtouint(argv[4], 0, &delay_seconds);
		if (ret != 0) {
			ps3stor_cli_printf("Can not parse delay seconds for event delay cmd!\n");
			goto l_out;
		}
		ret = ps3_event_delay_set(instance, delay_seconds);
		if (ret != 0) {
			ps3stor_cli_printf("Unable to config event delay ret %d\n", ret);
			goto l_out;
		} else {
			ps3stor_cli_printf("Config event delay %d success\n", delay_seconds);
		}
	}

l_out:
	return;
}

static void ps3_cli_crashdump_set(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	U16 host_no = 0;
	U32 delay_seconds = 0;
	S32 ret = 0;

	if (argc < 3) {
		ps3stor_cli_printf("Too few args for crashdump cli cmd!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no for event_subscribe_dump cmd!\n");
		goto l_out;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d for event_subscribe_dump cmd!\n",
			host_no);
		goto l_out;
	}

	if (strcmp(argv[3], "wait") == 0) {
		ret = kstrtouint(argv[4], 0, &delay_seconds);
		if (ret != 0) {
			ps3stor_cli_printf("Can not parse delay seconds for event delay cmd!\n");
			goto l_out;
		}

		if (delay_seconds < WAIT_DUMP_TIMES_MIN) {
			ps3stor_cli_printf("min %d!\n", WAIT_DUMP_TIMES_MIN);
			delay_seconds = WAIT_DUMP_TIMES_MIN;
		} else if (delay_seconds > WAIT_DUMP_TIMES_MAX) {
			ps3stor_cli_printf("max %d!\n", WAIT_DUMP_TIMES_MAX);
			delay_seconds = WAIT_DUMP_TIMES_MAX;
		}

		instance->dump_context.dump_dma_wait_times = delay_seconds;
		ps3stor_cli_printf("set crashdump dma wait times %d success\n", delay_seconds);
	}

l_out:
	return;
}

S32 ps3_dump_context_show(const char *prefix, struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_dump_context *ctxt = &instance->dump_context;

	ps3stor_cli_printf("%sdump_type            : %d \n", prefix, ctxt->dump_type);
	ps3stor_cli_printf("%sdump_state           : %d \n", prefix,ctxt->dump_state);
	ps3stor_cli_printf("%sdump_work_status     : %d \n", prefix,ctxt->dump_work_status);
	ps3stor_cli_printf("%sdump_env             : %d \n", prefix,ctxt->dump_env);
	ps3stor_cli_printf("%sdump_dma_vaddr       : %p \n", prefix, ctxt->dump_dma_buf);
	ps3stor_cli_printf("%sdump_dma_paddr       : %llx \n", prefix, ctxt->dump_dma_addr);
	ps3stor_cli_printf("%sdump_dma_buff_size   : %#x \n", prefix, PS3_DUMP_DMA_BUF_SIZE);
	ps3stor_cli_printf("%sdump_dma_wait_times  : %#x \n", prefix, ctxt->dump_dma_wait_times);
	ps3stor_cli_printf("%sdump_data_size       : %llu \n", prefix,ctxt->dump_data_size);
	ps3stor_cli_printf("%sdump_data_size_copyed: %llu \n", prefix, ctxt->copyed_data_size);
	ps3stor_cli_printf("%sdump_dir             : %s \n", prefix,ctxt->dump_dir);
	ps3stor_cli_printf("%sdump_file            : %s \n", prefix,ctxt->dump_out_file.filename);
	ps3stor_cli_printf("%sdump_file_type       : %d \n", prefix,ctxt->dump_out_file.type);
	ps3stor_cli_printf("%sdump_file_size       : %llu\n", prefix,ctxt->dump_out_file.file_size);
	ps3stor_cli_printf("%sdump_file_write_cnt  : %llu\n", prefix,ctxt->dump_out_file.file_w_cnt);
	ps3stor_cli_printf("%sdump_file_status     : %d\n", prefix,ctxt->dump_out_file.file_status);
	ps3stor_cli_printf("%sdump_file_fp         : %p\n", prefix,ctxt->dump_out_file.fp);
	if (ctxt->dump_pending_cmd) {
		ps3stor_cli_printf("%sdump_pending_cmd     : [trace_id:0x%llx][CFID:%d]\n", prefix,
			ctxt->dump_pending_cmd->trace_id, ctxt->dump_pending_cmd->cmd_word.cmdFrameID);
	}
	ps3stor_cli_printf("%sdump_notify_reg_times: %d\n", prefix,ctxt->dump_pending_send_times);
	ps3stor_cli_printf("%sdump_type_times      : %d\n", prefix,ctxt->dump_type_times);
	ps3stor_cli_printf("%sdump_state_times     : %d\n", prefix,ctxt->dump_state_times);

	return ret;
}

static void ps3_host_info_detail_dump(void)
{
	struct ps3_instance *instance = NULL;
	struct list_head *pitem = NULL;
	struct Scsi_Host *phost = NULL;
	list_for_each(pitem, &ps3_mgmt_info_get()->instance_list_head) {
		instance = list_entry(pitem, struct ps3_instance, list_item);
		phost = instance->host;
		ps3stor_cli_printf("host_no:%d scsi_host detail info\n", phost->host_no);
		ps3stor_cli_printf("    host_failed %d\n", phost->host_failed);
		ps3stor_cli_printf("    host_eh_scheduled %d\n", phost->host_eh_scheduled);
		ps3stor_cli_printf("    host_no %d\n", phost->host_no);
		ps3stor_cli_printf("    eh_deadline %d\n", phost->eh_deadline);
		ps3stor_cli_printf("    last_reset %ul\n", phost->last_reset);
		ps3stor_cli_printf("    max_id %d\n", phost->max_id);
		ps3stor_cli_printf("    max_lun %d\n", phost->max_lun);
		ps3stor_cli_printf("    max_channel %d\n", phost->max_channel);
		ps3stor_cli_printf("    unique_id %d\n", phost->unique_id);
		ps3stor_cli_printf("    max_cmd_len %d\n", phost->max_cmd_len);
		ps3stor_cli_printf("    can_queue %d\n", phost->can_queue);
		ps3stor_cli_printf("    cmd_per_lun %d\n", phost->cmd_per_lun);
		ps3stor_cli_printf("    sg_tablesize %d\n", phost->sg_tablesize);
		ps3stor_cli_printf("    sg_prot_tablesize %d\n", phost->sg_prot_tablesize);
		ps3stor_cli_printf("    max_sectors %d\n", phost->max_sectors);
		ps3stor_cli_printf("    dma_boundary 0x%x\n", phost->dma_boundary);

		switch (phost->shost_state)
		{
		case SHOST_CREATED:
			ps3stor_cli_printf("    shost_state SHOST_CREATED\n");
			break;
		case SHOST_RUNNING:
			ps3stor_cli_printf("    shost_state SHOST_RUNNING\n");
			break;
		case SHOST_CANCEL:
			ps3stor_cli_printf("    shost_state SHOST_CANCEL\n");
			break;
		case SHOST_DEL:
			ps3stor_cli_printf("    shost_state SHOST_DEL\n");
			break;
		case SHOST_RECOVERY:
			ps3stor_cli_printf("    shost_state SHOST_RECOVERY\n");
			break;
		case SHOST_CANCEL_RECOVERY:
			ps3stor_cli_printf("    shost_state SHOST_CANCEL_RECOVERY\n");
			break;
		case SHOST_DEL_RECOVERY:
			ps3stor_cli_printf("    shost_state SHOST_DEL_RECOVERY\n");
			break;
		default:
			break;
		}
		ps3stor_cli_printf("\n");
	}
}

static void ps3_cmd_context_detail_dump(const char *format_space,
	struct ps3_cmd_context *cmd_context)
{
	U32 i = 0;
	ps3stor_cli_printf("%smax_cmd_count:%d\n",
		format_space, cmd_context->max_cmd_count);
	ps3stor_cli_printf("%smax_scsi_cmd_count:%d\n",
		format_space, cmd_context->max_scsi_cmd_count);
	ps3stor_cli_printf("%smax_mgr_cmd_count:%d\n",
		format_space, cmd_context->max_mgr_cmd_count);
	ps3stor_cli_printf("%sreq_frame_buf_phys:0x%llx\n",
		format_space, cmd_context->req_frame_buf_phys);
	ps3stor_cli_printf("%sresponse_frame_buf_phys:0x%llx\n",
		format_space, cmd_context->response_frame_buf_phys);
	ps3stor_cli_printf("%sinit_frame_buf_phys:0x%llx\n",
		format_space, cmd_context->init_frame_buf_phys);
	ps3stor_cli_printf("%sinit_frame_sys_info_phys:0x%llx\n",
		format_space, cmd_context->init_frame_sys_info_phys);
	ps3stor_cli_printf("%s sgl_mode_support: %u\n",
		format_space, cmd_context->sgl_mode_support);

	if (cmd_context->cmd_buf == NULL) {
		goto l_out;
	}
	for (i = cmd_context->max_scsi_cmd_count;
		i < cmd_context->max_cmd_count; i++) {
		if (cmd_context->cmd_buf[i]->cmd_state.state == PS3_CMD_STATE_INIT) {
			continue;
		}
		ps3stor_cli_printf("%s pending cmd mgr : [trace_id:0x%llx][CFID:%d][type:%d]"
			"[isrSN:%d][%s]\n",
			format_space,
			cmd_context->cmd_buf[i]->trace_id,
			cmd_context->cmd_buf[i]->cmd_word.cmdFrameID,
			cmd_context->cmd_buf[i]->cmd_word.type,
			cmd_context->cmd_buf[i]->cmd_word.isrSN,
			namePS3CmdState(cmd_context->cmd_buf[i]->cmd_state.state));
	}
	for (i = 0; i < cmd_context->max_scsi_cmd_count; i++) {
		if (cmd_context->cmd_buf[i]->cmd_state.state == PS3_CMD_STATE_INIT) {
			continue;
		}
		ps3stor_cli_printf("%s pending cmd scsi: [trace_id:0x%llx][CFID:%d][type:%d]"
			"[isrSN:%d][%s]\n",
			format_space,
			cmd_context->cmd_buf[i]->trace_id,
			cmd_context->cmd_buf[i]->cmd_word.cmdFrameID,
			cmd_context->cmd_buf[i]->cmd_word.type,
			cmd_context->cmd_buf[i]->cmd_word.isrSN,
			namePS3CmdState(cmd_context->cmd_buf[i]->cmd_state.state));
	}
l_out:
	ps3stor_cli_printf("\n");
}

static void ps3_irq_context_detail_dump(const char *format_space,
	struct ps3_irq_context *irq_context)
{
	U32 i = 0;

	ps3stor_cli_printf("%sreply_fifo_depth:%d\n",
		format_space, irq_context->reply_fifo_depth);
	ps3stor_cli_printf("%svalid_msix_vector_count:%d\n",
		format_space, irq_context->valid_msix_vector_count);
	ps3stor_cli_printf("%shigh_iops_msix_vectors:%d\n",
		format_space, irq_context->high_iops_msix_vectors);
	ps3stor_cli_printf("%sreply_fifo_desc_buf_phys:0x%llx\n",
		format_space, irq_context->reply_fifo_desc_buf_phys);
	for (i = 0; i < irq_context->valid_msix_vector_count; i++) {
		ps3stor_cli_printf("%sreply_fifo_phys_base_addr_buf[%d]:0x%llx\n",
			format_space, i,
			irq_context->reply_fifo_phys_base_addr_buf[i]);
	}

	for (i = 0; (S32)i < irq_context->cpu_msix_table_sz - 1; i++) {
		if (irq_context->cpu_msix_table == NULL) {
			break;
		}
		ps3stor_cli_printf("%scpu_msix_table[%d]:0x%llx\n",
			format_space, i, irq_context->cpu_msix_table[i]);
	}

	ps3stor_cli_printf("%shigh_iops_io_count:%d\n",
		format_space, irq_context->high_iops_io_count.counter);
	ps3stor_cli_printf("%sis_enable_interrupts:%s\n",
		format_space,
		irq_context->is_enable_interrupts ? "PS3_TRUE" : "PS3_FALSE");
	ps3stor_cli_printf("%sis_support_balance:%s\n",
		format_space,
		irq_context->is_support_balance ? "PS3_TRUE" : "PS3_FALSE");
	ps3stor_cli_printf("%sis_balance_current_perf_mode:%s\n",
		format_space,
		irq_context->is_balance_current_perf_mode ?
			"PS3_TRUE" : "PS3_FALSE");
	switch (irq_context->pci_irq_type) {
	case PS3_PCI_IRQ_LEGACY:
		ps3stor_cli_printf("%spci_irq_type:PS3_PCI_IRQ_LEGACY\n", format_space);
	break;
	case PS3_PCI_IRQ_MSI:
		ps3stor_cli_printf("%spci_irq_type:PS3_PCI_IRQ_MSI\n", format_space);
	break;
	case PS3_PCI_IRQ_MSIX:
		ps3stor_cli_printf("%spci_irq_type:PS3_PCI_IRQ_MSIX\n", format_space);
	break;
	default:
		break;
	}
	ps3stor_cli_printf("\n");
}

static void ps3_dev_context_detail_dump(const char *format_space,
	struct ps3_dev_context *dev_context)
{
	ps3stor_cli_printf("%svd_table_idx:%d\n", format_space, dev_context->vd_table_idx);
	ps3stor_cli_printf("%spd_channel_count:%d\n",
		format_space, dev_context->pd_channel_count);
	ps3stor_cli_printf("%svd_channel_count:%d\n",
		format_space, dev_context->vd_channel_count);
	ps3stor_cli_printf("%stotal_vd_count:%d\n",
		format_space, dev_context->total_vd_count);
	ps3stor_cli_printf("%stotal_pd_count:%d\n",
		format_space, dev_context->total_pd_count);
	ps3stor_cli_printf("%smax_dev_per_channel:%d\n",
		format_space, dev_context->max_dev_per_channel);
	ps3stor_cli_printf("%spd_list_buf_phys:0x%llx\n",
		format_space, dev_context->pd_list_buf_phys);
	ps3stor_cli_printf("%svd_list_buf_phys:0x%llx\n",
		format_space, dev_context->vd_list_buf_phys);
	ps3stor_cli_printf("%spd_info_buf_phys:0x%llx\n",
		format_space, dev_context->pd_info_buf_phys);
	ps3stor_cli_printf("%svd_info_buf_phys_sync:0x%llx\n",
		format_space, dev_context->vd_info_buf_phys_sync);
	ps3stor_cli_printf("%svd_info_buf_phys_async:0x%llx\n",
		format_space, dev_context->vd_info_buf_phys_async);
	ps3stor_cli_printf("\n");
}

static void ps3_event_context_detail_dump(const char *format_space,
	struct ps3_event_context *event_context)
{
	if (event_context->delay_work) {
		ps3stor_cli_printf("%sevent_delay:%d\n",
			format_space, event_context->delay_work->event_delay);
	}
	ps3stor_cli_printf("\n");
}

static void ps3_fault_context_detail_dump(const char *format_space,
	struct ps3_fault_context *fault_context)
{
	ps3stor_cli_printf("%sioc_busy:%d\n",
		format_space, fault_context->ioc_busy);
	ps3stor_cli_printf("%slast_time:%d\n",
		format_space, fault_context->last_time);
	ps3stor_cli_printf("\n");
}

static void ps3_ioc_ctrl_info_detail_dump(const char *format_space,
	struct PS3IocCtrlInfo *ctrl_info)
{
	size_t i = 0;
	ps3stor_cli_printf("%smaxVdCount:%d\n",
		format_space, ctrl_info->maxVdCount);
	ps3stor_cli_printf("%smaxPdCount:%d\n",
		format_space, ctrl_info->maxPdCount);
	ps3stor_cli_printf("%smaxSectors:%d\n",
		format_space, ctrl_info->maxSectors);
	ps3stor_cli_printf("%sPS3IocCtrlProp.enableSnapshot:%s\n",
		format_space,
		ctrl_info->properties.enableSnapshot ? "PS3_TRUE" : "PS3_FALSE");
	ps3stor_cli_printf("%sPS3IocCtrlProp.enableSoftReset:%s\n",
		format_space,
		ctrl_info->properties.enableSoftReset ?
			"PS3_TRUE" : "PS3_FALSE");
	ps3stor_cli_printf("%sPS3IocCtrlCapable.supportUnevenSpans:%s\n",
		format_space,
		ctrl_info->capabilities.supportUnevenSpans ?
			"PS3_TRUE" : "PS3_FALSE");
	ps3stor_cli_printf("%sPS3IocCtrlCapable.supportJbodSecure:%s\n",
		format_space,
		ctrl_info->capabilities.supportJbodSecure ?
			"PS3_TRUE" : "PS3_FALSE");
	ps3stor_cli_printf("%sPS3IocCtrlCapable.supportNvmePassthru:%s\n",
		format_space,
		ctrl_info->capabilities.supportNvmePassthru ?
			"PS3_TRUE" : "PS3_FALSE");
	ps3stor_cli_printf("%sPS3IocCtrlCapable.supportDirectCmd:%s\n",
		format_space,
		ctrl_info->capabilities.supportDirectCmd ?
			"PS3_TRUE" : "PS3_FALSE");
	ps3stor_cli_printf("%sPS3IocCtrlCapable.supportSataDirectCmd:%s\n",
		format_space,
		ctrl_info->capabilities.supportSataDirectCmd ?
			"PS3_TRUE" : "PS3_FALSE");
	ps3stor_cli_printf("%sPS3IocCtrlCapable.supportSataNcq:%s\n",
		format_space,
		ctrl_info->capabilities.supportSataNcq ?
			"PS3_TRUE" : "PS3_FALSE");
	ps3stor_cli_printf("%sPS3IocCtrlCapable.supportAcceleration:%s\n",
		format_space,
		ctrl_info->capabilities.supportAcceleration ?
			"PS3_TRUE" : "PS3_FALSE");
	ps3stor_cli_printf("%sscsiTaskAbortTimeout:%d\n",
		format_space, ctrl_info->scsiTaskAbortTimeout);
	ps3stor_cli_printf("%sscsiTaskResetTimeout:%d\n",
		format_space, ctrl_info->scsiTaskResetTimeout);
	ps3stor_cli_printf("%siocPerfMode:%d\n",
		format_space, ctrl_info->iocPerfMode);
	ps3stor_cli_printf("%svdQueueNum:%d\n",
		format_space, ctrl_info->vdQueueNum);
	ps3stor_cli_printf("%scancelTimeOut:%d\n",
		format_space, ctrl_info->cancelTimeOut);
	ps3stor_cli_printf("%schannelInfo channelNum:%d\n",
		format_space, ctrl_info->channelInfo.channelNum);
	for (i = 0; i < ctrl_info->channelInfo.channelNum; i++) {
		ps3stor_cli_printf("%s    channels[%d].channelType:%d\n",
			format_space, i,
			ctrl_info->channelInfo.channels[i].channelType);
		ps3stor_cli_printf("%s    channels[%d].maxDevNum:%d\n",
			format_space, i,
			ctrl_info->channelInfo.channels[i].maxDevNum);
	}

	ps3stor_cli_printf("\n");
}

static void ps3_cmd_attr_context_detail_dump(const char *format_space,
	struct ps3_cmd_attr_context *cmd_attr)
{
	ps3stor_cli_printf("%sthrottle_que_depth:%d\n",
		format_space, cmd_attr->throttle_que_depth);
	ps3stor_cli_printf("%scur_can_que:%d\n",
		format_space, cmd_attr->cur_can_que);
	ps3stor_cli_printf("%svd_io_threshold:%d\n",
		format_space, cmd_attr->vd_io_threshold);
	ps3stor_cli_printf("%sis_support_direct_cmd:%s\n",
		format_space, cmd_attr->is_support_direct_cmd ?
			"PS3_TRUE" : "PS3_FALSE");
	ps3stor_cli_printf("%snvme_page_size:%d\n",
		format_space, cmd_attr->nvme_page_size);

	ps3stor_cli_printf("\n");
}

static void ps3_instance_info_detail_dump(void)
{
	struct ps3_instance *instance = NULL;
	struct list_head *pitem = NULL;
	struct Scsi_Host *phost = NULL;
	S32 instance_state = 0;
	list_for_each(pitem, &ps3_mgmt_info_get()->instance_list_head) {
		instance = list_entry(pitem, struct ps3_instance, list_item);
		phost = instance->host;
		instance_state = atomic_read(&instance->state_machine.state);
		ps3stor_cli_printf("host_no:%d instance detail info\n", phost->host_no);
		ps3stor_cli_printf("    %s\n", namePS3InstanceState(instance_state));
		ps3stor_cli_printf("    reg_bar:0x%x\n", instance->reg_bar);
		ps3stor_cli_printf("    is_load:%s\n",
			instance->state_machine.is_load ? "PS3_TRUE" : "PS3_FALSE");
		ps3stor_cli_printf("    is_support_sync_cache:%s\n",
			instance->is_support_sync_cache ? "PS3_TRUE" : "PS3_FALSE");

		ps3stor_cli_printf("    ps3_cmd_context detail info dump\n");
		ps3_cmd_context_detail_dump("        ", &instance->cmd_context);

		ps3stor_cli_printf("    ps3_irq_context detail info dump\n");
		ps3_irq_context_detail_dump("        ", &instance->irq_context);

		ps3stor_cli_printf("    ps3_dev_context detail info dump\n");
		ps3_dev_context_detail_dump("        ", &instance->dev_context);

		ps3stor_cli_printf("    ps3_event_context detail info dump\n");
		ps3_event_context_detail_dump("        ", &instance->event_context);

		ps3stor_cli_printf("    ps3_fault_context detail info dump\n");
		ps3_fault_context_detail_dump("        ", &instance->fault_context);

		ps3stor_cli_printf("    PS3IocCtrlInfo detail info dump\n");
		ps3_ioc_ctrl_info_detail_dump("        ",
			&instance->ctrl_info);

		ps3stor_cli_printf("    ps3_cmd_attr_context detail info dump\n");
		ps3_cmd_attr_context_detail_dump("        ",
			&instance->cmd_attr);

		ps3stor_cli_printf("    ps3_dump_context detail info dump\n");
		ps3_dump_context_show("        ", instance);
	}
}

static void ps3_cli_host_and_instance_ls(int argc, char *argv[])
{
	if (argc < 2) {
		ps3stor_cli_printf("ls host/instance/all\n");
		return ;
	}

	spin_lock(&ps3_mgmt_info_get()->spin_lock_obj);
	if (strcmp(argv[1], "host") == 0) {
		ps3_host_info_detail_dump();
		goto l_out;
	}

	if (strcmp(argv[1], "instance") == 0) {
		ps3_instance_info_detail_dump();
		goto l_out;
	}

	if (strcmp(argv[1], "all") == 0) {
		ps3_host_info_detail_dump();
		ps3_instance_info_detail_dump();
		goto l_out;
	}

l_out:
	spin_unlock(&ps3_mgmt_info_get()->spin_lock_obj);
	return ;
}

static ssize_t ps3_reply_fifo_dump(struct ps3_instance *instance, U16 isr_sn,
	char *buf, ssize_t total_len, U16 index, U16 count)
{
	ssize_t len = 0;
	struct PS3ReplyWord *fifo = NULL;
	U32 i = 0;
	struct ps3_irq *irq = NULL;
	U16 start_idx = 0;
	U16 once_print_cnt = 0;
	U16 cnt = 0;

	if (isr_sn >= instance->irq_context.valid_msix_vector_count) {
		ps3stor_cli_printf("invalid isr_sn %d (max:%d)\n", isr_sn,
			instance->irq_context.valid_msix_vector_count - 1);
		return PS3_FAILED;
	}

	irq = instance->irq_context.irqs + isr_sn;
	if (irq == NULL) {
		goto l_out;
	}
	len += snprintf(buf + len, total_len - len, "irq_name:          %s\n",
		irq->name);
	len += snprintf(buf + len, total_len - len, "irqNo:             %d\n",
		irq->irqNo);
	len += snprintf(buf + len, total_len - len, "isrSN:             %d\n",
		irq->isrSN);
	len += snprintf(buf + len, total_len - len, "last_reply_idx:    %d\n",
		irq->last_reply_idx);
	len += snprintf(buf + len, total_len - len, "irq_poll_th:       %d\n",
		irq->irq_poll_sched_threshold);
	len += snprintf(buf + len, total_len - len, "is_sched_irq_poll: %d\n",
		irq->is_sched_irq_poll);
	len += snprintf(buf + len, total_len - len, "is_enable_irq:     %d\n",
		irq->is_enable_irq);

	if(index == 0) {
		start_idx = irq->last_reply_idx;
	} else {
		start_idx = index;
	}

	if(count == 0){
		once_print_cnt = 100;
	} else {
		once_print_cnt = count;
	}

	while(cnt < once_print_cnt) {
		if(i == instance->irq_context.reply_fifo_depth) {
			i = 0;
		} else {
			i = start_idx;
		}
		for ( ; i < instance->irq_context.reply_fifo_depth; ++i, ++cnt) {
			if (total_len - len < 100) {
				ps3stor_cli_printf(buf);
				memset(buf, 0, total_len);
				len = 0;
			}
			fifo = irq->reply_fifo_virt_base_addr + i;
			len += snprintf(buf + len, total_len - len,
				"reply_word:index[%u], replyFlags[0x%x], CFID[0x%x], mode[%d], retType[%d] diskType[%d]\n",
				i, fifo->retStatus, fifo->cmdFrameID, fifo->mode, fifo->retType, fifo->diskType);
			if(cnt >= once_print_cnt) {
				break;
			}
		}
	}
	ps3stor_cli_printf(buf);

	i = 0;
	LOG_DEBUG("reply fifo[%d]", irq->isrSN);
	while (i < instance->irq_context.reply_fifo_depth) {
		fifo = irq->reply_fifo_virt_base_addr + i;
		LOG_DEBUG("reply_word:index[%u], replyFlags[0x%x], CFID[0x%x] mode[%d] retType[%d] diskType[%d]\n",
			i, fifo->retStatus, fifo->cmdFrameID, fifo->mode, fifo->retType, fifo->diskType);
		++i;
	}

l_out:
	return len;
}

static void ps3_cli_reply_fifo_dump(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	char *buf = NULL;
	U16 host_no = 0;
	U16 isr_sn = 0;
	S32 ret = 0;
	U16 start_idx = 0;
	U16 count = 0;

	buf = (char*)kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (buf == NULL) {
		ps3stor_cli_printf("malloc buf failed!\n");
		goto l_malloc_failed;
	}

	if (argc < 5) {
		ps3stor_cli_printf("Too few args, must input 5 args!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no!\n");
		goto l_out;
	}

	if (strcmp(argv[3], "isr_sn") != 0) {
		ps3stor_cli_printf("isr_sn is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[4], 0, &isr_sn);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse isr_sn!\n");
		goto l_out;
	}

	if (isr_sn >= PS3_MAX_REPLY_QUE_COUNT) {
		ps3stor_cli_printf("Invalid isr_sn %d, max isr_sn %d!\n",
			isr_sn, PS3_MAX_REPLY_QUE_COUNT);
		goto l_out;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d !\n", host_no);
		goto l_out;
	}

	if(argc == 7) {
		if (strcmp(argv[5], "start_idx") == 0) {
			ret = kstrtou16(argv[6], 0, &start_idx);
			if (ret != 0) {
				ps3stor_cli_printf("Can not parse start_idx!\n");
				goto l_out;
			}
		} else if (strcmp(argv[5], "count") == 0) {
			ret = kstrtou16(argv[6], 0, &count);
			if (ret != 0) {
				ps3stor_cli_printf("Can not parse count!\n");
				goto l_out;
			}
		}
	} else if (argc == 9) {
		if (strcmp(argv[5], "start_idx") == 0) {
			ret = kstrtou16(argv[6], 0, &start_idx);
			if (ret != 0) {
				ps3stor_cli_printf("Can not parse start_idx!\n");
				goto l_out;
			}
		}
		if (strcmp(argv[7], "count") == 0) {
			ret = kstrtou16(argv[8], 0, &count);
			if (ret != 0) {
				ps3stor_cli_printf("Can not parse count!\n");
				goto l_out;
			}
		}
	}

	(void)ps3_reply_fifo_dump(instance, isr_sn, buf, PAGE_SIZE, start_idx, count);

l_out:
	kfree(buf);
l_malloc_failed:
	return;
}

static ssize_t ps3_req_head_dump(struct PS3ReqFrameHead *head,
	char *buf, ssize_t total_len, size_t len)
{
	len += snprintf(buf + len, total_len - len,
		"cmdType:        %d\n", head->cmdType);
	len += snprintf(buf + len, total_len - len,
		"cmdSubType:     %d\n", head->cmdSubType);
	len += snprintf(buf + len, total_len - len,
		"cmdFrameID:     %d\n", head->cmdFrameID);
	len += snprintf(buf + len, total_len - len,
		"control:        0x%x\n", head->control);
	len += snprintf(buf + len, total_len - len,
		"noReplyWord:        0x%x\n", head->noReplyWord);
	len += snprintf(buf + len, total_len - len,
		"dataFormat:        0x%x\n", head->dataFormat);
	len += snprintf(buf + len, total_len - len,
		"reqFrameFormat:        0x%x\n", head->reqFrameFormat);
	len += snprintf(buf + len, total_len - len,
		"mapBlockVer:        0x%x\n", head->mapBlockVer);
	len += snprintf(buf + len, total_len - len,
		"isWrite:        0x%x\n", head->isWrite);
	len += snprintf(buf + len, total_len - len,
		"devID:          0x%x\n", head->devID.diskID);
	len += snprintf(buf + len, total_len - len,
		"traceID:        %lld\n", head->traceID);
	return len;
}

static ssize_t ps3_mgr_req_frame_dump(struct ps3_cmd *cmd,
	char *buf, ssize_t total_len, size_t len)
{
	PS3MgrReqFrame_s *mgr_req = (PS3MgrReqFrame_s*)cmd->req_frame;
	if (mgr_req->reqHead.cmdType == PS3_CMD_MANAGEMENT ||
		mgr_req->reqHead.cmdType == PS3_CMD_IOCTL) {
		len = ps3_req_head_dump(&mgr_req->reqHead, buf,
			total_len, len);
		len += snprintf(buf + len, total_len - len,
			"sgeCount:      %d\n", mgr_req->sgeCount);
		len += snprintf(buf + len, total_len - len,
			"sgeOffset:     %d\n", mgr_req->sgeOffset);
		len += snprintf(buf + len, total_len - len,
			"syncFlag:      %d\n", mgr_req->syncFlag);
		len += snprintf(buf + len, total_len - len,
			"timeout:       %d\n", mgr_req->timeout);
		len += snprintf(buf + len, total_len - len,
			"abortFlag:      %d\n", mgr_req->abortFlag);
		len += snprintf(buf + len, total_len - len,
			"pendingFlag:    %d\n", mgr_req->pendingFlag);
	} else if (mgr_req->reqHead.cmdType == PS3_CMD_SCSI_TASK_MANAGEMENT){
		PS3MgrTaskReqFrame_s *task_req =
			(PS3MgrTaskReqFrame_s*)cmd->req_frame;
		len = ps3_req_head_dump(&task_req->reqHead, buf,
			total_len, len);
		len += snprintf(buf + len, total_len - len,
			"taskID:         %d\n", task_req->taskID);
		len += snprintf(buf + len, total_len - len,
			"abortedCmdType: %d\n", task_req->abortedCmdType);
	} else {
		PS3FrontEndReqFrame_s *fe_req =
			(PS3FrontEndReqFrame_s*)cmd->req_frame;
		len = ps3_req_head_dump(&fe_req->reqHead, buf,
			total_len, len);
		len += snprintf(buf + len, total_len - len,
			"sgeCount:	%d\n", fe_req->sgeCount);
	}
	return len;
}

static ssize_t ps3_req_frame_dump(struct ps3_cmd *cmd,
	char *buf, ssize_t total_len, size_t len)
{
	if (PS3_CMD_TYPE_IS_RW(cmd->cmd_word.type)) {
		if (cmd->req_frame->mgrReq.reqHead.reqFrameFormat == PS3_REQFRAME_FORMAT_FRONTEND) {
			PS3FrontEndReqFrame_s *fe_req =
				(PS3FrontEndReqFrame_s*)cmd->req_frame;
			len = ps3_req_head_dump(&fe_req->reqHead, buf,
				total_len, len);
			len += snprintf(buf + len, total_len - len,
				"dataXferLen:   %d\n", fe_req->dataXferLen);
			len += snprintf(buf + len, total_len - len,
				"sgeCount:      %d\n", fe_req->sgeCount);
		} else {
			PS3HwReqFrame_s *hw_req =
				(PS3HwReqFrame_s*)cmd->req_frame;
			len += snprintf(buf + len, total_len - len,
				"traceID:      %llu\n", hw_req->reqHead.traceID);
			len += snprintf(buf + len, total_len - len,
				"vlba:         0x%llx\n",
				hw_req->softwareZone.virtDiskLba);
			len += snprintf(buf + len, total_len - len,
				"numBlocks:    0x%u\n",
				hw_req->softwareZone.numBlocks);
			len += snprintf(buf + len, total_len - len,
				"opcode:       0x%u\n",
				hw_req->softwareZone.opcode);
			len += snprintf(buf + len, total_len - len,
				"sglOffset:    0x%u\n",
				hw_req->softwareZone.sglOffset);
			len += snprintf(buf + len, total_len - len,
				"sglFormat:    0x%u\n",
				hw_req->softwareZone.sglFormat);
			len += snprintf(buf + len, total_len - len,
				"isResendCmd:  0x%u\n",
				hw_req->softwareZone.isResendCmd);
			len += snprintf(buf + len, total_len - len,
				"subOpcode:    0x%u\n",
				hw_req->softwareZone.subOpcode);
			len += snprintf(buf + len, total_len - len,
				"sgeCount:     0x%u\n",
				hw_req->softwareZone.sgeCount);
		}
	} else if (cmd->cmd_word.type == PS3_CMDWORD_TYPE_MGR){
		len = ps3_mgr_req_frame_dump(cmd,  buf, total_len, len);
	} else {
		len += snprintf(buf + len, total_len - len,
			"req frame is null\n");
	}

	return len;
}

static ssize_t ps3_cmd_dump(struct ps3_instance *instance, U16 CFID,
	char *buf, ssize_t total_len)
{
	ssize_t len = 0;
	struct ps3_cmd *cmd = NULL;

	cmd = (struct ps3_cmd*)instance->cmd_context.cmd_buf[CFID];
	if (cmd == NULL) {
		goto l_out;
	}
	len += snprintf(buf + len, total_len - len, "----dump ps3_cmd----\n");
	len += snprintf(buf + len, total_len - len, "cmdFrameID:        %d\n",
		CFID);
	len += snprintf(buf + len, total_len - len, "trace_id:          %lld\n",
		cmd->trace_id);
	len += snprintf(buf + len, total_len - len, "no_reply_word:     %d\n",
		cmd->no_reply_word);
	len += snprintf(buf + len, total_len - len, "is_retry_cmd:      %d\n",
		cmd->io_attr.is_retry_cmd);
	len += snprintf(buf + len, total_len - len, "direct_flag:       %d\n",
		cmd->io_attr.direct_flag);
	len += snprintf(buf + len, total_len - len, "dev_type:          %s\n",
		namePS3DevType((enum PS3DevType)cmd->io_attr.dev_type));
	len += snprintf(buf + len, total_len - len, "rw_flag:           %d\n",
		cmd->io_attr.rw_flag);

	len += snprintf(buf + len, total_len - len, "------cmd_word------\n");
	len += snprintf(buf + len, total_len - len, "type:              %d\n",
		cmd->cmd_word.type);
	len += snprintf(buf + len, total_len - len, "direct:            %d\n",
		cmd->cmd_word.direct);
	len += snprintf(buf + len, total_len - len, "isrSN:             %d\n",
		cmd->cmd_word.isrSN);
	len += snprintf(buf + len, total_len - len, "phyDiskID:         %d\n",
		cmd->cmd_word.phyDiskID);
	len += snprintf(buf + len, total_len - len, "queID:             %d\n",
		cmd->cmd_word.qMask);
	len += snprintf(buf + len, total_len - len, "cmdFrameID:        %d\n",
		cmd->cmd_word.cmdFrameID);
	len += snprintf(buf + len, total_len - len, "virtDiskID:        %d\n",
		cmd->cmd_word.virtDiskID);

	len += snprintf(buf + len, total_len - len, "------req_frame------\n");
	if (cmd->cmd_word.cmdFrameID == 0) {
		len += snprintf(buf + len, total_len - len, "--cmd is null--\n");
		goto l_out;
	}

	len = ps3_req_frame_dump(cmd, buf, total_len, len);

l_out:
	return len;
}

static void ps3_cli_cmd_dump(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	char *buf = NULL;
	U16 host_no = 0;
	U16 cmdFrameID = 0;
	S32 ret = 0;

	buf = (char*)kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (buf == NULL) {
		ps3stor_cli_printf("malloc buf failed!\n");
		goto l_malloc_failed;
	}

	if (argc < 5) {
		ps3stor_cli_printf("Too few args, must input 5 args!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no!\n");
		goto l_out;
	}

	if (strcmp(argv[3], "cmd_frame_id") != 0) {
		ps3stor_cli_printf("cmd_frame_id is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[4], 0, &cmdFrameID);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no!\n");
		goto l_out;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d !\n", host_no);
		goto l_out;
	}

	if (cmdFrameID >= instance->cmd_context.max_cmd_count) {
		ps3stor_cli_printf("Invalid cmd_frame_id %d, max id %d!\n",
			cmdFrameID, instance->cmd_context.max_cmd_count);
		goto l_out;
	}

	(void)ps3_cmd_dump(instance, cmdFrameID, buf, PAGE_SIZE);
	ps3stor_cli_printf(buf);

l_out:
	kfree(buf);
l_malloc_failed:
	return;
}

static ssize_t ps3_io_statis_to_str(struct ps3_dev_io_statis *disk_io_statis,
	char *buf, ssize_t total_len)
{
	ssize_t len = 0;
	const U32 temp_array_len = 256;
	char temp[256] = {0};
	ssize_t temp_len = 0;

	(void)disk_io_statis;
	(void)buf;

	memset(temp, 0, temp_array_len);
	temp_len = snprintf(temp, temp_array_len, "%-20s:%llu\n", "readSendCnt",
		(U64)atomic64_read(&disk_io_statis->read_send_cnt));
	if ((len + temp_len) > total_len) {
		goto l_out;
	}
	len += snprintf(buf + len, total_len - len, "%-20s:%llu\n", "readSendCnt",
		(U64)atomic64_read(&disk_io_statis->read_send_cnt));

	memset(temp, 0, temp_array_len);
	temp_len = snprintf(temp, temp_array_len, "%-20s:%llu\n", "readSendOkCnt",
		(U64)atomic64_read(&disk_io_statis->read_send_ok_cnt));
	if ((len + temp_len) > total_len) {
		goto l_out;
	}
	len += snprintf(buf + len, total_len - len, "%-20s:%llu\n", "readSendOkCnt",
		(U64)atomic64_read(&disk_io_statis->read_send_ok_cnt));

	memset(temp, 0, temp_array_len);
	temp_len = snprintf(temp, temp_array_len, "%-20s:%llu\n", "readOutStandCnt",
		(U64)atomic64_read(&disk_io_statis->read_send_wait_cnt));
	if ((len + temp_len) > total_len) {
		goto l_out;
	}
	len += snprintf(buf + len, total_len - len, "%-20s:%llu\n", "readOutStandCnt",
		(U64)atomic64_read(&disk_io_statis->read_send_wait_cnt));

	memset(temp, 0, temp_array_len);
	temp_len = snprintf(temp, temp_array_len, "%-20s:%llu\n", "readRecvCnt",
		(U64)atomic64_read(&disk_io_statis->read_recv_cnt));
	if ((len + temp_len) > total_len) {
		goto l_out;
	}
	len += snprintf(buf + len, total_len - len, "%-20s:%llu\n", "readRecvCnt",
		(U64)atomic64_read(&disk_io_statis->read_recv_cnt));

	memset(temp, 0, temp_array_len);
	temp_len = snprintf(temp, temp_array_len, "%-20s:%llu\n", "readRecvOkCnt",
		(U64)atomic64_read(&disk_io_statis->read_recv_ok_cnt));
	if ((len + temp_len) > total_len) {
		goto l_out;
	}
	len += snprintf(buf + len, total_len - len, "%-20s:%llu\n", "readRecvOkCnt",
		(U64)atomic64_read(&disk_io_statis->read_recv_ok_cnt));

	memset(temp, 0, temp_array_len);
	temp_len = snprintf(temp, temp_array_len, "%-20s:%llu\n", "readOkBytes",
		(U64)atomic64_read(&disk_io_statis->read_ok_bytes));
	if ((len + temp_len) > total_len) {
		goto l_out;
	}
	len += snprintf(buf + len, total_len - len, "%-20s:%llu\n", "readOkBytes",
		(U64)atomic64_read(&disk_io_statis->read_ok_bytes));

	memset(temp, 0, temp_array_len);
	temp_len = snprintf(temp, temp_array_len, "%-20s:%llu\n", "writeSendCnt",
		(U64)atomic64_read(&disk_io_statis->write_send_cnt));
	if ((len + temp_len) > total_len) {
		goto l_out;
	}
	len += snprintf(buf + len, total_len - len, "%-20s:%llu\n", "writeSendCnt",
		(U64)atomic64_read(&disk_io_statis->write_send_cnt));

	memset(temp, 0, temp_array_len);
	temp_len = snprintf(temp, temp_array_len, "%-20s:%llu\n", "writeSendOkCnt",
		(U64)atomic64_read(&disk_io_statis->write_send_ok_cnt));
	if ((len + temp_len) > total_len) {
		goto l_out;
	}
	len += snprintf(buf + len, total_len - len, "%-20s:%llu\n", "writeSendOkCnt",
		(U64)atomic64_read(&disk_io_statis->write_send_ok_cnt));

	memset(temp, 0, temp_array_len);
	temp_len = snprintf(temp, temp_array_len, "%-20s:%llu\n", "writeOutStandCnt",
		(U64)atomic64_read(&disk_io_statis->write_send_wait_cnt));
	if ((len + temp_len) > total_len) {
		goto l_out;
	}
	len += snprintf(buf + len, total_len - len, "%-20s:%llu\n", "writeOutStandCnt",
		(U64)atomic64_read(&disk_io_statis->write_send_wait_cnt));


	memset(temp, 0, temp_array_len);
	temp_len = snprintf(temp, temp_array_len, "%-20s:%llu\n", "writeRecvCnt",
		(U64)atomic64_read(&disk_io_statis->write_recv_cnt));
	if ((len + temp_len) > total_len) {
		goto l_out;
	}
	len += snprintf(buf + len, total_len - len, "%-20s:%llu\n", "writeRecvCnt",
		(U64)atomic64_read(&disk_io_statis->write_recv_cnt));

	memset(temp, 0, temp_array_len);
	temp_len = snprintf(temp, temp_array_len, "%-20s:%llu\n", "writeRecvOkCnt",
		(U64)atomic64_read(&disk_io_statis->write_recv_ok_cnt));
	if ((len + temp_len) > total_len) {
		goto l_out;
	}
	len += snprintf(buf + len, total_len - len, "%-20s:%llu\n", "writeRecvOkCnt",
		(U64)atomic64_read(&disk_io_statis->write_recv_ok_cnt));

	memset(temp, 0, temp_array_len);
	temp_len = snprintf(temp, temp_array_len, "%-20s:%llu\n\n", "writeOkBytes",
		(U64)atomic64_read(&disk_io_statis->write_ok_bytes));
	if ((len + temp_len) > total_len) {
		goto l_out;
	}
	len += snprintf(buf + len, total_len - len, "%-20s:%llu\n\n", "writeOkBytes",
		(U64)atomic64_read(&disk_io_statis->write_ok_bytes));

l_out:
	return len;
}

static ssize_t ps3_io_statis_detail_dump(struct ps3_instance *instance,
	char *buf, ssize_t total_len)
{
	ssize_t len = 0;
	struct scsi_device *sdev = NULL;
	struct ps3_scsi_priv_data *scsi_priv = NULL;
	struct ps3_dev_io_statis disk_io_statis;
	U32 i = 0;
	const U32 temp_array_len = 256;
	char temp[256] = {0};
	ssize_t temp_len = 0;
	U8 dev_type = 0;

	if (!instance || !buf || total_len <= 0) {
		ps3stor_cli_printf("invalid parameters");
		return 0;
	}

	memset(temp, 0, temp_array_len);
	temp_len = snprintf(temp, temp_array_len, "%s\n", "disk io statistics detail:");
	if ((len + temp_len) > total_len) {
		goto l_out;
	}
	len += snprintf(buf + len, total_len - len, "%s\n", "disk io statistics detail:");

	list_for_each_entry(sdev, &instance->host->__devices, siblings) {
		if (scsi_device_get(sdev))
			continue;

		ps3_mutex_lock(&instance->dev_context.dev_priv_lock);
		scsi_priv = (struct ps3_scsi_priv_data*)sdev->hostdata;
		if (scsi_priv == NULL) {
			ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);
			scsi_device_put(sdev);
			continue;
		}

		memset(&disk_io_statis, 0, sizeof(struct ps3_dev_io_statis));
		memcpy(&disk_io_statis, &scsi_priv->statis, sizeof(struct ps3_dev_io_statis));
		dev_type = scsi_priv->dev_type;
		ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);

		memset(temp, 0, temp_array_len);
		temp_len = snprintf(temp, temp_array_len, "%-20s:%d\n", "index", i);
		if ((len + temp_len) > total_len) {
			scsi_device_put(sdev);
			goto l_out;
		}
		len += snprintf(buf + len, total_len - len, "%-20s:%d\n", "index", i);

		memset(temp, 0, temp_array_len);
		temp_len = snprintf(temp, temp_array_len, "%-20s:[%d,%d]\n", "diskId",
			sdev->channel, sdev->id);
		if ((len + temp_len) > total_len) {
			scsi_device_put(sdev);
			goto l_out;
		}
		len += snprintf(buf + len, total_len - len, "%-20s:[%d,%d]\n", "diskId",
			sdev->channel, sdev->id);

		memset(temp, 0, temp_array_len);
		temp_len = snprintf(temp, temp_array_len, "%-20s:%s\n", "diskType",
			(dev_type == PS3_DEV_TYPE_VD ? "VD" : "PD"));
		if ((len + temp_len) > total_len) {
			scsi_device_put(sdev);
			goto l_out;
		}
		len += snprintf(buf + len, total_len - len, "%-20s:%s\n", "diskType",
			(dev_type == PS3_DEV_TYPE_VD ? "VD" : "PD"));

		if (len >= total_len) {
			scsi_device_put(sdev);
			break;
		}
		len += ps3_io_statis_to_str(&disk_io_statis, buf + len, total_len - len);

		scsi_device_put(sdev);
		i++;
	}

l_out:
	return len;
}

static void ps3_io_statis_clear_by_target(struct ps3_instance *instance,
	U32 channel, U32 target)
{
	struct scsi_device *sdev = NULL;

	if (!instance) {
		ps3stor_cli_printf("invalid parameters");
		return ;
	}

	sdev = scsi_device_lookup(instance->host, channel, target, 0);
	if (sdev){
		if (sdev->channel == channel && sdev->id == target) {
			ps3_io_statis_clear(sdev);
		}

		scsi_device_put(sdev);
	}
}

static ssize_t ps3_io_statis_summary_dump(struct ps3_instance *instance,
	char *buf, ssize_t total_len)
{
	ssize_t len = 0;
	struct scsi_device *sdev = NULL;
	struct ps3_scsi_priv_data *scsi_priv = NULL;
	struct ps3_dev_io_statis disk_io_statis;
	struct ps3_dev_io_statis disk_io_statis_total;
	U32 pd_cnt = 0, vd_cnt = 0;
	const U32 temp_array_len = 256;
	char temp[256] = {0};
	ssize_t temp_len = 0;
	U8 dev_type = 0;

	if (!instance || !buf || total_len <= 0) {
		ps3stor_cli_printf("invalid parameters");
		return 0;
	}

	ps3_dev_io_statis_init(&disk_io_statis_total);
	list_for_each_entry(sdev, &instance->host->__devices, siblings) {
		if (scsi_device_get(sdev))
			continue;

		ps3_mutex_lock(&instance->dev_context.dev_priv_lock);
		scsi_priv = (struct ps3_scsi_priv_data*)sdev->hostdata;
		if (scsi_priv == NULL) {
			ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);
			scsi_device_put(sdev);
			continue;
		}
		memset(&disk_io_statis, 0, sizeof(struct ps3_dev_io_statis));
		memcpy(&disk_io_statis, &scsi_priv->statis, sizeof(struct ps3_dev_io_statis));

		dev_type = scsi_priv->dev_type;
		ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);

		atomic64_add(atomic64_read(&disk_io_statis.read_send_cnt), &disk_io_statis_total.read_send_cnt);
		atomic64_add(atomic64_read(&disk_io_statis.read_send_ok_cnt), &disk_io_statis_total.read_send_ok_cnt);
		atomic64_add(atomic64_read(&disk_io_statis.read_send_err_cnt), &disk_io_statis_total.read_send_err_cnt);
		atomic64_add(atomic64_read(&disk_io_statis.read_send_wait_cnt), &disk_io_statis_total.read_send_wait_cnt);
		atomic64_add(atomic64_read(&disk_io_statis.read_recv_cnt), &disk_io_statis_total.read_recv_cnt);
		atomic64_add(atomic64_read(&disk_io_statis.read_recv_err_cnt), &disk_io_statis_total.read_recv_err_cnt);
		atomic64_add(atomic64_read(&disk_io_statis.read_recv_ok_cnt), &disk_io_statis_total.read_recv_ok_cnt);
		atomic64_add(atomic64_read(&disk_io_statis.read_ok_bytes), &disk_io_statis_total.read_ok_bytes);

		atomic64_add(atomic64_read(&disk_io_statis.write_send_cnt), &disk_io_statis_total.write_send_cnt);
		atomic64_add(atomic64_read(&disk_io_statis.write_send_ok_cnt), &disk_io_statis_total.write_send_ok_cnt);
		atomic64_add(atomic64_read(&disk_io_statis.write_send_err_cnt), &disk_io_statis_total.write_send_err_cnt);
		atomic64_add(atomic64_read(&disk_io_statis.write_send_wait_cnt), &disk_io_statis_total.write_send_wait_cnt);
		atomic64_add(atomic64_read(&disk_io_statis.write_recv_cnt), &disk_io_statis_total.write_recv_cnt);
		atomic64_add(atomic64_read(&disk_io_statis.write_recv_err_cnt), &disk_io_statis_total.write_recv_err_cnt);
		atomic64_add(atomic64_read(&disk_io_statis.write_recv_ok_cnt), &disk_io_statis_total.write_recv_ok_cnt);
		atomic64_add(atomic64_read(&disk_io_statis.write_ok_bytes), &disk_io_statis_total.write_ok_bytes);

		if (dev_type == PS3_DEV_TYPE_VD) {
			vd_cnt++;
		} else {
			pd_cnt++;
		}

		scsi_device_put(sdev);
	}

	memcpy(&disk_io_statis, &disk_io_statis_total, sizeof(struct ps3_dev_io_statis));

	memset(temp, 0, temp_array_len);
	temp_len = snprintf(temp, temp_array_len, "%s\n", "disk io statistics summary:");
	if ((len + temp_len) > total_len) {
		goto l_out;
	}
	len += snprintf(buf + len, total_len - len, "%s\n", "disk io statistics summary:");

	memset(temp, 0, temp_array_len);
	temp_len = snprintf(temp, temp_array_len, "%-20s:%d\n", "VD count", vd_cnt);
	if ((len + temp_len) > total_len) {
		goto l_out;
	}
	len += snprintf(buf + len, total_len - len, "%-20s:%d\n", "VD count", vd_cnt);

	memset(temp, 0, temp_array_len);
	temp_len = snprintf(temp, temp_array_len, "%-20s:%d\n", "PD count", pd_cnt);
	if ((len + temp_len) > total_len) {
		goto l_out;
	}
	len += snprintf(buf + len, total_len - len, "%-20s:%d\n", "PD count", pd_cnt);

	if (len >= total_len) {
		goto l_out;
	}
	len += ps3_io_statis_to_str(&disk_io_statis, buf, total_len - len);

l_out:
	return len;
}

void ps3_io_statis_dump_cli_cb_test(U8 detail)
{
	struct ps3_instance *instance = NULL;
	struct list_head *pitem = NULL;
	char *buf = NULL;

	buf = (char*)kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (buf == NULL) {
		ps3stor_cli_printf("malloc buf failed!\n");
		goto l_malloc_failed;
	}

	if (detail) {
		list_for_each(pitem, &ps3_mgmt_info_get()->instance_list_head) {
			instance = list_entry(pitem, struct ps3_instance, list_item);
			if (instance) {
				(void)ps3_io_statis_detail_dump(instance, buf, PAGE_SIZE);
				break;
			}
		}
	} else {
		list_for_each(pitem, &ps3_mgmt_info_get()->instance_list_head) {
			instance = list_entry(pitem, struct ps3_instance, list_item);
			if (instance) {
				(void)ps3_io_statis_summary_dump(instance, buf, PAGE_SIZE);
				break;
			}
		}
	}

	ps3stor_cli_printf(buf);
	LOG_DEBUG("buf = %s\n",buf);

	kfree(buf);
	buf = NULL;

l_malloc_failed:
	return;
}

static void ps3_io_statis_dump_cli_cb(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	struct list_head *pitem = NULL;
	char *buf = NULL;

	buf = (char*)kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (buf == NULL) {
		ps3stor_cli_printf("malloc buf failed!\n");
		goto l_malloc_failed;
	}

	if (argc < 4) {
		ps3stor_cli_printf("Too few args, must input 4 args!\n");
		goto l_out;
	}

	if (strcmp(argv[0], "show") != 0) {
		ps3stor_cli_printf("invalid arg 0 %s!\n", argv[0]);
		goto l_out;
	}

	if (strcmp(argv[1], "io") != 0) {
		ps3stor_cli_printf("invalid arg 1 %s!\n", argv[1]);
		goto l_out;
	}

	if (strcmp(argv[2], "statis") != 0) {
		ps3stor_cli_printf("invalid arg 2 %s!\n", argv[2]);
		goto l_out;
	}

	if (strcmp(argv[3], "detail") == 0) {
		list_for_each(pitem, &ps3_mgmt_info_get()->instance_list_head) {
			instance = list_entry(pitem, struct ps3_instance, list_item);
			if (instance) {
				(void)ps3_io_statis_detail_dump(instance, buf, PAGE_SIZE);
				break;
			}
		}
	} else {
		list_for_each(pitem, &ps3_mgmt_info_get()->instance_list_head) {
			instance = list_entry(pitem, struct ps3_instance, list_item);
			if (instance) {
				(void)ps3_io_statis_summary_dump(instance, buf, PAGE_SIZE);
				break;
			}
		}
	}

	ps3stor_cli_printf(buf);
	LOG_INFO("buf: %s\n",  buf);

l_out:
	kfree(buf);
	buf = NULL;

l_malloc_failed:
	return;
}

static void ps3_io_statis_clear_cli_cb(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	struct list_head *pitem = NULL;
	U32 channel = 0, target = 0;
	S32 ret = 0;

	if (argc < 5) {
		ps3stor_cli_printf("Too few args, must input 5 args!\n");
		goto l_out;
	}

	if (strcmp(argv[0], "clear") != 0) {
		ps3stor_cli_printf("invalid arg 0 %s!\n", argv[0]);
		goto l_out;
	}

	if (strcmp(argv[1], "io") != 0) {
		ps3stor_cli_printf("invalid arg 1 %s!\n", argv[1]);
		goto l_out;
	}

	if (strcmp(argv[2], "statis") != 0) {
		ps3stor_cli_printf("invalid arg 2 %s!\n", argv[2]);
		goto l_out;
	}

	ret = kstrtouint(argv[3], 0, &channel);
	if (ret != 0) {
		ps3stor_cli_printf("Invalid channel %s!\n", argv[3]);
		goto l_out;
	}
	ret = kstrtouint(argv[4], 0, &target);
	if (ret != 0) {
		ps3stor_cli_printf("Invalid target %s!\n", argv[4]);
		goto l_out;
	}

	list_for_each(pitem, &ps3_mgmt_info_get()->instance_list_head) {
		instance = list_entry(pitem, struct ps3_instance, list_item);
		if (instance) {
			ps3_io_statis_clear_by_target(instance, channel, target);
			ps3stor_cli_printf("clear host %d channel %d target %d io statistics\n",
				instance->host->host_no, channel, target);
			break;
		}
	}

l_out:
	return;
}
static ssize_t ps3_hardreset_cnt_show(struct ps3_instance *instance,
	char *buf, ssize_t total_len)
{
	ssize_t len = 0;

	if (!instance || !buf || total_len <= 0) {
		ps3stor_cli_printf("invalid parameters");
		goto l_out;
	}

	len += snprintf(buf + len, total_len - len, "%s:%u\n", "hard reset cnt",
		instance->recovery_context->hardreset_count);

l_out:
	return len;
}

static ssize_t ps3_hardreset_cnt_clear(struct ps3_instance *instance,
	char *buf, ssize_t total_len)
{
	ssize_t len = 0;
	if (!instance || !buf || total_len <= 0) {
		ps3stor_cli_printf("invalid parameters");
		goto l_out;
	}

	instance->recovery_context->hardreset_count = 0;
	len += snprintf(buf + len, total_len - len, "%s:%u\n",
		"hard reset cnt",instance->recovery_context->hardreset_count);
l_out:
	return len;
}

static void ps3_hardreset_cnt_show_cli_cb(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	char *buf = NULL;
	U16 host_no = 0;
	S32 ret = 0;

	buf = (char*)kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (buf == NULL) {
		ps3stor_cli_printf("malloc buf failed!\n");
		goto l_malloc_failed;
	}

	if (argc < 3) {
		ps3stor_cli_printf("Too few args for register dump cli cmd!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no for register dump cmd!\n");
		goto l_out;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d for register dump cmd!\n",
			host_no);
		goto l_out;
	}

	(void)ps3_hardreset_cnt_show(instance, buf, PAGE_SIZE);
	ps3stor_cli_printf(buf);
	LOG_INFO("buf: %s\n",  buf);

l_out:
	kfree(buf);
	buf = NULL;

l_malloc_failed:
	return;
}

static void ps3_hardreset_cnt_clear_cli_cb(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	char *buf = NULL;
	U16 host_no = 0;
	S32 ret = 0;
	buf = (char*)kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (buf == NULL) {
		ps3stor_cli_printf("malloc buf failed!\n");
		goto l_malloc_failed;
	}

	if (argc < 3) {
		ps3stor_cli_printf("Too few args for register dump cli cmd!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no for register dump cmd!\n");
		goto l_out;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d for register dump cmd!\n",
			host_no);
		goto l_out;
	}
	(void)ps3_hardreset_cnt_clear(instance, buf, PAGE_SIZE);
	ps3stor_cli_printf(buf);
	LOG_INFO("buf: %s\n",  buf);

l_out:
	kfree(buf);
	buf = NULL;

l_malloc_failed:
	return;

}

static void ps3_cli_stop_all_instance(void)
{
	struct ps3_instance *instance = NULL;
	struct list_head *pitem = NULL;
	struct ps3_cmd *cmd = NULL;
	struct scsi_cmnd *s_cmd = NULL;
	struct ps3_scsi_priv_data *data = NULL;
	U32 index = 0;
	list_for_each(pitem, &ps3_mgmt_info_get()->instance_list_head) {
		instance = list_entry(pitem, struct ps3_instance, list_item);
		if (instance == NULL) {
			continue;
		}

		ps3_instance_state_transfer_to_dead(instance);
		instance->ioc_adpter->irq_disable(instance);
		ps3_irqs_sync(instance);

		ps3_r1x_conflict_queue_clean_all(instance,
			PS3_SCSI_RESULT_HOST_STATUS(DID_NO_CONNECT), PS3_TRUE);
#ifdef PS3_SUPPORT_INJECT
		ps3_inject_clear();
#endif
		for (index = 0; index < instance->cmd_context.max_scsi_cmd_count; index++) {
			cmd = instance->cmd_context.cmd_buf[index];
			if (cmd->scmd != NULL) {
				PS3_IO_OUTSTAND_DEC(instance, cmd->scmd);
				PS3_VD_OUTSTAND_DEC(instance, cmd->scmd);
				PS3_IO_BACK_ERR_INC(instance, cmd->scmd);
				PS3_DEV_BUSY_DEC(cmd->scmd);
				cmd->scmd->result = ((S32)PS3_SCSI_RESULT_HOST_STATUS(DID_NO_CONNECT));
				s_cmd = cmd->scmd;
				ps3_scsi_dma_unmap(cmd);
				data = (struct ps3_scsi_priv_data *)cmd->scmd->device->hostdata;
				if(likely(data != NULL)){
					ps3_r1x_write_unlock(&data->lock_mgr, cmd);
				}
				ps3_scsi_cmd_free(cmd);
				SCMD_IO_DONE(s_cmd);
			}
		}

		for (index = instance->cmd_context.max_scsi_cmd_count;
			index < instance->cmd_context.max_cmd_count; index++) {
			cmd = instance->cmd_context.cmd_buf[index];
			if (cmd->req_frame->mgrReq.reqHead.noReplyWord == PS3_CMD_WORD_NO_REPLY_WORD) {
				cmd->resp_frame->normalRespFrame.respStatus = PS3_STATUS_DEVICE_NOT_FOUND;
			} else if (cmd->cmd_receive_cb != NULL) {
				cmd->resp_frame->normalRespFrame.respStatus = PS3_STATUS_DEVICE_NOT_FOUND;
				cmd->cmd_receive_cb(cmd, PS3_REPLY_WORD_FLAG_FAIL);
			}
		}

		cancel_delayed_work_sync(&instance->event_context.delay_work->event_work);
		ps3_watchdog_stop(instance);

		instance->ioc_adpter->irq_enable(instance);
		ps3_irqpolls_enable(instance);

		ps3stor_cli_printf("host_no:%d instance stopped!\n", instance->host->host_no);
	}
}

static void ps3_cli_force_to_stop(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	spin_lock(&ps3_mgmt_info_get()->spin_lock_obj);

	ps3stor_cli_printf("force_to_stop begain.... !\n");
	ps3_cli_stop_all_instance();
	ps3stor_cli_printf("force_to_stop success !\n");

	spin_unlock(&ps3_mgmt_info_get()->spin_lock_obj);
}

static void ps3_debug_mem_dump(struct ps3_instance *ins, U16 entry, U32 len)
{
	U32 i = 0;
	const U32 once_dump_len = 1024; 
	char buf[256] = { 0 }; 

	for (; i < ins->debug_context.debug_mem_vaddr[entry].debugMemSize; ++i) {
		if (ins->debug_context.debug_mem_vaddr[entry].debugMemAddr == 0 || len == 0) {
			break;
		}
		memset(buf, '\0', sizeof(buf));
		if (len <= once_dump_len) {
			(void)snprintf(buf, sizeof(buf),"base addr[0x%llx] len[%u]\n",
				ins->debug_context.debug_mem_vaddr[entry].debugMemAddr +
				i * once_dump_len, len);
			DATA_DUMP(((U8*)(ins->debug_context.debug_mem_vaddr[entry].debugMemAddr + i * once_dump_len)),
				len, buf);
			break;
		} else {
			len -= once_dump_len;
			(void)snprintf(buf, sizeof(buf),"base addr[0x%llx] len[%u]\n",
				ins->debug_context.debug_mem_vaddr[entry].debugMemAddr +
				i * once_dump_len, once_dump_len);
			DATA_DUMP(((U8*)(ins->debug_context.debug_mem_vaddr[entry].debugMemAddr + i * once_dump_len)),
				once_dump_len, buf);
		}
	}
	return;
}

static void ps3_debug_mem_write(struct ps3_instance *ins, U16 entry, U32 len)
{
	const U8 l_value = 0x55;
	const U8 h_value = 0xaa;
	U32 i = 0;

	for (i = 0; i < len; i+=2) {
		*((U8*)ins->debug_context.debug_mem_vaddr[entry].debugMemAddr + i) = l_value;
		*((U8*)ins->debug_context.debug_mem_vaddr[entry].debugMemAddr + i + 1) = h_value;
	}

	return;
}

static void ps3_debug_mem_rw_cli_cb(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	U16 host_no = 0;
	U16 entry = 0;
	U16 dir = 0; 
	U32 len = 0;
	S32 ret = 0;

	if (argc < 9) {
		ps3stor_cli_printf("Too few args, must input 9 args!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no!\n");
		goto l_out;
	}
	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %u !\n", host_no);
		goto l_out;
	}

	if (strcmp(argv[3], "entry_index") != 0) {
		ps3stor_cli_printf("entry_index is needed!\n");
		goto l_out;
	}
	ret = kstrtou16(argv[4], 0, &entry);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse entry!\n");
		goto l_out;
	}

	if (instance->debug_context.debug_mem_array_num == 0) {
		ps3stor_cli_printf("host_no %d not support debug mem!\n", host_no);
		goto l_out;
	}

	if (entry >= instance->debug_context.debug_mem_array_num) {
		ps3stor_cli_printf("Invalid entry %u !\n", entry);
		goto l_out;
	}

	if (strcmp(argv[5], "dir") != 0) {
		ps3stor_cli_printf("dir is needed!\n");
		goto l_out;
	}
	ret = kstrtou16(argv[6], 0, &dir);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse entry!\n");
		goto l_out;
	}
	if (!(dir == 0 || dir == 1)) {
		ps3stor_cli_printf("Invalid dir %u, need is 0 or 1 !\n", dir);
		goto l_out;
	}

	if (strcmp(argv[7], "length") != 0) {
		ps3stor_cli_printf("length is needed!\n");
		goto l_out;
	}
	ret = kstrtouint(argv[8], 0, &len);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse entry!\n");
		goto l_out;
	}
	if (len > (instance->debug_context.debug_mem_vaddr[entry].debugMemSize * 1024)) {
		ps3stor_cli_printf("len over mem [%u], max[%u]!\n", len,
			instance->debug_context.debug_mem_vaddr[entry].debugMemSize * 1024);
		goto l_out;
	}

	if (dir == 1) {
		ps3_debug_mem_write(instance, entry, len); 
 	} else {
		ps3_debug_mem_dump(instance, entry, len);
	}

l_out:
	return;
}

static void ps3_debug_mem_para_dump(struct ps3_instance *instance)
{
	struct ps3_debug_context *cxt = &instance->debug_context;
	U32 i = 0;
	ps3stor_cli_printf("debug mem array num %u\n", cxt->debug_mem_array_num);
	if (cxt->debug_mem_array_num == 0) {
		goto l_out;
	}

	for (; i < cxt->debug_mem_array_num; ++i) {
		ps3stor_cli_printf("entry[%u] vaddr[0x%llx] dma[0x%llx] max_size[%u]KB\n",
			i, cxt->debug_mem_vaddr[i].debugMemAddr,
			cxt->debug_mem_buf[i].debugMemAddr,
			cxt->debug_mem_buf[i].debugMemSize);
	}
l_out:
	return;
}

static void ps3_debug_mem_para_cli_cb(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	U16 host_no = 0;
	S32 ret = 0;

	if (argc < 3) {
		ps3stor_cli_printf("Too few args, must input 3 args!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no!\n");
		goto l_out;
	}
	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %u !\n", host_no);
		goto l_out;
	}

	if (instance->debug_context.debug_mem_array_num == 0) {
		ps3stor_cli_printf("host_no %d not support debug mem!\n", host_no);
		goto l_out;
	}

	ps3_debug_mem_para_dump(instance);

l_out:
	return;
}

static void ps3_scsi_device_loop_dump(struct ps3_instance *instance)
{
	struct scsi_device *sdev;
	ULong flags;
	struct Scsi_Host *shost = instance->host;

	if (instance->host == NULL) {
		goto l_out;
	}

	spin_lock_irqsave(shost->host_lock, flags);
	list_for_each_entry(sdev, &shost->__devices, siblings) {
		if (sdev == NULL) {
			continue;
		}
		ps3stor_cli_printf("channel[%u], id[%u], lun[%llu], state[%d]!\n",
			sdev->channel, sdev->id, sdev->lun, sdev->sdev_state);
	}
	spin_unlock_irqrestore(shost->host_lock, flags);

l_out:
	return;
}

static void ps3_scsi_device_lookup_cb(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	U16 host_no = 0;
	S32 ret = 0;

	if (argc < 3) {
		ps3stor_cli_printf("Too few args, must input 3 args!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no!\n");
		goto l_out;
	}
	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %u !\n", host_no);
		goto l_out;
	}

	ps3_scsi_device_loop_dump(instance);

l_out:
	return;
}

static void ps3_hard_reset_cb(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	U16 host_no = 0;
	S32 ret = 0;

	if (argc < 3) {
		ps3stor_cli_printf("Too few args, must input 3 args!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no!\n");
		goto l_out;
	}
	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %u !\n", host_no);
		goto l_out;
	}
	ps3stor_cli_printf("hno:%u  entry hard reset !\n", host_no);

	LOG_INFO("hno:%u  entry hard reset!\n",
		PS3_HOST(instance));

	LOG_WARN("hno:%u  cli call recovery request!\n",
		PS3_HOST(instance));
	if ((!PS3_IOC_HARD_RECOVERY_SUPPORT(instance))
		|| (!ps3_hard_reset_enable_query())) {
		LOG_WARN("hno:%u  soc feature unsupport Hard reset! need to be offline!\n",
			PS3_HOST(instance));
		goto l_out;
	}
	if (ps3_no_need_recovery(instance)) {
		goto l_out;
	}

	if (ps3_need_block_hard_reset_request(instance)) {
		LOG_WARN("hno:%u  can not start hard reset\n",
			PS3_HOST(instance));
		goto l_out;
	}

	ps3_hard_recovery_request_with_retry(instance);
	LOG_WARN("hno:%u  cli call recovery request!\n",
		PS3_HOST(instance));

l_out:
	return;
}

#ifdef CONFIG_PM
extern int ps3_cli_suspend(struct pci_dev *pdev, pm_message_t state);
extern int ps3_cli_resume(struct pci_dev *pdev);

static void ps3_suspend_cb(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	U16 host_no = 0;
	pm_message_t state;

	if (argc < 3) {
		ps3stor_cli_printf("Too few args, must input 3 args!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	if (kstrtou16(argv[2], 0, &host_no) != 0) {
		ps3stor_cli_printf("Can not parse host_no!\n");
		goto l_out;
	}
	state.event = 0;
	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %u !\n", host_no);
		goto l_out;
	}
	if (instance->peer_instance != NULL) {
		if (host_no > instance->peer_instance->host->host_no) {
			ps3_cli_suspend(instance->pdev, state);
			ps3_cli_suspend(instance->peer_instance->pdev, state);
		} else {
			ps3_cli_suspend(instance->peer_instance->pdev, state);
			ps3_cli_suspend(instance->pdev, state);
		}
		goto l_out;
	}
	ps3_cli_suspend(instance->pdev, state);

l_out:
	return;
}

static void ps3_resume_cb(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	U16 host_no = 0;

	if (argc < 3) {
		ps3stor_cli_printf("Too few args, must input 3 args!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	if (kstrtou16(argv[2], 0, &host_no) != 0) {
		ps3stor_cli_printf("Can not parse host_no!\n");
		goto l_out;
	}
	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %u !\n", host_no);
		goto l_out;
	}
	if (instance->peer_instance != NULL) {
		if (host_no < instance->peer_instance->host->host_no) {
			ps3_cli_resume(instance->pdev);
			ps3_cli_resume(instance->peer_instance->pdev);
		} else {
			ps3_cli_resume(instance->peer_instance->pdev);
			ps3_cli_resume(instance->pdev);
		}
		goto l_out;
	}
	ps3_cli_resume(instance->pdev);
l_out:
	return;
}

#endif

static void ps3_soc_halt_cb(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	U16 host_no = 0;
	S32 ret = 0;

	if (argc < 3) {
		ps3stor_cli_printf("Too few args, must input 3 args!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no!\n");
		goto l_out;
	}
	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %u !\n", host_no);
		goto l_out;
	}

	if (PS3_IOC_STATE_HALT_SUPPORT(instance)) {
		LOG_WARN("hno:%u  cli call trans to halt!\n",
			PS3_HOST(instance));
		ps3_instance_state_transfer_to_dead(instance);
		instance->ioc_adpter->ioc_force_to_halt(instance);
		LOG_WARN("hno:%u  cli call trans to halt end!\n",
			PS3_HOST(instance));
	} else {
		ps3stor_cli_printf("host does not support HALT!\n", host_no);
	}

l_out:
	return;
}

static void ps3_cmd_stat_switch_store(U16 host_no,
	U16 switch_flag, U16 mask)
{
	struct ps3_instance *instance = ps3_instance_lookup(host_no);
	struct ps3_cmd_stat_wrokq_context *ctx = NULL;
	Bool is_inc_switch_open = PS3_FALSE;

	if (instance == NULL) {
		ps3stor_cli_printf("invalid host_no %u\n", host_no);
		goto l_out;
	}

	if ((instance->cmd_statistics.cmd_stat_switch & mask) ==
		(switch_flag & mask)) {
		ps3stor_cli_printf("switch_flag no change 0x%x\n",
			instance->cmd_statistics.cmd_stat_switch);
		goto l_out;
	}

	is_inc_switch_open = ps3_stat_inc_switch_is_open(instance);

	instance->cmd_statistics.cmd_stat_switch &= (~mask) ;
	instance->cmd_statistics.cmd_stat_switch |= (switch_flag & mask) ;

	ctx = &instance->cmd_statistics.stat_workq;
	if ((ctx->stat_queue != NULL) && (!ctx->is_stop) &&
		ps3_stat_inc_switch_is_open(instance) && !is_inc_switch_open) {
		ps3stor_cli_printf("schedule delay work\n");
		queue_delayed_work(ctx->stat_queue, &ctx->stat_work,
			msecs_to_jiffies(instance->cmd_statistics.stat_interval));
	}

	ps3stor_cli_printf("stat switch value is 0x%x\n",
		instance->cmd_statistics.cmd_stat_switch);
l_out:
	return;
}

static void ps3_cmd_stat_switch_store_cb(int argc, char *argv[])
{
	U16 host_no = 0;
	U16 switch_flag = 0;
	U16 mask = 0;
	S32 ret = PS3_SUCCESS;

	if (argc < 7) {
		ps3stor_cli_printf("Too few args, must input 7 args!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no!\n");
		goto l_out;
	}

	if (strcmp(argv[3], "value") != 0) {
		ps3stor_cli_printf("value is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[4], 0, &switch_flag);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse value!\n");
		goto l_out;
	}

	if (strcmp(argv[5], "mask") != 0) {
		ps3stor_cli_printf("mask is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[6], 0, &mask);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse mask!\n");
		goto l_out;
	}

	ps3_cmd_stat_switch_store(host_no, switch_flag, mask);

l_out:
	return;
}

static inline void ps3_cmd_stat_switch_show(U16 host_no)
{
	struct ps3_instance *instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("invalid host_no %u\n", host_no);
		goto l_out;
	}

	ps3stor_cli_printf("[bit0:OUTSTAND_SWITCH_OPEN, bit1:INC_SWITCH_OPEN,"
		"bit2:LOG_SWITCH_OPEN, bit3:DEV_SWITCH_OPEN]\n");
	ps3stor_cli_printf("stat switch value is 0x%x\n",
		instance->cmd_statistics.cmd_stat_switch);

l_out:
	return;
}

static void ps3_cmd_stat_switch_show_cb(int argc, char *argv[])
{
	U16 host_no = 0;
	S32 ret = PS3_SUCCESS;

	if (argc < 3) {
		ps3stor_cli_printf("Too few args, must input 3 args!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no!\n");
		goto l_out;
	}

	ps3_cmd_stat_switch_show(host_no);
l_out:
	return;
}

static void ps3_stat_total_dump(struct ps3_instance *instance,
	char *buf, S32 total_len)
{
	S32 len = 0;
	const U32 temp_array_len = total_len;
	S8 *tmp_buf = NULL;
	S32 temp_len = 0;
	struct ps3_cmd_statistics_context *ctx = &instance->cmd_statistics;
	U16 i = 0;

	len += snprintf(buf + len, total_len - len,
		"----host[%u] total cmd statistics show begin----\n", PS3_HOST(instance));
	len += snprintf(buf + len, total_len - len,
		"----cmd_outstandin:%u----\n",
		ps3_atomic_read(&instance->cmd_statistics.cmd_outstanding));
	len += snprintf(buf + len, total_len - len,
		"%-25s   %-15s   %-15s   %-15s   %-15s   %-20s    %-20s    %-20s\n",
		" ", "start", "back_good", "back_err", "not_back", "avg(us)", "max(us)", "min(us)");

	tmp_buf = (S8 *)kzalloc(temp_array_len, GFP_KERNEL);
	if (tmp_buf == NULL) {
		goto l_out;
	}

	for (; i < PS3_QOS_PD_PRO; ++i) {
		temp_len = snprintf(tmp_buf, temp_array_len,
			"%-25s   %-15llu   %-15llu   %-15llu   %-15llu   "
			"%-20llu    %-20llu    %-20llu\n",
			ps3_cmd_stat_item_tostring((enum ps3_cmd_stat_item)i),
			ctx->total_stat.stat[i].start,
			ctx->total_stat.stat[i].back_good,
			ctx->total_stat.stat[i].back_err,
			ctx->total_stat.stat[i].not_back,
			ctx->total_stat.stat[i].lagency.avg,
			ctx->total_stat.stat[i].lagency.max_lagency,
			ctx->total_stat.stat[i].lagency.min_lagency);
		if ((len + temp_len) > total_len) {
			ps3stor_cli_printf("print over!\n");
			goto l_out;
		}

		memset(tmp_buf, 0, temp_array_len);

		len += snprintf(buf + len, total_len - len,
			"%-25s   %-15llu   %-15llu   %-15llu   %-15llu   "
			"%-20llu    %-20llu    %-20llu\n",
			ps3_cmd_stat_item_tostring((enum ps3_cmd_stat_item)i),
			ctx->total_stat.stat[i].start,
			ctx->total_stat.stat[i].back_good,
			ctx->total_stat.stat[i].back_err,
			ctx->total_stat.stat[i].not_back,
			ctx->total_stat.stat[i].lagency.avg,
			ctx->total_stat.stat[i].lagency.max_lagency,
			ctx->total_stat.stat[i].lagency.min_lagency);
	}

	temp_len = snprintf(tmp_buf, temp_array_len,
		"----host[%u] cmd statistics show end----\n", PS3_HOST(instance));
	if ((len + temp_len) > total_len) {
		ps3stor_cli_printf("print over!\n");
		goto l_out;
	}

	len += snprintf(buf + len, total_len - len,
		"----host[%u] cmd statistics show end----\n", PS3_HOST(instance));
l_out:
	if (tmp_buf != NULL) {
		kfree((void*)tmp_buf);
		tmp_buf = NULL;
	}
	return;
}

static void ps3_stat_total_show_cb(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	S8 *buf = NULL;
	U16 host_no = 0;
	S32 ret = PS3_SUCCESS;

	if (argc < 3) {
		ps3stor_cli_printf("Too few args, must input 3 args!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no!\n");
		goto l_out;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d !\n", host_no);
		goto l_out;
	}

	buf = (S8 *)kzalloc(PAGE_SIZE * 2, GFP_KERNEL);
	if (buf == NULL) {
		ps3stor_cli_printf("malloc buf failed!\n");
		goto l_out;
	}

	ps3_stat_total_dump(instance, buf, PAGE_SIZE * 2);
	ps3stor_cli_printf(buf);

l_out:
	if (buf != NULL) {
		kfree(buf);
		buf = NULL;
	}
	return;
}

static void ps3_stat_inc_dump(struct ps3_instance *instance,
	char *buf, S32 total_len)
{
	S32 len = 0;
	const U32 temp_array_len = PAGE_SIZE;
	S8 *tmp_buf = NULL;
	S32 temp_len = 0;
	struct ps3_cmd_statistics_context *ctx = &instance->cmd_statistics;
	U16 i = 0;

	len += snprintf(buf + len, total_len - len,
		"----host[%u] inc cmd statistics show begin----\n", PS3_HOST(instance));
	len += snprintf(buf + len, total_len - len,
		"%-25s   %-15s   %-15s   %-15s   %-15s   %-20s    %-20s    %-20s\n",
		" ", "start", "back_good", "back_err", "not_back", "avg(us)", "max(us)", "min(us)");

	tmp_buf = (S8 *)kzalloc(temp_array_len, GFP_KERNEL);
	if (tmp_buf == NULL) {
		goto l_out;
	}

	for (; i < PS3_QOS_PD_PRO; ++i) {
		temp_len = snprintf(tmp_buf, temp_array_len,
			"%-25s   %-15llu   %-15llu   %-15llu   %-15llu   "
			"%-20llu    %-20llu    %-20llu\n",
			ps3_cmd_stat_item_tostring((enum ps3_cmd_stat_item)i),
			ctx->inc_stat.stat[i].start,
			ctx->inc_stat.stat[i].back_good,
			ctx->inc_stat.stat[i].back_err,
			ctx->inc_stat.stat[i].not_back,
			ctx->inc_stat.stat[i].lagency.avg,
			ctx->inc_stat.stat[i].lagency.max_lagency,
			ctx->inc_stat.stat[i].lagency.min_lagency);
		if ((len + temp_len) > total_len) {
			ps3stor_cli_printf("print over!\n");
			goto l_out;
		}

		memset(tmp_buf, 0, temp_array_len);

		len += snprintf(buf + len, total_len - len,
			"%-25s   %-15llu   %-15llu   %-15llu   %-15llu   "
			"%-20llu    %-20llu    %-20llu\n",
			ps3_cmd_stat_item_tostring((enum ps3_cmd_stat_item)i),
			ctx->inc_stat.stat[i].start,
			ctx->inc_stat.stat[i].back_good,
			ctx->inc_stat.stat[i].back_err,
			ctx->inc_stat.stat[i].not_back,
			ctx->inc_stat.stat[i].lagency.avg,
			ctx->inc_stat.stat[i].lagency.max_lagency,
			ctx->inc_stat.stat[i].lagency.min_lagency);
	}

	temp_len = snprintf(tmp_buf, temp_array_len,
		"----host[%u] cmd statistics show end----\n", PS3_HOST(instance));
	if ((len + temp_len) > total_len) {
		ps3stor_cli_printf("print over!\n");
		goto l_out;
	}

	len += snprintf(buf + len, total_len - len,
		"----host[%u] cmd statistics show end----\n", PS3_HOST(instance));
l_out:
	if (tmp_buf != NULL) {
		kfree((void*)tmp_buf);
		tmp_buf = NULL;
	}
	return;
}

static void ps3_stat_inc_show_cb(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	S8 *buf = NULL;
	U16 host_no = 0;
	S32 ret = PS3_SUCCESS;

	if (argc < 3) {
		ps3stor_cli_printf("Too few args, must input 3 args!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no!\n");
		goto l_out;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d !\n", host_no);
		goto l_out;
	}

	buf = (S8 *)kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (buf == NULL) {
		ps3stor_cli_printf("malloc buf failed!\n");
		goto l_out;
	}

	ps3_stat_inc_dump(instance, buf, PAGE_SIZE);
	ps3stor_cli_printf(buf);

l_out:
	if (buf != NULL) {
		kfree(buf);
		buf = NULL;
	}
	return;
}

static void ps3_stat_buf_clr_cb(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	U16 host_no = 0;
	S32 ret = PS3_SUCCESS;

	if (argc < 3) {
		ps3stor_cli_printf("Too few args, must input 3 args!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no!\n");
		goto l_out;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d !\n", host_no);
		goto l_out;
	}

	ps3_stat_all_clear(instance);
	ps3stor_cli_printf("cmd stat clear complete\n");

l_out:
	return;
}

static void ps3_stat_interval_show_cb(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	U16 host_no = 0;
	S32 ret = PS3_SUCCESS;

	if (argc < 3) {
		ps3stor_cli_printf("Too few args, must input 3 args!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no!\n");
		goto l_out;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d !\n", host_no);
		goto l_out;
	}

	ps3stor_cli_printf("cmd stat interval is %u ms\n",
		instance->cmd_statistics.stat_interval);

l_out:
	return;
}

static void ps3_stat_interval_store_cb(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	U16 host_no = 0;
	U32 interval = 0;
	S32 ret = PS3_SUCCESS;

	if (argc < 5) {
		ps3stor_cli_printf("Too few args, must input 3 args!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no!\n");
		goto l_out;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d !\n", host_no);
		goto l_out;
	}

	if (strcmp(argv[3], "interval") != 0) {
		ps3stor_cli_printf("value err!\n");
		goto l_out;
	}

	ret = kstrtouint(argv[4], 0, &interval);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse value!\n");
		goto l_out;
	}

	if (interval < 1000) {
		ps3stor_cli_printf("interval need > 1000ms!\n");
		goto l_out;
	}

	instance->cmd_statistics.stat_interval = interval;

	ps3stor_cli_printf("cmd stat interval is %u ms\n",
		instance->cmd_statistics.stat_interval);

l_out:
	return;
}

static void ps3_reply_fifo_reset_cb(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	U16 host_no = 0;
	S32 ret = PS3_SUCCESS;

	if (argc < 3) {
		ps3stor_cli_printf("Too few args, must input 3 args!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no!\n");
		goto l_out;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d !\n", host_no);
		goto l_out;
	}

	ps3_all_reply_fifo_complete(instance);
	ps3_all_reply_fifo_init(instance);
	ps3stor_cli_printf("all reply fifo complete reply_index to 0\n");

l_out:
	return;
}

static void ps3_cli_remove_host_force(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	U16 host_no = 0;
	S32 ret = PS3_SUCCESS;

	if (argc < 3) {
		ps3stor_cli_printf("Too few args, must input 3 args!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no!\n");
		goto l_out;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d !\n", host_no);
		goto l_out;
	}

	ps3_remove(instance->pdev);

	ps3stor_cli_printf("remove instance %d completed\n", host_no);
l_out:
	return;
}

static void ps3_cli_ramfs_test_set(int argc, char *argv[])
{
	U16 ramfs_enable = 0;
	S32 ret = PS3_SUCCESS;

	if (argc < 2) {
		ps3stor_cli_printf("Too few args, must input 2 args!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[1], 0, &ramfs_enable);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse ramfs_enable!\n");
		goto l_out;
	}

	ps3_ramfs_test_store((S32)ramfs_enable);

	ps3stor_cli_printf("set ramfs test enable to %d\n", ramfs_enable);
l_out:
	return;
}

static void ps3_cli_r1x_delay(int argc, char *argv[])
{
	U32 r1x_interval_delay = 0;
	S32 ret = PS3_SUCCESS;

	if (argc < 2) {
		ps3stor_cli_printf("Too few args, must input 2 args!\n");
		goto l_out;
	}

	ret = kstrtouint(argv[1], 0, &r1x_interval_delay);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse r1x_interval_delay!\n");
		goto l_out;
	}

	ps3_r1x_interval_delay_set(r1x_interval_delay);

	ps3stor_cli_printf("set r1x_interval_delay to %ums\n", r1x_interval_delay);
l_out:
	return;
}

static void ps3_cli_err_inject_active(int argc, char *argv[])
{
	U32 err_inject_type = 0;
	U32 count;
	S32 ret = PS3_SUCCESS;

	if (argc < 3) {
		ps3stor_cli_printf("Too few args, must input 3 args!\n");
		goto l_out;
	}

	ret = kstrtouint(argv[1], 0, &err_inject_type);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse err_inject_type!\n");
		goto l_out;
	}

	ret = kstrtouint(argv[2], 0, &count);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse count!\n");
		goto l_out;
	}

	if ((err_inject_type >= PS3_ERR_IJ_WATCHDOG_CONCURY) &&
		(err_inject_type < PS3_ERR_IJ_MAX_COUNT)) {
		INJECT_ACTIVE(err_inject_type, count)
		ps3stor_cli_printf("active %u count %u\n", err_inject_type, count);
	}
l_out:
	return;
}
static void ps3_cli_err_inject_clear(int argc, char *argv[])
{
	INJECT_EXIT()
}

Bool ps3_get_wait_cli_flag(void)
{
	return ps3_cli_wait_flag;
}

static void ps3_no_wait_cli_cmd(int argc, char *argv[])
{
	U16 wait_cli_flag = PS3_FALSE;
	S32 ret = PS3_SUCCESS;

	if (argc < 3) {
		ps3stor_cli_printf("Too few args, must input 3 args!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "flag") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &wait_cli_flag);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse wait_cli_flag!\n",
			argv[3]);
		goto l_out;
	}

	if (wait_cli_flag) {
		ps3_cli_wait_flag = PS3_TRUE;
	} else {
		ps3_cli_wait_flag = PS3_FALSE;
	}
	ps3stor_cli_printf("ps3 no wait cli flag %d\n", ps3_cli_wait_flag);

l_out:
	return;
}

static void ps3_qos_show_pd(struct ps3_instance *instance, U16 disk_id)
{
	struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;
	struct qos_wait_queue *waitq = NULL;
	U16 i = 0;
	if (disk_id > 0 &&
			disk_id <= instance->qos_context.max_pd_count) {
		ps3stor_cli_printf("qos pd[%u] info\n", disk_id);
		qos_pd_mgr = &instance->qos_context.pd_ctx.qos_pd_mgrs[disk_id];
		ps3stor_cli_printf("-----valid[%u] vid[%u] used_quota[%u] quota[%u] total_waited_cmd[%u]\n",
			ps3_atomic_read(&qos_pd_mgr->valid), qos_pd_mgr->vd_id,
			ps3_atomic_read(&qos_pd_mgr->pd_used_quota),
			qos_pd_mgr->pd_quota, qos_pd_mgr->total_wait_cmd_cnt);
		if (qos_pd_mgr->total_wait_cmd_cnt > 0) {
			for (i = 1; i < qos_pd_mgr->waitq_cnt; i++) {
				waitq = &qos_pd_mgr->waitqs[i];
				if (waitq->count > 0) {
					ps3stor_cli_printf("     vid[%u] waitq_cnt[%u]\n",
						waitq->id, waitq->count);
				}
			}
		}

		if (qos_pd_mgr->dev_type == PS3_DEV_TYPE_NVME_SSD) {
			waitq = &qos_pd_mgr->waitqs[0];
			ps3stor_cli_printf("     direct_used_quota[%u] qid[%u] waitq_cnt[%u]\n",
					ps3_atomic_read(&qos_pd_mgr->direct_used_quota), waitq->count);
		}

	} else {
		ps3stor_cli_printf("disk_id[%u] is error\n", disk_id);
	}
}

static void ps3_qos_show_vd(struct ps3_instance *instance, U16 disk_id)
{
	U8 i = 0;
	struct ps3_qos_vd_mgr *qos_vd_mgr = NULL;
	struct ps3_qos_cq_context *qos_cq_ctx = &instance->qos_context.cq_ctx;
	struct ps3_qos_softq_mgr *qos_softq_mgr = NULL;
	if (disk_id > 0 && disk_id <= instance->qos_context.max_vd_count) {
		ps3stor_cli_printf("qos vd[%u] info\n", disk_id);
		if (instance->qos_context.vd_ctx.qos_vd_mgrs) {
			qos_vd_mgr = &instance->qos_context.vd_ctx.qos_vd_mgrs[disk_id];
			ps3stor_cli_printf("-----valid[%u] quota[%u] quota_waitq[%u] exclusive[%u]\n",
				qos_vd_mgr->valid, ps3_atomic_read(&qos_vd_mgr->vd_quota),
				qos_vd_mgr->vd_quota_wait_q.count, ps3_atomic_read(&qos_vd_mgr->exclusive_cmd_cnt));
		}

		if (qos_cq_ctx->cmdqs) {
			for (i = 0; i < qos_cq_ctx->cmdq_cnt; i++) {
				qos_softq_mgr = &qos_cq_ctx->cmdqs[i];
				ps3stor_cli_printf("-----cmdq[%u] waitq_cnt[%u]\n",
					i, qos_softq_mgr->waitqs[disk_id].count);
			}
		}
	} else {
		ps3stor_cli_printf("disk_id[%u] is error\n", disk_id);
	}
}

static void ps3_qos_stat_dump(struct ps3_instance *instance,
	char *buf, S32 total_len)
{
	S32 len = 0;
    struct ps3_qos_tg_context *qos_tg_ctx = &instance->qos_context.tg_ctx;
    struct ps3_qos_pd_context *qos_pd_ctx = &instance->qos_context.pd_ctx;
    struct ps3_qos_vd_context *qos_vd_ctx = &instance->qos_context.vd_ctx;
	struct ps3_qos_cq_context *qos_cq_ctx = &instance->qos_context.cq_ctx;
	struct ps3_qos_softq_mgr *qos_softq_mgr = NULL;
    struct ps3_qos_pd_mgr *qos_pd_mgr = NULL;
    struct ps3_qos_vd_mgr *qos_vd_mgr = NULL;
    struct qos_wait_queue *cmd_waitq = NULL;
	U16 i = 0;

	len += snprintf(buf + len, total_len - len,
		"----host[%u] qos cmd statistics show begin switch[%u] "
		"total_qos_cmd[%llu]\n",
         PS3_HOST(instance), instance->qos_context.qos_switch,
		ps3_atomic64_read(&instance->cmd_statistics.cmd_qos_total));

	if (qos_tg_ctx->vd_cmd_waitqs) {
		len += snprintf(buf + len, total_len - len,
			"tag: share[%u] mgr[%u]\n", ps3_atomic_read(&qos_tg_ctx->share_free_cnt),
			 ps3_atomic_read(&qos_tg_ctx->mgr_free_cnt));

	    if (qos_tg_ctx ->mgr_cmd_wait_q.count > 0) {
	        len += snprintf(buf + len, total_len - len,
			    "mgr cmd waitq count[%u]\n",
	                 qos_tg_ctx->mgr_cmd_wait_q.count);
	        if (len >= total_len) {
	            goto l_out;
	        }
	    }

		for (i = 1; i <= instance->qos_context.max_vd_count; i++) {
	        cmd_waitq = &qos_tg_ctx ->vd_cmd_waitqs[i];
	        if (cmd_waitq->count) {
	            len += snprintf(buf + len, total_len - len,
			    	"vd[%u] cmd waitq_count[%u]\n", i, cmd_waitq->count);
	            if (len >= total_len) {
	                goto l_out;
	            }
	        }
	    }
	}

	if (qos_cq_ctx->cmdqs) {
		qos_softq_mgr = &qos_cq_ctx->mgrq;
		len += snprintf(buf + len, total_len - len,
	    	"mgrq: free[%u] waitq_count[%u]\n", ps3_atomic_read(&qos_softq_mgr->free_cnt),
			 qos_softq_mgr->total_wait_cmd_cnt);
        if (len >= total_len) {
            goto l_out;
        }

		for (i = 0; i < qos_cq_ctx->cmdq_cnt; i++) {
			qos_softq_mgr = &qos_cq_ctx->cmdqs[i];
			len += snprintf(buf + len, total_len - len,
			    "cmdq[%u]: free[%u] waitq_count[%u]\n", i, ps3_atomic_read(&qos_softq_mgr->free_cnt),
				qos_softq_mgr->total_wait_cmd_cnt);
	        if (len >= total_len) {
	            goto l_out;
	        }
		}
	}

	if (qos_pd_ctx->qos_pd_mgrs) {
	    for (i = 1; i <= instance->qos_context.max_pd_count; i++) {
	        qos_pd_mgr = &qos_pd_ctx->qos_pd_mgrs[i];
	        if (qos_pd_mgr->total_wait_cmd_cnt) {
	            len += snprintf(buf + len, total_len - len,
			    	"pid[%u] vid[%u] valid[%u] used_quota[%u] waitq_count[%u]\n",
					qos_pd_mgr->disk_id, qos_pd_mgr->vd_id, ps3_atomic_read(&qos_pd_mgr->valid),
					ps3_atomic_read(&qos_pd_mgr->pd_used_quota),qos_pd_mgr->total_wait_cmd_cnt);
	            if (len >= total_len) {
	                goto l_out;
	            }
	        }

			if (qos_pd_mgr->total_waited_direct_cmd) {
				len += snprintf(buf + len, total_len - len,
			    	"pid[%u] valid[%u] direct_used_quota[%u] waitq_count[%u]\n",
					qos_pd_mgr->disk_id, qos_pd_mgr->vd_id, ps3_atomic_read(&qos_pd_mgr->direct_used_quota),
					qos_pd_mgr->total_waited_direct_cmd);
	            if (len >= total_len) {
	                goto l_out;
	            }
			}
	    }
	}

	if (qos_vd_ctx->qos_vd_mgrs) {
	    for (i = 1; i <= instance->qos_context.max_vd_count; i++) {
	        qos_vd_mgr = &qos_vd_ctx->qos_vd_mgrs[i];
	        if (qos_vd_mgr->vd_quota_wait_q.count) {
	            len += snprintf(buf + len, total_len - len,
			    "vid[%u] quota[%u] waitq_count[%u]\n",
	                 i, ps3_atomic_read(&qos_vd_mgr->vd_quota),qos_vd_mgr->vd_quota_wait_q.count);
	            if (len >= total_len) {
	                goto l_out;
	            }
	        }
	    }
	}

    len += snprintf(buf + len, total_len - len,
		"----host[%u] qos cmd statistics show end----\n", PS3_HOST(instance));
l_out:
	return;
}

static void ps3_qos_show_delay(struct ps3_instance *instance)
{
	struct ps3_cmd_statistics_context *ctx = &instance->cmd_statistics;
	U16 i = 0;

	ps3stor_cli_printf("----host[%u] qos delay show begin----\n", PS3_HOST(instance));
    ps3stor_cli_printf("%-25s  %-20s   %-20s   %-20s\n",
		" ", "avg(us)", "max(us)", "min(us)");
	for (i = PS3_QOS_PD_PRO; i < PS3_CMD_STAT_COUNT; i++) {
		ps3stor_cli_printf("%-25s  %-20llu    %-20llu    %-20llu\n",
			ps3_cmd_stat_item_tostring((enum ps3_cmd_stat_item)i),
			ctx->total_stat.stat[i].lagency.avg,
			ctx->total_stat.stat[i].lagency.max_lagency,
			ctx->total_stat.stat[i].lagency.min_lagency);
	}

	ps3stor_cli_printf("----host[%u] qos delay show end----\n", PS3_HOST(instance));
	return;
}

static void ps3_cli_qos_info(int argc, char *argv[])
{
    struct ps3_instance *instance = NULL;
	U16 host_no = 0;
	U16 disk_id = 0;
    S8 *buf = NULL;
	S32 ret = 0;

	if (argc < 4) {
		ps3stor_cli_printf("Too few args, must input 4 args!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no!\n");
		goto l_out;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %u !\n", host_no);
		goto l_out;
	}

    if (!instance->qos_context.inited) {
        ps3stor_cli_printf("qos not support host_no %u !\n", host_no);
		goto l_out;
    }

	if (strcmp(argv[3], "pd") == 0) {
		if (argc < 5) {
			ps3stor_cli_printf("pd_id is needed!\n");
			goto l_out;
		}

		ret = kstrtou16(argv[4], 0, &disk_id);
		if (ret != 0) {
			ps3stor_cli_printf("Can not parse disk id!\n");
			goto l_out;
		}
		ps3_qos_show_pd(instance, disk_id);
		goto l_out;
	} else if (strcmp(argv[3], "vd") == 0) {
		if (argc < 5) {
			ps3stor_cli_printf("vd_id is needed!\n");
			goto l_out;
		}

		ret = kstrtou16(argv[4], 0, &disk_id);
		if (ret != 0) {
			ps3stor_cli_printf("Can not parse disk id!\n");
			goto l_out;
		}
		ps3_qos_show_vd(instance, disk_id);
		goto l_out;
	} else if (strcmp(argv[3], "all") == 0) {
		buf = (S8 *)kzalloc(PAGE_SIZE * 2, GFP_KERNEL);
		if (buf == NULL) {
			ps3stor_cli_printf("malloc buf failed!\n");
			goto l_out;
		}

		ps3_qos_stat_dump(instance, buf, PAGE_SIZE * 2);
		ps3stor_cli_printf(buf);
	} else if (strcmp(argv[3], "delay") == 0) {
		ps3_qos_show_delay(instance);
	}
l_out:
    if (buf != NULL) {
		kfree(buf);
		buf = NULL;
	}

	return;
}

static void	ps3_cli_special_log(int argc, char *argv[])
{
	struct ps3_instance *instance = NULL;
	U16 host_no = 0;
	U16 print_switch = 0;
	S32 ret = 0;

	if (argc < 5) {
		ps3stor_cli_printf("Too few args, must input 5 args!\n");
		goto l_out;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_out;
	}

	ret = kstrtou16(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no!\n");
		goto l_out;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %u !\n", host_no);
		goto l_out;
	}

	if (strcmp(argv[3], "set") == 0) {
		ret = kstrtou16(argv[4], 0, &print_switch);
		if (ret != 0) {
			ps3stor_cli_printf("Can not parse log_switch!\n");
			goto l_out;
		}

		instance->is_print_special_log = (print_switch > 0 ? 1 : 0);
		ps3stor_cli_printf("set print_special_log=%u\n",
				instance->is_print_special_log);
	}
l_out:
	return;
}

#ifdef PS3_SUPPORT_INJECT
struct inject_cmds_t * ps3_inject_scsi_rw_exist(struct inject_cmds_t *this_pitem)
{
	struct inject_cmds_t *pitem = NULL;
	PS3Inject_s *p_inject_list = get_inject();
	Bool is_find = PS3_FALSE;

	ps3_mutex_lock(&p_inject_list->lock);
	if (list_empty(&p_inject_list->scsi_rw_list)) {
		goto l_out;
	}

	list_for_each_entry(pitem, &p_inject_list->scsi_rw_list, list) {
		if ((this_pitem->item.scsi_cmd.host_no ==  pitem->item.scsi_cmd.host_no) &&
			(this_pitem->item.scsi_cmd.channel == pitem->item.scsi_cmd.channel) &&
			(this_pitem->item.scsi_cmd.id == pitem->item.scsi_cmd.id)) {
			if (((this_pitem->item.scsi_cmd.lba >= pitem->item.scsi_cmd.lba) &&
				(this_pitem->item.scsi_cmd.lba <= pitem->item.scsi_cmd.lba + pitem->item.scsi_cmd.len)) ||
				((this_pitem->item.scsi_cmd.lba + this_pitem->item.scsi_cmd.len >= pitem->item.scsi_cmd.lba) &&
				(this_pitem->item.scsi_cmd.lba + this_pitem->item.scsi_cmd.len <=
				pitem->item.scsi_cmd.lba + pitem->item.scsi_cmd.len))) {
				ps3stor_cli_printf("inject [%u:%u:%u] lba[0x%llx], len[0x%x], is exist!\n",
					pitem->item.scsi_cmd.host_no, pitem->item.scsi_cmd.channel,
					pitem->item.scsi_cmd.id, pitem->item.scsi_cmd.lba,
					pitem->item.scsi_cmd.len);
				is_find = PS3_TRUE;
				break;
			}
		}
	}
l_out:
	if(!is_find) {
		pitem = NULL;
	}
	ps3_mutex_unlock(&p_inject_list->lock);
	return pitem;
}

struct inject_cmds_t * ps3_inject_scsi_task_exist(struct inject_cmds_t *this_pitem)
{
	struct inject_cmds_t *pitem = NULL;
	PS3Inject_s *p_inject_list = get_inject();
	Bool is_find = PS3_FALSE;

	ps3_mutex_lock(&p_inject_list->lock);
	if (list_empty(&p_inject_list->scsi_task_list)) {
		goto l_out;
	}

	list_for_each_entry(pitem, &p_inject_list->scsi_task_list, list) {
		if ((this_pitem->item.scsi_task_cmd.host_no ==  pitem->item.scsi_task_cmd.host_no) &&
			(this_pitem->item.scsi_task_cmd.channel == pitem->item.scsi_task_cmd.channel) &&
			(this_pitem->item.scsi_task_cmd.id == pitem->item.scsi_task_cmd.id)) {
			if (this_pitem->item.scsi_task_cmd.cmd_type == pitem->item.scsi_task_cmd.cmd_type &&
				this_pitem->item.scsi_task_cmd.cmd_sub_type == pitem->item.scsi_task_cmd.cmd_sub_type) {
				is_find = PS3_TRUE;
				break;
			}
		}
	}
l_out:
	if(!is_find) {
		pitem = NULL;
	}
	ps3_mutex_unlock(&p_inject_list->lock);
	return pitem;
}
struct inject_cmds_t * ps3_inject_mgr_exist(struct inject_cmds_t *this_pitem)
{
	struct inject_cmds_t *pitem = NULL;
	PS3Inject_s *p_inject_list = get_inject();
	Bool is_find = PS3_FALSE;

	ps3_mutex_lock(&p_inject_list->lock);
	if (list_empty(&p_inject_list->mgr_list)) {
		goto l_out;
	}

	list_for_each_entry(pitem, &p_inject_list->mgr_list, list) {
		if ((this_pitem->item.mgr_cmd.host_no ==  pitem->item.mgr_cmd.host_no) &&
			(this_pitem->item.mgr_cmd.cmd_type == pitem->item.mgr_cmd.cmd_type) &&
			(this_pitem->item.mgr_cmd.cmd_sub_type == pitem->item.mgr_cmd.cmd_sub_type)) {
				is_find = PS3_TRUE;
		}
	}
l_out:
	if(!is_find) {
		pitem = NULL;
	}
	ps3_mutex_unlock(&p_inject_list->lock);
	return pitem;
}

void ps3_show_inject_scsi_rw_item(struct inject_cmds_t *pitem)
{
	struct inject_cmds_t *pitem1 = NULL;
	PS3Inject_s *p_inject_list = get_inject();

	ps3_mutex_lock(&p_inject_list->lock);
	if (list_empty(&p_inject_list->scsi_rw_list)) {
		ps3stor_cli_printf("rw inject list is empty!\n");
		ps3_mutex_unlock(&p_inject_list->lock);
		goto l_out;
	}
	ps3_mutex_unlock(&p_inject_list->lock);

	if ((pitem1 = ps3_inject_scsi_rw_exist(pitem)) == NULL){
		ps3stor_cli_printf("rw inject type is noexist!\n");
		goto l_out;
	}
	ps3stor_cli_printf("inject_rw host_no %u channel %u id %u lba 0x%llx len 0x%u type %u, time %u!\n",
		pitem1->item.scsi_cmd.host_no, pitem1->item.scsi_cmd.channel,
		pitem1->item.scsi_cmd.id, pitem1->item.scsi_cmd.lba,
		pitem1->item.scsi_cmd.len, pitem1->item.scsi_cmd.dealType,
		pitem1->item.scsi_cmd.inject_count);

l_out:
	return;
}

void ps3_show_inject_all_scsi_rw(int argc, char *argv[])
{
	struct inject_cmds_t *pitem = NULL;
	PS3Inject_s *p_inject_list = get_inject();

	ps3_mutex_lock(&p_inject_list->lock);
	if (list_empty(&p_inject_list->scsi_rw_list)) {
		ps3stor_cli_printf("rw inject list is empty!\n");
		goto l_out;
	}

	list_for_each_entry(pitem, &p_inject_list->scsi_rw_list, list) {
		ps3stor_cli_printf("inject_rw host_no %u channel %u id %u lba 0x%llx len 0x%u!\n",
			pitem->item.scsi_cmd.host_no, pitem->item.scsi_cmd.channel,
			pitem->item.scsi_cmd.id, pitem->item.scsi_cmd.lba,
			pitem->item.scsi_cmd.len);
	}

l_out:
	ps3_mutex_unlock(&p_inject_list->lock);
	return;
}
void ps3_show_inject_scsi_task_item(struct inject_cmds_t *pitem)
{
	struct inject_cmds_t *pitem1;
	PS3Inject_s *p_inject_list = get_inject();

	ps3_mutex_lock(&p_inject_list->lock);
	if (list_empty(&p_inject_list->scsi_task_list)) {
		ps3stor_cli_printf("task inject list is empty!\n");
		ps3_mutex_unlock(&p_inject_list->lock);
		goto l_out;
	}
	ps3_mutex_unlock(&p_inject_list->lock);

	if ((pitem1 = ps3_inject_scsi_task_exist(pitem)) == NULL){
		ps3stor_cli_printf("task inject type is noexist!\n");
		goto l_out;
	}
	ps3stor_cli_printf("inject_rw host_no %u channel %u id %u cmdtype"
		" 0x%u cmdSubtype 0x%u dealType 0x%u times %u!\n",
		pitem1->item.scsi_task_cmd.host_no, pitem1->item.scsi_task_cmd.channel,
		pitem1->item.scsi_task_cmd.id, pitem1->item.scsi_task_cmd.cmd_type,
		pitem1->item.scsi_task_cmd.cmd_sub_type, pitem1->item.scsi_task_cmd.dealType,
		pitem1->item.scsi_task_cmd.inject_count);

l_out:
	return;
}
void ps3_show_inject_mgr_item(struct inject_cmds_t *pitem)
{
	struct inject_cmds_t *pitem1;
	PS3Inject_s *p_inject_list = get_inject();

	ps3_mutex_lock(&p_inject_list->lock);
	if (list_empty(&p_inject_list->mgr_list)) {
		ps3_mutex_unlock(&p_inject_list->lock);
		goto l_out;
	}
	ps3_mutex_unlock(&p_inject_list->lock);

	if ((pitem1 = ps3_inject_mgr_exist(pitem)) == NULL){
		ps3stor_cli_printf("mgr type is noexist!\n");
		goto l_out;
	}
	ps3stor_cli_printf("inject_mgr host_no %u cmdtype 0x%u cmdSubtype 0x%u dealType 0x%u times %u!\n",
		pitem1->item.mgr_cmd.host_no, pitem1->item.mgr_cmd.cmd_type,
		pitem1->item.mgr_cmd.cmd_sub_type, pitem1->item.mgr_cmd.dealType,
		pitem1->item.mgr_cmd.inject_count);

l_out:
	return;
}

void ps3_show_inject_all_scsi_task(int argc, char *argv[])
{
	struct inject_cmds_t *pitem = NULL;
	PS3Inject_s *p_inject_list = get_inject();

	ps3_mutex_lock(&p_inject_list->lock);
	if (list_empty(&p_inject_list->scsi_task_list)) {
		ps3stor_cli_printf("task inject list is empty!\n");
		goto l_out;
	}

	list_for_each_entry(pitem, &p_inject_list->scsi_task_list, list) {
		ps3stor_cli_printf("inject_task host_no %u channel %u id %u"
			" cmdtype 0x%u cmdSubtype 0x%u dealType 0x%u times %u!\n",
		pitem->item.scsi_task_cmd.host_no, pitem->item.scsi_task_cmd.channel,
		pitem->item.scsi_task_cmd.id, pitem->item.scsi_task_cmd.cmd_type,
		pitem->item.scsi_task_cmd.cmd_sub_type,pitem->item.scsi_task_cmd.dealType,
		pitem->item.scsi_task_cmd.inject_count);
	}

l_out:
	ps3_mutex_unlock(&p_inject_list->lock);
	return;
}
void ps3_show_inject_all_mgr(int argc, char *argv[])
{
	struct inject_cmds_t *pitem = NULL;
	PS3Inject_s *p_inject_list = get_inject();

	ps3_mutex_lock(&p_inject_list->lock);
	if (list_empty(&p_inject_list->mgr_list)) {
		goto l_out;
	}

	list_for_each_entry(pitem, &p_inject_list->mgr_list, list) {
		ps3stor_cli_printf("inject_task host_no %u cmdtype 0x%u "
			"cmdSubtype 0x%u dealType 0x%u times %u!\n",
		pitem->item.mgr_cmd.host_no, pitem->item.mgr_cmd.cmd_type,
		pitem->item.mgr_cmd.cmd_sub_type,pitem->item.mgr_cmd.dealType,
		pitem->item.mgr_cmd.inject_count);
	}

l_out:
	ps3_mutex_unlock(&p_inject_list->lock);
	return;
}

void ps3_clear_all_inject_scsi_rw(void)
{
	struct inject_cmds_t *pitem = NULL;
	PS3Inject_s *p_inject_list = get_inject();
	struct inject_cmds_t *pitem_next = NULL;

	ps3_mutex_lock(&p_inject_list->lock);

	if (list_empty(&p_inject_list->scsi_rw_list)) {
		goto l_out;
	}

	list_for_each_entry_safe(pitem, pitem_next, &p_inject_list->scsi_rw_list, list) {
		list_del(&pitem->list);
		kfree(pitem);
	}

l_out:
	ps3_mutex_unlock(&p_inject_list->lock);
	return;
}
void ps3_clear_all_inject_scsi_task(void)
{
	struct inject_cmds_t *pitem = NULL;
	struct inject_cmds_t *pitem_next = NULL;
	PS3Inject_s *p_inject_list = get_inject();

	ps3_mutex_lock(&p_inject_list->lock);

	if (list_empty(&p_inject_list->scsi_task_list)) {
		goto l_out;
	}

	list_for_each_entry_safe(pitem, pitem_next, &p_inject_list->scsi_task_list, list) {
		list_del(&pitem->list);
		kfree(pitem);
	}

l_out:
	ps3_mutex_unlock(&p_inject_list->lock);
	return;
}
void ps3_clear_all_inject_mgr(void)
{
	struct inject_cmds_t *pitem = NULL;
	struct inject_cmds_t *pitem_next = NULL;
	PS3Inject_s *p_inject_list = get_inject();

	ps3_mutex_lock(&p_inject_list->lock);

	if (list_empty(&p_inject_list->mgr_list)) {
		goto l_out;
	}

	list_for_each_entry_safe(pitem, pitem_next, &p_inject_list->mgr_list, list) {
		list_del(&pitem->list);
		kfree(pitem);
	}

l_out:
	ps3_mutex_unlock(&p_inject_list->lock);
	return;
}

void ps3_del_inject_scsi_rw(struct inject_cmds_t *pitem)
{
	struct inject_cmds_t *pitem1;
	PS3Inject_s *p_inject_list = get_inject();

	if ((pitem1 = ps3_inject_scsi_rw_exist(pitem)) == NULL){
		ps3stor_cli_printf("rw inject type is noexist!\n");
		goto l_err;
	}
	ps3_mutex_lock(&p_inject_list->lock);
	ps3stor_cli_printf("del inject_rw host_no %u channel %u id %u lba 0x%llx len 0x%u type %u, time %u!\n",
		pitem1->item.scsi_cmd.host_no, pitem1->item.scsi_cmd.channel,
		pitem1->item.scsi_cmd.id, pitem1->item.scsi_cmd.lba,
		pitem1->item.scsi_cmd.len, pitem1->item.scsi_cmd.dealType,
		pitem1->item.scsi_cmd.inject_count);
	list_del(&pitem1->list);
	ps3_mutex_unlock(&p_inject_list->lock);
	kfree(pitem1);

l_err:
	kfree(pitem);
	return;
}
void ps3_del_inject_scsi_task(struct inject_cmds_t *pitem)
{
	struct inject_cmds_t *pitem1;
	PS3Inject_s *p_inject_list = get_inject();

	if ((pitem1 = ps3_inject_scsi_task_exist(pitem)) == NULL){
		ps3stor_cli_printf("task inject type is noexist!\n");
		goto l_err;
	}
	ps3_mutex_lock(&p_inject_list->lock);
	ps3stor_cli_printf("del inject_task host_no %u channel %u id %u"
		" cmdtype 0x%u cmdSubtype 0x%u dealType 0x%u times %u!\n",
		pitem1->item.scsi_task_cmd.host_no, pitem1->item.scsi_task_cmd.channel,
		pitem1->item.scsi_task_cmd.id, pitem1->item.scsi_task_cmd.cmd_type,
		pitem1->item.scsi_task_cmd.cmd_sub_type, pitem1->item.scsi_task_cmd.dealType,
		pitem1->item.scsi_task_cmd.inject_count);
	list_del(&pitem1->list);
	ps3_mutex_unlock(&p_inject_list->lock);
	kfree(pitem1);

l_err:
	kfree(pitem);
	return;
}
void ps3_del_inject_mgr(struct inject_cmds_t *pitem)
{
	struct inject_cmds_t *pitem1;
	PS3Inject_s *p_inject_list = get_inject();

	if ((pitem1 = ps3_inject_mgr_exist(pitem)) == NULL){
		ps3stor_cli_printf("mgr inject type is noexist!\n");
		goto l_err;
	}
	ps3_mutex_lock(&p_inject_list->lock);
	ps3stor_cli_printf("del inject_mgr host_no %u cmdtype 0x%u cmdSubtype 0x%u dealType 0x%u times %u!\n",
		pitem1->item.mgr_cmd.host_no, pitem1->item.mgr_cmd.cmd_type,
		pitem1->item.mgr_cmd.cmd_sub_type,pitem1->item.mgr_cmd.dealType,
		pitem1->item.mgr_cmd.inject_count);
	list_del(&pitem1->list);
	ps3_mutex_unlock(&p_inject_list->lock);
	kfree(pitem1);

l_err:
	kfree(pitem);
	return;
}

S32 ps3_add_inject_scsi_rw(struct inject_cmds_t *pitem)
{
	S32 iRet = PS3_SUCCESS;
	PS3Inject_s *p_inject_list = get_inject();

	if (ps3_inject_scsi_rw_exist(pitem) != NULL){
		ps3stor_cli_printf("rw inject type is exist!\n");
		iRet = -PS3_FAILED;
		goto l_err;
	}
	ps3_mutex_lock(&p_inject_list->lock);
	ps3stor_cli_printf("inject_rw host_no %u channel %u id %u lba 0x%llx len 0x%u type %u, time %u!\n",
		pitem->item.scsi_cmd.host_no, pitem->item.scsi_cmd.channel,
		pitem->item.scsi_cmd.id, pitem->item.scsi_cmd.lba,
		pitem->item.scsi_cmd.len, pitem->item.scsi_cmd.dealType,
		pitem->item.scsi_cmd.inject_count);
	list_add_tail(&pitem->list, &p_inject_list->scsi_rw_list);
	ps3_mutex_unlock(&p_inject_list->lock);
	goto l_out;

l_err:
	kfree(pitem);
l_out:
	return iRet;
}
S32 ps3_add_inject_scsi_task(struct inject_cmds_t *pitem)
{
	S32 iRet = PS3_SUCCESS;
	PS3Inject_s *p_inject_list = get_inject();

	if (ps3_inject_scsi_task_exist(pitem) != NULL){
		ps3stor_cli_printf("task inject type is exist!\n");
		iRet = -PS3_FAILED;
		goto l_err;
	}
	ps3_mutex_lock(&p_inject_list->lock);
	ps3stor_cli_printf("inject_task host_no %u channel %u id %u "
		"cmdtype 0x%u cmdSubtype 0x%u dealType 0x%u times %u!\n",
		pitem->item.scsi_task_cmd.host_no, pitem->item.scsi_task_cmd.channel,
		pitem->item.scsi_task_cmd.id, pitem->item.scsi_task_cmd.cmd_type,
		pitem->item.scsi_task_cmd.cmd_sub_type, pitem->item.scsi_task_cmd.dealType,
		pitem->item.scsi_task_cmd.inject_count);
	list_add_tail(&pitem->list, &p_inject_list->scsi_task_list);
	ps3_mutex_unlock(&p_inject_list->lock);
	goto l_out;

l_err:
	kfree(pitem);
l_out:
	return iRet;
}
S32 ps3_add_inject_mgr(struct inject_cmds_t *pitem)
{
	S32 iRet = PS3_SUCCESS;
	PS3Inject_s *p_inject_list = get_inject();

	if (ps3_inject_mgr_exist(pitem) != NULL){
		ps3stor_cli_printf("mgr inject type is exist!\n");
		iRet = -PS3_FAILED;
		goto l_err;
	}
	ps3_mutex_lock(&p_inject_list->lock);
	ps3stor_cli_printf("inject_task host_no %u cmdtype 0x%u cmdSubtype 0x%u dealType 0x%u times %u!\n",
		pitem->item.mgr_cmd.host_no, pitem->item.mgr_cmd.cmd_type,
		pitem->item.mgr_cmd.cmd_sub_type, pitem->item.mgr_cmd.dealType,
		pitem->item.mgr_cmd.inject_count);
	list_add_tail(&pitem->list, &p_inject_list->mgr_list);
	ps3_mutex_unlock(&p_inject_list->lock);
	goto l_out;

l_err:
	kfree(pitem);
l_out:
	return iRet;
}

static void ps3_inject_scsi_rw(int argc, char *argv[])
{
	struct inject_cmds_t *pitem = NULL;
	U32 host_no;
	U32 id;
	U32	channel;
	U64 lba;
	U32 len;
	S32 ret;
	U32 result;
	U32 sense_info;
	struct scsi_device *sdev;
	U32 times;
	U32 dealType;
	struct ps3_instance *instance;

	if (argc < 13) {
		ps3stor_cli_printf("Too few args for enject!\n");
		return;
	}

	pitem = (struct inject_cmds_t*)kmalloc(sizeof(struct inject_cmds_t), GFP_KERNEL);
	if (pitem == NULL) {
		return;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no !\n");
		goto l_err;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d invalid !\n",
			host_no);
		goto l_err;
	}
	pitem->item.scsi_cmd.host_no = host_no;

	if (strcmp(argv[3], "dev") != 0) {
		ps3stor_cli_printf("dev is needed!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[4], 0, &channel);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse channel!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[5], 0, &id);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse id!\n");
		goto l_err;
	}
	sdev = ps3_scsi_device_lookup(instance, channel, id, 0);
	if (sdev == NULL) {
		ps3stor_cli_printf("Invalid sdev [%d:%d:%d] invalid !\n",
			host_no, channel, id);
		goto l_err;
	}
	pitem->item.scsi_cmd.channel = channel;
	pitem->item.scsi_cmd.id = id;
	pitem->item.scsi_cmd.device = sdev;
	if (strcmp(argv[6], "lba") != 0) {
		ps3stor_cli_printf("lba is needed!\n");
		goto l_err;
	}

	ret = kstrtou64(argv[7], 0, &lba);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse lba!\n");
		goto l_err;
	}
	pitem->item.scsi_cmd.lba = lba;

	if (strcmp(argv[8], "len") != 0) {
		ps3stor_cli_printf("len is needed!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[9], 0, &len);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse len!\n");
		goto l_err;
	}
	pitem->item.scsi_cmd.len = len;

	ret = kstrtouint(argv[10], 0, &dealType);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse dealType!\n");
		goto l_err;
	}
	if (dealType != PS3_SCSI_CMD_TIMEOUT_FORCE_REPLY
		&& dealType != PS3_SCSI_CMD_ERROR) {
		ps3stor_cli_printf("err type is invalid!\n");
		goto l_err;
	}

	pitem->item.scsi_cmd.dealType = dealType;
	switch (pitem->item.scsi_cmd.dealType)
	{
	case PS3_SCSI_CMD_TIMEOUT_FORCE_REPLY:
		if (strcmp(argv[11], "times") != 0) {
			ps3stor_cli_printf("times is needed!\n");
			goto l_err;
		}

		ret = kstrtouint(argv[12], 0, &times);
		if (ret != 0) {
			ps3stor_cli_printf("Can not parse times!\n");
			goto l_err;
		}
		pitem->item.scsi_cmd.inject_count = times;
		break;
	case PS3_SCSI_CMD_ERROR:
		if (strcmp(argv[11], "result") != 0) {
			ps3stor_cli_printf("result is needed!\n");
			goto l_err;
		}

		ret = kstrtouint(argv[12], 0, &result);
		if (ret != 0) {
			ps3stor_cli_printf("Can not parse result!\n");
			goto l_err;
		}
		pitem->item.scsi_cmd.cmdDeal.errReply.result = result;

		if (strcmp(argv[13], "sense") != 0) {
			ps3stor_cli_printf("sense is needed!\n");
			goto l_err;
		}

		ret = kstrtouint(argv[14], 0, &sense_info);
		if (ret != 0) {
			ps3stor_cli_printf("Can not parse sense!\n");
			goto l_err;
		}
		pitem->item.scsi_cmd.cmdDeal.errReply.sshdr.resp_code = sense_info & 0xf;
		pitem->item.scsi_cmd.cmdDeal.errReply.sshdr.sense_key = sense_info >> 8 & 0xf;
		pitem->item.scsi_cmd.cmdDeal.errReply.sshdr.asc = sense_info >> 16 & 0xf;
		pitem->item.scsi_cmd.cmdDeal.errReply.sshdr.ascq = sense_info >> 24 & 0xf;

		if (strcmp(argv[15], "times") != 0) {
			ps3stor_cli_printf("times is needed!\n");
			goto l_err;
		}

		ret = kstrtouint(argv[16], 0, &times);
		if (ret != 0) {
			ps3stor_cli_printf("Can not parse times!\n");
			goto l_err;
		}
		pitem->item.scsi_cmd.inject_count = times;
		break;
	default:
		ps3stor_cli_printf("handle type is invalid!\n");
			goto l_err;
		break;
	}
	ps3_add_inject_scsi_rw(pitem);
	goto l_out;

l_err:
	kfree(pitem);
l_out:
	return;
}
static void ps3_inject_scsi_task(int argc, char *argv[])
{
	struct inject_cmds_t *pitem = NULL;
	U32 host_no;
	U32 id;
	U32	channel;
	S32 ret;
	struct scsi_device *sdev;
	U32 times;
	U32 dealType;
	struct ps3_instance *instance;

	if (argc < 10) {
		ps3stor_cli_printf("Too few args for enject!\n");
		return;
	}

	pitem = (struct inject_cmds_t *)kmalloc(sizeof(struct inject_cmds_t), GFP_KERNEL);
	if (pitem == NULL) {
		return;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no !\n");
		goto l_err;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d invalid !\n",
			host_no);
		goto l_err;
	}
	pitem->item.scsi_task_cmd.host_no = host_no;

	if (strcmp(argv[3], "dev") != 0) {
		ps3stor_cli_printf("dev is needed!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[4], 0, &channel);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse channel!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[5], 0, &id);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse id!\n");
		goto l_err;
	}
	sdev = ps3_scsi_device_lookup(instance, channel, id, 0);
	if (sdev == NULL) {
		ps3stor_cli_printf("Invalid sdev [%d:%d:%d] invalid !\n",
			host_no, channel, id);
		goto l_err;
	}
	pitem->item.scsi_task_cmd.channel = channel;
	pitem->item.scsi_task_cmd.id = id;

	if (strcmp(argv[6], "abort") == 0) {
		pitem->item.scsi_task_cmd.cmd_type = PS3_CMD_SCSI_TASK_MANAGEMENT;
		pitem->item.scsi_task_cmd.cmd_sub_type = PS3_TASK_CMD_SCSI_TASK_ABORT;
	} else if (strcmp(argv[6], "reset") == 0) {
		pitem->item.scsi_task_cmd.cmd_type = PS3_CMD_SCSI_TASK_MANAGEMENT;
		pitem->item.scsi_task_cmd.cmd_sub_type = PS3_TASK_CMD_SCSI_TASK_RESET;
	} else {
		ps3stor_cli_printf("type is invalid!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[7], 0, &dealType);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse dealType!\n");
		goto l_err;
	}
	if (dealType != PS3_SCSI_TASK_CMD_TIMEOUT && dealType != PS3_SCSI_TASK_CMD_ERROE &&
		dealType != PS3_SCSI_TASK_CMD_NORMAL) {
		ps3stor_cli_printf("deal type is invalid!\n");
		goto l_err;
	}
	pitem->item.scsi_task_cmd.dealType = dealType;

	if (strcmp(argv[8], "times") != 0) {
		ps3stor_cli_printf("times is needed!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[9], 0, &times);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse times!\n");
		goto l_err;
	}
	pitem->item.scsi_task_cmd.inject_count = times;

	ps3_add_inject_scsi_task(pitem);
	goto l_out;

l_err:
	kfree(pitem);
l_out:
	return;
}
static void ps3_inject_mgr(int argc, char *argv[])
{
	struct inject_cmds_t *pitem = NULL;
	U32 host_no;
	U8 cmd_type;
	U8 cmd_sub_type;
	S32 ret;
	U32 times;
	U32 dealType;
	U32 errType;
	struct ps3_instance *instance;

	if (argc < 10) {
		ps3stor_cli_printf("Too few args for enject!\n");
		return;
	}

	pitem = (struct inject_cmds_t *)kmalloc(sizeof(struct inject_cmds_t), GFP_KERNEL);
	if (pitem == NULL) {
		return;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no !\n");
		goto l_err;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d invalid !\n",
			host_no);
		goto l_err;
	}
	pitem->item.scsi_task_cmd.host_no = host_no;

	if (strcmp(argv[3], "cmd_type") != 0) {
		ps3stor_cli_printf("cmd_type is needed!\n");
			goto l_err;
	}

	ret = kstrtou8(argv[4], 0, &cmd_type);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse cmd_type!\n");
		goto l_err;
	}
	pitem->item.mgr_cmd.cmd_type = cmd_type;

	if (strcmp(argv[5], "cmd_sub_type") != 0) {
		ps3stor_cli_printf("cmd_sub_type is needed!\n");
		goto l_err;
	}

	ret = kstrtou8(argv[6], 0, &cmd_sub_type);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse cmd_sub_type!\n");
		goto l_err;
	}
	pitem->item.mgr_cmd.cmd_sub_type = cmd_sub_type;

	ret = kstrtouint(argv[7], 0, &dealType);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse dealType!\n");
		goto l_err;
	}

	if (dealType == PS3_MGR_CMD_TIMEOUT) {
		if (strcmp(argv[8], "times") != 0) {
			ps3stor_cli_printf("times is needed!\n");
			goto l_err;
		}

		ret = kstrtouint(argv[9], 0, &times);
		if (ret != 0) {
			ps3stor_cli_printf("Can not parse times!\n");
			goto l_err;
		}
	} else if (dealType == PS3_MGR_CMD_ERROE) {
		ret = kstrtouint(argv[8], 0, &errType);
		if (ret != 0) {
			ps3stor_cli_printf("Can not parse errType!\n");
			goto l_err;
		}
		pitem->item.mgr_cmd.errType = errType;
		if (strcmp(argv[9], "times") != 0) {
			ps3stor_cli_printf("times is needed!\n");
			goto l_err;
		}

		ret = kstrtouint(argv[10], 0, &times);
		if (ret != 0) {
			ps3stor_cli_printf("Can not parse times!\n");
			goto l_err;
		}
	} else {
		ps3stor_cli_printf("deal type is invalid!\n");
		goto l_err;
	}

	pitem->item.mgr_cmd.dealType = dealType;
	pitem->item.mgr_cmd.inject_count = times;

	ps3_add_inject_mgr(pitem);
	goto l_out;

l_err:
	kfree(pitem);
l_out:
	return;
}

static void ps3_clear_inject_scsi_rw(int argc, char *argv[])
{
	struct inject_cmds_t *pitem = NULL;

	U32 host_no;
	U32 id;
	U32	channel;
	U64 lba;
	U32 len;
	S32 ret;
	struct scsi_device *sdev;
	struct ps3_instance *instance;

	if (argc < 10) {
		ps3stor_cli_printf("Too few args for enject!\n");
		return;
	}

	pitem = (struct inject_cmds_t*)kmalloc(sizeof(struct inject_cmds_t), GFP_KERNEL);
	if (pitem == NULL) {
		return;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no !\n");
		goto l_err;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d invalid !\n",
			host_no);
		goto l_err;
	}
	pitem->item.scsi_cmd.host_no = host_no;

	if (strcmp(argv[3], "dev") != 0) {
		ps3stor_cli_printf("dev is needed!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[4], 0, &channel);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse channel!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[5], 0, &id);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse id!\n");
		goto l_err;
	}
	sdev = ps3_scsi_device_lookup(instance, channel, id, 0);
	if (sdev == NULL) {
		ps3stor_cli_printf("Invalid sdev [%d:%d:%d] invalid !\n",
			host_no, channel, id);
		goto l_err;
	}
	pitem->item.scsi_cmd.channel = channel;
	pitem->item.scsi_cmd.id = id;
	pitem->item.scsi_cmd.device = sdev;
	if (strcmp(argv[6], "lba") != 0) {
		ps3stor_cli_printf("lba is needed!\n");
		goto l_err;
	}

	ret = kstrtou64(argv[7], 0, &lba);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse lba!\n");
		goto l_err;
	}
	pitem->item.scsi_cmd.lba = lba;

	if (strcmp(argv[8], "len") != 0) {
		ps3stor_cli_printf("len is needed!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[9], 0, &len);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse len!\n");
		goto l_err;
	}
	pitem->item.scsi_cmd.len = len;

	ps3_del_inject_scsi_rw(pitem);
	return;

l_err:
	kfree(pitem);
	return;
}
static void ps3_clear_inject_scsi_task(int argc, char *argv[])
{
	struct inject_cmds_t *pitem = NULL;

	U32 host_no;
	U32 id;
	U32	channel;
	S32 ret;
	struct scsi_device *sdev;
	struct ps3_instance *instance;

	if (argc < 7) {
		ps3stor_cli_printf("Too few args for enject!\n");
		return;
	}

	pitem = (struct inject_cmds_t *)kmalloc(sizeof(struct inject_cmds_t), GFP_KERNEL);
	if (pitem == NULL) {
		return;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no !\n");
		goto l_err;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d invalid !\n",
			host_no);
		goto l_err;
	}
	pitem->item.scsi_task_cmd.host_no = host_no;

	if (strcmp(argv[3], "dev") != 0) {
		ps3stor_cli_printf("dev is needed!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[4], 0, &channel);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse channel!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[5], 0, &id);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse id!\n");
		goto l_err;
	}
	sdev = ps3_scsi_device_lookup(instance, channel, id, 0);
	if (sdev == NULL) {
		ps3stor_cli_printf("Invalid sdev [%d:%d:%d] invalid !\n",
			host_no, channel, id);
		goto l_err;
	}
	pitem->item.scsi_task_cmd.channel = channel;
	pitem->item.scsi_task_cmd.id = id;

	if (strcmp(argv[6], "abort") == 0) {
		pitem->item.scsi_task_cmd.cmd_type = PS3_CMD_SCSI_TASK_MANAGEMENT;
		pitem->item.scsi_task_cmd.cmd_sub_type = PS3_TASK_CMD_SCSI_TASK_ABORT;
	} else if (strcmp(argv[6], "reset") == 0) {
		pitem->item.scsi_task_cmd.cmd_type = PS3_CMD_SCSI_TASK_MANAGEMENT;
		pitem->item.scsi_task_cmd.cmd_sub_type = PS3_TASK_CMD_SCSI_TASK_RESET;
	} else {
		ps3stor_cli_printf("type is invalid!\n");
		goto l_err;
	}
	ps3_del_inject_scsi_task(pitem);
	return;

l_err:
	kfree(pitem);
	return;
}

static void ps3_clear_inject_mgr(int argc, char *argv[])
{
	struct inject_cmds_t *pitem = NULL;
	U32 host_no;
	U32 cmd_type;
	U32	cmd_sub_type;
	S32 ret;
	struct ps3_instance *instance;

	if (argc < 7) {
		ps3stor_cli_printf("Too few args for enject!\n");
		return;
	}

	pitem = (struct inject_cmds_t *)kmalloc(sizeof(struct inject_cmds_t), GFP_KERNEL);
	if (pitem == NULL) {
		return;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no !\n");
		goto l_err;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d invalid !\n",
			host_no);
		goto l_err;
	}
	pitem->item.mgr_cmd.host_no = host_no;

	if (strcmp(argv[3], "cmd_type") != 0){
		ps3stor_cli_printf("cmd_type is invalid!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[4], 0, &cmd_type);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse cmd_type !\n");
		goto l_err;
	}
	pitem->item.mgr_cmd.cmd_type = cmd_type;

	if (strcmp(argv[5], "cmd_sub_type") != 0){
		ps3stor_cli_printf("cmd_sub_type is invalid!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[6], 0, &cmd_sub_type);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse cmd_type !\n");
		goto l_err;
	}
	pitem->item.mgr_cmd.cmd_sub_type = cmd_sub_type;

	ps3_del_inject_mgr(pitem);
	return;

l_err:
	kfree(pitem);
	return;
}

static void ps3_clear_all_inject(int argc, char *argv[])
{
	ps3_clear_all_inject_scsi_rw();
	ps3_clear_all_inject_scsi_task();
	ps3_clear_all_inject_mgr();
}

static void ps3_show_inject_scsi_rw(int argc, char *argv[])
{
	struct inject_cmds_t *pitem = NULL;

	U32 host_no;
	U32 id;
	U32	channel;
	U64 lba;
	U32 len;
	S32 ret;
	struct scsi_device *sdev;
	struct ps3_instance *instance;

	if (argc < 10) {
		ps3stor_cli_printf("Too few args for enject!\n");
		return;
	}

	pitem = (struct inject_cmds_t*)kmalloc(sizeof(struct inject_cmds_t), GFP_KERNEL);
	if (pitem == NULL) {
		return;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no !\n");
		goto l_err;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d invalid !\n",
			host_no);
		goto l_err;
	}
	pitem->item.scsi_cmd.host_no = host_no;

	if (strcmp(argv[3], "dev") != 0) {
		ps3stor_cli_printf("dev is needed!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[4], 0, &channel);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse channel!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[5], 0, &id);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse id!\n");
		goto l_err;
	}
	sdev = ps3_scsi_device_lookup(instance, channel, id, 0);
	if (sdev == NULL) {
		ps3stor_cli_printf("Invalid sdev [%d:%d:%d] invalid !\n",
			host_no, channel, id);
		goto l_err;
	}
	pitem->item.scsi_cmd.channel = channel;
	pitem->item.scsi_cmd.id = id;
	if (strcmp(argv[6], "lba") != 0) {
		ps3stor_cli_printf("lba is needed!\n");
		goto l_err;
	}

	ret = kstrtou64(argv[7], 0, &lba);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse lba!\n");
		goto l_err;
	}
	pitem->item.scsi_cmd.lba = lba;

	if (strcmp(argv[8], "len") != 0) {
		ps3stor_cli_printf("len is needed!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[9], 0, &len);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse len!\n");
		goto l_err;
	}
	pitem->item.scsi_cmd.len = len;
	ps3_show_inject_scsi_rw_item(pitem);
	return;

l_err:
	kfree(pitem);
	return;
}
static void ps3_show_inject_scsi_task(int argc, char *argv[])
{
	struct inject_cmds_t *pitem = NULL;

	U32 host_no;
	U32 id;
	U32	channel;
	S32 ret;
	struct scsi_device *sdev;
	struct ps3_instance *instance;

	if (argc < 7) {
		ps3stor_cli_printf("Too few args for enject!\n");
		return;
	}

	pitem = (struct inject_cmds_t *)kmalloc(sizeof(struct inject_cmds_t), GFP_KERNEL);
	if (pitem == NULL) {
		return;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no !\n");
		goto l_err;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d invalid !\n",
			host_no);
		goto l_err;
	}
	pitem->item.scsi_task_cmd.host_no = host_no;

	if (strcmp(argv[3], "dev") != 0) {
		ps3stor_cli_printf("dev is needed!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[4], 0, &channel);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse channel!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[5], 0, &id);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse id!\n");
		goto l_err;
	}
	sdev = ps3_scsi_device_lookup(instance, channel, id, 0);
	if (sdev == NULL) {
		ps3stor_cli_printf("Invalid sdev [%d:%d:%d] invalid !\n",
			host_no, channel, id);
		goto l_err;
	}
	pitem->item.scsi_task_cmd.channel = channel;
	pitem->item.scsi_task_cmd.id = id;

	if (strcmp(argv[6], "abort") == 0) {
		pitem->item.scsi_task_cmd.cmd_type = PS3_CMD_SCSI_TASK_MANAGEMENT;
		pitem->item.scsi_task_cmd.cmd_sub_type = PS3_TASK_CMD_SCSI_TASK_ABORT;
	} else if (strcmp(argv[6], "reset") == 0) {
		pitem->item.scsi_task_cmd.cmd_type = PS3_CMD_SCSI_TASK_MANAGEMENT;
		pitem->item.scsi_task_cmd.cmd_sub_type = PS3_TASK_CMD_SCSI_TASK_RESET;
	} else {
		ps3stor_cli_printf("type is invalid!\n");
		goto l_err;
	}
	ps3_show_inject_scsi_task_item(pitem);
	return;

l_err:
	kfree(pitem);
	return;
}
static void ps3_show_inject_mgr(int argc, char *argv[])
{
	struct inject_cmds_t *pitem = NULL;

	U32 host_no;
	U32 cmd_type;
	U32	cmd_sub_type;
	S32 ret;
	struct ps3_instance *instance;

	if (argc < 7) {
		ps3stor_cli_printf("Too few args for enject!\n");
		return;
	}

	pitem = (struct inject_cmds_t *)kmalloc(sizeof(struct inject_cmds_t), GFP_KERNEL);
	if (pitem == NULL) {
		return;
	}

	if (strcmp(argv[1], "host_no") != 0) {
		ps3stor_cli_printf("host_no is needed!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[2], 0, &host_no);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse host_no !\n");
		goto l_err;
	}

	instance = ps3_instance_lookup(host_no);
	if (instance == NULL) {
		ps3stor_cli_printf("Invalid host_no %d invalid !\n",
			host_no);
		goto l_err;
	}
	pitem->item.scsi_task_cmd.host_no = host_no;

	if (strcmp(argv[3], "cmd_type") != 0) {
		ps3stor_cli_printf("dev is needed!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[4], 0, &cmd_type);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse cmd_type!\n");
		goto l_err;
	}
	pitem->item.mgr_cmd.cmd_type = cmd_type;

	if (strcmp(argv[5], "cmd_sub_type") != 0) {
		ps3stor_cli_printf("cmd_sub_type is needed!\n");
		goto l_err;
	}

	ret = kstrtouint(argv[6], 0, &cmd_sub_type);
	if (ret != 0) {
		ps3stor_cli_printf("Can not parse cmd_sub_type!\n");
		goto l_err;
	}

	ps3_show_inject_mgr_item(pitem);
	return;

l_err:
	kfree(pitem);
	return;
}
static void ps3_show_all_inject(int argc, char *argv[])
{
	ps3_show_inject_all_scsi_rw(argc, argv);
	ps3_show_inject_all_scsi_task(argc, argv);
	ps3_show_inject_all_mgr(argc, argv);
	return;
}

void ps3_scsi_task_cmd_clear_all(void)
{
	struct inject_hit_cmds_t *p_cmd = NULL;
	struct inject_hit_cmds_t *p_cmd_next = NULL;
	struct inject_cmds_t *pitem = NULL;
	PS3Inject_s *p_inject_list = get_inject();
	PS3HitCmd_s * p_hit_inject = get_hit_inject();

	ps3_mutex_lock(&p_inject_list->lock);
	if (list_empty(&p_hit_inject->scsi_task_list)) {
		LOG_DEBUG("task inject list is empty!\n");
		goto l_out;
	}

	list_for_each_entry_safe(p_cmd, p_cmd_next, &p_hit_inject->scsi_task_list, list) {
		list_del(&p_cmd->list);
		kfree(p_cmd);
	}

	if(pitem->item.scsi_task_cmd.inject_count == 0) {
		ps3_delete_scsi_task_inject(pitem);
	}

l_out:
	ps3_mutex_unlock(&p_inject_list->lock);
	return ;
}

void ps3_mgr_cmd_clear_all(void)
{
	struct inject_hit_cmds_t *p_cmd = NULL;
	struct inject_hit_cmds_t *p_cmd_next = NULL;
	struct inject_cmds_t *pitem = NULL;
	PS3Inject_s *p_inject_list = get_inject();
	PS3HitCmd_s * p_hit_inject = get_hit_inject();

	ps3_mutex_lock(&p_inject_list->lock);
	if (list_empty(&p_hit_inject->mgr_list)) {
		LOG_DEBUG("mgr inject list is empty!\n");
		goto l_out;
	}

	list_for_each_entry_safe(p_cmd, p_cmd_next, &p_hit_inject->mgr_list, list) {
		list_del(&p_cmd->list);
		kfree(p_cmd);
	}

	if(pitem->item.mgr_cmd.inject_count == 0) {
		ps3_delete_mgr_inject(pitem);
	}

l_out:
	ps3_mutex_unlock(&p_inject_list->lock);
	return ;
}
void ps3_scsi_rw_cmd_clear_all(void)
{
	struct inject_hit_cmds_t *p_cmd = NULL;
	struct inject_hit_cmds_t *p_cmd_next = NULL;
	PS3Inject_s *p_inject_list = get_inject();
	PS3HitCmd_s * p_hit_inject = get_hit_inject();

	ps3_mutex_lock(&p_inject_list->lock);
	if (list_empty(&p_hit_inject->scsi_rw_list)) {
		LOG_DEBUG("rw inject cmd list is empty!\n");
		goto l_out;
	}

	list_for_each_entry_safe(p_cmd, p_cmd_next, &p_hit_inject->scsi_rw_list, list) {
		list_del(&p_cmd->list);
		ps3_scsi_dma_unmap(p_cmd->cmd);
		ps3_scsi_cmd_free(p_cmd->cmd);
		kfree(p_cmd);
	}

l_out:
	ps3_mutex_unlock(&p_inject_list->lock);
	return ;
}

static void ps3_clear_hit_cmd(int argc, char *argv[])
{
	ps3_scsi_rw_cmd_clear_all();
	ps3_scsi_task_cmd_clear_all();
	ps3_mgr_cmd_clear_all();
}
void ps3_inject_clear(void)
{
	ps3_scsi_rw_cmd_clear_all();
	ps3_scsi_task_cmd_clear_all();
	ps3_mgr_cmd_clear_all();

	ps3_clear_all_inject_scsi_rw();
	ps3_clear_all_inject_scsi_task();
	ps3_clear_all_inject_mgr();
}
#endif

void ps3_cli_debug_init(void)
{
	int ret = 0;
	struct ps3_cli_debug_cmd *cmd = NULL;
	U32 table_index = 0;
	U32 table_size = ARRAY_SIZE(g_ps3_cli_debug_cmd_table);
	for (table_index = 0; table_index < table_size; table_index++) {
		cmd = &g_ps3_cli_debug_cmd_table[table_index];
		ret = ps3stor_cli_register(cmd->func, cmd->func_name, cmd->help);
		if (ret) {
			printk("cli register failed:%s:%d\n", cmd->func_name, ret);
		}
	}
	INJECT_INIT()
}
