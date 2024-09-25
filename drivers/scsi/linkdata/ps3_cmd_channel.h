
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

#define PS3_DEFAULT_REQ_FRAME_SIZE (256)

#define PS3_WAIT_SCSI_CMD_DONE_COUNT 300

enum {
	DMA_ALIGN_BYTES_4K	= 4096,
	DMA_ALIGN_BYTES_2K	= 2048,
	DMA_ALIGN_BYTES_1K	= 1024,
	DMA_ALIGN_BYTES_512	= 512,
	DMA_ALIGN_BYTES_256	= 256,
	DMA_ALIGN_BYTES_128	= 128,
	DMA_ALIGN_BYTES_64	= 64,
	DMA_ALIGN_BYTES_16	= 16,
	DMA_ALIGN_BYTES_4	= 4,
};

enum {
	PS3_CMD_STATE_INIT	= 0, 
	PS3_CMD_STATE_PROCESS	= 1, 
	PS3_CMD_STATE_COMPLETE	= 2, 
	PS3_CMD_STATE_DEAD	= 3, 
};
enum {
	PS3_CMD_FLAG_SOFTRESET	= 1,
	PS3_CMD_FLAG_HARDRESET	= 2,
};

enum {
	PS3_R1X_MODE_NORMAL		= 0, 
	PS3_R1X_MODE_PERF		= 1, 
};

#define PS3_CMD_EXT_BUF_DEFAULT_SIZE (4096)
#define PS3_CMD_EXT_BUF_SIZE_MGR (4096)

#define PS3_MIN_SCSI_CMD_COUNT (4096)

static inline const S8 *namePS3CmdState(U32 s)
{
	static const S8 *myNames[] = {
		[PS3_CMD_STATE_INIT]		= "PS3_CMD_STATE_INIT",
		[PS3_CMD_STATE_PROCESS]		= "PS3_CMD_STATE_PROCESS",
		[PS3_CMD_STATE_COMPLETE]	= "PS3_CMD_STATE_COMPLETE",
		[PS3_CMD_STATE_DEAD]		= "PS3_CMD_STATE_DEAD"
	};

	if (s > PS3_CMD_STATE_DEAD) {
		return "PS3_CMD_STATE_INVALID";
	}

	return myNames[s];
}

struct ps3_cmd_state_t {
	U8 state;
	U8 reset_flag;
	U8 reserived[6];
	ps3_spinlock lock;
};
typedef union ps3_scsi_cdb_option {
    struct {
        U8 non_ncq : 1; 
        U8 reserved0 : 1;
        U8 reserved1 : 1;
        U8 fua : 1;
        U8 dpo : 1;
        U8 protect : 3;
    };
    U8 option;
} ps3_scsi_cdb_opts_u;

struct ps3_scsi_io_attr
{
	Bool is_retry_cmd;	
	U8  direct_flag; 	
	U8  seq_flag;		
	U8  dev_type;		
	union
	{
		struct{
			U8  rw_flag : 7;		
			U8  is_confilct_check : 1;	
		};
		U8 rw_type;
	};
	U32 num_blocks;		
	U32 lba_lo;		
	U32 lba_hi;		
	const struct ps3_pd_entry *pd_entry;
	const struct ps3_pd_entry *peer_pd_entry;
	const struct PS3VDEntry *vd_entry;

	U64 plba;	
	U64 plba_back;	
	U8  span_idx;	
	U8  span_pd_idx;
	U16 disk_id;	
	Bool is_use_frontend_prp; 
	U8  span_pd_idx_p;
	U8  span_pd_idx_q;
	U8 is_force_normal:1; 
	U8 reserved:7;
	ps3_scsi_cdb_opts_u cdb_opts;	
	U8  cdb[PS3_FRAME_CDB_BUFLEN];	
	U32 sgl_buf_len; 
	U32 reserved1;
};

struct ps3_ioctl_transient {
	U16 sge_num;
	U8 reserved[6];
	void *transient_buff[PS3_MAX_IOCTL_SGE_NUM];
};

#define PS3_QOS_MAX_PD_IN_VD (17)
struct ps3_qos_member_pd_info {
    U16  flat_disk_id;  
    U16  strip_count;   
    Bool get_quota;
};

#define PS3_QOS_MAX_CMDQ_ONE_CMD 2
struct ps3_qos_cmdq_info {
	U8  que_id;  		
    Bool get_rc;
};

struct ps3_cmd {
	union PS3ReqFrame *req_frame;
	U64 req_frame_phys;
	void *ext_buf;
	U64 ext_buf_phys;
	PS3RespFrame_u *resp_frame;
	U64 resp_frame_phys;
	struct ps3_instance *instance;
	union {
		struct PS3CmdWord cmd_word;		
		struct PS3InitCmdWord init_cmd_word;	
		U64 cmd_word_value;			
	};

	struct scsi_cmnd *scmd;	

#ifndef _WINDOWS
	struct list_head cmd_list;
#else
	SCSI_REQUEST_BLOCK *srb;
	ps3_list_head cmd_list;
#endif

	U64 trace_id;		
	U16 index;		
	U8 no_reply_word;
	U8 is_force_polling;
	U8 is_got_r1x;
	U8 is_inserted_c_q;
	U8 is_r1x_aborting;
	Bool is_r1x_scsi_complete;
	U16 r1x_read_pd;
	struct ps3_cmd *r1x_peer_cmd;
	U8 is_aborting;
	U8 r1x_reply_flag;
	U8 qos_processing;
	U32 os_sge_map_count;	
	struct ps3_cmd_state_t cmd_state;
	U16 time_out;
	Bool is_interrupt; 
	U8 szblock_cnt; 
	U32 retry_cnt; 
	void* node_buff; 
#ifdef _WINDOWS
	KEVENT sync_done;
#else
	struct completion sync_done;	
#endif
	S32 (*cmd_send_cb)(struct ps3_instance *, struct ps3_cmd *, U16 );
	S32 (*cmd_receive_cb)(struct ps3_cmd *, U16);
	struct ps3_ioctl_transient *transient;
	struct ps3_scsi_io_attr io_attr;
#ifdef _WINDOWS
	struct scsi_cmnd scmd_imp;	
#endif
	struct PS3ReplyWord reply_word; 

	struct list_head qos_list;
	struct ps3_qos_member_pd_info target_pd[PS3_QOS_MAX_PD_IN_VD];
	struct ps3_qos_cmdq_info cmdq_info[PS3_QOS_MAX_CMDQ_ONE_CMD];
	U16 target_pd_count;
	U16 first_over_quota_pd_idx;
	U8 qos_waitq_flag;
	U8 cmdq_count;
	Bool flighting;  
};

struct  ps3_cmd_context {
	U32 max_cmd_count;
	U32 max_scsi_cmd_count;
	U32 max_mgr_cmd_count;
	U32 max_prp_count; 
	struct ps3_cmd **cmd_buf;

	dma_addr_t  init_frame_buf_phys;
	U8 *init_frame_buf;
	dma_addr_t      init_filter_table_phy_addr;
	U8 *init_filter_table_buff;
#ifndef _WINDOWS
	struct dma_pool* req_frame_dma_pool;
#endif
	U32 req_frame_buf_size;
	dma_addr_t      req_frame_buf_phys;
	U8 *req_frame_buf;
#ifndef _WINDOWS
	struct dma_pool* response_frame_dma_pool;
#endif
	U32 response_frame_buf_size;
	dma_addr_t      response_frame_buf_phys;
	U8 *response_frame_buf;
#ifndef _WINDOWS
	struct dma_pool* ext_buf_dma_pool;
#endif
	U32             ext_buf_size;
	U32             ext_sge_frame_count;
#ifndef _WINDOWS
	struct dma_pool* mgr_ext_buf_dma_pool;
#endif
	U32             mgr_ext_buf_size;

	dma_addr_t      init_frame_sys_info_phys;
	U8 *init_frame_sys_info_buf;
	U8 sgl_mode_support; 
	U8 reserved0[1];
	U16 max_host_sge_count;

#ifndef _WINDOWS
	struct list_head mgr_cmd_pool;
	struct list_head task_cmd_pool;
	struct list_head r1x_scsi_cmd_pool;
	spinlock_t mgr_pool_lock;
	spinlock_t task_pool_lock;
	spinlock_t r1x_scsi_pool_lock;
#else
	ps3_list_head mgr_cmd_pool;
	ps3_list_head task_cmd_pool;
	ps3_list_head scsi_cmd_pool;
	ps3_spinlock mgr_pool_lock;
	ps3_spinlock task_pool_lock;
	ps3_spinlock scsi_pool_lock;

	ps3_atomic64 trace_id;
#endif
	U16 max_r1x_cmd_count;
	U8 reserved1[6];
};

S32 ps3_cmd_context_init(struct ps3_instance *instance);

void ps3_cmd_context_exit(struct ps3_instance *instance);

#ifndef _WINDOWS
struct ps3_cmd *ps3_scsi_cmd_alloc(struct ps3_instance *instance, U32 tag);
#else
struct ps3_cmd *ps3_scsi_cmd_alloc(struct ps3_instance *instance);
#endif
S32 ps3_scsi_cmd_free(struct ps3_cmd *cmd);

struct ps3_cmd *ps3_mgr_cmd_alloc(struct ps3_instance *instance);

S32 ps3_mgr_cmd_free_nolock(struct ps3_instance *instance, struct ps3_cmd *cmd);
S32 ps3_mgr_cmd_free(struct ps3_instance *instance, struct ps3_cmd *cmd);

struct ps3_cmd *ps3_task_cmd_alloc(struct ps3_instance *instance);

S32 ps3_task_cmd_free(struct ps3_instance *instance, struct ps3_cmd *cmd);

S32 ps3_reply_cmd_dispatcher(struct ps3_instance *instance, U16 cmd_frame_id);

S32 ps3_async_cmd_send(struct ps3_instance *instance, struct ps3_cmd *cmd);

#ifndef _WINDOWS
S32 ps3_scsi_cmd_send(struct ps3_instance *instance, struct ps3_cmd *cmd);
#endif

struct ps3_cmd *ps3_cmd_find(struct ps3_instance *instance, U16 cmd_frame_id);

S32 ps3_cmd_dispatch(struct ps3_instance *instance, U16 cmd_frame_id,
	struct PS3ReplyWord *reply_word);

struct ps3_cmd *ps3_r1x_peer_cmd_alloc(struct ps3_instance *instance, U32 index);

Bool ps3_r1x_peer_cmd_free_nolock(struct ps3_cmd *cmd);

static inline U16 ps3_cmd_frame_id(struct ps3_cmd *cmd)
{
	return cmd->index;
}

static inline U64 ps3_cmd_trace_id(struct ps3_cmd *cmd)
{
	return cmd->trace_id;
}

static inline void ps3_cmd_trace_id_replace(struct ps3_cmd *cmd, U64 trace_id)
{
	cmd->trace_id = trace_id;
}

Bool ps3_is_instance_state_allow_cmd_execute(struct ps3_instance *instance);

S32 ps3_cmd_send_pre_check(struct ps3_instance *instance);

void ps3_wait_scsi_cmd_done(struct ps3_instance *instance, Bool time_out);

void ps3_scsi_cmd_deliver_get(struct ps3_instance *instance);

void ps3_scsi_cmd_deliver_put(struct ps3_instance *instance);

void ps3_dma_addr_bit_pos_update(struct ps3_instance *instance, U8 bit_pos);

Bool ps3_bit_pos_update(struct ps3_instance *instance);

#endif
