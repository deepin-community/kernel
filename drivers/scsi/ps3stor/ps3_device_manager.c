
#ifndef _WINDOWS
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/uio.h>
#include <linux/irq_poll.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/log2.h>
#include <linux/backing-dev.h>
#include <scsi/scsi_tcq.h>

#include <scsi/scsi_host.h>
#include <scsi/scsi_tcq.h>
#endif

#include "ps3_meta.h"
#include "ps3_dev_type.h"
#include "ps3_device_manager.h"
#include "ps3_cmd_statistics.h"
#include "ps3_mgr_cmd.h"
#include "ps3_ioc_manager.h"
#include "ps3_util.h"
#include "ps3_device_update.h"
#include "ps3_module_para.h"
#include "ps3_r1x_write_lock.h"
#include "ps3_qos.h"

static void ps3_nvme_attr_set(const struct ps3_instance *instance,
	struct scsi_device *sdev);

static S32 ps3_dev_channel_init(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	U8 i = 0;
	U16 vd_start = 0;
	U16 pd_start = 0;
	struct PS3ChannelInfo *channel_info = &instance->ctrl_info.channelInfo;
	struct ps3_channel *pd_chan = instance->dev_context.channel_pd;
	struct ps3_channel *vd_chan = instance->dev_context.channel_vd;

	memset(pd_chan, 0,
		sizeof(struct ps3_channel) * PS3_MAX_CHANNEL_NUM);
	memset(vd_chan, 0,
		sizeof(struct ps3_channel) * PS3_MAX_CHANNEL_NUM);

	instance->dev_context.pd_channel_count = 0;
	instance->dev_context.vd_channel_count = 0;
	instance->dev_context.max_dev_per_channel = 0;
	instance->dev_context.abort_vdpending_cmd = 0;

	for (i = 0; i < channel_info->channelNum; i++) {
		LOG_INFO("hno:%u channel[%u] is type %s,max dev num is:%u\n",
			PS3_HOST(instance), i,
			namePS3ChannelType((enum PS3ChannelType)
			channel_info->channels[i].channelType),
			channel_info->channels[i].maxDevNum);

		if (channel_info->channels[i].maxDevNum >
				instance->dev_context.max_dev_per_channel) {
			instance->dev_context.max_dev_per_channel =
				channel_info->channels[i].maxDevNum;
		}

		if (channel_info->channels[i].channelType ==
				PS3_CHAN_TYPE_VD) {
			vd_chan->channel = i;
			vd_chan->max_dev_num =
				channel_info->channels[i].maxDevNum;
			vd_chan->channel_start_num = vd_start;
			vd_start += vd_chan->max_dev_num;
			instance->dev_context.max_dev_in_channel[i] =
				vd_chan->max_dev_num;

			vd_chan++;
			instance->dev_context.vd_channel_count++;
		} else if (channel_info->channels[i].channelType ==
				PS3_CHAN_TYPE_PD) {
			pd_chan->channel = i;
			pd_chan->max_dev_num =
				channel_info->channels[i].maxDevNum;
			pd_chan->channel_start_num = pd_start;
			pd_start += pd_chan->max_dev_num;
			instance->dev_context.max_dev_in_channel[i] =
				pd_chan->max_dev_num;
			pd_chan++;
			instance->dev_context.pd_channel_count++;
		}
	}

	if (instance->dev_context.max_dev_per_channel == 0) {
		LOG_WARN("hno:%u  total dev in channel == 0\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
	}

	return ret;
}

static void ps3_dev_buff_release(struct ps3_instance *instance)
{
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;
	U8 i = 0;

	LOG_INFO("hno:%u release all device mgr buffer\n",
		PS3_HOST(instance));

	if (p_dev_ctx->pd_pool.devs_buffer != NULL) {
		ps3_kfree(instance, p_dev_ctx->pd_pool.devs_buffer);
		p_dev_ctx->pd_pool.devs_buffer = NULL;
	}

	if (p_dev_ctx->vd_pri_data_table.vd_pri_data_idxs_array != NULL) {
		ps3_vfree(instance, p_dev_ctx->vd_pri_data_table.vd_pri_data_idxs_array);
		p_dev_ctx->vd_pri_data_table.vd_pri_data_idxs_array = NULL;
	}

	if (p_dev_ctx->vd_pool.devs_buffer != NULL) {
		ps3_kfree(instance, p_dev_ctx->vd_pool.devs_buffer);
		p_dev_ctx->vd_pool.devs_buffer = NULL;
	}

	if (p_dev_ctx->pd_table.pd_idxs_array != NULL) {
		ps3_kfree(instance, p_dev_ctx->pd_table.pd_idxs_array);
		p_dev_ctx->pd_table.pd_idxs_array = NULL;
	}

	if (p_dev_ctx->pd_entries_array != NULL) {
		ps3_kfree(instance, p_dev_ctx->pd_entries_array);
		p_dev_ctx->pd_entries_array = NULL;
	}

	for (i = 0; i < PS3_VD_TABLE_NUM; i++) {
		if (p_dev_ctx->vd_table[i].vd_idxs_array != NULL) {
			ps3_kfree(instance, p_dev_ctx->vd_table[i].vd_idxs_array);
			p_dev_ctx->vd_table[i].vd_idxs_array = NULL;
		}

		if (p_dev_ctx->vd_entries_array[i] != NULL) {
			ps3_kfree(instance, p_dev_ctx->vd_entries_array[i]);
			p_dev_ctx->vd_entries_array[i] = NULL;
		}
	}
}

static S32 ps3_vd_buff_alloc(struct ps3_instance *instance)
{
	U8 i = 0;
	U8 j = 0;
	struct ps3_channel *p_chan = NULL;
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;
	struct ps3_dev_pool *vd_pool = &p_dev_ctx->vd_pool;
	struct ps3_vd_table *p_vd_table = p_dev_ctx->vd_table;
	struct ps3_pri_data_table *p_vd_pri_data_table = &p_dev_ctx->vd_pri_data_table;

	p_dev_ctx->subwork = 0;
	p_dev_ctx->total_vd_count = 0;
	if (p_dev_ctx->vd_channel_count > 0) {
		p_chan = &p_dev_ctx->channel_vd[p_dev_ctx->vd_channel_count-1];
		p_dev_ctx->total_vd_count = p_chan->channel_start_num +
			p_chan->max_dev_num;
	}

	for (i = 0; i < PS3_VD_TABLE_NUM; i++) {
		p_vd_table[i].vd_idxs_array = (U16*)ps3_kzalloc(instance, sizeof(U16) *
			p_dev_ctx->total_vd_count);
		if (p_vd_table[i].vd_idxs_array == NULL) {
			LOG_ERROR("hno:%u, Failed to allocate VD table %d buffer\n",
				PS3_HOST(instance), i);
			goto free_dev_buff;
		}

		p_dev_ctx->vd_entries_array[i] = (struct PS3VDEntry*)
			ps3_kzalloc(instance, sizeof(struct PS3VDEntry) *
			(PS3_MAX_VD_COUNT(instance) + 1));
		if (p_dev_ctx->vd_entries_array[i] == NULL) {
			LOG_ERROR("hno:%u, Failed to allocate VD entry buffer\n",
				PS3_HOST(instance));
			goto free_dev_buff;
		}
	}

	vd_pool->devs_buffer = (union PS3Device*)ps3_kzalloc(instance,
		sizeof(union PS3Device) * p_dev_ctx->total_vd_count);
	if (vd_pool->devs_buffer == NULL) {
		LOG_ERROR("hno:%u  Failed to allocate VD pool buffer\n",
			PS3_HOST(instance));
		goto free_dev_buff;
	}

	memset(vd_pool->devs_buffer, 0, sizeof(union PS3Device) * p_dev_ctx->total_vd_count);

	p_vd_pri_data_table->vd_pri_data_idxs_array =
		(struct ps3_scsi_priv_data**)ps3_vzalloc(instance,
		sizeof(struct ps3_scsi_priv_data*) * p_dev_ctx->total_vd_count);
	if (p_vd_pri_data_table->vd_pri_data_idxs_array == NULL) {
		LOG_ERROR("hno:%u, Failed to allocate VD R1X table %d buffer\n",
			PS3_HOST(instance), i);
		goto free_dev_buff;
	}

	memset(p_vd_pri_data_table->vd_pri_data_idxs_array, 0,
		sizeof(struct ps3_scsi_priv_data*) * p_dev_ctx->total_vd_count);

	p_chan = p_dev_ctx->channel_vd;
	for (i = 0; i < p_dev_ctx->vd_channel_count; i++) {
		vd_pool->devs[p_chan->channel] =
			&vd_pool->devs_buffer[p_chan->channel_start_num];

		p_vd_pri_data_table->vd_pri_data_idxs[p_chan->channel] =
			&p_vd_pri_data_table->vd_pri_data_idxs_array[p_chan->channel_start_num];

		for (j = 0; j < PS3_VD_TABLE_NUM; j++) {
			p_vd_table[j].vd_idxs[p_chan->channel] =
				&p_vd_table[j].vd_idxs_array[p_chan->channel_start_num];
		}

		p_chan++;
	}

	return PS3_SUCCESS;
free_dev_buff:
	ps3_dev_buff_release(instance);

	return -PS3_ENOMEM;
}

static S32 ps3_pd_buff_alloc(struct ps3_instance *instance)
{
	U8 i = 0;
	struct ps3_channel *p_chan = NULL;
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;
	struct ps3_dev_pool *pd_pool = &p_dev_ctx->pd_pool;
	struct ps3_pd_table *pd_table = &p_dev_ctx->pd_table;

	p_dev_ctx->total_pd_count = 0;
	if (p_dev_ctx->pd_channel_count > 0) {
		p_chan = &p_dev_ctx->channel_pd[p_dev_ctx->pd_channel_count - 1];
		p_dev_ctx->total_pd_count = p_chan->channel_start_num +
			p_chan->max_dev_num;
	}

	p_dev_ctx->pd_entries_array = (struct ps3_pd_entry*)
		ps3_kzalloc(instance, sizeof(struct ps3_pd_entry) *
		(PS3_MAX_PD_COUNT(instance) + 1));
	if (p_dev_ctx->pd_entries_array == NULL) {
		LOG_ERROR("hno:%u, Failed to allocate PD entry buffer\n",
			PS3_HOST(instance));
		goto free_dev_pool;
	}

	pd_table->pd_idxs_array = (U16*)ps3_kzalloc(instance, sizeof(U16) *
		p_dev_ctx->total_pd_count);
	if (pd_table->pd_idxs_array == NULL) {
		LOG_ERROR("hno:%u, Failed to allocate PD table buffer\n",
			PS3_HOST(instance));
		goto free_dev_pool;
	}

	pd_pool->devs_buffer = (union PS3Device*)ps3_kzalloc(instance,
		sizeof(union PS3Device) * p_dev_ctx->total_pd_count);
	if (pd_pool->devs_buffer == NULL) {
		LOG_ERROR("hno:%u, Failed to allocate PD pool buffer\n",
			PS3_HOST(instance));
		goto free_dev_pool;
	}

	p_chan = p_dev_ctx->channel_pd;
	for (i = 0; i < p_dev_ctx->pd_channel_count; i++) {
		pd_pool->devs[p_chan->channel] =
			&pd_pool->devs_buffer[p_chan->channel_start_num];
		pd_table->pd_idxs[p_chan->channel] =
			&pd_table->pd_idxs_array[p_chan->channel_start_num];
		p_chan++;
	}

	return PS3_SUCCESS;
free_dev_pool:
	ps3_dev_buff_release(instance);

	return -PS3_ENOMEM;
}

static S32 ps3_dev_buff_alloc(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;

	LOG_INFO("hno:%u buff alloc pd count %d, vd count %d\n",
		PS3_HOST(instance), PS3_MAX_PD_COUNT(instance), PS3_MAX_VD_COUNT(instance));

	if (PS3_MAX_VD_COUNT(instance) > 0) {
		ret = ps3_vd_buff_alloc(instance);
		if (ret != PS3_SUCCESS) {
			goto l_out;
		}
	}

	if (PS3_MAX_PD_COUNT(instance) > 0) {
		ret = ps3_pd_buff_alloc(instance);
		if (ret != PS3_SUCCESS) {
			goto l_out;
		}
	}

l_out:
	return ret;
}

static inline U16 ps3_pd_dma_alignment_calc(U8 align_shift)
{
	return align_shift ? 1 << align_shift : 0;
}

static inline U8 ps3_pd_type_localed(const struct PS3PDInfo *pd_info)
{
	U8 dev_type = PS3_DEV_TYPE_UNKNOWN;

	dev_type = ps3_get_converted_dev_type(pd_info->driverType, pd_info->mediumType);

	if (unlikely(dev_type == PS3_DEV_TYPE_UNKNOWN)) {
		LOG_ERROR("pd[%u:%u:%u], magic_num[%#x], driver_type[%s], medium_type[%s], dev_type is unknown\n",
			PS3_CHANNEL(&pd_info->diskPos), PS3_TARGET(&pd_info->diskPos),
			PS3_PDID(&pd_info->diskPos), pd_info->diskPos.diskMagicNum,
			getDriverTypeName((DriverType_e)pd_info->driverType),
			getMediumTypeName((MediumType_e)pd_info->mediumType));
		PS3_BUG();
	} else {
		LOG_INFO("pd[%u:%u:%u], magic_num[%#x], driver_type[%s], medium_type[%s], dev_type[%s]\n",
			PS3_CHANNEL(&pd_info->diskPos), PS3_TARGET(&pd_info->diskPos),
			PS3_PDID(&pd_info->diskPos), pd_info->diskPos.diskMagicNum,
			getDriverTypeName((DriverType_e)pd_info->driverType),
			getMediumTypeName((MediumType_e)pd_info->mediumType),
			namePS3DevType((enum PS3DevType)dev_type));
	}
	return dev_type;
}

void ps3_vd_info_show(const struct ps3_instance *instance,
	const struct PS3VDEntry *vd_entry)
{
	U8 i = 0;
	U8 j = 0;
	U8 channel = (U8)PS3_CHANNEL(&vd_entry->diskPos);
	U16 target_id = PS3_TARGET(&vd_entry->diskPos);
	U16 vd_id = PS3_VDID(&vd_entry->diskPos);

	LOG_INFO_IN_IRQ(instance,
		"hno:%u  disk detail info - vd[%u:%u:%u], magicNum[%#x], "
		"dev_type: PS3_DEV_TYPE_VD, isHidden[%s], "
		"accessPolicy[%s], diskGrpId[%u], sectorSize[%u], "
		"stripeDataSize[%u], physDrvCnt[%u], stripSize[%u], "
		"isDirectEnable[%s], raidLevel[%u], spanCount[%u], "
		"diskState[%u], startLBA[%llu], extentSize[%llu], "
		"isTaskMgmtEnable[%u], taskAbortTimeout[%u], "
		"taskResetTimeout[%u], mapBlock[%llu], mapBlockVer[%u], "
		"maxIOSize[%u], devQueDepth[%u], capacity[%llu], isNvme[%u], isSsd[%u], virtDiskSeq[%d], "
		"bdev_bdi_cap[%d], umapBlkDescCnt[%d], umapNumblk[%d], normalQuota[%u], directQuota[%u], "
		"dev_busy_scale[%u]\n"
		, PS3_HOST(instance), channel, target_id, vd_id,
		vd_entry->diskPos.diskMagicNum,
		(vd_entry->isHidden) ? "true" : "false",
		ps3_get_vd_access_plolicy_str((VDAccessPolicy_e)vd_entry->accessPolicy),
		vd_entry->diskGrpId,
		vd_entry->sectorSize, vd_entry->stripeDataSize,
		vd_entry->physDrvCnt, vd_entry->stripSize,
		(vd_entry->isDirectEnable) ? "true" : "false",
		vd_entry->raidLevel,
		vd_entry->spanCount, vd_entry->diskState,
		vd_entry->startLBA, vd_entry->extentSize,
		vd_entry->isTaskMgmtEnable, vd_entry->taskAbortTimeout,
		vd_entry->taskResetTimeout, vd_entry->mapBlock,
		vd_entry->mapBlockVer, vd_entry->maxIOSize,
		vd_entry->devQueDepth, vd_entry->capacity, vd_entry->isNvme,
		vd_entry->isSsd, vd_entry->virtDiskSeq, vd_entry->bdev_bdi_cap,
		vd_entry->umapBlkDescCnt, vd_entry->umapNumblk,
		vd_entry->normalQuota, vd_entry->directQuota,
		vd_entry->dev_busy_scale);

	for (i = 0; i < vd_entry->spanCount; i++) {
		LOG_INFO_IN_IRQ(instance,
			"hno:%u  vd[%u:%u:%u]----span[%u]----"
			"spanStripeDataSize[%u], spanState[%u], spanPdNum[%u]\n"
			, PS3_HOST(instance), channel, target_id, vd_id, i,
			vd_entry->span[i].spanStripeDataSize,
			vd_entry->span[i].spanState,
			vd_entry->span[i].spanPdNum);

		for (j = 0; j < vd_entry->span[i].spanPdNum; j++) {
			LOG_INFO_IN_IRQ(instance,
				"hno:%u  vd[%u:%u:%u]----span[%u]."
				"extent[%u]----pd:[%u:%u:%u], state[%u]\n"
				, PS3_HOST(instance), channel, target_id, vd_id, i, j,
				vd_entry->span[i].extent[j].phyDiskID.ps3Dev.softChan,
				vd_entry->span[i].extent[j].phyDiskID.ps3Dev.devID,
				vd_entry->span[i].extent[j].phyDiskID.ps3Dev.phyDiskID,
				vd_entry->span[i].extent[j].state);
		}
	}
}

static void ps3_pd_info_show(const struct ps3_instance *instance,
	const struct ps3_pd_entry *pd_entry)
{
	LOG_INFO("hno:%u  disk detail info - pd[%u:%u:%u], magicNum[%#x], "
		"state[%s], dev_type[%s], config_flag[%s], pd_flags[0x%02x]"
		"RWCT[%u], scsi_interface_type[%u], "
		"task_abort_timeout[%u], task_reset_timeout[%u], "
		"max_io_size[%u], dev_queue_depth[%u], "
		"sector_size[%u], encl_id[%u], pd phy_id[%u], "
		"dma_addr_alignment[%u], dma_len_alignment[%u]"
		"normal_quota[%u] direct_quota[%u]\n"
		, PS3_HOST(instance),
		PS3_CHANNEL(&pd_entry->disk_pos),
		PS3_TARGET(&pd_entry->disk_pos),
		PS3_PDID(&pd_entry->disk_pos),
		pd_entry->disk_pos.diskMagicNum,
		getDeviceStateName((DeviceState_e)pd_entry->state),
		namePS3DevType((enum PS3DevType)pd_entry->dev_type),
		getPdStateName((MicPdState_e)pd_entry->config_flag, instance->is_raid), pd_entry->pd_flags,
		pd_entry->RWCT, pd_entry->scsi_interface_type,
		pd_entry->task_abort_timeout, pd_entry->task_reset_timeout,
		pd_entry->max_io_size, pd_entry->dev_queue_depth,
		pd_entry->sector_size, pd_entry->encl_id,
		pd_entry->phy_id, pd_entry->dma_addr_alignment,
		pd_entry->dma_len_alignment,
		pd_entry->normal_quota, pd_entry->direct_quota
	);
}

static inline void ps3_pd_info_localed(const struct ps3_instance *instance,
	struct ps3_pd_entry *local_entry, const struct PS3PDInfo *pd_info)
{
	local_entry->disk_pos = pd_info->diskPos;
	local_entry->dev_type = ps3_pd_type_localed(pd_info);
	local_entry->scsi_interface_type = pd_info->scsiInterfaceType;
	local_entry->state = pd_info->diskState;
	local_entry->task_abort_timeout = pd_info->taskAbortTimeout;
	local_entry->task_reset_timeout = pd_info->taskResetTimeout;
	local_entry->config_flag = pd_info->configFlag;
	local_entry->pd_flags = pd_info->pdFlags;
	local_entry->max_io_size = pd_info->maxIOSize;
	local_entry->dev_queue_depth = pd_info->devQueDepth;
	local_entry->sector_size = pd_info->sectorSize;
	local_entry->encl_id = pd_info->enclId;
	local_entry->phy_id = pd_info->phyId;
	local_entry->dma_addr_alignment = ps3_pd_dma_alignment_calc(pd_info->dmaAddrAlignShift);
	local_entry->dma_len_alignment = ps3_pd_dma_alignment_calc(pd_info->dmaLenAlignShift);
	local_entry->normal_quota = pd_info->normalQuota;
	local_entry->direct_quota = pd_info->directQuota;

	if (unlikely(local_entry->sector_size == 0)) {
		LOG_WARN("pd[%u:%u] sector_size is 0\n",
			PS3_CHANNEL(&local_entry->disk_pos),
			PS3_TARGET(&local_entry->disk_pos));
	}

	ps3_pd_info_show(instance, local_entry);
}

S32 ps3_dev_mgr_pd_info_get(struct ps3_instance *instance, U16 channel,
	U16 target_id, U16 pd_id)
{
	S32 ret = PS3_SUCCESS;
	struct PS3PDInfo *pd_info = NULL;
	struct ps3_dev_context *dev_ctx = &instance->dev_context;

	LOG_DEBUG("hno:%u, get PD info [%u:%u:%u] start\n",
		PS3_HOST(instance), channel, target_id, pd_id);

	ret = ps3_pd_info_get(instance, channel, target_id, pd_id);
	INJECT_START(PS3_ERR_IJ_FORCE_RET_FAIL6, &ret);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u, get single PD info [%u:%u:%u] NOK\n",
			PS3_HOST(instance), channel, target_id, pd_id);
		goto l_out;
	}

	pd_info = dev_ctx->pd_info_buf;
	INJECT_START(PS3_ERR_IJ_FORCE_MOD_DISKPOS1, &pd_info->diskPos.diskDev.diskID);
	if (PS3_DEV_INVALID(pd_info->diskPos)) {
		LOG_WARN("hno:%u, cannot found PD info [%u:%u:%u]\n",
			PS3_HOST(instance), channel, target_id, pd_id);
		ret = -PS3_FAILED;
		goto l_out;
	}

	INJECT_START(PS3_ERR_IJ_FORCE_MOD_ALIGN, &pd_info->dmaAddrAlignShift);
	if (pd_info->dmaAddrAlignShift >
			PS3_DMA_ALIGN_SHIFT_MAX) {
		LOG_WARN("hno:%u PD info [%u:%u:%u] invalid dmaAddrAlignShift\n",
			PS3_HOST(instance), channel, target_id, PS3_PDID(&pd_info->diskPos));
		ret = -PS3_FAILED;
		goto l_out;
	}
	INJECT_START(PS3_ERR_IJ_FORCE_MOD_ALIGN1, &pd_info->dmaLenAlignShift);
	if (pd_info->dmaLenAlignShift >
			PS3_DMA_ALIGN_SHIFT_MAX) {
		LOG_WARN("hno:%u PD info [%u:%u:%u] invalid dmalenAlignShift\n",
			PS3_HOST(instance), channel, target_id, PS3_PDID(&pd_info->diskPos));
		ret = -PS3_FAILED;
		goto l_out;
	}
	INJECT_START(PS3_ERR_IJ_FORCE_MOD_CHAN2, &channel);
	if (unlikely(PS3_CHANNEL(&pd_info->diskPos) != channel ||
			PS3_TARGET(&pd_info->diskPos) != target_id)) {
		LOG_ERROR("hno:%u PD info get [%u:%u:%u]!=[%u:%u:%u] magic[%#x] unmatched\n",
			PS3_HOST(instance), channel, target_id, pd_id,
			PS3_CHANNEL(&pd_info->diskPos),
			PS3_TARGET(&pd_info->diskPos),
			PS3_PDID(&pd_info->diskPos),
			pd_info->diskPos.diskMagicNum);
		PS3_BUG();
		ret = -PS3_FAILED;
		goto l_out;
	}

	INJECT_START(PS3_ERR_IJ_FORCE_MOD_DISKID, &PS3_PDID(&pd_info->diskPos));
	if (unlikely(PS3_PDID(&pd_info->diskPos) > PS3_MAX_PD_COUNT(instance))) {
		LOG_ERROR("hno:%u  init pd info NOK, pd_id[%d] > max[%d]\n",
			PS3_HOST(instance),
			PS3_PDID(&pd_info->diskPos), PS3_MAX_PD_COUNT(instance));

		PS3_BUG();
		ret = -PS3_FAILED;
		goto l_out;
	}

	ps3_pd_info_localed(instance, &dev_ctx->pd_entries_array[
		PS3_PDID(&pd_info->diskPos)], pd_info);
	dev_ctx->pd_table.pd_idxs[channel][target_id] =
		PS3_PDID(&pd_info->diskPos);
l_out:
	return ret;
}

static void ps3_pd_info_get_all(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	U16 i = 0;
	U16 j = 0;
	U8 chan_id = 0;

	struct ps3_channel *pd_chan = instance->dev_context.channel_pd;
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;
	struct ps3_pd_table *p_table = &p_dev_ctx->pd_table;
	struct ps3_dev_pool *p_pd_pool = &p_dev_ctx->pd_pool;

	memset(p_table->pd_idxs_array, PS3_INVALID_VALUE,
		p_dev_ctx->total_pd_count * sizeof(U16));

	for (i = 0; i < p_dev_ctx->pd_channel_count; i++) {
		chan_id = pd_chan[i].channel;
		for (j = 0; j < pd_chan[i].max_dev_num; j++) {
			if (PS3_DEV_INVALID(p_pd_pool->devs[chan_id][j].pd.diskPos)) {
				continue;
			}

			ret = ps3_dev_mgr_pd_info_get(instance, chan_id, j,
				PS3_PDID(&p_pd_pool->devs[chan_id][j].pd.diskPos));
			if (ret != PS3_SUCCESS) {
				LOG_ERROR("hno:%u, get PD[%u:%u] info NOK\n",
					PS3_HOST(instance), chan_id, j);
			}
		}
	}
}

S32 ps3_dev_mgr_vd_info_subscribe(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	LOG_FILE_INFO("hno:%u ps3_dev_mgr_vd_info_subscribe\n", PS3_HOST(instance));

	if (PS3_MAX_VD_COUNT(instance) <= 0) {
		goto l_out;
	}

	ret = ps3_vd_info_async_get(instance);
	if (ret != PS3_SUCCESS) {
		LOG_FILE_ERROR("hno:%u  async get VD info failed\n",
			PS3_HOST(instance));
	}
l_out:
	return ret;
}

S32 ps3_dev_mgr_vd_info_unsubscribe(struct ps3_instance *instance)
{
	ULong flags = 0;
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd *abort_cmd = NULL;
	if (!ps3_check_ioc_state_is_normal_in_unload(instance)) {
		ret = -PS3_FAILED;
		goto l_out;
	}

	cmd = instance->dev_context.vd_pending_cmd;
	if (cmd == NULL) {
		LOG_WARN("hno:%u  vd pending cmd has been cancel\n",
			PS3_HOST(instance));
		ret = PS3_SUCCESS;
		goto l_out;
	}

	ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
	if (cmd->cmd_state.state == PS3_CMD_STATE_INIT) {
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
		LOG_ERROR("hno:%u who steal free this cmd,CFID:%d\n",
			PS3_HOST(instance), cmd->index);
		instance->dev_context.vd_pending_cmd = NULL;
		goto l_out;
	} else {
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
	}

    if (ps3_atomic_add_unless(&instance->dev_context.is_vdpending_abort, 1, 1) == 0) {
        ret = PS3_SUCCESS;
        goto l_out;
    }

	ret = ps3_mgr_cmd_cancel_send(instance, cmd->index, PS3_CANCEL_VDPENDING_CMD);
	INJECT_START(PS3_ERR_IJ_CANCEL_VDPENDING_CMD_FAIL, &ret)
	if(ret == -PS3_ENOMEM){
		LOG_INFO("hno:%u  alloc failed\n",PS3_HOST(instance));
		ret = PS3_FAILED;
        ps3_atomic_set(&instance->dev_context.is_vdpending_abort, 0);
		goto l_out;
	}else if(ret != PS3_SUCCESS){
		LOG_INFO("hno:%u reqFrameId=%d cancel_cmd_frame_id[%u] free!\n",
			PS3_HOST(instance), ps3_cmd_frame_id(instance->dev_context.vdpending_abort_cmd), cmd->index);
		abort_cmd = instance->dev_context.vdpending_abort_cmd;
		instance->dev_context.vdpending_abort_cmd = NULL;
		if (abort_cmd != NULL) {
			ps3_task_cmd_free(instance, abort_cmd);
		}
		ret = PS3_FAILED;
        ps3_atomic_set(&instance->dev_context.is_vdpending_abort, 0);
		goto l_out;
	}

	ret = ps3_mgr_cmd_cancel_wait(instance, PS3_CANCEL_VDPENDING_CMD);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  unsubscribe vd pending, cancel cmd NOK\n",
			PS3_HOST(instance));
        ps3_atomic_set(&instance->dev_context.is_vdpending_abort, 0);
		goto l_out;
	}

	LOG_INFO("hno:%u  vd pending cmd free, CFID:%d\n",
		PS3_HOST(instance), cmd->index);
	instance->dev_context.vd_pending_cmd = NULL;
	ps3_mgr_cmd_free(instance, cmd);
    ps3_atomic_set(&instance->dev_context.is_vdpending_abort, 0);

l_out:
	return ret;
}
S32 ps3_dev_mgr_vd_info_resubscribe(struct ps3_instance *instance)
{
	ULong flags = 0;
	S32 ret = PS3_SUCCESS;
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd *abort_cmd = NULL;
	ULong flags1 = 0;
	Bool is_need_resend = PS3_FALSE;
	if (!ps3_check_ioc_state_is_normal_in_unload(instance)) {
		ret = -PS3_FAILED;
		goto l_out;
	}

	cmd = instance->dev_context.vd_pending_cmd;
	if (cmd == NULL) {
		LOG_WARN("hno:%u  vd pending cmd has been cancel\n",
			PS3_HOST(instance));
		is_need_resend = PS3_TRUE;
		ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags1);
		goto l_resend;
	}

	ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
	if (cmd->cmd_state.state == PS3_CMD_STATE_INIT) {
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
		LOG_ERROR("hno:%u free vdpending cmd,CFID:%d\n",
			PS3_HOST(instance), cmd->index);
		instance->dev_context.vd_pending_cmd = NULL;
		goto l_out;
	} else {
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
	}

    if (ps3_atomic_add_unless(&instance->dev_context.is_vdpending_abort, 1, 1) == 0) {
        ret = PS3_SUCCESS;
        goto l_out;
    }

	ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags1);
	ret = ps3_mgr_cmd_cancel_send(instance, cmd->index, PS3_CANCEL_VDPENDING_CMD);
	if(ret == -PS3_ENOMEM){
		ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
		LOG_INFO("hno:%u  alloc failed\n",PS3_HOST(instance));
		ret = -PS3_FAILED;
        ps3_atomic_set(&instance->dev_context.is_vdpending_abort, 0);
		goto l_out;
	}else if(ret != PS3_SUCCESS){
		LOG_FILE_INFO("hno:%u reqFrameId=%d cancel_cmd_frame_id[%u] free!\n",
			PS3_HOST(instance), ps3_cmd_frame_id(instance->dev_context.vdpending_abort_cmd), cmd->index);
		abort_cmd = instance->dev_context.vdpending_abort_cmd;
		instance->dev_context.vdpending_abort_cmd = NULL;
		if (abort_cmd != NULL) {
			ps3_task_cmd_free(instance, abort_cmd);
		}
		ret = -PS3_FAILED;
	    ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
        ps3_atomic_set(&instance->dev_context.is_vdpending_abort, 0);
		goto l_out;
	}
	instance->dev_context.abort_vdpending_cmd = 1;
	ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);

	ret = ps3_mgr_cmd_cancel_wait(instance, PS3_CANCEL_VDPENDING_CMD);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  unsubscribe vd pending, cancel cmd NOK\n",
			PS3_HOST(instance));
        ps3_atomic_set(&instance->dev_context.is_vdpending_abort, 0);
		goto l_out;
	}
	ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags1);
	if(instance->dev_context.subwork == 0){
		if(instance->dev_context.abort_vdpending_cmd != 0){
			instance->dev_context.abort_vdpending_cmd = 0;
			LOG_FILE_INFO("hno:%u  vd pending cmd free, CFID:%d\n",
				PS3_HOST(instance), cmd->index);
			instance->dev_context.vd_pending_cmd = NULL;
			ps3_mgr_cmd_free(instance, cmd);
			is_need_resend = PS3_TRUE;
		}
	}
    ps3_atomic_set(&instance->dev_context.is_vdpending_abort, 0);
l_resend:
	if (is_need_resend) {
		ret = ps3_dev_mgr_vd_info_subscribe(instance);
		if (ret != PS3_SUCCESS) {
			ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
			goto l_out;
		}
	}

	ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);

l_out:
	return ret;
}

void ps3_dev_mgr_vd_info_clear(struct ps3_instance *instance)
{
	ULong flags = 0;
	ULong flags1 = 0;
	struct ps3_cmd *cmd = NULL;
	instance->dev_context.abort_vdpending_cmd = 0;

	cmd = instance->dev_context.vd_pending_cmd;

	if (cmd == NULL) {
		LOG_WARN("hno:%u  vd pending cmd has been cancel\n",
			PS3_HOST(instance));
		goto l_out;
	}

	ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
	if (cmd->cmd_state.state == PS3_CMD_STATE_INIT) {
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
		LOG_INFO("hno:%u   free this cmd,CFID:%d\n",
			PS3_HOST(instance), cmd->index);
		ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags1);
		instance->dev_context.vd_pending_cmd = NULL;
		ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
		goto l_out;
	} else {
		ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
	}

	LOG_INFO("hno:%u  vd pending cmd free, CFID:%d\n",
		PS3_HOST(instance), cmd->index);
	instance->dev_context.vd_pending_cmd = NULL;
	instance->dev_context.subwork = 0;

	ps3_mgr_cmd_free(instance, cmd);

l_out:
	return;
}

#ifndef _WINDOWS 
void ps3_change_sdev_max_sector(struct ps3_instance *instance,
	struct PS3VDEntry *vd_entry)
{
	struct scsi_device *sdev = NULL;
	struct PS3Dev *p_vd = &vd_entry->diskPos.diskDev.ps3Dev;

	sdev = ps3_scsi_device_lookup(instance, p_vd->softChan,
		p_vd->devID, 0);
	if (sdev != NULL) {
		if (vd_entry->sectorSize == PS3_SECTORSIZE_512B) {
			blk_queue_max_hw_sectors(sdev->request_queue, vd_entry->maxIOSize);
		} else {
			blk_queue_max_hw_sectors(sdev->request_queue,
				vd_entry->maxIOSize << (ilog2(vd_entry->sectorSize) - PS3_512B_SHIFT));
		}

		LOG_INFO_IN_IRQ(instance,
			"hno:%u vd[%u:%u] max sector num change to:%d\n",
			PS3_HOST(instance), p_vd->softChan, p_vd->devID,
			vd_entry->maxIOSize);

		ps3_scsi_device_put(instance, sdev);
	}
}
#endif
S32 ps3_vd_info_get_all(struct ps3_instance *instance)
{
	S32 ret = -PS3_FAILED;
	U16 i = 0;
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;
	U8 vd_table_idx = (p_dev_ctx->vd_table_idx + 1) & 1;
	struct ps3_vd_table *p_vd_table =
		&p_dev_ctx->vd_table[vd_table_idx];
	struct PS3VDEntry *p_vd_array =
		p_dev_ctx->vd_entries_array[vd_table_idx];
	struct PS3VDEntry *p_vd_entry =
		p_dev_ctx->vd_info_buf_sync->vds;
	struct PS3Dev *p_dev = NULL;
	U16 virtDiskIdx = PS3_INVALID_DEV_ID;

	if (PS3_MAX_VD_COUNT(instance) == 0) {
		LOG_INFO("hno:%u vd max count is %d\n",
			PS3_HOST(instance), PS3_MAX_VD_COUNT(instance));
		ret = PS3_SUCCESS;
		goto l_out;
	}

	if (p_dev_ctx->vd_list_buf->count == 0) {
		LOG_INFO("hno:%u vd list count is 0\n",
			PS3_HOST(instance));
		ret = PS3_SUCCESS;
		goto l_out;
	}

	ret = ps3_vd_info_sync_get(instance, 0, PS3_MAX_VD_COUNT(instance));
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u, sync get VD info NOK\n",
			PS3_HOST(instance));
		goto l_out;
	}

	memset(p_vd_table->vd_idxs_array, 0,
		p_dev_ctx->total_vd_count * sizeof(U16));

	LOG_INFO("hno:%u get vd info count is %d\n",
		PS3_HOST(instance),p_dev_ctx->vd_info_buf_sync->count);

	for (i = 0; i < p_dev_ctx->vd_info_buf_sync->count; i++) {
		if (PS3_DEV_INVALID(p_vd_entry[i].diskPos)) {
			LOG_WARN("hno:%u, init %d of %d vd info NOK, disk_id is 0\n",
				PS3_HOST(instance), i,
				p_dev_ctx->vd_info_buf_sync->count);
			continue;
		}

		p_dev = PS3_DEV(&p_vd_entry[i].diskPos);

		virtDiskIdx = get_offset_of_vdid(PS3_VDID_OFFSET(instance), p_dev->virtDiskID);
		if (!ps3_dev_id_valid_check(instance, (U8)p_dev->softChan,
				p_dev->devID, PS3_DISK_TYPE_VD)) {
			PS3_BUG();
			continue;
		}

		if (unlikely(virtDiskIdx > PS3_MAX_VD_COUNT(instance))) {
			LOG_ERROR("hno:%u  init %d of %d vd info NOK, vir_id[%d] > max[%d]\n",
				PS3_HOST(instance),i,
				p_dev_ctx->vd_info_buf_sync->count-1,
				virtDiskIdx, PS3_MAX_VD_COUNT(instance));

			PS3_BUG();
			continue;
		}
		ps3_vd_busy_scale_get(&p_vd_entry[i]);
		memcpy(&p_vd_array[virtDiskIdx], &p_vd_entry[i],
			sizeof(struct PS3VDEntry));
		p_vd_table->vd_idxs[p_dev->softChan][p_dev->devID] = p_dev->virtDiskID;

		ps3_vd_info_show(instance, &p_vd_array[virtDiskIdx]);
#ifndef _WINDOWS 
		if (p_vd_entry[i].maxIOSize != 0) {
			ps3_change_sdev_max_sector(instance, &p_vd_entry[i]);
		}
#endif
	}
	mb();
	p_dev_ctx->vd_table_idx = vd_table_idx;
	mb();
l_out:
	return ret;
}

S32 ps3_device_mgr_data_init(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;

	LOG_DEBUG("hno:%u enter\n",
		PS3_HOST(instance));

	if (PS3_MAX_PD_COUNT(instance) != 0) {
		ret = ps3_dev_mgr_pd_list_get(instance);
		if (ret != PS3_SUCCESS) {
			goto l_out;
		}

		ps3_pd_info_get_all(instance);
	}

	if (PS3_MAX_VD_COUNT(instance) != 0) {
		ret = ps3_dev_mgr_vd_list_get(instance);
		if (ret != PS3_SUCCESS) {
			goto l_out;
		}
		instance->dev_context.vd_table_idx = 0;
		ps3_vd_info_get_all(instance);
	}
l_out:
	LOG_DEBUG("hno:%u out\n",
		PS3_HOST(instance));

	return ret ;
}

S32 ps3_device_mgr_data_exit(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;

	LOG_DEBUG("hno:%u \n", PS3_HOST(instance));

	return ret;
}

S32 ps3_device_mgr_init(struct ps3_instance *instance)
{
	S32 ret = -PS3_FAILED;

	if (PS3_MAX_VD_COUNT(instance) == 0 &&
			PS3_MAX_PD_COUNT(instance) == 0) {
		LOG_ERROR("hno:%u  max VD and PD count == 0\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}

	ret = ps3_dev_channel_init(instance);
	if (ret != PS3_SUCCESS) {
		goto l_out;
	}

	ret = ps3_dev_buff_alloc(instance);
	if (ret != PS3_SUCCESS) {
		goto l_out;
	}
	ps3_mutex_init(&instance->dev_context.dev_priv_lock);
	ps3_mutex_init(&instance->dev_context.dev_scan_lock);
#ifdef _WINDOWS
	ps3_windows_channel_map_init(instance);
	ret = ps3_windows_private_init(instance);
	if (ret != PS3_SUCCESS) {
		goto l_out;
	}
#endif

l_out:
	return ret ;
}

void ps3_device_mgr_exit(struct ps3_instance *instance)
{
#ifdef _WINDOWS
	ps3_windows_private_exit(instance);
#endif
	ps3_dev_buff_release(instance);
	ps3_mutex_destroy(&instance->dev_context.dev_priv_lock);
	ps3_mutex_destroy(&instance->dev_context.dev_scan_lock);
}

S32 ps3_dev_mgr_pd_list_get(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	U32 i = 0;
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;
	struct PS3DevList *p_pd_list = p_dev_ctx->pd_list_buf;
	struct PS3Dev *p_dev = NULL;

	ret = ps3_pd_list_get(instance);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u, dev mgr get pd list NOK\n",
			PS3_HOST(instance));
		goto l_out;
	}

	memset((U8*)p_dev_ctx->pd_pool.devs_buffer, PS3_INVALID_VALUE,
		p_dev_ctx->total_pd_count * sizeof(union PS3Device));

	LOG_INFO("hno:%u get pd list count is %d\n",
		PS3_HOST(instance), p_pd_list->count);

	for (i = 0; i < p_pd_list->count; i++) {
		p_dev = PS3_DEV(&p_pd_list->devs[i].pd.diskPos);

		if (PS3_DEV_INVALID(p_pd_list->devs[i].pd.diskPos)) {
			LOG_WARN("hno:%u, get pd list %d dev disk_id is 0\n",
				PS3_HOST(instance), i);
			continue;
		}

		if (!ps3_dev_id_valid_check(instance, (U8)p_dev->softChan,
				p_dev->devID, PS3_DISK_TYPE_PD)) {
			PS3_BUG();
			continue;
		}

		LOG_INFO("hno:%u, pd list %d dev[%u:%u:%u], magic[%#x], state[%s]\n",
			PS3_HOST(instance), i, p_dev->softChan, p_dev->devID,
			p_dev->phyDiskID,
			p_pd_list->devs[i].pd.diskPos.diskMagicNum,
			getDeviceStateName((DeviceState_e)p_pd_list->devs[i].pd.diskState));

		p_dev_ctx->pd_pool.devs[p_dev->softChan][p_dev->devID].pd =
			p_pd_list->devs[i].pd;

	}
l_out:
	return ret;
}

S32 ps3_dev_mgr_vd_list_get(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	U32 i = 0;
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;
	struct PS3DevList *p_vd_list = p_dev_ctx->vd_list_buf;
	struct PS3Dev *p_dev = NULL;

	ret = ps3_vd_list_get(instance);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u, dev mgr get vd list NOK\n",
			PS3_HOST(instance));
		goto l_out;
	}

	memset((U8*)p_dev_ctx->vd_pool.devs_buffer, PS3_INVALID_VALUE,
		p_dev_ctx->total_vd_count * sizeof(union PS3Device));

	LOG_INFO("hno:%u get vd list count is %d\n",
		PS3_HOST(instance), p_vd_list->count);

	for (i = 0; i < p_vd_list->count; i++) {
		p_dev = PS3_DEV(&p_vd_list->devs[i].vd.diskPos);

		if (PS3_DEV_INVALID(p_vd_list->devs[i].vd.diskPos)) {
			LOG_WARN("hno:%u, get vd list %d dev disk_id is 0\n",
				PS3_HOST(instance), i);
			continue;
		}

		if (!ps3_dev_id_valid_check(instance, (U8)p_dev->softChan,
				p_dev->devID, PS3_DISK_TYPE_VD)) {
			PS3_BUG();
			continue;
		}

		LOG_INFO("hno:%u, vd list %d dev[%u:%u:%u], magic[%#x]\n",
			PS3_HOST(instance), i, p_dev->softChan, p_dev->devID,
			p_dev->virtDiskID,
			p_vd_list->devs[i].vd.diskPos.diskMagicNum);

		p_dev_ctx->vd_pool.devs[p_dev->softChan][p_dev->devID].vd =
			p_vd_list->devs[i].vd;
	}
l_out:
	return ret;
}

Bool ps3_dev_id_valid_check(struct ps3_instance *instance, U8 channel,
	U16 target_id, U8 dev_type)
{
	U8 ret = PS3_DRV_FALSE;
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;

	if (dev_type == PS3_DISK_TYPE_VD) {
		INJECT_START(PS3_ERR_IJ_FORCE_MOD_CHAN1, &channel);
		if (!PS3_IS_VD_CHANNEL(instance, channel)) {
			LOG_ERROR_IN_IRQ(instance,
				"hno:%u  check channel[%u] is not vd channel\n",
				PS3_HOST(instance), channel);
			goto l_out;
		}

	} else if (dev_type == PS3_DISK_TYPE_PD) {
		INJECT_START(PS3_ERR_IJ_FORCE_MOD_CHAN3, &channel);
		if (!PS3_IS_PD_CHANNEL(instance, channel)) {
			LOG_ERROR_IN_IRQ(instance,
				"hno:%u  check channel[%u] is not pd channel\n",
				PS3_HOST(instance), channel);
			goto l_out;
		}
	} else {
		LOG_ERROR_IN_IRQ(instance,
			"hno:%u dev id[%u:%u] channel err\n",
			PS3_HOST(instance), channel, target_id);
		goto l_out;
	}
	INJECT_START(PS3_ERR_IJ_FORCE_MOD_TARGET, &target_id);
	if (unlikely(target_id >= p_dev_ctx->max_dev_in_channel[channel])) {
		LOG_ERROR_IN_IRQ(instance,
			"hno:%u  check disk[%u:%u] target >= max[%u]\n",
			PS3_HOST(instance), channel, target_id,
			p_dev_ctx->max_dev_in_channel[channel]);
		goto l_out;
	}

	ret = PS3_DRV_TRUE;
l_out:
	return ret;
}

U8 ps3_get_vd_raid_level(struct ps3_instance *instance,
	U8 channel, U16 target_id)
{
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;
	U8 vd_table_idx = p_dev_ctx->vd_table_idx & 1;
	struct ps3_vd_table *p_table =
		&p_dev_ctx->vd_table[vd_table_idx];
	struct PS3VDEntry *p_vd_array =
		p_dev_ctx->vd_entries_array[vd_table_idx];
	struct PS3VDEntry *p_entry = NULL;
	U8 ret = RAID_UNKNOWN;
	U16 virtDiskIdx = PS3_INVALID_DEV_ID;


	if (!ps3_dev_id_valid_check(instance, channel, target_id,
			PS3_DISK_TYPE_VD)) {
		goto l_out;
	}

	virtDiskIdx = get_offset_of_vdid(PS3_VDID_OFFSET(instance), p_table->vd_idxs[channel][target_id]);

	p_entry = &p_vd_array[virtDiskIdx];
	if (p_entry == NULL) {
		goto l_out;
	}

	LOG_DEBUG("hno:%u, vd[%u:%u] raid level is:%d\n",
		PS3_HOST(instance), channel, target_id, p_entry->raidLevel);

	ret = p_entry->raidLevel;
l_out:
	return ret;
}

struct ps3_scsi_priv_data *ps3_dev_mgr_lookup_vd_pri_data(
	struct ps3_instance *instance, U8 channel, U16 target_id)
{
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;
	struct ps3_scsi_priv_data *ret = NULL;

	if (!ps3_dev_id_valid_check(instance, channel, target_id,
			PS3_DISK_TYPE_VD)) {
		goto l_out;
	}

	ret = p_dev_ctx->vd_pri_data_table.vd_pri_data_idxs[channel]
		[target_id];
l_out:
	return ret;
}

static inline Bool ps3_dev_is_valid(struct PS3DiskDevPos *diskPos, U8 channel, U16 target_id)
{
	if (PS3_DEV_INVALID(*diskPos) || PS3_CHANNEL(diskPos) != channel || PS3_TARGET(diskPos) != target_id) {
		return PS3_FALSE;
	}
	return PS3_TRUE;
}

struct PS3VDEntry *ps3_dev_mgr_lookup_vd_info(
	struct ps3_instance *instance, U8 channel, U16 target_id)
{
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;
	U8 vd_table_idx = p_dev_ctx->vd_table_idx & 1;
	struct ps3_vd_table *p_table =
		&p_dev_ctx->vd_table[vd_table_idx];
	struct PS3VDEntry *p_vd_array =
		p_dev_ctx->vd_entries_array[vd_table_idx];
	struct PS3VDEntry *p_entry = NULL;
	U16 virtDiskIdx = PS3_INVALID_DEV_ID;

	LOG_DEBUG("hno:%u  cur_vd_idx[%d]\n",
		PS3_HOST(instance), p_dev_ctx->vd_table_idx);


	if (!ps3_dev_id_valid_check(instance, channel, target_id,
			PS3_DISK_TYPE_VD)) {
		goto l_out;
	}

	virtDiskIdx = get_offset_of_vdid(PS3_VDID_OFFSET(instance), p_table->vd_idxs[channel][target_id]);

	p_entry = &p_vd_array[virtDiskIdx];

	INJECT_START(PS3_ERR_IJ_FORCE_MOD_DISKPOS, &p_entry->diskPos.diskDev.diskID);
	if (ps3_dev_is_valid(&p_entry->diskPos, channel, target_id) != PS3_TRUE) {
		p_entry = NULL;
		goto l_out;
	}
l_out:
	return p_entry;
}

struct ps3_pd_entry *ps3_dev_mgr_lookup_pd_info(
	struct ps3_instance *instance, U8 channel, U16 target_id)
{
	struct ps3_pd_table *p_table = &instance->dev_context.pd_table;
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;
	struct ps3_pd_entry *p_entry = NULL;
	U16 disk_idx = 0;

	if (!ps3_dev_id_valid_check(instance, channel, target_id,
			PS3_DISK_TYPE_PD)) {
		goto l_out;
	}

	disk_idx = p_table->pd_idxs[channel][target_id];
	p_entry = &p_dev_ctx->pd_entries_array[disk_idx];

	INJECT_START(PS3_ERR_IJ_FORCE_MOD_DISKPOS, &p_entry->disk_pos.diskDev.diskID);
	if (ps3_dev_is_valid(&p_entry->disk_pos, channel, target_id) != PS3_TRUE) {
		p_entry = NULL;
		goto l_out;
	}
l_out:
	return p_entry;
}

struct PS3VDEntry *ps3_dev_mgr_lookup_vd_info_by_id(
	struct ps3_instance *instance, U16 disk_id)
{
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;
	U8 vd_table_idx = p_dev_ctx->vd_table_idx & 1;
	struct PS3VDEntry *p_vd_array =
		p_dev_ctx->vd_entries_array[vd_table_idx];
	struct PS3VDEntry *p_entry = NULL;
	U16 virtDiskIdx = PS3_INVALID_DEV_ID;

	LOG_DEBUG("hno:%u  cur_vd_idx[%d]\n",
		PS3_HOST(instance), p_dev_ctx->vd_table_idx);

	virtDiskIdx = get_offset_of_vdid(PS3_VDID_OFFSET(instance), disk_id);

	if (unlikely(virtDiskIdx > PS3_MAX_VD_COUNT(instance))) {
		LOG_ERROR_LIM("hno:%u, dev mgr lookup vd info disk id > max count:%d>%d\n",
			PS3_HOST(instance), disk_id,
			PS3_MAX_VD_COUNT(instance));

		PS3_BUG();
		goto l_out;
	}

	p_entry = &p_vd_array[virtDiskIdx];

	if (PS3_DEV_INVALID(p_entry->diskPos)) {
		LOG_INFO_LIM("hno:%u  idx[%d], virDisk[%d] dev id is invalid\n",
			PS3_HOST(instance), p_dev_ctx->vd_table_idx, disk_id);
		p_entry = NULL;
		goto l_out;
	}
l_out:
	return p_entry;
}

struct ps3_pd_entry *ps3_dev_mgr_lookup_pd_info_by_id(
	struct ps3_instance *instance, U16 disk_id)
{
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;
	struct ps3_pd_entry *p_entry = NULL;

	if (unlikely(disk_id > PS3_MAX_PD_COUNT(instance))) {
		LOG_ERROR_LIM("hno:%u, dev mgr lookup pd info disk id > max count:%d>%d\n",
			PS3_HOST(instance), disk_id,
			PS3_MAX_PD_COUNT(instance));
		PS3_BUG();
		goto l_out;
	}

	p_entry = &p_dev_ctx->pd_entries_array[disk_id];

	if (PS3_DEV_INVALID(p_entry->disk_pos) ||
			p_entry->config_flag == MIC_PD_STATE_UNKNOWN) {
		LOG_INFO_LIM("hno:%u  pdid[%d] dev[%x] id is invalid, config_flag[%d]\n",
			PS3_HOST(instance), disk_id,
			PS3_DISKID(&p_entry->disk_pos),
			p_entry->config_flag);
		p_entry = NULL;
		goto l_out;
	}
l_out:
	return p_entry;
}

union PS3Device* ps3_dev_mgr_lookup_vd_list(
	struct ps3_instance *instance, U8 channel, U16 target_id)
{
	struct ps3_dev_pool *p_vd_pool = &instance->dev_context.vd_pool;
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;
	union PS3Device *p_vd = NULL;

	if (!ps3_dev_id_valid_check(instance, channel, target_id,
			PS3_DISK_TYPE_VD)) {
		goto l_out;
	}

	p_vd = &p_vd_pool->devs[channel][target_id];

	if (PS3_DEV_INVALID(p_vd->vd.diskPos)) {
		LOG_INFO("hno:%u  idx[%d], dev[%u:%u] dev id is invalid\n",
			PS3_HOST(instance), p_dev_ctx->vd_table_idx, channel,
			target_id);
		p_vd = NULL;
		goto l_out;
	}
l_out:
	return p_vd;
}

union PS3Device* ps3_dev_mgr_lookup_pd_list(
	struct ps3_instance *instance, U8 channel, U16 target_id)
{
	struct ps3_dev_pool *p_pd_pool = &instance->dev_context.pd_pool;
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;
	union PS3Device *p_pd = NULL;

	if (!ps3_dev_id_valid_check(instance, channel, target_id,
			PS3_DISK_TYPE_PD)) {
		goto l_out;
	}

	p_pd = &p_pd_pool->devs[channel][target_id];

	if (PS3_DEV_INVALID(p_pd->pd.diskPos)) {
#if 0
		LOG_INFO("hno:%u  idx[%d], dev[%u:%u] dev id is invalid\n",
			PS3_HOST(instance), p_dev_ctx->vd_table_idx, channel,
			target_id);
#endif
		p_pd = NULL;
		goto l_out;
	}
	LOG_INFO("hno:%u  idx[%d], dev[%u:%u] dev id is valid\n",
		PS3_HOST(instance), p_dev_ctx->vd_table_idx, channel,
		target_id);
l_out:
	return p_pd;
}

S32 ps3_adjust_queue_depth(struct ps3_instance *instance,
		U8 dev_type, U32 queue_depth)
{
	S32 dev_queue_depth = PS3_QUEUE_DEPTH_DEFAULT;

	switch (dev_type) {
	case PS3_DEV_TYPE_SAS_HDD:
	case PS3_DEV_TYPE_SAS_SSD:
		dev_queue_depth = PS3_QUEUE_DEPTH_SAS;
		break;
	case PS3_DEV_TYPE_SATA_HDD:
	case PS3_DEV_TYPE_SATA_SSD:
		dev_queue_depth = PS3_QUEUE_DEPTH_SATA;
		break;
	case PS3_DEV_TYPE_NVME_SSD:
		dev_queue_depth = PS3_QUEUE_DEPTH_NVME;
		break;
	default:
		dev_queue_depth = PS3_QUEUE_DEPTH_DEFAULT;
		break;
	}

	if (queue_depth != 0 && (S32)queue_depth <=
		instance->cmd_attr.cur_can_que) {
		dev_queue_depth = queue_depth;
	}

	return dev_queue_depth;
}

static inline S32 ps3_adjust_device_queue_depth(struct scsi_device *sdev,
	struct ps3_instance *instance, S32 q_depth)
{
	S32 queue_depth = q_depth;
	struct ps3_pd_entry *p_pd_entry = NULL;
	if (PS3_IS_PD_CHANNEL(instance, sdev->channel)) {
		p_pd_entry = ps3_dev_mgr_lookup_pd_info(instance,
			sdev->channel, sdev->id);
		if (p_pd_entry == NULL) {
			LOG_WARN_IN_IRQ(instance, "hno:%u cannot found PD[%u:%u] device info\n",
				PS3_HOST(instance), sdev->channel, sdev->id);
			goto l_out;
		}
		if ((p_pd_entry->dev_type == PS3_DEV_TYPE_SATA_HDD) ||
			(p_pd_entry->dev_type == PS3_DEV_TYPE_SATA_SSD)) {
			if (q_depth > PS3_QUEUE_DEPTH_SATA) {
				queue_depth = PS3_QUEUE_DEPTH_SATA;
			}
		}
	}

l_out:
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,10,0)
	scsi_adjust_queue_depth(sdev, scsi_get_tag_type(sdev), queue_depth);
#else
	scsi_change_queue_depth(sdev, queue_depth);
#endif
	return queue_depth;
}

#ifndef _WINDOWS 
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,10,0)
S32 ps3_change_queue_depth(struct scsi_device *sdev,
	S32 queue_depth, S32 reason)
{
	S32 ret = -EOPNOTSUPP;
	struct ps3_instance *instance = NULL;
	instance = (struct ps3_instance*)sdev->host->hostdata;
	if (instance == NULL) {
		LOG_ERROR_IN_IRQ(instance,"hno:%u  have no host\n",
			sdev->host->host_no);
		goto l_out;
	}

	if (queue_depth > sdev->host->can_queue) {
		queue_depth = sdev->host->can_queue;
	}
	if (PS3_IS_VD_CHANNEL(instance, sdev->channel) &&
		reason != SCSI_QDEPTH_DEFAULT) {
		goto l_out;
	}
	if (reason == SCSI_QDEPTH_DEFAULT || reason == SCSI_QDEPTH_RAMP_UP) {
		ret = ps3_adjust_device_queue_depth(sdev, instance, queue_depth);
	} else if (reason == SCSI_QDEPTH_QFULL) {
		scsi_track_queue_full(sdev, queue_depth);
		ret = sdev->queue_depth;
	}
	LOG_INFO_IN_IRQ(instance,"hno:%u  change dev[%u:%u] queue depth to [%d] reason [%d]\n",
		PS3_HOST(instance), sdev->channel, sdev->id, ret, reason);
l_out:
	return ret;
}

#else
S32 ps3_change_queue_depth(struct scsi_device *sdev,
	S32 queue_depth)
{
	S32 ret = sdev->queue_depth;
	struct ps3_instance *instance = NULL;

	instance = (struct ps3_instance*)sdev->host->hostdata;
	if (instance == NULL) {
		LOG_ERROR("hno:%u  have no host\n",
			sdev->host->host_no);
		goto l_out;
	}

	if (queue_depth > sdev->host->can_queue) {
		queue_depth = sdev->host->can_queue;
	}

	ret = ps3_adjust_device_queue_depth(sdev, instance, queue_depth);

	LOG_INFO("hno:%u  change dev[%u:%u] queue depth to [%d]\n",
		PS3_HOST(instance), sdev->channel, sdev->id, ret);

l_out:
	return ret;
}

#endif
static inline void ps3_init_vd_stream(struct ps3_vd_stream_detect *vdsd)
{
    U32 index = 0;
    U8 tyepIndex = 0;
    for(tyepIndex = PS3_SCSI_CMD_TYPE_READ;tyepIndex < PS3_SCSI_CMD_TYPE_WRITE;tyepIndex++){
        vdsd[tyepIndex-PS3_SCSI_CMD_TYPE_READ].mru_bit_map = MR_STREM_BITMAP;
        ps3_spin_lock_init(&vdsd[tyepIndex-PS3_SCSI_CMD_TYPE_READ].ps3_sequence_stream_lock);

        for(index = 0;index < PS3_IO_MAX_STREAMS_TRACKED;index++){
            vdsd[tyepIndex-PS3_SCSI_CMD_TYPE_READ].stream_track[index].next_seq_lba = 0;
            vdsd[tyepIndex-PS3_SCSI_CMD_TYPE_READ].stream_track[index].rw_type = tyepIndex;
        }
    }
    return;
}
S32 ps3_scsi_private_init_pd(struct scsi_device *sdev)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_scsi_priv_data *p_priv_data = NULL;
	struct ps3_pd_entry *p_pd_entry = NULL;
	struct ps3_instance *instance =
		(struct ps3_instance*)sdev->host->hostdata;

	p_pd_entry = ps3_dev_mgr_lookup_pd_info(instance,
		sdev->channel, sdev->id);
	INJECT_START(PS3_ERR_IJ_FORCE_PD_ENTRY_NULL1, &p_pd_entry);
	if (p_pd_entry == NULL) {
		LOG_WARN("hno:%u cannot found PD[%u:%u] device info\n",
			PS3_HOST(instance), sdev->channel, sdev->id);
		ret = -ENXIO;
		goto l_out;
	}
	INJECT_START(PS3_ERR_IJ_FORCE_PD_STATE_ERROR, &p_pd_entry->state);
	if (! ps3_pd_scsi_visible_check(instance, &p_pd_entry->disk_pos,
			p_pd_entry->dev_type, p_pd_entry->config_flag,
			p_pd_entry->state)) {
		ret = -ENXIO;
		LOG_WARN("hno:%u pd was blocked: chan[%d] id[%d]\n",
			PS3_HOST(instance), sdev->channel, sdev->id);
		goto l_out;
	}

	LOG_DEBUG("hno:%u found PD[%u:%u:%u] magic[%#x] device info\n",
		PS3_HOST(instance), sdev->channel, sdev->id,
		PS3_PDID(&p_pd_entry->disk_pos),
		p_pd_entry->disk_pos.diskMagicNum);

	p_priv_data = (struct ps3_scsi_priv_data*)ps3_kzalloc(instance,
		sizeof(struct ps3_scsi_priv_data));
	INJECT_START(PS3_ERR_IJ_FORCE_PRIV_DATA_NULL2, &p_priv_data);
	if (p_priv_data == NULL) {
		LOG_ERROR("hno:%u  pd[%u:%u:%u] Failed"
			" to allocate scsi device private data\n",
			PS3_HOST(instance), sdev->channel, sdev->id,
			PS3_PDID(&p_pd_entry->disk_pos));
		ret = -ENOMEM;
		goto l_out;
	}

	p_priv_data->disk_pos = p_pd_entry->disk_pos;
	p_priv_data->dev_type = p_pd_entry->dev_type;
	p_priv_data->is_taskmgmt_enable = PS3_DRV_TRUE;
	p_priv_data->task_abort_timeout =
		p_pd_entry->task_abort_timeout;
	p_priv_data->task_reset_timeout =
		p_pd_entry->task_reset_timeout;
	p_priv_data->task_manager_busy = 0;
	p_priv_data->encl_id = p_pd_entry->encl_id;
	p_priv_data->phy_id = p_pd_entry->phy_id;
	ps3_atomic_set(&p_priv_data->rd_io_outstand, PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic_set(&p_priv_data->wr_io_outstand, PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic_set(&p_priv_data->r1x_read_cmd_swap_total_cnt, PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic_set(&p_priv_data->r1x_read_cmd_swap_res_cnt, PS3_CMD_STAT_INIT_VALUE);
	ps3_qos_pd_mgr_init(instance, p_pd_entry);
	p_priv_data->dev_deling = PS3_FALSE;
	p_priv_data->swap_flag = PS3_FALSE;
	sdev->hostdata = p_priv_data;
l_out:
	return ret;
}

void ps3_vd_busy_scale_get(struct PS3VDEntry *vd_entry)
{
	U16 scale = 0;
	U32 strip_size_shift = 0;

	strip_size_shift = ps3_blocksize_to_shift(vd_entry->stripSize);
	scale = vd_entry->span[0].spanStripeDataSize >> strip_size_shift;;
	if ((vd_entry->raidLevel == RAID1E ||
			vd_entry->raidLevel == RAID10) &&
		(vd_entry->span[0].spanPdNum & 1)) {
		scale = scale >> 1;
	}

	vd_entry->dev_busy_scale = scale;
	return;
}

S32 ps3_scsi_private_init_vd(struct scsi_device *sdev)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_scsi_priv_data *p_priv_data = NULL;
	struct PS3VDEntry *p_vd_entry = NULL;
	struct ps3_instance *instance =
		(struct ps3_instance*)sdev->host->hostdata;
	struct ps3_pri_data_table *p_vd_pri_data_table = &instance->dev_context.vd_pri_data_table;

	p_vd_entry = ps3_dev_mgr_lookup_vd_info(instance,
		sdev->channel, sdev->id);
	INJECT_START(PS3_ERR_IJ_FORCE_VD_ENTRY_NULL1, &p_vd_entry);
	if (p_vd_entry == NULL) {
		LOG_WARN("hno:%u cannot found VD[%u:%u] device info\n",
			PS3_HOST(instance), sdev->channel, sdev->id);
		ret = -ENXIO;
		goto l_out;
	}
	LOG_DEBUG("hno:%u found VD[%u:%u:%u] magic[%#x] device info\n",
		PS3_HOST(instance), sdev->channel, sdev->id,
		PS3_VDID(&p_vd_entry->diskPos),
		p_vd_entry->diskPos.diskMagicNum);

	p_priv_data = (struct ps3_scsi_priv_data*)ps3_kzalloc(instance,
		sizeof(struct ps3_scsi_priv_data));
	INJECT_START(PS3_ERR_IJ_FORCE_PRIV_DATA_NULL, &p_priv_data);
	if (p_priv_data == NULL) {
		LOG_ERROR("hno:%u  vd[%u:%u:%u] Failed"
			" to allocate scsi device private data\n",
			PS3_HOST(instance), sdev->channel, sdev->id,
			PS3_VDID(&p_vd_entry->diskPos));
		ret = -ENOMEM;
		goto l_out;
	}

	p_priv_data->disk_pos = p_vd_entry->diskPos;
	p_priv_data->dev_type = PS3_DEV_TYPE_VD;
	p_priv_data->is_taskmgmt_enable = p_vd_entry->isTaskMgmtEnable;
	p_priv_data->task_abort_timeout = p_vd_entry->taskAbortTimeout;
	p_priv_data->task_reset_timeout = p_vd_entry->taskResetTimeout;
	p_priv_data->task_manager_busy = 0;
	ps3_atomic_set(&p_priv_data->rd_io_outstand, PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic_set(&p_priv_data->wr_io_outstand, PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic_set(&p_priv_data->r1x_read_cmd_swap_total_cnt, PS3_CMD_STAT_INIT_VALUE);
	ps3_atomic_set(&p_priv_data->r1x_read_cmd_swap_res_cnt, PS3_CMD_STAT_INIT_VALUE);
	p_priv_data->dev_deling = PS3_FALSE;
	p_priv_data->swap_flag = PS3_FALSE;
	ps3_vd_busy_scale_get(p_vd_entry);
	ps3_init_vd_stream(p_priv_data->vd_sd);
	p_priv_data->r1x_rb_info = (struct ps3_r1x_read_balance_info *)ps3_kzalloc(instance,
		sizeof(struct ps3_r1x_read_balance_info));
	INJECT_START(PS3_ERR_IJ_FORCE_RB_INFO_NULL, &p_priv_data->r1x_rb_info);
	if (p_priv_data->r1x_rb_info == NULL) {
		LOG_ERROR("hno:%u  vd[%u:%u:%u] Failed to allocate r1x_lb_info\n",
			PS3_HOST(instance), sdev->channel, sdev->id,
			PS3_VDID(&p_vd_entry->diskPos));
		ret = -ENOMEM;
		goto l_err;
	}
	ps3_mutex_lock(&instance->dev_context.dev_priv_lock);
	sdev->hostdata = p_priv_data;

	ret = ps3_r1x_lock_prepare_for_vd(instance, sdev,
		p_vd_entry->raidLevel);
	if(unlikely(PS3_SUCCESS != ret)){
		ps3_kfree(instance, p_priv_data->r1x_rb_info);
		p_priv_data->r1x_rb_info = NULL;
		ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);
		LOG_ERROR("hno:%u  vd[%u:%u:%u] Failed"
			" to allocate raid1x write lock mgr\n",
			PS3_HOST(instance), sdev->channel, sdev->id,
			PS3_VDID(&p_vd_entry->diskPos));
		ret = -ENOMEM;
		goto l_err;
	}
	ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);

	p_vd_pri_data_table->vd_pri_data_idxs[PS3_SDEV_CHANNEL(sdev)]
		[PS3_SDEV_TARGET(sdev)] = p_priv_data;

	ps3_qos_vd_init(instance, p_vd_entry);

	goto l_out;
l_err:
	ps3_mutex_lock(&instance->dev_context.dev_priv_lock);
	if (p_priv_data != NULL) {
		ps3_kfree(instance, p_priv_data);
		sdev->hostdata = NULL;
	}
	ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);

l_out:
	return ret;
}

S32 ps3_scsi_slave_alloc(struct scsi_device *sdev)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_scsi_priv_data *p_priv_data = NULL;
	struct ps3_instance *instance = NULL;
	struct PS3VDEntry *p_vd_entry = NULL;
	struct ps3_pd_entry *p_pd_entry = NULL;
	U32 dma_addr_alignment = 0;
	U32 dma_len_alignment = 0;
	U8 dev_type = PS3_DISK_TYPE_UNKNOWN;
	struct PS3DiskDevPos* p_diskPos = NULL;

	LOG_DEBUG("enter, [%u:%u:%llu]\n",
		sdev->channel, sdev->id, (U64)sdev->lun);
	INJECT_START(PS3_ERR_IJ_FORCE_LUN_ERROR, &sdev->lun);
	if (sdev->lun != 0) {
		ret = -ENXIO;
		goto l_out;
	}

	instance = (struct ps3_instance*)sdev->host->hostdata;
	INJECT_START(PS3_ERR_IJ_FORCE_INSTANCE_NULL, &instance);
	if (instance == NULL) {
		LOG_ERROR("hno:%u  have no host\n",
			sdev->host->host_no);
		ret = -ENXIO;
		goto l_out;
	}

	sdev->hostdata = NULL;
	INJECT_START_AT_TIME_WAIT_REV_PRE(PS3_ERR_IJ_GET_PRIV_DATA_DELAY, PS3_ERR_IJ_FORCE_WAIT, sdev);

	ret = -ENXIO;
	if (PS3_IS_VD_CHANNEL(instance, sdev->channel)) {
		dev_type = PS3_DISK_TYPE_VD;
		p_vd_entry = ps3_dev_mgr_lookup_vd_info(instance, sdev->channel,
			sdev->id);
		INJECT_START(PS3_ERR_IJ_FORCE_VD_ENTRY_NULL, &p_vd_entry);
		if (p_vd_entry == NULL) {
			LOG_ERROR("hno:%u, cannot found VD[%u:%u] device info\n",
				PS3_HOST(instance), sdev->channel, sdev->id);

			goto l_out;
		}

		p_diskPos = &p_vd_entry->diskPos;

	} else if (PS3_IS_PD_CHANNEL(instance, sdev->channel)) {
		dev_type = PS3_DISK_TYPE_PD;
		p_pd_entry = ps3_dev_mgr_lookup_pd_info(instance, sdev->channel,
			sdev->id);
		INJECT_START(PS3_ERR_IJ_FORCE_PD_ENTRY_NULL, &p_pd_entry);
		if (p_pd_entry == NULL) {
			LOG_ERROR("hno:%u, cannot found PD[%u:%u] device info\n",
				PS3_HOST(instance), sdev->channel, sdev->id);

			goto l_out;
		}

		p_diskPos = &p_pd_entry->disk_pos;
	} else {
		LOG_ERROR("hno:%u dev channel[%u] type NOK\n",
			PS3_HOST(instance), sdev->channel);
		goto l_out;
	}

	if (dev_type == PS3_DISK_TYPE_VD) {
		ret = ps3_scsi_private_init_vd(sdev);
	} else {
		ret = ps3_scsi_private_init_pd(sdev);
	}

	if (ret != PS3_SUCCESS) {
		goto l_dev_done;
	}

	p_priv_data = (struct ps3_scsi_priv_data*)sdev->hostdata;

	INJECT_START_AT_TIME_WAIT_REV_PRE(PS3_ERR_IJ_GET_PRIV_DATA_DELAY, PS3_ERR_IJ_FORCE_WAIT, NULL);
	INJECT_START_AT_TIME_WAIT_REV_PRE(PS3_ERR_IJ_GET_PRIV_DATA_DELAY, PS3_ERR_IJ_ADD_DEV_WAIT_OS_PRIV_DATA, NULL);
	ret = ps3_scsi_add_device_ack(instance, p_diskPos,
		dev_type);
	INJECT_START(PS3_ERR_IJ_FORCE_RET_FAIL3, &ret);
	if (unlikely(ret != PS3_SUCCESS)) {
		ret = -ENXIO;
		goto l_dev_ack_failed;
	}

	LOG_INFO("[%u:%u:%llu], dev_type[%s]\n",
		sdev->channel, sdev->id, (U64)sdev->lun,
		namePS3DevType((enum PS3DevType)p_priv_data->dev_type));

	if (p_priv_data->task_abort_timeout == 0) {
		p_priv_data->task_abort_timeout = PS3_DEFAULT_TASK_MGR_TIMEOUT;
	}

	if (p_priv_data->task_reset_timeout == 0) {
		p_priv_data->task_reset_timeout = PS3_DEFAULT_TASK_MGR_TIMEOUT;
	}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,10,0)

	sdev->tagged_supported = 1;
	scsi_activate_tcq(sdev, sdev->queue_depth);
#endif
	if (p_priv_data->dev_type == PS3_DEV_TYPE_VD) {
		dma_addr_alignment = ps3_pd_dma_alignment_calc(p_vd_entry->dmaAddrAlignShift);
		dma_len_alignment  = ps3_pd_dma_alignment_calc(p_vd_entry->dmaLenAlignShift);
	} else {
		dma_addr_alignment = p_pd_entry->dma_addr_alignment;
		dma_len_alignment = p_pd_entry->dma_len_alignment;
	}

	blk_queue_dma_alignment(sdev->request_queue, PS3_SCSI_ALINNMENT_MASK);
	if(dma_addr_alignment){
		blk_queue_dma_alignment(sdev->request_queue, dma_addr_alignment - 1);
	}

	if(dma_len_alignment){
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
		blk_queue_dma_pad(sdev->request_queue, dma_len_alignment - 1);
#else
		blk_queue_update_dma_pad(sdev->request_queue, dma_len_alignment - 1);
#endif

	}

	LOG_INFO("slave_alloc,dma_addr_alignment[%d] dma_len_alignment[%d]\n",
		dma_addr_alignment, dma_len_alignment);

	goto l_out;
l_dev_ack_failed:
	ps3_scsi_slave_destroy(sdev);
	INJECT_START_AT_TIME_WAIT_REV_PRE(PS3_ERR_IJ_GET_PRIV_DATA_DELAY, PS3_ERR_IJ_FORCE_WAIT, sdev);
l_dev_done:
	ps3_scsi_remove_device_done(instance, p_diskPos, dev_type);
l_out:
	LOG_DEBUG("exit, hno:%u\n", sdev->host->host_no);
	return ret;
}

void ps3_scsi_slave_destroy(struct scsi_device *sdev)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_scsi_priv_data *p_priv_data = NULL;
	struct ps3_instance *instance =
		(struct ps3_instance*)sdev->host->hostdata;
	struct ps3_pri_data_table *p_vd_pri_data_table = &instance->dev_context.vd_pri_data_table;
	INJECT_START(PS3_ERR_IJ_FORCE_INSTANCE_NULL2, &instance);
	if (instance == NULL) {
		LOG_ERROR("hno:%u  have no host\n",sdev->host->host_no);
		goto l_out;
	}

	LOG_DEBUG("hno:%u enter, max_chan[%u], max_id[%u], max_lun[%llu]\n",
		PS3_HOST(instance), sdev->host->max_channel,
		sdev->host->max_id, (U64)sdev->host->max_lun);

	p_priv_data = (struct ps3_scsi_priv_data*)sdev->hostdata;
	if (p_priv_data != NULL) {
		if (PS3_IS_VD_CHANNEL(instance, sdev->channel)) {
			LOG_INFO("hno[%u], vd[%u:%u] r1x conflict destory\n",
				PS3_HOST(instance), PS3_SDEV_CHANNEL(sdev),
				PS3_SDEV_TARGET(sdev));
			INJECT_AT_TIMES(PS3_ERR_IJ_GET_PRIV_DATA_DELAY, NULL);
			ps3_mutex_lock(&instance->dev_context.dev_priv_lock);
			ps3_r1x_lock_destory_for_vd(instance, &p_priv_data->lock_mgr);
			ps3_qos_vd_reset(instance, PS3_VDID(&p_priv_data->disk_pos));
			p_vd_pri_data_table->vd_pri_data_idxs
				[PS3_SDEV_CHANNEL(sdev)]
				[PS3_SDEV_TARGET(sdev)] = NULL;
			ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);
			if (p_priv_data->r1x_rb_info != NULL) {
				ps3_kfree(instance, p_priv_data->r1x_rb_info);
				p_priv_data->r1x_rb_info = NULL;
			}
		} else if (PS3_IS_PD_CHANNEL(instance, sdev->channel)) {
			ps3_qos_pd_mgr_reset(instance, PS3_PDID(&p_priv_data->disk_pos));
		}

		ret = ps3_scsi_remove_device_done(instance, &p_priv_data->disk_pos,
			ps3_disk_type((enum PS3DevType) p_priv_data->dev_type));
		INJECT_START(PS3_ERR_IJ_FORCE_RET_FAIL5, &ret);
		if (ret != PS3_SUCCESS) {
			LOG_INFO("hno:%u dev[%u:%u:%u] magic[%#x] dev del done error %d\n",
				PS3_HOST(instance),
				PS3_CHANNEL(&p_priv_data->disk_pos),
				PS3_TARGET(&p_priv_data->disk_pos),
				PS3_PDID(&p_priv_data->disk_pos),
				p_priv_data->disk_pos.diskMagicNum, ret);
		}
		ps3_mutex_lock(&instance->dev_context.dev_priv_lock);
		if(sdev->hostdata != NULL) {
			ps3_kfree(instance, sdev->hostdata);
		}
		sdev->hostdata = NULL;
		ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);
	}

l_out:
	LOG_DEBUG("exit, hno:%u\n", sdev->host->host_no);
}

static Bool ps3_is_nvme_device(struct ps3_instance *instance, U8 dev_type,
	U8 channel, U16 target_id)
{
	Bool ret = PS3_FALSE;
	struct PS3VDEntry *vd_entry = NULL;

	if (dev_type == PS3_DEV_TYPE_NVME_SSD) {
		ret = PS3_TRUE;
		goto l_out;
	}

	if (dev_type == PS3_DEV_TYPE_VD) {
		vd_entry = ps3_dev_mgr_lookup_vd_info(instance, channel, target_id);
		if (vd_entry == NULL) {
			LOG_ERROR("hno:%u  cannot found VD[%u:%u] device\n",
				PS3_HOST(instance), channel, target_id);
			ret = PS3_FALSE;
			goto l_out;
		}
		if (vd_entry->isNvme == 1) {
			ret = PS3_TRUE;
			goto l_out;
		}

		goto l_out;
	}

l_out:
	return ret;
}

static void ps3_nvme_attr_set(const struct ps3_instance *instance,
	struct scsi_device *sdev)
{
	U32 page_size = instance->cmd_attr.nvme_page_size;
	U32 align_mask = (page_size == 0) ? page_size : (page_size- 1);

	LOG_INFO("nvme page size is %u\n", page_size);
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,16,0)
	queue_flag_set_unlocked(QUEUE_FLAG_NOMERGES, sdev->request_queue);
#else
	blk_queue_flag_set(QUEUE_FLAG_NOMERGES, sdev->request_queue);
#endif
	blk_queue_virt_boundary(sdev->request_queue, align_mask);
}

static inline void ps3_nvme_pd_attr_set(struct scsi_device *sdev,
	const struct ps3_pd_entry *p_pd_entry)
{
	struct ps3_instance *instance = (struct ps3_instance *)sdev->host->hostdata;
	U32 sector_count = 0;

	if (p_pd_entry->max_io_size != 0 && p_pd_entry->sector_size != 0) {
		sector_count = p_pd_entry->max_io_size >> PS3_512B_SHIFT;
		blk_queue_max_hw_sectors(sdev->request_queue, sector_count);
	}

	LOG_INFO("nvme attr max_io_size[%u], sector_size[%u], sector_count[%u]\n",
		p_pd_entry->max_io_size, p_pd_entry->sector_size, sector_count);

	ps3_nvme_attr_set(instance, sdev);
}

static inline void ps3_set_queue_depth(struct scsi_device *sdev, U8 dev_type,
	U32 queue_depth)
{
	S32 dev_queue_depth = queue_depth;
	struct ps3_instance *instance = (struct ps3_instance *)sdev->host->hostdata;

	dev_queue_depth = ps3_adjust_queue_depth(instance, dev_type, queue_depth);

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,10,0)
	scsi_adjust_queue_depth(sdev, scsi_get_tag_type(sdev), dev_queue_depth);
#else
	scsi_change_queue_depth(sdev, dev_queue_depth);
#endif
}
void ps3_sdev_bdi_stable_writes_set(struct ps3_instance *instance, struct scsi_device *sdev)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) || \
	(defined(RHEL_MAJOR) && (RHEL_MAJOR >= 8) && (RHEL_MINOR >= 6)))
	(void)instance;
	blk_queue_flag_set(QUEUE_FLAG_STABLE_WRITES,
			   sdev->request_queue);
#else
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0))
		sdev->request_queue->backing_dev_info.capabilities
			|= BDI_CAP_STABLE_WRITES;
		LOG_INFO("hno:%u, dev type[%u:%u] capabilities[0x%x]\n",
				PS3_HOST(instance), sdev->channel, sdev->id,
				sdev->request_queue->backing_dev_info.capabilities);
	#else
		sdev->request_queue->backing_dev_info->capabilities
			|= BDI_CAP_STABLE_WRITES;
		LOG_INFO("hno:%u, dev type[%u:%u] capabilities[0x%x]\n",
				PS3_HOST(instance), sdev->channel, sdev->id,
				sdev->request_queue->backing_dev_info->capabilities);
	#endif
#endif
}
S32 ps3_sdev_bdi_stable_writes_get(struct scsi_device *sdev)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) || \
	(defined(RHEL_MAJOR) && (RHEL_MAJOR >= 8) && (RHEL_MINOR >= 6)))
	return blk_queue_stable_writes(sdev->request_queue);
#else
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0))
		return 	((sdev->request_queue->backing_dev_info.capabilities & \
			BDI_CAP_STABLE_WRITES) == BDI_CAP_STABLE_WRITES);
	#else
		return ((sdev->request_queue->backing_dev_info->capabilities & \
			BDI_CAP_STABLE_WRITES) == BDI_CAP_STABLE_WRITES);
	#endif
#endif
}

void ps3_sdev_bdi_stable_writes_clear(struct ps3_instance *instance, struct scsi_device *sdev)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) || \
	(defined(RHEL_MAJOR) && (RHEL_MAJOR >= 8) && (RHEL_MINOR >= 6)))
	(void)instance;
	blk_queue_flag_clear(QUEUE_FLAG_STABLE_WRITES,
			   sdev->request_queue);
#else
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0))
		sdev->request_queue->backing_dev_info.capabilities
			&= ~BDI_CAP_STABLE_WRITES;;
		LOG_INFO("hno:%u, dev type[%u:%u] capabilities[0x%x]\n",
				PS3_HOST(instance), sdev->channel, sdev->id,
				sdev->request_queue->backing_dev_info.capabilities);
	#else
		sdev->request_queue->backing_dev_info->capabilities
			&= ~BDI_CAP_STABLE_WRITES;;
		LOG_INFO("hno:%u, dev type[%u:%u] capabilities[0x%x]\n",
				PS3_HOST(instance), sdev->channel, sdev->id,
				sdev->request_queue->backing_dev_info->capabilities);
	#endif
#endif
}

S32 ps3_scsi_slave_configure(struct scsi_device *sdev)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_scsi_priv_data *p_priv_data = NULL;
	struct ps3_pd_entry *p_pd_entry = NULL;
	struct PS3VDEntry *p_vd_entry = NULL;
	U32 queue_depth = 0;
	U32 dma_addr_alignment = 0;
	U32 dma_len_alignment = 0;
	U8 io_tmo = PS3_SCSI_CMD_TIMEOUT_DEFAULT;

	struct ps3_instance *instance =
		(struct ps3_instance*)sdev->host->hostdata;
	INJECT_START(PS3_ERR_IJ_FORCE_INSTANCE_NULL1, &instance);
	if (unlikely(instance == NULL)) {
		LOG_ERROR("hno:%u  slave configure have no host instance\n",
			sdev->host->host_no);
		PS3_BUG();
		ret = -ENXIO;
		goto l_out;
	}

	p_priv_data = (struct ps3_scsi_priv_data*)sdev->hostdata;
	INJECT_START(PS3_ERR_IJ_FORCE_PRIV_DATA_NULL1, &p_priv_data);
	if (unlikely(p_priv_data == NULL)) {
		LOG_ERROR("hno:%u, slave configure have no private data, [%u:%u]\n",
			PS3_HOST(instance), sdev->channel, sdev->id);
		PS3_BUG();
		ret = -ENXIO;
		goto l_out;
	}
	INJECT_START(PS3_ERR_IJ_FORCE_MOD_DEV_TYPE, &p_priv_data->dev_type);
	if (unlikely(p_priv_data->dev_type == PS3_DEV_TYPE_UNKNOWN)) {
		LOG_ERROR("hno:%u, dev type[%u:%u] is PS3_DEV_TYPE_UNKNOWN\n",
			PS3_HOST(instance), sdev->channel, sdev->id);
		ret = -ENXIO;
		goto l_out;
	}

	if (p_priv_data->dev_type == PS3_DEV_TYPE_VD) {
		p_vd_entry = ps3_dev_mgr_lookup_vd_info(instance, sdev->channel,
			sdev->id);
		INJECT_START(PS3_ERR_IJ_FORCE_VD_ENTRY_NULL2, &p_vd_entry);
		if (p_vd_entry == NULL) {
			LOG_ERROR("hno:%u, cannot found VD[%u:%u] device info\n",
				PS3_HOST(instance), sdev->channel, sdev->id);
			ret = -ENXIO;
			goto l_out;
		}
		queue_depth = p_vd_entry->devQueDepth;
		dma_addr_alignment = ps3_pd_dma_alignment_calc(p_vd_entry->dmaAddrAlignShift);
		dma_len_alignment  = ps3_pd_dma_alignment_calc(p_vd_entry->dmaLenAlignShift);
		if (p_vd_entry->bdev_bdi_cap & PS3_STABLE_WRITES_MASK) {
			ps3_sdev_bdi_stable_writes_set(instance, sdev);
		}
	} else {
		p_pd_entry = ps3_dev_mgr_lookup_pd_info(instance, sdev->channel,
			sdev->id);
		INJECT_START(PS3_ERR_IJ_FORCE_PD_ENTRY_NULL2, &p_pd_entry);
		if (p_pd_entry == NULL) {
			LOG_ERROR("hno:%u, cannot found PD[%u:%u] device info\n",
				PS3_HOST(instance), sdev->channel, sdev->id);
			ret = -ENXIO;
			goto l_out;
		}
		queue_depth = p_pd_entry->dev_queue_depth;
		dma_addr_alignment = p_pd_entry->dma_addr_alignment;
		dma_len_alignment = p_pd_entry->dma_len_alignment;
	}

	blk_queue_dma_alignment(sdev->request_queue, PS3_SCSI_ALINNMENT_MASK);
	if(dma_addr_alignment){
		blk_queue_dma_alignment(sdev->request_queue, dma_addr_alignment - 1);
	}

	if(dma_len_alignment){
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
		blk_queue_dma_pad(sdev->request_queue, dma_len_alignment - 1);
#else
		blk_queue_update_dma_pad(sdev->request_queue, dma_len_alignment - 1);
#endif
	}
	if (instance->ctrl_info.ioTimeOut != 0) {
		io_tmo = instance->ctrl_info.ioTimeOut;
	}
	if (ps3_scsi_cmd_timeout_query() != 0) {
		io_tmo = ps3_scsi_cmd_timeout_query();
	}

	LOG_INFO("slave_configure, dma_addr_alignment[%d], "
		"dma_len_alignment[%d], io_timeout[%u], queue_depth[%u]\n",
		dma_addr_alignment, dma_len_alignment, io_tmo, queue_depth);

	blk_queue_rq_timeout(sdev->request_queue, io_tmo * HZ);

	ps3_set_queue_depth(sdev, p_priv_data->dev_type, queue_depth);

	if (p_priv_data->dev_type == PS3_DEV_TYPE_VD) {
		if (p_vd_entry->maxIOSize != 0) {
			if (p_vd_entry->sectorSize == PS3_SECTORSIZE_512B) {
				blk_queue_max_hw_sectors(sdev->request_queue, p_vd_entry->maxIOSize);
			} else {
				blk_queue_max_hw_sectors(sdev->request_queue,
					p_vd_entry->maxIOSize << (ilog2(p_vd_entry->sectorSize) - PS3_512B_SHIFT));
			}
		} else {
			LOG_DEBUG("hno:%u vd[%u:%u] update max sector num is:0\n",
				PS3_HOST(instance), sdev->channel, sdev->id);
		}
		if (ps3_is_nvme_device(instance, p_priv_data->dev_type,
			sdev->channel, sdev->id)) {
			ps3_nvme_attr_set(instance, sdev);
		} else {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
				blk_queue_dma_pad(sdev->request_queue, dma_len_alignment - 1);
#else
				blk_queue_update_dma_pad(sdev->request_queue, dma_len_alignment - 1);
#endif
		}
	} else if (p_pd_entry != NULL) {
		if (p_priv_data->dev_type == PS3_DEV_TYPE_NVME_SSD) {
			ps3_nvme_pd_attr_set(sdev, p_pd_entry);
		} else if (PS3_IS_HAC_LIMIT_TYPE(p_priv_data->dev_type)) {
			if (p_pd_entry->max_io_size != 0 && p_pd_entry->sector_size != 0) {
				blk_queue_max_hw_sectors(sdev->request_queue,
					p_pd_entry->max_io_size >> PS3_512B_SHIFT);
			}
		}
	}

	if (ps3_sas_is_support_smp(instance) && p_pd_entry != NULL) {
		if (ps3_check_pd_is_vd_member(p_pd_entry->config_flag)) {
			LOG_DEBUG("hno:%u, PD[%u:%u] is belong to vd device\n",
				PS3_HOST(instance), sdev->channel, sdev->id);
			sdev->no_uld_attach = 1;
		}

		if (p_priv_data->dev_type == PS3_DEV_TYPE_SAS_HDD ||
			p_priv_data->dev_type == PS3_DEV_TYPE_SAS_SSD ||
			p_priv_data->dev_type == PS3_DEV_TYPE_SES) {
			LOG_DEBUG("hno:%u pd[%u:%u] dev_type[%s] ready read port mode page\n",
				PS3_HOST(instance), sdev->channel, sdev->id,
				namePS3DevType((enum PS3DevType)p_pd_entry->dev_type));
			sas_read_port_mode_page(sdev);
		}
	}

l_out:
	return ret;
}
#else
void ps3_nvme_attr_set(const struct ps3_instance *instance,
	struct scsi_device *sdev)
{
	(void)instance;
	(void)sdev;
}
#endif
