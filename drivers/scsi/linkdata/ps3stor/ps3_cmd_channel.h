/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_CMD_CHANNEL_H_
#define _PS3_CMD_CHANNEL_H_

#include "ps3_htp.h"
#include "ps3_htp_reqframe.h"

#ifdef _WINDOWS
#include "ps3_def.h"
#include "ps3_cmd_adp.h"

#endif
#include "ps3_platform_utils.h"
#include "ps3_inner_data.h"
#include "ps3_driver_log.h"
#include "ps3_htp_ioctl.h"
#include "ps3_kernel_version.h"

#define PS3_DEFAULT_REQ_FRAME_SIZE (256)

#define PS3_WAIT_SCSI_CMD_DONE_COUNT (1200)

enum {
	DMA_ALIGN_BYTES_4K = 4096,
	DMA_ALIGN_BYTES_2K = 2048,
	DMA_ALIGN_BYTES_1K = 1024,
	DMA_ALIGN_BYTES_512 = 512,
	DMA_ALIGN_BYTES_256 = 256,
	DMA_ALIGN_BYTES_128 = 128,
	DMA_ALIGN_BYTES_64 = 64,
	DMA_ALIGN_BYTES_16 = 16,
	DMA_ALIGN_BYTES_4 = 4,
};

enum {
	PS3_CMD_STATE_INIT = 0,
	PS3_CMD_STATE_PROCESS = 1,
	PS3_CMD_STATE_COMPLETE = 2,
	PS3_CMD_STATE_DEAD = 3,
};
enum {
	PS3_CMD_FLAG_SOFTRESET = 1,
	PS3_CMD_FLAG_HARDRESET = 2,
};

enum {
	PS3_R1X_MODE_NORMAL = 0,
	PS3_R1X_MODE_PERF = 1,
};

#define PS3_CMD_EXT_BUF_DEFAULT_SIZE (4096)
#define PS3_CMD_EXT_BUF_SIZE_MGR (4096)

#define PS3_MIN_SCSI_CMD_COUNT (4096)

static inline const char *namePS3CmdState(unsigned int s)
{
	static const char * const myNames[] = {
		[PS3_CMD_STATE_INIT] = "PS3_CMD_STATE_INIT",
		[PS3_CMD_STATE_PROCESS] = "PS3_CMD_STATE_PROCESS",
		[PS3_CMD_STATE_COMPLETE] = "PS3_CMD_STATE_COMPLETE",
		[PS3_CMD_STATE_DEAD] = "PS3_CMD_STATE_DEAD"
	};

	if (s > PS3_CMD_STATE_DEAD)
		return "PS3_CMD_STATE_INVALID";

	return myNames[s];
}

struct ps3_cmd_state_t {
	unsigned char state;
	unsigned char reset_flag;
	unsigned char reserived[6];
	spinlock_t lock;
};
union ps3_scsi_cdb_option {
	struct {
		unsigned char non_ncq : 1;
		unsigned char reserved0 : 1;
		unsigned char reserved1 : 1;
		unsigned char fua : 1;
		unsigned char dpo : 1;
		unsigned char protect : 3;
	};
	unsigned char option;
};

struct ps3_scsi_io_attr {
	unsigned char is_retry_cmd;
	unsigned char direct_flag;
	unsigned char seq_flag;
	unsigned char dev_type;
	union {
		struct {
			unsigned char rw_flag : 7;
			unsigned char is_confilct_check : 1;
		};
		unsigned char rw_type;
	};
	unsigned int num_blocks;
	unsigned int lba_lo;
	unsigned int lba_hi;
	const struct ps3_pd_entry *pd_entry;
	const struct ps3_pd_entry *peer_pd_entry;
	const struct PS3VDEntry *vd_entry;

	unsigned long long plba;
	unsigned long long plba_back;
	unsigned char span_idx;
	unsigned char span_pd_idx;
	unsigned short disk_id;
	unsigned char is_use_frontend_prp;
	unsigned char span_pd_idx_p;
	unsigned char span_pd_idx_q;
	unsigned char is_force_normal : 1;
#ifdef PS3_NCQ_PRIO_SUPPORT
	unsigned char is_ncq_prio_enable:1;
	unsigned char reserved:6;
#else
	unsigned char reserved:7;
#endif
	union ps3_scsi_cdb_option cdb_opts;
	unsigned char cdb[PS3_FRAME_CDB_BUFLEN];
	unsigned int sgl_buf_len;
	unsigned char target_pd_state;
	unsigned int reserved1;
};

struct ps3_ioctl_transient {
	unsigned short sge_num;
	unsigned char reserved[6];
	void *transient_buff[PS3_MAX_IOCTL_SGE_NUM];
};

#define PS3_QOS_MAX_PD_IN_VD (17)
struct ps3_qos_member_pd_info {
	unsigned short flat_disk_id;
	unsigned short strip_count;
	unsigned char get_quota;
};

#define PS3_QOS_MAX_CMDQ_ONE_CMD 2
struct ps3_qos_cmdq_info {
	unsigned char que_id;
	unsigned char get_rc;
};

struct ps3_cmd {
	union PS3ReqFrame *req_frame;
	unsigned long long req_frame_phys;
	void *ext_buf;
	unsigned long long ext_buf_phys;
	union PS3RespFrame *resp_frame;
	unsigned long long resp_frame_phys;
	struct ps3_instance *instance;
	union {
		struct PS3CmdWord cmd_word;
		struct PS3InitCmdWord init_cmd_word;
		unsigned long long cmd_word_value;
	};

	struct scsi_cmnd *scmd;

#ifndef _WINDOWS
	struct list_head cmd_list;
#else
	SCSI_REQUEST_BLOCK * srb;
	struct list_head cmd_list;
#endif

	unsigned long long trace_id;
	unsigned short index;
	unsigned char no_reply_word;
	unsigned char is_force_polling;
	unsigned char is_got_r1x;
	unsigned char is_inserted_c_q;
	unsigned char is_r1x_aborting;
	unsigned char is_r1x_scsi_complete;
	unsigned short r1x_read_pd;
	struct ps3_cmd *r1x_peer_cmd;
	unsigned char is_aborting;
	unsigned char r1x_reply_flag;
	unsigned char qos_processing;
	unsigned int os_sge_map_count;
	struct ps3_cmd_state_t cmd_state;
	unsigned short time_out;
	unsigned char is_interrupt;
	unsigned char szblock_cnt;
	unsigned int retry_cnt;
	void *node_buff;
#ifdef _WINDOWS
	KEVENT sync_done;
#else
	struct completion sync_done;
#endif
	int (*cmd_send_cb)(struct ps3_instance *, struct ps3_cmd *,
			   unsigned short);
	int (*cmd_receive_cb)(struct ps3_cmd *cmd, unsigned short reply_flags);
	struct ps3_ioctl_transient *transient;
	struct ps3_scsi_io_attr io_attr;
#ifdef _WINDOWS
	struct scsi_cmnd scmd_imp;
#endif
	struct PS3ReplyWord reply_word;

	struct list_head qos_list;
	struct ps3_qos_member_pd_info target_pd[PS3_QOS_MAX_PD_IN_VD];
	struct ps3_qos_cmdq_info cmdq_info[PS3_QOS_MAX_CMDQ_ONE_CMD];
	unsigned short target_pd_count;
	unsigned short first_over_quota_pd_idx;
	unsigned char qos_waitq_flag;
	unsigned char cmdq_count;
	unsigned char flighting;
	unsigned char vd_rd_outstand_flag;
	unsigned char vd_wr_outstand_flag;
};

struct ps3_cmd_context {
	unsigned int max_cmd_count;
	unsigned int max_scsi_cmd_count;
	unsigned int max_mgr_cmd_count;
	unsigned int max_prp_count;
	struct ps3_cmd **cmd_buf;

	dma_addr_t init_frame_buf_phys;
	unsigned char *init_frame_buf;
	dma_addr_t init_filter_table_phy_addr;
	unsigned char *init_filter_table_buff;
#ifndef _WINDOWS
	struct dma_pool *req_frame_dma_pool;
#endif
	unsigned int req_frame_buf_size;
	dma_addr_t req_frame_buf_phys;
	unsigned char *req_frame_buf;
#ifndef _WINDOWS
	struct dma_pool *response_frame_dma_pool;
#endif
	unsigned int response_frame_buf_size;
	dma_addr_t response_frame_buf_phys;
	unsigned char *response_frame_buf;
#ifndef _WINDOWS
	struct dma_pool *ext_buf_dma_pool;
#endif
	unsigned int ext_buf_size;
	unsigned int ext_sge_frame_count;
#ifndef _WINDOWS
	struct dma_pool *mgr_ext_buf_dma_pool;
#endif
	unsigned int mgr_ext_buf_size;

	dma_addr_t init_frame_sys_info_phys;
	unsigned char *init_frame_sys_info_buf;
	unsigned char sgl_mode_support;
	unsigned char reserved0[1];
	unsigned short max_host_sge_count;

#ifndef _WINDOWS
	struct list_head mgr_cmd_pool;
	struct list_head task_cmd_pool;
	struct list_head r1x_scsi_cmd_pool;
	spinlock_t mgr_pool_lock;
	spinlock_t task_pool_lock;
	spinlock_t r1x_scsi_pool_lock;
#else
	struct list_head mgr_cmd_pool;
	struct list_head task_cmd_pool;
	struct list_head scsi_cmd_pool;
	spinlock_t mgr_pool_lock;
	spinlock_t task_pool_lock;
	spinlock_t scsi_pool_lock;

	atomic64_t trace_id;
#endif
	unsigned short max_r1x_cmd_count;
	unsigned char reserved1[6];
};

int ps3_cmd_context_init(struct ps3_instance *instance);

void ps3_cmd_context_exit(struct ps3_instance *instance);

#ifndef _WINDOWS
struct ps3_cmd *ps3_scsi_cmd_alloc(struct ps3_instance *instance,
				   unsigned int tag);
#else
struct ps3_cmd *ps3_scsi_cmd_alloc(struct ps3_instance *instance);
#endif
int ps3_scsi_cmd_free(struct ps3_cmd *cmd);

struct ps3_cmd *ps3_mgr_cmd_alloc(struct ps3_instance *instance);

int ps3_mgr_cmd_free_nolock(struct ps3_instance *instance, struct ps3_cmd *cmd);
int ps3_mgr_cmd_free(struct ps3_instance *instance, struct ps3_cmd *cmd);

struct ps3_cmd *ps3_task_cmd_alloc(struct ps3_instance *instance);

int ps3_task_cmd_free(struct ps3_instance *instance, struct ps3_cmd *cmd);

int ps3_reply_cmd_dispatcher(struct ps3_instance *instance,
			     unsigned short cmd_frame_id);

int ps3_async_cmd_send(struct ps3_instance *instance, struct ps3_cmd *cmd);

#ifndef _WINDOWS
int ps3_scsi_cmd_send(struct ps3_instance *instance, struct ps3_cmd *cmd,
		      unsigned char need_prk_err);
#endif

struct ps3_cmd *ps3_cmd_find(struct ps3_instance *instance,
			     unsigned short cmd_frame_id);

int ps3_cmd_dispatch(struct ps3_instance *instance, unsigned short cmd_frame_id,
		     struct PS3ReplyWord *reply_word);

struct ps3_cmd *ps3_r1x_peer_cmd_alloc(struct ps3_instance *instance,
				       unsigned int index);

unsigned char ps3_r1x_peer_cmd_free_nolock(struct ps3_cmd *cmd);

static inline unsigned short ps3_cmd_frame_id(struct ps3_cmd *cmd)
{
	return cmd->index;
}

static inline unsigned long long ps3_cmd_trace_id(struct ps3_cmd *cmd)
{
	return cmd->trace_id;
}

static inline void ps3_cmd_trace_id_replace(struct ps3_cmd *cmd,
					    unsigned long long trace_id)
{
	cmd->trace_id = trace_id;
}

unsigned char
ps3_is_instance_state_allow_cmd_execute(struct ps3_instance *instance);

int ps3_cmd_send_pre_check(struct ps3_instance *instance);

void ps3_wait_scsi_cmd_done(struct ps3_instance *instance,
			    unsigned char time_out);

void ps3_scsi_cmd_deliver_get(struct ps3_instance *instance);

void ps3_scsi_cmd_deliver_put(struct ps3_instance *instance);

void ps3_dma_addr_bit_pos_update(struct ps3_instance *instance,
				 unsigned char bit_pos);

unsigned char ps3_bit_pos_update(struct ps3_instance *instance);

void ps3_wait_mgr_cmd_done(struct ps3_instance *instance,
			   unsigned char time_out);

int ps3_mgr_cmd_send_pre_check(struct ps3_instance *instance,
			       unsigned char no_check);

int ps3_mgr_cmd_send_check(struct ps3_instance *instance, struct ps3_cmd *cmd);

int ps3_ext_sge_cnt_calc_hba(struct ps3_instance *instance);

int ps3_ext_sge_cnt_calc_raid(struct ps3_instance *instance);

void ps3_max_sge_cfg_hba(struct ps3_instance *instance);

#endif
