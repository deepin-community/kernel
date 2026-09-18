/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_apt_se.h
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

#ifndef _DPP_APT_SE_H_
#define _DPP_APT_SE_H_

#include "dpp_apt_se_api.h"

#define SDT_OPER_ADD (ZXIC_UINT32)(0)
#define SDT_OPER_DEL (ZXIC_UINT32)(1)

#define SDT_DDR_RW_128BIT  (ZXIC_UINT32)(0)
#define SDT_DDR_RW_256BIT  (ZXIC_UINT32)(1)
#define SDT_DDR_RW_512BIT  (ZXIC_UINT32)(2)

#define DDR_128BIT_BYTE    (ZXIC_UINT32)(16)

#define ERAM_ENTRY_SOFT_MAX (ZXIC_UINT32)(16)   /*eram最大位宽128bit*/
#define HASH_ENTRY_SOFT_MAX (ZXIC_UINT32)(64)   /*hash最大位宽512bit*/
#define ACL_ENTRY_SOFT_MAX  (ZXIC_UINT32)(160)  /*acl最大位宽640bit key+640bit mask*/

typedef struct se_apt_eram_func_t
{
    ZXIC_UINT32 opr_mode;     /**cpu读写位宽模式DPP_ERAM128_OPR_MODE_E 0:128b 1:64b 2:1b 3:32b <@*/
    ZXIC_UINT32 rd_mode;      /*读清模式DPP_ERAM128_RD_CLR_MODE_E，0:正常读 1:读清模式*/
    DPP_APT_ERAM_SET_FUNC  eram_set_func;
    DPP_APT_ERAM_GET_FUNC  eram_get_func;
}SE_APT_ERAM_FUNC_T;

typedef struct se_apt_ddr_func_t
{
    ZXIC_UINT32 ddr_tbl_depth;     /*ddr表项深度，单位与ddr读写模式一致*/
    DPP_APT_DDR_SET_FUNC  ddr_set_func;
    DPP_APT_DDR_GET_FUNC  ddr_get_func;
}SE_APT_DDR_FUNC_T;

typedef struct se_apt_acl_func_t
{
    ZXIC_UINT32 sdt_partner;
    DPP_APT_ACL_ENTRY_SET_FUNC  acl_set_func;
    DPP_APT_ACL_ENTRY_GET_FUNC  acl_get_func;
}SE_APT_ACL_FUNC_T;

typedef struct se_apt_hash_func_t
{
    DPP_APT_HASH_ENTRY_SET_FUNC  hash_set_func;
    DPP_APT_HASH_ENTRY_GET_FUNC  hash_get_func;
}SE_APT_HASH_FUNC_T;

typedef struct se_apt_lpm_func_t
{
    DPP_APT_LPM_ENTRY_SET_FUNC  lpm_set_func;
    DPP_APT_LPM_ENTRY_GET_FUNC  lpm_get_func;
}SE_APT_LPM_FUNC_T;
typedef struct se_apt_callback_t
{
    ZXIC_UINT32 sdtNo;          /** <@brief sdt no 0~255 */
    ZXIC_UINT32 table_type;     /** <@brief 查找表项类型 */

    union
    {
        SE_APT_ERAM_FUNC_T eramFunc;
        SE_APT_DDR_FUNC_T  ddrFunc;
        SE_APT_ACL_FUNC_T  aclFunc;
        SE_APT_HASH_FUNC_T hashFunc;
        SE_APT_LPM_FUNC_T  lpmFunc;
    }se_func_info;
}SE_APT_CALLBACK_T;

typedef struct se_apt_eram_convert_t
{
    ZXIC_UINT32 sdt_no;
    DPP_APT_ERAM_SET_FUNC  eram_set_func;
    DPP_APT_ERAM_GET_FUNC  eram_get_func;
}SE_APT_ERAM_CONVERT_T;

typedef struct se_apt_ddr_convert_t
{
    ZXIC_UINT32 sdt_no;
    DPP_APT_DDR_SET_FUNC  ddr_set_func;
    DPP_APT_DDR_GET_FUNC  ddr_get_func;
}SE_APT_DDR_CONVERT_T;

typedef struct se_apt_hash_convert_t
{
    ZXIC_UINT32 sdt_no;
    DPP_APT_HASH_ENTRY_SET_FUNC  hash_set_func;
    DPP_APT_HASH_ENTRY_GET_FUNC  hash_get_func;
}SE_APT_HASH_CONVERT_T;

typedef struct se_apt_acl_convert_t
{
    ZXIC_UINT32 sdt_no;
    DPP_APT_ACL_ENTRY_SET_FUNC  acl_set_func;
    DPP_APT_ACL_ENTRY_GET_FUNC  acl_get_func;
}SE_APT_ACL_CONVERT_T;

typedef struct se_apt_lpm_convert_t
{
    ZXIC_UINT32 sdt_no;
    DPP_APT_LPM_ENTRY_SET_FUNC  lpm_set_func;
    DPP_APT_LPM_ENTRY_GET_FUNC  lpm_get_func;
}SE_APT_LPM_CONVERT_T;

typedef struct se_apt_eram_soft_t
{
    ZXIC_UINT32 index;
    ZXIC_UINT32  buff[ERAM_ENTRY_SOFT_MAX/4];
}SE_APT_ERAM_SOFT_T;

typedef struct se_apt_eram_hash_t
{
    ZXIC_UINT32 index;
    ZXIC_UINT8  aucData[HASH_ENTRY_SOFT_MAX];
}SE_APT_HASH_SOFT_T;

typedef struct se_apt_eram_acl_t
{
    ZXIC_UINT32 index;
    ZXIC_UINT8  aucData[ACL_ENTRY_SOFT_MAX];
}SE_APT_ACL_SOFT_T;

ZXIC_SINT32 dpp_apt_table_key_cmp(void *p_new_key, void *p_old_key, ZXIC_UINT32 key_len);
SE_APT_CALLBACK_T *dpp_apt_get_func(DPP_DEV_T *dev,ZXIC_UINT32 sdt_no);
DPP_STATUS dpp_apt_set_callback(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no, ZXIC_UINT32 table_type,ZXIC_VOID *pData);
DPP_STATUS dpp_apt_sw_list_insert(ZXIC_RB_CFG *rb_cfg,void *pData,ZXIC_UINT32 len);
DPP_STATUS dpp_apt_sw_list_search(ZXIC_RB_CFG *rb_cfg,void *pData,ZXIC_UINT32 len);
DPP_STATUS dpp_apt_sw_list_delete(ZXIC_RB_CFG *rb_cfg,void *pData,ZXIC_UINT32 len);
DPP_STATUS dpp_apt_get_zblock_index(ZXIC_UINT32 zblock_bitmap,ZXIC_UINT32 *zblk_idx);
DPP_STATUS dpp_apt_dtb_res_init(DPP_DEV_T *dev);
DPP_STATUS dpp_apt_se_callback_init(DPP_DEV_T *dev);
ZXIC_UINT32 dpp_apt_get_sdt_partner(DPP_DEV_T *dev,ZXIC_UINT32 sdt_no);
DPP_STATUS dpp_se_res_mem_alloc(DPP_DEV_T *dev);
DPP_STATUS dpp_se_res_mem_free(DPP_DEV_T *dev);

#endif
