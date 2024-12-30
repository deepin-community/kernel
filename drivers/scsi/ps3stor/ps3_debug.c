#ifndef _WINDOWS

#include <scsi/scsi_host.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/kallsyms.h>
#ifdef __KERNEL__
#include <linux/ctype.h>
#else
#include <ctype.h>
#endif
#include "ps3_htp_def.h"
#include "ps3_instance_manager.h"
#include "ps3_ioc_manager.h"
#include "ps3_ioc_state.h"

#include "ps3_debug.h"
#include "ps3_irq.h"
#include "ps3_driver_log.h"
#include "ps3_cli_debug.h"
#include "ps3_module_para.h"
#include "ps3_util.h"

#endif

#include "ps3_htp_def.h"
#include "ps3_instance_manager.h"
#include "ps3_event.h"
#include "ps3_dump.h"
#define REG_OFFSET_BY_ADDR(instance, reg) ((U8*)(reg) - ((U8*)(instance)->reg_set))

#ifndef _WINDOWS
typedef void (*dma_dump_mappings_cb)(struct device *dev);
#endif

struct ps3_reg_name {
	U64 	read_dump_interval_ms;
	U64 	write_dump_interval_ms;
	U32	reg_cnt; 
	S8	name[32];   
};

static U32 g_ps3_r1x_intrval_delay = 0;

static struct ps3_reg_name g_ps3_reg_name_table[] = {
	{0, 0, 8, "reserved0"},
	{0, 0, 1, "ps3Doorbell"},
	{0, 0, 1, "ps3DoorbellIrqClear"},
	{0, 0, 1, "ps3DoorbellIrqMask"},
	{0, 0, 1, "ps3IrqControl"},
	{0, 0, 20, "reserved1"},

	{0, 0, 1, "ps3SoftresetKey"},
	{1000, 0, 1, "ps3SoftresetState"},
	{0, 0, 1, "ps3Softreset"},
	{0, 0, 1, "ps3SoftresetIrqClear"},
	{0, 0, 1, "ps3SoftresetIrqMask"},
	{0, 0, 1, "ps3SoftresetKeyShiftRegLow"},
	{0, 0, 1, "ps3SoftresetKeyShiftRegHigh"},
	{0, 0, 1, "ps3SoftresetTimeCnt"},
	{0, 0, 1, "ps3SoftresetTimeOutEn"},
	{0, 0, 23, "reserved2"},

	{0, 0, 1, "ps3HardresetKey"},
	{1000, 0, 1, "ps3HardresetState"},
	{0, 0, 1, "ps3Hardreset"},
	{0, 0, 1, "ps3HardresetKeyShiftRegLow"},
	{0, 0, 1, "ps3HardresetKeyShiftRegHigh"},
	{0, 0, 1, "ps3HardresetTimeCnt"},
	{0, 0, 1, "ps3HardresetTimeOutEn"},
	{0, 0, 1, "ps3KeyGapCfg"},
	{0, 0, 1, "ps3HardresetIrqClear"},
	{0, 0, 1, "ps3HardresetIrqMask"},
	{0, 0, 22, "reserved3"},

	{10000, 0, 1, "ps3SocFwState"},
	{0, 0, 1, "ps3MaxFwCmd"},
	{0, 0, 1, "ps3MaxChainSize"},
	{0, 0, 1, "ps3MaxVdInfoSize"},
	{0, 0, 1, "ps3MaxNvmePageSize"},
	{0, 0, 1, "ps3FeatureSupport"},
	{0, 0, 1, "ps3FirmwareVersion"},
	{0, 0, 1, "ps3MaxReplyque"},
	{0, 0, 1, "ps3HardwareVersion"},
	{0, 0, 1, "ps3MgrQueueDepth"},
	{0, 0, 1, "ps3CmdQueueDepth"},
	{0, 0, 1, "ps3TfifoDepth"},
	{0, 0, 1, "ps3MaxSecR1xCmds"},
	{0, 0, 19, "reserved4"},

	{0, 0, 1, "ps3HilAdvice2directCnt0"},
	{0, 0, 1, "ps3HilAdvice2directCnt1"},
	{0, 0, 1, "ps3HilAdvice2directCnt2"},
	{0, 0, 1, "ps3HilAdvice2directCnt3"},
	{0, 0, 1, "ps3HilAdvice2directCntAll"},
	{0, 0, 3, "reserved5"},
	{0, 0, 1, "ps3IrqStatusRpt"},
	{0, 0, 23, "reserved6"},

	{0, 0, 1, "ps3DumpCtrl"},
	{0, 0, 1, "ps3DumpCtrlIrqClear"},
	{0, 0, 1, "ps3DumpCtrlIrqMask"},
	{0, 0, 1, "ps3DumpStatus"},
	{0, 0, 1, "ps3DumpDataSize"},
	{0, 0, 27, "reserved7"},

	{0, 0, 1, "ps3CmdTrigger"},
	{0, 0, 1, "ps3CmdTriggerIrqClear"},
	{0, 0, 1, "ps3CmdTriggerIrqMask"},
	{0, 0, 1, "ps3SoftresetCounter"},
	{0, 0, 1, "ps3RegCmdState"},
	{0, 0, 1, "ps3Debug0"},
	{0, 0, 1, "ps3Debug0IrqClear"},
	{0, 0, 1, "ps3Debug0IrqMask"},
	{0, 0, 1, "ps3Debug1"},
	{0, 0, 1, "ps3Debug1IrqClear"},
	{0, 0, 1, "ps3Debug1IrqMask"},
	{0, 0, 1, "ps3Debug2"},
	{0, 0, 1, "ps3Debug2IrqClear"},
	{0, 0, 1, "ps3Debug2IrqMask"},
	{0, 0, 1, "ps3Debug3"},
	{0, 0, 1, "ps3Debug3IrqClear"},
	{0, 0, 1, "ps3Debug3IrqMask"},
	{0, 0, 1, "ps3Debug4"},
	{0, 0, 1, "ps3Debug4IrqClear"},
	{0, 0, 1, "ps3Debug4IrqMask"},
	{0, 0, 1, "ps3Debug5"},
	{0, 0, 1, "ps3Debug6"},
	{0, 0, 1, "ps3Debug7"},
	{0, 0, 1, "ps3Debug8"},
	{0, 0, 1, "ps3Debug9"},
	{0, 0, 1, "ps3Debug10"},
	{0, 0, 1, "ps3Debug11"},
	{0, 0, 1, "ps3Debug12"},
	{0, 0, 4, "reserved8"},

	{0, 0, 1, "ps3SessioncmdAddr"},
	{0, 0, 1, "ps3SessioncmdAddrIrqClear"},
	{0, 0, 1, "ps3SessioncmdAddrIrqMask"},
	{0, 0, 7965, "reserved9"},
	{0, 0, 1, "ps3RequestQueue"},
	{0, 0, 1, "fifoErrCnt"},
	{0, 0, 1, "fifoStatus"},
	{0, 0, 1, "fifoLevelConfig"},
	{0, 0, 1, "fifoRst"},
	{0, 0, 1, "fifoIOCnt"},
	{0, 0, 1, "fifoFlowCnt"},
	{0, 0, 1, "fifoIntStatus"},
	{0, 0, 1, "fifoIntSet"},
	{0, 0, 1, "fifoIntClr"},
	{0, 0, 1, "fifoIntMask"},
	{0, 0, 1, "fifoCntClr"},
	{0, 0, 1, "fifoOrderError"},
	{0, 0, 4, "fifoDinShift"},
	{0, 0, 4, "fifoDoutShift"},
	{0, 0, 1, "fifoStatusMaxLevel"},
	{0, 0, 1, "fifoInit"},
	{0, 0, 1, "fifoinitEn"},
	{0, 0, 1, "fifoinitMax"},
	{0, 0, 1, "fifoStatusEccCnt"},
	{0, 0, 1, "fifoStatusEccAddr"},
	{0, 0, 1, "fifoDecoderOverflow"},
	{0, 0, 1, "fifoEccBadProject"},
	{0, 0, 1, "fifoOverFlowWord"},
	{0, 0, 34, "reserved10"},

	{0, 0, 1, "ps3FucntionLock"},
	{0, 0, 1, "ps3FunctionLockOwner"},
	{0, 0, 30, "reserved11"},
};
static inline U64 ps3_util_now_timestamp_ms_get(void)
{
	U64 timenow = 0;
#ifndef _WINDOWS

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
	struct timespec now;
	now = current_kernel_time();
#else
	struct timespec64 now;
	ktime_get_coarse_real_ts64(&now);
#endif
	timenow = now.tv_sec * 1000 + now.tv_nsec / 1000000;
#else
	LARGE_INTEGER now;
	StorPortQuerySystemTime(&now);

	timenow = (U64)(now.QuadPart / 10000);
#endif
	return timenow;

}

static void ps3_reg_dump_attr_init(struct ps3_instance *instance)
{
	U32 local_index = 0;
	U32 count = 0;
	U32 table_index = 0;
	U32 table_cnt = 0;
	U32 table_size = (sizeof(g_ps3_reg_name_table)/sizeof(g_ps3_reg_name_table[0]));
	struct ps3_reg_dump_attr *reg_dump = instance->debug_context.reg_dump;

	for (table_index = 0; table_index < table_size; table_index++) {
		table_cnt = g_ps3_reg_name_table[table_index].reg_cnt;

		for (local_index = 0; local_index < table_cnt; local_index++) {
			if (count >= (PS3_REGISTER_SET_SIZE/sizeof(U64))) {
				goto l_out;
			}

			reg_dump[count].read_dump_timestamp = 0;
			reg_dump[count].write_dump_timestamp = 0;
			reg_dump[count].read_dump_interval_ms =
				g_ps3_reg_name_table[table_index].read_dump_interval_ms;
			reg_dump[count].write_dump_interval_ms =
				g_ps3_reg_name_table[table_index].write_dump_interval_ms;
			memcpy(reg_dump[count].name,
				g_ps3_reg_name_table[table_index].name, 32);
			reg_dump[count].lastest_value = 0;

			count++;
		}
	}
l_out:
	return;
}

void ps3_reg_dump(struct ps3_instance *instance,
	void __iomem *reg, U64 value, Bool is_read)
{
	Bool is_dump = PS3_FALSE;
	const S8 *dump_type = NULL;
	U64 timestamp_ms = ps3_util_now_timestamp_ms_get();
	U64 last_dump_timestamp = 0;
	U64 dump_interval = 0;
	U32 reg_index = (U32)REG_OFFSET_BY_ADDR(instance, reg)/sizeof(U64);
	struct ps3_reg_dump_attr *reg_dump = instance->debug_context.reg_dump;

	if( (reg == NULL) ||
		((U8*)reg < (U8*)instance->reg_set) ||
		(((U8*)reg - (U8*)instance->reg_set) > PS3_REGISTER_SET_SIZE)) {
		return;
	}

	if (is_read) {
		dump_type = "reg read";
		last_dump_timestamp = reg_dump[reg_index].read_dump_timestamp;
		dump_interval = reg_dump[reg_index].read_dump_interval_ms;
	} else {
		dump_type = "reg write";
		last_dump_timestamp = reg_dump[reg_index].write_dump_timestamp;
		dump_interval = reg_dump[reg_index].write_dump_interval_ms;
	}

	if (timestamp_ms - last_dump_timestamp >= dump_interval) {
		is_dump = PS3_TRUE;
	}

	if (reg_dump[reg_index].lastest_value != value) {
		is_dump = PS3_TRUE;
		reg_dump[reg_index].lastest_value = value;
	}

	if (!is_dump) {
		return;
	}

	LOG_DEBUG("hno:%u %s:0x%04x:%s:0x%08llx \n",
		PS3_HOST(instance), dump_type,
		(U32)(reg_index * sizeof(U64)),
		reg_dump[reg_index].name, value);
	if (is_read) {
		reg_dump[reg_index].read_dump_timestamp = timestamp_ms;
	} else {
		reg_dump[reg_index].write_dump_timestamp = timestamp_ms;
	}
}

#ifndef _WINDOWS

static inline Bool ps3_is_alloc_debug_mem(U32 debug_mem_size)
{
	return (debug_mem_size != 0);
}

static S32 ps3_debug_mem_array_alloc(struct ps3_instance *instance,
	U32 debug_mem_size)
{
	S32 ret = PS3_SUCCESS;
	U32 array_num = 0;
	U32 alloc_size = 0;
	struct ps3_debug_context *ctx = &instance->debug_context;
	Ps3DebugMemEntry_s *entry = (Ps3DebugMemEntry_s*)ctx->debug_mem_buf;
	U8 i = 0;

	for (; i < PS3_DEBUG_MEM_ARRAY_MAX_NUM; ++i) {
		if (debug_mem_size == 0) {
			break;
		}

		array_num++;
		if (debug_mem_size <= PS3_MAX_DMA_MEM_SIZE) {
			alloc_size = debug_mem_size * 1024;
			ctx->debug_mem_vaddr[i].debugMemAddr =
				(U64)ps3_dma_alloc_coherent(instance, alloc_size,
					&entry[i].debugMemAddr);
			if (ctx->debug_mem_vaddr[i].debugMemAddr == 0) {
				LOG_ERROR("hno:%u alloc debug_mem[%u] end failed\n",
					PS3_HOST(instance), i);
				ret = -PS3_ENOMEM;
				goto l_out;
			}
			ctx->debug_mem_vaddr[i].debugMemSize = debug_mem_size;
			entry[i].debugMemSize = debug_mem_size;
			debug_mem_size = 0;
			LOG_INFO("hno:%u index[%u] vaddr[0x%llx], dma[0x%llx], size[%u]KB\n",
				PS3_HOST(instance), i, ctx->debug_mem_vaddr[i].debugMemAddr,
				entry[i].debugMemAddr, entry[i].debugMemSize);
			break;
		} else {
			debug_mem_size -= PS3_MAX_DMA_MEM_SIZE;
			alloc_size = PS3_MAX_DMA_MEM_SIZE * 1024;
			ctx->debug_mem_vaddr[i].debugMemAddr =
				(U64)ps3_dma_alloc_coherent(instance, alloc_size,
					&entry[i].debugMemAddr);
			if (ctx->debug_mem_vaddr[i].debugMemAddr == 0) {
				LOG_ERROR("hno:%u alloc debug_mem index[%u] failed\n",
					PS3_HOST(instance), i);
				ret = -PS3_ENOMEM;
				goto l_out;
			}
			ctx->debug_mem_vaddr[i].debugMemSize = PS3_MAX_DMA_MEM_SIZE;
			entry[i].debugMemSize = PS3_MAX_DMA_MEM_SIZE;
		}

		LOG_INFO("hno:%u index[%u] m vaddr[0x%llx], dma[0x%llx], size[%u]KB\n",
			PS3_HOST(instance), i, ctx->debug_mem_vaddr[i].debugMemAddr,
			entry[i].debugMemAddr, entry[i].debugMemSize);
	}
	ctx->debug_mem_array_num = array_num;
	LOG_INFO("hno:%u debug mem array num [%u]\n", PS3_HOST(instance), array_num);

l_out:
	return ret;
}

S32 ps3_debug_mem_alloc(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	U32 debug_mem_size = ps3_debug_mem_size_query();
	U32 buf_size = PS3_DEBUG_MEM_ARRAY_MAX_NUM * sizeof(Ps3DebugMemEntry_s);

	if (!ps3_is_alloc_debug_mem(debug_mem_size)) {
		goto l_out;
	}

	if (debug_mem_size > PS3_MAX_DEBUG_MEM_SIZE_PARA) {
		ret = -PS3_ENOMEM;
		goto l_out;
	}

	instance->debug_context.debug_mem_buf = (Ps3DebugMemEntry_s*)ps3_dma_alloc_coherent(instance,
		buf_size, &instance->debug_context.debug_mem_buf_phy);
	if (instance->debug_context.debug_mem_buf == NULL) {
		LOG_ERROR("hno:%u alloc debug_mem_buf failed\n", PS3_HOST(instance));
		ret = -PS3_ENOMEM;
		goto l_out;
	}
	memset(instance->debug_context.debug_mem_buf, 0, buf_size);

	ret = ps3_debug_mem_array_alloc(instance, debug_mem_size);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u alloc debug_mem_array failed\n", PS3_HOST(instance));
		ret = -PS3_ENOMEM;
		goto l_free;
	}

	LOG_INFO("hno:%u alloc debug_mem buf vaddr[0x%p], dma[0x%llx], size[%u]KB\n",
		PS3_HOST(instance), instance->debug_context.debug_mem_buf,
		instance->debug_context.debug_mem_buf_phy, buf_size);

	goto l_out;

l_free:
	ps3_debug_mem_free(instance);
l_out:
	return ret;
}

S32 ps3_debug_mem_free(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_debug_context *ctx = &instance->debug_context;
	Ps3DebugMemEntry_s *entry = ctx->debug_mem_buf;
	U32 size = 0;
	U8 i = 0;

	if (entry == NULL) {
		ret = PS3_SUCCESS;
		goto l_out;
	}

	for (i = 0; i < PS3_DEBUG_MEM_ARRAY_MAX_NUM; ++i) {
		if (ctx->debug_mem_vaddr[i].debugMemAddr == 0) {
			continue;
		}
		size = ctx->debug_mem_vaddr[i].debugMemSize * 1024;
		LOG_INFO("dma free size[%u] addr[0x%llx]\n",
			size, ctx->debug_mem_vaddr[i].debugMemAddr);
		(void)ps3_dma_free_coherent(instance, size,
			(void*)ctx->debug_mem_vaddr[i].debugMemAddr,
			entry[i].debugMemAddr);
	}
	memset(ctx->debug_mem_vaddr, 0, sizeof(ctx->debug_mem_vaddr));

	size = PS3_DEBUG_MEM_ARRAY_MAX_NUM * sizeof(Ps3DebugMemEntry_s);
	(void)ps3_dma_free_coherent(instance, size,
		ctx->debug_mem_buf, ctx->debug_mem_buf_phy);
	ctx->debug_mem_buf = NULL;
	ctx->debug_mem_buf_phy = 0;
	LOG_INFO("free debug mem end\n");

l_out:
	return ret;
}

void ps3_debug_context_init(struct ps3_instance *instance)
{
	memset(&instance->debug_context, 0, sizeof(struct ps3_debug_context));
	instance->debug_context.io_trace_switch = PS3_FALSE;

	ps3_reg_dump_attr_init(instance);
}

static inline struct ps3_instance *ps3_debug_instance_query(struct device *cdev,
	struct device_attribute *attr, const char *buf)
{
	struct Scsi_Host *shost = NULL;
	struct ps3_instance *instance = NULL;

	if (cdev == NULL || buf == NULL) {
		instance = NULL;
		goto l_out;
	}
	(void)attr;

	shost = class_to_shost(cdev);
	if (shost == NULL) {
		instance = NULL;
		goto l_out;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,19,0)
	if (shost->hostdata == NULL) {
		instance = NULL;
		goto l_out;
	}
#endif
	instance = (struct ps3_instance *)shost->hostdata;

l_out:
	return instance;
}

ssize_t ps3_vd_io_outstanding_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct ps3_instance *instance = ps3_debug_instance_query(cdev,
		attr, buf);

	if (instance == NULL) {
		ret = 0;
		goto l_out;
	}

	ret = snprintf(buf, PAGE_SIZE, "%d\n",
		atomic_read(&instance->cmd_statistics.vd_io_outstanding));

l_out:
	return ret;
}

ssize_t ps3_io_outstanding_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct ps3_instance *instance = ps3_debug_instance_query(cdev,
		attr, buf);

	if (instance == NULL) {
		ret = 0;
		goto l_out;
	}

	ret = snprintf(buf, PAGE_SIZE, "%d\n",
		atomic_read(&instance->cmd_statistics.io_outstanding));

l_out:
	return ret;
}

ssize_t ps3_is_load_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct ps3_instance *instance = ps3_debug_instance_query(cdev,
		attr, buf);

	if (instance == NULL) {
		ret = 0;
		goto l_out;
	}

	ret = snprintf(buf, PAGE_SIZE, "%d\n", instance->state_machine.is_load);

l_out:
	return ret;
}
ssize_t ps3_dump_ioc_regs_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct ps3_instance *instance = ps3_debug_instance_query(cdev,
		attr, buf);

	if (instance == NULL) {
		ret = 0;
		goto l_out;
	}

	ret = ps3_ioc_reg_dump(instance, buf);

l_out:
	return ret;
}

ssize_t ps3_max_scsi_cmds_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct ps3_instance *instance = ps3_debug_instance_query(cdev,
		attr, buf);

	if (instance == NULL) {
		ret = 0;
		goto l_out;
	}

	ret = snprintf(buf, PAGE_SIZE, "%d\n",
		instance->cmd_context.max_scsi_cmd_count);

l_out:
	return ret;
}
#endif
static ssize_t ps3_event_map_get(U32 event_type_map, char *buf,
	ssize_t left_len)
{
	ssize_t len = 0;
	U32 mask_bit = 0X00000001;
	U32 idx = 0;
	U32 event_type = event_type_map & mask_bit;
	U32 line_cnt = 2;

	while (mask_bit != 0) {
		if (event_type != 0) {
			if (idx && (!(idx % line_cnt))) {
				len += snprintf(buf + len,
					left_len - len, "\n");
			}

			len += snprintf(buf + len, left_len - len, "\t%s",
				ps3_event_print((MgrEvtType_e)event_type));
		}

		mask_bit = mask_bit << 1;
		event_type = event_type_map & mask_bit;
		idx++;
	}

	len += snprintf(buf + len, left_len - len, "\n");
	return len;
}

void ps3_r1x_interval_delay_set(u32 r1x_interval_delay)
{
	g_ps3_r1x_intrval_delay = r1x_interval_delay;
}

U32 ps3_r1x_interval_delay_get(void)
{
	return g_ps3_r1x_intrval_delay;
}

ssize_t ps3_event_subscribe_info_get(struct ps3_instance *instance,
	char *buf, ssize_t total_len)
{
	ssize_t len = 0;
	struct ps3_cmd *cmd = NULL;
	PS3MgrReqFrame_s *mgr_req_frame = NULL;
	struct PS3MgrEvent *event_req_info = NULL;
	ULong flags = 0;
	ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags);
	cmd = instance->event_context.event_cmd;
	if (cmd != NULL) {
		mgr_req_frame = (PS3MgrReqFrame_s *)cmd->req_frame;
		event_req_info =
			(struct PS3MgrEvent *)&mgr_req_frame->value.event;

		len += snprintf(buf + len, total_len - len,
			"Event subscribe cmd index:\t%d\n",
			cmd->index);

		len += snprintf(buf + len, total_len - len,
			"Event subscribe level:\t%d\n",
			event_req_info->eventLevel);

		len += snprintf(buf + len, total_len - len,
			"Event subscribe type:\n");
		len += ps3_event_map_get(event_req_info->eventTypeMap,
			buf + len, total_len - len);

		len += snprintf(buf + len, total_len - len,
			"Event proc failed type:\n");
		len += ps3_event_map_get(event_req_info->eventTypeMapProcResult,
			buf + len, total_len - len);
		goto l_out;
	}
l_out:
	ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags);
	return len;
}
#ifndef _WINDOWS
ssize_t ps3_event_subscribe_info_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct ps3_instance *instance = ps3_debug_instance_query(cdev,
		attr, buf);

	if (instance == NULL) {
		ret = 0;
		goto l_out;
	}

	ret = ps3_event_subscribe_info_get(instance, buf, PAGE_SIZE);
l_out:
	return ret;
}

static ssize_t ps3_ioc_state_dump(struct ps3_instance *instance, char *buf)
{
	ssize_t len = 0;
	ssize_t total_len = PAGE_SIZE;

	len += snprintf(buf + len, total_len - len, "%s\n",
		ps3_ioc_state_print(instance->ioc_adpter->ioc_state_get(instance)));

	return len;
}

ssize_t ps3_ioc_state_show(struct device *cdev, struct device_attribute *attr,
	char *buf)
{
	ssize_t ret = 0;
	struct ps3_instance *instance = ps3_debug_instance_query(cdev,
		attr, buf);

	if (instance == NULL) {
		ret = 0;
		goto l_out;
	}

	ret = ps3_ioc_state_dump(instance, buf);
l_out:
	return ret;
}

ssize_t ps3_log_level_store(struct device *cdev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int level = 0;
	ssize_t ret = count;
	(void)attr;

	if (cdev == NULL || buf == NULL) {
		goto l_out;
	}

	if (kstrtoint(buf, 10, &level)) {
		LOG_ERROR("invalid log level, could not set log level\n");
		ret = -EINVAL;
		goto l_out;
	}

#if defined(PS3_SUPPORT_DEBUG) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
#else
	if (level > LEVEL_INFO) {
		LOG_INFO("log level limited INFO\n");
		level = LEVEL_INFO;
	}
#endif

	LOG_WARN("set log level to %d\n", level);

	ps3_level_set(level);
l_out:
	return ret;
}

ssize_t ps3_log_level_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	S32 level = ps3_level_get();
	(void)attr;

	if (cdev == NULL || buf == NULL) {
		goto l_out;
	}
	ret = snprintf(buf, PAGE_SIZE, "%d\n", level);

	LOG_DEBUG("get log level to %d\n", level);
l_out:
	return ret;
}

ssize_t ps3_io_trace_switch_store(struct device *cdev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	U8 trace_switch = 0;
	ssize_t ret = count;
	struct ps3_instance *instance = ps3_debug_instance_query(cdev,
		attr, buf);

	if (instance == NULL) {
		ret = count;
		goto l_out;
	}

	if (kstrtou8(buf, 10, &trace_switch)) {
		LOG_ERROR("invalid io trace switch, could not set\n");
		ret = count;
		goto l_out;
	}

	instance->debug_context.io_trace_switch = trace_switch;

	LOG_WARN("set io trace switch is %d\n",
		instance->debug_context.io_trace_switch);

l_out:
	return ret;
}

ssize_t ps3_io_trace_switch_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct ps3_instance *instance = ps3_debug_instance_query(cdev,
		attr, buf);

	if (instance == NULL) {
		ret = 0;
		goto l_out;
	}

	ret = snprintf(buf, PAGE_SIZE, "%d\n",
		instance->debug_context.io_trace_switch);

	LOG_DEBUG("get io trace switch is %d\n",
		instance->debug_context.io_trace_switch);
l_out:
	return ret;
}

ssize_t ps3_dump_state_show(struct device *cdev,struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct ps3_dump_context *ctxt = NULL;
	struct ps3_instance *instance = ps3_debug_instance_query(cdev,
		attr, buf);

	if (instance == NULL) {
		ret = 0;
		goto l_out;
	}

	ctxt = &instance->dump_context;
	ret = snprintf(buf, PAGE_SIZE,"%d \n",ctxt->dump_state);
l_out:
	return ret;
}

ssize_t ps3_dump_state_store(struct device *cdev,struct device_attribute *attr,
	const char *buf, size_t count)
{
	S32 val = 0;
	S32 ret = 0;
	struct ps3_dump_context *ctxt = NULL;
	(void)attr;

	ctxt = dev_to_dump_context (cdev);

	if (kstrtoint(buf, 0, &val) != 0) {
		ret = -EINVAL;
		goto l_ret;
	}

	if ((val != PS3_DUMP_STATE_INVALID) && (val != PS3_DUMP_STATE_ABORTED)) {
		LOG_ERROR("hno:%u dump state should be %d or %d,"
			"%d is an invalid value\n",PS3_HOST(ctxt->instance),
			PS3_DUMP_STATE_INVALID,PS3_DUMP_STATE_ABORTED,val);
		ret = -EINVAL;
		goto l_ret;
	}

	if (ps3_dump_state_set(ctxt, val) != PS3_SUCCESS) {
		ret = -EPERM;
		goto l_ret;
	}

	return count;
l_ret:
	return ret;
}

ssize_t ps3_product_model_show(struct device *cdev,struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct ps3_instance *instance = ps3_debug_instance_query(cdev, attr, buf);
	(void)attr;

	if (instance == NULL) {
		goto l_out;
	}

	ret = snprintf(buf, PAGE_SIZE, "%s\n", instance->product_model);
l_out:
	return ret;
}

ssize_t ps3_dump_dir_show(struct device *cdev,struct device_attribute *attr, char *buf)
{
	struct ps3_dump_context *ctxt;
	struct ps3_instance *instance = ps3_debug_instance_query(cdev,
		attr, buf);
	ssize_t ret = 0;

	if (instance == NULL) {
		goto l_out;
	}

	ctxt = &instance->dump_context;

	ret = snprintf(buf,PAGE_SIZE,"%s\n",ctxt->dump_dir);

l_out:
	return ret;
}

S32 ps3_dump_dir_length( const char *buf, size_t count)
{
	S32 i = 0;
	char c;

	while ((size_t)i++ < count) {
		c = buf[i];

		if (isdigit(c))
			continue;

		if (isalpha(c))
			continue;

		switch(c) {
		case '-':
		case '_':
		case '/':
		case '~':
			continue;
			break;
		default:
			goto l_out;
		}
	}

l_out:
	return i;
}

#if 0
ssize_t ps3_dump_dir_store(struct device *cdev, struct device_attribute *attr,
	const char *buf, size_t count)
{
#if 1
    LOG_INFO("set dump log dir is not supported\n");
    return 0;
#else
        S32 ret = 0, length = 0;
        struct ps3_dump_context *ctxt = NULL;
        (void)attr;

        LOG_INFO("dir param %s\n", buf);

	ctxt = dev_to_dump_context (cdev);

	length = ps3_dump_dir_length(buf, count);
	length = min(length, (S32)PS3_DUMP_FILE_DIR_LEN);
	if (length <= 0 || count > PS3_DUMP_FILE_DIR_LEN) {
		ret = -EINVAL;
		LOG_ERROR("Invalid param %s\n", buf);
		goto l_ret;
	}

	ps3_mutex_lock(&ctxt->dump_lock);
	memset(ctxt->dump_dir, 0, sizeof(ctxt->dump_dir));
	(void)strncpy((char*)ctxt->dump_dir, buf, length);
	ps3_mutex_unlock(&ctxt->dump_lock);

	LOG_INFO("dump dir change to %s\n", ctxt->dump_dir);

	ret = count;

l_ret:
	return ret;
#endif
}
#endif

ssize_t ps3_dump_type_show(struct device *cdev,struct device_attribute *attr, char *buf)
{
	struct ps3_dump_context *ctxt;
	struct ps3_instance *instance = ps3_debug_instance_query(cdev,
		attr, buf);
	ssize_t ret = 0;

	if (instance == NULL) {
		goto l_out;
	}

	ctxt = &instance->dump_context;

	ret = snprintf(buf,PAGE_SIZE,"%d \n",ctxt->dump_type);

l_out:
	return ret;
}

ssize_t ps3_dump_type_store(struct device *cdev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	S32 val = 0;
	S32 ret = 0;
	struct ps3_dump_context *ctxt = NULL;
	Bool is_trigger_log = PS3_FALSE;
	(void)attr;

	ctxt = dev_to_dump_context (cdev);

	if (kstrtoint(buf, 0, &val) != 0) {
		ret = -EINVAL;
		goto l_ret;
	}

	if ((val < PS3_DUMP_TYPE_CRASH) || (val > PS3_DUMP_TYPE_BAR_DATA)) {
		LOG_ERROR("host_no[%d],dump state should be %d - %d,"
			"%d is an invalid value\n", PS3_HOST(ctxt->instance),
			PS3_DUMP_TYPE_CRASH, PS3_DUMP_TYPE_BAR_DATA, val);
		ret = -EINVAL;
		goto l_ret;
	}

	ps3_ioc_dump_support_get(ctxt->instance);
	is_trigger_log = ps3_dump_is_trigger_log(ctxt->instance);
	INJECT_START(PS3_ERR_IJ_STORE_NO_TRIGGER_LOG, &is_trigger_log)
	if(!is_trigger_log) {
		LOG_INFO("cannot dump type set!\n");
		ret = -EBUSY;
		goto l_ret;
	}

	if (ps3_dump_type_set(ctxt, val, PS3_DUMP_ENV_CLI) == -PS3_FAILED) {
		ret = -EBUSY;
		goto l_ret;
	}
	return count;
l_ret:
	return ret;
}

void ps3_dma_dump_mapping(struct pci_dev *pdev)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)
#if defined(PS3_SUPPORT_DEBUG) || \
		(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) || \
		(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
	dma_dump_mappings_cb dma_dump_mappings = NULL;
	dma_dump_mappings = (dma_dump_mappings_cb) kallsyms_lookup_name("debug_dma_dump_mappings");
	if (dma_dump_mappings && pdev) {
		printk(KERN_INFO "ps3 dma dump mapping begin\n");
		dma_dump_mappings(&pdev->dev);
		printk(KERN_INFO "ps3 dma dump mapping end\n");
	}
#endif
#endif
	(void)pdev;
}

ssize_t ps3_soc_dead_reset_store(struct device *cdev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	S32 val = 0;
	S32 ret = 0;
	struct ps3_instance *instance = ps3_debug_instance_query(cdev, attr, buf);

	(void)attr;
	(void)count;
	if (kstrtoint(buf, 0, &val) != 0) {
		ret = -EINVAL;
		goto l_ret;
	}
	if (ps3_atomic_read(&instance->state_machine.state)
		!= PS3_INSTANCE_STATE_DEAD) {
		ret = -EPERM;
		goto l_ret;
	}
	if (ps3_need_block_hard_reset_request(instance)) {
		LOG_WARN("hno:%u  can not start hard reset\n",
			PS3_HOST(instance));
	} else {
		ps3_hard_recovery_request_with_retry(instance);
	}

	return count;
l_ret:
	return ret;
}
ssize_t ps3_halt_support_cli_show(struct device *cdev,struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct ps3_instance *instance = ps3_debug_instance_query(cdev,
		attr, buf);

	if (instance == NULL) {
		ret = 0;
		goto l_out;
	}

	ret = snprintf(buf, PAGE_SIZE, "%d\n",
		instance->is_halt_support_cli);

	LOG_DEBUG("get halt_support_cli is %d\n",
		instance->is_halt_support_cli);
l_out:
	return ret;
}

ssize_t ps3_halt_support_cli_store(struct device *cdev,struct device_attribute *attr,
	const char *buf, size_t count)
{
	U8 halt_support_cli = 0;
	Bool is_halt_support_cli;
	ssize_t ret = count;
	struct ps3_instance *instance = ps3_debug_instance_query(cdev,
		attr, buf);

	if (instance == NULL) {

		goto l_out;
	}

	if (kstrtou8(buf, 10, &halt_support_cli)) {
		ret = -EINVAL;
		LOG_ERROR("invalid io halt_support_cli, could not set\n");
		goto l_out;
	}

	is_halt_support_cli = (halt_support_cli == 0)?(0):(1);
	instance->is_halt_support_cli = is_halt_support_cli;

	LOG_WARN("set halt_support_cli is %d\n",
		instance->is_halt_support_cli);

l_out:
	return ret;
}

#if defined(PS3_SUPPORT_DEBUG) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
#else
ssize_t ps3_irq_prk_support_store(struct device *cdev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int is_prk_in_irq= 0;
	ssize_t ret = count;
	struct ps3_instance *instance = ps3_debug_instance_query(cdev,
		attr, buf);

	if (cdev == NULL || buf == NULL) {
		goto l_out;
	}

	if (kstrtoint(buf, 10, &is_prk_in_irq)) {
		LOG_ERROR("invalid is_prk_in_irq, could not set is_prk_in_irq\n");
		ret = -EINVAL;
		goto l_out;
	}

	LOG_WARN("set is_prk_in_irq %d\n", is_prk_in_irq);

	instance->is_irq_prk_support = is_prk_in_irq;
l_out:
	return ret;
}

ssize_t ps3_irq_prk_support_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct ps3_instance *instance = ps3_debug_instance_query(cdev,
		attr, buf);

	if (cdev == NULL || buf == NULL) {
		goto l_out;
	}
	ret = snprintf(buf, PAGE_SIZE, "%d\n", instance->is_irq_prk_support);

	LOG_DEBUG("get log level to %d\n", instance->is_irq_prk_support);
l_out:
	return ret;
}
#endif

ssize_t ps3_qos_switch_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct ps3_instance *instance = ps3_debug_instance_query(cdev,
		attr, buf);

	if (instance == NULL) {
		ret = 0;
		goto l_out;
	}

	ret = snprintf(buf, PAGE_SIZE, "%d\n",
			instance->qos_context.qos_switch);

l_out:
	return ret;
}

ssize_t ps3_qos_switch_store(struct device *cdev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	U8 qos_switch = 0;
	ssize_t ret = count;
	struct ps3_instance *instance = ps3_debug_instance_query(cdev,
		attr, buf);

	if (instance == NULL) {
		ret = count;
		goto l_out;
	}

	if (kstrtou8(buf, 10, &qos_switch)) {
		LOG_ERROR("invalid io trace switch, could not set\n");
		ret = count;
		goto l_out;
	}

    if (instance->qos_context.qos_switch > 0 && qos_switch == 0) {
        instance->qos_context.qos_switch = 0;
        ps3_qos_close(instance);
    }

    if (instance->qos_context.qos_switch == 0 && qos_switch > 0) {
        ps3_qos_open(instance);
        instance->qos_context.qos_switch = qos_switch;
    }

	LOG_WARN("set qos switch is %d\n",
		instance->qos_context.qos_switch);

l_out:
	return ret;
}

#endif
