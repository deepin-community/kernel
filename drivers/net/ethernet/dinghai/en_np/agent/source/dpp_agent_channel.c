#include "dpp_agent_channel.h"
#include "dh_cmd.h"
#include "dpp_dev.h"
#include "dpp_pktrx_api.h"

DPP_STATUS dpp_agent_channel_init()
{
    // zxdh_bar_msg_chan_init();
    return DPP_OK;
}

DPP_STATUS dpp_agent_channel_exit()
{
    // zxdh_bar_msg_chan_remove();
    return DPP_OK;
}

static ZXIC_VOID dpp_agent_msg_prt(ZXIC_UINT8 type, ZXIC_UINT32 rtn)
{
    switch (rtn)
    {
        case DPP_RC_CTRLCH_MSG_LEN_ZERO:
        {
            ZXIC_COMM_TRACE_ERROR("type[%u]:msg len is zero!\n",type);
            break;
        }
        case DPP_RC_CTRLCH_MSG_PRO_ERR:
        {
            ZXIC_COMM_TRACE_ERROR("type[%u]:msg process error!\n",type);
            break;
        }
        case DPP_RC_CTRLCH_MSG_TYPE_NOT_SUPPORT:
        {
            ZXIC_COMM_TRACE_ERROR("type[%u]:fw not support the msg!\n",type);
            break;
        }
        case DPP_RC_CTRLCH_MSG_OPER_NOT_SUPPORT:
        {
            ZXIC_COMM_TRACE_ERROR("type[%u]:fw not support opr of the msg!\n",type);
            break;
        }
        case DPP_RC_CTRLCH_MSG_DROP:
        {
            ZXIC_COMM_TRACE_ERROR("type[%u]:fw not support,drop msg!\n",type);
            break;
        }
        default:
            break;
    }
    return;
}
static DPP_STATUS dpp_agent_bar_msg_check(DPP_DEV_T *dev, DPP_AGENT_CHANNEL_MSG_T* pMsg)
{
    ZXIC_UINT8 type = 0;
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(pMsg);

    type = *((ZXIC_UINT8 *)(pMsg->msg)+1);
    if(type != DPP_PCIE_BAR_MSG)
    {
        if(type >= DEV_PCIE_BAR_MSG_NUM(dev))
        {
            ZXIC_COMM_TRACE_ERROR("type[%u] > fw_bar_msg_num[%u]!\n",type,DEV_PCIE_BAR_MSG_NUM(dev));
            return DPP_RC_CTRLCH_MSG_TYPE_NOT_SUPPORT;
        }
    }

    return DPP_OK;
}
DPP_STATUS dpp_agent_channel_reg_sync_send(DPP_DEV_T *dev, DPP_AGENT_CHANNEL_REG_MSG_T *pMsg,
                                                        ZXIC_UINT32 *pData, ZXIC_UINT32 rep_len)
{
    DPP_STATUS ret = DPP_OK;
    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(pMsg);

    agentMsg.msg                     = (ZXIC_VOID *)pMsg;
    agentMsg.msg_len                 = sizeof(DPP_AGENT_CHANNEL_REG_MSG_T);

    ret = dpp_agent_channel_sync_send(dev, &agentMsg, pData, rep_len);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_sync_send");

    ret = *pData;
    if (DPP_OK != ret)
    {
        ZXIC_COMM_TRACE_ERROR("dpp_agent_channel_sync_send failed in buffer\n");
        return DPP_ERR;
    }

    return DPP_OK;
}

DPP_STATUS dpp_agent_channel_sync_send(DPP_DEV_T *dev, DPP_AGENT_CHANNEL_MSG_T *pMsg,
                                                        ZXIC_UINT32 *pData, ZXIC_UINT32 rep_len)
{
    DPP_STATUS ret            = DPP_OK;
    ZXIC_UINT8 *reply_ptr     = NULL;
    ZXIC_UINT8 retry_count    = 0;
    ZXIC_UINT16 reply_msg_len = 0;
    ZXIC_UINT32 *recv_buffer  = NULL;

    struct zxdh_pci_bar_msg in          = {0};
    struct zxdh_msg_recviver_mem result = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(pMsg);
    ZXIC_COMM_CHECK_POINT(pData);

    ret = dpp_agent_bar_msg_check(dev,pMsg);
    ZXIC_COMM_CHECK_RC(ret, "dpp_agent_bar_msg_check");

    recv_buffer = (ZXIC_UINT32 *)ZXIC_COMM_MALLOC(rep_len + CHANNEL_REPS_LEN);
    ZXIC_COMM_CHECK_POINT(recv_buffer);
    ZXIC_COMM_MEMSET(recv_buffer, 0, rep_len + CHANNEL_REPS_LEN);

    in.virt_addr    = DEV_PCIE_MSG_ADDR(dev);
    in.payload_addr = pMsg->msg;
    in.payload_len  = pMsg->msg_len;
    in.src          = MSG_CHAN_END_PF;
    in.dst          = MSG_CHAN_END_RISC;
    in.event_id     = NP_AGENT_ID;
    in.src_pcieid   = DEV_PCIE_ID(dev);

    result.buffer_len  = rep_len + CHANNEL_REPS_LEN;
    result.recv_buffer = recv_buffer;

    ZXIC_COMM_TRACE_DEBUG("in.virt_addr 0x%llx.\n", in.virt_addr);

    do
    {
        ret = zxdh_bar_chan_sync_msg_send(&in, &result);
        if (ret == BAR_MSG_ERR_LOCK_FAILED)
        {
            retry_count++;
            ZXIC_COMM_TRACE_INFO("zxdh_bar_chan_sync_msg_send return %d, retry %d times...\n", ret, retry_count);
            msleep(200);
        }
        else
        {
            break;
        }
    } while (retry_count < BAR_MSG_RETRY_MAX_TIME);

    if(retry_count >= BAR_MSG_RETRY_MAX_TIME)
    {
        ZXIC_COMM_CHECK_RC_MEMORY_FREE(ret, "zxdh_bar_chan_sync_msg_send", recv_buffer);
    }

    if(ret==BAR_MSG_ERR_BAR_ABNORMAL)
    {
        ret = ZXIC_PAR_CHK_BAR_ABNORMAL;
    }
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(ret, "zxdh_bar_chan_sync_msg_send", recv_buffer);

    reply_ptr = (ZXIC_UINT8 *)(result.recv_buffer);
    if (MSG_REP_VALID == *reply_ptr)
    {
        reply_msg_len = *(ZXIC_UINT16 *)(reply_ptr + MSG_REP_LEN_OFFSET);
        ZXIC_COMM_MEMCPY_S(pData,rep_len,reply_ptr + MSG_REP_OFFSET,reply_msg_len);

        ZXIC_COMM_FREE(recv_buffer);
        return DPP_OK;
    }

    ZXIC_COMM_FREE(recv_buffer);

    ZXIC_COMM_TRACE_ERROR("zxdh_bar_chan_sync_msg_send failed.\n");

    return DPP_ERR;
}

DPP_STATUS dpp_agent_channel_reg_write(DPP_DEV_T *dev, ZXIC_UINT32 reg_type, ZXIC_UINT32 reg_no,
                                            ZXIC_UINT32 reg_width, ZXIC_UINT32 addr, ZXIC_UINT32 *pData)
{
    DPP_STATUS ret          = 0;
    ZXIC_UINT32 resp_len    = 0;
    ZXIC_UINT8 *resp_buffer = NULL;

    DPP_AGENT_CHANNEL_REG_MSG_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(pData);

    msgcfg.devId                       = 0;
    msgcfg.type                        = DPP_REG_MSG;
    msgcfg.subtype                     = reg_type;
    msgcfg.oper                        = DPP_WR;
    msgcfg.reg_no                      = reg_no;
    msgcfg.addr                        = addr;
    msgcfg.val_len                     = reg_width / 4;
    memcpy(msgcfg.val, pData, reg_width);

    resp_len    = reg_width + 4;
    resp_buffer = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(resp_len);
    ZXIC_COMM_CHECK_POINT(resp_buffer);

    memset(resp_buffer, 0, resp_len);

    ret = dpp_agent_channel_reg_sync_send(dev, &msgcfg, (ZXIC_UINT32 *)resp_buffer, resp_len);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(ret, "dpp_agent_channel_reg_sync_send", resp_buffer);

    if (DPP_OK != *((ZXIC_UINT32 *)resp_buffer))
    {
        ZXIC_COMM_TRACE_ERROR("dpp_agent_channel_reg_sync_send failed in buffer\n");
        ZXIC_COMM_FREE(resp_buffer);
        return DPP_ERR;
    }

    memcpy(pData, resp_buffer + 4, reg_width);

    ZXIC_COMM_FREE(resp_buffer);

    return DPP_OK;
}

DPP_STATUS dpp_agent_channel_reg_read(DPP_DEV_T *dev, ZXIC_UINT32 reg_type, ZXIC_UINT32 reg_no, ZXIC_UINT32 reg_width, ZXIC_UINT32 addr,
                                      ZXIC_UINT32 *pData)
{
    DPP_STATUS ret          = 0;
    ZXIC_UINT32 resp_len    = 0;
    ZXIC_UINT8 *resp_buffer = NULL;

    DPP_AGENT_CHANNEL_REG_MSG_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(pData);

    msgcfg.devId                       = 0;
    msgcfg.type                        = DPP_REG_MSG;
    msgcfg.subtype                     = reg_type;
    msgcfg.oper                        = DPP_RD;
    msgcfg.reg_no                      = reg_no;
    msgcfg.addr                        = addr;
    msgcfg.val_len                     = reg_width / 4;

    resp_len    = reg_width + 4;
    resp_buffer = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(resp_len);
    ZXIC_COMM_CHECK_POINT(resp_buffer);

    memset(resp_buffer, 0, resp_len);

    ret = dpp_agent_channel_reg_sync_send(dev, &msgcfg, (ZXIC_UINT32 *)resp_buffer, resp_len);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(ret, "dpp_agent_channel_reg_sync_send", resp_buffer);
    
    if (DPP_OK != *((ZXIC_UINT32 *)resp_buffer))
    {
        ZXIC_COMM_TRACE_ERROR("dpp_agent_channel_reg_sync_send failed in buffer\n");
        ZXIC_COMM_FREE(resp_buffer);
        return DPP_ERR;
    }

    memcpy(pData, resp_buffer + 4, reg_width);

    ZXIC_COMM_FREE(resp_buffer);

    return DPP_OK;
}

DPP_STATUS dpp_agent_channel_dtb_sync_send(DPP_DEV_T *dev, DPP_AGENT_CHANNEL_DTB_MSG_T *pMsg, ZXIC_UINT32 *pData, ZXIC_UINT32 rep_len)
{
    DPP_STATUS ret = DPP_OK;

    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(pMsg);
    ZXIC_COMM_CHECK_POINT(pData);

    agentMsg.msg                     = (ZXIC_VOID *)pMsg;
    agentMsg.msg_len                 = sizeof(DPP_AGENT_CHANNEL_DTB_MSG_T);

    ret = dpp_agent_channel_sync_send(dev, &agentMsg, pData, rep_len);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_sync_send");

    return DPP_OK;
}

DPP_STATUS dpp_agent_channel_dtb_queue_request(DPP_DEV_T *dev, ZXIC_CONST ZXIC_UINT8 *p_name, ZXIC_UINT32 vport_info, ZXIC_UINT32 *p_queue_id)
{
    DPP_STATUS ret          = DPP_OK;
    ZXIC_UINT32 rsp_buff[2] = {0};
    ZXIC_UINT32 msg_result  = 0;
    ZXIC_UINT32 queue_id    = 0;

    DPP_AGENT_CHANNEL_DTB_MSG_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    msgcfg.devId = DEV_ID(dev);
    msgcfg.type  = DPP_DTB_MSG;
    msgcfg.oper  = QUEUE_REQUEST;
    ZXIC_COMM_MEMCPY(msgcfg.name, p_name, ZXIC_COMM_STRLEN(p_name));
    msgcfg.vport = vport_info;

    ZXIC_COMM_TRACE_INFO("msgcfg.name = %s.\n", msgcfg.name);

    ret = dpp_agent_channel_dtb_sync_send(dev, &msgcfg, rsp_buff, ZXIC_SIZEOF(rsp_buff));
    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_dtb_sync_send");

    msg_result = rsp_buff[0];
    queue_id   = rsp_buff[1];

    ZXIC_COMM_TRACE_INFO("msg_result: %d.\n", msg_result);
    ZXIC_COMM_TRACE_INFO("queue_id: %d.\n", queue_id);

    *p_queue_id = queue_id;

    return msg_result;
}

DPP_STATUS dpp_agent_channel_dtb_queue_release(DPP_DEV_T *dev, ZXIC_CONST ZXIC_UINT8 *p_name, ZXIC_UINT32 queue_id)
{
    DPP_STATUS ret          = DPP_OK;
    ZXIC_UINT32 msg_result  = 0;
    ZXIC_UINT32 rsp_buff[2] = {0};

    DPP_AGENT_CHANNEL_DTB_MSG_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    msgcfg.devId    = DEV_ID(dev);
    msgcfg.type     = DPP_DTB_MSG;
    msgcfg.oper     = QUEUE_RELEASE;
    msgcfg.queue_id = queue_id;
    ZXIC_COMM_MEMCPY(msgcfg.name, p_name, ZXIC_COMM_STRLEN(p_name));

    ZXIC_COMM_TRACE_INFO("msgcfg.name = %s.\n", msgcfg.name);

    ret = dpp_agent_channel_dtb_sync_send(dev, &msgcfg, rsp_buff, ZXIC_SIZEOF(rsp_buff));
    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_dtb_sync_send");

    msg_result = rsp_buff[0];
    ZXIC_COMM_TRACE_INFO("msg_result: %d.\n", msg_result);

    return msg_result;
}

DPP_STATUS dpp_agent_channel_dtb_queue_sync_cfg(DPP_DEV_T *dev, ZXIC_CONST ZXIC_UINT8 *p_name, ZXIC_UINT32 vport_info, ZXIC_UINT32 queue_id)
{
    DPP_STATUS ret          = DPP_OK;
    ZXIC_UINT32 rsp_buff[2] = {0};
    ZXIC_UINT32 msg_result  = 0;

    DPP_AGENT_CHANNEL_DTB_MSG_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    msgcfg.devId = DEV_ID(dev);
    msgcfg.type  = DPP_DTB_MSG;
    msgcfg.oper  = QUEUE_SYNC_CFG;
    msgcfg.queue_id = queue_id;
    ZXIC_COMM_MEMCPY(msgcfg.name, p_name, ZXIC_COMM_STRLEN(p_name));
    msgcfg.vport = vport_info;

    ZXIC_COMM_TRACE_INFO("msgcfg.name = %s.\n", msgcfg.name);

    ret = dpp_agent_channel_dtb_sync_send(dev, &msgcfg, rsp_buff, ZXIC_SIZEOF(rsp_buff));
    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_dtb_sync_send");

    msg_result = rsp_buff[0];

    ZXIC_COMM_TRACE_INFO("msg_result: %d.\n", msg_result);
    ZXIC_COMM_TRACE_INFO("queue_id: %d.\n", queue_id);

    return msg_result;
}

DPP_STATUS dpp_agent_channel_tm_sync_send(DPP_DEV_T *dev, DPP_AGENT_CHANNEL_TM_MSG_T *pMsg, ZXIC_UINT32 *pData, ZXIC_UINT32 rep_len)
{
    DPP_STATUS ret = DPP_OK;
    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(pMsg);
    ZXIC_COMM_CHECK_POINT(pData);

    agentMsg.msg                     = (ZXIC_VOID *)pMsg;
    agentMsg.msg_len                 = sizeof(DPP_AGENT_CHANNEL_TM_MSG_T);

    ret = dpp_agent_channel_sync_send(dev, &agentMsg, pData, rep_len);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_sync_send");

    // ret = *(ZXIC_UINT8 *)pData;
    // if (DPP_OK != ret)
    // {
    //     ZXIC_COMM_TRACE_ERROR("dpp_agent_channel_tm_sync_send failed in buffer\n",);
    //     return DPP_ERR;
    // }

    return DPP_OK;
}

DPP_STATUS dpp_agent_channel_tm_seid_request(DPP_DEV_T *dev, ZXIC_UINT32 port, ZXIC_UINT32 vport, ZXIC_UINT32 sche_level, ZXIC_UINT32 sche_type, ZXIC_UINT32 num, ZXIC_UINT32 *p_se_id)
{
    DPP_STATUS ret = DPP_OK;
    ZXIC_UINT32 resp_buffer[2] = {0};

    DPP_AGENT_CHANNEL_TM_MSG_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_se_id);

    msgcfg.devId = 0;
    msgcfg.type = DPP_TM_MSG;
    msgcfg.oper = SEID_REQUEST;
    msgcfg.port = port;
    msgcfg.vport = vport;
    msgcfg.sche_level = sche_level;
    msgcfg.sche_type = sche_type;
    msgcfg.num = num;
    msgcfg.se_id = SCHE_REQ_VALID;

    if (FLOW_SCHE != sche_type)
    {
        msgcfg.num = 1;
    }

    ret = dpp_agent_channel_tm_sync_send(dev, &msgcfg, resp_buffer, sizeof(resp_buffer));
    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_tm_sync_send");

    memcpy(p_se_id, resp_buffer, sizeof(ZXIC_UINT32)*SCHE_RSP_LEN);

    return ret;
}

DPP_STATUS dpp_agent_channel_tm_seid_release(DPP_DEV_T *dev, ZXIC_UINT32 port, ZXIC_UINT32 vport, ZXIC_UINT32 sche_level, ZXIC_UINT32 sche_type, ZXIC_UINT32 num, ZXIC_UINT32 se_id)
{
    DPP_STATUS ret = DPP_OK;
    ZXIC_UINT32 resp_buffer[2] = {0};

    DPP_AGENT_CHANNEL_TM_MSG_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    msgcfg.devId = 0;
    msgcfg.type = DPP_TM_MSG;
    msgcfg.oper = SEID_RELEASE;
    msgcfg.port = port;
    msgcfg.vport = vport;
    msgcfg.sche_level = sche_level;
    msgcfg.sche_type = sche_type;
    msgcfg.num = num;
    msgcfg.se_id = se_id;

    if (FLOW_SCHE != sche_type)
    {
        msgcfg.num = 1;
    }

    ret = dpp_agent_channel_tm_sync_send(dev, &msgcfg, resp_buffer, sizeof(resp_buffer));
    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_tm_sync_send");

    ret = *(ZXIC_UINT8*)resp_buffer;

    return ret;
}

DPP_STATUS dpp_agent_channel_tm_base_node_get(DPP_DEV_T *dev, ZXIC_UINT32 port, ZXIC_UINT32 vport, ZXIC_UINT32 *p_se_id)
{
    DPP_STATUS ret = DPP_OK;
    ZXIC_UINT32 resp_buffer[2] = {0};

    DPP_AGENT_CHANNEL_TM_MSG_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_se_id);

    msgcfg.devId = 0;
    msgcfg.type = DPP_TM_MSG;
    msgcfg.oper = SEID_QUERY;
    msgcfg.port = port;
    msgcfg.vport = vport;
    msgcfg.sche_level = EPID_LEVEL;
    msgcfg.sche_type = WFQ_SCHE;
    msgcfg.num = 1;
    msgcfg.se_id = SCHE_REQ_VALID;

    ret = dpp_agent_channel_tm_sync_send(dev, &msgcfg, resp_buffer, sizeof(resp_buffer));
    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_tm_sync_send");

    memcpy(p_se_id, resp_buffer, sizeof(ZXIC_UINT32)*SCHE_RSP_LEN);

    return ret;
}

DPP_STATUS dpp_agent_channel_plcr_sync_send(DPP_DEV_T *dev, DPP_AGENT_CHANNEL_PLCR_MSG_T *pMsg, ZXIC_UINT32 *pData, ZXIC_UINT32 rep_len)
{
    DPP_STATUS ret = DPP_OK;
    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(pMsg);

    agentMsg.msg = (ZXIC_VOID *)pMsg;
    agentMsg.msg_len = sizeof(DPP_AGENT_CHANNEL_PLCR_MSG_T);

    ret = dpp_agent_channel_sync_send(dev, &agentMsg, pData, rep_len);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_sync_send");

    // ret = *(ZXIC_UINT8*)pData;
    // if (DPP_OK != ret)
    // {
    //     ZXIC_COMM_TRACE_ERROR("dpp_agent_channel_sync_send failed in buffer\n");
    //     return DPP_ERR;
    // }

    return DPP_OK;
}

DPP_STATUS dpp_agent_channel_plcr_profileid_request(DPP_DEV_T *dev, ZXIC_UINT32 vport, ZXIC_UINT32 car_type, ZXIC_UINT32 *p_profileid)
{
    DPP_STATUS ret = DPP_OK;
    ZXIC_UINT32 resp_buffer[2] = {0};

    DPP_AGENT_CHANNEL_PLCR_MSG_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_profileid);

    msgcfg.devId = 0;
    msgcfg.type = DPP_PLCR_MSG;
    msgcfg.oper = PROFILEID_REQUEST;
    msgcfg.vport = vport;
    msgcfg.car_type = car_type;
    msgcfg.profile_id = PROFILEID_REQ_VALID;

    ret = dpp_agent_channel_plcr_sync_send(dev, &msgcfg, resp_buffer, sizeof(resp_buffer));
    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_plcr_sync_send");

    memcpy(p_profileid, resp_buffer, sizeof(ZXIC_UINT32)*SCHE_RSP_LEN);

    return ret;
}

DPP_STATUS dpp_agent_channel_plcr_profileid_release(DPP_DEV_T *dev, ZXIC_UINT32 vport, ZXIC_UINT32 car_type, ZXIC_UINT32 profileid)
{
    DPP_STATUS ret = DPP_OK;
    ZXIC_UINT32 resp_buffer[2] = {0};

    DPP_AGENT_CHANNEL_PLCR_MSG_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    msgcfg.devId = 0;
    msgcfg.type = DPP_PLCR_MSG;
    msgcfg.oper = PROFILEID_RELEASE;
    msgcfg.vport = vport;
    msgcfg.car_type = car_type;
    msgcfg.profile_id = profileid;

    ret = dpp_agent_channel_plcr_sync_send(dev, &msgcfg, resp_buffer, sizeof(resp_buffer));
    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_plcr_sync_send");

    ret = *(ZXIC_UINT8*)resp_buffer;

    return ret;
}

DPP_STATUS dpp_agent_channel_tm_flow_shape(DPP_DEV_T *dev, ZXIC_UINT32 flow_id, ZXIC_UINT32 cir, ZXIC_UINT32 cbs, ZXIC_UINT32 db_en, ZXIC_UINT32 eir, ZXIC_UINT32 ebs)
{
    DPP_STATUS ret = DPP_OK;
    ZXIC_UINT32 resp_buffer[2] = {0};
    ZXIC_UINT32 resp_len = 8;
    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};
    DPP_AGENT_TM_FLOW_SHAPE_MSG_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    msgcfg.devId = 0;
    msgcfg.type = DPP_TM_FLOW_SHAPE;
    msgcfg.flow_id = flow_id;
    msgcfg.cir = cir;
    msgcfg.cbs = cbs;
    msgcfg.db_en = db_en;
    msgcfg.eir = eir;
    msgcfg.ebs = ebs;

    agentMsg.msg = (ZXIC_VOID *)&msgcfg;
    agentMsg.msg_len = sizeof(DPP_AGENT_TM_FLOW_SHAPE_MSG_T);

    ret = dpp_agent_channel_sync_send(dev, &agentMsg, resp_buffer, resp_len);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_sync_send");

    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_tm_sync_send");

    ret = *(ZXIC_UINT8*)resp_buffer;

    return ret;

}

DPP_STATUS dpp_agent_channel_tm_td_set(DPP_DEV_T *dev, ZXIC_UINT32 level, ZXIC_UINT32 id, ZXIC_UINT32 td_th)
{
    DPP_STATUS ret = DPP_OK;
    ZXIC_UINT32 resp_buffer[2] = {0};
    ZXIC_UINT32 resp_len = 8;
    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};
    DPP_AGENT_TM_TD_MSG_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    msgcfg.devId = 0;
    msgcfg.type = DPP_TM_TD;
    msgcfg.level = level;
    msgcfg.id = id;
    msgcfg.td_th = td_th;

    agentMsg.msg = (ZXIC_VOID *)&msgcfg;
    agentMsg.msg_len = sizeof(DPP_AGENT_TM_TD_MSG_T);

    ret = dpp_agent_channel_sync_send(dev, &agentMsg, resp_buffer, resp_len);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_sync_send");

    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_tm_sync_send");

    ret = *(ZXIC_UINT8*)resp_buffer;

    return ret;
}

DPP_STATUS dpp_agent_channel_tm_se_shape(DPP_DEV_T *dev, ZXIC_UINT32 se_id, ZXIC_UINT32 pir, ZXIC_UINT32 pbs,ZXIC_UINT32 db_en, ZXIC_UINT32 cir, ZXIC_UINT32 cbs)
{
    DPP_STATUS ret = DPP_OK;
    ZXIC_UINT32 resp_buffer[2] = {0};
    ZXIC_UINT32 resp_len = 8;
    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};
    DPP_AGENT_TM_SE_SHAPE_MSG_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    msgcfg.devId = 0;
    msgcfg.type = DPP_TM_SE_SHAPE;
    msgcfg.se_id = se_id;
    msgcfg.pir = pir;
    msgcfg.pbs = pbs;
    msgcfg.db_en = db_en;
    msgcfg.cir = cir;
    msgcfg.cbs = cbs;

    agentMsg.msg = (ZXIC_VOID *)&msgcfg;
    agentMsg.msg_len = sizeof(DPP_AGENT_TM_SE_SHAPE_MSG_T);

    ret = dpp_agent_channel_sync_send(dev, &agentMsg, resp_buffer, resp_len);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_sync_send");

    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_tm_sync_send");

    ret = *(ZXIC_UINT8*)resp_buffer;

    return ret;
}

DPP_STATUS dpp_agent_channel_tm_port_shape(DPP_DEV_T *dev, ZXIC_UINT32 pp_port, ZXIC_UINT32 cir, ZXIC_UINT32 cbs, ZXIC_UINT32 c_en)
{
    DPP_STATUS ret = DPP_OK;
    ZXIC_UINT32 resp_buffer[2] = {0};
    ZXIC_UINT32 resp_len = 8;
    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};
    DPP_AGENT_TM_PP_SHAPE_MSG_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    msgcfg.devId = 0;
    msgcfg.type = DPP_TM_PP_SHAPE;
    msgcfg.pp_port = pp_port;
    msgcfg.cir = cir;
    msgcfg.cbs = cbs;
    msgcfg.c_en = c_en;

    agentMsg.msg = (ZXIC_VOID *)&msgcfg;
    agentMsg.msg_len = sizeof(DPP_AGENT_TM_PP_SHAPE_MSG_T);

    ret = dpp_agent_channel_sync_send(dev, &agentMsg, resp_buffer, resp_len);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_sync_send");

    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_tm_sync_send");

    ret = *(ZXIC_UINT8*)resp_buffer;

    return ret;
}

DPP_STATUS dpp_agent_channel_plcr_car_rate(DPP_DEV_T *dev, ZXIC_UINT32 car_type, ZXIC_UINT32 pkt_sign, ZXIC_UINT32 profile_id, ZXIC_VOID* p_car_profile_cfg)
{
    DPP_STATUS ret = DPP_OK;
    ZXIC_UINT32 resp_buffer[2] = {0};
    ZXIC_UINT32 resp_len = 8;
    ZXIC_UINT32 i = 0;
    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};
    DPP_AGENT_CAR_PKT_PROFILE_MSG_T msgpktcfg = {0};
    DPP_AGENT_CAR_PROFILE_MSG_T msgcfg = {0};
    DPP_STAT_CAR_PROFILE_CFG_T *p_stat_car_profile_cfg = NULL;
    DPP_STAT_CAR_PKT_PROFILE_CFG_T *p_stat_pkt_car_profile_cfg = NULL;

    ZXIC_COMM_CHECK_POINT(dev);

    if ((STAT_CAR_A_TYPE == car_type) && (1 == pkt_sign))
    {
        p_stat_pkt_car_profile_cfg = (DPP_STAT_CAR_PKT_PROFILE_CFG_T *)p_car_profile_cfg;
        msgpktcfg.devId = 0;
        msgpktcfg.type = DPP_PLCR_CAR_PKT_RATE;
        msgpktcfg.car_level = car_type;
        msgpktcfg.cir = p_stat_pkt_car_profile_cfg->cir;
        msgpktcfg.cbs = p_stat_pkt_car_profile_cfg->cbs;
        msgpktcfg.profile_id = p_stat_pkt_car_profile_cfg->profile_id;
        msgpktcfg.pkt_sign = p_stat_pkt_car_profile_cfg->pkt_sign;
        for (i = 0; i < DPP_CAR_PRI_MAX; i++)
        {
            msgpktcfg.pri[i] = p_stat_pkt_car_profile_cfg->pri[i];
        }

        agentMsg.msg = (ZXIC_VOID *)&msgpktcfg;
        agentMsg.msg_len = sizeof(DPP_AGENT_CAR_PKT_PROFILE_MSG_T);

        ret = dpp_agent_channel_sync_send(dev, &agentMsg, resp_buffer, resp_len);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_sync_send");

        ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_tm_sync_send");

        ret = *(ZXIC_UINT8*)resp_buffer;   
    }
    else
    {
        p_stat_car_profile_cfg = (DPP_STAT_CAR_PROFILE_CFG_T *)p_car_profile_cfg;
        msgcfg.devId = 0;
        msgcfg.type = DPP_PLCR_CAR_RATE;
        msgcfg.car_level = car_type;
        msgcfg.cir = p_stat_car_profile_cfg->cir;
        msgcfg.cbs = p_stat_car_profile_cfg->cbs;
        msgcfg.profile_id = p_stat_car_profile_cfg->profile_id;
        msgcfg.pkt_sign = p_stat_car_profile_cfg->pkt_sign;
        msgcfg.cd = p_stat_car_profile_cfg->cd;
        msgcfg.cf = p_stat_car_profile_cfg->cf;
        msgcfg.cm = p_stat_car_profile_cfg->cm;
        msgcfg.cir = p_stat_car_profile_cfg->cir;
        msgcfg.cbs = p_stat_car_profile_cfg->cbs;
        msgcfg.eir = p_stat_car_profile_cfg->eir;
        msgcfg.ebs = p_stat_car_profile_cfg->ebs;
        msgcfg.random_disc_e = p_stat_car_profile_cfg->random_disc_e;
        msgcfg.random_disc_c = p_stat_car_profile_cfg->random_disc_c;
        for (i = 0; i < DPP_CAR_PRI_MAX; i++)
        {
            msgcfg.c_pri[i] = p_stat_car_profile_cfg->c_pri[i];
            msgcfg.e_green_pri[i] = p_stat_car_profile_cfg->e_green_pri[i];
            msgcfg.e_yellow_pri[i] = p_stat_car_profile_cfg->e_yellow_pri[i];
        }

        agentMsg.msg = (ZXIC_VOID *)&msgcfg;
        agentMsg.msg_len = sizeof(DPP_AGENT_CAR_PROFILE_MSG_T);

        ret = dpp_agent_channel_sync_send(dev, &agentMsg, resp_buffer, resp_len);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_sync_send");

        //ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_tm_sync_send");

        ret = *(ZXIC_UINT8*)resp_buffer;
    }

    return ret;    
}

DPP_STATUS dpp_agent_channel_ppu_thash_rsk(DPP_DEV_T *dev, DPP_PPU_THASH_RSK_OPER_E oper, DPP_PPU_PPU_COP_THASH_RSK_T *p_para)
{
    DPP_STATUS ret = DPP_OK;
    ZXIC_UINT32 resp_buffer = 0;
    DPP_PPU_PPU_COP_THASH_RSK_T thash = {0};
    DPP_AGENT_CHANNEL_MSG_T agentMsg   = {0};
    DPP_AGENT_PPU_THASH_RSK_MSG_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_para);
    ZXIC_COMM_CHECK_INDEX(oper, DPP_PPU_THASH_RSK_RD, DPP_PPU_THASH_RSK_WR);

    switch (oper)
    {
        case DPP_PPU_THASH_RSK_RD:
            msgcfg.devId       = 0;
            msgcfg.type        = DPP_PPU_THASH_RSK;
            msgcfg.oper        = oper;
            msgcfg.rsv         = 0;
            msgcfg.rsk_031_000 = 0;
            msgcfg.rsk_063_032 = 0;
            msgcfg.rsk_095_064 = 0;
            msgcfg.rsk_127_096 = 0;
            msgcfg.rsk_159_128 = 0;
            msgcfg.rsk_191_160 = 0;
            msgcfg.rsk_223_192 = 0;
            msgcfg.rsk_255_224 = 0;
            msgcfg.rsk_287_256 = 0;
            msgcfg.rsk_319_288 = 0;

            agentMsg.msg = (ZXIC_VOID *)&msgcfg;
            agentMsg.msg_len = sizeof(DPP_AGENT_PPU_THASH_RSK_MSG_T);
        
            ret = dpp_agent_channel_sync_send(dev, &agentMsg, (ZXIC_UINT32*)&thash, sizeof(DPP_PPU_PPU_COP_THASH_RSK_T));
            ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_sync_send");

            memcpy(p_para, &thash, sizeof(DPP_PPU_PPU_COP_THASH_RSK_T));
            break;

        case DPP_PPU_THASH_RSK_WR:
            msgcfg.devId       = 0;
            msgcfg.type        = DPP_PPU_THASH_RSK;
            msgcfg.oper        = oper;
            msgcfg.rsv         = 0;
            msgcfg.rsk_031_000 = p_para->rsk_031_000;
            msgcfg.rsk_063_032 = p_para->rsk_063_032;
            msgcfg.rsk_095_064 = p_para->rsk_095_064;
            msgcfg.rsk_127_096 = p_para->rsk_127_096;
            msgcfg.rsk_159_128 = p_para->rsk_159_128;
            msgcfg.rsk_191_160 = p_para->rsk_191_160;
            msgcfg.rsk_223_192 = p_para->rsk_223_192;
            msgcfg.rsk_255_224 = p_para->rsk_255_224;
            msgcfg.rsk_287_256 = p_para->rsk_287_256;
            msgcfg.rsk_319_288 = p_para->rsk_319_288;

            agentMsg.msg     = (ZXIC_VOID *)&msgcfg;
            agentMsg.msg_len = sizeof(DPP_AGENT_PPU_THASH_RSK_MSG_T);

            ret = dpp_agent_channel_sync_send(dev, &agentMsg, &resp_buffer, sizeof(ZXIC_UINT32));
            ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_sync_send");

            ret = resp_buffer;
            break;

        default:
            ZXIC_COMM_TRACE_ERROR("The message to ppu_thash_rsk is not defined\n");
            ret =  DPP_ERR;
            break;
    }

    return ret;
}

DPP_STATUS dpp_agent_channel_pktrx_ind_reg_rw(DPP_DEV_T *dev,
                                              ZXIC_UINT32 mem_addr,
                                              ZXIC_UINT32 mem_id,
                                              ZXIC_UINT32 oper,
                                              ZXIC_UINT32 len,
                                              ZXIC_UINT32 *p_data)
{
    DPP_STATUS ret = DPP_OK;
    ZXIC_UINT32 resp_buffer = 0;
    ZXIC_UINT32 oper_len = 0;
    ZXIC_UINT32 data[8] = {0};
    DPP_AGENT_CHANNEL_MSG_T agentMsg   = {0};
    DPP_AGENT_PKTRX_IND_REG_RW_MSG_T pktrx_ind_msg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_data);
    ZXIC_COMM_CHECK_INDEX(mem_id, 0, (MEM_ID_MUX_NUM - 1));
    ZXIC_COMM_CHECK_INDEX(len, 1, 4 * 8);
    ZXIC_COMM_CHECK_INDEX(mem_addr, 0, (1 << 12) - 1);
    ZXIC_COMM_CHECK_INDEX(oper, DPP_PKTRX_IND_REG_RD, DPP_PKTRX_IND_REG_WR);

    ZXIC_COMM_MEMSET_S(&pktrx_ind_msg, sizeof(DPP_AGENT_PKTRX_IND_REG_RW_MSG_T), 0, sizeof(DPP_AGENT_PKTRX_IND_REG_RW_MSG_T));

    oper_len = (len % 4 != 0) ? (len / 4 + 1) : (len / 4);

    pktrx_ind_msg.devId = 0;
    pktrx_ind_msg.type = DPP_PKTRX_IND_REG_RW_MSG;
    pktrx_ind_msg.oper = oper;
    pktrx_ind_msg.rsv = 0;
    pktrx_ind_msg.mem_addr = mem_addr;
    pktrx_ind_msg.mem_id = mem_id;
    pktrx_ind_msg.len = len;

    agentMsg.msg = (ZXIC_VOID *)&pktrx_ind_msg;
    agentMsg.msg_len = sizeof(DPP_AGENT_PKTRX_IND_REG_RW_MSG_T);

    switch (oper)
    {
        case DPP_PKTRX_IND_REG_RD:
            ret = dpp_agent_channel_sync_send(dev, &agentMsg, data, 32);
            ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_sync_send");

            ZXIC_COMM_MEMCPY_S(p_data, oper_len * 4, data, oper_len * 4);
            break;

        case DPP_PKTRX_IND_REG_WR:
            ZXIC_COMM_MEMCPY_S(pktrx_ind_msg.ind_data, 32, p_data, oper_len * 4);
            ret = dpp_agent_channel_sync_send(dev, &agentMsg, &resp_buffer, sizeof(ZXIC_UINT32));
            ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_sync_send");

            ret = resp_buffer;
            break;

        default:
            ZXIC_COMM_TRACE_ERROR("The message to ppu_thash_rsk is not defined\n");
            ret =  DPP_ERR;
            break;
    }

    return ret;
}

/***********************************************************/
/** 通过代理通道申请acl index
* @param   dev          设备   
* @param   sdt_no       sdt号
* @param   vport        端口号
* @param   p_index      出参，申请到的acl index
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/09/14
************************************************************/
DPP_STATUS dpp_agent_channel_acl_index_request(DPP_DEV_T *dev,
                                               ZXIC_UINT32 sdt_no, 
                                               ZXIC_UINT32 vport,
                                               ZXIC_UINT32 *p_index)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 rsp_buff[2] = {0};
    ZXIC_UINT32 msg_result = 0;
    ZXIC_UINT32 acl_index = 0;
    DPP_AGENT_CHANNEL_ACL_MSG_T msgcfg = {0};
    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_index);

    msgcfg.devId = 0;  //在ricv上使用，只有1个np
    msgcfg.type = DPP_ACL_MSG;
    msgcfg.oper = ACL_INDEX_REQUEST;
    msgcfg.vport = vport;
    msgcfg.sdt_no = sdt_no;
    
    agentMsg.msg = (ZXIC_VOID*)&msgcfg;
    agentMsg.msg_len = sizeof(DPP_AGENT_CHANNEL_ACL_MSG_T);
    rc = dpp_agent_channel_sync_send(dev, &agentMsg,rsp_buff,ZXIC_SIZEOF(rsp_buff));
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_agent_channel_sync_send");

    msg_result = rsp_buff[0];
    acl_index = rsp_buff[1];

    ZXIC_COMM_TRACE_INFO("dev_id: %d, msg_result: %d\n", dev_id, msg_result);
    ZXIC_COMM_TRACE_INFO("dev_id: %d, acl_index: %d\n", dev_id, acl_index);

    *p_index = acl_index;

    return msg_result;
}

/***********************************************************/
/** 通过代理通道释放acl index
* @param   dev          设备
* @param   rel_type     释放类型，详见MSG_ACL_INDEX_OPER_E
* @param   sdt_no       sdt号
* @param   vport        端口号
* @param   p_index      指定释放的index
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/09/14
************************************************************/
DPP_STATUS dpp_agent_channel_acl_index_release(DPP_DEV_T *dev,
                                               ZXIC_UINT32 rel_type,
                                               ZXIC_UINT32 sdt_no, 
                                               ZXIC_UINT32 vport,
                                               ZXIC_UINT32 index)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 msg_result = 0;
    ZXIC_UINT32 rsp_buff[2] = {0};
    DPP_AGENT_CHANNEL_ACL_MSG_T msgcfg = {0};
    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);

    msgcfg.devId = 0;  //在ricv上使用，只有1个np
    msgcfg.type = DPP_ACL_MSG;
    msgcfg.oper = rel_type;
    msgcfg.index = index;
    msgcfg.sdt_no = sdt_no;
    msgcfg.vport = vport;

    agentMsg.msg = (ZXIC_VOID*)&msgcfg;
    agentMsg.msg_len = sizeof(DPP_AGENT_CHANNEL_ACL_MSG_T);
    rc = dpp_agent_channel_sync_send(dev, &agentMsg,rsp_buff,ZXIC_SIZEOF(rsp_buff));
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_agent_channel_sync_send");

    msg_result = rsp_buff[0];
    ZXIC_COMM_TRACE_INFO("msg_result: %d\n", msg_result);

    return msg_result;
}

/***********************************************************/
/** 发送消息给riscv读清stat
* @param   dev          设备
* @param   count_id     统计编号，对应微码中的address
* @param   rd_mode      读取位宽模式，参见STAT_CNT_MODE_E，0-64bit，1-128bit
* @param   num          连续读清的统计个数
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/09/14
************************************************************/
DPP_STATUS dpp_agent_channel_stat_clr(DPP_DEV_T *dev,
                                    ZXIC_UINT32 count_id,
                                    ZXIC_UINT32 rd_mode,
                                    ZXIC_UINT32 num)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 msg_result = 0;
    ZXIC_UINT32 rsp_buff[2] = {0};
    DPP_AGENT_CHANNEL_STAT_MSG_T msgcfg = {0};
    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);

    msgcfg.devId = 0;  //在ricv上使用，只有1个np
    msgcfg.type = DPP_STAT_MSG;
    msgcfg.oper = 0;
    msgcfg.counter_id = count_id;
    msgcfg.rd_mode = rd_mode;
    msgcfg.num = num;

    agentMsg.msg = (ZXIC_VOID*)&msgcfg;
    agentMsg.msg_len = sizeof(DPP_AGENT_CHANNEL_STAT_MSG_T);
    rc = dpp_agent_channel_sync_send(dev, &agentMsg,rsp_buff,ZXIC_SIZEOF(rsp_buff));
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_agent_channel_sync_send");

    msg_result = rsp_buff[0];
    ZXIC_COMM_TRACE_INFO("msg_result: %d\n", msg_result);

    return msg_result;
}

/***********************************************************/
/** 读清acl表项对应的stat表项
* @param   dev          设备，支持多芯片
* @param   sdt_no       sdt号
* @param   vport        端口号
* @param   counter_id   统计编号，对应微码中的address
* @param   rd_mode      读取位宽模式，参见STAT_CNT_MODE_E，0-64bit，1-128bit
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/09/14
************************************************************/
DPP_STATUS dpp_agent_channel_acl_stat_clr(DPP_DEV_T *dev,
                                    ZXIC_UINT32 sdt_no,
                                    ZXIC_UINT32 vport,
                                    ZXIC_UINT32 counter_id,
                                    ZXIC_UINT32 rd_mode)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;

    ZXIC_UINT32 msg_result = 0;
    ZXIC_UINT32 rsp_buff[2] = {0};
    DPP_AGENT_CHANNEL_ACL_MSG_T msgcfg = {0};
    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);

    msgcfg.devId = 0;  //在ricv上使用，只有1个np
    msgcfg.type = DPP_ACL_MSG;
    msgcfg.oper = ACL_INDEX_STAT_CLR;
    msgcfg.sdt_no = sdt_no;
    msgcfg.vport = vport;
    msgcfg.counter_id = counter_id;
    msgcfg.rd_mode = rd_mode;

    agentMsg.msg = (ZXIC_VOID*)&msgcfg;
    agentMsg.msg_len = sizeof(DPP_AGENT_CHANNEL_ACL_MSG_T);
    rc = dpp_agent_channel_sync_send(dev, &agentMsg,rsp_buff,ZXIC_SIZEOF(rsp_buff));
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_agent_channel_sync_send");

    msg_result = rsp_buff[0];
    ZXIC_COMM_TRACE_INFO("msg_result: %d\n", msg_result);

    return msg_result;
}
/***********************************************************/
/** 从固件获取流表资源
* @param   dev          设备，支持多芯片
* @param   sub_type     标卡资源或者非标卡资源MSG_RES_TYPE_E
* @param   opr          请求资源类型MSG_SE_RES_OPER_E
* @param   p_rsp_buff   出参，rsp[0]+se_res
* @param   buff_size    资源缓存大小
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/09/14
************************************************************/
DPP_STATUS dpp_agent_channel_se_res_get(DPP_DEV_T *dev,
                                    ZXIC_UINT32 sub_type,
                                    ZXIC_UINT32 opr,
                                    ZXIC_UINT32 *p_rsp_buff,
                                    ZXIC_UINT32 buff_size)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 msg_result = 0;
    DPP_AGENT_SE_RES_MSG_T msgcfg = {0};
    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(opr, 0, RES_REQ_MAX - 1);
    ZXIC_COMM_CHECK_POINT_NO_ASSERT(p_rsp_buff);

    msgcfg.devId = 0;  //在ricv上使用，只有1个np
    msgcfg.type = DPP_RES_MSG;
    msgcfg.sub_type = sub_type;
    msgcfg.oper = opr;
    agentMsg.msg = (ZXIC_VOID*)&msgcfg;
    agentMsg.msg_len = sizeof(DPP_AGENT_SE_RES_MSG_T);

    rc = dpp_agent_channel_sync_send(dev, &agentMsg,p_rsp_buff,buff_size);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_agent_channel_sync_send");

    msg_result = p_rsp_buff[0];
    ZXIC_COMM_TRACE_INFO("msg_result: %d\n", msg_result);
    dpp_agent_msg_prt(msgcfg.type,msg_result);

    return msg_result;
}

/***********************************************************/
/** 通过代理通道获取bar消息个数
* @param   dev              设备
* @param   p_pcie_bar_num   出参，bar消息个数
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/11/16
************************************************************/
DPP_STATUS dpp_agent_channel_pcie_bar_request(DPP_DEV_T *dev,
                                               ZXIC_UINT32 *p_bar_msg_num)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 rsp_buff[2] = {0};
    ZXIC_UINT32 msg_result = 0;
    ZXIC_UINT32 bar_msg_num = 0;
    DPP_AGENT_PCIE_BAR_MSG_T msgcfg = {0};
    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_bar_msg_num);
    
    msgcfg.devId = 0;  //在ricv上使用，只有1个np
    msgcfg.type = DPP_PCIE_BAR_MSG;
    msgcfg.oper = BAR_MSG_NUM_REQ;
    agentMsg.msg = (ZXIC_VOID*)&msgcfg;
    agentMsg.msg_len = sizeof(DPP_AGENT_PCIE_BAR_MSG_T);

    rc = dpp_agent_channel_sync_send(dev, &agentMsg, rsp_buff, ZXIC_SIZEOF(rsp_buff));
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_agent_channel_dtb_sync_send");

    msg_result = rsp_buff[0];
    bar_msg_num = rsp_buff[1];
    ZXIC_COMM_TRACE_INFO("dev_id: %d, msg_result: %d\n", dev_id, msg_result);
    ZXIC_COMM_TRACE_INFO("dev_id: %d, bar_num: %d\n", dev_id, bar_msg_num);
    dpp_agent_msg_prt(msgcfg.type,msg_result);

    *p_bar_msg_num = bar_msg_num;

    return msg_result;
}

/***********************************************************/
/** 通过代理通道把双平面配置写入L2D
* @param   dev      设备
* @param   psn_cfg  双平面配置
* @return  
* @remark  无
* @see     
* @author  cbb      @date  2024/11/16
************************************************************/
DPP_STATUS dpp_agent_channel_psn_cfg_l2d_write(DPP_DEV_T *dev,
                                               ZXIC_UINT8 psn_cfg)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 msg_result = 0;
    DPP_AGENT_PSN_CFG_MSG_T msgcfg = {0};
    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    
    msgcfg.devId = 0;  //在ricv上使用，只有1个np
    msgcfg.type = DPP_PSN_CFG_MSG;
    msgcfg.oper = PSN_CFG_L2D_WR;
    msgcfg.psn = psn_cfg;
    agentMsg.msg = (ZXIC_VOID*)&msgcfg;
    agentMsg.msg_len = sizeof(DPP_AGENT_PSN_CFG_MSG_T);

    rc = dpp_agent_channel_sync_send(dev, &agentMsg, &msg_result, ZXIC_SIZEOF(msg_result));
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_agent_channel_sync_send");

    ZXIC_COMM_TRACE_INFO("dev_id: %d, msg_result: %d\n", dev_id, msg_result);
    dpp_agent_msg_prt(msgcfg.type, msg_result);

    return msg_result;
}

/***********************************************************/
/** 通过代理通道从L2D中读出双平面配置
* @param   dev      设备
* @param   psn_cfg  双平面配置
* @return  
* @remark  无
* @see     
* @author  cbb      @date  2024/11/16
************************************************************/
DPP_STATUS dpp_agent_channel_psn_cfg_l2d_read(DPP_DEV_T *dev,
                                            ZXIC_UINT32 * p_psn_cfg)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 rsp_buff[2] = {0};
    ZXIC_UINT32 msg_result = 0;
    DPP_AGENT_PSN_CFG_MSG_T msgcfg = {0};
    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_psn_cfg);
    
    msgcfg.devId = 0;  //在ricv上使用，只有1个np
    msgcfg.type = DPP_PSN_CFG_MSG;
    msgcfg.oper = PSN_CFG_L2D_RD;
    agentMsg.msg = (ZXIC_VOID*)&msgcfg;
    agentMsg.msg_len = sizeof(DPP_AGENT_PSN_CFG_MSG_T);

    rc = dpp_agent_channel_sync_send(dev, &agentMsg, rsp_buff, ZXIC_SIZEOF(rsp_buff));
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_agent_channel_sync_send");

    msg_result = rsp_buff[0];
    ZXIC_COMM_TRACE_NOTICE("dev_id: %d, msg_result: %d\n", dev_id, msg_result);
    
    dpp_agent_msg_prt(msgcfg.type, msg_result);

    *p_psn_cfg = rsp_buff[1];

    ZXIC_COMM_TRACE_NOTICE("dev_id: %d, psn_cfg: %d\n", dev_id, rsp_buff[1]);

    return msg_result;
}

DPP_STATUS dpp_agent_channel_np_flow_monitor(DPP_DEV_T *dev, ZXIC_UINT32 vport, ZXIC_UINT32 flag)
{
    DPP_STATUS ret = DPP_OK;
    ZXIC_UINT32 resp_buffer[2] = {0};
    ZXIC_UINT32 resp_len = 8;
    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};
    DPP_AGENT_CHANNEL_FLOW_MONITOR_MSG_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    msgcfg.devId = 0;
    msgcfg.type = DPP_FLOW_MONITOR_MSG;
    msgcfg.vport = vport;
    msgcfg.vport_type = 1;
    msgcfg.flag = flag;

    agentMsg.msg = (ZXIC_VOID *)&msgcfg;
    agentMsg.msg_len = sizeof(DPP_AGENT_CHANNEL_FLOW_MONITOR_MSG_T);

    ret = dpp_agent_channel_sync_send(dev, &agentMsg, resp_buffer, resp_len);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_sync_send");

    ret = *(ZXIC_UINT8*)resp_buffer;

    return ret;
}

DPP_STATUS dpp_agent_channel_np_ingress_buffer_set(DPP_DEV_T *dev,
                                                   ZXIC_UINT32 port,
                                                   ZXIC_UINT32 mode,
                                                   ZXIC_UINT32 *buffer_size,
                                                   ZXIC_UINT32 *threshold_size)
{
    DPP_STATUS ret = DPP_OK;
    ZXIC_UINT32 resp_buffer[2] = {0};
    ZXIC_UINT32 resp_len = 8;
    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};
    DPP_AGENT_CHANNEL_BUFFER_MSG_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(buffer_size);
    ZXIC_COMM_CHECK_POINT(threshold_size);

    msgcfg.devId = 0;
    msgcfg.type = DPP_DHTOOL_QOS_MSG;
    msgcfg.phy_port = port;
    msgcfg.mode = mode;
    memcpy(&msgcfg.buffer_size, buffer_size, sizeof(ZXIC_UINT32)*8);
    memcpy(&msgcfg.threshold_size, threshold_size, sizeof(ZXIC_UINT32)*8);

    agentMsg.msg = (ZXIC_VOID *)&msgcfg;
    agentMsg.msg_len = sizeof(DPP_AGENT_CHANNEL_BUFFER_MSG_T);

    ret = dpp_agent_channel_sync_send(dev, &agentMsg, resp_buffer, resp_len);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_sync_send");

    ret = *(ZXIC_UINT8*)resp_buffer;

    return ret;
}

DPP_STATUS dpp_agent_channel_np_ingress_buffer_get(DPP_DEV_T *dev,
                                                   ZXIC_UINT32 port,
                                                   ZXIC_UINT32 *panel_buffer_size,
                                                   ZXIC_UINT32 *panel_threshold_size,
                                                   ZXIC_UINT32 *internal_buffer_size,
                                                   ZXIC_UINT32 *internal_threshold_size)
{
    DPP_STATUS ret = DPP_OK;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 resp_buffer[34] = {0};
    ZXIC_UINT32 resp_len = sizeof(resp_buffer);
    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};
    DPP_AGENT_CHANNEL_BUFFER_GET_MSG_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(panel_buffer_size);
    ZXIC_COMM_CHECK_POINT(panel_threshold_size);
    ZXIC_COMM_CHECK_POINT(internal_buffer_size);
    ZXIC_COMM_CHECK_POINT(internal_threshold_size);

    msgcfg.devId = 0;
    msgcfg.type = DPP_DHTOOL_QOS_GET_MSG;
    msgcfg.phy_port = port;

    agentMsg.msg = (ZXIC_VOID *)&msgcfg;
    agentMsg.msg_len = sizeof(DPP_AGENT_CHANNEL_BUFFER_GET_MSG_T);

    ret = dpp_agent_channel_sync_send(dev, &agentMsg, resp_buffer, resp_len);
    if (ret != DPP_OK)
    {
        ZXIC_COMM_TRACE_ERROR("dpp_agent_channel_sync_send failed with error code: %d\n", ret);
        return ret;
    }

    // 解析响应数据
    // 假设 resp_buffer 的格式如下：
    // [0]   : status
    // [1-8] : panel_buffer_size[0~7]
    // [9-16]: panel_threshold_size[0~7]
    // [17-24]: internal_buffer_size[0~7]
    // [25-32]: internal_threshold_size[0~7]

    if (*(ZXIC_UINT8 *)resp_buffer != 0)
    {
        ZXIC_COMM_TRACE_ERROR("Agent channel response indicates error: %u\n", *(ZXIC_UINT8 *)resp_buffer);
        return DPP_ERR;
    }

    for (i = 0; i < 8; ++i)
    {
        panel_buffer_size[i] = resp_buffer[1 + i];
        panel_threshold_size[i] = resp_buffer[9 + i];
        internal_buffer_size[i] = resp_buffer[17 + i];
        internal_threshold_size[i] = resp_buffer[25 + i];
    }

    return DPP_OK;
}


/***********************************************************/
/** 代理通道获取相应统计信息
* @param   dev         设备
* @param   phy_port    队列号
* @param   stat_info   获取统计需要的信息
* @return  
* @remark  无
* @see     
* @author  zth      @date  2025/12/01
************************************************************/
DPP_STATUS dpp_agent_channel_prio_stat_get(DPP_DEV_T *dev, DPP_AGENT_CHANNEL_STAT_INFO_T *stat_info, ZXIC_VOID* p_stat_data)
{
    DPP_STATUS ret = DPP_OK;
    ZXIC_UINT32 resp_len = sizeof(DPP_ODMA_PORT_STAT_T) * 8 + sizeof(DPP_TM_ETS_PRIO_STAT_T) * 8 + sizeof(ZXIC_UINT32);
    ZXIC_UINT32 *p_resp_buffer= NULL;
    DPP_PRIO_STAT_DATA_T *p_np_stat_data = NULL;
    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};
    DPP_AGENT_CHANNEL_NP_STAT_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(stat_info);
    ZXIC_COMM_CHECK_POINT(p_stat_data);

    msgcfg.devId = 0;
    msgcfg.type = DPP_PRIO_STAT_MSG;
    msgcfg.stat_info.phy_port = stat_info->phy_port;
    msgcfg.stat_info.flow_id = stat_info->flow_id;

    agentMsg.msg = (ZXIC_VOID *)&msgcfg;
    agentMsg.msg_len = sizeof(DPP_AGENT_CHANNEL_NP_STAT_T);

    p_resp_buffer = (ZXIC_UINT32 *)ZXIC_COMM_MALLOC(resp_len);

    ret = dpp_agent_channel_sync_send(dev, &agentMsg, p_resp_buffer, resp_len);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(ret, "dpp_agent_channel_sync_send", p_resp_buffer);

    if (DPP_OK != p_resp_buffer[0])
    {
        ZXIC_COMM_TRACE_ERROR("dpp_agent_channel_sync_send failed in buffer\n");
        ZXIC_COMM_FREE(p_resp_buffer);
        return DPP_ERR;
    }

    
    // 将返回的数据填充到结构体中
    p_np_stat_data = (DPP_PRIO_STAT_DATA_T *)p_stat_data;
    
    // 复制 ODMA 统计数据
    ZXIC_COMM_MEMCPY(p_np_stat_data->odma_stat_data, 
                    (ZXIC_UINT8*)p_resp_buffer + sizeof(ZXIC_UINT32), 
                    sizeof(DPP_ODMA_PORT_STAT_T) * 8);
    
    // 复制 TM ETS 优先级统计数据
    ZXIC_COMM_MEMCPY(p_np_stat_data->tm_ets_stat_data, 
                    (ZXIC_UINT8*)p_resp_buffer + sizeof(ZXIC_UINT32) + sizeof(DPP_ODMA_PORT_STAT_T) * 8, 
                    sizeof(DPP_TM_ETS_PRIO_STAT_T) * 8);

    ZXIC_COMM_FREE(p_resp_buffer);
    
    return ret;
}

DPP_STATUS dpp_agent_channel_np_pktrx_trust_set(DPP_DEV_T *dev, ZXIC_UINT32 port, ZXIC_UINT32 mode, ZXIC_UINT8 id)
{
    DPP_STATUS ret = DPP_OK;
    ZXIC_UINT32 resp_buffer[2] = {0};
    ZXIC_UINT32 resp_len = 8;
    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};
    DPP_AGENT_CHANNEL_PKTRX_TRUST_MSG_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    msgcfg.devId = 0;
    msgcfg.type = id;
    msgcfg.phy_port = port;
    msgcfg.mode = mode;

    agentMsg.msg = (ZXIC_VOID *)&msgcfg;
    agentMsg.msg_len = sizeof(DPP_AGENT_CHANNEL_PKTRX_TRUST_MSG_T);

    ret = dpp_agent_channel_sync_send(dev, &agentMsg, resp_buffer, resp_len);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(ret, "dpp_agent_channel_sync_send");

    ret = *(ZXIC_UINT8*)resp_buffer;

    return ret;
}

DPP_STATUS dpp_agent_channel_msg_nppu_tcam_enable_set(DPP_DEV_T *dev, ZXIC_UINT32 index, ZXIC_UINT32 en)
{
    DPP_STATUS ret = DPP_OK;
    ZXIC_UINT32 resp = 0;
    DPP_AGENT_CHANNEL_MSG_T agentMsg = {0};
    DPP_AGENT_CHANNEL_NPPU_TCAM_ENABLE_MSG_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(index, 0, (DPP_PKTRX_ICU_TCAM_NUM - 1));
    ZXIC_COMM_CHECK_INDEX(en, 0, 1);

    msgcfg.devId = 0;
    msgcfg.type = DPP_NPPU_TCAM_ENABLE_SET_MSG;
    msgcfg.index = (ZXIC_UINT16)index;
    msgcfg.enable = en;

    agentMsg.msg = (ZXIC_VOID *)&msgcfg;
    agentMsg.msg_len = sizeof(DPP_AGENT_CHANNEL_NPPU_TCAM_ENABLE_MSG_T);

    ret = dpp_agent_channel_sync_send(dev, &agentMsg, &resp, sizeof(ZXIC_UINT32));
    ZXIC_COMM_CHECK_RC(ret, "dpp_agent_channel_sync_send");

    if (DPP_OK != resp)
    {
        ZXIC_COMM_TRACE_ERROR("dpp_agent_channel_msg_nppu_tcam_enable_set failed, ret = %u\n", resp);
        return DPP_ERR;
    }
    
    return DPP_OK;
}

DPP_STATUS dpp_agent_channel_msg_nppu_tcam_pfc_set(DPP_DEV_T *dev, ZXIC_UINT8 pfc_map)
{
    DPP_STATUS ret                              = DPP_OK;
    ZXIC_UINT32 resp                            = 0;
    DPP_AGENT_CHANNEL_MSG_T agentMsg            = {0};
    DPP_RISCV_RCV_CHANNEL_TCAM_PFC_MSG_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    msgcfg.devId   = 0;
    msgcfg.type    = DPP_NPPU_TCAM_PFC_SET_MSG;
    msgcfg.option  = TCAM_PFC_OPER_SET;
    msgcfg.pfc_map = pfc_map;

    agentMsg.msg     = (ZXIC_VOID *)&msgcfg;
    agentMsg.msg_len = sizeof(DPP_RISCV_RCV_CHANNEL_TCAM_PFC_MSG_T);

    ret = dpp_agent_channel_sync_send(dev, &agentMsg, &resp, sizeof(ZXIC_UINT32));
    ZXIC_COMM_CHECK_RC(ret, "dpp_agent_channel_sync_send");

    if (DPP_OK != resp)
    {
        ZXIC_COMM_TRACE_ERROR("dpp_agent_channel_msg_nppu_tcam_pfc_set failed, ret = %u\n", resp);
        return DPP_ERR;
    }

    return DPP_OK;
}

DPP_STATUS dpp_agent_channel_msg_nppu_tcam_pfc_get(DPP_DEV_T *dev, ZXIC_UINT8 *pfc_map)
{
    DPP_STATUS ret                              = DPP_OK;
    ZXIC_UINT32 resp                            = 0;
    DPP_AGENT_CHANNEL_MSG_T agentMsg            = {0};
    DPP_RISCV_RCV_CHANNEL_TCAM_PFC_MSG_T msgcfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(pfc_map);

    msgcfg.devId   = 0;
    msgcfg.type    = DPP_NPPU_TCAM_PFC_SET_MSG;
    msgcfg.option  = TCAM_PFC_OPER_GET;
    msgcfg.pfc_map = 1;

    agentMsg.msg     = (ZXIC_VOID *)&msgcfg;
    agentMsg.msg_len = sizeof(DPP_RISCV_RCV_CHANNEL_TCAM_PFC_MSG_T);

    ret = dpp_agent_channel_sync_send(dev, &agentMsg, &resp, sizeof(ZXIC_UINT32));
    ZXIC_COMM_CHECK_RC(ret, "dpp_agent_channel_sync_send");

    if ((ZXIC_UINT8)resp != 0 && (ZXIC_UINT8)resp != 0xff && ((ZXIC_UINT8)resp & 0x1))
    {
        ZXIC_COMM_TRACE_ERROR("pfc_map %u is invalid\n", (ZXIC_UINT8)resp);
        return DPP_ERR;
    }

    *pfc_map = (ZXIC_UINT8)resp;

    return DPP_OK;
}