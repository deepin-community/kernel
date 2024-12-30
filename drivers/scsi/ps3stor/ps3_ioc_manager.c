#ifndef _WINDOWS

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/dmi.h>
#include <linux/limits.h>
#else
#include "ps3_def.h"
#endif

#include "ps3_cmd_statistics.h"
#include "ps3_ioc_manager.h"
#include "ps3_ioc_state.h"
#include "ps3_util.h"
#include "ps3_pci.h"
#include "ps3_htp_def.h"
#include "ps3_mgr_cmd.h"
#include "ps3_err_inject.h"
#include "ps3_inject.h"
#include "ps3_drv_ver.h"

#define PS3_INIT_CMD_WAIT_MAX_TIMEOUT     (180)  
#define PS3_INIT_CMD_WAIT_INTERVAL        (20)   
#define PS3_INIT_CMD_CHECK_FAULT_INTERVAL (1000)
#if (defined PS3_HARDWARE_FPGA && defined PS3_MODEL_V200)
#define PS3_REG_READ_MAX_TRY_COUNT	(60)
#else
#define PS3_REG_READ_MAX_TRY_COUNT	(10)
#endif
#define PS3_REG_SWITCH_QEQUEST_QUEUE_OFFSET (0x10000)
#define PS3_IOC_INIT_PROC_FAIL_RETRY_COUNT (2)

static S32 ps3_ioc_init_cmd_result_poll(struct ps3_instance *instance,
	struct PS3InitReqFrame *init_frame_msg)
{
	U32 state = PS3_FW_STATE_UNDEFINED;
	U32 wait_count = PS3_INIT_CMD_WAIT_MAX_TIMEOUT * 1000 /
		PS3_INIT_CMD_WAIT_INTERVAL;
	U32 count = 0;
	S32 ret   = -PS3_FAILED;

	state = instance->ioc_adpter->ioc_state_get(instance);

	while((count < wait_count) && (state != PS3_FW_STATE_FAULT)
		&& (state != PS3_FW_STATE_CRITICAL)) {
		rmb();
		ps3_msleep(PS3_INIT_CMD_WAIT_INTERVAL);

		INJECT_START(PS3_ERR_IJ_FORCE_RECOVERY_PRE_PCIE_ERR_8, instance)
		if (ps3_pci_err_recovery_get(instance)) {
			LOG_WARN("hno:%u  pci recovery resetting\n", PS3_HOST(instance));
			ret = -PS3_IN_PCIE_ERR;
			goto l_out;
		}

		if (init_frame_msg->respStatus != U32_MAX) {
			break;
		}

		if (!(count % PS3_INIT_CMD_CHECK_FAULT_INTERVAL)) {
			state = instance->ioc_adpter->ioc_state_get(instance)
				& PS3_FW_STATE_MASK;
		}

		count++;
	}

	if (state == PS3_FW_STATE_FAULT ||
		state == PS3_FW_STATE_CRITICAL) {
		LOG_ERROR("hno:%u  init cmd NOK since IOC state fault\n",
			PS3_HOST(instance));
		goto l_out;
	}

	if (count == wait_count) {
		LOG_ERROR("hno:%u  init cmd timeout, state=%#x, respStatus=%#x\n",
			PS3_HOST(instance), state, init_frame_msg->respStatus);
		goto l_out;
	}

	if (init_frame_msg->respStatus != 0) {
		LOG_ERROR("hno:%u  init cmd NOK[%d]\n",
			PS3_HOST(instance), init_frame_msg->respStatus);
		goto l_out;
	}

	ret   = PS3_SUCCESS;
	LOG_INFO("hno:%u  init cmd response successfully\n",
		PS3_HOST(instance));

l_out:
	init_frame_msg->respStatus = U32_MAX;
	return ret;
}

static S32 ps3_ioc_init_cmd_issue(struct ps3_instance *instance,
	struct PS3InitReqFrame *init_frame_msg)
{
	struct ps3_cmd_context *cmd_context = &instance->cmd_context;
	struct PS3InitCmdWord cmd_word;

	memset(&cmd_word, 0, sizeof(struct PS3InitCmdWord));

	cmd_word.lowAddr  =
		cpu_to_le32(lower_32_bits(cmd_context->init_frame_buf_phys));
	cmd_word.highAddr =
		cpu_to_le32(upper_32_bits(cmd_context->init_frame_buf_phys));
	cmd_word.type   = PS3_CMDWORD_TYPE_INIT;
	cmd_word.direct = PS3_CMDWORD_DIRECT_NORMAL;

	instance->ioc_adpter->init_cmd_send(instance, (struct PS3CmdWord *)&cmd_word);

	LOG_INFO("hno:%u  init command: cmd.lowAddr[0x%x], cmd.highAddr[0x%x], "
		"cmd.type[%u]\n", PS3_HOST(instance), cmd_word.lowAddr,
		cmd_word.highAddr, cmd_word.type);
	INJECT_START(PS3_ERR_IJ_FORCE_INIT_FAIL, instance)
	INJECT_START(PS3_ERR_IJ_RECOVERY_F1_A_INIT, instance)
	INJECT_START(PS3_ERR_IJ_FORCE_RECOVERY_INIT_FAIL, instance)

	return ps3_ioc_init_cmd_result_poll(instance, init_frame_msg);
}

static S32 ps3_ioc_init_cmd_alloc(struct ps3_instance *instance)
{
	struct ps3_cmd_context *cmd_context = &instance->cmd_context;
	S32 ret = PS3_SUCCESS;

	cmd_context->init_frame_buf =
		(U8*)ps3_dma_alloc_coherent(instance,
		sizeof(struct PS3InitReqFrame),
		&cmd_context->init_frame_buf_phys);

	if (!cmd_context->init_frame_buf) {
		LOG_ERROR("hno:%u  Failed to alloc init cmd dma buffer\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
	}

	cmd_context->init_filter_table_buff = (U8*)ps3_dma_alloc_coherent(instance,
		PS3_CMD_EXT_BUF_SIZE_MGR,
		&cmd_context->init_filter_table_phy_addr);

	if (!cmd_context->init_filter_table_buff) {
		LOG_ERROR("hno:%u  Failed to alloc init filter table dma buffer\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
	}

	return ret;
}
#ifndef _WINDOWS

static S32 ps3_ioc_sys_info_get(struct ps3_instance *instance)
{
	struct ps3_cmd_context *cmd_context  = &instance->cmd_context;
	struct PS3DrvSysInfo   *drv_sys_info = NULL;
	const char *sys_info = NULL;
	U32 sys_info_len = 0;
	S32 ret = -PS3_FAILED;

	cmd_context->init_frame_sys_info_buf =
		(U8*)ps3_dma_alloc_coherent(instance,
		sizeof(struct PS3DrvSysInfo),
		&cmd_context->init_frame_sys_info_phys);

	if (!cmd_context->init_frame_sys_info_buf) {
		LOG_ERROR("hno:%u  Failed to alloc sysinfo dma buffer\n",
			PS3_HOST(instance));
		goto l_out;
	}

	drv_sys_info =
		(struct PS3DrvSysInfo *)cmd_context->init_frame_sys_info_buf;

	memset(drv_sys_info->systemID, 0, PS3_DRV_SYSTEM_ID_MAX_LEN);

	sys_info = dmi_get_system_info(DMI_PRODUCT_UUID);
	if (!sys_info) {
		LOG_INFO("hno:%u  Failed to get sysinfo\n",
			PS3_HOST(instance));
		drv_sys_info->systemIDLen = 0;
		drv_sys_info->version = 0;
		ret = PS3_SUCCESS;
		goto l_out;
	}

	sys_info_len = strlen(sys_info) > PS3_DRV_SYSTEM_ID_MAX_LEN ?
		PS3_DRV_SYSTEM_ID_MAX_LEN : strlen(sys_info);

	memcpy(drv_sys_info->systemID, sys_info, sys_info_len);
	drv_sys_info->systemIDLen = sys_info_len;
	drv_sys_info->version = 0;
	ret = PS3_SUCCESS;

l_out:
	return ret;
}

#endif

S32 ps3_ioc_init_cmd_context_init(struct ps3_instance *instance)
{
	if (ps3_ioc_init_cmd_alloc(instance) != PS3_SUCCESS) {
		goto l_failed;
	}
#ifndef _WINDOWS
	if (ps3_ioc_sys_info_get(instance) != PS3_SUCCESS) {
		goto l_failed;
	}
#endif
	return PS3_SUCCESS;
l_failed:
	ps3_ioc_init_cmd_context_exit(instance);
	return -PS3_FAILED;
}

void ps3_ioc_init_cmd_context_exit(struct ps3_instance *instance)
{
	struct ps3_cmd_context *cmd_context  = &instance->cmd_context;

	LOG_INFO("entry\n");
#ifndef _WINDOWS
	if (cmd_context->init_frame_sys_info_buf != NULL) {
		LOG_INFO("free init_frame_sys_info_buf = %p\n", cmd_context->init_frame_sys_info_buf);
		ps3_dma_free_coherent(instance,
			sizeof(struct PS3DrvSysInfo),
			cmd_context->init_frame_sys_info_buf,
			cmd_context->init_frame_sys_info_phys);
		cmd_context->init_frame_sys_info_buf = NULL;
	}
#endif

	if (cmd_context->init_filter_table_buff != NULL) {
		LOG_INFO("free init_filter_table_buff = %p\n", cmd_context->init_filter_table_buff);
		ps3_dma_free_coherent(instance,
			PS3_CMD_EXT_BUF_SIZE_MGR,
			cmd_context->init_filter_table_buff,
			cmd_context->init_filter_table_phy_addr);

		cmd_context->init_filter_table_buff = NULL;
		cmd_context->init_filter_table_phy_addr = 0;
	}

	if (cmd_context->init_frame_buf != NULL) {
		LOG_INFO("free init_frame_buf = %p\n", cmd_context->init_frame_buf);
		ps3_dma_free_coherent(instance,
			sizeof(struct PS3InitReqFrame),
			cmd_context->init_frame_buf,
			cmd_context->init_frame_buf_phys);
		cmd_context->init_frame_buf = NULL;
	}
}

S32 ps3_drv_info_buf_alloc(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;

	instance->drv_info_buf =
		(U8*)ps3_dma_alloc_coherent(instance,
		sizeof(struct PS3DrvInfo),
		&instance->drv_info_buf_phys);

	if (instance->drv_info_buf == NULL) {
		LOG_ERROR("hno:%u  Failed to alloc drv info dma buffer\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
	}
	return ret;
}

void ps3_drv_info_buf_free(struct ps3_instance *instance)
{
	if (instance->drv_info_buf != NULL) {
		LOG_INFO("drv_info_buf = %p\n", instance->drv_info_buf);
		ps3_dma_free_coherent(instance, sizeof(struct PS3DrvInfo),
			instance->drv_info_buf,
			instance->drv_info_buf_phys);
		instance->drv_info_buf = NULL;
	}
}

S32 ps3_host_mem_info_buf_alloc(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;

	instance->host_mem_info_buf =
		(U8*)ps3_dma_alloc_coherent(instance,
		(sizeof(struct PS3HostMemInfo) * PS3_HOST_MEM_INFO_NUM),
		&instance->host_mem_info_buf_phys);

	if (instance->host_mem_info_buf == NULL) {
		LOG_ERROR("hno:%u Failed to alloc host mem info dma buffer\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
	}
	return ret;
}

void ps3_host_mem_info_buf_free(struct ps3_instance *instance)
{
	if (instance->host_mem_info_buf != NULL) {
		LOG_INFO("free host_mem_info_buf = %p\n", instance->host_mem_info_buf);
		ps3_dma_free_coherent(instance, (sizeof(struct PS3HostMemInfo) * PS3_HOST_MEM_INFO_NUM),
			instance->host_mem_info_buf,
			instance->host_mem_info_buf_phys);
		instance->host_mem_info_buf = NULL;
	}
}

S32 ps3_hard_reset_to_ready(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	if (instance->peer_instance != NULL) {
		if (instance->recovery_context->parall_hardreset_state == PS3_PARALLEL_HARDRESET_STATE_PENDING) {
			instance->recovery_context->parall_hardreset_state = PS3_PARALLEL_HARDRESET_STATE_CONTINUE;
			while (instance->recovery_context->parall_hardreset_state != PS3_PARALLEL_HARDRESET_STATE_INIT) {
				ps3_msleep(PS3_PARALLEL_HARDRESET_STATE_WAIT_INIT_INTERVAL);
			}
		} else {
			ret = ps3_hard_recovery_request(instance);
			if (ret != PS3_SUCCESS){
				LOG_WARN("hno:%u  hard recovery request failed\n",
					PS3_HOST(instance));
				goto l_out;
			}
		}
		ps3_recovery_cancel_work_sync(instance);
	} else {
		if (!instance->ioc_adpter->ioc_hard_reset) {
			ret = -PS3_FAILED;
			goto l_out;
		}

		ret = instance->ioc_adpter->ioc_hard_reset(instance);
		if (ret == -PS3_FAILED) {
			goto l_out;
		}
		INJECT_START(PS3_ERR_IJ_HARD_WAIT_READY_FAILED_F0, instance);
		if ((ret = ps3_ioc_state_ready_wait(instance)) != PS3_SUCCESS) {
			goto l_out;
		}
	}

l_out:
	if (ret == PS3_SUCCESS) {
		LOG_WARN("hno:%u  ps3 fw state reset to ready successfully\n",
			PS3_HOST(instance));
	} else {
		LOG_ERROR("hno:%u  PS3 fw state reset to ready NOK \n",
			PS3_HOST(instance));
	}

	return ret;
}
S32 ps3_ioc_hard_reset_to_ready(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;

	if (!instance->ioc_adpter->ioc_hard_reset) {
		ret = -PS3_FAILED;
		goto l_out;
	}

	if (instance->recovery_context->heartbeat_recovery != PS3_HEARTBEAT_HARDRESET_RECOVERY) {
		ret = instance->ioc_adpter->ioc_hard_reset(instance);
		if (ret != PS3_SUCCESS) {
			goto l_out;
		}
	}

	INJECT_START(PS3_ERR_IJ_HARD_WAIT_READY_FAILED_F0, instance);
	if ((ret = ps3_ioc_state_transfer_to_ready(instance)) != PS3_SUCCESS) {
		goto l_out;
	}

l_out:
	if (ret == PS3_SUCCESS) {
		LOG_WARN("hno:%u  ps3 fw state reset to ready successfully\n",
			PS3_HOST(instance));
	} else {
		LOG_ERROR("hno:%u  PS3 fw state reset to ready NOK \n",
			PS3_HOST(instance));
	}

	return ret;
}

S32 ps3_ioc_init_to_ready(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	Bool is_unload_valid = PS3_FALSE;

    INJECT_START(PS3_ERR_IJ_FORCE_IOC_RUNNING, instance);
    INJECT_START(PS3_ERR_IJ_FORCE_IOC_RUNNING_RESUME_RECOVERY, instance);
	ret = ps3_ioc_state_transfer_to_ready(instance);
	INJECT_START(PS3_ERR_IJ_FW_STATE_RUNNING, &ret)
	if (ret == PS3_SUCCESS) {
		goto l_out;
	}

	ps3_check_debug0_valid_with_check(instance, &is_unload_valid,
		PS3_CMD_TRIGGER_UNLOAD);
	LOG_INFO("hno:%u  PS3 fw state cannot directly init to ready successfully\n",
		PS3_HOST(instance));
	if (ret != -PS3_NO_RECOVERED) {
		if(ps3_is_need_hard_reset(instance)) {
			ret = ps3_hard_reset_to_ready(instance);
		}
	}

l_out:
	if (ret == PS3_SUCCESS) {
		LOG_INFO("hno:%u  PS3 fw state init to ready successfully\n",
			PS3_HOST(instance));
		if (is_unload_valid) {
			ret = ps3_ioc_notify_unload(instance);
		}
	}else {
		LOG_ERROR("hno:%u  PS3 fw state init to ready NOK\n",
			PS3_HOST(instance));
	}

	return ret;
}

static void ps3_drv_info_prepare(struct ps3_instance *instance)
{
	struct PS3DrvInfo *drv_info = NULL;
	drv_info = (struct PS3DrvInfo *)instance->drv_info_buf;
	memset(drv_info, 0, sizeof(struct PS3DrvInfo));

	snprintf(drv_info->drvName, PS3_DRV_NAME_MAX_LEN, PS3_DRV_AUTHOR);
	snprintf(drv_info->drvVersion, PS3_DRV_VERSION_MAX_LEN, PS3_DRV_VERSION);
	drv_info->domain_support = PS3_TRUE;
	drv_info->domain = (U32)ps3_get_pci_domain(instance->pdev);
	drv_info->bus = ps3_get_pci_bus(instance->pdev);
	drv_info->dev = PCI_SLOT(instance->pdev->devfn);
	drv_info->func = PCI_FUNC(instance->pdev->devfn);
	drv_info->compatVer = PS3_COMPAT_VER_1;
	LOG_DEBUG("hno:%u  driver information dump:\n"
		"\t drvName = %s, drvVersion = %s\n"
		"\t domain = %x bus = %llx, dev = %x, func = %x\n",
		PS3_HOST(instance), drv_info->drvName, drv_info->drvVersion,
		drv_info->domain, drv_info->bus, drv_info->dev, drv_info->func);
}

static inline bool ps3_pgdat_is_empty(pg_data_t *pgdat)
{
	return !pgdat->node_start_pfn && !pgdat->node_spanned_pages;
}

struct pglist_data *first_online_pgdat(void)
{
	return NODE_DATA(first_online_node);
}

struct pglist_data *next_online_pgdat(struct pglist_data *pgdat)
{
	int nid = next_online_node(pgdat->node_id);
	if (nid == MAX_NUMNODES) {
		return NULL;
	}
	return NODE_DATA(nid);
}

static Bool ps3_get_numa_mem_addr(struct ps3_instance *instance)
{
	Bool ret = PS3_FALSE;
	S32 node = -1;
	struct pglist_data *pgdata = NULL;
	node = dev_to_node(&instance->pdev->dev);
	if (node < 0) {
		goto l_out;
	}
	pgdata = NODE_DATA(node);
	if (pgdata == NULL || ps3_pgdat_is_empty(pgdata)) {
		goto l_out;
	}

	instance->start_pfn = pgdata->node_start_pfn << PAGE_SHIFT;
	instance->end_pfn = ((pgdat_end_pfn(pgdata) << PAGE_SHIFT) - 1);

	LOG_INFO("host_no:%u numa:%d, addr range:[0x%llx - 0x%llx], so addr range:[0x%llx - 0x%llx]\n",
		PS3_HOST(instance), node, instance->start_pfn, instance->end_pfn,
		instance->so_start_addr, instance->so_end_addr);
	ret = PS3_TRUE;
l_out:
	return ret;
}

static void ps3_get_all_numa_mem_addr(struct ps3_instance *instance)
{
	struct pglist_data *pgdata = NULL;
	S32 node = -1;
	Bool is_in_range = PS3_FALSE;
	U64 start_pfn = 0;
	U64 end_pfn = 0;
	if (ps3_get_numa_mem_addr(instance)) {
		goto l_out;
	}
	for_each_online_pgdat(pgdata) {
		if (ps3_pgdat_is_empty(pgdata)) {
			continue;
		}
		start_pfn = pgdata->node_start_pfn << PAGE_SHIFT;
		end_pfn = ((pgdat_end_pfn(pgdata) << PAGE_SHIFT) - 1);
		INJECT_START(PS3_ERR_IJ_MOD_SO_ADDR, instance);
		if (PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(instance->dma_addr_bit_pos, instance->so_start_addr) >= start_pfn &&
			PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(instance->dma_addr_bit_pos, instance->so_end_addr) <= end_pfn) {
			node = pgdata->node_id;
			is_in_range = PS3_TRUE;
			break;
		}
	}
	if (is_in_range) {
		instance->start_pfn = start_pfn;
		instance->end_pfn = end_pfn;
	} else {
		instance->start_pfn = 0;
		instance->end_pfn = 0;
	}
	LOG_INFO("host_no:%u numa:%d, addr range:[0x%llx - 0x%llx], so addr range:[0x%llx - 0x%llx]\n",
		PS3_HOST(instance), node, instance->start_pfn, instance->end_pfn,
		instance->so_start_addr, instance->so_end_addr);
l_out:
	return;
}

static void ps3_host_mem_info_prepare(struct ps3_instance *instance)
{
	struct PS3HostMemInfo *host_mem_info = NULL;
	host_mem_info = (struct PS3HostMemInfo *)instance->host_mem_info_buf;
	memset(host_mem_info, 0, sizeof(struct PS3HostMemInfo));

	host_mem_info->startAddr = instance->start_pfn;
	host_mem_info->endAddr = instance->end_pfn;
	host_mem_info->type = PS3_MEM_TYPE_SO;
	LOG_DEBUG("hno:%u host mem info dump:\n"
		"\t startAddr = 0x%llx, endAddr = 0x%llx, type = %u\n",
		PS3_HOST(instance), host_mem_info->startAddr, host_mem_info->endAddr,
		host_mem_info->type);
}

static void ps3_ioc_init_cmd_prepare(struct ps3_instance *instance)
{

	struct ps3_cmd_context *cmd_context    = &instance->cmd_context;
	struct ps3_irq_context *irq_context    = &instance->irq_context;
	struct ps3_cmd_attr_context *cmd_attr  = &instance->cmd_attr;
	struct ps3_dump_context *dump_context  = &instance->dump_context;
	struct PS3InitReqFrame* init_frame_msg = NULL;

	init_frame_msg = (struct PS3InitReqFrame *)cmd_context->init_frame_buf;

	memset(&init_frame_msg->reqHead, 0, sizeof(PS3ReqFrameHead_s));
	init_frame_msg->reqHead.cmdType = PS3_CMD_INIT_IOC;
	init_frame_msg->reqHead.timeout = PS3_INIT_CMD_WAIT_MAX_TIMEOUT;

	init_frame_msg->ver	= 0;
	init_frame_msg->length     = sizeof(struct PS3InitReqFrame);
	init_frame_msg->operater   = PS3_CMD_OPERATOR_TYPE_HOST;
	init_frame_msg->pageSize   = (U8)cmd_attr->nvme_page_size;
	init_frame_msg->msixVector = (U16)irq_context->valid_msix_vector_count;
	init_frame_msg->pciIrqType = (U16)irq_context->pci_irq_type;
	init_frame_msg->osType     = PS3_LINUX_FRAME;

	init_frame_msg->timeStamp  = cpu_to_le64(ps3_1970_now_ms_get());
	init_frame_msg->reqFrameBufBaseAddr =
		cpu_to_le64(cmd_context->req_frame_buf_phys);

	init_frame_msg->replyFifoDescBaseAddr =
		cpu_to_le64(irq_context->reply_fifo_desc_buf_phys);

	init_frame_msg->respFrameBaseAddr =
		cpu_to_le64(cmd_context->response_frame_buf_phys);

	init_frame_msg->reqFrameMaxNum = (U16)cmd_context->max_cmd_count;
	init_frame_msg->respFrameMaxNum = (U16)cmd_context->max_cmd_count;
	init_frame_msg->bufSizePerRespFrame = PS3_RESP_FRAME_BUFFER_SIZE;


	if(ps3_hil_mode_query() > HIL_MODEL_SW_ASSIST)
	{
		init_frame_msg->hilMode = instance->hilMode;
	}else{
		init_frame_msg->hilMode = (U8)ps3_hil_mode_query();
	}

#ifndef _WINDOWS
	init_frame_msg->systemInfoBufAddr =
		cmd_context->init_frame_sys_info_phys;
#endif

	init_frame_msg->dumpDmaBufAddr = cpu_to_le64(dump_context->dump_dma_addr);
	init_frame_msg->dumpDmaBufLen = cpu_to_le64(PS3_DUMP_DMA_BUF_SIZE);
	init_frame_msg->dumpIsrSN = cpu_to_le32(irq_context->dump_isrSN);
#ifndef _WINDOWS
	init_frame_msg->debugMemArrayNum =
		cpu_to_le32(instance->debug_context.debug_mem_array_num);
	init_frame_msg->debugMemArrayAddr =
		(instance->debug_context.debug_mem_array_num != 0) ?
		cpu_to_le64(instance->debug_context.debug_mem_buf_phy) : 0;
#endif
	memset(cmd_context->init_filter_table_buff, 0, PS3_CMD_EXT_BUF_SIZE_MGR);
	if (instance->ioc_adpter->event_filter_table_get != NULL) {
		init_frame_msg->filterTableAddr = cpu_to_le64(cmd_context->init_filter_table_phy_addr);
		init_frame_msg->filterTableLen = cpu_to_le32(PS3_CMD_EXT_BUF_SIZE_MGR);
		instance->ioc_adpter->event_filter_table_get(cmd_context->init_filter_table_buff);
	}
	ps3_drv_info_prepare(instance);
	init_frame_msg->drvInfoBufAddr = cpu_to_le64(instance->drv_info_buf_phys);
	init_frame_msg->drvInfoBufLen = cpu_to_le16(sizeof(struct PS3DrvInfo));
	ps3_get_all_numa_mem_addr(instance);
	if (instance->end_pfn != 0) {
		ps3_host_mem_info_prepare(instance);
		init_frame_msg->hostMemInfoBaseAddr = cpu_to_le64(instance->host_mem_info_buf_phys);
		init_frame_msg->hostMemInfoNum = cpu_to_le32(PS3_HOST_MEM_INFO_NUM);
	} else {
		init_frame_msg->hostMemInfoBaseAddr = 0;
		init_frame_msg->hostMemInfoNum = 0;
	}
	LOG_DEBUG("hno:%u  init frame information dump:\n"
		"\t initFramePhysicAddr = 0x%llx, initFrameLen = %d, version = %d\n"
		"\t pciIrqType = %d, msixVectorCount = %d, reqHead.cmdType = %d\n"
		"\t reqFrameBufBase = 0x%llx, reqFrameMaxNum = %d\n"
		"\t responseFrameBase = 0x%llx, responseFrameMaxNum = %d, responseFrameSize = %d\n"
		"\t replyFifoDescBase = 0x%llx, hostSystemInfoBuf = %llx\n"
		"\t filterTableAddr = 0x%llx, filterTableLen = %d\n"
		"\t drvInfoBufAddr = 0x%llx, drvInfoBufLen = %d\n"
		"\t hostMemInfoBaseAddr = 0x%llx, hostMemInfoNum = %d, function = %d\n",
		PS3_HOST(instance), cpu_to_le64(cmd_context->init_frame_buf_phys),
		init_frame_msg->length, init_frame_msg->ver,
		init_frame_msg->pciIrqType, init_frame_msg->msixVector, init_frame_msg->reqHead.cmdType,
		init_frame_msg->reqFrameBufBaseAddr, init_frame_msg->reqFrameMaxNum,
		init_frame_msg->respFrameBaseAddr, init_frame_msg->reqFrameMaxNum, init_frame_msg->bufSizePerRespFrame,
		init_frame_msg->replyFifoDescBaseAddr, init_frame_msg->systemInfoBufAddr,
		init_frame_msg->filterTableAddr, init_frame_msg->filterTableLen,
		init_frame_msg->drvInfoBufAddr, init_frame_msg->drvInfoBufLen,
		init_frame_msg->hostMemInfoBaseAddr, init_frame_msg->hostMemInfoNum, ps3_get_pci_function(instance->pdev));

	return;
}

static S32 ps3_ioc_init_cmd_proc(struct ps3_instance *instance)
{
	struct PS3MgrEvent *event_req_info = &instance->event_req_info;
	struct ps3_cmd_context *cmd_context    = &instance->cmd_context;
	struct PS3InitReqFrame *init_frame_msg = NULL;

	ps3_ioc_init_cmd_prepare(instance);

	init_frame_msg = (struct PS3InitReqFrame *)cmd_context->init_frame_buf;
	init_frame_msg->eventTypeMap = event_req_info->eventTypeMap;

	init_frame_msg->respStatus = U32_MAX;

	ps3_all_reply_fifo_init(instance);
	LOG_INFO("hno:%u  start to send init cmd![init:0x%llx, respstats:0x%llx]\n", PS3_HOST(instance),
        cmd_context->init_frame_buf_phys,
        (U64)cmd_context->init_frame_buf_phys + offsetof(struct PS3InitReqFrame, respStatus));

	return ps3_ioc_init_cmd_issue(instance, init_frame_msg);
}
S32 ps3_ioc_init_proc(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	S32 state = PS3_FW_STATE_UNDEFINED;
	S32 retry = 0;

	ps3_ioc_can_hardreset_set(instance, PS3_IOC_CANNOT_HARDRESET);
	while (retry < PS3_IOC_INIT_PROC_FAIL_RETRY_COUNT) {
		ret = ps3_ioc_init_cmd_proc(instance);
		INJECT_START(PS3_ERR_IJ_INIT_CMD_PROC_FAIL, &ret)
		if (ret != PS3_SUCCESS) {
			goto l_clean;
		}
		INJECT_START(PS3_ERR_IJ_FORCE_INIT_RUNNING_FAIL, instance)

		INJECT_START(PS3_ERR_IJ_RECOVERY_F1_RUNNING, instance)
		ret = ps3_ioc_state_transfer_wait_to_running(instance);
		INJECT_START(PS3_ERR_IJ_IOC_STATE_WAIT_TO_RUNNING_FAIL, &ret)
		if (ret != PS3_SUCCESS) {
			if (ret == -PS3_IN_PCIE_ERR) {
				goto l_out;
			}
			goto l_clean;
		}

		goto l_out;
	l_clean:
		state = instance->ioc_adpter->ioc_state_get(instance);
		INJECT_START(PS3_ERR_IJ_IOC_STATE_WAIT_TO_RUNNING_FAIL, &ret)
		INJECT_START(PS3_ERR_IJ_IOC_INIT_PROC_STATE_RUNNING, &state)
		INJECT_START(PS3_ERR_IJ_INIT_PROC_FAIL_NOUNLOAD, &state)
		if(state == PS3_FW_STATE_WAIT ||
			state == PS3_FW_STATE_RUNNING){
			if (ps3_soc_unload(instance, PS3_TRUE, PS3_UNLOAD_SUB_TYPE_REMOVE, PS3_SUSPEND_TYPE_NONE) == PS3_SUCCESS) {
				LOG_INFO("device[%d] unload success,exit init proc.\n", instance->pdev->dev.id);
				retry++;
				continue;
			}
		}
		ret = ps3_init_fail_hard_reset_with_doorbell(instance);
		if (ret != PS3_SUCCESS) {
			LOG_ERROR("device[%d] hard reset NOK,exit init proc.\n", instance->pdev->dev.id);
			goto l_out;
		}
		retry++;
	}
	if (retry == PS3_IOC_INIT_PROC_FAIL_RETRY_COUNT) {
		ret = -PS3_FAILED;
	}
	LOG_INFO("device[%d] doorbell success,exit init proc [%d].\n", instance->pdev->dev.id, ret);

l_out:
	return ret;
}

void ps3_ioc_legacy_irqs_enable(struct ps3_instance *instance)
{
	LOG_INFO("hno:%u  Enable legacy!\n", PS3_HOST(instance));
#ifndef  _WINDOWS
	pci_intx(instance->pdev, 1);
#else
	ps3_pci_intx(instance, 1);
#endif
	return;
}

void ps3_ioc_legacy_irqs_disable(struct ps3_instance *instance)
{
	LOG_INFO("hno:%u  Disable legacy!\n", PS3_HOST(instance));
#ifndef  _WINDOWS
	pci_intx(instance->pdev, 0);
#else
	ps3_pci_intx(instance, 0);
#endif
	return;
}

void ps3_ioc_msi_enable(struct ps3_instance *instance)
{
	U32 pos = 0;
	U16 control = 0;

	pos = ps3_pci_find_capability(instance, PCI_CAP_ID_MSI);
	if (pos == 0) {
		LOG_INFO("hno:%u  Find PCI_CAP_ID_MSI failed!\n",
			PS3_HOST(instance));
		return;
	}

	ps3_pci_read_config_word(instance, pos + PCI_MSI_FLAGS, &control);
	if (!(control & PCI_MSI_FLAGS_ENABLE)) {
		LOG_INFO("hno:%u  Enable Msi!\n", PS3_HOST(instance));
		ps3_pci_write_config_word(instance, pos + PCI_MSI_FLAGS,
		control | PCI_MSI_FLAGS_ENABLE);
	}

	return;
}

void ps3_ioc_msi_disable(struct ps3_instance *instance)
{
	U32 pos = 0;
	U16 control = 0;

	pos = ps3_pci_find_capability(instance, PCI_CAP_ID_MSI);
	if (pos == 0) {
		LOG_ERROR("hno:%u  Find PCI_CAP_ID_MSI failed!\n",
			PS3_HOST(instance));
		return;
	}

	ps3_pci_read_config_word(instance, pos + PCI_MSI_FLAGS, &control);
	if (control & PCI_MSI_FLAGS_ENABLE) {
		LOG_INFO("hno:%u  Disable Msi!\n", PS3_HOST(instance));
		ps3_pci_write_config_word(instance, pos + PCI_MSI_FLAGS,
		control & ~PCI_MSI_FLAGS_ENABLE);
	}

	return;

}

void ps3_ioc_msix_enable(struct ps3_instance *instance)
{
	U32 pos = 0;
	U16 control = 0;

	pos = ps3_pci_find_capability(instance, PCI_CAP_ID_MSIX);
	if (pos == 0) {
		LOG_ERROR("hno:%u  Find PCI_CAP_ID_MSIX failed!\n",
			PS3_HOST(instance));
		return;
	}

	ps3_pci_read_config_word(instance, pos + PCI_MSIX_FLAGS, &control);
	if (!(control & PCI_MSIX_FLAGS_ENABLE)) {
		LOG_INFO("hno:%u  Enable Msix!\n", PS3_HOST(instance));
		ps3_pci_write_config_word(instance, pos + PCI_MSIX_FLAGS,
		control | PCI_MSIX_FLAGS_ENABLE);
	}

	return;
}

void ps3_ioc_msix_disable(struct ps3_instance *instance)
{
	U32 pos = 0;
	U16 control = 0;

	pos = ps3_pci_find_capability(instance, PCI_CAP_ID_MSIX);
	if (pos == 0) {
		LOG_ERROR("hno:%u  Find PCI_CAP_ID_MSIX failed!\n",
			PS3_HOST(instance));
		return;
	}

	ps3_pci_read_config_word(instance, pos + PCI_MSIX_FLAGS, &control);
	if (control & PCI_MSIX_FLAGS_ENABLE) {
		LOG_INFO("hno:%u  disable Msix!\n", PS3_HOST(instance));
		ps3_pci_write_config_word(instance, pos + PCI_MSIX_FLAGS,
		control & ~PCI_MSIX_FLAGS_ENABLE);
	}

	return;
}

Bool ps3_ioc_is_legacy_irq_existed(struct ps3_instance *instance)
{
	U16 status = 0;
	Bool is_legacy_irq_existed = PS3_FALSE;

	ps3_pci_read_config_word(instance, PCI_COMMAND, &status);
	if (status & PCI_COMMAND_INTX_DISABLE) {
		LOG_INFO("hno:%u  Legacy irq is disabled!\n",
			PS3_HOST(instance));
		goto l_out;
	}

	ps3_pci_read_config_word(instance, PCI_STATUS, &status);
	if (status & PCI_STATUS_INTERRUPT) {
		LOG_INFO("hno:%u  Legacy irq is existed!\n",
			PS3_HOST(instance));
		is_legacy_irq_existed = PS3_TRUE;
	}

l_out:
	return is_legacy_irq_existed;
}

void ps3_ioc_cmd_send(struct ps3_instance *instance,
	struct PS3CmdWord *cmd_word)
{
	union PS3DefaultCmdWord *cmd_word_default = NULL;

	PS3_CMD_WORD_STAT_INC(instance, cmd_word);

	cmd_word_default = (union PS3DefaultCmdWord *) cmd_word;
#ifdef PS3_SUPPORT_INJECT
	if(ps3_add_cmd_filter(instance, cmd_word)) {
		goto l_out;
	}
#endif

	PS3_IOC_REG_WRITE(instance, cmd_fifo.request_fifo, ps3RequestQueue, cmd_word_default->words);
#ifdef PS3_SUPPORT_INJECT
l_out:
	return;
#endif
}

void ps3_ioc_scsi_cmd_send(struct ps3_instance *instance,
	struct PS3CmdWord *cmd_word)
{
	union PS3DefaultCmdWord *cmd_word_default = NULL;

	PS3_CMD_WORD_STAT_INC(instance, cmd_word);
	cmd_word_default = (union PS3DefaultCmdWord *)cmd_word;

	if(instance->is_pci_reset == PS3_FALSE) {
		ps3_ioc_reg_write(instance, cmd_word_default->words,
			 &instance->reg_set->cmd_fifo.request_fifo.ps3RequestQueue);
	} else {
		LOG_FILE_ERROR("hno:%u  register %p,write blocked by pci err\n",
			PS3_HOST(instance),
			&instance->reg_set->cmd_fifo.request_fifo.ps3RequestQueue);
	}
}

void ps3_switch_normal_cmd_send(struct ps3_instance *instance,
	struct PS3CmdWord *cmd_word)
{
	union PS3CmdWordU32 cmd_word_u32;
	union PS3DefaultCmdWord cmd_word_default;
	struct ps3_cmd *cmd = NULL;

	cmd = ps3_cmd_find(instance, cmd_word->cmdFrameID);

	PS3_CMD_WORD_STAT_INC(instance, cmd_word);

	cmd_word_u32.cmdWord.cmdFrameID = cmd_word->cmdFrameID;
	cmd_word_u32.cmdWord.type = cmd_word->type;
	cmd_word_u32.cmdWord.isrSN = cmd_word->isrSN;
	cmd_word_u32.cmdWord.noReplyWord = cmd->req_frame->frontendReq.reqHead.noReplyWord;

	cmd_word_default.words = 0;
	cmd_word_default.u.low = cmd_word_u32.val;
	PS3_IOC_REG_WRITE(instance, reg_f.Excl_reg, reserved0, cmd_word_default.words);
}

void ps3_switch_init_cmd_send(struct ps3_instance *instance,
	struct PS3CmdWord *cmd_word)
{
	union PS3DefaultCmdWord *cmd_word_default = NULL;
	PS3_CMD_WORD_STAT_INC(instance, cmd_word);

	cmd_word_default = (union PS3DefaultCmdWord *) cmd_word;
	ps3_atomic_inc(&(instance)->reg_op_count);
	mb();
	if((instance)->is_hard_reset == PS3_FALSE && (instance)->is_pci_reset == PS3_FALSE) {
		PS3_REG_SESSION_ADDR_WRITE(instance, cmd_word_default->words,
			&instance->reg_set->reg_f.Excl_reg.ps3SessioncmdAddr);
	} else {
		LOG_ERROR("hno:%u   register %p,write blocked by hardreset(%d) or pci recovery(%d)\n",
			PS3_HOST(instance), &instance->reg_set->reg_f.Excl_reg.ps3SessioncmdAddr,
			(instance)->is_hard_reset, instance->is_pci_reset);
	}
	ps3_atomic_dec(&(instance)->reg_op_count);

	LOG_DEBUG("hno:%u  init command: cmd.words = 0x%llx, "
		"session_reg_offset = %#lx\n", PS3_HOST(instance),
		cmd_word_default->words,
		offsetof(HilReg0Ps3RegisterF_s, ps3SessioncmdAddr));
}
#ifndef _WINDOWS
U64 ps3_switch_ioc_reg_read(struct ps3_instance *instance, void __iomem *reg)
{
	U64 value = 0;
	U64 addr = (U64)reg;
	value = (((U64)readl((void*)(addr + 0x4UL)) << 32) | (U64)readl(reg));

	if (&instance->reg_set->cmd_fifo.request_fifo.ps3RequestQueue == reg) {
		reg = (void __iomem *)((U64)reg - PS3_REG_SWITCH_QEQUEST_QUEUE_OFFSET);
	}
	ps3_reg_dump(instance, reg, value, PS3_TRUE);
	return value;
}
#endif
inline void ps3_ioc_reg_write(struct ps3_instance *instance,
	U64 val, void __iomem *reg)
{
	ps3_reg_dump(instance, reg, val, PS3_FALSE);
	if (instance->ioc_adpter->reg_write) {
		instance->ioc_adpter->reg_write(instance, val, reg);
	} else {
		LOG_FILE_ERROR("hno:%u  no register write\n", PS3_HOST(instance));
	}
}
void ps3_ioc_hardreset_reg_write(struct ps3_instance *instance,
	U64 val, void __iomem *reg, Bool is_warn_prk)
{
	ps3_reg_dump(instance, reg, val, PS3_FALSE);
	if (instance->ioc_adpter->reg_write) {
		if (is_warn_prk) {
		LOG_WARN("hno:%u   register write reset,val:0x%llx,reg=%p\n",
			PS3_HOST(instance),val,reg);
		}
		instance->ioc_adpter->reg_write(instance, val, reg);
	} else {
		LOG_ERROR("hno:%u  no register write\n", PS3_HOST(instance));
	}
}

inline U64 ps3_ioc_reg_read(struct ps3_instance *instance, void __iomem *reg)
{
	U64 value = 0;
	if (instance->ioc_adpter->reg_read) {
		value = instance->ioc_adpter->reg_read(instance, reg);
	} else {
		LOG_ERROR("hno:%u  no register read\n", PS3_HOST(instance));
	}

	ps3_reg_dump(instance, reg, value, PS3_TRUE);
	return value;
}
U64 ps3_ioc_hardreset_reg_read(struct ps3_instance *instance, void __iomem *reg)
{
	U64 value = 0;
	if (instance->ioc_adpter->reg_read) {
		value = instance->ioc_adpter->reg_read(instance, reg);
	} else {
		LOG_ERROR("hno:%u  no register read\n", PS3_HOST(instance));
	}


	return value;
}
inline U64 ps3_ioc_reg_read_with_check(struct ps3_instance *instance,
	void __iomem *reg)
{
	U8 try_count = 0;
	U64 reg_value = ps3_ioc_reg_read(instance, reg);

	while (reg_value == U64_MAX &&
			try_count != PS3_REG_READ_MAX_TRY_COUNT) {
		ps3_msleep(PS3_REG_READ_INTERVAL_MS);
		try_count++;
		reg_value = ps3_ioc_reg_read(instance, reg);
	}

	return reg_value;
}

static U64 ps3_ioc_reg_safe_read(struct ps3_instance *instance,
	void __iomem *reg)
{
	U32 fw_cur_state = PS3_FW_STATE_UNDEFINED;
	U32 count = 0;
	U32 retry_cnt = 0;
	U64 tmp_value = U64_MAX;
	U64 value = U64_MAX;
	Bool is_first = PS3_TRUE;
	for (; retry_cnt < PS3_REG_READ_SAFE_RETRY_NUM; retry_cnt++) {
		tmp_value = ps3_ioc_reg_read_with_check(instance, reg);
		INJECT_START(PS3_ERR_IJ_REG_READ_ERR, &tmp_value);
		if (tmp_value == U64_MAX) {
			value = U64_MAX;
			goto l_out;
		}
		fw_cur_state = instance->ioc_adpter->ioc_state_get(instance);
		INJECT_START(PS3_ERR_IJ_IOC_TO_READY_FAILED1, &fw_cur_state);
		INJECT_START(PS3_ERR_IJ_IOC_TO_CRITICAL, &fw_cur_state);
		if (!ps3_ioc_state_valid_check(fw_cur_state)) {
			for (; count < PS3_REG_READ_RETRY_NUM; count++) {
				fw_cur_state = instance->ioc_adpter->ioc_state_get(instance);
				INJECT_START(PS3_ERR_IJ_IOC_TO_READY_FAILED1, &fw_cur_state);
				INJECT_START(PS3_ERR_IJ_IOC_TO_CRITICAL, &fw_cur_state);
				if (ps3_ioc_state_valid_check(fw_cur_state)) {
					value = ps3_ioc_reg_read_with_check(instance, reg);
					goto l_out;
				}
				ps3_msleep(PS3_LOOP_TIME_INTERVAL_20MS);
			}
			value = U64_MAX;
			LOG_WARN_LIM("hno:%u wait ioc to vaild state NOK\n", PS3_HOST(instance));
			goto l_out;
		}
		if (is_first) {
			value = tmp_value;
			is_first = PS3_FALSE;
			continue;
		}
		INJECT_START(PS3_ERR_IJ_REG_READ_ZERO, &tmp_value);
		if (value != tmp_value) {
			LOG_WARN_LIM("hno:%u reg value not equal old-new[%llu, %llu]\n",
				PS3_HOST(instance), value, tmp_value);
			value = U64_MAX;
			goto l_out;
		}
		ps3_msleep(PS3_LOOP_TIME_INTERVAL_50MS);
	}
l_out:
	return value;
}

U64 ps3_ioc_reg_retry_safe_read(struct ps3_instance *instance, void __iomem *reg)
{
	U32 retry_cnt = PS3_REG_READ_SAFE_RETRY_NUM;
	U64 reg_value = 0;
	while (retry_cnt--) {
		reg_value = ps3_ioc_reg_safe_read(instance, reg);
		if (reg_value != U64_MAX) {
			break;
		}
		ps3_msleep(PS3_LOOP_TIME_INTERVAL_20MS);
	}
	return reg_value;
}

Bool ps3_feature_support_reg_get(struct ps3_instance *instance)
{
	Bool ret = PS3_FALSE;
	HilReg0Ps3RegisterFPs3FeatureSupport_u *ps3_feature_support = NULL;
	U64 value = 0;

	PS3_IOC_REG_READ_SAFE_WITH_RETRY(instance, reg_f.Excl_reg, ps3FeatureSupport, value);
	INJECT_START(PS3_ERR_IJ_FEATURE_REG_READ_ERR, &value);
	if (value == U64_MAX) {
		LOG_ERROR("hno:%u read reg ps3FeatureSupport NOK!\n",
			PS3_HOST(instance));
		goto l_out;
	}
	ps3_feature_support = (HilReg0Ps3RegisterFPs3FeatureSupport_u *)&value;
	instance->is_ioc_halt_support = (ps3_feature_support->reg.fwHaltSupport == 1);
	instance->is_shallow_soft_recovery_support = (ps3_feature_support->reg.shallowSoftRecoverySupport == 1);
	instance->is_deep_soft_recovery_support = (ps3_feature_support->reg.deepSoftRecoverySupport == 1);
	instance->is_hard_recovery_support = (ps3_feature_support->reg.hardRecoverySupport == 1);
	instance->cmd_context.sgl_mode_support = (ps3_feature_support->reg.sglModeSupport == 1);
	ret = PS3_TRUE;
l_out:
	return ret;
}

