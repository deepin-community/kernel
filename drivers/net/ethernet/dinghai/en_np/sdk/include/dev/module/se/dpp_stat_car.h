/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_stat_car.h
* 文件标识 : 
* 内容摘要 : 
* 其它说明 : 
* 当前版本 : 
* 作    者 : ls
* 完成日期 : 2016/04/05
* DEPARTMENT: ASIC_FPGA_R&D_Dept 
* MANUAL_PERCENT: 100%   
 
* 修改记录1: 
* 修改日期:  
* 版 本 号:  
* 修 改 人:  
* 修改内容:  
***************************************************************/
#ifndef _DPP_STAT_CAR_H_
#define _DPP_STAT_CAR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dpp_dev.h"
#include "dpp_stat_api.h"


#define     DPP_CAR_ID_MAX                                  (1)

#define     DPP_CAR_DEBUG_SWITCH                            (0)

#define     DPP_CAR_A_FLOW_ID_MAX                           (0x7fff)  /*car A支持动态配置暂取32K, 后续待修改*/
#define     DPP_CAR_B_FLOW_ID_MAX                           (0xfff)
#define     DPP_CAR_C_FLOW_ID_MAX                           (0x3ff)
#define     DPP_CAR_A_FLOW_ID_NUM                           (0x8000)  /*car A支持动态配置暂取32K, 后续待修改*/
#define     DPP_CAR_B_FLOW_ID_NUM                           (0x1000)
#define     DPP_CAR_C_FLOW_ID_NUM                           (0x400)

#define     DPP_CAR_PROFILE_ID_TOTAL                        ((0x200 + 0x80 + 0x20) * DPP_CAR_ID_MAX)

#define     DPP_CAR_PKT_PROFILE_ID_MAX                      (0x200)  /* 512 */
#define     DPP_CAR_A_PROFILE_ID_MAX                        (0x1ff)
#define     DPP_CAR_B_PROFILE_ID_MAX                        (0x7f)
#define     DPP_CAR_C_PROFILE_ID_MAX                        (0x1f)

#define     DPP_CAR_B_PROFILE_ID_RANDOM_MAX                 (0x1f)
#define     DPP_CAR_C_PROFILE_ID_RANDOM_MAX                 (0x7)
#define     DPP_CAR_RANDOM_OFFSET_VAL                       (7)

#define     DPP_CAR_MAX_CBS_VALUE                           ((1<<27) - 1)
#define     DPP_CAR_MAX_EBS_VALUE                           ((1<<27) - 1)
#define     DPP_CAR_MAX_CIR_VALUE                           ((1<<23) - 1)
#define     DPP_CAR_MAX_EIR_VALUE                           ((1<<23) - 1)
#define     DPP_CAR_MAX_PKT_CIR_VALUE                       ((1<<29) - 1)
#define     DPP_CAR_MAX_PKT_CBS_VALUE                       ((1<<14) - 1)
#define     DPP_CAR_MAX_PRI_VALUE                           ((1<<5) - 1)

#define     DPP_CAR_QUEUQ_CFG_TQ_LEN                        (64)
#define     DPP_CAR_QUEUQ_CFG_TQ_HIGH_13BIT_POS             (44)
#define     DPP_CAR_QUEUQ_CFG_TQ_HIGH_13BIT_LEN             (13)
#define     DPP_CAR_QUEUQ_CFG_TQ_LOW_32BIT_POS              (31)
#define     DPP_CAR_QUEUQ_CFG_TQ_LOW_32BIT_LEN              (32)

#define     DPP_CAR_PROFILE_CFG_ZXIC_UINT8                        (28)
#define     DPP_CAR_PROFILE_CFG_ZXIC_UINT32                      (DPP_CAR_PROFILE_CFG_ZXIC_UINT8>>2)
#define     DPP_CAR_PROFILE_CFG_WIDTH                       (DPP_CAR_PROFILE_CFG_ZXIC_UINT8<<3)

#define     DPP_CAR_QUEUE_CFG_ZXIC_UINT8                          (4)
#define     DPP_CAR_QUEUE_CFG_WIDTH                         (DPP_CAR_QUEUE_CFG_ZXIC_UINT8<<3)

#define     DPP_CAR_PROFILE_CFG_CAR_TYPE_POS                (1)
#define     DPP_CAR_PROFILE_CFG_CAR_TYPE_LEN                (2)
#define     DPP_CAR_PROFILE_CFG_PKT_SIGN_POS                (2)
#define     DPP_CAR_PROFILE_CFG_PKT_SIGN_LEN                (1)
#define     DPP_CAR_PROFILE_CFG_CD_POS                      (4)
#define     DPP_CAR_PROFILE_CFG_CD_LEN                      (2)
#define     DPP_CAR_PROFILE_CFG_CF_POS                      (5)
#define     DPP_CAR_PROFILE_CFG_CF_LEN                      (1)
#define     DPP_CAR_PROFILE_CFG_CM_POS                      (6)
#define     DPP_CAR_PROFILE_CFG_CM_LEN                      (1)
#define     DPP_CAR_PROFILE_CFG_EIR_POS                     (28)
#define     DPP_CAR_PROFILE_CFG_EIR_LEN                     (22)
#define     DPP_CAR_PROFILE_CFG_CIR_POS                     (50)
#define     DPP_CAR_PROFILE_CFG_CIR_LEN                     (22)
#define     DPP_CAR_PROFILE_CFG_EBS_POS                     (77)
#define     DPP_CAR_PROFILE_CFG_EBS_LEN                     (27)
#define     DPP_CAR_PROFILE_CFG_CBS_POS                     (104)
#define     DPP_CAR_PROFILE_CFG_CBS_LEN                     (27)
#define     DPP_CAR_PROFILE_CFG_C_PRI1_POS                  (139)
#define     DPP_CAR_PROFILE_CFG_C_PRI1_LEN                  (5)
#define     DPP_CAR_PROFILE_CFG_E_G_PRI1_POS                (174)
#define     DPP_CAR_PROFILE_CFG_E_G_PRI1_LEN                (5)
#define     DPP_CAR_PROFILE_CFG_E_Y_PRI0_POS                (214)
#define     DPP_CAR_PROFILE_CFG_E_Y_PRI0_LEN                (5)

#define     DPP_CAR_PKT_PROFILE_CFG_CAR_TYPE_POS            (1)
#define     DPP_CAR_PKT_PROFILE_CFG_CAR_TYPE_LEN            (2)
#define     DPP_CAR_PKT_PROFILE_CFG_PKT_SIGN_POS            (2)
#define     DPP_CAR_PKT_PROFILE_CFG_PKT_SIGN_LEN            (1)
#define     DPP_CAR_PKT_PROFILE_CFG_CIR_POS                 (32)
#define     DPP_CAR_PKT_PROFILE_CFG_CIR_LEN                 (30)
#define     DPP_CAR_PKT_PROFILE_CFG_CBS_POS                 (46)
#define     DPP_CAR_PKT_PROFILE_CFG_CBS_LEN                 (14)
#define     DPP_CAR_PKT_PROFILE_CFG_C_PRI0_POS              (86)
#define     DPP_CAR_PKT_PROFILE_CFG_C_PRI0_LEN              (5)

#define     DPP_CAR_QUEUE_CFG_CAR_TYPE_POS                  (1)
#define     DPP_CAR_QUEUE_CFG_CAR_TYPE_LEN                  (2)
#define     DPP_CAR_QUEUE_CFG_PKT_SIGN_POS                  (2)
#define     DPP_CAR_QUEUE_CFG_PKT_SIGN_LEN                  (1)
#define     DPP_CAR_QUEUE_CFG_QUEUE_ID_POS                  (16)
#define     DPP_CAR_QUEUE_CFG_QUEUE_ID_LEN                  (14)

/* car 算法模式 */
typedef enum dpp_car_cd_mode_e
{
    CAR_CD_MODE_SRTCM   = 0,
    CAR_CD_MODE_TRTCM,
    CAR_CD_MODE_MEF10_1,
    CAR_CD_MODE_INVALID
}DPP_CAR_CD_MODE_E;

/* car 读清模式*/
typedef enum dpp_car_rd_mode_e
{
    CAR_READ_NOT_CLEAR = 0,
    CAR_READ_AND_CLEAR = 1,
}DPP_CAR_RD_MODE_E;

/* car 翻转模式 */
typedef enum dpp_car_overflow_mode_e
{
    CAR_KEEP_COUNT   = 0,
    CAR_RE_COUNT     = 1,
}DPP_CAR_OVERFLOW_MODE_E;

/** QVOS翻转模式 */
typedef enum dpp_car_qvos_mode_e
{
    CAR_QVOS_MODE_OVERFLOW_0 = 0,
    CAR_QVOS_MODE_OVERFLOW_1 = 1,
    CAR_QVOS_MODE_OVERFLOW_2 = 2,
    CAR_QVOS_MODE_OVERFLOW_MAX
}DPP_CAR_QVOS_MODE_E;

typedef enum dpp_car_en_mode_e
{
    DPP_CAR_EN_MODE_BOTH_EN =  0,
    DPP_CAR_EN_MODE_A_EN    =  1,
    DPP_CAR_EN_MODE_A_B_EN  =  2,
    DPP_CAR_EN_MODE_INVALID
}DPP_CAR_EN_MODE_E;

typedef enum dpp_car_cfg_operate_mode_e
{
    CAR_OPERATE_MODE_ADD    = 0,
    CAR_OPERATE_MODE_DEL    = 1,
    CAR_OPERATE_MODE_SRH    = 2,
    CAR_OPERATE_MODE_GET    = 3,
    CAR_OPERATE_MODE_MAX,
}DPP_CAR_CFG_OPERATE_MODE_E;

typedef struct dpp_stat_car_dbg_cnt_t
{
    ZXIC_UINT32 pkt_input_total_cnt;
    ZXIC_UINT32 pkt_input_green_cnt;
    ZXIC_UINT32 pkt_input_yellow_cnt;
    ZXIC_UINT32 pkt_input_red_cnt;
    ZXIC_UINT32 pkt_output_total_cnt;
    ZXIC_UINT32 pkt_output_green_cnt;
    ZXIC_UINT32 pkt_output_yellow_cnt;
    ZXIC_UINT32 pkt_output_red_cnt;
    ZXIC_UINT32 pkt_fc_dbg_cnt;
    ZXIC_UINT32 pkt_size_cnt;
}DPP_STAT_CAR_DBG_CNT_T;

/** car 包监管模板参数设置的参数 */
typedef struct dpp_stat_car_pkt_profile_cfg_t
{
    ZXIC_UINT32 profile_id;
    ZXIC_UINT32 pkt_sign;
    ZXIC_UINT32 cir;
    ZXIC_UINT32 cbs;
    ZXIC_UINT32 pri[DPP_CAR_PRI_MAX];              /**< @brief pri 0~7是有效值*/
}DPP_STAT_CAR_PKT_PROFILE_CFG_T;

/** car A包队列设置的参数 */
typedef struct dpp_stat_car_a_pkt_queue_cfg_t
{
    ZXIC_UINT32 flow_id;
    ZXIC_UINT32 drop_flag;
    ZXIC_UINT32 plcr_en;
    ZXIC_UINT32 profile_id;
    ZXIC_UINT64 tq;
    ZXIC_UINT64 dc;
    ZXIC_UINT32 tc;
}DPP_STAT_CAR_A_PKT_QUEUE_CFG_T;

/** car B 队列设置的参数 */
typedef struct dpp_stat_car_b_queue_cfg_t
{
    ZXIC_UINT32 flow_id;
    ZXIC_UINT32 drop_flag;
    ZXIC_UINT32 plcr_en;
    ZXIC_UINT32 profile_id;
    ZXIC_UINT64 tq;
    ZXIC_UINT32 tce_flag;
    ZXIC_UINT32 tce;
    ZXIC_UINT32 tc;
    ZXIC_UINT32 te;
}DPP_STAT_CAR_B_QUEUE_CFG_T;

/** car C 队列设置的参数 */
typedef struct dpp_stat_car_c_queue_cfg_t
{
    ZXIC_UINT32 flow_id;
    ZXIC_UINT32 drop_flag;
    ZXIC_UINT32 plcr_en;
    ZXIC_UINT32 profile_id;
    ZXIC_UINT64 tq;
    ZXIC_UINT32 tce_flag;
    ZXIC_UINT32 tce;
    ZXIC_UINT32 tc;
    ZXIC_UINT32 te;
}DPP_STAT_CAR_C_QUEUE_CFG_T;

/* profile配置键值 */
typedef struct dpp_car_profile_rb_key_t
{
    /**carprofile: 
         car_type[2]+pkt_sign[1]+cd[2]+cf[1]+cm[1]+eir[22]+cir[22]+ebs[27]+cbs[27]+cpri[35]+e_g_pri[35]+e_y_pri[40]+0[9]*/
    /**carpktprofile: 
         car_type[2]+pkt_sign[1]+cir[30]+cbs[14]+c_pri[32]+0[115]*/
    ZXIC_UINT32 profile_cfg[DPP_CAR_PROFILE_CFG_ZXIC_UINT32];
    ZXIC_UINT32 is_static;                                    /* 是否静态，静态的profile必须由用户手动删除,待实现 */
    ZXIC_UINT32 use_count;
}DPP_CAR_PROFILE_RB_KEY_T;

/* profile id与profile 配置节点关系键值 */
typedef struct dpp_car_profile_id_rb_key_t
{
    ZXIC_UINT32 profile_id;
    void * p_car_node;
    ZXIC_UINT32 is_used;
}DPP_CAR_PROFILE_ID_RB_KEY_T;

/* 队列和profile id绑定关系键值 */
typedef struct dpp_car_queue_rb_key_t
{   /** carqueue
          car_type[2]+pkt_sign[1]+queue_id[14]+15[0]*/
    ZXIC_UINT8 profile_cfg[DPP_CAR_QUEUE_CFG_ZXIC_UINT8];   
    ZXIC_UINT32 is_used;
    ZXIC_UINT32 profile_id;
}DPP_CAR_QUEUE_RB_KEY_T;

/* 红黑树管理 */
typedef struct dpp_car_rb_mng_t
{
    ZXIC_UINT32          init_en;
    ZXIC_UINT32          total_num;
    ZXIC_UINT32          key_size;               /** 单个红黑树节点空间的大小 */
    ZXIC_RB_CMPFUN   p_cmpfun;
    ZXIC_RB_CMPFUN   p_id_cmpfun;
    ZXIC_RB_CFG *    p_plcr_rb;
    ZXIC_RB_CFG *    p_plcr_id_rb;           /** 由id反查car的配置 的红黑树结构 */
    ZXIC_LISTSTACK_MANGER *p_liststack_mng;  /** 索引分配 */
}DPP_CAR_RB_MNG_T;

/* profile管理 */
typedef struct dpp_car_profile_mng_t
{
    ZXIC_UINT32    is_init;
    DPP_CAR_RB_MNG_T *p_car_a_rb_profile_mng;
    DPP_CAR_RB_MNG_T *p_car_b_rb_profile_mng;
    DPP_CAR_RB_MNG_T *p_car_c_rb_profile_mng;
}DPP_CAR_PROFILE_MNG_T;

/* 队列管理 */
typedef struct dpp_car_queue_mng_t
{
    ZXIC_UINT32    is_init;
    DPP_CAR_RB_MNG_T *p_car_rb_queue_mng;       /** 不同的car*/
}DPP_CAR_QUEUE_MNG_T;

/* car配置信息 */
typedef struct dpp_car_cfg_t
{
    ZXIC_UINT32 is_init[DPP_DEV_CHANNEL_MAX];
    ZXIC_UINT32 car0_mono_mode[DPP_DEV_CHANNEL_MAX];
    DPP_CAR_QUEUE_MNG_T *     p_car_queue_mng[DPP_DEV_CHANNEL_MAX];
    DPP_CAR_PROFILE_MNG_T * p_car_profile_mng[DPP_DEV_CHANNEL_MAX];
}DPP_CAR_CFG_T;

/* car软复位需要存储的参数 */
typedef struct dpp_car_soft_reset_item_t
{
    ZXIC_UINT32 flow_id;
    ZXIC_UINT32 profile_id;
}DPP_CAR_SOFT_RESET_ITEM_T;

typedef struct dpp_car_soft_reset_data_t
{
    ZXIC_UINT8 car_pkt_sign[DPP_CAR_PKT_PROFILE_ID_MAX];   /*  */

    ZXIC_UINT32 is_init;                                   /* car是否初始化标志位 */
    ZXIC_UINT32 car0_pkt_num;                              /*  */

    ZXIC_UINT32 cara_flow_num;             /* car0已配置过的流ID数目 */
    ZXIC_UINT32 carb_flow_num;             /* car0已配置过的流ID数目 */
    ZXIC_UINT32 carc_flow_num;             /* car0已配置过的流ID数目 */

    DPP_CAR_SOFT_RESET_ITEM_T cara_item[DPP_CAR_A_FLOW_ID_NUM];
    DPP_CAR_SOFT_RESET_ITEM_T carb_item[DPP_CAR_B_FLOW_ID_NUM];
    DPP_CAR_SOFT_RESET_ITEM_T carc_item[DPP_CAR_C_FLOW_ID_NUM];

}DPP_CAR_SOFT_RESET_DATA_T;

typedef struct dpp_car_random_ram_t
{
    ZXIC_UINT32 p1;    /* 第一档丢弃概率(百分比)，取值0-100，推荐为1(百分比) */
    ZXIC_UINT32 p2;    /* 第二档丢弃概率(百分比)，取值0-100，推荐为10(百分比) */
    ZXIC_UINT32 p3;    /* 第三档丢弃概率(百分比)，取值0-100，推荐为50(百分比) */
    ZXIC_UINT32 tc;    /* 桶深，即CBS或EBS */
    ZXIC_UINT32 t1;    /* 桶深的低水线，取值0-CBS/EBS，推荐是70%的桶深 */
    ZXIC_UINT32 t2;    /* 桶深的中水线，取值0-CBS/EBS，推荐是85%的桶深 */
    ZXIC_UINT32 t3;    /* 桶深的高水线，取值0-CBS/EBS，推荐是95%的桶深 */
}DPP_CAR_RANDOM_RAM_T;

typedef struct dpp_car_soft_reset_queue_t
{
    ZXIC_UINT32 car_type;
    ZXIC_UINT32 flow_id;
    ZXIC_UINT32 drop_flag;
    ZXIC_UINT32 plcr_en;
    ZXIC_UINT32 profile_id;

}DPP_CAR_SOFT_RESET_QUEUE_T;

/***********************************************************/
/** car A包限速监管模板设定
* @param   dev_id               设备号               car号
* @param   profile_id           监管模板号
* @param   p_cara_profile_cfg   监管模板配置
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/04/05
************************************************************/
DPP_STATUS dpp_stat_cara_pkt_profile_cfg_set(DPP_DEV_T *dev,
                                             ZXIC_UINT32 profile_id,
                                             DPP_STAT_CAR_PKT_PROFILE_CFG_T* p_cara_profile_cfg);

/***********************************************************/
/** 获取car A 包长监管流配置
* @param   dev_id               设备号               car号
* @param   flow_id              流号
* @param   p_cara_queue_cfg     car A流配置信息
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/04/05
************************************************************/
DPP_STATUS dpp_stat_cara_pkt_queue_cfg_get(DPP_DEV_T *dev,
                                           ZXIC_UINT32 flow_id,
                                           DPP_STAT_CAR_A_PKT_QUEUE_CFG_T* p_cara_queue_cfg);

/***********************************************************/
/** 配置car B 概率丢弃配置参数
* @param   dev_id      
* @param   profile_id   
* @param   p_random_ram   
*
* @return  
* @remark  无
* @see     
* @author  YXH      @date  2019/04/01
************************************************************/
DPP_STATUS dpp_stat_carb_random_ram_set(DPP_DEV_T *dev,
                                        ZXIC_UINT32 profile_id,
                                        DPP_CAR_RANDOM_RAM_T *p_random_ram_e,
                                        DPP_CAR_RANDOM_RAM_T *p_random_ram_c);

/***********************************************************/
/** 获取car B 概率丢弃配置参数
* @param   dev_id      
* @param   profile_id   
* @param   p_random_ram   
*
* @return  
* @remark  无
* @see     
* @author  YXH      @date  2019/04/01
************************************************************/
DPP_STATUS dpp_stat_carb_random_ram_get(DPP_DEV_T *dev,
                                        ZXIC_UINT32 profile_id,
                                        DPP_CAR_RANDOM_RAM_T *p_random_ram_e,
                                        DPP_CAR_RANDOM_RAM_T *p_random_ram_c);

/***********************************************************/
/** 配置car C 概率丢弃配置参数
* @param   dev_id      
* @param   profile_id   
* @param   p_random_ram   
*
* @return  
* @remark  无
* @see     
* @author  YXH      @date  2019/04/01
************************************************************/
DPP_STATUS dpp_stat_carc_random_ram_set(DPP_DEV_T *dev,
                                        ZXIC_UINT32 profile_id,
                                        DPP_CAR_RANDOM_RAM_T *p_random_ram_e,
                                        DPP_CAR_RANDOM_RAM_T *p_random_ram_c);

/***********************************************************/
/** 获取car B 概率丢弃配置参数
* @param   dev_id      
* @param   profile_id   
* @param   p_random_ram   
*
* @return  
* @remark  无
* @see     
* @author  YXH      @date  2019/04/01
************************************************************/
DPP_STATUS dpp_stat_carc_random_ram_get(DPP_DEV_T *dev,
                                        ZXIC_UINT32 profile_id,
                                        DPP_CAR_RANDOM_RAM_T *p_random_ram_e,
                                        DPP_CAR_RANDOM_RAM_T *p_random_ram_c);

/***********************************************************/
/** 获取car的包长偏移
* @param   dev_id
* @param   p_pkt_size_off  
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  wll      @date  2019/06/06
************************************************************/
DPP_STATUS dpp_stat_car_pkt_size_offset_get(DPP_DEV_T *dev,
                                            ZXIC_UINT32 *p_pkt_size_off);

/***********************************************************/
/** 配置cara的最大包长
* @param   dev_id
* @param   max_pkt_size  
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  wll      @date  2019/06/06
************************************************************/
DPP_STATUS dpp_stat_cara_max_pkt_size_set(DPP_DEV_T *dev,
                                          ZXIC_UINT32 max_pkt_size);

/***********************************************************/
/** 获取cara的最大包长
* @param   dev_id
* @param   p_max_pkt_size  
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  wll      @date  2019/06/06
************************************************************/
DPP_STATUS dpp_stat_cara_max_pkt_size_get(DPP_DEV_T *dev,
                                          ZXIC_UINT32 *p_max_pkt_size);

/***********************************************************/
/** 配置carb的最大包长
* @param   dev_id
* @param   max_pkt_size  
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  wll      @date  2019/06/06
************************************************************/
DPP_STATUS dpp_stat_carb_max_pkt_size_set(DPP_DEV_T *dev,
                                          ZXIC_UINT32 max_pkt_size);

/***********************************************************/
/** 获取carb的最大包长
* @param   dev_id
* @param   p_max_pkt_size  
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  wll      @date  2019/06/06
************************************************************/
DPP_STATUS dpp_stat_carb_max_pkt_size_get(DPP_DEV_T *dev,
                                          ZXIC_UINT32 *p_max_pkt_size);

/***********************************************************/
/** 配置carc的最大包长
* @param   dev_id
* @param   max_pkt_size  
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  wll      @date  2019/06/06
************************************************************/
DPP_STATUS dpp_stat_carc_max_pkt_size_set(DPP_DEV_T *dev,
                                          ZXIC_UINT32 max_pkt_size);

/***********************************************************/
/** 获取carc的最大包长
* @param   dev_id
* @param   p_max_pkt_size  
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  wll      @date  2019/06/06
************************************************************/
DPP_STATUS dpp_stat_carc_max_pkt_size_get(DPP_DEV_T *dev,
                                          ZXIC_UINT32 *p_max_pkt_size);

/***********************************************************/
/** 获取最大包长
* @param   dev_id
* @param   car_type
* @param   p_max_pkt_len
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  wll      @date  2019/06/06
************************************************************/
DPP_STATUS dpp_stat_car_max_pkt_size_get(DPP_DEV_T *dev,
                                         ZXIC_UINT32 car_type,
                                         ZXIC_UINT32 *p_max_pkt_len);

/***********************************************************/
/** 配置car的包长偏移
* @param   dev_id
* @param   pkt_size_off  
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  wll      @date  2019/06/06
************************************************************/
DPP_STATUS dpp_stat_car_pkt_size_offset_set(DPP_DEV_T *dev,
                                            ZXIC_UINT32 pkt_size_off);

/***********************************************************/
/** 配置最大包长
* @param   dev_id
* @param   car_type
* @param   max_pkt_size
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  wll      @date  2019/06/06
************************************************************/
DPP_STATUS dpp_stat_car_max_pkt_size_set(DPP_DEV_T *dev,
                                         ZXIC_UINT32 car_type,
                                         ZXIC_UINT32 max_pkt_size);

/***********************************************************/
/** car 模块流配置
* @param   dev_id      
* @param   car_type   
* @param   flow_id   
* @param   drop_flag   
* @param   plcr_en   
* @param   profile_id   
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/05/06
************************************************************/
DPP_STATUS dpp_stat_car_queue_cfg_set(DPP_DEV_T *dev,
                                      ZXIC_UINT32 car_type,
                                      ZXIC_UINT32 flow_id,
                                      ZXIC_UINT32 drop_flag,
                                      ZXIC_UINT32 plcr_en,
                                      ZXIC_UINT32 profile_id);

/***********************************************************/
/** car profile硬件写入
* @param   dev_id      
* @param   car_type   
* @param   pkt_sign   
* @param   profile_id   
* @param   p_car_profile_cfg   
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/05/06
************************************************************/
DPP_STATUS dpp_stat_car_profile_cfg_set(DPP_DEV_T *dev,
                                        ZXIC_UINT32 car_type,
                                        ZXIC_UINT32 pkt_sign,
                                        ZXIC_UINT32 profile_id,
                                        ZXIC_VOID* p_car_profile_cfg);

/***********************************************************/
/** car 队列映射关系配置
* @param   dev_id      设备号      0:上行CAR限速 1:下行CAR限速
* @param   car_type    0:A级car，1:B级car，2:C级car
* @param   flow_id     流号
* @param   map_flow_id 映射后的流号
* @param   map_sp      映射后的优先级
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/09/27
************************************************************/
DPP_STATUS dpp_stat_car_queue_map_set(DPP_DEV_T *dev,
                                      ZXIC_UINT32 car_type,
                                      ZXIC_UINT32 flow_id,
                                      ZXIC_UINT32 map_flow_id,
                                      ZXIC_UINT32 map_sp);

/***********************************************************/
/** 
* @param   dev_id      
* @param   car_type   
* @param   pkt_sign   
* @param   flow_id   
* @param   p_data   
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/09/27
************************************************************/
DPP_STATUS dpp_stat_car_queue_get(DPP_DEV_T *dev,
                                  ZXIC_UINT32 car_type,
                                  ZXIC_UINT32 pkt_sign,
                                  ZXIC_UINT32 flow_id,
                                  ZXIC_VOID* p_data);
/***********************************************************/
/** car 模块流配置获取
* @param   dev_id      
* @param   car_type   
* @param   flow_id   
* @param   p_drop_flag   
* @param   p_plcr_en   
* @param   p_profile_id   
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/08/19
************************************************************/
DPP_STATUS dpp_stat_car_queue_cfg_get(DPP_DEV_T *dev,
                                      ZXIC_UINT32 car_type,
                                      ZXIC_UINT32 flow_id,
                                      ZXIC_UINT32 *p_drop_flag,
                                      ZXIC_UINT32 *p_plcr_en,
                                      ZXIC_UINT32 *p_profile_id);

DPP_STATUS dpp_stat_car_profile_cfg_get(DPP_DEV_T *dev,
                                        ZXIC_UINT32 car_type,
                                        ZXIC_UINT32 pkt_sign,
                                        ZXIC_UINT32 profile_id,
                                        ZXIC_VOID* p_car_profile_cfg);

/***********************************************************/
/** 
* @param   dev_id         设备ID         car ID
* @param   profile_id     模板ID
* @param   p_random_ram_e E桶概率丢弃配置参数
* @param   p_random_ram_c C桶概率丢弃配置参数
*
* @return  
* @remark  无
* @see     
* @author  YXH      @date  2019/04/01
************************************************************/
DPP_STATUS dpp_stat_car_random_ram_set(DPP_DEV_T *dev,
                                       ZXIC_UINT32 car_type,
                                       ZXIC_UINT32 profile_id,
                                       DPP_CAR_RANDOM_RAM_T *p_random_ram_e,
                                       DPP_CAR_RANDOM_RAM_T *p_random_ram_c);

/***********************************************************/
/** 
* @param   dev_id         设备ID         car ID
* @param   profile_id     模板ID
* @param   p_random_ram_e E桶概率丢弃配置参数
* @param   p_random_ram_c C桶概率丢弃配置参数
*
* @return  
* @remark  无
* @see     
* @author  YXH      @date  2019/04/01
************************************************************/
DPP_STATUS dpp_stat_car_random_ram_get(DPP_DEV_T *dev,
                                       ZXIC_UINT32 car_type,
                                       ZXIC_UINT32 profile_id,
                                       DPP_CAR_RANDOM_RAM_T *p_random_ram_e,
                                       DPP_CAR_RANDOM_RAM_T *p_random_ram_c);

/***********************************************************/
/** 获取 car 流号的绑定关系
* @param   dev_id      
* @param   car_type   
* @param   flow_id   
* @param   p_map_flow_id   
* @param   p_map_sp   
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/09/27
************************************************************/
DPP_STATUS dpp_stat_car_queue_map_get(DPP_DEV_T *dev,
                                      ZXIC_UINT32 car_type,
                                      ZXIC_UINT32 flow_id,
                                      ZXIC_UINT32* p_map_flow_id,
                                      ZXIC_UINT32* p_map_sp);

/***********************************************************/
/** car 模块调试计数 获取
* @param   dev_id      
* @param   car_type   
* @param   p_car_dbg_cnt   
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/09/27
************************************************************/
DPP_STATUS dpp_stat_car_dbg_cnt_get(ZXIC_UINT32 dev_id,
                                    ZXIC_UINT32 car_type,
                                    DPP_STAT_CAR_DBG_CNT_T *p_car_dbg_cnt);
/***********************************************************/
/** 
* @param   dev_id      
* @param   car_type   
* @param   overflow_mode   
* @param   rd_mode   
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/09/27
************************************************************/
DPP_STATUS dpp_stat_car_dbg_cnt_mode_set(ZXIC_UINT32 dev_id,
                                         ZXIC_UINT32 car_type,
                                         ZXIC_UINT32 overflow_mode,
                                         ZXIC_UINT32 rd_mode);
/***********************************************************/
/** 
* @param   dev_id      
* @param   car_type   
* @param   p_overflow_mode   
* @param   p_rd_mode   
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/09/27
************************************************************/
DPP_STATUS dpp_stat_car_dbg_cnt_mode_get(ZXIC_UINT32 dev_id,
                                         ZXIC_UINT32 car_type,
                                         ZXIC_UINT32* p_overflow_mode,
                                         ZXIC_UINT32* p_rd_mode);

/***********************************************************/
/** car 模块初始化
* @param   dev_id   
* @param   p_car_cfg   
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/05/05
************************************************************/
DPP_STATUS dpp_stat_car_init(ZXIC_UINT32 dev_id,
                             DPP_CAR_CFG_T *p_car_cfg);


/***********************************************************/
/** STAT CAR复位获取全局变量大小函数
* @param   dev_id   
* @param   p_size   
*
* @return  
* @remark  无
* @see     
* @author  yxh      @date  2018/06/26
************************************************************/
DPP_STATUS dpp_stat_car_glb_size_get(DPP_DEV_T *dev, ZXIC_UINT32* p_size);

/***********************************************************/
/** STAT CAR复位设置全局变量函数
* @param   dev_id           设备号
* @param   size             大小，字节数
* @param   p_data_buff      全局变量数据
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  yxh      @date  2018/06/26
************************************************************/
DPP_STATUS dpp_stat_car_glb_mgr_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 size, ZXIC_UINT8 *p_data_buff);

/***********************************************************/
/** STAT CAR复位获取全局变量函数
* @param   dev_id               设备号
* @param   p_flag               释放使能，1-需要手动free，0-不需要手动free
* @param   p_size               数据大小
* @param   pp_data_buff         全局变量数据
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  yxh      @date  2018/06/26
************************************************************/
DPP_STATUS dpp_stat_car_glb_mgr_get(ZXIC_UINT32 dev_id, 
                                    ZXIC_UINT32 *p_flag,
                                    ZXIC_UINT32 *p_size, 
                                    ZXIC_UINT8 **pp_data_buff);

DPP_STATUS dpp_stat_queue_rb_root_prt(ZXIC_RB_CFG *p_rb_cfg);
DPP_STATUS dpp_stat_car_profile_id_rb_root_prt(ZXIC_UINT32 dev_id, ZXIC_RB_CFG *p_rb_cfg);

/***********************************************************/
/** 配置car A的qvos溢出模式
* @param   dev_id        设备号           car编号
* @param   flow_id          流号
* @param   qvos_mode        溢出模式，参见DPP_CAR_QVOS_MODE_E
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_cara_queue_qvos_set(DPP_DEV_T *dev,
                                        ZXIC_UINT32 flow_id,
                                        ZXIC_UINT32 qvos_mode);

/***********************************************************/
/** 获取car A的qvos溢出模式
* @param   dev_id        设备号        car编号
* @param   flow_id       流号
* @param   p_qvos_mode   qvos溢出模式
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls	   @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_cara_queue_qvos_get(DPP_DEV_T *dev,
                                        ZXIC_UINT32 flow_id,
                                        ZXIC_UINT32 *p_qvos_mode);

/***********************************************************/
/** car A指定队列模式配置，仅用于调试
* @param   dev_id           设备号           car编号
* @param   global_en        全局队列使能，0-不使能，1-使能
* @param   sp_en            优先级队列使能，0-不使能，1-使能
* @param   appoint_sp       指定的优先级
* @param   appoint_queue    指定的队列号
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls       @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_cara_queue_appoint_mode_set(DPP_DEV_T *dev,
                                                ZXIC_UINT32 global_en,
                                                ZXIC_UINT32 sp_en,
                                                ZXIC_UINT32 appoint_sp,
                                                ZXIC_UINT32 appoint_queue);

/***********************************************************/
/** 获取car A指定队列模式的配置
* @param   dev_id           设备号           car编号
* @param   p_global_en      全局队列使能，0-不使能，1-使能
* @param   p_sp_en          优先级队列使能，0-不使能，1-使能
* @param   p_appoint_sp     指定的优先级
* @param   p_appoint_queue  指定的队列号
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls       @date  2016/04/07
************************************************************/
DPP_STATUS dpp_stat_cara_queue_appoint_mode_get(DPP_DEV_T *dev,
                                                ZXIC_UINT32 *p_global_en,
                                                ZXIC_UINT32 *p_sp_en,
                                                ZXIC_UINT32 *p_appoint_sp,
                                                ZXIC_UINT32 *p_appoint_queue);

/***********************************************************/
/** 配置car B的qvos溢出模式
* @param   dev_id           设备号           car编号
* @param   flow_id          流号
* @param   qvos_mode        溢出模式，参见DPP_CAR_QVOS_MODE_E
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_carb_queue_qvos_set(DPP_DEV_T *dev,
                                        ZXIC_UINT32 flow_id,
                                        ZXIC_UINT32 qvos_mode);

/***********************************************************/
/** 获取car B的qvos溢出模式
* @param   dev_id           设备号           car编号
* @param   flow_id          流号
* @param   p_qvos_mode      qvos溢出模式
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls       @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_carb_queue_qvos_get(DPP_DEV_T *dev,
                                        ZXIC_UINT32 flow_id,
                                        ZXIC_UINT32 *p_qvos_mode);

/***********************************************************/
/** car B指定队列模式配置，仅用于调试
* @param   dev_id           设备号           car编号
* @param   global_en        全局队列使能，0-不使能，1-使能
* @param   sp_en            优先级队列使能，0-不使能，1-使能
* @param   appoint_sp       指定的优先级
* @param   appoint_queue    指定的队列号
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls       @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_carb_queue_appoint_mode_set(ZXIC_UINT32 dev_id,
                                                ZXIC_UINT32 global_en,
                                                ZXIC_UINT32 sp_en,
                                                ZXIC_UINT32 appoint_sp,
                                                ZXIC_UINT32 appoint_queue);
/***********************************************************/
/** 获取car B指定队列模式的配置
* @param   dev_id        设备号           car编号
* @param   p_global_en      全局队列使能，0-不使能，1-使能
* @param   p_sp_en          优先级队列使能，0-不使能，1-使能
* @param   p_appoint_sp     指定的优先级
* @param   p_appoint_queue  指定的队列号
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/07
************************************************************/
DPP_STATUS dpp_stat_carb_queue_appoint_mode_get(ZXIC_UINT32 dev_id,
                                                ZXIC_UINT32 *p_global_en,
                                                ZXIC_UINT32 *p_sp_en,
                                                ZXIC_UINT32 *p_appoint_sp,
                                                ZXIC_UINT32 *p_appoint_queue);

/***********************************************************/
/** 配置car C的qvos溢出模式
* @param   dev_id           设备号           car编号
* @param   flow_id          流号
* @param   qvos_mode        溢出模式，参见DPP_CAR_QVOS_MODE_E
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_carc_queue_qvos_set(DPP_DEV_T *dev,
                                        ZXIC_UINT32 flow_id,
                                        ZXIC_UINT32 qvos_mode);

/***********************************************************/
/** 获取car C的qvos溢出模式
* @param   dev_id           设备号           car编号
* @param   flow_id          流号
* @param   p_qvos_mode      qvos溢出模式
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_carc_queue_qvos_get(DPP_DEV_T *dev,
                                        ZXIC_UINT32 flow_id,
                                        ZXIC_UINT32 *p_qvos_mode);

/***********************************************************/
/** car C指定队列模式配置，仅用于调试
* @param   dev_id           设备号           car编号
* @param   global_en        全局队列使能，0-不使能，1-使能
* @param   sp_en            优先级队列使能，0-不使能，1-使能
* @param   appoint_sp       指定的优先级
* @param   appoint_queue    指定的队列号
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_carc_queue_appoint_mode_set(ZXIC_UINT32 dev_id,
                                                ZXIC_UINT32 global_en,
                                                ZXIC_UINT32 sp_en,
                                                ZXIC_UINT32 appoint_sp,
                                                ZXIC_UINT32 appoint_queue);

/***********************************************************/
/** 获取car C指定队列模式的配置
* @param   dev_id           设备号           car编号
* @param   p_global_en      全局队列使能，0-不使能，1-使能
* @param   p_sp_en          优先级队列使能，0-不使能，1-使能
* @param   p_appoint_sp     指定的优先级
* @param   p_appoint_queue  指定的队列号
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/07
************************************************************/
DPP_STATUS dpp_stat_carc_queue_appoint_mode_get(ZXIC_UINT32 dev_id,
                                                ZXIC_UINT32 *p_global_en,
                                                ZXIC_UINT32 *p_sp_en,
                                                ZXIC_UINT32 *p_appoint_sp,
                                                ZXIC_UINT32 *p_appoint_queue);

/***********************************************************/
/**
* @param   dev_id
* @param   p_mode
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/09/28
************************************************************/
DPP_STATUS dpp_stat_car_en_mode_get(DPP_DEV_T *dev,
                                    ZXIC_UINT32 *p_mode);

#ifdef __cplusplus
}
#endif

#endif



