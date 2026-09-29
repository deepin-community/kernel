/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_dtb.h
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : zab
* 完成日期 : 2022/08/26
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/
#ifndef _DPP_DTB_H_
#define _DPP_DTB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dpp_dtb_cfg.h"

#define DPP_DTB_QUEUE_ITEM_NUM_MAX              (32)

#define DPP_DTB_ITEM_ACK_SIZE                   (16)
#define DPP_DTB_ITEM_BUFF_SIZE                  (16 * 1024)
#define DPP_DTB_ITEM_SIZE                       (16 + 16 * 1024)
#define DPP_DTB_TAB_UP_SIZE                     ((16 + 16 * 1024) * 32)
#define DPP_DTB_TAB_DOWN_SIZE                   ((16 + 16 * 1024) * 32)

#define DPP_DTB_TAB_UP_ACK_VLD_MASK             (0x555555)
#define DPP_DTB_TAB_DOWN_ACK_VLD_MASK           (0x5a5a5a)
#define DPP_DTB_TAB_ACK_IS_USING_MASK           (0x11111100)
#define DPP_DTB_TAB_ACK_UNUSED_MASK             (0x0)
#define DPP_DTB_TAB_ACK_SUCCESS_MASK            (0xff)
#define DPP_DTB_TAB_ACK_FAILED_MASK             (0x1)
#define DPP_DTB_TAB_ACK_CHECK_VALUE             (0x12345678)

#define DPP_DTB_TAB_ACK_VLD_SHIFT               (104)
#define DPP_DTB_TAB_ACK_STATUS_SHIFT            (96)

#define DPP_DTB_TAB_UP_PHY_ADDR_GET(SLOT_ID, DEV_ID, QUEUE_ID, INDEX)     \
        (p_dpp_dtb_mgr[SLOT_ID][DEV_ID]->queue_info[QUEUE_ID].tab_up.start_phy_addr + INDEX * p_dpp_dtb_mgr[SLOT_ID][DEV_ID]->queue_info[QUEUE_ID].tab_up.item_size)
#define DPP_DTB_TAB_UP_USER_PHY_ADDR_GET(SLOT_ID, DEV_ID, QUEUE_ID, INDEX)     \
        (p_dpp_dtb_mgr[SLOT_ID][DEV_ID]->queue_info[QUEUE_ID].tab_up.user_addr[INDEX].phy_addr)
#define DPP_DTB_TAB_UP_USER_PHY_ADDR_FLAG_GET(SLOT_ID, DEV_ID, QUEUE_ID, INDEX)     \
        (p_dpp_dtb_mgr[SLOT_ID][DEV_ID]->queue_info[QUEUE_ID].tab_up.user_addr[INDEX].user_flag)
#define DPP_DTB_TAB_UP_USER_ADDR_FLAG_SET(SLOT_ID, DEV_ID, QUEUE_ID, INDEX, VAL)     \
        do {\
            p_dpp_dtb_mgr[SLOT_ID][DEV_ID]->queue_info[QUEUE_ID].tab_up.user_addr[INDEX].user_flag = VAL;\
        }while(0)
#define DPP_DTB_TAB_DOWN_PHY_ADDR_GET(SLOT_ID, DEV_ID, QUEUE_ID, INDEX)   \
        (p_dpp_dtb_mgr[SLOT_ID][DEV_ID]->queue_info[QUEUE_ID].tab_down.start_phy_addr + INDEX * p_dpp_dtb_mgr[SLOT_ID][DEV_ID]->queue_info[QUEUE_ID].tab_down.item_size)
#define DPP_DTB_TAB_UP_VIR_ADDR_GET(SLOT_ID, DEV_ID, QUEUE_ID, INDEX)     \
        (p_dpp_dtb_mgr[SLOT_ID][DEV_ID]->queue_info[QUEUE_ID].tab_up.start_vir_addr + INDEX * p_dpp_dtb_mgr[SLOT_ID][DEV_ID]->queue_info[QUEUE_ID].tab_up.item_size)
#define DPP_DTB_TAB_UP_USER_VIR_ADDR_GET(SLOT_ID, DEV_ID, QUEUE_ID, INDEX)     \
            (p_dpp_dtb_mgr[SLOT_ID][DEV_ID]->queue_info[QUEUE_ID].tab_up.user_addr[INDEX].vir_addr)
#define DPP_DTB_TAB_DOWN_VIR_ADDR_GET(SLOT_ID, DEV_ID, QUEUE_ID, INDEX)   \
        (p_dpp_dtb_mgr[SLOT_ID][DEV_ID]->queue_info[QUEUE_ID].tab_down.start_vir_addr + INDEX * p_dpp_dtb_mgr[SLOT_ID][DEV_ID]->queue_info[QUEUE_ID].tab_down.item_size)
#define DPP_DTB_TAB_UP_WR_INDEX_GET(SLOT_ID, DEV_ID, QUEUE_ID)       \
        (p_dpp_dtb_mgr[SLOT_ID][DEV_ID]->queue_info[QUEUE_ID].tab_up.wr_index)
#define DPP_DTB_TAB_UP_RD_INDEX_GET(SLOT_ID, DEV_ID, QUEUE_ID)       \
        (p_dpp_dtb_mgr[SLOT_ID][DEV_ID]->queue_info[QUEUE_ID].tab_up.rd_index)
#define DPP_DTB_TAB_DOWN_WR_INDEX_GET(SLOT_ID, DEV_ID, QUEUE_ID)       \
        (p_dpp_dtb_mgr[SLOT_ID][DEV_ID]->queue_info[QUEUE_ID].tab_down.wr_index)
#define DPP_DTB_TAB_DOWN_RD_INDEX_GET(SLOT_ID, DEV_ID, QUEUE_ID)       \
        (p_dpp_dtb_mgr[SLOT_ID][DEV_ID]->queue_info[QUEUE_ID].tab_down.rd_index)
#define DPP_DTB_TAB_UP_DATA_LEN_GET(SLOT_ID, DEV_ID, QUEUE_ID, INDEX)       \
        (p_dpp_dtb_mgr[SLOT_ID][DEV_ID]->queue_info[QUEUE_ID].tab_up.data_len[INDEX])
#define DPP_DTB_QUEUE_INIT_FLAG_GET(SLOT_ID, DEV_ID, QUEUE_ID)       \
        (p_dpp_dtb_mgr[SLOT_ID][DEV_ID]->queue_info[QUEUE_ID].init_flag)
#define DPP_DTB_QUEUE_VPORT_GET(SLOT_ID, DEV_ID, QUEUE_ID)       \
        (p_dpp_dtb_mgr[SLOT_ID][DEV_ID]->queue_info[QUEUE_ID].vport)
#define DPP_DTB_QUEUE_TYPE_GET(SLOT_ID, DEV_ID, QUEUE_ID)       \
        (p_dpp_dtb_mgr[SLOT_ID][DEV_ID]->queue_info[QUEUE_ID].func_type)

#define DPP_BDRING_ITEM_SIZE                (16)

#define DPP_UP_MAC_BD_ITEM_NUM              (0XFF)

#define DPP_BD_ITEM_NUM_MAX                 (0XFF)
#define DPP_UP_BD_BUFF_SIZE_MAX             (4 * 1024)

#define DPP_UP_MAC_BD_ITEM_SIZE             ((DPP_UP_MAC_BD_ITEM_NUM + 1) * DPP_BDRING_ITEM_SIZE)
#define DPP_UP_MAC_BUFF_SIZE                (16 * 1024)
#define DPP_UP_MAC_BD_BUFF_SIZE             (DPP_UP_MAC_BD_ITEM_NUM * DPP_UP_MAC_BUFF_SIZE)
#define DPP_UP_MAC_BD_TOTAL_SIZE            (DPP_UP_MAC_BD_ITEM_SIZE + DPP_UP_MAC_BD_BUFF_SIZE)
#define DPP_DMA_BUFF_SIZE                   (4*1024*1024)

#define DPP_DMA_BD_VLD_MSK                  (0x80000000)
#define DPP_DMA_BD_DATA_LEN_MSK             (0x7FF)

#define DPP_EP_ID_MAX                       (16)

#define DPP_DTB_LEN_MIN                     (1)
#define DPP_DTB_DOWN_LEN                    (0x3FF)

#define DPP_DMA_HASH_KEY_OFFSET             (6)
#define DPP_DMA_HASH_ITEM_MAX               (64)
#define DPP_DMA_HASH_KEY_RST                (DPP_DMA_HASH_ITEM_MAX-DPP_DMA_HASH_KEY_OFFSET)

#define DPP_DMA_SENDTYPE_START_BIT          (23)
#define DPP_DMA_SENDTYPE_BIT_NUM            (2)
#define DPP_DMA_VALID_START_BIT             (23)
#define DPP_DMA_VALID_BIT_NUM               (1)
#define DPP_DMA_HASHID_START_BIT            (21)
#define DPP_DMA_HASHID_BIT_NUM              (2)
#define DPP_DMA_TBLID_START_BIT             (16)
#define DPP_DMA_TBLID_BIT_NUM               (2)

#define DPP_DTB_QUEUE_INIT                        (1)
#define DPP_DTB_QUEUE_UNINIT                      (0)

typedef enum dpp_dtb_queue_type_e
{
    DPP_DTB_QUEUE_TYPE_INVALID = 0,    
    DPP_DTB_QUEUE_TYPE_TABLE = 1,
    DPP_DTB_QUEUE_TYPE_STAT = 2, 
    DPP_DTB_QUEUE_TYPE_MAX
}DPP_DTB_QUEUE_TYPE_E;

typedef struct dpp_dtb_queue_cfg_t
{
    ZXIC_ADDR_T up_start_phy_addr;
    ZXIC_ADDR_T up_start_vir_addr;
    ZXIC_ADDR_T down_start_phy_addr;
    ZXIC_ADDR_T down_start_vir_addr;
    
    ZXIC_UINT32 up_item_size;
    ZXIC_UINT32 down_item_size;
}DPP_DTB_QUEUE_CFG_T;

typedef struct dpp_dtb_tab_up_user_addr_t
{
    ZXIC_UINT32 user_flag;
    
    ZXIC_ADDR_T phy_addr;
    ZXIC_ADDR_T vir_addr;
}DPP_DTB_TAB_UP_USER_ADDR_T;

typedef struct dpp_dtb_tab_up_info_t
{
    ZXIC_ADDR_T start_phy_addr;
    ZXIC_ADDR_T start_vir_addr;
    ZXIC_UINT32 item_size;

    ZXIC_UINT32 wr_index;
    ZXIC_UINT32 rd_index;

    ZXIC_UINT32 data_len[DPP_DTB_QUEUE_ITEM_NUM_MAX];
    DPP_DTB_TAB_UP_USER_ADDR_T user_addr[DPP_DTB_QUEUE_ITEM_NUM_MAX];
}DPP_DTB_TAB_UP_INFO_T;

typedef struct dpp_dtb_tab_down_info_t
{
    ZXIC_ADDR_T start_phy_addr;
    ZXIC_ADDR_T start_vir_addr;
    ZXIC_UINT32 item_size;

    ZXIC_UINT32 wr_index;
    ZXIC_UINT32 rd_index;
}DPP_DTB_TAB_DOWN_INFO_T;

typedef struct dpp_dtb_queue_info_t
{
    ZXIC_UINT32 init_flag;
    ZXIC_UINT32 slot_id;
    ZXIC_UINT32 vport;
    ZXIC_UINT32 vector;
    ZXIC_UINT32 func_type;
    
    DPP_DTB_TAB_UP_INFO_T tab_up;
    DPP_DTB_TAB_DOWN_INFO_T tab_down;
}DPP_DTB_QUEUE_INFO_T;

typedef struct dpp_dtb_mgr_t
{  
    DPP_DTB_QUEUE_INFO_T queue_info[DPP_DTB_QUEUE_NUM_MAX];
} DPP_DTB_MGR_T;

typedef enum dpp_dtb_dir_type_e
{
    DPP_DTB_DIR_DOWN_TYPE    = 0, 
    DPP_DTB_DIR_UP_TYPE    = 1,        
    DPP_DTB_DIR_TYPE_MAX,
} DPP_DTB_DIR_TYPE_E;


typedef enum dpp_dtb_tab_up_user_addr_type_e
{
    DPP_DTB_TAB_UP_NOUSER_ADDR_TYPE     = 0, 
    DPP_DTB_TAB_UP_USER_ADDR_TYPE       = 1,        
    DPP_DTB_TAB_UP_USER_ADDR_TYPE_MAX,
} DPP_DTB_TAB_UP_USER_ADDR_TYPE_E;

typedef enum dpp_dma_send_type_e
{
    DMA_LEARN_HASH  = 0,
    DMA_DEL_HASH    = 1,
    DMA_UPDATE_HASH = 2,
    DMA_ADD_HASH    = 3,
    DMA_SEND_TYPE_MAX
}DPP_DMA_SEND_TYPE_E;

/* 单个通道BD表管理结构体 */
typedef struct dpp_dma_bd_t
{
    ZXIC_ADDR_T bd_phy_addr;    /* BD表物理地址 */
    ZXIC_ADDR_T bd_vir_addr;    /* BD表进程空间虚拟地址 */
    ZXIC_ADDR_T buff_phy_addr;  /* BD表指向的BUFF物理地址 */
    ZXIC_ADDR_T buff_vir_addr;  /* BD表指向的BUFF进程空间虚拟地址 */
    ZXIC_UINT32 bd_index;       /* BD表当前使用index */
} DPP_DMA_BD_T;

/* 单个设备BD表管理结构体 */
typedef struct dpp_dma_mgr_t
{
    ZXIC_UINT32 init;
    ZXIC_UINT32 endian_flag;    /* DMA数据通道大小端，0-小端，1-大端 */
    DPP_DMA_BD_T up_mac;
} DPP_DMA_MGR_T;

extern DPP_DTB_MGR_T *p_dpp_dtb_mgr[DPP_PCIE_SLOT_MAX][DPP_DEV_CHANNEL_MAX];
extern ZXIC_UINT32 g_dtb_base_timeout;

/**获取dtb准备完成标记*/
ZXIC_UINT32 dtb_table_function_switch_get(ZXIC_VOID);

/**使能dtb准备完成标记*/
ZXIC_UINT32 dtb_table_function_switch_enable(ZXIC_VOID);

/**去使能dtb准备完成标记*/
ZXIC_UINT32 dtb_table_function_switch_disable(ZXIC_VOID);

/**使能dtb调试函数*/
ZXIC_UINT32 dpp_dtb_debug_fun_enable(ZXIC_VOID);

/**去使能dtb调试函数*/
ZXIC_UINT32 dpp_dtb_debug_fun_disable(ZXIC_VOID);

/**获取dtb调试函数*/
ZXIC_UINT32 dpp_dtb_debug_fun_get(ZXIC_VOID);

/**使能dtb打印函数*/
ZXIC_UINT32 dpp_dtb_prt_enable(ZXIC_VOID);

/**去使能dtb打印函数*/
ZXIC_UINT32 dpp_dtb_prt_disable(ZXIC_VOID);

/**获取dtb打印函数*/
ZXIC_UINT32 dpp_dtb_prt_get(ZXIC_VOID);

ZXIC_UINT32 dpp_dtb_soft_perf_test_set(ZXIC_UINT32 value);

ZXIC_UINT32 dpp_dtb_soft_perf_test_get(ZXIC_VOID);

ZXIC_UINT32 dpp_dtb_hardware_perf_test_set(ZXIC_UINT32 value);

ZXIC_UINT32 dpp_dtb_hardware_perf_test_get(ZXIC_VOID);

ZXIC_UINT32 dpp_dtb_down_table_overtime_set(ZXIC_UINT32 times_s);
ZXIC_UINT32 dpp_dtb_down_table_overtime_get(ZXIC_VOID);

ZXIC_UINT32 dpp_dtb_dump_table_overtime_set(ZXIC_UINT32 times_s);
ZXIC_UINT32 dpp_dtb_dump_table_overtime_get(ZXIC_VOID);

ZXIC_VOID dpp_dtb_overtime_prt(ZXIC_VOID);

ZXIC_UINT32 dpp_dtb_base_timeout_cal(DPP_DEV_T *dev);

#if ZXIC_REAL("MGR")
/***********************************************************/
/** 创建DTB的管理结构
* @param   dev_id       设备号，支持多芯片  
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_mgr_create(ZXIC_UINT32 slot_id, ZXIC_UINT32 dev_id);

/***********************************************************/
/** 注销DTB的管理结构
* @param   dev_id       设备号，支持多芯片   
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_mgr_destory_all(ZXIC_VOID);
ZXIC_UINT32 dpp_dtb_mgr_destory(ZXIC_UINT32 slot_id, ZXIC_UINT32 dev_id);

/***********************************************************/
/** 重置DTB管理结构
* @param   dev_id       设备号，支持多芯片   
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_mgr_reset(ZXIC_UINT32 slot_id, ZXIC_UINT32 dev_id);

/***********************************************************/
/** 获取DMA管理结构
* @param   dev_id               设备号，支持多芯片
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
DPP_DTB_MGR_T *dpp_dtb_mgr_get(ZXIC_UINT32 slot_id, ZXIC_UINT32 dev_id);
#endif

#if ZXIC_REAL("ACK_RW")
/***********************************************************/
/** 读取BD表条目信息
* @param   dev_id       芯片的id号  
* @param   queue_id     队列号,范围0-127
* @param   dir_flag     方向,1-上送表项,0-下发表项    
* @param   index        条目索引,范围0-31 
* @param   pos          一个item里面的4个32位，pos对应的是第几个ZXICP_WORD32，
*                       取值为0，1，2，3      
* @param   p_data       读取的数据,大端格式  
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2019/11/2
************************************************************/
ZXIC_UINT32 dpp_dtb_item_ack_rd(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 dir_flag,
                    ZXIC_UINT32 index, 
                    ZXIC_UINT32 pos,
                    ZXIC_UINT32 *p_data);

/***********************************************************/
/** 向BD表条目指定位置写入值
* @param   dev_id       芯片的id号  
* @param   queue_id     队列号,范围0-127
* @param   dir_flag     方向,1-上送表项,0-下发表项  
* @param   index        条目索引,范围0-31 
* @param   pos          一个item里面的4个32位，pos对应的是第几个ZXICP_WORD32，
*                       取值为0，1，2，3      
* @param   data         读取的数据,大端格式 
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2019/11/2
************************************************************/
ZXIC_UINT32 dpp_dtb_item_ack_wr(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 dir_flag,
                    ZXIC_UINT32 index, 
                    ZXIC_UINT32 pos,
                    ZXIC_UINT32 data);


/***********************************************************/
/** 在dtb初始化中使用，直接读取BD表条目信息
* @param   dev_id       芯片的id号  
* @param   queue_id     队列号,范围0-127
* @param   dir_flag     方向,1-上送表项,0-下发表项    
* @param   index        条目索引,范围0-31 
* @param   pos          一个item里面的4个32位，pos对应的是第几个ZXICP_WORD32，
*                       取值为0，1，2，3      
* @param   p_data       读取的数据,大端格式  
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2019/11/2
************************************************************/
ZXIC_UINT32 dpp_dtb_init_item_ack_rd(DPP_DEV_T *dev,
                                ZXIC_UINT32 queue_id,
                                ZXIC_UINT32 dir_flag,
                                ZXIC_UINT32 index, 
                                ZXIC_UINT32 pos,
                                ZXIC_UINT32 *p_data);

/***********************************************************/
/** 在dtb初始化中使用，向BD表条目指定位置写入值
* @param   dev_id       芯片的id号  
* @param   queue_id     队列号,范围0-127
* @param   dir_flag     方向,1-上送表项,0-下发表项  
* @param   index        条目索引,范围0-31 
* @param   pos          一个item里面的4个32位，pos对应的是第几个ZXICP_WORD32，
*                       取值为0，1，2，3      
* @param   data         读取的数据,大端格式 
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2019/11/2
************************************************************/
ZXIC_UINT32 dpp_dtb_init_item_ack_wr(DPP_DEV_T *dev,
                                ZXIC_UINT32 queue_id,
                                ZXIC_UINT32 dir_flag,
                                ZXIC_UINT32 index, 
                                ZXIC_UINT32 pos,
                                ZXIC_UINT32 data);
#endif

#if ZXIC_REAL("BUFF_RW")
/***********************************************************/
/** 读取dtb条目指向BUFF的数据
* @param   dev_id       芯片的id号  
* @param   queue_id     队列号,范围0-127
* @param   dir_flag     方向,1-上送表项,0-下发表项  
* @param   index        条目索引,范围0-31 
* @param   pos          相对BUFF起始地址的偏移,单位32bit;      
* @param   p_data       读取的数据,大端格式 
* @param   len          读取数据长度,单位32bit; 
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2019/11/2
************************************************************/
ZXIC_UINT32 dpp_dtb_item_buff_rd(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 dir_flag,
                    ZXIC_UINT32 index, 
                    ZXIC_UINT32 pos,
                    ZXIC_UINT32 len,
                    ZXIC_UINT32 *p_data);

/***********************************************************/
/** 向BD表条目指向的BUFF指定位置写入值
* @param   dev_id       芯片的id号  
* @param   queue_id     队列号,范围0-127
* @param   dir_flag     方向,1-上送表项,0-下发表项  
* @param   index        条目索引,范围0-31   
* @param   pos          相对BUFF起始地址的偏移,单位32bit;       
* @param   p_data       读取的数据,大端格式 
* @param   len          写入数据长度,单位32bit; 
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2019/11/2
************************************************************/
ZXIC_UINT32 dpp_dtb_item_buff_wr(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 dir_flag,
                    ZXIC_UINT32 index, 
                    ZXIC_UINT32 pos,
                    ZXIC_UINT32 len,
                    ZXIC_UINT32 *p_data);
#endif

#if ZXIC_REAL("API")
ZXIC_UINT32 dpp_dtb_info_print(DPP_DEV_T *dev, 
                               ZXIC_UINT32 queue_id, 
                               ZXIC_UINT32 item_index,
                               DPP_DTB_QUEUE_ITEM_INFO_T *item_info);

/***********************************************************/
/** 配置下发配置数据信息
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列号，范围0-31 
* @param   int_flag             中断标志，0-无，1-有 
* @param   data_len             数据长度，单位32bit;
* @param   p_data               待下发数据 
* @param   p_item_index         返回使用的条目编号 
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_tab_down_info_set(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 int_flag,
                    ZXIC_UINT32 data_len,
                    ZXIC_UINT32 *p_data,
                    ZXIC_UINT32 *p_item_index);

/***********************************************************/
/** 一个元素down成功状态检查
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列号，范围0-31 
* @param   element_id           条目编号 
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_tab_down_success_status_check(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 element_id);

/***********************************************************/
/** dump队列空闲条目获取
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列号，范围0-31 
* @param   p_item_index         返回使用的条目编号 
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_tab_up_free_item_get(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 *p_item_index);

/***********************************************************/
/** dump队列空闲条目获取------spin锁
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列号，范围0-31 
* @param   p_item_index         返回使用的条目编号 
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_spin_tab_up_free_item_get(DPP_DEV_T *dev,
                                             ZXIC_UINT32 queue_id,
                                             ZXIC_UINT32 *p_item_index);

/***********************************************************/
/** 获取dump指定条目物理地址
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列编号,范围:0-127  
* @param   item_index           条目编号,范围0-31  
* @param   p_phy_haddr          物理地址高32bit  
* @param   p_phy_laddr          物理地址低32bit  
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_tab_up_item_addr_get(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 item_index,
                    ZXIC_UINT32 *p_phy_haddr,
                    ZXIC_UINT32 *p_phy_laddr);

/***********************************************************/
/** 获取指定dump条目一定地址偏移的物理地址
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列编号,范围:0-127  
* @param   item_index           条目编号,范围0-31  
* @param   p_phy_haddr          物理地址高32bit  
* @param   p_phy_laddr          物理地址低32bit  
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_tab_up_item_offset_addr_get(DPP_DEV_T *dev,
                                                ZXIC_UINT32 queue_id,
                                                ZXIC_UINT32 item_index,
                                                ZXIC_UINT32 addr_offset,
                                                ZXIC_UINT32 *p_phy_haddr,
                                                ZXIC_UINT32 *p_phy_laddr);

/***********************************************************/
/** 设置dump指定条目空间地址，用于用户自定义空间传输
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列编号,范围:0-127  
* @param   item_index           条目编号,范围0-31  
* @param   phy_haddr            物理地址高  
* @param   vir_laddr            虚拟地址低  
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_tab_up_item_user_addr_set(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 item_index,
                    ZXIC_ADDR_T phy_addr,
                    ZXIC_ADDR_T vir_addr);                

/***********************************************************/
/** 清除用户dump指定条目空间地址
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列编号,范围:0-127  
* @param   item_index           条目编号,范围0-31   
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_tab_up_item_user_addr_clr(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 item_index);

/***********************************************************/
/** dump配置描述符信息设置
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列号，范围0-31 
* @param   item_index           返回使用的条目编号 
* @param   int_flag             中断标志，0-无，1-有 
* @param   data_len             数据长度，单位32bit;
* @param   desc_len             描述符长度，单位32bit;
* @param   p_desc_data          待下发描述符 
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_tab_up_info_set(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 item_index,
                    ZXIC_UINT32 int_flag,
                    ZXIC_UINT32 data_len,
                    ZXIC_UINT32 desc_len,
                    ZXIC_UINT32 *p_desc_data);

/***********************************************************/
/** 打印队列中指定元素的dump地址、ACK及下表中前768bit的数据
* @param   dev_id       芯片的id号  
* @param   queue_id     队列号，范围0-127    
* @param   element_id   元素号，范围0-31  
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2022/08/30
************************************************************/
ZXIC_UINT32 dpp_dtb_dump_table_element_info_prt(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 element_id);

/***********************************************************/
/** 一个元素dump成功状态检查
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列号，范围0-31 
* @param   element_id           条目编号 
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_tab_up_success_status_check(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 element_id);

/***********************************************************/
/** 获取dump数据
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列号，范围0-31 
* @param   item_index           数据对应的的条目编号 
* @param   data_len             数据长度，单位32bit; 
* @param   p_data               dump数据 
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_tab_up_data_get(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 item_index,
                    ZXIC_UINT32 data_len,
                    ZXIC_UINT32 *p_data);

/***********************************************************/
/** dtb初始化
* @param   dev_id       设备号，支持多芯片   
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2022/08/30
************************************************************/
ZXIC_UINT32 dpp_dtb_init(DPP_DEV_T *dev);

/***********************************************************/
/** dtb队列down初始化
* @param   dev_id           设备号，支持多芯片 
* @param   queue_id         队列号
* @param   p_queue_cfg      队列配置参数，具体见DPP_DTB_QUEUE_CFG_T结构体类型
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_down_init(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    DPP_DTB_QUEUE_CFG_T *p_queue_cfg);

/***********************************************************/
/** dtb队列dump初始化
* @param   dev_id           设备号，支持多芯片 
* @param   queue_id         队列号
* @param   p_queue_cfg      队列配置参数，具体见DPP_DTB_QUEUE_CFG_T结构体类型
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_dump_init(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    DPP_DTB_QUEUE_CFG_T *p_queue_cfg);

/***********************************************************/
/** dtb队列down 空间地址配置
* @param   channelId    dtb通道号 
* @param   phyAddr      down物理地址
* @param   virAddr      down虚拟地址
* @param   size         空间大小
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_down_channel_addr_set(DPP_DEV_T *dev,
                                          ZXIC_UINT32 channelId, 
                                          ZXIC_UINT64 phyAddr, 
                                          ZXIC_UINT64 virAddr, 
                                          ZXIC_UINT32 size);

/***********************************************************/
/** dtb队列dump 空间地址配置
* @param   channelId    dtb通道号 
* @param   phyAddr      dump物理地址
* @param   virAddr      dump虚拟地址
* @param   size         空间大小
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_dump_channel_addr_set(DPP_DEV_T *dev,
                                          ZXIC_UINT32 channelId, 
                                          ZXIC_UINT64 phyAddr, 
                                          ZXIC_UINT64 virAddr, 
                                          ZXIC_UINT32 size);

ZXIC_UINT32 dpp_dtb_queue_init_flag_set(DPP_DEV_T *dev, 
                                        ZXIC_UINT32 queue_id, 
                                        ZXIC_UINT32 flag);

/***********************************************************/
/** 释放队列资源
* @param   dev_id           设备号，支持多芯片  
* @param   queue_id         分配到的队列号;
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_id_free(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id);


/***********************************************************/
/** 检查dev设备的dtb队列是否在工作状态
* @param   dev       设备 
* @return  
* @remark  无
* @see     
* @author  cbb      @date  2026/06/25
************************************************************/
ZXIC_UINT32 dpp_dtb_dev_status_check(DPP_DEV_T *dev);                   

/***********************************************************/
/** 根据vport查找相应的队列号
* @param   dev_id       设备号，支持多芯片   
* @param   vport        vport信息
* @param   p_queue_arr 找到到队列数组
* @param   p_num          找到的队列个数
* @return  
* @remark  无
* @see     
* @author  cbb      @date  2023/09/13
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_id_search_by_vport(DPP_DEV_T *dev,
                                             ZXIC_UINT32 *p_queue_arr,
                                             ZXIC_UINT32 *p_num);

/***********************************************************/
/** 根据vport查找相应的队列号
* @param   dev_id       设备号，支持多芯片
* @param   vport        vport信息
* @param   p_queue_arr 找到到队列数组
* @param   p_num          找到的队列个数
* @return  
* @remark  无
* @see     
* @author  cbb      @date  2023/09/13
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_id_get(DPP_DEV_T *dev, ZXIC_UINT32 *queue);

/***********************************************************/
/** 根据vport和func_type查找相应队列号
* @param   dev_id       设备号，支持多芯片   
* @param   p_stat_queue 统计队列号
* @return  
* @remark  无
* @see     
* @author  cbb      @date  2025/11/21
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_id_get_by_func(DPP_DEV_T *dev, ZXIC_UINT32 func_type, ZXIC_UINT32 *p_stat_queue);

/***********************************************************/
/** 获取当前队列有效标识
* @param   dev          设备
* @param   queue        队列id
* @param   valid_flag   出参 0:当前队列未被使用 1：当前队列已被vport使用
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/11/06
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_valid_flag_get(DPP_DEV_T *dev, ZXIC_UINT32 queue, ZXIC_UINT32 *valid_flag);

/***********************************************************/
/** 获取当前队列初始化标识
* @param   dev          设备
* @param   queue        队列id
* @param   init_flag    出参 0:未初始化 1:已初始化
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/11/06
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_init_flag_get(DPP_DEV_T *dev, ZXIC_UINT32 queue, ZXIC_UINT32 *init_flag);

#endif

#if ZXIC_REAL("DMA")

#endif

#ifdef __cplusplus
}
#endif

#endif

