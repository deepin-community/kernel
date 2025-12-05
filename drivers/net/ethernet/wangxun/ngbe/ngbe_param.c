// SPDX-License-Identifier: GPL-2.0
/* WangXun Gigabit PCI Express Linux driver
 * Copyright (c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 */

#include <linux/types.h>
#include <linux/module.h>

#include "ngbe.h"

/* This is the only thing that needs to be changed to adjust the
 * maximum number of ports that the driver can manage.
 */
#define NGBE_MAX_NIC 32
#define OPTION_UNSET -1
#define OPTION_DISABLED 0
#define OPTION_ENABLED 1

#define STRINGIFY(foo) #foo /* magic for getting defines into strings */
#define XSTRINGIFY(bar) STRINGIFY(bar)

/* All parameters are treated the same, as an integer array of values.
 * This macro just reduces the need to repeat the same declaration code
 * over and over (plus this helps to avoid typo bugs).
 */

#define NGBE_PARAM_INIT {[0 ... NGBE_MAX_NIC] = OPTION_UNSET}
#define NGBE_PARAM(X, desc) \
	static int X[NGBE_MAX_NIC + 1] = NGBE_PARAM_INIT; \
	static unsigned int num_##X; \
	module_param_array_named(X, X, int, &num_##X, 0); \
	MODULE_PARM_DESC(X, desc)
/* IntMode (Interrupt Mode)
 * Valid Range: 0-2
 *  - 0 - Legacy Interrupt
 *  - 1 - MSI Interrupt
 *  - 2 - MSI-X Interrupt(s)
 * Default Value: 2
 */
NGBE_PARAM(InterruptType, "Change Interrupt Mode (0=Legacy, 1=MSI, 2=MSI-X)");
NGBE_PARAM(IntMode, "Change Interrupt Mode (0=Legacy, 1=MSI, 2=MSI-X), default 2");
#define NGBE_INT_LEGACY 0
#define NGBE_INT_MSI 1
#define NGBE_INT_MSIX 2
#define NGBE_DEFAULT_INT NGBE_INT_MSIX

/* MQ - Multiple Queue enable/disable
 * Valid Range: 0, 1
 *  - 0 - disables MQ
 *  - 1 - enables MQ
 * Default Value: 1
 */

NGBE_PARAM(MQ, "Disable or enable Multiple Queues, default 1");

/* RSS - Receive-Side Scaling (RSS) Descriptor Queues
 * Valid Range: 0-64
 *  - 0 - enables RSS and sets the Desc. Q's to min(64, num_online_cpus()).
 *  - 1-64 - enables RSS and sets the Desc. Q's to the specified value.
 * Default Value: 0
 */

NGBE_PARAM(RSS, "Number of Receive-Side Scaling Descriptor Queues");

/* VMDQ - Virtual Machine Device Queues (VMDQ)
 * Valid Range: 1-16
 *  - 1 Disables VMDQ by allocating only a single queue.
 *  - 2-16 - enables VMDQ and sets the Desc. Q's to the specified value.
 * Default Value: 1
 */

#define NGBE_DEFAULT_NUM_VMDQ 8

NGBE_PARAM(VMDQ, "Number of vmdq: 0/1 = disable, 2-16 enable (default="
	   XSTRINGIFY(NGBE_DEFAULT_NUM_VMDQ) ")");

#ifdef CONFIG_PCI_IOV
/* max_vfs - SR I/O Virtualization
 * Valid Range: 0-63
 *  - 0 Disables SR-IOV
 *  - 1-63 - enables SR-IOV and sets the number of VFs enabled
 * Default Value: 0
 */

#define MAX_SRIOV_VFS 8

NGBE_PARAM(max_vfs, "Number of Virtual Functions: 0 = disable (default), 1-"
	   XSTRINGIFY(MAX_SRIOV_VFS) " = enable this many VFs");

/* VEPA - Set internal bridge to VEPA mode
 * Valid Range: 0-1
 *  - 0 Set bridge to VEB mode
 *  - 1 Set bridge to VEPA mode
 * Default Value: 0
 */
/*Note:
 *=====
 * This provides ability to ensure VEPA mode on the internal bridge even if
 * the kernel does not support the netdev bridge setting operations.
 */
NGBE_PARAM(VEPA, "VEPA Bridge Mode: 0 = VEB (default), 1 = VEPA");
#endif

/* Interrupt Throttle Rate (interrupts/sec)
 * Valid Range: 980-500000 (0=off, 1=dynamic)
 * Default Value: 1
 */
#define DEFAULT_ITR                                        \
	((NGBE_STATIC_ITR == 0) || (NGBE_STATIC_ITR == 1) ? \
		NGBE_STATIC_ITR :                          \
		(u16)((1000000 / NGBE_STATIC_ITR) << 2))

NGBE_PARAM(InterruptThrottleRate, "Maximum itr (0,1,980-500000), default 1");
#define MAX_ITR NGBE_MAX_INT_RATE
#define MIN_ITR NGBE_MIN_INT_RATE

/* LLIPort (Low Latency Interrupt TCP Port)
 * Valid Range: 0 - 65535
 * Default Value: 0 (disabled)
 */
NGBE_PARAM(LLIPort, "Low Latency Interrupt TCP Port (0-65535)");

#define DEFAULT_LLIPORT 0
#define MAX_LLIPORT 0xFFFF
#define MIN_LLIPORT 0

/* LLISize (Low Latency Interrupt on Packet Size)
 * Valid Range: 0 - 1500
 * Default Value: 0 (disabled)
 */
NGBE_PARAM(LLISize, "Low Latency Interrupt on Packet Size (0-1500)");

#define DEFAULT_LLISIZE 0
#define MAX_LLISIZE 1500
#define MIN_LLISIZE 0

/* LLIEType (Low Latency Interrupt Ethernet Type)
 * Valid Range: 0 - 0x8fff
 * Default Value: 0 (disabled)
 */
NGBE_PARAM(LLIEType, "Low Latency Interrupt Ethernet Protocol Type");

#define DEFAULT_LLIETYPE 0
#define MAX_LLIETYPE 0x8fff
#define MIN_LLIETYPE 0

/* LLIVLANP (Low Latency Interrupt on VLAN priority threshold)
 * Valid Range: 0 - 7
 * Default Value: 0 (disabled)
 */
NGBE_PARAM(LLIVLANP, "Low Latency Interrupt on VLAN priority threshold");

#define DEFAULT_LLIVLANP 0
#define MAX_LLIVLANP 7
#define MIN_LLIVLANP 0

/* Software ATR packet sample rate
 * Valid Range: 0-255  0 = off, 1-255 = rate of Tx packet inspection
 * Default Value: 20
 */
NGBE_PARAM(AtrSampleRate, "Software ATR Tx packet sample rate");

#define NGBE_MAX_ATR_SAMPLE_RATE 255
#define NGBE_MIN_ATR_SAMPLE_RATE 1
#define NGBE_ATR_SAMPLE_RATE_OFF 0
#define NGBE_DEFAULT_ATR_SAMPLE_RATE 20

/* Enable/disable Large Receive Offload
 * Valid Values: 0(off), 1(on)
 * Default Value: 1
 */
NGBE_PARAM(LRO, "Large Receive Offload (0,1), default 1 = on");

/* Enable/disable support for DMA coalescing
 * Valid Values: 0(off), 41 - 10000(on)
 * Default Value: 0
 */
NGBE_PARAM(dmac_watchdog,
	   "DMA coalescing watchdog in microseconds (0,41-10000), default 0 = off");

/* Rx buffer mode
 * Valid Range: 0-1 0 = no header split, 1 = hdr split
 * Default Value: 0
 */
NGBE_PARAM(RxBufferMode, "0=(default)no header split\n\t\t\t1=hdr split\n");

/* motocomm phy yt8531s unidirectional mode
 * Valid Range: 0-1 0 = disable, 1 = enable
 * Default Value: 0
 */
NGBE_PARAM(YtUniMode, "0=(default)disable unidirectional mode\n\t\t\t1=enable\n");

#define NGBE_RXBUFMODE_NO_HEADER_SPLIT 0
#define NGBE_RXBUFMODE_HEADER_SPLIT 1
#define NGBE_DEFAULT_RXBUFMODE NGBE_RXBUFMODE_NO_HEADER_SPLIT

struct ngbe_option {
	enum { enable_option, range_option, list_option } type;
	const char *name;
	const char *err;
	const char *msg;
	int def;
	union {
		struct { /* range_option info */
			int min;
			int max;
		} r;
		struct { /* list_option info */
			int nr;
			const struct ngbe_opt_list {
				int i;
				char *str;
			} *p;
		} l;
	} arg;
};

static int ngbe_validate_option(u32 *value, struct ngbe_option *opt)
{
	int val = (int)*value;

	if (val == OPTION_UNSET) {
		pr_info("ngbe: Invalid %s specified (%d),  %s\n", opt->name, val, opt->err);
		*value = (u32)opt->def;
		return 0;
	}

	switch (opt->type) {
	case enable_option:
		switch (val) {
		case OPTION_ENABLED:
			pr_info("ngbe: %s Enabled\n", opt->name);
			return 0;
		case OPTION_DISABLED:
			pr_info("ngbe: %s Disabled\n", opt->name);
			return 0;
		}
		break;
	case range_option:
		if ((val >= opt->arg.r.min && val <= opt->arg.r.max) || val == opt->def) {
			if (opt->msg)
				pr_info("ngbe: %s set to %d, %s\n", opt->name, val, opt->msg);
			else
				pr_info("ngbe: %s set to %d\n", opt->name, val);
			return 0;
		}
		break;
	case list_option: {
		int i;
		const struct ngbe_opt_list *ent;

		for (i = 0; i < opt->arg.l.nr; i++) {
			ent = &opt->arg.l.p[i];
			if (val == ent->i) {
				if (ent->str[0] != '\0')
					pr_info("%s\n", ent->str);
				return 0;
			}
		}
	} break;
	default:
		WARN_ON_ONCE(1);
	}

	pr_info("ngbe: Invalid %s specified (%d),  %s\n", opt->name, val, opt->err);
	*value = (u32)opt->def;
	return -1;
}

/**
 * ngbe_check_options - Range Checking for Command Line Parameters
 * @adapter: board private structure
 * This routine checks all command line parameters for valid user
 * input.  If an invalid value is given, or if no user specified
 * value exists, a default value is used.  The final value is stored
 * in a variable in the adapter structure.
 **/
void ngbe_check_options(struct ngbe_adapter *adapter)
{
	u32 bd = adapter->bd_number;
	u32 *aflags = &adapter->flags;
	struct ngbe_ring_feature *feature = adapter->ring_feature;
	u32 vmdq;

	if (bd >= NGBE_MAX_NIC) {
		pr_notice("Warning: no configuration for board #%d\n", bd);
		pr_notice("Using defaults for all values\n");
	}

	{ /* Interrupt Mode */
		u32 int_mode;
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "Interrupt Mode",
			.err = "using default of " __MODULE_STRING(NGBE_DEFAULT_INT),
			.def = NGBE_DEFAULT_INT,
			.arg = {.r = {.min = NGBE_INT_LEGACY, .max = NGBE_INT_MSIX}}};
		if (num_IntMode > bd || num_InterruptType > bd) {
			int_mode = IntMode[bd];
			if (int_mode == OPTION_UNSET)
				int_mode = InterruptType[bd];
			ngbe_validate_option(&int_mode, &opt);
			switch (int_mode) {
			case NGBE_INT_MSIX:
				if (!(*aflags & NGBE_FLAG_MSIX_CAPABLE))
					pr_info("Ignoring MSI-X setting; support unavailable\n");
				break;
			case NGBE_INT_MSI:
				if (!(*aflags & NGBE_FLAG_MSI_CAPABLE))
					pr_info("Ignoring MSI setting; support unavailable\n");
				else
					*aflags &= ~NGBE_FLAG_MSIX_CAPABLE;
				break;
			case NGBE_INT_LEGACY:
			default:
				*aflags &= ~NGBE_FLAG_MSIX_CAPABLE;
				*aflags &= ~NGBE_FLAG_MSI_CAPABLE;
				break;
			}
		} else {
			/* default settings */
			if (opt.def == NGBE_INT_MSIX &&
			    *aflags & NGBE_FLAG_MSIX_CAPABLE) {
				*aflags |= NGBE_FLAG_MSIX_CAPABLE;
				*aflags |= NGBE_FLAG_MSI_CAPABLE;
			} else if (opt.def == NGBE_INT_MSI &&
			    *aflags & NGBE_FLAG_MSI_CAPABLE) {
				*aflags &= ~NGBE_FLAG_MSIX_CAPABLE;
				*aflags |= NGBE_FLAG_MSI_CAPABLE;
			} else {
				*aflags &= ~NGBE_FLAG_MSIX_CAPABLE;
				*aflags &= ~NGBE_FLAG_MSI_CAPABLE;
			}
		}
	}
	{ /* Multiple Queue Support */
		static struct ngbe_option opt = {
			.type = enable_option,
			.name = "Multiple Queue Support",
			.err = "defaulting to Enabled",
			.def = OPTION_ENABLED
		};
		if (num_MQ > bd) {
			u32 mq = MQ[bd];

			ngbe_validate_option(&mq, &opt);
			if (mq)
				*aflags |= NGBE_FLAG_MQ_CAPABLE;
			else
				*aflags &= ~NGBE_FLAG_MQ_CAPABLE;
		} else {
			if (opt.def == OPTION_ENABLED)
				*aflags |= NGBE_FLAG_MQ_CAPABLE;
			else
				*aflags &= ~NGBE_FLAG_MQ_CAPABLE;
		}
		/* Check Interoperability */
		if ((*aflags & NGBE_FLAG_MQ_CAPABLE) &&
		    !(*aflags & NGBE_FLAG_MSIX_CAPABLE)) {
			e_info(probe, "Multiple queues are not supported while MSI-X disabled.\n");
			*aflags &= ~NGBE_FLAG_MQ_CAPABLE;
		}
	}

	{ /* Receive-Side Scaling (RSS) */
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "Receive-Side Scaling (RSS)",
			.err = "using default.",
			.def = 0,
			.arg = { .r = { .min = 0, .max = 1 } }
		};
		u32 rss = min_t(int, ngbe_max_rss_indices(adapter),
				num_online_cpus());
		/* adjust Max allowed RSS queues based on MAC type */
		opt.arg.r.max = min_t(int, ngbe_max_rss_indices(adapter),
				      num_online_cpus());
		if (num_RSS > bd) {
			rss = RSS[bd];
			ngbe_validate_option(&rss, &opt);
			/* base it off num_online_cpus() with hardware limit */
			if (!rss)
				rss = min_t(int, opt.arg.r.max, num_online_cpus());

			feature[RING_F_RSS].limit = (u16)rss;
		} else if (opt.def == 0) {
			rss = min_t(int, ngbe_max_rss_indices(adapter),
				    num_online_cpus());
			feature[RING_F_RSS].limit = rss;
		}
		/* Check Interoperability */
		if (rss > 1) {
			if (!(*aflags & NGBE_FLAG_MQ_CAPABLE)) {
				e_info(probe, "Multiqueue is disabled. Limiting RSS.\n");
				feature[RING_F_RSS].limit = 1;
			}
		}
		adapter->flags2 |= NGBE_FLAG2_RSS_ENABLED;
	}
	{ /* Virtual Machine Device Queues (VMDQ) */
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "Virtual Machine Device Queues (VMDQ)",
			.err = "defaulting to Disabled",
			.def = OPTION_DISABLED,
			.arg = { .r = { .min = OPTION_DISABLED,
					.max = NGBE_MAX_VMDQ_INDICES } }
		};
		if (num_VMDQ > bd) {
			vmdq = VMDQ[bd];

			ngbe_validate_option(&vmdq, &opt);

			/* zero or one both mean disabled from our driver's
			 * perspective
			 */
			if (vmdq > 1)
				*aflags |= NGBE_FLAG_VMDQ_ENABLED;
			else
				*aflags &= ~NGBE_FLAG_VMDQ_ENABLED;

			feature[RING_F_VMDQ].limit = (u16)vmdq;
		} else {
			if (opt.def == OPTION_DISABLED)
				*aflags &= ~NGBE_FLAG_VMDQ_ENABLED;
			else
				*aflags |= NGBE_FLAG_VMDQ_ENABLED;

			feature[RING_F_VMDQ].limit = opt.def;
		}
		/* Check Interoperability */
		if (*aflags & NGBE_FLAG_VMDQ_ENABLED) {
			if (!(*aflags & NGBE_FLAG_MQ_CAPABLE)) {
				e_info(probe, "VMDQ is disable when multiple queues disabled.\n");
				*aflags &= ~NGBE_FLAG_VMDQ_ENABLED;
				feature[RING_F_VMDQ].limit = 0;
			}
		}
	}
#ifdef CONFIG_PCI_IOV
	{ /* Single Root I/O Virtualization (SR-IOV) */
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "I/O Virtualization (IOV)",
			.err = "defaulting to Disabled",
			.def = OPTION_DISABLED,
			.arg = { .r = { .min = OPTION_DISABLED,
					.max = MAX_SRIOV_VFS } }
		};
		if (num_max_vfs > bd) {
			u32 vfs = max_vfs[bd];

			if (ngbe_validate_option(&vfs, &opt)) {
				vfs = 0;
				e_info(probe, "max_vfs out of range Disabling SR-IOV.\n");
			}

			adapter->max_vfs = vfs;

			if (vfs)
				*aflags |= NGBE_FLAG_SRIOV_ENABLED;
			else
				*aflags &= ~NGBE_FLAG_SRIOV_ENABLED;
		} else {
			if (opt.def == OPTION_DISABLED) {
				adapter->max_vfs = 0;
				*aflags &= ~NGBE_FLAG_SRIOV_ENABLED;
			} else {
				adapter->max_vfs = opt.def;
				*aflags |= NGBE_FLAG_SRIOV_ENABLED;
			}
		}
		/* Check Interoperability */
		if (*aflags & NGBE_FLAG_SRIOV_ENABLED) {
			if (!(*aflags & NGBE_FLAG_SRIOV_CAPABLE)) {
				e_info(probe,
				       "IOV is not supported on this hardware.\n");
				*aflags &= ~NGBE_FLAG_SRIOV_ENABLED;
				adapter->max_vfs = 0;
			} else if (!(*aflags & NGBE_FLAG_MQ_CAPABLE)) {
				e_info(probe,
				       "IOV is disabled while multiple queues are disabled.\n");
				*aflags &= ~NGBE_FLAG_SRIOV_ENABLED;
				adapter->max_vfs = 0;
			}
		}
	}
	{ /* VEPA Bridge Mode enable for SR-IOV mode */
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "VEPA Bridge Mode Enable",
			.err = "defaulting to disabled",
			.def = OPTION_DISABLED,
			.arg = { .r = { .min = OPTION_DISABLED,
					.max = OPTION_ENABLED } }
		};
		if (num_VEPA > bd) {
			u32 vepa = VEPA[bd];

			ngbe_validate_option(&vepa, &opt);
			if (vepa)
				adapter->flags |= NGBE_FLAG_SRIOV_VEPA_BRIDGE_MODE;
		} else {
			if (opt.def == OPTION_ENABLED)
				adapter->flags |=
					NGBE_FLAG_SRIOV_VEPA_BRIDGE_MODE;
		}
	}
#endif /* CONFIG_PCI_IOV */
	{ /* Interrupt Throttling Rate */
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "Interrupt Throttling Rate (ints/sec)",
			.err = "using default of " __MODULE_STRING(DEFAULT_ITR),
			.def = DEFAULT_ITR,
			.arg = { .r = { .min = MIN_ITR, .max = MAX_ITR } }
		};
		if (num_InterruptThrottleRate > bd) {
			u32 itr = InterruptThrottleRate[bd];

			switch (itr) {
			case 0:
				e_info(probe, "%s turned off\n", opt.name);
				adapter->rx_itr_setting = 0;
				break;
			case 1:
				e_info(probe, "dynamic interrupt throttling enabled\n");
				adapter->rx_itr_setting = 1;
				break;
			default:
				ngbe_validate_option(&itr, &opt);
				/* the first bit is used as control */
				adapter->rx_itr_setting = (u16)((1000000 / itr) << 2);
				break;
			}
			adapter->tx_itr_setting = adapter->rx_itr_setting;
		} else {
			adapter->rx_itr_setting = opt.def;
			adapter->tx_itr_setting = opt.def;
		}
	}
	{ /* Low Latency Interrupt TCP Port*/
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "Low Latency Interrupt TCP Port",
			.err = "using default of " __MODULE_STRING(DEFAULT_LLIPORT),
			.def = DEFAULT_LLIPORT,
			.arg = {.r = {.min = MIN_LLIPORT, .max = MAX_LLIPORT}}};
		if (num_LLIPort > bd) {
			adapter->lli_port = LLIPort[bd];
			if (adapter->lli_port)
				ngbe_validate_option(&adapter->lli_port, &opt);
			else
				e_info(probe, "%s turned off\n", opt.name);
		} else {
			adapter->lli_port = opt.def;
		}
	}
	{ /* Low Latency Interrupt on Packet Size */
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "Low Latency Interrupt on Packet Size",
			.err = "using default of " __MODULE_STRING(DEFAULT_LLISIZE),
			.def = DEFAULT_LLISIZE,
			.arg = {.r = {.min = MIN_LLISIZE, .max = MAX_LLISIZE}}};
		if (num_LLISize > bd) {
			adapter->lli_size = LLISize[bd];
			if (adapter->lli_size)
				ngbe_validate_option(&adapter->lli_size, &opt);
			else
				e_info(probe, "%s turned off\n", opt.name);
		} else {
			adapter->lli_size = opt.def;
		}
	}
	{ /* Low Latency Interrupt EtherType*/
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "Low Latency Interrupt on Ethernet Protocol Type",
			.err = "using default of " __MODULE_STRING(DEFAULT_LLIETYPE),
			.def = DEFAULT_LLIETYPE,
			.arg = {.r = {.min = MIN_LLIETYPE, .max = MAX_LLIETYPE}}};
		if (num_LLIEType > bd) {
			adapter->lli_etype = LLIEType[bd];
			if (adapter->lli_etype)
				ngbe_validate_option(&adapter->lli_etype, &opt);
			else
				e_info(probe, "%s turned off\n", opt.name);
		} else {
			adapter->lli_etype = opt.def;
		}
	}
	{ /* LLI VLAN Priority */
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "Low Latency Interrupt on VLAN priority threshold",
			.err = "using default of " __MODULE_STRING(DEFAULT_LLIVLANP),
			.def = DEFAULT_LLIVLANP,
			.arg = {.r = {.min = MIN_LLIVLANP, .max = MAX_LLIVLANP}}};
		if (num_LLIVLANP > bd) {
			adapter->lli_vlan_pri = LLIVLANP[bd];
			if (adapter->lli_vlan_pri)
				ngbe_validate_option(&adapter->lli_vlan_pri, &opt);
			else
				e_info(probe, "%s turned off\n", opt.name);
		} else {
			adapter->lli_vlan_pri = opt.def;
		}
	}
	{ /* Flow Director ATR Tx sample packet rate */
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "Software ATR Tx packet sample rate",
			.err = "using default of " __MODULE_STRING(NGBE_DEFAULT_ATR_SAMPLE_RATE),
			.def = NGBE_DEFAULT_ATR_SAMPLE_RATE,
			.arg = {.r = {.min = NGBE_ATR_SAMPLE_RATE_OFF,
				      .max = NGBE_MAX_ATR_SAMPLE_RATE}}};
		static const char atr_string[] =
			"ATR Tx Packet sample rate set to";

		if (num_AtrSampleRate > bd) {
			adapter->atr_sample_rate = AtrSampleRate[bd];

			if (adapter->atr_sample_rate) {
				ngbe_validate_option(&adapter->atr_sample_rate,
						     &opt);
				e_info(probe, "%s %d\n", atr_string, adapter->atr_sample_rate);
			}
		} else {
			adapter->atr_sample_rate = opt.def;
		}
	}
	{ /* LRO - Set Large Receive Offload */
		struct ngbe_option opt = {
			.type = enable_option,
			.name = "LRO - Large Receive Offload",
			.err = "defaulting to Disabled",
			/* lro switch to ON when run on SW and FT platform */
			/* emerald temp setting */
			.def = OPTION_DISABLED
		};
		struct net_device *netdev = adapter->netdev;

		opt.def = OPTION_DISABLED;
		if (num_LRO > bd) {
			u32 lro = LRO[bd];

			ngbe_validate_option(&lro, &opt);
			if (lro)
				netdev->features |= NETIF_F_LRO;
			else
				netdev->features &= ~NETIF_F_LRO;
		} else if (opt.def == OPTION_ENABLED) {
			netdev->features |= NETIF_F_LRO;
		} else {
			netdev->features &= ~NETIF_F_LRO;
		}
		if ((netdev->features & NETIF_F_LRO)) {
			e_info(probe, "RSC is not supported on this hardware.  Disabling RSC.\n");
			netdev->features &= ~NETIF_F_LRO;
		}
	}
	{ /* DMA Coalescing */
		struct ngbe_option opt = {
			.type = range_option,
			.name = "dmac_watchdog",
			.err = "defaulting to 0 (disabled)",
			.def = 0,
			.arg = { .r = { .min = 41, .max = 10000 } },
		};
		const char *cmsg = "DMA coalescing not supported on this hardware";

		opt.err = cmsg;
		opt.msg = cmsg;
		opt.arg.r.min = 0;
		opt.arg.r.max = 0;
		if (num_dmac_watchdog > bd) {
			u32 dmac_wd = dmac_watchdog[bd];

			ngbe_validate_option(&dmac_wd, &opt);
			adapter->hw.mac.dmac_config.watchdog_timer = (u16)dmac_wd;
		} else {
			adapter->hw.mac.dmac_config.watchdog_timer = opt.def;
		}
	}

	{ /* Rx buffer mode */
		u32 rx_buf_mode;
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "Rx buffer mode",
			.err = "using default of " __MODULE_STRING(NGBE_DEFAULT_RXBUFMODE),
			.def = NGBE_DEFAULT_RXBUFMODE,
			.arg = {.r = {.min = NGBE_RXBUFMODE_NO_HEADER_SPLIT,
				      .max = NGBE_RXBUFMODE_HEADER_SPLIT}}

		};
		if (num_RxBufferMode > bd) {
			rx_buf_mode = RxBufferMode[bd];
			ngbe_validate_option(&rx_buf_mode, &opt);
			switch (rx_buf_mode) {
			case NGBE_RXBUFMODE_NO_HEADER_SPLIT:
				*aflags &= ~NGBE_FLAG_RX_HS_ENABLED;
				break;
			case NGBE_RXBUFMODE_HEADER_SPLIT:
				*aflags |= NGBE_FLAG_RX_HS_ENABLED;
				break;
			default:
				break;
			}
		} else {
			*aflags &= ~NGBE_FLAG_RX_HS_ENABLED;
		}
	}
	{ /* motocomm phy yt8531s unidirectional mode */
		static struct ngbe_option opt = {
			.type = enable_option,
			.name = "motocomm phy yt8531s unidirectional mode",
			.err = "using default.",
			.def = OPTION_DISABLED,
		};
		if (num_YtUniMode > bd) {
			u32 yt_uni_mode = YtUniMode[bd];

			ngbe_validate_option(&yt_uni_mode, &opt);
			adapter->hw.phy.yt_unidirectional = (u16)yt_uni_mode;
		} else {
			adapter->hw.phy.yt_unidirectional = opt.def;
		}
	}
}
