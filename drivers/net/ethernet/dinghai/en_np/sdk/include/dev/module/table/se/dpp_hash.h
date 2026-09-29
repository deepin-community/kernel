/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_hash.h
* 文件标识 : 
* 内容摘要 : 
* 其它说明 : 
* 当前版本 : 
* 作    者 : wcl
* 完成日期 : 2014/02/14
* DEPARTMENT: ASIC_FPGA_R&D_Dept 
* MANUAL_PERCENT: 100%   
 
* 修改记录1: 
* 修改日期:  
* 版 本 号:  
* 修 改 人:  
* 修改内容:  
***************************************************************/

#ifndef _DPP_HASH_H_
#define _DPP_HASH_H_

#include "dpp_se_cfg.h"

#define HASH_FUNC_ID_MIN          (0)
#define HASH_FUNC_ID_NUM          (4)

#define HASH_DDR_CRC_NUM          (4)

#define HASH_KEY_MAX              (49) /* 最大键值长度，以字节为单位 */
#define HASH_RST_MAX              (32) /* 最大结果长度，以字节为单位 */
#define HASH_ENTRY_POS_STEP       (16) 

#define HASH_TBL_ID_NUM           (32) /* 每个Hash引擎中的最大业务表数目 */
#define HASH_BULK_NUM             (8)  /* 每个Hash引擎DDR资源划分的块数 */


#define HASH_ACTU_KEY_MIN         (1)  /* 业务实际键值长度 */
#define HASH_ACTU_KEY_MAX         (48)
#define HASH_ACTU_KEY_STEP        (1)  /* actual key的长度单位: 1字节 */
#define HASH_KEY_CTR_SIZE         (1)  /* key中控制信息的长度, 1字节 */
#define ITEM_ENTRY_NUM_2          (2)
#define ITEM_ENTRY_NUM_4          (4)

#define HASH_DDR_ITEM_MIN         (1<<14)
#define HASH_DDR_ITEM_MAX         (1<<26)

#define HASH_ZBLK_ID_MAX          (31)

/* hash ext cfg reg */
#define HASH_EXT_MODE_BT_START    (1)
#define HASH_EXT_MODE_BT_WIDTH    (8)
#define HASH_EXT_FLAG_BT_START    (0)
#define HASH_EXT_FLAG_BT_WIDTH    (1)

/* hash tbl30 depth reg */
#define HASH_TBL0_DEPTH_BT_START     (0)
#define HASH_TBL0_DEPTH_BT_WIDTH     (8)
#define HASH_TBL1_DEPTH_BT_START     (8)
#define HASH_TBL1_DEPTH_BT_WIDTH     (8)
#define HASH_TBL2_DEPTH_BT_START     (16)
#define HASH_TBL2_DEPTH_BT_WIDTH     (8)
#define HASH_TBL3_DEPTH_BT_START     (24)
#define HASH_TBL3_DEPTH_BT_WIDTH     (8)

/* hash tbl74 depth reg*/
#define HASH_TBL4_DEPTH_BT_START     (0)
#define HASH_TBL4_DEPTH_BT_WIDTH     (8)
#define HASH_TBL5_DEPTH_BT_START     (8)
#define HASH_TBL5_DEPTH_BT_WIDTH     (8)
#define HASH_TBL6_DEPTH_BT_START     (16)
#define HASH_TBL6_DEPTH_BT_WIDTH     (8)
#define HASH_TBL7_DEPTH_BT_START     (24)
#define HASH_TBL7_DEPTH_BT_WIDTH     (8)
  
/* hash ext crc cfg*/  
#define TBL0_EXT_CRC_CFG_BT_START    (0)
#define TBL0_EXT_CRC_CFG_BT_WIDTH    (2)
#define TBL1_EXT_CRC_CFG_BT_START    (2)
#define TBL1_EXT_CRC_CFG_BT_WIDTH    (2)
#define TBL2_EXT_CRC_CFG_BT_START    (4)
#define TBL2_EXT_CRC_CFG_BT_WIDTH    (2)
#define TBL3_EXT_CRC_CFG_BT_START    (6)
#define TBL3_EXT_CRC_CFG_BT_WIDTH    (2)
#define TBL4_EXT_CRC_CFG_BT_START    (8)
#define TBL4_EXT_CRC_CFG_BT_WIDTH    (2)
#define TBL5_EXT_CRC_CFG_BT_START    (10)
#define TBL5_EXT_CRC_CFG_BT_WIDTH    (2)
#define TBL6_EXT_CRC_CFG_BT_START    (12)
#define TBL6_EXT_CRC_CFG_BT_WIDTH    (2)
#define TBL7_EXT_CRC_CFG_BT_START    (14)
#define TBL7_EXT_CRC_CFG_BT_WIDTH    (2)

/* hash mono flags*/
#define HASH0_MONO_FLAG_BT_START     (0)
#define HASH0_MONO_FLAG_BT_WIDTH     (8)
#define HASH1_MONO_FLAG_BT_START     (8)
#define HASH1_MONO_FLAG_BT_WIDTH     (8)
#define HASH2_MONO_FLAG_BT_START     (16)
#define HASH2_MONO_FLAG_BT_WIDTH     (8)
#define HASH3_MONO_FLAG_BT_START     (24)
#define HASH3_MONO_FLAG_BT_WIDTH     (8)

/* hash zcell mono*/
#define ZCELL0_BULK_ID_BT_START      (2)
#define ZCELL0_BULK_ID_BT_WIDTH      (3)
#define ZCELL0_MONO_FLAG_BT_START    (3)
#define ZCELL0_MONO_FLAG_BT_WIDTH    (1)
#define ZCELL1_BULK_ID_BT_START      (10)
#define ZCELL1_BULK_ID_BT_WIDTH      (3)
#define ZCELL1_MONO_FLAG_BT_START    (11)
#define ZCELL1_MONO_FLAG_BT_WIDTH    (1)
#define ZCELL2_BULK_ID_BT_START      (18)
#define ZCELL2_BULK_ID_BT_WIDTH      (3)
#define ZCELL2_MONO_FLAG_BT_START    (19)
#define ZCELL2_MONO_FLAG_BT_WIDTH    (1)
#define ZCELL3_BULK_ID_BT_START      (26)
#define ZCELL3_BULK_ID_BT_WIDTH      (3)
#define ZCELL3_MONO_FLAG_BT_START    (27)
#define ZCELL3_MONO_FLAG_BT_WIDTH    (1)

/* hash zreg mono                         */
#define ZREG0_BULK_ID_BT_START       (2) 
#define ZREG0_BULK_ID_BT_WIDTH       (3) 
#define ZREG0_MONO_FLAG_BT_START     (3) 
#define ZREG0_MONO_FLAG_BT_WIDTH     (1) 
#define ZREG1_BULK_ID_BT_START       (10)
#define ZREG1_BULK_ID_BT_WIDTH       (3) 
#define ZREG1_MONO_FLAG_BT_START     (11)
#define ZREG1_MONO_FLAG_BT_WIDTH     (1) 
#define ZREG2_BULK_ID_BT_START       (18)
#define ZREG2_BULK_ID_BT_WIDTH       (3) 
#define ZREG2_MONO_FLAG_BT_START     (19)
#define ZREG2_MONO_FLAG_BT_WIDTH     (1) 
#define ZREG3_BULK_ID_BT_START       (26)
#define ZREG3_BULK_ID_BT_WIDTH       (3) 
#define ZREG3_MONO_FLAG_BT_START     (27)
#define ZREG3_MONO_FLAG_BT_WIDTH     (1) 

#define OPR_CLR                (0)
#define OPR_WR                 (1)

#define OBTAIN_CONFLICT_KEY    (0)

/* HASH soft reset*/
#define  HASH_ARG_NUM_PER_BULK (8)  /* 每个bulk 需要记录的参数数目 */
#define  HASH_ARG_NUM_PER_TBL  (4)  /* 每个表   需要记录的参数数目 */
#define  HASH_INIT_NUM         (8)  /* HASH引擎初始化参数数目 */
#define  HASH_BULK_INIT_NUM    (1+HASH_BULK_NUM*HASH_ARG_NUM_PER_BULK+3)   /* 1-bulk_valid +3是为了凑够4的倍数 */
#define  HASH_TBL_INIT_NUM     (1+HASH_TBL_ID_NUM*HASH_ARG_NUM_PER_TBL+3)  /* 1-tbl_valid  +3是为了凑够4的整数倍 */

typedef struct dpp_hash_table_stat
{
    ZXIC_UINT32 ddr;
    ZXIC_UINT32 zcell;
    ZXIC_UINT32 zreg;
    ZXIC_UINT32 sum;
    ZXIC_UINT32 same;
    ZXIC_UINT32 delete;
}DPP_HASH_TABLE_STAT;

typedef struct dpp_hash_zreg_mono_stat
{
    ZXIC_UINT32 zblk_id;
    ZXIC_UINT32 zreg_id;
}DPP_HASH_ZREG_MONO_STAT;

typedef struct dpp_hash_bulk_zcam_stat
{
    ZXIC_UINT32 zcell_mono_idx[SE_ZBLK_NUM*SE_ZCELL_NUM];
    DPP_HASH_ZREG_MONO_STAT zreg_mono_id[SE_ZBLK_NUM][SE_ZREG_NUM];
}DPP_HASH_BULK_ZCAM_STAT;

typedef struct dpp_hash_stat
{
    ZXIC_UINT32 insert_ok;
    ZXIC_UINT32 insert_fail;
    ZXIC_UINT32 insert_same;
    ZXIC_UINT32 insert_ddr;
    ZXIC_UINT32 insert_zcell;
    ZXIC_UINT32 insert_zreg;

    ZXIC_UINT32 delete_ok;
    ZXIC_UINT32 delete_fail;

    ZXIC_UINT32 search_ok;
    ZXIC_UINT32 search_fail;

    ZXIC_UINT32 zblock_num;
    ZXIC_UINT32 zblock_array[SE_ZBLK_NUM];

    DPP_HASH_TABLE_STAT insert_table[HASH_TBL_ID_NUM];
    DPP_HASH_BULK_ZCAM_STAT *p_bulk_zcam_mono[HASH_BULK_NUM];
}DPP_HASH_STAT;

typedef enum dpp_hash_itme_pos
{
    HASH_ITEM_POS_0   = 0,
    HASH_ITEM_POS_1   = 1,
    HASH_ITEM_POS_2   = 2,
    HASH_ITEM_POS_3   = 3,
    HASH_ITEM_POS_MAX = 4,
}DPP_HASH_ITME_POS;

typedef enum dpp_hash_item_inst_mode
{
    HASH_ITEM_INSERT_LAST = 0,
    HASH_ITEM_INSERT_1ST,
    HASH_ITEM_INSERT_NULL
}DPP_HASH_ITEM_INST_MODE;

/* hash 表项信息 */
typedef struct dpp_hash_tbl_info
{
    ZXIC_UINT32 fun_id;
    ZXIC_UINT32 actu_key_size;  /**<  @brief 实际键值长度，以1字节为单位 */
    ZXIC_UINT32 key_type;       /**<  @brief 表项长度类型: 1-128bit, 2-256bit, 3-512bit */
    ZXIC_UINT8 is_init;        /**<  @brief 是否初始化*/
    ZXIC_UINT8 mono_zcell;     /**<  @brief 是否有独占的zcell*/
    ZXIC_UINT8 zcell_num;      /**<  @brief 独占的zcell的数目*/
    ZXIC_UINT8 mono_zreg;     /**<  @brief 是否有独占的zcell*/
    ZXIC_UINT8 zreg_num;      /**<  @brief 独占的zcell的数目*/    
    ZXIC_UINT8 is_age;         /* 硬件老化标志，业务表支持硬件老化 */
    ZXIC_UINT8 is_lrn;         /* 硬件学习标志，业务表支持硬件学习 */
    ZXIC_UINT8 is_mc_wrt;      /* 微码写表标志，业务表支持微码写表 */
    //ZXIC_UINT8 pad[3];
}DPP_HASH_TBL_ID_INFO;

typedef struct dpp_hash_rbkey_info
{
    ZXIC_UINT8         key[HASH_KEY_MAX];
    ZXIC_UINT8         rst[HASH_RST_MAX];
    D_NODE       entry_dn;
    SE_ITEM_CFG  *p_item_info;
/*    ZXIC_UINT32       rb_idx;*/
    ZXIC_UINT32       entry_size;   /* 条目宽度，以字节为单位 */
    ZXIC_UINT32       entry_pos;    /* 条目在item中的起始位置，以128bit为偏移单位 */
}DPP_HASH_RBKEY_INFO;

typedef struct dpp_hash_zcam_pos_info
{
    ZXIC_UINT32       entry_addr;   /* 基本存储单元物理的地址 */
    ZXIC_UINT32       entry_size;   /* 条目宽度，以字节为单位 */
    ZXIC_UINT32       entry_pos;    /* 条目在item中的起始位置，以128bit为偏移单位 */
}DPP_HASH_ZCAM_POS_INFO;

/* DDR*/
typedef struct hash_ddr_cfg
{
    ZXIC_UINT32                  bulk_use;         /**< @brief 该hash引擎bulk空间是否已经使用*/
    ZXIC_UINT32                  ddr_baddr;        /**< @brief 分配给hash的DDR空间的硬件基地址*/
    ZXIC_UINT32                  ddr_ecc_en;       /**< @brief DDR ECC使能: 0-不使能，1-使能*/
    ZXIC_UINT32                  item_num;         /**< @brief 硬件分配的ddr存储单元数目，以位宽为单位*/
    ZXIC_UINT32                  bulk_id;          /**< @brief DDR空间编号*/
    ZXIC_UINT32                  hash_ddr_arg;     /**< @brief hash ddr CRC 计算式*/
    ZXIC_UINT32                  width_mode;       /**< @brief ddr3 位宽*/
    ZXIC_UINT32                  hw_baddr;         /**< @brief ddr3 存储单元起始偏移，以256bit为单位*/
    ZXIC_UINT32                  zcell_num;          /*cpu 软复位 存储记录该参数*/
    ZXIC_UINT32                  zreg_num;           /*cpu 软复位 存储记录该参数*/

    SE_ITEM_CFG             **p_item_array;   /**< @brief 指向数组指针的指针*/
}HASH_DDR_CFG;

#define HASH_ADDR_EXT_FLAG_BT_OFF    (31)
#define HASH_ADDR_WRT_MASK_BT_OFF    (27)
#define HASH_ADDR_BT_OFF             (1)
#define HASH_ADDR_DDR_BT_LEN         (26)
#define HASH_ADDR_ZCAM_BT_LEN        (17)
typedef struct dpp_hash_wrt_lrn_rsp
{
    ZXIC_UINT8   space_vld; /* 仅硬件学习时此标志有效 */
    ZXIC_UINT8   ext_flag;  /* 软件模拟硬件学习标志位*/
    ZXIC_UINT8   wrt_mask;
    ZXIC_UINT8   width_flag;
    ZXIC_UINT32 lrn_addr;
}DPP_HASH_WRT_LRN_RSP;

typedef struct dpp_hash_cfg
{
    ZXIC_UINT32             fun_id;
    ZXIC_UINT8               ddr_valid;     /* 是否使用DDR表项 */
    ZXIC_UINT8               pad[3];
    HASH_FUNCTION32    p_hash32_fun;
    HASH_FUNCTION      p_hash16_fun;

    HASH_DDR_CFG       *p_bulk_ddr_info[HASH_BULK_NUM];   /* 每个DDR空间的配置*/
    ZXIC_UINT8               bulk_ram_mono[HASH_BULK_NUM];      /* 每个ZCAM空间独占标志*/
    SHARE_RAM          hash_shareram; /* 共享的ZCAM资源 */
    DPP_SE_CFG         *p_se_info;

    ZXIC_RB_CFG         hash_rb;
    ZXIC_RB_CFG         ddr_cfg_rb;
    DPP_HASH_STAT      hash_stat;
}DPP_HASH_CFG;

typedef struct hash_entry_cfg
{
    ZXIC_UINT32 fun_id;
    ZXIC_UINT8 bulk_id;
    ZXIC_UINT8 table_id;
    ZXIC_UINT8 key_type;
    ZXIC_UINT8 rsp_mode;
    ZXIC_UINT32 actu_key_size;
    ZXIC_UINT32 key_by_size;
    ZXIC_UINT32 rst_by_size;
    DPP_SE_CFG *p_se_cfg;
    DPP_HASH_CFG *p_hash_cfg;
    DPP_HASH_RBKEY_INFO *p_rbkey_new;
    ZXIC_RB_TN *p_rb_tn_new;
}HASH_ENTRY_CFG;

#define DPP_GET_HASH_KEY_CTRL(valid, type, tbl_id)  (((valid & 0x1) << 7) | ((type & 0x3) << 5) | (tbl_id & 0x1f))
#define DPP_GET_HASH_TBL_ID(p_key)    ((p_key)[0] & 0x1F)
#define DPP_GET_HASH_KEY_TYPE(p_key)  (((p_key)[0] >> 5) & 0x3)
#define DPP_GET_HASH_KEY_VALID(p_key) (((p_key)[0] >> 7) & 0x1)

/** 根据hash的类型获取其字节数，返回值包括16B,32B和64B，或者0*/
#define DPP_GET_HASH_ENTRY_SIZE(key_type) \
    ((key_type == HASH_KEY_128b)?16U:       \
       ((key_type == HASH_KEY_256b)?32U:    \
         ((key_type == HASH_KEY_512b)?64U:0)))

#define DPP_GET_ACTU_KEY_BY_SIZE(actu_key_size) \
    (actu_key_size * HASH_ACTU_KEY_STEP)

#define DPP_GET_KEY_SIZE(actu_key_size) \
    (DPP_GET_ACTU_KEY_BY_SIZE(actu_key_size) + HASH_KEY_CTR_SIZE)
#define DPP_GET_RST_SIZE(key_type, actu_key_size) \
    ((DPP_GET_HASH_ENTRY_SIZE(key_type) != 0)? \
    (DPP_GET_HASH_ENTRY_SIZE(key_type) - DPP_GET_ACTU_KEY_BY_SIZE(actu_key_size) - HASH_KEY_CTR_SIZE): 0xFF)  /* modify coverity kfr 2022.05.31 */

#define DPP_GET_HASH_RB_KEY(p_hash_rb, idx) \
    ((p_hash_rb)->p_keybase + ((p_hash_rb)->key_size * (idx)))

#define DPP_GET_DDR_WR_MODE(key_type) ((key_type == HASH_KEY_512b)?key_type : (key_type - 1))

/** 根据条目位宽和起始的位置获取写入掩码 */
#define DPP_GET_HASH_ENTRY_MASK(entry_size, entry_pos) \
    ((((1U << (entry_size/16U)) - 1U) << (4U - entry_size/16U - entry_pos)) & 0xF)

DPP_STATUS dpp_hash_zblkcfg_write(DPP_SE_CFG *p_se_cfg, ZXIC_UINT32 fun_id, SE_ZBLK_CFG  *p_zblk_cfg);

DPP_STATUS dpp_hash_bulk_mono_flags_write(DPP_SE_CFG *p_se_cfg, ZXIC_UINT32 hash_id, ZXIC_UINT32 bulk_id);

DPP_STATUS dpp_hash_zcell_mono_write(DPP_SE_CFG *p_se_cfg, SE_ZCELL_CFG *p_zcell_cfg);

DPP_STATUS dpp_hash_zreg_mono_write(DPP_SE_CFG *p_se_cfg,  
                                    ZXIC_UINT32     tbl_id,
                                    ZXIC_UINT32     zblk_idx,
                                    ZXIC_UINT32     zreg_id);

DPP_STATUS dpp_hash_ext_cfg_write(DPP_SE_CFG   *p_se_cfg, 
                                  ZXIC_UINT32       fun_id,
                                  ZXIC_UINT32       bulk_id,
                                  HASH_DDR_CFG   *p_ddr_cfg);

DPP_STATUS dpp_hash_ext_cfg_clr(DPP_SE_CFG *p_se_cfg, ZXIC_UINT32 fun_id);

DPP_STATUS dpp_hash_tbl_depth_write(DPP_SE_CFG   *p_se_cfg, 
                                    ZXIC_UINT32       fun_id,
                                    ZXIC_UINT32       bulk_id,
                                    HASH_DDR_CFG   *p_ddr_cfg);

DPP_STATUS dpp_hash_tbl_depth_clr(DPP_SE_CFG *p_se_cfg, ZXIC_UINT32 fun_id);

DPP_STATUS dpp_hash_tbl_crc_poly_write(DPP_SE_CFG *p_se_cfg,
                                       ZXIC_UINT32 fun_id,
                                       ZXIC_UINT32 bulk_id,
                                       ZXIC_UINT32 crc_sel);

ZXIC_SINT32 dpp_hash_rb_key_cmp(ZXIC_VOID *p_new, ZXIC_VOID *p_old, ZXIC_UINT32 key_size);

DPP_STATUS dpp_hash_insrt_to_item(DPP_HASH_CFG *p_hash_cfg,
                                  DPP_HASH_RBKEY_INFO *p_rbkey,
                                  SE_ITEM_CFG *p_item,
                                  ZXIC_UINT32 item_idx,
                                  ZXIC_UINT32 item_type,
                                  ZXIC_UINT32 insrt_key_type);

DPP_STATUS dpp_hash_red_black_node_alloc(DPP_DEV_T *dev,ZXIC_RB_TN **p_rb_tn_new,DPP_HASH_RBKEY_INFO **p_rbkey_new);

DPP_STATUS dpp_hash_rb_insert(DPP_DEV_T *dev,HASH_ENTRY_CFG *p_hash_entry_cfg,DPP_HASH_ENTRY *p_entry);

DPP_STATUS dpp_hash_set_crc_key(DPP_DEV_T *dev,HASH_ENTRY_CFG *p_hash_entry_cfg,DPP_HASH_ENTRY *p_entry,ZXIC_UINT8 *p_temp_key);

DPP_STATUS dpp_hash_insert_ddr(DPP_DEV_T *dev,HASH_ENTRY_CFG *p_hash_entry_cfg,ZXIC_UINT8 *p_temp_key,ZXIC_UINT8 *p_end_flag);

DPP_STATUS dpp_hash_insert_zcell(DPP_DEV_T *dev,DPP_SE_CFG *p_se_cfg,HASH_ENTRY_CFG *p_hash_entry_cfg,ZXIC_UINT8 *p_temp_key,ZXIC_UINT8 *p_end_flag);

DPP_STATUS dpp_hash_insert_zreg(DPP_DEV_T *dev,HASH_ENTRY_CFG *p_hash_entry_cfg,ZXIC_UINT8 *p_temp_key,ZXIC_UINT8 *p_end_flag);

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
DPP_STATUS dpp_hash_soft_all_entry_delete(DPP_SE_CFG *p_se_cfg,ZXIC_UINT32 hash_id);

/***********************************************************/
/** 释放当前sdt下的所有hash流表表项(仅删除软件表项，不操作硬件)
* @param   dev_id  设备号 
* @param   sdt_no  sdt号 
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/11/05
************************************************************/
DPP_STATUS dpp_hash_soft_delete_by_sdt(DPP_DEV_T *dev,ZXIC_UINT32 sdt_no);

DPP_STATUS dpp_hash_get_hash_info_from_sdt(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no, HASH_ENTRY_CFG *p_hash_entry_cfg);

DPP_STATUS dpp_hash_soft_uninstall(DPP_DEV_T *dev);

DPP_STATUS dpp_one_hash_soft_uninstall(DPP_DEV_T *dev,ZXIC_UINT32 hash_id);

DPP_STATUS dpp_hash_tbl_clr(ZXIC_UINT32 dev_id);
#endif /* dpp_hash.h */
