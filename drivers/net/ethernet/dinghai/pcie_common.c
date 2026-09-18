#include "pcie_common.h"

/* 看门狗复位场景,该接口为自愈配置回填是否成功的判断;在dpdie复位等其他场景,该接口返回值与配置回填是否成功无关 */
int zxdh_pf_pcie_config_reload_check(struct dh_core_dev *dh_dev)
{
    uint32_t low_addr = 0;
    uint32_t high_addr = 0;
    uint64_t bar0_addr = 0;

    if (dh_dev == NULL)
    {
        HEAL_ERR("dh_dev is null\n");
        return -1;
    }

    if (dh_core_is_vf(dh_dev))
    {
        return 0;
    }

    pci_read_config_dword(dh_dev->pdev, ZXDH_PCIE_BAR0_OFF, &low_addr);
    pci_read_config_dword(dh_dev->pdev, ZXDH_PCIE_BAR1_OFF, &high_addr);

    bar0_addr = (low_addr & (~0xFULL)) | ((uint64_t)high_addr << 32);
    if (bar0_addr == 0)
    {
        HEAL_ERR_DEV(dh_dev, "%s pcie_config_reload failed:0x%llx\n", pci_name(dh_dev->pdev),
                 bar0_addr);
        return -1;
    }

    return 0;
}

int is_zxdh_pf_vf_enable(struct dh_core_dev *dh_dev)
{
    int pos = 0;
    uint16_t vf_nums = 0;

    if (dh_dev == NULL)
    {
        HEAL_ERR("dh_dev is null\n");
        return 0;
    }

    if (dh_core_is_vf(dh_dev))
    {
        return 0;
    }

    pos = pci_find_ext_capability(dh_dev->pdev, PCI_EXT_CAP_ID_SRIOV);
    if (pos == 0)
    {
        return 0;
    }

    pci_read_config_word(dh_dev->pdev, (pos + PCI_SRIOV_NUM_VF), &vf_nums);
    if (vf_nums != 0)
    {
        return 1;
    }

    return 0;
}

#ifdef PCI_DEV_NOT_HAVE_SRIOV
void clear_pf_sriov_status(struct dh_core_dev *dh_dev)
{
    HEAL_INFO_DEV(dh_dev, "dh_dev->pdev no sriov\n");
    return;
}
#else
/*  该恢复接口依赖linux内核中的struct pci_sriov结构体，详见pcie_common.h
    目前在4.x、5.x内核中均支持该接口，其它版本内核是否支持需要进一步确认  */
void clear_pf_sriov_status(struct dh_core_dev *dh_dev)
{
    int pos = 0;
    uint16_t offset = 0;
    uint16_t stride = 0;
    u16 sriov_ctrl = 0;

    if (dh_dev == NULL || dh_dev->pdev == NULL)
    {
        HEAL_ERR("dh_dev is null\n");
        return;
    }

    if (dh_core_is_vf(dh_dev))
    {
        return;
    }

    if (dh_dev->pdev->sriov == NULL)
    {
        HEAL_INFO_DEV(dh_dev, "dh_dev->pdev->sriov is null\n");
        return;
    }

    pos = pci_find_ext_capability(dh_dev->pdev, PCI_EXT_CAP_ID_SRIOV);
    if (pos == 0)
    {
        return;
    }

    pci_read_config_word(dh_dev->pdev, pos + PCI_SRIOV_CTRL, &sriov_ctrl);
    sriov_ctrl &= ~PCI_SRIOV_CTRL_VFE;
    pci_write_config_word(dh_dev->pdev, pos + PCI_SRIOV_CTRL, sriov_ctrl);
    pci_write_config_word(dh_dev->pdev, pos + PCI_SRIOV_NUM_VF, 0);

    pci_read_config_word(dh_dev->pdev, (pos + PCI_SRIOV_VF_OFFSET), &offset);
    pci_read_config_word(dh_dev->pdev, (pos + PCI_SRIOV_VF_STRIDE), &stride);
    dh_dev->pdev->sriov->offset = offset;
    dh_dev->pdev->sriov->stride = stride;

    return;
}
#endif

static void set_vpd_len_in_bus(struct dh_core_dev *dh_dev, size_t vpd_max_size)
{
    struct pci_dev *tmp = NULL;
    struct pci_dev *pdev = NULL;
    struct pci_bus *bus = NULL;

    bus = dh_dev->pdev->bus;
    if (bus == NULL)
    {
        return;
    }

    list_for_each_entry_safe(pdev, tmp, &bus->devices, bus_list)
    {
        if (pdev->is_virtfn)
        {
            return;
        }
#ifdef VPD_STRUCT_COMPAT_V1
        if (!pdev->vpd.len)
        {
            LOG_WARN_DEV(dh_dev, " %s: vpd len set to max 0x%lx !!\n", pci_name(pdev), vpd_max_size);
            pdev->vpd.len = vpd_max_size;
        }
#else
        /* pdev 是同总线下的每一个设备 */
        if ((pdev->vpd) && (!pdev->vpd->len))
        {
            LOG_WARN_DEV(dh_dev, " %s: vpd len set to max 0x%lx !!\n", pci_name(pdev), vpd_max_size);
            pci_set_vpd_size(pdev, vpd_max_size);
        }
#endif
    }

    return;
}

void set_pci_vpd_len_to_max(struct dh_core_dev *dh_dev, size_t vpd_max_size)
{
    if (!dh_dev)
    {
        LOG_ERR("dev is null\n");
        return;
    }

    if (dh_core_is_vf(dh_dev))
    {
        return;
    }

    set_vpd_len_in_bus(dh_dev, vpd_max_size);

    return;
}

int handle_vf_bar_addr(struct dh_core_dev *dh_dev, struct pcie_sriov_bar_info *bar_info, u32 opt)
{
    int pos = 0;
    u32 bar_idx = 0;

    if ((dh_dev == NULL) || (dh_dev->pdev == NULL) || (bar_info == NULL))
    {
        LOG_ERR("Key parameter is NULL\n");
        return PCIE_ERR;
    }

    if (dh_core_is_vf(dh_dev))
    {
        return PCIE_ERR;
    }

    pos = pci_find_ext_capability(dh_dev->pdev, PCI_EXT_CAP_ID_SRIOV);
    if (pos == 0)
    {
        return PCIE_ERR;
    }

    for (bar_idx = 0; bar_idx < PCIE_NORMAL_BAR_MAX_NUMS; bar_idx++)
    {
        if (opt == 0)
        {
            pci_read_config_dword(dh_dev->pdev, pos + SRIOV_BAR_OFF(bar_idx), &bar_info->bar_addr[bar_idx]);
            continue;
        }
        pci_write_config_dword(dh_dev->pdev, pos + SRIOV_BAR_OFF(bar_idx), bar_info->bar_addr[bar_idx]);
    }

    return PCIE_OK;
}
