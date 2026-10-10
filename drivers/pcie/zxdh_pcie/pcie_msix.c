#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/err.h>

#include "bar_msg.h"
#include "pcie_msix.h"
#include "function_hotplug.h"
#include "pcie_common.h"

extern irqreturn_t func_hp_msix_handler(int irq_no, void *data);
extern irqreturn_t irq_handler2(int irq_no, void *data);
extern irqreturn_t func_hp_msix_thread(int irq_no, void *data);
extern irqreturn_t irq_thread2(int irq_no, void *data);

static struct msix_handler_info msix_handler_info_arr[] = {
    /* irq_id,              irq_handler              irq_thread                     irq_name */
    {IRQ_NO_INIT_VALUE,     func_hp_msix_handler,    func_hp_msix_thread,           "dinghai10e_hpf"},
    {IRQ_NO_INIT_VALUE,     irq_handler2,            irq_thread2,                   "dinghai10e_hpf"},
};

int request_hpf_msix(struct pci_dev *pdev)
{
    int ret = 0;
    int i = 0;
    int msix_nums = 0;

    if (pdev == NULL)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "Can not find devices\n");
        return -EINVAL;
    }

    msix_nums = sizeof(msix_handler_info_arr) / sizeof(struct msix_handler_info);
    ret = pci_alloc_irq_vectors(pdev, msix_nums, msix_nums, PCI_IRQ_MSIX);
    if (ret < 0)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "Can not alloc msix irq vector msix_nums=%d, ret=%d\n", msix_nums, ret);
        return ret;
    }

    for (i = 0; i < msix_nums; ++i)
    {
        msix_handler_info_arr[i].irq_id = i;

        ret = pci_request_irq(pdev, i, msix_handler_info_arr[i].irq_handler_func, msix_handler_info_arr[i].irq_thread_func,
                              (void *)pdev, "%s%d@pci:%s", msix_handler_info_arr[i].irq_name, i, pci_name(pdev));
        if (ret)
        {
            pci_free_irq_vectors(pdev);
            DH_LOG_ERR(MODULE_FUC_HP, "request msix[%d] failed --> %d\n", i, ret);
        }
    }

    return ret;
}

int free_hpf_msix(struct pci_dev *pdev)
{
    int msix_nums = 0;
    int i = 0;

    if (IS_ERR_OR_NULL(pdev))
    {
        DH_LOG_WARNING(MODULE_FUC_HP, "pdev has released\n");
        return 0;
    }

    msix_nums = sizeof(msix_handler_info_arr) / sizeof(struct msix_handler_info);
    for (i = 0; i < msix_nums; ++i)
    {
        pci_free_irq(pdev, msix_handler_info_arr[i].irq_id, (void *)pdev);
    }
    pci_free_irq_vectors(pdev);

    return 0;
}
