// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2024 3snic Technologies Co., Ltd */

#include <linux/pci.h>
#include <linux/aer.h>
#include "sss_kernel.h"
#include "sss_pci.h"

#ifdef USE_OLD_PCI_FUNCTION
#define	PCI_EXP_AER_FLAGS	(PCI_EXP_DEVCTL_CERE | PCI_EXP_DEVCTL_NFERE | \
				 PCI_EXP_DEVCTL_FERE | PCI_EXP_DEVCTL_URRE)

MODULE_IMPORT_NS(CXL);

int pci_disable_pcie_error_reporting(struct pci_dev *dev)
{
	int rc;

	if (!pcie_aer_is_native(dev))
		return -EIO;

	rc = pcie_capability_clear_word(dev, PCI_EXP_DEVCTL, PCI_EXP_AER_FLAGS);
	return pcibios_err_to_errno(rc);
}

int pci_enable_pcie_error_reporting(struct pci_dev *dev)
{
	int rc;

	if (!pcie_aer_is_native(dev))
		return -EIO;

	rc = pcie_capability_set_word(dev, PCI_EXP_DEVCTL, PCI_EXP_AER_FLAGS);
	return pcibios_err_to_errno(rc);
}

#endif
