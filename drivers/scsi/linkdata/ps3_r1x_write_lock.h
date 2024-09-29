
#ifndef __PS3_R1X_WRITE_LOCK_H__
#define __PS3_R1X_WRITE_LOCK_H__

#include "ps3_types.h"
#include "ps3_cmd_channel.h"

enum {
    PS3_R1X_HASHBIT_LOCK = 0,       
    PS3_R1X_HASHRANGE_LOCK = 1,     
};

extern U32 g_ps3_r1x_lock_flag;     
extern U32 g_ps3_r1x_lock_enable;   

U32 ps3_r1x_get_node_Buff_size(void);

static inline S32 ps3_r1x_write_lock(struct ps3_r1x_lock_mgr *mgr, struct ps3_cmd *cmd)
{
	if (mgr->hash_mgr == NULL) {
		return PS3_SUCCESS;
	}

	if (0 == g_ps3_r1x_lock_enable) {
		return PS3_SUCCESS;
	}

	if (!cmd->io_attr.is_confilct_check) {
		return PS3_SUCCESS;
	}

	if (cmd->is_inserted_c_q == 0) {
		return mgr->try_lock(mgr, cmd);
	} else {
		return mgr->resend_try_lock(mgr, cmd);
	}
}

static inline void ps3_r1x_write_unlock(struct ps3_r1x_lock_mgr *mgr, struct ps3_cmd *cmd)
{
	if(cmd->szblock_cnt == 0){
		return;
	}
	mgr->unlock(mgr, cmd);
	cmd->szblock_cnt = 0;
}

S32 ps3_r1x_lock_prepare_for_vd(struct ps3_instance *instance,
	struct scsi_device *sdev, U8 raid_level);

void ps3_r1x_lock_destory_for_vd(struct ps3_instance *instance,
	struct ps3_r1x_lock_mgr *mgr);

void ps3_r1x_conflict_queue_clean(struct ps3_scsi_priv_data *pri_data,
	S32 ret_code);

Bool ps3_r1x_conflict_queue_abort(struct ps3_cmd *cmd, struct scsi_cmnd *scmd);

void ps3_r1x_conflict_queue_target_reset(struct ps3_instance *instance,
	U16 target_id);

void ps3_r1x_conflict_queue_clean_all(struct ps3_instance *instance,
	S32 ret_code, Bool is_remove);

#endif 
