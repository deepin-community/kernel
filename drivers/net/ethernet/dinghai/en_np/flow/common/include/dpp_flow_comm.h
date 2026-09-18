#ifndef DPP_FLOW_COMM_H
#define DPP_FLOW_COMM_H

#include "zxic_common.h"
#include "dpp_type_api.h"
#include "dpp_dev.h"
#include "dpp_dtb.h"
#include "dpp_tbl_comm.h"

#define DPP_ATTR_FLAG_KEY      (0<<0)            /* 键值 */
#define DPP_ATTR_FLAG_MASK     (1<<0)            /* 掩码 */
#define DPP_ATTR_FLAG_RST      (1<<1)            /* 结果 */

typedef enum dpp_flow_sdt_type_e
{
    DPP_FLOW_SDT_INVALID = 0, /**<  @brief 无效类型*/
    DPP_FLOW_SDT_ERAM    = 1, /**<  @brief eRAM直接表类型*/
    DPP_FLOW_SDT_DDR     = 2, /**<  @brief DDR直接表类型*/
    DPP_FLOW_SDT_HASH    = 3, /**<  @brief Hash表类型*/
    DPP_FLOW_SDT_LPM     = 4, /**<  @brief LPM表类型*/
    DPP_FLOW_SDT_ACL     = 5, /**<  @brief 片内Tcam表类型*/
    DPP_FLOW_SDT_MAX     = 6,
} DPP_FLOW_SDT_TYPE_E;

typedef struct zxdh_flow_attr_field_t
{
    ZXIC_CHAR    *p_field_name;                   /* 属性名 */
    ZXIC_UINT32  flags;                           /* 属性特征key mask rst*/
    ZXIC_UINT16  array_num;                       /* 属性数组个数*/
    ZXIC_UINT32  element_size;                    /* 数组元素的大小，以字节为单位*/
    ZXIC_UINT16  msb_pos;                         /* 最高比特位置，以属性列表为准*/
    ZXIC_UINT16  len;                             /* 字段长度，以比特为单位 */
}ZXDH_FLOW_ATTR_FIELD_T;

typedef struct zxdh_flow_attr_t
{
    ZXIC_CHAR    *attr_name;                      /* 结构体名称*/
    ZXIC_UINT32  sdt_no;                          /* sdt号 */
    ZXIC_UINT32  table_type;                      /* 流表类型ERAM/HASH/ACL/DDR/LPM DPP_SDT_TABLE_TYPE_E*/      
    ZXIC_UINT32  width;                           /* hash条目或者直接表表项位宽，以bit为单位 */
    ZXIC_UINT32  key_width;                       /* hash键值或者acl键值掩码位宽，以bit为单位 */  
    ZXIC_UINT32  rst_width;                       /* hash结果或者acl级联结果位宽,以bit为单位*/
    ZXIC_UINT32  field_num;                       /* 包含的字段个数 */
    ZXDH_FLOW_ATTR_FIELD_T *p_fields;             /* 结构体所有字段 */
}ZXDH_FLOW_ATTR_T;

DPP_STATUS dpp_apt_dtb_eram_get_ex(DPP_DEV_T *dev, ZXIC_UINT32 queue_id, ZXIC_UINT32 sdt_no, ZXIC_UINT32 index, void *pData);
DPP_STATUS dpp_apt_dtb_eram_insert_ex(DPP_DEV_T *dev,ZXIC_UINT32 queue_id,ZXIC_UINT32 sdt_no,ZXIC_UINT32 index,void *pData);
DPP_STATUS dpp_apt_dtb_eram_clear_ex(DPP_DEV_T *dev,ZXIC_UINT32 queue_id,ZXIC_UINT32 sdt_no,ZXIC_UINT32 index);

DPP_STATUS dpp_apt_dtb_hash_search_ex(DPP_DEV_T *dev,ZXIC_UINT32 queue_id,ZXIC_UINT32 sdt_no, void *pData);
DPP_STATUS dpp_apt_dtb_hash_insert_ex(DPP_DEV_T *dev,ZXIC_UINT32 queue_id,ZXIC_UINT32 sdt_no,void *pData);
DPP_STATUS dpp_apt_dtb_hash_delete_ex(DPP_DEV_T *dev,ZXIC_UINT32 queue_id,ZXIC_UINT32 sdt_no,void *pData);
DPP_STATUS dpp_apt_dtb_hash_simu_mcode_insert_ex(DPP_DEV_T *dev, ZXIC_UINT32 queue_id, ZXIC_UINT32 sdt_no, void *pData);
DPP_STATUS dpp_apt_dtb_hash_simu_mcode_delete_ex(DPP_DEV_T *dev, ZXIC_UINT32 queue_id, ZXIC_UINT32 sdt_no, void *pData);

DPP_STATUS dpp_apt_dtb_acl_entry_search_ex(DPP_DEV_T *dev,ZXIC_UINT32 queue_id, ZXIC_UINT32 sdt_no, ZXIC_UINT32 handle, void *pData);
DPP_STATUS dpp_apt_dtb_acl_entry_get_ex(DPP_DEV_T *dev,ZXIC_UINT32 queue_id, ZXIC_UINT32 sdt_no, ZXIC_UINT32 handle, void *pData);
DPP_STATUS dpp_apt_dtb_acl_entry_insert_ex(DPP_DEV_T *dev,ZXIC_UINT32 queue_id, ZXIC_UINT32 sdt_no, ZXIC_UINT32 handle, void *pData);
DPP_STATUS dpp_apt_dtb_acl_entry_del_ex(DPP_DEV_T *dev,ZXIC_UINT32 queue_id, ZXIC_UINT32 sdt_no, ZXIC_UINT32 handle);


#endif