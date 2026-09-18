/**************************************************************
* 版权所有 (C)2013-2015,深圳市中兴通讯股份有限公司
* 文件名称 : dpp_tm.c
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : djf
* 完成日期 : 2014/02/17
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/

/******************************************************************************
 *                START: 头文件                                *
 *****************************************************************************/
#include "zxic_common.h"
#include "dpp_type_api.h"
#include "dpp_reg_api.h"
#include "dpp_reg_info.h"
#include "dpp_etm_reg.h"
#include "dpp_module.h"
#include "dpp_tm_api.h"
//#include "dpp_tm_diag.h"
#include "dpp_tm.h"
#include "dpp_dev.h"

/******************************************************************************
 *                 END: 头文件                                 *
 *****************************************************************************/


/******************************************************************************
 *               START: 常量定义                               *
 *****************************************************************************/

DPP_TM_SHAPE_PARA_TABLE g_dpp_etm_shape_para_table[DPP_PCIE_SLOT_MAX][DPP_ETM_SHAP_TABEL_ID_MAX][DPP_TM_SHAP_MAP_ID_MAX] = {{{{0}}}}; /* coverity告警修改：单一变量不能超过10000字节 */

/* 全局变量读写互斥锁 */
ZXIC_MUTEX_T g_dpp_tm_global_var_rw_mutex;
ZXIC_UINT32 g_dpp_tm_global_var_rw_mutex_flag = 0;

/***********************************************************/
/** 全局变量读写互斥锁初始化
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/23
************************************************************/
DPP_STATUS dpp_tm_global_var_mutex_init(ZXIC_VOID)
{
    DPP_STATUS rc = DPP_OK;

    if (!g_dpp_tm_global_var_rw_mutex_flag)
    {
        rc = zxic_comm_mutex_create(&g_dpp_tm_global_var_rw_mutex);
        ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_create");

        g_dpp_tm_global_var_rw_mutex_flag = 1;
    }

    return DPP_OK;
}


/******************************************************************************
 *                END: 常量定义                                *
 *****************************************************************************/

#if ZXIC_REAL("TM_REG")
#if 0
/***********************************************************/
/** 写TM寄存器
* @param   module_id 区分TM子模块
* @param   addr   基于子模块的地址
* @param   data   写入的数据
*
* @return
* @remark  无
* @see
* @author  yjd      @date  2015/07/26
************************************************************/
DPP_STATUS dpp_tm_wr_reg(ZXIC_UINT32 dev_id, ZXIC_UINT32 module_id, ZXIC_UINT32 addr, ZXIC_UINT32 data)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 wr_data = 0;
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, module_id, MODULE_TM_CFGMT, MODULE_TM_MAX - 1);

    wr_data = data;
    rc = dpp_tm_write(dev_id, module_id,  addr,  &wr_data);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_write");

    return DPP_OK;
}

/***********************************************************/
/** 读TM寄存器
* @param   tm_type   0-ETM,1-FTM
* @param   module_id 区分TM子模块
* @param   addr   基于子模块的地址
* @return
* @remark  无
* @see
* @author  yjd      @date  2015/07/26
************************************************************/
DPP_STATUS dpp_tm_rd_reg(ZXIC_UINT32 dev_id, ZXIC_UINT32 module_id, ZXIC_UINT32 addr)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 rd_data = 0;

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, module_id, MODULE_TM_CFGMT, MODULE_TM_MAX - 1);

    rc = dpp_tm_read(dev_id, module_id,  addr,  &rd_data);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_read");

    ZXIC_COMM_PRINT("[0x%08x] 0x%08x \n", addr, rd_data);

    return DPP_OK;
}

/***********************************************************/
/** 写一片连续的TM寄存器
* @param   module_id 区分TM子模块
* @param   first_addr   起始寄存器的地址
* @param   reg_num   总共读取的寄存器数
*
* @return
* @remark  无
* @see
* @author  yjd      @date  2015/07/26
************************************************************/
DPP_STATUS dpp_tm_wr_more_reg(ZXIC_UINT32 dev_id, ZXIC_UINT32 module_id, ZXIC_UINT32 first_addr, ZXIC_UINT32 first_data, ZXIC_UINT32 data_step, ZXIC_UINT32 reg_num)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 addr = 0;
    ZXIC_UINT32 data = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, module_id, MODULE_TM_CFGMT, MODULE_TM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id,  first_addr , reg_num);
    ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id,  data_step , reg_num);
    ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id,  first_data , (data_step * reg_num));     

    for (i = 0; i < reg_num; i++)
    {
        addr = first_addr + i;
        data = first_data + (data_step * i);
        rc = dpp_tm_wr_reg(dev_id, module_id, addr, data);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_wr_reg");
    }

    return DPP_OK;
}

/***********************************************************/
/** 读一片连续的TM寄存器
* @param   module_id 区分TM子模块
* @param   first_addr   起始寄存器的地址
* @param   reg_num   总共读取的寄存器数
*
* @return
* @remark  无
* @see
* @author  yjd      @date  2017/07/26
************************************************************/
DPP_STATUS dpp_tm_rd_more_reg(ZXIC_UINT32 dev_id, ZXIC_UINT32 module_id, ZXIC_UINT32 first_addr, ZXIC_UINT32 reg_num)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 addr = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, module_id, MODULE_TM_CFGMT, MODULE_TM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id,  first_addr , reg_num);

    for (i = 0; i < reg_num; i++)
    {
        addr = first_addr + i;
        rc = dpp_tm_rd_reg(dev_id, module_id, addr);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_rd_reg");
    }

    return DPP_OK;
}

/***********************************************************/
/** 写tm模块二层间接寄存器(仅crdt/shap模块使用)
* @param   module_id 区分TM子模块
* @param   addr   基于子模块的地址
* @param   data   写入的数据
*
* @return
* @remark  无
* @see
* @author  whuashan      @date  2019/02/25
************************************************************/
DPP_STATUS dpp_tm_ind_wr_reg(ZXIC_UINT32 dev_id, ZXIC_UINT32 module_id, ZXIC_UINT32 addr, ZXIC_UINT64 data)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 wr_data[2] = {0};
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, module_id, MODULE_TM_SHAP, MODULE_TM_CRDT);

    wr_data[1] = (data & 0xffffffff);
    wr_data[0] = (data >> 32)&0xffffffff;

    rc = dpp_tm_ind_write(dev_id, module_id, addr, wr_data);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_ind_write");

    return DPP_OK;
}

/***********************************************************/
/** 读tm模块二层间接寄存器(仅crdt/shap模块使用)
* @param   module_id 区分TM子模块
* @param   addr   基于子模块的地址
*
* @return
* @remark  无
* @see
* @author  whuashan      @date  2019/02/25
************************************************************/
DPP_STATUS dpp_tm_ind_rd_reg(ZXIC_UINT32 dev_id, ZXIC_UINT32 module_id, ZXIC_UINT32 addr)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 rd_data[2] = {0};
    ZXIC_UINT64 tmp_data = 0;
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, module_id, MODULE_TM_SHAP, MODULE_TM_CRDT);

    rc = dpp_tm_ind_read(dev_id, module_id,  addr,  rd_data);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_ind_read");

    tmp_data = ((tmp_data | rd_data[0]) << 32) | (rd_data[1]);
    ZXIC_COMM_PRINT("[0x%08x] 0x%016llx \n", addr, tmp_data);

    return DPP_OK;
}
#endif

#endif


#if ZXIC_REAL("TM_CFGMT")

#if 0
/***********************************************************/
/** 校验子系统初始化就绪，所有子系统均初始化就绪，p_rdy值为1
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_rdy   初始化就绪标记，1-就绪，0-未就绪
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_tm_cfgmt_subsystem_rdy_check(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 p_rdy = 0;    /*tm子系统是否初始化完成判断*/
    ZXIC_UINT32 read_times = 50;
    DPP_ETM_CFGMT_SUBSYSTEM_RDY_REG_T subsystem_rdy = {0};



    /* 循环判定TM子系统是否初始化完成 */
    do
    {
        rc  = dpp_reg_read(dev_id,
                           ETM_CFGMT_SUBSYSTEM_RDY_REGr,
                           0,
                           0,
                           &subsystem_rdy);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

        p_rdy = (subsystem_rdy.olif_rdy)
                && (subsystem_rdy.qmu_rdy)
                && (subsystem_rdy.tmmu_rdy)
                && (subsystem_rdy.cgavd_rdy)
                && (subsystem_rdy.shap_rdy)
                && (subsystem_rdy.crdt_rdy);

        if (p_rdy)
        {
            rc  = dpp_tm_cfgmt_subsystem_rdy_print(dev_id);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_subsystem_rdy_print");
            break;
        }

        read_times--;
        zxic_comm_sleep(100);
    }
    while (read_times > 0);

    if (read_times == 0)
    {
        rc  = dpp_tm_cfgmt_subsystem_rdy_print(dev_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_subsystem_rdy_print");
        ZXIC_COMM_PRINT("dpp_tm_cfgmt_subsystem_rdy_check：TM INIT FAILED !!\n");
        return DPP_ERR;
    }

    return DPP_OK;
}
#endif

/***********************************************************/
/** cpu读写通道验证，其读出值等于写入值。读出值不等于写入值时，返回err
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_tm_cfgmt_cpu_check(DPP_DEV_T *dev)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 input = 0x5a5a5a5a;
    ZXIC_UINT32 output = 0;
    DPP_ETM_CFGMT_CPU_CHECK_REG_T cpu_access_input = {0};
    DPP_ETM_CFGMT_CPU_CHECK_REG_T cpu_access_output = {0};
    ZXIC_COMM_CHECK_POINT(dev);

    cpu_access_input.cpu_check_reg = input;
    rc  = dpp_reg_write(dev,
                        ETM_CFGMT_CPU_CHECK_REGr,
                        0,
                        0,
                        &cpu_access_input);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    rc  = dpp_reg_read(dev,
                       ETM_CFGMT_CPU_CHECK_REGr,
                       0,
                       0,
                       &cpu_access_output);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    output = cpu_access_output.cpu_check_reg;

    /* 判断读出值是否等于写入值 */
    if (input != output)
    {
        ZXIC_COMM_TRACE_ERROR("dpp_tm_cpu_check :input != output");
        return DPP_ERR;
    }

    return DPP_OK;
}

/***********************************************************/
/** 配置内置TM的工作模式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   mode   配置的值，0-TM模式，1-SA模式
*ETM仅工作在TM模式，FTM可以工作TM或SA模式
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/25
************************************************************/
DPP_STATUS dpp_tm_cfgmt_sa_work_mode_set(DPP_DEV_T *dev, DPP_TM_WORK_MODE_E mode)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CFGMT_TM_SA_WORK_MODE_T tm_sa_mode = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), mode, DPP_TM_WORK_MODE_TM, DPP_TM_WORK_MODE_TM);

    tm_sa_mode.tm_sa_work_mode = mode;
    rc  = dpp_reg_write(dev,
                        ETM_CFGMT_TM_SA_WORK_MODEr,
                        0,
                        0,
                        &tm_sa_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/** 读取内置TM的工作模式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_mode   读取的值，0-TM模式，1-SA模式
*ETM仅工作在TM模式，FTM可以工作TM或SA模式
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/25
************************************************************/
DPP_STATUS dpp_tm_cfgmt_sa_work_mode_get(DPP_DEV_T *dev, DPP_TM_WORK_MODE_E *p_mode)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CFGMT_TM_SA_WORK_MODE_T tm_sa_mode = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_mode);

    *p_mode = DPP_TM_WORK_MODE_INVALID;

    rc  = dpp_reg_read(dev,
                       ETM_CFGMT_TM_SA_WORK_MODEr,
                       0,
                       0,
                       &tm_sa_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *p_mode = tm_sa_mode.tm_sa_work_mode;

    return DPP_OK;
}

/***********************************************************/
/** 配置ddr3挂接组数，共10bit[0-9]，每bit对应1组ddr，TM最多使用其中8组
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   ddr_num   单个tm使用的ddr组：bit[0-9]每bit代表一组ddr，如使用4567组，则配置0xf0
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/06/07
************************************************************/
DPP_STATUS dpp_tm_cfgmt_ddr_attach_set(DPP_DEV_T *dev, ZXIC_UINT32 ddr_num)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CFGMT_CFGMT_DDR_ATTACH_T ddr_attach = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), ddr_num, 0, 0x3FF);

    ddr_attach.cfgmt_ddr_attach = ddr_num;
    rc  = dpp_reg_write(dev,
                        ETM_CFGMT_CFGMT_DDR_ATTACHr,
                        0,
                        0,
                        &ddr_attach);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 读取ddr3挂接组数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_ddr_num   ddr组数,1-6组
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_tm_cfgmt_ddr_attach_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_ddr_num)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CFGMT_CFGMT_DDR_ATTACH_T ddr_attach = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_ddr_num);

    rc  = dpp_reg_read(dev,
                       ETM_CFGMT_CFGMT_DDR_ATTACHr,
                       0,
                       0,
                       &ddr_attach);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    if (ddr_attach.cfgmt_ddr_attach == 1)
    {
        *p_ddr_num = 1;
    }
    else if (ddr_attach.cfgmt_ddr_attach == 3)
    {
        *p_ddr_num = 2;
    }
    else if (ddr_attach.cfgmt_ddr_attach == 7)
    {
        *p_ddr_num = 3;
    }
    else if (ddr_attach.cfgmt_ddr_attach == 15)
    {
        *p_ddr_num = 4;
    }
    else if (ddr_attach.cfgmt_ddr_attach == 31)
    {
        *p_ddr_num = 5;
    }
    else if (ddr_attach.cfgmt_ddr_attach == 63)
    {
        *p_ddr_num = 6;
    }
    else if (ddr_attach.cfgmt_ddr_attach == 127)
    {
        *p_ddr_num = 7;
    }
    else if (ddr_attach.cfgmt_ddr_attach == 255)
    {
        *p_ddr_num = 8;
    }

    return DPP_OK;
}

/***********************************************************/
/** 配置QMU工作模式，0：8 block工作模式，1：16 block工作模式
*** (即一个chunk中block个数) 影响tm总可用的缓存节点数。
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   mode   0-128/256K节点，1-256/512K节点，目前固定配1
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/25
************************************************************/
DPP_STATUS dpp_tm_cfgmt_qmu_work_mode_set(DPP_DEV_T *dev, DPP_TM_QMU_WORK_MODE_E mode)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CFGMT_QMU_WORK_MODE_T qmu_mode = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), mode, DPP_TM_QMU_WORK_MODE_2M, DPP_TM_QMU_WORK_MODE_4M);

    qmu_mode.qmu_work_mode = mode;
    rc  = dpp_reg_write(dev,
                        ETM_CFGMT_QMU_WORK_MODEr,
                        0,
                        0,
                        &qmu_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/** 读取QMU工作模式，0：8 block工作模式，1：16 block工作模式
*** (即一个chunk中block个数) 影响tm总可用的缓存节点数。
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   mode   0-128/256K节点，1-256/512K节点，目前固定配1
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/25
************************************************************/
DPP_STATUS dpp_tm_cfgmt_qmu_work_mode_get(DPP_DEV_T *dev, DPP_TM_QMU_WORK_MODE_E *p_mode)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CFGMT_QMU_WORK_MODE_T qmu_mode = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_mode);

    rc  = dpp_reg_read(dev,
                       ETM_CFGMT_QMU_WORK_MODEr,
                       0,
                       0,
                       &qmu_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *p_mode = qmu_mode.qmu_work_mode;

    return DPP_OK;
}



#if 0
/***********************************************************/
/** 配置包存储的CRC功能是否使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   en   配置的值，0-禁止CRC功能，1-允许CRC功能
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/25
************************************************************/
DPP_STATUS dpp_tm_cfgmt_crc_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CFGMT_CFGMT_CRC_EN_T crc_en = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, en, 0, 1);

    crc_en.cfgmt_crc_en = en;
    rc  = dpp_reg_write(dev_id,
                        ETM_CFGMT_CFGMT_CRC_ENr,
                        0,
                        0,
                        &crc_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 读取包存储的CRC功能是否使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_en   读取的值，0-禁止CRC功能，1-允许CRC功能
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/25
************************************************************/
DPP_STATUS dpp_tm_cfgmt_crc_en_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CFGMT_CFGMT_CRC_EN_T crc_en = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_en);

    *p_en = 0xffffffff;
    rc  = dpp_reg_read(dev_id,
                       ETM_CFGMT_CFGMT_CRC_ENr,
                       0,
                       0,
                       &crc_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_en = crc_en.cfgmt_crc_en;

    return DPP_OK;
}
#endif
/***********************************************************/
/** 配置block长度模式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   size   block长度模式:256/512/1024
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_tm_cfgmt_blk_size_set(DPP_DEV_T *dev, ZXIC_UINT32 size)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CFGMT_CFGMT_BLKSIZE_T blk_size = {0};
    ZXIC_COMM_CHECK_POINT(dev);

    switch (size)
    {
        case 256:
        {
            blk_size.cfgmt_blksize =  DPP_ETM_BLK_SIZE_256_B;
            break;
        }

        case 512:
        {
            blk_size.cfgmt_blksize =  DPP_ETM_BLK_SIZE_512_B;
            break;
        }

        case 1024:
        {
            blk_size.cfgmt_blksize =  DPP_ETM_BLK_SIZE_1024_B;
            break;
        }

        default:
        {
            ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "dpp_tm_cfgmt_blk_size_set:TM set block size error!\n");
            return DPP_ERR;
        }
    }

    rc  = dpp_reg_write(dev,
                        ETM_CFGMT_CFGMT_BLKSIZEr,
                        0,
                        0,
                        &blk_size);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 读取block长度模式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_size   block长度模式，256/512/1024
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_tm_cfgmt_blk_size_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_size)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CFGMT_CFGMT_BLKSIZE_T blk_size = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_size);

    rc  = dpp_reg_read(dev,
                       ETM_CFGMT_CFGMT_BLKSIZEr,
                       0,
                       0,
                       &blk_size);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    switch (blk_size.cfgmt_blksize)
    {
        case DPP_ETM_BLK_SIZE_256_B:
        {
            *p_size =  256;
            break;
        }

        case DPP_ETM_BLK_SIZE_512_B:
        {
            *p_size =  512;
            break;
        }

        case DPP_ETM_BLK_SIZE_1024_B:
        {
            *p_size =  1024;
            break;
        }

        default:
        {
            *p_size =  256;
        }
    }

    return DPP_OK;
}

#if 0
/***********************************************************/
/** 配置计数模式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_mode   计数模式
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_tm_cfgmt_cnt_mode_set(ZXIC_UINT32 dev_id, DPP_TM_CNT_MODE_T *p_mode)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CFGMT_CNT_MODE_REG_T cnt_mode = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_mode);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_mode->fc_count_mode, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_mode->count_rd_mode, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_mode->count_overflow_mode, 0, 1);

    cnt_mode.cfgmt_fc_count_mode = p_mode->fc_count_mode;
    cnt_mode.cfgmt_count_rd_mode = p_mode->count_rd_mode;
    cnt_mode.cfgmt_count_overflow_mode = p_mode->count_overflow_mode;
    rc  = dpp_reg_write(dev_id,
                        ETM_CFGMT_CNT_MODE_REGr,
                        0,
                        0,
                        &cnt_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 读取计数模式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_mode   计数模式
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_tm_cfgmt_cnt_mode_get(ZXIC_UINT32 dev_id, DPP_TM_CNT_MODE_T *p_mode)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CFGMT_CNT_MODE_REG_T cnt_mode = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_mode);

    rc  = dpp_reg_read(dev_id,
                       ETM_CFGMT_CNT_MODE_REGr,
                       0,
                       0,
                       &cnt_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    p_mode->fc_count_mode = cnt_mode.cfgmt_fc_count_mode;
    p_mode->count_rd_mode = cnt_mode.cfgmt_count_rd_mode;
    p_mode->count_overflow_mode = cnt_mode.cfgmt_count_overflow_mode;

    return DPP_OK;
}

/***********************************************************/
/** 配置中断屏蔽
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_para   中断屏蔽
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_tm_cfgmt_int_mask_set(ZXIC_UINT32 dev_id, DPP_TM_INT_T *p_para)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CFGMT_REG_INT_MASK_REG_T int_mask = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_para);

    int_mask.shap_int_mask      = p_para->shap_int;
    int_mask.crdt_int_mask      = p_para->crdt_int;
    int_mask.tmmu_int_mask       = p_para->mmu_int;
    int_mask.qmu_int_mask       = p_para->qmu_int;
    int_mask.cgavd_int_mask     = p_para->cgavd_int;
    int_mask.olif_int_mask      = p_para->olif_int;
    int_mask.cfgmt_int_buf_mask = p_para->cfgmt_int;

    rc  = dpp_reg_write(dev_id,
                        ETM_CFGMT_REG_INT_MASK_REGr,
                        0,
                        0,
                        &int_mask);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/** 读取中断屏蔽
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_para   中断屏蔽
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_tm_cfgmt_int_mask_get(ZXIC_UINT32 dev_id, DPP_TM_INT_T *p_para)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CFGMT_REG_INT_MASK_REG_T int_mask = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_para);

    rc  = dpp_reg_read(dev_id,
                       ETM_CFGMT_REG_INT_MASK_REGr,
                       0,
                       0,
                       &int_mask);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    p_para->shap_int  = int_mask.shap_int_mask;
    p_para->crdt_int  = int_mask.crdt_int_mask;
    p_para->mmu_int   = int_mask.tmmu_int_mask;
    p_para->qmu_int   = int_mask.qmu_int_mask;
    p_para->cgavd_int = int_mask.cgavd_int_mask;
    p_para->olif_int  = int_mask.olif_int_mask;
    p_para->cfgmt_int = int_mask.cfgmt_int_buf_mask;

    return DPP_OK;
}

/***********************************************************/
/** 读取中断状态
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_para   中断状态
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_tm_cfgmt_int_state_get(ZXIC_UINT32 dev_id, DPP_TM_INT_T *p_para)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CFGMT_REG_INT_STATE_REG_T int_state = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_para);

    rc  = dpp_reg_read(dev_id,
                       ETM_CFGMT_REG_INT_STATE_REGr,
                       0,
                       0,
                       &int_state);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    p_para->shap_int  = int_state.shap_int;
    p_para->crdt_int  = int_state.crdt_int;
    p_para->mmu_int   = int_state.mmu_int;
    p_para->qmu_int   = int_state.qmu_int;
    p_para->cgavd_int = int_state.cgavd_int;
    p_para->olif_int  = int_state.olif_int;
    p_para->cfgmt_int = int_state.cfgmt_int_buf;

    return DPP_OK;
}

/***********************************************************/
/** 配置tm时钟门控是否使能
* @param   dev_id   设备编号
* @param   en   配置的值，0-禁止tm时钟门控，1-使能tm时钟门控
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  lsy      @date  2022/08/22
************************************************************/
DPP_STATUS dpp_tm_cfgmt_clkgate_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CFGMT_CLKGATE_EN_T clkgate_en = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, en, 0, 1);

    clkgate_en.clkgate_en = en;
    rc  = dpp_reg_write(dev_id,
                        ETM_CFGMT_CLKGATE_ENr,
                        0,
                        0,
                        &clkgate_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 读取tm时钟门控是否使能
* @param   dev_id   设备编号
* @param   en   配置的值，0-禁止tm时钟门控，1-使能tm时钟门控
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  lsy      @date  2022/08/22

************************************************************/
DPP_STATUS dpp_tm_cfgmt_clkgate_en_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CFGMT_CLKGATE_EN_T clkgate_en = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_en);

    *p_en = 0xffffffff;
    rc  = dpp_reg_read(dev_id,
                       ETM_CFGMT_CLKGATE_ENr,
                       0,
                       0,
                       &clkgate_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_en = clkgate_en.clkgate_en;

    return DPP_OK;
}

/***********************************************************/
/** 配置tm软复位是否使能
* @param   dev_id   设备编号
* @param   en   配置的值，0-禁止tm软复位，1-使能tm软复位
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  lsy      @date  2022/08/22
************************************************************/
DPP_STATUS dpp_tm_cfgmt_softrst_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CFGMT_SOFTRST_EN_T softrst_en = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, en, 0, 1);

    softrst_en.softrst_en = en;
    rc  = dpp_reg_write(dev_id,
                        ETM_CFGMT_SOFTRST_ENr,
                        0,
                        0,
                        &softrst_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 读取tm软复位是否使能
* @param   dev_id   设备编号
* @param   en   配置的值，0-禁止tm软复位，1-使能tm软复位
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  lsy      @date  2022/08/22

************************************************************/
DPP_STATUS dpp_tm_cfgmt_softrst_en_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CFGMT_SOFTRST_EN_T softrst_en = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_en);

    *p_en = 0xffffffff;
    rc  = dpp_reg_read(dev_id,
                       ETM_CFGMT_SOFTRST_ENr,
                       0,
                       0,
                       &softrst_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_en = softrst_en.softrst_en;

    return DPP_OK;
}

#endif
#endif



#if ZXIC_REAL("TM_CGAVD")

#if 0
/***********************************************************/
/**  配置各级搬移功能使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   要配置的拥塞避免层次号，0:队列级，1:端口级，2:系统级
* @param   en   使能标记，0-不使能，1-使能
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2016/10/18
************************************************************/
#ifdef ETM_REAL

DPP_STATUS dpp_tm_cgavd_move_en_set(ZXIC_UINT32 dev_id,
                                    DPP_TM_CGAVD_LEVEL_E level,
                                    ZXIC_UINT32 en)
{
    DPP_STATUS  rc = DPP_OK;

    DPP_ETM_CGAVD_CGAVD_CFG_MOVE_T cgavd_cfg_move = {0};
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, level, QUEUE_LEVEL, SYS_LEVEL);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, en, 0, 1);

    rc  = dpp_reg_read(dev_id,
                       ETM_CGAVD_CGAVD_CFG_MOVEr,
                       0,
                       0,
                       &cgavd_cfg_move);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    if (QUEUE_LEVEL == level)
    {
        cgavd_cfg_move.cfgmt_flow_move_en = en;
    }

    else if (PP_LEVEL == level)
    {
        cgavd_cfg_move.cfgmt_port_move_en = en;
    }

    else if (SYS_LEVEL == level)
    {
        cgavd_cfg_move.cfgmt_sys_move_en = en;
    }

    rc  = dpp_reg_write(dev_id,
                        ETM_CGAVD_CGAVD_CFG_MOVEr,
                        0,
                        0,
                        &cgavd_cfg_move);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/**  读取各级搬移功能使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   要配置的拥塞避免层次号，0:队列级，1:端口级，2:系统级
* @param   p_en   读出的使能标记，0-不使能，1-使能
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2016/10/18
************************************************************/
DPP_STATUS dpp_tm_cgavd_move_en_get(ZXIC_UINT32 dev_id,
                                    DPP_TM_CGAVD_LEVEL_E level,
                                    ZXIC_UINT32 *p_en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_CGAVD_CFG_MOVE_T cgavd_cfg_move = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, level, QUEUE_LEVEL, SYS_LEVEL);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_en);

    *p_en = 0xffffffff;

    rc  = dpp_reg_read(dev_id,
                       ETM_CGAVD_CGAVD_CFG_MOVEr,
                       0,
                       0,
                       &cgavd_cfg_move);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    if (QUEUE_LEVEL == level)
    {
        *p_en = cgavd_cfg_move.cfgmt_flow_move_en;
    }

    else if (PP_LEVEL == level)
    {
        *p_en = cgavd_cfg_move.cfgmt_port_move_en;
    }

    else if (SYS_LEVEL == level)
    {
        *p_en = cgavd_cfg_move.cfgmt_sys_move_en;
    }

    return DPP_OK;
}


/***********************************************************/
/** 配置各级搬移门限
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   id   队列号或端口号，系统级时，id参数无效
* @param   value   端口级和系统级时，为搬移门限值，单位为NPPU存包的单位，256B；
                   流级时为搬移profile_id,0~15
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2016/10/18
************************************************************/
DPP_STATUS dpp_tm_cgavd_move_th_set(ZXIC_UINT32 dev_id,
                                    DPP_TM_CGAVD_LEVEL_E level,
                                    ZXIC_UINT32 id,
                                    ZXIC_UINT32 value)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 q_move_profile_reg_index = 0;
    ZXIC_UINT32 pp_move_th_reg_index = 0;
    ZXIC_UINT32 sys_move_th_reg_index = 0;

    DPP_ETM_CGAVD_MOVE_FLOW_TH_PROFILE_T q_move_profile = {0};
    DPP_ETM_CGAVD_MV_PORT_TH_T pp_move_th = {0};
    DPP_ETM_CGAVD_CFGMT_TOTAL_TH_T sys_move_th = {0};
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, level, QUEUE_LEVEL, SYS_LEVEL);
    

    q_move_profile_reg_index = ETM_CGAVD_MOVE_FLOW_TH_PROFILEr;
    pp_move_th_reg_index = ETM_CGAVD_MV_PORT_THr;
    sys_move_th_reg_index = ETM_CGAVD_CFGMT_TOTAL_THr;


    if (QUEUE_LEVEL == level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, value, 0, DPP_TM_CGAVD_MOVE_PROFILE_NUM - 1);
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, id, 0, DPP_ETM_Q_NUM - 1);

        q_move_profile.move_drop_profile = value;
        rc  = dpp_reg_write(dev_id,
                            q_move_profile_reg_index,
                            0,
                            id,
                            &q_move_profile);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    }

    else if (PP_LEVEL == level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, id, 0, DPP_TM_PP_NUM - 1);
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, value, 0, 0x3fff);

        pp_move_th.port_th = value;
        rc  = dpp_reg_write(dev_id,
                            pp_move_th_reg_index,
                            0,
                            id,
                            &pp_move_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, value, 0, 0x3fff);

        sys_move_th.cfgmt_total_th = value;
        rc  = dpp_reg_write(dev_id,
                            sys_move_th_reg_index,
                            0,
                            0,
                            &sys_move_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    return DPP_OK;

}


/***********************************************************/
/** 读取各级搬移门限
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   id   队列号或端口号，系统级时，id参数无效
* @param   p_value   端口级和系统级时，为搬移门限值，单位为NPPU存包的单位，256B；
                     流级时为搬移profile_id,0~15
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2016/10/18
************************************************************/
DPP_STATUS dpp_tm_cgavd_move_th_get(ZXIC_UINT32 dev_id,
                                    DPP_TM_CGAVD_LEVEL_E level,
                                    ZXIC_UINT32 id,
                                    ZXIC_UINT32 *p_value)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 q_move_profile_reg_index = 0;
    ZXIC_UINT32 pp_move_th_reg_index = 0;
    ZXIC_UINT32 sys_move_th_reg_index = 0;

    DPP_ETM_CGAVD_MOVE_FLOW_TH_PROFILE_T q_move_profile = {0};
    DPP_ETM_CGAVD_MV_PORT_TH_T pp_move_th = {0};
    DPP_ETM_CGAVD_CFGMT_TOTAL_TH_T sys_move_th = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, level, QUEUE_LEVEL, SYS_LEVEL);
    
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_value);

    q_move_profile_reg_index = ETM_CGAVD_MOVE_FLOW_TH_PROFILEr;
    pp_move_th_reg_index = ETM_CGAVD_MV_PORT_THr;
    sys_move_th_reg_index = ETM_CGAVD_CFGMT_TOTAL_THr;



    if (QUEUE_LEVEL == level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, id, 0, DPP_ETM_Q_NUM - 1);

        rc  = dpp_reg_read(dev_id,
                           q_move_profile_reg_index,
                           0,
                           id,
                           &q_move_profile);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
        *p_value = q_move_profile.move_drop_profile;
    }

    else if (PP_LEVEL == level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, id, 0, DPP_TM_PP_NUM - 1);
        rc  = dpp_reg_read(dev_id,
                           pp_move_th_reg_index,
                           0,
                           id,
                           &pp_move_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
        *p_value = pp_move_th.port_th;
    }

    else
    {
        rc  = dpp_reg_read(dev_id,
                           sys_move_th_reg_index,
                           0,
                           0,
                           &sys_move_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
        *p_value = sys_move_th.cfgmt_total_th;
    }

    return DPP_OK;

}


/***********************************************************/
/**  配置flow级的搬移策略
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   move_profile  flow级的搬移门限分组索引,0~15
* @param   th  flow级的搬移门限，单位为KB；
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2016/10/18
************************************************************/
DPP_STATUS dpp_tm_cgavd_flow_move_profile_set(ZXIC_UINT32 dev_id,
                                              ZXIC_UINT32 move_profile,
                                              ZXIC_UINT32 th)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 flow_move_th_reg_index = 0;
    ZXIC_UINT32 blk_size = 0;
    ZXIC_UINT32 move_th = 0;
    ZXIC_UINT32 cgavd_cfg_mode = 0;
    DPP_ETM_CGAVD_MOVE_FLOW_TH_T flow_th = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, move_profile, 0, DPP_TM_CGAVD_MOVE_PROFILE_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, th , DPP_TM_CGAVD_KILO_UL);
    move_th = th * DPP_TM_CGAVD_KILO_UL;

    flow_move_th_reg_index = ETM_CGAVD_MOVE_FLOW_THr;

    rc = dpp_tm_cgavd_cfg_mode_get(dev_id, &cgavd_cfg_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_cfg_mode_get");

    if (cgavd_cfg_mode == DPP_TM_CGAVD_BLOCK_MODE)
    {
        rc  = dpp_tm_cfgmt_blk_size_get(dev_id, &blk_size);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_blk_size_get");

        if (blk_size != 0)
        {
            move_th = (move_th / blk_size);
            move_th = (move_th % blk_size == 0) ? (move_th ) : ((move_th) + 1);
        }
    }

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, move_th, 0, 0x1fffffff);

    flow_th.move_drop_flow_th = move_th;
    rc  = dpp_reg_write(dev_id,
                        flow_move_th_reg_index,
                        0,
                        move_profile,
                        &flow_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/**  读取flow级的搬移策略
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   move_profile  flow级的搬移门限分组索引,0~15
* @param   p_th  flow级的搬移门限，单位为KB；
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2016/10/18
************************************************************/
DPP_STATUS dpp_tm_cgavd_flow_move_profile_get(ZXIC_UINT32 dev_id,
                                              ZXIC_UINT32 move_profile,
                                              ZXIC_UINT32 *p_th)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 flow_move_th_reg_index = 0;
    ZXIC_UINT32 blk_size = 0;
    ZXIC_UINT32 move_th = 0;
    ZXIC_UINT32 cgavd_cfg_mode = 0;

    DPP_ETM_CGAVD_MOVE_FLOW_TH_T flow_th = {0};

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, move_profile, 0, DPP_TM_CGAVD_MOVE_PROFILE_NUM - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_th);

    flow_move_th_reg_index = ETM_CGAVD_MOVE_FLOW_THr;

    rc  = dpp_reg_read(dev_id,
                       flow_move_th_reg_index,
                       0,
                       move_profile,
                       &flow_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    move_th = flow_th.move_drop_flow_th;

    rc = dpp_tm_cgavd_cfg_mode_get(dev_id, &cgavd_cfg_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_cfg_mode_get");

    if (cgavd_cfg_mode == DPP_TM_CGAVD_BLOCK_MODE)
    {
        rc  = dpp_tm_cfgmt_blk_size_get(dev_id, &blk_size);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_blk_size_get");

        *p_th = (move_th * blk_size) / DPP_TM_CGAVD_KILO_UL;
    }
    else if (cgavd_cfg_mode == DPP_TM_CGAVD_ZXIC_UINT8_MODE)
    {
        *p_th = (move_th / DPP_TM_CGAVD_KILO_UL);
    }
    else
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "dpp_tm_cgavd_flow_move_profile_get:cgavd_cfg_mode is err!!\n");
        return DPP_ERR;
    }

    return DPP_OK;
}
#endif
/***********************************************************/
/**  配置端口共享的搬移门限
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   th  端口共享的搬移门限，单位为NPPU存包的单位，256B；
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2016/10/18
************************************************************/
DPP_STATUS dpp_tm_cgavd_port_share_th_set(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 th)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 port_share_th_reg_index = 0;

    DPP_ETM_CGAVD_CFGMT_PORT_SHARE_TH_T port_share_th = {0};
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, th, 0, 0x3fff);

    port_share_th_reg_index = ETM_CGAVD_CFGMT_PORT_SHARE_THr;

    port_share_th.cfgmt_port_share_th = th;

    rc  = dpp_reg_write(dev_id,
                        port_share_th_reg_index,
                        0,
                        0,
                        &port_share_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/**  读取端口共享的搬移门限
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_th  端口共享的搬移门限，单位为NPPU存包的单位，256B；
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2016/10/18
************************************************************/
DPP_STATUS dpp_tm_cgavd_port_share_th_get(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 *p_th)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 port_share_th_reg_index = 0;

    DPP_ETM_CGAVD_CFGMT_PORT_SHARE_TH_T port_share_th = {0};

    
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_th);

    port_share_th_reg_index = ETM_CGAVD_CFGMT_PORT_SHARE_THr;

    rc  = dpp_reg_read(dev_id,
                       port_share_th_reg_index,
                       0,
                       0,
                       &port_share_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    *p_th = port_share_th.cfgmt_port_share_th;

    return DPP_OK;
}

/***********************************************************/
/**  配置基于优先级的QMU接收NPPU数据的fifo阈值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   sp  优先级0~7
* @param   th  指定优先级的fifo阈值0~511，单位为fifo条目，fifo深度512
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2016/10/18
************************************************************/
DPP_STATUS dpp_tm_cgavd_move_drop_sp_th_set(ZXIC_UINT32 dev_id,
                                            ZXIC_UINT32 sp,
                                            ZXIC_UINT32 th)
{
    DPP_STATUS  rc = DPP_OK;

    DPP_ETM_CGAVD_MV_DROP_SP_TH_T sp_th = {0};

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, th, 0, 0x1ff);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, sp, 0, 7);

    sp_th.mvdrop_sp_th = th;

    rc  = dpp_reg_write(dev_id,
                        ETM_CGAVD_MV_DROP_SP_THr,
                        0,
                        sp,
                        &sp_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/**  读取基于优先级的QMU接收NPPU数据的fifo阈值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   sp  优先级0~7
* @param   p_th  指定优先级的fifo阈值0~511，单位为fifo条目，fifo深度512
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2016/10/18
************************************************************/
DPP_STATUS dpp_tm_cgavd_move_drop_sp_th_get(ZXIC_UINT32 dev_id,
                                            ZXIC_UINT32 sp,
                                            ZXIC_UINT32 *p_th)
{
    DPP_STATUS  rc = DPP_OK;

    DPP_ETM_CGAVD_MV_DROP_SP_TH_T sp_th = {0};

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, sp, 0, 7);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_th);

    rc  = dpp_reg_read(dev_id,
                       ETM_CGAVD_MV_DROP_SP_THr,
                       0,
                       sp,
                       &sp_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    *p_th = sp_th.mvdrop_sp_th;

    return DPP_OK;
}


/***********************************************************/
/**  配置强制片内或片外
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   en      1:使能
* @param   mode   1 :omem 强制片外  0:imem 强制片内
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2017/10/14
************************************************************/
DPP_STATUS dpp_tm_cgavd_imem_omem_set(ZXIC_UINT32 dev_id,
                                      ZXIC_UINT32 en,
                                      ZXIC_UINT32 mode)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_CGAVD_FORCE_IMEM_OMEM_T cgavd_imem_omem_mode = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, mode, 0, 1);

    rc  = dpp_reg_read(dev_id,
                       ETM_CGAVD_CGAVD_FORCE_IMEM_OMEMr,
                       0,
                       0,
                       &cgavd_imem_omem_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    cgavd_imem_omem_mode.choose_imem_omem = mode;
    cgavd_imem_omem_mode.imem_omem_force_en = en;

    rc  = dpp_reg_write(dev_id,
                        ETM_CGAVD_CGAVD_FORCE_IMEM_OMEMr,
                        0,
                        0,
                        &cgavd_imem_omem_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/**  获取强制片内或片外
* @param   dev_id   设备编号
* @param   tm_type     0-ETM,1-FTM
* @param   en          1:使能
* @param   mode        1 :omem 强制片外  0:imem 强制片内
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2017/10/14
************************************************************/
DPP_STATUS dpp_tm_cgavd_imem_omem_get(ZXIC_UINT32 dev_id,
                                      ZXIC_UINT32 *p_en,
                                      ZXIC_UINT32 *p_mode)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_CGAVD_FORCE_IMEM_OMEM_T cgavd_imem_omem_mode = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_mode);

    rc  = dpp_reg_read(dev_id,
                       ETM_CGAVD_CGAVD_FORCE_IMEM_OMEMr,
                       0,
                       0,
                       &cgavd_imem_omem_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_mode = cgavd_imem_omem_mode.choose_imem_omem;
    *p_en = cgavd_imem_omem_mode.imem_omem_force_en;

    return DPP_OK;
}

#endif

/***********************************************************/
/**  配置配置cgavd模式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   mode   0:block mode 1:byte mode
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/07/28
************************************************************/
DPP_STATUS dpp_tm_cgavd_cfg_mode_set(DPP_DEV_T *dev,
                                     ZXIC_UINT32 mode)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_CFGMT_BYTE_MODE_T cgavd_cfg_mode = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), mode, 0, 1);

    rc  = dpp_reg_read(dev,
                       ETM_CGAVD_CFGMT_BYTE_MODEr,
                       0,
                       0,
                       &cgavd_cfg_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    cgavd_cfg_mode.cfgmt_byte_mode = mode;

    rc  = dpp_reg_write(dev,
                        ETM_CGAVD_CFGMT_BYTE_MODEr,
                        0,
                        0,
                        &cgavd_cfg_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/**  获取cgavd模式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_mode   0:block mode 1:byte mode
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/07/29
************************************************************/
DPP_STATUS dpp_tm_cgavd_cfg_mode_get(DPP_DEV_T *dev,
                                     ZXIC_UINT32 *p_mode)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_CFGMT_BYTE_MODE_T cgavd_cfg_mode = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_mode);

    rc  = dpp_reg_read(dev,
                       ETM_CGAVD_CFGMT_BYTE_MODEr,
                       0,
                       0,
                       &cgavd_cfg_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *p_mode = cgavd_cfg_mode.cfgmt_byte_mode;

    return DPP_OK;
}


/***********************************************************/
/**  配置各级拥塞避免功能使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   要配置的拥塞避免层次号，0:队列级，1:端口级，2:系统级
* @param   en   使能标记，0-不使能，1-使能
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/28
************************************************************/
DPP_STATUS dpp_tm_cgavd_en_set(DPP_DEV_T *dev,
                               DPP_TM_CGAVD_LEVEL_E level,
                               ZXIC_UINT32 en)
{
    DPP_STATUS  rc = DPP_OK;

    DPP_ETM_CGAVD_CGAVD_SUB_EN_T cgavd_sub_en = {0};
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_POINT(dev);
    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), level, QUEUE_LEVEL, SA_LEVEL);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), en, 0, 1);

    rc  = dpp_reg_read(dev,
                       ETM_CGAVD_CGAVD_SUB_ENr,
                       0,
                       0,
                       &cgavd_sub_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    if (QUEUE_LEVEL == level)
    {
        cgavd_sub_en.cgavd_flow_sub_en = en;
    }

    else if (PP_LEVEL == level)
    {
        cgavd_sub_en.cgavd_pp_sub_en = en;
    }

    else if (SYS_LEVEL == level)
    {
        cgavd_sub_en.cgavd_sys_sub_en = en;
    }

    rc  = dpp_reg_write(dev,
                        ETM_CGAVD_CGAVD_SUB_ENr,
                        0,
                        0,
                        &cgavd_sub_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/**  读取各级拥塞避免功能使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   要读取的拥塞避免层次号，0:队列级，1:端口级，2:系统级
* @param   p_en   读出的使能标记，0-不使能，1-使能
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/28
************************************************************/
DPP_STATUS dpp_tm_cgavd_en_get(DPP_DEV_T *dev,
                               DPP_TM_CGAVD_LEVEL_E level,
                               ZXIC_UINT32 *p_en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_CGAVD_SUB_EN_T cgavd_sub_en = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), level, QUEUE_LEVEL, SA_LEVEL);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_en);

    *p_en = 0xffffffff;

    rc  = dpp_reg_read(dev,
                       ETM_CGAVD_CGAVD_SUB_ENr,
                       0,
                       0,
                       &cgavd_sub_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    if (level == QUEUE_LEVEL)
    {
        *p_en = cgavd_sub_en.cgavd_flow_sub_en;
    }

    else if (level == PP_LEVEL)
    {
        *p_en = cgavd_sub_en.cgavd_pp_sub_en;
    }

    else if (level == SYS_LEVEL)
    {
        *p_en = cgavd_sub_en.cgavd_sys_sub_en;
    }

    else if (level == SA_LEVEL)
    {
        *p_en = cgavd_sub_en.cgavd_sa_sub_en;
    }

    return DPP_OK;
}

/***********************************************************/
/**  dp选取来源
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   要配置的拥塞避免层次号，0:队列级，1:端口级，2:系统级
* @param   dp_sel   dp选取来源，0-dp，1-tc，2-pkt_len[2:0]
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2017/03/14
************************************************************/
DPP_STATUS dpp_tm_cgavd_dp_sel_set(DPP_DEV_T *dev,
                                   DPP_TM_CGAVD_LEVEL_E level,
                                   DPP_TM_CGAVD_DP_SEL_E dp_sel)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_CGAVD_DP_SEL_T cgavd_dp_sel = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), level, QUEUE_LEVEL, SYS_LEVEL);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), dp_sel, DP_SEL_DP, DP_SEL_PKT_LEN);

    rc  = dpp_reg_read(dev,
                       ETM_CGAVD_CGAVD_DP_SELr,
                       0,
                       0,
                       &cgavd_dp_sel);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    if (QUEUE_LEVEL == level)
    {
        if (DP_SEL_DP == dp_sel)
        {
            cgavd_dp_sel.flow_dp_sel_high = 0;
            cgavd_dp_sel.flow_dp_sel_mid = 0;
            cgavd_dp_sel.flow_dp_sel_low = 1;
        }
        else if (DP_SEL_TC == dp_sel)
        {
            cgavd_dp_sel.flow_dp_sel_high = 0;
            cgavd_dp_sel.flow_dp_sel_mid = 1;
            cgavd_dp_sel.flow_dp_sel_low = 0;
        }
        else if (DP_SEL_PKT_LEN == dp_sel)
        {
            cgavd_dp_sel.flow_dp_sel_high = 1;
            cgavd_dp_sel.flow_dp_sel_mid = 0;
            cgavd_dp_sel.flow_dp_sel_low = 0;
        }
    }

    else if (PP_LEVEL == level)
    {
        if (DP_SEL_DP == dp_sel)
        {
            cgavd_dp_sel.pp_dp_sel_high = 0;
            cgavd_dp_sel.pp_dp_sel_mid = 0;
            cgavd_dp_sel.pp_dp_sel_low = 1;
        }
        else if (DP_SEL_TC == dp_sel)
        {
            cgavd_dp_sel.pp_dp_sel_high = 0;
            cgavd_dp_sel.pp_dp_sel_mid = 1;
            cgavd_dp_sel.pp_dp_sel_low = 0;
        }
        else if (DP_SEL_PKT_LEN == dp_sel)
        {
            cgavd_dp_sel.pp_dp_sel_high = 1;
            cgavd_dp_sel.pp_dp_sel_mid = 0;
            cgavd_dp_sel.pp_dp_sel_low = 0;
        }
    }

    else if (SYS_LEVEL == level)
    {
        if (DP_SEL_DP == dp_sel)
        {
            cgavd_dp_sel.sys_dp_sel_high = 0;
            cgavd_dp_sel.sys_dp_sel_mid = 0;
            cgavd_dp_sel.sys_dp_sel_low = 1;
        }
        else if (DP_SEL_TC == dp_sel)
        {
            cgavd_dp_sel.sys_dp_sel_high = 0;
            cgavd_dp_sel.sys_dp_sel_mid = 1;
            cgavd_dp_sel.sys_dp_sel_low = 0;
        }
        else if (DP_SEL_PKT_LEN == dp_sel)
        {
            cgavd_dp_sel.sys_dp_sel_high = 1;
            cgavd_dp_sel.sys_dp_sel_mid = 0;
            cgavd_dp_sel.sys_dp_sel_low = 0;
        }
    }

    rc  = dpp_reg_write(dev,
                        ETM_CGAVD_CGAVD_DP_SELr,
                        0,
                        0,
                        &cgavd_dp_sel);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/** 配置拥塞避免算法
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   id   队列号或端口号，系统级时，id参数无效
* @param   method  配置的拥塞避免算法，0:TD，1:WRED/GRED
*          配置TD算法时，先配TD阈值，再配置TD算法
*          配置WRED算法时,先配置流级或端口级的平均队列深度，再配置WRED算法
*          配置GRED算法时，先配置系统级的平均队列深度，在配置成GRED算法
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark
* @see
* @author  taq      @date  2015/04/14
************************************************************/
DPP_STATUS dpp_tm_cgavd_method_set(DPP_DEV_T *dev,
                                   DPP_TM_CGAVD_LEVEL_E level,
                                   ZXIC_UINT32 id,
                                   DPP_TM_CGAVD_METHOD_E method)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 q_avg_q_len_reg_index = 0;
    ZXIC_UINT32 q_td_th_reg_index = 0;
    ZXIC_UINT32 q_ca_mtd_reg_index = 0;
    ZXIC_UINT32 pp_avg_q_len_reg_index = 0;
    ZXIC_UINT32 pp_td_th_reg_index = 0;
    ZXIC_UINT32 pp_ca_mtd_reg_index = 0;
    ZXIC_UINT32 sys_avg_q_len_reg_index = 0;
    ZXIC_UINT32 sys_td_th_reg_index = 0;
    ZXIC_UINT32 sys_ca_mtd_reg_index = 0;

    DPP_ETM_CGAVD_FLOW_CA_MTD_T q_cgavd_method = {0};
    DPP_ETM_CGAVD_PP_CA_MTD_T pp_cgavd_method = {0};
    DPP_ETM_CGAVD_SYS_CGAVD_METD_T sys_cgavd_method = {0};
    DPP_ETM_CGAVD_FLOW_AVG_Q_LEN_T flow_avg_q_len = {0};
    DPP_ETM_CGAVD_PP_AVG_Q_LEN_T pp_avg_q_len = {0};
    DPP_ETM_CGAVD_SYS_AVG_Q_LEN_T sys_avg_q_len =  {0};
    DPP_ETM_CGAVD_FLOW_TD_TH_T q_td_th = {0};
    DPP_ETM_CGAVD_PP_TD_TH_T pp_td_th = {0};
    DPP_ETM_CGAVD_SYS_TD_TH_T sys_td_th = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), level, QUEUE_LEVEL, SYS_LEVEL);

    q_avg_q_len_reg_index = ETM_CGAVD_FLOW_AVG_Q_LENr;
    q_td_th_reg_index = ETM_CGAVD_FLOW_TD_THr;
    q_ca_mtd_reg_index = ETM_CGAVD_FLOW_CA_MTDr;
    pp_avg_q_len_reg_index = ETM_CGAVD_PP_AVG_Q_LENr;
    pp_td_th_reg_index = ETM_CGAVD_PP_TD_THr;
    pp_ca_mtd_reg_index = ETM_CGAVD_PP_CA_MTDr;
    sys_avg_q_len_reg_index = ETM_CGAVD_SYS_AVG_Q_LENr;
    sys_td_th_reg_index = ETM_CGAVD_SYS_TD_THr;
    sys_ca_mtd_reg_index = ETM_CGAVD_SYS_CGAVD_METDr;


    switch (level)
    {
        case (QUEUE_LEVEL):
        {
            ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), id, 0, DPP_ETM_Q_NUM - 1);

            ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), method, TD_METHOD, WRED_GRED_METHOD);

            if (1 == method) /* WRED算法，需要先配置平均队列深度，在选择wred算法 */
            {
                rc  = dpp_reg_read(dev,
                                   q_avg_q_len_reg_index,
                                   0,
                                   id,
                                   &flow_avg_q_len);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

                rc  = dpp_reg_write(dev,
                                    q_avg_q_len_reg_index,
                                    0,
                                    id,
                                    &flow_avg_q_len);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
            }

            if (0 == method) /* TD算法，需要先配置尾部丢弃阈值，在选择TD算法 */
            {
                rc  = dpp_reg_read(dev,
                                   q_td_th_reg_index,
                                   0,
                                   id,
                                   &q_td_th);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");


                rc  = dpp_reg_write(dev,
                                    q_td_th_reg_index,
                                    0,
                                    id,
                                    &q_td_th);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
            }

            /* 寄存器写入值,0-TD,1-WRED */
            q_cgavd_method.flow_ca_mtd = method;
            rc  = dpp_reg_write(dev,
                                q_ca_mtd_reg_index,
                                0,
                                id,
                                &q_cgavd_method);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

            break;
        }

        case (PP_LEVEL):
        {
            ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), id, 0, DPP_TM_PP_NUM - 1);
            ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), method, TD_METHOD, WRED_GRED_METHOD);

            if (1 == method) /* WRED算法，需要先配置平均队列深度，在选择wred算法 */
            {
                rc  = dpp_reg_read(dev,
                                   pp_avg_q_len_reg_index,
                                   0,
                                   id,
                                   &pp_avg_q_len);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

                rc  = dpp_reg_write(dev,
                                    pp_avg_q_len_reg_index,
                                    0,
                                    id,
                                    &pp_avg_q_len);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
            }

            if (0 == method) /* TD算法，尾部丢弃阈值，在选择TD算法 */
            {
                rc  = dpp_reg_read(dev,
                                   pp_td_th_reg_index,
                                   0,
                                   id,
                                   &pp_td_th);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
                rc  = dpp_reg_write(dev,
                                    pp_td_th_reg_index,
                                    0,
                                    id,
                                    &pp_td_th);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
            }

            /* 寄存器写入值,0-TD,1-WRED */
            pp_cgavd_method.pp_ca_mtd = method;
            rc  = dpp_reg_write(dev,
                                pp_ca_mtd_reg_index,
                                0,
                                id,
                                &pp_cgavd_method);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");


            break;
        }

        case (SYS_LEVEL):
        {
            ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), method, TD_METHOD, WRED_GRED_METHOD);

            if (1 == method) /* GRED算法，需要先配置平均队列深度，在选择gred算法 */
            {
                rc  = dpp_reg_read(dev,
                                   sys_avg_q_len_reg_index,
                                   0,
                                   0,
                                   &sys_avg_q_len);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

                rc  = dpp_reg_write(dev,
                                    sys_avg_q_len_reg_index,
                                    0,
                                    0,
                                    &sys_avg_q_len);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
            }

            if (0 == method) /* TD算法，尾部丢弃阈值，在选择TD算法 */
            {
                rc  = dpp_reg_read(dev,
                                   sys_td_th_reg_index,
                                   0,
                                   0,
                                   &sys_td_th);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

                rc  = dpp_reg_write(dev,
                                    sys_td_th_reg_index,
                                    0,
                                    0,
                                    &sys_td_th);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
            }

            /* 寄存器写入值,0-TD,1-GRED */
            sys_cgavd_method.sys_cgavd_metd = method;
            rc  = dpp_reg_write(dev,
                                sys_ca_mtd_reg_index,
                                0,
                                0,
                                &sys_cgavd_method);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

            break;
        }

        default:
        {
            ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "method=%u error!\n", (method));
            return DPP_ERR;
        }
    }

    return DPP_OK;

}


/***********************************************************/
/** 读取拥塞避免算法
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   id   队列号或端口号,系统级时,id参数无效
* @param   p_method   配置的拥塞避免算法,0:TD,1:WRED/GRED
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_method_get(DPP_DEV_T *dev,
                                   DPP_TM_CGAVD_LEVEL_E level,
                                   ZXIC_UINT32 id,
                                   DPP_TM_CGAVD_METHOD_E *p_method)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 q_ca_mtd_reg_index = 0;
    ZXIC_UINT32 pp_ca_mtd_reg_index = 0;
    ZXIC_UINT32 sys_ca_mtd_reg_index = 0;

    DPP_ETM_CGAVD_FLOW_CA_MTD_T q_cgavd_method = {0};
    DPP_ETM_CGAVD_PP_CA_MTD_T pp_cgavd_method = {0};
    DPP_ETM_CGAVD_SYS_CGAVD_METD_T sys_cgavd_method = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), level, QUEUE_LEVEL, SYS_LEVEL);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_method);
    
    *p_method = INVALID_METHOD;

    q_ca_mtd_reg_index = ETM_CGAVD_FLOW_CA_MTDr;
    pp_ca_mtd_reg_index = ETM_CGAVD_PP_CA_MTDr;
    sys_ca_mtd_reg_index = ETM_CGAVD_SYS_CGAVD_METDr;

    if (QUEUE_LEVEL == level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), id, 0, DPP_ETM_Q_NUM - 1);

        rc  = dpp_reg_read(dev,
                           q_ca_mtd_reg_index,
                           0,
                           id,
                           &q_cgavd_method);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
        *p_method = q_cgavd_method.flow_ca_mtd;
    }

    else if (PP_LEVEL == level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), id, 0, DPP_TM_PP_NUM - 1);

        rc  = dpp_reg_read(dev,
                           pp_ca_mtd_reg_index,
                           0,
                           id,
                           &pp_cgavd_method);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
        *p_method = pp_cgavd_method.pp_ca_mtd;
    }

    else
    {
        rc  = dpp_reg_read(dev,
                           sys_ca_mtd_reg_index,
                           0,
                           0,
                           &sys_cgavd_method);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

        *p_method = sys_cgavd_method.sys_cgavd_metd;
    }

    return DPP_OK;
}


/***********************************************************/
/** CPU设置的各级队列深度配置
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level      拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   q_len_use_cpu_set_en   0：选取RAM中读出的队列深度；
* @param                          1：选取q_len_cpu_set值
* @param   q_len_cpu_set    CPU设置的各级队列深度,单位为block。
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/06/19
************************************************************/
DPP_STATUS dpp_tm_q_len_use_cpu_set(DPP_DEV_T *dev,
                                    DPP_TM_CGAVD_LEVEL_E level,
                                    ZXIC_UINT32 q_len_use_cpu_set_en,
                                    ZXIC_UINT32 q_len_cpu_set)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 rd_cpu_or_ram_reg_index = 0;
    ZXIC_UINT32 q_cpu_set_q_len_reg_index = 0;
    ZXIC_UINT32 pp_cpu_set_q_len_reg_index = 0;
    ZXIC_UINT32 sys_cpu_set_q_len_reg_index = 0;

    DPP_ETM_CGAVD_RD_CPU_OR_RAM_T len_use_cpu_set_en = {0};
    DPP_ETM_CGAVD_FLOW_CPU_SET_Q_LEN_T flow_q_len_cpu_set = {0};
    DPP_ETM_CGAVD_PP_CPU_SET_Q_LEN_T pp_q_len_cpu_set = {0};
    DPP_ETM_CGAVD_SYS_CPU_SET_Q_LEN_T sys_q_len_cpu_set = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), level, QUEUE_LEVEL, SYS_LEVEL);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), q_len_use_cpu_set_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), q_len_cpu_set, 0, 0x1ffffff);

    rd_cpu_or_ram_reg_index = ETM_CGAVD_RD_CPU_OR_RAMr;
    q_cpu_set_q_len_reg_index = ETM_CGAVD_FLOW_CPU_SET_Q_LENr;
    pp_cpu_set_q_len_reg_index = ETM_CGAVD_PP_CPU_SET_Q_LENr;
    sys_cpu_set_q_len_reg_index = ETM_CGAVD_SYS_CPU_SET_Q_LENr;

    rc  = dpp_reg_read(dev,
                       rd_cpu_or_ram_reg_index,
                       0,
                       0,
                       &len_use_cpu_set_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    switch (level)
    {
        case (QUEUE_LEVEL):
        {
            len_use_cpu_set_en.cpu_sel_flow_q_len_en = q_len_use_cpu_set_en;
            rc  = dpp_reg_write(dev,
                                rd_cpu_or_ram_reg_index,
                                0,
                                0,
                                &len_use_cpu_set_en);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

            if (q_len_use_cpu_set_en == 1)
            {
                flow_q_len_cpu_set.flow_cpu_set_q_len = q_len_cpu_set;
                rc  = dpp_reg_write(dev,
                                    q_cpu_set_q_len_reg_index,
                                    0,
                                    0,
                                    &flow_q_len_cpu_set);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
            }


            break;
        }

        case (PP_LEVEL):
        {
            len_use_cpu_set_en.cpu_sel_pp_q_len_en = q_len_use_cpu_set_en;
            rc  = dpp_reg_write(dev,
                                rd_cpu_or_ram_reg_index,
                                0,
                                0,
                                &len_use_cpu_set_en);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

            if (q_len_use_cpu_set_en == 1)
            {
                pp_q_len_cpu_set.pp_cpu_set_q_len = q_len_cpu_set;
                rc  = dpp_reg_write(dev,
                                    pp_cpu_set_q_len_reg_index,
                                    0,
                                    0,
                                    &pp_q_len_cpu_set);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
            }

            break;
        }

        case (SYS_LEVEL):
        {
            len_use_cpu_set_en.cpu_sel_sys_q_len_en = q_len_use_cpu_set_en;
            rc  = dpp_reg_write(dev,
                                rd_cpu_or_ram_reg_index,
                                0,
                                0,
                                &len_use_cpu_set_en);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

            if (q_len_use_cpu_set_en == 1)
            {
                sys_q_len_cpu_set.sys_cpu_set_q_len = q_len_cpu_set;
                rc  = dpp_reg_write(dev,
                                    sys_cpu_set_q_len_reg_index,
                                    0,
                                    0,
                                    &sys_q_len_cpu_set);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
            }

            break;
        }

        default:
        {
            ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "level=%u error!\n", level);
            return DPP_ERR;
        }

    }

    return DPP_OK;

}


/***********************************************************/
/** CPU设置的各级平均队列深度配置
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level      拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   q_avg_len_use_cpu_set_en   0：选取RAM中读出的队列深度；
* @param                              1：选取q_avg_len_cpu_set值
* @param   q_avg_len_cpu_set    CPU设置的各级平均队列深度。
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/06/19
************************************************************/
DPP_STATUS dpp_tm_q_avg_len_use_cpu_set(DPP_DEV_T *dev,
                                        DPP_TM_CGAVD_LEVEL_E level,
                                        ZXIC_UINT32 q_avg_len_use_cpu_set_en,
                                        ZXIC_UINT32 q_avg_len_cpu_set)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 rd_cpu_or_ram_reg_index = 0;
    ZXIC_UINT32 q_cpu_set_avg_len_reg_index = 0;
    ZXIC_UINT32 pp_cpu_set_avg_len_reg_index = 0;
    ZXIC_UINT32 sys_cpu_set_avg_len_reg_index = 0;

    DPP_ETM_CGAVD_RD_CPU_OR_RAM_T avg_len_use_cpu_set_en = {0};
    DPP_ETM_CGAVD_FLOW_CPU_SET_AVG_LEN_T flow_q_avg_len_cpu_set = {0};
    DPP_ETM_CGAVD_PP_CPU_SET_AVG_Q_LEN_T pp_q_avg_len_cpu_set = {0};
    DPP_ETM_CGAVD_SYS_CPU_SET_AVG_LEN_T  sys_q_avg_len_cpu_set = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), level, QUEUE_LEVEL, SYS_LEVEL);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), q_avg_len_use_cpu_set_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), q_avg_len_cpu_set, 0, 0x1ffffff);

    rd_cpu_or_ram_reg_index = ETM_CGAVD_RD_CPU_OR_RAMr;
    q_cpu_set_avg_len_reg_index = ETM_CGAVD_FLOW_CPU_SET_AVG_LENr;
    pp_cpu_set_avg_len_reg_index = ETM_CGAVD_PP_CPU_SET_AVG_Q_LENr;
    sys_cpu_set_avg_len_reg_index = ETM_CGAVD_SYS_CPU_SET_AVG_LENr;

    rc  = dpp_reg_read(dev,
                       rd_cpu_or_ram_reg_index,
                       0,
                       0,
                       &avg_len_use_cpu_set_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    switch (level)
    {
        case (QUEUE_LEVEL):
        {
            avg_len_use_cpu_set_en.cpu_sel_flow_avg_q_len_en = q_avg_len_use_cpu_set_en;
            rc  = dpp_reg_write(dev,
                                rd_cpu_or_ram_reg_index,
                                0,
                                0,
                                &avg_len_use_cpu_set_en);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

            if (q_avg_len_use_cpu_set_en == 1)
            {
                flow_q_avg_len_cpu_set.flow_cpu_set_avg_len = q_avg_len_cpu_set;
                rc  = dpp_reg_write(dev,
                                    q_cpu_set_avg_len_reg_index,
                                    0,
                                    0,
                                    &flow_q_avg_len_cpu_set);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
            }

            break;
        }

        case (PP_LEVEL):
        {
            avg_len_use_cpu_set_en.cpu_sel_pp_avg_q_len_en = q_avg_len_use_cpu_set_en;
            rc  = dpp_reg_write(dev,
                                rd_cpu_or_ram_reg_index,
                                0,
                                0,
                                &avg_len_use_cpu_set_en);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

            if (q_avg_len_use_cpu_set_en == 1)
            {
                pp_q_avg_len_cpu_set.pp_cpu_set_avg_q_len = q_avg_len_cpu_set;
                rc  = dpp_reg_write(dev,
                                    pp_cpu_set_avg_len_reg_index,
                                    0,
                                    0,
                                    &pp_q_avg_len_cpu_set);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
            }

            break;
        }

        case (SYS_LEVEL):
        {
            avg_len_use_cpu_set_en.cpu_sel_sys_avg_q_len_en = q_avg_len_use_cpu_set_en;
            rc  = dpp_reg_write(dev,
                                rd_cpu_or_ram_reg_index,
                                0,
                                0,
                                &avg_len_use_cpu_set_en);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

            if (q_avg_len_use_cpu_set_en == 1)
            {
                sys_q_avg_len_cpu_set.sys_cpu_set_avg_len = q_avg_len_cpu_set;
                rc  = dpp_reg_write(dev,
                                    sys_cpu_set_avg_len_reg_index,
                                    0,
                                    0,
                                    &sys_q_avg_len_cpu_set);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
            }

            break;
        }

        default:
        {
            ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "level=%u error!\n", level);
            return DPP_ERR;
        }

    }

    return DPP_OK;

}


/***********************************************************/
/** 流队列级队列深度的获取
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   que_id      队列号
*          p_len       队列深度以KB为单位
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/06/19
************************************************************/
DPP_STATUS dpp_tm_flow_que_len_get(DPP_DEV_T *dev,
                                   ZXIC_UINT32 que_id,
                                   ZXIC_UINT32 *p_len)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 blk_size = 0;
    ZXIC_UINT32 cgavd_cfg_mode = 0;

    DPP_ETM_CGAVD_FLOW_Q_LEN_T dpp_tm_flow_len = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_len);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), que_id, 0, DPP_ETM_Q_NUM - 1);

    rc  = dpp_reg_read(dev,
                       ETM_CGAVD_FLOW_Q_LENr,
                       0,
                       que_id,
                       &dpp_tm_flow_len);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    rc = dpp_tm_cgavd_cfg_mode_get(dev, &cgavd_cfg_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_cgavd_cfg_mode_get");

    if (cgavd_cfg_mode == DPP_TM_CGAVD_BLOCK_MODE)
    {
        rc  = dpp_tm_cfgmt_blk_size_get(dev, &blk_size);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_cfgmt_blk_size_get");

        *p_len = ((dpp_tm_flow_len.flow_q_len * blk_size) / DPP_TM_CGAVD_KILO_UL);
    }
    else if (cgavd_cfg_mode == DPP_TM_CGAVD_ZXIC_UINT8_MODE)
    {
        *p_len = ((dpp_tm_flow_len.flow_q_len) / DPP_TM_CGAVD_KILO_UL);
    }
    else
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "dpp_tm_flow_que_len_get:cgavd_cfg_mode is err!!\n");
        return DPP_ERR;
    }

    return DPP_OK;
}

#if 0
/***********************************************************/
/** 端口级队列深度的获取
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   pp_id      队列号
*          pp_len       队列深度以KB为单位
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/06/19
************************************************************/
DPP_STATUS dpp_tm_port_que_len_get(ZXIC_UINT32 dev_id,
                                   ZXIC_UINT32 pp_id,
                                   ZXIC_UINT32 *pp_len)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 blk_size = 0;
    DPP_ETM_CGAVD_PP_Q_LEN_T dpp_tm_pp_len = {0};
    ZXIC_UINT32 cgavd_cfg_mode = 0;

    
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, pp_len);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, pp_id, 0, DPP_TM_PP_NUM - 1);

    rc  = dpp_reg_read(dev_id,
                       ETM_CGAVD_FLOW_Q_LENr,
                       0,
                       pp_id,
                       &dpp_tm_pp_len);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc = dpp_tm_cgavd_cfg_mode_get(dev_id, &cgavd_cfg_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_cfg_mode_get");

    if (cgavd_cfg_mode == DPP_TM_CGAVD_BLOCK_MODE)
    {
        rc  = dpp_tm_cfgmt_blk_size_get(dev_id, &blk_size);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_blk_size_get");

        *pp_len = ((dpp_tm_pp_len.pp_q_len * blk_size) / DPP_TM_CGAVD_KILO_UL);
    }
    else if (cgavd_cfg_mode == DPP_TM_CGAVD_ZXIC_UINT8_MODE)
    {
        *pp_len = ((dpp_tm_pp_len.pp_q_len) / DPP_TM_CGAVD_KILO_UL);
    }
    else
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "dpp_tm_port_que_len_get  err!!\n");
        return DPP_ERR;
    }

    return DPP_OK;
}


/***********************************************************/
/** 系统级队列深度的获取
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   sys_len       系统级深度以block为单位
*          sys_protocol_len       系统级包含协议队列深度以KB为单位
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/06/19
************************************************************/
DPP_STATUS dpp_tm_sys_que_len_get(ZXIC_UINT32 dev_id,
                                  ZXIC_UINT32 *sys_len,
                                  ZXIC_UINT32 *sys_protocol_len)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 blk_size = 0;
    ZXIC_UINT32 sys_q_len_reg_index = 0;
    ZXIC_UINT32 sys_q_len_l_reg_index = 0;
    DPP_ETM_CGAVD_SYS_Q_LEN_T dpp_tm_sys_len = {0};
    DPP_ETM_CGAVD_CGAVD_SYS_Q_LEN_L_T dpp_tm_sys_protocol_len = {0};
    ZXIC_UINT32 cgavd_cfg_mode = 0;

    
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, sys_protocol_len);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, sys_len);

    sys_q_len_reg_index = ETM_CGAVD_SYS_Q_LENr;
    sys_q_len_l_reg_index = ETM_CGAVD_CGAVD_SYS_Q_LEN_Lr;

    rc  = dpp_reg_read(dev_id,
                       sys_q_len_reg_index,
                       0,
                       0,
                       &dpp_tm_sys_len);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       sys_q_len_l_reg_index,
                       0,
                       0,
                       &dpp_tm_sys_protocol_len);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc = dpp_tm_cgavd_cfg_mode_get(dev_id, &cgavd_cfg_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_cfg_mode_get");

    if (cgavd_cfg_mode == DPP_TM_CGAVD_BLOCK_MODE)
    {
        rc  = dpp_tm_cfgmt_blk_size_get(dev_id, &blk_size);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_blk_size_get");

        *sys_len = ((dpp_tm_sys_len.sys_q_len * blk_size) / DPP_TM_CGAVD_KILO_UL);
        *sys_protocol_len = ((dpp_tm_sys_protocol_len.cgavd_sys_q_len_l * blk_size) / DPP_TM_CGAVD_KILO_UL);
    }
    else if (cgavd_cfg_mode == DPP_TM_CGAVD_ZXIC_UINT8_MODE)
    {
        *sys_len = ((dpp_tm_sys_len.sys_q_len) / DPP_TM_CGAVD_KILO_UL);
        *sys_protocol_len = ((dpp_tm_sys_protocol_len.cgavd_sys_q_len_l) / DPP_TM_CGAVD_KILO_UL);
    }
    else
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "dpp_tm_sys_que_len_get  err!!\n");
        return DPP_ERR;
    }

    return DPP_OK;

}

#endif

/***********************************************************/
/** 配置TD拥塞避免模式下的丢弃门限值
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   id   队列号或端口号，系统级时，id参数无效
* @param   byte_block_th   配置的丢弃门限值，ZXIC_UINT8/BLOCK单位写入寄存器
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/07/29
************************************************************/
DPP_STATUS dpp_tm_cgavd_td_byte_block_th_set(DPP_DEV_T *dev,
                                             DPP_TM_CGAVD_LEVEL_E level,
                                             ZXIC_UINT32 id,
                                             ZXIC_UINT32 byte_block_th)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 q_td_th_reg_index = 0;
    ZXIC_UINT32 q_ca_mtd_reg_index = 0;
    ZXIC_UINT32 pp_td_th_reg_index = 0;
    ZXIC_UINT32 pp_ca_mtd_reg_index = 0;
    ZXIC_UINT32 sys_td_th_reg_index = 0;
    ZXIC_UINT32 sys_ca_mtd_reg_index = 0;
    ZXIC_UINT32 read_times = 50;
    DPP_ETM_CGAVD_FLOW_CA_MTD_T q_cgavd_method = {0};
    DPP_ETM_CGAVD_FLOW_TD_TH_T q_td_th = {0};
    DPP_ETM_CGAVD_PP_CA_MTD_T pp_cgavd_method = {0};
    DPP_ETM_CGAVD_PP_TD_TH_T pp_td_th = {0};
    DPP_ETM_CGAVD_SYS_CGAVD_METD_T sys_cgavd_method = {0};
    DPP_ETM_CGAVD_SYS_TD_TH_T sys_td_th = {0};
    ZXIC_UINT32 qlist_clr_done_flag = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), level, QUEUE_LEVEL, SYS_LEVEL);

    q_td_th_reg_index = ETM_CGAVD_FLOW_TD_THr;
    q_ca_mtd_reg_index = ETM_CGAVD_FLOW_CA_MTDr;
    pp_td_th_reg_index = ETM_CGAVD_PP_TD_THr;
    pp_ca_mtd_reg_index = ETM_CGAVD_PP_CA_MTDr;
    sys_td_th_reg_index = ETM_CGAVD_SYS_TD_THr;
    sys_ca_mtd_reg_index = ETM_CGAVD_SYS_CGAVD_METDr;

    if (QUEUE_LEVEL == level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), id, 0, DPP_ETM_Q_NUM - 1);
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), byte_block_th, 0, 0x1fffffff);

        q_td_th.flow_td_th = byte_block_th;
        rc  = dpp_reg_write(dev,
                            q_td_th_reg_index,
                            0,
                            id,
                            &q_td_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

        q_cgavd_method.flow_ca_mtd = TD_METHOD;

        rc  = dpp_reg_write(dev,
                            q_ca_mtd_reg_index,
                            0,
                            id,
                            &q_cgavd_method);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

        /* add by zhmy begin@20151103 */
        if (byte_block_th == 0)
        {
            do
            {
                rc = dpp_tm_qmu_qlist_qcfg_clr_done_get(dev, &qlist_clr_done_flag);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_qmu_qlist_qcfg_clr_done_get");

                read_times--;

                if (1 == qlist_clr_done_flag)
                {
                    break;
                }

                zxic_comm_delay(10);
            }
            while (read_times > 0);

            if (read_times == 0)
            {
                ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "dpp_tm_qmu_qlist_qcfg_clr_done_get time out\n");
                return DPP_ERR;
            }

        }

        /* add by zhmy end@20151103 */
    }

    else if (PP_LEVEL == level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), id, 0, DPP_TM_PP_NUM - 1);
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), byte_block_th, 0, 0x1fffffff);

        pp_td_th.pp_td_th = byte_block_th;
        rc  = dpp_reg_write(dev,
                            pp_td_th_reg_index,
                            0,
                            id,
                            &pp_td_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

        pp_cgavd_method.pp_ca_mtd = TD_METHOD;
        rc  = dpp_reg_write(dev,
                            pp_ca_mtd_reg_index,
                            0,
                            id,
                            &pp_cgavd_method);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
    }

    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), byte_block_th, 0, 0x1fffffff);

        sys_td_th.sys_td_th = byte_block_th;
        rc  = dpp_reg_write(dev,
                            sys_td_th_reg_index,
                            0,
                            0,
                            &sys_td_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

        sys_cgavd_method.sys_cgavd_metd = TD_METHOD;

        rc  = dpp_reg_write(dev,
                            sys_ca_mtd_reg_index,
                            0,
                            0,
                            &sys_cgavd_method);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
    }

    return DPP_OK;

}


/***********************************************************/
/** 配置TD拥塞避免模式下的丢弃门限值
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   id   队列号或端口号，系统级时，id参数无效
* @param   td_th   配置的丢弃门限值，用户配置门限值单位为Kbyte，需要转化为Block或者ZXIC_UINT8单位写入寄存器
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/07/29
************************************************************/
DPP_STATUS dpp_tm_cgavd_td_th_set(DPP_DEV_T *dev,
                                  DPP_TM_CGAVD_LEVEL_E level,
                                  ZXIC_UINT32 id,
                                  ZXIC_UINT32 td_th)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 blk_size = 0;
    ZXIC_UINT32 blk_th = 0;
    ZXIC_UINT32 cgavd_cfg_mode = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), level, QUEUE_LEVEL, SYS_LEVEL);
    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), td_th, 0, 512*1024);

    td_th = (ZXIC_UINT32)(td_th *  DPP_TM_CGAVD_KILO_UL);

    rc = dpp_tm_cgavd_cfg_mode_get(dev, &cgavd_cfg_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_cgavd_cfg_mode_get");

    if (cgavd_cfg_mode == DPP_TM_CGAVD_BLOCK_MODE)
    {
        rc  = dpp_tm_cfgmt_blk_size_get(dev, &blk_size);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_cfgmt_blk_size_get");

        if (blk_size != 0)
        {
            blk_th = (td_th / blk_size);
            blk_th = (td_th % blk_size == 0) ? (blk_th ) : ((blk_th) + 1);
        }

        rc = dpp_tm_cgavd_td_byte_block_th_set(dev, level, id, blk_th);        
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_cgavd_td_byte_block_th_set");
    }
    else if (cgavd_cfg_mode == DPP_TM_CGAVD_ZXIC_UINT8_MODE)
    {
        rc = dpp_tm_cgavd_td_byte_block_th_set(dev, level, id, td_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_cgavd_td_byte_block_th_set");
    }
    else
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "dpp_tm_cgavd_td_th_set err!!\n");
        return DPP_ERR;
    }

    return DPP_OK;

}


/***********************************************************/
/** 读取TD拥塞避免模式下的丢弃门限值
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   id   队列号或端口号，系统级时，id参数无效
* @param   p_byte_block_th   配置的丢弃门限值ZXIC_UINT8/BLOCK单位
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/07/29
************************************************************/
DPP_STATUS dpp_tm_cgavd_td_byte_block_th_get(DPP_DEV_T *dev,
                                             DPP_TM_CGAVD_LEVEL_E level,
                                             ZXIC_UINT32 id,
                                             ZXIC_UINT32 *p_byte_block_th)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 q_td_th_reg_index = 0;
    ZXIC_UINT32 pp_td_th_reg_index = 0;
    ZXIC_UINT32 sys_td_th_reg_index = 0;
    DPP_ETM_CGAVD_FLOW_TD_TH_T q_td_th = {0};
    DPP_ETM_CGAVD_PP_TD_TH_T pp_td_th = {0};
    DPP_ETM_CGAVD_SYS_TD_TH_T sys_td_th = {0};
    
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), level, QUEUE_LEVEL, SYS_LEVEL);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_byte_block_th);
    

    if (QUEUE_LEVEL == level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), id, 0, DPP_ETM_Q_NUM - 1);
    }

    else if (PP_LEVEL == level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), id, 0, DPP_TM_PP_NUM - 1);
    }

    else
    {
        ZXIC_COMM_PRINT("sys:id is not to be checked!!\n");
    }

    q_td_th_reg_index = ETM_CGAVD_FLOW_TD_THr;
    pp_td_th_reg_index = ETM_CGAVD_PP_TD_THr;
    sys_td_th_reg_index = ETM_CGAVD_SYS_TD_THr;

    if (QUEUE_LEVEL == level)
    {
        rc  = dpp_reg_read(dev,
                           q_td_th_reg_index,
                           0,
                           id,
                           &q_td_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
        *p_byte_block_th = q_td_th.flow_td_th;
    }

    else if (PP_LEVEL == level)
    {
        rc  = dpp_reg_read(dev,
                           pp_td_th_reg_index,
                           0,
                           id,
                           &pp_td_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
        *p_byte_block_th = pp_td_th.pp_td_th;
    }

    else
    {
        rc  = dpp_reg_read(dev,
                           sys_td_th_reg_index,
                           0,
                           0,
                           &sys_td_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
        *p_byte_block_th = sys_td_th.sys_td_th;
    }

    return DPP_OK;

}


/***********************************************************/
/** 读取TD拥塞避免模式下的丢弃门限值
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   id   队列号或端口号，系统级时，id参数无效
* @param   p_td_th   配置的丢弃门限值KZXIC_UINT8单位
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/07/29
************************************************************/
DPP_STATUS dpp_tm_cgavd_td_th_get(DPP_DEV_T *dev,
                                  DPP_TM_CGAVD_LEVEL_E level,
                                  ZXIC_UINT32 id,
                                  ZXIC_UINT32 *p_td_th)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 cgavd_cfg_mode = 0;
    ZXIC_UINT32 block_byte_th = 0;
    ZXIC_UINT32 blk_size = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), level, QUEUE_LEVEL, SYS_LEVEL);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_td_th);

    *p_td_th = 0;

    if (QUEUE_LEVEL == level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), id, 0, DPP_ETM_Q_NUM - 1);
    }

    else if (PP_LEVEL == level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), id, 0, DPP_TM_PP_NUM - 1);
    }

    else
    {
        ZXIC_COMM_PRINT("sys:id is not to be checked!!\n");
    }

    rc = dpp_tm_cgavd_td_byte_block_th_get(dev, level, id, &block_byte_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_cgavd_td_byte_block_th_get");

    rc = dpp_tm_cgavd_cfg_mode_get(dev, &cgavd_cfg_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_cgavd_cfg_mode_get");

    if (cgavd_cfg_mode == DPP_TM_CGAVD_BLOCK_MODE)
    {
        rc  = dpp_tm_cfgmt_blk_size_get(dev, &blk_size);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_cfgmt_blk_size_get");

        *p_td_th = (block_byte_th * blk_size) / DPP_TM_CGAVD_KILO_UL;
    }
    else if (cgavd_cfg_mode == DPP_TM_CGAVD_ZXIC_UINT8_MODE)
    {
        *p_td_th = (block_byte_th / DPP_TM_CGAVD_KILO_UL);
    }
    else
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "dpp_tm_cgavd_td_th_get err!!\n");
        return DPP_ERR;
    }

    return DPP_OK;

}

#if 0
/***********************************************************/
/** 配置指定端口或队列绑定的WRED GROUP ID
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   WRED支持层次号，0:队列级，1:端口级
* @param   id   队列号或端口号
* @param   wred_id   配置的WRED GROUP ID
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_wred_id_set(ZXIC_UINT32 dev_id,
                                    DPP_TM_CGAVD_LEVEL_E level,
                                    ZXIC_UINT32 id,
                                    ZXIC_UINT32 wred_id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 q_wred_grp_reg_index = 0;
    ZXIC_UINT32 pp_wrd_grp_th_en_reg_index = 0;

    DPP_ETM_CGAVD_FLOW_WRED_GRP_T q_wred_group = {0};
    DPP_ETM_CGAVD_PP_WRED_GRP_TH_EN_T pp_wred_group = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, level, QUEUE_LEVEL, PP_LEVEL);

    q_wred_grp_reg_index = ETM_CGAVD_FLOW_WRED_GRPr;
    pp_wrd_grp_th_en_reg_index = ETM_CGAVD_PP_WRED_GRP_TH_ENr;

    if (QUEUE_LEVEL == level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, id, 0, DPP_ETM_Q_NUM - 1);
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, wred_id, 0, DPP_TM_Q_WRED_NUM - 1);

        q_wred_group.flow_wred_grp = wred_id;
        rc  = dpp_reg_write(dev_id,
                            q_wred_grp_reg_index,
                            0,
                            id,
                            &q_wred_group);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, id, 0, DPP_TM_PP_NUM - 1);
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, wred_id, 0, DPP_TM_PP_WRED_NUM - 1);

        rc  = dpp_reg_read(dev_id,
                           pp_wrd_grp_th_en_reg_index,
                           0,
                           id,
                           &pp_wred_group);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

        pp_wred_group.pp_wred_grp = wred_id;
        rc  = dpp_reg_write(dev_id,
                            pp_wrd_grp_th_en_reg_index,
                            0,
                            id,
                            &pp_wred_group);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    return DPP_OK;

}


/***********************************************************/
/** 读取指定端口或队列绑定的WRED GROUP ID
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   WRED支持层次号，0:队列级，1:端口级
* @param   id   队列号或端口号
* @param   p_wred_id   配置的WRED GROUP ID
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_wred_id_get(ZXIC_UINT32 dev_id,
                                    DPP_TM_CGAVD_LEVEL_E level,
                                    ZXIC_UINT32 id,
                                    ZXIC_UINT32 *p_wred_id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 q_wred_grp_reg_index = 0;
    ZXIC_UINT32 pp_wrd_grp_th_en_reg_index = 0;

    DPP_ETM_CGAVD_FLOW_WRED_GRP_T q_wred_group = {0};
    DPP_ETM_CGAVD_PP_WRED_GRP_TH_EN_T pp_wred_group = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, level, QUEUE_LEVEL, PP_LEVEL);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_wred_id);

    q_wred_grp_reg_index = ETM_CGAVD_FLOW_WRED_GRPr;
    pp_wrd_grp_th_en_reg_index = ETM_CGAVD_PP_WRED_GRP_TH_ENr;

    if (QUEUE_LEVEL == level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, id, 0, DPP_ETM_Q_NUM - 1);
        rc  = dpp_reg_read(dev_id,
                           q_wred_grp_reg_index,
                           0,
                           id,
                           &q_wred_group);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
        *p_wred_id = q_wred_group.flow_wred_grp;
    }

    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, id, 0, DPP_TM_PP_NUM - 1);
        rc  = dpp_reg_read(dev_id,
                           pp_wrd_grp_th_en_reg_index,
                           0,
                           id,
                           &pp_wred_group);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

        *p_wred_id = pp_wred_group.pp_wred_grp;
    }

    return DPP_OK;

}


/***********************************************************/
/** 配置WRED丢弃曲线对应的参数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   WRED支持层次号，0:队列级，1:端口级
* @param   wred_id   队列级共支持16个WRED组0-15，端口级支持8组0-7
* @param   dp   共支持8个dp，取值0-7
* @param   p_para   配置的WRED组参数值，包含以下五个参数
           max_th  平均队列深度上限阈值ZXIC_UINT8/BLOCK单位
           min_th  平均队列深度下限阈值ZXIC_UINT8/BLOCK单位
           max_p  最大丢弃概率
           weight   平均队列深度计算权重
           q_len_th 队列深度阈值ZXIC_UINT8/BLOCK单位
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy     @date  2016/08/01
************************************************************/
DPP_STATUS dpp_tm_cgavd_wred_dp_line_block_byte_para_set(ZXIC_UINT32 dev_id,
                                                         DPP_TM_CGAVD_LEVEL_E level,
                                                         ZXIC_UINT32 wred_id,
                                                         ZXIC_UINT32 dp,
                                                         DPP_TM_WRED_DP_LINE_PARA_T *p_para)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 q_wred_max_th_reg_index = 0;
    ZXIC_UINT32 q_wred_min_th_reg_index = 0;
    ZXIC_UINT32 q_wred_cfg_para_reg_index = 0;
    ZXIC_UINT32 q_wq_reg_index = 0;
    ZXIC_UINT32 q_wred_len_th_reg_index = 0;

    ZXIC_UINT32 pp_wred_max_th_reg_index = 0;
    ZXIC_UINT32 pp_wred_min_th_reg_index = 0;
    ZXIC_UINT32 pp_wred_cfg_para_reg_index = 0;
    ZXIC_UINT32 pp_wq_reg_index = 0;
    ZXIC_UINT32 pp_wred_len_th_reg_index = 0;

    DPP_ETM_CGAVD_FLOW_WRED_MAX_TH_T q_max_th = {0};
    DPP_ETM_CGAVD_FLOW_WRED_MIN_TH_T q_min_th = {0};
    DPP_ETM_CGAVD_FLOW_WRED_CFG_PARA_T q_cfg_para = {0};
    DPP_ETM_CGAVD_FLOW_WQ_T q_wq = {0};
    DPP_ETM_CGAVD_FLOW_WRED_Q_LEN_TH_T q_len_th = {0};
    DPP_ETM_CGAVD_PP_WRED_MAX_TH_T pp_max_th = {0};
    DPP_ETM_CGAVD_PP_WRED_MIN_TH_T pp_min_th = {0};
    DPP_ETM_CGAVD_PP_CFG_PARA_T pp_cfg_para = {0};
    DPP_ETM_CGAVD_PP_WQ_T pp_wq = {0};
    DPP_ETM_CGAVD_PP_WRED_Q_LEN_TH_T pp_len_th = {0};

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, level, QUEUE_LEVEL, PP_LEVEL);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dp, 0, DPP_TM_DP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_para);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_para->max_p, 1, DPP_TM_RED_P_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_para->weight, 0, DPP_TM_CGAVD_WEIGHT_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_para->max_th, 0, 0x1fffffff);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_para->min_th, 0, 0x1fffffff);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_para->q_len_th, 0, 0x1fffffff);

    q_wred_max_th_reg_index = ETM_CGAVD_FLOW_WRED_MAX_THr;
    q_wred_min_th_reg_index = ETM_CGAVD_FLOW_WRED_MIN_THr;
    q_wred_cfg_para_reg_index = ETM_CGAVD_FLOW_WRED_CFG_PARAr;
    q_wq_reg_index = ETM_CGAVD_FLOW_WQr;
    q_wred_len_th_reg_index = ETM_CGAVD_FLOW_WRED_Q_LEN_THr;

    pp_wred_max_th_reg_index = ETM_CGAVD_PP_WRED_MAX_THr;
    pp_wred_min_th_reg_index = ETM_CGAVD_PP_WRED_MIN_THr;
    pp_wred_cfg_para_reg_index = ETM_CGAVD_PP_CFG_PARAr;
    pp_wq_reg_index = ETM_CGAVD_PP_WQr;
    pp_wred_len_th_reg_index = ETM_CGAVD_PP_WRED_Q_LEN_THr;

    ZXIC_COMM_CHECK_INDEX_MUL_OVERFLOW(wred_id, DPP_TM_DP_NUM);
    index = wred_id * DPP_TM_DP_NUM + dp;

    if (QUEUE_LEVEL == level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, wred_id, 0, DPP_TM_Q_WRED_NUM - 1);

        /* q_max_th */
        q_max_th.flow_wred_max_th = p_para->max_th;
        rc  = dpp_reg_write(dev_id,
                            q_wred_max_th_reg_index,
                            0,
                            index,
                            &q_max_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        /* q_min_th */
        q_min_th.flow_wred_min_th = p_para->min_th;
        rc  = dpp_reg_write(dev_id,
                            q_wred_min_th_reg_index,
                            0,
                            index,
                            &q_min_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        /* q_cfg_para，乘以100用来换算百分比，用户配置的是值是1-100 */
        if (p_para->max_p != 0)
        {
            ZXIC_COMM_CHECK_DEV_INDEX_SUB_OVERFLOW_NO_ASSERT(dev_id,   q_max_th.flow_wred_max_th+1 , q_min_th.flow_wred_min_th);   
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, 100 , (q_max_th.flow_wred_max_th - q_min_th.flow_wred_min_th));
            q_cfg_para.flow_wred_cfg_para = (ZXIC_UINT32)(100 * (q_max_th.flow_wred_max_th - q_min_th.flow_wred_min_th) / p_para->max_p);
        }
        else
        {
            q_cfg_para.flow_wred_cfg_para = DPP_TM_Q_WRED_MAX_CFG_PARA;
        }

        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, q_cfg_para.flow_wred_cfg_para, 0, 0xffffffff);

        rc  = dpp_reg_write(dev_id,
                            q_wred_cfg_para_reg_index,
                            0,
                            index,
                            &q_cfg_para);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        /* 必须先写q_wq */
        q_wq.wq_flow = p_para->weight;
        rc  = dpp_reg_write(dev_id,
                            q_wq_reg_index,
                            0,
                            wred_id,
                            &q_wq);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        /* 再写q_len_th */
        q_len_th.flow_wred_q_len_th = p_para->q_len_th;
        rc  = dpp_reg_write(dev_id,
                            q_wred_len_th_reg_index,
                            0,
                            wred_id,
                            &q_len_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, wred_id, 0, DPP_TM_PP_WRED_NUM - 1);

        /* pp_max_th */
        pp_max_th.pp_wred_max_th = p_para->max_th;
        rc  = dpp_reg_write(dev_id,
                            pp_wred_max_th_reg_index,
                            0,
                            index,
                            &pp_max_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        /* pp_min_th */
        pp_min_th.pp_wred_min_th = p_para->min_th;
        rc  = dpp_reg_write(dev_id,
                            pp_wred_min_th_reg_index,
                            0,
                            index,
                            &pp_min_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        /* pp_cfg_para，乘以100用来换算百分比，用户配置的是值是1-100 */
        if (p_para->max_p != 0)
        {
            ZXIC_COMM_CHECK_DEV_INDEX_SUB_OVERFLOW_NO_ASSERT(dev_id, pp_max_th.pp_wred_max_th+1 , pp_min_th.pp_wred_min_th);
            pp_cfg_para.pp_cfg_para = (ZXIC_UINT32)(100 * (pp_max_th.pp_wred_max_th - pp_min_th.pp_wred_min_th) / p_para->max_p);
        }
        else
        {
            pp_cfg_para.pp_cfg_para = DPP_TM_PP_WRED_MAX_CFG_PARA;
        }

        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, pp_cfg_para.pp_cfg_para, 0, 0xffffffff);

        rc  = dpp_reg_write(dev_id,
                            pp_wred_cfg_para_reg_index,
                            0,
                            index,
                            &pp_cfg_para);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        /* 必须先写pp_len_th */
        pp_len_th.pp_wred_q_len_th = p_para->q_len_th;
        rc  = dpp_reg_write(dev_id,
                            pp_wred_len_th_reg_index,
                            0,
                            wred_id,
                            &pp_len_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        /* 再写pp_wq */
        pp_wq.wq_pp = p_para->weight;
        rc  = dpp_reg_write(dev_id,
                            pp_wq_reg_index,
                            0,
                            wred_id,
                            &pp_wq);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    return DPP_OK;

}


/***********************************************************/
/** 配置WRED丢弃曲线对应的参数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   WRED支持层次号，0:队列级，1:端口级
* @param   wred_id   队列级共支持16个WRED组0-15，端口级支持8组0-7
* @param   dp   共支持8个dp，取值0-7
* @param   p_para   配置的WRED组参数值，用户配置门限值单位为Kbyte，需要转化为Block或者ZXIC_UINT8单位写入寄存器
            包含以下五个参数
           max_th  平均队列深度上限阈值
           min_th  平均队列深度下限阈值
           max_p  最大丢弃概率
           weight   平均队列深度计算权重
           q_len_th 队列深度阈值
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy     @date  2016/08/01
************************************************************/
DPP_STATUS dpp_tm_cgavd_wred_dp_line_para_set(ZXIC_UINT32 dev_id,
                                              DPP_TM_CGAVD_LEVEL_E level,
                                              ZXIC_UINT32 wred_id,
                                              ZXIC_UINT32 dp,
                                              DPP_TM_WRED_DP_LINE_PARA_T *p_para)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 cgavd_cfg_mode = 0;
    ZXIC_UINT32 blk_size = 0;
    DPP_TM_WRED_DP_LINE_PARA_T para = {0};

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, level, QUEUE_LEVEL, PP_LEVEL);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dp, 0, DPP_TM_DP_NUM - 1);

    if (QUEUE_LEVEL == level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, wred_id, 0, DPP_TM_Q_WRED_NUM - 1);
    }
    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, wred_id, 0, DPP_TM_PP_WRED_NUM - 1);
    }

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_para);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_para->max_p, 1, DPP_TM_RED_P_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_para->weight, 0, DPP_TM_CGAVD_WEIGHT_MAX);

    p_para->max_th = p_para->max_th * DPP_TM_CGAVD_KILO_UL;
    p_para->min_th = p_para->min_th * DPP_TM_CGAVD_KILO_UL;
    
    if ((p_para->q_len_th) * DPP_TM_CGAVD_KILO_UL > 0x1fffffff)
    {
        p_para->q_len_th = 0x1fffffff;
    }else{
        p_para->q_len_th = (ZXIC_UINT32)(p_para->q_len_th) * DPP_TM_CGAVD_KILO_UL;
    }

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_para->max_th, p_para->min_th, 0x1fffffff);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_para->min_th, 0, 0x1fffffff);

    para.max_p = p_para->max_p;
    para.weight = p_para->weight;

    if (p_para->max_th == p_para->min_th)
    {
        para.weight = 0;
    }

    rc = dpp_tm_cgavd_cfg_mode_get(dev_id, &cgavd_cfg_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_cfg_mode_get");

    if (cgavd_cfg_mode == DPP_TM_CGAVD_BLOCK_MODE)
    {
        rc  = dpp_tm_cfgmt_blk_size_get(dev_id, &blk_size);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_blk_size_get");

        if (blk_size != 0)
        {
            para.max_th = ((p_para->max_th ) / blk_size);
            para.max_th = ((p_para->max_th) % blk_size == 0) ? (para.max_th ) : ((para.max_th) + 1);
            para.min_th = ((p_para->min_th ) / blk_size);
            para.min_th = ((p_para->min_th) % blk_size == 0) ? (para.min_th ) : ((para.min_th) + 1);
            para.q_len_th = ((p_para->q_len_th ) / blk_size);
            para.q_len_th = ((p_para->q_len_th) % blk_size == 0) ? (para.q_len_th ) : ((para.q_len_th) + 1);
        }

    }
    else if (cgavd_cfg_mode == DPP_TM_CGAVD_ZXIC_UINT8_MODE)
    {
        para.max_th = p_para->max_th;
        para.min_th = p_para->min_th;
        para.q_len_th = p_para->q_len_th;
    }
    else
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "dpp_tm_cgavd_wred_dp_line_para_set err!!\n");
        return DPP_ERR;
    }

    rc = dpp_tm_cgavd_wred_dp_line_block_byte_para_set(dev_id, level, wred_id, dp, &para);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_wred_dp_line_block_byte_para_set");

    return DPP_OK;

}


/***********************************************************/
/** 读取WRED丢弃曲线对应的参数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   WRED支持层次号，0:队列级，1:端口级
* @param   wred_id   队列级共支持16个WRED组0-15，端口级支持8组0-7
* @param   dp   共支持8个dp，取值0-7
* @param   p_para   配置的WRED组参数值，包含以下五个参数
           max_th  平均队列深度上限阈值ZXIC_UINT8/BLOCK单位
           min_th  平均队列深度下限阈值ZXIC_UINT8/BLOCK单位
           max_p  最大丢弃概率
           weight   平均队列深度计算权重
           q_len_th 队列深度阈值ZXIC_UINT8/BLOCK单位
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy     @date  2016/08/01
************************************************************/
DPP_STATUS dpp_tm_cgavd_wred_dp_line_block_byte_para_get(ZXIC_UINT32 dev_id,
                                                         DPP_TM_CGAVD_LEVEL_E level,
                                                         ZXIC_UINT32 wred_id,
                                                         ZXIC_UINT32 dp,
                                                         DPP_TM_WRED_DP_LINE_PARA_T *p_para)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 q_wred_max_th_reg_index = 0;
    ZXIC_UINT32 q_wred_min_th_reg_index = 0;
    ZXIC_UINT32 q_wred_cfg_para_reg_index = 0;
    ZXIC_UINT32 q_wq_reg_index = 0;
    ZXIC_UINT32 q_wred_len_th_reg_index = 0;

    ZXIC_UINT32 pp_wred_max_th_reg_index = 0;
    ZXIC_UINT32 pp_wred_min_th_reg_index = 0;
    ZXIC_UINT32 pp_wred_cfg_para_reg_index = 0;
    ZXIC_UINT32 pp_wq_reg_index = 0;
    ZXIC_UINT32 pp_wred_len_th_reg_index = 0;

    DPP_ETM_CGAVD_FLOW_WRED_MAX_TH_T q_max_th = {0};
    DPP_ETM_CGAVD_FLOW_WRED_MIN_TH_T q_min_th = {0};
    DPP_ETM_CGAVD_FLOW_WRED_CFG_PARA_T q_cfg_para = {0};
    DPP_ETM_CGAVD_FLOW_WQ_T q_wq = {0};
    DPP_ETM_CGAVD_FLOW_WRED_Q_LEN_TH_T q_len_th = {0};
    DPP_ETM_CGAVD_PP_WRED_MAX_TH_T pp_max_th = {0};
    DPP_ETM_CGAVD_PP_WRED_MIN_TH_T pp_min_th = {0};
    DPP_ETM_CGAVD_PP_CFG_PARA_T pp_cfg_para = {0};
    DPP_ETM_CGAVD_PP_WQ_T pp_wq = {0};
    DPP_ETM_CGAVD_PP_WRED_Q_LEN_TH_T pp_len_th = {0};

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, level, QUEUE_LEVEL, PP_LEVEL);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dp, 0, DPP_TM_DP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_para);

    q_wred_max_th_reg_index = ETM_CGAVD_FLOW_WRED_MAX_THr;
    q_wred_min_th_reg_index = ETM_CGAVD_FLOW_WRED_MIN_THr;
    q_wred_cfg_para_reg_index = ETM_CGAVD_FLOW_WRED_CFG_PARAr;
    q_wq_reg_index = ETM_CGAVD_FLOW_WQr;
    q_wred_len_th_reg_index = ETM_CGAVD_FLOW_WRED_Q_LEN_THr;

    pp_wred_max_th_reg_index = ETM_CGAVD_PP_WRED_MAX_THr;
    pp_wred_min_th_reg_index = ETM_CGAVD_PP_WRED_MIN_THr;
    pp_wred_cfg_para_reg_index = ETM_CGAVD_PP_CFG_PARAr;
    pp_wq_reg_index = ETM_CGAVD_PP_WQr;
    pp_wred_len_th_reg_index = ETM_CGAVD_PP_WRED_Q_LEN_THr;

    ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, wred_id , DPP_TM_DP_NUM);
    index = wred_id * DPP_TM_DP_NUM + dp;

    if (QUEUE_LEVEL == level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, wred_id, 0, DPP_TM_Q_WRED_NUM - 1);

        /* q_max_th */
        rc  = dpp_reg_read(dev_id,
                           q_wred_max_th_reg_index,
                           0,
                           index,
                           &q_max_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

        /* q_min_th */
        rc  = dpp_reg_read(dev_id,
                           q_wred_min_th_reg_index,
                           0,
                           index,
                           &q_min_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

        /* q_cfg_para */
        rc  = dpp_reg_read(dev_id,
                           q_wred_cfg_para_reg_index,
                           0,
                           index,
                           &q_cfg_para);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

        /* q_wq */
        rc  = dpp_reg_read(dev_id,
                           q_wq_reg_index,
                           0,
                           wred_id,
                           &q_wq);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

        /* q_len_th */
        rc  = dpp_reg_read(dev_id,
                           q_wred_len_th_reg_index,
                           0,
                           wred_id,
                           &q_len_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

        p_para->max_th = q_max_th.flow_wred_max_th;
        p_para->min_th = q_min_th.flow_wred_min_th;

        if (q_cfg_para.flow_wred_cfg_para != 0)
        {
            p_para->max_p = (ZXIC_UINT32)(100 * (q_max_th.flow_wred_max_th - q_min_th.flow_wred_min_th) / q_cfg_para.flow_wred_cfg_para);
        }
        else
        {
            p_para->max_p = 0;
        }

        p_para->q_len_th = q_len_th.flow_wred_q_len_th;
        p_para->weight = q_wq.wq_flow;
    }

    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, wred_id, 0, DPP_TM_PP_WRED_NUM - 1);

        /* pp_max_th */
        rc  = dpp_reg_read(dev_id,
                           pp_wred_max_th_reg_index,
                           0,
                           index,
                           &pp_max_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

        /* pp_min_th */
        rc  = dpp_reg_read(dev_id,
                           pp_wred_min_th_reg_index,
                           0,
                           index,
                           &pp_min_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

        /* pp_cfg_para */
        rc  = dpp_reg_read(dev_id,
                           pp_wred_cfg_para_reg_index,
                           0,
                           index,
                           &pp_cfg_para);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

        /* pp_wq */
        rc  = dpp_reg_read(dev_id,
                           pp_wq_reg_index,
                           0,
                           wred_id,
                           &pp_wq);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

        /* q_len_th */
        rc  = dpp_reg_read(dev_id,
                           pp_wred_len_th_reg_index,
                           0,
                           wred_id,
                           &pp_len_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

        p_para->max_th = pp_max_th.pp_wred_max_th;
        p_para->min_th = pp_min_th.pp_wred_min_th;

        if (pp_cfg_para.pp_cfg_para != 0)
        {
            p_para->max_p = (ZXIC_UINT32)(100 * (pp_max_th.pp_wred_max_th - pp_min_th.pp_wred_min_th) / pp_cfg_para.pp_cfg_para);
        }
        else
        {
            p_para->max_p = 0;
        }

        p_para->q_len_th = pp_len_th.pp_wred_q_len_th;
        p_para->weight = pp_wq.wq_pp;
    }

    return DPP_OK;

}

/***********************************************************/
/** 读取WRED丢弃曲线对应的参数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   WRED支持层次号，0:队列级，1:端口级
* @param   wred_id   队列级共支持16个WRED组0-15，端口级支持8组0-7
* @param   dp   共支持8个dp，取值0-7
* @param   p_para   配置的WRED组参数值，包含以下五个参数
           max_th  平均队列深度上限阈值
           min_th  平均队列深度下限阈值
           max_p  最大丢弃概率
           weight   平均队列深度计算权重
           q_len_th 队列深度阈值
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author cy     @date  2016/08/01
************************************************************/
DPP_STATUS dpp_tm_cgavd_wred_dp_line_para_get(ZXIC_UINT32 dev_id,
                                              DPP_TM_CGAVD_LEVEL_E level,
                                              ZXIC_UINT32 wred_id,
                                              ZXIC_UINT32 dp,
                                              DPP_TM_WRED_DP_LINE_PARA_T *p_para)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 blk_size = 0;
    ZXIC_UINT32 cgavd_cfg_mode = 0;
    DPP_TM_WRED_DP_LINE_PARA_T para = {0};

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, level, QUEUE_LEVEL, PP_LEVEL);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dp, 0, DPP_TM_DP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_para);

    if (QUEUE_LEVEL == level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, wred_id, 0, DPP_TM_Q_WRED_NUM - 1);
    }

    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, wred_id, 0, DPP_TM_PP_WRED_NUM - 1);
    }

    rc = dpp_tm_cgavd_wred_dp_line_block_byte_para_get(dev_id, level, wred_id, dp, &para);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_wred_dp_line_block_byte_para_set");

    p_para->max_p = para.max_p;
    p_para->weight = para.weight;

    rc = dpp_tm_cgavd_cfg_mode_get(dev_id, &cgavd_cfg_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_cfg_mode_get");

    if (cgavd_cfg_mode == DPP_TM_CGAVD_BLOCK_MODE)
    {
        rc  = dpp_tm_cfgmt_blk_size_get(dev_id, &blk_size);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_blk_size_get");
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, blk_size, 256, 1024);

        p_para->max_th = (para.max_th * blk_size) / DPP_TM_CGAVD_KILO_UL;
        p_para->min_th = (para.min_th * blk_size) / DPP_TM_CGAVD_KILO_UL;
        p_para->q_len_th = (para.q_len_th * blk_size) / DPP_TM_CGAVD_KILO_UL;

    }
    else if (cgavd_cfg_mode == DPP_TM_CGAVD_ZXIC_UINT8_MODE)
    {
        p_para->max_th = (para.max_th / DPP_TM_CGAVD_KILO_UL);
        p_para->min_th = (para.min_th / DPP_TM_CGAVD_KILO_UL);
        p_para->q_len_th = (para.q_len_th / DPP_TM_CGAVD_KILO_UL);

    }
    else
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "dpp_tm_cgavd_wred_dp_line_para_get err!!\n");
        return DPP_ERR;
    }

    return DPP_OK;

}


/***********************************************************/
/** 配置系统级GRED丢弃曲线对应的参数
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   dp   共支持8个dp，取值0-7
* @param   p_para  配置的GRED丢弃曲线参数值，包含以下六个参数
           max_th  平均队列深度上限阈值ZXIC_UINT8/BLOCK单位
           mid_th  平均队列深度中间阈值ZXIC_UINT8/BLOCK单位
           min_th  平均队列深度下限阈值ZXIC_UINT8/BLOCK单位
           max_p   最大丢弃概率[1-99]
           weight   平均队列深度计算权重
           q_len_th 队列深度阈值ZXIC_UINT8/BLOCK单位
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/08/02
************************************************************/
DPP_STATUS dpp_tm_cgavd_gred_dp_line_block_byte_para_set(ZXIC_UINT32 dev_id,
                                                         ZXIC_UINT32 dp,
                                                         DPP_TM_GRED_DP_LINE_PARA_T *p_para)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 gred_max_th_reg_index = 0;
    ZXIC_UINT32 gred_mid_th_reg_index = 0;
    ZXIC_UINT32 gred_min_th_reg_index = 0;
    ZXIC_UINT32 gred_cfg_para0_reg_index = 0;
    ZXIC_UINT32 gred_cfg_para1_reg_index = 0;
    ZXIC_UINT32 gred_cfg_para2_reg_index = 0;
    ZXIC_UINT32 gred_sys_wfq_reg_index = 0;
    ZXIC_UINT32 gred_sys_cfg_q_grp_para_reg_index = 0;

    DPP_ETM_CGAVD_GRED_MAX_TH_T max_th = {0};
    DPP_ETM_CGAVD_GRED_MID_TH_T mid_th = {0};
    DPP_ETM_CGAVD_GRED_MIN_TH_T min_th = {0};
    DPP_ETM_CGAVD_GRED_CFG_PARA0_T cfg_para0 = {0};
    DPP_ETM_CGAVD_GRED_CFG_PARA1_T cfg_para1 = {0};
    DPP_ETM_CGAVD_GRED_CFG_PARA2_T cfg_para2 = {0};
    DPP_ETM_CGAVD_SYS_WQ_T sys_wq = {0};
    DPP_ETM_CGAVD_SYS_CFG_Q_GRP_PARA_T sys_len_th = {0};

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dp, 0, DPP_TM_DP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_para);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_para->max_p, 1, DPP_TM_RED_P_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_para->weight, 0, DPP_TM_CGAVD_WEIGHT_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_para->max_th, 0, 0x1fffffff);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_para->mid_th, 0, 0x1fffffff);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_para->min_th, 0, 0x1fffffff);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_para->q_len_th, 0, 0x1fffffff);

    gred_max_th_reg_index = ETM_CGAVD_GRED_MAX_THr;
    gred_mid_th_reg_index = ETM_CGAVD_GRED_MID_THr;
    gred_min_th_reg_index = ETM_CGAVD_GRED_MIN_THr;
    gred_cfg_para0_reg_index = ETM_CGAVD_GRED_CFG_PARA0r;
    gred_cfg_para1_reg_index = ETM_CGAVD_GRED_CFG_PARA1r;
    gred_cfg_para2_reg_index = ETM_CGAVD_GRED_CFG_PARA2r;
    gred_sys_wfq_reg_index = ETM_CGAVD_SYS_WQr;
    gred_sys_cfg_q_grp_para_reg_index = ETM_CGAVD_SYS_CFG_Q_GRP_PARAr;

    /* max_th */
    max_th.gred_max_th = p_para->max_th;
    rc  = dpp_reg_write(dev_id,
                        gred_max_th_reg_index,
                        0,
                        dp,
                        &max_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    /* mid_th */
    mid_th.gred_mid_th = p_para->mid_th;
    rc  = dpp_reg_write(dev_id,
                        gred_mid_th_reg_index,
                        0,
                        dp,
                        &mid_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    /* min_th */
    min_th.gred_min_th = p_para->min_th;
    rc  = dpp_reg_write(dev_id,
                        gred_min_th_reg_index,
                        0,
                        dp,
                        &min_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    /* cfg_para0，乘以100用来换算百分比，用户配置的是值是1-100 */
    ZXIC_COMM_CHECK_DEV_INDEX_SUB_OVERFLOW_NO_ASSERT(dev_id, mid_th.gred_mid_th+1 , min_th.gred_min_th);
    ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, 100 , (mid_th.gred_mid_th - min_th.gred_min_th));
    cfg_para0.gred_cfg_para0 = (ZXIC_UINT32)(100 * (mid_th.gred_mid_th - min_th.gred_min_th) / p_para->max_p);

    rc  = dpp_reg_write(dev_id,
                        gred_cfg_para0_reg_index,
                        0,
                        dp,
                        &cfg_para0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    /* cfg_para1 */
    ZXIC_COMM_CHECK_DEV_INDEX_SUB_OVERFLOW_NO_ASSERT(dev_id, max_th.gred_max_th+1 , mid_th.gred_mid_th);
    cfg_para1.gred_cfg_para1 = (ZXIC_UINT32)(100 * (max_th.gred_max_th - mid_th.gred_mid_th) / (100 - p_para->max_p));

    rc  = dpp_reg_write(dev_id,
                        gred_cfg_para1_reg_index,
                        0,
                        dp,
                        &cfg_para1);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");


    /* cfg_para2 */
    ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, p_para->max_p , cfg_para1.gred_cfg_para1);
    cfg_para2.gred_cfg_para2 = (ZXIC_UINT32)(p_para->max_p * cfg_para1.gred_cfg_para1 / 100);

    rc  = dpp_reg_write(dev_id,
                        gred_cfg_para2_reg_index,
                        0,
                        dp,
                        &cfg_para2);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    /* 必须先写sys_len_th */
    sys_len_th.gred_q_len_th_sys = p_para->q_len_th;
    rc  = dpp_reg_write(dev_id,
                        gred_sys_cfg_q_grp_para_reg_index,
                        0,
                        0,
                        &sys_len_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    /* 再写sys_wq */
    sys_wq.wq_sys = p_para->weight;
    rc  = dpp_reg_write(dev_id,
                        gred_sys_wfq_reg_index,
                        0,
                        0,
                        &sys_wq);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 配置系统级GRED丢弃曲线对应的参数
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   dp   共支持8个dp，取值0-7
* @param   p_para  配置的GRED丢弃曲线参数值，用户配置门限值单位为Kbyte，需要转化为Block或者ZXIC_UINT8单位写入寄存器
        包含以下六个参数
           max_th  平均队列深度上限阈值
           mid_th  平均队列深度中间阈值
           min_th  平均队列深度下限阈值
           max_p   最大丢弃概率
           weight   平均队列深度计算权重
           q_len_th 队列深度阈值
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/08/02
************************************************************/
DPP_STATUS dpp_tm_cgavd_gred_dp_line_para_set(ZXIC_UINT32 dev_id,
                                              ZXIC_UINT32 dp,
                                              DPP_TM_GRED_DP_LINE_PARA_T *p_para)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 cgavd_cfg_mode = 0;
    ZXIC_UINT32 blk_size = 0;
    DPP_TM_GRED_DP_LINE_PARA_T para = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dp, 0, DPP_TM_DP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_para);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_para->max_p, 1, DPP_TM_RED_P_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_para->weight, 0, DPP_TM_CGAVD_WEIGHT_MAX);

    p_para->max_th = p_para->max_th * DPP_TM_CGAVD_KILO_UL;
    p_para->mid_th = p_para->mid_th * DPP_TM_CGAVD_KILO_UL;
    p_para->min_th = p_para->min_th * DPP_TM_CGAVD_KILO_UL;
    if ((p_para->q_len_th) * DPP_TM_CGAVD_KILO_UL > 0x1fffffff)
    {
        p_para->q_len_th = 0x1fffffff;
    }else{
        p_para->q_len_th = (p_para->q_len_th) * DPP_TM_CGAVD_KILO_UL;
    }
    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_para->max_th, 0, 0x1fffffff);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_para->mid_th, 0, 0x1fffffff);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, p_para->min_th, 0, 0x1fffffff);
    
    para.max_p = p_para->max_p;
    para.weight = p_para->weight;

    rc = dpp_tm_cgavd_cfg_mode_get(dev_id, &cgavd_cfg_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_cfg_mode_get");

    if (cgavd_cfg_mode == DPP_TM_CGAVD_BLOCK_MODE)
    {
        rc  = dpp_tm_cfgmt_blk_size_get(dev_id, &blk_size);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_blk_size_get");

        if (blk_size != 0)
        {
            para.max_th = ((p_para->max_th ) / blk_size);
            para.max_th = ((p_para->max_th) % blk_size == 0) ? (para.max_th ) : ((para.max_th) + 1);
            para.mid_th = ((p_para->mid_th ) / blk_size);
            para.mid_th = ((p_para->mid_th) % blk_size == 0) ? (para.mid_th ) : ((para.mid_th) + 1);
            para.min_th = ((p_para->min_th ) / blk_size);
            para.min_th = ((p_para->min_th) % blk_size == 0) ? (para.min_th ) : ((para.min_th) + 1);
            para.q_len_th = ((p_para->q_len_th ) / blk_size);
            para.q_len_th = ((p_para->q_len_th) % blk_size == 0) ? (para.q_len_th ) : ((para.q_len_th) + 1);
        }

    }
    else if (cgavd_cfg_mode == DPP_TM_CGAVD_ZXIC_UINT8_MODE)
    {
        para.max_th = p_para->max_th;
        para.mid_th = p_para->mid_th;
        para.min_th = p_para->min_th;
        para.q_len_th = p_para->q_len_th;
    }
    else
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "dpp_tm_cgavd_gred_dp_line_para_set err!!\n");
        return DPP_ERR;
    }

    rc = dpp_tm_cgavd_gred_dp_line_block_byte_para_set(dev_id, dp, &para);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_gred_dp_line_block_byte_para_set");

    return DPP_OK;
}


/***********************************************************/
/** 配置系统级阶梯TD 丢弃曲线对应的参数
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   dp   共支持8个dp，取值0-7
* @param   td_th  TD 门限
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/08/02
************************************************************/
DPP_STATUS dpp_tm_cgavd_ladtd_dp_line_para_set(ZXIC_UINT32 dev_id,
                                               ZXIC_UINT32 dp,
                                               ZXIC_UINT32 td_th)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 gred_max_th_reg_index = 0;
    ZXIC_UINT32 gred_mid_th_reg_index = 0;
    ZXIC_UINT32 gred_min_th_reg_index = 0;
    ZXIC_UINT32 gred_cfg_para0_reg_index = 0;
    ZXIC_UINT32 gred_cfg_para1_reg_index = 0;
    ZXIC_UINT32 gred_cfg_para2_reg_index = 0;
    ZXIC_UINT32 cgavd_cfg_mode = 0;
    ZXIC_UINT32 blk_size = 0;

    DPP_ETM_CGAVD_GRED_MAX_TH_T max_th = {0};
    DPP_ETM_CGAVD_GRED_MID_TH_T mid_th = {0};
    DPP_ETM_CGAVD_GRED_MIN_TH_T min_th = {0};
    DPP_ETM_CGAVD_GRED_CFG_PARA0_T cfg_para0 = {0};
    DPP_ETM_CGAVD_GRED_CFG_PARA1_T cfg_para1 = {0};
    DPP_ETM_CGAVD_GRED_CFG_PARA2_T cfg_para2 = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dp, 0, DPP_TM_DP_NUM - 1);

    td_th = td_th * DPP_TM_CGAVD_KILO_UL;

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, td_th, 0, 0x1fffffff);

    rc = dpp_tm_cgavd_cfg_mode_get(dev_id, &cgavd_cfg_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_cfg_mode_get");

    if (cgavd_cfg_mode == DPP_TM_CGAVD_BLOCK_MODE)
    {
        rc  = dpp_tm_cfgmt_blk_size_get(dev_id, &blk_size);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_blk_size_get");

        if (blk_size != 0)
        {
            max_th.gred_max_th = (( td_th ) / blk_size);
            max_th.gred_max_th = ((td_th) % blk_size == 0) ? ( max_th.gred_max_th ) : (( max_th.gred_max_th) + 1);
            mid_th.gred_mid_th = (( td_th ) / blk_size);
            mid_th.gred_mid_th = ((td_th) % blk_size == 0) ? ( mid_th.gred_mid_th ) : (( mid_th.gred_mid_th) + 1);
            min_th.gred_min_th = (( td_th ) / blk_size);
            min_th.gred_min_th = ((td_th) % blk_size == 0) ? ( min_th.gred_min_th ) : (( min_th.gred_min_th) + 1);

        }

    }
    else if (cgavd_cfg_mode == DPP_TM_CGAVD_ZXIC_UINT8_MODE)
    {
        max_th.gred_max_th = td_th;
        mid_th.gred_mid_th = td_th;
        min_th.gred_min_th = td_th;
    }
    else
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "dpp_tm_cgavd_gred_dp_line_para_get err!!\n");
        return DPP_ERR;
    }

    gred_max_th_reg_index = ETM_CGAVD_GRED_MAX_THr;
    gred_mid_th_reg_index = ETM_CGAVD_GRED_MID_THr;
    gred_min_th_reg_index = ETM_CGAVD_GRED_MIN_THr;
    gred_cfg_para0_reg_index = ETM_CGAVD_GRED_CFG_PARA0r;
    gred_cfg_para1_reg_index = ETM_CGAVD_GRED_CFG_PARA1r;
    gred_cfg_para2_reg_index = ETM_CGAVD_GRED_CFG_PARA2r;

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, td_th, 0, 0x1fffffff);


    /* max_th */
    rc  = dpp_reg_write(dev_id,
                        gred_max_th_reg_index,
                        0,
                        dp,
                        &max_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    /* mid_th */
    rc  = dpp_reg_write(dev_id,
                        gred_mid_th_reg_index,
                        0,
                        dp,
                        &mid_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    /* min_th */
    rc  = dpp_reg_write(dev_id,
                        gred_min_th_reg_index,
                        0,
                        dp,
                        &min_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    /* cfg_para0，0 */
    cfg_para0.gred_cfg_para0 = 0;

    rc  = dpp_reg_write(dev_id,
                        gred_cfg_para0_reg_index,
                        0,
                        dp,
                        &cfg_para0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    /* cfg_para1 */
    cfg_para1.gred_cfg_para1 = 0;

    rc  = dpp_reg_write(dev_id,
                        gred_cfg_para1_reg_index,
                        0,
                        dp,
                        &cfg_para1);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");


    /* cfg_para2 */
    cfg_para2.gred_cfg_para2 = 0;

    rc  = dpp_reg_write(dev_id,
                        gred_cfg_para2_reg_index,
                        0,
                        dp,
                        &cfg_para2);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");


    return DPP_OK;
}


/***********************************************************/
/** 读取系统级GRED丢弃曲线对应的参数
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   dp   共支持8个dp，取值0-7
* @param   p_para  配置的GRED丢弃曲线参数值，包含以下六个参数
           max_th  平均队列深度上限阈值ZXIC_UINT8/BLOCK单位
           mid_th  平均队列深度中间阈值ZXIC_UINT8/BLOCK单位
           min_th  平均队列深度下限阈值ZXIC_UINT8/BLOCK单位
           max_p   最大丢弃概率
           weight   平均队列深度计算权重
           q_len_th 队列深度阈值ZXIC_UINT8/BLOCK单位
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/08/02
************************************************************/
DPP_STATUS dpp_tm_cgavd_gred_dp_line_block_byte_para_get(ZXIC_UINT32 dev_id,
                                                         ZXIC_UINT32 dp,
                                                         DPP_TM_GRED_DP_LINE_PARA_T *p_para)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 gred_max_th_reg_index = 0;
    ZXIC_UINT32 gred_mid_th_reg_index = 0;
    ZXIC_UINT32 gred_min_th_reg_index = 0;
    ZXIC_UINT32 gred_cfg_para0_reg_index = 0;
    ZXIC_UINT32 gred_cfg_para1_reg_index = 0;
    ZXIC_UINT32 gred_cfg_para2_reg_index = 0;
    ZXIC_UINT32 gred_sys_wfq_reg_index = 0;
    ZXIC_UINT32 gred_sys_cfg_q_grp_para_reg_index = 0;

    DPP_ETM_CGAVD_GRED_MAX_TH_T max_th = {0};
    DPP_ETM_CGAVD_GRED_MID_TH_T mid_th = {0};
    DPP_ETM_CGAVD_GRED_MIN_TH_T min_th = {0};
    DPP_ETM_CGAVD_GRED_CFG_PARA0_T cfg_para0 = {0};
    DPP_ETM_CGAVD_GRED_CFG_PARA1_T cfg_para1 = {0};
    DPP_ETM_CGAVD_GRED_CFG_PARA2_T cfg_para2 = {0};
    DPP_ETM_CGAVD_SYS_WQ_T sys_wq = {0};
    DPP_ETM_CGAVD_SYS_CFG_Q_GRP_PARA_T sys_len_th = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dp, 0, DPP_TM_DP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_para);

    gred_max_th_reg_index = ETM_CGAVD_GRED_MAX_THr;
    gred_mid_th_reg_index = ETM_CGAVD_GRED_MID_THr;
    gred_min_th_reg_index = ETM_CGAVD_GRED_MIN_THr;
    gred_cfg_para0_reg_index = ETM_CGAVD_GRED_CFG_PARA0r;
    gred_cfg_para1_reg_index = ETM_CGAVD_GRED_CFG_PARA1r;
    gred_cfg_para2_reg_index = ETM_CGAVD_GRED_CFG_PARA2r;
    gred_sys_wfq_reg_index = ETM_CGAVD_SYS_WQr;
    gred_sys_cfg_q_grp_para_reg_index = ETM_CGAVD_SYS_CFG_Q_GRP_PARAr;

    /* max_th */
    rc  = dpp_reg_read(dev_id,
                       gred_max_th_reg_index,
                       0,
                       dp,
                       &max_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    /* mid_th */
    rc  = dpp_reg_read(dev_id,
                       gred_mid_th_reg_index,
                       0,
                       dp,
                       &mid_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    /* min_th */
    rc  = dpp_reg_read(dev_id,
                       gred_min_th_reg_index,
                       0,
                       dp,
                       &min_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    /* cfg_para0 */
    rc  = dpp_reg_read(dev_id,
                       gred_cfg_para0_reg_index,
                       0,
                       dp,
                       &cfg_para0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    /* cfg_para1 */
    rc  = dpp_reg_read(dev_id,
                       gred_cfg_para1_reg_index,
                       0,
                       dp,
                       &cfg_para1);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");


    /* cfg_para2 */
    rc  = dpp_reg_read(dev_id,
                       gred_cfg_para2_reg_index,
                       0,
                       dp,
                       &cfg_para2);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    /* sys_len_th */
    rc  = dpp_reg_read(dev_id,
                       gred_sys_cfg_q_grp_para_reg_index,
                       0,
                       0,
                       &sys_len_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    /* sys_wq */
    rc  = dpp_reg_read(dev_id,
                       gred_sys_wfq_reg_index,
                       0,
                       0,
                       &sys_wq);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    p_para->max_th = max_th.gred_max_th;
    p_para->mid_th = mid_th.gred_mid_th;
    p_para->min_th = min_th.gred_min_th;

    if (cfg_para0.gred_cfg_para0)
    {
        p_para->max_p  = (ZXIC_UINT32)(100 * (mid_th.gred_mid_th - min_th.gred_min_th) / cfg_para0.gred_cfg_para0);
    }

    p_para->weight = sys_wq.wq_sys;
    p_para->q_len_th = sys_len_th.gred_q_len_th_sys;

    return DPP_OK;

}


/***********************************************************/
/** 读取系统级GRED丢弃曲线对应的参数
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   dp   共支持8个dp，取值0-7
* @param   p_para  配置的GRED丢弃曲线参数值，包含以下六个参数
           max_th  平均队列深度上限阈值
           mid_th  平均队列深度中间阈值
           min_th  平均队列深度下限阈值
           max_p   最大丢弃概率
           weight   平均队列深度计算权重
           q_len_th 队列深度阈值
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  taq      @date  2015/04/20
************************************************************/
DPP_STATUS dpp_tm_cgavd_gred_dp_line_para_get(ZXIC_UINT32 dev_id,
                                              ZXIC_UINT32 dp,
                                              DPP_TM_GRED_DP_LINE_PARA_T *p_para)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 cgavd_cfg_mode = 0;
    ZXIC_UINT32 blk_size = 0;
    DPP_TM_GRED_DP_LINE_PARA_T para = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dp, 0, DPP_TM_DP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_para);
    

    rc = dpp_tm_cgavd_gred_dp_line_block_byte_para_get(dev_id, dp, &para);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_gred_dp_line_block_byte_para_get");

    p_para->max_p = para.max_p;
    p_para->weight = para.weight;

    rc = dpp_tm_cgavd_cfg_mode_get(dev_id, &cgavd_cfg_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_cfg_mode_get");

    if (cgavd_cfg_mode == DPP_TM_CGAVD_BLOCK_MODE)
    {
        rc  = dpp_tm_cfgmt_blk_size_get(dev_id, &blk_size);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_blk_size_get");

        p_para->max_th = (para.max_th * blk_size) / DPP_TM_CGAVD_KILO_UL;
        p_para->mid_th = (para.mid_th * blk_size) / DPP_TM_CGAVD_KILO_UL;
        p_para->min_th = (para.min_th * blk_size) / DPP_TM_CGAVD_KILO_UL;
        p_para->q_len_th = (para.q_len_th * blk_size) / DPP_TM_CGAVD_KILO_UL;

    }
    else if (cgavd_cfg_mode == DPP_TM_CGAVD_ZXIC_UINT8_MODE)
    {
        p_para->max_th = (para.max_th / DPP_TM_CGAVD_KILO_UL);
        p_para->mid_th = (para.mid_th / DPP_TM_CGAVD_KILO_UL);
        p_para->min_th = (para.min_th / DPP_TM_CGAVD_KILO_UL);
        p_para->q_len_th = (para.q_len_th / DPP_TM_CGAVD_KILO_UL);

    }
    else
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "dpp_tm_cgavd_gred_dp_line_para_get err!!\n");
        return DPP_ERR;
    }

    return DPP_OK;

}

#endif
/***********************************************************/
/** 配置指定端口或队列是否支持动态门限机制
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   WRED支持层次号，0:队列级，1:端口级
* @param   id   队列号或端口号
* @param   en   配置的值，0-不支持动态门限机制，1-支持动态门限机制
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_dyn_th_en_set(DPP_DEV_T *dev,
                                      DPP_TM_CGAVD_LEVEL_E level,
                                      ZXIC_UINT32 id,
                                      ZXIC_UINT32 en)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 q_dyn_th_en_reg_index = 0;
    ZXIC_UINT32 pp_wrd_grp_th_en_reg_index = 0;

    DPP_ETM_CGAVD_FLOW_DYNAMIC_TH_EN_T q_dyn_th_en = {0};
    DPP_ETM_CGAVD_PP_WRED_GRP_TH_EN_T pp_dyn_th_en = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), level, QUEUE_LEVEL, PP_LEVEL);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), en, 0, 1);

    q_dyn_th_en_reg_index = ETM_CGAVD_FLOW_DYNAMIC_TH_ENr;
    pp_wrd_grp_th_en_reg_index = ETM_CGAVD_PP_WRED_GRP_TH_ENr;

    if (QUEUE_LEVEL == level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), id, 0, DPP_ETM_Q_NUM - 1);

        q_dyn_th_en.flow_dynamic_th_en = en;
        rc  = dpp_reg_write(dev,
                            q_dyn_th_en_reg_index,
                            0,
                            id,
                            &q_dyn_th_en);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
    }

    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), id, 0, DPP_TM_PP_NUM - 1);

        rc  = dpp_reg_read(dev,
                           pp_wrd_grp_th_en_reg_index,
                           0,
                           id,
                           &pp_dyn_th_en);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

        pp_dyn_th_en.pp_wred_grp_th_en = en;
        rc  = dpp_reg_write(dev,
                            pp_wrd_grp_th_en_reg_index,
                            0,
                            id,
                            &pp_dyn_th_en);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
    }

    return DPP_OK;

}


/***********************************************************/
/** 读取指定端口或队列是否支持动态门限机制
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   WRED支持层次号，0:队列级，1:端口级
* @param   id   队列号或端口号
* @param   p_en   读取的值，0-不支持动态门限机制，1-支持动态门限机制
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_dyn_th_en_get(DPP_DEV_T *dev,
                                      DPP_TM_CGAVD_LEVEL_E level,
                                      ZXIC_UINT32 id,
                                      ZXIC_UINT32 *p_en)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 q_dyn_th_en_reg_index = 0;
    ZXIC_UINT32 pp_wrd_grp_th_en_reg_index = 0;

    DPP_ETM_CGAVD_FLOW_DYNAMIC_TH_EN_T q_dyn_th_en = {0};
    DPP_ETM_CGAVD_PP_WRED_GRP_TH_EN_T pp_dyn_th_en = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), level, QUEUE_LEVEL, PP_LEVEL);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_en);

    q_dyn_th_en_reg_index = ETM_CGAVD_FLOW_DYNAMIC_TH_ENr;
    pp_wrd_grp_th_en_reg_index = ETM_CGAVD_PP_WRED_GRP_TH_ENr;

    if (QUEUE_LEVEL == level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), id, 0, DPP_ETM_Q_NUM - 1);

        rc  = dpp_reg_read(dev,
                           q_dyn_th_en_reg_index,
                           0,
                           id,
                           &q_dyn_th_en);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

        *p_en = q_dyn_th_en.flow_dynamic_th_en;

    }

    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), id, 0, DPP_TM_PP_NUM - 1);

        rc  = dpp_reg_read(dev,
                           pp_wrd_grp_th_en_reg_index,
                           0,
                           id,
                           &pp_dyn_th_en);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

        *p_en = pp_dyn_th_en.pp_wred_grp_th_en;
    }

    return DPP_OK;

}


/***********************************************************/
/**  配置等价包长使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   en   使能标记，0-不使能，1-使能
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/28
************************************************************/
DPP_STATUS dpp_tm_cgavd_equal_pkt_len_en_set(DPP_DEV_T *dev, ZXIC_UINT32 en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_EQUAL_PKT_LEN_EN_T equal_pkt_len_en = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), en, 0, 1);

    equal_pkt_len_en.equal_pkt_len_en = en;
    rc  = dpp_reg_write(dev,
                        ETM_CGAVD_EQUAL_PKT_LEN_ENr,
                        0,
                        0,
                        &equal_pkt_len_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/**  读取等价包长使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_en   使能标记，0-不使能，1-使能
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/28
************************************************************/
DPP_STATUS dpp_tm_cgavd_equal_pkt_len_en_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_EQUAL_PKT_LEN_EN_T equal_pkt_len_en = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_en);

    *p_en = 0xffffffff;
    rc  = dpp_reg_read(dev,
                       ETM_CGAVD_EQUAL_PKT_LEN_ENr,
                       0,
                       0,
                       &equal_pkt_len_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
    *p_en = equal_pkt_len_en.equal_pkt_len_en;

    return DPP_OK;
}

/***********************************************************/
/** 配置等价包长
* @param   dev_id
* @param   tm_type   0-ETM,1-FTM
* @param   p_equal_pkt_len 等价包长
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/28
************************************************************/
DPP_STATUS dpp_tm_cgavd_equal_pkt_len_para_set(DPP_DEV_T *dev,
                                               DPP_ETM_EQUAL_PKT_LEN_PARA_T *p_equal_pkt_len)
{
    /* 返回值变量定义 */
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 i = 0;

    /* 结构体变量定义 */
    DPP_ETM_CGAVD_EQUAL_PKT_LEN0_T equal_pkt_len0 = {0};

    /* 入参检查 */
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_equal_pkt_len);

    for (i = 0; i < 8; i++)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), p_equal_pkt_len->equal_pkt_len[i], 0x0, 0x7fff);
        equal_pkt_len0.equal_pkt_len0 = p_equal_pkt_len->equal_pkt_len[i];
        rc = dpp_reg_write(dev,
                           ETM_CGAVD_EQUAL_PKT_LEN0r + i,
                           0,
                           0,
                           &equal_pkt_len0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    }

    return DPP_OK;
}



/***********************************************************/
/** 读取等价包长
* @param   dev_id
* @param   tm_type   0-ETM,1-FTM
* @param   p_equal_pkt_len 等价包长
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/28
************************************************************/
DPP_STATUS dpp_tm_cgavd_equal_pkt_len_para_get(DPP_DEV_T *dev,
                                               DPP_ETM_EQUAL_PKT_LEN_PARA_T *p_equal_pkt_len)
{
    /* 返回值变量定义 */
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 i = 0;

    /* 结构体变量定义 */
    DPP_ETM_CGAVD_EQUAL_PKT_LEN0_T equal_pkt_len0 = {0};

    /* 入参检查 */
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_equal_pkt_len);

    for (i = 0; i < 8; i++)
    {
        ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(ETM_CGAVD_EQUAL_PKT_LEN0r, i);
        rc = dpp_reg_read(dev,
                          ETM_CGAVD_EQUAL_PKT_LEN0r + i,
                          0,
                          0,
                          &equal_pkt_len0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
        p_equal_pkt_len->equal_pkt_len[i] = equal_pkt_len0.equal_pkt_len0;
    }

    return DPP_OK;
}

/***********************************************************/
/** 配置等价包长阈值
* @param   dev_id
* @param   tm_type   0-ETM,1-FTM
* @param   p_equal_pkt_len_th 等价包长阈值
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/28
************************************************************/
DPP_STATUS dpp_tm_cgavd_equal_pkt_len_th_para_set(DPP_DEV_T *dev,
                                                  DPP_ETM_EQUAL_PKT_LEN_TH_PARA_T *p_equal_pkt_len_th)
{
    /* 返回值变量定义 */
    DPP_STATUS rc = 0;
    ZXIC_UINT32 i = 0;

    /* 结构体变量定义 */
    DPP_ETM_CGAVD_EQUAL_PKT_LEN_TH0_T equal_pkt_len_th0 = {0};

    /* 入参检查 */
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_equal_pkt_len_th);

    for (i = 0; i < 7; i++)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), p_equal_pkt_len_th->equal_pkt_len_th[i], 0x0, 0x7fff);

        if (i <= 5 )
        {
            ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), p_equal_pkt_len_th->equal_pkt_len_th[i + 1], p_equal_pkt_len_th->equal_pkt_len_th[i], 0x7fff);
        }

        equal_pkt_len_th0.equal_pkt_len_th0 = p_equal_pkt_len_th->equal_pkt_len_th[i];
        rc = dpp_reg_write(dev,
                           ETM_CGAVD_EQUAL_PKT_LEN_TH0r + i,
                           0,
                           0,
                           &equal_pkt_len_th0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
    }

    return DPP_OK;
}


/***********************************************************/
/** 读取等价包长阈值
* @param   dev_id
* @param   tm_type   0-ETM,1-FTM
* @param   p_equal_pkt_len_th 等价包长阈值
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/28
************************************************************/
DPP_STATUS dpp_tm_cgavd_equal_pkt_len_th_para_get(DPP_DEV_T *dev,
                                                  DPP_ETM_EQUAL_PKT_LEN_TH_PARA_T *p_equal_pkt_len_th)
{
    /* 返回值变量定义 */
    DPP_STATUS rc = 0;
    ZXIC_UINT32 i = 0;

    /* 结构体变量定义 */
    DPP_ETM_CGAVD_EQUAL_PKT_LEN_TH0_T equal_pkt_len_th0 = {0};

    /* 入参检查 */
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_equal_pkt_len_th);

    for (i = 0; i < 7; i++)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev),   i, ETM_CGAVD_EQUAL_PKT_LEN_TH0r);    
        rc = dpp_reg_read(dev,
                          ETM_CGAVD_EQUAL_PKT_LEN_TH0r + i,
                          0,
                          0,
                          &equal_pkt_len_th0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
        p_equal_pkt_len_th->equal_pkt_len_th[i] = equal_pkt_len_th0.equal_pkt_len_th0;
    }

    return DPP_OK;
}

/***********************************************************/
/** 动态门限放大因子参数配置
* @param   dev_id
* @param   tm_type   0-ETM,1-FTM
* @param   p_amplify_gene_para 动态门限放大因子参数
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/28
************************************************************/
DPP_STATUS dpp_tm_cgavd_amplify_gene_para_set(DPP_DEV_T *dev,
                                              DPP_ETM_AMPLIFY_GENE_PARA_T *p_amplify_gene_para)
{
    /* 返回值变量定义 */
    DPP_STATUS rc = 0;
    ZXIC_UINT32 i = 0;

    /* 结构体变量定义 */
    DPP_ETM_CGAVD_AMPLIFY_GENE0_T amplify_gene0 = {0};

    /* 入参检查 */
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_amplify_gene_para);

    for (i = 0; i < 16; i++)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), p_amplify_gene_para->amplify_gene[i], 0x0, 0xfff);
        amplify_gene0.amplify_gene0 = p_amplify_gene_para->amplify_gene[i];
        rc = dpp_reg_write(dev,
                           ETM_CGAVD_AMPLIFY_GENE0r + i,
                           0,
                           0,
                           &amplify_gene0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
    }

    return DPP_OK;
}

/***********************************************************/
/** 动态门限放大因子参数获取
* @param   dev_id
* @param   tm_type   0-ETM,1-FTM
* @param   p_amplify_gene_para 动态门限放大因子参数
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/28
************************************************************/
DPP_STATUS dpp_tm_cgavd_amplify_gene_para_get(DPP_DEV_T *dev,
                                              DPP_ETM_AMPLIFY_GENE_PARA_T *p_amplify_gene_para)
{
    /* 返回值变量定义 */
    DPP_STATUS rc = 0;
    ZXIC_UINT32 i = 0;

    /* 结构体变量定义 */
    DPP_ETM_CGAVD_AMPLIFY_GENE0_T amplify_gene0 = {0};

    /* 入参检查 */
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_amplify_gene_para);

    for (i = 0; i < 16; i++)
    {
        ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(ETM_CGAVD_AMPLIFY_GENE0r, i);
        rc = dpp_reg_read(dev,
                          ETM_CGAVD_AMPLIFY_GENE0r + i,
                          0,
                          0,
                          &amplify_gene0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
        p_amplify_gene_para->amplify_gene[i] = amplify_gene0.amplify_gene0;
    }

    return DPP_OK;
}

#if 0
/***********************************************************/
/**  配置默认队列使能
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   en   配置的值，0-不使能默认队列，1-使能默认队列，
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush     @date  2016/08/16
************************************************************/
DPP_STATUS dpp_tm_cgavd_default_queue_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_DEFAULT_QUEUE_EN_T default_que_en = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, en, 0, 1);

    default_que_en.default_queue_en = en;
    rc  = dpp_reg_write(dev_id,
                        ETM_CGAVD_DEFAULT_QUEUE_ENr,
                        0,
                        0,
                        &default_que_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/**  读取默认队列使能
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_en   读取的值，0-不使能默认队列，1-使能默认队列，
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush     @date  2016/08/16
************************************************************/
DPP_STATUS dpp_tm_cgavd_default_queue_en_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_DEFAULT_QUEUE_EN_T default_que_en = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_en);

    rc  = dpp_reg_read(dev_id,
                       ETM_CGAVD_DEFAULT_QUEUE_ENr,
                       0,
                       0,
                       &default_que_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_en = default_que_en.default_queue_en;

    return DPP_OK;
}


/***********************************************************/
/**  配置默认队列起始末尾
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   def_start_que  起始默认队列block/byte单位
* @param   def_finish_que  结束默认队列block/byte单位
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush     @date  2016/08/16
************************************************************/
DPP_STATUS dpp_tm_cgavd_default_queue_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 def_start_queue, ZXIC_UINT32 def_finish_queue)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_DEFAULT_START_QUEUE_T def_start_que = {0};
    DPP_ETM_CGAVD_DEFAULT_FINISH_QUEUE_T def_finish_que = {0};
    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, def_start_queue, 0, 0x1fffffff);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, def_finish_queue, 0, 0x1fffffff);

    def_start_que.default_start_queue = def_start_queue;
    def_finish_que.default_finish_queue = def_finish_queue;


    rc  = dpp_reg_write(dev_id,
                        ETM_CGAVD_DEFAULT_START_QUEUEr,
                        0,
                        0,
                        &def_start_que);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    rc  = dpp_reg_write(dev_id,
                        ETM_CGAVD_DEFAULT_FINISH_QUEUEr,
                        0,
                        0,
                        &def_finish_que);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/**  读取默认队列起始末尾值
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_def_start_que   默认队列起始值block/byte单位
* @param   p_def_finish_que   默认队列结束值block/byte单位
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2016/08/16
************************************************************/
DPP_STATUS dpp_tm_cgavd_default_queue_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_def_start_queue, ZXIC_UINT32 *p_def_finish_queue)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_DEFAULT_START_QUEUE_T def_start_que = {0};
    DPP_ETM_CGAVD_DEFAULT_FINISH_QUEUE_T def_finish_que = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_def_finish_queue);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_def_start_queue);

    rc  = dpp_reg_read(dev_id,
                       ETM_CGAVD_DEFAULT_START_QUEUEr,
                       0,
                       0,
                       &def_start_que);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_CGAVD_DEFAULT_FINISH_QUEUEr,
                       0,
                       0,
                       &def_finish_que);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_def_start_queue = def_start_que.default_start_queue;
    *p_def_finish_queue = def_finish_que.default_finish_queue;

    return DPP_OK;
}


/***********************************************************/
/**  配置协议队列使能
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   en   配置的值，0-不使能默认队列，1-使能默认队列，
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2016/08/16
************************************************************/
DPP_STATUS dpp_tm_cgavd_protocol_queue_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_PROTOCOL_QUEUE_EN_T protocol_que_en = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, en, 0, 1);

    protocol_que_en.protocol_queue_en = en;
    rc  = dpp_reg_write(dev_id,
                        ETM_CGAVD_PROTOCOL_QUEUE_ENr,
                        0,
                        0,
                        &protocol_que_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/**  读取协议队列使能
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_en   读取的值，0-不使能通用门限，1-使能通用门限，
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2016/08/16
************************************************************/
DPP_STATUS dpp_tm_cgavd_protocol_queue_en_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_PROTOCOL_QUEUE_EN_T protocol_que_en = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_en);

    rc  = dpp_reg_read(dev_id,
                       ETM_CGAVD_PROTOCOL_QUEUE_ENr,
                       0,
                       0,
                       &protocol_que_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_en = protocol_que_en.protocol_queue_en;

    return DPP_OK;
}


/***********************************************************/
/**  配置协议队列起始末尾
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   protocol_start_que  起始协议队列block/byte单位
*@param   protocol_-finish_que 末尾协议队列block/byte单位
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2016/08/16
************************************************************/
DPP_STATUS dpp_tm_cgavd_protocol_queue_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 protocol_start_que, ZXIC_UINT32 protocol_finish_que)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_PROTOCOL_START_QUEUE_T pro_start_que = {0};
    DPP_ETM_CGAVD_PROTOCOL_FINISH_QUEUE_T pro_finish_que = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, protocol_start_que, 0, 0x1fffffff);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, protocol_finish_que, 0, 0x1fffffff);

    pro_start_que.protocol_start_queue = protocol_start_que;
    pro_finish_que.protocol_finish_queue = protocol_finish_que;


    rc  = dpp_reg_write(dev_id,
                        ETM_CGAVD_PROTOCOL_START_QUEUEr,
                        0,
                        0,
                        &pro_start_que);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    rc  = dpp_reg_write(dev_id,
                        ETM_CGAVD_PROTOCOL_FINISH_QUEUEr,
                        0,
                        0,
                        &pro_finish_que);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/**  读取协议队列起始末尾值
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_protocol_start_que   协议认队列起始值block/byte单位
* @param   p_protocol_finish_que   协议认队列末尾值block/byte单位
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush     @date  2016/08/16
************************************************************/
DPP_STATUS dpp_tm_cgavd_protocol_queue_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_protocol_start_que, ZXIC_UINT32 *p_protocol_finish_que)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_PROTOCOL_START_QUEUE_T pro_start_que = {0};
    DPP_ETM_CGAVD_PROTOCOL_FINISH_QUEUE_T pro_finish_que = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_protocol_finish_que);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_protocol_start_que);

    rc  = dpp_reg_read(dev_id,
                       ETM_CGAVD_PROTOCOL_START_QUEUEr,
                       0,
                       0,
                       &pro_start_que);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_CGAVD_PROTOCOL_FINISH_QUEUEr,
                       0,
                       0,
                       &pro_finish_que);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_protocol_start_que = pro_start_que.protocol_start_queue;
    *p_protocol_finish_que = pro_finish_que.protocol_finish_queue;

    return DPP_OK;
}

#endif
/***********************************************************/
/**  配置通用门限使能
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   en   配置的值，0-不使能通用门限，1-使能通用门限，
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_uniform_th_en_set(DPP_DEV_T *dev, ZXIC_UINT32 en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_UNIFORM_TD_TH_EN_T uniform_en = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), en, 0, 1);

    uniform_en.uniform_td_th_en = en;
    rc  = dpp_reg_write(dev,
                        ETM_CGAVD_UNIFORM_TD_TH_ENr,
                        0,
                        0,
                        &uniform_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/**  读取通用门限使能
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_en   读取的值，0-不使能通用门限，1-使能通用门限，
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_uniform_th_en_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_UNIFORM_TD_TH_EN_T uniform_en = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_en);

    rc  = dpp_reg_read(dev,
                       ETM_CGAVD_UNIFORM_TD_TH_ENr,
                       0,
                       0,
                       &uniform_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *p_en = uniform_en.uniform_td_th_en;

    return DPP_OK;
}

/***********************************************************/
/**  配置通用门限
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   byte_block_uni_th   通用门限值block/byte单位
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/08/01
************************************************************/
DPP_STATUS dpp_tm_cgavd_uniform_byte_block_th_set(DPP_DEV_T *dev, ZXIC_UINT32 byte_block_uni_th)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_UNIFORM_TD_TH_T uniform_block_th = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), byte_block_uni_th, 0, 0x1fffffff);

    uniform_block_th.uniform_td_th = byte_block_uni_th;
    rc  = dpp_reg_write(dev,
                        ETM_CGAVD_UNIFORM_TD_THr,
                        0,
                        0,
                        &uniform_block_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/**  配置通用门限
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   th   通用门限值
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_uniform_th_set(DPP_DEV_T *dev, ZXIC_UINT32 uni_th)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 blk_size = 0;
    ZXIC_UINT32 blk_th = 0;
    ZXIC_UINT32 cgavd_cfg_mode = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW(DEV_ID(dev), uni_th, DPP_TM_CGAVD_KILO_UL);
    if ((uni_th * DPP_TM_CGAVD_KILO_UL) > 0x1fffffff)
    {
        uni_th = 0x1fffffff;
    }
    else
    {
        uni_th = (uni_th * DPP_TM_CGAVD_KILO_UL);
    }

    rc = dpp_tm_cgavd_cfg_mode_get(dev, &cgavd_cfg_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_cgavd_cfg_mode_get");

    if (cgavd_cfg_mode == DPP_TM_CGAVD_BLOCK_MODE)
    {
        rc  = dpp_tm_cfgmt_blk_size_get(dev, &blk_size);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_cfgmt_blk_size_get");

        if (blk_size != 0)
        {
            blk_th = (uni_th / blk_size);
            blk_th = (uni_th % blk_size == 0) ? (blk_th ) : ((blk_th) + 1);
        }

        rc = dpp_tm_cgavd_uniform_byte_block_th_set(dev, blk_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_cgavd_uniform_byte_block_th_set");
    }
    else if (cgavd_cfg_mode == DPP_TM_CGAVD_ZXIC_UINT8_MODE)
    {
        rc = dpp_tm_cgavd_uniform_byte_block_th_set(dev, uni_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_cgavd_uniform_byte_block_th_set");
    }
    else
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "dpp_tm_cgavd_uniform_th_set err!!\n");
        return DPP_ERR;
    }

    return DPP_OK;
}


/***********************************************************/
/**  读取通用门限
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_byte_block_uni_th   通用门限值block/byte单位
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/08/01
************************************************************/
DPP_STATUS dpp_tm_cgavd_uniform_byte_block_th_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_byte_block_uni_th)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_UNIFORM_TD_TH_T uniform_block_th = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_byte_block_uni_th);

    rc  = dpp_reg_read(dev,
                       ETM_CGAVD_UNIFORM_TD_THr,
                       0,
                       0,
                       &uniform_block_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *p_byte_block_uni_th = uniform_block_th.uniform_td_th;

    return DPP_OK;
}

/***********************************************************/
/**  读取通用门限
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_uni_th   通用门限值kbyte单位
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/08/01
************************************************************/
DPP_STATUS dpp_tm_cgavd_uniform_th_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_uni_th)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 blk_size = 0;
    ZXIC_UINT32 cgavd_cfg_mode = 0;
    ZXIC_UINT32 block_byte_th = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_uni_th);
    

    rc = dpp_tm_cgavd_uniform_byte_block_th_get(dev, &block_byte_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_cgavd_uniform_byte_block_th_get");

    rc = dpp_tm_cgavd_cfg_mode_get(dev, &cgavd_cfg_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_cgavd_cfg_mode_get");

    if (cgavd_cfg_mode == DPP_TM_CGAVD_BLOCK_MODE)
    {
        rc  = dpp_tm_cfgmt_blk_size_get(dev, &blk_size);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_cfgmt_blk_size_get");

        *p_uni_th = (block_byte_th * blk_size) / DPP_TM_CGAVD_KILO_UL;

    }
    else if (cgavd_cfg_mode == DPP_TM_CGAVD_ZXIC_UINT8_MODE)
    {
        *p_uni_th = (block_byte_th / DPP_TM_CGAVD_KILO_UL);
    }
    else
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "dpp_tm_cgavd_uniform_th_get err!!\n");
        return DPP_ERR;
    }

    return DPP_OK;
}

/***********************************************************/
/**  配置流队列所属优先级
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   q_id   队列号
* @param   pri   配置的优先级，0~4
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_q_pri_set(DPP_DEV_T *dev,
                                  ZXIC_UINT32 q_id,
                                  ZXIC_UINT32 pri)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_Q_PRI_T q_pri = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), pri, 0, 4);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), q_id, 0, DPP_ETM_Q_NUM - 1);

    q_pri.qpri_flow_cfg_din = pri;
    rc  = dpp_reg_write(dev,
                        ETM_CGAVD_Q_PRIr,
                        0,
                        q_id,
                        &q_pri);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/** 配置TM模式下流队列挂接的端口号；SA模式下流队列映射的目的芯片ID
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   q_id   队列号
* @param   pp_id   配置的端口号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/18
************************************************************/
DPP_STATUS dpp_tm_cgavd_q_map_pp_set(DPP_DEV_T *dev,
                                     ZXIC_UINT32 q_id,
                                     ZXIC_UINT32 pp_id)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_PP_NUM_T pp_num = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), q_id, 0, DPP_ETM_Q_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), pp_id, 0, DPP_TM_PP_NUM - 1);

    pp_num.pp_num = pp_id;
    rc  = dpp_reg_write(dev,
                        ETM_CGAVD_PP_NUMr,
                        0,
                        q_id,
                        &pp_num);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 读取TM模式下流队列挂接的端口号；SA模式下流队列映射的目的芯片ID
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   q_id   队列号
* @param   p_pp_id   读取的端口号
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/18
************************************************************/
DPP_STATUS dpp_tm_cgavd_q_map_pp_get(DPP_DEV_T *dev,
                                     ZXIC_UINT32 q_id,
                                     ZXIC_UINT32 *p_pp_id)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_PP_NUM_T pp_num = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_pp_id);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), q_id, 0, DPP_ETM_Q_NUM - 1);

    *p_pp_id = 0xffffffff;
    rc  = dpp_reg_read(dev,
                       ETM_CGAVD_PP_NUMr,
                       0,
                       q_id,
                       &pp_num);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *p_pp_id = pp_num.pp_num;

    return DPP_OK;
}

/***********************************************************/
/** 配置TM模式tc到flow的映射
* @param   dev_id 设备编号
* @param   tc_id   itmd tc优先级（0~7）
* @param   flow_id 映射的flowid号 （0~4095）
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  sun      @date  2023/07/04
************************************************************/
DPP_STATUS dpp_tm_tc_map_flow_set(DPP_DEV_T *dev,
                                  ZXIC_UINT32 tc_id,
                                  ZXIC_UINT32 flow_id)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_CFG_TC_FLOWID_DAT_T cfg_tc_flow = {0}; 

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), tc_id, 0, DPP_TM_TC_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), flow_id, 0, DPP_ETM_Q_NUM - 1);

    cfg_tc_flow.cfg_tc_flowid_dat = flow_id;
    rc  = dpp_reg_write(dev,
                        ETM_CGAVD_CFG_TC_FLOWID_DATr,
                        0,
                        tc_id,
                        &cfg_tc_flow);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 读取配置TM模式tc到flow的映射
* @param   dev_id 设备编号
* @param   tc_id   itmd tc优先级（0~7）
* @param   flow_id 读取映射的flowid号 （0~4095）
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  sun      @date  2023/07/04
************************************************************/
DPP_STATUS dpp_tm_tc_map_flow_get(DPP_DEV_T *dev,
                                  ZXIC_UINT32 tc_id,
                                  ZXIC_UINT32 *flow_id)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_CFG_TC_FLOWID_DAT_T cfg_tc_flow = {0}; 

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), flow_id);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), tc_id, 0, DPP_TM_TC_NUM - 1);

    *flow_id = 0xffffffff;
    rc  = dpp_reg_read(dev,
                       ETM_CGAVD_CFG_TC_FLOWID_DATr,
                       0,
                       tc_id,
                       &cfg_tc_flow);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *flow_id = cfg_tc_flow.cfg_tc_flowid_dat;

    return DPP_OK;
}

#if 0
/***********************************************************/
/** 系统级缓存使用上下限阈值配置
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   th_h: 系统级缓存使用上限阈值
* @param   th_l: 系统级缓存使用下限阈值
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/03
************************************************************/
DPP_STATUS dpp_tm_sys_window_th_set(ZXIC_UINT32 dev_id,
                                    ZXIC_UINT32 th_h,
                                    ZXIC_UINT32 th_l)
{
    DPP_STATUS  rc = DPP_OK;

    DPP_ETM_CGAVD_SYS_WINDOW_TH_H_T sys_window_th_h = {0};
    DPP_ETM_CGAVD_SYS_WINDOW_TH_L_T sys_window_th_l = {0};
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, th_h, 0, 0x1fffffff);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, th_l, 0, 0x1fffffff);

    if (th_l > th_h)
    {
        ZXIC_COMM_PRINT("input error th_l > th_h");
        return DPP_ERR;
    }

    sys_window_th_h.sys_window_th_h = th_h;
    sys_window_th_l.sys_window_th_l = th_l;

    rc  = dpp_reg_write(dev_id,
                        ETM_CGAVD_SYS_WINDOW_TH_Hr,
                        0,
                        0,
                        &sys_window_th_h);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    rc  = dpp_reg_write(dev_id,
                        ETM_CGAVD_SYS_WINDOW_TH_Lr,
                        0,
                        0,
                        &sys_window_th_l);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

#endif
/***********************************************************/
/** 配置QMU查询队列Qos开关
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   q_id:  队列号
* @param   qos_sign: qos开关 0:关闭  1:开启
* @param
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/03
************************************************************/
DPP_STATUS dpp_tm_qmu_qos_sign_set(DPP_DEV_T *dev,
                                   ZXIC_UINT32 q_id,
                                   ZXIC_UINT32 qos_sign)
{
    DPP_STATUS  rc = DPP_OK;

    DPP_ETM_CGAVD_QOS_SIGN_T qmu_qos_sign = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), qos_sign, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), q_id, 0, DPP_ETM_Q_NUM - 1);

    qmu_qos_sign.qos_sign_flow_cfg_din = qos_sign;

    rc  = dpp_reg_write(dev,
                        ETM_CGAVD_QOS_SIGNr,
                        0,
                        q_id,
                        &qmu_qos_sign);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

#if 0
/***********************************************************/
/** 配置cgavd强制反压
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   cgavd_fc: 0:不强制反压    1:强制反压
* @param
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/07/03
************************************************************/
DPP_STATUS dpp_tm_cgavd_cfg_fc_set(ZXIC_UINT32 dev_id,
                                   ZXIC_UINT32 cgavd_fc)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_CGAVD_CFG_FC_T cgavd_cfg_fc_t = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, cgavd_fc, 0, 1);

    cgavd_cfg_fc_t.cgavd_cfg_fc = cgavd_fc;

    rc  = dpp_reg_write(dev_id,
                        ETM_CGAVD_CGAVD_CFG_FCr,
                        0,
                        0,
                        &cgavd_cfg_fc_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/** 获取cgavd强制反压状态
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   cgavd_fc: 0:不强制反压    1:强制反压
* @param
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/07/03
************************************************************/
DPP_STATUS dpp_tm_cgavd_cfg_fc_get(ZXIC_UINT32 dev_id,
                                   ZXIC_UINT32 *cgavd_fc)

{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_CGAVD_CFG_FC_T cgavd_cfg_fc_t = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, cgavd_fc);

    rc  = dpp_reg_read(dev_id,
                       ETM_CGAVD_CGAVD_CFG_FCr,
                       0,
                       0,
                       &cgavd_cfg_fc_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    *cgavd_fc = cgavd_cfg_fc_t.cgavd_cfg_fc;

    return DPP_OK;
}

/***********************************************************/
/** 配置cgavd强制不反压
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   cgavd_no_fc: 0:不强制     1:强制不反压
* @param
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/07/03
************************************************************/
DPP_STATUS dpp_tm_cgavd_cfg_no_fc_set(ZXIC_UINT32 dev_id,
                                      ZXIC_UINT32 cgavd_no_fc)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_CGAVD_CFG_NO_FC_T cgavd_cfg_no_fc_t = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, cgavd_no_fc, 0, 1);

    cgavd_cfg_no_fc_t.cgavd_cfg_no_fc = cgavd_no_fc;

    rc  = dpp_reg_write(dev_id,
                        ETM_CGAVD_CGAVD_CFG_NO_FCr,
                        0,
                        0,
                        &cgavd_cfg_no_fc_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/** 获取cgavd强制不反压状态
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   cgavd_no_fc: 0:不强制 1:强制不反压
* @param
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/07/03
************************************************************/
DPP_STATUS dpp_tm_cgavd_cfg_no_fc_get(ZXIC_UINT32 dev_id,
                                      ZXIC_UINT32 *cgavd_no_fc)

{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_CGAVD_CFG_NO_FC_T cgavd_cfg_no_fc_t = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, cgavd_no_fc);

    rc  = dpp_reg_read(dev_id,
                       ETM_CGAVD_CGAVD_CFG_NO_FCr,
                       0,
                       0,
                       &cgavd_cfg_no_fc_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    *cgavd_no_fc = cgavd_cfg_no_fc_t.cgavd_cfg_no_fc;

    return DPP_OK;
}


/***********************************************************/
/** 配置cgavd平均队列深度归零
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   en: 0:关闭     1:使能
* @param
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/08/05
************************************************************/
DPP_STATUS dpp_tm_cgavd_avg_qlen_return_zero_en_set(ZXIC_UINT32 dev_id,
                                                ZXIC_UINT32 en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_AVG_QLEN_RETURN_ZERO_EN_T return_zero_en = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, en, 0, 1);

    return_zero_en.avg_qlen_return_zero_en = en;

    rc  = dpp_reg_write(dev_id,
                        ETM_CGAVD_AVG_QLEN_RETURN_ZERO_ENr,
                        0,
                        0,
                        &return_zero_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

#endif
#endif


#if ZXIC_REAL("TM_QMU")
#if 0
/***********************************************************/
/** QMU MMU 配置清除
* @param   dev_id
* @param   tm_type
*
* @return
* @remark  无
* @see
* @author  XXX      @date  2020/04/13
************************************************************/
DPP_STATUS dpp_tm_qmu_mmu_cfg_clr(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CFGMT_CFGMT_DDR_ATTACH_T attach = {0};

    ZXIC_UINT32 bdep[64] = {0};
    ZXIC_UINT32 bhead[64] = {0};
    ZXIC_UINT32 btail[64] = {0};
    ZXIC_UINT32 ddr_in_mmu[8] = {0};
    ZXIC_UINT32 ddr_in_qmu[10] = {0};
    ZXIC_UINT32 bank_to_mmu[64] = {0};
    ZXIC_UINT32 bank_to_qmu[80] = {0};
    ZXIC_UINT32 active[16] = {0};
    ZXIC_UINT32 random_grp[64] = {0};
    ZXIC_UINT32 random_ddr0[64] = {0};
    ZXIC_UINT32 random_ddr1[64] = {0};
    ZXIC_UINT32 random_ddr2[64] = {0};
    ZXIC_UINT32 random_ddr3[64] = {0};
    ZXIC_UINT32 random_ddr4[64] = {0};
    ZXIC_UINT32 random_ddr5[64] = {0};
    ZXIC_UINT32 random_ddr6[64] = {0};
    ZXIC_UINT32 random_ddr7[64] = {0};
    //ZXIC_UINT32 mmu_addr[128] = {0};

    ZXIC_UINT32 i = 0;

    ZXIC_UINT32 reg_attach = 0;
    ZXIC_UINT32 reg_ddr_in_mmu = 0;
    ZXIC_UINT32 reg_ddr_in_qmu = 0;
    ZXIC_UINT32 reg_bank_to_mmu = 0;
    ZXIC_UINT32 reg_bank_to_qmu = 0;
    ZXIC_UINT32 reg_bdep = 0;
    ZXIC_UINT32 reg_bhead = 0;
    ZXIC_UINT32 reg_btail = 0;
    ZXIC_UINT32 reg_active = 0;
    ZXIC_UINT32 reg_random_grp = 0;
    ZXIC_UINT32 reg_random_ddr0 = 0;
    ZXIC_UINT32 reg_random_ddr1 = 0;
    ZXIC_UINT32 reg_random_ddr2 = 0;
    ZXIC_UINT32 reg_random_ddr3 = 0;
    ZXIC_UINT32 reg_random_ddr4 = 0;
    ZXIC_UINT32 reg_random_ddr5 = 0;
    ZXIC_UINT32 reg_random_ddr6 = 0;
    ZXIC_UINT32 reg_random_ddr7 = 0;
    //ZXIC_UINT32 reg_mmu_addr = 0;

    reg_attach = ETM_CFGMT_CFGMT_DDR_ATTACHr;
    reg_ddr_in_mmu = ETM_QMU_CFGMT_DDR_IN_MMU_CFGr;
    reg_ddr_in_qmu = ETM_QMU_CFGMT_DDR_IN_QMU_CFGr;
    reg_bank_to_mmu = ETM_QMU_CFGMT_BANK_TO_MMU_CFGr;
    reg_bank_to_qmu = ETM_QMU_CFGMT_BANK_TO_QMU_CFGr;
    reg_bdep = ETM_QMU_QCFG_QLIST_BDEPr;
    reg_bhead = ETM_QMU_QCFG_QLIST_BHEADr;
    reg_btail = ETM_QMU_QCFG_QLIST_BTAILr;
    reg_active = ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFGr;
    reg_random_grp = ETM_QMU_QCFG_QLIST_GRPr;
    reg_random_ddr0 = ETM_QMU_QCFG_QLIST_GRP0_BANKr;
    reg_random_ddr1 = ETM_QMU_QCFG_QLIST_GRP1_BANKr;
    reg_random_ddr2 = ETM_QMU_QCFG_QLIST_GRP2_BANKr;
    reg_random_ddr3 = ETM_QMU_QCFG_QLIST_GRP3_BANKr;
    reg_random_ddr4 = ETM_QMU_QCFG_QLIST_GRP4_BANKr;
    reg_random_ddr5 = ETM_QMU_QCFG_QLIST_GRP5_BANKr;
    reg_random_ddr6 = ETM_QMU_QCFG_QLIST_GRP6_BANKr;
    reg_random_ddr7 = ETM_QMU_QCFG_QLIST_GRP7_BANKr;

    rc  = dpp_reg_write(dev_id, reg_attach, 0, 0, &attach);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    for (i = 0; i < 8; i++)
    {
        rc  = dpp_reg_write32_bymn(dev_id, reg_ddr_in_mmu, 0, i, ddr_in_mmu[i]);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write32_bymn");
    }

    for (i = 0; i < 10; i++)
    {
        rc  = dpp_reg_write32_bymn(dev_id, reg_ddr_in_qmu, 0, i, ddr_in_qmu[i]);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write32_bymn");
    }

    for (i = 0; i < 64; i++)
    {
        rc  = dpp_reg_write32_bymn(dev_id, reg_bank_to_mmu, 0, i, bank_to_mmu[i]);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write32_bymn");
    }

    for (i = 0; i < 80; i++)
    {
        rc  = dpp_reg_write32_bymn(dev_id, reg_bank_to_qmu, 0, i, bank_to_qmu[i]);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write32_bymn");
    }

    for (i = 0; i < 64; i++)
    {
        rc  = dpp_reg_write32_bymn(dev_id, reg_bhead, 0, i, bhead[i]);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write32_bymn");

        rc  = dpp_reg_write32_bymn(dev_id, reg_btail, 0, i, btail[i]);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write32_bymn");

        rc  = dpp_reg_write32_bymn(dev_id, reg_bdep, 0, i, bdep[i]);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write32_bymn");
    }

    for (i = 0; i < 16; i++)
    {
        rc  = dpp_reg_write32_bymn(dev_id, reg_active, 0, i, active[i]);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write32_bymn");
    }

    for (i = 0; i < 64; i++)
    {
        rc  = dpp_reg_write32_bymn(dev_id, reg_random_grp, 0, i, random_grp[i]);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write32_bymn");

        rc  = dpp_reg_write32_bymn(dev_id, reg_random_ddr0, 0, i, random_ddr0[i]);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write32_bymn");

        rc  = dpp_reg_write32_bymn(dev_id, reg_random_ddr1, 0, i, random_ddr1[i]);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write32_bymn");

        rc  = dpp_reg_write32_bymn(dev_id, reg_random_ddr2, 0, i, random_ddr2[i]);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write32_bymn");

        rc  = dpp_reg_write32_bymn(dev_id, reg_random_ddr3, 0, i, random_ddr3[i]);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write32_bymn");

        rc  = dpp_reg_write32_bymn(dev_id, reg_random_ddr4, 0, i, random_ddr4[i]);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write32_bymn");

        rc  = dpp_reg_write32_bymn(dev_id, reg_random_ddr5, 0, i, random_ddr5[i]);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write32_bymn");

        rc  = dpp_reg_write32_bymn(dev_id, reg_random_ddr6, 0, i, random_ddr6[i]);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write32_bymn");

        rc  = dpp_reg_write32_bymn(dev_id, reg_random_ddr7, 0, i, random_ddr7[i]);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write32_bymn");
    }

    /*for (i = 0; i < 128; i++)
    {
        rc  = dpp_reg_write32_bymn(dev_id, reg_mmu_addr + i, 0, 0, mmu_addr[i]);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write32_bymn");
    }*/

    return rc;
}

/***********************************************************/
/** 配置QMU队列授权价值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   credit_value   授权价值，默认值是533Byte
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_credit_value_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 credit_value)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_QCFG_QSCH_CREDIT_VALUE_T credit_val = {0};

    credit_val.qcfg_qsch_credit_value = credit_value;
    rc  = dpp_reg_write(dev_id,
                        ETM_QMU_QCFG_QSCH_CREDIT_VALUEr,
                        0,
                        0,
                        &credit_val);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

#endif
/***********************************************************/
/** 读取QMU队列授权价值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_credit_value   授权价值，默认值是533Byte
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy     @date  2016/04/14
************************************************************/
DPP_STATUS dpp_tm_qmu_credit_value_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_credit_value)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_QCFG_QSCH_CREDIT_VALUE_T credit_val = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_credit_value);

    *p_credit_value = 0;

    rc  = dpp_reg_read(dev,
                       ETM_QMU_QCFG_QSCH_CREDIT_VALUEr,
                       0,
                       0,
                       &credit_val);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *p_credit_value = credit_val.qcfg_qsch_credit_value;

    return DPP_OK;
}

#if 0
/***********************************************************/
/** 配置授权盈余初始化值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   crbal_initial_value   授权盈余初始化值
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_crbal_initial_value_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 crbal_initial_value)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_QCFG_QSCH_CRBAL_INIT_VALUE_T crbal_init_val = {0};

    crbal_init_val.qcfg_qsch_crbal_init_value = crbal_initial_value;
    rc  = dpp_reg_write(dev_id,
                        ETM_QMU_QCFG_QSCH_CRBAL_INIT_VALUEr,
                        0,
                        0,
                        &crbal_init_val);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}



/***********************************************************/
/** 配置CRS过滤使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   en   配置的值，0-不使能过滤，1-使能过滤
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_crs_filter_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_QCFG_QSCH_CRS_FILTER_T crs_filter_en = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, en, 0, 1);

    crs_filter_en.qcfg_qsch_crs_filter = en;
    rc  = dpp_reg_write(dev_id,
                        ETM_QMU_QCFG_QSCH_CRS_FILTERr,
                        0,
                        0,
                        &crs_filter_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/** 配置CRS发送强制使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   en   配置的值，0-不使能，1-使能
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_crs_force_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_QCFG_QSCH_CRS_FORCE_EN_T crs_force_en = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, en, 0, 1);

    crs_force_en.qcfg_qsch_crs_force_en = en;
    rc  = dpp_reg_write(dev_id,
                        ETM_QMU_QCFG_QSCH_CRS_FORCE_ENr,
                        0,
                        0,
                        &crs_force_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/** 配置CRS发送强制的队列
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   q_id   队列号
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_crs_force_q_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 q_id)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_QCFG_QSCH_CRS_FORCE_QNUM_T crs_force_q = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, q_id, 0, DPP_ETM_Q_NUM - 1);

    crs_force_q.qcfg_qsch_crs_force_qnum = q_id;
    rc  = dpp_reg_write(dev_id,
                        ETM_QMU_QCFG_QSCH_CRS_FORCE_QNUMr,
                        0,
                        0,
                        &crs_force_q);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}



/***********************************************************/
/** 配置CRS发送强置的状态
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   crs_state   CRS发送强置的状态
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/18
************************************************************/
DPP_STATUS dpp_tm_qmu_crs_force_state_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 crs_state)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_QCFG_QSCH_CRS_FORCE_CRS_T crs_force_crs = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, crs_state, 0, 1);

    crs_force_crs.qcfg_qsch_crs_force_crs = crs_state;
    rc = dpp_reg_write(dev_id,
                       ETM_QMU_QCFG_QSCH_CRS_FORCE_CRSr,
                       0,
                       0,
                       &crs_force_crs);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/** 配置特定队列发送特定CRS
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   qnum   队列号
*               qcfg_qsch_crs_force_crs:CRS状态(0：off；1：normal。)
                 qcfg_qsch_crs_force_en:CRS发送强置使能(0：不使能；1：使能。)
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_qnum_crs_force(ZXIC_UINT32 dev_id,
                                     ZXIC_UINT32 qnum,
                                     ZXIC_UINT32 qcfg_qsch_crs_force_crs,
                                     ZXIC_UINT32 qcfg_qsch_crs_force_en)

{
    DPP_STATUS  rc = DPP_OK;
    

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, qnum, 0, DPP_ETM_Q_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, qcfg_qsch_crs_force_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, qcfg_qsch_crs_force_crs, 0,  1);


    rc = dpp_tm_qmu_crs_force_q_set(dev_id, qnum);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_crs_force_q_set");

    rc = dpp_tm_qmu_crs_force_state_set(dev_id, qcfg_qsch_crs_force_crs);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_crs_force_state_set");

    rc = dpp_tm_qmu_crs_force_en_set(dev_id, qcfg_qsch_crs_force_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_crs_force_en_set");

    return DPP_OK;

}

/***********************************************************/
/** 配置QMU空闲链表:TM独享模式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   ddr_num   ddr组数，1-8组
* @param   bank_vld   bank有效信号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/06/07
************************************************************/
DPP_STATUS dpp_tm_qmu_qlist_set(ZXIC_UINT32 dev_id,
                                ZXIC_UINT32 ddr_num,
                                ZXIC_UINT32 bank_num_para,
                                ZXIC_UINT32 bank_vld,
                                ZXIC_UINT32 gene_para)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 k = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 reg_index = 0;
    ZXIC_UINT32 bdep_reg_index = 0;
    ZXIC_UINT32 bhead_reg_index = 0;
    ZXIC_UINT32 btail_reg_index = 0;
    ZXIC_UINT32 qmu_cfgmt_ddr_in_mmu_index = 0;
    ZXIC_UINT32 qmu_cfgmt_ddr_in_qmu_index = 0;
    ZXIC_UINT32 qmu_cfgmt_bank_to_mmu_index = 0;
    ZXIC_UINT32 qmu_cfgmt_bank_to_qmu_index = 0;
    ZXIC_UINT32 qmu_cfgmt_active_to_bank_index = 0;
    ZXIC_UINT32 qmu_qcfg_qlist_grp0_bank_index = 0;
    ZXIC_UINT32 qmu_qcfg_qlist_grp1_bank_index = 0;
    ZXIC_UINT32 qlist_grp0_bank_data[8] = {1, 2, 3, 4, 5, 6, 7, 0};
    DPP_ETM_QMU_QCFG_QLIST_BDEP_T qlist_bdep = {0};
    DPP_ETM_QMU_QCFG_QLIST_BHEAD_T qlist_bhead = {0};
    DPP_ETM_QMU_QCFG_QLIST_BTAIL_T qlist_btail = {0};
    DPP_ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFG_T active_to_bank_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_MMU_CFG_T ddr_in_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_QMU_CFG_T ddr_in_qmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_MMU_CFG_T bank_to_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_QMU_CFG_T bank_to_qmu_cfg = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP0_BANK_T qlist_grp0_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP1_BANK_T qlist_grp1_bank = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, ddr_num, 1, 8);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, bank_num_para, 1, 8);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, bank_vld, 0, 1);

    bdep_reg_index = ETM_QMU_QCFG_QLIST_BDEPr;
    bhead_reg_index = ETM_QMU_QCFG_QLIST_BHEADr;
    btail_reg_index = ETM_QMU_QCFG_QLIST_BTAILr;
    qmu_cfgmt_ddr_in_mmu_index = ETM_QMU_CFGMT_DDR_IN_MMU_CFGr;
    qmu_cfgmt_ddr_in_qmu_index = ETM_QMU_CFGMT_DDR_IN_QMU_CFGr;
    qmu_cfgmt_bank_to_mmu_index = ETM_QMU_CFGMT_BANK_TO_MMU_CFGr;
    qmu_cfgmt_bank_to_qmu_index = ETM_QMU_CFGMT_BANK_TO_QMU_CFGr;
    qmu_cfgmt_active_to_bank_index = ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFGr;
    qmu_qcfg_qlist_grp0_bank_index = ETM_QMU_QCFG_QLIST_GRP0_BANKr;
    qmu_qcfg_qlist_grp1_bank_index = ETM_QMU_QCFG_QLIST_GRP1_BANKr;

    /* cfgmt配置ddr_num组ddr */
    rc = dpp_tm_cfgmt_ddr_attach_set(dev_id, ddr_num);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_ddr_attach_set");

    /* ddr组映射:     qmu <--> mmu: ddr0~1 映射 0~1 */
    for (i = 0; i < ddr_num; i++)
    {
        /* qmu --> mmu */
        ddr_in_mmu_cfg.cfgmt_ddr_in_mmu_cfg = i;
        rc  = dpp_reg_write(dev_id,
                            qmu_cfgmt_ddr_in_mmu_index,
                            0,
                            i,
                            &ddr_in_mmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        /* mmu --> qmu */
        ddr_in_qmu_cfg.cfgmt_ddr_in_qmu_cfg = i;
        rc  = dpp_reg_write(dev_id,
                            qmu_cfgmt_ddr_in_qmu_index,
                            0,
                            i,
                            &ddr_in_qmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }



    /* qmu链表首尾指针及bank深度配置 */
    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++dpp_tm_qmu_qlist_set starting++++++\n");

    for (i = 0; i < ddr_num; i++)
    {
        for (j = 0; j < bank_num_para; j++)
        {
            reg_index = (i * 8 + j);

            ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++ reg_index=%d ++++++\n", reg_index);

            qlist_bdep.qcfg_qlist_bdep = gene_para;
            ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++ qlist_bdep.qcfg_qlist_bdep=0x%x ++++++\n", qlist_bdep.qcfg_qlist_bdep);
            rc  = dpp_reg_write(dev_id,
                                bdep_reg_index,
                                0,
                                reg_index,
                                &qlist_bdep);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            qlist_bhead.bank_vld = bank_vld;
            qlist_bhead.qcfg_qlist_bhead = (gene_para * (i * bank_num_para + j));
            ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++ qlist_bhead.bank_vld=0x%x ++++++\n", qlist_bhead.bank_vld);
            ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++ qlist_bhead.qcfg_qlist_bhead=0x%x ++++++\n", qlist_bhead.qcfg_qlist_bhead);
            rc  = dpp_reg_write(dev_id,
                                bhead_reg_index,
                                0,
                                reg_index,
                                &qlist_bhead);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id,  gene_para , ((i * bank_num_para + j) + 1) );
            ZXIC_COMM_CHECK_DEV_INDEX_SUB_OVERFLOW_NO_ASSERT(dev_id,  (gene_para * ((i * bank_num_para + j) + 1)) , 1 );
            qlist_btail.qcfg_qlist_btail = ((gene_para * ((i * bank_num_para + j) + 1)) - 1);
            ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++ qlist_btail.qcfg_qlist_btail=0x%x ++++++\n", qlist_btail.qcfg_qlist_btail);
            rc  = dpp_reg_write(dev_id,
                                btail_reg_index,
                                0,
                                reg_index,
                                &qlist_btail);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }

        ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++ j=%d ++++++\n", j);

        if (j != 8 )
        {
            for (k = bank_num_para; k < 8; k++)
            {
                reg_index = (i * 8 + k);
                ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++ reg_index=%d ++++++\n", reg_index);
                qlist_bhead.bank_vld = 0;
                qlist_bhead.qcfg_qlist_bhead = 0;
                ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++ qlist_bhead.bank_vld=0x%x ++++++\n", qlist_bhead.bank_vld);
                ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++ qlist_bhead.qcfg_qlist_bhead=0x%x ++++++\n", qlist_bhead.qcfg_qlist_bhead);
                rc  = dpp_reg_write(dev_id,
                                    bhead_reg_index,
                                    0,
                                    reg_index,
                                    &qlist_bhead);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
            }
        }
    }


    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++ i=%d ++++++\n", i);

    if (i != 8 )
    {
        for (k = ddr_num; k < 8; k++)
        {
            for (j = 0; j < 8; j++)
            {
                reg_index = (k * 8 + j);
                ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++ reg_index=%d ++++++\n", reg_index);
                qlist_bhead.bank_vld = 0;
                qlist_bhead.qcfg_qlist_bhead = 0;
                ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++ qlist_bhead.bank_vld=0x%x ++++++\n", qlist_bhead.bank_vld);
                ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++ qlist_bhead.qcfg_qlist_bhead=0x%x ++++++\n", qlist_bhead.qcfg_qlist_bhead);
                rc  = dpp_reg_write(dev_id,
                                    bhead_reg_index,
                                    0,
                                    reg_index,
                                    &qlist_bhead);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
            }
        }
    }

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++dpp_tm_qmu_qlist_set end++++++\n");

    /*** bank号映射：
      qmu-->mmu: bank0~7 映射 bank0~7
      mmu-->qmu: bank0~7 映射 bank0~7 *****/
    for (j = 0; j < ddr_num; j++)
    {
        for (i = 0; i < bank_num_para; i++)
        {
            /* qmu-->mmu: bank0~7 映射 bank0~7 */
            k = j * 8 + i;
            bank_to_mmu_cfg.cfgmt_bank_in_mmu_cfg = i;
            rc  = dpp_reg_write(dev_id,
                                qmu_cfgmt_bank_to_mmu_index,
                                0,
                                k,
                                &bank_to_mmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            /* mmu-->qmu: bank0~7 映射 bank0~7 */
            bank_to_qmu_cfg.cfgmt_bank_in_qmu_cfg = i;

            rc  = dpp_reg_write(dev_id,
                                qmu_cfgmt_bank_to_qmu_index,
                                0,
                                k,
                                &bank_to_qmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    /* random映射ram */
    for (j = 0; j < ddr_num; j++)
    {
        for (i = 0; i < bank_num_para; i++)
        {
            k = j * bank_num_para + i;

            active_to_bank_cfg.cfgmt_active_to_bank_cfg = i;

            rc  = dpp_reg_write(dev_id,
                                qmu_cfgmt_active_to_bank_index,
                                0,
                                k,
                                &active_to_bank_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }


    /* 随机表配置，决定每组bank的轮询顺序 */
    for (j = 0; j < ddr_num; j++)
    {
        for (i = 0; i < bank_num_para; i++)
        {
            k = j * 8 + i;

            qlist_grp0_bank.qcfg_qlist_grp0_bank_wr = qlist_grp0_bank_data[i];

            rc  = dpp_reg_write(dev_id,
                                qmu_qcfg_qlist_grp0_bank_index,
                                0,
                                k,
                                &qlist_grp0_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        }
    }

    for (j = 0; j < ddr_num; j++)
    {
        for (i = 0; i < bank_num_para; i++)
        {
            k = j * 8 + i;

            qlist_grp1_bank.qcfg_qlist_grp1_bank_wr = qlist_grp0_bank_data[i];

            rc  = dpp_reg_write(dev_id,
                                qmu_qcfg_qlist_grp1_bank_index,
                                0,
                                k,
                                &qlist_grp1_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        }
    }

    /* qmu配置完成 */
    rc = dpp_tm_qmu_cfg_done_set(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_cfg_done_set");

    return DPP_OK;
}


/***********************************************************/
/** QMU DDR随机模式时，DDR随机组配置
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   ddr_num   ddr组数，1-6组
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_ddr_rand_grp_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 ddr_num)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 bank_no[6][16] = {{0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7},
        {0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15},
        {0, 8, 16, 1, 9, 17, 2, 10, 18, 3, 11, 19, 4, 12, 20, 5},
        {0, 8, 16, 24, 1, 9, 17, 25, 2, 10, 18, 26, 3, 11, 19, 27},
        {0, 8, 16, 24, 32, 1, 9, 17, 25, 33, 2, 10, 18, 26, 34, 3},
        {0, 8, 16, 24, 32, 40, 1, 9, 17, 25, 33, 41, 2, 10, 18, 26}
    };

    DPP_ETM_QMU_QCFG_QLIST_GRP_T grp = {0};
    DPP_ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFG_T bank = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, ddr_num, 1, 6);

    for (i = 0; i < 64; i++)
    {
        grp.qcfg_qlist_grp_wr = i % ddr_num;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_GRPr,
                            0,
                            i,
                            &grp);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    for (i = 0; i < 16; i++)
    {
        bank.cfgmt_active_to_bank_cfg = bank_no[ddr_num - 1][i];
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFGr,
                            0,
                            i,
                            &bank);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    return DPP_OK;
}


/***********************************************************/
/** 配置QMU DDR BANK随机模式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   ddr_random   模式:0-轮询模式；1-随机模式
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_ddr_random_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 ddr_random)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_QCFG_QLIST_DDR_RANDOM_T ddr_rand = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, ddr_random, 0, 1);

    ddr_rand.qcfg_qlist_ddr_random = ddr_random;
    rc  = dpp_reg_write(dev_id,
                        ETM_QMU_QCFG_QLIST_DDR_RANDOMr,
                        0,
                        0,
                        &ddr_rand);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/** 配置QMU DDR BANK随机模式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_ddr_random   模式:0-轮询模式；1-随机模式
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_ddr_random_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_ddr_random)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_QCFG_QLIST_DDR_RANDOM_T ddr_rand = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_ddr_random);

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_QCFG_QLIST_DDR_RANDOMr,
                       0,
                       0,
                       &ddr_rand);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    *p_ddr_random = ddr_rand.qcfg_qlist_ddr_random;

    return DPP_OK;
}


/***********************************************************/
/** QMU配置完成寄存器，在QMU链表和DDR随机模式寄存器写入后，将此寄存器写1，完成QMU配置
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/05/08
************************************************************/
DPP_STATUS dpp_tm_qmu_cfg_done_set(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_QCFG_QLIST_CFG_DONE_T cfg_done = {0};

    cfg_done.qcfg_qlist_cfg_done = 1;
    rc  = dpp_reg_write(dev_id,
                        ETM_QMU_QCFG_QLIST_CFG_DONEr,
                        0,
                        0,
                        &cfg_done);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 配置CRS的e桶产生的crbal门限值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   index   crs组数：0~15
* @param   crs_th   CRS产生的crbal门限值
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  xuhb      @date  2021/02/14
************************************************************/
DPP_STATUS dpp_tm_qmu_crs_eir_th_set(ZXIC_UINT32 dev_id,
                                 ZXIC_UINT32 index,
                                 ZXIC_UINT32 crs_th)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_QCFG_QSCH_CRS_EIR_TH_T crs_eir_th = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, index, 0, 0xf);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, crs_th, 0, 0x3ffff);

    crs_eir_th.qcfg_qsch_crs_eir_th = crs_th;

    rc = dpp_reg_write(dev_id,
                       ETM_QMU_QCFG_QSCH_CRS_EIR_THr,
                       0,
                       index,
                       &crs_eir_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/** 配置CRS产生的crbal门限值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   index   crs组数：0~15
* @param   crs_th   CRS产生的crbal门限值
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/11
************************************************************/
DPP_STATUS dpp_tm_qmu_crs_th_set(ZXIC_UINT32 dev_id,
                                 ZXIC_UINT32 index,
                                 ZXIC_UINT32 crs_th)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_QCFG_QSCH_CRS_TH1_T crs_th1 = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, index, 0, 0xf);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, crs_th, 0, 0xffffffff);

    crs_th1.qcfg_qsch_crs_th1 = crs_th;

    rc = dpp_reg_write(dev_id,
                       ETM_QMU_QCFG_QSCH_CRS_TH1r,
                       0,
                       index,
                       &crs_th1);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 配置CRS产生的空队列确保门限值
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   que_type   队列类型编号(0~15)
* @param   empty_que_ack_th   空队列确保授权门限
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/09
************************************************************/
DPP_STATUS dpp_tm_qmu_crs_th2_set(ZXIC_UINT32 dev_id,
                                  ZXIC_UINT32 que_type,
                                  ZXIC_UINT32 empty_que_ack_th)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_QCFG_QSCH_CRS_TH2_T crs_th2 = {0};
    ZXIC_UINT32 rem_bit_sum = 4;
    ZXIC_UINT32 rem = 0;
    ZXIC_UINT32 exp = 0;

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, que_type, 0, 15);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, empty_que_ack_th, 0, 0x78000);

    rc = dpp_tm_rem_and_exp_translate(empty_que_ack_th,
                                      rem_bit_sum,
                                      &rem,
                                      &exp);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    crs_th2.qcfg_qsch_crs_th2 = (rem << 4) + (exp & 0xf);

    rc = dpp_reg_write(dev_id,
                       ETM_QMU_QCFG_QSCH_CRS_TH2r,
                       0,
                       que_type,
                       &crs_th2);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    return DPP_OK;
}

/***********************************************************/
/** 配置CRS发送的速率
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   sent_cyc   CRS发送的间隔(单位:时钟周期)
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  szq      @date  2015/03/25
************************************************************/
DPP_STATUS dpp_tm_qmu_crs_sent_rate_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 sent_cyc)
{
    DPP_STATUS  rc = DPP_OK;

    DPP_ETM_QMU_CFGMT_CRS_INTERVAL_T crs_interval = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, sent_cyc, 1, 0xffffffff);

    crs_interval.cfgmt_crs_interval = sent_cyc;


    rc = dpp_reg_write(dev_id,
                       ETM_QMU_CFGMT_CRS_INTERVALr,
                       0,
                       0,
                       &crs_interval);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    return DPP_OK;
}

/***********************************************************/
/** 获取CRS发送的速率
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_sent_cyc   CRS发送的间隔(单位:时钟周期)
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  szq      @date  2015/03/25
************************************************************/
DPP_STATUS dpp_tm_qmu_crs_sent_rate_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_sent_cyc)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_CFGMT_CRS_INTERVAL_T crs_interval = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_sent_cyc);


    *p_sent_cyc = 0;

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_CFGMT_CRS_INTERVALr,
                       0,
                       0,
                       &crs_interval);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_sent_cyc = crs_interval.cfgmt_crs_interval;

    return DPP_OK;
}

/***********************************************************/
/** 配置QMU端口间交织模式
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   pkt_blk_mode   交织模式: 1-按包交织; 0-按block交织  SA模式只能配置为1
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  szq      @date  2015/03/25
************************************************************/
DPP_STATUS dpp_tm_qmu_pkt_blk_mode_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 pkt_blk_mode)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_QCFG_CSW_PKT_BLK_MODE_T csw_pkt_blk_mode = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, pkt_blk_mode, 0, 1);

    csw_pkt_blk_mode.qcfg_csw_pkt_blk_mode = pkt_blk_mode;


    rc = dpp_reg_write(dev_id,
                       ETM_QMU_QCFG_CSW_PKT_BLK_MODEr,
                       0,
                       0,
                       &csw_pkt_blk_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    return DPP_OK;
}

/***********************************************************/
/** 获取QMU端口间交织模式
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_pkt_blk_mode   交织模式: 0-按包交织 ; 1-按block交织SA模式只能配置为1
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  szq      @date  2015/03/25
************************************************************/
DPP_STATUS dpp_tm_qmu_pkt_blk_mode_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_pkt_blk_mode)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_QCFG_CSW_PKT_BLK_MODE_T csw_pkt_blk_mode = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_pkt_blk_mode);

    *p_pkt_blk_mode = 0;

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_QCFG_CSW_PKT_BLK_MODEr,
                       0,
                       0,
                       &csw_pkt_blk_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_pkt_blk_mode = csw_pkt_blk_mode.qcfg_csw_pkt_blk_mode;

    return DPP_OK;
}

/***********************************************************/
/** 配置SA模式下各个版本的授权价值
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   sa_ver_id   版本号(0~7)
* @param   sa_credit_value   授权价值
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  szq      @date  2015/03/25
************************************************************/
DPP_STATUS dpp_tm_qmu_sa_credit_value_set(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 sa_ver_id,
                                          ZXIC_UINT32 sa_credit_value)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_CFG_QSCH_SA_CREDIT_VALUE_0_T qmu_sa_credit_value = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, sa_ver_id, 0, 7);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, sa_credit_value, 0, 0x3ffff);

    qmu_sa_credit_value.cfg_qsch_sa_credit_value_0 = sa_credit_value;
    rc  = dpp_reg_write(dev_id,
                        ETM_QMU_CFG_QSCH_SA_CREDIT_VALUE_0r + sa_ver_id,
                        0,
                        0,
                        &qmu_sa_credit_value);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 获取SA模式下各个版本的授权价值
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   sa_ver_id   版本号(0~7)
* @param   p_sa_credit_value   授权价值
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  szq      @date  2015/03/25
************************************************************/
DPP_STATUS dpp_tm_qmu_sa_credit_value_get(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 sa_ver_id,
                                          ZXIC_UINT32 *p_sa_credit_value)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_CFG_QSCH_SA_CREDIT_VALUE_0_T qmu_sa_credit_value = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, sa_ver_id, 0, 7);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_sa_credit_value);

    *p_sa_credit_value = 0;

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_CFG_QSCH_SA_CREDIT_VALUE_0r + sa_ver_id,
                       0,
                       0,
                       &qmu_sa_credit_value);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_sa_credit_value = qmu_sa_credit_value.cfg_qsch_sa_credit_value_0;

    return DPP_OK;
}

/***********************************************************/
/** 配置多播授权令牌添加个数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   token_add_num   令牌添加时，每次增加的令牌数目，取值范围为1~255，默认为1；禁止配置为0，配置为0时，将不会产生授权。
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/09
************************************************************/
DPP_STATUS dpp_tm_qmu_mul_token_gen_num_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 token_add_num)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_CFG_QSCH_MUL_TOKEN_GEN_NUM_T token_gen_num = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, token_add_num, 1, 255);

    token_gen_num.cfg_qsch_mul_token_gen_num = token_add_num;

    rc = dpp_reg_write(dev_id,
                       ETM_QMU_CFG_QSCH_MUL_TOKEN_GEN_NUMr,
                       0,
                       0,
                       &token_gen_num);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    return DPP_OK;
}

/***********************************************************/
/** 配置多播授权整形桶参数和使能参数
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   q3_lb_control_en   3号队列整形功能开启使能。0：关闭；1：开启。
* @param   q012_lb_control_en   0～2号队列整形功能开启使能。0：关闭；1：开启。
* @param   q3_lb_max_cnt   3号队列整形桶桶深。
* @param   q012_lb_max_cnt   0～2号队列整形桶桶深。
* @param   q3_lb_add_rate   3号队列令牌添加速率，时钟周期为单位。不可配置为0，配置为0整形使能时，不能产生队列3授权调度信号。
* @param   q012_lb_add_rate   0～2号队列令牌添加速率，以时钟周期单位。不可配置为0，配置为0并整形使能时，不能产生队列0、1、2授权调度信号。
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/09
************************************************************/
DPP_STATUS dpp_tm_qmu_mul_ack_lb_set(ZXIC_UINT32 dev_id,
                                     ZXIC_UINT32 q3_lb_control_en,
                                     ZXIC_UINT32 q012_lb_control_en,
                                     ZXIC_UINT32 q3_lb_max_cnt,
                                     ZXIC_UINT32 q012_lb_max_cnt,
                                     ZXIC_UINT32 q3_lb_add_rate,
                                     ZXIC_UINT32 q012_lb_add_rate)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 q3_crdt_lb_ctrl_en_reg_index = 0;
    ZXIC_UINT32 q012_crdt_lb_ctrl_en_reg_index = 0;
    ZXIC_UINT32 q3_crdt_lb_max_cnt_reg_index = 0;
    ZXIC_UINT32 q012_crdt_lb_max_cnt_reg_index = 0;
    ZXIC_UINT32 q3_crdt_lb_add_rate_reg_index = 0;
    ZXIC_UINT32 q012_crdt_lb_add_rate_reg_index = 0;
    DPP_ETM_QMU_CFG_QSCH_Q3_CREDIT_LB_CONTROL_EN_T  q3_en = {0};
    DPP_ETM_QMU_CFG_QSCH_Q012_CREDIT_LB_CONTROL_EN_T q012_en = {0};
    DPP_ETM_QMU_CFG_QSCH_Q3CREDITLBMAXCNT_T q3_max_cnt = {0};
    DPP_ETM_QMU_CFG_QSCH_Q012CREDITLBMAXCNT_T q012_max_cnt = {0};
    DPP_ETM_QMU_CFG_QSCH_Q3LBADDRATE_T q3_rate = {0};
    DPP_ETM_QMU_CFG_QSCH_Q012LBADDRATE_T q012_rate = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NONE(dev_id, q3_lb_control_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NONE(dev_id, q012_lb_control_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NONE(dev_id, q3_lb_max_cnt, 0, 0xff);
    ZXIC_COMM_CHECK_DEV_INDEX_NONE(dev_id, q012_lb_max_cnt, 0, 0xff);
    ZXIC_COMM_CHECK_DEV_INDEX_NONE(dev_id, q3_lb_add_rate, 0, 0xfffffff);
    ZXIC_COMM_CHECK_DEV_INDEX_NONE(dev_id, q012_lb_add_rate, 0, 0xfffffff);

    q3_crdt_lb_ctrl_en_reg_index = ETM_QMU_CFG_QSCH_Q3_CREDIT_LB_CONTROL_ENr;
    q012_crdt_lb_ctrl_en_reg_index = ETM_QMU_CFG_QSCH_Q012_CREDIT_LB_CONTROL_ENr;
    q3_crdt_lb_max_cnt_reg_index = ETM_QMU_CFG_QSCH_Q3CREDITLBMAXCNTr;
    q012_crdt_lb_max_cnt_reg_index = ETM_QMU_CFG_QSCH_Q012CREDITLBMAXCNTr;
    q3_crdt_lb_add_rate_reg_index = ETM_QMU_CFG_QSCH_Q3LBADDRATEr;
    q012_crdt_lb_add_rate_reg_index = ETM_QMU_CFG_QSCH_Q012LBADDRATEr;

    q3_en.cfg_qsch_q3_credit_lb_control_en = q3_lb_control_en;
    rc = dpp_reg_write(dev_id,
                       q3_crdt_lb_ctrl_en_reg_index,
                       0,
                       0,
                       &q3_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    q012_en.cfg_qsch_q012_credit_lb_control_en = q012_lb_control_en;
    rc = dpp_reg_write(dev_id,
                       q012_crdt_lb_ctrl_en_reg_index,
                       0,
                       0,
                       &q012_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    q3_max_cnt.cfg_qsch_q3creditlbmaxcnt = q3_lb_max_cnt;
    rc = dpp_reg_write(dev_id,
                       q3_crdt_lb_max_cnt_reg_index,
                       0,
                       0,
                       &q3_max_cnt);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    q012_max_cnt.cfg_qsch_q012creditlbmaxcnt = q012_lb_max_cnt;
    rc = dpp_reg_write(dev_id,
                       q012_crdt_lb_max_cnt_reg_index,
                       0,
                       0,
                       &q012_max_cnt);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    q3_rate.cfg_qsch_q3lbaddrate = q3_lb_add_rate;
    rc = dpp_reg_write(dev_id,
                       q3_crdt_lb_add_rate_reg_index,
                       0,
                       0,
                       &q3_rate);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    q012_rate.cfg_qsch_q012lbaddrate = q012_lb_add_rate;
    rc = dpp_reg_write(dev_id,
                       q012_crdt_lb_add_rate_reg_index,
                       0,
                       0,
                       &q012_rate);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    return DPP_OK;
}

/***********************************************************/
/** 配置0,1号队列挂接1或2号MCN漏桶信息
* @param   tm_type   0-ETM,1-FTM
* @param   dev_id   设备索引编号
* @param   mcn_lb_sel   0：0,1号队列挂接1号MCN漏桶 1：0,1号队列挂接2号MCN漏桶
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/09
************************************************************/
DPP_STATUS dpp_tm_qmu_mcn_lb_sel_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 mcn_lb_sel)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_CFG_QSCH_Q01_ATTACH_EN_T q01_attach_en = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, mcn_lb_sel, 0, 1);

    q01_attach_en.cfg_qsch_q01_attach_en = mcn_lb_sel;

    rc = dpp_reg_write(dev_id,
                       ETM_QMU_CFG_QSCH_Q01_ATTACH_ENr,
                       0,
                       0,
                       &q01_attach_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    return DPP_OK;
}

/***********************************************************/
/** 配置多播队列0~2的授权输出SP、DWRR
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   sp_or_dwrr   SP、DWRR模式选择。0：SP；1：DWRR。
* @param   dwrr_w0   0号队列DWRR权重(0~127)
* @param   dwrr_w1   1号队列DWRR权重(0~127)
* @param   dwrr_w2   2号队列DWRR权重(0~127)
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/09
************************************************************/
DPP_STATUS dpp_tm_qmu_mul_sp_dwrr_set(ZXIC_UINT32 dev_id,
                                      ZXIC_UINT32 sp_or_dwrr,
                                      ZXIC_UINT32 dwrr_w0,
                                      ZXIC_UINT32 dwrr_w1,
                                      ZXIC_UINT32 dwrr_w2)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_CFG_QSCH_SP_DWRR_EN_T sp_dwrr_en = {0};
    DPP_ETM_QMU_CFG_QSCH_W0_T qsch_w0 = {0};
    DPP_ETM_QMU_CFG_QSCH_W1_T qsch_w1 = {0};
    DPP_ETM_QMU_CFG_QSCH_W2_T qsch_w2 = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, sp_or_dwrr, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dwrr_w0, 0, 0x7f);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dwrr_w1, 0, 0x7f);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dwrr_w2, 0, 0x7f);

    sp_dwrr_en.cfg_qsch_sp_dwrr_en = sp_or_dwrr;
    rc = dpp_reg_write(dev_id,
                       ETM_QMU_CFG_QSCH_SP_DWRR_ENr,
                       0,
                       0,
                       &sp_dwrr_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    qsch_w0.cfg_qsch_w0 = dwrr_w0;
    rc = dpp_reg_write(dev_id,
                       ETM_QMU_CFG_QSCH_W0r,
                       0,
                       0,
                       &qsch_w0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    qsch_w1.cfg_qsch_w1 = dwrr_w1;
    rc = dpp_reg_write(dev_id,
                       ETM_QMU_CFG_QSCH_W1r,
                       0,
                       0,
                       &qsch_w1);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    qsch_w2.cfg_qsch_w2 = dwrr_w2;
    rc = dpp_reg_write(dev_id,
                       ETM_QMU_CFG_QSCH_W2r,
                       0,
                       0,
                       &qsch_w2);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    return DPP_OK;
}

/***********************************************************/
/** 配置分目的SA整形打开或关闭
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   shap_en   分目的SA整形使能开关 0：表示关闭 1：表示打开
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/09
************************************************************/
DPP_STATUS dpp_tm_qmu_dest_sa_shap_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 shap_en)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_CFGMT_QMU_SASHAP_EN_T sa_shap_en = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, shap_en, 0, 1);
 
    sa_shap_en.cfgmt_qmu_sashap_en = shap_en;
    rc = dpp_reg_write(dev_id,
                       ETM_QMU_CFGMT_QMU_SASHAP_ENr,
                       0,
                       0,
                       &sa_shap_en);

    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    return DPP_OK;
}

/***********************************************************/
/** 配置轮转扫描使能和扫描速率
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   scan_en   轮转扫描使能。0：关闭，1：开启
* @param   scan_rate   轮转扫描速率，配置扫描周期不得少于256个周期
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/10
************************************************************/
DPP_STATUS dpp_tm_qmu_scan_rate_set(ZXIC_UINT32 dev_id,
                                    ZXIC_UINT32 scan_en,
                                    ZXIC_UINT32 scan_rate)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_CFG_QSCH_SCAN_EN_T qsch_scan_en = {0};
    DPP_ETM_QMU_CFG_QSCH_SCANRATE_T qsch_scanrate = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, scan_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, scan_rate, 0x100, 0xfffff);


    qsch_scan_en.cfg_qsch_scan_en = scan_en;
    rc = dpp_reg_write(dev_id,
                       ETM_QMU_CFG_QSCH_SCAN_ENr,
                       0,
                       0,
                       &qsch_scan_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    qsch_scanrate.cfg_qsch_scanrate = scan_rate;
    rc = dpp_reg_write(dev_id,
                       ETM_QMU_CFG_QSCH_SCANRATEr,
                       0,
                       0,
                       &qsch_scanrate);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    return DPP_OK;
}

/***********************************************************/
/** 获得轮转扫描使能和扫描速率
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   scan_en   轮转扫描使能。0：关闭，1：开启
* @param   scan_rate   轮转扫描速率，配置扫描周期不得少于256个周期
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/10
************************************************************/
DPP_STATUS dpp_tm_qmu_scan_rate_get(ZXIC_UINT32 dev_id,
                                    ZXIC_UINT32 *p_scan_en,
                                    ZXIC_UINT32 *p_scan_rate)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_CFG_QSCH_SCAN_EN_T qsch_scan_en = {0};
    DPP_ETM_QMU_CFG_QSCH_SCANRATE_T qsch_scanrate = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_scan_en);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_scan_rate);


    rc = dpp_reg_read(dev_id,
                      ETM_QMU_CFG_QSCH_SCAN_ENr,
                      0,
                      0,
                      &qsch_scan_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    *p_scan_en = qsch_scan_en.cfg_qsch_scan_en;

    rc = dpp_reg_read(dev_id,
                      ETM_QMU_CFG_QSCH_SCANRATEr,
                      0,
                      0,
                      &qsch_scanrate);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    *p_scan_rate = qsch_scanrate.cfg_qsch_scanrate;

    return DPP_OK;


}

/***********************************************************/
/** 配置轮转扫描队列范围
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   first_que  起始队列号
* @param   last_que   终止队列号
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  whuashan      @date  2019/09/10
************************************************************/
DPP_STATUS dpp_tm_qmu_scan_que_range_set(ZXIC_UINT32 dev_id,
                                         ZXIC_UINT32 first_que,
                                         ZXIC_UINT32 last_que)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_CFG_QSCH_SCANFRSTQUE_T qsch_scanfirstque_t = {0};
    DPP_ETM_QMU_CFG_QSCH_SCANLASTQUE_T qsch_scanlastque_t = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, first_que, 0, DPP_ETM_Q_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, last_que, 0, DPP_ETM_Q_NUM - 1);

    qsch_scanfirstque_t.cfg_qsch_scanfrstque = first_que;
    qsch_scanlastque_t.cfg_qsch_scanlastque = last_que;

    rc = dpp_reg_write(dev_id,
                       ETM_QMU_CFG_QSCH_SCANFRSTQUEr,
                       0,
                       0,
                       &qsch_scanfirstque_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    rc = dpp_reg_write(dev_id,
                       ETM_QMU_CFG_QSCH_SCANLASTQUEr,
                       0,
                       0,
                       &qsch_scanlastque_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;

}

/***********************************************************/
/** 获取轮转扫描队列范围
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   first_que  起始队列号
* @param   last_que   终止队列号
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  whuashan      @date  2019/09/10
************************************************************/
DPP_STATUS dpp_tm_qmu_scan_que_range_get(ZXIC_UINT32 dev_id,
                                         ZXIC_UINT32 *first_que,
                                         ZXIC_UINT32 *last_que)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_CFG_QSCH_SCANFRSTQUE_T qsch_scanfirstque_t = {0};
    DPP_ETM_QMU_CFG_QSCH_SCANLASTQUE_T qsch_scanlastque_t = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, first_que);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, last_que);

    rc = dpp_reg_read(dev_id,
                      ETM_QMU_CFG_QSCH_SCANFRSTQUEr,
                      0,
                      0,
                      &qsch_scanfirstque_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc = dpp_reg_read(dev_id,
                      ETM_QMU_CFG_QSCH_SCANLASTQUEr,
                      0,
                      0,
                      &qsch_scanlastque_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *first_que = qsch_scanfirstque_t.cfg_qsch_scanfrstque;
    *last_que = qsch_scanlastque_t.cfg_qsch_scanlastque;

    return DPP_OK;

}
#endif

/***********************************************************/
/** 配置读命令老化使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   aged_en   读命令老化使能：0：不使能；1：使能
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/11
************************************************************/
DPP_STATUS dpp_tm_qmu_wr_aged_en_set(DPP_DEV_T *dev, ZXIC_UINT32 aged_en)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_QCFG_CSCH_AGED_CFG_T aged_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), aged_en, 0, 1);


    aged_cfg.qcfg_csch_aged_cfg = aged_en;

    rc = dpp_reg_write(dev,
                       ETM_QMU_QCFG_CSCH_AGED_CFGr,
                       0,
                       0,
                       &aged_cfg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    return DPP_OK;
}

/***********************************************************/
/** 配置读命令老化速率
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   scan_time   读命令老化速率（扫描间隔时间）
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/11
************************************************************/
DPP_STATUS dpp_tm_qmu_wr_aged_scan_time_set(DPP_DEV_T *dev, ZXIC_UINT32 scan_time)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_QCFG_CSCH_AGED_SCAN_TIME_T aged_scan_time = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), scan_time, 0, 0xffffffff);

    aged_scan_time.qcfg_csch_aged_scan_time = scan_time;

    rc = dpp_reg_write(dev,
                       ETM_QMU_QCFG_CSCH_AGED_SCAN_TIMEr,
                       0,
                       0,
                       &aged_scan_time);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    return DPP_OK;
}

/***********************************************************/
/** 获得读命令老化速率
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_scan_time   读命令老化速率（扫描间隔时间）
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/11
************************************************************/
DPP_STATUS dpp_tm_qmu_wr_aged_scan_time_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_scan_time)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_QCFG_CSCH_AGED_SCAN_TIME_T aged_scan_time = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_scan_time);

    rc = dpp_reg_read(dev,
                      ETM_QMU_QCFG_CSCH_AGED_SCAN_TIMEr,
                      0,
                      0,
                      &aged_scan_time);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *p_scan_time = aged_scan_time.qcfg_csch_aged_scan_time;

    return DPP_OK;
}


/***********************************************************/
/** 获取QMU清空状态
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_clr_done_flag   队列是否清空完成
*
* @return
* @remark  无
* @see
* @author  szq      @date  2015/05/21
************************************************************/
DPP_STATUS dpp_tm_qmu_qlist_qcfg_clr_done_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_clr_done_flag)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_QLIST_QCFG_CLR_DONE_T clr_done_flag = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_clr_done_flag);


    rc = dpp_reg_read(dev,
                      ETM_QMU_QLIST_QCFG_CLR_DONEr,
                      0,
                      0,
                      &clr_done_flag);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *p_clr_done_flag = clr_done_flag.qlist_qcfg_clr_done;

    return DPP_OK;
}

/***********************************************************/
/** 配置qsch调度分端口整形速率和使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   shape_en   整形使能
* @param   token_add_num   [23:12]:添加令牌数目
* @param   token_gap   [11:0]:添加令牌间隔，其中实际间隔为配置间隔+1
* @param   token_depth 桶深，单位B,范围[0-0x1EE00]
*公式：（1000*8*token_num）/(gap+1) = X Mbps
*     主频= 600 MHz
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  xuhb   2020-5-15
************************************************************/
DPP_STATUS dpp_tm_qmu_qsch_port_shape_set(DPP_DEV_T *dev,
                                     ZXIC_UINT32 port_id,
                                     ZXIC_UINT32 token_add_num,
                                     ZXIC_UINT32 token_gap,
                                     ZXIC_UINT32 token_depth,
                                     ZXIC_UINT32 shape_en)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_QCFG_QSCH_SHAP_PARAM_T qsch_shap_param = {0};
    DPP_ETM_QMU_QCFG_QSCH_SHAP_TOKEN_T qsch_shap_token_depth = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), port_id, 0, DPP_TM_PP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), shape_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), token_add_num, 0, 0xfff);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), token_gap, 0, 0xfff);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), token_depth, 0, 0x1f000);

    /* 配置整形桶深 */
    qsch_shap_token_depth.qcfg_qsch_shap_token= token_depth;
    rc = dpp_reg_write(dev,
                       ETM_QMU_QCFG_QSCH_SHAP_TOKENr,
                       0,
                       port_id,
                       &qsch_shap_token_depth);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    qsch_shap_param.qcfg_qsch_shap_en = shape_en;
    qsch_shap_param.qcfg_qsch_shap_param1= token_add_num ;
    qsch_shap_param.qcfg_qsch_shap_param2= token_gap;
    rc = dpp_reg_write(dev,
                       ETM_QMU_QCFG_QSCH_SHAP_PARAMr,
                       0,
                       port_id,
                       &qsch_shap_param);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");


    return DPP_OK;

}


/***********************************************************/
/** 配置CMD_SW分端口整形速率和使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   shape_en   整形使能
* @param   token_add_num   [23:12]:添加令牌数目
* @param   token_gap   [11:0]:添加令牌间隔，其中实际间隔为配置间隔+1
* @param   token_depth 桶深，单位B,范围[0-0x1EE00]
*公式：（1000*8*token_num）/(gap+1) = X Mbps
*     主频= 600 MHz
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  whuashan   2020-3-17
************************************************************/
DPP_STATUS dpp_tm_qmu_port_shape_set(DPP_DEV_T *dev,
                                     ZXIC_UINT32 port_id,
                                     ZXIC_UINT32 token_add_num,
                                     ZXIC_UINT32 token_gap,
                                     ZXIC_UINT32 token_depth,
                                     ZXIC_UINT32 shape_en)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_QCFG_CSW_SHAP_PARAMETER_T csw_shap_param = {0};
    DPP_ETM_QMU_QCFG_CSW_SHAP_TOKEN_DEPTH_T csw_shap_token_depth = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), port_id, 0, DPP_TM_PP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), shape_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), token_add_num, 0, 0xfff);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), token_gap, 0, 0xfff);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), token_depth, 0, 0x1ee00);

    /* 配置整形桶深 */
    csw_shap_token_depth.qcfg_csw_shap_token_depth = token_depth;
    rc = dpp_reg_write(dev,
                       ETM_QMU_QCFG_CSW_SHAP_TOKEN_DEPTHr,
                       0,
                       port_id,
                       &csw_shap_token_depth);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    csw_shap_param.qcfg_csw_shap_en = shape_en;
    csw_shap_param.qcfg_csw_shap_parameter = (token_add_num << 12) | token_gap;
    rc = dpp_reg_write(dev,
                       ETM_QMU_QCFG_CSW_SHAP_PARAMETERr,
                       0,
                       port_id,
                       &csw_shap_param);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;

}

/***********************************************************/
/** 获得CMD_SW分端口整形速率和使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_shape_en   整形使能
* @param   p_token_add_num   [23:12]:添加令牌数目
* @param   p_token_gap   [11:0]:添加令牌间隔，其中实际间隔为配置间隔+1
* @param   p_token_depth 桶深，单位B
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  whuashan   2020-3-17
************************************************************/
DPP_STATUS dpp_tm_qmu_port_shape_get(DPP_DEV_T *dev,
                                     ZXIC_UINT32 port_id,
                                     ZXIC_UINT32 *p_token_add_num,
                                     ZXIC_UINT32 *p_token_gap,
                                     ZXIC_UINT32 *p_token_depth,
                                     ZXIC_UINT32 *p_shape_en)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_QCFG_CSW_SHAP_PARAMETER_T csw_shap_param = {0};
    DPP_ETM_QMU_QCFG_CSW_SHAP_TOKEN_DEPTH_T csw_shap_token_depth = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), port_id, 0, DPP_TM_PP_NUM - 1);
    
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_token_add_num);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_token_gap);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_token_depth);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_shape_en);

    rc = dpp_reg_read(dev,
                      ETM_QMU_QCFG_CSW_SHAP_PARAMETERr,
                      0,
                      port_id,
                      &csw_shap_param);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *p_shape_en = csw_shap_param.qcfg_csw_shap_en;
    *p_token_add_num = (csw_shap_param.qcfg_csw_shap_parameter >> 12) & 0xfff;
    *p_token_gap = csw_shap_param.qcfg_csw_shap_parameter & 0xfff;

    rc = dpp_reg_read(dev,
                      ETM_QMU_QCFG_CSW_SHAP_TOKEN_DEPTHr,
                      0,
                      port_id,
                      &csw_shap_token_depth);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *p_token_depth = csw_shap_token_depth.qcfg_csw_shap_token_depth;

    return DPP_OK;

}

#if 0
/***********************************************************/
/** 配置CMD_SW分端口(qmu出端口)整形速率和使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   shape_cir 整形值，单位Mbps，范围[0-160000]
* @param   shape_cbs 桶深， 单位B，范围[0-0x1EE00]
* @param   shape_en   整形使能
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author   whuashan   2020-3-17
************************************************************/
DPP_STATUS dpp_tm_qmu_egress_shape_set(ZXIC_UINT32 dev_id,
                                       ZXIC_UINT32 port_id,
                                       ZXIC_UINT32 shape_cir,
                                       ZXIC_UINT32 shape_cbs,
                                       ZXIC_UINT32 shape_en)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_QCFG_CSW_SHAP_PARAMETER_T csw_shap_param = {0};
    DPP_ETM_QMU_QCFG_CSW_SHAP_TOKEN_DEPTH_T csw_shap_token_depth = {0};
    QMU_PORT_SHAPE_PARA qmu_port_shape_para[100] = {{0}};

    ZXIC_UINT32 token_add_num = 0;
    ZXIC_UINT32 token_gap = 0;
    ZXIC_UINT32 shape_value_amplified = 0;
    ZXIC_UINT32 compare_value = 0;
    ZXIC_UINT32 shape_para_cnt = 0;
    ZXIC_UINT32 shape_para_final_cnt = 0;
    ZXIC_UINT32 shape_min_value = 0;
    ZXIC_UINT32 shape_min_value_num = 0;

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, port_id, 0, DPP_TM_PP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, shape_cir, 0, 400000);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, shape_cbs, 0, 0x1EE00);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, shape_en, 0, 1);

    /* 获取token_add_num、token_gap */
    for (token_gap = 10; token_gap <= 0xfff; token_gap++ )
    {
        for (token_add_num = 1; token_add_num <= 0xfff; token_add_num++)
        {
            shape_value_amplified = (1000 * 8 * token_add_num) / (token_gap + 1);
            compare_value = (shape_value_amplified - (shape_cir * DPP_TM_QMU_PORT_SHAP_MAG));

            /* 0~20的范围,避免获取token_add_num、token_gap失败 */
            if ( (compare_value > 0) && (compare_value < 20) && (shape_para_cnt<100))
            {
                qmu_port_shape_para[shape_para_cnt].shape_value_amplified = shape_value_amplified;
                qmu_port_shape_para[shape_para_cnt].token_add_num = token_add_num;
                qmu_port_shape_para[shape_para_cnt].token_gap =  token_gap;
                shape_para_final_cnt = shape_para_cnt + 1;
                shape_para_cnt++;
            }
        }
    }

    /* 获取最小的整形值参数 */
    shape_min_value = qmu_port_shape_para[0].shape_value_amplified;

    for (shape_para_cnt = 0; shape_para_cnt < shape_para_final_cnt; shape_para_cnt++)
    {
        if (shape_min_value > qmu_port_shape_para[shape_para_cnt].shape_value_amplified)
        {
            shape_min_value = qmu_port_shape_para[shape_para_cnt].shape_value_amplified;
            shape_min_value_num  = shape_para_cnt ;
        }
    }

    /* 配置整形桶深 */
    csw_shap_token_depth.qcfg_csw_shap_token_depth = shape_cbs;
    rc = dpp_reg_write(dev_id,
                       ETM_QMU_QCFG_CSW_SHAP_TOKEN_DEPTHr,
                       0,
                       port_id,
                       &csw_shap_token_depth);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    /* 配置整形速率 */
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, shape_min_value_num, 0, 99);
    csw_shap_param.qcfg_csw_shap_en = shape_en;
    csw_shap_param.qcfg_csw_shap_parameter = (qmu_port_shape_para[shape_min_value_num].token_add_num << 12) | qmu_port_shape_para[shape_min_value_num].token_gap;
    rc = dpp_reg_write(dev_id,
                       ETM_QMU_QCFG_CSW_SHAP_PARAMETERr,
                       0,
                       port_id,
                       &csw_shap_param);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;

}


/***********************************************************/
/** 获取CMD_SW分端口整形速率和使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   shape_vlue 整形值，单位Mbps
* @param   shape_en   整形使能
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author   zmy   @20151217
************************************************************/
DPP_STATUS dpp_tm_qmu_egress_shape_get(ZXIC_UINT32 dev_id,
                                       ZXIC_UINT32 port_id,
                                       ZXIC_UINT32 *shape_value,
                                       ZXIC_UINT32 *shape_en)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_QCFG_CSW_SHAP_PARAMETER_T csw_shap_param = {0};
    ZXIC_UINT32 token_add_num = 0;
    ZXIC_UINT32 token_gap = 0;

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, port_id, 0, DPP_TM_PP_NUM - 1);

    rc = dpp_reg_read(dev_id,
                      ETM_QMU_QCFG_CSW_SHAP_PARAMETERr,
                      0,
                      port_id,
                      &csw_shap_param);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");


    token_add_num = (csw_shap_param.qcfg_csw_shap_parameter >> 12) & 0xfff;
    token_gap = csw_shap_param.qcfg_csw_shap_parameter & 0xfff;

    *shape_value = (((600 * 8 * token_add_num) / (token_gap + 1 )) / DPP_TM_QMU_PORT_SHAP_MAG);

    *shape_en = csw_shap_param.qcfg_csw_shap_en;

    return DPP_OK;

}

#endif
/***********************************************************/
/** 配置需要检测的特定队列号
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   qnum   需要检测统计的特定的队列号
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/18
************************************************************/
DPP_STATUS dpp_tm_qmu_spec_qnum_set(DPP_DEV_T *dev, ZXIC_UINT32 qnum)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_OBSERVE_QNUM_SET_T observe_qnum = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), qnum, 0, DPP_ETM_Q_NUM - 1);

    observe_qnum.observe_qnum_set = qnum;
    rc = dpp_reg_write(dev,
                       ETM_QMU_OBSERVE_QNUM_SETr,
                       0,
                       0,
                       &observe_qnum);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;

}

/***********************************************************/
/** 获得特定的队列号
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_qnum   需要检测统计的特定的队列号
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/23
************************************************************/
DPP_STATUS dpp_tm_qmu_spec_qnum_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_qnum)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_OBSERVE_QNUM_SET_T observe_qnum = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_qnum);

    rc = dpp_reg_read(dev,
                      ETM_QMU_OBSERVE_QNUM_SETr,
                      0,
                      0,
                      &observe_qnum);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *p_qnum = observe_qnum.observe_qnum_set;

    return DPP_OK;

}

/***********************************************************/
/** 配置需要检测的队列组
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   group_num   需要检测统计的特定的队列组。这里按取q的低3bit
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/18
************************************************************/
DPP_STATUS dpp_tm_qmu_spec_group_set(DPP_DEV_T *dev, ZXIC_UINT32 group_num)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_OBSERVE_BATCH_SET_T observe_batch = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), group_num, 0, 7);

    observe_batch.observe_batch_set = group_num;
    rc = dpp_reg_write(dev,
                       ETM_QMU_OBSERVE_BATCH_SETr,
                       0,
                       0,
                       &observe_batch);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 获得需要检测的队列组
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_group_num   需要检测统计的特定的队列组。这里按取q的低3bit
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/23
************************************************************/
DPP_STATUS dpp_tm_qmu_spec_group_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_group_num)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_OBSERVE_BATCH_SET_T observe_batch = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_group_num);

    rc = dpp_reg_read(dev,
                      ETM_QMU_OBSERVE_BATCH_SETr,
                      0,
                      0,
                      &observe_batch);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *p_group_num = observe_batch.observe_batch_set;

    return DPP_OK;
}

#if 0
/***********************************************************/
/** 配置出队暂存使用的进程总数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   used_inall   出队暂存使用的进程总数=19-N，默认3表示使用16个进程
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/18
************************************************************/
DPP_STATUS dpp_tm_qmu_pid_use_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 used_inall)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_CFG_PID_USE_INALL_T pid_use_inall = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, used_inall, 0, 19);

    pid_use_inall.cfgmt_nod_rd_buf_0_aful_th = 19 - used_inall;
    rc = dpp_reg_write(dev_id,
                       ETM_QMU_CFG_PID_USE_INALLr,
                       0,
                       0,
                       &pid_use_inall);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 获得出队暂存使用的进程总数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_used_inall   出队暂存使用的进程总数=19-N，默认3表示使用16个进程
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/23
************************************************************/
DPP_STATUS dpp_tm_qmu_pid_use_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_used_inall)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_CFG_PID_USE_INALL_T pid_use_inall = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_used_inall);

    rc = dpp_reg_read(dev_id,
                      ETM_QMU_CFG_PID_USE_INALLr,
                      0,
                      0,
                      &pid_use_inall);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_used_inall = 19 - pid_use_inall.cfgmt_nod_rd_buf_0_aful_th;

    return DPP_OK;
}

/***********************************************************/
/** 配置出队暂存自回加进程总数阈值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   round_th   出队暂存自回加进程总数阈值=19-N，默认4表示使用15个进程就自回加
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/18
************************************************************/
DPP_STATUS dpp_tm_qmu_pid_round_th_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 round_th)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_CFG_PID_ROUND_TH_T pid_round_th = {0};
    ZXIC_COMM_CHECK_DEV_INDEX_SUB_OVERFLOW_NO_ASSERT(dev_id,  19 , round_th);    
    pid_round_th.cfgmt_nod_rd_buf_1_aful_th = 19 - round_th;


    rc = dpp_reg_write(dev_id,
                       ETM_QMU_CFG_PID_ROUND_THr,
                       0,
                       0,
                       &pid_round_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 获得出队暂存自回加进程总数阈值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_round_th   出队暂存自回加进程总数阈值=19-N，默认4表示使用15个进程就自回加
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/23
************************************************************/

DPP_STATUS dpp_tm_qmu_pid_round_th_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_round_th)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_CFG_PID_ROUND_TH_T pid_round_th = {0};


    rc = dpp_reg_read(dev_id,
                      ETM_QMU_CFG_PID_ROUND_THr,
                      0,
                      0,
                      &pid_round_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_round_th = 19 - pid_round_th.cfgmt_nod_rd_buf_1_aful_th;

    return DPP_OK;
}


/***********************************************************/
/** 配置队列授权盈余
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   qnum   配置的队列号
* @param   value   授权盈余
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/18
************************************************************/
DPP_STATUS dpp_tm_qmu_crbal_value_set(ZXIC_UINT32 dev_id,
                                      ZXIC_UINT32 qnum,
                                      ZXIC_UINT32 value)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_QSCH_RW_CRBAL_T qsch_rw_crbal = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, value, 0, 0x1ffff);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, qnum, 0, DPP_ETM_Q_NUM - 1);

    qsch_rw_crbal.qsch_rw_crbal = value;
    rc = dpp_reg_write(dev_id,
                       ETM_QMU_QSCH_RW_CRBALr,
                       0,
                       qnum,
                       &qsch_rw_crbal);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 获得队列授权盈余
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   qnum   配置的队列号
* @param   p_value   授权盈余
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/23
************************************************************/
DPP_STATUS dpp_tm_qmu_crbal_value_get(ZXIC_UINT32 dev_id,
                                      ZXIC_UINT32 qnum,
                                      ZXIC_UINT32 *p_value)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_QSCH_RW_CRBAL_T qsch_rw_crbal = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_value);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, qnum, 0, DPP_ETM_Q_NUM - 1);

    rc = dpp_reg_read(dev_id,
                      ETM_QMU_QSCH_RW_CRBALr,
                      0,
                      qnum,
                      &qsch_rw_crbal);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_value = qsch_rw_crbal.qsch_rw_crbal & 0x1ffff;

    return DPP_OK;
}

/***********************************************************/
/** 配置分目的SA整形桶深上、下限参数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   max_value   分目的SA整形桶深上限，必须配置为正值
* @param   min_value   分目的SA整形桶深下限，必须配置为负值
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/18
************************************************************/
DPP_STATUS dpp_tm_qmu_dest_sa_shape_para_set(ZXIC_UINT32 dev_id,
                                             ZXIC_SINT32 max_value,
                                             ZXIC_SINT32 min_value)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_CFGMT_SASHAP_TOKEN_MAX_T sashap_token_max = {0};
    DPP_ETM_QMU_CFGMT_SASHAP_TOKEN_MIN_T sashap_token_min = {0};


    if (max_value < 0)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "[dev_id %d] max_value < 0, err!!!\n", dev_id);
        ZXIC_COMM_ASSERT(0);
        return DPP_ERR;
    }

    if (min_value > 0)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "[dev_id %d] min_value > 0, err!!!\n", dev_id);
        ZXIC_COMM_ASSERT(0);
        return DPP_ERR;
    }

    sashap_token_max.cfgmt_sashap_token_max = max_value;
    rc = dpp_reg_write(dev_id,
                       ETM_QMU_CFGMT_SASHAP_TOKEN_MAXr,
                       0,
                       0,
                       &sashap_token_max);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    sashap_token_min.cfgmt_sashap_token_min = min_value;
    rc = dpp_reg_write(dev_id,
                       ETM_QMU_CFGMT_SASHAP_TOKEN_MINr,
                       0,
                       0,
                       &sashap_token_min);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 获得分目的SA整形桶深上、下限参数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_max_value   分目的SA整形桶深上限，必须配置为正值
* @param   p_min_value   分目的SA整形桶深下限，必须配置为负值
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/23
************************************************************/
DPP_STATUS dpp_tm_qmu_dest_sa_shape_para_get(ZXIC_UINT32 dev_id,
                                             ZXIC_UINT32 *p_max_value,
                                             ZXIC_UINT32 *p_min_value)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_CFGMT_SASHAP_TOKEN_MAX_T sashap_token_max = {0};
    DPP_ETM_QMU_CFGMT_SASHAP_TOKEN_MIN_T sashap_token_min = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_max_value);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_min_value);


    rc = dpp_reg_read(dev_id,
                      ETM_QMU_CFGMT_SASHAP_TOKEN_MAXr,
                      0,
                      0,
                      &sashap_token_max);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    *p_max_value = sashap_token_max.cfgmt_sashap_token_max;

    rc = dpp_reg_read(dev_id,
                      ETM_QMU_CFGMT_SASHAP_TOKEN_MINr,
                      0,
                      0,
                      &sashap_token_min);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    *p_min_value = sashap_token_min.cfgmt_sashap_token_min;

    return DPP_OK;
}

/***********************************************************/
/** 配置CRS状态
* @param   dev_id  设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   qnum   队列号
* @param   state
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/19
************************************************************/
DPP_STATUS dpp_tm_qmu_crs_state_set(ZXIC_UINT32 dev_id,
                                    ZXIC_UINT32 qnum,
                                    ZXIC_UINT32 state)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_QSCH_RW_CRS_T qsch_rw_crs = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, state, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, qnum, 0, DPP_ETM_Q_NUM - 1);

    qsch_rw_crs.qsch_rw_crs = state;
    rc = dpp_reg_write(dev_id,
                       ETM_QMU_QSCH_RW_CRSr,
                       0,
                       qnum,
                       &qsch_rw_crs);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 获得CRS状态
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   qnum   队列号
* @param   p_state
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/23
************************************************************/
DPP_STATUS dpp_tm_qmu_crs_state_get(ZXIC_UINT32 dev_id,
                                    ZXIC_UINT32 qnum,
                                    ZXIC_UINT32 *p_state)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_QSCH_RW_CRS_T qsch_rw_crs = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_state);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, qnum, 0, DPP_ETM_Q_NUM - 1);


    rc = dpp_reg_read(dev_id,
                      ETM_QMU_QSCH_RW_CRSr,
                      0,
                      qnum,
                      &qsch_rw_crs);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    *p_state = qsch_rw_crs.qsch_rw_crs;

    return DPP_OK;
}

#endif
/***********************************************************/
/** 配置自动授权队列范围
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   first_que   自授权起始队列号
* @param   last_que   自授权终止队列号
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/19
************************************************************/
DPP_STATUS dpp_tm_qmu_auto_credit_que_set(DPP_DEV_T *dev,
                                          ZXIC_UINT32 first_que,
                                          ZXIC_UINT32 last_que)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_CFG_QSCH_AUTOCRFRSTQUE_T qsch_autocrfrstque = {0};
    DPP_ETM_QMU_CFG_QSCH_AUTOCRLASTQUE_T qsch_autocrlastque = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), first_que, 0, DPP_ETM_Q_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), last_que, 0, DPP_ETM_Q_NUM - 1);

    if (first_que > last_que)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "[dev_id %d] first_que > last_que, err!!!\n", DEV_ID(dev));

        return DPP_ERR;
    }

    qsch_autocrfrstque.cfg_qsch_autocrfrstque = first_que;
    rc = dpp_reg_write(dev,
                       ETM_QMU_CFG_QSCH_AUTOCRFRSTQUEr,
                       0,
                       0,
                       &qsch_autocrfrstque);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    qsch_autocrlastque.cfg_qsch_autocrlastque = last_que;
    rc = dpp_reg_write(dev,
                       ETM_QMU_CFG_QSCH_AUTOCRLASTQUEr,
                       0,
                       0,
                       &qsch_autocrlastque);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 获得自动授权队列范围
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_first_que   自授权起始队列号
* @param   p_last_que   自授权终止队列号
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/23
************************************************************/
DPP_STATUS dpp_tm_qmu_auto_credit_que_get(DPP_DEV_T *dev,
                                          ZXIC_UINT32 *p_first_que,
                                          ZXIC_UINT32 *p_last_que)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_CFG_QSCH_AUTOCRFRSTQUE_T qsch_autocrfrstque = {0};
    DPP_ETM_QMU_CFG_QSCH_AUTOCRLASTQUE_T qsch_autocrlastque = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_first_que);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_last_que);

    rc = dpp_reg_read(dev,
                      ETM_QMU_CFG_QSCH_AUTOCRFRSTQUEr,
                      0,
                      0,
                      &qsch_autocrfrstque);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
    *p_first_que = qsch_autocrfrstque.cfg_qsch_autocrfrstque;

    rc = dpp_reg_read(dev,
                      ETM_QMU_CFG_QSCH_AUTOCRLASTQUEr,
                      0,
                      0,
                      &qsch_autocrlastque);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
    *p_last_que = qsch_autocrlastque.cfg_qsch_autocrlastque;

    return DPP_OK;
}

/***********************************************************/
/** 配置自动授权开启使能及扫描速率
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   auto_crdt_en   自动授权开启使能，默认关闭。0：关闭；1：开启
* @param   auto_crdt_rate   自授权速率配置
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/19
************************************************************/
DPP_STATUS dpp_tm_qmu_auto_credit_rate_set(DPP_DEV_T *dev,
                                           ZXIC_UINT32 auto_crdt_en,
                                           ZXIC_UINT32 auto_crdt_rate)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_CFG_QSCH_AUTO_CREDIT_CONTROL_EN_T credit_control_en = {0};
    DPP_ETM_QMU_CFG_QSCH_AUTOCREDITRATE_T autocredit_rate = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), auto_crdt_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), auto_crdt_rate, 0, 0xfffff);


    credit_control_en.cfg_qsch_auto_credit_control_en = auto_crdt_en;
    rc = dpp_reg_write(dev,
                       ETM_QMU_CFG_QSCH_AUTO_CREDIT_CONTROL_ENr,
                       0,
                       0,
                       &credit_control_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    autocredit_rate.cfg_qsch_autocreditrate = auto_crdt_rate;
    rc = dpp_reg_write(dev,
                       ETM_QMU_CFG_QSCH_AUTOCREDITRATEr,
                       0,
                       0,
                       &autocredit_rate);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 获得自动授权开启使能及扫描速率
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_auto_crdt_en   自动授权开启使能，默认关闭。0：关闭；1：开启
* @param   p_auto_crdt_rate   自授权速率配置
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/23
************************************************************/
DPP_STATUS dpp_tm_qmu_auto_credit_rate_get(DPP_DEV_T *dev,
                                           ZXIC_UINT32 *p_auto_crdt_en,
                                           ZXIC_UINT32 *p_auto_crdt_rate)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_CFG_QSCH_AUTO_CREDIT_CONTROL_EN_T credit_control_en = {0};
    DPP_ETM_QMU_CFG_QSCH_AUTOCREDITRATE_T autocredit_rate = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_auto_crdt_en);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_auto_crdt_rate);

    rc = dpp_reg_read(dev,
                      ETM_QMU_CFG_QSCH_AUTO_CREDIT_CONTROL_ENr,
                      0,
                      0,
                      &credit_control_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
    *p_auto_crdt_en = credit_control_en.cfg_qsch_auto_credit_control_en;

    rc = dpp_reg_read(dev,
                      ETM_QMU_CFG_QSCH_AUTOCREDITRATEr,
                      0,
                      0,
                      &autocredit_rate);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
    *p_auto_crdt_rate = autocredit_rate.cfg_qsch_autocreditrate;

    return DPP_OK;
}

#if 0
/***********************************************************/
/** 配置授权丢弃使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   all_drop_en   所有授权丢弃使能：1：允许丢弃所有授权；0：仅允许丢弃拥塞授权
* @param   drop_en   授权丢弃使能：1：允许丢弃授权；0：禁止丢弃授权
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/23
************************************************************/
DPP_STATUS dpp_tm_qmu_crbal_drop_en_set(ZXIC_UINT32 dev_id,
                                        ZXIC_UINT32 all_drop_en,
                                        ZXIC_UINT32 drop_en)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_CFGMT_QSCH_CRBAL_DROP_EN_T crbal_drop_en = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, all_drop_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, drop_en, 0, 1);

    crbal_drop_en.cfgmt_qsch_all_crbal_drop_en = all_drop_en;
    crbal_drop_en.cfgmt_qsch_crbal_drop_en = drop_en;
    rc = dpp_reg_write(dev_id,
                       ETM_QMU_CFGMT_QSCH_CRBAL_DROP_ENr,
                       0,
                       0,
                       &crbal_drop_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 获得授权丢弃使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_all_drop_en   所有授权丢弃使能：1：允许丢弃所有授权；0：仅允许丢弃拥塞授权
* @param   p_drop_en   授权丢弃使能：1：允许丢弃授权；0：禁止丢弃授权
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/23
************************************************************/
DPP_STATUS dpp_tm_qmu_crbal_drop_en_get(ZXIC_UINT32 dev_id,
                                        ZXIC_UINT32 *p_all_drop_en,
                                        ZXIC_UINT32 *p_drop_en)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_CFGMT_QSCH_CRBAL_DROP_EN_T crbal_drop_en = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_all_drop_en);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_drop_en);

    rc = dpp_reg_read(dev_id,
                      ETM_QMU_CFGMT_QSCH_CRBAL_DROP_ENr,
                      0,
                      0,
                      &crbal_drop_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_all_drop_en = crbal_drop_en.cfgmt_qsch_all_crbal_drop_en;
    *p_drop_en = crbal_drop_en.cfgmt_qsch_crbal_drop_en;

    return DPP_OK;
}


/***********************************************************/
/** 获取特定队列发送的crs normal的个数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   注 须先设置统计的特定队列
* @param
* @return
* @remark  无
* @see
* @author  yjd     @date  2015/07/09
************************************************************/
DPP_STATUS dpp_tm_qmu_spec_q_crs_normal_cnt(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_que_crs_normal_cnt)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_SPEC_Q_CRS_NORMAL_CNT_T crs_normal_cnt;

    rc = dpp_reg_read(dev_id,
                      ETM_QMU_SPEC_Q_CRS_NORMAL_CNTr,
                      0,
                      0,
                      &crs_normal_cnt);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_que_crs_normal_cnt = crs_normal_cnt.spec_q_crs_normal_cnt;
    return DPP_OK;
}

/***********************************************************/
/** 获取特定队列发送的crs off的个数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   注 须先设置统计的特定队列
* @param
* @return
* @remark  无
* @see
* @author  yjd     @date  2015/07/09
************************************************************/
DPP_STATUS dpp_tm_qmu_spec_q_crs_off_cnt(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_que_crs_off_cnt)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_SPEC_Q_CRS_OFF_CNT_T crs_off_cnt;

    rc = dpp_reg_read(dev_id,
                      ETM_QMU_SPEC_Q_CRS_OFF_CNTr,
                      0,
                      0,
                      &crs_off_cnt);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_que_crs_off_cnt = crs_off_cnt.spec_q_crs_off_cnt;
    return DPP_OK;
}

#endif
/***********************************************************/
/**设置自然拥塞反压门限值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param
* @return
* @remark  无
* @see
* @author  yjd     @date  2015/07/16
************************************************************/
DPP_STATUS dpp_tm_qmu_qcfg_csch_congest_th_set(DPP_DEV_T *dev, ZXIC_UINT32 port_id, ZXIC_UINT32 qmu_congest_th)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_QCFG_CSCH_CONGEST_TH_T qcfg_csch_congest_th = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), port_id, 0, DPP_TM_PP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), qmu_congest_th, 0, 0x1ffff);

    qcfg_csch_congest_th.qcfg_csch_congest_th = qmu_congest_th;
    rc = dpp_reg_write(dev,
                       ETM_QMU_QCFG_CSCH_CONGEST_THr,
                       0,
                       port_id,
                       &qcfg_csch_congest_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/**获取自然拥塞反压门限值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param
* @return
* @remark  无
* @see
* @author  yjd     @date  2015/07/16
************************************************************/
DPP_STATUS dpp_tm_qmu_qcfg_csch_congest_th_get(DPP_DEV_T *dev, ZXIC_UINT32 port_id, ZXIC_UINT32 *p_qmu_congest_th)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_QCFG_CSCH_CONGEST_TH_T qcfg_csch_congest_th = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_qmu_congest_th);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), port_id, 0, DPP_TM_PP_NUM - 1);

    rc = dpp_reg_read(dev,
                      ETM_QMU_QCFG_CSCH_CONGEST_THr,
                      0,
                      port_id,
                      &qcfg_csch_congest_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *p_qmu_congest_th = qcfg_csch_congest_th.qcfg_csch_congest_th;

    return DPP_OK;
}

/***********************************************************/
/**设置CMD_SCH分优先级反压门限值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param
* @return
* @remark  无
* @see
* @author  yjd     @date  2015/07/16
************************************************************/
DPP_STATUS dpp_tm_qmu_qcfg_csch_sp_fc_th_set(DPP_DEV_T *dev, ZXIC_UINT32 port_id, ZXIC_UINT32 q_pri, ZXIC_UINT32 qmu_sp_fc_th)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 index = 0;
    DPP_ETM_QMU_QCFG_CSCH_SP_FC_TH_T qcfg_csch_sp_fc_th = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), port_id, 0, DPP_TM_PP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), q_pri, 0, 4);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), qmu_sp_fc_th, 0, 0x1ffff);

    index = port_id * 5 + q_pri;
    qcfg_csch_sp_fc_th.qcfg_csch_sp_fc_th = qmu_sp_fc_th;

    rc = dpp_reg_write(dev,
                       ETM_QMU_QCFG_CSCH_SP_FC_THr,
                       0,
                       index,
                       &qcfg_csch_sp_fc_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/**获取自然拥塞反压门限值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param
* @return
* @remark  无
* @see
* @author  yjd     @date  2015/07/16
************************************************************/
DPP_STATUS dpp_tm_qmu_qcfg_csch_sp_fc_th_get(DPP_DEV_T *dev, ZXIC_UINT32 port_id, ZXIC_UINT32 q_pri, ZXIC_UINT32 *p_qmu_sp_fc_th)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 index = 0;
    DPP_ETM_QMU_QCFG_CSCH_SP_FC_TH_T qcfg_csch_sp_fc_th = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), port_id, 0, DPP_TM_PP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), q_pri, 0, 4);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_qmu_sp_fc_th);

    index = port_id * 5 + q_pri;

    rc = dpp_reg_read(dev,
                      ETM_QMU_QCFG_CSCH_SP_FC_THr,
                      0,
                      index,
                      &qcfg_csch_sp_fc_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *p_qmu_sp_fc_th = qcfg_csch_sp_fc_th.qcfg_csch_sp_fc_th;

    return DPP_OK;
}

#if 0
/***********************************************************/
/**每隔10s获取crs状态的个数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param
* @return
* @remark  无
* @see
* @author  zmy     @date  2015/08/07
************************************************************/

DPP_STATUS dpp_tm_crs_statics(ZXIC_UINT32 dev_id, ZXIC_UINT32 que_id)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_TM_CNT_MODE_T que_get_mode = {0};
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 q_crs_normal_cnt = 0;
    ZXIC_UINT32 q_crs_off_cnt = 0;

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, que_id, 0, DPP_ETM_Q_NUM - 1);

    rc = dpp_tm_cfgmt_cnt_mode_get(dev_id, &que_get_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_get");
    que_get_mode.count_rd_mode = 1;

    rc = dpp_tm_cfgmt_cnt_mode_set(dev_id, &que_get_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_set");

    rc = dpp_tm_qmu_spec_qnum_set(dev_id, que_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_qnum_set");
    rc = dpp_tm_qmu_spec_q_crs_normal_cnt(dev_id, &q_crs_normal_cnt);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_q_crs_normal_cnt");
    rc = dpp_tm_qmu_spec_q_crs_off_cnt(dev_id, &q_crs_off_cnt);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_q_crs_off_cnt");

    for (i = 0; i <= 2; i++)
    {
        zxic_comm_sleep(10000);
        rc = dpp_tm_qmu_spec_q_crs_normal_cnt(dev_id, &q_crs_normal_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_q_crs_normal_cnt");
        ZXIC_COMM_PRINT("q_crs_normal_cnt is %d\n ", q_crs_normal_cnt);

        rc = dpp_tm_qmu_spec_q_crs_off_cnt(dev_id, &q_crs_off_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_q_crs_off_cnt");
        ZXIC_COMM_PRINT("q_crs_off_cnt is %d\n ", q_crs_off_cnt);
    }

    rc = dpp_tm_cfgmt_cnt_mode_get(dev_id, &que_get_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_get");
    que_get_mode.count_rd_mode = 0;

    rc = dpp_tm_cfgmt_cnt_mode_set(dev_id, &que_get_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_set");

    return DPP_OK;
}



/***********************************************************/
/** 统计QMU发送和CRDT模块指定授权流接收的CRS计数(10s内)
* @param   dev_id  设备编号
* @param   que_id   QMU队列号
* @param   ackflow_id   授权流号
* @param   valid_flag   0:队列发送和授权流接收都统计; 1:只关注队列发送，2:只关注授权流接收。
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2017/05/12
************************************************************/
DPP_STATUS dpp_tm_crs_cnt_prt(ZXIC_UINT32 dev_id, ZXIC_UINT32 que_id, ZXIC_UINT32 ackflow_id, ZXIC_UINT32 valid_flag)
{
    DPP_STATUS rc = DPP_OK;
    DPP_TM_CNT_MODE_T que_get_mode = {0};
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 q_crs_normal_cnt = 0;
    ZXIC_UINT32 q_crs_off_cnt = 0;
    ZXIC_UINT32 crdt_crs_que_id_index = 0;
    ZXIC_UINT32 all_crs_normal_cnt_index = 0;
    ZXIC_UINT32 all_crs_off_cnt_index = 0;
    ZXIC_UINT32 que_crs_normal_cnt_index = 0;
    ZXIC_UINT32 que_crd_off_cnt_index = 0;
    ZXIC_UINT32 crs_end_state_index = 0;



    /* 结构体变量定义 */
    DPP_ETM_CRDT_CRS_QUE_ID_T crdt_crs_que_id = {0};
    DPP_ETM_CRDT_FIFO_OUT_ALL_CRS_NORMAL_CNT_T all_crs_normal_cnt = {0};
    DPP_ETM_CRDT_FIFO_OUT_ALL_CRS_OFF_CNT_T all_crs_off_cnt = {0};
    DPP_ETM_CRDT_FIFO_OUT_QUE_CRS_NORMAL_CNT_T que_crs_normal_cnt = {0};
    DPP_ETM_CRDT_FIFO_OUT_QUE_CRS_OFF_CNT_T que_crd_off_cnt = {0};
    DPP_ETM_CRDT_QMU_CRS_END_STATE_T crs_end_state = {0};

    crdt_crs_que_id_index = ETM_CRDT_CRS_QUE_IDr;
    all_crs_normal_cnt_index = ETM_CRDT_FIFO_OUT_ALL_CRS_NORMAL_CNTr;
    all_crs_off_cnt_index = ETM_CRDT_FIFO_OUT_ALL_CRS_OFF_CNTr;
    que_crs_normal_cnt_index = ETM_CRDT_FIFO_OUT_QUE_CRS_NORMAL_CNTr;
    que_crd_off_cnt_index = ETM_CRDT_FIFO_OUT_QUE_CRS_OFF_CNTr;
    crs_end_state_index = ETM_CRDT_QMU_CRS_END_STATEr;

    if (0 == valid_flag)
    {
        /* 设置统计CRDT CRS 接收个数的队列号 */
        crdt_crs_que_id.crs_que_id = ackflow_id;
        rc = dpp_reg_write(dev_id,
                           crdt_crs_que_id_index,
                           0,
                           0,
                           &crdt_crs_que_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");


        /* 1.先配置读清 */
        rc = dpp_tm_cfgmt_cnt_mode_get(dev_id, &que_get_mode);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_get");
        que_get_mode.count_rd_mode = 1;

        rc = dpp_tm_cfgmt_cnt_mode_set(dev_id, &que_get_mode);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_set");

        /* 队列发送crs读清 */

        rc = dpp_tm_qmu_spec_qnum_set(dev_id, que_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_qnum_set");
        rc = dpp_tm_qmu_spec_q_crs_normal_cnt(dev_id, &q_crs_normal_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_q_crs_normal_cnt");
        rc = dpp_tm_qmu_spec_q_crs_off_cnt(dev_id, &q_crs_off_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_q_crs_off_cnt");

        /* crdt接收crs读清 */
        rc = dpp_tm_crdt_clr_diag(dev_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_crdt_clr_diag");


        /* 2.连读两次统计CRS发送和接收 */
        for (i = 0; i < 2; i++)
        {
            zxic_comm_sleep(10000);

            ZXIC_COMM_PRINT("------(%d th)qmu_send & crdt_recv crs_cnt in 10s------\n ", i + 1);
            /* 统计CRS发送 */
            rc = dpp_tm_qmu_spec_q_crs_normal_cnt(dev_id, &q_crs_normal_cnt);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_q_crs_normal_cnt");
            ZXIC_COMM_PRINT("que_id(0x%08x)qmu_send_crs_normal_cnt: 0x%08x\n ", que_id, q_crs_normal_cnt);

            rc = dpp_tm_qmu_spec_q_crs_off_cnt(dev_id, &q_crs_off_cnt);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_q_crs_off_cnt");
            ZXIC_COMM_PRINT("que_id(0x%08x)qmu_send_crs_off_cnt: 0x%08x\n ", que_id, q_crs_off_cnt);

            /* 统计CRS接收 */

            /* 统计CRDT接收到的CRS off总数 */
            rc = dpp_reg_read(dev_id,
                              all_crs_off_cnt_index,
                              0,
                              0,
                              &all_crs_off_cnt);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

            /* 统计CRDT指定队列接收到的CRS off总数 */
            rc = dpp_reg_read(dev_id,
                              que_crd_off_cnt_index,
                              0,
                              0,
                              &que_crd_off_cnt);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
            /* 统计CRDT接收到的CRS normal总数 */
            rc = dpp_reg_read(dev_id,
                              all_crs_normal_cnt_index,
                              0,
                              0,
                              &all_crs_normal_cnt);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");



            /* 统计CRDT指定队列接收到的CRS normal总数 */
            rc = dpp_reg_read(dev_id,
                              que_crs_normal_cnt_index,
                              0,
                              0,
                              &que_crs_normal_cnt);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");



            /* 统计CRDT指定队列接收到的crs最后的状态 */
            rc = dpp_reg_read(dev_id,
                              crs_end_state_index,
                              0,
                              0,
                              &crs_end_state);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

            /* crdt接收crs读清 */
            rc = dpp_tm_crdt_clr_diag(dev_id);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_crdt_clr_diag");

            /* 打印统计信息 */
            ZXIC_COMM_PRINT("crdt_recv_all_crs_normal_cnt: 0x%08x\n ", all_crs_normal_cnt.fifo_out_all_crs_normal_cnt);
            ZXIC_COMM_PRINT("crdt_recv_all_crs_off_cnt: 0x%08x\n ", all_crs_off_cnt.fifo_out_all_crs_off_cnt);
            ZXIC_COMM_PRINT("ackflow_id(0x%08x)recv_crs_normal_cnt: 0x%08x\n ", ackflow_id, que_crs_normal_cnt.fifo_out_que_crs_normal_cnt);
            ZXIC_COMM_PRINT("ackflow_id(0x%08x)recv_crs_off_cnt: 0x%08x\n ", ackflow_id, que_crd_off_cnt.fifo_out_que_crs_off_cnt);
            ZXIC_COMM_PRINT("ackflow_id(0x%08x)recv_crs_end_state: %d\n ", ackflow_id, crs_end_state.qmu_crs_end_state);

        }

        /* 3.配置成crs非读清 */
        rc = dpp_tm_cfgmt_cnt_mode_get(dev_id, &que_get_mode);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_get");
        que_get_mode.count_rd_mode = 0;

        rc = dpp_tm_cfgmt_cnt_mode_set(dev_id, &que_get_mode);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_set");

    }
    else if (1 == valid_flag)
    {
        /* 1.先配置读清 */
        rc = dpp_tm_cfgmt_cnt_mode_get(dev_id, &que_get_mode);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_get");
        que_get_mode.count_rd_mode = 1;

        rc = dpp_tm_cfgmt_cnt_mode_set(dev_id, &que_get_mode);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_set");

        /* 队列发送crs读清 */

        rc = dpp_tm_qmu_spec_qnum_set(dev_id, que_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_qnum_set");
        rc = dpp_tm_qmu_spec_q_crs_normal_cnt(dev_id, &q_crs_normal_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_q_crs_normal_cnt");
        rc = dpp_tm_qmu_spec_q_crs_off_cnt(dev_id, &q_crs_off_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_q_crs_off_cnt");

        /* 2.连读两次统计CRS发送和接收 */
        for (i = 0; i < 2; i++)
        {
            zxic_comm_sleep(10000);

            ZXIC_COMM_PRINT("------(%d th)qmu_send crs_cnt in 10s------\n ", i + 1);
            /* 统计CRS发送 */
            rc = dpp_tm_qmu_spec_q_crs_normal_cnt(dev_id, &q_crs_normal_cnt);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_q_crs_normal_cnt");
            ZXIC_COMM_PRINT("que_id(0x%08x)qmu_send_crs_normal_cnt: 0x%08x\n ", que_id, q_crs_normal_cnt);

            rc = dpp_tm_qmu_spec_q_crs_off_cnt(dev_id, &q_crs_off_cnt);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_q_crs_off_cnt");
            ZXIC_COMM_PRINT("que_id(0x%08x)qmu_send_crs_off_cnt: 0x%08x\n ", que_id, q_crs_off_cnt);
        }

        /* 3.配置成crs非读清 */
        rc = dpp_tm_cfgmt_cnt_mode_get(dev_id, &que_get_mode);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_get");
        que_get_mode.count_rd_mode = 0;

        rc = dpp_tm_cfgmt_cnt_mode_set(dev_id, &que_get_mode);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_set");
    }

    else if (2 == valid_flag)
    {

        /* 设置统计CRDT CRS 接收个数的队列号 */
        crdt_crs_que_id.crs_que_id = ackflow_id;
        rc = dpp_reg_write(dev_id,
                           crdt_crs_que_id_index,
                           0,
                           0,
                           &crdt_crs_que_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");


        /* 2.连读两次统计CRS发送和接收 */
        for (i = 0; i < 2; i++)
        {

            /* crdt接收crs读清 */
            rc = dpp_tm_crdt_clr_diag(dev_id);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_crdt_clr_diag");

            zxic_comm_sleep(10000);

            ZXIC_COMM_PRINT("------(%d th)crdt_recv crs_cnt in 10s------\n ", i + 1);

            /* 统计CRS接收 */
            /* 统计CRDT指定队列接收到的CRS off总数 */
            rc = dpp_reg_read(dev_id,
                              que_crd_off_cnt_index,
                              0,
                              0,
                              &que_crd_off_cnt);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
            /* 统计CRDT接收到的CRS off 总数 */
            rc = dpp_reg_read(dev_id,
                              all_crs_off_cnt_index,
                              0,
                              0,
                              &all_crs_off_cnt);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

            /* 统计CRDT接收到的CRS normal总数 */
            rc = dpp_reg_read(dev_id,
                              all_crs_normal_cnt_index,
                              0,
                              0,
                              &all_crs_normal_cnt);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");



            /* 统计CRDT指定队列接收到的CRS normal总数 */
            rc = dpp_reg_read(dev_id,
                              que_crs_normal_cnt_index,
                              0,
                              0,
                              &que_crs_normal_cnt);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");


            /* 统计CRDT指定队列接收到的crs最后的状态 */
            rc = dpp_reg_read(dev_id,
                              crs_end_state_index,
                              0,
                              0,
                              &crs_end_state);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
            /* crdt接收crs读清 */
            rc = dpp_tm_crdt_clr_diag(dev_id);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_crdt_clr_diag");
            /* 打印统计信息 */
            ZXIC_COMM_PRINT("crdt_recv_all_crs_normal_cnt: 0x%08x\n ", all_crs_normal_cnt.fifo_out_all_crs_normal_cnt);
            ZXIC_COMM_PRINT("crdt_recv_all_crs_off_cnt: 0x%08x\n ", all_crs_off_cnt.fifo_out_all_crs_off_cnt);
            ZXIC_COMM_PRINT("ackflow_id(0x%08x)recv_crs_normal_cnt: 0x%08x\n ", ackflow_id, que_crs_normal_cnt.fifo_out_que_crs_normal_cnt);
            ZXIC_COMM_PRINT("ackflow_id(0x%08x)recv_crs_off_cnt: 0x%08x\n ", ackflow_id, que_crd_off_cnt.fifo_out_que_crs_off_cnt);
            ZXIC_COMM_PRINT("ackflow_id(0x%08x)recv_crs_end_state: %d\n ", ackflow_id, crs_end_state.qmu_crs_end_state);
        }

    }
    else
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "dpp_tm_crs_cnt_prt:valid_flag_error!!: 0: print crs of que and ackflow;1:print que crs only; 2:print ackflow crs only!\n");

    }


    return DPP_OK;

}

/***********************************************************/
/** 带停流的统计QMU发送和CRDT模块指定授权流接收的CRS计数
* @param   dev_id  设备编号
* @param   que_id   QMU队列号
* @param   ackflow_id   授权流号
* @param   valid_flag   0:默认队列发送和授权流接收都统计,此时队列授权都在本板;
*                       1:只关注队列发送，2:只关注授权流接收，需要与源端队列停流配合使用，
                        先停流，运行该函数；或者直接不停流得到的是某段时间的计数。
* @param   sleep_time   统计多长时间内的crs计数
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2017/05/12
************************************************************/
DPP_STATUS dpp_tm_crs_cnt_prt_1(ZXIC_UINT32 dev_id,
                                ZXIC_UINT32 que_id,
                                ZXIC_UINT32 ackflow_id,
                                ZXIC_UINT32 valid_flag,
                                ZXIC_UINT32 sleep_time)
{
    DPP_STATUS rc = DPP_OK;
    DPP_TM_CNT_MODE_T que_get_mode = {0};
    ZXIC_UINT32 flow_td_th = 0;
    ZXIC_UINT32 q_crs_normal_cnt = 0;
    ZXIC_UINT32 q_crs_off_cnt = 0;
    ZXIC_UINT32 crdt_crs_que_id_index = 0;
    ZXIC_UINT32 all_crs_normal_cnt_index = 0;
    ZXIC_UINT32 all_crs_off_cnt_index = 0;
    ZXIC_UINT32 que_crs_normal_cnt_index = 0;
    ZXIC_UINT32 que_crd_off_cnt_index = 0;
    ZXIC_UINT32 crs_end_state_index = 0;

    /* 结构体变量定义 */
    DPP_ETM_CRDT_CRS_QUE_ID_T crdt_crs_que_id = {0};
    DPP_ETM_CRDT_FIFO_OUT_ALL_CRS_NORMAL_CNT_T all_crs_normal_cnt = {0};
    DPP_ETM_CRDT_FIFO_OUT_ALL_CRS_OFF_CNT_T all_crs_off_cnt = {0};
    DPP_ETM_CRDT_FIFO_OUT_QUE_CRS_NORMAL_CNT_T que_crs_normal_cnt = {0};
    DPP_ETM_CRDT_FIFO_OUT_QUE_CRS_OFF_CNT_T que_crd_off_cnt = {0};
    DPP_ETM_CRDT_QMU_CRS_END_STATE_T crs_end_state = {0};

    crdt_crs_que_id_index = ETM_CRDT_CRS_QUE_IDr;
    all_crs_normal_cnt_index = ETM_CRDT_FIFO_OUT_ALL_CRS_NORMAL_CNTr;
    all_crs_off_cnt_index = ETM_CRDT_FIFO_OUT_ALL_CRS_OFF_CNTr;
    que_crs_normal_cnt_index = ETM_CRDT_FIFO_OUT_QUE_CRS_NORMAL_CNTr;
    que_crd_off_cnt_index = ETM_CRDT_FIFO_OUT_QUE_CRS_OFF_CNTr;
    crs_end_state_index = ETM_CRDT_QMU_CRS_END_STATEr;

    rc = dpp_tm_cgavd_td_th_get(dev_id, QUEUE_LEVEL, que_id, &flow_td_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_td_th_get");

    if (0 == valid_flag)
    {


        /* 设置统计CRDT CRS 接收个数的队列号 */
        crdt_crs_que_id.crs_que_id = ackflow_id;
        rc = dpp_reg_write(dev_id,
                           crdt_crs_que_id_index,
                           0,
                           0,
                           &crdt_crs_que_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        rc = dpp_tm_cgavd_td_th_set(dev_id, QUEUE_LEVEL, que_id, 0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_td_th_set");

        /* 1.停流配置读清 */
        rc = dpp_tm_cfgmt_cnt_mode_get(dev_id, &que_get_mode);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_get");
        que_get_mode.count_rd_mode = 1;

        rc = dpp_tm_cfgmt_cnt_mode_set(dev_id, &que_get_mode);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_set");

        /* 队列发送crs读清 */

        rc = dpp_tm_qmu_spec_qnum_set(dev_id, que_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_qnum_set");
        rc = dpp_tm_qmu_spec_q_crs_normal_cnt(dev_id, &q_crs_normal_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_q_crs_normal_cnt");
        rc = dpp_tm_qmu_spec_q_crs_off_cnt(dev_id, &q_crs_off_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_q_crs_off_cnt");

        /* crdt接收crs读清 */
        rc = dpp_tm_crdt_clr_diag(dev_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_crdt_clr_diag");


        /* 2.统计CRS发送和接收 */
        rc = dpp_tm_cgavd_td_th_set(dev_id, QUEUE_LEVEL, que_id, flow_td_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_td_th_set");


        zxic_comm_sleep(sleep_time);

        rc = dpp_tm_qmu_spec_q_crs_off_cnt(dev_id, &q_crs_off_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_q_crs_off_cnt");

        /* 统计CRDT接收到的CRS off总数 */
        rc = dpp_reg_read(dev_id,
                          all_crs_off_cnt_index,
                          0,
                          0,
                          &all_crs_off_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
        /* 统计CRDT指定队列接收到的CRS off总数 */
        rc = dpp_reg_read(dev_id,
                          que_crd_off_cnt_index,
                          0,
                          0,
                          &que_crd_off_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

        rc = dpp_tm_cgavd_td_th_set(dev_id, QUEUE_LEVEL, que_id, 0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_td_th_set");
        zxic_comm_sleep(1000);

        /* 统计CRS发送 */
        rc = dpp_tm_qmu_spec_q_crs_normal_cnt(dev_id, &q_crs_normal_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_q_crs_normal_cnt");
        ZXIC_COMM_PRINT("que_id(0x%08x)qmu_send_crs_normal_cnt: 0x%08x\n ", que_id, q_crs_normal_cnt);
        ZXIC_COMM_PRINT("que_id(0x%08x)qmu_send_crs_off_cnt: 0x%08x\n ", que_id, q_crs_off_cnt);
        /* 统计CRS接收 */
        /* 统计CRDT接收到的CRS normal总数 */
        rc = dpp_reg_read(dev_id,
                          all_crs_normal_cnt_index,
                          0,
                          0,
                          &all_crs_normal_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

        /* 统计CRDT指定队列接收到的CRS normal总数 */
        rc = dpp_reg_read(dev_id,
                          que_crs_normal_cnt_index,
                          0,
                          0,
                          &que_crs_normal_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");



        /* 统计CRDT指定队列接收到的crs最后的状态 */
        rc = dpp_reg_read(dev_id,
                          crs_end_state_index,
                          0,
                          0,
                          &crs_end_state);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

        /* crdt接收crs读清 */
        rc = dpp_tm_crdt_clr_diag(dev_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_crdt_clr_diag");
        /* 打印统计信息 */
        ZXIC_COMM_PRINT("crdt_recv_all_crs_normal_cnt: 0x%08x\n", all_crs_normal_cnt.fifo_out_all_crs_normal_cnt);
        ZXIC_COMM_PRINT("crdt_recv_all_crs_off_cnt: 0x%08x\n", all_crs_off_cnt.fifo_out_all_crs_off_cnt);
        ZXIC_COMM_PRINT("ackflow_id(0x%08x)recv_crs_normal_cnt: 0x%08x\n", ackflow_id, que_crs_normal_cnt.fifo_out_que_crs_normal_cnt);
        ZXIC_COMM_PRINT("ackflow_id(0x%08x)recv_crs_off_cnt: 0x%08x\n", ackflow_id, que_crd_off_cnt.fifo_out_que_crs_off_cnt);
        ZXIC_COMM_PRINT("ackflow_id(0x%08x)recv_crs_end_state: %d\n", ackflow_id, crs_end_state.qmu_crs_end_state);



        /* 4.配置成crs非读清 */
        rc = dpp_tm_cfgmt_cnt_mode_get(dev_id, &que_get_mode);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_get");
        que_get_mode.count_rd_mode = 0;

        rc = dpp_tm_cfgmt_cnt_mode_set(dev_id, &que_get_mode);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_set");

        /* 5.恢复通流 */
        rc = dpp_tm_cgavd_td_th_set(dev_id, 0, que_id, flow_td_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_td_th_set");

    }
    else if (1 == valid_flag) /* 仅统计停流清零再发流该段时间发送的CRS */
    {
        rc = dpp_tm_cgavd_td_th_set(dev_id, QUEUE_LEVEL, que_id, 0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_td_th_set");

        /* 1.停流配置读清 */
        rc = dpp_tm_cfgmt_cnt_mode_get(dev_id, &que_get_mode);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_get");
        que_get_mode.count_rd_mode = 1;

        rc = dpp_tm_cfgmt_cnt_mode_set(dev_id, &que_get_mode);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_set");

        /* 队列发送crs读清 */

        rc = dpp_tm_qmu_spec_qnum_set(dev_id, que_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_qnum_set");
        rc = dpp_tm_qmu_spec_q_crs_normal_cnt(dev_id, &q_crs_normal_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_q_crs_normal_cnt");
        rc = dpp_tm_qmu_spec_q_crs_off_cnt(dev_id, &q_crs_off_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_q_crs_off_cnt");

        /* 2.开流设定时间后停流统计CRS发送 */
        rc = dpp_tm_cgavd_td_th_set(dev_id, QUEUE_LEVEL, que_id, flow_td_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_td_th_set");


        zxic_comm_sleep(sleep_time);

        rc = dpp_tm_qmu_spec_q_crs_off_cnt(dev_id, &q_crs_off_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_q_crs_off_cnt");

        rc = dpp_tm_cgavd_td_th_set(dev_id, QUEUE_LEVEL, que_id, 0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_td_th_set");
        zxic_comm_sleep(1000);

        /* 统计CRS发送 */
        rc = dpp_tm_qmu_spec_q_crs_normal_cnt(dev_id, &q_crs_normal_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_spec_q_crs_normal_cnt");
        ZXIC_COMM_PRINT("que_id(0x%08x)qmu_send_crs_normal_cnt: 0x%08x\n ", que_id, q_crs_normal_cnt);
        ZXIC_COMM_PRINT("que_id(0x%08x)qmu_send_crs_off_cnt: 0x%08x\n ", que_id, q_crs_off_cnt);

        /* 3.配置成crs非读清 */
        rc = dpp_tm_cfgmt_cnt_mode_get(dev_id, &que_get_mode);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_get");
        que_get_mode.count_rd_mode = 0;

        rc = dpp_tm_cfgmt_cnt_mode_set(dev_id, &que_get_mode);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_set");

        /* 4.恢复通流 */
        rc = dpp_tm_cgavd_td_th_set(dev_id, QUEUE_LEVEL, que_id, flow_td_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_td_th_set");
    }

    else if (2 == valid_flag) /* 仅统计接收，需要源端停流之后运行。*/
    {
        /* 设置统计CRDT CRS 接收个数的队列号 */
        crdt_crs_que_id.crs_que_id = ackflow_id;
        rc = dpp_reg_write(dev_id,
                           crdt_crs_que_id_index,
                           0,
                           0,
                           &crdt_crs_que_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        /* 1.停流配置读清 */
        /* crdt接收crs读清 */
        rc = dpp_tm_crdt_clr_diag(dev_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_crdt_clr_diag");

        zxic_comm_sleep(sleep_time);


        /* 统计CRDT接收到的CRS off总数 */
        rc = dpp_reg_read(dev_id,
                          all_crs_off_cnt_index,
                          0,
                          0,
                          &all_crs_off_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
        /* 统计CRDT指定队列接收到的CRS off总数 */
        rc = dpp_reg_read(dev_id,
                          que_crd_off_cnt_index,
                          0,
                          0,
                          &que_crd_off_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");


        /* 统计CRS接收 */
        /* 统计CRDT接收到的CRS normal总数 */
        rc = dpp_reg_read(dev_id,
                          all_crs_normal_cnt_index,
                          0,
                          0,
                          &all_crs_normal_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

        /* 统计CRDT指定队列接收到的CRS normal总数 */
        rc = dpp_reg_read(dev_id,
                          que_crs_normal_cnt_index,
                          0,
                          0,
                          &que_crs_normal_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");



        /* 统计CRDT指定队列接收到的crs最后的状态 */
        rc = dpp_reg_read(dev_id,
                          crs_end_state_index,
                          0,
                          0,
                          &crs_end_state);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
        /* crdt接收crs读清 */
        rc = dpp_tm_crdt_clr_diag(dev_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_crdt_clr_diag");
        /* 打印统计信息 */
        ZXIC_COMM_PRINT("crdt_recv_all_crs_normal_cnt: 0x%08x\n", all_crs_normal_cnt.fifo_out_all_crs_normal_cnt);
        ZXIC_COMM_PRINT("crdt_recv_all_crs_off_cnt: 0x%08x\n", all_crs_off_cnt.fifo_out_all_crs_off_cnt);
        ZXIC_COMM_PRINT("ackflow_id(0x%08x)recv_crs_normal_cnt: 0x%08x\n", ackflow_id, que_crs_normal_cnt.fifo_out_que_crs_normal_cnt);
        ZXIC_COMM_PRINT("ackflow_id(0x%08x)recv_crs_off_cnt: 0x%08x\n", ackflow_id, que_crd_off_cnt.fifo_out_que_crs_off_cnt);
        ZXIC_COMM_PRINT("ackflow_id(0x%08x)recv_crs_end_state: %d\n", ackflow_id, crs_end_state.qmu_crs_end_state);


    }
    else
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "dpp_tm_crs_cnt_prt_1:valid_flag_error!!: 0: print crs of que and ackflow;1:print que crs only; 2:print ackflow crs only\n");

    }


    return DPP_OK;

}


/***********************************************************/
/** 读取qlist入队及出队状态监控
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2015/08/26
************************************************************/
DPP_STATUS dpp_tm_qmu_qlist_state_query(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_QCFG_QMU_QLIST_STATE_QUERY_T  qcfg_qmu_qlist_state_query = {0};


    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_QCFG_QMU_QLIST_STATE_QUERYr,
                       0,
                       0,
                       &qcfg_qmu_qlist_state_query);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    ZXIC_COMM_PRINT("pkt_age_req_fifo_afull : %d\n", qcfg_qmu_qlist_state_query.pkt_age_req_fifo_afull);
    ZXIC_COMM_PRINT("rd_release_fwft_afull : %d\n", qcfg_qmu_qlist_state_query.rd_release_fwft_afull);
    ZXIC_COMM_PRINT("drop_imem_fwft_afull : %d\n", qcfg_qmu_qlist_state_query.drop_imem_fwft_afull);
    ZXIC_COMM_PRINT("pkt_age_req_fifo_empty : %d\n", qcfg_qmu_qlist_state_query.pkt_age_req_fifo_empty);
    ZXIC_COMM_PRINT("rd_release_fwft_empty : %d\n", qcfg_qmu_qlist_state_query.rd_release_fwft_empty);
    ZXIC_COMM_PRINT("drop_imem_fwft_empty : %d\n", qcfg_qmu_qlist_state_query.drop_imem_fwft_empty);
    ZXIC_COMM_PRINT("mmu_qmu_sop_rd_rdy : %d\n", qcfg_qmu_qlist_state_query.mmu_qmu_sop_rd_rdy);
    ZXIC_COMM_PRINT("big_fifo_empty : %d\n", qcfg_qmu_qlist_state_query.big_fifo_empty);
    ZXIC_COMM_PRINT("qmu_mmu_rd_release_rdy : %d\n", qcfg_qmu_qlist_state_query.qmu_mmu_rd_release_rdy);
    ZXIC_COMM_PRINT("xsw_qmu_crs_rdy : %d\n", qcfg_qmu_qlist_state_query.xsw_qmu_crs_rdy);
    ZXIC_COMM_PRINT("mmu_qmu_rdy : %d\n", qcfg_qmu_qlist_state_query.mmu_qmu_rdy);
    ZXIC_COMM_PRINT("mmu_ql_wr_rdy : %d\n", qcfg_qmu_qlist_state_query.mmu_ql_wr_rdy);
    ZXIC_COMM_PRINT("mmu_ql_rd_rdy : %d\n", qcfg_qmu_qlist_state_query.mmu_ql_rd_rdy);
    ZXIC_COMM_PRINT("csw_ql_rdy : %d\n", qcfg_qmu_qlist_state_query.csw_ql_rdy);
    ZXIC_COMM_PRINT("ql_init_done : %d\n", qcfg_qmu_qlist_state_query.ql_init_done);
    ZXIC_COMM_PRINT("free_addr_ready : %d\n", qcfg_qmu_qlist_state_query.free_addr_ready);
    ZXIC_COMM_PRINT("bank_group_afull : %d\n", qcfg_qmu_qlist_state_query.bank_group_afull);
    ZXIC_COMM_PRINT("pds_fwft_empty : %d\n", qcfg_qmu_qlist_state_query.pds_fwft_empty);
    ZXIC_COMM_PRINT("enq_rpt_fwft_afull : %d\n", qcfg_qmu_qlist_state_query.enq_rpt_fwft_afull);


    return DPP_OK;
}

#endif
/***********************************************************/
/** 配置QMU流控计数模式
* @param   dev_id   设备号
* @param   tm_type   0-ETM,1-FTM
* @param   mode   流控模式，0-电平流控；1-边沿流控
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/05/07
************************************************************/
DPP_STATUS dpp_tm_qmu_fc_cnt_mode_set(DPP_DEV_T *dev, ZXIC_UINT32 mode)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_FC_CNT_MODE_T fc_cnt_mode_reg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), mode, 0, 1);

    fc_cnt_mode_reg.fc_cnt_mode = mode;
    rc  = dpp_reg_write(dev,
                        ETM_QMU_FC_CNT_MODEr,
                        0,
                        0,
                        &fc_cnt_mode_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 读取QMU流控计数模式
* @param   dev_id   设备号
* @param   tm_type   0-ETM,1-FTM
* @param   p_mode   流控模式，0-电平流控；1-边沿流控
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/05/07
************************************************************/
DPP_STATUS dpp_tm_qmu_fc_cnt_mode_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_mode)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_FC_CNT_MODE_T fc_cnt_mode_reg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_mode);

    rc  = dpp_reg_read(dev,
                       ETM_QMU_FC_CNT_MODEr,
                       0,
                       0,
                       &fc_cnt_mode_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *p_mode = fc_cnt_mode_reg.fc_cnt_mode;

    return DPP_OK;
}

/***********************************************************/
/** 配置QMU需要检测流控的端口号
* @param   dev_id   设备号
* @param   tm_type   0-ETM,1-FTM
* @param   port_id   端口号
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/05/07
************************************************************/
DPP_STATUS dpp_tm_qmu_observe_portfc_set(DPP_DEV_T *dev, ZXIC_UINT32 port_id)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_OBSERVE_PORTFC_SPEC_T observe_portfc_reg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), port_id, 0, DPP_TM_PP_NUM - 1);


    observe_portfc_reg.observe_portfc_spec = port_id;
    rc  = dpp_reg_write(dev,
                        ETM_QMU_OBSERVE_PORTFC_SPECr,
                        0,
                        0,
                        &observe_portfc_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/** 配置QMU需要统计的队列号
* @param   dev_id   设备号
* @param   tm_type   0-ETM,1-FTM
* @param   q_id   队列号
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/05/07
************************************************************/
DPP_STATUS dpp_tm_qmu_observe_qnum_set(DPP_DEV_T *dev, ZXIC_UINT32 q_id)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_OBSERVE_QNUM_SET_T observe_qnum_reg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), q_id, 0, DPP_ETM_Q_NUM - 1);

    observe_qnum_reg.observe_qnum_set = q_id;
    rc  = dpp_reg_write(dev,
                        ETM_QMU_OBSERVE_PORTFC_SPECr,
                        0,
                        0,
                        &observe_qnum_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/** 配置QMU需要统计的队列组
* @param   dev_id   设备号
* @param   tm_type   0-ETM,1-FTM
* @param   batch_id   队列组
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/05/07
************************************************************/
DPP_STATUS dpp_tm_qmu_observe_batch_set(DPP_DEV_T *dev, ZXIC_UINT32 batch_id)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_OBSERVE_BATCH_SET_T observe_batch_reg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), batch_id, 0, 7);

    observe_batch_reg.observe_batch_set = batch_id;
    rc  = dpp_reg_write(dev,
                        ETM_QMU_OBSERVE_BATCH_SETr,
                        0,
                        0,
                        &observe_batch_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}


/******************************************************************************
*包老化配置
* @param: dev_id: 设备索引编号
* @param   tm_type   0-ETM,1-FTM
*           aging_en: 包老化使能：1表示包老化功能使能；0表示包老化功能关闭。
*           aging_interval: 普通老化两次的间隔配置
*           aging_step_interval: 普通老化的老化时间的步进配置值
*           aging_start_qnum: 老化起始队列
*           aging_end_qnum: 老化结束队列
*           aging_req_aful_th: 普通老化FIFO的将满阈值
*           aging_pkt_num: 一次老化的包个数
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/05/10
************************************************************/
DPP_STATUS dpp_tm_qmu_pkt_aging_set(DPP_DEV_T *dev,
                                    ZXIC_UINT32 aging_en,
                                    ZXIC_UINT32 aging_interval,
                                    ZXIC_UINT32 aging_step_interval,
                                    ZXIC_UINT32 aging_start_qnum,
                                    ZXIC_UINT32 aging_end_qnum,
                                    ZXIC_UINT32 aging_pkt_num,
                                    ZXIC_UINT32 aging_req_aful_th)
{
    /* 返回值变量定义 */
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 age_pkt_num_reg_index = 0;
    ZXIC_UINT32 age_step_interval_reg_index = 0;
    ZXIC_UINT32 age_interval_reg_index = 0;
    ZXIC_UINT32 age_qnum_reg_index = 0;
    ZXIC_UINT32 age_req_aful_th_reg_index = 0;
    ZXIC_UINT32 age_en_reg_index = 0;

    /* 结构体变量定义 */
    DPP_ETM_QMU_CFGMT_QMU_PKT_AGE_EN_T cfg_age_en = {0};
    DPP_ETM_QMU_CFGMT_PKT_AGE_STEP_INTERVAL_T cfg_age_step_interval = {0};
    DPP_ETM_QMU_CFGMT_QMU_PKT_AGE_INTERVAL_T cfg_age_interval  = {0};
    DPP_ETM_QMU_CFGMT_QMU_PKT_AGE_START_END_T cfg_age_qnum = {0};
    DPP_ETM_QMU_CFGMT_PKT_AGE_REQ_AFUL_TH_T cfg_age_req_aful_th  = {0};
    DPP_ETM_QMU_CFGMT_AGE_PKT_NUM_T cfg_age_pkt_num  = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), aging_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), aging_step_interval, 0, 0xff);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), aging_interval, 0, 0xffff);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), aging_req_aful_th, 0, 0x3f);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), aging_pkt_num, 0, 0xf);

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), aging_start_qnum, 0, DPP_ETM_Q_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), aging_end_qnum, 0, DPP_ETM_Q_NUM - 1);
    age_pkt_num_reg_index = ETM_QMU_CFGMT_AGE_PKT_NUMr;
    age_step_interval_reg_index = ETM_QMU_CFGMT_PKT_AGE_STEP_INTERVALr;
    age_interval_reg_index = ETM_QMU_CFGMT_QMU_PKT_AGE_INTERVALr;
    age_qnum_reg_index = ETM_QMU_CFGMT_QMU_PKT_AGE_START_ENDr;
    age_req_aful_th_reg_index = ETM_QMU_CFGMT_PKT_AGE_REQ_AFUL_THr;
    age_en_reg_index = ETM_QMU_CFGMT_QMU_PKT_AGE_ENr;

    cfg_age_en.cfgmt_qmu_pkt_age_en = aging_en;
    cfg_age_step_interval.cfgmt_pkt_age_step_interval = aging_step_interval;
    cfg_age_interval.cfgmt_qmu_pkt_age_interval = aging_interval;
    cfg_age_qnum.cfgmt_qmu_pkt_age_start = aging_start_qnum;
    cfg_age_qnum.cfgmt_qmu_pkt_age_end = aging_end_qnum;
    cfg_age_req_aful_th.cfgmt_pkt_age_req_aful_th = aging_req_aful_th;
    cfg_age_pkt_num.cfgmt_age_pkt_num = aging_pkt_num;


    rc  = dpp_reg_write(dev,
                        age_pkt_num_reg_index,
                        0,
                        0,
                        &cfg_age_pkt_num);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    rc  = dpp_reg_write(dev,
                        age_step_interval_reg_index,
                        0,
                        0,
                        &cfg_age_step_interval);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    rc  = dpp_reg_write(dev,
                        age_interval_reg_index,
                        0,
                        0,
                        &cfg_age_interval);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    rc  = dpp_reg_write(dev,
                        age_qnum_reg_index,
                        0,
                        0,
                        &cfg_age_qnum);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    rc  = dpp_reg_write(dev,
                        age_req_aful_th_reg_index,
                        0,
                        0,
                        &cfg_age_req_aful_th);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    rc  = dpp_reg_write(dev,
                        age_en_reg_index,
                        0,
                        0,
                        &cfg_age_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}


/******************************************************************************
*配置老化一个包的时间，一次老化一个包，老化队列范围为可配
* @param: dev_id: 设备索引编号
* @param    tm_type   0: etm; 1: ftm;
*           aging_en: 包老化使能：1表示包老化功能使能；0表示包老化功能关闭。
*           aging_time: 老化一个包的时间，单位ms
            aging_que_start:老化起始队列
            aging_que_start:老化终止队列
老化时间=2*aging_interval*step_interval*q_num
aging_interval = (aging_time * 600000) / (2 * 1 * DPP_TM_Q_NUM);
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb     @date  2020/12/08
************************************************************/
DPP_STATUS dpp_tm_qmu_pkt_age_time_set(DPP_DEV_T *dev,
                                       ZXIC_UINT32 aging_en,
                                       ZXIC_UINT32 aging_time,
                                       ZXIC_UINT32 aging_que_start,
                                       ZXIC_UINT32 aging_que_end)
{
    /* 返回值变量定义 */
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 aging_interval = 0;
    //DPP_CRM_CSR_PLL_CLK_SEL_T pll_clk_sel_t = {0};
    // ZXIC_UINT32 sys_clk = 0;
    // ZXIC_UINT32 sys_clk_temp[8]={200,250,300,500,600,800,1000,1200};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), aging_en, 0, 1);
    

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), aging_que_start, 0, DPP_ETM_Q_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), aging_que_end, 0, DPP_ETM_Q_NUM - 1);
#if 0
    rc = dpp_reg_read(dev_id,
                      CRM_CSR_PLL_CLK_SELr,
                      0,
                      0,
                      &pll_clk_sel_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, pll_clk_sel_t.sys_clk_2x_sel, CRM_SYS_CLK_200M, CRM_SYS_CLK_INVALID - 1);

    sys_clk = sys_clk_temp[pll_clk_sel_t.sys_clk_2x_sel] / 2;
    ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id,  aging_time , sys_clk );
    ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id,  aging_time * sys_clk , DPP_TM_KILO_ULL);
    aging_interval = (aging_time * sys_clk * DPP_TM_KILO_ULL * DPP_TM_KILO_ULL / 1000) / (2 * 1 * (aging_que_end - aging_que_start + 1));
#endif


    if (0 == aging_interval)
    {
        aging_interval = 1; /* 防止算出来的是小数，导致写入0 */
    }

    rc = dpp_tm_qmu_pkt_aging_set(dev, aging_en, aging_interval, 1, aging_que_start, aging_que_end, 1, 0xa);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_qmu_pkt_aging_set");

    return DPP_OK;
}

#if 0
/***********************************************************/
/** 获得队列空标志查询
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   qnum   配置的队列号
* @param   p_value   队列空标志查询
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  cy      @date  2016/06/20
************************************************************/
DPP_STATUS dpp_tm_qlist_ept_flag_get(ZXIC_UINT32 dev_id,
                                     ZXIC_UINT32 qnum,
                                     ZXIC_UINT32 *p_value)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_QCFG_QLIST_EPT_RD_T qcfg_qlist_ept_rd = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_value);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, qnum, 0, DPP_ETM_Q_NUM - 1);

    rc = dpp_reg_read(dev_id,
                      ETM_QMU_QCFG_QLIST_EPT_RDr,
                      0,
                      qnum,
                      &qcfg_qlist_ept_rd);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_value = qcfg_qlist_ept_rd.qcfg_qlist_ept_rd;

    return DPP_OK;
}


/***********************************************************/
/** 获得队列深度计数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   qnum   配置的队列号
* @param   p_value  队列深度计数
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  cy      @date  2016/06/20
************************************************************/
DPP_STATUS dpp_tm_qlist_r_bcnt_get(ZXIC_UINT32 dev_id,
                                   ZXIC_UINT32 qnum,
                                   ZXIC_UINT32 *p_value)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_QLIST_R_BCNT_T qlist_r_bcnt = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_value);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, qnum, 0, DPP_ETM_Q_NUM - 1);

    rc = dpp_reg_read(dev_id,
                      ETM_QMU_QLIST_R_BCNTr,
                      0,
                      qnum,
                      &qlist_r_bcnt);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_value = qlist_r_bcnt.qlist_r_bcnt;

    return DPP_OK;
}

#endif
/***********************************************************/
/** 配置pfc使能
* @param   dev_id   设备编号
* @param   pfc_en   配置的值，0-不使能pfc，1-使能pfc
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  lsy      @date  2022/08/22
************************************************************/
DPP_STATUS dpp_tm_qmu_pfc_en_set(DPP_DEV_T *dev, ZXIC_UINT32 pfc_en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_CFGMT_QMU_PFC_EN_T qmu_pfc_en = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), pfc_en, 0, 1);

    qmu_pfc_en.cfgmt_qmu_pfc_en = pfc_en;
    rc  = dpp_reg_write(dev,
                        ETM_QMU_CFGMT_QMU_PFC_ENr,
                        0,
                        0,
                        &qmu_pfc_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 读取pfc使能
* @param   dev_id   设备编号
* @param   pfc_en   配置的值，0-不使能pfc，1-使能pfc
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  lsy      @date  2022/08/22
************************************************************/
DPP_STATUS dpp_tm_qmu_pfc_en_get(DPP_DEV_T *dev, ZXIC_UINT32 *pfc_en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_CFGMT_QMU_PFC_EN_T qmu_pfc_en = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pfc_en);

    *pfc_en = 0xffffffff;
    rc  = dpp_reg_read(dev,
                       ETM_QMU_CFGMT_QMU_PFC_ENr,
                       0,
                       0,
                       &qmu_pfc_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *pfc_en = qmu_pfc_en.cfgmt_qmu_pfc_en;

    return DPP_OK;
}

/***********************************************************/
/** 配置端口pfc掩码
* @param   dev_id   设备编号
* @param   port_id   端口号：0~63
* @param   port_en   端口掩码配置，1pfc模式下该端口接收olif的优先级反压，
*                                  0pfc模式下该端口不接受olif的优先级反压，并将反压信号全部置1
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  lsy      @date  2022/08/22
************************************************************/
DPP_STATUS dpp_tm_qmu_port_pfc_make_set(DPP_DEV_T *dev,
                                        ZXIC_UINT32 port_id,
                                        ZXIC_UINT32 port_en)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 value = 0;
    DPP_ETM_QMU_CFGMT_QMU_PFC_MASK_1_T pfc_mask_31_0 = {0};
    DPP_ETM_QMU_CFGMT_QMU_PFC_MASK_2_T pfc_mask_63_32 = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), port_id, 0, DPP_TM_PP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), port_en, 0, 1);

    if (port_id <= 31)
    {
        /* port_id:[0-31] */
        rc = dpp_reg_read(dev,
                          ETM_QMU_CFGMT_QMU_PFC_MASK_1r,
                          0,
                          0,
                          &pfc_mask_31_0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

        value = pfc_mask_31_0.cfgmt_qmu_pfc_mask_1;

        if (port_en == 0)
        {
            value = value & (~(1u << port_id));
        }
        else
        {
            value = value | (1u << port_id);
        }

        pfc_mask_31_0.cfgmt_qmu_pfc_mask_1 = value;

        rc = dpp_reg_write(dev,
                           ETM_QMU_CFGMT_QMU_PFC_MASK_1r,
                           0,
                           0,
                           &pfc_mask_31_0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
    }
    else
    {
        /* port_id:[32-63] */
        rc = dpp_reg_read(dev,
                          ETM_QMU_CFGMT_QMU_PFC_MASK_2r,
                          0,
                          0,
                          &pfc_mask_63_32);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

        value = pfc_mask_63_32.cfgmt_qmu_pfc_mask_2;

        if (port_en == 0)
        {
            value = value & (~(1u << (port_id - 32)));
        }
        else
        {
            value = value | (1u<< (port_id - 32));
        }

        pfc_mask_63_32.cfgmt_qmu_pfc_mask_2 = value;

        rc = dpp_reg_write(dev,
                           ETM_QMU_CFGMT_QMU_PFC_MASK_2r,
                           0,
                           0,
                           &pfc_mask_63_32);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
    }

    return DPP_OK;
}

/***********************************************************/
/** 获得端口pfc掩码
* @param   dev_id   设备编号
* @param   port_id   端口号：0~63
* @param   port_en   端口掩码配置，1pfc模式下该端口接收olif的优先级反压，
*                                  0pfc模式下该端口不接受olif的优先级反压，并将反压信号全部置1
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  lsy      @date  2022/08/22
************************************************************/
DPP_STATUS dpp_tm_qmu_port_pfc_make_get(DPP_DEV_T *dev,
                                        ZXIC_UINT32 port_id,
                                        ZXIC_UINT32 *p_port_en)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 value = 0;
    DPP_ETM_QMU_CFGMT_QMU_PFC_MASK_1_T pfc_mask_31_0 = {0};
    DPP_ETM_QMU_CFGMT_QMU_PFC_MASK_2_T pfc_mask_63_32 = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), port_id, 0, DPP_TM_PP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_port_en);

    if (port_id <= 31)
    {
        /* port_id:[0-31] */
        rc = dpp_reg_read(dev,
                          ETM_QMU_CFGMT_QMU_PFC_MASK_1r,
                          0,
                          0,
                          &pfc_mask_31_0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

        value = pfc_mask_31_0.cfgmt_qmu_pfc_mask_1;

        *p_port_en = 1 & (value >> port_id);
    }
    else
    {
        /* port_id:[32-63] */
        rc = dpp_reg_read(dev,
                          ETM_QMU_CFGMT_QMU_PFC_MASK_2r,
                          0,
                          0,
                          &pfc_mask_63_32);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

        value = pfc_mask_63_32.cfgmt_qmu_pfc_mask_2;

        *p_port_en = 1 & (value >> (port_id - 32));
    }

    return DPP_OK;
}

#if 0
/***********************************************************/
/** QMU初始化配置场景
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   case_no   QMU初始化场景编号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2020/4/14
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 case_no_temp)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 case_no = 0;
    ZXIC_UINT32 ftm_ddr_no = 0;
    ZXIC_UINT32 etm_ddr_no = 0;

    
    g_qmu_init_case_no = case_no_temp;
    
    if ((case_no_temp & 0xff) == 0xef && (case_no_temp >> 8)!= 0 && (case_no_temp >> 
    16 != 0))
    {
        case_no = 0xef;
        ZXIC_COMM_PRINT("Here get in qmu init:%d case_num is %d\n", case_no);
        /*低8-15bit作为FTM的DDR0-9中的编号,高16-23bit作为ETM的DDR0-9中的编号(only case_16 use)*/
        ftm_ddr_no = (case_no_temp >> 8) & 0xff;
        etm_ddr_no = (case_no_temp >> 16) & 0xff;
    }
    else if ((case_no_temp & 0xf) == 0xc)
    {
        /*etm 和ftm共享指定1组ddr 8bank,其中etm 0-3bank ftm 4-7bank*/
        case_no = 0xc;
    }
    else if ((case_no_temp & 0xf) == 0xd)
    {
        /*一个tm独享指定4组8bank*/
        case_no = 0xd;
    }
    else if(((case_no_temp & 0xf) == 0xe) || ((case_no_temp & 0xf) == 0xf) \
           || ((case_no_temp & 0xf) == 0xb))
    {
        /*提取case_no_temp的低4bit作为case_no*/
        case_no = case_no_temp & 0xf;
        ZXIC_COMM_PRINT("Here get in qmu init:%d case_num is %d\n", case_no);

        /*中间4bit作为ETM的DDR0-9中的编号,最高4bit作为FTM的DDR0-9中的编号(only case_11 14 15 use)*/
        ftm_ddr_no = (case_no_temp >> 4) & 0xf;
        etm_ddr_no = (case_no_temp >> 8) & 0xf;
    }
    else
    {
        case_no = case_no_temp & 0xf;
    }
    /**clear mmu qmu**/
    rc = dpp_tm_qmu_mmu_cfg_clr(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_mmu_cfg_clr");


    if (case_no == 0)
    {
        /**纯片内pd16k模式**/
        //rc = dpp_tm_qmu_init_set_pd16k_2(dev_id, 64);
        //ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_init_set_pd16k_2");

        rc = dpp_tm_qmu_init_set_chuk32(dev_id, 512);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_init_set_chuk32");
    }
    else if (case_no == 1)
    {
        /* 场景1：每个tm用8组*4bank，MMU实际分配ftm:0-7组(0145bank); etm:2-9组(2367bank) */
        rc = dpp_tm_qmu_init_set_1(dev_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_init_set_1");
    }
    else if (case_no == 2)
    {
        /* 场景2：tm独享8组ddr:ftm为0-7组ddr的0123bank，etm为0-7组ddr 4567bank */
        rc = dpp_tm_qmu_init_set_2(dev_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_init_set_2");
    }
    else if (case_no == 3)
    {
        /* 场景3：TM独享8组ddr:FTM为0-3组ddr的0~3bank，ETM为4-7组ddr 0~3bank */
        rc = dpp_tm_qmu_init_set_3(dev_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_init_set_3");
    }
    else if (case_no == 4)
    {
        /* 场景4：TM共享2组ddr:FTM为2、4组ddr的01bank，ETM为2、4组ddr 23bank */
        /*需开启IP_rotation、mmu multi_burst,关闭mmu_rotation_en*/
        rc = dpp_tm_qmu_init_set_4(dev_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_init_set_4");
    }
    else if (case_no == 5)
    {
        /* 场景5：TM共享4组ddr:FTM为1234组ddr的01bank，ETM为1234组ddr 23bank */
        /*需开启IP_rotation、mmu multi_burst,关闭mmu_rotation_en*/
        rc = dpp_tm_qmu_init_set_5(dev_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_init_set_5");
    }
    else if (case_no == 6)
    {
        /*场景6：4组*2bank，MMU实际分配4,6,7,9; etm使用01bank,ftm使用23bank*/
        /*需开启IP_rotation、mmu multi_burst,关闭mmu_rotation_en*/
        rc = dpp_tm_qmu_init_set_6(dev_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_init_set_6");
    }
    else if (case_no == 7)
    {
        /* 关闭rotation*/
        //rc = dpp_mmu_init(0, 0, 0, 0, 0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_mmu_init");
        /* 场景7：每个tm用8组*4bank，MMU实际分配ftm:2-9组(0145bank); etm:2-9组(2367bank) */
        rc = dpp_tm_qmu_init_set_7(dev_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_init_set_7");
    }
    else if (case_no == 8)
    {
        /* 场景8：每个tm用8组*4bank，MMU实际分配ftm:2-9组(01bank); etm:2-9组(23bank) */
        /*需开启IP_rotation、mmu multi_burst,关闭mmu_rotation_en*/
        rc = dpp_tm_qmu_init_set_8(dev_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_init_set_8");
    }
    else if (case_no == 9)
    {
        /*场景9：4组*2bank，MMU实际分配6,7,8,9; etm使用01bank,ftm使用23bank*/
        /*需开启IP_rotation、mmu multi_burst,关闭mmu_rotation_en*/
        rc = dpp_tm_qmu_init_set_9(dev_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_init_set_9");
    }
    else if (case_no == 10)
    {
        /* 关闭rotation*/
        //rc = dpp_mmu_init(0, 0, 0, 0, 0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_mmu_init");
        /*场景10：独享2组*8bank，MMU实际分配1,2; etm or ftm使用0-7bank*/
        rc = dpp_tm_qmu_init_set_10(dev_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_init_set_10");
    }
    else if (case_no == 11)
    {
        /*场景11：TM共享2组ddr:FTM为ddr_no1,ddr_no2组ddr的01bank，ETM为ddr_no1,ddr_no2组ddr 23bank*/
        rc = dpp_tm_qmu_init_set_11(dev_id, ftm_ddr_no, etm_ddr_no);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_init_set_11");
    }
    else if (case_no == 0xc)
    {
        /*etm 和ftm共享指定1组ddr 8bank,其中etm 0-3bank ftm 4-7bank*/
        rc = dpp_tm_qmu_init_set_12(dev_id, case_no_temp >> 4);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_init_set_12");
    }
    else if (case_no == 0xd)
    {
        /* 关闭rotation 配置8bank必须关掉，不然会出现crc错包*/
        //rc = dpp_mmu_init(0, 0, 0, 0, 0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_mmu_init");
        /*场景13：TM独享4组0-7bank,ddr编号来自传参,如0x6789 即代表独享6 7 8 
        9组ddr的全部bank*/
        rc = dpp_tm_qmu_init_set_13(dev_id, case_no_temp >> 4);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_init_set_13");
    }
    else if (case_no == 0xe)
    {
        /* 关闭rotation*/
        //rc = dpp_mmu_init(dev_id, 0, 0, 0, 0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_mmu_init");

        /* 场景14：每个tm只用1组*8bank，MMU实际分配ftm:0-9中指定组(0-7bank); etm:0-9中指定组(0-7bank) */
        rc = dpp_tm_qmu_init_set_14(dev_id, etm_ddr_no, ftm_ddr_no);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_init_set_8");
    }
    else if (case_no == 0xf)
    {
        /* 场景15：每个tm只用1组*4bank，MMU实际分配ftm:0-9中指定组(0123bank); etm:0-9中指定组(0123bank) */
        rc = dpp_tm_qmu_init_set_15(dev_id, etm_ddr_no, ftm_ddr_no);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_init_set_8");
    }
    else/* if (case_no == 0xef)*/
    {
        /* 关闭rotation*/
        //rc = dpp_mmu_init(0, 0, 0, 0, 0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_mmu_init");
        /* 场景15：每个tm只用2组*8bank，MMU实际分配ftm:0-9中指定组(0-7bank); etm:0-9中指定组(0-7bank) */
        rc = dpp_tm_qmu_init_set_16(dev_id, etm_ddr_no, ftm_ddr_no);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_init_set_16");
    }

    return DPP_OK;
}



/***********************************************************/
/** QMU/MMU初始化配置场景1：每个tm用8组*4bank，MMU实际分配ftm:0-7组(0145bank); etm:2-9组(2367bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/10/14
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_1(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 k = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 depth = 1024;
    ZXIC_UINT32 bank_to_mmu_cfg_map[4] = {2, 3, 6, 7};
    ZXIC_UINT32 qlist_grp0_bank_data[4] = {0, 1, 2, 3};


    DPP_ETM_QMU_QCFG_QLIST_BDEP_T qlist_bdep = {0};
    DPP_ETM_QMU_QCFG_QLIST_BHEAD_T qlist_bhead = {0};
    DPP_ETM_QMU_QCFG_QLIST_BTAIL_T qlist_btail = {0};

    DPP_ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFG_T active_to_bank_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_MMU_CFG_T ddr_in_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_QMU_CFG_T ddr_in_qmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_MMU_CFG_T bank_to_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_QMU_CFG_T bank_to_qmu_cfg = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP0_BANK_T qlist_grp0_bank = {0};

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_1 start========n");


    /* ddr组从qmu到mmu映射,0~7映射成2~9 */
    for (i = 0; i < 8; i++)
    {
        ddr_in_mmu_cfg.cfgmt_ddr_in_mmu_cfg = (i + 2);

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_MMU_CFGr,
                            0,
                            i,
                            &ddr_in_mmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    /* ddr组从mmu到qmu映射,2~9映射成0~7 */
    for (i = 2; i < 10; i++)
    {
        ddr_in_qmu_cfg.cfgmt_ddr_in_qmu_cfg = (i - 2);

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_QMU_CFGr,
                            0,
                            i,
                            &ddr_in_qmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }


    /* bank号从qmu到mmu映射，每组的bank0~3映射成bank2、3、6、7 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 4; i++)
        {
            k = j * 8 + i;

            bank_to_mmu_cfg.cfgmt_bank_in_mmu_cfg = bank_to_mmu_cfg_map[i];

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_MMU_CFGr,
                                0,
                                k,
                                &bank_to_mmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }


    /* bank号从mmu到qmu映射，每组的bank2、3、6、7映射成bank0、1、2、3 */
    for (j = 2; j < 10; j++)
    {
        for (i = 0; i < 4; i++)
        {
            ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id,  j * 8 , bank_to_mmu_cfg_map[i]);
            k = j * 8 + bank_to_mmu_cfg_map[i];

            bank_to_qmu_cfg.cfgmt_bank_in_qmu_cfg = i;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_QMU_CFGr,
                                0,
                                k,
                                &bank_to_qmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    /* random映射ram:配置一个chunk中，bank的轮询顺序 */
    for (i = 0; i < 16; i++)
    {


        active_to_bank_cfg.cfgmt_active_to_bank_cfg = i % 4;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFGr,
                            0,
                            i,
                            &active_to_bank_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    }


    /* 首尾深度指针配置 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 4; i++)
        {
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 4 + i) , depth);
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id,  (j * 4 + i + 1) , depth);
            k = j * 8 + i;
            qlist_bhead.bank_vld = 1;
            qlist_bhead.qcfg_qlist_bhead = (j * 4 + i) * depth;
            qlist_btail.qcfg_qlist_btail = ((j * 4 + i + 1) * depth - 1);
            qlist_bdep.qcfg_qlist_bdep = depth;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BHEADr,
                                0,
                                k,
                                &qlist_bhead);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BTAILr,
                                0,
                                k,
                                &qlist_btail);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BDEPr,
                                0,
                                k,
                                &qlist_bdep);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }


    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(ETM_QMU_QCFG_QLIST_GRP0_BANKr, 8);
    /* 随机表配置，决定每组bank的轮询顺序 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 64; i++)
        {
            qlist_grp0_bank.qcfg_qlist_grp0_bank_wr = qlist_grp0_bank_data[i % 4];
            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP0_BANKr + j,
                                0,
                                i,
                                &qlist_grp0_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    rc = dpp_tm_qmu_cfg_done_set(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_cfg_done_set");

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_1 END=======\n");

    return DPP_OK;
}




/***********************************************************/
/** QMU/MMU初始化配置场景1：每个tm用8组*4bank，MMU实际分配ftm:0-7组(0145bank); etm:2-9组(2367bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/10/14
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_1(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;

    /* etm mmu基址配置:ddr2-9组，bank号2367，bank深度1024 */
    //rc = dpp_mmu_bank_base_addr_set(dev_id, 0x3fc, 0xcc, 1024);
    //ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_mmu_bank_base_addr_set");

    rc = dpp_etm_qmu_init_set_1(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_etm_qmu_init_set");

    return DPP_OK;
}

/***********************************************************/
/** QMU/MMU初始化配置场景二：tm独享8组ddr:ftm为0-7组ddr的0123bank，etm为0-7组ddr 4567bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/10/16
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_2(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 k = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 depth = 1024;
    ZXIC_UINT32 qlist_grp0_bank_data[4] = {1, 2, 3, 0};

    DPP_ETM_QMU_QCFG_QLIST_BDEP_T qlist_bdep = {0};
    DPP_ETM_QMU_QCFG_QLIST_BHEAD_T qlist_bhead = {0};
    DPP_ETM_QMU_QCFG_QLIST_BTAIL_T qlist_btail = {0};

    DPP_ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFG_T active_to_bank_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_MMU_CFG_T ddr_in_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_QMU_CFG_T ddr_in_qmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_MMU_CFG_T bank_to_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_QMU_CFG_T bank_to_qmu_cfg = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP0_BANK_T qlist_grp0_bank = {0};


    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++dpp_etm_qmu_init_set starting++++++\n");


    /* ddr组从qmu到mmu映射,0~7映射成0~7 */
    for (i = 0; i < 8; i++)
    {
        ddr_in_mmu_cfg.cfgmt_ddr_in_mmu_cfg = i;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_MMU_CFGr,
                            0,
                            i,
                            &ddr_in_mmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    /* ddr组从mmu到qmu映射,0~7映射成0~7 */
    for (i = 0; i < 8; i++)
    {
        ddr_in_qmu_cfg.cfgmt_ddr_in_qmu_cfg = i;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_QMU_CFGr,
                            0,
                            i,
                            &ddr_in_qmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }


    /* bank号从qmu到mmu映射，每组bank0、1、2、3映射成bank4567，组0~7循环配置 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 4; i++)
        {
            k = j * 8 + i;
            bank_to_mmu_cfg.cfgmt_bank_in_mmu_cfg = (i + 4);

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_MMU_CFGr,
                                0,
                                k,
                                &bank_to_mmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }


    /* bank号从mmu到qmu映射，每组bank4567映射成bank0、1、2、3，组0~7循环配置 */
    for (j = 0; j < 8; j++)
    {
        for (i = 4; i < 8; i++)
        {

            k = j * 8 + i;
            bank_to_qmu_cfg.cfgmt_bank_in_qmu_cfg = (i - 4);

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_QMU_CFGr,
                                0,
                                k,
                                &bank_to_qmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    /* random映射ram:配置一个chunk中，bank的轮询顺序 */
    for (i = 0; i < 16; i++)
    {
        active_to_bank_cfg.cfgmt_active_to_bank_cfg = i % 4;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFGr,
                            0,
                            i,
                            &active_to_bank_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }


    /* 首尾深度指针配置:j为4组循环 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 4; i++)
        {
            k = j * 8 + i;
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 4 + i), depth);
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 4 + i + 1), depth);
            qlist_bhead.bank_vld = 1;
            qlist_bhead.qcfg_qlist_bhead = (j * 4 + i) * depth;
            qlist_btail.qcfg_qlist_btail = ((j * 4 + i + 1) * depth - 1);
            qlist_bdep.qcfg_qlist_bdep = depth;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BHEADr,
                                0,
                                k,
                                &qlist_bhead);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BTAILr,
                                0,
                                k,
                                &qlist_btail);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BDEPr,
                                0,
                                k,
                                &qlist_bdep);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(ETM_QMU_QCFG_QLIST_GRP0_BANKr, 8);
    /* 随机表配置，决定ddr内bank的轮询顺序:使用几组ddr需要配置几组 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 64; i++)
        {

            qlist_grp0_bank.qcfg_qlist_grp0_bank_wr = qlist_grp0_bank_data[i % 4];

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP0_BANKr + j,
                                0,
                                i,
                                &qlist_grp0_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        }
    }

    rc = dpp_tm_qmu_cfg_done_set(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_cfg_done_set");

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++dpp_etm_qmu_init_set end++++++\n");

    return DPP_OK;
}



/***********************************************************/
/** QMU/MMU初始化配置场景二：tm独享8组ddr:ftm为0-7组ddr的0123bank，etm为0-7组ddr 4567bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/10/16
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_2(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;

    /* etm mmu基址配置:ddr0-7组，bank号4567，bank深度1024 */
    //rc = dpp_mmu_bank_base_addr_set(dev_id, 0xff, 0xf0, 1024);
    //ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_mmu_bank_base_addr_set");

    rc = dpp_etm_qmu_init_set_2(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_etm_qmu_init_set");


    return DPP_OK;
}

/***********************************************************/
/** QMU初始化配置场景二：ddr3模式，tm看到4组*4bank，对应DDR分配4~7组，每组2~3,6~7bank
*    depth=64,代表16k
* @param   dev_id   设备编号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_pd16k_2(ZXIC_UINT32 dev_id, ZXIC_UINT32 depth)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 k = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 qlist_grp0_bank_data[4] = {1, 2, 3, 0};
    ZXIC_UINT32 ddr_num = 4;

    DPP_ETM_QMU_QCFG_QLIST_BDEP_T qlist_bdep = {0};
    DPP_ETM_QMU_QCFG_QLIST_BHEAD_T qlist_bhead = {0};
    DPP_ETM_QMU_QCFG_QLIST_BTAIL_T qlist_btail = {0};
    DPP_ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFG_T active_to_bank_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_MMU_CFG_T ddr_in_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_QMU_CFG_T ddr_in_qmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_MMU_CFG_T bank_to_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_QMU_CFG_T bank_to_qmu_cfg = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP0_BANK_T qlist_grp0_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP1_BANK_T qlist_grp1_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP2_BANK_T qlist_grp2_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP3_BANK_T qlist_grp3_bank = {0};

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++dpp_etm_qmu_init_set starting++++++\n");

    /* cfgmt配置4组ddr */
    rc = dpp_tm_cfgmt_ddr_attach_set(dev_id, ddr_num);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_ddr_attach_set");


    /* ddr组从qmu到mmu映射,0~3映射成4~7 */
    for (i = 0; i < 4; i++)
    {
        ddr_in_mmu_cfg.cfgmt_ddr_in_mmu_cfg = (i + 4);

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_MMU_CFGr,
                            0,
                            i,
                            &ddr_in_mmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    /* ddr组从mmu到qmu映射,4~7映射成0~3 */
    for (i = 4; i < 8; i++)
    {
        ddr_in_qmu_cfg.cfgmt_ddr_in_qmu_cfg = (i - 4);

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_QMU_CFGr,
                            0,
                            i,
                            &ddr_in_qmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }


    /* bank号从qmu到mmu映射，每组bank0、1、2、3映射成bank2、3、6、7，组0~3循环配置 */
    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 4; i++)
        {
            k = j * 8 + i;

            if (i < 2)
            {
                bank_to_mmu_cfg.cfgmt_bank_in_mmu_cfg = (i + 2);
            }
            else
            {
                bank_to_mmu_cfg.cfgmt_bank_in_mmu_cfg = (i + 4);
            }

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_MMU_CFGr,
                                0,
                                k,
                                &bank_to_mmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }


    /* bank号从mmu到qmu映射，每组bank2、3、6、7映射成bank0、1、2、3，组4~7循环配置 */
    for (j = 4; j < 8; j++)
    {
        for (i = 2; i < 6; i++)
        {
            if (i < 4)
            {
                k = j * 8 + i;
            }
            else
            {
                k = j * 8 + i + 2;
            }

            bank_to_qmu_cfg.cfgmt_bank_in_qmu_cfg = (i - 2);

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_QMU_CFGr,
                                0,
                                k,
                                &bank_to_qmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    /* random映射ram */
    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 4; i++)
        {
            k = j * 4 + i;

            active_to_bank_cfg.cfgmt_active_to_bank_cfg = i;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFGr,
                                0,
                                k,
                                &active_to_bank_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }


    /* 首尾深度指针配置:j为4组循环 */
    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 4; i++)
        {
            k = j * 8 + i;

            qlist_bhead.bank_vld = 1;
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 4 + i), depth);
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 4 + i + 1), depth);
            qlist_bhead.qcfg_qlist_bhead = (j * 4 + i) * depth;
            qlist_btail.qcfg_qlist_btail = ((j * 4 + i + 1) * depth - 1);
            qlist_bdep.qcfg_qlist_bdep = depth;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BHEADr,
                                0,
                                k,
                                &qlist_bhead);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BTAILr,
                                0,
                                k,
                                &qlist_btail);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BDEPr,
                                0,
                                k,
                                &qlist_bdep);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }


    /* 随机表配置，决定每组bank的轮询顺序 */
    for (j = 0; j < 16; j++)
    {
        for (i = 0; i < 4; i++)
        {
            k = j * 4 + i;

            qlist_grp0_bank.qcfg_qlist_grp0_bank_wr = qlist_grp0_bank_data[i];;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP0_BANKr,
                                0,
                                k,
                                &qlist_grp0_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        }
    }

    for (j = 0; j < 16; j++)
    {
        for (i = 0; i < 4; i++)
        {
            k = j * 4 + i;

            qlist_grp1_bank.qcfg_qlist_grp1_bank_wr = qlist_grp0_bank_data[i];;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP1_BANKr,
                                0,
                                k,
                                &qlist_grp1_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        }
    }

    for (j = 0; j < 16; j++)
    {
        for (i = 0; i < 4; i++)
        {
            k = j * 4 + i;

            qlist_grp2_bank.qcfg_qlist_grp2_bank_wr = qlist_grp0_bank_data[i];;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP2_BANKr,
                                0,
                                k,
                                &qlist_grp2_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        }
    }

    for (j = 0; j < 16; j++)
    {
        for (i = 0; i < 4; i++)
        {
            k = j * 4 + i;

            qlist_grp3_bank.qcfg_qlist_grp3_bank_wr = qlist_grp0_bank_data[i];;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP3_BANKr,
                                0,
                                k,
                                &qlist_grp3_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        }
    }

    rc = dpp_tm_qmu_cfg_done_set(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_cfg_done_set");

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++dpp_etm_qmu_init_set end++++++\n");

    return DPP_OK;
}


/***********************************************************/
/** QMU初始化配置场景二：ddr3模式，tm看到4组*4bank，对应DDR分配4~7组，每组2~3,6~7bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_pd16k_2(ZXIC_UINT32 dev_id, ZXIC_UINT32 depth)
{
    DPP_STATUS  rc = DPP_OK;

    rc = dpp_etm_qmu_init_set_pd16k_2(dev_id, depth);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_etm_qmu_init_set");

    return DPP_OK;
}


DPP_STATUS dpp_qmu_init_info(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 ddr_num = 0;
    ZXIC_UINT32 k = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    DPP_ETM_QMU_QCFG_QLIST_BDEP_T qlist_bdep = {0};
    DPP_ETM_QMU_QCFG_QLIST_BHEAD_T qlist_bhead = {0};
    DPP_ETM_QMU_QCFG_QLIST_BTAIL_T qlist_btail = {0};
    DPP_ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFG_T active_to_bank_cfg = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP0_BANK_T qlist_grp0_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP1_BANK_T qlist_grp1_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP2_BANK_T qlist_grp2_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP3_BANK_T qlist_grp3_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP4_BANK_T qlist_grp4_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP5_BANK_T qlist_grp5_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP6_BANK_T qlist_grp6_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP7_BANK_T qlist_grp7_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP_T qcfg_qlist_grp = {0};
    dpp_tm_cfgmt_ddr_attach_get(0, &ddr_num);
    ZXIC_COMM_PRINT("dpp_tm_cfgmt_ddr_attach_get ddr_num:%d\n",ddr_num);

    /* random映射ram */
    for (j = 0; j < 4; j++)
    {
        for (i = 0, k = 0; i < 4; i++)
        {
            k = j * 4 + i;

            active_to_bank_cfg.cfgmt_active_to_bank_cfg = 0;

            rc  = dpp_reg_read(dev_id,
                                ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFGr,
                                0,
                                k,
                                &active_to_bank_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
            ZXIC_COMM_PRINT("cfgmt_active_to_bank_cfg: cfgmt_active_to_bank_cfg-%d value:%d\n",k,active_to_bank_cfg.cfgmt_active_to_bank_cfg);
        }
    }

    /* 首尾深度指针配置: */
    for (j = 0, k = 0; j < 8; j++)
    {
        k = j * 8;
        rc  = dpp_reg_read(dev_id,
                            ETM_QMU_QCFG_QLIST_BHEADr,
                            0,
                            k,
                            &qlist_bhead);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
        ZXIC_COMM_PRINT("qlist_bhead: qlist_bhead-%d bank_vld:%d qcfg_qlist_bhead:%d\n",k,qlist_bhead.bank_vld,qlist_bhead.qcfg_qlist_bhead);

        rc  = dpp_reg_read(dev_id,
                            ETM_QMU_QCFG_QLIST_BTAILr,
                            0,
                            k,
                            &qlist_btail);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
        ZXIC_COMM_PRINT("qlist_btail: qlist_btail-%d qcfg_qlist_btail:%d\n",k,qlist_btail.qcfg_qlist_btail);

        
    }
    for (j = 0; j < 64; j++)
    {
        
        rc  = dpp_reg_read(dev_id,
                            ETM_QMU_QCFG_QLIST_BDEPr,
                            0,
                            j,
                            &qlist_bdep);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
        ZXIC_COMM_PRINT("qlist_bdep: qlist_bdep-%d qcfg_qlist_bdep:%d\n",j,qlist_bdep.qcfg_qlist_bdep);
    }

    /* 随机表配置，决定每组bank的轮询顺序 */
    for (j = 0; j < 16; j++)
    {
        for (i = 0, k = 0; i < 4; i++)
        {
            k = j * 4 + i;

            rc  = dpp_reg_read(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP0_BANKr,
                                0,
                                k,
                                &qlist_grp0_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
            ZXIC_COMM_PRINT("qlist_grp0_bank: qlist_grp0_bank-%d qcfg_qlist_grp0_bank_wr:%d\n",k,qlist_grp0_bank.qcfg_qlist_grp0_bank_wr);
        }
    }

    for (j = 0; j < 16; j++)
    {
        for (i = 0, k = 0; i < 4; i++)
        {
            k = j * 4 + i;

            rc  = dpp_reg_read(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP1_BANKr,
                                0,
                                k,
                                &qlist_grp1_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
            ZXIC_COMM_PRINT("qlist_grp1_bank: qlist_grp1_bank-%d qcfg_qlist_grp1_bank_wr:%d\n",k,qlist_grp1_bank.qcfg_qlist_grp1_bank_wr);

        }
    }

    for (j = 0; j < 16; j++)
    {
        for (i = 0, k = 0; i < 4; i++)
        {
            k = j * 4 + i;

            rc  = dpp_reg_read(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP2_BANKr,
                                0,
                                k,
                                &qlist_grp2_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
            ZXIC_COMM_PRINT("qlist_grp2_bank: qlist_grp2_bank-%d qcfg_qlist_grp2_bank_wr:%d\n",k,qlist_grp2_bank.qcfg_qlist_grp2_bank_wr);

        }
    }

    for (j = 0; j < 16; j++)
    {
        for (i = 0, k = 0; i < 4; i++)
        {
            k = j * 4 + i;

            rc  = dpp_reg_read(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP3_BANKr,
                                0,
                                k,
                                &qlist_grp3_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
            ZXIC_COMM_PRINT("qlist_grp3_bank: qlist_grp3_bank-%d qcfg_qlist_grp3_bank_wr:%d\n",k,qlist_grp3_bank.qcfg_qlist_grp3_bank_wr);

        }
    }

    for (j = 0; j < 16; j++)
    {
        for (i = 0, k = 0; i < 4; i++)
        {
            k = j * 4 + i;

            rc  = dpp_reg_read(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP4_BANKr,
                                0,
                                k,
                                &qlist_grp4_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
            ZXIC_COMM_PRINT("qlist_grp4_bank: qlist_grp4_bank-%d qcfg_qlist_grp4_bank_wr:%d\n",k,qlist_grp4_bank.qcfg_qlist_grp4_bank_wr);

        }
    }

    for (j = 0; j < 16; j++)
    {
        for (i = 0, k = 0; i < 4; i++)
        {
            k = j * 4 + i;

            rc  = dpp_reg_read(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP5_BANKr,
                                0,
                                k,
                                &qlist_grp5_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
            ZXIC_COMM_PRINT("qlist_grp5_bank: qlist_grp5_bank-%d qcfg_qlist_grp5_bank_wr:%d\n",k,qlist_grp5_bank.qcfg_qlist_grp5_bank_wr);

        }
    }

    for (j = 0; j < 16; j++)
    {
        for (i = 0, k = 0; i < 4; i++)
        {
            k = j * 4 + i;

            rc  = dpp_reg_read(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP6_BANKr,
                                0,
                                k,
                                &qlist_grp6_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
            ZXIC_COMM_PRINT("qlist_grp6_bank: qlist_grp6_bank-%d qcfg_qlist_grp6_bank_wr:%d\n",k,qlist_grp6_bank.qcfg_qlist_grp6_bank_wr);

        }
    }

    for (j = 0; j < 16; j++)
    {
        for (i = 0, k = 0; i < 4; i++)
        {
            k = j * 4 + i;

            rc  = dpp_reg_read(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP7_BANKr,
                                0,
                                k,
                                &qlist_grp7_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
            ZXIC_COMM_PRINT("qlist_grp7_bank: qlist_grp7_bank-%d qcfg_qlist_grp7_bank_wr:%d\n",k,qlist_grp7_bank.qcfg_qlist_grp7_bank_wr);

        }
    }

    for (j = 0; j < 8; j++)     
    {
        for (i = 0, k = 0; i < 8; i++)
        {
            k = j * 8 + i;
 
            rc  = dpp_reg_read(dev_id,
                            ETM_QMU_QCFG_QLIST_GRPr,
                            0,
                            k,
                            &qcfg_qlist_grp);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
            ZXIC_COMM_PRINT("qcfg_qlist_grp: qcfg_qlist_grp-%d qcfg_qlist_grp_wr:%d\n",k,qcfg_qlist_grp.qcfg_qlist_grp_wr);
        }
    }


    return DPP_OK;
}
/***********************************************************/
/** QMU初始化配置场景二：ddr3模式，8组*8bank
*    depth=512,代表32k
* @param   dev_id   设备编号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  sun      @date  2023/04/12
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_chuk32(ZXIC_UINT32 dev_id, ZXIC_UINT32 depth)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 k = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 qlist_grp0_bank_data[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    ZXIC_UINT32 qlist_grp_bank_data[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    ZXIC_UINT32 ddr_num = 0xff;

    DPP_ETM_QMU_QCFG_QLIST_BDEP_T qlist_bdep = {0};
    DPP_ETM_QMU_QCFG_QLIST_BHEAD_T qlist_bhead = {0};
    DPP_ETM_QMU_QCFG_QLIST_BTAIL_T qlist_btail = {0};
    DPP_ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFG_T active_to_bank_cfg = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP0_BANK_T qlist_grp0_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP1_BANK_T qlist_grp1_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP2_BANK_T qlist_grp2_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP3_BANK_T qlist_grp3_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP4_BANK_T qlist_grp4_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP5_BANK_T qlist_grp5_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP6_BANK_T qlist_grp6_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP7_BANK_T qlist_grp7_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP_T qcfg_qlist_grp = {0};
    


    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++dpp_etm_qmu_init_set starting++++++\n");

    /* cfgmt配置8组ddr */
    rc = dpp_tm_cfgmt_ddr_attach_set(dev_id, ddr_num);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_ddr_attach_set");

    /* 首尾深度指针配置: */
    for (j = 0, k = 0; j < 8; j++)
    {
        
        k = j * 8;
        qlist_bhead.bank_vld = 1;
        qlist_bhead.qcfg_qlist_bhead = j * depth;
        qlist_btail.qcfg_qlist_btail = 511 + j * depth;
        

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_BHEADr,
                            0,
                            k,
                            &qlist_bhead);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_BTAILr,
                            0,
                            k,
                            &qlist_btail);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        
    }
    for (j = 0; j < 64; j++)
    {
        qlist_bdep.qcfg_qlist_bdep = depth;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_BDEPr,
                            0,
                            j,
                            &qlist_bdep);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    /* 随机表配置，决定每组bank的轮询顺序 */
    for (j = 0; j < 16; j++)
    {
        for (i = 0, k = 0; i < 4; i++)
        {
            k = j * 4 + i;

            qlist_grp0_bank.qcfg_qlist_grp0_bank_wr = qlist_grp0_bank_data[0];

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP0_BANKr,
                                0,
                                k,
                                &qlist_grp0_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        }
    }

    for (j = 0; j < 16; j++)
    {
        for (i = 0, k = 0; i < 4; i++)
        {
            k = j * 4 + i;

            qlist_grp1_bank.qcfg_qlist_grp1_bank_wr = qlist_grp0_bank_data[1];;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP1_BANKr,
                                0,
                                k,
                                &qlist_grp1_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        }
    }

    for (j = 0; j < 16; j++)
    {
        for (i = 0, k = 0; i < 4; i++)
        {
            k = j * 4 + i;

            qlist_grp2_bank.qcfg_qlist_grp2_bank_wr = qlist_grp0_bank_data[2];;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP2_BANKr,
                                0,
                                k,
                                &qlist_grp2_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        }
    }

    for (j = 0; j < 16; j++)
    {
        for (i = 0, k = 0; i < 4; i++)
        {
            k = j * 4 + i;

            qlist_grp3_bank.qcfg_qlist_grp3_bank_wr = qlist_grp0_bank_data[3];;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP3_BANKr,
                                0,
                                k,
                                &qlist_grp3_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        }
    }

    for (j = 0; j < 16; j++)
    {
        for (i = 0, k = 0; i < 4; i++)
        {
            k = j * 4 + i;

            qlist_grp4_bank.qcfg_qlist_grp4_bank_wr = qlist_grp0_bank_data[4];;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP4_BANKr,
                                0,
                                k,
                                &qlist_grp4_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        }
    }

    for (j = 0; j < 16; j++)
    {
        for (i = 0, k = 0; i < 4; i++)
        {
            k = j * 4 + i;

            qlist_grp5_bank.qcfg_qlist_grp5_bank_wr = qlist_grp0_bank_data[5];;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP5_BANKr,
                                0,
                                k,
                                &qlist_grp5_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        }
    }

    for (j = 0; j < 16; j++)
    {
        for (i = 0, k = 0; i < 4; i++)
        {
            k = j * 4 + i;

            qlist_grp6_bank.qcfg_qlist_grp6_bank_wr = qlist_grp0_bank_data[6];;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP6_BANKr,
                                0,
                                k,
                                &qlist_grp6_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        }
    }

    for (j = 0; j < 16; j++)
    {
        for (i = 0, k = 0; i < 4; i++)
        {
            k = j * 4 + i;

            qlist_grp7_bank.qcfg_qlist_grp7_bank_wr = qlist_grp0_bank_data[7];;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP7_BANKr,
                                0,
                                k,
                                &qlist_grp7_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        }
    }

    for (j = 0; j < 8; j++)     
    {
        for (i = 0, k = 0; i < 8; i++)
        {
            k = j * 8 + i;
            qcfg_qlist_grp.qcfg_qlist_grp_wr = qlist_grp_bank_data[i];
            rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_GRPr,
                            0,
                            k,
                            &qcfg_qlist_grp);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

        /* random映射ram */
    for (j = 0; j < 4; j++)
    {
        for (i = 0, k = 0; i < 4; i++)
        {
            
            k = j * 4 + i;

            active_to_bank_cfg.cfgmt_active_to_bank_cfg = 0;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFGr,
                                0,
                                k,
                                &active_to_bank_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }
    zxic_comm_sleep(5);
    rc = dpp_tm_qmu_cfg_done_set(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_cfg_done_set");

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++dpp_etm_qmu_init_set end++++++\n");

    return DPP_OK;
}

/***********************************************************/
/** QMU初始化配置场景：ddr3模式，8组ddr*8bank
* @param   dev_id   设备编号
* @param   depth   bank depth
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  sun      @date  2023/04/12
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_chuk32(ZXIC_UINT32 dev_id, ZXIC_UINT32 depth)
{
    DPP_STATUS  rc = DPP_OK;

    rc = dpp_etm_qmu_init_set_chuk32(dev_id, depth);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_etm_qmu_init_set");

    return DPP_OK;
}

/***********************************************************/
/** QMU初始化配置场景二：TM独享8组ddr:FTM为0-3组ddr的0~3bank，ETM为4-7组ddr 0~3bank
* @param   dev_id   设备编号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_3(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 k = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 depth = 2048;
    ZXIC_UINT32 qlist_grp0_bank_data[8] = {0, 1, 2, 3, 4, 5, 6, 7};


    DPP_ETM_QMU_QCFG_QLIST_BDEP_T qlist_bdep = {0};
    DPP_ETM_QMU_QCFG_QLIST_BHEAD_T qlist_bhead = {0};
    DPP_ETM_QMU_QCFG_QLIST_BTAIL_T qlist_btail = {0};
    DPP_ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFG_T active_to_bank_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_MMU_CFG_T ddr_in_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_QMU_CFG_T ddr_in_qmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_MMU_CFG_T bank_to_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_QMU_CFG_T bank_to_qmu_cfg = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP0_BANK_T qlist_grp0_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP_T qcfg_qlist_grp = {0};

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++dpp_etm_qmu_init_set starting++++++\n");


    /* ddr组从qmu到mmu映射,0~3映射成4~7 */
    for (i = 0; i < 4; i++)
    {
        ddr_in_mmu_cfg.cfgmt_ddr_in_mmu_cfg = i + 4;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_MMU_CFGr,
                            0,
                            i,
                            &ddr_in_mmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    /* ddr组从mmu到qmu映射,4~7映射成0~3 */
    for (i = 4; i < 8; i++)
    {
        ddr_in_qmu_cfg.cfgmt_ddr_in_qmu_cfg = i - 4;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_QMU_CFGr,
                            0,
                            i,
                            &ddr_in_qmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    /* bank号从qmu到mmu映射，组0~3的bank0-3映射成bank0-3 */
    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 4; i++)
        {
            k = j * 8 + i;

            bank_to_mmu_cfg.cfgmt_bank_in_mmu_cfg = i;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_MMU_CFGr,
                                0,
                                k,
                                &bank_to_mmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }


    /* bank号从mmu到qmu映射，组4~7的bank0-7映射成bank0-7 */
    for (j = 4; j < 8; j++)
    {
        for (i = 0; i < 4; i++)
        {
            k = j * 8 + i;

            bank_to_qmu_cfg.cfgmt_bank_in_qmu_cfg = i;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_QMU_CFGr,
                                0,
                                k,
                                &bank_to_qmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    /* random映射ram:配置一个chunk中，bank的轮询顺序 */
    for (i = 0; i < 16; i++)
    {
        active_to_bank_cfg.cfgmt_active_to_bank_cfg = i % 4;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFGr,
                            0,
                            i,
                            &active_to_bank_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    /* 首尾深度指针配置:j为8组循环 */
    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 4; i++)
        {
            k = j * 8 + i;

            qlist_bhead.bank_vld = 1;
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 4 + i), depth);
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 4 + i + 1), depth);
            qlist_bhead.qcfg_qlist_bhead = (j * 4 + i) * depth;
            qlist_btail.qcfg_qlist_btail = ((j * 4 + i + 1) * depth - 1);
            qlist_bdep.qcfg_qlist_bdep = depth;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BHEADr,
                                0,
                                k,
                                &qlist_bhead);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BTAILr,
                                0,
                                k,
                                &qlist_btail);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BDEPr,
                                0,
                                k,
                                &qlist_bdep);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(ETM_QMU_QCFG_QLIST_GRP0_BANKr, 8);
    /* 随机表配置，决定ddr内bank的轮询顺序:使用几组ddr需要配置几组 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 64; i++)
        {

            qlist_grp0_bank.qcfg_qlist_grp0_bank_wr = qlist_grp0_bank_data[i % 4];

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP0_BANKr + j,
                                0,
                                i,
                                &qlist_grp0_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        }
    }

    for (j = 0; j < 64; j++)
    {
        qcfg_qlist_grp.qcfg_qlist_grp_wr = j % 4;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_GRPr,
                            0,
                            j,
                            &qcfg_qlist_grp);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    rc = dpp_tm_qmu_cfg_done_set(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_cfg_done_set");

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++dpp_etm_qmu_init_set end++++++\n");

    return DPP_OK;
}




/***********************************************************/
/** QMU/MMU初始化配置场景三：TM独享8组ddr:FTM为0-3组ddr的0~3bank，ETM为4-7组ddr 0~3bank
*   MMU开启rotatjon
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_3(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_RANDOM_BYPASS_EN_T etm_bypass = {0};
//    DPP_FTM_QMU_RANDOM_BYPASS_EN_T ftm_bypass = {0};

    /* etm mmu基址配置:ddr4-7组，bank号0-3，bank深度2048 */
    //rc = dpp_mmu_bank_base_addr_set(dev_id, 0xf0, 0xf, 2048);
    //ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_mmu_bank_base_addr_set");

    rc = dpp_etm_qmu_init_set_3(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_etm_qmu_init_set_1");

    /* 关闭etm映射配置 */
    etm_bypass.random_bypass_en = 0;
    rc  = dpp_reg_write(dev_id,
                        ETM_QMU_RANDOM_BYPASS_ENr,
                        0,
                        0,
                        &etm_bypass);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");


    return DPP_OK;
}


/***********************************************************/
/**场景4：TM共享2组ddr:FTM为2、4组ddr的01bank，ETM为2、4组ddr 23bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/06/06
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_4(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 k = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 depth = 8 * 1024;
    ZXIC_UINT32 ddr_to_mmu_cfg_map[2] = {2, 4};
    ZXIC_UINT32 bank_to_mmu_cfg_map[2] = {2, 3};
    ZXIC_UINT32 qlist_grp0_bank_data[2] = {0, 1};


    DPP_ETM_QMU_QCFG_QLIST_BDEP_T qlist_bdep = {0};
    DPP_ETM_QMU_QCFG_QLIST_BHEAD_T qlist_bhead = {0};
    DPP_ETM_QMU_QCFG_QLIST_BTAIL_T qlist_btail = {0};

    DPP_ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFG_T active_to_bank_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_MMU_CFG_T ddr_in_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_QMU_CFG_T ddr_in_qmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_MMU_CFG_T bank_to_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_QMU_CFG_T bank_to_qmu_cfg = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP0_BANK_T qlist_grp0_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP_T qcfg_qlist_grp = {0};

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_4 start========n");


    /* ddr组从qmu到mmu映射,0~1映射成2 4 */
    for (i = 0; i < 2; i++)
    {
        ddr_in_mmu_cfg.cfgmt_ddr_in_mmu_cfg = ddr_to_mmu_cfg_map[i];

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_MMU_CFGr,
                            0,
                            i,
                            &ddr_in_mmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    /* ddr组从mmu到qmu映射,2 4映射成0~1 */
    for (i = 0; i < 2; i++)
    {
        ddr_in_qmu_cfg.cfgmt_ddr_in_qmu_cfg = i;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_QMU_CFGr,
                            0,
                            ddr_to_mmu_cfg_map[i],
                            &ddr_in_qmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }


    /* bank号从qmu到mmu映射，每组的bank0~1映射成bank2、3 */
    for (j = 0; j < 2; j++)
    {
        for (i = 0; i < 2; i++)
        {
            k = j * 8 + i;

            bank_to_mmu_cfg.cfgmt_bank_in_mmu_cfg = bank_to_mmu_cfg_map[i];

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_MMU_CFGr,
                                0,
                                k,
                                &bank_to_mmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }


    /* bank号从mmu到qmu映射，每组的bank2、3映射成bank0、1 */
    for (j = 0; j < 2; j++)
    {
        for (i = 0; i < 2; i++)
        {
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, ddr_to_mmu_cfg_map[j] , 8);
            ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id,  ddr_to_mmu_cfg_map[j] * 8 , bank_to_mmu_cfg_map[i]);
            k = ddr_to_mmu_cfg_map[j] * 8 + bank_to_mmu_cfg_map[i];
            bank_to_qmu_cfg.cfgmt_bank_in_qmu_cfg = i;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_QMU_CFGr,
                                0,
                                k,
                                &bank_to_qmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    /* random映射ram:配置一个chunk中，bank的轮询顺序 */
    for (i = 0; i < 16; i++)
    {


        active_to_bank_cfg.cfgmt_active_to_bank_cfg = i % 2;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFGr,
                            0,
                            i,
                            &active_to_bank_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    }


    /* 首尾深度指针配置 */
    for (j = 0; j < 2; j++)
    {
        for (i = 0; i < 2; i++)
        {
            k = j * 8 + i;

            qlist_bhead.bank_vld = 1;
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 2 + i), depth);
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 2 + i + 1), depth);
            qlist_bhead.qcfg_qlist_bhead = (j * 2 + i) * depth;
            qlist_btail.qcfg_qlist_btail = ((j * 2 + i + 1) * depth - 1);
            qlist_bdep.qcfg_qlist_bdep = depth;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BHEADr,
                                0,
                                k,
                                &qlist_bhead);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BTAILr,
                                0,
                                k,
                                &qlist_btail);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BDEPr,
                                0,
                                k,
                                &qlist_bdep);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    for (i = 0; i < 64; i++)
    {
        qlist_bdep.qcfg_qlist_bdep = depth;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_BDEPr,
                            0,
                            i,
                            &qlist_bdep);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(ETM_QMU_QCFG_QLIST_GRP0_BANKr, 8);
    /* 随机表配置，决定每组bank的轮询顺序 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 64; i++)
        {
            qlist_grp0_bank.qcfg_qlist_grp0_bank_wr = qlist_grp0_bank_data[i % 2];
            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP0_BANKr + j,
                                0,
                                i,
                                &qlist_grp0_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    for (j = 0; j < 64; j++)
    {
        qcfg_qlist_grp.qcfg_qlist_grp_wr = j % 2;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_GRPr,
                            0,
                            j,
                            &qcfg_qlist_grp);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    rc = dpp_tm_qmu_cfg_done_set(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_cfg_done_set");

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_4 END=======\n");

    return DPP_OK;
}




/***********************************************************/
/**场景4：TM共享2组ddr:FTM为2、4组ddr的01bank，ETM为2、4组ddr 23bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/06/06
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_4(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_RANDOM_BYPASS_EN_T etm_bypass = {0};
    //DPP_FTM_QMU_RANDOM_BYPASS_EN_T ftm_bypass = {0};

    /* etm mmu基址配置:ddr2、4组，bank号23，bank深度8*1024 */
    //rc = dpp_mmu_bank_base_addr_set(dev_id, 0x14, 0xc, 8 * 1024);
    //ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_mmu_bank_base_addr_set");

    rc = dpp_etm_qmu_init_set_4(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_etm_qmu_init_set_4");

    /* 关闭etm映射配置 */
    etm_bypass.random_bypass_en = 0;
    rc  = dpp_reg_write(dev_id,
                        ETM_QMU_RANDOM_BYPASS_ENr,
                        0,
                        0,
                        &etm_bypass);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 场景5：TM共享4组ddr:FTM为1234组ddr的01bank，ETM为1234组ddr 23bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/06/06
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_5(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 k = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 depth = 4*1024;
    ZXIC_UINT32 bank_to_mmu_cfg_map[2] = {2, 3};
    ZXIC_UINT32 qlist_grp0_bank_data[2] = {0, 1};
    ZXIC_UINT32 ddr_no[4] = {1, 2, 3, 4};


    DPP_ETM_QMU_QCFG_QLIST_BDEP_T qlist_bdep = {0};
    DPP_ETM_QMU_QCFG_QLIST_BHEAD_T qlist_bhead = {0};
    DPP_ETM_QMU_QCFG_QLIST_BTAIL_T qlist_btail = {0};

    DPP_ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFG_T active_to_bank_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_MMU_CFG_T ddr_in_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_QMU_CFG_T ddr_in_qmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_MMU_CFG_T bank_to_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_QMU_CFG_T bank_to_qmu_cfg = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP0_BANK_T qlist_grp0_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP_T qcfg_qlist_grp = {0};

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_5 start========n");


    /* ddr组从qmu到mmu映射,0~3映射成1234 */
    for (i = 0; i < 4; i++)
    {
        ddr_in_mmu_cfg.cfgmt_ddr_in_mmu_cfg = ddr_no[i];

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_MMU_CFGr,
                            0,
                            i,
                            &ddr_in_mmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    /* ddr组从mmu到qmu映射,1234映射成0~3 */
    for (i = 0; i < 4; i++)
    {
        ddr_in_qmu_cfg.cfgmt_ddr_in_qmu_cfg = i;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_QMU_CFGr,
                            0,
                            ddr_no[i],
                            &ddr_in_qmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }


    /* bank号从qmu到mmu映射，每组的bank0~1映射成bank2、3 */
    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 2; i++)
        {
            k = j * 8 + i;

            bank_to_mmu_cfg.cfgmt_bank_in_mmu_cfg = bank_to_mmu_cfg_map[i];

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_MMU_CFGr,
                                0,
                                k,
                                &bank_to_mmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }


    /* bank号从mmu到qmu映射，每组的bank2、3映射成bank0、1 */
    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 2; i++)
        {
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, ddr_no[j] , 8);
            ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id,  ddr_no[j] * 8 , bank_to_mmu_cfg_map[i]);
            k = ddr_no[j] * 8 + bank_to_mmu_cfg_map[i];
            bank_to_qmu_cfg.cfgmt_bank_in_qmu_cfg = i;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_QMU_CFGr,
                                0,
                                k,
                                &bank_to_qmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    /* random映射ram:配置一个chunk中，bank的轮询顺序 */
    for (i = 0; i < 16; i++)
    {


        active_to_bank_cfg.cfgmt_active_to_bank_cfg = i % 2;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFGr,
                            0,
                            i,
                            &active_to_bank_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    }


    /* 首尾深度指针配置 */
    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 2; i++)
        {
            k = j * 8 + i;

            qlist_bhead.bank_vld = 1;
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 2 + i), depth);
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 2 + i + 1), depth);
            qlist_bhead.qcfg_qlist_bhead = (j * 2 + i) * depth;
            qlist_btail.qcfg_qlist_btail = ((j * 2 + i + 1) * depth - 1);
            qlist_bdep.qcfg_qlist_bdep = depth;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BHEADr,
                                0,
                                k,
                                &qlist_bhead);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BTAILr,
                                0,
                                k,
                                &qlist_btail);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BDEPr,
                                0,
                                k,
                                &qlist_bdep);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    for (i = 0; i < 64; i++)
    {
        qlist_bdep.qcfg_qlist_bdep = depth;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_BDEPr,
                            0,
                            i,
                            &qlist_bdep);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(ETM_QMU_QCFG_QLIST_GRP0_BANKr, 8);
    /* 随机表配置，决定每组bank的轮询顺序 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 64; i++)
        {
            qlist_grp0_bank.qcfg_qlist_grp0_bank_wr = qlist_grp0_bank_data[i % 2];
            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP0_BANKr + j,
                                0,
                                i,
                                &qlist_grp0_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    for (j = 0; j < 64; j++)
    {
        qcfg_qlist_grp.qcfg_qlist_grp_wr = j % 4;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_GRPr,
                            0,
                            j,
                            &qcfg_qlist_grp);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    rc = dpp_tm_qmu_cfg_done_set(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_cfg_done_set");

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_5 END=======\n");

    return DPP_OK;
}


/***********************************************************/
/** 场景5：TM共享4组ddr:FTM为1234组ddr的01bank，ETM为1234组ddr 23bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/06/06
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_5(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;

    /* etm mmu基址配置:ddr1234组，bank号23，bank深度4*1024 */
    //rc = dpp_mmu_bank_base_addr_set(dev_id, 0x1e, 0xc, 4*1024);
    //ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_mmu_bank_base_addr_set");

    rc = dpp_etm_qmu_init_set_5(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_etm_qmu_init_set_5");

    return DPP_OK;
}


/***********************************************************/
/** QMU初始化配置场景6：4组*2bank，MMU实际分配ftm:4,6,7,9组(01bank); etm:4,6,7,9组(23bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/05/13
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_6(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 k = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 depth = 4*1024;
    ZXIC_UINT32 bank_to_mmu_cfg_map[4] = {2, 3};
    ZXIC_UINT32 qlist_grp0_bank_data[4] = {0, 1};
    ZXIC_UINT32 ddr_no[4] = {4, 6, 7, 9};


    DPP_ETM_QMU_QCFG_QLIST_BDEP_T qlist_bdep = {0};
    DPP_ETM_QMU_QCFG_QLIST_BHEAD_T qlist_bhead = {0};
    DPP_ETM_QMU_QCFG_QLIST_BTAIL_T qlist_btail = {0};

    DPP_ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFG_T active_to_bank_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_MMU_CFG_T ddr_in_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_QMU_CFG_T ddr_in_qmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_MMU_CFG_T bank_to_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_QMU_CFG_T bank_to_qmu_cfg = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP0_BANK_T qlist_grp0_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP_T qcfg_qlist_grp = {0};

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_1 start========n");


    /* ddr组从qmu到mmu映射,0~3映射成4679 */
    for (i = 0; i < 4; i++)
    {
        ddr_in_mmu_cfg.cfgmt_ddr_in_mmu_cfg = ddr_no[i];

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_MMU_CFGr,
                            0,
                            i,
                            &ddr_in_mmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    /* ddr组从mmu到qmu映射,4679映射成0~3 */
    for (i = 0; i < 4; i++)
    {
        ddr_in_qmu_cfg.cfgmt_ddr_in_qmu_cfg = i;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_QMU_CFGr,
                            0,
                            ddr_no[i],
                            &ddr_in_qmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }


    /* bank号从qmu到mmu映射，每组的bank0~1映射成bank2、3 */
    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 2; i++)
        {
            k = j * 8 + i;

            bank_to_mmu_cfg.cfgmt_bank_in_mmu_cfg = bank_to_mmu_cfg_map[i];

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_MMU_CFGr,
                                0,
                                k,
                                &bank_to_mmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }


    /* bank号从mmu到qmu映射，每组的bank2、3映射成bank0、1 */
    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 2; i++)
        {
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, ddr_no[j] , 8);
            ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id,  ddr_no[j] * 8 , bank_to_mmu_cfg_map[i]);
            k = ddr_no[j] * 8 + bank_to_mmu_cfg_map[i];
            bank_to_qmu_cfg.cfgmt_bank_in_qmu_cfg = i;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_QMU_CFGr,
                                0,
                                k,
                                &bank_to_qmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    /* random映射ram:配置一个chunk中，bank的轮询顺序 */
    for (i = 0; i < 16; i++)
    {


        active_to_bank_cfg.cfgmt_active_to_bank_cfg = i % 2;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFGr,
                            0,
                            i,
                            &active_to_bank_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    }


    /* 首尾深度指针配置 */
    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 2; i++)
        {
            k = j * 8 + i;

            qlist_bhead.bank_vld = 1;
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 2 + i), depth);
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 2 + i + 1), depth);
            qlist_bhead.qcfg_qlist_bhead = (j * 2 + i) * depth;
            qlist_btail.qcfg_qlist_btail = ((j * 2 + i + 1) * depth - 1);
            qlist_bdep.qcfg_qlist_bdep = depth;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BHEADr,
                                0,
                                k,
                                &qlist_bhead);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BTAILr,
                                0,
                                k,
                                &qlist_btail);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BDEPr,
                                0,
                                k,
                                &qlist_bdep);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    for (i = 0; i < 64; i++)
    {
        qlist_bdep.qcfg_qlist_bdep = depth;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_BDEPr,
                            0,
                            i,
                            &qlist_bdep);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(ETM_QMU_QCFG_QLIST_GRP0_BANKr, 8);
    /* 随机表配置，决定每组bank的轮询顺序 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 64; i++)
        {
            qlist_grp0_bank.qcfg_qlist_grp0_bank_wr = qlist_grp0_bank_data[i % 2];
            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP0_BANKr + j,
                                0,
                                i,
                                &qlist_grp0_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    for (j = 0; j < 64; j++)
    {
        qcfg_qlist_grp.qcfg_qlist_grp_wr = j % 4;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_GRPr,
                            0,
                            j,
                            &qcfg_qlist_grp);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    rc = dpp_tm_qmu_cfg_done_set(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_cfg_done_set");

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_1 END=======\n");

    return DPP_OK;
}



/***********************************************************/
/** QMU初始化配置场景6：4组*2bank，MMU实际分配ftm:4,6,7,9组(01bank); etm:4,6,7,9组(23bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/05/13
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_6(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;

    /* etm mmu基址配置:ddr4,6,7,9组，bank号01，bank深度4*1024 */
    //rc = dpp_mmu_bank_base_addr_set(dev_id, 0x2d0, 0xc, 4*1024);
    //ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_mmu_bank_base_addr_set");

    rc = dpp_etm_qmu_init_set_6(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_etm_qmu_init_set");

    return DPP_OK;
}


/***********************************************************/
/** QMU/MMU初始化配置场景7：每个tm用8组*4bank，MMU实际分配ftm:2-9组(0145bank); etm:2-9组(2367bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/10/14
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_7(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 k = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 depth = 1024;
    ZXIC_UINT32 bank_to_mmu_cfg_map[4] = {2, 3, 6, 7};
    ZXIC_UINT32 qlist_grp0_bank_data[4] = {0, 1, 2, 3};


    DPP_ETM_QMU_QCFG_QLIST_BDEP_T qlist_bdep = {0};
    DPP_ETM_QMU_QCFG_QLIST_BHEAD_T qlist_bhead = {0};
    DPP_ETM_QMU_QCFG_QLIST_BTAIL_T qlist_btail = {0};

    DPP_ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFG_T active_to_bank_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_MMU_CFG_T ddr_in_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_QMU_CFG_T ddr_in_qmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_MMU_CFG_T bank_to_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_QMU_CFG_T bank_to_qmu_cfg = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP0_BANK_T qlist_grp0_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP_T qcfg_qlist_grp = {0};

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_1 start========n");


    /* ddr组从qmu到mmu映射,0~7映射成2~9 */
    for (i = 0; i < 8; i++)
    {
        ddr_in_mmu_cfg.cfgmt_ddr_in_mmu_cfg = (i + 2);

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_MMU_CFGr,
                            0,
                            i,
                            &ddr_in_mmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    /* ddr组从mmu到qmu映射,2~9映射成0~7 */
    for (i = 2; i < 10; i++)
    {
        ddr_in_qmu_cfg.cfgmt_ddr_in_qmu_cfg = (i - 2);

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_QMU_CFGr,
                            0,
                            i,
                            &ddr_in_qmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }


    /* bank号从qmu到mmu映射，每组的bank0~3映射成bank2、3、6、7 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 4; i++)
        {
            k = j * 8 + i;

            bank_to_mmu_cfg.cfgmt_bank_in_mmu_cfg = bank_to_mmu_cfg_map[i];

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_MMU_CFGr,
                                0,
                                k,
                                &bank_to_mmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }


    /* bank号从mmu到qmu映射，每组的bank2、3、6、7映射成bank0、1、2、3 */
    for (j = 2; j < 10; j++)
    {
        for (i = 0; i < 4; i++)
        {
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id,  j , 8);
            ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id,  j* 8 , bank_to_mmu_cfg_map[i]);
            k = j * 8 + bank_to_mmu_cfg_map[i];
            bank_to_qmu_cfg.cfgmt_bank_in_qmu_cfg = i;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_QMU_CFGr,
                                0,
                                k,
                                &bank_to_qmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    /* random映射ram:配置一个chunk中，bank的轮询顺序 */
    for (i = 0; i < 16; i++)
    {


        active_to_bank_cfg.cfgmt_active_to_bank_cfg = i % 4;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFGr,
                            0,
                            i,
                            &active_to_bank_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    }


    /* 首尾深度指针配置 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 4; i++)
        {
            k = j * 8 + i;
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 4 + i), depth);
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 4 + i + 1), depth);
            qlist_bhead.bank_vld = 1;
            qlist_bhead.qcfg_qlist_bhead = (j * 4 + i) * depth;
            qlist_btail.qcfg_qlist_btail = ((j * 4 + i + 1) * depth - 1);
            qlist_bdep.qcfg_qlist_bdep = depth;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BHEADr,
                                0,
                                k,
                                &qlist_bhead);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BTAILr,
                                0,
                                k,
                                &qlist_btail);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BDEPr,
                                0,
                                k,
                                &qlist_bdep);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    for (i = 0; i < 64; i++)
    {
        qlist_bdep.qcfg_qlist_bdep = depth;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_BDEPr,
                            0,
                            i,
                            &qlist_bdep);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(ETM_QMU_QCFG_QLIST_GRP0_BANKr, 8);
    /* 随机表配置，决定每组bank的轮询顺序 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 64; i++)
        {
            qlist_grp0_bank.qcfg_qlist_grp0_bank_wr = qlist_grp0_bank_data[i % 4];
            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP0_BANKr + j,
                                0,
                                i,
                                &qlist_grp0_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    for (j = 0; j < 64; j++)
    {
        qcfg_qlist_grp.qcfg_qlist_grp_wr = j % 8;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_GRPr,
                            0,
                            j,
                            &qcfg_qlist_grp);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    rc = dpp_tm_qmu_cfg_done_set(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_cfg_done_set");

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_1 END=======\n");

    return DPP_OK;
}



/***********************************************************/
/** QMU/MMU初始化配置场景7：每个tm用8组*4bank，MMU实际分配ftm:2-9组(0145bank); etm:2-9组(2367bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/10/14
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_7(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;

    

    /*关闭随机模式*/
    rc = dpp_tm_qmu_ddr_random_set(dev_id, 0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_ddr_random_set");

    /* etm mmu基址配置:ddr2-9组，bank号2367，bank深度1024 */
    //rc = dpp_mmu_bank_base_addr_set(dev_id, 0x3fc, 0xcc, 1024);
    //ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_mmu_bank_base_addr_set");

    rc = dpp_etm_qmu_init_set_7(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_etm_qmu_init_set");

    return DPP_OK;
}


/***********************************************************/
/** QMU/MMU初始化配置场景8：每个tm用8组*4bank，MMU实际分配ftm:2-9组(01bank); etm:2-9组(23bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/10/14
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_8(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 k = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 depth = 2048;
    ZXIC_UINT32 bank_to_mmu_cfg_map[4] = {2, 3};
    ZXIC_UINT32 qlist_grp0_bank_data[4] = {0, 1};


    DPP_ETM_QMU_QCFG_QLIST_BDEP_T qlist_bdep = {0};
    DPP_ETM_QMU_QCFG_QLIST_BHEAD_T qlist_bhead = {0};
    DPP_ETM_QMU_QCFG_QLIST_BTAIL_T qlist_btail = {0};

    DPP_ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFG_T active_to_bank_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_MMU_CFG_T ddr_in_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_QMU_CFG_T ddr_in_qmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_MMU_CFG_T bank_to_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_QMU_CFG_T bank_to_qmu_cfg = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP0_BANK_T qlist_grp0_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP_T qcfg_qlist_grp = {0};

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_1 start========n");


    /* ddr组从qmu到mmu映射,0~7映射成2~9 */
    for (i = 0; i < 8; i++)
    {
        ddr_in_mmu_cfg.cfgmt_ddr_in_mmu_cfg = (i + 2);

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_MMU_CFGr,
                            0,
                            i,
                            &ddr_in_mmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    /* ddr组从mmu到qmu映射,2~9映射成0~7 */
    for (i = 2; i < 10; i++)
    {
        ddr_in_qmu_cfg.cfgmt_ddr_in_qmu_cfg = (i - 2);

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_QMU_CFGr,
                            0,
                            i,
                            &ddr_in_qmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }


    /* bank号从qmu到mmu映射，每组的bank0~1映射成bank2、3 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 2; i++)
        {
            k = j * 8 + i;

            bank_to_mmu_cfg.cfgmt_bank_in_mmu_cfg = bank_to_mmu_cfg_map[i];

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_MMU_CFGr,
                                0,
                                k,
                                &bank_to_mmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }


    /* bank号从mmu到qmu映射，每组的bank2、3映射成bank0、1 */
    for (j = 2; j < 10; j++)
    {
        for (i = 0; i < 2; i++)
        {
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id,  j, 8);
            ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id,  j * 8 , bank_to_mmu_cfg_map[i]);
            k = j * 8 + bank_to_mmu_cfg_map[i];
            bank_to_qmu_cfg.cfgmt_bank_in_qmu_cfg = i;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_QMU_CFGr,
                                0,
                                k,
                                &bank_to_qmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    /* random映射ram:配置一个chunk中，bank的轮询顺序 */
    for (i = 0; i < 16; i++)
    {


        active_to_bank_cfg.cfgmt_active_to_bank_cfg = i % 2;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFGr,
                            0,
                            i,
                            &active_to_bank_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    }


    /* 首尾深度指针配置 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 2; i++)
        {
            k = j * 8 + i;
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 2 + i), depth);
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 2 + i + 1), depth);
            qlist_bhead.bank_vld = 1;
            qlist_bhead.qcfg_qlist_bhead = (j * 2 + i) * depth;
            qlist_btail.qcfg_qlist_btail = ((j * 2 + i + 1) * depth - 1);
            qlist_bdep.qcfg_qlist_bdep = depth;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BHEADr,
                                0,
                                k,
                                &qlist_bhead);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BTAILr,
                                0,
                                k,
                                &qlist_btail);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BDEPr,
                                0,
                                k,
                                &qlist_bdep);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(ETM_QMU_QCFG_QLIST_GRP0_BANKr, 8);
    /* 随机表配置，决定每组bank的轮询顺序 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 64; i++)
        {
            qlist_grp0_bank.qcfg_qlist_grp0_bank_wr = qlist_grp0_bank_data[i % 2];
            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP0_BANKr + j,
                                0,
                                i,
                                &qlist_grp0_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    for (j = 0; j < 64; j++)
    {
        qcfg_qlist_grp.qcfg_qlist_grp_wr = j % 8;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_GRPr,
                            0,
                            j,
                            &qcfg_qlist_grp);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    rc = dpp_tm_qmu_cfg_done_set(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_cfg_done_set");

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_1 END=======\n");

    return DPP_OK;
}



/***********************************************************/
/** QMU/MMU初始化配置场景8：每个tm用8组*4bank，MMU实际分配ftm:2-9组(01bank); etm:2-9组(23bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/10/14
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_8(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;

    /* etm mmu基址配置:ddr2-9组，bank号23，bank深度1024 */
    //rc = dpp_mmu_bank_base_addr_set(dev_id, 0x3fc, 0xc, 2048);
    //ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_mmu_bank_base_addr_set");

    rc = dpp_etm_qmu_init_set_8(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_etm_qmu_init_set");

    return DPP_OK;
}


/***********************************************************/
/** QMU初始化配置场景9：4组*2bank，MMU实际分配ftm:6,7,8,9组(01bank); etm:6,7,8,9组(23bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/09/28
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_9(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 k = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 depth = 4*1024;
    ZXIC_UINT32 bank_to_mmu_cfg_map[4] = {2, 3};
    ZXIC_UINT32 qlist_grp0_bank_data[4] = {0, 1};
    ZXIC_UINT32 ddr_no[4] = {6, 7, 8, 9};


    DPP_ETM_QMU_QCFG_QLIST_BDEP_T qlist_bdep = {0};
    DPP_ETM_QMU_QCFG_QLIST_BHEAD_T qlist_bhead = {0};
    DPP_ETM_QMU_QCFG_QLIST_BTAIL_T qlist_btail = {0};

    DPP_ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFG_T active_to_bank_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_MMU_CFG_T ddr_in_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_QMU_CFG_T ddr_in_qmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_MMU_CFG_T bank_to_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_QMU_CFG_T bank_to_qmu_cfg = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP0_BANK_T qlist_grp0_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP_T qcfg_qlist_grp = {0};

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_1 start========n");


    /* ddr组从qmu到mmu映射,0~3映射成4679 */
    for (i = 0; i < 4; i++)
    {
        ddr_in_mmu_cfg.cfgmt_ddr_in_mmu_cfg = ddr_no[i];

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_MMU_CFGr,
                            0,
                            i,
                            &ddr_in_mmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    /* ddr组从mmu到qmu映射,4679映射成0~3 */
    for (i = 0; i < 4; i++)
    {
        ddr_in_qmu_cfg.cfgmt_ddr_in_qmu_cfg = i;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_QMU_CFGr,
                            0,
                            ddr_no[i],
                            &ddr_in_qmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }


    /* bank号从qmu到mmu映射，每组的bank0~1映射成bank2、3 */
    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 2; i++)
        {
            k = j * 8 + i;

            bank_to_mmu_cfg.cfgmt_bank_in_mmu_cfg = bank_to_mmu_cfg_map[i];

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_MMU_CFGr,
                                0,
                                k,
                                &bank_to_mmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }


    /* bank号从mmu到qmu映射，每组的bank2、3映射成bank0、1 */
    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 2; i++)
        {
            ZXIC_COMM_CHECK_INDEX_MUL_OVERFLOW_NO_ASSERT(ddr_no[j], 8);
            ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(ddr_no[j] * 8, bank_to_mmu_cfg_map[i]);
            k = ddr_no[j] * 8 + bank_to_mmu_cfg_map[i];

            bank_to_qmu_cfg.cfgmt_bank_in_qmu_cfg = i;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_QMU_CFGr,
                                0,
                                k,
                                &bank_to_qmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    /* random映射ram:配置一个chunk中，bank的轮询顺序 */
    for (i = 0; i < 16; i++)
    {


        active_to_bank_cfg.cfgmt_active_to_bank_cfg = i % 2;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFGr,
                            0,
                            i,
                            &active_to_bank_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    }


    /* 首尾深度指针配置 */
    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 2; i++)
        {
            k = j * 8 + i;
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 2 + i), depth);
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 2 + i + 1), depth);
            qlist_bhead.bank_vld = 1;
            qlist_bhead.qcfg_qlist_bhead = (j * 2 + i) * depth;
            qlist_btail.qcfg_qlist_btail = ((j * 2 + i + 1) * depth - 1);
            qlist_bdep.qcfg_qlist_bdep = depth;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BHEADr,
                                0,
                                k,
                                &qlist_bhead);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BTAILr,
                                0,
                                k,
                                &qlist_btail);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BDEPr,
                                0,
                                k,
                                &qlist_bdep);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    for (i = 0; i < 64; i++)
    {
        qlist_bdep.qcfg_qlist_bdep = depth;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_BDEPr,
                            0,
                            i,
                            &qlist_bdep);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }


    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(ETM_QMU_QCFG_QLIST_GRP0_BANKr, 8);
    /* 随机表配置，决定每组bank的轮询顺序 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 64; i++)
        {
            qlist_grp0_bank.qcfg_qlist_grp0_bank_wr = qlist_grp0_bank_data[i % 2];
            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP0_BANKr + j,
                                0,
                                i,
                                &qlist_grp0_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    for (j = 0; j < 64; j++)
    {
        qcfg_qlist_grp.qcfg_qlist_grp_wr = j % 4;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_GRPr,
                            0,
                            j,
                            &qcfg_qlist_grp);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    rc = dpp_tm_qmu_cfg_done_set(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_cfg_done_set");

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_1 END=======\n");

    return DPP_OK;
}


/***********************************************************/
/** QMU/MMU初始化配置场景8：每个tm用8组*4bank，MMU实际分配ftm:2-9组(01bank); etm:2-9组(23bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/10/14
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_9(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;

    /* etm mmu基址配置:ddr2-9组，bank号23，bank深度1024 */
    //rc = dpp_mmu_bank_base_addr_set(dev_id, 0x3fc, 0xc, 2048);
    //ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_mmu_bank_base_addr_set");

    rc = dpp_etm_qmu_init_set_8(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_etm_qmu_init_set");

    return DPP_OK;
}

/***********************************************************/
/** QMU初始化配置场景10：独享2组*8bank，MMU实际分配1,2; etm or ftm使用0-7bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/12/16
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_10(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 k = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 depth = 2*1024;
    ZXIC_UINT32 bank_to_mmu_cfg_map[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    ZXIC_UINT32 qlist_grp0_bank_data[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    ZXIC_UINT32 ddr_no[2] = {1,2};


    DPP_ETM_QMU_QCFG_QLIST_BDEP_T qlist_bdep = {0};
    DPP_ETM_QMU_QCFG_QLIST_BHEAD_T qlist_bhead = {0};
    DPP_ETM_QMU_QCFG_QLIST_BTAIL_T qlist_btail = {0};

    DPP_ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFG_T active_to_bank_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_MMU_CFG_T ddr_in_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_QMU_CFG_T ddr_in_qmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_MMU_CFG_T bank_to_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_QMU_CFG_T bank_to_qmu_cfg = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP0_BANK_T qlist_grp0_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP_T qcfg_qlist_grp = {0};

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_1 start========n");


    /* ddr组从qmu到mmu映射,0~1映射成12 */
    for (i = 0; i < 2; i++)
    {
        ddr_in_mmu_cfg.cfgmt_ddr_in_mmu_cfg = ddr_no[i];

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_MMU_CFGr,
                            0,
                            i,
                            &ddr_in_mmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    /* ddr组从mmu到qmu映射,12映射成0~1 */
    for (i = 0; i < 2; i++)
    {
        ddr_in_qmu_cfg.cfgmt_ddr_in_qmu_cfg = i;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_QMU_CFGr,
                            0,
                            ddr_no[i],
                            &ddr_in_qmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }


    /* bank号从qmu到mmu映射，每组的bank0~7映射成bank0-7 */
    for (j = 0; j < 2; j++)
    {
        for (i = 0; i < 8; i++)
        {
            k = j * 8 + i;

            bank_to_mmu_cfg.cfgmt_bank_in_mmu_cfg = bank_to_mmu_cfg_map[i];

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_MMU_CFGr,
                                0,
                                k,
                                &bank_to_mmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }


    /* bank号从mmu到qmu映射，每组的bank0-7映射成bank0-7 */
    for (j = 0; j < 2; j++)
    {
        for (i = 0; i < 8; i++)
        {
            ZXIC_COMM_CHECK_INDEX_MUL_OVERFLOW_NO_ASSERT(ddr_no[j], 8);
            ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(ddr_no[j] * 8, bank_to_mmu_cfg_map[i]);
            k = ddr_no[j] * 8 + bank_to_mmu_cfg_map[i];

            bank_to_qmu_cfg.cfgmt_bank_in_qmu_cfg = i;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_QMU_CFGr,
                                0,
                                k,
                                &bank_to_qmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    /* random映射ram:配置一个chunk中，bank的轮询顺序 */
    for (i = 0; i < 16; i++)
    {


        active_to_bank_cfg.cfgmt_active_to_bank_cfg = i % 8;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFGr,
                            0,
                            i,
                            &active_to_bank_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    }


    /* 首尾深度指针配置 */
    for (j = 0; j < 2; j++)
    {
        for (i = 0; i < 8; i++)
        {
            k = j * 8 + i;

            qlist_bhead.bank_vld = 1;
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 8 + i), depth);
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 8 + i + 1), depth);
            qlist_bhead.qcfg_qlist_bhead = (j * 8 + i) * depth;
            qlist_btail.qcfg_qlist_btail = ((j * 8 + i + 1) * depth - 1);
            qlist_bdep.qcfg_qlist_bdep = depth;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BHEADr,
                                0,
                                k,
                                &qlist_bhead);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BTAILr,
                                0,
                                k,
                                &qlist_btail);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BDEPr,
                                0,
                                k,
                                &qlist_bdep);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    for (i = 0; i < 64; i++)
    {
        qlist_bdep.qcfg_qlist_bdep = depth;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_BDEPr,
                            0,
                            i,
                            &qlist_bdep);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(ETM_QMU_QCFG_QLIST_GRP0_BANKr, 8);
    /* 随机表配置，决定每组bank的轮询顺序 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 64; i++)
        {
            qlist_grp0_bank.qcfg_qlist_grp0_bank_wr = qlist_grp0_bank_data[i % 8];
            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP0_BANKr + j,
                                0,
                                i,
                                &qlist_grp0_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    for (j = 0; j < 64; j++)
    {
        qcfg_qlist_grp.qcfg_qlist_grp_wr = j % 2;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_GRPr,
                            0,
                            j,
                            &qcfg_qlist_grp);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    rc = dpp_tm_qmu_cfg_done_set(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_cfg_done_set");

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_1 END=======\n");

    return DPP_OK;
}

/***********************************************************/
/** QMU初始化配置场景10：独享2组*8bank，MMU实际分配1,2; etm or ftm使用0-7bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/12/16
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_10(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;

    /* etm mmu基址配置:ddr1,2组，bank号0-7，bank深度2*1024 */
    //rc = dpp_mmu_bank_base_addr_set(dev_id, 0x6, 0xff, 2*1024);
    //ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_mmu_bank_base_addr_set");

    rc = dpp_etm_qmu_init_set_10(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_etm_qmu_init_set_10");

    return DPP_OK;
}


/***********************************************************/
/**场景11：TM共享2组ddr:FTM为ddr_no1、ddr_no2组ddr的01bank，ETM为ddr_no1、ddr_no2组ddr 23bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/06/06
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_11(ZXIC_UINT32 dev_id, ZXIC_UINT32 ddr_no1, ZXIC_UINT32 ddr_no2)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 k = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 depth = 8 * 1024;
    ZXIC_UINT32 ddr_to_mmu_cfg_map[2] = {ddr_no1, ddr_no2};
    ZXIC_UINT32 bank_to_mmu_cfg_map[2] = {2, 3};
    ZXIC_UINT32 qlist_grp0_bank_data[2] = {0, 1};


    DPP_ETM_QMU_QCFG_QLIST_BDEP_T qlist_bdep = {0};
    DPP_ETM_QMU_QCFG_QLIST_BHEAD_T qlist_bhead = {0};
    DPP_ETM_QMU_QCFG_QLIST_BTAIL_T qlist_btail = {0};

    DPP_ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFG_T active_to_bank_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_MMU_CFG_T ddr_in_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_QMU_CFG_T ddr_in_qmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_MMU_CFG_T bank_to_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_QMU_CFG_T bank_to_qmu_cfg = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP0_BANK_T qlist_grp0_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP_T qcfg_qlist_grp = {0};

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_11 start========n");


    /* ddr组从qmu到mmu映射,0~1映射成ddr_no1 ddr_no2 */
    for (i = 0; i < 2; i++)
    {
        ddr_in_mmu_cfg.cfgmt_ddr_in_mmu_cfg = ddr_to_mmu_cfg_map[i];

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_MMU_CFGr,
                            0,
                            i,
                            &ddr_in_mmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    /* ddr组从mmu到qmu映射,ddr_no1 ddr_no2映射成0~1 */
    for (i = 0; i < 2; i++)
    { 
        ddr_in_qmu_cfg.cfgmt_ddr_in_qmu_cfg = i;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_QMU_CFGr,
                            0,
                            ddr_to_mmu_cfg_map[i],
                            &ddr_in_qmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }


    /* bank号从qmu到mmu映射，每组的bank0~1映射成bank2、3 */
    for (j = 0; j < 2; j++)
    {
        for (i = 0; i < 2; i++)
        {
            k = j * 8 + i;

            bank_to_mmu_cfg.cfgmt_bank_in_mmu_cfg = bank_to_mmu_cfg_map[i];

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_MMU_CFGr,
                                0,
                                k,
                                &bank_to_mmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }


    /* bank号从mmu到qmu映射，每组的bank2、3映射成bank0、1 */
    for (j = 0; j < 2; j++)
    {
        for (i = 0; i < 2; i++)
        {
            ZXIC_COMM_CHECK_INDEX_MUL_OVERFLOW_NO_ASSERT(ddr_to_mmu_cfg_map[j], 8);
            ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(ddr_to_mmu_cfg_map[j] * 8, bank_to_mmu_cfg_map[i]);
            k = ddr_to_mmu_cfg_map[j] * 8 + bank_to_mmu_cfg_map[i];

            bank_to_qmu_cfg.cfgmt_bank_in_qmu_cfg = i;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_QMU_CFGr,
                                0,
                                k,
                                &bank_to_qmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    /* random映射ram:配置一个chunk中，bank的轮询顺序 */
    for (i = 0; i < 16; i++)
    {


        active_to_bank_cfg.cfgmt_active_to_bank_cfg = i % 2;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFGr,
                            0,
                            i,
                            &active_to_bank_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    }


    /* 首尾深度指针配置 */
    for (j = 0; j < 2; j++)
    {
        for (i = 0; i < 2; i++)
        {
            k = j * 8 + i;

            qlist_bhead.bank_vld = 1;
            ZXIC_COMM_CHECK_INDEX_MUL_OVERFLOW_NO_ASSERT((j * 2 + i), depth);
            qlist_bhead.qcfg_qlist_bhead = (j * 2 + i) * depth;
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 2 + i), depth);
            qlist_btail.qcfg_qlist_btail = ((j * 2 + i + 1) * depth - 1);
            qlist_bdep.qcfg_qlist_bdep = depth;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BHEADr,
                                0,
                                k,
                                &qlist_bhead);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BTAILr,
                                0,
                                k,
                                &qlist_btail);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BDEPr,
                                0,
                                k,
                                &qlist_bdep);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    for (i = 0; i < 64; i++)
    {
        qlist_bdep.qcfg_qlist_bdep = depth;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_BDEPr,
                            0,
                            i,
                            &qlist_bdep);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(ETM_QMU_QCFG_QLIST_GRP0_BANKr, 8);
    /* 随机表配置，决定每组bank的轮询顺序 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 64; i++)
        {
            qlist_grp0_bank.qcfg_qlist_grp0_bank_wr = qlist_grp0_bank_data[i % 2];
            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP0_BANKr + j,
                                0,
                                i,
                                &qlist_grp0_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    for (j = 0; j < 64; j++)
    {
        qcfg_qlist_grp.qcfg_qlist_grp_wr = j % 2;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_GRPr,
                            0,
                            j,
                            &qcfg_qlist_grp);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    rc = dpp_tm_qmu_cfg_done_set(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_cfg_done_set");

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_11 END=======\n");

    return DPP_OK;
}



/***********************************************************/
/**场景4：TM共享2组ddr:FTM为ddr_no1,ddr_no2组ddr的01bank，ETM为ddr_no1,ddr_no2组ddr 23bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/06/06
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_11(ZXIC_UINT32 dev_id, ZXIC_UINT32 ddr_no1, ZXIC_UINT32 ddr_no2)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_RANDOM_BYPASS_EN_T etm_bypass = {0};
    //DPP_FTM_QMU_RANDOM_BYPASS_EN_T ftm_bypass = {0};

    
    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT((1U<<ddr_no1), (1U<<ddr_no2));

    /* etm mmu基址配置:ddr_no1,ddr_no2组，bank号23，bank深度8*1024 */
    //rc = dpp_mmu_bank_base_addr_set(dev_id, ((1U<<ddr_no1)+(1U<<ddr_no2))&0xff, 0xc, 8 * 1024);
    //ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_mmu_bank_base_addr_set");

    rc = dpp_etm_qmu_init_set_11(dev_id, ddr_no1, ddr_no2);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_etm_qmu_init_set_11");

    /* 关闭etm映射配置 */
    etm_bypass.random_bypass_en = 0;
    rc  = dpp_reg_write(dev_id,
                        ETM_QMU_RANDOM_BYPASS_ENr,
                        0,
                        0,
                        &etm_bypass);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");


    return DPP_OK;
}




/***********************************************************/
/** 
QMU/MMU初始化配置场景15：每个tm只用1组*4bank，MMU实际分配ftm:0-9中指定组(4567bank); etm:0-9中指定组(0123bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/4/14
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_12(ZXIC_UINT32 dev_id, ZXIC_UINT32 ddr_no)
{
    DPP_STATUS  rc = DPP_OK;

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, ddr_no, 0, 9);

    /*关闭随机模式*/
    rc = dpp_tm_qmu_ddr_random_set(dev_id, 0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_ddr_random_set");

    /* etm mmu基址配置:ddr指定ddr_no组，bank号0123，bank深度8K */
    //rc = dpp_mmu_bank_base_addr_set(dev_id, 1U << ddr_no, 0xf, 8 * 1024);
    //ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_mmu_bank_base_addr_set");

    rc = dpp_etm_qmu_init_set_15(dev_id, ddr_no);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_etm_qmu_init_set_15");


    return DPP_OK;
}


/***********************************************************/
/** QMU初始化配置场景13：TM独享4组ddr:FTM为指定4组ddr的0~7bank，
                                      ETM为指定4组ddr 0~7bank
* @param   dev_id   设备编号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_13(ZXIC_UINT32 dev_id, ZXIC_UINT32 ddr_no)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 k = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 depth = 1024;
    ZXIC_UINT32 ddr_no_data[4] = {0};
    ZXIC_UINT32 qlist_grp0_bank_data[8] = {0, 1, 2, 3, 4, 5, 6, 7};


    DPP_ETM_QMU_QCFG_QLIST_BDEP_T qlist_bdep = {0};
    DPP_ETM_QMU_QCFG_QLIST_BHEAD_T qlist_bhead = {0};
    DPP_ETM_QMU_QCFG_QLIST_BTAIL_T qlist_btail = {0};
    DPP_ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFG_T active_to_bank_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_MMU_CFG_T ddr_in_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_QMU_CFG_T ddr_in_qmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_MMU_CFG_T bank_to_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_QMU_CFG_T bank_to_qmu_cfg = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP0_BANK_T qlist_grp0_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP_T qcfg_qlist_grp = {0};

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++dpp_etm_qmu_init_set starting++++++\n");

    /*将ddr_no中的编号取出存入数组中，如传入0x6789 依次存为 6 7 8 9*/
    ddr_no_data[3] = ddr_no & 0xf;
    ddr_no_data[2] = (ddr_no >> 4) & 0xf;
    ddr_no_data[1] = (ddr_no >> 8) & 0xf;
    ddr_no_data[0] = (ddr_no >> 12) & 0xf;

    /* ddr组从qmu到mmu映射,0~3映射成ddr_no */
    for (i = 0; i < 4; i++)
    {
        ddr_in_mmu_cfg.cfgmt_ddr_in_mmu_cfg = ddr_no_data[i];

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_MMU_CFGr,
                            0,
                            i,
                            &ddr_in_mmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    /* ddr组从mmu到qmu映射,ddr_no映射成0~3 */
    for (i = 0; i < 4; i++)
    {
        ddr_in_qmu_cfg.cfgmt_ddr_in_qmu_cfg = i;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_QMU_CFGr,
                            0,
                            ddr_no_data[i],
                            &ddr_in_qmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    /* bank号从qmu到mmu映射，组0~3的bank0-7映射成bank0-7 */
    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 8; i++)
        {
            k = j * 8 + i;

            bank_to_mmu_cfg.cfgmt_bank_in_mmu_cfg = i;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_MMU_CFGr,
                                0,
                                k,
                                &bank_to_mmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }


    /* bank号从mmu到qmu映射，组ddr_no的bank0-7映射成bank0-7 */
    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 8; i++)
        {
            ZXIC_COMM_CHECK_INDEX_MUL_OVERFLOW_NO_ASSERT(ddr_no_data[j], 8);
            ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(ddr_no_data[j] * 8, i);
            k = ddr_no_data[j] * 8 + i;

            bank_to_qmu_cfg.cfgmt_bank_in_qmu_cfg = i;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_QMU_CFGr,
                                0,
                                k,
                                &bank_to_qmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    /* random映射ram:配置一个chunk中，bank的轮询顺序 */
    for (i = 0; i < 16; i++)
    {
        active_to_bank_cfg.cfgmt_active_to_bank_cfg = i % 8;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFGr,
                            0,
                            i,
                            &active_to_bank_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    /* 首尾深度指针配置:j为4组循环 */
    for (j = 0; j < 4; j++)
    {
        for (i = 0; i < 8; i++)
        {
            k = j * 8 + i;
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 8 + i), depth);
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 8 + i + 1), depth);
            qlist_bhead.bank_vld = 1;
            qlist_bhead.qcfg_qlist_bhead = (j * 8 + i) * depth;
            qlist_btail.qcfg_qlist_btail = ((j * 8 + i + 1) * depth - 1);
            qlist_bdep.qcfg_qlist_bdep = depth;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BHEADr,
                                0,
                                k,
                                &qlist_bhead);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BTAILr,
                                0,
                                k,
                                &qlist_btail);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BDEPr,
                                0,
                                k,
                                &qlist_bdep);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }
    for (i = 0; i < 64; i++)
    {
        qlist_bdep.qcfg_qlist_bdep = depth;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_BDEPr,
                            0,
                            i,
                            &qlist_bdep);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(ETM_QMU_QCFG_QLIST_GRP0_BANKr, 8);
    /* 随机表配置，决定ddr内bank的轮询顺序:使用几组ddr需要配置几组 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 64; i++)
        {

            qlist_grp0_bank.qcfg_qlist_grp0_bank_wr = qlist_grp0_bank_data[i % 8];

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP0_BANKr + j,
                                0,
                                i,
                                &qlist_grp0_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        }
    }

    for (j = 0; j < 64; j++)
    {
        qcfg_qlist_grp.qcfg_qlist_grp_wr = j % 4;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_GRPr,
                            0,
                            j,
                            &qcfg_qlist_grp);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    rc = dpp_tm_qmu_cfg_done_set(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_cfg_done_set");

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "++++++dpp_etm_qmu_init_set end++++++\n");

    return DPP_OK;
}




/***********************************************************/
/** QMU/MMU初始化配置场景13：TM独享4组ddr:FTM为指定4组ddr的0~7bank 
                                          ETM为指定4组ddr的0-7bank
*   MMU开启rotatjon
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_13(ZXIC_UINT32 dev_id, ZXIC_UINT32 ddr_no)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_STATUS ddr_no_temp = 0;
    DPP_ETM_QMU_RANDOM_BYPASS_EN_T etm_bypass = {0};
    //DPP_FTM_QMU_RANDOM_BYPASS_EN_T ftm_bypass = {0};

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, ((ddr_no >> 0) & 0xfU), 0, 9);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, ((ddr_no >> 4) & 0xfU), 0, 9);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, ((ddr_no >> 8) & 0xfU), 0, 9);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, ((ddr_no >> 12) & 0xfU), 0, 9);

    ddr_no_temp = ((1U << (ddr_no & 0xfU)) + (1U << ((ddr_no >> 4) & 0xfU))+   \
             (1U << ((ddr_no >> 8) & 0xfU)) + (1U << ((ddr_no >> 12) & 0xfU))) & 0x3ff;


    /* etm mmu基址配置:ddr_no中的4组，bank号0-7，bank深度1024 */
    //rc = dpp_mmu_bank_base_addr_set(dev_id, ddr_no_temp, 0xff, 1024);
    //ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_mmu_bank_base_addr_set");

    rc = dpp_etm_qmu_init_set_13(dev_id, ddr_no);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_etm_qmu_init_set_13");

    /* 关闭etm映射配置 */
    etm_bypass.random_bypass_en = 0;
    rc  = dpp_reg_write(dev_id,
                        ETM_QMU_RANDOM_BYPASS_ENr,
                        0,
                        0,
                        &etm_bypass);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");


    return DPP_OK;
}



/***********************************************************/
/** QMU/MMU初始化配置场景14：每个tm只用1组*8bank，MMU实际分配ftm:0-9中指定组(0-7bank); etm:0-9中指定组(0-7bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/4/14
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_14(ZXIC_UINT32 dev_id, ZXIC_UINT32 etm_ddr_no)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 k = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 depth = 4 * 1024;
    ZXIC_UINT32 bank_to_mmu_cfg_map[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    ZXIC_UINT32 qlist_grp0_bank_data[8] = {0, 1, 2, 3, 4, 5, 6, 7};


    DPP_ETM_QMU_QCFG_QLIST_BDEP_T qlist_bdep = {0};
    DPP_ETM_QMU_QCFG_QLIST_BHEAD_T qlist_bhead = {0};
    DPP_ETM_QMU_QCFG_QLIST_BTAIL_T qlist_btail = {0};

    DPP_ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFG_T active_to_bank_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_MMU_CFG_T ddr_in_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_QMU_CFG_T ddr_in_qmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_MMU_CFG_T bank_to_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_QMU_CFG_T bank_to_qmu_cfg = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP0_BANK_T qlist_grp0_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP_T qcfg_qlist_grp = {0};

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_1 start========n");


    /* ddr组从qmu到mmu映射,将0映射成etm_ddr_no */
    i = 0;
    ddr_in_mmu_cfg.cfgmt_ddr_in_mmu_cfg = etm_ddr_no;
    rc = dpp_reg_write(dev_id,
                       ETM_QMU_CFGMT_DDR_IN_MMU_CFGr,
                       0,
                       i,
                       &ddr_in_mmu_cfg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");


    /* ddr组从mmu到qmu映射,etm_ddr_no映射成0 */
    i = etm_ddr_no;
    ddr_in_qmu_cfg.cfgmt_ddr_in_qmu_cfg = 0;

    rc = dpp_reg_write(dev_id,
                       ETM_QMU_CFGMT_DDR_IN_QMU_CFGr,
                       0,
                       i,
                       &ddr_in_qmu_cfg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");


    /* bank号从qmu到mmu映射，每组的bank0-7映射成bank0-7 */
    j = 0;

    for (i = 0; i < 8; i++)
    {
        k = j * 8 + i;
        bank_to_mmu_cfg.cfgmt_bank_in_mmu_cfg = bank_to_mmu_cfg_map[i];

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_BANK_TO_MMU_CFGr,
                            0,
                            k,
                            &bank_to_mmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }


    /* bank号从mmu到qmu映射，每组的bank0-7映射成bank0-7 */
    j = etm_ddr_no;

    for (i = 0; i < 8; i++)
    {
        ZXIC_COMM_CHECK_INDEX_MUL_OVERFLOW_NO_ASSERT(j, 8);
        ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(j * 8, bank_to_mmu_cfg_map[i]);
        k = j * 8 + bank_to_mmu_cfg_map[i];
        bank_to_qmu_cfg.cfgmt_bank_in_qmu_cfg = i;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_BANK_TO_QMU_CFGr,
                            0,
                            k,
                            &bank_to_qmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }


    /* random映射ram:配置一个chunk中，bank的轮询顺序 */
    for (i = 0; i < 16; i++)
    {
        active_to_bank_cfg.cfgmt_active_to_bank_cfg = i % 8;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFGr,
                            0,
                            i,
                            &active_to_bank_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }


    /* 首尾深度指针配置 */
    j = 0;

    for (i = 0; i < 8; i++)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 8 + i), depth);
        ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 8 + i + 1), depth);
        k = j * 8 + i;
        qlist_bhead.bank_vld = 1;
        qlist_bhead.qcfg_qlist_bhead = (j * 8 + i) * depth;
        qlist_btail.qcfg_qlist_btail = ((j * 8 + i + 1) * depth - 1);
        qlist_bdep.qcfg_qlist_bdep = depth;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_BHEADr,
                            0,
                            k,
                            &qlist_bhead);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_BTAILr,
                            0,
                            k,
                            &qlist_btail);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_BDEPr,
                            0,
                            k,
                            &qlist_bdep);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    for (i = 0; i < 64; i++)
    {
        qlist_bdep.qcfg_qlist_bdep = depth;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_BDEPr,
                            0,
                            i,
                            &qlist_bdep);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(ETM_QMU_QCFG_QLIST_GRP0_BANKr, 8);
    /* 随机表配置，决定每组bank的轮询顺序 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 64; i++)
        {
            qlist_grp0_bank.qcfg_qlist_grp0_bank_wr = qlist_grp0_bank_data[i % 8];
            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP0_BANKr + j,
                                0,
                                i,
                                &qlist_grp0_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    for (j = 0; j < 64; j++)
    {
        qcfg_qlist_grp.qcfg_qlist_grp_wr = 0;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_GRPr,
                            0,
                            j,
                            &qcfg_qlist_grp);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    rc = dpp_tm_qmu_cfg_done_set(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_cfg_done_set");

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_1 END=======\n");

    return DPP_OK;
}



/***********************************************************/
/** QMU/MMU初始化配置场景14：每个tm只用1组*8bank，MMU实际分配ftm:0-9中指定组(0-7bank); etm:0-9中指定组(0-7bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/4/14
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_14(ZXIC_UINT32 dev_id, ZXIC_UINT32 etm_ddr_no, ZXIC_UINT32 ftm_ddr_no)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 etm_ddr_no_temp = 0;   /*ETM使用的ddr,bit位表示*/

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, etm_ddr_no, 0, 9);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, ftm_ddr_no, 0, 9);

    /*etm ftm不能使用同一组ddr*/
    if (etm_ddr_no == ftm_ddr_no)
    {
        ZXIC_COMM_PRINT("dpp_tm_qmu_init_set_15: etm_ddr_no can't equal to ftm_ddr_no!!!\n");
    }

    ZXIC_COMM_ASSERT(etm_ddr_no != ftm_ddr_no);

    /*将etm ftm使用的ddr号转换成对应的bit位,bit[0-9]每bit代表一组ddr */
    etm_ddr_no_temp = 0x1u << etm_ddr_no;

    /*开启随机模式*/
    rc = dpp_tm_qmu_ddr_random_set(dev_id, 1);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_ddr_random_set");

    /* etm mmu基址配置:ddr指定etm_ddr_no组，bank号0-7bank，bank深度4K */
    //rc = dpp_mmu_bank_base_addr_set(dev_id, etm_ddr_no_temp, 0xff, 4 * 1024);
    //ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_mmu_bank_base_addr_set");

    rc = dpp_etm_qmu_init_set_14(dev_id, etm_ddr_no);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_etm_qmu_init_set");
    
    return DPP_OK;
}


/***********************************************************/
/** QMU/MMU初始化配置场景15：每个tm只用1组*4bank，MMU实际分配ftm:0-9中指定组(0123bank); etm:0-9中指定组(0123bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/4/14
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_15(ZXIC_UINT32 dev_id, ZXIC_UINT32 etm_ddr_no)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 k = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 depth = 8 * 1024;
    ZXIC_UINT32 bank_to_mmu_cfg_map[4] = {0, 1, 2, 3};
    ZXIC_UINT32 qlist_grp0_bank_data[4] = {0, 1, 2, 3};


    DPP_ETM_QMU_QCFG_QLIST_BDEP_T qlist_bdep = {0};
    DPP_ETM_QMU_QCFG_QLIST_BHEAD_T qlist_bhead = {0};
    DPP_ETM_QMU_QCFG_QLIST_BTAIL_T qlist_btail = {0};

    DPP_ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFG_T active_to_bank_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_MMU_CFG_T ddr_in_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_QMU_CFG_T ddr_in_qmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_MMU_CFG_T bank_to_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_QMU_CFG_T bank_to_qmu_cfg = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP0_BANK_T qlist_grp0_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP_T qcfg_qlist_grp = {0};

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_1 start========n");



    /* ddr组从qmu到mmu映射,将0映射成etm_ddr_no */
    i = 0;
    ddr_in_mmu_cfg.cfgmt_ddr_in_mmu_cfg = etm_ddr_no;
    rc = dpp_reg_write(dev_id,
                       ETM_QMU_CFGMT_DDR_IN_MMU_CFGr,
                       0,
                       i,
                       &ddr_in_mmu_cfg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");


    /* ddr组从mmu到qmu映射,etm_ddr_no映射成0 */
    i = etm_ddr_no;
    ddr_in_qmu_cfg.cfgmt_ddr_in_qmu_cfg = 0;

    rc = dpp_reg_write(dev_id,
                       ETM_QMU_CFGMT_DDR_IN_QMU_CFGr,
                       0,
                       i,
                       &ddr_in_qmu_cfg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");


    /* bank号从qmu到mmu映射，每组的bank0123映射成bank0123 */
    j = 0;

    for (i = 0; i < 4; i++)
    {
        k = j * 8 + i;
        bank_to_mmu_cfg.cfgmt_bank_in_mmu_cfg = bank_to_mmu_cfg_map[i];

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_BANK_TO_MMU_CFGr,
                            0,
                            k,
                            &bank_to_mmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }


    /* bank号从mmu到qmu映射，每组的bank0123映射成bank0123 */
    j = etm_ddr_no;

    for (i = 0; i < 4; i++)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id,  j, 8);
        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id,  j * 8 , bank_to_mmu_cfg_map[i]);
        k = j * 8 + bank_to_mmu_cfg_map[i];
        bank_to_qmu_cfg.cfgmt_bank_in_qmu_cfg = i;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_BANK_TO_QMU_CFGr,
                            0,
                            k,
                            &bank_to_qmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }


    /* random映射ram:配置一个chunk中，bank的轮询顺序 */
    for (i = 0; i < 16; i++)
    {
        active_to_bank_cfg.cfgmt_active_to_bank_cfg = i % 4;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFGr,
                            0,
                            i,
                            &active_to_bank_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }


    /* 首尾深度指针配置 */
    j = 0;

    for (i = 0; i < 4; i++)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 4 + i), depth);
        ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 4 + i + 1), depth);
        k = j * 8 + i;
        qlist_bhead.bank_vld = 1;
        qlist_bhead.qcfg_qlist_bhead = (j * 4 + i) * depth;
        qlist_btail.qcfg_qlist_btail = ((j * 4 + i + 1) * depth - 1);
        qlist_bdep.qcfg_qlist_bdep = depth;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_BHEADr,
                            0,
                            k,
                            &qlist_bhead);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_BTAILr,
                            0,
                            k,
                            &qlist_btail);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_BDEPr,
                            0,
                            k,
                            &qlist_bdep);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    for (i = 0; i < 64; i++)
    {
        qlist_bdep.qcfg_qlist_bdep = depth;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_BDEPr,
                            0,
                            i,
                            &qlist_bdep);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(ETM_QMU_QCFG_QLIST_GRP0_BANKr, 8);
    /* 随机表配置，决定每组bank的轮询顺序 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 64; i++)
        {
            qlist_grp0_bank.qcfg_qlist_grp0_bank_wr = qlist_grp0_bank_data[i % 4];
            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP0_BANKr + j,
                                0,
                                i,
                                &qlist_grp0_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    for (j = 0; j < 64; j++)
    {
        qcfg_qlist_grp.qcfg_qlist_grp_wr = 0;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_GRPr,
                            0,
                            j,
                            &qcfg_qlist_grp);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    rc = dpp_tm_qmu_cfg_done_set(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_cfg_done_set");

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_1 END=======\n");

    return DPP_OK;
}




/***********************************************************/
/** QMU/MMU初始化配置场景15：每个tm只用1组*4bank，MMU实际分配ftm:0-9中指定组(0123bank); etm:0-9中指定组(0123bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/4/14
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_15(ZXIC_UINT32 dev_id, ZXIC_UINT32 etm_ddr_no, ZXIC_UINT32 ftm_ddr_no)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 etm_ddr_no_temp = 0;   /*ETM使用的ddr,bit位表示*/

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, etm_ddr_no, 0, 9);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, ftm_ddr_no, 0, 9);

    /*etm ftm不能使用同一组ddr*/
    if (etm_ddr_no == ftm_ddr_no)
    {
        ZXIC_COMM_PRINT("dpp_tm_qmu_init_set_15: etm_ddr_no can't equal to ftm_ddr_no!!!\n");
    }

    ZXIC_COMM_ASSERT(etm_ddr_no == ftm_ddr_no);

    /*将etm ftm使用的ddr号转换成对应的bit位,bit[0-9]每bit代表一组ddr */
    etm_ddr_no_temp = 0x1u << etm_ddr_no;

    /*关闭随机模式*/
    rc = dpp_tm_qmu_ddr_random_set(dev_id, 0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_ddr_random_set");

    /* etm mmu基址配置:ddr指定etm_ddr_no组，bank号0123，bank深度8K */
    //rc = dpp_mmu_bank_base_addr_set(dev_id, etm_ddr_no_temp, 0xf, 8 * 1024);
    //ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_mmu_bank_base_addr_set");

    rc = dpp_etm_qmu_init_set_15(dev_id, etm_ddr_no);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_etm_qmu_init_set");


    return DPP_OK;
}

/***********************************************************/
/** QMU初始化配置场景16：独享2组*8bank，MMU实际分配etm_ddr_no; etm or ftm使用0-7bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/12/16
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_16(ZXIC_UINT32 dev_id, ZXIC_UINT32 etm_ddr_no)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 k = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 depth = 2*1024;
    ZXIC_UINT32 bank_to_mmu_cfg_map[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    ZXIC_UINT32 qlist_grp0_bank_data[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    ZXIC_UINT32 ddr_no[2] = {(etm_ddr_no>>4)&0xf, etm_ddr_no & 0xf};


    DPP_ETM_QMU_QCFG_QLIST_BDEP_T qlist_bdep = {0};
    DPP_ETM_QMU_QCFG_QLIST_BHEAD_T qlist_bhead = {0};
    DPP_ETM_QMU_QCFG_QLIST_BTAIL_T qlist_btail = {0};

    DPP_ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFG_T active_to_bank_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_MMU_CFG_T ddr_in_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_DDR_IN_QMU_CFG_T ddr_in_qmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_MMU_CFG_T bank_to_mmu_cfg = {0};
    DPP_ETM_QMU_CFGMT_BANK_TO_QMU_CFG_T bank_to_qmu_cfg = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP0_BANK_T qlist_grp0_bank = {0};
    DPP_ETM_QMU_QCFG_QLIST_GRP_T qcfg_qlist_grp = {0};

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_16 start========n");


    /* ddr组从qmu到mmu映射,0~1映射成12 */
    for (i = 0; i < 2; i++)
    {
        ddr_in_mmu_cfg.cfgmt_ddr_in_mmu_cfg = ddr_no[i];

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_MMU_CFGr,
                            0,
                            i,
                            &ddr_in_mmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    /* ddr组从mmu到qmu映射,12映射成0~1 */
    for (i = 0; i < 2; i++)
    {
        ddr_in_qmu_cfg.cfgmt_ddr_in_qmu_cfg = i;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_DDR_IN_QMU_CFGr,
                            0,
                            ddr_no[i],
                            &ddr_in_qmu_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }


    /* bank号从qmu到mmu映射，每组的bank0~7映射成bank0-7 */
    for (j = 0; j < 2; j++)
    {
        for (i = 0; i < 8; i++)
        {
            k = j * 8 + i;

            bank_to_mmu_cfg.cfgmt_bank_in_mmu_cfg = bank_to_mmu_cfg_map[i];

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_MMU_CFGr,
                                0,
                                k,
                                &bank_to_mmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }


    /* bank号从mmu到qmu映射，每组的bank0-7映射成bank0-7 */
    for (j = 0; j < 2; j++)
    {
        for (i = 0; i < 8; i++)
        {
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, ddr_no[j], 8);
            ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(ddr_no[j] * 8, bank_to_mmu_cfg_map[i]);
            k = ddr_no[j] * 8 + bank_to_mmu_cfg_map[i];

            bank_to_qmu_cfg.cfgmt_bank_in_qmu_cfg = i;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_CFGMT_BANK_TO_QMU_CFGr,
                                0,
                                k,
                                &bank_to_qmu_cfg);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    /* random映射ram:配置一个chunk中，bank的轮询顺序 */
    for (i = 0; i < 16; i++)
    {


        active_to_bank_cfg.cfgmt_active_to_bank_cfg = i % 8;

        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_CFGMT_ACTIVE_TO_BANK_CFGr,
                            0,
                            i,
                            &active_to_bank_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    }


    /* 首尾深度指针配置 */
    for (j = 0; j < 2; j++)
    {
        for (i = 0; i < 8; i++)
        {
            k = j * 8 + i;
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 8 + i), depth);
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, (j * 8 + i + 1), depth);
            qlist_bhead.bank_vld = 1;
            qlist_bhead.qcfg_qlist_bhead = (j * 8 + i) * depth;
            qlist_btail.qcfg_qlist_btail = ((j * 8 + i + 1) * depth - 1);
            qlist_bdep.qcfg_qlist_bdep = depth;

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BHEADr,
                                0,
                                k,
                                &qlist_bhead);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BTAILr,
                                0,
                                k,
                                &qlist_btail);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_BDEPr,
                                0,
                                k,
                                &qlist_bdep);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    for (i = 0; i < 64; i++)
    {
        qlist_bdep.qcfg_qlist_bdep = depth;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_BDEPr,
                            0,
                            i,
                            &qlist_bdep);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(ETM_QMU_QCFG_QLIST_GRP0_BANKr, 8);
    /* 随机表配置，决定每组bank的轮询顺序 */
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 64; i++)
        {
            qlist_grp0_bank.qcfg_qlist_grp0_bank_wr = qlist_grp0_bank_data[i % 8];
            rc  = dpp_reg_write(dev_id,
                                ETM_QMU_QCFG_QLIST_GRP0_BANKr + j,
                                0,
                                i,
                                &qlist_grp0_bank);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }
    }

    for (j = 0; j < 64; j++)
    {
        qcfg_qlist_grp.qcfg_qlist_grp_wr = j % 2;
        rc  = dpp_reg_write(dev_id,
                            ETM_QMU_QCFG_QLIST_GRPr,
                            0,
                            j,
                            &qcfg_qlist_grp);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }

    rc = dpp_tm_qmu_cfg_done_set(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_cfg_done_set");

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "========dpp_etm_qmu_init_set_16 END=======\n");

    return DPP_OK;
}

/***********************************************************/
/** QMU初始化配置场景16：独享2组*8bank，MMU实际分配1,2; etm or ftm使用0-7bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/12/16
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_16(ZXIC_UINT32 dev_id, ZXIC_UINT32 etm_ddr_no, ZXIC_UINT32 ftm_ddr_no)
{
    DPP_STATUS  rc = DPP_OK;

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, etm_ddr_no, 0, 0xff);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, ftm_ddr_no, 0, 0xff);
    ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id, (1UL<<(etm_ddr_no>>4&0xf)), (1UL<<(etm_ddr_no&0xf)));
    ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id, (1UL<<(ftm_ddr_no>>4&0xf)), (1UL<<(ftm_ddr_no&0xf)));

    /* etm mmu基址配置:ddr1,2组，bank号0-7，bank深度2*1024 */
    //rc = dpp_mmu_bank_base_addr_set(dev_id, ((1UL<<(etm_ddr_no>>4&0xf))+(1UL<<(etm_ddr_no&0xf)))&0x3ff, 0xff, 2*1024);
    //ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_mmu_bank_base_addr_set");

    rc = dpp_etm_qmu_init_set_16(dev_id, etm_ddr_no);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_etm_qmu_init_set_16");

    return DPP_OK;
}

#endif
#endif


#if ZXIC_REAL("TM_TMMU")

#if 0
/***********************************************************/
/** TMMU TM纯片内模式配置
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   imem_en   1纯片内
*
* @return
* @remark  说明：高有效，表示使能打开，TMMU不会再发起对MMU的读写操作，用户需要保证Cache PD全部命中。
* @see
* @author  wush      @date  2017/10/14
************************************************************/
DPP_STATUS dpp_tm_tmmu_imem_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 imem_en)
{
    DPP_STATUS  rc = DPP_OK;

    DPP_ETM_TMMU_CFGMT_TM_PURE_IMEM_EN_T pure_imem_t = {0};
    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, imem_en, 0, 1);

    pure_imem_t.cfgmt_tm_pure_imem_en = imem_en;
    rc  = dpp_reg_write(dev_id,
                        ETM_TMMU_CFGMT_TM_PURE_IMEM_ENr,
                        0,
                        0,
                        &pure_imem_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** TMMU TM纯片内模式配置获取
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_imem_en  1纯片内
*
* @return
* @remark  无
* @see
* @author  wush      @date  2017/10/14
************************************************************/
DPP_STATUS dpp_tm_tmmu_imem_en_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_imem_en)
{
    DPP_STATUS  rc = DPP_OK;

    DPP_ETM_TMMU_CFGMT_TM_PURE_IMEM_EN_T pure_imem_t = {0};
    
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_imem_en);

    rc  = dpp_reg_read(dev_id,
                       ETM_TMMU_CFGMT_TM_PURE_IMEM_ENr,
                       0,
                       0,
                       &pure_imem_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    *p_imem_en = pure_imem_t.cfgmt_tm_pure_imem_en;

    return DPP_OK;
}

/***********************************************************/
/** TMMU 强制DDR RDY配置
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   ddr_force_rdy   1、如果bit【0】配置为1，则QMU看到的DDR0 RDY一直为1。
                           2、bit【0】代表DDR0，bit【7】代表DDR7。
                           3、纯片内模式需要配置为8'hff，排除DDR干扰。
*
* @return
* @remark
* @see
* @author  wush      @date  2017/10/14
************************************************************/
DPP_STATUS dpp_tm_tmmu_ddr_force_rdy_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 ddr_force_rdy)
{
    DPP_STATUS  rc = DPP_OK;

    DPP_ETM_TMMU_CFGMT_FORCE_DDR_RDY_CFG_T ddr_force_rdy_t = {0};
    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, ddr_force_rdy, 0, 0x3FF);


    ddr_force_rdy_t.cfgmt_force_ddr_rdy_cfg = ddr_force_rdy;
    rc  = dpp_reg_write(dev_id,
                        ETM_TMMU_CFGMT_FORCE_DDR_RDY_CFGr,
                        0,
                        0,
                        &ddr_force_rdy_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** TMMU 强制DDR RDY配置获取
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_ddr_force_rdy  1、如果bit【0】配置为1，则QMU看到的DDR0 RDY一直为1。
                            2、bit【0】代表DDR0，bit【7】代表DDR7。
                            3、纯片内模式需要配置为8'hff，排除DDR干扰。
*
* @return
* @remark  无
* @see
* @author  wush      @date  2017/10/14
************************************************************/
DPP_STATUS dpp_tm_tmmu_ddr_force_rdy_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_ddr_force_rdy)
{
    DPP_STATUS  rc = DPP_OK;

    DPP_ETM_TMMU_CFGMT_FORCE_DDR_RDY_CFG_T ddr_force_rdy_t = {0};
    
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_ddr_force_rdy);

    rc  = dpp_reg_read(dev_id,
                       ETM_TMMU_CFGMT_FORCE_DDR_RDY_CFGr,
                       0,
                       0,
                       &ddr_force_rdy_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    *p_ddr_force_rdy = ddr_force_rdy_t.cfgmt_force_ddr_rdy_cfg;

    return DPP_OK;
}

#endif
#endif


#if ZXIC_REAL("TM_CRDT")

#if 0
/***********************************************************/
/** crdt ram初始化
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/22
************************************************************/
DPP_STATUS dpp_tm_crdt_ram_init(ZXIC_UINT32 dev_id)
{


    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 read_times = 0;
    DPP_ETM_CRDT_CRDT_CFG_RAM_INIT_T crdt_ram_cfg_init_t = {0};
    DPP_ETM_CRDT_CRDT_STA_RAM_INIT_T crdt_ram_sta_init_t = {0};

    /**RAM初始化**/
    crdt_ram_cfg_init_t.cfg_ram_init_en = 1;
    rc = dpp_reg_write(dev_id,
                       ETM_CRDT_CRDT_CFG_RAM_INITr,
                       0,
                       0,
                       &crdt_ram_cfg_init_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    crdt_ram_sta_init_t.sta_ram_init_en = 1;
    rc = dpp_reg_write(dev_id,
                       ETM_CRDT_CRDT_STA_RAM_INITr,
                       0,
                       0,
                       &crdt_ram_sta_init_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");



    /**初始化done确认**/
    do
    {
        rc = dpp_reg_read(dev_id,
                          ETM_CRDT_CRDT_CFG_RAM_INITr,
                          0,
                          0,
                          &crdt_ram_cfg_init_t);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

        rc = dpp_reg_read(dev_id,
                          ETM_CRDT_CRDT_STA_RAM_INITr,
                          0,
                          0,
                          &crdt_ram_sta_init_t);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
        read_times++;
        zxic_comm_usleep(100);

    }
    while (0 == crdt_ram_cfg_init_t.cfg_ram_init_done  || 0 == crdt_ram_sta_init_t.sta_ram_init_done);

    ZXIC_COMM_PRINT("wait_crdt_ram_done_times=%d\n",read_times);
    ZXIC_COMM_PRINT("crdt_ram_cfg_init_t.cfg_ram_init_done=%d, crdt_ram_sta_init_t.sta_ram_init_done=%d\n",
            crdt_ram_cfg_init_t.cfg_ram_init_done,crdt_ram_sta_init_t.sta_ram_init_done);

    return DPP_OK;

}

#endif
/***********************************************************/
/** 分配etm-FQ类型调度器资源：fq/fq2/fq4/fq8 个数，(共16K= 16384)
* @param   dev_id   设备编号
* @param   fq_num   FQ调度器个数，须是8的倍数
* @param   fq2_num  FQ2调度器个数，须是4的倍数
* @param   fq4_num  FQ4调度器个数，须是2的倍数
* @param   fq8_num  FQ8调度器个数
*          调度器总数不能超过：16K= 16384
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/26
************************************************************/
DPP_STATUS dpp_etm_crdt_fq_set(DPP_DEV_T *dev,
                               ZXIC_UINT32 fq_num,
                               ZXIC_UINT32 fq2_num,
                               ZXIC_UINT32 fq4_num,
                               ZXIC_UINT32 fq8_num)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 total_fq_num = 0;
    DPP_ETM_CRDT_TH_WFQ_FQ_T   etm_crdt_th_wfqfq_t = {0};
    DPP_ETM_CRDT_TH_WFQ2_FQ2_T etm_crdt_th_wfqfq2_t = {0};
    DPP_ETM_CRDT_TH_WFQ4_FQ4_T etm_crdt_th_wfqfq4_t = {0};
    ZXIC_UINT32 th_wfq_fq_index = ETM_CRDT_TH_WFQ_FQr;
    ZXIC_UINT32 th_wfq_fq2_index = ETM_CRDT_TH_WFQ2_FQ2r;
    ZXIC_UINT32 th_wfq_fq4_index = ETM_CRDT_TH_WFQ4_FQ4r;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    /* 参数合法性检查:fq调度器总数校验 */
    ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(DEV_ID(dev),  fq2_num , 2);
    ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(DEV_ID(dev),  fq4_num , 4);
    ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(DEV_ID(dev),  fq8_num , 8);
    ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev),  fq_num , fq2_num * 2);
    ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev),  fq_num + fq2_num * 2 , fq4_num * 4);
    ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev),  fq_num + fq2_num * 2 + fq4_num * 4 , fq8_num * 8);
    
    total_fq_num = (fq_num + fq2_num * 2 + fq4_num * 4 + fq8_num * 8);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), total_fq_num, 0, DPP_ETM_FQ_NUM);

    /* 各调度器数整齐性校验 */
    if ((fq_num != 0 && fq_num % 8 != 0) || (fq2_num != 0 && fq2_num % 4 != 0) || (fq4_num != 0 && fq4_num % 2 != 0))
    {
        //ZXIC_COMM__TRACE_ERR("Bad parameter: sp_num or wfq_num %8 != 0 !");
        return DPP_ERR;
    }


    /* 开始调度器阈值配置：th_fq参数配置：th_fq = fq_num/8 */
    rc  = dpp_reg_read(dev,
                       th_wfq_fq_index,
                       0,
                       0,
                       &etm_crdt_th_wfqfq_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    etm_crdt_th_wfqfq_t.th_fq = (fq_num / 8);
    rc  = dpp_reg_write(dev,
                        th_wfq_fq_index,
                        0,
                        0,
                        &etm_crdt_th_wfqfq_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    /* th_fq2参数配置：th_fq2 = (th_fq + fq2_num/4) */
    rc  = dpp_reg_read(dev,
                       th_wfq_fq2_index,
                       0,
                       0,
                       &etm_crdt_th_wfqfq2_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    etm_crdt_th_wfqfq2_t.th_fq2 = (etm_crdt_th_wfqfq_t.th_fq + fq2_num / 4);
    rc  = dpp_reg_write(dev,
                        th_wfq_fq2_index,
                        0,
                        0,
                        &etm_crdt_th_wfqfq2_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    /* th_fq4参数配置：th_fq4 = (th_fq2 + fq4_num/2) */
    rc  = dpp_reg_read(dev,
                       th_wfq_fq4_index,
                       0,
                       0,
                       &etm_crdt_th_wfqfq4_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    etm_crdt_th_wfqfq4_t.th_fq4 = (etm_crdt_th_wfqfq2_t.th_fq2 + fq4_num /  2);
    rc  = dpp_reg_write(dev,
                        th_wfq_fq4_index,
                        0,
                        0,
                        &etm_crdt_th_wfqfq4_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;

}

/***********************************************************/
/** 分配TM-SP/WFQ类型调度器资源：sp/wfq/wfq2/wfq4/wfq8 个数，（etm共9K=9216,ftm共1920个）
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   sp_num   SP调度器个数，须是8的倍数
* @param   wfq_num  WFQ调度器个数，须是8的倍数
* @param   wfq2_num  WFQ2调度器个数，须是4的倍数
* @param   wfq4_num  WFQ4调度器个数，须是2的倍数
* @param   wfq8_num  WFQ8调度器个数
*          调度器总数不能超过：ETM= 9216; FTM= 1920
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/26
************************************************************/
DPP_STATUS dpp_tm_crdt_wfqsp_set(DPP_DEV_T *dev,
                                 ZXIC_UINT32 sp_num,
                                 ZXIC_UINT32 wfq_num,
                                 ZXIC_UINT32 wfq2_num,
                                 ZXIC_UINT32 wfq4_num,
                                 ZXIC_UINT32 wfq8_num)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 total_wfqsp_num = 0;
    DPP_ETM_CRDT_TH_SP_T       etm_crdt_th_sp_t = {0};
    DPP_ETM_CRDT_TH_WFQ_FQ_T   etm_crdt_th_wfqfq_t = {0};
    DPP_ETM_CRDT_TH_WFQ2_FQ2_T etm_crdt_th_wfqfq2_t = {0};
    DPP_ETM_CRDT_TH_WFQ4_FQ4_T etm_crdt_th_wfqfq4_t = {0};
    ZXIC_UINT32 th_sp_index = ETM_CRDT_TH_SPr;
    ZXIC_UINT32 th_wfq_fq_index = ETM_CRDT_TH_WFQ_FQr;
    ZXIC_UINT32 th_wfq_fq2_index = ETM_CRDT_TH_WFQ2_FQ2r;
    ZXIC_UINT32 th_wfq_fq4_index = ETM_CRDT_TH_WFQ4_FQ4r;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    /* 参数合法性检查 */    
    ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(DEV_ID(dev),  wfq2_num , 2);
    ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(DEV_ID(dev),  wfq4_num , 4);
    ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(DEV_ID(dev),  wfq8_num , 8);
    ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev), sp_num , wfq_num);
    ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev), sp_num + wfq_num , wfq2_num * 2);
    ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev), sp_num + wfq_num + wfq2_num * 2 , wfq4_num * 4);
    ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev), sp_num + wfq_num + wfq2_num * 2 + wfq4_num * 4 , wfq8_num * 8);
    /* 调度器总数校验 */
    total_wfqsp_num = (sp_num + wfq_num + wfq2_num * 2 + wfq4_num * 4 + wfq8_num * 8);

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), total_wfqsp_num, 0, DPP_ETM_WFQSP_NUM);

    /* 各调度器数整齐性校验 */
    if ((sp_num != 0 && sp_num % 8 != 0) || (wfq_num != 0 && wfq_num % 8 != 0) ||
        (wfq2_num != 0 && wfq2_num % 4 != 0) || (wfq4_num != 0 && wfq4_num % 2 != 0))
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "Bad parameter: sp_num or wfq_num mod 8 != 0 !\n");
        return DPP_ERR;
    }

    th_sp_index = ETM_CRDT_TH_SPr;
    th_wfq_fq_index = ETM_CRDT_TH_WFQ_FQr;
    th_wfq_fq2_index = ETM_CRDT_TH_WFQ2_FQ2r;
    th_wfq_fq4_index = ETM_CRDT_TH_WFQ4_FQ4r;

    /* 开始调度器阈值配置：th_sp参数配置：th_sp=      sp_num/8 */
    rc  = dpp_reg_read(dev,
                       th_sp_index,
                       0,
                       0,
                       &etm_crdt_th_sp_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    etm_crdt_th_sp_t.th_sp = (sp_num / 8);
    rc  = dpp_reg_write(dev,
                        th_sp_index,
                        0,
                        0,
                        &etm_crdt_th_sp_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    /* th_wfq参数配置：th_wfq = (th_sp + wfq_num/8)       */
    rc  = dpp_reg_read(dev,
                       th_wfq_fq_index,
                       0,
                       0,
                       &etm_crdt_th_wfqfq_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    etm_crdt_th_wfqfq_t.th_wfq = (etm_crdt_th_sp_t.th_sp + wfq_num / 8);
    rc  = dpp_reg_write(dev,
                        th_wfq_fq_index,
                        0,
                        0,
                        &etm_crdt_th_wfqfq_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    /* th_wfq2参数配置：th_wfq2 = (th_wfq +      wfq2_num/4) */
    rc  = dpp_reg_read(dev,
                       th_wfq_fq2_index,
                       0,
                       0,
                       &etm_crdt_th_wfqfq2_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    etm_crdt_th_wfqfq2_t.th_wfq2 = (etm_crdt_th_wfqfq_t.th_wfq + wfq2_num / 4);
    rc  = dpp_reg_write(dev,
                        th_wfq_fq2_index,
                        0,
                        0,
                        &etm_crdt_th_wfqfq2_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    /* th_wfq4参数配置：th_wfq4 = (th_wfq2 +      wfq4_num/2) */
    rc  = dpp_reg_read(dev,
                       th_wfq_fq4_index,
                       0,
                       0,
                       &etm_crdt_th_wfqfq4_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    etm_crdt_th_wfqfq4_t.th_wfq4 = (etm_crdt_th_wfqfq2_t.th_wfq2 + wfq4_num / 2);
    rc  = dpp_reg_write(dev,
                        th_wfq_fq4_index,
                        0,
                        0,
                        &etm_crdt_th_wfqfq4_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;

}


/***********************************************************/
/** 获取各调度器的起始编号（etm共25K=25600,ftm共1920个）
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_spwfq_start_num  调度器起始编号结构体
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/26
************************************************************/
DPP_STATUS dpp_tm_crdt_wfqsp_get(DPP_DEV_T *dev,
                                 DPP_TM_CRDT_SPWFQ_START_NUM_T *p_spwfq_start_num)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CRDT_TH_SP_T       etm_crdt_th_sp_t = {0};
    DPP_ETM_CRDT_TH_WFQ_FQ_T   etm_crdt_th_wfqfq_t = {0};
    DPP_ETM_CRDT_TH_WFQ2_FQ2_T etm_crdt_th_wfqfq2_t = {0};
    DPP_ETM_CRDT_TH_WFQ4_FQ4_T etm_crdt_th_wfqfq4_t = {0};
    ZXIC_UINT32 th_sp_index = ETM_CRDT_TH_SPr;
    ZXIC_UINT32 th_wfq_fq_index = ETM_CRDT_TH_WFQ_FQr;
    ZXIC_UINT32 th_wfq_fq2_index = ETM_CRDT_TH_WFQ2_FQ2r;
    ZXIC_UINT32 th_wfq_fq4_index = ETM_CRDT_TH_WFQ4_FQ4r;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_spwfq_start_num);

    /* 读取调度器阈值配置：th_sp参数配置：th_sp=      sp_num/8 */
    rc  = dpp_reg_read(dev,
                       th_sp_index,
                       0,
                       0,
                       &etm_crdt_th_sp_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
    /* th_wfq阈值读取：th_wfq = (th_sp + wfq_num/8)       */
    rc  = dpp_reg_read(dev,
                       th_wfq_fq_index,
                       0,
                       0,
                       &etm_crdt_th_wfqfq_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
    /* th_wfq2阈值读取：th_wfq2 = (th_wfq +      wfq2_num/4) */
    rc  = dpp_reg_read(dev,
                       th_wfq_fq2_index,
                       0,
                       0,
                       &etm_crdt_th_wfqfq2_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
    /* th_wfq4阈值读取：th_wfq4 = (th_wfq2 +      wfq4_num/2) */
    rc  = dpp_reg_read(dev,
                       th_wfq_fq4_index,
                       0,
                       0,
                       &etm_crdt_th_wfqfq4_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    /* 各调度器起始编号计算 */
    p_spwfq_start_num->start_num_fq = 0;
    p_spwfq_start_num->start_num_fq2 = etm_crdt_th_wfqfq_t.th_fq * 8;
    p_spwfq_start_num->start_num_fq4 = etm_crdt_th_wfqfq2_t.th_fq2 * 8;
    p_spwfq_start_num->start_num_fq8 = etm_crdt_th_wfqfq4_t.th_fq4 * 8;
    p_spwfq_start_num->start_num_sp = DPP_ETM_WFQSP_OFFSET;
    p_spwfq_start_num->start_num_wfq = (DPP_ETM_WFQSP_OFFSET + etm_crdt_th_sp_t.th_sp * 8);
    p_spwfq_start_num->start_num_wfq2 = (DPP_ETM_WFQSP_OFFSET + etm_crdt_th_wfqfq_t.th_wfq * 8);
    p_spwfq_start_num->start_num_wfq4 = (DPP_ETM_WFQSP_OFFSET + etm_crdt_th_wfqfq2_t.th_wfq2 * 8);
    p_spwfq_start_num->start_num_wfq8 = (DPP_ETM_WFQSP_OFFSET + etm_crdt_th_wfqfq4_t.th_wfq4 * 8);

    return DPP_OK;


}


/***********************************************************/
/** 获取调度器类型
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   se_id     调度器编号
* @param   item_num  调度器中包含的子调度器个数
* @param   sch_type_num  调度器类型编号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/26
************************************************************/
DPP_STATUS dpp_tm_crdt_sch_type_get(DPP_DEV_T *dev, ZXIC_UINT32 se_id, ZXIC_UINT32 *item_num, ZXIC_UINT32 *sch_type_num)
{
    DPP_STATUS  rc = DPP_OK;

    DPP_TM_CRDT_SPWFQ_START_NUM_T spwfq_start_num_t = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), item_num);

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_id, 0, DPP_ETM_FQSPWFQ_NUM - 1);


    rc = dpp_tm_crdt_wfqsp_get(dev, &spwfq_start_num_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_crdt_wfqsp_get");


    if (se_id < spwfq_start_num_t.start_num_fq2)
    {
        *item_num = 1;
        *sch_type_num = 5;
    }
    else if (se_id < spwfq_start_num_t.start_num_fq4)
    {
        *item_num = 2;
        *sch_type_num = 6;
    }
    else if (se_id < spwfq_start_num_t.start_num_fq8)
    {
        *item_num = 4;
        *sch_type_num = 7;
    }
    else if (se_id < spwfq_start_num_t.start_num_sp)
    {
        *item_num = 8;
        *sch_type_num = 8;
    }
    else if (se_id < spwfq_start_num_t.start_num_wfq)
    {
        *item_num = 1;
        *sch_type_num = 0;
    }
    else if (se_id < spwfq_start_num_t.start_num_wfq2)
    {
        *item_num = 1;
        *sch_type_num = 1;
    }
    else if (se_id < spwfq_start_num_t.start_num_wfq4)
    {
        *item_num = 2;
        *sch_type_num = 2;
    }
    else if (se_id < spwfq_start_num_t.start_num_wfq8)
    {
        *item_num = 4;
        *sch_type_num = 3;
    }
    else
    {
        *item_num = 8;
        *sch_type_num = 4;
    }

    return DPP_OK;

}


/***********************************************************/
/** 获取pp->dev挂接关系
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   pp_id   0~63
* @param   p_weight  0~127
* @param   p_sp_mapping   0~7
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/04/20
************************************************************/
DPP_STATUS dpp_tm_crdt_pp_para_get(DPP_DEV_T *dev,
                                   ZXIC_UINT32 pp_id,
                                   ZXIC_UINT32 *p_weight,
                                   ZXIC_UINT32 *p_sp_mapping)
{
    DPP_STATUS  rc = DPP_OK;

    DPP_ETM_CRDT_PP_CFG_T pp_cfg_r = {0};
    DPP_ETM_CRDT_PP_WEIGHT_T pp_weight_r = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), pp_id, 0, DPP_TM_PP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_weight);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_sp_mapping);


    rc  = dpp_reg_read(dev,
                       ETM_CRDT_PP_WEIGHTr,
                       0,
                       pp_id,
                       &pp_weight_r);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
    *p_weight = pp_weight_r.pp_weight;

    rc  = dpp_reg_read(dev,
                       ETM_CRDT_PP_CFGr,
                       0,
                       pp_id,
                       &pp_cfg_r);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
    *p_sp_mapping = pp_cfg_r.pp_cfg;

    return DPP_OK;
}

/***********************************************************/
/** 配置se->pp->dev挂接关系
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   se_id   往端口挂接的调度器id
* @param   pp_id   [0-63]
* @param   weight  [1-511]
* @param   sp_mapping   [0~8]
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/3/4
************************************************************/
DPP_STATUS dpp_tm_crdt_se_pp_link_set(DPP_DEV_T *dev,
                                      ZXIC_UINT32 se_id,
                                      ZXIC_UINT32 pp_id,
                                      ZXIC_UINT32 weight,
                                      ZXIC_UINT32 sp_mapping)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 delay_time = 10;

    DPP_ETM_CRDT_PP_CFG_T pp_cfg = {0};
    DPP_ETM_CRDT_PP_WEIGHT_T pp_weight = {0};
    DPP_ETM_CRDT_PP_CFG_T pp_cfg_r = {0};
    DPP_ETM_CRDT_PP_WEIGHT_T pp_weight_r = {0};
    DPP_TM_SCH_SE_PARA_T sch_se_para_t = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_id, 0, DPP_ETM_FQSPWFQ_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), pp_id, 0, DPP_TM_PP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), weight, 0, DPP_TM_SCH_WEIGHT_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), sp_mapping, DPP_TM_SCH_SP_0, DPP_TM_SCH_SP_8);

    /* 参数赋值:仅需端口号 ,配置调度器到端口的挂接 */
    sch_se_para_t.se_linkid = DPP_ETM_PORT_LINKID_BASE + pp_id;

    rc = dpp_tm_crdt_se_link_wr(dev, se_id, &sch_se_para_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_crdt_se_link_wr");

    /* 检测CRDT寄存器是否空闲 */
    rc  = dpp_tm_crdt_idle_check(dev);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_crdt_idle_check");

    pp_weight.pp_weight = weight;
    rc  = dpp_reg_write(dev,
                        ETM_CRDT_PP_WEIGHTr,
                        0,
                        pp_id,
                        &pp_weight);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    /* 写入后，重新读出来校验 */
#if (ETM_WRITE_CHECK)
    {
        zxic_comm_delay(delay_time);
        rc  = dpp_reg_read(dev,
                           ETM_CRDT_PP_WEIGHTr,
                           0,
                           pp_id,
                           &pp_weight_r);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

        if (pp_weight_r.pp_weight != pp_weight.pp_weight)
        {
            ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "dpp_tm_crdt_pp_para_set pp[0x%x] wt_pp_weight[0x%x] rd_pp_weight[0x%x]\n", pp_id, pp_weight.pp_weight, pp_weight_r.pp_weight);
            return DPP_ERR;
        }
    }
#endif

    /* 检测CRDT寄存器是否空闲 */
    rc  = dpp_tm_crdt_idle_check(dev);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_crdt_idle_check");

    pp_cfg.pp_cfg = sp_mapping;
    rc  = dpp_reg_write(dev,
                        ETM_CRDT_PP_CFGr,
                        0,
                        pp_id,
                        &pp_cfg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    /* 写入后，重新读出来校验 */
#if (ETM_WRITE_CHECK)
    {
        zxic_comm_delay(delay_time);
        rc  = dpp_reg_read(dev,
                           ETM_CRDT_PP_CFGr,
                           0,
                           pp_id,
                           &pp_cfg_r);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

        if (pp_cfg_r.pp_cfg != pp_cfg.pp_cfg)
        {
            ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "dpp_tm_crdt_pp_para_set pp[0x%x] wt_pp_cfg[0x%x] rd_pp_cfg[0x%x]\n", pp_id, pp_cfg.pp_cfg, pp_cfg_r.pp_cfg);
            return DPP_ERR;
        }
    }
#endif

    return DPP_OK;
}

/***********************************************************/
/** 配置flow级流队列的挂接关系(flow到上级调度器的挂接)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   flow_id   流队列号
* @param   c_linkid  c桶要挂接到的上级调度器id
* @param   c_weight  c桶挂接到上级调度器的权重[1~511]
* @param   c_sp      c桶挂接到上级调度器的sp优先级,有效值[0-8],共9级，优先级依次降低
* @param   mode      挂接模式：0-单桶 1-双桶。配置单桶时无需关注后续参数，配0即可
* @param   e_linkid  e桶要挂接到的上级调度器id
* @param   e_weight  e桶挂接到上级调度器的权重[1~511]
* @param   e_sp      e桶挂接到上级调度器的sp优先级，有效值[0-8],共9级，优先级依次降低
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_flow_link_wr(DPP_DEV_T *dev,
                                    ZXIC_UINT32 flow_id,
                                    DPP_TM_SCH_FLOW_PARA_T *p_flow_para)

{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 flow_id_e = 0;
    ZXIC_UINT32 c_linkid;
    ZXIC_UINT32 c_weight;
    ZXIC_UINT32 c_sp;
    ZXIC_UINT32 mode;
    ZXIC_UINT32 e_linkid;
    ZXIC_UINT32 e_weight;
    ZXIC_UINT32 e_sp;
    DPP_ETM_CRDT_FLOWQUE_PARA_TBL_T etm_crdt_flow_para_tbl_t = {0};

    /* 取配置参数 */
    c_linkid = p_flow_para->c_linkid;
    c_weight = p_flow_para->c_weight;
    c_sp = p_flow_para->c_sp;
    mode = p_flow_para->mode;
    e_linkid = p_flow_para->e_linkid;
    e_weight = p_flow_para->e_weight;
    e_sp = p_flow_para->e_sp;
    /* 参数校验 */
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_flow_para);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), c_weight, 0, DPP_TM_SCH_WEIGHT_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), e_weight, 0, DPP_TM_SCH_WEIGHT_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), c_sp, 0, DPP_TM_SCH_SP_NUM);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), e_sp, 0, DPP_TM_SCH_SP_NUM);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), mode, 0, 1);

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), flow_id, 0, DPP_ETM_Q_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), c_linkid, 0, DPP_ETM_FQSPWFQ_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), e_linkid, 0, DPP_ETM_FQSPWFQ_NUM - 1);


    /* 开始流级挂接配置 */

    if (mode == 1)
    {
        flow_id_e = (flow_id + 0x2400);
    }

    /* c桶挂接配置 */
    etm_crdt_flow_para_tbl_t.flowque_link = c_linkid;
    etm_crdt_flow_para_tbl_t.flowque_w = c_weight;
    etm_crdt_flow_para_tbl_t.flowque_pri = c_sp;

    rc  = dpp_reg_write(dev,
                        ETM_CRDT_FLOWQUE_PARA_TBLr,
                        0,
                        flow_id,
                        &etm_crdt_flow_para_tbl_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    zxic_comm_delay(5);

    /* mode-1 需要配置双桶挂接 */
    if (mode == 1)
    {
        /* e桶挂接配置 */
        etm_crdt_flow_para_tbl_t.flowque_link = e_linkid;
        etm_crdt_flow_para_tbl_t.flowque_w = e_weight;
        etm_crdt_flow_para_tbl_t.flowque_pri = e_sp;

        rc  = dpp_reg_write(dev,
                            ETM_CRDT_FLOWQUE_PARA_TBLr,
                            0,
                            flow_id_e,
                            &etm_crdt_flow_para_tbl_t);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
    }

    zxic_comm_delay(5);

    return DPP_OK;


}

/***********************************************************/
/** 配置flow级流队列的挂接关系(flow到上级调度器的挂接)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   flow_id   流队列号
* @param   c_linkid  c桶要挂接到的上级调度器id
* @param   c_weight  c桶挂接到上级调度器的权重[1~511]
* @param   c_sp      c桶挂接到上级调度器的sp优先级,有效值[0-8],共9级，优先级依次降低
* @param   mode      挂接模式：0-单桶 1-双桶。配置单桶时无需关注后续参数，配0即可
* @param   e_linkid  e桶要挂接到的上级调度器id
* @param   e_weight  e桶挂接到上级调度器的权重[1~511]
* @param   e_sp      e桶挂接到上级调度器的sp优先级，有效值[0-8],共9级，优先级依次降低
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_flow_link_set(DPP_DEV_T *dev,
                                     ZXIC_UINT32 flow_id,
                                     ZXIC_UINT32 c_linkid,
                                     ZXIC_UINT32 c_weight,
                                     ZXIC_UINT32 c_sp,
                                     ZXIC_UINT32 mode,
                                     ZXIC_UINT32 e_linkid,
                                     ZXIC_UINT32 e_weight,
                                     ZXIC_UINT32 e_sp)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_TM_SCH_FLOW_PARA_T sch_flow_para_t = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    /* 参数赋值 */
    sch_flow_para_t.c_linkid = c_linkid;
    sch_flow_para_t.c_weight = c_weight;
    sch_flow_para_t.c_sp = c_sp;
    sch_flow_para_t.mode = mode;
    sch_flow_para_t.e_linkid = e_linkid;
    sch_flow_para_t.e_weight = e_weight;
    sch_flow_para_t.e_sp = e_sp;

    rc = dpp_tm_crdt_flow_link_wr(dev, flow_id, &sch_flow_para_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_crdt_flow_link_wr");

    return DPP_OK;

}

/***********************************************************/
/** 批量配置flow级流队列的挂接关系
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   flow_id_s 起始流队列号
* @param   flow_id_e 终止流队列号
* @param   c_linkid  c桶要挂接到的上级调度器id
* @param   c_weight  c桶挂接到上级调度器的权重[1~511]
* @param   c_sp      c桶挂接到上级调度器的sp优先级,有效值[0-8],共9级，优先级依次降低
* @param   mode      挂接模式：0-单桶 1-双桶。配置单桶时无需关注后续参数，配0即可
* @param   e_linkid  e桶要挂接到的上级调度器id
* @param   e_weight  e桶挂接到上级调度器的权重[1~511]
* @param   e_sp      e桶挂接到上级调度器的sp优先级，有效值[0-8],共9级，优先级依次降低
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_flow_link_more_set(DPP_DEV_T *dev,
                                          ZXIC_UINT32 flow_id_s,
                                          ZXIC_UINT32 flow_id_e,
                                          ZXIC_UINT32 c_linkid,
                                          ZXIC_UINT32 c_weight,
                                          ZXIC_UINT32 c_sp,
                                          ZXIC_UINT32 mode,
                                          ZXIC_UINT32 e_linkid,
                                          ZXIC_UINT32 e_weight,
                                          ZXIC_UINT32 e_sp)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 flow_id = 0;
    DPP_TM_SCH_FLOW_PARA_T sch_flow_para_t = {0};
    ZXIC_COMM_CHECK_POINT(dev);
    

    if (flow_id_s > flow_id_e)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "Bad parameters!  flow_id_s > flow_id_e !\n");
        return DPP_ERR;
    }

    /* 参数赋值 */
    sch_flow_para_t.c_linkid = c_linkid;
    sch_flow_para_t.c_weight = c_weight;
    sch_flow_para_t.c_sp = c_sp;
    sch_flow_para_t.mode = mode;
    sch_flow_para_t.e_linkid = e_linkid;
    sch_flow_para_t.e_weight = e_weight;
    sch_flow_para_t.e_sp = e_sp;

    for (flow_id = flow_id_s; flow_id <= flow_id_e; flow_id++)
    {
        rc = dpp_tm_crdt_flow_link_wr(dev, flow_id, &sch_flow_para_t);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_crdt_flow_link_wr");
    }

    return DPP_OK;

}


/***********************************************************/
/** 配置调度器层次化QOS的挂接关系:非优先级传递
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   se_id      本级调度器id
*                     对于FQX/WFQX必须是调度单元中首个调度器id
* @param   se_linkid  要挂接到的上级调度器id
* @param   se_weight  挂接到上级调度器的权重[1~511]
* @param   se_sp      挂接到上级调度器的sp优先级,有效值[0-8],共9级，优先级依次降低
* @param   se_insw    优先级传递使能：0-关 1-开。该参数不传递直接配0
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_se_link_wr(DPP_DEV_T *dev,
                                  ZXIC_UINT32 se_id,
                                  DPP_TM_SCH_SE_PARA_T *p_sch_se_para)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 se_linkid = 0;
    ZXIC_UINT32 se_weight = 0;
    ZXIC_UINT32 se_sp = 0;
    ZXIC_UINT32 se_insw = 0;     /* 优先级传递关闭 */
    ZXIC_UINT32 item_num = 0;    /* 调度单元中调度器的个数 */
    ZXIC_UINT32 sch_type_num = 0;
    ZXIC_UINT32 i = 0;
    DPP_ETM_CRDT_SE_PARA_TBL_T    etm_crdt_se_para_tbl_t = {0};


    /* 取配置参数 */
    se_linkid = p_sch_se_para->se_linkid;
    se_weight = p_sch_se_para->se_weight;
    se_sp = p_sch_se_para->se_sp;


    /* 参数校验 */
    ZXIC_COMM_CHECK_POINT(dev);    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_weight, 0, DPP_TM_SCH_WEIGHT_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_sp, 0, DPP_TM_SCH_SP_NUM);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_insw, 0, 0);

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_id, 0, DPP_ETM_FQSPWFQ_NUM - 1);

    if (se_linkid > DPP_ETM_FQSPWFQ_NUM)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_linkid, DPP_TM_PP_LINKID_PORT0, DPP_TM_PP_LINKID_PORT63);
    }
    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_linkid, 0, DPP_ETM_FQSPWFQ_NUM - 1);
    }


    /* 开始调度器挂接配置 */

    /* 先区分调度器类型：sp/fq/wfq调度器挂接方式相同，wfqx/fqx=2/4/8是另一种挂接方式 */
    rc = dpp_tm_crdt_sch_type_get(dev, se_id, &item_num, &sch_type_num);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_crdt_sch_type_get");


    /* 非优先级传递挂接:各调度器参数须相同 */
    etm_crdt_se_para_tbl_t.se_link = se_linkid;
    etm_crdt_se_para_tbl_t.se_w = se_weight;
    etm_crdt_se_para_tbl_t.se_pri = se_sp;
    etm_crdt_se_para_tbl_t.se_insw = se_insw;
    etm_crdt_se_para_tbl_t.cp_token_en = 1;

    for (i = 0; i < item_num; i++)
    {
        rc  = dpp_reg_write(dev,
                            ETM_CRDT_SE_PARA_TBLr,
                            0,
                            se_id + i,
                            &etm_crdt_se_para_tbl_t);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
    }

    return DPP_OK;
}


/***********************************************************/
/** 配置调度器层次化QOS的挂接关系:非优先级传递
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   se_id      本级调度器id
*                     对于FQX/WFQX必须是调度单元中首个调度器id
* @param   se_linkid  要挂接到的上级调度器id
* @param   se_weight  挂接到上级调度器的权重[1~511]
* @param   se_sp      挂接到上级调度器的sp优先级,有效值[0-8],共9级，优先级依次降低
* @param   se_insw    优先级传递使能：0-关 1-开. 该参数不传递直接配0
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_se_link_set(DPP_DEV_T *dev,
                                   ZXIC_UINT32 se_id,
                                   ZXIC_UINT32 se_linkid,
                                   ZXIC_UINT32 se_weight,
                                   ZXIC_UINT32 se_sp)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_TM_SCH_SE_PARA_T sch_se_para_t = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    /* 参数赋值 */
    sch_se_para_t.se_linkid = se_linkid;
    sch_se_para_t.se_weight = se_weight;
    sch_se_para_t.se_sp = se_sp;

    rc = dpp_tm_crdt_se_link_wr(dev, se_id, &sch_se_para_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_crdt_se_link_wr");

    return DPP_OK;
}

/***********************************************************/
/** 配置调度器层次化QOS的挂接关系:优先级传递
* @param   dev_id       设备编号
* @param   tm_type      0-ETM,1-FTM
* @param   se_id        本级调度器id
* @param   se_linkid    要挂接到的上级调度器id
* @param   se_sp        挂接到上级调度器的sp优先级,有效值[0-3],最多4级，优先级按调度单元分配，
*                       每个调度单元内部调度器优先级相同！
* @param   se_weight0-7 WFQ8中各调度器权重值[1~511]，若是WFQ2/4 只取前面对应值，后面无效
* @param   se_insw      优先级传递使能：0-关 1-开. 该参数不传递直接配1
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_se_link_insw_wr(DPP_DEV_T *dev,
                                       ZXIC_UINT32 se_id,
                                       DPP_TM_SCH_SE_PARA_INSW_T *p_sch_se_para_insw)
{
    ZXIC_UINT32  rc = DPP_OK;
    ZXIC_UINT32  se_linkid;          /**  要挂接到的上级调度器id */
    ZXIC_UINT32  se_sp;              /** 挂接到上级调度器的sp优先级,有效值[0-3],共4级，优先级依次降低,优先级按调度单元分配 */
    ZXIC_UINT32  se_weight[8] = {0}; /** WFQ8中各调度器权重值[1~511]，若是WFQ2/4 只取前面对应值，后面无效 */
    ZXIC_UINT32  se_insw = 1;        /* 优先级传递开启 */
    ZXIC_UINT32  item_num = 1;       /* 调度单元中调度器的个数 */
    ZXIC_UINT32  sch_type_num = 0;
    ZXIC_UINT32  item_num_link = 0;       /* 上级调度单元中调度器的个数 */
    ZXIC_UINT32  sch_type_num_link = 0;
    ZXIC_UINT32  i = 0;
    DPP_ETM_CRDT_SE_PARA_TBL_T    etm_crdt_se_para_tbl_t = {0};

    /* 取配置参数 */
    se_linkid = p_sch_se_para_insw->se_linkid;
    se_sp = p_sch_se_para_insw->se_sp;
    se_weight[0] = p_sch_se_para_insw->se_weight[0];
    se_weight[1] = p_sch_se_para_insw->se_weight[1];
    se_weight[2] = p_sch_se_para_insw->se_weight[2];
    se_weight[3] = p_sch_se_para_insw->se_weight[3];
    se_weight[4] = p_sch_se_para_insw->se_weight[4];
    se_weight[5] = p_sch_se_para_insw->se_weight[5];
    se_weight[6] = p_sch_se_para_insw->se_weight[6];
    se_weight[7] = p_sch_se_para_insw->se_weight[7];


    /* 参数校验 */
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_sp, 0, DPP_TM_SCH_SP_NUM - 5);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_insw, 1, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_weight[0], 0, DPP_TM_SCH_WEIGHT_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_weight[1], 0, DPP_TM_SCH_WEIGHT_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_weight[2], 0, DPP_TM_SCH_WEIGHT_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_weight[3], 0, DPP_TM_SCH_WEIGHT_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_weight[4], 0, DPP_TM_SCH_WEIGHT_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_weight[5], 0, DPP_TM_SCH_WEIGHT_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_weight[6], 0, DPP_TM_SCH_WEIGHT_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_weight[7], 0, DPP_TM_SCH_WEIGHT_MAX);

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_id, 0, DPP_ETM_FQSPWFQ_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_linkid, 0, DPP_ETM_FQSPWFQ_NUM - 1);



    /* 开始调度器挂接配置 */

    /* 先区分调度器类型：sp/fq/wfq调度器挂接方式相同，wfqx/fqx=2/4/8是另一种挂接方式 */
    rc = dpp_tm_crdt_sch_type_get(dev, se_id, &item_num, &sch_type_num);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_crdt_sch_type_get");
    rc = dpp_tm_crdt_sch_type_get(dev, se_linkid, &item_num_link, &sch_type_num_link);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_crdt_sch_type_get");

    /* 优先级传递挂接合法性检查:下级传入调度器需是首编号，且下级<=上级 */
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), item_num, 1, 8);

    if (se_id % item_num != 0 || item_num > item_num_link)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "dpp_tm_crdt_se_link_insw_wr： NOT CORRECT,bad parameters!\n");
        return DPP_ERR;
    }


    /* 优先级传递挂接:各调度器挂接的se_linkid依次递增1,需相邻不能错开
                      se_weight不限，取值[1-511]
                      se_sp:根据下级往上级挂接情况，取值[0-3]，调度单元内部须相同
                      se_insw 写死为1 */
    etm_crdt_se_para_tbl_t.se_pri = se_sp;
    etm_crdt_se_para_tbl_t.se_insw = se_insw;
    etm_crdt_se_para_tbl_t.cp_token_en = 1;

    for (i = 0; i < item_num; i++)
    {
        etm_crdt_se_para_tbl_t.se_link = (se_linkid + i);
        etm_crdt_se_para_tbl_t.se_w = se_weight[i];

        rc  = dpp_reg_write(dev,
                            ETM_CRDT_SE_PARA_TBLr,
                            0,
                            se_id + i,
                            &etm_crdt_se_para_tbl_t);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
    }

    return DPP_OK;
}


/***********************************************************/
/** 配置调度器层次化QOS的挂接关系:优先级传递
* @param   dev_id       设备编号
* @param   tm_type      0-ETM,1-FTM
* @param   se_id        本级调度器id
* @param   se_linkid    要挂接到的上级调度器id
* @param   se_weight    WFQ2/4/8中各调度器权重值[1~511]，取相等的值
* @param   se_sp        挂接到上级调度器的sp优先级,有效值[0-7],共8级，优先级依次降低
* @param   se_insw      优先级传递使能：0-关 1-开. 该参数不传递直接配1
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_se_link_insw_set(DPP_DEV_T *dev,
                                        ZXIC_UINT32 se_id,
                                        ZXIC_UINT32 se_linkid,
                                        ZXIC_UINT32 se_weight,
                                        ZXIC_UINT32 se_sp)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_TM_SCH_SE_PARA_INSW_T sch_se_para_insw_t = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    /* 参数赋值 */
    sch_se_para_insw_t.se_linkid = se_linkid;
    sch_se_para_insw_t.se_sp = se_sp;
    sch_se_para_insw_t.se_weight[0] = se_weight;
    sch_se_para_insw_t.se_weight[1] = se_weight;
    sch_se_para_insw_t.se_weight[2] = se_weight;
    sch_se_para_insw_t.se_weight[3] = se_weight;
    sch_se_para_insw_t.se_weight[4] = se_weight;
    sch_se_para_insw_t.se_weight[5] = se_weight;
    sch_se_para_insw_t.se_weight[6] = se_weight;
    sch_se_para_insw_t.se_weight[7] = se_weight;

    rc = dpp_tm_crdt_se_link_insw_wr(dev, se_id, &sch_se_para_insw_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_crdt_se_link_insw_wr");

    return DPP_OK;


}


/***********************************************************/
/** 配置调度器层次化QOS的挂接关系:优先级传递,单个调度器挂接
* @param   dev_id       设备编号
* @param   tm_type      0-ETM,1-FTM
* @param   se_id        本级调度器id
* @param   se_linkid    要挂接到的上级调度器id
* @param   se_weight    WFQ8中对应调度器权重值[1~511]
* @param   se_sp        挂接到上级调度器的sp优先级,有效值[0-7],共8级，优先级依次降低
* @param   se_insw      优先级传递使能：0-关 1-开. 该参数不传递直接配1
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_se_link_insw_single_set(DPP_DEV_T *dev,
                                               ZXIC_UINT32 se_id,
                                               ZXIC_UINT32 se_linkid,
                                               ZXIC_UINT32 se_weight,
                                               ZXIC_UINT32 se_sp)
{
    ZXIC_UINT32  rc = DPP_OK;
    ZXIC_UINT32  se_insw = 1;        /* 优先级传递开启 */
    DPP_ETM_CRDT_SE_PARA_TBL_T    etm_crdt_se_para_tbl_t = {0};


    /* 参数校验 */
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_sp, 0, DPP_TM_SCH_SP_NUM - 5);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_insw, 1, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_weight, 0, DPP_TM_SCH_WEIGHT_MAX);


    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_id, 0, DPP_ETM_FQSPWFQ_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_linkid, 0, DPP_ETM_FQSPWFQ_NUM - 1);


    /* 开始调度器挂接配置 */

    /* 优先级传递挂接:各调度器挂接的se_linkid依次递增1,需相邻不能错开
                      se_weight不限，取值[1-511]
                      se_sp:根据下级往上级挂接情况，取值[0-3]，调度单元内部须相同
                      se_insw 写死为1 */
    etm_crdt_se_para_tbl_t.se_pri = se_sp;
    etm_crdt_se_para_tbl_t.se_insw = se_insw;
    etm_crdt_se_para_tbl_t.cp_token_en = 1;
    etm_crdt_se_para_tbl_t.se_link = se_linkid;
    etm_crdt_se_para_tbl_t.se_w = se_weight;

    rc  = dpp_reg_write(dev,
                        ETM_CRDT_SE_PARA_TBLr,
                        0,
                        se_id,
                        &etm_crdt_se_para_tbl_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");


    return DPP_OK;


}

/***********************************************************/
/** 获取调度器挂接配置参数
* @param   dev_id      设备编号
* @param   tm_type     0-ETM,1-FTM
* @param   se_id       调度器编号
* @param   p_se_para_tbl  调度器参数
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_se_para_get(DPP_DEV_T *dev, ZXIC_UINT32 se_id, DPP_ETM_CRDT_SE_PARA_TBL_T *p_se_para_tbl)
{
    DPP_STATUS  rc = DPP_OK;

    /* 参数校验 */
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_se_para_tbl);

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_id, 0, DPP_ETM_FQSPWFQ_NUM - 1);

    rc = dpp_reg_read(dev,
                      ETM_CRDT_SE_PARA_TBLr,
                      0,
                      se_id,
                      p_se_para_tbl);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    return DPP_OK;

}



/***********************************************************/
/** 获取流队列入链状态
* @param   dev_id      设备编号
* @param   tm_type     0-ETM,1-FTM
* @param   flow_id     流队列号
* @param   link_state  0-未入链     1-在调度器链表中
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_flow_link_state_get(DPP_DEV_T *dev, ZXIC_UINT32 flow_id, ZXIC_UINT32 *link_state)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_CRDT_FLOWQUE_INS_TBL_T crdt_flow_ins_tbl_t = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), link_state);

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), flow_id, 0, DPP_ETM_CRDT_NUM);

    rc = dpp_reg_read(dev,
                      ETM_CRDT_FLOWQUE_INS_TBLr,
                      0,
                      flow_id,
                      &crdt_flow_ins_tbl_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *link_state = crdt_flow_ins_tbl_t.flowque_ins;

    return DPP_OK;

}

/***********************************************************/
/** 获取调度器入链状态
* @param   dev_id      设备编号
* @param   tm_type     0-ETM,1-FTM
* @param   se_id       调度器编号
* @param   link_state  0-未入链     1-在调度器链表中
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_se_link_state_get(DPP_DEV_T *dev, ZXIC_UINT32 se_id, ZXIC_UINT32 *link_state)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_CRDT_SE_INS_TBL_T crdt_se_ins_tbl_t = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), link_state);

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_id, 0, DPP_ETM_FQSPWFQ_NUM - 1);

    rc = dpp_reg_read(dev,
                      ETM_CRDT_SE_INS_TBLr,
                      0,
                      se_id,
                      &crdt_se_ins_tbl_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *link_state = crdt_se_ins_tbl_t.se_ins_flag;

    return DPP_OK;

}

/***********************************************************/
/** 判断crdt流删除命令是否空闲
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_del_cmd_idle(DPP_DEV_T *dev)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 read_times = 30;
    DPP_ETM_CRDT_FLOW_DEL_CMD_T crdt_del_cmd_busy = {0};
    ZXIC_COMM_CHECK_POINT(dev);

    do
    {
        rc = dpp_reg_read(dev,
                          ETM_CRDT_FLOW_DEL_CMDr,
                          0,
                          0,
                          &crdt_del_cmd_busy);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
        read_times--;
        zxic_comm_delay(5);
    }
    while ((0 != (crdt_del_cmd_busy.flow_del_busy)) && (read_times > 0));

    if (0 == read_times)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "CRDT Del command busy!\n");
        return DPP_ERR;
    }

    return DPP_OK;

}

/***********************************************************/
/** 删除流/调度器挂接关系(调度器编号非从0开始，需要偏移)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   id        要删除的流号或调度器id
*          ETM范围:0--0xABFF; FTM范围:0-0x177F
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_del_link_set(DPP_DEV_T *dev, ZXIC_UINT32 id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 c_sta = 0;
    ZXIC_UINT32 e_sta = 0;
    ZXIC_UINT32 flow_e = 0;
    ZXIC_UINT32 link_state = 0;      /*流或调度器入链状态*/
    ZXIC_UINT32 read_times = 300;
    ZXIC_UINT32 crdt_del_cmd_reg_index = 0;
    DPP_ETM_CRDT_FLOW_DEL_CMD_T crdt_del_cmd_t = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), id, 0, DPP_ETM_SCH_DEL_NUM);
    flow_e = DPP_ETM_Q_NUM;

    /* 循环判断入链状态是否为1 */
    do
    {

        /* 判断当前流或调度器入链状态：非入链情况才能删除挂接 */
        if (id <= DPP_ETM_CRDT_NUM)
        {
            /* 流入链状态 */
            rc = dpp_tm_crdt_flow_link_state_get(dev, id, &c_sta);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_crdt_flow_link_state_get");

            rc = dpp_tm_crdt_flow_link_state_get(dev, id + DPP_ETM_Q_NUM, &e_sta);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_crdt_flow_link_state_get");

            link_state = c_sta || e_sta;
        }
        else
        {
            /*调度器入链状态*/
            rc = dpp_tm_crdt_se_link_state_get(dev, id - DPP_ETM_SHAP_SEID_BASE, &link_state);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_crdt_se_link_state_get");
           
        }

        if (0 == link_state)
        {
            break;
        }
        read_times--;
        zxic_comm_delay(1);
    }
    while (read_times > 0);

    if (read_times == 0)
    {
        ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(id, flow_e);
        ZXIC_COMM_TRACE_ERROR("id: 0x%08x ins_flag is always 1 (Maybe it's because cir equal zero) !!!\n", id);
        /*此时要 继续往下走，执行强删！如果不删除队列没释放下次直接覆写问题更严重。zhaoyan*/
    }

    /*删挂接命令是否空闲*/
    rc = dpp_tm_crdt_del_cmd_idle(dev);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_crdt_del_cmd_idle");


    /*进行流或调度器删除操作*/
    crdt_del_cmd_reg_index = ETM_CRDT_FLOW_DEL_CMDr;
    crdt_del_cmd_t.flow_alt_cmd = 1;
    crdt_del_cmd_t.flow_alt_ind = id;

    /* 流删除：如果删除c桶，需要同时删除e桶 */
    if (id < DPP_ETM_Q_NUM)
    {
        rc = dpp_reg_write(dev,
                           crdt_del_cmd_reg_index,
                           0,
                           0,
                           &crdt_del_cmd_t);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

        /*删挂e桶：判断命令是否空闲*/
        rc = dpp_tm_crdt_del_cmd_idle(dev);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_crdt_del_cmd_idle");
        crdt_del_cmd_t.flow_alt_ind = (id + DPP_ETM_Q_NUM);
        rc = dpp_reg_write(dev,
                           crdt_del_cmd_reg_index,
                           0,
                           0,
                           &crdt_del_cmd_t);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
    }
    else
    {

        rc = dpp_reg_write(dev,
                           crdt_del_cmd_reg_index,
                           0,
                           0,
                           &crdt_del_cmd_t);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
    }

    return DPP_OK;
}

/***********************************************************/
/** 删除调度器挂接关系(调度器编号从0开始)：对外API
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   se_id_s   要删除的起始调度器id
* @param   se_id_e   要删除的终止调度器id
*          ETM范围:0--0x63FF; FTM范围:0-0x77F
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_del_se_link_set(DPP_DEV_T *dev, ZXIC_UINT32 id_s, ZXIC_UINT32 id_e)
{

    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 se_id_offset = 0;
    ZXIC_UINT32 id = 0 ;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), id_s, 0, DPP_ETM_FQSPWFQ_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), id_e, 0, DPP_ETM_FQSPWFQ_NUM - 1);
    se_id_offset = DPP_ETM_SHAP_SEID_BASE;

    if (id_s > id_e)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "Bad parameters!  id_s > id_e!\n");
        return DPP_ERR;
    }

    for (id = id_s; id <= id_e; id++)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev), id , se_id_offset);    
        rc = dpp_tm_crdt_del_link_set(dev, id + se_id_offset);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_crdt_del_link_set");
    }


    return DPP_OK;

}


/***********************************************************/
/** 删除流挂接关系：对外API
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   id_s      要删除的流号或调度器起始id
* @param   id_e      要删除的流号或调度器终止id
*          ETM范围:0--0x47FF; FTM范围:0-0xFFF
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_del_flow_link_set(DPP_DEV_T *dev, ZXIC_UINT32 id_s, ZXIC_UINT32 id_e)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 id = 0;
    ZXIC_UINT32 q_td_th = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), id_s, 0, DPP_ETM_CRDT_NUM);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), id_e, 0, DPP_ETM_CRDT_NUM);

    if (id_s > id_e)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "Bad parameters!  id_s > id_e !\n");
        return DPP_ERR;
    }

    for (id = id_s; id <= id_e; id++)
    {
        rc = dpp_tm_cgavd_td_th_get(dev, QUEUE_LEVEL, id, &q_td_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_cgavd_td_th_get");

        if (q_td_th != 0)
        {
            ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "queue TD_TH is not equal 0 !  q_td_th != 0 !\n");
            return DPP_ERR;
        }
    }

    for (id = id_s; id <= id_e; id++)
    {
        rc = dpp_tm_crdt_del_link_set(dev, id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_crdt_del_link_set");
    }

    return DPP_OK;
}

#if 0
/***********************************************************/
/** 配置授权分发使能或者关闭
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   en   配置的值，0-关闭授权分发，1-使能授权分发
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/25
************************************************************/
DPP_STATUS dpp_tm_crdt_credit_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CRDT_CREDIT_EN_T credit_en = {0};

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, en, 0, 1);

    credit_en.credit_en = en;
    rc  = dpp_reg_write(dev_id,
                        ETM_CRDT_CREDIT_ENr,
                        0,
                        0,
                        &credit_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 读取授权分发使能或者关闭
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_en   读出的值，0-关闭授权分发，1-使能授权分发
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/25
************************************************************/
DPP_STATUS dpp_tm_crdt_credit_en_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CRDT_CREDIT_EN_T credit_en = {0};
    
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_en);

    *p_en = 0xffffffff;
    rc  = dpp_reg_read(dev_id,
                       ETM_CRDT_CREDIT_ENr,
                       0,
                       0,
                       &credit_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_en = credit_en.credit_en;

    return DPP_OK;
}

/***********************************************************/
/** 配置授权产生间隔
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   crdt_space_choose  授权发送间隔 0:固定16个周期 1：查表
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/10
************************************************************/
DPP_STATUS dpp_tm_crdt_space_choose_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 crdt_space_choose)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_CRDT_CREDIT_SPACE_SELECT_T space_select = {0};

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, crdt_space_choose, 0, 1);

    space_select.credit_space_select = crdt_space_choose;

    rc = dpp_reg_write(dev_id,
                       ETM_CRDT_CREDIT_SPACE_SELECTr,
                       0,
                       0,
                       &space_select);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    return DPP_OK;
}

/***********************************************************/
/** 获得授权产生间隔
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   crdt_space_choose   授权发送间隔 0:固定16个周期 1：查表
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/10
************************************************************/
DPP_STATUS dpp_tm_crdt_space_choose_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_crdt_space_choose)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_CRDT_CREDIT_SPACE_SELECT_T space_select = {0};


    rc = dpp_reg_read(dev_id,
                      ETM_CRDT_CREDIT_SPACE_SELECTr,
                      0,
                      0,
                      &space_select);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_crdt_space_choose = space_select.credit_space_select;

    return DPP_OK;
}

#endif
/***********************************************************/
/** 配置端口拥塞令牌桶使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   port_id   端口号：0~63
* @param   port_en   端口拥塞令牌桶使能，1表示不使用拥塞令牌桶的授权，0表示可以使用拥塞令牌桶授权
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/11
************************************************************/
DPP_STATUS dpp_tm_crdt_port_congest_en_set(DPP_DEV_T *dev,
                                           ZXIC_UINT32 port_id,
                                           ZXIC_UINT32 port_en)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 value = 0;
    DPP_ETM_CRDT_CONGEST_TOKEN_DISABLE_31_0_T disable_31_0 = {0};
    DPP_ETM_CRDT_CONGEST_TOKEN_DISABLE_63_32_T disable_63_32 = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), port_id, 0, DPP_TM_PP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), port_en, 0, 1);

    if (port_id <= 31)
    {
        /* port_id:[0-31] */
        rc = dpp_reg_read(dev,
                          ETM_CRDT_CONGEST_TOKEN_DISABLE_31_0r,
                          0,
                          0,
                          &disable_31_0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

        value = disable_31_0.congest_token_disable_31_0;

        if (port_en == 0)
        {
            value = value & (~(1u << port_id));
        }
        else
        {
            value = value | (1u << port_id);
        }

        disable_31_0.congest_token_disable_31_0 = value;

        rc = dpp_reg_write(dev,
                           ETM_CRDT_CONGEST_TOKEN_DISABLE_31_0r,
                           0,
                           0,
                           &disable_31_0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
    }
    else
    {
        /* port_id:[32-63] */
        rc = dpp_reg_read(dev,
                          ETM_CRDT_CONGEST_TOKEN_DISABLE_63_32r,
                          0,
                          0,
                          &disable_63_32);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

        value = disable_63_32.congest_token_disable_63_32;

        if (port_en == 0)
        {
            value = value & (~(1u << (port_id - 32)));
        }
        else
        {
            value = value | (1u<< (port_id - 32));
        }

        disable_63_32.congest_token_disable_63_32 = value;

        rc = dpp_reg_write(dev,
                           ETM_CRDT_CONGEST_TOKEN_DISABLE_63_32r,
                           0,
                           0,
                           &disable_63_32);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
    }

    return DPP_OK;
}

/***********************************************************/
/** 获得端口拥塞令牌桶使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   port_id   端口号：0~120
* @param   p_port_en   端口拥塞令牌桶使能，1表示不使用拥塞令牌桶的授权，0表示可以使用拥塞令牌桶授权
*
* @return
* @remark  无
* @see
* @author  djf      @date  2015/03/11
************************************************************/
DPP_STATUS dpp_tm_crdt_port_congest_en_get(DPP_DEV_T *dev,
                                           ZXIC_UINT32 port_id,
                                           ZXIC_UINT32 *p_port_en)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 value = 0;
    DPP_ETM_CRDT_CONGEST_TOKEN_DISABLE_31_0_T disable_31_0 = {0};
    DPP_ETM_CRDT_CONGEST_TOKEN_DISABLE_63_32_T disable_63_32 = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), port_id, 0, DPP_TM_PP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_port_en);

    if (port_id <= 31)
    {
        /* port_id:[0-31] */
        rc = dpp_reg_read(dev,
                          ETM_CRDT_CONGEST_TOKEN_DISABLE_31_0r,
                          0,
                          0,
                          &disable_31_0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

        value = disable_31_0.congest_token_disable_31_0;

        *p_port_en = 1 & (value >> port_id);
    }
    else
    {
        /* port_id:[32-63] */
        rc = dpp_reg_read(dev,
                          ETM_CRDT_CONGEST_TOKEN_DISABLE_63_32r,
                          0,
                          0,
                          &disable_63_32);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

        value = disable_63_32.congest_token_disable_63_32;

        *p_port_en = 1 & (value >> (port_id - 32));
    }

    return DPP_OK;
}


/***********************************************************/
/** CRDT 模块 读写是否超时检查:只有端口sp优先级的配置需检测cfg_state的状态
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
*
* @return  DPP_OK-空闲，DPP_ERR-忙
* @remark  无
* @see
* @author  szq      @date  2015/05/26
************************************************************/
DPP_STATUS dpp_tm_crdt_idle_check(DPP_DEV_T *dev)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 read_times = 30;
    DPP_ETM_CRDT_CFG_STATE_T is_idle_flag = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    do
    {
        rc = dpp_reg_read(dev,
                          ETM_CRDT_CFG_STATEr,
                          0,
                          0,
                          &is_idle_flag);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
        read_times--;
        zxic_comm_delay(5);

    }
    while ((1 == is_idle_flag.cfg_state) && (read_times > 0));

    if (0 == read_times)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "crdt rw time out\n");
        return DPP_ERR;
    }

    return DPP_OK;
}

#if 0
/***********************************************************/
/** 授权个数统计寄存器清零
* @param   dev_id  设备编号
* @param   tm_type   0-ETM,1-FTM
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  taq      @date  2015/06/19
************************************************************/
DPP_STATUS dpp_tm_crdt_clr_diag(ZXIC_UINT32 dev_id)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_CRDT_CNT_CLR_T cnt_clr = {0};

    /* 不使能CRDT */
    rc = dpp_tm_crdt_credit_en_set(dev_id, 0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_crdt_credit_en_set");

    /* 清零所有的授权数统计寄存器 */
    cnt_clr.cnt_clr = 1;
    rc = dpp_reg_write(dev_id,
                       ETM_CRDT_CNT_CLRr,
                       0,
                       0,
                       &cnt_clr);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    /* 保持所有的授权数统计寄存器值 */
    cnt_clr.cnt_clr = 0;
    rc = dpp_reg_write(dev_id,
                       ETM_CRDT_CNT_CLRr,
                       0,
                       0,
                       &cnt_clr);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    /* 使能CRDT */
    rc = dpp_tm_crdt_credit_en_set(dev_id, 1);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_crdt_credit_en_set");

    return DPP_OK;
}



/***********************************************************/
/** 打印各级及指定被统计的第0~15个授权流得到的授权个数 stm模式下使用
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/06/19
************************************************************/
DPP_STATUS dpp_tm_crdt_ackcnt_diag(ZXIC_UINT32 dev_id, ZXIC_UINT32 delay_ms)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 credit_value = 0;
    ZXIC_UINT32 flow_spec_id_offset = 0;
    ZXIC_FLOAT traffic_amplified = 0.0;
    ZXIC_FLOAT flow_spec_traffic = 0.0;

    DPP_ETM_CRDT_DEV_CREDIT_CNT_T dev_crdit_cnt = {0};
    //DPP_ETM_CRDT_PP_CREDIT_CNT_T pp_crdit_cnt = {0};

    DPP_ETM_CRDT_STAT_QUE_ID_0_T stat_que_id_0 = {0};
    DPP_ETM_CRDT_STAT_QUE_CREDIT_T que_credit = {0};

    rc = dpp_tm_crdt_clr_diag(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_crdt_clr_diag");
    zxic_comm_sleep(delay_ms);


    /* dev级接收到的授权总数 */
    rc = dpp_reg_read(dev_id,
                      ETM_CRDT_DEV_CREDIT_CNTr,
                      0,
                      0,
                      &dev_crdit_cnt);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    /* pp级接收到的授权总数
    rc = dpp_reg_read(dev_id,
                      ETM_CRDT_PP_CREDIT_CNTr,
                      0,
                      0,
                      &pp_crdit_cnt);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");  */


    /* 读取credit_value */
    rc = dpp_tm_qmu_credit_value_get(dev_id, &credit_value);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_credit_value_get");

    traffic_amplified = ((ZXIC_FLOAT)delay_ms * (ZXIC_FLOAT)(1000000.0)) / ((ZXIC_FLOAT)(8.0) * (ZXIC_FLOAT)(credit_value));
    ZXIC_COMM_PRINT("dev:   ack_cnt = 0x%08x,   traffic = %.6f.(Gb)\n", dev_crdit_cnt.dev_credit_cnt, (ZXIC_FLOAT)(dev_crdit_cnt.dev_credit_cnt) / traffic_amplified);
    //ZXIC_COMM_PRINT("pp:    ack_cnt = 0x%08x,   traffic = %.6f.(G)\n", pp_crdit_cnt.pp_credit_cnt, (ZXIC_FLOAT)(pp_crdit_cnt.pp_credit_cnt) / traffic_amplified);

    for (flow_spec_id_offset = 0; flow_spec_id_offset < 16; flow_spec_id_offset++)
    {
        rc = dpp_reg_read(dev_id,
                          ETM_CRDT_STAT_QUE_ID_0r + flow_spec_id_offset,
                          0,
                          0,
                          &stat_que_id_0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

        rc = dpp_reg_read(dev_id,
                          ETM_CRDT_STAT_QUE_CREDITr,
                          0,
                          flow_spec_id_offset,
                          &que_credit);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

        flow_spec_traffic = (ZXIC_FLOAT)(que_credit.stat_que_credit_cnt) / traffic_amplified;

        ZXIC_COMM_PRINT("flow_0x%04x(%5d):   ", (stat_que_id_0.stat_que_id_0 & 0xffff), (stat_que_id_0.stat_que_id_0 & 0xffff));
        ZXIC_COMM_PRINT("ack_cnt = 0x%08x,   ", que_credit.stat_que_credit_cnt);
        ZXIC_COMM_PRINT("traffic = %.6f.(Gb)\n", flow_spec_traffic);

    }


    return DPP_OK;
}

/***********************************************************/
/**
* @param   dev_id   设备编号
* @param   tm_type  0-ETM,1-FTM
* @param   que_id   queue id
* @param   en       1:过滤E桶队列CRS状态为SLOW的入链请求；0:E桶队列CRS SLOW正常入链；
*
* @return
* @remark  无
* @see
* @author  XXX      @date  2019/05/08
************************************************************/
DPP_STATUS dpp_tm_crdt_eir_crs_filter_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 que_id, ZXIC_UINT32 en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CRDT_EIR_CRS_FILTER_TBL_T eir_crs_filter = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, que_id, 0, DPP_ETM_Q_NUM - 1);

    eir_crs_filter.eir_crs_filter = en;
    rc = dpp_reg_write(dev_id,
                       ETM_CRDT_EIR_CRS_FILTER_TBLr,
                       0,
                       que_id,
                       &eir_crs_filter);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return rc;
}

/***********************************************************/
/**
* @param   dev_id   设备编号
* @param   tm_type  0-ETM,1-FTM
* @param   que_id_s 起始队列号
* @param   que_id_e 终止队列号
* @param   en       1:过滤E桶队列CRS状态为SLOW的入链请求；0:E桶队列CRS SLOW正常入链；
*
* @return
* @remark  无
* @see
* @author  XXX      @date  2019/05/08
************************************************************/
DPP_STATUS dpp_tm_crdt_eir_crs_filter_en_more_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 que_id_s, ZXIC_UINT32 que_id_e, ZXIC_UINT32 en)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 i = 0;

    if (que_id_s > que_id_e)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "Bad parameters!  que_id_s > que_id_e !\n");
        return DPP_ERR;
    }

    for (i = que_id_s; i <= que_id_e; i++)
    {
        rc = dpp_tm_crdt_eir_crs_filter_en_set(dev_id, i, en);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_crdt_eir_crs_filter_en_set");
    }

    return DPP_OK;
}


/***********************************************************/
/**
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   que_id   queue id
* @param   p_en
*
* @return
* @remark  无
* @see
* @author  XXX      @date  2019/05/08
************************************************************/
DPP_STATUS dpp_tm_crdt_eir_crs_filter_en_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 que_id, ZXIC_UINT32 *p_en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CRDT_EIR_CRS_FILTER_TBL_T eir_crs_filter = {0};

    
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_en);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, que_id, 0, DPP_ETM_Q_NUM - 1);

    rc = dpp_reg_read(dev_id,
                      ETM_CRDT_EIR_CRS_FILTER_TBLr,
                      0,
                      que_id,
                      &eir_crs_filter);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    *p_en = eir_crs_filter.eir_crs_filter;

    return rc;
}


/***********************************************************/
/**cpu配置flow_id的crs强制为normal或者off开关使能，用于检测SA模式下队列到授权流的多对一问题
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   flow_id:流号(和授权流号一一对应)
*                en   强制配置crs的使能，0-不使能，1-使能
*                crs_value:强制配置crs的值2'b00:off; 2'b01:low; 2'b10:normal；
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/04/25
************************************************************/
DPP_STATUS dpp_tm_crdt_crs_sheild_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 flow_id, ZXIC_UINT32 en, ZXIC_UINT32 crs_value)
{
    DPP_STATUS  rc = DPP_OK;

    DPP_ETM_CRDT_CRS_SHEILD_FLOW_ID_CFG_T crs_sheild_flow_id_cfg = {0};
    DPP_ETM_CRDT_CRS_SHEILD_EN_CFG_T crs_sheild_en_cfg = {0};
    DPP_ETM_CRDT_CRS_SHEILD_VALUE_CFG_T crs_sheild_value_cfg = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, crs_value, 0, 2);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, flow_id, 0, DPP_ETM_Q_NUM - 1);


    crs_sheild_flow_id_cfg.crs_sheild_flow_id_cfg = flow_id;
    rc  = dpp_reg_write(dev_id,
                        ETM_CRDT_CRS_SHEILD_FLOW_ID_CFGr,
                        0,
                        0,
                        &crs_sheild_flow_id_cfg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    crs_sheild_en_cfg.crs_sheild_en_cfg = en;
    rc  = dpp_reg_write(dev_id,
                        ETM_CRDT_CRS_SHEILD_EN_CFGr,
                        0,
                        0,
                        &crs_sheild_en_cfg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    crs_sheild_value_cfg.crs_sheild_value_cfg = crs_value;
    rc  = dpp_reg_write(dev_id,
                        ETM_CRDT_CRS_SHEILD_VALUE_CFGr,
                        0,
                        0,
                        &crs_sheild_value_cfg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/**获取flow_id的crs强制为normal或者off开关使能，用于检测SA模式下队列到授权流的多对一问题
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   flow_id:流号(和授权流号一一对应)
*                en   强制配置crs的使能，0-不使能，1-使能
*                crs_value:强制配置crs的值2'b00:off; 2'b01:low; 2'b10:normal；
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/04/25
************************************************************/
DPP_STATUS dpp_tm_crdt_crs_sheild_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_flow_id, ZXIC_UINT32 *p_en, ZXIC_UINT32 *p_crs_value)
{
    DPP_STATUS  rc = DPP_OK;

    DPP_ETM_CRDT_CRS_SHEILD_FLOW_ID_CFG_T crs_sheild_flow_id_cfg = {0};
    DPP_ETM_CRDT_CRS_SHEILD_EN_CFG_T crs_sheild_en_cfg = {0};
    DPP_ETM_CRDT_CRS_SHEILD_VALUE_CFG_T crs_sheild_value_cfg = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_flow_id);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_en);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_crs_value);

    rc  = dpp_reg_read(dev_id,
                       ETM_CRDT_CRS_SHEILD_FLOW_ID_CFGr,
                       0,
                       0,
                       &crs_sheild_flow_id_cfg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    *p_flow_id = crs_sheild_flow_id_cfg.crs_sheild_flow_id_cfg;

    rc  = dpp_reg_read(dev_id,
                       ETM_CRDT_CRS_SHEILD_EN_CFGr,
                       0,
                       0,
                       &crs_sheild_en_cfg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    *p_en = crs_sheild_en_cfg.crs_sheild_en_cfg;

    rc  = dpp_reg_read(dev_id,
                       ETM_CRDT_CRS_SHEILD_VALUE_CFGr,
                       0,
                       0,
                       &crs_sheild_value_cfg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    *p_crs_value = crs_sheild_value_cfg.crs_sheild_value_cfg;

    return DPP_OK;
}

/***********************************************************/
/** 控制授权速率的门限
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   index   0~6
* @param   rci_grade_th_0_data
*
* @return
* @remark  无
* @see
* @author  wush      @date  2017/10/17
************************************************************/
DPP_STATUS dpp_tm_crdt_rci_grade_th_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 index, ZXIC_UINT32 rci_grade_th_0_data)
{
    DPP_STATUS rc = DPP_OK;

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, index, 0, 6);

    rc = dpp_reg_write(dev_id,
                       ETM_CRDT_RCI_GRADE_TH_0_CFGr + index,
                       0,
                       0,
                       &rci_grade_th_0_data);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return  DPP_OK;
}


DPP_STATUS dpp_tm_crdt_rci_grade_th_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 index, ZXIC_UINT32 *p_rci_grade_th_0_data)
{
    DPP_STATUS rc = DPP_OK;

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, index, 0, 6);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_rci_grade_th_0_data);

    rc = dpp_reg_read(dev_id,
                      ETM_CRDT_RCI_GRADE_TH_0_CFGr + index,
                      0,
                      0,
                      p_rci_grade_th_0_data);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    return  DPP_OK;
}

/***********************************************************/
/** 控制授权间隔的门限，建议大于等于0XF，不可取0；
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   index   0~7
* @param   asm_interval_0_data   控制授权间隔的门限，建议大于等于0XF，不可取0；
*
* @return
* @remark  无
* @see
* @author  wush      @date  2017/10/17
************************************************************/
DPP_STATUS dpp_tm_crdt_asm_interval_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 index, ZXIC_UINT32 asm_interval_0_data)
{
    DPP_STATUS rc = DPP_OK;

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, index, 0, 7);

    rc = dpp_reg_write(dev_id,
                       ETM_CRDT_ASM_INTERVAL_0_CFGr + index,
                       0,
                       0,
                       &asm_interval_0_data);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");


    return  DPP_OK;
}

DPP_STATUS dpp_tm_crdt_asm_interval_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 index, ZXIC_UINT32 *p_asm_interval_0_data)
{
    DPP_STATUS rc = DPP_OK;

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, index, 0, 7);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_asm_interval_0_data);

    rc = dpp_reg_read(dev_id,
                      ETM_CRDT_ASM_INTERVAL_0_CFGr + index,
                      0,
                      0,
                      p_asm_interval_0_data);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    return  DPP_OK;
}

/***********************************************************/
/** rci的级别
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_rci_grade_data
*
* @return
* @remark  无
* @see
* @author  wush      @date  2017/10/17
************************************************************/
DPP_STATUS dpp_tm_crdt_rci_grade_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_rci_grade_data)
{
    DPP_STATUS rc = DPP_OK;

    
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_rci_grade_data);

    rc = dpp_reg_read(dev_id,
                      ETM_CRDT_RCI_GRADEr,
                      0,
                      0,
                      p_rci_grade_data);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    return  DPP_OK;
}

DPP_STATUS dpp_tm_crdt_rci_value_r_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_crdt_rci_value_r_data)
{
    DPP_STATUS rc = DPP_OK;

    
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_crdt_rci_value_r_data);


    rc = dpp_reg_read(dev_id,
                      ETM_CRDT_CRDT_RCI_VALUE_Rr,
                      0,
                      0,
                      p_crdt_rci_value_r_data);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    return  DPP_OK;
}

DPP_STATUS dpp_tm_crdt_interval_now_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_crdt_interval_now_data)
{
    DPP_STATUS rc = DPP_OK;

    
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_crdt_interval_now_data);

    rc = dpp_reg_read(dev_id,
                      ETM_CRDT_CRDT_INTERVAL_NOWr,
                      0,
                      0,
                      p_crdt_interval_now_data);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    return  DPP_OK;
}

/***********************************************************/
/** 配置crdt interval使能，
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   crdt_interval_en_cfg_data   授权分发间隔使能，1打开，0关闭
*
* @return
* @remark  无
* @see
* @author  wush      @date  2017/03/27
************************************************************/
DPP_STATUS dpp_tm_crdt_interval_en_cfg_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 crdt_interval_en_cfg_data)
{
    DPP_STATUS rc = DPP_OK;

    rc = dpp_reg_write(dev_id,
                       ETM_CRDT_CRDT_INTERVAL_EN_CFGr,
                       0,
                       0,
                       &crdt_interval_en_cfg_data);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return  DPP_OK;
}

/***********************************************************/
/** 读取crdt interval使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   crdt_interval_en_cfg_data   授权分发间隔使能，1打开，0关闭
*
* @return
* @remark  无
* @see
* @author  wush      @date  2017/03/27
************************************************************/
DPP_STATUS dpp_tm_crdt_interval_en_cfg_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_crdt_interval_en_cfg_data)
{
    DPP_STATUS rc = DPP_OK;

    
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_crdt_interval_en_cfg_data);

    rc = dpp_reg_read(dev_id,
                      ETM_CRDT_CRDT_INTERVAL_EN_CFGr,
                      0,
                      0,
                      p_crdt_interval_en_cfg_data);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    return  DPP_OK;
}

/***********************************************************/
/** 屏蔽ucn/asm_rdy的时能信号
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   ucn_rdy_shield_en   是否屏蔽ucn_rdy信号，1屏蔽，0不屏蔽
* @param   asm_rdy_shield_en   是否屏蔽asm_rdy信号，1屏蔽，0不屏蔽
*
* @return
* @remark  无
* @see
* @author  wush      @date  2017/10/17
************************************************************/
DPP_STATUS dpp_tm_crdt_ucn_asm_rdy_shield_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 ucn_rdy_shield_en, ZXIC_UINT32 asm_rdy_shield_en)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_CRDT_UCN_ASM_RDY_SHIELD_EN_T ucn_rdy_shield_en_data = {0};

    

    ucn_rdy_shield_en_data.ucn_rdy_shield_en = ucn_rdy_shield_en;
    ucn_rdy_shield_en_data.asm_rdy_shield_en = asm_rdy_shield_en;

        rc = dpp_reg_write(dev_id,
                           ETM_CRDT_UCN_ASM_RDY_SHIELD_ENr,
                           0,
                           0,
                           &ucn_rdy_shield_en_data);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return  DPP_OK;
}

DPP_STATUS dpp_tm_crdt_ucn_asm_rdy_shield_en_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_ucn_rdy_shield_en, ZXIC_UINT32 *p_asm_rdy_shield_en)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_CRDT_UCN_ASM_RDY_SHIELD_EN_T ucn_rdy_shield_en_data = {0};

    
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_ucn_rdy_shield_en);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_asm_rdy_shield_en);

    rc = dpp_reg_read(dev_id,
                      ETM_CRDT_UCN_ASM_RDY_SHIELD_ENr,
                      0,
                      0,
                      &ucn_rdy_shield_en_data);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_ucn_rdy_shield_en = ucn_rdy_shield_en_data.ucn_rdy_shield_en;
    *p_asm_rdy_shield_en = ucn_rdy_shield_en_data.asm_rdy_shield_en;

    return  DPP_OK;
}

#endif
#endif


#if ZXIC_REAL("TM_SHAPE")

#if 0
/***********************************************************/
/** 把整数分解成(指定位长)最高有效数和(2的)指数位数的形式，data=p_remdata*2^(p_exp)
* @param   data   需要转换前的数
* @param   rembitsum   余数的位数
* @param   p_remdata   余数大小
* @param   p_exp   指数大小
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/27
************************************************************/
DPP_STATUS dpp_tm_rem_and_exp_translate(ZXIC_UINT32 data,
                                        ZXIC_UINT32 rembitsum,
                                        ZXIC_UINT32 *p_remdata,
                                        ZXIC_UINT32 *p_exp)
{
    ZXIC_UINT32 i = 0;
    ZXIC_COMM_CHECK_POINT(p_remdata);
    ZXIC_COMM_CHECK_POINT(p_exp);

    if ((0 == data) || (0 == rembitsum))
    {
        *p_remdata = 0;
        *p_exp = 0;
        return DPP_OK;
    }

    for (i = 1; i <= 32; i++)
    {
        /* ZXIC_UINT64位长64位，如果ZXIC_UINT32在，左移32位时会有问题 */
        if (0 == (data & (((ZXIC_UINT64) 0xffffffff) << i)))
        {
            break;
        }
    }

    if (i <= rembitsum)
    {
        *p_remdata = data;
        *p_exp = 0;
    }
    else
    {
        *p_remdata = (data >> (i - rembitsum));
        *p_exp = i - rembitsum;
    }

    return DPP_OK;

}

/***********************************************************/
/** shap ram初始化
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/22
************************************************************/
DPP_STATUS dpp_tm_shap_ram_init(ZXIC_UINT32 dev_id)
{


    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 read_times = 30;
    DPP_ETM_SHAP_SHAP_CFG_INIT_CFG_T shap_ram_cfg_init_t = {0};
    DPP_ETM_SHAP_SHAP_STA_INIT_CFG_T shap_ram_sta_init_t = {0};


    /**RAM初始化**/
    shap_ram_cfg_init_t.cfg_ram_init_en = 1;
    rc = dpp_reg_write(dev_id,
                       ETM_SHAP_SHAP_CFG_INIT_CFGr,
                       0,
                       0,
                       &shap_ram_cfg_init_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    shap_ram_sta_init_t.sta_ram_init_en = 1;
    rc = dpp_reg_write(dev_id,
                       ETM_SHAP_SHAP_STA_INIT_CFGr,
                       0,
                       0,
                       &shap_ram_sta_init_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");



    /**初始化done确认**/
    do
    {
        rc = dpp_reg_read(dev_id,
                          ETM_SHAP_SHAP_CFG_INIT_CFGr,
                          0,
                          0,
                          &shap_ram_cfg_init_t);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

        rc = dpp_reg_read(dev_id,
                          ETM_SHAP_SHAP_STA_INIT_CFGr,
                          0,
                          0,
                          &shap_ram_sta_init_t);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
        read_times--;
        zxic_comm_usleep(100);

    }
    while ((0 == shap_ram_cfg_init_t.cfg_ram_init_done  || 0 == shap_ram_sta_init_t.sta_ram_init_done ) && (read_times > 0));

    if (0 == read_times)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "SHAP RAM init failed!\n");
        return DPP_ERR;
    }

    return DPP_OK;

}

#endif
/***********************************************************/
/** 配置流队列双桶整形使能及模式
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   db_en     双桶整形使能
* @param   mode      0:c+e模式，1:c+p模式
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_flow_db_en_set(DPP_DEV_T *dev,
                                       ZXIC_UINT32 db_en,
                                       ZXIC_UINT32 mode)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CRDT_DB_TOKEN_T tm_shape_db_en_t = {0};
    DPP_ETM_SHAP_TOKEN_MODE_SWITCH_T tm_shap_db_mode_t = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), db_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), mode, 0, 1);


    tm_shape_db_en_t.db_token = db_en;
    tm_shap_db_mode_t.token_mode_switch = mode;

    rc = dpp_reg_write(dev,
                       ETM_CRDT_DB_TOKENr,
                       0,
                       0,
                       &tm_shape_db_en_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    rc = dpp_reg_write(dev,
                       ETM_SHAP_TOKEN_MODE_SWITCHr,
                       0,
                       0,
                       &tm_shap_db_mode_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 获取流队列双桶整形使能及模式
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   db_en     双桶整形使能
* @param   mode      0:c+e模式，1:c+p模式
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_flow_db_en_get(DPP_DEV_T *dev,
                                       ZXIC_UINT32 *db_en,
                                       ZXIC_UINT32 *mode)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CRDT_DB_TOKEN_T tm_shape_db_en_t = {0};
    DPP_ETM_SHAP_TOKEN_MODE_SWITCH_T tm_shap_db_mode_t = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), db_en);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), mode);

    rc = dpp_reg_read(dev,
                      ETM_CRDT_DB_TOKENr,
                      0,
                      0,
                      &tm_shape_db_en_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    rc = dpp_reg_read(dev,
                      ETM_SHAP_TOKEN_MODE_SWITCHr,
                      0,
                      0,
                      &tm_shap_db_mode_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *db_en = tm_shape_db_en_t.db_token;
    *mode = tm_shap_db_mode_t.token_mode_switch;

    return DPP_OK;
}

#if 0
/***********************************************************/
/** 配置桶深最小单位配置:共8档：0-7
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   token_grain    3’d0：最小单位为128K
*                         3’d1：最小单位为64k
*                         3’d2：最小单位为32k
*                         3’d3：最小单位为16k
*                         3’d4：最小单位为8k
*                         3’d5：最小单位为4k
*                         3’d6：最小单位为2k
*                         3’d7：最小单位为1k
*           默认为0，即128K
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_token_grain_set(ZXIC_UINT32 dev_id,
                                        ZXIC_UINT32 token_grain)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_SHAP_TOKEN_GRAIN_T tm_shape_token_grain_t = {0};

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, token_grain, 0, 7);

    tm_shape_token_grain_t.token_grain = token_grain;

    rc = dpp_reg_write(dev_id,
                       ETM_SHAP_TOKEN_GRAINr,
                       0,
                       0,
                       &tm_shape_token_grain_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;

}

#endif
/***********************************************************/
/** 获取桶深最小单位配置:共8档：0-7
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   token_grain    3’d0：最小单位为128K
*                         3’d1：最小单位为64k
*                         3’d2：最小单位为32k
*                         3’d3：最小单位为16k
*                         3’d4：最小单位为8k
*                         3’d5：最小单位为4k
*                         3’d6：最小单位为2k
*                         3’d7：最小单位为1k
*           默认为0，即128K
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_token_grain_get(DPP_DEV_T *dev,
                                        ZXIC_UINT32 *token_grain)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_SHAP_TOKEN_GRAIN_T tm_shape_token_grain_t = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), token_grain);


    rc = dpp_reg_read(dev,
                      ETM_SHAP_TOKEN_GRAINr,
                      0,
                      0,
                      &tm_shape_token_grain_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *token_grain = tm_shape_token_grain_t.token_grain;

    return DPP_OK;

}

/***********************************************************/
/** 配置流或调度器映射到整形参数表的某个ID
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   id        流或调度器编号ETM:0-ABFF,FTM:0-177F
* @param   profile_id    整形参数表id索引:[0-127]
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_map_table_set(DPP_DEV_T *dev,
                                      ZXIC_UINT32 id,
                                      ZXIC_UINT32 profile_id)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_SHAP_SHAP_BUCKET_MAP_TBL_T tm_shape_map_tbl_t = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), profile_id, 0, 127);

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), id, 0, DPP_ETM_SCH_DEL_NUM);

    tm_shape_map_tbl_t.shap_map = profile_id;

    rc = dpp_reg_write(dev,
                       ETM_SHAP_SHAP_BUCKET_MAP_TBLr,
                       0,
                       id,
                       &tm_shape_map_tbl_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;

}

/***********************************************************/
/** 获取流或调度器映射到整形参数表的配置ID
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   id        流或调度器编号ETM:0-ABFF,FTM:0-177F
* @param   profile_id  整形参数表:[0-127]
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_map_table_get(DPP_DEV_T *dev,
                                      ZXIC_UINT32 id,
                                      ZXIC_UINT32 *profile_id)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_SHAP_SHAP_BUCKET_MAP_TBL_T tm_shape_map_tbl_t = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), profile_id);

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), id, 0, DPP_ETM_SCH_DEL_NUM);

    rc = dpp_reg_read(dev,
                      ETM_SHAP_SHAP_BUCKET_MAP_TBLr,
                      0,
                      id,
                      &tm_shape_map_tbl_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    *profile_id = tm_shape_map_tbl_t.shap_map;

    return DPP_OK;

}

/***********************************************************/
/** 根据流或调度器id查找对应配置表中的模板id
           找到直接进行整形配置并返回1；未找到返回0
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   id        流或调度器编号 ETM:0-ABFF,FTM:0-AFF
* @param   cir       cir速率，单位Kb，范围[64Kb - 160Gb]
* @param   cbs       cbs桶深，单位KB，范围[1KB - 64M]
* @return  找到：1，未找到：0
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_find_map_id(DPP_DEV_T *dev,
                                    ZXIC_UINT32 id,
                                    ZXIC_UINT32 cir,
                                    ZXIC_UINT32 cbs)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 table_id = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 2);
    

    /* 根据id计算归属哪张表：每2K对应一个128项表 */
    table_id = id / 2048;

    for (i = 1; i < 128; i++)
    {
        if (g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][i].shape_cir == cir &&
            g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][i].shape_cbs == cbs)
        {
            rc = dpp_tm_shape_map_table_set(dev, id, i);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_shape_map_table_set");
            g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][i].shape_num++;

            return 1;
        }

    }

    return 0;

}

/***********************************************************/
/** 配置流级整形参数
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   flow_id   流队列号 ETM:0-9215,FTM:0-2047
* @param   cir       cir速率，单位Kb，范围[64Kb - 160Gb]
* @param   cbs       cbs桶深，单位KB，范围[1KB - 64M]
*                     注：cbs=0 表示关闭整形,即不限速
* @param   db_en     双桶整形使能，0-单桶，1-双桶
* @param   eir       eir速率，单位Kb，范围同cir
* @param   ebs       ebs桶深，单位KB，范围同cbs
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_flow_para_set(DPP_DEV_T *dev,
                                      ZXIC_UINT32 flow_id,
                                      ZXIC_UINT32 cir,
                                      ZXIC_UINT32 cbs,
                                      ZXIC_UINT32 db_en,
                                      ZXIC_UINT32 eir,
                                      ZXIC_UINT32 ebs)
{
    DPP_STATUS  rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    

    rc = dpp_etm_shape_flow_para_set(dev, flow_id, cir, cbs, db_en, eir, ebs);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_etm_shape_flow_para_set");

    return DPP_OK;

}

/***********************************************************/
/** 获取流级整形参数
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   flow_id   流队列号 ETM:0-9215,FTM:0-2047
* @param   cir       cir速率，单位Kb，范围[64Kb - 160Gb]
* @param   cbs       cbs桶深，单位KB，范围[1KB - 64M]
*                    注：cbs=0 表示关闭整形,即不限速
* @param   mode_e    整形模式，0-获取c桶参数，1-获取对应e桶参数
* @param   p_para_id   整形模板索引：ETM=[0-AFF]，FTM=[0-17F]
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_flow_para_get(DPP_DEV_T *dev,
                                      ZXIC_UINT32 flow_id,
                                      ZXIC_UINT32 mode,
                                      ZXIC_UINT32 *p_para_id,
                                      DPP_TM_SHAPE_PARA_TABLE *p_flow_para_tbl)
{

    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 flow_id_e = 0;
    ZXIC_UINT32 table_id = 0;
    ZXIC_UINT32 profile_id = 0;
    ZXIC_UINT32 bucket_para_n = 0;
    ZXIC_UINT32 bucket_depth = 0;  /* 实际写入寄存器的桶深，为多少个调节单位 */
    ZXIC_UINT32 bucket_rate = 0;   /* 实际写入寄存器的速率，为每4096周期添加的字节数 */
    ZXIC_UINT32 token_grain = 0;   /* 令牌桶调节档位 */
    ZXIC_UINT32 token_grain_kb[8] = {128, 64, 32, 16, 8, 4, 2, 1}; /* 档位对应值 */
    DPP_ETM_SHAP_BKT_PARA_TBL_T shap_para_tbl_t = {0};


    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), mode, 0, 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_para_id);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_flow_para_tbl);

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), flow_id, 0, DPP_ETM_Q_NUM - 1);

    flow_id_e = flow_id + DPP_ETM_Q_NUM;

    table_id = flow_id / 2048;

    /*获取流的profile_id*/
    if (mode)
    {
        table_id = flow_id_e / 2048;
        rc = dpp_tm_shape_map_table_get(dev, flow_id_e, &profile_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_shape_map_table_get");
    }
    else
    {
        rc = dpp_tm_shape_map_table_get(dev, flow_id, &profile_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_shape_map_table_get");

    }

    /*从寄存器读取流配置参数*/
    bucket_para_n = table_id * 128 + profile_id;

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), bucket_para_n, 0, 0xAFF);

    rc = dpp_reg_read(dev,
                      ETM_SHAP_BKT_PARA_TBLr,
                      0,
                      bucket_para_n,
                      &shap_para_tbl_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    bucket_depth = shap_para_tbl_t.bucket_depth;
    bucket_rate = shap_para_tbl_t.bucket_rate;

    /*数据转换处理*/
    rc = dpp_tm_shape_token_grain_get(dev, &token_grain);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_shape_token_grain_get");
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), token_grain, 0, 7);

    *p_para_id = bucket_para_n;
    p_flow_para_tbl->shape_cbs = bucket_depth * token_grain_kb[token_grain];
    p_flow_para_tbl->shape_cir = (ZXIC_UINT64)bucket_rate * DPP_TM_SYS_HZ * 8 / ((ZXIC_UINT64)4096 * DPP_TM_KILO_ULL * 64);

    return DPP_OK;

}

/***********************************************************/
/** etm配置流级整形参数
* @param   dev_id    设备编号
* @param   flow_id   流队列号 ETM:0-9215,FTM:0-2047
* @param   cir       cir速率，单位Kb，范围[20Kb - 160Gb]
* @param   cbs       cbs桶深，单位KB，范围[1KB - 64M]
*                    注：cbs=0 表示关闭整形,即不限速
* @param   db_en     双桶整形使能，0-单桶，1-双桶
* @param   eir       eir速率，单位Kb，范围同cir
* @param   ebs       ebs桶深，单位KB，范围同cbs
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_etm_shape_flow_para_set(DPP_DEV_T *dev,
                                       ZXIC_UINT32 flow_id,
                                       ZXIC_UINT32 cir,
                                       ZXIC_UINT32 cbs,
                                       ZXIC_UINT32 db_en,
                                       ZXIC_UINT32 eir,
                                       ZXIC_UINT32 ebs)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 table_id = 0;
    ZXIC_UINT32 profile_id = 0;
    ZXIC_UINT32 total_para_id = 0;
    ZXIC_UINT32 get_profile_success_flag_c = 0;  /* 当前已配置表中是否找到需要的整形模板 */
    ZXIC_UINT32 get_profile_success_flag_e = 0;  /* 当前已配置表中是否找到需要的整形模板 */

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 2);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), flow_id, 0, DPP_ETM_Q_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), cir, DPP_TM_SHAPE_CIR_MIN, DPP_TM_SHAPE_CIR_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), cbs, DPP_TM_SHAPE_CBS_MIN, DPP_TM_SHAPE_CBS_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), db_en, 0, 1);

    if (db_en)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), eir, DPP_TM_SHAPE_CIR_MIN, DPP_TM_SHAPE_CIR_MAX);
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), ebs, DPP_TM_SHAPE_CBS_MIN, DPP_TM_SHAPE_CBS_MAX);
    }

    rc = dpp_tm_global_var_mutex_init();
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_global_var_mutex_init");

    rc = zxic_comm_mutex_lock(&g_dpp_tm_global_var_rw_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "zxic_comm_mutex_lock");

    /**双桶开关配置**/
    rc = dpp_tm_shape_flow_db_en_set(dev, db_en, 0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_shape_flow_db_en_set", &g_dpp_tm_global_var_rw_mutex);

    /******STEP1:先解除原profile_id映射******/
    /**处理c桶**/
    rc = dpp_tm_shape_map_table_get(dev, flow_id, &profile_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_shape_map_table_get", &g_dpp_tm_global_var_rw_mutex);

    if (profile_id > 0 && (profile_id < DPP_TM_SHAP_MAP_ID_MAX))
    {
        /***表示当前有配置整形: 根据id计算归属哪张表：每2K队列对应一个128项表***/
        table_id = flow_id / 2048;

        if (g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][profile_id].shape_num != 0)
        {
            g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][profile_id].shape_num--;

            if (0 == g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][profile_id].shape_num)
            {
                g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][profile_id].shape_cbs = 0;
                g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][profile_id].shape_cir = 0;
            }
        }
    }

    /**处理e桶**/
    rc = dpp_tm_shape_map_table_get(dev, (flow_id + DPP_ETM_Q_NUM), &profile_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_shape_map_table_get", &g_dpp_tm_global_var_rw_mutex);

    if (profile_id > 0 && (profile_id < DPP_TM_SHAP_MAP_ID_MAX))
    {
        /*表示当前有配置整形*/
        table_id = (flow_id + DPP_ETM_Q_NUM) / 2048;

        if (g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][profile_id].shape_num != 0)
        {
            g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][profile_id].shape_num--;

            if (0 == g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][profile_id].shape_num)
            {
                g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][profile_id].shape_cbs = 0;
                g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][profile_id].shape_cir = 0;
            }
        }
    }


    /******STEP2:整形关闭的处理******/
    if (cbs == 0)
    {
        /*关闭c桶e桶整形,并返回函数*/
        rc = dpp_tm_shape_map_table_set(dev, (flow_id), 0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_shape_map_table_set", &g_dpp_tm_global_var_rw_mutex);
        rc = dpp_tm_shape_map_table_set(dev, (flow_id + DPP_ETM_Q_NUM), 0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_shape_map_table_set", &g_dpp_tm_global_var_rw_mutex);

        rc = zxic_comm_mutex_unlock(&g_dpp_tm_global_var_rw_mutex);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
        return DPP_OK;
    }

    if (ebs == 0 || db_en == 0)
    {
        /*关闭e桶整形*/
        rc = dpp_tm_shape_map_table_set(dev, (flow_id + DPP_ETM_Q_NUM), 0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_shape_map_table_set", &g_dpp_tm_global_var_rw_mutex);
        get_profile_success_flag_e = 1;

    }


    /******STEP3:整形配置处理,先在现有模板中查找******/
    /* 单桶整形 :c桶 */
    if (ebs == 0)
    {
        /* 此时只有cbs>0，仅开启c桶整形：先查找c桶整形profile配置 */
        rc = dpp_tm_shape_find_map_id(dev, flow_id, cir, cbs);

        if (rc)
        {
            get_profile_success_flag_c = 1;
        }
        else
        {
            get_profile_success_flag_c = 0;
        }
    }
    /* 双桶整形 :c+e桶 */
    else
    {
        /* 此时cbs>0,ebs>0：先查找c桶整形profile配置 */
        rc = dpp_tm_shape_find_map_id(dev, flow_id, cir, cbs);

        if (rc)
        {
            get_profile_success_flag_c = 1;
        }
        else
        {
            get_profile_success_flag_c = 0;
        }

        /* 查找e桶整形profile配置 */
        rc = dpp_tm_shape_find_map_id(dev, flow_id + DPP_ETM_Q_NUM, eir, ebs);

        if (rc )
        {
            get_profile_success_flag_e = 1;
        }
        else
        {
            get_profile_success_flag_e = 0;
        }

    }

    /******STEP4:现有整形模板中未找到所需profile******/
    if (!get_profile_success_flag_c)
    {
        /* 根据id计算归属哪张表：每2K对应一个128项表 */
        table_id = flow_id / 2048;

        for (i = 1; i < 128;  i++)
        {
            if (g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][i].shape_num == 0)
            {

                /**********映射模板id********/
                rc = dpp_tm_shape_map_table_set(dev, flow_id, i);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_shape_map_table_set", &g_dpp_tm_global_var_rw_mutex);

                /*********整形参数配置********/
                total_para_id = table_id * 128 + i;
                rc = dpp_tm_shape_para_set(dev, total_para_id, cir, cbs);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_shape_para_set", &g_dpp_tm_global_var_rw_mutex);

                /********同步更新全局数组******/
                g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][i].shape_cir = cir;
                g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][i].shape_cbs = cbs;
                g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][i].shape_num++;

                get_profile_success_flag_c = 1;
                break;
            }

        }
    }

    if (!get_profile_success_flag_e && ebs)
    {
        /* 根据id计算归属哪张表：每2K对应一个128项表 */
        table_id = (flow_id + DPP_ETM_Q_NUM) / 2048;

        for (i = 1; i < 128; i++)
        {
            if (g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][i].shape_num == 0)
            {

                /**********映射模板id********/
                rc = dpp_tm_shape_map_table_set(dev, (flow_id + DPP_ETM_Q_NUM), i);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_shape_map_table_set", &g_dpp_tm_global_var_rw_mutex);

                /*********整形参数配置********/
                total_para_id = table_id * 128 + i;
                rc = dpp_tm_shape_para_set(dev, total_para_id, eir, ebs);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_shape_para_set", &g_dpp_tm_global_var_rw_mutex);

                /********同步更新全局数组******/
                g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][i].shape_cir = eir;
                g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][i].shape_cbs = ebs;
                g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][i].shape_num++;

                get_profile_success_flag_e = 1;
                break;
            }

        }
    }


    if (!get_profile_success_flag_c || !get_profile_success_flag_e)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "Failure!  Profile resource are FULL!\n");
        rc = zxic_comm_mutex_unlock(&g_dpp_tm_global_var_rw_mutex);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");

        return DPP_ERR;
    }

    rc = zxic_comm_mutex_unlock(&g_dpp_tm_global_var_rw_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");

    return DPP_OK;
}


/***********************************************************/
/** tm配置调度器整形参数
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   se_id     调度器编号号,用户看到：ETM 0-0x63FF, FTM 0-0x77F
*                    实际：ETM:0x4800-0xABFF,FTM:0x1000-0x177F
* @param   pir       pir总速率，单位Kb，范围同cir
* @param   pbs       pbs总桶深，单位KB，范围同cbs
* @param   db_en      整形模式，0-单桶，1-双桶，仅FQ8/WFQ8有效
* @param   cir       [0-3]调度器cir速率，单位Kb，范围[64Kb - 160Gb]
* @param   cbs       [0-3]调度器cbs桶深，单位KB，范围[1KB - 64M]
*                    注：cbs=0 表示关闭整形,即不限速
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_se_para_set(DPP_DEV_T *dev,
                                    ZXIC_UINT32 se_id,
                                    ZXIC_UINT32 pir,
                                    ZXIC_UINT32 pbs,
                                    ZXIC_UINT32 db_en,
                                    ZXIC_UINT32 cir,
                                    ZXIC_UINT32 cbs)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);

    rc = dpp_etm_shape_se_para_set(dev, se_id, pir, pbs, db_en, cir, cbs);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_etm_shape_se_para_set");
    

    return DPP_OK;

}

/***********************************************************/
/** 获取调度单元整形参数
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   se_id     调度器单元号 ETM:0-63FF,FTM:0-77F
* @param   cir       cir速率，单位Kb，范围[64Kb - 160Gb]
* @param   cbs       cbs桶深，单位KB，范围[1KB - 64M]
*                     注：cbs=0 表示关闭整形,即不限速
* @param   mode    整形模式，0-获取p桶参数，1-获取对应c桶参数(仅FQ8/WFQ8支持)
* @param   p_para_id   整形模板索引：ETM=[0-AFF]，FTM=[0-17F]
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_se_para_get(DPP_DEV_T *dev,
                                    ZXIC_UINT32 se_id,
                                    ZXIC_UINT32 mode,
                                    ZXIC_UINT32 *p_para_id,
                                    DPP_TM_SHAPE_PARA_TABLE *p_se_para_tbl)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 real_se_id = 0;
    ZXIC_UINT32 se_id_c = 0;
    ZXIC_UINT32 table_id = 0;
    ZXIC_UINT32 profile_id = 0;
    ZXIC_UINT32 bucket_para_n = 0;
    ZXIC_UINT32 bucket_depth = 0;  /* 实际写入寄存器的桶深，为多少个调节单位 */
    ZXIC_UINT32 bucket_rate = 0;   /* 实际写入寄存器的速率，为每4096周期添加的字节数 */
    ZXIC_UINT32 token_grain = 0;   /* 令牌桶调节档位 */
    ZXIC_UINT32 token_grain_kb[8] = {128, 64, 32, 16, 8, 4, 2, 1}; /* 档位对应值 */
    DPP_ETM_SHAP_BKT_PARA_TBL_T shap_para_tbl_t = {0};


    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), mode, 0, 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_para_id);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_se_para_tbl);

    real_se_id = (se_id + DPP_ETM_SHAP_SEID_BASE);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), real_se_id, 0, DPP_ETM_SCH_DEL_NUM);


    se_id_c = real_se_id + 4;

    table_id = real_se_id / 2048;

    /*获取调度器的profile_id*/
    if (mode)
    {
        rc = dpp_tm_shape_map_table_get(dev, se_id_c, &profile_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_shape_map_table_get");
    }
    else
    {
        rc = dpp_tm_shape_map_table_get(dev, real_se_id, &profile_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_shape_map_table_get");

    }


    /*从寄存器读取调度器配置参数*/
    bucket_para_n = table_id * 128 + profile_id;

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), bucket_para_n, 0, 0xAFF);

    rc = dpp_reg_read(dev,
                      ETM_SHAP_BKT_PARA_TBLr,
                      0,
                      bucket_para_n,
                      &shap_para_tbl_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    bucket_depth = shap_para_tbl_t.bucket_depth;
    bucket_rate = shap_para_tbl_t.bucket_rate;

    /*数据转换处理*/
    rc = dpp_tm_shape_token_grain_get(dev, &token_grain);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_shape_token_grain_get");
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), token_grain, 0, 7);

    *p_para_id = bucket_para_n;
    p_se_para_tbl->shape_cbs = bucket_depth * token_grain_kb[token_grain];
    p_se_para_tbl->shape_cir = (ZXIC_UINT64)bucket_rate * DPP_TM_SYS_HZ * 8 / ((ZXIC_UINT64)4096 * DPP_TM_KILO_ULL * 64);

    return DPP_OK;

}




/***********************************************************/
/** etm配置调度器整形参数
* @param   dev_id    设备编号
* @param   se_id     调度器编号号 ETM 0-0x63FF, FTM 0-0x77F
*                    实际：ETM:0x4800-0xABFF,FTM:0x1000-0x177F
* @param   pir       pir总速率，单位Kb，范围同cir
* @param   pbs       pbs总桶深，单位Kb，范围同cbs
* @param   db_en     整形模式，0-单桶，1-双桶，仅FQ8/WFQ8有效
* @param   cir       [0-3]调度器cir速率，单位Kb，范围[64Kb - 160Gb]
* @param   cbs       [0-3]调度器cbs桶深，单位KB，范围[1KB - 64M]
*                    注：cbs=0 表示关闭整形,即不限速
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_etm_shape_se_para_set(DPP_DEV_T *dev,
                                     ZXIC_UINT32 se_id,
                                     ZXIC_UINT32 pir,
                                     ZXIC_UINT32 pbs,
                                     ZXIC_UINT32 db_en,
                                     ZXIC_UINT32 cir,
                                     ZXIC_UINT32 cbs)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 i = 0;    
    ZXIC_UINT32 real_se_id = 0;
    ZXIC_UINT32 sch_type = 0;
    ZXIC_UINT32 sch_type_num = 0;
    ZXIC_UINT32 table_id = 0;
    ZXIC_UINT32 profile_id = 0;
    ZXIC_UINT32 total_para_id = 0;
    ZXIC_UINT32 get_profile_success_flag_p = 0;  /* 当前已配置表中是否找到需要的p桶整形模板 */
    ZXIC_UINT32 get_profile_success_flag_c = 0;  /* 当前已配置表中是否找到需要的c桶整形模板 */
    DPP_ETM_CRDT_SE_PARA_TBL_T crdt_se_para_tabl_t = {0};  /*配置cp双桶模式,仅FQ8/WFQ8使用*/
    
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 2);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), se_id, 0, DPP_ETM_FQSPWFQ_NUM-1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), pir, DPP_TM_SHAPE_CIR_MIN, DPP_TM_SHAPE_CIR_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), pbs, DPP_TM_SHAPE_CBS_MIN, DPP_TM_SHAPE_CBS_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), db_en, 0, 1);
    
    real_se_id = (se_id + DPP_ETM_SHAP_SEID_BASE);

    if (db_en)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), cir, DPP_TM_SHAPE_CIR_MIN, DPP_TM_SHAPE_CIR_MAX);
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), cbs, DPP_TM_SHAPE_CBS_MIN, DPP_TM_SHAPE_CBS_MAX);
    }

    rc = dpp_tm_global_var_mutex_init();
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_global_var_mutex_init");

    rc = zxic_comm_mutex_lock(&g_dpp_tm_global_var_rw_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "zxic_comm_mutex_lock");


    rc = dpp_tm_crdt_sch_type_get(dev, se_id, &sch_type, &sch_type_num);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_crdt_sch_type_get", &g_dpp_tm_global_var_rw_mutex);

    /**双桶模式配置:crdt模块se_id不用转换**/
    if (sch_type == 8)
    {

        for (i = 0; i < 8; i++)
        {
            rc = dpp_reg_read(dev,
                              ETM_CRDT_SE_PARA_TBLr,
                              0,
                              se_id + i,
                              &crdt_se_para_tabl_t);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_reg_read", &g_dpp_tm_global_var_rw_mutex);

            crdt_se_para_tabl_t.cp_token_en = db_en;

            rc = dpp_reg_write(dev,
                               ETM_CRDT_SE_PARA_TBLr,
                               0,
                               se_id + i,
                               &crdt_se_para_tabl_t);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_reg_write", &g_dpp_tm_global_var_rw_mutex);
        }

    }

    /******STEP1:先解除原profile_id映射******/
    /**处理p桶**/
    rc = dpp_tm_shape_map_table_get(dev, real_se_id, &profile_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_shape_map_table_get", &g_dpp_tm_global_var_rw_mutex);

    if (profile_id > 0 && (profile_id < DPP_TM_SHAP_MAP_ID_MAX))
    {
        /***表示当前有配置整形: 根据id计算归属哪张表：每2K队列对应一个128项表***/
        table_id = real_se_id / 2048;
        
        if (g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][profile_id].shape_num != 0)
        {
            g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][profile_id].shape_num--;
        }
    }

    if (sch_type == 8)
    {
        /**处理c桶**/
        rc = dpp_tm_shape_map_table_get(dev, (real_se_id + 4), &profile_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_shape_map_table_get", &g_dpp_tm_global_var_rw_mutex);

        if (profile_id > 0 && (profile_id < DPP_TM_SHAP_MAP_ID_MAX))
        {
            /*表示当前有配置整形*/
            table_id = (real_se_id + 4) / 2048;

            if (g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][profile_id].shape_num != 0)
            {
                g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][profile_id].shape_num--;
            }
        }
    }



    /******STEP2:整形关闭的处理******/
    if (sch_type < 8)
    {
        /**非FQ8/WFQ8类型调度器：无双桶模式**/
        if (pbs == 0 )
        {
            /*关闭p桶整形,并返回函数*/
            rc = dpp_tm_shape_map_table_set(dev, (real_se_id), 0);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_shape_map_table_set", &g_dpp_tm_global_var_rw_mutex);

            rc = zxic_comm_mutex_unlock(&g_dpp_tm_global_var_rw_mutex);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");

            return DPP_OK;
        }
    }
    else
    {
        /**FQ8/WFQ8类型调度器：考虑双桶**/
        if (pbs == 0 && db_en == 0)
        {
            /*单桶模式：关闭p桶整形,并返回函数*/
            rc = dpp_tm_shape_map_table_set(dev, (real_se_id), 0);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_shape_map_table_set", &g_dpp_tm_global_var_rw_mutex);

            rc = zxic_comm_mutex_unlock(&g_dpp_tm_global_var_rw_mutex);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");

            return DPP_OK;
        }

        if (pbs == 0 && db_en == 1 && cbs == 0)
        {
            /*双桶模式：关闭p+c桶整形，并返回函数*/
            rc = dpp_tm_shape_map_table_set(dev, (real_se_id), 0);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_shape_map_table_set", &g_dpp_tm_global_var_rw_mutex);
            rc = dpp_tm_shape_map_table_set(dev, (real_se_id + 4), 0);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_shape_map_table_set", &g_dpp_tm_global_var_rw_mutex);

            rc = zxic_comm_mutex_unlock(&g_dpp_tm_global_var_rw_mutex);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");

            return DPP_OK;
        }

    }


    /******STEP3:整形配置处理,先在现有模板中查找******/
    /* 非FQ8/WFQ8类型：仅处理p桶，c桶flag直接至1 */
    if (sch_type < 8)
    {
        /* 查找p桶整形profile配置 */
        rc = dpp_tm_shape_find_map_id(dev, real_se_id, pir, pbs);

        if (rc)
        {
            get_profile_success_flag_p = 1;
        }
        else
        {
            get_profile_success_flag_p = 0;
        }

        get_profile_success_flag_c = 1;
    }
    else
    {
        /* FQ8/WFQ8类型：p+c桶 */
        if (db_en == 0)
        {
            /* 单桶模式：仅查找p桶整形profile配置 */
            rc = dpp_tm_shape_find_map_id(dev, real_se_id, pir, pbs);

            if (rc)
            {
                get_profile_success_flag_p = 1;
            }
            else
            {
                get_profile_success_flag_p = 0;
            }

            get_profile_success_flag_c = 1;
        }
        else
        {

            /* 双桶模式：先查找p桶整形profile配置 */
            if (pbs == 0)
            {
                rc = dpp_tm_shape_map_table_set(dev, real_se_id, 0);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_shape_map_table_set", &g_dpp_tm_global_var_rw_mutex);

                get_profile_success_flag_p = 1;
            }
            else
            {
                rc = dpp_tm_shape_find_map_id(dev, real_se_id, pir, pbs);

                if (rc)
                {
                    get_profile_success_flag_p = 1;
                }
                else
                {
                    get_profile_success_flag_p = 0;
                }
            }


            /* 查找c桶整形profile配置 */
            if (cbs == 0)
            {
                rc = dpp_tm_shape_map_table_set(dev, (real_se_id + 4), 0);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_shape_map_table_set", &g_dpp_tm_global_var_rw_mutex);

                get_profile_success_flag_c = 1;
            }
            else
            {
                rc = dpp_tm_shape_find_map_id(dev, (real_se_id + 4), cir, cbs);

                if (rc )
                {
                    get_profile_success_flag_c = 1;
                }
                else
                {
                    get_profile_success_flag_c = 0;
                }

            }


        }


    }


    /******STEP4:现有整形模板中未找到所需profile******/
    if (!get_profile_success_flag_p)
    {
        /* 根据id计算归属哪张表：每2K对应一个128项表 */
        table_id = real_se_id / 2048;

        for (i = 1; i < 128;  i++)
        {
            if (g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][i].shape_num == 0)
            {

                /**********映射模板id********/
                rc = dpp_tm_shape_map_table_set(dev, real_se_id, i);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_shape_map_table_set", &g_dpp_tm_global_var_rw_mutex);

                /*********整形参数配置********/
                total_para_id = table_id * 128 + i;
                rc = dpp_tm_shape_para_set(dev, total_para_id, pir, pbs);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_shape_para_set", &g_dpp_tm_global_var_rw_mutex);

                /********同步更新全局数组******/
                g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][i].shape_cir = pir;
                g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][i].shape_cbs = pbs;
                g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][i].shape_num++;

                get_profile_success_flag_p = 1;
                break;
            }

        }
    }

    if (!get_profile_success_flag_c)
    {
        /* 根据id计算归属哪张表：每2K对应一个128项表 */
        table_id = (real_se_id + 4) / 2048;

        for (i = 1; i < 128; i++)
        {
            if (g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][i].shape_num == 0)
            {

                /**********映射模板id********/
                rc = dpp_tm_shape_map_table_set(dev, (real_se_id + 4), i);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_shape_map_table_set", &g_dpp_tm_global_var_rw_mutex);

                /*********整形参数配置********/
                total_para_id = table_id * 128 + i;
                rc = dpp_tm_shape_para_set(dev, total_para_id, cir, cbs);
                ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_tm_shape_para_set", &g_dpp_tm_global_var_rw_mutex);

                /********同步更新全局数组******/
                g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][i].shape_cir = cir;
                g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][i].shape_cbs = cbs;
                g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][i].shape_num++;

                get_profile_success_flag_c = 1;
                break;
            }

        }
    }


    if (!get_profile_success_flag_p || !get_profile_success_flag_c)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "Failure!  Profile resource are FULL!\n");

        rc = zxic_comm_mutex_unlock(&g_dpp_tm_global_var_rw_mutex);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
        return DPP_ERR;
    }

    rc = zxic_comm_mutex_unlock(&g_dpp_tm_global_var_rw_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");

    return DPP_OK;
}


/***********************************************************/
/** 写入流/调度器整形参数配置表
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   total_para_id    整形参数表中模板索引id ETM:0-AFF,FTM:0-17F
* @param   cir       整形速率(c/e桶统一)
* @param   cbs       桶深
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_para_set(DPP_DEV_T *dev,
                                 ZXIC_UINT32 total_para_id,
                                 ZXIC_UINT32 cir,
                                 ZXIC_UINT32 cbs)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 bucket_depth = 0;  /* 实际写入寄存器的桶深，为多少个调节单位 */
    ZXIC_UINT32 bucket_rate = 0;   /* 实际写入寄存器的速率，为每4096周期添加的字节数 */
    ZXIC_UINT32 token_grain = 0;   /* 令牌桶调节档位 */
    ZXIC_UINT32 token_grain_kb[8] = {128, 64, 32, 16, 8, 4, 2, 1}; /* 档位对应值 */
    DPP_ETM_SHAP_BKT_PARA_TBL_T shap_para_tbl_t = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), cir, DPP_TM_SHAPE_CIR_MIN, DPP_TM_SHAPE_CIR_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), cbs, DPP_TM_SHAPE_CBS_MIN, DPP_TM_SHAPE_CBS_MAX);

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), total_para_id, 0, 0xAFF);


    /********* 数据转换处理:Begin ********/

    rc = dpp_tm_shape_token_grain_get(dev, &token_grain);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_shape_token_grain_get");
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), token_grain, 0, 7);

    if (cbs < token_grain_kb[token_grain] && (cbs != 0))
    {
        bucket_depth = 1; /* 最小为1个桶深调节单位 */
    }
    else
    {
        bucket_depth = cbs / token_grain_kb[token_grain];
    }

    /* 寄存器最大可写范围为[0-2047] */
    if (bucket_depth > DPP_TM_SHAPE_CBS_REG_MAX)
    {
        bucket_depth = DPP_TM_SHAPE_CBS_REG_MAX;
    }

    /* 平均每周期添加cir*1/64bit，即每4096周期添加的字节数 */
    //bucket_rate = (ZXIC_UINT64)4096 * cir * DPP_TM_KILO_ULL * 64 / (((ZXIC_UINT64)DPP_TM_SYS_HZ / (ZXIC_UINT64)13393) * 8);
    bucket_rate = (ZXIC_UINT64)4096 * cir * DPP_TM_KILO_ULL * 64 / ((ZXIC_UINT64)DPP_TM_SYS_HZ * 8);
    shap_para_tbl_t.bucket_rate = bucket_rate;
    shap_para_tbl_t.bucket_depth = bucket_depth;

    /******** 数据转换处理:End *********/

    rc = dpp_reg_write(dev,
                       ETM_SHAP_BKT_PARA_TBLr,
                       0,
                       total_para_id,
                       &shap_para_tbl_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;

}


/***********************************************************/
/** 读取流/调度器整形参数配置表
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   total_para_id    整形参数表中模板索引id ETM:0-AFF,FTM:0-17F
* @param   cir       整形速率(c/e桶统一)
* @param   cbs       桶深
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_para_get(DPP_DEV_T *dev,
                                 ZXIC_UINT32 total_para_id,
                                 DPP_TM_SHAPE_PARA_TABLE *p_shap_para_tbl)
{

    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 token_grain = 0;   /* 令牌桶调节档位 */
    ZXIC_UINT32 token_grain_kb[8] = {128, 64, 32, 16, 8, 4, 2, 1}; /* 档位对应值 */
    DPP_ETM_SHAP_BKT_PARA_TBL_T shap_para_tbl_t = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_shap_para_tbl);

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), total_para_id, 0, 0xAFF);


    /*读取配置*/
    rc = dpp_reg_read(dev,
                      ETM_SHAP_BKT_PARA_TBLr,
                      0,
                      total_para_id,
                      &shap_para_tbl_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    /********* 数据转换处理: ********/
    rc = dpp_tm_shape_token_grain_get(dev, &token_grain);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_shape_token_grain_get");
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), token_grain, 0, 7);

    p_shap_para_tbl->shape_cbs = shap_para_tbl_t.bucket_depth * token_grain_kb[token_grain];
    p_shap_para_tbl->shape_cir = (ZXIC_UINT64)shap_para_tbl_t.bucket_rate * DPP_TM_SYS_HZ * 8 / ((ZXIC_UINT64)4096 * DPP_TM_KILO_ULL * 64);

    return DPP_OK;

}


/***********************************************************/
/** 配置端口级整形参数  更改整形转换公式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   port_id   端口号
* @param   p_para   整形信息:CIR/CBS/EN
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/04/15
************************************************************/
DPP_STATUS dpp_tm_shape_pp_para_set(DPP_DEV_T *dev,
                                    ZXIC_UINT32 port_id,
                                    const DPP_TM_SHAPE_PP_PARA_T *p_para)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 cir = 0;/* 颗粒度的倍数 */
    ZXIC_UINT32 cbs = 0;/* Credit的倍数 */
    ZXIC_UINT32 qmu_credit_value = 0;
    //ZXIC_FLOAT DPP_TM_SHAPE_CIR_STEP_TEST = 160.069565217 * 1000 * 1000 * 1000 / 0x3FFFFFE;/*测试使用 by xuhb*/

    DPP_ETM_CRDT_PP_WEIGHT_RAM_T pp_weight = {0};
    DPP_ETM_CRDT_PP_CBS_SHAPE_EN_RAM_T pp_cbs_shape_en = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_para);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), port_id, 0, DPP_TM_PP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), p_para->c_en, 0, 1);

    if (p_para->c_en == 0)
    {
        rc  = dpp_reg_read(dev,
                           ETM_CRDT_PP_CBS_SHAPE_EN_RAMr,
                           0,
                           port_id,
                           &pp_cbs_shape_en);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");
        pp_cbs_shape_en.pp_c_shap_en = p_para->c_en;
        rc  = dpp_reg_write(dev,
                            ETM_CRDT_PP_CBS_SHAPE_EN_RAMr,
                            0,
                            port_id,
                            &pp_cbs_shape_en);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
    }
    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), p_para->cir, DPP_TM_SHAPE_CIR_MIN, DPP_TM_SHAPE_CIR_MAX);

        /* 读取授权价值 */
        rc = dpp_tm_qmu_credit_value_get(dev, &qmu_credit_value);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_qmu_credit_value_get");

        cir = p_para->cir;
        //cir = (ZXIC_UINT32)(((ZXIC_UINT64)cir * DPP_TM_KILO_ULL) / DPP_TM_SHAPE_CIR_STEP)*(crdt_credit_value / qmu_credit_value);
        cir = (ZXIC_UINT32)(((ZXIC_UINT64)cir * DPP_TM_KILO_ULL) / DPP_TM_SHAPE_CIR_STEP);



        /* 解决160Gbps设置出错的问题add by cuiy at 2016-4-15   */
        if (cir > 0x3FFFFFE)
        {
            cir = 0x3FFFFFE;
        }

        /* 检查以kbyte为单位的CBS */
        if (qmu_credit_value != 0)
        {
            cbs = p_para->cbs;
            cbs = cbs * DPP_TM_KILO_UL / qmu_credit_value;
        }

        /* 寄存器写入CBS的最小值为20，小于该值时，整形不准 */
        if (cbs < DPP_TM_SHAPE_DEFAULT_CBS)
        {
            cbs = DPP_TM_SHAPE_DEFAULT_CBS;
        }

        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), cbs, DPP_TM_SHAPE_DEFAULT_CBS, 0x1ffff);

        pp_cbs_shape_en.pp_cbs = cbs;
        pp_weight.pp_c_weight = cir;
        pp_cbs_shape_en.pp_c_shap_en = p_para->c_en;

        rc  = dpp_reg_write(dev,
                            ETM_CRDT_PP_WEIGHT_RAMr,
                            0,
                            port_id,
                            &pp_weight);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");

        rc  = dpp_reg_write(dev,
                            ETM_CRDT_PP_CBS_SHAPE_EN_RAMr,
                            0,
                            port_id,
                            &pp_cbs_shape_en);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_write");
    }

    return DPP_OK;
}


/***********************************************************/
/** 读取端口级整形参数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   port_id   端口号
* @param   p_para   整形信息:CIR/CBS/EN
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/04/15
************************************************************/
DPP_STATUS dpp_tm_shape_pp_para_get(DPP_DEV_T *dev,
                                    ZXIC_UINT32 port_id,
                                    DPP_TM_SHAPE_PP_PARA_T *p_para)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 cbs = 0;/* Credit的倍数 */
    ZXIC_UINT32 cir = 0;/* 颗粒度的倍数 */
    ZXIC_UINT32 qmu_credit_value = 0;

    DPP_ETM_CRDT_PP_WEIGHT_RAM_T pp_weight = {0};
    DPP_ETM_CRDT_PP_CBS_SHAPE_EN_RAM_T pp_cbs_shape_en = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_para);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), port_id, 0, DPP_TM_PP_NUM - 1);

    /* 读取授权价值 */
    rc = dpp_tm_qmu_credit_value_get(dev, &qmu_credit_value);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_qmu_credit_value_get");

    rc  = dpp_reg_read(dev,
                       ETM_CRDT_PP_WEIGHT_RAMr,
                       0,
                       port_id,
                       &pp_weight);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev,
                       ETM_CRDT_PP_CBS_SHAPE_EN_RAMr,
                       0,
                       port_id,
                       &pp_cbs_shape_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_reg_read");

    cbs = pp_cbs_shape_en.pp_cbs;
    cir = pp_weight.pp_c_weight;
    p_para->c_en = pp_cbs_shape_en.pp_c_shap_en;
    p_para->cir = (ZXIC_UINT32)((ZXIC_UINT64)cir * DPP_TM_SHAPE_CIR_STEP / DPP_TM_KILO_ULL) ;
    p_para->cbs = (cbs * qmu_credit_value / DPP_TM_KILO_UL);

    return DPP_OK;

}

/***********************************************************/
/** 写入端口级整形信息
* @param   tm_type   0-ETM,1-FTM
* @param   port_id   端口号0-63
* @param   cir   单位Kb
* @param   cbs   单位KB
* @param   c_en   c桶使能
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/03
************************************************************/
DPP_STATUS dpp_tm_shape_pp_para_wr(DPP_DEV_T *dev,
                                   ZXIC_UINT32 port_id,
                                   ZXIC_UINT32 cir,
                                   ZXIC_UINT32 cbs,
                                   ZXIC_UINT32 c_en)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_TM_SHAPE_PP_PARA_T para = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), port_id, 0, DPP_TM_PP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), c_en, 0, 1);

    para.cir = cir;
    para.cbs = cbs;
    para.c_en = c_en;

    rc = dpp_tm_shape_pp_para_set(dev, port_id, &para);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_shape_pp_para_set");

    return DPP_OK;
}

#if 0
/***********************************************************/
/** 配置第0~15个被统计得到令牌个数的端口号
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   port_id   被统计得到令牌个数的端口号
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/3/15           STM模式下使用
************************************************************/
DPP_STATUS dpp_tm_shape_token_pp_cfg(ZXIC_UINT32 dev_id,
                                     ZXIC_UINT32 port_id)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 tmp_port = 0;
    ZXIC_UINT32 i = 0;
    DPP_ETM_CRDT_Q_TOKEN_STAUE_CFG_T crdt_q_token_staue_cfg = {0};

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, port_id, 0, 63);

    tmp_port = port_id;

    if (tmp_port > 48)
    {
        tmp_port = 48;
    }

    for (i = 0; i < 16; i++)
    {
        crdt_q_token_staue_cfg.test_token_q_id = tmp_port + i;
        rc = dpp_reg_write(dev_id,
                           ETM_CRDT_Q_TOKEN_STAUE_CFGr,
                           0,
                           i,
                           &crdt_q_token_staue_cfg);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    }


    return DPP_OK;
}


/***********************************************************/
/** 打印被指定统计的第0~15个端口消耗的令牌个数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM

* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  统计时间为2s，其中c桶统计1s，e桶统计1s
* @see
* @author  whuashan      @date  2019/03/15
************************************************************/
DPP_STATUS dpp_tm_shape_token_dec_cnt_diag(ZXIC_UINT32 dev_id)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 dec_num[16] = {0};
    ZXIC_UINT32 credit_value = 0;
    ZXIC_UINT32 port_num = 0;
    ZXIC_FLOAT traffic_amplified = 0.0;
    ZXIC_UINT32 shape_token_cycle_reg_index = 0;
    ZXIC_UINT32 shape_q_token_sta_cfg_reg_index = 0;
    ZXIC_UINT32 shape_test_token_calc_ctrl_reg_index = 0;
    ZXIC_UINT32 shape_q_token_dec_cnt_reg_index = 0;

    DPP_TM_WORK_MODE_E sa_work_mode = 0;
    DPP_TM_CNT_MODE_T que_get_mode = {0};
    DPP_ETM_CRDT_TEST_TOKEN_CALC_CTRL_T shap_test_token_calc_ctrl = {0};
    DPP_ETM_CRDT_TEST_TOKEN_SAMPLE_CYCLE_NUM_T test_token_sample_cycle_num = {0};
    DPP_ETM_CRDT_Q_TOKEN_STAUE_CFG_T q_token_staue_cfg = {0};
    DPP_ETM_CRDT_Q_TOKEN_DEC_CNT_T q_token_dec_cnt = {0};


    shape_token_cycle_reg_index = ETM_CRDT_TEST_TOKEN_SAMPLE_CYCLE_NUMr;
    shape_q_token_sta_cfg_reg_index = ETM_CRDT_Q_TOKEN_STAUE_CFGr;
    shape_test_token_calc_ctrl_reg_index = ETM_CRDT_TEST_TOKEN_CALC_CTRLr;
    shape_q_token_dec_cnt_reg_index = ETM_CRDT_Q_TOKEN_DEC_CNTr;


    /* 配置寄存器为读清模式 */
    rc = dpp_tm_cfgmt_cnt_mode_get(dev_id, &que_get_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_get");
    que_get_mode.count_rd_mode = 1;

    rc = dpp_tm_cfgmt_cnt_mode_set(dev_id, &que_get_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_set");

    /* 获取端口号 */
    rc = dpp_reg_read(dev_id,
                      shape_q_token_sta_cfg_reg_index,
                      0,
                      0,
                      &q_token_staue_cfg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    port_num = q_token_staue_cfg.test_token_q_id;

    /* 配置统计的时间，单位是令牌下发周期，令牌下发频率为600M/32 */
    test_token_sample_cycle_num.sample_cycle_num = 18750000;
    rc = dpp_reg_write(dev_id,
                       shape_token_cycle_reg_index,
                       0,
                       0,
                       &test_token_sample_cycle_num);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");


    /* 启动统计功能 */
    shap_test_token_calc_ctrl.test_token_calc_trigger = 1;
    rc = dpp_reg_write(dev_id,
                       shape_test_token_calc_ctrl_reg_index,
                       0,
                       0,
                       &shap_test_token_calc_ctrl);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    /* 等待统计完成 */
    rc = dpp_reg_read(dev_id,
                      shape_test_token_calc_ctrl_reg_index,
                      0,
                      0,
                      &shap_test_token_calc_ctrl);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    while (!shap_test_token_calc_ctrl.test_token_calc_state)
    {
        rc = dpp_reg_read(dev_id,
                          shape_test_token_calc_ctrl_reg_index,
                          0,
                          0,
                          &shap_test_token_calc_ctrl);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    }

    /* 关闭统计功能 */
    shap_test_token_calc_ctrl.test_token_calc_trigger = 0;
    rc = dpp_reg_write(dev_id,
                       shape_test_token_calc_ctrl_reg_index,
                       0,
                       0,
                       &shap_test_token_calc_ctrl);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");


    /* 读取计数器的计数 */
    for (i = 0; i < 16; i++)
    {
        rc = dpp_reg_read(dev_id,
                          shape_q_token_dec_cnt_reg_index,
                          0,
                          i,
                          &q_token_dec_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

        dec_num[i] = q_token_dec_cnt.q_token_dec_counter;

    }


    rc = dpp_tm_cfgmt_sa_work_mode_get(dev_id, &sa_work_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_sa_work_mode_get");

    if (0 == sa_work_mode)
    {
        rc = dpp_tm_qmu_credit_value_get(dev_id, &credit_value);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_credit_value_get");
    }
    else if (1 == sa_work_mode)
    {
        rc = dpp_tm_qmu_sa_credit_value_get(dev_id, 4, &credit_value);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_sa_credit_value_get");
    }
    else
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "SA work mode error\n");
        return DPP_ERR;
    }


    /* 计算各级消耗令牌速率:cnt*8*credit_value/(2*cycle_num*32/600M) */
    traffic_amplified = (ZXIC_FLOAT)(credit_value) * (ZXIC_FLOAT)(8.0) / ( (ZXIC_FLOAT)(2.0) * (ZXIC_FLOAT)(18750000.0));

    for (i = 0; i < 16; i++)
    {
        ZXIC_COMM_PRINT("pp_%d:            traffic = %.6f.(M)\n", (port_num + i), (ZXIC_FLOAT)(dec_num[i]) * traffic_amplified);
    }


    /* 配置寄存器为不读清模式 */
    rc = dpp_tm_cfgmt_cnt_mode_get(dev_id, &que_get_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_get");

    que_get_mode.count_rd_mode = 0;
    rc = dpp_tm_cfgmt_cnt_mode_set(dev_id, &que_get_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_set");

    return DPP_OK;
}


/***********************************************************/
/** 打印被指定统计的第0~15个端口接收的令牌个数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  统计时间为2s，其中c桶统计1s，e桶统计1s
* @see
* @author  whuashan      @date  2019/03/15
************************************************************/
DPP_STATUS dpp_tm_shape_token_dist_cnt_diag(ZXIC_UINT32 dev_id)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 dist_num[16] = {0};
    ZXIC_UINT32 credit_value = 0;
    ZXIC_UINT32 port_num = 0;
    ZXIC_FLOAT traffic_amplified = 0.0;
    ZXIC_UINT32 shape_token_cycle_reg_index = 0;
    ZXIC_UINT32 shape_q_token_sta_cfg_reg_index = 0;
    ZXIC_UINT32 shape_test_token_calc_ctrl_reg_index = 0;
    ZXIC_UINT32 shape_q_token_dist_cnt_reg_index = 0;
    DPP_TM_WORK_MODE_E sa_work_mode = 0;
    DPP_TM_CNT_MODE_T que_get_mode = {0};
    DPP_ETM_CRDT_TEST_TOKEN_CALC_CTRL_T shap_test_token_calc_ctrl = {0};
    DPP_ETM_CRDT_TEST_TOKEN_SAMPLE_CYCLE_NUM_T test_token_sample_cycle_num = {0};
    DPP_ETM_CRDT_Q_TOKEN_STAUE_CFG_T q_token_staue_cfg = {0};
    DPP_ETM_CRDT_Q_TOKEN_DIST_CNT_T q_token_dist_cnt = {0};

    shape_token_cycle_reg_index = ETM_CRDT_TEST_TOKEN_SAMPLE_CYCLE_NUMr;
    shape_q_token_sta_cfg_reg_index = ETM_CRDT_Q_TOKEN_STAUE_CFGr;
    shape_test_token_calc_ctrl_reg_index = ETM_CRDT_TEST_TOKEN_CALC_CTRLr;
    shape_q_token_dist_cnt_reg_index = ETM_CRDT_Q_TOKEN_DIST_CNTr;

    /* 配置寄存器为读清模式 */
    rc = dpp_tm_cfgmt_cnt_mode_get(dev_id, &que_get_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_get");
    que_get_mode.count_rd_mode = 1;

    rc = dpp_tm_cfgmt_cnt_mode_set(dev_id, &que_get_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_set");

    /* 获取端口号 */
    rc = dpp_reg_read(dev_id,
                      shape_q_token_sta_cfg_reg_index,
                      0,
                      0,
                      &q_token_staue_cfg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    port_num = q_token_staue_cfg.test_token_q_id;

    /* 配置统计的时间，单位是令牌下发周期，令牌下发频率为600M/32 */
    test_token_sample_cycle_num.sample_cycle_num = 18750000;
    rc = dpp_reg_write(dev_id,
                       shape_token_cycle_reg_index,
                       0,
                       0,
                       &test_token_sample_cycle_num);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");


    /* 启动统计功能 */
    shap_test_token_calc_ctrl.test_token_calc_trigger = 1;
    rc = dpp_reg_write(dev_id,
                       shape_test_token_calc_ctrl_reg_index,
                       0,
                       0,
                       &shap_test_token_calc_ctrl);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
    /* 等待统计完成 */
    rc = dpp_reg_read(dev_id,
                      shape_test_token_calc_ctrl_reg_index,
                      0,
                      0,
                      &shap_test_token_calc_ctrl);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    while (!shap_test_token_calc_ctrl.test_token_calc_state)
    {
        rc = dpp_reg_read(dev_id,
                          shape_test_token_calc_ctrl_reg_index,
                          0,
                          0,
                          &shap_test_token_calc_ctrl);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
    }

    /* 关闭统计功能 */
    shap_test_token_calc_ctrl.test_token_calc_trigger = 0;
    rc = dpp_reg_write(dev_id,
                       shape_test_token_calc_ctrl_reg_index,
                       0,
                       0,
                       &shap_test_token_calc_ctrl);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    /* 读取c桶计数器的计数 */
    for (i = 0; i < 16; i++)
    {

        rc = dpp_reg_read(dev_id,
                          shape_q_token_dist_cnt_reg_index,
                          0,
                          i,
                          &q_token_dist_cnt);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
        dist_num[i] = q_token_dist_cnt.q_token_dist_counter;

    }


    rc = dpp_tm_qmu_credit_value_get(dev_id, &credit_value);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_credit_value_get");
    rc = dpp_tm_cfgmt_sa_work_mode_get(dev_id, &sa_work_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_sa_work_mode_get");

    if (0 == sa_work_mode)
    {
        rc = dpp_tm_qmu_credit_value_get(dev_id, &credit_value);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_credit_value_get");
    }
    else if (1 == sa_work_mode)
    {
        rc = dpp_tm_qmu_sa_credit_value_get(dev_id, 4, &credit_value);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_sa_credit_value_get");
    }
    else
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "SA work mode error\n");
        return DPP_ERR;
    }

    /* 计算各级消耗令牌速率:cnt*8*credit_value/(2*cycle_num*32/600M) */
    traffic_amplified = (ZXIC_FLOAT)(credit_value) * (ZXIC_FLOAT)(8.0) / ( (ZXIC_FLOAT)(2.0) * (ZXIC_FLOAT)(18750000.0));

    for (i = 0; i < 16; i++)
    {
        ZXIC_COMM_PRINT("pp_%d:            traffic = %.6f.(M)\n", (port_num + i), (ZXIC_FLOAT)(dist_num[i]) * traffic_amplified);
    }


    /* 配置寄存器为不读清模式 */
    rc = dpp_tm_cfgmt_cnt_mode_get(dev_id, &que_get_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_get");
    que_get_mode.count_rd_mode = 0;
    rc = dpp_tm_cfgmt_cnt_mode_set(dev_id, &que_get_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_set");

    return DPP_OK;
}

/***********************************************************/
/** 打印指定的全局数组值以及清空全局数组
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   para_x   数组index_x
* @param   para_y   数组index_y
* @param   clear_flag 清空shape全局数组
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark
* @see
* @author  xuhb      @date  2019/06/10
************************************************************/
DPP_STATUS dpp_tm_shape_para_array_prt(ZXIC_UINT32 dev_id, ZXIC_UINT32 para_x, ZXIC_UINT32 para_y, ZXIC_UINT32 clear_flag)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 2);

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, para_x, 0, 21);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, para_y, 0, 127);
    ZXIC_COMM_PRINT("cir(kb):%d  cbs(KB):%d  num:%d\n", g_dpp_etm_shape_para_table[dev_id][para_x][para_y].shape_cir,
              g_dpp_etm_shape_para_table[dev_id][para_x][para_y].shape_cbs,
              g_dpp_etm_shape_para_table[dev_id][para_x][para_y].shape_num);

    if (clear_flag)
    {
        rc = dpp_tm_clr_shape_para(dev_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_clr_shape_para");
    }


    return DPP_OK;
}

#endif
/***********************************************************/
/** 获取全局数组中用户实际配置的整形值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   flow_id   流队列号 ETM:0-9215,FTM:0-2047
* @param   cir       cir速率，单位Kb，范围[64Kb - 160Gb]
* @param   cbs       cbs桶深，单位KB，范围[1KB - 64M]
*                    注：cbs=0 表示关闭整形,即不限速
* @param   mode_e    整形模式，0-获取c桶参数，1-获取对应e桶参数
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark
* @see
* @author  xuhb      @date  2020/09/22
************************************************************/
DPP_STATUS dpp_tm_shape_flow_para_array_get(DPP_DEV_T *dev,
                                      ZXIC_UINT32 flow_id,
                                      ZXIC_UINT32 mode,
                                      DPP_TM_SHAPE_PARA_TABLE *p_flow_para_tbl)
{

    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 flow_id_e = 0;
    ZXIC_UINT32 table_id = 0;
    ZXIC_UINT32 profile_id = 0;


    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), mode, 0, 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_flow_para_tbl);

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), flow_id, 0, DPP_ETM_Q_NUM - 1);

    flow_id_e = flow_id + DPP_ETM_Q_NUM;

    table_id = flow_id / 2048;

    /*获取流的profile_id*/
    if (mode)
    {
        table_id = flow_id_e / 2048;
        rc = dpp_tm_shape_map_table_get(dev, flow_id_e, &profile_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_shape_map_table_get");
    }
    else
    {
        rc = dpp_tm_shape_map_table_get(dev, flow_id, &profile_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_shape_map_table_get");

    }

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), profile_id, 0, DPP_TM_SHAP_MAP_ID_MAX - 1);

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), table_id, 0, DPP_ETM_SHAP_TABEL_ID_MAX - 1);
    p_flow_para_tbl->shape_cbs = g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][profile_id].shape_cbs;
    p_flow_para_tbl->shape_cir = g_dpp_etm_shape_para_table[DEV_PCIE_SLOT(dev)][table_id][profile_id].shape_cir;

    return DPP_OK;

}



#if 0
/***********************************************************/
/** 配置shap模块中 crd_grain授权价值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   credit_value   授权价值，默认值是0x5feByte
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/08/13
************************************************************/
DPP_STATUS dpp_tm_shap_crd_grain_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 credit_value)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_SHAP_CRD_GRAIN_T credit_val = {0};

    credit_val.crd_grain = credit_value;
    rc  = dpp_reg_write(dev_id,
                        ETM_SHAP_CRD_GRAINr,
                        0,
                        0,
                        &credit_val);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

#endif
#endif

#if ZXIC_REAL("TM_EXTEND_API")

#if 0
/***********************************************************/
/** 打印队列空标志查询
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   qnum   配置的队列号
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  cy      @date  2016/06/20
************************************************************/
DPP_STATUS dpp_tm_qlist_ept_flag_get_diag(ZXIC_UINT32 dev_id, ZXIC_UINT32 qnum)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 value = 0;

    rc = dpp_tm_qlist_ept_flag_get(dev_id, qnum, &value);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qlist_ept_flag_get");
    ZXIC_COMM_PRINT("qlist_ept_flag is %d\n", value);

    return DPP_OK;
}


/***********************************************************/
/** 打印队列深度计数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   qnum   配置的队列号
* @param   p_value  队列深度计数
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  cy      @date  2016/06/20
************************************************************/
DPP_STATUS dpp_tm_qlist_r_bcnt_get_diag(ZXIC_UINT32 dev_id, ZXIC_UINT32 qnum)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 value = 0;

    rc = dpp_tm_qlist_r_bcnt_get(dev_id, qnum, &value);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qlist_r_bcnt_get");
    ZXIC_COMM_PRINT("qlist_r_bcnt is 0x%x\n", value);

    return DPP_OK;
}

/***********************************************************/
/** CMDSCH中分端口分优先级的BLOCK计数
* @param   dev_id   设备编号
* @param   pri   
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  sun      @date  2023/09/19
************************************************************/
DPP_STATUS dpp_tm_csch_r_block_cnt_get(ZXIC_UINT32 dev_id,
                                       ZXIC_UINT32 pri,
                                       ZXIC_UINT32 *p_value)
{
    DPP_STATUS rc = DPP_OK;
    DPP_ETM_QMU_CSCH_R_BLOCK_CNT_T csch_r_block_cnt = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_value);

    rc = dpp_reg_read(dev_id,
                      ETM_QMU_CSCH_R_BLOCK_CNTr,
                      0,
                      pri,
                      &csch_r_block_cnt);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    *p_value = csch_r_block_cnt.csch_r_block_cnt;

    return DPP_OK;
}

/***********************************************************/
/** 打印CMDSCH中分端口分优先级的BLOCK计数
* @param   dev_id   设备编号
* @param   port   
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  sun      @date  2023/09/19
************************************************************/
DPP_STATUS dpp_tm_csch_r_block_cnt_diag(ZXIC_UINT32 dev_id, ZXIC_UINT32 port)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 pri = 0;
    ZXIC_UINT32 pri_th = 0;
    for(int i = 0; i < 8; ++i)
    {
        pri = port * 8 + i;
        rc = dpp_tm_csch_r_block_cnt_get(dev_id, pri, &pri_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_csch_r_block_cnt_get");
        ZXIC_COMM_PRINT("csch_r_block_pri 0x%x value is 0x%x\n", pri, pri_th);
    }

    return DPP_OK;
}

/***********************************************************/
/** 打印队列入链状态
* @param   dev_id   设备编号
* @param   flow_id   
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  sun      @date  2023/09/19
************************************************************/
DPP_STATUS dpp_tm_crdt_flow_link_state_get_diag(ZXIC_UINT32 dev_id, ZXIC_UINT32 flow_id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 link_status = 0;
    DPP_ETM_CRDT_FLOWQUE_INS_TBL_T crdt_flow_ins_tbl_t = {0};
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, flow_id, 0, DPP_ETM_CRDT_NUM);    
    rc = dpp_tm_crdt_flow_link_state_get(dev_id, flow_id, &crdt_flow_ins_tbl_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_crdt_flow_link_state_get");

    link_status = crdt_flow_ins_tbl_t.flowque_ins;
    ZXIC_COMM_PRINT("flowque_ins_tbl flow 0x%x linkstatus is 0x%x\n", flow_id, link_status);

    return DPP_OK;
}

/***********************************************************/
/** 打印调度器入链状态
* @param   dev_id   设备编号
* @param   flow_id   
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  sun      @date  2023/09/19
************************************************************/
DPP_STATUS dpp_tm_crdt_se_link_state_get_diag(ZXIC_UINT32 dev_id, ZXIC_UINT32 se_id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 link_status = 0;
    DPP_ETM_CRDT_SE_INS_TBL_T crdt_se_ins_tbl_t = {0};
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, se_id, 0, DPP_ETM_FQSPWFQ_NUM - 1);    
    rc = dpp_tm_crdt_se_link_state_get(dev_id, se_id, &crdt_se_ins_tbl_t);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_crdt_se_link_state_get");

    link_status = crdt_se_ins_tbl_t.se_ins_flag;
    ZXIC_COMM_PRINT("flowque_ins_tbl se_id 0x%x linkstatus is 0x%x\n", se_id, link_status);

    return DPP_OK;
}

/***********************************************************/
/** 打印olif的fifo是否空状态
* @param   dev_id   设备编号
* @param      
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  sun      @date  2023/09/19
************************************************************/
DPP_STATUS dpp_tm_olif_fifo_empty_state_get_diag(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 qmu_para_fifo_empty = 0;
    ZXIC_UINT32 emem_empty = 0;
    ZXIC_UINT32 imem_empty = 0;
    DPP_ETM_OLIF_OLIF_FIFO_EMPTY_STATE_T empty_status = {0};
  
    rc = dpp_reg_read(dev_id,
                      ETM_OLIF_OLIF_FIFO_EMPTY_STATEr,
                      0,
                      0,
                      &empty_status);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    qmu_para_fifo_empty = empty_status.qmu_para_fifo_empty;
    emem_empty = empty_status.emem_empty;
    imem_empty = empty_status.imem_empty;
    ZXIC_COMM_PRINT("qmu_para_fifo_empty 0x%x, emem_empty 0x%x, imem_empty 0x%x\n", qmu_para_fifo_empty, emem_empty, imem_empty);

    return DPP_OK;
}

/***********************************************************/
/**
* @param   dev_id
* @param   tm_type
* @param   que_id
*
* @return
* @remark  无
* @see
* @author  XXX      @date  2019/05/08
************************************************************/
DPP_STATUS dpp_tm_crdt_eir_crs_filter_en_get_diag(ZXIC_UINT32 dev_id, ZXIC_UINT32 que_id)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CRDT_EIR_CRS_FILTER_TBL_T eir_crs_filter = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, que_id, 0, DPP_ETM_Q_NUM - 1);

    rc = dpp_reg_read(dev_id,
                      ETM_CRDT_EIR_CRS_FILTER_TBLr,
                      0,
                      que_id,
                      &eir_crs_filter);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    ZXIC_COMM_PRINT("Tm type:%d [0:ftm 1:etm] eir_crs_filter_en:%d [1:enable 0:disable]", eir_crs_filter.eir_crs_filter);

    return rc;
}



/***********************************************************/
/**  读取指定队列获得授权个数(只打印授权非零的队列号)
* @param   dev_id   设备编号
* @param   ackflow_start   授权起始流号
* @param   ackflow_end   授权终止流号
* @param   sleep_time_ms   等待时间
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2017/01/19
************************************************************/
DPP_STATUS dpp_etm_crdt_traffic_diag(ZXIC_UINT32 dev_id,
                                     ZXIC_UINT32 ackflow_start,
                                     ZXIC_UINT32 ackflow_end,
                                     ZXIC_UINT32 sleep_time_ms)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 ackflow_id = 0;
    ZXIC_UINT32 credit_value = 0;
    ZXIC_FLOAT traffic_amplified = 0.0;
    ZXIC_FLOAT flow_spec_traffic = 0.0;

    DPP_ETM_CRDT_STAT_QUE_CREDIT_T que_credit = {0};
    DPP_TM_WORK_MODE_E sa_work_mode = {0};
    DPP_ETM_CRDT_STAT_QUE_ID_0_T stat_que_id_0 = {0};

    if (ackflow_start > DPP_ETM_Q_NUM - 1)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ackflow_start is out of range!!!\n");
        return DPP_ERR;
    }

    if (ackflow_end > DPP_ETM_Q_NUM - 1)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ackflow_end is out of range!!!\n");
        return DPP_ERR;
    }

    rc = dpp_tm_cfgmt_sa_work_mode_get(dev_id, &sa_work_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_sa_work_mode_get");

    if (0 == sa_work_mode)
    {
        rc = dpp_tm_qmu_credit_value_get(dev_id, &credit_value);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_credit_value_get");
    }
    else if (1 == sa_work_mode)
    {
        rc = dpp_tm_qmu_sa_credit_value_get(dev_id, 4, &credit_value);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_sa_credit_value_get");
    }
    else
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "SA work mode error\n");
        return DPP_ERR;
    }

    traffic_amplified = ((ZXIC_FLOAT)sleep_time_ms * (ZXIC_FLOAT)(1000000.0)) / ((ZXIC_FLOAT)(8.0) * (ZXIC_FLOAT)(credit_value));

    for (ackflow_id = ackflow_start; ackflow_id <= ackflow_end; ackflow_id += 16)
    {
        /* 配置16条授权流 */
        for (index = 0; index < 16; index++)
        {

            stat_que_id_0.stat_que_id_0 = ackflow_id + index;
            rc = dpp_reg_write(dev_id,
                               ETM_CRDT_STAT_QUE_ID_0r + index,
                               0,
                               0,
                               &stat_que_id_0);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");
        }

        /* 授权个数统计计数器清零 */
        rc = dpp_tm_crdt_clr_diag(0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_crdt_clr_diag");

        zxic_comm_sleep(sleep_time_ms);
        /* 不使能CRDT */
        rc = dpp_tm_crdt_credit_en_set(dev_id, 0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_crdt_credit_en_set");

        for (index = 0; index < 16; index++)
        {

            rc = dpp_reg_read(dev_id,
                              ETM_CRDT_STAT_QUE_ID_0r + index,
                              0,
                              0,
                              &stat_que_id_0);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "DPP_reg_read");

            rc = dpp_reg_read(dev_id,
                              ETM_CRDT_STAT_QUE_CREDITr,
                              0,
                              index,
                              &que_credit);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "DPP_reg_read");

            flow_spec_traffic = (ZXIC_FLOAT)(que_credit.stat_que_credit_cnt) / traffic_amplified;

            if (que_credit.stat_que_credit_cnt != 0)
            {
                ZXIC_COMM_PRINT("flow_0x%04x(%5d):   ", (stat_que_id_0.stat_que_id_0), (stat_que_id_0.stat_que_id_0));
                ZXIC_COMM_PRINT("ack_cnt = 0x%08x,   ", que_credit.stat_que_credit_cnt);
                ZXIC_COMM_PRINT("traffic = %.6f.(G)\n", flow_spec_traffic);
            }
        }
    }

    /* 使能CRDT */
    rc = dpp_tm_crdt_credit_en_set(dev_id, 1);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_crdt_credit_en_set");

    return DPP_OK;
}



/***********************************************************/
/** 读取QMU所有队列的统计信息
* @param   dev_id   设备编号
* @param   p_para   获得的统计信息
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_stat_get(ZXIC_UINT32 dev_id, DPP_ETM_QMU_STAT_INFO_T *p_para)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_FC_CNT_MODE_T fc_cnt_mode_reg = {0};
    DPP_ETM_QMU_MMU_QMU_WR_FC_CNT_T mmu_qmu_wr_fc_cnt_reg = {0};
    DPP_ETM_QMU_MMU_QMU_RD_FC_CNT_T mmu_qmu_rd_fc_cnt_reg = {0};
    DPP_ETM_QMU_QMU_CGAVD_FC_CNT_T qmu_cgavd_fc_cnt_reg = {0};
    DPP_ETM_QMU_CGAVD_QMU_PKT_CNT_T cgavd_qmu_pkt_cnt_reg = {0};
    DPP_ETM_QMU_CGAVD_QMU_PKTLEN_ALL_T cgavd_qmu_pktlen_all_reg = {0};
    DPP_ETM_QMU_LAST_DROP_QNUM_GET_T last_drop_qnum_reg = {0};
    DPP_ETM_QMU_CRDT_QMU_CREDIT_CNT_T crdt_qmu_credit_cnt_reg = {0};
    DPP_ETM_QMU_QMU_TO_QSCH_REPORT_CNT_T qmu_to_qsch_report_cnt_reg = {0};
    DPP_ETM_QMU_QMU_TO_CGAVD_REPORT_CNT_T qmu_to_cgavd_report_cnt_reg = {0};
    DPP_ETM_QMU_QMU_CRDT_CRS_NORMAL_CNT_T qmu_crdt_crs_normal_cnt_reg = {0};
    DPP_ETM_QMU_QMU_CRDT_CRS_OFF_CNT_T qmu_crdt_crs_off_cnt_reg = {0};
    DPP_ETM_QMU_QSCH_QLIST_SHEDULE_CNT_T qsch_qlist_shedule_cnt_reg = {0};
    DPP_ETM_QMU_QSCH_QLIST_SCH_EPT_CNT_T qsch_qlist_sch_ept_cnt_reg = {0};
    DPP_ETM_QMU_QMU_TO_MMU_BLK_WR_CNT_T qmu_to_mmu_blk_wr_cnt_reg = {0};
    DPP_ETM_QMU_QMU_TO_CSW_BLK_RD_CNT_T qmu_to_csw_blk_rd_cnt_reg = {0};
    DPP_ETM_QMU_QMU_TO_MMU_SOP_WR_CNT_T qmu_to_mmu_sop_wr_cnt_reg = {0};
    DPP_ETM_QMU_QMU_TO_MMU_EOP_WR_CNT_T qmu_to_mmu_eop_wr_cnt_reg = {0};
    DPP_ETM_QMU_QMU_TO_MMU_DROP_WR_CNT_T qmu_to_mmu_drop_wr_cnt_reg = {0};
    DPP_ETM_QMU_QMU_TO_CSW_SOP_RD_CNT_T qmu_to_csw_sop_rd_cnt_reg = {0};
    DPP_ETM_QMU_QMU_TO_CSW_EOP_RD_CNT_T qmu_to_csw_eop_rd_cnt_reg = {0};
    DPP_ETM_QMU_QMU_TO_CSW_DROP_RD_CNT_T qmu_to_csw_drop_rd_cnt_reg = {0};
    DPP_ETM_QMU_MMU_TO_QMU_WR_RELEASE_CNT_T mmu_to_qmu_wr_release_cnt_reg = {0};
    DPP_ETM_QMU_MMU_TO_QMU_RD_RELEASE_CNT_T mmu_to_qmu_rd_release_cnt_reg = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_para);

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_FC_CNT_MODEr,
                       0,
                       0,
                       &fc_cnt_mode_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_MMU_QMU_WR_FC_CNTr,
                       0,
                       0,
                       &mmu_qmu_wr_fc_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_MMU_QMU_RD_FC_CNTr,
                       0,
                       0,
                       &mmu_qmu_rd_fc_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_QMU_CGAVD_FC_CNTr,
                       0,
                       0,
                       &qmu_cgavd_fc_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_CGAVD_QMU_PKT_CNTr,
                       0,
                       0,
                       &cgavd_qmu_pkt_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_CGAVD_QMU_PKTLEN_ALLr,
                       0,
                       0,
                       &cgavd_qmu_pktlen_all_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_LAST_DROP_QNUM_GETr,
                       0,
                       0,
                       &last_drop_qnum_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_CRDT_QMU_CREDIT_CNTr,
                       0,
                       0,
                       &crdt_qmu_credit_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_QMU_TO_QSCH_REPORT_CNTr,
                       0,
                       0,
                       &qmu_to_qsch_report_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_QMU_TO_CGAVD_REPORT_CNTr,
                       0,
                       0,
                       &qmu_to_cgavd_report_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_QMU_CRDT_CRS_NORMAL_CNTr,
                       0,
                       0,
                       &qmu_crdt_crs_normal_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_QMU_CRDT_CRS_OFF_CNTr,
                       0,
                       0,
                       &qmu_crdt_crs_off_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_QSCH_QLIST_SHEDULE_CNTr,
                       0,
                       0,
                       &qsch_qlist_shedule_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_QSCH_QLIST_SCH_EPT_CNTr,
                       0,
                       0,
                       &qsch_qlist_sch_ept_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_QMU_TO_MMU_BLK_WR_CNTr,
                       0,
                       0,
                       &qmu_to_mmu_blk_wr_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_QMU_TO_CSW_BLK_RD_CNTr,
                       0,
                       0,
                       &qmu_to_csw_blk_rd_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_QMU_TO_MMU_SOP_WR_CNTr,
                       0,
                       0,
                       &qmu_to_mmu_sop_wr_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_QMU_TO_MMU_EOP_WR_CNTr,
                       0,
                       0,
                       &qmu_to_mmu_eop_wr_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_QMU_TO_MMU_DROP_WR_CNTr,
                       0,
                       0,
                       &qmu_to_mmu_drop_wr_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_QMU_TO_CSW_SOP_RD_CNTr,
                       0,
                       0,
                       &qmu_to_csw_sop_rd_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_QMU_TO_CSW_EOP_RD_CNTr,
                       0,
                       0,
                       &qmu_to_csw_eop_rd_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_QMU_TO_CSW_DROP_RD_CNTr,
                       0,
                       0,
                       &qmu_to_csw_drop_rd_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_MMU_TO_QMU_WR_RELEASE_CNTr,
                       0,
                       0,
                       &mmu_to_qmu_wr_release_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_MMU_TO_QMU_RD_RELEASE_CNTr,
                       0,
                       0,
                       &mmu_to_qmu_rd_release_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    p_para->fc_cnt_mode               = fc_cnt_mode_reg.fc_cnt_mode;
    p_para->mmu_qmu_wr_fc_cnt         = mmu_qmu_wr_fc_cnt_reg.mmu_qmu_wr_fc_cnt;
    p_para->mmu_qmu_rd_fc_cnt         = mmu_qmu_rd_fc_cnt_reg.mmu_qmu_rd_fc_cnt;
    p_para->qmu_cgavd_fc_cnt          = qmu_cgavd_fc_cnt_reg.qmu_cgavd_fc_cnt;
    p_para->cgavd_qmu_pkt_cnt         = cgavd_qmu_pkt_cnt_reg.cgavd_qmu_pkt_cnt;
    p_para->cgavd_qmu_pktlen_all      = cgavd_qmu_pktlen_all_reg.cgavd_qmu_pktlen_all;
    p_para->cgavd_qmu_drop_tap        = last_drop_qnum_reg.cgavd_qmu_drop_tap;
    p_para->last_drop_qnum            = last_drop_qnum_reg.last_drop_qnum;
    p_para->crdt_qmu_credit_cnt       = crdt_qmu_credit_cnt_reg.crdt_qmu_credit_cnt;
    p_para->qmu_to_qsch_report_cnt    = qmu_to_qsch_report_cnt_reg.qmu_to_qsch_report_cnt;
    p_para->qmu_to_cgavd_report_cnt   = qmu_to_cgavd_report_cnt_reg.qmu_to_cgavd_report_cnt;
    p_para->qmu_crdt_crs_normal_cnt   = qmu_crdt_crs_normal_cnt_reg.qmu_crdt_crs_normal_cnt;
    p_para->qmu_crdt_crs_off_cnt      = qmu_crdt_crs_off_cnt_reg.qmu_crdt_crs_off_cnt;
    p_para->qsch_qlist_shedule_cnt    = qsch_qlist_shedule_cnt_reg.qsch_qlist_shedule_cnt;
    p_para->qsch_qlist_sch_ept_cnt    = qsch_qlist_sch_ept_cnt_reg.qsch_qlist_sch_ept_cnt;
    p_para->qmu_to_mmu_blk_wr_cnt     = qmu_to_mmu_blk_wr_cnt_reg.qmu_to_mmu_blk_wr_cnt;
    p_para->qmu_to_csw_blk_rd_cnt     = qmu_to_csw_blk_rd_cnt_reg.qmu_to_csw_blk_rd_cnt;
    p_para->qmu_to_mmu_sop_wr_cnt     = qmu_to_mmu_sop_wr_cnt_reg.qmu_to_mmu_sop_wr_cnt;
    p_para->qmu_to_mmu_eop_wr_cnt     = qmu_to_mmu_eop_wr_cnt_reg.qmu_to_mmu_eop_wr_cnt;
    p_para->qmu_to_mmu_drop_wr_cnt    = qmu_to_mmu_drop_wr_cnt_reg.qmu_to_mmu_drop_wr_cnt;
    p_para->qmu_to_csw_sop_rd_cnt     = qmu_to_csw_sop_rd_cnt_reg.qmu_to_csw_sop_rd_cnt;
    p_para->qmu_to_csw_eop_rd_cnt     = qmu_to_csw_eop_rd_cnt_reg.qmu_to_csw_eop_rd_cnt;
    p_para->qmu_to_csw_drop_rd_cnt    = qmu_to_csw_drop_rd_cnt_reg.qmu_to_csw_drop_rd_cnt;
    p_para->mmu_to_qmu_wr_release_cnt = mmu_to_qmu_wr_release_cnt_reg.mmu_to_qmu_wr_release_cnt;
    p_para->mmu_to_qmu_rd_release_cnt = mmu_to_qmu_rd_release_cnt_reg.mmu_to_qmu_rd_release_cnt;

    return DPP_OK;

}


/***********************************************************/
/** 读取QMU指定队列的计数信息
* @param   dev_id   设备编号
* @param   p_para   获得的统计信息
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_spec_q_stat_get(ZXIC_UINT32 dev_id, DPP_ETM_QMU_SPEC_Q_STAT_INFO_T *p_para)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_OBSERVE_PORTFC_SPEC_T observe_portfc_spec_reg = {0};
    DPP_ETM_QMU_SPEC_LIF_PORTFC_COUNT_T spec_lif_portfc_count_reg = {0};
    DPP_ETM_QMU_OBSERVE_QNUM_SET_T observe_qnum_set_reg = {0};
    DPP_ETM_QMU_SPEC_Q_PKT_RECEIVED_T spec_q_pkt_received_reg = {0};
    DPP_ETM_QMU_SPEC_Q_PKT_DROPPED_T spec_q_pkt_dropped_reg = {0};
    DPP_ETM_QMU_SPEC_Q_PKT_SCHEDULED_T spec_q_pkt_scheduled_reg = {0};
    DPP_ETM_QMU_SPEC_Q_WR_CMD_SENT_T spec_q_wr_cmd_sent_reg = {0};
    DPP_ETM_QMU_SPEC_Q_RD_CMD_SENT_T spec_q_rd_cmd_sent_reg = {0};
    DPP_ETM_QMU_SPEC_Q_PKT_ENQ_T spec_q_pkt_enq_reg = {0};
    DPP_ETM_QMU_SPEC_Q_PKT_DEQ_T spec_q_pkt_deq_reg = {0};
    DPP_ETM_QMU_SPEC_Q_CRDT_UNCON_RECEIVED_T spec_q_crdt_uncon_received_reg = {0};
    DPP_ETM_QMU_SPEC_Q_CRDT_CONG_RECEIVED_T spec_q_crdt_cong_received_reg = {0};
    DPP_ETM_QMU_SPEC_Q_CRS_NORMAL_CNT_T spec_q_crs_normal_cnt_reg = {0};
    DPP_ETM_QMU_SPEC_Q_CRS_OFF_CNT_T spec_q_crs_off_cnt_reg = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_para);

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_OBSERVE_PORTFC_SPECr,
                       0,
                       0,
                       &observe_portfc_spec_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_LIF_PORTFC_COUNTr,
                       0,
                       0,
                       &spec_lif_portfc_count_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_OBSERVE_QNUM_SETr,
                       0,
                       0,
                       &observe_qnum_set_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_Q_PKT_RECEIVEDr,
                       0,
                       0,
                       &spec_q_pkt_received_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_Q_PKT_DROPPEDr,
                       0,
                       0,
                       &spec_q_pkt_dropped_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_Q_PKT_SCHEDULEDr,
                       0,
                       0,
                       &spec_q_pkt_scheduled_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_Q_WR_CMD_SENTr,
                       0,
                       0,
                       &spec_q_wr_cmd_sent_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_Q_RD_CMD_SENTr,
                       0,
                       0,
                       &spec_q_rd_cmd_sent_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_Q_PKT_ENQr,
                       0,
                       0,
                       &spec_q_pkt_enq_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_Q_PKT_DEQr,
                       0,
                       0,
                       &spec_q_pkt_deq_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_Q_CRDT_UNCON_RECEIVEDr,
                       0,
                       0,
                       &spec_q_crdt_uncon_received_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_Q_CRDT_CONG_RECEIVEDr,
                       0,
                       0,
                       &spec_q_crdt_cong_received_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_Q_CRS_NORMAL_CNTr,
                       0,
                       0,
                       &spec_q_crs_normal_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_Q_CRS_OFF_CNTr,
                       0,
                       0,
                       &spec_q_crs_off_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    p_para->observe_portfc_spec        =  observe_portfc_spec_reg.observe_portfc_spec;
    p_para->spec_lif_portfc_count      =  spec_lif_portfc_count_reg.spec_lif_portfc_count;
    p_para->observe_qnum_set           =  observe_qnum_set_reg.observe_qnum_set;
    p_para->spec_q_pkt_received        =  spec_q_pkt_received_reg.spec_q_pkt_received;
    p_para->spec_q_pkt_dropped         =  spec_q_pkt_dropped_reg.spec_q_pkt_dropped;
    p_para->spec_q_pkt_scheduled       =  spec_q_pkt_scheduled_reg.spec_q_pkt_scheduled;
    p_para->spec_q_wr_cmd_sent         =  spec_q_wr_cmd_sent_reg.spec_q_wr_cmd_sent;
    p_para->spec_q_rd_cmd_sent         =  spec_q_rd_cmd_sent_reg.spec_q_rd_cmd_sent;
    p_para->spec_q_pkt_enq             =  spec_q_pkt_enq_reg.spec_q_pkt_enq;
    p_para->spec_q_pkt_deq             =  spec_q_pkt_deq_reg.spec_q_pkt_deq;
    p_para->spec_q_crdt_uncon_received =  spec_q_crdt_uncon_received_reg.spec_q_crdt_uncon_received;
    p_para->spec_q_crdt_cong_received  =  spec_q_crdt_cong_received_reg.spec_q_crdt_cong_received;
    p_para->spec_q_crs_normal_cnt      =  spec_q_crs_normal_cnt_reg.spec_q_crs_normal_cnt;
    p_para->spec_q_crs_off_cnt         =  spec_q_crs_off_cnt_reg.spec_q_crs_off_cnt;

    return DPP_OK;

}

/***********************************************************/
/** 读取QMU指定队列组的计数信息
* @param   dev_id   设备编号
* @param   p_para   获得的统计信息
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_spec_bat_stat_get(ZXIC_UINT32 dev_id, DPP_ETM_QMU_SPEC_BAT_STAT_INFO_T *p_para)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_QMU_OBSERVE_BATCH_SET_T observe_batch_set_reg = {0};
    DPP_ETM_QMU_SPEC_BAT_PKT_RECEIVED_T spec_bat_pkt_received_reg = {0};
    DPP_ETM_QMU_SPEC_BAT_PKT_DROPPED_T spec_bat_pkt_dropped_reg = {0};
    DPP_ETM_QMU_SPEC_BAT_BLK_SCHEDULED_T spec_bat_blk_scheduled_reg = {0};
    DPP_ETM_QMU_SPEC_BAT_WR_CMD_SENT_T spec_bat_wr_cmd_sent_reg = {0};
    DPP_ETM_QMU_SPEC_BAT_RD_CMD_SENT_T spec_bat_rd_cmd_sent_reg = {0};
    DPP_ETM_QMU_SPEC_BAT_PKT_ENQ_T spec_bat_pkt_enq_reg = {0};
    DPP_ETM_QMU_SPEC_BAT_PKT_DEQ_T spec_bat_pkt_deq_reg = {0};
    DPP_ETM_QMU_SPEC_BAT_CRDT_UNCON_RECEIVED_T spec_bat_crdt_uncon_received_reg = {0};
    DPP_ETM_QMU_SPEC_BAT_CRDT_CONG_RECEIVED_T spec_bat_crdt_cong_received_reg = {0};
    DPP_ETM_QMU_SPEC_BAT_CRS_NORMAL_CNT_T spec_bat_crs_normal_cnt_reg = {0};
    DPP_ETM_QMU_SPEC_BAT_CRS_OFF_CNT_T spec_bat_crs_off_cnt_reg = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_para);

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_OBSERVE_BATCH_SETr,
                       0,
                       0,
                       &observe_batch_set_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_BAT_PKT_RECEIVEDr,
                       0,
                       0,
                       &spec_bat_pkt_received_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_BAT_PKT_DROPPEDr,
                       0,
                       0,
                       &spec_bat_pkt_dropped_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_BAT_BLK_SCHEDULEDr,
                       0,
                       0,
                       &spec_bat_blk_scheduled_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_BAT_WR_CMD_SENTr,
                       0,
                       0,
                       &spec_bat_wr_cmd_sent_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_BAT_RD_CMD_SENTr,
                       0,
                       0,
                       &spec_bat_rd_cmd_sent_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_BAT_PKT_ENQr,
                       0,
                       0,
                       &spec_bat_pkt_enq_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_BAT_PKT_DEQr,
                       0,
                       0,
                       &spec_bat_pkt_deq_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_BAT_CRDT_UNCON_RECEIVEDr,
                       0,
                       0,
                       &spec_bat_crdt_uncon_received_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_BAT_CRDT_CONG_RECEIVEDr,
                       0,
                       0,
                       &spec_bat_crdt_cong_received_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_BAT_CRS_NORMAL_CNTr,
                       0,
                       0,
                       &spec_bat_crs_normal_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev_id,
                       ETM_QMU_SPEC_BAT_CRS_OFF_CNTr,
                       0,
                       0,
                       &spec_bat_crs_off_cnt_reg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");

    p_para->observe_batch_set            =  observe_batch_set_reg.observe_batch_set;
    p_para->spec_bat_pkt_received        =  spec_bat_pkt_received_reg.spec_bat_pkt_received;
    p_para->spec_bat_pkt_dropped         =  spec_bat_pkt_dropped_reg.spec_bat_pkt_dropped;
    p_para->spec_bat_blk_scheduled       =  spec_bat_blk_scheduled_reg.spec_bat_blk_scheduled;
    p_para->spec_bat_wr_cmd_sent         =  spec_bat_wr_cmd_sent_reg.spec_bat_wr_cmd_sent;
    p_para->spec_bat_rd_cmd_sent         =  spec_bat_rd_cmd_sent_reg.spec_bat_rd_cmd_sent;
    p_para->spec_bat_pkt_enq             =  spec_bat_pkt_enq_reg.spec_bat_pkt_enq;
    p_para->spec_bat_pkt_deq             =  spec_bat_pkt_deq_reg.spec_bat_pkt_deq;
    p_para->spec_bat_crdt_uncon_received =  spec_bat_crdt_uncon_received_reg.spec_bat_crdt_uncon_received;
    p_para->spec_bat_crdt_cong_received  =  spec_bat_crdt_cong_received_reg.spec_bat_crdt_cong_received;
    p_para->spec_bat_crs_normal_cnt      =  spec_bat_crs_normal_cnt_reg.spec_bat_crs_normal_cnt;
    p_para->spec_bat_crs_off_cnt         =  spec_bat_crs_off_cnt_reg.spec_bat_crs_off_cnt;

    return DPP_OK;

}

/***********************************************************/
/** 连续配置各级搬移门限
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   start_id 为起始 队列号或端口号，系统级时，id参数无效
* @param   value   端口级和系统级时，为搬移门限值，单位为NPPU存包的单位，256B；
                   流级时为搬移profile_id,0~15
* @param  num   为队列或端口个数
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2016/11/19
************************************************************/
#ifdef ETM_REAL
DPP_STATUS dpp_tm_cgavd_move_th_together_wr(ZXIC_UINT32 dev_id,
                                            DPP_TM_CGAVD_LEVEL_E level,
                                            ZXIC_UINT32 start_id,
                                            ZXIC_UINT32 value,
                                            ZXIC_UINT32 num)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32  i = 0;
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, level, QUEUE_LEVEL, SYS_LEVEL);
    

    for (i = 0; i < num; i++)
    {
        ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW(start_id, i);
        rc  = dpp_tm_cgavd_move_th_set(dev_id, level, start_id + i, value);
        zxic_comm_usleep(100);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_move_th_set");

    }

    return DPP_OK;

}
#endif
#endif
/***********************************************************/
/** 连续设置多个队列或端口的TD门限
* @param   level   层次号，0:队列级，1:端口级
* @param   tm_type   0-ETM,1-FTM
* @param   id   起始队列号或端口号
* @param   td_th   TD门限，单位是KB，转换为block写入寄存器
* @param   num  需要设置的队列或端口数量
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_td_th_together_wr(DPP_DEV_T *dev,
                                          ZXIC_UINT32 level,
                                          ZXIC_UINT32 id,
                                          ZXIC_UINT32 td_th,
                                          ZXIC_UINT32 num)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32  i = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), level, QUEUE_LEVEL, PP_LEVEL);
    ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev),  id , num);

    for (i = 0; i < num; i++)
    {
        rc  = dpp_tm_cgavd_td_th_set(dev, level, id + i, td_th);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_cgavd_td_th_set");
    }

    return DPP_OK;

}


/***********************************************************/
/** 连续获取多个队列或端口的TD门限
* @param   level   层次号，0:队列级，1:端口级
* @param   tm_type   0-ETM,1-FTM
* @param   level   拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   id   起始队列号或端口号
* @param   num  需要设置的队列或端口数量
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/03/22
************************************************************/
DPP_STATUS dpp_tm_cgavd_td_th_together_get(DPP_DEV_T *dev,
                                           ZXIC_UINT32 level,
                                           ZXIC_UINT32 id,
                                           ZXIC_UINT32 num)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32  i = 0;
    ZXIC_UINT32  td_th = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), level, QUEUE_LEVEL, PP_LEVEL);
    ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev),  id , num);

    for (i = 0; i < num; i++)
    {
        rc  = dpp_tm_cgavd_td_th_get(dev, level, id + i, &td_th);
        zxic_comm_delay(5);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_cgavd_td_th_get");

        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev), id , i);
        if (level == QUEUE_LEVEL)
        {
            ZXIC_COMM_PRINT("   flow_id: 0x%x,   td_th: 0x%x\n", id + i, td_th);
        }
        else
        {
            ZXIC_COMM_PRINT("   pp_id: 0x%x,   td_th: 0x%x\n", id + i, td_th);
        }

    }

    return DPP_OK;

}

/***********************************************************/
/** 连续配置多个队列或端口是否支持动态门限机制
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   WRED支持层次号，0:队列级，1:端口级
* @param   id   队列号或端口号
* @param   en   配置的值，0-不支持动态门限机制，1-支持动态门限机制
* @param   num   连续配置的队列数
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_dyn_th_en_set_more(DPP_DEV_T *dev,
                                           DPP_TM_CGAVD_LEVEL_E level,
                                           ZXIC_UINT32 id,
                                           ZXIC_UINT32 en,
                                           ZXIC_UINT32 num)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32  i = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), level, QUEUE_LEVEL, PP_LEVEL);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev),  id , num);

    for (i = 0; i < num; i++)
    {
        rc  = dpp_tm_cgavd_dyn_th_en_set(dev, level, id + i, en);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_tm_cgavd_dyn_th_en_set");

    }

    return DPP_OK;

}

#if 0
/***********************************************************/
/** 配置各级WRED丢弃曲线对应的参数
* @param   tm_type   0-ETM,1-FTM
* @param   level   WRED支持层次号，0:队列级，1:端口级
* @param   wred_id   队列级共支持16个WRED组0-15，端口级支持8组0-7
* @param   dp   共支持8个dp，取值0-7
* @param   max_th  平均队列深度上限阈值
* @param   min_th  平均队列深度下限阈值
* @param   max_p  最大丢弃概率
* @param   weight   平均队列深度计算权重
* @param   q_len_th   队列深度阈值
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  taq      @date  2015/04/20
************************************************************/
DPP_STATUS dpp_tm_wred_dp_line_para_wr(ZXIC_UINT32 dev_id,
                                       ZXIC_UINT32 level,
                                       ZXIC_UINT32 wred_id,
                                       ZXIC_UINT32 dp,
                                       ZXIC_UINT32 max_th,
                                       ZXIC_UINT32 min_th,
                                       ZXIC_UINT32 max_p,
                                       ZXIC_UINT32 weight,
                                       ZXIC_UINT32 q_len_th)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_TM_WRED_DP_LINE_PARA_T para = {0};

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, level, QUEUE_LEVEL, PP_LEVEL);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dp, 0, DPP_TM_DP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, max_p, 1, DPP_TM_RED_P_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, weight, 0, DPP_TM_CGAVD_WEIGHT_MAX);

    if (QUEUE_LEVEL == level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, wred_id, 0, DPP_TM_Q_WRED_NUM - 1);
    }
    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, wred_id, 0, DPP_TM_PP_WRED_NUM - 1);
    }

    para.max_th = max_th;
    para.min_th = min_th;
    para.max_p = max_p;
    para.weight = weight;
    para.q_len_th = q_len_th;

    rc = dpp_tm_cgavd_wred_dp_line_para_set(dev_id, level, wred_id, dp, &para);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_wred_dp_line_para_set");

    return DPP_OK;

}


/***********************************************************/
/** 配置各级WRED丢弃曲线对应的参数
* @param   tm_type   0-ETM,1-FTM
* @param   level   WRED支持层次号，0:队列级，1:端口级
* @param   wred_id   队列级共支持16个WRED组0-15，端口级支持8组0-7
* @param   dp   共支持8个dp，取值0-7
* @param   max_th  平均队列深度上限阈值
* @param   min_th  平均队列深度下限阈值
* @param   max_p  最大丢弃概率
* @param   weight   平均队列深度计算权重
* @param   q_len_th   队列深度阈值
* @param   flag   忽略乘法里的当前包长和最大包长比标志位:1为忽略
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy     @date  2015/11/9
************************************************************/
DPP_STATUS dpp_tm_wred_dp_line_para_flag_wr(ZXIC_UINT32 dev_id,
                                            ZXIC_UINT32 level,
                                            ZXIC_UINT32 wred_id,
                                            ZXIC_UINT32 dp,
                                            ZXIC_UINT32 max_th,
                                            ZXIC_UINT32 min_th,
                                            ZXIC_UINT32 max_p,
                                            ZXIC_UINT32 weight,
                                            ZXIC_UINT32 q_len_th,
                                            ZXIC_UINT32 flag)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_TM_WRED_DP_LINE_PARA_T para = {0};
    DPP_ETM_CGAVD_PKE_LEN_CALC_SIGN_T cgavd_pke_len_calc_sign = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, level, QUEUE_LEVEL, PP_LEVEL);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dp, 0, DPP_TM_DP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, max_p, 1, DPP_TM_RED_P_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, weight, 0, DPP_TM_CGAVD_WEIGHT_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, flag, 0, 1);

    if (QUEUE_LEVEL == level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, wred_id, 0, DPP_TM_Q_WRED_NUM - 1);
    }
    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, wred_id, 0, DPP_TM_PP_WRED_NUM - 1);
    }

    para.max_th = max_th;
    para.min_th = min_th;
    para.max_p = max_p;
    para.weight = weight;
    para.q_len_th = q_len_th;

    cgavd_pke_len_calc_sign.pke_len_calc_sign = flag;
    rc  = dpp_reg_write(dev_id,
                        ETM_CGAVD_PKE_LEN_CALC_SIGNr,
                        0,
                        0,
                        &cgavd_pke_len_calc_sign);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    rc = dpp_tm_cgavd_wred_dp_line_para_set(dev_id, level, wred_id, dp, &para);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_wred_dp_line_para_set");

    return DPP_OK;

}


/***********************************************************/
/** 配置CPU设置的报文长度是否参与计算丢弃概率的使能
* @param   tm_type   0-ETM,1-FTM
* @param   flag   忽略乘法里的当前包长和最大包长比标志位:1为忽略
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy    @date  2015/11/9
************************************************************/
DPP_STATUS dpp_tm_cgavd_wred_pke_len_calc_sign_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 flag)
{
    DPP_STATUS  rc = DPP_OK;

    DPP_ETM_CGAVD_PKE_LEN_CALC_SIGN_T cgavd_pke_len_calc_sign = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, flag, 0, 1);

    cgavd_pke_len_calc_sign.pke_len_calc_sign = flag;
    rc  = dpp_reg_write(dev_id,
                        ETM_CGAVD_PKE_LEN_CALC_SIGNr,
                        0,
                        0,
                        &cgavd_pke_len_calc_sign);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;

}


/***********************************************************/
/** 获取配置CPU设置的报文长度是否参与计算丢弃概率的使能
* @param   tm_type   0-ETM,1-FTM
* @param   flag   忽略乘法里的当前包长和最大包长比标志位:1为忽略
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy     @date  2015/11/9
************************************************************/
DPP_STATUS dpp_tm_cgavd_wred_pke_len_calc_sign_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_flag)
{
    DPP_STATUS  rc = DPP_OK;

    DPP_ETM_CGAVD_PKE_LEN_CALC_SIGN_T cgavd_pke_len_calc_sign = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_flag);

    rc  = dpp_reg_read(dev_id,
                       ETM_CGAVD_PKE_LEN_CALC_SIGNr,
                       0,
                       0,
                       &cgavd_pke_len_calc_sign);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    *p_flag = cgavd_pke_len_calc_sign.pke_len_calc_sign;

    return DPP_OK;

}

/***********************************************************/
/** 配置系统级GRED丢弃曲线对应的参数
* @param   tm_type   0-ETM,1-FTM
* @param   dp   共支持8个dp，取值0-7
* @param   max_th  平均队列深度上限阈值
* @param   mid_th  平均队列深度中间阈值
* @param   min_th  平均队列深度下限阈值
* @param   max_p   最大丢弃概率   1~99
* @param   weight   平均队列深度计算权重
* @param   q_len_th   队列深度阈值
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  taq      @date  2015/04/20
************************************************************/
DPP_STATUS dpp_tm_gred_dp_line_para_wr(ZXIC_UINT32 dev_id,
                                       ZXIC_UINT32 dp,
                                       ZXIC_UINT32 max_th,
                                       ZXIC_UINT32 mid_th,
                                       ZXIC_UINT32 min_th,
                                       ZXIC_UINT32 max_p,
                                       ZXIC_UINT32 weight,
                                       ZXIC_UINT32 q_len_th)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_TM_GRED_DP_LINE_PARA_T para = {0};

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dp, 0, DPP_TM_DP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, max_p, 1, DPP_TM_RED_P_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, weight, 0, DPP_TM_CGAVD_WEIGHT_MAX);

    para.max_th = max_th;
    para.mid_th = mid_th;
    para.min_th = min_th;
    para.max_p = max_p;


    para.weight = weight;
    para.q_len_th = q_len_th;

    rc = dpp_tm_cgavd_gred_dp_line_para_set(dev_id, dp, &para);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_gred_dp_line_para_set");


    return DPP_OK;

}



/***********************************************************/
/** 配置olif统计组信息
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   id   olif统计组号
* @param   all_or_by_port   0-统计所有，1-统计某一端口或某一dest_id
* @param   i_or_e_sel   10-统计片外，01-统计片内，其他值-统计所有
* @param   port_or_dest_id_sel   0-统计port，1-统计dest_id
* @param   port_dest_id   port号或dest_id号
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/04/21
************************************************************/
DPP_STATUS dpp_tm_olif_stat_set(ZXIC_UINT32 dev_id,
                                ZXIC_UINT32 id,
                                ZXIC_UINT32 all_or_by_port,
                                ZXIC_UINT32 i_or_e_sel,
                                ZXIC_UINT32 port_or_dest_id_sel,
                                ZXIC_UINT32 port_dest_id)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_OLIF_TM_LIF_STAT_CFG_T tm_lif_stat_cft = {0};

    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, id, 0, 15);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, all_or_by_port, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, i_or_e_sel, 0, 3);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, port_or_dest_id_sel, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, port_dest_id, 0, 255);


    tm_lif_stat_cft.all_or_by_port = all_or_by_port;
    tm_lif_stat_cft.i_or_e_sel = i_or_e_sel;
    tm_lif_stat_cft.port_or_dest_id_sel = port_or_dest_id_sel;
    tm_lif_stat_cft.port_dest_id = port_dest_id;
    rc  = dpp_reg_write(dev_id,
                        ETM_OLIF_TM_LIF_STAT_CFGr,
                        0,
                        id,
                        &tm_lif_stat_cft);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/**
* @param   dev_id
* @param   tm_type
* @param   i_or_e_sel
* @param   port_or_dest_id_sel
* @param   start_id
* @param   start_port_dest_id
* @param   num
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xjw      @date  2018/02/01
************************************************************/
DPP_STATUS dpp_tm_olif_stat_set_mul(ZXIC_UINT32 dev_id,
                                    ZXIC_UINT32 i_or_e_sel,
                                    ZXIC_UINT32 port_or_dest_id_sel,
                                    ZXIC_UINT32 start_id,
                                    ZXIC_UINT32 start_port_dest_id,
                                    ZXIC_UINT32 num)
{
    DPP_STATUS rt = DPP_OK;
    ZXIC_UINT32 i = 0;

    for (i = 0; i < num; i++)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id, start_port_dest_id, i);
        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id,  start_id , i);
        
        rt = dpp_tm_olif_stat_set(dev_id, start_id + i,
                                  1, i_or_e_sel,  port_or_dest_id_sel, start_port_dest_id + i);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rt, "dpp_reg_write");
    }

    return rt;
}

DPP_STATUS dpp_tm_mr_init(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;
    rc = dpp_tm_qmu_qos_sign_set(dev_id, 0, 0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_qos_sign_set");
    rc = dpp_tm_cgavd_q_map_pp_set(dev_id, 0, 60);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_q_map_pp_set");
    rc = dpp_tm_cgavd_td_byte_block_th_get_diag(dev_id, 1, 60);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_td_byte_block_th_get_diag");
    rc = dpp_tm_cgavd_td_byte_block_th_set(dev_id, 0, 0, 1024);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_td_byte_block_th_set");
    rc = dpp_tm_cgavd_td_th_set(dev_id, 0, 0, 200);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_td_th_set");
    // rc = dpp_tm_shape_pp_para_wr(dev_id, 60, 1000000, 1000, 1);
    // ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_shape_pp_para_wr");
    rc = dpp_tm_qmu_port_shape_set(dev_id, 60, 4, 31, 8192, 1);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_port_shape_set");
    
    return rc;
}

#if 0
/***********************************************************/
/** 配置TM模式下初始化代码
* @param   dev_id
* @param   tm_type   0-ETM,1-FTM
* @param   p_tm_init_info    配置TM模式下初始化信息包括以下
*              blk_size    配置qmu block大小：512B[default]/1024B
*              case_num    QMU初始化场景编号：0/1/2/3...
*              imem_omem;  0-片内外混合; 1-纯片内;2-纯片外
*              mode        0-TM; 1-SA
*
* @return
* @remark  无
* @see
* @author  whuashan      @date  2015/03/26
************************************************************/
DPP_STATUS dpp_tm_asic_init(ZXIC_UINT32 dev_id, DPP_TM_ASIC_INIT_INFO_T *p_tm_asic_init_info)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 case_num = 0;
    ZXIC_UINT32 blk_size = 0;
    ZXIC_UINT32 imem_omem = 0;
    ZXIC_UINT32 mode = 0;
    //ZXIC_UINT32 port_id = 0;
    /*ZXIC_UINT32 flow_id_index = 0;*/
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 sys_cgavd_td = 0;
    DPP_TM_CNT_MODE_T cfgmt_count_mode = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_tm_asic_init_info);
    
/*
    if (tm_type == DPP_ETM)
    {
        flow_id_index = DPP_ETM_Q_NUM;
    }
    else if (tm_type == DPP_FTM)
    {
        flow_id_index = DPP_FTM_Q_NUM;
    }
*/
    case_num = p_tm_asic_init_info->case_num;
    ZXIC_COMM_PRINT("case_num =:%d TM ASIC INIT START! <======\n", case_num);
    blk_size = p_tm_asic_init_info->blk_size;
    imem_omem = p_tm_asic_init_info->imem_omem;
    mode = p_tm_asic_init_info->mode;
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, mode, 0, 1);

    ZXIC_COMM_PRINT("======>dev_id:%d TM ASIC INIT START! <======\n", dev_id);

    /*开启tm时钟门控使能*/
    rc  = dpp_tm_cfgmt_clkgate_en_set(dev_id, 1);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_clkgate_en_set");
    ZXIC_COMM_PRINT("DPP tm clk enable ok\n");
    zxic_comm_sleep(5);
    /*开启tm软复位使能*/
    rc  = dpp_tm_cfgmt_softrst_en_set(dev_id, 1);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_softrst_en_set");
    ZXIC_COMM_PRINT("DPP tm softrst enable ok\n");
    zxic_comm_sleep(5);


    DPP_ETM_CFGMT_CLKGATE_EN_T timeout = {0};
    timeout.clkgate_en = 0xfff;
    rc  = dpp_reg_write(dev_id,
                        ETM_CFGMT_TIMEOUT_LIMITr,
                        0,
                        0,
                        &timeout);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");



    /* 整包和交织模式:TM模式配置成交织模式 */
    rc  = dpp_tm_qmu_pkt_blk_mode_set(dev_id, 0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_pkt_blk_mode_set");
    ZXIC_COMM_PRINT("DPP tm mode cfg ok\n");

    /* 配置block大小 */
    rc = dpp_tm_cfgmt_blk_size_set(dev_id, blk_size);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_blk_size_set");

    /* 启动包存储CRC使能 */
    rc = dpp_tm_cfgmt_crc_en_set(dev_id, 1);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_crc_en_set");

    /* 配置QMU的每chunk节点数：目前配置1chk=8 block */
    rc = dpp_tm_cfgmt_qmu_work_mode_set(dev_id, 0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_qmu_work_mode_set");
    ZXIC_COMM_PRINT("DPP tm qmu_work_mode_set ok\n");
    /* 计数模式配置 */
    cfgmt_count_mode.fc_count_mode = 1;  /* 翻转 */
    cfgmt_count_mode.count_rd_mode = 0;  /* 非读清 */
    cfgmt_count_mode.count_overflow_mode = 1; /* 允许溢出翻转 */
    rc = dpp_tm_cfgmt_cnt_mode_set(dev_id, &cfgmt_count_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_cnt_mode_set");

    /* 总缓存字节数=节点数量*节点大小ftm256k etm512k:配置总缓存90% */
    sys_cgavd_td = (512 * 1024) * blk_size;
    sys_cgavd_td = sys_cgavd_td / 1024;
    sys_cgavd_td = sys_cgavd_td / 100 * 90;
    rc = dpp_tm_cgavd_td_th_set(dev_id, SYS_LEVEL, 0, sys_cgavd_td);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_td_th_set");

    /* 纯片内模式 */
    dpp_tm_tmmu_imem_en_set(dev_id, 1);
    dpp_tm_cgavd_imem_omem_set(dev_id, 1, 0);
    dpp_tm_tmmu_ddr_force_rdy_set(dev_id, 0x3ff);

    /* QMU链表配置：保证16K配置 */
    rc = dpp_tm_qmu_init_set(dev_id, 0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_init_set");

    /* 空队列确保门限值配置 */
    rc = dpp_tm_qmu_crs_th2_set(dev_id, 0, 200);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_crs_th2_set");

    /*crs的e桶产生门限:每个队列可映射16种配置，
      映射表是cgavd的流队列WRED队列策略组*/
    for (index = 0 ; index < 16; index++)
    {
        rc = dpp_tm_qmu_crs_eir_th_set(dev_id, index, 0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_crs_eir_th_set");
    }

    /* CRS限速 */

    /* 普通老化使能和配置 */
    rc = dpp_tm_qmu_pkt_aging_set(dev_id, 0, 0x1ff, 0xff, 0, DPP_ETM_Q_NUM - 1, 1, 0xa);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_pkt_aging_set");

    /* CRDT SHAP配置 */
    rc  = dpp_tm_crdt_ram_init(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_crdt_ram_init");
    rc  = dpp_tm_shap_ram_init(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_shap_ram_init");
    /* CRDT 授权使能打开 */
    rc  = dpp_tm_crdt_credit_en_set(dev_id, 1);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_crdt_credit_en_set");
    ZXIC_COMM_PRINT("dev_id:%d DPP_TM crdt credit enable set success!!!\n", dev_id);

    /* CRDT开启E桶CRS过滤使能(不能打开,会导致延时大) */
    /*for (index = 0; index < flow_id_index; index++)
    {
        rc = dpp_tm_ind_write32(dev_id, MODULE_TM_CRDT, 0x600000 + index, 0);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_move_th_set");
    }*/

    /*打开flow db_token,配置成c+e*/
    rc  = dpp_tm_shape_flow_db_en_set(dev_id, 1, 0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_shape_flow_db_en_set");

    /*开启cgavd平均队列深度归零使能*/
    rc  = dpp_tm_cgavd_avg_qlen_return_zero_en_set(dev_id, 1);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_avg_qlen_return_zero_en_set");

    rc  = dpp_tm_qmu_qlist_cfgmt_ram_init_done_print(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_qlist_cfgmt_ram_init_done_print");

    /*开启tm时钟门控使能*/
    // rc  = dpp_tm_cfgmt_clkgate_en_set(dev_id, 1);
    // ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_clkgate_en_set");

    // /*开启tm软复位使能*/
    // rc  = dpp_tm_cfgmt_softrst_en_set(dev_id, 1);
    // ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_softrst_en_set");

    /* 子系统就绪判断 */
    rc  = dpp_tm_cfgmt_subsystem_rdy_check(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_subsystem_rdy_check");


    ZXIC_COMM_PRINT("=====>dev_id:%d TM ASIC INIT END! <======\n", dev_id);

    return DPP_OK;
}

/***********************************************************/
/** 配置TM模式下初始化代码
* @param   dev_id
* @param   tm_type   0-ETM,1-FTM
* @param   p_tm_init_info    配置TM模式下初始化信息包括以下
*              blk_size    配置qmu block大小：512B[default]/1024B
*              case_num    QMU初始化场景编号：0/1/2/3...
*              imem_omem;  0-片内外混合; 1-纯片内;2-纯片外
*              mode        0-TM; 1-SA
*
* @return
* @remark  无
* @see
* @author  szq      @date  2015/03/26
************************************************************/
DPP_STATUS dpp_tm_asic_init_diag(ZXIC_UINT32 dev_id, ZXIC_UINT32 blk_size, ZXIC_UINT32 case_num, ZXIC_UINT32 imem_omem, ZXIC_UINT32 mode)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_TM_ASIC_INIT_INFO_T tm_asic_init_info = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, blk_size, 256, 1024);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, case_num, 0, 0x99F);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, imem_omem, 0, 2);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, mode, 0, 1);

    tm_asic_init_info.blk_size = blk_size;
    tm_asic_init_info.case_num = case_num;
    tm_asic_init_info.imem_omem = imem_omem;
    tm_asic_init_info.mode = mode;
    rc = dpp_tm_asic_init(dev_id, &tm_asic_init_info);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_asic_init");

    return DPP_OK;
}

#endif
/****************************************************************************
* 函数名称: dpp_tm_avg_que_len_get
* 功能描述: 各级平均队列深度获取
* 输入参数: dev_id: 设备索引编号
* @param   tm_type   0-ETM,1-FTM
*           cgavd_level: 拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
*           que_id: 本级别层次内的队列编号。
* 输出参数: p_avg_len: 平均队列深度，单位为BLOCK。
* 返 回 值: DPP_OK-成功，DPP_ERR-失败
* 其它说明:
* author  cy      @date  2015/06/29
*****************************************************************************/
DPP_STATUS dpp_tm_avg_que_len_get(ZXIC_UINT32 dev_id,
                                  DPP_TM_CGAVD_LEVEL_E cgavd_level,
                                  ZXIC_UINT32 que_id,
                                  ZXIC_UINT32 *p_avg_len)
{
    /* 返回值变量定义 */
    DPP_STATUS  rc = DPP_OK;

    /* 结构体变量定义 */
    DPP_ETM_CGAVD_PP_AVG_Q_LEN_T cgavd_pp_avg_q_len = {0};
    DPP_ETM_CGAVD_SYS_AVG_Q_LEN_T cgavd_sys_avg_q_len = {0};
    DPP_ETM_CGAVD_FLOW_AVG_Q_LEN_T cgavd_flow_avg_q_len = {0};


    /* 入参检查 */
    
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, cgavd_level, QUEUE_LEVEL, SYS_LEVEL);

    if (QUEUE_LEVEL == cgavd_level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, que_id, 0, DPP_ETM_Q_NUM - 1);
    }
    else if (PP_LEVEL == cgavd_level)
    {
        ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, que_id, 0, DPP_TM_PP_NUM - 1);
    }
    else
    {
  
    }

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_avg_len);

    switch (cgavd_level)
    {
        case QUEUE_LEVEL:
        {
            rc = dpp_reg_read(dev_id, ETM_CGAVD_FLOW_AVG_Q_LENr, 0, que_id, &cgavd_flow_avg_q_len);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
            *p_avg_len = cgavd_flow_avg_q_len.flow_avg_q_len;
            break;
        }

        case PP_LEVEL:
        {
            rc = dpp_reg_read(dev_id, ETM_CGAVD_PP_AVG_Q_LENr, 0, que_id, &cgavd_pp_avg_q_len);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
            *p_avg_len = cgavd_pp_avg_q_len.pp_avg_q_len;
            break;
        }

        case SYS_LEVEL:
        {
            rc = dpp_reg_read(dev_id, ETM_CGAVD_SYS_AVG_Q_LENr, 0, 0, &cgavd_sys_avg_q_len);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_read");
            *p_avg_len = cgavd_sys_avg_q_len.sys_avg_q_len;
            break;
        }

        default:
        {
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "[dev_id %d] cgavd_level is out of dev_id!!!\n", dev_id);
            return DPP_ERR;
        }
    }

    return DPP_OK;
}

#endif
/***********************************************************/
/** 清除整形表格里面的值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/16
************************************************************/
DPP_STATUS dpp_tm_clr_shape_para(DPP_DEV_T *dev)
{
    /*DPP_STATUS  rc = DPP_OK;*/

/*    rc = dpp_tm_global_var_mutex_init();
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_global_var_mutex_init");

    rc = zxic_comm_mutex_lock(&g_dpp_tm_global_var_rw_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_lock");*/

    // ZXIC_COMM_CHECK_POINT(dev);
    // ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 2);

    memset(g_dpp_etm_shape_para_table, 0, sizeof(g_dpp_etm_shape_para_table));


/*    rc = zxic_comm_mutex_unlock(&g_dpp_tm_global_var_rw_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_unlock");*/
    return DPP_OK;

}

#if 0
/***********************************************************/
/** 配置CFGMT中断屏蔽
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   shap_int_mask_flag  shap模块中断屏蔽位  0:不屏蔽   1:屏蔽
* @param   crdt_int_mask_flag  crdt模块中断屏蔽位
* @param   mmu_int_mask_flag   mmu模块中断屏蔽位
* @param   qmu_int_mask_flag  qmu模块中断屏蔽位
* @param   cgavd_int_mask_flag  cgavd模块中断屏蔽位
* @param   olif_int_mask_flag  olif模块中断屏蔽位
* @param   cfgmt_int_buf_mask_flag  cfgmt模块中断屏蔽位
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/08
************************************************************/
DPP_STATUS dpp_tm_cfgmt_int_mask_set_diag(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 shap_int_mask_flag,
                                          ZXIC_UINT32 crdt_int_mask_flag,
                                          ZXIC_UINT32 mmu_int_mask_flag,
                                          ZXIC_UINT32 qmu_int_mask_flag,
                                          ZXIC_UINT32 cgavd_int_mask_flag,
                                          ZXIC_UINT32 olif_int_mask_flag,
                                          ZXIC_UINT32 cfgmt_int_buf_mask_flag)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CFGMT_REG_INT_MASK_REG_T int_mask = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, shap_int_mask_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, crdt_int_mask_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, mmu_int_mask_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, qmu_int_mask_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, cgavd_int_mask_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, olif_int_mask_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, cfgmt_int_buf_mask_flag, 0, 1);

    int_mask.shap_int_mask      = shap_int_mask_flag;
    int_mask.crdt_int_mask      = crdt_int_mask_flag;
    int_mask.tmmu_int_mask       = mmu_int_mask_flag;
    int_mask.qmu_int_mask       = qmu_int_mask_flag;
    int_mask.cgavd_int_mask     = cgavd_int_mask_flag;
    int_mask.olif_int_mask      = olif_int_mask_flag;
    int_mask.cfgmt_int_buf_mask = cfgmt_int_buf_mask_flag;

    rc  = dpp_reg_write(dev_id,
                        ETM_CFGMT_REG_INT_MASK_REGr,
                        0,
                        0,
                        &int_mask);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 读取CFGMT中断状态
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_para   中断状态
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/08
************************************************************/
DPP_STATUS dpp_tm_cfgmt_int_state_get_diag(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;

    rc = dpp_reg_fields_print_with_def(dev_id, ETM_CFGMT_REG_INT_STATE_REGr, 0, 0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_fields_print_with_def");

    return DPP_OK;
}

/***********************************************************/
/** 读取olif中断状态
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/08
************************************************************/
DPP_STATUS dpp_tm_olif_int_state_get(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;

    rc = dpp_reg_fields_print_with_def(dev_id, ETM_OLIF_ITMHRAM_PARITY_ERR_2_INTr, 0, 0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_fields_print_with_def");


    return DPP_OK;
}


/***********************************************************/
/** 配置OLIF中断屏蔽
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   crcram_parity_err_mask    0:不屏蔽   1:屏蔽
* @param   itmhram_parity_err_mask
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/08
************************************************************/
DPP_STATUS dpp_tm_olif_int_mask_set(ZXIC_UINT32 dev_id,
                                    ZXIC_UINT32 olif_int_mask_flag)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_OLIF_OLIF_INT_MASK_T int_mask = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, olif_int_mask_flag, 0, 1);

    int_mask.crcram_parity_err_mask = olif_int_mask_flag;
    int_mask.emem_fifo_ecc_mask = olif_int_mask_flag;
    int_mask.emem_fifo_ovf_mask = olif_int_mask_flag;
    int_mask.emem_fifo_udf_mask = olif_int_mask_flag;
    int_mask.imem_fifo_ecc_mask = olif_int_mask_flag;
    int_mask.imem_fifo_ovf_mask = olif_int_mask_flag;
    int_mask.imem_fifo_udf_mask = olif_int_mask_flag;
    int_mask.itmh_ecc_double_err_mask = olif_int_mask_flag;
    int_mask.itmh_ecc_single_err_mask = olif_int_mask_flag;
    int_mask.order_fifo_ovf_mask = olif_int_mask_flag;
    int_mask.order_fifo_parity_err_mask = olif_int_mask_flag;
    int_mask.order_fifo_udf_mask = olif_int_mask_flag;
    int_mask.para_fifo_ecc_mask = olif_int_mask_flag;
    int_mask.para_fifo_ovf_mask = olif_int_mask_flag;
    int_mask.para_fifo_udf_mask = olif_int_mask_flag;

    rc  = dpp_reg_write(dev_id,
                        ETM_OLIF_OLIF_INT_MASKr,
                        0,
                        0,
                        &int_mask);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/** 配置cgavd中断屏蔽
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   cgavd_int_mask_flag    0:不屏蔽   1:屏蔽
* @param   写1 屏蔽cgavd中断寄存器和cgavd_ram_err寄存器所有位
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/08
************************************************************/
DPP_STATUS dpp_tm_cgavd_int_mask_set(ZXIC_UINT32 dev_id,
                                     ZXIC_UINT32 cgavd_int_mask_flag)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CGAVD_CGAVD_INT_MASK_T cgavd_int_mask = {0};
    DPP_ETM_CGAVD_CGAVD_RAM_ERR_INT_MASK_T cgavd_ram_err_int_mask = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, cgavd_int_mask_flag, 0, 1);


    cgavd_int_mask.cgavd_int_mask = cgavd_int_mask_flag;
    cgavd_ram_err_int_mask.flow_qlen_inta_mask = cgavd_int_mask_flag;
    cgavd_ram_err_int_mask.flow_qlen_intb_mask = cgavd_int_mask_flag;
    cgavd_ram_err_int_mask.flow_qnum_inta_mask = cgavd_int_mask_flag;
    cgavd_ram_err_int_mask.flow_qnum_intb_mask = cgavd_int_mask_flag;
    cgavd_ram_err_int_mask.flow_tdth_inta_mask = cgavd_int_mask_flag;
    cgavd_ram_err_int_mask.flow_tdth_intb_mask = cgavd_int_mask_flag;

    cgavd_ram_err_int_mask.pds_deal_fifo_ov_int_mask = cgavd_int_mask_flag;
    cgavd_ram_err_int_mask.pds_deal_fifo_uv_int_mask = cgavd_int_mask_flag;
    cgavd_ram_err_int_mask.pp_qlen_inta_mask = cgavd_int_mask_flag;
    cgavd_ram_err_int_mask.pp_qlen_intb_mask = cgavd_int_mask_flag;
    cgavd_ram_err_int_mask.pp_tdth_int_mask = cgavd_int_mask_flag;
    cgavd_ram_err_int_mask.qmu_cgavd_fifo_ov_int_mask = cgavd_int_mask_flag;
    cgavd_ram_err_int_mask.qmu_cgavd_fifo_uv_int_mask = cgavd_int_mask_flag;

    rc  = dpp_reg_write(dev_id,
                        ETM_CGAVD_CGAVD_INT_MASKr,
                        0,
                        0,
                        &cgavd_int_mask);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    rc  = dpp_reg_write(dev_id,
                        ETM_CGAVD_CGAVD_RAM_ERR_INT_MASKr,
                        0,
                        0,
                        &cgavd_ram_err_int_mask);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 读取cgavd中断状态
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_para   中断状态
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/08
************************************************************/
DPP_STATUS dpp_tm_cgavd_int_state_get(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;


    rc = dpp_reg_fields_print_with_def(dev_id, ETM_CGAVD_CGAVD_INTr, 0, 0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_fields_print_with_def");

    rc = dpp_reg_fields_print_with_def(dev_id, ETM_CGAVD_CGAVD_RAM_ERRr, 0, 0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_fields_print_with_def");

    return DPP_OK;
}

/***********************************************************/
/** 读取tmmu中断状态
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_para   中断状态
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/08
************************************************************/
DPP_STATUS dpp_tm_tmmu_int_state_get(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;

    rc = dpp_reg_fields_print_with_def(dev_id, ETM_TMMU_TMMU_STATES_1r, 0, 0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_fields_print_with_def");


    return DPP_OK;
}


/***********************************************************/
/** 配置crdt中断屏蔽
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   crdt_int_mask_flag    0:不屏蔽   1:屏蔽
* @param   写1   屏蔽ETM_CGAVD_RD_CPU_OR_RAMr
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/4/10
************************************************************/
DPP_STATUS dpp_tm_crdt_int_mask_set(ZXIC_UINT32 dev_id,
                                    ZXIC_UINT32 crdt_int_mask_flag)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CRDT_CRDT_INT_MASK_T crdt_int_mask = {0};


    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, crdt_int_mask_flag, 0, 1);


    crdt_int_mask.crdt_int_mask = crdt_int_mask_flag;
    rc  = dpp_reg_write(dev_id,
                        ETM_CRDT_CRDT_INT_MASKr,
                        0,
                        0,
                        &crdt_int_mask);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");


    return DPP_OK;
}

/***********************************************************/
/** 读取crdt中断状态
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/4/10
************************************************************/
DPP_STATUS dpp_tm_crdt_int_state_get(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;


    rc = dpp_reg_fields_print_with_def(dev_id, ETM_CRDT_CRDT_INT_BUSr, 0, 0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_fields_print_with_def");

    return DPP_OK;
}

/***********************************************************/
/** 配置qmu中断屏蔽
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   qmu_int_mask_flag    0:不屏蔽   1:屏蔽
* @param   写1  屏蔽qmu_int_mask5的所有位
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/09
************************************************************/
DPP_STATUS dpp_tm_qmu_int_mask_set(ZXIC_UINT32 dev_id,
                                   ZXIC_UINT32 qmu_int_mask_flag)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 qmu_qsch_crbal_init_mask_reg_index = 0;
    ZXIC_UINT32 qmu_int_mask1_reg_index = 0;
    ZXIC_UINT32 qmu_int_mask2_reg_index = 0;
    ZXIC_UINT32 qmu_int_mask3_reg_index = 0;
    ZXIC_UINT32 qmu_int_mask4_reg_index = 0;
    ZXIC_UINT32 qmu_int_mask5_reg_index = 0;

    DPP_ETM_QMU_QCFG_QSCH_CRBAL_INIT_MASK_T qmu_qsch_crbal_init_mask = {0};
    DPP_ETM_QMU_QMU_INT_MASK1_T qmu_int_mask1 = {0};
    DPP_ETM_QMU_QMU_INT_MASK2_T qmu_int_mask2 = {0};
    DPP_ETM_QMU_QMU_INT_MASK3_T qmu_int_mask3 = {0};
    DPP_ETM_QMU_QMU_INT_MASK4_T qmu_int_mask4 = {0};
    DPP_ETM_QMU_QMU_INT_MASK5_T qmu_int_mask5 = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, qmu_int_mask_flag, 0, 1);

    qmu_qsch_crbal_init_mask_reg_index = ETM_QMU_QCFG_QSCH_CRBAL_INIT_MASKr;
    qmu_int_mask1_reg_index = ETM_QMU_QMU_INT_MASK1r;
    qmu_int_mask2_reg_index = ETM_QMU_QMU_INT_MASK2r;
    qmu_int_mask3_reg_index = ETM_QMU_QMU_INT_MASK3r;
    qmu_int_mask4_reg_index = ETM_QMU_QMU_INT_MASK4r;
    qmu_int_mask5_reg_index = ETM_QMU_QMU_INT_MASK5r;

    qmu_qsch_crbal_init_mask.qcfg_qsch_crbal_init_mask = qmu_int_mask_flag;

    qmu_int_mask1.qmu_int_mask1 = qmu_int_mask_flag;
    qmu_int_mask2.qmu_int_mask2 = qmu_int_mask_flag;
    qmu_int_mask3.qmu_int_mask3 = qmu_int_mask_flag;
    qmu_int_mask4.qmu_int_mask4 = qmu_int_mask_flag;
    qmu_int_mask5.qmu_int_mask5 = qmu_int_mask_flag;

    rc  = dpp_reg_write(dev_id,
                        qmu_qsch_crbal_init_mask_reg_index,
                        0,
                        0,
                        &qmu_qsch_crbal_init_mask);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    rc  = dpp_reg_write(dev_id,
                        qmu_int_mask1_reg_index,
                        0,
                        0,
                        &qmu_int_mask1);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    rc  = dpp_reg_write(dev_id,
                        qmu_int_mask2_reg_index,
                        0,
                        0,
                        &qmu_int_mask2);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    rc  = dpp_reg_write(dev_id,
                        qmu_int_mask3_reg_index,
                        0,
                        0,
                        &qmu_int_mask3);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    rc  = dpp_reg_write(dev_id,
                        qmu_int_mask4_reg_index,
                        0,
                        0,
                        &qmu_int_mask4);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");


    rc  = dpp_reg_write(dev_id,
                        qmu_int_mask5_reg_index,
                        0,
                        0,
                        &qmu_int_mask5);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 读取qmu中断状态
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/09
************************************************************/
DPP_STATUS dpp_tm_qmu_int_state_get(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;

    rc = dpp_reg_fields_print_with_def(dev_id, ETM_QMU_QLIST_CFGMT_FIFO_STATEr, 0, 0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_fields_print_with_def");
    rc = dpp_reg_fields_print_with_def(dev_id, ETM_QMU_CMD_SCH_CFGMT_FIFO_STATEr, 0, 0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_fields_print_with_def");

    return DPP_OK;
}

/***********************************************************/
/** 配置shape中断屏蔽
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   shape_int_mask_flag    0:不屏蔽   1:屏蔽
* @param   写1   屏蔽ETM_CGAVD_RD_CPU_OR_RAMr
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/4/10
************************************************************/
DPP_STATUS dpp_tm_shape_int_mask_set(ZXIC_UINT32 dev_id,
                                     ZXIC_UINT32 shape_int_mask_flag)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CRDT_SHAP_INT_MASK_REG_T shap_int_mask = {0};


    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, shape_int_mask_flag, 0, 1);

    shap_int_mask.pp_c_token_min_int_mask = shape_int_mask_flag;
    rc  = dpp_reg_write(dev_id,
                        ETM_CRDT_SHAP_INT_MASK_REGr,
                        0,
                        0,
                        &shap_int_mask);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_write");


    return DPP_OK;
}


/***********************************************************/
/** 读取shape中断状态
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/4/10
************************************************************/
DPP_STATUS dpp_tm_shape_int_state_get(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_ETM_CRDT_SHAP_INT_REG_T shap_int_reg_t = {0};

    rc = dpp_reg_fields_print_with_def(dev_id, ETM_CRDT_SHAP_INT_REGr, 0, 0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_reg_fields_print_with_def");


    ZXIC_COMM_PRINT("pp_c_token_min_int : %d\n", shap_int_reg_t.pp_c_token_min_int);

    return DPP_OK;
}


/***********************************************************/
/** 配置dpp tm所有模块中断屏蔽
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   int_mask_flag    0:不屏蔽   1:屏蔽
* @param
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/09
************************************************************/
DPP_STATUS dpp_tm_int_mask_set(ZXIC_UINT32 dev_id,
                               ZXIC_UINT32 int_mask_flag)
{
    DPP_STATUS  rc = DPP_OK;

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, int_mask_flag, 0, 1);
    

    rc = dpp_tm_cfgmt_int_mask_set_diag(dev_id,
                                        int_mask_flag,
                                        int_mask_flag,
                                        int_mask_flag,
                                        int_mask_flag,
                                        int_mask_flag,
                                        int_mask_flag,
                                        int_mask_flag);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_int_mask_set_diag");

    rc = dpp_tm_olif_int_mask_set(dev_id, int_mask_flag);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_olif_int_mask_set");

    rc = dpp_tm_cgavd_int_mask_set(dev_id, int_mask_flag);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_int_mask_set");

    rc = dpp_tm_shape_int_mask_set(dev_id, int_mask_flag);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_shape_int_mask_set");

    rc = dpp_tm_crdt_int_mask_set(dev_id, int_mask_flag);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_crdt_int_mask_set");

    rc = dpp_tm_qmu_int_mask_set(dev_id, int_mask_flag);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_int_mask_set");


    return DPP_OK;
}

/***********************************************************/
/** 打印dpp tm所有模块中断状态
* @param   tm_type   0-ETM,1-FTM
* @param   dev_id   设备编号
* @param
* @param
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/09
************************************************************/
DPP_STATUS diag_dpp_tm_int(ZXIC_UINT32 dev_id)
{
    DPP_STATUS  rc = DPP_OK;

    ZXIC_COMM_PRINT("**************************************\n");
    ZXIC_COMM_PRINT("dpp tm cfgmt int state\n");
    ZXIC_COMM_PRINT("**************************************\n");
    rc = dpp_tm_cfgmt_int_state_get_diag(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cfgmt_int_state_get_diag");

    ZXIC_COMM_PRINT("**************************************\n");
    ZXIC_COMM_PRINT("dpp tm olif int state\n");
    ZXIC_COMM_PRINT("**************************************\n");
    rc = dpp_tm_olif_int_state_get(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_olif_int_state_get");

    ZXIC_COMM_PRINT("**************************************\n");
    ZXIC_COMM_PRINT("dpp tm cgavd int state\n");
    ZXIC_COMM_PRINT("**************************************\n");
    rc = dpp_tm_cgavd_int_state_get(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_cgavd_int_state_get");

    ZXIC_COMM_PRINT("**************************************\n");
    ZXIC_COMM_PRINT("dpp tm tmmu int state\n");
    ZXIC_COMM_PRINT("**************************************\n");
    rc = dpp_tm_tmmu_int_state_get(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_tmmu_int_state_get");

    ZXIC_COMM_PRINT("**************************************\n");
    ZXIC_COMM_PRINT("dpp tm shap int state\n");
    ZXIC_COMM_PRINT("**************************************\n");
    rc = dpp_tm_shape_int_state_get(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_shape_int_state_get");

    ZXIC_COMM_PRINT("**************************************\n");
    ZXIC_COMM_PRINT("dpp tm crdt int state\n");
    ZXIC_COMM_PRINT("**************************************\n");
    rc = dpp_tm_crdt_int_state_get(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_crdt_int_state_get");

    ZXIC_COMM_PRINT("**************************************\n");
    ZXIC_COMM_PRINT("dpp tm qmu int state\n");
    ZXIC_COMM_PRINT("**************************************\n");
    rc = dpp_tm_qmu_int_state_get(dev_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_int_state_get");

    ZXIC_COMM_PRINT("**************************************\n");
    ZXIC_COMM_PRINT("dpp tm qmu int case_no\n");
    ZXIC_COMM_PRINT("**************************************\n");
    ZXIC_COMM_PRINT("g_qmu_init_case_no = 0x%x\n", g_qmu_init_case_no);

    return DPP_OK;
}


/***********************************************************/
/** 获取tm.c中qmu_init_set中配置的case_num
* @param   tm_type   0-ETM,1-FTM
* @param   dev_id   设备编号
* @param
* @param
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/04/15
************************************************************/
DPP_STATUS dpp_tm_case_no_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *case_no)
{
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, case_no);

    *case_no = g_qmu_init_case_no;

    return DPP_OK;
}

/***********************************************************/
/** 配置tm授权价值总接口,包含qmu授权价值、shap授权价值、FTM对应授权价值版本设置
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   credit_value   授权价值，默认值是0x5feByte
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/08/13
************************************************************/
DPP_STATUS dpp_tm_credit_value_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 credit_value)
{
    DPP_STATUS  rc = DPP_OK;

    rc = dpp_tm_qmu_credit_value_set(dev_id, credit_value);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_qmu_credit_value_set");

    rc = dpp_tm_shap_crd_grain_set(dev_id, credit_value);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_tm_shap_crd_grain_set");

    return DPP_OK;
}

#endif
#endif

#if ZXIC_REAL("TM_CPU_SOFT_RESET")
#if 0
/***********************************************************/
/**  设置TM的全局变量，shape_para只保存profile被使用的数量，整形相关参数从寄存器中重新读取
* @param   dev_id
* @param   size         data_buff的长度
* @param   p_data_buff  需要恢复的内容
*
* @return
* @remark  无
* @see
* @author  XXX      @date  2017/03/09
************************************************************/
DPP_STATUS dpp_tm_glb_mgr_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 size, ZXIC_UINT8 *p_data_buff)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 shape_table_profile_num = 128;
    static ZXIC_UINT32 etm_shape_num[22][128] = {{0}};
    ZXIC_UINT32 total_para_id = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    DPP_TM_SHAPE_PARA_TABLE shap_para_tbl_t = {0};

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 2);
    
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_data_buff);

    rc = zxic_comm_mutex_lock(&g_dpp_tm_global_var_rw_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_lock");

    if (size != (22 * 128 * sizeof(ZXIC_UINT32)))
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "date is not complete.size[%d] err!!\n", size);
        rc = zxic_comm_mutex_unlock(&g_dpp_tm_global_var_rw_mutex);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_unlock");

        return DPP_ERR;
    }

    ZXIC_COMM_MEMCPY(etm_shape_num, p_data_buff, 22 * 128 * sizeof(ZXIC_UINT32));


    /* ETM: set flow and se profile to cpu mem */
    for (i = 0; i < 22; i++)
    {
        for (j = 0; j < shape_table_profile_num; j++)
        {
            total_para_id = i * shape_table_profile_num + j;

            rc = dpp_tm_shape_para_get(dev_id, total_para_id, &shap_para_tbl_t);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(dev_id, rc, "dpp_tm_shape_para_get", &g_dpp_tm_global_var_rw_mutex);

            g_dpp_etm_shape_para_table[dev_id][i][j].shape_num = etm_shape_num[i][j];
            g_dpp_etm_shape_para_table[dev_id][i][j].shape_cbs = shap_para_tbl_t.shape_cbs;
            g_dpp_etm_shape_para_table[dev_id][i][j].shape_cir = shap_para_tbl_t.shape_cir;

        }
    }


    rc = zxic_comm_mutex_unlock(&g_dpp_tm_global_var_rw_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_unlock");

    return DPP_OK;

}

/***********************************************************/
/** 获取TM的全局变量，shape_para只保存profile被使用的数量，整形相关参数从寄存器中重新读取
* @param   dev_id
* @param   p_flag           上层释放data_buff的标志，1:需要上层free,0:不需要上层free
* @param   p_size           data_buff的长度
* @param   pp_data_buff     二级指针(指向函数内部malloc空间的地址)
*
* @return
* @remark  无
* @see
* @author  XXX      @date  2017/03/09
************************************************************/
DPP_STATUS dpp_tm_glb_mgr_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_flag, ZXIC_UINT32 *p_size, ZXIC_UINT8 **pp_data_buff)
{
    ZXIC_UINT32 size = 0;
    static ZXIC_UINT32 etm_shape_num[22][128] = {{0}};
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;

    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 2);
    
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_flag);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_size);

    size = 22 * 128 * sizeof(ZXIC_UINT32);
    *pp_data_buff = ZXIC_COMM_MALLOC(size);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, *pp_data_buff);

    for (i = 0; i < 22; i++)
    {
        for (j = 0; j < 128; j++)
        {
            etm_shape_num[i][j] = g_dpp_etm_shape_para_table[dev_id][i][j].shape_num;
        }

    }

    ZXIC_COMM_MEMCPY(*pp_data_buff, etm_shape_num, 22 * 128 * sizeof(ZXIC_UINT32));


    *p_flag = 1;
    *p_size = size;

    return DPP_OK;
}

/***********************************************************/
/** 获取TM的全局变量大小，shape_para只保存profile被使用的数量，整形相关参数从寄存器中重新读取
* @param   dev_id
* @param   p_size         data_buff的长度
*
* @return
* @remark  无
* @see
* @author  XXX      @date  2017/03/09
************************************************************/
DPP_STATUS dpp_tm_glb_size_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_size)
{
    
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_size);

    *p_size = 22 * 128 * sizeof(ZXIC_UINT32);

    return DPP_OK;
}

#endif
#endif













