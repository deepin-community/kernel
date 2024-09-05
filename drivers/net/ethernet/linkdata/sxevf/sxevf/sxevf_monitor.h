#ifndef __SXEVF_MONITOR_H__
#define __SXEVF_MONITOR_H__

struct sxevf_adapter;

enum sxevf_monitor_task_state {
	SXEVF_MONITOR_WORK_INITED,

	SXEVF_MONITOR_WORK_SCHED,

	SXEVF_RESET_REQUESTED,

	SXEVF_LINK_CHECK_REQUESTED,

	SXEVF_RING_REASSIGN_REQUESTED,

	SXEVF_NETDEV_DOWN,
};

struct sxevf_monitor_context {
	struct timer_list timer; 
	struct work_struct work; 
	unsigned long state;     
};

struct sxevf_link_info {
	u8 is_up :1;       
	u8 need_reinit :1; 
	u8 link_enable :1;  
	u8 reservd :5;     
	u32  speed;        

	unsigned long check_timeout;
};

void sxevf_task_timer_trigger(struct sxevf_adapter *adapter);

void sxevf_monitor_init(struct sxevf_adapter *adapter);

void sxevf_monitor_work_schedule(struct sxevf_adapter *adapter);

#endif
