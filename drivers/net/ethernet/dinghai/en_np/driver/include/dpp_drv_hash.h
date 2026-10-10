/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_drv_hash.h
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 :
* 完成日期 : 2014/01/27
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/
#ifndef DPP_DRV_HASH_H
#define DPP_DRV_HASH_H

#include "zxic_common.h"
#include "dpp_apt_se_api.h"


/* hash-function for hash tbl */

/* L2 forward */
typedef struct zxdh_l2_fwd_key
{
    ZXIC_UINT16 sriov_vlan_id            /* : 16; */;
    ZXIC_UINT16 sriov_vlan_tpid          /* : 16; */;
    ZXIC_UINT8  dmac_addr[6]             /* : 48; */;
} ZXDH_L2_FWD_KEY;

typedef struct zxdh_l2_fwd_entry
{
    ZXIC_UINT32 vqm_vfid   /* : 11;*/;
    ZXIC_UINT32 rsv        /* : 20; */;
    ZXIC_UINT32 hit_flag   /* : 1; */;
}ZXDH_L2_FWD_ENTRY;

typedef struct zxdh_l2_fwd_t
{
    ZXDH_L2_FWD_KEY key;
    ZXDH_L2_FWD_ENTRY entry;
} ZXDH_L2_ENTRY_T;

/* multicast */
typedef struct zxdh_mc_key
{
    ZXIC_UINT8  mc_mac[6];
    ZXIC_UINT32 group_id;
    ZXIC_UINT32 rsv;
}ZXDH_MC_KEY;

typedef struct zxdh_mc_entry
{
    ZXIC_UINT64 mc_bitmap;
    ZXIC_UINT32 rsv2;
    ZXIC_UINT32 rsv1;
    ZXIC_UINT32 mc_pf_enable;
    ZXIC_UINT32 hit_flag;
}ZXDH_MC_ENTRY;

typedef struct zxdh_mc_t
{
    ZXDH_MC_KEY key;
    ZXDH_MC_ENTRY entry;
} ZXDH_MC_T;

typedef struct zxdh_rdma_trans_key
{
    ZXIC_UINT8  mac_addr[6];  /**<  @brief key */
    ZXIC_UINT16 rsv            /* : 16; */;
}ZXDH_RDMA_TRANS_KEY;

typedef struct zxdh_rdma_trans_entry
{
    ZXIC_UINT32 rdma_vhca_id   /* : 10;*/;
    ZXIC_UINT32 rsv            /* : 21; */;
    ZXIC_UINT32 hit_flag       /* : 1; */;
}ZXDH_RDMA_TRANS_ENTRY;


typedef struct zxdh_rdma_trans_t
{
    ZXDH_RDMA_TRANS_KEY key;
    ZXDH_RDMA_TRANS_ENTRY entry;
} ZXDH_RDMA_TRANS_T;

ZXIC_UINT32 dpp_apt_set_l2entry_data(ZXIC_VOID *pData, DPP_HASH_ENTRY *pEntry);
ZXIC_UINT32 dpp_apt_get_l2entry_data(ZXIC_VOID *pData, DPP_HASH_ENTRY *pEntry);

ZXIC_UINT32 dpp_apt_set_mc_data(ZXIC_VOID *pData, DPP_HASH_ENTRY *pEntry);
ZXIC_UINT32 dpp_apt_get_mc_data(ZXIC_VOID *pData, DPP_HASH_ENTRY *pEntry);

ZXIC_UINT32 dpp_apt_set_rdma_trans_data(ZXIC_VOID *pData, DPP_HASH_ENTRY *pEntry);
ZXIC_UINT32 dpp_apt_get_rdma_trans_data(ZXIC_VOID *pData, DPP_HASH_ENTRY *pEntry);

typedef struct dpp_hash_init_t
{   
    ZXIC_UINT32 func_num; 
    DPP_APT_HASH_FUNC_RES_T *func;
    ZXIC_UINT32 bulk_num;
    DPP_APT_HASH_BULK_RES_T *bulk;
    ZXIC_UINT32 ser_num;
    DPP_APT_HASH_TABLE_T  *ser;
} DPP_HASH_INIT_T;


DPP_STATUS dpp_apt_dtb_hash_table_unicast_mac_dump(DPP_DEV_T *dev, ZXIC_UINT32 queue_id, ZXIC_UINT32 sdt_no, ZXDH_L2_ENTRY_T *pHashDataArr, ZXIC_UINT32 *p_entry_num);
DPP_STATUS dpp_apt_dtb_hash_table_multicast_mac_dump(DPP_DEV_T *dev, ZXIC_UINT32 queue_id, ZXIC_UINT32 sdt_no, ZXDH_MC_T *pHashDataArr, ZXIC_UINT32 *p_entry_num);
SE_APT_HASH_CONVERT_T *se_hash_callback_get(ZXIC_UINT32 sdt_no);

#endif
