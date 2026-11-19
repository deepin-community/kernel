/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_MANAGEMENT_CMD_H_
#define _PS3_MANAGEMENT_CMD_H_

#include "ps3_instance_manager.h"
#include "ps3_device_manager.h"

#define PS3_MGR_BASE_DATA_SIZE (64)

#define PS3_MGR_CMD_SGL_OFFSET_DWORD_SHIFT (2)

#define PS3_MGR_CMD_TYPE(cmd) ((cmd)->req_frame->mgrReq.reqHead.cmdType)
#define PS3_MGR_CMD_SUBTYPE(cmd) ((cmd)->req_frame->mgrReq.reqHead.cmdSubType)
enum {
	PS3_CANCEL_EVENT_CMD = 1,
	PS3_CANCEL_VDPENDING_CMD,
	PS3_CANCEL_WEB_CMD,
};
int ps3_ctrl_info_buf_alloc(struct ps3_instance *instance);

void ps3_ctrl_info_buf_free(struct ps3_instance *instance);

int ps3_mgr_cmd_init(struct ps3_instance *instance);

void ps3_mgr_cmd_exit(struct ps3_instance *instance);

int ps3_pd_list_get(struct ps3_instance *instance);

int ps3_vd_list_get(struct ps3_instance *instance);

int ps3_pd_info_get(struct ps3_instance *instance, unsigned short channel,
		    unsigned short target_id, unsigned short pd_disk_id);

int ps3_vd_info_sync_get(struct ps3_instance *instance, unsigned int disk_id,
			 unsigned short vd_num);

int ps3_vd_info_async_get(struct ps3_instance *instance);

int ps3_ctrl_info_get(struct ps3_instance *instance);

int ps3_soc_unload(struct ps3_instance *instance, unsigned char is_polling,
		   unsigned char type, unsigned char suspend_type);

int ps3_scsi_remove_device_done(struct ps3_instance *instance,
				struct PS3DiskDevPos *disk_pos,
				unsigned char dev_type);

int ps3_scsi_add_device_ack(struct ps3_instance *instance,
			    struct PS3DiskDevPos *disk_pos,
			    unsigned char dev_type);

int ps3_mgr_cmd_cancel(struct ps3_instance *instance,
		       unsigned short cancel_cmd_frame_id);

int ps3_event_register(struct ps3_instance *instance,
		       struct PS3MgrEvent *event);

int ps3_web_register(struct ps3_instance *instance);

int ps3_scsi_task_mgr_abort(struct ps3_instance *instance,
			    struct ps3_scsi_priv_data *priv_data,
			    unsigned short aborted_cmd_frame_id,
			    struct scsi_cmnd *scmd);

void ps3_mgr_cmd_word_build(struct ps3_cmd *cmd);

int ps3_sas_expander_all_get(struct ps3_instance *instance);

int ps3_sas_phy_get(struct ps3_instance *instance, struct PS3SasMgr *sas_req);

int ps3_sas_expander_get(struct ps3_instance *instance,
			 struct PS3SasMgr *sas_req);

int ps3_mgr_complete_proc(struct ps3_instance *instance, struct ps3_cmd *cmd,
			  int send_result);

struct ps3_cmd *ps3_dump_notify_cmd_build(struct ps3_instance *instance);

struct ps3_cmd *
ps3_scsi_task_mgr_reset_build(struct ps3_instance *instance,
			      struct ps3_scsi_priv_data *priv_data);

int ps3_mgr_cmd_no_resp_proc(struct ps3_instance *instance,
			     struct ps3_cmd *cmd);

unsigned char
ps3_check_ioc_state_is_normal_in_unload(struct ps3_instance *instance);

int ps3_mgr_cmd_cancel_send(struct ps3_instance *instance,
			    unsigned short cancel_cmd_frame_id,
			    unsigned char type);

int ps3_mgr_cmd_cancel_wait(struct ps3_instance *instance, unsigned char type);

#endif
