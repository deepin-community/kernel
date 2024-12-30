
#ifndef _PS3_MANAGMENT_CMD_H_
#define _PS3_MANAGMENT_CMD_H_

#include "ps3_instance_manager.h"
#include "ps3_device_manager.h"

#define PS3_MGR_BASE_DATA_SIZE		(64)

#define PS3_MGR_CMD_SGL_OFFSET_DWORD_SHIFT (2)

#define PS3_MGR_CMD_TYPE(cmd) ((cmd)->req_frame->mgrReq.reqHead.cmdType)
#define PS3_MGR_CMD_SUBTYPE(cmd) ((cmd)->req_frame->mgrReq.reqHead.cmdSubType)
enum{
	PS3_CANCEL_EVENT_CMD = 1,
	PS3_CANCEL_VDPENDING_CMD,
    PS3_CANCEL_WEB_CMD,
};
S32 ps3_ctrl_info_buf_alloc(struct ps3_instance *instance);

void ps3_ctrl_info_buf_free(struct ps3_instance *instance);

S32 ps3_mgr_cmd_init(struct ps3_instance *instance);

void ps3_mgr_cmd_exit(struct ps3_instance *instance);

S32 ps3_pd_list_get(struct ps3_instance *instance);

S32 ps3_vd_list_get(struct ps3_instance *instance);

S32 ps3_pd_info_get(struct ps3_instance *instance, U16 channel, U16 target_id, U16 pd_disk_id);

S32 ps3_vd_info_sync_get(struct ps3_instance *instance, U32 disk_id,
	U16 vd_num);

S32 ps3_vd_info_async_get(struct ps3_instance *instance);

S32 ps3_ctrl_info_get(struct ps3_instance *instance);

S32 ps3_soc_unload(struct ps3_instance *instance, Bool is_polling, U8 type, U8 suspend_type);

S32 ps3_scsi_remove_device_done(struct ps3_instance *instance,
	struct PS3DiskDevPos *disk_pos, U8 dev_type);

S32 ps3_scsi_add_device_ack(struct ps3_instance *instance,
	struct PS3DiskDevPos *disk_pos, U8 dev_type);

S32 ps3_mgr_cmd_cancel(struct ps3_instance *instance, U16 cancel_cmd_frame_id);

S32 ps3_event_register(struct ps3_instance *instance,
		struct PS3MgrEvent *event);
S32 ps3_web_register(struct ps3_instance *instance);

S32 ps3_scsi_task_mgr_abort(struct ps3_instance *instance,
	struct ps3_scsi_priv_data *priv_data, U16 aborted_cmd_frame_id, struct scsi_cmnd *scmd);

S32 ps3_scsi_task_mgr_reset(struct ps3_instance *instance,
	struct ps3_cmd *cmd);

void ps3_mgr_cmd_word_build(struct ps3_cmd *cmd);

S32 ps3_sas_expander_all_get(struct ps3_instance *instance);

S32 ps3_sas_phy_get(struct ps3_instance *instance, struct PS3SasMgr *sas_req);

S32 ps3_sas_expander_get(struct ps3_instance *instance, struct PS3SasMgr *sas_req);

S32 ps3_mgr_complete_proc(struct ps3_instance *instance,
	struct ps3_cmd *cmd, S32 send_result);

struct ps3_cmd *ps3_dump_notify_cmd_build(struct ps3_instance *instance);
struct ps3_cmd * ps3_scsi_task_mgr_reset_build(struct ps3_instance *instance,
	struct ps3_scsi_priv_data *priv_data);

struct ps3_cmd * ps3_scsi_task_mgr_reset_build(struct ps3_instance *instance,
	struct ps3_scsi_priv_data *priv_data);

S32 ps3_mgr_cmd_no_resp_proc(struct ps3_instance *instance,
	struct ps3_cmd *cmd);

Bool ps3_check_ioc_state_is_normal_in_unload(
	struct ps3_instance *instance);

S32 ps3_mgr_cmd_cancel_send(struct ps3_instance *instance,
	U16 cancel_cmd_frame_id, U8 type);
S32 ps3_mgr_cmd_cancel_wait(struct ps3_instance *instance, U8 type);

#endif
