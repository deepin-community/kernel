// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 1999 - 2021 Intel Corporation. */

#include <linux/types.h>
#include <linux/module.h>

#include "grtnic.h"

/* This is the only thing that needs to be changed to adjust the
 * maximum number of ports that the driver can manage.
 */

#define GRTNIC_MAX_NIC	32

#define OPTION_UNSET	-1
#define OPTION_DISABLED	0
#define OPTION_ENABLED	1

#define STRINGIFY(foo)	#foo /* magic for getting defines into strings */
#define XSTRINGIFY(bar)	STRINGIFY(bar)

/* All parameters are treated the same, as an integer array of values.
 * This macro just reduces the need to repeat the same declaration code
 * over and over (plus this helps to avoid typo bugs).
 */

#define GRTNIC_PARAM_INIT { [0 ... GRTNIC_MAX_NIC] = OPTION_UNSET }
#ifndef module_param_array
/* Module Parameters are always initialized to -1, so that the driver
 * can tell the difference between no user specified value or the
 * user asking for the default value.
 * The true default values are loaded in when ixgbe_check_options is called.
 *
 * This is a GCC extension to ANSI C.
 * See the item "Labelled Elements in Initializers" in the section
 * "Extensions to the C Language Family" of the GCC documentation.
 */

#define GRTNIC_PARAM(X, desc) \
	static const int __devinitdata X[GRTNIC_MAX_NIC+1] = GRTNIC_PARAM_INIT; \
	MODULE_PARM(X, "1-" __MODULE_STRING(GRTNIC_MAX_NIC) "i"); \
	MODULE_PARM_DESC(X, desc);
#else
#define GRTNIC_PARAM(X, desc) \
	static int X[GRTNIC_MAX_NIC+1] = GRTNIC_PARAM_INIT; \
	static unsigned int num_##X; \
	module_param_array_named(X, X, int, &num_##X, 0); \
	MODULE_PARM_DESC(X, desc);
#endif //module_param_array

/* IntMode (Interrupt Mode)
 *
 * Valid Range: 0-2
 *  - 0 - Legacy Interrupt
 *  - 1 - MSI Interrupt
 *  - 2 - MSI-X Interrupt(s)
 *
 * Default Value: 2
 */
GRTNIC_PARAM(IntMode, "Change Interrupt Mode (0=Legacy, 1=MSI, 2=MSI-X), "
	    "default 2");
#define GRTNIC_INT_LEGACY		0
#define GRTNIC_INT_MSI			1
#define GRTNIC_INT_MSIX			2

GRTNIC_PARAM(InterruptType, "Change Interrupt Mode (0=Legacy, 1=MSI, 2=MSI-X), "
	    "default IntMode (deprecated)");


/* Interrupt Throttle Rate (interrupts/sec)
 *
 * Valid Range: 956-488281 (0=off, 1=dynamic)
 *
 * Default Value: 1
 */
#define DEFAULT_ITR		1
GRTNIC_PARAM(InterruptThrottleRate, "Maximum interrupts per second, per vector, "
	    "(0,1,956-488281), default 1");
#define MAX_ITR		488281
#define MIN_ITR		956


GRTNIC_PARAM(csum_tx_mode, "Disable or enable tx hecksum offload, default 1");
GRTNIC_PARAM(csum_rx_mode, "Disable or enable rx hecksum offload, default 1");



struct grtnic_option {
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
			const struct grtnic_opt_list {
				int i;
				char *str;
			} *p;
		} l;
	} arg;
};

static int grtnic_validate_option(struct net_device *netdev,
				 unsigned int *value,
				 struct grtnic_option *opt)
{
	if (*value == OPTION_UNSET) {
		netdev_info(netdev, "Invalid %s specified (%d),  %s\n",
			    opt->name, *value, opt->err);
		*value = opt->def;
		return 0;
	}

	switch (opt->type) {
	case enable_option:
		switch (*value) {
		case OPTION_ENABLED:
			netdev_info(netdev, "%s Enabled\n", opt->name);
			return 0;
		case OPTION_DISABLED:
			netdev_info(netdev, "%s Disabled\n", opt->name);
			return 0;
		}
		break;
	case range_option:
		if ((*value >= opt->arg.r.min && *value <= opt->arg.r.max) ||
		    *value == opt->def) {
			if (opt->msg)
				netdev_info(netdev, "%s set to %d, %s\n",
					    opt->name, *value, opt->msg);
			else
				netdev_info(netdev, "%s set to %d\n",
					    opt->name, *value);
			return 0;
		}
		break;
	case list_option: {
		int i;

		for (i = 0; i < opt->arg.l.nr; i++) {
			const struct grtnic_opt_list *ent = &opt->arg.l.p[i];
			if (*value == ent->i) {
				if (ent->str[0] != '\0')
					netdev_info(netdev, "%s\n", ent->str);
				return 0;
			}
		}
	}
		break;
	default:
		BUG();
	}

	netdev_info(netdev, "Invalid %s specified (%d),  %s\n",
		    opt->name, *value, opt->err);
	*value = opt->def;
	return -1;
}

#define LIST_LEN(l) (sizeof(l) / sizeof(l[0]))
#define PSTR_LEN 10

/**
 * grtnic_check_options - Range Checking for Command Line Parameters
 * @adapter: board private structure
 *
 * This routine checks all command line parameters for valid user
 * input.  If an invalid value is given, or if no user specified
 * value exists, a default value is used.  The final value is stored
 * in a variable in the adapter structure.
 **/
void grtnic_check_options(struct grtnic_adapter *adapter)
{
	int bd = adapter->bd_number;
	u32 *aflags = &adapter->flags;

	if (bd >= GRTNIC_MAX_NIC) {
		netdev_notice(adapter->netdev,
			      "Warning: no configuration for board #%d\n", bd);
		netdev_notice(adapter->netdev,
			      "Using defaults for all values\n");
#ifndef module_param_array
		bd = GRTNIC_MAX_NIC;
#endif
	}


	{ /* Interrupt Mode */
		unsigned int int_mode;
		static struct grtnic_option opt = {
			.type = range_option,
			.name = "Interrupt Mode",
			.err =
			  "using default of " __MODULE_STRING(GRTNIC_INT_MSIX),
			.def = GRTNIC_INT_MSIX,
			.arg = { .r = { .min = GRTNIC_INT_LEGACY,
					.max = GRTNIC_INT_MSIX} }
		};

#ifdef module_param_array
		if (num_IntMode > bd || num_InterruptType > bd) {
#endif
			int_mode = IntMode[bd];
			if (int_mode == OPTION_UNSET)
				int_mode = InterruptType[bd];
			grtnic_validate_option(adapter->netdev,
					      &int_mode, &opt);
			switch (int_mode) {
			case GRTNIC_INT_MSIX:
				if (!(*aflags & GRTNIC_FLAG_MSIX_CAPABLE))
					netdev_info(adapter->netdev,
						    "Ignoring MSI-X setting; "
						    "support unavailable\n");
				break;
			case GRTNIC_INT_MSI:
				if (!(*aflags & GRTNIC_FLAG_MSI_CAPABLE)) {
					netdev_info(adapter->netdev,
						    "Ignoring MSI setting; "
						    "support unavailable\n");
				} else {
					*aflags &= ~GRTNIC_FLAG_MSIX_CAPABLE;
				}
				break;
			case GRTNIC_INT_LEGACY:
			default:
				*aflags &= ~GRTNIC_FLAG_MSIX_CAPABLE;
				*aflags &= ~GRTNIC_FLAG_MSI_CAPABLE;
				break;
			}
#ifdef module_param_array
		} else {
			/* default settings */
			if (*aflags & GRTNIC_FLAG_MSIX_CAPABLE) {
				*aflags |= GRTNIC_FLAG_MSI_CAPABLE;
			} else {
				*aflags &= ~GRTNIC_FLAG_MSIX_CAPABLE;
				*aflags &= ~GRTNIC_FLAG_MSI_CAPABLE;
			}
		}
#endif
	}

	{ /* Interrupt Throttling Rate */
		static struct grtnic_option opt = {
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
				grtnic_validate_option(adapter->netdev,
						      &itr, &opt);
				/* the first bit is used as control */
				adapter->rx_itr_setting = (1000000/itr) << 2;
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

	{ /* Tx Checksum Support */
		static struct grtnic_option opt = {
			.type = enable_option,
			.name = "Tx checksum Enable",
			.err  = "defaulting to Enabled",
			.def  = OPTION_ENABLED
		};

#ifdef module_param_array
		if (num_csum_tx_mode > bd) {
#endif
			unsigned int csum_tx = csum_tx_mode[bd];
			grtnic_validate_option(adapter->netdev, &csum_tx, &opt);
			if (csum_tx)
				*aflags |= GRTNIC_FLAG_TXCSUM_CAPABLE;
			else
				*aflags &= ~GRTNIC_FLAG_TXCSUM_CAPABLE;
#ifdef module_param_array
		} else {
			*aflags |= GRTNIC_FLAG_TXCSUM_CAPABLE;
		}
#endif
	}

	{ /* Rx Checksum Support */
		static struct grtnic_option opt = {
			.type = enable_option,
			.name = "Rx checksum Enable",
			.err  = "defaulting to Enabled",
			.def  = OPTION_ENABLED
		};

#ifdef module_param_array
		if (num_csum_rx_mode > bd) {
#endif
			unsigned int csum_rx = csum_rx_mode[bd];
			grtnic_validate_option(adapter->netdev, &csum_rx, &opt);
			if (csum_rx)
				*aflags |= GRTNIC_FLAG_RXCSUM_CAPABLE;
			else
				*aflags &= ~GRTNIC_FLAG_RXCSUM_CAPABLE;
#ifdef module_param_array
		} else {
			*aflags |= GRTNIC_FLAG_RXCSUM_CAPABLE;
		}
#endif
	}


}
