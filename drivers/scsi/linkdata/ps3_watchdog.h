
#ifndef _PS3_WATCHDOG_H_
#define _PS3_WATCHDOG_H_

#ifndef _WINDOWS
#include <linux/mutex.h>
#include <linux/atomic.h>
#include <linux/workqueue.h>
#else
#include "ps3_worker.h"
#endif

#include "ps3_htp_def.h"
#include "ps3_instance_manager.h"
#define PS3_WATCHDOG_INTERVAL			(1000)	

struct ps3_watchdog_context {
#ifndef _WINDOWS
	struct delayed_work watchdog_work;		
	struct workqueue_struct *watchdog_queue;	
	struct ps3_instance *instance;			
#else
	struct ps3_delay_worker watchdog_work;
#endif
	Bool is_stop;
	Bool is_halt;
};

S32 ps3_watchdog_start(struct ps3_instance *instance);
void ps3_watchdog_stop(struct ps3_instance *instance);

#endif
