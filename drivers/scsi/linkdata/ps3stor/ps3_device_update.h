/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_DEVICE_UPDATE_H_
#define _PS3_DEVICE_UPDATE_H_

#ifndef _WINDOWS
#include <linux/workqueue.h>
#endif

#include "ps3_htp_event.h"
#include "ps3_instance_manager.h"

unsigned char ps3_pd_scsi_visible_check(struct ps3_instance *instance,
					struct PS3DiskDevPos *disk_pos,
					unsigned char dev_type,
					unsigned char config_flag,
					unsigned char pd_state);

int ps3_dev_update_detail_proc(struct ps3_instance *instance,
			       struct PS3EventDetail *event_detail,
			       unsigned int event_cnt);

int ps3_dev_update_full_proc(struct ps3_instance *instance,
			     enum MgrEvtType event_type);

int ps3_dev_vd_pending_proc(struct ps3_cmd *cmd, unsigned short reply_flags);

#ifdef _WINDOWS

int ps3_device_check_and_ack(struct ps3_instance *instance,
			     unsigned char channel_type, unsigned char channel,
			     unsigned short target_id);
#endif
unsigned int ps3_scsi_dev_magic(struct ps3_instance *instance,
				struct scsi_device *sdev);

void ps3_scsi_scan_host(struct ps3_instance *instance);

void ps3_check_vd_member_change(struct ps3_instance *instance,
				struct ps3_pd_entry *local_entry);

#endif
