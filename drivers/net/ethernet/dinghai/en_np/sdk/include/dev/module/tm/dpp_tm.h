/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_tm.h
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : djf
* 完成日期 : 2014/02/25
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/
#ifndef  _DPP_ETM_H_
#define  _DPP_ETM_H_


#include "dpp_tm_api.h"
#include "dpp_etm_reg.h"
#if ZXIC_REAL("ETM_MACRO")
/******************************************************************************
 *                               START: 宏定义                                *
 *****************************************************************************/
#define ETM_WRITE_CHECK (1)
#define DPP_TM_CGAVD_KILO_UL   (1024)/* Kbyte和byte换算 */
#define DPP_TM_CGAVD_TD_MAX (16*512)
/* CIR颗粒度: bps */
#define DPP_TM_SHAPE_CIR_STEP  ((ZXIC_DOUBLE) 400 * 1000 * 1000 * 1000 / 0x3FFFFFE)
//#define DPP_TM_SHAPE_CIR_STEP  (160069565217 / 0x3FFFFFE)
/* EIR颗粒度: bps */
//define DPP_TM_SHAPE_EIR_STEP  ((ZXIC_FLOAT) 160 * 1000 * 1000 * 1000 / 0x3FFFFFE)
#define DPP_TM_SHAPE_EIR_STEP  (160 * 1000 * 1000 * 1000 / 0x3FFFFFE)
#define DPP_TM_SHAPE_DEFAULT_CBS   (20)/*寄存器写入CBS的最小值，小于该值时，整形不准*/
#define DPP_TM_KILO_UL   (1024)
#define DPP_TM_KILO_ULL   (1000)
#define DPP_TM_QMU_PORT_SHAP_MAG (1.03)
#define DPP_ETM_SA_EGRS_MAX_PORTID   (66)
/*cfgmt_byte_mode:0:block mode 1:byte mode*/
#define DPP_TM_CGAVD_BLOCK_MODE   (0)
#define DPP_TM_CGAVD_ZXIC_UINT8_MODE   (1)

/*shap整形模板最大值:每2K流或调度器一一映射一块令牌桶资源,每块资源拥有0-127个模板*/
/*ETM 0xABFF/2K = 21 */
#define DPP_ETM_SHAP_TABEL_ID_MAX (22)
/*FTM 0x17FF/2K = 2 */
#define DPP_FTM_SHAP_TABEL_ID_MAX (3)
#define DPP_TM_SHAP_MAP_ID_MAX (128)

/******************************************************************************
 *                                END: 宏定义                                 *
 *****************************************************************************/
#endif /* ETM_MACRO */



#if ZXIC_REAL("ETM_STRUCT")
/******************************************************************************
 *                              START: 类型定义                               *
 *****************************************************************************/
/* block长度模式 */
typedef enum dpp_tm_blk_size_e
{
    DPP_ETM_BLK_SIZE_128_B = 0,
    DPP_ETM_BLK_SIZE_256_B,
    DPP_ETM_BLK_SIZE_512_B,
    DPP_ETM_BLK_SIZE_1024_B,
    DPP_ETM_BLK_SIZE_INVALID
} DPP_ETM_BLK_SIZE_E;

/* 动态门限放大因子参数 */
typedef struct dpp_tm_amplify_gene_para_t
{
    ZXIC_UINT32 amplify_gene[16];
} DPP_ETM_AMPLIFY_GENE_PARA_T;

/* 等价包长阈值 */
typedef struct dpp_tm_equal_pkt_len_th_para_t
{
    ZXIC_UINT32 equal_pkt_len_th[7];
} DPP_ETM_EQUAL_PKT_LEN_TH_PARA_T;

/* 等价包长*/
typedef struct dpp_tm_equal_pkt_len_para_t
{
    ZXIC_UINT32 equal_pkt_len[8];
} DPP_ETM_EQUAL_PKT_LEN_PARA_T;

/*ETM_STRUCT_STAT */
typedef enum dpp_tm_cgavd_stat_qnum_e
{
    DPP_ETM_CGAVD_STAT_QNUM1 = 0,
    DPP_ETM_CGAVD_STAT_QNUM2 = 1,
    DPP_ETM_CGAVD_STAT_QNUM_INVALID
} DPP_ETM_CGAVD_STAT_QNUM_E;

typedef enum dpp_tm_cgavd_stat_mode_e
{
    DPP_ETM_CGAVD_STAT_ALL_QUEUE = 0,
    DPP_ETM_CGAVD_STAT_ONE_QUEUE = 1,
    DPP_ETM_CGAVD_STAT_MODE_INVALID
} DPP_ETM_CGAVD_STAT_MODE_E;


typedef struct dpp_tm_cgavd_stat_para_t
{
    DPP_ETM_CGAVD_STAT_MODE_E mode;
    ZXIC_UINT32 q_id;
} DPP_ETM_CGAVD_STAT_PARA_T;

typedef struct dpp_tm_cgavd_stat_info_t
{
    DPP_ETM_CGAVD_STAT_MODE_E mode;
    ZXIC_UINT32 q_id;
    ZXIC_UINT32 lif_in_pkt_num;
    ZXIC_UINT32 enqueue_pkt_num;
    ZXIC_UINT32 dequeue_pkt_num;
    ZXIC_UINT32 td_drop_pkt_num;
    ZXIC_UINT32 wred_drop_pkt_num;
    ZXIC_UINT32 wred_dpi_pkt_num[8];
    ZXIC_UINT32 gred_drop_pkt_num;
    ZXIC_UINT32 gred_dpi_pkt_num[8];
} DPP_ETM_CGAVD_STAT_INFO_T;

typedef struct dpp_tm_qmu_stat_info_t
{
    ZXIC_UINT32 fc_cnt_mode;
    ZXIC_UINT32 mmu_qmu_wr_fc_cnt;
    ZXIC_UINT32 mmu_qmu_rd_fc_cnt;
    ZXIC_UINT32 qmu_cgavd_fc_cnt;
    ZXIC_UINT32 cgavd_qmu_pkt_cnt;
    ZXIC_UINT32 cgavd_qmu_pktlen_all;
    ZXIC_UINT32 cgavd_qmu_drop_tap;
    ZXIC_UINT32 last_drop_qnum;
    ZXIC_UINT32 crdt_qmu_credit_cnt;
    ZXIC_UINT32 qmu_to_qsch_report_cnt;
    ZXIC_UINT32 qmu_to_cgavd_report_cnt;
    ZXIC_UINT32 qmu_crdt_crs_normal_cnt;
    ZXIC_UINT32 qmu_crdt_crs_off_cnt;
    ZXIC_UINT32 qsch_qlist_shedule_cnt;
    ZXIC_UINT32 qsch_qlist_sch_ept_cnt;
    ZXIC_UINT32 qmu_to_mmu_blk_wr_cnt;
    ZXIC_UINT32 qmu_to_csw_blk_rd_cnt;
    ZXIC_UINT32 qmu_to_mmu_sop_wr_cnt;
    ZXIC_UINT32 qmu_to_mmu_eop_wr_cnt;
    ZXIC_UINT32 qmu_to_mmu_drop_wr_cnt;
    ZXIC_UINT32 qmu_to_csw_sop_rd_cnt;
    ZXIC_UINT32 qmu_to_csw_eop_rd_cnt;
    ZXIC_UINT32 qmu_to_csw_drop_rd_cnt;
    ZXIC_UINT32 mmu_to_qmu_wr_release_cnt;
    ZXIC_UINT32 mmu_to_qmu_rd_release_cnt;
} DPP_ETM_QMU_STAT_INFO_T;

typedef struct dpp_tm_qmu_spec_q_stat_info_t
{
    ZXIC_UINT32 observe_portfc_spec;
    ZXIC_UINT32 spec_lif_portfc_count;
    ZXIC_UINT32 observe_qnum_set;
    ZXIC_UINT32 spec_q_pkt_received;
    ZXIC_UINT32 spec_q_pkt_dropped;
    ZXIC_UINT32 spec_q_pkt_scheduled;
    ZXIC_UINT32 spec_q_wr_cmd_sent;
    ZXIC_UINT32 spec_q_rd_cmd_sent;
    ZXIC_UINT32 spec_q_pkt_enq;
    ZXIC_UINT32 spec_q_pkt_deq;
    ZXIC_UINT32 spec_q_crdt_uncon_received;
    ZXIC_UINT32 spec_q_crdt_cong_received;
    ZXIC_UINT32 spec_q_crs_normal_cnt;
    ZXIC_UINT32 spec_q_crs_off_cnt;
} DPP_ETM_QMU_SPEC_Q_STAT_INFO_T;

typedef struct dpp_tm_qmu_spec_bat_stat_info_t
{
    ZXIC_UINT32 observe_batch_set;
    ZXIC_UINT32 spec_bat_pkt_received;
    ZXIC_UINT32 spec_bat_pkt_dropped;
    ZXIC_UINT32 spec_bat_blk_scheduled;
    ZXIC_UINT32 spec_bat_wr_cmd_sent;
    ZXIC_UINT32 spec_bat_rd_cmd_sent;
    ZXIC_UINT32 spec_bat_pkt_enq;
    ZXIC_UINT32 spec_bat_pkt_deq;
    ZXIC_UINT32 spec_bat_crdt_uncon_received;
    ZXIC_UINT32 spec_bat_crdt_cong_received;
    ZXIC_UINT32 spec_bat_crs_normal_cnt;
    ZXIC_UINT32 spec_bat_crs_off_cnt;
} DPP_ETM_QMU_SPEC_BAT_STAT_INFO_T;

typedef struct qmu_port_shape_para
{
    ZXIC_UINT32 shape_value_amplified;
    ZXIC_UINT32 token_add_num;
    ZXIC_UINT32 token_gap;
} QMU_PORT_SHAPE_PARA;

/*etm 实际使用的ddr_attach、bank_num、depth(mmu中看到的)*/
typedef struct dpp_etm_qmu_init_para
{
    ZXIC_UINT32 etm_mmu_ddr_attach;
    ZXIC_UINT32 etm_mmu_bank_num;
    ZXIC_UINT32 etm_mmu_depth;
} DPP_ETM_QMU_INIT_PARA;

/*ftm 实际使用的ddr_attach、bank_num、depth(mmu中看到的)*/
typedef struct dpp_ftm_qmu_init_para
{
    ZXIC_UINT32 ftm_mmu_ddr_attach;
    ZXIC_UINT32 ftm_mmu_bank_num;
    ZXIC_UINT32 ftm_mmu_depth;
} DPP_FTM_QMU_INIT_PARA;

typedef struct dpp_tm_crdt_spwfq_start_num_t
{
    ZXIC_UINT32 start_num_fq;
    ZXIC_UINT32 start_num_fq2;
    ZXIC_UINT32 start_num_fq4;
    ZXIC_UINT32 start_num_fq8;
    ZXIC_UINT32 start_num_sp;
    ZXIC_UINT32 start_num_wfq;
    ZXIC_UINT32 start_num_wfq2;
    ZXIC_UINT32 start_num_wfq4;
    ZXIC_UINT32 start_num_wfq8;
} DPP_TM_CRDT_SPWFQ_START_NUM_T;


/******************************************************************************
 *                               END: 类型定义                                *
 *****************************************************************************/
#endif



#if ZXIC_REAL("ETM_FUNCTION")
/******************************************************************************
 *                              START: 函数声明                               *
 *****************************************************************************/
/***********************************************************/
/** 打印指定的全局数组值以及清空全局数组
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   para_x   数组index_x
* @param   para_y   数组index_y
* @param   clear_flag 清空shape全局数组
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark
* @see
* @author  xuhb      @date  2019/06/10
************************************************************/
DPP_STATUS dpp_tm_shape_para_array_prt(ZXIC_UINT32 dev_id,
                                       ZXIC_UINT32 para_x,
                                       ZXIC_UINT32 para_y,
                                       ZXIC_UINT32 clear_flag);

DPP_STATUS dpp_tm_qmu_qlist_set(ZXIC_UINT32 dev_id,
                                ZXIC_UINT32 ddr_num,
                                ZXIC_UINT32 bank_num_para,
                                ZXIC_UINT32 bank_vld,
                                ZXIC_UINT32 gene_para);

DPP_STATUS dpp_tm_cgavd_td_th_together_wr(DPP_DEV_T *dev,
                                          ZXIC_UINT32 level,
                                          ZXIC_UINT32 id,
                                          ZXIC_UINT32 td_th,
                                          ZXIC_UINT32 num);


DPP_STATUS dpp_tm_cgavd_td_th_together_get(DPP_DEV_T *dev,
                                           ZXIC_UINT32 level,
                                           ZXIC_UINT32 id,
                                           ZXIC_UINT32 num);

DPP_STATUS dpp_tm_cgavd_dyn_th_en_set_more(DPP_DEV_T *dev,
                                           DPP_TM_CGAVD_LEVEL_E level,
                                           ZXIC_UINT32 id,
                                           ZXIC_UINT32 en,
                                           ZXIC_UINT32 num);

/***********************************************************/
/**  配置基于优先级的QMU接收NPPU数据的fifo阈值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   sp  优先级0~7
* @param   th  指定优先级的fifo阈值0~511，单位为fifo条目，fifo深度512
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2016/10/18
************************************************************/
DPP_STATUS dpp_tm_cgavd_move_drop_sp_th_set(ZXIC_UINT32 dev_id,
                                            ZXIC_UINT32 sp,
                                            ZXIC_UINT32 th);


DPP_STATUS dpp_tm_wred_dp_line_para_wr(ZXIC_UINT32 dev_id,
                                       ZXIC_UINT32 level,
                                       ZXIC_UINT32 wred_id,
                                       ZXIC_UINT32 dp,
                                       ZXIC_UINT32 max_th,
                                       ZXIC_UINT32 min_th,
                                       ZXIC_UINT32 max_p,
                                       ZXIC_UINT32 weight,
                                       ZXIC_UINT32 q_len_th);


DPP_STATUS dpp_tm_gred_dp_line_para_wr(ZXIC_UINT32 dev_id,
                                       ZXIC_UINT32 dp,
                                       ZXIC_UINT32 max_th,
                                       ZXIC_UINT32 mid_th,
                                       ZXIC_UINT32 min_th,
                                       ZXIC_UINT32 max_p,
                                       ZXIC_UINT32 weight,
                                       ZXIC_UINT32 q_len_th);

DPP_STATUS dpp_tm_crdt_idle_check(DPP_DEV_T *dev);

/***********************************************************/
/** 获取各调度器的起始编号（etm共25K=25600,ftm共1920个）
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_spwfq_start_num  调度器起始编号结构体
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/28
************************************************************/
DPP_STATUS dpp_tm_crdt_wfqsp_get(DPP_DEV_T *dev,
                                 DPP_TM_CRDT_SPWFQ_START_NUM_T *p_spwfq_start_num);

/***********************************************************/
/** 获取调度器挂接配置参数
* @param   dev_id      设备编号
* @param   tm_type     0-ETM,1-FTM
* @param   se_id       调度器编号
* @param   p_se_para_tbl  调度器参数
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_se_para_get(DPP_DEV_T *dev, ZXIC_UINT32 se_id, DPP_ETM_CRDT_SE_PARA_TBL_T *p_se_para_tbl);



/***********************************************************/
/** 配置QMU工作模式，2M节点或4M节点
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   mode   0-第一种工作模式2M节点，1-第二种工作模式4M节点
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/25
************************************************************/
DPP_STATUS dpp_tm_cfgmt_qmu_work_mode_set(DPP_DEV_T *dev, DPP_TM_QMU_WORK_MODE_E mode);

/***********************************************************/
/** 读取QMU工作模式，2M节点或4M节点
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_mode   0-第一种工作模式2M节点，1-第二种工作模式4M节点
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/25
************************************************************/
DPP_STATUS dpp_tm_cfgmt_qmu_work_mode_get(DPP_DEV_T *dev, DPP_TM_QMU_WORK_MODE_E *p_mode);

/***********************************************************/
/** 配置本地sa_id，SA模式下需要配置
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   sa_id   配置的sa_id值
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/25    ftm模式下使用
************************************************************/
DPP_STATUS dpp_tm_cfgmt_local_sa_id_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 sa_id);

/***********************************************************/
/** 读取本地sa_id，SA模式下有效
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_sa_id   读取的sa_id值
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/25
************************************************************/
DPP_STATUS dpp_tm_cfgmt_local_sa_id_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_sa_id);

/***********************************************************/
/** CPU设置的各级队列深度配置
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level      拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   q_len_use_cpu_set_en   0：选取RAM中读出的队列深度；
* @param                          1：选取q_len_cpu_set值
* @param   q_len_cpu_set    CPU设置的各级队列深度,单位为block。
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/06/19
************************************************************/
DPP_STATUS dpp_tm_q_len_use_cpu_set(DPP_DEV_T *dev,
                                    DPP_TM_CGAVD_LEVEL_E level,
                                    ZXIC_UINT32 q_len_use_cpu_set_en,
                                    ZXIC_UINT32 q_len_cpu_set);

/***********************************************************/
/** CPU设置的各级平均队列深度配置
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level      拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   q_avg_len_use_cpu_set_en   0：选取RAM中读出的队列深度；
* @param                              1：选取q_avg_len_cpu_set值
* @param   q_avg_len_cpu_set    CPU设置的各级平均队列深度。
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/06/19
************************************************************/
DPP_STATUS dpp_tm_q_avg_len_use_cpu_set(DPP_DEV_T *dev,
                                        DPP_TM_CGAVD_LEVEL_E level,
                                        ZXIC_UINT32 q_avg_len_use_cpu_set_en,
                                        ZXIC_UINT32 q_avg_len_cpu_set);

/***********************************************************/
/** 配置QMU查询队列Qos开关
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   q_id:  队列号
* @param   qos_sign: qos开关 0:关闭  1:开启
* @param
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/03
************************************************************/
DPP_STATUS dpp_tm_qmu_qos_sign_set(DPP_DEV_T *dev,
                                   ZXIC_UINT32 q_id,
                                   ZXIC_UINT32 qos_sign);

/***********************************************************/
/** 配置授权分发使能或者关闭
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   en   配置的值，0-关闭授权分发，1-使能授权分发
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/25
************************************************************/
DPP_STATUS dpp_tm_crdt_credit_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 en);

/***********************************************************/
/** 读取授权分发使能或者关闭
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_en   读出的值，0-关闭授权分发，1-使能授权分发
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/25
************************************************************/
DPP_STATUS dpp_tm_crdt_credit_en_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_en);

/***********************************************************/
/** 配置授权产生间隔
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   crdt_space_choose  授权发送间隔 0:固定16个周期 1：查表
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/10
************************************************************/
DPP_STATUS dpp_tm_crdt_space_choose_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 crdt_space_choose);

/***********************************************************/
/** 获得授权产生间隔
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   crdt_space_choose   授权发送间隔 0:固定16个周期 1：查表
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/10
************************************************************/
DPP_STATUS dpp_tm_crdt_space_choose_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_crdt_space_choose);

/***********************************************************/
/** 配置端口拥塞令牌桶使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   port_id   端口号：0~63
* @param   port_en   端口拥塞令牌桶使能，1表示不使用拥塞令牌桶的授权，0表示可以使用拥塞令牌桶授权
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/11
************************************************************/
DPP_STATUS dpp_tm_crdt_port_congest_en_set(DPP_DEV_T *dev,
                                           ZXIC_UINT32 port_id,
                                           ZXIC_UINT32 port_en);

/***********************************************************/
/** 获得端口拥塞令牌桶使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   port_id   端口号：0~120
* @param   p_port_en   端口拥塞令牌桶使能，1表示不使用拥塞令牌桶的授权，0表示可以使用拥塞令牌桶授权
*
* @return
* @remark  无
* @see
* @author  djf      @date  2015/03/11
************************************************************/
DPP_STATUS dpp_tm_crdt_port_congest_en_get(DPP_DEV_T *dev,
                                           ZXIC_UINT32 port_id,
                                           ZXIC_UINT32 *p_port_en);

/***********************************************************/
/** 流级中间级补充扫描机制使能配置
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   renew_scan_flow:0为不开启补充扫描  others:开启补充扫描，扫描间隔
* @param   renew_scan_mid:0为不开启补充扫描  others:开启补充扫描，扫描间隔
* @param
* @return
* @remark  无
* @see
* @author  yjd      @date  2015/07/03
************************************************************/
DPP_STATUS dpp_tm_crdt_renew_scan_set(ZXIC_UINT32 dev_id,
                                      ZXIC_UINT32 renew_scan_flow,
                                      ZXIC_UINT32 renew_scan_mid);

/***********************************************************/
/** 获取流级中间级补充扫描机制使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   renew_scan_flow:0为不开启补充扫描  others:开启补充扫描，扫描间隔
* @param   renew_scan_mid:0为不开启补充扫描  others:开启补充扫描，扫描间隔
* @param
* @return
* @remark  无
* @see
* @author  yjd      @date  2015/07/03
************************************************************/
DPP_STATUS dpp_tm_crdt_renew_scan_get(ZXIC_UINT32 dev_id,
                                      ZXIC_UINT32 *renew_scan_flow,
                                      ZXIC_UINT32 *renew_scan_mid);


/***********************************************************/
/**cpu配置flow_id的crs强制为normal或者off开关使能，用于检测SA模式下队列到授权流的多对一问题
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   flow_id:流号(和授权流号一一对应)
*                en   强制配置crs的使能，0-不使能，1-使能
*                crs_value:强制配置crs的值2'b00:off; 2'b01:low; 2'b10:normal；
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/04/25
************************************************************/
DPP_STATUS dpp_tm_crdt_crs_sheild_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 flow_id, ZXIC_UINT32 en, ZXIC_UINT32 crs_value);


/***********************************************************/
/**获取flow_id的crs强制为normal或者off开关使能，用于检测SA模式下队列到授权流的多对一问题
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   flow_id:流号(和授权流号一一对应)
*                en   强制配置crs的使能，0-不使能，1-使能
*                crs_value:强制配置crs的值2'b00:off; 2'b01:low; 2'b10:normal；
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/04/25
************************************************************/
DPP_STATUS dpp_tm_crdt_crs_sheild_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_flow_id, ZXIC_UINT32 *p_en, ZXIC_UINT32 *p_crs_value);

/***********************************************************/
/** 控制授权速率的门限
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   index   0~6
* @param   rci_grade_th_0_data
*
* @return
* @remark  无
* @see
* @author  wush      @date  2017/10/17
************************************************************/
DPP_STATUS dpp_tm_crdt_rci_grade_th_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 index, ZXIC_UINT32 rci_grade_th_0_data);

DPP_STATUS dpp_tm_crdt_rci_grade_th_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 index, ZXIC_UINT32 *p_rci_grade_th_0_data);


/***********************************************************/
/** 控制授权间隔的门限，建议大于等于0XF，不可取0；
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   index   0~7
* @param   asm_interval_0_data   控制授权间隔的门限，建议大于等于0XF，不可取0；
*
* @return
* @remark  无
* @see
* @author  wush      @date  2017/10/17
************************************************************/
DPP_STATUS dpp_tm_crdt_asm_interval_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 index, ZXIC_UINT32 asm_interval_0_data);

DPP_STATUS dpp_tm_crdt_asm_interval_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 index, ZXIC_UINT32 *p_asm_interval_0_data);


/***********************************************************/
/** rci的级别
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_rci_grade_data
*
* @return
* @remark  无
* @see
* @author  wush      @date  2017/10/17
************************************************************/
DPP_STATUS dpp_tm_crdt_rci_grade_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_rci_grade_data);

DPP_STATUS dpp_tm_crdt_rci_value_r_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_crdt_rci_value_r_data);

DPP_STATUS dpp_tm_crdt_interval_now_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_crdt_interval_now_data);


/***********************************************************/
/** 配置crdt interval使能，
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   crdt_interval_en_cfg_data   授权分发间隔使能，1打开，0关闭
*
* @return
* @remark  无
* @see
* @author  wush      @date  2017/03/27
************************************************************/
DPP_STATUS dpp_tm_crdt_interval_en_cfg_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 crdt_interval_en_cfg_data);

/***********************************************************/
/**配置拥塞状态(有效链路数+UCN等级)到授权产生间隔的映射表
* @param   dev_id   设备编号
* @param   valid_serdes_num   有效的链路数(0~32)
* @param   ucn_level   UCN等级(0~7)
* @param   cr_clk   授权产生间隔(0~0x3fffff)
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2017/03/28
************************************************************/
DPP_STATUS dpp_tm_crdt_cfgmt_interval_set(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 valid_serdes_num,
                                          ZXIC_UINT32 ucn_level,
                                          ZXIC_UINT32 cr_clk);

/***********************************************************/
/** 读取crdt interval使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   crdt_interval_en_cfg_data   授权分发间隔使能，1打开，0关闭
*
* @return
* @remark  无
* @see
* @author  wush      @date  2017/03/27
************************************************************/
DPP_STATUS dpp_tm_crdt_interval_en_cfg_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_crdt_interval_en_cfg_data);


/***********************************************************/
/** 屏蔽ucn/asm_rdy的时能信号
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   ucn_rdy_shield_en   是否屏蔽ucn_rdy信号，1屏蔽，0不屏蔽
* @param   asm_rdy_shield_en   是否屏蔽asm_rdy信号，1屏蔽，0不屏蔽
*
* @return
* @remark  无
* @see
* @author  wush      @date  2017/10/17
************************************************************/
DPP_STATUS dpp_tm_crdt_ucn_asm_rdy_shield_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 ucn_rdy_shield_en, ZXIC_UINT32 asm_rdy_shield_en);

DPP_STATUS dpp_tm_crdt_ucn_asm_rdy_shield_en_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_ucn_rdy_shield_en, ZXIC_UINT32 *p_asm_rdy_shield_en);


/***********************************************************/
/** 配置QMU队列授权价值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   credit_value   授权价值，默认值是400Byte
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_credit_value_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 credit_value);

/***********************************************************/
/** QMU DDR随机模式时，DDR随机组配置
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   ddr_num   ddr组数，1-6组
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_ddr_rand_grp_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 ddr_num);

/***********************************************************/
/** 配置QMU DDR BANK随机模式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   ddr_random   模式:0-轮询模式；1-随机模式
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_ddr_random_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 ddr_random);

/***********************************************************/
/** QMU配置完成寄存器，在QMU链表和DDR随机模式寄存器写入后，将此寄存器写1，完成QMU配置
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_cfg_done_set(ZXIC_UINT32 dev_id);


/***********************************************************/
/** 配置CRS的e桶产生的crbal门限值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   index   crs组数：0~15
* @param   crs_th   CRS产生的crbal门限值
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  xuhb      @date  2021/02/14
************************************************************/
DPP_STATUS dpp_tm_qmu_crs_eir_th_set(ZXIC_UINT32 dev_id,
                                 ZXIC_UINT32 index,
                                 ZXIC_UINT32 crs_th);


/***********************************************************/
/** 配置CRS产生的crbal门限值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   index   crs组数：0~15
* @param   crs_th   CRS产生的crbal门限值
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/11
************************************************************/
DPP_STATUS dpp_tm_qmu_crs_th_set(ZXIC_UINT32 dev_id,
                                 ZXIC_UINT32 index,
                                 ZXIC_UINT32 crs_th);

/***********************************************************/
/** 配置CRS产生的空队列确保门限值
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   que_type   队列类型编号(0~15)
* @param   empty_que_ack_th   空队列确保授权门限
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/09
************************************************************/
DPP_STATUS dpp_tm_qmu_crs_th2_set(ZXIC_UINT32 dev_id,
                                  ZXIC_UINT32 que_type,
                                  ZXIC_UINT32 empty_que_ack_th);

/***********************************************************/
/** 配置QMU端口间交织模式
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   pkt_blk_mode   交织模式: 1-按包交织; 0-按block交织SA模式只能配置为1
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  szq      @date  2015/03/25
************************************************************/
DPP_STATUS dpp_tm_qmu_pkt_blk_mode_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 pkt_blk_mode);

/***********************************************************/
/** 获取QMU端口间交织模式
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_pkt_blk_mode   交织模式: 0-按包交织; 1-按block交织SA模式只能配置为1
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  szq      @date  2015/03/25
************************************************************/
DPP_STATUS dpp_tm_qmu_pkt_blk_mode_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_pkt_blk_mode);

/***********************************************************/
/** 配置读命令老化使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   aged_en   读命令老化使能：0：不使能；1：使能
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/11
************************************************************/
DPP_STATUS dpp_tm_qmu_wr_aged_en_set(DPP_DEV_T *dev, ZXIC_UINT32 aged_en);

/***********************************************************/
/** 配置读命令老化速率
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   scan_time   读命令老化速率（扫描间隔时间）
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/11
************************************************************/
DPP_STATUS dpp_tm_qmu_wr_aged_scan_time_set(DPP_DEV_T *dev, ZXIC_UINT32 scan_time);

/***********************************************************/
/** 获得读命令老化速率
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_scan_time   读命令老化速率（扫描间隔时间）
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/11
************************************************************/
DPP_STATUS dpp_tm_qmu_wr_aged_scan_time_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_scan_time);


/***********************************************************/
/** 配置QMU队列到目的SAId的映射ETM 模式没有，
FTM 模式才有1024个。
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   que_id   队列号
* @param   dest_said   目的said
*
* @return   0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  szq      @date  2015/04/08
************************************************************/
DPP_STATUS dpp_tm_qmu_qcfg_dest_id_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 que_id, ZXIC_UINT32 dest_said);


/***********************************************************/
/** 获取QMU队列到目的SAId的映射ETM 模式没有，
FTM 模式才有1024个。
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   que_id   队列号
* @param   p_dest_said   目的said
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  szq      @date  2015/04/08
************************************************************/
DPP_STATUS dpp_tm_qmu_qcfg_dest_id_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 que_id, ZXIC_UINT32 *p_dest_said);

/***********************************************************/
/** 配置出队暂存使用的进程总数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   used_inall   出队暂存使用的进程总数=19-N，默认3表示使用16个进程
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/18
************************************************************/
DPP_STATUS dpp_tm_qmu_pid_use_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 used_inall);

/***********************************************************/
/** 获得出队暂存使用的进程总数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_used_inall   出队暂存使用的进程总数=19-N，默认3表示使用16个进程
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/23
************************************************************/
DPP_STATUS dpp_tm_qmu_pid_use_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_used_inall);

/***********************************************************/
/** 配置出队暂存自回加进程总数阈值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   round_th   出队暂存自回加进程总数阈值=19-N，默认4表示使用15个进程就自回加
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/18
************************************************************/
DPP_STATUS dpp_tm_qmu_pid_round_th_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 round_th);

/***********************************************************/
/** 获得出队暂存自回加进程总数阈值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_round_th   出队暂存自回加进程总数阈值=19-N，默认4表示使用15个进程就自回加
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/23
************************************************************/

DPP_STATUS dpp_tm_qmu_pid_round_th_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_round_th);

/***********************************************************/
/** 配置CRS发送强制使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   en   配置的值，0-不使能，1-使能
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_crs_force_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 en);

/***********************************************************/
/** 配置CRS发送强制的队列
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   q_id   队列号
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_crs_force_q_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 q_id);


/***********************************************************/
/** 配置CRS发送强置的状态
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   crs_state   CRS发送强置的状态
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/18
************************************************************/
DPP_STATUS dpp_tm_qmu_crs_force_state_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 crs_state);

/***********************************************************/
/** 配置特定队列发送特定CRS
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   qnum   队列号
*               qcfg_qsch_crs_force_crs:CRS状态(0：off；1：normal。)
                 qcfg_qsch_crs_force_en:CRS发送强置使能(0：不使能；1：使能。)
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_qnum_crs_force(ZXIC_UINT32 dev_id,
                                     ZXIC_UINT32 qnum,
                                     ZXIC_UINT32 qcfg_qsch_crs_force_crs,
                                     ZXIC_UINT32 qcfg_qsch_crs_force_en);


/***********************************************************/
/** 配置CRS状态
* @param   dev_id  设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   qnum   队列号
* @param   state
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/19
************************************************************/
DPP_STATUS dpp_tm_qmu_crs_state_set(ZXIC_UINT32 dev_id,
                                    ZXIC_UINT32 qnum,
                                    ZXIC_UINT32 state);

/***********************************************************/
/** 获得CRS状态
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   qnum   队列号
* @param   p_state
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/23
************************************************************/
DPP_STATUS dpp_tm_qmu_crs_state_get(ZXIC_UINT32 dev_id,
                                    ZXIC_UINT32 qnum,
                                    ZXIC_UINT32 *p_state);

/***********************************************************/
/** 配置自动授权队列范围
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   first_que   自授权起始队列号
* @param   last_que   自授权终止队列号
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/19
************************************************************/
DPP_STATUS dpp_tm_qmu_auto_credit_que_set(DPP_DEV_T *dev,
                                          ZXIC_UINT32 first_que,
                                          ZXIC_UINT32 last_que);

/***********************************************************/
/** 获得自动授权队列范围
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_first_que   自授权起始队列号
* @param   p_last_que   自授权终止队列号
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/23
************************************************************/
DPP_STATUS dpp_tm_qmu_auto_credit_que_get(DPP_DEV_T *dev,
                                          ZXIC_UINT32 *p_first_que,
                                          ZXIC_UINT32 *p_last_que);

/***********************************************************/
/** 配置自动授权开启使能及扫描速率
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   auto_crdt_en   自动授权开启使能，默认关闭。0：关闭；1：开启
* @param   auto_crdt_rate   自授权速率配置
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/19
************************************************************/
DPP_STATUS dpp_tm_qmu_auto_credit_rate_set(DPP_DEV_T *dev,
                                           ZXIC_UINT32 auto_crdt_en,
                                           ZXIC_UINT32 auto_crdt_rate);

/***********************************************************/
/** 获得自动授权开启使能及扫描速率
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_auto_crdt_en   自动授权开启使能，默认关闭。0：关闭；1：开启
* @param   p_auto_crdt_rate   自授权速率配置
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/23
************************************************************/
DPP_STATUS dpp_tm_qmu_auto_credit_rate_get(DPP_DEV_T *dev,
                                           ZXIC_UINT32 *p_auto_crdt_en,
                                           ZXIC_UINT32 *p_auto_crdt_rate);

/***********************************************************/
/** 配置授权丢弃使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   all_drop_en   所有授权丢弃使能：1：允许丢弃所有授权；0：仅允许丢弃拥塞授权
* @param   drop_en   授权丢弃使能：1：允许丢弃授权；0：禁止丢弃授权
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/23
************************************************************/
DPP_STATUS dpp_tm_qmu_crbal_drop_en_set(ZXIC_UINT32 dev_id,
                                        ZXIC_UINT32 all_drop_en,
                                        ZXIC_UINT32 drop_en);

/***********************************************************/
/** 获得授权丢弃使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_all_drop_en   所有授权丢弃使能：1：允许丢弃所有授权；0：仅允许丢弃拥塞授权
* @param   p_drop_en   授权丢弃使能：1：允许丢弃授权；0：禁止丢弃授权
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/23
************************************************************/
DPP_STATUS dpp_tm_qmu_crbal_drop_en_get(ZXIC_UINT32 dev_id,
                                        ZXIC_UINT32 *p_all_drop_en,
                                        ZXIC_UINT32 *p_drop_en);

/***********************************************************/
/**设置自然拥塞反压门限值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param
* @return
* @remark  无
* @see
* @author  yjd     @date  2015/07/16
************************************************************/
DPP_STATUS dpp_tm_qmu_qcfg_csch_congest_th_set(DPP_DEV_T *dev, ZXIC_UINT32 port_id, ZXIC_UINT32 qmu_congest_th);

/***********************************************************/
/**获取自然拥塞反压门限值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param
* @return
* @remark  无
* @see
* @author  yjd     @date  2015/07/16
************************************************************/
DPP_STATUS dpp_tm_qmu_qcfg_csch_congest_th_get(DPP_DEV_T *dev, ZXIC_UINT32 port_id, ZXIC_UINT32 *p_qmu_congest_th);

/***********************************************************/
/**设置CMD_SCH分优先级反压门限值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param
* @return
* @remark  无
* @see
* @author  yjd     @date  2015/07/16
************************************************************/
DPP_STATUS dpp_tm_qmu_qcfg_csch_sp_fc_th_set(DPP_DEV_T *dev, ZXIC_UINT32 port_id, ZXIC_UINT32 q_pri, ZXIC_UINT32 qmu_sp_fc_th);

/***********************************************************/
/**获取自然拥塞反压门限值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param
* @return
* @remark  无
* @see
* @author  yjd     @date  2015/07/16
************************************************************/
DPP_STATUS dpp_tm_qmu_qcfg_csch_sp_fc_th_get(DPP_DEV_T *dev, ZXIC_UINT32 port_id, ZXIC_UINT32 q_pri, ZXIC_UINT32 *p_qmu_sp_fc_th);

/***********************************************************/
/** 配置QMU需要检测流控的端口号
* @param   dev_id   设备号
* @param   tm_type   0-ETM,1-FTM
* @param   port_id   端口号
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/05/07
************************************************************/
DPP_STATUS dpp_tm_qmu_observe_portfc_set(DPP_DEV_T *dev, ZXIC_UINT32 port_id);

/***********************************************************/
/** 配置pfc使能
* @param   dev_id   设备编号
* @param   pfc_en   配置的值，0-不使能pfc，1-使能pfc
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  lsy      @date  2022/08/22
************************************************************/
DPP_STATUS dpp_tm_qmu_pfc_en_set(DPP_DEV_T *dev, ZXIC_UINT32 pfc_en);

/***********************************************************/
/** 读取pfc使能
* @param   dev_id   设备编号
* @param   pfc_en   配置的值，0-不使能pfc，1-使能pfc
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  lsy      @date  2022/08/22
************************************************************/
DPP_STATUS dpp_tm_qmu_pfc_en_get(DPP_DEV_T *dev, ZXIC_UINT32 *pfc_en);

/***********************************************************/
/** 配置端口pfc掩码
* @param   dev_id   设备编号
* @param   port_id   端口号：0~63
* @param   port_en   端口掩码配置，1pfc模式下该端口接收olif的优先级反压，
*                                  0pfc模式下该端口不接受olif的优先级反压，并将反压信号全部置1
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  lsy      @date  2022/08/22
************************************************************/
DPP_STATUS dpp_tm_qmu_port_pfc_make_set(DPP_DEV_T *dev,
                                        ZXIC_UINT32 port_id,
                                        ZXIC_UINT32 port_en);

/***********************************************************/
/** 获得端口pfc掩码
* @param   dev_id   设备编号
* @param   port_id   端口号：0~63
* @param   port_en   端口掩码配置，1pfc模式下该端口接收olif的优先级反压，
*                                  0pfc模式下该端口不接受olif的优先级反压，并将反压信号全部置1
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  lsy      @date  2022/08/22
************************************************************/
DPP_STATUS dpp_tm_qmu_port_pfc_make_get(DPP_DEV_T *dev,
                                        ZXIC_UINT32 port_id,
                                        ZXIC_UINT32 *p_port_en);



/***********************************************************/
/** QMU初始化配置场景一：4组*2bank，MMU开启rotatjon，MMU实际分配4~7组，每组2~3bank
* @param   dev_id   设备编号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_1(ZXIC_UINT32 dev_id);


/***********************************************************/
/** QMU初始化配置场景一：4组*2bank，MMU开启rotatjon，MMU实际分配0~3组，每组2~3bank
* @param   dev_id   设备编号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_ftm_qmu_init_set_1(ZXIC_UINT32 dev_id);


/***********************************************************/
/** QMU初始化配置场景一ddr4模式，开启MMU/IP rotatjon，tm看到4组*2bank，对应DDR分配4~7组，每组2~3bank*************
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_1(ZXIC_UINT32 dev_id);


/***********************************************************/
/** QMU初始化配置场景二：ddr3模式，tm看到4组*4bank，对应DDR分配4~7组，每组2~3,6~7bank
* @param   dev_id   设备编号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_2(ZXIC_UINT32 dev_id);


/***********************************************************/
/** QMU初始化配置场景二：ddr3模式，tm看到4组*4bank，对应DDR分配0~3组，每组2~3,6~7bank*
* @param   dev_id   设备编号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_ftm_qmu_init_set_2(ZXIC_UINT32 dev_id);


/***********************************************************/
/** QMU初始化配置场景二：ddr3模式，tm看到4组*4bank，对应DDR分配4~7组，每组2~3,6~7bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_2(ZXIC_UINT32 dev_id);


/***********************************************************/
/** QMU初始化配置场景二：ddr3模式，tm看到4组*4bank，对应DDR分配4~7组，每组2~3,6~7bank
*    depth=64,代表16k
* @param   dev_id   设备编号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_pd16k_2(ZXIC_UINT32 dev_id, ZXIC_UINT32 depth);


/***********************************************************/
/** QMU初始化配置场景二：ddr3模式，8组*8bank
*    depth=64,代表16k
* @param   dev_id   设备编号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  sun      @date  2023/04/12
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_chuk32(ZXIC_UINT32 dev_id, ZXIC_UINT32 depth);

/***********************************************************/
/** QMU初始化配置场景二：ddr3模式，tm看到4组*4bank，对应DDR分配0~3组，每组2~3,6~7bank*
* @param   dev_id   设备编号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_ftm_qmu_init_set_pd16k_2(ZXIC_UINT32 dev_id, ZXIC_UINT32 depth);


/***********************************************************/
/** QMU初始化配置场景二：ddr3模式，tm看到4组*4bank，对应DDR分配4~7组，每组2~3,6~7bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_pd16k_2(ZXIC_UINT32 dev_id, ZXIC_UINT32 depth);

/***********************************************************/
/** QMU初始化配置场景：ddr3模式，8组ddr*8bank
* @param   dev_id   设备编号
* @param   depth   bank depth
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  sun      @date  2023/04/12
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_chuk32(ZXIC_UINT32 dev_id, ZXIC_UINT32 depth);


/***********************************************************/
/** QMU初始化配置场景三：8组*2bank，MMU开启rotatjon，MMU实际分配0~7组，每组6~7bank
* @param   dev_id   设备编号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_3(ZXIC_UINT32 dev_id);

/***********************************************************/
/** QMU初始化配置场景三：8组*2bank，MMU开启rotatjon，MMU实际分配0~3组，每组2~3bank
* @param   dev_id   设备编号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_ftm_qmu_init_set_3(ZXIC_UINT32 dev_id);

/***********************************************************/
/** QMU初始化配置场景三ETM: 8组*2bank，MMU开启rotatjon，MMU实际分配0~7组，每组6~7bank
*     FTM: 8组*2bank，MMU开启rotatjon，MMU实际分配0~3组，每组2~3bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_3(ZXIC_UINT32 dev_id);


/***********************************************************/
/**场景4：TM共享2组ddr:FTM为2、4组ddr的01bank，ETM为2、4组ddr 23bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/06/06
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_4(ZXIC_UINT32 dev_id);

/***********************************************************/
/**场景4：TM共享2组ddr:FTM为2、4组ddr的01bank，ETM为2、4组ddr 23bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/06/06
************************************************************/
DPP_STATUS dpp_ftm_qmu_init_set_4(ZXIC_UINT32 dev_id);

/***********************************************************/
/**场景4：TM共享2组ddr:FTM为2、4组ddr的01bank，ETM为2、4组ddr 23bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/06/06
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_4(ZXIC_UINT32 dev_id);


/***********************************************************/
/** 场景5：TM共享4组ddr:FTM为1234组ddr的01bank，ETM为1234组ddr 23bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/06/06
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_5(ZXIC_UINT32 dev_id);


/***********************************************************/
/** 场景5：TM共享4组ddr:FTM为1234组ddr的01bank，ETM为1234组ddr 23bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/06/06
************************************************************/
DPP_STATUS dpp_ftm_qmu_init_set_5(ZXIC_UINT32 dev_id);


/***********************************************************/
/** 场景5：TM共享4组ddr:FTM为1234组ddr的01bank，ETM为1234组ddr 23bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/06/06
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_5(ZXIC_UINT32 dev_id);


/***********************************************************/
/** QMU初始化配置场景6：4组*2bank，MMU实际分配ftm:4,6,7,9组(01bank); etm:4,6,7,9组(23bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/05/13
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_6(ZXIC_UINT32 dev_id);

/***********************************************************/
/** QMU初始化配置场景6：4组*2bank，MMU实际分配ftm:4,6,7,9组(01bank); etm:4,6,7,9组(23bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/05/13
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_6(ZXIC_UINT32 dev_id);


/***********************************************************/
/** QMU初始化配置场景6：4组*2bank，MMU实际分配ftm:4,6,7,9组(01bank); etm:4,6,7,9组(23bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/05/13
************************************************************/
DPP_STATUS dpp_ftm_qmu_init_set_6(ZXIC_UINT32 dev_id);

/***********************************************************/
/** QMU初始化配置场景6：4组*4bank，MMU实际分配1,2(ftm); 5,6(etm)组，每组2~3 6~7bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_7(ZXIC_UINT32 dev_id);

/***********************************************************/
/** QMU初始化配置场景6：4组*4bank，MMU实际分配5~6组，每组2~3 6~7bank
* @param   dev_id   设备编号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_7(ZXIC_UINT32 dev_id);


/***********************************************************/
/** QMU初始化配置场景6：4组*4bank，MMU实际分配1~2组，每组2~3 6~7bank
* @param   dev_id   设备编号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_ftm_qmu_init_set_7(ZXIC_UINT32 dev_id);


/***********************************************************/
/** QMU初始化配置场景6：4组*4bank，MMU实际分配1,2(ftm); 5,6(etm)组，每组2~3 6~7bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_8(ZXIC_UINT32 dev_id);

/***********************************************************/
/** QMU初始化配置场景6：4组*4bank，MMU实际分配5~6组，每组2~3 6~7bank
* @param   dev_id   设备编号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_8(ZXIC_UINT32 dev_id);


/***********************************************************/
/** QMU初始化配置场景6：4组*4bank，MMU实际分配1~2组，每组2~3 6~7bank
* @param   dev_id   设备编号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_ftm_qmu_init_set_8(ZXIC_UINT32 dev_id);


/***********************************************************/
/** QMU初始化配置场景9：4组*2bank，MMU实际分配ftm:6,7,8,9组(01bank); etm:6,7,8,9组(23bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/09/28
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_9(ZXIC_UINT32 dev_id);

/***********************************************************/
/** QMU初始化配置场景9：4组*2bank，MMU实际分配ftm:6,7,8,9组(01bank); etm:6,7,8,9组(23bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/09/28
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_9(ZXIC_UINT32 dev_id);


/***********************************************************/
/** QMU初始化配置场景9：4组*2bank，MMU实际分配ftm:6,7,8,9组(01bank); etm:6,7,8,9组(23bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/09/28
************************************************************/
DPP_STATUS dpp_ftm_qmu_init_set_9(ZXIC_UINT32 dev_id);

/***********************************************************/
/** QMU初始化配置场景10：独享2组*8bank，MMU实际分配1,2; etm or ftm使用0-7bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/12/16
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_10(ZXIC_UINT32 dev_id);

/***********************************************************/
/** QMU初始化配置场景10：独享2组*8bank，MMU实际分配1,2; etm or ftm使用0-7bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/12/16
************************************************************/
DPP_STATUS dpp_ftm_qmu_init_set_10(ZXIC_UINT32 dev_id);

/***********************************************************/
/** QMU初始化配置场景10：独享2组*8bank，MMU实际分配1,2; etm or ftm使用0-7bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/12/16
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_10(ZXIC_UINT32 dev_id);


/***********************************************************/
/**场景11：TM共享2组ddr:FTM为ddr_no1、ddr_no2组ddr的01bank，ETM为ddr_no1、ddr_no2组ddr 23bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/06/06
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_11(ZXIC_UINT32 dev_id, ZXIC_UINT32 ddr_no1, ZXIC_UINT32 ddr_no2);

/***********************************************************/
/**场景11：TM共享2组ddr:FTM为ddr_no1、ddr_no2组ddr的01bank，ETM为ddr_no1、ddr_no2组ddr 23bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/06/06
************************************************************/
DPP_STATUS dpp_ftm_qmu_init_set_11(ZXIC_UINT32 dev_id, ZXIC_UINT32 ddr_no1, ZXIC_UINT32 ddr_no2);

/***********************************************************/
/**场景4：TM共享2组ddr:FTM为ddr_no1,ddr_no2组ddr的01bank，ETM为ddr_no1,ddr_no2组ddr 23bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/06/06
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_11(ZXIC_UINT32 dev_id, ZXIC_UINT32 ddr_no1, ZXIC_UINT32 ddr_no2);


/***********************************************************/
/** 
QMU/MMU初始化配置场景12：每个tm只用1组*4bank，MMU实际分配ftm:0-9中指定组(4567bank); etm:0-9中指定组(0123bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/4/14
************************************************************/
DPP_STATUS dpp_ftm_qmu_init_set_12(ZXIC_UINT32 dev_id, ZXIC_UINT32 ftm_ddr_no);


/***********************************************************/
/** 
QMU/MMU初始化配置场景15：每个tm只用1组*4bank，MMU实际分配ftm:0-9中指定组(4567bank); etm:0-9中指定组(0123bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/4/14
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_12(ZXIC_UINT32 dev_id, ZXIC_UINT32 ddr_no);



/***********************************************************/
/** QMU初始化配置场景13：TM独享4组ddr:FTM为指定4组ddr的0~7bank，
                                      ETM为指定4组ddr 0~7bank
* @param   dev_id   设备编号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_13(ZXIC_UINT32 dev_id, ZXIC_UINT32 ddr_no);

/***********************************************************/
/** QMU初始化配置场景13：TM独享4组ddr:FTM为指定4组ddr的0~7bank，
                                      ETM为指定4组ddr 0~7bank
* @param   dev_id   设备编号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_ftm_qmu_init_set_13(ZXIC_UINT32 dev_id, ZXIC_UINT32 ddr_no);


/***********************************************************/
/** QMU/MMU初始化配置场景13：TM独享4组ddr:FTM为指定4组ddr的0~7bank 
                                          ETM为指定4组ddr的0-7bank
*   MMU开启rotatjon
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_13(ZXIC_UINT32 dev_id, ZXIC_UINT32 ddr_no);


/** QMU/MMU初始化配置场景14：每个tm只用1组*8bank，MMU实际分配ftm:0-9中指定组(0-7bank); etm:0-9中指定组(0-7bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/4/14
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_14(ZXIC_UINT32 dev_id, ZXIC_UINT32 etm_ddr_no, ZXIC_UINT32 ftm_ddr_no);


/** QMU/MMU初始化配置场景14：每个tm只用1组*8bank，MMU实际分配ftm:0-9中指定组(0-7bank); etm:0-9中指定组(0-7bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/4/14
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_14(ZXIC_UINT32 dev_id, ZXIC_UINT32 etm_ddr_no);


/** QMU/MMU初始化配置场景14：每个tm只用1组*8bank，MMU实际分配ftm:0-9中指定组(0-7bank); etm:0-9中指定组(0-7bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/4/14
************************************************************/
DPP_STATUS dpp_ftm_qmu_init_set_14(ZXIC_UINT32 dev_id, ZXIC_UINT32 ftm_ddr_no);


/** QMU/MMU初始化配置场景15：每个tm只用1组*4bank，MMU实际分配ftm:0-9中指定组(0123bank); etm:0-9中指定组(0123bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/4/14
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_15(ZXIC_UINT32 dev_id, ZXIC_UINT32 etm_ddr_no, ZXIC_UINT32 ftm_ddr_no);


/** QMU/MMU初始化配置场景15：每个tm只用1组*4bank，MMU实际分配ftm:0-9中指定组(0123bank); etm:0-9中指定组(0123bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/4/14
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_15(ZXIC_UINT32 dev_id, ZXIC_UINT32 etm_ddr_no);


/** QMU/MMU初始化配置场景15：每个tm只用1组*4bank，MMU实际分配ftm:0-9中指定组(0123bank); etm:0-9中指定组(0123bank)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/4/14
************************************************************/
DPP_STATUS dpp_ftm_qmu_init_set_15(ZXIC_UINT32 dev_id, ZXIC_UINT32 ftm_ddr_no);


/***********************************************************/
/** QMU初始化配置场景16：独享2组*8bank，MMU实际分配1,2; etm or ftm使用0-7bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/12/16
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set_16(ZXIC_UINT32 dev_id, ZXIC_UINT32 etm_ddr_no, ZXIC_UINT32 ftm_ddr_no);

/***********************************************************/
/** QMU初始化配置场景16：独享2组*8bank，MMU实际分配1,2; etm or ftm使用0-7bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/12/16
************************************************************/
DPP_STATUS dpp_etm_qmu_init_set_16(ZXIC_UINT32 dev_id, ZXIC_UINT32 ftm_ddr_no);

/***********************************************************/
/** QMU初始化配置场景16：独享2组*8bank，MMU实际分配1,2; etm or ftm使用0-7bank
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/12/16
************************************************************/
DPP_STATUS dpp_ftm_qmu_init_set_16(ZXIC_UINT32 dev_id, ZXIC_UINT32 etm_ddr_no);

/***********************************************************/
/** 64k队列的简单挂接函数配置
*flow0~65535 -> ses0~4095 -> pp0
:具体为flow0~15挂接到ses0的sp0，flow16~31挂接到ses1的sp0，以此类推
*                                  ses到PP的挂接都是挂接于pp0的sp0
*@param   dev_id   设备编号
*         port_id端口号
* @
* @return
* @remark  无
* @see
* @author  cy      @date  2015/06/30
************************************************************/
DPP_STATUS dpp_tm_crdt_sch_64k_test(ZXIC_UINT32 dev_id, ZXIC_UINT32 port_id);


/***********************************************************/
/** 把整数分解成(指定位长)最高有效数和(2的)指数位数的形式，data=p_remdata*2^(p_exp)
* @param   data   需要转换前的数
* @param   rembitsum   余数的位数
* @param   p_remdata   余数大小
* @param   p_exp   指数大小
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/27
************************************************************/
DPP_STATUS dpp_tm_rem_and_exp_translate(ZXIC_UINT32 data,
                                        ZXIC_UINT32 rembitsum,
                                        ZXIC_UINT32 *p_remdata,
                                        ZXIC_UINT32 *p_exp);

/***********************************************************/
/** 配置统计计数寄存器的统计方式
* @param   dev_id   设备编号
* @param   qnum   支持2组统计寄存器，0-qnum1，1-qnum2
* @param   mode   统计方式，0-统计全部队列，1-统计指定q_id
* @param   q_id   mode=1时生效，统计该队列的信息
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_cgavd_stat_q_set(ZXIC_UINT32 dev_id,
                                   DPP_ETM_CGAVD_STAT_QNUM_E qnum,
                                   DPP_ETM_CGAVD_STAT_MODE_E mode,
                                   ZXIC_UINT32 q_id);

/***********************************************************/
/** 读取统计计数寄存器的计数信息
* @param   dev_id   设备编号
* @param   qnum   支持2组统计寄存器，0-qnum1，1-qnum2
* @param   p_para   获得的统计信息
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_cgavd_stat_q_get(ZXIC_UINT32 dev_id,
                                   DPP_ETM_CGAVD_STAT_QNUM_E qnum,
                                   DPP_ETM_CGAVD_STAT_INFO_T *p_para);

/***********************************************************/
/** 配置等价包长
* @param   dev_id
* @param   tm_type   0-ETM,1-FTM
* @param   p_equal_pkt_len 等价包长
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/28
************************************************************/
DPP_STATUS dpp_tm_cgavd_equal_pkt_len_para_set(DPP_DEV_T *dev,
                                               DPP_ETM_EQUAL_PKT_LEN_PARA_T *p_equal_pkt_len);

/***********************************************************/
/** 读取等价包长
* @param   dev_id
* @param   tm_type   0-ETM,1-FTM
* @param   p_equal_pkt_len 等价包长
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/28
************************************************************/
DPP_STATUS dpp_tm_cgavd_equal_pkt_len_para_get(DPP_DEV_T *dev,
                                               DPP_ETM_EQUAL_PKT_LEN_PARA_T *p_equal_pkt_len);

/***********************************************************/
/** 动态门限放大因子参数配置
* @param   dev_id
* @param   tm_type   0-ETM,1-FTM
* @param   p_amplify_gene_para 动态门限放大因子参数
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/28
************************************************************/
DPP_STATUS dpp_tm_cgavd_amplify_gene_para_set(DPP_DEV_T *dev,
                                              DPP_ETM_AMPLIFY_GENE_PARA_T *p_amplify_gene_para);


/***********************************************************/
/** 动态门限放大因子参数获取
* @param   dev_id
* @param   tm_type   0-ETM,1-FTM
* @param   p_amplify_gene_para 动态门限放大因子参数
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/28
************************************************************/
DPP_STATUS dpp_tm_cgavd_amplify_gene_para_get(DPP_DEV_T *dev,
                                              DPP_ETM_AMPLIFY_GENE_PARA_T *p_amplify_gene_para);

/***********************************************************/
/** 配置等价包长阈值
* @param   dev_id
* @param   tm_type   0-ETM,1-FTM
* @param   p_equal_pkt_len_th 等价包长阈值
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/28
************************************************************/
DPP_STATUS dpp_tm_cgavd_equal_pkt_len_th_para_set(DPP_DEV_T *dev,
                                                  DPP_ETM_EQUAL_PKT_LEN_TH_PARA_T *p_equal_pkt_len_th);

/***********************************************************/
/** 配置QMU DDR BANK随机模式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_ddr_random   模式:0-轮询模式；1-随机模式
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_ddr_random_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_ddr_random);

/***********************************************************/
/** 读取等价包长阈值
* @param   dev_id
* @param   tm_type   0-ETM,1-FTM
* @param   p_equal_pkt_len_th 等价包长阈值
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/28
************************************************************/
DPP_STATUS dpp_tm_cgavd_equal_pkt_len_th_para_get(DPP_DEV_T *dev,
                                                  DPP_ETM_EQUAL_PKT_LEN_TH_PARA_T *p_equal_pkt_len_th);



/***********************************************************/
/** 读取全部队列的计数信息
* @param   dev_id   设备编号*
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark   使用第2组统计寄存器进行统计
* @see
* @author    yjd    @date  2015/07/04
************************************************************/
DPP_STATUS dpp_tm_cgavd_stat_q_all_get_diag(ZXIC_UINT32 dev_id);

/***********************************************************/
/** 读取某一队列统计计数寄存器的计数信息
* @param   dev_id   设备编号
* @param   q_id   统计该队列的信息
*          使用第1组统计寄存器进行统计
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/04
************************************************************/
DPP_STATUS dpp_tm_cgavd_stat_q_single_get_diag(ZXIC_UINT32 dev_id);



/***********************************************************/
/**  配置默认队列使能
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   en   配置的值，0-不使能默认队列，1-使能默认队列，
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush     @date  2016/08/16
************************************************************/
DPP_STATUS dpp_tm_cgavd_default_queue_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 en);

/***********************************************************/
/**  读取默认队列使能
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_en   读取的值，0-不使能默认队列，1-使能默认队列，
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush     @date  2016/08/16
************************************************************/
DPP_STATUS dpp_tm_cgavd_default_queue_en_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_en);


/***********************************************************/
/**  配置默认队列起始末尾
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   def_start_que  起始默认队列block/byte单位
* @param   def_finish_que  结束默认队列block/byte单位
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush     @date  2016/08/16
************************************************************/
DPP_STATUS dpp_tm_cgavd_default_queue_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 def_start_queue, ZXIC_UINT32 def_finish_queue);

/***********************************************************/
/**  读取默认队列起始末尾值
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_def_start_que   默认队列起始值block/byte单位
* @param   p_def_finish_que   默认队列结束值block/byte单位
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2016/08/16
************************************************************/
DPP_STATUS dpp_tm_cgavd_default_queue_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_def_start_queue, ZXIC_UINT32 *p_def_finish_queue);

/***********************************************************/
/**  配置协议队列使能
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   en   配置的值，0-不使能默认队列，1-使能默认队列，
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2016/08/16
************************************************************/
DPP_STATUS dpp_tm_cgavd_protocol_queue_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 en);

/***********************************************************/
/**  读取协议队列使能
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_en   读取的值，0-不使能通用门限，1-使能通用门限，
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2016/08/16
************************************************************/
DPP_STATUS dpp_tm_cgavd_protocol_queue_en_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_en);

/***********************************************************/
/**  配置协议队列起始末尾
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   protocol_start_que  起始协议队列block/byte单位
*@param   protocol_-finish_que 末尾协议队列block/byte单位
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2016/08/16
************************************************************/
DPP_STATUS dpp_tm_cgavd_protocol_queue_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 protocol_start_que, ZXIC_UINT32 protocol_finish_que);

/***********************************************************/
/**  读取协议队列起始末尾值
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_protocol_start_que   协议认队列起始值block/byte单位
* @param   p_protocol_finish_que   协议认队列末尾值block/byte单位
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush     @date  2016/08/16
************************************************************/
DPP_STATUS dpp_tm_cgavd_protocol_queue_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_protocol_start_que, ZXIC_UINT32 *p_protocol_finish_que);


/***********************************************************/
/** 配置QMU需要统计的队列组
* @param   dev_id   设备号
* @param   tm_type   0-ETM,1-FTM
* @param   batch_id   队列组
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/05/07
************************************************************/
DPP_STATUS dpp_tm_qmu_observe_batch_set(DPP_DEV_T *dev, ZXIC_UINT32 batch_id);

/***********************************************************/
/** 配置QMU需要统计的队列号
* @param   dev_id   设备号
* @param   tm_type   0-ETM,1-FTM
* @param   q_id   队列号
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/05/07
************************************************************/
DPP_STATUS dpp_tm_qmu_observe_qnum_set(DPP_DEV_T *dev, ZXIC_UINT32 q_id);

/***********************************************************/
/** 配置授权盈余初始化值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   crbal_initial_value   授权盈余初始化值
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_crbal_initial_value_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 crbal_initial_value);

/***********************************************************/
/**  配置等价包长使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   en   使能标记，0-不使能，1-使能
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/28
************************************************************/
DPP_STATUS dpp_tm_cgavd_equal_pkt_len_en_set(DPP_DEV_T *dev, ZXIC_UINT32 en);

/***********************************************************/
/**  读取等价包长使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_en   使能标记，0-不使能，1-使能
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/28
************************************************************/
DPP_STATUS dpp_tm_cgavd_equal_pkt_len_en_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_en);


/***********************************************************/
/**  读取指定队列获得授权个数(只打印授权非零的队列号)
* @param   dev_id   设备编号
* @param   ackflow_start   授权起始流号
* @param   ackflow_end   授权终止流号
* @param   sleep_time_ms   等待时间
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2017/01/19
************************************************************/
DPP_STATUS dpp_etm_crdt_traffic_diag(ZXIC_UINT32 dev_id,
                                     ZXIC_UINT32 ackflow_start,
                                     ZXIC_UINT32 ackflow_end,
                                     ZXIC_UINT32 sleep_time_ms);

/***********************************************************/
/**  读取指定队列获得授权个数(只打印授权非零的队列号)
* @param   dev_id   设备编号
* @param   ackflow_start   授权起始流号
* @param   ackflow_end   授权终止流号
* @param   sleep_time_ms   等待时间
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2017/01/19
************************************************************/
DPP_STATUS dpp_ftm_crdt_traffic_diag(ZXIC_UINT32 dev_id,
                                     ZXIC_UINT32 ackflow_start,
                                     ZXIC_UINT32 ackflow_end,
                                     ZXIC_UINT32 sleep_time_ms);


/***********************************************************/
/** 读取指定队列获得授权个数(只打印授权非零的队列号)
* @param   dev_id   设备编号
* @param   tm_type   0-ETM, 1FTM
* @param   ackflow_start   授权流起始
* @param   ackflow_end   授权流终止
* @param   sleep_time_ms   等待时间
*
* @return
* @remark  无
* @see
* @author  wush      @date  2017/01/24
************************************************************/
DPP_STATUS diag_dpp_tm_crdt_traffic_diag(ZXIC_UINT32 dev_id,
                                         ZXIC_UINT32 ackflow_start,
                                         ZXIC_UINT32 ackflow_end,
                                         ZXIC_UINT32 sleep_time_ms);

/***********************************************************/
/**
* @param   dev_id
* @param   tm_type
* @param   i_or_e_sel
* @param   port_or_dest_id_sel
* @param   start_id
* @param   start_port_dest_id
* @param   num
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xjw      @date  2018/02/01
************************************************************/
DPP_STATUS dpp_tm_olif_stat_set_mul(ZXIC_UINT32 dev_id,
                                    ZXIC_UINT32 i_or_e_sel,
                                    ZXIC_UINT32 port_or_dest_id_sel,
                                    ZXIC_UINT32 start_id,
                                    ZXIC_UINT32 start_port_dest_id,
                                    ZXIC_UINT32 num);


/******************************************************************************
 *                               END: 函数声明                                *
 *****************************************************************************/
#endif


#if ZXIC_REAL("ETM_STAT")


/***********************************************************/
/** 打印dpp tm所有模块中断状态
* @param   tm_type   0-ETM,1-FTM
* @param   dev_id   设备编号
* @param
* @param
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/09
************************************************************/
DPP_STATUS diag_dpp_tm_int(ZXIC_UINT32 dev_id);

/***********************************************************/
/** 获取tm.c中qmu_init_set中配置的case_num
* @param   tm_type   0-ETM,1-FTM
* @param   dev_id   设备编号
* @param
* @param
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/04/15
************************************************************/
DPP_STATUS dpp_tm_case_no_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *case_no);

/***********************************************************/
/** 配置dpp tm所有模块中断屏蔽
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   int_mask_flag    0:不屏蔽   1:屏蔽
* @param
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/09
************************************************************/
DPP_STATUS dpp_tm_int_mask_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 int_mask_flag);

/***********************************************************/
/**  读取基于优先级的QMU接收NPPU数据的fifo阈值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   sp  优先级0~7
* @param   p_th  指定优先级的fifo阈值0~511，单位为fifo条目，fifo深度512
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2016/10/18
************************************************************/
DPP_STATUS dpp_tm_cgavd_move_drop_sp_th_get(ZXIC_UINT32 dev_id,
                                            ZXIC_UINT32 sp,
                                            ZXIC_UINT32 *p_th);

/***********************************************************/
/** 打印cgavd模式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_mode   0:block mode 1:byte mode
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/07/29
************************************************************/
DPP_STATUS dpp_tm_cgavd_cfg_mode_get_diag(ZXIC_UINT32 dev_id);


/***********************************************************/
/** 打印cgavd TD阈值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   id   队列号或端口号，系统级时，id参数无效
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/08/01
************************************************************/
DPP_STATUS dpp_tm_cgavd_td_byte_block_th_get_diag(ZXIC_UINT32 dev_id,
                                                  DPP_TM_CGAVD_LEVEL_E level,
                                                  ZXIC_UINT32 id);

/***********************************************************/
/** QMU MMU 配置清除
* @param   dev_id   
* @param   tm_type   
*
* @return  
* @remark  无
* @see     
* @author  XXX      @date  2020/04/13
************************************************************/
DPP_STATUS dpp_tm_qmu_mmu_cfg_clr(ZXIC_UINT32 dev_id);

/***********************************************************/
/** 读取QMU所有队列的统计信息
* @param   dev_id   设备编号
* @param   p_para   获得的统计信息
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_stat_get(ZXIC_UINT32 dev_id, DPP_ETM_QMU_STAT_INFO_T *p_para);

/***********************************************************/
/** 读取QMU指定队列的计数信息
* @param   dev_id   设备编号
* @param   p_para   获得的统计信息
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_spec_q_stat_get(ZXIC_UINT32 dev_id, DPP_ETM_QMU_SPEC_Q_STAT_INFO_T *p_para);

/***********************************************************/
/** 读取QMU指定队列组的计数信息
* @param   dev_id   设备编号
* @param   p_para   获得的统计信息
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_spec_bat_stat_get(ZXIC_UINT32 dev_id, DPP_ETM_QMU_SPEC_BAT_STAT_INFO_T *p_para);

/***********************************************************/
/** 配置QMU流控计数模式
* @param   dev_id   设备号
* @param   tm_type   0-ETM,1-FTM
* @param   mode   流控模式，0-电平流控；1-边沿流控
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/05/07
************************************************************/
DPP_STATUS dpp_tm_qmu_fc_cnt_mode_set(DPP_DEV_T *dev, ZXIC_UINT32 mode);

/***********************************************************/
/** 读取QMU流控计数模式
* @param   dev_id   设备号
* @param   tm_type   0-ETM,1-FTM
* @param   p_mode   流控模式，0-电平流控；1-边沿流控
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/05/07
************************************************************/
DPP_STATUS dpp_tm_qmu_fc_cnt_mode_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_mode);

/***********************************************************/
/** 打印block长度
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/20
************************************************************/
DPP_STATUS dpp_tm_cfgmt_blk_size_get_diag(ZXIC_UINT32 dev_id);


/***********************************************************/
/** 打印队列空标志查询
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   qnum   配置的队列号
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  cy      @date  2016/06/20
************************************************************/
DPP_STATUS dpp_tm_qlist_ept_flag_get_diag(ZXIC_UINT32 dev_id, ZXIC_UINT32 qnum);


/***********************************************************/
/** 获得队列空标志查询
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   qnum   配置的队列号
* @param   p_value   队列空标志查询
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  cy      @date  2016/06/20
************************************************************/
DPP_STATUS dpp_tm_qlist_ept_flag_get(ZXIC_UINT32 dev_id,
                                     ZXIC_UINT32 qnum,
                                     ZXIC_UINT32 *p_value);


/***********************************************************/
/** 获得队列深度计数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   qnum   配置的队列号
* @param   p_value  队列深度计数
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  cy      @date  2016/06/20
************************************************************/
DPP_STATUS dpp_tm_qlist_r_bcnt_get(ZXIC_UINT32 dev_id,
                                   ZXIC_UINT32 qnum,
                                   ZXIC_UINT32 *p_value);


/***********************************************************/
/** 打印队列深度计数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   qnum   配置的队列号
* @param   p_value  队列深度计数
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  cy      @date  2016/06/20
************************************************************/
DPP_STATUS dpp_tm_qlist_r_bcnt_get_diag(ZXIC_UINT32 dev_id, ZXIC_UINT32 qnum);

/***********************************************************/
/** CMDSCH中分端口分优先级的BLOCK计数
* @param   dev_id   设备编号
* @param   pri   
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  sun      @date  2023/09/19
************************************************************/
DPP_STATUS dpp_tm_csch_r_block_cnt_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 pri, ZXIC_UINT32 *p_value);

/***********************************************************/
/** 打印CMDSCH中分端口分优先级的BLOCK计数
* @param   dev_id   设备编号
* @param   port   
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  sun      @date  2023/09/19
************************************************************/
DPP_STATUS dpp_tm_csch_r_block_cnt_diag(ZXIC_UINT32 dev_id, ZXIC_UINT32 port);

/***********************************************************/
/** 打印队列入链状态
* @param   dev_id   设备编号
* @param   flow_id   
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  sun      @date  2023/09/19
************************************************************/
DPP_STATUS dpp_tm_crdt_flow_link_state_get_diag(ZXIC_UINT32 dev_id, ZXIC_UINT32 flow_id);

/***********************************************************/
/** 打印调度器入链状态
* @param   dev_id   设备编号
* @param   flow_id   
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  sun      @date  2023/09/19
************************************************************/
DPP_STATUS dpp_tm_crdt_se_link_state_get_diag(ZXIC_UINT32 dev_id, ZXIC_UINT32 se_id);

/***********************************************************/
/** 打印olif的fifo是否空状态
* @param   dev_id   设备编号
* @param      
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  sun      @date  2023/09/19
************************************************************/
DPP_STATUS dpp_tm_olif_fifo_empty_state_get_diag(ZXIC_UINT32 dev_id);

/***********************************************************/
/** 授权个数统计寄存器清零
* @param   dev_id  设备编号
* @param   tm_type   0-ETM,1-FTM
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  taq      @date  2015/06/19
************************************************************/
DPP_STATUS dpp_tm_crdt_clr_diag(ZXIC_UINT32 dev_id);

/***********************************************************/
/**cpu配置flow_id的crs强制为normal或者off开关使能，用于检测SA模式下队列到授权流的多对一问题
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   flow_id:流号(和授权流号一一对应)
*                en   强制配置crs的使能，0-不使能，1-使能
*                crs_value:强制配置crs的值2'b00:off; 2'b01:low; 2'b10:normal；
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/04/25
************************************************************/
DPP_STATUS dpp_tm_crdt_crs_sheild_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 flow_id, ZXIC_UINT32 en, ZXIC_UINT32 crs_value);


/***********************************************************/
/**获取flow_id的crs强制为normal或者off开关使能，用于检测SA模式下队列到授权流的多对一问题
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   flow_id:流号(和授权流号一一对应)
*                en   强制配置crs的使能，0-不使能，1-使能
*                crs_value:强制配置crs的值2'b00:off; 2'b01:low; 2'b10:normal；
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/04/25
************************************************************/
DPP_STATUS dpp_tm_crdt_crs_sheild_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_flow_id, ZXIC_UINT32 *p_en, ZXIC_UINT32 *p_crs_value);


/***********************************************************/
/** 获得拥塞状态(有效链路数+UCN等级)到授权产生间隔的映射表
* @param   dev_id   设备编号
* @param   valid_serdes_num   有效的链路数(0~32
* @param   ucn_level   UCN等级(0~7)
* @param   p_cr_clk   UCN等级(0~7)
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2017/03/28
************************************************************/
DPP_STATUS dpp_tm_crdt_cfgmt_interval_get(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 valid_serdes_num,
                                          ZXIC_UINT32 ucn_level,
                                          ZXIC_UINT32 *p_cr_clk);

/***********************************************************/
/**每隔10s获取crs状态的个数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param
* @return
* @remark  无
* @see
* @author  zmy     @date  2015/08/07
************************************************************/
DPP_STATUS dpp_tm_crs_statics(ZXIC_UINT32 dev_id, ZXIC_UINT32 que_id);


/***********************************************************/
/** 统计QMU发送和CRDT模块指定授权流接收的CRS计数(10s内)
* @param   dev_id  设备编号
* @param   que_id   QMU队列号
* @param   ackflow_id   授权流号
* @param   valid_flag   0:队列发送和授权流接收都统计; 1:只关注队列发送，2:只关注授权流接收。
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2017/05/12
************************************************************/
DPP_STATUS dpp_tm_crs_cnt_prt(ZXIC_UINT32 dev_id, ZXIC_UINT32 que_id, ZXIC_UINT32 ackflow_id, ZXIC_UINT32 valid_flag);


/***********************************************************/
/** 带停流的统计QMU发送和CRDT模块指定授权流接收的CRS计数
* @param   dev_id  设备编号
* @param   que_id   QMU队列号
* @param   ackflow_id   授权流号
* @param   valid_flag   0:默认队列发送和授权流接收都统计,此时队列授权都在本板;
*                       1:只关注队列发送，2:只关注授权流接收，需要与源端队列停流配合使用，
                        先停流，运行该函数；或者直接不停流得到的是某段时间的计数。
* @param   sleep_time   统计多长时间内的crs计数
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2017/05/12
************************************************************/
DPP_STATUS dpp_tm_crs_cnt_prt_1(ZXIC_UINT32 dev_id,
                                ZXIC_UINT32 que_id,
                                ZXIC_UINT32 ackflow_id,
                                ZXIC_UINT32 valid_flag,
                                ZXIC_UINT32 sleep_time);

/***********************************************************/
/** 读取qlist入队及出队状态监控
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2015/08/26
************************************************************/
DPP_STATUS dpp_tm_qmu_qlist_state_query(ZXIC_UINT32 dev_id);


/***********************************************************/
/** 打印被指定统计的第0~15个端口消耗的令牌个数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM

* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  统计时间为2s，其中c桶统计1s，e桶统计1s
* @see
* @author  whuashan      @date  2019/03/15
************************************************************/
DPP_STATUS dpp_tm_shape_token_dec_cnt_diag(ZXIC_UINT32 dev_id);

/***********************************************************/
/** 打印被指定统计的第0~15个端口接收的令牌个数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  统计时间为2s，其中c桶统计1s，e桶统计1s
* @see
* @author  whuashan      @date  2019/03/15
************************************************************/
DPP_STATUS dpp_tm_shape_token_dist_cnt_diag(ZXIC_UINT32 dev_id);


/***********************************************************/
/** 配置olif统计组信息
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   id   olif统计组号
* @param   all_or_by_port   0-统计所有，1-统计某一端口或某一dest_id
* @param   i_or_e_sel   10-统计片外，01-统计片内，其他值-统计所有
* @param   port_or_dest_id_sel   0-统计port，1-统计dest_id
* @param   port_dest_id   port号或dest_id号
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/04/21
************************************************************/
DPP_STATUS dpp_tm_olif_stat_set(ZXIC_UINT32 dev_id,
                                ZXIC_UINT32 id,
                                ZXIC_UINT32 all_or_by_port,
                                ZXIC_UINT32 i_or_e_sel,
                                ZXIC_UINT32 port_or_dest_id_sel,
                                ZXIC_UINT32 port_dest_id);


/***********************************************************/
/** 查看CRDT接收到和发送的某端口拥塞授权总数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   pp_id   0~63
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/12/08
************************************************************/
DPP_STATUS diag_dpp_tm_crdt_port_congest_credit_cnt(ZXIC_UINT32 dev_id, ZXIC_UINT32 pp_id);

#endif /*ETM_STAT */


#if ZXIC_REAL("TM_REG")
/***********************************************************/
/** 写TM寄存器
* @param   module_id 区分TM子模块
* @param   addr   基于子模块的地址
* @param   data   写入的数据
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/26
************************************************************/
DPP_STATUS dpp_tm_wr_reg(ZXIC_UINT32 dev_id, ZXIC_UINT32 module_id, ZXIC_UINT32 addr, ZXIC_UINT32 data);


/***********************************************************/
/** 读TM寄存器
* @param   module_id 区分TM子模块
* @param   addr   基于子模块的地址
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/26
************************************************************/
DPP_STATUS dpp_tm_rd_reg(ZXIC_UINT32 dev_id, ZXIC_UINT32 module_id, ZXIC_UINT32 addr);


/***********************************************************/
/** 读一片连续的TM寄存器
* @param   module_id 区分TM子模块
* @param   first_addr   起始寄存器的地址
* @param   reg_num   总共读取的寄存器数
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2017/07/26
************************************************************/
DPP_STATUS dpp_tm_rd_more_reg(ZXIC_UINT32 dev_id, ZXIC_UINT32 module_id, ZXIC_UINT32 first_addr, ZXIC_UINT32 reg_num);

/***********************************************************/
/** 写tm模块二层间接寄存器(仅crdt/shap模块使用)
* @param   module_id 区分TM子模块
* @param   addr   基于子模块的地址
* @param   data   写入的数据
*
* @return
* @remark  无
* @see
* @author  whuashan      @date  2019/02/25
************************************************************/
DPP_STATUS dpp_tm_ind_wr_reg(ZXIC_UINT32 dev_id, ZXIC_UINT32 module_id, ZXIC_UINT32 addr, ZXIC_UINT64 data);

/***********************************************************/
/** 读tm模块二层间接寄存器(仅crdt/shap模块使用)
* @param   module_id 区分TM子模块
* @param   addr   基于子模块的地址
*
* @return
* @remark  无
* @see
* @author  whuashan      @date  2019/02/25
************************************************************/
DPP_STATUS dpp_tm_ind_rd_reg(ZXIC_UINT32 dev_id, ZXIC_UINT32 module_id, ZXIC_UINT32 addr);
#endif /*TM_REG */

#if ZXIC_REAL("TM_CFGMT")
/***********************************************************/
/** 校验子系统初始化就绪，所有子系统均初始化就绪，p_rdy值为1
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_rdy   初始化就绪标记，1-就绪，0-未就绪
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_tm_cfgmt_subsystem_rdy_check(ZXIC_UINT32 dev_id);


/***********************************************************/
/** cpu读写通道验证，其读出值等于写入值。读出值不等于写入值时，返回err
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_tm_cfgmt_cpu_check(DPP_DEV_T *dev);


/***********************************************************/
/** 读取内置TM的工作模式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_mode   读取的值，0-TM模式，1-SA模式
*ETM仅工作在TM模式，FTM可以工作TM或SA模式
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/25
************************************************************/
DPP_STATUS dpp_tm_cfgmt_sa_work_mode_get(DPP_DEV_T *dev, DPP_TM_WORK_MODE_E *p_mode);


/***********************************************************/
/** 配置ddr3挂接组数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   ddr_num   ddr组数，1-6组
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_tm_cfgmt_ddr_attach_set(DPP_DEV_T *dev, ZXIC_UINT32 ddr_num);


/***********************************************************/
/** 读取ddr3挂接组数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_ddr_num   ddr组数,1-6组
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_tm_cfgmt_ddr_attach_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_ddr_num);

DPP_STATUS dpp_qmu_init_info(ZXIC_UINT32 dev_id);

/***********************************************************/
/** 配置包存储的CRC功能是否使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   en   配置的值，0-禁止CRC功能，1-允许CRC功能
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/25
************************************************************/
DPP_STATUS dpp_tm_cfgmt_crc_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 en);


/***********************************************************/
/** 配置qmu端口转换使能，SA模式下需要配置
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   en   配置的使能值，0-不使能，1-使能
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/25
************************************************************/
DPP_STATUS dpp_tm_cfgmt_port_transfer_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 en);


/***********************************************************/
/** 读取qmu端口转换使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_en   读取的使能值，0-不使能，1-使能
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/25
************************************************************/
DPP_STATUS dpp_tm_cfgmt_port_transfer_en_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_en);


/***********************************************************/
/** 读取包存储的CRC功能是否使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_en   读取的值，0-禁止CRC功能，1-允许CRC功能
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/25
************************************************************/
DPP_STATUS dpp_tm_cfgmt_crc_en_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_en);


/***********************************************************/
/** 配置block长度模式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   size   block长度模式:256/512/1024
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_tm_cfgmt_blk_size_set(DPP_DEV_T *dev, ZXIC_UINT32 size);


/***********************************************************/
/** 配置计数模式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_mode   计数模式
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_tm_cfgmt_cnt_mode_set(ZXIC_UINT32 dev_id, DPP_TM_CNT_MODE_T *p_mode);


/***********************************************************/
/** 读取计数模式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_mode   计数模式
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_tm_cfgmt_cnt_mode_get(ZXIC_UINT32 dev_id, DPP_TM_CNT_MODE_T *p_mode);


/***********************************************************/
/** 配置中断屏蔽
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_para   中断屏蔽
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_tm_cfgmt_int_mask_set(ZXIC_UINT32 dev_id, DPP_TM_INT_T *p_para);


/***********************************************************/
/** 读取中断屏蔽
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_para   中断屏蔽
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_tm_cfgmt_int_mask_get(ZXIC_UINT32 dev_id, DPP_TM_INT_T *p_para);


/***********************************************************/
/** 读取中断状态
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_para   中断状态
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_tm_cfgmt_int_state_get(ZXIC_UINT32 dev_id, DPP_TM_INT_T *p_para);

/***********************************************************/
/** 配置tm时钟门控是否使能
* @param   dev_id   设备编号
* @param   en   配置的值，0-禁止tm时钟门控，1-使能tm时钟门控
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  lsy      @date  2022/08/22
************************************************************/
DPP_STATUS dpp_tm_cfgmt_clkgate_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 en);

/***********************************************************/
/** 读取tm时钟门控是否使能
* @param   dev_id   设备编号
* @param   en   配置的值，0-禁止tm时钟门控，1-使能tm时钟门控
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  lsy      @date  2022/08/22

************************************************************/
DPP_STATUS dpp_tm_cfgmt_clkgate_en_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_en);

/***********************************************************/
/** 配置tm软复位是否使能
* @param   dev_id   设备编号
* @param   en   配置的值，0-禁止tm软复位，1-使能tm软复位
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  lsy      @date  2022/08/22
************************************************************/
DPP_STATUS dpp_tm_cfgmt_softrst_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 en);

/***********************************************************/
/** 读取tm软复位是否使能
* @param   dev_id   设备编号
* @param   en   配置的值，0-禁止tm软复位，1-使能tm软复位
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  lsy      @date  2022/08/22

************************************************************/
DPP_STATUS dpp_tm_cfgmt_softrst_en_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_en);

#endif /*TM_CFGMT */

#if ZXIC_REAL("TM_CGAVD")
/***********************************************************/
/**  读取各级搬移功能使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   要配置的拥塞避免层次号，0:队列级，1:端口级，2:系统级
* @param   p_en   读出的使能标记，0-不使能，1-使能
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2016/10/18
************************************************************/
#ifdef ETM_REAL
DPP_STATUS dpp_tm_cgavd_move_en_get(ZXIC_UINT32 dev_id,
                                    DPP_TM_CGAVD_LEVEL_E level,
                                    ZXIC_UINT32 *p_en);

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
/** 读取各级搬移门限
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   id   队列号或端口号，系统级时，id参数无效
* @param   p_value   端口级和系统级时，为搬移门限值，单位为NPPU存包的单位，256B；
                     流级时为搬移profile_id,0~15
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2016/10/18
************************************************************/
DPP_STATUS dpp_tm_cgavd_move_th_get(ZXIC_UINT32 dev_id,
                                    DPP_TM_CGAVD_LEVEL_E level,
                                    ZXIC_UINT32 id,
                                    ZXIC_UINT32 *p_value);

/***********************************************************/
/**  读取flow级的搬移策略
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   move_profile  flow级的搬移门限分组索引,0~15
* @param   p_th  flow级的搬移门限，单位为KB；
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2016/10/18
************************************************************/
DPP_STATUS dpp_tm_cgavd_flow_move_profile_get(ZXIC_UINT32 dev_id,
                                              ZXIC_UINT32 move_profile,
                                              ZXIC_UINT32 *p_th);
#endif
/***********************************************************/
/**  读取端口共享的搬移门限
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_th  端口共享的搬移门限，单位为NPPU存包的单位，256B；
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2016/10/18
************************************************************/
DPP_STATUS dpp_tm_cgavd_port_share_th_get(ZXIC_UINT32 dev_id,
                                           ZXIC_UINT32 *p_th);

/***********************************************************/
/**  读取各级拥塞避免功能使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   要读取的拥塞避免层次号，0:队列级，1:端口级，2:系统级
* @param   p_en   读出的使能标记，0-不使能，1-使能
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/28
************************************************************/
DPP_STATUS dpp_tm_cgavd_en_get(DPP_DEV_T *dev,
                               DPP_TM_CGAVD_LEVEL_E level,
                               ZXIC_UINT32 *p_en);

/***********************************************************/
/**  dp选取来源
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   要配置的拥塞避免层次号，0:队列级，1:端口级，2:系统级
* @param   dp_sel   dp选取来源，0-dp，1-tc，2-pkt_len[2:0]
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2017/03/14
************************************************************/
DPP_STATUS dpp_tm_cgavd_dp_sel_set(DPP_DEV_T *dev,
                                   DPP_TM_CGAVD_LEVEL_E level,
                                   DPP_TM_CGAVD_DP_SEL_E dp_sel);

/***********************************************************/
/** 读取拥塞避免算法
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   id   队列号或端口号,系统级时,id参数无效
* @param   p_method   配置的拥塞避免算法,0:TD,1:WRED/GRED
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_method_get(DPP_DEV_T *dev,
                                   DPP_TM_CGAVD_LEVEL_E level,
                                   ZXIC_UINT32 id,
                                   DPP_TM_CGAVD_METHOD_E *p_method);


/***********************************************************/
/** 流队列级队列深度的获取
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   que_id      队列号
*          p_len       队列深度以KB为单位
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/06/19
************************************************************/
DPP_STATUS dpp_tm_flow_que_len_get(DPP_DEV_T *dev,
                                   ZXIC_UINT32 que_id,
                                   ZXIC_UINT32 *p_len);

/***********************************************************/
/** 端口级队列深度的获取
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   pp_id      队列号
*          pp_len       队列深度以KB为单位
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/06/19
************************************************************/
DPP_STATUS dpp_tm_port_que_len_get(ZXIC_UINT32 dev_id,
                                   ZXIC_UINT32 pp_id,
                                   ZXIC_UINT32 *pp_len);

/***********************************************************/
/** 系统级队列深度的获取
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   sys_len       系统级深度以block为单位
*          sys_protocol_len       系统级包含协议队列深度以KB为单位
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/06/19
************************************************************/
DPP_STATUS dpp_tm_sys_que_len_get(ZXIC_UINT32 dev_id,
                                  ZXIC_UINT32 *sys_len,
                                  ZXIC_UINT32 *sys_protocol_len);

/***********************************************************/
/** 读取TD拥塞避免模式下的丢弃门限值
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   id   队列号或端口号，系统级时，id参数无效
* @param   p_td_th   配置的丢弃门限值
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_td_th_get(DPP_DEV_T *dev,
                                  DPP_TM_CGAVD_LEVEL_E level,
                                  ZXIC_UINT32 id,
                                  ZXIC_UINT32 *p_td_th);

/***********************************************************/
/** 读取指定端口或队列绑定的WRED GROUP ID
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   WRED支持层次号，0:队列级，1:端口级
* @param   id   队列号或端口号
* @param   p_wred_id   配置的WRED GROUP ID
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_wred_id_get(ZXIC_UINT32 dev_id,
                                    DPP_TM_CGAVD_LEVEL_E level,
                                    ZXIC_UINT32 id,
                                    ZXIC_UINT32 *p_wred_id);

/***********************************************************/
/** 读取WRED丢弃曲线对应的参数
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
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  taq     @date  2015/04/20
************************************************************/
DPP_STATUS dpp_tm_cgavd_wred_dp_line_para_get(ZXIC_UINT32 dev_id,
                                              DPP_TM_CGAVD_LEVEL_E level,
                                              ZXIC_UINT32 wred_id,
                                              ZXIC_UINT32 dp,
                                              DPP_TM_WRED_DP_LINE_PARA_T *p_para);

/***********************************************************/
/** 配置系统级GRED丢弃曲线对应的参数
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   dp   共支持8个dp，取值0-7
* @param   p_para  配置的GRED丢弃曲线参数值，包含以下六个参数
           max_th  平均队列深度上限阈值
           mid_th  平均队列深度中间阈值
           min_th  平均队列深度下限阈值
           max_p   最大丢弃概率
           weight   平均队列深度计算权重
           q_len_th 队列深度阈值
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  taq      @date  2015/04/20
************************************************************/
DPP_STATUS dpp_tm_cgavd_gred_dp_line_para_set(ZXIC_UINT32 dev_id,
                                              ZXIC_UINT32 dp,
                                              DPP_TM_GRED_DP_LINE_PARA_T *p_para);

/***********************************************************/
/** 配置系统级阶梯TD 丢弃曲线对应的参数
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   dp   共支持8个dp，取值0-7
* @param   td_th  TD 门限
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/08/02
************************************************************/
DPP_STATUS dpp_tm_cgavd_ladtd_dp_line_para_set(ZXIC_UINT32 dev_id,
                                               ZXIC_UINT32 dp,
                                               ZXIC_UINT32 td_th);


/***********************************************************/
/** 读取系统级GRED丢弃曲线对应的参数
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   dp   共支持8个dp，取值0-7
* @param   p_para  配置的GRED丢弃曲线参数值，包含以下六个参数
           max_th  平均队列深度上限阈值
           mid_th  平均队列深度中间阈值
           min_th  平均队列深度下限阈值
           max_p   最大丢弃概率
           weight   平均队列深度计算权重
           q_len_th 队列深度阈值
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  taq      @date  2015/04/20
************************************************************/
DPP_STATUS dpp_tm_cgavd_gred_dp_line_para_get(ZXIC_UINT32 dev_id,
                                              ZXIC_UINT32 dp,
                                              DPP_TM_GRED_DP_LINE_PARA_T *p_para);

/***********************************************************/
/** 读取指定端口或队列是否支持动态门限机制
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   WRED支持层次号，0:队列级，1:端口级
* @param   id   队列号或端口号
* @param   p_en   读取的值，0-不支持动态门限机制，1-支持动态门限机制
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_dyn_th_en_get(DPP_DEV_T *dev,
                                      DPP_TM_CGAVD_LEVEL_E level,
                                      ZXIC_UINT32 id,
                                      ZXIC_UINT32 *p_en);

/***********************************************************/
/**  配置通用门限使能
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   en   配置的值，0-不使能通用门限，1-使能通用门限，
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_uniform_th_en_set(DPP_DEV_T *dev, ZXIC_UINT32 en);

/***********************************************************/
/**  读取通用门限使能
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_en   读取的值，0-不使能通用门限，1-使能通用门限，
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_uniform_th_en_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_en);

/***********************************************************/
/**  配置通用门限
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   th   通用门限值
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_uniform_th_set(DPP_DEV_T *dev, ZXIC_UINT32 th);

/***********************************************************/
/**  读取通用门限
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_th   通用门限值
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_uniform_th_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_th);

/***********************************************************/
/**  配置流队列所属优先级
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   q_id   队列号
* @param   pri   配置的优先级，0~7
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/17
************************************************************/
DPP_STATUS dpp_tm_cgavd_q_pri_set(DPP_DEV_T *dev,
                                  ZXIC_UINT32 q_id,
                                  ZXIC_UINT32 pri);

/***********************************************************/
/** 读取TM模式下流队列挂接的端口号；SA模式下流队列映射的目的芯片ID
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   q_id   队列号
* @param   p_pp_id   读取的端口号
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/02/18
************************************************************/
DPP_STATUS dpp_tm_cgavd_q_map_pp_get(DPP_DEV_T *dev,
                                     ZXIC_UINT32 q_id,
                                     ZXIC_UINT32 *p_pp_id);

/***********************************************************/
/** 读取配置TM模式tc到flow的映射
* @param   dev_id 设备编号
* @param   tc_id   itmd tc优先级（0~7）
* @param   flow_id 读取映射的flowid号 （0~4095）
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  sun      @date  2023/07/04
************************************************************/
DPP_STATUS dpp_tm_tc_map_flow_get(DPP_DEV_T *dev,
                                  ZXIC_UINT32 tc_id,
                                  ZXIC_UINT32 *flow_id);

/***********************************************************/
/**  获取强制片内或片外
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   en      1:使能
* @param   mode   1 :omem 强制片外  0:imem 强制片内
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2017/10/14
************************************************************/
DPP_STATUS dpp_tm_cgavd_imem_omem_get(ZXIC_UINT32 dev_id,
                                      ZXIC_UINT32 *p_en,
                                      ZXIC_UINT32 *p_mode);

/***********************************************************/
/** 连续配置各级搬移门限
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   start_id 为起始 队列号或端口号，系统级时，id参数无效
* @param   value   端口级和系统级时，为搬移门限值，单位为NPPU存包的单位，256B；
                   流级时为搬移profile_id,0~15
* @param  num   为队列或端口个数
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2016/11/19
************************************************************/
#ifdef ETM_REAL
DPP_STATUS dpp_tm_cgavd_move_th_together_wr(ZXIC_UINT32 dev_id,
                                            DPP_TM_CGAVD_LEVEL_E level,
                                            ZXIC_UINT32 start_id,
                                            ZXIC_UINT32 value,
                                            ZXIC_UINT32 num);
#endif
/***********************************************************/
/** 系统级缓存使用上下限阈值配置
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   th_h: 系统级缓存使用上限阈值
* @param   th_l: 系统级缓存使用下限阈值
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/07/03
************************************************************/
DPP_STATUS dpp_tm_sys_window_th_set(ZXIC_UINT32 dev_id,
                                    ZXIC_UINT32 th_h,
                                    ZXIC_UINT32 th_l);

/***********************************************************/
/** 配置cgavd强制反压
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   cgavd_fc: 0:不强制反压    1:强制反压
* @param
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/07/03
************************************************************/
DPP_STATUS dpp_tm_cgavd_cfg_fc_set(ZXIC_UINT32 dev_id,
                                   ZXIC_UINT32 cgavd_fc);

/***********************************************************/
/** 获取cgavd强制反压状态
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   cgavd_fc: 0:不强制反压    1:强制反压
* @param
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/07/03
************************************************************/
DPP_STATUS dpp_tm_cgavd_cfg_fc_get(ZXIC_UINT32 dev_id,
                                   ZXIC_UINT32 *cgavd_fc);

/***********************************************************/
/** 配置cgavd强制不反压
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   cgavd_no_fc: 0:不强制     1:强制不反压
* @param
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/07/03
************************************************************/
DPP_STATUS dpp_tm_cgavd_cfg_no_fc_set(ZXIC_UINT32 dev_id,
                                      ZXIC_UINT32 cgavd_no_fc);

/***********************************************************/
/** 获取cgavd强制不反压状态
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   cgavd_no_fc: 0:不强制 1:强制不反压
* @param
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/07/03
************************************************************/
DPP_STATUS dpp_tm_cgavd_cfg_no_fc_get(ZXIC_UINT32 dev_id,
                                      ZXIC_UINT32 *cgavd_no_fc);


/***********************************************************/
/** 配置cgavd平均队列深度归零
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   en: 0:关闭     1:使能
* @param
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/08/05
************************************************************/
DPP_STATUS dpp_tm_cgavd_avg_qlen_return_zero_en_set(ZXIC_UINT32 dev_id,
                                                ZXIC_UINT32 en);
#endif /*TM_CGAVD */

/** crdt ram初始化
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/22
************************************************************/
DPP_STATUS dpp_tm_crdt_ram_init(ZXIC_UINT32 dev_id);

/***********************************************************/
/** 获取调度器类型
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   se_id     调度器编号
* @param   item_num  调度器中包含的子调度器个数
* @param   sch_type_num  调度器类型编号
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/26
************************************************************/
DPP_STATUS dpp_tm_crdt_sch_type_get(DPP_DEV_T *dev, ZXIC_UINT32 se_id, ZXIC_UINT32 *item_num, ZXIC_UINT32 *sch_type_num);

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
DPP_STATUS dpp_tm_crdt_flow_link_wr(DPP_DEV_T *dev,
                                    ZXIC_UINT32 flow_id,
                                    DPP_TM_SCH_FLOW_PARA_T *p_flow_para);

/***********************************************************/
/** 批量配置flow级流队列的挂接关系
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   flow_id_s 起始流队列号
* @param   flow_id_e 终止流队列号
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
DPP_STATUS dpp_tm_crdt_flow_link_more_set(DPP_DEV_T *dev,
                                          ZXIC_UINT32 flow_id_s,
                                          ZXIC_UINT32 flow_id_e,
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
* @param   se_insw    优先级传递使能：0-关 1-开。该参数不传递直接配0
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_se_link_wr(DPP_DEV_T *dev,
                                  ZXIC_UINT32 se_id,
                                  DPP_TM_SCH_SE_PARA_T *p_sch_se_para);


/***********************************************************/
/** 配置调度器层次化QOS的挂接关系:优先级传递
* @param   dev_id       设备编号
* @param   tm_type      0-ETM,1-FTM
* @param   se_id        本级调度器id
* @param   se_linkid    要挂接到的上级调度器id
* @param   se_sp        挂接到上级调度器的sp优先级,有效值[0-3],最多4级，优先级按调度单元分配，
*                       每个调度单元内部调度器优先级相同！
* @param   se_weight0-7 WFQ8中各调度器权重值[1~511]，若是WFQ2/4 只取前面对应值，后面无效
* @param   se_insw      优先级传递使能：0-关 1-开. 该参数不传递直接配1
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_se_link_insw_wr(DPP_DEV_T *dev,
                                       ZXIC_UINT32 se_id,
                                       DPP_TM_SCH_SE_PARA_INSW_T *p_sch_se_para_insw);

/***********************************************************/
/** 获取流队列入链状态
* @param   dev_id      设备编号
* @param   tm_type     0-ETM,1-FTM
* @param   flow_id     流队列号
* @param   link_state  0-未入链     1-在调度器链表中
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_flow_link_state_get(DPP_DEV_T *dev, ZXIC_UINT32 flow_id, ZXIC_UINT32 *link_state);

/***********************************************************/
/** 获取调度器入链状态
* @param   dev_id      设备编号
* @param   tm_type     0-ETM,1-FTM
* @param   se_id       调度器编号
* @param   link_state  0-未入链     1-在调度器链表中
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_se_link_state_get(DPP_DEV_T *dev, ZXIC_UINT32 se_id, ZXIC_UINT32 *link_state);

/***********************************************************/
/** 判断crdt流删除命令是否空闲
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_del_cmd_idle(DPP_DEV_T *dev);

/***********************************************************/
/** 删除流/调度器挂接关系
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   id        要删除的流号或调度器id
*          ETM范围:0--0xABFF; FTM范围:0-0x177F
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/02/27
************************************************************/
DPP_STATUS dpp_tm_crdt_del_link_set(DPP_DEV_T *dev, ZXIC_UINT32 id);




/***********************************************************/
/** 获取pp->dev挂接关系
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   pp_id   0~63
* @param   p_weight  0~127
* @param   p_sp_mapping   0~7
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/04/20
************************************************************/
DPP_STATUS dpp_tm_crdt_pp_para_get(DPP_DEV_T *dev,
                                   ZXIC_UINT32 pp_id,
                                   ZXIC_UINT32 *p_weight,
                                   ZXIC_UINT32 *p_sp_mapping);

/***********************************************************/
/**
* @param   dev_id   设备编号
* @param   tm_type  0-ETM,1-FTM
* @param   que_id_s 起始队列号
* @param   que_id_e 终止队列号
* @param   en       1:过滤E桶队列CRS状态为SLOW的入链请求；0:E桶队列CRS SLOW正常入链；
*
* @return
* @remark  无
* @see
* @author  XXX      @date  2019/05/08
************************************************************/
DPP_STATUS dpp_tm_crdt_eir_crs_filter_en_more_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 que_id_s, ZXIC_UINT32 que_id_e, ZXIC_UINT32 en);


/***********************************************************/
/**
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   que_id   queue id
* @param   p_en
*
* @return
* @remark  无
* @see
* @author  XXX      @date  2019/05/08
************************************************************/
DPP_STATUS dpp_tm_crdt_eir_crs_filter_en_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 que_id, ZXIC_UINT32 *p_en);

/***********************************************************/
/**
* @param   dev_id
* @param   tm_type
* @param   que_id
*
* @return
* @remark  无
* @see
* @author  XXX      @date  2019/05/08
************************************************************/
DPP_STATUS dpp_tm_crdt_eir_crs_filter_en_get_diag(ZXIC_UINT32 dev_id, ZXIC_UINT32 que_id);


/***********************************************************/
/** 打印指定的全局数组值以及清空全局数组
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   para_x   数组index_x
* @param   para_y   数组index_y
* @param   clear_flag 清空shape全局数组
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark
* @see
* @author  xuhb      @date  2019/06/10
************************************************************/
DPP_STATUS dpp_tm_shape_para_array_prt(ZXIC_UINT32 dev_id, ZXIC_UINT32 para_x, ZXIC_UINT32 para_y, ZXIC_UINT32 clear_flag);


/***********************************************************/
/** 配置shap模块中 crd_grain授权价值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   credit_value   授权价值，默认值是0x5feByte
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/08/13
************************************************************/
DPP_STATUS dpp_tm_shap_crd_grain_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 credit_value);


/***********************************************************/
/** shap ram初始化
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/22
************************************************************/
DPP_STATUS dpp_tm_shap_ram_init(ZXIC_UINT32 dev_id);

/***********************************************************/
/** 获取流队列双桶整形使能及模式
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
DPP_STATUS dpp_tm_shape_flow_db_en_get(DPP_DEV_T *dev, ZXIC_UINT32 *db_en, ZXIC_UINT32 *mode);

/***********************************************************/
/** 配置桶深最小单位配置:共8档：0-7
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   token_grain    3’d0：最小单位为128K
*                         3’d1：最小单位为64k
*                         3’d2：最小单位为32k
*                         3’d3：最小单位为16k
*                         3’d4：最小单位为8k
*                         3’d5：最小单位为4k
*                         3’d6：最小单位为2k
*                         3’d7：最小单位为1k
*           默认为0，即128K
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_token_grain_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 token_grain);

/***********************************************************/
/** 获取桶深最小单位配置:共8档：0-7
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   token_grain    3’d0：最小单位为128K
*                         3’d1：最小单位为64k
*                         3’d2：最小单位为32k
*                         3’d3：最小单位为16k
*                         3’d4：最小单位为8k
*                         3’d5：最小单位为4k
*                         3’d6：最小单位为2k
*                         3’d7：最小单位为1k
*           默认为0，即128K
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_token_grain_get(DPP_DEV_T *dev, ZXIC_UINT32 *token_grain);

/***********************************************************/
/** 配置流或调度器映射到整形参数表的某个ID
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   id        流或调度器编号ETM:0-ABFF,FTM:0-177F
* @param   profile_id  整形参数表:[0-127]
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_map_table_set(DPP_DEV_T *dev, ZXIC_UINT32 id, ZXIC_UINT32 profile_id);

/***********************************************************/
/** 获取流或调度器映射到整形参数表的配置ID
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   id        流或调度器编号ETM:0-ABFF,FTM:0-177F
* @param   profile_id  整形参数表:[0-127]
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_map_table_get(DPP_DEV_T *dev, ZXIC_UINT32 id, ZXIC_UINT32 *profile_id);

/***********************************************************/
/** 获取流级整形参数
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   flow_id   流队列号 ETM:0-9215,FTM:0-2047
* @param   cir       cir速率，单位Kb，范围[64Kb - 160Gb]
* @param   cbs       cbs桶深，单位KB，范围[1KB - 64M]
*                    注：cbs=0 表示关闭整形,即不限速
* @param   mode_e    整形模式，0-获取c桶参数，1-获取对应e桶参数
* @param   p_para_id   整形模板索引：ETM=[0-AFF]，FTM=[0-17F]
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_flow_para_get(DPP_DEV_T *dev,
                                      ZXIC_UINT32 flow_id,
                                      ZXIC_UINT32 mode,
                                      ZXIC_UINT32 *p_para_id,
                                      DPP_TM_SHAPE_PARA_TABLE *p_flow_para_tbl);


/***********************************************************/
/** etm配置流级整形参数
* @param   dev_id    设备编号
* @param   flow_id   流队列号 ETM:0-9215,FTM:0-2047
* @param   cir       cir速率，单位Kb，范围[64Kb - 160Gb]
* @param   cbs       cbs桶深，单位KB，范围[1KB - 64M]
* @param   db_en     双桶整形使能，0-单桶，1-双桶
* @param   eir       eir速率，单位Kb，范围同cir
* @param   ebs       ebs桶深，单位Kb，范围同cbs
*                    注：cbs=0 表示关闭整形,即不限速
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_etm_shape_flow_para_set(DPP_DEV_T *dev,
                                       ZXIC_UINT32 flow_id,
                                       ZXIC_UINT32 cir,
                                       ZXIC_UINT32 cbs,
                                       ZXIC_UINT32 db_en,
                                       ZXIC_UINT32 eir,
                                       ZXIC_UINT32 ebs);

/***********************************************************/
/** 获取调度单元整形参数
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   se_id     调度器单元号 ETM:0-63FF,FTM:0-77F
* @param   cir       cir速率，单位Kb，范围[64Kb - 160Gb]
* @param   cbs       cbs桶深，单位KB，范围[1KB - 64M]
* @param   mode    整形模式，0-获取p桶参数，1-获取对应c桶参数(仅FQ8/WFQ8支持)
* @param   p_para_id   整形模板索引：ETM=[0-AFF]，FTM=[0-17F]
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_se_para_get(DPP_DEV_T *dev,
                                    ZXIC_UINT32 se_id,
                                    ZXIC_UINT32 mode,
                                    ZXIC_UINT32 *p_para_id,
                                    DPP_TM_SHAPE_PARA_TABLE *p_se_para_tbl);



/***********************************************************/
/** ftm配置调度器整形参数
* @param   dev_id    设备编号
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
DPP_STATUS dpp_ftm_shape_se_para_set(ZXIC_UINT32 dev_id,
                                     ZXIC_UINT32 se_id,
                                     ZXIC_UINT32 pir,
                                     ZXIC_UINT32 pbs,
                                     ZXIC_UINT32 db_en,
                                     ZXIC_UINT32 cir,
                                     ZXIC_UINT32 cbs);

/***********************************************************/
/** etm配置调度器整形参数
* @param   dev_id    设备编号
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
DPP_STATUS dpp_etm_shape_se_para_set(DPP_DEV_T *dev,
                                     ZXIC_UINT32 se_id,
                                     ZXIC_UINT32 pir,
                                     ZXIC_UINT32 pbs,
                                     ZXIC_UINT32 db_en,
                                     ZXIC_UINT32 cir,
                                     ZXIC_UINT32 cbs);


/***********************************************************/
/** 写入流/调度器整形参数配置表
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   map_id    整形参数表中模板索引id ETM:0-AFF,FTM:0-17F
* @param   cir       整形速率(c/e桶统一)
* @param   cbs       桶深
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_para_set(DPP_DEV_T *dev,
                                 ZXIC_UINT32 total_para_id,
                                 ZXIC_UINT32 cir,
                                 ZXIC_UINT32 cbs);

/***********************************************************/
/** 读取流/调度器整形参数配置表
* @param   dev_id    设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   total_para_id    整形参数表中模板索引id ETM:0-AFF,FTM:0-17F
* @param   cir       整形速率(c/e桶统一)
* @param   cbs       桶深
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/03/11
************************************************************/
DPP_STATUS dpp_tm_shape_para_get(DPP_DEV_T *dev,
                                 ZXIC_UINT32 total_para_id,
                                 DPP_TM_SHAPE_PARA_TABLE *p_shap_para_tbl);

/***********************************************************/
/** 配置第0~15个被统计得到令牌个数的端口号
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   port_id   被统计得到令牌个数的端口号
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  whuashan      @date  2019/3/15           STM模式下使用
************************************************************/
DPP_STATUS dpp_tm_shape_token_pp_cfg(ZXIC_UINT32 dev_id,
                                     ZXIC_UINT32 port_id);

/***********************************************************/
/** 打印各级及指定被统计的第0~15个授权流得到的授权个数 stm模式下使用
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  yjd      @date  2015/06/19
************************************************************/
DPP_STATUS dpp_tm_crdt_ackcnt_diag(ZXIC_UINT32 dev_id, ZXIC_UINT32 delay_ms);

/***********************************************************/
/** 读取端口级整形参数
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
DPP_STATUS dpp_tm_shape_pp_para_get(DPP_DEV_T *dev,
                                    ZXIC_UINT32 port_id,
                                    DPP_TM_SHAPE_PP_PARA_T *p_para);

/***********************************************************/
/** 配置SA模式下各个版本的授权价值
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   sa_ver_id   版本号(0~7)
* @param   sa_credit_value   授权价值
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  szq      @date  2015/03/25
************************************************************/
DPP_STATUS dpp_tm_qmu_sa_credit_value_set(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 sa_ver_id,
                                          ZXIC_UINT32 sa_credit_value);

/***********************************************************/
/** 获取SA模式下各个版本的授权价值
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   sa_ver_id   版本号(0~7)
* @param   p_sa_credit_value   授权价值
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  szq      @date  2015/03/25
************************************************************/
DPP_STATUS dpp_tm_qmu_sa_credit_value_get(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 sa_ver_id,
                                          ZXIC_UINT32 *p_sa_credit_value);

/***********************************************************/
/** 配置CRS发送的速率
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   sent_cyc   CRS发送的间隔(单位:时钟周期)
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  szq      @date  2015/03/25
************************************************************/
DPP_STATUS dpp_tm_qmu_crs_sent_rate_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 sent_cyc);

/***********************************************************/
/** 获取CRS发送的速率
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_sent_cyc   CRS发送的间隔(单位:时钟周期)
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  szq      @date  2015/03/25
************************************************************/
DPP_STATUS dpp_tm_qmu_crs_sent_rate_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_sent_cyc);

/***********************************************************/
/** 配置CRS过滤使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   en   配置的值，0-不使能过滤，1-使能过滤
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/03/04
************************************************************/
DPP_STATUS dpp_tm_qmu_crs_filter_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 en);


/***********************************************************/
/** 配置多播授权令牌添加个数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   token_add_num   令牌添加时，每次增加的令牌数目，取值范围为1~255，默认为1；禁止配置为0，配置为0时，将不会产生授权。
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/09
************************************************************/
DPP_STATUS dpp_tm_qmu_mul_token_gen_num_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 token_add_num);

/***********************************************************/
/** 配置多播授权整形桶参数和使能参数
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   q3_lb_control_en   3号队列整形功能开启使能。0：关闭；1：开启。
* @param   q012_lb_control_en   0～2号队列整形功能开启使能。0：关闭；1：开启。
* @param   q3_lb_max_cnt   3号队列整形桶桶深。
* @param   q012_lb_max_cnt   0～2号队列整形桶桶深。
* @param   q3_lb_add_rate   3号队列令牌添加速率，时钟周期为单位。不可配置为0，配置为0整形使能时，不能产生队列3授权调度信号。
* @param   q012_lb_add_rate   0～2号队列令牌添加速率，以时钟周期单位。不可配置为0，配置为0并整形使能时，不能产生队列0、1、2授权调度信号。
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/09
************************************************************/
DPP_STATUS dpp_tm_qmu_mul_ack_lb_set(ZXIC_UINT32 dev_id,
                                     ZXIC_UINT32 q3_lb_control_en,
                                     ZXIC_UINT32 q012_lb_control_en,
                                     ZXIC_UINT32 q3_lb_max_cnt,
                                     ZXIC_UINT32 q012_lb_max_cnt,
                                     ZXIC_UINT32 q3_lb_add_rate,
                                     ZXIC_UINT32 q012_lb_add_rate);

/***********************************************************/
/** 配置0,1号队列挂接1或2号MCN漏桶信息
* @param   tm_type   0-ETM,1-FTM
* @param   dev_id   设备索引编号
* @param   mcn_lb_sel   0：0,1号队列挂接1号MCN漏桶 1：0,1号队列挂接2号MCN漏桶
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/09
************************************************************/
DPP_STATUS dpp_tm_qmu_mcn_lb_sel_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 mcn_lb_sel);

/***********************************************************/
/** 配置多播队列0~2的授权输出SP、DWRR
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   sp_or_dwrr   SP、DWRR模式选择。0：SP；1：DWRR。
* @param   dwrr_w0   0号队列DWRR权重(0~127)
* @param   dwrr_w1   1号队列DWRR权重(0~127)
* @param   dwrr_w2   2号队列DWRR权重(0~127)
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/09
************************************************************/
DPP_STATUS dpp_tm_qmu_mul_sp_dwrr_set(ZXIC_UINT32 dev_id,
                                      ZXIC_UINT32 sp_or_dwrr,
                                      ZXIC_UINT32 dwrr_w0,
                                      ZXIC_UINT32 dwrr_w1,
                                      ZXIC_UINT32 dwrr_w2);

/***********************************************************/
/** 配置分目的SA整形打开或关闭
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   shap_en   分目的SA整形使能开关 0：表示关闭 1：表示打开
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/03/09
************************************************************/
DPP_STATUS dpp_tm_qmu_dest_sa_shap_en_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 shap_en);

/***********************************************************/
/** 获得轮转扫描使能和扫描速率
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
DPP_STATUS dpp_tm_qmu_scan_rate_get(ZXIC_UINT32 dev_id,
                                    ZXIC_UINT32 *p_scan_en,
                                    ZXIC_UINT32 *p_scan_rate);

/***********************************************************/
/** 配置轮转扫描队列范围
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   first_que  起始队列号
* @param   last_que   终止队列号
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  whuashan      @date  2019/09/10
************************************************************/
DPP_STATUS dpp_tm_qmu_scan_que_range_set(ZXIC_UINT32 dev_id,
                                         ZXIC_UINT32 first_que,
                                         ZXIC_UINT32 last_que);

/***********************************************************/
/** 获取轮转扫描队列范围
* @param   dev_id   设备索引编号
* @param   tm_type   0-ETM,1-FTM
* @param   first_que  起始队列号
* @param   last_que   终止队列号
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  whuashan      @date  2019/09/10
************************************************************/
DPP_STATUS dpp_tm_qmu_scan_que_range_get(ZXIC_UINT32 dev_id,
                                         ZXIC_UINT32 *first_que,
                                         ZXIC_UINT32 *last_que);

/***********************************************************/
/** 获取QMU清空状态
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_clr_done_flag   队列是否清空完成
*
* @return
* @remark  无
* @see
* @author  szq      @date  2015/05/21
************************************************************/
DPP_STATUS dpp_tm_qmu_qlist_qcfg_clr_done_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_clr_done_flag);


/***********************************************************/
/** 配置qsch调度分端口整形速率和使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   shape_en   整形使能
* @param   token_add_num   [23:12]:添加令牌数目
* @param   token_gap   [11:0]:添加令牌间隔，其中实际间隔为配置间隔+1
* @param   token_depth 桶深，单位B,范围[0-0x1EE00]
*公式：（600*8*token_num）/(gap+1) = X Mbps
*     主频= 600 MHz
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  xuhb   2020-5-15
************************************************************/
DPP_STATUS dpp_tm_qmu_qsch_port_shape_set(DPP_DEV_T *dev,
                                     ZXIC_UINT32 port_id,
                                     ZXIC_UINT32 token_add_num,
                                     ZXIC_UINT32 token_gap,
                                     ZXIC_UINT32 token_depth,
                                     ZXIC_UINT32 shape_en);

/***********************************************************/
/** 配置CMD_SW分端口整形速率和使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   shape_en   整形使能
* @param   token_add_num   [23:12]:添加令牌数目
* @param   token_gap   [11:0]:添加令牌间隔，其中实际间隔为配置间隔+1
* @param   token_depth 桶深，单位B,范围[0-0x1EE00]
*公式：（600*8*token_num）/(gap+1) = X Mbps
*     主频= 600 MHz
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  whuashan   2020-3-17
************************************************************/
DPP_STATUS dpp_tm_qmu_port_shape_set(DPP_DEV_T *dev,
                                     ZXIC_UINT32 port_id,
                                     ZXIC_UINT32 token_add_num,
                                     ZXIC_UINT32 token_gap,
                                     ZXIC_UINT32 token_depth,
                                     ZXIC_UINT32 shape_en);

/***********************************************************/
/** 获得CMD_SW分端口整形速率和使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_shape_en   整形使能
* @param   p_token_add_num   [23:12]:添加令牌数目
* @param   p_token_gap   [11:0]:添加令牌间隔，其中实际间隔为配置间隔+1
* @param   p_token_depth 桶深，单位B
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  whuashan   2020-3-17
************************************************************/
DPP_STATUS dpp_tm_qmu_port_shape_get(DPP_DEV_T *dev,
                                     ZXIC_UINT32 port_id,
                                     ZXIC_UINT32 *p_token_add_num,
                                     ZXIC_UINT32 *p_token_gap,
                                     ZXIC_UINT32 *p_token_depth,
                                     ZXIC_UINT32 *p_shape_en);

/***********************************************************/
/** 获取CMD_SW分端口整形速率和使能
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   shape_vlue 整形值，单位Mbps
* @param   shape_en   整形使能
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author   zmy   @20151217
************************************************************/
DPP_STATUS dpp_tm_qmu_egress_shape_get(ZXIC_UINT32 dev_id,
                                       ZXIC_UINT32 port_id,
                                       ZXIC_UINT32 *shape_value,
                                       ZXIC_UINT32 *shape_en);


/***********************************************************/
/** 配置需要检测的特定队列号
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   qnum   需要检测统计的特定的队列号
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/18
************************************************************/
DPP_STATUS dpp_tm_qmu_spec_qnum_set(DPP_DEV_T *dev, ZXIC_UINT32 qnum);

/***********************************************************/
/** 获得特定的队列号
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_qnum   需要检测统计的特定的队列号
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/23
************************************************************/
DPP_STATUS dpp_tm_qmu_spec_qnum_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_qnum);

/***********************************************************/
/** 配置需要检测的队列组
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   group_num   需要检测统计的特定的队列组。这里按取q的低3bit
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/18
************************************************************/
DPP_STATUS dpp_tm_qmu_spec_group_set(DPP_DEV_T *dev, ZXIC_UINT32 group_num);

/***********************************************************/
/** 获得需要检测的队列组
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_group_num   需要检测统计的特定的队列组。这里按取q的低3bit
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/23
************************************************************/
DPP_STATUS dpp_tm_qmu_spec_group_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_group_num);


/***********************************************************/
/** 配置队列授权盈余
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   qnum   配置的队列号
* @param   value   授权盈余
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/18
************************************************************/
DPP_STATUS dpp_tm_qmu_crbal_value_set(ZXIC_UINT32 dev_id,
                                      ZXIC_UINT32 qnum,
                                      ZXIC_UINT32 value);

/***********************************************************/
/** 获得队列授权盈余
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   qnum   配置的队列号
* @param   p_value   授权盈余
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/23
************************************************************/
DPP_STATUS dpp_tm_qmu_crbal_value_get(ZXIC_UINT32 dev_id,
                                      ZXIC_UINT32 qnum,
                                      ZXIC_UINT32 *p_value);

/***********************************************************/
/** 配置分目的SA整形桶深上、下限参数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   max_value   分目的SA整形桶深上限，必须配置为正值
* @param   min_value   分目的SA整形桶深下限，必须配置为负值
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/18
************************************************************/
DPP_STATUS dpp_tm_qmu_dest_sa_shape_para_set(ZXIC_UINT32 dev_id,
                                             ZXIC_SINT32 max_value,
                                             ZXIC_SINT32 min_value);

/***********************************************************/
/** 获得分目的SA整形桶深上、下限参数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_max_value   分目的SA整形桶深上限，必须配置为正值
* @param   p_min_value   分目的SA整形桶深下限，必须配置为负值
*
* @return  0表示成功 非0表示操作失败
* @remark  无
* @see
* @author  taq      @date  2015/06/23
************************************************************/
DPP_STATUS dpp_tm_qmu_dest_sa_shape_para_get(ZXIC_UINT32 dev_id,
                                             ZXIC_UINT32 *p_max_value,
                                             ZXIC_UINT32 *p_min_value);

/***********************************************************/
/** 获取特定队列发送的crs normal的个数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   注 须先设置统计的特定队列
* @param
* @return
* @remark  无
* @see
* @author  yjd     @date  2015/07/09
************************************************************/
DPP_STATUS dpp_tm_qmu_spec_q_crs_normal_cnt(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_que_crs_normal_cnt);

/***********************************************************/
/** 获取特定队列发送的crs off的个数
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   注 须先设置统计的特定队列
* @param
* @return
* @remark  无
* @see
* @author  yjd     @date  2015/07/09
************************************************************/
DPP_STATUS dpp_tm_qmu_spec_q_crs_off_cnt(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_que_crs_off_cnt);

/***********************************************************/
/** QMU初始化配置场景
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   case_no   四组QMU初始化场景编号为1-4;ddr*bank:1:4x2;2:4x4;3:8x2;4:4x8.
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cuiy      @date  2016/11/17
************************************************************/
DPP_STATUS dpp_tm_qmu_init_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 case_no);

/***********************************************************/
/** TMMU TM纯片内模式配置获取
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_imem_en  1纯片内
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2017/10/14
************************************************************/
DPP_STATUS dpp_tm_tmmu_imem_en_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_imem_en);

/***********************************************************/
/** TMMU 强制DDR RDY配置获取
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_ddr_force_rdy  1、如果bit【0】配置为1，则QMU看到的DDR0 RDY一直为1。
                            2、bit【0】代表DDR0，bit【7】代表DDR7。
                            3、纯片内模式需要配置为8'hff，排除DDR干扰。
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  wush      @date  2017/10/14
************************************************************/
DPP_STATUS dpp_tm_tmmu_ddr_force_rdy_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_ddr_force_rdy);

DPP_STATUS dpp_tm_mr_init(ZXIC_UINT32 dev_id);
/***********************************************************/
/** 配置TM模式下初始化代码
* @param   dev_id
* @param   tm_type   0-ETM,1-FTM
* @param   p_tm_init_info    配置TM模式下初始化信息包括以下
*          blk_size  配置qmu block大小
*          case_num  四组QMU初始化场景编号为1-4;ddr*bank:1:4x2;2:4x4;3:8x2;4:4x8.
*          imem_omem;   0:片内外混合; 1:纯片内;2:纯片外
*          mode 0:TM 1:SA
*
* @return
* @remark  无
* @see
* @author  szq      @date  2015/03/26
************************************************************/
DPP_STATUS dpp_tm_asic_init(ZXIC_UINT32 dev_id, DPP_TM_ASIC_INIT_INFO_T *p_tm_asic_init_info);

/***********************************************************/
/** 配置TM模式下初始化代码
* @param   dev_id
* @param   tm_type   0-ETM,1-FTM
* @param   p_tm_init_info    配置TM模式下初始化信息包括以下
*          blk_size  配置qmu block大小
*          case_num  四组QMU初始化场景编号为1-4;ddr*bank:1:4x2;2:4x4;3:8x2;4:4x8.
*          imem_omem;   0:片内外混合; 1:纯片内;2:纯片外
*          mode 0:TM 1:SA
*
* @return
* @remark  无
* @see
* @author  szq      @date  2015/03/26
************************************************************/
DPP_STATUS dpp_tm_asic_init_diag(ZXIC_UINT32 dev_id, ZXIC_UINT32 blk_size, ZXIC_UINT32 case_num, ZXIC_UINT32 imem_omem, ZXIC_UINT32 mode);

/****************************************************************************
* 函数名称: dpp_tm_avg_que_len_get
* 功能描述: 各级平均队列深度获取
* 输入参数: dev_id: 设备索引编号
* @param   tm_type   0-ETM,1-FTM
*           cgavd_level: 拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
*           que_id: 本级别层次内的队列编号。
* 输出参数: p_avg_len: 平均队列深度，单位为BLOCK。
* 返 回 值: DPP_OK-成功，DPP_ERR-失败
* 其它说明:
* author  cy      @date  2015/06/29
*****************************************************************************/
DPP_STATUS dpp_tm_avg_que_len_get(ZXIC_UINT32 dev_id,
                                  DPP_TM_CGAVD_LEVEL_E cgavd_level,
                                  ZXIC_UINT32 que_id,
                                  ZXIC_UINT32 *p_avg_len);

/***********************************************************/
/** 获取配置CPU设置的报文长度是否参与计算丢弃概率的使能
* @param   tm_type   0-ETM,1-FTM
* @param   flag   忽略乘法里的当前包长和最大包长比标志位:1为忽略
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy     @date  2015/11/9
************************************************************/
DPP_STATUS dpp_tm_cgavd_wred_pke_len_calc_sign_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_flag);

/***********************************************************/
/**  配置配置cgavd模式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   mode   0:block mode 1:byte mode
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/07/28
************************************************************/
DPP_STATUS dpp_tm_cgavd_cfg_mode_set(DPP_DEV_T *dev,
                                     ZXIC_UINT32 mode);


/***********************************************************/
/**  配置配置cgavd模式
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_mode   0:block mode 1:byte mode
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/07/29
************************************************************/
DPP_STATUS dpp_tm_cgavd_cfg_mode_get(DPP_DEV_T *dev,
                                     ZXIC_UINT32 *p_mode);


/***********************************************************/
/** 配置TD拥塞避免模式下的丢弃门限值
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   id   队列号或端口号，系统级时，id参数无效
* @param   byte_block_th   配置的丢弃门限值，ZXIC_UINT8/BLOCK单位写入寄存器
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/07/29
************************************************************/
DPP_STATUS dpp_tm_cgavd_td_byte_block_th_set(DPP_DEV_T *dev,
                                             DPP_TM_CGAVD_LEVEL_E level,
                                             ZXIC_UINT32 id,
                                             ZXIC_UINT32 byte_block_th);


/***********************************************************/
/** 读取TD拥塞避免模式下的丢弃门限值字节模式
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   level   拥塞避免支持层次号，0:队列级，1:端口级，2:系统级
* @param   id   队列号或端口号，系统级时，id参数无效
* @param   p_byte_block_th   配置的丢弃门限值ZXIC_UINT8/BLOCK单位
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/07/29
************************************************************/
DPP_STATUS dpp_tm_cgavd_td_byte_block_th_get(DPP_DEV_T *dev,
                                             DPP_TM_CGAVD_LEVEL_E level,
                                             ZXIC_UINT32 id,
                                             ZXIC_UINT32 *p_byte_block_th);


/***********************************************************/
/**  配置通用门限block模式
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   byte_block_uni_th   通用门限值block/byte单位
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/08/01
************************************************************/
DPP_STATUS dpp_tm_cgavd_uniform_byte_block_th_set(DPP_DEV_T *dev, ZXIC_UINT32 byte_block_uni_th);

/***********************************************************/
/**  读取通用门限
* @param   dev_id 设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   p_byte_block_uni_th   通用门限值block/byte单位
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  cy      @date  2016/08/01
************************************************************/
DPP_STATUS dpp_tm_cgavd_uniform_byte_block_th_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_byte_block_uni_th);



/***********TM CPU软复位接口 Begin*************/
/***********************************************************/
/**  设置TM的全局变量，shape_para只保存profile被使用的数量，整形相关参数从寄存器中重新读取
* @param   dev_id
* @param   size         data_buff的长度
* @param   p_data_buff  需要恢复的内容
*
* @return
* @remark  无
* @see
* @author  XXX      @date  2018/06/25
************************************************************/
DPP_STATUS dpp_tm_glb_mgr_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 size, ZXIC_UINT8 *p_data_buff);

/***********************************************************/
/** 获取TM的全局变量，shape_para只保存profile被使用的数量，整形相关参数从寄存器中重新读取
* @param   dev_id
* @param   p_flag           上层释放data_buff的标志，1:需要上层free,0:不需要上层free
* @param   p_size           data_buff的长度
* @param   pp_data_buff     二级指针(指向函数内部malloc空间的地址)
*
* @return
* @remark  无
* @see
* @author  XXX      @date  2018/06/25
************************************************************/
DPP_STATUS dpp_tm_glb_mgr_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_flag, ZXIC_UINT32 *p_size, ZXIC_UINT8 **pp_data_buff);

/***********************************************************/
/** 获取TM的全局变量，shape_para只保存profile被使用的数量，整形相关参数从寄存器中重新读取
* @param   dev_id
* @param   p_size           data_buff的长度
*
* @return
* @remark  无
* @see
* @author  XXX      @date  2018/06/25
************************************************************/
DPP_STATUS dpp_tm_glb_size_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_size);


/***********************************************************/
/** 配置tm授权价值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   credit_value   授权价值，默认值是0x5feByte
*
* @return   DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  xuhb      @date  2020/08/13
************************************************************/
DPP_STATUS dpp_tm_credit_value_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 credit_value);

/***********************************************************/
/** 获取全局数组中用户实际配置的整形值
* @param   dev_id   设备编号
* @param   tm_type   0-ETM,1-FTM
* @param   flow_id   流队列号 ETM:0-9215,FTM:0-2047
* @param   cir       cir速率，单位Kb，范围[64Kb - 160Gb]
* @param   cbs       cbs桶深，单位KB，范围[1KB - 64M]
*                    注：cbs=0 表示关闭整形,即不限速
* @param   mode_e    整形模式，0-获取c桶参数，1-获取对应e桶参数
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark
* @see
* @author  xuhb      @date  2019/06/10
************************************************************/
DPP_STATUS dpp_tm_shape_flow_para_array_get(DPP_DEV_T *dev,
                                      ZXIC_UINT32 flow_id,
                                      ZXIC_UINT32 mode,
                                      DPP_TM_SHAPE_PARA_TABLE *p_flow_para_tbl);


#endif
/* 必须有个空行，否则可能编不过 */














