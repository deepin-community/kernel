/*********************************************************************
* 版权所有 (C)2001, 深圳市中兴通讯股份有限公司。
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
* 作    者： 石金锋
* 完成日期：2012-09-21
********************************************************************/
#ifndef _DPP_SDT_DEF_H_
#define _DPP_SDT_DEF_H_


typedef enum   dpp_tbl_type_e
{
    TblType_Invalid = 0,
    TblType_Eram128 = 1,
    TblType_DDR3    = 2,
    TblType_HASH    = 3,
    TblType_LPM     = 4,
    TblType_eTcam   = 5,
    TblType_PORTTBL = 6,
    TblType_MAX     = 7
} DPP_TBL_TYPE_E;

typedef enum   dpp_eram128_mode_e
{
    Eram128Mode_1BITS   = 0,
    Eram128Mode_32BITS  = 1,
    Eram128Mode_64BITS  = 2,
    Eram128Mode_128BITS = 3,
    Eram128Mode_2BITS   = 4,/** Add By LiuShuo @ 2016年2月24日15:56:39*/
    Eram128Mode_4BITS   = 5,
    Eram128Mode_8BITS   = 6,
    Eram128Mode_16BITS  = 7,
    Eram128Mode_MAX     = 8
} DPP_ERAM128_MODE_E;

typedef enum   dpp_dde3_mode_e
{
    DDR3Mode_128BITS = 0,
    DDR3Mode_256BITS = 1,    
    DDR3Mode_512BITS = 2,
    DDR3Mode_MAX     = 3
} DPP_DDR3_MODE_E;
/** DDR3 共享模式 */
typedef enum   dpp_ddr3_share_mode_e
{
    DDR_SHARE_MODE_NONE = 0,    /** <@brief 不共享模式  */
    DDR_SHARE_MODE_1_2  = 1,    /** <@brief 1/2共享模式 */
    DDR_SHARE_MODE_1_4  = 2,    /** <@brief 1/4共享模式 */
    DDR_SHARE_MODE_1_8  = 3,    /** <@brief 1/8共享模式 */
    DDR_SHARE_MODE_MAX  = 4,    /** <@brief 不可用      */
} DPP_DDR3_SHARE_MODE_E;

typedef enum dpp_ddr3_copy_type_e
{
    DDR3_COPY_TYPE_INN_0  = 0,
    DDR3_COPY_TYPE_INN_2  = 1,
    DDR3_COPY_TYPE_INN_4  = 2,
    DDR3_COPY_TYPE_INN_8  = 3,
    DDR3_COPY_TYPE_OUT_0  = 4,
    DDR3_COPY_TYPE_OUT_1  = 5,
    DDR3_COPY_TYPE_OUT_2  = 7,
} DPP_DDR3_COPY_TYPE_E;

/**  仿真器表项操作特殊定制，仅用于操作仿真器设备*/
#define HASH_SIM_ADD_ADDR       (0xFFFFFFF4)
#define HASH_SIM_DEL_ADDR       (0xFFFFFFF8)
#define LPM_SIM_ADD_ADDR        (0xFFFFFFE4)
#define LPM_SIM_DEL_ADDR        (0xFFFFFFE8)
#define ETCAM_SIM_ADD_ADDR      (0xFFFFFFD4)
#define ETCAM_SIM_DEL_ADDR      (0xFFFFFFD8)

typedef struct dpp_sdt_smmu0_t
{
    ZXIC_UINT32    *p_data;            /*输入,输出数据地址       */
    ZXIC_UINT32     wr_rd_flag;        /*0: Write 1: Read        */
    ZXIC_UINT32     tbl_index;         /*表项索引                */
    ZXIC_UINT32     tbl_base_addr;     /*表项基地址              */
    ZXIC_UINT32     mode;              /*参见DPP_ERAM128_MODE_E  */
    ZXIC_UINT32     tbl_depth;         /*表项的深度，越界检测    */
} DPP_SDT_SMMU0_T;

typedef struct dpp_sdt_smmu1_t
{
    ZXIC_UINT32    *p_data;            /*输入,输出数据地址           */
    ZXIC_UINT32     crc_chk_en;        /*Crc效验使能                 */
    ZXIC_UINT32     wr_rd_flag;        /*0: Write 1: Read            */
    ZXIC_UINT32     ddr_share_type;    /*参见DPP_DDR3_SHARE_MODE_E   */
    ZXIC_UINT32     tbl_index;         /*表项索引                    */
    ZXIC_UINT32     tbl_base_addr;     /*DDR3片内基地址              */
    ZXIC_UINT32     ddr_mode;          /*参见DPP_DDR3_MODE_E         */
    ZXIC_UINT32     sdt_no;            /*SDT表号                     */
} DPP_SDT_SMMU1_T;

typedef struct dpp_sdt_hash_t
{
    ZXIC_UINT32     id;           /* hash引擎号 */
    ZXIC_UINT32     tbl_id;       /* hash业务号 */
    ZXIC_UINT32     *p_data;
    ZXIC_UINT32     addr;
    ZXIC_UINT32     length;       /* 数据长度，以4字节为单位 */
    ZXIC_UINT32     key_size;     /* 键值长度，以字节为单位 */
    ZXIC_UINT32     key_type;     /* 表项存储位宽 */
    ZXIC_UINT32     rsp_mode;
    ZXIC_UINT32     wr_rd_flag;   /* 0: Write 1: Read */
} DPP_SDT_HASH_T;

typedef struct dpp_sdt_lpm_t
{
    ZXIC_UINT32     *p_data;
    ZXIC_UINT32     addr;
    ZXIC_UINT32     length;       /* 数据长度，以4字节为单位 */
    ZXIC_UINT32     wr_rd_flag;   /* 0: Write 1: Read */
    ZXIC_UINT32     v46_flag;     /* 1:ipv4, 0:ipv6 */
} DPP_SDT_LPM_T;

typedef struct dpp_sdt_etcam_t
{
    ZXIC_UINT32     id;           /* etcam号 */
    ZXIC_UINT32     tbl_id;       /* 业务号 */
    ZXIC_UINT32     *p_data;
    ZXIC_UINT32     length;       /* 数据长度，以4字节为单位 */
    ZXIC_UINT32     addr;
    ZXIC_UINT32     wr_rd_flag;   /* 0: Write 1: Read */
    ZXIC_UINT32     key_mode;
    ZXIC_UINT32     rsp_mode;
    ZXIC_UINT32     as_en;
    ZXIC_UINT32     as_eram_baddr;
    ZXIC_UINT32     as_rsp_mode;
} DPP_SDT_ETCAM_T;

#endif



