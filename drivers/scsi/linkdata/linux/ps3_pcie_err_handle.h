
#ifndef _PS3_PCIE_ERR_HANDLE_H_
#define _PS3_PCIE_ERR_HANDLE_H_

#include <linux/pci.h>
#include <linux/aer.h>

#include "ps3_instance_manager.h"
#ifdef __cplusplus
extern "C"{
#endif

void ps3_pci_err_handler_init(struct pci_driver *drv);

int ps3_base_init_resources(struct ps3_instance *instance);

void ps3_base_free_resources(struct ps3_instance *instance);

#ifdef __cplusplus
}
#endif

#endif
