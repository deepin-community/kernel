// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium Soc PCIe performance monitoring unit support
 *
 * Copyright (c) 2023, Phytium Technology Co., Ltd.
 */

#include <linux/acpi.h>
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

#if IS_ENABLED(CONFIG_ARM || CONFIG_ARM64)
#include <asm/cputype.h>
#endif /* CONFIG_ARM || CONFIG_ARM64 */
#include <asm/local64.h>

#undef pr_fmt
#define pr_fmt(fmt) "phytium_pcie_pmu: " fmt

#define PCIE_PERF_DRIVER_VERSION "1.4.0"

#define PHYTIUM_PCIE_MAX_COUNTERS       112
#define PHYTIUM_PCIE_V1_COUNTERS_NUM    18
#define PHYTIUM_PCIE_V2_COUNTERS_NUM    112
#define PHYTIUM_PCIE_EVENTS_MAX_MASK    0x7F

#define PCIE_START_TIMER		0x000
#define PCIE_STOP_TIMER			0x004
#define PCIE_CLEAR_EVENT		0x008
#define PCIE_SET_TIMER_L		0x00C
#define PCIE_SET_TIMER_H		0x010
#define PCIE_TRIG_MODE			0x014
#define PCIE_NOW_STATE			0x0E0
#define PCIE_STATE_STOP			0x0EC
#define PCIE_EVENT_THRESHOLD_EN		0x01C
#define PCIE_EVENT_THRESHOLD_0		0x020
#define PCIE_EVENT_THRESHOLD_1		0x024
#define PCIE_PMU_OVER_STATE		0x2

#define PCIE_TPOINT_END_L	0x0E4
#define PCIE_TPOINT_END_H	0x0E8

#define PCIE_V2_PMU_TIME_RG_THD_0	0x030
#define PCIE_V2_PMU_TIME_RG_THD_1	0x034
#define PCIE_V2_PMU_TIME_RG_THD_2	0x038
#define PCIE_V2_PMU_SORT_MODE		0x080
#define PCIE_V2_PMU_REQID_CFG		0x084
#define PCIE_V2_PMU_AT_CFG		0x090
#define PCIE_V2_PMU_MEM_STOP_EN		0x094
#define PCIE_V2_PMU_NOW_STATE		0x0E0
#define PCIE_V2_PMU_TPOINT_END_L	0x0E4
#define PCIE_V2_PMU_TPOINT_END_H	0x0E8
#define PCIE_V2_PMU_STATE_STOP		0x0EC

#define PCIE_V2_TIMEOUT_EN		0x600
#define PCIE_V2_TIMEOUT_THRESHOLD	0x604

#define PCIE_V2_PMU_CLK_EN		0x650
#define PCIE_V2_PMU_CHANNEL_SEL		0x654

#define PCIEA_V2P0_CHANNEL_NUM 8
#define PCIEB_V2P0_CHANNEL_NUM 6
#define PCIEC_V2P0_CHANNEL_NUM 5

#define PCIE_EVENT_CYCLES		0x0E4
#define PCIE_EVENT_AW			0x100
#define PCIE_EVENT_W_LAST		0x104
#define PCIE_EVENT_B			0x108
#define PCIE_EVENT_AR			0x10c
#define PCIE_EVENT_R_LAST		0x110
#define PCIE_EVENT_R_FULL		0x114
#define PCIE_EVENT_R_ERR		0x118
#define PCIE_EVENT_W_ERR		0x11c
#define PCIE_EVENT_W_DATA		0x200
#define PCIE_W_DATA_BASE		0x200

#define PCIE_EVENT_DELAY_RD		0x120
#define PCIE_EVENT_DELAY_WR		0x124
#define PCIE_EVENT_RD_MAX		0x128
#define PCIE_EVENT_RD_MIN		0x12c
#define PCIE_EVENT_WR_MAX		0x130
#define PCIE_EVENT_WR_MIN		0x134
#define PCIE_EVENT_RDELAY_TIME	0x300
#define PCIE_EVENT_WDELAY_TIME	0x700

#define PCIE_V2_G0_EVENT_AW			0x100
#define PCIE_V2_G0_EVENT_W_LAST		0x104
#define PCIE_V2_G0_EVENT_B			0x108
#define PCIE_V2_G0_EVENT_AR			0x10C
#define PCIE_V2_G0_EVENT_R_LAST		0x110
#define PCIE_V2_G0_EVENT_R_FULL		0x114
#define PCIE_V2_G0_EVENT_R_ERR		0x118
#define PCIE_V2_G0_EVENT_W_ERR		0x11C
#define PCIE_V2_G0_EVENT_ATOMIC_AW	0x120
#define PCIE_V2_G0_EVENT_R_DISCARDED 0x124
#define PCIE_V2_G0_EVENT_AT_AW		0x128
#define PCIE_V2_G0_EVENT_AT_AR		0x12C
#define PCIE_V2_G0_EVENT_W_DATA_AW_L 0x140
#define PCIE_V2_G0_EVENT_W_DATA_AW_H 0x144
#define PCIE_V2_EVENT_W_DATA		0x200
#define PCIE_V2_G0_EVENT_DELAY_RD	0x400
#define PCIE_V2_G0_EVENT_DELAY_WR	0x404
#define PCIE_V2_G0_EVENT_DELAY_RD_MAX 0x408
#define PCIE_V2_G0_EVENT_DELAY_RD_MIN 0x40C
#define PCIE_V2_G0_EVENT_DELAY_WR_MAX 0x410
#define PCIE_V2_G0_EVENT_DELAY_WR_MIN 0x414
#define PCIE_V2_G0_EVENT_DELAY_RD_TOTAL_H 0x418
#define PCIE_V2_G0_EVENT_DELAY_RD_TOTAL_L 0x41C
#define PCIE_V2_G0_EVENT_DELAY_WR_TOTAL_H 0x420
#define PCIE_V2_G0_EVENT_DELAY_WR_TOTAL_L 0x424
#define PCIE_V2_G0_EVENT_DELAY_RD_0 0x428
#define PCIE_V2_G0_EVENT_DELAY_RD_1 0x42C
#define PCIE_V2_G0_EVENT_DELAY_RD_2 0x430
#define PCIE_V2_G0_EVENT_DELAY_RD_3 0x434
#define PCIE_V2_G0_EVENT_DELAY_WR_0 0x438
#define PCIE_V2_G0_EVENT_DELAY_WR_1 0x43C
#define PCIE_V2_G0_EVENT_DELAY_WR_2 0x440
#define PCIE_V2_G0_EVENT_DELAY_WR_3 0x444

#define PCIE_V2_G1_EVENT_AW 0x900
#define PCIE_V2_G1_EVENT_B 0x908
#define PCIE_V2_G1_EVENT_AR 0x90C
#define PCIE_V2_G1_EVENT_R_LAST 0x910
#define PCIE_V2_G1_EVENT_R_FULL 0x914
#define PCIE_V2_G1_EVENT_R_ERR 0x918
#define PCIE_V2_G1_EVENT_W_ERR 0x91C
#define PCIE_V2_G1_EVENT_ATOMIC_AW 0x920
#define PCIE_V2_G1_EVENT_AT_AW 0x928
#define PCIE_V2_G1_EVENT_AT_AR 0x92C
#define PCIE_V2_G1_EVENT_W_DATA_AW_L 0x940
#define PCIE_V2_G1_EVENT_W_DATA_AW_H 0x944
#define PCIE_V2_G1_EVENT_DELAY_RD 0xa00
#define PCIE_V2_G1_EVENT_DELAY_WR 0xa04
#define PCIE_V2_G1_EVENT_DELAY_RD_MAX 0xa08
#define PCIE_V2_G1_EVENT_DELAY_RD_MIN 0xa0C
#define PCIE_V2_G1_EVENT_DELAY_WR_MAX 0xa10
#define PCIE_V2_G1_EVENT_DELAY_WR_MIN 0xa14
#define PCIE_V2_G1_EVENT_DELAY_RD_TOTAL_H 0xa18
#define PCIE_V2_G1_EVENT_DELAY_RD_TOTAL_L 0xa1C
#define PCIE_V2_G1_EVENT_DELAY_WR_TOTAL_H 0xa20
#define PCIE_V2_G1_EVENT_DELAY_WR_TOTAL_L 0xa24
#define PCIE_V2_G1_EVENT_DELAY_RD_0 0xa28
#define PCIE_V2_G1_EVENT_DELAY_RD_1 0xa2C
#define PCIE_V2_G1_EVENT_DELAY_RD_2 0xa30
#define PCIE_V2_G1_EVENT_DELAY_RD_3 0xa34
#define PCIE_V2_G1_EVENT_DELAY_WR_0 0xa38
#define PCIE_V2_G1_EVENT_DELAY_WR_1 0xa3C
#define PCIE_V2_G1_EVENT_DELAY_WR_2 0xa40
#define PCIE_V2_G1_EVENT_DELAY_WR_3 0xa44

#define PCIE_V2_G2_EVENT_AW 0xb00
#define PCIE_V2_G2_EVENT_B 0xb08
#define PCIE_V2_G2_EVENT_AR 0xb0C
#define PCIE_V2_G2_EVENT_R_LAST 0xb10
#define PCIE_V2_G2_EVENT_R_FULL 0xb14
#define PCIE_V2_G2_EVENT_R_ERR 0xb18
#define PCIE_V2_G2_EVENT_W_ERR 0xb1C
#define PCIE_V2_G2_EVENT_ATOMIC_AW 0xb20
#define PCIE_V2_G2_EVENT_AT_AW 0xb28
#define PCIE_V2_G2_EVENT_AT_AR 0xb2C
#define PCIE_V2_G2_EVENT_W_DATA_AW_L 0xb40
#define PCIE_V2_G2_EVENT_W_DATA_AW_H 0xb44
#define PCIE_V2_G2_EVENT_DELAY_RD 0xc00
#define PCIE_V2_G2_EVENT_DELAY_WR 0xc04
#define PCIE_V2_G2_EVENT_DELAY_RD_MAX 0xc08
#define PCIE_V2_G2_EVENT_DELAY_RD_MIN 0xc0C
#define PCIE_V2_G2_EVENT_DELAY_WR_MAX 0xc10
#define PCIE_V2_G2_EVENT_DELAY_WR_MIN 0xc14
#define PCIE_V2_G2_EVENT_DELAY_RD_TOTAL_H 0xc18
#define PCIE_V2_G2_EVENT_DELAY_RD_TOTAL_L 0xc1C
#define PCIE_V2_G2_EVENT_DELAY_WR_TOTAL_H 0xc20
#define PCIE_V2_G2_EVENT_DELAY_WR_TOTAL_L 0xc24
#define PCIE_V2_G2_EVENT_DELAY_RD_0 0xc28
#define PCIE_V2_G2_EVENT_DELAY_RD_1 0xc2C
#define PCIE_V2_G2_EVENT_DELAY_RD_2 0xc30
#define PCIE_V2_G2_EVENT_DELAY_RD_3 0xc34
#define PCIE_V2_G2_EVENT_DELAY_WR_0 0xc38
#define PCIE_V2_G2_EVENT_DELAY_WR_1 0xc3C
#define PCIE_V2_G2_EVENT_DELAY_WR_2 0xc40
#define PCIE_V2_G2_EVENT_DELAY_WR_3 0xc44

#define PCIE_V2_G3_EVENT_AW 0xd00
#define PCIE_V2_G3_EVENT_B 0xd08
#define PCIE_V2_G3_EVENT_AR 0xd0C
#define PCIE_V2_G3_EVENT_R_LAST 0xd10
#define PCIE_V2_G3_EVENT_R_FULL 0xd14
#define PCIE_V2_G3_EVENT_R_ERR 0xd18
#define PCIE_V2_G3_EVENT_W_ERR 0xd1C
#define PCIE_V2_G3_EVENT_ATOMIC_AW 0xd20
#define PCIE_V2_G3_EVENT_AT_AW 0xd28
#define PCIE_V2_G3_EVENT_AT_AR 0xd2C
#define PCIE_V2_G3_EVENT_W_DATA_AW_L 0xd40
#define PCIE_V2_G3_EVENT_W_DATA_AW_H 0xd44
#define PCIE_V2_G3_EVENT_DELAY_RD 0xe00
#define PCIE_V2_G3_EVENT_DELAY_WR 0xe04
#define PCIE_V2_G3_EVENT_DELAY_RD_MAX 0xe08
#define PCIE_V2_G3_EVENT_DELAY_RD_MIN 0xe0C
#define PCIE_V2_G3_EVENT_DELAY_WR_MAX 0xe10
#define PCIE_V2_G3_EVENT_DELAY_WR_MIN 0xe14
#define PCIE_V2_G3_EVENT_DELAY_RD_TOTAL_H 0xe18
#define PCIE_V2_G3_EVENT_DELAY_RD_TOTAL_L 0xe1C
#define PCIE_V2_G3_EVENT_DELAY_WR_TOTAL_H 0xe20
#define PCIE_V2_G3_EVENT_DELAY_WR_TOTAL_L 0xe24
#define PCIE_V2_G3_EVENT_DELAY_RD_0 0xe28
#define PCIE_V2_G3_EVENT_DELAY_RD_1 0xe2C
#define PCIE_V2_G3_EVENT_DELAY_RD_2 0xe30
#define PCIE_V2_G3_EVENT_DELAY_RD_3 0xe34
#define PCIE_V2_G3_EVENT_DELAY_WR_0 0xe38
#define PCIE_V2_G3_EVENT_DELAY_WR_1 0xe3C
#define PCIE_V2_G3_EVENT_DELAY_WR_2 0xe40
#define PCIE_V2_G3_EVENT_DELAY_WR_3 0xe44

#define PCIE_V1_AXI_CLK_FRE		0xE00
#define PCIE_V1_DATA_WIDTH		0xE04

#define PCIE_V2_AXI_CLK_FRE		0xF00
#define PCIE_V2_DATA_WIDTH		0xF04

#define PCIE_PMU_PIDR			0xFE0
#define PCIE_PMU_VER_BIT        GENMASK(7, 0)
#define PCIE_PMU_PART_BIT       GENMASK(11, 8)
#define PMU_CHANNEL_SEL_BIT     GENMASK(2, 0)

#define PCIEV2_PMU_CLK_EN_BIT	BIT(0)
#define PCIE_EVENT_AW_THD_ENBIT	BIT(0)
#define PCIE_EVENT_AR_THD_ENBIT	BIT(1)

#define PCIE_EVENT_THD0_STOP_TYPE	0x1
#define PCIE_EVENT_THD1_STOP_TYPE	0x2
#define PCIE_TIME_STOP_TYPE		0x10

#define PCIE_PMU_DEFAULT_THD0	1000000
#define PCIE_PMU_DEFAULT_THD1	10000000
#define PCIE_PMU_DEFAULT_THD2	100000000

#define to_phytium_pcie_pmu(p) (container_of(p, struct phytium_pcie_pmu, pmu))

#define GET_PCIE_EVENTID(hwc) (hwc->config_base & PHYTIUM_PCIE_EVENTS_MAX_MASK)
#define EVENT_VALID_V1(idx) ((idx >= 0) && (idx < PHYTIUM_PCIE_V1_COUNTERS_NUM))
#define EVENT_VALID_V2(idx) ((idx >= 0) && (idx < PHYTIUM_PCIE_V2_COUNTERS_NUM))

enum {
	PCIEA_V1P0 = 1,
	PCIEA_V1P5 = 3,
	PCIEA_V2P0 = 5,
	PCIEB_V2P0,
	PCIEC_V2P0,
};

static int phytium_pcie_pmu_hp_state;

struct phytium_pcie_pmu_hwevents {
	struct perf_event *hw_events[PHYTIUM_PCIE_MAX_COUNTERS];
	DECLARE_BITMAP(used_mask, PHYTIUM_PCIE_MAX_COUNTERS);
};

struct phytium_pcie_pmu;

struct phytium_pcie_pmu_ops {
	void (*get_event_config)(struct perf_event *event, struct phytium_pcie_pmu *pmu);
	int (*set_event_config)(struct perf_event *event, struct phytium_pcie_pmu *pmu);
	void (*reset_event_config)(struct phytium_pcie_pmu *pmu);
	u64 (*read_counter)(struct phytium_pcie_pmu *pmu, struct hw_perf_event *event);
	void (*enable_clk)(struct phytium_pcie_pmu *pmu);
	void (*disable_clk)(struct phytium_pcie_pmu *pmu);
	void (*clear_all_counters)(struct phytium_pcie_pmu *pmu);
	void (*start_all_counters)(struct phytium_pcie_pmu *pmu);
	void (*stop_all_counters)(struct phytium_pcie_pmu *pmu);
	unsigned long (*get_stop_state)(struct phytium_pcie_pmu *pmu);
	u32 (*get_counter_offset)(u32 idx);
	void (*reset_pmu)(struct phytium_pcie_pmu *pmu);
};

struct phytium_pcie_pmu_event_cfg {
	u32 chansel;
	u32 by_timer;
	u32 by_trig_mode;
	u32 delay_thd0_us;
	u32 delay_thd1_us;
	u32 delay_thd2_us;
	u32 by_evthod;
	u32 aw_threshold;
	u32 ar_threshold;
	u32 by_reqid;
	u32 bdf_id;
	u32 sort_mode;
	u32 byargs0;
	u32 byargs1;
	u64 timer_64;
};

struct phytium_pcie_pmu {
	struct device *dev;
	void __iomem *base;
	void __iomem *cfg_base;
	void __iomem *irq_reg;
	struct pmu pmu;
	const struct phytium_pcie_pmu_ops *ops;
	struct phytium_pcie_pmu_hwevents pmu_events;
	struct phytium_pcie_pmu_event_cfg event_cfg;
	u32 die_id;
	u32 dev_id;
	u32 pcie_id;
	u32 pmu_id;
	int on_cpu;
	int irq;
	int irq_bit;
	int cnts_num;
	int channel_num;
	struct hlist_node node;
	int ctrler_id;
	int real_ctrler;
	u32 clk_bits;
	u32 ver;
};

static const char pcie_type_part_name[] = {
	'a', 'b', 'c'
};

static const u32 pcie_v1_counter_reg_offset[] = {
	PCIE_EVENT_CYCLES,   PCIE_EVENT_AW,	     PCIE_EVENT_W_LAST,
	PCIE_EVENT_B,	     PCIE_EVENT_AR,	     PCIE_EVENT_R_LAST,
	PCIE_EVENT_R_FULL,   PCIE_EVENT_R_ERR,	     PCIE_EVENT_W_ERR,
	PCIE_EVENT_DELAY_RD, PCIE_EVENT_DELAY_WR,    PCIE_EVENT_RD_MAX,
	PCIE_EVENT_RD_MIN,   PCIE_EVENT_WR_MAX,	     PCIE_EVENT_WR_MIN,
	PCIE_EVENT_W_DATA,   PCIE_EVENT_RDELAY_TIME, PCIE_EVENT_WDELAY_TIME
};

static const u32 pcie_v2_counter_reg_offset[] = {
	PCIE_EVENT_CYCLES,
	PCIE_V2_G0_EVENT_AW, PCIE_V2_G0_EVENT_B, PCIE_V2_G0_EVENT_AR,
	PCIE_V2_G0_EVENT_R_LAST, PCIE_V2_G0_EVENT_R_FULL,
	PCIE_V2_G0_EVENT_R_ERR,	PCIE_V2_G0_EVENT_W_ERR,
	PCIE_V2_G0_EVENT_ATOMIC_AW,	PCIE_V2_G0_EVENT_AT_AW,	PCIE_V2_G0_EVENT_AT_AR,
	PCIE_V2_G0_EVENT_W_DATA_AW_L, PCIE_V2_G0_EVENT_DELAY_RD, PCIE_V2_G0_EVENT_DELAY_WR,
	PCIE_V2_G0_EVENT_DELAY_RD_MAX, PCIE_V2_G0_EVENT_DELAY_RD_MIN,
	PCIE_V2_G0_EVENT_DELAY_WR_MAX, PCIE_V2_G0_EVENT_DELAY_WR_MIN,
	PCIE_V2_G0_EVENT_DELAY_RD_TOTAL_H, PCIE_V2_G0_EVENT_DELAY_WR_TOTAL_H,
	PCIE_V2_G0_EVENT_DELAY_RD_0, PCIE_V2_G0_EVENT_DELAY_RD_1,
	PCIE_V2_G0_EVENT_DELAY_RD_2, PCIE_V2_G0_EVENT_DELAY_RD_3,
	PCIE_V2_G0_EVENT_DELAY_WR_0, PCIE_V2_G0_EVENT_DELAY_WR_1,
	PCIE_V2_G0_EVENT_DELAY_WR_2, PCIE_V2_G0_EVENT_DELAY_WR_3,
	PCIE_V2_EVENT_W_DATA, PCIE_V2_G0_EVENT_W_LAST, PCIE_V2_G0_EVENT_R_DISCARDED,
	PCIE_V2_G1_EVENT_AW, PCIE_V2_G1_EVENT_B, PCIE_V2_G1_EVENT_AR,
	PCIE_V2_G1_EVENT_R_LAST, PCIE_V2_G1_EVENT_R_FULL,
	PCIE_V2_G1_EVENT_R_ERR, PCIE_V2_G1_EVENT_W_ERR,
	PCIE_V2_G1_EVENT_ATOMIC_AW, PCIE_V2_G1_EVENT_AT_AW,	PCIE_V2_G1_EVENT_AT_AR,
	PCIE_V2_G1_EVENT_W_DATA_AW_L, PCIE_V2_G1_EVENT_DELAY_RD, PCIE_V2_G1_EVENT_DELAY_WR,
	PCIE_V2_G1_EVENT_DELAY_RD_MAX, PCIE_V2_G1_EVENT_DELAY_RD_MIN,
	PCIE_V2_G1_EVENT_DELAY_WR_MAX, PCIE_V2_G1_EVENT_DELAY_WR_MIN,
	PCIE_V2_G1_EVENT_DELAY_RD_TOTAL_H, PCIE_V2_G1_EVENT_DELAY_WR_TOTAL_H,
	PCIE_V2_G1_EVENT_DELAY_RD_0, PCIE_V2_G1_EVENT_DELAY_RD_1,
	PCIE_V2_G1_EVENT_DELAY_RD_2, PCIE_V2_G1_EVENT_DELAY_RD_3,
	PCIE_V2_G1_EVENT_DELAY_WR_0, PCIE_V2_G1_EVENT_DELAY_WR_1,
	PCIE_V2_G1_EVENT_DELAY_WR_2, PCIE_V2_G1_EVENT_DELAY_WR_3,
	PCIE_V2_G2_EVENT_AW, PCIE_V2_G2_EVENT_B, PCIE_V2_G2_EVENT_AR,
	PCIE_V2_G2_EVENT_R_LAST, PCIE_V2_G2_EVENT_R_FULL,
	PCIE_V2_G2_EVENT_R_ERR,	PCIE_V2_G2_EVENT_W_ERR,
	PCIE_V2_G2_EVENT_ATOMIC_AW,	PCIE_V2_G2_EVENT_AT_AW, PCIE_V2_G2_EVENT_AT_AR,
	PCIE_V2_G2_EVENT_W_DATA_AW_L, PCIE_V2_G2_EVENT_DELAY_RD, PCIE_V2_G2_EVENT_DELAY_WR,
	PCIE_V2_G2_EVENT_DELAY_RD_MAX, PCIE_V2_G2_EVENT_DELAY_RD_MIN,
	PCIE_V2_G2_EVENT_DELAY_WR_MAX, PCIE_V2_G2_EVENT_DELAY_WR_MIN,
	PCIE_V2_G2_EVENT_DELAY_RD_TOTAL_H, PCIE_V2_G2_EVENT_DELAY_WR_TOTAL_H,
	PCIE_V2_G2_EVENT_DELAY_RD_0, PCIE_V2_G2_EVENT_DELAY_RD_1,
	PCIE_V2_G2_EVENT_DELAY_RD_2, PCIE_V2_G2_EVENT_DELAY_RD_3,
	PCIE_V2_G2_EVENT_DELAY_WR_0, PCIE_V2_G2_EVENT_DELAY_WR_1,
	PCIE_V2_G2_EVENT_DELAY_WR_2, PCIE_V2_G2_EVENT_DELAY_WR_3,
	PCIE_V2_G3_EVENT_AW, PCIE_V2_G3_EVENT_B, PCIE_V2_G3_EVENT_AR,
	PCIE_V2_G3_EVENT_R_LAST, PCIE_V2_G3_EVENT_R_FULL,
	PCIE_V2_G3_EVENT_R_ERR, PCIE_V2_G3_EVENT_W_ERR,
	PCIE_V2_G3_EVENT_ATOMIC_AW, PCIE_V2_G3_EVENT_AT_AW,
	PCIE_V2_G3_EVENT_AT_AR, PCIE_V2_G3_EVENT_W_DATA_AW_L,
	PCIE_V2_G3_EVENT_DELAY_RD, PCIE_V2_G3_EVENT_DELAY_WR,
	PCIE_V2_G3_EVENT_DELAY_RD_MAX, PCIE_V2_G3_EVENT_DELAY_RD_MIN,
	PCIE_V2_G3_EVENT_DELAY_WR_MAX, PCIE_V2_G3_EVENT_DELAY_WR_MIN,
	PCIE_V2_G3_EVENT_DELAY_RD_TOTAL_H, PCIE_V2_G3_EVENT_DELAY_WR_TOTAL_H,
	PCIE_V2_G3_EVENT_DELAY_RD_0, PCIE_V2_G3_EVENT_DELAY_RD_1,
	PCIE_V2_G3_EVENT_DELAY_RD_2, PCIE_V2_G3_EVENT_DELAY_RD_3,
	PCIE_V2_G3_EVENT_DELAY_WR_0, PCIE_V2_G3_EVENT_DELAY_WR_1,
	PCIE_V2_G3_EVENT_DELAY_WR_2, PCIE_V2_G3_EVENT_DELAY_WR_3
};

ssize_t phytium_pcie_pmu_format_sysfs_show(struct device *dev,
		   struct device_attribute *attr,
		   char *buf)
{
	struct dev_ext_attribute *eattr;

	eattr = container_of(attr, struct dev_ext_attribute, attr);

	return sprintf(buf, "%s\n", (char *)eattr->var);
}

ssize_t phytium_pcie_pmu_event_sysfs_show(struct device *dev,
		  struct device_attribute *attr,
		  char *page)
{
	struct dev_ext_attribute *eattr;

	eattr = container_of(attr, struct dev_ext_attribute, attr);

	return sprintf(page, "config=0x%lx\n", (unsigned long)eattr->var);
}

static ssize_t cpumask_show(struct device *dev, struct device_attribute *attr,
		     char *buf)
{
	struct phytium_pcie_pmu *pcie_pmu =
		to_phytium_pcie_pmu(dev_get_drvdata(dev));

	return cpumap_print_to_pagebuf(true, buf, cpumask_of(pcie_pmu->on_cpu));
}

#define PHYTIUM_PMU_ATTR(_name, _func, _config)                             \
	(&((struct dev_ext_attribute[]){                                    \
		{ __ATTR(_name, 0444, _func, NULL), (void *)_config } })[0] \
		  .attr.attr)

#define PHYTIUM_PCIE_PMU_FORMAT_ATTR(_name, _config)            \
	PHYTIUM_PMU_ATTR(_name, phytium_pcie_pmu_format_sysfs_show, \
			 (void *)_config)

#define PHYTIUM_PCIE_PMU_EVENT_ATTR(_name, _config)                \
	PHYTIUM_PMU_ATTR(_name, phytium_pcie_pmu_event_sysfs_show, \
			 (unsigned long)_config)

#define PCIE_PMU_V1_EVENT_ATTR_EXTRACTOR_U32(_name, _config, _start, _end)        \
static inline u32 pcie_pmu_v1_get_u32_##_name(struct perf_event *event)            \
{                                                                  \
	return FIELD_GET(GENMASK_ULL(_end, _start),                \
			event->attr._config);                     \
}

#define PCIE_PMU_V1_EVENT_ATTR_EXTRACTOR_U64(_name, _config, _start, _end)        \
static inline u64 pcie_pmu_v1_get_u64_##_name(struct perf_event *event)            \
{                                                                  \
	return FIELD_GET(GENMASK_ULL(_end, _start),                \
			event->attr._config);                     \
}

#define PCIE_PMU_V2_EVENT_ATTR_EXTRACTOR(_name, _config, _start, _end)        \
static inline u32 pcie_pmu_v2_get_##_name(struct perf_event *event)            \
{                                                                  \
	return FIELD_GET(GENMASK_ULL(_end, _start),                \
			event->attr._config);                     \
}

PCIE_PMU_V1_EVENT_ATTR_EXTRACTOR_U32(event, config, 0, 4);
PCIE_PMU_V1_EVENT_ATTR_EXTRACTOR_U32(ctrler, config, 8, 10);
PCIE_PMU_V1_EVENT_ATTR_EXTRACTOR_U64(timer, config1, 0, 63);

static struct attribute *phytium_pcie_pmu_v1_format_attr[] = {
	PHYTIUM_PCIE_PMU_FORMAT_ATTR(event, "config:0-4"),
	PHYTIUM_PCIE_PMU_FORMAT_ATTR(ctrler, "config:8-10"),
	PHYTIUM_PCIE_PMU_FORMAT_ATTR(timer, "config1:0-63"),
	NULL,
};

PCIE_PMU_V2_EVENT_ATTR_EXTRACTOR(event, config, 0, 6);
PCIE_PMU_V2_EVENT_ATTR_EXTRACTOR(chansel, config, 7, 9);
PCIE_PMU_V2_EVENT_ATTR_EXTRACTOR(by_timer, config, 10, 10);
PCIE_PMU_V2_EVENT_ATTR_EXTRACTOR(by_evthod, config, 11, 11);
PCIE_PMU_V2_EVENT_ATTR_EXTRACTOR(by_trig_mode, config, 12, 12);
PCIE_PMU_V2_EVENT_ATTR_EXTRACTOR(sort_mode, config, 13, 14);
PCIE_PMU_V2_EVENT_ATTR_EXTRACTOR(bdf_id, config, 16, 31);
PCIE_PMU_V2_EVENT_ATTR_EXTRACTOR(delay_thd0_us, config, 32, 63);
PCIE_PMU_V2_EVENT_ATTR_EXTRACTOR(delay_thd1_us, config1, 0, 31);
PCIE_PMU_V2_EVENT_ATTR_EXTRACTOR(delay_thd2_us, config1, 32, 63);
PCIE_PMU_V2_EVENT_ATTR_EXTRACTOR(byargs0, config2, 0, 31);
PCIE_PMU_V2_EVENT_ATTR_EXTRACTOR(byargs1, config2, 32, 63);

static struct attribute *phytium_pcie_pmu_v2_format_attr[] = {
	PHYTIUM_PCIE_PMU_FORMAT_ATTR(event, "config:0-6"),
	PHYTIUM_PCIE_PMU_FORMAT_ATTR(chansel, "config:7-9"),
	PHYTIUM_PCIE_PMU_FORMAT_ATTR(by_timer, "config:10-10"),
	PHYTIUM_PCIE_PMU_FORMAT_ATTR(by_evthod, "config:11-11"),
	PHYTIUM_PCIE_PMU_FORMAT_ATTR(by_trig_mode, "config:12-12"),
	PHYTIUM_PCIE_PMU_FORMAT_ATTR(sort_mode, "config:13-14"),
	PHYTIUM_PCIE_PMU_FORMAT_ATTR(bdf_id, "config:16-31"),
	PHYTIUM_PCIE_PMU_FORMAT_ATTR(delay_thd0_us, "config:32-63"),
	PHYTIUM_PCIE_PMU_FORMAT_ATTR(delay_thd1_us, "config1:0-31"),
	PHYTIUM_PCIE_PMU_FORMAT_ATTR(delay_thd2_us, "config1:32-63"),
	PHYTIUM_PCIE_PMU_FORMAT_ATTR(byargs0, "config2:0-31"),
	PHYTIUM_PCIE_PMU_FORMAT_ATTR(byargs1, "config2:32-63"),
	NULL,
};

static const struct attribute_group phytium_pcie_pmu_v1_format_group = {
	.name = "format",
	.attrs = phytium_pcie_pmu_v1_format_attr,
};

static const struct attribute_group phytium_pcie_pmu_v2_format_group = {
	.name = "format",
	.attrs = phytium_pcie_pmu_v2_format_attr,
};

static struct attribute *phytium_pcie_pmu_v1_events_attr[] = {
	PHYTIUM_PCIE_PMU_EVENT_ATTR(pcie_cycles, 0x00),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(aw, 0x01),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(w_last, 0x02),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(b, 0x03),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(ar, 0x04),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(r_last, 0x05),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(r_full, 0x06),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(r_err, 0x07),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(w_err, 0x08),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd, 0x09),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr, 0x0a),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(rd_max, 0x0b),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(rd_min, 0x0c),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(wr_max, 0x0d),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(wr_min, 0x0e),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(w_data, 0x0f),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(rdelay_time, 0x10),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(wdelay_time, 0x11),
	NULL,
};

static struct attribute *phytium_pcie_pmu_v2_events_attr[] = {
	PHYTIUM_PCIE_PMU_EVENT_ATTR(pcie_cycles, 0x0),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(aw_g0, 0x1),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(b_g0, 0x2),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(ar_g0, 0x3),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(r_last_g0, 0x4),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(r_full_g0, 0x5),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(r_err_g0, 0x6),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(w_err_g0, 0x7),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(atomic_aw_g0, 0x8),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(at_aw_g0, 0x9),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(at_ar_g0, 0xA),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(w_data_aw_g0, 0xB),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_g0, 0xC),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_g0, 0xD),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_max_g0, 0xE),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_min_g0, 0xF),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_max_g0, 0x10),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_min_g0, 0x11),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_total_g0, 0x12),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_total_g0, 0x13),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_0_g0, 0x14),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_1_g0, 0x15),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_2_g0, 0x16),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_3_g0, 0x17),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_0_g0, 0x18),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_1_g0, 0x19),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_2_g0, 0x1A),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_3_g0, 0x1B),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(w_data_g0, 0x1C),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(w_last_g0, 0x1D),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(r_discarded_g0, 0x1E),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(aw_g1, 0x1F),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(b_g1, 0x20),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(ar_g1, 0x21),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(r_last_g1, 0x22),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(r_full_g1, 0x23),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(r_err_g1, 0x24),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(w_err_g1, 0x25),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(atomic_aw_g1, 0x26),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(at_aw_g1, 0x27),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(at_ar_g1, 0x28),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(w_data_aw_g1, 0x29),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_g1, 0x2A),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_g1, 0x2B),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_max_g1, 0x2C),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_min_g1, 0x2D),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_max_g1, 0x2E),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_min_g1, 0x2F),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_total_g1, 0x30),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_total_g1, 0x31),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_0_g1, 0x32),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_1_g1, 0x33),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_2_g1, 0x34),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_3_g1, 0x35),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_0_g1, 0x36),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_1_g1, 0x37),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_2_g1, 0x38),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_3_g1, 0x39),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(aw_g2, 0x3A),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(b_g2, 0x3B),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(ar_g2, 0x3C),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(r_last_g2, 0x3D),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(r_full_g2, 0x3E),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(r_err_g2, 0x3F),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(w_err_g2, 0x40),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(atomic_aw_g2, 0x41),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(at_aw_g2, 0x42),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(at_ar_g2, 0x43),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(w_data_aw_g2, 0x44),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_g2, 0x45),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_g2, 0x46),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_max_g2, 0x47),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_min_g2, 0x48),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_max_g2, 0x49),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_min_g2, 0x4A),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_total_g2, 0x4B),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_total_g2, 0x4C),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_0_g2, 0x4D),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_1_g2, 0x4E),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_2_g2, 0x4F),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_3_g2, 0x50),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_0_g2, 0x51),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_1_g2, 0x52),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_2_g2, 0x53),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_3_g2, 0x54),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(aw_g3, 0x55),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(b_g3, 0x56),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(ar_g3, 0x57),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(r_last_g3, 0x58),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(r_full_g3, 0x59),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(r_err_g3, 0x5A),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(w_err_g3, 0x5B),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(atomic_aw_g3, 0x5C),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(at_aw_g3, 0x5D),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(at_ar_g3, 0x5E),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(w_data_aw_g3, 0x5F),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_g3, 0x60),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_g3, 0x61),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_max_g3, 0x62),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_min_g3, 0x63),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_max_g3, 0x64),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_min_g3, 0x65),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_total_g3, 0x66),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_total_g3, 0x67),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_0_g3, 0x68),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_1_g3, 0x69),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_2_g3, 0x6A),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_rd_3_g3, 0x6B),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_0_g3, 0x6C),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_1_g3, 0x6D),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_2_g3, 0x6E),
	PHYTIUM_PCIE_PMU_EVENT_ATTR(delay_wr_3_g3, 0x6F),
	NULL,
};

static const struct attribute_group phytium_pcie_pmu_v1_events_group = {
	.name = "events",
	.attrs = phytium_pcie_pmu_v1_events_attr,
};

static const struct attribute_group phytium_pcie_pmu_v2_events_group = {
	.name = "events",
	.attrs = phytium_pcie_pmu_v2_events_attr,
};

static DEVICE_ATTR_RO(cpumask);

static struct attribute *phytium_pcie_pmu_cpumask_attrs[] = {
	&dev_attr_cpumask.attr,
	NULL,
};

static const struct attribute_group phytium_pcie_pmu_cpumask_attr_group = {
	.attrs = phytium_pcie_pmu_cpumask_attrs,
};

static const struct attribute_group *phytium_pcie_pmu_v1_attr_groups[] = {
	&phytium_pcie_pmu_v1_format_group,
	&phytium_pcie_pmu_v1_events_group,
	&phytium_pcie_pmu_cpumask_attr_group,
	NULL,
};

static const struct attribute_group *phytium_pcie_pmu_v2_attr_groups[] = {
	&phytium_pcie_pmu_v2_format_group,
	&phytium_pcie_pmu_v2_events_group,
	&phytium_pcie_pmu_cpumask_attr_group,
	NULL,
};

static void phytium_pcie_pmu_v1_set_timer(struct phytium_pcie_pmu *pcie_pmu, u64 th_val)
{
	u32 val_l, val_h;

	val_l = th_val & 0xFFFFFFFF;
	val_h = (th_val >> 32) & 0xFFFFFFFF;
	writel(val_l, pcie_pmu->base + PCIE_SET_TIMER_L);
	writel(val_h, pcie_pmu->base + PCIE_SET_TIMER_H);
}

static void phytium_pcie_pmu_v1_reset_timer(struct phytium_pcie_pmu *pcie_pmu)
{
	writel(0xFFFFFFFF, pcie_pmu->base + PCIE_SET_TIMER_L);
	writel(0xFFFFFFFF, pcie_pmu->base + PCIE_SET_TIMER_H);
}

static void phytium_pcie_pmu_v1_select_ctrler(struct phytium_pcie_pmu *pcie_pmu)
{
	u32 val, offset;
	u32 mask = 0xfffffffc;

	if (pcie_pmu->ver == PCIEA_V1P0) {
		if (pcie_pmu->pmu_id == 2) {
			mask = 0xffffffcf;
			offset = 0x0;
		} else
			offset = 0xc;
	} else {
		offset = 0x170;
	}

	val = readl(pcie_pmu->cfg_base + offset);
	val &= mask;
	val |= pcie_pmu->real_ctrler;
	writel(val, pcie_pmu->cfg_base + offset);
}

static void phytium_pcie_pmu_v2_set_timer(struct phytium_pcie_pmu *pcie_pmu,
					u64 th_val)
{
	u32 val_l, val_h;

	val_l = th_val & 0xFFFFFFFF;
	val_h = (th_val >> 32) & 0xFFFFFFFF;

	writel(val_l, pcie_pmu->base + PCIE_SET_TIMER_L);
	writel(val_h, pcie_pmu->base + PCIE_SET_TIMER_H);

}

static void phytium_pcie_pmu_v2_reset_timer(struct phytium_pcie_pmu *pcie_pmu)
{
	writel(0xFFFFFFFF, pcie_pmu->base + PCIE_SET_TIMER_L);
	writel(0xFFFFFFFF, pcie_pmu->base + PCIE_SET_TIMER_H);

}

static void phytium_pcie_pmu_v2_set_delay_threshold(struct phytium_pcie_pmu *pcie_pmu)
{
	u32 clk_fre;
	u32 thd0_val, thd1_val, thd2_val;

	// MHz
	clk_fre = readl(pcie_pmu->base + PCIE_V2_AXI_CLK_FRE);

	thd0_val = clk_fre * pcie_pmu->event_cfg.delay_thd0_us;
	thd1_val = clk_fre * pcie_pmu->event_cfg.delay_thd1_us;
	thd2_val = clk_fre * pcie_pmu->event_cfg.delay_thd2_us;

	writel(thd0_val, pcie_pmu->base + PCIE_V2_PMU_TIME_RG_THD_0);
	writel(thd1_val, pcie_pmu->base + PCIE_V2_PMU_TIME_RG_THD_1);
	writel(thd1_val, pcie_pmu->base + PCIE_V2_PMU_TIME_RG_THD_2);
}

static void phytium_pcie_pmu_v2_reset_delay_threshold(struct phytium_pcie_pmu *pcie_pmu)
{
	writel(PCIE_PMU_DEFAULT_THD0, pcie_pmu->base + PCIE_V2_PMU_TIME_RG_THD_0);
	writel(PCIE_PMU_DEFAULT_THD1, pcie_pmu->base + PCIE_V2_PMU_TIME_RG_THD_1);
	writel(PCIE_PMU_DEFAULT_THD2, pcie_pmu->base + PCIE_V2_PMU_TIME_RG_THD_2);
}

static void phytium_pcie_pmu_v2_set_event_trig_mode(struct phytium_pcie_pmu *pcie_pmu)
{
	writel(0x1, pcie_pmu->base + PCIE_TRIG_MODE);
}

static void phytium_pcie_pmu_v2_reset_event_trig_mode(struct phytium_pcie_pmu *pcie_pmu)
{
	writel(0, pcie_pmu->base + PCIE_TRIG_MODE);
}

static void phytium_pcie_pmu_v2_set_event_threshold(struct phytium_pcie_pmu *pcie_pmu,
					u32 val, u32 en_bit, u32 offset)
{
	writel(en_bit, pcie_pmu->base + PCIE_EVENT_THRESHOLD_EN);
	writel(val, pcie_pmu->base + offset);
}

static void phytium_pcie_pmu_v2_reset_event_threshold(struct phytium_pcie_pmu *pcie_pmu)
{
	writel(0x0, pcie_pmu->base + PCIE_EVENT_THRESHOLD_EN);
	writel(0xFFFFFFFF, pcie_pmu->base + PCIE_EVENT_THRESHOLD_0);
	writel(0xFFFFFFFF, pcie_pmu->base + PCIE_EVENT_THRESHOLD_1);
}

static void phytium_pcie_pmu_v2_select_monitor_channel(struct phytium_pcie_pmu *pcie_pmu)
{
	/*
	 *peu: 0:c0, 1:c1, 2:c2, 3:c3, 4:iommu_tbu, 5:qtw, 6:pio_in, 7:vdm_in
	 *pxu: 0:c0, 1:c1, 2:vdm, 3:pio, 4:qtw, 5:iommu_out
	 *pcu: 0:pio_in, 2:ncc dma_out, 3:iommu_out,4:c0 dma
	 */
	writel(pcie_pmu->real_ctrler, pcie_pmu->cfg_base + PCIE_V2_PMU_CHANNEL_SEL);
}

static void phytium_pcie_pmu_v2_set_event_group_sort_mode(struct phytium_pcie_pmu *pcie_pmu)
{
	writel(pcie_pmu->event_cfg.sort_mode, pcie_pmu->base + PCIE_V2_PMU_SORT_MODE);
}

static void phytium_pcie_pmu_v2_reset_event_group_sort_mode(struct phytium_pcie_pmu *pcie_pmu)
{
	writel(0, pcie_pmu->base + PCIE_V2_PMU_SORT_MODE);
}

static void phytium_pcie_pmu_v2_set_bdf_id_cfg(struct phytium_pcie_pmu *pcie_pmu, u32 bdf_id)
{
	bdf_id &= 0xFFFF;
	writel(bdf_id, pcie_pmu->base + PCIE_V2_PMU_REQID_CFG);
}

static void phytium_pcie_pmu_v2_reset_bdf_id_cfg(struct phytium_pcie_pmu *pcie_pmu)
{
	writel(0, pcie_pmu->base + PCIE_V2_PMU_REQID_CFG);
}

static void phytium_pcie_pmu_v1_get_event_config(struct perf_event *event,
					 struct phytium_pcie_pmu *pcie_pmu)
{
	pcie_pmu->event_cfg.chansel = pcie_pmu_v1_get_u32_ctrler(event);
	pcie_pmu->event_cfg.timer_64 = pcie_pmu_v1_get_u64_timer(event);
}

static void phytium_pcie_pmu_v2_get_event_config(struct perf_event *event,
					 struct phytium_pcie_pmu *pcie_pmu)
{
	u32 delay_thd0_us, delay_thd1_us, delay_thd2_us;

	pcie_pmu->event_cfg.chansel = pcie_pmu_v2_get_chansel(event);
	pcie_pmu->event_cfg.by_timer = pcie_pmu_v2_get_by_timer(event);
	pcie_pmu->event_cfg.by_evthod = pcie_pmu_v2_get_by_evthod(event);
	pcie_pmu->event_cfg.by_trig_mode = pcie_pmu_v2_get_by_trig_mode(event);
	pcie_pmu->event_cfg.sort_mode = pcie_pmu_v2_get_sort_mode(event);
	pcie_pmu->event_cfg.bdf_id = pcie_pmu_v2_get_bdf_id(event);
	pcie_pmu->event_cfg.byargs0 = pcie_pmu_v2_get_byargs0(event);
	pcie_pmu->event_cfg.byargs1 = pcie_pmu_v2_get_byargs1(event);
	delay_thd0_us = pcie_pmu_v2_get_delay_thd0_us(event);
	delay_thd1_us = pcie_pmu_v2_get_delay_thd1_us(event);
	delay_thd2_us = pcie_pmu_v2_get_delay_thd2_us(event);
	if (delay_thd0_us + delay_thd1_us + delay_thd2_us == 0) {
		pcie_pmu->event_cfg.delay_thd0_us = PCIE_PMU_DEFAULT_THD0;
		pcie_pmu->event_cfg.delay_thd1_us = PCIE_PMU_DEFAULT_THD1;
		pcie_pmu->event_cfg.delay_thd2_us = PCIE_PMU_DEFAULT_THD2;
	} else {
		pcie_pmu->event_cfg.delay_thd0_us = delay_thd0_us;
		pcie_pmu->event_cfg.delay_thd1_us = delay_thd1_us;
		pcie_pmu->event_cfg.delay_thd2_us = delay_thd2_us;
	}

}

static int phytium_pcie_pmu_v1_set_event_config(struct perf_event *event,
					 struct phytium_pcie_pmu *pcie_pmu)
{
	if (pcie_pmu->event_cfg.timer_64 > 0)
		phytium_pcie_pmu_v1_set_timer(pcie_pmu, pcie_pmu->event_cfg.timer_64);

	if (pcie_pmu->ver == PCIEA_V1P5) {
		if (pcie_pmu->pmu_id == 2) {
			if (pcie_pmu->event_cfg.chansel == 0)
				pcie_pmu->event_cfg.chansel = 2;
			else if ((pcie_pmu->event_cfg.chansel < 2) ||
					(pcie_pmu->event_cfg.chansel > 3)) {
				dev_warn(pcie_pmu->dev, "Wrong ctrler id(%d) for pciea%u-pmu2!\n",
					pcie_pmu->event_cfg.chansel, pcie_pmu->pcie_id);
				return -EINVAL;
			}
			if (pcie_pmu->ctrler_id != pcie_pmu->event_cfg.chansel) {
				pcie_pmu->ctrler_id = pcie_pmu->event_cfg.chansel;
				pcie_pmu->real_ctrler = pcie_pmu->ctrler_id;
				phytium_pcie_pmu_v1_select_ctrler(pcie_pmu);
			}
		} else {
			if (pcie_pmu->event_cfg.chansel != 0) {
				dev_warn(pcie_pmu->dev, "Don't set ctrler id(%d) for pciea%u-pmu%d!\n",
					pcie_pmu->event_cfg.chansel, pcie_pmu->pcie_id,
					pcie_pmu->pmu_id);
				return -EINVAL;
			}
			pcie_pmu->ctrler_id = pcie_pmu->pmu_id;
			pcie_pmu->real_ctrler = pcie_pmu->ctrler_id;
		}
	} else if (pcie_pmu->ver == PCIEA_V1P0) {
		switch (pcie_pmu->pmu_id) {
		case 0:
			if (pcie_pmu->event_cfg.chansel != 0) {
				dev_warn(pcie_pmu->dev,
					"Wrong ctrler id(%d) for pciea%u-pmu0!\n",
					pcie_pmu->event_cfg.chansel, pcie_pmu->pcie_id);
				return -EINVAL;
			}
			break;
		case 1:
			if (pcie_pmu->event_cfg.chansel == 0)
				pcie_pmu->event_cfg.chansel = 1;
			else if ((pcie_pmu->event_cfg.chansel < 1) ||
					(pcie_pmu->event_cfg.chansel > 3)) {
				dev_warn(pcie_pmu->dev,
					"Wrong ctrler id(%d) for pciea%u-pmu1!\n",
					pcie_pmu->event_cfg.chansel, pcie_pmu->pcie_id);
				return -EINVAL;
			}
			break;
		case 2:
			if (pcie_pmu->event_cfg.chansel == 0)
				pcie_pmu->event_cfg.chansel = 4;
			else if ((pcie_pmu->event_cfg.chansel < 4) ||
					(pcie_pmu->event_cfg.chansel > 7)) {
				dev_warn(pcie_pmu->dev,
					"Wrong ctrler id(%d) for pciea%u-pmu2!\n",
					pcie_pmu->event_cfg.chansel, pcie_pmu->pcie_id);
				return -EINVAL;
			}
			break;
		default:
			dev_err(pcie_pmu->dev, "Unsupported pmu id:%d!\n",
				pcie_pmu->pmu_id);
			return -EINVAL;
		}

		pcie_pmu->ctrler_id = pcie_pmu->event_cfg.chansel;
		switch (pcie_pmu->pmu_id) {
		case 0:
		case 1:
			pcie_pmu->real_ctrler = pcie_pmu->ctrler_id;
			break;
		case 2:
			pcie_pmu->real_ctrler = (pcie_pmu->ctrler_id - 4) * 16;
			break;
		default:
			dev_err(pcie_pmu->dev, "Unsupported pmu id:%d!\n",
				pcie_pmu->pmu_id);
			return -EINVAL;
		}
		phytium_pcie_pmu_v1_select_ctrler(pcie_pmu);
	}

	return 0;
}

static int phytium_pcie_pmu_v2_set_event_config(struct perf_event *event,
					 struct phytium_pcie_pmu *pcie_pmu)
{
	pcie_pmu->ctrler_id = pcie_pmu->event_cfg.chansel;

	if (pcie_pmu->event_cfg.chansel >= pcie_pmu->channel_num) {
		dev_err(pcie_pmu->dev, "Wrong ctrler id(%d) for pcie-pmu!\n",
				pcie_pmu->event_cfg.chansel);
		return -EINVAL;
	}

	pcie_pmu->real_ctrler = pcie_pmu->ctrler_id;
	phytium_pcie_pmu_v2_select_monitor_channel(pcie_pmu);

	if ((pcie_pmu->event_cfg.delay_thd0_us < pcie_pmu->event_cfg.delay_thd1_us)
		&& (pcie_pmu->event_cfg.delay_thd1_us < pcie_pmu->event_cfg.delay_thd2_us)) {
		phytium_pcie_pmu_v2_set_delay_threshold(pcie_pmu);
	} else {
		dev_err(pcie_pmu->dev, "Wrong delay threshold value for pcie%u-pmu%d!\n",
				pcie_pmu->pcie_id, pcie_pmu->pmu_id);

		dev_err(pcie_pmu->dev, "Wrong value:(delay_thd0_us:%d,delay_thd1_us:%d,delay_thd2_us:%d).\n",
			pcie_pmu->event_cfg.delay_thd0_us, pcie_pmu->event_cfg.delay_thd1_us,
			pcie_pmu->event_cfg.delay_thd2_us);
		return -EINVAL;
	}

	if (pcie_pmu->event_cfg.by_trig_mode)
		phytium_pcie_pmu_v2_set_event_trig_mode(pcie_pmu);

	if (pcie_pmu->event_cfg.by_evthod) {
		if (pcie_pmu->event_cfg.by_timer) {
			dev_err(pcie_pmu->dev, "The by_ethod and by_timer are set incorrectly!");
			dev_err(pcie_pmu->dev, "Cannot be set for pcie%u-pmu%d at the same time!",
				pcie_pmu->pcie_id, pcie_pmu->pmu_id);
			return -EINVAL;
		}

		if (pcie_pmu->event_cfg.byargs0 > 0) {
			pcie_pmu->event_cfg.aw_threshold = pcie_pmu->event_cfg.byargs0;
			phytium_pcie_pmu_v2_set_event_threshold(pcie_pmu,
					pcie_pmu->event_cfg.aw_threshold,
					PCIE_EVENT_AW_THD_ENBIT, PCIE_EVENT_THRESHOLD_0);
		} else if (pcie_pmu->event_cfg.byargs1 > 0) {
			pcie_pmu->event_cfg.ar_threshold = pcie_pmu->event_cfg.byargs1;
			phytium_pcie_pmu_v2_set_event_threshold(pcie_pmu,
					pcie_pmu->event_cfg.ar_threshold,
					PCIE_EVENT_AR_THD_ENBIT, PCIE_EVENT_THRESHOLD_1);
		}
	} else if (pcie_pmu->event_cfg.by_timer == 1 && pcie_pmu->event_cfg.byargs0 > 0) {
		pcie_pmu->event_cfg.timer_64 = pcie_pmu->event_cfg.byargs0;
		phytium_pcie_pmu_v2_set_timer(pcie_pmu, pcie_pmu->event_cfg.timer_64);
	}

	if (pcie_pmu->event_cfg.sort_mode > 1)
		phytium_pcie_pmu_v2_set_bdf_id_cfg(pcie_pmu, pcie_pmu->event_cfg.bdf_id);

	phytium_pcie_pmu_v2_set_event_group_sort_mode(pcie_pmu);

	return 0;
}

static void phytium_pcie_pmu_v1_reset_event_config(struct phytium_pcie_pmu *pcie_pmu)
{
	phytium_pcie_pmu_v1_reset_timer(pcie_pmu);
	pcie_pmu->event_cfg.timer_64 = 0;
	pcie_pmu->event_cfg.chansel = -1;
}

static void phytium_pcie_pmu_v2_reset_event_config(struct phytium_pcie_pmu *pcie_pmu)
{
	phytium_pcie_pmu_v2_reset_timer(pcie_pmu);
	phytium_pcie_pmu_v2_reset_event_trig_mode(pcie_pmu);
	phytium_pcie_pmu_v2_reset_delay_threshold(pcie_pmu);
	phytium_pcie_pmu_v2_reset_event_threshold(pcie_pmu);
	phytium_pcie_pmu_v2_reset_event_group_sort_mode(pcie_pmu);
	phytium_pcie_pmu_v2_reset_bdf_id_cfg(pcie_pmu);
	pcie_pmu->event_cfg.by_timer = 0;
	pcie_pmu->event_cfg.by_evthod = 0;
	pcie_pmu->event_cfg.by_trig_mode = 0;
	pcie_pmu->event_cfg.sort_mode = 0;
	pcie_pmu->event_cfg.bdf_id = 0;
	pcie_pmu->event_cfg.byargs0 = 0;
	pcie_pmu->event_cfg.byargs1 = 0;
	pcie_pmu->event_cfg.timer_64 = 0;
	pcie_pmu->event_cfg.chansel = -1;
	pcie_pmu->event_cfg.aw_threshold = 0;
	pcie_pmu->event_cfg.ar_threshold = 0;
	pcie_pmu->event_cfg.delay_thd0_us = PCIE_PMU_DEFAULT_THD0;
	pcie_pmu->event_cfg.delay_thd1_us = PCIE_PMU_DEFAULT_THD1;
	pcie_pmu->event_cfg.delay_thd2_us = PCIE_PMU_DEFAULT_THD2;
}

static u64 phytium_pcie_pmu_v1_read_counter(struct phytium_pcie_pmu *pcie_pmu,
					 struct hw_perf_event *hwc)
{
	u32 idx = GET_PCIE_EVENTID(hwc);
	u32 cycle_l, cycle_h, rdelay_l, rdelay_h, w_data, wdelay_l, wdelay_h,
		pcie_data_width;
	u64 val64 = 0;
	int i;
	u32 counter_offset = pcie_pmu->ops->get_counter_offset(idx);
	u32 rdelay_num = 127;

	if (!EVENT_VALID_V1(idx)) {
		dev_err(pcie_pmu->dev, "Unsupported event index:%d!\n", idx);
		return 0;
	}

	if (pcie_pmu->ver == PCIEA_V1P5 && pcie_pmu->pmu_id == 3)
		rdelay_num = 63;

	switch (idx) {
	case 0:
		cycle_l = readl(pcie_pmu->base + counter_offset);
		cycle_h = readl(pcie_pmu->base + counter_offset + 4);
		val64 = (u64)cycle_h << 32 | (u64)cycle_l;
		break;
	case 15:
		pcie_data_width = readl(pcie_pmu->base + PCIE_V1_DATA_WIDTH);
		for (i = 0; i < (pcie_data_width / 8); i++) {
			w_data = readl(pcie_pmu->base + counter_offset + 4 * i);
			val64 += w_data;
		}
		break;
	case 16:
		for (i = 0; i <= rdelay_num; i = i + 2) {
			rdelay_l =
				readl(pcie_pmu->base + counter_offset + 4 * i);
			rdelay_h = readl(pcie_pmu->base + counter_offset +
					 4 * i + 4);
			val64 += (u64)rdelay_h << 32 | (u64)rdelay_l;
		}
		break;
	case 17:
		for (i = 0; i <= 63; i++) {
			wdelay_l = readl(pcie_pmu->base + counter_offset + 4 * i);
			wdelay_h = readl(pcie_pmu->base + counter_offset + 4 * i + 4);
			val64 += (u64)wdelay_h << 32 | (u64)wdelay_l;
		}
		break;
	default:
		val64 = readl(pcie_pmu->base + counter_offset);
		break;
	}

	return val64;
}

static u64 phytium_pcie_pmu_v2_read_counter(struct phytium_pcie_pmu *pcie_pmu,
					 struct hw_perf_event *hwc)
{
	int i;
	u64 val64;
	u32 w_data, pcie_data_width, val_l, val_h;
	u32 idx = GET_PCIE_EVENTID(hwc);
	u32 counter_offset = pcie_pmu->ops->get_counter_offset(idx);

	if (!EVENT_VALID_V2(idx)) {
		dev_err(pcie_pmu->dev, "Unsupported event index:%d!\n", idx);
		return 0;
	}

	switch (idx) {
	case 0x0:
	case 0xb:
	case 0x29:
	case 0x44:
	case 0x5f:
		// cycles、w_data_aw_g*
		val_l = readl(pcie_pmu->base + counter_offset);
		val_h = readl(pcie_pmu->base + counter_offset + 4);
		val64 = (u64)val_h << 32 | (u64)val_l;
		break;
	case 0x1C:
		// w_data_g0
		pcie_data_width = readl(pcie_pmu->base + PCIE_V2_DATA_WIDTH);
		val64 = 0;
		for (i = 0; i < (pcie_data_width / 8); i++) {
			w_data = readl(pcie_pmu->base + counter_offset + 4 * i);
			val64 += w_data;
		}
		break;
	case 0x12:
	case 0x13:
	case 0x30:
	case 0x31:
	case 0x4b:
	case 0x4c:
	case 0x66:
	case 0x67:
		// rd_total_g*、wr_total_g*
		val_l = readl(pcie_pmu->base + counter_offset + 4);
		val_h = readl(pcie_pmu->base + counter_offset);
		val64 = (u64)val_h << 32 | (u64)val_l;
		break;
	default:
		val64 = readl(pcie_pmu->base + counter_offset);
		break;
	}
	return val64;
}

static void phytium_pcie_pmu_v1_enable_clk(struct phytium_pcie_pmu *pcie_pmu)
{
	u32 val;

	val = readl(pcie_pmu->cfg_base);
	val |= (pcie_pmu->clk_bits);
	writel(val, pcie_pmu->cfg_base);
}

static void phytium_pcie_pmu_v1_disable_clk(struct phytium_pcie_pmu *pcie_pmu)
{
	u32 val;

	writel(0x1, pcie_pmu->base + PCIE_CLEAR_EVENT);

	val = readl(pcie_pmu->cfg_base);
	val &= ~(pcie_pmu->clk_bits);

	writel(val, pcie_pmu->cfg_base);
}

static void phytium_pcie_pmu_v2_enable_clk(struct phytium_pcie_pmu *pcie_pmu)
{
	u32 val;

	val = readl(pcie_pmu->cfg_base + PCIE_V2_PMU_CLK_EN);
	val |= (pcie_pmu->clk_bits);
	writel(val, pcie_pmu->cfg_base + PCIE_V2_PMU_CLK_EN);
}

static void phytium_pcie_pmu_v2_disable_clk(struct phytium_pcie_pmu *pcie_pmu)
{
	u32 val;

	val = readl(pcie_pmu->cfg_base + PCIE_V2_PMU_CLK_EN);
	val &= ~(pcie_pmu->clk_bits);
	writel(val, pcie_pmu->cfg_base + PCIE_V2_PMU_CLK_EN);
}

static void
phytium_pcie_pmu_clear_all_counters(struct phytium_pcie_pmu *pcie_pmu)
{
	writel(0x1, pcie_pmu->base + PCIE_CLEAR_EVENT);
}

static void
phytium_pcie_pmu_start_all_counters(struct phytium_pcie_pmu *pcie_pmu)
{
	writel(0x1, pcie_pmu->base + PCIE_START_TIMER);
}

static void
phytium_pcie_pmu_stop_all_counters(struct phytium_pcie_pmu *pcie_pmu)
{
	writel(0x1, pcie_pmu->base + PCIE_STOP_TIMER);
}

static unsigned long
phytium_pcie_pmu_get_stop_state(struct phytium_pcie_pmu *pcie_pmu)
{
	unsigned long val;

	val = (unsigned long)readl(pcie_pmu->base + PCIE_STATE_STOP);
	return val;
}

static unsigned long
phytium_pcie_pmu_get_now_state(struct phytium_pcie_pmu *pcie_pmu)
{
	unsigned long val;

	val = (unsigned long)readl(pcie_pmu->base + PCIE_NOW_STATE);
	return val;
}

static u32 phytium_pcie_pmu_v1_get_counter_offset(u32 idx)
{
	return pcie_v1_counter_reg_offset[idx];
}

static u32 phytium_pcie_pmu_v2_get_counter_offset(u32 idx)
{
	return pcie_v2_counter_reg_offset[idx];
}

void phytium_pcie_pmu_v1_reset(struct phytium_pcie_pmu *pcie_pmu)
{
	phytium_pcie_pmu_clear_all_counters(pcie_pmu);
}

void phytium_pcie_pmu_v2_reset(struct phytium_pcie_pmu *pcie_pmu)
{
	phytium_pcie_pmu_clear_all_counters(pcie_pmu);
}

static const struct phytium_pcie_pmu_ops phytium_pcie_pmu_v1_ops = {
	.get_event_config = phytium_pcie_pmu_v1_get_event_config,
	.set_event_config = phytium_pcie_pmu_v1_set_event_config,
	.reset_event_config = phytium_pcie_pmu_v1_reset_event_config,
	.read_counter = phytium_pcie_pmu_v1_read_counter,
	.enable_clk = phytium_pcie_pmu_v1_enable_clk,
	.disable_clk = phytium_pcie_pmu_v1_disable_clk,
	.clear_all_counters = phytium_pcie_pmu_clear_all_counters,
	.start_all_counters = phytium_pcie_pmu_start_all_counters,
	.stop_all_counters = phytium_pcie_pmu_stop_all_counters,
	.get_stop_state = phytium_pcie_pmu_get_stop_state,
	.get_counter_offset = phytium_pcie_pmu_v1_get_counter_offset,
	.reset_pmu = phytium_pcie_pmu_v1_reset,
};

static const struct phytium_pcie_pmu_ops phytium_pcie_pmu_v2_ops = {
	.get_event_config = phytium_pcie_pmu_v2_get_event_config,
	.set_event_config = phytium_pcie_pmu_v2_set_event_config,
	.reset_event_config = phytium_pcie_pmu_v2_reset_event_config,
	.read_counter = phytium_pcie_pmu_v2_read_counter,
	.enable_clk = phytium_pcie_pmu_v2_enable_clk,
	.disable_clk = phytium_pcie_pmu_v2_disable_clk,
	.clear_all_counters = phytium_pcie_pmu_clear_all_counters,
	.start_all_counters = phytium_pcie_pmu_start_all_counters,
	.stop_all_counters = phytium_pcie_pmu_stop_all_counters,
	.get_stop_state = phytium_pcie_pmu_get_stop_state,
	.get_counter_offset = phytium_pcie_pmu_v2_get_counter_offset,
	.reset_pmu = phytium_pcie_pmu_v2_reset,
};

static unsigned long
phytium_pcie_pmu_get_irq_flag(struct phytium_pcie_pmu *pcie_pmu)
{
	unsigned long val;

	val = (unsigned long)readl(pcie_pmu->irq_reg);
	return val;
}

static int phytium_pcie_pmu_mark_event(struct perf_event *event)
{
	struct phytium_pcie_pmu *pcie_pmu = to_phytium_pcie_pmu(event->pmu);
	unsigned long *used_mask = pcie_pmu->pmu_events.used_mask;
	struct hw_perf_event *hwc = &event->hw;

	int idx = GET_PCIE_EVENTID(hwc);

	if (pcie_pmu->ver < PCIEA_V2P0) {
		if (!EVENT_VALID_V1(idx)) {
			dev_err(pcie_pmu->dev, "Unsupported event index:%d!\n", idx);
			return -ENODEV;
		}
	} else {
		if (!EVENT_VALID_V2(idx)) {
			dev_err(pcie_pmu->dev, "Unsupported event index:%d!\n", idx);
			return -ENODEV;
		}
	}

	if (test_bit(idx, used_mask))
		return -EAGAIN;

	set_bit(idx, used_mask);

	return idx;
}

static void phytium_pcie_pmu_unmark_event(struct phytium_pcie_pmu *pcie_pmu,
					  int idx)
{
	if (pcie_pmu->ver < PCIEA_V2P0) {
		if (!EVENT_VALID_V1(idx)) {
			dev_err(pcie_pmu->dev, "Unsupported event index:%d!\n", idx);
			return;
		}
	} else {
		if (!EVENT_VALID_V2(idx)) {
			dev_err(pcie_pmu->dev, "Unsupported event index:%d!\n", idx);
			return;
		}
	}

	clear_bit(idx, pcie_pmu->pmu_events.used_mask);
}

int phytium_pcie_pmu_event_init(struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;
	struct phytium_pcie_pmu *pcie_pmu;

	if (event->attr.type != event->pmu->type)
		return -ENOENT;

	if (is_sampling_event(event) || event->attach_state & PERF_ATTACH_TASK)
		return -EOPNOTSUPP;

	pcie_pmu = to_phytium_pcie_pmu(event->pmu);

	if (event->cpu < 0) {
		dev_warn(pcie_pmu->dev, "Can't provide per-task data!\n");
		return -EINVAL;
	}

	if ((event->attr.config & PHYTIUM_PCIE_EVENTS_MAX_MASK) > pcie_pmu->cnts_num)
		return -EINVAL;

	if (pcie_pmu->on_cpu == -1)
		return -EINVAL;

	hwc->idx = -1;
	hwc->config_base = event->attr.config;

	event->cpu = pcie_pmu->on_cpu;
	return 0;
}

void phytium_pcie_pmu_event_update(struct perf_event *event)
{
	struct phytium_pcie_pmu *pcie_pmu = to_phytium_pcie_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	u64 delta;

	delta = pcie_pmu->ops->read_counter(pcie_pmu, hwc);
	local64_add(delta, &event->count);
}

void phytium_pcie_pmu_event_start(struct perf_event *event, int flags)
{
	struct hw_perf_event *hwc = &event->hw;

	hwc->state = 0;
	perf_event_update_userpage(event);
}

void phytium_pcie_pmu_event_stop(struct perf_event *event, int flags)
{
	struct hw_perf_event *hwc = &event->hw;

	hwc->state |= PERF_HES_STOPPED;

	if (flags & PERF_EF_UPDATE)
		phytium_pcie_pmu_event_update(event);
}

int phytium_pcie_pmu_event_add(struct perf_event *event, int flags)
{
	struct phytium_pcie_pmu *pcie_pmu = to_phytium_pcie_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	int idx, ret;

	hwc->state |= PERF_HES_STOPPED;
	pcie_pmu->ops->get_event_config(event, pcie_pmu);
	ret = pcie_pmu->ops->set_event_config(event, pcie_pmu);
	if (ret < 0)
		return ret;

	idx = phytium_pcie_pmu_mark_event(event);
	if (idx < 0)
		return idx;

	event->hw.idx = idx;
	pcie_pmu->pmu_events.hw_events[idx] = event;

	return 0;
}

void phytium_pcie_pmu_event_del(struct perf_event *event, int flags)
{
	struct phytium_pcie_pmu *pcie_pmu = to_phytium_pcie_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;

	phytium_pcie_pmu_event_stop(event, PERF_EF_UPDATE);
	phytium_pcie_pmu_unmark_event(pcie_pmu, hwc->idx);

	pcie_pmu->ops->reset_event_config(pcie_pmu);

	perf_event_update_userpage(event);
	pcie_pmu->pmu_events.hw_events[hwc->idx] = NULL;
}

void phytium_pcie_pmu_enable(struct pmu *pmu)
{
	struct phytium_pcie_pmu *pcie_pmu = to_phytium_pcie_pmu(pmu);
	int event_added = bitmap_weight(pcie_pmu->pmu_events.used_mask,
					pcie_pmu->cnts_num);

	if (event_added) {
		pcie_pmu->ops->clear_all_counters(pcie_pmu);
		pcie_pmu->ops->enable_clk(pcie_pmu);
		pcie_pmu->ops->start_all_counters(pcie_pmu);
	}
}

void phytium_pcie_pmu_disable(struct pmu *pmu)
{
	struct phytium_pcie_pmu *pcie_pmu = to_phytium_pcie_pmu(pmu);
	int event_added;

	event_added = bitmap_weight(pcie_pmu->pmu_events.used_mask,
					pcie_pmu->cnts_num);
	if (event_added)
		pcie_pmu->ops->stop_all_counters(pcie_pmu);
}

static const struct acpi_device_id phytium_pcie_pmu_acpi_match[] = {
	{ "PHYT0044", },
	{ "PHYT0068", },
	{ "PHYT3009", },
	{ "PHYT300B", },
	{ "PHYT300A", },
	{},
};
MODULE_DEVICE_TABLE(acpi, phytium_pcie_pmu_acpi_match);

static irqreturn_t phytium_pcie_pmu_overflow_handler(int irq, void *dev_id)
{
	struct phytium_pcie_pmu *pcie_pmu = dev_id;
	struct perf_event *event;
	unsigned long overflown, stop_state, now_state;
	int idx;

	unsigned long *used_mask = pcie_pmu->pmu_events.used_mask;
	int event_added = bitmap_weight(used_mask, pcie_pmu->cnts_num);

	now_state = phytium_pcie_pmu_get_now_state(pcie_pmu);
	if (now_state != PCIE_PMU_OVER_STATE)
		return IRQ_NONE;

	if (!event_added) {
		pcie_pmu->ops->clear_all_counters(pcie_pmu);
		return IRQ_HANDLED;
	}
	overflown = phytium_pcie_pmu_get_irq_flag(pcie_pmu);
	stop_state = phytium_pcie_pmu_get_stop_state(pcie_pmu);

	dev_dbg(pcie_pmu->dev, "%s, pcie_pmu->irq_bit=%d,overflown=%lu, stop_state=%lu, now_state=%lu, event_added=%u.\n",
		__func__, pcie_pmu->irq_bit, overflown, stop_state, now_state, event_added);

	if (bitmap_weight(&stop_state, 6)) {
		for_each_set_bit(idx, used_mask, pcie_pmu->cnts_num) {
			event = pcie_pmu->pmu_events.hw_events[idx];
			if (!event)
				continue;
			phytium_pcie_pmu_event_update(event);
		}
		pcie_pmu->ops->clear_all_counters(pcie_pmu);

		if (stop_state & PCIE_TIME_STOP_TYPE)
			dev_info(pcie_pmu->dev, "Setting time has been reached, pmu stopped!");
		else if (stop_state & PCIE_EVENT_THD0_STOP_TYPE)
			dev_info(pcie_pmu->dev, "Setting event(aw) threshold has been reached, pmu stopped!");
		else if (stop_state & PCIE_EVENT_THD1_STOP_TYPE)
			dev_info(pcie_pmu->dev, "Setting event(ar) threshold has been reached, pmu stopped!");
		else
			pcie_pmu->ops->start_all_counters(pcie_pmu);

		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static int phytium_pcie_pmu_version(struct platform_device *pdev,
	struct phytium_pcie_pmu *pcie_pmu)
{
	struct acpi_device *acpi_dev;
	const char *hid;
	u32 pidr, clkfre, data_width;

	acpi_dev = ACPI_COMPANION(&pdev->dev);
	hid = acpi_device_hid(acpi_dev);
	if (!strcmp(acpi_device_hid(acpi_dev), "PHYT0044")) {
		pcie_pmu->ver = PCIEA_V1P0;
		pcie_pmu->dev_id = 0;
	} else if (!strcmp(acpi_device_hid(acpi_dev), "PHYT0068")) {
		pcie_pmu->ver = PCIEA_V1P5;
		pcie_pmu->dev_id = 0;
	} else if (!strcmp(acpi_device_hid(acpi_dev), "PHYT3009")) {
		pidr = readl(pcie_pmu->base + PCIE_PMU_PIDR);
		clkfre = readl(pcie_pmu->base + PCIE_V2_AXI_CLK_FRE);
		data_width = readl(pcie_pmu->base + PCIE_V2_DATA_WIDTH);
		dev_dbg(pcie_pmu->dev, "PCIE0 PIDR=%#x,VER=%#lx,clkfre=%u, data_width=%u.\n",
			pidr, (pidr & PCIE_PMU_VER_BIT), clkfre, data_width);
		pidr &= PCIE_PMU_VER_BIT;
		if (pidr == 0x1) {
			pcie_pmu->ver = PCIEA_V2P0;
			pcie_pmu->dev_id = 0;
			pcie_pmu->channel_num = PCIEA_V2P0_CHANNEL_NUM;
		} else {
			dev_err(&pdev->dev, "The current driver does not support this device.\n");
			return -ENODEV;
		}
	} else if (!strcmp(acpi_device_hid(acpi_dev), "PHYT300B")) {
		pidr = readl(pcie_pmu->base + PCIE_PMU_PIDR);
		clkfre = readl(pcie_pmu->base + PCIE_V2_AXI_CLK_FRE);
		data_width = readl(pcie_pmu->base + PCIE_V2_DATA_WIDTH);
		dev_dbg(pcie_pmu->dev, "PCIE1 PIDR=%#x,VER=%#lx,clkfre=%u, data_width=%u.\n",
			pidr, (pidr & PCIE_PMU_VER_BIT), clkfre, data_width);
		pidr &= PCIE_PMU_VER_BIT;
		if (pidr == 0x1) {
			pcie_pmu->ver = PCIEB_V2P0;
			pcie_pmu->channel_num = PCIEB_V2P0_CHANNEL_NUM;
			pcie_pmu->dev_id = 1;
		} else {
			dev_err(&pdev->dev, "The current driver does not support this device.\n");
			return -ENODEV;
		}
	} else if (!strcmp(acpi_device_hid(acpi_dev), "PHYT300A")) {
		pidr = readl(pcie_pmu->base + PCIE_PMU_PIDR);
		clkfre = readl(pcie_pmu->base + PCIE_V2_AXI_CLK_FRE);
		data_width = readl(pcie_pmu->base + PCIE_V2_DATA_WIDTH);
		dev_dbg(pcie_pmu->dev, "PCIE1 PIDR=%#x,VER=%#lx,clkfre=%u, data_width=%u.\n",
			pidr, (pidr & PCIE_PMU_VER_BIT), clkfre, data_width);
		pidr &= PCIE_PMU_VER_BIT;
		if (pidr == 0x1) {
			pcie_pmu->ver = PCIEC_V2P0;
			pcie_pmu->channel_num = PCIEC_V2P0_CHANNEL_NUM;
			pcie_pmu->dev_id = 2;
		} else {
			dev_err(&pdev->dev, "The current driver does not support this device.\n");
			return -ENODEV;
		}
	} else {
		dev_err(&pdev->dev, "The current driver does not support this device.\n");
		return -ENODEV;
	}
	return 0;
}

static int phytium_pcie_pmu_init_irq(struct phytium_pcie_pmu *pcie_pmu,
				     struct platform_device *pdev)
{
	int irq, ret;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	phytium_pcie_pmu_clear_all_counters(pcie_pmu);

	ret = devm_request_irq(&pdev->dev, irq,
				phytium_pcie_pmu_overflow_handler,
				IRQF_NOBALANCING | IRQF_NO_THREAD | IRQF_SHARED,
				dev_name(&pdev->dev), pcie_pmu);
	if (ret < 0) {
		dev_err(&pdev->dev, "Fail to request IRQ:%d ret:%d\n", irq,
			ret);
		return ret;
	}

	pcie_pmu->irq = irq;

	return 0;
}

static int phytium_pcie_pmu_init_data(struct platform_device *pdev,
		struct phytium_pcie_pmu *pcie_pmu)
{
	struct resource *res, *csr, *irqres;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pcie_pmu->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pcie_pmu->base)) {
		dev_err(&pdev->dev, "ioremap failed for pcie_pmu resource\n");
		return PTR_ERR(pcie_pmu->base);
	}

	csr = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!csr) {
		dev_err(&pdev->dev, "failed for get pcie_pmu csr resource.\n");
		return -EINVAL;
	}
	pcie_pmu->cfg_base =
		devm_ioremap(&pdev->dev, csr->start, resource_size(csr));
	if (IS_ERR(pcie_pmu->cfg_base)) {
		dev_err(&pdev->dev,
			"ioremap failed for pcie_pmu csr resource\n");
		return PTR_ERR(pcie_pmu->cfg_base);
	}

	irqres = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (!irqres) {
		dev_err(&pdev->dev,
			"failed for get pcie_pmu irq reg resource.\n");
		return -EINVAL;
	}
	pcie_pmu->irq_reg =
		devm_ioremap(&pdev->dev, irqres->start, resource_size(irqres));
	if (IS_ERR(pcie_pmu->irq_reg)) {
		dev_err(&pdev->dev,
			"ioremap failed for pcie_pmu irq resource\n");
		return PTR_ERR(pcie_pmu->irq_reg);
	}

	ret = phytium_pcie_pmu_version(pdev, pcie_pmu);
	if (ret)
		return ret;

	if (device_property_read_u32(&pdev->dev, "phytium,die-id",
		&pcie_pmu->die_id)) {
		dev_err(&pdev->dev, "Can not read phytium,die-id!\n");
		return -EINVAL;
	}

	if (device_property_read_u32(&pdev->dev, "phytium,pmu-id",
			&pcie_pmu->pmu_id)) {
		dev_err(&pdev->dev, "Can not read phytium,pmu-id!\n");
		return -EINVAL;
	}

	if (pcie_pmu->ver == PCIEA_V1P0) {
		pcie_pmu->cnts_num = PHYTIUM_PCIE_V1_COUNTERS_NUM;
		pcie_pmu->ops = &phytium_pcie_pmu_v1_ops;

		switch (pcie_pmu->pmu_id) {
		case 0:
			pcie_pmu->clk_bits = 0x1;
			break;
		case 1:
			pcie_pmu->clk_bits = 0xe;
			break;
		case 2:
			pcie_pmu->clk_bits = 0xf;
			break;
		default:
			dev_err(&pdev->dev, "Unsupported pmu id:%d!\n", pcie_pmu->pmu_id);
			break;
		}

		pcie_pmu->irq_bit = pcie_pmu->pmu_id + 4;
	} else if (pcie_pmu->ver == PCIEA_V1P5) {
		pcie_pmu->cnts_num = PHYTIUM_PCIE_V1_COUNTERS_NUM;
		pcie_pmu->ops = &phytium_pcie_pmu_v1_ops;

		if (device_property_read_u32(&pdev->dev, "phytium,pcie-id", &pcie_pmu->pcie_id)) {
			dev_err(&pdev->dev, "Can not read phytium,pcie-id!\n");
			return -EINVAL;
		}

		switch (pcie_pmu->pmu_id) {
		case 0:
		case 3:
			pcie_pmu->clk_bits = 0x1;
			break;
		case 1:
			pcie_pmu->clk_bits = 0x2;
			break;
		case 2:
			pcie_pmu->clk_bits = 0xc;
			break;
		default:
			dev_err(&pdev->dev, "Unsupported pmu id:%d!\n", pcie_pmu->pmu_id);
			break;
		}
		pcie_pmu->irq_bit = pcie_pmu->pcie_id * 4 + pcie_pmu->pmu_id + 16;
	} else {
		pcie_pmu->cnts_num = PHYTIUM_PCIE_V2_COUNTERS_NUM;
		pcie_pmu->ops = &phytium_pcie_pmu_v2_ops;

		if (device_property_read_u32(&pdev->dev, "phytium,pcie-id", &pcie_pmu->pcie_id)) {
			dev_err(&pdev->dev, "Can not read phytium,pcie-id!\n");
			return -EINVAL;
		}

		pcie_pmu->clk_bits = PCIEV2_PMU_CLK_EN_BIT;
		pcie_pmu->irq_bit = 2 - pcie_pmu->dev_id;
	}

	pcie_pmu->ops->reset_pmu(pcie_pmu);

	return 0;
}

static int phytium_pcie_pmu_dev_probe(struct platform_device *pdev,
		struct phytium_pcie_pmu *pcie_pmu)
{
	int ret;

	ret = phytium_pcie_pmu_init_data(pdev, pcie_pmu);
	if (ret)
		return ret;

	ret = phytium_pcie_pmu_init_irq(pcie_pmu, pdev);
	if (ret)
		return ret;
	pcie_pmu->dev = &pdev->dev;
	pcie_pmu->on_cpu = -1;
	pcie_pmu->ctrler_id = -1;

	return 0;
}

static int phytium_pcie_pmu_probe(struct platform_device *pdev)
{
	struct phytium_pcie_pmu *pcie_pmu;

	char *name;
	int ret;

	pcie_pmu = devm_kzalloc(&pdev->dev, sizeof(*pcie_pmu), GFP_KERNEL);
	if (!pcie_pmu)
		return -ENOMEM;

	platform_set_drvdata(pdev, pcie_pmu);

	ret = phytium_pcie_pmu_dev_probe(pdev, pcie_pmu);
	if (ret)
		return ret;

	ret = cpuhp_state_add_instance(
		phytium_pcie_pmu_hp_state, &pcie_pmu->node);
	if (ret) {
		dev_err(&pdev->dev, "Error %d registering hotplug;\n", ret);
		return ret;
	}

	if (pcie_pmu->ver == PCIEA_V1P0)
		name = devm_kasprintf(&pdev->dev, GFP_KERNEL, "phyt%u_pcie_pmu%u",
					pcie_pmu->die_id, pcie_pmu->pmu_id);
	else
		name = devm_kasprintf(&pdev->dev, GFP_KERNEL, "phyt%u_pcie%u_pmu%u",
					pcie_pmu->die_id, pcie_pmu->pcie_id, pcie_pmu->pmu_id);

	pcie_pmu->pmu = (struct pmu) {
		.name = name,
		.module = THIS_MODULE,
		.task_ctx_nr = perf_invalid_context,
		.event_init = phytium_pcie_pmu_event_init,
		.pmu_enable = phytium_pcie_pmu_enable,
		.pmu_disable = phytium_pcie_pmu_disable,
		.add = phytium_pcie_pmu_event_add,
		.del = phytium_pcie_pmu_event_del,
		.start = phytium_pcie_pmu_event_start,
		.stop = phytium_pcie_pmu_event_stop,
		.read = phytium_pcie_pmu_event_update,
	};
	if (pcie_pmu->ver < PCIEA_V2P0)
		pcie_pmu->pmu.attr_groups = phytium_pcie_pmu_v1_attr_groups;
	else
		pcie_pmu->pmu.attr_groups = phytium_pcie_pmu_v2_attr_groups;

	ret = perf_pmu_register(&pcie_pmu->pmu, name, -1);
	if (ret) {
		dev_err(pcie_pmu->dev, "%s register failed!\n", name);
		cpuhp_state_remove_instance_nocalls(
			phytium_pcie_pmu_hp_state,
			&pcie_pmu->node);
	}

	pcie_pmu->ops->enable_clk(pcie_pmu);

	dev_info(pcie_pmu->dev, "%s on cpu%d.\n", name, pcie_pmu->on_cpu);

	return ret;
}

static int phytium_pcie_pmu_remove(struct platform_device *pdev)
{
	struct phytium_pcie_pmu *pcie_pmu = platform_get_drvdata(pdev);

	pcie_pmu->ops->disable_clk(pcie_pmu);

	perf_pmu_unregister(&pcie_pmu->pmu);
	cpuhp_state_remove_instance_nocalls(
		phytium_pcie_pmu_hp_state, &pcie_pmu->node);

	return 0;
}

static struct platform_driver phytium_pcie_pmu_driver = {
	.driver = {
			.name = "phytium_pcie_pmu",
			.acpi_match_table = ACPI_PTR(phytium_pcie_pmu_acpi_match),
			.suppress_bind_attrs = true,
	},
	.probe = phytium_pcie_pmu_probe,
	.remove = phytium_pcie_pmu_remove,
};

int phytium_pcie_pmu_online_cpu(unsigned int cpu, struct hlist_node *node)
{
	struct phytium_pcie_pmu *pcie_pmu =
		hlist_entry_safe(node, struct phytium_pcie_pmu, node);

	if (!cpumask_test_cpu(cpu, cpumask_of_node(pcie_pmu->die_id)))
		return 0;

	if (pcie_pmu->on_cpu != -1) {
		if (!cpumask_test_cpu(pcie_pmu->on_cpu, cpumask_of_node(pcie_pmu->die_id))) {
			perf_pmu_migrate_context(&pcie_pmu->pmu, pcie_pmu->on_cpu, cpu);
			pcie_pmu->on_cpu = cpu;
			WARN_ON(irq_set_affinity_hint(pcie_pmu->irq, cpumask_of(cpu)));
		}
		return 0;
	}

	pcie_pmu->on_cpu = cpu;
	WARN_ON(irq_set_affinity_hint(pcie_pmu->irq, cpumask_of(cpu)));

	return 0;
}

int phytium_pcie_pmu_offline_cpu(unsigned int cpu, struct hlist_node *node)
{
	struct phytium_pcie_pmu *pcie_pmu =
		hlist_entry_safe(node, struct phytium_pcie_pmu, node);
	unsigned int target;
	cpumask_t available_cpus;

	if (pcie_pmu->on_cpu != cpu)
		return 0;

	if (cpumask_and(&available_cpus, cpumask_of_node(pcie_pmu->die_id), cpu_online_mask) &&
		cpumask_andnot(&available_cpus, &available_cpus, cpumask_of(cpu)))
		target = cpumask_last(&available_cpus);
	else {
		cpumask_andnot(&available_cpus, cpu_online_mask, cpumask_of(cpu));
		target = cpumask_last(&available_cpus);
	}

	if (target >= nr_cpu_ids) {
		dev_err(pcie_pmu->dev, "offline cpu%d with no target to migrate.\n",
			cpu);
		return 0;
	}

	perf_pmu_migrate_context(&pcie_pmu->pmu, cpu, target);
	WARN_ON(irq_set_affinity_hint(pcie_pmu->irq, cpumask_of(target)));
	pcie_pmu->on_cpu = target;

	return 0;
}

static int __init phytium_pcie_pmu_module_init(void)
{
	int ret;

	phytium_pcie_pmu_hp_state =
		cpuhp_setup_state_multi(CPUHP_AP_ONLINE_DYN,
					"perf/phytium/pciepmu:online", phytium_pcie_pmu_online_cpu,
					phytium_pcie_pmu_offline_cpu);
	if (phytium_pcie_pmu_hp_state < 0) {
		pr_err("PCIE PMU: setup hotplug, ret = %d\n",
			phytium_pcie_pmu_hp_state);
		return phytium_pcie_pmu_hp_state;
	}

	ret = platform_driver_register(&phytium_pcie_pmu_driver);
	if (ret)
		cpuhp_remove_multi_state(
			phytium_pcie_pmu_hp_state);

	return ret;
}
module_init(phytium_pcie_pmu_module_init);

static void __exit phytium_pcie_pmu_module_exit(void)
{
	platform_driver_unregister(&phytium_pcie_pmu_driver);
	cpuhp_remove_multi_state(phytium_pcie_pmu_hp_state);
}
module_exit(phytium_pcie_pmu_module_exit);

MODULE_DESCRIPTION("Phytium PCIe PMU driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(PCIE_PERF_DRIVER_VERSION);
MODULE_AUTHOR("Hu Xianghua <huxianghua@phytium.com.cn>");
MODULE_AUTHOR("Tan Rui <tanrui2142@phytium.com.cn>");
