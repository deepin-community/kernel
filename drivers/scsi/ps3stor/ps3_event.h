
#ifndef _PS3_EVENT_H_
#define _PS3_EVENT_H_

#include "ps3_instance_manager.h"

#define PS3_EVENT_WORKER_POOL_SIZE (3)
#define PS3_WEB_FLAG_INIT_VALUE (0)
S32 ps3_event_context_init(struct ps3_instance *instance);
void ps3_event_context_exit(struct ps3_instance *instance);
const char *ps3_event_print(MgrEvtType_e event_type);
S32 ps3_event_subscribe(struct ps3_instance *instance);
S32 ps3_event_unsubscribe(struct ps3_instance *instance);
S32 ps3_soft_reset_event_resubscribe(struct ps3_instance *instance);

S32 ps3_event_service(struct ps3_cmd *cmd, U16 reply_flags);
S32 ps3_event_delay_set(struct ps3_instance *instance, U32 delay);

void ps3_event_filter_table_get_raid(U8 *data);

void ps3_event_filter_table_get_hba(U8 *data);

void ps3_event_filter_table_get_switch(U8 *data);

void ps3_vd_pending_filter_table_build(U8 *data);

S32 ps3_fasync(int fd, struct file *filp, int mode);
S32 ps3_webSubscribe_context_init(struct ps3_instance *instance);
void ps3_webSubscribe_context_exit(struct ps3_instance *instance);
S32 ps3_webSubscribe_service(struct ps3_cmd *cmd, U16 reply_flags);
S32 ps3_web_subscribe(struct ps3_instance *instance);
S32 ps3_web_unsubscribe(struct ps3_instance *instance);
S32 ps3_soft_reset_web_resubscribe(struct ps3_instance *instance);
void ps3_web_cmd_clear(struct ps3_instance *instance);

#endif
