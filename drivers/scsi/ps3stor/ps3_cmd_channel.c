
#ifndef  _WINDOWS
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/percpu-defs.h>
#include "ps3_trace_id_alloc.h"
#endif

#include "ps3_cmd_channel.h"
#include "ps3_instance_manager.h"
#include "ps3_inner_data.h"
#include "ps3_irq.h"
#include "ps3_cmd_complete.h"
#include "ps3_ioc_manager.h"
#include "ps3_scsih_cmd_parse.h"
#include "ps3_htp.h"
#include "ps3_util.h"
#include "ps3_ioctl.h"
#include "ps3_cmd_statistics.h"
#include "ps3_r1x_write_lock.h"
#include "ps3_module_para.h"
#include "ps3_scsih.h"
#include "ps3_ioc_state.h"

static S32 ps3_req_frame_alloc(struct ps3_instance *instance);
static void ps3_req_frame_free(struct ps3_instance *instance);
static S32 ps3_cmd_resp_frame_alloc(struct ps3_instance *instance);
static void ps3_cmd_resp_frame_free(struct ps3_instance *instance);
static S32 ps3_cmd_buf_alloc(struct ps3_instance *instance);
static void ps3_cmd_buf_free(struct ps3_instance *instance);
static S32 ps3_cmd_ext_buf_alloc(struct ps3_instance *instance);
static void ps3_cmd_ext_buf_free(struct ps3_instance *instance);
static S32 ps3_cmd_r1xlock_buff_alloc(struct ps3_instance *instance);
static void ps3_cmd_r1xlock_buff_free(struct ps3_instance *instance);
static S32 ps3_cmd_init(struct ps3_instance *instance);
static void ps3_cmd_content_init(struct ps3_cmd *cmd);
static inline Bool is_mgr_cmd(struct ps3_instance *instance, U32 index);
static inline Bool is_task_cmd(struct ps3_instance *instance, U32 index);
static void cmd_pool_free(ps3_list_head *pool_list,
	ps3_spinlock *pool_lock, struct ps3_cmd *cmd);
static struct ps3_cmd *cmd_pool_alloc(ps3_list_head *pool_list,
	ps3_spinlock *pool_lock);

#define PS3_RESP_FRAME_LENGH (PS3_SENSE_BUFFER_SIZE + 32)

static inline Bool is_mgr_cmd(struct ps3_instance *instance, U32 index)
{
	index -= instance->cmd_context.max_scsi_cmd_count;
	return index < instance->max_mgr_cmd_count ? PS3_DRV_TRUE : PS3_DRV_FALSE;
}

static inline Bool is_task_cmd(struct ps3_instance *instance, U32 index)
{
	index -= (instance->cmd_context.max_scsi_cmd_count + instance->max_mgr_cmd_count);
	return index < instance->max_task_cmd_count ? PS3_DRV_TRUE : PS3_DRV_FALSE;
}

static inline Bool is_r1x_peer_cmd(struct ps3_instance *instance, U32 index)
{
	struct ps3_cmd_context *cmd_ctx = &instance->cmd_context;
	return ((cmd_ctx->max_scsi_cmd_count - cmd_ctx->max_r1x_cmd_count) <= index &&
		index < cmd_ctx->max_scsi_cmd_count) ?
		PS3_DRV_TRUE : PS3_DRV_FALSE;
}

struct ps3_cmd *ps3_r1x_peer_cmd_alloc(struct ps3_instance *instance, U32 index)
{
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd_context *context = &instance->cmd_context;
	U32 offset = context->max_scsi_cmd_count - context->max_r1x_cmd_count;
	if (instance->r1x_mode == PS3_R1X_MODE_PERF) {
		cmd = context->cmd_buf[index + offset];
		cmd->cmd_state.state = PS3_CMD_STATE_PROCESS;
		ps3_trace_id_alloc(&cmd->trace_id);
		init_completion(&cmd->sync_done);
	} else {
		cmd = cmd_pool_alloc(&context->r1x_scsi_cmd_pool, &context->r1x_scsi_pool_lock);
		if (cmd != NULL && cmd->is_aborting == 1) {
			cmd->cmd_state.state = PS3_CMD_STATE_INIT;
			cmd->trace_id = 0;
			INJECT_START(PS3_ERR_IJ_SET_CMD_TID_FLAG, cmd);
			cmd_pool_free(&context->r1x_scsi_cmd_pool, &context->r1x_scsi_pool_lock, cmd);
			cmd = NULL;
		}
		INJECT_START(PS3_ERR_IJ_SET_CMD_TID_FLAG, cmd);
	}
	return cmd;
}

struct ps3_cmd *ps3_mgr_cmd_alloc(struct ps3_instance *instance)
{
	struct ps3_cmd_context *context = &instance->cmd_context;
	return cmd_pool_alloc(&context->mgr_cmd_pool, &context->mgr_pool_lock);
}

S32 ps3_mgr_cmd_free(struct ps3_instance *instance, struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	ULong flags = 0;

	ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
	ret = ps3_mgr_cmd_free_nolock(instance, cmd);
	ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);

	return ret;
}

struct ps3_cmd *ps3_task_cmd_alloc(struct ps3_instance *instance)
{
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd_context *context = NULL;

	context = &instance->cmd_context;
	cmd = cmd_pool_alloc(&context->task_cmd_pool, &context->task_pool_lock);
	if (cmd == NULL) {
		cmd = ps3_mgr_cmd_alloc(instance);
	}
	return cmd;
}

S32 ps3_task_cmd_free(struct ps3_instance *instance, struct ps3_cmd *cmd)
{
	ULong flags = 0;
	S32 ret = PS3_SUCCESS;
	(void)instance;

	ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
	ret = ps3_mgr_cmd_free_nolock(instance, cmd);
	ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);

	return ret;
}

static S32 ps3_req_frame_alloc(struct ps3_instance *instance)
{
	U32 size = 0;
	struct ps3_cmd_context *context = &instance->cmd_context;

	size = PS3_DEFAULT_REQ_FRAME_SIZE * context->max_cmd_count;

	context->req_frame_dma_pool = (struct dma_pool *) ps3_dma_pool_create(
		"PS3 req frame pool",&instance->pdev->dev, size,
		DMA_ALIGN_BYTES_256, 0);
	if (!context->req_frame_dma_pool) {
		LOG_ERROR("Failed to setup frame pool\n");
		goto l_create_dma_pool_failed;
	}

	context->req_frame_buf = (U8*)ps3_dma_pool_alloc(instance,
		context->req_frame_dma_pool,
		GFP_KERNEL, &context->req_frame_buf_phys);
	if (!context->req_frame_buf) {
		LOG_ERROR("Failed to alloc frame dma memory\n");
		goto l_free_mem;
	}

	return PS3_SUCCESS;

l_free_mem:
	if (context->req_frame_dma_pool) {
		ps3_dma_pool_destroy(context->req_frame_dma_pool);
		context->req_frame_dma_pool = NULL;
	}
l_create_dma_pool_failed:
	return -PS3_FAILED;
}

static void ps3_req_frame_free(struct ps3_instance *instance)
{
	struct ps3_cmd_context *context = &instance->cmd_context;
#ifndef _WINDOWS
	if (context->req_frame_buf) {
		ps3_dma_pool_free(context->req_frame_dma_pool,
			context->req_frame_buf,
			context->req_frame_buf_phys);
		context->req_frame_buf = NULL;
	}
	if (context->req_frame_dma_pool) {
		ps3_dma_pool_destroy(context->req_frame_dma_pool);
		context->req_frame_dma_pool = NULL;
	}
#else
	if (context->req_frame_buf != NULL) {
		ps3_dma_free_coherent(instance,
			context->req_frame_buf_size,
			context->req_frame_buf,
			context->req_frame_buf_phys);
		context->req_frame_buf = NULL;
		context->req_frame_buf_size = 0;
	}
#endif
}

static S32 ps3_cmd_resp_frame_alloc(struct ps3_instance *instance)
{
	U32 sense_size = 0;
	struct ps3_cmd_context *context = &instance->cmd_context;

	sense_size = PS3_RESP_FRAME_LENGH * context->max_cmd_count;

	context->response_frame_dma_pool = (struct dma_pool *)ps3_dma_pool_create("PS3 respSense pool",
		&instance->pdev->dev, sense_size,
		DMA_ALIGN_BYTES_4K, 0);

	if (!context->response_frame_dma_pool) {
		LOG_ERROR("Failed to setup sense pool\n");
		goto l_failed_alloc;
	}
	context->response_frame_buf = (U8*)ps3_dma_pool_alloc(instance, context->response_frame_dma_pool,
		GFP_KERNEL, &context->response_frame_buf_phys);
	if (!context->response_frame_buf) {
		LOG_ERROR("Failed to alloc sense dma memory\n");
		goto l_free_mem;
	}
	ps3_get_so_addr_ranger(instance, context->response_frame_buf_phys, sense_size);
	return PS3_SUCCESS;

l_free_mem:
	if (context->response_frame_dma_pool) {
		ps3_dma_pool_destroy(context->response_frame_dma_pool);
		context->response_frame_dma_pool = NULL;
	}

l_failed_alloc:
	return -PS3_FAILED;
}

static void ps3_cmd_resp_frame_free(struct ps3_instance *instance)
{
	struct ps3_cmd_context *context = &instance->cmd_context;
#ifndef _WINDOWS
	if (context->response_frame_buf) {
		ps3_dma_pool_free(context->response_frame_dma_pool,
			context->response_frame_buf, context->response_frame_buf_phys);
		context->response_frame_buf = NULL;
	}
	if (context->response_frame_dma_pool) {
		ps3_dma_pool_destroy(context->response_frame_dma_pool);
		context->response_frame_dma_pool = NULL;
	}
#else
	if (context->response_frame_buf != NULL) {
		ps3_dma_free_coherent(instance,
			context->response_frame_buf_size,
			context->response_frame_buf,
			context->response_frame_buf_phys);
		context->response_frame_buf = NULL;
		context->response_frame_buf_size = 0;
	}
#endif
}

static void ps3_cmd_mgr_trans_free(struct ps3_instance *instance)
{
	U32 i = 0;
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd_context *context = &instance->cmd_context;

	if (context->cmd_buf == NULL) {
		return;
	}

	for (i = 0; i < instance->max_mgr_cmd_count; i++) {
		if (context->cmd_buf) {
			cmd = context->cmd_buf[instance->cmd_context.max_scsi_cmd_count + i];
			if (cmd && cmd->transient) {
				ps3_kfree(instance, cmd->transient);
				cmd->transient = NULL;
			}
		}
	}
}

static S32 ps3_cmd_mgr_trans_alloc(struct ps3_instance *instance)
{
	U32 i = 0;
	struct ps3_cmd *cmd;
	struct ps3_cmd_context *context = &instance->cmd_context;

	for (i = 0; i < instance->max_mgr_cmd_count; i++) {
		cmd = context->cmd_buf[instance->cmd_context.max_scsi_cmd_count + i];

		cmd->transient = (struct ps3_ioctl_transient *)
				ps3_kzalloc(instance, sizeof(struct ps3_ioctl_transient));
		if (cmd->transient == NULL) {
			LOG_ERROR("Failed to alloc sge dma memory\n");
			goto l_free;
		}
	}
	return PS3_SUCCESS;
l_free:
	ps3_cmd_mgr_trans_free(instance);
	return -PS3_FAILED;

}

static S32 ps3_cmd_ext_buf_alloc(struct ps3_instance *instance)
{
	U32 i = 0;
	U32 sge_frame_size = 0;
	struct ps3_cmd *cmd;
	struct ps3_cmd_context *context = &instance->cmd_context;

	sge_frame_size = sizeof(struct PS3Sge) * context->ext_sge_frame_count;
	context->ext_buf_size = PS3_MAX(sge_frame_size, PS3_CMD_EXT_BUF_DEFAULT_SIZE);
#ifndef _WINDOWS
	context->ext_buf_dma_pool = (struct dma_pool *)ps3_dma_pool_create("PS3 ext buf pool",
		&instance->pdev->dev, context->ext_buf_size,
		PS3_CMD_EXT_BUF_DEFAULT_SIZE, 0);
	if (!context->ext_buf_dma_pool) {
		LOG_ERROR("Failed to setup sense pool\n");
		goto l_failed_alloc;
	}

	for (i = 0; i < context->max_scsi_cmd_count; i++) {
		cmd = context->cmd_buf[i];
		cmd->ext_buf = ps3_dma_pool_zalloc(instance, context->ext_buf_dma_pool,
			GFP_KERNEL, &cmd->ext_buf_phys);
		if (!cmd->ext_buf) {
			LOG_ERROR("Failed to alloc scsi ext buf memory\n");
			goto l_free_sge;
		}
	}

	context->mgr_ext_buf_size = PS3_CMD_EXT_BUF_SIZE_MGR;
	context->mgr_ext_buf_dma_pool = (struct dma_pool *)ps3_dma_pool_create("PS3 mgr ext buf pool",
		&instance->pdev->dev, context->ext_buf_size,
		PS3_CMD_EXT_BUF_SIZE_MGR, 0);
	if (!context->mgr_ext_buf_dma_pool) {
		LOG_ERROR("Failed to setup sense pool\n");
		goto l_failed_alloc;
	}

	for (i = context->max_scsi_cmd_count; i < context->max_cmd_count; i++) {
		cmd = context->cmd_buf[i];
		cmd->ext_buf = ps3_dma_pool_zalloc(instance, context->mgr_ext_buf_dma_pool,
			GFP_KERNEL, &cmd->ext_buf_phys);
		if (!cmd->ext_buf) {
			LOG_ERROR("Failed to alloc mgr ext buf memory\n");
			goto l_free_sge;
		}
	}

	return PS3_SUCCESS;
l_free_sge:
	ps3_cmd_ext_buf_free(instance);
l_failed_alloc:
	return  -PS3_FAILED;
#else
	for (i = 0; i < context->max_scsi_cmd_count; i++) {
		cmd = context->cmd_buf[i];
		cmd->ext_buf = ps3_dma_alloc_coherent(instance, context->ext_buf_size,
			&cmd->ext_buf_phys);
		if (cmd->ext_buf == NULL) {
			LOG_ERROR("Failed to alloc scsi ext buf memory\n");
			goto l_failed;
		}
	}

	context->mgr_ext_buf_size = PS3_CMD_EXT_BUF_SIZE_MGR;
	for (i = context->max_scsi_cmd_count; i < context->max_cmd_count; i++) {
		cmd = context->cmd_buf[i];
		cmd->ext_buf = ps3_dma_alloc_coherent(instance, context->mgr_ext_buf_size,
			&cmd->ext_buf_phys);
		if (cmd->ext_buf == NULL) {
			LOG_ERROR("Failed to alloc mgr ext buf memory\n");
			goto l_failed;
		}
	}

	return PS3_SUCCESS;

l_failed:
	ps3_cmd_ext_buf_free(instance);
	return -PS3_FAILED;

#endif
}

static void ps3_cmd_ext_buf_free(struct ps3_instance *instance)
{
	U32 i = 0;
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd_context *context = &instance->cmd_context;

	if (context->cmd_buf == NULL) {
		return;
	}
#ifndef _WINDOWS
	for (i = 0; i < context->max_scsi_cmd_count; i++) {
		cmd = context->cmd_buf[i];
		if ((cmd != NULL) && (cmd->ext_buf != NULL)) {
			ps3_dma_pool_free(context->ext_buf_dma_pool,
			cmd->ext_buf,
			cmd->ext_buf_phys);
			cmd->ext_buf = NULL;
		}
	}

	if (context->ext_buf_dma_pool) {
		ps3_dma_pool_destroy(context->ext_buf_dma_pool);
		context->ext_buf_dma_pool = NULL;
	}

	for (i = context->max_scsi_cmd_count; i < context->max_cmd_count; i++) {
		cmd = context->cmd_buf[i];
		if ((cmd != NULL) && (cmd->ext_buf != NULL)) {
			ps3_dma_pool_free(context->mgr_ext_buf_dma_pool,
			cmd->ext_buf,
			cmd->ext_buf_phys);
			cmd->ext_buf = NULL;
		}
	}

	if (context->mgr_ext_buf_dma_pool) {
		ps3_dma_pool_destroy(context->mgr_ext_buf_dma_pool);
		context->mgr_ext_buf_dma_pool = NULL;
	}
#else
	for (i = 0; i < context->max_scsi_cmd_count; i++) {
		cmd = context->cmd_buf[i];
		if ((cmd != NULL) && (cmd->ext_buf != NULL)) {
			ps3_dma_free_coherent(instance, context->ext_buf_size,
				cmd->ext_buf,
				cmd->ext_buf_phys);
			cmd->ext_buf = NULL;
		}
	}

	for (i = context->max_scsi_cmd_count; i < context->max_cmd_count; i++) {
		cmd = context->cmd_buf[i];
		if ((cmd != NULL) && (cmd->ext_buf != NULL)) {
			ps3_dma_free_coherent(instance, context->mgr_ext_buf_size,
				cmd->ext_buf,
				cmd->ext_buf_phys);
			cmd->ext_buf = NULL;
		}
	}
#endif
}

static S32 ps3_cmd_r1xlock_buff_alloc(struct ps3_instance *instance)
{
	U32 i = 0;
	struct ps3_cmd_context *context = &instance->cmd_context;
	struct ps3_cmd *cmd = NULL;
	U32 node_buff_size = ps3_r1x_get_node_Buff_size();

	for (i = 0; i < context->max_scsi_cmd_count; i++) {
		cmd = context->cmd_buf[i];
		cmd->szblock_cnt = 0;
		cmd->node_buff = ps3_kzalloc(instance, node_buff_size);
		if (!cmd->node_buff) {
			LOG_ERROR("Failed to alloc r1x write lock range node buf memory\n");
			goto l_free_node;
		}
	}

	return PS3_SUCCESS;

l_free_node:
	ps3_cmd_r1xlock_buff_free(instance);
	return -PS3_FAILED;
}

static void ps3_cmd_r1xlock_buff_free(struct ps3_instance *instance)
{
	U32 i = 0;
	struct ps3_cmd_context *context = &instance->cmd_context;
	struct ps3_cmd* cmd = NULL;

	if (context->cmd_buf == NULL) {
		return;
	}

	for (i = 0; i < context->max_cmd_count; i++) {
		cmd = context->cmd_buf[i];
		if(cmd != NULL) {
			cmd->szblock_cnt = 0;
			if(cmd->node_buff != NULL){
				ps3_kfree(instance, cmd->node_buff);
				cmd->node_buff = NULL;
			}
		}
	}

	return;
}

static S32 ps3_cmd_buf_alloc(struct ps3_instance *instance)
{
	U32 i = 0;
	struct ps3_cmd_context *context = &instance->cmd_context;

	context->cmd_buf = (struct ps3_cmd**)ps3_kcalloc(instance, context->max_cmd_count,
		sizeof(struct ps3_cmd*));
	if (context->cmd_buf == NULL) {
		LOG_ERROR("Failed to kcalloc memory for cmd_buf\n");
		goto l_failed;
	}
	memset(context->cmd_buf, 0,
		sizeof(struct ps3_cmd*) * context->max_cmd_count);
	for (i = 0; i < context->max_cmd_count; i++) {
		context->cmd_buf[i] = (struct ps3_cmd*)ps3_kzalloc(instance,
			sizeof(struct ps3_cmd));
		if (context->cmd_buf[i] == NULL) {
			LOG_ERROR("Failed to malloc memory for ps3_cmd\n");
			goto l_failed;
		}
	}

	INIT_LIST_HEAD(&context->mgr_cmd_pool);
	INIT_LIST_HEAD(&context->task_cmd_pool);
	INIT_LIST_HEAD(&context->r1x_scsi_cmd_pool);
	ps3_spin_lock_init(&context->mgr_pool_lock);
	ps3_spin_lock_init(&context->task_pool_lock);
	ps3_spin_lock_init(&context->r1x_scsi_pool_lock);
	return PS3_SUCCESS;

l_failed:
	ps3_cmd_buf_free(instance);
	return -PS3_FAILED;
}

static void ps3_cmd_buf_free(struct ps3_instance *instance)
{
	U32 i = 0;
	struct ps3_cmd_context *context = &instance->cmd_context;

	if (context->cmd_buf == NULL) {
		goto l_out;
	}

	while(i < context->max_cmd_count && context->cmd_buf[i]) {
		ps3_kfree(instance, context->cmd_buf[i]);
		context->cmd_buf[i] = NULL;
		i++;
	}

	ps3_kfree(instance, context->cmd_buf);
	context->cmd_buf = NULL;
	INIT_LIST_HEAD(&context->mgr_cmd_pool);
	INIT_LIST_HEAD(&context->task_cmd_pool);
	INIT_LIST_HEAD(&context->r1x_scsi_cmd_pool);
#ifdef _WINDOWS
	INIT_LIST_HEAD(&context->scsi_cmd_pool);
#endif
l_out:
	return;
}

static S32 ps3_cmd_init(struct ps3_instance *instance)
{
	U16 i = 0;
	S32 ret = PS3_SUCCESS;
	U32 offset = 0;

	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd_context *context = &instance->cmd_context;

	for (i = 0; i < context->max_cmd_count; i++) {
		cmd = context->cmd_buf[i];
		if (!cmd) {
			LOG_ERROR("Failed ps3_cmd_init \n");
			ret = -PS3_FAILED;
			goto l_out;
		}

		cmd->instance = instance;
		offset = i * PS3_RESP_FRAME_LENGH;
		cmd->resp_frame = (PS3RespFrame_u *)
			(context->response_frame_buf + offset);
		cmd->resp_frame_phys = context->response_frame_buf_phys + offset;
		cmd->index = i;
		cmd->is_aborting = 0;
#ifndef _WINDOWS
		cmd->scmd = NULL;
#endif
		offset = i * PS3_DEFAULT_REQ_FRAME_SIZE;
		cmd->req_frame = (union PS3ReqFrame *)
			(context->req_frame_buf + offset);
		cmd->req_frame_phys = context->req_frame_buf_phys + offset;
		ps3_spin_lock_init(&cmd->cmd_state.lock);
		ps3_cmd_content_init(cmd);
		if (is_r1x_peer_cmd(instance, i)) {
			if (instance->r1x_mode == PS3_R1X_MODE_NORMAL) {
				list_add_tail(&cmd->cmd_list,
					&context->r1x_scsi_cmd_pool);
			}
		} else if (is_task_cmd(instance,i)) {
			list_add_tail(&cmd->cmd_list, &context->task_cmd_pool);
			init_completion(&cmd->sync_done);
		} else if (is_mgr_cmd(instance,i)) {
			list_add_tail(&cmd->cmd_list, &context->mgr_cmd_pool);
			init_completion(&cmd->sync_done);
		}
#ifdef _WINDOWS
		else {
			list_add_tail(&cmd->cmd_list, &context->scsi_cmd_pool);
		}
#endif
	}

l_out:
	return ret;
}

static inline void ps3_host_max_sge_count(struct ps3_cmd_context *context)
{
	context->max_host_sge_count = PS3_FRAME_REQ_SGE_NUM_HW;
	if (context->sgl_mode_support) {
		if (context->ext_sge_frame_count > PS3_FRAME_REQ_EXT_SGE_MIN) {
			context->max_host_sge_count += (U16)context->ext_sge_frame_count - PS3_FRAME_REQ_EXT_SGE_MIN;
		}
	} else if (context->ext_sge_frame_count > 1) {
		context->max_host_sge_count = PS3_MAX((U16)(context->ext_sge_frame_count - 1),
			context->max_host_sge_count);
	}
}

static inline void ps3_r1x_mode_set(struct ps3_instance *instance)
{
	struct ps3_cmd_context *context = &instance->cmd_context;
	if (context->max_r1x_cmd_count >= context->max_scsi_cmd_count / 2) {
		instance->r1x_mode = PS3_R1X_MODE_PERF;
	}
	LOG_INFO("host_no:%u r1x_mode:%u\n", PS3_HOST(instance), instance->r1x_mode);
}

S32 ps3_cmd_context_init(struct ps3_instance *instance)
{
	S32 ret = -PS3_FAILED;
	struct ps3_cmd_context *context = &instance->cmd_context;
	int cpu = 0;
	S64 *scsi_cmd_deliver = NULL;

	if (!ps3_ioc_mgr_max_fw_cmd_get(instance, &context->max_cmd_count)) {
		goto l_failed;
	}
#ifdef PS3_HARDWARE_SIM
	context->max_r1x_cmd_count = 16;
#else
	if (!ps3_get_max_r1x_cmds_with_check(instance,
			&context->max_r1x_cmd_count)) {
		goto l_failed;
	}
#endif
	LOG_DEBUG("host_no:%u max_r1x_cmd_count:%u\n",
		PS3_HOST(instance), context->max_r1x_cmd_count);
	context->max_mgr_cmd_count = instance->max_mgr_cmd_total_count;
	context->max_scsi_cmd_count = context->max_cmd_count -
		instance->max_mgr_cmd_total_count;

	if (context->max_r1x_cmd_count > (context->max_scsi_cmd_count / 2 )) {
		context->max_r1x_cmd_count = (context->max_scsi_cmd_count / 2);
	}
	ps3_r1x_mode_set(instance);

	LOG_DEBUG("host_no:%u max_r1x_cmd_final count:%u\n",
		PS3_HOST(instance), context->max_r1x_cmd_count);

	if (!ps3_ioc_mgr_max_chain_size_get(instance,
			&context->ext_sge_frame_count)) {
		goto l_failed;
	}
	context->ext_sge_frame_count /= sizeof(struct PS3Sge);

	if (!ps3_ioc_mgr_max_nvme_page_size_get(instance,
			&instance->cmd_attr.nvme_page_size)) {
		goto l_failed;
	}
	context->max_prp_count = PS3_FRAME_REQ_PRP_NUM_FE +
		(instance->cmd_attr.nvme_page_size / sizeof(U64));

	ps3_host_max_sge_count(context);

	if (context->max_cmd_count <= instance->max_mgr_cmd_total_count) {
		LOG_ERROR("max_cmd_count %d too few\n",
			context->max_cmd_count);
		goto l_failed;
	}

	ret = ps3_cmd_buf_alloc(instance);
	if (ret != PS3_SUCCESS) {
		goto l_failed;
	}

	ret = ps3_req_frame_alloc(instance);
	if (ret != PS3_SUCCESS) {
		goto l_failed;
	}

	ret = ps3_cmd_resp_frame_alloc(instance);
	if (ret != PS3_SUCCESS) {
		goto l_failed;
	}

	ret = ps3_cmd_ext_buf_alloc(instance);
	if (ret != PS3_SUCCESS) {
		goto l_failed;
	}

	ret = ps3_cmd_r1xlock_buff_alloc(instance);
	if (ret != PS3_SUCCESS) {
		goto l_failed;
	}

	ret = ps3_cmd_mgr_trans_alloc(instance);
	if (ret != PS3_SUCCESS) {
		goto l_failed;
	}

	ret = ps3_cmd_init(instance);
	if (ret != PS3_SUCCESS) {
		goto l_failed;
	}

	instance->scsi_cmd_deliver = alloc_percpu(S64);
	if (!instance->scsi_cmd_deliver) {
		LOG_ERROR("alloc per_cpu scsi_cmd_deliver failed. hno:%u\n",
				PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_failed;
	} else {
		for_each_possible_cpu(cpu) {
			scsi_cmd_deliver = per_cpu_ptr(instance->scsi_cmd_deliver, cpu);
			*scsi_cmd_deliver = 0;
		}
	}

	return ret;

l_failed:
	ps3_cmd_context_exit(instance);
	return ret;
}

void ps3_cmd_context_exit(struct ps3_instance *instance)
{
	ps3_cmd_mgr_trans_free(instance);
	ps3_cmd_ext_buf_free(instance);
	ps3_cmd_r1xlock_buff_free(instance);
	ps3_cmd_resp_frame_free(instance);
	ps3_req_frame_free(instance);
	ps3_cmd_buf_free(instance);

	if (instance->scsi_cmd_deliver) {
		free_percpu(instance->scsi_cmd_deliver);
		instance->scsi_cmd_deliver = NULL;
	}
}

static void ps3_cmd_content_init(struct ps3_cmd *cmd)
{
	cmd->cmd_word_value = 0;
#ifndef _WINDOWS
	cmd->scmd = NULL;
#else
	cmd->srb = NULL;
	memset(&cmd->scmd_imp, 0, sizeof(cmd->scmd_imp));
	cmd->scmd = &cmd->scmd_imp;
#endif
	cmd->trace_id = 0;
	cmd->no_reply_word = 0;
	cmd->os_sge_map_count = 0;
	INIT_LIST_HEAD(&cmd->cmd_list);
	cmd->cmd_receive_cb = NULL;
	cmd->cmd_send_cb = NULL;

	cmd->cmd_state.state = PS3_CMD_STATE_INIT;
	cmd->cmd_state.reset_flag = 0;
	cmd->qos_processing = PS3_FALSE;
	cmd->time_out = 0;
	cmd->is_interrupt = PS3_DRV_FALSE;
	cmd->retry_cnt = 0;
	cmd->is_force_polling = 0;
	cmd->is_inserted_c_q = 0;
	cmd->is_got_r1x = 0;
	cmd->is_r1x_aborting = 0;
	cmd->r1x_peer_cmd = NULL;
	cmd->r1x_reply_flag = 0;
	cmd->is_r1x_scsi_complete = PS3_FALSE;
	cmd->flighting = PS3_FALSE;
	cmd->r1x_read_pd = 0;

	memset(&cmd->io_attr, 0, sizeof(struct ps3_scsi_io_attr));
	memset((void*)&cmd->sync_done, 0, sizeof(cmd->sync_done));
	if (cmd->transient == NULL || cmd->transient->sge_num == 0) {
		memset((void*)cmd->req_frame, 0, sizeof(union PS3ReqFrame));
	}

	memset((void*)cmd->resp_frame, 0xff, sizeof(PS3RespFrame_u));

	memset(cmd->ext_buf, 0, cmd->instance->cmd_context.ext_buf_size);

	INIT_LIST_HEAD(&cmd->qos_list);
	memset(&cmd->target_pd, 0, sizeof(struct ps3_qos_member_pd_info) * PS3_QOS_MAX_PD_IN_VD);
	cmd->target_pd_count = 0;
	cmd->first_over_quota_pd_idx = 0;
	cmd->qos_waitq_flag = 0;
	memset(&cmd->cmdq_info, 0, sizeof(struct ps3_qos_cmdq_info) * PS3_QOS_MAX_CMDQ_ONE_CMD);
	cmd->cmdq_count = 0;
}

static void ps3_scsi_cmd_content_init(struct ps3_cmd *cmd)
{
	cmd->cmd_word_value = 0;
#ifndef _WINDOWS
	cmd->scmd = NULL;
#else
	cmd->srb = NULL;
	memset(&cmd->scmd_imp, 0, sizeof(cmd->scmd_imp));
	cmd->scmd = &cmd->scmd_imp;
#endif
	cmd->trace_id = 0;
	cmd->no_reply_word = 0;
	cmd->os_sge_map_count = 0;
	INIT_LIST_HEAD(&cmd->cmd_list);
	cmd->cmd_receive_cb = NULL;
	cmd->cmd_send_cb = NULL;

	cmd->cmd_state.state = PS3_CMD_STATE_INIT;
	cmd->cmd_state.reset_flag = 0;
	cmd->qos_processing = PS3_FALSE;
	cmd->time_out = 0;
	cmd->is_interrupt = PS3_DRV_FALSE;
	cmd->retry_cnt = 0;
	cmd->is_force_polling = 0;
	cmd->is_inserted_c_q = 0;
	cmd->is_got_r1x = 0;
	cmd->is_r1x_aborting = 0;
	cmd->r1x_peer_cmd = NULL;
	cmd->r1x_reply_flag = 0;
	cmd->is_r1x_scsi_complete = PS3_FALSE;
	cmd->flighting = PS3_FALSE;
	cmd->r1x_read_pd = 0;

	if (cmd->req_frame->hwReq.sgl[PS3_FRAME_REQ_SGE_NUM_HW - 1].length != 0) {
		memset(cmd->ext_buf, 0, cmd->instance->cmd_context.ext_buf_size);
	}

	if (!ps3_scsih_is_rw_type(cmd->io_attr.rw_flag) &&
			(cmd->transient == NULL || cmd->transient->sge_num == 0)) {
		memset((void*)cmd->req_frame, 0, sizeof(union PS3ReqFrame));
	}

	if (cmd->resp_frame->normalRespFrame.respStatus != 0xFF) {
		cmd->resp_frame->normalRespFrame.respStatus = 0xFF;
	}

	if (cmd->resp_frame->sasRespFrame.status != 0xFF) {
		cmd->resp_frame->sasRespFrame.status = 0xFF;
	}

	memset((void*)&cmd->sync_done, 0, sizeof(cmd->sync_done));
	memset(&cmd->io_attr, 0, sizeof(struct ps3_scsi_io_attr));
	INIT_LIST_HEAD(&cmd->qos_list);
	memset(&cmd->target_pd, 0, sizeof(struct ps3_qos_member_pd_info) * PS3_QOS_MAX_PD_IN_VD);
	cmd->target_pd_count = 0;
	cmd->first_over_quota_pd_idx = 0;
	cmd->qos_waitq_flag = 0;
	memset(&cmd->cmdq_info, 0, sizeof(struct ps3_qos_cmdq_info) * PS3_QOS_MAX_CMDQ_ONE_CMD);
	cmd->cmdq_count = 0;
}

#ifndef _WINDOWS
struct ps3_cmd *ps3_scsi_cmd_alloc(struct ps3_instance *instance, U32 tag)
{
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd_context *context = &instance->cmd_context;

	if (tag < (U32)instance->cmd_attr.cur_can_que) {
		cmd = context->cmd_buf[tag];
		cmd->cmd_state.state = PS3_CMD_STATE_PROCESS;
		ps3_trace_id_alloc(&cmd->trace_id);
		cmd->flighting = PS3_TRUE;
	}
	return cmd;
}

S32 ps3_scsi_cmd_free(struct ps3_cmd *cmd)
{
	S32 ret = -PS3_FAILED;

	if (cmd->index < (U32)cmd->instance->cmd_attr.cur_can_que) {
		ps3_scsi_cmd_content_init(cmd);
		ret = PS3_SUCCESS;
	}
	return ret;
}
#else
struct ps3_cmd *ps3_scsi_cmd_alloc(struct ps3_instance *instance)
{
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd_context *context = &instance->cmd_context;
	ULong flags = 0;

	ps3_spin_lock_irqsave(&context->scsi_pool_lock, &flags);
	if (!list_empty(&context->scsi_cmd_pool)) {
		cmd = list_entry(list_remove_head(&context->scsi_cmd_pool), struct ps3_cmd, cmd_list);
	}
	ps3_spin_unlock_irqrestore(&context->scsi_pool_lock, flags);

	if (cmd != NULL) {
		cmd->trace_id = ps3_atomic64_inc(&context->trace_id);
		cmd->cmd_state.state = PS3_CMD_STATE_PROCESS;
	}
	return cmd;
}

S32 ps3_scsi_cmd_free(struct ps3_cmd *cmd)
{
	S32 ret = -PS3_FAILED;
	U32 max_count = 0;
	ULong flags = 0;
	struct ps3_cmd_context *context = &cmd->instance->cmd_context;

	if (unlikely(cmd == NULL)) {
		goto l_out;
	}

	max_count = context->max_scsi_cmd_count;
	if (cmd->index < max_count) {
		ps3_cmd_content_init(cmd);
		ret = PS3_SUCCESS;
	}

	ps3_spin_lock_irqsave(&context->scsi_pool_lock, &flags);
	list_add_tail(&cmd->cmd_list, &context->scsi_cmd_pool);
	ps3_spin_unlock_irqrestore(&context->scsi_pool_lock, flags);
l_out:
	return ret;
}
#endif

static void cmd_pool_free(ps3_list_head *pool_list,
	ps3_spinlock *pool_lock, struct ps3_cmd *cmd)
{
	ULong flags = 0;
	ps3_spin_lock_irqsave(pool_lock, &flags);
	list_add_tail(&cmd->cmd_list, pool_list);
	ps3_spin_unlock_irqrestore(pool_lock, flags);
}

static struct ps3_cmd *cmd_pool_alloc(ps3_list_head *pool_list,
	ps3_spinlock *pool_lock)
{
	struct ps3_cmd *cmd = NULL;
	ULong flags = 0;
#ifdef _WINDOWS
	struct ps3_cmd_context *context = NULL;
#endif
	ps3_spin_lock_irqsave(pool_lock, &flags);
	if (!list_empty(pool_list)) {
#ifdef _WINDOWS
		cmd = list_first_entry(pool_list, struct ps3_cmd, cmd_list);
#else
		cmd = list_entry(pool_list->next, struct ps3_cmd, cmd_list);
#endif
		list_del_init(&cmd->cmd_list);
	}
	ps3_spin_unlock_irqrestore(pool_lock, flags);

	if(cmd != NULL) {
#ifndef _WINDOWS
		ps3_trace_id_alloc(&cmd->trace_id);
#else
		context = &cmd->instance->cmd_context;
		cmd->trace_id = ps3_atomic64_inc(&context->trace_id);
#endif
		cmd->cmd_state.state = PS3_CMD_STATE_PROCESS;
		init_completion(&cmd->sync_done);
	}

	return cmd;
}

Bool ps3_r1x_peer_cmd_free_nolock(struct ps3_cmd *cmd)
{
	Bool ret = PS3_TRUE;
	struct ps3_cmd_context *context = NULL;
	struct ps3_instance *instance = cmd->instance;

	context = &instance->cmd_context;
	if (!is_r1x_peer_cmd(instance, cmd->index)) {
		ret = PS3_FALSE;
		goto l_out;
	}

	if(cmd->cmd_state.state == PS3_CMD_STATE_INIT) {
		goto l_out;
	}

	ps3_scsi_cmd_content_init(cmd);
	if (instance->r1x_mode == PS3_R1X_MODE_NORMAL) {
		cmd_pool_free(&context->r1x_scsi_cmd_pool,
			&context->r1x_scsi_pool_lock, cmd);
	}
l_out:
	return ret;
}

S32 ps3_mgr_cmd_free_nolock(struct ps3_instance *instance, struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd_context *context = NULL;

	context = &instance->cmd_context;
	if (cmd->index < context->max_scsi_cmd_count) {
		ret = -PS3_FAILED;
		goto l_out;
	}

	if(cmd->cmd_state.state == PS3_CMD_STATE_INIT) {
		goto l_out;
	}

	ps3_cmd_content_init(cmd);
	INJECT_START(PS3_ERR_FORCE_SET_CMD_INDEX_NOT_MGR, &cmd->index);
	if (is_task_cmd(instance, cmd->index)) {
		cmd_pool_free(&context->task_cmd_pool,
			&context->task_pool_lock, cmd);
	} else if (is_mgr_cmd(instance, cmd->index)) {
		cmd_pool_free(&context->mgr_cmd_pool,
			&context->mgr_pool_lock, cmd);
	} else {
		LOG_INFO_IN_IRQ(instance, "host_no:%u CFID:%u not mgr cmd!\n",
			PS3_HOST(instance), cmd->index);
		PS3_BUG();
		ret = -PS3_FAILED;
	}
l_out:
	return ret;
}

S32 ps3_async_cmd_send(struct ps3_instance *instance, struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	S32 cur_state = PS3_INSTANCE_STATE_INIT;

	ps3_atomic_inc(&instance->cmd_statistics.cmd_delivering);
	mb();    
	ret = ps3_cmd_send_pre_check(instance);
	if (ret != PS3_SUCCESS) {
		goto l_out;
	}

	cur_state = ps3_atomic_read(&instance->state_machine.state);
	if (cmd == instance->event_context.event_cmd) {
		INJECT_START(PS3_ERR_IJ_FORCE_EVENT_CMD_FAIL_DEAD, &cur_state)
	}
	if (cur_state != PS3_INSTANCE_STATE_OPERATIONAL) {
		if (instance->is_probe_finish == PS3_TRUE) {
			LOG_FILE_ERROR("host_no:%u cannot send async cmd due to %s, return fail\n",
				PS3_HOST(instance), namePS3InstanceState(cur_state));
			ret = -PS3_FAILED;
		} else {
			LOG_FILE_WARN("host_no:%u cannot send async cmd due to %s, return recovered\n",
				PS3_HOST(instance), namePS3InstanceState(cur_state));
			ret = -PS3_RECOVERED;
		}
		goto l_out;
	}

	instance->ioc_adpter->cmd_send(instance, &cmd->cmd_word);
l_out:
	ps3_atomic_dec(&instance->cmd_statistics.cmd_delivering);
	return ret;
}
#ifndef _WINDOWS

static void ps3_r1x_peer_cmd_build(struct ps3_cmd *cmd,
	struct ps3_cmd *peer_cmd)
{
	memcpy(peer_cmd->req_frame, cmd->req_frame,
		sizeof(union PS3ReqFrame));
	memcpy((void*)&peer_cmd->io_attr, (void*)&cmd->io_attr,
		sizeof(struct ps3_scsi_io_attr));
	memcpy(peer_cmd->ext_buf, cmd->ext_buf, cmd->instance->cmd_context.ext_buf_size);

	peer_cmd->scmd = cmd->scmd;
	peer_cmd->is_got_r1x = cmd->is_got_r1x;
	peer_cmd->szblock_cnt = cmd->szblock_cnt;
	peer_cmd->os_sge_map_count = cmd->os_sge_map_count;
	peer_cmd->cmd_receive_cb = cmd->cmd_receive_cb;
	peer_cmd->io_attr.pd_entry = cmd->io_attr.peer_pd_entry;
	peer_cmd->io_attr.disk_id = PS3_PDID(&peer_cmd->io_attr.pd_entry->disk_pos);
	peer_cmd->io_attr.plba = cmd->io_attr.plba_back;

	peer_cmd->cmd_word_value = cmd->cmd_word_value;
	peer_cmd->cmd_word.phyDiskID = PS3_PDID(&peer_cmd->io_attr.pd_entry->disk_pos);
	peer_cmd->cmd_word.cmdFrameID = peer_cmd->index;

	peer_cmd->req_frame->hwReq.reqHead.cmdFrameID = peer_cmd->index;
	peer_cmd->req_frame->hwReq.reqHead.devID.diskID =
		peer_cmd->io_attr.pd_entry->disk_pos.diskDev.diskID;
	peer_cmd->req_frame->hwReq.reqHead.traceID = peer_cmd->trace_id;

	ps3_vd_direct_req_frame_build(peer_cmd);

	cmd->r1x_peer_cmd = peer_cmd;
	peer_cmd->r1x_peer_cmd = cmd;
	cmd->is_r1x_scsi_complete = PS3_FALSE;
	peer_cmd->is_r1x_scsi_complete = PS3_FALSE;

	LOG_DEBUG("host_no:%u r1x direct write cmd:%d, peer cmd:%d build:"
		" tid:0x%llx pid:%u plba:0x%llx\n",
		PS3_HOST(cmd->instance), cmd->index, peer_cmd->index,
		peer_cmd->trace_id, peer_cmd->cmd_word.phyDiskID,
		peer_cmd->io_attr.plba);
}

static struct ps3_cmd* ps3_r1x_scsi_peer_prepare(struct ps3_instance *instance,
	struct ps3_cmd *cmd)
{
	struct ps3_cmd *peer_cmd = NULL;
	if (cmd->io_attr.direct_flag != PS3_CMDWORD_DIRECT_ADVICE ||
		cmd->io_attr.peer_pd_entry == NULL) {
		goto _lout;
	}

	peer_cmd = ps3_r1x_peer_cmd_alloc(instance, cmd->index);
	if (peer_cmd != NULL) {
		ps3_r1x_peer_cmd_build(cmd, peer_cmd);
	} else {
		LOG_DEBUG("host_no:%u cmd:%d can not alloc r1x peer cmd any more\n",
			PS3_HOST(instance), cmd->index);
		instance->ioc_adpter->io_cmd_rebuild(cmd);
	}
_lout:
	return peer_cmd;
}

void ps3_wait_scsi_cmd_done(struct ps3_instance *instance, Bool time_out)
{
	int cpu = 0;
	S64 result = 0;
	U16 try_cnt = 0;

	if (instance->scsi_cmd_deliver) {
		do {
			result = 0;
			for_each_possible_cpu(cpu) {
				result  += *per_cpu_ptr(instance->scsi_cmd_deliver, cpu);
			}

			if (result > 0) {
				ps3_msleep(PS3_LOOP_TIME_INTERVAL_100MS);
				if (time_out) {
					try_cnt++;
					if (try_cnt > PS3_WAIT_SCSI_CMD_DONE_COUNT) {
						LOG_WARN("hno:%u wait scsi cmd done NOK\n",
							PS3_HOST(instance));
						INJECT_START(PS3_ERR_IJ_WAIT_SCSI_CMD_DONE_FAIL, instance);
						break;
					}
				}
			}
		} while (result);
	}

	LOG_INFO("wait scsi cmd done end. hno:%u try_cnt[%u]\n",
				PS3_HOST(instance), try_cnt);
}

void ps3_scsi_cmd_deliver_get(struct ps3_instance *instance)
{
	S64 *cmd_deliver = NULL;
	cmd_deliver = get_cpu_ptr(instance->scsi_cmd_deliver);
	(*cmd_deliver)++;
	put_cpu_ptr(cmd_deliver);
}

void ps3_scsi_cmd_deliver_put(struct ps3_instance *instance)
{
	S64 *cmd_deliver = NULL;
	cmd_deliver = get_cpu_ptr(instance->scsi_cmd_deliver);
	(*cmd_deliver)--;
	put_cpu_ptr(cmd_deliver);
}

S32 ps3_scsi_cmd_send(struct ps3_instance *instance, struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd *peer_cmd = NULL;

	ret = ps3_cmd_send_pre_check(instance);
	if (ret != PS3_SUCCESS) {
		goto l_out;
	}

	PS3_IJ_SLEEP(10000, PS3_ERR_IJ_SCSI_DELIVER_DELAY);
	INJECT_START(PS3_ERR_IJ_WAIT_TASK_MGR_BUSY, instance);
	INJECT_START(PS3_ERR_IJ_FORCE_TASK_MGR_BUSY, instance);
	if (unlikely(instance->task_manager_host_busy)) {
		INJECT_START(PS3_ERR_IJ_SEND_CMD_TASK_MGR_BUSY, instance);
		LOG_INFO_LIM("host_no:%u cannot send block cmd due to task_manager_host_busy\n",
			PS3_HOST(instance));
		ret = -PS3_RETRY;
		goto l_out;
	}

	INJECT_START(PS3_ERR_IJ_CMD_SEND_FORCE_INS_UNLOAD, instance)
	if (!instance->state_machine.is_load) {
		LOG_WARN_LIM("host_no:%u instance state not is_load\n", PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}
	INJECT_START(PS3_ERR_IJ_CMD_SEND_BLOCK, instance);
	INJECT_START(PS3_ERR_IJ_CMD_SEND_FORCE_UNORMAL, instance)

	PS3_IJ_SLEEP(10000, PS3_ERR_IJ_SCSI_DELIVER_DELAY_2);

	if (!ps3_is_instance_state_normal(instance)) {
		ret = -PS3_RECOVERED;
		goto l_out;
	}

	peer_cmd = ps3_r1x_scsi_peer_prepare(instance, cmd);

	PS3_DEV_IO_START_INC(instance, cmd);
	PS3_DEV_IO_START_OK_INC(instance, cmd);
	PS3_DEV_IO_OUTSTAND_INC(instance, cmd);
	PS3_IO_DRV2IOC_START_INC(instance, cmd);
	cmd->flighting = PS3_FALSE;
	wmb();

	ps3_ioc_scsi_cmd_send(instance, &cmd->cmd_word);
	INJECT_START(PS3_ERR_IJ_WAIT_CMD_DONE, instance);
	if (peer_cmd != NULL) {
		if (ps3_r1x_interval_delay_get() != 0) {
			msleep(ps3_r1x_interval_delay_get());
		}
		PS3_IO_DRV2IOC_START_INC(instance, peer_cmd);
		ps3_ioc_scsi_cmd_send(instance, &peer_cmd->cmd_word);
	}
l_out:

	return ret;
}
#endif

struct ps3_cmd *ps3_cmd_find(struct ps3_instance *instance, U16 cmd_frame_id)
{
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd_context *context = &instance->cmd_context;
	if (cmd_frame_id >= context->max_cmd_count) {
		LOG_ERROR_IN_IRQ(instance, "host_no:%u CFID:%d invalid\n",
			PS3_HOST(instance), cmd_frame_id);
		goto l_failed;
	}

	cmd = context->cmd_buf[cmd_frame_id];
	if (cmd->index != cmd_frame_id) {
		LOG_ERROR_IN_IRQ(instance, "host_no:%u CFID:%d incorrect, expect CFID:%d\n",
			PS3_HOST(instance), cmd_frame_id, cmd->index);
		cmd = NULL;
		goto l_failed;
	}

l_failed:
	return cmd;
}

S32 ps3_cmd_dispatch(struct ps3_instance *instance, U16 cmd_frame_id, struct PS3ReplyWord *reply_word)
{
	S32 ret = -PS3_FAILED;
	struct ps3_cmd *cmd = NULL;
	U16 reply_flags = 0xff;

	if (reply_word->retType == PS3_HARD_RET &&
		reply_word->retStatus == PS3_REPLY_WORD_FLAG_REPEAT_REPLY) {
		LOG_ERROR_IN_IRQ(instance, "hno:%u repeated response CFID:%u reply_word:0x%llx\n",
			PS3_HOST(instance), cmd_frame_id, *(U64 *)reply_word);
		goto l_out;
	}
	cmd = ps3_cmd_find(instance, cmd_frame_id);
	if (cmd == NULL) {
		goto l_out;
	}
	memcpy(&(cmd->reply_word), reply_word, sizeof(struct PS3ReplyWord));
	reply_flags = reply_word->retStatus;
	if (cmd->cmd_receive_cb) {
		ret = cmd->cmd_receive_cb(cmd, reply_flags);
	} else {
		LOG_ERROR_IN_IRQ(instance, "warn ps3 cmd index %d has no cmd_receive_cb\n",
			cmd->index);
	}
l_out:
	return ret;
}

Bool ps3_is_instance_state_allow_cmd_execute(struct ps3_instance *instance)
{
	Bool ret = PS3_TRUE;
	S32 cur_state = PS3_INSTANCE_STATE_INIT;

	cur_state = ps3_atomic_read(&instance->state_machine.state);
	INJECT_START(PS3_ERR_IJ_INS_STATE_UNNORMAL, &cur_state)
	INJECT_START(PS3_ERR_IJ_INS_STATE_DEAD, &cur_state)
	if (cur_state == PS3_INSTANCE_STATE_DEAD &&
		(PS3_IOC_STATE_HALT_SUPPORT(instance) == PS3_TRUE) &&
		PS3_HALT_CLI_SUPPORT(instance)){
		goto l_out;
	}

	if (cur_state != PS3_INSTANCE_STATE_OPERATIONAL &&
		cur_state != PS3_INSTANCE_STATE_PRE_OPERATIONAL &&
		cur_state != PS3_INSTANCE_STATE_SOFT_RECOVERY) {
		LOG_FILE_INFO("host_no:%u cannot handle cmd, driver state: %s\n",
			PS3_HOST(instance), namePS3InstanceState(cur_state));
		ret = PS3_FALSE;
	}

l_out:
	return ret;
}

S32 ps3_cmd_send_pre_check(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	if (instance->is_probe_finish && !instance->state_machine.is_load) {
		LOG_FILE_INFO("host_no:%u instance state state is unloading\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}
	INJECT_START(PS3_ERR_IJ_CMD_SEND_FORCE_PCI_ERR, instance)
	if (ps3_pci_err_recovery_get(instance)) {
		LOG_FILE_WARN("host_no:%u cannot send block cmd due to pci err recovery\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}
l_out:
	return ret;
}

void ps3_dma_addr_bit_pos_update(struct ps3_instance *instance, U8 bit_pos)
{
	U32 i = 0;
	struct ps3_irq_context *irq_context = &instance->irq_context;
	struct ps3_cmd_context *cmd_context = &instance->cmd_context;
	struct ps3_debug_context *debug_context = &instance->debug_context;
	struct ps3_dump_context *dump_context = &instance->dump_context;
	struct ps3_dev_context *dev_context = &instance->dev_context;
	struct ps3_sas_dev_context *ps3_sas_ctx = &instance->sas_dev_context;
	struct ps3_cmd *cmd = NULL;

	irq_context->reply_fifo_desc_buf_phys = PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos,
			irq_context->reply_fifo_desc_buf_phys);
	irq_context->reply_fifo_desc_buf_phys = PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos,
			irq_context->reply_fifo_desc_buf_phys);
	for (; i < irq_context->valid_msix_vector_count; i++) {
		irq_context->reply_fifo_desc_buf[i].ReplyFifoBaseAddr =
			PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, irq_context->reply_fifo_desc_buf[i].ReplyFifoBaseAddr);
		irq_context->reply_fifo_desc_buf[i].ReplyFifoBaseAddr =
			PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos,
				irq_context->reply_fifo_desc_buf[i].ReplyFifoBaseAddr);
		irq_context->reply_fifo_phys_base_addr_buf[i] = PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos,
				irq_context->reply_fifo_phys_base_addr_buf[i]);
		irq_context->reply_fifo_phys_base_addr_buf[i] = PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos,
				irq_context->reply_fifo_phys_base_addr_buf[i]);
	}
	cmd_context->init_frame_buf_phys =
			PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, cmd_context->init_frame_buf_phys);
	cmd_context->init_frame_buf_phys =
			PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos, cmd_context->init_frame_buf_phys);
	cmd_context->init_filter_table_phy_addr =
			PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, cmd_context->init_filter_table_phy_addr);
	cmd_context->init_filter_table_phy_addr =
			PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos, cmd_context->init_filter_table_phy_addr);
	if (cmd_context->init_frame_sys_info_buf != NULL) {
		cmd_context->init_frame_sys_info_phys =
			PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, cmd_context->init_frame_sys_info_phys);
		cmd_context->init_frame_sys_info_phys =
			PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos, cmd_context->init_frame_sys_info_phys);
	}
	instance->ctrl_info_buf_h = PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, instance->ctrl_info_buf_h);
	instance->ctrl_info_buf_h =
			PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos, instance->ctrl_info_buf_h);
	cmd_context->req_frame_buf_phys =
			PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, cmd_context->req_frame_buf_phys);
	cmd_context->req_frame_buf_phys =
			PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos, cmd_context->req_frame_buf_phys);
	cmd_context->response_frame_buf_phys =
			PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, cmd_context->response_frame_buf_phys);
	cmd_context->response_frame_buf_phys =
			PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos, cmd_context->response_frame_buf_phys);
	for (i = 0; i < cmd_context->max_cmd_count; i++) {
		cmd = cmd_context->cmd_buf[i];
		if (cmd->ext_buf != NULL) {
			cmd->ext_buf_phys = PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, cmd->ext_buf_phys);
			cmd->ext_buf_phys = PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos, cmd->ext_buf_phys);
		}
	}
	if (debug_context->debug_mem_buf != NULL) {
		debug_context->debug_mem_buf_phy =
				PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, debug_context->debug_mem_buf_phy);
		debug_context->debug_mem_buf_phy =
				PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos, debug_context->debug_mem_buf_phy);
		for (i = 0; i < debug_context->debug_mem_array_num; i++) {
			debug_context->debug_mem_buf[i].debugMemAddr = PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos,
					debug_context->debug_mem_buf[i].debugMemAddr);
			debug_context->debug_mem_buf[i].debugMemAddr =
				PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos,
					debug_context->debug_mem_buf[i].debugMemAddr);
		}
	}
	if (dump_context->dump_dma_buf != NULL) {
		dump_context->dump_dma_addr =
				PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, dump_context->dump_dma_addr);
		dump_context->dump_dma_addr =
				PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos, dump_context->dump_dma_addr);
	}
	instance->drv_info_buf_phys = PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, instance->drv_info_buf_phys);
	instance->drv_info_buf_phys =
			PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos, instance->drv_info_buf_phys);
	instance->host_mem_info_buf_phys =
			PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, instance->host_mem_info_buf_phys);
	instance->host_mem_info_buf_phys =
			PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos, instance->host_mem_info_buf_phys);
	if (dev_context->pd_list_buf != NULL) {
		dev_context->pd_list_buf_phys =
				PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, dev_context->pd_list_buf_phys);
		dev_context->pd_list_buf_phys =
				PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos, dev_context->pd_list_buf_phys);
	}
	if (dev_context->pd_info_buf != NULL) {
		dev_context->pd_info_buf_phys =
				PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, dev_context->pd_info_buf_phys);
		dev_context->pd_info_buf_phys =
				PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos, dev_context->pd_info_buf_phys);
	}
	if (dev_context->vd_list_buf != NULL) {
		dev_context->vd_list_buf_phys =
				PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, dev_context->vd_list_buf_phys);
		dev_context->vd_list_buf_phys =
				PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos, dev_context->vd_list_buf_phys);
	}
	if (dev_context->vd_info_buf_sync != NULL) {
		dev_context->vd_info_buf_phys_sync =
				PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, dev_context->vd_info_buf_phys_sync);
		dev_context->vd_info_buf_phys_sync =
				PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos, dev_context->vd_info_buf_phys_sync);
	}
	if (dev_context->vd_info_buf_async != NULL) {
		dev_context->vd_info_buf_phys_async =
				PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, dev_context->vd_info_buf_phys_async);
		dev_context->vd_info_buf_phys_async =
				PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos, dev_context->vd_info_buf_phys_async);
	}
	if (ps3_sas_ctx->ps3_sas_buff != NULL) {
		ps3_sas_ctx->ps3_sas_buff_dma_addr =
				PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, ps3_sas_ctx->ps3_sas_buff_dma_addr);
		ps3_sas_ctx->ps3_sas_buff_dma_addr =
				PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos, ps3_sas_ctx->ps3_sas_buff_dma_addr);
	}
	if (ps3_sas_ctx->ps3_sas_phy_buff != NULL) {
		ps3_sas_ctx->ps3_sas_phy_buff_dma_addr =
				PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, ps3_sas_ctx->ps3_sas_phy_buff_dma_addr);
		ps3_sas_ctx->ps3_sas_phy_buff_dma_addr =
				PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos, ps3_sas_ctx->ps3_sas_phy_buff_dma_addr);
	}
	instance->so_start_addr = PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, instance->so_start_addr);
	instance->so_start_addr = PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos, instance->so_start_addr);
	instance->so_end_addr = PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, instance->so_end_addr);
	instance->so_end_addr = PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos, instance->so_end_addr);
}

Bool ps3_bit_pos_update(struct ps3_instance *instance)
{
	U8 old_bit_pos = instance->dma_addr_bit_pos;
	U8 bit_pos = 0;
	Bool ret = PS3_FALSE;

	if (!ps3_ioc_atu_support_retry_read(instance, &bit_pos)) {
		goto l_out;
	}
	switch(bit_pos) {
		case PS3_BIT_POS_DEFAULT:
		case PS3_BIT_POS_44:
			instance->dma_addr_bit_pos = PCIE_DMA_HOST_ADDR_BIT_POS;
			break;
		case PS3_BIT_POS_53:
			instance->dma_addr_bit_pos = PCIE_DMA_HOST_ADDR_BIT_POS_F1;
			break;
		case PS3_BIT_POS_54:
			instance->dma_addr_bit_pos = PCIE_DMA_HOST_ADDR_BIT_POS_F0;
			break;
		default:
			LOG_WARN("hno:%u bit pos value is unexpect %u\n",
					PS3_HOST(instance), bit_pos);
			goto l_out;
	}
	mb();
	if (instance->dma_addr_bit_pos == old_bit_pos) {
		ret = PS3_TRUE;
		goto l_out;
	}
	ps3_dma_addr_bit_pos_update(instance, old_bit_pos);
	LOG_WARN("hno:%u bit pos %u change to %u\n",
			PS3_HOST(instance), old_bit_pos, instance->dma_addr_bit_pos);
	ret = PS3_TRUE;
l_out:
	return ret;
}

