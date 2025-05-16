/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef CQM_CMD_H
#define CQM_CMD_H

#include <linux/types.h>

#include "cqm_object.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define CQM_CMD_TIMEOUT 10000 /* ms */

struct tag_cqm_cmd_buf *cqm_cmd_alloc(void *ex_handle);
void cqm_cmd_free(void *ex_handle, struct tag_cqm_cmd_buf *cmd_buf);
s32 cqm_send_cmd_box(void *ex_handle, u8 mod, u8 cmd, struct tag_cqm_cmd_buf *buf_in,
		     struct tag_cqm_cmd_buf *buf_out, u64 *out_param, u32 timeout,
		     u16 channel);
s32 cqm_lb_send_cmd_box(void *ex_handle, u8 mod, u8 cmd, u8 cos_id,
			struct tag_cqm_cmd_buf *buf_in, struct tag_cqm_cmd_buf *buf_out,
			u64 *out_param, u32 timeout, u16 channel);
s32 cqm_lb_send_cmd_box_async(void *ex_handle, u8 mod, u8 cmd,
			      u8 cos_id, struct tag_cqm_cmd_buf *buf_in,
			      u16 channel);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* CQM_CMD_H */
