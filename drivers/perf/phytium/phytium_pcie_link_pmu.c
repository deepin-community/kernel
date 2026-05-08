// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium SoC PCIe Link performance monitoring unit support
 *
 * Copyright (c) 2025, Phytium Technology Co., Ltd.
 */

#include <linux/acpi.h>
#include <linux/arm-smccc.h>
#include <linux/bitfield.h>
#include <linux/bitmap.h>
#include <linux/bitops.h>
#include <linux/bug.h>
#include <linux/cpuhotplug.h>
#include <linux/cpumask.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/perf_event.h>
#include <linux/platform_device.h>
#include <linux/smp.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/string.h>

#include <asm/cputype.h>
#include <asm/local64.h>

#undef pr_fmt
#define pr_fmt(fmt) "phytium_pcie_link_pmu: " fmt
#define PCIE_LINK_PERF_DRIVER_VERSION "1.0.0"
#define REG_ENABLE				  0x000
#define REG_SIG_CLEAR			   0x00C

// TLP/DLLP Package
#define REG_XTLH_XDLH_SOT_CNT	   0x02c
#define REG_XTLH_XDLH_EOT_CNT	   0x030
#define REG_RDLH_RTLH_TLP_SOT_CNT   0x034
#define REG_RDLH_RTLH_TLP_EOT_CNT   0x038
#define REG_XDLH_XPLH_SDP_CNT	   0x03c
#define REG_XDLH_XPLH_STP_CNT	   0x040
#define REG_XDLH_XPLH_EOT_CNT	   0x044
// TLP/PL/DLLP ERR
#define REG_ERRO_SIG_SEL			0x008
#define REG_TLP_ERR_CNT			 0x048
#define REG_DLLP_ERR_CNT			0x04C
#define REG_PL_ERR_CNT			  0x050
#define REG_TLP_ERR_MASK			GENMASK(23, 16)
#define REG_DLLP_ERR_MASK		   GENMASK(15, 8)
#define REG_PL_ERR_MASK			 GENMASK(7, 0)
#define TLP_ERR_IDX					 0x07
#define DLLP_ERR_IDX					0x08
#define PL_ERR_IDX					  0x09
#define PL_DLLP_ERR_NUM				 0x05
#define TLP_ERR_NUM					 0x08

// TLP TYPE
#define REG_TLP_TYPE_CNT_ENABLE	 0x0e0
#define REG_TLP_TYPE_CNT_CLEAR	  0x0e4

#define REG_CONTROL_TIME_MODE_TYPE   0x0e8
#define REG_START_TYPE_CNT		  0x0ec
#define REG_TLP_TIMER_TYPE_CNT_0	0x0f4
#define REG_TLP_TIMER_TYPE_CNT_1	0x0f8

#define REG_TX_MRD_CNT			  0x19c
#define REG_TX_MRDLK_CNT			0x1a0
#define REG_TX_MWR_CNT			  0x1a4
#define REG_TX_IORD_CNT			 0x1a8
#define REG_TX_IOWR_CNT			 0x1ac
#define REG_TX_CFGRD0_CNT		   0x1b0
#define REG_TX_CFGWR0_CNT		   0x1b4
#define REG_TX_CFGRD1_CNT		   0x1b8
#define REG_TX_CFGWR1_CNT		   0x1bc
#define REG_TX_MSG_CNT			  0x1c0
#define REG_TX_MSGD_CNT			 0x1c4
#define REG_TX_CPL_CNT			  0x1c8
#define REG_TX_CPLD_CNT			 0x1cc
#define REG_TX_CPLLK_CNT			0x1d0
#define REG_TX_CPLDLK_CNT		   0x1d4
#define REG_TX_FAA_REQ_CNT		  0x1d8
#define REG_TX_USA_REQ_CNT		  0x1dc
#define REG_TX_CSA_REQ_CNT		  0x1e0
#define REG_TX_LOCAL_TLP_PREFIX_CNT 0x1e4
#define REG_TX_ENDTOEND_TLP_PREFIX_CNT  0x1e8
#define REG_TX_TCFGRD_CNT		   0x1ec
#define REG_TX_TCFGWR_CNT		   0x1f0
#define REG_RX_MRD_CNT			  0x1f8
#define REG_RX_MRDLK_CNT			0x1fc
#define REG_RX_MWR_CNT			  0x200
#define REG_RX_IORD_CNT			 0x204
#define REG_RX_IOWR_CNT			 0x208
#define REG_RX_CFGRD0_CNT		   0x20c
#define REG_RX_CFGWR0_CNT		   0x210
#define REG_RX_CFGRD1_CNT		   0x214
#define REG_RX_CFGWR1_CNT		   0x218
#define REG_RX_MSG_CNT			  0x21c
#define REG_RX_MSGD_CNT			 0x220
#define REG_RX_CPL_CNT			  0x224
#define REG_RX_CPLD_CNT			 0x228
#define REG_RX_CPLLK_CNT			0x22c
#define REG_RX_CPLDLK_CNT		   0x230
#define REG_RX_FAA_REQ_CNT		  0x234
#define REG_RX_USA_REQ_CNT		  0x238
#define REG_RX_CSA_REQ_CNT		  0x23c
#define REG_RX_LOCAL_TLP_PREFIX_CNT 0x240
#define REG_RX_ENDTOEND_TLP_PREFIX_CNT  0x244
#define REG_RX_TCFGRD_CNT		   0x248
#define REG_RX_TCFGWR_CNT		   0x24c
// window time (start-top)
#define REG_WINDOW_TIME_TLP_RX_TYPE_CNT_1   0x254
#define REG_WINDOW_TIME_TLP_RX_TYPE_CNT_0   0x258
#define REG_WINDOW_TIME_TLP_TX_TYPE_CNT_1   0x25c
#define REG_WINDOW_TIME_TLP_TX_TYPE_CNT_0   0x260

// tlp performance
#define REG_ENABLE_TLP_PERFORMANCE	  0x0fc
#define REG_CLEAR_TLP_PERFORMANCE	   0x100
#define REG_CONTROL_TIME_MODE_TLP_PERFORMANCE   0x104
#define REG_START_TLP_PERFORMANCE	   0x108
#define REG_TIMER_TLP_PERFORMANCE_0	 0x110
#define REG_TIMER_TLP_PERFORMANCE_1	 0x114

#define REG_WINDOW_TIME_TLP_PERFORMANCE_0   0x264
#define REG_WINDOW_TIME_TLP_PERFORMANCE_1   0x268
#define REG_TX_PAYLOAD_TOTAL_0  0x134
#define REG_TX_PAYLOAD_TOTAL_1  0x138
#define REG_TX_HEADER_TOTAL_0   0x13c
#define REG_TX_HEADER_TOTAL_1   0x140
#define REG_RX_PAYLOAD_TOTAL_0  0x144
#define REG_RX_PAYLOAD_TOTAL_1  0x148
#define REG_RX_HEADER_TOTAL_0   0x14c
#define REG_RX_HEADER_TOTAL_1   0x150

// buffer usage
#define REG_ENABLE_BUFFER	   0x118
#define REG_CLEAR_BUFFER		0x11c
#define REG_CONTROL_TIME_MODE_BUFFER	0x120
#define REG_START_BUFFER		0x124
#define REG_TIMER_BUFFER_0	  0x12c
#define REG_TIMER_BUFFER_1	  0x130

#define REG_BUFFER_USAGE_LN0	0x158
#define REG_BUFFER_USAGE_LN1	0x15c
#define REG_BUFFER_USAGE_LN2	0x160
#define REG_BUFFER_USAGE_LN3	0x164
#define REG_BUFFER_USAGE_LN4	0x168
#define REG_BUFFER_USAGE_LN5	0x16c
#define REG_BUFFER_USAGE_LN6	0x170
#define REG_BUFFER_USAGE_LN7	0x174
#define REG_BUFFER_USAGE_LN8	0x178
#define REG_BUFFER_USAGE_LN9	0x17c
#define REG_BUFFER_USAGE_LN10   0x180
#define REG_BUFFER_USAGE_LN11   0x184
#define REG_BUFFER_USAGE_LN12   0x18c
#define REG_BUFFER_USAGE_LN13   0x190
#define REG_BUFFER_USAGE_LN14   0x194
#define REG_BUFFER_USAGE_LN15   0x198
#define REG_BUFFER_CURR_USAGE_MASK  GENMASK(15, 8)
#define REG_BUFFER_PEAK_USAGE_MASK  GENMASK(7, 0)

#define TLP_TYPE_CLASSIFY_0			0x0a
#define TLP_TYPE_CLASSIFY_1			0x37
#define TLP_PERFORM_TYPE_1			0x38
#define BUFFER_USAGE_TYPE_0			0x3d
#define TLP_WINDOW_TIME_RX_TYPE		0x36
#define TLP_WINDOW_TIME_TX_TYPE		0x37
#define TLP_RX_HEADER_TOTAL_TYPE	0x3c

#define PCIE_LINK_DEVCH		  0xFE0
#define PCIE_LINK_PMU_VER_BIT	GENMASK(7, 0)
#define PHYTIUM_PCIE_LINK_MAX_COUNTERS 93

#define to_phytium_pcie_link_pmu(p) (container_of(p, struct phytium_pcie_link_pmu, pmu))

enum {
	PCIEV1P0 = 0x01,
	PCIEV1P5 = 0x02,
	PCIEV2P0 = 0x03,
};

static int phytium_pcie_link_pmu_hp_state;

struct phytium_pcie_link_pmu_hwevents {
	struct perf_event *hw_events[PHYTIUM_PCIE_LINK_MAX_COUNTERS];
	DECLARE_BITMAP(used_mask_pcie_link, PHYTIUM_PCIE_LINK_MAX_COUNTERS);
};

struct phytium_pcie_link_pmu {
	struct device *dev;
	void __iomem *base;
	struct pmu pmu;
	struct phytium_pcie_link_pmu_hwevents pmu_events;
	u32 die_id;
	u32 pcie_id;
	u32 pmu_id;
	int on_cpu;
	int ver;
	struct hlist_node node;
};

#define EVENT_VALID(idx) ((idx >= 0) && (idx < PHYTIUM_PCIE_LINK_MAX_COUNTERS))

static const u32 pcie_link_counter_reg_offset[] = {
	REG_XTLH_XDLH_SOT_CNT,
	REG_XTLH_XDLH_EOT_CNT,
	REG_RDLH_RTLH_TLP_SOT_CNT,
	REG_RDLH_RTLH_TLP_EOT_CNT,
	REG_XDLH_XPLH_SDP_CNT,
	REG_XDLH_XPLH_STP_CNT,
	REG_XDLH_XPLH_EOT_CNT,

	REG_TLP_ERR_CNT,
	REG_DLLP_ERR_CNT,
	REG_PL_ERR_CNT,

	REG_TX_MRD_CNT,
	REG_TX_MRDLK_CNT,
	REG_TX_MWR_CNT,
	REG_TX_IORD_CNT,
	REG_TX_IOWR_CNT,
	REG_TX_CFGRD0_CNT,
	REG_TX_CFGWR0_CNT,
	REG_TX_CFGRD1_CNT,
	REG_TX_CFGWR1_CNT,
	REG_TX_MSG_CNT,
	REG_TX_MSGD_CNT,
	REG_TX_CPL_CNT,
	REG_TX_CPLD_CNT,
	REG_TX_CPLLK_CNT,
	REG_TX_CPLDLK_CNT,
	REG_TX_FAA_REQ_CNT,
	REG_TX_USA_REQ_CNT,
	REG_TX_CSA_REQ_CNT,
	REG_TX_LOCAL_TLP_PREFIX_CNT,
	REG_TX_ENDTOEND_TLP_PREFIX_CNT,
	REG_TX_TCFGRD_CNT,
	REG_TX_TCFGWR_CNT,
	REG_RX_MRD_CNT,
	REG_RX_MRDLK_CNT,
	REG_RX_MWR_CNT,
	REG_RX_IORD_CNT,
	REG_RX_IOWR_CNT,
	REG_RX_CFGRD0_CNT,
	REG_RX_CFGWR0_CNT,
	REG_RX_CFGRD1_CNT,
	REG_RX_CFGWR1_CNT,
	REG_RX_MSG_CNT,
	REG_RX_MSGD_CNT,
	REG_RX_CPL_CNT,
	REG_RX_CPLD_CNT,
	REG_RX_CPLLK_CNT,
	REG_RX_CPLDLK_CNT,
	REG_RX_FAA_REQ_CNT,
	REG_RX_USA_REQ_CNT,
	REG_RX_CSA_REQ_CNT,
	REG_RX_LOCAL_TLP_PREFIX_CNT,
	REG_RX_ENDTOEND_TLP_PREFIX_CNT,
	REG_RX_TCFGRD_CNT,
	REG_RX_TCFGWR_CNT,
	REG_WINDOW_TIME_TLP_RX_TYPE_CNT_0,
	REG_WINDOW_TIME_TLP_TX_TYPE_CNT_0,

	REG_WINDOW_TIME_TLP_PERFORMANCE_0,
	REG_TX_PAYLOAD_TOTAL_0,
	REG_TX_HEADER_TOTAL_0,
	REG_RX_PAYLOAD_TOTAL_0,
	REG_RX_HEADER_TOTAL_0,

	REG_BUFFER_USAGE_LN0,
	REG_BUFFER_USAGE_LN0,
	REG_BUFFER_USAGE_LN1,
	REG_BUFFER_USAGE_LN1,
	REG_BUFFER_USAGE_LN2,
	REG_BUFFER_USAGE_LN2,
	REG_BUFFER_USAGE_LN3,
	REG_BUFFER_USAGE_LN3,
	REG_BUFFER_USAGE_LN4,
	REG_BUFFER_USAGE_LN4,
	REG_BUFFER_USAGE_LN5,
	REG_BUFFER_USAGE_LN5,
	REG_BUFFER_USAGE_LN6,
	REG_BUFFER_USAGE_LN6,
	REG_BUFFER_USAGE_LN7,
	REG_BUFFER_USAGE_LN7,
	REG_BUFFER_USAGE_LN8,
	REG_BUFFER_USAGE_LN8,
	REG_BUFFER_USAGE_LN9,
	REG_BUFFER_USAGE_LN9,
	REG_BUFFER_USAGE_LN10,
	REG_BUFFER_USAGE_LN10,
	REG_BUFFER_USAGE_LN11,
	REG_BUFFER_USAGE_LN11,
	REG_BUFFER_USAGE_LN12,
	REG_BUFFER_USAGE_LN12,
	REG_BUFFER_USAGE_LN13,
	REG_BUFFER_USAGE_LN13,
	REG_BUFFER_USAGE_LN14,
	REG_BUFFER_USAGE_LN14,
	REG_BUFFER_USAGE_LN15,
	REG_BUFFER_USAGE_LN15
};

ssize_t phytium_pcie_link_pmu_format_sysfs_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct dev_ext_attribute *eattr;

	eattr = container_of(attr, struct dev_ext_attribute, attr);

	return sprintf(buf, "%s\n", (char *)eattr->var);
}

ssize_t phytium_pcie_link_pmu_event_sysfs_show(struct device *dev,
	struct device_attribute *attr,
	char *page)
{
	struct dev_ext_attribute *eattr;

	eattr = container_of(attr, struct dev_ext_attribute, attr);
	unsigned long var_val = (unsigned long)eattr->var;

	if (var_val >= 7 && var_val <= 9)
		return sprintf(page, "config=0x%lx,err_sig=?\n", var_val);

	return sprintf(page, "config=0x%lx\n", var_val);
}

static ssize_t cpumask_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct phytium_pcie_link_pmu *pcie_link_pmu =
					to_phytium_pcie_link_pmu(dev_get_drvdata(dev));

	return cpumap_print_to_pagebuf(true, buf, cpumask_of(pcie_link_pmu->on_cpu));
}

#define PHYTIUM_PMU_ATTR(_name, _func, _config)			\
	(&((struct dev_ext_attribute[]){						\
		{ __ATTR(_name, 0444, _func, NULL), (void *)_config } })[0] \
		.attr.attr)

#define PHYTIUM_PCIE_LINK_PMU_FORMAT_ATTR(_name, _config)				\
	PHYTIUM_PMU_ATTR(_name, phytium_pcie_link_pmu_format_sysfs_show, \
			(void *)_config)

#define PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(_name, _config)				\
	PHYTIUM_PMU_ATTR(_name, phytium_pcie_link_pmu_event_sysfs_show, \
			(unsigned long)_config)

static struct attribute *phytium_pcie_link_pmu_events_attr[] = {
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_xtlh_xdlh_sot, 0x00),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_xtlh_xdlh_eot, 0x01),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rdlh_rtlh_sot, 0x02),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rdlh_rtlh_eot, 0x03),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(dllp_xdlh_xplh_sdp, 0x04),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_xdlh_xplh_stp, 0x05),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_dllp_xdlh_xplh_eot, 0x06),

	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_type_err, 0x07),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(dllp_type_err, 0x08),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(pl_type_err, 0x09),

	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_mrd, 0x0a),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_mrdlk, 0x0b),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_mwr, 0x0c),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_iord, 0x0d),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_iowr, 0x0e),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_cfgrd0, 0x0f),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_cfgwr0, 0x10),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_cfgrd1, 0x11),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_cfgwr1, 0x12),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_msg, 0x13),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_msgd, 0x14),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_cpl, 0x15),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_cpld, 0x16),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_cpllk, 0x17),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_cpldlk, 0x18),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_faa_req, 0x19),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_usa_req, 0x1a),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_csa_req, 0x1b),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_loal_tlp_prefix, 0x1c),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_endtoend_tlp_prefix, 0x1d),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_tcfgrd, 0x1e),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_tcfgwr, 0x1f),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_mrd, 0x20),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_mrdlk, 0x21),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_mwr, 0x22),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_iord, 0x23),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_iowr, 0x24),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_cfgrd0, 0x25),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_cfgwr0, 0x26),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_cfgrd1, 0x27),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_cfgwr1, 0x28),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_msg, 0x29),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_msgd, 0x2a),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_cpl, 0x2b),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_cpld, 0x2c),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_cpllk, 0x2d),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_cpldlk, 0x2e),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_faa_req, 0x2f),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_usa_req, 0x30),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_csa_req, 0x31),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_local_tlp_prefix, 0x32),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_endtoend_tlp_prefix, 0x33),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_tcfgrd, 0x34),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_tcfgwr, 0x35),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_window_time_rx_type, 0x36),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_window_time_tx_type, 0x37),

	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_window_time_performance, 0x38),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_payload_total, 0x39),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_tx_header_total, 0x3a),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_payload_total, 0x3b),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(tlp_rx_header_total, 0x3c),

	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_curr_usage_ln0, 0x3d),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_peak_usage_ln0, 0x3e),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_curr_usage_ln1, 0x3f),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_peak_usage_ln1, 0x40),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_curr_usage_ln2, 0x41),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_peak_usage_ln2, 0x42),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_curr_usage_ln3, 0x43),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_peak_usage_ln3, 0x44),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_curr_usage_ln4, 0x45),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_peak_usage_ln4, 0x46),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_curr_usage_ln5, 0x47),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_peak_usage_ln5, 0x48),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_curr_usage_ln6, 0x49),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_peak_usage_ln6, 0x4a),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_curr_usage_ln7, 0x4b),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_peak_usage_ln7, 0x4c),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_curr_usage_ln8, 0x4d),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_peak_usage_ln8, 0x4e),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_curr_usage_ln9, 0x4f),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_peak_usage_ln9, 0x50),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_curr_usage_ln10, 0x51),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_peak_usage_ln10, 0x52),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_curr_usage_ln11, 0x53),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_peak_usage_ln11, 0x54),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_curr_usage_ln12, 0x55),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_peak_usage_ln12, 0x56),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_curr_usage_ln13, 0x57),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_peak_usage_ln13, 0x58),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_curr_usage_ln14, 0x59),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_peak_usage_ln14, 0x5a),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_curr_usage_ln15, 0x5b),
	PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR(buffer_peak_usage_ln15, 0x5c),
	NULL
};

#define PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR_EXTRACTOR(_name, _config, _start, _end)	\
static inline u32 get_##_name(struct perf_event *event)					\
{											\
	return FIELD_GET(GENMASK_ULL(_end, _start),					\
				event->attr._config);					\
}											\

PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR_EXTRACTOR(eventid, config, 0, 6);
PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR_EXTRACTOR(err_sig, config, 7, 10);
PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR_EXTRACTOR(time_mode, config, 11, 11);
PHYTIUM_PCIE_LINK_PMU_EVENT_ATTR_EXTRACTOR(timer, config1, 0, 63);

static struct attribute *phytium_pcie_link_pmu_format_attr[] = {
	PHYTIUM_PCIE_LINK_PMU_FORMAT_ATTR(event, "config:0-6"),
	PHYTIUM_PCIE_LINK_PMU_FORMAT_ATTR(err_sig, "config:7-10"),
	PHYTIUM_PCIE_LINK_PMU_FORMAT_ATTR(time_mode, "config:11-11"),
	PHYTIUM_PCIE_LINK_PMU_FORMAT_ATTR(timer, "config1:0-63"),
	NULL,
};

static const struct attribute_group phytium_pcie_link_pmu_format_group = {
	.name = "format",
	.attrs = phytium_pcie_link_pmu_format_attr,
};

static const struct attribute_group phytium_pcie_link_pmu_events_group = {
	.name = "events",
	.attrs = phytium_pcie_link_pmu_events_attr,
};

static DEVICE_ATTR_RO(cpumask);

static struct attribute *phytium_pcie_link_pmu_cpumask_attrs[] = {
	&dev_attr_cpumask.attr,
	NULL,
};

static const struct attribute_group phytium_pcie_link_pmu_cpumask_attr_group = {
	.attrs = phytium_pcie_link_pmu_cpumask_attrs,
};

static const struct attribute_group *phytium_pcie_link_pmu_attr_groups[] = {
	&phytium_pcie_link_pmu_format_group,
	&phytium_pcie_link_pmu_events_group,
	&phytium_pcie_link_pmu_cpumask_attr_group,
	NULL,
};


static const u32 phytium_pcie_link_err_reg_mask[] = {
	REG_TLP_ERR_MASK,
	REG_DLLP_ERR_MASK,
	REG_PL_ERR_MASK
};

static u64 phytium_pcie_link_pmu_read_counter(struct phytium_pcie_link_pmu *pcie_link_pmu,
	struct perf_event *event)
{
	u32 val64, val32_l, val32_h;
	u32 idx = get_eventid(event);
	u32 counter_offset = pcie_link_counter_reg_offset[idx];

	if (!EVENT_VALID(idx)) {
		dev_err(pcie_link_pmu->dev, "Unsupported event index:%d!\n", idx);
		return 0;
	}

	if (idx >= BUFFER_USAGE_TYPE_0) {
		val32_l = readl(pcie_link_pmu->base + counter_offset);
		if (idx % 2 == 0)
			val32_l = (val32_l & REG_BUFFER_CURR_USAGE_MASK) >> 8;
		else
			val32_l = val32_l & REG_BUFFER_PEAK_USAGE_MASK;
		val64 = (u64)val32_l;
	} else if (idx == TLP_WINDOW_TIME_RX_TYPE || idx == TLP_WINDOW_TIME_TX_TYPE) {
		val32_l = readl(pcie_link_pmu->base + counter_offset);
		val32_h = readl(pcie_link_pmu->base + counter_offset - 4);
		val64 = (u64)val32_h << 32 | (u64)val32_l;
	} else if (idx > TLP_WINDOW_TIME_TX_TYPE && idx <= TLP_RX_HEADER_TOTAL_TYPE) {
		val32_l = readl(pcie_link_pmu->base + counter_offset);
		val32_h = readl(pcie_link_pmu->base + counter_offset + 4);
		val64 = (u64)val32_h << 32 | (u64)val32_l;
	} else {
		val32_l = readl(pcie_link_pmu->base + counter_offset);
		val64 = (u64)val32_l;
	}
	return val64;
}

static void phytium_pcie_link_pmu_clear_all_counters(struct phytium_pcie_link_pmu *pcie_link_pmu)
{
	writel(0xffffffff, pcie_link_pmu->base + REG_SIG_CLEAR);
	writel(0x00000000, pcie_link_pmu->base + REG_SIG_CLEAR);

	writel(0x3, pcie_link_pmu->base + REG_TLP_TYPE_CNT_CLEAR);
	writel(0x0, pcie_link_pmu->base + REG_TLP_TYPE_CNT_CLEAR);
	writel(0x1, pcie_link_pmu->base + REG_CLEAR_TLP_PERFORMANCE);
	writel(0x0, pcie_link_pmu->base + REG_CLEAR_TLP_PERFORMANCE);
	writel(0x1, pcie_link_pmu->base + REG_CLEAR_BUFFER);
	writel(0x0, pcie_link_pmu->base + REG_CLEAR_BUFFER);
}

static void phytium_pcie_link_pmu_enable_all_counters(struct phytium_pcie_link_pmu *pcie_link_pmu)
{
	writel(0xffffffff, pcie_link_pmu->base + REG_ENABLE);
	writel(0x3, pcie_link_pmu->base + REG_TLP_TYPE_CNT_ENABLE);
	writel(0x1, pcie_link_pmu->base + REG_ENABLE_TLP_PERFORMANCE);
	writel(0x1, pcie_link_pmu->base + REG_ENABLE_BUFFER);
}

static void phytium_pcie_link_pmu_disable_all_counters(struct phytium_pcie_link_pmu *pcie_link_pmu)
{
	writel(0x00000000, pcie_link_pmu->base + REG_ENABLE);
	writel(0x00, pcie_link_pmu->base + REG_TLP_TYPE_CNT_ENABLE);
	writel(0x0, pcie_link_pmu->base + REG_ENABLE_TLP_PERFORMANCE);
	writel(0x0, pcie_link_pmu->base + REG_ENABLE_BUFFER);
}

static void phytium_pcie_link_pmu_start_all_counters(struct perf_event *event)
{
	struct phytium_pcie_link_pmu *pcie_link_pmu = to_phytium_pcie_link_pmu(event->pmu);
	u32 idx = get_eventid(event);

	if (idx >= TLP_TYPE_CLASSIFY_0 && idx <= TLP_TYPE_CLASSIFY_1)
		writel(0x1, pcie_link_pmu->base + REG_START_TYPE_CNT);
	else if (idx >= TLP_PERFORM_TYPE_1 && idx < BUFFER_USAGE_TYPE_0)
		writel(0x1, pcie_link_pmu->base + REG_START_TLP_PERFORMANCE);
	else if (idx >= BUFFER_USAGE_TYPE_0)
		writel(0x1, pcie_link_pmu->base + REG_START_BUFFER);
}

static void phytium_pcie_link_pmu_stop_all_counters(struct perf_event *event)
{
	struct phytium_pcie_link_pmu *pcie_link_pmu = to_phytium_pcie_link_pmu(event->pmu);
	u32 idx = get_eventid(event);

	if (idx >= TLP_TYPE_CLASSIFY_0 && idx <= TLP_TYPE_CLASSIFY_1)
		writel(0x0, pcie_link_pmu->base + REG_START_TYPE_CNT);
	else if (idx >= TLP_PERFORM_TYPE_1 && idx < BUFFER_USAGE_TYPE_0)
		writel(0x0, pcie_link_pmu->base + REG_START_TLP_PERFORMANCE);
	else if (idx >= BUFFER_USAGE_TYPE_0)
		writel(0x0, pcie_link_pmu->base + REG_START_BUFFER);
}

static void phytium_pcie_link_pmu_set_timer(struct perf_event *event, u64 th_val)
{
	struct phytium_pcie_link_pmu *pcie_link_pmu = to_phytium_pcie_link_pmu(event->pmu);

	u32 val_l, val_h;

	val_l = th_val & 0xFFFFFFFF;
	val_h = (th_val >> 32) & 0xFFFFFFFF;
	u32 idx = get_eventid(event);

	if (idx >= TLP_TYPE_CLASSIFY_0 && idx <= TLP_TYPE_CLASSIFY_1) {
		writel(0x1, pcie_link_pmu->base + REG_CONTROL_TIME_MODE_TYPE);
		writel(val_l, pcie_link_pmu->base + REG_TLP_TIMER_TYPE_CNT_0);
		writel(val_h, pcie_link_pmu->base + REG_TLP_TIMER_TYPE_CNT_1);
	} else if (idx >= TLP_PERFORM_TYPE_1 && idx < BUFFER_USAGE_TYPE_0) {
		writel(0x1, pcie_link_pmu->base + REG_CONTROL_TIME_MODE_TLP_PERFORMANCE);
		writel(val_l, pcie_link_pmu->base + REG_TIMER_TLP_PERFORMANCE_0);
		writel(val_h, pcie_link_pmu->base + REG_TIMER_TLP_PERFORMANCE_1);
	} else if (idx >= BUFFER_USAGE_TYPE_0) {
		writel(0x1, pcie_link_pmu->base + REG_CONTROL_TIME_MODE_BUFFER);
		writel(val_l, pcie_link_pmu->base + REG_TIMER_BUFFER_0);
		writel(val_h, pcie_link_pmu->base + REG_TIMER_BUFFER_1);
	}
}

static void phytium_pcie_link_pmu_reset_timer(struct perf_event *event)
{
	struct phytium_pcie_link_pmu *pcie_link_pmu = to_phytium_pcie_link_pmu(event->pmu);

	u32 idx = get_eventid(event);

	if (idx >= TLP_TYPE_CLASSIFY_0 && idx <= TLP_TYPE_CLASSIFY_1) {
		writel(0x0, pcie_link_pmu->base + REG_CONTROL_TIME_MODE_TYPE);
		writel(0xFFFFFFFF, pcie_link_pmu->base + REG_TLP_TIMER_TYPE_CNT_0);
		writel(0xFFFFFFFF, pcie_link_pmu->base + REG_TLP_TIMER_TYPE_CNT_1);
	} else if (idx >= TLP_PERFORM_TYPE_1 && idx < BUFFER_USAGE_TYPE_0) {
		writel(0x0, pcie_link_pmu->base + REG_CONTROL_TIME_MODE_TLP_PERFORMANCE);
		writel(0xFFFFFFFF, pcie_link_pmu->base + REG_TIMER_TLP_PERFORMANCE_0);
		writel(0xFFFFFFFF, pcie_link_pmu->base + REG_TIMER_TLP_PERFORMANCE_1);
	} else if (idx >= BUFFER_USAGE_TYPE_0) {
		writel(0x0, pcie_link_pmu->base + REG_CONTROL_TIME_MODE_BUFFER);
		writel(0xFFFFFFFF, pcie_link_pmu->base + REG_TIMER_BUFFER_0);
		writel(0xFFFFFFFF, pcie_link_pmu->base + REG_TIMER_BUFFER_1);
	}
}

static int phytium_pcie_link_pmu_mark_event(struct perf_event *event)
{
	struct phytium_pcie_link_pmu *pcie_link_pmu = to_phytium_pcie_link_pmu(event->pmu);
	unsigned long *used_mask;
	int idx = get_eventid(event);

	used_mask = pcie_link_pmu->pmu_events.used_mask_pcie_link;

	if (test_bit(idx, used_mask))
		return -EAGAIN;

	set_bit(idx, used_mask);

	return idx;
}

static void phytium_pcie_link_pmu_unmark_event(struct phytium_pcie_link_pmu *pcie_link_pmu, int idx)
{
	unsigned long *used_mask;

	if (!EVENT_VALID(idx)) {
		dev_err(pcie_link_pmu->dev, "Unsupported event index:%d!\n", idx);
		return;
	}

	used_mask = pcie_link_pmu->pmu_events.used_mask_pcie_link;

	clear_bit(idx, used_mask);
}

static bool phytium_pcie_link_pmu_is_reg_err_sig(u32 event_id, u32 err_id)
{
	if (event_id == PL_ERR_IDX || event_id == DLLP_ERR_IDX) {
		if (err_id > PL_DLLP_ERR_NUM)
			return false;
	} else if (event_id == TLP_ERR_IDX) {
		if (err_id > TLP_ERR_NUM)
			return false;
	}
	return true;
}

static void phytium_pcie_link_pmu_set_reg_err_sig(struct phytium_pcie_link_pmu *pcie_link_pmu,
						u32 event_id, u32 err_id)
{
	int err_mask_id = event_id - 7;

	u32 err_reg_mask = phytium_pcie_link_err_reg_mask[err_mask_id];
	u32 val = readl(pcie_link_pmu->base + REG_ERRO_SIG_SEL);

	if (event_id == PL_ERR_IDX)
		err_reg_mask &= (err_id << 16);
	else if (event_id == DLLP_ERR_IDX)
		err_reg_mask &= (err_id << 8);
	else
		err_reg_mask &= err_id;
	val |= err_reg_mask;

	writel(val, pcie_link_pmu->base + REG_ERRO_SIG_SEL);
}

int phytium_pcie_link_pmu_event_init(struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;
	struct phytium_pcie_link_pmu *pcie_link_pmu;

	if (event->attr.type != event->pmu->type)
		return -ENOENT;

	if (is_sampling_event(event) || event->attach_state & PERF_ATTACH_TASK)
		return -EOPNOTSUPP;

	pcie_link_pmu = to_phytium_pcie_link_pmu(event->pmu);

	if (event->cpu < 0) {
		dev_warn(pcie_link_pmu->dev, "Can't provide per-task data!\n");
		return -EINVAL;
	}

	if (pcie_link_pmu->on_cpu == -1)
		return -EINVAL;

	hwc->idx = -1;
	hwc->config_base = event->attr.config;

	event->cpu = pcie_link_pmu->on_cpu;

	return 0;
}

void phytium_pcie_link_pmu_event_update(struct perf_event *event)
{
	struct phytium_pcie_link_pmu *pcie_link_pmu = to_phytium_pcie_link_pmu(event->pmu);
	u64 delta;

	delta = phytium_pcie_link_pmu_read_counter(pcie_link_pmu, event);
	local64_add(delta, &event->count);
}

void phytium_pcie_link_pmu_event_start(struct perf_event *event, int flags)
{
	struct hw_perf_event *hwc = &event->hw;

	hwc->state = 0;
	perf_event_update_userpage(event);
}

void phytium_pcie_link_pmu_event_stop(struct perf_event *event, int flags)
{
	struct hw_perf_event *hwc = &event->hw;

	hwc->state |= PERF_HES_STOPPED;

	if (flags & PERF_EF_UPDATE)
		phytium_pcie_link_pmu_event_update(event);
}

int phytium_pcie_link_pmu_event_add(struct perf_event *event, int flags)
{
	struct phytium_pcie_link_pmu *pcie_link_pmu = to_phytium_pcie_link_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	int idx;
	u32 event_id = get_eventid(event);
	u32 err_id  =   get_err_sig(event);
	u64 event_timer = get_timer(event);
	int time_mode = get_time_mode(event);

	if (event_id >= TLP_ERR_IDX && event_id <= PL_ERR_IDX) {
		if (phytium_pcie_link_pmu_is_reg_err_sig(event_id, err_id))
			phytium_pcie_link_pmu_set_reg_err_sig(pcie_link_pmu, event_id, err_id);
		else {
			dev_err(pcie_link_pmu->dev, "Register's err type is incorrect!\n");
			return -EINVAL;
		}
	}

	if (event_timer != 0 && time_mode != 0)
		phytium_pcie_link_pmu_set_timer(event, event_timer);
	else
		phytium_pcie_link_pmu_start_all_counters(event);

	hwc->state |= PERF_HES_STOPPED;

	idx = phytium_pcie_link_pmu_mark_event(event);
	if (idx < 0)
		return idx;

	event->hw.idx = idx;
	pcie_link_pmu->pmu_events.hw_events[idx] = event;

	return 0;
}

void phytium_pcie_link_pmu_event_del(struct perf_event *event, int flags)
{
	struct phytium_pcie_link_pmu *pcie_link_pmu = to_phytium_pcie_link_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	u32 event_timer = get_timer(event);

	phytium_pcie_link_pmu_event_stop(event, PERF_EF_UPDATE);
	if (event_timer != 0)
		phytium_pcie_link_pmu_reset_timer(event);
	else
		phytium_pcie_link_pmu_stop_all_counters(event);

	phytium_pcie_link_pmu_unmark_event(pcie_link_pmu, hwc->idx);

	perf_event_update_userpage(event);
	pcie_link_pmu->pmu_events.hw_events[hwc->idx] = NULL;
}

void phytium_pcie_link_pmu_enable(struct pmu *pmu)
{
	struct phytium_pcie_link_pmu *pcie_link_pmu = to_phytium_pcie_link_pmu(pmu);
	int event_added = 0;

	event_added = bitmap_weight(pcie_link_pmu->pmu_events.used_mask_pcie_link,
					PHYTIUM_PCIE_LINK_MAX_COUNTERS);

	if (event_added) {
		phytium_pcie_link_pmu_clear_all_counters(pcie_link_pmu);
		phytium_pcie_link_pmu_enable_all_counters(pcie_link_pmu);
	}
}

void phytium_pcie_link_pmu_disable(struct pmu *pmu)
{
	struct phytium_pcie_link_pmu *pcie_link_pmu = to_phytium_pcie_link_pmu(pmu);
	int event_added = 0;

	event_added = bitmap_weight(pcie_link_pmu->pmu_events.used_mask_pcie_link,
					PHYTIUM_PCIE_LINK_MAX_COUNTERS);

	if (event_added)
		phytium_pcie_link_pmu_disable_all_counters(pcie_link_pmu);
}

static const struct acpi_device_id phytium_pcie_link_pmu_acpi_match[] = {
	{ "PHYT300E", },
	{},
};
MODULE_DEVICE_TABLE(acpi, phytium_pcie_link_pmu_acpi_match);

static int phytium_pcie_link_pmu_version(struct phytium_pcie_link_pmu *pcie_link_pmu,
	struct platform_device *pdev)
{
	u32 pidr;

	pidr = readl(pcie_link_pmu->base + PCIE_LINK_DEVCH);
	dev_info(pcie_link_pmu->dev, "PIDR=%#x,VER=%#lx.\n", pidr, (pidr & PCIE_LINK_PMU_VER_BIT));
	pidr &= PCIE_LINK_PMU_VER_BIT;

	if (pidr == 0x1) {
		pcie_link_pmu->ver = PCIEV1P0;
	} else {
		dev_err(&pdev->dev, "The current driver does not support this device.\n");
		return -ENODEV;
	}

	return 0;
}

static int phytium_pcie_link_pmu_init_data(struct platform_device *pdev,
	struct phytium_pcie_link_pmu *pcie_link_pmu)
{
	int ret;
	struct resource *res;

	if (device_property_read_u32(&pdev->dev, "phytium,die-id",
					&pcie_link_pmu->die_id)) {
		dev_err(&pdev->dev, "Can not read phytium,die-id!\n");
		return -EINVAL;
	}

	if (device_property_read_u32(&pdev->dev, "phytium,pcie-id",
					&pcie_link_pmu->pcie_id)) {
		dev_err(&pdev->dev, "Can not read phytium,pcie-id!\n");
		return -EINVAL;
	}

	if (device_property_read_u32(&pdev->dev, "phytium,pmu-id",
					&pcie_link_pmu->pmu_id)) {
		dev_err(&pdev->dev, "Can not read pcie link pmu-id!\n");
return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pcie_link_pmu->base = devm_ioremap_resource(&pdev->dev, res);

	if (IS_ERR(pcie_link_pmu->base)) {
		dev_err(&pdev->dev,
			"ioremap failed for pcie_link_pmu base resource\n");
		return PTR_ERR(pcie_link_pmu->base);
	}

	ret = phytium_pcie_link_pmu_version(pcie_link_pmu, pdev);
	if (ret)
		return ret;

	return 0;
}

static int phytium_pcie_link_pmu_dev_probe(struct platform_device *pdev,
	struct phytium_pcie_link_pmu *pcie_link_pmu)
{
	int ret;

	ret = phytium_pcie_link_pmu_init_data(pdev, pcie_link_pmu);
	if (ret)
		return ret;

	pcie_link_pmu->dev = &pdev->dev;
	pcie_link_pmu->on_cpu = -1;

	return 0;
}

static int phytium_pcie_link_pmu_probe(struct platform_device *pdev)
{
	struct phytium_pcie_link_pmu *pcie_link_pmu;
	char *name;
	int ret;

	pcie_link_pmu = devm_kzalloc(&pdev->dev, sizeof(*pcie_link_pmu), GFP_KERNEL);
	if (!pcie_link_pmu)
		return -ENOMEM;

	platform_set_drvdata(pdev, pcie_link_pmu);

	ret = phytium_pcie_link_pmu_dev_probe(pdev, pcie_link_pmu);
	if (ret)
		return ret;

	ret = cpuhp_state_add_instance(phytium_pcie_link_pmu_hp_state,
						&pcie_link_pmu->node);
	if (ret) {
		dev_err(&pdev->dev, "Error %d registering hotplug;\n", ret);
		return ret;
	}

	name = devm_kasprintf(&pdev->dev, GFP_KERNEL, "phyt%u_pcie%u_link_pmu%u",
				pcie_link_pmu->die_id, pcie_link_pmu->pcie_id,
				pcie_link_pmu->pmu_id);
	pcie_link_pmu->pmu = (struct pmu){
		.name = name,
		.module = THIS_MODULE,
		.task_ctx_nr = perf_invalid_context,
		.event_init = phytium_pcie_link_pmu_event_init,
		.pmu_enable = phytium_pcie_link_pmu_enable,
		.pmu_disable = phytium_pcie_link_pmu_disable,
		.add = phytium_pcie_link_pmu_event_add,
		.del = phytium_pcie_link_pmu_event_del,
		.start = phytium_pcie_link_pmu_event_start,
		.stop = phytium_pcie_link_pmu_event_stop,
		.read = phytium_pcie_link_pmu_event_update,
		.attr_groups = phytium_pcie_link_pmu_attr_groups,
		.capabilities = PERF_PMU_CAP_NO_EXCLUDE,
	};

	ret = perf_pmu_register(&pcie_link_pmu->pmu, name, -1);
	if (ret) {
		dev_err(pcie_link_pmu->dev, "PCIE LINK PMU register failed!\n");
		cpuhp_state_remove_instance_nocalls(phytium_pcie_link_pmu_hp_state,
							&pcie_link_pmu->node);
	}

	dev_info(pcie_link_pmu->dev, "%s on cpu%d.\n", name, pcie_link_pmu->on_cpu);
	return ret;
}

static int phytium_pcie_link_pmu_remove(struct platform_device *pdev)
{
	struct phytium_pcie_link_pmu *pcie_link_pmu = platform_get_drvdata(pdev);

	phytium_pcie_link_pmu_disable_all_counters(pcie_link_pmu);

	perf_pmu_unregister(&pcie_link_pmu->pmu);
	cpuhp_state_remove_instance_nocalls(phytium_pcie_link_pmu_hp_state,
						&pcie_link_pmu->node);

	return 0;
}

static struct platform_driver phytium_pcie_link_pmu_driver = {
	.driver = {
		.name = "phytium_pcie_link_pmu",
		.acpi_match_table = ACPI_PTR(phytium_pcie_link_pmu_acpi_match),
		.suppress_bind_attrs = true,
	},
	.probe = phytium_pcie_link_pmu_probe,
	.remove = phytium_pcie_link_pmu_remove,
};

int phytium_pcie_link_pmu_online_cpu(unsigned int cpu, struct hlist_node *node)
{
	struct phytium_pcie_link_pmu *pcie_link_pmu =
		hlist_entry_safe(node, struct phytium_pcie_link_pmu, node);

	if (!cpumask_test_cpu(cpu, cpumask_of_node(pcie_link_pmu->die_id)))
		return 0;

	if (pcie_link_pmu->on_cpu != -1) {
		if (!cpumask_test_cpu(pcie_link_pmu->on_cpu,
		cpumask_of_node(pcie_link_pmu->die_id))) {
			perf_pmu_migrate_context(&pcie_link_pmu->pmu, pcie_link_pmu->on_cpu, cpu);
			pcie_link_pmu->on_cpu = cpu;
		}
		return 0;
	}

	pcie_link_pmu->on_cpu = cpu;

	return 0;
}

int phytium_pcie_link_pmu_offline_cpu(unsigned int cpu, struct hlist_node *node)
{
	struct phytium_pcie_link_pmu *pcie_link_pmu =
		hlist_entry_safe(node, struct phytium_pcie_link_pmu, node);
	unsigned int target;
	cpumask_t available_cpus;

	if (pcie_link_pmu->on_cpu != cpu)
		return 0;

	if (cpumask_and(&available_cpus, cpumask_of_node(pcie_link_pmu->die_id), cpu_online_mask) &&
		cpumask_andnot(&available_cpus, &available_cpus, cpumask_of(cpu)))
		target = cpumask_last(&available_cpus);
	else {
		cpumask_andnot(&available_cpus, cpu_online_mask, cpumask_of(cpu));
		target = cpumask_last(&available_cpus);
	}

	if (target >= nr_cpu_ids) {
		dev_err(pcie_link_pmu->dev, "offline cpu%d with no target to migrate.\n", cpu);
		return 0;
	}

	perf_pmu_migrate_context(&pcie_link_pmu->pmu, cpu, target);
	pcie_link_pmu->on_cpu = target;

	return 0;
}

static int __init phytium_pcie_link_pmu_module_init(void)
{
	int ret;

	phytium_pcie_link_pmu_hp_state = cpuhp_setup_state_multi(CPUHP_AP_ONLINE_DYN,
					"perf/phytium/pcielinkpmu:online",
					phytium_pcie_link_pmu_online_cpu,
					phytium_pcie_link_pmu_offline_cpu);
	if (phytium_pcie_link_pmu_hp_state < 0) {
		pr_err("PCIE LINK PMU: setup hotplug, phytium_pcie_link_pmu_hp_state = %d\n",
			phytium_pcie_link_pmu_hp_state);
		return phytium_pcie_link_pmu_hp_state;
	}

	ret = platform_driver_register(&phytium_pcie_link_pmu_driver);
	if (ret)
		cpuhp_remove_multi_state(phytium_pcie_link_pmu_hp_state);

	return ret;
}
module_init(phytium_pcie_link_pmu_module_init);

static void __exit phytium_pcie_link_pmu_module_exit(void)
{
	platform_driver_unregister(&phytium_pcie_link_pmu_driver);
	cpuhp_remove_multi_state(phytium_pcie_link_pmu_hp_state);
}
module_exit(phytium_pcie_link_pmu_module_exit);

MODULE_DESCRIPTION("Phytium PCIE LINK PMU driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(PCIE_LINK_PERF_DRIVER_VERSION);
MODULE_AUTHOR("Fu Boyi <fuboyi2150@phytium.com.cn>");
