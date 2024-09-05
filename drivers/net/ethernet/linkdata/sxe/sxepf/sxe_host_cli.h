
#ifndef __SXE_HOST_CLI_H__
#define __SXE_HOST_CLI_H__

#include <linux/device.h>
#include <linux/pci.h>
#include <linux/cdev.h>

#include "sxe.h"
#include "sxe_cli.h"
#include "sxe_msg.h"
#include "drv_msg.h"

#define SXE_CHRDEV_NAME        "sxe-cli"
#define SXE_MAX_DEVICES_NUM    (1U << MINORBITS)  
#define SXE_CHRDEV_CLASS_NAME  SXE_CHRDEV_NAME

struct sxe_pci_addr {
	s32 domain;
	u8 bus;
	u32 deviceNo;
	u32 devfn;
};

s32 sxe_cli_cdev_register(void);

void sxe_cli_cdev_unregister(void);

s32 sxe_cli_cdev_create(struct sxe_adapter *adapter);

void sxe_cli_cdev_delete(struct sxe_adapter *adapter);

#endif
