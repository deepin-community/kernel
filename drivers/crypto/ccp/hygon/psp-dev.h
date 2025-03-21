/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * HYGON Platform Security Processor (PSP) driver interface
 *
 * Copyright (C) 2024 Hygon Info Technologies Ltd.
 *
 * Author: Liyang Han <hanliyang@hygon.cn>
 */

#ifndef __CCP_HYGON_PSP_DEV_H__
#define __CCP_HYGON_PSP_DEV_H__

#include <linux/mutex.h>
#include <linux/bits.h>
#include <linux/miscdevice.h>
#include <linux/pci.h>

#include "ring-buffer.h"
#include "sp-dev.h"

#include "../psp-dev.h"
#include "../sev-dev.h"

#ifdef CONFIG_HYGON_PSP2CPU_CMD
#define PSP_X86_CMD			BIT(2)
#define P2C_NOTIFIERS_MAX		16
#endif

/*
 * Hooks table: a table of function and variable pointers filled in
 * when psp init.
 */
extern struct hygon_psp_hooks_table {
	bool sev_dev_hooks_installed;
	struct mutex *sev_cmd_mutex;
	struct psp_misc_dev *psp_misc;
	bool psp_mutex_enabled;
	bool *psp_dead;
	int *psp_timeout;
	int *psp_cmd_timeout;
	int (*sev_cmd_buffer_len)(int cmd);
	int (*__sev_do_cmd_locked)(int cmd, void *data, int *psp_ret);
	int (*__sev_platform_init_locked)(int *error);
	int (*__sev_platform_shutdown_locked)(int *error);
	int (*sev_wait_cmd_ioc)(struct sev_device *sev,
				unsigned int *reg, unsigned int timeout);
	int (*sev_do_cmd)(int cmd, void *data, int *psp_ret);
	long (*sev_ioctl)(struct file *file, unsigned int ioctl, unsigned long arg);
} hygon_psp_hooks;

extern struct wait_queue_head psp_int_queue;
extern uint8_t psp_legacy_rb_supported;		// support legacy ringbuffer
extern uint8_t psp_rb_oc_supported;		// support overcommit
extern uint8_t psp_generic_rb_supported;	// support generic ringbuffer

#define PSP_MUTEX_TIMEOUT 60000
struct psp_mutex {
	uint64_t locked;
};

struct tkm_cmdresp_head {
	uint32_t buf_size; //including this header
	uint32_t cmdresp_size; //including this header
	uint32_t cmdresp_code;
} __packed;

struct tkm_device_info {
	uint32_t api_version;
	uint32_t fw_version;
	uint32_t kek_sm4_total;
	uint32_t isk_sm2_sign_total;
	uint32_t isk_sm2_enc_total;
	uint8_t chip_id[32];
} __packed;
struct tkm_cmdresp_device_info_get {
	struct tkm_cmdresp_head head;
	struct tkm_device_info dev_info;
} __packed;

struct queue_info {
	uint32_t head;   /* In|Out */
	uint32_t tail;   /* In */
	uint32_t mask;   /* In */
	uint64_t cmdptr_address;     /* In */
	uint64_t statval_address;    /* In */
	uint8_t  reserved[36];
} __packed;	// total 64 bytes
struct psp_ringbuffer_cmd_buf {
	struct queue_info high;
	struct queue_info low;
	uint8_t reserved[128];
} __packed;	// total 256 bytes

#define PSP_RB_IS_SUPPORTED(buildid)		(buildid >= 1913 && boot_cpu_has(X86_FEATURE_SEV))
#define PSP_RB_OC_IS_SUPPORTED(buildid)		(buildid >= 2167)
#define PSP_GRB_IS_SUPPORTED(buildid)		(buildid >= 2270)
#define PSP_CMD_STATUS_RUNNING			0xffff
#define PSP_RB_OVERCOMMIT_SIZE			1024
#define TKM_DEVICE_INFO_GET			0x1001

#define PSP_CMD_RING_BUFFER		0x304
#define PSP_DO_CMD_OP_PHYADDR	BIT(0)   // Input data as physical address
#define PSP_DO_CMD_OP_NOWAIT	BIT(1)   // No need to wait ioc
int psp_do_cmd_locked(int cmd, void *data, int *psp_ret, uint32_t op);

struct psp_dev_data {
	struct psp_mutex mb_mutex;
};

struct psp_misc_dev {
	struct kref refcount;
	struct psp_dev_data *data_pg_aligned;
	struct miscdevice dev_misc;
	struct miscdevice resource2_misc;
};

extern int psp_mutex_trylock(struct psp_mutex *mutex);

int hygon_psp_additional_setup(struct sp_device *sp);
void hygon_psp_exit(struct kref *ref);
int psp_mutex_lock_timeout(struct psp_mutex *mutex, uint64_t ms);
int psp_mutex_unlock(struct psp_mutex *mutex);
int fixup_hygon_psp_caps(struct psp_device *psp);
int sp_request_hygon_psp_irq(struct sp_device *sp, irq_handler_t handler,
			     const char *name, void *data);
/**
 * When PSP_DO_CMD_OP_NOWAIT is used with psp_do_cmd_locked,
 * psp_worker_register_notify must be called first to register async notify
 * for PSP worker bottom-half execution. Note: triggering worker bottom-half
 * always clears previous notify.
 */
void psp_worker_register_notify(work_func_t notify);

/**
 * psp generic ringbuffer implement.
 **/
uint32_t psp_ringbuffer_enqueue(struct csv_ringbuffer_queue *ringbuffer,
				uint32_t cmd, phys_addr_t phy_addr, uint16_t flags);
void psp_ringbuffer_dequeue(struct csv_ringbuffer_queue *ringbuffer,
				struct csv_cmdptr_entry *cmdptr, struct csv_statval_entry *statval,
				uint32_t num);
int psp_ringbuffer_queue_init(struct csv_ringbuffer_queue *ring_buffer);
void psp_ringbuffer_queue_free(struct csv_ringbuffer_queue *ring_buffer);
int psp_ringbuffer_get_newhead(uint32_t *hi_head, uint32_t *low_head);
void psp_ringbuffer_check_support(void);

/**
 * psp_do_ringbuffer_cmds_locked is a no-wait PSP operation.
 * Must register notify via psp_worker_register_notify before use.
 */
int psp_do_ringbuffer_cmds_locked(struct csv_ringbuffer_queue *ring_buffer, int *psp_ret);

#endif	/* __CCP_HYGON_PSP_DEV_H__ */
