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
#include "sss_version.h"
#include "sss_adapter_mgmt.h"
#include "sss_pci_id_tbl.h"
#include "sss_pci_sriov.h"
#include "sss_pci_probe.h"
#include "sss_pci_remove.h"
#include "sss_pci_shutdown.h"
#include "sss_pci_error.h"
#include "sss_hwdev.h"

#define SSS_DRV_DESC "Intelligent Network Interface Card Driver"

MODULE_AUTHOR("steven.song@3snic.com");
MODULE_DESCRIPTION("3SNIC Network Interface Card Driver");
MODULE_VERSION(SSS_DRV_VERSION);
MODULE_LICENSE("GPL");

static const struct pci_device_id g_pci_table[] = {
	{PCI_VDEVICE(SSSNIC, SSS_DEV_ID_STANDARD), 0},
	{PCI_VDEVICE(SSSNIC, SSS_DEV_ID_SPN120), 0},
	{PCI_VDEVICE(SSSNIC, SSS_DEV_ID_VF), 0},
	{0, 0}
};

MODULE_DEVICE_TABLE(pci, g_pci_table);

#ifdef HAVE_RHEL6_SRIOV_CONFIGURE
static struct pci_driver_rh g_pci_driver_rh = {
	.sriov_configure = sss_pci_configure_sriov,
};
#endif

static struct pci_error_handlers g_pci_err_handler = {
	.error_detected = sss_detect_pci_error,
};

static struct pci_driver g_pci_driver = {
	.name		 = SSS_DRV_NAME,
	.id_table	 = g_pci_table,
	.probe		 = sss_pci_probe,
	.remove		 = sss_pci_remove,
	.shutdown	 = sss_pci_shutdown,
#if defined(HAVE_SRIOV_CONFIGURE)
	.sriov_configure = sss_pci_configure_sriov,
#elif defined(HAVE_RHEL6_SRIOV_CONFIGURE)
	.rh_reserved = &g_pci_driver_rh,
#endif
	.err_handler	 = &g_pci_err_handler
};

int sss_init_pci(void)
{
	int ret;

	pr_info("%s - version %s\n", SSS_DRV_DESC, SSS_DRV_VERSION);
	sss_pre_init();

	ret = pci_register_driver(&g_pci_driver);
	if (ret != 0)
		return ret;

	return 0;
}

void sss_exit_pci(void)
{
	pci_unregister_driver(&g_pci_driver);
}
