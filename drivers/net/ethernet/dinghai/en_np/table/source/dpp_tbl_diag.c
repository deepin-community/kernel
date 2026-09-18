#include "dpp_tbl_api.h"
#include "dpp_tbl_comm.h"
#include "dpp_dtb.h"
#include "dpp_drv_sdt.h"
#include "dpp_ppu_api.h"
#include "dpp_ppu.h"
#include "dpp_sdt.h"
#include "dpp_hash.h"
#include "dpp_se_api.h"
#include "dpp_apt_se.h"
#include "dpp_drv_hash.h"
#include "dpp_tbl_plcr.h"
#include "dpp_tbl_tm.h"
#include "dpp_tbl_vlan.h"
#include "dpp_drv_acl.h"
#include "dpp_dtb_table_api.h"
#include "dpp_tbl_cfg.h"
#include "dpp_tbl_pkt_cap.h"
#include "dpp_np_init.h"
#include "dpp_tbl_stat.h"
#include "dpp_tbl_fd_cfg.h"
#include "dpp_tbl_pmtu_info.h"
#include "dpp_tbl_qp_info_ex.h"
#include "dpp_tbl_roce_match.h"

static ZXDH_FD_CFG_T g_diag_fd_cfg = {0};
static ZXIC_UINT32 g_diag_fd_index = 0;
static ZXIC_UINT32 g_eram_buff[4] = {0};
static ZXIC_UINT8 g_hash_key_buff[HASH_KEY_MAX] = {0};
static ZXIC_UINT8 g_hash_rst_buff[HASH_RST_MAX] = {0};
extern DPP_DEV_MGR_T *dpp_dev_mgr_get(ZXIC_VOID);
extern DPP_SE_CFG *dpp_apt_get_se_cfg(DPP_DEV_T *dev);
ZXIC_CONST ZXIC_CHAR* g_vport_table_attr_name[] = {
    // byte[15:16]
     "rsv6",
    // byte[13:14]
     "vhca",
     "rsv5",
    // byte[12]
     "rss_hash_factor",
    // byte[11]
     "hash_alg",
     "uplink_phy_port_id",
    // byte[9:10]
     "lag_id",
     "vxlan_offload_en",
     "pf_vqm_vfid", 
     "rsv3",
    // byte[7:8]
     "mtu",
    // byte[5:6]
     "port_base_qid",
     "hash_search_index",
     "rsv2",
    // byte[4]
     "np_egress_meter_enable",
     "np_ingress_meter_enable",
     "np_egress_meter_mode",
     "np_ingress_meter_mode",
     "np_egress_tm_enable",
     "np_ingress_tm_enable",
     "rsv1",
     "spoof_check_enable",
    // byte[3]
     "inline_sec_offload",
     "fd_enable",
     "lag_enable",
     "vepa_enable",
     "is_vf",
     "virtio_version",
     "virtio_enable",
    // byte[2]
     "accelerator_offload_flag",
     "lro_offload",
     "ip_recombine_offload",
     "tcp_udp_checksum_offload",
     "ip_checksum_offload",
     "outer_ip_checksum_offload",
     "is_up",
     "business_enable",
    // byte[1]
     "hw_bond_enable",
     "rdma_offload_enable",
     "promisc_enable",
     "sriov_vlan_offload_enable",
     "sriov_business_vlan_offload_enable",
     "rss_enable",
     "mtu_offload_enable",
     "hit_flag",

     // byte[13:14]
     "flag_1588_enable",
     "k8s_cni_flag",
     "roce_overlay_enable",

     //byte[15]
     "multi_host_group_id",
     "multi_host_enable",

     //byte[12]
     "frag_pkt_use_ipid",

     // byte[13:14]
     "outer_l4_checksum_offload",
     // byte[9:10]
     "multi_plane_en",
};

ZXIC_CONST ZXIC_CHAR* g_uplink_phy_port_table_attr_name[] = {
    "rsv6","pf_vqm_vfid","rsv5","lacp_pf_memport_qid","rsv4","lacp_pf_vqm_vfid","rsv3","is_up","bond_link_up","hw_bond_enable",
    "mtu","mtu_offload_enable","rsv2","tm_base_queue","ptp_port_vfid","rsv1","magic_packet_enable","tm_shape_enable","ptp_tc_enable",
    "trust_mode","hit_flag","primary_pf_vqm_vfid","sriov_hdbond_enable",
    "multi_host_group_id",
    "multi_host_enable",
    "roce_overlay_enable"
};

ZXIC_CONST ZXIC_CHAR* g_vqm_vfid_vlan_attr_name[] = {
    "sriov_vlan_tci","sriov_vlan_tpid","sriov_business_vlan_tpid","rsv","sriov_business_vlan_strip_offload",
    "sriov_business_qinq_vlan_strip_offload","sriov_business_vlan_filter","hit_flag"
};

ZXIC_CONST ZXIC_CHAR* dpp_vport_table_attr_name_get(ZXIC_UINT32 attr)
{
    if (attr >= (sizeof(g_vport_table_attr_name) / sizeof(ZXIC_CHAR*)))
    {
        return NULL;
    }

    return g_vport_table_attr_name[attr];
}

ZXIC_CONST ZXIC_CHAR* dpp_uplink_phy_port_table_attr_name_get(ZXIC_UINT32 attr)
{
    if (attr >= (sizeof(g_uplink_phy_port_table_attr_name) / sizeof(ZXIC_CHAR*)))
    {
        return NULL;
    }

    return g_uplink_phy_port_table_attr_name[attr];
}

ZXIC_CONST ZXIC_CHAR* dpp_vqm_vfid_vlan_attr_name_get(ZXIC_UINT32 attr)
{
    if (attr >= (sizeof(g_vqm_vfid_vlan_attr_name) / sizeof(ZXIC_CHAR*)))
    {
        return NULL;
    }

    return g_vqm_vfid_vlan_attr_name[attr];
}

ZXIC_UINT32 diag_dpp_sdt_tbl_prt(ZXIC_UINT32 sdt_no)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 tbl_type = 0;
    ZXIC_UINT32 slot = 0;

    DPP_SDT_TBL_DATA_T sdt_tbl = {0};
    DPP_SDTTBL_ERAM_T sdt_eram = {0};
    DPP_SDTTBL_HASH_T sdt_hash = {0};
    DPP_SDTTBL_ETCAM_T sdt_etcam = {0};
    DPP_SDTTBL_PORTTBL_T sdt_porttbl = {0};
    DPP_DEV_T dev = {0}; 

    if (sdt_no > PPU_SDT_IDX_MAX)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "sdt_no[%d] error, please check it.\n", sdt_no);
        return DPP_ERR;
    }

    for(slot = 0;slot<DPP_PCIE_SLOT_MAX;slot++)
    {
        dev.pcie_channel.slot = slot;
        dev.device_id = 0;
        rc = dpp_sdt_tbl_data_get(&dev, sdt_no, &sdt_tbl);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_sdt_tbl_data_get");

        if ((sdt_tbl.data_low32 == 0xFFFFFFFF) && (sdt_tbl.data_high32 == 0xFFFFFFFF))
        {
            continue;
        }

        ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "slot", slot);
        ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "sdt_no", sdt_no);
        ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "data_high32", sdt_tbl.data_high32);
        ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "data_low32",  sdt_tbl.data_low32);
        ZXIC_COMM_PRINT("\n");

        /* 获取sdt 类型信息 */
        ZXIC_COMM_UINT32_GET_BITS(tbl_type, sdt_tbl.data_high32, DPP_SDT_H_TBL_TYPE_BT_POS, DPP_SDT_H_TBL_TYPE_BT_LEN);

        if (tbl_type >= DPP_SDT_TBLT_eRAM && tbl_type <= DPP_SDT_TBLT_PORTTBL)
        {
            switch (tbl_type)
            {
                case DPP_SDT_TBLT_eRAM:
                {
                    rc = dpp_soft_sdt_tbl_get(&dev, sdt_no, &sdt_eram);
                    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_soft_sdt_tbl_get");

                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "eram_mode",          sdt_eram.eram_mode          );
                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "eram_base_addr",     sdt_eram.eram_base_addr     );
                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "eram_table_depth",   sdt_eram.eram_table_depth   );
                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "eram_clutch_en",     sdt_eram.eram_clutch_en     );
                    break;
                }

                case DPP_SDT_TBLT_HASH:
                {
                    rc = dpp_soft_sdt_tbl_get(&dev, sdt_no, &sdt_hash);
                    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_soft_sdt_tbl_get");

                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "hash_id",            sdt_hash.hash_id            );
                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "hash_table_width",   sdt_hash.hash_table_width   );
                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "key_size",           sdt_hash.key_size           );
                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "hash_table_id",      sdt_hash.hash_table_id      );
                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "learn_en",           sdt_hash.learn_en           );
                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "keep_alive",         sdt_hash.keep_alive         );
                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "keep_alive_baddr",   sdt_hash.keep_alive_baddr   );
                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "rsp_mode",           sdt_hash.rsp_mode           );
                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "hash_clutch_en",     sdt_hash.hash_clutch_en     );
                    break;
                }

                case DPP_SDT_TBLT_eTCAM:
                {
                    rc = dpp_soft_sdt_tbl_get(&dev, sdt_no, &sdt_etcam);
                    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_soft_sdt_tbl_get");

                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "etcam_id",           sdt_etcam.etcam_id          );
                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "etcam_key_mode",     sdt_etcam.etcam_key_mode    );
                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "etcam_table_id",     sdt_etcam.etcam_table_id    );
                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "no_as_rsp_mode",     sdt_etcam.no_as_rsp_mode    );
                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "as_en",              sdt_etcam.as_en             );
                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "as_eram_baddr",      sdt_etcam.as_eram_baddr     );
                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "as_rsp_mode",        sdt_etcam.as_rsp_mode       );
                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "etcam_table_depth",  sdt_etcam.etcam_table_depth );
                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "etcam_clutch_en",    sdt_etcam.etcam_clutch_en   );
                    break;
                }

                case DPP_SDT_TBLT_PORTTBL:
                {
                    rc = dpp_soft_sdt_tbl_get(&dev, sdt_no, &sdt_porttbl);
                    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_soft_sdt_tbl_get");

                    ZXIC_COMM_PRINT("%-30s : 0x%08x\n", "porttbl_clutch_en",  sdt_porttbl.porttbl_clutch_en);
                    break;
                }

                default:
                {
                    ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "SDT table_type[ %d ] is invalid!\n", tbl_type);
                    return DPP_ERR;
                }
            }

            ZXIC_COMM_PRINT("\n");
        }
        else
        {
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "no sdt information \n");
        }
    }

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_se_smmu0_wr64(ZXIC_UINT16 slot, ZXIC_UINT16 vport,
                                   ZXIC_UINT32 base_addr,
                                   ZXIC_UINT32 index,
                                   ZXIC_UINT32 data0,
                                   ZXIC_UINT32 data1)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    DPP_DEV_T dev = {0};

    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_UINT32 buff[2] = {0};


    ZXIC_COMM_CHECK_INDEX(base_addr, 0, SE_SMMU0_ERAM_ADDR_NUM_TOTAL - 1);
    ZXIC_COMM_CHECK_INDEX(data0, 0, 0xffffffff);
    ZXIC_COMM_CHECK_INDEX(data1, 0, 0xffffffff);

    buff[0] = data0;
    buff[1] = data1;

    rc = dpp_dev_get(&pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_se_smmu0_ind_write(&dev,
                                base_addr,
                                index,
                                ERAM128_OPR_64b,
                                buff);
    ZXIC_COMM_CHECK_RC(rc, "dpp_se_smmu0_ind_write");

    return rc;
}

ZXIC_UINT32 diag_dpp_se_smmu0_rd64(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 base_addr, ZXIC_UINT32 index)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    DPP_DEV_T dev = {0};

    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_UINT32 buff[2] = {0};

    rc = dpp_dev_get(&pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX(base_addr, 0, SE_SMMU0_ERAM_ADDR_NUM_TOTAL - 1);

    rc = dpp_se_smmu0_ind_read(&dev,
                               base_addr,
                               index,
                               ERAM128_OPR_64b,
                               RD_MODE_HOLD,
                               buff);
    ZXIC_COMM_CHECK_RC(rc, "dpp_se_smmu0_ind_read");

    ZXIC_COMM_PRINT("base_addr[0x%08x] index[0x%08x] value[0x%08x 0x%08x] \n", base_addr, index, buff[0], buff[1]);

    return rc;
}

ZXIC_UINT32 diag_dpp_se_smmu0_wr128(ZXIC_UINT16 slot, ZXIC_UINT16 vport, 
                                    ZXIC_UINT32 base_addr,
                                    ZXIC_UINT32 index,
                                    ZXIC_UINT32 data0,
                                    ZXIC_UINT32 data1,
                                    ZXIC_UINT32 data2,
                                    ZXIC_UINT32 data3)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    DPP_DEV_T dev = {0};

    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_UINT32 buff[4] = {0};

    rc = dpp_dev_get(&pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX(base_addr, 0, SE_SMMU0_ERAM_ADDR_NUM_TOTAL - 1);
    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(base_addr, index);
    ZXIC_COMM_CHECK_INDEX(base_addr + index, 0, SE_SMMU0_ERAM_ADDR_NUM_TOTAL - 1);
    ZXIC_COMM_CHECK_INDEX(data0, 0, 0xffffffff);
    ZXIC_COMM_CHECK_INDEX(data1, 0, 0xffffffff);
    ZXIC_COMM_CHECK_INDEX(data2, 0, 0xffffffff);
    ZXIC_COMM_CHECK_INDEX(data3, 0, 0xffffffff);

    buff[0] = data0;
    buff[1] = data1;
    buff[2] = data2;
    buff[3] = data3;

    rc = dpp_se_smmu0_ind_write(&dev,
                                base_addr,
                                index,
                                ERAM128_OPR_128b,
                                buff);
    ZXIC_COMM_CHECK_RC(rc, "dpp_se_smmu0_ind_write");

    return rc;
}

ZXIC_UINT32 diag_dpp_se_smmu0_rd128(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 base_addr, ZXIC_UINT32 index)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    DPP_DEV_T dev = {0};

    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 buff[4] = {0};

    rc = dpp_dev_get(&pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX(base_addr, 0, SE_SMMU0_ERAM_ADDR_NUM_TOTAL - 1);
    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(base_addr, index);
    ZXIC_COMM_CHECK_INDEX(base_addr + index, 0, SE_SMMU0_ERAM_ADDR_NUM_TOTAL - 1);

    rc = dpp_se_smmu0_ind_read(&dev,
                               base_addr,
                               index,
                               ERAM128_OPR_128b,
                               RD_MODE_HOLD,
                               buff);
    ZXIC_COMM_CHECK_RC(rc, "dpp_se_smmu0_ind_read");

    ZXIC_COMM_PRINT("base_addr[0x%08x] index[0x%08x] value[0x%08x 0x%08x 0x%08x 0x%08x] \n", base_addr, index, buff[0], buff[1], buff[2], buff[3]);

    return rc;
}

ZXIC_UINT32 diag_dpp_vport_mac_add(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT16 sriov_vlan_tpid, ZXIC_UINT16 sriov_vlan_id,
                                   ZXIC_UINT8 mac0, ZXIC_UINT8 mac1, ZXIC_UINT8 mac2,
                                   ZXIC_UINT8 mac3, ZXIC_UINT8 mac4, ZXIC_UINT8 mac5)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT8  mac[6];
    ZXIC_UINT32 rc = DPP_OK;

    mac[0] = mac0;
    mac[1] = mac1;
    mac[2] = mac2;
    mac[3] = mac3;
    mac[4] = mac4;
    mac[5] = mac5;

    rc = dpp_add_mac(&pf_info, mac, sriov_vlan_tpid, sriov_vlan_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_add_mac");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_mac_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT16 sriov_vlan_tpid, ZXIC_UINT16 sriov_vlan_id,
                                   ZXIC_UINT8 mac0, ZXIC_UINT8 mac1, ZXIC_UINT8 mac2,
                                   ZXIC_UINT8 mac3, ZXIC_UINT8 mac4, ZXIC_UINT8 mac5)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT8  mac[6];
    ZXIC_UINT32 rc = DPP_OK;

    mac[0] = mac0;
    mac[1] = mac1;
    mac[2] = mac2;
    mac[3] = mac3;
    mac[4] = mac4;
    mac[5] = mac5;

    rc = dpp_del_mac(&pf_info, mac, sriov_vlan_tpid, sriov_vlan_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_del_mac");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_batch_mac_add(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT16 mac_num,
                                   ZXIC_UINT32 vlan_id, ZXIC_UINT16 mac16, ZXIC_UINT32 mac32)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 index = 0;
    ZXDH_L2_FWD_KEY *p_key_temp = NULL;
    ZXDH_L2_FWD_KEY *p_mac_key = NULL;
    ZXIC_UINT32 mac = 0;
    ZXIC_UINT32 rc = DPP_OK;

    p_mac_key = (ZXDH_L2_FWD_KEY *)ZXIC_COMM_MALLOC(mac_num*sizeof(ZXDH_L2_FWD_KEY));
    ZXIC_COMM_CHECK_POINT(p_mac_key);

    for(index = 0; index<mac_num; index++)
    {
        p_key_temp = p_mac_key + index;
        mac = mac32 + index;
        p_key_temp->dmac_addr[0] = (mac16>>8)&0xff;
        p_key_temp->dmac_addr[1] = mac16&0xff;
        p_key_temp->dmac_addr[2] = (mac>>24)&0xff;
        p_key_temp->dmac_addr[3] = (mac>>16)&0xff;
        p_key_temp->dmac_addr[4] = (mac>>8)&0xff;
        p_key_temp->dmac_addr[5] = mac&0xff;
        p_key_temp->sriov_vlan_tpid = (vlan_id>>16)&0xffff;
        p_key_temp->sriov_vlan_id = (vlan_id&0xffff)+index;
    }
    rc = dpp_batch_add_unicast_mac(&pf_info, mac_num, (ZXIC_VOID *)p_mac_key);
    ZXIC_COMM_FREE(p_mac_key);
    ZXIC_COMM_CHECK_RC(rc, "dpp_batch_add_unicast_mac");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_batch_mac_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT16 mac_num,
                                   ZXIC_UINT32 vlan_id, ZXIC_UINT16 mac16, ZXIC_UINT32 mac32)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 index = 0;
    ZXDH_L2_FWD_KEY *p_key_temp = NULL;
    ZXDH_L2_FWD_KEY *p_mac_key = NULL;
    ZXIC_UINT32 mac = 0;
    ZXIC_UINT32 rc = DPP_OK;

    p_mac_key = (ZXDH_L2_FWD_KEY *)ZXIC_COMM_MALLOC(mac_num*sizeof(ZXDH_L2_FWD_KEY));
    ZXIC_COMM_CHECK_POINT(p_mac_key);

    for(index = 0;index<mac_num; index++)
    {
        p_key_temp = p_mac_key + index;
        mac = mac32 + index;
        p_key_temp->dmac_addr[0] = (mac16>>8)&0xff;
        p_key_temp->dmac_addr[1] = mac16&0xff;
        p_key_temp->dmac_addr[2] = (mac>>24)&0xff;
        p_key_temp->dmac_addr[3] = (mac>>16)&0xff;
        p_key_temp->dmac_addr[4] = (mac>>8)&0xff;
        p_key_temp->dmac_addr[5] = mac&0xff;
        p_key_temp->sriov_vlan_tpid = (vlan_id>>16)&0xffff;
        p_key_temp->sriov_vlan_id = (vlan_id&0xffff)+index;
    }
    rc = dpp_batch_del_unicast_mac(&pf_info, mac_num, (ZXIC_VOID *)p_mac_key);
    ZXIC_COMM_FREE(p_mac_key);
    ZXIC_COMM_CHECK_RC(rc, "dpp_batch_del_unicast_mac");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_mac_transter(ZXIC_UINT16 slot, ZXIC_UINT16 vport,ZXIC_UINT16 new_vport)
{
    ZXIC_UINT32 rc = DPP_OK;

    DPP_PF_INFO_T pf_info = {slot, vport};
    DPP_PF_INFO_T new_pf_info = {slot, new_vport};

    rc = dpp_unicast_mac_transfer(&pf_info, &new_pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_unicast_mac_transfer");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_mac_max_num(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 max_num = 0;

    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_unicast_mac_max_get(&pf_info, &max_num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_multicast_mac_max_get");

    ZXIC_COMM_PRINT("uc_max_num: %u\n", max_num);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_batch_mc_mac_add(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT16 mac_num,
                                   ZXIC_UINT8 mac0, ZXIC_UINT8 mac1, ZXIC_UINT8 mac2,
                                   ZXIC_UINT8 mac3, ZXIC_UINT8 mac4, ZXIC_UINT8 mac5)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 index = 0;
    ZXIC_UINT8 *p_mac = NULL;
    ZXIC_UINT8 *p_mac_temp = NULL;
    ZXIC_UINT32 rc = DPP_OK;

    p_mac = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(mac_num*6);
    ZXIC_COMM_CHECK_POINT(p_mac);

    for(index = 0;index<mac_num; index++)
    {
        p_mac_temp = p_mac + index * 6;
        p_mac_temp[0] = mac0;
        p_mac_temp[1] = mac1;
        p_mac_temp[2] = mac2;
        p_mac_temp[3] = mac3;
        p_mac_temp[4] = mac4+((index>>8)&0xff);
        p_mac_temp[5] = mac5+(index&0xff);
    }
    rc = dpp_batch_add_multicast_mac(&pf_info, mac_num, p_mac);
    ZXIC_COMM_FREE(p_mac);
    ZXIC_COMM_CHECK_RC(rc, "dpp_batch_add_multicast_mac");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_batch_mc_mac_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport,ZXIC_UINT16 mac_num,
                                   ZXIC_UINT8 mac0, ZXIC_UINT8 mac1, ZXIC_UINT8 mac2,
                                   ZXIC_UINT8 mac3, ZXIC_UINT8 mac4, ZXIC_UINT8 mac5)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 index = 0;
    ZXIC_UINT8 *p_mac = NULL;
    ZXIC_UINT8 *p_mac_temp = NULL;
    ZXIC_UINT32 rc = DPP_OK;


    p_mac = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(mac_num*6);
    ZXIC_COMM_CHECK_POINT(p_mac);

    for(index = 0;index<mac_num; index++)
    {
        p_mac_temp = p_mac + index * 6;
        p_mac_temp[0] = mac0;
        p_mac_temp[1] = mac1;
        p_mac_temp[2] = mac2;
        p_mac_temp[3] = mac3;
        p_mac_temp[4] = mac4+((index>>8)&0xff);
        p_mac_temp[5] = mac5+(index&0xff);
    }
    rc = dpp_batch_del_multicast_mac(&pf_info, mac_num, p_mac);
    ZXIC_COMM_FREE(p_mac);
    ZXIC_COMM_CHECK_RC(rc, "dpp_batch_del_multicast_mac");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_mc_mac_transter(ZXIC_UINT16 slot, ZXIC_UINT16 vport,ZXIC_UINT16 new_vport)
{
    ZXIC_UINT32 rc = DPP_OK;

    DPP_PF_INFO_T pf_info = {slot, vport};
    DPP_PF_INFO_T new_pf_info = {slot, new_vport};

    rc = dpp_multicast_mac_transfer(&pf_info, &new_pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_unicast_mac_transfer");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_mc_mac_max_num(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 max_num = 0;

    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_multicast_mac_max_get(&pf_info, &max_num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_multicast_mac_max_get");

    ZXIC_COMM_PRINT("mc_max_num: %u\n", max_num);

    return DPP_OK;
}

/* Started by AICoder, pid:51ed8w97c8bf8e214f3608def09084150ce944fb */
ZXIC_UINT32 diag_dpp_vport_mac_flush_online(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXIC_UINT32 rc = dpp_unicast_all_mac_online_delete(&pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_unicast_all_mac_online_delete");
    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_mac_flush_offline(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXIC_UINT32 rc = dpp_unicast_all_mac_delete(&pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_unicast_all_mac_delete");
    return DPP_OK;
}
/* Ended by AICoder, pid:51ed8w97c8bf8e214f3608def09084150ce944fb */

ZXIC_UINT32 diag_dpp_vport_mac_search(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT16 sriov_vlan_tpid, ZXIC_UINT16 sriov_vlan_id,
                                   ZXIC_UINT8 mac0, ZXIC_UINT8 mac1, ZXIC_UINT8 mac2,
                                   ZXIC_UINT8 mac3, ZXIC_UINT8 mac4, ZXIC_UINT8 mac5)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT8  mac[6];
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT16 current_vport = 0;

    mac[0] = mac0;
    mac[1] = mac1;
    mac[2] = mac2;
    mac[3] = mac3;
    mac[4] = mac4;
    mac[5] = mac5;

    rc = dpp_unicast_mac_search(&pf_info, mac, sriov_vlan_tpid, sriov_vlan_id, &current_vport);
    ZXIC_COMM_CHECK_RC(rc, "dpp_unicast_mac_search");

    ZXIC_COMM_PRINT("current_mac_vport = 0x%04x\n", current_vport);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_mac_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    ZXIC_UINT32 mac_num = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 rc = DPP_OK;

    DPP_PF_INFO_T pf_info = {slot, vport};

    MAC_VPORT_INFO *p_mac_arr = (MAC_VPORT_INFO *)ZXIC_COMM_MALLOC(DTB_DUMP_UNICAST_MAC_DUMP_NUM *
                                                                          sizeof(MAC_VPORT_INFO));
    ZXIC_COMM_CHECK_POINT(p_mac_arr);

    rc = dpp_unicast_mac_dump(&pf_info, p_mac_arr, &mac_num);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_unicast_mac_dump", p_mac_arr);

    ZXIC_COMM_PRINT("-----------------------------------------------\n");
    for (i = 0; i < mac_num; i++)
    {
        ZXIC_COMM_PRINT("slot: %u vport: 0x%04x sriov_vlan_tpid: 0x%04x sriov_vlan_id: 0x%04x mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
                                                          slot, p_mac_arr[i].vport,p_mac_arr[i].sriov_vlan_tpid, p_mac_arr[i].sriov_vlan_id,
                                                          p_mac_arr[i].addr[0], p_mac_arr[i].addr[1],
                                                          p_mac_arr[i].addr[2], p_mac_arr[i].addr[3],
                                                          p_mac_arr[i].addr[4], p_mac_arr[i].addr[5]);
    }
    ZXIC_COMM_PRINT("------------dump mac num:0x%x-------------------\n",mac_num);

    ZXIC_COMM_FREE(p_mac_arr);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_mc_mac_add(ZXIC_UINT16 slot, ZXIC_UINT16 vport,
                                   ZXIC_UINT8 mac0, ZXIC_UINT8 mac1, ZXIC_UINT8 mac2,
                                   ZXIC_UINT8 mac3, ZXIC_UINT8 mac4, ZXIC_UINT8 mac5)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT8  mac[6];
    ZXIC_UINT32 rc = DPP_OK;

    mac[0] = mac0;
    mac[1] = mac1;
    mac[2] = mac2;
    mac[3] = mac3;
    mac[4] = mac4;
    mac[5] = mac5;

    rc = dpp_multi_mac_add_member(&pf_info, mac);
    ZXIC_COMM_CHECK_RC(rc, "dpp_multi_mac_add_member");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_mc_mac_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport,
                                   ZXIC_UINT8 mac0, ZXIC_UINT8 mac1, ZXIC_UINT8 mac2,
                                   ZXIC_UINT8 mac3, ZXIC_UINT8 mac4, ZXIC_UINT8 mac5)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT8  mac[6];
    ZXIC_UINT32 rc = DPP_OK;

    mac[0] = mac0;
    mac[1] = mac1;
    mac[2] = mac2;
    mac[3] = mac3;
    mac[4] = mac4;
    mac[5] = mac5;

    rc = dpp_multi_mac_del_member(&pf_info, mac);
    ZXIC_COMM_CHECK_RC(rc, "dpp_multi_mac_del_member");

    return DPP_OK;
}

/* Started by AICoder, pid:t2b06ldb39ibd58147ae09f7b04d8517f6751d65 */
ZXIC_UINT32 diag_dpp_vport_mc_mac_flush_online(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXIC_UINT32 rc = dpp_multicast_all_mac_online_delete(&pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_multicast_all_mac_online_delete");
    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_mc_mac_flush_offline(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXIC_UINT32 rc = dpp_multicast_all_mac_delete(&pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_multicast_all_mac_delete");
    return DPP_OK;
}
/* Ended by AICoder, pid:t2b06ldb39ibd58147ae09f7b04d8517f6751d65 */

ZXIC_UINT32 diag_dpp_vport_mc_mac_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    ZXIC_UINT32 mac_num = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 rc = DPP_OK;

    DPP_PF_INFO_T pf_info = {slot, vport};

    MAC_VPORT_INFO *p_mac_arr = (MAC_VPORT_INFO *)ZXIC_COMM_MALLOC(DTB_DUMP_MULTICAST_MAC_DUMP_NUM *
                                                                            sizeof(MAC_VPORT_INFO));
    ZXIC_COMM_CHECK_POINT(p_mac_arr);

    rc = dpp_multicast_mac_dump(&pf_info, p_mac_arr, &mac_num);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_multicast_mac_dump", p_mac_arr);

    ZXIC_COMM_PRINT("-----------------------------------------------\n");
    for (i = 0; i < mac_num; i++)
    {
        ZXIC_COMM_PRINT("slot: %u vport: 0x%04x mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
                                                          slot, p_mac_arr[i].vport,
                                                          p_mac_arr[i].addr[0], p_mac_arr[i].addr[1],
                                                          p_mac_arr[i].addr[2], p_mac_arr[i].addr[3],
                                                          p_mac_arr[i].addr[4], p_mac_arr[i].addr[5]);
    }
    ZXIC_COMM_PRINT("------------dump multicast mac num:0x%x---------\n",mac_num);

    ZXIC_COMM_FREE(p_mac_arr);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_table_init(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_create(&pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_create");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_table_delete(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_delete(&pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_delete");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_table_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 attr,
                                                                         ZXIC_UINT32 value)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_attr_set(&pf_info, attr, value);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_attr_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_table_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    ZXDH_SRIOV_VPORT_T port_table = {0};
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_attr_get(&pf_info, &port_table);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_attr_get");

    ZXIC_COMM_PRINT("-----------------------------------------------\n");
    ZXIC_COMM_PRINT("hit_flag = %u\n", port_table.hit_flag);
    ZXIC_COMM_PRINT("%02u mtu_offload_enable = %u\n", SRIOV_VPORT_MTU_OFFLOAD_EN_OFF, port_table.mtu_offload_enable);
    ZXIC_COMM_PRINT("%02u rss_enable = %u\n", SRIOV_VPORT_RSS_EN_OFF, port_table.rss_enable);
    ZXIC_COMM_PRINT("%02u sriov_business_vlan_offload_enable = %u\n", SRIOV_VPORT_BUSINESS_VLAN_OFFLOAD_EN, port_table.sriov_business_vlan_offload_enable);
    ZXIC_COMM_PRINT("%02u sriov_vlan_offload_enable = %u\n", SRIOV_VPORT_VLAN_OFFLOAD_EN, port_table.sriov_vlan_offload_enable);
    ZXIC_COMM_PRINT("%02u promisc_enable = %u\n", SRIOV_VPORT_PROMISC_EN, port_table.promisc_enable);
    ZXIC_COMM_PRINT("%02u rdma_offload_enable = %u\n", SRIOV_VPORT_RDMA_OFFLOAD_EN_OFF, port_table.rdma_offload_enable);
    ZXIC_COMM_PRINT("%02u hw_bond_enable = %u\n", SRIOV_VPORT_HW_BOND_EN_OFF, port_table.hw_bond_enable);
    
    ZXIC_COMM_PRINT("%02u business_enable = %u\n", SRIOV_VPORT_BUSINESS_EN_OFF, port_table.business_enable);
    ZXIC_COMM_PRINT("%02u is_up = %u\n", SRIOV_VPORT_IS_UP, port_table.is_up);
    ZXIC_COMM_PRINT("%02u outer_ip_checksum_offload = %u\n", SRIOV_VPORT_OUTER_IP_CHECKSUM_OFFLOAD, port_table.outer_ip_checksum_offload);
    ZXIC_COMM_PRINT("%02u ip_checksum_offload = %u\n", SRIOV_VPORT_IP_CHKSUM, port_table.ip_checksum_offload);
    ZXIC_COMM_PRINT("%02u tcp_udp_checksum_offload = %u\n", SRIOV_VPORT_TCP_UDP_CHKSUM, port_table.tcp_udp_checksum_offload);
    ZXIC_COMM_PRINT("%02u ip_recombine_offload = %u\n", SRIOV_VPORT_IP_RECOMBINE, port_table.ip_recombine_offload);
    ZXIC_COMM_PRINT("%02u lro_offload = %u\n", SRIOV_VPORT_IPV6_TCP_ASSEMBLE, port_table.lro_offload);
    ZXIC_COMM_PRINT("%02u lro_offload = %u\n", SRIOV_VPORT_IPV4_TCP_ASSEMBLE, port_table.lro_offload);
    ZXIC_COMM_PRINT("%02u accelerator_offload_flag = %u\n", SRIOV_VPORT_ACCELERATOR_OFFLOAD_FLAG, port_table.accelerator_offload_flag);

    ZXIC_COMM_PRINT("%02u virtio_enable = %u\n", SRIOV_VPORT_VIRTIO_EN_OFF, port_table.virtio_enable);
    ZXIC_COMM_PRINT("%02u virtio_version = %u\n", SRIOV_VPORT_VIRTIO_VERSION, port_table.virtio_version);
    ZXIC_COMM_PRINT("%02u is_vf = %u\n", SRIOV_VPORT_IS_VF, port_table.is_vf);
    ZXIC_COMM_PRINT("%02u vepa_enable = %u\n", SRIOV_VPORT_VEPA_EN_OFF, port_table.vepa_enable);
    ZXIC_COMM_PRINT("%02u lag_enable = %u\n", SRIOV_VPORT_LAG_EN_OFF, port_table.lag_enable);
    ZXIC_COMM_PRINT("%02u fd_enable = %u\n", SRIOV_VPORT_FD_EN_OFF, port_table.fd_enable);
    ZXIC_COMM_PRINT("%02u inline_sec_offload = %u\n", SRIOV_VPORT_INLINE_SEC_OFFLOAD, port_table.inline_sec_offload);

    ZXIC_COMM_PRINT("%02u spoof_check_enable = %u\n", SRIOV_VPORT_SPOOFCHK_EN_OFF, port_table.spoof_check_enable);
    ZXIC_COMM_PRINT("%02u np_ingress_tm_enable = %u\n", SRIOV_VPORT_NP_INGRESS_TM_EN_OFF, port_table.np_ingress_tm_enable);
    ZXIC_COMM_PRINT("%02u np_egress_tm_enable = %u\n", SRIOV_VPORT_NP_EGRESS_TM_EN_OFF, port_table.np_egress_tm_enable);
    ZXIC_COMM_PRINT("%02u np_ingress_meter_mode = %u\n", SRIOV_VPORT_NP_INGRESS_MODE, port_table.np_ingress_meter_mode);
    ZXIC_COMM_PRINT("%02u np_egress_meter_mode = %u\n", SRIOV_VPORT_NP_EGRESS_MODE, port_table.np_egress_meter_mode);   
    ZXIC_COMM_PRINT("%02u np_ingress_meter_enable = %u\n", SRIOV_VPORT_NP_INGRESS_METER_EN_OFF, port_table.np_ingress_meter_enable);
    ZXIC_COMM_PRINT("%02u np_egress_meter_enable = %u\n", SRIOV_VPORT_NP_EGRESS_METER_EN_OFF, port_table.np_egress_meter_enable);

    ZXIC_COMM_PRINT("%02u hash_search_index = %u\n", SRIOV_VPORT_HASH_SEARCH_INDEX, port_table.hash_search_index);
    ZXIC_COMM_PRINT("%02u port_base_qid = %u\n", SRIOV_VPORT_PORT_BASE_QID, port_table.port_base_qid);
    ZXIC_COMM_PRINT("%02u mtu = %u\n", SRIOV_VPORT_MTU, port_table.mtu);
    ZXIC_COMM_PRINT("%02u pf_vqm_vfid = %u\n", SRIOV_VPORT_PF_VQM_VFID, port_table.pf_vqm_vfid);
    ZXIC_COMM_PRINT("%02u fd_vxlan_offload_en = %u\n", SRIOV_VPORT_FD_VXLAN_OFFLOAD_EN, port_table.fd_vxlan_offload_en);
    ZXIC_COMM_PRINT("%02u lag_id = %u\n", SRIOV_VPORT_LAG_ID, port_table.lag_id); 
    ZXIC_COMM_PRINT("%02u uplink_phy_port_id = %u\n", SRIOV_VPORT_UPLINK_PHY_PORT_ID, port_table.uplink_phy_port_id);
    ZXIC_COMM_PRINT("%02u hash_alg = %u\n", SRIOV_VPORT_HASH_ALG, port_table.hash_alg);
    ZXIC_COMM_PRINT("%02u rss_hash_factor = %u\n", SRIOV_VPORT_RSS_HASH_FACTOR, port_table.rss_hash_factor);
    ZXIC_COMM_PRINT("%02u 1588_enable = %u\n", SRIOV_VPORT_1588_EN, port_table.flag_1588_enable);
    ZXIC_COMM_PRINT("%02u roce_overlay_enable = %u\n", SRIOV_VPORT_ROCE_OVERLAY_EN, port_table.roce_overlay_enable);
    ZXIC_COMM_PRINT("%02u k8s_cni_flag = %u\n", SRIOV_VPORT_K8S_CNI_FLAG, port_table.k8s_cni_flag);

    ZXIC_COMM_PRINT("%02u vhca = %u\n", SRIOV_VPORT_VHCA, port_table.vhca);
    ZXIC_COMM_PRINT("%02u multi_host_enable = %u\n", SRIOV_VPORT_MULTI_HOST_EN, port_table.multi_host_enable);
    ZXIC_COMM_PRINT("%02u multi_host_group_id = %u\n", SRIOV_VPORT_MULTI_HOST_GROUP_ID, port_table.multi_host_group_id);
    ZXIC_COMM_PRINT("%02u frag_pkt_use_ipid = %u\n", SRIOV_VPORT_FRAG_PKT_USE_IPID, port_table.frag_pkt_use_ipid);
    ZXIC_COMM_PRINT("%02u outer_l4_checksum_offload = %u\n", SRIOV_VPORT_OUTER_L4_CHECKSUM_OFFLOAD, port_table.outer_l4_checksum_offload);
    ZXIC_COMM_PRINT("%02u multi_plane_en = %u\n", SRIOV_VPORT_MULTI_PLANE_EN, port_table.multi_plane_en);
    
    ZXIC_COMM_PRINT("-----------------------------------------------\n");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_egress_meter_en_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 enable)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_egress_meter_en_set(&pf_info, enable);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_egress_meter_en_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_egress_meter_en_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 enable = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_egress_meter_en_get(&pf_info, &enable);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_egress_meter_en_get");

    ZXIC_COMM_PRINT("enable: %u\n", enable);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_ingress_meter_en_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 enable)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_ingress_meter_en_set(&pf_info, enable);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_ingress_meter_en_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_ingress_meter_en_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 enable = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_ingress_meter_en_get(&pf_info, &enable);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_ingress_meter_en_get");

    ZXIC_COMM_PRINT("enable: %u\n", enable);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_egress_meter_mode_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_egress_meter_mode_set(&pf_info, mode);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_egress_meter_mode_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_egress_meter_mode_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 mode = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_egress_meter_mode_get(&pf_info, &mode);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_egress_meter_mode_get");

    ZXIC_COMM_PRINT("mode: %u\n", mode);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_ingress_meter_mode_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_ingress_meter_mode_set(&pf_info, mode);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_ingress_meter_mode_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_ingress_meter_mode_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 mode = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_ingress_meter_mode_get(&pf_info, &mode);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_ingress_meter_mode_get");

    ZXIC_COMM_PRINT("mode: %u\n", mode);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_rx_flow_hash_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 hash_mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_rx_flow_hash_set(&pf_info, hash_mode);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_rx_flow_hash_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_rx_flow_hash_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 hash_mode = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_rx_flow_hash_get(&pf_info, &hash_mode);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_rx_flow_hash_get");

    ZXIC_COMM_PRINT("hash_mode: %u\n", hash_mode);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_hash_index_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 hash_index = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_hash_index_get(&pf_info, &hash_index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_hash_index_get");

    ZXIC_COMM_PRINT("hash_index: %u\n", hash_index);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_hash_funcs_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 funcs)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_hash_funcs_set(&pf_info, funcs);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_hash_funcs_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_rss_en_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 enable)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_rss_en_set(&pf_info, enable);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_rss_en_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_virtio_en_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 enable)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_virtio_en_set(&pf_info, enable);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_virtio_en_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_virtio_version_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 version)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_virtio_version_set(&pf_info, version);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_virtio_version_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_promisc_en_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 enable)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_promisc_en_set(&pf_info, enable);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_promisc_en_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_business_vlan_offload_en_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 enable)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_business_vlan_offload_en_set(&pf_info, enable);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_business_vlan_offload_en_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_vlan_offload_en_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 enable)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_vlan_offload_en_set(&pf_info, enable);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_vlan_offload_en_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_uplink_phy_port_table_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id,
                                                                ZXIC_UINT32 attr, ZXIC_UINT32 value)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_uplink_phy_attr_set(&pf_info, uplink_phy_port_id, attr, value);
    ZXIC_COMM_CHECK_RC(rc, "dpp_uplink_phy_attr_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_uplink_phy_port_table_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id)
{
    DPP_DEV_T dev = {0};
    ZXDH_UPLINK_PHY_PORT_T uplink_phy_port_table = {0};
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 sdt_no = ZXDH_SDT_UPLINK_PHY_PORT_ATTR_TABLE;
    ZXIC_UINT32 rc     = DPP_OK;

    rc = dpp_dev_get(&pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_apt_dtb_eram_get(&dev, queue, sdt_no, uplink_phy_port_id, &uplink_phy_port_table);
    ZXIC_COMM_CHECK_RC(rc, "dpp_apt_dtb_eram_get");

    ZXIC_COMM_PRINT("-----------------------------------------------\n");
    ZXIC_COMM_PRINT("hit_flag = %u\n", uplink_phy_port_table.hit_flag);
    ZXIC_COMM_PRINT("%02u trust_mode = %u\n", UPLINK_PHY_PORT_TRUST_MODE, uplink_phy_port_table.trust_mode);
    ZXIC_COMM_PRINT("%02u ptp_tc_enable = %u\n", UPLINK_PHY_PORT_PTP_TC_ENABLE, uplink_phy_port_table.ptp_tc_enable);
    ZXIC_COMM_PRINT("%02u tm_shape_enable = %u\n", UPLINK_PHY_PORT_TM_SHAPE_ENABLE, uplink_phy_port_table.tm_shape_enable);
    ZXIC_COMM_PRINT("%02u magic_packet_enable = %u\n", UPLINK_PHY_PORT_MAGIC_PACKET_ENABLE, uplink_phy_port_table.magic_packet_enable);
    ZXIC_COMM_PRINT("%02u ptp_port_vfid = %u\n", UPLINK_PHY_PORT_PTP_PORT_VFID, uplink_phy_port_table.ptp_port_vfid);
    ZXIC_COMM_PRINT("%02u tm_base_queue = %u\n", UPLINK_PHY_PORT_TM_BASE_QUEUE, uplink_phy_port_table.tm_base_queue);
    ZXIC_COMM_PRINT("%02u mtu_offload_enable = %u\n", UPLINK_PHY_PORT_MTU_OFFLOAD_ENABLE, uplink_phy_port_table.mtu_offload_enable);
    ZXIC_COMM_PRINT("%02u mtu = %u\n", UPLINK_PHY_PORT_MTU, uplink_phy_port_table.mtu);
    ZXIC_COMM_PRINT("%02u hw_bond_enable = %u\n", UPLINK_PHY_PORT_HW_BOND_ENABLE, uplink_phy_port_table.hw_bond_enable);
    ZXIC_COMM_PRINT("%02u bond_link_up = %u\n", UPLINK_PHY_PORT_BOND_LINK_UP, uplink_phy_port_table.bond_link_up);
    ZXIC_COMM_PRINT("%02u is_up = %u\n", UPLINK_PHY_PORT_IS_UP, uplink_phy_port_table.is_up); 
    ZXIC_COMM_PRINT("%02u sriov_dh_bond_en = %u\n", UPLINK_PHY_PORT_SRIOV_HD_BOND_EN, uplink_phy_port_table.sriov_hdbond_enable);   
    ZXIC_COMM_PRINT("%02u lacp_pf_vqm_vfid = %u\n", UPLINK_PHY_PORT_LACP_PF_VQM_VFID, uplink_phy_port_table.lacp_pf_vqm_vfid);
    ZXIC_COMM_PRINT("%02u lacp_pf_memport_qid = %u\n", UPLINK_PHY_PORT_LACP_PF_MEMPORT_QID, uplink_phy_port_table.lacp_pf_memport_qid);
    ZXIC_COMM_PRINT("%02u pf_vqm_vfid = %u\n", UPLINK_PHY_PORT_PF_VQM_VFID, uplink_phy_port_table.pf_vqm_vfid);  
    ZXIC_COMM_PRINT("%02u roce_overlay_enable = %u\n", UPLINK_PHY_PORT_ROCE_OVERLAY_EN, uplink_phy_port_table.roce_overlay_enable);
    ZXIC_COMM_PRINT("%02u multi_host_enable = %u\n", UPLINK_PHY_PORT_MULTI_HOST_EN, uplink_phy_port_table.multi_host_enable);
    ZXIC_COMM_PRINT("%02u multi_host_group_id = %u\n", UPLINK_PHY_PORT_MULTI_HOST_GROUP_ID, uplink_phy_port_table.multi_host_group_id); 
    ZXIC_COMM_PRINT("%02u primary_pf_vqm_vfid = %u\n", UPLINK_PHY_PORT_PRIMARY_PF_VQM_VFID, uplink_phy_port_table.primary_pf_vqm_vfid); 
    ZXIC_COMM_PRINT("-----------------------------------------------\n");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_uplink_phy_bond_vport(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_uplink_phy_bond_vport(&pf_info, uplink_phy_port_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_uplink_phy_bond_vport");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_uplink_phy_hardware_bond_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id, ZXIC_UINT8 enable)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_uplink_phy_hardware_bond_set(&pf_info, uplink_phy_port_id, enable);
    ZXIC_COMM_CHECK_RC(rc, "dpp_uplink_phy_hardware_bond_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_uplink_phy_lacp_pf_vqm_vfid_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id, ZXIC_UINT16 vqm_vfid)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_uplink_phy_lacp_pf_vqm_vfid_set(&pf_info, uplink_phy_port_id, vqm_vfid);
    ZXIC_COMM_CHECK_RC(rc, "dpp_uplink_phy_lacp_pf_vqm_vfid_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_uplink_phy_lacp_pf_memport_qid_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id, ZXIC_UINT16 qid)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_uplink_phy_lacp_pf_memport_qid_set(&pf_info, uplink_phy_port_id, qid);
    ZXIC_COMM_CHECK_RC(rc, "dpp_uplink_phy_lacp_pf_memport_qid_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_ptp_port_vfid_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 ptp_port_vfid)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_ptp_port_vfid_set(&pf_info, ptp_port_vfid);
    ZXIC_COMM_CHECK_RC(rc, "dpp_ptp_port_vfid_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_ptp_tc_enable_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 ptp_tc_enable)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_ptp_tc_enable_set(&pf_info, ptp_tc_enable);
    ZXIC_COMM_CHECK_RC(rc, "dpp_ptp_tc_enable_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_tm_flowid_pport_table_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id, ZXIC_UINT32 flow_id)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_tm_flowid_pport_table_set(&pf_info, uplink_phy_port_id, flow_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tm_flowid_pport_table_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_tm_flowid_pport_table_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_tm_flowid_pport_table_del(&pf_info, uplink_phy_port_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tm_flowid_pport_table_del");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_tm_pport_trust_mode_table_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_tm_pport_trust_mode_table_set(&pf_info, uplink_phy_port_id, mode);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tm_pport_trust_mode_table_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_tm_pport_trust_mode_table_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_tm_pport_trust_mode_table_del(&pf_info, uplink_phy_port_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tm_pport_trust_mode_table_del");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_tm_pport_mcode_switch_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_tm_pport_mcode_switch_set(&pf_info, uplink_phy_port_id, mode);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tm_pport_mcode_switch_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_tm_pport_mcode_switch_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 uplink_phy_port_id)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_tm_pport_mcode_switch_del(&pf_info, uplink_phy_port_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tm_pport_mcode_switch_del");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_bc_table_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 enable)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    if (enable == 1)
    {
        rc = dpp_vport_bond_pf(&pf_info);
        ZXIC_COMM_CHECK_RC(rc, "dpp_vport_bond_pf");
    }
    else
    {
        rc = dpp_vport_unbond_pf(&pf_info);
        ZXIC_COMM_CHECK_RC(rc, "dpp_vport_unbond_pf");
    }

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_bc_table_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_DEV_T dev = {0};
    ZXDH_BC_T bc_table = {0};
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 queue    = 0;
    ZXIC_UINT32 sdt_no   = ZXDH_SDT_BC_TABLE;
    ZXIC_UINT32 group_id = 0;
    ZXIC_UINT32 index    = 0;
    ZXIC_UINT32 i        = 0;
    ZXIC_UINT32 rc       = DPP_OK;

    rc = dpp_dev_get(&pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    ZXIC_COMM_PRINT("-----------------------------------------------\n");
    for (group_id = 0; group_id < BC_GROUP_NUM; group_id++)
    {
        index = (((OWNER_PF_VQM_VFID(pf_info.vport) - PF_VQM_VFID_OFFSET) << 2)| group_id);

        rc = dpp_apt_dtb_eram_get(&dev, queue, sdt_no, index, &bc_table);
        ZXIC_COMM_CHECK_RC(rc, "dpp_apt_dtb_eram_get");

        if (bc_table.hit_flag == 1)
        {
            for (i = 0; i < BC_MEMBER_NUM_IN_GROUP; i++)
            {
                if ((bc_table.bc_bitmap & ((ZXIC_UINT64)(1) << (BC_MEMBER_NUM_IN_GROUP - 1 - i))) != 0)
                {
                    ZXIC_COMM_PRINT("vf %u enable\n", i + (group_id * BC_MEMBER_NUM_IN_GROUP));
                }
            }
        }
    }
    ZXIC_COMM_PRINT("-----------------------------------------------\n");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_promisc_table_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no)
{
    DPP_DEV_T dev = {0};
    ZXDH_PROMISC_T promisc_table[4] = {0};
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 queue    = 0;
    ZXIC_UINT32 group_id = 0;
    ZXIC_UINT32 index    = 0;
    ZXIC_UINT32 i        = 0;
    ZXIC_UINT32 rc       = DPP_OK;

    rc = dpp_dev_get(&pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    ZXIC_COMM_PRINT("-----------------------------------------------\n");
    for (group_id = 0; group_id < BC_GROUP_NUM; group_id++)
    {
        index = (((OWNER_PF_VQM_VFID(pf_info.vport) - PF_VQM_VFID_OFFSET) << 2)| group_id);

        rc = dpp_apt_dtb_eram_get(&dev, queue, sdt_no, index, &promisc_table[group_id]);
        ZXIC_COMM_CHECK_RC(rc, "dpp_apt_dtb_eram_get");

        if (promisc_table[group_id].hit_flag == 1)
        {
            for (i = 0; i < PROMISC_MEMBER_NUM_IN_GROUP; i++)
            {
                if ((promisc_table[group_id].bitmap & ((ZXIC_UINT64)(1) << (PROMISC_MEMBER_NUM_IN_GROUP - 1 - i))) != 0)
                {
                    ZXIC_COMM_PRINT("vf %u enable\n", i + (group_id * PROMISC_MEMBER_NUM_IN_GROUP));
                }
            }
        }
    }
    if ((promisc_table[0].pf_enable == 1) && (promisc_table[1].pf_enable == 1) &&
        (promisc_table[2].pf_enable == 1) && (promisc_table[3].pf_enable == 1))
    {
        ZXIC_COMM_PRINT("pf enable\n");
    }
    ZXIC_COMM_PRINT("-----------------------------------------------\n");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_uc_promisc_table_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 enable)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_uc_promisc_set(&pf_info, enable);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_uc_promisc_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_uc_promisc_table_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    diag_dpp_vport_promisc_table_prt(slot, vport, ZXDH_SDT_UC_PROMISC_TABLE);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_mc_promisc_table_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 enable)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_mc_promisc_set(&pf_info, enable);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_mc_promisc_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_mc_promisc_table_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    diag_dpp_vport_promisc_table_prt(slot, vport, ZXDH_SDT_MC_PROMISC_TABLE);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_rdma_trans_item_add(ZXIC_UINT16 slot, ZXIC_UINT16 vport,
                                   ZXIC_UINT8 mac0, ZXIC_UINT8 mac1, ZXIC_UINT8 mac2,
                                   ZXIC_UINT8 mac3, ZXIC_UINT8 mac4, ZXIC_UINT8 mac5, ZXIC_UINT16 vhcaId)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT8  mac[6];
    ZXIC_UINT32 rc = DPP_OK;

    mac[0] = mac0;
    mac[1] = mac1;
    mac[2] = mac2;
    mac[3] = mac3;
    mac[4] = mac4;
    mac[5] = mac5;

    rc = dpp_add_rdma_trans_item(&pf_info, mac, vhcaId);
    ZXIC_COMM_CHECK_RC(rc, "dpp_add_rdma_trans_item");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_rdma_trans_item_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport,
                                   ZXIC_UINT8 mac0, ZXIC_UINT8 mac1, ZXIC_UINT8 mac2,
                                   ZXIC_UINT8 mac3, ZXIC_UINT8 mac4, ZXIC_UINT8 mac5)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT8  mac[6];
    ZXIC_UINT32 rc = DPP_OK;

    mac[0] = mac0;
    mac[1] = mac1;
    mac[2] = mac2;
    mac[3] = mac3;
    mac[4] = mac4;
    mac[5] = mac5;

    rc = dpp_del_rdma_trans_item(&pf_info, mac);
    ZXIC_COMM_CHECK_RC(rc, "dpp_del_rdma_trans_item");

    return DPP_OK;
}

DPP_STATUS diag_dpp_pcie_channel_prt(ZXIC_VOID)
{
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 slot = 0;
    ZXIC_UINT32 channel = 0;
    DPP_DEV_CFG_T *p_dev_info = NULL;
    DPP_DEV_MGR_T *p_dev_mgr  = NULL;
    DPP_PCIE_CHANNEL_T *p_pcie = NULL;
    DPP_SE_CFG *p_se_cfg = NULL;
    DPP_DEV_T dev = {0};

    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id, DPP_DEV_CHANNEL_MAX - 1);

    p_dev_mgr = dpp_dev_mgr_get();
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_mgr);
    p_dev_info = p_dev_mgr->p_dev_array[dev_id];
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_info);

    for(slot=0;slot<DPP_PCIE_SLOT_MAX;slot++)
    {
        for(channel=0;channel<DPP_PCIE_CHANNEL_MAX;channel++)
        {
            if(p_dev_info->pcie_channel[slot][channel].is_used)
            {
                p_pcie = &p_dev_info->pcie_channel[slot][channel];
                ZXIC_COMM_PRINT("\n------------PCIE SLOT[%d] CHANNEL[%d]--------------------\n",slot,channel);
                ZXIC_COMM_PRINT("|slot:%d  vport:0x%x  pcie_id:0x%x\n", p_pcie->slot, p_pcie->vport, p_pcie->pcie_id);
                ZXIC_COMM_PRINT("|base_addr:0x%llx  offset_addr:0x%llx\n", p_pcie->base_addr,p_pcie->offset_addr);
                ZXIC_COMM_PRINT("|hash_index:0x%x\n",p_pcie->hash_index);
                ZXIC_COMM_PRINT("|dma_size:0x%x  dma_phy_addr:0x%llx  dma_vir_addr:0x%llx\n",
                                 p_pcie->dump_dma_size,p_pcie->dump_dma_phy_addr,p_pcie->dump_dma_vir_addr);
            }
        }
    }

    for(slot=0;slot<DPP_PCIE_SLOT_MAX;slot++)
    {
        ZXIC_COMM_MEMSET_S(&dev,sizeof(DPP_DEV_T),0x0,sizeof(DPP_DEV_T));
        dev.pcie_channel.slot = slot;
        p_se_cfg = dpp_apt_get_se_cfg(&dev);
        if(p_se_cfg!=NULL)
        {
            p_pcie = &(p_se_cfg->dev.pcie_channel);
            ZXIC_COMM_PRINT("\n------------SE CFG SLOT[%d] USED[%d]--------------------\n",slot,p_pcie->is_used);
            ZXIC_COMM_PRINT("|slot:%d  vport:0x%x  pcie_id:0x%x\n", p_pcie->slot, p_pcie->vport, p_pcie->pcie_id);
            ZXIC_COMM_PRINT("|base_addr:0x%llx  offset_addr:0x%llx\n", p_pcie->base_addr,p_pcie->offset_addr);
        }
    }

    return DPP_OK;
}

DPP_STATUS diag_dpp_se_hash_stat_prt(ZXIC_UINT32 slot_id,ZXIC_UINT32 fun_id)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 table_id = 0;
    DPP_SE_CFG *p_se_cfg = NULL;
    DPP_DEV_T dev = {0};
    DPP_HASH_CFG *p_hash_cfg = NULL;
    FUNC_ID_INFO *p_func_info = NULL;
    DPP_APT_SE_RES_T *p_se_res = NULL;

    ZXIC_COMM_CHECK_INDEX(fun_id, HASH_FUNC_ID_MIN, HASH_FUNC_ID_NUM - 1);
    ZXIC_COMM_CHECK_INDEX(slot_id, 0, DPP_PCIE_SLOT_MAX - 1);

    /* 获取 g_se_cfg  */
    dev.device_id = 0;
    dev.pcie_channel.slot = slot_id;
    rc = dpp_se_cfg_get(&dev, &p_se_cfg);
    ZXIC_COMM_CHECK_RC(rc, "dpp_se_cfg_get");
    ZXIC_COMM_CHECK_POINT(p_se_cfg);
    
    p_func_info = DPP_GET_FUN_INFO(p_se_cfg, fun_id);
    DPP_SE_CHECK_FUN(p_func_info, fun_id, FUN_HASH);
    p_hash_cfg = (DPP_HASH_CFG *)p_func_info->fun_ptr;

    ZXIC_COMM_PRINT("------------slot[%d]--------------------\n",slot_id);
    ZXIC_COMM_PRINT("|insert ok       num is:%d  |\n", p_hash_cfg->hash_stat.insert_ok);
    ZXIC_COMM_PRINT("|insert ddr      num is:%d  |\n", p_hash_cfg->hash_stat.insert_ddr);
    ZXIC_COMM_PRINT("|insert zcell    num is:%d  |\n", p_hash_cfg->hash_stat.insert_zcell);
    ZXIC_COMM_PRINT("|insert zreg     num is:%d  |\n", p_hash_cfg->hash_stat.insert_zreg);
    ZXIC_COMM_PRINT("|insert same     num is:%d  |\n", p_hash_cfg->hash_stat.insert_same);
    ZXIC_COMM_PRINT("|insert fail     num is:%d  |\n", p_hash_cfg->hash_stat.insert_fail);
    ZXIC_COMM_PRINT("|delete ok       num is:%d  |\n", p_hash_cfg->hash_stat.delete_ok);
    ZXIC_COMM_PRINT("|delete fail     num is:%d  |\n", p_hash_cfg->hash_stat.delete_fail);
    ZXIC_COMM_PRINT("|search ok       num is:%d  |\n", p_hash_cfg->hash_stat.search_ok);
    ZXIC_COMM_PRINT("|search fail     num is:%d  |\n", p_hash_cfg->hash_stat.search_fail);
    ZXIC_COMM_PRINT("--------------------------------\n");

    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(&dev);
    ZXIC_COMM_CHECK_POINT(p_se_res);

    for(i=0;i<(p_se_res->hash_tbl_num);i++)
    {
        if(p_se_res->hash_tbl[i].hashSdt.hash_id==fun_id)
        {
            table_id = p_se_res->hash_tbl[i].hashSdt.hash_table_id;
            ZXIC_COMM_CHECK_INDEX_UPPER(table_id,HASH_TBL_ID_NUM - 1);
            ZXIC_COMM_PRINT("------------sdt_no[%u]--------------------\n",p_se_res->hash_tbl[i].sdtNo);
            ZXIC_COMM_PRINT("|insert succ     num is:%d  |\n", p_hash_cfg->hash_stat.insert_table[table_id].sum);
            ZXIC_COMM_PRINT("|insert ddr      num is:%d  |\n", p_hash_cfg->hash_stat.insert_table[table_id].ddr);
            ZXIC_COMM_PRINT("|insert zcell    num is:%d  |\n", p_hash_cfg->hash_stat.insert_table[table_id].zcell);
            ZXIC_COMM_PRINT("|insert zreg     num is:%d  |\n", p_hash_cfg->hash_stat.insert_table[table_id].zreg);
            ZXIC_COMM_PRINT("|insert same     num is:%d  |\n", p_hash_cfg->hash_stat.insert_table[table_id].same);
            ZXIC_COMM_PRINT("|delete succ     num is:%d  |\n", p_hash_cfg->hash_stat.insert_table[table_id].delete);
        }
    }

    return DPP_OK;
}

DPP_STATUS diag_dpp_se_hash_stat_clr(ZXIC_UINT32 slot_id,ZXIC_UINT32 fun_id)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 table_id = 0;
    DPP_SE_CFG *p_se_cfg = NULL;
    DPP_DEV_T dev = {0};
    DPP_HASH_CFG *p_hash_cfg = NULL;
    FUNC_ID_INFO *p_func_info = NULL;
    DPP_APT_SE_RES_T *p_se_res = NULL;

    ZXIC_COMM_CHECK_INDEX(fun_id, HASH_FUNC_ID_MIN, HASH_FUNC_ID_NUM - 1);
    ZXIC_COMM_CHECK_INDEX(slot_id, 0, DPP_PCIE_SLOT_MAX - 1);

    /* 获取 g_se_cfg  */
    dev.device_id = 0;
    dev.pcie_channel.slot = slot_id;
    rc = dpp_se_cfg_get(&dev, &p_se_cfg);
    ZXIC_COMM_CHECK_RC(rc, "dpp_se_cfg_get");
    ZXIC_COMM_CHECK_POINT(p_se_cfg);
    
    p_func_info = DPP_GET_FUN_INFO(p_se_cfg, fun_id);
    DPP_SE_CHECK_FUN(p_func_info, fun_id, FUN_HASH);
    p_hash_cfg = (DPP_HASH_CFG *)p_func_info->fun_ptr;
    ZXIC_COMM_CHECK_POINT(p_hash_cfg);

    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(&dev);
    ZXIC_COMM_CHECK_POINT(p_se_res);

    p_hash_cfg->hash_stat.insert_ok = 0;
    p_hash_cfg->hash_stat.insert_ddr = 0;
    p_hash_cfg->hash_stat.insert_zcell = 0;
    p_hash_cfg->hash_stat.insert_zreg = 0;
    p_hash_cfg->hash_stat.insert_same = 0;
    p_hash_cfg->hash_stat.insert_fail = 0;
    p_hash_cfg->hash_stat.delete_ok = 0;
    p_hash_cfg->hash_stat.delete_fail = 0;
    p_hash_cfg->hash_stat.search_ok = 0;
    p_hash_cfg->hash_stat.search_fail = 0;

    for(i=0;i<(p_se_res->hash_tbl_num);i++)
    {
        if(p_se_res->hash_tbl[i].hashSdt.hash_id==fun_id)
        {
            table_id = p_se_res->hash_tbl[i].hashSdt.hash_table_id;
            ZXIC_COMM_CHECK_INDEX_UPPER(table_id,HASH_TBL_ID_NUM - 1);
            p_hash_cfg->hash_stat.insert_table[table_id].sum = 0;
            p_hash_cfg->hash_stat.insert_table[table_id].ddr = 0;
            p_hash_cfg->hash_stat.insert_table[table_id].zcell = 0;
            p_hash_cfg->hash_stat.insert_table[table_id].zreg = 0;
            p_hash_cfg->hash_stat.insert_table[table_id].same = 0;
            p_hash_cfg->hash_stat.insert_table[table_id].delete = 0;
        }
    }

    return DPP_OK;
}

DPP_STATUS diag_dpp_hash_item_prt(ZXIC_UINT32 slot, ZXIC_UINT32 sdt_no)
{
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT8 key_valid = 0;
    ZXIC_UINT32 table_id = 0;
    ZXIC_UINT32 key_type = 0;
    DPP_DEV_T dev = {0};

    D_NODE *p_node = NULL;
    ZXIC_RB_TN *p_rb_tn = NULL;
    D_HEAD *p_head_hash_rb = NULL;
    DPP_HASH_CFG *p_hash_cfg = NULL;
    DPP_HASH_RBKEY_INFO *p_rbkey = NULL;
    SE_APT_CALLBACK_T *pAptCallback = NULL;

    DPP_HASH_ENTRY hash_entry = {0};
    HASH_ENTRY_CFG  hash_entry_cfg = {0}; 
    ZXDH_L2_ENTRY_T l2_entry = {0};
    ZXDH_MC_T mc_entry = {0};
    ZXDH_RDMA_TRANS_T rdma_trans = {0};
    ZXIC_UINT8 key[HASH_KEY_MAX] = {0};
    ZXIC_UINT8 rst[HASH_RST_MAX] = {0};
    
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, slot, 0, DPP_PCIE_SLOT_MAX - 1);

    ZXIC_COMM_MEMSET_S(&hash_entry,sizeof(DPP_HASH_ENTRY),0x0,sizeof(DPP_HASH_ENTRY));
    ZXIC_COMM_MEMSET_S(&hash_entry_cfg,sizeof(HASH_ENTRY_CFG),0x0,sizeof(HASH_ENTRY_CFG));
    ZXIC_COMM_MEMSET_S(&l2_entry,sizeof(ZXDH_L2_ENTRY_T),0x0,sizeof(ZXDH_L2_ENTRY_T));
    ZXIC_COMM_MEMSET_S(&mc_entry,sizeof(ZXDH_MC_T),0x0,sizeof(ZXDH_MC_T));
    ZXIC_COMM_MEMSET_S(&rdma_trans,sizeof(ZXDH_RDMA_TRANS_T),0x0,sizeof(ZXDH_RDMA_TRANS_T));
    ZXIC_COMM_MEMSET_S(&dev,sizeof(DPP_DEV_T),0x0,sizeof(DPP_DEV_T));
    ZXIC_COMM_MEMSET_S(key,sizeof(key),0x0,sizeof(key));
    ZXIC_COMM_MEMSET_S(rst,sizeof(rst),0x0,sizeof(rst));

    //从sdt_no中获取hash配置
    dev.device_id = dev_id;
    dev.pcie_channel.slot = slot;
    rc = dpp_hash_get_hash_info_from_sdt(&dev, sdt_no, &hash_entry_cfg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_hash_get_hash_info_from_sdt");

    p_hash_cfg = hash_entry_cfg.p_hash_cfg;
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_hash_cfg);

    pAptCallback = dpp_apt_get_func(&dev, sdt_no);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, pAptCallback);

    hash_entry.p_key = key;
    hash_entry.p_rst = rst;

    p_head_hash_rb = &p_hash_cfg->hash_rb.tn_list;
    p_node = p_head_hash_rb->p_next;
    while(p_node)
    {
        p_rb_tn = (ZXIC_RB_TN *)p_node->data;
        p_node = p_node->next;
        ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_rb_tn);
        p_rbkey = (DPP_HASH_RBKEY_INFO *)p_rb_tn->p_key;
        key_valid = DPP_GET_HASH_KEY_VALID(p_rbkey->key);
        table_id = DPP_GET_HASH_TBL_ID(p_rbkey->key);
        key_type = DPP_GET_HASH_KEY_TYPE(p_rbkey->key);
        if((!key_valid) 
            || (table_id != hash_entry_cfg.table_id)
            || (key_type != hash_entry_cfg.key_type))
        {
            continue;
        }

        ZXIC_COMM_MEMCPY_S(hash_entry.p_key,HASH_KEY_MAX,p_rbkey->key,HASH_KEY_MAX);
        ZXIC_COMM_MEMCPY_S(hash_entry.p_rst,HASH_RST_MAX,p_rbkey->rst,HASH_RST_MAX);
        if((sdt_no>=ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT0)&&(sdt_no<=ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT3))
        {
            rc = pAptCallback->se_func_info.hashFunc.hash_get_func(&l2_entry, &hash_entry);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "hash_get_func");

            ZXIC_COMM_PRINT("slot:%d sdt:%d vqm_vfid:0x%x uni-mac:%02x:%02x:%02x:%02x:%02x:%02x\n",slot, sdt_no, l2_entry.entry.vqm_vfid,
                                l2_entry.key.dmac_addr[0],l2_entry.key.dmac_addr[1],l2_entry.key.dmac_addr[2],
                                l2_entry.key.dmac_addr[3],l2_entry.key.dmac_addr[4],l2_entry.key.dmac_addr[5]); 
        }

        if((sdt_no>=ZXDH_SDT_MC_TABLE_PHYPORT0)&&(sdt_no<=ZXDH_SDT_MC_TABLE_PHYPORT3))
        {
            rc = pAptCallback->se_func_info.hashFunc.hash_get_func(&mc_entry, &hash_entry);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "hash_get_func");

            ZXIC_COMM_PRINT("slot:%d sdt:%d bitmap:0x%llx multi-mac:%02x:%02x:%02x:%02x:%02x:%02x\n",slot, sdt_no, mc_entry.entry.mc_bitmap,
                                        mc_entry.key.mc_mac[0],mc_entry.key.mc_mac[1],mc_entry.key.mc_mac[2],
                                        mc_entry.key.mc_mac[3],mc_entry.key.mc_mac[4],mc_entry.key.mc_mac[5]);
        }  

        if(sdt_no==ZXDH_SDT_RDMA_ENTRY_TABLE)
        {
            rc = pAptCallback->se_func_info.hashFunc.hash_get_func(&rdma_trans, &hash_entry);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "hash_get_func");

            ZXIC_COMM_PRINT("slot:%d sdt:%d vhca_id:0x%x rdma-mac:%02x:%02x:%02x:%02x:%02x:%02x\n",slot, sdt_no, rdma_trans.entry.rdma_vhca_id,
                                rdma_trans.key.mac_addr[0],rdma_trans.key.mac_addr[1],rdma_trans.key.mac_addr[2],
                                rdma_trans.key.mac_addr[3],rdma_trans.key.mac_addr[4],rdma_trans.key.mac_addr[5]); 
        }  
    }

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vqm_vfid_vlan_init(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vqm_vfid_vlan_init(&pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vqm_vfid_vlan_init");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vqm_vfid_vlan_delete(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vqm_vfid_vlan_delete(&pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vqm_vfid_vlan_delete");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vqm_vfid_vlan_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 attr, ZXIC_UINT32 value)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vqm_vfid_vlan_set(&pf_info, attr, value);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vqm_vfid_vlan_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vqm_vfid_vlan_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    ZXDH_VQM_VFID_VLAN_T vqm_vfid_vlan_entry = {0};
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vqm_vfid_vlan_get(&pf_info, &vqm_vfid_vlan_entry);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vqm_vfid_vlan_get");

    ZXIC_COMM_PRINT("-----------------------------------------------\n");
    ZXIC_COMM_PRINT("hit_flag = %u\n", vqm_vfid_vlan_entry.hit_flag);
    ZXIC_COMM_PRINT("%02u sriov_business_vlan_filter = %u\n", VLAN_SRIOV_BUSINESS_VLAN_FILTER, vqm_vfid_vlan_entry.sriov_business_vlan_filter);
    ZXIC_COMM_PRINT("%02u sriov_business_qinq_vlan_strip_offload = %u\n", VLAN_SRIOV_BUSINESS_QINQ_VLAN_STRIP_OFFLOAD, vqm_vfid_vlan_entry.sriov_business_qinq_vlan_strip_offload);
    ZXIC_COMM_PRINT("%02u sriov_business_vlan_strip_offload = %u\n", VLAN_SRIOV_BUSINESS_VLAN_STRIP_OFFLIAD, vqm_vfid_vlan_entry.sriov_business_vlan_strip_offload);
    ZXIC_COMM_PRINT("%02u sriov_business_vlan_tpid = %u\n", VLAN_SRIOV_BUSINESS_VLAN_TPID, vqm_vfid_vlan_entry.sriov_business_vlan_tpid);
    ZXIC_COMM_PRINT("%02u sriov_vlan_tpid = %u\n", VLAN_SRIOV_VLAN_TPID, vqm_vfid_vlan_entry.sriov_vlan_tpid);
    ZXIC_COMM_PRINT("%02u sriov_vlan_tci = %u\n", VLAN_SRIOV_VLAN_TCI, vqm_vfid_vlan_entry.sriov_vlan_tci);
    ZXIC_COMM_PRINT("-----------------------------------------------\n");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_register_info_prt(ZXIC_VOID)
{
    DPP_DEV_T dev = {0};
    DPP_PF_INFO_T pf_info = {0};
    ZXIC_UINT32 print_level = 0;
    ZXIC_UINT32 slot_id = 0;
    ZXIC_UINT32 ep_id = 0;
    ZXIC_UINT32 pf_id = 0;
    ZXIC_UINT32 rc = DPP_OK;
    print_level = zxic_comm_get_print_level();
    zxic_comm_set_print_level(0);
    for (slot_id = 0; slot_id < DPP_PCIE_SLOT_MAX; slot_id++)
    {
        pf_info.slot = slot_id;
        for (ep_id = 0; ep_id < 8; ep_id++)
        {
            for (pf_id = 0; pf_id < 8; pf_id++)
            {
                pf_info.vport = ((ep_id << 12) | (pf_id << 8));
                rc = dpp_dev_get(&pf_info, &dev);
                if (rc == DPP_OK)
                {
                    ZXIC_COMM_PRINT("slot: %u vport: 0x%04x device: %s registered.\n", pf_info.slot, pf_info.vport, pci_name(dev.pcie_channel.device));
                }
            }
        }
    }
    zxic_comm_set_print_level(print_level);
    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_mc_packet_rx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_mc_packet_rx_cnt_get(&pf_info, index, mode, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_mc_packet_rx_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu\n", pkt_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_bc_packet_rx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_bc_packet_rx_cnt_get(&pf_info, index, mode, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_bc_packet_rx_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu\n", pkt_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_1588_packet_rx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_1588_packet_rx_cnt_get(&pf_info, index, mode, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_1588_packet_rx_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu\n", pkt_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_1588_packet_tx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_1588_packet_tx_cnt_get(&pf_info, index, mode, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_1588_packet_tx_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu\n", pkt_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_1588_packet_drop_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_1588_packet_drop_cnt_get(&pf_info, index, mode, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_1588_packet_drop_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu\n", pkt_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_1588_enc_packet_rx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_1588_enc_packet_rx_cnt_get(&pf_info, index, mode, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_1588_enc_packet_rx_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu\n", pkt_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_1588_enc_packet_tx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_1588_enc_packet_tx_cnt_get(&pf_info, index, mode, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_1588_enc_packet_tx_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu\n", pkt_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_spoof_packet_drop_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_spoof_packet_drop_cnt_get(&pf_info, index, mode, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_spoof_packet_drop_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu\n", pkt_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_mcode_packet_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_mcode_packet_cnt_get(&pf_info, index, mode, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_mcode_packet_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu\n", pkt_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_port_RDMA_packet_msg_tx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT64 byte_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_port_RDMA_packet_msg_tx_cnt_get(&pf_info, index, mode, &byte_cnt, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_port_RDMA_packet_msg_tx_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu, byte_cnt: %llu\n", pkt_cnt, byte_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_port_RDMA_packet_msg_rx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT64 byte_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_port_RDMA_packet_msg_rx_cnt_get(&pf_info, index, mode, &byte_cnt, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_port_RDMA_packet_msg_rx_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu, byte_cnt: %llu\n", pkt_cnt, byte_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_plcr_packet_drop_tx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT64 byte_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_plcr_packet_drop_tx_cnt_get(&pf_info, index, mode, &byte_cnt, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_plcr_packet_drop_tx_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu, byte_cnt: %llu\n", pkt_cnt, byte_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_plcr_packet_drop_rx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT64 byte_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_plcr_packet_drop_rx_cnt_get(&pf_info, index, mode, &byte_cnt, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_plcr_packet_drop_rx_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu, byte_cnt: %llu\n", pkt_cnt, byte_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_MTU_packet_msg_tx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT64 byte_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_MTU_packet_msg_tx_cnt_get(&pf_info, index, mode, &byte_cnt, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_MTU_packet_msg_tx_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu, byte_cnt: %llu\n", pkt_cnt, byte_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_MTU_packet_msg_rx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT64 byte_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_MTU_packet_msg_rx_cnt_get(&pf_info, index, mode, &byte_cnt, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_MTU_packet_msg_rx_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu, byte_cnt: %llu\n", pkt_cnt, byte_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_port_uc_packet_rx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT64 byte_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_port_uc_packet_rx_cnt_get(&pf_info, index, mode, &byte_cnt, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_port_uc_packet_rx_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu, byte_cnt: %llu\n", pkt_cnt, byte_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_port_uc_packet_tx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT64 byte_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_port_uc_packet_tx_cnt_get(&pf_info, index, mode, &byte_cnt, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_port_uc_packet_tx_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu, byte_cnt: %llu\n", pkt_cnt, byte_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_port_mc_packet_rx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT64 byte_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_port_mc_packet_rx_cnt_get(&pf_info, index, mode, &byte_cnt, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_port_mc_packet_rx_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu, byte_cnt: %llu\n", pkt_cnt, byte_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_port_mc_packet_tx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT64 byte_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_port_mc_packet_tx_cnt_get(&pf_info, index, mode, &byte_cnt, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_port_mc_packet_tx_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu, byte_cnt: %llu\n", pkt_cnt, byte_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_port_bc_packet_rx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT64 byte_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_port_bc_packet_rx_cnt_get(&pf_info, index, mode, &byte_cnt, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_port_bc_packet_rx_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu, byte_cnt: %llu\n", pkt_cnt, byte_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_port_bc_packet_tx_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT64 byte_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_port_bc_packet_tx_cnt_get(&pf_info, index, mode, &byte_cnt, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_port_bc_packet_tx_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu, byte_cnt: %llu\n", pkt_cnt, byte_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_asn_phyport_rx_pkt_cnt_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_asn_phyport_rx_pkt_cnt_get(&pf_info, index, mode, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_asn_phyport_rx_pkt_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu\n", pkt_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_psn_phyport_tx_pkt_cnt_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_psn_phyport_tx_pkt_cnt_get(&pf_info, index, mode, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_psn_phyport_tx_pkt_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu\n", pkt_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_psn_phyport_rx_pkt_cnt_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_psn_phyport_rx_pkt_cnt_get(&pf_info, index, mode, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_psn_phyport_rx_pkt_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu\n", pkt_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_psn_ack_phyport_tx_pkt_cnt_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_psn_ack_phyport_tx_pkt_cnt_get(&pf_info, index, mode, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_psn_ack_phyport_tx_pkt_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu\n", pkt_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_psn_ack_phyport_rx_pkt_cnt_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index, ZXIC_UINT32 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT64 pkt_cnt = 0;
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_stat_psn_ack_phyport_rx_pkt_cnt_get(&pf_info, index, mode, &pkt_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_psn_ack_phyport_rx_pkt_cnt_get");

    ZXIC_COMM_PRINT("pkt_cnt: %llu\n", pkt_cnt);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_rxfh_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport,
                                                ZXIC_UINT32 qid0, ZXIC_UINT32 qid1,
                                                ZXIC_UINT32 qid2, ZXIC_UINT32 qid3, ZXIC_UINT32 qnum)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 *queue_list = NULL;
    ZXIC_UINT32 group_id = 0;
    ZXIC_UINT32 rc = DPP_OK;

    queue_list = (ZXIC_UINT32*)ZXIC_COMM_MALLOC(256 * sizeof(ZXIC_UINT32));
    ZXIC_COMM_CHECK_POINT(queue_list);

    for (group_id = 0; group_id < RSS_TO_VQID_GROUP_NUM; group_id++)
    {
        queue_list[(group_id * 8) + 0] = qid0;
        queue_list[(group_id * 8) + 1] = qid1;
        queue_list[(group_id * 8) + 2] = qid2;
        queue_list[(group_id * 8) + 3] = qid3;
        queue_list[(group_id * 8) + 4] = qid0;
        queue_list[(group_id * 8) + 5] = qid1;
        queue_list[(group_id * 8) + 6] = qid2;
        queue_list[(group_id * 8) + 7] = qid3;
    }

    rc = dpp_rxfh_set(&pf_info, queue_list, qnum);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_rxfh_set", queue_list);

    ZXIC_COMM_FREE(queue_list);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_rxfh_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_rxfh_del(&pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_rxfh_del");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_rxfh_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 *queue_list = NULL;
    ZXIC_UINT32 group_id = 0;
    ZXIC_UINT32 rc = DPP_OK;

    queue_list = (ZXIC_UINT32*)ZXIC_COMM_MALLOC(256 * sizeof(ZXIC_UINT32));
    ZXIC_COMM_CHECK_POINT(queue_list);

    rc = dpp_rxfh_get(&pf_info, queue_list, 256);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_rxfh_get", queue_list);

    for (group_id = 0; group_id < RSS_TO_VQID_GROUP_NUM; group_id++)
    {
        ZXIC_COMM_PRINT("qid%u: 0x%04x qid%u: 0x%04x qid%u: 0x%04x qid%u: 0x%04x qid%u: 0x%04x qid%u: 0x%04x qid%u: 0x%04x qid%u: 0x%04x\n",
                                                                                              (group_id * 8) + 0, queue_list[(group_id * 8) + 0],
                                                                                              (group_id * 8) + 1, queue_list[(group_id * 8) + 1],
                                                                                              (group_id * 8) + 2, queue_list[(group_id * 8) + 2],
                                                                                              (group_id * 8) + 3, queue_list[(group_id * 8) + 3],
                                                                                              (group_id * 8) + 4, queue_list[(group_id * 8) + 4],
                                                                                              (group_id * 8) + 5, queue_list[(group_id * 8) + 5],
                                                                                              (group_id * 8) + 6, queue_list[(group_id * 8) + 6],
                                                                                              (group_id * 8) + 7, queue_list[(group_id * 8) + 7]);
    }

    ZXIC_COMM_FREE(queue_list);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_thash_key_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, \
                                   ZXIC_UINT32 rsk_031_000, ZXIC_UINT32 rsk_063_032, \
                                   ZXIC_UINT32 rsk_095_064, ZXIC_UINT32 rsk_127_096, \
                                   ZXIC_UINT32 rsk_159_128, ZXIC_UINT32 rsk_191_160, \
                                   ZXIC_UINT32 rsk_223_192, ZXIC_UINT32 rsk_255_224, \
                                   ZXIC_UINT32 rsk_287_256, ZXIC_UINT32 rsk_319_288)
{
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PPU_PPU_COP_THASH_RSK_T ppu_cop_thash_rsk;

    ppu_cop_thash_rsk.rsk_031_000 = rsk_031_000;
    ppu_cop_thash_rsk.rsk_063_032 = rsk_063_032;
    ppu_cop_thash_rsk.rsk_095_064 = rsk_095_064;
    ppu_cop_thash_rsk.rsk_127_096 = rsk_127_096;
    ppu_cop_thash_rsk.rsk_159_128 = rsk_159_128;
    ppu_cop_thash_rsk.rsk_191_160 = rsk_191_160;
    ppu_cop_thash_rsk.rsk_223_192 = rsk_223_192;
    ppu_cop_thash_rsk.rsk_255_224 = rsk_255_224;
    ppu_cop_thash_rsk.rsk_287_256 = rsk_287_256;
    ppu_cop_thash_rsk.rsk_319_288 = rsk_319_288;

    rc = dpp_thash_key_set(&pf_info, (ZXIC_UINT8*)&ppu_cop_thash_rsk, sizeof(DPP_PPU_PPU_COP_THASH_RSK_T));
    ZXIC_COMM_CHECK_RC(rc, "dpp_thash_key_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_thash_key_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_PF_INFO_T pf_info = {slot, vport};
    DPP_PPU_PPU_COP_THASH_RSK_T ppu_cop_thash_rsk = {0};
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_thash_key_get(&pf_info, (ZXIC_UINT8*)&ppu_cop_thash_rsk, sizeof(DPP_PPU_PPU_COP_THASH_RSK_T));
    ZXIC_COMM_CHECK_RC(rc, "dpp_thash_key_get");

    ZXIC_COMM_DBGCNT32_PRINT("rsk_319_288", ppu_cop_thash_rsk.rsk_319_288);
    ZXIC_COMM_DBGCNT32_PRINT("rsk_287_256", ppu_cop_thash_rsk.rsk_287_256);
    ZXIC_COMM_DBGCNT32_PRINT("rsk_255_224", ppu_cop_thash_rsk.rsk_255_224);
    ZXIC_COMM_DBGCNT32_PRINT("rsk_223_192", ppu_cop_thash_rsk.rsk_223_192);
    ZXIC_COMM_DBGCNT32_PRINT("rsk_191_160", ppu_cop_thash_rsk.rsk_191_160);
    ZXIC_COMM_DBGCNT32_PRINT("rsk_159_128", ppu_cop_thash_rsk.rsk_159_128);
    ZXIC_COMM_DBGCNT32_PRINT("rsk_127_096", ppu_cop_thash_rsk.rsk_127_096);
    ZXIC_COMM_DBGCNT32_PRINT("rsk_095_064", ppu_cop_thash_rsk.rsk_095_064);
    ZXIC_COMM_DBGCNT32_PRINT("rsk_063_032", ppu_cop_thash_rsk.rsk_063_032);
    ZXIC_COMM_DBGCNT32_PRINT("rsk_031_000", ppu_cop_thash_rsk.rsk_031_000);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_lag_group_create(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 lag_id)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_lag_group_create(&pf_info, lag_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_lag_group_create");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_lag_group_delete(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 lag_id)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_lag_group_delete(&pf_info, lag_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_lag_group_delete");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_lag_mode_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 lag_id, ZXIC_UINT8 mode)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_lag_mode_set(&pf_info, lag_id, mode);
    ZXIC_COMM_CHECK_RC(rc, "dpp_lag_mode_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_lag_group_hash_factor_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 lag_id, ZXIC_UINT8 factor)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_lag_group_hash_factor_set(&pf_info, lag_id, factor);
    ZXIC_COMM_CHECK_RC(rc, "dpp_lag_group_hash_factor_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_lag_group_member_add(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 lag_id, ZXIC_UINT8 uplink_phy_port_id)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_lag_group_member_add(&pf_info, lag_id, uplink_phy_port_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_lag_group_member_add");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_lag_group_member_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 lag_id, ZXIC_UINT8 uplink_phy_port_id)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_lag_group_member_del(&pf_info, lag_id, uplink_phy_port_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_lag_group_member_del");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_lag_table_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 lag_id)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 sdt_no = ZXDH_SDT_LAG_TABLE;
    ZXIC_UINT32 rc     = DPP_OK;

    ZXDH_LAG_T lag_entry = {0};

    ZXIC_COMM_MEMSET(&lag_entry, 0, sizeof(ZXDH_LAG_T));

    rc = dpp_dev_get(&pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_apt_dtb_eram_get(&dev, queue, sdt_no, lag_id, &lag_entry);
    ZXIC_COMM_CHECK_RC(rc, "dpp_apt_dtb_eram_get");

    ZXIC_COMM_PRINT("-----------------------------------------------\n");
    ZXIC_COMM_PRINT("hit_flag = %u\n", lag_entry.hit_flag);
    ZXIC_COMM_PRINT("member_num = %u\n", lag_entry.member_num);
    ZXIC_COMM_PRINT("bond_mode = %u\n", lag_entry.bond_mode);
    ZXIC_COMM_PRINT("hash_factor = %u\n", lag_entry.hash_factor);
    ZXIC_COMM_PRINT("member_bitmap = %u\n", lag_entry.member_bitmap);
    ZXIC_COMM_PRINT("-----------------------------------------------\n");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_tm_pport_dscp_map_table_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 port, ZXIC_UINT32 dscp_id, ZXIC_UINT32 up_id)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_tm_pport_dscp_map_table_set(&pf_info, port, dscp_id, up_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tm_pport_dscp_map_table_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_tm_pport_dscp_map_table_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 port, ZXIC_UINT32 dscp_id)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_tm_pport_dscp_map_table_del(&pf_info, port, dscp_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tm_pport_dscp_map_table_del");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_tm_pport_dscp_map_table_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 port, ZXIC_UINT32 dscp_id)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 sdt_no = ZXDH_SDT_DSCP_TO_UP_TABLE;
    ZXIC_UINT32 index  = 0x3ff & ((port << 6) | (dscp_id & 0x3f));
    ZXIC_UINT32 rc     = DPP_OK;

    ZXDH_DSCP_TO_UP_T dscp_to_up = {0};

    rc = dpp_dev_get(&pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_apt_dtb_eram_get(&dev, queue, sdt_no, index, &dscp_to_up);
    ZXIC_COMM_CHECK_RC(rc, "dpp_apt_dtb_eram_get");

    ZXIC_COMM_PRINT("-----------------------------------------------\n");
    ZXIC_COMM_PRINT("hit_flag = %u\n", dscp_to_up.hit_flag);
    ZXIC_COMM_PRINT("up = %u\n", dscp_to_up.up);
    ZXIC_COMM_PRINT("-----------------------------------------------\n");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_tm_pport_up_map_table_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 port, ZXIC_UINT32 up_id, ZXIC_UINT32 tc_id)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_tm_pport_up_map_table_set(&pf_info, port, up_id, tc_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tm_pport_up_map_table_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_tm_pport_up_map_table_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 port, ZXIC_UINT32 up_id)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_tm_pport_up_map_table_del(&pf_info, port, up_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tm_pport_up_map_table_del");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_tm_pport_up_map_table_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 port, ZXIC_UINT32 up_id)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 sdt_no = ZXDH_SDT_UP_TO_TC_TABLE;
    ZXIC_UINT32 index  = 0x7F & ((port << 3) | (up_id & 0x7));
    ZXIC_UINT32 rc     = DPP_OK;

    ZXDH_UP_TO_TC_T up_to_tc = {0};

    rc = dpp_dev_get(&pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_apt_dtb_eram_get(&dev, queue, sdt_no, index, &up_to_tc);
    ZXIC_COMM_CHECK_RC(rc, "dpp_apt_dtb_eram_get");

    ZXIC_COMM_PRINT("-----------------------------------------------\n");
    ZXIC_COMM_PRINT("hit_flag = %u\n", up_to_tc.hit_flag);
    ZXIC_COMM_PRINT("tc = %u\n", up_to_tc.tc);
    ZXIC_COMM_PRINT("-----------------------------------------------\n");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_vhca_id_add(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 vhca_id)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_vhca_id_add(&pf_info, vhca_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_vhca_id_add");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_vhca_id_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 vhca_id)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_vhca_id_del(&pf_info, vhca_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_vhca_id_del");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_vhca_id_table_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 vhca_id)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 sdt_no = ZXDH_SDT_VHCA_TABLE;
    ZXIC_UINT32 rc     = DPP_OK;

    ZXDH_VHCA_T vhca_entry = {0};

    ZXIC_COMM_MEMSET(&vhca_entry, 0, sizeof(ZXDH_VHCA_T));

    rc = dpp_dev_get(&pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_apt_dtb_eram_get(&dev, queue, sdt_no, vhca_id, &vhca_entry);
    ZXIC_COMM_CHECK_RC(rc, "dpp_apt_dtb_eram_get");

    ZXIC_COMM_PRINT("-----------------------------------------------\n");
    ZXIC_COMM_PRINT("hit_flag = %u\n", vhca_entry.valid);
    ZXIC_COMM_PRINT("vqm_vfid = %u\n", vhca_entry.vqm_vfid);
    ZXIC_COMM_PRINT("-----------------------------------------------\n");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vport_reset(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vport_reset(&pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_reset");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vlan_filter_init(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_vlan_filter_init(&pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vlan_filter_init");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_add_vlan_filter(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT16 vlan_id)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_add_vlan_filter(&pf_info, vlan_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_add_vlan_filter");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_del_vlan_filter(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT16 vlan_id)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_del_vlan_filter(&pf_info, vlan_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_del_vlan_filter");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_vlan_filter_table_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 vlan_group_id)
{
    DPP_PF_INFO_T pf_info = {slot, vport};

    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue = 0;
    ZXIC_UINT32 sdt_no = ZXDH_SDT_VLAN_FILTER_TABLE;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 rc = DPP_OK;

    ZXDH_VLAN_FILTER_T vlan_filter_entry = {0};

    rc = dpp_dev_get(&pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");
 
    index = ((vlan_group_id << 11) | (VQM_VFID(pf_info.vport)));
    rc = dpp_apt_dtb_eram_get(&dev, queue, sdt_no, index, &vlan_filter_entry);
    ZXIC_COMM_CHECK_RC(rc, "dpp_apt_dtb_eram_get");

    ZXIC_COMM_PRINT("-----------------------------------------------\n");
    ZXIC_COMM_PRINT("hit_flag = %u\n", vlan_filter_entry.hit_flag);
    for (i = 0; i < sizeof(vlan_filter_entry.vport_bitmap); i++)
    {
        ZXIC_COMM_PRINT("vport_bitmap[%u]: 0x%02x\n", i, vlan_filter_entry.vport_bitmap[i]);
    }
    ZXIC_COMM_PRINT("-----------------------------------------------\n");

    return DPP_OK;
}

ZXIC_VOID diag_dpp_fd_cfg_pre1(ZXIC_UINT32 smac, ZXIC_UINT32 dmac, ZXIC_UINT32 sip,
                                   ZXIC_UINT32 dip, ZXIC_UINT32 sport, ZXIC_UINT32 dport)
{
    ZXIC_COMM_MEMSET_S(&g_diag_fd_cfg,sizeof(ZXDH_FD_CFG_T),0x0,sizeof(ZXDH_FD_CFG_T));
    
    g_diag_fd_cfg.key.dmac[0] = 0x00;
    g_diag_fd_cfg.key.dmac[1] = 0x00;
    g_diag_fd_cfg.key.dmac[2] = (dmac>>24)&0xff;
    g_diag_fd_cfg.key.dmac[3] = (dmac>>16)&0xff;
    g_diag_fd_cfg.key.dmac[4] = (dmac>>8)&0xff;
    g_diag_fd_cfg.key.dmac[5] = dmac&0xff;

    g_diag_fd_cfg.key.smac[0] = 0x00;
    g_diag_fd_cfg.key.smac[1] = 0x00;
    g_diag_fd_cfg.key.smac[2] = (smac>>24)&0xff;
    g_diag_fd_cfg.key.smac[3] = (smac>>16)&0xff;
    g_diag_fd_cfg.key.smac[4] = (smac>>8)&0xff;
    g_diag_fd_cfg.key.smac[5] = smac&0xff;

    g_diag_fd_cfg.key.dip[0] = (dip>>24)&0xff;
    g_diag_fd_cfg.key.dip[1] = (dip>>16)&0xff;
    g_diag_fd_cfg.key.dip[2] = (dip>>8)&0xff;
    g_diag_fd_cfg.key.dip[3] = dip&0xff;

    g_diag_fd_cfg.key.sip[0] = (sip>>24)&0xff;
    g_diag_fd_cfg.key.sip[1] = (sip>>16)&0xff;
    g_diag_fd_cfg.key.sip[2] = (sip>>8)&0xff;
    g_diag_fd_cfg.key.sip[3] = sip&0xff;

    g_diag_fd_cfg.key.dport = dport;
    g_diag_fd_cfg.key.sport = sport;

    g_diag_fd_cfg.key.rsv1 = 0xff;
    g_diag_fd_cfg.key.rsv2 = 0xffffffff;
    g_diag_fd_cfg.key.rsv3 = 0xffff;

    g_diag_fd_cfg.mask.rsv1 = 0x0;
    g_diag_fd_cfg.mask.rsv2 = 0x0;
    g_diag_fd_cfg.mask.rsv3 = 0x0;
}

ZXIC_VOID diag_dpp_fd_cfg_pre2(ZXIC_UINT32 ethtype, ZXIC_UINT32 cvlan_pri, ZXIC_UINT32 vlan, 
                                      ZXIC_UINT32 vxlan_vni,ZXIC_UINT32 vqm_vfid)
{
    g_diag_fd_cfg.key.ethtype = ethtype;
    g_diag_fd_cfg.key.cvlan_pri = cvlan_pri;
    g_diag_fd_cfg.key.cvlanid = vlan;
    g_diag_fd_cfg.key.vxlan_vni = vxlan_vni;
    g_diag_fd_cfg.key.vqm_vfid = vqm_vfid;
}

ZXIC_VOID diag_dpp_fd_cfg_pre3(ZXIC_UINT32 action_index, ZXIC_UINT32 action_index2, ZXIC_UINT32 count_id, ZXIC_UINT32 hash_alg, 
                                      ZXIC_UINT32 rss_hash_factor)
{
    g_diag_fd_cfg.as_rlt.hit_flag = 0x00;
    g_diag_fd_cfg.as_rlt.action_index = action_index;
    g_diag_fd_cfg.as_rlt.action_index2 = action_index2;
    g_diag_fd_cfg.as_rlt.count_id = count_id;
    g_diag_fd_cfg.as_rlt.hash_alg = hash_alg;
    g_diag_fd_cfg.as_rlt.rss_hash_factor = rss_hash_factor;
}

ZXIC_VOID diag_dpp_fd_cfg_pre4(ZXIC_UINT32 uplink_fd_id,ZXIC_UINT32 v_qid)
{
    g_diag_fd_cfg.as_rlt.uplink_fd_id = uplink_fd_id;
    g_diag_fd_cfg.as_rlt.v_qid = v_qid;
}
        
ZXIC_UINT32 diag_dpp_fd_cfg_add(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_tbl_fd_cfg_add(&pf_info, ZXDH_SDT_FD_CFG_TABLE, g_diag_fd_index, &g_diag_fd_cfg);
    ZXIC_COMM_CHECK_RC(rc, "dpp_fd_cfg_entry_add");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_fd_cfg_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index)
{
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_tbl_fd_cfg_del(&pf_info, ZXDH_SDT_FD_CFG_TABLE, index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tbl_fd_cfg_del");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_fd_cfg_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index)
{
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_tbl_fd_cfg_get(&pf_info, ZXDH_SDT_FD_CFG_TABLE, index, &g_diag_fd_cfg);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tbl_fd_cfg_get");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_fd_cfg_search(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index)
{
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_tbl_fd_cfg_search(&pf_info, ZXDH_SDT_FD_CFG_TABLE, index, &g_diag_fd_cfg);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tbl_fd_cfg_search");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_fd_acl_index_req(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 index = 0;

    rc = dpp_fd_acl_index_request(&pf_info, &index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_acl_index_request");

    g_diag_fd_index = index;
    ZXIC_COMM_PRINT("reuqest index=%u\n", index);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_fd_acl_index_rel(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index)
{
    DPP_STATUS rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_fd_acl_index_release(&pf_info, index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_fd_acl_index_release");

    ZXIC_COMM_PRINT("slot[%u] vport[0x%x] release index= %u\n", slot, vport, index);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_fd_acl_all_delete(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_STATUS rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_fd_acl_all_delete(&pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_fd_acl_all_delete");

    return DPP_OK;
}

DPP_STATUS diag_dpp_dtb_stat_ppu_cnt_clr(ZXIC_UINT16 slot, 
                                         ZXIC_UINT16 vport, 
                                         ZXIC_UINT32 rd_mode,
                                         ZXIC_UINT32 counter_id,
                                         ZXIC_UINT32 num)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 queue = 0;
    DPP_DEV_T dev = {0};
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_dev_get(&pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_dtb_stat_ppu_cnt_clr(&dev, queue, rd_mode, counter_id, num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_stat_ppu_cnt_clr");

    return DPP_OK;
}

DPP_STATUS diag_dpp_fd_acl_stat_clear(ZXIC_UINT16 slot, 
                                         ZXIC_UINT16 vport)
{
    DPP_STATUS rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_fd_acl_stat_clear(&pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_fd_acl_stat_clear");

    return DPP_OK;
}

#define FD_ACL_INDEX_MAX   (2048)
DPP_STATUS diag_dpp_fd_acl_index_dump(ZXIC_UINT16 slot, 
                                         ZXIC_UINT16 vport)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 index_num = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 *p_index_array = NULL;
    DPP_PF_INFO_T pf_info = {slot, vport};

    p_index_array = ZXIC_COMM_MALLOC(sizeof(ZXIC_UINT32)*FD_ACL_INDEX_MAX);
    ZXIC_COMM_CHECK_POINT(p_index_array);

    rc = dpp_fd_acl_index_dump(&pf_info,p_index_array,&index_num);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_fd_acl_index_dump",p_index_array);

    ZXIC_COMM_PRINT("slot[0x%x] vport[0x%x] index_num=%u\n",slot,vport,index_num);
    for(i=0;i<index_num;i++)
    {
        ZXIC_COMM_PRINT("[%u] ",p_index_array[i]);
        if((i+1)%16==0)
        {
            ZXIC_COMM_PRINT("\n");
        }
    }
    ZXIC_COMM_PRINT("\n");

    ZXIC_COMM_FREE(p_index_array);

    return DPP_OK;
}

ZXIC_VOID diag_dpp_acl_glb_data_prt(ZXIC_VOID)
{
    ZXDH_FD_CFG_T *p_fd_cfg = NULL;
    
    p_fd_cfg = &g_diag_fd_cfg;
    ZXIC_COMM_PRINT("key--smac: 0x%02x:%02x:%02x:%02x:%02x:%02x.\n", 
                               p_fd_cfg->key.smac[0], p_fd_cfg->key.smac[1],
                               p_fd_cfg->key.smac[2], p_fd_cfg->key.smac[3],
                               p_fd_cfg->key.smac[4], p_fd_cfg->key.smac[5]);
    ZXIC_COMM_PRINT("key--dmac: 0x%02x:%02x:%02x:%02x:%02x:%02x.\n", 
                               p_fd_cfg->key.dmac[0], p_fd_cfg->key.dmac[1],
                               p_fd_cfg->key.dmac[2], p_fd_cfg->key.dmac[3],
                               p_fd_cfg->key.dmac[4], p_fd_cfg->key.dmac[5]);
    ZXIC_COMM_PRINT("key--sip: 0x%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:", 
                               p_fd_cfg->key.sip[0], p_fd_cfg->key.sip[1],
                               p_fd_cfg->key.sip[2], p_fd_cfg->key.sip[3],
                               p_fd_cfg->key.sip[4], p_fd_cfg->key.sip[5],
                               p_fd_cfg->key.sip[6], p_fd_cfg->key.sip[7]);
    ZXIC_COMM_PRINT("%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x.\n", 
                               p_fd_cfg->key.sip[8], p_fd_cfg->key.sip[9],
                               p_fd_cfg->key.sip[10], p_fd_cfg->key.sip[11],
                               p_fd_cfg->key.sip[12], p_fd_cfg->key.sip[13],
                               p_fd_cfg->key.sip[14], p_fd_cfg->key.sip[15]);
    ZXIC_COMM_PRINT("key--dip: 0x%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:", 
                               p_fd_cfg->key.dip[0], p_fd_cfg->key.dip[1],
                               p_fd_cfg->key.dip[2], p_fd_cfg->key.dip[3],
                               p_fd_cfg->key.dip[4], p_fd_cfg->key.dip[5],
                               p_fd_cfg->key.dip[6], p_fd_cfg->key.dip[7]);
    ZXIC_COMM_PRINT("%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x.\n", 
                               p_fd_cfg->key.dip[8], p_fd_cfg->key.dip[9],
                               p_fd_cfg->key.dip[10], p_fd_cfg->key.dip[11],
                               p_fd_cfg->key.dip[12], p_fd_cfg->key.dip[13],
                               p_fd_cfg->key.dip[14], p_fd_cfg->key.dip[15]);
    ZXIC_COMM_PRINT("key--ethtype:   0x%04x\n", p_fd_cfg->key.ethtype);
    ZXIC_COMM_PRINT("key--cvlan_pri: 0x%02x\n", p_fd_cfg->key.cvlan_pri);
    ZXIC_COMM_PRINT("key--cvlanid:   0x%04x\n", p_fd_cfg->key.cvlanid);
    ZXIC_COMM_PRINT("key--tos:       0x%02x\n", p_fd_cfg->key.tos);
    ZXIC_COMM_PRINT("key--proto:     0x%02x\n", p_fd_cfg->key.proto);
    ZXIC_COMM_PRINT("key--fragment:  0x%02x\n", p_fd_cfg->key.fragment);
    ZXIC_COMM_PRINT("key--sport:     0x%04x\n", p_fd_cfg->key.sport);
    ZXIC_COMM_PRINT("key--dport:     0x%04x\n", p_fd_cfg->key.dport);
    ZXIC_COMM_PRINT("key--vxlan_vni: 0x%08x\n", p_fd_cfg->key.vxlan_vni);
    ZXIC_COMM_PRINT("key--vqm_vfid:  0x%04x\n", p_fd_cfg->key.vqm_vfid);

    ZXIC_COMM_PRINT("mask--smac: 0x%02x:%02x:%02x:%02x:%02x:%02x.\n", 
                               p_fd_cfg->mask.smac[0], p_fd_cfg->mask.smac[1],
                               p_fd_cfg->mask.smac[2], p_fd_cfg->mask.smac[3],
                               p_fd_cfg->mask.smac[4], p_fd_cfg->mask.smac[5]);
    ZXIC_COMM_PRINT("mask--dmac: 0x%02x:%02x:%02x:%02x:%02x:%02x.\n", 
                               p_fd_cfg->mask.dmac[0], p_fd_cfg->mask.dmac[1],
                               p_fd_cfg->mask.dmac[2], p_fd_cfg->mask.dmac[3],
                               p_fd_cfg->mask.dmac[4], p_fd_cfg->mask.dmac[5]);
    ZXIC_COMM_PRINT("mask--sip: 0x%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:", 
                               p_fd_cfg->mask.sip[0], p_fd_cfg->mask.sip[1],
                               p_fd_cfg->mask.sip[2], p_fd_cfg->mask.sip[3],
                               p_fd_cfg->mask.sip[4], p_fd_cfg->mask.sip[5],
                               p_fd_cfg->mask.sip[6], p_fd_cfg->mask.sip[7]);
    ZXIC_COMM_PRINT("%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x.\n", 
                               p_fd_cfg->mask.sip[8], p_fd_cfg->mask.sip[9],
                               p_fd_cfg->mask.sip[10], p_fd_cfg->mask.sip[11],
                               p_fd_cfg->mask.sip[12], p_fd_cfg->mask.sip[13],
                               p_fd_cfg->mask.sip[14], p_fd_cfg->mask.sip[15]);
    ZXIC_COMM_PRINT("mask--dip: 0x%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:", 
                               p_fd_cfg->mask.dip[0], p_fd_cfg->mask.dip[1],
                               p_fd_cfg->mask.dip[2], p_fd_cfg->mask.dip[3],
                               p_fd_cfg->mask.dip[4], p_fd_cfg->mask.dip[5],
                               p_fd_cfg->mask.dip[6], p_fd_cfg->mask.dip[7]);
    ZXIC_COMM_PRINT("%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x.\n", 
                               p_fd_cfg->mask.dip[8], p_fd_cfg->mask.dip[9],
                               p_fd_cfg->mask.dip[10], p_fd_cfg->mask.dip[11],
                               p_fd_cfg->mask.dip[12], p_fd_cfg->mask.dip[13],
                               p_fd_cfg->mask.dip[14], p_fd_cfg->mask.dip[15]);
    ZXIC_COMM_PRINT("mask--ethtype:   0x%04x\n", p_fd_cfg->mask.ethtype);
    ZXIC_COMM_PRINT("mask--cvlan_pri: 0x%02x\n", p_fd_cfg->mask.cvlan_pri);
    ZXIC_COMM_PRINT("mask--cvlanid:   0x%04x\n", p_fd_cfg->mask.cvlanid);
    ZXIC_COMM_PRINT("mask--tos:       0x%02x\n", p_fd_cfg->mask.tos);
    ZXIC_COMM_PRINT("mask--proto:     0x%02x\n", p_fd_cfg->mask.proto);
    ZXIC_COMM_PRINT("mask--fragment:  0x%02x\n", p_fd_cfg->mask.fragment);
    ZXIC_COMM_PRINT("mask--sport:     0x%04x\n", p_fd_cfg->mask.sport);
    ZXIC_COMM_PRINT("mask--dport:     0x%04x\n", p_fd_cfg->mask.dport);
    ZXIC_COMM_PRINT("mask--vxlan_vni: 0x%08x\n", p_fd_cfg->mask.vxlan_vni);
    ZXIC_COMM_PRINT("mask--vqm_vfid:  0x%04x\n", p_fd_cfg->mask.vqm_vfid);

    ZXIC_COMM_PRINT("rst--hit_flag:       0x%02x\n", p_fd_cfg->as_rlt.hit_flag);
    ZXIC_COMM_PRINT("rst--action_index:   0x%02x\n", p_fd_cfg->as_rlt.action_index);
    ZXIC_COMM_PRINT("rst--action_index2:  0x%02x\n", p_fd_cfg->as_rlt.action_index2);
    ZXIC_COMM_PRINT("rst--v_qid:          0x%04x\n", p_fd_cfg->as_rlt.v_qid);
    ZXIC_COMM_PRINT("rst--uplink_fd_id:   0x%08x\n", p_fd_cfg->as_rlt.uplink_fd_id);
    ZXIC_COMM_PRINT("rst--count_id:       0x%08x\n", p_fd_cfg->as_rlt.count_id);
    ZXIC_COMM_PRINT("rst--hash_alg:       0x%02x\n", p_fd_cfg->as_rlt.hash_alg);
    ZXIC_COMM_PRINT("rst--rss_hash_factor:0x%02x\n", p_fd_cfg->as_rlt.rss_hash_factor);
    ZXIC_COMM_PRINT("rst--spec_port_vfid: 0x%02x\n", p_fd_cfg->as_rlt.spec_port_vfid);

    return;
    
}

DPP_STATUS diag_dpp_se_eram_res_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 index = 0;
    ZXIC_CHAR res_name[32] = "STD_NIC_RES";
    DPP_APT_ERAM_RES_INIT_T tEramResInit = {0};
    DPP_APT_ERAM_TABLE_T *pEramResTemp = NULL;
    DPP_APT_SE_RES_T *p_se_res = NULL;
    DPP_DEV_T dev = {0};
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_dev_get(&pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    ZXIC_COMM_MEMSET_S(&tEramResInit, sizeof(DPP_APT_ERAM_RES_INIT_T), 0x0, sizeof(DPP_APT_ERAM_RES_INIT_T));
    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(&dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(&dev),p_se_res);
    tEramResInit.eram_res = p_se_res->eram_tbl;
    tEramResInit.tbl_num = p_se_res->eram_num;

    ZXIC_COMM_PRINT("#######################[%s] ERAM_NUM[%u]#######################\n",res_name,p_se_res->eram_num);
    for(index=0;index<tEramResInit.tbl_num;index++)
    {
        pEramResTemp = tEramResInit.eram_res + index;
        ZXIC_COMM_CHECK_POINT(pEramResTemp);
        ZXIC_COMM_PRINT("#######################sdt_no=%d#######################\n",pEramResTemp->sdtNo);
        ZXIC_COMM_PRINT("    eram_base_addr=0x%x eram_table_depth=0x%x\n",pEramResTemp->eRamSdt.eram_base_addr,pEramResTemp->eRamSdt.eram_table_depth);
        ZXIC_COMM_PRINT("    eram_mode=%u(0:1bit 1:32bit 2:64bit 3:128bit 4:2bit 5:4bit 6:8bit 7:16bit)\n",pEramResTemp->eRamSdt.eram_mode);
        ZXIC_COMM_PRINT("    opr_mode=%u(0:128bit 1:64bit 2:1bit 3:32bit)\n",pEramResTemp->opr_mode);
    }

    return DPP_OK;
}

DPP_STATUS diag_dpp_se_hash_res_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 zblk_idx[32] = {0};
    ZXIC_UINT32 ddr_en = 0;
    ZXIC_UINT32 array[4] = {0};
    ZXIC_CHAR res_name[32] = "STD_NIC_RES";
    DPP_DEV_T dev = {0};
    DPP_PF_INFO_T pf_info = {slot, vport};

    DPP_APT_HASH_RES_INIT_T tHashResInit = {0};
    DPP_APT_HASH_FUNC_RES_T *pHashFuncTemp = NULL;
    DPP_APT_HASH_BULK_RES_T *pHashBulkTemp = NULL;
    DPP_APT_HASH_TABLE_T *pHashResTemp = NULL;
    DPP_APT_SE_RES_T *p_se_res = NULL;

    rc = dpp_dev_get(&pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");
    dev_id = DEV_ID(&dev);

    ZXIC_COMM_MEMSET_S(&tHashResInit, sizeof(DPP_APT_HASH_RES_INIT_T), 0x0, sizeof(DPP_APT_HASH_RES_INIT_T));
    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(&dev);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id,p_se_res);
    tHashResInit.func_res = p_se_res->hash_func;
    tHashResInit.bulk_res = p_se_res->hash_bulk;
    tHashResInit.tbl_res = p_se_res->hash_tbl;
    tHashResInit.func_num = p_se_res->hash_func_num;
    tHashResInit.bulk_num = p_se_res->hash_bulk_num;
    tHashResInit.tbl_num = p_se_res->hash_tbl_num;
    
    ZXIC_COMM_PRINT("#######################[%s]#######################\n",res_name);
    ZXIC_COMM_PRINT("#######################hash func info#######################\n");
    for(index=0;index<tHashResInit.func_num;index++)
    {
        pHashFuncTemp = tHashResInit.func_res + index;
        ZXIC_COMM_CHECK_POINT(pHashFuncTemp);
        rc = dpp_apt_get_zblock_index(pHashFuncTemp->zblk_bitmap,zblk_idx);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "se_apt_get_zblock_index");
        ddr_en = (0==pHashFuncTemp->ddr_dis)?1:0; 
        array[pHashFuncTemp->func_id%4] = ddr_en;
        ZXIC_COMM_PRINT("[hash%u] ddr_en:%u zblock_num:%u zblk_index:",pHashFuncTemp->func_id,ddr_en,pHashFuncTemp->zblk_num);
        for(i=0;i<pHashFuncTemp->zblk_num;i++)
        {
            ZXIC_COMM_PRINT("%u ",zblk_idx[i]);
        }
        ZXIC_COMM_PRINT("\n");
    }

    ZXIC_COMM_PRINT("#######################hash bulk info#######################\n");
    for(index=0;index<tHashResInit.bulk_num;index++)
    {
        pHashBulkTemp = tHashResInit.bulk_res + index;
        ZXIC_COMM_CHECK_POINT(pHashBulkTemp);
        ZXIC_COMM_PRINT("[hash%u][bulk%u]:zcell_num=%u zreg_num=%u\n",pHashBulkTemp->func_id,pHashBulkTemp->bulk_id,pHashBulkTemp->zcell_num,pHashBulkTemp->zreg_num);
        if(array[pHashBulkTemp->func_id%4])
        {
            ZXIC_COMM_PRINT("    ddr_baddr=0x%x item_num=0x%x width_mode=%u(1:256b 2:512b) crc_sel=%u ecc_en=%u\n",
                           pHashBulkTemp->ddr_baddr,
                           pHashBulkTemp->ddr_item_num,
                           pHashBulkTemp->ddr_width_mode,
                           pHashBulkTemp->ddr_crc_sel,
                           pHashBulkTemp->ddr_ecc_en);
        }
    }

    ZXIC_COMM_PRINT("#######################hash table num[%u]#######################\n",p_se_res->hash_tbl_num);
    ZXIC_COMM_PRINT("------[table_width] 1:128bit 2:256bit 3:512bit-----\n");
    ZXIC_COMM_PRINT("------[rsp_mode] 0:32bit 1:64bit 2:128bit 3:256bit-----\n");
    ZXIC_COMM_PRINT("------[tbl_flag] bit0:age_en bit1:learn_en bit2:mc_write_en-----\n");
    for(index=0;index<tHashResInit.tbl_num;index++)
    {
        pHashResTemp = tHashResInit.tbl_res + index;
        ZXIC_COMM_CHECK_POINT(pHashResTemp);
        ZXIC_COMM_PRINT("[sdt%u][hash%u][table%u]:table_width=%u key_size=%u rsp_mode=%u sdt_parter=0x%x\n",
                           pHashResTemp->sdtNo,
                           pHashResTemp->hashSdt.hash_id,
                           pHashResTemp->hashSdt.hash_table_id,
                           pHashResTemp->hashSdt.hash_table_width,
                           pHashResTemp->hashSdt.key_size,
                           pHashResTemp->hashSdt.rsp_mode,
                           pHashResTemp->sdt_partner);
        ZXIC_COMM_PRINT("   tbl_flag=%u alive=%u alive_baddr=0x%x learn_en=%u\n",
                            pHashResTemp->tbl_flag,
                            pHashResTemp->hashSdt.keep_alive,
                            pHashResTemp->hashSdt.keep_alive_baddr,
                            pHashResTemp->hashSdt.learn_en);
    }

    return DPP_OK;
}

DPP_STATUS diag_dpp_se_acl_res_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_STATUS rc  = DPP_OK;
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 dev_id = 0;
    DPP_APT_ACL_RES_INIT_T tAclResInit = {0};
    DPP_APT_ACL_TABLE_T *pAclResTemp = NULL;
    DPP_APT_SE_RES_T *p_se_res = NULL;
    ZXIC_CHAR res_name[32] = "STD_NIC_RES";
    DPP_DEV_T dev = {0};
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_dev_get(&pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");
    dev_id = DEV_ID(&dev);

    ZXIC_COMM_MEMSET_S(&tAclResInit, sizeof(DPP_APT_ACL_RES_INIT_T), 0x0, sizeof(DPP_APT_ACL_RES_INIT_T));
    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(&dev);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id,p_se_res);
    tAclResInit.acl_res = p_se_res->acl_tbl;
    tAclResInit.tbl_num = p_se_res->acl_num;
    
    ZXIC_COMM_PRINT("#######################%s:ACL table info ACL_NUM[%u]#######################\n",res_name,p_se_res->acl_num);
    ZXIC_COMM_PRINT("------[key_mode]0:640bit 1:320bit 2:160bit 3:80bit------\n");
    ZXIC_COMM_PRINT("------[as_rsp_mode]0:1b 1:32b 2:64b 3:128b 4:2b 5:4b 6:8b 7:16b------\n");
    for(index=0;index<tAclResInit.tbl_num;index++)
    {
        pAclResTemp = tAclResInit.acl_res + index;
        ZXIC_COMM_CHECK_POINT(pAclResTemp);
        ZXIC_COMM_PRINT("[sdt%u][tbl%u]:block_num=%u block_index=",
                        pAclResTemp->sdtNo,
                        pAclResTemp->aclSdt.etcam_table_id,
                        pAclResTemp->aclRes.block_num);
        for(i=0;(i<(pAclResTemp->aclRes.block_num))&&(i<DPP_ETCAM_BLOCK_NUM);i++)
        {
            ZXIC_COMM_PRINT("%u ",pAclResTemp->aclRes.block_index[i]);
        }
        ZXIC_COMM_PRINT("\n");
        ZXIC_COMM_PRINT("   key_mode=%u depth=%u entry_num=%u pri_mode=%u as_en=%u as_eram_baddr=0x%x as_rsp_mode=%u\n",
                            pAclResTemp->aclSdt.etcam_key_mode,
                            pAclResTemp->aclSdt.etcam_table_depth,
                            pAclResTemp->aclRes.entry_num,
                            pAclResTemp->aclRes.pri_mode,
                            pAclResTemp->aclSdt.as_en,
                            pAclResTemp->aclSdt.as_eram_baddr,
                            pAclResTemp->aclSdt.as_rsp_mode);
    }

    return DPP_OK;
}

DPP_STATUS diag_dpp_se_ddr_res_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_STATUS rc  = DPP_OK;
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 dev_id = 0;
    DPP_APT_DDR_RES_INIT_T tDdrResInit = {0};
    DPP_APT_DDR_TABLE_T *pDdrResTemp = NULL;
    DPP_APT_SE_RES_T *p_se_res = NULL;
    ZXIC_CHAR res_name[32] = "STD_NIC_RES";
    DPP_DEV_T dev = {0};
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_dev_get(&pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");
    dev_id = DEV_ID(&dev);

    ZXIC_COMM_MEMSET_S(&tDdrResInit, sizeof(DPP_APT_DDR_RES_INIT_T), 0x0, sizeof(DPP_APT_DDR_RES_INIT_T));

    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(&dev);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id,p_se_res);
    tDdrResInit.ddr_res = p_se_res->ddr_tbl;
    tDdrResInit.tbl_num = p_se_res->ddr_num;

    ZXIC_COMM_PRINT("#######################%s:DDR table info DDR_NUM[%u]#######################\n",res_name,p_se_res->ddr_num);
    for(index=0;index<tDdrResInit.tbl_num;index++)
    {
        pDdrResTemp = tDdrResInit.ddr_res + index;
        ZXIC_COMM_CHECK_POINT(pDdrResTemp);
        ZXIC_COMM_PRINT("[sdt%u]:baddr=0x%x tbl_depth=0x%x rw_len=%u(0:128b 1:256b 2:512b) ecc_en=%u\n",
                           pDdrResTemp->sdtNo,
                           pDdrResTemp->eDdrSdt.ddr3_base_addr,
                           pDdrResTemp->ddr_table_depth,
                           pDdrResTemp->eDdrSdt.ddr3_rw_len,
                           pDdrResTemp->eDdrSdt.ddr3_ecc_en);
    }

    return DPP_OK;
}

DPP_STATUS diag_dpp_se_lpm_res_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_STATUS rc  = DPP_OK;
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 lpm_flags = 0;
    ZXIC_UINT32 as_en = 0;
    ZXIC_UINT32 as_mode = 0;
    ZXIC_UINT32 v4_ddr_en = 0;
    ZXIC_UINT32 v6_ddr_en = 0;
    ZXIC_UINT32 zblk_idx[32] = {0};

    DPP_APT_LPM_RES_INIT_T tLpmResInit = {0};
    DPP_APT_LPM_TABLE_T *pLpmResTemp = NULL;
    DPP_APT_SE_RES_T *p_se_res = NULL;
    DPP_DEV_T dev = {0};
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_dev_get(&pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");
    dev_id = DEV_ID(&dev);

    ZXIC_COMM_MEMSET_S(&tLpmResInit, sizeof(DPP_APT_LPM_RES_INIT_T), 0x0, sizeof(DPP_APT_LPM_RES_INIT_T));
    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(&dev);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id,p_se_res);
    tLpmResInit.lpm_res = p_se_res->lpm_tbl;
    tLpmResInit.glb_res = &p_se_res->lpm_global_res;
    tLpmResInit.tbl_num = p_se_res->lpm_num;

    lpm_flags = tLpmResInit.glb_res->lpm_flags;
    ZXIC_COMM_UINT32_GET_BITS(as_en, lpm_flags, LPM_FLAG_RT_HANDLE_START, LPM_FLAG_RT_HANDLE_WIDTH);
    ZXIC_COMM_UINT32_GET_BITS(v4_ddr_en, lpm_flags, LPM4_FLAG_DDR_EN_START, LPM4_FLAG_DDR_EN_WIDTH);
    ZXIC_COMM_UINT32_GET_BITS(v6_ddr_en, lpm_flags, LPM6_FLAG_DDR_EN_START, LPM6_FLAG_DDR_EN_WIDTH);
    ZXIC_COMM_UINT32_GET_BITS(as_mode, lpm_flags, LPM_FLAG_AS_MODE_START, LPM_FLAG_AS_MODE_WIDTH);

    ZXIC_COMM_PRINT("#######################LPM table info:lpm_flags=%u#######################\n",tLpmResInit.glb_res->lpm_flags);
    if(v4_ddr_en)
    {
        ZXIC_COMM_PRINT("    ddr4_baddr=0x%x  ddr4_item_num=0x%x ecc=%u\n",
                             tLpmResInit.glb_res->ddr4_baddr,tLpmResInit.glb_res->ddr4_item_num,tLpmResInit.glb_res->ddr4_ecc_en);
    }
  
    if(v6_ddr_en)
    {
        ZXIC_COMM_PRINT("    ddr6_baddr=0x%x  ddr6_item_num=0x%x ecc=%u\n",
                             tLpmResInit.glb_res->ddr6_baddr,tLpmResInit.glb_res->ddr6_item_num,tLpmResInit.glb_res->ddr6_ecc_en);
    }

    rc = dpp_apt_get_zblock_index(p_se_res->lpm_global_res.zblk_bitmap,zblk_idx);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "se_apt_get_zblock_index");
    ZXIC_COMM_PRINT("[ipv4&ipv6] share_zblock_num:%u zblk_index:",p_se_res->lpm_global_res.zblk_num);
    for(i=0;i<(p_se_res->lpm_global_res.zblk_num);i++)
    {
            ZXIC_COMM_PRINT("%u ",zblk_idx[i]);
    }
    ZXIC_COMM_PRINT("\n");

    rc = dpp_apt_get_zblock_index(p_se_res->lpm_global_res.mono_ipv4_zblk_bitmap,zblk_idx);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "se_apt_get_zblock_index");
    ZXIC_COMM_PRINT("[ipv4] mono_zblock_num:%u zblk_index:",p_se_res->lpm_global_res.mono_ipv4_zblk_num);
    for(i=0;i<(p_se_res->lpm_global_res.mono_ipv4_zblk_num);i++)
    {
            ZXIC_COMM_PRINT("%u ",zblk_idx[i]);
    }
    ZXIC_COMM_PRINT("\n");

    rc = dpp_apt_get_zblock_index(p_se_res->lpm_global_res.mono_ipv6_zblk_bitmap,zblk_idx);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "se_apt_get_zblock_index");
    ZXIC_COMM_PRINT("[ipv6] mono_zblock_num:%u zblk_index:",p_se_res->lpm_global_res.mono_ipv6_zblk_num);
    for(i=0;i<(p_se_res->lpm_global_res.mono_ipv6_zblk_num);i++)
    {
            ZXIC_COMM_PRINT("%u ",zblk_idx[i]);
    }
    ZXIC_COMM_PRINT("\n");

    for(index=0;index<tLpmResInit.tbl_num;index++)
    {
        pLpmResTemp = tLpmResInit.lpm_res + index;
        ZXIC_COMM_CHECK_POINT(pLpmResTemp);
        ZXIC_COMM_PRINT("[sdt%u]:v46_id=%u(0:ipv6 1:ipv4) rsp_mode=%u(0:32b 1:64b 2:128b 3:256b)\n",
                           pLpmResTemp->sdtNo,
                           pLpmResTemp->lpmSdt.lpm_v46_id,
                           pLpmResTemp->lpmSdt.rsp_mode);
        if(as_en)
        {
            if(as_mode)
            {
                 ZXIC_COMM_PRINT("    as_ddr_baddr=0x%x rsp_mode=%u(0:128b 1:256b) ecc=%u\n",
                             pLpmResTemp->as_ddr_cfg.baddr,pLpmResTemp->as_ddr_cfg.rsp_len,pLpmResTemp->as_ddr_cfg.ecc_en);
            }
            else
            {
                for(i=0;i<DPP_SMMU0_LPM_AS_TBL_ID_NUM;i++)
                {
                    ZXIC_COMM_PRINT("    as_eram_baddr=0x%x rsp_mode=%u(0:1b 1:32b 2:64b 3:128b 4:2b 5:4b 6:8b 7:16b)\n",
                             pLpmResTemp->as_eram_cfg[i].baddr,pLpmResTemp->as_eram_cfg[i].rsp_mode);
                }
            }
        }
    }

    return DPP_OK;
}

DPP_STATUS diag_dpp_se_stat_res_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    DPP_APT_SE_RES_T *p_se_res = NULL;
    DPP_DEV_T dev = {0};
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_dev_get(&pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");
    dev_id = DEV_ID(&dev);

    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(&dev);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id,p_se_res);

    ZXIC_COMM_PRINT("eram_baddr(unit:128bit):0x%x\n",p_se_res->stat_cfg.eram_baddr);
    ZXIC_COMM_PRINT("eram_depth(unit:128bit):0x%x\n",p_se_res->stat_cfg.eram_depth);
    ZXIC_COMM_PRINT("ddr_baddr(unit:2k*256bit):0x%x\n",p_se_res->stat_cfg.ddr_baddr);
    ZXIC_COMM_PRINT("ppu_ddr_offset:0x%x\n",p_se_res->stat_cfg.ppu_ddr_offset);

    return DPP_OK;
}

DPP_STATUS diag_dpp_stat_item_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT16 stat_item_no)
{
    DPP_STATUS rc = DPP_OK;
    DPP_APT_SE_RES_T *p_se_res = NULL;
    DPP_DEV_T dev = {0};
    DPP_PF_INFO_T pf_info = {slot, vport};

    ZXIC_COMM_CHECK_INDEX(stat_item_no,0,STAT_ITEM_MAX_NUM-1);

    rc = dpp_dev_get(&pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");
    
    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(&dev);
    ZXIC_COMM_CHECK_POINT(p_se_res);

    ZXIC_COMM_PRINT("STAT ITEM No. %d:\n", stat_item_no);
    if(p_se_res->stat_item[stat_item_no].valid)
    {
        ZXIC_COMM_PRINT("\tmode: %u(0:64bit 1:128bit)\n", p_se_res->stat_item[stat_item_no].mode);
        ZXIC_COMM_PRINT("\taddr_offset: 0x%x\n", p_se_res->stat_item[stat_item_no].addr_offset);
        ZXIC_COMM_PRINT("\tdepth: 0x%x\n", p_se_res->stat_item[stat_item_no].depth);
    }
    else
    {
        ZXIC_COMM_PRINT("stat item invalid!\n");
    }
    
    return DPP_OK;
}

DPP_STATUS diag_dpp_stat_item_prt_all(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 index = 0;
    DPP_APT_SE_RES_T *p_se_res = NULL;
    DPP_DEV_T dev = {0};
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_dev_get(&pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");
    
    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(&dev);
    ZXIC_COMM_CHECK_POINT(p_se_res);

    if(p_se_res->stat_item_num==0)
    {
        ZXIC_COMM_PRINT("stat item num is 0!\n");
        return DPP_OK;
    }

    for(index=0;index<STAT_ITEM_MAX_NUM;index++)
    {
        if(p_se_res->stat_item[index].valid)
        {
            ZXIC_COMM_PRINT("STAT ITEM No. %d:\n", index);
            ZXIC_COMM_PRINT("\tmode: %u(0:64bit 1:128bit)\n", p_se_res->stat_item[index].mode);
            ZXIC_COMM_PRINT("\taddr_offset: 0x%x\n", p_se_res->stat_item[index].addr_offset);
            ZXIC_COMM_PRINT("\tdepth: 0x%x\n", p_se_res->stat_item[index].depth);
        }
    }
    
    return DPP_OK;
}

ZXIC_VOID diag_dpp_eram_data_stub(ZXIC_UINT32 data0,ZXIC_UINT32 data1,ZXIC_UINT32 data2,ZXIC_UINT32 data3)
{
    g_eram_buff[0] = data0;
    g_eram_buff[1] = data1;
    g_eram_buff[2] = data2;
    g_eram_buff[3] = data3;
    return;
}

DPP_STATUS diag_dpp_eram_entry_insert(ZXIC_UINT16 slot,ZXIC_UINT16 vport,ZXIC_UINT32 sdt_no,ZXIC_UINT32 index)
{
    DPP_STATUS rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_eram_entry_insert(&pf_info,sdt_no,index,(ZXIC_UINT8 *)g_eram_buff);
    ZXIC_COMM_CHECK_RC(rc, "dpp_eram_entry_insert");

    return DPP_OK;
}

DPP_STATUS diag_dpp_eram_entry_delete(ZXIC_UINT16 slot,ZXIC_UINT16 vport,ZXIC_UINT32 sdt_no,ZXIC_UINT32 index)
{
    DPP_STATUS rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_eram_entry_delete(&pf_info,sdt_no,index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_eram_entry_delete");

    return DPP_OK;
}

DPP_STATUS diag_dpp_eram_entry_get(ZXIC_UINT16 slot,ZXIC_UINT16 vport,ZXIC_UINT32 sdt_no,ZXIC_UINT32 index)
{
    DPP_STATUS rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXIC_UINT32 eram_buff[4] = {0};

    ZXIC_COMM_MEMSET_S(eram_buff,sizeof(eram_buff),0,sizeof(eram_buff));
    rc = dpp_eram_entry_get(&pf_info,sdt_no,index,(ZXIC_UINT8 *)eram_buff);
    ZXIC_COMM_CHECK_RC(rc, "dpp_eram_entry_get");

    ZXIC_COMM_PRINT("eram_data: 0x%08x-%08x-%08x-%08x\n",eram_buff[0],eram_buff[1],eram_buff[2],eram_buff[3]);

    return DPP_OK;
}
DPP_STATUS diag_dpp_stat_item_cnt_prt(ZXIC_UINT16 slot,
                                    ZXIC_UINT16 vport, 
                                    ZXIC_UINT32 stat_item_no, 
                                    ZXIC_UINT32 index, 
                                    ZXIC_UINT32 rd_mode)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_stat_item_cnt_get(&pf_info, stat_item_no, index, rd_mode, &stat_value);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_item_cnt_get");

    ZXIC_COMM_PRINT("slot: %u vport: 0x%04x stat_item_no: %u index: %u h64_cnt: %llu l64_cnt: %llu success.\n",
                    slot, vport, stat_item_no, index, stat_value.stat_cnt_128.pkts,stat_value.stat_cnt_128.bytes);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_glb_cfg_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 glb_cfg_data_0, ZXIC_UINT32 glb_cfg_data_1, ZXIC_UINT32 glb_cfg_data_2, ZXIC_UINT32 glb_cfg_data_3)
{
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_glb_cfg_set_0(&pf_info, glb_cfg_data_0);
    ZXIC_COMM_CHECK_RC(rc, "dpp_glb_cfg_set_0");

    rc = dpp_glb_cfg_set_1(&pf_info, glb_cfg_data_1);
    ZXIC_COMM_CHECK_RC(rc, "dpp_glb_cfg_set_1");

    rc = dpp_glb_cfg_set_2(&pf_info, glb_cfg_data_2);
    ZXIC_COMM_CHECK_RC(rc, "dpp_glb_cfg_set_2");

    rc = dpp_glb_cfg_set_3(&pf_info, glb_cfg_data_3);
    ZXIC_COMM_CHECK_RC(rc, "dpp_glb_cfg_set_3");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_glb_cfg_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 mcode_glb_cfg_0 = 0;
    ZXIC_UINT32 mcode_glb_cfg_1 = 0;
    ZXIC_UINT32 mcode_glb_cfg_2 = 0;
    ZXIC_UINT32 mcode_glb_cfg_3 = 0;

    rc = dpp_glb_cfg_get_0(&pf_info, &mcode_glb_cfg_0);
    ZXIC_COMM_CHECK_RC(rc, "dpp_glb_cfg_get_0");

    rc = dpp_glb_cfg_get_1(&pf_info, &mcode_glb_cfg_1);
    ZXIC_COMM_CHECK_RC(rc, "dpp_glb_cfg_get_1");

    rc = dpp_glb_cfg_get_2(&pf_info, &mcode_glb_cfg_2);
    ZXIC_COMM_CHECK_RC(rc, "dpp_glb_cfg_get_2");

    rc = dpp_glb_cfg_get_3(&pf_info, &mcode_glb_cfg_3);
    ZXIC_COMM_CHECK_RC(rc, "dpp_glb_cfg_get_3");

    ZXIC_COMM_PRINT("-----------------------------------------------\n");
    ZXIC_COMM_PRINT("pktrx_mcode_glb_cfg_data0 = 0x%08X\n", mcode_glb_cfg_0);
    ZXIC_COMM_PRINT("pktrx_mcode_glb_cfg_data1 = 0x%08X\n", mcode_glb_cfg_1);
    ZXIC_COMM_PRINT("pktrx_mcode_glb_cfg_data2 = 0x%08X\n", mcode_glb_cfg_2);
    ZXIC_COMM_PRINT("pktrx_mcode_glb_cfg_data3 = 0x%08X\n", mcode_glb_cfg_3); 
    ZXIC_COMM_PRINT("-----------------------------------------------\n");

    return DPP_OK;

}
ZXDH_PKT_CAP_RULE g_rule = {0};

ZXIC_UINT32 diag_dpp_pkt_capture_enable(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXDH_PKT_CAP_POINT capture_pkt_flag)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_pkt_capture_enable(&pf_info, capture_pkt_flag);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pkt_capture_enable");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_pkt_capture_disable(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXDH_PKT_CAP_POINT capture_pkt_flag)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};
    
    rc = dpp_pkt_capture_disable(&pf_info, capture_pkt_flag);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pkt_capture_disable");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_pkt_capture_disable_all(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};
    
    rc = dpp_pkt_capture_disable_all(&pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pkt_capture_disable_all");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_pkt_capture_enable_status_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXDH_PKT_CAP_ENABLE_STATUS enable_status = {0};
    DPP_PF_INFO_T pf_info = {slot, vport};
    
    rc = dpp_pkt_capture_enable_status_get(&pf_info, &enable_status);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pkt_capture_enable_status_get");

    ZXIC_COMM_PRINT("panel_rx_enable_status = %d", enable_status.panel_rx_enable_status);
    ZXIC_COMM_PRINT("panel_tx_enable_status = %d", enable_status.panel_tx_enable_status);
    ZXIC_COMM_PRINT("vqm_rx_enable_status   = %d", enable_status.vqm_rx_enable_status);
    ZXIC_COMM_PRINT("vqm_tx_enable_status   = %d", enable_status.vqm_tx_enable_status);
    ZXIC_COMM_PRINT("rdma_rx_enable_status  = %d", enable_status.rdma_rx_enable_status);
    ZXIC_COMM_PRINT("rdma_tx_enable_status  = %d", enable_status.rdma_tx_enable_status);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_pkt_capture_rule_index_to_tcam_index(ZXIC_UINT32 rule_index, ZXDH_PKT_CAP_MODE rule_mode, ZXDH_PKT_CAP_POINT capture_pkt_flag)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 tcam_index = 0;

    rc = dpp_pkt_capture_rule_index_to_tcam_index(rule_index, rule_mode, capture_pkt_flag, &tcam_index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pkt_capture_rule_index_to_tcam_index");

    ZXIC_COMM_PRINT("rule index = %d, rule mode = %d, cap_point = %d, tcam index = %d", rule_index, rule_mode, capture_pkt_flag, tcam_index);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_pkt_capture_tcam_index_to_rule_index(ZXIC_UINT32 tcam_index)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXDH_PKT_CAP_MODE rule_mode = 0;
    ZXIC_UINT32 rule_index = 0;

    rc = dpp_pkt_capture_tcam_index_to_rule_index(tcam_index, &rule_mode, &rule_index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pkt_capture_tcam_index_to_rule_index");

    ZXIC_COMM_PRINT("tcam index = %d, rule mode = %d, rule index = %d", tcam_index, rule_mode, rule_index);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_pkt_capture_item_l3_set(ZXIC_UINT32 sip_0, ZXIC_UINT32 sip_1, ZXIC_UINT32 sip_2, ZXIC_UINT32 sip_3, \
                                             ZXIC_UINT32 dip_0, ZXIC_UINT32 dip_1, ZXIC_UINT32 dip_2, ZXIC_UINT32 dip_3, \
                                             ZXIC_UINT8 protocol)
{
    ZXIC_UINT32 sip[4] = {sip_0, sip_1, sip_2, sip_3};
    ZXIC_UINT32 dip[4] = {dip_0, dip_1, dip_2, dip_3};

    ZXIC_COMM_MEMCPY(&g_rule.pkt_cap_key.sip, sip, 16);
    ZXIC_COMM_MEMCPY(&g_rule.pkt_cap_key.dip, dip, 16);

    g_rule.pkt_cap_key.protocol = protocol;

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_pkt_capture_item_l2_set(ZXIC_UINT16 dmac_0, ZXIC_UINT32 dmac_1, ZXIC_UINT16 smac_0, ZXIC_UINT32 smac_1, \
                                             ZXIC_UINT16 ethtype)
{
    g_rule.pkt_cap_key.dmac[0] = (dmac_0 >> 8 ) & 0xFF;
    g_rule.pkt_cap_key.dmac[1] = (dmac_0 >> 0 ) & 0xFF;
    g_rule.pkt_cap_key.dmac[2] = (dmac_1 >> 24) & 0xFF;
    g_rule.pkt_cap_key.dmac[3] = (dmac_1 >> 16) & 0xFF;
    g_rule.pkt_cap_key.dmac[4] = (dmac_1 >> 8 ) & 0xFF;
    g_rule.pkt_cap_key.dmac[5] = (dmac_1 >> 0 ) & 0xFF;

    g_rule.pkt_cap_key.smac[0] = (smac_0 >> 8 ) & 0xFF;
    g_rule.pkt_cap_key.smac[1] = (smac_0 >> 0 ) & 0xFF;
    g_rule.pkt_cap_key.smac[2] = (smac_1 >> 24) & 0xFF;
    g_rule.pkt_cap_key.smac[3] = (smac_1 >> 16) & 0xFF;
    g_rule.pkt_cap_key.smac[4] = (smac_1 >> 8 ) & 0xFF;
    g_rule.pkt_cap_key.smac[5] = (smac_1 >> 0 ) & 0xFF;

    g_rule.pkt_cap_key.ethtype = ethtype;

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_pkt_capture_item_l4_set(ZXIC_UINT16 dport, ZXIC_UINT16 sport, ZXIC_UINT32 qp)
{
    g_rule.pkt_cap_key.dport = dport;
    g_rule.pkt_cap_key.sport = sport;

    g_rule.pkt_cap_key.qp = qp;

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_pkt_capture_item_kw_set(ZXIC_UINT32 kw_0, ZXIC_UINT32 kw_1, ZXIC_UINT32 kw_2, ZXIC_UINT32 kw_3, \
                                             ZXIC_UINT16 kw_off, ZXIC_UINT8 kw_len)
{
    ZXIC_UINT32 kw[4] = {kw_0, kw_1, kw_2, kw_3};

    g_rule.pkt_cap_key.key_word_len = kw_len;
    g_rule.pkt_cap_key.key_word_off = kw_off;

    ZXIC_COMM_MEMCPY(&g_rule.pkt_cap_key.key_word, kw, 15);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_pkt_capture_item_insert(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 tcam_index, \
                                             ZXIC_UINT16 rule_config, ZXIC_UINT8 capture_pkt_flag, ZXIC_UINT8 panel_id, \
                                             ZXIC_UINT16 vqm_vfid, ZXIC_UINT16 vhca_id)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};

    g_rule.tcam_index = tcam_index;
    ZXIC_COMM_MEMCPY(&g_rule.rule_config, &rule_config, sizeof(ZXDH_PKT_CAP_NORMAL_CONFIG));
    
    g_rule.dst_vqm_vfid = VQM_VFID(pf_info.vport);

    g_rule.pkt_cap_key.capture_pkt_flag = capture_pkt_flag;
    g_rule.pkt_cap_key.panel_id = panel_id;
    g_rule.pkt_cap_key.vqm_vfid = vqm_vfid;
    g_rule.pkt_cap_key.vhca_id = vhca_id;

    rc = dpp_pkt_capture_item_insert(&pf_info, &g_rule);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pkt_capture_item_insert");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_pkt_capture_item_delete(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 tcam_index)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_pkt_capture_item_delete(&pf_info, tcam_index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pkt_capture_item_delete");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_pkt_capture_table_dump(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 dump_num = 68;
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXDH_PKT_CAP_RULE *p_rule_array = NULL;

    p_rule_array = (ZXDH_PKT_CAP_RULE*)ZXIC_COMM_MALLOC(68 * sizeof(ZXDH_PKT_CAP_RULE));
    ZXIC_COMM_CHECK_POINT(p_rule_array);

    rc = dpp_pkt_capture_table_dump(&pf_info, p_rule_array, &dump_num);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_pkt_capture_table_dump", p_rule_array);

    for (i = 0; i < dump_num; i++)
    {
        ZXIC_COMM_PRINT("rule [%u] tcam_index   = %u\n", i, p_rule_array[i].tcam_index);
        ZXIC_COMM_PRINT("rule [%u] dst_vqm_vfid = %u\n", i, p_rule_array[i].dst_vqm_vfid);
        ZXIC_COMM_PRINT("rule [%u] rule_config.sourceid  = %u\n", i, p_rule_array[i].rule_config.sourceid);
        ZXIC_COMM_PRINT("rule [%u] rule_config.dmac      = %u\n", i, p_rule_array[i].rule_config.dmac);
        ZXIC_COMM_PRINT("rule [%u] rule_config.smac      = %u\n", i, p_rule_array[i].rule_config.smac);
        ZXIC_COMM_PRINT("rule [%u] rule_config.ethtype   = %u\n", i, p_rule_array[i].rule_config.ethtype);
        ZXIC_COMM_PRINT("rule [%u] rule_config.sip       = %u\n", i, p_rule_array[i].rule_config.sip);
        ZXIC_COMM_PRINT("rule [%u] rule_config.dip       = %u\n", i, p_rule_array[i].rule_config.dip);
        ZXIC_COMM_PRINT("rule [%u] rule_config.protocol  = %u\n", i, p_rule_array[i].rule_config.protocol);
        ZXIC_COMM_PRINT("rule [%u] rule_config.sport     = %u\n", i, p_rule_array[i].rule_config.sport);
        ZXIC_COMM_PRINT("rule [%u] rule_config.dport     = %u\n", i, p_rule_array[i].rule_config.dport);
        ZXIC_COMM_PRINT("rule [%u] rule_config.qp        = %u\n", i, p_rule_array[i].rule_config.qp);
        ZXIC_COMM_PRINT("rule [%u] l2_info:\n", i);
        ZXIC_COMM_PRINT("\t dmac:\n");
        ZXIC_COMM_PRINT("\t\t 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", p_rule_array[i].pkt_cap_key.dmac[0], \
                                                                p_rule_array[i].pkt_cap_key.dmac[1], \
                                                                p_rule_array[i].pkt_cap_key.dmac[2], \
                                                                p_rule_array[i].pkt_cap_key.dmac[3], \
                                                                p_rule_array[i].pkt_cap_key.dmac[4], \
                                                                p_rule_array[i].pkt_cap_key.dmac[5]);
        ZXIC_COMM_PRINT("\t smac:\n");
        ZXIC_COMM_PRINT("\t\t 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", p_rule_array[i].pkt_cap_key.smac[0], \
                                                                p_rule_array[i].pkt_cap_key.smac[1], \
                                                                p_rule_array[i].pkt_cap_key.smac[2], \
                                                                p_rule_array[i].pkt_cap_key.smac[3], \
                                                                p_rule_array[i].pkt_cap_key.smac[4], \
                                                                p_rule_array[i].pkt_cap_key.smac[5]);
        ZXIC_COMM_PRINT("\t ethtype:\n");
        ZXIC_COMM_PRINT("\t\t 0x%x\n", p_rule_array[i].pkt_cap_key.ethtype);
        ZXIC_COMM_PRINT("rule [%u] l3_info:\n", i);
        ZXIC_COMM_PRINT("\t sip:\n");
        ZXIC_COMM_PRINT("\t\t 0x%x 0x%x 0x%x 0x%x\n", p_rule_array[i].pkt_cap_key.sip[0], \
                                                      p_rule_array[i].pkt_cap_key.sip[1], \
                                                      p_rule_array[i].pkt_cap_key.sip[2], \
                                                      p_rule_array[i].pkt_cap_key.sip[3]);
        ZXIC_COMM_PRINT("\t\t 0x%x 0x%x 0x%x 0x%x\n", p_rule_array[i].pkt_cap_key.sip[4], \
                                                      p_rule_array[i].pkt_cap_key.sip[5], \
                                                      p_rule_array[i].pkt_cap_key.sip[6], \
                                                      p_rule_array[i].pkt_cap_key.sip[7]);
        ZXIC_COMM_PRINT("\t\t 0x%x 0x%x 0x%x 0x%x\n", p_rule_array[i].pkt_cap_key.sip[8], \
                                                      p_rule_array[i].pkt_cap_key.sip[9], \
                                                      p_rule_array[i].pkt_cap_key.sip[10], \
                                                      p_rule_array[i].pkt_cap_key.sip[11]);
        ZXIC_COMM_PRINT("\t\t 0x%x 0x%x 0x%x 0x%x\n", p_rule_array[i].pkt_cap_key.sip[12], \
                                                      p_rule_array[i].pkt_cap_key.sip[13], \
                                                      p_rule_array[i].pkt_cap_key.sip[14], \
                                                      p_rule_array[i].pkt_cap_key.sip[15]);
        ZXIC_COMM_PRINT("\t dip:\n");
        ZXIC_COMM_PRINT("\t\t 0x%x 0x%x 0x%x 0x%x\n", p_rule_array[i].pkt_cap_key.dip[0], \
                                                      p_rule_array[i].pkt_cap_key.dip[1], \
                                                      p_rule_array[i].pkt_cap_key.dip[2], \
                                                      p_rule_array[i].pkt_cap_key.dip[3]);
        ZXIC_COMM_PRINT("\t\t 0x%x 0x%x 0x%x 0x%x\n", p_rule_array[i].pkt_cap_key.dip[4], \
                                                      p_rule_array[i].pkt_cap_key.dip[5], \
                                                      p_rule_array[i].pkt_cap_key.dip[6], \
                                                      p_rule_array[i].pkt_cap_key.dip[7]);
        ZXIC_COMM_PRINT("\t\t 0x%x 0x%x 0x%x 0x%x\n", p_rule_array[i].pkt_cap_key.dip[8], \
                                                      p_rule_array[i].pkt_cap_key.dip[9], \
                                                      p_rule_array[i].pkt_cap_key.dip[10], \
                                                      p_rule_array[i].pkt_cap_key.dip[11]);
        ZXIC_COMM_PRINT("\t\t 0x%x 0x%x 0x%x 0x%x\n", p_rule_array[i].pkt_cap_key.dip[12], \
                                                      p_rule_array[i].pkt_cap_key.dip[13], \
                                                      p_rule_array[i].pkt_cap_key.dip[14], \
                                                      p_rule_array[i].pkt_cap_key.dip[15]);
        ZXIC_COMM_PRINT("\t protocol:\n");
        ZXIC_COMM_PRINT("\t\t 0x%x\n", p_rule_array[i].pkt_cap_key.protocol);
        ZXIC_COMM_PRINT("rule [%u] l4_info:\n", i);
        ZXIC_COMM_PRINT("\t dport: 0x%x\n", p_rule_array[i].pkt_cap_key.dport);
        ZXIC_COMM_PRINT("\t sport: 0x%x\n", p_rule_array[i].pkt_cap_key.sport);
        ZXIC_COMM_PRINT("rule [%u] qp: 0x%x\n", i, p_rule_array[i].pkt_cap_key.qp);
        ZXIC_COMM_PRINT("rule [%u] pkt_cap_flag: %u\n", i, p_rule_array[i].pkt_cap_key.capture_pkt_flag);
        ZXIC_COMM_PRINT("rule [%u] panel_id: 0x%x\n", i, p_rule_array[i].pkt_cap_key.panel_id);
        ZXIC_COMM_PRINT("rule [%u] vqm_vfid: 0x%x\n", i, p_rule_array[i].pkt_cap_key.vqm_vfid);
        ZXIC_COMM_PRINT("rule [%u] vhca_id: 0x%x\n", i, p_rule_array[i].pkt_cap_key.vhca_id);
        ZXIC_COMM_PRINT("rule [%u] kw_len: 0x%x\n", i, p_rule_array[i].pkt_cap_key.key_word_len);
        ZXIC_COMM_PRINT("rule [%u] kw_off: 0x%x\n", i, p_rule_array[i].pkt_cap_key.key_word_off);
        ZXIC_COMM_PRINT("rule [%u] kw:\n", i);
        ZXIC_COMM_PRINT("\t\t 0x%x 0x%x 0x%x 0x%x\n", p_rule_array[i].pkt_cap_key.key_word[0], \
                                                      p_rule_array[i].pkt_cap_key.key_word[1], \
                                                      p_rule_array[i].pkt_cap_key.key_word[2], \
                                                      p_rule_array[i].pkt_cap_key.key_word[3]);
        ZXIC_COMM_PRINT("\t\t 0x%x 0x%x 0x%x 0x%x\n", p_rule_array[i].pkt_cap_key.key_word[4], \
                                                      p_rule_array[i].pkt_cap_key.key_word[5], \
                                                      p_rule_array[i].pkt_cap_key.key_word[6], \
                                                      p_rule_array[i].pkt_cap_key.key_word[7]);
        ZXIC_COMM_PRINT("\t\t 0x%x 0x%x 0x%x 0x%x\n", p_rule_array[i].pkt_cap_key.key_word[8], \
                                                      p_rule_array[i].pkt_cap_key.key_word[9], \
                                                      p_rule_array[i].pkt_cap_key.key_word[10], \
                                                      p_rule_array[i].pkt_cap_key.key_word[11]);
        ZXIC_COMM_PRINT("\t\t 0x%x 0x%x 0x%x\n",      p_rule_array[i].pkt_cap_key.key_word[12], \
                                                      p_rule_array[i].pkt_cap_key.key_word[13], \
                                                      p_rule_array[i].pkt_cap_key.key_word[14]);
    }

    ZXIC_COMM_FREE(p_rule_array);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_pkt_capture_table_flush(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_pkt_capture_table_flush(&pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pkt_capture_table_flush");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_pkt_capture_speed_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 speed)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_pkt_capture_speed_set(&pf_info, speed);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pkt_capture_speed_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_pkt_capture_speed_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXIC_UINT32 speed = 0;

    rc = dpp_pkt_capture_speed_get(&pf_info, &speed);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pkt_capture_speed_get");

    ZXIC_COMM_PRINT("pkt cap speed is %u\n", speed);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_mcode_feature_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 index)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXIC_UINT64 feature = 0;

    rc = dpp_mcode_feature_get(&pf_info, index, &feature);
    ZXIC_COMM_CHECK_RC(rc, "dpp_mcode_feature_get");

    ZXIC_COMM_PRINT("mcode feature[%d] is 0x%llx\n", index, feature);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_pktrx_mcode_glb_cfg_write(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 start_bit_no, ZXIC_UINT32 end_bit_no,
                                            ZXIC_UINT32 glb_cfg_data_1)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_pktrx_mcode_glb_cfg_write(&pf_info, start_bit_no, end_bit_no, glb_cfg_data_1);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_mcode_glb_cfg_write");

    ZXIC_COMM_PRINT("diag_dpp_pktrx_mcode_glb_cfg_write success\n");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_l2d_psn_cfg_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT8 psn_cfg)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_l2d_psn_cfg_set(&pf_info, psn_cfg);
    ZXIC_COMM_CHECK_RC(rc, "dpp_l2d_psn_cfg_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_l2d_psn_cfg_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXIC_UINT32 psn_cfg = 0;

    rc = dpp_l2d_psn_cfg_get(&pf_info, &psn_cfg);
    ZXIC_COMM_CHECK_RC(rc, "dpp_l2d_psn_cfg_get");

    ZXIC_COMM_PRINT("l2d psn cfg is 0x%x\n", psn_cfg);
    return DPP_OK;
}

ZXIC_VOID diag_dpp_hash_data_stub(ZXIC_UINT32 data0,ZXIC_UINT32 data1,ZXIC_UINT32 data2,ZXIC_UINT32 data3)
{
    g_hash_key_buff[0] = (data0>>24)&0xff;
    g_hash_key_buff[1] = (data0>>16)&0xff;
    g_hash_key_buff[2] = (data0>>8)&0xff;
    g_hash_key_buff[3] = data0&0xff;
    
    g_hash_key_buff[4] = (data1>>24)&0xff;
    g_hash_key_buff[5] = (data1>>16)&0xff;
    g_hash_key_buff[6] = (data1>>8)&0xff;
    g_hash_key_buff[7] = data1&0xff;

    g_hash_rst_buff[0] = (data2>>24)&0xff;
    g_hash_rst_buff[1] = (data2>>16)&0xff;
    g_hash_rst_buff[2] = (data2>>8)&0xff;
    g_hash_rst_buff[3] = data2&0xff;
    
    g_hash_rst_buff[4] = (data3>>24)&0xff;
    g_hash_rst_buff[5] = (data3>>16)&0xff;
    g_hash_rst_buff[6] = (data3>>8)&0xff;
    g_hash_rst_buff[7] = data3&0xff;
    return;
}

DPP_STATUS diag_dpp_hash_entry_insert(ZXIC_UINT16 slot,ZXIC_UINT16 vport,ZXIC_UINT32 sdt_no)
{
    DPP_STATUS rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_hash_entry_insert(&pf_info,sdt_no,g_hash_key_buff,g_hash_rst_buff);
    ZXIC_COMM_CHECK_RC(rc, "dpp_hash_entry_insert");
    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_pktrx_mcode_port_cfg_read(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 port_id)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 data[4] = {0};
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_pktrx_mcode_port_cfg_read(&pf_info, port_id, data);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_mcode_port_cfg_read");

    ZXIC_COMM_PRINT("port_id[0x%08x] ", port_id);
    ZXIC_COMM_PRINT("value[0x%08x 0x%08x 0x%08x 0x%08x]\n", 
    *(data + 0), *(data + 1), *(data + 2), *(data + 3));

    return rc;
}

ZXIC_UINT32 diag_dpp_pktrx_mcode_port_cfg_write(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 port_id, 
                                                ZXIC_UINT32 index, ZXIC_UINT32 start_bit_no, ZXIC_UINT32 end_bit_no, ZXIC_UINT32 port_cfg_data)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_pktrx_mcode_port_cfg_write(&pf_info, port_id, index, start_bit_no, end_bit_no, port_cfg_data);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_mcode_port_cfg_read");
    return rc;
}

DPP_STATUS diag_dpp_hash_entry_delete(ZXIC_UINT16 slot,ZXIC_UINT16 vport,ZXIC_UINT32 sdt_no)
{
    DPP_STATUS rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_hash_entry_delete(&pf_info,sdt_no,g_hash_key_buff);
    ZXIC_COMM_CHECK_RC(rc, "dpp_hash_entry_delete");

    return DPP_OK;
}

DPP_STATUS diag_dpp_hash_entry_get(ZXIC_UINT16 slot,ZXIC_UINT16 vport,ZXIC_UINT32 sdt_no)
{
    DPP_STATUS rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXIC_UINT8 rst_buff[HASH_RST_MAX] = {0};

    ZXIC_COMM_MEMSET_S(rst_buff,sizeof(rst_buff),0,sizeof(rst_buff));
    rc = dpp_hash_entry_get(&pf_info,sdt_no,g_hash_key_buff,(ZXIC_UINT8 *)rst_buff,HASH_SRH_MODE_HDW);
    ZXIC_COMM_CHECK_RC(rc, "dpp_hash_entry_get");

    ZXIC_COMM_PRINT("rst_buff: 0x%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",
                    rst_buff[0],rst_buff[1],rst_buff[2],rst_buff[3],rst_buff[4],rst_buff[5],rst_buff[6],rst_buff[7]);

    return DPP_OK;
}

DPP_STATUS diag_dpp_hash_entry_flush(ZXIC_UINT16 slot,ZXIC_UINT16 vport,ZXIC_UINT32 sdt_no, ZXIC_UINT32 flush_mode)
{
    DPP_STATUS rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};
    HASH_FLUSH_MODE_ENUM mode = flush_mode ? HASH_FLUSH_OFFLINE_MODE : HASH_FLUSH_ONLINE_MODE;

    rc = dpp_hash_entry_flush(&pf_info,sdt_no,mode);
    ZXIC_COMM_CHECK_RC(rc, "dpp_hash_entry_flush");

    return DPP_OK;
}

DPP_STATUS diag_dpp_hash_entry_num_prt(ZXIC_UINT16 slot,ZXIC_UINT16 vport,ZXIC_UINT32 sdt_no)
{
    DPP_STATUS rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXIC_UINT32 item_num = 0;

    rc = dpp_hash_item_num_by_soft(&pf_info, sdt_no, &item_num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_hash_item_num_by_soft");

    ZXIC_COMM_PRINT("slot[%u] sdt[%u] hash item num: %u\n", slot, sdt_no, item_num);
    return DPP_OK;
}

DPP_STATUS diag_dpp_hash_entry_soft_del_by_sdt(ZXIC_UINT16 slot,ZXIC_UINT16 vport,ZXIC_UINT32 sdt_no)
{
    DPP_STATUS rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_hash_entry_soft_delete_by_sdt(&pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_hash_entry_soft_delete_by_sdt");

    ZXIC_COMM_PRINT("slot[%u] sdt[%u] soft delete succ!\n", slot, sdt_no);
    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_dtb_dump_test(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 num,ZXIC_UINT32 flag)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 i  = 0;

    switch (flag)
    {
        case 1:
        {
            for(i=0;i<num;i++)
            {
                rc = diag_dpp_vport_mac_prt(slot,vport);
                ZXIC_COMM_CHECK_RC(rc, "diag_dpp_vport_mac_prt");
            }
            break;
        }
        case 2:
        {
            for(i=0;i<num;i++)
            {
                rc = diag_dpp_vport_mc_mac_prt(slot,vport);
                ZXIC_COMM_CHECK_RC(rc, "diag_dpp_vport_mc_mac_prt");
            }
            break;
        }
        case 3:
        {
            for(i=0;i<num;i++)
            {
                rc = diag_dpp_pkt_capture_table_dump(slot,vport);
                ZXIC_COMM_CHECK_RC(rc, "diag_dpp_pkt_capture_table_dump");
            }
            break;
        }
        case 4:
        {
            for(i=0;i<num;i++)
            {
                rc = diag_dpp_fd_acl_all_delete(slot,vport);
                ZXIC_COMM_CHECK_RC(rc, "diag_dpp_fd_acl_all_delete");
            }
            break;
        }
        default:
        {
            break;
        }
    }
    
    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_rdma_trans_pos_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport,
                                        ZXIC_UINT8 mac0, ZXIC_UINT8 mac1, ZXIC_UINT8 mac2,
                                        ZXIC_UINT8 mac3, ZXIC_UINT8 mac4, ZXIC_UINT8 mac5)
{
    DPP_PF_INFO_T pf_info = {slot, vport};
    DPP_HASH_ZCAM_POS_INFO pos_info = {0};

    ZXIC_UINT8  mac[6];
    ZXIC_UINT32 rc = DPP_OK;

    mac[0] = mac0;
    mac[1] = mac1;
    mac[2] = mac2;
    mac[3] = mac3;
    mac[4] = mac4;
    mac[5] = mac5;

    rc = dpp_get_rdma_trans_item_pos(&pf_info, mac, &pos_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_get_rdma_trans_item_pos");

    ZXIC_COMM_PRINT("entry_addr: 0x%04x entry_pos: %u entry_size: %u.\n", pos_info.entry_addr, pos_info.entry_pos, pos_info.entry_size);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_pktrx_udf_icmp_item_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 port)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_pktrx_udf_icmp_item_set(&pf_info, port);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_udf_icmp_item_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_pktrx_tcam_icmp_item_set(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 mach, ZXIC_UINT32 macl, ZXIC_UINT32 port)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXIC_UINT8 mac[6]     = {(mach >> 8) & 0xFF,  (mach >> 0) & 0xFF, (macl >> 24) & 0xFF,
                             (macl >> 16) & 0xFF, (macl >> 8) & 0xFF, (macl >> 0) & 0xFF};

    rc = dpp_pktrx_tcam_icmp_item_set(&pf_info, mac, port);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_tcam_icmp_item_set");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_tbl_pmtu_info_add(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 qp, ZXIC_UINT32 pmtu)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXDH_PMTU_INFO_T pmtu_info = {0};

    pmtu_info.key.qp = qp;
    pmtu_info.entry.hit_flag = 0;
    pmtu_info.entry.pmtu = pmtu;

    rc = dpp_tbl_pmtu_info_add(&pf_info, ZXDH_SDT_PMTU_INFO_TABLE, &pmtu_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tbl_pmtu_info_add");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_tbl_pmtu_info_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 qp)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXDH_PMTU_INFO_T pmtu_info = {0};

    pmtu_info.key.qp = qp;

    rc = dpp_tbl_pmtu_info_del(&pf_info, ZXDH_SDT_PMTU_INFO_TABLE, &pmtu_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tbl_pmtu_info_del");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_tbl_pmtu_info_srh(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 qp)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXDH_PMTU_INFO_T pmtu_info = {0};

    pmtu_info.key.qp = qp;

    rc = dpp_tbl_pmtu_info_search(&pf_info, ZXDH_SDT_PMTU_INFO_TABLE, &pmtu_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tbl_pmtu_info_search");

    ZXIC_COMM_PRINT("qp = %u, pmtu = %u\n", pmtu_info.key.qp, pmtu_info.entry.pmtu);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_tbl_qp_info_add_ex(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 qp, ZXIC_UINT32 psn,
                                                          ZXIC_UINT32 length, ZXIC_UINT32 va_h, ZXIC_UINT32 va_l)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXDH_QP_INFO_T qp_info = {0};

    qp_info.key.qp = qp;
    qp_info.entry.hit_flag = 0;
    qp_info.entry.psn = psn;
    qp_info.entry.length = length;
    qp_info.entry.va_h = va_h;
    qp_info.entry.va_l = va_l;

    rc = dpp_tbl_qp_info_add_ex(&pf_info, ZXDH_SDT_QP_INFO_TABLE, &qp_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tbl_qp_info_add_ex");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_tbl_qp_info_del_ex(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 qp)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXDH_QP_INFO_T qp_info = {0};

    qp_info.key.qp = qp;

    rc = dpp_tbl_qp_info_del_ex(&pf_info, ZXDH_SDT_QP_INFO_TABLE, &qp_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tbl_qp_info_del_ex");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_np_stat_info_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 phy_port, ZXIC_UINT32 flow_id)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 i = 0;
    DPP_PF_INFO_T pf_info = {slot, vport};
    DPP_AGENT_CHANNEL_STAT_INFO_T stat_info = {0};
    DPP_PRIO_STAT_DATA_T stat_data = {0};

    stat_info.phy_port = phy_port;
    stat_info.flow_id = flow_id;

    rc = dpp_channel_stat_info_get(&pf_info, &stat_info, &stat_data);
    ZXIC_COMM_CHECK_RC(rc, "dpp_channel_stat_info_get");
    for(i = 0; i < 8; i++)
    {
        ZXIC_COMM_PRINT("cos[%u] odma_queue_tx_cnt = 0x%08x\n", i, stat_data.odma_stat_data[i].queue_tx_cnt);
    }

    for(i = 0; i < 8; i++)
    {
        ZXIC_COMM_PRINT("tc[%u] tm_deque_pkt_cnt = 0x%016llx\n", i, stat_data.tm_ets_stat_data[i].deque_pkt_cnt);
        ZXIC_COMM_PRINT("tc[%u] tm_deque_pktB_cnt = 0x%016llx\n", i, stat_data.tm_ets_stat_data[i].deque_pktB_cnt);
    }
    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_idma_phyport_prio_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 mode, ZXIC_UINT32 phy_port)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};
    DPP_STAT_VALUE_U stat_value = {0};
    ZXIC_UINT64 p_pkB_cnt = 0;
    ZXIC_UINT64 p_pk_cnt = 0;
    ZXIC_UINT32 i = 0;

    ZXIC_COMM_PRINT("\n");
    ZXIC_COMM_PRINT("slot: %u, vport: 0x%x, phy_port: %u\n", slot, vport, phy_port);
    ZXIC_COMM_PRINT("+---------+----------------------+----------------------+\n");
    ZXIC_COMM_PRINT("| PRIOcos |    Packet Count     |     Byte Count      |\n");
    ZXIC_COMM_PRINT("+---------+----------------------+----------------------+\n");
    
    for (i = 0; i < 8; i++) 
    {
        rc = dpp_stat_item_cnt_get(&pf_info, DPP_STAT_ITEM_IDMA_PHYPORT_PRIO_STAT, phy_port * 8 + i, mode, &stat_value);
        ZXIC_COMM_CHECK_RC(rc, "dpp_channel_stat_info_get");
        p_pkB_cnt = stat_value.stat_cnt_128.bytes;
        p_pk_cnt = stat_value.stat_cnt_128.pkts;
        ZXIC_COMM_PRINT("|   %4u  | %20llu | %20llu |\n", i, p_pk_cnt, p_pkB_cnt);
    }
    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_stat_odma_phyport_prio_cnt_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 mode, ZXIC_UINT32 phy_port)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};
    DPP_STAT_VALUE_U stat_value = {0};
    ZXIC_UINT64 p_pkB_cnt = 0;
    ZXIC_UINT64 p_pk_cnt = 0;
    ZXIC_UINT32 i = 0;

    ZXIC_COMM_PRINT("\n");
    ZXIC_COMM_PRINT("slot: %u, vport: 0x%x, phy_port: %u\n", slot, vport, phy_port);
    ZXIC_COMM_PRINT("+---------+----------------------+----------------------+\n");
    ZXIC_COMM_PRINT("| PRIOcos |    Packet Count     |     Byte Count      |\n");
    ZXIC_COMM_PRINT("+---------+----------------------+----------------------+\n");
    
    for (i = 0; i < 8; i++) 
    {
        rc = dpp_stat_item_cnt_get(&pf_info, DPP_STAT_ITEM_ODMA_PHYPORT_PRIO_STAT, phy_port * 8 + i, mode, &stat_value);
        ZXIC_COMM_CHECK_RC(rc, "dpp_channel_stat_info_get");
        p_pkB_cnt = stat_value.stat_cnt_128.bytes;
        p_pk_cnt = stat_value.stat_cnt_128.pkts;
        ZXIC_COMM_PRINT("|   %4u  | %20llu | %20llu |\n", i, p_pk_cnt, p_pkB_cnt);
    }
    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_acl_index_max_num_prt(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 max_num = 0;
    DPP_PF_INFO_T pf_info = {slot, vport};
    
    rc = dpp_acl_index_max_num(&pf_info, sdt_no, &max_num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_index_max_num");

    ZXIC_COMM_PRINT("acl index max num: %u\n", max_num);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_acl_index_unused_num(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 unused_num = 0;
    DPP_PF_INFO_T pf_info = {slot, vport};
    
    rc = dpp_acl_index_unused_num(&pf_info, sdt_no, &unused_num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_index_max_num");

    ZXIC_COMM_PRINT("acl index free num: %u\n", unused_num);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_acl_index_request(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 acl_index = 0;
    DPP_PF_INFO_T pf_info = {slot, vport};
    
    rc = dpp_acl_index_request(&pf_info, sdt_no, &acl_index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_index_request");

    ZXIC_COMM_PRINT("request acl index: %u\n", acl_index);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_acl_index_release(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no, ZXIC_UINT32 index)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};
    
    rc = dpp_acl_index_release(&pf_info, sdt_no, index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_index_release");

    ZXIC_COMM_PRINT("release acl index: %u succ!\n", index);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_acl_index_dump(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 index_num = 0;
    ZXIC_UINT32 max_num = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 *p_index_array = NULL;
    DPP_PF_INFO_T pf_info = {slot, vport};
    
    rc = dpp_acl_index_max_num(&pf_info, sdt_no, &max_num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_index_max_num");

    p_index_array = ZXIC_COMM_VMALLOC(max_num * sizeof(ZXIC_UINT32));
    ZXIC_COMM_CHECK_POINT(p_index_array);

    rc = dpp_acl_index_dump(&pf_info, sdt_no, &index_num, p_index_array);
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE(rc, "dpp_acl_index_dump", p_index_array);

    ZXIC_COMM_PRINT("slot[%u] vport[0x%x] sdt[%u] index_num:%u\n",slot, vport, sdt_no, index_num);
    for(i=0; i<index_num; i++)
    {
        ZXIC_COMM_PRINT("index: %u ", p_index_array[i]);
    }

    ZXIC_COMM_VFREE(p_index_array);
    
    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_acl_entry_add(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no,ZXIC_UINT32 entry_num)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 max_num = 0;
    DPP_ACL_ENTRY_INFO_T *p_acl_entry_info = NULL;
    ZXIC_UINT8 key_data[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_UINT8 key_mask[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_UINT8 as_rst[16] = {0};
    ZXIC_UINT8 *p_key_data = NULL;
    ZXIC_UINT8 *p_temp_key_data = NULL;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_acl_index_max_num(&pf_info, sdt_no, &max_num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_index_max_num");

    ZXIC_COMM_CHECK_INDEX(entry_num, 0, max_num);

    p_acl_entry_info = ZXIC_COMM_VMALLOC(max_num * sizeof(DPP_ACL_ENTRY_INFO_T));
    ZXIC_COMM_CHECK_POINT(p_acl_entry_info);
    ZXIC_COMM_MEMSET_S(p_acl_entry_info, max_num * sizeof(DPP_ACL_ENTRY_INFO_T), 0, max_num * sizeof(DPP_ACL_ENTRY_INFO_T));

    p_key_data = ZXIC_COMM_VMALLOC(max_num * sizeof(key_data));
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE(p_key_data, p_acl_entry_info);
    ZXIC_COMM_MEMSET_S(p_key_data, max_num * sizeof(key_data), 0, max_num * sizeof(key_data));
    ZXIC_COMM_MEMSET_S(key_mask, sizeof(key_mask), 0, sizeof(key_mask));
    ZXIC_COMM_MEMSET_S(as_rst, sizeof(as_rst), 0, sizeof(as_rst));

    for(i=0;i<entry_num;i++)
    {
        rc = dpp_acl_index_request(&pf_info, sdt_no, &p_acl_entry_info[i].handle);
        ZXIC_COMM_CHECK_RC_MEMORY_VFREE2PTR_NO_ASSERT(rc, "dpp_acl_index_request",p_acl_entry_info,p_key_data); 

        p_temp_key_data = p_key_data + i * sizeof(key_data);
        p_temp_key_data[0] = 0x11;
        p_temp_key_data[1] = 0x22;
        p_temp_key_data[2] = 0x33;
        p_temp_key_data[3] = 0x44+i;
        as_rst[0] = 0x11;
        as_rst[1] = 0x22;
        as_rst[2] = 0x33;
        as_rst[3] = 0x44;

        p_acl_entry_info[i].key_data =  p_temp_key_data;  
        p_acl_entry_info[i].key_mask =  key_mask;  
        p_acl_entry_info[i].p_as_rslt =  as_rst;     
    }

    rc = dpp_acl_entry_add(&pf_info,sdt_no,entry_num,p_acl_entry_info);
    ZXIC_COMM_VFREE(p_key_data);
    ZXIC_COMM_VFREE(p_acl_entry_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_entry_add"); 

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_acl_entry_del_by_key(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no,ZXIC_UINT32 entry_num)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 max_num = 0;
    ZXIC_UINT32 del_succ_num = 0;
    DPP_ACL_ENTRY_INFO_T *p_acl_entry_info = NULL;
    ZXIC_UINT8 key_data[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_UINT8 key_mask[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_UINT8 as_rst[16] = {0};
    ZXIC_UINT8 *p_key_data = NULL;
    ZXIC_UINT8 *p_temp_key_data = NULL;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_acl_index_max_num(&pf_info, sdt_no, &max_num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_index_max_num");

    ZXIC_COMM_CHECK_INDEX(entry_num, 0, max_num);

    p_acl_entry_info = ZXIC_COMM_VMALLOC(max_num * sizeof(DPP_ACL_ENTRY_INFO_T));
    ZXIC_COMM_CHECK_POINT(p_acl_entry_info);
    ZXIC_COMM_MEMSET_S(p_acl_entry_info, max_num * sizeof(DPP_ACL_ENTRY_INFO_T), 0, max_num * sizeof(DPP_ACL_ENTRY_INFO_T));

    p_key_data = ZXIC_COMM_VMALLOC(max_num * sizeof(key_data));
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE(p_key_data, p_acl_entry_info);
    ZXIC_COMM_MEMSET_S(p_key_data, max_num * sizeof(key_data), 0, max_num * sizeof(key_data));
    ZXIC_COMM_MEMSET_S(key_mask, sizeof(key_mask), 0, sizeof(key_mask));
    ZXIC_COMM_MEMSET_S(as_rst, sizeof(as_rst), 0, sizeof(as_rst));

    for(i=0;i<entry_num;i++)
    {
        p_temp_key_data = p_key_data + i * sizeof(key_data);
        p_temp_key_data[0] = 0x11;
        p_temp_key_data[1] = 0x22;
        p_temp_key_data[2] = 0x33;
        p_temp_key_data[3] = 0x44+i;

        p_acl_entry_info[i].handle = 0xFFFFFFFF;
        p_acl_entry_info[i].key_data =  p_temp_key_data;  
        p_acl_entry_info[i].key_mask =  key_mask;  
        p_acl_entry_info[i].p_as_rslt =  as_rst;     
    }

    rc = dpp_acl_entry_del_by_key(&pf_info,sdt_no,entry_num,p_acl_entry_info);
    for(i=0;i<entry_num;i++)
    {
        if(p_acl_entry_info[i].handle != 0xFFFFFFFF)
        {
            del_succ_num++;
            ZXIC_COMM_PRINT("rel acl index:%u \n", p_acl_entry_info[i].handle);
        }
    }
    ZXIC_COMM_VFREE(p_acl_entry_info);
    ZXIC_COMM_VFREE(p_key_data);
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_entry_del_by_key"); 
    ZXIC_COMM_PRINT("del_succ_num:%u\n", del_succ_num);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_acl_entry_search_by_key(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no,ZXIC_UINT32 entry_num)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 max_num = 0;
    ZXIC_UINT32 srh_succ_num  = 0;
    DPP_ACL_ENTRY_INFO_T *p_acl_entry_info = NULL;
    ZXIC_UINT8 key_data[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_UINT8 key_mask[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_UINT8 as_rst[16] = {0};
    ZXIC_UINT8 *p_key_data = NULL;
    ZXIC_UINT8 *p_temp_key_data = NULL;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_acl_index_max_num(&pf_info, sdt_no, &max_num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_index_max_num");

    ZXIC_COMM_CHECK_INDEX(entry_num, 0, max_num);

    p_acl_entry_info = ZXIC_COMM_VMALLOC(max_num * sizeof(DPP_ACL_ENTRY_INFO_T));
    ZXIC_COMM_CHECK_POINT(p_acl_entry_info);
    ZXIC_COMM_MEMSET_S(p_acl_entry_info, max_num * sizeof(DPP_ACL_ENTRY_INFO_T), 0, max_num * sizeof(DPP_ACL_ENTRY_INFO_T));

    p_key_data = ZXIC_COMM_VMALLOC(max_num * sizeof(key_data));
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE(p_key_data, p_acl_entry_info);
    ZXIC_COMM_MEMSET_S(p_key_data, max_num * sizeof(key_data), 0, max_num * sizeof(key_data));
    ZXIC_COMM_MEMSET_S(key_mask, sizeof(key_mask), 0, sizeof(key_mask));
    ZXIC_COMM_MEMSET_S(as_rst, sizeof(as_rst), 0, sizeof(as_rst));

    for(i=0;i<entry_num;i++)
    {
        p_temp_key_data = p_key_data + i * sizeof(key_data);
        p_temp_key_data[0] = 0x11;
        p_temp_key_data[1] = 0x22;
        p_temp_key_data[2] = 0x33;
        p_temp_key_data[3] = 0x44+i;
  
        p_acl_entry_info[i].handle = 0xFFFFFFFF;
        p_acl_entry_info[i].key_data =  p_temp_key_data;  
        p_acl_entry_info[i].key_mask =  key_mask;  
        p_acl_entry_info[i].p_as_rslt =  as_rst;     
    }

    rc = dpp_acl_entry_search_by_key(&pf_info,sdt_no,entry_num,p_acl_entry_info);
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE2PTR_NO_ASSERT(rc, "dpp_acl_entry_search_by_key",p_acl_entry_info,p_key_data); 

    for(i=0;i<entry_num;i++)
    {
        if(p_acl_entry_info[i].handle != 0xFFFFFFFF)
        {
            srh_succ_num++;
            ZXIC_COMM_PRINT("acl index:%u as_rlt:0x%x-%x-%x-%x \n", p_acl_entry_info[i].handle,
                                  p_acl_entry_info[i].p_as_rslt[0],
                                  p_acl_entry_info[i].p_as_rslt[1],
                                  p_acl_entry_info[i].p_as_rslt[2],
                                  p_acl_entry_info[i].p_as_rslt[3]);
        }
    }
    ZXIC_COMM_VFREE(p_acl_entry_info);
    ZXIC_COMM_VFREE(p_key_data);
    ZXIC_COMM_PRINT("srh_succ_num:%u\n",srh_succ_num);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_acl_entry_del_by_index(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no,ZXIC_UINT32 entry_num)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 max_num = 0;
    ZXIC_UINT32 del_succ_num  = 0;
    DPP_ACL_ENTRY_INFO_T *p_acl_entry_info = NULL;
    ZXIC_UINT8 key_data[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_UINT8 key_mask[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_UINT8 as_rst[16] = {0};
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_acl_index_max_num(&pf_info, sdt_no, &max_num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_index_max_num");

    ZXIC_COMM_CHECK_INDEX(entry_num, 0, max_num);

    p_acl_entry_info = ZXIC_COMM_VMALLOC(max_num * sizeof(DPP_ACL_ENTRY_INFO_T));
    ZXIC_COMM_CHECK_POINT(p_acl_entry_info);
    ZXIC_COMM_MEMSET_S(p_acl_entry_info, max_num * sizeof(DPP_ACL_ENTRY_INFO_T), 0, max_num * sizeof(DPP_ACL_ENTRY_INFO_T));

    for(i=0;i<entry_num;i++)
    {
        p_acl_entry_info[i].handle = i%(entry_num/2);
        p_acl_entry_info[i].key_data =  key_data;  
        p_acl_entry_info[i].key_mask =  key_mask;  
        p_acl_entry_info[i].p_as_rslt =  as_rst;     
    }

    rc = dpp_acl_entry_del_by_index(&pf_info,sdt_no,entry_num,p_acl_entry_info);
    for(i=0;i<entry_num;i++)
    {
        if(p_acl_entry_info[i].handle != 0xFFFFFFFF)
        {
            del_succ_num++;
            ZXIC_COMM_PRINT("rel acl index:%u \n", p_acl_entry_info[i].handle);
        }
    }
    ZXIC_COMM_VFREE(p_acl_entry_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_entry_del_by_index"); 
    ZXIC_COMM_PRINT("del_succ_num:%u\n",del_succ_num);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_acl_entry_search_by_index(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no,ZXIC_UINT32 entry_num)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 max_num = 0;
    ZXIC_UINT32 srh_succ_num  = 0;
    DPP_ACL_ENTRY_INFO_T *p_acl_entry_info = NULL;
    ZXIC_UINT8 key_data[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_UINT8 key_mask[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_UINT8 as_rst[16] = {0};
    ZXIC_UINT8 *p_key_data = NULL;
    ZXIC_UINT8 *p_temp_key_data = NULL;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_acl_index_max_num(&pf_info, sdt_no, &max_num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_index_max_num");

    ZXIC_COMM_CHECK_INDEX(entry_num, 0, max_num);

    p_acl_entry_info = ZXIC_COMM_VMALLOC(max_num * sizeof(DPP_ACL_ENTRY_INFO_T));
    ZXIC_COMM_CHECK_POINT(p_acl_entry_info);
    ZXIC_COMM_MEMSET_S(p_acl_entry_info, max_num * sizeof(DPP_ACL_ENTRY_INFO_T), 0, max_num * sizeof(DPP_ACL_ENTRY_INFO_T));

    p_key_data = ZXIC_COMM_VMALLOC(max_num * sizeof(key_data));
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE(p_key_data, p_acl_entry_info);
    ZXIC_COMM_MEMSET_S(p_key_data, max_num * sizeof(key_data), 0, max_num * sizeof(key_data));
    ZXIC_COMM_MEMSET_S(key_mask, sizeof(key_mask), 0, sizeof(key_mask));
    ZXIC_COMM_MEMSET_S(as_rst, sizeof(as_rst), 0, sizeof(as_rst));

    for(i=0;i<entry_num;i++)
    {
        p_temp_key_data = p_key_data + i * sizeof(key_data);
        p_acl_entry_info[i].handle = i%(entry_num/2);
        p_acl_entry_info[i].key_data =  p_temp_key_data;  
        p_acl_entry_info[i].key_mask =  key_mask;  
        p_acl_entry_info[i].p_as_rslt =  as_rst;     
    }

    rc = dpp_acl_entry_search_by_index(&pf_info,sdt_no,entry_num,p_acl_entry_info);
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE2PTR_NO_ASSERT(rc, "dpp_acl_entry_search_by_key",p_acl_entry_info,p_key_data); 

    for(i=0;i<entry_num;i++)
    {
        if(p_acl_entry_info[i].handle != 0xFFFFFFFF)
        {
            srh_succ_num++;
            ZXIC_COMM_PRINT("acl index:%u as_rlt:0x%x-%x-%x-%x \n", p_acl_entry_info[i].handle,
                                  p_acl_entry_info[i].p_as_rslt[0],
                                  p_acl_entry_info[i].p_as_rslt[1],
                                  p_acl_entry_info[i].p_as_rslt[2],
                                  p_acl_entry_info[i].p_as_rslt[3]);
        }
    }
    ZXIC_COMM_VFREE(p_acl_entry_info);
    ZXIC_COMM_VFREE(p_key_data);
    ZXIC_COMM_PRINT("srh_succ_num:%u\n",srh_succ_num);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_acl_entry_dump_by_vport(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 max_num = 0;
    ZXIC_UINT32 entry_num = 0;
    ZXIC_UINT32 valid_num  = 0;
    ZXIC_UINT32 len = 0;
    DPP_ACL_ENTRY_INFO_T *p_acl_entry_info = NULL;
    ZXIC_UINT8 key_len = 80;
    ZXIC_UINT8 rst_len = 16;
    ZXIC_UINT8 *p_key_data = NULL;
    ZXIC_UINT8 *p_key_mask = NULL;
    ZXIC_UINT8 *p_as_rst = NULL;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_acl_index_max_num(&pf_info, sdt_no, &max_num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_index_max_num");

    p_acl_entry_info = ZXIC_COMM_VMALLOC(max_num * sizeof(DPP_ACL_ENTRY_INFO_T));
    ZXIC_COMM_CHECK_POINT(p_acl_entry_info);
    ZXIC_COMM_MEMSET_S(p_acl_entry_info, max_num * sizeof(DPP_ACL_ENTRY_INFO_T), 0, max_num * sizeof(DPP_ACL_ENTRY_INFO_T));

    len = max_num * (key_len*2+rst_len);
    p_key_data = ZXIC_COMM_VMALLOC(len);
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE(p_acl_entry_info, p_acl_entry_info);
    ZXIC_COMM_MEMSET_S(p_key_data, len, 0, len);
    p_key_mask = p_key_data + max_num * key_len;
    p_as_rst = p_key_mask + max_num * key_len;

    for(i=0;i<max_num;i++)
    {
        p_acl_entry_info[i].key_data =  p_key_data+i*key_len;  
        p_acl_entry_info[i].key_mask =  p_key_mask+i*key_len;  
        p_acl_entry_info[i].p_as_rslt =  p_as_rst+i*rst_len;     
    }

    rc = dpp_acl_entry_dump_by_vport(&pf_info, sdt_no, &entry_num, p_acl_entry_info);
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE2PTR_NO_ASSERT(rc, "dpp_acl_entry_dump_by_vport",p_acl_entry_info,p_key_data); 

    for(i=0;i<entry_num;i++)
    {
        if(p_acl_entry_info[i].handle != 0xFFFFFFFF)
        {
            valid_num++;
            ZXIC_COMM_PRINT("acl index:%u key_data:0x%02x-%02x-%02x-%02x as_rlt:0x%02x-%02x-%02x-%02x \n", p_acl_entry_info[i].handle,
                                  p_acl_entry_info[i].key_data[0],
                                  p_acl_entry_info[i].key_data[1],
                                  p_acl_entry_info[i].key_data[2],
                                  p_acl_entry_info[i].key_data[3],
                                  p_acl_entry_info[i].p_as_rslt[0],
                                  p_acl_entry_info[i].p_as_rslt[1],
                                  p_acl_entry_info[i].p_as_rslt[2],
                                  p_acl_entry_info[i].p_as_rslt[3]);
        }
    }

    ZXIC_COMM_VFREE(p_acl_entry_info);
    ZXIC_COMM_VFREE(p_key_data);

    ZXIC_COMM_PRINT("slot[%u] vport[0x%x] dump entry num:%u valid entry num:%u\n",
                     slot,vport,entry_num,valid_num);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_acl_entry_dump_all(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 max_num = 0;
    ZXIC_UINT32 entry_num = 0;
    ZXIC_UINT32 valid_num  = 0;
    ZXIC_UINT32 len = 0;
    DPP_ACL_ENTRY_INFO_T *p_acl_entry_info = NULL;
    ZXIC_UINT8 key_len = 80;
    ZXIC_UINT8 rst_len = 16;
    ZXIC_UINT8 *p_key_data = NULL;
    ZXIC_UINT8 *p_key_mask = NULL;
    ZXIC_UINT8 *p_as_rst = NULL;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_acl_index_max_num(&pf_info, sdt_no, &max_num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_index_max_num");

    p_acl_entry_info = ZXIC_COMM_VMALLOC(max_num * sizeof(DPP_ACL_ENTRY_INFO_T));
    ZXIC_COMM_CHECK_POINT(p_acl_entry_info);
    ZXIC_COMM_MEMSET_S(p_acl_entry_info, max_num * sizeof(DPP_ACL_ENTRY_INFO_T), 0, max_num * sizeof(DPP_ACL_ENTRY_INFO_T));

    len = max_num * (key_len*2+rst_len);
    p_key_data = ZXIC_COMM_VMALLOC(len);
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE(p_acl_entry_info, p_acl_entry_info);
    ZXIC_COMM_MEMSET_S(p_key_data, len, 0, len);
    p_key_mask = p_key_data + max_num * key_len;
    p_as_rst = p_key_mask + max_num * key_len;

    for(i=0;i<max_num;i++)
    {
        p_acl_entry_info[i].key_data =  p_key_data+i*key_len;  
        p_acl_entry_info[i].key_mask =  p_key_mask+i*key_len;  
        p_acl_entry_info[i].p_as_rslt =  p_as_rst+i*rst_len;     
    }

    rc = dpp_acl_entry_dump_all(&pf_info, sdt_no, &entry_num, p_acl_entry_info);
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE2PTR_NO_ASSERT(rc, "dpp_acl_entry_dump_all",p_acl_entry_info,p_key_data); 

    for(i=0;i<entry_num;i++)
    {
        if(p_acl_entry_info[i].handle != 0xFFFFFFFF)
        {
            valid_num++;
            ZXIC_COMM_PRINT("acl index:%u key_data:0x%02x-%02x-%02x-%02x as_rlt:0x%02x-%02x-%02x-%02x \n", p_acl_entry_info[i].handle,
                                  p_acl_entry_info[i].key_data[0],
                                  p_acl_entry_info[i].key_data[1],
                                  p_acl_entry_info[i].key_data[2],
                                  p_acl_entry_info[i].key_data[3],
                                  p_acl_entry_info[i].p_as_rslt[0],
                                  p_acl_entry_info[i].p_as_rslt[1],
                                  p_acl_entry_info[i].p_as_rslt[2],
                                  p_acl_entry_info[i].p_as_rslt[3]);
        }
    }

    ZXIC_COMM_VFREE(p_acl_entry_info);
    ZXIC_COMM_VFREE(p_key_data);

    ZXIC_COMM_PRINT("slot[%u] vport[0x%x] dump entry num:%u valid entry num:%u\n",
                     slot,vport,entry_num,valid_num);

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_acl_entry_flush_by_vport(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_acl_entry_flush_by_vport(&pf_info,sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_entry_flush_by_vport");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_acl_entry_flush_all(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_acl_entry_flush_all(&pf_info,sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_entry_flush_all");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_acl_entry_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no,ZXIC_UINT32 handle)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};

    rc = dpp_acl_entry_del(&pf_info,sdt_no,handle);
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_entry_del");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_acl_entry_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 sdt_no,ZXIC_UINT32 handle)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {slot, vport};
    DPP_ACL_ENTRY_INFO_T acl_entry_info = {0};
    ZXIC_UINT8 key_data[80] = {0};
    ZXIC_UINT8 key_mask[80] = {0};
    ZXIC_UINT8 as_rst[16] = {0};
    
    acl_entry_info.handle = handle;
    acl_entry_info.key_data = key_data;
    acl_entry_info.key_mask = key_mask;
    acl_entry_info.p_as_rslt = as_rst;

    rc = dpp_acl_entry_get(&pf_info,sdt_no,&acl_entry_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_entry_get");

    ZXIC_COMM_PRINT("acl index=%u\n",acl_entry_info.handle);
    ZXIC_COMM_PRINT("key_data=0x%02x-%02x-%02x-%02x\n",key_data[0],key_data[1],key_data[2],key_data[3]);
    ZXIC_COMM_PRINT("key_mask=0x%02x-%02x-%02x-%02x\n",key_mask[0],key_mask[1],key_mask[2],key_mask[3]);
    ZXIC_COMM_PRINT("as_rst=0x%02x-%02x-%02x-%02x\n",as_rst[0],as_rst[1],as_rst[2],as_rst[3]);

    return DPP_OK;
}
ZXIC_UINT32 diag_dpp_roce_match_add(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 ip, ZXIC_UINT32 vqm_vfid)
{
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXIC_UINT32 rc = DPP_OK;
    ZXDH_ROCE_MATCH_T roce_match = {0};

    ZXIC_COMM_MEMSET_S(&roce_match, sizeof(ZXDH_ROCE_MATCH_T), 0x0, sizeof(ZXDH_ROCE_MATCH_T));
    roce_match.key.sip[0] = (ip>>24)&0xff;
    roce_match.key.sip[1] = (ip>>16)&0xff;
    roce_match.key.sip[2] = (ip>>8)&0xff;
    roce_match.key.sip[3] = ip&0xff;
    roce_match.key.dip[0] = ((ip>>24)&0xff) + 1;
    roce_match.key.dip[1] = ((ip>>16)&0xff) + 1;
    roce_match.key.dip[2] = ((ip>>8)&0xff) + 1;
    roce_match.key.dip[3] = (ip&0xff) + 1;
    roce_match.entry.vqm_vfid = vqm_vfid;

    rc = dpp_tbl_roce_match_add(&pf_info, ZXDH_SDT_ROCE_MATCH_TABLE, &roce_match);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tbl_roce_match_add");
    
    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_roce_match_del(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 ip)
{
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXIC_UINT32 rc = DPP_OK;
    ZXDH_ROCE_MATCH_T roce_match = {0};

    ZXIC_COMM_MEMSET_S(&roce_match, sizeof(ZXDH_ROCE_MATCH_T), 0x0, sizeof(ZXDH_ROCE_MATCH_T));
    roce_match.key.sip[0] = (ip>>24)&0xff;
    roce_match.key.sip[1] = (ip>>16)&0xff;
    roce_match.key.sip[2] = (ip>>8)&0xff;
    roce_match.key.sip[3] = ip&0xff;
    roce_match.key.dip[0] = ((ip>>24)&0xff) + 1;
    roce_match.key.dip[1] = ((ip>>16)&0xff) + 1;
    roce_match.key.dip[2] = ((ip>>8)&0xff) + 1;
    roce_match.key.dip[3] = (ip&0xff) + 1;

    rc = dpp_tbl_roce_match_del(&pf_info, ZXDH_SDT_ROCE_MATCH_TABLE, &roce_match);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tbl_roce_match_del");

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_roce_match_get(ZXIC_UINT16 slot, ZXIC_UINT16 vport, ZXIC_UINT32 ip)
{
    DPP_PF_INFO_T pf_info = {slot, vport};
    ZXIC_UINT32 rc = DPP_OK;
    ZXDH_ROCE_MATCH_T roce_match = {0};

    ZXIC_COMM_MEMSET_S(&roce_match, sizeof(ZXDH_ROCE_MATCH_T), 0x0, sizeof(ZXDH_ROCE_MATCH_T));
    roce_match.key.sip[0] = (ip>>24)&0xff;
    roce_match.key.sip[1] = (ip>>16)&0xff;
    roce_match.key.sip[2] = (ip>>8)&0xff;
    roce_match.key.sip[3] = ip&0xff;
    roce_match.key.dip[0] = ((ip>>24)&0xff) + 1;
    roce_match.key.dip[1] = ((ip>>16)&0xff) + 1;
    roce_match.key.dip[2] = ((ip>>8)&0xff) + 1;
    roce_match.key.dip[3] = (ip&0xff) + 1;
    rc = dpp_tbl_roce_match_search(&pf_info, ZXDH_SDT_ROCE_MATCH_TABLE, &roce_match);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tbl_roce_match_search");

    ZXIC_COMM_PRINT("hit_flag    = %u\n",roce_match.entry.hit_flag);
    ZXIC_COMM_PRINT("vqm_vfid    = 0x%x\n",roce_match.entry.vqm_vfid);

    return DPP_OK;
}
