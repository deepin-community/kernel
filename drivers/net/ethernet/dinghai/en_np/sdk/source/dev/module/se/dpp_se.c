/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_se.c
* 文件标识 :
* 内容摘要 : 芯片se模块基本接口函数实现
* 其它说明 :
* 当前版本 :
* 完成日期 : 2014/03/11
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
#include "dpp_se_api.h"
#include "dpp_etcam.h"
#include "dpp_se.h"
#include "dpp_dev.h"
#include "dpp_se_cfg.h"
#include "dpp_sdt.h"
#include "dpp_smmu14k_reg.h"
#include "dpp_se4k_reg.h"

#define SE_OPR_WR         (0)
#define SE_OPR_RD         (1)
#define SE_CLS_MAX        (8)
#define SE_SMMU1_PAGE_MAX (5)
#define SE_LPMID_OFF      (4)

static SMMU1_KSCHD_HASH_DDR_CFG_T g_smmu1_kschd_hash[DPP_DEV_CHANNEL_MAX][DPP_HASH_ID_NUM][HASH_BULK_NUM] = {{{{0}}}};

ZXIC_UINT32 g_lpm_dat_wr_type_flag = 2; /* 1:开启DMA 2:REG mode */
ZXIC_UINT8 g_lpm_hw_dat_buf[LPM_HW_DAT_BUFF_SIZE_MAX] = {0};
ZXIC_UINT32 g_lpm_hw_dat_offset = 0;

#define GET_SMMU1_KSCHD_HASH_CFG(dev_id,hash_id,bulk_id) (&g_smmu1_kschd_hash[dev_id][hash_id][bulk_id])

#define SMMU1_WDAT0_R      (SYS_SE_BASE_ADDR + MODULE_SE_SMMU1_BASE_ADDR + 0x0)
#define SMMU1_CMD0_R       (SYS_SE_BASE_ADDR + MODULE_SE_SMMU1_BASE_ADDR + 0x40)
#define SMMU1_CMD1_R       (SYS_SE_BASE_ADDR + MODULE_SE_SMMU1_BASE_ADDR + 0x44)
#define SMMU1_CMD0_F_ADDR_START          (30)
#define SMMU1_CMD0_F_ADDR_WIDTH          (2)
#define SMMU1_CMD0_F_ECC_EN_START        (5)
#define SMMU1_CMD0_F_ECC_EN_WIDTH        (1)
#define SMMU1_CMD0_F_DDR_MODE_START      (3)
#define SMMU1_CMD0_F_DDR_MODE_WIDTH      (2)
#define SMMU1_CMD0_F_BK_INFO_START       (0)
#define SMMU1_CMD0_F_BK_INFO_WIDTH       (3)

#define SMMU1_CMD1_F_DDR_WR_START        (30)
#define SMMU1_CMD1_F_DDR_WR_WIDTH        (1)
#define SMMU1_CMD1_F_ADDR_START          (0)
#define SMMU1_CMD1_F_ADDR_WIDTH          (30)


#if ZXIC_REAL("SMMU0")
/***********************************************************/
/** SE通用完成状态检查
* @param   dev_id    设备号
* @param   reg_no    寄存器编号
* @param   pos       所要读的寄存器的done_flag的位置(只支持1bit)
*
* @return
* @remark  无
* @see
* @author  wyt      @date  2018/07/09
************************************************************/
DPP_STATUS dpp_se_done_status_check(DPP_DEV_T *dev, ZXIC_UINT32 reg_no, ZXIC_UINT32 pos)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 data = 0;
    ZXIC_UINT32 rd_cnt = 0;
    ZXIC_UINT32 done_flag = 0;

    /* 打桩使用 */
#ifdef  DPP_FOR_LLT   
    ZXIC_UINT32 done_sig = 0xffffffff;
    
    rc = dpp_reg_write32(dev_id, reg_no, done_sig);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_reg_write32");
#endif

    ZXIC_COMM_CHECK_POINT(dev);

    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev), DPP_DEV_CHANNEL_MAX - 1);

    while (!done_flag)
    {
        rc = dpp_reg_read32(dev, reg_no, 0, 0, &data);
        if(ZXIC_OK != rc)
        {
           ZXIC_COMM_TRACE_ERROR("\n [ErrorCode:0x%x] !-- dpp_reg_read32 Fail!\n", rc);
           return rc;
        }

        done_flag = (data >> pos) & 0x1;

        if (done_flag)
        {
            break;
        }

        if (rd_cnt > DPP_RD_CNT_MAX * DPP_RD_CNT_MAX)
        {
            ZXIC_COMM_TRACE_ERROR("Error!!! dpp se rd reg_no [%d] is overtime!\n", reg_no);
            return DPP_ERR;
        }

        rd_cnt++;
        /*zxic_comm_usleep(1000);*/
    }

    return rc;
}

/***********************************************************/
/** 写eRam
* @param   dev_id    设备号
* @param   base_addr 基地址，以128bit为单位
* @param   index     条目索引
* @param   wrt_mode  数据位宽模式, 取值参考ERAM128_OPR_MODE_E的定义
* @param   p_data    数据
*
* @return
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_se_smmu0_ind_write(DPP_DEV_T *dev,
                                  ZXIC_UINT32 base_addr,
                                  ZXIC_UINT32 index,
                                  ZXIC_UINT32 wrt_mode,
                                  ZXIC_UINT32 *p_data)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 temp_idx = 0;
    ZXIC_MUTEX_T *p_ind_mutex = NULL;

    DPP_SMMU0_SMMU0_CPU_IND_CMD_T cpu_ind_cmd = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev),DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), wrt_mode, ERAM128_OPR_128b, ERAM128_OPR_1b);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), base_addr, 0, SE_SMMU0_ERAM_ADDR_NUM_TOTAL - 1);

    rc = dpp_dev_opr_mutex_get(dev, DPP_DEV_MUTEX_T_SMMU0, &p_ind_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_ind_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_lock");

    rc = dpp_se_done_status_check(dev, SMMU0_SMMU0_WR_ARB_CPU_RDYr, 0);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_se_done_status_check", p_ind_mutex);

    switch (wrt_mode)
    {
        case ERAM128_OPR_128b:
        {
            if((0xFFFFFFFF - (base_addr)) < (index))
            {
                ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev),  "ICM %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, base_addr, index);
                rc = zxic_comm_mutex_unlock(p_ind_mutex);
                ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
                return ZXIC_PAR_CHK_INVALID_INDEX;
            }
            if ((base_addr + index) > (SE_SMMU0_ERAM_ADDR_NUM_TOTAL - 1))
            {
                ZXIC_COMM_PRINT("dpp_se_smmu0_ind_write : index out of range !\n");
                rc = zxic_comm_mutex_unlock(p_ind_mutex);
                ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
                return DPP_ERR;
            }

            temp_idx = index << 7;

            for (i = 0; i < 4; i++)
            {
                rc = dpp_reg_write(dev,
                                   SMMU0_SMMU0_CPU_IND_WDAT0r + i,
                                   0,
                                   0,
                                   p_data + 3 - i);
                ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_reg_write", p_ind_mutex);

            }

            break;
        }

        case ERAM128_OPR_64b:
        {
            if ((base_addr + (index >> 1)) > SE_SMMU0_ERAM_ADDR_NUM_TOTAL - 1)
            {
                ZXIC_COMM_PRINT("dpp_se_smmu0_ind_write : index out of range !\n");
                rc = zxic_comm_mutex_unlock(p_ind_mutex);
                ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
                return DPP_ERR;
            }

            temp_idx = index << 6;

            for (i = 0; i < 2; i++)
            {
                rc = dpp_reg_write(dev,
                                   SMMU0_SMMU0_CPU_IND_WDAT0r + i,
                                   0,
                                   0,
                                   p_data + 1 - i);
                ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_reg_write", p_ind_mutex);

            }

            break;
        }

        case ERAM128_OPR_1b:
        {
            if ((base_addr + (index >> 7)) > SE_SMMU0_ERAM_ADDR_NUM_TOTAL - 1)
            {
                ZXIC_COMM_PRINT("dpp_se_smmu0_ind_write : index out of range !\n");
                rc = zxic_comm_mutex_unlock(p_ind_mutex);
                ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
                return DPP_ERR;
            }

            temp_idx = index;
            rc = dpp_reg_write(dev,
                               SMMU0_SMMU0_CPU_IND_WDAT0r,
                               0,
                               0,
                               p_data);
            ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_reg_write", p_ind_mutex);
            break;
        }
    }

    cpu_ind_cmd.cpu_ind_rw = SE_OPR_WR;
    cpu_ind_cmd.cpu_req_mode = wrt_mode;
    if((0xFFFFFFFF - (temp_idx)) < ((base_addr << 7) & DPP_ERAM128_BADDR_MASK))
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev),  "ICM %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, temp_idx, ((base_addr << 7) & DPP_ERAM128_BADDR_MASK));
        rc = zxic_comm_mutex_unlock(p_ind_mutex);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
        return ZXIC_PAR_CHK_INVALID_INDEX;
    }
    cpu_ind_cmd.cpu_ind_addr = ((base_addr << 7) & DPP_ERAM128_BADDR_MASK) + temp_idx;

    rc = dpp_reg_write(dev,
                       SMMU0_SMMU0_CPU_IND_CMDr,
                       0,
                       0,
                       &cpu_ind_cmd);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_reg_write", p_ind_mutex);

    rc = zxic_comm_mutex_unlock(p_ind_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");

    return DPP_OK;
}

/***********************************************************/
/** 读eRam
* @param   dev_id       设备号
* @param   base_addr    基地址，以128bit为单位
* @param   index        条目索引,支持128、64、32和1bit的索引值
* @param   rd_mode      读eRam模式, 取值参照ERAM128_OPR_MODE_E定义，读清模式下不支持1bit模式
* @param   rd_clr_mode  eRam读清模式, 取值参照ERAM128_RD_CLR_MODE_E定义
* @param   p_data       返回数据缓存的指针
*
* @return
* @remark  无
* @see
* @author  wcl      @date  2015/01/30
************************************************************/
DPP_STATUS dpp_se_smmu0_ind_read(DPP_DEV_T *dev,
                                 ZXIC_UINT32 base_addr,
                                 ZXIC_UINT32 index,
                                 ZXIC_UINT32 rd_mode,
                                 ZXIC_UINT32 rd_clr_mode,
                                 ZXIC_UINT32 *p_data)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 row_index = 0;
    ZXIC_UINT32 col_index = 0;
    ZXIC_UINT32 temp_data[4] = {0};
    ZXIC_UINT32 *p_temp_data = NULL;

    DPP_SMMU0_SMMU0_CPU_IND_CMD_T cpu_ind_cmd = {0};

    ZXIC_MUTEX_T *p_ind_mutex = NULL;

    ZXIC_COMM_CHECK_POINT(dev);

    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev),DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), rd_clr_mode, RD_MODE_HOLD, RD_MODE_CLEAR);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), rd_mode, ERAM128_OPR_128b, ERAM128_OPR_32b);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), base_addr, 0, SE_SMMU0_ERAM_ADDR_NUM_TOTAL - 1);

    rc = dpp_dev_opr_mutex_get(dev, DPP_DEV_MUTEX_T_SMMU0, &p_ind_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_ind_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_lock");

    rc = dpp_se_done_status_check(dev, SMMU0_SMMU0_WR_ARB_CPU_RDYr, 0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "time_out", p_ind_mutex);

    /**  正常模式读数据，仅能以128bit读出数据，软件提取数据*/
    if (RD_MODE_HOLD == rd_clr_mode)
    {
        cpu_ind_cmd.cpu_ind_rw = SE_OPR_RD;
        cpu_ind_cmd.cpu_ind_rd_mode = RD_MODE_HOLD;
        cpu_ind_cmd.cpu_req_mode = ERAM128_OPR_128b;

        /* 先以128bit模式读出数据 */
        switch (rd_mode)
        {
            case ERAM128_OPR_128b:
            {
                if((0xFFFFFFFF - (base_addr)) < (index))
                {
                   ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev),  "ICM %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, base_addr, index);
                   rc = zxic_comm_mutex_unlock(p_ind_mutex);
                   ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
                   return ZXIC_PAR_CHK_INVALID_INDEX;
                }
                if (base_addr + index > SE_SMMU0_ERAM_ADDR_NUM_TOTAL - 1)
                {
                    ZXIC_COMM_PRINT("dpp_se_smmu0_ind_read : index out of range !\n");
                    rc = zxic_comm_mutex_unlock(p_ind_mutex);
                    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
                    return DPP_ERR;
                }

                row_index = (index << 7) & DPP_ERAM128_BADDR_MASK;
                break;
            }

            case ERAM128_OPR_64b:
            {
                if ((base_addr + (index >> 1)) > SE_SMMU0_ERAM_ADDR_NUM_TOTAL - 1)
                {
                    ZXIC_COMM_PRINT("dpp_se_smmu0_ind_read : index out of range !\n");
                    rc = zxic_comm_mutex_unlock(p_ind_mutex);
                    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
                    return DPP_ERR;
                }

                row_index = (index << 6) & DPP_ERAM128_BADDR_MASK;
                col_index = index & 0x1;
                break;
            }

            case ERAM128_OPR_32b:
            {
                if ((base_addr + (index >> 2)) > SE_SMMU0_ERAM_ADDR_NUM_TOTAL - 1)
                {
                    ZXIC_COMM_PRINT("dpp_se_smmu0_ind_read : index out of range !\n");
                    rc = zxic_comm_mutex_unlock(p_ind_mutex);
                    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
                    return DPP_ERR;
                }

                row_index = (index << 5) & DPP_ERAM128_BADDR_MASK;
                col_index = index & 0x3;
                break;
            }

            case ERAM128_OPR_1b:
            {
                if ((base_addr + (index >> 7)) > SE_SMMU0_ERAM_ADDR_NUM_TOTAL - 1)
                {
                    ZXIC_COMM_PRINT("dpp_se_smmu0_ind_read : index out of range !\n");
                    rc = zxic_comm_mutex_unlock(p_ind_mutex);
                    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
                    return DPP_ERR;
                }

                row_index = index & DPP_ERAM128_BADDR_MASK;
                col_index = index & 0x7F;
                break;
            }
        }

        cpu_ind_cmd.cpu_ind_addr = ((base_addr << 7) & DPP_ERAM128_BADDR_MASK) + row_index;
    }
    /** 读清模式*/
    else
    {
        cpu_ind_cmd.cpu_ind_rw = SE_OPR_RD;
        cpu_ind_cmd.cpu_ind_rd_mode = RD_MODE_CLEAR;

        switch (rd_mode)
        {
            case ERAM128_OPR_128b:
            {
                if((0xFFFFFFFF - (base_addr)) < (index))
                {
                    ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev),  "ICM %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, base_addr, index);
                    rc = zxic_comm_mutex_unlock(p_ind_mutex);
                    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
                    return ZXIC_PAR_CHK_INVALID_INDEX;
                }
                if (base_addr + index > SE_SMMU0_ERAM_ADDR_NUM_TOTAL - 1)
                {
                    ZXIC_COMM_PRINT("dpp_se_smmu0_ind_read : index out of range !\n");
                    rc = zxic_comm_mutex_unlock(p_ind_mutex);
                    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
                    return DPP_ERR;
                }

                row_index = (index << 7);
                cpu_ind_cmd.cpu_req_mode = ERAM128_OPR_128b;
                break;
            }

            case ERAM128_OPR_64b:
            {
                if ((base_addr + (index >> 1)) > SE_SMMU0_ERAM_ADDR_NUM_TOTAL - 1)
                {
                    ZXIC_COMM_PRINT("dpp_se_smmu0_ind_read : index out of range !\n");
                    rc = zxic_comm_mutex_unlock(p_ind_mutex);
                    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
                    return DPP_ERR;
                }

                row_index = (index << 6);
                cpu_ind_cmd.cpu_req_mode = 2;
                break;
            }

            case ERAM128_OPR_32b:
            {
                if ((base_addr + (index >> 2)) > SE_SMMU0_ERAM_ADDR_NUM_TOTAL - 1)
                {
                    ZXIC_COMM_PRINT("dpp_se_smmu0_ind_read : index out of range !\n");
                    rc = zxic_comm_mutex_unlock(p_ind_mutex);
                    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
                    return DPP_ERR;
                }

                row_index = (index << 5);
                cpu_ind_cmd.cpu_req_mode = 1;
                break;
            }

            case ERAM128_OPR_1b:
            {
                ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "Param Error! rd_clr_mode[%d] or rd_mode[%d] error!\n ", rd_clr_mode, rd_mode);
                rc = zxic_comm_mutex_unlock(p_ind_mutex);
                ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
                ZXIC_COMM_ASSERT(0);
                return DPP_ERR;
            }
        }

        cpu_ind_cmd.cpu_ind_addr = ((base_addr << 7) & DPP_ERAM128_BADDR_MASK) + row_index;
    }

    rc = dpp_reg_write(dev,
                       SMMU0_SMMU0_CPU_IND_CMDr,
                       0,
                       0,
                       &cpu_ind_cmd);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_reg_write", p_ind_mutex);

    rc = dpp_se_done_status_check(dev, SMMU0_SMMU0_CPU_IND_RD_DONEr, 0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "time_out", p_ind_mutex);

    p_temp_data = temp_data;
    for (i = 0; i < 4; i++)
    {
/*
        rc = dpp_reg_read(dev_id,
                          SMMU0_SMMU0_CPU_IND_RDAT0r + i,
                          0,
                          0,
                          temp_data + 3 - i);
        ZXIC_COMM_CHECK_DEV_RC_UNLOCK(dev_id, rc, "dpp_reg_read", p_ind_mutex);
*/
        rc = dpp_reg_read(dev,
                          SMMU0_SMMU0_CPU_IND_RDAT0r + i,
                          0,
                          0,
                          p_temp_data + 3 - i);
        ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_reg_read", p_ind_mutex);
    }

    /**  正常模式读数据，仅能以128bit读出数据，软件提取数据*/
    if (RD_MODE_HOLD == rd_clr_mode)
    {
        switch (rd_mode)
        {
            case ERAM128_OPR_128b:
            {
                /* ZXIC_COMM_MEMCPY(p_data, &temp_data[0], (128 / 8)); */
                /* modify by ghm for coverity @20200714 */
                ZXIC_COMM_MEMCPY(p_data, p_temp_data, (128 / 8));
                break;
            }

            case ERAM128_OPR_64b:
            {
                /* ZXIC_COMM_MEMCPY(p_data, &temp_data[(1 - col_index) << 1], (64 / 8)); */
                /* modify by ghm for coverity @20200714 */
                ZXIC_COMM_MEMCPY(p_data, p_temp_data + ((1 - col_index) << 1), (64 / 8));
                break;
            }

            case ERAM128_OPR_32b:
            {
                /* ZXIC_COMM_MEMCPY(p_data, &temp_data[(3 - col_index)], (32 / 8)); */
                /* modify by ghm for coverity @20200714 */
                ZXIC_COMM_MEMCPY(p_data, p_temp_data + ((3 - col_index)), (32 / 8));
                break;
            }

            case ERAM128_OPR_1b:
            {
                /* ZXIC_COMM_UINT32_GET_BITS(p_data[0], temp_data[3 - col_index / 32], (col_index % 32), 1); */
                /* modify by ghm for coverity @20200714 */
                ZXIC_COMM_UINT32_GET_BITS(p_data[0], *(p_temp_data + (3 - col_index / 32)), (col_index % 32), 1);
                break;
            }
        }
    }
    else /** 读清模式*/
    {
        switch (rd_mode)
        {
            case ERAM128_OPR_128b:
            {
                /* ZXIC_COMM_MEMCPY(p_data, temp_data, (128 / 8)); */
                /* modify by ghm for coverity @20200714 */
                ZXIC_COMM_MEMCPY(p_data, p_temp_data, (128 / 8));
                break;
            }

            case ERAM128_OPR_64b:
            {
                /* ZXIC_COMM_MEMCPY(p_data, temp_data, (64 / 8)); */
                /* modify by ghm for coverity @20200714 */
                ZXIC_COMM_MEMCPY(p_data, p_temp_data, (64 / 8));
                break;
            }

            case ERAM128_OPR_32b:
            {
                /* ZXIC_COMM_MEMCPY(p_data, temp_data, (32 / 8)); */
                /* modify by ghm for coverity @20200714 */
                ZXIC_COMM_MEMCPY(p_data, p_temp_data, (64 / 8));
                break;
            }
        }
    }

    rc = zxic_comm_mutex_unlock(p_ind_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");

    return rc;
}



#endif

#if ZXIC_REAL("SMMU1")
/***********************************************************/
/** dpp hash的smmu1属性设置
* @param   dev_id    设备号
* @param   hash_id      hash引擎号
* @param   tbl_id       hash表号
* @param   ecc_en       ecc使能
* @param   baddr        ddr基地址
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/12
************************************************************/
DPP_STATUS dpp_se_smmu1_hash_tbl_cfg_set(DPP_DEV_T *dev,
                                         ZXIC_UINT32 hash_id,
                                         ZXIC_UINT32 tbl_id,
                                         ZXIC_UINT32 ecc_en,
                                         ZXIC_UINT32 baddr)
{
    DPP_STATUS rc = DPP_OK;

    DPP_SMMU14K_SE_SMMU1_HASH0_TBL0_CFG_T hash_tbl_cfg = {0};

    SMMU1_KSCHD_HASH_DDR_CFG_T *p_hash_ddr_cfg = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev),DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_UPPER(hash_id,DPP_HASH_ID_MAX);
    ZXIC_COMM_CHECK_INDEX_UPPER(tbl_id,HASH_BULK_ID_MAX);
    ZXIC_COMM_CHECK_INDEX_UPPER(ecc_en,1);
    ZXIC_COMM_CHECK_INDEX_UPPER(baddr,DPP_SE_SMMU1_MAX_BADDR_NO_SHARE);

    /* save to soft buffer */
    p_hash_ddr_cfg = GET_SMMU1_KSCHD_HASH_CFG(DEV_ID(dev), hash_id, tbl_id);
    p_hash_ddr_cfg->baddr = baddr;
    p_hash_ddr_cfg->crcen = ecc_en;
    p_hash_ddr_cfg->mode = SMMU1_DDR_SRH_256b; /* alg search ddr mode, not write mode */

    hash_tbl_cfg.hash0_tbl0_ecc_en = ecc_en;
    hash_tbl_cfg.hash0_tbl0_baddr = baddr;

#ifdef DPP_FLOW_HW_INIT
    rc = dpp_reg_write(dev,
                       SMMU14K_SE_SMMU1_HASH0_TBL0_CFGr + hash_id * HASH_BULK_NUM + tbl_id,
                       0,
                       0,
                       &hash_tbl_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");
#endif

    return rc;
}

/***********************************************************/
/** 获取hash算法访问DDR空间的属性，从软件获取(待优化)
* @param   dev_id    设备号
* @param   hash_id      hash引擎号
* @param   bulk_id      Hash引擎存储资源划分块数的ID号
* @param   p_ecc_en     使能ECC校验
* @param   p_base_addr  DDR空间基地址
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  tf      @date  2016/06/15
************************************************************/
DPP_STATUS dpp_se_smmu1_hash_tbl_soft_cfg_get(DPP_DEV_T *dev,
                                              ZXIC_UINT32 hash_id,
                                              ZXIC_UINT32 bulk_id,
                                              ZXIC_UINT32 *p_ecc_en,
                                              ZXIC_UINT32 *p_base_addr)
{

    SMMU1_KSCHD_HASH_DDR_CFG_T *p_schd_hash = NULL;

    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev),DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_UPPER(hash_id,DPP_HASH_ID_MAX);
    ZXIC_COMM_CHECK_INDEX_UPPER(bulk_id,HASH_BULK_ID_MAX);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_base_addr);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_ecc_en);

    /* read from soft buffer */
    p_schd_hash = GET_SMMU1_KSCHD_HASH_CFG(DEV_ID(dev), hash_id, bulk_id);
    *p_base_addr = p_schd_hash->baddr;
    *p_ecc_en = p_schd_hash->crcen;

    return DPP_OK;
}

#endif

#if ZXIC_REAL("ALG")
/***********************************************************/
/** 设置zblock service信息
* @param   dev_id
* @param   zblk_idx   zblock索引(2bit zgroup + 3bit zblock)
* @param   serv_sel   0-lpm, 1-hash
* @param   hash_id    zblock对应的hash_id, lpm不关心此字段
* @param   enable     业务表使能标志
*
* @return
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_se_zblk_serv_cfg_set(DPP_DEV_T *dev,
                                    ZXIC_UINT32 zblk_idx,
                                    ZXIC_UINT32 serv_sel,
                                    ZXIC_UINT32 hash_id,
                                    ZXIC_UINT32 enable)
{
    DPP_STATUS rtn = DPP_OK;

    DPP_SE4K_SE_ALG_ZBLOCK_SERVICE_CONFIGURE_T zblk_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev),DPP_DEV_CHANNEL_MAX - 1);

    zblk_cfg.hash_channel_sel = hash_id;
    zblk_cfg.service_sel = serv_sel;
    zblk_cfg.st_en = enable;
    rtn = dpp_reg_write(dev,
                         SE4K_SE_ALG_ZBLOCK_SERVICE_CONFIGUREr,
                        ((ZBLK_ADDR_CONV(zblk_idx) >> 3) & 0x3),
                        (ZBLK_ADDR_CONV(zblk_idx) & 0x7),
                        &zblk_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 设置业务表独占的zcell
* @param   dev_id
* @param   zblk_idx           zblock索引(2bit zgroup + 3bit zblock)
* @param   zcell0_bulk_id      独占zcell0的业务表号
* @param   zcell0_mono_flag   zcell0被独占的标志
* @param   zcell1_bulk_id      独占zcell1的业务表号
* @param   zcell1_mono_flag   zcell1被独占的标志
* @param   zcell2_bulk_id      独占zcell2的业务表号
* @param   zcell2_mono_flag   zcell2被独占的标志
* @param   zcell3_bulk_id      独占zcell3的业务表号
* @param   zcell3_mono_flag   zcell3被独占的标志
*
* @return
* @remark  无
* @see
* @author  tf      @date  2016/04/26
************************************************************/
DPP_STATUS dpp_se_zcell_mono_cfg_set(DPP_DEV_T *dev,
                                     ZXIC_UINT32 zblk_idx,
                                     ZXIC_UINT32 zcell0_bulk_id,
                                     ZXIC_UINT32 zcell0_mono_flag,
                                     ZXIC_UINT32 zcell1_bulk_id,
                                     ZXIC_UINT32 zcell1_mono_flag,
                                     ZXIC_UINT32 zcell2_bulk_id,
                                     ZXIC_UINT32 zcell2_mono_flag,
                                     ZXIC_UINT32 zcell3_bulk_id,
                                     ZXIC_UINT32 zcell3_mono_flag)
{
    DPP_STATUS rtn = DPP_OK;

    DPP_SE4K_SE_ALG_ZBLOCK_HASH_ZCELL_MONO_T zblk_zcell_mono_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev),DPP_DEV_CHANNEL_MAX - 1);

    zblk_zcell_mono_cfg.ha_zcell0_tbl_id = zcell0_bulk_id;
    zblk_zcell_mono_cfg.ha_zcell0_mono_flag = zcell0_mono_flag;
    zblk_zcell_mono_cfg.ha_zcell1_tbl_id = zcell1_bulk_id;
    zblk_zcell_mono_cfg.ha_zcell1_mono_flag = zcell1_mono_flag;
    zblk_zcell_mono_cfg.ha_zcell2_tbl_id = zcell2_bulk_id;
    zblk_zcell_mono_cfg.ha_zcell2_mono_flag = zcell2_mono_flag;
    zblk_zcell_mono_cfg.ha_zcell3_tbl_id = zcell3_bulk_id;
    zblk_zcell_mono_cfg.ha_zcell3_mono_flag = zcell3_mono_flag;

    rtn = dpp_reg_write(dev,
                        SE4K_SE_ALG_ZBLOCK_HASH_ZCELL_MONOr,
                        ((ZBLK_ADDR_CONV(zblk_idx) >> 3) & 0x3),
                        (ZBLK_ADDR_CONV(zblk_idx) & 0x7),
                        &zblk_zcell_mono_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 获取业务表独占的zcell
* @param   dev_id
* @param   zblk_idx
* @param   *zcell0_bulk_id
* @param   *zcell0_mono_flag
* @param   *zcell1_bulk_id
* @param   *zcell1_mono_flag
* @param   *zcell2_bulk_id
* @param   *zcell2_mono_flag
* @param   *zcell3_bulk_id
* @param   *zcell3_mono_flag
*
* @return
* @remark  无
* @see
* @author  tf      @date  2016/04/26
************************************************************/
DPP_STATUS dpp_se_zcell_mono_cfg_get(DPP_DEV_T *dev,
                                     ZXIC_UINT32 zblk_idx,
                                     ZXIC_UINT32 *zcell0_bulk_id,
                                     ZXIC_UINT32 *zcell0_mono_flag,
                                     ZXIC_UINT32 *zcell1_bulk_id,
                                     ZXIC_UINT32 *zcell1_mono_flag,
                                     ZXIC_UINT32 *zcell2_bulk_id,
                                     ZXIC_UINT32 *zcell2_mono_flag,
                                     ZXIC_UINT32 *zcell3_bulk_id,
                                     ZXIC_UINT32 *zcell3_mono_flag)
{
    DPP_STATUS rtn = DPP_OK;

    DPP_SE4K_SE_ALG_ZBLOCK_HASH_ZCELL_MONO_T zblk_zcell_mono_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev),DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), zcell0_bulk_id);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), zcell0_mono_flag);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), zcell1_bulk_id);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), zcell1_mono_flag);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), zcell2_bulk_id);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), zcell2_mono_flag);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), zcell3_bulk_id);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), zcell3_mono_flag);

    rtn = dpp_reg_read(dev,
                       SE4K_SE_ALG_ZBLOCK_HASH_ZCELL_MONOr,
                       ((ZBLK_ADDR_CONV(zblk_idx) >> 3) & 0x3),
                       (ZBLK_ADDR_CONV(zblk_idx) & 0x7),
                       &zblk_zcell_mono_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_reg_write");

    *zcell0_bulk_id = zblk_zcell_mono_cfg.ha_zcell0_tbl_id;
    *zcell0_mono_flag = zblk_zcell_mono_cfg.ha_zcell0_mono_flag;
    *zcell1_bulk_id = zblk_zcell_mono_cfg.ha_zcell1_tbl_id;
    *zcell1_mono_flag = zblk_zcell_mono_cfg.ha_zcell1_mono_flag;
    *zcell2_bulk_id = zblk_zcell_mono_cfg.ha_zcell2_tbl_id;
    *zcell2_mono_flag = zblk_zcell_mono_cfg.ha_zcell2_mono_flag;
    *zcell3_bulk_id = zblk_zcell_mono_cfg.ha_zcell3_tbl_id;
    *zcell3_mono_flag = zblk_zcell_mono_cfg.ha_zcell3_mono_flag;

    return DPP_OK;
}

/***********************************************************/
/** 设置业务表独占的zreg
* @param   dev_id
* @param   zblk_idx          zblock索引(2bit zgroup + 3bit zblock)
* @param   zreg0_bulk_id      独占zreg0的业务表号
* @param   zreg0_mono_flag   zreg0被独占的标志
* @param   zreg1_bulk_id      独占zreg1的业务表号
* @param   zreg1_mono_flag   zreg1被独占的标志
* @param   zreg2_bulk_id      独占zreg2的业务表号
* @param   zreg2_mono_flag   zreg2被独占的标志
* @param   zreg3_bulk_id      独占zreg3的业务表号
* @param   zreg3_mono_flag   zreg3被独占的标志
*
* @return
* @remark  无
* @see
* @author  tf      @date  2016/04/27
************************************************************/
DPP_STATUS dpp_se_zreg_mono_cfg_set(DPP_DEV_T *dev,
                                    ZXIC_UINT32 zblk_idx,
                                    ZXIC_UINT32 zreg0_bulk_id,
                                    ZXIC_UINT32 zreg0_mono_flag,
                                    ZXIC_UINT32 zreg1_bulk_id,
                                    ZXIC_UINT32 zreg1_mono_flag,
                                    ZXIC_UINT32 zreg2_bulk_id,
                                    ZXIC_UINT32 zreg2_mono_flag,
                                    ZXIC_UINT32 zreg3_bulk_id,
                                    ZXIC_UINT32 zreg3_mono_flag)
{
    DPP_STATUS rtn = DPP_OK;

    DPP_SE4K_SE_ALG_ZLOCK_HASH_ZREG_MONO_T zblk_zreg_mono_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev),DPP_DEV_CHANNEL_MAX - 1);

    zblk_zreg_mono_cfg.ha_zreg0_tbl_id = zreg0_bulk_id;
    zblk_zreg_mono_cfg.ha_zreg0_mono_flag = zreg0_mono_flag;
    zblk_zreg_mono_cfg.ha_zreg1_tbl_id = zreg1_bulk_id;
    zblk_zreg_mono_cfg.ha_zreg1_mono_flag = zreg1_mono_flag;
    zblk_zreg_mono_cfg.ha_zreg2_tbl_id = zreg2_bulk_id;
    zblk_zreg_mono_cfg.ha_zreg2_mono_flag = zreg2_mono_flag;
    zblk_zreg_mono_cfg.ha_zreg3_tbl_id = zreg3_bulk_id;
    zblk_zreg_mono_cfg.ha_zreg3_mono_flag = zreg3_mono_flag;

    rtn = dpp_reg_write(dev,
                        SE4K_SE_ALG_ZLOCK_HASH_ZREG_MONOr,
                        ((ZBLK_ADDR_CONV(zblk_idx) >> 3) & 0x3),
                        (ZBLK_ADDR_CONV(zblk_idx) & 0x7),
                        &zblk_zreg_mono_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 获取业务表独占的zcell
* @param   dev_id
* @param   zblk_idx
* @param   *zreg0_bulk_id
* @param   *zreg0_mono_flag
* @param   *zreg1_bulk_id
* @param   *zreg1_mono_flag
* @param   *zreg2_bulk_id
* @param   *zreg2_mono_flag
* @param   *zreg3_bulk_id
* @param   *zreg3_mono_flag
*
* @return
* @remark  无
* @see
* @author  tf      @date  2016/04/27
************************************************************/
DPP_STATUS dpp_se_zreg_mono_cfg_get(DPP_DEV_T *dev,
                                    ZXIC_UINT32 zblk_idx,
                                    ZXIC_UINT32 *zreg0_bulk_id,
                                    ZXIC_UINT32 *zreg0_mono_flag,
                                    ZXIC_UINT32 *zreg1_bulk_id,
                                    ZXIC_UINT32 *zreg1_mono_flag,
                                    ZXIC_UINT32 *zreg2_bulk_id,
                                    ZXIC_UINT32 *zreg2_mono_flag,
                                    ZXIC_UINT32 *zreg3_bulk_id,
                                    ZXIC_UINT32 *zreg3_mono_flag)
{
    DPP_STATUS rtn = DPP_OK;

    DPP_SE4K_SE_ALG_ZLOCK_HASH_ZREG_MONO_T zblk_zreg_mono_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev),DPP_DEV_CHANNEL_MAX - 1);

    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), zreg0_bulk_id);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), zreg0_mono_flag);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), zreg1_bulk_id);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), zreg1_mono_flag);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), zreg2_bulk_id);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), zreg2_mono_flag);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), zreg3_bulk_id);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), zreg3_mono_flag);

    rtn = dpp_reg_read(dev,
                       SE4K_SE_ALG_ZLOCK_HASH_ZREG_MONOr,
                       ((ZBLK_ADDR_CONV(zblk_idx) >> 3) & 0x3),
                       (ZBLK_ADDR_CONV(zblk_idx) & 0x7),
                       &zblk_zreg_mono_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_reg_write");

    *zreg0_bulk_id = zblk_zreg_mono_cfg.ha_zreg0_tbl_id;
    *zreg0_mono_flag = zblk_zreg_mono_cfg.ha_zreg0_mono_flag;
    *zreg1_bulk_id = zblk_zreg_mono_cfg.ha_zreg1_tbl_id;
    *zreg1_mono_flag = zblk_zreg_mono_cfg.ha_zreg1_mono_flag;
    *zreg2_bulk_id = zblk_zreg_mono_cfg.ha_zreg2_tbl_id;
    *zreg2_mono_flag = zblk_zreg_mono_cfg.ha_zreg2_mono_flag;
    *zreg3_bulk_id = zblk_zreg_mono_cfg.ha_zreg3_tbl_id;
    *zreg3_mono_flag = zblk_zreg_mono_cfg.ha_zreg3_mono_flag;

    return DPP_OK;
}

/***********************************************************/
/** 设置hash访问片外DDR的属性
* @param   dev_id
* @param   hash0_mono_flag
* @param   hash1_mono_flag
* @param   hash2_mono_flag
* @param   hash3_mono_flag
*
* @return
* @remark  无
* @see
* @author  tf      @date  2016/05/7
************************************************************/
DPP_STATUS dpp_se_hash_zcam_mono_flags_set(DPP_DEV_T *dev,
                                           ZXIC_UINT32 hash0_mono_flag,
                                           ZXIC_UINT32 hash1_mono_flag,
                                           ZXIC_UINT32 hash2_mono_flag,
                                           ZXIC_UINT32 hash3_mono_flag)
{
    DPP_STATUS rtn = DPP_OK;

    DPP_SE4K_SE_ALG_HASH_MONO_FLAG_T hash_mono_flag = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev),DPP_DEV_CHANNEL_MAX - 1);

    hash_mono_flag.hash0_mono_flag = hash0_mono_flag;
    hash_mono_flag.hash1_mono_flag = hash1_mono_flag;
    hash_mono_flag.hash2_mono_flag = hash2_mono_flag;
    hash_mono_flag.hash3_mono_flag = hash3_mono_flag;

    rtn = dpp_reg_write(dev,
                        SE4K_SE_ALG_HASH_MONO_FLAGr,
                        0,
                        0,
                        &hash_mono_flag);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 设置hash访问片外DDR的属性
* @param   dev_id
* @param   hash0_mono_flag
* @param   hash1_mono_flag
* @param   hash2_mono_flag
* @param   hash3_mono_flag
*
* @return
* @remark  无
* @see
* @author  tf      @date  2016/05/7
************************************************************/
DPP_STATUS dpp_se_hash_zcam_mono_flags_get(DPP_DEV_T *dev,
                                           ZXIC_UINT32 *hash0_mono_flag,
                                           ZXIC_UINT32 *hash1_mono_flag,
                                           ZXIC_UINT32 *hash2_mono_flag,
                                           ZXIC_UINT32 *hash3_mono_flag)
{
    DPP_STATUS rtn = DPP_OK;

    DPP_SE4K_SE_ALG_HASH_MONO_FLAG_T hash_mono_flag = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev),DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), hash0_mono_flag);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), hash1_mono_flag);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), hash2_mono_flag);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), hash3_mono_flag);

    rtn = dpp_reg_read(dev,
                       SE4K_SE_ALG_HASH_MONO_FLAGr,
                       0,
                       0,
                       &hash_mono_flag);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_reg_write");

    *hash0_mono_flag = hash_mono_flag.hash0_mono_flag;
    *hash1_mono_flag = hash_mono_flag.hash1_mono_flag;
    *hash2_mono_flag = hash_mono_flag.hash2_mono_flag;
    *hash3_mono_flag = hash_mono_flag.hash3_mono_flag;

    return DPP_OK;
}

/***********************************************************/
/** 设置hash访问片外DDR的属性
* @param   dev_id
* @param   hash_id
* @param   ext_mode
* @param   flag
*
* @return
* @remark  无
* @see
* @author  wcl      @date  2014/07/26
************************************************************/
DPP_STATUS dpp_se_hash_ext_cfg_set(DPP_DEV_T *dev,
                                   ZXIC_UINT32 hash_id,
                                   ZXIC_UINT32 ext_mode,
                                   ZXIC_UINT32 flag)
{
    DPP_STATUS rtn = DPP_OK;

    DPP_SE4K_SE_ALG_HASH0_EXT_CFG_RGT_T hash_ext_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev),DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_UPPER(hash_id,DPP_HASH_ID_MAX);

    hash_ext_cfg.hash0_ext_flag = flag;
    hash_ext_cfg.hash0_ext_mode = ext_mode;

    rtn = dpp_reg_write(dev,
                        SE4K_SE_ALG_HASH0_EXT_CFG_RGTr + hash_id,
                        0,
                        0,
                        &hash_ext_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 获取hash访问片外DDR的属性
* @param   dev_id           设备号
* @param   hash_id          hash引擎，范围0~3
* @param   p_content_type   每个hash表项读一次ddr3还是两次；1-读512b的宽度，读两次 0-读256宽度，读一次
* @param   p_flag           片外ddr3是否存储hash表，1-片外存储hash表 0-片外不存储hash表
*
* @return
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_se_hash_ext_cfg_get(DPP_DEV_T *dev,
                                   ZXIC_UINT32 hash_id,
                                   ZXIC_UINT32 *p_content_type,
                                   ZXIC_UINT32 *p_flag)
{
    DPP_STATUS rtn = DPP_OK;

    DPP_SE4K_SE_ALG_HASH0_EXT_CFG_RGT_T hash_ext_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev),DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_UPPER(hash_id,DPP_HASH_ID_MAX);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_content_type);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_flag);

    rtn = dpp_reg_read(dev,
                       SE4K_SE_ALG_HASH0_EXT_CFG_RGTr + hash_id,
                       0,
                       0,
                       &hash_ext_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_reg_read");

    *p_content_type = hash_ext_cfg.hash0_ext_mode;
    *p_flag = hash_ext_cfg.hash0_ext_flag;

    return DPP_OK;
}

/***********************************************************/
/** 设置hash业务表深度
* @param   dev_id
* @param   hash_id
* @param   depth_bit
* @param   content_type
* @param   flag
*
* @return
* @remark  无
* @see
* @author  tf      @date  2016/01/28
************************************************************/
DPP_STATUS dpp_se_hash_tbl_depth_set(DPP_DEV_T *dev,
                                     ZXIC_UINT32 hash_id,
                                     ZXIC_UINT32 hash_tbl0_depth,
                                     ZXIC_UINT32 hash_tbl1_depth,
                                     ZXIC_UINT32 hash_tbl2_depth,
                                     ZXIC_UINT32 hash_tbl3_depth,
                                     ZXIC_UINT32 hash_tbl4_depth,
                                     ZXIC_UINT32 hash_tbl5_depth,
                                     ZXIC_UINT32 hash_tbl6_depth,
                                     ZXIC_UINT32 hash_tbl7_depth)

{
    DPP_STATUS rtn = DPP_OK;

    DPP_SE4K_SE_ALG_HASH0_TBL30_DEPTH_T hash_tbl30_depth = {0};
    DPP_SE4K_SE_ALG_HASH0_TBL74_DEPTH_T hash_tbl74_depth = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev),DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_UPPER(hash_id,DPP_HASH_ID_MAX);

    hash_tbl30_depth.hash0_tbl0_depth = hash_tbl0_depth;
    hash_tbl30_depth.hash0_tbl1_depth = hash_tbl1_depth;
    hash_tbl30_depth.hash0_tbl2_depth = hash_tbl2_depth;
    hash_tbl30_depth.hash0_tbl3_depth = hash_tbl3_depth;
    hash_tbl74_depth.hash0_tbl4_depth = hash_tbl4_depth;
    hash_tbl74_depth.hash0_tbl5_depth = hash_tbl5_depth;
    hash_tbl74_depth.hash0_tbl6_depth = hash_tbl6_depth;
    hash_tbl74_depth.hash0_tbl7_depth = hash_tbl7_depth;

    rtn = dpp_reg_write(dev,
                        SE4K_SE_ALG_HASH0_TBL30_DEPTHr + 2 * hash_id,
                        0,
                        0,
                        &hash_tbl30_depth);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_reg_write");

    rtn = dpp_reg_write(dev,
                        SE4K_SE_ALG_HASH0_TBL74_DEPTHr + 2 * hash_id,
                        0,
                        0,
                        &hash_tbl74_depth);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 获取hash业务表深度
* @param   dev_id
* @param   hash_id
* @param   p_content_type
* @param   p_flag
*
* @return
* @remark  无
* @see
* @author  tf      @date  2016/01/28
************************************************************/
DPP_STATUS dpp_se_hash_tbl_depth_get(DPP_DEV_T *dev,
                                     ZXIC_UINT32 hash_id,
                                     ZXIC_UINT32 *hash_tbl0_depth,
                                     ZXIC_UINT32 *hash_tbl1_depth,
                                     ZXIC_UINT32 *hash_tbl2_depth,
                                     ZXIC_UINT32 *hash_tbl3_depth,
                                     ZXIC_UINT32 *hash_tbl4_depth,
                                     ZXIC_UINT32 *hash_tbl5_depth,
                                     ZXIC_UINT32 *hash_tbl6_depth,
                                     ZXIC_UINT32 *hash_tbl7_depth)

{
    DPP_STATUS rtn = DPP_OK;

    DPP_SE4K_SE_ALG_HASH0_TBL30_DEPTH_T hash_tbl30_depth = {0};
    DPP_SE4K_SE_ALG_HASH0_TBL74_DEPTH_T hash_tbl74_depth = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev),DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_UPPER(hash_id,DPP_HASH_ID_MAX);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), hash_tbl0_depth);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), hash_tbl1_depth);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), hash_tbl2_depth);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), hash_tbl3_depth);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), hash_tbl4_depth);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), hash_tbl5_depth);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), hash_tbl6_depth);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), hash_tbl7_depth);

    rtn = dpp_reg_read(dev,
                       SE4K_SE_ALG_HASH0_TBL30_DEPTHr + 2 * hash_id,
                       0,
                       0,
                       &hash_tbl30_depth);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_reg_read");

    rtn = dpp_reg_read(dev,
                       SE4K_SE_ALG_HASH0_TBL74_DEPTHr + 2 * hash_id,
                       0,
                       0,
                       &hash_tbl74_depth);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_reg_read");

    *hash_tbl0_depth = hash_tbl30_depth.hash0_tbl0_depth;
    *hash_tbl1_depth = hash_tbl30_depth.hash0_tbl1_depth;
    *hash_tbl2_depth = hash_tbl30_depth.hash0_tbl2_depth;
    *hash_tbl3_depth = hash_tbl30_depth.hash0_tbl3_depth;
    *hash_tbl4_depth = hash_tbl74_depth.hash0_tbl4_depth;
    *hash_tbl5_depth = hash_tbl74_depth.hash0_tbl5_depth;
    *hash_tbl6_depth = hash_tbl74_depth.hash0_tbl6_depth;
    *hash_tbl7_depth = hash_tbl74_depth.hash0_tbl7_depth;

    return DPP_OK;
}

#endif
