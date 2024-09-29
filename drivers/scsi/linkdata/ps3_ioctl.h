
#ifndef _PS3_IOCTL_H_
#define _PS3_IOCTL_H_

#ifdef _WINDOWS
#include "ps3_def.h"
#endif
#include "ps3_instance_manager.h"
#include "ps3_cmd_channel.h"
#include "ps3_htp_ioctl.h"

#define PS3_MAX_IOCTL_CMDS  3  

#ifdef _WINDOWS
U8 ps3_ioctl_start_io(_In_ struct ps3_instance *instance, _In_ PSCSI_REQUEST_BLOCK Srb);
U8 ps3_ioctl_build_io(_In_ struct ps3_instance *instance, _In_ PSCSI_REQUEST_BLOCK Srb);
#else
long ps3_ioctl_fops(struct file *file, U32 cmd, ULong arg);
#endif

S32 ps3_ioctl_init(struct ps3_instance *instance, S32 cmd_num);

void ps3_ioctl_buff_release(struct ps3_cmd *cmd);

inline static void ps3_ioctl_req_frame_build(struct ps3_cmd *cmd)
{
	PS3MgrReqFrame_s* mgr_req = &cmd->req_frame->mgrReq;

	mgr_req->reqHead.traceID = cmd->trace_id;
	mgr_req->reqHead.cmdType = PS3_CMD_IOCTL;
	mgr_req->reqHead.cmdSubType = 0;
	mgr_req->reqHead.cmdFrameID = cmd->index;
	mgr_req->reqHead.timeout = 0;
	mgr_req->timeout = 0;
	mgr_req->syncFlag = 1;
}

inline static void ps3_ioctl_cmd_word_build(struct ps3_instance *instance,
	struct ps3_cmd *cmd, U16 cmd_frame_id)
{
	struct PS3CmdWord* cmd_word = &cmd->cmd_word;
	memset(cmd_word, 0, sizeof(*cmd_word));

	(void)instance;

	cmd_word->type = PS3_CMDWORD_TYPE_MGR;
	cmd_word->direct = PS3_CMDWORD_DIRECT_NORMAL;
	cmd_word->cmdFrameID = cmd_frame_id;
#ifndef _WINDOWS
	cmd_word->isrSN = ps3_msix_index_get(cmd, 1);
#endif
}

S32 ps3_ioctl_callback_proc(struct ps3_cmd *cmd, U8 reply_flags);

void ps3_ioctl_clean(struct ps3_instance *instance);

void ps3_ioctl_buff_bit_pos_update(struct ps3_cmd *cmd);

#endif

