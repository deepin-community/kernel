/*
 * WangXun Gigabit PCI Express Linux driver
 * Copyright (c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 */


#include <linux/types.h>
#include <linux/module.h>

#include "ngbe.h"

/* This is the only thing that needs to be changed to adjust the
 * maximum number of ports that the driver can manage.
 */
#define NGBE_MAX_NIC   32
#define OPTION_UNSET    -1
#define OPTION_DISABLED 0
#define OPTION_ENABLED  1

#define STRINGIFY(foo)  #foo /* magic for getting defines into strings */
#define XSTRINGIFY(bar) STRINGIFY(bar)

/* All parameters are treated the same, as an integer array of values.
 * This macro just reduces the need to repeat the same declaration code
 * over and over (plus this helps to avoid typo bugs).
 */

#define NGBE_PARAM_INIT { [0 ... NGBE_MAX_NIC] = OPTION_UNSET }
#ifndef module_param_array
/* Module Parameters are always initialized to -1, so that the driver
 * can tell the difference between no user specified value or the
 * user asking for the default value.
 * The true default values are loaded in when ngbe_check_options is called.
 *
 * This is a GCC extension to ANSI C.
 * See the item "Labelled Elements in Initializers" in the section
 * "Extensions to the C Language Family" of the GCC documentation.
 */

#define NGBE_PARAM(X, desc) \
	static const int __devinitconst X[NGBE_MAX_NIC+1] = NGBE_PARAM_INIT; \
	MODULE_PARM(X, "1-" __MODULE_STRING(NGBE_MAX_NIC) "i"); \
	MODULE_PARM_DESC(X, desc);
#else /* !module_param_array */
#define NGBE_PARAM(X, desc) \
	static int __devinitdata X[NGBE_MAX_NIC+1] = NGBE_PARAM_INIT; \
	static unsigned int num_##X; \
	module_param_array_named(X, X, int, &num_##X, 0); \
	MODULE_PARM_DESC(X, desc);
#endif /* module_param_array */

/* IntMode (Interrupt Mode)
 *
 * Valid Range: 0-2
 *  - 0 - Legacy Interrupt
 *  - 1 - MSI Interrupt
 *  - 2 - MSI-X Interrupt(s)
 *
 * Default Value: 2
 */
NGBE_PARAM(InterruptType, "Change Interrupt Mode (0=Legacy, 1=MSI, 2=MSI-X), "
	    "default IntMode (deprecated)");
NGBE_PARAM(IntMode, "Change Interrupt Mode (0=Legacy, 1=MSI, 2=MSI-X), "
	    "default 2");
#define NGBE_INT_LEGACY                0
#define NGBE_INT_MSI                   1
#define NGBE_INT_MSIX                  2
#define NGBE_DEFAULT_INT               NGBE_INT_MSIX

/* MQ - Multiple Queue enable/disable
 *
 * Valid Range: 0, 1
 *  - 0 - disables MQ
 *  - 1 - enables MQ
 *
 * Default Value: 1
 */

NGBE_PARAM(MQ, "Disable or enable Multiple Queues, default 1");


/* RSS - Receive-Side Scaling (RSS) Descriptor Queues
 *
 * Valid Range: 0-64
 *  - 0 - enables RSS and sets the Desc. Q's to min(64, num_online_cpus()).
 *  - 1-64 - enables RSS and sets the Desc. Q's to the specified value.
 *
 * Default Value: 0
 */

NGBE_PARAM(RSS, "Number of Receive-Side Scaling Descriptor Queues, "
	    "default 0=number of cpus");

/* VMDQ - Virtual Machine Device Queues (VMDQ)
 *
 * Valid Range: 1-16
 *  - 1 Disables VMDQ by allocating only a single queue.
 *  - 2-16 - enables VMDQ and sets the Desc. Q's to the specified value.
 *
 * Default Value: 1
 */

#define NGBE_DEFAULT_NUM_VMDQ 8

NGBE_PARAM(VMDQ, "Number of Virtual Machine Device Queues: 0/1 = disable, "
	    "2-16 enable (default=" XSTRINGIFY(NGBE_DEFAULT_NUM_VMDQ) ")");

#ifdef CONFIG_PCI_IOV
/* max_vfs - SR I/O Virtualization
 *
 * Valid Range: 0-63
 *  - 0 Disables SR-IOV
 *  - 1-63 - enables SR-IOV and sets the number of VFs enabled
 *
 * Default Value: 0
 */

#define MAX_SRIOV_VFS 8

NGBE_PARAM(max_vfs, "Number of Virtual Functions: 0 = disable (default), "
	    "1-" XSTRINGIFY(MAX_SRIOV_VFS) " = enable "
	    "this many VFs");

/* VEPA - Set internal bridge to VEPA mode
 *
 * Valid Range: 0-1
 *  - 0 Set bridge to VEB mode
 *  - 1 Set bridge to VEPA mode
 *
 * Default Value: 0
 */
/*
 *Note:
 *=====
 * This provides ability to ensure VEPA mode on the internal bridge even if
 * the kernel does not support the netdev bridge setting operations.
*/
NGBE_PARAM(VEPA, "VEPA Bridge Mode: 0 = VEB (default), 1 = VEPA");
#endif

/* Interrupt Throttle Rate (interrupts/sec)
 *
 * Valid Range: 980-500000 (0=off, 1=dynamic)
 *
 * Default Value: 1
 */
#define DEFAULT_ITR             (NGBE_STATIC_ITR == 0) || \
	(NGBE_STATIC_ITR == 1)?NGBE_STATIC_ITR:(u16)((1000000/NGBE_STATIC_ITR) << 2)

NGBE_PARAM(InterruptThrottleRate, "Maximum interrupts per second, per vector, "
	    "(0,1,980-500000), default 1");
#define MAX_ITR         NGBE_MAX_INT_RATE
#define MIN_ITR         NGBE_MIN_INT_RATE

#ifndef NGBE_NO_LLI

/* LLIPort (Low Latency Interrupt TCP Port)
 *
 * Valid Range: 0 - 65535
 *
 * Default Value: 0 (disabled)
 */
NGBE_PARAM(LLIPort, "Low Latency Interrupt TCP Port (0-65535)");

#define DEFAULT_LLIPORT         0
#define MAX_LLIPORT             0xFFFF
#define MIN_LLIPORT             0


/* LLISize (Low Latency Interrupt on Packet Size)
 *
 * Valid Range: 0 - 1500
 *
 * Default Value: 0 (disabled)
 */
NGBE_PARAM(LLISize, "Low Latency Interrupt on Packet Size (0-1500)");

#define DEFAULT_LLISIZE         0
#define MAX_LLISIZE             1500
#define MIN_LLISIZE             0

/* LLIEType (Low Latency Interrupt Ethernet Type)
 *
 * Valid Range: 0 - 0x8fff
 *
 * Default Value: 0 (disabled)
 */
NGBE_PARAM(LLIEType, "Low Latency Interrupt Ethernet Protocol Type");

#define DEFAULT_LLIETYPE        0
#define MAX_LLIETYPE            0x8fff
#define MIN_LLIETYPE            0

/* LLIVLANP (Low Latency Interrupt on VLAN priority threshold)
 *
 * Valid Range: 0 - 7
 *
 * Default Value: 0 (disabled)
 */
NGBE_PARAM(LLIVLANP, "Low Latency Interrupt on VLAN priority threshold");

#define DEFAULT_LLIVLANP        0
#define MAX_LLIVLANP            7
#define MIN_LLIVLANP            0

#endif /* NGBE_NO_LLI */
#ifdef HAVE_TX_MQ
/* Software ATR packet sample rate
 *
 * Valid Range: 0-255  0 = off, 1-255 = rate of Tx packet inspection
 *
 * Default Value: 20
 */
NGBE_PARAM(AtrSampleRate, "Software ATR Tx packet sample rate");

#define NGBE_MAX_ATR_SAMPLE_RATE       255
#define NGBE_MIN_ATR_SAMPLE_RATE       1
#define NGBE_ATR_SAMPLE_RATE_OFF       0
#define NGBE_DEFAULT_ATR_SAMPLE_RATE   20
#endif /* HAVE_TX_MQ */

/* Enable/disable Large Receive Offload
 *
 * Valid Values: 0(off), 1(on)
 *
 * Default Value: 1
 */
NGBE_PARAM(LRO, "Large Receive Offload (0,1), default 1 = on");

/* Enable/disable support for DMA coalescing
 *
 * Valid Values: 0(off), 41 - 10000(on)
 *
 * Default Value: 0
 */
NGBE_PARAM(dmac_watchdog,
	    "DMA coalescing watchdog in microseconds (0,41-10000),"
	    "default 0 = off");

/* Rx buffer mode
 *
 * Valid Range: 0-1 0 = no header split, 1 = hdr split
 *
 * Default Value: 0
 */
NGBE_PARAM(RxBufferMode, "0=(default)no header split\n"
			  "\t\t\t1=hdr split for recognized packet\n");

#define NGBE_RXBUFMODE_NO_HEADER_SPLIT                 0
#define NGBE_RXBUFMODE_HEADER_SPLIT                    1
#define NGBE_DEFAULT_RXBUFMODE   NGBE_RXBUFMODE_NO_HEADER_SPLIT


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

static int __devinit ngbe_validate_option(u32 *value,
					   struct ngbe_option *opt)
{
	int val = (int)*value;

	if (val == OPTION_UNSET) {
		ngbe_info("ngbe: Invalid %s specified (%d),  %s\n",
			opt->name, val, opt->err);
		*value = (u32)opt->def;
		return 0;
	}

	switch (opt->type) {
	case enable_option:
		switch (val) {
		case OPTION_ENABLED:
			ngbe_info("ngbe: %s Enabled\n", opt->name);
			return 0;
		case OPTION_DISABLED:
			ngbe_info("ngbe: %s Disabled\n", opt->name);
			return 0;
		}
		break;
	case range_option:
		if ((val >= opt->arg.r.min && val <= opt->arg.r.max) ||
			val == opt->def) {
			if (opt->msg)
				ngbe_info("ngbe: %s set to %d, %s\n",
				       opt->name, val, opt->msg);
			else
				ngbe_info("ngbe: %s set to %d\n",
				       opt->name, val);
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
					ngbe_info("%s\n", ent->str);
				return 0;
			}
		}
	}
		break;
	default:
		BUG_ON(1);
	}

	ngbe_info("ngbe: Invalid %s specified (%d),  %s\n",
			opt->name, val, opt->err);
	*value = (u32)opt->def;
	return -1;
}

/**
 * ngbe_check_options - Range Checking for Command Line Parameters
 * @adapter: board private structure
 *
 * This routine checks all command line parameters for valid user
 * input.  If an invalid value is given, or if no user specified
 * value exists, a default value is used.  The final value is stored
 * in a variable in the adapter structure.
 **/
void __devinit ngbe_check_options(struct ngbe_adapter *adapter)
{
	u32 bd = adapter->bd_number;
	u32 *aflags = &adapter->flags;
	struct ngbe_ring_feature *feature = adapter->ring_feature;
	u32 vmdq;

	if (bd >= NGBE_MAX_NIC) {
		ngbe_notice("Warning: no configuration for board #%d\n", bd);
		ngbe_notice("Using defaults for all values\n");
#ifndef module_param_array
		bd = NGBE_MAX_NIC;
#endif
	}

	{ /* Interrupt Mode */
		u32 int_mode;
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "Interrupt Mode",
			.err =
			  "using default of "__MODULE_STRING(NGBE_DEFAULT_INT),
			.def = NGBE_DEFAULT_INT,
			.arg = { .r = { .min = NGBE_INT_LEGACY,
					.max = NGBE_INT_MSIX} }
		};

#ifdef module_param_array
		if (num_IntMode > bd || num_InterruptType > bd) {
#endif
			int_mode = IntMode[bd];
			if (int_mode == OPTION_UNSET)
				int_mode = InterruptType[bd];
			ngbe_validate_option(&int_mode, &opt);
			switch (int_mode) {
			case NGBE_INT_MSIX:
				if (!(*aflags & NGBE_FLAG_MSIX_CAPABLE))
					ngbe_info(
							"Ignoring MSI-X setting; "
							"support unavailable\n");
				break;
			case NGBE_INT_MSI:
				if (!(*aflags & NGBE_FLAG_MSI_CAPABLE)) {
					ngbe_info(
							"Ignoring MSI setting; "
							"support unavailable\n");
				} else {
					*aflags &= ~NGBE_FLAG_MSIX_CAPABLE;
				}
				break;
			case NGBE_INT_LEGACY:
			default:
				*aflags &= ~NGBE_FLAG_MSIX_CAPABLE;
				*aflags &= ~NGBE_FLAG_MSI_CAPABLE;
				break;
			}
#ifdef module_param_array
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
#endif
	}
	{ /* Multiple Queue Support */
		static struct ngbe_option opt = {
			.type = enable_option,
			.name = "Multiple Queue Support",
			.err  = "defaulting to Enabled",
			.def  = OPTION_ENABLED
		};

#ifdef module_param_array
		if (num_MQ > bd) {
#endif
			u32 mq = MQ[bd];
			ngbe_validate_option(&mq, &opt);
			if (mq)
				*aflags |= NGBE_FLAG_MQ_CAPABLE;
			else
				*aflags &= ~NGBE_FLAG_MQ_CAPABLE;
#ifdef module_param_array
		} else {
			if (opt.def == OPTION_ENABLED)
				*aflags |= NGBE_FLAG_MQ_CAPABLE;
			else
				*aflags &= ~NGBE_FLAG_MQ_CAPABLE;
		}
#endif
		/* Check Interoperability */
		if ((*aflags & NGBE_FLAG_MQ_CAPABLE) &&
		    !(*aflags & NGBE_FLAG_MSIX_CAPABLE)) {
			DPRINTK(PROBE, INFO,
				"Multiple queues are not supported while MSI-X "
				"is disabled.  Disabling Multiple Queues.\n");
			*aflags &= ~NGBE_FLAG_MQ_CAPABLE;
		}
	}

	{ /* Receive-Side Scaling (RSS) */
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "Receive-Side Scaling (RSS)",
			.err  = "using default.",
			.def  = 0,
			.arg  = { .r = { .min = 0,
					 .max = 1} }
		};
		u32 rss = RSS[bd];
		/* adjust Max allowed RSS queues based on MAC type */
		opt.arg.r.max = ngbe_max_rss_indices(adapter);

#ifdef module_param_array
		if (num_RSS > bd) {
#endif
			ngbe_validate_option(&rss, &opt);
			/* base it off num_online_cpus() with hardware limit */
			if (!rss)
				rss = min_t(int, opt.arg.r.max,
					    num_online_cpus());

			feature[RING_F_RSS].limit = (u16)rss;
#ifdef module_param_array
		} else if (opt.def == 0) {
			rss = min_t(int, ngbe_max_rss_indices(adapter),
				    num_online_cpus());
			feature[RING_F_RSS].limit = rss;
		}
#endif
		/* Check Interoperability */
		if (rss > 1) {
			if (!(*aflags & NGBE_FLAG_MQ_CAPABLE)) {
				DPRINTK(PROBE, INFO,
					"Multiqueue is disabled.  "
					"Limiting RSS.\n");
				feature[RING_F_RSS].limit = 1;
			}
		}
		adapter->flags2 |= NGBE_FLAG2_RSS_ENABLED;
	}
	{ /* Virtual Machine Device Queues (VMDQ) */
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "Virtual Machine Device Queues (VMDQ)",
			.err  = "defaulting to Disabled",
			.def  = OPTION_DISABLED,
			.arg  = { .r = { .min = OPTION_DISABLED,
					 .max = NGBE_MAX_VMDQ_INDICES
				} }
		};

#ifdef module_param_array
		if (num_VMDQ > bd) {
#endif
			vmdq = VMDQ[bd];

			ngbe_validate_option(&vmdq, &opt);

			/* zero or one both mean disabled from our driver's
			 * perspective */
			if (vmdq > 1) {
				*aflags |= NGBE_FLAG_VMDQ_ENABLED;
			} else
				*aflags &= ~NGBE_FLAG_VMDQ_ENABLED;

			feature[RING_F_VMDQ].limit = (u16)vmdq;
#ifdef module_param_array
		} else {
			if (opt.def == OPTION_DISABLED)
				*aflags &= ~NGBE_FLAG_VMDQ_ENABLED;
			else
				*aflags |= NGBE_FLAG_VMDQ_ENABLED;

			feature[RING_F_VMDQ].limit = opt.def;
		}
#endif
		/* Check Interoperability */
		if (*aflags & NGBE_FLAG_VMDQ_ENABLED) {
			if (!(*aflags & NGBE_FLAG_MQ_CAPABLE)) {
				DPRINTK(PROBE, INFO,
					"VMDQ is not supported while multiple "
					"queues are disabled.  "
					"Disabling VMDQ.\n");
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
			.err  = "defaulting to Disabled",
			.def  = OPTION_DISABLED,
			.arg  = { .r = { .min = OPTION_DISABLED,
					 .max = MAX_SRIOV_VFS} }
		};

#ifdef module_param_array
		if (num_max_vfs > bd) {
#endif
			u32 vfs = max_vfs[bd];
			if (ngbe_validate_option(&vfs, &opt)) {
				vfs = 0;
				DPRINTK(PROBE, INFO,
					"max_vfs out of range "
					"Disabling SR-IOV.\n");
			}

			adapter->num_vfs = vfs;

			if (vfs)
				*aflags |= NGBE_FLAG_SRIOV_ENABLED;
			else
				*aflags &= ~NGBE_FLAG_SRIOV_ENABLED;
#ifdef module_param_array
		} else {
			if (opt.def == OPTION_DISABLED) {
				adapter->num_vfs = 0;
				*aflags &= ~NGBE_FLAG_SRIOV_ENABLED;
			} else {
				adapter->num_vfs = opt.def;
				*aflags |= NGBE_FLAG_SRIOV_ENABLED;
			}
		}
#endif

		/* Check Interoperability */
		if (*aflags & NGBE_FLAG_SRIOV_ENABLED) {
			if (!(*aflags & NGBE_FLAG_SRIOV_CAPABLE)) {
				DPRINTK(PROBE, INFO,
					"IOV is not supported on this "
					"hardware.  Disabling IOV.\n");
				*aflags &= ~NGBE_FLAG_SRIOV_ENABLED;
				adapter->num_vfs = 0;
			} else if (!(*aflags & NGBE_FLAG_MQ_CAPABLE)) {
				DPRINTK(PROBE, INFO,
					"IOV is not supported while multiple "
					"queues are disabled.  "
					"Disabling IOV.\n");
				*aflags &= ~NGBE_FLAG_SRIOV_ENABLED;
				adapter->num_vfs = 0;
			}
		}
	}
	{ /* VEPA Bridge Mode enable for SR-IOV mode */
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "VEPA Bridge Mode Enable",
			.err  = "defaulting to disabled",
			.def  = OPTION_DISABLED,
			.arg  = { .r = { .min = OPTION_DISABLED,
					 .max = OPTION_ENABLED} }
		};

#ifdef module_param_array
		if (num_VEPA > bd) {
#endif
			u32 vepa = VEPA[bd];
			ngbe_validate_option(&vepa, &opt);
			if (vepa)
				adapter->flags |=
					NGBE_FLAG_SRIOV_VEPA_BRIDGE_MODE;
#ifdef module_param_array
		} else {
			if (opt.def == OPTION_ENABLED)
				adapter->flags |=
					NGBE_FLAG_SRIOV_VEPA_BRIDGE_MODE;
		}
#endif
	}
#endif /* CONFIG_PCI_IOV */
	{ /* Interrupt Throttling Rate */
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "Interrupt Throttling Rate (ints/sec)",
			.err  = "using default of "__MODULE_STRING(DEFAULT_ITR),
			.def  = DEFAULT_ITR,
			.arg  = { .r = { .min = MIN_ITR,
					 .max = MAX_ITR } }
		};

#ifdef module_param_array
		if (num_InterruptThrottleRate > bd) {
#endif
			u32 itr = InterruptThrottleRate[bd];
			switch (itr) {
			case 0:
				DPRINTK(PROBE, INFO, "%s turned off\n",
					opt.name);
				adapter->rx_itr_setting = 0;
				break;
			case 1:
				DPRINTK(PROBE, INFO, "dynamic interrupt "
					"throttling enabled\n");
				adapter->rx_itr_setting = 1;
				break;
			default:
				ngbe_validate_option(&itr, &opt);
				/* the first bit is used as control */
				adapter->rx_itr_setting = (u16)((1000000/itr) << 2);
				break;
			}
			adapter->tx_itr_setting = adapter->rx_itr_setting;
#ifdef module_param_array
		} else {
			adapter->rx_itr_setting = opt.def;
			adapter->tx_itr_setting = opt.def;
		}
#endif
	}
#ifndef NGBE_NO_LLI
	{ /* Low Latency Interrupt TCP Port*/
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "Low Latency Interrupt TCP Port",
			.err  = "using default of "
					__MODULE_STRING(DEFAULT_LLIPORT),
			.def  = DEFAULT_LLIPORT,
			.arg  = { .r = { .min = MIN_LLIPORT,
					 .max = MAX_LLIPORT } }
		};

#ifdef module_param_array
		if (num_LLIPort > bd) {
#endif
			adapter->lli_port = LLIPort[bd];
			if (adapter->lli_port) {
				ngbe_validate_option(&adapter->lli_port, &opt);
			} else {
				DPRINTK(PROBE, INFO, "%s turned off\n",
					opt.name);
			}
#ifdef module_param_array
		} else {
			adapter->lli_port = opt.def;
		}
#endif
	}
	{ /* Low Latency Interrupt on Packet Size */
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "Low Latency Interrupt on Packet Size",
			.err  = "using default of "
					__MODULE_STRING(DEFAULT_LLISIZE),
			.def  = DEFAULT_LLISIZE,
			.arg  = { .r = { .min = MIN_LLISIZE,
					 .max = MAX_LLISIZE } }
		};

#ifdef module_param_array
		if (num_LLISize > bd) {
#endif
			adapter->lli_size = LLISize[bd];
			if (adapter->lli_size) {
				ngbe_validate_option(&adapter->lli_size, &opt);
			} else {
				DPRINTK(PROBE, INFO, "%s turned off\n",
					opt.name);
			}
#ifdef module_param_array
		} else {
			adapter->lli_size = opt.def;
		}
#endif
	}
	{ /* Low Latency Interrupt EtherType*/
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "Low Latency Interrupt on Ethernet Protocol "
				"Type",
			.err  = "using default of "
					__MODULE_STRING(DEFAULT_LLIETYPE),
			.def  = DEFAULT_LLIETYPE,
			.arg  = { .r = { .min = MIN_LLIETYPE,
					 .max = MAX_LLIETYPE } }
		};

#ifdef module_param_array
		if (num_LLIEType > bd) {
#endif
			adapter->lli_etype = LLIEType[bd];
			if (adapter->lli_etype) {
				ngbe_validate_option(&adapter->lli_etype,
						      &opt);
			} else {
				DPRINTK(PROBE, INFO, "%s turned off\n",
					opt.name);
			}
#ifdef module_param_array
		} else {
			adapter->lli_etype = opt.def;
		}
#endif
	}
	{ /* LLI VLAN Priority */
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "Low Latency Interrupt on VLAN priority "
				"threshold",
			.err  = "using default of "
					__MODULE_STRING(DEFAULT_LLIVLANP),
			.def  = DEFAULT_LLIVLANP,
			.arg  = { .r = { .min = MIN_LLIVLANP,
					 .max = MAX_LLIVLANP } }
		};

#ifdef module_param_array
		if (num_LLIVLANP > bd) {
#endif
			adapter->lli_vlan_pri = LLIVLANP[bd];
			if (adapter->lli_vlan_pri) {
				ngbe_validate_option(&adapter->lli_vlan_pri,
						      &opt);
			} else {
				DPRINTK(PROBE, INFO, "%s turned off\n",
					opt.name);
			}
#ifdef module_param_array
		} else {
			adapter->lli_vlan_pri = opt.def;
		}
#endif
	}
#endif /* NGBE_NO_LLI */
#ifdef HAVE_TX_MQ
	{ /* Flow Director ATR Tx sample packet rate */
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "Software ATR Tx packet sample rate",
			.err = "using default of "
				__MODULE_STRING(NGBE_DEFAULT_ATR_SAMPLE_RATE),
			.def = NGBE_DEFAULT_ATR_SAMPLE_RATE,
			.arg = {.r = {.min = NGBE_ATR_SAMPLE_RATE_OFF,
				      .max = NGBE_MAX_ATR_SAMPLE_RATE} }
		};
		static const char atr_string[] =
					    "ATR Tx Packet sample rate set to";

		if (num_AtrSampleRate > bd) {
			adapter->atr_sample_rate = AtrSampleRate[bd];

			if (adapter->atr_sample_rate) {
				ngbe_validate_option(&adapter->atr_sample_rate,
						      &opt);
				DPRINTK(PROBE, INFO, "%s %d\n", atr_string,
					adapter->atr_sample_rate);
			}
		} else {
			adapter->atr_sample_rate = opt.def;
		}
	}
#endif /* HAVE_TX_MQ */
	{ /* LRO - Set Large Receive Offload */
		struct ngbe_option opt = {
			.type = enable_option,
			.name = "LRO - Large Receive Offload",
			.err  = "defaulting to Disabled",
/* lro switch to ON when run on SW and FT platform */
/* emerald temp setting */            
#if defined(TXGBE_SUPPORT_DEEPIN_SW) || \
	defined(TXGBE_SUPPORT_KYLIN_SW) || \
	defined(TXGBE_SUPPORT_KYLIN_FT)			
			.def  = OPTION_ENABLED
#else
            .def  = OPTION_DISABLED
#endif
		};
		struct net_device *netdev = adapter->netdev;

#ifdef NGBE_NO_LRO
		opt.def = OPTION_DISABLED;

#endif
#ifdef module_param_array
		if (num_LRO > bd) {
#endif
			u32 lro = LRO[bd];
			ngbe_validate_option(&lro, &opt);
			if (lro)
				netdev->features |= NETIF_F_LRO;
			else
				netdev->features &= ~NETIF_F_LRO;
#ifdef module_param_array
		} else if (opt.def == OPTION_ENABLED) {
			netdev->features |= NETIF_F_LRO;
		} else {
			netdev->features &= ~NETIF_F_LRO;
		}
#endif
#ifdef NGBE_NO_LRO
		if ((netdev->features & NETIF_F_LRO)) {
			DPRINTK(PROBE, INFO,
				"RSC is not supported on this "
				"hardware.  Disabling RSC.\n");
			netdev->features &= ~NETIF_F_LRO;
		}
#endif /* NGBE_NO_LRO */
	}
	{ /* DMA Coalescing */
		struct ngbe_option opt = {
			.type = range_option,
			.name = "dmac_watchdog",
			.err  = "defaulting to 0 (disabled)",
			.def  = 0,
			.arg  = { .r = { .min = 41, .max = 10000 } },
		};
		const char *cmsg = "DMA coalescing not supported on this "
				   "hardware";

		opt.err = cmsg;
		opt.msg = cmsg;
		opt.arg.r.min = 0;
		opt.arg.r.max = 0;

#ifdef module_param_array
		if (num_dmac_watchdog > bd) {
#endif
			u32 dmac_wd = dmac_watchdog[bd];

			ngbe_validate_option(&dmac_wd, &opt);
			adapter->hw.mac.dmac_config.watchdog_timer = (u16)dmac_wd;
#ifdef module_param_array
		} else {
			adapter->hw.mac.dmac_config.watchdog_timer = opt.def;
		}
#endif
	}

	{ /* Rx buffer mode */
		u32 rx_buf_mode;
		static struct ngbe_option opt = {
			.type = range_option,
			.name = "Rx buffer mode",
			.err = "using default of "
				__MODULE_STRING(NGBE_DEFAULT_RXBUFMODE),
			.def = NGBE_DEFAULT_RXBUFMODE,
			.arg = {.r = {.min = NGBE_RXBUFMODE_NO_HEADER_SPLIT,
							.max = NGBE_RXBUFMODE_HEADER_SPLIT} }

		};

#ifdef module_param_array
		if (num_RxBufferMode > bd) {
#endif
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
#ifdef module_param_array
		} else {
			*aflags &= ~NGBE_FLAG_RX_HS_ENABLED;
		}
#endif

	}
}
