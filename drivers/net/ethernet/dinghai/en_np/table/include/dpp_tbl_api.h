/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_tbl_api.h
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

#ifndef DPP_TBL_API_H
#define DPP_TBL_API_H

#include "zxic_common.h"
#include "dpp_drv_eram.h"
#include "dpp_drv_acl.h"
#include "dpp_tbl_comm.h"
#include "dpp_tbl_stat.h"
#include "dpp_stat_api.h"
#include "dpp_agent_channel.h"
#include "dpp_dtb_table_api.h"

typedef enum _dpp_glb_cfg_reg_index_e
{
    DPP_GLB_CFG_REG_INDEX_0     = 0,
    DPP_GLB_CFG_REG_INDEX_1     = 1,
    DPP_GLB_CFG_REG_INDEX_2     = 2,
    DPP_GLB_CFG_REG_INDEX_3     = 3,
    DPP_GLB_CFG_REG_INDEX_MAX
} DPP_GLB_CFG_REG_INDEX_E;

#define DPP_ACL_ENTRY_INFO_T DPP_DTB_ACL_ENTRY_INFO_T

// byte[15:16]
// #define SRIOV_VPORT_TPID                       ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, rsv6) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_MULTI_HOST_EN              ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, multi_host_enable) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_MULTI_HOST_GROUP_ID        ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, multi_host_group_id) / sizeof(ZXIC_UINT32)))

// byte[13:14]
#define SRIOV_VPORT_1588_EN                    ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, flag_1588_enable) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_K8S_CNI_FLAG               ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, k8s_cni_flag) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_ROCE_OVERLAY_EN            ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, roce_overlay_enable) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_OUTER_L4_CHECKSUM_OFFLOAD  ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, outer_l4_checksum_offload) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_VHCA                       ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, vhca) / sizeof(ZXIC_UINT32)))
// byte[12]
#define SRIOV_VPORT_FRAG_PKT_USE_IPID          ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, frag_pkt_use_ipid) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_RSS_HASH_FACTOR            ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, rss_hash_factor) / sizeof(ZXIC_UINT32)))
// byte[11]
#define SRIOV_VPORT_HASH_ALG                   ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, hash_alg) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_UPLINK_PHY_PORT_ID         ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, uplink_phy_port_id) / sizeof(ZXIC_UINT32)))
// byte[9:10]
#define SRIOV_VPORT_LAG_ID                     ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, lag_id) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_FD_VXLAN_OFFLOAD_EN        ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, fd_vxlan_offload_en) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_PF_VQM_VFID                ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, pf_vqm_vfid) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_MULTI_PLANE_EN             ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, multi_plane_en) / sizeof(ZXIC_UINT32)))
// byte[7:8]
#define SRIOV_VPORT_MTU                        ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, mtu) / sizeof(ZXIC_UINT32)))
// byte[5:6]
#define SRIOV_VPORT_HASH_SEARCH_INDEX          ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, hash_search_index) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_PORT_BASE_QID              ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, port_base_qid) / sizeof(ZXIC_UINT32)))
// byte[4]
#define SRIOV_VPORT_SPOOFCHK_EN_OFF            ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, spoof_check_enable) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_NP_INGRESS_TM_EN_OFF       ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, np_ingress_tm_enable) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_NP_EGRESS_TM_EN_OFF        ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, np_egress_tm_enable) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_NP_INGRESS_MODE            ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, np_ingress_meter_mode) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_NP_EGRESS_MODE             ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, np_egress_meter_mode) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_NP_INGRESS_METER_EN_OFF    ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, np_ingress_meter_enable) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_NP_EGRESS_METER_EN_OFF     ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, np_egress_meter_enable) / sizeof(ZXIC_UINT32)))
// byte[3]
#define SRIOV_VPORT_VIRTIO_EN_OFF             ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, virtio_enable) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_VIRTIO_VERSION            ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, virtio_version) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_IS_VF                      ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, is_vf) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_VEPA_EN_OFF                ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, vepa_enable) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_LAG_EN_OFF                 ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, lag_enable) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_FD_EN_OFF                  ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, fd_enable) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_INLINE_SEC_OFFLOAD         ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, inline_sec_offload) / sizeof(ZXIC_UINT32)))
// byte[2]
#define SRIOV_VPORT_BUSINESS_EN_OFF            ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, business_enable) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_IS_UP                      ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, is_up) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_OUTER_IP_CHECKSUM_OFFLOAD  ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, outer_ip_checksum_offload) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_IP_CHKSUM                  ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, ip_checksum_offload) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_TCP_UDP_CHKSUM             ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, tcp_udp_checksum_offload) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_IP_RECOMBINE               ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, ip_recombine_offload) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_IPV4_TCP_ASSEMBLE          ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, lro_offload) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_IPV6_TCP_ASSEMBLE          ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, lro_offload) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_ACCELERATOR_OFFLOAD_FLAG   ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, accelerator_offload_flag) / sizeof(ZXIC_UINT32)))
// byte[1]
#define SRIOV_VPORT_HW_BOND_EN_OFF             ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, hw_bond_enable) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_RDMA_OFFLOAD_EN_OFF        ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, rdma_offload_enable) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_PROMISC_EN                 ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, promisc_enable) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_VLAN_OFFLOAD_EN            ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, sriov_vlan_offload_enable) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_BUSINESS_VLAN_OFFLOAD_EN   ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, sriov_business_vlan_offload_enable) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_RSS_EN_OFF                 ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, rss_enable) / sizeof(ZXIC_UINT32)))
#define SRIOV_VPORT_MTU_OFFLOAD_EN_OFF         ((ZXIC_UINT32)(offsetof(ZXDH_SRIOV_VPORT_T, mtu_offload_enable) / sizeof(ZXIC_UINT32)))


#define UPLINK_PHY_PORT_PF_VQM_VFID              ((ZXIC_UINT32)(offsetof(ZXDH_UPLINK_PHY_PORT_T, pf_vqm_vfid) / sizeof(ZXIC_UINT32)))
#define UPLINK_PHY_PORT_LACP_PF_MEMPORT_QID      ((ZXIC_UINT32)(offsetof(ZXDH_UPLINK_PHY_PORT_T, lacp_pf_memport_qid) / sizeof(ZXIC_UINT32)))
#define UPLINK_PHY_PORT_LACP_PF_VQM_VFID         ((ZXIC_UINT32)(offsetof(ZXDH_UPLINK_PHY_PORT_T, lacp_pf_vqm_vfid) / sizeof(ZXIC_UINT32)))
#define UPLINK_PHY_PORT_IS_UP                    ((ZXIC_UINT32)(offsetof(ZXDH_UPLINK_PHY_PORT_T, is_up) / sizeof(ZXIC_UINT32)))
#define UPLINK_PHY_PORT_BOND_LINK_UP             ((ZXIC_UINT32)(offsetof(ZXDH_UPLINK_PHY_PORT_T, bond_link_up) / sizeof(ZXIC_UINT32)))
#define UPLINK_PHY_PORT_HW_BOND_ENABLE           ((ZXIC_UINT32)(offsetof(ZXDH_UPLINK_PHY_PORT_T, hw_bond_enable) / sizeof(ZXIC_UINT32)))
#define UPLINK_PHY_PORT_MTU                      ((ZXIC_UINT32)(offsetof(ZXDH_UPLINK_PHY_PORT_T, mtu) / sizeof(ZXIC_UINT32)))
#define UPLINK_PHY_PORT_MTU_OFFLOAD_ENABLE       ((ZXIC_UINT32)(offsetof(ZXDH_UPLINK_PHY_PORT_T, mtu_offload_enable) / sizeof(ZXIC_UINT32)))
#define UPLINK_PHY_PORT_TM_BASE_QUEUE            ((ZXIC_UINT32)(offsetof(ZXDH_UPLINK_PHY_PORT_T, tm_base_queue) / sizeof(ZXIC_UINT32)))
#define UPLINK_PHY_PORT_PTP_PORT_VFID            ((ZXIC_UINT32)(offsetof(ZXDH_UPLINK_PHY_PORT_T, ptp_port_vfid) / sizeof(ZXIC_UINT32)))
#define UPLINK_PHY_PORT_MAGIC_PACKET_ENABLE      ((ZXIC_UINT32)(offsetof(ZXDH_UPLINK_PHY_PORT_T, magic_packet_enable) / sizeof(ZXIC_UINT32)))
#define UPLINK_PHY_PORT_TM_SHAPE_ENABLE          ((ZXIC_UINT32)(offsetof(ZXDH_UPLINK_PHY_PORT_T, tm_shape_enable) / sizeof(ZXIC_UINT32)))
#define UPLINK_PHY_PORT_PTP_TC_ENABLE            ((ZXIC_UINT32)(offsetof(ZXDH_UPLINK_PHY_PORT_T, ptp_tc_enable) / sizeof(ZXIC_UINT32)))
#define UPLINK_PHY_PORT_TRUST_MODE               ((ZXIC_UINT32)(offsetof(ZXDH_UPLINK_PHY_PORT_T, trust_mode) / sizeof(ZXIC_UINT32)))
#define UPLINK_PHY_PORT_PRIMARY_PF_VQM_VFID      ((ZXIC_UINT32)(offsetof(ZXDH_UPLINK_PHY_PORT_T, primary_pf_vqm_vfid) / sizeof(ZXIC_UINT32)))
#define UPLINK_PHY_PORT_SRIOV_HD_BOND_EN         ((ZXIC_UINT32)(offsetof(ZXDH_UPLINK_PHY_PORT_T, sriov_hdbond_enable) / sizeof(ZXIC_UINT32)))
#define UPLINK_PHY_PORT_ROCE_OVERLAY_EN          ((ZXIC_UINT32)(offsetof(ZXDH_UPLINK_PHY_PORT_T, roce_overlay_enable) / sizeof(ZXIC_UINT32)))
#define UPLINK_PHY_PORT_MULTI_HOST_EN            ((ZXIC_UINT32)(offsetof(ZXDH_UPLINK_PHY_PORT_T, multi_host_enable) / sizeof(ZXIC_UINT32)))
#define UPLINK_PHY_PORT_MULTI_HOST_GROUP_ID      ((ZXIC_UINT32)(offsetof(ZXDH_UPLINK_PHY_PORT_T, multi_host_group_id) / sizeof(ZXIC_UINT32)))

#define VLAN_SRIOV_VLAN_TCI                          ((ZXIC_UINT32)(offsetof(ZXDH_VQM_VFID_VLAN_T, sriov_vlan_tci) / sizeof(ZXIC_UINT32)))
#define VLAN_SRIOV_VLAN_TPID                         ((ZXIC_UINT32)(offsetof(ZXDH_VQM_VFID_VLAN_T, sriov_vlan_tpid) / sizeof(ZXIC_UINT32)))
#define VLAN_SRIOV_BUSINESS_VLAN_TPID                ((ZXIC_UINT32)(offsetof(ZXDH_VQM_VFID_VLAN_T, sriov_business_vlan_tpid) / sizeof(ZXIC_UINT32)))
#define VLAN_SRIOV_BUSINESS_VLAN_STRIP_OFFLIAD       ((ZXIC_UINT32)(offsetof(ZXDH_VQM_VFID_VLAN_T, sriov_business_vlan_strip_offload) / sizeof(ZXIC_UINT32)))
#define VLAN_SRIOV_BUSINESS_QINQ_VLAN_STRIP_OFFLOAD  ((ZXIC_UINT32)(offsetof(ZXDH_VQM_VFID_VLAN_T, sriov_business_qinq_vlan_strip_offload) / sizeof(ZXIC_UINT32)))
#define VLAN_SRIOV_BUSINESS_VLAN_FILTER              ((ZXIC_UINT32)(offsetof(ZXDH_VQM_VFID_VLAN_T, sriov_business_vlan_filter) / sizeof(ZXIC_UINT32)))

#define DPP_RC_TBL_BASE                     (DPP_RC_DTB_BASE | 0x80000000)
#define DPP_RC_TBL_IS_FULL                  (DPP_RC_TBL_BASE | 0x0)

#define DPP_STAT_ERAM_DEPTH_128BIT (0X12000)

#define DPP_STAT_ERAM_DEPTH_64BIT (DPP_STAT_ERAM_DEPTH_128BIT * 2)

typedef struct dpp_stat_recode_t
{
    ZXIC_UINT64 unlock_cnt;
    ZXIC_UINT64 *p_dpp_stat_recode;
}DPP_STAT_RECODE_T;

typedef enum
{
    HASH_FLUSH_ONLINE_MODE = 0,         /*删除软硬件hash表*/
    HASH_FLUSH_OFFLINE_MODE = 1,        /*仅删除硬件hash表*/
    HASH_FLUSH_MODE_MAX
}HASH_FLUSH_MODE_ENUM;

ZXIC_UINT32 dpp_vport_create(DPP_PF_INFO_T* pf_info);
ZXIC_UINT32 dpp_vport_create_by_vqm_vfid(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 vqm_vfid);
ZXIC_UINT32 dpp_vport_delete(DPP_PF_INFO_T* pf_info);
ZXIC_UINT32 dpp_vport_attr_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 attr, ZXIC_UINT32 value);
ZXIC_UINT32 dpp_vport_attr_get(DPP_PF_INFO_T* pf_info, ZXDH_SRIOV_VPORT_T *port_attr_entry);
ZXIC_UINT32 dpp_vport_rx_flow_hash_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 hash_mode);
ZXIC_UINT32 dpp_vport_rx_flow_hash_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *hash_mode);
ZXIC_UINT32 dpp_vport_base_qid_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *base_qid);
ZXIC_UINT32 dpp_vport_hash_index_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *hash_index);
ZXIC_UINT32 dpp_vport_hash_funcs_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 funcs);
ZXIC_UINT32 dpp_vport_rss_en_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 enable);
ZXIC_UINT32 dpp_vport_fd_en_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 enable);
ZXIC_UINT32 dpp_vport_virtio_en_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 enable);
ZXIC_UINT32 dpp_vport_virtio_version_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 version);
ZXIC_UINT32 dpp_vport_promisc_en_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 enable);
ZXIC_UINT32 dpp_vport_business_vlan_offload_en_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 enable);
ZXIC_UINT32 dpp_vport_vlan_offload_en_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 enable);

ZXIC_UINT32 dpp_vlan_filter_init(DPP_PF_INFO_T* pf_info);
ZXIC_UINT32 dpp_add_vlan_filter(DPP_PF_INFO_T* pf_info, ZXIC_UINT16 vlan_id);
ZXIC_UINT32 dpp_del_vlan_filter(DPP_PF_INFO_T* pf_info, ZXIC_UINT16 vlan_id);

ZXIC_UINT32 dpp_vport_bond_pf(DPP_PF_INFO_T* pf_info);
ZXIC_UINT32 dpp_vport_unbond_pf(DPP_PF_INFO_T* pf_info);

ZXIC_UINT32 dpp_rxfh_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *queue_list, ZXIC_UINT32 queue_num);
ZXIC_UINT32 dpp_rxfh_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *queue_list, ZXIC_UINT32 queue_num);
ZXIC_UINT32 dpp_rxfh_del(DPP_PF_INFO_T* pf_info);
ZXIC_UINT32 dpp_thash_key_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 *hash_key, ZXIC_UINT32 key_num);
ZXIC_UINT32 dpp_thash_key_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 *hash_key, ZXIC_UINT32 key_num);
ZXIC_UINT32 dpp_add_mac(DPP_PF_INFO_T* pf_info, ZXIC_CONST ZXIC_VOID *mac, ZXIC_UINT16 sriov_vlan_tpid, ZXIC_UINT16 sriov_vlan_id);
ZXIC_UINT32 dpp_del_mac(DPP_PF_INFO_T* pf_info, ZXIC_CONST ZXIC_VOID *mac, ZXIC_UINT16 sriov_vlan_tpid, ZXIC_UINT16 sriov_vlan_id);
ZXIC_UINT32 dpp_unicast_mac_search(DPP_PF_INFO_T* pf_info, ZXIC_CONST ZXIC_VOID *mac, ZXIC_UINT16 sriov_vlan_tpid, ZXIC_UINT16 sriov_vlan_id, ZXIC_UINT16 *current_vport);
ZXIC_UINT32 dpp_batch_add_unicast_mac(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 mac_num, ZXIC_CONST ZXIC_VOID *l2key);
ZXIC_UINT32 dpp_batch_del_unicast_mac(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 mac_num, ZXIC_CONST ZXIC_VOID *l2key);
ZXIC_UINT32 dpp_unicast_mac_dump(DPP_PF_INFO_T* pf_info, MAC_VPORT_INFO *p_mac_arr, ZXIC_UINT32 *p_mac_num);
ZXIC_UINT32 dpp_unicast_mac_transfer(DPP_PF_INFO_T* pf_info, DPP_PF_INFO_T* new_pf_info);
ZXIC_UINT32 dpp_unicast_mac_max_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *max_num);
ZXIC_UINT32 dpp_unicast_all_mac_delete(DPP_PF_INFO_T* pf_info);
ZXIC_UINT32 dpp_unicast_all_mac_online_delete(DPP_PF_INFO_T* pf_info);
ZXIC_UINT32 dpp_unicast_all_mac_soft_delete(DPP_PF_INFO_T* pf_info);

ZXIC_UINT32 dpp_multi_mac_add_member(DPP_PF_INFO_T* pf_info, ZXIC_CONST ZXIC_VOID *mac);
ZXIC_UINT32 dpp_multi_mac_del_member(DPP_PF_INFO_T* pf_info, ZXIC_CONST ZXIC_VOID *mac);
ZXIC_UINT32 dpp_batch_add_multicast_mac(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 mac_num, ZXIC_CONST ZXIC_VOID *mac);
ZXIC_UINT32 dpp_batch_del_multicast_mac(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 mac_num, ZXIC_CONST ZXIC_VOID *mac);
ZXIC_UINT32 dpp_multicast_mac_dump(DPP_PF_INFO_T* pf_info, MAC_VPORT_INFO *p_mac_arr, ZXIC_UINT32 *p_mac_num);
ZXIC_UINT32 dpp_multicast_mac_transfer(DPP_PF_INFO_T* pf_info, DPP_PF_INFO_T* new_pf_info);
ZXIC_UINT32 dpp_multicast_mac_max_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *max_num);
ZXIC_UINT32 dpp_multicast_all_mac_delete(DPP_PF_INFO_T* pf_info);
ZXIC_UINT32 dpp_multicast_all_mac_online_delete(DPP_PF_INFO_T* pf_info);
ZXIC_UINT32 dpp_multicast_all_mac_soft_delete(DPP_PF_INFO_T* pf_info);

ZXIC_UINT32 dpp_ptp_port_vfid_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 ptp_port_vfid);
ZXIC_UINT32 dpp_ptp_tc_enable_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 ptp_tc_enable);

ZXIC_UINT32 dpp_ipsec_enc_entry_add(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT8 *sip, ZXIC_UINT8 *dip,
                                    ZXIC_UINT8 *sip_mask, ZXIC_UINT8 *dip_mask, ZXIC_UINT32 is_ipv4, ZXIC_UINT32 sa_id);
ZXIC_UINT32 dpp_ipsec_enc_entry_del(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index);

ZXIC_UINT32 dpp_lag_group_create(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 lag_id);
ZXIC_UINT32 dpp_lag_group_delete(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 lag_id);
ZXIC_UINT32 dpp_lag_mode_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 lag_id, ZXIC_UINT8 mode);
ZXIC_UINT32 dpp_lag_group_hash_factor_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 lag_id, ZXIC_UINT8 factor);
ZXIC_UINT32 dpp_lag_group_hash_factor_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 lag_id, ZXIC_UINT8 *factor);
ZXIC_UINT32 dpp_lag_group_member_add(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 lag_id, ZXIC_UINT8 uplink_phy_port_id);
ZXIC_UINT32 dpp_lag_group_member_del(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 lag_id, ZXIC_UINT8 uplink_phy_port_id);
ZXIC_UINT32 dpp_lag_hit_flag_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 lag_id, ZXIC_UINT8* hit_flag);

ZXIC_UINT32 dpp_uplink_phy_bond_vport(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 uplink_phy_id);
ZXIC_UINT32 dpp_uplink_phy_hardware_bond_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 uplink_phy_id, ZXIC_UINT8 enable);
ZXIC_UINT32 dpp_uplink_phy_lacp_pf_vqm_vfid_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 uplink_phy_id, ZXIC_UINT16 vqm_vfid);
ZXIC_UINT32 dpp_uplink_phy_lacp_pf_memport_qid_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 uplink_phy_id, ZXIC_UINT16 qid);
ZXIC_UINT32 dpp_uplink_phy_attr_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 uplink_phy_id, ZXIC_UINT32 attr, ZXIC_UINT32 value);

ZXIC_UINT32 dpp_vport_uc_promisc_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 enable);
ZXIC_UINT32 dpp_vport_mc_promisc_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 enable);

ZXIC_UINT32 dpp_std_nic_stat_recode_init(DPP_PF_INFO_T* pf_info);
ZXIC_UINT32 dpp_std_nic_stat_recode_uninit(DPP_PF_INFO_T* pf_info);
ZXIC_UINT32 dpp_stat_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt);
ZXIC_UINT32 dpp_stat_item_cnt_get(DPP_PF_INFO_T* pf_info, 
                            ZXIC_UINT32 stat_item_no, 
                            ZXIC_UINT32 index, 
                            ZXIC_UINT32 rd_mode, 
                            DPP_STAT_VALUE_U *p_stat_value);
ZXIC_UINT32 dpp_stat_cnt_get_128(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt);
ZXIC_UINT32 dpp_stat_mc_packet_rx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt);
ZXIC_UINT32 dpp_stat_bc_packet_rx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt);
ZXIC_UINT32 dpp_stat_1588_packet_rx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt);
ZXIC_UINT32 dpp_stat_1588_packet_tx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt);
ZXIC_UINT32 dpp_stat_1588_packet_drop_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt);
ZXIC_UINT32 dpp_stat_1588_enc_packet_rx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt);
ZXIC_UINT32 dpp_stat_1588_enc_packet_tx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt);
ZXIC_UINT32 dpp_stat_spoof_packet_drop_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt);
ZXIC_UINT32 dpp_stat_mcode_packet_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt);
ZXIC_UINT32 dpp_stat_port_RDMA_packet_msg_tx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt);
ZXIC_UINT32 dpp_stat_port_RDMA_packet_msg_rx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt);
ZXIC_UINT32 dpp_stat_plcr_packet_drop_tx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt);
ZXIC_UINT32 dpp_stat_plcr_packet_drop_rx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt);
ZXIC_UINT32 dpp_stat_MTU_packet_msg_tx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt);
ZXIC_UINT32 dpp_stat_MTU_packet_msg_rx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt);
ZXIC_UINT32 dpp_stat_port_uc_packet_rx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt);
ZXIC_UINT32 dpp_stat_port_uc_packet_tx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt);
ZXIC_UINT32 dpp_stat_port_mc_packet_rx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt);
ZXIC_UINT32 dpp_stat_port_mc_packet_tx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt);
ZXIC_UINT32 dpp_stat_port_bc_packet_rx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt);
ZXIC_UINT32 dpp_stat_port_bc_packet_tx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt);
ZXIC_UINT32 dpp_stat_fd_stat_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt);

ZXIC_UINT32 dpp_vport_vhca_id_add(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 vhca_id);
ZXIC_UINT32 dpp_vport_vhca_id_del(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 vhca_id);
ZXIC_UINT32 dpp_add_rdma_trans_item(DPP_PF_INFO_T* pf_info, ZXIC_CONST ZXIC_VOID *mac, ZXIC_CONST ZXIC_UINT16 vhcaId);
ZXIC_UINT32 dpp_del_rdma_trans_item(DPP_PF_INFO_T* pf_info, ZXIC_CONST ZXIC_VOID *mac);
ZXIC_UINT32 dpp_rdma_trans_item_soft_delete(DPP_PF_INFO_T* pf_info);
ZXIC_UINT32 dpp_get_rdma_trans_item_pos(DPP_PF_INFO_T* pf_info, ZXIC_CONST ZXIC_VOID *mac, DPP_HASH_ZCAM_POS_INFO *p_pos_info);
ZXIC_UINT32 dpp_search_rdma_trans_item(DPP_PF_INFO_T* pf_info, ZXIC_CONST ZXIC_VOID *mac, ZXIC_UINT32 *vhca_id);

ZXIC_UINT32 dpp_vqm_vfid_vlan_init(DPP_PF_INFO_T* pf_info);
ZXIC_UINT32 dpp_vqm_vfid_vlan_delete(DPP_PF_INFO_T* pf_info);
ZXIC_UINT32 dpp_vqm_vfid_vlan_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 attr, ZXIC_UINT32 value);
ZXIC_UINT32 dpp_vqm_vfid_vlan_get(DPP_PF_INFO_T* pf_info, ZXDH_VQM_VFID_VLAN_T *vqm_vfid_vlan_entry);
ZXIC_UINT32 dpp_fd_acl_index_request(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *p_index);
ZXIC_UINT32 dpp_fd_acl_index_release(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index);
ZXIC_UINT32 dpp_fd_acl_entry_add(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 handle, ZXIC_UINT8 *key, ZXIC_UINT8 *key_mask, ZXIC_UINT8 *result);
ZXIC_UINT32 dpp_fd_acl_entry_del(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index);
ZXIC_UINT32 dpp_fd_acl_entry_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 handle, ZXIC_UINT8 *key, ZXIC_UINT8 *key_mask, ZXIC_UINT8 *result);
ZXIC_UINT32 dpp_fd_acl_entry_search(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 handle, ZXIC_UINT8 *key, ZXIC_UINT8 *key_mask, ZXIC_UINT8 *result);
ZXIC_UINT32 dpp_fd_acl_all_delete(DPP_PF_INFO_T* pf_info);
ZXIC_UINT32 dpp_fd_acl_stat_clear(DPP_PF_INFO_T* pf_info);
ZXIC_UINT32 dpp_fd_acl_index_dump(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *p_index_array, ZXIC_UINT32 *index_num);
ZXIC_UINT32 dpp_glb_cfg_set_0(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 glb_cfg_data_0);
ZXIC_UINT32 dpp_glb_cfg_set_1(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 glb_cfg_data_1);
ZXIC_UINT32 dpp_glb_cfg_set_2(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 glb_cfg_data_2);
ZXIC_UINT32 dpp_glb_cfg_set_3(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 glb_cfg_data_3);
ZXIC_UINT32 dpp_glb_cfg_get_0(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *p_glb_cfg_data_0);
ZXIC_UINT32 dpp_glb_cfg_get_1(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *p_glb_cfg_data_1);
ZXIC_UINT32 dpp_glb_cfg_get_2(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *p_glb_cfg_data_2);
ZXIC_UINT32 dpp_glb_cfg_get_3(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *p_glb_cfg_data_3);
ZXIC_UINT32 dpp_l2d_psn_cfg_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 psn_cfg);
ZXIC_UINT32 dpp_l2d_psn_cfg_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *p_psn_cfg);
ZXIC_UINT32 dpp_stat_asn_phyport_rx_pkt_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt);
ZXIC_UINT32 dpp_stat_psn_phyport_tx_pkt_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt);
ZXIC_UINT32 dpp_stat_psn_phyport_rx_pkt_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt);
ZXIC_UINT32 dpp_stat_psn_ack_phyport_tx_pkt_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt);
ZXIC_UINT32 dpp_stat_psn_ack_phyport_rx_pkt_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt);
ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_write(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 start_bit_no, ZXIC_UINT32 end_bit_no,
                                            ZXIC_UINT32 glb_cfg_data_1);
ZXIC_UINT32 dpp_mcode_feature_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT64 *feature);
ZXIC_UINT32 dpp_pktrx_tcam_icmp_item_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 *pMac, ZXIC_UINT32 phyport);
ZXIC_UINT32 dpp_pktrx_udf_icmp_item_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 phyport);

ZXIC_UINT32 dpp_eram_entry_insert(DPP_PF_INFO_T* pf_info,ZXIC_UINT32 sdt_no,ZXIC_UINT32 index,ZXIC_UINT8 *p_data);
ZXIC_UINT32 dpp_eram_entry_delete(DPP_PF_INFO_T* pf_info,ZXIC_UINT32 sdt_no,ZXIC_UINT32 index);
ZXIC_UINT32 dpp_eram_entry_get(DPP_PF_INFO_T* pf_info,ZXIC_UINT32 sdt_no,ZXIC_UINT32 index,ZXIC_UINT8 *p_data);
ZXIC_UINT32 dpp_pktrx_mcode_port_cfg_read(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 port_id, ZXIC_UINT32 *port_cfg_data);
ZXIC_UINT32 dpp_pktrx_mcode_port_cfg_write(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 port_id, ZXIC_UINT32 index, ZXIC_UINT32 start_bit_no,
                                           ZXIC_UINT32 end_bit_no,ZXIC_UINT32 port_cfg_data);
ZXIC_UINT32 dpp_hash_entry_insert(DPP_PF_INFO_T* pf_info,ZXIC_UINT32 sdt_no,ZXIC_UINT8 *key,ZXIC_UINT8 *rst);
ZXIC_UINT32 dpp_hash_entry_delete(DPP_PF_INFO_T* pf_info,ZXIC_UINT32 sdt_no,ZXIC_UINT8 *key);
ZXIC_UINT32 dpp_hash_entry_get(DPP_PF_INFO_T* pf_info,ZXIC_UINT32 sdt_no, ZXIC_UINT8 *key,ZXIC_UINT8 *rst, ZXIC_UINT32 srch_mode);
ZXIC_UINT32 dpp_hash_entry_flush(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, HASH_FLUSH_MODE_ENUM flush_mode);
ZXIC_UINT32 dpp_hash_entry_soft_delete_by_sdt(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no);
ZXIC_UINT32 dpp_acl_entry_add(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 entry_num, DPP_ACL_ENTRY_INFO_T *p_acl_entry_info);
ZXIC_UINT32 dpp_acl_entry_del(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 handle);
ZXIC_UINT32 dpp_acl_entry_del_by_key(DPP_PF_INFO_T* pf_info, 
                                  ZXIC_UINT32 sdt_no, 
                                  ZXIC_UINT32 entry_num, 
                                  DPP_ACL_ENTRY_INFO_T *p_acl_entry_info);
ZXIC_UINT32 dpp_acl_entry_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, DPP_ACL_ENTRY_INFO_T *p_acl_entry_info);
ZXIC_UINT32 dpp_channel_stat_info_get(DPP_PF_INFO_T* pf_info, DPP_AGENT_CHANNEL_STAT_INFO_T *p_stats_info, DPP_PRIO_STAT_DATA_T *p_stats_data);
ZXIC_UINT32 dpp_acl_entry_search_by_key(DPP_PF_INFO_T* pf_info, 
                                  ZXIC_UINT32 sdt_no, 
                                  ZXIC_UINT32 entry_num, 
                                  DPP_ACL_ENTRY_INFO_T *p_acl_entry_info);
ZXIC_UINT32 dpp_acl_entry_dump_by_vport(DPP_PF_INFO_T* pf_info, 
                                  ZXIC_UINT32 sdt_no, 
                                  ZXIC_UINT32 *entry_num, 
                                  DPP_ACL_ENTRY_INFO_T *p_acl_entry_info);
ZXIC_UINT32 dpp_acl_entry_flush_by_vport(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no);
ZXIC_UINT32 dpp_acl_entry_del_by_index(DPP_PF_INFO_T* pf_info, 
                                  ZXIC_UINT32 sdt_no, 
                                  ZXIC_UINT32 entry_num, 
                                  DPP_ACL_ENTRY_INFO_T *p_acl_entry_info);
ZXIC_UINT32 dpp_acl_entry_search_by_index(DPP_PF_INFO_T* pf_info, 
                                  ZXIC_UINT32 sdt_no, 
                                  ZXIC_UINT32 entry_num, 
                                  DPP_ACL_ENTRY_INFO_T *p_acl_entry_info);
ZXIC_UINT32 dpp_acl_entry_flush_all(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no);
ZXIC_UINT32 dpp_acl_entry_dump_all(DPP_PF_INFO_T* pf_info, 
                                  ZXIC_UINT32 sdt_no, 
                                  ZXIC_UINT32 *entry_num, 
                                  DPP_ACL_ENTRY_INFO_T *p_acl_entry_info);


ZXIC_UINT32 dpp_acl_index_request(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 *p_index);
ZXIC_UINT32 dpp_acl_index_release(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 index);
ZXIC_UINT32 dpp_acl_index_unused_num(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 *unused_num);
ZXIC_UINT32 dpp_acl_index_dump(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 *index_num, ZXIC_UINT32 *p_index_array);
ZXIC_UINT32 dpp_acl_index_max_num(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 *p_max_num);
ZXIC_UINT32 dpp_pktrx_tcam_pfc_set(DPP_PF_INFO_T *pf_info, ZXIC_UINT8 pfc_map);
ZXIC_UINT32 dpp_pktrx_tcam_pfc_get(DPP_PF_INFO_T *pf_info, ZXIC_UINT8 *pfc_map);
ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_wr(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 reg_index, ZXIC_UINT32 start_bit_no, 
                                            ZXIC_UINT32 end_bit_no, ZXIC_UINT32 glb_cfg_data);
ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_rd(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 reg_index, ZXIC_UINT32 start_bit_no, 
                                        ZXIC_UINT32 end_bit_no, ZXIC_UINT32 *p_glb_cfg_data);
ZXIC_UINT32 dpp_hash_item_num_by_soft(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 *p_item_num);
#endif
