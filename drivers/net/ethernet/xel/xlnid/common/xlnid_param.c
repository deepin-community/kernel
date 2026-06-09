// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2008 - 2022 Xel Technology. */

#include <linux/types.h>
#include <linux/module.h>

#include "xlnid.h"

/*  This is the only thing that needs to be changed to adjust the
		maximum number of ports that the driver can manage.
*/

#define XLNID_MAX_NIC   32

#define OPTION_UNSET    -1
#define OPTION_DISABLED 0
#define OPTION_ENABLED  1

#define STRINGIFY(foo)  #foo    /* magic for getting defines into strings */
#define XSTRINGIFY(bar) STRINGIFY(bar)

/*  All parameters are treated the same, as an integer array of values.
		This macro just reduces the need to repeat the same declaration code
		over and over (plus this helps to avoid typo bugs).
*/

#define XLNID_PARAM_INIT { [0 ... XLNID_MAX_NIC] = OPTION_UNSET }
#ifndef module_param_array
/*  Module Parameters are always initialized to -1, so that the driver
		can tell the difference between no user specified value or the
		user asking for the default value.
		The true default values are loaded in when xlnid_check_options is called.

		This is a GCC extension to ANSI C.
		See the item "Labelled Elements in Initializers" in the section
		"Extensions to the C Language Family" of the GCC documentation.
*/

#define XLNID_PARAM(X, desc) \
	static int __devinitdata X[XLNID_MAX_NIC+1] = XLNID_PARAM_INIT; \
	MODULE_PARM(X, "1-" __MODULE_STRING(XLNID_MAX_NIC) "i"); \
	MODULE_PARM_DESC(X, desc);
#else
#define XLNID_PARAM(X, desc) \
	static int __devinitdata X[XLNID_MAX_NIC+1] = XLNID_PARAM_INIT; \
	static unsigned int num_##X; \
	module_param_array_named(X, X, int, &num_##X, 0); \
	MODULE_PARM_DESC(X, desc);
#endif

/*  IntMode (Interrupt Mode)

		Valid Range: 0-2
		- 0 - Legacy Interrupt
		- 1 - MSI Interrupt
		- 2 - MSI-X Interrupt(s)

		Default Value: 2
*/
XLNID_PARAM(IntMode, "Change Interrupt Mode (0 = Legacy, 1 = MSI, 2 = MSI-X), "
			"default 2");
#define XLNID_INT_LEGACY        0
#define XLNID_INT_MSI           1
#define XLNID_INT_MSIX          2

XLNID_PARAM(InterruptType,
			"Change Interrupt Mode (0 = Legacy, 1 = MSI, 2 = MSI-X), "
			"default IntMode (deprecated)");

/*  MQ - Multiple Queue enable/disable

		Valid Range: 0, 1
		- 0 - disables MQ
		- 1 - enables MQ

		Default Value: 1
*/

XLNID_PARAM(MQ, "Disable or enable Multiple Queues, default 1");

#if IS_ENABLED(CONFIG_DCA)
/*  DCA - Direct Cache Access (DCA) Control

		This option allows the device to hint to DCA enabled processors
		which CPU should have its cache warmed with the data being
		transferred over PCIe.  This can increase performance by reducing
		cache misses.  xlnid hardware supports DCA for:
		tx descriptor writeback
		rx descriptor writeback
		rx data
		rx data header only (in packet split mode)

		enabling option 2 can cause cache thrash in some tests, particularly
		if the CPU is completely utilized

		Valid Range: 0 - 2
		- 0 - disables DCA
		- 1 - enables DCA
		- 2 - enables DCA with rx data included

		Default Value: 2
*/

#define XLNID_MAX_DCA 2

XLNID_PARAM(DCA, "Disable or enable Direct Cache Access, 0 = disabled, "
			"1 = descriptor only, 2 = descriptor and data");
#endif /* CONFIG_DCA */

/*  RSS - Receive-Side Scaling (RSS) Descriptor Queues

		Valid Range: 0-16
		- 0 - enables RSS and sets the Desc. Q's to min(16, num_online_cpus()).
		- 1-16 - enables RSS and sets the Desc. Q's to the specified value.

		Default Value: 0
*/

XLNID_PARAM(RSS, "Number of Receive-Side Scaling Descriptor Queues, "
			"default 0 = number of cpus");

/*  VMDQ - Virtual Machine Device Queues (VMDQ)

		Valid Range: 1-16
		- 0/1 Disables VMDQ by allocating only a single queue.
		- 2-16 - enables VMDQ and sets the Desc. Q's to the specified value.

		Default Value: 8
*/

#define XLNID_DEFAULT_NUM_VMDQ 8

XLNID_PARAM(VMDQ,
			"Number of Virtual Machine Device Queues: 0/1 = disable (1 queue) "
			"2-16 enable (default=" XSTRINGIFY(XLNID_DEFAULT_NUM_VMDQ) ")");

#if 0
/*  max_vfs - SR I/O Virtualization

		Valid Range: 0-63
		- 0 Disables SR-IOV
		- 1-63 - enables SR-IOV and sets the number of VFs enabled

		Default Value: 0
*/

#define MAX_SRIOV_VFS 63

XLNID_PARAM(max_vfs, "Number of Virtual Functions: 0 = disable (default), " "1-"
			XSTRINGIFY(MAX_SRIOV_VFS) " = enable " "this many VFs");

/*  VEPA - Set internal bridge to VEPA mode

		Valid Range: 0-1
		- 0 Set bridge to VEB mode
		- 1 Set bridge to VEPA mode

		Default Value: 0
*/
/*
		Note:
		=====
		This provides ability to ensure VEPA mode on the internal bridge even if
		the kernel does not support the netdev bridge setting operations.
*/
XLNID_PARAM(VEPA, "VEPA Bridge Mode: 0 = VEB (default), 1 = VEPA");
#endif

/*  Interrupt Throttle Rate (interrupts/sec)

		Valid Range: 956-488281 (0 = off, 1 = dynamic)

		Default Value: 1
*/
#define DEFAULT_ITR     1
XLNID_PARAM(InterruptThrottleRate,
			"Maximum interrupts per second, per vector, " "(0, 1, 956-488281), default 1");
#define MAX_ITR     XLNID_MAX_INT_RATE
#define MIN_ITR     XLNID_MIN_INT_RATE

#ifndef XLNID_NO_LLI

/*  LLIPort (Low Latency Interrupt TCP Port)

		Valid Range: 0 - 65535

		Default Value: 0 (disabled)
*/
XLNID_PARAM(LLIPort, "Low Latency Interrupt TCP Port (0-65535)");

#define DEFAULT_LLIPORT     0
#define MAX_LLIPORT     0xFFFF
#define MIN_LLIPORT     0

/*  LLIPush (Low Latency Interrupt on TCP Push flag)

		Valid Range: 0, 1

		Default Value: 0 (disabled)
*/
XLNID_PARAM(LLIPush, "Low Latency Interrupt on TCP Push flag (0, 1)");

#define DEFAULT_LLIPUSH     0
#define MAX_LLIPUSH     1
#define MIN_LLIPUSH     0

/*  LLISize (Low Latency Interrupt on Packet Size)

		Valid Range: 0 - 1500

		Default Value: 0 (disabled)
*/
XLNID_PARAM(LLISize, "Low Latency Interrupt on Packet Size (0-1500)");

#define DEFAULT_LLISIZE     0
#define MAX_LLISIZE     1500
#define MIN_LLISIZE     0

/*  LLIEType (Low Latency Interrupt Ethernet Type)

		Valid Range: 0 - 0x8fff

		Default Value: 0 (disabled)
*/
XLNID_PARAM(LLIEType, "Low Latency Interrupt Ethernet Protocol Type");

#define DEFAULT_LLIETYPE    0
#define MAX_LLIETYPE        0x8fff
#define MIN_LLIETYPE        0

/*  LLIVLANP (Low Latency Interrupt on VLAN priority threshold)

		Valid Range: 0 - 7

		Default Value: 0 (disabled)
*/
XLNID_PARAM(LLIVLANP, "Low Latency Interrupt on VLAN priority threshold");

#define DEFAULT_LLIVLANP    0
#define MAX_LLIVLANP        7
#define MIN_LLIVLANP        0

#endif /* XLNID_NO_LLI */
#ifdef HAVE_TX_MQ
/*  Flow Director packet buffer allocation level

		Valid Range: 1-3
		1 = 8k hash/2k perfect,
		2 = 16k hash/4k perfect,
		3 = 32k hash/8k perfect

		Default Value: 0
*/
XLNID_PARAM(FdirPballoc, "Flow Director packet buffer allocation level:\n"
			"\t\t\t1 = 8k hash filters or 2k perfect filters\n"
			"\t\t\t2 = 16k hash filters or 4k perfect filters\n"
			"\t\t\t3 = 32k hash filters or 8k perfect filters");

#define XLNID_DEFAULT_FDIR_PBALLOC XLNID_FDIR_PBALLOC_64K

/*  Software ATR packet sample rate

		Valid Range: 0-255  0 = off, 1-255 = rate of Tx packet inspection

		Default Value: 20
*/
XLNID_PARAM(AtrSampleRate, "Software ATR Tx packet sample rate");

#define XLNID_MAX_ATR_SAMPLE_RATE   255
#define XLNID_MIN_ATR_SAMPLE_RATE   1
#define XLNID_ATR_SAMPLE_RATE_OFF   0
#define XLNID_DEFAULT_ATR_SAMPLE_RATE   0
#endif /* HAVE_TX_MQ */

/*  Enable/disable Malicious Driver Detection

		Valid Values: 0(off), 1(on)

		Default Value: 1
*/
XLNID_PARAM(MDD, "Malicious Driver Detection: (0, 1), default 1 = on");

/*  Enable/disable Large Receive Offload

		Valid Values: 0(off), 1(on)

		Default Value: 1
*/
XLNID_PARAM(LRO, "Large Receive Offload (0, 1), default 0 = off");

/*  Enable/disable support for untested SFP+ modules on adapters

		Valid Values: 0(Disable), 1(Enable)

		Default Value: 0
*/
XLNID_PARAM(allow_unsupported_sfp,
			"Allow unsupported and untested "
			"SFP+ modules on adapters, default 0 = Disable");

/*  Enable/disable support for DMA coalescing

		Valid Values: 0(off), 41 - 10000(on)

		Default Value: 0
*/
XLNID_PARAM(dmac_watchdog,
			"DMA coalescing watchdog in microseconds (0, 41-10000), default 0 = off");

/*  Enable/disable support for VXLAN rx checksum offload

		Valid Values: 0(Disable), 1(Enable)

		Default Value: 1 on hardware that supports it
*/
XLNID_PARAM(vxlan_rx,
			"VXLAN receive checksum offload (0, 1), default 1 = Enable");

struct xlnid_option {
	enum { enable_option, range_option, list_option } type;
	const char *name;
	const char *err;
	const char *msg;
	int def;
	union {
		struct {                /* range_option info */
			int min;
			int max;
		} r;
		struct {                /* list_option info */
			int nr;
			const struct xlnid_opt_list {
				int i;
				char *str;
			} *p;
		} l;
	} arg;
};

#ifndef XLNID_NO_LLI
#ifdef module_param_array
/**
		helper function to determine LLI support
		@adapter: board private structure
		@opt: pointer to option struct

		LLI is not supported on SKYLAKE
	**/
#ifdef HAVE_CONFIG_HOTPLUG
static bool __devinit xlnid_lli_supported(struct xlnid_adapter *adapter,
		struct xlnid_option *opt)
#else
static bool xlnid_lli_supported(struct xlnid_adapter *adapter,
								struct xlnid_option *opt)
#endif
{
	struct xlnid_hw *hw = &adapter->hw;

	if (hw->mac.type == xlnid_mac_WESTLAKE) {

		if (LLIPush[adapter->bd_number] > 0) {
			goto not_supp;
		}

		return true;
	}

not_supp:
	DPRINTK(PROBE, INFO, "%s not supported on this HW\n", opt->name);
	return false;
}
#endif /* module_param_array */
#endif /* XLNID_NO_LLI */

#ifdef HAVE_CONFIG_HOTPLUG
static int __devinit xlnid_validate_option(unsigned int *value,
		struct xlnid_option *opt)
#else
static int xlnid_validate_option(unsigned int *value, struct xlnid_option *opt)
#endif
{
	if (*value == OPTION_UNSET) {
		printk(KERN_INFO "xlnid: Invalid %s specified (%d),  %s\n", opt->name, *value,
				opt->err);
		*value = opt->def;
		return 0;
	}

	switch (opt->type) {
	case enable_option:
			switch (*value) {
			case OPTION_ENABLED:
				printk(KERN_INFO "xlnid: %s Enabled\n", opt->name);
				return 0;

			case OPTION_DISABLED:
				printk(KERN_INFO "xlnid: %s Disabled\n", opt->name);
				return 0;
			}

			break;

	case range_option:
		if ((*value >= opt->arg.r.min && *value <= opt->arg.r.max)
				|| *value == opt->def) {
			if (opt->msg) {
				printk(KERN_INFO "xlnid: %s set to %d, %s\n", opt->name, *value, opt->msg);
			}

			else {
				printk(KERN_INFO "xlnid: %s set to %d\n", opt->name, *value);
			}

			return 0;
		}

		break;

	case list_option: {
		int i;

		for (i = 0; i < opt->arg.l.nr; i++) {
			const struct xlnid_opt_list *ent = &opt->arg.l.p[i];

			if (*value == ent->i) {
				if (ent->str[0] != '\0') {
					printk(KERN_INFO "%s\n", ent->str);
				}

				return 0;
			}
		}
	}
	break;

	default:
		BUG();
	}

	printk(KERN_INFO "xlnid: Invalid %s specified (%d),  %s\n", opt->name, *value,
			opt->err);
	*value = opt->def;
	return -1;
}

#define LIST_LEN(l) (sizeof(l) / sizeof(l[0]))

/**
		xlnid_check_options - Range Checking for Command Line Parameters
		@adapter: board private structure

		This routine checks all command line parameters for valid user
		input.  If an invalid value is given, or if no user specified
		value exists, a default value is used.  The final value is stored
		in a variable in the adapter structure.
	**/
#ifdef HAVE_CONFIG_HOTPLUG
void __devinit xlnid_check_options(struct xlnid_adapter *adapter)
#else
void xlnid_check_options(struct xlnid_adapter *adapter)
#endif
{
	int bd = adapter->bd_number;
	u32 *aflags = &adapter->flags;
	struct xlnid_ring_feature *feature = adapter->ring_feature;
	unsigned int vmdq;

	if (bd >= XLNID_MAX_NIC) {
		printk(KERN_NOTICE "Warning: no configuration for board #%d\n", bd);
		printk(KERN_NOTICE "Using defaults for all values\n");
#ifndef module_param_array
		bd = XLNID_MAX_NIC;
#endif
	}

	{                           /* Interrupt Mode */
		unsigned int int_mode;

		static struct xlnid_option opt = {
			.type = range_option,
			.name = "Interrupt Mode",
			.err = "using default of " __MODULE_STRING(XLNID_INT_MSIX),
			.def = XLNID_INT_MSIX,
			.arg = {
				.r = {
					.min = XLNID_INT_LEGACY,
					.max = XLNID_INT_MSIX
				}
			}
		};

#ifdef module_param_array

		if (num_IntMode > bd || num_InterruptType > bd) {
#endif
			int_mode = IntMode[bd];

			if (int_mode == OPTION_UNSET) {
				int_mode = InterruptType[bd];
			}

			xlnid_validate_option(&int_mode, &opt);

			switch (int_mode) {
			case XLNID_INT_MSIX:
				if (!(*aflags & XLNID_FLAG_MSIX_CAPABLE)) {
				printk(KERN_INFO "Ignoring MSI-X setting; " "support unavailable\n");
					}

					break;

			case XLNID_INT_MSI:
				if (!(*aflags & XLNID_FLAG_MSI_CAPABLE)) {
					printk(KERN_INFO "Ignoring MSI setting; " "support unavailable\n");
				}

				else {
					*aflags &= ~XLNID_FLAG_MSIX_CAPABLE;
				}

				break;

			case XLNID_INT_LEGACY:
			default:
				*aflags &= ~XLNID_FLAG_MSIX_CAPABLE;
				*aflags &= ~XLNID_FLAG_MSI_CAPABLE;
				break;
			}

#ifdef module_param_array
		}

		else {
			/* default settings */
			if (*aflags & XLNID_FLAG_MSIX_CAPABLE) {
				*aflags |= XLNID_FLAG_MSI_CAPABLE;
			}

			else {
				*aflags &= ~XLNID_FLAG_MSIX_CAPABLE;
				*aflags &= ~XLNID_FLAG_MSI_CAPABLE;
			}
		}

#endif
	}

	{                           /* Multiple Queue Support */
		static struct xlnid_option opt = {
			.type = enable_option,
			.name = "Multiple Queue Support",
			.err = "defaulting to Enabled",
			.def = OPTION_ENABLED
		};

#ifdef module_param_array

		if (num_MQ > bd) {
#endif
			unsigned int mq = MQ[bd];

			xlnid_validate_option(&mq, &opt);

			if (mq) {
				*aflags |= XLNID_FLAG_MQ_CAPABLE;
			}

			else {
				*aflags &= ~XLNID_FLAG_MQ_CAPABLE;
			}

#ifdef module_param_array
		}

		else {
			*aflags |= XLNID_FLAG_MQ_CAPABLE;
		}

#endif

		/* Check Interoperability */
		if ((*aflags & XLNID_FLAG_MQ_CAPABLE) && !(*aflags & XLNID_FLAG_MSIX_CAPABLE)) {
			DPRINTK(PROBE, INFO, "Multiple queues are not supported while MSI-X "
					"is disabled.  Disabling Multiple Queues.\n");
			*aflags &= ~XLNID_FLAG_MQ_CAPABLE;
		}
	}

#if 0
	{                           /* Direct Cache Access (DCA) */
		static struct xlnid_option opt = {
			.type = range_option,
			.name = "Direct Cache Access (DCA)",
			.err = "defaulting to Enabled",
			.def = XLNID_MAX_DCA,
			.arg = {
				.r = {
					.min = OPTION_DISABLED,
					.max = XLNID_MAX_DCA
				}
			}
		};
		unsigned int dca = opt.def;

#ifdef module_param_array

		if (num_DCA > bd) {
#endif
			dca = DCA[bd];
			xlnid_validate_option(&dca, &opt);

			if (!dca) {
				*aflags &= ~XLNID_FLAG_DCA_CAPABLE;
			}

			/* Check Interoperability */
			if (!(*aflags & XLNID_FLAG_DCA_CAPABLE)) {
				DPRINTK(PROBE, INFO, "DCA is disabled\n");
				*aflags &= ~XLNID_FLAG_DCA_ENABLED;
			}

			if (dca == XLNID_MAX_DCA) {
				DPRINTK(PROBE, INFO, "DCA enabled for rx data\n");
				adapter->flags |= XLNID_FLAG_DCA_ENABLED_DATA;
			}

#ifdef module_param_array
		}

		else {
			/*  make sure to clear the capability flag if the
				option is disabled by default above */
			if (opt.def == OPTION_DISABLED) {
				*aflags &= ~XLNID_FLAG_DCA_CAPABLE;
			}
		}

#endif

		if (dca == XLNID_MAX_DCA) {
			adapter->flags |= XLNID_FLAG_DCA_ENABLED_DATA;
		}
	}
#endif /* CONFIG_DCA */
	{                           /* Receive-Side Scaling (RSS) */
		static struct xlnid_option opt = {
			.type = range_option,
			.name = "Receive-Side Scaling (RSS)",
			.err = "using default.",
			.def = 0,
			.arg = {
				.r = {
					.min = 0,
					.max = 16
				}
			}
		};
		unsigned int rss = RSS[bd];

		/* adjust Max allowed RSS queues based on MAC type */
		opt.arg.r.max = xlnid_max_rss_indices(adapter);

#ifdef module_param_array

		if (num_RSS > bd) {
#endif
			xlnid_validate_option(&rss, &opt);

			/* base it off num_online_cpus() with hardware limit */
			if (!rss) {
				rss = min_t(int, opt.arg.r.max, num_online_cpus());
			}

			else {
				feature[RING_F_FDIR].limit = rss;
			}

			feature[RING_F_RSS].limit = rss;
#ifdef module_param_array
		}

		else if (opt.def == 0) {
			rss = min_t(int, xlnid_max_rss_indices(adapter), num_online_cpus());

			feature[RING_F_RSS].limit = rss;
		}

#endif

		/* Check Interoperability */
		if (rss > 1) {
			if (!(*aflags & XLNID_FLAG_MQ_CAPABLE)) {
				DPRINTK(PROBE, INFO, "Multiqueue is disabled.  " "Limiting RSS.\n");
				feature[RING_F_RSS].limit = 1;
			}
		}
	}
	{                           /* Virtual Machine Device Queues (VMDQ) */
		static struct xlnid_option opt = {
			.type = range_option,
			.name = "Virtual Machine Device Queues (VMDQ)",
			.err = "defaulting to Disabled",
			.def = OPTION_DISABLED,
			.arg = {
				.r = {
					.min = OPTION_DISABLED,
					.max = XLNID_MAX_VMDQ_INDICES
				}
			}
		};

#ifdef module_param_array

		if (num_VMDQ > bd) {
#endif
			vmdq = VMDQ[bd];

			xlnid_validate_option(&vmdq, &opt);

			/*  zero or one both mean disabled from our driver's
				perspective */
			if (vmdq > 1) {
				*aflags |= XLNID_FLAG_VMDQ_ENABLED;
			}

			else {
				*aflags &= ~XLNID_FLAG_VMDQ_ENABLED;
			}

			feature[RING_F_VMDQ].limit = vmdq;
#ifdef module_param_array
		}

		else {
			if (opt.def == OPTION_DISABLED) {
				*aflags &= ~XLNID_FLAG_VMDQ_ENABLED;
			}

			else {
				*aflags |= XLNID_FLAG_VMDQ_ENABLED;
			}

			feature[RING_F_VMDQ].limit = opt.def;
		}

#endif

		/* Check Interoperability */
		if (*aflags & XLNID_FLAG_VMDQ_ENABLED) {
			if (!(*aflags & XLNID_FLAG_MQ_CAPABLE)) {
				DPRINTK(PROBE, INFO, "VMDQ is not supported while multiple "
						"queues are disabled.  " "Disabling VMDQ.\n");
				*aflags &= ~XLNID_FLAG_VMDQ_ENABLED;
				feature[RING_F_VMDQ].limit = 0;
			}
		}
	}
#if 0
	{                           /* Single Root I/O Virtualization (SR-IOV) */
		static struct xlnid_option opt = {
			.type = range_option,
			.name = "I/O Virtualization (IOV)",
			.err = "defaulting to Disabled",
			.def = OPTION_DISABLED,
			.arg = {
				.r = {
					.min = OPTION_DISABLED,
					.max = MAX_SRIOV_VFS
				}
			}
		};

#ifdef module_param_array

		if (num_max_vfs > bd) {
#endif
			unsigned int vfs = max_vfs[bd];

			if (xlnid_validate_option(&vfs, &opt)) {
				vfs = 0;
				DPRINTK(PROBE, INFO, "max_vfs out of range " "Disabling SR-IOV.\n");
			}

			adapter->max_vfs = vfs;

			if (vfs) {
				*aflags |= XLNID_FLAG_SRIOV_ENABLED;
			}

			else {
				*aflags &= ~XLNID_FLAG_SRIOV_ENABLED;
			}

#ifdef module_param_array
		}

		else {
			if (opt.def == OPTION_DISABLED) {
				adapter->max_vfs = 0;
				*aflags &= ~XLNID_FLAG_SRIOV_ENABLED;
			}

			else {
				adapter->max_vfs = opt.def;
				*aflags |= XLNID_FLAG_SRIOV_ENABLED;
			}
		}

#endif

		/* Check Interoperability */
		if (*aflags & XLNID_FLAG_SRIOV_ENABLED) {
			if (!(*aflags & XLNID_FLAG_SRIOV_CAPABLE)) {
				DPRINTK(PROBE, INFO, "IOV is not supported on this "
						"hardware.  Disabling IOV.\n");
				*aflags &= ~XLNID_FLAG_SRIOV_ENABLED;
				adapter->max_vfs = 0;
			}

			else if (!(*aflags & XLNID_FLAG_MQ_CAPABLE)) {
				DPRINTK(PROBE, INFO, "IOV is not supported while multiple "
						"queues are disabled.  " "Disabling IOV.\n");
				*aflags &= ~XLNID_FLAG_SRIOV_ENABLED;
				adapter->max_vfs = 0;
			}
		}
	}
	{                           /* VEPA Bridge Mode enable for SR-IOV mode */
		static struct xlnid_option opt = {
			.type = range_option,
			.name = "VEPA Bridge Mode Enable",
			.err = "defaulting to disabled",
			.def = OPTION_DISABLED,
			.arg = {
				.r = {
					.min = OPTION_DISABLED,
					.max = OPTION_ENABLED
				}
			}
		};

#ifdef module_param_array

		if (num_VEPA > bd) {
#endif
			unsigned int vepa = VEPA[bd];

			xlnid_validate_option(&vepa, &opt);

			if (vepa) {
				adapter->flags |= XLNID_FLAG_SRIOV_VEPA_BRIDGE_MODE;
			}

#ifdef module_param_array
		}

		else {
			if (opt.def == OPTION_ENABLED) {
				adapter->flags |= XLNID_FLAG_SRIOV_VEPA_BRIDGE_MODE;
			}
		}

#endif
	}
#endif /* CONFIG_PCI_IOV */
	{                           /* Interrupt Throttling Rate */
		static struct xlnid_option opt = {
			.type = range_option,
			.name = "Interrupt Throttling Rate (ints/sec)",
			.err = "using default of " __MODULE_STRING(DEFAULT_ITR),
			.def = DEFAULT_ITR,
			.arg = {
				.r = {
					.min = MIN_ITR,
					.max = MAX_ITR
				}
			}
		};

#ifdef module_param_array

		if (num_InterruptThrottleRate > bd) {
#endif
			u32 itr = InterruptThrottleRate[bd];

			switch (itr) {
			case 0:
				DPRINTK(PROBE, INFO, "%s turned off\n", opt.name);
				adapter->rx_itr_setting = 0;
				break;

			case 1:
				DPRINTK(PROBE, INFO, "dynamic interrupt " "throttling enabled\n");
				adapter->rx_itr_setting = 1;
				break;

			default:
				xlnid_validate_option(&itr, &opt);
				/* the first bit is used as control */
				adapter->rx_itr_setting = (1000000 / itr) << 2;
				break;
			}

			adapter->tx_itr_setting = adapter->rx_itr_setting;
#ifdef module_param_array
		}

		else {
			adapter->rx_itr_setting = opt.def;
			adapter->tx_itr_setting = opt.def;
		}

#endif
	}
#ifndef XLNID_NO_LLI
	{                           /* Low Latency Interrupt TCP Port */
		static struct xlnid_option opt = {
			.type = range_option,
			.name = "Low Latency Interrupt TCP Port",
			.err = "using default of " __MODULE_STRING(DEFAULT_LLIPORT),
			.def = DEFAULT_LLIPORT,
			.arg = {
				.r = {
					.min = MIN_LLIPORT,
					.max = MAX_LLIPORT
				}
			}
		};

#ifdef module_param_array

		if (num_LLIPort > bd && xlnid_lli_supported(adapter, &opt)) {
#endif
			adapter->lli_port = LLIPort[bd];

			if (adapter->lli_port) {
				xlnid_validate_option(&adapter->lli_port, &opt);
			}

			else {
				DPRINTK(PROBE, INFO, "%s turned off\n", opt.name);
			}

#ifdef module_param_array
		}

		else {
			adapter->lli_port = opt.def;
		}

#endif
	}
	{                           /* Low Latency Interrupt on Packet Size */
		static struct xlnid_option opt = {
			.type = range_option,
			.name = "Low Latency Interrupt on Packet Size",
			.err = "using default of " __MODULE_STRING(DEFAULT_LLISIZE),
			.def = DEFAULT_LLISIZE,
			.arg = {
				.r = {
					.min = MIN_LLISIZE,
					.max = MAX_LLISIZE
				}
			}
		};

#ifdef module_param_array

		if (num_LLISize > bd && xlnid_lli_supported(adapter, &opt)) {
#endif
			adapter->lli_size = LLISize[bd];

			if (adapter->lli_size) {
				xlnid_validate_option(&adapter->lli_size, &opt);
			}

			else {
				DPRINTK(PROBE, INFO, "%s turned off\n", opt.name);
			}

#ifdef module_param_array
		}

		else {
			adapter->lli_size = opt.def;
		}

#endif
	}
	{                           /*Low Latency Interrupt on TCP Push flag */
		static struct xlnid_option opt = {
			.type = enable_option,
			.name = "Low Latency Interrupt on TCP Push flag",
			.err = "defaulting to Disabled",
			.def = OPTION_DISABLED
		};

#ifdef module_param_array

		if (num_LLIPush > bd && xlnid_lli_supported(adapter, &opt)) {
#endif
			unsigned int lli_push = LLIPush[bd];

			xlnid_validate_option(&lli_push, &opt);

			if (lli_push) {
				*aflags |= XLNID_FLAG_LLI_PUSH;
			}

			else {
				*aflags &= ~XLNID_FLAG_LLI_PUSH;
			}

#ifdef module_param_array
		}

		else {
			*aflags &= ~XLNID_FLAG_LLI_PUSH;
		}

#endif
	}
	{                           /* Low Latency Interrupt EtherType */
		static struct xlnid_option opt = {
			.type = range_option,
			.name = "Low Latency Interrupt on Ethernet Protocol " "Type",
			.err = "using default of " __MODULE_STRING(DEFAULT_LLIETYPE),
			.def = DEFAULT_LLIETYPE,
			.arg = {
				.r = {
					.min = MIN_LLIETYPE,
					.max = MAX_LLIETYPE
				}
			}
		};

#ifdef module_param_array

		if (num_LLIEType > bd && xlnid_lli_supported(adapter, &opt)) {
#endif
			adapter->lli_etype = LLIEType[bd];

			if (adapter->lli_etype) {
				xlnid_validate_option(&adapter->lli_etype, &opt);
			}

			else {
				DPRINTK(PROBE, INFO, "%s turned off\n", opt.name);
			}

#ifdef module_param_array
		}

		else {
			adapter->lli_etype = opt.def;
		}

#endif
	}
	{                           /* LLI VLAN Priority */
		static struct xlnid_option opt = {
			.type = range_option,
			.name = "Low Latency Interrupt on VLAN priority " "threshold",
			.err = "using default of " __MODULE_STRING(DEFAULT_LLIVLANP),
			.def = DEFAULT_LLIVLANP,
			.arg = {
				.r = {
					.min = MIN_LLIVLANP,
					.max = MAX_LLIVLANP
				}
			}
		};

#ifdef module_param_array

		if (num_LLIVLANP > bd && xlnid_lli_supported(adapter, &opt)) {
#endif
			adapter->lli_vlan_pri = LLIVLANP[bd];

			if (adapter->lli_vlan_pri) {
				xlnid_validate_option(&adapter->lli_vlan_pri, &opt);
			}

			else {
				DPRINTK(PROBE, INFO, "%s turned off\n", opt.name);
			}

#ifdef module_param_array
		}

		else {
			adapter->lli_vlan_pri = opt.def;
		}

#endif
	}
#endif /* XLNID_NO_LLI */
#ifdef HAVE_TX_MQ
	{                           /* Flow Director packet buffer allocation */
		unsigned int fdir_pballoc_mode;

		static struct xlnid_option opt = {
			.type = range_option,
			.name = "Flow Director packet buffer allocation",
			.err = "using default of " __MODULE_STRING(XLNID_DEFAULT_FDIR_PBALLOC),
			.def = XLNID_DEFAULT_FDIR_PBALLOC,
			.arg = {
				.r = {
					.min = XLNID_FDIR_PBALLOC_64K,
					.max = XLNID_FDIR_PBALLOC_256K
				}
			}
		};
		char pstring[10];

		if (num_FdirPballoc > bd) {
			fdir_pballoc_mode = FdirPballoc[bd];
			xlnid_validate_option(&fdir_pballoc_mode, &opt);

			switch (fdir_pballoc_mode) {
			case XLNID_FDIR_PBALLOC_256K:
				adapter->fdir_pballoc = XLNID_FDIR_PBALLOC_256K;
				sprintf(pstring, "256kB");
				break;

			case XLNID_FDIR_PBALLOC_128K:
				adapter->fdir_pballoc = XLNID_FDIR_PBALLOC_128K;
				sprintf(pstring, "128kB");
				break;

			case XLNID_FDIR_PBALLOC_64K:
			default:
				adapter->fdir_pballoc = XLNID_FDIR_PBALLOC_64K;
				sprintf(pstring, "64kB");
				break;
			}

			DPRINTK(PROBE, INFO, "Flow Director will be allocated " "%s of packet buffer\n",
					pstring);
		}

		else {
			adapter->fdir_pballoc = opt.def;
		}

	}
	{                           /* Flow Director ATR Tx sample packet rate */
		static struct xlnid_option opt = {
			.type = range_option,
			.name = "Software ATR Tx packet sample rate",
			.err = "using default of " __MODULE_STRING(XLNID_DEFAULT_ATR_SAMPLE_RATE),
			.def = XLNID_DEFAULT_ATR_SAMPLE_RATE,
			.arg = {
				.r = {
					.min = XLNID_ATR_SAMPLE_RATE_OFF,
					.max = XLNID_MAX_ATR_SAMPLE_RATE
				}
			}
		};
		static const char atr_string[] = "ATR Tx Packet sample rate set to";

		if (num_AtrSampleRate > bd) {
			adapter->atr_sample_rate = AtrSampleRate[bd];

			if (adapter->atr_sample_rate) {
				xlnid_validate_option(&adapter->atr_sample_rate, &opt);
				DPRINTK(PROBE, INFO, "%s %d\n", atr_string, adapter->atr_sample_rate);
			}
		}

		else {
			adapter->atr_sample_rate = opt.def;
		}
	}
#endif /* HAVE_TX_MQ */

	{                           /* LRO - Set Large Receive Offload */
		struct xlnid_option opt = {
			.type = enable_option,
			.name = "LRO - Large Receive Offload",
			.err = "defaulting to Disabled",
			.def = OPTION_DISABLED
		};
		struct net_device *netdev = adapter->netdev;

		if (!(adapter->flags2 & XLNID_FLAG2_RSC_CAPABLE)) {
			opt.def = OPTION_DISABLED;
		}

#ifdef module_param_array

		if (num_LRO > bd) {
#endif
			unsigned int lro = LRO[bd];

			xlnid_validate_option(&lro, &opt);

			if (lro) {
				netdev->features |= NETIF_F_LRO;
			}

			else {
				netdev->features &= ~NETIF_F_LRO;
			}

#ifdef module_param_array
		}

		else {
			netdev->features &= ~NETIF_F_LRO;
		}

#endif

		if ((netdev->features & NETIF_F_LRO)
				&& !(adapter->flags2 & XLNID_FLAG2_RSC_CAPABLE)) {
			DPRINTK(PROBE, INFO, "RSC is not supported on this "
					"hardware.  Disabling RSC.\n");
			netdev->features &= ~NETIF_F_LRO;
		}
	}
	{                           /*
			allow_unsupported_sfp - Enable/Disable support for unsupported
			and untested SFP+ modules.
		*/
		struct xlnid_option opt = {
			.type = enable_option,
			.name = "allow_unsupported_sfp",
			.err = "defaulting to Disabled",
			.def = OPTION_DISABLED
		};
#ifdef module_param_array

		if (num_allow_unsupported_sfp > bd) {
#endif
			unsigned int enable_unsupported_sfp = allow_unsupported_sfp[bd];

			xlnid_validate_option(&enable_unsupported_sfp, &opt);

			if (enable_unsupported_sfp) {
				adapter->hw.allow_unsupported_sfp = true;
			}

			else {
				adapter->hw.allow_unsupported_sfp = false;
			}

#ifdef module_param_array
		}

		else {
			adapter->hw.allow_unsupported_sfp = false;
		}

#endif
	}
	{                           /* DMA Coalescing */
		struct xlnid_option opt = {
			.type = range_option,
			.name = "dmac_watchdog",
			.err = "defaulting to 0 (disabled)",
			.def = 0,
			.arg = {.r = {.min = 41, .max = 10000} },
		};
		const char *cmsg = "DMA coalescing not supported on this hardware";

		opt.err = cmsg;
		opt.msg = cmsg;
		opt.arg.r.min = 0;
		opt.arg.r.max = 0;
#ifdef module_param_array

		if (num_dmac_watchdog > bd) {
#endif
			unsigned int dmac_wd = dmac_watchdog[bd];

			xlnid_validate_option(&dmac_wd, &opt);
			adapter->hw.mac.dmac_config.watchdog_timer = dmac_wd;
#ifdef module_param_array
		}

		else {
			adapter->hw.mac.dmac_config.watchdog_timer = opt.def;
		}

#endif
	}
	{                           /* VXLAN rx offload */
		struct xlnid_option opt = {
			.type = range_option,
			.name = "vxlan_rx",
			.err = "defaulting to 1 (enabled)",
			.def = 1,
			.arg = {.r = {.min = 0, .max = 1} },
		};
		const char *cmsg = "VXLAN rx offload not supported on this hardware";
		const u32 flag = XLNID_FLAG_VXLAN_OFFLOAD_ENABLE;

		if (!(adapter->flags & XLNID_FLAG_VXLAN_OFFLOAD_CAPABLE)) {
			opt.err = cmsg;
			opt.msg = cmsg;
			opt.def = 0;
			opt.arg.r.max = 0;
		}

#ifdef module_param_array

		if (num_vxlan_rx > bd) {
#endif
			unsigned int enable_vxlan_rx = vxlan_rx[bd];

			xlnid_validate_option(&enable_vxlan_rx, &opt);

			if (enable_vxlan_rx) {
				adapter->flags |= flag;
			}

			else {
				adapter->flags &= ~flag;
			}

#ifdef module_param_array
		}

		else if (opt.def) {
			adapter->flags |= flag;
		}

		else {
			adapter->flags &= ~flag;
		}

#endif
	}

	{                           /* MDD support */
		*aflags &= ~XLNID_FLAG_MDD_ENABLED;
	}
}
