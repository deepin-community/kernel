/**************************************************************
* TM调度器资源分配如下
* FQ 
*fq_num:  1280 : 0, 1, 2 ... 0x4ff
*fq2_num: 1024 : 0x500, 0x502, 0x504 ... 0xcfe 
*fq4_num: 1024 : 0xd00, 0xd04, 0xd08 ... 0x1cfc 
*fq8_num: 1120 : 0x1d00, 0x1d08, 0x1d10 ... 0x3ff8
* SP-WFQ
*sp_num: 128 : 0x4000, 0x4001, 0x4002 ... 0x407f 
*wfq_num: 2048 : 0x4080, 0x4081, 0x4082 ... 0x487f 
*wfq2_num: 256 : 0x4880, 0x4882, 0x4884 ... 0x4a7e 
*wfq4_num: 256 : 0x4a80, 0x4a84, 0x4a88 ... 0x4e7c 
*wfq8_num: 688 : 0x4e80, 0x4e88, 0x4e90 ... 0x63f8 
*
*队列资源分配如下
*flow id 0~0xf 预留 0队列用于MR复制队列， 0x10~0xfff 通用队列

* PLCR profileid资源分配如下
* 一级CAR 
* flowid:32K, profileid:512 
* 一级CAR 
* flowid:4K, profileid:128 
* 一级CAR 
* flowid:1K, profileid:32 
***************************************************************/
#ifndef  _DPP_DRV_QOS_H_
#define  _DPP_DRV_QOS_H_

#include "zxic_common.h"
#include "dpp_tm.h"
#include "dpp_tm_api.h"
#include "dpp_stat_car.h"
#include "dpp_stat_api.h"
#include "dpp_agent_channel.h"

/******************************************************************************
 *                                宏定义                                *
 *****************************************************************************/
#define  DPP_VPORT_NUM_MAX  (0x7fff)
#define  DPP_CRDT_LEVEL_MAX (7)
#define  DPP_SCHE_TYPE_MAX  (10)

#define  DPP_TM_PORT_WIDTH   (56)
#define  DPP_TM_VPORT_WIDTH  (32)
#define  DPP_TM_LEVEL_WIDTH  (28)
#define  DPP_TM_TYPE_WIDTH   (24)

#define  G_SCH_ID_LEN (8)

#define  CAR_TYPE_MAX (4)

#define  G_PROFILE_ID_LEN (8)

/* Get Real port */
#define DPP_TM_CRDT_PP_PORT_GET(_gsch_id, _pp_port)  \
    ((_pp_port) = ((_gsch_id >> DPP_TM_PORT_WIDTH) & 0xffff))

/* Get Real vport */
#define DPP_TM_CRDT_VPORT_GET(_gsch_id, _vport)  \
    ((_vport) = ((_gsch_id >> DPP_TM_VPORT_WIDTH) & 0xffff))

/* Get Real level */
#define DPP_TM_CRDT_LEVEL_GET(_gsch_id,_sche_level)  \
    ((_sche_level) = ((_gsch_id >> DPP_TM_LEVEL_WIDTH) & 0xf))

/* Get Real type */
#define DPP_TM_CRDT_TYPE_GET(_gsch_id,_sche_type)  \
    ((_sche_type) = ((_gsch_id >> DPP_TM_TYPE_WIDTH) & 0xf))

/* Get Real se_id */
#define DPP_TM_CRDT_SE_ID_GET(_gsch_id,_se_id)  \
    ((_se_id) = ((_gsch_id & 0xffff)))

/* Get Real profile */
#define DPP_CAR_PROFILE_ID_GET(_profile_id,_profileid)  \
    ((_profileid) = ((_profile_id & 0xffff)))

/******************************************************************************
 *                                类型定义                               *
 *****************************************************************************/

/******************************************************************************
 *                                接口定义                               *
 *****************************************************************************/
/***********************************************************/
/**对外接口  TM资源申请
* @param   vport_id--vport号
* @param   pp_port--端口0~9 
* @param   numq--申请id个数 1
* @param   level--挂接层级
* @param   flags--se_id类型
* @param   gsch_id--调度单元号
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_cosq_gsch_id_add(DPP_PF_INFO_T* pf_info, 
                                ZXIC_UINT32 pp_port,
                                ZXIC_UINT32 numq, 
                                ZXIC_UINT32 level, 
                                ZXIC_UINT32 flags, 
                                ZXIC_UINT64 *p_gsch_id);

/***********************************************************/
/**对外接口  TM资源释放
* @param   vport_id--vport号
* @param   pp_port--端口0~9 
* @param   gsch_id--调度单元号
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_cosq_gsch_id_delete(DPP_PF_INFO_T* pf_info, 
                                   ZXIC_UINT32 pp_port, 
                                   ZXIC_UINT64 gsch_id);

/***********************************************************/
/**对外接口  读取TM根节点
* @param   vport_id--vport号
* @param   pp_port--端口0~9 
* @param   gsch_id--调度单元号
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_sch_base_node_get(DPP_PF_INFO_T* pf_info,
                                 ZXIC_UINT32 pp_port, 
                                 ZXIC_UINT64 *gsch_id);

/***********************************************************/
/**对外接口  配置se->pp->dev挂接关系
* @param   vport_id--vport号
* @param   se_id--调度器号
* @param   pp_id-端口号
* @param   weight-权重1
* @param   sp_mapping-优先级0-7
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_crdt_se_pp_link_set(DPP_PF_INFO_T* pf_info, 
                                   ZXIC_UINT32 se_id, 
                                   ZXIC_UINT32 pp_id, 
                                   ZXIC_UINT32 weight, 
                                   ZXIC_UINT32 sp_mapping);

/***********************************************************/
/**对外接口  配置se->se层次化挂接关系
* @param   vport_id--vport号
* @param   se_id--调度器号
* @param   se_linkid--上级调度器号
* @param   se_weight -权重
* @param   se_sp-优先级0-7
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_crdt_se_link_set(DPP_PF_INFO_T* pf_info, 
                                ZXIC_UINT32 se_id, 
                                ZXIC_UINT32 se_linkid, 
                                ZXIC_UINT32 se_weight, 
                                ZXIC_UINT32 se_sp);

/***********************************************************/
/**对外接口  配置flow级流队列挂接关系
* @param   vport_id--vport号
* @param   flow_id--0~4095
* @param   c_linkid--c桶se_id
* @param   c_weight--c桶权重
* @param   c_sp--c桶优先级
* @param   mode--0-单桶 1-双桶
* @param   e_linkid--e桶se_id
* @param   e_weight--e桶权重
* @param   e_sp--e桶优先级
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_crdt_flow_link_set(DPP_PF_INFO_T* pf_info,
                                  ZXIC_UINT32 flow_id,
                                  ZXIC_UINT32 c_linkid,
                                  ZXIC_UINT32 c_weight,
                                  ZXIC_UINT32 c_sp,
                                  ZXIC_UINT32 mode,
                                  ZXIC_UINT32 e_linkid,
                                  ZXIC_UINT32 e_weight,
                                  ZXIC_UINT32 e_sp);

/***********************************************************/
/**对外接口  删除flow级流队列挂接关系
* @param   vport_id--vport号
* @param   id_s--起始flowid
* @param   id_e--终止flowid
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_crdt_del_flow_link_set(DPP_PF_INFO_T* pf_info,
                                      ZXIC_UINT32  id_s,
                                      ZXIC_UINT32  id_e);

/***********************************************************/
/**对外接口  删除调度器挂接关系
* @param   vport_id--vport号
* @param   id_s--起始seid
* @param   id_e--终止seid
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_crdt_del_se_link_set(DPP_PF_INFO_T* pf_info,
                                    ZXIC_UINT32  id_s,
                                    ZXIC_UINT32  id_e);

/***********************************************************/
/**对外接口  配置端口级整形
* @param   vport_id--vport号
* @param   pp_port--端口0~9
* @param   cir   单位Kb
* @param   cbs   单位KB
* @param   c_en   c桶使能
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_port_shape_set(DPP_PF_INFO_T* pf_info,
                              ZXIC_UINT32 pp_port,
                              ZXIC_UINT32 cir,
                              ZXIC_UINT32 cbs,
                              ZXIC_UINT32 c_en);

/***********************************************************/
/**对外接口  读取端口级整形
* @param   vport_id--vport号
* @param   pp_port--端口0~9
* @param   p_para   整形信息:cir/cbs/en
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_port_shape_get(DPP_PF_INFO_T* pf_info,
                              ZXIC_UINT32 pp_port,
                              DPP_TM_SHAPE_PP_PARA_T *p_para);

/***********************************************************/
/**对外接口  配置调度器整形
* @param   vport_id  vport号
* @param   se_id     调度器编号号
* @param   pir       pir总速率，单位Kb，范围同cir
* @param   pbs       pbs总桶深，单位KB，范围同cbs
* @param   db_en     整形模式，0-单桶，1-双桶，仅FQ8/WFQ8有效
* @param   cir       调度器cir速率，单位Kb
* @param   cbs       调度器cbs桶深，单位KB
*                    注：cbs=0 表示关闭整形,即不限速
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_se_shape_set(DPP_PF_INFO_T* pf_info,
                            ZXIC_UINT32 se_id,
                            ZXIC_UINT32 pir,
                            ZXIC_UINT32 pbs,
                            ZXIC_UINT32 db_en,
                            ZXIC_UINT32 cir,
                            ZXIC_UINT32 cbs);

/***********************************************************/
/**对外接口  配置flow整形
* @param   vport_id  vport号
* @param   flow_id   流队列号
* @param   cir       cir速率，单位Kb
* @param   cbs       cbs桶深，单位KB
*                     注：cbs=0 表示关闭整形,即不限速
* @param   db_en     双桶整形使能，0-单桶，1-双桶
* @param   eir       eir速率，单位Kb
* @param   ebs       ebs桶深，单位KB
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_flow_shape_set(DPP_PF_INFO_T* pf_info,
                              ZXIC_UINT32 flow_id,
                              ZXIC_UINT32 cir,
                              ZXIC_UINT32 cbs,
                              ZXIC_UINT32 db_en,
                              ZXIC_UINT32 eir,
                              ZXIC_UINT32 ebs);

/***********************************************************/
/**对外接口  配置流队列挂接到端口号
* @param   vport_id  vport号
* @param   flow_id    流队列号
* @param   port      端口0~9      
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_flow_map_port_set(DPP_PF_INFO_T* pf_info,
                                 ZXIC_UINT32  flow_id,
                                 ZXIC_UINT32  port);

/***********************************************************/
/**对外接口  读取流队列挂接的端口号
* @param   vport_id  vport号
* @param   flowid    流队列号
* @param   port      端口0~9      
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_flow_map_port_get(DPP_PF_INFO_T* pf_info,
                                 ZXIC_UINT32  flow_id,
                                 ZXIC_UINT32  *p_port);

/***********************************************************/
/**对外接口  配置TD门限值
* @param   vport_id  vport号
* @param   flow_id   流队列号
* @param   td_th     配置的丢弃门限值      
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_flow_td_th_set(DPP_PF_INFO_T* pf_info,
                              ZXIC_UINT32  flow_id,
                              ZXIC_UINT32  td_th);

/***********************************************************/
/**对外接口  读取TD门限值
* @param   vport_id  vport号
* @param   flow_id   流队列号
* @param   p_td_th   配置的丢弃门限值      
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_flow_td_th_get(DPP_PF_INFO_T* pf_info,
                              ZXIC_UINT32  flow_id,
                              ZXIC_UINT32  *p_td_th);

/***********************************************************/
/**对外接口  设置block值
* @param   vport_id  vport号
* @param   size   配置block值      
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_blk_size_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 size);

/***********************************************************/
/**对外接口  配置全局pfc使能状态
* @param   vport_id  vport号
* @param   pfc_en    使能开关    
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_qmu_pfc_en_set(DPP_PF_INFO_T* pf_info, 
                              ZXIC_UINT32 pfc_en);

/***********************************************************/
/**对外接口  读取全局pfc使能状态
* @param   vport_id  vport号
* @param   p_pfc_en    使能开关    
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_qmu_pfc_en_get(DPP_PF_INFO_T* pf_info, 
                              ZXIC_UINT32 *p_pfc_en);

/***********************************************************/
/**对外接口  配置物理端口pfc使能状态
* @param   vport_id  vport号
* @param   port_id    端口0~9
* @param   port_en    使能开关   
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_qmu_port_pfc_set(DPP_PF_INFO_T* pf_info, 
                                ZXIC_UINT32 port_id,
                                ZXIC_UINT32 port_en);

/***********************************************************/
/**对外接口  读取物理端口pfc使能状态
* @param   vport_id  vport号
* @param   port_id    端口0~9
* @param   p_port_en  使能开关    
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_qmu_port_pfc_get(DPP_PF_INFO_T* pf_info, 
                                ZXIC_UINT32 port_id,
                                ZXIC_UINT32 *p_port_en);


/***********************************************************/
/**对外接口  申请profile_id资源
* @param   vport_id    vport号
* @param   numq        申请id个数 1
* @param   flags       car类型
* @param   profile_id  限速模版号   
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_car_profile_id_add(DPP_PF_INFO_T* pf_info,
                                  ZXIC_UINT32 flags,
                                  ZXIC_UINT64 *profile_id);

/***********************************************************/
/**对外接口  释放profile_id资源
* @param   vport_id    vport号
* @param   flags       car类型
* @param   profile_id  限速模版号  
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_car_profile_id_delete(DPP_PF_INFO_T* pf_info, 
                                     ZXIC_UINT32 flags, 
                                     ZXIC_UINT64 profile_id);

/***********************************************************/
/**对外接口 配置flow_id和profile_id的绑定关系，并配置限速模板使能
* @param   vport_id     vport号
* @param   car_type     car模式
* @param   flow_id      队列号
* @param   drop_flag    丢弃标志
* @param   plcr_en      限速使能
* @param   profile_id   模板编号
*
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_car_queue_cfg_set(DPP_PF_INFO_T* pf_info,
                                 ZXIC_UINT32 car_type,
                                 ZXIC_UINT32 flow_id,
                                 ZXIC_UINT32 drop_flag,
                                 ZXIC_UINT32 plcr_en,
                                 ZXIC_UINT32 profile_id);

/***********************************************************/
/**对外接口 查询flow_id和profile_id的绑定关系
* @param   vport_id     vport号
* @param   car_type     car模式
* @param   flow_id      队列号
* @param   p_drop_flag    丢弃标志
* @param   p_plcr_en      限速使能
* @param   p_profile_id   模板编号
*
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_car_queue_cfg_get(DPP_PF_INFO_T* pf_info,
                                 ZXIC_UINT32 car_type,
                                 ZXIC_UINT32 flow_id,
                                 ZXIC_UINT32 *p_drop_flag,
                                 ZXIC_UINT32 *p_plcr_en,
                                 ZXIC_UINT32 *p_profile_id);

/***********************************************************/
/**对外接口 配置profile_id限速模版
* @param   vport_id     vport号      
* @param   car_type     car模式
* @param   pkt_sign     限速模式0-字节;1-包
* @param   profile_id   模板编号
* @param   p_car_profile_cfg   限速参数
*
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_car_profile_cfg_set(DPP_PF_INFO_T* pf_info,
                                   ZXIC_UINT32 car_type,
                                   ZXIC_UINT32 pkt_sign,
                                   ZXIC_UINT32 profile_id,
                                   ZXIC_VOID* p_car_profile_cfg);

/***********************************************************/
/**对外接口 查询profile_id限速模版参数
* @param   vport_id     vport号      
* @param   car_type     car模式
* @param   pkt_sign     限速模式0-字节;1-包
* @param   profile_id   模板编号
* @param   p_car_profile_cfg   限速参数
*
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_car_profile_cfg_get(DPP_PF_INFO_T* pf_info,
                                   ZXIC_UINT32 car_type,
                                   ZXIC_UINT32 pkt_sign,
                                   ZXIC_UINT32 profile_id,
                                   ZXIC_VOID* p_car_profile_cfg);

/***********************************************************/
/**对外接口 配置队列映射关系
* @param   dev_id       设备号
* @param   car_type     car模式类型，参见STAT_CAR_TYPE_E
* @param   flow_id      队列号
* @param   map_flow_id  映射队列号
* @param   map_sp       映射sp
*
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_car_queue_map_set(DPP_PF_INFO_T* pf_info,
                                 ZXIC_UINT32 car_type,
                                 ZXIC_UINT32 flow_id,
                                 ZXIC_UINT32 map_flow_id,
                                 ZXIC_UINT32 map_sp);

/***********************************************************/
/**对外接口 配置队列映射关系
* @param   dev_id       设备号
* @param   car_type     car模式类型，参见STAT_CAR_TYPE_E
* @param   flow_id      队列号
* @param   map_flow_id  映射队列号
* @param   map_sp       映射sp
*
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_car_queue_map_get(DPP_PF_INFO_T* pf_info,
                                 ZXIC_UINT32 car_type,
                                 ZXIC_UINT32 flow_id,
                                 ZXIC_UINT32 *p_map_flow_id,
                                 ZXIC_UINT32 *p_map_sp);

DPP_STATUS dpp_np_flow_monitor_set_mode(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 flag);

/***********************************************************/
/**对外接口 设置入口水线阈值
* @param   dev_id       设备号
* @param   phy_port
* @param   mode
* @param   buffer_size
* @param   threshold_size
*
* @return
* @remark  无
* @see
* @author  sun      @date  2025/12/18
************************************************************/
DPP_STATUS dpp_qos_port_ingerss_buffer_set(DPP_PF_INFO_T *pf_info,
                                           ZXIC_UINT32 phy_port,
                                           ZXIC_UINT32 mode,
                                           ZXIC_UINT32 *buffer_size,
                                           ZXIC_UINT32 *threshold_size);

/***********************************************************/
/**对外接口 获取入口水线阈值
* @param   dev_id       设备号
* @param   phy_port
* @param   mode
* @param   buffer_size
* @param   threshold_size
*
* @return
* @remark  无
* @see
* @author  sun      @date  2025/12/18
************************************************************/
DPP_STATUS dpp_qos_port_ingerss_buffer_get(DPP_PF_INFO_T *pf_info,
                                           ZXIC_UINT32 phy_port,
                                           ZXIC_UINT32 *panel_buffer_size,
                                           ZXIC_UINT32 *panel_threshold_size,
                                           ZXIC_UINT32 *internal_buffer_size,
                                           ZXIC_UINT32 *internal_threshold_size);

/***********************************************************/
/**对外接口 入口信任模式配置
* @param   dev_id       设备号
* @param   phy_port
* @param   mode
* @param   trust_mode
*
* @return
* @remark  无
* @see
* @author  sun      @date  2026/1/13
************************************************************/
ZXIC_UINT32 dpp_pktrx_phy_trust_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 phy_port, ZXIC_UINT32 trust_mode, ZXIC_UINT8 id);


#endif