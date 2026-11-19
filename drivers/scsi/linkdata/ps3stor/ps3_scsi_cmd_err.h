/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_SCSI_CMD_ERR_H_
#define _PS3_SCSI_CMD_ERR_H_

#ifndef _WINDOWS
#include <linux/blkdev.h>
#else
#include "ps3_cmd_adp.h"
#endif

#include "ps3_htp_def.h"
#include "ps3_instance_manager.h"
#include "ps3_kernel_version.h"

#define PS3_SCSI_HOST_SHIFT (16)
#define PS3_SCSI_DRIVER_SHIFT (24)

#define PS3_SAS_DATA_PRES_SHIFT (8)
#define PS3_SAS_DATA_PRES_MASK (0x3)
#define PS3_SAS_SENSE_LEN_OFFSET (16)
#define PS3_SAS_RESPDATA_LEN_OFFSET (20)
#define PS3_SAS_SENSE_RESPDATA_LEN_BYTE (4)
#define PS3_SAS_SCSI_STATUS_OFFSET (11)
#define PS3_SAS_RESP_CODE_BYTE (3)

#define PS3_SCSI_STATUS_MASK (0xFF)

#define PS3_SCSI_RESULT_HOST_STATUS(STATUS) ((STATUS) << PS3_SCSI_HOST_SHIFT)
#define PS3_SCSI_RESULT_DRIVER_STATUS(STATUS)                                  \
	((STATUS) << PS3_SCSI_DRIVER_SHIFT)

#define PS3_SENSE_RESP_CODE_VALID_MASK (0x70)
#define PS3_SENSE_RESP_CODE_DESC_FORMAT (0x72)
#define PS3_SENSE_RESP_CODE_MASK (0x7F)
#define PS3_SENSE_KEY_MASK (0xF)
#define PS3_ABORT_WAIT_CMD_FLIGHT_END 50
#define PS3_NVME_RESP_STATUS_SQ_FULL (0x102)

enum ps3_sas_resp_code {
	PS3_SAS_RESP_CODE_TASK_MGR_COMPLETE = 0x0,
	PS3_SAS_RESP_CODE_INVALID_FRAME = 0x2,
	PS3_SAS_RESP_CODE_TASK_MGR_NOT_SUPPORT = 0x4,
	PS3_SAS_RESP_CODE_TASK_MGR_FAILED = 0x5,
	PS3_SAS_RESP_CODE_TASK_MGR_SUCCESS = 0x8,
	PS3_SAS_RESP_CODE_INCORRECT_LUN = 0x9,
	PS3_SAS_RESP_CODE_OVERLAPPED_INIT_PORT = 0xa,
};

enum ps3_sas_data_pres {
	PS3_SAS_PRES_NO_DATA = 0x0,
	PS3_SAS_PRES_REPSNSE_DATA = 0x1,
	PS3_SAS_PRES_SENSE_DATA = 0x2,
	PS3_SAS_PRES_RESERVED = 0x3,
};

struct ps3_scsi_sense_hdr {
	unsigned char resp_code;
	unsigned char sense_key;
	unsigned char asc;
	unsigned char ascq;
	unsigned char byte4;
	unsigned char byte5;
	unsigned char byte6;
	unsigned char additional_len;
};

int ps3_err_scsi_cmd_fault_proc(struct ps3_instance *instance,
				struct ps3_cmd *cmd);
int ps3_err_scsi_task_mgr_reset(struct scsi_cmnd *scmd);
int ps3_err_scsi_task_mgr_abort(struct scsi_cmnd *scmd);
int ps3_device_reset_handler(struct scsi_cmnd *scmd);
int ps3_err_reset_target(struct scsi_cmnd *scmd);
int ps3_err_reset_host(struct scsi_cmnd *scmd);
#if defined(PS3_RESET_TIMER)
enum scsi_timeout_action ps3_err_reset_timer(struct scsi_cmnd *scmd);
#else
enum blk_eh_timer_return ps3_err_reset_timer(struct scsi_cmnd *scmd);
#endif
unsigned char ps3_err_is_resp_from_direct_cmd(unsigned short mode);

void ps3_errcode_to_scsi_status(struct ps3_instance *instance,
				struct scsi_cmnd *s_cmd, unsigned int err_code,
				union PS3RespFrame *resp_frame,
				unsigned int xfer_cnt, struct ps3_cmd *cmd);

int ps3_err_scsi_io_processing(struct ps3_instance *instance, unsigned int id,
	unsigned int channel);

unsigned char ps3_host_reset_state_check(struct ps3_instance *instance,
	unsigned char is_host_reset);

int ps3_reset_host(struct ps3_instance *instance);

void ps3_scsih_drv_io_reply_scsi(struct scsi_cmnd *s_cmd, struct ps3_cmd *cmd,
				 unsigned char resp_status,
				 unsigned char cmd_lock);
void ps3_check_and_wait_host_reset(struct ps3_instance *instance);

#endif
