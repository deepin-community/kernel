#include <linux/dinghai/dh_cmd.h>
#include "en_aux_ioctl.h"
#include "en_aux_cmd.h"
#include "../zxdh_tools/zxdh_tools_ioctl.h"
#include "queue.h"
#include "../en_np/table/include/dpp_tbl_api.h"
#include "../en_pf/msg_func.h"
#include "../en_pf/en_pf_eq.h"
#ifdef CONFIG_DINGHAI_TSN
#include "../en_tsn/zxdh_tsn_ioctl.h"
#endif

#ifdef PTP_DRIVER_INTERFACE_EN
extern int32_t tod_device_set_bar_virtual_addr(uint64_t virtaddr, uint16_t pcieid);
#endif
int32_t print_data(uint8_t *data, uint32_t len)
{
    int32_t i = 0;
    uint32_t loopcnt = 0;
    uint32_t last_line_len = 0;
    uint32_t line_len = PKT_PRINT_LINE_LEN;
    uint8_t last_line_data[PKT_PRINT_LINE_LEN] = {0};

    if (len == 0)
    {
        return 0;
    }
    loopcnt = len / line_len;
    last_line_len = len % line_len;

    LOG_DEBUG("***************packet data[len: %d]***************\n", len);
    for (i = 0; i < loopcnt; i++)
    {
        LOG_INFO("%.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x\n",                            \
                 *(data + (line_len * i) + 0), *(data + (line_len * i) + 1), *(data + (line_len * i) + 2), *(data + (line_len * i) + 3),     \
                 *(data + (line_len * i) + 4), *(data + (line_len * i) + 5), *(data + (line_len * i) + 6), *(data + (line_len * i) + 7),     \
                 *(data + (line_len * i) + 8), *(data + (line_len * i) + 9), *(data + (line_len * i) + 10), *(data + (line_len * i) + 11),   \
                 *(data + (line_len * i) + 12), *(data + (line_len * i) + 13), *(data + (line_len * i) + 14), *(data + (line_len * i) + 15));
    }
    if (last_line_len != 0)
    {
        memcpy(last_line_data, (data + (line_len * i)), last_line_len);
        LOG_INFO("%.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x\n", \
                 last_line_data[0], last_line_data[1], last_line_data[2], last_line_data[3],     \
                 last_line_data[4], last_line_data[5], last_line_data[6], last_line_data[7],     \
                 last_line_data[8], last_line_data[9], last_line_data[10], last_line_data[11],   \
                 last_line_data[12], last_line_data[13], last_line_data[14], last_line_data[15]);
    }
    LOG_INFO("****************end packet data**************\n");

    return 0;
}

int32_t zxdh_read_reg_cmd(struct net_device *netdev, struct ifreq *ifr)
{
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    struct zxdh_en_reg *reg = NULL;
    uint32_t size = sizeof(struct zxdh_en_reg);
    uint64_t base_addr = 0;
    uint32_t num = 0;
    int32_t err = 0;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;

    reg = kzalloc(size, GFP_KERNEL);
    CHECK_EQUAL_ERR(reg, NULL, -EADDRNOTAVAIL, "reg is null!\n");

    if (copy_from_user(reg, ifr->ifr_ifru.ifru_data, size))
    {
        LOG_ERR_DEV(en_dev->parent, "copy_from_user failed\n");
        err = -EFAULT;
        goto err_ret;
    }

    if ((reg->num == 0) || (reg->num > MAX_ACCESS_NUM))
    {
        LOG_ERR_DEV(en_dev->parent, "transmit failed, reg->num=%u\n", reg->num);
        err = -EFAULT;
        goto err_ret;
    }

    base_addr = en_dev->ops->get_bar_virt_addr(en_dev->parent, 0);

    for (num = 0; num < reg->num; num++)
    {
        reg->data[num] = readl((const volatile void *)(base_addr + (reg->offset & 0xfffffffc) + num * 4));
    }

    if (copy_to_user(ifr->ifr_ifru.ifru_data, reg,  size))
    {
        LOG_ERR_DEV(en_dev->parent, "copy_to_user failed\n");
        err = -EFAULT;
    }

err_ret:
    kfree(reg);
    return err;
}

int32_t zxdh_write_reg_cmd(struct net_device *netdev, struct ifreq *ifr)
{
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    struct zxdh_en_reg *reg = NULL;
    uint32_t size = sizeof(struct zxdh_en_reg);
    uint64_t base_addr = 0;
    uint32_t num = 0;
    int32_t err = 0;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;

    reg = kzalloc(size, GFP_KERNEL);
    CHECK_EQUAL_ERR(reg, NULL, -EADDRNOTAVAIL, "reg is null!\n");

    if (copy_from_user(reg, ifr->ifr_ifru.ifru_data, size))
    {
        LOG_ERR_DEV(en_dev->parent, "copy_from_user failed\n");
        err = -EFAULT;
        goto err_ret;
    }

    if ((reg->num == 0) || (reg->num > MAX_ACCESS_NUM))
    {
        LOG_ERR_DEV(en_dev->parent, "transmit failed, reg->num=%u\n", reg->num);
        err = -EFAULT;
        goto err_ret;
    }

    base_addr = en_dev->ops->get_bar_virt_addr(en_dev->parent, 0);

    for (num = 0; num < reg->num; num++)
    {
        writel(reg->data[num], (volatile void *)(base_addr + (reg->offset & 0xfffffffc) + num * 4));
    }

err_ret:
    kfree(reg);
    return err;
}

int32_t print_vring_info(struct virtqueue *vq, struct zxdh_en_reg *reg)
{
    struct vring_virtqueue *vvq = to_vvq(vq);

    if (vvq->packed_ring == true)
    {
        if ((reg->num + reg->data[0]) > vvq->packed.vring.num)
        {
            LOG_ERR("the sum of desc_index %u and desc_num %u over desc depth %u, should be [0-%u]\n", \
                    reg->num, reg->data[0], vvq->packed.vring.num, vvq->packed.vring.num - 1);
            return -EINVAL;
        }
    }
    else
    {
        if ((reg->num + reg->data[0]) > vvq->split.vring.num)
        {
            LOG_ERR("the sum of desc_index %u and desc_num %u over desc depth %u, should be [0-%u]\n", \
                    reg->num, reg->data[0], vvq->split.vring.num, vvq->split.vring.num - 1);
            return -EINVAL;
        }
    }

    zxdh_print_vring_info(vq, reg->num, reg->data[0]);

    return 0;
}

int32_t zxdh_en_show_vf_vring_info(struct zxdh_en_device *en_dev, struct zxdh_en_reg *reg)
{
    int32_t ret = 0;
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = 0;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    if (!en_dev->ops->get_vf_is_probe(en_dev->parent, reg->data[2]))
    {
        LOG_ERR_DEV(en_dev->parent, "vf(%u) is not probed\n",reg->data[2]);
        ret = EOPNOTSUPP;
        goto free_msg;
    }

    msg->payload.hdr_vf.op_code = ZXDH_VF_QUEUE_INFO_SHOW;
    msg->payload.hdr_vf.dst_pcie_id = FIND_VF_PCIE_ID(en_dev->pcie_id, reg->data[2]);
    msg->payload.vf_queue_info_msg.queue_idx = reg->offset;
    msg->payload.vf_queue_info_msg.desc_start = reg->num;
    msg->payload.vf_queue_info_msg.desc_num = reg->data[0];
    msg->payload.vf_queue_info_msg.vf_idx = reg->data[2];

    err = en_dev->ops->msg_send_cmd(en_dev->parent,  MODULE_PF_BAR_MSG_TO_VF, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "pf send msg to vf to show vf queue info failed, err: %d\n", err);
        goto free_msg;
    }

free_msg:
    kfree(msg);
    return ret;
}

/* Started by AICoder, pid:yfb85v143eac661140ba0a3d2045a86e9421ac1b */
int32_t zxdh_queue_info_common_print(struct zxdh_en_device *en_dev, struct zxdh_en_reg *reg)
{
    uint32_t vf_id = GET_VFID(en_dev->vport);
    struct virtqueue *vq = NULL;
    struct pci_dev *pdev = NULL;
    uint32_t domain = 0;
    uint32_t bus= 0;
    uint32_t devid = 0;
    uint32_t function = 0;
    int32_t ret = 0;

    if (reg->offset >= en_dev->max_queue_pairs)
    {
        LOG_ERR_DEV(en_dev->parent, "the queue index %u over the max_queue_pairs %u, should be [0-%u]\n", \
                 reg->offset, en_dev->max_queue_pairs, en_dev->max_queue_pairs - 1);
        return -EINVAL;
    }

    pdev = en_dev->ops->get_pdev(en_dev->parent);
    if (sscanf(pci_name(pdev), "%x:%x:%x.%u", &domain, &bus, &devid, &function) != 4)
    {
        LOG_ERR_DEV(en_dev->parent, "failed to get pcie rp bus-info\n");
        return -EINVAL;
    }

    LOG_INFO("*************************************************device info*********************************************\n");
    LOG_INFO("dev_name  : %s", en_dev->netdev->name);
    if (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
        LOG_INFO("vf_idx    : %d\n", reg->data[2]);
    LOG_INFO("mac_addr  : %pM\n", en_dev->netdev->dev_addr);
    LOG_INFO("vport     : %#x\n", en_dev->vport);
    LOG_INFO("vf_id     : %d\n",vf_id);
    LOG_INFO("phy_port  : %#x\n", en_dev->phy_port);
    LOG_INFO("ep_bdf    : %04x:%02x:%02x.%x\n", domain, bus, devid, function);
    LOG_INFO("pcie_id   : %#x\n", en_dev->pcie_id);
    LOG_INFO("slot_id   : %d\n", en_dev->slot_id);
    LOG_INFO("panel_id  : %d\n", en_dev->panel_id);
    LOG_INFO("is_bond   : %d\n", en_dev->ops->is_bond(en_dev->parent));
    LOG_INFO("duplex    : %#x\n", en_dev->duplex);
    LOG_INFO("ro_func   : %d\n", en_dev->ro_flag);

    vq = en_dev->sq[reg->offset].vq;
    LOG_INFO("*************************************************tx vring info*********************************************\n");
    ret = print_vring_info(vq, reg);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "print tx vring info failed!\n");
        return -EINVAL;
    }

    vq = en_dev->rq[reg->offset].vq;
    LOG_INFO("*************************************************rx vring info*********************************************\n");
    ret = print_vring_info(vq, reg);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "print rx vring info failed!\n");
        return -EINVAL;
    }
    return 0;
}
/* Ended by AICoder, pid:yfb85v143eac661140ba0a3d2045a86e9421ac1b */

int32_t zxdh_get_vring_info(struct net_device *netdev, struct ifreq *ifr)
{
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    struct zxdh_en_reg *reg = NULL;
    uint32_t size = sizeof(struct zxdh_en_reg);
    int32_t ret = 0;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;

    reg = kzalloc(size, GFP_KERNEL);
    CHECK_EQUAL_ERR(reg, NULL, -EADDRNOTAVAIL, "reg is null!\n");

    if (copy_from_user(reg, ifr->ifr_ifru.ifru_data, size))
    {
        LOG_ERR_DEV(en_dev->parent, "copy_from_user failed\n");
        ret = -EFAULT;
        goto err_ret;
    }

    if (reg->data[1] == 1)
    {
        ret = zxdh_en_show_vf_vring_info(en_dev, reg);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_en_show_vf_vring_info, ret: %d\n", ret);
            goto err_ret;
        }
    }
    else if (reg->data[1] == 0)
    {
        ret = zxdh_queue_info_common_print(en_dev, reg);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_queue_info_common_print failed, ret: %d\n", ret);
            goto err_ret;
        }
    }
    else
    {
        LOG_ERR_DEV(en_dev->parent, "invalid data[1]:%d, ret: %d\n", ret, reg->data[1]);
        ret = -EINVAL;
        goto err_ret;
    }

err_ret:
    kfree(reg);
    return ret;
}

int32_t zxdh_en_set_clock_no(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg)
{
    uint32_t reg_size = sizeof(struct zxdh_en_reg);
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;

    if (reg->num != 1)
    {
        LOG_ERR_DEV(en_dev->parent, "Transmit failed[len = %d]!\n", reg->num);
        goto err_ret;
    }

    en_dev->clock_no = reg->data[0];
    LOG_INFO_DEV(en_dev->parent, "en_dev %s clock_no = %d\n", en_dev->netdev->name, en_dev->clock_no);

    reg->num = 0;
    if (unlikely(copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size)))
    {
        LOG_ERR_DEV(en_dev->parent, "copy_to_user failed!\n");
        goto err_ret;
    }

    return 0;

err_ret:
    return -1;
}

void copy_u32_to_u8(uint8_t *data_pkt, uint32_t *data, uint32_t pktlen)
{
    uint32_t i = 0;

    for (i = 0; i < pktlen; i++)
    {
        *data_pkt++ = data[i];
    }
}

int32_t zxdh_tx_file_pkts(struct zxdh_en_priv *en_priv, struct zxdh_en_reg *reg)
{
    int32_t total_sg = 0;
    uint8_t *data_pkt = NULL;
    struct scatterlist *sg = NULL;
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct send_queue *sq = en_dev->sq;
    struct page *page = NULL;
    struct data_packet pkt = {0};
    uint16_t i = 0;
    uint32_t len = 0;
    void *ptr = NULL;
    uint32_t last_buff_len = 0;
    uint32_t pktLen = reg->num;
    uint32_t buffLen = 4096;

    while ((ptr = virtqueue_get_buf_ctx(sq->vq, &len, NULL, FALSE, NULL)) != NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "virtqueue_get_buf_ctx() != NULL, ptr=0x%llx, len=0x%x\n", (uint64_t)ptr, len);
    };

    sg = sq->sg;
    pkt.buf_size = 16 * PAGE_SIZE;
    page = alloc_pages(GFP_KERNEL, 4);
    if (unlikely(page == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "page is null\n");
        goto err;
    }

    pkt.buf = page_address(page);
    if (unlikely(pkt.buf == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "pkt.buf is null\n");
        goto err1;
    }
    memset(pkt.buf, 0, pkt.buf_size);

    data_pkt = (uint8_t*)pkt.buf;
    copy_u32_to_u8(data_pkt, reg->data, pktLen);
    print_data(data_pkt, (pktLen > PKT_PRINT_LEN_MAX) ? PKT_PRINT_LEN_MAX : pktLen);

    total_sg = pktLen / buffLen;
    last_buff_len = pktLen % buffLen;
    if (last_buff_len != 0)
    {
        total_sg += 1;
    }

    sg_init_table(sg, total_sg);
    for (i = 0; i < total_sg; i++)
    {
        if (i == (total_sg - 1))
        {
            sg_set_buf(&sg[i], data_pkt + (i * buffLen), ((last_buff_len != 0) ? last_buff_len : buffLen));
        }
        else
        {
            sg_set_buf(&sg[i], data_pkt + (i * buffLen), buffLen);
        }
    }

    if (unlikely(virtqueue_add_outbuf(sq->vq, sg, total_sg, data_pkt, GFP_ATOMIC) != 0))
    {
        LOG_ERR_DEV(en_dev->parent, "virtqueue_add_outbuf failure!\n");
        goto err1;
    }

    if (virtqueue_kick_prepare(sq->vq) && virtqueue_notify(sq->vq))
    {
        u64_stats_update_begin(&sq->stats.syncp);
        sq->stats.kicks++;
        u64_stats_update_end(&sq->stats.syncp);
    }

    en_dev->netdev->stats.tx_packets++;
    en_dev->netdev->stats.tx_bytes += pktLen;
    LOG_INFO_DEV(en_dev->parent, "en_dev->netdev->stats.tx_packets=%ld, tx pktLen=%d\n",
                en_dev->netdev->stats.tx_packets, pktLen);

    return 0;

err1:
    free_pages((uint64_t)pkt.buf, 4);
err:
    return -1;
}

int32_t zxdh_send_file_pkt(struct net_device *netdev, struct ifreq *ifr)
{
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_reg *reg = NULL;
    uint32_t size = sizeof(struct zxdh_en_reg);
    int32_t ret = 0;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");

    reg = kzalloc(size, GFP_KERNEL);
    CHECK_EQUAL_ERR(reg, NULL, -EADDRNOTAVAIL, "reg is null!\n");

    if (copy_from_user(reg, ifr->ifr_ifru.ifru_data, size))
    {
        LOG_ERR_DEV(en_priv->edev.parent, "copy_from_user failed\n");
        ret = -EFAULT;
        goto err_ret;
    }

    if ((reg->num == 0) || (reg->num > MAX_ACCESS_NUM))
    {
        LOG_ERR_DEV(en_priv->edev.parent, "transmit failed, reg->num=%d\n", reg->num);
        ret = -EFAULT;
        goto err_ret;
    }

    ret = zxdh_tx_file_pkts(en_priv, reg);
    if (unlikely(ret != 0))
    {
        LOG_ERR_DEV(en_priv->edev.parent, "transmit failed[ret = %d]!", ret);
        ret = -1;
        goto err_ret;
    }

    reg->num = 0;
    if (copy_to_user(ifr->ifr_ifru.ifru_data, reg, size))
    {
        LOG_ERR_DEV(en_priv->edev.parent, "copy_to_user failed\n");
        ret = -EFAULT;
    }

err_ret:
    kfree(reg);
    return ret;
}

#ifdef  PTP_DRIVER_INTERFACE_EN
/* ptp发送加密报文时，需要调用使能函数进行使能 */
extern int32_t enable_write_ts_to_fifo(struct zxdh_en_device *en_dev, uint32_t enable, uint32_t mac_number);
extern int32_t set_interrupt_capture_timer(struct zxdh_en_device *en_dev, uint32_t index);
extern int32_t zxdh_set_pps_selection(struct zxdh_en_device *en_dev, uint32_t pps_type, uint32_t selection);
extern int32_t zxdh_set_pd_detection(struct zxdh_en_device *en_dev, uint32_t pd_index, uint32_t pd_input1, uint32_t pd_input2);
extern int32_t zxdh_get_pd_value(struct zxdh_en_device *en_dev, uint32_t pd_index, uint32_t *pd_result);
extern int32_t zxdh_set_pps_interrupt_support(struct zxdh_en_device *en_dev, uint32_t support);
extern int32_t zxdh_get_pps_interrupt_support(struct zxdh_en_device *en_dev, uint32_t *support);
extern int32_t zxdh_set_local_pps_interrupt_enable(struct zxdh_en_device *en_dev, uint32_t enable);
extern int32_t zxdh_set_ext_pps_interrupt_enable(struct zxdh_en_device *en_dev, uint32_t pps_src, uint32_t enable);
extern int32_t zxdh_set_pd_sel_shift(struct zxdh_en_device *en_dev, uint32_t pd_index, uint32_t sel, uint32_t shift);
extern int32_t zxdh_get_ptp_clock_index(struct zxdh_en_device *en_dev, uint32_t *ptp_clock_idx);
#endif /* PTP_DRIVER_INTERFACE_EN */
int32_t zxdh_en_enable_ptp_encrypted_msg(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg)
{
    uint32_t reg_size = sizeof(struct zxdh_en_reg);
    int32_t mac_num = 0; //0-2
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    uint32_t enable = 0;
    int32_t ret = 0;

    LOG_INFO("enter in zxdh_en_enable_ptp_encrypted_msg\n");
    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;

    mac_num = zxdh_pf_macpcs_num_get(en_dev);
    if (mac_num < 0)
    {
        LOG_ERR_DEV(en_dev->parent, "get mac num %d err, its value should is 0-2!\n", mac_num);
        goto err_ret;
    }

    if (unlikely(copy_from_user(reg, ifr->ifr_ifru.ifru_data, reg_size)))
    {
        LOG_ERR_DEV(en_dev->parent, "copy_from_user failed!\n");
        goto err_ret;
    }
    if (reg->num != 1)
    {
        LOG_ERR_DEV(en_dev->parent, "Transmit failed[len = %d]!\n", reg->num);
        goto err_ret;
    }

    enable = reg->data[0];
    if ((enable != 0) && (enable != 1))
    {
        LOG_ERR_DEV(en_dev->parent, "Transmit failed[enable = %u]!\n", enable);
        goto err_ret;
    }

    LOG_INFO_DEV(en_dev->parent, "enable = %u\n", enable);

#ifdef  PTP_DRIVER_INTERFACE_EN
    /* 使能ptp加密报文发送接口 */
    ret = enable_write_ts_to_fifo(en_dev, enable, mac_num);
    CHECK_UNEQUAL_ERR(ret, 0, -EFAULT, "enable ptp encrypted msg failed!!\n");
#endif /* PTP_DRIVER_INTERFACE_EN */

    reg->num = 0;
    if (unlikely(copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size)))
    {
        LOG_ERR_DEV(en_dev->parent, "copy_to_user failed!\n");
        goto err_ret;
    }

    return ret;

err_ret:
    return -1;
}

int32_t zxdh_en_set_intr_capture_timer(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg)
{
    u_int32_t index;
    uint32_t reg_size = sizeof(struct zxdh_en_reg);
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    int32_t ret = 0;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;

    if (reg->num != 1)
    {
        LOG_ERR_DEV(en_dev->parent, "Transmit failed[len = %d]!", reg->num);
        goto err_ret;
    }

    index = reg->data[0];
    LOG_INFO_DEV(en_dev->parent, "index = %d\n", index);
    if (index > 4)
    {
        LOG_ERR_DEV(en_dev->parent, "capture timer out of range!");
        goto err_ret;
    }
#ifdef  PTP_DRIVER_INTERFACE_EN
    ret = set_interrupt_capture_timer(en_dev, index);
    CHECK_UNEQUAL_ERR(ret, 0, -EFAULT, "set interrupt capture timer failed!!\n");
#endif /* PTP_DRIVER_INTERFACE_EN */

    reg->num = 0;
    if (unlikely(copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size)))
    {
        LOG_ERR_DEV(en_dev->parent, "copy_to_user failed!!!\n");
        goto err_ret;
    }

    return ret;

err_ret:
    return -1;

}

int32_t zxdh_en_set_pps_selection(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg)
{
    uint32_t pps_type;
    uint32_t selection;
    uint32_t reg_size = sizeof(struct zxdh_en_reg);
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    int32_t ret = 0;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;

    if (reg->num != 2)
    {
        LOG_ERR_DEV(en_dev->parent, "Transmit failed[len = %d]!", reg->num);
        goto err_ret;
    }

    pps_type = reg->data[0];
    selection = reg->data[1];
    LOG_INFO_DEV(en_dev->parent, "pps_type = %u, selection = %u\n", pps_type, selection);
#ifdef  PTP_DRIVER_INTERFACE_EN
    ret = zxdh_set_pps_selection(en_dev, pps_type, selection);
    CHECK_UNEQUAL_ERR(ret, 0, -EFAULT, "set pps selection failed!!\n");
#endif /* PTP_DRIVER_INTERFACE_EN */

    reg->num = 0;
    if (unlikely(copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size)))
    {
        LOG_ERR_DEV(en_dev->parent, "copy_to_user failed!!!\n");
        goto err_ret;
    }

    return ret;

err_ret:
    return -1;
}

int32_t zxdh_en_set_phase_detection(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg)
{
    uint32_t pd_index;
    uint32_t pd_input1;
    uint32_t pd_input2;
    uint32_t reg_size = sizeof(struct zxdh_en_reg);
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    int32_t ret = 0;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;

    if (reg->num != 3)
    {
        LOG_ERR_DEV(en_dev->parent, "Transmit failed[len = %d]!", reg->num);
        goto err_ret;
    }

    pd_index = reg->data[0];
    pd_input1 = reg->data[1];
    pd_input2 = reg->data[2];
    LOG_INFO_DEV(en_dev->parent, "pd_index = %u, pd_input1 = %u, pd_input2 = %u\n", pd_index, pd_input1, pd_input2);
#ifdef  PTP_DRIVER_INTERFACE_EN
    ret = zxdh_set_pd_detection(en_dev, pd_index, pd_input1, pd_input2);
    CHECK_UNEQUAL_ERR(ret, 0, -EFAULT, "set pd detection failed!!\n");
#endif /* PTP_DRIVER_INTERFACE_EN */

    reg->num = 0;
    if (unlikely(copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size)))
    {
        LOG_ERR_DEV(en_dev->parent, "copy_to_user failed!!!\n");
        goto err_ret;
    }

    return ret;

err_ret:
    return -1;
}

int32_t zxdh_en_get_pd_value(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg)
{
    uint32_t pd_index;
    uint32_t pd_result;
    uint32_t reg_size = sizeof(struct zxdh_en_reg);
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    int32_t ret = 0;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;

    if (reg->num != 1)
    {
        LOG_ERR_DEV(en_dev->parent, "Transmit failed[len = %d]!", reg->num);
        goto err_ret;
    }

    pd_index = reg->data[0];
    LOG_INFO_DEV(en_dev->parent, "pd_index = %u\n", pd_index);
    pd_result = 0; /* 3.10 内核未启用 PTP 时初始化为 0 */
#ifdef  PTP_DRIVER_INTERFACE_EN
    ret = zxdh_get_pd_value(en_dev, pd_index, &pd_result);
    CHECK_UNEQUAL_ERR(ret, 0, -EFAULT, "get pd value failed!!\n");
#endif /* PTP_DRIVER_INTERFACE_EN */

    reg->num = 1;
    reg->data[0] = pd_result;
    if (unlikely(copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size)))
    {
        LOG_ERR_DEV(en_dev->parent, "copy_to_user failed!!!\n");
        goto err_ret;
    }

    return ret;

err_ret:
    return -1;
}

int32_t zxdh_en_set_l2_ptp_port(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg)
{
    uint32_t reg_size = sizeof(struct zxdh_en_reg);
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    int32_t ret = 0;
    DPP_PF_INFO_T pf_info = {0};

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;
    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    LOG_INFO_DEV(en_dev->parent, "reg->num: %d", reg->num);
    LOG_INFO_DEV(en_dev->parent, "reg->offset: %d", reg->offset);
    if (reg->num != 1)
    {
        LOG_ERR_DEV(en_dev->parent, "Transmit failed[len = %d]!", reg->num);
        goto err_ret;
    }

    en_dev->vf_1588_call_np_num = PTP_PORT_VFID_SET;
    LOG_INFO_DEV(en_dev->parent, "en_dev->vport: 0x%x, IS_PF(en_dev->vport): %d", en_dev->vport, IS_PF(en_dev->vport));
    if (IS_PF(en_dev->vport))
    {
        ret = dpp_ptp_port_vfid_set(&pf_info, VQM_VFID(en_dev->vport));
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_ptp_port_vfid_set failed!!!\n");
            goto err_ret;
        }
    }
    else
    {
        ret = zxdh_vf_1588_call_np_interface(en_dev);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_vf_1588_call_np_interface failed!!!\n");
            goto err_ret;
        }
    }

    reg->num = 0;
    if (unlikely(copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size)))
    {
        LOG_ERR_DEV(en_dev->parent, "copy_to_user failed!!!\n");
        goto err_ret;
    }
    LOG_INFO_DEV(en_dev->parent, "dpp_ptp_port_vfid_set success");

    return ret;

err_ret:
    return -1;
}

int32_t zxdh_en_set_ptp_tc_enable(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg)
{
    uint32_t reg_size = sizeof(struct zxdh_en_reg);
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    int32_t ret = 0;
    DPP_PF_INFO_T pf_info = {0};

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;
    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    if (reg->num != 1)
    {
        LOG_ERR_DEV(en_dev->parent, "Transmit failed[len = %d]!", reg->num);
        goto err_ret;
    }

    en_dev->ptp_tc_enable_opt = reg->data[0];
    LOG_INFO_DEV(en_dev->parent, "en_dev->ptp_tc_enable_opt = %u\n", en_dev->ptp_tc_enable_opt);

    en_dev->vf_1588_call_np_num = PTP_TC_ENABLE_SET;

    if (IS_PF(en_dev->vport))
    {
        ret = dpp_ptp_tc_enable_set(&pf_info, en_dev->ptp_tc_enable_opt);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_ptp_tc_enable_set failed!!!\n");
            goto err_ret;
        }
    }
    else
    {
        ret = zxdh_vf_1588_call_np_interface(en_dev);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_vf_1588_call_np_interface failed!!!\n");
            goto err_ret;
        }
    }

    reg->num = 0;
    if (unlikely(copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size)))
    {
        goto err_ret;
    }

    return ret;

err_ret:
    return -1;
}

int32_t zxdh_en_set_synce_recovery_port(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg)
{
    uint32_t reg_size = sizeof(struct zxdh_en_reg);
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;

    if (reg->num != 1)
    {
        LOG_ERR_DEV(en_dev->parent, "Transmit failed[len = %d]!", reg->num);
       return -1;
    }

    if (en_dev->phy_port == INVALID_PHY_PORT)
    {
        LOG_ERR_DEV(en_dev->parent, "phyport is invalid!");
        return -EOPNOTSUPP;
    }

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }
    msg->payload.hdr_to_agt.op_code = AGENT_MAC_RECOVERY_CLK_SET;
    msg->payload.hdr_to_agt.phyport = en_dev->phy_port;
    msg->payload.synce_clk_recovery_port.clk_speed = reg->data[0];
    LOG_INFO_DEV(en_dev->parent, "phyport = %u, clk_speed = %u\n", msg->payload.hdr_to_agt.phyport, \
            msg->payload.synce_clk_recovery_port.clk_speed);
    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_MAC, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_en_set_synce_recovery_port failed, err: %d\n", err);
        goto free_msg;
    }

    reg->num = 0;
    err = copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size);

free_msg:
    kfree(msg);
    return err;
}

int32_t zxdh_en_get_synce_clk_stats(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg)
{
    uint32_t reg_size = sizeof(struct zxdh_en_reg);
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;

    if (reg->num != 1)
    {
        LOG_ERR_DEV(en_dev->parent, "Transmit failed[len = %d]!", reg->num);
        return -1;
    }

    if (en_dev->phy_port == INVALID_PHY_PORT)
    {
        LOG_ERR_DEV(en_dev->parent, "phyport is invalid!");
        return -EOPNOTSUPP;
    }

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }
    msg->payload.hdr_to_agt.op_code = AGENT_MAC_SYNCE_CLK_STATS_GET;
    msg->payload.hdr_to_agt.phyport = en_dev->phy_port;
    LOG_INFO_DEV(en_dev->parent, "phyport = %u\n", msg->payload.hdr_to_agt.phyport);
    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_MAC, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_en_get_synce_clk_stats failed, err: %d\n", err);
        goto free_msg;
    }

    reg->num = 1;
    reg->data[0] = msg->reps.synce_clk_recovery_port.clk_stats;
    err = copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size);
    LOG_INFO_DEV(en_dev->parent, "num = %u, clk_stats: 0x%x\n", reg->num, reg->data[0]);

free_msg:
    kfree(msg);
    return err;
}

int32_t zxdh_en_set_spm_port_tstamp_enable(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg)
{
    uint32_t reg_size = sizeof(struct zxdh_en_reg);
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    int32_t ret = 0;
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;

    if (reg->num != 2)
    {
        LOG_ERR_DEV(en_dev->parent, "Transmit failed[len = %d]!", reg->num);
        return -1;
    }

    if (en_dev->phy_port == INVALID_PHY_PORT)
    {
        LOG_ERR_DEV(en_dev->parent, "phyport is invalid!");
        return -EOPNOTSUPP;
    }

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }
    msg->payload.hdr_to_agt.op_code = AGENT_MAC_PORT_TSTAMP_ENABLE_SET;
    msg->payload.hdr_to_agt.phyport = en_dev->phy_port; // 0~9
    msg->payload.mac_tstamp_msg.tx_enable = reg->data[0];
    msg->payload.mac_tstamp_msg.rx_enable = reg->data[1];
    LOG_INFO_DEV(en_dev->parent, "phyport = %u, tx_enable: %u, rx_enable: %u\n", msg->payload.hdr_to_agt.phyport, \
                msg->payload.mac_tstamp_msg.tx_enable, msg->payload.mac_tstamp_msg.rx_enable);
    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_MAC, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_en_set_spm_port_tstamp_enable failed, err: %d\n", err);
        goto free_msg;
    }

    reg->num = 0;
    ret = copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size);

free_msg:
    kfree(msg);
    return ret;
}

int32_t zxdh_en_get_spm_port_tstamp_enable(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg)
{
    uint32_t reg_size = sizeof(struct zxdh_en_reg);
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    int32_t ret = 0;
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;

    if (en_dev->phy_port == INVALID_PHY_PORT)
    {
        LOG_ERR_DEV(en_dev->parent, "phyport is invalid!");
        return -EOPNOTSUPP;
    }

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }
    msg->payload.hdr_to_agt.op_code = AGENT_MAC_PORT_TSTAMP_ENABLE_GET;
    msg->payload.hdr_to_agt.phyport = en_dev->phy_port; // 0~9
    LOG_INFO_DEV(en_dev->parent, "phyport = %u\n", msg->payload.hdr_to_agt.phyport);
    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_MAC, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_en_get_spm_port_tstamp_enable failed, err: %d\n", err);
        goto free_msg;
    }

    reg->num = 2;
    reg->data[0] = msg->reps.mac_tstamp_msg.tx_enable;
    reg->data[1] = msg->reps.mac_tstamp_msg.rx_enable;
    LOG_INFO_DEV(en_dev->parent, "tx_enable: %u, rx_enable: %u\n",
                msg->reps.mac_tstamp_msg.tx_enable, msg->reps.mac_tstamp_msg.rx_enable);

    ret = copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size);

free_msg:
    kfree(msg);
    return ret;
}

int32_t zxdh_en_set_spm_port_tstamp_mode(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg)
{
    uint32_t reg_size = sizeof(struct zxdh_en_reg);
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    int32_t ret = 0;
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;

    if (en_dev->phy_port == INVALID_PHY_PORT)
    {
        LOG_ERR_DEV(en_dev->parent, "phyport is invalid!");
        return -EOPNOTSUPP;
    }

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    if (reg->num != 2)
    {
        LOG_ERR_DEV(en_dev->parent, "Transmit failed[len = %d]!", reg->num);
        goto err_ret;
    }

    msg->payload.hdr_to_agt.op_code = AGENT_MAC_PORT_TSTAMP_MODE_SET;
    msg->payload.hdr_to_agt.phyport = en_dev->phy_port;// 0~9
    msg->payload.mac_tstamp_msg.tx_mode = reg->data[0];
    msg->payload.mac_tstamp_msg.rx_mode = reg->data[1];
    LOG_INFO_DEV(en_dev->parent, "phyport = %u, tx_mode: %u, rx_mode: %u\n", msg->payload.hdr_to_agt.phyport, msg->payload.mac_tstamp_msg.tx_mode, msg->payload.mac_tstamp_msg.rx_mode);
    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_MAC, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_en_set_spm_port_tstamp_mode failed, err: %d\n", err);
        kfree(msg);
        return err;
    }

    reg->num = 0;
    if (unlikely(copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size)))
    {
        goto err_ret;
    }
    kfree(msg);
    return ret;

err_ret:
    kfree(msg);
    return -1;
}

int32_t zxdh_en_get_spm_port_tstamp_mode(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg)
{
    uint32_t reg_size = sizeof(struct zxdh_en_reg);
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;

    if (en_dev->phy_port == INVALID_PHY_PORT)
    {
        LOG_ERR_DEV(en_dev->parent, "phyport is invalid!");
        return -EOPNOTSUPP;
    }

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    msg->payload.hdr_to_agt.op_code = AGENT_MAC_PORT_TSTAMP_MODE_GET;
    msg->payload.hdr_to_agt.phyport = en_dev->phy_port;// 0~9
    LOG_INFO_DEV(en_dev->parent, "phyport = %u\n", msg->payload.hdr_to_agt.phyport);
    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_MAC, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_en_get_spm_port_tstamp_mode failed, err: %d\n", err);
        kfree(msg);
        return err;
    }

    reg->num = 2;
    reg->data[0] = msg->reps.mac_tstamp_msg.tx_mode;
    reg->data[1] = msg->reps.mac_tstamp_msg.rx_mode;
    LOG_INFO_DEV(en_dev->parent, "tx_mode: %u, rx_mode: %u\n", msg->reps.mac_tstamp_msg.tx_mode, msg->reps.mac_tstamp_msg.rx_mode);

    if (unlikely(copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size)))
    {
        goto err_ret;
    }

    kfree(msg);
    return 0;

err_ret:
    kfree(msg);
    return -1;
}

/* 配置时延测量功能是否打开, 维测功能 */
int32_t zxdh_en_set_delay_statistics_enable(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg)
{
    uint32_t reg_size = sizeof(struct zxdh_en_reg);
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    int32_t ret = 0;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;

    if (reg->num != 1)
    {
        LOG_ERR_DEV(en_dev->parent, "Transmit failed[len = %d]!", reg->num);
        goto err_ret;
    }

    en_dev->delay_statistics_enable = reg->data[0];
    LOG_INFO_DEV(en_dev->parent, "en_dev->delay_statistics_enable = %u\n", en_dev->delay_statistics_enable);

    reg->num = 0;
    if (unlikely(copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size)))
    {
        goto err_ret;
    }

    return ret;

err_ret:
    return -1;
}

int32_t zxdh_en_get_delay_statistics_value(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg)
{
    uint32_t reg_size = sizeof(struct zxdh_en_reg);
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    int32_t ret = 0;
    union zxdh_msg *msg = NULL;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;

    if (en_dev->phy_port == INVALID_PHY_PORT)
    {
        LOG_ERR_DEV(en_dev->parent, "phyport is invalid!");
        return -EOPNOTSUPP;
    }

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    msg->payload.hdr_to_agt.op_code = AGENT_MAC_PORT_DELAY_VALUE_GET;
    msg->payload.hdr_to_agt.phyport = en_dev->phy_port;// 0~9
    LOG_INFO_DEV(en_dev->parent, "phyport = %u\n", msg->payload.hdr_to_agt.phyport);
    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_MAC, msg, msg, &para);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_en_get_delay_statistics_value failed, ret: %d\n", ret);
        kfree(msg);
        return ret;
    }

    reg->num = 4;
    reg->data[0] = (uint32_t)(msg->reps.delay_statistics_val.min_delay & 0xffffffff);
    reg->data[1] = (uint32_t)((msg->reps.delay_statistics_val.min_delay >> 32) & 0xffffffff);
    reg->data[2] = (uint32_t)(msg->reps.delay_statistics_val.max_delay & 0xffffffff);
    reg->data[3] = (uint32_t)((msg->reps.delay_statistics_val.max_delay >> 32) & 0xffffffff);
    LOG_INFO_DEV(en_dev->parent, "delay val: min_delay: %llu, max_delay: %llu\n", msg->reps.delay_statistics_val.min_delay, \
        msg->reps.delay_statistics_val.max_delay);
    if (unlikely(copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size)))
    {
        goto err_ret;
    }
    kfree(msg);
    return 0;

err_ret:
    kfree(msg);
    return -1;
}

int32_t zxdh_en_clear_delay_statistics_value(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg)
{
    uint32_t reg_size = sizeof(struct zxdh_en_reg);
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    int32_t ret = 0;
    union zxdh_msg *msg = NULL;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;

    if (en_dev->phy_port == INVALID_PHY_PORT)
    {
        LOG_ERR_DEV(en_dev->parent, "phyport is invalid!");
        return -EOPNOTSUPP;
    }

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    msg->payload.hdr_to_agt.op_code = AGENT_MAC_PORT_DELAY_VALUE_CLR;
    msg->payload.hdr_to_agt.phyport = en_dev->phy_port;// 0~9
    LOG_INFO_DEV(en_dev->parent, "phyport = %u\n", msg->payload.hdr_to_agt.phyport);
    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_MAC, msg, msg, &para);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_en_clear_delay_statistics_value failed, ret: %d\n", ret);
        kfree(msg);
        return ret;
    }

    reg->num = 0;
    if (unlikely(copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size)))
    {
        kfree(msg);
        goto err_ret;
    }

    kfree(msg);
    return 0;

err_ret:
    return -1;
}

int32_t zxdh_en_set_local_pps_interrupt_enable(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg)
{
#ifdef  PTP_DRIVER_INTERFACE_EN
    uint32_t enable;
    uint32_t support;
    uint32_t reg_size = sizeof(struct zxdh_en_reg);
#endif
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    int32_t ret = 0;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;

    if (reg->num != 1)
    {
        LOG_ERR_DEV(en_dev->parent, "Transmit failed[len = %d]!", reg->num);
        goto err_ret;
    }

#ifdef  PTP_DRIVER_INTERFACE_EN
    ret = zxdh_get_pps_interrupt_support(en_dev, &support);
    CHECK_UNEQUAL_ERR(ret, 0, -EFAULT, "get pps interrupt support failed!!\n");
    enable = reg->data[0];
    LOG_INFO_DEV(en_dev->parent, "enable = %u\n", enable);
    // not support
    if(support != 1)
    {
        reg->num = 1;
        reg->data[0] = 1;// notify user not support pps interrupt
        if (unlikely(copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size)))
        {
            LOG_ERR_DEV(en_dev->parent, "copy_to_user failed!!!\n");
            goto err_ret;
        }
        goto err_ret;
    }

    ret = zxdh_set_local_pps_interrupt_enable(en_dev, enable);
    CHECK_UNEQUAL_ERR(ret, 0, -EFAULT, "set local pps interrupt failed!!\n");

    reg->num = 1;
    reg->data[0] = 0; // notify user support pps interrupt
    if (unlikely(copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size)))
    {
        LOG_ERR_DEV(en_dev->parent, "copy_to_user failed!!!\n");
        goto err_ret;
    }
#endif /* PTP_DRIVER_INTERFACE_EN */
    return ret;

err_ret:
    return -1;
}

int32_t zxdh_en_set_ext_pps_interrupt_enable(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg)
{
#ifdef  PTP_DRIVER_INTERFACE_EN
    uint32_t pps_type;
    uint32_t enable;
    uint32_t support;
    uint32_t reg_size = sizeof(struct zxdh_en_reg);
#endif
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    int32_t ret = 0;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;

    if (reg->num != 2)
    {
        LOG_ERR_DEV(en_dev->parent, "Transmit failed[len = %d]!", reg->num);
        goto err_ret;
    }

#ifdef  PTP_DRIVER_INTERFACE_EN
    ret = zxdh_get_pps_interrupt_support(en_dev, &support);
    CHECK_UNEQUAL_ERR(ret, 0, -EFAULT, "get pps interrupt support failed!!\n");
    pps_type = reg->data[0];
    enable = reg->data[1];
    LOG_INFO_DEV(en_dev->parent, "pps_type = %u, enable = %u\n", pps_type, enable);
    // not support
    if(support != 1)
    {
        reg->num = 1;
        reg->data[0] = 1;// notify user not support pps interrupt
        if (unlikely(copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size)))
        {
            LOG_ERR_DEV(en_dev->parent, "copy_to_user failed!!!\n");
            goto err_ret;
        }
        goto err_ret;
    }

    ret = zxdh_set_ext_pps_interrupt_enable(en_dev, pps_type, enable);
    CHECK_UNEQUAL_ERR(ret, 0, -EFAULT, "set ext pps interrupt enable failed!!\n");

    reg->num = 1;
    reg->data[0] = 0; // notify user support pps interrupt
    if (unlikely(copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size)))
    {
        LOG_ERR_DEV(en_dev->parent, "copy_to_user failed!!!\n");
        goto err_ret;
    }
#endif /* PTP_DRIVER_INTERFACE_EN */
    return ret;

err_ret:
    return -1;
}

int32_t zxdh_en_set_pd_sel_shift(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg)
{
    uint32_t pd_index;
    uint32_t pd_sel;
    uint32_t shift;
    uint32_t reg_size = sizeof(struct zxdh_en_reg);
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    int32_t ret = 0;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;

    if (reg->num != 3)
    {
        LOG_ERR_DEV(en_dev->parent, "Transmit failed[len = %d]!", reg->num);
        goto err_ret;
    }

    pd_index = reg->data[0];
    pd_sel = reg->data[1];
    shift = reg->data[2];
    LOG_INFO_DEV(en_dev->parent, "pd_index = %u, pd_sel = %u, shift = %u\n", pd_index, pd_sel, shift);

#ifdef  PTP_DRIVER_INTERFACE_EN
    ret = zxdh_set_pd_sel_shift(en_dev, pd_index, pd_sel, shift);
    CHECK_UNEQUAL_ERR(ret, 0, -EFAULT, "set pd sel shift failed!!\n");
#endif /* PTP_DRIVER_INTERFACE_EN */
    reg->num = 1;
    reg->data[0] = 0; // notify user support pps interrupt
    if (unlikely(copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size)))
    {
        LOG_ERR_DEV(en_dev->parent, "copy_to_user failed!!!\n");
        goto err_ret;
    }

    return ret;

err_ret:
    return -1;
}

int32_t zxdh_en_get_ptp_clock_index(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg)
{
    uint32_t ptp_clock_index;
    uint32_t reg_size = sizeof(struct zxdh_en_reg);
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    int32_t ret = 0;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;

    ptp_clock_index = 0; /* 3.10 内核未启用 PTP 时初始化为 0 */
#ifdef  PTP_DRIVER_INTERFACE_EN
    ret = zxdh_get_ptp_clock_index(en_dev, &ptp_clock_index);
    CHECK_UNEQUAL_ERR(ret, 0, -EFAULT, "get ptp clock index failed!!\n");
#endif /* PTP_DRIVER_INTERFACE_EN */

    reg->num = 1;
    reg->data[0] = ptp_clock_index;
    if (unlikely(copy_to_user(ifr->ifr_ifru.ifru_data, reg, reg_size)))
    {
        LOG_ERR_DEV(en_dev->parent, "copy_to_user failed!!!\n");
        goto err_ret;
    }

    return ret;

err_ret:
    return -1;
}

struct zxdh_en_ptp_ioctl_table ioctl_ptp_table[] =
{
    {PTP_SET_CLOCK_NO,               zxdh_en_set_clock_no},
    {PTP_ENABLE_PTP_ENCRYPTED_MSG,   zxdh_en_enable_ptp_encrypted_msg},
    {PTP_SET_INTR_CAPTURE_TIMER,     zxdh_en_set_intr_capture_timer},
    {PTP_SET_PP1S_SELECTION,         zxdh_en_set_pps_selection},
    {PTP_SET_PHASE_DETECTION,        zxdh_en_set_phase_detection},
    {PTP_GET_PD_VALUE,               zxdh_en_get_pd_value},
    {PTP_SET_L2PTP_PORT,             zxdh_en_set_l2_ptp_port},
    {PTP_SET_PTP_EC_ENABLE,          zxdh_en_set_ptp_tc_enable},
    {PTP_SET_SYNCE_CLK_PORT,         zxdh_en_set_synce_recovery_port},
    {PTP_GET_SYNCE_CLK_STATS,        zxdh_en_get_synce_clk_stats},
    {PTP_SET_SPM_PORT_TSTAMP_ENABLE,      zxdh_en_set_spm_port_tstamp_enable},
    {PTP_GET_SPM_PORT_TSTAMP_ENABLE,      zxdh_en_get_spm_port_tstamp_enable},
    {PTP_SET_SPM_PORT_TSTAMP_MODE,        zxdh_en_set_spm_port_tstamp_mode},
    {PTP_GET_SPM_PORT_TSTAMP_MODE,        zxdh_en_get_spm_port_tstamp_mode},
    {PTP_SET_DELAY_STATISTICS_ENABLE,     zxdh_en_set_delay_statistics_enable},
    {PTP_GET_DELAY_STATISTICS_VALUE,      zxdh_en_get_delay_statistics_value},
    {PTP_CLR_DELAY_STATISTICS_VALUE,      zxdh_en_clear_delay_statistics_value},
    {PTP_SET_LOCAL_PPS_INTERRUPT_ENABLE,  zxdh_en_set_local_pps_interrupt_enable},
    {PTP_SET_EXT_PPS_INTERRUPT_ENABLE,    zxdh_en_set_ext_pps_interrupt_enable},
    {PTP_SET_PD_SEL_SHIFT,                zxdh_en_set_pd_sel_shift},
    {PTP_GET_PTP_CLOCK_INDEX,             zxdh_en_get_ptp_clock_index}
};

int32_t ptp_table_match_func(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg)
{
    uint32_t i = 0;
    uint32_t ret = 0;
    uint32_t table_size = sizeof(ioctl_ptp_table) / sizeof(struct zxdh_en_ioctl_table);
    for(i = 0; i < table_size; i++)
    {
        if((reg->offset == ioctl_ptp_table[i].cmd) && (ioctl_ptp_table[i].func != NULL))
        {
            ret = ioctl_ptp_table[i].func(netdev, ifr, reg);
            break;
        }
    }
    return ret;
}

int32_t zxdh_en_ptp_func(struct net_device *netdev, struct ifreq *ifr)
{
    struct zxdh_en_reg *reg = NULL;
    uint32_t reg_size = sizeof(struct zxdh_en_reg);

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    reg = kzalloc(reg_size, GFP_KERNEL);
    CHECK_EQUAL_ERR(reg, NULL, -EADDRNOTAVAIL, "reg is null!\n");

    if (unlikely(copy_from_user(reg, ifr->ifr_ifru.ifru_data, reg_size)))
    {
        LOG_ERR("copy_from_user failed!\n");
        goto err_ret;
    }

    if(-1 == ptp_table_match_func(netdev, ifr, reg))
    {
        LOG_ERR("ptp_table_match_func failed!\n");
        goto err_ret;
    }

    kfree(reg);
    return 0;

err_ret:
    kfree(reg);
    return -1;
}

int32_t zxdh_en_pps_func(struct net_device *netdev, struct ifreq *ifr)
{
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    struct dh_core_dev *dh_dev = NULL;
    struct zxdh_pf_device *pf_dev = NULL;
    struct dh_eq_table *table = NULL;
    struct dh_pf_eq_table *table_priv = NULL;
    uint64_t virtaddr = 0x0;
    struct dh_irq *expps = NULL;
    struct dh_irq *lopps = NULL;
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

#ifdef  PTP_DRIVER_INTERFACE_EN
    int32_t ret = 0;
#endif /* PTP_DRIVER_INTERFACE_EN */

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");
    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");

    en_priv = netdev_priv(netdev);
    en_dev = &en_priv->edev;
    dh_dev = en_dev->parent->parent;
    pf_dev = dh_core_priv(dh_dev);

    table = &dh_dev->eq_table;
    table_priv = table->priv;

    LOG_ERR_DEV(dh_dev, "pf_dev->pci_ioremap_addr[0]: 0x%llx\n", pf_dev->pci_ioremap_addr[0]);

    virtaddr = pf_dev->pci_ioremap_addr[0] + ZXDH_BAR_MSG_OFFSET;
#ifdef PTP_DRIVER_INTERFACE_EN
    tod_device_set_bar_virtual_addr(virtaddr, pf_dev->pcie_id);
#endif

    expps = table_priv->async_irq_tbl[3];
    lopps = table_priv->async_irq_tbl[4];

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(dh_dev, "failed to kzalloc\n");
        return -ENOMEM;
    }

    msg->payload.msg_pps.pcieid = pf_dev->pcie_id;
    msg->payload.msg_pps.extern_pps_vector = expps->index;
    msg->payload.msg_pps.local_pps_vector = lopps->index;
    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_PPS, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_en_pps_func failed, err: %d\n", err);
        goto free_msg;
    }

#ifdef  PTP_DRIVER_INTERFACE_EN
    ret = zxdh_set_pps_interrupt_support(en_dev, msg->reps.msg_pps.pps_intr_support);
    if (unlikely(ret != 0))
    {
        LOG_ERR_DEV(dh_dev, "set pps interrupt support failed!!\n");
        err = -EFAULT;
        goto free_msg;
    }
#endif /* PTP_DRIVER_INTERFACE_EN */

free_msg:
    kfree(msg);

    return err;
}

struct zxdh_en_ioctl_table ioctl_table[] =
{
    {SIOCGMIIREG,                               zxdh_read_reg_cmd},
    {SIOCSMIIREG,                               zxdh_write_reg_cmd},
    {SIOCDEVPRIVATE_VQ_INFO,                    zxdh_get_vring_info},
    {SIOCDEVPRIVATE_SEND_FILE_PKT,              zxdh_send_file_pkt},
    {SIOCDEVPRIVATE_PTP_FUNC,                   zxdh_en_ptp_func},
    {SIOCDEVPRIVATE_PPS_FUNC,                   zxdh_en_pps_func},
#ifdef CONFIG_DINGHAI_TSN
    {SIOCDEVPRIVATE_TSN_FUNC,                   zxdh_en_tsn_func},
#endif
    {SIOCDEVPRIVATE_DH_TOOLS,                   zxdh_tools_ioctl_dispatcher},
};

int32_t ioctl_table_match_func(struct net_device *netdev, struct ifreq *ifr, int32_t cmd,
                               struct zxdh_en_ioctl_table *func_table, uint32_t table_size)
{
    int32_t ret = -EOPNOTSUPP;
    uint32_t i = 0;

    CHECK_EQUAL_ERR(ifr, NULL, -EADDRNOTAVAIL, "ifr is null!\n");
    for (i = 0; i < table_size; i++)
    {
        if ((func_table[i].cmd == cmd) && (func_table[i].func != NULL))
        {
            ret = func_table[i].func(netdev, ifr);
            break;
        }
    }

    return ret;
}

int32_t zxdh_en_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd)
{
    uint32_t table_size = sizeof(ioctl_table) / sizeof(struct zxdh_en_ioctl_table);
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    return ioctl_table_match_func(netdev, ifr, cmd, ioctl_table, table_size);
}

#if defined(ZXDH_ADAPT_REDHAT_9_2) || defined(USE_PRIV_IOCTL) || defined(ZXDH_ADAPT_REDHAT_9_1)
int32_t zxdh_en_private_ioctl(struct net_device *netdev, struct ifreq *ifr, void *data, int cmd)
{
    uint32_t table_size = sizeof(ioctl_table) / sizeof(struct zxdh_en_ioctl_table);
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    return ioctl_table_match_func(netdev, ifr, cmd, ioctl_table, table_size);
}
#endif