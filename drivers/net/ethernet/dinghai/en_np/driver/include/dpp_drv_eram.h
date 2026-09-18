/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_drv_eram.h
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

#ifndef DPP_DRV_ERAM_H
#define DPP_DRV_ERAM_H

#include "zxic_common.h"
#include "dpp_apt_se_api.h"
#include "dpp_apt_se.h"

typedef struct zxdh_vxlan_t
{
    ZXIC_UINT64 port : 16;
    ZXIC_UINT64 rsv : 47;
    ZXIC_UINT64 hit_flag : 1;
} ZXDH_VXLAN_T;

typedef struct zxdh_sriov_vport_t
{
    // byte[15:16]
    ZXIC_UINT32 rsv6/* : 16; */;

    // byte[13:14]
    ZXIC_UINT32 vhca/* : 10; */;
    ZXIC_UINT32 rsv5/* : 5; */;

    // byte[12]
    ZXIC_UINT32 rss_hash_factor/* : 8; */;

    // byte[11]
    ZXIC_UINT32 hash_alg/* : 4; */;
    ZXIC_UINT32 uplink_phy_port_id/* : 4; */;

    // byte[9:10]
    ZXIC_UINT32 lag_id/* : 3; */;
    ZXIC_UINT32 fd_vxlan_offload_en/* : 1; */;
    ZXIC_UINT32 pf_vqm_vfid/* : 11; */;   
    ZXIC_UINT32 rsv3/* : 1; */;

    // byte[7:8]
    ZXIC_UINT32 mtu/* : 16; */;

    // byte[5:6]
    ZXIC_UINT32 port_base_qid/* : 12; */;
    ZXIC_UINT32 hash_search_index/* : 3; */;
    ZXIC_UINT32 rsv2/* : 1; */;
    
    // byte[4]
    ZXIC_UINT32 np_egress_meter_enable/* : 1; */;
    ZXIC_UINT32 np_ingress_meter_enable/* : 1; */;
    ZXIC_UINT32 np_egress_meter_mode/* : 1; */;
    ZXIC_UINT32 np_ingress_meter_mode/* : 1; */;
    ZXIC_UINT32 np_egress_tm_enable/* : 1; */;
    ZXIC_UINT32 np_ingress_tm_enable/* : 1; */;
    ZXIC_UINT32 rsv1/* : 1; */;
    ZXIC_UINT32 spoof_check_enable/* : 1; */;

    // byte[3]
    ZXIC_UINT32 inline_sec_offload/* : 1; */;
    ZXIC_UINT32 fd_enable/* : 1; */;
    ZXIC_UINT32 lag_enable/* : 1; */;
    ZXIC_UINT32 vepa_enable/* : 1; */;
    ZXIC_UINT32 is_vf/* : 1; */;
    ZXIC_UINT32 virtio_version/* : 2; */;
    ZXIC_UINT32 virtio_enable/* : 1; */;
    
    // byte[2]
    ZXIC_UINT32 accelerator_offload_flag/* : 1; */;
    ZXIC_UINT32 lro_offload/* : 1; */;
    ZXIC_UINT32 ip_recombine_offload/* : 1; */;
    ZXIC_UINT32 tcp_udp_checksum_offload/* : 1; */;
    ZXIC_UINT32 ip_checksum_offload/* : 1; */;
    ZXIC_UINT32 outer_ip_checksum_offload/* : 1; */;
    ZXIC_UINT32 is_up/* : 1; */;
    ZXIC_UINT32 business_enable/* : 1; */;

    // byte[1]
    ZXIC_UINT32 hw_bond_enable/* : 1; */;
    ZXIC_UINT32 rdma_offload_enable/* : 1; */;
    ZXIC_UINT32 promisc_enable/* : 1; */;
    ZXIC_UINT32 sriov_vlan_offload_enable/* : 1; */;
    ZXIC_UINT32 sriov_business_vlan_offload_enable/* : 1; */;
    ZXIC_UINT32 rss_enable/* : 1; */;
    ZXIC_UINT32 mtu_offload_enable/* : 1; */;
    ZXIC_UINT32 hit_flag/*: 1; */;

    // byte[13:14]
    ZXIC_UINT32 flag_1588_enable/*: 1; */;
    ZXIC_UINT32 k8s_cni_flag/*: 1; */;
    ZXIC_UINT32 roce_overlay_enable/*: 1; */;

    // byte[15]
    ZXIC_UINT32 multi_host_group_id  /* : 3; */;
    ZXIC_UINT32 multi_host_enable    /* : 1; */;

    // byte[12]
    ZXIC_UINT32 frag_pkt_use_ipid   /* : 1; */;

    // byte[13:14]
    ZXIC_UINT32 outer_l4_checksum_offload   /* : 1; */;
    
    //byte[9:10]
    ZXIC_UINT32 multi_plane_en  /* : 1; */;
} ZXDH_SRIOV_VPORT_T;

// 注意新增字段会打乱原有顺序
typedef struct zxdh_uplink_phy_port_t
{
    ZXIC_UINT32 rsv6                /* : 5; */;
    ZXIC_UINT32 pf_vqm_vfid         /* : 11; */;
    ZXIC_UINT32 rsv5                /* : 5; */;
    ZXIC_UINT32 lacp_pf_memport_qid /* : 12; */;
    ZXIC_UINT32 rsv4                /* : 4; */;
    ZXIC_UINT32 lacp_pf_vqm_vfid    /* : 11; */;
    ZXIC_UINT32 rsv3                /* : 1; */;
    ZXIC_UINT32 is_up               /* : 1; */;
    ZXIC_UINT32 bond_link_up        /* : 1; */;
    ZXIC_UINT32 hw_bond_enable      /* : 1; */;
    ZXIC_UINT32 mtu                 /* : 16; */;
    ZXIC_UINT32 mtu_offload_enable  /* : 1; */;
    ZXIC_UINT32 rsv2                /* : 3; */;
    ZXIC_UINT32 tm_base_queue       /* : 12; */;
    ZXIC_UINT32 ptp_port_vfid       /* : 11; */;
    ZXIC_UINT32 rsv1                /* : 15 */;
    ZXIC_UINT32 magic_packet_enable /* : 1; */;
    ZXIC_UINT32 tm_shape_enable     /* : 1; */;
    ZXIC_UINT32 ptp_tc_enable       /* : 2; */;
    ZXIC_UINT32 trust_mode          /* : 1; */;
    ZXIC_UINT32 hit_flag            /* : 1; */;
    ZXIC_UINT32 primary_pf_vqm_vfid /* : 11; */;
    ZXIC_UINT32 sriov_hdbond_enable /* : 1; */;
    ZXIC_UINT32 multi_host_group_id /* : 3; */;
    ZXIC_UINT32 multi_host_enable   /* : 1; */;
    ZXIC_UINT32 roce_overlay_enable /* : 1; */;
} ZXDH_UPLINK_PHY_PORT_T;

typedef struct zxdh_dscp_to_up_t
{
    ZXIC_UINT32 rsv2                /* : 32; */;
    ZXIC_UINT32 up                  /* : 3; */;
    ZXIC_UINT32 rsv1                /* : 28; */;
    ZXIC_UINT32 hit_flag            /* : 1;  */;
} ZXDH_DSCP_TO_UP_T;

typedef struct zxdh_up_to_tc_t
{
    ZXIC_UINT32 rsv2                /* : 32; */;
    ZXIC_UINT32 tc                  /* : 3; */;
    ZXIC_UINT32 rsv1                /* : 28; */;
    ZXIC_UINT32 hit_flag            /* : 1;  */;
} ZXDH_UP_TO_TC_T;

typedef struct zxdh_rss_to_vqid_t
{
    ZXIC_UINT32 vqm_qid[8];
    ZXIC_UINT32 hit_flag;
} ZXDH_RSS_TO_VQID_T;

typedef struct zxdh_vlan_filter_t
{
    ZXIC_UINT8 vport_bitmap[15];
    ZXIC_UINT8 rsv : 7;
    ZXIC_UINT8 hit_flag : 1;

} ZXDH_VLAN_FILTER_T;

typedef struct zxdh_lag_t
{
    ZXIC_UINT32 member_bitmap;
    ZXIC_UINT32 rsv2;
    ZXIC_UINT32 hash_factor;
    ZXIC_UINT32 bond_mode;
    ZXIC_UINT32 member_num;
    ZXIC_UINT32 rsv1;
    ZXIC_UINT32 hit_flag;
} ZXDH_LAG_T;

typedef struct zxdh_bc_t
{
    ZXIC_UINT64 bc_bitmap;
    ZXIC_UINT32 rsv2;
    ZXIC_UINT32 rsv1;
    ZXIC_UINT32 hit_flag;
} ZXDH_BC_T;

typedef struct zxdh_promisc_t
{
    ZXIC_UINT64 bitmap;
    ZXIC_UINT32 rsv2;
    ZXIC_UINT32 rsv1;
    ZXIC_UINT32 pf_enable;
    ZXIC_UINT32 hit_flag;
} ZXDH_PROMISC_T;

typedef struct zxdh_vhca_t
{
    ZXIC_UINT32 rsv2;
    ZXIC_UINT32 vqm_vfid;
    ZXIC_UINT32 rsv1;
    ZXIC_UINT32 valid;
} ZXDH_VHCA_T;

typedef struct zxdh_network_attr_t
{
    ZXIC_UINT32 rsv;
    ZXIC_UINT32 upf;
    ZXIC_UINT32 sdn_dyn_sriov_cni;
    ZXIC_UINT32 three_plane_aggr;
    ZXIC_UINT32 single_pipe;
    ZXIC_UINT32 hit_flag;
} ZXDH_NETWORK_ATTR_T;

typedef struct ovs_attr_para_t
{
    ZXIC_UINT32 rsv1;
    ZXIC_UINT32 uplink_vqm_vfid;
    ZXIC_UINT32 rsv0;
    ZXIC_UINT32 is_passthrough;
}OVS_ATTR_PARA_T;

typedef struct upf_attr_para_t
{
    ZXIC_UINT32 offload_eio_vfw;
    ZXIC_UINT32 offload_raw_vfw;
    ZXIC_UINT32 offload_eion_lb;
    ZXIC_UINT32 offload_raw_lb;
    ZXIC_UINT32 offload_eio;
    ZXIC_UINT32 offload_raw;
    ZXIC_UINT32 normal;
}UPF_ATTR_PARA_T;
typedef struct zxdh_vport_traffic_attr_t
{
    union
    {
        OVS_ATTR_PARA_T ovs_attr;
        UPF_ATTR_PARA_T upf_attr;
    }vport_traffic_attr;
    ZXIC_UINT32 hit_flag;
} ZXDH_VPORT_TRAFFIC_ATTR_T;

typedef struct zxdh_vqm_vfid_vlan_t
{
    ZXIC_UINT32 sriov_vlan_tci;
    ZXIC_UINT32 sriov_vlan_tpid;
    ZXIC_UINT32 sriov_business_vlan_tpid;
    ZXIC_UINT32 rsv;
    ZXIC_UINT32 sriov_business_vlan_strip_offload;
    ZXIC_UINT32 sriov_business_qinq_vlan_strip_offload;
    ZXIC_UINT32 sriov_business_vlan_filter;
    ZXIC_UINT32 hit_flag;
} ZXDH_VQM_VFID_VLAN_T;

typedef struct zxdh_fd_index_mng_t
{
    ZXIC_UINT32 vport;
    ZXIC_UINT32 rsv;
    ZXIC_UINT32 hit_flag;
}ZXDH_FD_INDEX_MNG_T;

typedef struct zxdh_pkt_cap_kw_mode_t
{
    ZXIC_UINT64 rule2_key_word_off : 13;
    ZXIC_UINT64 rsv4 : 3;
    ZXIC_UINT64 rule2_key_word_len : 4;
    ZXIC_UINT64 rsv3 : 12;
    ZXIC_UINT64 rule1_key_word_off :13;
    ZXIC_UINT64 rsv2 : 3;
    ZXIC_UINT64 rule1_key_word_len : 4;
    ZXIC_UINT64 rsv1 : 11;
    ZXIC_UINT64 hit_flag : 1;
} ZXDH_PKT_CAP_KW_MODE_T;

typedef struct zxdh_stat_attr_t
{
    ZXIC_UINT32 valid;
    ZXIC_UINT32 mode;
    ZXIC_UINT32 addr_offset;
    ZXIC_UINT32 depth;
} ZXDH_STAT_ATTR_T;

/*************eram call back ****************/
ZXIC_UINT32 dpp_apt_set_vxlan_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);
ZXIC_UINT32 dpp_apt_get_vxlan_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);

ZXIC_UINT32 dpp_apt_set_vport_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);
ZXIC_UINT32 dpp_apt_get_vport_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);

ZXIC_UINT32 dpp_apt_set_uplink_phy_port_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);
ZXIC_UINT32 dpp_apt_get_uplink_phy_port_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);

ZXIC_UINT32 dpp_apt_set_dscp_to_up_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);
ZXIC_UINT32 dpp_apt_get_dscp_to_up_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);

ZXIC_UINT32 dpp_apt_set_up_to_tc_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);
ZXIC_UINT32 dpp_apt_get_up_to_tc_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);

ZXIC_UINT32 dpp_apt_set_rss_to_vqid_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);
ZXIC_UINT32 dpp_apt_get_rss_to_vqid_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);

ZXIC_UINT32 dpp_apt_set_vlan_filter_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);
ZXIC_UINT32 dpp_apt_get_vlan_filter_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);

ZXIC_UINT32 dpp_apt_set_lag_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);
ZXIC_UINT32 dpp_apt_get_lag_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);

ZXIC_UINT32 dpp_apt_set_bc_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);
ZXIC_UINT32 dpp_apt_get_bc_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);

ZXIC_UINT32 dpp_apt_set_promisc_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);
ZXIC_UINT32 dpp_apt_get_promisc_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);

ZXIC_UINT32 dpp_apt_set_vhca_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);
ZXIC_UINT32 dpp_apt_get_vhca_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);

ZXIC_UINT32 dpp_apt_set_network_attr_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);
ZXIC_UINT32 dpp_apt_get_network_attr_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);

ZXIC_UINT32 dpp_apt_set_vport_traffic_attr_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);
ZXIC_UINT32 dpp_apt_get_vport_traffic_attr_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);

ZXIC_UINT32 dpp_apt_set_vqm_vfid_vlan_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);
ZXIC_UINT32 dpp_apt_get_vqm_vfid_vlan_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);

ZXIC_UINT32 dpp_apt_set_fd_index_mng(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);
ZXIC_UINT32 dpp_apt_get_fd_index_mng(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);

ZXIC_UINT32 dpp_apt_set_cap_keyword_attr_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);
ZXIC_UINT32 dpp_apt_get_cap_keyword_attr_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);

ZXIC_UINT32 dpp_apt_set_stat_attr_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);
ZXIC_UINT32 dpp_apt_get_stat_attr_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4]);

SE_APT_ERAM_CONVERT_T *se_eram_callback_get(ZXIC_UINT32 sdt_no);

#endif
