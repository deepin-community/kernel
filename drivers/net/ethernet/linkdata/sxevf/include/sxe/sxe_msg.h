
#ifndef __SXE_MSG_H__
#define __SXE_MSG_H__

#ifdef SXE_HOST_DRIVER
#include "sxe_drv_type.h"
#endif

#define SXE_MAC_ADDR_LEN 6

#define SXE_HDC_CMD_HDR_SIZE  sizeof(struct sxe_hdc_cmd_hdr)
#define SXE_HDC_MSG_HDR_SIZE  sizeof(struct sxe_hdc_drv_cmd_msg)

enum sxe_cmd_type {
    SXE_CMD_TYPE_CLI,
    SXE_CMD_TYPE_DRV,
    SXE_CMD_TYPE_UNKOWN,
};

typedef struct sxe_hdc_cmd_hdr {
    U8 cmd_type;       
    U8 cmd_sub_type;   
    U8 reserve[6];
}sxe_hdc_cmd_hdr_s;



typedef enum SxeFWState {
    SXE_FW_START_STATE_UNDEFINED    = 0x00,   
    SXE_FW_START_STATE_INIT_BASE    = 0x10,   
    SXE_FW_START_STATE_SCAN_DEVICE  = 0x20,   
    SXE_FW_START_STATE_FINISHED     = 0x30,   
    SXE_FW_START_STATE_UPGRADE      = 0x31,   
    SXE_FW_RUNNING_STATE_ABNOMAL    = 0x40,   
    SXE_FW_START_STATE_MASK         = 0xF0,
}SxeFWState_e;

typedef struct SxeFWStateInfo {
    U8 socStatus;          
    char statBuff[32];       
} SxeFWStateInfo_s;


typedef enum MsiEvt {
    MSI_EVT_SOC_STATUS          = 0x1,
    MSI_EVT_HDC_FWOV            = 0x2,
    MSI_EVT_HDC_TIME_SYNC       = 0x4,

    MSI_EVT_MAX                 = 0x80000000,
} MsiEvt_u;


typedef enum SxeFwHdcState {
    SXE_FW_HDC_TRANSACTION_IDLE = 0x01,
    SXE_FW_HDC_TRANSACTION_BUSY,

    SXE_FW_HDC_TRANSACTION_ERR,
} SxeFwHdcState_e;

enum sxe_hdc_cmd_opcode {
    SXE_CMD_SET_WOL         = 1,
    SXE_CMD_LED_CTRL,
    SXE_CMD_SFP_READ,
    SXE_CMD_SFP_WRITE,
    SXE_CMD_TX_DIS_CTRL     = 5,
    SXE_CMD_TINE_SYNC,
    SXE_CMD_RATE_SELECT,
    SXE_CMD_R0_MAC_GET,
    SXE_CMD_LOG_EXPORT,
    SXE_CMD_FW_VER_GET  = 10,
    SXE_CMD_PCS_SDS_INIT,         
    SXE_CMD_AN_SPEED_GET,         
    SXE_CMD_AN_CAP_GET,           
    SXE_CMD_GET_SOC_INFO,         
    SXE_CMD_MNG_RST = 15,         

    SXE_CMD_MAX,
};

enum sxe_hdc_cmd_errcode {
    SXE_ERR_INVALID_PARAM = 1,
};

typedef struct sxe_hdc_drv_cmd_msg {

    U16 opcode;
    U16 errcode;
    union dataLength {
        U16 req_len;
        U16 ack_len;
    } length;
    U8 reserve[8];
    U64 traceid;
    U8 body[0];
} sxe_hdc_drv_cmd_msg_s;


typedef struct sxe_sfp_rw_req {
    U16 offset;       
    U16 len;          
    U8  write_data[0];
} sxe_sfp_rw_req_s;


typedef struct sxe_sfp_read_resp {
    U16 len;     
    U8  resp[0]; 
} sxe_sfp_read_resp_s;

typedef enum sxe_sfp_rate{
    SXE_SFP_RATE_1G     = 0,
    SXE_SFP_RATE_10G    = 1,
} sxe_sfp_rate_e;


typedef struct sxe_sfp_rate_able {
    sxe_sfp_rate_e rate;       
} sxe_sfp_rate_able_s;


typedef struct sxe_spp_tx_able {
    BOOL isDisable;       
} sxe_spp_tx_able_s;


typedef struct sxe_default_mac_addr_resp {
    U8  addr[SXE_MAC_ADDR_LEN]; 
} sxe_default_mac_addr_resp_s;


typedef struct sxe_mng_rst {
    BOOL enable;       
} sxe_mng_rst_s;

#endif 

