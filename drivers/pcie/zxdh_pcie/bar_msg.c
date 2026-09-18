#include "bar_msg.h"
#include "pcie_common.h"

int hpf_send_msg_to_riscv(void *msg_info, u32 msg_size, void *resp_msg, u32 resp_size, struct pci_dev *pdev)
{
    struct zxdh_pci_bar_msg in = {0};
    struct zxdh_msg_recviver_mem result = {0};
    void __iomem *bar_virt_addr = NULL;
    u8 head_data = 0xFF;
    u16 ret = 0;
    u16 pcie_id = 0;
    u64 bar_addr = 0;
    u64 bar_len = 0;

    if (msg_info == NULL)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "The msg_info is NULL\n");
        return -EINVAL;
    }

    if (pdev == NULL)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "Can not find devices: deviceID %x, VendorID: %x\n", HP_VENDOR_ID, HP_DEVICE_ID);
        return -EINVAL;
    }

    bar_addr = pci_resource_start(pdev, 0);
    bar_len = pci_resource_len(pdev, 0);
    bar_virt_addr = ioremap(bar_addr, bar_len);

    /* 填充用户参数in */
    in.virt_addr = (u64)bar_virt_addr + HP_BAR_MSG_OFFSET; /* 使用PF1的bar0偏移8k */
    in.payload_addr = msg_info;                            /* 消息静荷buffer地址 */
    in.payload_len = msg_size;                             /* 消息长度 */
    in.src = MSG_CHAN_END_PF;                              /* 从mpf通道下发 */
    in.dst = MSG_CHAN_END_RISC;                            /* 消息发到risc */
    in.event_id = PCIE_FUNC_HP_EVENT_ID;                   /* 调用PCIE的消息处理函数 */
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
        DH_LOG_ERR(MODULE_FUC_HP, "Pcie send msg failed, ret:%d.\n", ret);
        goto exit;
    }

    /* 如果消息发送成功， 从recv_buffer + 1的位置往后两字节取回复数据长度， recv_buffer + 4的位置开始取数据内容 */
    head_data = *((u8 *)(result.recv_buffer + 4));
    if (head_data == 0x1)
    {
        DH_LOG_INFO(MODULE_FUC_HP, "Pcie send bar_info SUCCESS!\n");
        memcpy(resp_msg, result.recv_buffer + 4, resp_size);
        ret = 0;
    }
    else
    {
        DH_LOG_ERR(MODULE_FUC_HP, "Pcie send bar_info fail!\n");
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