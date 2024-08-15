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

static bool attach_uld = true;
module_param(attach_uld, bool, 0444);
MODULE_PARM_DESC(attach_uld, "enable attach upper driver - default is true");

static struct sss_uld_info g_uld_info[SSS_SERVICE_TYPE_MAX];

static const char *g_uld_name[SSS_SERVICE_TYPE_MAX] = {
	"nic", "ovs", "roce", "toe", "ioe",
	"fc", "vbs", "ipsec", "virtio", "migrate", "ppa", "custom"
};

/* lock for attach/detach all uld and register/ unregister uld */
struct mutex g_uld_mutex;

void sss_init_uld_lock(void)
{
	mutex_init(&g_uld_mutex);
}

void sss_lock_uld(void)
{
	mutex_lock(&g_uld_mutex);
}

void sss_unlock_uld(void)
{
	mutex_unlock(&g_uld_mutex);
}

const char **sss_get_uld_names(void)
{
	return g_uld_name;
}

struct sss_uld_info *sss_get_uld_info(void)
{
	return g_uld_info;
}

bool sss_attach_is_enable(void)
{
	return attach_uld;
}
