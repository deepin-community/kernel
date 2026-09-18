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
#include <linux/string.h>

#include "pcie_common.h"

struct pci_dev *zxdh_get_pci_device(u32 domain, u32 bdf)
{
    struct pci_bus *bus = NULL;
    struct pci_dev *dev = NULL;

    bus = pci_find_bus(domain, (bdf >> 8));
    if (!bus)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "Can not find this bus\n");
        return NULL;
    }

    while ((dev = pci_get_device(PCI_ANY_ID, PCI_ANY_ID, dev)) != NULL)
    {
        if ((dev->bus == bus) && (PCI_SLOT(dev->devfn) == PCI_SLOT(bdf)) && (PCI_FUNC(dev->devfn) == PCI_FUNC(bdf)))
        {
            return dev;
        }
    }

    return NULL;
}

int fill_domain_bdf_str(const char *token, char *dst_str, const int len)
{
    int ret = PCIE_SUCCESS;
    int i = 0;
    int j = 0;

    if (strlen(token) < len)
    {
        for (i = 0; i < (len - strlen(token)); ++i)
        {
            dst_str[i] = '0';
        }
        for (j = i; j < len; ++j)
        {
            dst_str[j] = token[j - i];
        }
        dst_str[j] = 0;
    }
    else if (strlen(token) > len)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "error token, size=%ld\n", strlen(token));
        ret = PCIE_FAILED;
    }
    else
    {
        memcpy(dst_str, token, strlen(token) + 1);
    }

    return ret;
}

int parse_bdf(char *str)
{
    int ret = PCIE_SUCCESS;
    int size = 0;
    char *token = NULL;
    struct domain_bdf domain_bdf = {0};
    char *tempString = kstrdup(str, GFP_KERNEL);

    memset(&domain_bdf, 0, sizeof(struct domain_bdf));

    if (tempString == NULL)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "tempString is NULL\n");
        return PCIE_FAILED;
    }

    token = strsep(&tempString, ":");
    if (token == NULL)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "strsep fail\n");
        goto failed;
    }
    ret = fill_domain_bdf_str(token, domain_bdf.domain, DOMAIN_LEN);
    if (ret != PCIE_SUCCESS)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "error domain, token=%s\n", token);
        goto failed;
    }

    token = strsep(&tempString, ":");
    if (token == NULL)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "strsep fail\n");
        goto failed;
    }
    ret = fill_domain_bdf_str(token, domain_bdf.bus, BUS_LEN);
    if (ret != PCIE_SUCCESS)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "rror bus\n");
        goto failed;
    }

    token = strsep(&tempString, ".");
    if (token == NULL)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "strsep fail\n");
        goto failed;
    }
    ret = fill_domain_bdf_str(token, domain_bdf.device, DEVICE_LEN);
    if (ret != PCIE_SUCCESS)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "error device\n");
        goto failed;
    }

    token = strsep(&tempString, ".");
    if (token == NULL)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "strsep fail\n");
        goto failed;
    }
    ret = fill_domain_bdf_str(token, domain_bdf.func, FUNC_LEN);
    if (ret != PCIE_SUCCESS)
    {
        DH_LOG_ERR(MODULE_FUC_HP, "error func\n");
        goto failed;
    }

    size = snprintf(str, FILE_PATH_LEN, "%s:%s:%s.%s", domain_bdf.domain, domain_bdf.bus, domain_bdf.device, domain_bdf.func);
    if (size >= FILE_PATH_LEN)
    {
        str[FILE_PATH_LEN - 1] = '\0';
    }

    kfree(tempString);
    tempString = NULL;

    return PCIE_SUCCESS;

failed:
    kfree(tempString);
    tempString = NULL;
    DH_LOG_ERR(MODULE_FUC_HP, "Input format error, refer to '[domain]:[bus_no]:[dev_no].[func_no]'\n");
    return PCIE_FAILED;
}
