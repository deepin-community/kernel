	
#ifndef _PS3_QOS_H_
#define _PS3_QOS_H_
	
#include "ps3_platform_utils.h"
#include "ps3_htp.h"
#include "ps3_cmd_channel.h"
	
#define QOS_HIGH_PRI_EXCLUSIVE_CMD_COUNT 32
#define QOS_MGR_EXCLUSIVE_CMD_COUNT 64
#define PS3_QOS_DEFAULT_PD_QUOTA 40
#define PS3_QOS_SAS_PD_QUOTA 125
#define	PS3_QOS_NVME_MEMBER_QUOTA 127
#define PS3_QOS_NVME_DIRECT_QUOTA 127
#define PS3_QOS_HBA_NVME_NORMAL_QUOTA 126
#define PS3_QOS_RAID_NVME_NORMAL_QUOTA 252
#define PS3_QOS_FUNC1_JBOD_VD_QUOTA 768
#define PS3_QOS_VD_EXCLUSIVE_CMD_COUNT 40
#define PS3_QOS_JBOD_EXCLUSIVE_CMD_COUNT 128
#define PS3_QOS_POLL_INTERVAL   2 
#define PS3_QOS_WAITQ_TIMEOUT   5 
#define PS3_QOS_HBA_MAX_CMD 944
#define PS3_QOS_CS_FUNC0_SHARE_CMD 80
#define PS3_QOS_CS_FUNC0_JBOD_VD_QUOTA 80
#define PS3_QOS_NOTIFY_CMD_COUNT 32 
#define PS3_QOS_FUNC0_PD_WORKQ_COUNT 1
#define PS3_QOS_FUNC1_PD_WORKQ_COUNT 4
#define PS3_QOS_POLL_CMD_COUNT 16

#define PS3_CMD_NEED_QOS(cmd) \
		((cmd)->cmd_word.direct == PS3_CMDWORD_DIRECT_NORMAL || \
		 (cmd)->cmd_word.direct == PS3_CMDWORD_DIRECT_ADVICE)

#define PS3_QOS_INITED(instance) ((instance)->qos_context.inited)
	
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
	ps3_list_head wait_list;       
	U32 count;                     
	U16 id;
	ps3_spinlock *rsc_lock;        
	S32 *free_rsc;		           
	ps3_atomic32 *used_rsc;        
	U32 *total_waited_cnt;		   
	U16 can_resend;                
	U16 has_resend;                
	ULong last_sched_jiffies;      
};

struct ps3_qos_vd_mgr {
	Bool valid;
	U16 id;                                  
	U8 workq_id;							
	ps3_atomic32 vd_quota;                  
	ps3_spinlock rsc_lock;
	struct qos_wait_queue vd_quota_wait_q;  
	ps3_atomic32 exclusive_cmd_cnt;         
	ps3_atomic32 share_cmd_used;            
	struct PS3VDEntry *vd_entry;
	struct ps3_instance *instance;
	struct work_struct resend_work;         
	U64 last_sched_jiffies; 				
};

struct ps3_qos_pd_mgr {
	ps3_atomic32 valid;
	U8 workq_id;						
	U8 dev_type;
	Bool clearing;
	U16 disk_id;                            
	U16 vd_id;                              
	S32 pd_quota;                           
	ps3_atomic32 pd_used_quota;             
	struct ps3_instance *instance;
	struct work_struct resend_work;          
	U64 last_sched_jiffies;   				
	struct qos_wait_queue *waitqs;
	U16 waitq_cnt;
	U32 total_wait_cmd_cnt; 			
	ps3_spinlock rc_lock;				
	U16 poll_que_id;					
	U16 poll_start_que_id;				
	U16 poll_cmd_cnt;
	ps3_spinlock direct_rsc_lock;		
	S32 direct_quota;					
	ps3_atomic32 direct_used_quota;		
	U32 total_waited_direct_cmd;		
	ps3_atomic32 processing_cnt;		   
	ps3_spinlock adjust_quota_lock;     
	S32 adjust_max_quota;               
	S32 adjust_min_quota;               
	S32 pd_init_quota;                  
};

struct ps3_qos_pd_context {
	struct ps3_qos_pd_mgr *qos_pd_mgrs;         
	struct workqueue_struct **work_queues;      
	U16 sas_sata_hdd_quota;
	U16 sas_sata_ssd_quota;
	U16 nvme_normal_quota;
	U16 nvme_direct_quota;
	ps3_atomic32 workq_id_cnt;					
	U8 workq_count;								
};

struct ps3_qos_vd_context {
	struct ps3_qos_vd_mgr *qos_vd_mgrs;         
	struct workqueue_struct **work_queues;      
	U16 jbod_exclusive_cnt;                 	
	U16 vd_exclusive_cnt;                   	
	U8 workq_count;								
	Bool inited;
};

struct ps3_qos_tg_context {
	U32 share;                          	
	U32 mgr_exclusive_cnt;                  
	U16 high_pri_exclusive_cnt;             
	ps3_atomic32 mgr_free_cnt;                 
	ps3_atomic32 mgr_share_used;            
	ps3_atomic32 share_free_cnt;            
	struct qos_wait_queue mgr_cmd_wait_q;   
	struct qos_wait_queue *vd_cmd_waitqs;   
	U32 total_wait_cmd_cnt;                 
	U8 poll_vd_id;                          
	ps3_spinlock lock;                      
	struct ps3_instance *instance;
	struct workqueue_struct *work_queue;	
	struct work_struct resend_work;			 
	U64 last_sched_jiffies; 				
};

struct ps3_qos_ops {
	S32 (*qos_init)(struct ps3_instance *instance);
	void (*qos_exit)(struct ps3_instance *instance);
	void (*qos_vd_init)(struct ps3_instance *instance, struct PS3VDEntry *vd_entry);
	void (*qos_vd_reset)(struct ps3_instance *instance, U16 disk_id);
	Bool (*qos_decision)(struct ps3_cmd *cmd);
	void (*qos_cmd_update)(struct ps3_cmd *cmd);	
	void (*qos_waitq_notify)(struct ps3_instance *instance);
	Bool (*qos_pd_resend_check)(struct ps3_cmd *cmd);
	Bool (*qos_waitq_abort)(struct ps3_cmd *cmd);	
	void (*qos_vd_clean)(struct ps3_instance *instance, struct ps3_scsi_priv_data *pri_data, S32 ret_code);
	void (*qos_pd_clean)(struct ps3_instance *instance, struct ps3_scsi_priv_data *priv_data, S32 ret_code);
	void (*qos_waitq_clear)(struct ps3_instance *instance, S32 ret_code);
	void (*qos_waitq_poll)(struct ps3_instance *instance);
	void  (*qos_reset)(struct ps3_instance *instance);
};

Bool inline ps3_qos_enable(struct ps3_instance *instance);

S32 ps3_hba_qos_init(struct ps3_instance *instance);

void ps3_hba_qos_exit(struct ps3_instance *instance);

void ps3_qos_vd_init(struct ps3_instance *instance, struct PS3VDEntry *vd_entry);

void ps3_qos_vd_reset(struct ps3_instance *instance, U16 disk_id);

struct ps3_qos_pd_mgr* ps3_qos_pd_mgr_init(struct ps3_instance *instance, struct ps3_pd_entry *pd_entry);

void ps3_qos_pd_mgr_reset(struct ps3_instance *instance, U16 pd_id);

void ps3_qos_vd_member_change(struct ps3_instance *instance, struct ps3_pd_entry *pd_entry,
			 struct scsi_device *sdev, Bool is_vd_member);

S32 ps3_qos_decision(struct ps3_cmd *cmd);

void ps3_qos_cmd_update(struct ps3_instance *instance, struct ps3_cmd *cmd);

void ps3_qos_waitq_notify(struct ps3_instance *instance);

Bool ps3_qos_waitq_abort(struct ps3_cmd *aborted_cmd);

void ps3_qos_device_clean(struct ps3_instance *instance, struct ps3_scsi_priv_data *pri_data,
			 S32 ret_code);

void ps3_qos_disk_del(struct ps3_instance *instance, struct ps3_scsi_priv_data *priv_data);

void ps3_qos_vd_member_del(struct ps3_instance *instance, struct PS3DiskDevPos *dev_pos);

void ps3_qos_hard_reset(struct ps3_instance *instance);

void ps3_qos_waitq_clear_all(struct ps3_instance *instance, S32 resp_status);

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
	U16 id;
	struct ps3_instance *instance;
	ps3_spinlock rc_lock;
	ps3_atomic32 free_cnt;
	struct qos_wait_queue *waitqs;
	U16 waitq_cnt;
	U32 total_wait_cmd_cnt;                 
	struct workqueue_struct *work_queue;      
	struct work_struct resend_work;			
	U64 last_sched_jiffies; 				
	U16 poll_cmd_cnt;
	U16 poll_que_id;
};

struct ps3_qos_cq_context {
	struct ps3_qos_softq_mgr mgrq;
	struct ps3_qos_softq_mgr *cmdqs;
	U8 cmdq_cnt;
	U32 mgrq_depth;
	U32 cmdq_depth;
};

struct ps3_qos_context {	
	struct ps3_qos_ops opts;
	struct ps3_qos_pd_context pd_ctx;
	struct ps3_qos_vd_context vd_ctx;
	struct ps3_qos_tg_context tg_ctx;
	struct ps3_qos_cq_context cq_ctx;
	U16 max_vd_count;                           
	U16 max_pd_count;							
	U8 poll_count;                              
	U8 qos_switch;                              
	U8 inited;
};

S32 ps3_raid_qos_init(struct ps3_instance *instance);

void ps3_raid_qos_exit(struct ps3_instance *instance);

S32 ps3_qos_init(struct ps3_instance *instance);

void ps3_qos_exit(struct ps3_instance *instance);

void ps3_qos_adjust_pd_rsc(struct scsi_device *sdev,
	struct ps3_instance *instance, S32 reason);

void ps3_qos_vd_attr_change(struct ps3_instance *instance,
	struct PS3VDEntry *vd_entry_old, struct PS3VDEntry *vd_entry);

#if (LINUX_VERSION_CODE > KERNEL_VERSION(4,9,0) && LINUX_VERSION_CODE < KERNEL_VERSION(4,10,0))
void ps3_linx80_vd_member_change(struct ps3_instance *instance, struct ps3_pd_entry *pd_entry);
#endif

#endif
