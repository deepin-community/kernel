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

#include "bar_msg.h"
#include "function_hotplug.h"
#include "pcie_common.h"

#ifdef DRIVER_VERSION_VAL
#define DRV_VERSION DRIVER_VERSION_VAL
#else
#define DRV_VERSION "1.0-1"
#endif

int debug_print;
module_param(debug_print, int, 0644);

const struct fw_compat_version hpf_version =
{
    /*major fw_minor drv_minor patch*/
        0,      0,       0,      0
};

int zxdh_pcie_init(void)
{
    int ret = 0;

    ret = enable_vf();
    if (ret != 0)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "enable_vf failed\n");
        return ret;
    }

    DH_LOG_INFO(MODULE_FUC_HP, "zxdh pf hotplug module init success.\n");
    return ret;
}

void zxdh_pcie_exit(struct pci_dev *pdev)
{
    DH_LOG_INFO(MODULE_FUC_HP, "zxdh pcie remove success.\n");
}

static const struct pci_device_id dh_hpf_pci_table[] = {
   { PCI_DEVICE(HP_VENDOR_ID, HP_DEVICE_ID), 0 },
   { 0, }
};

MODULE_DEVICE_TABLE(pci, dh_hpf_pci_table);

int is_hpf_version_match_fw(struct pci_dev *pdev)
{
    int ret = PCIE_SUCCESS;
    u64 bar_addr = 0;
    u64 bar_len = 0;
    struct version_compat_reg fw_version = {0};
    void __iomem *bar_virt_addr = NULL;
    u32 fw_feature = 0;

    bar_addr = pci_resource_start(pdev, 0);
    bar_len = pci_resource_len(pdev, 0);
    bar_virt_addr = (void __iomem *)ioremap(bar_addr, bar_len);
    if (bar_virt_addr == NULL)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "bar_virt_addr map failed\n");
        return -ENOMEM;
    }
    memcpy(&fw_feature, bar_virt_addr + FW_FEATURE_OF_ZF_MPF_OFFSET, sizeof(u32));
    if (!(fw_feature & FW_FEATURE_SUPPORT_MASK))
    {
        DH_LOG_WARNING(MODULE_MPF, "fw dont support feature function\n");
        ret = PCIE_SUCCESS;
        goto finish;
    }

    memcpy(&fw_version, bar_virt_addr + VERSION_OF_HPF_OFFSET, sizeof(struct version_compat_reg));
    if (fw_version.version_compat_item != HPF_COMPAT_ITEM)
    {
        DH_LOG_ERR(MODULE_MPF, "version_compat_item 0x%x is not hpf(%x)! \n",
                   fw_version.version_compat_item,
                   HPF_COMPAT_ITEM);
        ret = -1;
        goto finish;
    }

    if (fw_version.major != hpf_version.major ||
        fw_version.fw_minor < hpf_version.fw_minor ||
        fw_version.drv_minor > hpf_version.drv_minor)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "fw_version: %x.%x.%x.%x\n",
                   fw_version.major, fw_version.fw_minor,
                   fw_version.drv_minor, fw_version.patch);
        DH_LOG_ERR(MODULE_FUC_HP, "hpf_version: %x.%x.%x.%x\n",
                   hpf_version.major, hpf_version.fw_minor,
                   hpf_version.drv_minor, hpf_version.patch);
        ret = -1;
        goto finish;
    }

finish:
    iounmap(bar_virt_addr);
    return ret;
}

static int32_t dh_hpf_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    int32_t ret = 0;

    ret = pci_enable_device(pdev);
    if (ret != 0)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "enable hotplug device failed.\n");
        return ret;
    }

    ret = is_hpf_version_match_fw(pdev);
    if (ret != 0)
    {
        return ret;
    }

    ret = init_bdf(pdev);
    if (ret != 0)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "init_bdf failed.\n");
    }

    ret = update_pf_state(pdev);
    if (ret != 0)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "init_pf_state failed\n");
        return ret;
    }

    ret = request_hpf_msix(pdev);
    if (ret != 0)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "request_hpf_msix failed\n");
        return ret;
    }

    DH_LOG_INFO(MODULE_FUC_HP, "hpf driver probe completed\n");
    return 0;
}

static void dh_hpf_remove(struct pci_dev *pdev)
{
    free_hpf_msix(pdev);
    zxdh_pcie_exit(pdev);
    DH_LOG_INFO(MODULE_FUC_HP, "hpf driver remove completed\n");
}

static int32_t dh_hpf_suspend(struct pci_dev *pdev, pm_message_t state)
{
    return 0;
}

static int32_t dh_hpf_resume(struct pci_dev *pdev)
{
    return 0;
}

static void dh_hpf_shutdown(struct pci_dev *pdev)
{
    dh_hpf_remove(pdev);
}

static pci_ers_result_t dh_pci_err_detected(struct pci_dev *pdev,
                                            pci_channel_state_t state)
{
    DH_LOG_INFO(MODULE_FUC_HP, "%s was called\n", __func__);

    return state == pci_channel_io_perm_failure ? PCI_ERS_RESULT_DISCONNECT : PCI_ERS_RESULT_NEED_RESET;
}

static pci_ers_result_t dh_hpf_pci_slot_reset(struct pci_dev *pdev)
{
    return PCI_ERS_RESULT_RECOVERED;
}

static void dh_hpf_pci_resume(struct pci_dev *pdev)
{
    DH_LOG_INFO(MODULE_FUC_HP, "%s was called\n", __func__);
}

static const struct pci_error_handlers dh_hpf_err_handler = {
    .error_detected = dh_pci_err_detected,
    .slot_reset     = dh_hpf_pci_slot_reset,
    .resume         = dh_hpf_pci_resume
};

static struct pci_driver dh_hpf_driver = {
    .name           = KBUILD_MODNAME,
    .id_table       = dh_hpf_pci_table,
    .probe          = dh_hpf_probe,
    .remove         = dh_hpf_remove,
    .suspend        = dh_hpf_suspend,
    .resume         = dh_hpf_resume,
    .shutdown       = dh_hpf_shutdown,
    .err_handler    = &dh_hpf_err_handler,
};

static int32_t __init init(void)
{
    int32_t err = 0;

    err = pci_register_driver(&dh_hpf_driver);
    if (err != 0)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "pci_register_driver failed: %d\n", err);
        return err;
    }

    zxdh_pcie_init();

    DH_LOG_INFO(MODULE_FUC_HP, "zxdh_hpf driver init success\n");
    return err;
}

static void __exit cleanup(void)
{
    pci_unregister_driver(&dh_hpf_driver);
    DH_LOG_INFO(MODULE_FUC_HP, "zxdh_hpf driver remove success\n");
}

module_init(init);
module_exit(cleanup);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("ZTE ZXDH HP");
MODULE_VERSION(DRV_VERSION);
