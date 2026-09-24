#include <net/sock.h>
#include <net/genetlink.h>
#include <linux/dinghai/kcompat.h>
#include "../en_aux.h"
#include "zxdh_tools_ioctl.h"
#include "zxdh_tools_netlink.h"


int32_t zxdh_tools_genl_recv_doit(struct sk_buff *skb, struct genl_info *info);
  /* operation definition */
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
static struct nla_policy zxdh_tools_genl_policy[ZXDH_TOOLS_A_MAX + 1] = {
        [ZXDH_TOOLS_A_MSG] = { .type = NLA_NUL_STRING },
};
#endif

struct genl_ops zxdh_tools_gnl_ops[] = {
    {
        .cmd = ZXDH_TOOLS_C_ECHO,
        .flags = 0,
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
        .policy = zxdh_tools_genl_policy,
#endif
        .doit = zxdh_tools_genl_recv_doit,
        .dumpit = NULL,
    }
};

static struct genl_family zxdh_tools_msg_family = {
       .hdrsize = 0,
       .name = ZXDH_TOOLS_NETLINK_NAME,
       .version = 1,
       .maxattr = ZXDH_TOOLS_A_MAX,
       .ops = zxdh_tools_gnl_ops,
       .n_ops = 1,
};

/*
* genl_msg_prepare_usr_msg : 构建netlink及gennetlink首部
* @cmd : genl_ops的cmd
* @size : gen_netlink用户数据的长度（包括用户定义的首部）
*/
int32_t zxdh_tools_genl_msg_prepare_usr_msg(u8 cmd, size_t size, uint32_t pid, struct sk_buff **skbp)
{
    void *ptr = NULL;
    struct sk_buff *skb;
    //DHTOOLS_LOG_INFO("is called!\n");
    /* create a new netlink msg */
    skb = genlmsg_new(size, GFP_KERNEL);
    if (skb == NULL) {
        DHTOOLS_LOG_ERR("genlmsg_new failed!!!\n");
        return -1;
    }
    /* Add a new netlink message to an skb */
    ptr = genlmsg_put(skb, pid, 0, &zxdh_tools_msg_family, 0, cmd);
    if(ptr == NULL) {
        DHTOOLS_LOG_ERR("genlmsg_put failed!!!\n");
        return -1;
    }
    *skbp = skb;
    return 0;
}

/*
* 添加用户数据，及添加一个netlink addribute
*@type : nlattr的type
*@len : nlattr中的len
*@data : 用户数据
*/
int32_t zxdh_tools_genl_msg_mk_usr_msg(struct sk_buff *skb, int type, void *data, int len)
{
    int ret = 0;
    //DHTOOLS_LOG_INFO("is called!\n");
    /* add a netlink attribute to a socket buffer */
    ret = nla_put(skb, type, len, data);
    if(ret != 0) {
        DHTOOLS_LOG_ERR("nla_put failed, ret=%d!!!\n", ret);
        return -1;
    }
    return 0;
}

/**
* genl_msg_send_to_user - 通过generic netlink发送数据到netlink
*
* @data: 发送数据缓存
* @len: 数据长度 单位：byte
* @pid: 发送到的客户端pid
*/
int32_t zxdh_tools_genl_msg_send_to_user(void *data, uint16_t len, uint32_t pid)
{
    struct sk_buff *skb;
    uint16_t size;
    int ret = 0;

    //DHTOOLS_LOG_INFO("is called!\n");
    ret = nla_total_size(len); /* total length of attribute including padding */
    if(ret <= 0) {
        DHTOOLS_LOG_ERR("nla_total_size failed, ret=%d!\n", ret);
        return -1;
    }
    size = ret;

    ret = zxdh_tools_genl_msg_prepare_usr_msg(ZXDH_TOOLS_C_ECHO, size, pid, &skb);
    if (ret) {
        DHTOOLS_LOG_ERR("zxdh_tools_genl_msg_prepare_usr_msg failed, ret=%d!!!\n", ret);
        return -1;
    }

    ret = zxdh_tools_genl_msg_mk_usr_msg(skb, ZXDH_TOOLS_A_MSG, data, len);
    if (ret) {
        DHTOOLS_LOG_ERR("zxdh_tools_genl_msg_mk_usr_msg failed, ret=%d!!!\n", ret);
        kfree_skb(skb);
        return -1;
    }

    ret = genlmsg_unicast(&init_net, skb, pid);
    if (ret != 0) {
        struct task_struct *task = NULL;
        task = pid_task(find_vpid(pid), PIDTYPE_PID);
        if (!task) {
            DHTOOLS_LOG_ERR("dhtool with pid %d has exited!\n", pid);
        }
        DHTOOLS_LOG_ERR("genlmsg_unicast failed, ret=%d!!!\n", ret);
        return -1;
    }
    //DHTOOLS_LOG_INFO("genlnetlink msg send to user success.\n");
    return 0;
}


/* Started by AICoder, pid:t59ebl8546g627314a890b5e207c778ad317015e */
extern struct dhtool_eventpid_devbdf_array eventpid_devbdf_array[MAX_DHTOOL_PID_NUMS];
int32_t dhtool_find_eventpid_of_devbdf(uint32_t dev_bdf, uint32_t *event_pid)
{
    int i = 0;
    for (i = 0; i < ARRAY_SIZE(eventpid_devbdf_array); i++)
    {
        if(eventpid_devbdf_array[i].is_valid) {
            if(dev_bdf == eventpid_devbdf_array[i].dev_bdf) {
                *event_pid = eventpid_devbdf_array[i].event_pid;
                //DHTOOLS_LOG_INFO("found event_pid = %u of dev_bdf %d.\n", eventpid_devbdf_array[i].event_pid, dev_bdf);
                return 0;
            }
        }
    }

    DHTOOLS_LOG_ERR("can not found the event_pid of dev_bdf %d.\n", dev_bdf);
    return -1;
}
/* Ended by AICoder, pid:t59ebl8546g627314a890b5e207c778ad317015e */


/* Started by AICoder, pid:d8c3fca75fx34f614b080ba560ac35630222a5da */
int32_t zxdh_tools_sendto_user_netlink(void *pay_load, uint16_t len, void *reps_buffer, uint16_t *reps_len, void *dev)
{
    // 获取设备信息
    struct zxdh_en_device *en_dev = (struct zxdh_en_device *)dev;
    int32_t ret = 0;
    uint32_t event_pid = 0;
    struct pci_dev *pdev = NULL;
    uint32_t domain_no = 0;
    uint32_t bus_no = 0;
    uint32_t device_no = 0;
    uint32_t func_no = 0;
    uint32_t dev_bdf = 0;

    // 检查设备是否为空
    if (en_dev == NULL) {
        DHTOOLS_LOG_ERR("dev is NULL\n");
        return -1;
    }

    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF)
    {
        return 0;
    }

    // 检查负载和长度是否有效
    if ((pay_load == NULL) || (len == 0)) {
        DHTOOLS_LOG_ERR("invalid para, pay_load = 0x%llx, len = %d\n", (uint64_t)pay_load, len);
        return -1;
    }

    // 获取PCI设备
    pdev = en_dev->ops->get_pdev(en_dev->parent);
    if (!pdev) {
        DHTOOLS_LOG_ERR("pdev is NULL\n");
        return -1;
    }

    // 解析PCI设备名称以获取域号、总线号、设备号和功能号
    ret = sscanf(pci_name(pdev), "%x:%x:%x.%u", &domain_no, &bus_no, &device_no, &func_no);
    if(ret != 4) {
        DHTOOLS_LOG_ERR_DEV(en_dev->parent, "could not get dev domain_no、bus_no、device_no、func_no from pci_name(pdev)\n");
        return -1;
    }

    // 计算设备的BDF（基地址寄存器）
    dev_bdf = DBDF_ECAM(domain_no, bus_no, device_no, func_no);

    // 查找事件PID
    ret = dhtool_find_eventpid_of_devbdf(dev_bdf, &event_pid);
    if(ret != 0) {
        return -1;
    }

    // 特殊处理操作码为 EVENT_OP_CODE_LOG_GET_FINISH_TO_H 的情况
    if((*(uint32_t *)pay_load) == EVENT_OP_CODE_LOG_GET_FINISH_TO_H) {
        //DHTOOLS_LOG_INFO("*(uint32_t *)pay_load(op_code)=%d\n", *(uint32_t *)pay_load);
    }

    // 发送消息到用户空间
    ret = zxdh_tools_genl_msg_send_to_user(pay_load, len, event_pid);
    if (ret) {
        DHTOOLS_LOG_ERR_DEV(en_dev->parent, "zxdh_tools_genl_msg_send_to_user failed, ret=%d!!!\n", ret);
        return -1;
    }

    return 0;
}
/* Ended by AICoder, pid:d8c3fca75fx34f614b080ba560ac35630222a5da */



/* doit函数*/
int32_t zxdh_tools_genl_recv_doit(struct sk_buff *skb, struct genl_info *info)
{
    DHTOOLS_LOG_INFO("is called!\n");
    return 0;
}

int32_t zxdh_tools_netlink_register(void)
{
    int ret = 0;
    ret = genl_register_family(&zxdh_tools_msg_family);
    if(ret) {
       DHTOOLS_LOG_ERR("zxdh_tools_netlink_family register failed, ret=%d!!!\n", ret);
       return -1;
    }

    return 0;
}


void zxdh_tools_netlink_unregister(void)
{
    genl_unregister_family(&zxdh_tools_msg_family);
}

