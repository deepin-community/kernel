/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_dtb_cfg.h
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : zab
* 完成日期 : 2022/08/23
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/
#ifndef _DPP_DTB_CFG_H_
#define _DPP_DTB_CFG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dpp_dev.h"

#define DPP_DEV_SLOT_MAX                   (DPP_PCIE_SLOT_MAX)
#define DPP_DTB_QUEUE_NUM_MAX              (128)
#define DPP_DTB_TRAF_CTRL_RAM_SIZE         (256)
#define DPP_DTB_TRAF_CTRL_RAM_5_SIZE       (64)
#define DPP_DTB_DUMP_PD_RAM_SIZE           (2048)
#define DPP_DTB_RD_CTRL_RAM_SIZE           (4096)
#define DPP_DTB_RD_TABLE_RAM_SIZE          (8192)
#define DPP_DTB_CMD_MAN_RAM_SIZE           (16384)

/*vport格式
15 |14 13 12 |     11    |10  9  8|7 6 5 4 3 2 1 0|
rsv| ep_id   |func_active|func_num|    vfunc_num  |
*/
#define VPORT_EPID_BT_START          (12)  /*EPID起始位*/
#define VPORT_EPID_BT_LEN            (3)  /*EPID长度*/
#define VPORT_FUNC_ACTIVE_BT_START   (11)  /*FUNC_ACTIVE起始位*/
#define VPORT_FUNC_ACTIVE_BT_LEN     (1)  /*FUNC_ACTIVE长度*/
#define VPORT_FUNC_NUM_BT_START      (8)  /*FUNC_NUM起始位*/
#define VPORT_FUNC_NUM_BT_LEN        (3)  /*FUNC_NUM长度*/
#define VPORT_VFUNC_NUM_BT_START     (0)  /*FUNC_NUM起始位*/
#define VPORT_VFUNC_NUM_BT_LEN       (8)  /*FUNC_NUM长度*/

typedef struct dpp_dtb_queue_item_info_t
{
    ZXIC_UINT32 cmd_vld;
    ZXIC_UINT32 cmd_type;
    ZXIC_UINT32 int_en;
    ZXIC_UINT32 data_len;
    ZXIC_UINT32 data_laddr;
    ZXIC_UINT32 data_hddr;
} DPP_DTB_QUEUE_ITEM_INFO_T;

typedef struct dpp_dtb_queue_vm_info_t
{
    ZXIC_UINT32 dbi_en;
    ZXIC_UINT32 queue_en;
    ZXIC_UINT32 epid;
    ZXIC_UINT32 vfunc_num;
    ZXIC_UINT32 vector;
    ZXIC_UINT32 func_num;
    ZXIC_UINT32 vfunc_active;
} DPP_DTB_QUEUE_VM_INFO_T;

#if ZXIC_REAL("DTB_CFG")
/***********************************************************/
/** DTB队列元素信息配置
* @param   dev_id           芯片id
* @param   queue_id         队列ID，范围0-127
* @param   p_item_info      队列元素配置信息
*
* @return
* @remark  无
* @see
* @author  zab      @date  2018/08/23
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_item_info_set(DPP_DEV_T *dev, 
                        ZXIC_UINT32 queue_id,
                        DPP_DTB_QUEUE_ITEM_INFO_T *p_item_info);

/***********************************************************/
/** 获取DTB队列中剩余未使用的条目数量
* @param   dev_id               芯片id
* @param   queue_id             队列ID，范围0-127
* @param   p_item_num           剩余未使用条目数量
*
* @return
* @remark  无
* @see
* @author  zab      @date  2018/08/23
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_unused_item_num_get(DPP_DEV_T *dev,
                        ZXIC_UINT32 queue_id,
                        ZXIC_UINT32 *p_item_num);

/***********************************************************/
/** 配置队列VM相关信息
* @param   dev_id               芯片id
* @param   queue_id             队列ID，范围0-127
* @param   p_vm_info            VM配置信息
*
* @return
* @remark  无
* @see
* @author  zab      @date  2018/08/23
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_vm_info_set(DPP_DEV_T *dev,
                        ZXIC_UINT32 queue_id,
                        DPP_DTB_QUEUE_VM_INFO_T *p_vm_info);


/***********************************************************/
/** 获取队列VM配置信息
* @param   dev_id               оƬid
* @param   queue_id             队列ID，范围0-127
* @param   p_vm_info            VM配置信息
*
* @return
* @remark  无
* @see
* @author  zab      @date  2018/08/23
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_vm_info_get(DPP_DEV_T *dev,
                        ZXIC_UINT32 queue_id,
                        DPP_DTB_QUEUE_VM_INFO_T *p_vm_info);

/***********************************************************/
/** 配置队列使能
* @param   dev_id               芯片id
* @param   queue_id             队列ID，范围0-127
* @param   enable               1:队列使能,0:队列去使能
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/09/27
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_enable_set(DPP_DEV_T *dev,
                                     ZXIC_UINT32 queue_id,
                                     ZXIC_UINT32 enable);

/***********************************************************/
/** 获取队列使能状态
* @param   dev_id               芯片id
* @param   queue_id             队列ID，范围0-127
* @param   enable               1:队列使能,0:队列去使能
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/09/27
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_enable_get(DPP_DEV_T *dev,
                                     ZXIC_UINT32 queue_id,
                                     ZXIC_UINT32 *enable);


/***********************************************************/
/** 配置 dtb 完成中断事件状态
* @param   dev_id               芯片id
* @param   queue_id             队列ID，范围0-127
* @param   state                中断事件状态，1-发生中断，0-无中断发生
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/11/12
************************************************************/
ZXIC_UINT32 dpp_dtb_finish_interrupt_event_state_set(DPP_DEV_T *dev,
                        ZXIC_UINT32 queue_id,
                        ZXIC_UINT32 state);


/***********************************************************/
/** 清除 dtb 完成中断事件状态
* @param   dev_id               芯片id
* @param   queue_id             队列ID，范围0-127
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/11/12
************************************************************/
ZXIC_UINT32 dpp_dtb_finish_interrupt_event_state_clr(DPP_DEV_T *dev,
                                                    ZXIC_UINT32 queue_id);

ZXIC_UINT32 dpp_dtb_debug_mode_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_debug_mode);

ZXIC_UINT32 dpp_dtb_mode_is_debug(DPP_DEV_T *dev); 

/***********************************************************/
/** 读取axi 最近一次读表相关信息
* @param   dev_id           芯片id
* @param   p_last_rd_table_addr_h      axim最近一次读表高地址
* @param   p_last_rd_table_addr_l      axim最近一次读表低地址
* @param   p_last_rd_table_len         axim最近一次读表长度
* @param   p_last_rd_table_user        axim最近一次读表USER信号
* @param   p_last_rd_table_onload_cnt  axim最近一次读表在线计数
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/11/09
************************************************************/
ZXIC_UINT32 dpp_dtb_axi_last_rd_table_info_get(DPP_DEV_T *dev, 
                                               ZXIC_UINT32 *p_last_rd_table_addr_h,
                                               ZXIC_UINT32 *p_last_rd_table_addr_l,
                                               ZXIC_UINT32 *p_last_rd_table_len,
                                               ZXIC_UINT32 *p_last_rd_table_user,
                                               ZXIC_UINT32 *p_last_rd_table_onload_cnt
                                               );

/***********************************************************/
/** 读表通道错误次数统计
* @param   dev_id           芯片id
* @param   p_axi_rd_table_resp_err_cnt      读表通道返回错误次数
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/11/09
************************************************************/
ZXIC_UINT32 dpp_dtb_axi_rd_table_resp_err_cnt_get(DPP_DEV_T *dev, 
                                                  ZXIC_UINT32 *p_axi_rd_table_resp_err_cnt
                                                  ); 

/***********************************************************/
/** 读取axi 最近一次读PD相关信息
* @param   dev_id           芯片id
* @param   p_last_rd_pd_addr_h      axim最近一次读PD高地址
* @param   p_last_rd_pd_addr_l      axim最近一次读PD低地址
* @param   p_last_rd_pd_len         axim最近一次读PD长度
* @param   p_last_rd_pd_user        axim最近一次读PD USER信号
* @param   p_last_rd_pd_onload_cnt  axim最近一次读PD在线计数
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/11/09
************************************************************/
ZXIC_UINT32 dpp_dtb_axi_last_rd_pd_info_get(DPP_DEV_T *dev, 
                                            ZXIC_UINT32 *p_last_rd_pd_addr_h,
                                            ZXIC_UINT32 *p_last_rd_pd_addr_l,
                                            ZXIC_UINT32 *p_last_rd_pd_len,
                                            ZXIC_UINT32 *p_last_rd_pd_user,
                                            ZXIC_UINT32 *p_last_rd_pd_onload_cnt
                                            );

/***********************************************************/
/** 读PD通道错误次数统计
* @param   dev_id           芯片id
* @param   p_axi_rd_pd_resp_err_cnt      读PD通道返回错误次数
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/11/09
************************************************************/
ZXIC_UINT32 dpp_dtb_axi_rd_pd_resp_err_cnt_get(DPP_DEV_T *dev,
                                               ZXIC_UINT32 *p_axi_rd_pd_resp_err_cnt
                                                ); 

/***********************************************************/
/** 读取axim 最近一次写控制相关信息
* @param   dev_id           芯片id
* @param   p_last_wr_ctrl_addr_h      axim最近一次写控制高地址
* @param   p_last_wr_ctrl_addr_l      axim最近一次写控制低地址
* @param   p_last_wr_ctrl_len         axim最近一次写控制长度
* @param   p_last_wr_ctrl_user        axim最近一次写控制 USER信号
* @param   p_last_wr_ctrl_onload_cnt  axim最近一次写控制在线计数
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/11/09
************************************************************/
ZXIC_UINT32 dpp_dtb_axi_last_wr_ctrl_info_get(DPP_DEV_T *dev, 
                                              ZXIC_UINT32 *p_last_wr_ctrl_addr_h,
                                              ZXIC_UINT32 *p_last_wr_ctrl_addr_l,
                                              ZXIC_UINT32 *p_last_wr_ctrl_len,
                                              ZXIC_UINT32 *p_last_wr_ctrl_user,
                                              ZXIC_UINT32 *p_last_wr_ctrl_onload_cnt
                                             );

/***********************************************************/
/** 获取写控制通道错误次数统计
* @param   dev_id           芯片id
* @param   p_axi_wr_ctrl_resp_err_cnt      写控制通道返回错误次数
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/11/09
************************************************************/
ZXIC_UINT32 dpp_dtb_axi_wr_ctrl_resp_err_cnt_get(DPP_DEV_T *dev,
                                                ZXIC_UINT32 *p_axi_wr_ctrl_resp_err_cnt
                                                );                                              

/***********************************************************/
/** 读取axim 最近一次写DDR相关信息
* @param   dev_id           芯片id
* @param   p_last_wr_ddr_addr_h      axim最近一次写控制高地址
* @param   p_last_wr_ddr_addr_l      axim最近一次写控制低地址
* @param   p_last_wr_ddr_len         axim最近一次写控制长度
* @param   p_last_wr_ddr_user        axim最近一次写控制 USER信号
* @param   p_last_wr_ddr_onload_cnt  axim最近一次写控制在线计数
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/11/09
************************************************************/
ZXIC_UINT32 dpp_dtb_axi_last_wr_ddr_info_get(DPP_DEV_T *dev,
                                            ZXIC_UINT32 *p_last_wr_ddr_addr_h,
                                            ZXIC_UINT32 *p_last_wr_ddr_addr_l,
                                            ZXIC_UINT32 *p_last_wr_ddr_len,
                                            ZXIC_UINT32 *p_last_wr_ddr_user,
                                            ZXIC_UINT32 *p_last_wr_ddr_onload_cnt
                                            ) ;

/***********************************************************/
/** 获取写DDR通道错误次数统计
* @param   dev_id           芯片id
* @param   p_axi_wr_ddr_resp_err_cnt      写DDR通道返回错误次数
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/11/09
************************************************************/
ZXIC_UINT32 dpp_dtb_axi_wr_ddr_resp_err_cnt_get(DPP_DEV_T *dev,
                                                ZXIC_UINT32 *p_axi_wr_ddr_resp_err_cnt
                                                ) ;  

/***********************************************************/
/** 读取axim 最近一次写完成相关信息
* @param   dev_id           芯片id
* @param   p_last_wr_fin_addr_h      axim最近一次写控制高地址
* @param   p_last_wr_fin_addr_l      axim最近一次写控制低地址
* @param   p_last_wr_fin_len         axim最近一次写控制长度
* @param   p_last_wr_fin_user        axim最近一次写控制 USER信号
* @param   p_last_wr_fin_onload_cnt  axim最近一次写控制在线计数
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/11/09
************************************************************/
ZXIC_UINT32 dpp_dtb_axi_last_wr_fin_info_get(DPP_DEV_T *dev, 
                                             ZXIC_UINT32 *p_last_wr_fin_addr_h,
                                             ZXIC_UINT32 *p_last_wr_fin_addr_l,
                                             ZXIC_UINT32 *p_last_wr_fin_len,
                                             ZXIC_UINT32 *p_last_wr_fin_user,
                                             ZXIC_UINT32 *p_last_wr_fin_onload_cnt
                                            ) ; 

/***********************************************************/
/** 获取写完成通道错误次数统计
* @param   dev_id           芯片id
* @param   p_axi_wr_fin_resp_err_cnt      写完成通道返回错误次数
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/11/09
************************************************************/
ZXIC_UINT32 dpp_dtb_axi_wr_fin_resp_err_cnt_get(DPP_DEV_T *dev,
                                                ZXIC_UINT32 *p_axi_wr_fin_resp_err_cnt
                                                ) ; 

/***********************************************************/
/** 读取 DTB 各通道状态机
* @param   dev_id           芯片id
* @param   p_wr_ctrl_state_info     写控制状态机
* @param   p_rd_table_state_info    读表状态机
* @param   p_rd_pd_state_info       读描述符数据状态机
* @param   p_wr_ddr_state_info      写数据到ddr的状态机
* @param   p_wr_fin_state_info      写结束标志的状态机
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/11/09
************************************************************/
ZXIC_UINT32 dpp_dtb_state_info_get(DPP_DEV_T *dev,
                                   ZXIC_UINT32 *p_wr_ctrl_state_info,
                                   ZXIC_UINT32 *p_rd_table_state_info,
                                   ZXIC_UINT32 *p_rd_pd_state_info,
                                   ZXIC_UINT32 *p_wr_ddr_state_info,
                                   ZXIC_UINT32 *p_dump_cmd_state_info
                                   ); 
                                                                                                                                                                                                                      
/***********************************************************/
/** 各通道错误统计打印
* @param   dev_id       芯片的id号  
*
* @return  
* @remark  无
* @see     
* @author  cbb      @date  2022/11/11
************************************************************/
ZXIC_UINT32 diag_dpp_dtb_channels_axi_resp_err_cnt_prt(DPP_DEV_T *dev);

/***********************************************************/
/** AXIM最近一次操作信息记录打印
* @param   dev_id       芯片的id号  
*
* @return  
* @remark  无
* @see     
* @author  cbb      @date  2022/11/11
************************************************************/
ZXIC_UINT32 diag_dpp_dtb_axi_last_operate_info_prt(DPP_DEV_T *dev);

/***********************************************************/
/** DTB 各通道状态机信息获取
* @param   dev_id       芯片的id号  
*
* @return  
* @remark  无
* @see     
* @author  cbb      @date  2022/11/11
************************************************************/
ZXIC_UINT32 diag_dpp_dtb_channels_state_info_prt(DPP_DEV_T *dev);

#endif

#ifdef __cplusplus
}
#endif

#endif

