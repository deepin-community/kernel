#ifndef _DPP_PKT_CAP_TBL_H_
#define _DPP_PKT_CAP_TBL_H_

#include "zxic_common.h"
#include "dpp_apt_se_api.h"
#include "dpp_drv_acl.h"

#define DH_PKT_CAP_POINT_IN_MF_GLOBAL_OFFSET (18U)
#define DH_PKT_CAP_POINT_IN_MF_GLOBAL_LENGTH (6U)

#define DH_PKT_CAP_POINT_NORMAL_RULE_NUM       (10U)
#define DH_PKT_CAP_POINT_KEY_WORD_RULE_NUM     (2U)

#define DH_PKT_CAP_TCAM_ITEM_NUM  (DH_PKT_CAP_POINT_MAX * DH_PKT_CAP_POINT_NORMAL_RULE_NUM + DH_PKT_CAP_POINT_RDMA_RX * DH_PKT_CAP_POINT_KEY_WORD_RULE_NUM)

#define DH_PKT_CAP_SPEED_MIN      (0U)
#define DH_PKT_CAP_SPEED_MAX      (300000U)
#define DH_PKT_CAP_SPEED_DEFAULT  (10000U)

#define DH_PKT_ROCE_SPEED_MIN      (10000U)
#define DH_PKT_ROCE_SPEED_MAX      (400000000U)
#define DH_PKT_ROCE_SPEED_DEFAULT  (1000000U)

#define DH_PKT_ROCE_FLOW           (30009)
#define DH_PKT_ROCE_FLOW_PROFILE   (509)
#define DH_PKT_ROCE_CBS            (1280000)

typedef enum zxdh_pkt_cap_point
{
    DH_PKT_CAP_POINT_PANEL_RX = 0,
    DH_PKT_CAP_POINT_PANEL_TX = 1,
    DH_PKT_CAP_POINT_VQM_RX   = 2,
    DH_PKT_CAP_POINT_VQM_TX   = 3,
    DH_PKT_CAP_POINT_RDMA_RX  = 4,
    DH_PKT_CAP_POINT_RDMA_TX  = 5,
    DH_PKT_CAP_POINT_MAX      = 6,
}ZXDH_PKT_CAP_POINT;

typedef enum zxdh_pkt_cap_mode
{
    DH_PKT_CAP_MODE_NORMAL   = 0,
    DH_PKT_CAP_MODE_KEY_WORD = 1,
    DH_PKT_CAP_MODE_MAX      = 2,
}ZXDH_PKT_CAP_MODE;

typedef struct zxdh_pkt_cap_enable_status
{
    ZXIC_UINT8 panel_rx_enable_status;
    ZXIC_UINT8 panel_tx_enable_status;
    ZXIC_UINT8 vqm_rx_enable_status;
    ZXIC_UINT8 vqm_tx_enable_status;
    ZXIC_UINT8 rdma_rx_enable_status;
    ZXIC_UINT8 rdma_tx_enable_status;
}ZXDH_PKT_CAP_ENABLE_STATUS;

typedef struct zxdh_pkt_cap_normal_configure
{
    ZXIC_UINT16 rsv : 6;
    ZXIC_UINT16 sourceid : 1;
    ZXIC_UINT16 dmac : 1;
    ZXIC_UINT16 smac : 1;
    ZXIC_UINT16 ethtype : 1;
    ZXIC_UINT16 sip : 1;
    ZXIC_UINT16 dip : 1;
    ZXIC_UINT16 sport : 1;
    ZXIC_UINT16 dport : 1;
    ZXIC_UINT16 protocol : 1;
    ZXIC_UINT16 qp : 1;
}ZXDH_PKT_CAP_NORMAL_CONFIG;

typedef struct zxdh_pkt_cap_rule
{
    ZXIC_UINT16 dst_vqm_vfid;
    ZXDH_PKT_CAP_NORMAL_CONFIG rule_config;
    ZXIC_UINT32 tcam_index;
    ZXDH_PKT_CAP_KEY pkt_cap_key;
}ZXDH_PKT_CAP_RULE;

/***********************************************************/
/** 抓包初始化函数
* @param   pf_info  镜像流上送PF的信息
*
* @return  DPP_OK  函数执行成功, DPP_ERR  函数执行失败
* @remark  无
* @see
* @author  sl     @date  2024/10/28
************************************************************/
ZXIC_UINT32 dpp_pkt_capture_init(DPP_PF_INFO_T* pf_info);

/***********************************************************/
/** 抓包退出函数
* @param   pf_info  镜像流上送PF的信息
*
* @return  DPP_OK  函数执行成功, DPP_ERR  函数执行失败
* @remark  无
* @see
* @author  sl     @date  2024/10/28
************************************************************/
ZXIC_UINT32 dpp_pkt_capture_uninit(DPP_PF_INFO_T* pf_info);

/***********************************************************/
/** 抓包点使能函数
* @param   pf_info  镜像流上送PF的信息
* @param   capture_pkt_flag  抓包点使能标志位(0:panel_rx, 1:panel_tx, 2:vqm_rx 3:vqm_tx 4:rdma_rx, 5:rdma_tx)
*
* @return  DPP_OK  函数执行成功, DPP_ERR  函数执行失败
* @remark  无
* @see
* @author  sl     @date  2024/10/28
************************************************************/
ZXIC_UINT32 dpp_pkt_capture_enable(DPP_PF_INFO_T* pf_info, ZXDH_PKT_CAP_POINT capture_pkt_flag);

/***********************************************************/
/** 抓包点去使能函数
* @param   pf_info  镜像流上送PF的信息
* @param   capture_pkt_flag  抓包点使能标志位(0:panel_rx, 1:panel_tx, 2:vqm_rx 3:vqm_tx 4:rdma_rx, 5:rdma_tx)
*
* @return  DPP_OK  函数执行成功, DPP_ERR  函数执行失败
* @remark  无
* @see
* @author  sl     @date  2024/10/28
************************************************************/
ZXIC_UINT32 dpp_pkt_capture_disable(DPP_PF_INFO_T* pf_info, ZXDH_PKT_CAP_POINT capture_pkt_flag);

/***********************************************************/
/** 抓包点全去使能函数
* @param   pf_info  镜像流上送PF的信息
*
* @return  DPP_OK  函数执行成功, DPP_ERR  函数执行失败
* @remark  无
* @see
* @author  sl     @date  2024/10/28
************************************************************/
ZXIC_UINT32 dpp_pkt_capture_disable_all(DPP_PF_INFO_T* pf_info);

/***********************************************************/
/** 抓包点使能状态获取函数
* @param   pf_info  镜像流上送PF的信息
* @param   enable_status  抓包点状态
*
* @return  DPP_OK  函数执行成功, DPP_ERR  函数执行失败
* @remark  无
* @see
* @author  sl     @date  2024/10/28
************************************************************/
ZXIC_UINT32 dpp_pkt_capture_enable_status_get(DPP_PF_INFO_T* pf_info, ZXDH_PKT_CAP_ENABLE_STATUS *enable_status);

/***********************************************************/
/** 将rule_index转换成tcam_index
* @param   rule_index  规则索引
* @param   rule_mode   规则模式
* @param   capture_pkt_flag  抓包点使能标志位(0:panel_rx, 1:panel_tx, 2:vqm_rx 3:vqm_tx 4:rdma_rx, 5:rdma_tx)
* @param   tcam_index  tcam表索引
*
* @return  DPP_OK  函数执行成功, DPP_ERR  函数执行失败
* @remark  无
* @see
* @author  sl     @date  2024/10/28
************************************************************/
ZXIC_UINT32 dpp_pkt_capture_rule_index_to_tcam_index(ZXIC_UINT32 rule_index, ZXDH_PKT_CAP_MODE rule_mode, \
                                                     ZXDH_PKT_CAP_POINT capture_pkt_flag, ZXIC_UINT32 *tcam_index);

/***********************************************************/
/** 将tcam_index转换成rule_index
* @param   tcam_index  tcam表索引
* @param   rule_mode   规则模式
* @param   rule_index  规则索引
*
* @return  DPP_OK  函数执行成功, DPP_ERR  函数执行失败
* @remark  无
* @see
* @author  sl     @date  2024/10/28
************************************************************/
ZXIC_UINT32 dpp_pkt_capture_tcam_index_to_rule_index(ZXIC_UINT32 tcam_index, ZXDH_PKT_CAP_MODE *rule_mode, ZXIC_UINT32 *rule_index);

/***********************************************************/
/** 插入表项
* @param   pf_info  镜像流上送PF的信息
* @param   rule     表项信息
*
* @return  DPP_OK  函数执行成功, DPP_ERR  函数执行失败
* @remark  无
* @see
* @author  sl     @date  2024/10/28
************************************************************/
ZXIC_UINT32 dpp_pkt_capture_item_insert(DPP_PF_INFO_T* pf_info, ZXDH_PKT_CAP_RULE *rule);

/***********************************************************/
/** 删除表项
* @param   pf_info    镜像流上送PF的信息
* @param   tcam_index TCAM表索引
*
* @return  DPP_OK  函数执行成功, DPP_ERR  函数执行失败
* @remark  无
* @see
* @author  sl     @date  2024/10/28
************************************************************/
ZXIC_UINT32 dpp_pkt_capture_item_delete(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 tcam_index);

/***********************************************************/
/** 获取所有表项
* @param   pf_info    镜像流上送PF的信息
* @param   rule_array rule数组
* @param   entry_num  表项个数
*
* @return  DPP_OK  函数执行成功, DPP_ERR  函数执行失败
* @remark  无
* @see
* @author  sl     @date  2024/10/28
************************************************************/
ZXIC_UINT32 dpp_pkt_capture_table_dump(DPP_PF_INFO_T* pf_info, ZXDH_PKT_CAP_RULE *rule_array, ZXIC_UINT32 *entry_num);

/***********************************************************/
/** 删除所有表项
* @param   pf_info    镜像流上送PF的信息
*
* @return  DPP_OK  函数执行成功, DPP_ERR  函数执行失败
* @remark  无
* @see
* @author  sl     @date  2024/10/28
************************************************************/
ZXIC_UINT32 dpp_pkt_capture_table_flush(DPP_PF_INFO_T* pf_info);

/***********************************************************/
/** 设置镜像流速率
* @param   pf_info    镜像流上送PF的信息
* @param   speed_kbps 镜像流速率kbps
*
* @return  DPP_OK  函数执行成功, DPP_ERR  函数执行失败
* @remark  无
* @see
* @author  sl     @date  2024/10/28
************************************************************/
ZXIC_UINT32 dpp_pkt_capture_speed_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 speed_kbps);

/***********************************************************/
/** 获取镜像流速率
* @param   pf_info    镜像流上送PF的信息
* @param   speed_kbps 镜像流速率kbps
*
* @return  DPP_OK  函数执行成功, DPP_ERR  函数执行失败
* @remark  无
* @see
* @author  sl     @date  2024/10/28
************************************************************/
ZXIC_UINT32 dpp_pkt_capture_speed_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *speed_kbps);

/***********************************************************/
/** 设置ROCE流速率
* @param   pf_info    镜像流上送PF的信息
* @param   speed_kbps 镜像流速率kbps
*
* @return  DPP_OK  函数执行成功, DPP_ERR  函数执行失败
* @remark  无
* @see
* @author  sun     @date  2026/06/26
************************************************************/
ZXIC_UINT32 dpp_pkt_roce_speed_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 speed_kbps);

#endif // !_DPP_PKT_CAP_TBL_H_