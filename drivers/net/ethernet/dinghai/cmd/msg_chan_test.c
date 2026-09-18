#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/if_bridge.h>
#include <linux/ktime.h>
#include <linux/dinghai/driver.h>
#include <linux/dinghai/dh_cmd.h>
#include "cmd/msg_chan_priv.h"

#ifdef BAR_MSG_TEST

/*计算方法： risc时间戳 - host时间戳*/
#define   HOST_RISC_DIFF        (307762)

uint64_t print_time(char *str)
{
    ktime_t kt = ktime_get();
    s64 us_since_boot = ktime_to_us(kt);
    LOG_INFO("%s timestamp: %llu us\n", str, us_since_boot);
    return us_since_boot;
}
struct msg_time_statis_reps
{
    uint16_t sum_check;

    uint64_t risc_recv_msg_t;
    uint64_t risc_push_msg_t;
    uint64_t risc_pop_msg_t;
    uint64_t risc_msg_proc_t;
    uint64_t risc_notice_peer_t;
} __attribute__((packed));

struct msg_time_host_risc
{
    uint64_t host_send_msg_t;
    uint64_t host_recv_msg_t;
    struct msg_time_statis_reps risc_time;
}__attribute__((packed));

struct msg_time_host_risc global_time_stat = {0};

void print_risc_time_stamp(struct msg_time_host_risc *stat)
{
#if 0
    LOF_INFO("host_send_msg_t:    %llu us.\n", stat->host_send_msg_t);
    LOF_INFO("risc_recv_msg_t:    %llu us.\n", stat->risc_time.risc_recv_msg_t);
    LOF_INFO("risc_push_msg_t:    %llu us.\n", stat->risc_time.risc_push_msg_t);
    LOF_INFO("risc_pop_msg_t:     %llu us.\n", stat->risc_time.risc_pop_msg_t);
    LOF_INFO("risc_msg_proc_t:    %llu us.\n", stat->risc_time.risc_msg_proc_t);
    LOF_INFO("risc_notice_peer_t: %llu us.\n", stat->risc_time.risc_notice_peer_t);
    LOF_INFO("host_recv_msg_t:    %llu us.\n", stat->host_recv_msg_t);
#endif
    LOG_INFO("risc recv->msg push:         %llu us.\n", stat->risc_time.risc_push_msg_t - stat->risc_time.risc_recv_msg_t);
    LOG_INFO("risc push->risc pop:         %llu us.\n", stat->risc_time.risc_pop_msg_t - stat->risc_time.risc_push_msg_t);
    LOG_INFO("risc pop->before proc   :    %llu us.\n", stat->risc_time.risc_msg_proc_t- stat->risc_time.risc_pop_msg_t);
    LOG_INFO("after proc->risc set valid:  %llu us.\n", stat->risc_time.risc_notice_peer_t - stat->risc_time.risc_msg_proc_t);
}

uint16_t sum_func(void *data, uint16_t len)
{
    uint64_t result = 0;
    int idx = 0;
    uint16_t ret = 0;

    if (data == NULL)
    {
        return 0;
    }

    for (idx = 0; idx < len; idx++)
    {
        result += *((uint8_t *)data + idx);
    }

    ret = (uint16_t)result;
    return ret;
}

uint16_t test_sync_send(void)
{
    struct zxdh_pci_bar_msg in = {0};
    struct zxdh_msg_recviver_mem result = {0};
    uint16_t payload_len = 0;
    uint64_t bar_base_addr = 0;
    void *payload_addr = NULL;
    uint8_t recv_buffer[200] = {0};
    uint16_t reps = 0;
    uint16_t ret = 0;

    payload_len = 100;
    payload_addr = kmalloc(payload_len, GFP_KERNEL);
    if (!payload_addr)
    {
        LOG_ERR("malloca failed");
        return 0xaa;
    }
    get_random_bytes(payload_addr, payload_len);
    LOG_INFO("sync send msg len: %x", payload_len);

    in.src_pcieid = 0x900;
    in.virt_addr = 0;
    in.payload_addr = payload_addr;
    in.payload_len = payload_len;
    in.src = MSG_CHAN_END_MPF;
    in.dst = MSG_CHAN_END_RISC;
    in.event_id = 50;

    /* 构造result接收参数*/
    result.recv_buffer = recv_buffer;
    result.buffer_len = sizeof(recv_buffer);

    /* 调用发送接口*/
    LOG_INFO("start to sync send test.");
    global_time_stat.host_send_msg_t = print_time("before send.") + HOST_RISC_DIFF;
    ret = zxdh_bar_chan_sync_msg_send(&in, &result);
    global_time_stat.host_recv_msg_t = print_time("after send.") + HOST_RISC_DIFF;

    if (ret != BAR_MSG_OK)
    {
        LOG_ERR("sync send failed");
        ret = 0xAA;
        goto out;
    }

    struct msg_time_statis_reps *reps_ptr = (struct msg_time_statis_reps*)((uint8_t *)result.recv_buffer + 4);
    if (reps_ptr->sum_check == sum_func(payload_addr, payload_len))
    {
        memcpy(&global_time_stat.risc_time, (void*)reps_ptr, sizeof(struct msg_time_statis_reps));
        print_risc_time_stamp(&global_time_stat);
        ret = 0;
        LOG_ERR("reps validate success: %d", reps);
        goto out;
    }
    else
    {
        LOG_ERR("reps valide failed: %d", reps);
        ret = 0xAA;
    }

out:
    if (!payload_addr)
    {
        kfree(payload_addr);
        payload_addr = NULL;
    }
    return ret;
}
#endif
