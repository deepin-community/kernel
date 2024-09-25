
#ifndef _PS3_CMD_STAT_DEF_H_
#define _PS3_CMD_STAT_DEF_H_

#ifndef _WINDOWS
#include <linux/workqueue.h>
#include <linux/mutex.h>
#else
#include "ps3_worker.h"
#endif
#include "ps3_htp_def.h"

#ifdef __cplusplus
extern "C"{
#endif

struct ps3_instance;

enum {
	PS3_STAT_LOG_COUNT = 0,
	PS3_STAT_LOG_MAX_COUNT = 10,
	PS3_STAT_START = 0,
	PS3_STAT_BACK = 1,
	PS3_STAT_WROKQ_NAME_MAX_LEN = 16,
	PS3_STAT_WORKQ_INTERVAL_DEFAULT = 5000,
};

enum ps3_cmd_stat_item {
	PS3_SCSI_DRV_READ,
	PS3_SCSI_DRV_WRITE, 
	PS3_SCSI_DRV_NORW,  
	PS3_SCSI_DRV_ALL,   
	PS3_DRV_IOC_READ,
	PS3_DRV_IOC_WRITE,
	PS3_DRV_IOC_NORW,
	PS3_DRV_IOC_ALL,
	PS3_DRV_IOC_VD_READ,
	PS3_DRV_IOC_VD_WRITE,
	PS3_DRV_IOC_VD_NORW,
	PS3_DRV_IOC_PD_READ,
	PS3_DRV_IOC_PD_WRITE,
	PS3_DRV_IOC_PD_NORW,
	PS3_DRV_IOC_VD_D_READ,
	PS3_DRV_IOC_VD_D_WRITE,
	PS3_DRV_IOC_PD_D_READ,
	PS3_DRV_IOC_PD_D_WRITE,
	PS3_DRV_IOC_MGR,
	PS3_SCSI_ABORT,
	PS3_SCSI_DEVICE_RESET, 
	PS3_SCSI_RETRY_CMD, 
    PS3_QOS_PD_PRO,         
    PS3_QOS_VD_PRO,         
    PS3_QOS_TAG_PRO,        
	PS3_QOS_MGR_PRO,        
	PS3_QOS_CMD_PRO,        
	PS3_QOS_PD_QUEUE,       
    PS3_QOS_VD_QUEUE,       
    PS3_QOS_TAG_QUEUE,      
	PS3_QOS_MGR_QUEUE,      
	PS3_QOS_CMD_QUEUE,      
	PS3_CMD_STAT_COUNT,
};

static inline const S8 *ps3_cmd_stat_item_tostring(enum ps3_cmd_stat_item type)
{
	static const S8 *itme_string[] = {
		[PS3_SCSI_DRV_READ]        = "scsi_drv_read",
		[PS3_SCSI_DRV_WRITE]       = "scsi_drv_write",
		[PS3_SCSI_DRV_NORW]        = "scsi_drv_norw",
		[PS3_SCSI_DRV_ALL]         = "scsi_drv_all",
		[PS3_DRV_IOC_READ]         = "scsi_drv_ioc_read",
		[PS3_DRV_IOC_WRITE]        = "scsi_drv_ioc_write",
		[PS3_DRV_IOC_NORW]         = "scsi_drv_ioc_norw",
		[PS3_DRV_IOC_ALL]          = "scsi_drv_ioc_all",
		[PS3_DRV_IOC_VD_READ]      = "scsi_drv_ioc_vd_read",
		[PS3_DRV_IOC_VD_WRITE]     = "scsi_drv_ioc_vd_write",
		[PS3_DRV_IOC_VD_NORW]      = "scsi_drv_ioc_vd_norw",
		[PS3_DRV_IOC_PD_READ]      = "scsi_drv_ioc_pd_read",
		[PS3_DRV_IOC_PD_WRITE]     = "scsi_drv_ioc_pd_write",
		[PS3_DRV_IOC_PD_NORW]      = "scsi_drv_ioc_pd_norw",
		[PS3_DRV_IOC_VD_D_READ]    = "scsi_drv_ioc_vd_d_read",
		[PS3_DRV_IOC_VD_D_WRITE]   = "scsi_drv_ioc_vd_d_write",
		[PS3_DRV_IOC_PD_D_READ]    = "scsi_drv_ioc_pd_d_read",
		[PS3_DRV_IOC_PD_D_WRITE]   = "scsi_drv_ioc_pd_d_write",
		[PS3_DRV_IOC_MGR]          = "mgr_drv_ioc_cmd",
		[PS3_SCSI_ABORT]           = "task_abort",
		[PS3_SCSI_DEVICE_RESET]    = "task_reset",
		[PS3_SCSI_RETRY_CMD]       = "scsi_retry_cmd",
        [PS3_QOS_PD_PRO]           = "qos_pd_pro",
        [PS3_QOS_VD_PRO]           = "qos_vd_pro",
        [PS3_QOS_TAG_PRO]          = "qos_tag_pro",
		[PS3_QOS_MGR_PRO]          = "qos_mgr_pro",
		[PS3_QOS_CMD_PRO]          = "qos_cmd_pro",
		[PS3_QOS_PD_QUEUE]		   = "qos_pd_queue",
    	[PS3_QOS_VD_QUEUE]         = "qos_vd_queue",
    	[PS3_QOS_TAG_QUEUE]		   = "qos_tag_queue",
		[PS3_QOS_MGR_QUEUE]        = "qos_mgr_queue",
		[PS3_QOS_CMD_QUEUE]        = "qos_cmd_queue",
		[PS3_CMD_STAT_COUNT]       = "type_unknown",
	};

	return (type >= PS3_CMD_STAT_COUNT) ? "type_unknown" : itme_string[type];
}

struct ps3_lagency_info {
	U64 start_time; 
	U64 back_time;   
	U64 avg; 
	U64 max_lagency; 
	U64 min_lagency; 
	U64 all; 
};

struct ps3_cmd_stat_entry {
	U64 start; 
	U64 back_good; 
	U64 back_err;  
	U64 not_back;  
	struct ps3_lagency_info lagency;
};

struct ps3_total_cmd_stat {
	struct ps3_cmd_stat_entry stat[PS3_CMD_STAT_COUNT];
};

enum ps3_cmd_stat_back_flag {
	PS3_STAT_BACK_OK,        
	PS3_STAT_BACK_FAIL,      
	PS3_STAT_BACK_NO = 0xFF,        
};

struct ps3_single_cmd_stat {
	struct ps3_cmd_stat_entry stat[PS3_CMD_STAT_COUNT];
};

enum ps3_cmd_stat_switch_flag {
	PS3_STAT_ALL_SWITCH_CLOSE     = 0x00,
	PS3_STAT_OUTSTAND_SWITCH_OPEN = 0x01, 
	PS3_STAT_INC_SWITCH_OPEN      = 0x02,
	PS3_STAT_LOG_SWITCH_OPEN      = 0x04,
	PS3_STAT_DEV_SWITCH_OPEN      = 0x08,
    PS3_STAT_QOS_SWITCH_OPEN      = 0x10,
	PS3_STAT_ALL_SWITCH_OPEN      = (PS3_STAT_OUTSTAND_SWITCH_OPEN |
					PS3_STAT_INC_SWITCH_OPEN |
					PS3_STAT_LOG_SWITCH_OPEN |
					PS3_STAT_DEV_SWITCH_OPEN |
                    PS3_STAT_QOS_SWITCH_OPEN),
};

struct ps3_cmd_stat_wrokq_context {
#ifndef _WINDOWS
	struct delayed_work stat_work;        
	struct workqueue_struct *stat_queue;  
	struct ps3_instance *instance;        
#else
	struct ps3_delay_worker statis_work;
#endif
	Bool is_stop; 
};

struct ps3_cmd_statistics_context {
	ps3_atomic32 cmd_outstanding; 
	ps3_atomic32 io_outstanding; 
	ps3_atomic32 vd_io_outstanding; 
	ps3_atomic32 cmd_delivering; 
	ps3_atomic32 scsi_cmd_delivering; 
	ps3_atomic64 cmd_word_send_count; 
	struct ps3_total_cmd_stat total_stat; 
	struct ps3_total_cmd_stat inc_stat;   
	struct ps3_single_cmd_stat **cmd_stat_backup_buf;   
	struct ps3_single_cmd_stat **last_stat_buf;   
	struct ps3_single_cmd_stat **cmd_stat_buf; 
	U32 stat_entry_max_count; 
	U32 stat_interval; 
	struct ps3_cmd_stat_wrokq_context stat_workq; 
	U8 cmd_stat_switch; 
	U8 log_record_count; 
	ps3_atomic32 cli_cnt;
    ps3_atomic32 cmd_qos_processing; 
    ps3_atomic64 cmd_qos_total;      
	U8 reserved2[2];
};

#ifdef __cplusplus
}
#endif

#endif