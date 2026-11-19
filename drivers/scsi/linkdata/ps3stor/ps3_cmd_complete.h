/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_CMD_COMPLETE_H_
#define _PS3_CMD_COMPLETE_H_

#include "ps3_htp_def.h"
#include "ps3_irq.h"
#include "ps3_instance_manager.h"
#include "ps3_inner_data.h"

int ps3_cmd_complete(struct ps3_irq *irq);

int ps3_cmd_reply_polling(struct ps3_instance *instance, struct ps3_cmd *cmd,
			  unsigned long timeout, unsigned char ignore);

int ps3_cmd_reply_polling_when_recovery(struct ps3_instance *instance,
					struct ps3_cmd *cmd,
					unsigned long timeout);

void ps3_all_reply_fifo_complete(struct ps3_instance *instance);

void ps3_can_queue_depth_update(struct ps3_instance *instance);

#endif
