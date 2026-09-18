/**************************************************************
 * 文件名称 : dpp_drv_qos.c
 * 文件标识 :
 * 内容摘要 : QOS出口调度树资源维护
 * 其它说明 :
 * 当前版本 : 1.0
 * 作   者 : sun
 * 完成日期 :
 ***************************************************************/

/******************************************************************************
 *                               头文件                                *
 *****************************************************************************/
#include "dpp_drv_qos.h"

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
DPP_STATUS dpp_cosq_gsch_id_add(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 pp_port, ZXIC_UINT32 numq, ZXIC_UINT32 level,
                                ZXIC_UINT32 flags, ZXIC_UINT64 *p_gsch_id)
{
    DPP_STATUS ret         = DPP_OK;
    DPP_DEV_T dev          = {0};
    ZXIC_UINT32 num        = 0;
    ZXIC_UINT32 *gsch_id = ZXIC_NULL;
    ZXIC_UINT32 gsch_id_h = 0;
    ZXIC_UINT32 gsch_id_l = 0;
    ZXIC_UINT64 temp_id    = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);

    gsch_id = (ZXIC_UINT32 *)ZXIC_COMM_MALLOC(G_SCH_ID_LEN);
    ZXIC_COMM_CHECK_POINT(gsch_id);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(ret, "dpp_dev_get", gsch_id);

    ZXIC_COMM_CHECK_INDEX_MEMORY_FREE_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX, gsch_id);
    ZXIC_COMM_CHECK_INDEX_MEMORY_FREE_NO_ASSERT(pp_port, 0, DPP_TM_PP_NUM - 1, gsch_id);
    ZXIC_COMM_CHECK_INDEX_MEMORY_FREE_NO_ASSERT(level, 0, DPP_CRDT_LEVEL_MAX, gsch_id);
    ZXIC_COMM_CHECK_INDEX_MEMORY_FREE_NO_ASSERT(flags, 0, DPP_SCHE_TYPE_MAX, gsch_id);
    num = ((flags == FLOW_SCHE) ? numq : 1);

    ret = dpp_agent_channel_tm_seid_request(&dev, pp_port, pf_info->vport, level, flags, num, gsch_id);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(ret, "dpp_agent_channel_tm_seid_request", gsch_id);

    gsch_id_h = *(gsch_id + 1);
    gsch_id_l = *gsch_id;

    temp_id = ((ZXIC_UINT64)gsch_id_h) << 32 | ((ZXIC_UINT64)gsch_id_l);

    if (DPP_OK != (ZXIC_UINT32)(temp_id >> 56))
    {
        ZXIC_COMM_FREE(gsch_id);
        return DPP_ERR;
    }

    *p_gsch_id = temp_id;
    ZXIC_COMM_FREE(gsch_id);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_cosq_gsch_id_add);

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
DPP_STATUS dpp_cosq_gsch_id_delete(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 pp_port, ZXIC_UINT64 gsch_id)
{
    DPP_STATUS ret         = DPP_OK;
    DPP_DEV_T dev          = {0};
    ZXIC_UINT32 sche_level = 0;
    ZXIC_UINT32 sche_type  = 0;
    ZXIC_UINT32 num        = 1;
    ZXIC_UINT32 se_id      = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pp_port, 0, DPP_TM_PP_NUM - 1);

    DPP_TM_CRDT_LEVEL_GET(gsch_id, sche_level);
    DPP_TM_CRDT_TYPE_GET(gsch_id, sche_type);
    DPP_TM_CRDT_SE_ID_GET(gsch_id, se_id);

    ret = dpp_agent_channel_tm_seid_release(&dev, pp_port, pf_info->vport, sche_level, sche_type, num, se_id);
    ZXIC_COMM_CHECK_RC(ret, "dpp_agent_channel_tm_seid_release");

    // if (DPP_OK != ret)
    // {
    //     return DPP_ERR;
    // }

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_cosq_gsch_id_delete);

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
DPP_STATUS dpp_sch_base_node_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 pp_port, ZXIC_UINT64 *p_gsch_id)
{
    DPP_STATUS ret         = DPP_OK;
    DPP_DEV_T dev          = {0};
    ZXIC_UINT32 *gsch_id = ZXIC_NULL;
    ZXIC_UINT32 gsch_id_h = 0;
    ZXIC_UINT32 gsch_id_l = 0;
    ZXIC_UINT64 temp_id    = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);

    gsch_id = (ZXIC_UINT32 *)ZXIC_COMM_MALLOC(G_SCH_ID_LEN);
    ZXIC_COMM_CHECK_POINT(gsch_id);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(ret, "dpp_dev_get", gsch_id);

    ZXIC_COMM_CHECK_INDEX_MEMORY_FREE_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX, gsch_id);
    ZXIC_COMM_CHECK_INDEX_MEMORY_FREE_NO_ASSERT(pp_port, 0, DPP_TM_PP_NUM - 1, gsch_id);

    ret = dpp_agent_channel_tm_base_node_get(&dev, pp_port, pf_info->vport, gsch_id);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(ret, "dpp_agent_channel_tm_base_node_get", gsch_id);

    gsch_id_h = *(gsch_id + 1);
    gsch_id_l = *gsch_id;

    temp_id = ((ZXIC_UINT64)gsch_id_h) << 32 | ((ZXIC_UINT64)gsch_id_l);
    if (DPP_OK != (ZXIC_UINT32)(temp_id >> 56))
    {
        ZXIC_COMM_FREE(gsch_id);
        return DPP_ERR;
    }

    *p_gsch_id = temp_id;
    ZXIC_COMM_FREE(gsch_id);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_sch_base_node_get);

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
DPP_STATUS dpp_crdt_se_pp_link_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 se_id, ZXIC_UINT32 pp_id, ZXIC_UINT32 weight,
                                   ZXIC_UINT32 sp_mapping)
{
    DPP_STATUS ret     = DPP_OK;
    DPP_DEV_T dev      = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(se_id, 0, DPP_ETM_FQSPWFQ_NUM - 1);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pp_id, 0, DPP_TM_PP_NUM - 1);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(weight, 0, DPP_TM_SCH_WEIGHT_MAX);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(sp_mapping, DPP_TM_SCH_SP_0, DPP_TM_SCH_SP_8);

    ret = dpp_tm_crdt_se_pp_link_set(&dev, se_id, pp_id, weight, sp_mapping);
    ZXIC_COMM_CHECK_RC(ret, "dpp_tm_crdt_se_pp_link_set");

    return ret;
}
EXPORT_SYMBOL(dpp_crdt_se_pp_link_set);

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
DPP_STATUS dpp_crdt_se_link_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 se_id, ZXIC_UINT32 se_linkid, ZXIC_UINT32 se_weight,
                                ZXIC_UINT32 se_sp)
{
    DPP_STATUS ret     = DPP_OK;
    DPP_DEV_T dev      = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(se_id, 0, DPP_ETM_FQSPWFQ_NUM - 1);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(se_linkid, 0, DPP_ETM_FQSPWFQ_NUM - 1);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(se_weight, 0, DPP_TM_SCH_WEIGHT_MAX);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(se_sp, DPP_TM_SCH_SP_0, DPP_TM_SCH_SP_8);

    ret = dpp_tm_crdt_se_link_set(&dev, se_id, se_linkid, se_weight, se_sp);
    ZXIC_COMM_CHECK_RC(ret, "dpp_tm_crdt_se_link_set");

    return ret;
}
EXPORT_SYMBOL(dpp_crdt_se_link_set);

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
DPP_STATUS dpp_crdt_flow_link_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 flow_id, ZXIC_UINT32 c_linkid, ZXIC_UINT32 c_weight,
                                  ZXIC_UINT32 c_sp, ZXIC_UINT32 mode, ZXIC_UINT32 e_linkid, ZXIC_UINT32 e_weight,
                                  ZXIC_UINT32 e_sp)
{
    DPP_STATUS ret     = DPP_OK;
    DPP_DEV_T dev      = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);

    ret = dpp_tm_crdt_flow_link_set(&dev, flow_id, c_linkid, c_weight, c_sp, mode, e_linkid, e_weight, e_sp);
    ZXIC_COMM_CHECK_RC(ret, "dpp_tm_crdt_flow_link_set");

    return ret;
}
EXPORT_SYMBOL(dpp_crdt_flow_link_set);

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
DPP_STATUS dpp_crdt_del_flow_link_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 id_s, ZXIC_UINT32 id_e)
{
    DPP_STATUS ret     = DPP_OK;
    DPP_DEV_T dev      = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(id_s, 0, DPP_ETM_CRDT_NUM);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(id_e, 0, DPP_ETM_CRDT_NUM);

    ret = dpp_tm_crdt_del_flow_link_set(&dev, id_s, id_e);
    ZXIC_COMM_CHECK_RC(ret, "dpp_tm_crdt_del_flow_link_set");

    return ret;
}
EXPORT_SYMBOL(dpp_crdt_del_flow_link_set);

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
DPP_STATUS dpp_crdt_del_se_link_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 id_s, ZXIC_UINT32 id_e)
{
    DPP_STATUS ret     = DPP_OK;
    DPP_DEV_T dev      = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(id_s, 0, DPP_ETM_FQSPWFQ_NUM);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(id_e, 0, DPP_ETM_FQSPWFQ_NUM);

    ret = dpp_tm_crdt_del_se_link_set(&dev, id_s, id_e);
    ZXIC_COMM_CHECK_RC(ret, "dpp_tm_crdt_del_se_link_set");

    return ret;
}
EXPORT_SYMBOL(dpp_crdt_del_se_link_set);

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
DPP_STATUS dpp_port_shape_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 pp_port, ZXIC_UINT32 cir, ZXIC_UINT32 cbs,
                              ZXIC_UINT32 c_en)
{
    DPP_STATUS ret     = DPP_OK;
    DPP_DEV_T dev      = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pp_port, 0, DPP_TM_PP_NUM - 1);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(c_en, 0, 1);

    // ret = dpp_tm_shape_pp_para_wr(&dev, pp_port, cir, cbs, c_en);
    // ZXIC_COMM_CHECK_RC(ret, "dpp_tm_shape_pp_para_wr");

    ret = dpp_agent_channel_tm_port_shape(&dev, pp_port, cir, cbs, c_en);
    ZXIC_COMM_CHECK_RC(ret, "dpp_agent_channel_tm_port_shape");

    return ret;
}
EXPORT_SYMBOL(dpp_port_shape_set);

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
DPP_STATUS dpp_port_shape_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 pp_port, DPP_TM_SHAPE_PP_PARA_T *p_para)
{
    DPP_STATUS ret                      = DPP_OK;
    DPP_DEV_T dev                       = {0};
    DPP_TM_SHAPE_PP_PARA_T pp_shap_para = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pp_port, 0, DPP_TM_PP_NUM - 1);

    ret = dpp_tm_shape_pp_para_get(&dev, pp_port, &pp_shap_para);
    ZXIC_COMM_CHECK_RC(ret, "dpp_tm_shape_pp_para_get");

    p_para->c_en = pp_shap_para.c_en;
    p_para->cir  = pp_shap_para.cir;
    p_para->cbs  = pp_shap_para.cbs;

    return ret;
}
EXPORT_SYMBOL(dpp_port_shape_get);

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
DPP_STATUS dpp_se_shape_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 se_id, ZXIC_UINT32 pir, ZXIC_UINT32 pbs,
                            ZXIC_UINT32 db_en, ZXIC_UINT32 cir, ZXIC_UINT32 cbs)
{
    DPP_STATUS ret     = DPP_OK;
    DPP_DEV_T dev      = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);

    // ret = dpp_tm_shape_se_para_set(&dev, se_id, pir, pbs, db_en, cir, cbs);
    // ZXIC_COMM_CHECK_RC(ret, "dpp_tm_shape_se_para_set");

    ret = dpp_agent_channel_tm_se_shape(&dev, se_id, pir, pbs, db_en, cir, cbs);
    ZXIC_COMM_CHECK_RC(ret, "dpp_agent_channel_tm_se_shape");    

    return ret;
}
EXPORT_SYMBOL(dpp_se_shape_set);

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
DPP_STATUS dpp_flow_shape_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 flow_id, ZXIC_UINT32 cir, ZXIC_UINT32 cbs,
                              ZXIC_UINT32 db_en, ZXIC_UINT32 eir, ZXIC_UINT32 ebs)
{
    DPP_STATUS ret     = DPP_OK;
    DPP_DEV_T dev      = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);

    // ret = dpp_tm_shape_flow_para_set(&dev, flow_id, cir, cbs, db_en, eir, ebs);
    // ZXIC_COMM_CHECK_RC(ret, "dpp_tm_shape_flow_para_set");

    ret = dpp_agent_channel_tm_flow_shape(&dev, flow_id, cir, cbs, db_en, eir, ebs);
    ZXIC_COMM_CHECK_RC(ret, "dpp_agent_channel_tm_flow_shape");

    return ret;
}
EXPORT_SYMBOL(dpp_flow_shape_set);

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
DPP_STATUS dpp_flow_map_port_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 flow_id, ZXIC_UINT32 port)
{
    DPP_STATUS ret     = DPP_OK;
    DPP_DEV_T dev      = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);

    ret = dpp_tm_cgavd_q_map_pp_set(&dev, flow_id, port);
    ZXIC_COMM_CHECK_RC(ret, "dpp_tm_cgavd_q_map_pp_set");

    return ret;
}
EXPORT_SYMBOL(dpp_flow_map_port_set);

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
DPP_STATUS dpp_flow_map_port_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 flow_id, ZXIC_UINT32 *p_port)
{
    DPP_STATUS ret     = DPP_OK;
    DPP_DEV_T dev      = {0};
    ZXIC_UINT32 pp_id  = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);

    ret = dpp_tm_cgavd_q_map_pp_get(&dev, flow_id, &pp_id);
    ZXIC_COMM_CHECK_RC(ret, "dpp_tm_cgavd_q_map_pp_get");

    *p_port = pp_id;

    return ret;
}
EXPORT_SYMBOL(dpp_flow_map_port_get);

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
DPP_STATUS dpp_flow_td_th_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 flow_id, ZXIC_UINT32 td_th)
{
    DPP_STATUS ret     = DPP_OK;
    DPP_DEV_T dev      = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);

    // ret = dpp_tm_cgavd_td_th_set(&dev, QUEUE_LEVEL, flow_id, td_th);
    // ZXIC_COMM_CHECK_RC(ret, "dpp_tm_cgavd_td_th_set");

    ret = dpp_agent_channel_tm_td_set(&dev, QUEUE_LEVEL, flow_id, td_th);
    ZXIC_COMM_CHECK_RC(ret, "dpp_agent_channel_tm_td_set");

    return ret;
}
EXPORT_SYMBOL(dpp_flow_td_th_set);

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
DPP_STATUS dpp_flow_td_th_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 flow_id, ZXIC_UINT32 *p_td_th)
{
    DPP_STATUS ret     = DPP_OK;
    DPP_DEV_T dev      = {0};
    ZXIC_UINT32 td_th  = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);

    ret = dpp_tm_cgavd_td_th_get(&dev, QUEUE_LEVEL, flow_id, &td_th);
    ZXIC_COMM_CHECK_RC(ret, "dpp_tm_cgavd_td_th_get");

    *p_td_th = td_th;

    return ret;
}
EXPORT_SYMBOL(dpp_flow_td_th_get);

/***********************************************************/
/**对外接口  设置block值
* @param   vport_id  vport号
* @param   size   配置block值      
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_blk_size_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 size)
{
    DPP_STATUS ret  = DPP_OK;
    DPP_DEV_T dev      = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);
    ret = dpp_tm_cfgmt_blk_size_set(&dev, size);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_tm_cfgmt_blk_size_set");

    return ret;
}
EXPORT_SYMBOL(dpp_blk_size_set);

/***********************************************************/
/**对外接口  配置全局pfc使能状态
 * @param   vport_id  vport号
 * @param   pfc_en    使能开关
 * @return
 * @remark  无
 * @see
 * @author  sun      @date  2023/11/17
 ************************************************************/
DPP_STATUS dpp_qmu_pfc_en_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 pfc_en)
{
    DPP_STATUS ret     = DPP_OK;
    DPP_DEV_T dev      = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pfc_en, 0, 1);

    ret = dpp_tm_qmu_pfc_en_set(&dev, pfc_en);
    ZXIC_COMM_CHECK_RC(ret, "dpp_tm_qmu_pfc_en_set");

    return ret;
}
EXPORT_SYMBOL(dpp_qmu_pfc_en_set);

/***********************************************************/
/**对外接口  读取全局pfc使能状态
 * @param   vport_id  vport号
 * @param   p_pfc_en    使能开关
 * @return
 * @remark  无
 * @see
 * @author  sun      @date  2023/11/17
 ************************************************************/
DPP_STATUS dpp_qmu_pfc_en_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *p_pfc_en)
{
    DPP_STATUS ret     = DPP_OK;
    DPP_DEV_T dev      = {0};
    ZXIC_UINT32 pfc_en = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pfc_en, 0, 1);

    ret = dpp_tm_qmu_pfc_en_get(&dev, &pfc_en);
    ZXIC_COMM_CHECK_RC(ret, "dpp_tm_qmu_pfc_en_set");

    *p_pfc_en = pfc_en;

    return ret;
}
EXPORT_SYMBOL(dpp_qmu_pfc_en_get);

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
DPP_STATUS dpp_qmu_port_pfc_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 port_id, ZXIC_UINT32 port_en)
{
    DPP_STATUS ret     = DPP_OK;
    DPP_DEV_T dev      = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(port_id, 0, DPP_TM_PP_NUM - 1);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(port_en, 0, 1);

    ret = dpp_tm_qmu_port_pfc_make_set(&dev, port_id, port_en);
    ZXIC_COMM_CHECK_RC(ret, "dpp_tm_qmu_port_pfc_make_set");

    return ret;
}
EXPORT_SYMBOL(dpp_qmu_port_pfc_set);

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
DPP_STATUS dpp_qmu_port_pfc_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 port_id, ZXIC_UINT32 *p_port_en)
{
    DPP_STATUS ret      = DPP_OK;
    DPP_DEV_T dev       = {0};
    ZXIC_UINT32 port_en = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(port_id, 0, DPP_TM_PP_NUM - 1);

    ret = dpp_tm_qmu_port_pfc_make_get(&dev, port_id, &port_en);
    ZXIC_COMM_CHECK_RC(ret, "dpp_tm_qmu_port_pfc_make_get");

    *p_port_en = port_en;

    return ret;
}
EXPORT_SYMBOL(dpp_qmu_port_pfc_get);


/***********************************************************/
/**对外接口  申请profile_id资源
* @param   vport_id    vport号
* @param   flags       car类型
* @param   p_profile_id  限速模版号   
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_car_profile_id_add(DPP_PF_INFO_T* pf_info,
                                  ZXIC_UINT32 flags,
                                  ZXIC_UINT64 *p_profile_id)
{
    DPP_STATUS ret         = DPP_OK;
    DPP_DEV_T dev          = {0};
    ZXIC_UINT32 *profile_id = ZXIC_NULL;
    ZXIC_UINT32 profile_id_h = 0;
    ZXIC_UINT32 profile_id_l = 0;
    ZXIC_UINT64 temp_profile_id    = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);

    profile_id = (ZXIC_UINT32 *)ZXIC_COMM_MALLOC(G_PROFILE_ID_LEN);
    ZXIC_COMM_CHECK_POINT(profile_id);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(ret, "dpp_dev_get", profile_id);

    ZXIC_COMM_CHECK_INDEX_MEMORY_FREE_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX, profile_id);
    ZXIC_COMM_CHECK_INDEX_MEMORY_FREE_NO_ASSERT(flags, 0, CAR_TYPE_MAX, profile_id);

    ret = dpp_agent_channel_plcr_profileid_request(&dev, pf_info->vport, flags, profile_id);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(ret, "dpp_agent_channel_plcr_profileid_request", profile_id);

    profile_id_h = *(profile_id + 1);
    profile_id_l = *profile_id;

    temp_profile_id = ((ZXIC_UINT64)profile_id_l) << 32 | ((ZXIC_UINT64)profile_id_h);

    if (DPP_OK != (ZXIC_UINT32)(temp_profile_id >> 56))
    {
        ZXIC_COMM_FREE(profile_id);
        return DPP_ERR;
    }

    *p_profile_id = temp_profile_id;
    ZXIC_COMM_FREE(profile_id);

    return ret;
}
EXPORT_SYMBOL(dpp_car_profile_id_add);

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
DPP_STATUS dpp_car_profile_id_delete(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 flags, ZXIC_UINT64 profile_id)
{
    DPP_STATUS ret         = DPP_OK;
    DPP_DEV_T dev          = {0};
    ZXIC_UINT32 profileid     = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    DPP_CAR_PROFILE_ID_GET(profile_id, profileid);

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(flags, 0, CAR_TYPE_MAX);

    ret = dpp_agent_channel_plcr_profileid_release(&dev, pf_info->vport, flags, profileid);
    ZXIC_COMM_CHECK_RC(ret, "dpp_agent_channel_plcr_profileid_release");

    // if (DPP_OK != ret)
    // {
    //     return DPP_ERR;
    // }

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_car_profile_id_delete);

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
                                 ZXIC_UINT32 profile_id)
{
    DPP_STATUS ret         = DPP_OK;
    DPP_DEV_T dev          = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);

    ret = dpp_stat_car_queue_cfg_set(&dev, car_type, flow_id, drop_flag, plcr_en, profile_id);
    ZXIC_COMM_CHECK_RC(ret, "dpp_stat_car_queue_cfg_set");

    return ret;

}
EXPORT_SYMBOL(dpp_car_queue_cfg_set);

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
                                 ZXIC_UINT32 *p_profile_id)
{
    DPP_STATUS ret         = DPP_OK;
    DPP_DEV_T dev          = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_CHECK_DEV_POINT(0, p_drop_flag);
    ZXIC_COMM_CHECK_DEV_POINT(0, p_plcr_en);
    ZXIC_COMM_CHECK_DEV_POINT(0, p_profile_id);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);

    ret = dpp_stat_car_queue_cfg_get(&dev, car_type, flow_id, p_drop_flag, p_plcr_en, p_profile_id);
    ZXIC_COMM_CHECK_RC(ret, "dpp_stat_car_queue_cfg_get");

    return ret;
}
EXPORT_SYMBOL(dpp_car_queue_cfg_get);

/***********************************************************/
/**对外接口 配置profile_id限速模版
* @param   dev_id      
* @param   car_type   
* @param   pkt_sign   
* @param   profile_id   
* @param   p_car_profile_cfg   
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
                                   ZXIC_VOID* p_car_profile_cfg)
{
    DPP_STATUS ret         = DPP_OK;
    DPP_DEV_T dev          = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_POINT(p_car_profile_cfg);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);

    // ret = dpp_stat_car_profile_cfg_set(&dev, car_type, pkt_sign, profile_id, p_car_profile_cfg);
    // ZXIC_COMM_CHECK_RC(ret, "dpp_stat_car_queue_cfg_set");

    ret = dpp_agent_channel_plcr_car_rate(&dev, car_type, pkt_sign, profile_id, p_car_profile_cfg);
    ZXIC_COMM_CHECK_RC(ret, "dpp_agent_channel_plcr_car_rate");

    return ret;   
}
EXPORT_SYMBOL(dpp_car_profile_cfg_set);

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
                                   ZXIC_VOID* p_car_profile_cfg)
{
    DPP_STATUS ret         = DPP_OK;
    DPP_DEV_T dev          = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_POINT(p_car_profile_cfg);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);

    ret = dpp_stat_car_profile_cfg_get(&dev, car_type, pkt_sign, profile_id, p_car_profile_cfg);
    ZXIC_COMM_CHECK_RC(ret, "dpp_stat_car_profile_cfg_get");

    return ret;      
}
EXPORT_SYMBOL(dpp_car_profile_cfg_get);

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
                                 ZXIC_UINT32 map_sp)
{
    DPP_STATUS ret         = DPP_OK;
    DPP_DEV_T dev          = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);

    ret = dpp_stat_car_queue_map_set(&dev, car_type, flow_id, map_flow_id, map_sp);
    ZXIC_COMM_CHECK_RC(ret, "dpp_stat_car_queue_map_set");

    return ret;  
}
EXPORT_SYMBOL(dpp_car_queue_map_set);

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
                                 ZXIC_UINT32 *p_map_sp)
{
    DPP_STATUS ret         = DPP_OK;
    DPP_DEV_T dev          = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_POINT(p_map_flow_id);
    ZXIC_COMM_CHECK_POINT(p_map_sp);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);

    ret = dpp_stat_car_queue_map_get(&dev, car_type, flow_id, p_map_flow_id, p_map_sp);
    ZXIC_COMM_CHECK_RC(ret, "dpp_stat_car_queue_map_get");

    return ret;  
}
EXPORT_SYMBOL(dpp_car_queue_map_get);

DPP_STATUS dpp_np_flow_monitor_set_mode(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 flag)
{
    DPP_STATUS ret     = DPP_OK;
    DPP_DEV_T dev      = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(flag, 0, 1);

    ret = dpp_agent_channel_np_flow_monitor(&dev, pf_info->vport, flag);
    ZXIC_COMM_CHECK_RC(ret, "dpp_agent_channel_np_flow_monitor");

    return ret;
}
EXPORT_SYMBOL(dpp_np_flow_monitor_set_mode);

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
                                           ZXIC_UINT32 *threshold_size)
{
    DPP_STATUS ret = DPP_OK;
    DPP_DEV_T dev = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(mode, 0, 1);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(phy_port, 0, DPP_TM_PP_NUM);
    ZXIC_COMM_CHECK_POINT(buffer_size);
    ZXIC_COMM_CHECK_POINT(threshold_size);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");


    ret = dpp_agent_channel_np_ingress_buffer_set(&dev, phy_port, mode, buffer_size, threshold_size);
    if (ret != DPP_OK)
    {
        ZXIC_COMM_TRACE_ERROR("Failed to set ingress buffer: port=%u, mode=%u, ret=%d\n",
                               phy_port, mode, ret);
        return ret;
    }

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_qos_port_ingerss_buffer_set);

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
                                           ZXIC_UINT32 *internal_threshold_size)
{
    DPP_STATUS ret = DPP_OK;
    DPP_DEV_T dev = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ret = dpp_agent_channel_np_ingress_buffer_get(&dev, phy_port, panel_buffer_size, panel_threshold_size, internal_buffer_size, internal_threshold_size);
    if (ret != DPP_OK)
    {
        ZXIC_COMM_TRACE_ERROR("Failed to get ingress buffer: port=%u, ret=%d\n",
                               phy_port, ret);
        return ret;
    }

    return DPP_OK;

}
EXPORT_SYMBOL(dpp_qos_port_ingerss_buffer_get);

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
ZXIC_UINT32 dpp_pktrx_phy_trust_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 phy_port, ZXIC_UINT32 trust_mode, ZXIC_UINT8 id)
{
    DPP_STATUS ret = DPP_OK;
    DPP_DEV_T dev = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ret = dpp_agent_channel_np_pktrx_trust_set(&dev, phy_port, trust_mode, id);
    if (ret != DPP_OK)
    {
        ZXIC_COMM_TRACE_ERROR("Failed to set trust: port=%u, ret=%d\n", phy_port, ret);
        return ret;
    }

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_pktrx_phy_trust_set);
