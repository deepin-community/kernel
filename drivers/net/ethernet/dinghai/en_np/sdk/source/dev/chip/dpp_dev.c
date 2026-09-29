/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_dev.c
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1: 代码规范性修改
* 修改日期:  2014/02/10
* 版 本 号:
* 修 改 人:  丁金凤
* 修改内容:
***************************************************************/

#include "zxic_common.h"
#include "dpp_module.h"
#include "dpp_pci.h"
#include "dpp_dev.h"
#include "dpp_type_api.h"
#include "dh_cmd.h"
#include "dpp_dtb_table.h"
#include "dpp_np_init.h"

static DPP_DEV_MGR_T g_dev_mgr = {0};

#define DPP_DEV_INFO_GET(id) (g_dev_mgr.p_dev_array[id])

/***********************************************************/
/** 初始化设备管理模块
* @param   ZXIC_VOID
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_dev_init(ZXIC_VOID)
{
    if (g_dev_mgr.is_init)
    {
        ZXIC_COMM_TRACE_ERROR("Dev is already initialized.\n");
        return DPP_OK;
    }

    g_dev_mgr.device_num = 0;
    g_dev_mgr.is_init    = 1;

    return DPP_OK;
}

/***********************************************************/
/**
* @return
* @remark  无
* @see
* @author  XXX      @date  2014/02/14
************************************************************/
DPP_DEV_MGR_T *dpp_dev_mgr_get(ZXIC_VOID)
{
    if (!g_dev_mgr.is_init)
    {
        ZXIC_COMM_TRACE_ERROR("Error: dev_mgr is not init.\n");
        ZXIC_COMM_ASSERT(0);
        return NULL;
    }

    return &g_dev_mgr;
}

/***********************************************************/
/** 添加新设备实例，并初始化
* @param   dev_id         新增设备的设备号
* @param   dev_type          设备类型，取值参照DPP_DEV_TYPE_E的定义
* @param   access_type       设备访问类型，取值参照DPP_DEV_ACCESS_TYPE_E的定义
* @param   pcie_addr         PCIe映射地址
* @param   riscv_addr         RISCV映射地址
* @param   dma_vir_addr      DMA映射地址
* @param   dma_phy_addr      DMA内存物理地址
* @param   p_pcie_write_fun  PCIe硬件写回调函数
* @param   p_pcie_read_fun   PCIe硬件读回调函数
* @param   p_riscv_write_fun  RISCV硬件写回调函数
* @param   p_riscv_read_fun   RISCV硬件读回调函数
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_dev_add(ZXIC_UINT32  dev_id,
                       DPP_DEV_TYPE_E dev_type,
                       DPP_DEV_ACCESS_TYPE_E  access_type,
                       ZXIC_ADDR_T  pcie_addr,
                       ZXIC_ADDR_T  riscv_addr,
                       ZXIC_ADDR_T  dma_vir_addr,
                       ZXIC_ADDR_T  dma_phy_addr,
                       DPP_DEV_WRITE_FUNC p_pcie_write_fun,
                       DPP_DEV_READ_FUNC  p_pcie_read_fun,
                       DPP_DEV_WRITE_FUNC p_riscv_write_fun,
                       DPP_DEV_READ_FUNC  p_riscv_read_fun)
{
    DPP_STATUS rtn = DPP_OK;
    DPP_DEV_CFG_T *p_dev_info = NULL;
    DPP_DEV_MGR_T *p_dev_mgr  = NULL;
    ZXIC_UINT32     i = 0;
    ZXIC_UINT32     slot = 0;

    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id,DPP_DEV_CHANNEL_MAX - 1);

    p_dev_mgr = dpp_dev_mgr_get();
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_mgr);
    if (!p_dev_mgr->is_init)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ErrorCode[ 0x%x]: Device Manager is not init!!!\n",
                                 DPP_RC_DEV_MGR_NOT_INIT);
        return DPP_RC_DEV_MGR_NOT_INIT;
    }

    if (p_dev_mgr->p_dev_array[dev_id] != NULL)
    {
        /* device is already exist. */
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "Device is added again!!!\n");
        p_dev_info = p_dev_mgr->p_dev_array[dev_id];
    }
    else
    {
        /* device is new. */
        p_dev_info = (DPP_DEV_CFG_T *)ZXIC_COMM_MALLOC(sizeof(DPP_DEV_CFG_T));
        ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_info);
        ZXIC_COMM_MEMSET_S(p_dev_info,sizeof(DPP_DEV_CFG_T),0x0,sizeof(DPP_DEV_CFG_T));
        p_dev_mgr->p_dev_array[dev_id] = p_dev_info;
        p_dev_mgr->device_num++;
    }

    p_dev_info->device_id   = dev_id;
    p_dev_info->dev_type    = dev_type;
    p_dev_info->access_type = access_type;
    p_dev_info->pcie_addr   = pcie_addr;
    p_dev_info->riscv_addr   = riscv_addr;
    p_dev_info->dma_vir_addr = dma_vir_addr;
    p_dev_info->dma_phy_addr = dma_phy_addr;
    p_dev_info->p_riscv_write_fun = NULL;
    p_dev_info->p_riscv_read_fun  = NULL;
    p_dev_info->p_pcie_write_fun = dpp_dev_pcie_default_write;
    p_dev_info->p_pcie_read_fun  = dpp_dev_pcie_default_read;
    ZXIC_COMM_MEMSET_S(p_dev_info->bar_msg_num,sizeof(p_dev_info->bar_msg_num),0xff,sizeof(p_dev_info->bar_msg_num));

    if(p_riscv_write_fun)
    {
        p_dev_info->p_riscv_write_fun  = p_riscv_write_fun;
    }

    if(p_riscv_read_fun)
    {
        p_dev_info->p_riscv_read_fun  = p_riscv_read_fun;
    }

    if (p_pcie_write_fun)
    {
        p_dev_info->p_pcie_write_fun = p_pcie_write_fun;
    }

    if (p_pcie_read_fun)
    {
        p_dev_info->p_pcie_read_fun  = p_pcie_read_fun;
    }

    ZXIC_COMM_MEMSET(p_dev_info->pcie_channel, 0x00, sizeof(p_dev_info->pcie_channel));

    rtn = zxic_comm_mutex_create(&p_dev_info->reg_opr_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");

    rtn = zxic_comm_mutex_create(&p_dev_info->oam_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");

    rtn = zxic_comm_mutex_create(&p_dev_info->etm_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");

    rtn = zxic_comm_mutex_create(&p_dev_info->ddr_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");

    rtn = zxic_comm_mutex_create(&p_dev_info->ind_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");

    rtn = zxic_comm_mutex_create(&p_dev_info->etcam_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");

    rtn = zxic_comm_mutex_create(&p_dev_info->car0_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");

    rtn = zxic_comm_mutex_create(&p_dev_info->alg_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");

    rtn = zxic_comm_mutex_create(&p_dev_info->nppu_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");

    rtn = zxic_comm_mutex_create(&p_dev_info->smmu0_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");

    rtn = zxic_comm_mutex_create(&p_dev_info->smmu1_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");

    rtn = zxic_comm_mutex_create(&p_dev_info->etm_2nd_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");

    rtn = zxic_comm_mutex_create(&p_dev_info->lpm_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");

    rtn = zxic_comm_mutex_create(&p_dev_info->crm_temp_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");

    rtn = zxic_comm_mutex_create(&p_dev_info->sim_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");

    rtn = zxic_comm_mutex_create(&p_dev_info->dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");

    rtn = zxic_comm_mutex_create(&p_dev_info->pktrx_mf_glb_cfg_mutex_0);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");

    rtn = zxic_comm_mutex_create(&p_dev_info->pktrx_mf_glb_cfg_mutex_1);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");

    rtn = zxic_comm_mutex_create(&p_dev_info->pktrx_mf_glb_cfg_mutex_2);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");

    rtn = zxic_comm_mutex_create(&p_dev_info->pktrx_mf_glb_cfg_mutex_3);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");

    for (i = 0; i < DPP_DTB_QUEUE_MAX; i++)
    {
        rtn = zxic_comm_mutex_create(&p_dev_info->dtb_rb_mutex[i]);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");
    }

    for (i = 0; i < DPP_DTB_QUEUE_MAX; i++)
    {
        rtn = zxic_comm_mutex_create(&p_dev_info->dtb_queue_mutex[i]);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");
    }

    for(slot = 0; slot < DPP_PCIE_SLOT_MAX; slot++)
    {
        for (i = 0; i < DPP_DTB_QUEUE_MAX; i++)
        {
            rtn = zxic_comm_spin_create(&p_dev_info->dtb_queue_spin_lock[slot][i]);
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_spin_create");
        }
        
    }

    rtn = zxic_comm_mutex_create(&p_dev_info->self_recover_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");


    return DPP_OK;
}

/***********************************************************/
/** 删除设备实例
* @param   dev_id  设备号
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_dev_del(ZXIC_UINT32 dev_id)
{
    DPP_STATUS rtn = DPP_OK;
    DPP_DEV_CFG_T *p_dev_info = NULL;
    DPP_DEV_MGR_T *p_dev_mgr  = NULL;
    ZXIC_UINT32     i = 0;

    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id,DPP_DEV_CHANNEL_MAX - 1);

    p_dev_mgr = dpp_dev_mgr_get();
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_mgr);
    p_dev_info = p_dev_mgr->p_dev_array[dev_id];

    if (p_dev_info != NULL)
    {
        rtn = zxic_comm_mutex_destroy(&p_dev_info->reg_opr_mutex);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_destroy");

        rtn = zxic_comm_mutex_destroy(&p_dev_info->oam_mutex);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_destroy");

        rtn = zxic_comm_mutex_destroy(&p_dev_info->etm_mutex);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_destroy");

        rtn = zxic_comm_mutex_destroy(&p_dev_info->ddr_mutex);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_destroy");

        rtn = zxic_comm_mutex_destroy(&p_dev_info->ind_mutex);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_destroy");

        rtn = zxic_comm_mutex_destroy(&p_dev_info->etcam_mutex);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_destroy");

        rtn = zxic_comm_mutex_destroy(&p_dev_info->car0_mutex);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_destroy");

        rtn = zxic_comm_mutex_destroy(&p_dev_info->alg_mutex);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_destroy");

        rtn = zxic_comm_mutex_destroy(&p_dev_info->nppu_mutex);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_destroy");

        rtn = zxic_comm_mutex_destroy(&p_dev_info->smmu0_mutex);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_destroy");

        rtn = zxic_comm_mutex_destroy(&p_dev_info->smmu1_mutex);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_destroy");

        rtn = zxic_comm_mutex_destroy(&p_dev_info->etm_2nd_mutex);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_destroy");

        rtn = zxic_comm_mutex_destroy(&p_dev_info->lpm_mutex);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_destroy");

        rtn = zxic_comm_mutex_destroy(&p_dev_info->crm_temp_mutex);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_destroy");


        rtn = zxic_comm_mutex_destroy(&p_dev_info->dtb_mutex);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_destroy");

        rtn = zxic_comm_mutex_destroy(&p_dev_info->pktrx_mf_glb_cfg_mutex_0);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_destroy");

        rtn = zxic_comm_mutex_destroy(&p_dev_info->pktrx_mf_glb_cfg_mutex_1);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_destroy");

        rtn = zxic_comm_mutex_destroy(&p_dev_info->pktrx_mf_glb_cfg_mutex_2);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_destroy");

        rtn = zxic_comm_mutex_destroy(&p_dev_info->pktrx_mf_glb_cfg_mutex_3);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_destroy");

        for (i = 0; i < DPP_DTB_QUEUE_MAX; i++)
        {
            rtn = zxic_comm_mutex_destroy(&p_dev_info->dtb_queue_mutex[i]);
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");
        }

        for (i = 0; i < DPP_DTB_QUEUE_MAX; i++)
        {
            rtn = zxic_comm_mutex_destroy(&p_dev_info->dtb_rb_mutex[i]);
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_create");
        }

        rtn = zxic_comm_mutex_destroy(&p_dev_info->self_recover_mutex);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "zxic_comm_mutex_destroy");


        ZXIC_COMM_FREE(p_dev_info);
        p_dev_mgr->p_dev_array[dev_id] = NULL;
        ZXIC_COMM_CHECK_DEV_INDEX_SUB_OVERFLOW_NO_ASSERT(dev_id, p_dev_mgr->device_num, 1);
        p_dev_mgr->device_num--;
    }

    return DPP_OK;
}

/***********************************************************/
/** 获取 DPP_DEV_T
* @param   dev_id
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_dev_get(DPP_PF_INFO_T* pf_info, DPP_DEV_T *dev)
{
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT16 slot = 0;
    ZXIC_UINT16 channel_id = 0;

    DPP_DEV_CFG_T *p_dev_info = NULL;
    DPP_DEV_MGR_T *p_dev_mgr  = NULL;

    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, pf_info);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, dev);

    slot = pf_info->slot;
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, slot, 0, DPP_PCIE_SLOT_MAX - 1);

    channel_id = DPP_PCIE_CHANNEL_ID(pf_info->vport);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, channel_id, 0, DPP_PCIE_CHANNEL_MAX - 1);

    p_dev_mgr = dpp_dev_mgr_get();
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_mgr);
    if (!p_dev_mgr->is_init)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ErrorCode[ 0x%x]: Device Manager is not init!!!\n",
                                                                     DPP_RC_DEV_MGR_NOT_INIT);
        return DPP_RC_DEV_MGR_NOT_INIT;
    }
    p_dev_info = p_dev_mgr->p_dev_array[dev_id];
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_info);

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_info->pcie_channel[slot][channel_id].device);

    dev->device_id = p_dev_info->device_id;
    dev->pcie_channel.is_used = p_dev_info->pcie_channel[slot][channel_id].is_used;
    dev->pcie_channel.slot = p_dev_info->pcie_channel[slot][channel_id].slot;
    dev->pcie_channel.vport = p_dev_info->pcie_channel[slot][channel_id].vport;
    dev->pcie_channel.pcie_id = p_dev_info->pcie_channel[slot][channel_id].pcie_id;
    dev->pcie_channel.device = p_dev_info->pcie_channel[slot][channel_id].device;
    dev->pcie_channel.base_addr = p_dev_info->pcie_channel[slot][channel_id].base_addr;
    dev->pcie_channel.offset_addr = p_dev_info->pcie_channel[slot][channel_id].offset_addr;
    dev->pcie_channel.bar_msg_num = p_dev_info->bar_msg_num[slot];
    dev->pcie_channel.dev_status = p_dev_info->pcie_channel[slot][channel_id].dev_status;

    return DPP_OK;
}

/***********************************************************/
/** 校验当前dev是否为当前slot最后一个有效的pf
* @param   dev
* @param   last_flag   出参
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cq      @date  2024/08/01
************************************************************/
DPP_STATUS dpp_dev_last_check(DPP_DEV_T *dev, ZXIC_UINT32 *last_flag)
{
    ZXIC_UINT32 slot = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 used_num = 0;
    ZXIC_UINT32 channel_id = 0;
    ZXIC_UINT32 vport = 0;
    ZXIC_UINT32 dev_id = 0;
    DPP_DEV_CFG_T *p_dev_info = NULL;
    DPP_DEV_MGR_T *p_dev_mgr  = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(last_flag);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id, DPP_DEV_CHANNEL_MAX - 1);

    p_dev_mgr = dpp_dev_mgr_get();
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_mgr);
    if (!p_dev_mgr->is_init)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ErrorCode[ 0x%x]: Device Manager is not init!!!\n",
                                                                     DPP_RC_DEV_MGR_NOT_INIT);
        return DPP_RC_DEV_MGR_NOT_INIT;
    }

    p_dev_info = p_dev_mgr->p_dev_array[dev_id];
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_info);

    vport = DEV_PCIE_VPORT(dev);
    channel_id = DPP_PCIE_CHANNEL_ID(vport);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, channel_id, 0, DPP_PCIE_CHANNEL_MAX - 1);

    slot = DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, slot, 0, DPP_PCIE_SLOT_MAX - 1);

    for(i=0;i<DPP_PCIE_CHANNEL_MAX;i++)
    {
        if(p_dev_info->pcie_channel[slot][i].is_used)
        {
            used_num++;
        }
    }

    if((used_num==1)&&p_dev_info->pcie_channel[slot][channel_id].is_used)
    {
        *last_flag = 1;
    }

    return DPP_OK;
}

/***********************************************************/
/** 
* @param   dev_id         设备的设备号
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_dev_pcie_channel_add(DPP_PF_INFO_T* pf_info, struct pci_dev* p_dev)
{
    ZXIC_UINT32 dev_id = 0;
    ZXIC_VOID* base_addr = 0;
    ZXIC_UINT8  type = 0;
    ZXIC_UINT32 post = 0;
    ZXIC_UINT16 pcie_id = 0;
    ZXIC_UINT16 slot = 0;
    ZXIC_UINT16 channel_id = 0;
    ZXIC_UINT32 dma_size = DTB_SDT_DUMP_SIZE;
    dma_addr_t dma_handle;
    ZXIC_VOID* cpu_addr = NULL;
    DPP_DEV_CFG_T *p_dev_info = NULL;
    DPP_DEV_MGR_T *p_dev_mgr  = NULL;

#ifdef DPP_FLOW_HW_INIT
    struct bar_offset_res res;
    struct bar_offset_params paras;
#endif

    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, pf_info);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev);

    slot = pf_info->slot;
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, slot, 0, DPP_PCIE_SLOT_MAX - 1);

    channel_id = DPP_PCIE_CHANNEL_ID(pf_info->vport);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, channel_id, 0, DPP_PCIE_CHANNEL_MAX - 1);

    p_dev_mgr = dpp_dev_mgr_get();
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_mgr);
    if (!p_dev_mgr->is_init)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ErrorCode[ 0x%x]: Device Manager is not init!!!\n",
                                                                     DPP_RC_DEV_MGR_NOT_INIT);
        return DPP_RC_DEV_MGR_NOT_INIT;
    }
    p_dev_info = p_dev_mgr->p_dev_array[dev_id];
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_info);

    if (p_dev_info->pcie_channel[slot][channel_id].device != NULL)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ErrorCode[ 0x%x]: pcie slot %u vport 0x%04x already init.\n",
                                                         DPP_RC_DEV_PARA_INVALID, slot, pf_info->vport);
        return DPP_RC_DEV_PARA_INVALID;
    }

    base_addr = ioremap(pci_resource_start(p_dev, 0), pci_resource_len(p_dev, 0));
    if (IS_ERR_OR_NULL(base_addr))
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ErrorCode[ 0x%x]: pcie slot %u vport 0x%04x ioremap failed.\n",
                                                           DPP_RC_DEV_PARA_INVALID, slot, pf_info->vport);
        return DPP_RC_DEV_PARA_INVALID;
    }

    for (post = pci_find_capability(p_dev, PCI_CAP_ID_VNDR);
         post > 0; post = pci_find_next_capability(p_dev, post, PCI_CAP_ID_VNDR))
    {
        pci_read_config_byte(p_dev, post + 3, &type);

        if (type == 5)
        {
            pci_read_config_word(p_dev, post + 6, &pcie_id);
        }
    }

    if (pcie_id == 0)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ErrorCode[ 0x%x]: pcie slot %u vport 0x%04x get pcieid failed.\n",
                                                              DPP_RC_DEV_PARA_INVALID, slot, pf_info->vport);
        return DPP_RC_DEV_PARA_INVALID;
    }

#ifdef DPP_FLOW_HW_INIT
    paras.type = URI_NP;
    paras.pcie_id = pcie_id;
    paras.virt_addr = ZXIC_COMM_PTR_TO_VAL(base_addr) + DEV_PCIE_MSG_OFFSET_ADDR;
    if (zxdh_get_bar_offset(&paras, &res) != BAR_MSG_OK)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ErrorCode[ 0x%x]: pcie slot %u vport 0x%04x get bar offset failed.\n",
                                                                  DPP_RC_DEV_PARA_INVALID, slot, pf_info->vport);
        return DPP_RC_DEV_PARA_INVALID;
    }
#endif
    cpu_addr = dma_alloc_coherent(&(p_dev->dev), dma_size, &dma_handle, GFP_KERNEL);
    if(!cpu_addr)
    {
        dev_err(&(p_dev->dev),"dma_alloc_coherent buff allocation failed\n");
        return DPP_RC_DEV_DMA_MEM_ALLOC_FAIL;
    }
    p_dev_info->pcie_channel[slot][channel_id].slot      = slot;
    p_dev_info->pcie_channel[slot][channel_id].vport     = pf_info->vport;
    p_dev_info->pcie_channel[slot][channel_id].pcie_id   = pcie_id;
    p_dev_info->pcie_channel[slot][channel_id].device    = p_dev;
    p_dev_info->pcie_channel[slot][channel_id].base_addr = ZXIC_COMM_PTR_TO_VAL(base_addr);
    p_dev_info->pcie_channel[slot][channel_id].dev_status = 0;
    p_dev_info->pcie_channel[slot][channel_id].dump_dma_size   = dma_size;
    p_dev_info->pcie_channel[slot][channel_id].dump_dma_phy_addr = (ZXIC_ADDR_T)dma_handle;
    p_dev_info->pcie_channel[slot][channel_id].dump_dma_vir_addr = (ZXIC_ADDR_T)(ZXIC_COMM_PTR_TO_VAL(cpu_addr));

#ifdef DPP_FLOW_HW_INIT
    p_dev_info->pcie_channel[slot][channel_id].offset_addr = res.bar_offset;
#else
    p_dev_info->pcie_channel[slot][channel_id].offset_addr = 0x6000;
#endif

    p_dev_info->pcie_channel[slot][channel_id].is_used = 1;

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x base_addr: 0x%llx success.\n",
                            slot, pf_info->vport, ZXIC_COMM_PTR_TO_VAL(base_addr));
    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x pcie_id: 0x%04x offset_addr: 0x%llx success.\n",
                     slot, pf_info->vport, pcie_id, p_dev_info->pcie_channel[slot][channel_id].offset_addr);

    return DPP_OK;
}

/***********************************************************/
/** 
* @param   dev_id         设备的设备号
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_dev_pcie_channel_del(DPP_PF_INFO_T* pf_info)
{
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT16 slot = 0;
    ZXIC_UINT16 channel_id = 0;
    dma_addr_t dma_handle = 0;
    ZXIC_VOID* cpu_addr = NULL;
    ZXIC_UINT32 dma_size      = 0;
    DPP_DEV_CFG_T *p_dev_info = NULL;
    DPP_DEV_MGR_T *p_dev_mgr  = NULL;
    struct pci_dev* p_dev     = NULL;

    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, pf_info);

    slot = pf_info->slot;
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, slot, 0, DPP_PCIE_SLOT_MAX - 1);

    channel_id = DPP_PCIE_CHANNEL_ID(pf_info->vport);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, channel_id, 0, DPP_PCIE_CHANNEL_MAX - 1);

    p_dev_mgr = dpp_dev_mgr_get();
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_mgr);
    if (!p_dev_mgr->is_init)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ErrorCode[ 0x%x]: Device Manager is not init!!!\n",
                                                                     DPP_RC_DEV_MGR_NOT_INIT);
        return DPP_RC_DEV_MGR_NOT_INIT;
    }
    p_dev_info = p_dev_mgr->p_dev_array[dev_id];
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_info);

    p_dev = p_dev_info->pcie_channel[slot][channel_id].device;
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev);

    dma_handle = (dma_addr_t)(p_dev_info->pcie_channel[slot][channel_id].dump_dma_phy_addr);
    cpu_addr = ZXIC_COMM_VAL_TO_PTR(p_dev_info->pcie_channel[slot][channel_id].dump_dma_vir_addr);
    dma_size = p_dev_info->pcie_channel[slot][channel_id].dump_dma_size;
    if(dma_handle)
    {
        dma_free_coherent(&(p_dev->dev), dma_size, cpu_addr, dma_handle);
    }

    iounmap((ZXIC_VOID*)p_dev_info->pcie_channel[slot][channel_id].base_addr);

    p_dev_info->pcie_channel[slot][channel_id].device = NULL;
    p_dev_info->pcie_channel[slot][channel_id].slot = 0;
    p_dev_info->pcie_channel[slot][channel_id].vport = 0;
    p_dev_info->pcie_channel[slot][channel_id].pcie_id = 0;
    p_dev_info->pcie_channel[slot][channel_id].base_addr = 0;
    p_dev_info->pcie_channel[slot][channel_id].offset_addr = 0;
    p_dev_info->pcie_channel[slot][channel_id].is_used = 0;
    p_dev_info->pcie_channel[slot][channel_id].bar_msg_num = 0xFFFFFFFF;
    p_dev_info->pcie_channel[slot][channel_id].dump_dma_size = 0;
    p_dev_info->pcie_channel[slot][channel_id].dump_dma_phy_addr = 0;
    p_dev_info->pcie_channel[slot][channel_id].dump_dma_vir_addr = 0;
    p_dev_info->pcie_channel[slot][channel_id].dev_status = 0;

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x success.\n", slot, pf_info->vport);

    return DPP_OK;
}

/***********************************************************/
/** dev status更新，该状态主要用于dtb是否具备使用条件的判断
* @param   dev     设备
* @param   dev_status   设备状态
* @return  
* @remark  无
* @see
* @author  cbb      @date  2026/6/23
************************************************************/
DPP_STATUS dpp_dev_status_update(DPP_DEV_T *dev, ZXIC_UINT32 dev_status)
{
    ZXIC_UINT16 slot = 0;
    ZXIC_UINT16 vport = 0;
    ZXIC_UINT16 channel_id = 0;
    ZXIC_UINT32 dev_id = 0;
    DPP_DEV_MGR_T *p_dev_mgr  = NULL;
    DPP_DEV_CFG_T *p_dev_info = NULL;

    ZXIC_COMM_CHECK_POINT(dev);

    slot = DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_INDEX(slot, 0, DPP_PCIE_SLOT_MAX - 1);

    vport = DEV_PCIE_VPORT(dev);
    channel_id = DPP_PCIE_CHANNEL_ID(vport);
    ZXIC_COMM_CHECK_INDEX(channel_id, 0, DPP_PCIE_CHANNEL_MAX - 1);

    p_dev_mgr = dpp_dev_mgr_get();
    ZXIC_COMM_CHECK_POINT(p_dev_mgr);
    if (!p_dev_mgr->is_init)
    {
        ZXIC_COMM_TRACE_ERROR("ErrorCode[ 0x%x]: Device Manager is not init!!!\n",
                                                                     DPP_RC_DEV_MGR_NOT_INIT);
        return DPP_RC_DEV_MGR_NOT_INIT;
    }
    p_dev_info = p_dev_mgr->p_dev_array[dev_id];
    ZXIC_COMM_CHECK_POINT(p_dev_info);

    ZXIC_COMM_CHECK_POINT(p_dev_info->pcie_channel[slot][channel_id].device);

    p_dev_info->pcie_channel[slot][channel_id].dev_status = dev_status;

    dev->pcie_channel.dev_status = dev_status;

    return DPP_OK;
}

/***********************************************************/
/** 获取存储流表资源信息的指针
* @param   dev     设备
* @param 
* @return  
* @remark  无
* @see
* @author  cq      @date  2024/11/13
************************************************************/
ZXIC_VOID *dpp_dev_get_se_res_ptr(DPP_DEV_T *dev)
{
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 slot_id = 0;
    DPP_DEV_MGR_T *p_dev_mgr = NULL;
    DPP_DEV_CFG_T *p_dev_info = NULL;

    ZXIC_COMM_CHECK_POINT_RETURN_NULL(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX_RETURN_NULL(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);

    p_dev_mgr = dpp_dev_mgr_get();
    ZXIC_COMM_CHECK_POINT_RETURN_NULL(p_dev_mgr);
    p_dev_info = p_dev_mgr->p_dev_array[dev_id];
    ZXIC_COMM_CHECK_POINT_RETURN_NULL(p_dev_info);

    slot_id = DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_RETURN_NULL(dev_id, slot_id, 0, DPP_PCIE_SLOT_MAX - 1);

    return p_dev_info->p_std_nic_res[slot_id];
}

/***********************************************************/
/** 设置存储流表资源信息的指针
* @param   dev     设备
* @param   se_ptr  
* @return  
* @remark  无
* @see
* @author  cq      @date  2024/11/13
************************************************************/
ZXIC_VOID dpp_dev_set_se_res_ptr(DPP_DEV_T *dev, ZXIC_VOID *se_ptr)
{
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 slot_id = 0;
    DPP_DEV_MGR_T *p_dev_mgr = NULL;
    DPP_DEV_CFG_T *p_dev_info = NULL;

    ZXIC_COMM_CHECK_POINT_RETURN_NONE(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER_RETURN_NONE(dev_id, DPP_DEV_CHANNEL_MAX - 1);
    
    p_dev_mgr = dpp_dev_mgr_get();
    ZXIC_COMM_CHECK_DEV_POINT_RETURN_NONE(dev_id,p_dev_mgr);

    p_dev_info = p_dev_mgr->p_dev_array[dev_id];
    ZXIC_COMM_CHECK_DEV_POINT_RETURN_NONE(dev_id, p_dev_info);

    slot_id = DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_UPPER_RETURN_NONE(dev_id, slot_id, DPP_PCIE_SLOT_MAX - 1);
    p_dev_info->p_std_nic_res[slot_id] = se_ptr;

}

/***********************************************************/
/** 根据类型获取互斥锁
* @param   dev_id    设备号
* @param   type         互斥锁类型，取值参照DPP_DEV_MUTEX_TYPE_E的定义
* @param   p_mutex_out  返回的互斥锁指针地址
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_dev_opr_mutex_get(DPP_DEV_T *dev, ZXIC_UINT32 type, ZXIC_MUTEX_T **p_mutex_out)
{
    //DPP_STATUS rc = 0;
    DPP_DEV_MGR_T *p_dev_mgr = NULL;
    DPP_DEV_CFG_T *p_dev_info = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev), DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_UPPER(type, DPP_DEV_MUTEX_T_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_mutex_out);

    p_dev_mgr = dpp_dev_mgr_get();
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_dev_mgr);
    p_dev_info = p_dev_mgr->p_dev_array[DEV_ID(dev)];

    if (NULL == p_dev_info)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "Get dev_info[ %d ] fail!\n", DEV_ID(dev));
        return DPP_DEV_TYPE_INVALID;
    }

    switch (type)
    {
        case DPP_DEV_MUTEX_T_REG:
        {
            *p_mutex_out = &(p_dev_info->reg_opr_mutex);
        }
        break;

        case DPP_DEV_MUTEX_T_OAM:
        {
            *p_mutex_out = &(p_dev_info->oam_mutex);
        }
        break;

        case DPP_DEV_MUTEX_T_ETM:
        {
            *p_mutex_out = &(p_dev_info->etm_mutex);
        }
        break;

        case DPP_DEV_MUTEX_T_DDR:
        {
            *p_mutex_out = &(p_dev_info->ddr_mutex);
        }
        break;

        case DPP_DEV_MUTEX_T_IND:
        {
            *p_mutex_out = &(p_dev_info->ind_mutex);
        }
        break;

        case DPP_DEV_MUTEX_T_ETCAM:
        {
            *p_mutex_out = &(p_dev_info->etcam_mutex);
        }
        break;

        case DPP_DEV_MUTEX_T_CAR0:
        {
            *p_mutex_out = &(p_dev_info->car0_mutex);
        }
        break;

        case DPP_DEV_MUTEX_T_ALG:
        {
            *p_mutex_out = &(p_dev_info->alg_mutex);
        }
        break;

        case DPP_DEV_MUTEX_T_NPPU:
        {
            *p_mutex_out = &(p_dev_info->nppu_mutex);
        }
        break;

        case DPP_DEV_MUTEX_T_SMMU0:
        {
            *p_mutex_out = &(p_dev_info->smmu0_mutex);
        }
        break;

        case DPP_DEV_MUTEX_T_SMMU1:
        {
            *p_mutex_out = &(p_dev_info->smmu1_mutex);
        }
        break;

        case DPP_DEV_MUTEX_T_ETM_2ND:
        {
            *p_mutex_out = &(p_dev_info->etm_2nd_mutex);
        }
        break;

        case DPP_DEV_MUTEX_T_LPM:
        {
            *p_mutex_out = &(p_dev_info->lpm_mutex);
        }
        break;

        case DPP_DEV_MUTEX_T_CRM_TEMP:
        {
            *p_mutex_out = &(p_dev_info->crm_temp_mutex);
        }
        break;

        case DPP_DEV_MUTEX_T_SIM:
        {
            *p_mutex_out = &(p_dev_info->sim_mutex); 
        }
        break;

        case DPP_DEV_MUTEX_T_DTB:
        {
            *p_mutex_out = &(p_dev_info->dtb_mutex); 
        }
        break;

        case DPP_DEV_MUTEX_T_PKTRX_MF_GLB_CFG_0:
        {
            *p_mutex_out = &(p_dev_info->pktrx_mf_glb_cfg_mutex_0); 
        }
        break;

        case DPP_DEV_MUTEX_T_PKTRX_MF_GLB_CFG_1:
        {
            *p_mutex_out = &(p_dev_info->pktrx_mf_glb_cfg_mutex_1); 
        }
        break;

        case DPP_DEV_MUTEX_T_PKTRX_MF_GLB_CFG_2:
        {
            *p_mutex_out = &(p_dev_info->pktrx_mf_glb_cfg_mutex_2); 
        }
        break;

        case DPP_DEV_MUTEX_T_PKTRX_MF_GLB_CFG_3:
        {
            *p_mutex_out = &(p_dev_info->pktrx_mf_glb_cfg_mutex_3); 
        }
        break;

        case DPP_DEV_MUTEX_T_SELF_RECOVER:
        {
            *p_mutex_out = &(p_dev_info->self_recover_mutex); 
        }
        break;

        case DPP_DEV_MUTEX_T_PKTRX_MF_UDF_CFG:
        {
            *p_mutex_out = &(p_dev_info->pktrx_mf_udf_cfg_mutex); 
        }
        break;

        default:
        {
            ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "mutex type is invalid!\n");
            return DPP_ERR;
        }
    }

    return DPP_OK;
}

/***********************************************************/
DPP_STATUS dpp_dev_hash_opr_mutex_create(DPP_DEV_T *dev)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 slot = 0;
    ZXIC_UINT32 hash_id = 0;
    DPP_DEV_MGR_T *p_dev_mgr = NULL;
    DPP_DEV_CFG_T *p_dev_info = NULL;
    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id, DPP_DEV_CHANNEL_MAX - 1);
    p_dev_mgr = dpp_dev_mgr_get();
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_mgr);
    p_dev_info = p_dev_mgr->p_dev_array[dev_id];
    if (NULL == p_dev_info)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "Get dev_info[ %d ] fail!\n", dev_id);
        return DPP_DEV_TYPE_INVALID;
    }
    slot = DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_INDEX(slot, 0, (DPP_PCIE_SLOT_MAX-1));
    for(hash_id = 0;hash_id < DEV_HASH_FUNC_ID_NUM;hash_id++)
    {
        rc = zxic_comm_mutex_create(&p_dev_info->hash_mutex[slot][hash_id]);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_mutex_create");
    } 
    return DPP_OK;
}
DPP_STATUS dpp_dev_hash_opr_mutex_destroy(DPP_DEV_T *dev)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 slot = 0;
    ZXIC_UINT32 hash_id = 0;
    DPP_DEV_MGR_T *p_dev_mgr = NULL;
    DPP_DEV_CFG_T *p_dev_info = NULL;
    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id, DPP_DEV_CHANNEL_MAX - 1);
    p_dev_mgr = dpp_dev_mgr_get();
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_mgr);
    p_dev_info = p_dev_mgr->p_dev_array[dev_id];
    if (NULL == p_dev_info)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "Get dev_info[ %d ] fail!\n", dev_id);
        return DPP_DEV_TYPE_INVALID;
    }
    slot = DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_INDEX(slot, 0, (DPP_PCIE_SLOT_MAX-1));
    for(hash_id = 0;hash_id < DEV_HASH_FUNC_ID_NUM;hash_id++)
    {
        rc = zxic_comm_mutex_destroy(&p_dev_info->hash_mutex[slot][hash_id]);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_mutex_destroy");
    }
    return DPP_OK;
}
/** 根据hash引擎获取红黑树的互斥锁
* @param   dev_id    设备号
* @param   fun_id        
* @param   p_mutex_out  0~3
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author   cq    @date  2024/04/02
************************************************************/
DPP_STATUS dpp_dev_hash_opr_mutex_get(DPP_DEV_T *dev, ZXIC_UINT32 fun_id, ZXIC_MUTEX_T **p_mutex_out)
{
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 slot = 0;
    DPP_DEV_MGR_T *p_dev_mgr = NULL;
    DPP_DEV_CFG_T *p_dev_info = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_UPPER(fun_id, DEV_HASH_FUNC_ID_NUM - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_mutex_out);

    p_dev_mgr = dpp_dev_mgr_get();
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_mgr);
    p_dev_info = p_dev_mgr->p_dev_array[dev_id];

    if (NULL == p_dev_info)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "Get dev_info[ %d ] fail!\n", dev_id);
        return DPP_DEV_TYPE_INVALID;
    }

    slot = DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_INDEX(slot, 0, (DPP_PCIE_SLOT_MAX-1));
    *p_mutex_out = &p_dev_info->hash_mutex[slot][fun_id];

    return DPP_OK;
}

/***********************************************************/
/** 根据index获取dtb对应队列的互斥锁
* @param   dev_id    设备号
* @param   type         互斥锁类型，取值参照DPP_DEV_MUTEX_TYPE_E的定义
* @param   p_mutex_out  返回的互斥锁指针地址
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_dev_dtb_opr_mutex_get(DPP_DEV_T *dev, ZXIC_UINT32 type, ZXIC_UINT32 index, ZXIC_MUTEX_T **p_mutex_out)
{
    //DPP_STATUS rc = 0;
    DPP_DEV_MGR_T *p_dev_mgr = NULL;
    DPP_DEV_CFG_T *p_dev_info = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev), DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_UPPER(type, DPP_DEV_MUTEX_T_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_mutex_out);

    p_dev_mgr = dpp_dev_mgr_get();
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_dev_mgr);
    p_dev_info = p_dev_mgr->p_dev_array[DEV_ID(dev)];

    if (NULL == p_dev_info)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "Get dev_info[ %d ] fail!\n", DEV_ID(dev));
        return DPP_DEV_TYPE_INVALID;
    }

    switch (type)
    {
        case DPP_DEV_MUTEX_T_DTB:
        {
            *p_mutex_out = &(p_dev_info->dtb_queue_mutex[index]);
        }
        break;

        case DPP_DEV_MUTEX_T_DTB_RB:
        {
            *p_mutex_out = &(p_dev_info->dtb_rb_mutex[index]);
        }
        break;

        default:
        {
            ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "mutex type is invalid!\n");
            return DPP_ERR;
        }
    }

    return DPP_OK;
}

/***********************************************************/
/** 根据index获取dtb对应队列的自旋锁
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   p_spin_out  返回队列的自旋锁指针地址
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cbb      @date  2025/11/20
************************************************************/
DPP_STATUS dpp_dev_dtb_opr_spin_lock_get(DPP_DEV_T *dev, ZXIC_UINT32 queue_id, ZXIC_SPIN_LOCK_T **p_spin_out)
{
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 slot = 0;
    DPP_DEV_MGR_T *p_dev_mgr = NULL;
    DPP_DEV_CFG_T *p_dev_info = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_spin_out);

    p_dev_mgr = dpp_dev_mgr_get();
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_mgr);
    p_dev_info = p_dev_mgr->p_dev_array[dev_id];

    if (NULL == p_dev_info)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "Get dev_info[ %d ] fail!\n", dev_id);
        return DPP_DEV_TYPE_INVALID;
    }

    slot = DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_INDEX(slot, 0, (DPP_PCIE_SLOT_MAX-1));
    *p_spin_out = &p_dev_info->dtb_queue_spin_lock[slot][queue_id];

    return DPP_OK;
}
/***********************************************************/
/** PCIE 默认读接口
* @param   dev_id
* @param   addr
* @param   size
* @param   p_data
*
* @return
* @remark  无
* @see
* @author  XXX      @date  2018/05/02
************************************************************/
DPP_STATUS dpp_dev_pcie_default_write(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 size, ZXIC_UINT32 *p_data)
{
    DPP_STATUS rc = 0;
    ZXIC_UINT32 i;
    ZXIC_ADDR_T abs_addr = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);

    abs_addr = DEV_PCIE_REG_ADDR(dev);

#ifdef MACRO_CPU64
        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_64_NO_ASSERT(DEV_ID(dev), abs_addr, (ZXIC_ADDR_T)addr);
#else
        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev), abs_addr, (ZXIC_ADDR_T)addr);
#endif

#ifdef DPP_FOR_AARCH64
    addr = addr - (SYS_NP_BASE_ADDR1 - DEV_PCIE_OFFSET_ADDR(dev));
    addr = X86_ADDR_2_ARRCH64(addr);
    addr = addr + (SYS_NP_BASE_ADDR1 - DEV_PCIE_OFFSET_ADDR(dev));
#endif

    abs_addr += addr;
    ZXIC_COMM_TRACE_DEBUG("dpp_dev_pcie_default_write: write abs_addr:0x%llx\n", abs_addr);

    for (i = 0; i < size; i++)
    {
        
#ifdef MACRO_CPU64
        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_64_NO_ASSERT(DEV_ID(dev), abs_addr, (ZXIC_ADDR_T)(4 * ((ZXIC_ADDR_T)(i))));
#else
        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev), abs_addr, (ZXIC_ADDR_T)(4 * ((ZXIC_ADDR_T)(i))));
#endif

        rc = dpp_pci_write32(dev, abs_addr + (ZXIC_ADDR_T)(4 * ((ZXIC_ADDR_T)(i))), p_data + i);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_pci_write32");
        ZXIC_COMM_TRACE_DEBUG("dpp_dev_pcie_default_write: write Addr:0x%llx ,Value 0x%x\n", (abs_addr + (ZXIC_ADDR_T)(4 * ((ZXIC_ADDR_T)(i)))), *(p_data + i));
    }

    return DPP_OK;
}

/***********************************************************/
/** PCIE 默认写接口
* @param   dev_id
* @param   addr
* @param   size
* @param   p_data
*
* @return
* @remark  无
* @see
* @author  XXX      @date  2018/05/02
************************************************************/
DPP_STATUS dpp_dev_pcie_default_read(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 size, ZXIC_UINT32 *p_data)
{
    DPP_STATUS rc = 0;
    ZXIC_UINT32 i;
    ZXIC_ADDR_T abs_addr = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);

    abs_addr = DEV_PCIE_REG_ADDR(dev);

#ifdef MACRO_CPU64
    ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_64_NO_ASSERT(DEV_ID(dev), abs_addr, (ZXIC_ADDR_T)(addr));
#else
    ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev), abs_addr, (ZXIC_ADDR_T)(addr));
#endif

#ifdef DPP_FOR_AARCH64
    addr = addr - (SYS_NP_BASE_ADDR1 - DEV_PCIE_OFFSET_ADDR(dev));
    addr = X86_ADDR_2_ARRCH64(addr);
    addr = addr + (SYS_NP_BASE_ADDR1 - DEV_PCIE_OFFSET_ADDR(dev));
#endif

    abs_addr += addr;

    for (i = 0; i < size; i++)
    {
        rc = dpp_pci_read32(dev, abs_addr + (ZXIC_ADDR_T)(4 * ((ZXIC_ADDR_T)(i))), p_data + i);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_pci_read32");

        ZXIC_COMM_TRACE_DEBUG("dpp_dev_pcie_default_read: Read Addr:0x%llx ,Value 0x%x\n", (abs_addr + (ZXIC_ADDR_T)(4 * ((ZXIC_ADDR_T)(i)))), *(p_data + i));
    }

    return DPP_OK;
}

/***********************************************************/
/** 获取软件存储的hash index信息
* @param   dev
* @param   hash_index
*
* @return
* @remark  无
* @see
* @author  cq      @date  2024/12/17
************************************************************/
DPP_STATUS dpp_soft_hash_index_get(DPP_DEV_T *dev,ZXIC_UINT32 *hash_index)
{
    ZXIC_UINT16 slot       = 0;
    ZXIC_UINT16 channel_id = 0;
    ZXIC_UINT32 dev_id     =0;

    DPP_DEV_CFG_T *p_dev_info = NULL;
    DPP_DEV_MGR_T *p_dev_mgr  = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, hash_index);
    
    p_dev_mgr = dpp_dev_mgr_get();
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_mgr);
    if (!p_dev_mgr->is_init)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ErrorCode[ 0x%x]: Device Manager is not init!!!\n",
                                                                     DPP_RC_DEV_MGR_NOT_INIT);
        return DPP_RC_DEV_MGR_NOT_INIT;
    }

    p_dev_info = p_dev_mgr->p_dev_array[dev_id];
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_info);

    slot = dev->pcie_channel.slot;
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, slot, 0, DPP_PCIE_SLOT_MAX - 1);
    channel_id = DPP_PCIE_CHANNEL_ID(dev->pcie_channel.vport);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, channel_id, 0, DPP_PCIE_CHANNEL_MAX - 1);
    
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_info->pcie_channel[slot][channel_id].device);

    *hash_index = p_dev_info->pcie_channel[slot][channel_id].hash_index;

    return DPP_OK;
}

/***********************************************************/
/** 软件存储hash index信息
* @param   dev
* @param   hash_index
*
* @return
* @remark  无
* @see
* @author  cq      @date  2024/12/17
************************************************************/
DPP_STATUS dpp_soft_hash_index_set(DPP_DEV_T *dev,ZXIC_UINT32 hash_index)
{
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT16 slot = 0;
    ZXIC_UINT16 channel_id = 0;

    DPP_DEV_CFG_T *p_dev_info = NULL;
    DPP_DEV_MGR_T *p_dev_mgr  = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id, DPP_DEV_CHANNEL_MAX - 1);
    
    p_dev_mgr = dpp_dev_mgr_get();
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_mgr);
    if (!p_dev_mgr->is_init)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ErrorCode[ 0x%x]: Device Manager is not init!!!\n",
                                                                     DPP_RC_DEV_MGR_NOT_INIT);
        return DPP_RC_DEV_MGR_NOT_INIT;
    }

    p_dev_info = p_dev_mgr->p_dev_array[dev_id];
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_info);

    slot = dev->pcie_channel.slot;
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, slot, 0, DPP_PCIE_SLOT_MAX - 1);
    channel_id = DPP_PCIE_CHANNEL_ID(dev->pcie_channel.vport);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, channel_id, 0, DPP_PCIE_CHANNEL_MAX - 1);

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_info->pcie_channel[slot][channel_id].device);

    p_dev_info->pcie_channel[slot][channel_id].hash_index = hash_index;

    return DPP_OK;
}

/***********************************************************/
/** 获取软件存储的DMA内存信息
* @param   dev
* @param   p_dma_size          DMA大小
* @param   p_dma_phy_addr      DMA物理地址
* @param   p_dma_vir_addr      DMA虚拟地址
*
* @return
* @remark  无
* @see
* @author  cq      @date  2025/06/11
************************************************************/
DPP_STATUS dpp_dev_dump_dma_mem_get(DPP_DEV_T *dev,ZXIC_UINT32 *p_dma_size, 
                                   ZXIC_UINT64 *p_dma_phy_addr, 
                                   ZXIC_UINT64 *p_dma_vir_addr)
{
    ZXIC_UINT16 slot       = 0;
    ZXIC_UINT16 channel_id = 0;
    ZXIC_UINT32 dev_id     =0;

    DPP_DEV_CFG_T *p_dev_info = NULL;
    DPP_DEV_MGR_T *p_dev_mgr  = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dma_size);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dma_phy_addr);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dma_vir_addr);
    
    p_dev_mgr = dpp_dev_mgr_get();
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_mgr);
    if (!p_dev_mgr->is_init)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ErrorCode[ 0x%x]: Device Manager is not init!!!\n",
                                                                     DPP_RC_DEV_MGR_NOT_INIT);
        return DPP_RC_DEV_MGR_NOT_INIT;
    }

    p_dev_info = p_dev_mgr->p_dev_array[dev_id];
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_info);

    slot = dev->pcie_channel.slot;
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, slot, 0, DPP_PCIE_SLOT_MAX - 1);
    channel_id = DPP_PCIE_CHANNEL_ID(dev->pcie_channel.vport);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, channel_id, 0, DPP_PCIE_CHANNEL_MAX - 1);
    
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_info->pcie_channel[slot][channel_id].device);

    *p_dma_size = p_dev_info->pcie_channel[slot][channel_id].dump_dma_size;
    *p_dma_phy_addr = p_dev_info->pcie_channel[slot][channel_id].dump_dma_phy_addr;
    *p_dma_vir_addr = p_dev_info->pcie_channel[slot][channel_id].dump_dma_vir_addr;

    if((*p_dma_size==0) || (*p_dma_phy_addr==0) ||(*p_dma_vir_addr==0))
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ErrorCode[ 0x%x]: dump dma mem get fail!!!\n",
                                                                     DPP_RC_DEV_DMA_MEM_GET_FAIL);
        return DPP_RC_DEV_DMA_MEM_GET_FAIL;
    }

    ZXIC_COMM_MEMSET_S(ZXIC_COMM_VAL_TO_PTR(*p_dma_vir_addr),*p_dma_size,0x0,*p_dma_size);

    return DPP_OK;
}

#ifndef ES_FOR_LLT
/***********************************************************/
/** 写设备底层接口
* @param   dev_id  设备号
* @param   addr       单个设备内部的相对地址
* @param   size       数据的长度，以32bit为单位
* @param   p_data     数据
*
* @return
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_dev_write_channel(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 size, ZXIC_UINT32 *p_data)
{
    /*ZXIC_UINT32     i = 0;*/
    DPP_STATUS rtn      = 0;
#if DPP_HW_OPR_EN
    DPP_DEV_CFG_T *p_dev_info = NULL;
#endif

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev), DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);
    
#ifdef DPP_FOR_LLT
    rtn = dpp_stump_reg_rb_debug_wr(DEV_ID(dev), 
                                    addr,
                                    size,
                                    p_data);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_stump_reg_rb_debug_wr");
    return DPP_OK;
#endif

#if DPP_HW_OPR_EN
    p_dev_info = DPP_DEV_INFO_GET(DEV_ID(dev));

    if (p_dev_info == NULL)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "Error: Channel[%d] dev is not exist!\n ", DEV_ID(dev));
        return DPP_ERR;
    }
    else
    {
        if (p_dev_info->access_type == DPP_DEV_ACCESS_TYPE_PCIE)
        {
            ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_dev_info->p_pcie_write_fun);
            rtn = p_dev_info->p_pcie_write_fun(dev, addr, size, p_data);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "p_dev_info->p_pcie_write_fun");
        }
        else
        {
            ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "Dev access type[ %d ] is invalid!\n", p_dev_info->access_type);
            return DPP_ERR;
        }
    }

#endif

    return DPP_OK;
}

/***********************************************************/
/** 读设备底层接口
* @param   dev_id  设备号
* @param   addr       单个设备内部的相对地址
* @param   size       数据的长度，以32bit为单位
* @param   p_data     数据
*
* @return
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_dev_read_channel(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 size, ZXIC_UINT32 *p_data)
{
    DPP_STATUS rtn = 0;
#if DPP_HW_OPR_EN
    DPP_DEV_CFG_T *p_dev_info = NULL;
#endif

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev), DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);

#ifdef DPP_FOR_LLT
    rtn = dpp_stump_reg_rb_debug_rd(DEV_ID(dev), 
                                    addr,
                                    size,
                                    p_data);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_stump_reg_rb_debug_rd");
    return DPP_OK;
#endif

#if DPP_HW_OPR_EN
    p_dev_info = DPP_DEV_INFO_GET(DEV_ID(dev));

    if (p_dev_info == NULL)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "Error: Channel[%d] dev is not exist!\n ", DEV_ID(dev));
        return DPP_ERR;
    }
    else
    {
        if (p_dev_info->access_type == DPP_DEV_ACCESS_TYPE_PCIE)
        {
            ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_dev_info->p_pcie_read_fun);
            rtn = p_dev_info->p_pcie_read_fun(dev, addr, size, p_data);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "p_dev_info->p_pcie_read_fun");
        }
        else
        {
            ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "Dev access type[ %d ] is invalid!\n", p_dev_info->access_type);
            return DPP_ERR;
        }
    }

#else

    for (ZXIC_UINT32 i = 0; i < size; i++)
    {
        p_data[i] = 0xffffffff;
    }

#endif

    return DPP_OK;
}
#endif /* ES_FOR_LLT */
