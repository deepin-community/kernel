/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_se_api.h
* 文件标识 : se模块对外数据类型定义和接口函数声明
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : wcl
* 完成日期 : 2015/01/30
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/
#ifndef _DPP_SE_API_H_
#define _DPP_SE_API_H_

#if ZXIC_REAL("header file")
 #include "dpp_type_api.h"  /* xcx_0619 */
#endif

#if ZXIC_REAL("data struct define")

/** DPP+ 所有eram block 大小都变为2M  ~深度是16K */
#define SE_SMMU0_ERAM_BLOCK_NUM            (32)
#define SE_SMMU0_ERAM_ADDR_NUM_PER_BLOCK   (0x4000)
#define SE_SMMU0_ERAM_ADDR_NUM_TOTAL       (SE_SMMU0_ERAM_BLOCK_NUM * SE_SMMU0_ERAM_ADDR_NUM_PER_BLOCK)

#define DPP_DDR4_PER_BANK_BADDR_SETP       (0x800)/* 8G X16 */
#define DPP_DDR4_BANK_NUM                  (64)
#define DPP_DDR4_PER_BANK_BADDR_CONV(ddr4_phy_type) \
    (DPP_DDR4_PER_BANK_BADDR_SETP*(4U>>ddr4_phy_type))

/*普通上送上送类型*/
typedef enum dpp_dma_ecc_en_e
{
    DMA_ECC_DISABLE  = 0,
    DMA_ECC_ENABLE   = 1,
} DPP_DMA_ECC_EN_E;

/**  eRam表读请模式 */
typedef enum dpp_eram128_rd_clr_mode_e
{
    RD_MODE_HOLD     = 0,  /**<  @brief 正常读，读完数据不清空   */
    RD_MODE_CLEAR    = 1,  /**<  @brief 读清模式，返回eram中的值 */
} DPP_ERAM128_RD_CLR_MODE_E;

/**  eRam表cpu读写位宽模式*/
typedef enum dpp_eram128_opr_mode_e
{
    ERAM128_OPR_128b = 0,  /**<  @brief 128bit模式读写eRam, 支持128bit读清模式*/
    ERAM128_OPR_64b  = 1,  /**<  @brief 64bit模式读写eRam,  支持64bit  读清模式*/
    ERAM128_OPR_1b   = 2,  /**<  @brief 1bit模式读写eRam,   不支持1bit  读清模式*/
    ERAM128_OPR_32b  = 3   /**<  @brief 32bit模式读eRam,    不能用于写，支持32bit 读清模式*/
} DPP_ERAM128_OPR_MODE_E;

/**  eRam直接表位宽模式,也可用于eRam做结果表时的位宽  */
typedef enum dpp_eram128_tbl_mode_e
{
    ERAM128_TBL_1b   = 0,  /**<  @brief 1bit   eRam片内直接表  */
    ERAM128_TBL_32b  = 1,  /**<  @brief 32bit  eRam片内直接表  */
    ERAM128_TBL_64b  = 2,  /**<  @brief 64bit  eRam片内直接表  */
    ERAM128_TBL_128b = 3,  /**<  @brief 128bit eRam片内直接表  */
    ERAM128_TBL_2b   = 4,  /**<  @brief 2bit   eRam片内直接表  */
    ERAM128_TBL_4b   = 5,  /**<  @brief 4bit   eRam片内直接表  */
    ERAM128_TBL_8b   = 6,  /**<  @brief 8bit   eRam片内直接表  */
    ERAM128_TBL_16b  = 7   /**<  @brief 16bit  eRam片内直接表  */
} DPP_ERAM128_TBL_MODE_E;

/**  eRam查表通道*/
typedef enum smmu0_empty_type_e
{
    SMMU0_EMPTY_CLS0 = 0,  /**<  @brief PPU cluster0通道*/
    SMMU0_EMPTY_CLS1 = 1,  /**<  @brief PPU cluster1通道*/
    SMMU0_EMPTY_CLS2 = 2,  /**<  @brief PPU cluster2通道*/
    SMMU0_EMPTY_CLS3 = 3,  /**<  @brief PPU cluster3通道*/
    SMMU0_EMPTY_CLS4 = 4,  /**<  @brief PPU cluster4通道*/
    SMMU0_EMPTY_CLS5 = 5,  /**<  @brief PPU cluster5通道*/
    SMMU0_EMPTY_CLS6 = 6,  /**<  @brief PPU cluster6通道*/
    SMMU0_EMPTY_CLS7 = 7,  /**<  @brief PPU cluster7通道*/
    SMMU0_EMPTY_MCAST = 8, /**<  @brief 组播复制表通道*/
    SMMU0_EMPTY_ODMA  = 9, /**<  @brief ODMA保序计数通道*/
} SMMU0_EMPTY_TYPE_E;

/**  组播报文类型*/
typedef enum smmu0_mcast_tbl_type_e
{
    MCAST_TDM = 0,         /**<  @brief TDM组播报文*/
    MCAST_DATA = 1,        /**<  @brief 数据组播报文*/

    MCAST_INVALID,         /**<  @brief 无效值 */
} SMMU0_MCAST_TBL_TYPE_E;

/** 调试计数读清模式 */
typedef enum se_dbg_cnt_read_mode_e
{
    SE_DBG_CNT_READ_UNCLR = 0,  /**<  @brief 非读清模式 */
    SE_DBG_CNT_READ_CLR,        /**<  @brief 读清模式 */
} SE_DBG_CNT_READ_MODE_E;

/** 调试计数溢出模式 */
typedef enum se_dbg_cnt_overflow_mode_e
{
    SE_DBG_CNT_OVERFLOW_UNREVERSE = 0,  /**<  @brief 溢出保持 */
    SE_DBG_CNT_OVERFLOW_REVERSE,        /**<  @brief 溢出翻转 */
} SE_DBG_CNT_OVERFLOW_MODE_E;

/**  DDR颗粒类型*/
typedef enum se_ddr_phy_cfg_type_e
{
    DPP_SE_DDR_PHY_32G_16 = 0,  /**<  @brief DDR颗粒每组32G容量,x16颗粒*/
    DPP_SE_DDR_PHY_16G_16,      /**<  @brief DDR颗粒每组16G容量,x16颗粒*/
    DPP_SE_DDR_PHY_8G_16,       /**<  @brief DDR颗粒每组8G容量,x16颗粒*/
    DPP_SE_DDR_PHY_4G_16,       /**<  @brief DDR颗粒每组4G容量,x16颗粒*/
    DPP_SE_DDR_PHY_32G_8,       /**<  @brief DDR颗粒每组32G容量,x8颗粒*/
    DPP_SE_DDR_PHY_16G_8,       /**<  @brief DDR颗粒每组16G容量,x8颗粒*/
    DPP_SE_DDR_PHY_8G_8,        /**<  @brief DDR颗粒每组8G容量,x8颗粒*/
    DPP_SE_DDR_PHY_4G_8,        /**<  @brief DDR颗粒每组4G容量,x8颗粒*/
    DPP_SE_DDR_PHY_MAX,
} SE_DDR_PHY_CFG_TYPE_E;

/**  DDR表写数据位宽模式*/
typedef enum smmu1_ddr_wrt_mode_e
{
    SMMU1_DDR_WRT_128b = 0,  /**<  @brief 128bit模式写DDR*/
    SMMU1_DDR_WRT_256b = 1,  /**<  @brief 256bit模式写DDR*/
    SMMU1_DDR_WRT_384b = 2,  /**<  @brief 384bit模式写DDR*/
    SMMU1_DDR_WRT_512b = 3,  /**<  @brief 512bit模式写DDR*/
} SMMU1_DDR_WRT_MODE_E;

/** DDR表的共享模式 */
typedef enum smmu1_ddr_share_mode_e
{
    SMMU1_DDR_SHARE_NO_SHARE    = 0,    /**<  @brief SE 占用整个DDR*/
    SMMU1_DDR_SHARE_1_2         = 1,    /**<  @brief SE 占用1/2 DDR*/
    SMMU1_DDR_SHARE_1_4         = 2,    /**<  @brief SE 占用1/4 DDR*/
    SMMU1_DDR_SHARE_1_8         = 3,    /**<  @brief SE 占用1/8 DDR*/
    SMMU1_DDR_SHARE_MAX
} SMMU1_DDR_SHARE_MODE_E;

/** 占用smmu1的表类型 */
typedef enum smmu1_ddr_tbl_type_e
{
    SMMU1_DDR_TBL_TYPE_DIR_TBL         = 0,    /**<  @brief 片外直接表 */
    SMMU1_DDR_TBL_TYPE_HASH_TBL        = 1,    /**<  @brief HASH占用表 */
    SMMU1_DDR_TBL_TYPE_LPM_TBL         = 2,    /**<  @brief LPM 占用表  */
    SMMU1_DDR_TBL_TYPE_OAM_TBL         = 3,    /**<  @brief OAM 占用表  */
    SMMU1_DDR_TBL_TYPE_FTM_TBL         = 4,    /**<  @brief FTM 占用表  */
    SMMU1_DDR_TBL_TYPE_ETM_TBL         = 5,    /**<  @brief ETM 占用表  */
    SMMU1_DDR_TBL_TYPE_MAX
} SMMU1_DDR_TBL_TYPE_E;

typedef enum cmmu_rd_mode_e
{
    CMMU_RD_MODE_29_35  = 0,
    CMMU_RD_MODE_32     = 1,
    CMMU_RD_MODE_64     = 2,
    CMMU_RD_MODE_128    = 3,
    CMMU_RD_MODE_MAX,
} CMMU_RD_OPR_MODE_E;

/**  CMMU读请模式 */
typedef enum cmmu_rd_clr_mode_e
{
    CMMU_RD_CLR_MODE_UNCLR     = 0,  /**<  @brief 正常读，读完数据不清空   */
    CMMU_RD_CLR_MODE_CLR       = 1,  /**<  @brief 读清模式*/
    CMMU_RD_CLR_MODE_MAX,
} CMMU_RD_CLR_MODE_E;

/**  DDR表读数据位宽模式*/
typedef enum smmu1_ddr_srh_mode_e
{
    SMMU1_DDR_SRH_128b  = 0, /**<  @brief 访问DDR，以128bit模式返回数据*/
    SMMU1_DDR_SRH_256b,      /**<  @brief 访问DDR，以256bit模式返回数据*/
    SMMU1_DDR_SRH_512b,      /**<  @brief 访问DDR，以512bit模式返回数据*/
    SMMU1_DDR_SRH_MAX
} SMMU1_DDR_SRH_MODE_E;

/**  DDR地址映射参数*/
typedef struct se_ddr_addr_map_info_t
{
    ZXIC_UINT32 is_inited;            /**<  @brief 用于检测此结构体是否已经经过初始化 0xAA-已经初始化 其他值-未初始化 */
    ZXIC_UINT32 ddr_phy_type;         /**<  @brief DDR颗粒类型 0-32G,x16颗粒,1-16G,x16颗粒,2-8G,x16颗粒 */
    ZXIC_UINT32 se_use_bank_num;      /**<  @brief SE使用的bank数目 */
    ZXIC_UINT32 bank_used_sate[64];   /**<  @brief 表示64个bank的使用状态 0-不存在 1-SE使用 2-PUB使用 3-FTM使用 4-ETM使用  */
} SE_DDR_ADDR_MAP_INFO_T;

/**  se从ppu接收的查表请求排空状态*/
typedef struct dpp_se_ept_flag_t
{
    ZXIC_UINT32 ppu5_ept_flag; /**<  @brief se从ppu通道5接收的查表请求排空状态*/
    ZXIC_UINT32 ppu4_ept_flag; /**<  @brief se从ppu通道4接收的查表请求排空状态*/
    ZXIC_UINT32 ppu3_ept_flag; /**<  @brief se从ppu通道3接收的查表请求排空状态*/
    ZXIC_UINT32 ppu2_ept_flag; /**<  @brief se从ppu通道2接收的查表请求排空状态*/
    ZXIC_UINT32 ppu1_ept_flag; /**<  @brief se从ppu通道1接收的查表请求排空状态*/
    ZXIC_UINT32 ppu0_ept_flag; /**<  @brief se从ppu通道0接收的查表请求排空状态*/
} DPP_SE_EPT_FLAG_T;

/**  smmu0(eRam)各通道查表请求排空状态*/
typedef struct dpp_smmu0_ept_flag_t
{
    ZXIC_UINT32 ept_flag8; /**<  @brief odam请求排空状态*/
    ZXIC_UINT32 ept_flag7; /**<  @brief odam tdm请求排空状态*/
    ZXIC_UINT32 ept_flag6; /**<  @brief sa组播份数表请求排空状态*/
    ZXIC_UINT32 ept_flag5; /**<  @brief ppu通道5查表请求排空状态*/
    ZXIC_UINT32 ept_flag4; /**<  @brief ppu通道4查表请求排空状态*/
    ZXIC_UINT32 ept_flag3; /**<  @brief ppu通道3查表请求排空状态*/
    ZXIC_UINT32 ept_flag2; /**<  @brief ppu通道2查表请求排空状态*/
    ZXIC_UINT32 ept_flag1; /**<  @brief ppu通道1查表请求排空状态*/
    ZXIC_UINT32 ept_flag0; /**<  @brief ppu通道0查表请求排空状态*/
} DPP_SMMU0_EPT_FLAG_T;

/**  SMMU0保序FIFO写满读空中断原因*/
typedef struct dpp_se_smmu0_int0_t
{
    ZXIC_UINT32 dma_ordfifo;    /**<  @brief DMA保序fifo 写满读空标志*/
    ZXIC_UINT32 odma_ordfifo;   /**<  @brief ODMA保序fifo 写满读空标志*/
    ZXIC_UINT32 mcast_ordfifo;  /**<  @brief MCAST保序fifo 写满读空标志*/
} DPP_SE_SMMU0_INT0_T;

/* 数据组播报文类型 */
typedef struct data_mcast_t_
{
    ZXIC_UINT8   valid;             /**< @brief valid 表示是否可用 */
    ZXIC_UINT8   rsv;               /**< @brief rsv   保留位，字节对齐使用,勿需关心 */
    ZXIC_UINT16 mc_cnt;            /**< @brief mc_cnt 16bit */
} DATA_MCAST_T;

/* TDM 组播报文类型 */
typedef struct tdm_mcast_t_
{
    ZXIC_UINT8   valid;             /**< @brief valid 表示是否可用 */
    ZXIC_UINT8   rsv;               /**< @brief rsv   保留位，字节对齐使用，勿需关心 */
    ZXIC_UINT8   bitmap[6];         /**< @brief bitmap[0]是最高位，bitmap[2]是最低位 */
} TDM_MCAST_T;

#endif

#if ZXIC_REAL("eTcam data struct define")

#endif

#if ZXIC_REAL("interrupt data struct define")
/** SMMU0 中断状态 */
typedef struct dpp_se_smmu0_int_t
{
    ZXIC_UINT32 smmu0_int0;
    ZXIC_UINT32 smmu0_int1;
    ZXIC_UINT32 smmu0_int2;
    ZXIC_UINT32 smmu0_int3;
    ZXIC_UINT32 smmu0_int4;
    ZXIC_UINT32 smmu0_int5;
    ZXIC_UINT32 smmu0_int6;
    ZXIC_UINT32 smmu0_int7;
    ZXIC_UINT32 smmu0_int8;
    ZXIC_UINT32 smmu0_int9;
    ZXIC_UINT32 smmu0_int10;
    ZXIC_UINT32 smmu0_int11;
    ZXIC_UINT32 smmu0_int12;
    ZXIC_UINT32 smmu0_int13;
    ZXIC_UINT32 smmu0_int14;
    ZXIC_UINT32 smmu0_int15;
    ZXIC_UINT32 smmu0_int16;
    ZXIC_UINT32 smmu0_int17;
    ZXIC_UINT32 smmu0_int18;
    ZXIC_UINT32 smmu0_int19;
    ZXIC_UINT32 smmu0_int20;
    ZXIC_UINT32 smmu0_int21;
    ZXIC_UINT32 smmu0_int22;
    ZXIC_UINT32 smmu0_int23;
    ZXIC_UINT32 smmu0_int24;
    ZXIC_UINT32 smmu0_int25;
    ZXIC_UINT32 smmu0_int26;
    ZXIC_UINT32 smmu0_int27;
    ZXIC_UINT32 smmu0_int28;
    ZXIC_UINT32 smmu0_int29;
    ZXIC_UINT32 smmu0_int30;
    ZXIC_UINT32 smmu0_int31;
    ZXIC_UINT32 smmu0_int32;
    ZXIC_UINT32 smmu0_int33;
    ZXIC_UINT32 smmu0_int34;
    ZXIC_UINT32 smmu0_int35;
    ZXIC_UINT32 smmu0_int36;
    ZXIC_UINT32 smmu0_int37;
    ZXIC_UINT32 smmu0_int38;

}DPP_SE_SMMU0_INT_T;

/** SMMU0 模块总的中断状态 */
typedef struct dpp_smmu0_brief_int_t
{
    ZXIC_UINT32 smmu0_int14_unmask_flag;
    ZXIC_UINT32 smmu0_int13_unmask_flag;
    ZXIC_UINT32 smmu0_int12_unmask_flag;
    ZXIC_UINT32 smmu0_int11_unmask_flag;
    ZXIC_UINT32 smmu0_int10_unmask_flag;
    ZXIC_UINT32 smmu0_int9_unmask_flag;
    ZXIC_UINT32 smmu0_int8_unmask_flag;
    ZXIC_UINT32 smmu0_int7_unmask_flag;
    ZXIC_UINT32 smmu0_int6_unmask_flag;
    ZXIC_UINT32 smmu0_int5_unmask_flag;
    ZXIC_UINT32 smmu0_int4_unmask_flag;
    ZXIC_UINT32 smmu0_int3_unmask_flag;
    ZXIC_UINT32 smmu0_int2_unmask_flag;
    ZXIC_UINT32 smmu0_int1_unmask_flag;
    ZXIC_UINT32 smmu0_int0_unmask_flag;
} DPP_SMMU0_BRIEF_INT_T;

/** SE 模块总的中断状态 */
typedef struct dpp_se_int_status_t
{
    ZXIC_UINT32 as_int_unmask_flag;      /**< @brief 级联查找模块（AS）总中断状态 */
    ZXIC_UINT32 kschd_int_unmask_flag;   /**< @brief 键值调度模块（KSCHD）总中断状态 */
    ZXIC_UINT32 rschd_int_unmask_flag;   /**< @brief 返回调度模块（RSCHD）总中断状态 */
    ZXIC_UINT32 smmu1_int_unmask_flag;  /**< @brief SMMU1 中断总中断状态 */
    ZXIC_UINT32 cmmu_int_unmask_flag;    /**< @brief CMMU  总中断状态 */
    ZXIC_UINT32 parser_int_unmask_flag;  /**< @brief 解析模块（parse）总中断状态 */
} DPP_SE_INT_STATUS_T;

/** se解析模块中断状态 */
typedef struct dpp_se_parser_int_t
{
    ZXIC_UINT32 parser_int_en;
    ZXIC_UINT32 parser_int_mask;
    ZXIC_UINT32 parser_int_status;
} DPP_SE_PARSER_INT_T;

/** key调度模块(KSCHD)中断状态 */
typedef struct dpp_se_kschd_int_t
{
    ZXIC_UINT32 kschd_int_0;
    ZXIC_UINT32 kschd_int_1;
    ZXIC_UINT32 kschd_int_2;
    ZXIC_UINT32 kschd_int_3;
    ZXIC_UINT32 kschd_int_4;
} DPP_SE_KSCHD_INT_T;

/** rsp调度模块(RSCHD)中断状态 */
typedef struct dpp_se_rschd_int_t
{
    ZXIC_UINT32 port0_int;
    ZXIC_UINT32 port1_int;
} DPP_SE_RSCHD_INT_T;

/**  关联查找(AS)模块的中断原因*/
typedef struct dpp_se_as_int_t
{
    ZXIC_UINT32 as_int_0;
    ZXIC_UINT32 as_int_1;
    ZXIC_UINT32 as_int_2;
} DPP_SE_AS_INT_T;

/** cmmu 模块中断*/
typedef struct dpp_se_cmmu_int_t
{
    ZXIC_UINT32 cmmu_int12;
    ZXIC_UINT32 cmmu_int11;
    ZXIC_UINT32 cmmu_int10;
    ZXIC_UINT32 cmmu_int9;
    ZXIC_UINT32 cmmu_int8;
    ZXIC_UINT32 cmmu_int7;
    ZXIC_UINT32 cmmu_int6;
    ZXIC_UINT32 cmmu_int5;
    ZXIC_UINT32 cmmu_int4;
    ZXIC_UINT32 cmmu_int3;
    ZXIC_UINT32 cmmu_int2;
    ZXIC_UINT32 cmmu_int1;
    ZXIC_UINT32 cmmu_int0;
} DPP_SE_CMMU_INT_T;

/** ALG 模块中断*/
typedef struct dpp_se_alg_int_t
{
    ZXIC_UINT32 wr_rsp_fifo_ovfl_int;              /**< @brief 写返回缓存ptr fifo 溢出中断上报 */
    ZXIC_UINT32 init_rd_cft_int;                /**< @brief 初始化过程中出现读命令，冲突中断 */
    ZXIC_UINT32 schd_lpm_fifo_parity_err_int;     /**< @brief 调度lpm缓存FIFO奇偶校验错误上报 */
    ZXIC_UINT32 schd_hash3_fifo_parity_err_int;    /**< @brief 调度hash3缓存FIFO奇偶校验错误上报 */
    ZXIC_UINT32 schd_hash2_fifo_parity_err_int;    /**< @brief 调度hash2缓存FIFO奇偶校验错误上报 */
    ZXIC_UINT32 schd_hash1_fifo_parity_err_int;    /**< @brief 调度hash1缓存FIFO奇偶校验错误上报 */
    ZXIC_UINT32 schd_hash0_fifo_parity_err_int;    /**< @brief 调度hash0缓存FIFO奇偶校验错误上报 */
    ZXIC_UINT32 schd_learn_fifo_parity_err_int;    /**< @brief 调度学习缓存FIFO奇偶校验错误上报 */
    ZXIC_UINT32 schd_lpm_fifo_ovfl_int;            /**< @brief 调度lpm键值缓存FIFO溢出中断上报 */
    ZXIC_UINT32 schd_hash3_fifo_ovfl_int;          /**< @brief 调度hash3键值缓存FIFO溢出中断上报 */
    ZXIC_UINT32 schd_hash2_fifo_unfl_int;          /**< @brief 调度hash2键值缓存FIFO溢出中断上报 */
    ZXIC_UINT32 schd_hash1_fifo_ovfl_int;          /**< @brief 调度hash1键值缓存FIFO溢出中断上报 */
    ZXIC_UINT32 schd_hash0_fifo_ovfl_int;          /**< @brief 调度hash0键值缓存FIFO溢出中断上报 */
    ZXIC_UINT32 schd_learn_fifo_ovfl_int;          /**< @brief 调度学习命令缓存FIFO溢出中断上报 */

    ZXIC_UINT32 zblk31_parity_int;           /**< @brief zblock31  parity 中断 */
    ZXIC_UINT32 zblk30_parity_int;           /**< @brief zblock30  parity 中断 */
    ZXIC_UINT32 zblk29_parity_int;           /**< @brief zblock29  parity 中断 */
    ZXIC_UINT32 zblk28_parity_int;           /**< @brief zblock28  parity 中断 */
    ZXIC_UINT32 zblk27_parity_int;           /**< @brief zblock27  parity 中断 */
    ZXIC_UINT32 zblk26_parity_int;           /**< @brief zblock26  parity 中断 */
    ZXIC_UINT32 zblk25_parity_int;           /**< @brief zblock25  parity 中断 */
    ZXIC_UINT32 zblk24_parity_int;           /**< @brief zblock24  parity 中断 */
    ZXIC_UINT32 zblk23_parity_int;           /**< @brief zblock23  parity 中断 */
    ZXIC_UINT32 zblk22_parity_int;           /**< @brief zblock22  parity 中断 */
    ZXIC_UINT32 zblk21_parity_int;           /**< @brief zblock21  parity 中断 */
    ZXIC_UINT32 zblk20_parity_int;           /**< @brief zblock20  parity 中断 */
    ZXIC_UINT32 zblk19_parity_int;           /**< @brief zblock19  parity 中断 */
    ZXIC_UINT32 zblk18_parity_int;           /**< @brief zblock18  parity 中断 */
    ZXIC_UINT32 zblk17_parity_int;           /**< @brief zblock17  parity 中断 */
    ZXIC_UINT32 zblk16_parity_int;           /**< @brief zblock16  parity 中断 */
    ZXIC_UINT32 zblk15_parity_int;           /**< @brief zblock15  parity 中断 */
    ZXIC_UINT32 zblk14_parity_int;           /**< @brief zblock14  parity 中断 */
    ZXIC_UINT32 zblk13_parity_int;           /**< @brief zblock13  parity 中断 */
    ZXIC_UINT32 zblk12_parity_int;           /**< @brief zblock12  parity 中断 */
    ZXIC_UINT32 zblk11_parity_int;           /**< @brief zblock11  parity 中断 */
    ZXIC_UINT32 zblk10_parity_int;           /**< @brief zblock10  parity 中断 */
    ZXIC_UINT32 zblk9_parity_int;            /**< @brief zblock9   parity 中断 */
    ZXIC_UINT32 zblk8_parity_int;            /**< @brief zblock8   parity 中断 */
    ZXIC_UINT32 zblk7_parity_int;            /**< @brief zblock7   parity 中断 */
    ZXIC_UINT32 zblk6_parity_int;            /**< @brief zblock6   parity 中断 */
    ZXIC_UINT32 zblk5_parity_int;            /**< @brief zblock5   parity 中断 */
    ZXIC_UINT32 zblk4_parity_int;            /**< @brief zblock4   parity 中断 */
    ZXIC_UINT32 zblk3_parity_int;            /**< @brief zblock3   parity 中断 */
    ZXIC_UINT32 zblk2_parity_int;            /**< @brief zblock2   parity 中断 */
    ZXIC_UINT32 zblk1_parity_int;            /**< @brief zblock1   parity 中断 */
    ZXIC_UINT32 zblk0_parity_int;            /**< @brief zblock0   parity 中断 */

    ZXIC_UINT32 zcam_hash_p0_err_int;                 /**< @brief hash0 ZCAM查找出错中断 */
    ZXIC_UINT32 hash0_agree_int_fifo_ovf_int;         /**< @brief hash0 汇聚fifo 片内通道溢出中断 */
    ZXIC_UINT32 hash0_agree_ext_fifo_ovf_int;         /**< @brief hash0 汇聚fifo 片外通道parity中断 */
    ZXIC_UINT32 hash0_agree_ext_fifo_parity_err_int;  /**< @brief hash0 汇聚fifo 片外通道溢出中断*/
    ZXIC_UINT32 hash0_agree_int_fifo_parity_err_int;  /**< @brief hash0 汇聚fifo 片内通道parity中断 */
    ZXIC_UINT32 hash0_key_fifo_ovfl_int;              /**< @brief hash0 片外键值缓存FIFO 溢出中断 */
    ZXIC_UINT32 hash0_sreq_fifo_ovfl_int;             /**< @brief hash0 片外读地址缓存FIFO溢出中断 */
    ZXIC_UINT32 hash0_key_fifo_parity_err_int;        /**< @brief hash0 片外键值缓存FIFO parity err中断 */

    ZXIC_UINT32 zcam_hash_p1_err_int;                 /**< @brief hash1 ZCAM查找出错中断 */
    ZXIC_UINT32 hash1_agree_int_fifo_ovf_int;         /**< @brief hash1 汇聚fifo 片内通道溢出中断 */
    ZXIC_UINT32 hash1_agree_ext_fifo_ovf_int;         /**< @brief hash1 汇聚fifo 片外通道parity中断 */
    ZXIC_UINT32 hash1_agree_ext_fifo_parity_err_int;  /**< @brief hash1 汇聚fifo 片外通道溢出中断*/
    ZXIC_UINT32 hash1_agree_int_fifo_parity_err_int;  /**< @brief hash1 汇聚fifo 片内通道parity中断 */
    ZXIC_UINT32 hash1_key_fifo_ovfl_int;              /**< @brief hash1 片外键值缓存FIFO 溢出中断 */
    ZXIC_UINT32 hash1_sreq_fifo_ovfl_int;             /**< @brief hash1 片外读地址缓存FIFO溢出中断 */
    ZXIC_UINT32 hash1_key_fifo_parity_err_int;        /**< @brief hash1 片外键值缓存FIFO parity err中断 */

    ZXIC_UINT32 zcam_hash_p2_err_int;                 /**< @brief hash2 ZCAM查找出错中断 */
    ZXIC_UINT32 hash2_agree_int_fifo_ovf_int;         /**< @brief hash2 汇聚fifo 片内通道溢出中断 */
    ZXIC_UINT32 hash2_agree_ext_fifo_ovf_int;         /**< @brief hash2 汇聚fifo 片外通道parity中断 */
    ZXIC_UINT32 hash2_agree_ext_fifo_parity_err_int;  /**< @brief hash2 汇聚fifo 片外通道溢出中断*/
    ZXIC_UINT32 hash2_agree_int_fifo_parity_err_int;  /**< @brief hash2 汇聚fifo 片内通道parity中断 */
    ZXIC_UINT32 hash2_key_fifo_ovfl_int;              /**< @brief hash2 片外键值缓存FIFO 溢出中断 */
    ZXIC_UINT32 hash2_sreq_fifo_ovfl_int;             /**< @brief hash2 片外读地址缓存FIFO溢出中断 */
    ZXIC_UINT32 hash2_key_fifo_parity_err_int;        /**< @brief hash2 片外键值缓存FIFO parity err中断 */

    ZXIC_UINT32 zcam_hash_p3_err_int;                 /**< @brief hash3 ZCAM查找出错中断 */
    ZXIC_UINT32 hash3_agree_int_fifo_ovf_int;         /**< @brief hash3 汇聚fifo 片内通道溢出中断 */
    ZXIC_UINT32 hash3_agree_ext_fifo_ovf_int;         /**< @brief hash3 汇聚fifo 片外通道parity中断 */
    ZXIC_UINT32 hash3_agree_ext_fifo_parity_err_int;  /**< @brief hash3 汇聚fifo 片外通道溢出中断*/
    ZXIC_UINT32 hash3_agree_int_fifo_parity_err_int;  /**< @brief hash3 汇聚fifo 片内通道parity中断 */
    ZXIC_UINT32 hash3_key_fifo_ovfl_int;              /**< @brief hash3 片外键值缓存FIFO 溢出中断 */
    ZXIC_UINT32 hash3_sreq_fifo_ovfl_int;             /**< @brief hash3 片外读地址缓存FIFO溢出中断 */
    ZXIC_UINT32 hash3_key_fifo_parity_err_int;        /**< @brief hash3 片外键值缓存FIFO parity err中断 */

    ZXIC_UINT32 zcam_lpm_err_int;                     /**< @brief lpm ZCAM查找出错中断 */
    ZXIC_UINT32 lpm_as_int_rsp_fifo_ovfl_int;         /**< @brief lpm as 片内查找结果缓存FIFO溢出中断 */
    ZXIC_UINT32 lpm_as_req_fifo_ovfl_int;             /**< @brief lpm as 查找ddr 地址 FIFO溢出中断 */
    ZXIC_UINT32 lpm_ext_ddr_rsp_fifo_parity_int;      /**< @brief lpm ext ddr rsp FIFO parity中断 */
    ZXIC_UINT32 lpm_ext_v6_key_parity_int;            /**< @brief lpm ext v6 key FIFO parity中断 */
    ZXIC_UINT32 lpm_ext_v4_key_parity_int;            /**< @brief lpm ext v4 key FIFO parity中断 */
    ZXIC_UINT32 lpm_ext_addr_fifo_ovfl_int;           /**< @brief lpm ext ddr3 地址溢出中断 */
    ZXIC_UINT32 lpm_ext_v4_fifo_ovfl_int;             /**< @brief lpm ext v4 key溢出中断 */
    ZXIC_UINT32 lpm_ext_v6_fifo_ovfl_int;             /**< @brief lpm ext v6 key溢出中断 */
    ZXIC_UINT32 lpm_ext_ddr_rsp_ovf_int;              /**< @brief lpm ext ddr返回缓存fifo溢出中断 */
} DPP_SE_ALG_INT_T;

/** ALG 模块对外总中断*/
typedef struct dpp_se_alg_brief_int_t
{
    ZXIC_UINT32 schd_int_unmask_flag;        /**<@brief alg模块调度状态 */
    ZXIC_UINT32 zblk_parity_int_unmask_flag; /**<@brief alg模块zblock奇偶校验中断 */
    ZXIC_UINT32 hash0_int_unmask_flag;       /**<@brief alg模块hash0中断状态 */
    ZXIC_UINT32 hash1_int_unmask_flag;       /**<@brief alg模块hash1中断状态 */
    ZXIC_UINT32 hash2_int_unmask_flag;       /**<@brief alg模块hash2中断状态 */
    ZXIC_UINT32 hash3_int_unmask_flag;       /**<@brief alg模块hash3中断状态 */
    ZXIC_UINT32 lpm_int_unmask_flag;         /**<@brief alg模块lpm中断状态 */
} DPP_SE_ALG_BRIEF_INT_T;

/** SMMU1 中断 */
typedef struct dpp_se_smmu1_int_t
{
    ZXIC_UINT32 smmu1_int0;
    ZXIC_UINT32 smmu1_int1;
    ZXIC_UINT32 smmu1_int2;
    ZXIC_UINT32 smmu1_int3;
    ZXIC_UINT32 smmu1_int4;
    ZXIC_UINT32 smmu1_int5;
    ZXIC_UINT32 smmu1_int6;
    ZXIC_UINT32 smmu1_int7;
    ZXIC_UINT32 smmu1_int8;
    ZXIC_UINT32 smmu1_int9;
    ZXIC_UINT32 smmu1_int10;
    ZXIC_UINT32 smmu1_int11;
    ZXIC_UINT32 smmu1_int12;
    ZXIC_UINT32 smmu1_int13;
    ZXIC_UINT32 smmu1_int14;
    ZXIC_UINT32 smmu1_int15;
    ZXIC_UINT32 smmu1_int16;
    ZXIC_UINT32 smmu1_int17;
} DPP_SE_SMMU1_INT_T;

typedef struct dpp_etcam_intr_t
{
    ZXIC_UINT32 etcam_int_33;
    ZXIC_UINT32 etcam_int_32;
    ZXIC_UINT32 etcam_int_31;
    ZXIC_UINT32 etcam_int_30;
    ZXIC_UINT32 etcam_int_29;
    ZXIC_UINT32 etcam_int_28;
    ZXIC_UINT32 etcam_int_27;
    ZXIC_UINT32 etcam_int_26;
    ZXIC_UINT32 etcam_int_25;
    ZXIC_UINT32 etcam_int_24;
    ZXIC_UINT32 etcam_int_23;
    ZXIC_UINT32 etcam_int_22;
    ZXIC_UINT32 etcam_int_21;
    ZXIC_UINT32 etcam_int_20;
    ZXIC_UINT32 etcam_int_19;
    ZXIC_UINT32 etcam_int_18;
    ZXIC_UINT32 etcam_int_17;
    ZXIC_UINT32 etcam_int_16;
    ZXIC_UINT32 etcam_int_15;
    ZXIC_UINT32 etcam_int_14;
    ZXIC_UINT32 etcam_int_13;
    ZXIC_UINT32 etcam_int_12;
    ZXIC_UINT32 etcam_int_11;
    ZXIC_UINT32 etcam_int_10;
    ZXIC_UINT32 etcam_int_9;
    ZXIC_UINT32 etcam_int_8;
    ZXIC_UINT32 etcam_int_7;
    ZXIC_UINT32 etcam_int_6;
    ZXIC_UINT32 etcam_int_5;
    ZXIC_UINT32 etcam_int_4;
    ZXIC_UINT32 etcam_int_3;
    ZXIC_UINT32 etcam_int_2;
    ZXIC_UINT32 etcam_int_1;
    ZXIC_UINT32 etcam_int_0;
} DPP_ETCAM_INTR_T;

typedef struct dpp_se_stat_int_t
{
    ZXIC_UINT32 stat_int0;
    ZXIC_UINT32 stat_int1;
    ZXIC_UINT32 stat_int2;
    ZXIC_UINT32 stat_int3;
    ZXIC_UINT32 stat_int4;
    ZXIC_UINT32 stat_int5;
} DPP_SE_STAT_INT_T;

#endif

#if ZXIC_REAL("macro function define")

#endif

#if ZXIC_REAL("function declaration")
/***********************************************************/
/** 写eRam
* @param   dev_id    设备号
* @param   base_addr 基地址，以128bit为单位
* @param   index     条目索引
* @param   wrt_mode  数据位宽模式, 取值参考ERAM128_OPR_MODE_E的定义
* @param   p_data    数据
*
* @return
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_se_smmu0_ind_write(DPP_DEV_T *dev,
                                  ZXIC_UINT32 base_addr,
                                  ZXIC_UINT32 index,
                                  ZXIC_UINT32 wrt_mode,
                                  ZXIC_UINT32 *p_data);
/***********************************************************/
/** 读eRam
* @param   dev_id       设备号
* @param   base_addr    基地址，以128bit为单位
* @param   index        条目索引,支持128、64、32和1bit的索引值
* @param   rd_mode      读eRam模式, 取值参照ERAM128_OPR_MODE_E定义，读清模式下不支持1bit模式
* @param   rd_clr_mode  eRam读清模式, 取值参照ERAM128_RD_CLR_MODE_E定义
* @param   p_data       返回数据缓存的指针
*
* @return
* @remark  无
* @see
* @author  wcl      @date  2015/01/30
************************************************************/
DPP_STATUS dpp_se_smmu0_ind_read(DPP_DEV_T *dev, ZXIC_UINT32 base_addr, ZXIC_UINT32 index, ZXIC_UINT32 rd_mode, ZXIC_UINT32 rd_clr_mode, ZXIC_UINT32 *p_data);

#endif

#if ZXIC_REAL("Hash & LPM data struct and function")

/** lpm&hash算法模块硬件读写函数指针类型 */
typedef DPP_STATUS (*WRITE32_FUN)(ZXIC_UINT32 dev_id, ZXIC_UINT32 addr, ZXIC_UINT32 write_data);
typedef DPP_STATUS (*READ32_FUN) (ZXIC_UINT32 dev_id, ZXIC_UINT32 addr, ZXIC_UINT32 *read_data);

/**  前缀匹配路由关联结果表写硬件函数指针类型*/
typedef DPP_STATUS (*LPM_AS_RSLT_WRT_FUNCTION)(ZXIC_UINT32 dev_id, ZXIC_UINT32 as_type, ZXIC_UINT32 tbl_id, ZXIC_UINT32 index, ZXIC_UINT8 *p_data);

#define     SE_ZGRP_NUM         (4)
#define     SE_ZBLK_NUM         (32)
#define     SE_ZCELL_NUM        (4)
#define     SE_ZCELL_TOTAL_NUM  (SE_ZBLK_NUM * SE_ZCELL_NUM)
#define     SE_ZREG_NUM         (4)
#define     SE_RAM_DEPTH        (512)
#define     MAX_FUN_NUM         (8)
#define     SE_ALG_BANK_NUM     (29)

#define LPM_THREAD_HW_WRITE_EN             (0)      /* ??????????????? */
#if LPM_THREAD_HW_WRITE_EN
#define ROUTE_DEV_CHANNEL_MAX              (4)      /* ?????? NPE_DEV_CHANNEL_MAX ??????? */
#define MAX_ITEM_INFO_BAK_NUM              (0x100)  
#endif
/*
*    zcell/zreg 基本存储单元的配置信息,512bit为单位，
*    zcell包含512个基本存储单元，zreg包含4个基本存储单元，
*    每个单元可以存储1个512bit条目、2个256bit条目或4个128bit条目，
*    本结构体不感知内部存储条目的位宽，hash中键值中的key_type可以
*    可以获取表项的位宽信息
*/
typedef struct se_item_cfg
{
    D_HEAD  item_list;      /** 基本存储单元双链表头节点，可以存放放在本单元内的具体表项条目*/
    ZXIC_UINT32  item_index;     /** 基本存储单元所处的索引值 */
    ZXIC_UINT32  hw_addr;        /** 基本存储单元物理的地址 */
    ZXIC_UINT32  bulk_id;
    ZXIC_UINT32  item_type;      /** 基本存储单元的存储类型，包括片外或片内，参见SE_ITEM_TYPE */
    ZXIC_UINT8    wrt_mask;       /** 基本存储单元写入的掩码，4bit，每bit表示待写入的128bit位置 */
    ZXIC_UINT8    valid;          /** 基本存储单元是否被占用，0-未被占用，1-被占用 */
    ZXIC_UINT8    pad[2];
} SE_ITEM_CFG;

/**  zcell属性标志*/
#define DPP_ZCELL_FLAG_IS_MONO       (1) /**<  @brief ZCELL是否被独占: 0-未被独占，1-已经被独占*/

/**  zcell属性标志*/
#define DPP_ZREG_FLAG_IS_MONO        (1) /**<  @brief ZREG是否被独占:  0-未被独占，1-已经被独占*/

/* zcell 的配置信息*/
typedef struct se_zcell_cfg
{
    ZXIC_UINT8            flag;           /* zcell属性标志，各比特的含义见上面DPP_ZCELL_FLAG_MODE等的定义 */
    ZXIC_UINT32          bulk_id;        /* zcell所属空间ID  */
    ZXIC_UINT32          zcell_idx;      /* zcell 的索引值, 高6bit为zblock的索引     */
    ZXIC_UINT16          mask_len;       /* 掩码长度 */
    ZXIC_UINT8            is_used;        /* 本zcell是否被占用*/
    ZXIC_UINT8            is_share;       /* 是否共享block中的zcell*/
    ZXIC_UINT32          item_used;
    SE_ITEM_CFG     item_info[SE_RAM_DEPTH];    /* 本zcell的基本存储单元配置*/

    D_NODE          zcell_dn;       /* 双链表节点，初始化其data指针指向SE_ZCELL_CFG实例自身，然后插入双链表中*/
    ZXIC_AVL_NODE    zcell_avl;      /* 本hash程序未使用， 平衡二叉树节点*/
} SE_ZCELL_CFG;
/* zreg 的配置信息*/
typedef struct se_zreg_cfg
{
    ZXIC_UINT8            flag;           /* z属性标志，各比特的含义见上面DPP_ZCELL_FLAG_MODE等的定义 */
    ZXIC_UINT8            pad[3];
    ZXIC_UINT32          bulk_id;        /* zcell所属空间ID  */
    SE_ITEM_CFG     item_info;     /* zreg 存储的表条目配置*/
} SE_ZREG_CFG;
/* zblock 的配置信息*/
typedef struct se_zblk_cfg
{
    ZXIC_UINT32          zblk_idx;   /* 统一编址，范围0~31 */
    ZXIC_UINT16          is_used;    /* 本zblock 是否被占用 */
    ZXIC_UINT16          zcell_bm;   /* 本hash程序未使用 */
    ZXIC_UINT16          hash_arg;   /* hash crc算法的因子 */
    ZXIC_UINT16          pad;

    SE_ZCELL_CFG    zcell_info[SE_ZCELL_NUM];   /* 本block的4个zcell配置 */
    SE_ZREG_CFG     zreg_info[SE_ZREG_NUM];     /* 本block的4个zreg配置*/

    D_NODE          zblk_dn;                    /* 指向本实例的双链表节点 */

} SE_ZBLK_CFG;
/* alg表类型配置，包括4路hash引擎和v4 和 v6 lpm表项*/
typedef struct func_id_info
{
    ZXIC_VOID *fun_ptr;
    ZXIC_UINT8  fun_type;             /* 业务类型 参见 SE_FUN_TYPE */
    ZXIC_UINT8  fun_id;
    ZXIC_UINT8  is_used;
    ZXIC_UINT8  pad;
} FUNC_ID_INFO;

typedef struct ddr_mem
{
    ZXIC_UINT32     total_num;
    ZXIC_UINT32     base_addr;
    ZXIC_UINT32     base_addr_offset;
    ZXIC_UINT32     ecc_en;
    ZXIC_UINT32     bank_num;
    ZXIC_UINT32     bank_info[SE_ALG_BANK_NUM];
    ZXIC_UINT32     share_type;  /* DDR bank共享模式 */
    ZXIC_UINT32     item_used;
    ZXIC_LISTSTACK_MANGER *p_ddr_mng;
} DDR_MEM;

typedef struct share_ram
{
    ZXIC_UINT32    zblk_array[SE_ZBLK_NUM];
    D_HEAD    zblk_list;
    D_HEAD    zcell_free_list;
    ZXIC_UINT32    def_route_num;

    ZXIC_RB_CFG       def_rb;
    struct def_route_info  *p_dr_info;

    DDR_MEM   ddr4_info;
    DDR_MEM   ddr6_info;
} SHARE_RAM;

/**  算法模块管理数据结构，用户仅需创建实例变量，然后将指针传给初始化函数dpp_se_init()和dpp_se_client_init()即可，不需要单独赋值成员变量*/
typedef struct dpp_se_cfg
{
    SE_ZBLK_CFG        zblk_info[SE_ZBLK_NUM];

    FUNC_ID_INFO       fun_info[MAX_FUN_NUM];

    SHARE_RAM          route_shareram;
    ZXIC_UINT32             reg_base;

    WRITE32_FUN        p_write32_fun;
    READ32_FUN         p_read32_fun;

    ZXIC_UINT32             lpm_flags;

    ZXIC_VOID               *p_client;

    DPP_DEV_T               dev;
    ZXIC_UINT32             dev_id;

    LPM_AS_RSLT_WRT_FUNCTION p_as_rslt_wrt_fun;           /* dpp_se_lpm_as_rslt_write */

#if LPM_THREAD_HW_WRITE_EN
    // ZXIC_UINT32  mutex_location;                     /* dpp_route_cfg ?? cache mutex???????????route mode */
    ZXIC_MUTEX_T cache_index_mutex[MAX_ITEM_INFO_BAK_NUM];         /* cache每个节点锁 */
    ZXIC_UINT32  thread_hw_write_is_create;          /* ??????????????? ???????? add by lining for thread_hw_write */
    ZXIC_LISTSTACK_MANGER *p_thread_liststack_mng;
#endif

} DPP_SE_CFG;

/**  hash物理存储位宽类型*/
typedef enum dpp_hash_ddr_width_mode
{
    DDR_WIDTH_INVALID = 0,
    DDR_WIDTH_256b,          /**<  @brief 256bit位宽模式*/
    DDR_WIDTH_512b,          /**<  @brief 512bit位宽模式*/
} DPP_HASH_DDR_WIDTH_MODE;

/**  hash条目类型*/
typedef enum dpp_hash_key_type
{
    HASH_KEY_INVALID = 0, /**<  @brief 无效类型*/
    HASH_KEY_128b,        /**<  @brief 128bit位宽类型*/
    HASH_KEY_256b,        /**<  @brief 256bit位宽类型*/
    HASH_KEY_512b,        /**<  @brief 512bit位宽类型*/
} DPP_HASH_KEY_TYPE;

/**  hash ddr resource cfg info*/
typedef struct dpp_hash_ddr_resc_cfg_t
{
    ZXIC_UINT32 ddr_width_mode; /**<  @brief 分配给hash的DDR空间物理存储位宽模式，取值参考DPP_HASH_DDR_WIDTH_MODE的定义*/
    ZXIC_UINT32 ddr_crc_sel;                     /**<  @brief 选择一个DDR CRC多项式，取值范围0~3,0~3分别对应一个CRC多项式*/
    ZXIC_UINT32 ddr_item_num;                    /**<  @brief 分配给hash的DDR空间单元数目，以256bit为一个单元*/
    ZXIC_UINT32 ddr_baddr;                       /**<  @brief 分配给hash的DDR空间的硬件基地址,以2k*256bit为单位*/
    ZXIC_UINT32 ddr_ecc_en;                      /**<  @brief DDR ECC使能: 0-不使能，1-使能*/
} DPP_HASH_DDR_RESC_CFG_T;

/** hash search mode */
typedef enum dpp_hash_srh_mode
{
    HASH_SRH_MODE_SOFT = 1,  /**<  @brief 查软件 */
    HASH_SRH_MODE_HDW  = 2,  /**<  @brief 查硬件 */
} DPP_HASH_SRH_MODE;

/** hash tbl_flag */
#define HASH_TBL_FLAG_AGE             (1<<0)  /**<  @brief 老化保活置位使能: 0-不使能；1-使能*/
#define HASH_TBL_FLAG_LEARN           (1<<1)  /**<  @brief 硬件学习使能: 0-不使能；1-使能*/
#define HASH_TBL_FLAG_MC_WRT          (1<<2)  /**<  @brief 微码写表使能: 0-不使能，1-使能 */

/**  hash条目*/
typedef struct dpp_hash_entry
{
    ZXIC_UINT8 *p_key; /**<  @brief 键值，格式详见各操作函数的说明*/
    ZXIC_UINT8 *p_rst; /**<  @brief 结果*/
} DPP_HASH_ENTRY;


/**  前缀路由业务初始化标志*/
#define LPM_FLAG_RT_HANDLE_START       (0)  /**<  @brief 是否使能级联结果表查找: 0-不使能，1-使能*/
#define LPM_FLAG_RT_HANDLE_WIDTH       (1)
#define LPM4_FLAG_DDR_EN_START         (1)  /**<  @brief 是否使能ipv4片外查找模式: 0-不使能，1-使能 */
#define LPM4_FLAG_DDR_EN_WIDTH         (1)
#define LPM6_FLAG_DDR_EN_START         (2)  /**<  @brief 是否使能ipv6片外查找模式: 0-不使能，1-使能 */
#define LPM6_FLAG_DDR_EN_WIDTH         (1)
#define LPM4_FLAG_DDR_SEL_START        (3)  /**<  @brief 是否为ipv4非线速模式: 0-线速，1-非线速 */
#define LPM4_FLAG_DDR_SEL_WIDTH        (1)
#define LPM6_FLAG_DDR_SEL_START        (4)  /**<  @brief 是否为ipv6非线速模式: 0-线速，1-非线速 */
#define LPM6_FLAG_DDR_SEL_WIDTH        (1)
#define LPM_FLAG_AS_MODE_START         (5)  /**<  @brief 级联结果表模式: 1-级联DDR，0-级联eRam  */
#define LPM_FLAG_AS_MODE_WIDTH         (1)

/**  前缀匹配路由级联DDR结果表返回位宽模式*/
typedef enum dpp_route_as_rsp_len_e
{
    DPP_ROUTE_AS_128b  = 0,  /**<  @brief 返回128bit模式*/
    DPP_ROUTE_AS_256b  = 1,  /**<  @brief 返回256bit模式*/
    DPP_ROUTE_AS_384b  = 2,  /**<  @brief 返回384bit模式*/
    DPP_ROUTE_AS_512b  = 3   /**<  @brief 返回512bit模式*/
} DPP_ROUTE_AS_RSP_LEN_E;

/**  前缀匹配路由业务ID*/
typedef enum dpp_route_id_e
{
    DPP_ROUTE_V4_ID = 4, /**<  @brief route ipv4 ID*/
    DPP_ROUTE_V6_ID = 5, /**<  @brief route ipv6 ID*/
} DPP_ROUTE_ID_E;

/**  前缀匹配路由业务模式*/
typedef enum dpp_route_mode_e
{
    DPP_ROUTE_MODE_IPV4 = 1UL, /**<  @brief ipv4路由模式*/
    DPP_ROUTE_MODE_IPV6        /**<  @brief ipv6路由模式*/
} DPP_ROUTE_MODE_E;

/**  前缀匹配路由DDR空间占用模式*/
typedef enum dpp_route_ddr_use_mode_e
{
    DPP_ROUTE_DDR_USE_MINOR   = 1, /**<  @brief DDR使用量较少*/
    DPP_ROUTE_DDR_USE_MIDDLE  = 2, /**<  @brief DDR使用量中等*/
    DPP_ROUTE_DDR_USE_MAJOR   = 3, /**<  @brief DDR使用量较多*/
} DPP_ROUTE_DDR_USE_MODE_E;

/**  前缀匹配路由调试查找模式*/
typedef enum dpp_route_srh_mode_e
{
    DPP_ROUTE_SRH_MODE_LP    = 1, /**<  @brief 前缀匹配查找模式*/
    DPP_ROUTE_SRH_MODE_EQUAL = 2, /**<  @brief 精确匹配查找模式*/
} DPP_ROUTE_SRH_MODE_E;

/**  前缀匹配路由硬件资源 */
typedef struct dpp_route_resource_t
{
    ZXIC_UINT32 zblk_num;          /**<  @brief LPM ipv4和ipv6共享的zblock数目*/
    ZXIC_UINT32 *zblk_idx;         /**<  @brief LPM ipv4和ipv6共享的zblock编号数组*/
    ZXIC_UINT32 ddr4_item_num;     /**<  @brief 分配给ipv4前缀查找的ddr存储条目数，以256bit为单位*/
    ZXIC_UINT32 ddr4_baddr;        /**<  @brief 分配给ipv4前缀查找的ddr存储空间的基地址，以4K*128bit为单位*/
    ZXIC_UINT32 ddr4_base_offset;  /**<  @brief ipv4前缀查找相对于片外ddr存储空间基地址的偏移量，以256bit为单位*/
    ZXIC_UINT32 ddr4_ecc_en;       /**<  @brief 固定配为1，分配给ipv4前缀查找的ddr存储空间的ECC校验使能标志*/
    ZXIC_UINT32 ddr4_bank_num;     /**<  @brief ipv4前缀查找的ddr存储空间bank复制份数 */
    ZXIC_UINT32 *ddr4_bank_info;   /**<  @brief ipv4前缀查找的ddr存储空间bank号，数组传入，支持离散分配*/
    ZXIC_UINT32 ddr4_share_type;   /**<  @brief ipv4前缀查找的ddr存储空间bank共享模式，参见SMMU1_DDR_SHARE_MODE_E*/
    ZXIC_UINT32 ddr6_item_num;     /**<  @brief 分配给ipv6前缀查找的ddr存储条目数，以256bit为单位*/
    ZXIC_UINT32 ddr6_baddr;        /**<  @brief 分配给ipv6前缀查找的ddr存储空间的基地址，以4K*128bit为单位*/
    ZXIC_UINT32 ddr6_base_offset;  /**<  @brief ipv6前缀查找相对于片外ddr存储空间基地址的偏移量，以256bit为单位*/
    ZXIC_UINT32 ddr6_ecc_en;       /**<  @brief 固定配为1，分配给ipv4前缀查找的ddr存储空间的ECC校验使能标志*/
    ZXIC_UINT32 ddr6_bank_num;     /**<  @brief ipv6前缀查找的ddr存储空间bank复制份数 */
    ZXIC_UINT32 *ddr6_bank_info;   /**<  @brief ipv6前缀查找的ddr存储空间bank号，数组传入，支持离散分配*/
    ZXIC_UINT32 ddr6_share_type;   /**<  @brief ipv6前缀查找的ddr存储空间bank共享模式，参见SMMU1_DDR_SHARE_MODE_E*/
} DPP_ROUTE_RESOURCE_T;

/**  前缀匹配路由级联片内eRam结果表属性 */
typedef struct dpp_route_as_eram_t
{
    ZXIC_UINT32 baddr;     /**<  @brief LPM级联eRam结果基地址*/
    ZXIC_UINT32 rsp_mode;  /**<  @brief LPM级联eRam结果位宽模式，取值参照ERAM128_TBL_MODE_E的定义*/
} DPP_ROUTE_AS_ERAM_T;

/**  前缀匹配路由级联片外DDR结果表属性 */
typedef struct dpp_route_as_ddr_t
{
    ZXIC_UINT32 baddr;        /**<  @brief 分配给级联ddr结果表空间的基地址，以4K*128bit为单位*/
    ZXIC_UINT32 rsp_len;      /**<  @brief LPM级联DDR结果位宽模式，取值参照DPP_ROUTE_AS_RSP_LEN_E的定义*/
    ZXIC_UINT32 ecc_en;       /**<  @brief 级联结果表DDR空间ECC校验使能标志: 0-不使能，1-使能*/
} DPP_ROUTE_AS_DDR_T;

typedef union dpp_route_as_rslttbl_u
{
    DPP_ROUTE_AS_ERAM_T as_eram_cfg;  /**<  @brief LPM级联eRam结果表空间属性*/
    DPP_ROUTE_AS_DDR_T  as_ddr_cfg;   /**<  @brief LPM级联DDR结果表空间属性*/
} DPP_ROUTE_AS_RSLTTBL_U;

/**  前缀匹配IPv4键值*/
typedef struct dpp_route_ipv4_key_t
{
    ZXIC_UINT32     vpnid;          /**<  @brief vpnid，16bit*/
    ZXIC_UINT32     mask_len;       /**<  @brief IP地址的掩码长度，最小为0(表示默认路由)，最大为32*/
    ZXIC_UINT32     ipv4_addr;      /**<  @brief IP地址，32bit*/
} DPP_ROUTE_IPV4_KEY_T;

/**  前缀匹配IPv4单个路由条目*/
typedef struct dpp_route_entry_ipv4_t
{
    DPP_ROUTE_IPV4_KEY_T route_key; /**<  @brief 键值*/
    ZXIC_UINT32         route_handle;    /**<  @brief 键值匹配后，继续查转发结果表的索引*/
    ZXIC_UINT8           *p_as_rslt;      /**<  @brief 转发结果，仅在使能lpm关联结果查找的情况下有效 */
} DPP_ROUTE_ENTRY_IPV4_T;

/**  前缀匹配IPv6键值*/
typedef struct dpp_route_ipv6_key_t
{
    ZXIC_UINT32    vpnid;      /**<  @brief vpnid，16bit*/
    ZXIC_UINT32    mask_len;   /**<  @brief IP地址的掩码长度，最小为0(表示默认路由)，最大为128*/
    ZXIC_UINT32    ipaddr[4];  /**<  @brief IP地址，128bit*/
} DPP_ROUTE_IPV6_KEY_T;

/**  前缀匹配IPv6单个路由条目*/
typedef struct dpp_route_entry_ipv6_t
{
    DPP_ROUTE_IPV6_KEY_T route_key;  /**<  @brief 键值*/
    ZXIC_UINT32         route_handle;     /**<  @brief 键值匹配后，继续查转发结果表的索引*/
    ZXIC_UINT8           *p_as_rslt;       /**<  @brief 转发结果，仅在使能lpm关联结果查找的情况下有效 */
} DPP_ROUTE_ENTRY_IPV6_T;

/* 在HASH进行初始化的时候，需要存储的参数 */
typedef struct dpp_hash_soft_reset_stor_dat
{
    /* 参照dpp_hash_init()的参数 */
    ZXIC_UINT32 ddr_dis_flag[4];     /**<  @brief 4个HASH引擎是否DISABLE DDR标志 */
    ZXIC_UINT32 zblk_num[4];         /**<  @brief 4个HASH引擎使用的zblk数量 */
    ZXIC_UINT32 *zblk_idx_start[4];  /**<  @brief 4个HASH引擎使用的起始zblk index */

    /* 参照dpp_hash_bulk_init()的参数 */
    ZXIC_UINT32 ddr_item_num[4][8];  /**<  @brief 每个bulk空间存放的ddr item数 */
    ZXIC_UINT32 ddr_base_addr[4];    /**<  @brief 每个hash引擎基地址 */
    ZXIC_UINT32 ddr_bank_cp[4];      /**<  @brief 每个hash引擎bank copy数量 */
    ZXIC_UINT32 ddr_ecc_en[4];       /**<  @brief ddr是否开启ecc使能 */

    /*     ZXIC_UINT32 ddr_width_mode[4]; */       /**<  @brief ddr存储位宽 */
    /*     ZXIC_UINT32 ddr_crc_sel[4];  */      /**<  @brief ddr多项式选择 */
    /*     ZXIC_UINT32 ddr_share_type[4]; */       /**<  @brief ddr 共享类型 */

    ZXIC_UINT32 hash_id_valid;       /** HASH引擎是否已经初始化标志 */
} DPP_HASH_SOFT_RESET_STOR_DAT;



/***********************************************************/
/** 初始化算法管理数据结构，不包含用户自定义的数据指针
* @param   p_se_cfg  算法管理数据结构指针
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wcl      @date  2015/05/20
************************************************************/
DPP_STATUS  dpp_se_init(DPP_DEV_T *dev, DPP_SE_CFG *p_se_cfg);

/***********************************************************/
/** 初始化算法管理数据结构用户自定义的数据指针，当前仅用于传入设备号的值
* @param   p_se_cfg  算法管理数据结构指针
* @param   p_client  用户自定义的数据指针
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wcl      @date  2015/05/20
************************************************************/
DPP_STATUS dpp_se_client_init(DPP_SE_CFG *p_se_cfg, ZXIC_VOID *p_client);

/***********************************************************/
/** 单个hash引擎初始化
* @param   p_se_cfg      算法模块公共管理数据结构指针
* @param   fun_id         hash引擎号
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
                         ZXIC_UINT32     fun_id,
                         ZXIC_UINT32     zblk_num,
                         ZXIC_UINT32     *zblk_idx,
                         ZXIC_UINT32     ddr_dis);

/***********************************************************/
/** 初始化单个hash引擎内的某个业务表，此接口支持为该业务表分配独占的zcell。
*     必须先初始化hash引擎，再初始化业务表。
* @param   p_se_cfg        算法模块公共管理数据结构指针
* @param   fun_id         hash引擎号
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
                              ZXIC_UINT32     fun_id,
                              ZXIC_UINT32     bulk_id,
                              DPP_HASH_DDR_RESC_CFG_T *p_ddr_resc_cfg,
                              ZXIC_UINT32     zcell_num,
                              ZXIC_UINT32     zreg_num);

/***********************************************************/
/** 初始化单个hash引擎内的某个业务表，此接口支持为该业务表分配独占的zcell。
*   必须先初始化hash引擎，如果是片内+片外模式还必须先初始化dpp_hash_ddr_bulk_init，
*   再初始化业务表。
* @param   p_se_cfg    算法模块公共管理数据结构指针
* @param   fun_id      hash引擎号
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
                                     ZXIC_UINT32  fun_id,
                                     ZXIC_UINT32  tbl_id,
                                     ZXIC_UINT32  tbl_flag,
                                     ZXIC_UINT32  key_type,
                                     ZXIC_UINT32  actu_key_size);

#endif

#if ZXIC_REAL("eTcam data struct and function")
typedef DPP_STATUS (*ACL_AS_RSLT_WRT_FUNCTION)(ZXIC_UINT32 dev_id, ZXIC_UINT32 base_addr, ZXIC_UINT32 index, ZXIC_UINT32 as_mode, ZXIC_UINT8 *p_data);
#define DPP_ACL_TBL_ID_NUM      (8U)
#define DPP_ACL_ETCAM_ID_NUM    (1U)
#define DPP_ACL_BLOCK_NUM       (8U)

/**  eTcam条目位宽模式*/
typedef enum dpp_etcam_entry_mode_e
{
    DPP_ETCAM_KEY_640b = 0,
    DPP_ETCAM_KEY_320b = 1,
    DPP_ETCAM_KEY_160b = 2,
    DPP_ETCAM_KEY_80b  = 3,
    DPP_ETCAM_KEY_INVALID,
} DPP_ETCAM_ENTRY_MODE_E;

/**  eTcam操作类型*/
typedef enum dpp_etcam_opr_type_e
{
    DPP_ETCAM_OPR_DM = 0,  /**<  @brief data & mask类型*/
    DPP_ETCAM_OPR_XY = 1,  /**<  @brief X & Y类型*/
} DPP_ETCAM_OPR_TYPE_E;

/**  eTcam条目格式*/
typedef struct dpp_etcam_entry_t
{
    ZXIC_UINT32 mode;     /**<  @brief 条目位宽模式: 2'b00-640bit,2'b01-320bit,2'b10-160bit,2'b11-80bit*/
    ZXIC_UINT8   *p_data;  /**<  @brief 键值*/
    ZXIC_UINT8   *p_mask;  /**<  @brief 掩码*/
} DPP_ETCAM_ENTRY_T;

/**  ACL键值位宽模式*/
typedef enum dpp_acl_key_mode_e
{
    DPP_ACL_KEY_640b = 0, /**<  @brief 640bit键值位宽*/
    DPP_ACL_KEY_320b,     /**<  @brief 320bit键值位宽*/
    DPP_ACL_KEY_160b,     /**<  @brief 160bit键值位宽*/
    DPP_ACL_KEY_80b,      /**<  @brief 80bit键值位宽*/
    DPP_ACL_KEY_INVALID,
} DPP_ACL_KEY_MODE_E;

/**  ACL关联查找结果表位宽模式*/
typedef enum dpp_acl_as_mode_e
{
    DPP_ACL_AS_MODE_16b  = 0,
    DPP_ACL_AS_MODE_32b  = 1,
    DPP_ACL_AS_MODE_64b  = 2,  /**<  @brief 64bit结果位宽*/
    DPP_ACL_AS_MODE_128b = 3,  /**<  @brief 128bit结果位宽*/
    DPP_ACL_AS_MODE_INVALID,
} DPP_ACL_AS_MODE_E;

/**  ACL调试查找模式*/
typedef enum dpp_acl_srh_mode_e
{
    DPP_ACL_SRH_SOFT     = 0, /**<  @brief 查软件*/
    DPP_ACL_SRH_HARDWARE = 1, /**<  @brief 查硬件*/
} DPP_ACL_SRH_MODE_E;

/**  ACL条目*/
typedef struct dpp_acl_entry_t
{
    ZXIC_UINT32  handle;     /**<  @brief 条目索引*/
    ZXIC_UINT8    *key_data;  /**<  @brief 键值data部分*/
    ZXIC_UINT8    *key_mask;  /**<  @brief 键值mask部分: 0为关心，1为不关心*/
    ZXIC_UINT8    *p_as_rslt; /**<  @brief 关联结果，仅使能关联查找情况有效*/
} DPP_ACL_ENTRY_T;

typedef struct dpp_acl_block_info_t
{
    ZXIC_UINT32 is_used;
    ZXIC_UINT32 tbl_id;
    ZXIC_UINT32 idx_base;
} DPP_ACL_BLOCK_INFO_T;

typedef struct dpp_acl_etcamid_cfg_t
{
    ZXIC_UINT32 is_valid;
    ZXIC_UINT32 as_enable;     /* eTcam自动关联eRam结果表使能: 0-不使能，1-使能 */
    ZXIC_UINT32 as_idx_offset; /* 关联结果表基地址偏移，以128bit为单位 */
    ZXIC_UINT32 as_eRam_base;  /* eTcam自动关联的eRam block的基地址，以128bit为单位 */
    D_HEAD tbl_list;
} DPP_ACL_ETCAMID_CFG_T;

typedef struct dpp_acl_key_info_t
{
    ZXIC_UINT32 handle;
    ZXIC_UINT32 pri;
    ZXIC_UINT8   key[0];  /* data+mask */
} DPP_ACL_KEY_INFO_T;

/**  ACL初始化使能标志*/
#define DPP_ACL_FLAG_ETCAM0_EN        (1<<0) /**<  @brief 开启eTcam端口0: 0-不开启，1-开启.*/
#define DPP_ACL_FLAG_ETCAM0_AS        (1<<2) /**<  @brief 开启eTcam端口0的关联结果查找: 0-不开启，1-开启.*/
//#define DPP_ACL_FLAG_ETCAM1_AS        (1<<3) /**<  @brief 开启eTcam端口1的关联结果查找: 0-不开启，1-开启.*/

typedef DPP_STATUS (*ACL_TBL_AS_DDR_WR_FUN)(ZXIC_UINT32 dev_id, ZXIC_UINT32 tbl_type, ZXIC_UINT32 tbl_id, ZXIC_UINT32 dir_tbl_share_type, ZXIC_UINT32 dir_tbl_base_addr, ZXIC_UINT32 ecc_en, ZXIC_UINT32 index, ZXIC_UINT32 as_mode, ZXIC_UINT8 *p_data);
typedef DPP_STATUS (*ACL_TBL_AS_DDR_RD_FUN)(ZXIC_UINT32 dev_id, ZXIC_UINT32 base_addr, ZXIC_UINT32 index, ZXIC_UINT32 as_mode, ZXIC_UINT8 *p_data);

/**  */
typedef struct dpp_acl_tbl_cfg_t
{
    ZXIC_UINT32 tbl_type;
    ZXIC_UINT32 table_id;
    ZXIC_UINT8   is_as_ddr;
    ZXIC_UINT8   ddr_bankcp_info;
    ZXIC_UINT32 dir_tbl_share_type;
    ZXIC_UINT8   ddr_ecc_en;
    ZXIC_UINT32 pri_mode;
    ZXIC_UINT32 key_mode;
    ZXIC_UINT32 entry_num;
    ZXIC_UINT32 block_num;
    ZXIC_UINT32 *block_array;
    ZXIC_UINT32 is_used;
    ZXIC_UINT32 as_mode;
    ZXIC_UINT32 as_idx_base;
    ZXIC_UINT32 as_enable;           /* eTcam自动关联eRam结果表使能: 0-不使能，1-使能 */
    ZXIC_UINT32 as_eRam_base;        /* eTcam自动关联的eRam block的基地址，以128bit为单位 */
    ZXIC_UINT32 ddr_baddr;
    ZXIC_UINT32 idx_offset; /* 相对于ddr_baddr的基地址的索引偏移，以as_mode对应的数据位宽为单位 */
    ACL_TBL_AS_DDR_WR_FUN p_as_ddr_wr_fun;
    ACL_TBL_AS_DDR_RD_FUN p_as_ddr_rd_fun;
    D_NODE         entry_dn;
    INDEX_FILL_CFG index_mng;
    ZXIC_RB_CFG     acl_rb;
    DPP_ACL_KEY_INFO_T  **acl_key_buff;
    ZXIC_UINT8  *as_rslt_buff;
} DPP_ACL_TBL_CFG_T;

/**  ACL公共管理数据结构*/
typedef struct dpp_acl_cfg_t
{
    ZXIC_VOID    *p_client;
    DPP_DEV_T    *dev;
    ZXIC_UINT32  dev_id;
    ZXIC_UINT32  flags;
    ACL_AS_RSLT_WRT_FUNCTION  p_as_rslt_write_fun;
    ACL_AS_RSLT_WRT_FUNCTION  p_as_rslt_read_fun;
    DPP_ACL_BLOCK_INFO_T      acl_blocks[DPP_ACL_BLOCK_NUM];
    DPP_ACL_ETCAMID_CFG_T     acl_etcamids;
    DPP_ACL_TBL_CFG_T         acl_tbls[DPP_ACL_TBL_ID_NUM];
} DPP_ACL_CFG_T;

/**<  @brief ACL公共管理数据结构*/
typedef struct dpp_acl_cfg_ex_t
{
    ZXIC_VOID    *p_client;
    DPP_DEV_T    *dev;
    ZXIC_UINT32  dev_id;
    ZXIC_UINT32  flags;
    ACL_AS_RSLT_WRT_FUNCTION  p_as_rslt_write_fun;
    ACL_AS_RSLT_WRT_FUNCTION  p_as_rslt_read_fun;
    DPP_ACL_BLOCK_INFO_T      acl_blocks[DPP_ACL_BLOCK_NUM];
    DPP_ACL_ETCAMID_CFG_T     acl_etcamids;
    DPP_ACL_TBL_CFG_T         acl_tbls[DPP_ACL_TBL_ID_NUM];
}DPP_ACL_CFG_EX_T;

/**  acl优先级模式*/
typedef enum dpp_acl_pri_mode_e
{
    DPP_ACL_PRI_EXPLICIT = 1, /**<  @brief 显示优先级*/
    DPP_ACL_PRI_IMPLICIT,     /**<  @brief 隐式优先级，以条目下发顺序作为优先级*/
    DPP_ACL_PRI_SPECIFY,      /**<  @brief 用户指定每个条目的在tcam中的存放索引*/
    DPP_ACL_PRI_INVALID,
} DPP_ACL_PRI_MODE_E;

/**  */
typedef struct dpp_acl_entry_ex_t
{
    ZXIC_UINT32  idx_val;   /**<  @brief 一次插入单个acl条目时有效，返回单个索引*/
    D_HEAD  idx_list;  /**<  @brief 一次插入多个acl条目时有效，返回多个索引*/
    ZXIC_UINT32  pri;       /* PRI_EXPLICIT: pri is priority, PRI_IMPLICIT: pri is invalid, PRI_SPECIFY: pri is handle */
    ZXIC_UINT8    *key_data;
    ZXIC_UINT8    *key_mask;
    ZXIC_UINT8    *p_as_rslt;
} DPP_ACL_ENTRY_EX_T;

/***********************************************************/
/** eTcam模块初始化
* @param   dev_id 设备号
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wcl      @date  2015/07/24
************************************************************/
DPP_STATUS dpp_etcam_init(ZXIC_UINT32 dev_id);

/***********************************************************/
/**
* @param   p_acl_cfg   ACL公共管理数据结构指针
* @param   p_client   用户自定义的数据指针
* @param   flags
* @param   p_as_wrt_fun   关联结果写硬件表回调函数指针
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yxh      @date  2017/12/15
************************************************************/
DPP_STATUS dpp_acl_cfg_init_ex(DPP_DEV_T *dev,
                               DPP_ACL_CFG_EX_T *p_acl_cfg,
                               ZXIC_VOID *p_client,
                               ZXIC_UINT32 flags,
                               ACL_AS_RSLT_WRT_FUNCTION p_as_wrt_fun);

/***********************************************************/
/** acl业务表初始化，注意分配给一个table的多个block_idx
    必须按从小到大的顺序给定。支持多个优先级模式，暂不对外开放。
* @param   p_acl_cfg   ACL公共管理数据结构指针
* @param   table_id   业务表号
* @param   as_enable   是否使能关联结果查找，0-不使能，1-使能
* @param   entry_num   最大条目数
* @param   pri_mode   ACL优先级模式
* @param   key_mode   ACL键值位宽模式, 取值参照DPP_ACL_KEY_MODE_E的定义
* @param   as_mode   ACL关联查找结果表位宽模式
* @param   as_baddr   基地址
* @param   block_num   分配给当前业务表号的block数目
* @param   p_block_idx   分配给当前业务表号的block编号数组
*
* @return
* @remark  无
* @see
* @author  wcl      @date  2014/12/23
************************************************************/
DPP_STATUS dpp_acl_tbl_init_ex(DPP_ACL_CFG_EX_T *p_acl_cfg,
                               ZXIC_UINT32 table_id,
                               ZXIC_UINT32 as_enable,
                               ZXIC_UINT32 entry_num,
                               DPP_ACL_PRI_MODE_E pri_mode,
                               ZXIC_UINT32 key_mode,
                               DPP_ACL_AS_MODE_E  as_mode,
                               ZXIC_UINT32 as_baddr,
                               ZXIC_UINT32 block_num,
                               ZXIC_UINT32 *p_block_idx);
DPP_STATUS dpp_acl_res_destroy(ZXIC_UINT32 dev_id);

#endif

#if ZXIC_REAL("SDT data struct and function")
/** SDT属性中的表类型 */
typedef enum dpp_sdt_table_type_e
{
    DPP_SDT_TBLT_INVALID = 0, /**<  @brief 无效类型*/
    DPP_SDT_TBLT_eRAM    = 1, /**<  @brief eRAM直接表类型*/
    DPP_SDT_TBLT_DDR3    = 2, /**<  @brief DDR3直接表类型*/
    DPP_SDT_TBLT_HASH    = 3, /**<  @brief Hash表类型*/
    DPP_SDT_TBLT_LPM     = 4, /**<  @brief LPM表类型*/
    DPP_SDT_TBLT_eTCAM   = 5, /**<  @brief 片内Tcam表类型*/
    DPP_SDT_TBLT_PORTTBL = 6, /**<  @brief 物理端口属性表*/
    DPP_SDT_TBLT_MAX     = 7,
} DPP_SDT_TABLE_TYPE_E;

/**  返回位宽模式*/
typedef enum dpp_sdt_rsp_mode_e
{
    DPP_SDT_RSP_32b  = 0, /**  返回32bit位宽表结果*/
    DPP_SDT_RSP_64b  = 1, /**  返回64bit位宽表结果*/
    DPP_SDT_RSP_128b = 2, /**  返回128bit位宽表结果*/
    DPP_SDT_RSP_256b = 3, /**  返回256bit位宽表结果*/
} DPP_SDT_RSP_MODE_E;

/**  eRam直接表SDT属性*/
typedef struct dpp_sdt_tbl_eram_t
{
    ZXIC_UINT32 table_type;          /** <@brief 查找表项类型 */
    ZXIC_UINT32 eram_mode;           /** <@brief eRam返回位宽 */
    ZXIC_UINT32 eram_base_addr;      /** <@brief eRam表项基地址，128bit为单位 */
    ZXIC_UINT32 eram_table_depth;    /** <@brief 表项深度，作为越界检查使用   */
    ZXIC_UINT32 eram_clutch_en;      /** <@brief 抓包使能 */
} DPP_SDTTBL_ERAM_T;

/**  DDR3直接表SDT属性*/
typedef struct dpp_sdt_tbl_ddr3_t
{
    ZXIC_UINT32 table_type;           /** <@brief 查找表项类型   */
    ZXIC_UINT32 ddr3_base_addr;       /** <@brief ddr 基地址     */
    ZXIC_UINT32 ddr3_share_type;      /** <@brief ddr 共享类型   */
    ZXIC_UINT32 ddr3_rw_len;          /** <@brief 表项返回/写入位宽 */
    ZXIC_UINT32 ddr3_sdt_num;         /** <@brief SDT表号/复制信息ram的表号 */
    ZXIC_UINT32 ddr3_ecc_en;          /** <@brief ecc使能        */
    ZXIC_UINT32 ddr3_clutch_en;       /** <@brief 抓包使能       */
} DPP_SDTTBL_DDR3_T;

/**  HASH表SDT属性*/
typedef struct dpp_sdt_tbl_hash_t
{
    ZXIC_UINT32 table_type;          /** <@brief 查找表项类型       */
    ZXIC_UINT32 hash_id;             /** <@brief 访问hash的引擎     */
    ZXIC_UINT32 hash_table_width;    /** <@brief hash 表项存储位宽  */
    ZXIC_UINT32 key_size;            /** <@brief hash 键值长度      */
    ZXIC_UINT32 hash_table_id;       /** <@brief hash 逻辑表号      */
    ZXIC_UINT32 learn_en;            /** <@brief 硬件学习使能       */
    ZXIC_UINT32 keep_alive;          /** <@brief 保活标志使能       */
    ZXIC_UINT32 keep_alive_baddr;     /** <@brief 保活标志基地址     */
    ZXIC_UINT32 rsp_mode;            /** <@brief 表项返回数据位宽   */
    ZXIC_UINT32 hash_clutch_en;      /** <@brief 抓包使能           */
} DPP_SDTTBL_HASH_T;

/**  LPM表SDT属性*/
typedef struct dpp_sdt_tbl_lpm_t
{
    ZXIC_UINT32 table_type;          /** <@brief 查找表项类型       */
    ZXIC_UINT32 lpm_v46_id;          /** <@brief ipv4/ipv6标志      */
    ZXIC_UINT32 rsp_mode;            /** <@brief 表项返回数据位宽   */
    ZXIC_UINT32 lpm_table_depth;     /** <@brief 表项深度，越界检查 */
    ZXIC_UINT32 lpm_clutch_en;       /** <@brief 抓包使能           */
} DPP_SDTTBL_LPM_T;

/**  eTCAM表SDT属性*/
typedef struct dpp_sdt_tbl_etcam_t
{
    ZXIC_UINT32 table_type;          /** <@brief 查找表项类型       */
    ZXIC_UINT32 etcam_id;            /** <@brief etcam通道         */
    ZXIC_UINT32 etcam_key_mode;      /** <@brief etcam键值长度      */
    ZXIC_UINT32 etcam_table_id;      /** <@brief etcam表项号        */
    ZXIC_UINT32 no_as_rsp_mode;      /** <@brief handle模式返回位宽 */
    ZXIC_UINT32 as_en;               /** <@brief 级联eram使能       */
    ZXIC_UINT32 as_eram_baddr;       /** <@brief 级联eram基地址     */
    ZXIC_UINT32 as_rsp_mode;         /** <@brief 级联返回位宽       */
    ZXIC_UINT32 etcam_table_depth;   /** <@brief 表项深度，越界检查 */
    ZXIC_UINT32 etcam_clutch_en;     /** <@brief 抓包使能           */
} DPP_SDTTBL_ETCAM_T;

/**  物理端口属性表SDT属性*/
typedef struct dpp_sdt_tbl_porttbl_t
{
    ZXIC_UINT32 table_type;          /** <@brief 查找表项类型       */
    ZXIC_UINT32 porttbl_clutch_en;   /** <@brief 抓包使能           */
} DPP_SDTTBL_PORTTBL_T;

/***********************************************************/
/** 初始化SDT表配置管理
* @param   dev_num       设备数目
* @param   dev_id_array  设备dev_id数组
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wcl      @date  2015/07/13
************************************************************/
DPP_STATUS dpp_sdt_init(ZXIC_UINT32 dev_num, ZXIC_UINT32 *dev_id_array);

/***********************************************************/
/** 写SDT属性表条目到硬件表，同时向8个cluster写入
* @param   dev_id      设备号
* @param   sdt_no      业务表对应的sdt号
* @param   table_type  SDT属性中的表类型，取值参考DPP_SDT_TABLE_TYPE_E的定义(仅添加操作时有效)
* @param   p_sdt_info  写入的SDT属性(仅添加操作时有效)。由table_type确定此ZXIC_VOID型指针对应的数据结构, 包括: \n
*                      DPP_SDTTBL_ERAM_T、DPP_SDTTBL_DDR3_T、DPP_SDTTBL_HASH_T、DPP_SDTTBL_LPM_T、\n
*                      DPP_SDTTBL_ETCAM_T、DPP_SDTTBL_XTCAM_T、DPP_SDTTBL_PORTTBL_T。
* @param   opr_type    操作类型: 0-添加条目，1-删除条目.
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wcl      @date  2015/07/11
************************************************************/
DPP_STATUS dpp_sdt_tbl_write(DPP_DEV_T *dev,
                             ZXIC_UINT32 sdt_no,
                             ZXIC_UINT32 table_type,
                             ZXIC_VOID *p_sdt_info,
                             ZXIC_UINT32 opr_type);

/***********************************************************/
/** 单个hash引擎查找调试函数
* @param   p_se_cfg    算法模块公共管理数据结构指针
* @param   fun_id      hash引擎号
* @param   p_entry     hash条目，包括key和result，查找时的key的位宽为392bit，格式为:wr_flag(1bit) + key_type(2bit) + tbl_id(5bit) + reserve(M bit)+ actu_key(32*N bit) \n
*                      result用于在硬件学习使能的情况下，返回空闲位置的地址。
* @param   p_space_vld 学习使能时，是否有空闲空间
* @param   srh_mode    查找模式，取值参考DPP_HASH_SRH_MODE的定义
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wcl      @date  2015/05/15
************************************************************/
DPP_STATUS dpp_hash_search(DPP_SE_CFG *p_se_cfg, ZXIC_UINT32 fun_id, DPP_HASH_ENTRY *p_entry, ZXIC_UINT32 *p_space_vld, ZXIC_UINT32 srh_mode);

#endif

#endif /*dpp_se_api.h*/
