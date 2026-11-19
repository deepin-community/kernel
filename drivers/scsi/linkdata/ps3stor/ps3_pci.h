/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_PCI_H_
#define _PS3_PCI_H_

#ifdef _WINDOWS
#include "ps3_def.h"

#define PCI_CAP_ID_MSIX PCI_CAPABILITY_ID_MSIX
#define PCI_CAP_ID_MSI PCI_CAPABILITY_ID_MSI

#define PCI_MSIX_FLAGS_ENABLE 0x8000
#define PCI_MSIX_FLAGS 2
#define PCI_MSI_FLAGS 2
#define PCI_MSI_FLAGS_ENABLE 0x0001
#define PCI_COMMAND_INTX_DISABLE 0x400
#define PCI_COMMAND 0x04
#define PCI_STATUS 0x06
#define PCI_STATUS_INTERRUPT 0x08
#define PCI_MSIX_FLAGS_QSIZE 0x07FF
#define PCI_MSI_FLAGS_QMASK 0x000e
#endif

#ifndef _WINDOWS
#if defined PS3_HARDWARE_ASIC
#include "ps3_ioc_manager.h"
#endif
#endif

struct ps3_instance;

#ifdef _WINDOWS
struct ps3_pci_context {
	unsigned long long slot_number;
	unsigned short vendor_id;
	unsigned short device_id;
	unsigned short sub_vendor_id;
	unsigned short sub_device_id;
	unsigned long long bar_base_addr;
	unsigned short irq_vec_count;
	unsigned short valid_irq_count;
	unsigned char pci_irq_type;
};

int ps3_pci_init(struct ps3_instance *instance, void *config);

void ps3_pci_exit(struct ps3_instance *instance);

void ps3_pci_intx(struct ps3_instance *instance, unsigned char enable);

#endif
int ps3_pci_find_capability(struct ps3_instance *instance, int cap_id);

int ps3_pci_read_config_word(struct ps3_instance *instance, unsigned int offset,
			     unsigned short *val);

int ps3_pci_write_config_word(struct ps3_instance *instance,
			      unsigned int offset, unsigned short val);

void ps3_reg_write_u64(struct ps3_instance *instance, unsigned long long val,
		       void *reg);

unsigned long long ps3_reg_read_u64(struct ps3_instance *instance, void *reg);

void __iomem *ps3_reg_set_ioremap(struct pci_dev *pdev, unsigned long reg_bar);

#endif
