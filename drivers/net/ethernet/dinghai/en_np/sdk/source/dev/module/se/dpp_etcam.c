/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_etcam.c
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/

#include "zxic_common.h"

#include "dpp_reg.h"
#include "dpp_se_api.h"
#include "dpp_etcam.h"
#include "dpp_dev.h"
#include "dpp_stat4k_reg.h"

#define DPP_ETCAM_OPR_WR            (1)
#define DPP_ETCAM_OPR_RD            (2)
#define DPP_ETCAM_OPR_UNLOAD        (3)
#define DPP_ETCAM_OPR_VBIT          (4)

#define TBLID_CFG_SETP          (8)
#define BADDR_CFG_SETP          (4)

/** 当前etcam 条目vld信息 */
DPP_ETCAM_ENTRY_VLD_T g_etcam_vld_info[DPP_DEV_CHANNEL_MAX][DPP_ETCAM_BLOCK_NUM][DPP_ETCAM_RAM_DEPTH] = {{{{0}}}};
#define GET_ETCAM_VLD_INFO(dev_id,block_id,block_index)     (g_etcam_vld_info[dev_id][block_id] + block_index)

#if ZXIC_REAL("IN_FUNC")
/***********************************************************/
/** 将用户输入的D/M格式的数据转换为写硬件需要的X/Y数据
* @param   p_dm
* @param   p_xy
* @param   len
*
* @return
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_etcam_dm_to_xy(DPP_ETCAM_ENTRY_T *p_dm,
                              DPP_ETCAM_ENTRY_T *p_xy,
                              ZXIC_UINT32 len)
{
    ZXIC_UINT32 i = 0;

    ZXIC_COMM_CHECK_POINT(p_dm);
    ZXIC_COMM_CHECK_POINT(p_xy);
    ZXIC_COMM_CHECK_INDEX(len, 0, DPP_ETCAM_WIDTH_MAX / 8);
    ZXIC_COMM_ASSERT(p_dm->p_data && p_dm->p_mask && p_xy->p_data && p_xy->p_mask);

    for (i = 0; i < len; i++)
    {
        p_xy->p_data[i] = ZXIC_COMM_DM_TO_X(p_dm->p_data[i], p_dm->p_mask[i]);
        p_xy->p_mask[i] = ZXIC_COMM_DM_TO_Y(p_dm->p_data[i], p_dm->p_mask[i]);
    }

    return DPP_OK;
}

/***********************************************************/
/** 将从硬件读到的X/Y格式的数据转换为用户需要的D/M数据
* @param   p_xy
* @param   p_dm
* @param   len
*
* @return
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_etcam_xy_to_dm(DPP_ETCAM_ENTRY_T *p_dm,
                              DPP_ETCAM_ENTRY_T *p_xy,
                              ZXIC_UINT32 len)
{
    ZXIC_UINT32 i = 0;

    ZXIC_COMM_CHECK_POINT(p_dm);
    ZXIC_COMM_CHECK_POINT(p_xy);
    ZXIC_COMM_CHECK_INDEX(len, 0, DPP_ETCAM_WIDTH_MAX / 8);
    ZXIC_COMM_ASSERT(p_dm->p_data && p_dm->p_mask && p_xy->p_data && p_xy->p_mask);

    for (i = 0; i < len; i++)
    {
        p_dm->p_data[i] = ZXIC_COMM_XY_TO_DATA(p_xy->p_data[i], p_xy->p_mask[i]); /* valid only when mask is 0 */
        p_dm->p_mask[i] = ZXIC_COMM_XY_TO_MASK(p_xy->p_data[i], p_xy->p_mask[i]);
    }

    return DPP_OK;
}

/***********************************************************/
/** 根据读写模式，获取需要操作的间接寄存器掩码
* @param   mask  block RAM操作位图，共8比特，每比特对应一个block RAM行的80bit
*
* @return
* @remark  无
* @see

************************************************************/
ZXIC_UINT32 dpp_etcam_ind_data_reg_opr_mask_get(ZXIC_UINT32 mask)
{
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 reg_mask = 0;

    ZXIC_COMM_CHECK_INDEX(mask, 0, 0xff);

    for (i = 0; i < DPP_ETCAM_RAM_NUM; i++)
    {
        if ((mask >> i) & 0x1)
        {
            reg_mask |= ((ZXIC_UINT32)0x7 << ((i / 2) * 5 + (i % 2) * 2));
        }
    }

    return reg_mask;
}

/***********************************************************/
/** 写eTcam表项数据
* @param   dev_id
* @param   wr_mask
* @param   p_data
*
* @return
* @remark  无
* @see

************************************************************/
DPP_STATUS dpp_etcam_ind_data_set(DPP_DEV_T *dev, ZXIC_UINT32 wr_mask, ZXIC_UINT8 *p_data)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 offset = 0;
    ZXIC_UINT32 reg_mask = 0;
    ZXIC_UINT8 *p_temp = NULL;
    ZXIC_UINT8  buff[DPP_ETCAM_WIDTH_MAX / 8] = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), wr_mask, 0, DPP_ETCAM_WR_MASK_MAX);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);

    p_temp = p_data;

    /* 160bit key: high 80bit in tcam_ram1, low 80bit in tcam_ram0, and so on. */
    for (i = 0; i < DPP_ETCAM_RAM_NUM; i++)
    {
        offset = i * ((ZXIC_UINT32)DPP_ETCAM_WIDTH_MIN / 8);

        if ((wr_mask >> ((DPP_ETCAM_RAM_NUM - 1 - i) % 32)) & 0x1)
        {
            ZXIC_COMM_MEMCPY(buff + offset, p_temp, DPP_ETCAM_WIDTH_MIN / 8);
            p_temp += DPP_ETCAM_WIDTH_MIN / 8;
        }
    }

    zxic_comm_swap(buff, DPP_ETCAM_WIDTH_MAX / 8);

    /* get ind data reg operate mask, 20bit */
    reg_mask = dpp_etcam_ind_data_reg_opr_mask_get(wr_mask);

    /* cpu_ind_wdat0 reg is for lowest 32bit data. */
    for (i = 0; i < (DPP_ETCAM_WIDTH_MAX / 32); i++)
    {
        if ((reg_mask >> (DPP_ETCAM_WIDTH_MAX / 32 - 1 - i)) & 0x1)
        {
            rc = dpp_reg_write(dev,
                               STAT_ETCAM_CPU_IND_WDAT19r - i,
                               0,
                               0,
                               (buff + i * sizeof(ZXIC_UINT32)));
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");
        }
    }

    return DPP_OK;
}

/***********************************************************/
/** 写eTcam间接命令寄存器
* @param   dev_id
* @param   addr            etcam地址(0-511)
* @param   block_idx       block索引(0-15)
* @param   data_or_mask    1：写X(data), 0:写Y(mask)
* @param   wr_mask         写入掩码, 最高8bit, 对应bit为1代表对应的80bit数据
* @param   opr_type        1-write, 2-read, 3-unload
* @param   tacm_reg_flag   1：配置内部row_col_mask寄存器 还是0：读写tcam
* @param   row_mask_flag   1: write row_mask reg, 0: write col_mask reg
* @param   vben            enable the valid bit addressed by addr
* @param   vbit            valid bit input
*
* @return
* @remark  无
* @see

************************************************************/
DPP_STATUS dpp_etcam_ind_cmd_set(DPP_DEV_T *dev,
                                 ZXIC_UINT32 addr,
                                 ZXIC_UINT32 block_idx,
                                 ZXIC_UINT32 data_or_mask,
                                 ZXIC_UINT32 wr_mask,
                                 ZXIC_UINT32 opr_type,
                                 ZXIC_UINT32 tacm_reg_flag,
                                 ZXIC_UINT32 row_mask_flag,
                                 ZXIC_UINT32 vben,
                                 ZXIC_UINT32 vbit)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_ETCAM_CPU_IND_CTRL_TMP0_T ind_cmd = {0};
    DPP_STAT_ETCAM_CPU_IND_CTRL_TMP1_T ind_cmd_1 = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), addr, 0, DPP_ETCAM_RAM_DEPTH - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), block_idx, 0, DPP_ETCAM_BLOCK_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), data_or_mask, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), wr_mask, 0, DPP_ETCAM_WR_MASK_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), opr_type, 1, 4);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), tacm_reg_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), row_mask_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), vben, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), vbit, 0, 0xff);

    /* 一定要先配置 配置寄存器1,再配置 配置寄存器0 */
    ind_cmd_1.row_or_col_msk = row_mask_flag;
    ind_cmd_1.vben = vben;
    ind_cmd_1.vbit = vbit;

    rc = dpp_reg_write(dev,
                       STAT_ETCAM_CPU_IND_CTRL_TMP1r,
                       0,
                       0,
                       &ind_cmd_1);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    /* opr_type: 1-write, 2-read, 3-unload */
    switch (opr_type)
    {
        case DPP_ETCAM_OPR_RD:
        {
            ind_cmd.rd_wr = 1;
        }
        break;

        case DPP_ETCAM_OPR_WR:
        {
            ind_cmd.rd_wr = 0;
            ind_cmd.wr_mode = wr_mask;
        }
        break;

        case DPP_ETCAM_OPR_UNLOAD:
        {
            /* 掩码表，决定哪些条目删除,8bit */
            ind_cmd.flush = wr_mask;
        }
        break;

        case DPP_ETCAM_OPR_VBIT:
        {
            /* 获取该地址的vbit valid位状态 */
            ind_cmd.rd_wr = 1;
            ind_cmd.wr_mode = wr_mask;
        }
        break;

        default:
        {
            ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "Invalid opr_type!\n");
            ZXIC_COMM_ASSERT(0);
            return DPP_ERR;
        }
    }

    ind_cmd.dat_or_mask = data_or_mask;
    ind_cmd.ram_sel = block_idx;
    ind_cmd.addr = addr;

    ZXIC_COMM_TRACE_DEBUG("data_or_mask:%d\n", ind_cmd.dat_or_mask);
    ZXIC_COMM_TRACE_DEBUG("block_idx:%d\n", ind_cmd.ram_sel);
    ZXIC_COMM_TRACE_DEBUG("addr:0x%08x\n", ind_cmd.addr);

    rc = dpp_reg_write(dev,
                       STAT_ETCAM_CPU_IND_CTRL_TMP0r,
                       0,
                       0,
                       &ind_cmd);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

#endif

#if ZXIC_REAL("EX_FUNC")
/***********************************************************/
/** 获取etcam中每个block的cpu操作fifo将满信号，若将满则cpu不能再操作当前block
* @param   dev_id      设备号
* @param   block_idx   etcam中的block编号，范围0~15
* @param   p_cpu_afull
*
* @return
* @remark  无
* @see

************************************************************/
DPP_STATUS dpp_etcam_cpu_afull_get(DPP_DEV_T *dev, ZXIC_UINT32 block_idx, ZXIC_UINT32 *p_cpu_afull)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_ETCAM_ETCAM_CPU_FL_T cpu_fl = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), block_idx, 0, DPP_ETCAM_BLOCK_NUM - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_cpu_afull);

    rc = dpp_reg_read(dev, STAT_ETCAM_ETCAM_CPU_FLr, 0, 0, &cpu_fl);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    *p_cpu_afull = (cpu_fl.etcam_cpu_fl >> block_idx) & 0x1;

    return DPP_OK;
}

/***********************************************************/
/** 校验etcam中每个block的cpu操作fifo将满信号，若将满则cpu不能再操作当前block
* @param   dev_id      设备号
* @param   block_idx   etcam中的block编号，范围0~7
*
* @return
* @remark  无
* @see
* @author  wll      @date  2019/04/15
************************************************************/
DPP_STATUS dpp_etcam_cpu_afull_check(DPP_DEV_T *dev, ZXIC_UINT32 block_idx)
{
    ZXIC_UINT32 read_cnt = 0;
    ZXIC_UINT32 cpu_afull = 1;
    DPP_STATUS rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), block_idx, 0, DPP_ETCAM_BLOCK_NUM - 1);

    while (cpu_afull)
    {
        rc = dpp_etcam_cpu_afull_get(dev, block_idx, &cpu_afull);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_etcam_cpu_afull_get");

        if (!cpu_afull)
        {
            break;
        }

        read_cnt++;

        if (read_cnt > DPP_RD_CNT_MAX * DPP_RD_CNT_MAX)
        {
            ZXIC_COMM_TRACE_ERROR("Error!!! dpp_etcam_cpu_afull_check is overtime!\n");
            return DPP_ERR;
        }

        /* zxic_comm_usleep(100); */
    } 

    return DPP_OK;
}

/***********************************************************/
/** 添加eTcam表条目
* @param   dev_id     设备号
* @param   addr       每个block中的ram地址，位宽为8*80bit
* @param   block_idx  block编号，范围0~7
* @param   wr_mask    写表掩码，共8bit，每bit控制ram中对应位置的80bit数据是否有效
* @param   opr_type   etcam操作类型，详见 DPP_ETCAM_OPR_TYPE_E
* @param   p_entry    条目数据，data和mask
*
* @return
* @remark  无
* @see

************************************************************/
DPP_STATUS dpp_etcam_entry_add(DPP_DEV_T *dev,
                               ZXIC_UINT32 addr,
                               ZXIC_UINT32 block_idx,
                               ZXIC_UINT32 wr_mask,
                               ZXIC_UINT32 opr_type,
                               DPP_ETCAM_ENTRY_T *p_entry)
{
    DPP_STATUS rc = DPP_OK;
    // ZXIC_UINT32 i = 0;
    // ZXIC_UINT32 tbl_id = 0;
    // ZXIC_UINT32 handle_row = 0;
    // ZXIC_UINT32 handle = 0;
    // ZXIC_UINT32 basea_addr = 0;
    ZXIC_UINT8 temp_data[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_UINT8 temp_mask[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_MUTEX_T *p_etcam_mutex = NULL;
    DPP_ETCAM_ENTRY_T entry_xy = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), addr, 0, DPP_ETCAM_RAM_DEPTH - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), block_idx, 0, DPP_ETCAM_BLOCK_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), wr_mask, 0, DPP_ETCAM_WR_MASK_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), opr_type, DPP_ETCAM_OPR_DM, DPP_ETCAM_OPR_XY);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_entry);

    rc = dpp_dev_opr_mutex_get(dev, DPP_DEV_MUTEX_T_ETCAM, &p_etcam_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_etcam_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_lock");

    /* check cpu fifo is afull */
    rc = dpp_etcam_cpu_afull_check(dev, block_idx);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_etcam_cpu_afull_check", p_etcam_mutex);

    ZXIC_COMM_ASSERT(p_entry->p_data && p_entry->p_mask);

    entry_xy.p_data = temp_data;
    entry_xy.p_mask = temp_mask;

    if (opr_type == DPP_ETCAM_OPR_DM)
    {
        /* convert user D/M data to X/Y */
        rc = dpp_etcam_dm_to_xy(p_entry, &entry_xy, DPP_ETCAM_ENTRY_SIZE_GET(p_entry->mode));
        ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_etcam_dm_to_xy", p_etcam_mutex);
    }
    else
    {
        ZXIC_COMM_MEMCPY(entry_xy.p_data, p_entry->p_data, DPP_ETCAM_ENTRY_SIZE_GET(p_entry->mode));
        ZXIC_COMM_MEMCPY(entry_xy.p_mask, p_entry->p_mask, DPP_ETCAM_ENTRY_SIZE_GET(p_entry->mode));
    }

    /* write data X */
    rc = dpp_etcam_ind_data_set(dev, wr_mask, entry_xy.p_data);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_etcam_ind_data_set", p_etcam_mutex);

    rc = dpp_etcam_ind_cmd_set(dev,
                               addr,
                               block_idx,
                               DPP_ETCAM_DTYPE_DATA,
                               wr_mask,
                               DPP_ETCAM_OPR_WR,
                               0,
                               0,
                               1,
                               0);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_etcam_ind_cmd_set", p_etcam_mutex);

    /* write mask Y */
    rc = dpp_etcam_ind_data_set(dev, wr_mask, entry_xy.p_mask);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_etcam_ind_data_set", p_etcam_mutex);

    rc = dpp_etcam_ind_cmd_set(dev,
                               addr,
                               block_idx,
                               DPP_ETCAM_DTYPE_MASK,
                               wr_mask,
                               DPP_ETCAM_OPR_WR,
                               0,
                               0,
                               1,
                               0xFF);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_etcam_ind_cmd_set", p_etcam_mutex);

    rc = zxic_comm_mutex_unlock(p_etcam_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");

    return DPP_OK;
}

/***********************************************************/
/** 删除eTcam表项条目
* @param   dev_id     设备号
* @param   addr       每个block中的ram地址，位宽为8*80bit
* @param   block_idx  block的编号，范围0~7
* @param   wr_mask    写表掩码，共8bit，每bit控制ram中对应位置的80bit数据是否有效
*
* @return
* @remark  无
* @see

************************************************************/
DPP_STATUS dpp_etcam_entry_del(DPP_DEV_T *dev,
                               ZXIC_UINT32 addr,
                               ZXIC_UINT32 block_idx,
                               ZXIC_UINT32 wr_mask)
{
    DPP_STATUS rc = DPP_OK;
    // ZXIC_UINT32 i = 0;
    // ZXIC_UINT32 tbl_id = 0;
    // ZXIC_UINT32 handle_row = 0;
    // ZXIC_UINT32 handle = 0;
    // ZXIC_UINT32 basea_addr = 0;

    ZXIC_UINT8 temp_data[DPP_ETCAM_WIDTH_MAX / 8] = {0xff,};
    ZXIC_UINT8 temp_mask[DPP_ETCAM_WIDTH_MAX / 8] = {0,};
    ZXIC_MUTEX_T *p_etcam_mutex = NULL;
    DPP_ETCAM_ENTRY_T entry_xy = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), addr, 0, DPP_ETCAM_RAM_DEPTH - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), block_idx, 0, DPP_ETCAM_BLOCK_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), wr_mask, 0, DPP_ETCAM_WR_MASK_MAX);

    ZXIC_COMM_MEMSET(temp_data, 0xff, DPP_ETCAM_WIDTH_MAX / 8);
    ZXIC_COMM_MEMSET(temp_mask, 0, DPP_ETCAM_WIDTH_MAX / 8);

    entry_xy.p_data = temp_data;
    entry_xy.p_mask = temp_mask;

    rc = dpp_dev_opr_mutex_get(dev, DPP_DEV_MUTEX_T_ETCAM, &p_etcam_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_etcam_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_lock");

    /* check cpu fifo is afull */
    rc = dpp_etcam_cpu_afull_check(dev, block_idx);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_etcam_cpu_afull_check", p_etcam_mutex);

    /* write data X */
    rc = dpp_etcam_ind_data_set(dev, wr_mask, entry_xy.p_data);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_etcam_ind_data_set", p_etcam_mutex);

    rc = dpp_etcam_ind_cmd_set(dev,
                               addr,
                               block_idx,
                               DPP_ETCAM_DTYPE_DATA,
                               wr_mask,
                               DPP_ETCAM_OPR_WR,
                               0,
                               0,
                               1,
                               0xFF);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_etcam_ind_cmd_set", p_etcam_mutex);

    /* write mask Y */
    rc = dpp_etcam_ind_data_set(dev, wr_mask, entry_xy.p_mask);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_etcam_ind_data_set", p_etcam_mutex);

    rc = dpp_etcam_ind_cmd_set(dev,
                               addr,
                               block_idx,
                               DPP_ETCAM_DTYPE_MASK,
                               wr_mask,
                               DPP_ETCAM_OPR_WR,
                               0,
                               0,
                               1,
                               0xFF);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(dev), rc, "dpp_etcam_ind_cmd_set", p_etcam_mutex);

    rc = zxic_comm_mutex_unlock(p_etcam_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");

    return DPP_OK;
}

/***********************************************************/
/** 比较一组data/mask和一组X/Y数据内容是否相异
* @param   p_entry_dm   待比较的data/mask数据
* @param   p_entry_xy   待比较的一组X/Y组合数据
*
* @return  1-不同，0-相同
* @remark  无
* @see

************************************************************/
ZXIC_UINT32 dpp_etcam_entry_cmp(DPP_ETCAM_ENTRY_T *p_entry_dm, DPP_ETCAM_ENTRY_T *p_entry_xy)
{
    DPP_STATUS rc = 0;
    ZXIC_UINT32 data_len = 0;
    ZXIC_UINT8 temp_data[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_UINT8 temp_mask[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    DPP_ETCAM_ENTRY_T entry_xy_temp = {0};

    ZXIC_COMM_CHECK_POINT(p_entry_dm);
    ZXIC_COMM_CHECK_POINT(p_entry_xy);

    entry_xy_temp.mode = p_entry_dm->mode;
    entry_xy_temp.p_data = temp_data;
    entry_xy_temp.p_mask = temp_mask;
    data_len = DPP_ETCAM_ENTRY_SIZE_GET(entry_xy_temp.mode);

    if(data_len > 80)
    {
        return 1;
    }

    ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW_NO_ASSERT(3U, entry_xy_temp.mode);
    rc = dpp_etcam_dm_to_xy(p_entry_dm, &entry_xy_temp, data_len);
    ZXIC_COMM_CHECK_RC(rc, "dpp_etcam_dm_to_xy");

    if ((ZXIC_COMM_MEMCMP(entry_xy_temp.p_data, p_entry_xy->p_data, data_len) != 0) ||
        (ZXIC_COMM_MEMCMP(entry_xy_temp.p_mask, p_entry_xy->p_mask, data_len) != 0))
    {
        return 1;
    }

    return 0;
}

/***********************************************************/
/** 配置block的业务号，table_id
* @param   dev_id       设备号
* @param   block_idx    block的索引值。范围[0~7]
* @param   tbl_id       表号，范围[0~7]
*
* @return
* @remark  无
* @see

************************************************************/
DPP_STATUS dpp_etcam_block_tbl_id_set(DPP_DEV_T *dev,
                                      ZXIC_UINT32 block_idx,
                                      ZXIC_UINT32 tbl_id)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 reg_offset = 0;
    ZXIC_UINT32 bit_offset = 0;
    ZXIC_UINT32 *p_temp = 0;
    DPP_STAT4K_ETCAM_BLOCK0_7_PORT_ID_CFG_T block_tbl_id = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), block_idx, 0, DPP_ETCAM_BLOCK_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), tbl_id, 0, DPP_ETCAM_TBLID_NUM - 1);

    /*reg_offset 表示第几个寄存器进行配置，共四组配置寄存器*/
    reg_offset = block_idx / TBLID_CFG_SETP;
    /*bit_offset 一个寄存器可以配置四组port_id,本位表示寄存器内配置的偏移*/
    bit_offset = block_idx % TBLID_CFG_SETP;

    /*取出当前的配置*/
    rc = dpp_reg_read(dev,
                      STAT4K_ETCAM_BLOCK0_7_PORT_ID_CFGr + reg_offset,
                      0,
                      0,
                      &block_tbl_id);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");
    /*计算出来配置当前的port_id所处的地址*/
    p_temp = (ZXIC_UINT32 *)(&block_tbl_id) + 7 - bit_offset;

    *p_temp = tbl_id;

    rc = dpp_reg_write(dev,
                       STAT4K_ETCAM_BLOCK0_7_PORT_ID_CFGr + reg_offset,
                       0,
                       0,
                       &block_tbl_id);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 获取block的业务号，table_id
* @param   dev_id
* @param   block_idx
* @param   p_tbl_id
*
* @return
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_etcam_block_tbl_id_get(DPP_DEV_T *dev,
                                      ZXIC_UINT32 block_idx,
                                      ZXIC_UINT32 *p_tbl_id)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 reg_offset = 0;
    ZXIC_UINT32 bit_offset = 0;
    ZXIC_UINT32 *p_temp = 0;
    DPP_STAT4K_ETCAM_BLOCK0_7_PORT_ID_CFG_T block_tbl_id = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), block_idx, 0, DPP_ETCAM_BLOCK_NUM - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_tbl_id);

    reg_offset = block_idx / TBLID_CFG_SETP;
    bit_offset = block_idx % TBLID_CFG_SETP;

    rc = dpp_reg_read(dev,
                      STAT4K_ETCAM_BLOCK0_7_PORT_ID_CFGr + reg_offset,
                      0,
                      0,
                      &block_tbl_id);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    p_temp = ((ZXIC_UINT32 *)(&block_tbl_id)) + (TBLID_CFG_SETP - 1 - bit_offset);

    *p_tbl_id = *p_temp;

    return DPP_OK;
}


/***********************************************************/
/** 配置block的基地址。
* @param   dev_id      设备号
* @param   block_idx   block编号
* @param   base_addr   基地址。
*          配置规则:每个寄存器寄存器位宽是7bit，逻辑实现为{base_addr[6:0],9'b0},
*                   80 bit键值模式下，base_addr格式遵循{base_addr[6,3],3'0}
*                   160bit键值模式下，base_addr格式遵循{1'b0,base_addr[5,2],2'0}
*                   320bit键值模式下，base_addr格式遵循{2'b0,base_addr[4,1],1'0}
*                   640bit键值模式下，base_addr格式遵循{3'b0,base_addr[3,0]}
* @return
* @remark  无
* @see     共有8组配置寄存器，每组寄存器支持两组block基地址的配置
*          其中第0组支持0和1号block基地址配置，第1组支持1和2号block基地址配置 ，依次类推
************************************************************/
DPP_STATUS dpp_etcam_block_baddr_set(DPP_DEV_T *dev,
                                     ZXIC_UINT32 block_idx,
                                     ZXIC_UINT32 base_addr)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 reg_offset = 0;
    ZXIC_UINT32 bit_offset = 0;
    ZXIC_UINT32 *p_temp = 0;
    DPP_STAT4K_ETCAM_BLOCK0_3_BASE_ADDR_CFG_T block_baddr = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), block_idx, 0, DPP_ETCAM_BLOCK_NUM - 1);

    reg_offset = block_idx / BADDR_CFG_SETP;
    bit_offset = block_idx % BADDR_CFG_SETP;

    rc = dpp_reg_read(dev,
                      STAT4K_ETCAM_BLOCK0_3_BASE_ADDR_CFGr + reg_offset,
                      0,
                      0,
                      &block_baddr);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    p_temp = (ZXIC_UINT32 *)(&block_baddr) + 3 - bit_offset;

    *p_temp = base_addr;

    rc = dpp_reg_write(dev,
                       STAT4K_ETCAM_BLOCK0_3_BASE_ADDR_CFGr + reg_offset,
                       0,
                       0,
                       &block_baddr);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 获取block的基地址
* @param   dev_id
* @param   block_idx
* @param   p_base_addr
*
* @return
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_etcam_block_baddr_get(DPP_DEV_T *dev,
                                     ZXIC_UINT32 block_idx,
                                     ZXIC_UINT32 *p_base_addr)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 reg_offset = 0;
    ZXIC_UINT32 bit_offset = 0;
    ZXIC_UINT32 *p_temp = 0;
    DPP_STAT4K_ETCAM_BLOCK0_3_BASE_ADDR_CFG_T block_baddr = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), block_idx, 0, DPP_ETCAM_BLOCK_NUM - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_base_addr);

    reg_offset = block_idx / BADDR_CFG_SETP;
    bit_offset = block_idx % BADDR_CFG_SETP;

    rc = dpp_reg_read(dev,
                      STAT4K_ETCAM_BLOCK0_3_BASE_ADDR_CFGr + reg_offset,
                      0,
                      0,
                      &block_baddr);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    p_temp = ((ZXIC_UINT32 *)(&block_baddr)) + (BADDR_CFG_SETP - 1 - bit_offset);

    *p_base_addr = *p_temp;

    return DPP_OK;
}

#endif