/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_acl.h
* 文件标识 : 
* 内容摘要 : 
* 其它说明 : 
* 当前版本 : 
* 完成日期 : 2014/12/17
* DEPARTMENT: ASIC_FPGA_R&D_Dept 
* MANUAL_PERCENT: 100%   
 
* 修改记录1: 
* 修改日期:  
* 版 本 号:  
* 修 改 人:  
* 修改内容:  
***************************************************************/
#ifndef _DPP_ACL_H_
#define _DPP_ACL_H_

#include "dpp_se_api.h"

#define DPP_ACL_TBL_ID_MIN      (0)
#define DPP_ACL_TBL_ID_MAX      (7)
#define DPP_ACL_ETCAM_ID_MIN    (0)
#define DPP_ACL_ETCAM_ID_MAX    (0)

#define DPP_ACL_ENTRY_MAX_GET(key_mode, block_num) \
    ((block_num) * DPP_ETCAM_RAM_DEPTH * (1U<<(key_mode)))

#define DPP_ACL_AS_RSLT_SIZE_GET(mode) \
    (((mode)==DPP_ACL_AS_MODE_128b)?(128/8):(((mode)==DPP_ACL_AS_MODE_64b)?(64/8):(((mode)==DPP_ACL_AS_MODE_32b)?\
    (32/8):(((mode)==DPP_ACL_AS_MODE_16b)?(16/8):(0)))))

/** 仅内部调试接口使用*/
#define DPP_ACL_AS_RSLT_SIZE_GET_EX(mode) (2U<<(mode))


/**  ACL关联查找结果表位宽模式，仅调试用*/
typedef enum dpp_acl_as_mode_ex_e
{
    DPP_ACL_AS_MODE_EX_64b  = 1,  /**<  @brief 64bit结果位宽*/
    DPP_ACL_AS_MODE_EX_128b = 2,  /**<  @brief 128bit结果位宽*/
    DPP_ACL_AS_MODE_EX_256b = 3,  /**<  @brief 256bit结果位宽，仅当关联结果表为DDR时有效*/
    DPP_ACL_AS_MODE_EX_INVALID,
}DPP_ACL_AS_MODE_EX_E;

/***********************************************************/
/** 初始化ACL公共管理数据结构
* @param   p_acl_cfg     ACL公共管理数据结构指针
* @param   p_client      用户自定义数据指针，目前仅为传入dev_id的值
* @param   flags         ACL初始化使能标志，详见DPP_ACL_FLAG_ETCAM0_EN等的定义
* @param   p_as_wrt_fun  关联结果G从布s表回调函数指针
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wcl      @date  2015/05/20
************************************************************/
DPP_STATUS dpp_acl_cfg_init(DPP_ACL_CFG_T *p_acl_cfg,
                            ZXIC_VOID *p_client,
                            ZXIC_UINT32 flags,
                            ACL_AS_RSLT_WRT_FUNCTION p_as_wrt_fun);

/***********************************************************/
/** 获取ACL公共管理数据结构
* @param   p_acl_cfg     ACL公共管理配置结构指针
*
* @return  DPP_OK-成功，DPP_ERR-失败
************************************************************/
DPP_STATUS dpp_acl_cfg_get(DPP_DEV_T *dev,DPP_ACL_CFG_EX_T **p_acl_cfg);

/***********************************************************/
/** 设置ACL全局配置
* @param   p_acl_cfg   ACL公共管理数据结构指针
*
* @return  
************************************************************/
ZXIC_VOID dpp_acl_cfg_set(DPP_DEV_T *dev, DPP_ACL_CFG_EX_T *p_acl_cfg);

/***********************************************************/
/** 初始化ACL业务表属G?
* @param   p_acl_cfg   ACL公共管理数据结构指针
* @param   table_id    业务表号，取值范围0~15
* @param   as_enable   是否使能关联结果查找，0-不使能，1-使能
* @param   entry_num   最大条目数
* @param   key_mode    键值位宽模式，取值参照DPP_ACL_KEY_MODE_E的定义
* @param   as_mode     关联结果位宽模式，取值参照DPP_ACL_AS_MODE_E的定义
* @param   block_num   分配给当前业务表号的block数目
* @param   p_block_idx 分配给当前业务表号的block编号数组
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wcl      @date  2015/05/20
************************************************************/
DPP_STATUS dpp_acl_tbl_init(DPP_ACL_CFG_T *p_acl_cfg,
                            ZXIC_UINT32 table_id,
                            ZXIC_UINT32 as_enable,
                            ZXIC_UINT32 entry_num,
                            DPP_ACL_KEY_MODE_E key_mode,
                            DPP_ACL_AS_MODE_E  as_mode,
                            ZXIC_UINT32 block_num,
                            ZXIC_UINT32 *p_block_idx);

DPP_STATUS dpp_acl_hdw_addr_get(DPP_ACL_TBL_CFG_T *p_tbl_cfg, ZXIC_UINT32 handle, ZXIC_UINT32 *p_block_idx, ZXIC_UINT32 *p_addr, ZXIC_UINT32 *p_wr_mask);
#endif



