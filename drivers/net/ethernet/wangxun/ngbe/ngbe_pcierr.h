/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _NGBE_PCIERR_H_
#define _NGBE_PCIERR_H_

#include "ngbe.h"

#define NGBE_AER_UNCORRECTABLE			1
#define NGBE_AER_CORRECTABLE			2

void ngbe_pcie_do_recovery(struct pci_dev *dev);
void ngbe_aer_print_error(struct ngbe_adapter *adapter, u32 severity, u32 status);
bool ngbe_check_recovery_capability(struct pci_dev *dev);

#endif

