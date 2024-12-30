
#include "ps3_device_update.h"

#ifdef _WINDOWS

#include "ps3_dev_adp.h"

#else
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/uio.h>
#include <linux/irq_poll.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/compiler.h>
#include <linux/delay.h>

#include <scsi/scsi_host.h>
#include "ps3_device_manager_sas.h"
#endif

#include "ps3_event.h"
#include "ps3_meta.h"
#include "ps3_htp_event.h"
#include "ps3_mgr_cmd.h"
#include "ps3_ioc_manager.h"
#include "ps3_driver_log.h"
#include "ps3_cmd_statistics.h"
#include "ps3_device_update.h"
#include "ps3_scsi_cmd_err.h"
#include "ps3_r1x_write_lock.h"
#include "ps3_ioc_state.h"

static S32 ps3_pd_del(struct ps3_instance *instance,
	struct PS3DiskDevPos *dev_pos, U8 config_flag);

static S32 ps3_pd_add(struct ps3_instance *instance,
	struct PS3DiskDevPos* dev_pos);

U32 ps3_scsi_dev_magic(struct ps3_instance *instance, struct scsi_device *sdev)
{
	U32 dev_magic_num = 0;
	struct ps3_scsi_priv_data *p_priv_data = NULL;

	ps3_mutex_lock(&instance->dev_context.dev_priv_lock);
	p_priv_data = PS3_SDEV_PRI_DATA(sdev);
	if (p_priv_data != NULL) {
		if (PS3_IS_VD_CHANNEL(instance, PS3_SDEV_CHANNEL(sdev)) ||
			ps3_sas_is_support_smp(instance)) {
			dev_magic_num = p_priv_data->disk_pos.diskMagicNum;
		} else {
			dev_magic_num = p_priv_data->disk_pos.checkSum;
		}
	}
	ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);

	return dev_magic_num;
}

Bool ps3_pd_scsi_visible_check(struct ps3_instance *instance,
	struct PS3DiskDevPos *disk_pos, U8 dev_type, U8 config_flag,
	U8 pd_state)
{
	Bool visible = PS3_DRV_TRUE;

	if (pd_state == DEVICE_STATE_OUTING) {
		LOG_INFO("hno:%u PD[%u:%u:%u], dev_type[%s] state[%s] is outing\n",
			PS3_HOST(instance), PS3_CHANNEL(disk_pos),
			PS3_TARGET(disk_pos), PS3_PDID(disk_pos),
			namePS3DevType((enum PS3DevType)dev_type),
			getDeviceStateName((DeviceState_e)pd_state));
		visible = PS3_DRV_FALSE;
		goto l_out;
	}

	if (ps3_is_fake_pd(dev_type)) {
		LOG_DEBUG("hno:%u PD[%u:%u:%u], dev_type[%s] device is visible all time\n",
			PS3_HOST(instance), PS3_CHANNEL(disk_pos),
			PS3_TARGET(disk_pos), PS3_PDID(disk_pos),
			namePS3DevType((enum PS3DevType)dev_type));
		goto l_out;
	}
	
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4,9,0) && LINUX_VERSION_CODE < KERNEL_VERSION(4,10,0))
		if (!instance->is_support_jbod &&
			ps3_check_pd_is_vd_member(config_flag)) {
			LOG_DEBUG("hno:%u PD[%u:%u:%u] is vd componet, config_flag[%s] \n",
				PS3_HOST(instance), PS3_CHANNEL(disk_pos),
				PS3_TARGET(disk_pos), PS3_PDID(disk_pos),
				getPdStateName((MicPdState_e)config_flag, instance->is_raid));
			visible = PS3_DRV_FALSE;
			goto l_out;
		}
#endif

	if (instance->is_support_jbod && config_flag !=
			MIC_PD_STATE_JBOD) {
		LOG_DEBUG("hno:%u PD[%u:%u:%u] config_flag is [%s], invisible \n",
			PS3_HOST(instance), PS3_CHANNEL(disk_pos),
			PS3_TARGET(disk_pos), PS3_PDID(disk_pos),
			getPdStateName((MicPdState_e)config_flag, instance->is_raid));
		visible = PS3_DRV_FALSE;
		goto l_out;
	}

l_out:
	LOG_DEBUG("hno:%u PD[%u:%u:%u] is %s\n",
		PS3_HOST(instance), PS3_CHANNEL(disk_pos),
		PS3_TARGET(disk_pos), PS3_PDID(disk_pos),
		visible ? "visible" : "invisible");

	return visible;
}

static inline Bool ps3_vd_scsi_visible_check(struct ps3_instance *instance,
	struct PS3DiskDevPos *disk_pos, Bool is_hidden, U8 disk_state)
{
	Bool visible = PS3_DRV_TRUE;

	if (is_hidden) {
		LOG_INFO("hno:%u vd[%u:%u:%u] is hidden\n",
			PS3_HOST(instance), PS3_CHANNEL(disk_pos),
			PS3_TARGET(disk_pos), PS3_VDID(disk_pos));
		visible = PS3_DRV_FALSE;
	}

	if (disk_state == MIC_VD_STATE_OFFLINE) {
		LOG_INFO("hno:%u vd[%u:%u:%u] state offline\n",
			PS3_HOST(instance), PS3_CHANNEL(disk_pos),
			PS3_TARGET(disk_pos), PS3_VDID(disk_pos));
		visible = PS3_DRV_FALSE;
	}

	return visible;
}

static Bool ps3_scsi_visible_check(struct ps3_instance *instance,
	U8 channel, U16 target_id, U8 disk_type)
{
	struct PS3VDEntry *p_vd_entry = NULL;
	struct ps3_pd_entry *p_pd_entry = NULL;
	Bool visible = PS3_DRV_FALSE;

	if (disk_type == PS3_DISK_TYPE_PD) {
		p_pd_entry = ps3_dev_mgr_lookup_pd_info(instance, channel, target_id);
		if (p_pd_entry) {
			visible = ps3_pd_scsi_visible_check(instance,
				&p_pd_entry->disk_pos, p_pd_entry->dev_type,
				p_pd_entry->config_flag, p_pd_entry->state);
		}
	} else if (disk_type == PS3_DISK_TYPE_VD){
		p_vd_entry = ps3_dev_mgr_lookup_vd_info(instance, channel, target_id);
		if (p_vd_entry) {
			visible = ps3_vd_scsi_visible_check(instance,
				&p_vd_entry->diskPos, p_vd_entry->isHidden, p_vd_entry->diskState);
		}
	}

	if (p_pd_entry || p_vd_entry) {
		LOG_INFO("hno:%u dev[%u:%u] visible is %s\n",
			PS3_HOST(instance), channel, target_id,
			visible ? "PS3_TRUE" : "PS3_FALSE");
	}
	return visible;
}

static inline void __ps3_scsi_add_device(struct ps3_instance *instance,
	struct PS3DiskDevPos *disk_pos, U8 dev_type)
{
	S32 ret = PS3_SUCCESS;
	U8 channel = PS3_CHANNEL(disk_pos);
	U16 id = PS3_TARGET(disk_pos);

	ret = ps3_scsi_add_device_ack(instance, disk_pos, dev_type);
	INJECT_START(PS3_ERR_IJ_FORCE_RET_FAIL2, &ret);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u dev[%u:%u] magic[%#x] add scsi device ack NOK, ret %d\n",
			PS3_HOST(instance), channel, id,
			disk_pos->diskMagicNum, ret);
	} else {
		LOG_INFO("hno:%u dev_type[%d] id[%u:%u] add device begin\n",
			PS3_HOST(instance), dev_type, channel, id);

		ret = ps3_scsi_add_device(instance, channel, id, 0);

		LOG_INFO("hno:%u dev_type[%d] id[%u:%u] add end, add ret %d\n",
			PS3_HOST(instance), dev_type, channel, id, ret);
	}
}

static void _ps3_scsi_add_device(struct ps3_instance *instance,
	struct PS3DiskDevPos *disk_pos, U8 dev_type)
{
	struct scsi_device *sdev = NULL;
	U8 channel = PS3_CHANNEL(disk_pos);
	U16 target_id = PS3_TARGET(disk_pos);
	U16 disk_id = PS3_VDID(disk_pos);

	LOG_DEBUG("hno:%u dev_type[%s] id[%u:%u] disk_id[%d] add scsi device start\n",
		PS3_HOST(instance), namePS3DiskType((enum PS3DiskType)dev_type),
		channel, target_id, disk_id);

	sdev = ps3_scsi_device_lookup(instance, channel, target_id, 0);
	if (sdev == NULL) {
		__ps3_scsi_add_device(instance, disk_pos, dev_type);
	} else {
		ps3_scsi_device_put(instance, sdev);
		LOG_INFO("hno:%u channel[%u] target[%u] device already exists in os\n",
			PS3_HOST(instance), channel, target_id);
	}

	LOG_DEBUG("hno:%u dev_type[%s] id[%u:%u] disk_id[%d] add scsi device end\n",
		PS3_HOST(instance), namePS3DiskType((enum PS3DiskType)dev_type),
		channel, target_id, disk_id);
}

static S32 _ps3_add_disk(struct ps3_instance *instance,
	U8 channel, U16 target_id, U8 disk_type)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_pd_entry *p_pd_entry = NULL;
	struct PS3VDEntry *p_vd_entry = NULL;
	struct PS3DiskDevPos *p_diskPos = NULL;
	U8 dev_type = PS3_DISK_TYPE_UNKNOWN;

	if (!ps3_scsi_visible_check(instance, channel, target_id, disk_type)) {
		goto l_out;
	}

	if (disk_type == PS3_DISK_TYPE_PD) {
		p_pd_entry = ps3_dev_mgr_lookup_pd_info(instance, channel, target_id);
		if (p_pd_entry) {
#ifndef _WINDOWS
			if (ps3_sas_is_support_smp(instance) &&
				p_pd_entry->dev_type != PS3_DEV_TYPE_NVME_SSD &&
				p_pd_entry->dev_type != PS3_DEV_TYPE_VEP) {
				ps3_sas_add_device(instance, p_pd_entry);
				goto l_out;
			} else {
#endif
				p_diskPos = &p_pd_entry->disk_pos;
				dev_type = PS3_DISK_TYPE_PD;
#ifndef _WINDOWS
			}
#endif
		}
	} else if (disk_type == PS3_DISK_TYPE_VD){
		p_vd_entry = ps3_dev_mgr_lookup_vd_info(instance, channel, target_id);
		if (p_vd_entry) {
			p_diskPos = &p_vd_entry->diskPos;
			dev_type = PS3_DISK_TYPE_VD;
		}
	}

	_ps3_scsi_add_device(instance, p_diskPos, dev_type);
l_out:
	return ret;
}
#ifndef _WINDOWS 

static Bool ps3_sd_available_check(struct scsi_device *sdev)
{
	Bool ret = PS3_TRUE;
	if (dev_get_drvdata(&sdev->sdev_gendev) == NULL) {
		ret = PS3_FALSE;
	}
	return ret;
}

void ps3_scsi_scan_host(struct ps3_instance *instance)
{
	struct PS3ChannelInfo *channel_info = &instance->ctrl_info.channelInfo;
	U16 timeout = instance->ctrl_info.isotoneTimeOut;
	U8 channelType = 0;
	U16 maxDevNum = 0;
	U8 i = 0;
	U16 j = 0;
	struct scsi_device *sdev = NULL;
	Bool bootdrv_ok = PS3_FALSE;
	U16 count = 0;

	LOG_WARN("hno:%u scan device begin, channel number %d\n",
		PS3_HOST(instance), channel_info->channelNum);
	INJECT_START(PS3_ERR_IJ_ADD_DISK_HOST_RESET, instance)
	for (i = 0; i < channel_info->channelNum; i++) {
		channelType = channel_info->channels[i].channelType;
		maxDevNum = channel_info->channels[i].maxDevNum;

		LOG_INFO("hno:%u channel[%u] is type %s,max dev num is:%d\n",
			PS3_HOST(instance), i,
			namePS3ChannelType((enum PS3ChannelType)channelType),
			maxDevNum);
		for (j = 0; j < maxDevNum; j++) {
			_ps3_add_disk(instance, i, j, channelType);
			if (timeout != 0 && !bootdrv_ok && i == 0 &&
				ps3_scsi_visible_check(instance, i, j, channelType)) {
				for (count = 0; count < timeout; count++) {
					sdev = ps3_scsi_device_lookup(instance, i, j, 0);
					if (sdev == NULL) {
						continue;
					}
					if (!ps3_sd_available_check(sdev)) {
						ps3_scsi_device_put(instance, sdev);
						ps3_msleep(PS3_PS3_LOOP_TIME_INTERVAL_1000MS);
						continue;
					}
					ps3_scsi_device_put(instance, sdev);
					LOG_WARN("hno:%u bootdrive disk[%u:%u] drive-letter add complete\n",
						PS3_HOST(instance), i, j);
					break;
				}
				bootdrv_ok = PS3_TRUE;
			}
		}
	}

	LOG_WARN("hno:%u scan device end, channel number %d\n",
		PS3_HOST(instance), channel_info->channelNum);
}
#endif
static inline S32 _ps3_del_disk(struct ps3_instance *instance,
	struct scsi_device *sdev)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_scsi_priv_data *p_priv_data = NULL;

	U8 dev_type = PS3_DISK_TYPE_UNKNOWN;
	U8 channel = PS3_SDEV_CHANNEL(sdev);
	U16 target_id = PS3_SDEV_TARGET(sdev);
	struct PS3DiskDevPos disk_pos;
	U8 encl_id = 0;
	U8 phy_id = 0;

	if (PS3_IS_PD_CHANNEL(instance, channel)) {
		dev_type = PS3_DISK_TYPE_PD;
	} else if (PS3_IS_VD_CHANNEL(instance, channel)) {
		dev_type = PS3_DISK_TYPE_VD;
	} else {
		LOG_ERROR("hno:%u dev id[%u:%u] channel NOK\n",
			PS3_HOST(instance), channel,
			target_id);
		PS3_BUG();
		goto l_out;
	}

	ps3_mutex_lock(&instance->dev_context.dev_priv_lock);
	p_priv_data = (struct ps3_scsi_priv_data*)sdev->hostdata;
	if (p_priv_data != NULL) {
		INJECT_AT_TIMES(PS3_ERR_IJ_FORCE_WAIT, sdev);

		INJECT_START_AT_TIME_WAIT_REV_PRE(PS3_ERR_IJ_DEL_DEV_WAIT_OS_PRIV_DATA,
			PS3_ERR_IJ_GET_PRIV_DATA_DELAY, NULL);

		ps3_qos_disk_del(instance, p_priv_data);
		if (ps3_sas_is_support_smp(instance) && dev_type == PS3_DISK_TYPE_PD &&
				p_priv_data->dev_type != PS3_DEV_TYPE_NVME_SSD &&
				p_priv_data->dev_type != PS3_DEV_TYPE_VEP) {
				memset(&disk_pos, 0, sizeof(struct PS3DiskDevPos));
				memcpy(&disk_pos, &p_priv_data->disk_pos, sizeof(struct PS3DiskDevPos));
				encl_id = p_priv_data->encl_id;
				phy_id = p_priv_data->phy_id;
			ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);
			ps3_sas_remove_device(instance, &disk_pos, encl_id, phy_id);
		} else {
			if (p_priv_data->dev_type == PS3_DEV_TYPE_VD &&
					p_priv_data->lock_mgr.hash_mgr != NULL) {
				p_priv_data->lock_mgr.dev_deling = PS3_TRUE;
				ps3_r1x_conflict_queue_clean(p_priv_data,
					PS3_SCSI_RESULT_HOST_STATUS(DID_NO_CONNECT));
			}
			ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);
			LOG_WARN("remove device channel[%u], id[%u] begin\n", channel, target_id);
			ps3_scsi_remove_device(instance, sdev);
			LOG_WARN("remove device channel[%u], id[%u] end\n", channel, target_id);
		}
	} else {

		ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);
		LOG_DEBUG("hno:%u del disk, priv data NULL, [%u:%u],\n",
			PS3_HOST(instance), PS3_SDEV_CHANNEL(sdev), PS3_SDEV_TARGET(sdev));
	}
l_out:
	return ret;
}

static S32 ps3_del_disk(struct ps3_instance *instance,
	U8 channel, U16 target_id)
{
	struct scsi_device *sdev = NULL;
	S32 ret = PS3_SUCCESS;
#if 0
	LOG_INFO("hno:%u dev id[%u:%u] delete scsi device start\n",
		PS3_HOST(instance), channel, target_id);
#endif
#ifdef _WINDOWS
	sdev = ps3_scsi_device_lookup_win(instance, channel, target_id);
	if (sdev == NULL || sdev->add_ack != 1) {
		LOG_INFO("hno:%u dev[%p] id[%u:%u] not exist scsi device\n",
			PS3_HOST(instance), sdev, channel, target_id);
		goto l_out;
	}
#else
	sdev = ps3_scsi_device_lookup(instance, channel, target_id, 0);
	if(sdev == NULL) {
#if 0
		LOG_INFO("hno:%u dev id[%u:%u] not exist scsi device\n",
			PS3_HOST(instance), channel, target_id);
#endif
		goto l_out;
	}
#endif
	INJECT_AT_TIMES(PS3_ERR_IJ_FORCE_WAIT, sdev);
	ret = _ps3_del_disk(instance, sdev);
	ps3_scsi_device_put(instance, sdev);
l_out:
#if 0
	LOG_INFO("hno:%u dev id[%u:%u] "
		"delete scsi device end\n", PS3_HOST(instance),
		channel, target_id);
#endif
	return ret;
}

static struct PS3VDEntry *ps3_get_single_vd_info(struct ps3_instance *instance,
	struct PS3Dev *dev)
{
	S32 ret = -PS3_FAILED;
	PS3DiskDev_u vd_id = {0};
	struct PS3VDEntry *p_vd_entry = NULL;

	vd_id.ps3Dev.virtDiskID = dev->virtDiskID;
	vd_id.ps3Dev.softChan = dev->softChan;
	vd_id.ps3Dev.devID = dev->devID;

	ret = ps3_vd_info_sync_get(instance, vd_id.diskID, 1);
	INJECT_START(PS3_ERR_IJ_FORCE_RET_FAIL1, &ret);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u, sync get VD info NOK\n",
			PS3_HOST(instance));
		goto l_out;
	}
	INJECT_START(PS3_ERR_IJ_FORCE_VD_COUNT_ERR, &instance->dev_context.vd_info_buf_sync->count);
	if (instance->dev_context.vd_info_buf_sync->count != 1) {
		LOG_ERROR("hno:%u, single add VD, has no info\n",
			PS3_HOST(instance));
		goto l_out;
	}

	p_vd_entry = instance->dev_context.vd_info_buf_sync->vds;
	INJECT_START(PS3_ERR_IJ_FORCE_MOD_CHAN, dev);
	if (PS3_CHANNEL(&p_vd_entry->diskPos) != dev->softChan ||
		PS3_TARGET(&p_vd_entry->diskPos) != dev->devID) {
		LOG_ERROR("hno:%u single add VD[%u:%u:%u] != req VD[%u:%u:%u] magic[%#x] unmatched\n",
			PS3_HOST(instance), dev->softChan, dev->devID,
			dev->virtDiskID, PS3_CHANNEL(&p_vd_entry->diskPos),
			PS3_TARGET(&p_vd_entry->diskPos),
			PS3_VDID(&p_vd_entry->diskPos),
			p_vd_entry->diskPos.diskMagicNum);
		p_vd_entry = NULL;
		goto l_out;
	}
	ps3_vd_busy_scale_get(p_vd_entry);

	LOG_DEBUG("hno:%u  VD[%u:%u:%u] got single vd info success\n",
		PS3_HOST(instance), dev->softChan, dev->devID, dev->virtDiskID);
l_out:
	return p_vd_entry;
}

static S32 ps3_update_single_vd_info(struct ps3_instance *instance,
	struct PS3Dev *dev)
{
	S32 ret = PS3_SUCCESS;
	struct PS3VDEntry *p_vd_entry = NULL;
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;
	U8 vd_table_idx = p_dev_ctx->vd_table_idx & 1;
	struct ps3_vd_table *p_vd_tb =
		&p_dev_ctx->vd_table[vd_table_idx];
	struct PS3VDEntry *p_vd_array =
		p_dev_ctx->vd_entries_array[vd_table_idx];
	U16 virtDiskIdx = 0;

	p_vd_entry = ps3_get_single_vd_info(instance, dev);
	if (p_vd_entry == NULL) {
		LOG_ERROR("hno:%u  sync get VD info NOK\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}

	virtDiskIdx = get_offset_of_vdid(PS3_VDID_OFFSET(instance), PS3_VDID(&p_vd_entry->diskPos));

	if (p_vd_entry->isHidden) {
		LOG_INFO("hno:%u single add VD[%u:%u] vd info is hidden\n",
			PS3_HOST(instance), dev->softChan, dev->devID);
		goto l_out;
	}

	ps3_qos_vd_init(instance, p_vd_entry);
	memcpy(&p_vd_array[virtDiskIdx],
		p_vd_entry, sizeof(struct PS3VDEntry));
	p_vd_tb->vd_idxs[dev->softChan][dev->devID] =
		PS3_VDID(&p_vd_entry->diskPos);

	ps3_vd_info_show(instance, &p_vd_array[virtDiskIdx]);

	LOG_DEBUG("hno:%u, idx[%d], VD[%u:%u] single add, got vd info\n",
		PS3_HOST(instance), p_dev_ctx->vd_table_idx, dev->softChan,
		dev->devID);
l_out:
	return ret;
}

static Bool _ps3_add_disk_prepare(struct ps3_instance *instance,
	struct PS3DiskDevPos *dev_pos)
{
	Bool is_need_add = PS3_TRUE;
	struct scsi_device *sdev = NULL;
	U32 sdev_magic = 0;
	U32 new_magic = 0;
	S32 ret = PS3_SUCCESS;

	sdev = ps3_scsi_device_lookup(instance, PS3_CHANNEL(dev_pos),
		PS3_TARGET(dev_pos), 0);
	if (sdev == NULL) {
		goto l_out;
	}

	sdev_magic = ps3_scsi_dev_magic(instance, sdev);
	if (PS3_IS_VD_CHANNEL(instance, PS3_SDEV_CHANNEL(sdev)) ||
		ps3_sas_is_support_smp(instance)) {
		new_magic = dev_pos->diskMagicNum;
	} else {
		new_magic = dev_pos->checkSum;
	}

	if (new_magic == sdev_magic) {
		LOG_INFO("hno:%u check magic same [%u:%u] magic[%#x]\n",
			PS3_HOST(instance), PS3_SDEV_CHANNEL(sdev), PS3_SDEV_TARGET(sdev),
			sdev_magic);
		is_need_add = PS3_FALSE;
	} else {
		LOG_WARN("hno:%u check dev[%u:%u] magic is diff[%#x != %#x]\n",
				PS3_HOST(instance), PS3_SDEV_CHANNEL(sdev),
				PS3_SDEV_TARGET(sdev),
				new_magic, sdev_magic);
		ret = _ps3_del_disk(instance, sdev);
		if (ret != PS3_SUCCESS) {
			LOG_ERROR("hno:%u del dev[%u:%u] unexpect ret[%d]\n",
				PS3_HOST(instance), PS3_SDEV_CHANNEL(sdev),
				PS3_SDEV_TARGET(sdev), ret);
		}
	}
	ps3_scsi_device_put(instance, sdev);
l_out:
	return is_need_add;
}

void ps3_check_vd_member_change(struct ps3_instance *instance,
	struct ps3_pd_entry *local_entry)
{
	struct scsi_device *sdev = NULL;

	if (!ps3_sas_is_support_smp(instance)) {
		goto l_out;
	}
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4,9,0) && LINUX_VERSION_CODE < KERNEL_VERSION(4,10,0))
	(void)sdev;
	if (ps3_check_pd_is_vd_member(local_entry->config_flag)) {
		LOG_WARN("hno:%u change ready to componet, remove device channel[%u], id[%u], begin\n",
			PS3_HOST(instance),
			PS3_CHANNEL(&local_entry->disk_pos),
			PS3_TARGET(&local_entry->disk_pos));
		ps3_pd_del(instance, &local_entry->disk_pos, local_entry->config_flag);
		LOG_WARN("hno:%u change ready to componet, remove device channel[%u], id[%u] end\n",
			PS3_HOST(instance),
			PS3_CHANNEL(&local_entry->disk_pos),
			PS3_TARGET(&local_entry->disk_pos));
	} else {
		ps3_linx80_vd_member_change(instance, local_entry);
		LOG_WARN("hno:%u change componet to ready, add device channel[%u], id[%u], begin\n",
			PS3_HOST(instance),
			PS3_CHANNEL(&local_entry->disk_pos),
			PS3_TARGET(&local_entry->disk_pos));
		ps3_pd_add(instance, &local_entry->disk_pos);
		LOG_WARN("hno:%u change componet to ready, add device channel[%u], id[%u], end\n",
			PS3_HOST(instance),
			PS3_CHANNEL(&local_entry->disk_pos),
			PS3_TARGET(&local_entry->disk_pos));
	}
#else
	sdev = ps3_scsi_device_lookup(instance, PS3_CHANNEL(&local_entry->disk_pos),
		PS3_TARGET(&local_entry->disk_pos), 0);
	if (sdev) {
		if (ps3_check_pd_is_vd_member(local_entry->config_flag)) {
			if (sdev->no_uld_attach != 1) {
				sdev->no_uld_attach = 1;
				ps3_qos_vd_member_change(instance, local_entry, sdev, PS3_TRUE);
				LOG_WARN("hno:%u pd[%u:%u] change ready to componet start\n",
					PS3_HOST(instance), sdev->channel, sdev->id);
				PS3_WARN_ON(scsi_device_reprobe(sdev));
				LOG_WARN("hno:%u pd[%u:%u] change ready to componet end\n",
					PS3_HOST(instance), sdev->channel, sdev->id);
			}
		} else {
			if (sdev->no_uld_attach != 0) {
				sdev->no_uld_attach = 0;
				ps3_qos_vd_member_change(instance, local_entry, sdev, PS3_FALSE);
				LOG_WARN("hno:%u pd[%u:%u] change componet to ready start\n",
					PS3_HOST(instance), sdev->channel, sdev->id);
				PS3_WARN_ON(scsi_device_reprobe(sdev));
				LOG_WARN("hno:%u pd[%u:%u] change componet to ready end\n",
					PS3_HOST(instance), sdev->channel, sdev->id);
			}
		}
		ps3_scsi_device_put(instance, sdev);
	}
#endif
l_out:
	return;
}

static struct PS3DiskDevPos* ps3_get_info_pos(
	struct ps3_instance *instance, U8 channel, U16 target_id, U8 disk_type)
{
	struct PS3DiskDevPos *disk_Pos_ret = NULL;
	struct ps3_pd_entry *pd_entry = NULL;
	struct PS3VDEntry *vd_entry = NULL;

	if (disk_type == PS3_DISK_TYPE_PD) {
		pd_entry = ps3_dev_mgr_lookup_pd_info(instance,
			channel, target_id);
		if (pd_entry != NULL) {
			disk_Pos_ret = &pd_entry->disk_pos;
		}
	}
	else if (disk_type == PS3_DISK_TYPE_VD) {
		vd_entry = ps3_dev_mgr_lookup_vd_info(instance,
			channel, target_id);
		if (vd_entry != NULL) {
			disk_Pos_ret = &vd_entry->diskPos;
		}
	}

	return disk_Pos_ret;
}

static inline S32 ps3_add_disk(struct ps3_instance *instance,
	struct PS3DiskDevPos* dev_pos, U8 disk_type)
{
	S32 ret = PS3_SUCCESS;
	struct PS3DiskDevPos* dev_pos_newes = NULL;

	if (disk_type == PS3_DISK_TYPE_PD) {
		ret = ps3_dev_mgr_pd_info_get(instance,
			PS3_CHANNEL(dev_pos),
			PS3_TARGET(dev_pos), PS3_PDID(dev_pos));
	}
	else if (disk_type == PS3_DISK_TYPE_VD) {
		ret = ps3_update_single_vd_info(instance, &dev_pos->diskDev.ps3Dev);
	}

	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u single add dev[%u:%u] get info NOK\n",
			PS3_HOST(instance), PS3_CHANNEL(dev_pos),
			PS3_TARGET(dev_pos));
		goto l_out;
	}

	dev_pos_newes = ps3_get_info_pos(instance, PS3_CHANNEL(dev_pos),
		PS3_TARGET(dev_pos), disk_type);
	if (dev_pos_newes == NULL) {
		ret = -PS3_FAILED;
		LOG_WARN("hno:%u update pos[%u:%u] invalid\n",
			PS3_HOST(instance), PS3_CHANNEL(dev_pos),
			PS3_TARGET(dev_pos));
		goto l_out;
	}

	INJECT_START_AT_TIME_WAIT_REV_PRE(PS3_ERR_IJ_ADD_DEV_WAIT_OS_PRIV_DATA,
		PS3_ERR_IJ_GET_PRIV_DATA_DELAY, NULL);
	if (!_ps3_add_disk_prepare(instance, dev_pos_newes)) {
		goto l_out;
	}

	INJECT_START_AT_TIME_WAIT_REV_PRE(PS3_ERR_IJ_ADD_DEV_WAIT_OS_PRIV_DATA,
		PS3_ERR_IJ_GET_PRIV_DATA_DELAY, NULL);
	ret = _ps3_add_disk(instance, PS3_CHANNEL(dev_pos_newes),
		PS3_TARGET(dev_pos_newes), disk_type);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u single add dev[%u:%u] NOK\n",
			PS3_HOST(instance), PS3_CHANNEL(dev_pos_newes),
			PS3_TARGET(dev_pos_newes));
		goto l_out;
	}
l_out:
	return ret;
}

static S32 ps3_pd_add(struct ps3_instance *instance,
	struct PS3DiskDevPos* dev_pos)
{
	S32 ret = -PS3_FAILED;
	ps3_mutex_lock(&instance->dev_context.dev_scan_lock);

	if (unlikely(!PS3_IS_PD_CHANNEL(instance,
		PS3_CHANNEL(dev_pos)))) {
		LOG_ERROR("hno:%u pd[%u:%u] single add, but channel is not pd\n",
			PS3_HOST(instance), PS3_CHANNEL(dev_pos),
			PS3_TARGET(dev_pos));
		PS3_BUG();
		goto l_out;
	}

	ret = ps3_add_disk(instance, dev_pos, PS3_DISK_TYPE_PD);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  add PD NOK\n",
			PS3_HOST(instance));
		goto l_out;
	}
l_out:
	ps3_mutex_unlock(&instance->dev_context.dev_scan_lock);

	return ret;
}

static Bool ps3_pd_ext_del_done_check(struct ps3_instance *instance,
	struct PS3DiskDevPos *dev_pos)
{
	struct scsi_device *sdev = NULL;
	Bool is_need_del_done = PS3_FALSE;
	S32 ret = PS3_SUCCESS;

#ifdef _WINDOWS
	sdev = ps3_scsi_device_lookup_win(instance, PS3_CHANNEL(dev_pos), PS3_TARGET(dev_pos));
	if (unlikely(sdev == NULL || sdev->add_ack != 1)) {
		is_need_del_done = TRUE;
	}
#else
	struct ps3_pd_entry *pd_entry = NULL;

	sdev = ps3_scsi_device_lookup(instance, PS3_CHANNEL(dev_pos), PS3_TARGET(dev_pos), 0);
	if(unlikely(sdev == NULL)) {
		if (ps3_sas_is_support_smp(instance)) {
			pd_entry = ps3_dev_mgr_lookup_pd_info_by_id(instance,
				PS3_PDID(dev_pos));
			if (pd_entry != NULL && pd_entry->sas_rphy != NULL) {
				ps3_sas_remove_device(instance, &pd_entry->disk_pos,
					pd_entry->encl_id, pd_entry->phy_id);
			} else {
				LOG_ERROR("hno:%u, can not found PD [%u:%u:%u] pd_entry:%p or sas_rphy is NULL\n",
					PS3_HOST(instance), PS3_CHANNEL(dev_pos),
					PS3_TARGET(dev_pos), PS3_PDID(dev_pos), pd_entry);
			}
		}
		is_need_del_done = PS3_TRUE;
	}
#endif
	if (is_need_del_done) {
		LOG_WARN("hno:%u dev id[%u:%u] not exist scsi device\n",
			PS3_HOST(instance), PS3_CHANNEL(dev_pos), PS3_TARGET(dev_pos));

		ret = ps3_scsi_remove_device_done(instance, dev_pos,
			PS3_DISK_TYPE_PD);
		INJECT_START(PS3_ERR_IJ_FORCE_RET_FAIL7, &ret);
		if (ret != PS3_SUCCESS) {
			LOG_ERROR("hno:%u dev[%u:%u:%u] magic[%#x] dev del done NOK %d\n",
				PS3_HOST(instance), PS3_CHANNEL(dev_pos),
				PS3_TARGET(dev_pos), PS3_PDID(dev_pos),
				dev_pos->diskMagicNum, ret);
		}
	} else {
		ps3_scsi_device_put(instance, sdev);
	}

	return is_need_del_done;
}

static S32 ps3_pd_del(struct ps3_instance *instance,
	struct PS3DiskDevPos *dev_pos, U8 config_flag)
{
	S32 ret = -PS3_FAILED;
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;
	struct ps3_pd_table* p_pd_tb = &p_dev_ctx->pd_table;
	U16 disk_idx = 0;

	ps3_mutex_lock(&instance->dev_context.dev_scan_lock);

	if (!ps3_dev_id_valid_check(instance, PS3_CHANNEL(dev_pos),
			PS3_TARGET(dev_pos),PS3_DISK_TYPE_PD)) {
		PS3_BUG();
		goto l_out;
	}

	if (ps3_pd_ext_del_done_check(instance, dev_pos)) {
		ret = PS3_SUCCESS;
		goto l_clean_again;
	}

	ret = ps3_del_disk(instance, PS3_CHANNEL(dev_pos), PS3_TARGET(dev_pos));
	INJECT_START(PS3_ERR_IJ_FORCE_RET_FAIL8, &ret);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u unable to delete scsi device PD[%u:%u] ret %d\n",
			PS3_HOST(instance), PS3_CHANNEL(dev_pos), PS3_TARGET(dev_pos), ret);
		goto l_out;
	}
l_clean_again:
	disk_idx = p_pd_tb->pd_idxs[PS3_CHANNEL(dev_pos)][PS3_TARGET(dev_pos)];
	p_pd_tb->pd_idxs[PS3_CHANNEL(dev_pos)][PS3_TARGET(dev_pos)] = PS3_INVALID_VALUE;

	p_dev_ctx->pd_entries_array[disk_idx].config_flag = config_flag;

l_out:
	ps3_mutex_unlock(&instance->dev_context.dev_scan_lock);
	return ret;

}

static S32 ps3_vd_add(struct ps3_instance *instance,
	struct PS3DiskDevPos* dev_pos)
{
	S32 ret = -PS3_FAILED;
	struct PS3Dev* dev = &dev_pos->diskDev.ps3Dev;

	if (unlikely(!PS3_IS_VD_CHANNEL(instance, dev->softChan))) {
		LOG_ERROR("hno:%u VD[%u:%u] single add, but channel is not vd\n",
			PS3_HOST(instance), dev->softChan, dev->devID);
		PS3_BUG();
		goto l_out;
	}

	ret = ps3_add_disk(instance, dev_pos, PS3_DISK_TYPE_VD);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  add VD NOK\n",
			PS3_HOST(instance));
		goto l_out;
	}
l_out:
	return ret;
}

static S32 ps3_vd_del(struct ps3_instance *instance,
	U8 channel, U16 target_id)
{
	S32 ret = -PS3_FAILED;

	if (!ps3_dev_id_valid_check(instance, channel, target_id,
		PS3_DISK_TYPE_VD)) {
		PS3_BUG();
		goto l_out;
	}

	ret = ps3_del_disk(instance, channel, target_id);
	INJECT_START(PS3_ERR_IJ_FORCE_RET_FAIL4, &ret);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u unable to delete scsi device VD[%u:%u] ret %d\n",
			PS3_HOST(instance), channel, target_id, ret);
		goto l_out;
	}

l_out:
	return ret;
}

static S32 ps3_vd_hidden_change(struct ps3_instance *instance,
	struct PS3Dev* dev)
{
	S32 ret = -PS3_FAILED;
	struct PS3VDEntry *p_vd_entry = NULL;
	struct PS3DiskDevPos disk_pos;

	p_vd_entry = ps3_get_single_vd_info(instance, dev);
	if (unlikely(p_vd_entry == NULL)) {
		LOG_ERROR("hno:%u  sync get VD info NOK\n",
			PS3_HOST(instance));
		goto l_out;
	}

	if (!p_vd_entry->isHidden) {
		disk_pos = p_vd_entry->diskPos;
		ret = ps3_vd_add(instance, &disk_pos);
	} else {
		ret = ps3_vd_del(instance, (U8)dev->softChan, dev->devID);
	}

	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u vd hidden change NOK, dev[%u:%u:%u], ret[%d]\n",
			PS3_HOST(instance), dev->softChan, dev->devID,
			dev->virtDiskID, ret);
		ret = -PS3_FAILED;
	}

l_out:
	return ret;
}

static inline Bool ps3_check_pd_is_same(struct ps3_instance *instance,
	struct PS3DiskDevPos* src_dev_pos, struct PS3DiskDevPos* dest_dev_pos)
{
	Bool ret_valid = PS3_FALSE;

	if (likely(dest_dev_pos->diskMagicNum == src_dev_pos->diskMagicNum &&
		PS3_DISKID(dest_dev_pos) == PS3_DISKID(src_dev_pos))) {
		LOG_DEBUG("hno:%u check pd id same [%u:%u:%u] magic[%#x]\n",
			PS3_HOST(instance), PS3_CHANNEL(dest_dev_pos),
			PS3_TARGET(dest_dev_pos), PS3_PDID(dest_dev_pos),
			src_dev_pos->diskMagicNum);
		ret_valid = PS3_TRUE;
	} else {
		LOG_FILE_WARN("hno:%u check mismatch, src dev[%u:%u:%u] magic[%#x], "
			"dest dev[%u:%u:%u] magic[%#x]\n",
			PS3_HOST(instance), PS3_CHANNEL(src_dev_pos),
			PS3_TARGET(src_dev_pos), PS3_PDID(src_dev_pos),
			src_dev_pos->diskMagicNum, PS3_CHANNEL(dest_dev_pos),
			PS3_TARGET(dest_dev_pos), PS3_PDID(dest_dev_pos),
			dest_dev_pos->diskMagicNum);
	}

	return ret_valid;
}
static inline void ps3_update_info_by_list_item(struct ps3_pd_entry *local_entry,
	union PS3Device *list_item, Bool is_raid)
{
	if (unlikely((local_entry->disk_pos.diskMagicNum !=
		list_item->pd.diskPos.diskMagicNum) ||
		(PS3_DISKID(&local_entry->disk_pos) !=
			PS3_DISKID(&list_item->pd.diskPos)))) {
		LOG_ERROR("Device is mismatch, local entry device[%u:%u:%u] "
			"and magic[%#x], new device[%u:%u:%u] and magic[%#x]\n",
			PS3_CHANNEL(&local_entry->disk_pos),
			PS3_TARGET(&local_entry->disk_pos),
			PS3_PDID(&local_entry->disk_pos),
			local_entry->disk_pos.diskMagicNum,
			PS3_CHANNEL(&list_item->pd.diskPos),
			PS3_TARGET(&list_item->pd.diskPos),
			PS3_PDID(&list_item->pd.diskPos),
			list_item->pd.diskPos.diskMagicNum);
		return;
	}

	local_entry->config_flag = list_item->pd.configFlag;
	local_entry->state = list_item->pd.diskState;
	LOG_INFO("single pd info update PD[%u:%u] config_flag[%s], state[%s]\n",
		PS3_CHANNEL(&list_item->pd.diskPos), PS3_TARGET(&list_item->pd.diskPos),
		getPdStateName((MicPdState_e)local_entry->config_flag, is_raid),
		getDeviceStateName((DeviceState_e)local_entry->state));
}

static S32 ps3_update_single_pd_info(struct ps3_instance *instance,
	struct PS3DiskDevPos *dev_pos, union PS3Device *list_item)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_pd_entry *p_pd_entry = NULL;

	if (unlikely(!PS3_IS_PD_CHANNEL(instance, PS3_CHANNEL(dev_pos)))) {
		LOG_ERROR("hno:%u, PD[%u:%u] info update, but channel is not pd\n",
			PS3_HOST(instance), PS3_CHANNEL(dev_pos),
			PS3_TARGET(dev_pos));
		ret = -PS3_FAILED;
		PS3_BUG();
		goto l_out;
	}

	p_pd_entry = ps3_dev_mgr_lookup_pd_info_by_id(instance,
		PS3_PDID(dev_pos));
	if (p_pd_entry == NULL) {
		LOG_FILE_WARN("hno:%u single PD info update[%u:%u], cannot find entry\n",
			PS3_HOST(instance),
			PS3_CHANNEL(dev_pos),
			PS3_TARGET(dev_pos));
	}

	if (p_pd_entry != NULL && !ps3_check_pd_is_same(instance,
			&p_pd_entry->disk_pos, dev_pos) && list_item) {
		ps3_update_info_by_list_item(p_pd_entry, list_item, instance->is_raid);
	} else {
		ret = ps3_dev_mgr_pd_info_get(instance, PS3_CHANNEL(dev_pos),
			PS3_TARGET(dev_pos), PS3_PDID(dev_pos));
		if (ret != PS3_SUCCESS) {
			LOG_ERROR("hno:%u, single add get PD[%u:%u] info NOK\n",
				PS3_HOST(instance), PS3_CHANNEL(dev_pos),
				PS3_TARGET(dev_pos));
			goto l_out;
		}

		p_pd_entry = ps3_dev_mgr_lookup_pd_info_by_id(instance,
			PS3_PDID(dev_pos));
		if (p_pd_entry == NULL) {
			LOG_WARN("hno:%u single PD info update[%u:%u], cannot find new entry\n",
				PS3_HOST(instance),
				PS3_CHANNEL(dev_pos),
				PS3_TARGET(dev_pos));
			goto l_out;
		}
	}

	if (instance->ioc_adpter->check_vd_member_change != NULL) {
		instance->ioc_adpter->check_vd_member_change(instance, p_pd_entry);
	}

l_out:
	return ret;
}

static void ps3_dev_update_pre_check(struct PS3EventDetail *event_detail,
		U32 event_cnt)
{
	U8 i = 0;
	U8 vd_count_idx = 0XFF;
	U8 pd_count_idx = 0XFF;
	U8 pd_info_idx = 0XFF;

	for (i = 0; i < event_cnt; i++) {
		switch (event_detail[i].eventCode) {
		case PS3_EVT_CODE(MGR_EVT_MULITPILE_PD_IN):
		case PS3_EVT_CODE(MGR_EVT_MULITPILE_PD_OUT):
		case PS3_EVT_CODE(MGR_EVT_MULITPILE_JBOD):
		case PS3_EVT_CODE(MGR_EVT_MULITPILE_READY):
		case PS3_EVT_CODE(MGR_EVT_BACKPLANE_ON):
		case PS3_EVT_CODE(MGR_EVT_BACKPLANE_OFF):
		case PS3_EVT_CODE(MGR_EVT_MULITPILE_PD_STATE_CHANGE):
			pd_count_idx = i;
			break;
		case PS3_EVT_CODE(MGR_EVT_MULITPILE_VD_IN):
		case PS3_EVT_CODE(MGR_EVT_MULITPILE_VD_OUT):
			vd_count_idx = i;
			break;
		default:
			break;
		}
	}

	if (pd_count_idx == 0xFF && vd_count_idx == 0xFF &&
			pd_info_idx == 0xFF) {
		return;
	}

	LOG_DEBUG("detail update pd_count_idx[%d], vd_count_idx[%d], pd_info_idx[%d]\n",
		pd_count_idx, vd_count_idx, pd_info_idx);

	for (i = 0; i < event_cnt; i++) {
		switch (event_detail[i].eventType) {
		case PS3_EVT_PD_COUNT:
			if (pd_count_idx !=0xFF && pd_count_idx != i) {
				LOG_DEBUG("detail update remove %d eventCode[%d]\n",
					i, event_detail[i].eventCode);
				event_detail[i].eventType =
					PS3_EVT_ILLEGAL_TYPE;
				event_detail[i].eventCode =
					PS3_EVT_CODE(MGR_EVT_PD_COUNT_START);
			}
			break;
		case PS3_EVT_VD_COUNT:
			if (vd_count_idx !=0xFF && vd_count_idx != i) {
				LOG_DEBUG("detail update remove %d eventCode[%d]\n",
					i, event_detail[i].eventCode);
				event_detail[i].eventType =
					PS3_EVT_ILLEGAL_TYPE;
				event_detail[i].eventCode =
					PS3_EVT_CODE(MGR_EVT_VD_COUNT_START);
			}
			break;
		case PS3_EVT_PD_ATTR:
			if (pd_info_idx !=0xFF && pd_info_idx != i) {
				LOG_DEBUG("detail update remove %d eventCode[%d]\n",
					i, event_detail[i].eventCode);
				event_detail[i].eventType =
					PS3_EVT_ILLEGAL_TYPE;
				event_detail[i].eventCode =
					PS3_EVT_CODE(MGR_EVT_PD_ATTR_START);
			}

			break;
		default:
			break;
		}
	}
}

static Bool ps3_get_sdev_pose_by_chl_tid(struct ps3_instance *instance,
	const U8 channel, const U16 target_id, struct PS3DiskDevPos *disk_Pos)
{
	struct scsi_device *sdev = NULL;
	struct ps3_scsi_priv_data *scsi_priv = NULL;
	Bool ret = PS3_TRUE;

	sdev = ps3_scsi_device_lookup(instance, channel, target_id, 0);
	if(unlikely(sdev == NULL)) {
		ret = PS3_FALSE;
	}
	if (ret) {
		ps3_mutex_lock(&instance->dev_context.dev_priv_lock);
		scsi_priv = PS3_SDEV_PRI_DATA(sdev);
		if (scsi_priv != NULL) {
			*disk_Pos = scsi_priv->disk_pos;
		} else {
			ret = PS3_FALSE;
			LOG_DEBUG("hno:%u get sdev pos, priv data NULL, [%u:%u],\n",
				PS3_HOST(instance), PS3_SDEV_CHANNEL(sdev), PS3_SDEV_TARGET(sdev));
		}
		ps3_mutex_unlock(&instance->dev_context.dev_priv_lock);
		ps3_scsi_device_put(instance, sdev);
	}

	return ret;
}

static S32 _ps3_add_remove_multi_pd(struct ps3_instance *instance,
	U8 channel, U16 target_id)
{
	S32 ret = PS3_SUCCESS;
	S32 ret_tmp = PS3_SUCCESS;
	union PS3Device* p_dev = NULL;
	struct PS3DiskDevPos pd_Pos;
	U8 config_flag = MIC_PD_STATE_UNKNOWN;

	p_dev = ps3_dev_mgr_lookup_pd_list(instance, channel, target_id);
	if (p_dev == NULL) {
		if (ps3_get_sdev_pose_by_chl_tid(instance, channel,
				target_id, &pd_Pos)) {
			ret_tmp = ps3_pd_del(instance, &pd_Pos,
				MIC_PD_STATE_UNKNOWN);
		}

		goto l_out;
	}

	if (ps3_pd_scsi_visible_check(instance, &p_dev->pd.diskPos,
			ps3_get_converted_dev_type(p_dev->pd.driverType,
			p_dev->pd.mediumType), p_dev->pd.configFlag,
			p_dev->pd.diskState)) {
		ret_tmp = ps3_pd_add(instance, &p_dev->pd.diskPos);
	} else {

		ret_tmp = ps3_dev_mgr_pd_info_get(instance,
			channel, target_id, PS3_PDID(&p_dev->pd.diskPos));
		if (ret_tmp != PS3_SUCCESS) {
			LOG_ERROR("hno:%u  get pd info NOK, [%u:%u], ret[%d]\n",
				PS3_HOST(instance), channel, target_id, ret_tmp);
			ret = -PS3_FAILED;
		}
		config_flag = p_dev->pd.configFlag;

		ret_tmp = ps3_pd_del(instance, &p_dev->pd.diskPos, config_flag);
	}
l_out:

	if (ret_tmp != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  add or del pd NOK, [%u:%u], ret[%d]\n",
			PS3_HOST(instance), channel, target_id, ret_tmp);
		ret = -PS3_FAILED;
	}

	return ret;
}

static S32 ps3_add_remove_multi_pd(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	S32 ret_tmp = PS3_SUCCESS;
	U16 i = 0;
	U16 j = 0;
	U8 chan_id = 0;
	struct ps3_channel *pd_chan = instance->dev_context.channel_pd;
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;

	LOG_DEBUG("hno:%u, ready to update full pd count\n",
		PS3_HOST(instance));

	ret = ps3_dev_mgr_pd_list_get(instance);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u  get pd list NOK\n",
			PS3_HOST(instance));
		goto l_out;
	}

	for (i = 0; i < p_dev_ctx->pd_channel_count; i++) {
		chan_id = pd_chan[i].channel;
		for (j = 0; j < pd_chan[i].max_dev_num; j++) {
			ret_tmp = _ps3_add_remove_multi_pd(instance, chan_id, j);
			if (ret_tmp != PS3_SUCCESS) {
				ret = -PS3_FAILED;
			}
		}
	}

l_out:
	LOG_DEBUG("hno:%u, update full pd count end\n",
		PS3_HOST(instance));

	return ret;
}

static S32 ps3_add_remove_multi_vd(struct ps3_instance *instance)
{
	S32 ret_tmp = PS3_SUCCESS;
	S32 ret = PS3_SUCCESS;
	U16 i = 0;
	U16 j = 0;
	U8 chan_id = 0;
	struct ps3_channel *vd_chan = instance->dev_context.channel_vd;
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;
	union PS3Device* p_dev = NULL;

	LOG_DEBUG("hno:%u ready to update full vd count\n",
		PS3_HOST(instance));

	ret = ps3_dev_mgr_vd_list_get(instance);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u get vd list NOK\n",
			PS3_HOST(instance));
		goto l_out;
	}

	for (i = 0; i < p_dev_ctx->vd_channel_count; i++) {
		chan_id = vd_chan[i].channel;
		for (j = 0; j < vd_chan[i].max_dev_num; j++) {

			p_dev = ps3_dev_mgr_lookup_vd_list(instance, chan_id, j);
			if (p_dev != NULL && ps3_vd_scsi_visible_check(instance,
				&p_dev->vd.diskPos, p_dev->vd.isHidden, p_dev->vd.diskState)) {
				ret_tmp = ps3_vd_add(instance, &p_dev->pd.diskPos);
			} else {
				ret_tmp = ps3_vd_del(instance, chan_id, j);
			}

			if (ret_tmp != PS3_SUCCESS) {
				LOG_ERROR("hno:%u update vd[%u:%u] count NOK, ret[%d]\n",
					PS3_HOST(instance), chan_id, j, ret_tmp);
				ret = -PS3_FAILED;
			}
		}
	}
l_out:
	LOG_DEBUG("hno:%u update full vd count end\n",
		PS3_HOST(instance));

	return ret;
}

static S32 ps3_update_multi_pd_info(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	S32 ret_tmp = PS3_SUCCESS;
	U16 i = 0;
	struct ps3_dev_context *p_dev_ctx = &instance->dev_context;
	struct PS3DevList *p_pd_list = p_dev_ctx->pd_list_buf;
	struct PS3Dev *p_dev = NULL;

	LOG_DEBUG("hno:%u  ready to update full pd info\n",
		PS3_HOST(instance));

	ret = ps3_pd_list_get(instance);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u, dev mgr get pd list NOK\n",
			PS3_HOST(instance));
		goto l_out;
	}

	LOG_INFO("hno:%u get pd list count is %d\n",
		PS3_HOST(instance), p_pd_list->count);

	for (i = 0; i < p_pd_list->count; i++) {
		p_dev = PS3_DEV(&p_pd_list->devs[i].pd.diskPos);

		if (PS3_DEV_INVALID(p_pd_list->devs[i].pd.diskPos)) {
			LOG_WARN("hno:%u  check pd list %d dev disk_id is 0\n",
				PS3_HOST(instance), i);
			continue;
		}

		if (!ps3_dev_id_valid_check(instance, (U8)p_dev->softChan,
				p_dev->devID, PS3_DISK_TYPE_PD)) {
			PS3_BUG();
			continue;
		}

		LOG_INFO("hno:%u  ready update pd info by list"
			" %d dev[%u:%u:%u], magic[%#x], state[%s]\n",
			PS3_HOST(instance), i, p_dev->softChan, p_dev->devID,
			p_dev->phyDiskID,
			p_pd_list->devs[i].pd.diskPos.diskMagicNum,
			getDeviceStateName((DeviceState_e)p_pd_list->devs[i].pd.diskState));

		ret_tmp = ps3_update_single_pd_info(instance, &p_pd_list->devs[i].pd.diskPos,
			&p_pd_list->devs[i]);
		if (ret_tmp != PS3_SUCCESS) {
			LOG_ERROR("hno:%u NOK, %u:%u, ret[%d]\n",
				PS3_HOST(instance), p_dev->softChan, p_dev->devID, ret_tmp);
			ret = -PS3_FAILED;
		}
	}

l_out:
	LOG_DEBUG("hno:%u  update full pd info end ret[%d]\n",
		PS3_HOST(instance), ret);

	return ret;
}

static inline S32 ps3_dev_evt_update_pd_count(struct ps3_instance *instance,
	U32 eventCode, struct PS3DiskDevPos *dev_pos)
{
	S32 ret = PS3_SUCCESS;

	switch(eventCode)
	{
	case PS3_EVT_CODE(MGR_EVT_DEVM_DISK_IN):
	case PS3_EVT_CODE(MGR_EVT_DEVM_JBOD):
	case PS3_EVT_CODE(MGR_EVT_DEVM_DISK_CHANGE):
		ret = ps3_pd_add(instance, dev_pos);
		break;
	case PS3_EVT_CODE(MGR_EVT_DEVM_DISK_OUT):
		ret = ps3_pd_del(instance, dev_pos, MIC_PD_STATE_UNKNOWN);
		break;
	case PS3_EVT_CODE(MGR_EVT_DEVM_READY):
	case PS3_EVT_CODE(MGR_EVT_PD_PRE_READY):
		ret = ps3_pd_del(instance, dev_pos, MIC_PD_STATE_READY);
		break;
	default:
		ret = ps3_add_remove_multi_pd(instance);
		break;
	}

	return ret;
}

static inline S32 ps3_dev_evt_update_vd_count(struct ps3_instance *instance,
	U32 eventCode, struct PS3DiskDevPos *dev_pos)
{
	S32 ret = PS3_SUCCESS;

	switch (eventCode) {
	case PS3_EVT_CODE(MGR_EVT_VD_CREATED):
	case PS3_EVT_CODE(MGR_EVT_VD_OPTIMAL):
	case PS3_EVT_CODE(MGR_EVT_VD_PARTIAL_DEGRADE):
	case PS3_EVT_CODE(MGR_EVT_VD_DEGRADE):
	case PS3_EVT_CODE(MGR_EVT_VD_UNLOCK):
		ret = ps3_vd_add(instance, dev_pos);
		break;
	case PS3_EVT_CODE(MGR_EVT_VD_DELETED):
	case PS3_EVT_CODE(MGR_EVT_VD_OFFLINE):
		ret = ps3_vd_del(instance, PS3_CHANNEL(dev_pos), PS3_TARGET(dev_pos));
		break;
	case PS3_EVT_CODE(MGR_EVT_VD_HIDDEN_CHANGE):
		ret = ps3_vd_hidden_change(instance, PS3_DEV(dev_pos));
		break;
	default:
		ret = ps3_add_remove_multi_vd(instance);
		break;
	}

	return ret;
}

static inline S32 ps3_dev_evt_update_pd_attr(struct ps3_instance *instance,
	U32 eventCode, struct PS3DiskDevPos *dev_pos)
{
	S32 ret = PS3_SUCCESS;
	(void)eventCode;

	ret = ps3_update_single_pd_info(instance, dev_pos, NULL);

	return ret;
}

S32 ps3_dev_update_detail_proc(struct ps3_instance *instance,
	struct PS3EventDetail *event_detail, U32 event_cnt)
{
	S32 ret = PS3_SUCCESS;
	S32 ret_map = PS3_SUCCESS;
	U32 i = 0;

	LOG_INFO("hno:%u, event detail count[%d]\n",
		PS3_HOST(instance), event_cnt);

	ps3_dev_update_pre_check(event_detail, event_cnt);

	for (i = 0; i < event_cnt; i++) {

		LOG_INFO("hno:%u  event detail %d event type[%s],"
			" eventCode is [%s], dev[%u:%u:%u]\n",
			PS3_HOST(instance), i,
			nameMgrEvtType(event_detail[i].eventType),
			mgrEvtCodeTrans(event_detail[i].eventCode),
			PS3_CHANNEL(&event_detail[i].devicePos),
			PS3_TARGET(&event_detail[i].devicePos),
			PS3_VDID(&event_detail[i].devicePos));

		switch (event_detail[i].eventType) {
		case PS3_EVT_PD_COUNT:
			ret = ps3_dev_evt_update_pd_count(instance,
				event_detail[i].eventCode,
				&event_detail[i].devicePos);
			break;
		case PS3_EVT_VD_COUNT:
			ret = ps3_dev_evt_update_vd_count(instance,
				event_detail[i].eventCode,
				&event_detail[i].devicePos);
			break;
		case PS3_EVT_PD_ATTR:
			ret = ps3_dev_evt_update_pd_attr(instance,
				event_detail[i].eventCode,
				&event_detail[i].devicePos);
			break;
		default:
			break;
		}

		if (ret != PS3_SUCCESS) {
			ret_map |= event_detail[i].eventType;
		}
	}

	return ret_map;
}

S32 ps3_dev_update_full_proc(struct ps3_instance *instance,
	MgrEvtType_e event_type)
{
	S32 ret = -PS3_FAILED;

	switch (event_type) {
	case PS3_EVT_PD_COUNT:
		ret = ps3_add_remove_multi_pd(instance);
		break;
	case PS3_EVT_VD_COUNT:
		ret = ps3_add_remove_multi_vd(instance);
		break;
	case PS3_EVT_PD_ATTR:
		ret = ps3_update_multi_pd_info(instance);
		break;
	default:
		break;
	}

	return ret;
}

static S32 ps3_dev_vd_pending_resend(struct ps3_cmd *cmd)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_dev_context *p_dev_ctx = &cmd->instance->dev_context;
	memset(p_dev_ctx->vd_info_buf_async,
		0, sizeof(struct PS3VDInfo));

	ps3_vd_pending_filter_table_build((U8*)
		p_dev_ctx->vd_info_buf_async);

	PS3_MGR_CMD_STAT_INC(cmd->instance, cmd);

	ret = ps3_async_cmd_send(cmd->instance, cmd);
	if (ret != PS3_SUCCESS) {
		LOG_FILE_ERROR("trace_id[0x%llx], hno:%u  re send vd pending cmd faild\n",
			cmd->trace_id, PS3_HOST(cmd->instance));
	}

	return ret;

}

static Bool ps3_dev_vd_pending_data_switch(struct ps3_cmd *cmd)
{
	S32 i = 0;
	Bool ret = PS3_FALSE;
	struct ps3_dev_context *p_dev_ctx = &cmd->instance->dev_context;
	U8 new_vd_table_idx = (p_dev_ctx->vd_table_idx + 1) & 1;
	struct ps3_vd_table *p_new_vd_tb = &p_dev_ctx->vd_table[new_vd_table_idx];
	struct PS3VDEntry *p_new_vd_array = p_dev_ctx->vd_entries_array[new_vd_table_idx];
	U8 old_vd_table_idx = p_dev_ctx->vd_table_idx & 1;
	struct ps3_vd_table *p_old_vd_tb = &p_dev_ctx->vd_table[old_vd_table_idx];
	struct PS3VDEntry *p_old_vd_array = p_dev_ctx->vd_entries_array[old_vd_table_idx];
	struct PS3VDEntry *p_vd_entry = p_dev_ctx->vd_info_buf_async->vds;
	struct PS3VDEntry *vd_entry_old = NULL;
	struct PS3Dev *p_dev = NULL;
	U16 virtDiskIdx = PS3_INVALID_DEV_ID;
	struct scsi_device *sdev = NULL;

	memcpy(p_new_vd_tb->vd_idxs_array, p_old_vd_tb->vd_idxs_array,
		p_dev_ctx->total_vd_count * sizeof(U16));
	memcpy(p_new_vd_array, p_old_vd_array, (PS3_MAX_VD_COUNT(cmd->instance) + 1) *
		sizeof(struct PS3VDEntry));

	for (i = 0; i < p_dev_ctx->vd_info_buf_async->count; i++) {
		p_dev = PS3_DEV(&p_vd_entry[i].diskPos);

		if (!ps3_dev_id_valid_check(cmd->instance, (U8)p_dev->softChan,
				p_dev->devID, PS3_DISK_TYPE_VD)) {
			LOG_ERROR_IN_IRQ(cmd->instance,
				"trace_id[0x%llx], hno:%u  vd pending "
				"%d err softchan:%d, or dev id > max, %d>%d\n",
				cmd->trace_id, PS3_HOST(cmd->instance),
				i, p_dev->softChan, p_dev->devID,
				p_dev_ctx->max_dev_in_channel[p_dev->softChan]);
			PS3_BUG_NO_SYNC();
			continue;
		}

		LOG_INFO_IN_IRQ(cmd->instance,
			"hno:%u update vd[%u:%u] info\n",
			PS3_HOST(cmd->instance), p_dev->softChan, p_dev->devID);

		ps3_qos_vd_init(cmd->instance, &p_vd_entry[i]);
		vd_entry_old = ps3_dev_mgr_lookup_vd_info_by_id(cmd->instance, p_dev->virtDiskID);
		ps3_qos_vd_attr_change(cmd->instance, vd_entry_old, &p_vd_entry[i]);
		p_new_vd_tb->vd_idxs[p_dev->softChan][p_dev->devID] =
			p_dev->virtDiskID;
		virtDiskIdx = get_offset_of_vdid(PS3_VDID_OFFSET(cmd->instance), p_dev->virtDiskID);
		memcpy(&p_new_vd_array[virtDiskIdx], &p_vd_entry[i],
			sizeof(struct PS3VDEntry));

		sdev = ps3_scsi_device_lookup(cmd->instance, p_dev->softChan, p_dev->devID, 0);
		if(sdev != NULL) {
			ps3_vd_busy_scale_get(&p_new_vd_array[virtDiskIdx]);
			if (ps3_sdev_bdi_stable_writes_get(sdev)) {
				if (!(p_vd_entry[i].bdev_bdi_cap & PS3_STABLE_WRITES_MASK)) {
					ps3_sdev_bdi_stable_writes_clear(cmd->instance, sdev);
				}
			} else {
				if (p_vd_entry[i].bdev_bdi_cap & PS3_STABLE_WRITES_MASK) {
					ps3_sdev_bdi_stable_writes_set(cmd->instance, sdev);
				}
			}
			ps3_scsi_device_put(cmd->instance, sdev);
		}

		ps3_vd_info_show(cmd->instance, &p_new_vd_array[virtDiskIdx]);
#ifndef _WINDOWS 
		if (p_vd_entry[i].maxIOSize != 0) {
			ps3_change_sdev_max_sector(cmd->instance, &p_vd_entry[i]);
		} else {
			LOG_DEBUG("hno:%u vd[%u:%u] update max sector num is:0\n",
				PS3_HOST(cmd->instance), p_dev->softChan, p_dev->devID);
		}
#endif
	}
	mb();
	if ( p_dev_ctx->vd_table_idx == (PS3_VD_TABLE_NUM -1)) {
		p_dev_ctx->vd_table_idx = 0;
	} else {
		p_dev_ctx->vd_table_idx++;
	}
	mb();

	LOG_INFO_IN_IRQ(cmd->instance,
		"trace_id[0x%llx], hno:%u  vd pending change cur_idx to [%d]\n",
		cmd->trace_id, PS3_HOST(cmd->instance), p_dev_ctx->vd_table_idx);

	return ret;
}

S32 ps3_dev_vd_pending_proc(struct ps3_cmd *cmd, U16 reply_flags)
{
	ULong flags = 0;
	ULong flags1 = 0;
	S32 ret = PS3_SUCCESS;
	struct ps3_dev_context *p_dev_ctx = NULL;
	struct ps3_instance *instance = cmd->instance;
	S32 cur_state = PS3_INSTANCE_STATE_INIT;

	p_dev_ctx = &instance->dev_context;
	p_dev_ctx->subwork += 1;
	LOG_DEBUG("trace_id[0x%llx], hno:%u  got a vd pending response, cur_idx[%d]\n",
			cmd->trace_id, PS3_HOST(instance), p_dev_ctx->vd_table_idx);

	PS3_MGR_CMD_BACK_INC(instance, cmd, reply_flags);
	if (unlikely(reply_flags == PS3_REPLY_WORD_FLAG_FAIL)) {
		LOG_ERROR_IN_IRQ(instance,
			"trace_id[0x%llx], hno:%u  vd pending cmd return failed\n",
			cmd->trace_id, PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_failed;
	}

	if (p_dev_ctx->vd_pending_cmd == NULL) {
		LOG_INFO_IN_IRQ(instance,
			"trace_id[0x%llx], hno:%u vd pending is unsubscribed!\n",
			cmd->trace_id, PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}

	if (!instance->state_machine.is_load) {
		LOG_INFO_IN_IRQ(instance,
			"trace_id[0x%llx], hno:%u instance is suspend or instance unload!\n",
			cmd->trace_id, PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_failed;
	}

	cur_state = ps3_atomic_read(&instance->state_machine.state);
	if (cur_state != PS3_INSTANCE_STATE_OPERATIONAL)  {
		LOG_INFO_IN_IRQ(instance,
			"trace_id[0x%llx], hno:%u instance is not operational!\n",
			cmd->trace_id, PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_failed;
	}

	LOG_INFO_IN_IRQ(instance,
		"trace_id[0x%llx], hno:%u  vd pending had [%d] vds\n",
		cmd->trace_id, PS3_HOST(instance), p_dev_ctx->vd_info_buf_async->count);

	ps3_dev_vd_pending_data_switch(cmd);

	ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
	cmd->cmd_state.state = PS3_CMD_STATE_PROCESS;
	ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);

	ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags1);
	if(instance->dev_context.abort_vdpending_cmd == 0){
		ret = ps3_dev_vd_pending_resend(cmd);
		ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
		if (ret == PS3_SUCCESS) {
			goto l_out;
		}
	}else{
		LOG_FILE_INFO("hno:%u  vd pending cmd free, CFID:%d\n",
			PS3_HOST(instance), cmd->index);
		instance->dev_context.vd_pending_cmd = NULL;
		ps3_mgr_cmd_free(instance, cmd);
		ret = ps3_dev_mgr_vd_info_subscribe(instance);
		if (ret != PS3_SUCCESS) {
			LOG_INFO("hno:%u subscribe failed!\n",
				PS3_HOST(instance));
		}

		instance->dev_context.abort_vdpending_cmd = 0;
		ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags1);
		goto l_out;
	}
l_failed:
	ps3_spin_lock_irqsave(&cmd->cmd_state.lock, &flags);
	cmd->cmd_state.state = PS3_CMD_STATE_DEAD;
	ps3_spin_unlock_irqrestore(&cmd->cmd_state.lock, flags);
	LOG_INFO_IN_IRQ(instance,
		"trace_id[0x%llx], CFID[%d], hno:%u to dead\n",
		cmd->trace_id, cmd->index, PS3_HOST(instance));

l_out:
	p_dev_ctx->subwork -= 1;
	LOG_INFO_IN_IRQ(instance,
		"trace_id[0x%llx], CFID[%d], hno:%u  end proc a vd pending response, subwork:%d,ret:%d\n",
		cmd->trace_id, cmd->index, PS3_HOST(instance), p_dev_ctx->subwork,ret);
	return ret;
}

#ifdef _WINDOWS
BOOL ps3_device_check_and_ack(struct ps3_instance *instance,
	U8 channel_type, U8 channel, U16 target_id)
{
	S32 ret_tmp = PS3_SUCCESS;
	BOOL ret = FALSE;
	struct ps3_pd_entry *p_pd_entry = NULL;
	struct PS3VDEntry *p_vd_entry = NULL;
	struct PS3DiskDevPos* p_diskPos = NULL;
	U8 dev_type = PS3_DISK_TYPE_UNKNOWN;

	if (!ps3_scsi_visible_check(instance, channel, target_id, channel_type)) {
		goto l_out;
	}

	if (channel_type == PS3_DISK_TYPE_PD) {
		p_pd_entry = ps3_dev_mgr_lookup_pd_info(instance, channel, target_id);
		if (p_pd_entry) {

			p_diskPos = &p_pd_entry->disk_pos;
			dev_type = PS3_DISK_TYPE_PD;

		}
	}
	else if (channel_type == PS3_DISK_TYPE_VD) {
		p_vd_entry = ps3_dev_mgr_lookup_vd_info(instance, channel, target_id);
		if (p_vd_entry) {
			p_diskPos = &p_vd_entry->diskPos;
			dev_type = PS3_DISK_TYPE_VD;
		}
	}

	if (p_diskPos == NULL) {
		goto l_out;
	}

	ret_tmp = ps3_scsi_add_device_ack(instance, p_diskPos, dev_type);
	if (ret_tmp != PS3_SUCCESS) {
		LOG_ERROR("hno:%u dev[%u:%u] magic[%#x] pre scan ack NOK, ret %d\n",
			PS3_HOST(instance), channel, target_id,
			p_diskPos->diskMagicNum, ret_tmp);
	} else {
		LOG_INFO("hno:%u dev_type[%d] id[%u:%u] send ack success\n",
			PS3_HOST(instance), dev_type, channel, target_id);

		ret = TRUE;
	}
l_out:
	return ret;
}
#endif
