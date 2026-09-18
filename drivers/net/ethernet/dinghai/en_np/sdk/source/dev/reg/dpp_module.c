/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_module.c
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : 石金锋
* 完成日期 : 2014/02/10
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
#include "dpp_module.h"
#include "dpp_dev.h"
#include "dpp_reg_struct.h"
#include "dpp_reg_api.h"

/***********************************************************/
/** dpp通用读寄存器函数
* @param   dev_id   设备id
* @param   addr        地址读地址
* @param   p_data   返回数据
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  石金锋      @date  2014/01/28
************************************************************/
DPP_STATUS dpp_read(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 *p_data)
{
    return dpp_dev_read_channel(dev,
                                addr,
                                1,
                                p_data);
}

/***********************************************************/
/** dpp通用写寄存器函数
* @param   dev_id   设备id
* @param   addr        地址写地址
* @param   p_data   写入数据
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  石金锋      @date  2014/01/28
************************************************************/
DPP_STATUS dpp_write(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 *p_data)
{
    return dpp_dev_write_channel(dev,
                                 addr,
                                 1,
                                 p_data);
}

#if ZXIC_REAL("SE")

/***********************************************************/
/** SE读接口
* @param   dev_id   设备号
* @param   addr        地址
* @param   p_data      数据
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/11/15
************************************************************/
DPP_STATUS dpp_se_read(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 *p_data)
{
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);

    return dpp_read(dev, addr, p_data);
}

/***********************************************************/
/** SE写接口
* @param   dev_id   设备号
* @param   addr        地址
* @param   p_data      数据
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/11/15
************************************************************/
DPP_STATUS dpp_se_write(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 *p_data)
{
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);

    return dpp_write(dev, addr, p_data);
}
#endif

#if ZXIC_REAL("SE ALG")
/***********************************************************/
/** se alg模块读寄存器函数
* @param   dev_id   设备id
* @param   addr        地址     读地址
* @param   p_data      返回数据
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_se_alg_read(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 *p_data)
{
    DPP_STATUS rtn = DPP_OK;

    ZXIC_UINT32 cpu_rd_rdy_addr = SYS_SE_BASE_ADDR + MODULE_SE_ALG_BASE_ADDR + 0x0048;
    ZXIC_UINT32 ind_data0_addr = SYS_SE_BASE_ADDR + MODULE_SE_ALG_BASE_ADDR + 0x004c;
    ZXIC_UINT32 ind_cmd_addr = SYS_SE_BASE_ADDR + MODULE_SE_ALG_BASE_ADDR + 0x0004;
    ZXIC_UINT32 cpu_rd_rdy_reg = 0;
    ZXIC_UINT32 ind_data0_reg = 0;
    ZXIC_UINT32 ind_cmd_reg = 0;
    ZXIC_UINT32 cmd_data = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 cpu_rdy  = 0;
    ZXIC_UINT32 read_cnt = 0;
    ZXIC_UINT32 recheck_flag = 20;

    ZXIC_MUTEX_T *p_alg_mutex = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);

    ind_data0_reg = dpp_reg_addr_convert(DEV_ID(dev), SE4K, DPP_REG_FLAG_DIRECT,ind_data0_addr);
    ind_cmd_reg = dpp_reg_addr_convert(DEV_ID(dev), SE4K, DPP_REG_FLAG_DIRECT,ind_cmd_addr);
    cpu_rd_rdy_reg = dpp_reg_addr_convert(DEV_ID(dev), SE4K, DPP_REG_FLAG_DIRECT,cpu_rd_rdy_addr);

    rtn = dpp_dev_opr_mutex_get(dev, DPP_DEV_MUTEX_T_REG, &p_alg_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_dev_opr_mutex_get");

    rtn = zxic_comm_mutex_lock(p_alg_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "zxic_comm_mutex_lock");

    /* dpp_module_get_se_alg_baseaddr(&base_addr); */

    cmd_data = ((ZXIC_UINT32)0x1 << 31) | ((ZXIC_UINT32)0xF << 17) | addr;
    rtn = dpp_dev_write_channel(dev, ind_cmd_reg, 1, &cmd_data);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rtn, "dpp_dev_write_channel", p_alg_mutex);

    while (!(cpu_rdy & 0x1))
    {
        rtn = dpp_dev_read_channel(dev, cpu_rd_rdy_reg, 1, &cpu_rdy);
        ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rtn, "dpp_dev_read_channel", p_alg_mutex);

        read_cnt++;
        /* zxic_comm_sleep(10); */

        if (read_cnt > DPP_RD_CNT_MAX * DPP_RD_CNT_MAX)
        {
            if (recheck_flag > 0)
            {
                recheck_flag--;
                read_cnt = 0;
                rtn = dpp_dev_write_channel(dev, ind_cmd_reg, 1, &cmd_data);
                ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rtn, "dpp_dev_write_channel", p_alg_mutex);
            }
            else
            {
                ZXIC_COMM_PRINT("Error!!! dpp_se_alg_read get cpu_rd_rdone failed!!!\n");
                zxic_comm_mutex_unlock(p_alg_mutex);
                /* ZXIC_COMM_ASSERT(0); */     /* xjw mod for OLT to not assert because of causing reboot at 18.8.7 */
                return DPP_ERR;
            }
        }

        /* ZXIC_COMM_CHECK_DEV_INDEX(dev_id, read_cnt, 0, DPP_RD_CNT_MAX); */
    }

    for (i = 0; i < 16; i++)
    {
        rtn = dpp_dev_read_channel(dev, ind_data0_reg + 4 * i, 1, p_data + 15 - i);
        ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rtn, "dpp_dev_read_channel", p_alg_mutex);
    }

    rtn = zxic_comm_mutex_unlock(p_alg_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "zxic_comm_mutex_unlock");

    return DPP_OK;
}

/***********************************************************/
/** se alg模块写寄存器函数
* @param   dev_id  设备id
* @param   addr        地址    读地址
* @param   p_data     返回数据
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_se_alg_write(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 *p_data)
{
    DPP_STATUS rtn = DPP_OK;

    ZXIC_UINT32 ind_data0_addr = SYS_SE_BASE_ADDR + MODULE_SE_ALG_BASE_ADDR + 0x0008;
    ZXIC_UINT32 cmd_data = 0;
    ZXIC_UINT32 ind_cmd_addr = SYS_SE_BASE_ADDR + MODULE_SE_ALG_BASE_ADDR + 0x0004;
    ZXIC_UINT32 ind_data0_reg = 0;
    ZXIC_UINT32 ind_cmd_reg = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_MUTEX_T *p_alg_mutex = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);

    ind_data0_reg = dpp_reg_addr_convert(DEV_ID(dev), SE4K, DPP_REG_FLAG_DIRECT, ind_data0_addr);
    ind_cmd_reg = dpp_reg_addr_convert(DEV_ID(dev), SE4K, DPP_REG_FLAG_DIRECT, ind_cmd_addr);

    /* dpp_module_get_se_alg_baseaddr(&base_addr); */
    rtn = dpp_dev_opr_mutex_get(dev, DPP_DEV_MUTEX_T_REG, &p_alg_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_dev_opr_mutex_get");

    rtn = zxic_comm_mutex_lock(p_alg_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "zxic_comm_mutex_lock");

    /* write data */
    for (i = 0; i < 16; i++)
    {
        ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), "dpp_se_alg_write: addr=0x%08x, data=0x%08x.\n", ind_data0_reg + 4 * i, *(p_data + 15 - i));
        rtn = dpp_dev_write_channel(dev, ind_data0_reg + 4 * i, 1, p_data + 15 - i);
        ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rtn, "dpp_dev_write_channel", p_alg_mutex);
    }

    /* write cmd */
    /* cmd_data = ((ZXIC_UINT32)0xF << 17) | addr; */
    cmd_data = addr & 0x1fffff;  /* mod by tf */
    ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), "dpp_se_alg_write: addr=0x%08x, data=0x%08x.\n", ind_cmd_reg, cmd_data);
    rtn = dpp_dev_write_channel(dev, ind_cmd_reg, 1, &cmd_data);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rtn, "dpp_dev_write_channel", p_alg_mutex);

    rtn = zxic_comm_mutex_unlock(p_alg_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "zxic_comm_mutex_unlock");

    return DPP_OK;
}

#endif

#if ZXIC_REAL("PPU")
/***********************************************************/
/** PPU读接口
* @param   dev_id   设备号
* @param   addr        地址
* @param   p_data      数据
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/11/15
************************************************************/
DPP_STATUS dpp_ppu_read(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 *p_data)
{
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);

    return dpp_read(dev, addr, p_data);
}

/***********************************************************/
/** PPU写接口
* @param   dev_id   设备号
* @param   addr        地址
* @param   p_data      数据
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/11/15
************************************************************/
DPP_STATUS dpp_ppu_write(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 *p_data)
{
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);

    return dpp_write(dev, addr, p_data);
}
#endif