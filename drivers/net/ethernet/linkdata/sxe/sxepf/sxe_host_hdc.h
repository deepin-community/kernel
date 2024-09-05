#ifndef __SXE_HOST_HDC_H__
#define __SXE_HOST_HDC_H__

#include "sxe_hdc.h"
#include "sxe_hw.h"
#include "sxe.h"

#define SXE_SUCCESS      (0)
#define SXE_FAILED       (512)

#define SXE_HDC_SUCCESS              0
#define SXE_HDC_FALSE                SXE_ERR_HDC(1)
#define SXE_HDC_INVAL_PARAM          SXE_ERR_HDC(2)
#define SXE_HDC_BUSY                 SXE_ERR_HDC(3)
#define SXE_HDC_FW_OPS_FAILED        SXE_ERR_HDC(4)
#define SXE_HDC_FW_OV_TIMEOUT        SXE_ERR_HDC(5)
#define SXE_HDC_REQ_ACK_HEAD_ERR     SXE_ERR_HDC(6)
#define SXE_HDC_REQ_ACK_TLEN_ERR     SXE_ERR_HDC(7)
#define SXE_HDC_PKG_SKIP_ERR         SXE_ERR_HDC(8)
#define SXE_HDC_PKG_OTHER_ERR        SXE_ERR_HDC(9)
#define SXE_HDC_RETRY_ERR            SXE_ERR_HDC(10)
#define SXE_FW_STATUS_ERR            SXE_ERR_HDC(11)

#define SXE_HDC_TRYLOCK_MAX	     200

#define SXE_HDC_RELEASELOCK_MAX	     20

#define SXE_HDC_TEST_POLL_LOCK_MAX   10
#define SXE_HDC_WAIT_TIME            200

#define SXE_HDC_BIT_1	             0x1

#define BYTE_PER_DWORD               (4)
#define DWORD_TO_BYTE_SHIFT          (2)

union sxe_trace_info {
	u64 trace_id;
	struct {
		U64 count  : 53; 
		U64 cpu_id : 11; 
	} sxe_trace_id_param;
};

struct sxe_hdc_data_info {
	u8 *data;
	u16 len;
};

struct sxe_hdc_trans_info {
	struct sxe_hdc_data_info in;
	struct sxe_hdc_data_info out;
};

struct sxe_driver_cmd {
	void *req;		
	void *resp;		
	u64  trace_id;		
	bool is_interruptible;	
	u16  opcode;		
	u16  req_len;		
	u16  resp_len;		
};

s32 sxe_driver_cmd_trans(struct sxe_hw *hw, struct sxe_driver_cmd *cmd);

s32 sxe_cli_cmd_trans(struct sxe_hw *hw, struct sxe_driver_cmd *cmd);

void sxe_hdc_channel_init(struct sxe_hdc_context *hdc_ctxt);

struct semaphore *sxe_hdc_sema_get(void);

void sxe_hdc_irq_handler(struct sxe_adapter *adapter);

s32 sxe_host_to_fw_time_sync(struct sxe_adapter *adapter);

void sxe_hdc_channel_destroy(struct sxe_hw *hw);

void sxe_hdc_available_set(s32 value);

void sxe_time_sync_handler(struct work_struct *work);

#endif
