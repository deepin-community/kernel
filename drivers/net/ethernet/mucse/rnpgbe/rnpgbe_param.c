// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#include <linux/types.h>
#include <linux/module.h>

#include "rnpgbe.h"

/* This is the only thing that needs to be changed to adjust the
 * maximum number of ports that the driver can manage.
 */

#define RNP_MAX_NIC 32

#define OPTION_UNSET -1
#define OPTION_DISABLED 0
#define OPTION_ENABLED 1

#define STRINGIFY(foo) #foo /* magic for getting defines into strings */
#define XSTRINGIFY(bar) STRINGIFY(bar)

/* All parameters are treated the same, as an integer array of values.
 * This macro just reduces the need to repeat the same declaration code
 * over and over (plus this helps to avoid typo bugs).
 */

#define RNP_PARAM_INIT                                                         \
	{                                                                      \
		[0 ... RNP_MAX_NIC] = OPTION_UNSET                             \
	}
#define RNP_PARAM(X, desc)                                                     \
	static int X[RNP_MAX_NIC + 1] = RNP_PARAM_INIT;          \
	static unsigned int num_##X;                                           \
	module_param_array_named(X, X, int, &num_##X, 0);                      \
	MODULE_PARM_DESC(X, desc);
/* IntMode (Interrupt Mode)
 *
 * Valid Range: 0-2
 *  - 0 - Legacy Interrupt
 *  - 1 - MSI Interrupt
 *  - 2 - MSI-X Interrupt(s)
 *
 * Default Value: 2
 */
RNP_PARAM(IntMode, "Change Interrupt Mode (0=Legacy, 1=MSI, 2=MSI-X), "
		   "default 2");
#define RNP_INT_LEGACY 0
#define RNP_INT_MSI 1
#define RNP_INT_MSIX 2

#ifdef CONFIG_PCI_IOV
/* max_vfs - SR I/O Virtualization
 *
 * Valid Range: 0-63 for n10
 * Valid Range: 0-7 for n400/n10
 *  - 0 Disables SR-IOV
 *  - 1-x - enables SR-IOV and sets the number of VFs enabled
 *
 * Default Value: 0
 */

RNP_PARAM(max_vfs, "Number of Virtual Functions: 0 = disable (default), "
		   "1-" XSTRINGIFY(MAX_SRIOV_VFS) " = enable "
						  "this many VFs");

/* SRIOV_Mode (SRIOV Mode)
 *
 * Valid Range: 0-1
 *  - 0 - Legacy Interrupt
 *  - 1 - MSI Interrupt
 *  - 2 - MSI-X Interrupt(s)
 *
 * Default Value: 0
 */
RNP_PARAM(SRIOV_Mode, "Change SRIOV Mode (0=MAC_MODE, 1=VLAN_MODE), "
		      "default 0");
#define RNP_SRIOV_MAC_MODE 0
#define RNP_SRIOV_VLAN_MODE 1
#endif

/* pf_msix_counts_set - Limit max msix counts
 *
 * Valid Range: 2-63 for n10
 * Valid Range: 2-7 for n400/n10
 *
 * Default Value: 0 (un-limit)
 */
RNP_PARAM(pf_msix_counts_set, "Number of Max MSIX Count: (default un-limit)");
#define RNP_INT_MIN 2
#define RNP_INT_MAX 64

/* eee_timer - LPI tx expiration time in msec
 *
 * Valid Range: 100-10000
 *
 * Default Value: 4000
 */
RNP_PARAM(eee_timer, "LPI tx expiration time in msec: (default 1000)");
#define RNP_EEE_MIN (100)
#define RNP_EEE_DEFAULT (4000)
#define RNP_EEE_MAX (10000)

/* priv_rx_skip - priv header len
 *
 * Valid Range: [0, 16]
 *
 * Default Value: -
 */
RNP_PARAM(rx_skip, "rx_skip header in DW: (default 0)");
#define RNP_RX_SKIP_MIN (0)
#define RNP_RX_SKIP_DEFAULT (0)
#define RNP_RX_SKIP_MAX (16)

struct rnpgbe_option {
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
			const struct rnpgbe_opt_list {
				int i;
				char *str;
			} *p;
		} l;
	} arg;
};

static int rnpgbe_validate_option(struct net_device *netdev,
				  unsigned int *value,
				  struct rnpgbe_option *opt)
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
				netdev_info(netdev, "%s set to %d\n", opt->name,
					    *value);
			return 0;
		}
		break;
	case list_option: {
		int i;

		for (i = 0; i < opt->arg.l.nr; i++) {
			const struct rnpgbe_opt_list *ent = &opt->arg.l.p[i];

			if (*value == ent->i) {
				if (ent->str[0] != '\0')
					netdev_info(netdev, "%s\n", ent->str);
				return 0;
			}
		}
	} break;
	default:
		BUG();
	}

	netdev_info(netdev, "Invalid %s specified (%d),  %s\n", opt->name,
		    *value, opt->err);
	*value = opt->def;
	return -1;
}

#define LIST_LEN(l) (sizeof(l) / sizeof(l[0]))
#define PSTR_LEN 10

/**
 * rnpgbe_check_options - Range Checking for Command Line Parameters
 * @adapter: board private structure
 *
 * This routine checks all command line parameters for valid user
 * input.  If an invalid value is given, or if no user specified
 * value exists, a default value is used.  The final value is stored
 * in a variable in the adapter structure.
 **/
void rnpgbe_check_options(struct rnpgbe_adapter *adapter)
{
	int bd = adapter->bd_number;
	u32 *aflags = &adapter->flags;

	if (bd >= RNP_MAX_NIC) {
		netdev_notice(adapter->netdev,
			      "Warning: no configuration for board #%d\n", bd);
		netdev_notice(adapter->netdev,
			      "Using defaults for all values\n");
	}

	{ /* Interrupt Mode */
		unsigned int int_mode;
		static struct rnpgbe_option opt = {
			.type = range_option,
			.name = "Interrupt Mode",
			.err = "using default of " __MODULE_STRING(
				RNP_INT_MSIX),
			.def = RNP_INT_MSIX,
			.arg = { .r = { .min = RNP_INT_LEGACY,
					.max = RNP_INT_MSIX } }
		};

		if (num_IntMode > bd) {
			int_mode = IntMode[bd];
			if (int_mode == OPTION_UNSET)
				int_mode = RNP_INT_MSIX;
			rnpgbe_validate_option(adapter->netdev, &int_mode,
					       &opt);
			switch (int_mode) {
			case RNP_INT_MSIX:
				if (!(*aflags & RNP_FLAG_MSIX_CAPABLE)) {
					netdev_info(adapter->netdev,
						    "Ignoring MSI-X setting; "
						    "support unavailable\n");
				} else
					adapter->irq_mode = irq_mode_msix;
				break;
			case RNP_INT_MSI:
				if (!(*aflags & RNP_FLAG_MSI_CAPABLE)) {
					netdev_info(adapter->netdev,
						    "Ignoring MSI setting; "
						    "support unavailable\n");
				} else
					adapter->irq_mode = irq_mode_msi;

				break;
			case RNP_INT_LEGACY:
				if (!(*aflags & RNP_FLAG_LEGACY_CAPABLE)) {
					netdev_info(adapter->netdev,
						    "Ignoring MSI setting; "
						    "support unavailable\n");
				} else
					adapter->irq_mode = irq_mode_legency;

				break;
			}
		} else {
			/* default settings */
			if (*aflags & RNP_FLAG_MSIX_CAPABLE)
				adapter->irq_mode = irq_mode_msix;
			else if (*aflags & RNP_FLAG_MSI_CAPABLE)
				adapter->irq_mode = irq_mode_msi;
			else
				adapter->irq_mode = irq_mode_legency;
		}
	}

#ifdef CONFIG_PCI_IOV
	{ /* Single Root I/O Virtualization (SR-IOV) */
		struct rnpgbe_hw *hw = &adapter->hw;
		static struct rnpgbe_option opt = {
			.type = range_option,
			.name = "I/O Virtualization (IOV)",
			.err = "defaulting to Disabled",
			.def = OPTION_DISABLED,
			.arg = { .r = { .min = OPTION_DISABLED,
					.max = OPTION_DISABLED } }
		};

		opt.arg.r.max = hw->max_vfs;
		if (num_max_vfs > bd) {
			unsigned int vfs = max_vfs[bd];

			if (rnpgbe_validate_option(adapter->netdev, &vfs,
						   &opt)) {
				vfs = 0;
				DPRINTK(PROBE, INFO,
					"max_vfs out of range "
					"Disabling SR-IOV.\n");
			}

			adapter->num_vfs = vfs;

			if (vfs)
				*aflags |= RNP_FLAG_SRIOV_ENABLED;
			else
				*aflags &= ~RNP_FLAG_SRIOV_ENABLED;
		} else {
			if (opt.def == OPTION_DISABLED) {
				adapter->num_vfs = 0;
				*aflags &= ~RNP_FLAG_SRIOV_ENABLED;
			} else {
				adapter->num_vfs = opt.def;
				*aflags |= RNP_FLAG_SRIOV_ENABLED;
			}
		}
	}

	{ /* Interrupt Mode */
		unsigned int sriov_mode;
		static struct rnpgbe_option opt = {
			.type = range_option,
			.name = "SRIOV Mode",
			.err = "using default of " __MODULE_STRING(
				RNP_SRIOV_MAC_MODE),
			.def = RNP_SRIOV_MAC_MODE,
			.arg = { .r = { .min = RNP_SRIOV_MAC_MODE,
					.max = RNP_SRIOV_VLAN_MODE } }
		};
		if (num_SRIOV_Mode > bd) {
			sriov_mode = SRIOV_Mode[bd];
			if (sriov_mode == OPTION_UNSET)
				sriov_mode = RNP_SRIOV_MAC_MODE;
			rnpgbe_validate_option(adapter->netdev, &sriov_mode,
					       &opt);

			if (sriov_mode == RNP_SRIOV_VLAN_MODE)
				adapter->priv_flags |=
					RNP_PRIV_FLAG_SRIOV_VLAN_MODE;
		} else {
			/* default settings */
			/* msix -> msi -> Legacy */
			adapter->priv_flags &= (~RNP_PRIV_FLAG_SRIOV_VLAN_MODE);
		}
	}
#endif /* CONFIG_PCI_IOV */

	{ /* max msix count setup */
		unsigned int pf_msix_counts;
		struct rnpgbe_hw *hw = &adapter->hw;
		static struct rnpgbe_option opt = {
			.type = range_option,
			.name = "Limit Msix Count",
			.err = "using default of Un-limit",
			.def = OPTION_DISABLED,
			.arg = { .r = { .min = RNP_INT_MIN,
					.max = RNP_INT_MIN } }
		};

		opt.arg.r.max = hw->max_msix_vectors;
		if (num_pf_msix_counts_set > bd) {
			pf_msix_counts = pf_msix_counts_set[bd];
			if (pf_msix_counts == OPTION_DISABLED)
				pf_msix_counts = 0;
			rnpgbe_validate_option(adapter->netdev, &pf_msix_counts,
					       &opt);

			if (pf_msix_counts) {
				if (hw->ops.update_msix_count)
					hw->ops.update_msix_count(
						hw, pf_msix_counts);
			}

		} else {
		}
	}

	{ /* LPI tx expiration time in msec */
		unsigned int eee_timer_delay;
		static struct rnpgbe_option opt = {
			.type = range_option,
			.name = "eee timer exp",
			.err = "using default of 1000",
			.def = OPTION_DISABLED,
			.arg = { .r = { .min = RNP_EEE_MIN,
					.max = RNP_EEE_MAX } }
		};

		if (num_eee_timer > bd) {
			eee_timer_delay = eee_timer[bd];
			if (eee_timer_delay == OPTION_DISABLED)
				eee_timer_delay = RNP_EEE_DEFAULT;
			rnpgbe_validate_option(adapter->netdev,
					       &eee_timer_delay, &opt);
			adapter->eee_timer = eee_timer_delay;
		} else {
			adapter->eee_timer = RNP_EEE_DEFAULT;
		}
	}

	{ /* rx_skip in DW */
		unsigned int rx_skip_priv;
		static struct rnpgbe_option opt = {
			.type = range_option,
			.name = "rx_skip in DW",
			.err = "using default of 0",
			.def = OPTION_DISABLED,
			.arg = { .r = { .min = RNP_RX_SKIP_MIN,
					.max = RNP_RX_SKIP_MAX } }
		};

		if (num_rx_skip > bd) {
			rx_skip_priv = rx_skip[bd];
			if (rx_skip_priv == OPTION_DISABLED)
				rx_skip_priv = RNP_RX_SKIP_DEFAULT;
			rnpgbe_validate_option(adapter->netdev, &rx_skip_priv,
					       &opt);
			if (rx_skip_priv) {
				adapter->priv_skip_count = rx_skip_priv - 1;
				adapter->priv_flags |= RNP_PRIV_FLAG_RX_SKIP_EN;
			} else
				adapter->priv_flags &=
					~RNP_PRIV_FLAG_RX_SKIP_EN;
		} else {
			adapter->priv_skip_count = RNP_RX_SKIP_DEFAULT;
			adapter->priv_flags &= ~RNP_PRIV_FLAG_RX_SKIP_EN;
		}
	}
}
