/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_sdt_mgr.c
* 文件标识 :
* 内容摘要 : SDT属性软件缓存，以及接收上层配置并下发给底层设备
* 其它说明 :
* 当前版本 :
* 完成日期 : 2015/06/25
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/

#include "zxic_common.h"

#include "dpp_dev.h"
#include "dpp_sdt_def.h"
#include "dpp_sdt.h"
#include "dpp_sdt_mgr.h"
#include "dpp_se_api.h"

static DPP_SDT_MGR_T g_sdt_mgr = {0};

#define DPP_SDT_MGR_PTR_GET()    (&g_sdt_mgr)

#define DPP_SDT_SOFT_TBL_GET(id) (g_sdt_mgr.sdt_tbl_array[id])

ZXIC_UINT32 dpp_sdt_mgr_init(ZXIC_VOID)
{
    if (!g_sdt_mgr.is_init)
    {
        g_sdt_mgr.channel_num = 0;
        g_sdt_mgr.is_init = 1;
        // g_sdt_mgr.p_sdt_mgr_smmu0_mux = dpp_tbl_dir_sdt_smmu0_mux;
        // g_sdt_mgr.p_sdt_mgr_smmu1_mux = dpp_tbl_dir_sdt_smmu1_mux;
        // g_sdt_mgr.p_sdt_mgr_hash_mux = dpp_tbl_dir_sdt_hash_mux;
        // g_sdt_mgr.p_sdt_mgr_lpm_mux = dpp_tbl_dir_sdt_lpm_mux;
        // g_sdt_mgr.p_sdt_mgr_etcam_mux = dpp_tbl_dir_sdt_etcam_mux;
        ZXIC_COMM_MEMSET(g_sdt_mgr.sdt_tbl_array, 0, DPP_DEV_CHANNEL_MAX * sizeof(DPP_SDT_SOFT_TABLE_T *));
    }

    return DPP_OK;
}

ZXIC_UINT32 dpp_sdt_mgr_create(ZXIC_UINT32 dev_id)
{
    DPP_SDT_SOFT_TABLE_T *p_sdt_tbl_temp = NULL;
    DPP_SDT_MGR_T *p_sdt_mgr = NULL;

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);

    p_sdt_mgr = DPP_SDT_MGR_PTR_GET();

    if (DPP_SDT_SOFT_TBL_GET(dev_id) == NULL)
    {
        p_sdt_tbl_temp = ZXIC_COMM_MALLOC(sizeof(DPP_SDT_SOFT_TABLE_T));
        ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_sdt_tbl_temp);      /* mod for KW_0411 # 474 */

        p_sdt_tbl_temp->device_id = dev_id;
        ZXIC_COMM_MEMSET(p_sdt_tbl_temp->sdt_array, 0, DPP_PCIE_SLOT_MAX*DPP_DEV_SDT_ID_MAX * sizeof(DPP_SDT_ITEM_T));

        DPP_SDT_SOFT_TBL_GET(dev_id) = p_sdt_tbl_temp;
        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id, p_sdt_mgr->channel_num, 1);
        p_sdt_mgr->channel_num++;
    }
    else
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "Error: dpp_sdt_mgr_create for dev[%d] is called repeatedly!\n", dev_id);
    }

    return DPP_OK;
}

ZXIC_UINT32 dpp_sdt_mgr_destroy(ZXIC_UINT32 dev_id)
{
    DPP_SDT_SOFT_TABLE_T *p_sdt_tbl_temp = NULL;
    DPP_SDT_MGR_T *p_sdt_mgr = NULL;

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);

    p_sdt_tbl_temp = DPP_SDT_SOFT_TBL_GET(dev_id);
    p_sdt_mgr = DPP_SDT_MGR_PTR_GET();

    if (NULL != p_sdt_tbl_temp)
    {
        ZXIC_COMM_FREE(p_sdt_tbl_temp);
    }

    DPP_SDT_SOFT_TBL_GET(dev_id) = NULL;

    ZXIC_COMM_CHECK_DEV_INDEX_SUB_OVERFLOW_NO_ASSERT(dev_id, p_sdt_mgr->channel_num, 1);
    p_sdt_mgr->channel_num--;

    return DPP_OK;
}


#if 1
/***********************************************************/
/** 向软件缓存中添加SDT表条目
* @param   dev_id  设备号
* @param   sdt_no     业务表对应的sdt号
* @param   sdt_hig32  SDT属性高32bit
* @param   sdt_low32  SDT属性低32bit
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_sdt_mgr_sdt_item_add(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no, ZXIC_UINT32 sdt_hig32, ZXIC_UINT32 sdt_low32)
{
    ZXIC_UINT32 slot = 0;
    ZXIC_UINT32 dev_id = 0;
    DPP_SDT_SOFT_TABLE_T *p_sdt_soft_tbl = NULL;
    DPP_SDT_ITEM_T *p_sdt_item = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    slot = DEV_PCIE_SLOT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, slot,    0, DPP_PCIE_SLOT_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, sdt_no,    0, DPP_DEV_SDT_ID_MAX - 1);

    p_sdt_soft_tbl = DPP_SDT_SOFT_TBL_GET(dev_id);

    if (NULL == p_sdt_soft_tbl)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "Error: dpp_sdt_mgr_sdt_item_add soft sdt table not Init! \n");
        ZXIC_COMM_ASSERT(0);
        return DPP_RC_TABLE_SDT_MGR_INVALID;
    }

    if (dev_id != p_sdt_soft_tbl->device_id)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "Error: dpp_sdt_mgr_sdt_item_add soft sdt table Item Invalid! \n");
        ZXIC_COMM_ASSERT(0);
        return DPP_RC_TABLE_PARA_INVALID;
    }

    /*添加SDT表项*/
    p_sdt_item = &(p_sdt_soft_tbl->sdt_array[slot][sdt_no]);
    p_sdt_item->valid = DPP_SDT_VALID;
    p_sdt_item->table_cfg[0] = sdt_hig32; /* hig32 */
    p_sdt_item->table_cfg[1] = sdt_low32; /* low32 */

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "dpp_sdt_mgr_sdt_item_add 0x%08x 0x%08x \n", p_sdt_item->table_cfg[0], p_sdt_item->table_cfg[1]);

    return DPP_OK;
}

/***********************************************************/
/** 从软件缓存中读取SDT表条目
* @param   dev_id    设备号
* @param   sdt_no       业务表对应的sdt号
* @param   p_sdt_hig32  SDT属性高32bit
* @param   p_sdt_low32  SDT属性低32bit
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_sdt_mgr_sdt_item_srh(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no, ZXIC_UINT32 *p_sdt_hig32, ZXIC_UINT32 *p_sdt_low32)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 slot = 0;
    DPP_SDT_SOFT_TABLE_T *p_sdt_soft_tbl = NULL;
    DPP_SDT_ITEM_T *p_sdt_item = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    slot = DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, slot,    0, DPP_PCIE_SLOT_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, sdt_no,  0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_sdt_hig32);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_sdt_low32);

    p_sdt_soft_tbl = DPP_SDT_SOFT_TBL_GET(dev_id);

    if (NULL == p_sdt_soft_tbl)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "Error: dpp_sdt_mgr_sdt_item_srh Soft Table not Init! \n");
        ZXIC_COMM_ASSERT(0);
        return DPP_RC_TABLE_SDT_MGR_INVALID;
    }

    if (dev_id != p_sdt_soft_tbl->device_id)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "Error: dpp_sdt_mgr_sdt_item_srh Soft Table Item Invalid ! \n");
        ZXIC_COMM_ASSERT(0);
        return DPP_RC_TABLE_PARA_INVALID;
    }

    /* 获取SDT表项 */
    p_sdt_item = &p_sdt_soft_tbl->sdt_array[slot][sdt_no];

    if (DPP_SDT_VALID == p_sdt_item->valid)
    {
        *p_sdt_hig32 = p_sdt_item->table_cfg[0];
        *p_sdt_low32 = p_sdt_item->table_cfg[1];
    }
    else
    {
        *p_sdt_hig32 = 0xFFFFFFFF;
        *p_sdt_low32 = 0xFFFFFFFF;
    }

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "dpp_sdt_mgr_sdt_item_srh is %s: sdt_no: 0x%08x sdt_hig32:0x%08x sdt_low32:0x%08x \n", ((DPP_SDT_VALID == p_sdt_item->valid) ? ("success") : ("fail")), sdt_no, *p_sdt_hig32, *p_sdt_low32);

    return rc;
}

/***********************************************************/
/** 从软件缓存中删除SDT表条目
* @param   dev_id  设备号
* @param   sdt_no     业务表对应的sdt号
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_sdt_mgr_sdt_item_del(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no)
{
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 slot = 0;
    DPP_SDT_SOFT_TABLE_T *p_sdt_soft_tbl = NULL;
    DPP_SDT_ITEM_T *p_sdt_item = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    slot = DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, sdt_no,  0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, slot,  0, DPP_PCIE_SLOT_MAX - 1);

    p_sdt_soft_tbl = DPP_SDT_SOFT_TBL_GET(dev_id);
    if (NULL != p_sdt_soft_tbl)
    {
        if (dev_id != p_sdt_soft_tbl->device_id)
        {
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "Error: dpp_sdt_mgr_sdt_item_del Soft Table Item Invalid ! \n");
            ZXIC_COMM_ASSERT(0);
            return DPP_RC_TABLE_PARA_INVALID;
        }

        p_sdt_item = &p_sdt_soft_tbl->sdt_array[slot][sdt_no];
        p_sdt_item->valid = DPP_SDT_INVALID;
        p_sdt_item->table_cfg[0] = 0;
        p_sdt_item->table_cfg[1] = 0;
    }
    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "dpp_sdt_mgr_sdt_item_del sdt_no: 0x%08x  \n", sdt_no);
    return DPP_OK;
}

#endif


#if 1
/*********************************************************************
 * 函数名称：NpeSdtMgr_GetTblType
 *
 * 功能描述:
 *
 * 输入参数：
 * 输出参数：
 * 返 回 值：
 * 全局变量：
 * 注    释：
 ============================================================
 * 修改记录:
 * 修改日期      版本号      修改人       修改内容
 * 20120327      v1.0        石金锋       创建
 ============================================================
 *
 *********************************************************************/
DPP_TBL_TYPE_E dpp_sdt_mgr_get_tbl_type(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no)
{
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 rtn = 0;
    DPP_TBL_TYPE_E   table_type = 0;
    ZXIC_UINT32 table_cfg[DPP_SDT_CFG_LEN] = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);

    rtn = dpp_sdt_mgr_sdt_item_srh(dev, sdt_no, &(table_cfg[0]), &(table_cfg[1]));
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "dpp_sdt_mgr_sdt_item_srh");

    table_type = (DPP_TBL_TYPE_E)((table_cfg[0] >> 29) & 0x7);

    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "dpp_sdt_mgr_get_tbl_type: dev_id: %d, sdt_no: %d, table_type: %d. \n", dev_id, sdt_no, table_type);

    return table_type;
}

#endif
