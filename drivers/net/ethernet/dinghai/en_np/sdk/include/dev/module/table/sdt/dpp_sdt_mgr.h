/*********************************************************************
* 版权所有 (C)2013, 深圳市中兴通讯股份有限公司。
*
* 文件名称：
* 文件标识：
* 内容摘要:
* 其它说明:
*
*
* 当前版本：
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT   : 100%
* 完成日期： 2013-9-2
********************************************************************/

#ifndef _DPP_SDT_MGR_H_
#define _DPP_SDT_MGR_H_

#include "dpp_sdt_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DPP_SDT_CFG_LEN         (2)

#define DPP_SDT_VALID           (1)
#define DPP_SDT_INVALID         (0)

#define DPP_SDT_WRITE          (0)
#define DPP_SDT_READ           (1)

#define DPP_SDT_MAX_BUFF       (18)

#define DPP_TBL_DATA_MAX       (50)

typedef enum dpp_opr_type_e
{
    DPP_TABLE_UPDATE = 0,
    DPP_TABLE_DELETE = 1,
    DPP_TABLE_SEARCH = 2,
} DPP_OPR_TYEP_E;

/** eRam SDT变量获取中间结构 */
typedef struct dpp_eram128_params_t
{
    ZXIC_UINT32              tbl_base_addr;     /** 表项基地址，128bit为单位         */
    DPP_ERAM128_MODE_E  eram128_mode;      /** 表项位宽模式                     */
    ZXIC_UINT32              tbl_depth;         /** 表项深度值。越界检测             */
    ZXIC_UINT32              count;             /** 表示写入数据的长度，以32bit为单位*/
} DPP_ERAM128_PARAMS_T;

/** DDR3 SDT变量获取中间结构 */
typedef struct dpp_ddr3_params_t
{
    ZXIC_UINT32          base_addr;             /** 表项基地址，以4K*128bit 为单位   */
    ZXIC_UINT32          crc_check;             /** ECC校验使能位                     */
    DPP_DDR3_MODE_E ddr3_mode;             /** DDR位宽模式                       */
    ZXIC_UINT32          tbl_share_mode;        /** bank 共享模式                     */
    ZXIC_UINT32          wr_rd_count;           /** 表示写入数据的长度，以32bit为单位 */
} DPP_DDR3_PARAMS_T;

typedef enum dpp_hash_key_width_e
{
    HashKey_Invalid = 0,
    HashKey_128b,
    HashKey_256b,
    HashKey_512b,
    HashKey_MAX
} DPP_HASH_KEY_WIDTH_E;

/** HASH SDT变量获取中间结构 */
typedef struct dpp_hash_params_t
{
    ZXIC_UINT8      hash_id;              /** hash引擎号          */
    ZXIC_UINT8      key_tbl_width;        /** hash表项存储位宽,   */
    ZXIC_UINT8      key_size;             /** hash键值长度,以8bit为单位，1~48 */
    ZXIC_UINT8      table_id;             /** hash逻辑表号        */
    ZXIC_UINT8      rsp_mode;             /** 返回数据宽度    0:32  1:64  2:128  3:256*/
} DPP_HASH_PARAMS_T;

typedef struct dpp_lpm_params_t
{
    ZXIC_UINT8      v46_flag; /* 1:Ipv4, 0:Ipv6 */
    ZXIC_UINT8      count;    /* 以4字节为单位  */
    ZXIC_UINT8      pad[2];   /* 字节对齐       */
} DPP_LPM_PARAMS_T;

typedef struct dpp_etcam_params_t
{
    ZXIC_UINT8 id;
    ZXIC_UINT8 table_id;
    ZXIC_UINT8 key_mode;
    ZXIC_UINT8 rsp_mode;
    ZXIC_UINT8 as_en;
    ZXIC_UINT32 as_baddr;
    ZXIC_UINT8 as_rsp_mode;
} DPP_ETCAM_PARAMS_T;

typedef ZXIC_UINT32 (*dpp_sdt_mgr_smmu0_mux_fun_ptr)(ZXIC_UINT32 dev_id, DPP_SDT_SMMU0_T *p_sdt_smmu0);
typedef ZXIC_UINT32 (*dpp_sdt_mgr_smmu1_mux_fun_ptr)(ZXIC_UINT32 dev_id, DPP_SDT_SMMU1_T *p_sdt_smmu1);
typedef ZXIC_UINT32 (*dpp_sdt_mgr_hash_mux_fun_ptr)(ZXIC_UINT32  dev_id, DPP_SDT_HASH_T  *p_sdt_hash);
typedef ZXIC_UINT32 (*dpp_sdt_mgr_lpm_mux_fun_ptr)(ZXIC_UINT32  dev_id,  DPP_SDT_LPM_T   *p_sdt_lpm);
typedef ZXIC_UINT32 (*dpp_sdt_mgr_etcam_mux_fun_ptr)(ZXIC_UINT32  dev_id,  DPP_SDT_ETCAM_T   *p_sdt_etcam);

typedef struct dpp_sdt_item_t
{
    ZXIC_UINT32     valid;
    ZXIC_UINT32     table_cfg[DPP_SDT_CFG_LEN];
} DPP_SDT_ITEM_T;

typedef struct dpp_sdt_soft_table_t
{
    ZXIC_UINT32          device_id;
    DPP_SDT_ITEM_T  sdt_array[DPP_PCIE_SLOT_MAX][DPP_DEV_SDT_ID_MAX];
} DPP_SDT_SOFT_TABLE_T;

typedef struct dpp_sdt_mgr_t
{
    ZXIC_UINT32          channel_num;
    ZXIC_UINT32          is_init;
    DPP_SDT_SOFT_TABLE_T* sdt_tbl_array[DPP_DEV_CHANNEL_MAX];
    dpp_sdt_mgr_smmu0_mux_fun_ptr p_sdt_mgr_smmu0_mux;
    dpp_sdt_mgr_smmu1_mux_fun_ptr p_sdt_mgr_smmu1_mux;
    dpp_sdt_mgr_hash_mux_fun_ptr  p_sdt_mgr_hash_mux;
    dpp_sdt_mgr_lpm_mux_fun_ptr   p_sdt_mgr_lpm_mux;
    dpp_sdt_mgr_etcam_mux_fun_ptr p_sdt_mgr_etcam_mux;
} DPP_SDT_MGR_T;

ZXIC_UINT32 dpp_sdt_mgr_init(ZXIC_VOID);
ZXIC_UINT32 dpp_sdt_mgr_create(ZXIC_UINT32 dev_id);
ZXIC_UINT32 dpp_sdt_mgr_destroy(ZXIC_UINT32 dev_id);

DPP_STATUS dpp_sdt_mgr_sdt_item_add(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no, ZXIC_UINT32 sdt_hig32, ZXIC_UINT32 sdt_low32);
DPP_STATUS dpp_sdt_mgr_sdt_item_srh(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no, ZXIC_UINT32 *p_sdt_hig32, ZXIC_UINT32 *p_sdt_low32);
DPP_STATUS dpp_sdt_mgr_sdt_item_del(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no);

#ifdef __cplusplus
}
#endif

#endif
