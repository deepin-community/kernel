/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_ppu.c
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 完成日期 : 2014/03/18
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
#include "dpp_dev.h"
#include "dpp_ppu_api.h"
#include "dpp_ppu.h"
#include "dpp_ppu4k_reg.h"
#include "dpp_agent_channel.h"

#define OPR_WRITE                (0)
#define OPR_READ                 (1)

DPP_PPU_CLS_BITMAP_T g_ppu_cls_bit_map[DPP_DEV_CHANNEL_MAX];

#if ZXIC_REAL("INIT")

ZXIC_UINT32 dpp_ppu_cls_use_set(ZXIC_UINT32 dev_id,
                           ZXIC_UINT32 cluster_id,
                           ZXIC_UINT32 flag)
{
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, cluster_id, 0, DPP_PPU_CLUSTER_NUM - 1);
    g_ppu_cls_bit_map[dev_id].cls_use[cluster_id] = flag;

    return DPP_OK;
}

ZXIC_UINT32 dpp_ppu_cls_use_get(ZXIC_UINT32 dev_id,
                           ZXIC_UINT32 cluster_id)
{
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, cluster_id, 0, DPP_PPU_CLUSTER_NUM - 1);

    return g_ppu_cls_bit_map[dev_id].cls_use[cluster_id];
}

ZXIC_UINT32 dpp_ppu_instr_mem_set(ZXIC_UINT32 dev_id,
                             ZXIC_UINT32 mem_id,
                             ZXIC_UINT32 flag)
{
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, mem_id, 0, PPU_INSTR_MEM_NUM - 1);
    g_ppu_cls_bit_map[dev_id].instr_mem[mem_id] = flag;

    return DPP_OK;
}

#define DPP_PPU_CLS_USE_CHECK(dev_id,cls_id) \
    do{ \
       if (!dpp_ppu_cls_use_get(dev_id, cls_id))\
         {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:cluster %d stop] !\n",__FILE__,__LINE__,cls_id);\
            ZXIC_COMM_ASSERT(0);\
            return DPP_ERR;\
         }\
    }while (0)

ZXIC_UINT32 dpp_ppu_parse_cls_bitmap(ZXIC_UINT32 dev_id,
                                ZXIC_UINT32 bitmap)
{
    ZXIC_UINT32 cls_id = 0;
    ZXIC_UINT32 mem_id = 0;

    ZXIC_UINT32 cls_use = 0;
    ZXIC_UINT32 instr_mem = 0;

    /*cluster 使用标记必须保证至少有一个cluster是打开的使用的*/
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, bitmap, 0, DPP_PPU_CLS_ALL_START);

    for (cls_id = 0; cls_id < DPP_PPU_CLUSTER_NUM; cls_id++)
    {
        cls_use = (bitmap >> cls_id) & 0x1;

        dpp_ppu_cls_use_set(dev_id, cls_id, cls_use);
    }

    for (mem_id = 0; mem_id < PPU_INSTR_MEM_NUM; mem_id++)
    {
        instr_mem = (bitmap >> (mem_id * 2)) & 0x3;

        dpp_ppu_instr_mem_set(dev_id, mem_id, ((instr_mem > 0) ? 1 : 0));
    }

    return DPP_OK;
}

#endif

#if ZXIC_REAL("TABEL_CFG")
/***********************************************************/
/** 配置SDT表
* @param   dev_id   设备号，范围0~3
* @param   cluster_id  me cluster编号，范围0~7
* @param   index       地址，即sdt表号，范围0~255
* @param   p_sdt_data  sdt表数据
*
* @return
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_ppu_sdt_tbl_write(DPP_DEV_T *dev, ZXIC_UINT32 cluster_id, ZXIC_UINT32 index, DPP_SDT_TBL_DATA_T *p_sdt_data)
{
    DPP_STATUS rtn = DPP_OK;
    DPP_PPU4K_CLUSTER_WR_HIGH_DATA_R_MEX_T high_data = {0};
    DPP_PPU4K_CLUSTER_WR_LOW_DATA_R_MEX_T  low_data  = {0};
    DPP_PPU4K_CLUSTER_ADDR_R_MEX_T         sdt_cmd   = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), index, PPU_SDT_IDX_MIN, PPU_SDT_IDX_MAX);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_sdt_data);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), cluster_id, 0, DPP_PPU_CLUSTER_NUM - 1);
    DPP_PPU_CLS_USE_CHECK(DEV_ID(dev), cluster_id);

    /* write data reg*/
    high_data.wr_high_data_r_mex = p_sdt_data->data_high32;
    low_data.wr_low_data_r_mex   = p_sdt_data->data_low32;
    rtn = dpp_reg_write(dev,
                        PPU4K_CLUSTER_WR_HIGH_DATA_R_MEXr,
                        cluster_id,
                        0,
                        &high_data);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_reg_write");

    rtn = dpp_reg_write(dev,
                         PPU4K_CLUSTER_WR_LOW_DATA_R_MEXr,
                         cluster_id,
                         0,
                         &low_data);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_reg_write");

    /* write cmd reg*/
    sdt_cmd.operate_type = OPR_WRITE;
    sdt_cmd.addr_r_mex   = index;

    rtn = dpp_reg_write(dev,
                        PPU4K_CLUSTER_ADDR_R_MEXr,
                        cluster_id,
                        0,
                        &sdt_cmd);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_reg_write");

    return DPP_OK;
}
#endif

#if ZXIC_REAL("PPU_STATICS")

/***********************************************************/
/**配置协处理器哈希计算密钥
* @param   dev_id     设备号
* @param   DPP_PPU_PPU_COP_THASH_RSK_T 
*
* @return
* @remark  无
* @see
* @author  yangmy      @date  2022/09/09
************************************************************/
DPP_STATUS dpp_ppu_ppu_cop_thash_rsk_set(DPP_DEV_T *dev, DPP_PPU_PPU_COP_THASH_RSK_T *p_para)
{
    DPP_STATUS  rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_para);

    ZXIC_COMM_TRACE_NOTICE("dpp_ppu_ppu_cop_thash_rsk_set start\n");

    rc = dpp_agent_channel_ppu_thash_rsk(dev, DPP_PPU_THASH_RSK_WR, p_para);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_agent_channel_ppu_thash_rsk");

    ZXIC_COMM_TRACE_NOTICE("dpp_ppu_ppu_cop_thash_rsk_set end\n");

    return DPP_OK;
}

DPP_STATUS dpp_ppu_ppu_cop_thash_rsk_get(DPP_DEV_T *dev, DPP_PPU_PPU_COP_THASH_RSK_T *p_ppu_cop_thash_rsk)
{
    DPP_STATUS rtn = DPP_OK;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_ppu_cop_thash_rsk);

    ZXIC_COMM_TRACE_NOTICE("dpp_ppu_ppu_cop_thash_rsk_get start\n");

    rtn = dpp_agent_channel_ppu_thash_rsk(dev, DPP_PPU_THASH_RSK_RD, p_ppu_cop_thash_rsk);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_agent_channel_ppu_thash_rsk");

    ZXIC_COMM_TRACE_NOTICE("dpp_ppu_ppu_cop_thash_rsk_get end\n");

    return DPP_OK;
}

#endif
DPP_STATUS dpp_ppu_debug_en_set(DPP_DEV_T* dev, ZXIC_UINT32 enable)
{
    DPP_STATUS rtn = DPP_OK;
    DPP_PPU_PPU_PPU_DEBUG_EN_R_T debug_en = {0};

    debug_en.debug_en_r = enable;

    rtn = dpp_reg_write(dev,
                        PPU_PPU_PPU_DEBUG_EN_Rr,
                        0,
                        0,
                        &debug_en);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_reg_write");

    return DPP_OK;
}

DPP_STATUS dpp_pktrx_port_en_set(DPP_DEV_T* dev, ZXIC_UINT32 port_no, ZXIC_UINT32 flag)
{
    DPP_STATUS  rc = 0;
    ZXIC_UINT32 port_en_index = 0;
    ZXIC_UINT32 port_en_mask = 0;
    DPP_NPPU_PKTRX_CFG_PORT_EN_3_T port_en_3_reg = {0};

    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), port_no, 0, DPP_PHYPORT_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flag, 0, 1);

    /* 由于端口使能配置cpu可能跟微码冲突，需要先写port_en_mask，再写en */
    port_en_mask = 1u << (port_no % 32);
    port_en_index = flag << (port_no % 32);
    
    rc = dpp_reg_write(dev,  NPPU_PKTRX_CFG_CPU_PORT_EN_MASKr, 0, 0, &port_en_mask);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    /* 由于有en_mask的存在，不用先读后写，直接写入逻辑只更新mask为1的，不影响原值 */
    if (port_no < 96)
    {
        rc = dpp_reg_write(dev,  NPPU_PKTRX_CFG_PORT_EN_0r + port_no / 32, 0, 0, &port_en_index);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");
    }
    /* port_en3寄存器需要先读后写，不完全看mask，只有端口使能相关的才看mask */
    else
    {
        rc = dpp_reg_read(dev,  NPPU_PKTRX_CFG_PORT_EN_3r, 0, 0, &port_en_3_reg);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

        port_en_3_reg.cfg_isch_port_en_3 = port_en_index;

        rc = dpp_reg_write(dev,  NPPU_PKTRX_CFG_PORT_EN_3r, 0, 0, &port_en_3_reg);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");
    }

    return DPP_OK;
}
DPP_STATUS dpp_ppu_debug_valid_get(DPP_DEV_T* dev,
                                   ZXIC_UINT32 *p_valid)
{
    DPP_STATUS rtn = DPP_OK;
    DPP_PPU_PPU_PPU_DEBUG_VLD_T debug_vld = {0};

    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_valid);

    rtn = dpp_reg_read(dev,
                       PPU_PPU_PPU_DEBUG_VLDr,
                       0,
                       0,
                       &debug_vld);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_reg_read");

    *p_valid = debug_vld.ppu_debug_vld;

    return DPP_OK;
}

DPP_STATUS dpp_ppu_set_debug_mode(DPP_PF_INFO_T* pf_info,ZXIC_UINT32 *dbg_status)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 pkt_empty = 0;
    ZXIC_UINT32 rd_count  = 0;
    DPP_DEV_T dev = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x register start.\n",
                                                   pf_info->slot, pf_info->vport);
    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    /* 使能调试模式寄存器 */
    rc = dpp_ppu_debug_en_set(&dev, 1);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(&dev), rc, "dpp_ppu_debug_en_set");

    rc = dpp_ppu_debug_valid_get(&dev, &pkt_empty);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(&dev), rc, "dpp_ppu_debug_valid_get");

    while (!pkt_empty)
    {
        rc = dpp_ppu_debug_valid_get(&dev, &pkt_empty);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(&dev), rc, "dpp_ppu_debug_valid_get");

        if (pkt_empty)
        {
            break;
        }

        if (rd_count > DPP_RD_CNT_MAX)
        {
            ZXIC_COMM_TRACE_NOTICE("debug start is fail!!!\n");
            *dbg_status = 0;
            rc = dpp_ppu_debug_en_set(&dev, 0);/*包未排空关闭调试使能*/
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(&dev), rc, "dpp_ppu_debug_en_set");
            return DPP_OK;
        }

        rd_count++;
        usleep_range(5, 10);
    }

    *dbg_status = 1;

    return DPP_OK;
}

EXPORT_SYMBOL(dpp_ppu_set_debug_mode);
DPP_STATUS dpp_ppu_close_debug_mode(DPP_PF_INFO_T* pf_info)
{
    DPP_STATUS rc = DPP_OK;
    DPP_DEV_T dev = {0};

    rc = dpp_dev_get(pf_info, &dev);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x debug mode disable\n",
                                                   pf_info->slot, pf_info->vport);
    dpp_ppu_debug_en_set(&dev,0);
    return DPP_OK;
}
EXPORT_SYMBOL(dpp_ppu_close_debug_mode);