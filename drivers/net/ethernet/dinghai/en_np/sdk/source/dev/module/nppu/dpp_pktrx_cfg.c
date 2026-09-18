/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_pktrx_cfg.c
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : zzh
* 完成日期 : 2015/02/06
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/

#include "zxic_common.h"
#include "dpp_type_api.h"
#include "dpp_reg.h"
#include "dpp_pktrx_api.h"
#include "dpp_dev.h"
#include "dpp_agent_channel.h"


#if ZXIC_REAL("mcode glb cfg ")


ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_set_0(DPP_DEV_T *dev, ZXIC_UINT32 glb_cfg_data_0)
{
    ZXIC_UINT32  rc = 0;

    rc = dpp_reg_write(dev,  NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_0r, 0, 0, &glb_cfg_data_0);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_set_1(DPP_DEV_T *dev, ZXIC_UINT32 glb_cfg_data_1)
{
    ZXIC_UINT32  rc = 0;

    rc = dpp_reg_write(dev,  NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_1r, 0, 0, &glb_cfg_data_1);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_set_2(DPP_DEV_T *dev, ZXIC_UINT32 glb_cfg_data_2)
{
    ZXIC_UINT32  rc = 0;

    rc = dpp_reg_write(dev,  NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_2r, 0, 0, &glb_cfg_data_2);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_set_3(DPP_DEV_T *dev, ZXIC_UINT32 glb_cfg_data_3)
{
    ZXIC_UINT32  rc = 0;

    rc = dpp_reg_write(dev,  NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_3r, 0, 0, &glb_cfg_data_3);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}
/***********************************************************/
/**
* @param   dev_id
* @param   p_mcode_glb_cfg
*
* @return
* @remark  无
* @see
* @author  czd      @date  2016/04/27
************************************************************/
ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_get_0(DPP_DEV_T *dev, ZXIC_UINT32 *p_glb_cfg_data_0)
{
    ZXIC_UINT32  rc = 0;

    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_glb_cfg_data_0);

    rc = dpp_reg_read(dev,  NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_0r, 0, 0, p_glb_cfg_data_0);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    return DPP_OK;
}

ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_get_1(DPP_DEV_T *dev, ZXIC_UINT32 *p_glb_cfg_data_1)
{
    ZXIC_UINT32  rc = 0;

    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_glb_cfg_data_1);

    rc = dpp_reg_read(dev,  NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_1r, 0, 0, p_glb_cfg_data_1);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    return DPP_OK;
}

ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_get_2(DPP_DEV_T *dev, ZXIC_UINT32 *p_glb_cfg_data_2)
{
    ZXIC_UINT32  rc = 0;

    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_glb_cfg_data_2);

    rc = dpp_reg_read(dev,  NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_2r, 0, 0, p_glb_cfg_data_2);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    return DPP_OK;
}

ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_get_3(DPP_DEV_T *dev, ZXIC_UINT32 *p_glb_cfg_data_3)
{
    ZXIC_UINT32  rc = 0;

    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_glb_cfg_data_3);

    rc = dpp_reg_read(dev,  NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_3r, 0, 0, p_glb_cfg_data_3);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    return DPP_OK;
}

ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_write_0(DPP_DEV_T *dev, ZXIC_UINT32 start_bit_no, ZXIC_UINT32 end_bit_no,
                                            ZXIC_UINT32 glb_cfg_data_0)
{
    ZXIC_UINT32 rc        = 0;
    ZXIC_UINT32 data      = 0;
    ZXIC_MUTEX_T *p_mutex = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(start_bit_no, 0, 31);
    ZXIC_COMM_CHECK_INDEX(end_bit_no, start_bit_no, 31);

    rc = dpp_dev_opr_mutex_get(dev, DPP_DEV_MUTEX_T_PKTRX_MF_GLB_CFG_0, &p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_lock");

    rc = dpp_reg_read(dev, NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_0r, 0, 0, &data);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_reg_read", p_mutex);

    ZXIC_COMM_UINT32_WRITE_BITS(data, glb_cfg_data_0, start_bit_no, end_bit_no - start_bit_no + 1);

    rc = dpp_reg_write(dev, NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_0r, 0, 0, &data);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_reg_write", p_mutex);

    rc = zxic_comm_mutex_unlock(p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");

    return DPP_OK;
}

ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_write_1(DPP_DEV_T *dev, ZXIC_UINT32 start_bit_no, ZXIC_UINT32 end_bit_no,
                                            ZXIC_UINT32 glb_cfg_data_1)
{
    ZXIC_UINT32 rc        = 0;
    ZXIC_UINT32 data      = 0;
    ZXIC_MUTEX_T *p_mutex = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(start_bit_no, 0, 31);
    ZXIC_COMM_CHECK_INDEX(end_bit_no, start_bit_no, 31);

    rc = dpp_dev_opr_mutex_get(dev, DPP_DEV_MUTEX_T_PKTRX_MF_GLB_CFG_1, &p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_lock");

    rc = dpp_reg_read(dev, NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_1r, 0, 0, &data);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_reg_read", p_mutex);

    ZXIC_COMM_UINT32_WRITE_BITS(data, glb_cfg_data_1, start_bit_no, end_bit_no - start_bit_no + 1);

    rc = dpp_reg_write(dev, NPPU_PKTRX_CFG_PKTRX_GLBAL_CFG_1r, 0, 0, &data);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_reg_write", p_mutex);

    rc = zxic_comm_mutex_unlock(p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");

    return DPP_OK;
}

DPP_STATUS dpp_pktrx_ind_rd(DPP_DEV_T *dev,
                            ZXIC_UINT32 mem_addr,
                            ZXIC_UINT32 mem_id,
                            ZXIC_UINT32 len,
                            ZXIC_UINT32 *p_data)
{
    DPP_STATUS rtn = DPP_OK;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_data);
    ZXIC_COMM_CHECK_INDEX(mem_id, 0, (MEM_ID_MUX_NUM - 1));

    ZXIC_COMM_TRACE_NOTICE("dpp_pktrx_ind_rd start\n");

    rtn = dpp_agent_channel_pktrx_ind_reg_rw(dev, mem_addr, mem_id, DPP_PKTRX_IND_REG_RD, len, p_data);
    ZXIC_COMM_CHECK_RC(rtn, "dpp_agent_channel_pktrx_ind_reg_rw");

    ZXIC_COMM_TRACE_NOTICE("dpp_pktrx_ind_rd success\n");

    return DPP_OK;
}

DPP_STATUS dpp_pktrx_ind_wrt(DPP_DEV_T *dev,
                            ZXIC_UINT32 mem_addr,
                            ZXIC_UINT32 mem_id,
                            ZXIC_UINT32 len,
                            ZXIC_UINT32 *p_data)
{
    DPP_STATUS rtn = DPP_OK;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_data);
    ZXIC_COMM_CHECK_INDEX(mem_id, 0, (MEM_ID_MUX_NUM - 1));

    ZXIC_COMM_TRACE_NOTICE("dpp_pktrx_ind_wrt start\n");

    rtn = dpp_agent_channel_pktrx_ind_reg_rw(dev, mem_addr, mem_id, DPP_PKTRX_IND_REG_WR, len, p_data);
    ZXIC_COMM_CHECK_RC(rtn, "dpp_agent_channel_pktrx_ind_reg_rw");

    ZXIC_COMM_TRACE_NOTICE("dpp_pktrx_ind_wrt success\n");

    return DPP_OK;
}

DPP_STATUS dpp_pktrx_udf_table_set(DPP_DEV_T *dev, ZXIC_UINT32 index, DPP_PKTRX_PHYPORT_UDF_TABLE_T *p_phyport_user_info)
{
    DPP_STATUS  rc = 0;

    ZXIC_COMM_CHECK_INDEX(index, 0, (DPP_PHYPORT_NUM - 1));
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_phyport_user_info);

    /* read phyport_udf_tbl item */
    rc = dpp_pktrx_ind_wrt(dev,
                          index,
                          PHYPORT_TAB_2_MEM_ID,
                          16,
                          &(p_phyport_user_info->port_based_user_data[0]));
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_ind_wrt");

    return DPP_OK;
}

DPP_STATUS dpp_pktrx_udf_table_get(DPP_DEV_T *dev, ZXIC_UINT32 index, DPP_PKTRX_PHYPORT_UDF_TABLE_T *p_phyport_user_info)
{
    DPP_STATUS  rc = 0;

    ZXIC_COMM_CHECK_INDEX(index, 0, (DPP_PHYPORT_NUM - 1));
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_phyport_user_info);

    /* read phyport_udf_tbl item */
    rc = dpp_pktrx_ind_rd(dev,
                          index,
                          PHYPORT_TAB_2_MEM_ID,
                          16,
                          &(p_phyport_user_info->port_based_user_data[0]));
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_ind_rd");

    return DPP_OK;
}

DPP_STATUS dpp_pktrx_tcam_item_vld_get(DPP_DEV_T *dev, ZXIC_UINT32 index, ZXIC_UINT32 *p_vld_flg)
{
    DPP_STATUS rc = 0;
    ZXIC_UINT32 buff = 0;
    ZXIC_UINT32 pktrx_tcam_reg_no = 0;

    ZXIC_COMM_CHECK_POINT(p_vld_flg);
    ZXIC_COMM_CHECK_INDEX(index, 0, (DPP_PKTRX_ICU_TCAM_NUM - 1));

    pktrx_tcam_reg_no = DPP_PKTRX_TCAM_REGID_GET(index);
    index = DPP_PKTRX_TCAM_INDEX_GET(index);

    rc = dpp_reg_read(dev,  pktrx_tcam_reg_no, 0, index / 32, &buff);
    ZXIC_COMM_CHECK_RC(rc, "dpp_reg_read");
    ZXIC_COMM_UINT32_GET_BITS(*p_vld_flg, buff, index%32, 1 );

    return DPP_OK;
}

DPP_STATUS dpp_pktrx_tcam_item_disable(DPP_DEV_T *dev, ZXIC_UINT32 index)
{
    DPP_STATUS rc                 = 0;
    ZXIC_UINT32 buff              = 0;
    ZXIC_UINT32 pktrx_tcam_reg_no = 0;
    ZXIC_MUTEX_T *p_nppu_mutex    = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(index, 0, (DPP_PKTRX_ICU_TCAM_NUM - 1));

    if (DPP_NPPU_TCAM_ENABLE_SET_MSG >= DEV_PCIE_BAR_MSG_NUM(dev))
    {
        pktrx_tcam_reg_no = DPP_PKTRX_TCAM_REGID_GET(index);
        index             = DPP_PKTRX_TCAM_INDEX_GET(index);

        rc = dpp_dev_opr_mutex_get(dev, DPP_DEV_MUTEX_T_NPPU, &p_nppu_mutex);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dev_opr_mutex_get");

        rc = zxic_comm_mutex_lock(p_nppu_mutex);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_lock");

        /* clear tcam */
        rc = dpp_reg_read(dev, pktrx_tcam_reg_no, 0, index / 32, &buff);
        ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_reg_read", p_nppu_mutex);

        /* 将要操作的tcam条目无效 */ /* 此处是否牵涉到读锁的问题??? */
        buff &= (~(1u << (index % 32)));

        rc = dpp_reg_write(dev, pktrx_tcam_reg_no, 0, index / 32, &buff);
        ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_reg_write", p_nppu_mutex);

        rc = zxic_comm_mutex_unlock(p_nppu_mutex);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
    }
    else
    {
        rc = dpp_agent_channel_msg_nppu_tcam_enable_set(dev, index, 0);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_agent_channel_msg_nppu_tcam_enable_set");
    }

    return DPP_OK;
}

/***********************************************************/
/** 将指定的tcam条目置为有效
* @param   dev_id   芯片ID
* @param   index       索引号
*
* @return
* @remark  无
* @see
* @author  czd      @date  2016/02/23
************************************************************/
DPP_STATUS dpp_pktrx_tcam_item_enable(DPP_DEV_T *dev, ZXIC_UINT32 index)
{
    DPP_STATUS  rc = 0;
    ZXIC_UINT32        buff = 0;
    ZXIC_UINT32   pktrx_tcam_reg_no = 0;
    ZXIC_MUTEX_T *p_nppu_mutex = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(index, 0, (DPP_PKTRX_ICU_TCAM_NUM - 1));

    if (DPP_NPPU_TCAM_ENABLE_SET_MSG >= DEV_PCIE_BAR_MSG_NUM(dev))
    {
        pktrx_tcam_reg_no = DPP_PKTRX_TCAM_REGID_GET(index);
        index             = DPP_PKTRX_TCAM_INDEX_GET(index);

        rc = dpp_dev_opr_mutex_get(dev, DPP_DEV_MUTEX_T_NPPU, &p_nppu_mutex);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dev_opr_mutex_get");

        rc = zxic_comm_mutex_lock(p_nppu_mutex);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_lock");

        /* enable tcam */
        rc = dpp_reg_read(dev, pktrx_tcam_reg_no, 0, index / 32, &buff);
        ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_reg_read", p_nppu_mutex);

        buff |= (1u << (index % 32));

        rc = dpp_reg_write(dev, pktrx_tcam_reg_no, 0, index / 32, &buff);
        ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_reg_write", p_nppu_mutex);

        rc = zxic_comm_mutex_unlock(p_nppu_mutex);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
    }
    else
    {
        rc = dpp_agent_channel_msg_nppu_tcam_enable_set(dev, index, 1);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_agent_channel_msg_nppu_tcam_enable_set");
    }

    return DPP_OK;
}

DPP_STATUS dpp_pktrx_xytcam_x_set(DPP_DEV_T *dev, ZXIC_UINT32 index, DPP_PKTRX_TCAM_DT_TABLE_T *p_tcam_info)
{
    ZXIC_UINT32 data[5] = {0};
    DPP_STATUS  rc = 0;
    ZXIC_UINT32   pktrx_tcam_mem_id = 0;

    ZXIC_COMM_CHECK_INDEX(index, 0, (DPP_PKTRX_ICU_TCAM_NUM - 1));
    ZXIC_COMM_CHECK_POINT(p_tcam_info);

    pktrx_tcam_mem_id = DPP_PKTRX_TCAM_MEMID_GET(index);
    index = DPP_PKTRX_TCAM_INDEX_GET(index);

    data[4] = ZXIC_COMM_DM_TO_X(p_tcam_info->tcam_key_udf_l32, p_tcam_info->tcam_key_mask_udf_l32);
    data[3] = ZXIC_COMM_DM_TO_X(p_tcam_info->tcam_key_udf_m32, p_tcam_info->tcam_key_mask_udf_m32);
    data[2] = ZXIC_COMM_DM_TO_X(((((p_tcam_info->tcam_key_l3type) & 0xff) << 24) | (((p_tcam_info->tcam_key_priority) << 21) & 0x00e00000) | ((p_tcam_info->tcam_key_cfi << 20) & 0x00100000) | ((p_tcam_info->tcam_key_ex_vlanid << 8) & 0x000fff00) | (p_tcam_info->tcam_key_udf_h8 & 0xff)),
                                    ((((p_tcam_info->tcam_key_mask_l3type) & 0xff) << 24) | (((p_tcam_info->tcam_key_mask_priority) << 21) & 0x00e00000) | ((p_tcam_info->tcam_key_mask_cfi << 20) & 0x00100000) | ((p_tcam_info->tcam_key_mask_ex_vlanid << 8) & 0x000fff00) | (p_tcam_info->tcam_key_mask_udf_h8 & 0xff)));
    data[1] = ZXIC_COMM_DM_TO_X((((p_tcam_info->tcam_key_dmac_l24) << 8) | (((p_tcam_info->tcam_key_l3type) >> 8) & 0xff)), (((p_tcam_info->tcam_key_mask_dmac_l24) << 8) | (((p_tcam_info->tcam_key_mask_l3type) >> 8) & 0xff)));
    data[0] = ZXIC_COMM_DM_TO_X((((p_tcam_info->tcam_key_mode) << 31) | (((p_tcam_info->tcam_key_port_num) << 24) & 0x7f000000) | (p_tcam_info->tcam_key_dmac_h24 & 0xffffff)),
                                    (((p_tcam_info->tcam_key_mask_mode) << 31) | (((p_tcam_info->tcam_key_mask_port_num) << 24) & 0x7f000000) | (p_tcam_info->tcam_key_mask_dmac_h24 & 0xffffff)));

    /* write xytcam_x reg */
    rc = dpp_pktrx_ind_wrt(dev,
                           ((1 << 8)  | index),
                           pktrx_tcam_mem_id,
                           20,
                           data);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_ind_wrt");

    return DPP_OK;
}

DPP_STATUS dpp_pktrx_xytcam_y_set(DPP_DEV_T *dev, ZXIC_UINT32 index, DPP_PKTRX_TCAM_DT_TABLE_T *p_tcam_info)
{
    ZXIC_UINT32 mask[5] = {0};
    DPP_STATUS  rc = 0;
    ZXIC_UINT32   pktrx_tcam_mem_id = 0;

    ZXIC_COMM_CHECK_INDEX(index, 0, (DPP_PKTRX_ICU_TCAM_NUM - 1));
    ZXIC_COMM_CHECK_POINT(p_tcam_info);

    pktrx_tcam_mem_id = DPP_PKTRX_TCAM_MEMID_GET(index);
    index = DPP_PKTRX_TCAM_INDEX_GET(index);

    mask[4] = ZXIC_COMM_DM_TO_Y(p_tcam_info->tcam_key_udf_l32, p_tcam_info->tcam_key_mask_udf_l32);
    mask[3] = ZXIC_COMM_DM_TO_Y(p_tcam_info->tcam_key_udf_m32, p_tcam_info->tcam_key_mask_udf_m32);
    mask[2] = ZXIC_COMM_DM_TO_Y(((((p_tcam_info->tcam_key_l3type) & 0xff) << 24) | (((p_tcam_info->tcam_key_priority) << 21) & 0x00e00000) | ((p_tcam_info->tcam_key_cfi << 20) & 0x00100000) | ((p_tcam_info->tcam_key_ex_vlanid << 8) & 0x000fff00) | (p_tcam_info->tcam_key_udf_h8 & 0xff)),
                                    ((((p_tcam_info->tcam_key_mask_l3type) & 0xff) << 24) | (((p_tcam_info->tcam_key_mask_priority) << 21) & 0x00e00000) | ((p_tcam_info->tcam_key_mask_cfi << 20) & 0x00100000) | ((p_tcam_info->tcam_key_mask_ex_vlanid << 8) & 0x000fff00) | (p_tcam_info->tcam_key_mask_udf_h8 & 0xff)));
    mask[1] = ZXIC_COMM_DM_TO_Y((((p_tcam_info->tcam_key_dmac_l24) << 8) | (((p_tcam_info->tcam_key_l3type) >> 8) & 0xff)), (((p_tcam_info->tcam_key_mask_dmac_l24) << 8) | (((p_tcam_info->tcam_key_mask_l3type) >> 8) & 0xff)));
    mask[0] = ZXIC_COMM_DM_TO_Y((((p_tcam_info->tcam_key_mode) << 31) | (((p_tcam_info->tcam_key_port_num) << 24) & 0x7f000000) | (p_tcam_info->tcam_key_dmac_h24 & 0xffffff)),
                                    (((p_tcam_info->tcam_key_mask_mode) << 31) | (((p_tcam_info->tcam_key_mask_port_num) << 24) & 0x7f000000) | (p_tcam_info->tcam_key_mask_dmac_h24 & 0xffffff)));

    /* write xytcam_x reg */
    rc = dpp_pktrx_ind_wrt(dev,
                           index,
                           pktrx_tcam_mem_id,
                           20,
                           mask);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_ind_wrt");

    return DPP_OK;
}

DPP_STATUS dpp_pktrx_tcam_table_set(DPP_DEV_T *dev, ZXIC_UINT32 index, DPP_PKTRX_TCAM_DT_TABLE_T *p_tcam_info)
{
    DPP_STATUS  rc = 0;
    ZXIC_UINT32 icu_tcam_result_tbl_buff = 0;
    ZXIC_UINT32 tcam_result_mem_id = 0;
    ZXIC_UINT32 vldFlag = 0;
    ZXIC_UINT32 ind_cmd_index = 0;    /* 间接读写命令寄存器 需要传入的tcam0或者tcam1的index */

    ZXIC_COMM_CHECK_INDEX(index, 0, (DPP_PKTRX_ICU_TCAM_NUM - 1));
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_tcam_info);
    ZXIC_COMM_CHECK_INDEX(p_tcam_info->tcam_result_table_type, 0, 1);
    ZXIC_COMM_CHECK_INDEX(p_tcam_info->tcam_result_vld, 0, 1);

    rc = dpp_pktrx_tcam_item_vld_get(dev, index, &vldFlag);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_tcam_item_vld_get");

    /*1. disable icu_tcam_vld reg */
    if(vldFlag)
    {
        rc = dpp_pktrx_tcam_item_disable(dev, index);
        ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_tcam_item_disable");
    }
    
    /* 2. wrt icu_tcam_key */
    rc = dpp_pktrx_xytcam_x_set(dev, index, p_tcam_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_icu_xytcam_x_wrt");

    rc = dpp_pktrx_xytcam_y_set(dev, index, p_tcam_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_icu_xytcam_y_wrt");

    tcam_result_mem_id = DPP_PKTRX_TCAM_RESULT_MEMID_GET(index);
    ind_cmd_index = DPP_PKTRX_TCAM_INDEX_GET(index);

    if (!p_tcam_info->tcam_result_table_type)
    {
        icu_tcam_result_tbl_buff =
            (((p_tcam_info->tcam_result_flownum & 0xff) << 18) | ((p_tcam_info->tcam_result_vld & 0x1) << 17)
             | ((p_tcam_info->tcam_result_table_type & 0x1) << 16)
             | (p_tcam_info->tcam_result_pc_or_cos & STARTPC_MASK));
    }
    else
    {
        icu_tcam_result_tbl_buff =
            (((p_tcam_info->tcam_result_vld & 0x1) << 17) | ((p_tcam_info->tcam_result_table_type & 0x1) << 16)
             | (p_tcam_info->tcam_result_pc_or_cos & 0x7));
    }

    /* 3. write icu_tcam result tbl */
    rc = dpp_pktrx_ind_wrt(dev,
                           ind_cmd_index,
                           tcam_result_mem_id,
                           4,
                           &icu_tcam_result_tbl_buff);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_ind_wrt");

    /* 4. enable icu_tcam_vld reg  */
    /* 将要操作的tcam条目生效 */
    if(vldFlag)
    {
        rc = dpp_pktrx_tcam_item_enable(dev, index);
        ZXIC_COMM_CHECK_RC(rc, "dpp_pktrx_tcam_item_enable");
    }

    return DPP_OK;
}

#endif