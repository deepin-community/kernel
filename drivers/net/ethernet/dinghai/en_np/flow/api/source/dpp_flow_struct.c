#include "dpp_flow_comm.h"
#include "dpp_tbl_pmtu_info.h"
#include "dpp_tbl_qp_info.h"
#include "dpp_tbl_roce_match.h"
#include "dpp_tbl_fd_cfg.h"
ZXDH_FLOW_ATTR_FIELD_T g_pmtu_info_fields[] = 
{
    {"rsv0", DPP_ATTR_FLAG_KEY, 1, 4, 63, 8},
    {"rsv1", DPP_ATTR_FLAG_KEY, 1, 4, 55, 32},
    {"qp", DPP_ATTR_FLAG_KEY, 1, 4, 23, 24},
    {"hit_flag", DPP_ATTR_FLAG_RST, 1, 4, 31, 1},
    {"rsv0", DPP_ATTR_FLAG_RST, 1, 4, 30, 15},
    {"pmtu", DPP_ATTR_FLAG_RST, 1, 4, 15, 16},
};

ZXDH_FLOW_ATTR_FIELD_T g_qp_info_fields[] = 
{
    {"rsv0", DPP_ATTR_FLAG_KEY, 1, 4, 63, 8},
    {"rsv1", DPP_ATTR_FLAG_KEY, 1, 4, 55, 32},
    {"qp", DPP_ATTR_FLAG_KEY, 1, 4, 23, 24},
    {"hit_flag", DPP_ATTR_FLAG_RST, 1, 4, 127, 1},
    {"rsv0", DPP_ATTR_FLAG_RST, 1, 4, 126, 7},
    {"psn", DPP_ATTR_FLAG_RST, 1, 4, 119, 24},
    {"length", DPP_ATTR_FLAG_RST, 1, 4, 95, 32},
    {"rsv1", DPP_ATTR_FLAG_RST, 1, 4, 63, 16},
    {"va_h", DPP_ATTR_FLAG_RST, 1, 4, 47, 16},
    {"va_l", DPP_ATTR_FLAG_RST, 1, 4, 31, 32},
};
ZXDH_FLOW_ATTR_FIELD_T g_roce_match_fields[] = 
{
    {"sip", DPP_ATTR_FLAG_KEY, 16, 1, 287, 128},
    {"dip", DPP_ATTR_FLAG_KEY, 16, 1, 159, 128},
    {"rsv", DPP_ATTR_FLAG_KEY, 1, 4, 31, 32},
    {"hit_flag", DPP_ATTR_FLAG_RST, 1, 4, 31, 1},
    {"rsv0", DPP_ATTR_FLAG_RST, 1, 4, 30, 15},
    {"vqm_vfid", DPP_ATTR_FLAG_RST, 1, 4, 15, 16},
};

ZXDH_FLOW_ATTR_FIELD_T g_fd_cfg_fields[] = 
{
    {"dmac", DPP_ATTR_FLAG_KEY, 6, 1, 639, 48},
    {"smac", DPP_ATTR_FLAG_KEY, 6, 1, 591, 48},
    {"ethtype", DPP_ATTR_FLAG_KEY, 1, 4, 543, 16},
    {"cvlan_pri", DPP_ATTR_FLAG_KEY, 1, 2, 527, 4},
    {"cvlanid", DPP_ATTR_FLAG_KEY, 1, 2, 523, 12},
    {"sip", DPP_ATTR_FLAG_KEY, 16, 1, 511, 128},
    {"dip", DPP_ATTR_FLAG_KEY, 16, 1, 383, 128},
    {"rsv1", DPP_ATTR_FLAG_KEY, 1, 1, 255, 8},
    {"tos", DPP_ATTR_FLAG_KEY, 1, 1, 247, 8},
    {"proto", DPP_ATTR_FLAG_KEY, 1, 1, 239, 8},
    {"fragment", DPP_ATTR_FLAG_KEY, 1, 1, 231, 8},
    {"sport", DPP_ATTR_FLAG_KEY, 1, 2, 223, 16},
    {"dport", DPP_ATTR_FLAG_KEY, 1, 2, 207, 16},
    {"rsv2", DPP_ATTR_FLAG_KEY, 1, 4, 191, 8},
    {"vxlan_vni", DPP_ATTR_FLAG_KEY, 1, 4, 183, 24},
    {"vqm_vfid", DPP_ATTR_FLAG_KEY, 1, 2, 159, 16},
    {"rsv3", DPP_ATTR_FLAG_KEY, 1, 2, 143, 16},
    {"dmac", DPP_ATTR_FLAG_MASK, 6, 1, 639, 48},
    {"smac", DPP_ATTR_FLAG_MASK, 6, 1, 591, 48},
    {"ethtype", DPP_ATTR_FLAG_MASK, 1, 4, 543, 16},
    {"cvlan_pri", DPP_ATTR_FLAG_MASK, 1, 2, 527, 4},
    {"cvlanid", DPP_ATTR_FLAG_MASK, 1, 2, 523, 12},
    {"sip", DPP_ATTR_FLAG_MASK, 16, 1, 511, 128},
    {"dip", DPP_ATTR_FLAG_MASK, 16, 1, 383, 128},
    {"rsv1", DPP_ATTR_FLAG_MASK, 1, 1, 255, 8},
    {"tos", DPP_ATTR_FLAG_MASK, 1, 1, 247, 8},
    {"proto", DPP_ATTR_FLAG_MASK, 1, 1, 239, 8},
    {"fragment", DPP_ATTR_FLAG_MASK, 1, 1, 231, 8},
    {"sport", DPP_ATTR_FLAG_MASK, 1, 2, 223, 16},
    {"dport", DPP_ATTR_FLAG_MASK, 1, 2, 207, 16},
    {"rsv2", DPP_ATTR_FLAG_MASK, 1, 4, 191, 8},
    {"vxlan_vni", DPP_ATTR_FLAG_MASK, 1, 4, 183, 24},
    {"vqm_vfid", DPP_ATTR_FLAG_MASK, 1, 2, 159, 16},
    {"rsv3", DPP_ATTR_FLAG_MASK, 1, 2, 143, 16},
    {"hit_flag", DPP_ATTR_FLAG_RST, 1, 1, 127, 1},
    {"action_index", DPP_ATTR_FLAG_RST, 1, 1, 126, 7},
    {"action_index2", DPP_ATTR_FLAG_RST, 1, 2, 119, 8},
    {"v_qid", DPP_ATTR_FLAG_RST, 1, 4, 111, 16},
    {"uplink_fd_id", DPP_ATTR_FLAG_RST, 1, 4, 95, 32},
    {"spec_port_vfid", DPP_ATTR_FLAG_RST, 1, 4, 63, 12},
    {"count_id", DPP_ATTR_FLAG_RST, 1, 4, 51, 20},
    {"hash_alg", DPP_ATTR_FLAG_RST, 1, 2, 31, 8},
    {"rss_hash_factor", DPP_ATTR_FLAG_RST, 1, 2, 23, 8},
    {"rsv3", DPP_ATTR_FLAG_RST, 1, 2, 15, 4},
    {"encap0_index", DPP_ATTR_FLAG_RST, 1, 2, 11, 12},
};

ZXDH_FLOW_ATTR_T g_flow_attr_list[]= 
{
    {
        "pmtu_info",
         85,
         DPP_FLOW_SDT_HASH,
         128,
         64,
         32,
         6,
         g_pmtu_info_fields
    },
    {
        "qp_info",
         86,
         DPP_FLOW_SDT_HASH,
         256,
         64,
         128,
         10,
         g_qp_info_fields
    },
    {
        "roce_match",
         84,
         DPP_FLOW_SDT_HASH,
         512,
         288,
         32,
         6,
         g_roce_match_fields
    },
    {
        "fd_cfg",
         130,
         DPP_FLOW_SDT_ACL,
         640,
         640,
         128,
         45,
         g_fd_cfg_fields
    },
};
ZXIC_UINT32 dpp_flow_attr_list_size_get(void)
{
    return sizeof(g_flow_attr_list)/sizeof(ZXDH_FLOW_ATTR_T);
}
