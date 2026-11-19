/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_CRASH_DUMP_H_
#define _PS3_CRASH_DUMP_H_

#ifndef _WINDOWS
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/sysfs.h>

#include <linux/mutex.h>
#include <linux/atomic.h>
#include <linux/workqueue.h>
#include <linux/pci.h>
#include <linux/irqreturn.h>
#else
#include "ps3_worker.h"
#endif
#include "ps3_platform_utils.h"
#include "ps3_htp.h"
#include "ps3_htp_def.h"

#define PS3_DUMP_DATA_MAX_NUM 512

#define WAIT_DUMP_COLLECT (1000)
#define WAIT_DUMP_TIMES_DEFAULT (300)
#define WAIT_DUMP_TIMES_MIN (10)
#define WAIT_DUMP_TIMES_MAX (3600)

#define WAIT_DUMP_DMA_DONE (200)
#ifndef _WINDOWS
#ifdef __KERNEL__
#define PS3_DUMP_FILE_DIR "/var/log"
#else
#define PS3_DUMP_FILE_DIR "/tmp"
#endif
#else
#define PS3_DUMP_FILE_DIR "c:"
#endif
#define PS3_DUMP_FILE_CORE_PREFIX "core-"
#define PS3_DUMP_FILE_FWLOG_PREFIX "fwlog-"
#define PS3_DUMP_FILE_BAR_PREFIX "bar-"

#define PS3_DUMP_STATUS_REG_DMA_FINISH_MASK (0x1 << 0)
#define PS3_DUMP_STATUS_REG_CRASH_DUMP_MASK (0x1 << 1)
#define PS3_DUMP_STATUS_REG_FW_DUMP_MASK (0x1 << 2)
#define PS3_DUMP_STATUS_REG_BAR_DUMP_MASK (0x1 << 3)
#define PS3_DUMP_STATUS_REG_CRASH_DUMP_TRIGGER_MASK (0x1 << 4)
#define PS3_DUMP_STATUS_REG_FW_DUMP_TRIGGER_MASK (0x2 << 4)
#define PS3_DUMP_STATUS_REG_BAR_DUMP_TRIGGER_MASK (0x3 << 4)
#define PS3_DUMP_STATUS_REG_ABORT_MASK (0x1 << 6)
#define PS3_DUMP_STATUS_REG_MASK (0x0F)
#define PS3_DUMP_STATUS_REG_INVALID_BITS_MASK (0x7F)

#define PS3_REG_TEST(val, mask) (((val) & (mask)) ? PS3_TRUE : PS3_FALSE)
#define PS3_REG_SET(val, mask) (((val) |= (mask)))
#define PS3_REG_CLR(val, mask) ((val) &= (~(mask)))

#define PS3_IOC_DUMP_SUPPORT(ins) (ins->dump_context.is_dump_support)

enum ps3_dump_work_status {
	PS3_DUMP_WORK_UNKNOWN,
	PS3_DUMP_WORK_RUNNING,
	PS3_DUMP_WORK_STOP,
	PS3_DUMP_WORK_DONE,
	PS3_DUMP_WORK_CANCEL,
};

enum ps3_dump_irq_handler_work_status {
	PS3_DUMP_IRQ_HANDLER_WORK_UNKNOWN,
	PS3_DUMP_IRQ_HANDLER_WORK_RUNNING,
	PS3_DUMP_IRQ_HANDLER_WORK_STOP,
	PS3_DUMP_IRQ_HANDLER_WORK_DONE,
	PS3_DUMP_IRQ_HANDLER_WORK_CANCEL,
};

enum ps3_dump_env {
	PS3_DUMP_ENV_UNKNOWN,
	PS3_DUMP_ENV_CLI,
	PS3_DUMP_ENV_NOTIFY,
	PS3_DUMP_ENV_COUNT,
};

enum ps3_dump_file_status {
	PS3_DUMP_FILE_UNKNOWN,
	PS3_DUMP_FILE_OPEN,
	PS3_DUMP_FILE_CLOSE,
	PS3_DUMP_FILE_MAX,
};

#define PS3_DUMP_FILE_DIR_LEN (128)
#define PS3_DUMP_FILE_NAME_LEN (256)

struct ps3_dump_file_info {
	unsigned char filename[PS3_DUMP_FILE_NAME_LEN];
#ifdef _WINDOWS
	HANDLE fp;
#else
	struct file *fp;
#endif
	unsigned long long file_size;
	unsigned int file_w_cnt;
	unsigned int file_status;
	unsigned int type;
	unsigned char reserved[4];
};

struct ps3_dump_context {
	unsigned char dump_dir[PS3_DUMP_FILE_DIR_LEN];
	unsigned long long dump_data_size;
	char dump_type;
	int dump_state;
	unsigned int dump_env;
	unsigned int dump_work_status;
	unsigned int dump_irq_handler_work_status;
	unsigned long long copyed_data_size;
	unsigned char *dump_dma_buf;
	dma_addr_t dump_dma_addr;
	unsigned int dump_dma_wait_times;
#ifdef _WINDOWS
	struct ps3_delay_worker dump_work;
	struct mutex dump_mutex;
#else
	struct workqueue_struct *dump_work_queue;
	struct delayed_work dump_work;

	struct workqueue_struct *dump_irq_handler_work_queue;
	struct work_struct dump_irq_handler_work;
	spinlock_t dump_irq_handler_lock;
	unsigned int dump_enabled;

#endif
	struct mutex dump_lock;
	struct ps3_instance *instance;
	struct ps3_dump_file_info dump_out_file;
	struct ps3_cmd *dump_pending_cmd;

	unsigned int dump_pending_send_times;
	unsigned int dump_type_times;
	unsigned int dump_state_times;
	unsigned char is_dump_support;
	unsigned char is_hard_recovered;

	unsigned char reserved[2];
};
static inline const char *ps3_dump_type_to_name(int dump_type)
{
	static const char * const name[] = { [PS3_DUMP_TYPE_UNKNOWN] = "NULL",
				      [PS3_DUMP_TYPE_CRASH] = "dump_crash",
				      [PS3_DUMP_TYPE_FW_LOG] = "fw_log",
				      [PS3_DUMP_TYPE_BAR_DATA] = "bar_data" };
	return name[dump_type];
}

extern int ps3_dump_init(struct ps3_instance *instance);
extern void ps3_dump_exit(struct ps3_instance *instance);
extern int ps3_dump_type_set(struct ps3_dump_context *ctxt, int type,
			     unsigned int env);
extern int ps3_dump_state_set(struct ps3_dump_context *ctxt, int state);

#ifndef _WINDOWS
extern struct ps3_dump_context *dev_to_dump_context(struct device *cdev);

void ps3_dump_work_stop(struct ps3_instance *instance);

#endif

int ps3_dump_dma_buf_alloc(struct ps3_instance *instance);

void ps3_dump_dma_buf_free(struct ps3_instance *instance);

void ps3_dump_detect(struct ps3_instance *instance);

#ifndef _WINDOWS
irqreturn_t ps3_dump_irq_handler(int virq, void *dev_id);

#endif
void ps3_dump_ctrl_set_int_ready(struct ps3_instance *instance);

unsigned char ps3_dump_is_trigger_log(struct ps3_instance *instance);

#endif
