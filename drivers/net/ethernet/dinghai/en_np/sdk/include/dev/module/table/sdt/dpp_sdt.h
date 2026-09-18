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

#ifndef _DPP_SDT_H_
#define _DPP_SDT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dpp_ppu.h"

/**  SDT属性表高32bit中各比特字段的位宽定义*/
#define DPP_SDT_H_TBL_TYPE_BT_POS                   (29)
#define DPP_SDT_H_TBL_TYPE_BT_LEN                   (3)

#define DPP_SDT_H_ERAM_MODE_BT_POS                  (26)
#define DPP_SDT_H_ERAM_MODE_BT_LEN                  (3)
#define DPP_SDT_H_ERAM_BASE_ADDR_BT_POS             (7)
#define DPP_SDT_H_ERAM_BASE_ADDR_BT_LEN             (19)
#define DPP_SDT_L_ERAM_TABLE_DEPTH_BT_POS           (1)
#define DPP_SDT_L_ERAM_TABLE_DEPTH_BT_LEN           (22)

#define DPP_SDT_H_DDR3_BASE_ADDR_BT_POS             (9)
#define DPP_SDT_H_DDR3_BASE_ADDR_BT_LEN             (20)
#define DPP_SDT_H_DDR3_SHARE_TYPE_BT_POS            (7)
#define DPP_SDT_H_DDR3_SHARE_TYPE_BT_LEN            (2)
#define DPP_SDT_H_DDR3_RW_LEN_BT_POS                (5)
#define DPP_SDT_H_DDR3_RW_LEN_BT_LEN                (2)
#define DPP_SDT_H_DDR3_SDT_NUM_BT_POS               (0)
#define DPP_SDT_H_DDR3_SDT_NUM_BT_LEN               (5)
#define DPP_SDT_L_DDR3_SDT_NUM_BT_POS               (29)
#define DPP_SDT_L_DDR3_SDT_NUM_BT_LEN               (3)
#define DPP_SDT_L_DDR3_ECC_EN_BT_POS                (28)
#define DPP_SDT_L_DDR3_ECC_EN_BT_LEN                (1)

#define DPP_SDT_H_HASH_ID_BT_POS                    (27)
#define DPP_SDT_H_HASH_ID_BT_LEN                    (2)
#define DPP_SDT_H_HASH_TABLE_WIDTH_BT_POS           (25)
#define DPP_SDT_H_HASH_TABLE_WIDTH_BT_LEN           (2)
#define DPP_SDT_H_HASH_KEY_SIZE_BT_POS              (19)
#define DPP_SDT_H_HASH_KEY_SIZE_BT_LEN              (6)
#define DPP_SDT_H_HASH_TABLE_ID_BT_POS              (14)
#define DPP_SDT_H_HASH_TABLE_ID_BT_LEN              (5)
#define DPP_SDT_H_LEARN_EN_BT_POS                   (13)
#define DPP_SDT_H_LEARN_EN_BT_LEN                   (1)
#define DPP_SDT_H_KEEP_ALIVE_BT_POS                 (12)
#define DPP_SDT_H_KEEP_ALIVE_BT_LEN                 (1)
#define DPP_SDT_H_KEEP_ALIVE_BADDR_BT_POS           (0)
#define DPP_SDT_H_KEEP_ALIVE_BADDR_BT_LEN           (12)
#define DPP_SDT_L_KEEP_ALIVE_BADDR_BT_POS           (25)
#define DPP_SDT_L_KEEP_ALIVE_BADDR_BT_LEN           (7)
#define DPP_SDT_L_RSP_MODE_BT_POS                   (23)
#define DPP_SDT_L_RSP_MODE_BT_LEN                   (2)

#define DPP_SDT_H_LPM_V46ID_BT_POS                  (28)
#define DPP_SDT_H_LPM_V46ID_BT_LEN                  (1)
#define DPP_SDT_H_LPM_RSP_MODE_BT_POS               (0)
#define DPP_SDT_H_LPM_RSP_MODE_BT_LEN               (2)
#define DPP_SDT_L_LPM_TABLE_DEPTH_BT_POS            (1)
#define DPP_SDT_L_LPM_TABLE_DEPTH_BT_LEN            (30)

#define DPP_SDT_H_ETCAM_ID_BT_POS                   (27)
#define DPP_SDT_H_ETCAM_ID_BT_LEN                   (1)
#define DPP_SDT_H_ETCAM_KEY_MODE_BT_POS             (25)
#define DPP_SDT_H_ETCAM_KEY_MODE_BT_LEN             (2)
#define DPP_SDT_H_ETCAM_TABLE_ID_BT_POS             (21)
#define DPP_SDT_H_ETCAM_TABLE_ID_BT_LEN             (4)
#define DPP_SDT_H_ETCAM_NOAS_RSP_MODE_BT_POS        (19)
#define DPP_SDT_H_ETCAM_NOAS_RSP_MODE_BT_LEN        (2)
#define DPP_SDT_H_ETCAM_AS_EN_BT_POS                (18)
#define DPP_SDT_H_ETCAM_AS_EN_BT_LEN                (1)
#define DPP_SDT_H_ETCAM_AS_ERAM_BADDR_BT_POS        (0)
#define DPP_SDT_H_ETCAM_AS_ERAM_BADDR_BT_LEN        (18)
#define DPP_SDT_L_ETCAM_AS_ERAM_BADDR_BT_POS        (31)
#define DPP_SDT_L_ETCAM_AS_ERAM_BADDR_BT_LEN        (1)
#define DPP_SDT_L_ETCAM_AS_RSP_MODE_BT_POS          (28)
#define DPP_SDT_L_ETCAM_AS_RSP_MODE_BT_LEN          (3)
#define DPP_SDT_L_ETCAM_TABLE_DEPTH_BT_POS          (1)
#define DPP_SDT_L_ETCAM_TABLE_DEPTH_BT_LEN          (20)

#define DPP_SDT_L_CLUTCH_EN_BT_POS                  (0)
#define DPP_SDT_L_CLUTCH_EN_BT_LEN                  (1)

#define DPP_SDT_TBL_TYPE_NUM                        (8)

/***********************************************************/
/** 解析从硬件读取的64bit SDT属性
* @param   sdt_hig32   硬件表中存储的SDT属性高32bit
* @param   sdt_low32   硬件表中存储的SDT属性低32bit
* @param   p_sdt_info  解析之后的SDT属性，根据SDT属性中的table_type确定此ZXIC_VOID型指针对应的数据结构, 包括: \n
*                      DPP_SDTTBL_ERAM_T、DPP_SDTTBL_DDR3_T、DPP_SDTTBL_HASH_T、DPP_SDTTBL_LPM_T、\n
*                      DPP_SDTTBL_ETCAM_T、DPP_SDTTBL_PORTTBL_T。
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_sdt_tbl_data_parser(DPP_DEV_T *dev, ZXIC_UINT32 sdt_hig32, ZXIC_UINT32 sdt_low32, ZXIC_VOID *p_sdt_info);

/***********************************************************/
/** 从软件缓存中获取table data信息
* @param   dev_id      设备号
* @param   sdt_no      业务表对应的sdt号,0-255
* @param   p_sdt_data  sdt表信息
*
* @return  
* @remark  无
* @see
* @author  lim      @date  2020/04/16
************************************************************/
DPP_STATUS dpp_sdt_tbl_data_get(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no, DPP_SDT_TBL_DATA_T *p_sdt_data);

/***********************************************************/
/** 将sdt信息保存在软件缓存中
* @param   dev_id      设备号
* @param   sdt_no      业务表对应的sdt号
* @param   table_type  SDT属性中的表类型，取值参考DPP_SDT_TABLE_TYPE_E的定义
* @param   p_sdt_info  写入的SDT表信息。
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  lim      @date  2020/04/16
************************************************************/
DPP_STATUS dpp_soft_sdt_tbl_set(DPP_DEV_T *dev, 
                                ZXIC_UINT32 sdt_no, 
                                ZXIC_UINT32 table_type, 
                                DPP_SDT_TBL_DATA_T *p_sdt_info);

/***********************************************************/
/** 从软件缓存中获取sdt信息
* @param   device_id   设备号
* @param   sdt_no      业务表对应的sdt号
* @param   p_sdt_info  写入的SDT属性。由table_type确定此ZXIC_VOID型指针对应的数据结构, 包括: \n
*                      DPP_SDTTBL_ERAM_T、DPP_SDTTBL_DDR_T、DPP_SDTTBL_HASH_T、DPP_SDTTBL_LPM_T、
*                      DPP_SDTTBL_ETCAM_T。
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  lim      @date  2020/04/16
************************************************************/
DPP_STATUS dpp_soft_sdt_tbl_get(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no, ZXIC_VOID *p_sdt_info);

#ifdef __cplusplus
}
#endif

#endif
