/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_tbl_diag.h
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

#ifndef DPP_TBL_DIAG_H
#define DPP_TBL_DIAG_H

#include "zxic_common.h"
#include "dpp_type_api.h"
#include "dpp_tbl_pkt_cap.h"

ZXIC_CONST ZXIC_CHAR* dpp_vport_table_attr_name_get(ZXIC_UINT32 attr);
ZXIC_CONST ZXIC_CHAR* dpp_uplink_phy_port_table_attr_name_get(ZXIC_UINT32 attr);
ZXIC_CONST ZXIC_CHAR* dpp_vqm_vfid_vlan_attr_name_get(ZXIC_UINT32 attr);

ZXIC_UINT32 diag_dpp_sdt_tbl_prt(ZXIC_UINT32 sdt_no);
ZXIC_UINT32 diag_dpp_se_smmu0_wr64(ZXIC_UINT16 slot, ZXIC_UINT16 vport,
                                   ZXIC_UINT32 base_addr,
                                   ZXIC_UINT32 index,
                                   ZXIC_UINT32 data0,
                                   ZXIC_UINT32 data1);
ZXIC_UINT32 diag_dpp_se_smmu0_rd64(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 base_addr, ZXIC_UINT32 index);
ZXIC_UINT32 diag_dpp_se_smmu0_wr128(ZXIC_UINT16 slot, ZXIC_UINT16 vport, 
                                    ZXIC_UINT32 base_addr,
                                    ZXIC_UINT32 index,
                                    ZXIC_UINT32 data0,
                                    ZXIC_UINT32 data1,
                                    ZXIC_UINT32 data2,
                                    ZXIC_UINT32 data3);
ZXIC_UINT32 diag_dpp_se_smmu0_rd128(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 base_addr, ZXIC_UINT32 index);
ZXIC_UINT32 diag_dpp_vport_mac_add(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT16 sriov_vlan_tpid, ZXIC_UINT16 sriov_vlan_id,
                                   ZXIC_UINT8 mac0, ZXIC_UINT8 mac1, ZXIC_UINT8 mac2,
                                   ZXIC_UINT8 mac3, ZXIC_UINT8 mac4, ZXIC_UINT8 mac5);
ZXIC_UINT32 diag_dpp_vport_mac_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT16 sriov_vlan_tpid, ZXIC_UINT16 sriov_vlan_id,
                                   ZXIC_UINT8 mac0, ZXIC_UINT8 mac1, ZXIC_UINT8 mac2,
                                   ZXIC_UINT8 mac3, ZXIC_UINT8 mac4, ZXIC_UINT8 mac5);
ZXIC_UINT32 diag_dpp_vport_batch_mac_add(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT16 mac_num,
                                   ZXIC_UINT32 vlan_id, ZXIC_UINT16 mac16, ZXIC_UINT32 mac32);
ZXIC_UINT32 diag_dpp_vport_batch_mac_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT16 mac_num,
                                   ZXIC_UINT32 vlan_id, ZXIC_UINT16 mac16, ZXIC_UINT32 mac32);
ZXIC_UINT32 diag_dpp_vport_mac_transter(ZXIC_UINT16 slot, ZXIC_UINT16 vport,ZXIC_UINT16 new_vport);
ZXIC_UINT32 diag_dpp_vport_mac_max_num(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_vport_mac_flush_online(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_vport_mac_flush_offline(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_vport_mac_search(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT16 sriov_vlan_tpid, ZXIC_UINT16 sriov_vlan_id,
                                   ZXIC_UINT8 mac0, ZXIC_UINT8 mac1, ZXIC_UINT8 mac2,
                                   ZXIC_UINT8 mac3, ZXIC_UINT8 mac4, ZXIC_UINT8 mac5);
ZXIC_UINT32 diag_dpp_vport_mac_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_vport_mc_mac_add(ZXIC_UINT16 slot, ZXIC_UINT16 vport,
                                   ZXIC_UINT8 mac0, ZXIC_UINT8 mac1, ZXIC_UINT8 mac2,
                                   ZXIC_UINT8 mac3, ZXIC_UINT8 mac4, ZXIC_UINT8 mac5);
ZXIC_UINT32 diag_dpp_vport_mc_mac_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport,
                                   ZXIC_UINT8 mac0, ZXIC_UINT8 mac1, ZXIC_UINT8 mac2,
                                   ZXIC_UINT8 mac3, ZXIC_UINT8 mac4, ZXIC_UINT8 mac5);
ZXIC_UINT32 diag_dpp_vport_batch_mc_mac_add(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT16 mac_num,
                                   ZXIC_UINT8 mac0, ZXIC_UINT8 mac1, ZXIC_UINT8 mac2,
                                   ZXIC_UINT8 mac3, ZXIC_UINT8 mac4, ZXIC_UINT8 mac5);
ZXIC_UINT32 diag_dpp_vport_batch_mc_mac_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT16 mac_num,
                                   ZXIC_UINT8 mac0, ZXIC_UINT8 mac1, ZXIC_UINT8 mac2,
                                   ZXIC_UINT8 mac3, ZXIC_UINT8 mac4, ZXIC_UINT8 mac5);
ZXIC_UINT32 diag_dpp_vport_mc_mac_transter(ZXIC_UINT16 slot, ZXIC_UINT16 vport,ZXIC_UINT16 new_vport);
ZXIC_UINT32 diag_dpp_vport_mc_mac_max_num(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_vport_mc_mac_flush_online(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_vport_mc_mac_flush_offline(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_vport_mc_mac_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_vport_table_init(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_vport_table_delete(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_vport_table_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 attr,
                                                                         ZXIC_UINT32 value);
ZXIC_UINT32 diag_dpp_vport_table_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport);

ZXIC_UINT32 diag_dpp_vport_egress_meter_en_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 enable);
ZXIC_UINT32 diag_dpp_vport_egress_meter_en_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_vport_ingress_meter_en_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 enable);
ZXIC_UINT32 diag_dpp_vport_ingress_meter_en_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_vport_egress_meter_mode_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_vport_egress_meter_mode_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_vport_ingress_meter_mode_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_vport_ingress_meter_mode_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport);

ZXIC_UINT32 diag_dpp_vport_rx_flow_hash_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 hash_mode);
ZXIC_UINT32 diag_dpp_vport_rx_flow_hash_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_vport_hash_index_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_vport_hash_funcs_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 funcs);
ZXIC_UINT32 diag_dpp_vport_rss_en_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 enable);
ZXIC_UINT32 diag_dpp_vport_virtio_en_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 enable);
ZXIC_UINT32 diag_dpp_vport_virtio_version_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 version);
ZXIC_UINT32 diag_dpp_vport_promisc_en_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 enable);
ZXIC_UINT32 diag_dpp_vport_business_vlan_offload_en_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 enable);
ZXIC_UINT32 diag_dpp_vport_vlan_offload_en_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 enable);

ZXIC_UINT32 diag_dpp_uplink_phy_port_table_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id,
                                                                ZXIC_UINT32 attr, ZXIC_UINT32 value);
ZXIC_UINT32 diag_dpp_uplink_phy_port_table_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id);
ZXIC_UINT32 diag_dpp_uplink_phy_bond_vport(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id);
ZXIC_UINT32 diag_dpp_uplink_phy_hardware_bond_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id, ZXIC_UINT8 enable);
ZXIC_UINT32 diag_dpp_uplink_phy_lacp_pf_vqm_vfid_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id, ZXIC_UINT16 vqm_vfid);
ZXIC_UINT32 diag_dpp_uplink_phy_lacp_pf_memport_qid_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id, ZXIC_UINT16 qid);
ZXIC_UINT32 diag_dpp_ptp_port_vfid_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 ptp_port_vfid);
ZXIC_UINT32 diag_dpp_ptp_tc_enable_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 ptp_tc_enable);
ZXIC_UINT32 diag_dpp_tm_flowid_pport_table_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id, ZXIC_UINT32 flow_id);
ZXIC_UINT32 diag_dpp_tm_flowid_pport_table_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id);
ZXIC_UINT32 diag_dpp_tm_pport_trust_mode_table_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_tm_pport_trust_mode_table_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id);
ZXIC_UINT32 diag_dpp_tm_pport_mcode_switch_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_tm_pport_mcode_switch_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id);


ZXIC_UINT32 diag_dpp_vport_bc_table_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 enable);
ZXIC_UINT32 diag_dpp_vport_bc_table_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_vport_uc_promisc_table_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 enable);
ZXIC_UINT32 diag_dpp_vport_uc_promisc_table_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_vport_mc_promisc_table_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 enable);
ZXIC_UINT32 diag_dpp_vport_mc_promisc_table_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_rdma_trans_item_add(ZXIC_UINT16 slot, ZXIC_UINT16 vport,
                                   ZXIC_UINT8 mac0, ZXIC_UINT8 mac1, ZXIC_UINT8 mac2,
                                   ZXIC_UINT8 mac3, ZXIC_UINT8 mac4, ZXIC_UINT8 mac5, ZXIC_UINT16 vhcaId);
ZXIC_UINT32 diag_dpp_rdma_trans_item_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport,
                                   ZXIC_UINT8 mac0, ZXIC_UINT8 mac1, ZXIC_UINT8 mac2,
                                   ZXIC_UINT8 mac3, ZXIC_UINT8 mac4, ZXIC_UINT8 mac5);
DPP_STATUS diag_dpp_pcie_channel_prt(ZXIC_VOID);
DPP_STATUS diag_dpp_se_hash_stat_prt(ZXIC_UINT32 slot_id, ZXIC_UINT32 fun_id);
DPP_STATUS diag_dpp_se_hash_stat_clr(ZXIC_UINT32 slot_id, ZXIC_UINT32 fun_id);
DPP_STATUS diag_dpp_hash_item_prt(ZXIC_UINT32 slot, ZXIC_UINT32 sdt_no);
ZXIC_UINT32 diag_dpp_vqm_vfid_vlan_init(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_vqm_vfid_vlan_delete(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_vqm_vfid_vlan_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 attr, ZXIC_UINT32 value);
ZXIC_UINT32 diag_dpp_vqm_vfid_vlan_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_rxfh_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport,
                                                ZXIC_UINT32 qid0, ZXIC_UINT32 qid1,
                                                ZXIC_UINT32 qid2, ZXIC_UINT32 qid3, ZXIC_UINT32 qnum);
ZXIC_UINT32 diag_dpp_rxfh_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_rxfh_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_thash_key_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport,
                                   ZXIC_UINT8 key0, ZXIC_UINT8 key1,
                                   ZXIC_UINT8 key2, ZXIC_UINT8 key3, ZXIC_UINT32 knum);
ZXIC_UINT32 diag_dpp_thash_key_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport);

ZXIC_UINT32 diag_dpp_vport_register_info_prt(ZXIC_VOID);

ZXIC_UINT32 diag_dpp_stat_mc_packet_rx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_bc_packet_rx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_1588_packet_rx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_1588_packet_tx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_1588_packet_drop_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_1588_enc_packet_rx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_1588_enc_packet_tx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_spoof_packet_drop_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_mcode_packet_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_port_RDMA_packet_msg_tx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_port_RDMA_packet_msg_rx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_plcr_packet_drop_tx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_plcr_packet_drop_rx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_MTU_packet_msg_tx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_MTU_packet_msg_rx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_port_uc_packet_rx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_port_uc_packet_tx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_port_mc_packet_rx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_port_mc_packet_tx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_port_bc_packet_rx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_port_bc_packet_tx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_asn_phyport_rx_pkt_cnt_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_psn_phyport_tx_pkt_cnt_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_psn_phyport_rx_pkt_cnt_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_psn_ack_phyport_tx_pkt_cnt_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);
ZXIC_UINT32 diag_dpp_stat_psn_ack_phyport_rx_pkt_cnt_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode);

ZXIC_UINT32 diag_dpp_lag_group_create(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 lag_id);
ZXIC_UINT32 diag_dpp_lag_group_delete(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 lag_id);
ZXIC_UINT32 diag_dpp_lag_mode_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 lag_id, ZXIC_UINT8 mode);
ZXIC_UINT32 diag_dpp_lag_group_hash_factor_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 lag_id, ZXIC_UINT8 factor);
ZXIC_UINT32 diag_dpp_lag_group_member_add(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 lag_id, ZXIC_UINT8 uplink_phy_port_id);
ZXIC_UINT32 diag_dpp_lag_group_member_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 lag_id, ZXIC_UINT8 uplink_phy_port_id);
ZXIC_UINT32 diag_dpp_lag_table_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 lag_id);
ZXIC_UINT32 diag_dpp_tm_pport_dscp_map_table_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 port, ZXIC_UINT32 dscp_id, ZXIC_UINT32 up_id);
ZXIC_UINT32 diag_dpp_tm_pport_dscp_map_table_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 port, ZXIC_UINT32 dscp_id);
ZXIC_UINT32 diag_dpp_tm_pport_dscp_map_table_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 port, ZXIC_UINT32 dscp_id);
ZXIC_UINT32 diag_dpp_tm_pport_up_map_table_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 port, ZXIC_UINT32 up_id, ZXIC_UINT32 tc_id);
ZXIC_UINT32 diag_dpp_tm_pport_up_map_table_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 port, ZXIC_UINT32 up_id);
ZXIC_UINT32 diag_dpp_tm_pport_up_map_table_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 port, ZXIC_UINT32 up_id);
ZXIC_UINT32 diag_dpp_vport_vhca_id_add(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 vhca_id);
ZXIC_UINT32 diag_dpp_vport_vhca_id_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 vhca_id);
ZXIC_UINT32 diag_dpp_vport_vhca_id_table_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 vhca_id);
ZXIC_UINT32 diag_dpp_vport_reset(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_vlan_filter_init(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_add_vlan_filter(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT16 vlan_id);
ZXIC_UINT32 diag_dpp_del_vlan_filter(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT16 vlan_id);
ZXIC_UINT32 diag_dpp_vlan_filter_table_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 vlan_group_id);
ZXIC_VOID diag_dpp_fd_cfg_pre1(ZXIC_UINT32 smac, ZXIC_UINT32 dmac, ZXIC_UINT32 sip,
                                   ZXIC_UINT32 dip, ZXIC_UINT32 sport, ZXIC_UINT32 dport);
ZXIC_VOID diag_dpp_fd_cfg_pre2(ZXIC_UINT32 ethertype, ZXIC_UINT32 cvlan_pri, ZXIC_UINT32 vlan, 
                                      ZXIC_UINT32 vxlan_vni,ZXIC_UINT32 vqm_vfid);
ZXIC_VOID diag_dpp_fd_cfg_pre3(ZXIC_UINT32 action_index, ZXIC_UINT32 action_index2,ZXIC_UINT32 count_id, ZXIC_UINT32 hash_alg, 
                                      ZXIC_UINT32 rss_hash_factor); 
ZXIC_VOID diag_dpp_fd_cfg_pre4(ZXIC_UINT32 uplink_fd_id,ZXIC_UINT32 v_qid);                                        
ZXIC_UINT32 diag_dpp_fd_cfg_add(ZXIC_UINT16 slot, ZXIC_UINT16 vport);   
ZXIC_UINT32 diag_dpp_fd_cfg_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index);  
ZXIC_UINT32 diag_dpp_fd_cfg_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index); 
ZXIC_UINT32 diag_dpp_fd_cfg_search(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index); 
ZXIC_UINT32 diag_dpp_fd_acl_index_req(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_fd_acl_index_rel(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index);
ZXIC_UINT32 diag_dpp_fd_acl_all_delete(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_VOID diag_dpp_acl_glb_data_prt(ZXIC_VOID);
DPP_STATUS diag_dpp_dtb_stat_ppu_cnt_clr(ZXIC_UINT16 slot, 
                                         ZXIC_UINT16 vport, 
                                         ZXIC_UINT32 rd_mode,
                                         ZXIC_UINT32 counter_id,
                                         ZXIC_UINT32 num);
DPP_STATUS diag_dpp_fd_acl_stat_clear(ZXIC_UINT16 slot, 
                                         ZXIC_UINT16 vport);
DPP_STATUS diag_dpp_fd_acl_index_dump(ZXIC_UINT16 slot, 
                                         ZXIC_UINT16 vport);
DPP_STATUS diag_dpp_se_eram_res_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
DPP_STATUS diag_dpp_se_hash_res_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport);  
DPP_STATUS diag_dpp_se_acl_res_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
DPP_STATUS diag_dpp_se_lpm_res_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
DPP_STATUS diag_dpp_se_ddr_res_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
DPP_STATUS diag_dpp_se_stat_res_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport); 
ZXIC_VOID diag_dpp_eram_data_stub(ZXIC_UINT32 data0,ZXIC_UINT32 data1,ZXIC_UINT32 data2,ZXIC_UINT32 data3);
DPP_STATUS diag_dpp_eram_entry_insert(ZXIC_UINT16 slot,ZXIC_UINT16 vport,ZXIC_UINT32 sdt_no,ZXIC_UINT32 index);
DPP_STATUS diag_dpp_eram_entry_delete(ZXIC_UINT16 slot,ZXIC_UINT16 vport,ZXIC_UINT32 sdt_no,ZXIC_UINT32 index);
DPP_STATUS diag_dpp_eram_entry_get(ZXIC_UINT16 slot,ZXIC_UINT16 vport,ZXIC_UINT32 sdt_no,ZXIC_UINT32 index); 
ZXIC_VOID diag_dpp_hash_data_stub(ZXIC_UINT32 data0,ZXIC_UINT32 data1,ZXIC_UINT32 data2,ZXIC_UINT32 data3);
DPP_STATUS diag_dpp_hash_entry_insert(ZXIC_UINT16 slot,ZXIC_UINT16 vport,ZXIC_UINT32 sdt_no);
DPP_STATUS diag_dpp_hash_entry_delete(ZXIC_UINT16 slot,ZXIC_UINT16 vport,ZXIC_UINT32 sdt_no);
DPP_STATUS diag_dpp_hash_entry_get(ZXIC_UINT16 slot,ZXIC_UINT16 vport,ZXIC_UINT32 sdt_no);
DPP_STATUS diag_dpp_hash_entry_flush(ZXIC_UINT16 slot,ZXIC_UINT16 vport,ZXIC_UINT32 sdt_no,ZXIC_UINT32 flush_mode);
DPP_STATUS diag_dpp_hash_entry_num_prt(ZXIC_UINT16 slot,ZXIC_UINT16 vport,ZXIC_UINT32 sdt_no);
DPP_STATUS diag_dpp_hash_entry_soft_del_by_sdt(ZXIC_UINT16 slot,ZXIC_UINT16 vport,ZXIC_UINT32 sdt_no);
DPP_STATUS diag_dpp_stat_item_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT16 stat_item_no);
DPP_STATUS diag_dpp_stat_item_prt_all(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
DPP_STATUS diag_dpp_stat_item_cnt_prt(ZXIC_UINT16 slot,
                                    ZXIC_UINT16 vport, 
                                    ZXIC_UINT32 stat_item_no, 
                                    ZXIC_UINT32 index, 
                                    ZXIC_UINT32 rd_mode);
ZXIC_UINT32 diag_dpp_glb_cfg_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 glb_cfg_data_0, ZXIC_UINT32 glb_cfg_data_1, ZXIC_UINT32 glb_cfg_data_2, ZXIC_UINT32 glb_cfg_data_3);
ZXIC_UINT32 diag_dpp_glb_cfg_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_pkt_capture_enable(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXDH_PKT_CAP_POINT capture_pkt_flag);
ZXIC_UINT32 diag_dpp_pkt_capture_disable(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXDH_PKT_CAP_POINT capture_pkt_flag);
ZXIC_UINT32 diag_dpp_pkt_capture_disable_all(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_pkt_capture_enable_status_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_pkt_capture_rule_index_to_tcam_index(ZXIC_UINT32 rule_index, ZXDH_PKT_CAP_MODE rule_mode, ZXDH_PKT_CAP_POINT capture_pkt_flag);
ZXIC_UINT32 diag_dpp_pkt_capture_tcam_index_to_rule_index(ZXIC_UINT32 tcam_index);
ZXIC_UINT32 diag_dpp_pkt_capture_item_l3_set(ZXIC_UINT32 sip_0, ZXIC_UINT32 sip_1, ZXIC_UINT32 sip_2, ZXIC_UINT32 sip_3, \
                                             ZXIC_UINT32 dip_0, ZXIC_UINT32 dip_1, ZXIC_UINT32 dip_2, ZXIC_UINT32 dip_3, \
                                             ZXIC_UINT8 protocol);
ZXIC_UINT32 diag_dpp_pkt_capture_item_l2_set(ZXIC_UINT16 dmac_0, ZXIC_UINT32 dmac_1, ZXIC_UINT16 smac_0, ZXIC_UINT32 smac_1, \
                                             ZXIC_UINT16 ethtype);
ZXIC_UINT32 diag_dpp_pkt_capture_item_l4_set(ZXIC_UINT16 dport, ZXIC_UINT16 sport, ZXIC_UINT32 qp);
ZXIC_UINT32 diag_dpp_pkt_capture_item_kw_set(ZXIC_UINT32 kw_0, ZXIC_UINT32 kw_1, ZXIC_UINT32 kw_2, ZXIC_UINT32 kw_3, \
                                             ZXIC_UINT16 kw_off, ZXIC_UINT8 kw_len);
ZXIC_UINT32 diag_dpp_pkt_capture_item_insert(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 tcam_index, \
                                             ZXIC_UINT16 rule_config, ZXIC_UINT8 capture_pkt_flag, ZXIC_UINT8 panel_id, \
                                             ZXIC_UINT16 vqm_vfid, ZXIC_UINT16 vhca_id);
ZXIC_UINT32 diag_dpp_pkt_capture_item_delete(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 tcam_index);
ZXIC_UINT32 diag_dpp_pkt_capture_table_dump(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_pkt_capture_table_flush(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_pkt_capture_speed_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 speed);
ZXIC_UINT32 diag_dpp_pkt_capture_speed_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_mcode_feature_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index);
ZXIC_UINT32 diag_dpp_pktrx_mcode_glb_cfg_write(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 start_bit_no, ZXIC_UINT32 end_bit_no,
                                            ZXIC_UINT32 glb_cfg_data_1);
ZXIC_UINT32 diag_dpp_pktrx_mcode_port_cfg_read(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 port_id);
ZXIC_UINT32 diag_dpp_pktrx_mcode_port_cfg_write(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 port_id, 
                                                ZXIC_UINT32 index, ZXIC_UINT32 start_bit_no, ZXIC_UINT32 end_bit_no, ZXIC_UINT32 port_cfg_data);
ZXIC_UINT32 diag_dpp_l2d_psn_cfg_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 psn_cfg);
ZXIC_UINT32 diag_dpp_l2d_psn_cfg_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport);
ZXIC_UINT32 diag_dpp_rdma_trans_pos_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport,
                                        ZXIC_UINT8 mac0, ZXIC_UINT8 mac1, ZXIC_UINT8 mac2,
                                        ZXIC_UINT8 mac3, ZXIC_UINT8 mac4, ZXIC_UINT8 mac5);
ZXIC_UINT32 diag_dpp_dtb_dump_test(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 num,ZXIC_UINT32 flag);
ZXIC_UINT32 diag_dpp_pktrx_udf_icmp_item_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 port);
ZXIC_UINT32 diag_dpp_pktrx_tcam_icmp_item_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 mach, ZXIC_UINT32 macl, ZXIC_UINT32 port);
ZXIC_UINT32 diag_dpp_tbl_pmtu_info_add(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 qp, ZXIC_UINT32 pmtu);
ZXIC_UINT32 diag_dpp_tbl_pmtu_info_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 qp);
ZXIC_UINT32 diag_dpp_tbl_pmtu_info_srh(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 qp);
ZXIC_UINT32 diag_dpp_tbl_qp_info_add_ex(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 qp, ZXIC_UINT32 psn, ZXIC_UINT32 length, ZXIC_UINT32 va_h, ZXIC_UINT32 va_l);
ZXIC_UINT32 diag_dpp_tbl_qp_info_del_ex(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 qp);
ZXIC_UINT32 diag_dpp_np_stat_info_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 phy_port);
ZXIC_UINT32 diag_dpp_stat_idma_phyport_prio_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 mode, ZXIC_UINT32 phy_port);
ZXIC_UINT32 diag_dpp_stat_odma_phyport_prio_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 mode, ZXIC_UINT32 phy_port);

ZXIC_UINT32 diag_dpp_std_nic_stat_unlock_cnt_prt(ZXIC_UINT32 slot);
ZXIC_UINT32 diag_dpp_acl_index_max_num_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no);
ZXIC_UINT32 diag_dpp_acl_index_unused_num(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no);
ZXIC_UINT32 diag_dpp_acl_index_request(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no);
ZXIC_UINT32 diag_dpp_acl_index_release(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no, ZXIC_UINT32 index);
ZXIC_UINT32 diag_dpp_acl_index_dump(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no);

ZXIC_UINT32 diag_dpp_acl_entry_add(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no,ZXIC_UINT32 entry_num);
ZXIC_UINT32 diag_dpp_acl_entry_del_by_key(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no,ZXIC_UINT32 entry_num);
ZXIC_UINT32 diag_dpp_acl_entry_search_by_key(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no,ZXIC_UINT32 entry_num);
ZXIC_UINT32 diag_dpp_acl_entry_del_by_index(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no,ZXIC_UINT32 entry_num);
ZXIC_UINT32 diag_dpp_acl_entry_search_by_index(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no,ZXIC_UINT32 entry_num);
ZXIC_UINT32 diag_dpp_acl_entry_dump_by_vport(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no);
ZXIC_UINT32 diag_dpp_acl_entry_flush_by_vport(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no);
ZXIC_UINT32 diag_dpp_acl_entry_dump_all(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no);
ZXIC_UINT32 diag_dpp_acl_entry_flush_all(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no);

ZXIC_UINT32 diag_dpp_acl_entry_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no,ZXIC_UINT32 handle);
ZXIC_UINT32 diag_dpp_acl_entry_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no,ZXIC_UINT32 handle);
ZXIC_UINT32 diag_dpp_roce_match_add(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 ip, ZXIC_UINT32 vqm_vfid);
ZXIC_UINT32 diag_dpp_roce_match_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 ip);
ZXIC_UINT32 diag_dpp_roce_match_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 ip);
#endif
