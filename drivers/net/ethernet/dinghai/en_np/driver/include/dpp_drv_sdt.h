/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_drv_sdt.h
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
#ifndef DPP_DRV_SDT_H
#define DPP_DRV_SDT_H

#include "zxic_common.h"
#include "dpp_se_api.h"

/*************SDT配置***************************/
/* eram直接表 */
#define ZXDH_SDT_VXLAN_ATTR_TABLE           (ZXIC_UINT32)(0)
#define ZXDH_SDT_SRIOV_VPORT_ATTR_TABLE     (ZXIC_UINT32)(1)
#define ZXDH_SDT_UPLINK_PHY_PORT_ATTR_TABLE (ZXIC_UINT32)(2)
#define ZXDH_SDT_RSS_TO_VQID_TABLE          (ZXIC_UINT32)(3)
#define ZXDH_SDT_VLAN_FILTER_TABLE          (ZXIC_UINT32)(4)
#define ZXDH_SDT_LAG_TABLE                  (ZXIC_UINT32)(5)
#define ZXDH_SDT_BC_TABLE                   (ZXIC_UINT32)(6)
#define ZXDH_SDT_DSCP_TO_UP_TABLE           (ZXIC_UINT32)(7)
#define ZXDH_SDT_UP_TO_TC_TABLE             (ZXIC_UINT32)(8)
#define ZXDH_SDT_UC_PROMISC_TABLE           (ZXIC_UINT32)(10)
#define ZXDH_SDT_MC_PROMISC_TABLE           (ZXIC_UINT32)(11)
#define ZXDH_SDT_FLOW_ID_TABLE              (ZXIC_UINT32)(12)
#define ZXDH_SDT_MAINTAIN_TABLE             (ZXIC_UINT32)(13)
#define ZXDH_SDT_NETWORK_ATTR_TABLE         (ZXIC_UINT32)(14)
#define ZXDH_SDT_VPORT_TRAFFIC_ATTR_TABLE   (ZXIC_UINT32)(15)
#define ZXDH_SDT_VQM_VFID_VLAN_ATTR_TABLE   (ZXIC_UINT32)(16)
#define ZXDH_SDT_CAP_KEYWORD_ATTR_TABLE     (ZXIC_UINT32)(17)
#define ZXDH_SDT_MAINTAIN_DIAG_TABLE        (ZXIC_UINT32)(18)
#define ZXDH_SDT_PSN_ARN_ATTR_TABLE         (ZXIC_UINT32)(19)
#define ZXDH_SDT_STAT_ATTR_TABLE            (ZXIC_UINT32)(20)
#define ZXDH_SDT_TUNNEL_ENCAP0_TABLE        (ZXIC_UINT32)(28)
#define ZXDH_SDT_TUNNEL_ENCAP1_TABLE        (ZXIC_UINT32)(29)
#define ZXDH_SDT_ACL_INDEX_MNG_TABLE        (ZXIC_UINT32)(30)
#define ZXDH_SDT_VHCA_TABLE                 (ZXIC_UINT32)(50)

/* hash表 */
#define ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT0    (ZXIC_UINT32)(64)
#define ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT1    (ZXIC_UINT32)(65)
#define ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT2    (ZXIC_UINT32)(66)
#define ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT3    (ZXIC_UINT32)(67)
#define ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT4    (ZXIC_UINT32)(68)
#define ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT5    (ZXIC_UINT32)(69)

#define ZXDH_SDT_MC_TABLE_PHYPORT0          (ZXIC_UINT32)(76)
#define ZXDH_SDT_MC_TABLE_PHYPORT1          (ZXIC_UINT32)(77)
#define ZXDH_SDT_MC_TABLE_PHYPORT2          (ZXIC_UINT32)(78)
#define ZXDH_SDT_MC_TABLE_PHYPORT3          (ZXIC_UINT32)(79)
#define ZXDH_SDT_MC_TABLE_PHYPORT4          (ZXIC_UINT32)(80)
#define ZXDH_SDT_MC_TABLE_PHYPORT5          (ZXIC_UINT32)(81)
#define ZXDH_SDT_ROCE_MATCH_TABLE           (ZXIC_UINT32)(84)
#define ZXDH_SDT_PMTU_INFO_TABLE            (ZXIC_UINT32)(85)
#define ZXDH_SDT_QP_INFO_TABLE              (ZXIC_UINT32)(86)
#define ZXDH_SDT_LOGIC_GROUP_TABLE          (ZXIC_UINT32)(87)
#define ZXDH_SDT_RDMA_ENTRY_TABLE           (ZXIC_UINT32)(90)

/* etcam表 */
#define ZXDH_SDT_FD_CFG_TABLE               (ZXIC_UINT32)(130)
#define ZXDH_SDT_IPSEC_ENC_TABLE            (ZXIC_UINT32)(131)
#define ZXDH_SDT_CAPTURE_PKT_TABLE          (ZXIC_UINT32)(132)
#define ZXDH_SDT_CSIG_TAG_TABLE             (ZXIC_UINT32)(133)

typedef union zxdh_sdt_tbl_t {
    DPP_SDTTBL_ERAM_T eram;
    DPP_SDTTBL_DDR3_T ddr3;
    DPP_SDTTBL_HASH_T hash;
    DPP_SDTTBL_LPM_T lpm;
    DPP_SDTTBL_ETCAM_T etcam;
    DPP_SDTTBL_PORTTBL_T port;
} ZXDH_SDT_TBL_T;

typedef struct zxdh_sdt_tbl_info_t {
    ZXIC_UINT32 tbl_type;
    ZXIC_UINT32 tbl_depth;
    ZXDH_SDT_TBL_T sdt_tbl_info;
} ZXDH_SDT_TBL_INFO_T;

typedef union zxdh_sdt_tbl_item_data_t{
    struct {
        ZXIC_UINT32 index;
        ZXIC_UINT8 rst[16];
    } eram;
    struct {
        ZXIC_UINT8 key[49];
        ZXIC_UINT8 rst[32];
    } hash;
    struct {
        ZXIC_UINT32 handle;
        ZXIC_UINT8 key_data[80];
        ZXIC_UINT8 key_mask[80];
        ZXIC_UINT8 as_rst[16];
    } etcam;
} ZXDH_SDT_TBL_ITEM_DATA_T;

typedef struct zxdh_sdt_tbl_item_t {
    ZXIC_UINT32 item_num;
    ZXDH_SDT_TBL_ITEM_DATA_T item_data[0];
} ZXDH_SDT_TBL_ITEM_T;

DPP_STATUS dpp_sdt_tbl_type_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 *tbl_type);
DPP_STATUS dpp_sdt_tbl_info_dump(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXDH_SDT_TBL_INFO_T *p_sdt_info);
DPP_STATUS dpp_sdt_tbl_item_dump(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXDH_SDT_TBL_ITEM_DATA_T *data, ZXIC_UINT32 *num);
DPP_STATUS dpp_sdt_tbl_item_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 index, ZXDH_SDT_TBL_ITEM_DATA_T *data);

#endif
