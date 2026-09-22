/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef ZXDH_TCHMCDMA_H
#define ZXDH_TCHMCDMA_H

#include "main.h"
#include "protos.h"

int host_test_dma_write32(struct zxdh_pci_f *rf);
int host_test_dma_write64(struct zxdh_pci_f *rf);
int host_test_dma_write(struct zxdh_pci_f *rf);
int host_test_dma_write_bysmmu(struct zxdh_pci_f *rf);
int zxdh_sc_dma_wr32_auto(struct zxdh_pci_f *rf);
int zxdh_sc_dma_w32r32_auto(struct zxdh_pci_f *rf);

#endif /* ZXDH_TCHMCDMA_H */
