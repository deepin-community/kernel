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
#include "sss_hwdev.h"

static void sss_record_pcie_error(void *dev)
{
	struct sss_hwdev *hwdev = (struct sss_hwdev *)dev;

	atomic_inc(&hwdev->hw_stats.fault_event_stats.pcie_fault_stats);
}

pci_ers_result_t sss_detect_pci_error(struct pci_dev *pdev,
				      pci_channel_state_t state)
{
	struct sss_pci_adapter *adapter = sss_get_adapter_by_pcidev(pdev);

	sdk_err(&pdev->dev, "Pci error, state: 0x%08x\n", state);

	pci_cleanup_aer_uncorrect_error_status(pdev);

	if (adapter)
		sss_record_pcie_error(adapter->hwdev);

	return PCI_ERS_RESULT_CAN_RECOVER;
}
