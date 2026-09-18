#ifndef DPP_AGENT_CHANNEL_H
#define DPP_AGENT_CHANNEL_H

#include "zxic_common.h"
#include "dpp_dev.h"
#include "dpp_type_api.h"
#include "dpp_stat_api.h"
#include "dpp_stat_car.h"
#include "dpp_ppu.h"
#include "dpp_agent_se_res.h"

#define BUFFER_LEN_MAX     (256)
#define REG_REPS_LEN       (8)
#define CHANNEL_REPS_LEN   (4)
#define NP_AGENT_ID        (16)
#define MSG_REP_OFFSET     (4)
#define MSG_REP_VALID      (0Xff)
#define MSG_REP_LEN_OFFSET (1)
#define SCHE_RSP_LEN (2)
#define SCHE_REQ_VALID      (0Xffff)
#define PROFILEID_REQ_VALID (0Xffff)
#define EPID_LEVEL         (4)
#define BAR_MSG_RETRY_MAX_TIME    (10)


#pragma pack(1)

typedef enum dpp_tm_level
{
    Q_LEVEL = 0,
    P_LEVEL,
    S_LEVEL,
    MAX_LEVEL
}TM_LEVEL_E;
typedef enum dpp_agent_msg_type
{
    DPP_REG_MSG = 0,
    DPP_DTB_MSG,
    DPP_TM_MSG,
    DPP_PLCR_MSG,
    DPP_PKTRX_IND_REG_RW_MSG,
    DPP_PCIE_BAR_MSG,
    DPP_RESET_MSG,
    DPP_PXE_MSG,
    DPP_TM_FLOW_SHAPE,
    DPP_TM_TD,
    DPP_TM_SE_SHAPE,
    DPP_TM_PP_SHAPE,
    DPP_PLCR_CAR_RATE,
    DPP_PLCR_CAR_PKT_RATE,
    DPP_PPU_THASH_RSK,
    DPP_ACL_MSG,
    DPP_STAT_MSG,
    DPP_RES_MSG,
    DPP_PSN_CFG_MSG,
    DPP_FLOW_MONITOR_MSG,
    DPP_DVS_FREQ_MSG,
    DPP_DHTOOL_QOS_MSG,
    DPP_DHTOOL_QOS_GET_MSG,
    DPP_PRIO_STAT_MSG,
    DPP_QOS_PKTRX_TRUST_SET_MSG,
    DPP_NPPU_TCAM_ENABLE_SET_MSG,
    DPP_COMMON_CFG_MSG,
    DPP_NPPU_TCAM_PFC_SET_MSG,
    DPP_MSG_MAX
} MSG_TYPE_E;

typedef enum dpp_agent_msg_oper
{
    DPP_WR = 0,
    DPP_RD,
    DPP_WR_RD_MAX
} MSG_OPER_E;
typedef enum dpp_agent_msg_res
{
    RES_STD_NIC_MSG = 0,
    RES_OFFLOAD_MSG,
    RES_MAX_MSG
}MSG_RES_TYPE_E;

typedef enum dpp_msg_dtb_oper
{
    QUEUE_REQUEST  = 0,
    QUEUE_RELEASE  = 1,
    QUEUE_SYNC_CFG = 2,
} MSG_DTB_OPER_E;

typedef enum dpp_msg_tm_oper
{
    SEID_REQUEST = 0,
    SEID_RELEASE = 1,
    SEID_QUERY = 2
} MSG_TM_OPER_E;

typedef enum dpp_msg_plcr_oper
{
    PROFILEID_REQUEST = 0,
    PROFILEID_RELEASE = 1,
} MSG_PLCR_OPER_E;
typedef enum dpp_cosq_sche_type
{
    FQ_SCHE = 0,
    FQ2_SCHE = 1,
    FQ4_SCHE = 2,
    FQ8_SCHE = 3,
    SP_SCHE = 4,
    WFQ_SCHE = 5,
    WFQ2_SCHE = 6,
    WFQ4_SCHE = 7,
    WFQ8_SCHE = 8,
    FLOW_SCHE = 9,
    SCHE_TYPE = 10
} DPP_COSQ_SCHE_TYPE;

typedef enum dpp_agent_msg_csflag
{
    DPP_CS_ADDR_FLAG = 0,
    DPP_CS_REGNO_FLAG,
    DPP_CS_FLAG_MAX
} MSG_CSFLAG_E;

typedef enum dpp_ppu_thash_rsk_oper
{
    DPP_PPU_THASH_RSK_RD = 0,
    DPP_PPU_THASH_RSK_WR,
    DPP_PPU_THASH_RSK_MAX
} DPP_PPU_THASH_RSK_OPER_E;

typedef enum dpp_pktrx_ind_reg_rw_oper
{
    DPP_PKTRX_IND_REG_RD = 0,
    DPP_PKTRX_IND_REG_WR,
    DPP_PKTRX_IND_REG_MAX
}DPP_PKTRX_IND_REG_RW_OPER_E;

typedef enum dpp_msg_acl_index_oper
{
    ACL_INDEX_REQUEST    = 0,    /*申请一个index*/
    ACL_INDEX_RELEASE    = 1,    /*释放指定index*/
    ACL_INDEX_VPORT_REL  = 2,    /*释放vport下的所有index*/
    ACL_INDEX_ALL_REL    = 3,    /*释放所有的index*/
    ACL_INDEX_STAT_CLR   = 4,    /*释放指定vport下的index对应的统计项*/
    ACL_INDEX_MAX
}MSG_ACL_INDEX_OPER_E;
typedef enum dpp_se_res_oper
{
    HASH_FUNC_BULK_REQ    = 0, 
    HASH_TBL_REQ          = 1,  
    ERAM_TBL_REQ          = 2,
    ACL_TBL_REQ           = 3,
    LPM_TBL_REQ           = 4,
    DDR_TBL_REQ           = 5,
    STAT_CFG_REQ          = 6,
    RES_REQ_MAX
}MSG_SE_RES_OPER_E;

typedef enum  dpp_agent_pcie_bar
{
   BAR_MSG_NUM_REQ = 0,
   PCIE_BAR_MAX
}MSG_PCIE_BAR_E;

typedef enum  dpp_psn_cfg_oper
{
   PSN_CFG_L2D_WR  = 0,   /*PSN配置到L2D*/
   PSN_CFG_L2D_RD  = 1,   /*从L2D中获取PSN*/  
   PSN_CFG_OPR_MAX
}MSG_PSN_CFG_OPER_E;

typedef enum dpp_tcam_pfc_oper
{
    TCAM_PFC_OPER_ERR = 0,
    TCAM_PFC_OPER_SET = 1,
    TCAM_PFC_OPER_GET = 2,
    TCAM_PFC_OPER_MAX
} MSG_CTAM_PFC_OPER_E;

typedef struct dpp_agent_channel_reg_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 subtype;
    ZXIC_UINT8 oper;
    ZXIC_UINT32 reg_no;
    ZXIC_UINT32 addr;
    ZXIC_UINT32 val_len;
    ZXIC_UINT32 val[32];
} DPP_AGENT_CHANNEL_REG_MSG_T;

typedef struct dpp_agent_channel_dtb_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 oper;
    ZXIC_UINT8 rsv;
    ZXIC_UINT8 name[32];
    ZXIC_UINT32 vport;
    ZXIC_UINT32 queue_id;
} DPP_AGENT_CHANNEL_DTB_MSG_T;
typedef struct dpp_agent_channel_tm_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 oper;
    ZXIC_UINT8  num;
    ZXIC_UINT32 port;
    ZXIC_UINT32 vport;
    ZXIC_UINT32 sche_level;
    ZXIC_UINT32 sche_type;
    ZXIC_UINT32 se_id;
} DPP_AGENT_CHANNEL_TM_MSG_T;
typedef struct dpp_agent_channel_plcr_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 oper;
    ZXIC_UINT8 rsv;
    ZXIC_UINT32 vport;
    ZXIC_UINT32 car_type;
    ZXIC_UINT32 profile_id;
} DPP_AGENT_CHANNEL_PLCR_MSG_T;

typedef struct dpp_agent_tm_flow_shape_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 rsv;
    ZXIC_UINT8 rsv1;
    ZXIC_UINT32 flow_id;
    ZXIC_UINT32 cir;
    ZXIC_UINT32 cbs;
    ZXIC_UINT32 db_en;
    ZXIC_UINT32 eir;
    ZXIC_UINT32 ebs;
}DPP_AGENT_TM_FLOW_SHAPE_MSG_T;

typedef struct dpp_agent_tm_td_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 rsv;
    ZXIC_UINT8 rsv1;
    ZXIC_UINT32 level;
    ZXIC_UINT32 id;
    ZXIC_UINT32 td_th;
}DPP_AGENT_TM_TD_MSG_T;

typedef struct dpp_agent_tm_se_shape_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 rsv;
    ZXIC_UINT8 rsv1;
    ZXIC_UINT32 se_id;
    ZXIC_UINT32 pir;
    ZXIC_UINT32 pbs;
    ZXIC_UINT32 db_en; 
    ZXIC_UINT32 cir; 
    ZXIC_UINT32 cbs;
}DPP_AGENT_TM_SE_SHAPE_MSG_T;

typedef struct dpp_agent_tm_pp_shape_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 rsv;
    ZXIC_UINT8 rsv1;
    ZXIC_UINT32 pp_port; 
    ZXIC_UINT32 cir;
    ZXIC_UINT32 cbs;
    ZXIC_UINT32 c_en;
}DPP_AGENT_TM_PP_SHAPE_MSG_T;

typedef struct dpp_agent_car_pkt_profile_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 rsv;
    ZXIC_UINT8 rsv1;
    ZXIC_UINT32 car_level;
    ZXIC_UINT32 profile_id;
    ZXIC_UINT32 pkt_sign;
    ZXIC_UINT32 cir;
    ZXIC_UINT32 cbs;
    ZXIC_UINT32 pri[DPP_CAR_PRI_MAX];
}DPP_AGENT_CAR_PKT_PROFILE_MSG_T;

typedef struct dpp_agent_car_profile_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 rsv;
    ZXIC_UINT8 rsv1;
    ZXIC_UINT32 car_level;
    ZXIC_UINT32 profile_id;
    ZXIC_UINT32 pkt_sign;
    ZXIC_UINT32 cd;
    ZXIC_UINT32 cf;
    ZXIC_UINT32 cm;
    ZXIC_UINT32 cir;
    ZXIC_UINT32 cbs;
    ZXIC_UINT32 eir;
    ZXIC_UINT32 ebs;
    ZXIC_UINT32 random_disc_e;
    ZXIC_UINT32 random_disc_c;
    ZXIC_UINT32 c_pri[DPP_CAR_PRI_MAX];
    ZXIC_UINT32 e_green_pri[DPP_CAR_PRI_MAX];
    ZXIC_UINT32 e_yellow_pri[DPP_CAR_PRI_MAX];
}DPP_AGENT_CAR_PROFILE_MSG_T;

typedef struct dpp_agent_ppu_thash_rsk_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 oper;
    ZXIC_UINT8 rsv;
    ZXIC_UINT32 rsk_319_288;
    ZXIC_UINT32 rsk_287_256;
    ZXIC_UINT32 rsk_255_224;
    ZXIC_UINT32 rsk_223_192;
    ZXIC_UINT32 rsk_191_160;
    ZXIC_UINT32 rsk_159_128;
    ZXIC_UINT32 rsk_127_096;
    ZXIC_UINT32 rsk_095_064;
    ZXIC_UINT32 rsk_063_032;
    ZXIC_UINT32 rsk_031_000;
}DPP_AGENT_PPU_THASH_RSK_MSG_T;

typedef struct dpp_agent_pktrx_ind_reg_rw_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 oper;
    ZXIC_UINT8 rsv;
    ZXIC_UINT32 mem_addr;
    ZXIC_UINT32 mem_id;
    ZXIC_UINT32 len;
    ZXIC_UINT32 ind_data[8];
}DPP_AGENT_PKTRX_IND_REG_RW_MSG_T;

typedef struct dpp_agent_channel_acl_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 oper;
    ZXIC_UINT8 rsv;
    ZXIC_UINT32 sdt_no;
    ZXIC_UINT32 vport;
    ZXIC_UINT32 index;
    ZXIC_UINT32 counter_id;
    ZXIC_UINT32 rd_mode;
}DPP_AGENT_CHANNEL_ACL_MSG_T;

typedef struct dpp_agent_channel_stat_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 oper;
    ZXIC_UINT8 rsv;
    ZXIC_UINT32 counter_id;
    ZXIC_UINT32 rd_mode;
    ZXIC_UINT32 num;
}DPP_AGENT_CHANNEL_STAT_MSG_T;

typedef struct dpp_agent_se_res_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 sub_type;
    ZXIC_UINT8 oper;
}DPP_AGENT_SE_RES_MSG_T;

typedef struct dpp_agent_channel_pcie_bar_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 oper;
    ZXIC_UINT8 rsv;
}DPP_AGENT_PCIE_BAR_MSG_T;

typedef struct dpp_agent_channel_psn_cfg_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 oper;
    ZXIC_UINT8 psn;
}DPP_AGENT_PSN_CFG_MSG_T;

typedef struct dpp_agent_channel_flow_monitor_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 rsv;
    ZXIC_UINT8 rsv1;
    ZXIC_UINT32 vport;
    ZXIC_UINT32 vport_type;
    ZXIC_UINT32 flag;
}DPP_AGENT_CHANNEL_FLOW_MONITOR_MSG_T;

typedef struct dpp_agent_channel_buffer_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 rsv;
    ZXIC_UINT8 rsv1;
    ZXIC_UINT32 phy_port;
    ZXIC_UINT32 mode;
    ZXIC_UINT32 buffer_size[8];
    ZXIC_UINT32 threshold_size[8];
}DPP_AGENT_CHANNEL_BUFFER_MSG_T;

typedef struct dpp_agent_channel_buffer_get_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 rsv;
    ZXIC_UINT8 rsv1;
    ZXIC_UINT32 phy_port;
}DPP_AGENT_CHANNEL_BUFFER_GET_MSG_T;

typedef struct dpp_agent_channel_stat_info
{
    ZXIC_UINT32 phy_port;
    ZXIC_UINT32 flow_id;
}
DPP_AGENT_CHANNEL_STAT_INFO_T;

typedef struct dpp_agent_channel_np_stat_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 rsv;
    ZXIC_UINT8 rsv1;
    DPP_AGENT_CHANNEL_STAT_INFO_T stat_info;
}DPP_AGENT_CHANNEL_NP_STAT_T;

typedef struct dpp_agent_channel_pktrx_trust_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 rsv;
    ZXIC_UINT8 rsv1;
    ZXIC_UINT32 phy_port;
    ZXIC_UINT32 mode;
}DPP_AGENT_CHANNEL_PKTRX_TRUST_MSG_T;

typedef struct dpp_agent_channel_nppu_tcam_enable_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT16 index;
    ZXIC_UINT32 enable;
}DPP_AGENT_CHANNEL_NPPU_TCAM_ENABLE_MSG_T;

typedef struct dpp_riscv_rcv_channel_tcam_pfc_msg
{
    ZXIC_UINT8 devId;
    ZXIC_UINT8 type;
    ZXIC_UINT8 option;
    ZXIC_UINT8 pfc_map;
}DPP_RISCV_RCV_CHANNEL_TCAM_PFC_MSG_T;

typedef struct dpp_agent_channel_msg
{
    ZXIC_UINT32 msg_len;
    ZXIC_VOID *msg;
} DPP_AGENT_CHANNEL_MSG_T;

#pragma pack()

DPP_STATUS dpp_agent_channel_init(ZXIC_VOID);
DPP_STATUS dpp_agent_channel_exit(ZXIC_VOID);
DPP_STATUS dpp_agent_channel_sync_send(DPP_DEV_T *dev, DPP_AGENT_CHANNEL_MSG_T *pMsg,
                                                            ZXIC_UINT32 *pData, ZXIC_UINT32 rep_len);
DPP_STATUS dpp_agent_channel_reg_sync_send(DPP_DEV_T *dev, DPP_AGENT_CHANNEL_REG_MSG_T *pMsg,
                                                            ZXIC_UINT32 *pData, ZXIC_UINT32 rep_len);
DPP_STATUS dpp_agent_channel_reg_write(DPP_DEV_T *dev, ZXIC_UINT32 reg_type, ZXIC_UINT32 reg_no,
                                        ZXIC_UINT32 reg_width, ZXIC_UINT32 addr, ZXIC_UINT32 *pData);
DPP_STATUS dpp_agent_channel_reg_read(DPP_DEV_T *dev, ZXIC_UINT32 reg_type, ZXIC_UINT32 reg_no,
                                        ZXIC_UINT32 reg_width, ZXIC_UINT32 addr, ZXIC_UINT32 *pData);
DPP_STATUS dpp_agent_channel_dtb_sync_send(DPP_DEV_T *dev, DPP_AGENT_CHANNEL_DTB_MSG_T *pMsg,
                                                            ZXIC_UINT32 *pData, ZXIC_UINT32 rep_len);
DPP_STATUS dpp_agent_channel_dtb_queue_request(DPP_DEV_T *dev, ZXIC_CONST ZXIC_UINT8 *p_name, ZXIC_UINT32 vport_info, ZXIC_UINT32 *p_queue_id);
DPP_STATUS dpp_agent_channel_dtb_queue_release(DPP_DEV_T *dev, ZXIC_CONST ZXIC_UINT8 *p_name, ZXIC_UINT32 queue_id);
DPP_STATUS dpp_agent_channel_dtb_queue_sync_cfg(DPP_DEV_T *dev, ZXIC_CONST ZXIC_UINT8 *p_name, ZXIC_UINT32 vport_info, ZXIC_UINT32 queue_id);

DPP_STATUS dpp_agent_channel_tm_sync_send(DPP_DEV_T *dev, DPP_AGENT_CHANNEL_TM_MSG_T *pMsg, ZXIC_UINT32 *pData, ZXIC_UINT32 rep_len);
DPP_STATUS dpp_agent_channel_tm_seid_request(DPP_DEV_T *dev, ZXIC_UINT32 port, ZXIC_UINT32 vport, ZXIC_UINT32 sche_level, ZXIC_UINT32 sche_type, ZXIC_UINT32 num, ZXIC_UINT32 *p_se_id);
DPP_STATUS dpp_agent_channel_tm_seid_release(DPP_DEV_T *dev, ZXIC_UINT32 port, ZXIC_UINT32 vport, ZXIC_UINT32 sche_level, ZXIC_UINT32 sche_type, ZXIC_UINT32 num, ZXIC_UINT32 se_id);

DPP_STATUS dpp_agent_channel_tm_base_node_get(DPP_DEV_T *dev, ZXIC_UINT32 port, ZXIC_UINT32 vport, ZXIC_UINT32 *p_se_id);

DPP_STATUS dpp_agent_channel_plcr_sync_send(DPP_DEV_T *dev, DPP_AGENT_CHANNEL_PLCR_MSG_T *pMsg, ZXIC_UINT32 *pData, ZXIC_UINT32 rep_len);
DPP_STATUS dpp_agent_channel_plcr_profileid_request(DPP_DEV_T *dev, ZXIC_UINT32 vport, ZXIC_UINT32 car_type, ZXIC_UINT32 *p_profileid);
DPP_STATUS dpp_agent_channel_plcr_profileid_release(DPP_DEV_T *dev, ZXIC_UINT32 vport, ZXIC_UINT32 car_type, ZXIC_UINT32 profileid);

DPP_STATUS dpp_agent_channel_tm_flow_shape(DPP_DEV_T *dev, ZXIC_UINT32 flow_id, ZXIC_UINT32 cir, ZXIC_UINT32 cbs, ZXIC_UINT32 db_en, ZXIC_UINT32 eir, ZXIC_UINT32 ebs);
DPP_STATUS dpp_agent_channel_tm_td_set(DPP_DEV_T *dev, ZXIC_UINT32 level, ZXIC_UINT32 id, ZXIC_UINT32 td_th);
DPP_STATUS dpp_agent_channel_tm_se_shape(DPP_DEV_T *dev, ZXIC_UINT32 se_id, ZXIC_UINT32 pir, ZXIC_UINT32 pbs,ZXIC_UINT32 db_en, ZXIC_UINT32 cir, ZXIC_UINT32 cbs);
DPP_STATUS dpp_agent_channel_tm_port_shape(DPP_DEV_T *dev, ZXIC_UINT32 pp_port, ZXIC_UINT32 cir, ZXIC_UINT32 cbs, ZXIC_UINT32 c_en);

DPP_STATUS dpp_agent_channel_plcr_car_rate(DPP_DEV_T *dev, ZXIC_UINT32 car_type, ZXIC_UINT32 pkt_sign, ZXIC_UINT32 profile_id, ZXIC_VOID* p_car_profile_cfg);
DPP_STATUS dpp_agent_channel_ppu_thash_rsk(DPP_DEV_T *dev, DPP_PPU_THASH_RSK_OPER_E oper, DPP_PPU_PPU_COP_THASH_RSK_T *p_para);

DPP_STATUS dpp_agent_channel_acl_index_request(DPP_DEV_T *dev,
                                               ZXIC_UINT32 sdt_no, 
                                               ZXIC_UINT32 vport,
                                               ZXIC_UINT32 *p_index);
DPP_STATUS dpp_agent_channel_acl_index_release(DPP_DEV_T *dev,
                                               ZXIC_UINT32 rel_type,
                                               ZXIC_UINT32 sdt_no, 
                                               ZXIC_UINT32 vport,
                                               ZXIC_UINT32 index);
DPP_STATUS dpp_agent_channel_acl_stat_clr(DPP_DEV_T *dev,
                                    ZXIC_UINT32 sdt_no,
                                    ZXIC_UINT32 vport,
                                    ZXIC_UINT32 counter_id,
                                    ZXIC_UINT32 rd_mode); 
DPP_STATUS dpp_agent_channel_stat_clr(DPP_DEV_T *dev,
                                    ZXIC_UINT32 count_id,
                                    ZXIC_UINT32 rd_mode,
                                    ZXIC_UINT32 num);
DPP_STATUS dpp_agent_channel_se_res_get(DPP_DEV_T *dev,
                                    ZXIC_UINT32 sub_type,
                                    ZXIC_UINT32 opr,
                                    ZXIC_UINT32 *p_rsp_buff,
                                    ZXIC_UINT32 buff_size);
DPP_STATUS dpp_agent_channel_pcie_bar_request(DPP_DEV_T *dev,ZXIC_UINT32 *p_bar_msg_num);
DPP_STATUS dpp_agent_channel_pktrx_ind_reg_rw(DPP_DEV_T *dev,
                                              ZXIC_UINT32 mem_addr,
                                              ZXIC_UINT32 mem_id,
                                              ZXIC_UINT32 oper,
                                              ZXIC_UINT32 len,
                                              ZXIC_UINT32 *p_data);
DPP_STATUS dpp_agent_channel_psn_cfg_l2d_write(DPP_DEV_T *dev,
                                               ZXIC_UINT8 psn_cfg);
DPP_STATUS dpp_agent_channel_psn_cfg_l2d_read(DPP_DEV_T *dev,
                                            ZXIC_UINT32 * p_psn_cfg);

DPP_STATUS dpp_agent_channel_np_flow_monitor(DPP_DEV_T *dev, ZXIC_UINT32 vport, ZXIC_UINT32 flag); 

DPP_STATUS dpp_agent_channel_np_ingress_buffer_set(DPP_DEV_T *dev,
                                                   ZXIC_UINT32 port,
                                                   ZXIC_UINT32 mode,
                                                   ZXIC_UINT32 *buffer_size,
                                                   ZXIC_UINT32 *threshold_size);

DPP_STATUS dpp_agent_channel_np_ingress_buffer_get(DPP_DEV_T *dev,
                                                   ZXIC_UINT32 port,
                                                   ZXIC_UINT32 *panel_buffer_size,
                                                   ZXIC_UINT32 *panel_threshold_size,
                                                   ZXIC_UINT32 *internal_buffer_size,
                                                   ZXIC_UINT32 *internal_threshold_size);
DPP_STATUS dpp_agent_channel_prio_stat_get(DPP_DEV_T *dev, DPP_AGENT_CHANNEL_STAT_INFO_T *stat_info, ZXIC_VOID* p_stat_data);

DPP_STATUS dpp_agent_channel_np_pktrx_trust_set(DPP_DEV_T *dev, ZXIC_UINT32 port, ZXIC_UINT32 mode, ZXIC_UINT8 id);
DPP_STATUS dpp_agent_channel_msg_nppu_tcam_enable_set(DPP_DEV_T *dev, ZXIC_UINT32 index, ZXIC_UINT32 en);
DPP_STATUS dpp_agent_channel_msg_nppu_tcam_pfc_set(DPP_DEV_T *dev, ZXIC_UINT8 pfc_map);
DPP_STATUS dpp_agent_channel_msg_nppu_tcam_pfc_get(DPP_DEV_T *dev, ZXIC_UINT8 *pfc_map);

#endif