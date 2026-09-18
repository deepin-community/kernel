#include <linux/module.h>
#include <linux/kmod.h>
#include <linux/pci.h>
#include <linux/dinghai/helper.h>
#include <linux/dinghai/devlink.h>
#include <linux/dinghai/dinghai_irq.h>
#include <linux/dinghai/eq.h>
#include <linux/dinghai/driver.h>
#include <linux/dinghai/kcompat.h>
#include <linux/dinghai/queue.h>
#include <en_pf.h>
#include "zf_events.h"
#include "zf_mpf_eq.h"
#include "zf_mpf_irq.h"
#include "zf_mpf.h"
#include "gdma.h"

#ifdef PCIE_ZF_EPC_OPEN
#include "epc/pcie-zte-zf-epc.h"
#endif

#ifdef DRIVER_VERSION_VAL
#define DRV_VERSION DRIVER_VERSION_VAL
#else
#define DRV_VERSION "1.0-1"
#endif

const struct fw_compat_version zf_mpf_version =
{
    /*major fw_minor drv_minor patch*/
        0,      0,       0,      0
};

MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION(DRV_VERSION);

uint32_t zf_dh_debug_mask;
module_param_named(debug_mask, zf_dh_debug_mask, uint, 0644);
MODULE_PARM_DESC(debug_mask, "debug mask: 1 = dump cmd data, 2 = dump cmd exec time, 3 = both. Default=0");

extern struct devlink_ops dh_mpf_devlink_ops;
extern struct dh_core_devlink_ops dh_mpf_core_devlink_ops;

extern int zxdh_host_reset_driver_init(struct dh_core_dev *core_dev);
extern void zxdh_host_reset_driver_exit(struct dh_core_dev *core_dev);
extern int zxdh_host_fuc_hotplug_driver_init(void);
extern void zxdh_host_fuc_hotplug_driver_exit(void);
extern int zf_reset_finish_flag_init(struct dh_core_dev *dh_dev, unsigned long ep_mpf_paddr);
extern void zf_reset_finish_flag_exit(void);
extern int zxdh_bar_ioctl_msg_mdl_init(struct dh_core_dev *core_dev);
extern void zxdh_bar_ioctl_msg_mdl_exit(struct dh_core_dev *core_dev);

int is_zf_mpf_version_match_fw(struct pci_dev *pdev);

static int32_t dh_zf_mpf_pci_init(struct dh_core_dev *dev)
{
    int32_t ret = 0;
    struct dh_en_mpf_dev *mpf_dev = NULL;

    pci_set_drvdata(dev->pdev, dev);

    ret = pci_enable_device(dev->pdev);
    if (ret)
    {
        dev_err(dev->device, "pci_enable_device failed: %d\n", ret);
        return -ENOMEM;
    }

    if (is_zf_mpf_version_match_fw(dev->pdev))
    {
        DH_LOG_ERR_DEV(MODULE_MPF, dev, "zf_mpf_version cant match fw\n");
        return -ENOMEM;
    }

    ret = dma_set_mask_and_coherent(dev->device, DMA_BIT_MASK(64));
    if (ret != 0)
    {
        ret = dma_set_mask_and_coherent(dev->device, DMA_BIT_MASK(32));
        if (ret != 0)
        {
            dev_err(dev->device, "dma_set_mask_and_coherent failed: %d\n", ret);
            goto err_pci;
        }
    }

    ret = pci_request_selected_regions(dev->pdev, pci_select_bars(dev->pdev, IORESOURCE_MEM), "dh-mpf");
    if (ret != 0)
    {
        dev_err(dev->device, "pci_request_selected_regions failed: %d\n", ret);
        goto err_pci;
    }

    pci_enable_pcie_error_reporting(dev->pdev);
    pci_set_master(dev->pdev);
    ret = pci_save_state(dev->pdev);
    if (ret != 0)
    {
        dev_err(dev->device, "pci_save_state failed: %d\n", ret);
        goto err_pci_save_state;
    }

    mpf_dev = dh_core_priv(dev);
    mpf_dev->pci_ioremap_addr = (uint64_t)ioremap(pci_resource_start(dev->pdev, 0), pci_resource_len(dev->pdev, 0));
    DH_LOG_INFO_DEV(MODULE_MPF, dev, "%s pci_ioremap_addr=0x%llx, ioremap(0x%llx, 0x%llx)\n",
        __func__, mpf_dev->pci_ioremap_addr, pci_resource_start(dev->pdev, 0), pci_resource_len(dev->pdev, 0));
    if (mpf_dev->pci_ioremap_addr == 0)
    {
        ret = -1;
        DH_LOG_ERR_DEV(MODULE_MPF, dev, "ioremap(0x%llx, 0x%llx) failed\n",
            pci_resource_start(dev->pdev, 0), pci_resource_len(dev->pdev, 0));
        goto err_pci_save_state;
    }

    return 0;

err_pci_save_state:
    pci_disable_pcie_error_reporting(dev->pdev);
    pci_release_selected_regions(dev->pdev, pci_select_bars(dev->pdev, IORESOURCE_MEM));
err_pci:
    pci_disable_device(dev->pdev);
    return ret;
}

static const struct pci_device_id dh_zf_mpf_pci_table[] = {
   { PCI_DEVICE(ZXDH_MPF_VENDOR_ID, ZXDH_MPF_DEVICE_ID0), 0 },
   { PCI_DEVICE(ZXDH_MPF_VENDOR_ID, ZXDH_MPF_DEVICE_ID1), 0 },
   { 0, }
};

MODULE_DEVICE_TABLE(pci, dh_zf_mpf_pci_table);

void dh_zf_mpf_pci_close(struct dh_core_dev *dev)
{
    struct dh_en_mpf_dev *mpf_dev = NULL;

    mpf_dev = dh_core_priv(dev);
    iounmap((void __iomem *)mpf_dev->pci_ioremap_addr);
    pci_disable_pcie_error_reporting(dev->pdev);
    pci_release_selected_regions(dev->pdev, pci_select_bars(dev->pdev, IORESOURCE_MEM));
    pci_disable_device(dev->pdev);

    return;
}

int32_t dh_zf_mpf_pcie_id_get(struct dh_core_dev *dh_dev)
{
    u8 pos = 0;
    uint8_t type = 0;
    uint16_t padding = 0;
    struct dh_en_mpf_dev *mpf_dev = dh_core_priv(dh_dev);
    struct pci_dev *pdev = dh_dev->pdev;

    for (pos = pci_find_capability(pdev, PCI_CAP_ID_VNDR); pos > 0; pos = pci_find_next_capability(pdev, pos, PCI_CAP_ID_VNDR))
    {
        pci_read_config_byte(pdev, pos + offsetof(struct zxdh_pf_pci_cap, cfg_type), &type);

        if (type == ZXDH_PCI_CAP_PCI_CFG)
        {
            pci_read_config_word(pdev, pos + offsetof(struct zxdh_pf_pci_cap, padding[0]), &padding);
            mpf_dev->pcie_id = padding;
            DH_LOG_INFO_DEV(MODULE_MPF, dh_dev, "%s pf_dev->pcie_id: 0x%x\n", __func__, mpf_dev->pcie_id);
            return 0;
        }
    }
    return -1;
}

int is_zf_mpf_version_match_fw(struct pci_dev *pdev)
{
    int ret = 0;
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
        DH_LOG_ERR(MODULE_MPF, "bar_virt_addr map failed\n");
        return -1;
    }

    memcpy(&fw_feature, bar_virt_addr + FW_FEATURE_OF_ZF_MPF_OFFSET, sizeof(u32));
    if (!(fw_feature & FW_FEATURE_SUPPORT_MASK))
    {
        DH_LOG_WARNING(MODULE_MPF, "fw dont support feature function\n");
        ret = 0;
        goto finish;
    }

    memcpy(&fw_version, bar_virt_addr + VERSION_OF_ZF_MPF_OFFSET, sizeof(struct version_compat_reg));
    if (fw_version.version_compat_item != ZF_MPF_COMPAT_ITEM)
    {
        DH_LOG_ERR(MODULE_MPF, "version_compat_item 0x%x is not zf_mpf(%x)! \n",
                   fw_version.version_compat_item,
                   ZF_MPF_COMPAT_ITEM);
        ret = -1;
        goto finish;
    }

    if (fw_version.major != zf_mpf_version.major ||
        fw_version.fw_minor < zf_mpf_version.fw_minor ||
        fw_version.drv_minor > zf_mpf_version.drv_minor)
    {
        DH_LOG_ERR(MODULE_MPF, "fw_version: %x.%x.%x.%x\n",
                   fw_version.major,
                   fw_version.fw_minor,
                   fw_version.drv_minor,
                   fw_version.patch);
        DH_LOG_ERR(MODULE_MPF, "zf_mpf_version: %x.%x.%x.%x\n",
                   zf_mpf_version.major,
                   zf_mpf_version.fw_minor,
                   zf_mpf_version.drv_minor,
                   zf_mpf_version.patch);
        ret = -1;
    }

finish:
    iounmap(bar_virt_addr);
    return ret;
}

static int32_t dh_zf_mpf_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    struct dh_core_dev *dh_dev = NULL;
    struct devlink *devlink    = NULL;
    int32_t err                = 0;

    DH_LOG_INFO(MODULE_MPF, "mpf driver start to probe\n");

    devlink = zxdh_devlink_alloc(&pdev->dev, &dh_mpf_devlink_ops, sizeof(struct dh_en_mpf_dev));
    if (devlink == NULL)
    {
        dev_err(&pdev->dev, "devlink alloc failed\n");
        return -ENOMEM;
    }

    dh_dev = devlink_priv(devlink);
    dh_dev->device = &pdev->dev;
    dh_dev->pdev = pdev;
    dh_dev->devlink_ops = &dh_mpf_core_devlink_ops;

    err = dh_zf_mpf_pci_init(dh_dev);
    if (err != 0)
    {
        dev_err(&pdev->dev, "%s failed: %d\n", __func__, err);
        goto err_devlink_cleanup;
    }

    err = dh_zf_mpf_pcie_id_get(dh_dev);
    if (err != 0)
    {
        dev_err(&pdev->dev, "dh_pf_pcie_id_get failed: %d\n", err);
        goto err_pci;
    }

    err = dh_mpf_irq_table_init(dh_dev); // dh_dev->irq_table.priv = kvzalloc(sizeof(struct dh_mpf_irq_table), GFP_KERNEL);
    if (err != 0)
    {
        dh_err(dh_dev, "Failed to alloc IRQs\n");
        goto err_pci;
    }

    err = dh_mpf_eq_table_init(dh_dev);
    if (err != 0)
    {
        dh_err(dh_dev, "Failed to alloc IRQs\n");
        goto err_eq_table_init;
    }

    err = dh_mpf_irq_table_create(dh_dev);
    if (err != 0)
    {
        dh_err(dh_dev, "Failed to alloc IRQs\n");
        goto err_irq_table_create;
    }

    err = dh_mpf_eq_table_create(dh_dev);
    if (err != 0)
    {
        dh_err(dh_dev, "Failed to alloc EQs\n");
        goto err_eq_table_create;
    }

#ifdef PCIE_ZF_EPC_OPEN
    err = pcie_zte_zf_epc_module_init(dh_dev, id);
    if (err != 0)
    {
        dh_err(dh_dev, "failed to initialize epc\n");
        goto err_epc_init;
    }

    if (dh_dev->zf_ep != NULL)
    {
        if (dh_dev->zf_ep->mpf_paddr != 0)
        {
            err = zf_reset_finish_flag_init(dh_dev, dh_dev->zf_ep->mpf_paddr);
            if (err != 0)
            {
                dh_err(dh_dev, "failed to initialize zf_reset_finish_flag_init\n");
                goto err_epc_init;
            }
        }
    }
#endif

#ifdef CONFIG_ZF_GDMA
    if (dh_dev->zf_ep != NULL)
    {
        err = dh_zf_mpf_gdma_init(dh_dev);
        if (err != 0)
        {
            dh_err(dh_dev, "Failed to initialize gdma module\n");
            goto err_gdma_init;
        }
        err = create_gdma_eqs(dh_dev);
        if (err != 0)
        {
            dh_err(dh_dev, "Failed to create gdma EQs\n");
            goto err_gdma_eq_destory;
        }
    }
#endif

    err = dh_zf_mpf_events_init(dh_dev);
    if (err != 0)
    {
        dh_err(dh_dev, "failed to initialize events\n");
        goto err_events_init_cleanup;
    }

    err = zxdh_host_reset_driver_init(dh_dev);
    if (err != 0)
    {
        DH_LOG_ERR(MODULE_MPF, "zxdh_host_reset_driver_init failed: %d\n", err);
        goto err_zf_host_reset;
    }

    err = zxdh_bar_ioctl_msg_mdl_init(dh_dev);
    if (err != 0)
    {
        DH_LOG_ERR(MODULE_MPF, "zxdh_bar_ioctl_msg_mdl_init failed: %d\n", err);
        goto err_zf_host_reset;
    }

#ifdef HAVE_DEVLINK_REGISTER_GET_1_PARAMS
    zxdh_devlink_register(devlink);
#else
    zxdh_devlink_register(devlink, &pdev->dev);
#endif

    DH_LOG_INFO_DEV(MODULE_MPF, dh_dev, "mpf driver probe completed\n");
    return 0;

err_zf_host_reset:
    zxdh_host_reset_driver_exit(dh_dev);
err_events_init_cleanup:
#ifdef CONFIG_ZF_GDMA
    destroy_gdma_eqs(dh_dev);
err_gdma_eq_destory:
    dh_zf_mpf_gdma_uninit(dh_dev);
err_gdma_init:
#endif
#ifdef PCIE_ZF_EPC_OPEN
    zf_reset_finish_flag_exit();
    pcie_zte_zf_epc_free(dh_dev);
err_epc_init:
#endif
    dh_mpf_eq_table_destroy(dh_dev);
err_eq_table_create:
    dh_mpf_irq_table_destroy(dh_dev);
err_irq_table_create:
    dh_eq_table_cleanup(dh_dev);
err_eq_table_init:
    dh_irq_table_cleanup(dh_dev);
err_pci:
    dh_zf_mpf_pci_close(dh_dev);
err_devlink_cleanup:
    zxdh_devlink_free(devlink);
    return err;
}

static void dh_zf_mpf_remove(struct pci_dev *pdev)
{
    struct dh_core_dev *dh_dev = pci_get_drvdata(pdev);
    struct devlink *devlink = priv_to_devlink(dh_dev);

    DH_LOG_INFO_DEV(MODULE_MPF, dh_dev, "mpf driver start to remove");

    zxdh_devlink_unregister(devlink);
    zxdh_bar_ioctl_msg_mdl_exit(dh_dev);
    zxdh_host_reset_driver_exit(dh_dev);
    dh_zf_mpf_events_uninit(dh_dev);
#ifdef CONFIG_ZF_GDMA
    destroy_gdma_eqs(dh_dev);
    dh_zf_mpf_gdma_uninit(dh_dev);
#endif

#ifdef PCIE_ZF_EPC_OPEN
    zf_reset_finish_flag_exit();
    pcie_zte_zf_epc_free(dh_dev);
#endif
    dh_mpf_eq_table_destroy(dh_dev);
    dh_mpf_irq_table_destroy(dh_dev);
    dh_eq_table_cleanup(dh_dev);
    dh_irq_table_cleanup(dh_dev);
    dh_zf_mpf_pci_close(dh_dev);
    zxdh_devlink_free(devlink);

    pci_set_drvdata(pdev, NULL);

    DH_LOG_INFO(MODULE_MPF, "mpf driver remove completed\n");
}

static int32_t dh_zf_mpf_suspend(struct pci_dev *pdev, pm_message_t state)
{

    return 0;
}

static int32_t dh_zf_mpf_resume(struct pci_dev *pdev)
{

    return 0;
}

static void dh_zf_mpf_shutdown(struct pci_dev *pdev)
{
    dh_zf_mpf_remove(pdev);
}

static pci_ers_result_t dh_pci_err_detected(struct pci_dev *pdev,
                                            pci_channel_state_t state)
{
    DH_LOG_INFO(MODULE_MPF, "%s was called\n", __func__);

    return state == pci_channel_io_perm_failure ? PCI_ERS_RESULT_DISCONNECT : PCI_ERS_RESULT_NEED_RESET;
}

static pci_ers_result_t dh_zf_mpf_pci_slot_reset(struct pci_dev *pdev)
{
    return PCI_ERS_RESULT_RECOVERED;
}

static void dh_zf_mpf_pci_resume(struct pci_dev *pdev)
{
    DH_LOG_INFO(MODULE_MPF, "%s was called\n", __func__);
}

static const struct pci_error_handlers dh_zf_mpf_err_handler = {
    .error_detected = dh_pci_err_detected,
    .slot_reset     = dh_zf_mpf_pci_slot_reset,
    .resume         = dh_zf_mpf_pci_resume
};

static struct pci_driver zf_dh_mpf_driver = {
    .name           = KBUILD_MODNAME,
    .id_table       = dh_zf_mpf_pci_table,
    .probe          = dh_zf_mpf_probe,
    .remove         = dh_zf_mpf_remove,
    .suspend        = dh_zf_mpf_suspend,
    .resume         = dh_zf_mpf_resume,
    .shutdown       = dh_zf_mpf_shutdown,
    .err_handler    = &dh_zf_mpf_err_handler,
};

static int32_t __init init(void)
{
    int32_t err = 0;

    err = pci_register_driver(&zf_dh_mpf_driver);
    if (err != 0)
    {
        DH_LOG_ERR(MODULE_MPF, "pci_register_driver failed: %d\n", err);
        return err;
    }

    err = zxdh_host_fuc_hotplug_driver_init();
    if (err != 0)
    {
        DH_LOG_ERR(MODULE_MPF, "fuc_hotplug_driver_init failed: %d\n", err);
        goto err_fuc_hotplug;
    }
    DH_LOG_INFO(MODULE_MPF, "zxdh_mpf driver init success\n");

    return 0;

err_fuc_hotplug:
    return err;
}

static void __exit cleanup(void)
{
    zxdh_host_fuc_hotplug_driver_exit();
    pci_unregister_driver(&zf_dh_mpf_driver);

#ifdef PCIE_ZF_EPC_OPEN
    zf_reset_finish_flag_exit();
#endif

    DH_LOG_INFO(MODULE_MPF, "zxdh_mpf driver remove success\n");
}

module_init(init);
module_exit(cleanup);
