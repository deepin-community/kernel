#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/err.h>
#include <linux/msi.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/uaccess.h>

#include "bar_msg.h"
#include "function_hotplug.h"
#include "pcie_common.h"

extern int pci_sriov_configure_simple(struct pci_dev *dev, int nr_virtfn);

char bind_type_name[FUNCTION_HP_TYPE_COUNT][FILE_PATH_LEN] = {"unbind", "bind"};

static struct zte_dpu_id_info zte_dpu_id_info[] = {
    /*    name,        device_id, vendor_id,*/
    {"virtio_blk",        0x1001, 0x1af4},
    {"nvme",              0x8053, 0x1cf2},
};

static u32 dpu_bus_list[MAX_EP_NUMS] = {0,};

int remove_pci_dev(struct func_hotplug_req *hp_req)
{
    int ret = PCIE_SUCCESS;
    struct pci_dev *pdev = NULL;

    DH_LOG_DEBUG(MODULE_FUC_HP, "domain=0x%x, bdf=0x%x\n", hp_req->domain, hp_req->bdf);
    pdev = zxdh_get_pci_device(hp_req->domain, hp_req->bdf);
    if (!pdev)
    {
        DH_LOG_DEBUG(MODULE_FUC_HP, "This device has removed\n");
        goto finish;
    }

    pci_sriov_configure_simple(pdev, 0);

    pci_stop_and_remove_bus_device_locked(pdev);

    pdev = zxdh_get_pci_device(hp_req->domain, hp_req->bdf);
    if (pdev)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "remove fail! domain=0x%x, bdf=0x%x\n", hp_req->domain, hp_req->bdf);
        ret = PCIE_FAILED;
    }

finish:
    return ret;
}

int rescan_pci_dev(struct pci_dev *pdev)
{
    int ret = PCIE_SUCCESS;
    struct pci_dev *switch_pdev = NULL;

    if (pdev == NULL)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "%s: hp_device not found\n", __func__);
        return PCIE_FAILED;
    }

    if (pdev->bus->parent)
    {
        switch_pdev = pdev->bus->parent->self;
    }

    if (switch_pdev == NULL)
    {
        DH_LOG_INFO(MODULE_FUC_HP, "%s: switch_pdev not found\n", __func__);
        pci_rescan_bus(pdev->bus);
    }
    else if ((switch_pdev->vendor == SWITCH_VENDOR_ID) && (switch_pdev->device == SWITCH_DEVICE_ID))
    {
        DH_LOG_INFO(MODULE_FUC_HP, "%s: recan from switch \n", __func__);
        DH_LOG_INFO(MODULE_FUC_HP, "%s: devfn:0x%x \n", __func__, switch_pdev->bus->number);
        pci_rescan_bus(switch_pdev->bus);
    }
    else
    {
        DH_LOG_INFO(MODULE_FUC_HP, "%s: parent vendor:0x%x device:0x%x\n", __func__, switch_pdev->vendor, switch_pdev->device);
        DH_LOG_INFO(MODULE_FUC_HP, "%s: rescan from hp deveice\n", __func__);
        pci_rescan_bus(pdev->bus);
    }

    update_pf_state(pdev);

    return ret;
}

int vf_bind_unbind(struct func_hotplug_req *hp_req)
{
    int ret = PCIE_SUCCESS;
    int size = 0;
    struct file *filp = NULL;
    char filename[FILE_PATH_LEN] = {0};
    char bdf[FILE_PATH_LEN] = {0};
    unsigned int scene_code = 0;
    u32 bus_no = 0;
    u32 device_no = 0;
    u32 func_no = 0;

    memset(filename, 0, FILE_PATH_LEN);

    scene_code = SCENE_CODE_OF_FUNC_HP_INFO(hp_req->hotplug_info);
    bus_no = (hp_req->bdf & BDF_NO_BUS_NO_MASK) >> BDF_B_START_BIT;
    device_no = (hp_req->bdf & BDF_NO_DEV_NO_MASK) >> BDF_D_START_BIT;
    func_no = (hp_req->bdf & BDF_NO_FUNC_NO_MASK) >> BDF_F_START_BIT;
    size = snprintf(bdf, FILE_PATH_LEN, "%x:%x:%x.%x", hp_req->domain, bus_no, device_no, func_no);
    if (size >= sizeof(bdf))
    {
        bdf[sizeof(bdf) - 1] = '\0';
    }
    DH_LOG_INFO(MODULE_FUC_HP, "scene_code=%d, bind_type_name=%s, bdf=%s\n", scene_code, bind_type_name[scene_code - FUNCTION_REMOVE], bdf);

    ret = parse_bdf(bdf);
    if (ret != PCIE_SUCCESS)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "%s: parse bdf fail\n", __func__);
        return PCIE_FAILED;
    }

    size = snprintf(filename, FILE_PATH_LEN, "%s/%s/%s/%s", PCI_DEVICES_DIR, bdf, PCI_PHYSFN_DRV_PATH, bind_type_name[scene_code - FUNCTION_REMOVE]);
    if (size >= sizeof(filename))
    {
        filename[sizeof(filename) - 1] = '\0';
    }
    DH_LOG_INFO(MODULE_FUC_HP, "filename=%s\n", filename);

    filp = filp_open(filename, O_WRONLY, 0);
    if (IS_ERR(filp))
    {
        DH_LOG_ERR(MODULE_FUC_HP, "open file error\n");
        return PCIE_FAILED;
    }

#ifdef CGS_V5_693
    kernel_write(filp, (unsigned char *)bdf, sizeof(bdf), filp->f_pos);
#else
    kernel_write(filp, (unsigned char *)bdf, sizeof(bdf), &filp->f_pos);
#endif
    filp_close(filp, NULL);

    return ret;
}

static void print_func_hotplug_req(struct func_hotplug_req *func_hp_req)
{
    DH_LOG_DEBUG(MODULE_FUC_HP, "scene_code=0x%x, function_type=0x%x, ep_id=0x%x, pf_id=0x%x, vf_id=0x%x, domain=0x%x, bdf=0x%x\n",
                 SCENE_CODE_OF_FUNC_HP_INFO(func_hp_req->hotplug_info), FUNC_TYPE_OF_FUNC_HP_INFO(func_hp_req->hotplug_info),
                 EP_ID_OF_FUNC_HP_INFO(func_hp_req->hotplug_info), PF_ID_OF_FUNC_HP_INFO(func_hp_req->hotplug_info),
                 VF_ID_OF_FUNC_HP_INFO(func_hp_req->hotplug_info), func_hp_req->domain, func_hp_req->bdf);
}

static void func_hp_info_decode(struct func_hotplug_req *hp_req, struct func_hotplug_info *hp_info)
{
    hp_info->scene_code = SCENE_CODE_OF_FUNC_HP_INFO(hp_req->hotplug_info);
    hp_info->function_type = FUNC_TYPE_OF_FUNC_HP_INFO(hp_req->hotplug_info);
    hp_info->ep_id = EP_ID_OF_FUNC_HP_INFO(hp_req->hotplug_info);
    hp_info->pf_id = PF_ID_OF_FUNC_HP_INFO(hp_req->hotplug_info);
    hp_info->vf_id = VF_ID_OF_FUNC_HP_INFO(hp_req->hotplug_info);
}

static int func_remove(unsigned int function_type, struct func_hotplug_req *hp_req, struct pci_dev *hp_pdev)
{
    int ret = PCIE_SUCCESS;

    if (function_type == PCIE_FUNC_TYPE_PF)
    {
        ret = update_pf_state(hp_pdev);
    }
    else if (function_type == PCIE_FUNC_TYPE_VF)
    {
        ret = vf_bind_unbind(hp_req);
    }
    else
    {
        DH_LOG_ERR(MODULE_FUC_HP, "error function_type=0x%x\n", function_type);
        ret = PCIE_FAILED;
    }

    return ret;
}

static int func_insert(unsigned int function_type, struct func_hotplug_req *hp_req, struct pci_dev *hp_pdev)
{
    int ret = PCIE_SUCCESS;

    if (function_type == PCIE_FUNC_TYPE_PF)
    {
        ret = rescan_pci_dev(hp_pdev);
    }
    else if (function_type == PCIE_FUNC_TYPE_VF)
    {
        ret = vf_bind_unbind(hp_req);
    }
    else
    {
        DH_LOG_ERR(MODULE_FUC_HP, "error function_type=0x%x\n", function_type);
        ret = PCIE_FAILED;
    }

    return ret;
}

static void remove_history_pf(u32 pf_remove_mask, struct func_hotplug_req *hp_req, struct func_hotplug_state *hp_state)
{
    u32 mask_idx = 0;
    u32 ep_no = 0;
    u32 pf_no = 0;

    DH_LOG_INFO(MODULE_FUC_HP, "pf_remove_mask 0x%x\n", pf_remove_mask);

    for (mask_idx = 0; mask_idx < MAX_PF_NUMS; mask_idx++)
    {
        ep_no = mask_idx / MAX_PF_NUMS_OF_EP;
        pf_no = mask_idx % MAX_PF_NUMS_OF_EP;
        hp_req->bdf = hp_state->bdf[ep_no];
        hp_req->bdf = hp_req->bdf + pf_no;
        if (((pf_remove_mask >> mask_idx) & MASK_BIT))
        {
            remove_pci_dev(hp_req);
        }
    }
}

int func_hp(struct pci_dev *hp_pdev)
{
    int ret = PCIE_SUCCESS;
    struct func_hotplug_req hp_req = {0};
    struct func_hotplug_result hp_result = {0};
    struct func_hotplug_info hp_info = {0};
    void __iomem *bar_virt_addr = NULL;
    u64 bar_addr = 0;
    int resp_msg = 0;

    if (IS_ERR_OR_NULL(hp_pdev))
    {
        DH_LOG_ERR(MODULE_FUC_HP, "This hp device was not found. Please confirm the BDF\n");
        return PCIE_FAILED;
    }

    bar_addr = pci_resource_start(hp_pdev, 0);
    bar_virt_addr = (void __iomem *)ioremap(bar_addr, HP_IOREMAP_SIZE);
    if (bar_virt_addr == NULL)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "bar_virt_addr map failed\n");
        return PCIE_FAILED;
    }

    memcpy(&hp_req, bar_virt_addr, sizeof(struct func_hotplug_req));
    func_hp_info_decode(&hp_req, &hp_info);
    print_func_hotplug_req(&hp_req);

    hp_req.domain = pci_domain_nr(hp_pdev->bus);

    switch (hp_info.scene_code)
    {
    case FUNCTION_REMOVE:
    {
        ret = func_remove(hp_info.function_type, &hp_req, hp_pdev);
        break;
    }
    case FUNCTION_INSERT:
    {
        ret = func_insert(hp_info.function_type, &hp_req, hp_pdev);
        break;
    }
    default:
    {
        DH_LOG_ERR(MODULE_FUC_HP, "error scene_code=0x%x\n", hp_info.scene_code);
        ret = PCIE_FAILED;
    }
    }

    hp_result.cmd = PCIE_HOTPLUG_FINISH;
    hp_result.hotplug_info = hp_req.hotplug_info;
    hp_result.result = (ret == PCIE_SUCCESS) ? FUNC_HP_RESULT_SUCC : FUNC_HP_RESULT_FAIL;

    ret = hpf_send_msg_to_riscv(&hp_result, sizeof(struct func_hotplug_result), &resp_msg, sizeof(int), hp_pdev);
    if (ret)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "send failed\n");
    }

    iounmap(bar_virt_addr);
    bar_virt_addr = NULL;
    return ret;
}

int enable_vf(void)
{
    int pos = 0;
    u16 total_vf_nums = 0;
    int ret = PCIE_SUCCESS;
    struct pci_dev *dev = NULL;
    int i = 0;
    int entry_num = sizeof(zte_dpu_id_info) / sizeof(struct zte_dpu_id_info);

    for (i = 0; i < entry_num; i++)
    {
        while ((dev = pci_get_device(zte_dpu_id_info[i].vendor_id, zte_dpu_id_info[i].device_id, dev)) != NULL)
        {
            pos = pci_find_ext_capability(dev, PCI_EXT_CAP_ID_SRIOV);
            if (!pos)
            {
                continue;
            }
            pci_read_config_word(dev, (pos + PCI_SRIOV_TOTAL_VF), &total_vf_nums);
            DH_LOG_DEBUG(MODULE_FUC_HP, "Total VFs: %d  ", total_vf_nums);
            pci_sriov_configure_simple(dev, total_vf_nums);
        }
    }

    return ret;
}

int init_dpu_bus_info(struct pci_dev *pdev)
{
    struct pci_dev *switch_pdev = NULL;
    struct pci_dev *child_pdev = NULL;
    u32 ep_no = 0;

    if (pdev == NULL)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "%s: hp_device not found\n", __func__);
        return PCIE_FAILED;
    }

    switch_pdev = pdev->bus->parent->self;
    if (!switch_pdev)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "%s: switch_pdev not found\n", __func__);
        return PCIE_FAILED;
    }

    list_for_each_entry(child_pdev, &switch_pdev->subordinate->devices, bus_list)
    {
        if (!child_pdev->subordinate)
        {
            DH_LOG_ERR(MODULE_FUC_HP, "%s: child_pdev->subordinate is NULL!\n", __func__);
            return PCIE_FAILED;
        }

        if (ep_no < sizeof(dpu_bus_list) / sizeof(u32))
        {
            dpu_bus_list[ep_no] = child_pdev->subordinate->number;
            ep_no++;
        }
        else
        {
            break;
        }
    }

    return PCIE_SUCCESS;
}

int init_bdf(struct pci_dev *pdev)
{
    int ret = PCIE_SUCCESS;
    int resp_msg = 0;
    struct fuc_hotplug_bar_msg fuc_hotplug_bar_msg = {0};

    fuc_hotplug_bar_msg.cmd = PCIE_HOTPLUG_START;
    fuc_hotplug_bar_msg.fuc_hotplug_info = FUC_HOTPLUG_PF_INIT_FLAG;

    if (pdev == NULL)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "pdev is null.\n");
        return PCIE_FAILED;
    }

    ret = init_dpu_bus_info(pdev);
    if (ret != 0)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "init_dpu_bus_info failed.\n");
    }

    if (pci_enable_device(pdev))
    {
        DH_LOG_ERR(MODULE_FUC_HP, "enbale dev failed\n");
    }

    ret = hpf_send_msg_to_riscv(&fuc_hotplug_bar_msg, sizeof(struct fuc_hotplug_bar_msg), &resp_msg, sizeof(int), pdev);
    if (ret != 0)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "get dbf failed\n");
    }

    return PCIE_SUCCESS;
}

int update_pf_state(struct pci_dev *hp_pdev)
{
    u64 bar_addr = 0;
    u8 ep_no = 0;
    u8 pf_no = 0;
    u32 mask_idx = 0;
    struct func_hotplug_req hp_req = {0};
    struct func_hotplug_info hp_info = {0};
    struct func_hotplug_state func_hotplug_state = {0};
    u32 pf_state_mask = 0;
    void __iomem *bar_virt_addr = NULL;
    u32 pf_remove_history = 0;
    static u32 rescan_count = 0;

    if (hp_pdev == NULL)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "hp_pdev is null\n");
        return -ENOMEM;
    }

    bar_addr = pci_resource_start(hp_pdev, 0);
    bar_virt_addr = (void __iomem *)ioremap(bar_addr, HP_IOREMAP_SIZE);
    if (bar_virt_addr == NULL)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "bar_virt_addr map failed\n");
        return -ENOMEM;
    }

    // 获取当前操作的pf
    memcpy(&hp_req, bar_virt_addr, sizeof(struct func_hotplug_req));
    func_hp_info_decode(&hp_req, &hp_info);

    memcpy(&func_hotplug_state, bar_virt_addr + FUC_HOTPLUG_PF_STATE_FLAG_OFFSET, sizeof(struct func_hotplug_state));
    memcpy(&pf_remove_history, bar_virt_addr + FUC_HOTPLUG_PF_INIT_FLAG_OFFSET, sizeof(u32));
    memset(bar_virt_addr + FUC_HOTPLUG_PF_INIT_FLAG_OFFSET, 0, sizeof(u32));

    iounmap(bar_virt_addr);
    bar_virt_addr = NULL;

    pf_state_mask = func_hotplug_state.pf_state_mask | 0x1;

    for (mask_idx = 0; mask_idx < MAX_PF_NUMS; mask_idx++)
    {
        ep_no = mask_idx / MAX_PF_NUMS_OF_EP;
        pf_no = mask_idx % MAX_PF_NUMS_OF_EP;
        hp_req.domain = pci_domain_nr(hp_pdev->bus);
        hp_req.bdf = PCI_DEVID(dpu_bus_list[ep_no], pf_no);
        if (!((pf_state_mask >> mask_idx) & MASK_BIT))
        {
            remove_pci_dev(&hp_req);
        }
    }

    if ((pf_remove_history != INVALID_MASK_VALUE) && (pf_remove_history != DEFLUAT_DDR_VALUE) && (pf_remove_history & func_hotplug_state.pf_state_mask))
    {
        rescan_count++;
        if (rescan_count == MAX_RESCAN_NUMS)
        {
            DH_LOG_ERR(MODULE_FUC_HP, "raise max rescan count\n");
            return PCIE_SUCCESS;
        }

        DH_LOG_INFO(MODULE_FUC_HP, "rescan_count %x\n", rescan_count);

        remove_history_pf(pf_remove_history & func_hotplug_state.pf_state_mask, &hp_req, &func_hotplug_state);
        rescan_pci_dev(hp_pdev);
    }
    rescan_count = 0;
    return PCIE_SUCCESS;
}

irqreturn_t func_hp_msix_handler(int irq_no, void *data)
{
    DH_LOG_INFO(MODULE_FUC_HP, "irq[%d] has been triggered\n", irq_no);

    return IRQ_WAKE_THREAD;
}

irqreturn_t irq_handler2(int irq_no, void *data)
{
    DH_LOG_INFO(MODULE_FUC_HP, "irq[%d] has been triggered\n", irq_no);

    return IRQ_WAKE_THREAD;
}

irqreturn_t func_hp_msix_thread(int irq, void *data)
{
    struct pci_dev *dev = (struct pci_dev *)data;
    if (dev == NULL)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "dev is null\n");
        return IRQ_HANDLED;
    }

    func_hp(dev);
    return IRQ_HANDLED;
}

irqreturn_t irq_thread2(int irq, void *data)
{
    return IRQ_HANDLED;
}
