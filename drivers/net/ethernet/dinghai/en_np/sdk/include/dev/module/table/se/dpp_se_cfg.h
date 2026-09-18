/*****************************************************************************
 * 版权所有 (C)2001-2015, 深圳市中兴通讯股份有限公司。
 * 
 * 文件名称：   dpp_se_cfg.h
 * 文件标识：   SE配置部分头文件
 * 内容摘要：   
 * 其它说明：   
 * 当前版本：    
 * 作    者：    ChenWei10088471
 * 完成日期：   
 * 当前责任人-1：
 * 当前责任人-2：
 *
 * DEPARTMENT       : ASIC_FPGA_R&D_Dept
 * MANUAL_PERCENT   : 100% 
 *****************************************************************************/

#ifndef _DPP_SE_CFG_H_
#define _DPP_SE_CFG_H_

#include "dpp_se_api.h"


#define DPP_WRITE_FILE_EN         (0)

#define LPM_OPTIMIZE_EZXIC_VOLUTION_TIME_SET_EN (0)

#define SE_ITEM_WIDTH_MAX         (64) /* 单条item最大宽度, 以字节为单位 */
#define SE_ENTRY_WIDTH_MAX        (64) /* 单条entry最大宽度, 以字节为单位 */

#define SE_RAM_WIDTH              (512)

#define IPV4_DDR_WIDTH            (256)
#define IPV6_DDR_WIDTH            (512)
#define IPV6_DDR_WIDTH_LR         (384)    /*线速宽度为384*/

#define ROUTE_DEFAULT_REG_NUM     (8)
#define ZBLK_LAST_INDREG_ADDR     (0x15)
#define ZBLK_ECC_STATU_REG_ADDR   (0x11)
#define ZBLK_HASH_LIST_REG0_ADDR  (0xd)
#define ZBLK_HASH_LIST_REG3_ADDR  (0x10)

#define ZCELL_ADDR_BT_START       (0)
#define ZCELL_ADDR_BT_WIDTH       (9)
#define ZCELL_IDX_BT_START        (9)
#define ZCELL_IDX_BT_WIDTH        (2)
#define ZBLK_IDX_BT_START         (11)
#define ZBLK_IDX_BT_WIDTH         (3)
#define ZGRP_IDX_BT_START         (14)
#define ZGRP_IDX_BT_WIDTH         (2)
#define REG_SRAM_FLAG_BT_START    (16)
#define REG_SRAM_FLAG_BT_WIDTH    (1)
#define ZBLK_WRT_MASK_BT_START    (17)
#define ZBLK_WRT_MASK_BT_WIDTH    (4)

#define ZBLK_NUM_PER_ZGRP         (8)

/*片外部分*/
#define     SE_DDR_WIDTH          (128)


struct def_route_info;


/*HASH 函数部分*/
typedef ZXIC_UINT16 (*HASH_FUNCTION)(ZXIC_UINT8 *pkey,ZXIC_UINT32 width,ZXIC_UINT16 arg);
typedef ZXIC_UINT32 (*HASH_FUNCTION32)(ZXIC_UINT8 *pkey,ZXIC_UINT32 width,ZXIC_UINT32 arg);

typedef ZXIC_UINT32 (*WR_PROCESS) (ZXIC_UINT8*p_buff, ZXIC_UINT32 size);


typedef enum file_type
{
    FILE_TYPE_REG = 0,
    FILE_TYPE_RAM,
    FILE_TYPE_ZBLK_CFG,
    FILE_TYPE_ZCELL_CFG,
    FILE_TYPE_DEF_ROUTE,
    FILE_TYPE_DDR256,
    FILE_TYPE_DDR512, 
    FILE_TYPE_V6CMP_CFG,
    FILE_TYPE_V4CMP_CFG,
    FILE_TYPE_DDR128,
}FILE_TYPE;

typedef enum se_item_type
{
    ITEM_INVALID = 0,
    ITEM_RAM,
    ITEM_DDR_256,
    ITEM_DDR_512,
    ITEM_REG,
}SE_ITEM_TYPE;


typedef enum se_fun_type
{
    FUN_HASH = 1,
    FUN_LPM,
    FUN_ACL,
    FUN_MAX
}SE_FUN_TYPE;


typedef struct file_info
{
    ZXIC_FILE       *fp;
    ZXIC_UINT32     f_status; /*1 open, 0 close*/    
}FILE_INFO;

typedef struct file_mng
{
    ZXIC_RB_CFG   rb_fn;
    FILE_INFO    *p_fi;
}SE_FILE_MNG;


#define     ZBLK_CFG_BASE          (0x8000)
#define     SERVICE_REG_ADDR       (0)
#define     MASK_REG_ADDR          (1)
#define     DEFAULT_REG_ADDR       (5)
#define     V6CMP_REG_ADDR         (0x12)
#define     V4CMP_REG_ADDR         (0x13)

#define GET_ZBLK_IDX(zcell_idx) \
    (((zcell_idx) & 0x7F) >> 2)

#define GET_ZCELL_IDX(zcell_idx) \
    ((zcell_idx) & 0x3)

#define DPP_SE_GET_ZBLK_CFG(p_se,zblk_idx) \
    (&(((DPP_SE_CFG*)(p_se))->zblk_info[zblk_idx]))

#define DPP_SE_GET_ZCELL_CFG(p_se,zcell_idx) \
    (&(((DPP_SE_CFG*)(p_se))->zblk_info[GET_ZBLK_IDX(zcell_idx)].zcell_info[GET_ZCELL_IDX(zcell_idx)]))

#define DPP_GET_FUN_INFO(p_se,fun_id) \
    (&(((DPP_SE_CFG*)(p_se))->fun_info[fun_id]))


#define ZBLK_CHECK_FULL(p_zblk_cfg) \
    (((((SE_ZBLK_CFG*)(p_zblk_cfg))->zcell_bm & 0xF )== 0xF ) ?  1 : 0 )

#define GET_ZCELL_CRC_VAL(zcell_id, crc16_val) \
    (((crc16_val) >> (zcell_id)) & (SE_RAM_DEPTH - 1))

#define ZBLK_ADDR_CONV(zblk_idx) \
    (((zblk_idx) / ZBLK_NUM_PER_ZGRP) * (1 << ZBLK_IDX_BT_WIDTH) + (zblk_idx)%ZBLK_NUM_PER_ZGRP)

#define ZCELL_ADDR_CONV(zcell_idx) \
    ((ZBLK_ADDR_CONV(((zcell_idx) >> ZCELL_IDX_BT_WIDTH) & ((1<<(ZBLK_IDX_BT_WIDTH+ZGRP_IDX_BT_WIDTH))-1)) << ZCELL_IDX_BT_WIDTH) | ((zcell_idx) & ((1<<ZCELL_IDX_BT_WIDTH)-1)))

#define ZCELL_BASE_ADDR_CALC(zcell_idx) \
    ((0xF << ZBLK_WRT_MASK_BT_START) | (((ZCELL_ADDR_CONV(zcell_idx)) & ((1<<(ZCELL_IDX_BT_WIDTH+ZBLK_IDX_BT_WIDTH+ZGRP_IDX_BT_WIDTH))-1)) << ZCELL_ADDR_BT_WIDTH))

#define ZBLK_ITEM_ADDR_CALC(zcell_idx,item_idx) \
    ((ZCELL_BASE_ADDR_CALC(zcell_idx)) | ((item_idx) & (SE_RAM_DEPTH - 1)))

#define ZBLK_REG_ADDR_CALC(zblk_idx, offset) \
    ((0xF << ZBLK_WRT_MASK_BT_START) | (0x1 << REG_SRAM_FLAG_BT_START) | ((ZBLK_ADDR_CONV(zblk_idx) & 0x1F) << ZBLK_IDX_BT_START) | ((offset) & 0x1FF))

#define ZBLK_HASH_LIST_REG_ADDR_CALC(zblk_idx, reg_idx) \
    (ZBLK_REG_ADDR_CALC((zblk_idx), (0xD + (reg_idx))))


#define  ROUTEID_CONVT_ROUTEMODE(rout_id)       ((rout_id & 0x01) ? DPP_ROUTE_MODE_IPV6 : DPP_ROUTE_MODE_IPV4)


DPP_STATUS dpp_se_cfg_set(DPP_DEV_T *dev, DPP_SE_CFG *p_se_cfg);

DPP_STATUS dpp_se_cfg_get(DPP_DEV_T *dev, DPP_SE_CFG **p_se_cfg);

DPP_STATUS dpp_se_init(DPP_DEV_T *dev, DPP_SE_CFG *p_se_cfg);

DPP_STATUS dpp_se_fun_init(DPP_SE_CFG *p_se_cfg, ZXIC_UINT8 id, ZXIC_UINT32 fun_type);

DPP_STATUS dpp_se_fun_deinit(DPP_SE_CFG *p_se_cfg, ZXIC_UINT8 id, ZXIC_UINT32 fun_type);

#define DPP_SE_CHECK_FUN(p_func_id,id,type) \
    do{\
        if (!(p_func_id)->is_used)\
        {\
            ZXIC_COMM_TRACE_ERROR("\n Error[0x%x],is_used Fun_id is invalid,(p_func_id)->is_used is [%d]",DPP_SE_RC_FUN_INVALID,(p_func_id)->is_used);\
            return DPP_SE_RC_FUN_INVALID;\
        }\
        else if ((p_func_id)->fun_id != (id))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n Error[0x%x],fun_id != (id) Fun_id is invalid;p_func_id->fun_id is [%d]",DPP_SE_RC_FUN_INVALID,(p_func_id)->fun_id);\
            return DPP_SE_RC_FUN_INVALID;\
        }\
        else if (!(p_func_id)->fun_ptr)\
        {\
            ZXIC_COMM_TRACE_ERROR("\n Error[0x%x],fun_ptr Fun_id is invalid",DPP_SE_RC_FUN_INVALID);\
            return DPP_SE_RC_FUN_INVALID;\
        }\
        else if ((p_func_id)->fun_type != (type))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n Error[0x%x],type Fun_id is invalid",DPP_SE_RC_FUN_INVALID);\
            return DPP_SE_RC_FUN_INVALID;\
        }\
     }while (0)

#define DPP_SE_CHECK_FUN_MEMORY_FREE(p_func_id,id,type,ptr) \
    do{\
        if (!(p_func_id)->is_used)\
        {\
            ZXIC_COMM_TRACE_ERROR("\n Error[0x%x],is_used Fun_id is invalid,(p_func_id)->is_used is [%d]",DPP_SE_RC_FUN_INVALID,(p_func_id)->is_used);\
            ZXIC_COMM_FREE(ptr);\
            return DPP_SE_RC_FUN_INVALID;\
        }\
        else if ((p_func_id)->fun_id != (id))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n Error[0x%x],fun_id != (id) Fun_id is invalid;p_func_id->fun_id is [%d]",DPP_SE_RC_FUN_INVALID,(p_func_id)->fun_id);\
            ZXIC_COMM_FREE(ptr);\
            return DPP_SE_RC_FUN_INVALID;\
        }\
        else if (!(p_func_id)->fun_ptr)\
        {\
            ZXIC_COMM_TRACE_ERROR("\n Error[0x%x],fun_ptr Fun_id is invalid",DPP_SE_RC_FUN_INVALID);\
            ZXIC_COMM_FREE(ptr);\
            return DPP_SE_RC_FUN_INVALID;\
        }\
        else if ((p_func_id)->fun_type != (type))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n Error[0x%x],type Fun_id is invalid",DPP_SE_RC_FUN_INVALID);\
            ZXIC_COMM_FREE(ptr);\
            return DPP_SE_RC_FUN_INVALID;\
        }\
     }while (0)

#define DPP_SE_CHECK_FUN_MUTEX_UNLOCK(p_func_id, id, type, mutex) \
do{\
    if (!(p_func_id)->is_used)\
    {\
        ZXIC_COMM_TRACE_ERROR("\n Error[0x%x],is_used Fun_id is invalid,(p_func_id)->is_used is [%d]", DPP_SE_RC_FUN_INVALID,(p_func_id)->is_used);\
        zxic_comm_mutex_unlock(mutex);\
        return DPP_SE_RC_FUN_INVALID;\
    }\
    else if ((p_func_id)->fun_id != (id))\
    {\
        ZXIC_COMM_TRACE_ERROR("\n Error[0x%x],fun_id != (id) Fun_id is invalid;p_func_id->fun_id is [%d]", DPP_SE_RC_FUN_INVALID,(p_func_id)->fun_id);\
        zxic_comm_mutex_unlock(mutex);\
        return DPP_SE_RC_FUN_INVALID;\
    }\
    else if (!(p_func_id)->fun_ptr)\
    {\
        ZXIC_COMM_TRACE_ERROR("\n Error[0x%x],fun_ptr Fun_id is invalid", DPP_SE_RC_FUN_INVALID);\
        zxic_comm_mutex_unlock(mutex);\
        return DPP_SE_RC_FUN_INVALID;\
    }\
    else if ((p_func_id)->fun_type != (type))\
    {\
        ZXIC_COMM_TRACE_ERROR("\n Error[0x%x],type Fun_id is invalid", DPP_SE_RC_FUN_INVALID);\
        zxic_comm_mutex_unlock(mutex);\
        return DPP_SE_RC_FUN_INVALID;\
    }\
  }while (0) 


#define  DPP_SE_HW_POS(x) (SE_RAM_WIDTH - 1 -(x))  

#define         DPP_SE_ZBLK_OUT_DDR_V6_START         (0)
#define         DPP_SE_ZBLK_OUT_DDR_V6_END           (0)

#define         DPP_SE_ZBLK_OUT_DDR_V4_START         (0)
#define         DPP_SE_ZBLK_OUT_DDR_V4_END           (0)

#define         DPP_SE_ZBLK_SERVICE_TYPE_START      (3)
#define         DPP_SE_ZBLK_SERVICE_TYPE_END        (3)

#define         DPP_SE_ZBLK_HASH_CHAN_START         (2)
#define         DPP_SE_ZBLK_HASH_CHAN_END           (1)

#define         DPP_SE_ZBLK_HW_POS_EN_START         (0)
#define         DPP_SE_ZBLK_HW_POS_EN_END           (0)

#endif


