#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/if_bridge.h>
#include <linux/dinghai/driver.h>
#include <linux/dinghai/dh_cmd.h>
#include "msg_chan_priv.h"
#include "msg_chan_lock.h"
#include "msg_common.h"

/*****************************************
[src/dst]时应该将消息发到低2k(0)还是高2K(1)
src/dst: TO_RISC, TO_PFVF, TO_MPF
MPF:       0         0       0
PF:        0         0       1
VF:        0         1       1
******************************************/
uint8_t subchan_id_tbl[BAR_MSG_SRC_NUM][BAR_MSG_DST_NUM] =
{
    {BAR_SUBCHAN_INDEX_SEND, BAR_SUBCHAN_INDEX_SEND, BAR_SUBCHAN_INDEX_SEND},
    {BAR_SUBCHAN_INDEX_SEND, BAR_SUBCHAN_INDEX_SEND, BAR_SUBCHAN_INDEX_RECV},
    {BAR_SUBCHAN_INDEX_SEND, BAR_SUBCHAN_INDEX_RECV, BAR_SUBCHAN_INDEX_RECV}
};

uint8_t chan_id_tbl[BAR_MSG_SRC_NUM][BAR_MSG_DST_NUM] =
{
    {BAR_INDEX_TO_RISC, BAR_INDEX_MPF_TO_PFVF, BAR_INDEX_MPF_TO_MPF},
    {BAR_INDEX_TO_RISC, BAR_INDEX_PF_TO_VF,    BAR_INDEX_PFVF_TO_MPF},
    {BAR_INDEX_TO_RISC, BAR_INDEX_PF_TO_VF,    BAR_INDEX_PFVF_TO_MPF}
};

void *internal_addr;

bool is_mpf_scaned = FALSE;

static struct msgid_ring g_msgid_ring;

/* 消息处理函数表*/
zxdh_bar_chan_msg_recv_callback msg_recv_func_tbl[MSG_MODULE_NUM];

void bar_chan_check_chan_stats(int ret, uint64_t addr)
{
    struct bar_msg_header *hdr = (struct bar_msg_header*)addr;

    if (ret == 0)
    {
        return;
    }
    /* check bar msg_header*/
    BAR_LOG_ERR("bar msg err, ret: %d, valid: %u, msg_id: %u, event_id: %u, "
                 "ack: %u, src_pcieid: 0x%x, dst_pcieid: 0x%x, chan_addr: 0x%llx.\n",
                 ret, hdr->valid, hdr->msg_id, hdr->event_id, hdr->ack, hdr->src_pcieid, hdr->dst_pcieid, addr);
}

uint16_t bar_msg_src_parse(struct zxdh_pci_bar_msg *in)
{
    if (in == NULL)
    {
        return BAR_MSG_ERR_NULL;
    }

    if (in->src == MSG_CHAN_END_MPF)
    {
        if (!is_mpf_scaned)
        {
            return  BAR_MSG_ERR_MPF_NOT_SCANED;
        }
        in->virt_addr = (uint64_t)internal_addr + BAR_MSG_OFFSET;
        in->src_pcieid = PF0_PCIEID;
    }
    return BAR_MSG_OK;
}

void bar_chan_sync_fill_header(uint32_t msg_id, struct zxdh_pci_bar_msg *in, struct bar_msg_header *msg_header)
{
    memset(msg_header, 0, sizeof(*msg_header));
    msg_header->sync        = BAR_CHAN_MSG_SYNC;
    msg_header->event_id   = in->event_id;
    msg_header->len         = in->payload_len;
    msg_header->msg_id      = msg_id;
    msg_header->dst_pcieid  = in->dst_pcieid;
    msg_header->src_pcieid  = in->src_pcieid;
}

int bar_chan_msgid_allocate(uint16_t *msgid)
{
    int ret = BAR_MSG_OK;
    uint16_t msg_id = 0;
    struct msgid_reps_info *msgid_reps_info = NULL;
    uint16_t count = 0;

    spin_lock(&g_msgid_ring.lock);
    msg_id = g_msgid_ring.msg_id;
    do
    {
        count++;
        ++msg_id;
        msg_id %= MAX_MSG_BUFF_NUM;
        msgid_reps_info = &g_msgid_ring.reps_info_tbl[msg_id];

    }while(msgid_reps_info->flag != REPS_INFO_FLAG_USABLE && (count < MAX_MSG_BUFF_NUM));

    if (count >= MAX_MSG_BUFF_NUM)
    {
        ret = -1;
        goto out;
    }

    msgid_reps_info->flag = REPS_INFO_FLAG_USED;
    g_msgid_ring.msg_id = msg_id;
    *msgid = msg_id;

out:
    spin_unlock(&g_msgid_ring.lock);
    return ret;
}

uint16_t bar_chan_save_recv_info(struct zxdh_msg_recviver_mem *result, uint16_t *msg_id)
{
    int ret = 0;
    struct msgid_reps_info *reps_info = NULL;

    ret = bar_chan_msgid_allocate(msg_id);
    if (ret == -1)
    {
        return BAR_MSG_ERR_MSGID;
    }
    reps_info = &g_msgid_ring.reps_info_tbl[*msg_id];
    reps_info->reps_buffer = result->recv_buffer;
    reps_info->buffer_len = result->buffer_len;

    return BAR_MSG_OK;
}

void bar_chan_msgid_free(uint16_t msg_id)
{
    struct msgid_reps_info *msgid_reps_info = NULL;
    if (msg_id >= MAX_MSG_BUFF_NUM)
    {
        return;
    }
    msgid_reps_info = &g_msgid_ring.reps_info_tbl[msg_id];
    spin_lock(&g_msgid_ring.lock);
    msgid_reps_info->flag = REPS_INFO_FLAG_USABLE;
    spin_unlock(&g_msgid_ring.lock);
    return;
}

uint8_t bar_msg_row_index_trans(uint8_t src)
{
    uint8_t src_index = 0;

    switch (src)
    {
        case MSG_CHAN_END_MPF:
        {
            src_index = BAR_MSG_SRC_MPF;
            break;
        }
        case MSG_CHAN_END_PF:
        {
            src_index = BAR_MSG_SRC_PF;
            break;
        }
        case MSG_CHAN_END_VF:
        {
            src_index = BAR_MSG_SRC_VF;
            break;
        }
        default:
        {
            src_index = BAR_MSG_SRC_ERR;
            break;
        }
    }
    return src_index;
}

uint8_t bar_msg_col_index_trans(uint8_t dst)
{
    uint8_t dst_index = 0;

    switch (dst)
    {
        case MSG_CHAN_END_MPF:
        {
            dst_index = BAR_MSG_DST_MPF;
            break;
        }
        case MSG_CHAN_END_PF:
        {
            dst_index = BAR_MSG_DST_PFVF;
            break;
        }
        case MSG_CHAN_END_VF:
        {
            dst_index = BAR_MSG_DST_PFVF;
            break;
        }
        case MSG_CHAN_END_RISC:
        {
            dst_index = BAR_MSG_DST_RISC;
            break;
        }
        default:
        {
            dst_index = BAR_MSG_SRC_ERR;
            break;
        }
    }
    return dst_index;
}

int bar_chan_send_para_check(struct zxdh_pci_bar_msg *in, struct zxdh_msg_recviver_mem *result)
{
    uint8_t src_index = 0;
    uint8_t dst_index = 0;

    if (in == NULL || result == NULL)
    {
        BAR_LOG_ERR("send para ERR: null para.\n");
        return  BAR_MSG_ERR_NULL_PARA;
    }

    src_index =  bar_msg_row_index_trans((uint8_t)in->src);
    dst_index =  bar_msg_col_index_trans((uint8_t)in->dst);
    if (src_index == BAR_MSG_SRC_ERR || dst_index == BAR_MSG_DST_ERR)
    {
        BAR_LOG_ERR("send para ERR: chan doesn't exist.\n");
        return BAR_MSG_ERR_TYPE;
    }
    if (in->event_id > MSG_MODULE_NUM)
    {
        BAR_LOG_ERR("send para ERR: invalid event_id: %d.\n", in->event_id);
        return BAR_MSG_ERR_MODULE;
    }
    if (in->payload_addr == NULL)
    {
        BAR_LOG_ERR("send para ERR: null message.\n");
        return BAR_MSG_ERR_BODY_NULL;
    }
    if (in->payload_len > BAR_MSG_PAYLOAD_MAX_LEN)
    {
        BAR_LOG_ERR("send para ERR: len %x is too long.\n", in->payload_len);
        return BAR_MSG_ERR_LEN;
    }
    if (in->virt_addr == 0 || result->recv_buffer == NULL)
    {
        BAR_LOG_ERR("send para ERR: virt_addr or recv_buffer is NULL.\n");
        return BAR_MSG_ERR_VIRTADDR_NULL;
    }
    if (result->buffer_len < REPS_HEADER_PAYLOAD_OFFSET)
    {
        BAR_LOG_ERR("recv buffer's len: %d is short than mininal 4 bytes\n", result->buffer_len);
    }
    return BAR_MSG_OK;
}

/* 根据用户提供的src和dst和当前的场景来推算2K的偏移*/
void bar_chan_subchan_addr_get(struct zxdh_pci_bar_msg *in, uint64_t *subchan_addr)
{
    uint8_t src_index, dst_index;
    uint16_t chan_id, subchan_id;

    src_index =  bar_msg_row_index_trans((uint8_t)in->src);
    dst_index =  bar_msg_col_index_trans((uint8_t)in->dst);

    if (src_index == BAR_MSG_SRC_ERR || dst_index == BAR_MSG_DST_ERR)
    {
        return;
    }

    chan_id = chan_id_tbl[src_index][dst_index];
    subchan_id = subchan_id_tbl[src_index][dst_index];
    *subchan_addr = in->virt_addr + (2 * chan_id + subchan_id) * BAR_MSG_ADDR_CHAN_INTERVAL;
    return;
}

int bar_chan_reg_write(uint64_t subchan_addr, uint32_t offset, uint32_t data)
{
    uint32_t algin_offset = (offset & BAR_ALIGN_WORD_MASK);

    if (algin_offset >= BAR_MSG_ADDR_CHAN_INTERVAL)
    {
        return -EADDRNOTAVAIL;
    }

    writel(data, (volatile void*)(subchan_addr + algin_offset));
    return 0;
}

int bar_chan_reg_read(uint64_t subchan_addr, uint32_t offset, uint32_t *pdata)
{
    uint32_t algin_offset = (offset & BAR_ALIGN_WORD_MASK);

    if (algin_offset >= BAR_MSG_ADDR_CHAN_INTERVAL)
    {
        return -EADDRNOTAVAIL;
    }

    *pdata = readl((const volatile void *)(subchan_addr + algin_offset));
    return 0;
}

uint16_t bar_chan_msg_header_set(uint64_t subchan_addr, struct bar_msg_header *msg_header)
{
    uint32_t *data = (uint32_t*)msg_header;
    uint16_t idx = 0;

    for (idx = 0; idx < (BAR_MSG_PLAYLOAD_OFFSET >> 2); idx++)
    {
        bar_chan_reg_write(subchan_addr, idx * 4, *(data + idx));
    }

    return BAR_MSG_OK;
}

uint16_t bar_chan_msg_header_get(uint64_t subchan_addr, struct bar_msg_header *msg_header)
{
    uint32_t *data = (uint32_t*)msg_header;
    uint16_t idx = 0;

    for (idx = 0; idx < (BAR_MSG_PLAYLOAD_OFFSET >> 2); idx++)
    {
        bar_chan_reg_read(subchan_addr, idx * 4, data + idx);
    }

    return BAR_MSG_OK;
}

uint16_t bar_chan_msg_payload_set(uint64_t subchan_addr, uint8_t *msg, uint16_t len)
{
    uint32_t *data = (uint32_t*)msg;
    uint32_t count = (len / sizeof(uint32_t));
    uint32_t remain = (len % sizeof(uint32_t));
    uint32_t ix = 0, remain_data = 0;

    for (ix = 0; ix < count; ix++)
    {
        bar_chan_reg_write(subchan_addr, 4 * ix + BAR_MSG_PLAYLOAD_OFFSET, *(data + ix));
    }
    for (ix = 0; ix < remain; ix++)
    {
        remain_data  |= *((uint8_t *)(msg + (len - remain + ix))) << (8 * ix);
    }
    bar_chan_reg_write(subchan_addr, 4 * count + BAR_MSG_PLAYLOAD_OFFSET, remain_data);

    return BAR_MSG_OK;
}

uint16_t bar_chan_msg_payload_get(uint64_t subchan_addr, uint8_t *msg, uint16_t len)
{
    uint32_t *data = (uint32_t*)msg;
    uint32_t count = (len / sizeof(uint32_t));
    uint32_t remain = (len % sizeof(uint32_t));
    uint32_t ix = 0, remain_data = 0;

    for (ix = 0; ix < count; ix++)
    {
        bar_chan_reg_read(subchan_addr, 4 * ix + BAR_MSG_PLAYLOAD_OFFSET, (data + ix));
    }
    bar_chan_reg_read(subchan_addr, 4 * count + BAR_MSG_PLAYLOAD_OFFSET, &remain_data);
    for (ix = 0; ix < remain; ix++)
    {
        *((uint8_t *)(msg + (len - remain + ix))) = remain_data >> (8 * ix);
    }
    return BAR_MSG_OK;
}

uint16_t  bar_chan_msg_valid_set(uint64_t subchan_addr, uint8_t valid_label)
{
    uint32_t data = 0;

    bar_chan_reg_read(subchan_addr, BAR_MSG_VALID_OFFSET, &data);
    data &= (~BAR_MSG_VALID_MASK);
    data |= (uint32_t)valid_label;
    bar_chan_reg_write(subchan_addr, BAR_MSG_VALID_OFFSET, data);

    return BAR_MSG_OK;
}

uint16_t bar_msg_valid_stat_get(uint64_t subchan_addr)
{
    uint32_t data = 0;

    bar_chan_reg_read(subchan_addr, BAR_MSG_VALID_OFFSET, &data);
    if (BAR_MSG_CHAN_USABLE == (data & BAR_MSG_VALID_MASK))
    {
        return BAR_MSG_CHAN_USABLE;
    }

    return BAR_MSG_CHAN_USED;
}

uint16_t  bar_chan_msg_poltag_set(uint64_t subchan_addr, uint8_t label)
{
    uint32_t data = 0;

    bar_chan_reg_read(subchan_addr, BAR_MSG_VALID_OFFSET, &data);
    data &= (~(uint32_t)BAR_MSG_POL_MASK);
    data |= ((uint32_t)label << BAR_MSG_POL_OFFSET);
    bar_chan_reg_write(subchan_addr, BAR_MSG_VALID_OFFSET, data);

    return BAR_MSG_OK;
}

static uint8_t payload_temp_buf[BAR_MSG_ADDR_CHAN_INTERVAL] = {0};
uint16_t bar_chan_msg_send(uint64_t subchan_addr, void *payload_addr, uint16_t payload_len, struct bar_msg_header *msg_header)
{
    uint8_t *msg = (uint8_t*)(payload_addr);
    struct bar_msg_header hdr_read = {0};
    uint16_t valid = 0;

    bar_chan_msg_header_set(subchan_addr, msg_header);
    bar_chan_msg_header_get(subchan_addr, &hdr_read);

    bar_chan_msg_payload_set(subchan_addr, msg, payload_len);
    bar_chan_msg_payload_get(subchan_addr, payload_temp_buf, payload_len);

    bar_chan_msg_valid_set(subchan_addr, BAR_MSG_CHAN_USED);
    valid = bar_msg_valid_stat_get(subchan_addr);

    return BAR_MSG_OK;
}

int bar_chan_recv_func_check(uint16_t check)
{
    if (CHECK_STATE_OK == check)
    {
        return BAR_MSG_OK;
    }
    else
    {
        BAR_LOG_ERR("recv func check failed, check field: 0x%x", check);
        return BAR_MSG_ERR_USR_RET_ERR;
    }
}

int bar_chan_sync_msg_reps_get(uint64_t subchan_addr, uint64_t recv_buffer, uint16_t buffer_len, uint16_t send_msg_id)
{
    int ret = BAR_MSG_OK;
    uint16_t recv_msg_id = 0;
    uint16_t recv_len = 0;
    uint8_t *recv_msg =  (uint8_t*)recv_buffer;
    struct bar_msg_header msg_header;
    struct msgid_reps_info *reps_info = NULL;

    /*从消息头中取出消息回复的长度，取出msg_id，如果msg_id对应的usable的话，该条同步回复作废*/
    memset(&msg_header, 0, sizeof(msg_header));
    bar_chan_msg_header_get(subchan_addr, &msg_header);
    recv_len = msg_header.len;
    recv_msg_id = msg_header.msg_id;

    if (recv_msg_id != send_msg_id)
    {
        BAR_LOG_ERR("send msg id: %d, but get reply msg id: %d.\n", send_msg_id, recv_msg_id);
        ret = BAR_MSG_ERR_REPLY;
        goto out;
    }

    reps_info = &g_msgid_ring.reps_info_tbl[recv_msg_id];
    if (reps_info->flag != REPS_INFO_FLAG_USED)
    {
        BAR_LOG_ERR("msg_id: %d is release", recv_msg_id);
        ret = BAR_MSG_ERR_REPLY;
        goto out;
    }

    if (recv_len > buffer_len - REPS_HEADER_PAYLOAD_OFFSET)
    {
        BAR_LOG_ERR("reps_buf_len is %d, but reps_msg_len is %d", buffer_len, recv_len + 4);
        ret = BAR_MSG_ERR_REPSBUFF_LEN;
        goto out;
    }

    /* 从reps_buff + 4的位置拷贝进回复数据*/
    bar_chan_msg_payload_get(subchan_addr, recv_msg + REPS_HEADER_PAYLOAD_OFFSET, recv_len);

    ret = bar_chan_recv_func_check(msg_header.check);
    if (ret != BAR_MSG_OK)
    {
        goto out;
    }

    /* 拷贝数据长度*/
    *(uint16_t*)(recv_msg + REPS_HEADER_LEN_OFFSET) = recv_len;
    /* reps头valid置位*/
    *recv_msg = REPS_HEADER_REPLYED;

out:
    return ret;
}

static void bar_chan_reset_flag_normal(uint64_t subchan_addr, uint8_t dst)
{
    if (dst != MSG_CHAN_END_RISC)
    {
        bar_chan_msg_valid_set(subchan_addr, BAR_MSG_CHAN_USABLE);
    }
    return;
}

#define VALID_FLAG_DETECT_SPAN_MS    (10)
#define VALID_FLAG_DETECT_TIMEOUT_MS (6000)
#define US_NUMS_PER_MS               (1000)

static int bar_chan_sync_wait(uint64_t subchan_addr, uint8_t dst, uint32_t *wait_reps_retry_times)
{
    uint8_t valid = 0;
    int retry_cnt = 0;
    uint32_t timeout_th_res_ms = 0;
    int max_retries = VALID_FLAG_DETECT_TIMEOUT_MS / VALID_FLAG_DETECT_SPAN_MS / 2;

    if (dst != MSG_CHAN_END_RISC)
    {
        *wait_reps_retry_times = BAR_MSG_TIMEOUT_TH;
        return 0;
    }

    for (retry_cnt = 0; retry_cnt < max_retries; retry_cnt++) {
        valid = bar_msg_valid_stat_get(subchan_addr);
        if (valid == BAR_MSG_CHAN_USABLE) {
            timeout_th_res_ms = VALID_FLAG_DETECT_TIMEOUT_MS - retry_cnt * VALID_FLAG_DETECT_SPAN_MS;
            *wait_reps_retry_times = timeout_th_res_ms * US_NUMS_PER_MS / BAR_MSG_POLLING_SPAN_US;
            return 0;
        }
        msleep(VALID_FLAG_DETECT_SPAN_MS);
    }
    return -1;
}

/**
 * zxdh_bar_chan_sync_msg_send - 通过PCIE BAR空间发送同步消息
 * @in: 消息发送信息
 * @result: 消息结果反馈
 * @return: 0 成功，其他失败
 * 注意：bar通道使能消息也是调用这个接口，不能全部拦截
 */
int zxdh_bar_chan_sync_msg_send(struct zxdh_pci_bar_msg *in, struct zxdh_msg_recviver_mem *result)
{
    int ret = 0;
    uint16_t valid = 0;
    uint16_t time_out_cnt = 0;
    uint32_t wait_reps_retry_times = 0;
    uint16_t msg_id = 0;
    uint64_t subchan_addr = 0;
    struct bar_msg_header msg_header = {0};

    ret =  bar_msg_src_parse(in);
    if (ret != BAR_MSG_OK)
    {
        return ret;
    }

    ret = bar_chan_send_para_check(in, result);
    if (ret != BAR_MSG_OK)
    {
        BAR_LOG_ERR("para check failed, %d.", ret);
        return ret;
    }

    /* 申请msg_id,并将缓存信息存放到表中*/
    ret = bar_chan_save_recv_info(result, &msg_id);
    if (ret != BAR_MSG_OK)
    {
       BAR_LOG_ERR("msg_id allocated failed.");
       return ret;
    }
    /* 计算2K通道的地址*/
    bar_chan_subchan_addr_get(in, &subchan_addr);
    if (*(uint32_t*)subchan_addr == 0xffffffff)
    {
       BAR_LOG_ERR("pcie bar abnormal.\n");
       ret = BAR_MSG_ERR_BAR_ABNORMAL;
       bar_chan_msgid_free(msg_id);
       return ret;
    }
    /* 填充消息头*/
    bar_chan_sync_fill_header(msg_id, in, &msg_header);
    /* 给通道上锁，根据src和dst判断是分配硬件锁还是软件锁*/
    ret = bar_chan_lock(in->src, in->dst, in->src_pcieid, in->virt_addr);
    if (ret != BAR_MSG_OK)
    {
        bar_chan_msgid_free(msg_id);
        return ret;
    }

    ret = bar_chan_sync_wait(subchan_addr, in->dst, &wait_reps_retry_times);
    if (ret != 0)
    {
        BAR_LOG_ERR("chan valid flag is used while send msg-%u.\n", msg_id);
        goto free_chan;
    }
    BAR_LOG_DEBUG("pcie_id-0x%x  src-%u, dst-%u get     lock.\n", in->src_pcieid, in->src, in->dst);

    /* 消息头、消息体发送到bar空间， valid置位*/
    bar_chan_msg_send(subchan_addr, in->payload_addr, in->payload_len, &msg_header);
    /* 轮询等待消息回复*/
    do
    {
        usleep_range(BAR_MSG_POLLING_SPAN_US, BAR_MSG_POLLING_SPAN_US + 10);
        valid = bar_msg_valid_stat_get(subchan_addr);
        time_out_cnt++;
    }while((time_out_cnt < wait_reps_retry_times) && (BAR_MSG_CHAN_USED == valid));

    /* 如果超时恢复标志位*/
    if ((wait_reps_retry_times == time_out_cnt) && (BAR_MSG_CHAN_USABLE != valid))
    {
        bar_chan_reset_flag_normal(subchan_addr, in->dst);
        bar_chan_msg_poltag_set(subchan_addr, 0);
        BAR_LOG_ERR("BAR MSG ERR: msg_id: %d time out.\n", msg_header.msg_id);
        ret =  BAR_MSG_ERR_TIME_OUT;
    }
    else
    {
        /* 从消息头中取出回复消息的长度len, 从payload中取出消息内容，放到本地缓存reps_buff*/
        ret = bar_chan_sync_msg_reps_get(subchan_addr, (uint64_t)result->recv_buffer, result->buffer_len, msg_id);
    }
free_chan:
    bar_chan_msgid_free(msg_id);
    /*通道解锁*/
    bar_chan_check_chan_stats(ret, subchan_addr);
    BAR_LOG_DEBUG("pcie_id-0x%x  src-%u, dst-%u release lock.\n", in->src_pcieid, in->src, in->dst);
    bar_chan_unlock((uint8_t)in->src, (uint8_t)in->dst, in->src_pcieid, in->virt_addr);
    return ret;
}