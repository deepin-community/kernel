
#ifndef __SXE_CLI_H__
#define __SXE_CLI_H__

#ifdef SXE_HOST_DRIVER
#include "sxe_drv_type.h"
#endif

#define SXE_VERION_LEN                  (32)
#define SXE_MAC_NUM                     (128)
#define SXE_PORT_TRANSCEIVER_LEN        (32)
#define SXE_PORT_VENDOR_LEN             (32)
#define SXE_CHIP_TYPE_LEN               (32)
#define SXE_VPD_SN_LEN                  (16)
#define SXE_SOC_RST_TIME                (0x93A80)  
#define SXE_SFP_TEMP_THRESHOLD_INTERVAL (3)        
#define MGC_TERMLOG_INFO_MAX_LEN        (12 * 1024)
#define SXE_REGS_DUMP_MAX_LEN           (12 * 1024)
#define SXE_PRODUCT_NAME_LEN        (32)       

typedef enum sxe_led_mode {
    SXE_IDENTIFY_LED_BLINK_ON   = 0,    
    SXE_IDENTIFY_LED_BLINK_OFF,         
    SXE_IDENTIFY_LED_ON,                
    SXE_IDENTIFY_LED_OFF,               
    SXE_IDENTIFY_LED_RESET,             
} sxe_led_mode_s;

typedef struct sxe_led_ctrl {
    U32    mode;      
    U32    duration;  

} sxe_led_ctrl_s;

typedef struct sxe_led_ctrl_resp {
    U32    ack;       
} sxe_led_ctrl_resp_s;

typedef enum PortLinkSpeed {
    PORT_LINK_NO            = 0,     
    PORT_LINK_100M          = 1,     
    PORT_LINK_1G            = 2,     
    PORT_LINK_10G           = 3,     
} PortLinkSpeed_e;

typedef struct SysSocInfo {
    S8     fwVer[SXE_VERION_LEN];        
    S8     optVer[SXE_VERION_LEN];       
    U8     socStatus;                    
    U8     pad[3];
    S32    socTemp;                      
    U64    chipId;                       
    S8     chipType[SXE_CHIP_TYPE_LEN];  
    S8     pba[SXE_VPD_SN_LEN];          
    S8     productName[SXE_PRODUCT_NAME_LEN];   
} SysSocInfo_s;

typedef struct SysPortInfo {
    U64    mac[SXE_MAC_NUM];         
    U8     isPortAbs;                
    U8     linkStat;                 
    U8     linkSpeed;                


    U8     isSfp:1;                                     
    U8     isGetInfo:1;                                 
    U8     rvd:6;                                       
    S8     opticalModTemp;                              
    U8     pad[3];
    S8     transceiverType[SXE_PORT_TRANSCEIVER_LEN];   
    S8     vendorName[SXE_PORT_VENDOR_LEN];             
    S8     vendorPn[SXE_PORT_VENDOR_LEN];               
} SysPortInfo_s;

typedef struct SysInfoResp {
    SysSocInfo_s     socInfo;        
    SysPortInfo_s    portInfo;       
} SysInfoResp_s;

typedef enum SfpTempTdMode {
    SFP_TEMP_THRESHOLD_MODE_ALARM   = 0,
    SFP_TEMP_THRESHOLD_MODE_WARN,
} SfpTempTdMode_e;

typedef struct SfpTempTdSet{
    U8     mode;             
    U8     pad[3];
    S8     hthreshold;       
    S8     lthreshold;       
} SfpTempTdSet_s;

typedef struct SxeLogExportResp {
    U16    curLogLen;       
    U8     isEnd;
    U8     pad;
    S32    sessionId;       
    S8     data[0];
} SxeLogExportResp_s;

typedef enum SxeLogExportType  {
    SXE_LOG_EXPORT_REQ    = 0,     
    SXE_LOG_EXPORT_FIN,            
    SXE_LOG_EXPORT_ABORT,          
} SxeLogExportType_e;

typedef struct SxeLogExportReq {
    U8     isALLlog;       
    U8     cmdtype;        
    U8     isBegin;        
    U8     pad;
    S32    sessionId;      
    U32    logLen;         
} SxeLogExportReq_s;

typedef struct SocRstReq {
    U32    time;        
} SocRstReq_s;

typedef struct RegsDumpResp {
    U32    curdwLen;    
    U8     data[0];
} RegsDumpResp_s;

enum {
    SXE_MFG_PART_NUMBER_LEN   = 8,
    SXE_MFG_SERIAL_NUMBER_LEN = 16,
    SXE_MFG_REVISION_LEN      = 4,
    SXE_MFG_OEM_STR_LEN       = 64,
    SXE_MFG_SXE_BOARD_ASSEMBLY_LEN  = 32,
    SXE_MFG_SXE_BOARD_TRACE_NUM_LEN = 16,
    SXE_MFG_SXE_MAC_ADDR_CNT        = 2,
};

typedef struct sxeMfgInfo {
    U8 partNumber[SXE_MFG_PART_NUMBER_LEN];      
    U8 serialNumber [SXE_MFG_SERIAL_NUMBER_LEN]; 
    U32 mfgDate;                               
    U8 revision[SXE_MFG_REVISION_LEN];         
    U32 reworkDate;                            
    U8 pad[4];
    U64 macAddr[SXE_MFG_SXE_MAC_ADDR_CNT];             
    U8 boardTraceNum[SXE_MFG_SXE_BOARD_TRACE_NUM_LEN]; 
    U8 boardAssembly[SXE_MFG_SXE_BOARD_ASSEMBLY_LEN];  
    U8 extra1[SXE_MFG_OEM_STR_LEN];                    
    U8 extra2[SXE_MFG_OEM_STR_LEN];                    
} sxeMfgInfo_t;

typedef struct RegsDumpReq {
    U32    baseAddr;    
    U32    dwLen;       
} RegsDumpReq_s;

typedef enum sxe_pcs_mode {
    SXE_PCS_MODE_1000BASE_KX_WO = 0, 
    SXE_PCS_MODE_1000BASE_KX_W,      
    SXE_PCS_MODE_SGMII,              
    SXE_PCS_MODE_10GBASE_KR_WO,      
    SXE_PCS_MODE_AUTO_NEGT_73,       
    SXE_PCS_MODE_LPBK_PHY_TX2RX,     
    SXE_PCS_MODE_LPBK_PHY_RX2TX,     
    SXE_PCS_MODE_LPBK_PCS_RX2TX,     
    SXE_PCS_MODE_BUTT,               
} sxe_pcs_mode_e;

typedef enum sxe_remote_fault_mode {
	SXE_REMOTE_FALUT_NO_ERROR		= 0,
	SXE_REMOTE_FALUT_OFFLINE,
	SXE_REMOTE_FALUT_LINK_FAILURE,
	SXE_REMOTE_FALUT_AUTO_NEGOTIATION,
	SXE_REMOTE_UNKNOWN,
} sxe_remote_fault_e;

typedef struct sxe_phy_cfg {
    sxe_pcs_mode_e mode;          
    U32 mtu;
} sxe_pcs_cfg_s;

typedef enum sxe_an_speed {
    SXE_AN_SPEED_NO_LINK = 0,
    SXE_AN_SPEED_100M,
    SXE_AN_SPEED_1G,      
    SXE_AN_SPEED_10G,     
    SXE_AN_SPEED_UNKNOWN,
} sxe_an_speed_e;

typedef enum sxe_phy_pause_cap {
    SXE_PAUSE_CAP_NO_PAUSE    = 0,   
    SXE_PAUSE_CAP_ASYMMETRIC_PAUSE,  
    SXE_PAUSE_CAP_SYMMETRIC_PAUSE,   
    SXE_PAUSE_CAP_BOTH_PAUSE,        
    SXE_PAUSE_CAP_UNKNOWN,
} sxe_phy_pause_cap_e;

typedef enum sxe_phy_duplex_type {
    SXE_FULL_DUPLEX	= 0,	  
    SXE_HALF_DUPLEX	= 1,	  
    SXE_UNKNOWN_DUPLEX,
} sxe_phy_duplex_type_e;

typedef struct sxe_phy_an_cap {
    sxe_remote_fault_e   remote_fault; 
    sxe_phy_pause_cap_e  pause_cap;    
    sxe_phy_duplex_type_e duplex_cap;  
} sxe_phy_an_cap_s;

typedef struct sxe_an_cap {
    sxe_phy_an_cap_s local;     
    sxe_phy_an_cap_s peer;      
} sxe_an_cap_s;
#endif
