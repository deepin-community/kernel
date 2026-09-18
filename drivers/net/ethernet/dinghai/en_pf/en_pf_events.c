#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/dinghai/driver.h>
#include <linux/notifier.h>
#include <linux/dinghai/events.h>
#include <linux/dinghai/dh_cmd.h>
#include <linux/workqueue.h>
#include <linux/kernel.h>
#include "en_pf_events.h"
#include "../en_pf.h"
#include "../msg_common.h"
#include "en_pf_eq.h"
#include "../en_aux.h"

static int32_t riscv2pf_notifier(struct notifier_block *, unsigned long, void *);
static int32_t riscv_ready_notifier(struct notifier_block *, unsigned long, void *);
static int32_t vf2pf_notifier(struct notifier_block *, unsigned long, void *);
static int32_t riscv_ext_pps_notifier(struct notifier_block *, unsigned long, void *);
static int32_t riscv_local_pps_notifier(struct notifier_block *nb, unsigned long type, void *data);

#ifdef PTP_DRIVER_INTERFACE_EN
extern irqreturn_t msix_extern_pps_irq_from_risc_handler(struct zxdh_pf_device *dev);
extern irqreturn_t msix_local_pps_irq_from_risc_handler(struct zxdh_pf_device *dev);
#endif

static struct dh_nb pf_events[] = {
    {.nb.notifier_call = riscv_ready_notifier, .event_type = DH_EVENT_TYPE_RISCV_READY},
    {.nb.notifier_call = vf2pf_notifier, .event_type = DH_EVENT_TYPE_NOTIFY_VF_TO_PF},
    {.nb.notifier_call = riscv_ext_pps_notifier, .event_type = DH_EVENT_TYPE_NOTIFY_RISC_EXT_PPS},
    {.nb.notifier_call = riscv_local_pps_notifier, .event_type = DH_EVENT_TYPE_NOTIFY_RISC_LOCAL_PPS},
    {.nb.notifier_call = riscv2pf_notifier, .event_type = DH_EVENT_TYPE_NOTIFY_ANY},
};

static int32_t riscv2pf_notifier(struct notifier_block *nb, unsigned long type, void *data)
{
    struct dh_event_nb *event_nb = dh_nb_cof(nb, struct dh_event_nb, nb);
    struct dh_core_dev *dh_dev = (struct dh_core_dev *)event_nb->ctx;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    zxdh_events_work_enqueue(dh_dev, &pf_dev->riscv2pf_msg_proc_work);

    return NOTIFY_OK;
}

static int32_t riscv_ready_notifier(struct notifier_block *nb, unsigned long type, void *data)
{
    struct dh_event_nb *event_nb = dh_nb_cof(nb, struct dh_event_nb, nb);
    struct dh_core_dev *dh_dev = (struct dh_core_dev *)event_nb->ctx;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    uint64_t virt_addr = pf_dev->pci_ioremap_addr[0] + ZXDH_BAR_MSG_OFFSET;
    zxdh_bar_reset_valid(virt_addr);
    zxdh_events_work_enqueue(dh_dev, &pf_dev->riscv_ready_work);

    return NOTIFY_OK;
}

static int32_t vf2pf_notifier(struct notifier_block *nb, unsigned long type, void *data)
{
    struct dh_event_nb *event_nb = dh_nb_cof(nb, struct dh_event_nb, nb);
    struct dh_core_dev *dh_dev = (struct dh_core_dev *)event_nb->ctx;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    LOG_DEBUG_DEV(dh_dev, "is called\n");

    zxdh_events_work_enqueue(dh_dev, &pf_dev->vf2pf_msg_proc_work);

    return NOTIFY_OK;
}

static int32_t riscv_ext_pps_notifier(struct notifier_block *nb, unsigned long type, void *data)
{
    struct dh_event_nb *event_nb = dh_nb_cof(nb, struct dh_event_nb, nb);
    struct dh_core_dev *dh_dev = (struct dh_core_dev *)event_nb->ctx;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    LOG_DEBUG_DEV(dh_dev, "is called\n");

    zxdh_events_work_enqueue(dh_dev, &pf_dev->riscv_ext_pps_work);

    return NOTIFY_OK;
}

static int32_t riscv_local_pps_notifier(struct notifier_block *nb, unsigned long type, void *data)
{
    struct dh_event_nb *event_nb = dh_nb_cof(nb, struct dh_event_nb, nb);
    struct dh_core_dev *dh_dev = (struct dh_core_dev *)event_nb->ctx;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    LOG_DEBUG_DEV(dh_dev, "is called\n");

    zxdh_events_work_enqueue(dh_dev, &pf_dev->riscv_local_pps_work);

    return NOTIFY_OK;
}

extern int32_t zxdh_plug_aux_dev(struct dh_core_dev *dh_dev);

static void riscv2pf_msg_proc_work_handler(struct work_struct *_work)
{
    struct zxdh_pf_device *pf_dev = container_of(_work, struct zxdh_pf_device, riscv2pf_msg_proc_work);

    uint16_t src = MSG_CHAN_END_RISC;
    uint16_t dst = MSG_CHAN_END_PF;
    uint64_t virt_addr = pf_dev->pci_ioremap_addr[0] + ZXDH_BAR_MSG_OFFSET;

    ZXDH_AUX_COMP_FLAG_CHECK(pf_dev);

    zxdh_bar_irq_recv(src, dst, virt_addr, NULL);
}

static void vf2pf_msg_proc_work_handler(struct work_struct *_work)
{
    struct zxdh_pf_device *pf_dev = container_of(_work, struct zxdh_pf_device, vf2pf_msg_proc_work);

    uint16_t src = MSG_CHAN_END_VF;
    uint16_t dst = MSG_CHAN_END_PF;
    uint64_t virt_addr = pf_dev->pci_ioremap_addr[0] + ZXDH_BAR_MSG_OFFSET + ZXDH_BAR_PFVF_MSG_OFFSET;

    ZXDH_AUX_COMP_FLAG_CHECK(pf_dev);

    zxdh_bar_irq_recv(src, dst, virt_addr, pf_dev);
}

static void riscv_ready_work_handler(struct work_struct *_work)
{
    struct zxdh_pf_device *pf_dev = container_of(_work, struct zxdh_pf_device, riscv_ready_work);
    struct dh_core_dev * dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);

    ZXDH_AUX_COMP_FLAG_CHECK(pf_dev);

    zxdh_plug_aux_dev(dh_dev);
}


int32_t findFirstSetBit(uint8_t link_up_val) {
    uint8_t i = 0;
    for ( ; i < 8; i++)
    {
        if (link_up_val & (1 << i))
        {
            return i;  // 返回第一个设置位的位置
        }
    }
    return -1;  // 没有找到
}

int32_t get_link_up_phyport(uint8_t link_up_val,struct zxdh_pf_device *pf_dev, uint8_t *phyport_val)
{
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);
    int16_t first_link_up_idx = -1;
    uint8_t port_num = pf_dev->port_resource.pannel_num;
    struct zxdh_pannle_port *port;
    int32_t idx = 0;

    first_link_up_idx = findFirstSetBit(link_up_val);
    if(first_link_up_idx<0)
    {
        return -1;
    }
    //基于first_link_up_idx找到phyport值TODO:待完善
    for (idx = 0; idx < port_num; idx++)
    {
        port = &pf_dev->port_resource.port[idx];
        if (port->link_check_bit == first_link_up_idx)
        {
            *phyport_val = port->phyport;
            LOG_DEBUG_DEV(dh_dev, "first link_up idx %d <-> phyport 0x%x\n", first_link_up_idx, port->phyport);
            return 0;
        }
    }

    return -1;
}

static void link_info_irq_update_vf_bond_pf_work_handler(struct work_struct *_work)
{
    struct zxdh_pf_device *pf_dev = container_of(_work, struct zxdh_pf_device, link_info_irq_update_vf_bond_pf_work);
    struct dh_core_dev * dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
    struct zxdh_vf_item *vf_item = NULL;
    int32_t err = 0;
    uint16_t vf_idx = 0;
    struct pci_dev *pdev = dh_dev->pdev;
    uint16_t num_vfs = 0;
    uint8_t link_up_val = 0;
    uint8_t phyport_val = 0;
    uint8_t link_info = 0;
    uint16_t func_no = 0;
    uint16_t pf_no = FIND_PF_ID(pf_dev->pcie_id);
    union zxdh_msg *msg = NULL;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    ZXDH_AUX_COMP_FLAG_CHECK(pf_dev);

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(dh_dev, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return;
    }

    zxdh_pf_get_link_info_from_vqm(dh_dev, &link_up_val);
    LOG_DEBUG_DEV(dh_dev, "[pf_level] bond_pf pcie_id:0x%x read link_up_val, val: 0x%x\n", pf_dev->pcie_id, link_up_val);
    pf_dev->link_up = (link_up_val == 0) ? FALSE : TRUE;

    if(pf_dev->link_up)
    {
        if(get_link_up_phyport(link_up_val, pf_dev, &phyport_val)<0)
        {
            LOG_ERR_DEV(dh_dev, "failed to get link up phyport\n");
            kfree(msg);
	    return;
        }
        link_up_val = 1;
    }

    link_info = (phyport_val & 0x0F) << 4 | (link_up_val & 0x0F);
    msg->payload.hdr_to_agt.op_code = AGENT_DEV_STATUS_NOTIFY;
    msg->payload.hdr_to_agt.pcie_id = pf_dev->pcie_id;
    num_vfs = pci_num_vf(pdev);
    for (vf_idx = 0; vf_idx < num_vfs; vf_idx++)
    {
        vf_item = zxdh_pf_get_vf_item(dh_dev, vf_idx);
        if(vf_item->link_forced == FALSE && vf_item->is_probed)
        {
            func_no = GET_FUNC_NO(pf_no, vf_idx);
            msg->payload.pcie_msix_msg.func_no[msg->payload.pcie_msix_msg.num++] = func_no;
            zxdh_pf_set_vf_link_info(dh_dev, vf_idx, link_info);
            LOG_DEBUG_DEV(dh_dev, "[pf_level] bond_pf pcie_id:0x%x write phyport[0x%x] and link_up[%d] to VF[%d] [0x%x]\n",
                        pf_dev->pcie_id, phyport_val, link_up_val, vf_idx, link_info);
        }
    }
    LOG_DEBUG_DEV(dh_dev, "vf num:%d\n", msg->payload.pcie_msix_msg.num);
    if(msg->payload.pcie_msix_msg.num > 0)
    {
        err = zxdh_pf_msg_send_cmd(dh_dev, MODULE_MAC, msg, msg, &para);
        if (err != 0)
        {
            LOG_ERR_DEV(dh_dev, "failed to update VF link info\n");
        }
    }

    kfree(msg);
}

static void init_vf_link_info_work_handler(struct work_struct *_work)
{
    struct zxdh_pf_device *pf_dev = container_of(_work, struct zxdh_pf_device, init_vf_link_info_work);
    struct dh_core_dev * dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
    struct zxdh_vf_item *vf_item = NULL;
    int32_t err = 0;
    uint16_t vf_idx = 0;
    struct pci_dev *pdev = dh_dev->pdev;
    uint16_t num_vfs = 0;
    uint8_t link_up_val = 0;
    uint8_t link_info = 0;
    uint16_t func_no = 0;
    uint16_t pf_no = FIND_PF_ID(pf_dev->pcie_id);
    union zxdh_msg *msg = NULL;
    struct zxdh_bar_extra_para para = {0};

    LOG_DEBUG_DEV(dh_dev, "is called\n");
    ZXDH_AUX_COMP_FLAG_CHECK(pf_dev);
    

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(dh_dev, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return;
    }

    zxdh_pf_get_link_info_from_vqm(dh_dev, &link_up_val);
    pf_dev->link_up = (link_up_val == 0) ? FALSE : TRUE;

    /* VGCF 不对应面板口的pflink状态默认为1 */
    if ((zxdh_pf_get_dev_type(dh_dev) == ZXDH_DEV_NE0) || (zxdh_pf_get_dev_type(dh_dev) == ZXDH_DEV_NE1))
    {
        pf_dev->link_up = 1;
        LOG_DEBUG("VGCF NE0/NE1, link_up: %u\n", pf_dev->link_up);
    }

    if(zxdh_pf_is_upf(dh_dev))
    {
        link_info = (pf_dev->phy_port & 0x0F) << 4 | (link_up_val & 0x0F);
        LOG_DEBUG_DEV(dh_dev, "upf update vf link_info: %u\n", link_info);
    }
    else
    {
        if(!pf_dev->is_special_bond && pf_dev->is_hwbond && pf_dev->is_primary_port && pf_dev->is_bond_slave)
        {
            link_info = pf_dev->bond_link_info == 0 ? 0 : 1;
            LOG_INFO_DEV(dh_dev, "vf %d link state is %d (bond_link_info %d)\n", vf_idx, link_info, pf_dev->bond_link_info);
        }
        else
        {
            link_info = pf_dev->link_up ? 1 : 0;
        }
    }

    msg->payload.hdr_to_agt.op_code = AGENT_DEV_STATUS_NOTIFY;
    msg->payload.hdr_to_agt.pcie_id = pf_dev->pcie_id;

    num_vfs = pci_num_vf(pdev);
    for (vf_idx = 0; vf_idx < num_vfs; vf_idx++)
    {
        vf_item = zxdh_pf_get_vf_item(dh_dev, vf_idx);
        if(vf_item->link_forced == FALSE && vf_item->is_probed)
        {
            func_no = GET_FUNC_NO(pf_no, vf_idx);
            msg->payload.pcie_msix_msg.func_no[msg->payload.pcie_msix_msg.num++] = func_no;
            zxdh_pf_set_vf_link_info(dh_dev, vf_idx, link_info);
            LOG_DEBUG_DEV(dh_dev, "pcie_id:0x%x init VF[%d] [0x%x]\n", pf_dev->pcie_id, vf_idx, link_info);
        }
    }
    LOG_DEBUG_DEV(dh_dev, "pcie_id:0x%x vf num:%d\n", pf_dev->pcie_id, msg->payload.pcie_msix_msg.num);
    if(msg->payload.pcie_msix_msg.num > 0)
    {
        err = zxdh_pf_msg_send_cmd(dh_dev, MODULE_MAC, msg, msg, &para);
        if (err != 0)
        {
            LOG_ERR_DEV(dh_dev, "failed to update VF link info\n");
        }
    }

    kfree(msg);
}

static void riscv_extern_pps_handler(struct work_struct *_work)
{
#ifdef PTP_DRIVER_INTERFACE_EN
    struct zxdh_pf_device *pf_dev = container_of(_work, struct zxdh_pf_device, riscv_ext_pps_work);
    msix_extern_pps_irq_from_risc_handler(pf_dev);
#endif
}

static void riscv_local_pps_handler(struct work_struct *_work)
{
#ifdef PTP_DRIVER_INTERFACE_EN
    struct zxdh_pf_device *pf_dev = container_of(_work, struct zxdh_pf_device, riscv_local_pps_work);
    msix_local_pps_irq_from_risc_handler(pf_dev);
#endif
}

uint32_t get_offset(uint32_t ep, uint32_t pf_no, const uint8_t *pf_count)
{
    uint32_t offset = 0;
    uint32_t i = 0;

    for (i = 0; i < ep; ++i)
    {
        offset += pf_count[i] * ZXDH_PF_FLAG_SIZE;
    }

    for (i = 0; i < pf_no; ++i)
    {
        offset += ZXDH_PF_FLAG_SIZE;
    }

    return offset;
}

static void mac_info_pf_work_handler(struct work_struct *_work)
{
    struct zxdh_pf_device *pf_dev = container_of(_work, struct zxdh_pf_device, mac_info_pf_work);
    struct dh_core_dev * dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
    struct zxdh_vf_item *vf_item = NULL;
    struct pci_dev *pdev = dh_dev->pdev;
    uint16_t vf_vport = 0;
    DPP_PF_INFO_T pf_info = {0};
    MAC_VPORT_INFO *unicast_mac_arry = NULL;
    uint8_t read_headr_value = 0;
    uint32_t current_unicast_num = 0;
    int32_t retval = 0;
    uint32_t i = 0;
    uint32_t j = 0;
    int32_t num_vfs = 0;
    uint8_t val = 0;
    uint8_t count = 0;
    uint32_t offset = 0;
    uint8_t pf_count[MAX_EP] = {0};
    uint8_t vf_flag[ZXDH_VF_NUM] = {0};
    uint8_t sum_flag = 0;
    uint8_t ep_id = (pf_dev->pcie_id >> 12) & 0x7;
    uint8_t pf_id = (pf_dev->pcie_id >> 8) & 0x7;

    LOG_INFO_DEV(dh_dev, "mac_info_pf_work_handler is call\n");
    ZXDH_AUX_COMP_FLAG_CHECK(pf_dev);

    if(dh_dev->coredev_type == DH_COREDEV_VF || 0 == pf_dev->pci_ioremap_addr[0])
    {
        return;
    }

    read_headr_value = ioread8((void __iomem *)(pf_dev->pci_ioremap_addr[0] + ZXDH_MAC_FLAG_BAR_OFFSET + ZXDH_MAC_VERSION_OFFSET));
    if (read_headr_value == ZXDH_MAC_VERSION)
    {
        for (i = 0; i < MAX_EP; ++i)
        {
            read_headr_value = ioread8((void __iomem *)(pf_dev->pci_ioremap_addr[0] + ZXDH_MAC_FLAG_BAR_OFFSET + i));
            count = 0;

            while (read_headr_value)
            {
                read_headr_value &= (read_headr_value - 1);
                count++;
            }
            pf_count[i] = count;
        }
        offset = get_offset(ep_id, pf_id, pf_count);
        offset += ZXDH_MAC_FLAG_BAR_OFFSET + ZXDH_MAC_HEADER_BAR_OFFSET;
    }
    else
    {
        offset += ZXDH_MAC_FLAG_BAR_OFFSET + ZXDH_MAC_HEADER_OLD_OFFSET + ep_id * ZXDH_EP_FLAG_SIZE + pf_id * ZXDH_PF_FLAG_SIZE;
    }

    unicast_mac_arry = (MAC_VPORT_INFO *)kzalloc(sizeof(MAC_VPORT_INFO)*UNICAST_MAX_NUM, GFP_KERNEL);
    if (unicast_mac_arry == NULL)
    {
        LOG_ERR_DEV(dh_dev, "kzalloc unicast_mac_arry failed \n");
        return;
    }

    num_vfs = pci_num_vf(pdev);
    if(num_vfs == 0)
    {
        kfree(unicast_mac_arry);
        return;
    }

    for (i=0; i<num_vfs; i++)
    {
        val = ioread8((void __iomem *)(pf_dev->pci_ioremap_addr[0] + offset + i));
        if (val == 1)
        {
            vf_flag[sum_flag] = i;
            iowrite8(0, (void __iomem*)(pf_dev->pci_ioremap_addr[0] + offset + i));
            sum_flag++;
        }
    }

    for (i=0; i<sum_flag; i++)
    {
        vf_item = zxdh_pf_get_vf_item(dh_dev, vf_flag[i]);
        pf_info.slot = pf_dev->slot_id;
        pf_info.vport = pf_dev->vport;
        vf_vport = vf_item->vport;
        retval = dpp_unicast_mac_dump(&pf_info, unicast_mac_arry, &current_unicast_num);
        if (retval != 0)
        {
            kfree(unicast_mac_arry);
            LOG_ERR_DEV(dh_dev, "dpp_unicast_mac_dump failed, retval:%d\n", retval);
            return;
        }

        for (j=0; j< current_unicast_num; j++)
        {
            if (vf_vport == unicast_mac_arry[j].vport)
            {
                retval = dpp_del_mac(&pf_info, unicast_mac_arry[j].addr, unicast_mac_arry[j].sriov_vlan_tpid, unicast_mac_arry[j].sriov_vlan_id);
                if (retval != 0)
                {
                    LOG_ERR_DEV(dh_dev, "dpp_del_mac failed, ret: %d\n", retval);
                    kfree(unicast_mac_arry);
                    return;
                }
                LOG_INFO_DEV(dh_dev, "dpp_del_mac sucess, vport: 0x%x\n", vf_vport);
            }
        }

        vf_item->is_probed = false;
    }

    kfree(unicast_mac_arry);
    return;
}

void zxdh_pf_nh_attach(struct dh_core_dev *dev, struct dh_nb *nb, bool attach)
{
    struct dh_eq_table *eq_table = &dev->eq_table;

    if (attach)
        dh_eq_notifier_register(eq_table, nb);
    else
        dh_eq_notifier_unregister(eq_table, nb);
}

static void rdma_dev_proc_work_handler(struct work_struct *_work)
{
    struct zxdh_pf_device *pf_dev = container_of(_work, struct zxdh_pf_device, rdma_dev_proc_work);
    struct dh_core_dev * dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
    int ret = 0;

    mutex_lock(&dh_dev->lock);
    ret = zxdh_pf_call_aux_events(dh_dev, DH_EVENT_TYPE_RDMA_LOAD);
    mutex_unlock(&dh_dev->lock);
    if (ret != 0) {
        LOG_ERR("%s DH_EVENT_TYPE_RDMA_LOAD failed: %d\n", pci_name(dh_dev->pdev), ret);
    }

    return;
}

static void rdma_dev_event_work_handler(struct work_struct *_work)
{
    struct zxdh_pf_device *pf_dev = container_of(_work, struct zxdh_pf_device, rdma_dev_event_work);
    struct dh_core_dev * dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
    int ret = 0;

    mutex_lock(&dh_dev->lock);
    ret = zxdh_pf_call_aux_events(dh_dev, DH_EVENT_TYPE_RDMA_EVENT_NOTIFY);
    mutex_unlock(&dh_dev->lock);
    if (ret != 0) {
        LOG_ERR("%s DH_EVENT_TYPE_RDMA_EVENT_NOTIFY failed: %d\n", pci_name(dh_dev->pdev), ret);
    }

    return;
}

int32_t dh_pf_events_init(struct dh_core_dev *dev)
{
    struct dh_events *events = NULL;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);
    int32_t i = 0;
    int32_t ret = 0;
    uint32_t evt_num = ARRAY_SIZE(pf_events);

    if (pf_dev->bond_num != 0)
    {
        evt_num -= 1;
    }
    events = kzalloc((sizeof(*events) + evt_num * sizeof(struct dh_event_nb)), GFP_KERNEL);
    if (unlikely(events == NULL))
    {
        LOG_ERR_DEV(dev, "events kzalloc failed: %p\n", events);
        ret = -ENOMEM;
        goto err_events_kzalloc;
    }

    events->evt_num = evt_num;
    events->dev = dev;
    dev->events = events;
    events->wq = create_singlethread_workqueue("dh_pf_events");
    if (!events->wq)
    {
        LOG_ERR_DEV(dev, "events->wq create_singlethread_workqueue failed: %p\n", events->wq);
        ret = -ENOMEM;
        goto err_create_wq;
    }

    INIT_WORK(&pf_dev->riscv_ready_work, riscv_ready_work_handler);
    INIT_WORK(&pf_dev->riscv2pf_msg_proc_work, riscv2pf_msg_proc_work_handler);
    INIT_WORK(&pf_dev->vf2pf_msg_proc_work, vf2pf_msg_proc_work_handler);
    INIT_WORK(&pf_dev->link_info_irq_update_vf_bond_pf_work, link_info_irq_update_vf_bond_pf_work_handler);
    INIT_WORK(&pf_dev->init_vf_link_info_work, init_vf_link_info_work_handler);
    INIT_WORK(&pf_dev->riscv_ext_pps_work, riscv_extern_pps_handler);
    INIT_WORK(&pf_dev->riscv_local_pps_work, riscv_local_pps_handler);
    INIT_WORK(&pf_dev->mac_info_pf_work, mac_info_pf_work_handler);
    INIT_WORK(&pf_dev->rdma_dev_proc_work, rdma_dev_proc_work_handler);
    INIT_WORK(&pf_dev->rdma_dev_event_work, rdma_dev_event_work_handler);

    for (i = 0; i < evt_num; i++)
    {
        events->notifiers[i].nb = pf_events[i];
        events->notifiers[i].ctx = dev;
        dh_eq_notifier_register(&dev->eq_table, &events->notifiers[i].nb);
    }

    return 0;

err_create_wq:
    kfree(events);
err_events_kzalloc:
    return ret;
}

void dh_pf_events_uninit(struct dh_core_dev *dev)
{
    struct dh_events *events = dev->events;
    int32_t i = 0;

    for (i = events->evt_num - 1; i >= 0 ; i--)
    {
        dh_eq_notifier_unregister(&dev->eq_table, &events->notifiers[i].nb);
    }

    zxdh_events_cleanup(dev);
    return;
}


void dh_pf_sriov_cap_cfg_uninit(struct dh_core_dev *dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);
    if (dev->coredev_type == DH_COREDEV_PF && pf_dev->pf_sriov_cap_base != NULL)
    {
        iounmap((void *)pf_dev->pf_sriov_cap_base);
        pf_dev->pf_sriov_cap_base = NULL;
    }
    return;
}
