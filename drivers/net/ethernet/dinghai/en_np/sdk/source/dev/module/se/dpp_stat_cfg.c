/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_stat_cfg.c
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : ls
* 完成日期 : 2016/03/29
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
#include "dpp_dev.h"
#include "dpp_stat_reg.h"
#include "dpp_stat_cfg.h"
#include "dpp_stat_api.h"
#include "dpp_se_api.h"
#include "dpp_se.h"
#include "dpp_reg_api.h"
#include "dpp_reg_info.h"

PPU_STAT_CFG_T g_ppu_stat_cfg = {0};

#if ZXIC_REAL("Basic Reg Operation")

/***********************************************************/
/** 获取ppu统计片内深度
* @param   dev_id               设备号
* @param   p_ppu_eram_depth     ppu统计片内深度
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/03/31
************************************************************/
DPP_STATUS dpp_stat_ppu_eram_depth_get(DPP_DEV_T *dev,
                                       ZXIC_UINT32 *p_ppu_eram_depth)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_STAT_CFG_PPU_ERAM_DEPTH_T ppu_eram_depth_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_ppu_eram_depth);

    rc = dpp_reg_read(dev,
                      STAT_STAT_CFG_PPU_ERAM_DEPTHr,
                      0,
                      0,
                      &ppu_eram_depth_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    *p_ppu_eram_depth = ppu_eram_depth_cfg.ppu_eram_depth;

    return rc;
}

/***********************************************************/
/** 获取ppu统计 ERAM基地址
* @param   dev_id               设备号
* @param   p_ppu_eram_baddr     ppu统计片内基地址
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/03/31
************************************************************/
DPP_STATUS dpp_stat_ppu_eram_baddr_get(DPP_DEV_T *dev,
                                       ZXIC_UINT32 *p_ppu_eram_baddr)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_STAT_CFG_PPU_ERAM_BASE_ADDR_T ppu_eram_baddr_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_ppu_eram_baddr);

    rc = dpp_reg_read(dev,
                      STAT_STAT_CFG_PPU_ERAM_BASE_ADDRr,
                      0,
                      0,
                      &ppu_eram_baddr_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    *p_ppu_eram_baddr = ppu_eram_baddr_cfg.ppu_eram_base_addr;

    return rc;
}

/***********************************************************/
/** 获取ppu统计 ddr基地址
* @param   dev_id              设备号
* @param   p_ppu_ddr_baddr     ppu统计片外基地址
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/03/31
************************************************************/
DPP_STATUS dpp_stat_ppu_ddr_baddr_get(DPP_DEV_T *dev,
                                      ZXIC_UINT32 *p_ppu_ddr_baddr)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_STAT_CFG_PPU_DDR_BASE_ADDR_T ppu_ddr_baddr_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_ppu_ddr_baddr);

    rc = dpp_reg_read(dev,
                      STAT_STAT_CFG_PPU_DDR_BASE_ADDRr,
                      0,
                      0,
                      &ppu_ddr_baddr_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    *p_ppu_ddr_baddr = ppu_ddr_baddr_cfg.ppu_ddr_base_addr;

    return rc;
}
#endif

#if ZXIC_REAL("Advanced Function")
/***********************************************************/
/** ppu计数值获取
* @param   dev_id           设备号
* @param   rd_mode          读取位宽模式，参见STAT_CNT_MODE_E，0-64bit，1-128bit
* @param   index            索引，具体位宽参见rd_mode
* @param   clr_mode         读清模式，参见STAT_RD_CLR_MODE_E，0-不读清，1-读清
* @param   p_data           出参，读取的数据
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/07/11
************************************************************/
DPP_STATUS dpp_stat_ppu_cnt_get(DPP_DEV_T *dev,
                                STAT_CNT_MODE_E rd_mode,
                                ZXIC_UINT32 index,
                                ZXIC_UINT32 clr_mode,
                                ZXIC_UINT32 *p_data)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 ppu_eram_baddr = 0;
    ZXIC_UINT32 ppu_eram_depth = 0;
    ZXIC_UINT32 ppu_ddr_baddr  = 0;
    ZXIC_UINT32 eram_rd_mode   = 0;
    ZXIC_UINT32 eram_clr_mode  = 0;
    // ZXIC_UINT32 ddr_rd_mode    = 0;
    // ZXIC_UINT32 ddr_clr_mode   = 0;
    // ZXIC_UINT32 ddr_index      = 0;

    ZXIC_COMM_CHECK_POINT(dev);

    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), rd_mode, STAT_64_MODE, STAT_MAX_MODE - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), clr_mode, STAT_RD_CLR_MODE_UNCLR, STAT_RD_CLR_MODE_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);

    rc = dpp_stat_ppu_eram_depth_get(dev,
                                     &ppu_eram_depth);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_ppu_eram_depth_get");

    rc = dpp_stat_ppu_eram_baddr_get(dev,
                                     &ppu_eram_baddr);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_ppu_eram_baddr_get");

    rc = dpp_stat_ppu_ddr_baddr_get(dev,
                                    &ppu_ddr_baddr);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_ppu_ddr_baddr_get");

    /** 片内存储 */
    if ((index >> (STAT_128_MODE - rd_mode)) < ppu_eram_depth)
    {
        if (STAT_128_MODE == rd_mode)
        {
            eram_rd_mode = ERAM128_OPR_128b;
        }
        else
        {
            eram_rd_mode = ERAM128_OPR_64b;
        }

        if (STAT_RD_CLR_MODE_UNCLR == clr_mode)
        {
            eram_clr_mode = RD_MODE_HOLD;
        }
        else
        {
            eram_clr_mode = RD_MODE_CLEAR;
        }

        rc = dpp_se_smmu0_ind_read(dev,
                                   ppu_eram_baddr,
                                   index,
                                   eram_rd_mode,
                                   eram_clr_mode,
                                   p_data);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_se_smmu0_ind_read");
    }
    /** 片外存储 */
    else
    {
        // if (STAT_128_MODE == rd_mode)
        // {
        //     ddr_rd_mode = CMMU_RD_MODE_128;
        // }
        // else
        // {
        //     ddr_rd_mode = CMMU_RD_MODE_64;
        // }

        // if (STAT_RD_CLR_MODE_UNCLR == clr_mode)
        // {
        //     ddr_clr_mode = CMMU_RD_CLR_MODE_UNCLR;
        // }
        // else
        // {
        //     ddr_clr_mode = CMMU_RD_CLR_MODE_CLR;
        // }

        // ddr_index = index - (ppu_eram_depth << (STAT_128_MODE - rd_mode));

        // rc = dpp_se_cmmu_ddr_read(dev_id,
        //                           ppu_ddr_baddr,
        //                           ddr_rd_mode,
        //                           ddr_clr_mode,
        //                           ddr_index,
        //                           p_data);
        // ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_se_cmmu_ddr_read");
    }

    return rc;
}

/***********************************************************/
/** 设置ppu统计片内深度
* @param   dev_id           设备号
* @param   ppu_eram_depth   ppu统计片内深度,128bit为单位
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/03/31
************************************************************/
DPP_STATUS dpp_stat_ppu_eram_depth_set(DPP_DEV_T *dev, ZXIC_UINT32 ppu_eram_depth)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_STAT_CFG_PPU_ERAM_DEPTH_T ppu_eram_depth_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), ppu_eram_depth, 0, DPP_STAT_PPU_ERAM_DEPTH_MAX);

    ppu_eram_depth_cfg.ppu_eram_depth = ppu_eram_depth;

    rc = dpp_reg_write(dev,
                       STAT_STAT_CFG_PPU_ERAM_DEPTHr,
                       0,
                       0,
                       &ppu_eram_depth_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    g_ppu_stat_cfg.eram_depth = ppu_eram_depth;

    return rc;
}

/***********************************************************/
/** 设置ppu统计 ERAM基地址
* @param   dev_id           设备号
* @param   ppu_eram_baddr   ppu统计eRam基地址,128bit为单位
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/03/31
************************************************************/
DPP_STATUS dpp_stat_ppu_eram_baddr_set(DPP_DEV_T *dev, ZXIC_UINT32 ppu_eram_baddr)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_STAT_CFG_PPU_ERAM_BASE_ADDR_T ppu_eram_baddr_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), ppu_eram_baddr, 0, DPP_STAT_PPU_ERAM_BADDR_MAX);

    ppu_eram_baddr_cfg.ppu_eram_base_addr = ppu_eram_baddr;

    rc = dpp_reg_write(dev,
                       STAT_STAT_CFG_PPU_ERAM_BASE_ADDRr,
                       0,
                       0,
                       &ppu_eram_baddr_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    g_ppu_stat_cfg.eram_baddr = ppu_eram_baddr;

    return rc;
}

#endif
