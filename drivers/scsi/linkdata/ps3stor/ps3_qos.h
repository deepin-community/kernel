/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_QOS_H_
#define _PS3_QOS_H_

#include "ps3_platform_utils.h"
#include "ps3_htp.h"
#include "ps3_cmd_channel.h"
#include "ps3_kernel_version.h"

#define QOS_HIGH_PRI_EXCLUSIVE_CMD_COUNT 32
#define QOS_MGR_EXCLUSIVE_CMD_COUNT 64
#define PS3_QOS_DEFAULT_PD_QUOTA 40
#define PS3_QOS_SAS_PD_QUOTA 125
#define PS3_QOS_NVME_MEMBER_QUOTA 127
#define PS3_QOS_NVME_DIRECT_QUOTA 127
#define PS3_QOS_HBA_NVME_NORMAL_QUOTA 126
#define PS3_QOS_RAID_NVME_NORMAL_QUOTA 252
#define PS3_QOS_FUNC1_JBOD_VD_QUOTA 768
#define PS3_QOS_VD_EXCLUSIVE_CMD_COUNT 40
#define PS3_QOS_JBOD_EXCLUSIVE_CMD_COUNT 128
#define PS3_QOS_POLL_INTERVAL 2
#define PS3_QOS_WAITQ_TIMEOUT 5
#define PS3_QOS_HBA_MAX_CMD 944
#define PS3_QOS_CS_FUNC0_SHARE_CMD 80
#define PS3_QOS_CS_FUNC0_JBOD_VD_QUOTA 80
#define PS3_QOS_NOTIFY_CMD_COUNT 32
#define PS3_QOS_FUNC0_PD_WORKQ_COUNT 1
#define PS3_QOS_FUNC1_PD_WORKQ_COUNT 4
#define PS3_QOS_POLL_CMD_COUNT 16

#define PS3_CMD_NEED_QOS(cmd)                                                  \
	((cmd)->cmd_word.direct == PS3_CMDWORD_DIRECT_NORMAL ||                \
	 (cmd)->cmd_word.direct == PS3_CMDWORD_DIRECT_ADVICE)

#define PS3_QOS_INITED(instance) ((instance)->qos_context.inited)

#define PS3_NEED_UPDATE_PD_QUOTAS(pd_mgr, pd_entry)               \
	(((pd_mgr)->dev_type != (pd_entry)->dev_type) ||           \
	((pd_mgr)->pd_quota != (pd_entry)->normal_quota) ||       \
	((pd_mgr)->direct_quota != (pd_entry)->direct_quota)      \
	)

enum ps3_qos_fifo_type {
	PS3_QOS_CMDQ_0,
	PS3_QOS_CMDQ_1,
	PS3_QOS_CMDQ_2,
	PS3_QOS_CMDQ_3,
	PS3_QOS_MGRQ,
	PS3_QOS_TFIFO,
	PS3_QOS_FIFO_TYPE_MAX
};

enum ps3_qos_cmd_flag {
	PS3_QOS_CMD_INIT = 0,
	PS3_QOS_CMD_IN_PD,
	PS3_QOS_CMD_IN_VD,
	PS3_QOS_CMD_IN_FRAME,
	PS3_QOS_CMD_IN_MGR,
	PS3_QOS_CMD_IN_CMDQ0,
	PS3_QOS_CMD_IN_CMDQ1,
	PS3_QOS_CMD_IN_CMDQ2,
	PS3_QOS_CMD_IN_CMDQ3
};

enum ps3_qos_quota_adjust_flag {
	PS3_QOS_QUOTA_ADJUST_DEFULAT = 0,
	PS3_QOS_QUOTA_ADJUST_QFULL,
	PS3_QOS_QUOTA_ADJUST_UP,
	PS3_QOS_QUOTA_ADJUST_INVALID
};

enum ps3_qos_vd_change_type {
	PS3_QOS_VD_TYPE_CHANGE_TO_HDD,
	PS3_QOS_VD_TYPE_CHANGE_TO_SSD,
	PS3_QOS_VD_TYPE_CHANGE_INVALID
};

struct qos_wait_queue {
	struct list_head wait_list;
	unsigned int count;
	unsigned short id;
	spinlock_t *rsc_lock;
	int *free_rsc;
	atomic_t *used_rsc;
	unsigned int *total_waited_cnt;
	unsigned short can_resend;
	unsigned short has_resend;
	unsigned long last_sched_jiffies;
};

struct ps3_qos_vd_mgr {
	unsigned char valid;
	unsigned short id;
	unsigned char workq_id;
	atomic_t vd_quota;
	spinlock_t rsc_lock;
	struct qos_wait_queue vd_quota_wait_q;
	atomic_t exclusive_cmd_cnt;
	atomic_t share_cmd_used;
	struct PS3VDEntry *vd_entry;
	struct ps3_instance *instance;
	struct work_struct resend_work;
	unsigned long long last_sched_jiffies;
};

struct ps3_qos_pd_mgr {
	atomic_t valid;
	unsigned char workq_id;
	unsigned char dev_type;
	unsigned char clearing;
	unsigned short disk_id;
	unsigned short vd_id;
	int pd_quota;
	atomic_t pd_used_quota;
	struct ps3_instance *instance;
	struct work_struct resend_work;
	unsigned long long last_sched_jiffies;
	struct qos_wait_queue *waitqs;
	unsigned short waitq_cnt;
	unsigned int total_wait_cmd_cnt;
	spinlock_t rc_lock;
	unsigned short poll_que_id;
	unsigned short poll_start_que_id;
	unsigned short poll_cmd_cnt;
	spinlock_t direct_rsc_lock;
	int direct_quota;
	atomic_t direct_used_quota;
	unsigned int total_waited_direct_cmd;
	atomic_t processing_cnt;
	spinlock_t adjust_quota_lock;
	int adjust_max_quota;
	int adjust_min_quota;
	int pd_init_quota;
	unsigned int wdt_awake;
};

struct ps3_qos_pd_reply_info {
	unsigned char *processed_pd_state;
	unsigned short *processed_pd_id;
	unsigned short processed_pd_cnt;
};

struct ps3_qos_pd_context {
	struct ps3_qos_pd_mgr *qos_pd_mgrs;
	struct workqueue_struct **work_queues;
	unsigned short sas_sata_hdd_quota;
	unsigned short sas_sata_ssd_quota;
	unsigned short nvme_normal_quota;
	unsigned short nvme_direct_quota;
	atomic_t workq_id_cnt;
	unsigned char workq_count;
	struct ps3_qos_pd_reply_info *reply_info;
	unsigned int reply_info_cnt;
};

struct ps3_qos_vd_context {
	struct ps3_qos_vd_mgr *qos_vd_mgrs;
	struct workqueue_struct **work_queues;
	unsigned short jbod_exclusive_cnt;
	unsigned short vd_exclusive_cnt;
	unsigned char workq_count;
	unsigned char inited;
};

struct ps3_qos_tg_context {
	unsigned int share;
	unsigned int mgr_exclusive_cnt;
	unsigned short high_pri_exclusive_cnt;
	atomic_t mgr_free_cnt;
	atomic_t mgr_share_used;
	atomic_t share_free_cnt;
	struct qos_wait_queue mgr_cmd_wait_q;
	struct qos_wait_queue *vd_cmd_waitqs;
	unsigned int total_wait_cmd_cnt;
	unsigned char poll_vd_id;
	spinlock_t lock;
	struct ps3_instance *instance;
	struct workqueue_struct *work_queue;
	struct work_struct resend_work;
	unsigned long long last_sched_jiffies;
};

struct ps3_qos_ops {
	int (*qos_init)(struct ps3_instance *instance);
	void (*qos_exit)(struct ps3_instance *instance);
	void (*qos_vd_init)(struct ps3_instance *instance,
			    struct PS3VDEntry *vd_entry);
	void (*qos_vd_reset)(struct ps3_instance *instance,
			     unsigned short disk_id);
	unsigned char (*qos_decision)(struct ps3_cmd *cmd);
	void (*qos_cmd_update)(struct ps3_cmd *cmd, unsigned char in_reply);
	void (*qos_waitq_notify)(struct ps3_instance *instance, unsigned short isr_sn);
	unsigned char (*qos_pd_resend_check)(struct ps3_cmd *cmd);
	unsigned char (*qos_waitq_abort)(struct ps3_cmd *cmd);
	void (*qos_vd_clean)(struct ps3_instance *instance,
			     struct ps3_scsi_priv_data *pri_data, int ret_code);
	void (*qos_pd_clean)(struct ps3_instance *instance,
			     struct ps3_scsi_priv_data *priv_data,
			     int ret_code);
	void (*qos_waitq_clear)(struct ps3_instance *instance, int ret_code);
	void (*qos_waitq_poll)(struct ps3_instance *instance);
	void (*qos_reset)(struct ps3_instance *instance);
};

int ps3_hba_qos_init(struct ps3_instance *instance);

void ps3_hba_qos_exit(struct ps3_instance *instance);

void ps3_qos_vd_init(struct ps3_instance *instance,
		     struct PS3VDEntry *vd_entry);

void ps3_qos_vd_reset(struct ps3_instance *instance, unsigned short disk_id);

struct ps3_qos_pd_mgr *ps3_qos_pd_mgr_init(struct ps3_instance *instance,
					   struct ps3_pd_entry *pd_entry);

void ps3_qos_pd_mgr_reset(struct ps3_instance *instance, unsigned short pd_id);

void ps3_qos_vd_member_change(struct ps3_instance *instance,
			      struct ps3_pd_entry *pd_entry,
			      struct scsi_device *sdev,
			      unsigned char is_vd_member);

int ps3_qos_decision(struct ps3_cmd *cmd);

void ps3_qos_cmd_update(struct ps3_instance *instance,
		struct ps3_cmd *cmd, unsigned char in_reply);

void ps3_qos_waitq_notify(struct ps3_instance *instance, unsigned short isr_sn);

unsigned char ps3_qos_waitq_abort(struct ps3_cmd *aborted_cmd);

void ps3_qos_device_clean(struct ps3_instance *instance,
			  struct ps3_scsi_priv_data *pri_data, int ret_code);

void ps3_qos_disk_del(struct ps3_instance *instance,
		      struct ps3_scsi_priv_data *priv_data);

void ps3_qos_vd_member_del(struct ps3_instance *instance,
			   struct PS3DiskDevPos *dev_pos);

void ps3_qos_hard_reset(struct ps3_instance *instance);

void ps3_qos_waitq_clear_all(struct ps3_instance *instance, int resp_status);

void ps3_qos_waitq_poll(struct ps3_instance *instance);

void ps3_qos_close(struct ps3_instance *instance);

void ps3_qos_open(struct ps3_instance *instance);

void ps3_hba_qos_prepare(struct ps3_instance *instance);

void ps3_raid_qos_prepare(struct ps3_instance *instance);

#define PS3_QOS_CMDQ_COUNT 4
#define PS3_QOS_CMDQ_DEPTH 4096
#define PS3_QOS_MGRQ_DEPTH 1024
#define PS3_QOS_HIGH_PRI_MGR_CMD_COUNT 32
struct ps3_qos_softq_mgr {
	unsigned short id;
	struct ps3_instance *instance;
	spinlock_t rc_lock;
	atomic_t free_cnt;
	struct qos_wait_queue *waitqs;
	unsigned short waitq_cnt;
	unsigned int total_wait_cmd_cnt;
	struct workqueue_struct *work_queue;
	struct work_struct resend_work;
	unsigned long long last_sched_jiffies;
	unsigned short poll_cmd_cnt;
	unsigned short poll_que_id;
};

struct ps3_qos_cq_context {
	struct ps3_qos_softq_mgr mgrq;
	struct ps3_qos_softq_mgr *cmdqs;
	unsigned char cmdq_cnt;
	unsigned int mgrq_depth;
	unsigned int cmdq_depth;
};

struct ps3_qos_context {
	struct ps3_qos_ops opts;
	struct ps3_qos_pd_context pd_ctx;
	struct ps3_qos_vd_context vd_ctx;
	struct ps3_qos_tg_context tg_ctx;
	struct ps3_qos_cq_context cq_ctx;
	unsigned short max_vd_count;
	unsigned short max_pd_count;
	unsigned char poll_count;
	unsigned char qos_switch;
	unsigned char inited;
};

int ps3_raid_qos_init(struct ps3_instance *instance);

void ps3_raid_qos_exit(struct ps3_instance *instance);

int ps3_qos_init(struct ps3_instance *instance);

void ps3_qos_exit(struct ps3_instance *instance);

void ps3_qos_adjust_pd_rsc(struct scsi_device *sdev,
			   struct ps3_instance *instance, int reason);

void ps3_qos_vd_attr_change(struct ps3_instance *instance,
			    struct PS3VDEntry *vd_entry_old,
			    struct PS3VDEntry *vd_entry);

void ps3_qos_pd_rsc_init(struct ps3_qos_pd_mgr *qos_pd_mgr,
			 struct ps3_pd_entry *pd_entry);

#if defined(PS3_SUPPORT_LINX80)
void ps3_linx80_vd_member_change(struct ps3_instance *instance,
				 struct ps3_pd_entry *pd_entry);
#endif

#endif
