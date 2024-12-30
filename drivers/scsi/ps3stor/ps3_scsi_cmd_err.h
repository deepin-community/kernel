
#ifndef _PS3_SCSI_CMD_ERR_H_
#define _PS3_SCSI_CMD_ERR_H_

#ifndef _WINDOWS
#include <linux/blkdev.h>
#else
#include "ps3_cmd_adp.h"
#endif

#include "ps3_htp_def.h"
#include "ps3_instance_manager.h"

#define PS3_SCSI_HOST_SHIFT	(16)
#define PS3_SCSI_DRIVER_SHIFT	(24)

#define PS3_SAS_DATA_PRES_SHIFT			(8)     
#define PS3_SAS_DATA_PRES_MASK			(0x3)	
#define PS3_SAS_SENSE_LEN_OFFSET		(16)	
#define PS3_SAS_RESPDATA_LEN_OFFSET		(20)	
#define PS3_SAS_SENSE_RESPDATA_LEN_BYTE		(4)	
#define PS3_SAS_SCSI_STATUS_OFFSET		(11)	
#define PS3_SAS_RESP_CODE_BYTE			(3)	

#define PS3_SCSI_STATUS_MASK			(0xFF)	

#define PS3_SCSI_RESULT_HOST_STATUS(STATUS) ((STATUS) << PS3_SCSI_HOST_SHIFT)
#define PS3_SCSI_RESULT_DRIVER_STATUS(STATUS) ((STATUS) << PS3_SCSI_DRIVER_SHIFT)

#define PS3_SENSE_RESP_CODE_VALID_MASK          (0x70) 
#define PS3_SENSE_RESP_CODE_DESC_FORMAT         (0x72) 
#define PS3_SENSE_RESP_CODE_MASK                (0x7F) 
#define PS3_SENSE_KEY_MASK                      (0xF)  
#define PS3_ABORT_WAIT_CMD_FLIGHT_END           50    

enum ps3_sas_resp_code {
	PS3_SAS_RESP_CODE_TASK_MGR_COMPLETE	= 0x0,
	PS3_SAS_RESP_CODE_INVALID_FRAME		= 0x2,
	PS3_SAS_RESP_CODE_TASK_MGR_NOT_SUPPORT	= 0x4,
	PS3_SAS_RESP_CODE_TASK_MGR_FAILED	= 0x5,
	PS3_SAS_RESP_CODE_TASK_MGR_SUCCESS	= 0x8,
	PS3_SAS_RESP_CODE_INCORRECT_LUN		= 0x9,
	PS3_SAS_RESP_CODE_OVERLAPPED_INIT_PORT	= 0xa,
};

enum ps3_sas_data_pres {
	PS3_SAS_PRES_NO_DATA	  = 0x0,
	PS3_SAS_PRES_REPSNSE_DATA = 0x1,
	PS3_SAS_PRES_SENSE_DATA   = 0x2,
	PS3_SAS_PRES_RESERVED     = 0x3,
};

typedef struct ps3_scsi_sense_hdr {
	U8 resp_code; 
	U8 sense_key;
	U8 asc;
	U8 ascq;
	U8 byte4;
	U8 byte5;
	U8 byte6;
	U8 additional_len; 
} ps3_scsi_sense_hdr_s;

S32 ps3_err_scsi_cmd_fault_proc(struct ps3_instance *instance,
	struct ps3_cmd *cmd);
S32 ps3_err_scsi_task_mgr_reset(struct scsi_cmnd *scmd);
S32 ps3_err_scsi_task_mgr_abort(struct scsi_cmnd *scmd);
S32 ps3_device_reset_handler(struct scsi_cmnd *scmd);
S32 ps3_err_reset_target(struct scsi_cmnd *scmd);
S32 ps3_err_reset_host(struct scsi_cmnd *scmd);
#if ((defined(RHEL_MAJOR) && (RHEL_MAJOR == 9) && (RHEL_MINOR >= 3)) || \
     (LINUX_VERSION_CODE >= KERNEL_VERSION(6,2,0)))
enum scsi_timeout_action ps3_err_reset_timer(struct scsi_cmnd *scmd);
#else
enum blk_eh_timer_return ps3_err_reset_timer(struct scsi_cmnd *scmd);
#endif
Bool ps3_err_is_resp_from_direct_cmd(U16 mode);

void ps3_errcode_to_scsi_status(struct ps3_instance *instance,
	struct scsi_cmnd *s_cmd, U32 err_code,
	PS3RespFrame_u *resp_frame, U32 xfer_cnt, struct ps3_cmd *cmd);

S32 ps3_err_scsi_io_processing(struct ps3_instance *instance,
	U32 id, U32 channel);

S32 ps3_err_scsi_task_pre_check(struct ps3_instance *instance);

S32 ps3_reset_host(struct ps3_instance *instance);

void ps3_scsih_drv_io_reply_scsi(struct scsi_cmnd *s_cmd, struct ps3_cmd *cmd, U8 resp_status, Bool cmd_lock);
void ps3_check_and_wait_host_reset(struct ps3_instance *instance);	

#endif
