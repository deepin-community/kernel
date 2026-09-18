/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_stat_api.h
* 文件标识 : stat计数模块对外数据类型定义和接口函数声明
* 内容摘要 : 
* 其它说明 : 
* 当前版本 : 
* 作    者 : xjw
* 完成日期 : 2015/02/09
* DEPARTMENT: ASIC_FPGA_R&D_Dept 
* MANUAL_PERCENT: 100%   
 
* 修改记录1: 
* 修改日期:  
* 版 本 号:  
* 修 改 人:  
* 修改内容:  
***************************************************************/

#ifndef _DPP_STAT_API_H_
#define _DPP_STAT_API_H_


#if ZXIC_REAL("header file")

#include "dpp_dev.h"

#endif

#if ZXIC_REAL("data struct define")
/**  STAT模块TM模式*/
typedef enum stat_tm_mode_e
{
    STAT_TM_MODE_ETM = 0,        /**< @brief 上行TM */
    STAT_TM_MODE_FTM = 1,        /**< @brief 下行TM */
    STAT_TM_MODE_MAX
}STAT_TM_MODE_E;

/** TM统计端口选择 */
typedef enum stat_tm_port_mode_e
{
    STAT_TM_PORT_MODE_0_1 = 0,      /**< @brief 端口0~1 */
    STAT_TM_PORT_MODE_2_3 = 1,      /**< @brief 端口2~3 */
    STAT_TM_PORT_MODE_MAX,
}STAT_TM_PORT_MODE_E;

/**TM统计计数模式选择  */
typedef enum stat_tm_cnt_mode_e
{
    STAT_TM_CNT_MODE_MIX = 0,        /**< @brief 混合模式 */
    STAT_TM_CNT_MODE_INNER = 1,      /**< @brief 内部模式 */
    STAT_TM_CNT_MODE_MAX
}STAT_TM_CNT_MODE_E;

/* TM计数器类型，共支持20种  */
typedef enum tm_stat_type_e {

    TM_STAT_ENQUE_PKT                   = 0,  /**<  @brief 入队包计数*/
    TM_STAT_ENQUE_VALID_PKT             = 1,  /**<  @brief 入队VALID包计数*/      
    TM_STAT_ENQUE_DROP_PKT              = 2,  /**<  @brief 入队丢弃包计数*/      
    TM_STAT_ENQUE_TD_PKT                = 3,  /**<  @brief 入队TD包计数*/      
    TM_STAT_ENQUE_WRED_PKT              = 4,  /**<  @brief 入队WRED包计数*/      
    TM_STAT_ENQUE_DP0_PKT               = 5,  /**<  @brief 入队DP0包计数*/      
    TM_STAT_ENQUE_DP1_PKT               = 6,  /**<  @brief 入队DP1包计数*/     
    TM_STAT_ENQUE_DP2_PKT               = 7,  /**<  @brief 入队DP2包计数*/     
    TM_STAT_ENQUE_DP3_PKT               = 8,  /**<  @brief 入队DP3包计数*/      
    TM_STAT_ENQUE_DP4_PKT               = 9,  /**<  @brief 入队DP4包计数*/      
    TM_STAT_ENQUE_DP5_PKT               = 10, /**<  @brief 入队DP5包计数*/       
    TM_STAT_ENQUE_DP6_PKT               = 11, /**<  @brief 入队DP6包计数*/    
    TM_STAT_ENQUE_DP7_PKT               = 12, /**<  @brief 入队DP7包计数*/     
    TM_STAT_ENQUE_BLOCK_PKT             = 13, /**<  @brief 入队BLOCK包计数*/     
    TM_STAT_ENQUE_DISABLE_PKT           = 14, /**<  @brief 入队DISABLE包计数*/   
    TM_STAT_DEQUE_PKT                   = 15, /**<  @brief 出队包计数*/
    TM_STAT_DEQUE_VALID_PKT             = 16, /**<  @brief 出队VALID包计数*/
    TM_STAT_DEQUE_DISCARD_PKT           = 17, /**<  @brief 出队DISCARD包计数*/
    TM_STAT_DEQUE_CLEAR_PKT             = 18, /**<  @brief 出队CLEAR包计数*/   
    TM_STAT_DEQUE_AGE_PKT               = 19, /**<  @brief 出队AGE包计数*/
    TM_STAT_TYPE_UNEN                   = 20, /**<  @brief 不使能的时候，需配置成这个*/
    TM_STAT_TYPE_MAX
} TM_STAT_TYPE_E;

/** car的监管类型 */
typedef enum stat_car_type_e
{
    STAT_CAR_A_TYPE  = 0,      /**<  @brief A级CAR*/
    STAT_CAR_B_TYPE,         /**<  @brief B级CAR*/
    STAT_CAR_C_TYPE,         /**<  @brief C级CAR*/
    STAT_CAR_MAX_TYPE
}STAT_CAR_TYPE_E;

/** car监管队列配置*/
typedef struct stat_car_queue_cfg_t
{
    ZXIC_UINT32 queue_id;        /**<  @brief 队列号*/
    ZXIC_UINT32 plcr_en;         /**<  @brief CAR使能*/
    ZXIC_UINT32 drop_flag;       /**<  @brief 丢弃标记*/
    ZXIC_VOID * profile_cfg;     /**< @brief carA的包模式时，结构体类型是DPP_STAT_CAR_PKT_PROFILE_CFG_T，其余模式组合时，结构体类型DPP_STAT_CAR_PROFILE_CFG_T*/    
}STAT_CAR_QUEUE_CFG_T;

/**  stat模块计数模式设置*/
typedef struct stat_count_cfg_t
{
    ZXIC_UINT32 rd_mode;       /**< @brief 0:计数器在CPU读的下一拍自动清理,1:不自动清零*/
    ZXIC_UINT32 overflow_mode; /**< @brief 0:计数器达到最大值后，一直保持最大值,1:计数器累积到最高1bit为1，最高1bit始终为1，而其余为继续计数*/
}STAT_COUNT_CFG_T;

/** stat 的smmu1属性 */
typedef struct dpp_stat_smmu1_cfg_t
{
    ZXIC_UINT32 baddr;         /**<  @brief 基地址*/
}DPP_STAT_SMMU1_CFG_T;

/** stat模块公共配置 */
typedef struct dpp_stat_comm_cfg_t
{
    DPP_STAT_SMMU1_CFG_T stat_smmu1_cfg[DPP_DEV_CHANNEL_MAX];   /**<  @brief stat 的smmu1属性*/
    ZXIC_UINT32 is_init[DPP_DEV_CHANNEL_MAX];                        /**<  @brief 初始化选择*/
}DPP_STAT_COMM_CFG_T;

/** stat模块tm统计配置 */
typedef struct dpp_stat_tm_cfg_t
{
    ZXIC_UINT32 tm_en;                          /**<  @brief TM统计使能 */
    ZXIC_UINT32 mov_en;                         /**<  @brief 搬移使能*/
    ZXIC_UINT32 eram_en;                        /**<  @brief 片内计数使能*/
    ZXIC_UINT32 ftm_pkt_en;                     /**<  @brief ftm包计数使能*/
    ZXIC_UINT32 etm_pkt_en;                     /**<  @brief etm包计数使能*/
    ZXIC_UINT32 ftm_port_type[4];               /**<  @brief ftm包计数端口类型选择 参考 TM_STAT_TYPE_E */
    ZXIC_UINT32 etm_port_type[4];               /**<  @brief etm包计数端口类型选择 参考 TM_STAT_TYPE_E */
    ZXIC_UINT32 etm_start_queue_id;             /**<  @brief etm起始队列号*/
    ZXIC_UINT32 etm_queue_depth_mode;           /**<  @brief etm队列深度*/
    DPP_STAT_SMMU1_CFG_T ftm_smmu1_cfg;    /**<  @brief ftm统计的smmu1属性*/
    DPP_STAT_SMMU1_CFG_T etm_smmu1_cfg;    /**<  @brief etm统计的smmu1属性*/
    ZXIC_UINT32 is_init[DPP_DEV_CHANNEL_MAX];   /**<  @brief 初始化选择*/
}DPP_STAT_TM_CFG_T;

/** TM 统计计数信息 */
typedef struct dpp_stat_tm_cnt_t
{
    ZXIC_UINT32 tm_cnt_en;               /**<  @brief tm计数使能*/
    ZXIC_UINT32 tm_mode;                 /**<  @brief TM统计模式:0-ftm, 1-etm*/
    ZXIC_UINT32 tm_flow_id;              /**<  @brief TM统计流号*/
    ZXIC_UINT32 tm_stat_type;            /**<  @brief TM统计端口类型*/
    ZXIC_UINT32 is_tm_byte_en;           /**<  @brief TM统计字节计数使能*/
    ZXIC_UINT32 is_eram_en;              /**<  @brief TM统计片内计数使能*/
    ZXIC_UINT64 tm_cnt;                  /**<  @brief TM计数结果*/
}DPP_STAT_TM_CNT_T;

/** STAT 中断状态  */
typedef struct dpp_stat_brief_int_t
{
    ZXIC_UINT32 etcam_int;               /**< @brief etcam模块中断 */
    ZXIC_UINT32 stat_sch_int;            /**< @brief stat sch剩余部分的中断状态 */
}DPP_STAT_BRIEF_INT_T;

/** STAT fifo中断状态选择  */
typedef struct dpp_stat_sch_intr_t
{
    ZXIC_UINT32 hardware_rsv;            /**< @brief 系统保留，用户无需关心*/
    ZXIC_UINT32 oam0_ord_fifo_int;       /**< @brief oam0保序模块中断*/
    ZXIC_UINT32 oam2_ord_fifo_int;       /**< @brief oam2保序模块中断*/
    ZXIC_UINT32 oam3_ord_fifo_int;       /**< @brief oam3保序模块中断*/
    ZXIC_UINT32 ddr_sch_fifo_int;        /**< @brief ddr调度中断*/
    ZXIC_UINT32 plcr_sch_fifo_int;       /**< @brief plcr调度中断*/
    ZXIC_UINT32 stat_schd_fifo_int;      /**< @brief stat模块key调度中断*/
    ZXIC_UINT32 stat_rschd_fifo_int;     /**< @brief stat模块rsp调度中断*/
}DPP_STAT_SCH_INTR_T;

/*stat 计数类型*/
typedef enum stat_cnt_mode_e
{
    STAT_64_MODE  = 0,         /**<  @brief 64bit位宽模式*/
    STAT_128_MODE = 1,         /**<  @brief 128bit位宽模式*/
    STAT_MAX_MODE,
}STAT_CNT_MODE_E;

/**DPP STAT读清模式选择  */
typedef enum stat_rd_clr_mode_e
{
    STAT_RD_CLR_MODE_UNCLR = 0,  /**<  @brief 不读请*/
    STAT_RD_CLR_MODE_CLR   = 1,  /**<  @brief 读清*/
    STAT_RD_CLR_MODE_MAX,
}STAT_RD_CLR_MODE_E;

/* car 优先级 */
typedef enum dpp_car_priority_e
{
    DPP_CAR_PRI0 = 0,        /**<  @brief CAR优先级0配置*/
    DPP_CAR_PRI1 = 1,        /**<  @brief CAR优先级1配置*/
    DPP_CAR_PRI2 = 2,        /**<  @brief CAR优先级2配置*/
    DPP_CAR_PRI3 = 3,        /**<  @brief CAR优先级3配置*/
    DPP_CAR_PRI4 = 4,        /**<  @brief CAR优先级4配置*/
    DPP_CAR_PRI5 = 5,        /**<  @brief CAR优先级5配置*/
    DPP_CAR_PRI6 = 6,        /**<  @brief CAR优先级6配置*/
    DPP_CAR_PRI7 = 7,        /**<  @brief CAR优先级7配置*/
    DPP_CAR_PRI_MAX 
}DPP_CAR_PRIORITY_E;

/** car 监管模板参数设置的参数 */
typedef struct dpp_stat_car_profile_cfg_t
{
    ZXIC_UINT32 profile_id;                        /**<  @brief car模板号*/
    ZXIC_UINT32 pkt_sign;                          /**<  @brief 包限速选择标志*/
    ZXIC_UINT32 cd;                                /**<  @brief CD算法标志/令牌桶算法标志 0:srtcm 1:trtcm 2:MEF10.1*/
    ZXIC_UINT32 cf;                                /**<  @brief CF溢出耦合标志，0:不溢出，1:溢出*/
    ZXIC_UINT32 cm;                                /**<  @brief CM色盲/色敏标志，0:色盲模式，1:色敏模式 */
    ZXIC_UINT32 cir;                               /**<  @brief C令牌桶添加速率(0~X, X Gbps/64kbps),最小值为64Kbps，步长为64Kbps*/
    ZXIC_UINT32 cbs;                               /**<  @brief C桶桶深(XM),配置范围为0~XMByte-1，步长为1Byte*/
    ZXIC_UINT32 eir;                               /**<  @brief E令牌桶添加速率(0~X, XGbps/64kbps),最小值为64Kbps，步长为64Kbps*/
    ZXIC_UINT32 ebs;                               /**<  @brief E桶桶深(XM),配置范围为0~XMByte-1，步长为1Byte*/
    ZXIC_UINT32 random_disc_e;                     /**<  @brief 仅carB、carC支持 */
    ZXIC_UINT32 random_disc_c;                     /**<  @brief 仅carB、carC支持 */
    ZXIC_UINT32 c_pri[DPP_CAR_PRI_MAX];            /**<  @brief 仅pri 1~7是有效值*/
    ZXIC_UINT32 e_green_pri[DPP_CAR_PRI_MAX];      /**<  @brief 仅pri 1~7是有效值*/
    ZXIC_UINT32 e_yellow_pri[DPP_CAR_PRI_MAX];
}DPP_STAT_CAR_PROFILE_CFG_T;

/* car 独占smmu0的模式 */
typedef enum dpp_car_smmu0_mono_mode_e
{
    CAR_SMMU0_MONO_MODE_NONE = 0,             /**<  @brief CAR不独占smmu0*/
    CAR_SMMU0_MONO_MODE_1    = 1,             /**<  @brief CAR独占1片smmu0*/
    CAR_SMMU0_MONO_MODE_2    = 2,             /**<  @brief CAR独占2片smmu0*/
    CAR_SMMU0_MONO_MODE_MAX
}DPP_CAR_SMMU0_MONO_MODE_E;

/* TM统计的读清模式 */
typedef enum stat_tm_clr_mode_e
{
    STAT_TM_CLR_MODE_UNCLR = 0,         /**<  @brief  TM统计不读请*/
    STAT_TM_CLR_MODE_CLR   = 1,           /**<  @brief  TM统计读请*/
    STAT_TM_CLR_MODE_MAX,
}STAT_TM_CLR_MODE_E;

/** car A 队列设置的参数 */
typedef struct dpp_stat_car_a_queue_cfg_t
{
    ZXIC_UINT32 flow_id;
    ZXIC_UINT32 drop_flag;
    ZXIC_UINT32 plcr_en;
    ZXIC_UINT32 profile_id;
    ZXIC_UINT64 tq;
    ZXIC_UINT32 ted;
    ZXIC_UINT32 tcd;
    ZXIC_UINT32 tei;
    ZXIC_UINT32 tci;
}DPP_STAT_CAR_A_QUEUE_CFG_T;

typedef struct dpp_odma_port_prio_stat
{
    ZXIC_UINT32 queue_tx_cnt;
}DPP_ODMA_PORT_STAT_T;

typedef struct dpp_tm_ets_prio_stat
{
    ZXIC_UINT64 deque_pkt_cnt;
    ZXIC_UINT64 deque_pktB_cnt;
}DPP_TM_ETS_PRIO_STAT_T;

typedef struct dpp_prio_stat_data
{
    DPP_ODMA_PORT_STAT_T odma_stat_data[8];
    DPP_TM_ETS_PRIO_STAT_T tm_ets_stat_data[8];
}DPP_PRIO_STAT_DATA_T;

#endif

#if ZXIC_REAL("macro function define")

#endif


#if ZXIC_REAL("function declaration")
/***********************************************************/
/** stat公共配置初始化
* @param   dev_id   
* @param   p_dpp_stat_comm_cfg   
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/07/14
************************************************************/
DPP_STATUS dpp_stat_comm_init(ZXIC_UINT32 dev_id, DPP_STAT_COMM_CFG_T * p_dpp_stat_comm_cfg);

/***********************************************************/
/** 设置ppu统计 ERAM基地址
* @param   dev_id           设备号 
* @param   ppu_eram_baddr   ppu统计eRam基地址,128bit为单位
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/03/31
************************************************************/
DPP_STATUS dpp_stat_ppu_eram_baddr_set(DPP_DEV_T *dev, ZXIC_UINT32 ppu_eram_baddr);

/***********************************************************/
/** 设置ppu统计片内深度
* @param   dev_id           设备号
* @param   ppu_eram_depth   ppu统计片内深度
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/03/31
************************************************************/
DPP_STATUS dpp_stat_ppu_eram_depth_set(DPP_DEV_T *dev, ZXIC_UINT32 ppu_eram_depth);

/***********************************************************/
/** 设置ppu统计 DDR基地址
* @param   dev_id          设备号 
* @param   ppu_ddr_baddr   ppu统计DDR基地址
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/03/31
************************************************************/
DPP_STATUS dpp_stat_ppu_ddr_baddr_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 ppu_ddr_baddr);

/***********************************************************/
/** TM配置初始化
* @param   dev_id   
* @param   p_stat_tm_cfg   
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/08/03
************************************************************/
DPP_STATUS dpp_stat_tm_init(ZXIC_UINT32 dev_id, DPP_STAT_TM_CFG_T *p_stat_tm_cfg);

/***********************************************************/
/** 配置Etm 统计类型
* @param   dev_id               设备号
* @param   etm_port0_type       统计类型0
* @param   etm_port1_type       统计类型1
* @param   etm_port2_type       统计类型2
* @param   etm_port3_type       统计类型3
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/03/31
************************************************************/
DPP_STATUS dpp_stat_etm_port_type_set(ZXIC_UINT32 dev_id,
                                      ZXIC_UINT32 etm_port0_type,
                                      ZXIC_UINT32 etm_port1_type,
                                      ZXIC_UINT32 etm_port2_type,
                                      ZXIC_UINT32 etm_port3_type);

/***********************************************************/
/** 配置Ftm 统计类型
* @param   dev_id               设备号
* @param   ftm_port0_type       统计类型0
* @param   ftm_port1_type       统计类型1
* @param   ftm_port2_type       统计类型2
* @param   ftm_port3_type       统计类型3
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/03/31
************************************************************/
DPP_STATUS dpp_stat_ftm_port_type_set(ZXIC_UINT32 dev_id,
                                      ZXIC_UINT32 ftm_port0_type,
                                      ZXIC_UINT32 ftm_port1_type,
                                      ZXIC_UINT32 ftm_port2_type,
                                      ZXIC_UINT32 ftm_port3_type);



/***********************************************************/
/** car硬件初始化
* @param   dev_id        设备号
* @param   car_type      car编号
* @param   car_type      car模式，参见STAT_CAR_TYPE_E
* @param   car_mono_mode car独占mono模式
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/04/27
************************************************************/
DPP_STATUS dpp_stat_car_hardware_init(ZXIC_UINT32 dev_id,
                                      ZXIC_UINT32 car_type,
                                      ZXIC_UINT32 car_mono_mode);

/***********************************************************/
/** 配置car的层级模式
* @param   dev_id      
* @param   mode   2 - 三级car, 第一级支持16K
*                 1 - 两级car, 第一级扩展为17K
*                 0 - 一级car, 第一级扩展为21K
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/09/28
************************************************************/
DPP_STATUS dpp_stat_car_en_mode_set(DPP_DEV_T *dev, ZXIC_UINT32 mode);

/***********************************************************/
/** car A 字节限速监管模板设定
* @param   dev_id               设备               car号
* @param   profile_id           监管模板号
* @param   p_cara_profile_cfg   监管模板配置
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/04/05
************************************************************/
DPP_STATUS dpp_stat_cara_profile_cfg_set(DPP_DEV_T *dev,
                                         ZXIC_UINT32 profile_id,
                                         DPP_STAT_CAR_PROFILE_CFG_T* p_cara_profile_cfg);

/***********************************************************/
/** 获取car A的流设置
* @param   dev_id               设备               car编号
* @param   flow_id              流号
* @param   p_cara_queue_cfg     car A流配置信息
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/04/05
************************************************************/
DPP_STATUS dpp_stat_cara_queue_cfg_get(DPP_DEV_T *dev,
                                       ZXIC_UINT32 flow_id,
                                       DPP_STAT_CAR_A_QUEUE_CFG_T* p_cara_queue_cfg);

/***********************************************************/
/** car A的流设置
* @param   dev_id           设备           car编号
* @param   flow_id          流号
* @param   drop_flag        丢弃标志
* @param   plcr_en          监管使能
* @param   profile_id       监管模板号
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/04/05
************************************************************/
DPP_STATUS dpp_stat_cara_queue_cfg_set(DPP_DEV_T *dev,
                                       ZXIC_UINT32 flow_id,
                                       ZXIC_UINT32 drop_flag,
                                       ZXIC_UINT32 plcr_en,
                                       ZXIC_UINT32 profile_id);

/***********************************************************/
/** stat模块常用函数列表
* @param   dev_id   
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see     
* @author  刘硕10181552      @date  2016/02/03
************************************************************/
DPP_STATUS dpp_stat_help(ZXIC_UINT32 dev_id);

/***********************************************************/
/** stat 模块上电初始化
* @param   dev_id   
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2017/07/20
************************************************************/
DPP_STATUS dpp_stat_module_init(ZXIC_UINT32 dev_id);

/***********************************************************/
/** ppu stat读统计结果配置
* @param   dev_id           设备号
* @param   rd_mode          读取位宽模式，参见STAT_CNT_MODE_E，0-64bit，1-128bit
* @param   index            索引，具体位宽参见rd_mode
* @param   p_data           出参，读取的数据
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  xhj      @date  2018/02/01
************************************************************/
DPP_STATUS dpp_stat_ppu_cnt_set(ZXIC_UINT32 dev_id,
                                STAT_CNT_MODE_E rd_mode,
                                ZXIC_UINT32 index,
                                ZXIC_UINT32 *p_data);

/***********************************************************/
/** ppu计数值获取
* @param   dev_id           设备号
* @param   rd_mode          读取位宽模式，参见STAT_CNT_MODE_E，0-64bit，1-128bit
* @param   index            索引，具体位宽参见rd_mode
* @param   clr_mode         读清模式，参见STAT_RD_CLR_MODE_E，0-不读清，1-读清
* @param   p_data           出参，读取的数据
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/07/11
************************************************************/
DPP_STATUS dpp_stat_ppu_cnt_get(DPP_DEV_T *dev,
                                STAT_CNT_MODE_E rd_mode,
                                ZXIC_UINT32 index,
                                ZXIC_UINT32 clr_mode,
                                ZXIC_UINT32 *p_data);

/***********************************************************/
/** TM计数值获取
* @param   dev_id               设备号 
* @param   tm_mode              tm模式，0-ftm，1-etm 参见STAT_TM_MODE_E
* @param   only_pkt_num_en      全包模式使能
* @param   port_mode            端口位置模式，参见STAT_TM_PORT_MODE_E
* @param   cnt_mode             计数模式，0-片内计数，1-混合计数
* @param   clr_mode             读清模式，0-非读清，1-读清
* @param   index                索引值
* @param   p_data               数据
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/07/19
************************************************************/
DPP_STATUS dpp_stat_tm_cnt_get(ZXIC_UINT32 dev_id,
                               ZXIC_UINT32 tm_mode,
                               ZXIC_UINT32 only_pkt_num_en,
                               ZXIC_UINT32 port_mode,
                               ZXIC_UINT32 cnt_mode,
                               ZXIC_UINT32 clr_mode,
                               ZXIC_UINT32 index,
                               ZXIC_UINT32 *p_data);

/***********************************************************/
/** CMMU配置信息、ppu stat 统计计数配置打印
* @param    dev_id
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark   无
* @see
* @author   刘硕10181552      @date  2016/01/20
************************************************************/
DPP_STATUS diag_dpp_stat_ppu_cfg_prt(ZXIC_UINT32 dev_id);

#endif
#endif /*dpp_stat_api.h*/



