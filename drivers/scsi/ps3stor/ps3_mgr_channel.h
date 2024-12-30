
#ifndef _PS3_MGR_CHANNEL_H_
#define _PS3_MGR_CHANNEL_H_

#include "ps3_cmd_channel.h"

S32 ps3_cmd_send_sync(struct ps3_instance *instance, struct ps3_cmd *cmd);
S32 ps3_unload_cmd_send_sync(struct ps3_instance *instance, struct ps3_cmd *cmd);
S32 ps3_block_cmd_wait(struct ps3_instance *instance, struct ps3_cmd *cmd, ULong timeout);
S32 ps3_cmd_send_async(struct ps3_instance *instance, struct ps3_cmd *cmd,
S32 (*cmd_receive_cb)(struct ps3_cmd *, U16));
S32 ps3_cmd_no_block_send(struct ps3_instance *instance, struct ps3_cmd *cmd);

#ifndef _WINDOWS
void ps3_wait_cmd_for_completion_interrupt(struct ps3_instance *instance, struct ps3_cmd *cmd);
#endif

static inline PS3RespFrame_u * ps3_cmd_resp_frame_get(struct ps3_cmd *cmd){
	return cmd->resp_frame;
}
static inline U32 ps3_cmd_resp_status(struct ps3_cmd *cmd){
	return le32_to_cpu(cmd->resp_frame->normalRespFrame.respStatus);
};

#endif
