#include <linux/dinghai/dh_cmd.h>
#include "fuc_hotplug.h"
#include "fuc_hotplug_commom.h"

extern int get_fuc_hp_ret(void);
extern int reset_fuc_hp_ret(void);
static int fuc_hotplug_trigger(u64 arg);
static int fuc_hotplug_get_pf_state(u64 arg);
static int ep_hotplug(u64 arg);

static struct func_sel ioctl_func_sel[] = {
    {FUC_HP_IOCTL_CMD0,  fuc_hotplug_trigger         },
    {FUC_HP_IOCTL_CMD1,  fuc_hotplug_get_pf_state    },
    {FUC_HP_IOCTL_CMD2,  ep_hotplug                  },
};

static struct zte_id_info zf_mpf_id_info[] = {
    /*  name,           device_id, vendor_id*/
    {"x510_zf_mpf",      0x8044,    0x1cf2},
    {"x512_zf_mpf",      0x806a,    0x1cf2},
};

static int zf_mpf_send_msg_to_riscv(void *msg_info, u32 msg_size, void *resp_msg, u32 resp_size)
{
    struct zxdh_pci_bar_msg in = {0};
    struct zxdh_msg_recviver_mem result = {0};
    struct pci_dev *pdev = NULL;
    void __iomem *bar_virt_addr = NULL;
    u16 ret = 0, pcie_id = 0;
    u64 bar_addr = 0;
    u64 bar_len = 0;
    int i = 0;

    if (msg_info == NULL)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "The msg_info is NULL\n");
        return -EINVAL;
    }

    for (i = 0; i < sizeof(zf_mpf_id_info) / sizeof(struct zte_id_info); i++)
    {
        pdev = pci_get_device(zf_mpf_id_info[i].vendor_id, zf_mpf_id_info[i].device_id, NULL);
        if (pdev)
        {
            break;
        }
    }

    if (pdev == NULL)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "Can not find zf_mpf devices\n");
        return -EINVAL;
    }

    bar_addr = pci_resource_start(pdev, 0);
    bar_len = pci_resource_len(pdev, 0);
    DH_LOG_INFO(MODULE_FUC_HP, "bar_addr->0x%llx\n", bar_addr);
    bar_virt_addr = ioremap(bar_addr, bar_len);

    /* 填充用户参数in */
    in.virt_addr = (u64)bar_virt_addr + FUC_HP_BAR_MSG_OFFSET; /* 使用PF1的bar0偏移8k */
    in.payload_addr = msg_info;                                /* 消息静荷buffer地址 */
    in.payload_len = msg_size;                                 /* 消息长度 */
    in.src = MSG_CHAN_END_PF;                                  /* 从mpf通道下发 */
    in.dst = MSG_CHAN_END_RISC;                                /* 消息发到risc */
    in.event_id = FUC_HP_EVENT_ID;                             /* 调用PCIE的消息处理函数 */
    in.src_pcieid = pcie_id;

    result.buffer_len = BUF_SIZE;                                /* 用户准备一个存放消息回复的buffer, buffer长度 */
    result.recv_buffer = kmalloc(result.buffer_len, GFP_KERNEL); /* 消息回复buffer地址 */
    if (!result.recv_buffer)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "Failed to allocate recv_buffer\n");
        ret = -EINVAL;
        goto free_map;
    }

    memset(result.recv_buffer, 0, result.buffer_len);
    ret = zxdh_bar_chan_sync_msg_send(&in, &result); /* 发送同步消息 */

    /* 如果接口返回值不为0，则说明消息失败 */
    if (ret != 0)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "pcie send msg failed, ret:%d.\n", ret);
        goto exit;
    }

    /* 如果消息发送成功， 从recv_buffer + 1的位置往后两字节取回复数据长度， recv_buffer + 4的位置开始取数据内容 */
    if (*((u8 *)(result.recv_buffer + 4)) == 0x1)
    {
        memcpy(resp_msg, result.recv_buffer + 4, resp_size);
        ret = 0;
    }
    else
    {
        DH_LOG_ERR(MODULE_FUC_HP, "pcie  result failed!\n");
        ret = -EINVAL;
    }

exit:
    kfree(result.recv_buffer);
    result.recv_buffer = NULL;

free_map:
    iounmap(bar_virt_addr);
    bar_virt_addr = NULL;
    return ret;
}

static int fuc_hotplug_trigger(u64 arg)
{
    int ret = 0;
    int resp_msg = 0;
    struct fuc_hotplug_bar_msg fuc_hotplug_bar_msg = {0};

    if (copy_from_user(&fuc_hotplug_bar_msg, (void __user *)arg, sizeof(struct fuc_hotplug_bar_msg)))
    {
        DH_LOG_ERR(MODULE_FUC_HP, "Can not copy from user\n");
        return -EFAULT;
    }

    ret = zf_mpf_send_msg_to_riscv(&fuc_hotplug_bar_msg, sizeof(struct fuc_hotplug_bar_msg), &resp_msg, sizeof(int));
    if (ret != 0)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "send failed\n");
        fuc_hotplug_bar_msg.cpl_chk = FUC_HP_RET_FAILED;
    }
    else
    {
        fuc_hotplug_bar_msg.cpl_chk = FUC_HP_RET_FINISH;
    }

    if (copy_to_user((void __user *)arg, &fuc_hotplug_bar_msg, sizeof(struct fuc_hotplug_bar_msg)))
    {
        DH_LOG_ERR(MODULE_FUC_HP, "Can not copy to user\n");
        ret = -EFAULT;
    }

    return ret;
}

static int fuc_hotplug_get_pf_state(u64 arg)
{
    int ret = FUC_HP_OK;
    struct get_pf_state_resp get_pf_state_resp = {0};
    struct get_pf_state_info get_pf_state_info = {0};

    if (copy_from_user(&get_pf_state_info, (void __user *)arg, sizeof(struct get_pf_state_info)))
    {
        DH_LOG_ERR(MODULE_FUC_HP, "Can not copy from user\n");
        return -EFAULT;
    }

    ret = zf_mpf_send_msg_to_riscv(&get_pf_state_info, sizeof(struct get_pf_state_info), (void *)&get_pf_state_resp, sizeof(struct get_pf_state_resp));
    if (ret)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "Remote test failed\n");
        goto finish;
    }

    if (get_pf_state_info.ep_no >= MAX_FUCTION_HOTPLUG_EP_NUMS)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "Invalid ep_id\n");
        ret = -EINVAL;
        goto finish;
    }

    get_pf_state_info.cpl_chk = ( get_pf_state_resp.pf_state_of_ep[get_pf_state_info.ep_no]  >> get_pf_state_info.pf_no) & 0x1;
    if (copy_to_user((void __user *)arg, &get_pf_state_info, sizeof(struct get_pf_state_info)))
    {
        DH_LOG_ERR(MODULE_FUC_HP, "Can not copy to user\n");
        ret = -EFAULT;
    }

    return ret;

finish:
    get_pf_state_info.cpl_chk = FUNCTION_INVALID_TYPE;
    if (copy_to_user((void __user *)arg, &get_pf_state_info, sizeof(struct get_pf_state_info)))
    {
        DH_LOG_ERR(MODULE_FUC_HP, "Can not copy to user\n");
        ret = -EFAULT;
    }

    return ret;
}

static int ep_hotplug(u64 arg)
{
    int ret = FUC_HP_OK;
    struct ep_hotplug_resp ep_hotplug_resp = {0};
    struct ep_hotplug_info ep_hotplug_info = {0};

    if (copy_from_user(&ep_hotplug_info, (void __user *)arg, sizeof(struct ep_hotplug_info)))
    {
        DH_LOG_ERR(MODULE_FUC_HP, "Can not copy from user\n");
        return -EFAULT;
    }

    ret = zf_mpf_send_msg_to_riscv(&ep_hotplug_info, sizeof(struct ep_hotplug_info), (void *)&ep_hotplug_resp, sizeof(struct ep_hotplug_resp));
    if (ret)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "Remote test failed\n");
        goto finish;
    }

    ep_hotplug_info.cpl_chk = FUC_HP_RET_FINISH;
    if (copy_to_user((void __user *)arg, &ep_hotplug_info, sizeof(struct ep_hotplug_info)))
    {
        DH_LOG_ERR(MODULE_FUC_HP, "Can not copy to user\n");
        ret = -EFAULT;
    }

    return ret;

finish:
    ep_hotplug_info.cpl_chk = FUC_HP_RET_FAILED;
    if (copy_to_user((void __user *)arg, &ep_hotplug_info, sizeof(struct ep_hotplug_info)))
    {
        DH_LOG_ERR(MODULE_FUC_HP, "Can not copy to user\n");
        ret = -EFAULT;
    }

    return ret;
}


long fuc_hp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    u32 i = 0;
    u32 ioctl_func_nums = sizeof(ioctl_func_sel) / sizeof(struct func_sel);

    for (i = 0; i < ioctl_func_nums; i++)
    {
        if (ioctl_func_sel[i].cmd == cmd)
        {
            return ioctl_func_sel[i].ioctl_func(arg);
        }
    }

    return -EINVAL;
}
