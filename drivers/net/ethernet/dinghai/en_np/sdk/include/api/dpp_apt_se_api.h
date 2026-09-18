/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_apt_se_api.h
* 文件标识 : 
* 内容摘要 : SE适配业务接口数据结构和函数声明
* 其它说明 : 
* 当前版本 : 
* 作    者 : chenqin00181032
* 完成日期 : 2022/02/22
* MANUAL_PERCENT: 100%   
 
* 修改记录1: 
* 修改日期:  
* 版 本 号:  
* 修 改 人:  
* 修改内容:  
***************************************************************/

#ifndef _DPP_APT_SE_API_H_
#define _DPP_APT_SE_API_H_

#include "zxic_common.h"

#if ZXIC_REAL("header file")
#include "dpp_dev.h"
#include "dpp_se_api.h"
#include "dpp_etcam.h"
#include "dpp_se.h"
#include "dpp_hash.h"
#include "dpp_agent_se_res.h"
#endif


#if ZXIC_REAL("data struct define")
typedef ZXIC_UINT32 (*DPP_APT_ACL_ENTRY_SET_FUNC)(ZXIC_VOID *pData,DPP_ACL_ENTRY_EX_T *aclEntry);
typedef ZXIC_UINT32 (*DPP_APT_ACL_ENTRY_GET_FUNC)(ZXIC_VOID *pData,DPP_ACL_ENTRY_EX_T *aclEntry);

typedef ZXIC_UINT32 (*DPP_APT_ERAM_SET_FUNC)(ZXIC_VOID *pData,ZXIC_UINT32 buf[4]);
typedef ZXIC_UINT32 (*DPP_APT_ERAM_GET_FUNC)(ZXIC_VOID *pData,ZXIC_UINT32 buf[4]);

typedef ZXIC_UINT32 (*DPP_APT_HASH_ENTRY_SET_FUNC)(ZXIC_VOID *pData,DPP_HASH_ENTRY *pEntry);
typedef ZXIC_UINT32 (*DPP_APT_HASH_ENTRY_GET_FUNC)(ZXIC_VOID *pData,DPP_HASH_ENTRY *pEntry);

typedef ZXIC_UINT32 (*DPP_APT_LPM_ENTRY_SET_FUNC)(ZXIC_VOID *pData,ZXIC_VOID *pEntry);
typedef ZXIC_UINT32 (*DPP_APT_LPM_ENTRY_GET_FUNC)(ZXIC_VOID *pData,ZXIC_VOID *pEntry);

typedef ZXIC_UINT32 (*DPP_APT_DDR_SET_FUNC)(ZXIC_VOID *pData,ZXIC_UINT32 buf[DPP_DIR_TBL_BUF_MAX_NUM]);
typedef ZXIC_UINT32 (*DPP_APT_DDR_GET_FUNC)(ZXIC_VOID *pData,ZXIC_UINT32 buf[DPP_DIR_TBL_BUF_MAX_NUM]);

typedef enum dpp_se_res_type_e
{
    SE_STD_NIC_RES_TYPE      = 0,  /**<  @brief 标卡资源*/
    SE_NON_STD_NIC_RES_TYPE  = 1,  /**<  @brief 非标卡资源(业务卸载)*/
    SE_RES_TYPE_BUTT
} DPP_SE_RES_TYPE_E;

typedef struct dpp_apt_eram_table_t
{
    ZXIC_UINT32 sdtNo;          /** <@brief sdt no 0~255 */
    DPP_SDTTBL_ERAM_T eRamSdt; /** <@brief eRam属性*/
    ZXIC_UINT32 opr_mode;     /**cpu读写位宽模式DPP_ERAM128_OPR_MODE_E 0:128b 1:64b 2:1b 3:32b <@*/
    ZXIC_UINT32 rd_mode;      /*读清模式DPP_ERAM128_RD_CLR_MODE_E，0:正常读 1:读清模式*/
    DPP_APT_ERAM_SET_FUNC  eram_set_func;   /** <@brief 结构体转换为码流 */
    DPP_APT_ERAM_GET_FUNC  eram_get_func;   /** <@brief 码流转换为结构体 */
} DPP_APT_ERAM_TABLE_T;

typedef struct dpp_apt_ddr_table_t
{
    ZXIC_UINT32 sdtNo;          /** <@brief sdt no 0~255 */
    DPP_SDTTBL_DDR3_T eDdrSdt;  /** <@brief DDR属性*/
    ZXIC_UINT32 ddr_table_depth;/** <@brief DDR表项深度，单位与读写模式一致*/
    DPP_APT_DDR_SET_FUNC  ddr_set_func;     /** <@brief 结构体转换为码流 */
    DPP_APT_DDR_GET_FUNC  ddr_get_func;     /** <@brief 码流转换为结构体 */
} DPP_APT_DDR_TABLE_T;

typedef struct dpp_apt_acl_res_t
{
    ZXIC_UINT32 pri_mode;      /** <@brief1：显式优先级，2：隐式优先级，以条目下发顺序作为优先级，3：用户指定每个条目在tcam中的存放索引*/
    ZXIC_UINT32 entry_num;      /** <@brief 可配置的条目数*/
    ZXIC_UINT32 block_num;      /** <@brief  最大8个 */
    ZXIC_UINT32 block_index[DPP_ETCAM_BLOCK_NUM]; /** <@brief  0~7 */
} DPP_APT_ACL_RES_T;

typedef struct dpp_apt_acl_table_t
{
    ZXIC_UINT32 sdtNo;          /** <@brief sdt no 0~255 */
    ZXIC_UINT32 sdt_partner;    /** <@brief sdt no 0~255,eram直接表维护acl index信息，不存在，则设置无效值-1(0xffffffff) */
    DPP_SDTTBL_ETCAM_T aclSdt;  /** <@brief acl属性*/
    DPP_APT_ACL_RES_T aclRes;   /** <@brief acl资源*/
    DPP_APT_ACL_ENTRY_SET_FUNC  acl_set_func;  /** <@brief 结构体转换为码流 */
    DPP_APT_ACL_ENTRY_GET_FUNC  acl_get_func;  /** <@brief 码流转换为结构体 */
} DPP_APT_ACL_TABLE_T;

typedef struct dpp_apt_hash_table_t
{
    ZXIC_UINT32 sdtNo;          /** <@brief sdt no 0~255 */
    ZXIC_UINT32 sdt_partner;    /** <@brief 二级hash sdt号0~255，如果没有二级hash，则设置为无效值-1(0xffffffff) */
    DPP_SDTTBL_HASH_T hashSdt;  /** <@brief hash sdt属性*/
    ZXIC_UINT32 tbl_flag;       /**<  @brief 业务表初始化标记（bit0：老化保活置位使能，bit1：硬件学习使能，bit2：微码写表使能）*/
    DPP_APT_HASH_ENTRY_SET_FUNC hash_set_func;  /** <@brief 结构体转换为码流,转换时预留一个字节，从第1字节开始填充 */
    DPP_APT_HASH_ENTRY_GET_FUNC hash_get_func;  /** <@brief 码流转换为结构体 */
} DPP_APT_HASH_TABLE_T;

typedef struct dpp_apt_hash_func_res_t
{
   ZXIC_UINT32 func_id;        /**<  @brief hash引擎id 0~3*/
   ZXIC_UINT32 zblk_num;       /**<  @brief 0~32*/
   ZXIC_UINT32 zblk_bitmap;    /**<  @brief 置1的bit位表示分配的block编号 */
   ZXIC_UINT32 ddr_dis;        /** <@brief 0:混合模式，1：纯片内模式*/
} DPP_APT_HASH_FUNC_RES_T;

typedef struct dpp_apt_hash_bulk_res_t
{
    ZXIC_UINT32 func_id;                     /**<  @brief 0~3*/
    ZXIC_UINT32 bulk_id;                     /**<  @brief 0~7*/
    ZXIC_UINT32 zcell_num;                   /**<  @brief 0~128*/
    ZXIC_UINT32 zreg_num;                    /**<  @brief 0~128*/
    ZXIC_UINT32 ddr_baddr;                   /**<  @brief 分配给hash的DDR空间的硬件基地址,单位2k*256bit*/
    ZXIC_UINT32 ddr_item_num;                /**<  @brief 复用字段，分配给hash的DDR空间单元数目，以256bit为一个单元（根据分配的基地址和单元数目确定分配的DDR空间大小）*/
                                             /**<  @brief 复用字段，纯片内场景，支持的hash条目最大数目*/
    DPP_HASH_DDR_WIDTH_MODE ddr_width_mode;  /**<  @brief 分配给hash的DDR空间物理存储位宽模式，0:无效值，1:256bit 2:512bit*/
    ZXIC_UINT32 ddr_crc_sel;                 /**<  @brief 选择一个DDR CRC多项式，取值范围0~3,0~3分别对应一个CRC多项式*/
    ZXIC_UINT32 ddr_ecc_en;                  /**<  @brief DDR ECC使能: 0-不使能，1-使能*/
} DPP_APT_HASH_BULK_RES_T;

typedef struct dpp_apt_route_res_t
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
} DPP_APT_ROUTE_RES_T;

typedef struct dpp_apt_lpm_table_t
{
    ZXIC_UINT32 sdtNo;             /** <@brief sdt no 0~255 */
    DPP_SDTTBL_LPM_T lpmSdt;       /** <@brief lpm属性*/
    DPP_ROUTE_AS_ERAM_T as_eram_cfg[DPP_SMMU0_LPM_AS_TBL_ID_NUM];  /**<  @brief LPM级联eRam结果表空间属性*/
    DPP_ROUTE_AS_DDR_T  as_ddr_cfg;   /**<  @brief LPM级联DDR结果表空间属性*/
    DPP_APT_LPM_ENTRY_SET_FUNC  lpm_set_func;  /** <@brief 结构体转换为码流 */
    DPP_APT_LPM_ENTRY_GET_FUNC  lpm_get_func;  /** <@brief 码流转换为结构体 */
} DPP_APT_LPM_TABLE_T;

typedef struct dpp_apt_eram_res_init_t
{
    ZXIC_UINT32 tbl_num;
    DPP_APT_ERAM_TABLE_T *eram_res;
} DPP_APT_ERAM_RES_INIT_T;

typedef struct dpp_ddr_res_init_t
{
    ZXIC_UINT32 tbl_num;
    DPP_APT_DDR_TABLE_T *ddr_res;
} DPP_APT_DDR_RES_INIT_T;

typedef struct dpp_apt_hash_res_init_t
{   
    ZXIC_UINT32 func_num; 
    ZXIC_UINT32 bulk_num;
    ZXIC_UINT32 tbl_num;
    DPP_APT_HASH_FUNC_RES_T *func_res;
    DPP_APT_HASH_BULK_RES_T *bulk_res;
    DPP_APT_HASH_TABLE_T  *tbl_res;
} DPP_APT_HASH_RES_INIT_T;

typedef struct dpp_apt_lpm_res_init_t
{
    ZXIC_UINT32 tbl_num;              /*最大个数为2*/
    DPP_APT_LPM_TABLE_T *lpm_res;     /*ipv4/ipv6资源*/
    DPP_APT_ROUTE_RES_T *glb_res;     /*ipv4/ipv6公共资源*/
} DPP_APT_LPM_RES_INIT_T;

typedef struct dpp_apt_acl_res_init_t
{
    ZXIC_UINT32 tbl_num; 
    DPP_APT_ACL_TABLE_T *acl_res;
} DPP_APT_ACL_RES_INIT_T;

typedef struct dpp_apt_stat_res_init_t
{
    ZXIC_UINT32 eram_baddr;      /*片内统计基地址，单位128bit*/
    ZXIC_UINT32 eram_depth;      /*片内统计深度，单位128bit*/
    ZXIC_UINT32 ddr_baddr;       /*片外统计基地址，单位2k*256bit*/ 
    ZXIC_UINT32 ppu_ddr_offset;  /*片外DDR统计偏移，单位128bit，默认为0*/
} DPP_APT_STAT_RES_INIT_T;

typedef struct dpp_stat_item_t{
    ZXIC_UINT32 valid;            /*有效值*/
    ZXIC_UINT32 mode;             /*统计项模式：0：64bit，1：128bit*/
    ZXIC_UINT32 addr_offset;      /*统计项地址偏移，单位与模式一致*/
    ZXIC_UINT32 depth;            /*统计项深度,单位与模式一致*/
}DPP_APT_STAT_ITEM_T;

typedef struct dpp_apt_se_res_t
{
    ZXIC_UINT32              valid;
    ZXIC_UINT32              hash_func_num;
    ZXIC_UINT32              hash_bulk_num;
    ZXIC_UINT32              hash_tbl_num;
    ZXIC_UINT32              eram_num;
    ZXIC_UINT32              acl_num;
    ZXIC_UINT32              lpm_num;
    ZXIC_UINT32              ddr_num;
    ZXIC_UINT32              stat_item_num;
    DPP_APT_HASH_FUNC_RES_T  hash_func[HASH_FUNC_MAX_NUM];
    DPP_APT_HASH_BULK_RES_T  hash_bulk[HASH_BULK_MAX_NUM];
    DPP_APT_HASH_TABLE_T     hash_tbl[HASH_TABLE_MAX_NUM];
    DPP_APT_ERAM_TABLE_T     eram_tbl[ERAM_MAX_NUM];
    DPP_APT_ACL_TABLE_T      acl_tbl[ETCAM_MAX_NUM];
    DPP_APT_ROUTE_RES_T      lpm_global_res;
    DPP_APT_LPM_TABLE_T      lpm_tbl[LPM_MAX_NUM];
    DPP_APT_DDR_TABLE_T      ddr_tbl[DDR_MAX_NUM];
    DPP_APT_STAT_RES_INIT_T  stat_cfg;
    DPP_APT_STAT_ITEM_T      stat_item[STAT_ITEM_MAX_NUM];
}DPP_APT_SE_RES_T;

#define DTB_DUMP_UNICAST_MAC_DUMP_NUM (64 * 257)
#define DTB_DUMP_MULTICAST_MAC_DUMP_NUM (32 * 257)
#endif

#if ZXIC_REAL("SE APT FUNCTION")

/***********************************************************/
/** eram表资源初始化
* @param   dev_id  设备号 
* @param   tbl_num  需初始化的eram表个数
* @param   pEramTbl  eram资源信息，包括SDT配置信息，直接表读取位宽和结构体码流转换回调函数
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_eram_res_init(DPP_DEV_T *dev,ZXIC_UINT32 tbl_num,DPP_APT_ERAM_TABLE_T *pEramTbl);

/***********************************************************/
/** DDR表资源初始化
* @param   dev_id  设备号 
* @param   tbl_num  需初始化的DDR表个数
* @param   pDdrTbl  ddr资源信息，包括SDT配置信息，直接表读取位宽和结构体码流转换回调函数
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/07/26
************************************************************/
DPP_STATUS dpp_apt_ddr_res_init(DPP_DEV_T *dev,ZXIC_UINT32 tbl_num,DPP_APT_DDR_TABLE_T *pDdrTbl);

/***********************************************************/
/** acl资源初始化
* @param   dev_id  设备号 
* @param   tbl_num     etcam对应的sdt表个数
* @param   pAclTblRes  acl表资源信息，包括SDT配置信息，acl资源(条目数，存放方式和占用的block)和结构体码流转换回调函数
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_acl_res_init(DPP_DEV_T *dev,ZXIC_UINT32 tbl_num,DPP_APT_ACL_TABLE_T *pAclTblRes);

/***********************************************************/
/** acl软件资源释放
* @param   dev  设备号 
* @return  
* @remark  无
* @see     
* @author  cq      @date  2025/06/30
************************************************************/
DPP_STATUS dpp_apt_acl_soft_res_uninit(DPP_DEV_T *dev);

/***********************************************************/
/** hash表全局资源初始化
* @param   dev_id  设备号 
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_hash_global_res_init(DPP_DEV_T *dev);

/***********************************************************/
/** hash表全局资源去初始化
* @param   dev_id  设备号 
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/08/01
************************************************************/
DPP_STATUS dpp_apt_hash_global_res_uninit(DPP_DEV_T *dev);

/***********************************************************/
/** hash引擎初始化
* @param   dev_id  设备号 
* @param   func_num     需初始化的hash引擎个数 1~4
* @param   pHashFuncRes  每个hash引擎分配的zblock个数和编号，以及分配模式(混合模式或者纯片内模式)
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_hash_func_res_init(DPP_DEV_T *dev,ZXIC_UINT32 func_num,DPP_APT_HASH_FUNC_RES_T *pHashFuncRes);

/***********************************************************/
/** hash引擎初始化(删除硬件数据)
* @param   dev_id  设备号 
* @param   func_num     需初始化的hash引擎个数 1~4
* @param   pHashFuncRes  每个hash引擎分配的zblock个数和编号，以及分配模式(混合模式或者纯片内模式)
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_hash_func_flush_hardware_all(DPP_DEV_T *dev, 
                                            ZXIC_UINT32 func_num, 
                                            DPP_APT_HASH_FUNC_RES_T *pHashFuncRes, 
                                            ZXIC_UINT32 queue_id);

/***********************************************************/
/** hash引擎bulk空间初始化
* @param   dev_id  设备号 
* @param   bulk_num     需初始化的bulk表个数 1~32
* @param   pBulkRes  zcell和zreg资源占用信息，如果是混合模式，需进行DDR资源分配
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_hash_bulk_res_init(DPP_DEV_T *dev,ZXIC_UINT32 bulk_num,DPP_APT_HASH_BULK_RES_T *pBulkRes);

/***********************************************************/
/** hash业务表属性初始化
* @param   dev_id  设备号 
* @param   tbl_num     需初始化的业务表表个数 1~128
* @param   pHashTbl  sdt配置信息，初始化标记和业务结构体码流转换函数
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_hash_tbl_res_init(DPP_DEV_T *dev,ZXIC_UINT32 tbl_num,DPP_APT_HASH_TABLE_T *pHashTbl);

/***********************************************************/
/** dtb eram表项插入/更新
* @param   dev_id  设备号 
* @param   sdt_no  SDT号 0~255
* @param   index   条目index，索引范围随wrt_mode模式不同
* @param   pData   插入表项内容，由业务确定
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_dtb_eram_insert(DPP_DEV_T *dev,ZXIC_UINT32 queue_id,ZXIC_UINT32 sdt_no,ZXIC_UINT32 index,void *pData);

/***********************************************************/
/** eram表项数据获取,从软件缓存中获取
* @param   dev_id  设备号 
* @param   sdt_no  SDT号 0~255
* @param   index   条目index，索引范围随wrt_mode模式不同
* @param   pData   出参，返回业务表项内容
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_dtb_eram_get(DPP_DEV_T *dev, ZXIC_UINT32 queue_id, ZXIC_UINT32 sdt_no, ZXIC_UINT32 index, void *pData);

/***********************************************************/
/** eram表项删除,软件维护删除
* @param   dev_id  设备号 
* @param   sdt_no  SDT号 0~255
* @param   index   条目index，索引范围随wrt_mode模式不同
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_dtb_eram_clear(DPP_DEV_T *dev,ZXIC_UINT32 queue_id,ZXIC_UINT32 sdt_no,ZXIC_UINT32 index);

/***********************************************************/
/** dtb hash表项插入/更新
* @param   dev_id  设备号 
* @param   sdt_no  sdt号 0~255
* @param   pData   插入hash表项信息,由业务确定
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_dtb_hash_insert(DPP_DEV_T *dev,ZXIC_UINT32 queue_id,ZXIC_UINT32 sdt_no,void *pData);

/***********************************************************/
/** dtb hash表项删除
* @param   dev_id  设备号 
* @param   sdt_no  sdt号 0~255
* @param   pData   删除hash表项信息,由业务传入
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_dtb_hash_delete(DPP_DEV_T *dev,ZXIC_UINT32 queue_id,ZXIC_UINT32 sdt_no,void *pData);

/***********************************************************/
/** 软件查找存储在ZCAM空间的hash表项的zcam位置
* @param   dev           设备号，支持多芯片 
* @param   sdt_no        SDT表号
* @param   pData         查找键值信息(查找成功后，填充rst)
* @param   p_pos_info    出参，zcam位置信息
* @param   p_srh_succ    出参，查找是否成功
* @return  
* @remark  无
* @see     
* @author  zth      @date  2023/08/02
************************************************************/
DPP_STATUS dpp_hash_zcam_pos_get(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no, void *pData, DPP_HASH_ZCAM_POS_INFO *p_pos_info, ZXIC_UINT8 *p_srh_succ);

/***********************************************************/
/** dtb hash表项批量插入/更新
* @param   dev_id     设备号 
* @param   queue_id   队列id
* @param   sdt_no     sdt号 0~255
* @param   entry_num  插入条目数
* @param   entry_size 插入条目结构体大小
* @param   pData      插入hash表项信息,由业务确定
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/10/23
************************************************************/
DPP_STATUS dpp_apt_dtb_multi_hash_insert(DPP_DEV_T *dev,
                                          ZXIC_UINT32 queue_id,
                                          ZXIC_UINT32 sdt_no,
                                          ZXIC_UINT32 entry_num,
                                          ZXIC_UINT32 entry_size, 
                                          ZXIC_VOID *pData);

/***********************************************************/
/** dtb hash表项批量删除
* @param   dev_id     设备号 
* @param   queue_id   队列id
* @param   sdt_no     sdt号 0~255
* @param   entry_num  删除条目数
* @param   entry_size 删除条目结构体大小
* @param   pData      删除hash表项信息,由业务确定
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/10/23
************************************************************/
DPP_STATUS dpp_apt_dtb_multi_hash_delete(DPP_DEV_T *dev,
                                          ZXIC_UINT32 queue_id,
                                          ZXIC_UINT32 sdt_no,
                                          ZXIC_UINT32 entry_num,
                                          ZXIC_UINT32 entry_size, 
                                          ZXIC_VOID *pData);

/***********************************************************/
/** acl表项插入/更新
* @param   dev_id  设备号 
* @param   sdt_no  sdt号 0~255
* @param   pData   业务插入表项内容，具体结构体由业务确定(结构体的第一个字段必须为index)，SDK不感知
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_dtb_acl_entry_insert(DPP_DEV_T *dev,ZXIC_UINT32 queue_id, ZXIC_UINT32 sdt_no, void *pData);

/***********************************************************/
/** acl表项删除
* @param   dev_id  设备号 
* @param   sdt_no  sdt号 0~255
* @param   pData   删除业务表项内容，仅需填入index信息
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/09/25
************************************************************/
DPP_STATUS dpp_apt_dtb_acl_entry_del(DPP_DEV_T *dev,ZXIC_UINT32 queue_id, ZXIC_UINT32 sdt_no, void *pData);

/***********************************************************/
/** acl表项查找(handle+data+mask有效)
* @param   dev      设备
* @param   queue_id 队列号
* @param   sdt_no   sdt号 0~255
* @param   pData    查找表项
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/09/21
************************************************************/
DPP_STATUS dpp_apt_dtb_acl_entry_search(DPP_DEV_T *dev,ZXIC_UINT32 queue_id, ZXIC_UINT32 sdt_no, void *pData);

/***********************************************************/
/** 根据handle获取到acl表项信息
* @param   dev      设备
* @param   queue_id 队列号
* @param   sdt_no   sdt号 0~255
* @param   pData    查找表项
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/09/21
************************************************************/
DPP_STATUS dpp_apt_dtb_acl_entry_get(DPP_DEV_T *dev,ZXIC_UINT32 queue_id, ZXIC_UINT32 sdt_no, void *pData);

/***********************************************************/
/** 释放sdt资源以及适配资源
* @param   dev_id      设备号
* @param   sdt_no      sdt号
* @return  
* @remark  无
* @see
* @author  cq      @date  2023/11/09
************************************************************/
DPP_STATUS dpp_apt_sdt_res_deinit(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no);

/***********************************************************/
/** 消息通道获取指定类型的所有流表资源
* @param   dev            NP设备
* @param 
* @return
* @remark  无
* @see
* @author  cq      @date  2024/11/11
************************************************************/
DPP_STATUS dpp_agent_se_res_get(DPP_DEV_T *dev);

/***********************************************************/
/** 初始化流表资源
* @param   dev            NP设备
* @param
* @return
* @remark  无
* @see
* @author  cq      @date  2024/11/11
************************************************************/
DPP_STATUS dpp_se_res_init(DPP_DEV_T *dev);

/***********************************************************/
/** 消息通道获取指定类型流表资源&流表资源初始化
* @param   dev            NP设备
* @param
* @return
* @remark  无
* @see
* @author  cq      @date  2024/11/11
************************************************************/
DPP_STATUS dpp_se_res_get_and_init(DPP_DEV_T *dev);

/***********************************************************/
/** 获取sdt对应的hash最大条目数
* @param   dev            NP设备
* @param   sdt_no   
* @param   max_num        出参，获取的条目数上限
* @param
* @return
* @remark  无
* @see
* @author  cq      @date  2024/12/03
************************************************************/
DPP_STATUS dpp_hash_max_item_num_get(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no, ZXIC_UINT32 *max_num);

/***********************************************************/
/** 获取sdt对应的可dump到的最多hash条目数
* @param   dev            NP设备
* @param   sdt_no   
* @param   max_num        出参，用户可dump出的最多条目数
* @param
* @return
* @remark  无
* @see
* @author  cq      @date  2025/08/02
************************************************************/
DPP_STATUS dpp_hash_dump_max_item_num_get(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no, ZXIC_UINT32 *max_num);


/***********************************************************/
/** 获取统计项的信息
* @param   dev               NP设备
* @param   p_se_res          流表资源
* @param
* @return
* @remark  无
* @see
* @author  cq      @date  2025/02/15
************************************************************/
DPP_STATUS dpp_stat_tbl_get(DPP_DEV_T *dev,DPP_APT_SE_RES_T *p_se_res);

/***********************************************************/
/** 查看当前sdt号是否存在
* @param   p_se_res  流表资源
* @param   sdt_type  表类型
* @param   sdt_no    sdt号
* @return  
* @remark  无
* @see     
* @author  cq      @date  2025/02/17
************************************************************/
DPP_STATUS dpp_apt_sdt_is_exist(DPP_APT_SE_RES_T *p_se_res,DPP_SDT_TABLE_TYPE_E sdt_type, ZXIC_UINT32 sdt_no,ZXIC_UINT32 *p_is_exist);

#endif

#endif