// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/bitops.h>
#include <linux/netdevice.h>

#include "rnp.h"
#include "rnp_common.h"
#include "rnp_mbx.h"

unsigned int rnp_loglevel;
module_param(rnp_loglevel, uint, S_IRUSR | S_IWUSR);


