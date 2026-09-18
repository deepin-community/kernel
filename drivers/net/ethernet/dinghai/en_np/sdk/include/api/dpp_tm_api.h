/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_tm_api.h
* 文件标识 : tm模块对外数据类型定义和接口函数声明
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : djf
* 完成日期 : 2015/02/04
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/
#ifndef  _DPP_TM_API_H_
#define  _DPP_TM_API_H_


#if ZXIC_REAL("header file")
#include "dpp_module.h"
#endif

#if ZXIC_REAL("macro")
#define DPP_TM_SA_NUM   (128)
#define DPP_TM_PP_NUM   (64)/**< @brief TM内部逻辑端口数 */
#define DPP_ETM_Q_NUM   (9216)/**< @ETM qmu支持9K物理队列 */
#define DPP_FTM_Q_NUM   (2048)/**< @FTM qmu支持2k队列 */
#define DPP_ETM_CRDT_NUM     (0x47FF)  /*etm 支持的crdt流队列逻辑编号cir+eir=18K */
#define DPP_FTM_CRDT_NUM     (0xFFF)   /*ftm 支持的crdt流队列逻辑编号cir+eir=4K */
#define DPP_ETM_MID_SE_NUM   (6144)/**< @ETM中间级共享0x17FF调度单元 */
#define DPP_FTM_MID_SE_NUM   (512)/**< @FTM 中间级共享0x1FF调度单元 */
#define DPP_ETM_MID_WFQFQ_NUM   (8 * 6144)/**< @brief 中间级共享8 * 6144调度器 */
#define DPP_FTM_MID_WFQFQ_NUM   (8 * 512)/**< @brief 中间级共享8 * 512调度器 */


#define DPP_ETM_WFQSP_OFFSET    (0x4000)  /*etm sp-wfq调度器相对于fq调度器编号偏移*/
#define DPP_ETM_FQ_NUM          (16*1024) /*etm fq调度器个数*/
#define DPP_ETM_WFQSP_NUM       (9*1024)  /*etm sp8和wfq调度器个数*/
#define DPP_ETM_FQSPWFQ_NUM     (25*1024) /*etm fq-sp-wfq调度器总个数*/
#define DPP_FTM_WFQSP_NUM       (1920+64)    /*ftm sp8和wfq调度器个数+64个端口*/
#define DPP_ETM_SCH_DEL_NUM     (0xABFF)  /*etm-crdt要删除的流和调度器编号*/
#define DPP_FTM_SCH_DEL_NUM     (0x177F)  /*ftm-crdt要删除的流和调度器编号*/

#define DPP_TM_INVALID_PORT     (0xFFFF)  /* 定义crdt无效端口号,用于判定crdt挂接状态 */
#define DPP_FTM_DELETED_LINK_ID (0x7FF)   /**定义ftm已被删除的link_id**/
#define DPP_ETM_DELETED_LINK_ID (0x7FFF)  /**定义etm已被删除的link_id**/



/* ftm/etm调度器往端口级挂接link_id偏移 */
#define DPP_FTM_PORT_LINKID_BASE   (0x780)
#define DPP_ETM_PORT_LINKID_BASE   (0x7F80)
/**SHAP模块se_id编号基址(非从0开始)**/
#define DPP_FTM_SHAP_SEID_BASE   (0x1000)
#define DPP_ETM_SHAP_SEID_BASE   (0x4800)



/**< @brief TD */
#define DPP_TM_Q_TD_TH_MAX   (8192)/**< @brief Kbyte */
#define DPP_TM_PP_TD_TH_MAX   (8192)/**< @brief Kbyte */
#define DPP_TM_SYS_TD_TH_MAX   (8192)/**< @brief Kbyte */

#define DPP_TM_Q_AVG_Q_LEN_MAX   (8192)/**< @brief Kbyte */
#define DPP_TM_PP_AVG_Q_LEN_MAX   (8192)/**< @brief Kbyte */
#define DPP_TM_SYS_AVG_Q_LEN_MAX   (8192)/**< @brief Kbyte */

/**< @brief WRED */
#define DPP_TM_Q_WRED_NUM   (32)/**< @brief 队列级WRED组数 */
#define DPP_TM_Q_WRED_TH_MAX   (8192)/**< @brief Kbyte */
#define DPP_TM_Q_WRED_MAX_TH_MAX   (8192)/**< @brief Kbyte */
#define DPP_TM_Q_WRED_MIN_TH_MAX   (8192)/**< @brief Kbyte */
#define DPP_TM_Q_WRED_MAX_CFG_PARA   (0xffffffff)

#define DPP_TM_PP_WRED_NUM   (8)/**< @brief 端口级WRED组数 */
#define DPP_TM_PP_WRED_TH_MAX   (8192)/**< @brief Kbyte */
#define DPP_TM_PP_WRED_MAX_TH_MAX   (8192)/**< @brief Kbyte */
#define DPP_TM_PP_WRED_MIN_TH_MAX   (8192)/**< @brief Kbyte */
#define DPP_TM_PP_WRED_MAX_CFG_PARA   (0xffffffff)

/**< @brief GRED */
#define DPP_TM_SYS_GRED_TH_MAX   (8192)/**< @brief Kbyte */
#define DPP_TM_SYS_GRED_MAX_TH_MAX   (8192)/**< @brief Kbyte */
#define DPP_TM_SYS_GRED_MIN_TH_MAX   (8192)/**< @brief Kbyte */
#define DPP_TM_SYS_GRED_MID_TH_MAX   (8192)/**< @brief Kbyte */

#define DPP_TM_DP_NUM   (8)/**< @brief DP曲线数 */
#define DPP_TM_RED_P_MIN   (1)
#define DPP_TM_RED_P_MAX   (100)
#define DPP_TM_CGAVD_WEIGHT_MAX   (15)
#define DPP_TM_CGAVD_MOVE_PROFILE_NUM   (16)


#define DPP_TM_SCH_WEIGHT_INVALID   (0) /**< @brief WFQ权重，FQ时weight是无效的 */
#define DPP_TM_SCH_WEIGHT_MIN   (1)
#define DPP_TM_SCH_WEIGHT_MAX   (511)
#define DPP_TM_SCH_SP_NUM   (8)

#define DPP_ETM_MID_SHAPE_PROFILE_NUM   (512)/**< ETM中间级整形策略数 */
#define DPP_FTM_MID_SHAPE_PROFILE_NUM   (64)/**< FTM中间级整形策略数 */
#define DPP_ETM_FLOW_SHAPE_PROFILE_NUM   (512)/**< ETM 流级整形策略数 */
#define DPP_FTM_FLOW_SHAPE_PROFILE_NUM   (128)/**< FTM 流级整形策略数 */

#define DPP_TM_SHAPE_CIR_MIN   (0)                  /**< @brief kbps */
#define DPP_TM_SHAPE_CIR_MAX   (800 * 1000 * 1000)   /**< @brief kbps:用户可配最大整形 */
#define DPP_TM_SHAPE_CBS_MIN   (0)                   /**< @brief kbyte */
#define DPP_TM_SHAPE_CBS_MAX   (128 * 1024)             /**< @brief kbyte:用户可配最大桶深 128M */


/**< CBS写入寄存器最大值为0x7FF */
#define DPP_TM_SHAPE_CBS_REG_MIN   (0)                   /**< @brief kbyte */
#define DPP_TM_SHAPE_CBS_REG_MAX   (0x7FF)                /**< @brief kbyte */

#define DPP_TM_SYS_HZ   (1000*1000*1000)  /* 系统主频1000MHz */

#define DPP_TM_TC_NUM (8)

#endif


#if ZXIC_REAL("data struct define")
/**< @brief 内置TM工作模式 */
typedef enum dpp_tm_work_mode_e
{
    DPP_TM_WORK_MODE_TM = 0,
    DPP_TM_WORK_MODE_SA,
    DPP_TM_WORK_MODE_INVALID
} DPP_TM_WORK_MODE_E;

/**< @brief QMU工作模式 */
typedef enum dpp_tm_qmu_work_mode_e
{
    DPP_TM_QMU_WORK_MODE_2M = 0, /**< @brief 2M节点工作模式 */
    DPP_TM_QMU_WORK_MODE_4M, /**< @brief 4M节点工作模式 */
    DPP_TM_QMU_WORK_MODE_INVALID
} DPP_TM_QMU_WORK_MODE_E;

/**< @brief QMU DDR随机模式 */
typedef enum dpp_tm_qmu_ddr_random_mode_e
{
    DPP_TM_QMU_DDR_NOT_RANDOM = 0, /**< @brief DDR不随机模式 */
    DPP_TM_QMU_DDR_RANDOM, /**< @brief DDR随机模式 */
    DPP_TM_QMU_DDR_RANDOM_MODE_INVALID
} DPP_TM_QMU_DDR_RANDOM_MODE_E;

/**< @brief 计数模式寄存器 */
typedef struct dpp_tm_cnt_mode_t
{

    ZXIC_UINT32 fc_count_mode; /**< @brief 计数流控模式 */
    ZXIC_UINT32 count_rd_mode; /**< @brief 计数读模式 */
    ZXIC_UINT32 count_overflow_mode; /**< @brief 计数溢出模式  */
} DPP_TM_CNT_MODE_T;

/**< @brief 中断信息 */
typedef struct dpp_tm_int_t
{

    ZXIC_UINT32 shap_int;
    ZXIC_UINT32 crdt_int;
    ZXIC_UINT32 mmu_int;
    ZXIC_UINT32 qmu_int;
    ZXIC_UINT32 cgavd_int;
    ZXIC_UINT32 olif_int;
    ZXIC_UINT32 cfgmt_int;
} DPP_TM_INT_T;


/**< @brief 拥塞避免层次 */
typedef enum dpp_tm_cgavd_level_e
{
    QUEUE_LEVEL = 0, /**< @brief 流队列级 */
    PP_LEVEL,        /**< @brief 端口级 */
    SYS_LEVEL,       /**< @brief 系统级 */
    SA_LEVEL,        /**< @brief SA队列不可达拥塞避免 */
    INVALID_LEVEL
} DPP_TM_CGAVD_LEVEL_E;

/**< @brief dp选取值 */
typedef enum dpp_tm_cgavd_dp_sel_e
{
    DP_SEL_DP = 0, /**< @brief 选tm头中dp字段作为cgavd的dp */
    DP_SEL_TC,        /**< @brief 选tm头中tc字段作为cgavd的dp */
    DP_SEL_PKT_LEN,       /**< @brief 选tm头中pkt[2:0]字段作为cgavd的dp */
    INVALID_DP
} DPP_TM_CGAVD_DP_SEL_E;

/**< @brief 拥塞避免模式 */
typedef enum dpp_tm_cgavd_method_e
{
    TD_METHOD = 0, /**< @brief TD模式 */
    WRED_GRED_METHOD, /**< @brief 流级和端口级为WRED模式，系统级为GRED模式 */
    INVALID_METHOD
} DPP_TM_CGAVD_METHOD_E;

/**< @brief WRED DP曲线配置参数 */
typedef struct dpp_tm_wred_dp_line_para_t
{
    ZXIC_UINT32 max_th; /**< @brief 平均队列深度上限阈值 */
    ZXIC_UINT32 min_th; /**< @brief 平均队列深度下限阈值 */
    ZXIC_UINT32 max_p; /**< @brief 最大丢弃概率 */
    ZXIC_UINT32 weight; /**< @brief 平均队列深度计算权重 */
    ZXIC_UINT32 q_len_th; /**< @brief 队列深度阈值 */
} DPP_TM_WRED_DP_LINE_PARA_T;

/**< @brief GRED DP曲线配置参数 */
typedef struct dpp_tm_gred_dp_line_para_t
{
    ZXIC_UINT32 max_th; /**< @brief 第2段平均队列深度上限阈值 */
    ZXIC_UINT32 mid_th; /**< @brief 第1段平均队列深度上限阈值 */
    ZXIC_UINT32 min_th; /**< @brief 第1段平均队列深度下限阈值 */
    ZXIC_UINT32 max_p; /**< @brief 最大丢弃概率 */
    ZXIC_UINT32 weight; /**< @brief 平均队列深度计算权重 */
    ZXIC_UINT32 q_len_th; /**< @brief 队列深度阈值 */
} DPP_TM_GRED_DP_LINE_PARA_T;

/**< @brief CRDT调度层次 */
typedef enum dpp_tm_sch_level_e
{
    DPP_TM_SCH_LEVEL_Q = 1,
    DPP_TM_SCH_LEVEL_VC = 2,
    DPP_TM_SCH_LEVEL_VCG = 3,
    DPP_TM_SCH_LEVEL_VP = 4,
    DPP_TM_SCH_LEVEL_PP = 5,
    DPP_TM_SCH_LEVEL_INVALID
} DPP_TM_SCH_LEVEL_E;

/**< @brief SP_ID */
typedef enum dpp_tm_sch_sp_e
{
    DPP_TM_SCH_SP_0 = 0,
    DPP_TM_SCH_SP_1 = 1,
    DPP_TM_SCH_SP_2 = 2,
    DPP_TM_SCH_SP_3 = 3,
    DPP_TM_SCH_SP_4 = 4,
    DPP_TM_SCH_SP_5 = 5,
    DPP_TM_SCH_SP_6 = 6,
    DPP_TM_SCH_SP_7 = 7,
    DPP_TM_SCH_SP_8 = 8,
    DPP_TM_SCH_SP_INVALID
} DPP_TM_SCH_SP_E;

/**< @brief 调度单元挂接参数 */
typedef struct dpp_tm_sch_para_t
{
    ZXIC_UINT32 level_id;                    /**< @brief 当前为中间级时，vc:0 vcg:1 vp:2 para_get已加2供调用*/
    DPP_TM_SCH_LEVEL_E se_last_level;   /**< @brief 挂接到的上级层次 */
    ZXIC_UINT32 se_id;                       /**< @brief 挂接到的上级调度单元ID */
    DPP_TM_SCH_SP_E c_sp_id;            /**< @brief C桶挂接到的SPID,SP0~SP7 */
    DPP_TM_SCH_SP_E e_sp_id;            /**< @brief E桶挂接到的SPID,SP0~SP7 */
    ZXIC_UINT32 sp_relay;                    /**< @brief 队列优先级传递标志,FLOW级时表示FLOW_WORK_MODE */
    ZXIC_UINT32 c_sp_weight;                 /**< @brief C桶挂接到的调度器是WFQ的话，WFQ的权重 */
    ZXIC_UINT32 e_sp_weight;                 /**< @brief E桶挂接到的调度器是WFQ的话，WFQ的权重 */
} DPP_TM_SCH_PARA_T;

/**< @brief 调度器参数 */
typedef struct dpp_tm_wfqfq_t
{
    ZXIC_UINT32 wfqfq_id[8];     /**< @brief 本级调度单元下挂接的8个调度器ID，FQ/WFQ，由ID号区分 */
} DPP_TM_WFQFQ_T;

/**< @brief 流队列挂接参数 */
typedef struct dpp_tm_sch_flow_para_t
{
    ZXIC_UINT32 c_linkid;            /**< @brief c桶要挂接到的上级调度器id */
    ZXIC_UINT32 c_weight;            /**< @brief c桶挂接到上级调度器的权重[1~511] */
    ZXIC_UINT32 c_sp;                /**< @brief c桶挂接到上级调度器的sp优先级,有效值[0-8],共9级，优先级依次降低 */
    ZXIC_UINT32 mode;                /**< @brief 挂接模式：0-单桶 1-双桶。配置单桶时无需关注后续参数，配0即可 */
    ZXIC_UINT32 e_linkid;            /**< @brief e桶要挂接到的上级调度器id */
    ZXIC_UINT32 e_weight;            /**< @brief e桶挂接到上级调度器的权重[1~511] */
    ZXIC_UINT32 e_sp;                /**< @brief e桶挂接到上级调度器的sp优先级，有效值[0-8],共9级，优先级依次降低 */
} DPP_TM_SCH_FLOW_PARA_T;

/**< @brief 调度单元挂接参数:非优先级传递 */
typedef struct dpp_tm_sch_se_para_t
{
    ZXIC_UINT32  se_linkid;             /**< @brief 要挂接到的上级调度器id */
    ZXIC_UINT32  cp_token_en;           /**< @brief 调度器cp双桶使能开关，仅fq8/wfq8支持 */
    ZXIC_UINT32  se_weight;             /**< @brief 挂接到上级调度器的权重[1~511] */
    ZXIC_UINT32  se_sp;                  /**< @brief 挂接到上级调度器的sp优先级,有效值[0-8],共9级，优先级依次降低 */
} DPP_TM_SCH_SE_PARA_T;

/**< @brief 调度单元挂接参数:优先级传递开启 */
typedef struct dpp_tm_sch_se_para_insw_t
{
    ZXIC_UINT32  se_linkid;             /**< @brief  要挂接到的上级调度器id */
    ZXIC_UINT32  cp_token_en;           /**< @brief 调度器cp双桶使能开关，仅fq8/wfq8支持 */
    ZXIC_UINT32  se_sp;                  /**< @brief 挂接到上级调度器的sp优先级,有效值[0-8],共9级，优先级依次降低 */
    ZXIC_UINT32  se_weight[8];             /**< @brief WFQ8中各调度器权重值[1~511]，若是WFQ2/4 只取前面对应值，后面无效 */
} DPP_TM_SCH_SE_PARA_INSW_T;

typedef enum dpp_tm_sch_port_linkid_t
{
    DPP_TM_PP_LINKID_PORT0 = 0x7F80,
    DPP_TM_PP_LINKID_PORT1 = 0x7F81,
    DPP_TM_PP_LINKID_PORT2 = 0x7F82,
    DPP_TM_PP_LINKID_PORT3 = 0x7F83,
    DPP_TM_PP_LINKID_PORT4 = 0x7F84,
    DPP_TM_PP_LINKID_PORT5 = 0x7F85,
    DPP_TM_PP_LINKID_PORT6 = 0x7F86,
    DPP_TM_PP_LINKID_PORT7 = 0x7F87,
    DPP_TM_PP_LINKID_PORT8 = 0x7F88,
    DPP_TM_PP_LINKID_PORT9 = 0x7F89,
    DPP_TM_PP_LINKID_PORT10 = 0x7F8A,
    DPP_TM_PP_LINKID_PORT11 = 0x7F8B,
    DPP_TM_PP_LINKID_PORT12 = 0x7F8C,
    DPP_TM_PP_LINKID_PORT13 = 0x7F8D,
    DPP_TM_PP_LINKID_PORT14 = 0x7F8E,
    DPP_TM_PP_LINKID_PORT15 = 0x7F8F,
    DPP_TM_PP_LINKID_PORT16 = 0x7F90,
    DPP_TM_PP_LINKID_PORT17 = 0x7F91,
    DPP_TM_PP_LINKID_PORT18 = 0x7F92,
    DPP_TM_PP_LINKID_PORT19 = 0x7F93,
    DPP_TM_PP_LINKID_PORT20 = 0x7F94,
    DPP_TM_PP_LINKID_PORT21 = 0x7F95,
    DPP_TM_PP_LINKID_PORT22 = 0x7F96,
    DPP_TM_PP_LINKID_PORT23 = 0x7F97,
    DPP_TM_PP_LINKID_PORT24 = 0x7F98,
    DPP_TM_PP_LINKID_PORT25 = 0x7F99,
    DPP_TM_PP_LINKID_PORT26 = 0x7F9A,
    DPP_TM_PP_LINKID_PORT27 = 0x7F9B,
    DPP_TM_PP_LINKID_PORT28 = 0x7F9C,
    DPP_TM_PP_LINKID_PORT29 = 0x7F9D,
    DPP_TM_PP_LINKID_PORT30 = 0x7F9E,
    DPP_TM_PP_LINKID_PORT31 = 0x7F9F,
    DPP_TM_PP_LINKID_PORT32 = 0x7FA0,
    DPP_TM_PP_LINKID_PORT33 = 0x7FA1,
    DPP_TM_PP_LINKID_PORT34 = 0x7FA2,
    DPP_TM_PP_LINKID_PORT35 = 0x7FA3,
    DPP_TM_PP_LINKID_PORT36 = 0x7FA4,
    DPP_TM_PP_LINKID_PORT37 = 0x7FA5,
    DPP_TM_PP_LINKID_PORT38 = 0x7FA6,
    DPP_TM_PP_LINKID_PORT39 = 0x7FA7,
    DPP_TM_PP_LINKID_PORT40 = 0x7FA8,
    DPP_TM_PP_LINKID_PORT41 = 0x7FA9,
    DPP_TM_PP_LINKID_PORT42 = 0x7FAA,
    DPP_TM_PP_LINKID_PORT43 = 0x7FAB,
    DPP_TM_PP_LINKID_PORT44 = 0x7FAC,
    DPP_TM_PP_LINKID_PORT45 = 0x7FAD,
    DPP_TM_PP_LINKID_PORT46 = 0x7FAE,
    DPP_TM_PP_LINKID_PORT47 = 0x7FAF,
    DPP_TM_PP_LINKID_PORT48 = 0x7FB0,
    DPP_TM_PP_LINKID_PORT49 = 0x7FB1,
    DPP_TM_PP_LINKID_PORT50 = 0x7FB2,
    DPP_TM_PP_LINKID_PORT51 = 0x7FB3,
    DPP_TM_PP_LINKID_PORT52 = 0x7FB4,
    DPP_TM_PP_LINKID_PORT53 = 0x7FB5,
    DPP_TM_PP_LINKID_PORT54 = 0x7FB6,
    DPP_TM_PP_LINKID_PORT55 = 0x7FB7,
    DPP_TM_PP_LINKID_PORT56 = 0x7FB8,
    DPP_TM_PP_LINKID_PORT57 = 0x7FB9,
    DPP_TM_PP_LINKID_PORT58 = 0x7FBA,
    DPP_TM_PP_LINKID_PORT59 = 0x7FBB,
    DPP_TM_PP_LINKID_PORT60 = 0x7FBC,
    DPP_TM_PP_LINKID_PORT61 = 0x7FBD,
    DPP_TM_PP_LINKID_PORT62 = 0x7FBE,
    DPP_TM_PP_LINKID_PORT63 = 0x7FBF,
    DPP_TM_PP_LINKID_INVALID
} DPP_TM_SCH_PORT_LINKID_T;
typedef enum dpp_tm_shape_flag_e
{
    DPP_TM_SHAPE_FLAG_CIR = 0,
    DPP_TM_SHAPE_FLAG_EIR = 1,
    DPP_TM_SHAPE_FLAG_INVALID
} DPP_TM_SHAPE_FLAG_E;

/**< @brief 整形令牌桶模式 */
typedef enum dpp_tm_shape_mode_e
{
    DPP_TM_SINGLE_MIX_BUCKET = 0,       /**< @brief 单桶/组合流 */
    DPP_TM_DUAL_BUCKET = 1,       /**< @brief 双桶 */
    DPP_TM_DUAL_PIPE = 2,      /**< @brief 被双桶 */
    DPP_TM_SHAPE_MODE_INVALID
} DPP_TM_SHAPE_MODE_E;

/**< @brief 整形profile参数 */
typedef struct dpp_tm_shape_profile_t
{
    ZXIC_UINT32 cir;
    ZXIC_UINT32 cbs;
    ZXIC_UINT32 eir;
    ZXIC_UINT32 ebs;
} DPP_TM_SHAPE_PROFILE_T;

/**< @brief 端口级整形参数 */
typedef struct dpp_tm_shape_pp_para_t
{
    ZXIC_UINT32 cir;
    ZXIC_UINT32 cbs;
    ZXIC_UINT32 c_en;
} DPP_TM_SHAPE_PP_PARA_T;

/**< @brief 调度单元整形参数 */
typedef struct dpp_tm_shape_para_t
{
    ZXIC_UINT32 class_id;      /**< @brief 当前中间级所属层次，vc:1 vcg:2 vp:3 */
    ZXIC_UINT32 profile_id;
    ZXIC_UINT32 c_en;
    ZXIC_UINT32 e_en;
    DPP_TM_SHAPE_MODE_E mode;
} DPP_TM_SHAPE_PARA_T;

/**< @brief 整形参数 */
typedef struct dpp_tm_shape_t
{
    ZXIC_UINT32 mid_level;      /**< @brief 当前中间级所属层次，vc:1 vcg:2 vp:3 */
    ZXIC_UINT32 cir;
    ZXIC_UINT32 cbs;
    ZXIC_UINT32 eir;
    ZXIC_UINT32 ebs;
    ZXIC_UINT32 c_en;
    ZXIC_UINT32 e_en;
    ZXIC_UINT32 mode;
} DPP_TM_SHAPE_T;

typedef struct dpp_tm_shape_para
{
    ZXIC_UINT32 shape_cir;
    ZXIC_UINT32 shape_cbs;
    ZXIC_UINT32 shape_num;
} DPP_TM_SHAPE_PARA_TABLE;


/**< @brief TM初始化参数 */
typedef struct dpp_tm_init_para_t
{
    DPP_TM_WORK_MODE_E tm_sa_mode;     /**< @brief TM或SA模式 */
    DPP_TM_QMU_WORK_MODE_E qmu_mode;   /**< @brief QMU 2M或4M节点模式 */
    ZXIC_UINT32 case_num;                   /**< @brief 四组QMU初始化场景编号为1-4;ddr*bank:1:4x2;2:4x4;3:8x2;4:4x8.*/
    DPP_TM_QMU_DDR_RANDOM_MODE_E ddr_random_mode;   /**< @brief ddr随机模式 */
    ZXIC_UINT32 block_size;                              /**< @brief block模式，128/256/512/1024 */
    ZXIC_UINT32 local_sa_id;                             /**< @brief SA模式时，本地sa_id*/
} DPP_TM_INIT_PARA_T;


/* TM ASIC初始化信息配置 */
typedef struct dpp_tm_asic_init_info_t
{
    ZXIC_UINT32 blk_size;       /**< @brief qmu配置的block大小 256B/512B[default]/1024B */
    ZXIC_UINT32 case_num;       /**< @brief 四组QMU初始化场景编号为1-4;ddr*bank:1:4x2;2:4x4;3:8x2;4:4x8. */
    ZXIC_UINT32 imem_omem;      /**< @brief 0:片内外混合; 1:纯片内;2:纯片外 */
    ZXIC_UINT32 mode;           /**< @brief TM工作模式 0:TM模式; 1:SA模式 */
} DPP_TM_ASIC_INIT_INFO_T;

#endif

#if ZXIC_REAL("function declaration")



/***********************************************************/
/** 读取block长度模式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_size   block长度模式，256/512/1024
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_tm_cfgmt_blk_size_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_size);


/***********************************************************/
/** 配置内置TM的工作模式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   mode   配置的值，0-TM模式，1-SA模式
*ETM仅工作在TM模式，FTM可以工作TM或SA模式
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/25
************************************************************/
DPP_STATUS dpp_tm_cfgmt_sa_work_mode_set(DPP_DEV_T *dev, DPP_TM_WORK_MODE_E mode);


/***********************************************************/
/**  配置各级搬移功能使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   要配置的拥塞避免层次号，0:队列级，1:端口级，2:系统级
* @param   en   使能标记，0-不使能，1-使能
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2016/10/18
************************************************************/
#ifdef ETM_REAL
DPP_STATUS dpp_tm_cgavd_move_en_set(ZXIC_UINT32 dev_id,
                                    DPP_TM_CGAVD_LEVEL_E level,
                                    ZXIC_UINT32 en);

/***********************************************************/
/** 配置各级搬移门限
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   id   队列号或端口号，系统级时，id参数无效
* @param   value   端口级和系统级时，为搬移门限值，单位为NPPU存包的单位，256B；
                   流级时为搬移profile_id,0~15
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2016/10/18
************************************************************/
DPP_STATUS dpp_tm_cgavd_move_th_set(ZXIC_UINT32 dev_id,
                                    DPP_TM_CGAVD_LEVEL_E level,
                                    ZXIC_UINT32 id,
                                    ZXIC_UINT32 value);

/***********************************************************/
/**  配置flow级的搬移策略
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   move_profile  flow级的搬移门限分组索引,0~15
* @param   th  flow级的搬移门限，单位为KB；
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2016/10/18
************************************************************/
DPP_STATUS dpp_tm_cgavd_flow_move_profile_set(ZXIC_UINT32 dev_id,
                                              ZXIC_UINT32 move_profile,
                                              ZXIC_UINT32 th);
#endif

/***********************************************************/
/**  配置端口共享的搬移门限
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   th  端口共享的搬移门限，单位为NPPU存包的单位，256B；
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2016/10/18
************************************************************/
DPP_STATUS dpp_tm_cgavd_port_share_th_set(ZXIC_UINT32 dev_id,
                                           ZXIC_UINT32 th);

/***********************************************************/
/**  配置各级拥塞避免功能使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   要配置的拥塞避免层次号，0:队列级，1:端口级，2:系统级
* @param   en   使能标记，0-不使能，1-使能
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/28
************************************************************/
DPP_STATUS dpp_tm_cgavd_en_set(DPP_DEV_T *dev,
                               DPP_TM_CGAVD_LEVEL_E level,
                               ZXIC_UINT32 en);

/***********************************************************/
/** 配置拥塞避免算法
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   id   队列号或端口号，系统级时，id参数无效
* @param   method  配置的拥塞避免算法，0:TD，1:WRED/GRED
*          配置TD算法时，先配TD阈值，再配置TD算法
*          配置WRED算法时,先配置流级或端口级的平均队列深度，再配置WRED算法
*          配置GRED算法时，先配置系统级的平均队列深度，在配置成GRED算法
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark
* @see
* @author  taq      @date  2015/04/14
************************************************************/
DPP_STATUS dpp_tm_cgavd_method_set(DPP_DEV_T *dev,
                                   DPP_TM_CGAVD_LEVEL_E level,
                                   ZXIC_UINT32 id,
                                   DPP_TM_CGAVD_METHOD_E method);

/***********************************************************/
/** 配置TD拥塞避免模式下的丢弃门限值
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   id   队列号或端口号，系统级时，id参数无效
* @param   td_th   配置的丢弃门限值，用户配置门限值单位为Kbyte，需要转化为Block单位写入寄存器
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_td_th_set(DPP_DEV_T *dev,
                                  DPP_TM_CGAVD_LEVEL_E level,
                                  ZXIC_UINT32 id,
                                  ZXIC_UINT32 td_th);

/***********************************************************/
/** 配置指定端口或队列绑定的WRED GROUP ID
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   WRED支持层次号，0:队列级，1:端口级
* @param   id   队列号或端口号
* @param   wred_id   配置的WRED GROUP ID
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_wred_id_set(ZXIC_UINT32 dev_id,
                                    DPP_TM_CGAVD_LEVEL_E level,
                                    ZXIC_UINT32 id,
                                    ZXIC_UINT32 wred_id);

/***********************************************************/
/** 配置TM模式下流队列挂接的端口号；SA模式下流队列映射的目的芯片ID
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   q_id   队列号
* @param   pp_id   配置的端口号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/18
************************************************************/
DPP_STATUS dpp_tm_cgavd_q_map_pp_set(DPP_DEV_T *dev,
                                     ZXIC_UINT32 q_id,
                                     ZXIC_UINT32 pp_id);

/***********************************************************/
/** 配置TM模式tc到flow的映射
* @param   dev_id 设备编号
* @param   tc_id   itmd tc优先级（0~7）
* @param   flow_id 映射的flowid号 （0~4095）
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  sun      @date  2023/07/04
************************************************************/
DPP_STATUS dpp_tm_tc_map_flow_set(DPP_DEV_T *dev,
                                  ZXIC_UINT32 tc_id,
                                  ZXIC_UINT32 flow_id);                                     

/***********************************************************/
/** 配置指定端口或队列是否支持动态门限机制
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   WRED支持层次号，0:队列级，1:端口级
* @param   id   队列号或端口号
* @param   en   配置的值，0-不支持动态门限机制，1-支持动态门限机制
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_dyn_th_en_set(DPP_DEV_T *dev,
                                      DPP_TM_CGAVD_LEVEL_E level,
                                      ZXIC_UINT32 id,
                                      ZXIC_UINT32 en);

/***********************************************************/
/**  配置强制片内或片外
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   en      1:使能
* @param   mode   1 :omem 强制片外  0:imem 强制片内
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2017/10/14
************************************************************/
DPP_STATUS dpp_tm_cgavd_imem_omem_set(ZXIC_UINT32 dev_id,
                                      ZXIC_UINT32 en,
                                      ZXIC_UINT32 mode);


/***********************************************************/
/** 读取QMU队列授权价值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_credit_value   授权价值，默认值是400Byte
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy     @date  2016/04/14
************************************************************/
DPP_STATUS dpp_tm_qmu_credit_value_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_credit_value);

/******************************************************************************
*包老化配置
* @param: dev_id: 设备索引编号
* @param   tm_type   0-ETM,1-FTM
*           aging_en: 包老化使能：1表示包老化功能使能；0表示包老化功能关闭。
*           aging_interval: 普通老化两次的间隔配置
*           aging_step_interval: 普通老化的老化时间的步进配置值
*           aging_start_qnum: 老化起始队列
*           aging_end_qnum: 老化结束队列
*           aging_req_aful_th: 普通老化FIFO的将满阈值
*           aging_pkt_num: 一次老化的包个数
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/05/10
************************************************************/
DPP_STATUS dpp_tm_qmu_pkt_aging_set(DPP_DEV_T *dev,
                                    ZXIC_UINT32 aging_en,
                                    ZXIC_UINT32 aging_interval,
                                    ZXIC_UINT32 aging_step_interval,
                                    ZXIC_UINT32 aging_start_qnum,
                                    ZXIC_UINT32 aging_end_qnum,
                                    ZXIC_UINT32 aging_pkt_num,
                                    ZXIC_UINT32 aging_req_aful_th);

/******************************************************************************
*配置老化一个包的时间，一次老化一个包，老化队列范围为可配
* @param: dev_id: 设备索引编号
* @param    tm_type   0: etm; 1: ftm;
*           aging_en: 包老化使能：1表示包老化功能使能；0表示包老化功能关闭。
*           aging_time: 老化一个包的时间，单位ms
            aging_que_start:老化起始队列
            aging_que_start:老化终止队列
老化时间=2*aging_interval*step_interval*q_num
aging_interval = (aging_time * 600000) / (2 * 1 * DPP_TM_Q_NUM);
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb     @date  2020/12/08
************************************************************/
DPP_STATUS dpp_tm_qmu_pkt_age_time_set(DPP_DEV_T *dev,
                                       ZXIC_UINT32 aging_en,
                                       ZXIC_UINT32 aging_time,
                                       ZXIC_UINT32 aging_que_start,
                                       ZXIC_UINT32 aging_que_end);


/***********************************************************/
/** 分配etm调度器资源：fq/fq2/fq4/fq8 个数，(共16K= 16384)
* @param   dev_id   设备编号
* @param   fq_num   FQ调度器个数，须是8的倍数
* @param   fq2_num  FQ2调度器个数，须是4的倍数
* @param   fq4_num  FQ4调度器个数，须是2的倍数
* @param   fq8_num  FQ8调度器个数
*          调度器总数不能超过：16K= 16384
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/26
************************************************************/
DPP_STATUS dpp_etm_crdt_fq_set(DPP_DEV_T *dev,
                               ZXIC_UINT32 fq_num,
                               ZXIC_UINT32 fq2_num,
                               ZXIC_UINT32 fq4_num,
                               ZXIC_UINT32 fq8_num);

/***********************************************************/
/** 分配TM调度器资源：sp/wfq/wfq2/wfq4/wfq8 个数，（etm共9K=9216,ftm共1920个）
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   sp_num   SP调度器个数，须是8的倍数
* @param   wfq_num  WFQ调度器个数，须是8的倍数
* @param   wfq2_num  WFQ2调度器个数，须是4的倍数
* @param   wfq4_num  WFQ4调度器个数，须是2的倍数
* @param   wfq8_num  WFQ8调度器个数
*          调度器总数不能超过：ETM= 9216; FTM= 1920
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/26
************************************************************/
DPP_STATUS dpp_tm_crdt_wfqsp_set(DPP_DEV_T *dev,
                                 ZXIC_UINT32 sp_num,
                                 ZXIC_UINT32 wfq_num,
                                 ZXIC_UINT32 wfq2_num,
                                 ZXIC_UINT32 wfq4_num,
                                 ZXIC_UINT32 wfq8_num);

/***********************************************************/
/** 配置flow级流队列的挂接关系(flow到上级调度器的挂接)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   flow_id   流队列号
* @param   c_linkid  c桶要挂接到的上级调度器id
* @param   c_weight  c桶挂接到上级调度器的权重[1~511]
* @param   c_sp      c桶挂接到上级调度器的sp优先级,有效值[0-8],共9级，优先级依次降低
* @param   mode      挂接模式：0-单桶 1-双桶。配置单桶时无需关注后续参数，配0即可
* @param   e_linkid  e桶要挂接到的上级调度器id
* @param   e_weight  e桶挂接到上级调度器的权重[1~511]
* @param   e_sp      e桶挂接到上级调度器的sp优先级，有效值[0-8],共9级，优先级依次降低
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_flow_link_set(DPP_DEV_T *dev,
                                     ZXIC_UINT32 flow_id,
                                     ZXIC_UINT32 c_linkid,
                                     ZXIC_UINT32 c_weight,
                                     ZXIC_UINT32 c_sp,
                                     ZXIC_UINT32 mode,
                                     ZXIC_UINT32 e_linkid,
                                     ZXIC_UINT32 e_weight,
                                     ZXIC_UINT32 e_sp);

/***********************************************************/
/** 配置调度器层次化QOS的挂接关系:非优先级传递
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   se_id      本级调度器id
* @param   se_linkid  要挂接到的上级调度器id
* @param   se_weight  挂接到上级调度器的权重[1~511]
* @param   se_sp      挂接到上级调度器的sp优先级,有效值[0-8],共9级，优先级依次降低
* @param   se_insw    优先级传递使能：0-关 1-开. 该参数不传递直接配0
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_se_link_set(DPP_DEV_T *dev,
                                   ZXIC_UINT32 se_id,
                                   ZXIC_UINT32 se_linkid,
                                   ZXIC_UINT32 se_weight,
                                   ZXIC_UINT32 se_sp);

/***********************************************************/
/** 配置调度器层次化QOS的挂接关系:优先级传递
* @param   dev_id       设备编号
* @param   tm_type      0-ETM,1-FTM
* @param   se_id        本级调度器id
* @param   se_linkid    要挂接到的上级调度器id
* @param   se_weight    WFQ2/4/8中各调度器权重值[1~511]，取相等的值
* @param   se_sp        挂接到上级调度器的sp优先级,有效值[0-7],共8级，优先级依次降低
* @param   se_insw      优先级传递使能：0-关 1-开. 该参数不传递直接配1
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_se_link_insw_set(DPP_DEV_T *dev,
                                        ZXIC_UINT32 se_id,
                                        ZXIC_UINT32 se_linkid,
                                        ZXIC_UINT32 se_weight,
                                        ZXIC_UINT32 se_sp);

/***********************************************************/
/** 配置调度器层次化QOS的挂接关系:优先级传递,单个调度器挂接
* @param   dev_id       设备编号
* @param   tm_type      0-ETM,1-FTM
* @param   se_id        本级调度器id
* @param   se_linkid    要挂接到的上级调度器id
* @param   se_weight    WFQ8中对应调度器权重值[1~511]
* @param   se_sp        挂接到上级调度器的sp优先级,有效值[0-7],共8级，优先级依次降低
* @param   se_insw      优先级传递使能：0-关 1-开. 该参数不传递直接配1
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_se_link_insw_single_set(DPP_DEV_T *dev,
                                               ZXIC_UINT32 se_id,
                                               ZXIC_UINT32 se_linkid,
                                               ZXIC_UINT32 se_weight,
                                               ZXIC_UINT32 se_sp);

/***********************************************************/
/** 删除流挂接关系
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   id_s      要删除的流号或调度器起始id
* @param   id_e      要删除的流号或调度器终止id
*             ETM范围:0--0x47FF; FTM范围:0-0xFFF
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_del_flow_link_set(DPP_DEV_T *dev, ZXIC_UINT32 id_s, ZXIC_UINT32 id_e);

/***********************************************************/
/** 删除调度器挂接关系(调度器编号从0开始)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   se_id_s   要删除的起始调度器id
* @param   se_id_e   要删除的终止调度器id
*          ETM范围:0--0x63FF; FTM范围:0-0x77F
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_del_se_link_set(DPP_DEV_T *dev, ZXIC_UINT32 id_s, ZXIC_UINT32 id_e);

/***********************************************************/
/** 配置se->pp->dev挂接关系
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   se_id   往端口挂接的调度器id
* @param   pp_id   [0-63]
* @param   weight  [1-511]
* @param   sp_mapping   0~7
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/3/4
************************************************************/
DPP_STATUS dpp_tm_crdt_se_pp_link_set(DPP_DEV_T *dev,
                                      ZXIC_UINT32 se_id,
                                      ZXIC_UINT32 pp_id,
                                      ZXIC_UINT32 weight,
                                      ZXIC_UINT32 sp_mapping);

/***********************************************************/
/**
* @param   dev_id   设备编号
* @param   tm_type  0-ETM,1-FTM
* @param   que_id   queue id
* @param   en       1:过滤E桶队列CRS状态为SLOW的入链请求；0:E桶队列CRS SLOW正常入链；
*
* @return
* @remark  无
* @see
* @author  XXX      @date  2019/05/08
************************************************************/
DPP_STATUS dpp_tm_crdt_eir_crs_filter_en_set(DPP_DEV_T *dev, ZXIC_UINT32 que_id, ZXIC_UINT32 en);


/***********************************************************/
/** 清除整形表格里面的值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/16
************************************************************/
DPP_STATUS dpp_tm_clr_shape_para(DPP_DEV_T *dev);

/***********************************************************/
/** 配置流队列双桶整形使能及模式
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   db_en     双桶整形使能
* @param   mode      0:c+e模式，1:c+p模式
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_flow_db_en_set(DPP_DEV_T *dev, ZXIC_UINT32 db_en, ZXIC_UINT32 mode);

/***********************************************************/
/** 配置流级整形参数
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   flow_id   流队列号 ETM:0-9215,FTM:0-2047
* @param   cir       cir速率，单位Kb，范围[64Kb - 160Gb]
* @param   cbs       cbs桶深，单位KB，范围[1KB - 64M]
*                    注：cbs=0 表示关闭整形,即不限速
* @param   db_en     双桶整形使能，0-单桶，1-双桶
* @param   eir       eir速率，单位Kb，范围同cir
* @param   ebs       ebs桶深，单位Kb，范围同cbs
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_flow_para_set(DPP_DEV_T *dev,
                                      ZXIC_UINT32 flow_id,
                                      ZXIC_UINT32 cir,
                                      ZXIC_UINT32 cbs,
                                      ZXIC_UINT32 db_en,
                                      ZXIC_UINT32 eir,
                                      ZXIC_UINT32 ebs);


/***********************************************************/
/** tm配置调度器整形参数
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   se_id     调度器编号号 ETM:0x4800-0xABFF,FTM:0x1000-0x177F
* @param   pir       pir总速率，单位Kb，范围同cir
* @param   pbs       pbs总桶深，单位Kb，范围同cbs
* @param   db_en     整形模式，0-单桶，1-双桶，仅FQ8/WFQ8有效
* @param   cir       [0-3]调度器cir速率，单位Kb，范围[64Kb - 160Gb]
* @param   cbs       [0-3]调度器cbs桶深，单位KB，范围[1KB - 64M]
*                    注：cbs=0 表示关闭整形,即不限速
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_se_para_set(DPP_DEV_T *dev,
                                    ZXIC_UINT32 se_id,
                                    ZXIC_UINT32 pir,
                                    ZXIC_UINT32 pbs,
                                    ZXIC_UINT32 db_en,
                                    ZXIC_UINT32 cir,
                                    ZXIC_UINT32 cbs);


/***********************************************************/
/** 写入端口级整形信息
* @param   tm_type   0-ETM,1-FTM
* @param   port_id   端口号0-63
* @param   cir
* @param   cbs
* @param   c_en   c桶使能
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/03
************************************************************/
DPP_STATUS dpp_tm_shape_pp_para_wr(DPP_DEV_T *dev,
                                   ZXIC_UINT32 port_id,
                                   ZXIC_UINT32 cir,
                                   ZXIC_UINT32 cbs,
                                   ZXIC_UINT32 c_en);

/***********************************************************/
/** 配置端口级整形参数  更改整形转换公式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   port_id   端口号
* @param   p_para   整形信息:CIR/CBS/EN
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/04/15
************************************************************/
DPP_STATUS dpp_tm_shape_pp_para_set(DPP_DEV_T *dev,
                                    ZXIC_UINT32 port_id,
                                    const DPP_TM_SHAPE_PP_PARA_T *p_para);

/***********************************************************/
/** 配置轮转扫描使能和扫描速率
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   scan_en   轮转扫描使能。0：关闭，1：开启
* @param   scan_rate   轮转扫描速率，配置扫描周期不得少于256个周期
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/10
************************************************************/
DPP_STATUS dpp_tm_qmu_scan_rate_set(ZXIC_UINT32 dev_id,
                                    ZXIC_UINT32 scan_en,
                                    ZXIC_UINT32 scan_rate);

/***********************************************************/
/** 配置CMD_SW分端口(qmu出端口)整形速率和使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   shape_cir 整形值，单位Mbps，范围[0-160000]
* @param   shape_cbs 桶深， 单位B，范围[0-0x1EE00]
* @param   shape_en   整形使能
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author   whuashan   2020-3-17
************************************************************/
DPP_STATUS dpp_tm_qmu_egress_shape_set(ZXIC_UINT32 dev_id,
                                       ZXIC_UINT32 port_id,
                                       ZXIC_UINT32 shape_cir,
                                       ZXIC_UINT32 shape_cbs,
                                       ZXIC_UINT32 shape_en);


/***********************************************************/
/** 配置WRED丢弃曲线对应的参数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   WRED支持层次号，0:队列级，1:端口级
* @param   wred_id   队列级共支持16个WRED组0-15，端口级支持8组0-7
* @param   dp   共支持8个dp，取值0-7
* @param   p_para   配置的WRED组参数值，包含以下五个参数
           max_th  平均队列深度上限阈值
           min_th  平均队列深度下限阈值
           max_p  最大丢弃概率
           weight   平均队列深度计算权重
           q_len_th 队列深度阈值
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  taq     @date  2015/04/20
************************************************************/
DPP_STATUS dpp_tm_cgavd_wred_dp_line_para_set(ZXIC_UINT32 dev_id,
                                              DPP_TM_CGAVD_LEVEL_E level,
                                              ZXIC_UINT32 wred_id,
                                              ZXIC_UINT32 dp,
                                              DPP_TM_WRED_DP_LINE_PARA_T *p_para);


/***********************************************************/
/** 配置各级WRED丢弃曲线对应的参数
* @param   tm_type   0-ETM,1-FTM
* @param   level   WRED支持层次号，0:队列级，1:端口级
* @param   wred_id   队列级共支持16个WRED组0-15，端口级支持8组0-7
* @param   dp   共支持8个dp，取值0-7
* @param   max_th  平均队列深度上限阈值
* @param   min_th  平均队列深度下限阈值
* @param   max_p  最大丢弃概率
* @param   weight   平均队列深度计算权重
* @param   q_len_th   队列深度阈值
* @param   flag   忽略乘法里的当前包长和最大包长比标志位:1为忽略
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy     @date  2015/11/9
************************************************************/
DPP_STATUS dpp_tm_wred_dp_line_para_flag_wr(ZXIC_UINT32 dev_id,
                                            ZXIC_UINT32 level,
                                            ZXIC_UINT32 wred_id,
                                            ZXIC_UINT32 dp,
                                            ZXIC_UINT32 max_th,
                                            ZXIC_UINT32 min_th,
                                            ZXIC_UINT32 max_p,
                                            ZXIC_UINT32 weight,
                                            ZXIC_UINT32 q_len_th,
                                            ZXIC_UINT32 flag);

/***********************************************************/
/** 配置CPU设置的报文长度是否参与计算丢弃概率的使能
* @param   tm_type   0-ETM,1-FTM
* @param   flag   忽略乘法里的当前包长和最大包长比标志位:1为忽略
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy    @date  2015/11/9
************************************************************/
DPP_STATUS dpp_tm_cgavd_wred_pke_len_calc_sign_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 flag);


/***********************************************************/
/** TMMU TM纯片内模式配置
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   imem_en   1纯片内
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  说明：高有效，表示使能打开，TMMU不会再发起对MMU的读写操作，用户需要保证Cache PD全部命中。
* @see
* @author  wush      @date  2017/10/14
************************************************************/
DPP_STATUS dpp_tm_tmmu_imem_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 imem_en);

/***********************************************************/
/** TMMU 强制DDR RDY配置
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   ddr_force_rdy   1、如果bit【0】配置为1，则QMU看到的DDR0 RDY一直为1。
                           2、bit【0】代表DDR0，bit【7】代表DDR7。
                           3、纯片内模式需要配置为8'hff，排除DDR干扰。
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark
* @see
* @author  wush      @date  2017/10/14
************************************************************/
DPP_STATUS dpp_tm_tmmu_ddr_force_rdy_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 ddr_force_rdy);

/***********************************************************/
/** 写一片连续的TM寄存器
* @param   module_id 区分TM子模块
* @param   first_addr   起始寄存器的地址
* @param   reg_num   总共读取的寄存器数
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/26
************************************************************/
DPP_STATUS dpp_tm_wr_more_reg(ZXIC_UINT32 dev_id, ZXIC_UINT32 module_id, ZXIC_UINT32 first_addr, ZXIC_UINT32 first_data, ZXIC_UINT32 data_step, ZXIC_UINT32 reg_num);

/***********************************************************/
/** 打印tm诊断常用函数信息
*
* @return
* @remark  无
* @see
* @author  张明月      @date  2015/10/21
************************************************************/
DPP_STATUS dpp_tm_help(ZXIC_UINT32 dev_id);

/***********TM CPU软复位接口 End*************/


#endif/***function declaration***/

#endif/****_DPP_TM_H_****/





