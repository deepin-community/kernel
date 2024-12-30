
#ifndef _PS3_DEVICE_UPDATE_H_
#define _PS3_DEVICE_UPDATE_H_

#ifndef _WINDOWS
#include <linux/workqueue.h>
#endif

#include "ps3_htp_event.h"
#include "ps3_instance_manager.h"

Bool ps3_pd_scsi_visible_check(struct ps3_instance *instance,
	struct PS3DiskDevPos *disk_pos, U8 dev_type, U8 config_flag,
	U8 pd_state);

S32 ps3_dev_update_detail_proc(struct ps3_instance *instance,
	struct PS3EventDetail *event_detail, U32 event_cnt);

S32 ps3_dev_update_full_proc(struct ps3_instance *instance,
	MgrEvtType_e event_type);

S32 ps3_dev_vd_pending_proc(struct ps3_cmd *cmd, U16 reply_flags);

#ifdef _WINDOWS

BOOL ps3_device_check_and_ack(struct ps3_instance *instance,
	U8 channel_type, U8 channel, U16 target_id);
#endif
U32 ps3_scsi_dev_magic(struct ps3_instance *instance, struct scsi_device *sdev);

void ps3_scsi_scan_host(struct ps3_instance *instance);

void ps3_check_vd_member_change(struct ps3_instance *instance,
	struct ps3_pd_entry *local_entry);

#endif
