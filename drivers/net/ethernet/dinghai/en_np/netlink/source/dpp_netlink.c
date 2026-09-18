#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/socket.h>
#include <linux/net.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/skbuff.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <net/sock.h>
#include <asm/types.h>
#include "dpp_netlink.h"

#define DPP_NETLINK_PROTOCOL    ((ZXIC_UINT32)(29))
#define DPP_NETLINK_GROUP_ID    ((ZXIC_UINT32)(1))
#define DPP_NETLINK_MAX_PROC    ((ZXIC_UINT32)(2048))

typedef DPP_STATUS (*DPP_NETLINK_PROC_PTR)(ZXIC_VOID *msg_body, ZXIC_UINT32 msg_len, ZXIC_VOID **resp, ZXIC_UINT32 *reps_len);

static struct sock *dpp_netlink_sk;
static DPP_NETLINK_PROC_PTR dpp_netlink_proc_ptr[DPP_NETLINK_MAX_PROC] = {0};

static DPP_STATUS dpp_netlink_send_ack_msg(ZXIC_VOID *data, ZXIC_UINT32 len)
{
    struct sk_buff *skb  = NULL;
    struct nlmsghdr *nlh = NULL;
    ZXIC_SINT32 rtn = DPP_OK;

    if (data == NULL)
    {
        ZXIC_COMM_TRACE_NOTICE("data invalid.\n");
        return DPP_ERR;
    }

    skb = alloc_skb(NLMSG_SPACE(len), GFP_KERNEL);
    if (skb == NULL)
    {
        ZXIC_COMM_TRACE_NOTICE("alloc_skb failed.\n");
        return DPP_ERR;
    }

    nlh = nlmsg_put(skb, 0, 0, 0, len, 0);
    if (nlh == NULL)
    {
        ZXIC_COMM_TRACE_NOTICE("nlmsg_put failed.\n");
        return DPP_ERR;
    }

    nlh->nlmsg_flags = NLM_F_ACK;
    memcpy(NLMSG_DATA(nlh), data, len);

    rtn = nlmsg_multicast(dpp_netlink_sk, skb, 0, DPP_NETLINK_GROUP_ID, 0);
    if (rtn < 0)
    {
        ZXIC_COMM_TRACE_NOTICE("nlmsg_multicast failed, rtn %d.\n", rtn);
        return DPP_ERR;
    }
    return DPP_OK;
}

static DPP_STATUS dpp_netlink_dispach_msg(struct nlmsghdr *nlh)
{
    ZXIC_UINT8 *data     = NULL;
    ZXIC_UINT32 id       = 0;
    ZXIC_UINT32 len      = 0;
    ZXIC_UINT8 *req      = NULL;
    ZXIC_VOID *resp      = NULL;
    ZXIC_UINT32 resp_len = 0;
    ZXIC_UINT32 rtn      = DPP_OK;

    DPP_NETLINK_PROC_PTR ptr = NULL;

    if (nlh == NULL)
    {
        ZXIC_COMM_TRACE_NOTICE("nlh invalid.\n");
        return DPP_ERR;
    }

    id  = *(ZXIC_UINT32 *)NLMSG_DATA(nlh);
    len = nlh->nlmsg_len - NLMSG_HDRLEN;
    req = (ZXIC_UINT8 *)NLMSG_DATA(nlh);
    if (id > (DPP_NETLINK_MAX_PROC - 1))
    {
        ZXIC_COMM_TRACE_NOTICE("id %u invalid.\n", id);
        return DPP_ERR;
    }

    ptr = dpp_netlink_proc_ptr[id];
    if (ptr == NULL)
    {
        ZXIC_COMM_TRACE_NOTICE("ptr invalid.\n");
        return DPP_ERR;
    }

    rtn = ptr(req, len, &resp, &resp_len);
    if (rtn != DPP_OK)
    {
        ZXIC_COMM_FREE(resp);
        ZXIC_COMM_TRACE_NOTICE("proc id %u failed.\n", id);
        return rtn;
    }

    data = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(NLMSG_ALIGN(resp_len) + sizeof(ZXIC_UINT32));
    if (data == NULL)
    {
        ZXIC_COMM_FREE(resp);
        ZXIC_COMM_TRACE_NOTICE("ZXIC_COMM_MALLOC failed.\n");
        return DPP_ERR;
    }
    memcpy(data, &rtn, sizeof(ZXIC_UINT32));

    if (resp != NULL)
    {
        memcpy(data + sizeof(ZXIC_UINT32), resp, resp_len);
    }

    dpp_netlink_send_ack_msg(data, NLMSG_ALIGN(resp_len) + sizeof(ZXIC_UINT32));

    ZXIC_COMM_FREE(data);
    ZXIC_COMM_FREE(resp);

    return DPP_OK;
}

static ZXIC_VOID dpp_netlink_recv_msg(struct sk_buff *__skb)
{
    struct sk_buff *skb;
    struct nlmsghdr *nlh;
    ZXIC_UINT32 rtn = DPP_ERR;

    skb = skb_get(__skb);
    if (skb == NULL)
    {
        ZXIC_COMM_TRACE_NOTICE("get skb failed.\n");
        return;
    }

    nlh = nlmsg_hdr(skb);
    if ((nlh == NULL) || !NLMSG_OK(nlh, skb->len))
    {
        kfree_skb(skb);
        ZXIC_COMM_TRACE_NOTICE("skb format invalid.\n");
        return;
    }

    if ((nlh->nlmsg_flags & NLM_F_REQUEST) != 0)
    {
        if (dpp_netlink_dispach_msg(nlh) != DPP_OK)
        {
            dpp_netlink_send_ack_msg((ZXIC_UINT8 *)&rtn, sizeof(ZXIC_UINT32));
            ZXIC_COMM_TRACE_NOTICE("dpp_netlink_dispach_msg failed.\n");
        }
        kfree_skb(skb);
        return;
    }

    kfree_skb(skb);
    ZXIC_COMM_TRACE_NOTICE("nlmsg_flags 0x%04x invalid.\n", nlh->nlmsg_flags);
    return;
}

DPP_STATUS dpp_netlink_regist_msg_proc_fun(ZXIC_UINT32 id, ZXIC_VOID *ptr)
{
    if (ptr == NULL)
    {
        ZXIC_COMM_TRACE_NOTICE("ptr invalid.\n");
        return DPP_ERR;
    }
    if (id > (DPP_NETLINK_MAX_PROC - 1))
    {
        ZXIC_COMM_TRACE_NOTICE("id %u invalid.\n", id);
        return DPP_ERR;
    }
    dpp_netlink_proc_ptr[id] = ptr;
    return DPP_OK;
}

ZXIC_SINT32 dpp_netlink_init(ZXIC_VOID)
{
    struct netlink_kernel_cfg cfg = {
        .input = dpp_netlink_recv_msg,
    };

    ZXIC_COMM_TRACE_NOTICE("start.\n");

    // dpp_netlink_sk = netlink_kernel_create(get_net_ns_by_pid(1), DPP_NETLINK_PROTOCOL, &cfg);
    dpp_netlink_sk = netlink_kernel_create(&init_net, DPP_NETLINK_PROTOCOL, &cfg);
    if (!dpp_netlink_sk)
    {
        ZXIC_COMM_TRACE_NOTICE("create socket failed.\n");
        return DPP_ERR;
    }

    ZXIC_COMM_TRACE_NOTICE("success.\n");

    return DPP_OK;
}

ZXIC_VOID dpp_netlink_exit(ZXIC_VOID)
{
    ZXIC_COMM_TRACE_NOTICE("start.\n");
    netlink_kernel_release(dpp_netlink_sk);
    ZXIC_COMM_TRACE_NOTICE("success.\n");
}
