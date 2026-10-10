#include <linux/module.h>
#include <linux/kmod.h>
#include <linux/pci.h>
#include <linux/dinghai/helper.h>
#include <linux/dinghai/devlink.h>
#include <linux/dinghai/dinghai_irq.h>
#include <linux/dinghai/eq.h>
#include <linux/dinghai/driver.h>
#include <linux/dinghai/kcompat.h>
#include "./en_mpf/en_mpf_events.h"
#include "./en_mpf/en_mpf_eq.h"
#include "./en_mpf/en_mpf_irq.h"
#include "en_mpf.h"
#include "en_mpf/en_mpf_cfg_sf.h"

MODULE_LICENSE("Dual BSD/GPL");

uint32_t dh_debug_mask;
module_param_named(debug_mask, dh_debug_mask, uint, 0644);
MODULE_PARM_DESC(debug_mask, "debug mask: 1 = dump cmd data, 2 = dump cmd exec time, 3 = both. Default=0");

extern struct devlink_ops dh_mpf_devlink_ops;
extern struct dh_core_devlink_ops dh_mpf_core_devlink_ops;

int32_t dh_mpf_pci_init(struct dh_core_dev *dev)
{
    int32_t ret = 0;
    struct dh_en_mpf_dev *mpf_dev = NULL;

    pci_set_drvdata(dev->pdev, dev);

    ret = pci_enable_device(dev->pdev);
    if (ret != 0)
    {
        dev_err(dev->device, "pci_enable_device failed: %d\n", ret);
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
    LOG_INFO_DEV(dev, "%s pci_ioremap_addr=0x%llx, ioremap(0x%llx, 0x%llx)\n", __func__,
                mpf_dev->pci_ioremap_addr, pci_resource_start(dev->pdev, 0), pci_resource_len(dev->pdev, 0));
    if (mpf_dev->pci_ioremap_addr == 0)
    {
        ret = -1;
        LOG_ERR_DEV(dev, "ioremap(0x%llx, 0x%llx) failed\n", pci_resource_start(dev->pdev, 0), pci_resource_len(dev->pdev, 0));
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

static const struct pci_device_id dh_mpf_pci_table[] = {
   { PCI_DEVICE(ZXDH_MPF_VENDOR_ID, ZXDH_MPF_DEVICE_ID), 0 },
   { 0, }
};

MODULE_DEVICE_TABLE(pci, dh_mpf_pci_table);

void dh_mpf_pci_close(struct dh_core_dev *dev)
{
    struct dh_en_mpf_dev *mpf_dev = NULL;

    mpf_dev = dh_core_priv(dev);
    iounmap((void *)mpf_dev->pci_ioremap_addr);
    pci_disable_pcie_error_reporting(dev->pdev);
    pci_release_selected_regions(dev->pdev, pci_select_bars(dev->pdev, IORESOURCE_MEM));
    pci_disable_device(dev->pdev);

    return;
}

static int32_t dh_mpf_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    struct dh_core_dev *dh_dev = NULL;
    struct devlink *devlink    = NULL;
    int32_t err                = 0;

    LOG_INFO("mpf driver start to probe %s\n", pci_name(pdev));

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

    err = dh_mpf_pci_init(dh_dev);
    if (err != 0)
    {
        dev_err(&pdev->dev, "dh_mpf_pci_init failed: %d\n", err);
        goto err_devlink_cleanup;
    }

    err = dh_mpf_irq_table_init(dh_dev);
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

    err = dh_mpf_events_init(dh_dev);
    if (err != 0)
    {
        dh_err(dh_dev, "failed to initialize events\n");
        goto err_events_init_cleanup;
    }

#ifdef HAVE_DEVLINK_REGISTER_GET_1_PARAMS
    zxdh_devlink_register(devlink);
#else
    zxdh_devlink_register(devlink, &pdev->dev);
#endif

    LOG_INFO_DEV(dh_dev, "mpf driver probe %s completed\n", pci_name(pdev));
    return 0;

err_events_init_cleanup:
    dh_mpf_eq_table_destroy(dh_dev);
err_eq_table_create:
    dh_mpf_irq_table_destroy(dh_dev);
err_irq_table_create:
    dh_eq_table_cleanup(dh_dev);
err_eq_table_init:
    dh_irq_table_cleanup(dh_dev);
err_pci:
    dh_mpf_pci_close(dh_dev);
err_devlink_cleanup:
    zxdh_devlink_free(devlink);
    return err;
}

static void dh_mpf_remove(struct pci_dev *pdev)
{
    struct dh_core_dev *dh_dev = pci_get_drvdata(pdev);
    struct devlink *devlink = priv_to_devlink(dh_dev);
    LOG_INFO_DEV(dh_dev, "mpf driver start to remove %s", pci_name(pdev));

    zxdh_devlink_unregister(devlink);
    dh_mpf_events_uninit(dh_dev);
    dh_mpf_eq_table_destroy(dh_dev);
    dh_mpf_irq_table_destroy(dh_dev);
    dh_eq_table_cleanup(dh_dev);
    dh_irq_table_cleanup(dh_dev);
    dh_mpf_pci_close(dh_dev);
    zxdh_devlink_free(devlink);

    pci_set_drvdata(pdev, NULL);
    LOG_INFO("mpf driver remove %s completed\n", pci_name(pdev));
}

static int32_t dh_mpf_suspend(struct pci_dev *pdev, pm_message_t state)
{

    return 0;
}

static int32_t dh_mpf_resume(struct pci_dev *pdev)
{

    return 0;
}

static void dh_mpf_shutdown(struct pci_dev *pdev)
{
    dh_mpf_remove(pdev);
}

static pci_ers_result_t dh_pci_err_detected(struct pci_dev *pdev,
                                            pci_channel_state_t state)
{
    return PCI_ERS_RESULT_NONE;
}

static pci_ers_result_t dh_mpf_pci_slot_reset(struct pci_dev *pdev)
{
    return PCI_ERS_RESULT_NONE;
}

static void dh_mpf_pci_resume(struct pci_dev *pdev)
{

}

static const struct pci_error_handlers dh_mpf_err_handler = {
    .error_detected = dh_pci_err_detected,
    .slot_reset     = dh_mpf_pci_slot_reset,
    .resume         = dh_mpf_pci_resume
};

static struct pci_driver dh_mpf_driver = {
    .name           = KBUILD_MODNAME,
    .id_table       = dh_mpf_pci_table,
    .probe          = dh_mpf_probe,
    .remove         = dh_mpf_remove,
    .suspend        = dh_mpf_suspend,
    .resume         = dh_mpf_resume,
    .shutdown       = dh_mpf_shutdown,
    .err_handler    = &dh_mpf_err_handler,
};

static int32_t __init init(void)
{
    int32_t err = 0;

    err = pci_register_driver(&dh_mpf_driver);
    if (err != 0)
    {
        LOG_ERR("pci_register_driver failed: %d\n", err);
        return err;
    }

#ifdef CONFIG_ZXDH_SF
    err = zxdh_mpf_sf_driver_register();
    if (err != 0)
    {
        LOG_ERR("zxdh_en_sf_driver_register failed: %d\n", err);
        goto err_sf;
    }
#endif

    LOG_INFO("zxdh_mpf driver init success\n");

    return 0;

err_sf:
    pci_unregister_driver(&dh_mpf_driver);
    return err;
}

static void __exit cleanup(void)
{
#ifdef CONFIG_ZXDH_SF
    zxdh_mpf_sf_driver_uregister();
#endif
    pci_unregister_driver(&dh_mpf_driver);

    LOG_INFO("zxdh_mpf driver remove success\n");
}

module_init(init);
module_exit(cleanup);
