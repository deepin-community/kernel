// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include <net/addrconf.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/io-mapping.h>
#include <linux/interrupt.h>
#include <linux/inetdevice.h>
#include <linux/time.h>
#include <linux/timex.h>
#include <linux/rtc.h>
#include <linux/aer.h>
#include <linux/debugfs.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_pci_id_tbl.h"
#include "sss_pci_sriov.h"
#include "sss_adapter_mgmt.h"
#include "sss_hwdev_api.h"
#include "sss_hwdev_init.h"

void sss_pci_shutdown(struct pci_dev *pdev)
{
	struct sss_pci_adapter *adapter = sss_get_adapter_by_pcidev(pdev);

	sdk_info(&pdev->dev, "Shutdown device\n");

	if (adapter)
		sss_hwdev_shutdown(adapter->hwdev);

	pci_disable_device(pdev);

	if (adapter)
		sss_hwdev_stop(adapter->hwdev);
}
