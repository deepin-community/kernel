/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_hash.c
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : wcl
* 完成日期 : 2014/02/08
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
#include "dpp_module.h"
#include "dpp_se_cfg.h"
#include "dpp_hash_crc.h"
#include "dpp_hash.h"
#include "dpp_se.h"
#include "dpp_se_reg.h"
#include "dpp_reg_api.h"
#include "dpp_reg_info.h"
#include "dpp_se4k_reg.h"
#include "dpp_sdt.h"

#define HASH_CMP_ZCELL     (1)
#define HASH_CMP_ZBLK      (2)

#define HASH_ENTRY_STAT

/* 全局变量定义区 */
static ZXIC_UINT32 g_ddr_hash_arg[HASH_DDR_CRC_NUM] = {
    0x04C11DB7, 0xF4ACFB13, 0x20044009, 0x00210801
};

static DPP_HASH_TBL_ID_INFO g_tbl_id_info[DPP_PCIE_SLOT_MAX][HASH_FUNC_ID_NUM][HASH_TBL_ID_NUM] = {{{{0}}}};

/** 4个hash引擎，每个最多使用32个zblock */
static ZXIC_UINT32 g_hash_zblk_idx[DPP_PCIE_SLOT_MAX][HASH_FUNC_ID_NUM][HASH_TBL_ID_NUM] = {{{0}}};

static DPP_HASH_SOFT_RESET_STOR_DAT g_hash_store_dat[DPP_PCIE_SLOT_MAX] = {{{0}}};

#if DPP_WRITE_FILE_EN
static DPP_HASH_FILE_REG_T g_hash_file_reg = {
    {0x55550000, 0xffffaaaa}, {1, 1, 1, 1},
    {
        {0x12121212, 0x12121212}, {0x12121212, 0x12121212},
        {0x12121212, 0x12121212}, {0x12121212, 0x12121212}
    },
    0,
    0xffff,
    0,
    {0},
    {0}
};
#endif
/* 宏函数定义区 */
#define GET_DDR_HASH_ARG(ddr_crc_sel)              (g_ddr_hash_arg[ddr_crc_sel])
#define GET_HASH_TBL_ID_INFO(slot_id, fun_id, tbl_id)       (&g_tbl_id_info[slot_id][fun_id][tbl_id])
#define GET_ACTU_KEY_SIZE_BY_TBLID(slot_id,fun_id, tbl_id) (g_tbl_id_info[slot_id][fun_id][tbl_id].actu_key_size)

#define HASH_TBL_ID_INFO_CHECK(slot_id,fun_id, tbl_id, key_type) \
    do{\
        if (!g_tbl_id_info[slot_id][fun_id][tbl_id].is_init || \
            (g_tbl_id_info[slot_id][fun_id][tbl_id].key_type != key_type)) \
        { \
            ZXIC_COMM_PRINT("init [%d], config ketype[%d] parameter key_type[%d].\n",\
                      g_tbl_id_info[slot_id][fun_id][tbl_id].is_init, \
                      g_tbl_id_info[slot_id][fun_id][tbl_id].key_type, \
                      key_type);\
            ZXIC_COMM_ASSERT(0); \
            return DPP_HASH_RC_INVALID_TBL_ID_INFO; \
        } \
    }while (0)


#define GET_HASH_DDR_HW_ADDR(base_addr, item_idx) \
    ((base_addr) + (item_idx))

/* 9:zcell depth */
#define GET_HASH_ZCAM_HW_ADDR(base_addr, zblk_idx, item_idx) \
    ((base_addr) + ((zblk_idx) << 9) + (item_idx))

#define DPP_GET_HASH_FILE_REG() (&g_hash_file_reg)

/* 局部函数声明区 */

/* 函数实现区 */

#if ZXIC_REAL("inter func.")
/***********************************************************/
/** zblock资源链表排序比较函数
* @param   data1        待比较的数据1
* @param   data2        待比较的数据2
* @param   data         排序比较的类型，1-zcell比较，2-zblock比较
*
* @return  1-data1比data2的索引大
           0-data1和data2索引相同
          -1-data1比data2的索引小
* @remark  无
* @see
* @author  wcl      @date  2014/11/10
************************************************************/
ZXIC_SINT32 dpp_hash_list_cmp(D_NODE *data1, D_NODE *data2, void *data)
{
    ZXIC_UINT32 flag = 0;
    ZXIC_UINT32 data_new = 0;
    ZXIC_UINT32 data_pre = 0;

    ZXIC_COMM_CHECK_POINT(data1);
    ZXIC_COMM_CHECK_POINT(data2);
    ZXIC_COMM_CHECK_POINT(data);

    flag = *(ZXIC_UINT32 *)data;

    if (flag == HASH_CMP_ZCELL)
    {
        data_new = ((SE_ZCELL_CFG *)data1->data)->zcell_idx;
        data_pre = ((SE_ZCELL_CFG *)data2->data)->zcell_idx;
    }
    else if (flag == HASH_CMP_ZBLK)
    {
        data_new = ((SE_ZBLK_CFG *)data1->data)->zblk_idx;
        data_pre = ((SE_ZBLK_CFG *)data2->data)->zblk_idx;
    }

    if (data_new > data_pre)
    {
        return 1;
    }
    else if (data_new == data_pre)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

/***********************************************************/
/** hash键值红黑树插入比较函数
* @param   p_new
* @param   p_old
* @param   key_size
*
* @return
* @remark  无
* @see
* @author  wcl      @date  2014/11/10
************************************************************/
ZXIC_SINT32 dpp_hash_rb_key_cmp(ZXIC_VOID *p_new, ZXIC_VOID *p_old, ZXIC_UINT32 key_size)
{
    DPP_HASH_RBKEY_INFO *p_rbkey_new = NULL;
    DPP_HASH_RBKEY_INFO *p_rbkey_old = NULL;

    ZXIC_COMM_CHECK_POINT(p_new);
    ZXIC_COMM_CHECK_POINT(p_old);
    p_rbkey_new = (DPP_HASH_RBKEY_INFO *)(p_new);
    p_rbkey_old = (DPP_HASH_RBKEY_INFO *)(p_old);

    return ZXIC_COMM_MEMCMP(p_rbkey_new->key, p_rbkey_old->key, HASH_KEY_MAX);
}

/***********************************************************/
/** hash键值红黑树插入比较函数
* @param   p_new
* @param   p_old
* @param   key_size
*
* @return
* @remark  无
* @see
* @author  tf      @date  2016/03/26
************************************************************/
ZXIC_SINT32 dpp_hash_ddr_cfg_rb_key_cmp(ZXIC_VOID *p_new, ZXIC_VOID *p_old, ZXIC_UINT32 key_size)
{
    HASH_DDR_CFG *p_rbkey_new = NULL;
    HASH_DDR_CFG *p_rbkey_old = NULL;

    ZXIC_COMM_CHECK_POINT(p_new);
    ZXIC_COMM_CHECK_POINT(p_old);
    p_rbkey_new = (HASH_DDR_CFG *)(p_new);
    p_rbkey_old = (HASH_DDR_CFG *)(p_old);

    return ZXIC_COMM_MEMCMP(&p_rbkey_new->ddr_baddr, &p_rbkey_old->ddr_baddr, sizeof(ZXIC_UINT32));
}

/***********************************************************/
/** 获取位宽
* @param   ddr_item_num
*
* @return
* @remark  无
* @see
* @author  wcl      @date  2014/11/10
************************************************************/
ZXIC_UINT32 dpp_hash_ddr_depth_conv(ZXIC_UINT32 ddr_item_num)
{
    ZXIC_UINT32 count = 0;

    while (ddr_item_num > ((ZXIC_UINT32)1 << count))
    {
        count++;
    }

    return count;
}

/***********************************************************/
/** 片内资源初始化
* @param   p_hash_cfg
* @param   zblk_num
* @param   zblk_idx_array
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wcl      @date  2015/10/13
************************************************************/
DPP_STATUS dpp_hash_zcam_resource_init(DPP_HASH_CFG *p_hash_cfg, ZXIC_UINT32 zblk_num, ZXIC_UINT32 *zblk_idx_array)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 cmp_type = 0;
    ZXIC_UINT32 zblk_idx = 0;
    ZXIC_UINT32 zcell_idx = 0;
    ZXIC_UINT32 dev_id = 0;

    D_HEAD *p_zblk_list = NULL;
    D_HEAD *p_zcell_free = NULL;
    SE_ZBLK_CFG *p_zblk_cfg = NULL;
    SE_ZCELL_CFG *p_zcell_cfg = NULL;

    ZXIC_COMM_CHECK_POINT(p_hash_cfg);
    ZXIC_COMM_CHECK_INDEX(zblk_num, 0, SE_ZBLK_NUM);
    ZXIC_COMM_CHECK_POINT(zblk_idx_array);

    dev_id = p_hash_cfg->p_se_info->dev_id;

    /* init zblock list */
    p_zblk_list = &p_hash_cfg->hash_shareram.zblk_list;
    rc = zxic_comm_double_link_init(SE_ZBLK_NUM, p_zblk_list);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_double_link_init");

    /* init zcell list */
    p_zcell_free = &p_hash_cfg->hash_shareram.zcell_free_list;
    rc = zxic_comm_double_link_init(SE_ZBLK_NUM * SE_ZCELL_NUM, p_zcell_free);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_double_link_init");

    for (i = 0; i < zblk_num; i++)
    {
        zblk_idx = zblk_idx_array[i];
        /* debug start */
        //ZXIC_COMM_PRINT("zblk_idx is [%d]\n", zblk_idx); /* t */
        /* debug end */
        ZXIC_COMM_CHECK_DEV_INDEX(dev_id, zblk_idx, 0, SE_ZBLK_NUM - 1);
        p_zblk_cfg = DPP_SE_GET_ZBLK_CFG(p_hash_cfg->p_se_info, zblk_idx);

        if (p_zblk_cfg->is_used)
        {
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ErrorCode:[0x%x], ZBlock[%d] is already used by other function!\n", \
                                     DPP_HASH_RC_INVALID_ZBLCK, zblk_idx);
            ZXIC_COMM_ASSERT(0);
            return DPP_HASH_RC_INVALID_ZBLCK;
        }

        for (j = 0; j < SE_ZCELL_NUM; j++)
        {
            zcell_idx = p_zblk_cfg->zcell_info[j].zcell_idx;
            p_zcell_cfg = DPP_SE_GET_ZCELL_CFG(p_hash_cfg->p_se_info, zcell_idx);

            if (p_zcell_cfg->is_used)
            {
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ErrorCode:[0x%x], ZBlk[%d], ZCell[%d] is already used by other function!\n", \
                                         DPP_HASH_RC_INVALID_ZCELL, zblk_idx, zcell_idx);
                ZXIC_COMM_ASSERT(0);
                return DPP_HASH_RC_INVALID_ZCELL;
            }

            p_zcell_cfg->is_used = 1;

            /* insert to free zcell free list */
            cmp_type = HASH_CMP_ZCELL;
            rc = zxic_comm_double_link_insert_sort(&p_zcell_cfg->zcell_dn, p_zcell_free, dpp_hash_list_cmp, &cmp_type);
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_double_link_insert_sort");
        }

        /* insert to zblock list */
        p_zblk_cfg->is_used = 1;
        cmp_type = HASH_CMP_ZBLK;
        rc = zxic_comm_double_link_insert_sort(&p_zblk_cfg->zblk_dn, p_zblk_list, dpp_hash_list_cmp, &cmp_type);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_double_link_insert_last");

        /* config zblock */
        rc = dpp_hash_zblkcfg_write(p_hash_cfg->p_se_info, p_hash_cfg->fun_id, p_zblk_cfg);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_hash_zblkcfg_write");
    }

    return DPP_OK;
}

/***********************************************************/
/**
* @param   p_hash_cfg
*
* @return
* @remark  无
* @see
* @author  XCX      @date  2018/03/22
************************************************************/
DPP_STATUS dpp_hash_zcam_resource_deinit(DPP_HASH_CFG *p_hash_cfg)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 dev_id = 0;

    D_NODE *p_node = NULL;
    D_HEAD *p_head = NULL;
    SE_ZBLK_CFG *p_zblk_cfg = NULL;
    SE_ZCELL_CFG *p_zcell_cfg = NULL;

    ZXIC_COMM_CHECK_POINT(p_hash_cfg);

    dev_id = p_hash_cfg->p_se_info->dev_id;

    /*delete zcell node*/
    p_head = &p_hash_cfg->hash_shareram.zcell_free_list;

    while (p_head->used)
    {
        p_node = p_head->p_next;

        rc = zxic_comm_double_link_del(p_node, p_head);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_double_link_del");
        p_zcell_cfg = (SE_ZCELL_CFG *)p_node->data;
        p_zcell_cfg->is_used = 0;
    }

    /*delete zblk node*/
    p_head = &p_hash_cfg->hash_shareram.zblk_list;

    while (p_head->used)
    {
        p_node = p_head->p_next;

        rc = zxic_comm_double_link_del(p_node, p_head);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_double_link_del");

        ZXIC_COMM_CHECK_DEV_INDEX(dev_id, ((SE_ZBLK_CFG *)p_node->data)->zblk_idx, 0, HASH_ZBLK_ID_MAX);
        p_zblk_cfg = DPP_SE_GET_ZBLK_CFG(p_hash_cfg->p_se_info, ((SE_ZBLK_CFG *)p_node->data)->zblk_idx);
        p_zblk_cfg->is_used = 0;

        /*clear zblk config*/
        /* dpp_hash_zblkcfg_clr(p_hash_cfg->p_se_info, p_hash_cfg->fun_id, p_zblk_cfg); */
    }

    return DPP_OK;
}

/***********************************************************/
/**
* @param   item_entry_max
* @param   wrt_mask
* @param   entry_size
*
* @return
* @remark  无
* @see
* @author  XCX      @date  2018/03/22
************************************************************/
ZXIC_UINT32 dpp_hash_get_item_free_pos(ZXIC_UINT32 item_entry_max, ZXIC_UINT32 wrt_mask, ZXIC_UINT32 entry_size)
{
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 pos = 0xFFFFFFFF;/* -1; */
    ZXIC_UINT32 mask = 0;

    for (i = 0; i < item_entry_max; i += entry_size / HASH_ENTRY_POS_STEP)
    {
        ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(i, entry_size / HASH_ENTRY_POS_STEP);
        mask = DPP_GET_HASH_ENTRY_MASK(entry_size, i);

        if (0 == (mask & wrt_mask))
        {
            pos = i;
            break;
        }
    }

    return pos;
}

/***********************************************************/
/**
* @param   p_hash_cfg
* @param   p_rbkey
* @param   p_item
* @param   item_idx
* @param   item_type
* @param   insrt_key_type
*
* @return
* @remark  无
* @see
* @author  XCX      @date  2018/03/22
************************************************************/
DPP_STATUS dpp_hash_insrt_to_item(DPP_HASH_CFG *p_hash_cfg,
                                  DPP_HASH_RBKEY_INFO *p_rbkey,
                                  SE_ITEM_CFG *p_item,
                                  ZXIC_UINT32 item_idx,
                                  ZXIC_UINT32 item_type,
                                  ZXIC_UINT32 insrt_key_type)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 free_pos = 0;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 item_entry_max = ITEM_ENTRY_NUM_4;

    ZXIC_COMM_CHECK_POINT(p_hash_cfg);
    ZXIC_COMM_CHECK_POINT(p_rbkey);
    ZXIC_COMM_CHECK_POINT(p_item);

    dev_id = p_hash_cfg->p_se_info->dev_id;

    if (ITEM_DDR_256 == item_type)
    {
        item_entry_max = ITEM_ENTRY_NUM_2;
    }

    if (!p_item->valid) /* item is not used */
    {
        rc = zxic_comm_double_link_init(item_entry_max, &p_item->item_list);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_double_link_init");

        p_rbkey->entry_pos = HASH_ITEM_POS_0;
        p_item->wrt_mask = DPP_GET_HASH_ENTRY_MASK(p_rbkey->entry_size, p_rbkey->entry_pos);
        p_item->item_index = item_idx;
        p_item->item_type = item_type;
        p_item->valid = 1;
    }
    else
    {
        free_pos = dpp_hash_get_item_free_pos(item_entry_max, p_item->wrt_mask, p_rbkey->entry_size);

        if (0xFFFFFFFF == free_pos)
        {
            return DPP_HASH_RC_ITEM_FULL;
        }
        else
        {
            p_rbkey->entry_pos = free_pos;
            p_item->wrt_mask   |= DPP_GET_HASH_ENTRY_MASK(p_rbkey->entry_size, p_rbkey->entry_pos);
        }
    }

    /* debug*/
    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "Entry in item pos is: [%d], entry size is: [%d].\n", free_pos, p_rbkey->entry_size);

    rc = zxic_comm_double_link_insert_last(&p_rbkey->entry_dn, &p_item->item_list);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_double_link_insert_last");

    p_rbkey->p_item_info = p_item;

    return DPP_OK;
}

#endif

#if ZXIC_REAL("External Func.")
/***********************************************************/
/** 单个hash引擎初始化
* @param   p_se_cfg      算法模块公共管理数据结构指针
* @param   fun_id        hash引擎号
* @param   zblk_num      分配给此hash引擎的zblock数目
* @param   zblk_idx      分配给此hash引擎的zblock编号
* @param   ddr_dis       DDR关闭位，0-不关闭片外DDR, 1-关闭片外DDR
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wcl      @date  2015/05/15
************************************************************/
DPP_STATUS dpp_hash_init(DPP_SE_CFG *p_se_cfg,
                         ZXIC_UINT32 fun_id,
                         ZXIC_UINT32 zblk_num,
                         ZXIC_UINT32 *zblk_idx,
                         ZXIC_UINT32 ddr_dis)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 slot_id = 0;

    FUNC_ID_INFO *p_func_info = NULL;
    DPP_HASH_CFG *p_hash_cfg = NULL;

    ZXIC_COMM_CHECK_POINT(p_se_cfg);
    ZXIC_COMM_CHECK_POINT(zblk_idx);
    ZXIC_COMM_CHECK_INDEX(fun_id, HASH_FUNC_ID_MIN, HASH_FUNC_ID_NUM - 1);
    ZXIC_COMM_CHECK_INDEX(zblk_num, 0, SE_ZBLK_NUM);
    ZXIC_COMM_CHECK_INDEX(ddr_dis, 0, 1);

    dev_id = p_se_cfg->dev_id;
    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id,DPP_DEV_CHANNEL_MAX - 1);

    slot_id  = p_se_cfg->dev.pcie_channel.slot;
    ZXIC_COMM_CHECK_INDEX_UPPER(slot_id,DPP_PCIE_SLOT_MAX - 1);

    rc = dpp_se_fun_init(p_se_cfg, (fun_id & 0xff), FUN_HASH);
    if(DPP_SE_RC_FUN_INVALID==rc)
    {
        return DPP_OK;
    }
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_se_fun_init");

    p_func_info = DPP_GET_FUN_INFO(p_se_cfg, fun_id);
    p_hash_cfg = (DPP_HASH_CFG *)p_func_info->fun_ptr;
    p_hash_cfg->fun_id = fun_id;
    p_hash_cfg->p_hash32_fun = dpp_crc32_calc;
    p_hash_cfg->p_hash16_fun = dpp_crc16_calc;
    p_hash_cfg->p_se_info = p_se_cfg;

    /*    判定hash引擎是片内模式还是混合模式*/
    if (ddr_dis == 1)
    {
        /* disable ddr */
        p_hash_cfg->ddr_valid = 0;

#ifdef DPP_FLOW_HW_INIT
        rc = dpp_hash_ext_cfg_clr(p_se_cfg, fun_id);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_hash_ddrcfg_clr");

        rc = dpp_hash_tbl_depth_clr(p_se_cfg, fun_id);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_hash_tbl_depth_clr");
#endif

    }
    else
    {
        p_hash_cfg->ddr_valid = 1;

    }

    /*    zcam初始化*/
    p_hash_cfg->hash_stat.zblock_num = zblk_num;
    rc = dpp_hash_zcam_resource_init(p_hash_cfg, zblk_num, zblk_idx);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_hash_zcam_resource_init");

    /* 将zblock_id记录在软件缓存中 */
    for(i = 0; i < zblk_num; i++)
    {
        p_hash_cfg->hash_stat.zblock_array[i] = zblk_idx[i];
    }

    /* dynamic alloc rb_tree node by user. */

    rc = (DPP_STATUS)zxic_comm_rb_init(&p_hash_cfg->hash_rb,
                                 0,
                                 sizeof(DPP_HASH_RBKEY_INFO),
                                 dpp_hash_rb_key_cmp);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_rb_init");

    /* dynamic alloc rb_tree node by user. */
    rc = (DPP_STATUS)zxic_comm_rb_init(&p_hash_cfg->ddr_cfg_rb,
                                 0,
                                 sizeof(HASH_DDR_CFG),
                                 dpp_hash_ddr_cfg_rb_key_cmp);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_rb_init");

    /** for 软复位 */
    /** 保存zblock内容 */
    for (i = 0; i < zblk_num; i++)
    {
        g_hash_zblk_idx[slot_id][fun_id][i] = zblk_idx[i];
    }

    g_hash_store_dat[slot_id].ddr_dis_flag[fun_id] = ddr_dis;
    g_hash_store_dat[slot_id].zblk_num[fun_id] = zblk_num;
    g_hash_store_dat[slot_id].zblk_idx_start[fun_id] = g_hash_zblk_idx[dev_id][fun_id];
    g_hash_store_dat[slot_id].hash_id_valid |= (ZXIC_UINT32)((1U << fun_id) & 0xffffffff);

    return DPP_OK;
}

/***********************************************************/
/** 初始化单个hash引擎内的某个业务表，此接口支持为该业务表分配独占的zcell。
*     必须先初始化hash引擎，再初始化业务表。
* @param   p_se_cfg        算法模块公共管理数据结构指针
* @param   fun_id          hash引擎号
* @param   bulk_id         每个Hash引擎资源划分的空间ID号
* @param   p_ddr_resc_cfg  分配给hash引擎此资源空间的ddr资源属性
* @param   zcell_num       分配给hash引擎此资源空间的zcell数量
* @param   zreg_num        分配给hash引擎此资源空间的zreg数量
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wcl      @date  2015/05/15
************************************************************/
DPP_STATUS dpp_hash_bulk_init(DPP_SE_CFG *p_se_cfg,
                              ZXIC_UINT32 fun_id,
                              ZXIC_UINT32 bulk_id,
                              DPP_HASH_DDR_RESC_CFG_T *p_ddr_resc_cfg,
                              ZXIC_UINT32 zcell_num,
                              ZXIC_UINT32 zreg_num)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 zblk_idx = 0;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 ddr_item_num = 0;

    D_NODE *p_zblk_dn = NULL;
    D_NODE *p_zcell_dn = NULL;
    ZXIC_RB_TN *p_rb_tn_new = NULL;
    ZXIC_RB_TN *p_rb_tn_rtn = NULL;
    SE_ZBLK_CFG *p_zblk_cfg = NULL;
    SE_ZREG_CFG *p_zreg_cfg = NULL;
    HASH_DDR_CFG *p_ddr_cfg = NULL;
    HASH_DDR_CFG *p_rbkey_new = NULL;
    HASH_DDR_CFG *p_rbkey_rtn = NULL;
    SE_ZCELL_CFG *p_zcell_cfg = NULL;
    DPP_HASH_CFG *p_hash_cfg = NULL;
    FUNC_ID_INFO *p_func_info = NULL;
    SE_ITEM_CFG **p_item_array = NULL;
    DPP_HASH_BULK_ZCAM_STAT *p_bulk_zcam_mono = NULL;
    /*    D_HEAD *p_zcell_free = NULL;*/
    /*    D_HEAD *p_zblk_free = NULL;*/

    ZXIC_COMM_CHECK_POINT(p_se_cfg);
    ZXIC_COMM_CHECK_POINT(p_ddr_resc_cfg);
    ZXIC_COMM_CHECK_INDEX(p_ddr_resc_cfg->ddr_crc_sel, 0, HASH_DDR_CRC_NUM - 1);
    ZXIC_COMM_CHECK_INDEX(fun_id, DPP_HASH_ID_MIN, DPP_HASH_ID_MAX);
    ZXIC_COMM_CHECK_INDEX(bulk_id, HASH_BULK_ID_MIN, HASH_BULK_ID_MAX);
    ZXIC_COMM_CHECK_INDEX(zcell_num, 0, SE_ZBLK_NUM * SE_ZCELL_NUM);
    ZXIC_COMM_CHECK_INDEX(zreg_num, 0, SE_ZBLK_NUM * SE_ZREG_NUM);

    dev_id = p_se_cfg->dev_id;

    p_func_info = DPP_GET_FUN_INFO(p_se_cfg, fun_id);
    p_hash_cfg = (DPP_HASH_CFG *)p_func_info->fun_ptr;
    ZXIC_COMM_CHECK_POINT(p_hash_cfg);

    ZXIC_COMM_TRACE_INFO("p_hash_cfg->ddr_valid = %d!\n", p_hash_cfg->ddr_valid);
    if (NULL != p_hash_cfg->hash_stat.p_bulk_zcam_mono[bulk_id])
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "slot[%d] fun_id[%u] bulk_id[%u] is already init,do not init again!\n",
                                  p_se_cfg->dev.pcie_channel.slot,fun_id,bulk_id);
        return DPP_OK;
    }

    if (1 == p_hash_cfg->ddr_valid)
    {
        ZXIC_COMM_CHECK_DEV_INDEX(dev_id, p_ddr_resc_cfg->ddr_item_num, HASH_DDR_ITEM_MIN, HASH_DDR_ITEM_MAX);

        ddr_item_num = p_ddr_resc_cfg->ddr_item_num;

        if (DDR_WIDTH_512b == p_ddr_resc_cfg->ddr_width_mode)
        {
            ddr_item_num = p_ddr_resc_cfg->ddr_item_num >> 1;
            ZXIC_COMM_CHECK_DEV_INDEX(dev_id, ddr_item_num, HASH_DDR_ITEM_MIN, HASH_DDR_ITEM_MAX);   /* 512bit存储时，hash存储深度不能少于14bit */
        }

        /** red&black key*/
        p_rbkey_new = (HASH_DDR_CFG *)ZXIC_COMM_MALLOC(sizeof(HASH_DDR_CFG));
        ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_rbkey_new);
        ZXIC_COMM_MEMSET(p_rbkey_new, 0, sizeof(HASH_DDR_CFG));

        p_rbkey_new->ddr_baddr = p_ddr_resc_cfg->ddr_baddr;

        /** red&black tree node*/
        p_rb_tn_new = (ZXIC_RB_TN *)ZXIC_COMM_MALLOC(sizeof(ZXIC_RB_TN));
        if (NULL == (p_rb_tn_new))
        {
            ZXIC_COMM_FREE(p_rbkey_new);
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ICM %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);
            return ZXIC_PAR_CHK_POINT_NULL;
        }
        INIT_RBT_TN(p_rb_tn_new, p_rbkey_new);

        rc = zxic_comm_rb_insert(&p_hash_cfg->ddr_cfg_rb, (ZXIC_VOID *)p_rb_tn_new, (ZXIC_VOID *)(&p_rb_tn_rtn));
        if (ZXIC_RBT_RC_FULL == rc)
        {
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "The red black tree is full!\n");
            ZXIC_COMM_FREE(p_rbkey_new);
            ZXIC_COMM_FREE(p_rb_tn_new);
            ZXIC_COMM_ASSERT(0);
            return DPP_HASH_RC_RB_TREE_FULL;
        }
        else if (ZXIC_RBT_RC_UPDATE == rc)
        {
            ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "some bulk_id share one bulk!\n");

            ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_rb_tn_rtn);
            p_rbkey_rtn = (HASH_DDR_CFG *)(p_rb_tn_rtn->p_key);
            ZXIC_COMM_CHECK_DEV_INDEX(dev_id, p_rbkey_rtn->bulk_id, 0, HASH_BULK_NUM - 1);
            p_ddr_cfg = p_hash_cfg->p_bulk_ddr_info[p_rbkey_rtn->bulk_id];  /* 已经初始化过的ddr_bulk_id属性 */

            if (p_ddr_cfg->hash_ddr_arg != GET_DDR_HASH_ARG(p_ddr_resc_cfg->ddr_crc_sel) ||
                p_ddr_cfg->width_mode   != p_ddr_resc_cfg->ddr_width_mode ||
                p_ddr_cfg->ddr_ecc_en   != p_ddr_resc_cfg->ddr_ecc_en     ||
                p_ddr_cfg->item_num     != ddr_item_num)
            {
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "The base address is same but other ddr attribute is different\n");
                ZXIC_COMM_FREE(p_rbkey_new);
                ZXIC_COMM_FREE(p_rb_tn_new);
                ZXIC_COMM_ASSERT(0);
                return DPP_HASH_RC_INVALID_PARA;
            }

            p_hash_cfg->p_bulk_ddr_info[bulk_id] = p_hash_cfg->p_bulk_ddr_info[p_rbkey_rtn->bulk_id];

            ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "bulk id init bulk_ddr_cfg ptr is:");

            for (i = 0; i < HASH_BULK_NUM; i++)
            {
                ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "%p ", p_hash_cfg->p_bulk_ddr_info[i]);
            }

            ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "\n");

            ZXIC_COMM_FREE(p_rbkey_new);
            ZXIC_COMM_FREE(p_rb_tn_new);
        }
        else
        {
            p_item_array = (SE_ITEM_CFG **)ZXIC_COMM_MALLOC(ddr_item_num * sizeof(SE_ITEM_CFG *));
            if (NULL == (p_item_array))
            {
                ZXIC_COMM_FREE(p_rbkey_new);
                ZXIC_COMM_FREE(p_rb_tn_new);
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ICM %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);
                return ZXIC_PAR_CHK_POINT_NULL;
            }
            ZXIC_COMM_MEMSET(p_item_array, 0, ddr_item_num * sizeof(SE_ITEM_CFG *));

            p_rbkey_new->p_item_array = p_item_array;
            p_rbkey_new->bulk_id = bulk_id;
            p_rbkey_new->hw_baddr = 0;
            p_rbkey_new->width_mode = p_ddr_resc_cfg->ddr_width_mode;
            p_rbkey_new->item_num = ddr_item_num;
            p_rbkey_new->ddr_ecc_en = p_ddr_resc_cfg->ddr_ecc_en;
            p_rbkey_new->hash_ddr_arg = GET_DDR_HASH_ARG(p_ddr_resc_cfg->ddr_crc_sel);
            p_rbkey_new->bulk_use = 1;
            p_rbkey_new->zcell_num = zcell_num;
            p_rbkey_new->zreg_num = zreg_num;
            p_hash_cfg->p_bulk_ddr_info[bulk_id] = p_rbkey_new;

            ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "one new ddr_bulk init!\n");
        }
#ifdef DPP_FLOW_HW_INIT
        rc = dpp_hash_tbl_crc_poly_write(p_hash_cfg->p_se_info, p_hash_cfg->fun_id, bulk_id, p_ddr_resc_cfg->ddr_crc_sel);
        if (DPP_OK != rc)
        {
            ZXIC_COMM_FREE(p_rbkey_new);
            ZXIC_COMM_FREE(p_rb_tn_new);
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ICM  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,"dpp_hash_tbl_crc_poly_write");
            return rc;
        }

        rc = dpp_hash_ext_cfg_write(p_hash_cfg->p_se_info, p_hash_cfg->fun_id, bulk_id, p_hash_cfg->p_bulk_ddr_info[bulk_id]);
        if (DPP_OK != rc)
        {
            ZXIC_COMM_FREE(p_rbkey_new);
            ZXIC_COMM_FREE(p_rb_tn_new);
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ICM  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,"dpp_hash_ext_cfg_write");
            return rc;
        }
        rc = dpp_hash_tbl_depth_write(p_hash_cfg->p_se_info,
                                      p_hash_cfg->fun_id,
                                      bulk_id,
                                      p_hash_cfg->p_bulk_ddr_info[bulk_id]);
        if (DPP_OK != rc)
        {
            ZXIC_COMM_FREE(p_rbkey_new);
            ZXIC_COMM_FREE(p_rb_tn_new);
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ICM  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,"dpp_hash_tbl_depth_write");
            return rc;
        }
#endif
        rc = dpp_se_smmu1_hash_tbl_cfg_set(&p_se_cfg->dev, p_hash_cfg->fun_id, bulk_id, p_ddr_resc_cfg->ddr_ecc_en, p_ddr_resc_cfg->ddr_baddr);
        if (DPP_OK != rc)
        {
            ZXIC_COMM_FREE(p_rbkey_new);
            ZXIC_COMM_FREE(p_rb_tn_new);
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ICM  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,"dpp_se_smmu1_hash_tbl_cfg_set");
            return rc;
        }

    }

    p_bulk_zcam_mono = (DPP_HASH_BULK_ZCAM_STAT *)ZXIC_COMM_MALLOC(sizeof(DPP_HASH_BULK_ZCAM_STAT));
    if (NULL == (p_bulk_zcam_mono))
    {
        ZXIC_COMM_FREE(p_rbkey_new);
        ZXIC_COMM_FREE(p_rb_tn_new);
        ZXIC_COMM_FREE(p_item_array);
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ICM %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);
        return ZXIC_PAR_CHK_POINT_NULL;
    }
    ZXIC_COMM_MEMSET(p_bulk_zcam_mono, 0, sizeof(DPP_HASH_BULK_ZCAM_STAT));
    (&p_hash_cfg->hash_stat)->p_bulk_zcam_mono[bulk_id] = p_bulk_zcam_mono;

    for (i = 0; i < SE_ZBLK_NUM * SE_ZCELL_NUM; i++)
    {
        p_bulk_zcam_mono->zcell_mono_idx[i] = 0xffffffff;
    }

    for (i = 0; i < SE_ZBLK_NUM; i++)
    {
        for (j = 0; j < SE_ZREG_NUM; j++)
        {
            p_bulk_zcam_mono->zreg_mono_id[i][j].zblk_id = 0xffffffff;
            p_bulk_zcam_mono->zreg_mono_id[i][j].zreg_id = 0xffffffff;
        }
    }

    if (zcell_num > 0)
    {
        p_hash_cfg->bulk_ram_mono[bulk_id] = 1;

#ifdef DPP_FLOW_HW_INIT
        rc = dpp_hash_bulk_mono_flags_write(p_se_cfg, fun_id, bulk_id);
        if (DPP_OK != rc)
        {
            ZXIC_COMM_FREE(p_rbkey_new);
            ZXIC_COMM_FREE(p_rb_tn_new);
            ZXIC_COMM_FREE(p_item_array);
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ICM  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,"dpp_hash_bulk_mono_flags_write");
            return rc;
        }
#endif

        /* 初始分配此业务表独占的zcell，只能在非独占的zcell中分配 */
        /* p_zcell_free = &(p_hash_cfg->hash_shareram.zcell_free_list);*/
        p_zcell_dn = p_hash_cfg->hash_shareram.zcell_free_list.p_next;

        i = 0;

        while (p_zcell_dn)
        {
            p_zcell_cfg = (SE_ZCELL_CFG *)p_zcell_dn->data;

            if (p_zcell_cfg->is_used)
            {
                if (!(p_zcell_cfg->flag & DPP_ZCELL_FLAG_IS_MONO))
                {
                    p_zcell_cfg->flag     |= DPP_ZCELL_FLAG_IS_MONO;
                    p_zcell_cfg->bulk_id = bulk_id;

#ifdef DPP_FLOW_HW_INIT
                    rc = dpp_hash_zcell_mono_write(p_se_cfg, p_zcell_cfg);   /* 写入硬件 */
                    if (DPP_OK != rc)
                    {
                        ZXIC_COMM_FREE(p_rbkey_new);
                        ZXIC_COMM_FREE(p_rb_tn_new);
                        ZXIC_COMM_FREE(p_item_array);
                        ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ICM  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,"dpp_hash_zcell_mono_write");
                        return rc;
                    }
#endif

                    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, p_zcell_cfg->zcell_idx, 0, SE_ZCELL_TOTAL_NUM - 1);
                    p_bulk_zcam_mono->zcell_mono_idx[p_zcell_cfg->zcell_idx] = p_zcell_cfg->zcell_idx;  /* 统计独占的zcell */

                    if (++i >= zcell_num)
                    {
                        break;
                    }
                }
            }
            else
            {
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "zcell[ %d ] is not init before used!\n", p_zcell_cfg->zcell_idx);
                ZXIC_COMM_ASSERT(0);
                return DPP_HASH_RC_INVALID_PARA;
            }

            p_zcell_dn = p_zcell_dn->next;
        }

        if (i < zcell_num)
        {
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "Input param 'zcell_num' is [ %d ], actually bulk[ %d ]monopolize zcells is [ %d ]!\n", zcell_num, bulk_id, i);
        }
    }

    if (zreg_num > 0)
    {
        p_hash_cfg->bulk_ram_mono[bulk_id] = 1;

#ifdef DPP_FLOW_HW_INIT
        rc = dpp_hash_bulk_mono_flags_write(p_se_cfg, fun_id, bulk_id);
        if (DPP_OK != rc)
        {
            ZXIC_COMM_FREE(p_rbkey_new);
            ZXIC_COMM_FREE(p_rb_tn_new);
            ZXIC_COMM_FREE(p_item_array);
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ICM  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,"dpp_hash_bulk_mono_flags_write");
            return rc;
        }
#endif
        /*        p_tbl_id_info->zreg_num= zreg_num;*/

        /* 初始分配此业务表独占的zreg，只能在非独占的zreg中分配 */
        /*        p_zblk_free = &(p_hash_cfg->hash_shareram.zblk_list);*/
        p_zblk_dn = p_hash_cfg->hash_shareram.zblk_list.p_next;
        j = 0;

        while (p_zblk_dn)
        {
            p_zblk_cfg = (SE_ZBLK_CFG *)p_zblk_dn->data;
            zblk_idx = p_zblk_cfg->zblk_idx;
            ZXIC_COMM_CHECK_DEV_INDEX(dev_id, zblk_idx, 0, SE_ZBLK_NUM - 1);

            if (p_zblk_cfg->is_used)
            {
                for (i = 0; i < SE_ZREG_NUM; i++)
                {
                    p_zreg_cfg = &(p_zblk_cfg->zreg_info[i]);

                    if (!(p_zreg_cfg->flag & DPP_ZREG_FLAG_IS_MONO))
                    {
                        p_zreg_cfg->flag = DPP_ZREG_FLAG_IS_MONO;
                        p_zreg_cfg->bulk_id = bulk_id;
#ifdef DPP_FLOW_HW_INIT
                        rc = dpp_hash_zreg_mono_write(p_se_cfg, bulk_id, zblk_idx, i); /* 写入硬件 */
                        if (DPP_OK != rc)
                        {
                            ZXIC_COMM_FREE(p_rbkey_new);
                            ZXIC_COMM_FREE(p_rb_tn_new);
                            ZXIC_COMM_FREE(p_item_array);
                            ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ICM  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,"dpp_hash_zreg_mono_write");
                            return rc;
                        }
#endif
                        p_bulk_zcam_mono->zreg_mono_id[zblk_idx][i].zblk_id = zblk_idx;
                        p_bulk_zcam_mono->zreg_mono_id[zblk_idx][i].zreg_id = i;

                        if (++j >= zreg_num)
                        {
                            break;
                        }
                    }
                }

                if (j >= zreg_num)
                {
                    break;
                }
            }
            else
            {
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "zblk [ %d ] is not init before used!\n", p_zblk_cfg->zblk_idx);
                ZXIC_COMM_ASSERT(0);
                return DPP_HASH_RC_INVALID_PARA;
            }

            p_zblk_dn = p_zblk_dn->next;
        }

        if (j < zreg_num)
        {
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "Input param 'zreg_num' is [ %d ], actually bulk[ %d ]monopolize zregs is [ %d ]!\n", zreg_num, bulk_id, j);
        }
    }

    /** for 软复位 */
    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id,DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(fun_id, 0, 3);
    ZXIC_COMM_CHECK_INDEX(bulk_id, 0, 7);
    g_hash_store_dat[dev_id].ddr_item_num[fun_id][bulk_id] = p_ddr_resc_cfg->ddr_item_num;

    return DPP_OK;
}

/***********************************************************/
/** 初始化单个hash引擎内的某个业务表，此接口支持为该业务表分配独占的zcell。
*   必须先初始化hash引擎，如果是片内+片外模式还必须先初始化dpp_hash_ddr_bulk_init，
*   再初始化业务表。
* @param   p_se_cfg        算法模块公共管理数据结构指针
* @param   fun_id          hash引擎号
* @param   tbl_id          业务表
* @param   tbl_flag        初始化标记, bitmap的形式使用，如:HASH_TBL_FLAG_AGE等
* @param   key_type        hash条目类型，取值参照DPP_HASH_KEY_TYPE的定义
* @param   actu_key_size   业务键值有效长度: 8bit*N，N=1~48
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wcl      @date  2015/05/15
************************************************************/
DPP_STATUS dpp_hash_tbl_id_info_init(DPP_SE_CFG *p_se_cfg,
                                     ZXIC_UINT32 fun_id,
                                     ZXIC_UINT32 tbl_id,
                                     ZXIC_UINT32 tbl_flag,
                                     ZXIC_UINT32 key_type,
                                     ZXIC_UINT32 actu_key_size)
{
    ZXIC_UINT32 key_by_size = 0;
    ZXIC_UINT32 entry_size = 0;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 slot_id = 0;

    DPP_HASH_TBL_ID_INFO *p_tbl_id_info = NULL;

    ZXIC_COMM_CHECK_INDEX(tbl_id, 0, HASH_TBL_ID_NUM - 1);
    ZXIC_COMM_CHECK_INDEX(fun_id, HASH_FUNC_ID_MIN, HASH_FUNC_ID_NUM - 1);
    ZXIC_COMM_CHECK_INDEX(actu_key_size, HASH_ACTU_KEY_MIN, HASH_ACTU_KEY_MAX);
    ZXIC_COMM_CHECK_INDEX(key_type, HASH_KEY_128b, HASH_KEY_512b);
    ZXIC_COMM_CHECK_POINT(p_se_cfg);

    dev_id = p_se_cfg->dev_id;
    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id,DPP_DEV_CHANNEL_MAX - 1);
    key_by_size = DPP_GET_KEY_SIZE(actu_key_size);
    entry_size = DPP_GET_HASH_ENTRY_SIZE(key_type);

    if (key_by_size > entry_size)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ErrorCode[%x]: actu_key_size[%d] not match to key_type[%d].\n",
                                 DPP_HASH_RC_INVALID_PARA,
                                 key_by_size,
                                 entry_size);
        ZXIC_COMM_ASSERT(0);
        return DPP_HASH_RC_INVALID_PARA;
    }

    slot_id = p_se_cfg->dev.pcie_channel.slot;
    ZXIC_COMM_CHECK_INDEX_UPPER(slot_id,DPP_PCIE_SLOT_MAX - 1);
    p_tbl_id_info = GET_HASH_TBL_ID_INFO(slot_id, fun_id, tbl_id);

    if (p_tbl_id_info->is_init)
    {
        ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "slot[%d] fun_id[%d],table_id[%d] is already init, do not init again!\n", 
                                    slot_id, fun_id, tbl_id);
        //return DPP_HASH_RC_REPEAT_INIT;
        return DPP_OK;
    }

    p_tbl_id_info->fun_id = fun_id;
    p_tbl_id_info->actu_key_size = actu_key_size;
    p_tbl_id_info->key_type = key_type;
    p_tbl_id_info->is_init = 1;

    if (tbl_flag & HASH_TBL_FLAG_AGE)
    {
        p_tbl_id_info->is_age = 1;
    }

    if (tbl_flag & HASH_TBL_FLAG_LEARN)
    {
        p_tbl_id_info->is_lrn = 1;
    }

    if (tbl_flag & HASH_TBL_FLAG_MC_WRT)
    {
        p_tbl_id_info->is_mc_wrt = 1;
    }

    return DPP_OK;
}

DPP_STATUS dpp_hash_red_black_node_alloc(DPP_DEV_T *dev,ZXIC_RB_TN **p_rb_tn_new,DPP_HASH_RBKEY_INFO **p_rbkey_new)
{
    ZXIC_RB_TN *p_rb_tn_new_temp = NULL;
    DPP_HASH_RBKEY_INFO *p_rbkey_new_temp = NULL;

    ZXIC_COMM_CHECK_POINT(dev);

    /** red&black key*/
    p_rbkey_new_temp = (DPP_HASH_RBKEY_INFO *)ZXIC_COMM_MALLOC(sizeof(DPP_HASH_RBKEY_INFO));
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_rbkey_new_temp);
    ZXIC_COMM_MEMSET(p_rbkey_new_temp, 0, sizeof(DPP_HASH_RBKEY_INFO));
    
    /** red&black tree node*/
    p_rb_tn_new_temp = (ZXIC_RB_TN *)ZXIC_COMM_MALLOC(sizeof(ZXIC_RB_TN));
    if (NULL == p_rb_tn_new_temp)
    {
        ZXIC_COMM_FREE(p_rbkey_new_temp);
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev),"\n ICM %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);
        return ZXIC_PAR_CHK_POINT_NULL;
    }

    INIT_RBT_TN(p_rb_tn_new_temp, p_rbkey_new_temp);

    *p_rb_tn_new = p_rb_tn_new_temp;
    *p_rbkey_new = p_rbkey_new_temp;

    return DPP_OK;
}

DPP_STATUS dpp_hash_get_hash_info_from_sdt(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no, HASH_ENTRY_CFG *p_hash_entry_cfg)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    FUNC_ID_INFO *p_func_info = NULL;
    DPP_SE_CFG *p_se_cfg = NULL;

    DPP_SDTTBL_HASH_T sdt_hash_info = {0};  /*SDT内容*/
    
    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_hash_entry_cfg);

    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_hash_info);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_soft_sdt_tbl_read");

    p_hash_entry_cfg->fun_id   = sdt_hash_info.hash_id; /*hash引擎*/
    ZXIC_COMM_CHECK_INDEX(p_hash_entry_cfg->fun_id, HASH_FUNC_ID_MIN, HASH_FUNC_ID_NUM - 1);

    p_hash_entry_cfg->table_id = sdt_hash_info.hash_table_id; /*hash表号*/
    ZXIC_COMM_CHECK_INDEX_UPPER(p_hash_entry_cfg->table_id,HASH_TBL_ID_NUM - 1);

    p_hash_entry_cfg->bulk_id = ((p_hash_entry_cfg->table_id >> 2) & 0x7);
    ZXIC_COMM_CHECK_INDEX_UPPER(p_hash_entry_cfg->bulk_id,HASH_BULK_NUM - 1);

    p_hash_entry_cfg->key_type = sdt_hash_info.hash_table_width; /*表宽度*/
    ZXIC_COMM_CHECK_INDEX(p_hash_entry_cfg->key_type, HASH_KEY_128b, HASH_KEY_512b);

    p_hash_entry_cfg->actu_key_size = sdt_hash_info.key_size; /*业务表键值长度*/
    ZXIC_COMM_CHECK_INDEX(p_hash_entry_cfg->actu_key_size, HASH_ACTU_KEY_MIN, HASH_ACTU_KEY_MAX);
    p_hash_entry_cfg->key_by_size = DPP_GET_KEY_SIZE(p_hash_entry_cfg->actu_key_size);
    p_hash_entry_cfg->rst_by_size = DPP_GET_RST_SIZE(p_hash_entry_cfg->key_type, p_hash_entry_cfg->actu_key_size);

     /* 取出se配置 */
    rc = dpp_se_cfg_get(dev, &p_se_cfg);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_se_cfg_get");
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_se_cfg);
    p_hash_entry_cfg->p_se_cfg = p_se_cfg;

    p_func_info = DPP_GET_FUN_INFO(p_se_cfg, p_hash_entry_cfg->fun_id);
    DPP_SE_CHECK_FUN(p_func_info, p_hash_entry_cfg->fun_id, FUN_HASH);

    p_hash_entry_cfg->p_hash_cfg = (DPP_HASH_CFG *)p_func_info->fun_ptr;
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_hash_entry_cfg->p_hash_cfg);
    
    return DPP_OK;
}

DPP_STATUS dpp_hash_rb_insert(DPP_DEV_T *dev,HASH_ENTRY_CFG *p_hash_entry_cfg,DPP_HASH_ENTRY *p_entry)
{
    ZXIC_UINT32         rc              = DPP_OK;
    DPP_HASH_RBKEY_INFO *p_rbkey_rtn    = NULL;
    DPP_HASH_CFG        *p_hash_cfg     = NULL;
    DPP_HASH_RBKEY_INFO *p_rbkey_new    = NULL;
    ZXIC_RB_TN          *p_rb_tn_new    = NULL;
    ZXIC_RB_TN          *p_rb_tn_rtn    = NULL;
    ZXIC_UINT32         rst_actual_size = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_hash_entry_cfg);
    ZXIC_COMM_CHECK_POINT(p_hash_entry_cfg->p_hash_cfg);
    ZXIC_COMM_CHECK_POINT(p_entry);
    ZXIC_COMM_CHECK_POINT(p_entry->p_rst);
    

    p_rbkey_new = p_hash_entry_cfg->p_rbkey_new;
    p_rb_tn_new = p_hash_entry_cfg->p_rb_tn_new;
    p_hash_cfg = p_hash_entry_cfg->p_hash_cfg;
    rst_actual_size = ((p_hash_entry_cfg->rst_by_size) > HASH_RST_MAX) ? HASH_RST_MAX : p_hash_entry_cfg->rst_by_size;
    rc = zxic_comm_rb_insert(&p_hash_cfg->hash_rb, (ZXIC_VOID *)p_rb_tn_new, (ZXIC_VOID *)(&p_rb_tn_rtn));
    if (ZXIC_RBT_RC_FULL == rc)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "The red black tree is full!\n");
        ZXIC_COMM_FREE(p_rbkey_new);
        ZXIC_COMM_FREE(p_rb_tn_new);
        ZXIC_COMM_ASSERT(0);
        return DPP_HASH_RC_RB_TREE_FULL;
    }
    else if (ZXIC_RBT_RC_UPDATE == rc)
    {
        p_hash_cfg->hash_stat.insert_same++;
        ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), "Hash update exist entry!\n");

        ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_rb_tn_rtn);
        p_rbkey_rtn = (DPP_HASH_RBKEY_INFO *)(p_rb_tn_rtn->p_key);

        /* when the result is more than 256bit, get first 256bit valid data. */
        ZXIC_COMM_MEMCPY(p_rbkey_rtn->rst, p_entry->p_rst, rst_actual_size);

        ZXIC_COMM_FREE(p_rbkey_new);
        ZXIC_COMM_FREE(p_rb_tn_new);
        p_hash_entry_cfg->p_rbkey_new = p_rbkey_rtn;
        p_hash_entry_cfg->p_rb_tn_new = p_rb_tn_rtn;
        ZXIC_COMM_CHECK_INDEX_UPPER(p_hash_entry_cfg->table_id,HASH_TBL_ID_NUM - 1);
        p_hash_cfg->hash_stat.insert_table[p_hash_entry_cfg->table_id].same++;

        return DPP_HASH_RC_ADD_UPDATE;
    }
    else
    {
        ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), "Hash insert new entry!\n");
        /* when the result is more than 256bit, get first 256bit valid data. */
        ZXIC_COMM_MEMCPY(p_rbkey_new->rst, p_entry->p_rst, rst_actual_size);
        p_rbkey_new->entry_size = DPP_GET_HASH_ENTRY_SIZE(p_hash_entry_cfg->key_type);
        INIT_D_NODE(&p_rbkey_new->entry_dn, p_rbkey_new);
    }

    return DPP_OK;
}

DPP_STATUS dpp_hash_set_crc_key(DPP_DEV_T *dev,HASH_ENTRY_CFG *p_hash_entry_cfg,DPP_HASH_ENTRY *p_entry,ZXIC_UINT8 *p_temp_key)
{
    ZXIC_UINT32 key_by_size = 0;    
    ZXIC_UINT8 temp_tbl_id  = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_hash_entry_cfg);
    ZXIC_COMM_CHECK_POINT(p_entry);
    ZXIC_COMM_CHECK_POINT(p_entry->p_key);
    ZXIC_COMM_CHECK_POINT(p_temp_key);

    key_by_size = p_hash_entry_cfg->key_by_size;
    ZXIC_COMM_MEMCPY(p_temp_key, p_entry->p_key, key_by_size);

    /* 约定: 参与CRC运算的key, 需要将tbl_id的前面补3bit的0，然后放到actu_key的后面. */
    temp_tbl_id = (*p_temp_key) & 0x1F;
    memmove(p_temp_key, p_temp_key+1, key_by_size - HASH_KEY_CTR_SIZE);
    ZXIC_COMM_CHECK_DEV_INDEX_SUB_OVERFLOW_NO_ASSERT(DEV_ID(dev), key_by_size, HASH_KEY_CTR_SIZE);
    p_temp_key[key_by_size - HASH_KEY_CTR_SIZE] = temp_tbl_id;

    return DPP_OK;
}

DPP_STATUS dpp_hash_insert_ddr(DPP_DEV_T *dev,HASH_ENTRY_CFG *p_hash_entry_cfg,ZXIC_UINT8 *p_temp_key,ZXIC_UINT8 *p_end_flag)
{
    DPP_HASH_CFG        *p_hash_cfg   = NULL;
    ZXIC_UINT8           bulk_id      = 0;
    ZXIC_UINT8           key_type     = 0;
    ZXIC_UINT8           table_id     = 0;
    ZXIC_UINT32          key_by_size  = 0;
    ZXIC_UINT32          crc_value    = 0;
    ZXIC_UINT32          item_idx     = 0xFFFFFFFF;/* -1 */
    ZXIC_UINT32          item_type    = 0;
    HASH_DDR_CFG         *p_ddr_cfg   = NULL;
    SE_ITEM_CFG          *p_item      = NULL;
    DPP_HASH_RBKEY_INFO  *p_rbkey_new = NULL;
    ZXIC_UINT32          rc           = DPP_OK;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_hash_entry_cfg);
    ZXIC_COMM_CHECK_POINT(p_temp_key);
    ZXIC_COMM_CHECK_POINT(p_end_flag);

    p_hash_cfg = p_hash_entry_cfg->p_hash_cfg;
    ZXIC_COMM_CHECK_POINT(p_hash_cfg);
    bulk_id = p_hash_entry_cfg->bulk_id;
    ZXIC_COMM_CHECK_INDEX_UPPER(bulk_id,HASH_BULK_NUM - 1);
    p_ddr_cfg = p_hash_cfg->p_bulk_ddr_info[bulk_id];
    ZXIC_COMM_CHECK_POINT(p_ddr_cfg);
    table_id = p_hash_entry_cfg->table_id;
    ZXIC_COMM_CHECK_INDEX_UPPER(table_id,HASH_TBL_ID_NUM - 1);
    p_rbkey_new = p_hash_entry_cfg->p_rbkey_new;
    ZXIC_COMM_CHECK_POINT(p_rbkey_new);

    key_type = p_hash_entry_cfg->key_type;
    if ((HASH_KEY_512b == key_type) && (DDR_WIDTH_256b == p_ddr_cfg->width_mode))
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "ErrorCode[0x%x]: Hash DDR width mode is not match to the key type.\n", DPP_HASH_RC_DDR_WIDTH_MODE_ERR);
        return DPP_HASH_RC_DDR_WIDTH_MODE_ERR;
    }
    
    key_by_size = p_hash_entry_cfg->key_by_size;
    crc_value = p_hash_cfg->p_hash32_fun(p_temp_key, key_by_size, p_ddr_cfg->hash_ddr_arg);
    ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), "hash ddr arg is: 0x%x.crc_value is 0x%x\n", p_ddr_cfg->hash_ddr_arg, crc_value); /* t */
    item_idx = crc_value % p_ddr_cfg->item_num; /*按照256bit进行划分*/
    item_type = ITEM_DDR_256;
    if (DDR_WIDTH_512b == p_ddr_cfg->width_mode)
    {
        item_idx = crc_value % p_ddr_cfg->item_num;/* 512b模式时，item_num已经在bulk初始化做过转换 ，按照512bit进行划分*/
        item_type = ITEM_DDR_512;
    }

    ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), "Hash insert in ITEM_DDR_%s, item_idx is: 0x%x.\n", ((item_type == ITEM_DDR_256) ? "256" : "512"), item_idx);

    ZXIC_COMM_CHECK_INDEX_UPPER(item_idx, p_ddr_cfg->item_num);    /* modify coverity yinxh 2021.03.10*/
    p_item = p_ddr_cfg->p_item_array[item_idx];
    if (NULL == p_item)
    {
        p_item = (SE_ITEM_CFG *)ZXIC_COMM_MALLOC(sizeof(SE_ITEM_CFG));
        ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_item);
        ZXIC_COMM_MEMSET(p_item, 0, sizeof(SE_ITEM_CFG));
        p_ddr_cfg->p_item_array[item_idx] = p_item;
    }

    rc = dpp_hash_insrt_to_item(p_hash_cfg,
                                    p_hash_entry_cfg->p_rbkey_new,
                                    p_item,
                                    item_idx,
                                    item_type,
                                    key_type);

    if (DPP_HASH_RC_ITEM_FULL != rc)
    {
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_hash_insrt_to_item");
        *p_end_flag = 1;
#ifdef HASH_ENTRY_STAT
        p_hash_cfg->hash_stat.insert_ddr++;
        p_hash_cfg->hash_stat.insert_table[table_id].ddr++;
#endif
        /* calc the item hardware address in DDR */
        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev), p_ddr_cfg->hw_baddr, item_idx);
        p_item->hw_addr = GET_HASH_DDR_HW_ADDR(p_ddr_cfg->hw_baddr, item_idx);
        p_item->bulk_id = p_hash_entry_cfg->bulk_id;
    }

#if OBTAIN_CONFLICT_KEY
    /* debug start 获取冲突键值*/
    if (DPP_HASH_RC_ITEM_FULL == rc)
    {
        p_item->hw_addr = GET_HASH_DDR_HW_ADDR(p_ddr_cfg->hw_baddr, item_idx);
        ZXIC_COMM_PRINT("ddr conflict item_idx is:0x%x,p_item->hw_addr is:0x%x\n", item_idx, p_item->hw_addr);
        ZXIC_COMM_PRINT("ddr conflict key is:");

        for (index = 0; index < 10; index++)
        {
            ZXIC_COMM_PRINT("0x%02x ", p_rbkey_new->key[index]);
        }

        ZXIC_COMM_PRINT("\n");
    }

    /*        debug end*/
#endif

    return DPP_OK;
}

DPP_STATUS dpp_hash_insert_zcell(DPP_DEV_T *dev,DPP_SE_CFG *p_se_cfg,HASH_ENTRY_CFG *p_hash_entry_cfg,ZXIC_UINT8 *p_temp_key,ZXIC_UINT8 *p_end_flag)
{
    ZXIC_UINT8   bulk_id      = 0;
    D_NODE       *p_zcell_dn  = NULL;
    SE_ZCELL_CFG *p_zcell     = NULL;
    ZXIC_UINT32  zblk_idx     = 0;
    ZXIC_UINT32  zcell_id     = 0;
    ZXIC_UINT32  pre_zblk_idx = 0xFFFFFFFF;/* -1; */
    SE_ITEM_CFG *p_item       = NULL;
    ZXIC_UINT32  item_idx     = 0xFFFFFFFF;/* -1 */
    ZXIC_UINT32  item_type    = 0;
    ZXIC_UINT32  rc           = DPP_OK;
    ZXIC_UINT32  crc_value    = 0;
    ZXIC_UINT8   table_id     = 0;
    DPP_HASH_CFG *p_hash_cfg  = NULL;
    SE_ZBLK_CFG  *p_zblk      = NULL;
    
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_hash_entry_cfg);
    ZXIC_COMM_CHECK_POINT(p_temp_key);
    ZXIC_COMM_CHECK_POINT(p_se_cfg);
    ZXIC_COMM_CHECK_POINT(p_end_flag);

    /* if insert into DDR is fail, insert into ZCAM. */
    ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), "insert zcell start\n");  
    bulk_id = p_hash_entry_cfg->bulk_id;
    ZXIC_COMM_CHECK_INDEX_UPPER(bulk_id,HASH_BULK_NUM - 1);
    table_id = p_hash_entry_cfg->table_id;
    ZXIC_COMM_CHECK_INDEX_UPPER(table_id,HASH_TBL_ID_NUM - 1);
    p_hash_cfg = p_hash_entry_cfg->p_hash_cfg;
    ZXIC_COMM_CHECK_POINT(p_hash_cfg);
    p_zcell_dn = p_hash_cfg->hash_shareram.zcell_free_list.p_next;
    
    while (p_zcell_dn)
    {
        p_zcell = (SE_ZCELL_CFG *)p_zcell_dn->data;
        ZXIC_COMM_CHECK_POINT(p_zcell);

        /* 只在hash引擎开启zcell独占模式下，才进行相关的处理 */
        if (((p_zcell->flag & DPP_ZCELL_FLAG_IS_MONO) && (p_zcell->bulk_id != bulk_id)) 
             ||((!(p_zcell->flag & DPP_ZCELL_FLAG_IS_MONO)) && (p_hash_cfg->bulk_ram_mono[bulk_id])))
        {
            p_zcell_dn = p_zcell_dn->next;
            continue;
        }

        zblk_idx = GET_ZBLK_IDX(p_zcell->zcell_idx);
        ZXIC_COMM_CHECK_INDEX_UPPER(zblk_idx,SE_ZBLK_NUM - 1);
        p_zblk = &(p_se_cfg->zblk_info[zblk_idx]);
        if (zblk_idx != pre_zblk_idx)
        {
            pre_zblk_idx = zblk_idx;
            crc_value = p_hash_cfg->p_hash16_fun(p_temp_key, p_hash_entry_cfg->key_by_size, p_zblk->hash_arg);
        }

        ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), "zblk_idx is [0x%x],p_zblk->hash_arg is [0x%x],crc_value is [0x%x]\n", zblk_idx, p_zblk->hash_arg, crc_value); /* t */

        zcell_id = GET_ZCELL_IDX(p_zcell->zcell_idx);
        item_idx = GET_ZCELL_CRC_VAL(zcell_id, crc_value);
        ZXIC_COMM_CHECK_INDEX_UPPER(item_idx,SE_RAM_DEPTH - 1);
        p_item = &(p_zcell->item_info[item_idx]);
        item_type = ITEM_RAM;

        ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), "zcell_id is [0x%x],item_idx is [0x%x]\n", zcell_id, item_idx); /* t */

        rc = dpp_hash_insrt_to_item(p_hash_cfg,
                                        p_hash_entry_cfg->p_rbkey_new,
                                        p_item,
                                        item_idx,
                                        item_type,
                                        p_hash_entry_cfg->key_type);

        if (DPP_HASH_RC_ITEM_FULL == rc)
        {
            ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), "The item is full, check next. \n");
#if OBTAIN_CONFLICT_KEY

            /*                debug start 获取冲突键值*/
            if (DPP_HASH_RC_ITEM_FULL == rc)
            {
                p_item->hw_addr = ZBLK_ITEM_ADDR_CALC(p_zcell->zcell_idx, item_idx);
                ZXIC_COMM_PRINT("zcell conflict item_idx is:0x%x,p_item->hw_addr is:0x%x\n", item_idx, p_item->hw_addr);
                ZXIC_COMM_PRINT("zcell conflict key is:");

                for (index = 0; index < 10; index++)
                {
                    ZXIC_COMM_PRINT("0x%x ", p_rbkey_new->key[index]);
                }

                ZXIC_COMM_PRINT("\n");
            }

            /*                debug end*/
#endif
        }
        else
        {
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_hash_insrt_to_item");
            *p_end_flag = 1;
#ifdef HASH_ENTRY_STAT
            p_hash_cfg->hash_stat.insert_zcell++;
            p_hash_cfg->hash_stat.insert_table[table_id].zcell++;
#endif

            /* calc the item hardware address in ZCAM. */
            p_item->hw_addr = ZBLK_ITEM_ADDR_CALC(p_zcell->zcell_idx, item_idx);
            /*                debug start 获取冲突键值*/
            /*                ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), "ZCAM hw_addr is [0x%x]\n", p_item->hw_addr); t*/
            /*                ZXIC_COMM_PRINT("after ddr conflict item_idx is:%d,p_item->hw_addr is:%d \n", item_idx, p_item->hw_addr);*/
            /*                ZXIC_COMM_PRINT("after ddr conflict zcell_id is [0x%x],zblk_idx is [0x%x]\n", zcell_id, zblk_idx); t;*/
            /*                debug end*/
            break;
        }

        p_zcell_dn = p_zcell_dn->next;
    }
    
    return DPP_OK;
}

DPP_STATUS dpp_hash_insert_zreg(DPP_DEV_T *dev,HASH_ENTRY_CFG *p_hash_entry_cfg,ZXIC_UINT8 *p_temp_key,ZXIC_UINT8 *p_end_flag)
{
    DPP_HASH_CFG *p_hash_cfg  = NULL;
    D_NODE       *p_zblk_dn   = NULL;
    SE_ZBLK_CFG  *p_zblk      = NULL;
    SE_ZREG_CFG  *p_zreg      = NULL;
    SE_ITEM_CFG  *p_item      = NULL;
    ZXIC_UINT8   reg_index    = 0;
    ZXIC_UINT32  zblk_idx     = 0;
    ZXIC_UINT32  rc           = DPP_OK;
    ZXIC_UINT8   bulk_id      = 0;
    ZXIC_UINT32  item_idx     = 0xFFFFFFFF;/* -1; */
    ZXIC_UINT32  item_type    = 0;
    ZXIC_UINT32  table_id     = 0;
    DPP_HASH_RBKEY_INFO *p_rbkey_new = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_hash_entry_cfg);
    ZXIC_COMM_CHECK_POINT(p_temp_key);
    ZXIC_COMM_CHECK_POINT(p_end_flag);

    ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), "insert zreg start\n");  /* t */
    bulk_id = p_hash_entry_cfg->bulk_id;
    ZXIC_COMM_CHECK_INDEX_UPPER(bulk_id,HASH_BULK_NUM - 1);
    table_id = p_hash_entry_cfg->table_id;
    ZXIC_COMM_CHECK_INDEX_UPPER(table_id,HASH_TBL_ID_NUM - 1);
    p_rbkey_new = p_hash_entry_cfg->p_rbkey_new;
    ZXIC_COMM_CHECK_POINT(p_rbkey_new);

    p_hash_cfg = p_hash_entry_cfg->p_hash_cfg;
    p_zblk_dn = p_hash_cfg->hash_shareram.zblk_list.p_next;
    while (p_zblk_dn)
    {
        p_zblk = (SE_ZBLK_CFG *)p_zblk_dn->data;
        zblk_idx = p_zblk->zblk_idx;

        for (reg_index = 0; reg_index < SE_ZREG_NUM; reg_index++)
        {
            p_zreg = &(p_zblk->zreg_info[reg_index]);

            if (((p_zreg->flag & DPP_ZREG_FLAG_IS_MONO) && (p_zreg->bulk_id != bulk_id)) ||
                ((!(p_zreg->flag & DPP_ZREG_FLAG_IS_MONO)) && (p_hash_cfg->bulk_ram_mono[bulk_id])))
            {
                continue;
            }

            p_item = &(p_zblk->zreg_info[reg_index].item_info);
            item_type = ITEM_REG;
            item_idx = reg_index;
            rc = dpp_hash_insrt_to_item(p_hash_cfg,
                                        p_hash_entry_cfg->p_rbkey_new,
                                        p_item,
                                        item_idx,
                                        item_type,
                                        p_hash_entry_cfg->key_type);

            if (DPP_HASH_RC_ITEM_FULL == rc)
            {
                ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), "The item is full, check next. \n");
#if OBTAIN_CONFLICT_KEY
                /*                    debug start 获取冲突键值*/
                ZXIC_COMM_PRINT("zreg_mono conflict,not inserted,reg_num is:0x%x,zblk_idx is:0x%x,p_item->hw_addr is:0x%x\n", reg_index, zblk_idx, p_item->hw_addr);
                ZXIC_COMM_PRINT("zreg_mono conflict,key is:");

                for (index = 0; index < 8; index++)
                {
                    ZXIC_COMM_PRINT("0x%x ", p_rbkey_new->key[index]);
                }

                ZXIC_COMM_PRINT("\n");
                /*                    debug end*/
#endif
            }
            else
            {
                ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_hash_insrt_to_item");
                *p_end_flag = 1;
#ifdef HASH_ENTRY_STAT
                p_hash_cfg->hash_stat.insert_zreg++;
                p_hash_cfg->hash_stat.insert_table[table_id].zreg++;
#endif

                /* calc the item hardware address in ZBLK Reg. */
                p_item->hw_addr = ZBLK_HASH_LIST_REG_ADDR_CALC(zblk_idx, reg_index);
#if OBTAIN_CONFLICT_KEY
                /*                    debug start 获取冲突键值*/
                ZXIC_COMM_PRINT("zreg_mono conflict,inserted,reg_num is:0x%x,zblk_idx is:0x%x,p_item->hw_addr is:0x%x\n", reg_index, zblk_idx, p_item->hw_addr);
                ZXIC_COMM_PRINT("zreg_mono conflict,key is:");

                for (index = 0; index < 8; index++)
                {
                    ZXIC_COMM_PRINT("0x%x ", p_rbkey_new->key[index]);
                }

                ZXIC_COMM_PRINT("\n");
                /*                    debug end*/
#endif
                break;
            }
        }

        if (*p_end_flag)
        {
            break;
        }

        p_zblk_dn = p_zblk_dn->next;
    } 

    return DPP_OK;
}

/***********************************************************/
/**
* @param   p_se_cfg  算法模块公共管理数据结构指针
* @param   fun_id      hash引擎号
* @param   p_zblk_cfg
*
* @return
* @remark  无
* @see
* @author  XCX      @date  2017/03/27
************************************************************/
DPP_STATUS dpp_hash_zblkcfg_write(DPP_SE_CFG *p_se_cfg, ZXIC_UINT32 fun_id, SE_ZBLK_CFG *p_zblk_cfg)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT8 ram_buf[SE_RAM_WIDTH / 8] = {0};

    ZXIC_UINT32 dev_id = 0;

#if DPP_WRITE_FILE_EN
    ZXIC_UINT32 hw_addr = 0;
#endif

    ZXIC_COMM_CHECK_POINT(p_se_cfg);
    ZXIC_COMM_CHECK_POINT(p_zblk_cfg);
    ZXIC_COMM_CHECK_INDEX(fun_id, HASH_FUNC_ID_MIN, HASH_FUNC_ID_NUM - 1);

    dev_id = p_se_cfg->dev_id;

    /* write hash type*/
    rc = zxic_comm_write_bits_ex(ram_buf,
                           SE_RAM_WIDTH,
                           1,
                           DPP_SE_ZBLK_SERVICE_TYPE_START,
                           DPP_SE_ZBLK_SERVICE_TYPE_START - DPP_SE_ZBLK_SERVICE_TYPE_END + 1);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_write_bits_ex");

    /* write hash channel*/
    rc = zxic_comm_write_bits_ex(ram_buf,
                           SE_RAM_WIDTH,
                           fun_id,
                           DPP_SE_ZBLK_HASH_CHAN_START,
                           DPP_SE_ZBLK_HASH_CHAN_START - DPP_SE_ZBLK_HASH_CHAN_END + 1);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_write_bits_ex");

    /* write enable flag*/
    rc = zxic_comm_write_bits_ex(ram_buf,
                           SE_RAM_WIDTH,
                           1,
                           DPP_SE_ZBLK_HW_POS_EN_START,
                           DPP_SE_ZBLK_HW_POS_EN_START - DPP_SE_ZBLK_HW_POS_EN_END + 1);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_write_bits_ex");

#if DPP_WRITE_FILE_EN
    hw_addr = ZBLK_REG_ADDR_CALC(p_zblk_cfg->zblk_idx, 0);

    rc = dpp_data_w2f(hw_addr, ram_buf, FILE_TYPE_ZBLK_CFG);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_data_w2f");
#endif

#ifdef DPP_FLOW_HW_INIT
    rc = dpp_se_zblk_serv_cfg_set(&p_se_cfg->dev,
                                  p_zblk_cfg->zblk_idx,
                                  ALG_ZBLK_SERV_HASH,
                                  fun_id,
                                  1);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_se_zblk_serv_cfg_set");
#endif

    return DPP_OK;
}

/***********************************************************/
/**
* @param   p_se_cfg  算法模块公共管理数据结构指针
* @param   hash_id   hash引擎号
* @param   bulk_id
*
* @return
* @remark  无
* @see
* @author  XCX      @date  2017/03/27
************************************************************/
DPP_STATUS dpp_hash_bulk_mono_flags_write(DPP_SE_CFG *p_se_cfg, ZXIC_UINT32 hash_id, ZXIC_UINT32 bulk_id)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 hash0_mono_flag = 0;
    ZXIC_UINT32 hash1_mono_flag = 0;
    ZXIC_UINT32 hash2_mono_flag = 0;
    ZXIC_UINT32 hash3_mono_flag = 0;

#if DPP_WRITE_FILE_EN
    ZXIC_UINT32 hash0_mono_flag_file = 0;
    ZXIC_UINT32 hash1_mono_flag_file = 0;
    ZXIC_UINT32 hash2_mono_flag_file = 0;
    ZXIC_UINT32 hash3_mono_flag_file = 0;
    ZXIC_UINT32 hash_mono_flags_file_reg = 0;
    ZXIC_UINT32 hash_mono_flags_file_reg_addr = 0;
    DPP_HASH_FILE_REG_T *p_hash_file_reg = NULL;
#endif

    ZXIC_COMM_CHECK_POINT(p_se_cfg);
    ZXIC_COMM_CHECK_INDEX(hash_id, HASH_FUNC_ID_MIN, HASH_FUNC_ID_NUM - 1);
    ZXIC_COMM_CHECK_INDEX(bulk_id, HASH_BULK_ID_MIN, HASH_BULK_ID_MAX);

    dev_id = p_se_cfg->dev_id;

    rc = dpp_se_hash_zcam_mono_flags_get(&p_se_cfg->dev,
                                         &hash0_mono_flag,
                                         &hash1_mono_flag,
                                         &hash2_mono_flag,
                                         &hash3_mono_flag);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_se_hash_zcam_mono_flags_get");

    switch (hash_id)
    {
        case 0:
        {
            hash0_mono_flag |= (1U << bulk_id);
            break;
        }

        case 1:
        {
            hash1_mono_flag |= (1U << bulk_id);
            break;
        }

        case 2:
        {
            hash2_mono_flag |= (1U << bulk_id);
            break;
        }

        case 3:
        {
            hash3_mono_flag |= (1U << bulk_id);
            break;
        }

        default:
        {
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n hash id is out of rang.");
            ZXIC_COMM_ASSERT(0);
        }
    }

    rc = dpp_se_hash_zcam_mono_flags_set(&p_se_cfg->dev,
                                         hash0_mono_flag,
                                         hash1_mono_flag,
                                         hash2_mono_flag,
                                         hash3_mono_flag);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_se_hash_zcam_mono_flags_set");

#if DPP_WRITE_FILE_EN
    hash_mono_flags_file_reg_addr = 0x1c4;

    p_hash_file_reg = DPP_GET_HASH_FILE_REG();
    hash_mono_flags_file_reg = p_hash_file_reg->hash_mono_flags_file_reg;

    ZXIC_COMM_UINT32_GET_BITS(hash0_mono_flag_file, hash_mono_flags_file_reg, HASH0_MONO_FLAG_BT_START, HASH0_MONO_FLAG_BT_WIDTH);
    ZXIC_COMM_UINT32_GET_BITS(hash1_mono_flag_file, hash_mono_flags_file_reg, HASH1_MONO_FLAG_BT_START, HASH1_MONO_FLAG_BT_WIDTH);
    ZXIC_COMM_UINT32_GET_BITS(hash2_mono_flag_file, hash_mono_flags_file_reg, HASH2_MONO_FLAG_BT_START, HASH2_MONO_FLAG_BT_WIDTH);
    ZXIC_COMM_UINT32_GET_BITS(hash3_mono_flag_file, hash_mono_flags_file_reg, HASH3_MONO_FLAG_BT_START, HASH3_MONO_FLAG_BT_WIDTH);

    switch (hash_id)
    {
        case 0:
        {
            hash0_mono_flag_file |= (1U << bulk_id);
            break;
        }

        case 1:
        {
            hash1_mono_flag_file |= (1U << bulk_id);
            break;
        }

        case 2:
        {
            hash2_mono_flag_file |= (1U << bulk_id);
            break;
        }

        case 3:
        {
            hash3_mono_flag_file |= (1U << bulk_id);
            break;
        }

        default:
        {
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n hash id is out of rang.");
            ZXIC_COMM_ASSERT(0);
        }
    }

    ZXIC_COMM_UINT32_WRITE_BITS(hash_mono_flags_file_reg,
                          hash0_mono_flag_file,
                          HASH0_MONO_FLAG_BT_START,
                          HASH0_MONO_FLAG_BT_WIDTH);
    ZXIC_COMM_UINT32_WRITE_BITS(hash_mono_flags_file_reg,
                          hash1_mono_flag_file,
                          HASH1_MONO_FLAG_BT_START,
                          HASH1_MONO_FLAG_BT_WIDTH);
    ZXIC_COMM_UINT32_WRITE_BITS(hash_mono_flags_file_reg,
                          hash2_mono_flag_file,
                          HASH2_MONO_FLAG_BT_START,
                          HASH2_MONO_FLAG_BT_WIDTH);
    ZXIC_COMM_UINT32_WRITE_BITS(hash_mono_flags_file_reg,
                          hash3_mono_flag_file,
                          HASH3_MONO_FLAG_BT_START,
                          HASH3_MONO_FLAG_BT_WIDTH);

    p_hash_file_reg->hash_mono_flags_file_reg = hash_mono_flags_file_reg;

    rc = dpp_data_w2f(p_se_cfg->reg_base + hash_mono_flags_file_reg_addr, &p_hash_file_reg->hash_mono_flags_file_reg, FILE_TYPE_REG);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_data_w2f");
#endif

    return DPP_OK;
}

/***********************************************************/
/**
* @param   p_se_cfg  算法模块公共管理数据结构指针
* @param   p_zcell_cfg
*
* @return
* @remark  无
* @see
* @author  XCX      @date  2017/03/27
************************************************************/
DPP_STATUS dpp_hash_zcell_mono_write(DPP_SE_CFG *p_se_cfg, SE_ZCELL_CFG *p_zcell_cfg)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 zblk_idx = 0;
    ZXIC_UINT32 zcell_id = 0;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 zcell0_bulk_id = 0;
    ZXIC_UINT32 zcell0_mono_flag = 0;
    ZXIC_UINT32 zcell1_bulk_id = 0;
    ZXIC_UINT32 zcell1_mono_flag = 0;
    ZXIC_UINT32 zcell2_bulk_id = 0;
    ZXIC_UINT32 zcell2_mono_flag = 0;
    ZXIC_UINT32 zcell3_bulk_id = 0;
    ZXIC_UINT32 zcell3_mono_flag = 0;

#if DPP_WRITE_FILE_EN
    ZXIC_UINT32 zcell0_bulk_id_file = 0;
    ZXIC_UINT32 zcell0_mono_flag_file = 0;
    ZXIC_UINT32 zcell1_bulk_id_file = 0;
    ZXIC_UINT32 zcell1_mono_flag_file = 0;
    ZXIC_UINT32 zcell2_bulk_id_file = 0;
    ZXIC_UINT32 zcell2_mono_flag_file = 0;
    ZXIC_UINT32 zcell3_bulk_id_file = 0;
    ZXIC_UINT32 zcell3_mono_flag_file = 0;
    ZXIC_UINT32 zcell_mono_file_ram_addr = 0;

    ZXIC_UINT8 *zcell_mono_file_ram = NULL;
    DPP_HASH_FILE_REG_T *p_hash_file_reg = NULL;
#endif

    ZXIC_COMM_CHECK_POINT(p_se_cfg);
    ZXIC_COMM_CHECK_POINT(p_zcell_cfg);

    dev_id = p_se_cfg->dev_id;

    zblk_idx = GET_ZBLK_IDX(p_zcell_cfg->zcell_idx);
    zcell_id = p_zcell_cfg->zcell_idx & 0x3;
    rc = dpp_se_zcell_mono_cfg_get(&p_se_cfg->dev,
                                   zblk_idx,
                                   &zcell0_bulk_id,
                                   &zcell0_mono_flag,
                                   &zcell1_bulk_id,
                                   &zcell1_mono_flag,
                                   &zcell2_bulk_id,
                                   &zcell2_mono_flag,
                                   &zcell3_bulk_id,
                                   &zcell3_mono_flag);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_se_zcell_mono_cfg_get");

    switch (zcell_id)
    {
        case 0:
        {
            zcell0_bulk_id = p_zcell_cfg->bulk_id;
            zcell0_mono_flag = 1;
            break;
        }

        case 1:
        {
            zcell1_bulk_id = p_zcell_cfg->bulk_id;
            zcell1_mono_flag = 1;
            break;
        }

        case 2:
        {
            zcell2_bulk_id = p_zcell_cfg->bulk_id;
            zcell2_mono_flag = 1;
            break;
        }

        case 3:
        {
            zcell3_bulk_id = p_zcell_cfg->bulk_id;
            zcell3_mono_flag = 1;
            break;
        }

    }

    rc = dpp_se_zcell_mono_cfg_set(&p_se_cfg->dev,
                                   zblk_idx,
                                   zcell0_bulk_id,
                                   zcell0_mono_flag,
                                   zcell1_bulk_id,
                                   zcell1_mono_flag,
                                   zcell2_bulk_id,
                                   zcell2_mono_flag,
                                   zcell3_bulk_id,
                                   zcell3_mono_flag);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_se_zcell_mono_cfg_set");

#if DPP_WRITE_FILE_EN
    zcell_mono_file_ram_addr = ZBLK_REG_ADDR_CALC(zblk_idx, 0x14);

    p_hash_file_reg = DPP_GET_HASH_FILE_REG();
    zcell_mono_file_ram = p_hash_file_reg->zcell_mono_file_ram;

    if (0 == zcell_id)
    {
        ZXIC_COMM_MEMSET(zcell_mono_file_ram, 0, (SE_RAM_WIDTH / 8));
    }

    zxic_comm_read_bits_ex(zcell_mono_file_ram, SE_RAM_WIDTH, &zcell0_bulk_id_file,   ZCELL0_BULK_ID_BT_START,   ZCELL0_BULK_ID_BT_WIDTH);
    zxic_comm_read_bits_ex(zcell_mono_file_ram, SE_RAM_WIDTH, &zcell0_mono_flag_file, ZCELL0_MONO_FLAG_BT_START, ZCELL0_MONO_FLAG_BT_WIDTH);
    zxic_comm_read_bits_ex(zcell_mono_file_ram, SE_RAM_WIDTH, &zcell1_bulk_id_file,   ZCELL1_BULK_ID_BT_START,   ZCELL1_BULK_ID_BT_WIDTH);
    zxic_comm_read_bits_ex(zcell_mono_file_ram, SE_RAM_WIDTH, &zcell1_mono_flag_file, ZCELL1_MONO_FLAG_BT_START, ZCELL1_MONO_FLAG_BT_WIDTH);
    zxic_comm_read_bits_ex(zcell_mono_file_ram, SE_RAM_WIDTH, &zcell2_bulk_id_file,   ZCELL2_BULK_ID_BT_START,   ZCELL2_BULK_ID_BT_WIDTH);
    zxic_comm_read_bits_ex(zcell_mono_file_ram, SE_RAM_WIDTH, &zcell2_mono_flag_file, ZCELL2_MONO_FLAG_BT_START, ZCELL2_MONO_FLAG_BT_WIDTH);
    zxic_comm_read_bits_ex(zcell_mono_file_ram, SE_RAM_WIDTH, &zcell3_bulk_id_file,   ZCELL3_BULK_ID_BT_START,   ZCELL3_BULK_ID_BT_WIDTH);
    zxic_comm_read_bits_ex(zcell_mono_file_ram, SE_RAM_WIDTH, &zcell3_mono_flag_file, ZCELL3_MONO_FLAG_BT_START, ZCELL3_MONO_FLAG_BT_WIDTH);

    switch (zcell_id)
    {
        case 0:
        {
            zcell0_bulk_id_file = p_zcell_cfg->bulk_id;
            zcell0_mono_flag_file = 1;
            break;
        }

        case 1:
        {
            zcell1_bulk_id_file = p_zcell_cfg->bulk_id;
            zcell1_mono_flag_file = 1;
            break;
        }

        case 2:
        {
            zcell2_bulk_id_file = p_zcell_cfg->bulk_id;
            zcell2_mono_flag_file = 1;
            break;
        }

        case 3:
        {
            zcell3_bulk_id_file = p_zcell_cfg->bulk_id;
            zcell3_mono_flag_file = 1;
            break;
        }

        default:
        {
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n zcell id is out of rang.");
            ZXIC_COMM_ASSERT(0);
        }
    }

    zxic_comm_write_bits_ex(zcell_mono_file_ram,
                      SE_RAM_WIDTH,
                      zcell0_bulk_id_file,
                      ZCELL0_BULK_ID_BT_START,
                      ZCELL0_BULK_ID_BT_WIDTH);
    zxic_comm_write_bits_ex(zcell_mono_file_ram,
                      SE_RAM_WIDTH,
                      zcell0_mono_flag_file,
                      ZCELL0_MONO_FLAG_BT_START,
                      ZCELL0_MONO_FLAG_BT_WIDTH);
    zxic_comm_write_bits_ex(zcell_mono_file_ram,
                      SE_RAM_WIDTH,
                      zcell1_bulk_id_file,
                      ZCELL1_BULK_ID_BT_START,
                      ZCELL1_BULK_ID_BT_WIDTH);
    zxic_comm_write_bits_ex(zcell_mono_file_ram,
                      SE_RAM_WIDTH,
                      zcell1_mono_flag_file,
                      ZCELL1_MONO_FLAG_BT_START,
                      ZCELL1_MONO_FLAG_BT_WIDTH);
    zxic_comm_write_bits_ex(zcell_mono_file_ram,
                      SE_RAM_WIDTH,
                      zcell2_bulk_id_file,
                      ZCELL2_BULK_ID_BT_START,
                      ZCELL2_BULK_ID_BT_WIDTH);
    zxic_comm_write_bits_ex(zcell_mono_file_ram,
                      SE_RAM_WIDTH,
                      zcell2_mono_flag_file,
                      ZCELL2_MONO_FLAG_BT_START,
                      ZCELL2_MONO_FLAG_BT_WIDTH);
    zxic_comm_write_bits_ex(zcell_mono_file_ram,
                      SE_RAM_WIDTH,
                      zcell3_bulk_id_file,
                      ZCELL3_BULK_ID_BT_START,
                      ZCELL3_BULK_ID_BT_WIDTH);
    zxic_comm_write_bits_ex(zcell_mono_file_ram,
                      SE_RAM_WIDTH,
                      zcell3_mono_flag_file,
                      ZCELL3_MONO_FLAG_BT_START,
                      ZCELL3_MONO_FLAG_BT_WIDTH);

    rc = dpp_data_w2f(zcell_mono_file_ram_addr, &p_hash_file_reg->zcell_mono_file_ram, FILE_TYPE_ZBLK_CFG);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_data_w2f");
#endif

    return DPP_OK;
}

/***********************************************************/
/**
* @param   p_se_cfg  算法模块公共管理数据结构指针
* @param   bulk_id
* @param   zblk_idx
* @param   zreg_id
*
* @return
* @remark  无
* @see
* @author  XCX      @date  2017/03/27
************************************************************/
DPP_STATUS dpp_hash_zreg_mono_write(DPP_SE_CFG *p_se_cfg,
                                    ZXIC_UINT32 bulk_id,
                                    ZXIC_UINT32 zblk_idx,
                                    ZXIC_UINT32 zreg_id)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 zreg0_bulk_id = 0;
    ZXIC_UINT32 zreg0_mono_flag = 0;
    ZXIC_UINT32 zreg1_bulk_id = 0;
    ZXIC_UINT32 zreg1_mono_flag = 0;
    ZXIC_UINT32 zreg2_bulk_id = 0;
    ZXIC_UINT32 zreg2_mono_flag = 0;
    ZXIC_UINT32 zreg3_bulk_id = 0;
    ZXIC_UINT32 zreg3_mono_flag = 0;

#if DPP_WRITE_FILE_EN
    ZXIC_UINT32 zreg0_bulk_id_file = 0;
    ZXIC_UINT32 zreg0_mono_flag_file = 0;
    ZXIC_UINT32 zreg1_bulk_id_file = 0;
    ZXIC_UINT32 zreg1_mono_flag_file = 0;
    ZXIC_UINT32 zreg2_bulk_id_file = 0;
    ZXIC_UINT32 zreg2_mono_flag_file = 0;
    ZXIC_UINT32 zreg3_bulk_id_file = 0;
    ZXIC_UINT32 zreg3_mono_flag_file = 0;
    ZXIC_UINT8 *zreg_mono_file_ram = NULL;
    ZXIC_UINT32 zreg_mono_file_ram_addr = 0;
    DPP_HASH_FILE_REG_T *p_hash_file_reg = NULL;
#endif

    ZXIC_COMM_CHECK_POINT(p_se_cfg);

    dev_id = p_se_cfg->dev_id;

    rc = dpp_se_zreg_mono_cfg_get(&p_se_cfg->dev,
                                  zblk_idx,
                                  &zreg0_bulk_id,
                                  &zreg0_mono_flag,
                                  &zreg1_bulk_id,
                                  &zreg1_mono_flag,
                                  &zreg2_bulk_id,
                                  &zreg2_mono_flag,
                                  &zreg3_bulk_id,
                                  &zreg3_mono_flag);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_se_zreg_mono_cfg_get");

    switch (zreg_id)
    {
        case 0:
        {
            zreg0_bulk_id = bulk_id;
            zreg0_mono_flag = 1;
            break;
        }

        case 1:
        {
            zreg1_bulk_id = bulk_id;
            zreg1_mono_flag = 1;
            break;
        }

        case 2:
        {
            zreg2_bulk_id = bulk_id;
            zreg2_mono_flag = 1;
            break;
        }

        case 3:
        {
            zreg3_bulk_id = bulk_id;
            zreg3_mono_flag = 1;
            break;
        }

        default:
        {
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n zreg id is out of rang.");
            ZXIC_COMM_ASSERT(0);
        }
    }

    rc = dpp_se_zreg_mono_cfg_set(&p_se_cfg->dev,
                                  zblk_idx,
                                  zreg0_bulk_id,
                                  zreg0_mono_flag,
                                  zreg1_bulk_id,
                                  zreg1_mono_flag,
                                  zreg2_bulk_id,
                                  zreg2_mono_flag,
                                  zreg3_bulk_id,
                                  zreg3_mono_flag);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_se_zreg_mono_cfg_set");

#if DPP_WRITE_FILE_EN
    zreg_mono_file_ram_addr = ZBLK_REG_ADDR_CALC(zblk_idx, 0x15);

    p_hash_file_reg = DPP_GET_HASH_FILE_REG();
    zreg_mono_file_ram = p_hash_file_reg->zreg_mono_file_ram;

    if (0 == zreg_id)
    {
        ZXIC_COMM_MEMSET(zreg_mono_file_ram, 0, (SE_RAM_WIDTH / 8));
    }

    zxic_comm_read_bits_ex(zreg_mono_file_ram,
                     SE_RAM_WIDTH,
                     &zreg0_bulk_id_file,
                     ZREG0_BULK_ID_BT_START,
                     ZREG0_BULK_ID_BT_WIDTH);
    zxic_comm_read_bits_ex(zreg_mono_file_ram,
                     SE_RAM_WIDTH,
                     &zreg0_mono_flag_file,
                     ZREG0_MONO_FLAG_BT_START,
                     ZREG0_MONO_FLAG_BT_WIDTH);
    zxic_comm_read_bits_ex(zreg_mono_file_ram,
                     SE_RAM_WIDTH,
                     &zreg1_bulk_id_file,
                     ZREG1_BULK_ID_BT_START,
                     ZREG1_BULK_ID_BT_WIDTH);
    zxic_comm_read_bits_ex(zreg_mono_file_ram,
                     SE_RAM_WIDTH,
                     &zreg1_mono_flag_file,
                     ZREG1_MONO_FLAG_BT_START,
                     ZREG1_MONO_FLAG_BT_WIDTH);
    zxic_comm_read_bits_ex(zreg_mono_file_ram,
                     SE_RAM_WIDTH,
                     &zreg2_bulk_id_file,
                     ZREG2_BULK_ID_BT_START,
                     ZREG2_BULK_ID_BT_WIDTH);
    zxic_comm_read_bits_ex(zreg_mono_file_ram,
                     SE_RAM_WIDTH,
                     &zreg2_mono_flag_file,
                     ZREG2_MONO_FLAG_BT_START,
                     ZREG2_MONO_FLAG_BT_WIDTH);
    zxic_comm_read_bits_ex(zreg_mono_file_ram,
                     SE_RAM_WIDTH,
                     &zreg3_bulk_id_file,
                     ZREG3_BULK_ID_BT_START,
                     ZREG3_BULK_ID_BT_WIDTH);
    zxic_comm_read_bits_ex(zreg_mono_file_ram,
                     SE_RAM_WIDTH,
                     &zreg3_mono_flag_file,
                     ZREG3_MONO_FLAG_BT_START,
                     ZREG3_MONO_FLAG_BT_WIDTH);

    switch (zreg_id)
    {
        case 0:
        {
            zreg0_bulk_id_file = bulk_id;
            zreg0_mono_flag_file = 1;
            break;
        }

        case 1:
        {
            zreg1_bulk_id_file = bulk_id;
            zreg1_mono_flag_file = 1;
            break;
        }

        case 2:
        {
            zreg2_bulk_id_file = bulk_id;
            zreg2_mono_flag_file = 1;
            break;
        }

        case 3:
        {
            zreg3_bulk_id_file = bulk_id;
            zreg3_mono_flag_file = 1;
            break;
        }

        default:
        {
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n zcell id is out of rang.");
            ZXIC_COMM_ASSERT(0);
        }
    }

    zxic_comm_write_bits_ex(zreg_mono_file_ram,
                      SE_RAM_WIDTH,
                      zreg0_bulk_id_file,
                      ZREG0_BULK_ID_BT_START,
                      ZREG0_BULK_ID_BT_WIDTH);
    zxic_comm_write_bits_ex(zreg_mono_file_ram,
                      SE_RAM_WIDTH,
                      zreg0_mono_flag_file,
                      ZREG0_MONO_FLAG_BT_START,
                      ZREG0_MONO_FLAG_BT_WIDTH);
    zxic_comm_write_bits_ex(zreg_mono_file_ram,
                      SE_RAM_WIDTH,
                      zreg1_bulk_id_file,
                      ZREG1_BULK_ID_BT_START,
                      ZREG1_BULK_ID_BT_WIDTH);
    zxic_comm_write_bits_ex(zreg_mono_file_ram,
                      SE_RAM_WIDTH,
                      zreg1_mono_flag_file,
                      ZREG1_MONO_FLAG_BT_START,
                      ZREG1_MONO_FLAG_BT_WIDTH);
    zxic_comm_write_bits_ex(zreg_mono_file_ram,
                      SE_RAM_WIDTH,
                      zreg2_bulk_id_file,
                      ZREG2_BULK_ID_BT_START,
                      ZREG2_BULK_ID_BT_WIDTH);
    zxic_comm_write_bits_ex(zreg_mono_file_ram,
                      SE_RAM_WIDTH,
                      zreg2_mono_flag_file,
                      ZREG2_MONO_FLAG_BT_START,
                      ZREG2_MONO_FLAG_BT_WIDTH);
    zxic_comm_write_bits_ex(zreg_mono_file_ram,
                      SE_RAM_WIDTH,
                      zreg3_bulk_id_file,
                      ZREG3_BULK_ID_BT_START,
                      ZREG3_BULK_ID_BT_WIDTH);
    zxic_comm_write_bits_ex(zreg_mono_file_ram,
                      SE_RAM_WIDTH,
                      zreg3_mono_flag_file,
                      ZREG3_MONO_FLAG_BT_START,
                      ZREG3_MONO_FLAG_BT_WIDTH);

    rc = dpp_data_w2f(zreg_mono_file_ram_addr, &p_hash_file_reg->zreg_mono_file_ram, FILE_TYPE_ZBLK_CFG);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_data_w2f");
#endif

    return DPP_OK;
}

/***********************************************************/
/** hash ddr参数配置
* @param   p_se_cfg  算法模块公共管理数据结构指针
* @param   fun_id    hash引擎号
* @param   bulk_id
* @param   p_ddr_cfg
*
* @return
* @remark  无
* @see
* @author  wcl      @date  2014/07/31
************************************************************/
DPP_STATUS dpp_hash_ext_cfg_write(DPP_SE_CFG *p_se_cfg,
                                  ZXIC_UINT32 fun_id,
                                  ZXIC_UINT32 bulk_id,
                                  HASH_DDR_CFG *p_ddr_cfg)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 ext_mode = 0;
    ZXIC_UINT32 ext_flag = 0;
    ZXIC_UINT32 dev_id = 0;

#if DPP_WRITE_FILE_EN
    ZXIC_UINT32 ext_mode_file = 0;
    ZXIC_UINT32 ext_flag_file = 0;
    ZXIC_UINT32 ext_cfg_file_reg = 0;
    ZXIC_UINT32 ext_cfg_file_reg_addr = 0;
    DPP_HASH_FILE_REG_T *p_hash_file_reg = NULL;
#endif

    ZXIC_COMM_CHECK_POINT(p_se_cfg);
    ZXIC_COMM_CHECK_POINT(p_ddr_cfg);
    ZXIC_COMM_CHECK_INDEX(fun_id, DPP_HASH_ID_MIN, DPP_HASH_ID_MAX);
    ZXIC_COMM_CHECK_INDEX(bulk_id, HASH_BULK_ID_MIN, HASH_BULK_ID_MAX);

    dev_id = p_se_cfg->dev_id;

    rc = dpp_se_hash_ext_cfg_get(&p_se_cfg->dev, fun_id, &ext_mode, &ext_flag);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_se_hash_ext_cfg_get");

    ext_flag = 1;

    if (p_ddr_cfg->width_mode == DDR_WIDTH_256b)
    {
        ext_mode = (ext_mode & (~(1U << bulk_id)));
    }
    else if (p_ddr_cfg->width_mode == DDR_WIDTH_512b)
    {
        ext_mode = (ext_mode | (1U << bulk_id));
    }

#if DPP_WRITE_FILE_EN
    ext_cfg_file_reg_addr = 0xbc + fun_id * 4;

    p_hash_file_reg = DPP_GET_HASH_FILE_REG();
    ext_cfg_file_reg = p_hash_file_reg->ext_cfg_file_reg[fun_id];

    ZXIC_COMM_UINT32_GET_BITS(ext_mode_file,
                        ext_cfg_file_reg,
                        HASH_EXT_MODE_BT_START,
                        HASH_EXT_MODE_BT_WIDTH);
    ZXIC_COMM_UINT32_GET_BITS(ext_flag_file,
                        ext_cfg_file_reg,
                        HASH_EXT_FLAG_BT_START,
                        HASH_EXT_FLAG_BT_WIDTH);

    ext_flag_file = 1;

    if (DDR_WIDTH_256b == p_ddr_cfg->width_mode)
    {
        ext_mode_file = (ext_mode_file & (~(1U << bulk_id)));
    }
    else if (p_ddr_cfg->width_mode == DDR_WIDTH_512b)
    {
        ext_mode_file = (ext_mode_file | (1U << bulk_id));
    }

    ZXIC_COMM_UINT32_WRITE_BITS(ext_cfg_file_reg,
                          ext_mode_file,
                          HASH_EXT_MODE_BT_START,
                          HASH_EXT_MODE_BT_WIDTH);
    ZXIC_COMM_UINT32_WRITE_BITS(ext_cfg_file_reg,
                          ext_flag_file,
                          HASH_EXT_FLAG_BT_START,
                          HASH_EXT_FLAG_BT_WIDTH);

    p_hash_file_reg->ext_cfg_file_reg[fun_id] = ext_cfg_file_reg;

    rc = dpp_data_w2f(p_se_cfg->reg_base + ext_cfg_file_reg_addr, &p_hash_file_reg->ext_cfg_file_reg[fun_id], FILE_TYPE_REG);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_data_w2f");
#endif

    rc = dpp_se_hash_ext_cfg_set(&p_se_cfg->dev, fun_id, ext_mode, ext_flag);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_se_hash_ext_cfg_set");

    return DPP_OK;
}

/***********************************************************/
/**
* @param   p_se_cfg  算法模块公共管理数据结构指针
* @param   fun_id    hash引擎号
*
* @return
* @remark  无
* @see
* @author  wcl      @date  2014/07/31
************************************************************/
DPP_STATUS dpp_hash_ext_cfg_clr(DPP_SE_CFG *p_se_cfg, ZXIC_UINT32 fun_id)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 ext_mode = 0;
    ZXIC_UINT32 ext_flag = 0;
    ZXIC_UINT32 dev_id = 0;

#if DPP_WRITE_FILE_EN
    ZXIC_UINT32 ddr_cfg_dat = 0;
    ZXIC_UINT32 ext_cfg_reg_addr = 0;
#endif

    ZXIC_COMM_CHECK_POINT(p_se_cfg);
    ZXIC_COMM_CHECK_INDEX(fun_id, DPP_HASH_ID_MIN, DPP_HASH_ID_MAX);

    dev_id = p_se_cfg->dev_id;

#if DPP_WRITE_FILE_EN
    ext_cfg_reg_addr = 0xbc + fun_id * 4;

    rc = dpp_data_w2f(p_se_cfg->reg_base + ext_cfg_reg_addr, &ddr_cfg_dat, FILE_TYPE_REG);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_data_w2f");
#endif

    rc = dpp_se_hash_ext_cfg_set(&p_se_cfg->dev,
                                 fun_id,
                                 ext_mode,
                                 ext_flag);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_se_hash_ext_cfg_set");

    return DPP_OK;
}

/***********************************************************/
/** hash 业务表ddr深度配置
* @param   p_se_cfg  算法模块公共管理数据结构指针
* @param   fun_id    hash引擎号
* @param   bulk_id
* @param   p_ddr_cfg
*
* @return
* @remark  无
* @see
* @author  tf      @date  2016/01/28
************************************************************/
DPP_STATUS dpp_hash_tbl_depth_write(DPP_SE_CFG *p_se_cfg,
                                    ZXIC_UINT32 fun_id,
                                    ZXIC_UINT32 bulk_id,
                                    HASH_DDR_CFG *p_ddr_cfg)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 ext_depth = 0;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 hash_tbl0_depth = 0;
    ZXIC_UINT32 hash_tbl1_depth = 0;
    ZXIC_UINT32 hash_tbl2_depth = 0;
    ZXIC_UINT32 hash_tbl3_depth = 0;
    ZXIC_UINT32 hash_tbl4_depth = 0;
    ZXIC_UINT32 hash_tbl5_depth = 0;
    ZXIC_UINT32 hash_tbl6_depth = 0;
    ZXIC_UINT32 hash_tbl7_depth = 0;

#if DPP_WRITE_FILE_EN
    ZXIC_UINT32 hash_tbl0_depth_file = 0;
    ZXIC_UINT32 hash_tbl1_depth_file = 0;
    ZXIC_UINT32 hash_tbl2_depth_file = 0;
    ZXIC_UINT32 hash_tbl3_depth_file = 0;
    ZXIC_UINT32 hash_tbl4_depth_file = 0;
    ZXIC_UINT32 hash_tbl5_depth_file = 0;
    ZXIC_UINT32 hash_tbl6_depth_file = 0;
    ZXIC_UINT32 hash_tbl7_depth_file = 0;
    ZXIC_UINT32 tbl03_depth_file_reg = 0;
    ZXIC_UINT32 tbl47_depth_file_reg = 0;
    ZXIC_UINT32 hash_tbl_depth_file_reg_addr = 0;
    DPP_HASH_FILE_REG_T *p_hash_file_reg = NULL;
#endif

    ZXIC_UINT32 hash_tbl_depth_array[HASH_BULK_NUM] = {0};

    ZXIC_COMM_CHECK_POINT(p_se_cfg);
    ZXIC_COMM_CHECK_POINT(p_ddr_cfg);
    ZXIC_COMM_CHECK_INDEX(fun_id, DPP_HASH_ID_MIN, DPP_HASH_ID_MAX);
    ZXIC_COMM_CHECK_INDEX(bulk_id, HASH_BULK_ID_MIN, HASH_BULK_ID_MAX);

    dev_id = p_se_cfg->dev_id;

    ext_depth = dpp_hash_ddr_depth_conv(p_ddr_cfg->item_num);

    if (((ZXIC_UINT32)1 << ext_depth) != p_ddr_cfg->item_num)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ErrCode[ 0x%x ]: Hash DDR item num:[ %d ] is not N power of 2.\n",
                                 DPP_HASH_RC_INVALID_PARA,
                                 p_ddr_cfg->item_num);
        ZXIC_COMM_ASSERT(0);
        return DPP_HASH_RC_INVALID_PARA;
    }

    rc = dpp_se_hash_tbl_depth_get(&p_se_cfg->dev,
                                   fun_id,
                                   &hash_tbl0_depth,
                                   &hash_tbl1_depth,
                                   &hash_tbl2_depth,
                                   &hash_tbl3_depth,
                                   &hash_tbl4_depth,
                                   &hash_tbl5_depth,
                                   &hash_tbl6_depth,
                                   &hash_tbl7_depth);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_se_hash_tbl_depth_get");

    hash_tbl_depth_array[0] = hash_tbl0_depth;
    hash_tbl_depth_array[1] = hash_tbl1_depth;
    hash_tbl_depth_array[2] = hash_tbl2_depth;
    hash_tbl_depth_array[3] = hash_tbl3_depth;
    hash_tbl_depth_array[4] = hash_tbl4_depth;
    hash_tbl_depth_array[5] = hash_tbl5_depth;
    hash_tbl_depth_array[6] = hash_tbl6_depth;
    hash_tbl_depth_array[7] = hash_tbl7_depth;


    hash_tbl_depth_array[bulk_id] = ext_depth;

    hash_tbl0_depth = hash_tbl_depth_array[0];
    hash_tbl1_depth = hash_tbl_depth_array[1];
    hash_tbl2_depth = hash_tbl_depth_array[2];
    hash_tbl3_depth = hash_tbl_depth_array[3];
    hash_tbl4_depth = hash_tbl_depth_array[4];
    hash_tbl5_depth = hash_tbl_depth_array[5];
    hash_tbl6_depth = hash_tbl_depth_array[6];
    hash_tbl7_depth = hash_tbl_depth_array[7];

#if DPP_WRITE_FILE_EN
    hash_tbl_depth_file_reg_addr = 0x01a4 + fun_id * 8;

    p_hash_file_reg = DPP_GET_HASH_FILE_REG();
    tbl03_depth_file_reg = p_hash_file_reg->hash_depth_file_regs[fun_id].tbl03_depth_file_reg;
    tbl47_depth_file_reg = p_hash_file_reg->hash_depth_file_regs[fun_id].tbl47_depth_file_reg;

    ZXIC_COMM_UINT32_GET_BITS(hash_tbl0_depth_file, tbl03_depth_file_reg, HASH_TBL0_DEPTH_BT_START, HASH_TBL0_DEPTH_BT_WIDTH);
    ZXIC_COMM_UINT32_GET_BITS(hash_tbl1_depth_file, tbl03_depth_file_reg, HASH_TBL1_DEPTH_BT_START, HASH_TBL1_DEPTH_BT_WIDTH);
    ZXIC_COMM_UINT32_GET_BITS(hash_tbl2_depth_file, tbl03_depth_file_reg, HASH_TBL2_DEPTH_BT_START, HASH_TBL2_DEPTH_BT_WIDTH);
    ZXIC_COMM_UINT32_GET_BITS(hash_tbl3_depth_file, tbl03_depth_file_reg, HASH_TBL3_DEPTH_BT_START, HASH_TBL3_DEPTH_BT_WIDTH);

    ZXIC_COMM_UINT32_GET_BITS(hash_tbl4_depth_file, tbl47_depth_file_reg, HASH_TBL4_DEPTH_BT_START, HASH_TBL4_DEPTH_BT_WIDTH);
    ZXIC_COMM_UINT32_GET_BITS(hash_tbl5_depth_file, tbl47_depth_file_reg, HASH_TBL5_DEPTH_BT_START, HASH_TBL5_DEPTH_BT_WIDTH);
    ZXIC_COMM_UINT32_GET_BITS(hash_tbl6_depth_file, tbl47_depth_file_reg, HASH_TBL6_DEPTH_BT_START, HASH_TBL6_DEPTH_BT_WIDTH);
    ZXIC_COMM_UINT32_GET_BITS(hash_tbl7_depth_file, tbl47_depth_file_reg, HASH_TBL7_DEPTH_BT_START, HASH_TBL7_DEPTH_BT_WIDTH);

    hash_tbl_depth_array[0] = hash_tbl0_depth_file;
    hash_tbl_depth_array[1] = hash_tbl1_depth_file;
    hash_tbl_depth_array[2] = hash_tbl2_depth_file;
    hash_tbl_depth_array[3] = hash_tbl3_depth_file;
    hash_tbl_depth_array[4] = hash_tbl4_depth_file;
    hash_tbl_depth_array[5] = hash_tbl5_depth_file;
    hash_tbl_depth_array[6] = hash_tbl6_depth_file;
    hash_tbl_depth_array[7] = hash_tbl7_depth_file;


    hash_tbl_depth_array[bulk_id] = ext_depth;

    hash_tbl0_depth_file = hash_tbl_depth_array[0];
    hash_tbl1_depth_file = hash_tbl_depth_array[1];
    hash_tbl2_depth_file = hash_tbl_depth_array[2];
    hash_tbl3_depth_file = hash_tbl_depth_array[3];
    hash_tbl4_depth_file = hash_tbl_depth_array[4];
    hash_tbl5_depth_file = hash_tbl_depth_array[5];
    hash_tbl6_depth_file = hash_tbl_depth_array[6];
    hash_tbl7_depth_file = hash_tbl_depth_array[7];

    ZXIC_COMM_UINT32_WRITE_BITS(tbl03_depth_file_reg, hash_tbl0_depth_file, HASH_TBL0_DEPTH_BT_START, HASH_TBL0_DEPTH_BT_WIDTH);
    ZXIC_COMM_UINT32_WRITE_BITS(tbl03_depth_file_reg, hash_tbl1_depth_file, HASH_TBL1_DEPTH_BT_START, HASH_TBL1_DEPTH_BT_WIDTH);
    ZXIC_COMM_UINT32_WRITE_BITS(tbl03_depth_file_reg, hash_tbl2_depth_file, HASH_TBL2_DEPTH_BT_START, HASH_TBL2_DEPTH_BT_WIDTH);
    ZXIC_COMM_UINT32_WRITE_BITS(tbl03_depth_file_reg, hash_tbl3_depth_file, HASH_TBL3_DEPTH_BT_START, HASH_TBL3_DEPTH_BT_WIDTH);

    ZXIC_COMM_UINT32_WRITE_BITS(tbl47_depth_file_reg, hash_tbl4_depth_file, HASH_TBL4_DEPTH_BT_START, HASH_TBL4_DEPTH_BT_WIDTH);
    ZXIC_COMM_UINT32_WRITE_BITS(tbl47_depth_file_reg, hash_tbl5_depth_file, HASH_TBL5_DEPTH_BT_START, HASH_TBL5_DEPTH_BT_WIDTH);
    ZXIC_COMM_UINT32_WRITE_BITS(tbl47_depth_file_reg, hash_tbl6_depth_file, HASH_TBL6_DEPTH_BT_START, HASH_TBL6_DEPTH_BT_WIDTH);
    ZXIC_COMM_UINT32_WRITE_BITS(tbl47_depth_file_reg, hash_tbl7_depth_file, HASH_TBL7_DEPTH_BT_START, HASH_TBL7_DEPTH_BT_WIDTH);

    p_hash_file_reg->hash_depth_file_regs[fun_id].tbl03_depth_file_reg = tbl03_depth_file_reg;
    p_hash_file_reg->hash_depth_file_regs[fun_id].tbl47_depth_file_reg = tbl47_depth_file_reg;

    rc = dpp_data_w2f(p_se_cfg->reg_base + hash_tbl_depth_file_reg_addr,     &tbl03_depth_file_reg, FILE_TYPE_REG);
    rc = dpp_data_w2f(p_se_cfg->reg_base + hash_tbl_depth_file_reg_addr + 4, &tbl47_depth_file_reg, FILE_TYPE_REG) + rc;
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_data_w2f");
#endif

    rc = dpp_se_hash_tbl_depth_set(&p_se_cfg->dev,
                                   fun_id,
                                   hash_tbl0_depth,
                                   hash_tbl1_depth,
                                   hash_tbl2_depth,
                                   hash_tbl3_depth,
                                   hash_tbl4_depth,
                                   hash_tbl5_depth,
                                   hash_tbl6_depth,
                                   hash_tbl7_depth);

    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_se_hash_tbl_depth_set");

    return DPP_OK;
}

/***********************************************************/
/** hash 业务表ddr深度恢复默认值
* @param   p_se_cfg  算法模块公共管理数据结构指针
* @param   fun_id    hash引擎号
*
* @return
* @remark  无
* @see
* @author  tf      @date  2016/01/28
************************************************************/
DPP_STATUS dpp_hash_tbl_depth_clr(DPP_SE_CFG *p_se_cfg, ZXIC_UINT32 fun_id)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 hash_tbl0_depth = 0x12;
    ZXIC_UINT32 hash_tbl1_depth = 0x12;
    ZXIC_UINT32 hash_tbl2_depth = 0x12;
    ZXIC_UINT32 hash_tbl3_depth = 0x12;
    ZXIC_UINT32 hash_tbl4_depth = 0x12;
    ZXIC_UINT32 hash_tbl5_depth = 0x12;
    ZXIC_UINT32 hash_tbl6_depth = 0x12;
    ZXIC_UINT32 hash_tbl7_depth = 0x12;

#if DPP_WRITE_FILE_EN
    ZXIC_UINT32 hash_tbl_depth_reg_file_addr = 0;
    ZXIC_UINT32 hash_tbl03_depth_reg_file_dat = 0;
    ZXIC_UINT32 hash_tbl47_depth_reg_file_dat = 0;
#endif

    ZXIC_COMM_CHECK_POINT(p_se_cfg);
    ZXIC_COMM_CHECK_INDEX(fun_id, DPP_HASH_ID_MIN, DPP_HASH_ID_MAX);

    dev_id = p_se_cfg->dev_id;

#if DPP_WRITE_FILE_EN
    hash_tbl_depth_reg_file_addr = 0x01a4 + fun_id * 8;

    ZXIC_COMM_UINT32_WRITE_BITS(hash_tbl03_depth_reg_file_dat, hash_tbl0_depth, HASH_TBL0_DEPTH_BT_START, HASH_TBL0_DEPTH_BT_WIDTH);
    ZXIC_COMM_UINT32_WRITE_BITS(hash_tbl03_depth_reg_file_dat, hash_tbl1_depth, HASH_TBL1_DEPTH_BT_START, HASH_TBL1_DEPTH_BT_WIDTH);
    ZXIC_COMM_UINT32_WRITE_BITS(hash_tbl03_depth_reg_file_dat, hash_tbl2_depth, HASH_TBL2_DEPTH_BT_START, HASH_TBL2_DEPTH_BT_WIDTH);
    ZXIC_COMM_UINT32_WRITE_BITS(hash_tbl03_depth_reg_file_dat, hash_tbl3_depth, HASH_TBL3_DEPTH_BT_START, HASH_TBL3_DEPTH_BT_WIDTH);

    ZXIC_COMM_UINT32_WRITE_BITS(hash_tbl47_depth_reg_file_dat, hash_tbl4_depth, HASH_TBL4_DEPTH_BT_START, HASH_TBL4_DEPTH_BT_WIDTH);
    ZXIC_COMM_UINT32_WRITE_BITS(hash_tbl47_depth_reg_file_dat, hash_tbl5_depth, HASH_TBL5_DEPTH_BT_START, HASH_TBL5_DEPTH_BT_WIDTH);
    ZXIC_COMM_UINT32_WRITE_BITS(hash_tbl47_depth_reg_file_dat, hash_tbl6_depth, HASH_TBL6_DEPTH_BT_START, HASH_TBL6_DEPTH_BT_WIDTH);
    ZXIC_COMM_UINT32_WRITE_BITS(hash_tbl47_depth_reg_file_dat, hash_tbl7_depth, HASH_TBL7_DEPTH_BT_START, HASH_TBL7_DEPTH_BT_WIDTH);

    rc = dpp_data_w2f(p_se_cfg->reg_base + hash_tbl_depth_reg_file_addr, &hash_tbl03_depth_reg_file_dat, FILE_TYPE_REG);
    rc = dpp_data_w2f(p_se_cfg->reg_base + hash_tbl_depth_reg_file_addr + 4, &hash_tbl47_depth_reg_file_dat, FILE_TYPE_REG) + rc;
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_data_w2f");
#endif

    rc = dpp_se_hash_tbl_depth_set(&p_se_cfg->dev,
                                   fun_id,
                                   hash_tbl0_depth,
                                   hash_tbl1_depth,
                                   hash_tbl2_depth,
                                   hash_tbl3_depth,
                                   hash_tbl4_depth,
                                   hash_tbl5_depth,
                                   hash_tbl6_depth,
                                   hash_tbl7_depth);

    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_se_hash_tbl_depth_set");

    return DPP_OK;
}

/***********************************************************/
/** hash CRC多项式选择
* @param   fun_id    hash引擎号
* @param   crc_sel
*
* @return
* @remark  无
* @see
* @author  tf      @date  2016/01/28
************************************************************/
DPP_STATUS dpp_hash_tbl_crc_poly_write(DPP_SE_CFG *p_se_cfg,
                                       ZXIC_UINT32 fun_id,
                                       ZXIC_UINT32 bulk_id,
                                       ZXIC_UINT32 crc_sel)
{
    DPP_STATUS rtn = DPP_OK;

    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 hash_tbl_crc_cfg_bt_start = 0;
    ZXIC_UINT32 hash_tbl_crc_cfg_bt_width = 2;
    DPP_SE4K_SE_ALG_HASH10_EXT_CRC_CFG_T  hash01_ext_crc_cfg = {0};
    DPP_SE4K_SE_ALG_HASH32_EXT_CRC_CFG_T  hash23_ext_crc_cfg = {0};

#if DPP_WRITE_FILE_EN
    ZXIC_UINT32 ext_crc_cfg_file_reg = 0;
    ZXIC_UINT32 ext_crc_cfg_file_reg_addr = 0;
    DPP_HASH_FILE_REG_T *p_hash_file_reg = NULL;
#endif

    ZXIC_COMM_ASSERT(p_se_cfg);

    dev_id = p_se_cfg->dev_id;

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, fun_id, DPP_HASH_ID_MIN, DPP_HASH_ID_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, bulk_id, HASH_BULK_ID_MIN, HASH_BULK_ID_MAX);

    hash_tbl_crc_cfg_bt_start = bulk_id * 2;

    if (fun_id == 0 || fun_id == 1)
    {
        rtn = dpp_reg_read(&p_se_cfg->dev, SE4K_SE_ALG_HASH10_EXT_CRC_CFGr, 0, 0, &hash01_ext_crc_cfg);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "dpp_reg_read");

        if (fun_id == 0)
        {
            ZXIC_COMM_UINT32_WRITE_BITS(hash01_ext_crc_cfg.hash0_crc_cfg, crc_sel, hash_tbl_crc_cfg_bt_start, hash_tbl_crc_cfg_bt_width);
        }
        else
        {
            ZXIC_COMM_UINT32_WRITE_BITS(hash01_ext_crc_cfg.hash1_crc_cfg, crc_sel, hash_tbl_crc_cfg_bt_start, hash_tbl_crc_cfg_bt_width);
        }

        rtn = dpp_reg_write(&p_se_cfg->dev, SE4K_SE_ALG_HASH10_EXT_CRC_CFGr, 0, 0, &hash01_ext_crc_cfg);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "dpp_reg_write");

    }
    else
    {
        rtn = dpp_reg_read(&p_se_cfg->dev, SE4K_SE_ALG_HASH32_EXT_CRC_CFGr, 0, 0, &hash23_ext_crc_cfg);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "dpp_reg_read");

        if (fun_id == 2)
        {
            ZXIC_COMM_UINT32_WRITE_BITS(hash23_ext_crc_cfg.hash2_crc_cfg, crc_sel, hash_tbl_crc_cfg_bt_start, hash_tbl_crc_cfg_bt_width);
        }
        else
        {
            ZXIC_COMM_UINT32_WRITE_BITS(hash23_ext_crc_cfg.hash3_crc_cfg, crc_sel, hash_tbl_crc_cfg_bt_start, hash_tbl_crc_cfg_bt_width);
        }

        rtn = dpp_reg_write(&p_se_cfg->dev, SE4K_SE_ALG_HASH32_EXT_CRC_CFGr, 0, 0, &hash23_ext_crc_cfg);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "dpp_reg_write");
    }

#if DPP_WRITE_FILE_EN
    p_hash_file_reg = DPP_GET_HASH_FILE_REG();

    if (fun_id == 0 || fun_id == 1)
    {
        ext_crc_cfg_file_reg_addr = 0x01cc;

        hash_tbl_crc_cfg_bt_start = fun_id * 16 + bulk_id * 2;

        ext_crc_cfg_file_reg = p_hash_file_reg->ext_crc_cfg_file_reg[0];
        ZXIC_COMM_UINT32_WRITE_BITS(ext_crc_cfg_file_reg, crc_sel, hash_tbl_crc_cfg_bt_start, hash_tbl_crc_cfg_bt_width);

        p_hash_file_reg->ext_crc_cfg_file_reg[0] = ext_crc_cfg_file_reg;

        rtn = dpp_data_w2f(p_se_cfg->reg_base + ext_crc_cfg_file_reg_addr, &p_hash_file_reg->ext_crc_cfg_file_reg[0], FILE_TYPE_REG);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "dpp_data_w2f");

    }
    else
    {
        ext_crc_cfg_file_reg_addr = 0x01d0;

        hash_tbl_crc_cfg_bt_start = (fun_id - 2) * 16 + bulk_id * 2;

        ext_crc_cfg_file_reg = p_hash_file_reg->ext_crc_cfg_file_reg[1];
        ZXIC_COMM_UINT32_WRITE_BITS(ext_crc_cfg_file_reg, crc_sel, hash_tbl_crc_cfg_bt_start, hash_tbl_crc_cfg_bt_width);

        p_hash_file_reg->ext_crc_cfg_file_reg[1] = ext_crc_cfg_file_reg;

        rtn = dpp_data_w2f(p_se_cfg->reg_base + ext_crc_cfg_file_reg_addr, &p_hash_file_reg->ext_crc_cfg_file_reg[1], FILE_TYPE_REG);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rtn, "dpp_data_w2f");
    }

#endif

    return DPP_OK;
}


/***********************************************************/
/** 清除hash引擎的所有hash表项(清除软件配置)
* @param   p_se_cfg
* @param   hash_id
* @param   bulk_id
*
* @return
* @remark  无
* @see
* @author  cq      @date  2023/08/18
************************************************************/
DPP_STATUS dpp_hash_soft_all_entry_delete(DPP_SE_CFG *p_se_cfg,ZXIC_UINT32 hash_id)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT8 table_id = 0;
    ZXIC_UINT32 bulk_id = 0;

    D_NODE *p_node = NULL;
    ZXIC_RB_TN *p_rb_tn = NULL;
    ZXIC_RB_TN *p_rb_tn_rtn = NULL;
    D_HEAD *p_head_hash_rb = NULL;
    DPP_HASH_CFG *p_hash_cfg = NULL;
    FUNC_ID_INFO *p_func_info = NULL;
    DPP_HASH_RBKEY_INFO *p_rbkey = NULL;
    DPP_HASH_RBKEY_INFO *p_rbkey_rtn = NULL;
    SE_ITEM_CFG *p_item = NULL;

    ZXIC_COMM_CHECK_POINT(p_se_cfg);
    ZXIC_COMM_CHECK_INDEX(hash_id, 0, HASH_FUNC_ID_NUM - 1);

    p_func_info = DPP_GET_FUN_INFO(p_se_cfg, hash_id);
    DPP_SE_CHECK_FUN(p_func_info, hash_id, FUN_HASH);

    p_hash_cfg = (DPP_HASH_CFG *)p_func_info->fun_ptr;
    p_head_hash_rb = &p_hash_cfg->hash_rb.tn_list;

    while (p_head_hash_rb->used)
    {
        p_node = p_head_hash_rb->p_next;
        p_rb_tn = (ZXIC_RB_TN *)p_node->data;
        p_rbkey = (DPP_HASH_RBKEY_INFO *)p_rb_tn->p_key;
        table_id = DPP_GET_HASH_TBL_ID(p_rbkey->key);
        bulk_id = ((table_id >> 2) & 0x7);
        ZXIC_COMM_CHECK_DEV_INDEX(dev_id, bulk_id, 0, HASH_BULK_NUM - 1);

        rc = zxic_comm_rb_delete(&p_hash_cfg->hash_rb, p_rbkey, &p_rb_tn_rtn);
        if (ZXIC_RBT_RC_SRHFAIL == rc)
        {
            p_hash_cfg->hash_stat.delete_fail++;
            ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "Error!there is not item in hash!\n");
            return DPP_HASH_RC_DEL_SRHFAIL;
        }

        ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_rb_tn_rtn);
        p_rbkey_rtn = (DPP_HASH_RBKEY_INFO *)(p_rb_tn_rtn->p_key);
        p_item = p_rbkey_rtn->p_item_info;
            
        rc = zxic_comm_double_link_del(&(p_rbkey_rtn->entry_dn), &(p_item->item_list));
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_double_link_del");
        p_item->wrt_mask &= ~(DPP_GET_HASH_ENTRY_MASK(p_rbkey_rtn->entry_size, p_rbkey_rtn->entry_pos)) & 0xF;

        if (0 == p_item->item_list.used)
        {
            if ((ITEM_DDR_256 == p_item->item_type) || (ITEM_DDR_512 == p_item->item_type))
            {
                /* modify coverity by yinxh 2021.03.10*,以256bit为单位。暂不考虑512bit的情况*/
                ZXIC_COMM_CHECK_INDEX_UPPER(p_item->item_index, p_hash_cfg->p_bulk_ddr_info[bulk_id]->item_num);
                p_hash_cfg->p_bulk_ddr_info[bulk_id]->p_item_array[p_item->item_index] = NULL;
                ZXIC_COMM_FREE(p_item);
            }
            else
            {
                p_item->valid = 0;
            }
        }

        ZXIC_COMM_FREE(p_rbkey_rtn);
        ZXIC_COMM_FREE(p_rb_tn_rtn);
        p_hash_cfg->hash_stat.delete_ok++;
        p_hash_cfg->hash_stat.insert_table[table_id].delete++;
    }


    return DPP_OK;
}

/***********************************************************/
/** 卸载软件上分配的hash资源(主要针对malloc释放)
* @param   dev_id  设备号 
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/12/20
************************************************************/
DPP_STATUS dpp_hash_soft_uninstall(DPP_DEV_T *dev)
{
    DPP_STATUS  rc       = DPP_OK;
    ZXIC_UINT32 hash_id  = 0;
    ZXIC_UINT32 dev_id = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);

    for(hash_id=0;hash_id<HASH_FUNC_ID_NUM;hash_id++)
    {
        rc = dpp_one_hash_soft_uninstall(dev,hash_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_one_hash_soft_uninstall");
    }

    return DPP_OK;
}

/***********************************************************/
/** 释放当前sdt下的所有hash流表表项(仅删除软件表项，不操作硬件)
* @param   dev_id  设备号 
* @param   sdt_no  sdt号 
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/11/05
************************************************************/
DPP_STATUS dpp_hash_soft_delete_by_sdt(DPP_DEV_T *dev,
                                        ZXIC_UINT32 sdt_no)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT8 key_valid = 0;
    ZXIC_UINT32 table_id = 0;
    ZXIC_UINT32 key_type = 0;

    D_NODE *p_node = NULL;
    ZXIC_RB_TN *p_rb_tn = NULL;
    D_HEAD *p_head_hash_rb = NULL;
    DPP_HASH_CFG *p_hash_cfg = NULL;
    DPP_HASH_RBKEY_INFO *p_rbkey = NULL;
    DPP_HASH_RBKEY_INFO *p_rbkey_rtn = NULL;
    ZXIC_RB_TN *p_rb_tn_rtn = NULL;
    SE_ITEM_CFG *p_item = NULL;
    ZXIC_MUTEX_T *p_hash_mutex = NULL;
    HASH_ENTRY_CFG  hash_entry_cfg = {0}; 
    
    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);

    //从sdt_no中获取hash配置
    rc = dpp_hash_get_hash_info_from_sdt(dev, sdt_no, &hash_entry_cfg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_hash_get_hash_info_from_sdt");

    p_hash_cfg = hash_entry_cfg.p_hash_cfg;
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_hash_cfg);

    rc = dpp_dev_hash_opr_mutex_get(dev, p_hash_cfg->fun_id, &p_hash_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_dev_hash_opr_mutex_get"); 
    rc = zxic_comm_mutex_lock(p_hash_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_mutex_lock"); 

    p_head_hash_rb = &p_hash_cfg->hash_rb.tn_list;
    p_node = p_head_hash_rb->p_next;
    while(p_node)
    {
        p_rb_tn = (ZXIC_RB_TN *)p_node->data;
        p_rbkey = (DPP_HASH_RBKEY_INFO *)p_rb_tn->p_key;

        key_valid = DPP_GET_HASH_KEY_VALID(p_rbkey->key);
        table_id = DPP_GET_HASH_TBL_ID(p_rbkey->key);
        key_type = DPP_GET_HASH_KEY_TYPE(p_rbkey->key);
        if((!key_valid) 
            || (table_id != hash_entry_cfg.table_id)
            || (key_type != hash_entry_cfg.key_type))
        {
            p_node = p_node->next; 
            continue;
        }
        p_node = p_node->next;   

        rc = zxic_comm_rb_delete(&p_hash_cfg->hash_rb, p_rbkey, &p_rb_tn_rtn);
        if (ZXIC_RBT_RC_SRHFAIL == rc)
        {
            p_hash_cfg->hash_stat.delete_fail++;
            ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "Error!there is not item in hash!\n");

            rc = zxic_comm_mutex_unlock(p_hash_mutex);
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_mutex_unlock"); 
            return DPP_HASH_RC_DEL_SRHFAIL;
        }

        ZXIC_COMM_CHECK_DEV_POINT_UNLOCK(dev_id, p_rb_tn_rtn, p_hash_mutex);
        p_rbkey_rtn = (DPP_HASH_RBKEY_INFO *)(p_rb_tn_rtn->p_key);
        p_item = p_rbkey_rtn->p_item_info;
        ZXIC_COMM_CHECK_DEV_POINT_UNLOCK(dev_id,p_item,p_hash_mutex);
        
        rc = zxic_comm_double_link_del(&(p_rbkey_rtn->entry_dn), &(p_item->item_list));
        ZXIC_COMM_CHECK_DEV_RC_UNLOCK(dev_id, rc, "zxic_comm_double_link_del", p_hash_mutex);
        p_item->wrt_mask &= ~(DPP_GET_HASH_ENTRY_MASK(p_rbkey_rtn->entry_size, p_rbkey_rtn->entry_pos)) & 0xF;
        if (0 == p_item->item_list.used)
        {
            p_item->valid = 0; 
        }

        ZXIC_COMM_FREE(p_rbkey_rtn);
        ZXIC_COMM_FREE(p_rb_tn_rtn);
        p_hash_cfg->hash_stat.delete_ok++;
        p_hash_cfg->hash_stat.insert_table[table_id].delete++;
    }

    rc = zxic_comm_mutex_unlock(p_hash_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_mutex_unlock"); 

    return DPP_OK;
}

/***********************************************************/
/** 卸载指定hashid上软件分配的hash资源(主要针对malloc释放)
* @param   dev_id  设备号 
* @param   hash_id  hash id 0~3
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/12/20
************************************************************/
DPP_STATUS dpp_one_hash_soft_uninstall(DPP_DEV_T *dev,ZXIC_UINT32 hash_id)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 dev_id = 0;

    D_NODE *p_node = NULL;
    DPP_SE_CFG *p_se_cfg = NULL;
    ZXIC_RB_TN *p_rb_tn = NULL;
    ZXIC_RB_TN *p_rb_tn_rtn = NULL;
    HASH_DDR_CFG *p_rbkey = NULL;
    DPP_HASH_CFG *p_hash_cfg = NULL;
    FUNC_ID_INFO *p_func_info = NULL;
    D_HEAD *p_head_ddr_cfg_rb = NULL;
    HASH_DDR_CFG *p_temp_rbkey = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(hash_id, 0, HASH_FUNC_ID_NUM - 1);

    /* 取出se配置 */
    rc = dpp_se_cfg_get(dev, &p_se_cfg);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_se_cfg_get");
    ZXIC_COMM_CHECK_POINT(p_se_cfg);
    p_func_info = DPP_GET_FUN_INFO(p_se_cfg, hash_id);
    if (0 == p_func_info->is_used)
    {
        ZXIC_COMM_TRACE_DEBUG("Error[0x%x], fun_id [%d] is not init\n!", DPP_SE_RC_FUN_INVALID, hash_id);
        return DPP_OK;
    }

    /*软件删除表项,删除红黑树节点*/
    rc = dpp_hash_soft_all_entry_delete(p_se_cfg,hash_id);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_hash_soft_all_entry_delete");

    /*释放独占标识内存*/
    p_hash_cfg = (DPP_HASH_CFG *)p_func_info->fun_ptr;
    for (i = 0; i < HASH_BULK_NUM; i++)
    {
        if ((&p_hash_cfg->hash_stat)->p_bulk_zcam_mono[i] != NULL)
        {
            ZXIC_COMM_FREE((&p_hash_cfg->hash_stat)->p_bulk_zcam_mono[i]);
            (&p_hash_cfg->hash_stat)->p_bulk_zcam_mono[i] = NULL;
        }
    }

    /*释放DDR分配的所有内存*/
    p_head_ddr_cfg_rb = &p_hash_cfg->ddr_cfg_rb.tn_list;
    while (p_head_ddr_cfg_rb->used)
    {
        p_node = p_head_ddr_cfg_rb->p_next;

        p_rb_tn = (ZXIC_RB_TN *)p_node->data;
        p_rbkey = p_rb_tn->p_key;

        rc = zxic_comm_rb_delete(&p_hash_cfg->ddr_cfg_rb, p_rbkey, &p_rb_tn_rtn);

        if (ZXIC_RBT_RC_SRHFAIL == rc)
        {
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ddr_cfg_rb delete key is not exist, key: 0x");
        }
        else
        {
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_rb_delete");
        }

        ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_rb_tn_rtn);
        p_temp_rbkey = (HASH_DDR_CFG *)(p_rb_tn_rtn->p_key);
        ZXIC_COMM_FREE(p_temp_rbkey->p_item_array);
        p_temp_rbkey->p_item_array = NULL;
        ZXIC_COMM_FREE(p_temp_rbkey);
        ZXIC_COMM_FREE(p_rb_tn_rtn);
    }

    rc = dpp_hash_zcam_resource_deinit(p_hash_cfg);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_hash_zcam_resource_deinit");

    rc = dpp_se_fun_deinit(p_se_cfg, (hash_id & 0xff), FUN_HASH);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_se_fun_deinit");

    return DPP_OK;
}

/***********************************************************/
/**
* @param   p_hash_cfg
* @param   p_entry
* @param   key_by_size
* @param   rst_by_size
* @param   p_item_info
* @param   p_wrt_lrn_rsp
*
* @return
* @remark  无
* @see
* @author  XCX      @date  2018/03/22
************************************************************/
DPP_STATUS dpp_hash_software_item_check(DPP_HASH_CFG *p_hash_cfg,
                                        DPP_HASH_ENTRY *p_entry,
                                        ZXIC_UINT32 key_by_size,
                                        ZXIC_UINT32 rst_by_size,
                                        SE_ITEM_CFG *p_item_info,
                                        DPP_HASH_WRT_LRN_RSP *p_wrt_lrn_rsp)
{
    ZXIC_UINT8 srh_succ = 0;
    ZXIC_UINT8 srh_key_type = 0;
    ZXIC_UINT8 srh_entry_size = 0;
    ZXIC_UINT8 temp_key_type = 0;
    ZXIC_UINT8 srh_key[HASH_KEY_MAX] = {0};

    ZXIC_UINT32 tbl_id = 0;
    ZXIC_UINT32 free_pos = 0xFFFFFFFF; /* ** -1;  */
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 slot_id = 0;
    ZXIC_UINT32 item_entry_max = ITEM_ENTRY_NUM_4;

    D_NODE *p_entry_dn = NULL;
    DPP_HASH_RBKEY_INFO *p_rbkey = NULL;
    DPP_HASH_TBL_ID_INFO *p_tbl_id_info = NULL;

    ZXIC_COMM_CHECK_INDEX(key_by_size, 0, HASH_KEY_MAX);
    ZXIC_COMM_CHECK_POINT(p_hash_cfg);
    ZXIC_COMM_CHECK_POINT(p_entry);
    ZXIC_COMM_CHECK_POINT(p_item_info);
    ZXIC_COMM_CHECK_POINT(p_wrt_lrn_rsp);
    ZXIC_COMM_CHECK_POINT(p_hash_cfg->p_se_info);

    dev_id = p_hash_cfg->p_se_info->dev_id;
    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id,DPP_DEV_CHANNEL_MAX - 1);

    tbl_id = DPP_GET_HASH_TBL_ID(p_entry->p_key);
    slot_id = p_hash_cfg->p_se_info->dev.pcie_channel.slot;
    ZXIC_COMM_CHECK_INDEX_UPPER(slot_id,DPP_PCIE_SLOT_MAX - 1);
    p_tbl_id_info = GET_HASH_TBL_ID_INFO(slot_id, p_hash_cfg->fun_id, tbl_id);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_tbl_id_info);

    srh_key_type = DPP_GET_HASH_KEY_TYPE(p_entry->p_key);
    ZXIC_COMM_MEMCPY(srh_key, p_entry->p_key, 1);                             /* wr_flag + key_type + tbl_id */
    ZXIC_COMM_MEMCPY(srh_key + 1, p_entry->p_key + 1 + (HASH_KEY_MAX - key_by_size), key_by_size - 1); /* actural_key */
    srh_key[0] = (ZXIC_UINT8)((srh_key[0] | 0x80) & 0xFF);    /* set valid bit */
    srh_entry_size = DPP_GET_HASH_ENTRY_SIZE(srh_key_type);

    p_entry_dn = p_item_info->item_list.p_next;

    while (p_entry_dn)
    {
        p_rbkey = (DPP_HASH_RBKEY_INFO *)p_entry_dn->data;
        ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_rbkey);

        ZXIC_COMM_ASSERT(p_rbkey->p_item_info == p_item_info);

        temp_key_type = DPP_GET_HASH_KEY_TYPE(p_rbkey->key);

        if (DPP_GET_HASH_KEY_VALID(p_rbkey->key) && (srh_key_type == temp_key_type))
        {
            if (0 == ZXIC_COMM_MEMCMP(srh_key, p_rbkey->key, key_by_size))
            {
                srh_succ = 1;
                break;
            }
        }

        p_entry_dn = p_entry_dn->next;
    }

    if (NULL == p_rbkey)
    {
        return ZXIC_PAR_CHK_POINT_NULL;
    }

    if (!srh_succ)
    {
        if ((p_tbl_id_info->is_lrn || p_tbl_id_info->is_mc_wrt) && !p_wrt_lrn_rsp->space_vld) /* mod by tf 2016-5-25 14:39:36 */
        {
            if (ITEM_DDR_256 == p_item_info->item_type)
            {
                item_entry_max = ITEM_ENTRY_NUM_2;
            }

            free_pos = dpp_hash_get_item_free_pos(item_entry_max, p_item_info->wrt_mask, srh_entry_size);

            if (free_pos != 0xFFFFFFFF)
            {
                /* modify coverity by yinxh 2021.03.10*/
                // ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW_NO_ASSERT(4, srh_entry_size/16);
                ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW_NO_ASSERT(4U - srh_entry_size/16U, free_pos);
                p_wrt_lrn_rsp->space_vld = 1;
                p_wrt_lrn_rsp->wrt_mask = DPP_GET_HASH_ENTRY_MASK(srh_entry_size, free_pos);

                if (ITEM_DDR_256 == p_item_info->item_type)
                {
                    p_wrt_lrn_rsp->ext_flag = 1;
                    p_wrt_lrn_rsp->width_flag = 0;
                    p_wrt_lrn_rsp->lrn_addr = p_item_info->hw_addr & ZXIC_COMM_GET_BIT_MASK(ZXIC_UINT32, HASH_ADDR_DDR_BT_LEN);
                }
                else if (ITEM_DDR_512 == p_item_info->item_type)
                {
                    p_wrt_lrn_rsp->ext_flag = 1;
                    p_wrt_lrn_rsp->width_flag = 1;
                    p_wrt_lrn_rsp->lrn_addr = (p_item_info->hw_addr) & ZXIC_COMM_GET_BIT_MASK(ZXIC_UINT32, HASH_ADDR_DDR_BT_LEN);
                }
                else
                {
                    p_wrt_lrn_rsp->ext_flag = 0;
                    p_wrt_lrn_rsp->width_flag = 1;
                    p_wrt_lrn_rsp->lrn_addr = p_item_info->hw_addr & ZXIC_COMM_GET_BIT_MASK(ZXIC_UINT32, HASH_ADDR_ZCAM_BT_LEN);
                }
            }
        }

        return DPP_HASH_RC_MATCH_ITEM_FAIL;
    }

    /* search success */
    if (p_tbl_id_info->is_mc_wrt)
    {
        p_wrt_lrn_rsp->space_vld = 0;
        p_wrt_lrn_rsp->wrt_mask = DPP_GET_HASH_ENTRY_MASK(p_rbkey->entry_size, p_rbkey->entry_pos);

        if (ITEM_DDR_256 == p_item_info->item_type)
        {
            p_wrt_lrn_rsp->ext_flag = 1;
            p_wrt_lrn_rsp->width_flag = 0;
            p_wrt_lrn_rsp->lrn_addr = p_item_info->hw_addr & ZXIC_COMM_GET_BIT_MASK(ZXIC_UINT32, HASH_ADDR_DDR_BT_LEN);
        }
        else if (ITEM_DDR_512 == p_item_info->item_type)
        {
            p_wrt_lrn_rsp->ext_flag = 1;
            p_wrt_lrn_rsp->width_flag = 1;
            p_wrt_lrn_rsp->lrn_addr = (p_item_info->hw_addr) & ZXIC_COMM_GET_BIT_MASK(ZXIC_UINT32, HASH_ADDR_DDR_BT_LEN);
        }
        else
        {
            p_wrt_lrn_rsp->ext_flag = 0;
            p_wrt_lrn_rsp->width_flag = 1;
            p_wrt_lrn_rsp->lrn_addr = p_item_info->hw_addr & ZXIC_COMM_GET_BIT_MASK(ZXIC_UINT32, HASH_ADDR_ZCAM_BT_LEN);
        }
    }

    /* copy result */
    ZXIC_COMM_MEMCPY(p_entry->p_rst, p_rbkey->rst, (rst_by_size > HASH_RST_MAX) ? HASH_RST_MAX : rst_by_size);

    return DPP_OK;
}

/***********************************************************/
/** 单个hash引擎查找调试函数
* @param   p_se_cfg    算法模块公共管理数据结构指针
* @param   fun_id      hash引擎号
* @param   p_entry     hash条目，包括key和result，查找时的key的位宽为392bit，格式为:wr_flag(1bit) + key_type(2bit) + tbl_id(5bit) + reserve(M bit)+ actu_key(8*N bit) \n
*                      result用于在硬件学习使能的情况下，返回空闲位置的地址。p_entry->p_rst需要预留 HASH_RST_MAX 字节的空间
* @param   p_space_vld 学习使能时，是否有空闲空间
* @param   srh_mode    查找模式，取值参考DPP_HASH_SRH_MODE的定义
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wcl      @date  2015/05/15
************************************************************/
DPP_STATUS dpp_hash_search(DPP_SE_CFG *p_se_cfg,
                           ZXIC_UINT32 fun_id,
                           DPP_HASH_ENTRY *p_entry,
                           ZXIC_UINT32 *p_space_vld,
                           ZXIC_UINT32 srh_mode)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT8 tbl_id = 0;
    ZXIC_UINT8 wr_flag = 0;
    ZXIC_UINT8 bulk_id = 0;
    ZXIC_UINT8 key_type = 0;
    ZXIC_UINT8 srh_succ = 0;
    ZXIC_UINT32 key_by_size = 0;
    ZXIC_UINT32 rst_by_size = 0;
    ZXIC_UINT8 temp_tbl_id = 0;
    ZXIC_UINT32 actu_key_size = 0;
    ZXIC_UINT8 temp_key[HASH_KEY_MAX] = {0};

    ZXIC_UINT16 crc16_value = 0;

    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 l = 0;
    ZXIC_UINT32 hw_addr = 0;
    ZXIC_UINT32 zcell_id = 0;
    ZXIC_UINT32 rsp_addr = 0;
    ZXIC_UINT32 zblk_idx = 0;
    ZXIC_UINT32 item_idx = 0;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 slot_id = 0;
    ZXIC_UINT32 item_type = 0;
    ZXIC_UINT32 crc32_value = 0;
    ZXIC_UINT32 pre_zblk_idx = 0xFFFFFFFF;/* -1; */

    D_NODE *p_zblk_dn = NULL;
    D_NODE *p_zcell_dn = NULL;
    SE_ITEM_CFG *p_item = NULL;
    SE_ZBLK_CFG *p_zblk = NULL;
    SE_ZCELL_CFG *p_zcell = NULL;
    HASH_DDR_CFG *p_ddr_cfg = NULL;
    DPP_HASH_CFG *p_hash_cfg = NULL;
    FUNC_ID_INFO *p_func_info = NULL;
    DPP_HASH_TBL_ID_INFO *p_tbl_id_info = NULL;
    DPP_HASH_WRT_LRN_RSP wrt_lrn_rsp = {0};

    ZXIC_COMM_CHECK_POINT(p_se_cfg);
    ZXIC_COMM_CHECK_INDEX(fun_id, HASH_FUNC_ID_MIN, HASH_FUNC_ID_NUM - 1);
    ZXIC_COMM_CHECK_POINT(p_entry);
    ZXIC_COMM_CHECK_POINT(p_space_vld);
    ZXIC_COMM_CHECK_INDEX(srh_mode, HASH_SRH_MODE_SOFT, HASH_SRH_MODE_HDW);
    ZXIC_COMM_CHECK_POINT(p_entry->p_key);
    ZXIC_COMM_CHECK_POINT(p_entry->p_rst);

    *p_space_vld = 0;
    dev_id = p_se_cfg->dev_id;
    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id,DPP_DEV_CHANNEL_MAX - 1);

    p_func_info = DPP_GET_FUN_INFO(p_se_cfg, fun_id);
    DPP_SE_CHECK_FUN(p_func_info, fun_id, FUN_HASH);

    p_hash_cfg = (DPP_HASH_CFG *)p_func_info->fun_ptr;
    wr_flag = DPP_GET_HASH_KEY_VALID(p_entry->p_key);

    tbl_id = DPP_GET_HASH_TBL_ID(p_entry->p_key);
    slot_id = p_se_cfg->dev.pcie_channel.slot;
    ZXIC_COMM_CHECK_INDEX_UPPER(slot_id,DPP_PCIE_SLOT_MAX - 1);
    p_tbl_id_info = GET_HASH_TBL_ID_INFO(slot_id, fun_id, tbl_id);

    key_type = DPP_GET_HASH_KEY_TYPE(p_entry->p_key);
    HASH_TBL_ID_INFO_CHECK(slot_id, fun_id, tbl_id, key_type);

    actu_key_size = GET_ACTU_KEY_SIZE_BY_TBLID(slot_id, fun_id, tbl_id);
    ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id, DPP_GET_ACTU_KEY_BY_SIZE(actu_key_size), HASH_KEY_CTR_SIZE);
    key_by_size = DPP_GET_KEY_SIZE(actu_key_size);
    ZXIC_COMM_CHECK_DEV_INDEX_SUB_OVERFLOW_NO_ASSERT(dev_id, DPP_GET_HASH_ENTRY_SIZE(key_type), DPP_GET_ACTU_KEY_BY_SIZE(actu_key_size));
    rst_by_size = DPP_GET_RST_SIZE(key_type, actu_key_size);

    /* Coverity err fixed (参数key_by_size(其值为0)传递下标越界给被调用方在-2处的数组) */
    if (key_by_size < (HASH_ACTU_KEY_MIN + HASH_KEY_CTR_SIZE) || key_by_size > HASH_KEY_MAX)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n dpp_hash_search key_by_size[%d] INVALID !\n", key_by_size);
        return ZXIC_PAR_CHK_INVALID_INDEX;
    }

    /* 查找时接收392bit的key格式为: wr_flag + key_type + tbl_id + reserve + actural_key,
     * 需要先将格式调整为:wr_flag + key_type + tbl_id + actural_key + reserve.
     */
    /* wr_flag + key_type + tbl_id */
    ZXIC_COMM_MEMCPY(temp_key, p_entry->p_key, 1);
    /* actural_key */
    ZXIC_COMM_MEMCPY(temp_key + 1, p_entry->p_key + 1 + (HASH_KEY_MAX - key_by_size), key_by_size - 1);

    /* 约定: 参与CRC运算的key, 需要将tbl_id的前面补3bit的0，然后放到actu_key的后面. */
    temp_tbl_id = temp_key[0] & 0x1F;
    memmove(&temp_key[0], &temp_key[1], key_by_size - HASH_KEY_CTR_SIZE);
    temp_key[key_by_size - HASH_KEY_CTR_SIZE] = temp_tbl_id;
    bulk_id = ((temp_tbl_id >> 2) & 0x7);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, bulk_id, 0, HASH_BULK_NUM - 1);


    /* [1]. search in DDR */
    if (p_hash_cfg->ddr_valid)
    {
        item_type = ITEM_DDR_256;
        p_ddr_cfg = p_hash_cfg->p_bulk_ddr_info[bulk_id];
        crc32_value = p_hash_cfg->p_hash32_fun(temp_key, key_by_size, p_ddr_cfg->hash_ddr_arg);
        item_idx = crc32_value % p_ddr_cfg->item_num;

        if (DDR_WIDTH_512b == p_ddr_cfg->width_mode)
        {
            item_idx = crc32_value % p_ddr_cfg->item_num;
            item_type = ITEM_DDR_512;
        }

        ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "Hash search in ITEM_DDR_%s, CRC32 index is: 0x%x.\n", ((item_type == ITEM_DDR_256) ? "256" : "512"), item_idx);

        if (HASH_SRH_MODE_HDW == srh_mode)
        {
            /* search hardware mode */
        }
        else
        {
            /* search software mode */
            ZXIC_COMM_CHECK_INDEX_UPPER(item_idx, p_ddr_cfg->item_num - 1);
            p_item = p_ddr_cfg->p_item_array[item_idx];

            if (p_item != NULL)
            {
                ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id, p_ddr_cfg->hw_baddr, item_idx);
                p_item->hw_addr = GET_HASH_DDR_HW_ADDR(p_ddr_cfg->hw_baddr, item_idx);
                p_item->item_type = item_type;
                p_item->item_index = item_idx;
                rc = dpp_hash_software_item_check(p_hash_cfg,
                                                  p_entry,
                                                  key_by_size,
                                                  rst_by_size,
                                                  p_item,
                                                  &wrt_lrn_rsp);

                if (DPP_OK == rc)
                {
                    srh_succ = 1;
                }
            }
            else
            {
                wrt_lrn_rsp.space_vld = 1;
                wrt_lrn_rsp.ext_flag = 1;
                wrt_lrn_rsp.wrt_mask = DPP_GET_HASH_ENTRY_MASK(DPP_GET_HASH_ENTRY_SIZE(key_type), 0);
                ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id, p_ddr_cfg->hw_baddr, item_idx);
                hw_addr = GET_HASH_DDR_HW_ADDR(p_ddr_cfg->hw_baddr, item_idx);

                if (ITEM_DDR_256 == item_type)
                {
                    wrt_lrn_rsp.width_flag = 0;
                    wrt_lrn_rsp.lrn_addr = hw_addr & ZXIC_COMM_GET_BIT_MASK(ZXIC_UINT32, HASH_ADDR_DDR_BT_LEN);
                }
                else
                {
                    wrt_lrn_rsp.width_flag = 1;
                    wrt_lrn_rsp.lrn_addr = (hw_addr) & ZXIC_COMM_GET_BIT_MASK(ZXIC_UINT32, HASH_ADDR_DDR_BT_LEN);
                }
            }
        }
    }

    if (!srh_succ)
    {
        /* [2]. search in ZCAM */
        item_type = ITEM_RAM;
        p_zcell_dn = p_hash_cfg->hash_shareram.zcell_free_list.p_next;

        while (p_zcell_dn)
        {
            p_zcell = (SE_ZCELL_CFG *)p_zcell_dn->data;
            zblk_idx = GET_ZBLK_IDX(p_zcell->zcell_idx);
            ZXIC_COMM_CHECK_DEV_INDEX(dev_id, zblk_idx, 0, SE_ZBLK_NUM - 1);
            p_zblk = &(p_se_cfg->zblk_info[zblk_idx]);

            if (zblk_idx != pre_zblk_idx)
            {
                pre_zblk_idx = zblk_idx;
                crc16_value = p_hash_cfg->p_hash16_fun(temp_key, key_by_size, p_zblk->hash_arg);
            }

            zcell_id = GET_ZCELL_IDX(p_zcell->zcell_idx);
            item_idx = GET_ZCELL_CRC_VAL(zcell_id, crc16_value);
            ZXIC_COMM_CHECK_DEV_INDEX(dev_id, item_idx, 0, SE_RAM_DEPTH - 1);

            ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "Hash search in ZCAM_RAM, zblk[%d], zcell[%d], CRC16 ram_index[0x%x].\n", zblk_idx, zcell_id, item_idx);

            if (HASH_SRH_MODE_HDW == srh_mode)
            {
                /* search hardware mode */
            }
            else
            {
                /* search software mode */
                p_item = &(p_zcell->item_info[item_idx]);
                p_item->hw_addr = ZBLK_ITEM_ADDR_CALC(p_zcell->zcell_idx, item_idx);
                p_item->item_type = item_type;
                p_item->item_index = item_idx;
                rc = dpp_hash_software_item_check(p_hash_cfg,
                                                  p_entry,
                                                  key_by_size,
                                                  rst_by_size,
                                                  p_item,
                                                  &wrt_lrn_rsp);

                if (DPP_OK == rc)
                {
                    srh_succ = 1;
                    break;
                }
            }

            p_zcell_dn = p_zcell_dn->next;
        }
    }

    if (!srh_succ)
    {
        /* [3]. search in ZBLK Reg */
        item_type = ITEM_REG;
        p_zblk_dn = p_hash_cfg->hash_shareram.zblk_list.p_next;

        while (p_zblk_dn)
        {
            p_zblk = (SE_ZBLK_CFG *)p_zblk_dn->data;
            zblk_idx = p_zblk->zblk_idx;

            for (i = 0; i < SE_ZREG_NUM; i++)
            {
                item_idx = i;
                ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "Hash search in ZCAM_REG, zblk[%d], reg_index[0x%x].\n", zblk_idx, item_idx);

                if (HASH_SRH_MODE_HDW == srh_mode)
                {
                    /* search hardware mode */
                }
                else
                {
                    /* search software mode */
                    p_item = &(p_zblk->zreg_info[i].item_info);
                    p_item->hw_addr = ZBLK_HASH_LIST_REG_ADDR_CALC(zblk_idx, item_idx);
                    p_item->item_type = item_type;
                    p_item->item_index = item_idx;
                    rc = dpp_hash_software_item_check(p_hash_cfg,
                                                      p_entry,
                                                      key_by_size,
                                                      rst_by_size,
                                                      p_item,
                                                      &wrt_lrn_rsp);

                    if (DPP_OK == rc)
                    {
                        srh_succ = 1;
                        break;
                    }
                }
            }

            if (srh_succ == 1)
            {
                break;
            }

            p_zblk_dn = p_zblk_dn->next;
        }
    }

    if (!srh_succ)
    {
        /* if hardware learn enable or mcode write table enable, return space valid and address. */
        if (p_tbl_id_info->is_lrn || (wr_flag && p_tbl_id_info->is_mc_wrt))
        {
            *p_space_vld = wrt_lrn_rsp.space_vld;
            rsp_addr = (((ZXIC_UINT32)(wrt_lrn_rsp.ext_flag & 0x1) << HASH_ADDR_EXT_FLAG_BT_OFF) |
                        ((ZXIC_UINT32)(wrt_lrn_rsp.wrt_mask & 0xF) << HASH_ADDR_WRT_MASK_BT_OFF) |
                        ((ZXIC_UINT32)wrt_lrn_rsp.lrn_addr << HASH_ADDR_BT_OFF) |
                        (wrt_lrn_rsp.width_flag & 0x1));

            zxic_comm_swap((ZXIC_UINT8 *)&rsp_addr, sizeof(ZXIC_UINT32));

            ZXIC_COMM_MEMSET_S(p_entry->p_rst, HASH_RST_MAX, 0, HASH_RST_MAX);
            ZXIC_COMM_MEMCPY(p_entry->p_rst, &rsp_addr, sizeof(ZXIC_UINT32));

            ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "search fun p_entry->p_rst is:");

            for (l = 0; l < 4; l++)
            {
                ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "0x%x ", p_entry->p_rst[l]);
            }

            ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "\n");
        }

        p_hash_cfg->hash_stat.search_fail++;
        ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "Hash search key fail!\n");
        return DPP_HASH_RC_SRH_FAIL;
    }
    #if 0
    if ((wr_flag) && (p_tbl_id_info->is_mc_wrt))
    {
        /* mcode update table */
        *p_space_vld = wrt_lrn_rsp.space_vld;
        rsp_addr = (((wrt_lrn_rsp.ext_flag & 0x1) << HASH_ADDR_EXT_FLAG_BT_OFF) |
                    ((wrt_lrn_rsp.wrt_mask & 0xF) << HASH_ADDR_WRT_MASK_BT_OFF) |
                    (wrt_lrn_rsp.lrn_addr << HASH_ADDR_BT_OFF) |
                    (wrt_lrn_rsp.width_flag & 0x1));

        zxic_comm_swap((ZXIC_UINT8 *)&rsp_addr, sizeof(ZXIC_UINT32));

        ZXIC_COMM_MEMSET(p_entry->p_rst, 0, HASH_RST_MAX);
        ZXIC_COMM_MEMCPY(p_entry->p_rst, &rsp_addr, sizeof(ZXIC_UINT32));

        ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "search fun p_entry->p_rst is:");

        for (l = 0; l < 4; l++)
        {
            ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "0x%x ", p_entry->p_rst[l]);
        }

        ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "\n");
    }
    #endif
    ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "Hash search successfully.\n");
    p_hash_cfg->hash_stat.search_ok++;

    return DPP_OK;
}

DPP_STATUS dpp_hash_tbl_clr(ZXIC_UINT32 dev_id)
{
    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id,DPP_DEV_CHANNEL_MAX - 1);

    ZXIC_COMM_MEMSET_S(&g_tbl_id_info[0][0][0], 
                     sizeof(g_tbl_id_info),
                     0, 
                     sizeof(g_tbl_id_info));

    return DPP_OK;
}
#endif
