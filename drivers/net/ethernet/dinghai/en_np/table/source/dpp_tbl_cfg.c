#include "dpp_tbl_cfg.h"
#include "dpp_dev.h"
#include "dpp_pktrx_cfg.h"
#include "dpp_pktrx_api.h"
#include "dpp_agent_channel.h"
#include "dpp_tbl_comm.h"
#include "dpp_tbl_api.h"


ZXIC_UINT32 dpp_glb_cfg_set_0(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 glb_cfg_data_0)
{
    ZXIC_UINT32  rc = 0;
    DPP_DEV_T dev = {0};
    ZXIC_COMM_CHECK_POINT(pf_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_pktrx_mcode_glb_cfg_set_0(&dev, glb_cfg_data_0);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_mcode_glb_cfg_set_0");
    
    return DPP_OK;
}
EXPORT_SYMBOL(dpp_glb_cfg_set_0);

ZXIC_UINT32 dpp_glb_cfg_set_1(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 glb_cfg_data_1)
{
    ZXIC_UINT32  rc = 0;
    DPP_DEV_T dev = {0};
    ZXIC_COMM_CHECK_POINT(pf_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_pktrx_mcode_glb_cfg_set_1(&dev, glb_cfg_data_1);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_mcode_glb_cfg_set_1");
    
    return DPP_OK;
}
EXPORT_SYMBOL(dpp_glb_cfg_set_1);

ZXIC_UINT32 dpp_glb_cfg_set_2(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 glb_cfg_data_2)
{
    ZXIC_UINT32  rc = 0;
    DPP_DEV_T dev = {0};
    ZXIC_COMM_CHECK_POINT(pf_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_pktrx_mcode_glb_cfg_set_2(&dev, glb_cfg_data_2);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_mcode_glb_cfg_set_2");
    
    return DPP_OK;
}
EXPORT_SYMBOL(dpp_glb_cfg_set_2);

ZXIC_UINT32 dpp_glb_cfg_set_3(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 glb_cfg_data_3)
{
    ZXIC_UINT32  rc = 0;
    DPP_DEV_T dev = {0};
    ZXIC_COMM_CHECK_POINT(pf_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_pktrx_mcode_glb_cfg_set_3(&dev, glb_cfg_data_3);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_mcode_glb_cfg_set_3");
    
    return DPP_OK;
}
EXPORT_SYMBOL(dpp_glb_cfg_set_3);

ZXIC_UINT32 dpp_glb_cfg_get_0(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *p_glb_cfg_data_0)
{
    ZXIC_UINT32  rc = 0;
    DPP_DEV_T dev = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_glb_cfg_data_0);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_pktrx_mcode_glb_cfg_get_0(&dev, p_glb_cfg_data_0);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_mcode_glb_cfg_get_0");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_glb_cfg_get_0);

ZXIC_UINT32 dpp_glb_cfg_get_1(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *p_glb_cfg_data_1)
{
    ZXIC_UINT32  rc = 0;
    DPP_DEV_T dev = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_glb_cfg_data_1);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_pktrx_mcode_glb_cfg_get_1(&dev, p_glb_cfg_data_1);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_mcode_glb_cfg_get_1");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_glb_cfg_get_1);

ZXIC_UINT32 dpp_glb_cfg_get_2(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *p_glb_cfg_data_2)
{
    ZXIC_UINT32  rc = 0;
    DPP_DEV_T dev = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_glb_cfg_data_2);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_pktrx_mcode_glb_cfg_get_2(&dev, p_glb_cfg_data_2);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_mcode_glb_cfg_get_2");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_glb_cfg_get_2);

ZXIC_UINT32 dpp_glb_cfg_get_3(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *p_glb_cfg_data_3)
{
    ZXIC_UINT32  rc = 0;
    DPP_DEV_T dev = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_glb_cfg_data_3);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_pktrx_mcode_glb_cfg_get_3(&dev, p_glb_cfg_data_3);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_mcode_glb_cfg_get_3");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_glb_cfg_get_3);

ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_wr(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 reg_index, ZXIC_UINT32 start_bit_no, 
                                            ZXIC_UINT32 end_bit_no, ZXIC_UINT32 glb_cfg_data)
{
    ZXIC_UINT32 rc        = 0;
    ZXIC_UINT32 data      = 0;
    ZXIC_UINT32 reg_no    = 0;
    ZXIC_UINT32 reg_lock  = 0;
    DPP_DEV_T dev = {0};
    ZXIC_MUTEX_T *p_mutex = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_INDEX(start_bit_no, 0, 31);
    ZXIC_COMM_CHECK_INDEX(end_bit_no, start_bit_no, 31);
    ZXIC_COMM_CHECK_INDEX_UPPER(reg_index, DPP_GLB_CFG_REG_INDEX_MAX - 1);

    reg_no = NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_0r + reg_index;
    reg_lock = DPP_DEV_MUTEX_T_PKTRX_MF_GLB_CFG_0 + reg_index;

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");
    
    rc = dpp_dev_opr_mutex_get(&dev, reg_lock, &p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(&dev), rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(&dev), rc, "zxic_comm_mutex_lock");

    rc = dpp_reg_read(&dev, reg_no, 0, 0, &data);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(&dev), rc, "dpp_reg_read", p_mutex);

    ZXIC_COMM_UINT32_WRITE_BITS(data, glb_cfg_data, start_bit_no, end_bit_no - start_bit_no + 1);

    rc = dpp_reg_write(&dev, reg_no, 0, 0, &data);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(&dev), rc, "dpp_reg_write", p_mutex);

    rc = zxic_comm_mutex_unlock(p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(&dev), rc, "zxic_comm_mutex_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_pktrx_mcode_glb_cfg_wr);

ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_rd(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 reg_index, ZXIC_UINT32 start_bit_no, 
                                        ZXIC_UINT32 end_bit_no, ZXIC_UINT32 *p_glb_cfg_data)
{
    ZXIC_UINT32 rc        = 0;
    ZXIC_UINT32 data      = 0;
    ZXIC_UINT32 reg_no    = 0;
    ZXIC_UINT32 reg_lock  = 0;
    DPP_DEV_T dev = {0};
    ZXIC_MUTEX_T *p_mutex = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_INDEX(start_bit_no, 0, 31);
    ZXIC_COMM_CHECK_INDEX(end_bit_no, start_bit_no, 31);
    ZXIC_COMM_CHECK_INDEX_UPPER(reg_index, DPP_GLB_CFG_REG_INDEX_MAX - 1);

    reg_no = NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_0r + reg_index;
    reg_lock = DPP_DEV_MUTEX_T_PKTRX_MF_GLB_CFG_0 + reg_index;

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");
    
    rc = dpp_dev_opr_mutex_get(&dev, reg_lock, &p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(&dev), rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(&dev), rc, "zxic_comm_mutex_lock");

    rc = dpp_reg_read(&dev, reg_no, 0, 0, &data);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(&dev), rc, "dpp_reg_read", p_mutex);

    rc = zxic_comm_mutex_unlock(p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(&dev), rc, "zxic_comm_mutex_unlock");

    ZXIC_COMM_UINT32_GET_BITS((*p_glb_cfg_data), data, start_bit_no, end_bit_no - start_bit_no + 1);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_pktrx_mcode_glb_cfg_rd);

ZXIC_UINT32 dpp_l2d_psn_cfg_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 psn_cfg)
{
    ZXIC_UINT32  rc = 0;
    DPP_DEV_T dev = {0};
    ZXIC_MUTEX_T *p_mutex = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dev_opr_mutex_get(&dev, DPP_DEV_MUTEX_T_PKTRX_MF_GLB_CFG_1, &p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(&dev), rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(&dev), rc, "zxic_comm_mutex_lock");

    rc = dpp_agent_channel_psn_cfg_l2d_write(&dev, psn_cfg);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(&dev), rc, "dpp_agent_channel_psn_cfg_l2d_write", p_mutex);

    rc = zxic_comm_mutex_unlock(p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(&dev), rc, "zxic_comm_mutex_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_l2d_psn_cfg_set);

ZXIC_UINT32 dpp_l2d_psn_cfg_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *p_psn_cfg)
{
    ZXIC_UINT32  rc = 0;
    DPP_DEV_T dev = {0};
    ZXIC_MUTEX_T *p_mutex = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_psn_cfg);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dev_opr_mutex_get(&dev, DPP_DEV_MUTEX_T_PKTRX_MF_GLB_CFG_1, &p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(&dev), rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(&dev), rc, "zxic_comm_mutex_lock");

    rc = dpp_agent_channel_psn_cfg_l2d_read(&dev, p_psn_cfg);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(&dev), rc, "dpp_agent_channel_psn_cfg_l2d_read", p_mutex);

    rc = zxic_comm_mutex_unlock(p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(&dev), rc, "zxic_comm_mutex_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_l2d_psn_cfg_get);

ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_write(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 start_bit_no, ZXIC_UINT32 end_bit_no,
                                            ZXIC_UINT32 glb_cfg_data_1)
{
    ZXIC_UINT32 rc        = 0;
    DPP_DEV_T dev = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    
    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_pktrx_mcode_glb_cfg_write_1(&dev, start_bit_no, end_bit_no, glb_cfg_data_1);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_mcode_glb_cfg_write_1");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_pktrx_mcode_glb_cfg_write);

ZXIC_UINT32 dpp_mcode_feature_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT64 *feature)
{
    ZXIC_UINT32 rc = 0;
    DPP_DEV_T dev = {0};
    DPP_PKTRX_PHYPORT_UDF_TABLE_T phy_udf_table = {0};

    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_MCODE_FEATURE_LIST_NUM - 1);
    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(feature);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_pktrx_udf_table_get(&dev, 11 + index / 2, &phy_udf_table);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_udf_table_get");

    *feature = ZXIC_COMM_COUNTER64_BUILD(phy_udf_table.port_based_user_data[(index % 2) * 2], \
                                         phy_udf_table.port_based_user_data[(index % 2) * 2 + 1]);

    return 0;
}
EXPORT_SYMBOL(dpp_mcode_feature_get);

ZXIC_UINT32 dpp_pktrx_mcode_port_cfg_read(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 port_id, ZXIC_UINT32 *port_cfg_data)
{
    ZXIC_UINT32 rc = 0;
    DPP_DEV_T dev = {0};
    ZXIC_MUTEX_T *p_mutex = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(port_cfg_data);
    ZXIC_COMM_CHECK_INDEX(port_id, 0, 118);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dev_opr_mutex_get(&dev, DPP_DEV_MUTEX_T_NPPU, &p_mutex);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_mutex);
    ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_lock");

    rc = dpp_agent_channel_pktrx_ind_reg_rw(&dev, port_id, PHYPORT_TAB_2_MEM_ID, DPP_PKTRX_IND_REG_RD, 16, port_cfg_data);
    ZXIC_COMM_CHECK_RC_NO_ASSERT_UNLOCK(rc, "dpp_agent_channel_pktrx_ind_reg_rw", p_mutex);

    rc = zxic_comm_mutex_unlock(p_mutex);
    ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_pktrx_mcode_port_cfg_read);

ZXIC_UINT32 dpp_pktrx_mcode_port_cfg_write(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 port_id, ZXIC_UINT32 index, ZXIC_UINT32 start_bit_no,
                                           ZXIC_UINT32 end_bit_no,ZXIC_UINT32 port_cfg_data)
{
    ZXIC_UINT32 rc        = 0;
    ZXIC_UINT32 data[4]   = {0};
    DPP_DEV_T dev         = {0};
    ZXIC_MUTEX_T *p_mutex = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_INDEX(index, 0, 3);
    ZXIC_COMM_CHECK_INDEX(port_id, 0, 118);
    ZXIC_COMM_CHECK_INDEX(start_bit_no, 0, 31);
    ZXIC_COMM_CHECK_INDEX(end_bit_no, start_bit_no, 31);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dev_opr_mutex_get(&dev, DPP_DEV_MUTEX_T_NPPU, &p_mutex);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_mutex);
    ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_lock");

    rc = dpp_agent_channel_pktrx_ind_reg_rw(&dev, port_id, PHYPORT_TAB_2_MEM_ID, DPP_PKTRX_IND_REG_RD, 16, data);
    ZXIC_COMM_CHECK_RC_NO_ASSERT_UNLOCK(rc, "dpp_agent_channel_pktrx_ind_reg_rw", p_mutex);

    ZXIC_COMM_UINT32_WRITE_BITS(data[index], port_cfg_data, start_bit_no, end_bit_no - start_bit_no + 1);

    rc = dpp_agent_channel_pktrx_ind_reg_rw(&dev, port_id, PHYPORT_TAB_2_MEM_ID, DPP_PKTRX_IND_REG_WR, 16, data);
    ZXIC_COMM_CHECK_RC_NO_ASSERT_UNLOCK(rc, "dpp_agent_channel_pktrx_ind_reg_rw", p_mutex);

    rc = zxic_comm_mutex_unlock(p_mutex);
    ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_pktrx_mcode_port_cfg_write);

ZXIC_UINT32 dpp_pktrx_udf_icmp_item_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 phyport)
{
    ZXIC_UINT32 rc = 0;
    DPP_DEV_T dev = {0};
    ZXIC_UINT32 vfid = 0;
    ZXIC_MUTEX_T *p_mutex = NULL;
    DPP_PKTRX_PHYPORT_UDF_TABLE_T phy_udf_table = {0};

    ZXIC_COMM_CHECK_INDEX(phyport, TRPG_EXT_PORT_NUM_BEGIN, TRPG_EXT_PORT_NUM_END);
    ZXIC_COMM_CHECK_POINT(pf_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    vfid = VQM_VFID(pf_info->vport);

    rc = dpp_dev_opr_mutex_get(&dev, DPP_DEV_MUTEX_T_PKTRX_MF_UDF_CFG, &p_mutex);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_opr_mutex_get");    

    rc = zxic_comm_mutex_lock(p_mutex);
    ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_lock");

    rc = dpp_pktrx_udf_table_get(&dev, phyport, &phy_udf_table);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_pktrx_udf_table_get", p_mutex);

    phy_udf_table.port_based_user_data[1] = vfid;

    rc = dpp_pktrx_udf_table_set(&dev, phyport, &phy_udf_table);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_pktrx_udf_table_set", p_mutex);

    rc = zxic_comm_mutex_unlock(p_mutex);
    ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_unlock");

    return 0;
}
EXPORT_SYMBOL(dpp_pktrx_udf_icmp_item_set);

#define MCODE_ICMP_LABEL  (0x1f)
#define MULTICAST_MAC_ADDR          (0x010000000000)
#define BROADCAST_MAC_ADDR          (0xFFFFFFFFFFFF)
#define ZERO_MAC_ADDR               (0x000000000000)
#define DPP_PKTRX_TCAM_USED_ITEMS_NUM      (192U) /* NCSI DEBUG PXE已经占用了前188条, 32bit对齐，先不下沉 */

ZXIC_UINT32 dpp_pktrx_tcam_icmp_item_set(DPP_PF_INFO_T *pf_info, ZXIC_UINT8 *pMac, ZXIC_UINT32 phyport)
{
    ZXIC_UINT32 rc                       = 0;
    DPP_DEV_T dev                        = {0};
    ZXIC_UINT64 mac                      = 0;
    ZXIC_UINT64 index                    = 0;
    DPP_PKTRX_TCAM_DT_TABLE_T tcamDtInfo = {0};

    ZXIC_COMM_CHECK_INDEX(phyport, TRPG_EXT_PORT_NUM_BEGIN, TRPG_EXT_PORT_NUM_END);
    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(pMac);

    mac = (((ZXIC_UINT64)(pMac[0])) << 40) | (((ZXIC_UINT64)(pMac[1])) << 32) | (((ZXIC_UINT64)(pMac[2])) << 24)
          | (((ZXIC_UINT64)(pMac[3])) << 16) | (((ZXIC_UINT64)(pMac[4])) << 8) | ((ZXIC_UINT64)(pMac[5]));

    mac = mac & 0xFFFFFFFFFFFF;

    if (mac == BROADCAST_MAC_ADDR || mac == ZERO_MAC_ADDR || (mac & MULTICAST_MAC_ADDR) != 0)
    {
        ZXIC_COMM_TRACE_ERROR("MAC(0x%012llx) is invalid\n", mac);
        return DPP_ERR;
    }

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    index = DPP_PKTRX_TCAM_USED_ITEMS_NUM + phyport;

    /* 配置BMC MAC过滤 */
    tcamDtInfo.tcam_key_mode      = 1; /**< @brief 查表类型 1b  0:cos 1:start_pc+flownum */
    tcamDtInfo.tcam_key_mask_mode = 0;

    tcamDtInfo.tcam_key_port_num      = 0; /**<  @brief  端口类型 7b*/
    tcamDtInfo.tcam_key_mask_port_num = 0xF;

    tcamDtInfo.tcam_key_dmac_h24      = (mac >> 24) & 0xFFFFFF; /**< @brief 目的mac高24bit */
    tcamDtInfo.tcam_key_mask_dmac_h24 = 0;

    tcamDtInfo.tcam_key_dmac_l24      = mac & 0xFFFFFF; /**< @brief 目的mac低24bit */
    tcamDtInfo.tcam_key_mask_dmac_l24 = 0;

    tcamDtInfo.tcam_key_l3type      = 0; /**< @brief l3_类型字段 16b*/
    tcamDtInfo.tcam_key_mask_l3type = 0xFFFF;

    tcamDtInfo.tcam_key_priority      = 0; /**< @brief priority 3b*/
    tcamDtInfo.tcam_key_mask_priority = 0x7;

    tcamDtInfo.tcam_key_cfi      = 0; /**< @brief cfi 1b */
    tcamDtInfo.tcam_key_mask_cfi = 0x1;

    tcamDtInfo.tcam_key_ex_vlanid      = 0; /**< @brief ex_vlanid 12b */
    tcamDtInfo.tcam_key_mask_ex_vlanid = 0xFFF;

    tcamDtInfo.tcam_key_udf_h8      = 0; /**< @brief udf高8bit(udf8) */
    tcamDtInfo.tcam_key_mask_udf_h8 = 0xFF;

    tcamDtInfo.tcam_key_udf_m32      = 0x04; /**< @brief udf中间32bit(udf7 udf6 udf5 udf4) */
    tcamDtInfo.tcam_key_mask_udf_m32 = 0xFFFFFBFB;

    tcamDtInfo.tcam_key_udf_l32      = 0; /**< @brief udf低32bit(udf3 udf2 udf1 udf0) */
    tcamDtInfo.tcam_key_mask_udf_l32 = 0xFFFFFFFF;

    tcamDtInfo.tcam_result_flownum    = 0; /**< @brief 结果表flownum  8b*/
    tcamDtInfo.tcam_result_vld        = 1; /**< @brief 结果表vld 1b*/
    tcamDtInfo.tcam_result_table_type = 0; /**< @brief 结果表table_type 1b 0:pc  1:cos  */
    tcamDtInfo.tcam_result_pc_or_cos  = MCODE_ICMP_LABEL;

    rc = dpp_pktrx_tcam_table_set(&dev, index, &tcamDtInfo);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_tcam_table_set");

    rc = dpp_pktrx_tcam_item_enable(&dev, index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_tcam_item_enable");

    return 0;
}
EXPORT_SYMBOL(dpp_pktrx_tcam_icmp_item_set);

ZXIC_UINT32 dpp_pktrx_tcam_pfc_set(DPP_PF_INFO_T *pf_info, ZXIC_UINT8 pfc_map)
{
    ZXIC_UINT32 rc = 0;
    DPP_DEV_T dev  = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_agent_channel_msg_nppu_tcam_pfc_set(&dev, pfc_map);
    ZXIC_COMM_CHECK_RC(rc, "dpp_agent_channel_msg_nppu_tcam_pfc_set");

    return 0;
}
EXPORT_SYMBOL(dpp_pktrx_tcam_pfc_set);

ZXIC_UINT32 dpp_pktrx_tcam_pfc_get(DPP_PF_INFO_T *pf_info, ZXIC_UINT8 *pfc_map)
{
    ZXIC_UINT32 rc = 0;
    DPP_DEV_T dev  = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(pfc_map);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_agent_channel_msg_nppu_tcam_pfc_get(&dev, pfc_map);
    ZXIC_COMM_CHECK_RC(rc, "dpp_agent_channel_msg_nppu_tcam_pfc_get");

    return 0;
}
EXPORT_SYMBOL(dpp_pktrx_tcam_pfc_get);