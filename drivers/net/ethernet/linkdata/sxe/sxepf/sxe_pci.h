#ifndef _SXE_PCI_H_
#define _SXE_PCI_H_

#include "sxe.h"

#define PCI_VENDOR_ID_STARS      0x1FF2
#define SXE_DEV_ID_ASIC          0x10a1

#define SXE_DMA_BIT_WIDTH_64     64
#define SXE_DMA_BIT_WIDTH_32     32

#define SXE_READ_CFG_WORD_FAILED   0xFFFFU

#define SXE_FAILED_READ_CFG_DWORD 0xFFFFFFFFU

u16 sxe_read_pci_cfg_word(struct pci_dev *pdev, struct sxe_hw *hw, u32 reg);

bool sxe_check_cfg_fault(struct sxe_hw *hw, struct pci_dev *dev);

unsigned long sxe_get_completion_timeout(struct sxe_adapter *adapter);

u32 sxe_pcie_timeout_poll(struct pci_dev *pdev, struct sxe_hw *hw);

u32 sxe_read_pci_cfg_dword(struct sxe_adapter *adapter, u32 reg);

#endif
