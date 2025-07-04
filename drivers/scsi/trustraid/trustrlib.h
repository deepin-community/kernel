#ifndef _TRUSTRLIB_H_
#define _TRUSTRLIB_H_

#include "smartpqi.h"

#if	defined(CONFIG_LOONGARCH) || defined(CONFIG_SW64)
#define KFEATURE_ENABLE_TRUSTRLIB
#endif

struct trustrlib_template {
	void (*invalid_response)(void *, int);
	int (*handle_response)(void *, void *, void *);
	void (*inc_scmd_residual)(void *);
	void (*scsi_done)(void *);
};

struct trustrlib_ops {
	void (*set_aio_request_id)(void *, void *);
	void (*set_aio_r1_request_id)(void *, void *);
	void (*set_aio_r56_request_id)(void *, void *);
	void (*set_raid_request_id)(void *, void *);
	u16 (*get_and_update_io_request)(void *, void *);
	void (*free_io_request)(void *);
	int (*eh_timed_out)(void *);
	void (*reset_io_request_index)(void *);
	int (*process_io_intr)(void *, void *);
	void (*set_sg_table_size)(void *, void *);
	u16 (*alloc_hw_queue)(void *, u16);
	void (*start_io)(void *, void *, int);
	int (*init_ctrl)(struct trustrlib_template *, void *, void *);
	void (*uninit_ctrl)(void *);
	void (*init_works)(void *);
	void (*uninit_works)(void *);
	int (*init_queue_group)(void *);
	int (*init_io_request)(void *);
	void (*uninit_queue_group)(void *);
	void (*uninit_io_request)(void *);
};

extern struct trustrlib_ops trustrlib_operations;

struct trustrlib_ctrl_info {
	struct pqi_ctrl_info	*ctrl_info;
	struct timer_list	poll_complete_queue_timer;
	struct work_struct	check_iq_hang_work;
	struct pqi_queue_group	*check_queue_group;
	struct delayed_work	recheck_iq_hang_work;
	struct pqi_queue_group	*recheck_queue_group;
	struct trustrlib_template *template;
};

struct trustrlib_queue_group {
	atomic_t	inflight_num;
	spinlock_t	complete_lock;
	u8	iq_hanging[2];
};

struct trustrlib_io_request {
	spinlock_t	lock;
	atomic_t	in_interrupt_or_timedout;
	u8	timedout_count;
};

/* make sure ctrl_info->max_outstanding_requests <= 1024 */
#define PQI_IO_REQUEST_INDEX_MASK	0x3ff
#define PQI_IO_REQUEST_INDEX_MAGIC(index)	(index >> 10)

#define PQI_IO_REQUEST_INDEX_UPDATE_MAGIC(index) \
	do { \
		index = ((((index >> 10) + 1) & 0x3f) << 10) + (index & PQI_IO_REQUEST_INDEX_MASK); \
	} while(0)

#define	PQI_IO_REQUEST_INDEX_SUB_MAGIC(index, i) \
	(((((index >> 10) - i) & 0x3f) << 10) + (index & PQI_IO_REQUEST_INDEX_MASK))

#ifdef KFEATURE_ENABLE_TRUSTRLIB
bool trustrlib_match_cpu(void);
#endif

#endif