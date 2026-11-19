/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_EVENT_H_
#define _PS3_EVENT_H_

#include "ps3_instance_manager.h"

#define PS3_EVENT_WORKER_POOL_SIZE (3)
#define PS3_WEB_FLAG_INIT_VALUE (0)
int ps3_event_context_init(struct ps3_instance *instance);
void ps3_event_context_exit(struct ps3_instance *instance);
const char *ps3_event_print(enum MgrEvtType event_type);
int ps3_event_subscribe(struct ps3_instance *instance);
int ps3_event_unsubscribe(struct ps3_instance *instance);
int ps3_soft_reset_event_resubscribe(struct ps3_instance *instance);
void ps3_event_handle(struct ps3_event_delay_work *ps3_delay_work);

int ps3_event_service(struct ps3_cmd *cmd, unsigned short reply_flags);
int ps3_event_delay_set(struct ps3_instance *instance, unsigned int delay);

void ps3_event_filter_table_get_raid(unsigned char *data);

void ps3_event_filter_table_get_hba(unsigned char *data);

void ps3_event_filter_table_get_switch(unsigned char *data);

void ps3_vd_pending_filter_table_build(unsigned char *data);

int ps3_fasync(int fd, struct file *filp, int mode);
int ps3_webSubscribe_context_init(struct ps3_instance *instance);
void ps3_webSubscribe_context_exit(struct ps3_instance *instance);
int ps3_webSubscribe_service(struct ps3_cmd *cmd, unsigned short reply_flags);
int ps3_web_subscribe(struct ps3_instance *instance);
int ps3_web_unsubscribe(struct ps3_instance *instance);
int ps3_soft_reset_web_resubscribe(struct ps3_instance *instance);
void ps3_web_cmd_clear(struct ps3_instance *instance);

#endif
