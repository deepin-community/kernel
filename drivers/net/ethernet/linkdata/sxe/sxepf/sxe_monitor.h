#ifndef __SXE_MONITOR_H__
#define __SXE_MONITOR_H__

#include <linux/types.h>
#include <linux/netdevice.h>

struct sxe_adapter;

enum sxe_monitor_task_state {
	SXE_MONITOR_WORK_INITED,

	SXE_MONITOR_WORK_SCHED,

	SXE_RESET_REQUESTED,

	SXE_LINK_CHECK_REQUESTED,

	SXE_FNAV_REQUIRES_REINIT,

	SXE_SFP_NEED_RESET,

	SXE_LINK_NEED_CONFIG,

	SXE_LINK_SPEED_CHANGE,
};

struct sxe_monitor_context {
	struct timer_list timer; 
	struct work_struct work; 
	unsigned long state;     
};

struct sxe_link_info {
	bool is_up;                             
	u32  speed;                             
	struct mutex carrier_mutex;             

	unsigned long check_timeout;
	unsigned long sfp_reset_timeout;
	unsigned long last_lkcfg_time;
	unsigned long sfp_multispeed_time;
};

void sxe_monitor_init(struct sxe_adapter *adapter);

void sxe_monitor_work_schedule(struct sxe_adapter *adapter);

void sxe_task_timer_trigger(struct sxe_adapter *adapter);

void sxe_sfp_reset_task_submit(struct sxe_adapter *adapter);

void sxe_work_cb(struct work_struct *work);
#endif 