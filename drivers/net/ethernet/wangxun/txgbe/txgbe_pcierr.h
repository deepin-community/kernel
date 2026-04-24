/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2015 - 2022 Beijing WangXun Technology Co., Ltd. */

#ifndef _TXGBE_PCIERR_H_
#define _TXGBE_PCIERR_H_

#include "txgbe.h"

#define TXGBE_AER_UNCORRECTABLE			1
#define TXGBE_AER_CORRECTABLE			2

void txgbe_pcie_do_recovery(struct pci_dev *dev);
void txgbe_aer_print_error(struct txgbe_adapter *adapter, u32 severity, u32 status);
bool txgbe_check_recovery_capability(struct pci_dev *dev);

#endif
