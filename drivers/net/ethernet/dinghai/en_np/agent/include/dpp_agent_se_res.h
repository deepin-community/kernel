#ifndef DPP_AGENT_SE_RES_H
#define DPP_AGENT_SE_RES_H

#include "zxic_common.h"
#include "dpp_type_api.h"

#define HASH_FUNC_MAX_NUM      (4)
#define HASH_BULK_MAX_NUM      (32)
#define HASH_TABLE_MAX_NUM     (38)
#define ERAM_MAX_NUM           (60)
#define ETCAM_MAX_NUM          (8)
#define DDR_MAX_NUM            (50)
#define LPM_MAX_NUM            (2)
#define STAT_ITEM_MAX_NUM      (256)

#define ETCAM_BLOCK_NUM           (8)
#define SMMU0_LPM_AS_TBL_ID_NUM   (8)

#pragma pack(1)

typedef struct sdt_tbl_eram_t
{
    ZXIC_UINT32 table_type;          /** <@brief 查找表项类型 */
    ZXIC_UINT32 eram_mode;           /** <@brief eRam返回位宽 */
    ZXIC_UINT32 eram_base_addr;      /** <@brief eRam表项基地址，128bit为单位 */
    ZXIC_UINT32 eram_table_depth;    /** <@brief 表项深度，作为越界检查使用   */
    ZXIC_UINT32 eram_clutch_en;      /** <@brief 抓包使能 */
} SDTTBL_ERAM_T;

/**  DDR3直接表SDT属性*/
typedef struct sdt_tbl_ddr3_t
{
    ZXIC_UINT32 table_type;           /** <@brief 查找表项类型   */
    ZXIC_UINT32 ddr3_base_addr;       /** <@brief ddr 基地址     */
    ZXIC_UINT32 ddr3_share_type;      /** <@brief ddr 共享类型   */
    ZXIC_UINT32 ddr3_rw_len;          /** <@brief 表项返回/写入位宽 */
    ZXIC_UINT32 ddr3_sdt_num;         /** <@brief SDT表号/复制信息ram的表号 */
    ZXIC_UINT32 ddr3_ecc_en;          /** <@brief ecc使能        */
    ZXIC_UINT32 ddr3_clutch_en;       /** <@brief 抓包使能       */
} SDTTBL_DDR3_T;

/**  HASH表SDT属性*/
typedef struct sdt_tbl_hash_t
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
} SDTTBL_HASH_T;

/**  LPM表SDT属性*/
typedef struct sdt_tbl_lpm_t
{
    ZXIC_UINT32 table_type;          /** <@brief 查找表项类型       */
    ZXIC_UINT32 lpm_v46_id;          /** <@brief ipv4/ipv6标志      */
    ZXIC_UINT32 rsp_mode;            /** <@brief 表项返回数据位宽   */
    ZXIC_UINT32 lpm_table_depth;     /** <@brief 表项深度，越界检查 */
    ZXIC_UINT32 lpm_clutch_en;       /** <@brief 抓包使能           */
} SDTTBL_LPM_T;

/**  eTCAM表SDT属性*/
typedef struct sdt_tbl_etcam_t
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
} SDTTBL_ETCAM_T;

typedef struct hash_func_res_t
{
    ZXIC_UINT32 func_id;        
    ZXIC_UINT32 zblk_num;      
    ZXIC_UINT32 zblk_bitmap;   
    ZXIC_UINT32 ddr_dis;        
} HASH_FUNC_RES_T;

typedef struct hash_bulk_res_t
{
    ZXIC_UINT32 func_id;                     
    ZXIC_UINT32 bulk_id;                    
    ZXIC_UINT32 zcell_num;                  
    ZXIC_UINT32 zreg_num;                    
    ZXIC_UINT32 ddr_baddr;                   
    ZXIC_UINT32 ddr_item_num;                
    ZXIC_UINT32 ddr_width_mode;  
    ZXIC_UINT32 ddr_crc_sel;                 
    ZXIC_UINT32 ddr_ecc_en;                  
} HASH_BULK_RES_T;

typedef struct hash_table_t
{
    ZXIC_UINT32 sdtNo;         
    ZXIC_UINT32 sdt_partner;   
    SDTTBL_HASH_T hashSdt;  
    ZXIC_UINT32 tbl_flag;       
} HASH_TABLE_T;

typedef struct eram_table_t
{
    ZXIC_UINT32 sdtNo;          
    SDTTBL_ERAM_T eRamSdt; 
    ZXIC_UINT32 opr_mode;     
    ZXIC_UINT32 rd_mode;      
} ERAM_TABLE_T;

typedef struct ddr_table_t
{
    ZXIC_UINT32 sdtNo;          /** <@brief sdt no 0~255 */
    SDTTBL_DDR3_T eDdrSdt;  /** <@brief DDR属性*/
    ZXIC_UINT32 ddr_table_depth;/** <@brief DDR表项深度，单位与读写模式一致*/
} DDR_TABLE_T;

typedef struct acl_res_t
{
    ZXIC_UINT32 pri_mode;      /** <@brief1：显式优先级，2：隐式优先级，以条目下发顺序作为优先级，3：用户指定每个条目在tcam中的存放索引*/
    ZXIC_UINT32 entry_num;      /** <@brief 可配置的条目数*/
    ZXIC_UINT32 block_num;      /** <@brief  最大8个 */
    ZXIC_UINT32 block_index[ETCAM_BLOCK_NUM]; /** <@brief  0~7 */
} ACL_RES_T;

typedef struct acl_table_t
{
    ZXIC_UINT32 sdtNo;          /** <@brief sdt no 0~255 */
    ZXIC_UINT32 sdt_partner;    /** <@brief sdt no 0~255,eram直接表维护acl index信息，不存在，则设置无效值-1(0xffffffff) */
    SDTTBL_ETCAM_T aclSdt;  /** <@brief acl属性*/
    ACL_RES_T aclRes;   /** <@brief acl资源*/
} ACL_TABLE_T;

typedef struct route_as_eram_t
{
    ZXIC_UINT32 baddr;     /**<  @brief LPM级联eRam结果基地址*/
    ZXIC_UINT32 rsp_mode;  /**<  @brief LPM级联eRam结果位宽模式，取值参照ERAM128_TBL_MODE_E的定义*/
} ROUTE_AS_ERAM_T;

/**  前缀匹配路由级联片外DDR结果表属性 */
typedef struct route_as_ddr_t
{
    ZXIC_UINT32 baddr;        /**<  @brief 分配给级联ddr结果表空间的基地址，以4K*128bit为单位*/
    ZXIC_UINT32 rsp_len;      /**<  @brief LPM级联DDR结果位宽模式，取值参照DPP_ROUTE_AS_RSP_LEN_E的定义*/
    ZXIC_UINT32 ecc_en;       /**<  @brief 级联结果表DDR空间ECC校验使能标志: 0-不使能，1-使能*/
} ROUTE_AS_DDR_T;

typedef struct lpm_res_t
{
    ZXIC_UINT32 pri_mode;      /** <@brief1：显式优先级，2：隐式优先级，以条目下发顺序作为优先级，3：用户指定每个条目在tcam中的存放索引*/
    ZXIC_UINT32 entry_num;      /** <@brief 可配置的条目数*/
    ZXIC_UINT32 block_num;      /** <@brief  最大8个 */
    ZXIC_UINT32 block_index[ETCAM_BLOCK_NUM]; /** <@brief  0~7 */
} LPM_RES_T;

/* @param   lpm_flags        配置信息
*|0:eRam(第5bit)1:ddr|   (第4bit)      |   (第3bit)      |    (第2bit)   |   (第1bit)   |       (第0bit)       |
*| 级联结果表模式      | v6是否非线速模式  | v4是否非线速模式  | 是否v6片外查找  | 是否v4片外查找 | 是否使能级联结果表查找  |*/
typedef struct route_res_t
{
    ZXIC_UINT32 lpm_flags;
    ZXIC_UINT32 zblk_num;              /**<  @brief LPM ipv4和ipv6共享的zblock数目*/
    ZXIC_UINT32 zblk_bitmap;           /**<  @brief LPM ipv4和ipv6共享的bitmap*/
    ZXIC_UINT32 mono_ipv4_zblk_num;    /**<  @brief ipv4独占zblock数目*/
    ZXIC_UINT32 mono_ipv4_zblk_bitmap; /**<  @brief ipv4独占zblock bitmap*/
    ZXIC_UINT32 mono_ipv6_zblk_num;    /**<  @brief ipv6独占zblock数目*/
    ZXIC_UINT32 mono_ipv6_zblk_bitmap; /**<  @brief ipv6独占zblock bitmap*/
    ZXIC_UINT32 ddr4_item_num;         /**<  @brief 分配给ipv4前缀查找的ddr存储条目数，以256bit为单位*/
    ZXIC_UINT32 ddr4_baddr;            /**<  @brief 分配给ipv4前缀查找的ddr存储空间的基地址，以4K*128bit为单位*/
    ZXIC_UINT32 ddr4_base_offset;      /**<  @brief ipv4前缀查找相对于片外ddr存储空间基地址的偏移量，以256bit为单位*/
    ZXIC_UINT32 ddr4_ecc_en;           /**<  @brief 固定配为1，分配给ipv4前缀查找的ddr存储空间的ECC校验使能标志*/
    ZXIC_UINT32 ddr6_item_num;         /**<  @brief 分配给ipv6前缀查找的ddr存储条目数，以256bit为单位*/
    ZXIC_UINT32 ddr6_baddr;            /**<  @brief 分配给ipv6前缀查找的ddr存储空间的基地址，以4K*128bit为单位*/
    ZXIC_UINT32 ddr6_base_offset;      /**<  @brief ipv6前缀查找相对于片外ddr存储空间基地址的偏移量，以256bit为单位*/
    ZXIC_UINT32 ddr6_ecc_en;           /**<  @brief 固定配为1，分配给ipv4前缀查找的ddr存储空间的ECC校验使能标志*/
} ROUTE_RES_T;

typedef struct lpm_table_t
{
    ZXIC_UINT32 sdtNo;             /** <@brief sdt no 0~255 */
    SDTTBL_LPM_T lpmSdt;       /** <@brief lpm属性*/
    ROUTE_AS_ERAM_T as_eram_cfg[SMMU0_LPM_AS_TBL_ID_NUM];  /**<  @brief LPM级联eRam结果表空间属性*/
    ROUTE_AS_DDR_T  as_ddr_cfg;   /**<  @brief LPM级联DDR结果表空间属性*/
} LPM_TABLE_T;

typedef struct se_hash_func_bulk_t
{
    ZXIC_UINT32 func_num; 
    ZXIC_UINT32 bulk_num;
    HASH_FUNC_RES_T fun[HASH_FUNC_MAX_NUM];
    HASH_BULK_RES_T bulk[HASH_BULK_MAX_NUM];
}SE_HASH_FUNC_BULK_T;

typedef struct se_hash_tbl_t
{
    ZXIC_UINT32 tbl_num;
    HASH_TABLE_T table[HASH_TABLE_MAX_NUM];
}SE_HASH_TBL_T;

typedef struct se_eram_tbl_t
{
    ZXIC_UINT32 tbl_num;
    ERAM_TABLE_T eram[ERAM_MAX_NUM];
}SE_ERAM_TBL_T;

typedef struct se_acl_tbl_t
{
    ZXIC_UINT32 tbl_num; 
    ACL_TABLE_T acl[ETCAM_MAX_NUM];
} SE_ACL_TBL_T;

typedef struct se_ddr_tbl_t
{
    ZXIC_UINT32 tbl_num;
    DDR_TABLE_T ddr[DDR_MAX_NUM];
}SE_DDR_TBL_T;

typedef struct se_lpm_tbl_t
{
    ZXIC_UINT32 tbl_num;              /*最大个数为2*/
    LPM_TABLE_T lpm_res[LPM_MAX_NUM];     /*ipv4/ipv6资源*/
    ROUTE_RES_T glb_res;                  /*ipv4/ipv6公共资源*/
} SE_LPM_TBL_T;

typedef struct se_stat_cfg_t
{
    ZXIC_UINT32 eram_baddr;      /*片内统计基地址，单位128bit*/
    ZXIC_UINT32 eram_depth;      /*片内统计深度，单位128bit*/
    ZXIC_UINT32 ddr_baddr;       /*片外统计基地址，单位2k*256bit*/ 
    ZXIC_UINT32 ppu_ddr_offset;  /*片外DDR统计偏移，单位128bit，默认为0*/
} SE_STAT_CFG_T;

typedef struct zxdh_np_se_res_t
{
    SE_HASH_FUNC_BULK_T hash_func_bulk;
    SE_HASH_TBL_T       hash_tbl;
    SE_ERAM_TBL_T       eram_tbl;
    SE_ACL_TBL_T        acl_tbl;
    SE_LPM_TBL_T        lpm_tbl;
    SE_DDR_TBL_T        ddr_tbl;
    SE_STAT_CFG_T       stat_cfg;
} ZXDH_NP_SE_RES_T;

typedef struct zxdh_np_res
{
    ZXDH_NP_SE_RES_T std_res;
    ZXDH_NP_SE_RES_T offload_res;
}ZXDH_NP_RES;

#pragma pack()

#endif