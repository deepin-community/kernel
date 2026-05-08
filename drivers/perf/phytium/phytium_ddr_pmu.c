// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium SoC DDR performance monitoring unit support
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
#include <linux/arm-smccc.h>

#include <asm/cputype.h>
#include <asm/local64.h>

#undef pr_fmt
#define pr_fmt(fmt) "phytium_ddr_pmu: " fmt

#define DDR_PERF_DRIVER_VERSION "1.4.0"

#define DDR_PMUV1_START_TIMER		0x000
#define DDR_PMUV1_STOP_TIMER		0x004
#define DDR_PMUV1_CLEAR_EVENT		0x008
#define DDR_PMUV1_SET_TIMER_L		0x00c
#define DDR_PMUV1_SET_TIMER_H		0x010
#define DDR_PMUV1_TRIG_MODE		0x014
#define DDR_PMUV1_NOW_STATE		0x0e0
#define DDR_PMUV1_EVENT_CYCLES	0x0e4
#define DDR_PMUV1_TPOINT_END_L	0x0e4
#define DDR_PMUV1_TPOINT_END_H	0x0e8
#define DDR_PMUV1_STATE_STOP		0x0ec
#define DDR_PMUV1_EVENT_RXREQ		0x100
#define DDR_PMUV1_EVENT_RXDAT		0x104
#define DDR_PMUV1_EVENT_TXDAT		0x108
#define DDR_PMUV1_EVENT_RXREQ_RNS	0x10c
#define DDR_PMUV1_EVENT_RXREQ_WNSP	0x110
#define DDR_PMUV1_EVENT_RXREQ_WNSF	0x114
#define DDR_PMUV1_EVENT_BANDWIDTH	0x200
#define DDR_PMUV1_W_DATA_BASE		0x200
#define DDR_PMUV1_CLK_FRE		0xe00
#define DDR_PMUV1_DATA_WIDTH		0xe04

#define DDR_PMUV1_OFL_STOP_TYPE_VAL 0x10

#define PBFVER_FUNC_ID         0x82000001
#define DDR_PMUV2_TIMER_START			0x0
#define DDR_PMUV2_TIMER_STOP			0x4
#define DDR_PMUV2_CLEAR_EVENT			0x8
#define DDR_PMUV2_SET_TIMER_L			0xc
#define DDR_PMUV2_SET_TIMER_H			0x10
#define DDR_PMUV2_AXI_MONITOR_EN		0x1c

#define DDR_PMUV2_TIMER_INT_CLEAR			0x2c
#define DDR_PMUV2_AXI_MONITOR_INT_CLEAR	0x30
#define DDR_PMUV2_TIMER_INT_STA			0x40
#define DDR_PMUV2_AXI_MONITOR_INT_STA		0x44
#define DDR_PMUV2_TIMER_INT_MASK			0x54
#define DDR_PMUV2_AXI_MONITOR_INT_MASK		0x58

#define DDR_PMUV2_EVENT_CYCLES 0x208
#define DDR_PMUV2_EVENT_AXI_READ_CMD_CNT              0x074
#define DDR_PMUV2_EVENT_AXI_WRITE_CMD_CNT             0x07c
#define DDR_PMUV2_EVENT_AXI_READ_FLUX_CNT             0x084
#define DDR_PMUV2_EVENT_AXI_WRITE_FLUX_CNT            0x08c

#define DDR_PMUV2_ALL_EVENT_CLEAR_BIT		0x1
#define DDR_PMUV2_TIMER_OPT_BIT		0x1
#define DDR_PMUV2_AXI_MONITOR_OPT_BIT		0x01010101

#define DDR_PMUV2_NOTICE_START  0x0
#define DDR_PMUV2_NOTICE_STOP   0x1

#define DDR_PMUV3_SNAPSHOT_PMU        0x000
#define DDR_PMUV3_START_TIMER         0x008
#define DDR_PMUV3_CLEAR_TIMER         0x00C
#define DDR_PMUV3_CLK_EN              0x288
#define DDR_PMUV3_PIDR0               0xFE0
#define DDR_PMUV3_PMU_VER_BIT         GENMASK(7, 0)
#define DDR_PMUV3_PMU_PART_BIT        GENMASK(11, 8)

#define DDR_PMUV3_HPR_FIFO_THRE       0x010
#define DDR_PMUV3_LPR_FIFO_THRE       0x014
#define DDR_PMUV3_WR_DCQ_THRE         0x018
#define DDR_PMUV3_WDP_BUFFER_THRE     0x01C
#define DDR_PMUV3_RDAT_INFO_THRE      0x020
#define DDR_PMUV3_RDAT_FIFO_THRE      0x024
#define DDR_PMUV3_RSP_COMP_THRE       0x028
#define DDR_PMUV3_RSP_DBID_THRE       0x02C
#define DDR_PMUV3_RSP_REQ_THRE        0x030
#define DDR_PMUV3_RSP_CRQ_THRE        0x034
#define DDR_PMUV3_RSP_RTQ_THRE        0x038

#define DDR_PMUV3_MC_DFI_CMD_CNT_SEL          0x03C
#define DDR_PMUV3_MC_UIF_CNT_SEL              0x040
#define DDR_PMUV3_MC_CAM_OCCU_CNT_SEL         0x044
#define DDR_PMUV3_MC_CMD_SCHEDULING_CNT_SEL   0x048
#define DDR_PMUV3_PORT_CMD_CNT_SEL            0x04C
#define DDR_PMUV3_PORT_CMD_OPCODE_CNT_SEL     0x050
#define DDR_PMUV3_PORT_CMD_RETRY_CNT_SEL      0x054
#define DDR_PMUV3_PORT_PREF_STATUS_CNT_SEL    0x058
#define DDR_PMUV3_PORT_CQ_OCCU_CNT_SEL        0x05C
#define DDR_PMUV3_PORT_DATA_OCCU_CNT_SEL      0x060
#define DDR_PMUV3_PORT_RSP_OCCU_CNT_SEL       0x064
#define DDR_PMUV3_MC_RDWR_SWITCH_CNT_SEL      0x2f0

#define DDR_PMUV3_MC_DFI_CMD_CNT_L        0x070
#define DDR_PMUV3_MC_UIF_CNT_L            0x074
#define DDR_PMUV3_MC_BANK_MAGT_CNT_L      0x078
#define DDR_PMUV3_MC_CAM_OCCU_CNT_L       0x07C
#define DDR_PMUV3_MC_CAM_OCCU_CNT_H       0x080
#define DDR_PMUV3_MC_CMD_SCHE_CNT_L       0x084
#define DDR_PMUV3_MC_CMD_SCHE_CNT_H       0x088
#define DDR_PMUV3_MC_T_CMD_SCHE_CNT_L     0x08C
#define DDR_PMUV3_MC_T_CMD_SCHE_CNT_H     0x090
#define DDR_PMUV3_PORT_CMD_CNT_L          0x0B8
#define DDR_PMUV3_PORT_CMD_OPCODE_CNT_L   0x0BC
#define DDR_PMUV3_PORT_RETRY_CNT_L        0x0C0
#define DDR_PMUV3_PORT_PREF_STATUS_CNT_L  0x0C4
#define DDR_PMUV3_PORT_CQ_OCCU_CNT_L      0x0C8
#define DDR_PMUV3_PORT_CQ_OCCU_CNT_H      0x0CC
#define DDR_PMUV3_PORT_DATA_OCCU_CNT_L    0x0D0
#define DDR_PMUV3_PORT_DATA_OCCU_CNT_H    0x0D4
#define DDR_PMUV3_PORT_RSP_OCCU_CNT_L     0x0D8
#define DDR_PMUV3_PORT_RSP_OCCU_CNT_H     0x0DC
#define DDR_PMUV3_MC_RDWR_SWITCH_CNT_L    0x2f4
#define DDR_PMUV3_MC_RDWR_SWITCH_CNT_H    0x2f8
#define DDR_PMUV3_MC_T_RDWR_SWITCH_CNT_L  0x2fc
#define DDR_PMUV3_MC_T_RDWR_SWITCH_CNT_H  0x300

#define DDR_PMUV3_GLOBAL_CNT_L            0x068
#define DDR_PMUV3_GLOBAL_CNT_H            0x06C
#define DDR_PMUV3_MC_CQ_IDLE_CNT_L        0x094
#define DDR_PMUV3_MC_EXP_GPR_CNT_L        0x098
#define DDR_PMUV3_MC_EXP_GPW_CNT_L        0x09C
#define DDR_PMUV3_MC_ADDR_COLLISION_CNT_L    0x0A0
#define DDR_PMUV3_MC_RDCAM_CRITICAL_CNT_L    0x0A4
#define DDR_PMUV3_MC_RDCAM_CRITICAL_CNT_H    0x0A8
#define DDR_PMUV3_MC_WRCAM_CRITICAL_CNT_L    0x0AC
#define DDR_PMUV3_MC_WECAM_CRITICAL_CNT_H    0x0B0
#define DDR_PMUV3_MC_RETRY_CMD_CNT_L         0x0B4
#define DDR_PMUV3_PORT_WDAT_BE_CNT_L         0x0E0
#define DDR_PMUV3_PORT_WDAT_RM_BUFFER_DEALLOC_CNT_L   0x0E4
#define DDR_PMUV3_PORT_RD_CMD_DELAY1                  0x2BC
#define DDR_PMUV3_PORT_RD_CMD_DELAY2                  0x2C0
#define DDR_PMUV3_PORT_WR_CMD_DELAY1                  0x2C4
#define DDR_PMUV3_PORT_WR_CMD_DELAY2                  0x2C8

#define DDR_PMUV3_CNT_OVERFLOW_FLAG               0x0E8
#define DDR_PMUV3_MC_DFI_CMD_CNT_OFL_BIT          BIT(0)
#define DDR_PMUV3_MC_UFI_CMD_CNT_OFL_BIT          BIT(1)
#define DDR_PMUV3_MC_BANK_MAGT_CNT_OFL_BIT        BIT(2)
#define DDR_PMUV3_MC_CQ_IDLE_CNT_OFL_BIT          BIT(3)
#define DDR_PMUV3_MC_EXP_GPR_CNT_OFL_BIT          BIT(4)
#define DDR_PMUV3_MC_EXP_GPW_CNT_OFL_BIT          BIT(5)
#define DDR_PMUV3_MC_ADDR_COLLISION_CNT_OFL_BIT   BIT(6)

#define DDR_PMUV3_PORT_CMD_CNT_OFL_BIT            BIT(7)
#define DDR_PMUV3_PORT_CMD_OPCODE_CNT_OFL_BIT     BIT(8)
#define DDR_PMUV3_PORT_RETRY_CNT_OFL_BIT          BIT(9)
#define DDR_PMUV3_PORT_PREF_STATUS_CNT_OFL_BIT    BIT(10)
#define DDR_PMUV3_PORT_WDAT_BE_CNT_OFL_BIT        BIT(11)
#define DDR_PMUV3_PORT_WDAT_RM_BUFFER_DEALLOC_CNT_OFL_BIT     BIT(12)

#define DDR_PMUV3_MC_DFI_CMD_SEL_MASK         GENMASK(3, 0)
#define DDR_PMUV3_MC_UIF_CMD_SEL_MASK         GENMASK(2, 0)
#define DDR_PMUV3_MC_BANK_SRC_SEL_MASK        GENMASK(6, 5)
#define DDR_PMUV3_MC_CAM_OCCU_SEL_MASK        GENMASK(4, 0)
#define DDR_PMUV3_MC_T_CMD_SCHE_SEL_MASK      GENMASK(7, 4)
#define DDR_PMUV3_MC_CMD_SCHE_SEL_MASK        GENMASK(3, 0)
#define DDR_PMUV3_PORT_CMD_SEL_MASK           GENMASK(2, 0)
#define DDR_PMUV3_PORT_CMD_OPCODE_SEL_MASK    GENMASK(2, 0)
#define DDR_PMUV3_PORT_CMD_RETRY_SEL_MASK     GENMASK(2, 0)
#define DDR_PMUV3_PORT_PREF_STATUS_SEL_MASK   GENMASK(2, 0)

#define DDR_PMUV3_LPR_FIFO_OCCU_SEL_MASK      GENMASK(3, 0)
#define DDR_PMUV3_HPR_FIFO_OCCU_SEL_MASK      GENMASK(3, 0)
#define DDR_PMUV3_WR_DCQ_OCCU_SEL_MASK        GENMASK(3, 0)

#define DDR_PMUV3_RDAT_INFO_OCCU_SEL_MASK     GENMASK(3, 0)
#define DDR_PMUV3_RDAT_FIFO_OCCU_SEL_MASK     GENMASK(3, 0)
#define DDR_PMUV3_WDP_BUFFER_OCCU_SEL_MASK    GENMASK(3, 0)

#define DDR_PMUV3_RSP_COMP_OCCU_SEL_MASK      GENMASK(3, 0)
#define DDR_PMUV3_REP_DBID_OCCU_SEL_MASK      GENMASK(3, 0)
#define DDR_PMUV3_RSP_CRQ_OCCU_SEL_MASK       GENMASK(3, 0)
#define DDR_PMUV3_RSP_REQ_OCCU_SEL_MASK       GENMASK(3, 0)
#define DDR_PMUV3_RSP_RTQ_OCCU_SEL_MASK       GENMASK(3, 0)

#define DDR_PMUV3_MC_T_RDWR_SWITCH_CNT_SEL_MASK   GENMASK(7, 4)
#define DDR_PMUV3_MC_RDWR_SWITCH_CNT_SEL_MASK GENMASK(3, 0)

#define DDR_PMUV3_HPR_FIFO_THRE_BIT_SIZE      6
#define DDR_PMUV3_LPR_FIFO_THRE_BIT_SIZE      6
#define DDR_PMUV3_WR_DCQ_THRE_BIT_SIZE        6
#define DDR_PMUV3_WDP_BUFFER_THRE_BIT_SIZE    6
#define DDR_PMUV3_RDAT_INFO_THRE_BIT_SIZE     7
#define DDR_PMUV3_RDAT_FIFO_THRE_BIT_SIZE     7
#define DDR_PMUV3_RSP_COMP_THRE_BIT_SIZE      5
#define DDR_PMUV3_RSP_DBID_THRE_BIT_SIZE      5
#define DDR_PMUV3_RSP_REQ_THRE_BIT_SIZE       6
#define DDR_PMUV3_RSP_CRQ_THRE_BIT_SIZE       6
#define DDR_PMUV3_RSP_RTQ_THRE_BIT_SIZE       6

#define DDR_PMUV3_MC_DFI_CMD_EVENT_NUM        15
#define DDR_PMUV3_MC_UIF_CMD_EVENT_NUM        5
#define DDR_PMUV3_MC_BANK_SRC_EVENT_NUM       3
#define DDR_PMUV3_MC_CAM_OCCU_EVENT_NUM       18
#define DDR_PMUV3_MC_T_CMD_SCHE_EVENT_NUM     16
#define DDR_PMUV3_MC_CMD_SCHE_EVENT_NUM       16
#define DDR_PMUV3_PORT_CMD_EVENT_NUM          7
#define DDR_PMUV3_PORT_CMD_OPCODE_EVENT_NUM   6
#define DDR_PMUV3_PORT_CMD_RETRY_EVENT_NUM    5
#define DDR_PMUV3_PORT_PREF_STATUS_EVENT_NUM  5
#define DDR_PMUV3_LPR_FIFO_OCCU_EVENT_NUM     3
#define DDR_PMUV3_HPR_FIFO_OCCU_EVENT_NUM     3
#define DDR_PMUV3_WR_DCQ_OCCU_EVENT_NUM       3
#define DDR_PMUV3_RDAT_INFO_OCCU_EVENT_NUM    3
#define DDR_PMUV3_RDAT_FIFO_OCCU_EVENT_NUM    3
#define DDR_PMUV3_WDP_BUFFER_OCCU_EVENT_NUM   3
#define DDR_PMUV3_RSP_COMP_OCCU_EVENT_NUM     3
#define DDR_PMUV3_RSP_DBID_OCCU_EVENT_NUM     3
#define DDR_PMUV3_RSP_CRQ_OCCU_EVENT_NUM      3
#define DDR_PMUV3_RSP_REQ_OCCU_EVENT_NUM      3
#define DDR_PMUV3_RSP_RTQ_OCCU_EVENT_NUM      3
#define DDR_PMUV3_MC_RDWR_SWTICH_EVENT_NUM    12
#define DDR_PMUV3_MC_T_RDWR_SWTICH_EVENT_NUM  12
#define DDR_PMUV3_SPECIAL_EVENT_NUM           12

#define PHYTIUM_DDR_MAX_COUNTERS		165
#define PHYTIUM_DDR_PMUV1_COUNTERS_NUM		8
#define PHYTIUM_DDR_PMUV2_COUNTERS_NUM		5
#define PHYTIUM_DDR_PMUV3_COUNTERS_NUM		165
#define PHYTIUM_DDR_PMUV1V2_EVENTS_MAX_MASK		0x7
#define PHYTIUM_DDR_PMUV3_EVENTS_MAX_MASK		0xA5

#define GET_DDR_PMUV1V2_EVENTID(hwc) (hwc->config_base & PHYTIUM_DDR_PMUV1V2_EVENTS_MAX_MASK)
#define EVENT_VALID_V1(idx) ((idx >= 0) && (idx < PHYTIUM_DDR_PMUV1_COUNTERS_NUM))
#define EVENT_VALID_V2(idx) ((idx >= 0) && (idx < PHYTIUM_DDR_PMUV2_COUNTERS_NUM))
#define EVENT_VALID_V3(idx) ((idx >= 0) && (idx < PHYTIUM_DDR_PMUV3_COUNTERS_NUM))

#define to_phytium_ddr_pmu(p) (container_of(p, struct phytium_ddr_pmu, pmu))
enum {
	DDR_PMUV1P0 = 0x01,
	DDR_PMUV1P5 = 0x02,
	DDR_PMUV2P0,
	DDR_PMUV3P0,

};

static int phytium_ddr_pmu_hp_state;

struct phytium_ddr_pmu_hwevents {
	struct perf_event *hw_events[PHYTIUM_DDR_MAX_COUNTERS];
	DECLARE_BITMAP(used_mask, PHYTIUM_DDR_MAX_COUNTERS);
};

int used_event_v2;

struct phytium_ddr_pmu_v3_port_occu_thre {
	u32 lpr_fifo_low_thre;
	u32 lpr_fifo_hight_thre;
	u32 hpr_fifo_low_thre;
	u32 hpr_fifo_hight_thre;
	u32 wr_dcq_low_thre;
	u32 wr_dcq_hight_thre;

	u32 rdat_info_low_thre;
	u32 rdat_info_hight_thre;
	u32 rdat_fifo_low_thre;
	u32 rdat_fifo_hight_thre;
	u32 wdp_buffer_low_thre;
	u32 wdp_buffer_hight_thre;

	u32 rsp_comp_low_thre;
	u32 rsp_comp_hight_thre;

	u32 rsp_dbid_low_thre;
	u32 rsp_dbid_hight_thre;
	u32 rsp_crq_low_thre;
	u32 rsp_crq_hight_thre;
	u32 rsp_req_low_thre;
	u32 rsp_req_hight_thre;
	u32 rsp_rtq_low_thre;
	u32 rsp_rtq_hight_thre;
};

enum ddr_pmu_v3_event_type {
	DDR_PMUV3_MC_DFI_CMD_EVTYPE,
	DDR_PMUV3_MC_UIF_CMD_EVTYPE,
	DDR_PMUV3_MC_BANK_SRC_EVTYPE,
	DDR_PMUV3_MC_CAM_OCCU_EVTYPE,
	DDR_PMUV3_MC_T_CMD_SCHE_EVTYPE,
	DDR_PMUV3_MC_CMD_SCHE_EVTYPE,
	DDR_PMUV3_PORT_CMD_EVTYPE,
	DDR_PMUV3_PORT_CMD_OPCODE_EVTYPE,
	DDR_PMUV3_PORT_CMD_RETRY_EVTYPE,
	DDR_PMUV3_PORT_PREF_STATUS_EVTYPE,
	DDR_PMUV3_MC_RDWR_SWTICH_EVTYPE,
	DDR_PMUV3_MC_T_RDWR_SWTICH_EVTYPE,

	DDR_PMUV3_LPR_FIFO_OCCU_EVTYPE,
	DDR_PMUV3_HPR_FIFO_OCCU_EVTYPE,
	DDR_PMUV3_WR_DCQ_OCCU_EVTYPE,

	DDR_PMUV3_RDAT_INFO_OCCU_EVTYPE,
	DDR_PMUV3_RDAT_FIFO_OCCU_EVTYPE,
	DDR_PMUV3_WDP_BUFFER_OCCU_EVTYPE,

	DDR_PMUV3_RSP_COMP_OCCU_EVTYPE,
	DDR_PMUV3_RSP_DBID_OCCU_EVTYPE,
	DDR_PMUV3_RSP_CRQ_OCCU_EVTYPE,
	DDR_PMUV3_RSP_REQ_OCCU_EVTYPE,
	DDR_PMUV3_RSP_RTQ_OCCU_EVTYPE,

	DDR_PMUV3_SPECIAL_EVTYPE
};

struct phytium_ddr_pmu_v3_event_attr {
	struct device_attribute attr;
	enum ddr_pmu_v3_event_type type;
	u8 eventid;
	u8 occupid;
	u8 cntsize;
	u8 occuflag;
};

struct phytium_ddr_pmu {
	struct device *dev;
	void __iomem *base;
	void __iomem *cfg_base;
	void __iomem *irq_reg;
	struct pmu pmu;
	const struct phytium_ddr_pmu_ops *ops;
	struct phytium_ddr_pmu_hwevents pmu_events;
	struct phytium_ddr_pmu_v3_port_occu_thre port_occu_thre;
	u32 die_id;
	u32 ddr_id;
	u32 pmu_id;
	int irq_bit;
	int on_cpu;
	int irq;
	int ver;
	int cnts_num;
	bool used_flag;
	struct hlist_node node;
};

struct phytium_ddr_pmu_ops {
	irqreturn_t (*overflow_handler)(int irq, void *dev_id);
	u64 (*read_counter)(struct phytium_ddr_pmu *pmu, struct perf_event *event);
	void (*clear_all_counters)(struct phytium_ddr_pmu *pmu);
	void (*start_all_counters)(struct phytium_ddr_pmu *pmu);
	void (*stop_all_counters)(struct phytium_ddr_pmu *pmu);
	void (*enable_counters)(struct phytium_ddr_pmu *pmu);
	void (*disable_counters)(struct phytium_ddr_pmu *pmu);
	void (*enable_clk)(struct phytium_ddr_pmu *pmu);
	void (*disable_clk)(struct phytium_ddr_pmu *pmu);
};

ssize_t phytium_ddr_pmu_format_sysfs_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct dev_ext_attribute *eattr;

	eattr = container_of(attr, struct dev_ext_attribute, attr);

	return sprintf(buf, "%s\n", (char *)eattr->var);
}

ssize_t phytium_ddr_pmu_event_sysfs_show(struct device *dev,
				 struct device_attribute *attr,
				 char *page)
{
	struct dev_ext_attribute *eattr;

	eattr = container_of(attr, struct dev_ext_attribute, attr);

	return sprintf(page, "config=0x%lx\n", (unsigned long)eattr->var);
}

static bool phytium_ddr_pmu_v3_is_port_occup_event(enum ddr_pmu_v3_event_type type,
						unsigned int occuflag)
{
	bool ret = (type != DDR_PMUV3_MC_CAM_OCCU_EVTYPE) && (occuflag == 1);

	return ret;
}

static ssize_t phytium_ddr_pmu_v3_event_show(struct device *dev, struct device_attribute *attr,
					char *buf)
{
	struct phytium_ddr_pmu_v3_event_attr *eattr;

	eattr = container_of(attr, typeof(*eattr), attr);

	if (phytium_ddr_pmu_v3_is_port_occup_event(eattr->type, eattr->occuflag))
		return snprintf(buf, PAGE_SIZE, "type=0x%x,eventid=0x%x,low_thre=?,hight_thre=?\n",
				eattr->type, eattr->eventid);

	return snprintf(buf, PAGE_SIZE, "type=0x%x,eventid=0x%x\n", eattr->type, eattr->eventid);
}

static ssize_t cpumask_show(struct device *dev, struct device_attribute *attr,
		     char *buf)
{
	struct phytium_ddr_pmu *ddr_pmu =
		to_phytium_ddr_pmu(dev_get_drvdata(dev));

	return cpumap_print_to_pagebuf(true, buf, cpumask_of(ddr_pmu->on_cpu));
}

#define PHYTIUM_PMU_ATTR(_name, _func, _config)                             \
		(&((struct dev_ext_attribute[]){                                    \
		{ __ATTR(_name, 0444, _func, NULL), (void *)_config } })[0] \
		  .attr.attr)

#define PHYTIUM_DDR_PMU_FORMAT_ATTR(_name, _config)                \
		PHYTIUM_PMU_ATTR(_name, phytium_ddr_pmu_format_sysfs_show, \
			 (void *)_config)

static const u32 ddr_pmu_v1_counter_reg_offset[] = {
	DDR_PMUV1_EVENT_CYCLES,     DDR_PMUV1_EVENT_RXREQ,	   DDR_PMUV1_EVENT_RXDAT,
	DDR_PMUV1_EVENT_TXDAT,      DDR_PMUV1_EVENT_RXREQ_RNS, DDR_PMUV1_EVENT_RXREQ_WNSP,
	DDR_PMUV1_EVENT_RXREQ_WNSF, DDR_PMUV1_EVENT_BANDWIDTH
};

static const u32 ddr_pmu_v2_counter_reg_offset[] = {
	DDR_PMUV2_EVENT_CYCLES,
	DDR_PMUV2_EVENT_AXI_WRITE_FLUX_CNT, DDR_PMUV2_EVENT_AXI_READ_FLUX_CNT,
	DDR_PMUV2_EVENT_AXI_WRITE_CMD_CNT, DDR_PMUV2_EVENT_AXI_READ_CMD_CNT
};

#define PHYTIUM_DDR_PMUV1V2_EVENT_ATTR(_name, _config)                \
		PHYTIUM_PMU_ATTR(_name, phytium_ddr_pmu_event_sysfs_show, \
				 (unsigned long)_config)

static struct attribute *phytium_ddr_pmu_v1v2_format_attr[] = {
	PHYTIUM_DDR_PMU_FORMAT_ATTR(event, "config:0-2"),
	NULL,
};

static struct attribute *phytium_ddr_pmu_v1_events_attr[] = {
	PHYTIUM_DDR_PMUV1V2_EVENT_ATTR(ddr_cycles, 0x00),
	PHYTIUM_DDR_PMUV1V2_EVENT_ATTR(rxreq, 0x01),
	PHYTIUM_DDR_PMUV1V2_EVENT_ATTR(rxdat, 0x02),
	PHYTIUM_DDR_PMUV1V2_EVENT_ATTR(txdat, 0x03),
	PHYTIUM_DDR_PMUV1V2_EVENT_ATTR(rxreq_rns, 0x04),
	PHYTIUM_DDR_PMUV1V2_EVENT_ATTR(rxreq_wnsp, 0x05),
	PHYTIUM_DDR_PMUV1V2_EVENT_ATTR(rxreq_wnsf, 0x06),
	PHYTIUM_DDR_PMUV1V2_EVENT_ATTR(bandwidth, 0x07),
	NULL,
};

static struct attribute *phytium_ddr_pmu_v2_events_attr[] = {
	PHYTIUM_DDR_PMUV1V2_EVENT_ATTR(ddr_axi_cycles, 0x00),
	PHYTIUM_DDR_PMUV1V2_EVENT_ATTR(axi_write_flux, 0x01),
	PHYTIUM_DDR_PMUV1V2_EVENT_ATTR(axi_read_flux, 0x02),
	PHYTIUM_DDR_PMUV1V2_EVENT_ATTR(axi_write_cmd, 0x03),
	PHYTIUM_DDR_PMUV1V2_EVENT_ATTR(axi_read_cmd, 0x04),
	NULL,
};

static const u32 phytium_ddr_pmu_v3_event_num[] = {
	DDR_PMUV3_MC_DFI_CMD_EVENT_NUM,
	DDR_PMUV3_MC_UIF_CMD_EVENT_NUM,
	DDR_PMUV3_MC_BANK_SRC_EVENT_NUM,
	DDR_PMUV3_MC_CAM_OCCU_EVENT_NUM,
	DDR_PMUV3_MC_T_CMD_SCHE_EVENT_NUM,
	DDR_PMUV3_MC_CMD_SCHE_EVENT_NUM,
	DDR_PMUV3_PORT_CMD_EVENT_NUM,
	DDR_PMUV3_PORT_CMD_OPCODE_EVENT_NUM,
	DDR_PMUV3_PORT_CMD_RETRY_EVENT_NUM,
	DDR_PMUV3_PORT_PREF_STATUS_EVENT_NUM,
	DDR_PMUV3_MC_RDWR_SWTICH_EVENT_NUM,
	DDR_PMUV3_MC_T_RDWR_SWTICH_EVENT_NUM,
	DDR_PMUV3_LPR_FIFO_OCCU_EVENT_NUM,
	DDR_PMUV3_HPR_FIFO_OCCU_EVENT_NUM,
	DDR_PMUV3_WR_DCQ_OCCU_EVENT_NUM,
	DDR_PMUV3_RDAT_INFO_OCCU_EVENT_NUM,
	DDR_PMUV3_RDAT_FIFO_OCCU_EVENT_NUM,
	DDR_PMUV3_WDP_BUFFER_OCCU_EVENT_NUM,
	DDR_PMUV3_RSP_COMP_OCCU_EVENT_NUM,
	DDR_PMUV3_RSP_DBID_OCCU_EVENT_NUM,
	DDR_PMUV3_RSP_CRQ_OCCU_EVENT_NUM,
	DDR_PMUV3_RSP_REQ_OCCU_EVENT_NUM,
	DDR_PMUV3_RSP_RTQ_OCCU_EVENT_NUM,
	DDR_PMUV3_SPECIAL_EVENT_NUM
};

static const u32 phytium_ddr_pmu_v3_event_sel_reg_offset[] = {
	DDR_PMUV3_MC_DFI_CMD_CNT_SEL,
	DDR_PMUV3_MC_UIF_CNT_SEL,
	DDR_PMUV3_MC_CAM_OCCU_CNT_SEL,
	DDR_PMUV3_MC_CAM_OCCU_CNT_SEL,
	DDR_PMUV3_MC_CMD_SCHEDULING_CNT_SEL,
	DDR_PMUV3_MC_CMD_SCHEDULING_CNT_SEL,
	DDR_PMUV3_PORT_CMD_CNT_SEL,
	DDR_PMUV3_PORT_CMD_OPCODE_CNT_SEL,
	DDR_PMUV3_PORT_CMD_RETRY_CNT_SEL,
	DDR_PMUV3_PORT_PREF_STATUS_CNT_SEL,
	DDR_PMUV3_MC_RDWR_SWITCH_CNT_SEL,
	DDR_PMUV3_MC_RDWR_SWITCH_CNT_SEL,
	DDR_PMUV3_PORT_CQ_OCCU_CNT_SEL,
	DDR_PMUV3_PORT_CQ_OCCU_CNT_SEL,
	DDR_PMUV3_PORT_CQ_OCCU_CNT_SEL,
	DDR_PMUV3_PORT_DATA_OCCU_CNT_SEL,
	DDR_PMUV3_PORT_DATA_OCCU_CNT_SEL,
	DDR_PMUV3_PORT_DATA_OCCU_CNT_SEL,
	DDR_PMUV3_PORT_RSP_OCCU_CNT_SEL,
	DDR_PMUV3_PORT_RSP_OCCU_CNT_SEL,
	DDR_PMUV3_PORT_RSP_OCCU_CNT_SEL,
	DDR_PMUV3_PORT_RSP_OCCU_CNT_SEL,
	DDR_PMUV3_PORT_RSP_OCCU_CNT_SEL
};

static const u32 phytium_ddr_pmu_v3_cnt_sel_mask_offset[] = {
	DDR_PMUV3_MC_DFI_CMD_SEL_MASK,
	DDR_PMUV3_MC_UIF_CMD_SEL_MASK,
	DDR_PMUV3_MC_BANK_SRC_SEL_MASK,
	DDR_PMUV3_MC_CAM_OCCU_SEL_MASK,
	DDR_PMUV3_MC_T_CMD_SCHE_SEL_MASK,
	DDR_PMUV3_MC_CMD_SCHE_SEL_MASK,
	DDR_PMUV3_PORT_CMD_SEL_MASK,
	DDR_PMUV3_PORT_CMD_OPCODE_SEL_MASK,
	DDR_PMUV3_PORT_CMD_RETRY_SEL_MASK,
	DDR_PMUV3_PORT_PREF_STATUS_SEL_MASK,
	DDR_PMUV3_MC_RDWR_SWITCH_CNT_SEL_MASK,
	DDR_PMUV3_MC_T_RDWR_SWITCH_CNT_SEL_MASK,
	DDR_PMUV3_LPR_FIFO_OCCU_SEL_MASK,
	DDR_PMUV3_HPR_FIFO_OCCU_SEL_MASK,
	DDR_PMUV3_WR_DCQ_OCCU_SEL_MASK,
	DDR_PMUV3_RDAT_INFO_OCCU_SEL_MASK,
	DDR_PMUV3_RDAT_FIFO_OCCU_SEL_MASK,
	DDR_PMUV3_WDP_BUFFER_OCCU_SEL_MASK,
	DDR_PMUV3_RSP_COMP_OCCU_SEL_MASK,
	DDR_PMUV3_REP_DBID_OCCU_SEL_MASK,
	DDR_PMUV3_RSP_CRQ_OCCU_SEL_MASK,
	DDR_PMUV3_RSP_REQ_OCCU_SEL_MASK,
	DDR_PMUV3_RSP_RTQ_OCCU_SEL_MASK
};

static const u32 phytium_ddr_pmu_v3_thre_reg_offset[] = {
	DDR_PMUV3_LPR_FIFO_THRE,
	DDR_PMUV3_HPR_FIFO_THRE,
	DDR_PMUV3_WR_DCQ_THRE,
	DDR_PMUV3_WDP_BUFFER_THRE,
	DDR_PMUV3_RDAT_INFO_THRE,
	DDR_PMUV3_RDAT_FIFO_THRE,
	DDR_PMUV3_RSP_COMP_THRE,
	DDR_PMUV3_RSP_DBID_THRE,
	DDR_PMUV3_RSP_REQ_THRE,
	DDR_PMUV3_RSP_CRQ_THRE,
	DDR_PMUV3_RSP_RTQ_THRE
};

static const u32 phytium_ddr_pmu_v3_thre_bit_size[] = {
	DDR_PMUV3_LPR_FIFO_THRE_BIT_SIZE,
	DDR_PMUV3_HPR_FIFO_THRE_BIT_SIZE,
	DDR_PMUV3_WR_DCQ_THRE_BIT_SIZE,
	DDR_PMUV3_WDP_BUFFER_THRE_BIT_SIZE,
	DDR_PMUV3_RDAT_INFO_THRE_BIT_SIZE,
	DDR_PMUV3_RDAT_FIFO_THRE_BIT_SIZE,
	DDR_PMUV3_RSP_COMP_THRE_BIT_SIZE,
	DDR_PMUV3_RSP_DBID_THRE_BIT_SIZE,
	DDR_PMUV3_RSP_REQ_THRE_BIT_SIZE,
	DDR_PMUV3_RSP_CRQ_THRE_BIT_SIZE,
	DDR_PMUV3_RSP_RTQ_THRE_BIT_SIZE
};

static const u32 phytium_ddr_pmu_v3_genc_cnt_reg_offset[] = {
	DDR_PMUV3_MC_DFI_CMD_CNT_L,
	DDR_PMUV3_MC_UIF_CNT_L,
	DDR_PMUV3_MC_BANK_MAGT_CNT_L,
	DDR_PMUV3_MC_CAM_OCCU_CNT_L,
	DDR_PMUV3_MC_T_CMD_SCHE_CNT_L,
	DDR_PMUV3_MC_CMD_SCHE_CNT_L,
	DDR_PMUV3_PORT_CMD_CNT_L,
	DDR_PMUV3_PORT_CMD_OPCODE_CNT_L,
	DDR_PMUV3_PORT_RETRY_CNT_L,
	DDR_PMUV3_PORT_PREF_STATUS_CNT_L,
	DDR_PMUV3_MC_RDWR_SWITCH_CNT_L,
	DDR_PMUV3_MC_T_RDWR_SWITCH_CNT_L,
	DDR_PMUV3_PORT_CQ_OCCU_CNT_L,
	DDR_PMUV3_PORT_CQ_OCCU_CNT_L,
	DDR_PMUV3_PORT_CQ_OCCU_CNT_L,
	DDR_PMUV3_PORT_DATA_OCCU_CNT_L,
	DDR_PMUV3_PORT_DATA_OCCU_CNT_L,
	DDR_PMUV3_PORT_DATA_OCCU_CNT_L,
	DDR_PMUV3_PORT_RSP_OCCU_CNT_L,
	DDR_PMUV3_PORT_RSP_OCCU_CNT_L,
	DDR_PMUV3_PORT_RSP_OCCU_CNT_L,
	DDR_PMUV3_PORT_RSP_OCCU_CNT_L,
	DDR_PMUV3_PORT_RSP_OCCU_CNT_L
};

static const u32 phytium_ddr_pmu_v3_spec_cnt_reg_offset[] = {
	DDR_PMUV3_GLOBAL_CNT_L,
	DDR_PMUV3_MC_CQ_IDLE_CNT_L,
	DDR_PMUV3_MC_EXP_GPR_CNT_L,
	DDR_PMUV3_MC_EXP_GPW_CNT_L,
	DDR_PMUV3_MC_ADDR_COLLISION_CNT_L,
	DDR_PMUV3_MC_RDCAM_CRITICAL_CNT_L,
	DDR_PMUV3_MC_WRCAM_CRITICAL_CNT_L,
	DDR_PMUV3_MC_RETRY_CMD_CNT_L,
	DDR_PMUV3_PORT_WDAT_BE_CNT_L,
	DDR_PMUV3_PORT_WDAT_RM_BUFFER_DEALLOC_CNT_L,
	DDR_PMUV3_PORT_RD_CMD_DELAY1,
	DDR_PMUV3_PORT_WR_CMD_DELAY1
};

static const u32 phytium_ddr_pmu_v3_genc_cnt_overflow_bit[] = {
	0, // DDR_PMUV3_MC_DFI_CMD_CNT_L,
	1, // DDR_PMUV3_MC_UIF_CNT_L,
	2, // DDR_PMUV3_MC_BANK_MAGT_CNT_L,
	-1, // DDR_PMUV3_MC_CAM_OCCU_CNT_L,
	-1, // DDR_PMUV3_MC_T_CMD_SCHE_CNT_L,
	-1, // DDR_PMUV3_MC_CMD_SCHE_CNT_L,
	7, // DDR_PMUV3_PORT_CMD_CNT_L,
	8, // DDR_PMUV3_PORT_CMD_OPCODE_CNT_L,
	9, // DDR_PMUV3_PORT_RETRY_CNT_L,
	10, // DDR_PMUV3_PORT_PREF_STATUS_CNT_L,
	-1, // DDR_PMUV3_MC_RDWR_SWITCH_CNT_L
	-1, // DDR_PMUV3_MC_T_RDWR_SWITCH_CNT_L
	-1, // DDR_PMUV3_PORT_CQ_OCCU_CNT_L,
	-1, // DDR_PMUV3_PORT_CQ_OCCU_CNT_L,
	-1, // DDR_PMUV3_PORT_CQ_OCCU_CNT_L,
	-1, // DDR_PMUV3_PORT_DATA_OCCU_CNT_L,
	-1, // DDR_PMUV3_PORT_DATA_OCCU_CNT_L,
	-1, // DDR_PMUV3_PORT_DATA_OCCU_CNT_L,
	-1, // DDR_PMUV3_PORT_RSP_OCCU_CNT_L,
	-1, // DDR_PMUV3_PORT_RSP_OCCU_CNT_L,
	-1, // DDR_PMUV3_PORT_RSP_OCCU_CNT_L,
	-1, // DDR_PMUV3_PORT_RSP_OCCU_CNT_L,
	-1 // DDR_PMUV3_PORT_RSP_OCCU_CNT_L
};

static const int phytium_ddr_pmu_v3_spec_cnt_overflow_bit[] = {
	-1,
	3, // DDR_PMUV3_MC_CQ_IDLE_CNT_L,
	4, // DDR_PMUV3_MC_EXP_GPR_CNT_L,
	5, // DDR_PMUV3_MC_EXP_GPW_CNT_L,
	6, // DDR_PMUV3_MC_ADDR_COLLISION_CNT_L,
	-1, // DDR_PMUV3_MC_RDCAM_CRITICAL_CNT_L,
	-1, // DDR_PMUV3_MC_WRCAM_CRITICAL_CNT_L,
	-1, // DDR_PMUV3_MC_RETRY_CMD_CNT_L,
	11, // DDR_PMUV3_PORT_WDAT_BE_CNT_L,
	12, // DDR_PMUV3_PORT_WDAT_RM_BUFFER_DEALLOC_CNT_L,
	-1, // DDR_PMUV3_PORT_RD_CMD_DELAY1,
	-1 // DDR_PMUV3_PORT_WR_CMD_DELAY1
};

#define DDR_PMUV3_EVENT_ATTR(_name, _type, _eventid, _cntsize, _occuflag)               \
	(&((struct phytium_ddr_pmu_v3_event_attr[]){{                                    \
		.attr = __ATTR(_name, 0444, phytium_ddr_pmu_v3_event_show, NULL),                \
		.type = _type,                                                            \
		.eventid = _eventid,                                                      \
		.cntsize = _cntsize,                                                      \
		.occuflag = _occuflag,                                                    \
	}})[0]                                                                        \
		.attr.attr)

#define DDR_PMUV3_SPECIAL_EVENT(_name, _event, _cntsize)                                 \
	DDR_PMUV3_EVENT_ATTR(_name, DDR_PMUV3_SPECIAL_EVTYPE, _event, _cntsize, 0)
#define DDR_PMUV3_MC_DFI_CMD_EVENT(_name, _event)                                        \
	DDR_PMUV3_EVENT_ATTR(mc_dfi_##_name, DDR_PMUV3_MC_DFI_CMD_EVTYPE, _event, 32, 0)
#define DDR_PMUV3_MC_UIF_CMD_EVENT(_name, _event)                                        \
	DDR_PMUV3_EVENT_ATTR(mc_uif_##_name, DDR_PMUV3_MC_UIF_CMD_EVTYPE, _event, 32, 0)
#define DDR_PMUV3_MC_BANK_SRC_EVENT(_name, _event)                                       \
	DDR_PMUV3_EVENT_ATTR(mc_bank_mgt_##_name, DDR_PMUV3_MC_BANK_SRC_EVTYPE, _event, 32, 0)
#define DDR_PMUV3_MC_CAM_OCCU_EVENT(_name, _event)                                        \
	DDR_PMUV3_EVENT_ATTR(mc_occu_##_name, DDR_PMUV3_MC_CAM_OCCU_EVTYPE, _event, 64, 1)
#define DDR_PMUV3_MC_T_CMD_SCHE_EVENT(_name, _event)                                       \
	DDR_PMUV3_EVENT_ATTR(mc_t_cmd_sche_##_name, DDR_PMUV3_MC_T_CMD_SCHE_EVTYPE, _event, 64, 0)
#define DDR_PMUV3_MC_CMD_SCHE_EVENT(_name, _event)                                         \
	DDR_PMUV3_EVENT_ATTR(mc_cmd_sche_##_name, DDR_PMUV3_MC_CMD_SCHE_EVTYPE, _event, 64, 0)
#define DDR_PMUV3_PORT_CMD_EVENT(_name, _event)                                            \
	DDR_PMUV3_EVENT_ATTR(port_##_name, DDR_PMUV3_PORT_CMD_EVTYPE, _event, 32, 0)
#define DDR_PMUV3_PORT_CMD_OPCODE_EVENT(_name, _event)                                      \
	DDR_PMUV3_EVENT_ATTR(port_opcode_##_name, DDR_PMUV3_PORT_CMD_OPCODE_EVTYPE, _event, 32, 0)
#define DDR_PMUV3_PORT_CMD_RETRY_EVENT(_name, _event)                                       \
	DDR_PMUV3_EVENT_ATTR(port_retry_##_name, DDR_PMUV3_PORT_CMD_RETRY_EVTYPE, _event, 32, 0)
#define DDR_PMUV3_PORT_PREF_STATUS_EVENT(_name, _event)                                     \
	DDR_PMUV3_EVENT_ATTR(port_pref_##_name, DDR_PMUV3_PORT_PREF_STATUS_EVTYPE, _event, 32, 0)

#define DDR_PMUV3_LPR_FIFO_OCCU_EVENT(_name, _event)                                        \
	DDR_PMUV3_EVENT_ATTR(port_cq_occu_lpr_##_name, DDR_PMUV3_LPR_FIFO_OCCU_EVTYPE, _event, \
						64, 1)
#define DDR_PMUV3_HPR_FIFO_OCCU_EVENT(_name, _event)                                        \
	DDR_PMUV3_EVENT_ATTR(port_cq_occu_hpr_##_name, DDR_PMUV3_HPR_FIFO_OCCU_EVTYPE, _event, \
						64, 1)
#define DDR_PMUV3_WR_DCQ_OCCU_EVENT(_name, _event)                                          \
	DDR_PMUV3_EVENT_ATTR(port_cq_occu_dwr_cq_##_name, DDR_PMUV3_WR_DCQ_OCCU_EVTYPE, _event, \
						64, 1)

#define DDR_PMUV3_RDAT_INFO_OCCU_EVENT(_name, _event)                                        \
	DDR_PMUV3_EVENT_ATTR(port_data_occu_rdatinfo_##_name, DDR_PMUV3_RDAT_INFO_OCCU_EVTYPE, \
						_event, 64, 1)
#define DDR_PMUV3_RDAT_FIFO_OCCU_EVENT(_name, _event)                                        \
	DDR_PMUV3_EVENT_ATTR(port_data_occu_rdatfifo_##_name, DDR_PMUV3_RDAT_FIFO_OCCU_EVTYPE, \
						_event, 64, 1)
#define DDR_PMUV3_WDP_BUFFER_OCCU_EVENT(_name, _event)                                       \
	DDR_PMUV3_EVENT_ATTR(port_data_occu_wdp_buffer_##_name, DDR_PMUV3_WDP_BUFFER_OCCU_EVTYPE,\
					_event, 64, 1)

#define DDR_PMUV3_RSP_COMP_OCCU_EVENT(_name, _event)                                         \
	DDR_PMUV3_EVENT_ATTR(port_rsp_occu_rspcomp_##_name, DDR_PMUV3_RSP_COMP_OCCU_EVTYPE, \
					_event, 64, 1)
#define DDR_PMUV3_RSP_DBID_OCCU_EVENT(_name, _event)                                         \
	DDR_PMUV3_EVENT_ATTR(port_rsp_occu_rspdbid_##_name, DDR_PMUV3_RSP_DBID_OCCU_EVTYPE, \
					_event, 64, 1)
#define DDR_PMUV3_RSP_CRQ_OCCU_EVENT(_name, _event)                                          \
	DDR_PMUV3_EVENT_ATTR(port_rsp_occu_rspcrq_##_name, DDR_PMUV3_RSP_CRQ_OCCU_EVTYPE, _event, \
						64, 1)
#define DDR_PMUV3_RSP_REQ_OCCU_EVENT(_name, _event)                                          \
	DDR_PMUV3_EVENT_ATTR(port_rsp_occu_rspreq_##_name, DDR_PMUV3_RSP_REQ_OCCU_EVTYPE, _event, \
						64, 1)
#define DDR_PMUV3_RSP_RTQ_OCCU_EVENT(_name, _event)                                          \
	DDR_PMUV3_EVENT_ATTR(port_rsp_occu_rsprtq_##_name, DDR_PMUV3_RSP_RTQ_OCCU_EVTYPE, _event, \
						64, 1)
#define DDR_PMUV3_MC_RDWR_SWTICH_EVENT(_name, _event)                                        \
	DDR_PMUV3_EVENT_ATTR(mc_switch_##_name, DDR_PMUV3_MC_RDWR_SWTICH_EVTYPE, _event, 64, 0)
#define DDR_PMUV3_MC_T_RDWR_SWTICH_EVENT(_name, _event)                                      \
	DDR_PMUV3_EVENT_ATTR(mc_switch_t_##_name, DDR_PMUV3_MC_T_RDWR_SWTICH_EVTYPE, _event, 64, 0)

#define PHYTIUM_DDR_PMUV3_EVENT_ATTR_EXTRACTOR(_name, _config, _start, _end)        \
static inline u32 get_##_name(struct perf_event *event)            \
{                                                                  \
	return FIELD_GET(GENMASK_ULL(_end, _start),                \
				event->attr._config);                     \
}

PHYTIUM_DDR_PMUV3_EVENT_ATTR_EXTRACTOR(eventid, config, 0, 4);
PHYTIUM_DDR_PMUV3_EVENT_ATTR_EXTRACTOR(type, config, 5, 9);
PHYTIUM_DDR_PMUV3_EVENT_ATTR_EXTRACTOR(cntsize, config, 10, 16);
PHYTIUM_DDR_PMUV3_EVENT_ATTR_EXTRACTOR(occuflag, config, 17, 17);
PHYTIUM_DDR_PMUV3_EVENT_ATTR_EXTRACTOR(low_thre, config, 18, 24);
PHYTIUM_DDR_PMUV3_EVENT_ATTR_EXTRACTOR(hight_thre, config, 25, 31);


static struct attribute *phytium_ddr_pmu_v3_format_attr[] = {
	PHYTIUM_DDR_PMU_FORMAT_ATTR(eventid, "config:0-4"),
	PHYTIUM_DDR_PMU_FORMAT_ATTR(type, "config:5-9"),
	PHYTIUM_DDR_PMU_FORMAT_ATTR(cntsize, "config:10-16"),
	PHYTIUM_DDR_PMU_FORMAT_ATTR(occuflag, "config:17"),
	PHYTIUM_DDR_PMU_FORMAT_ATTR(low_thre, "config:18-24"),
	PHYTIUM_DDR_PMU_FORMAT_ATTR(hight_thre, "config:25-31"),
	NULL,
};

static struct attribute *phytium_ddr_pmu_v3_events_attr[] = {
	DDR_PMUV3_MC_DFI_CMD_EVENT(act_cnt, 0x0), DDR_PMUV3_MC_DFI_CMD_EVENT(pdx_cnt, 0x1),
	DDR_PMUV3_MC_DFI_CMD_EVENT(pde_cnt, 0x2), DDR_PMUV3_MC_DFI_CMD_EVENT(mrs_cnt, 0x3),
	DDR_PMUV3_MC_DFI_CMD_EVENT(rd_cnt, 0x4), DDR_PMUV3_MC_DFI_CMD_EVENT(rda_cnt, 0x5),
	DDR_PMUV3_MC_DFI_CMD_EVENT(wr_cnt, 0x6), DDR_PMUV3_MC_DFI_CMD_EVENT(wra_cnt, 0x7),
	DDR_PMUV3_MC_DFI_CMD_EVENT(pre_cnt, 0x8), DDR_PMUV3_MC_DFI_CMD_EVENT(refpb_cnt, 0x9),
	DDR_PMUV3_MC_DFI_CMD_EVENT(refab_cnt, 0xa), DDR_PMUV3_MC_DFI_CMD_EVENT(rfmab_cnt, 0xb),
	DDR_PMUV3_MC_DFI_CMD_EVENT(rfmpb_cnt, 0xc), DDR_PMUV3_MC_DFI_CMD_EVENT(ctrlupd_cnt, 0xd),
	DDR_PMUV3_MC_DFI_CMD_EVENT(phyupd_cnt, 0xe),

	DDR_PMUV3_MC_UIF_CMD_EVENT(hpr_cnt, 0x0), DDR_PMUV3_MC_UIF_CMD_EVENT(lpr_cnt, 0x1),
	DDR_PMUV3_MC_UIF_CMD_EVENT(gpr_cnt, 0x2), DDR_PMUV3_MC_UIF_CMD_EVENT(tpw_cnt, 0x3),
	DDR_PMUV3_MC_UIF_CMD_EVENT(gpw_cnt, 0x4),

	DDR_PMUV3_MC_BANK_SRC_EVENT(new_cnt, 0x0), DDR_PMUV3_MC_BANK_SRC_EVENT(hit_cnt, 0x1),
	DDR_PMUV3_MC_BANK_SRC_EVENT(reallocate_cnt, 0x2),

	DDR_PMUV3_MC_CAM_OCCU_EVENT(lgpr_thre16_cnt, 0x0),
	DDR_PMUV3_MC_CAM_OCCU_EVENT(lgpr_thre24_cnt, 0x1),
	DDR_PMUV3_MC_CAM_OCCU_EVENT(lgpr_thre32_cnt, 0x2),
	DDR_PMUV3_MC_CAM_OCCU_EVENT(lgpr_thre40_cnt, 0x3),
	DDR_PMUV3_MC_CAM_OCCU_EVENT(lgpr_thre48_cnt, 0x4),
	DDR_PMUV3_MC_CAM_OCCU_EVENT(lgpr_thre64_cnt, 0x5),
	DDR_PMUV3_MC_CAM_OCCU_EVENT(hpr_thre16_cnt, 0x6),
	DDR_PMUV3_MC_CAM_OCCU_EVENT(hpr_thre24_cnt, 0x7),
	DDR_PMUV3_MC_CAM_OCCU_EVENT(hpr_thre32_cnt, 0x8),
	DDR_PMUV3_MC_CAM_OCCU_EVENT(hpr_thre40_cnt, 0x9),
	DDR_PMUV3_MC_CAM_OCCU_EVENT(hpr_thre48_cnt, 0xa),
	DDR_PMUV3_MC_CAM_OCCU_EVENT(hpr_thre64_cnt, 0xb),
	DDR_PMUV3_MC_CAM_OCCU_EVENT(tpw_thre16_cnt, 0xc),
	DDR_PMUV3_MC_CAM_OCCU_EVENT(tpw_thre24_cnt, 0xd),
	DDR_PMUV3_MC_CAM_OCCU_EVENT(tpw_thre32_cnt, 0xe),
	DDR_PMUV3_MC_CAM_OCCU_EVENT(tpw_thre40_cnt, 0xf),
	DDR_PMUV3_MC_CAM_OCCU_EVENT(tpw_thre48_cnt, 0x10),
	DDR_PMUV3_MC_CAM_OCCU_EVENT(tpw_thre64_cnt, 0x11),

	DDR_PMUV3_MC_T_CMD_SCHE_EVENT(rd2rd_diff_rank_cnt, 0x0),
	DDR_PMUV3_MC_T_CMD_SCHE_EVENT(rd2rd_diff_bg_cnt, 0x1),
	DDR_PMUV3_MC_T_CMD_SCHE_EVENT(rd2rd_diff_page_cnt, 0x2),
	DDR_PMUV3_MC_T_CMD_SCHE_EVENT(rd2rd_same_page_cnt, 0x3),
	DDR_PMUV3_MC_T_CMD_SCHE_EVENT(rd2wr_diff_rank_cnt, 0x4),
	DDR_PMUV3_MC_T_CMD_SCHE_EVENT(rd2wr_diff_bg_cnt, 0x5),
	DDR_PMUV3_MC_T_CMD_SCHE_EVENT(rd2wr_diff_page_cnt, 0x6),
	DDR_PMUV3_MC_T_CMD_SCHE_EVENT(rd2wr_same_page_cnt, 0x7),
	DDR_PMUV3_MC_T_CMD_SCHE_EVENT(wr2wr_diff_rank_cnt, 0x8),
	DDR_PMUV3_MC_T_CMD_SCHE_EVENT(wr2wr_diff_bg_cnt, 0x9),
	DDR_PMUV3_MC_T_CMD_SCHE_EVENT(wr2wr_diff_page_cnt, 0xa),
	DDR_PMUV3_MC_T_CMD_SCHE_EVENT(wr2wr_same_page_cnt, 0xb),
	DDR_PMUV3_MC_T_CMD_SCHE_EVENT(wr2rd_diff_rank_cnt, 0xc),
	DDR_PMUV3_MC_T_CMD_SCHE_EVENT(wr2rd_diff_bg_cnt, 0xd),
	DDR_PMUV3_MC_T_CMD_SCHE_EVENT(wr2rd_diff_page_cnt, 0xe),
	DDR_PMUV3_MC_T_CMD_SCHE_EVENT(wr2rd_same_page_cnt, 0xf),

	DDR_PMUV3_MC_CMD_SCHE_EVENT(rd2rd_diff_rank_cnt, 0x0),
	DDR_PMUV3_MC_CMD_SCHE_EVENT(rd2rd_diff_bg_cnt, 0x1),
	DDR_PMUV3_MC_CMD_SCHE_EVENT(rd2rd_diff_page_cnt, 0x2),
	DDR_PMUV3_MC_CMD_SCHE_EVENT(rd2rd_same_page_cnt, 0x3),
	DDR_PMUV3_MC_CMD_SCHE_EVENT(rd2wr_diff_rank_cnt, 0x4),
	DDR_PMUV3_MC_CMD_SCHE_EVENT(rd2wr_diff_bg_cnt, 0x5),
	DDR_PMUV3_MC_CMD_SCHE_EVENT(rd2wr_diff_page_cnt, 0x6),
	DDR_PMUV3_MC_CMD_SCHE_EVENT(rd2wr_same_page_cnt, 0x7),
	DDR_PMUV3_MC_CMD_SCHE_EVENT(wr2wr_diff_rank_cnt, 0x8),
	DDR_PMUV3_MC_CMD_SCHE_EVENT(wr2wr_diff_bg_cnt, 0x9),
	DDR_PMUV3_MC_CMD_SCHE_EVENT(wr2wr_diff_page_cnt, 0xa),
	DDR_PMUV3_MC_CMD_SCHE_EVENT(wr2wr_same_page_cnt, 0xb),
	DDR_PMUV3_MC_CMD_SCHE_EVENT(wr2rd_diff_rank_cnt, 0xc),
	DDR_PMUV3_MC_CMD_SCHE_EVENT(wr2rd_diff_bg_cnt, 0xd),
	DDR_PMUV3_MC_CMD_SCHE_EVENT(wr2rd_diff_page_cnt, 0xe),
	DDR_PMUV3_MC_CMD_SCHE_EVENT(wr2rd_same_page_cnt, 0xf),

	DDR_PMUV3_PORT_CMD_EVENT(hpr_cnt, 0x0), DDR_PMUV3_PORT_CMD_EVENT(lpr_cnt, 0x1),
	DDR_PMUV3_PORT_CMD_EVENT(gpr_cnt, 0x2), DDR_PMUV3_PORT_CMD_EVENT(tpw_cnt, 0x3),
	DDR_PMUV3_PORT_CMD_EVENT(gpw_cnt, 0x4), DDR_PMUV3_PORT_CMD_EVENT(gpr_expired_cnt, 0x5),
	DDR_PMUV3_PORT_CMD_EVENT(gpw_expired_cnt, 0x6),

	DDR_PMUV3_PORT_CMD_OPCODE_EVENT(pcrd_return_cnt, 0x0),
	DDR_PMUV3_PORT_CMD_OPCODE_EVENT(read_no_snp_cnt, 0x1),
	DDR_PMUV3_PORT_CMD_OPCODE_EVENT(write_no_snpfull_cnt, 0x2),
	DDR_PMUV3_PORT_CMD_OPCODE_EVENT(write_no_snp_ptl_cnt, 0x3),
	DDR_PMUV3_PORT_CMD_OPCODE_EVENT(prefetch_tgt_cnt, 0x4),
	DDR_PMUV3_PORT_CMD_OPCODE_EVENT(cleansharepersist_cnt, 0x5),

	DDR_PMUV3_PORT_CMD_RETRY_EVENT(hpr_cnt, 0x0), DDR_PMUV3_PORT_CMD_RETRY_EVENT(lpr_cnt, 0x1),
	DDR_PMUV3_PORT_CMD_RETRY_EVENT(gpr_cnt, 0x2), DDR_PMUV3_PORT_CMD_RETRY_EVENT(tpw_cnt, 0x3),
	DDR_PMUV3_PORT_CMD_RETRY_EVENT(gpw_cnt, 0x4),

	DDR_PMUV3_PORT_PREF_STATUS_EVENT(hit_cnt, 0x0),
	DDR_PMUV3_PORT_PREF_STATUS_EVENT(invalid_cnt, 0x1),
	DDR_PMUV3_PORT_PREF_STATUS_EVENT(replace_cnt, 0x2),
	DDR_PMUV3_PORT_PREF_STATUS_EVENT(discard_bcs_resource_cnt, 0x3),
	DDR_PMUV3_PORT_PREF_STATUS_EVENT(discard_bcs_addr_coll_cnt, 0x4),

	DDR_PMUV3_MC_RDWR_SWTICH_EVENT(rdw_1cmd_cnt, 0x0),
	DDR_PMUV3_MC_RDWR_SWTICH_EVENT(rdw_2cmd_cnt, 0x1),
	DDR_PMUV3_MC_RDWR_SWTICH_EVENT(rdw_3to4cmd_cnt, 0x2),
	DDR_PMUV3_MC_RDWR_SWTICH_EVENT(rdw_5to8cmd_cnt, 0x3),
	DDR_PMUV3_MC_RDWR_SWTICH_EVENT(rdw_9to12cmd_cnt, 0x4),
	DDR_PMUV3_MC_RDWR_SWTICH_EVENT(rdw_13cmd_cnt, 0x5),
	DDR_PMUV3_MC_RDWR_SWTICH_EVENT(wdr_1cmd_cnt, 0x6),
	DDR_PMUV3_MC_RDWR_SWTICH_EVENT(wdr_2cmd_cnt, 0x7),
	DDR_PMUV3_MC_RDWR_SWTICH_EVENT(wdr_3to4cmd_cnt, 0x8),
	DDR_PMUV3_MC_RDWR_SWTICH_EVENT(wdr_5to8cmd_cnt, 0x9),
	DDR_PMUV3_MC_RDWR_SWTICH_EVENT(wdr_9to12cmd_cnt, 0xa),
	DDR_PMUV3_MC_RDWR_SWTICH_EVENT(wdr_ge13cmd_cnt, 0xb),

	DDR_PMUV3_MC_T_RDWR_SWTICH_EVENT(rdw_1cmd_cnt, 0x0),
	DDR_PMUV3_MC_T_RDWR_SWTICH_EVENT(rdw_2cmd_cnt, 0x1),
	DDR_PMUV3_MC_T_RDWR_SWTICH_EVENT(rdw_3to4cmd_cnt, 0x2),
	DDR_PMUV3_MC_T_RDWR_SWTICH_EVENT(rdw_5to8cmd_cnt, 0x3),
	DDR_PMUV3_MC_T_RDWR_SWTICH_EVENT(rdw_9to12cmd_cnt, 0x4),
	DDR_PMUV3_MC_T_RDWR_SWTICH_EVENT(rdw_13cmd_cnt, 0x5),
	DDR_PMUV3_MC_T_RDWR_SWTICH_EVENT(wdr_1cmd_cnt, 0x6),
	DDR_PMUV3_MC_T_RDWR_SWTICH_EVENT(wdr_2cmd_cnt, 0x7),
	DDR_PMUV3_MC_T_RDWR_SWTICH_EVENT(wdr_3to4cmd_cnt, 0x8),
	DDR_PMUV3_MC_T_RDWR_SWTICH_EVENT(wdr_5to8cmd_cnt, 0x9),
	DDR_PMUV3_MC_T_RDWR_SWTICH_EVENT(wdr_9to12cmd_cnt, 0xa),
	DDR_PMUV3_MC_T_RDWR_SWTICH_EVENT(wdr_ge13cmd_cnt, 0xb),

	DDR_PMUV3_LPR_FIFO_OCCU_EVENT(low_thre, 0x0), DDR_PMUV3_LPR_FIFO_OCCU_EVENT(med_thre, 0x1),
	DDR_PMUV3_LPR_FIFO_OCCU_EVENT(high_thre, 0x2),
	DDR_PMUV3_HPR_FIFO_OCCU_EVENT(low_thre, 0x3), DDR_PMUV3_HPR_FIFO_OCCU_EVENT(med_thre, 0x4),
	DDR_PMUV3_HPR_FIFO_OCCU_EVENT(high_thre, 0x5),
	DDR_PMUV3_WR_DCQ_OCCU_EVENT(low_thre, 0x6), DDR_PMUV3_WR_DCQ_OCCU_EVENT(med_thre, 0x7),
	DDR_PMUV3_WR_DCQ_OCCU_EVENT(high_thre, 0x8),

	DDR_PMUV3_RDAT_INFO_OCCU_EVENT(low_thre, 0x0),
	DDR_PMUV3_RDAT_INFO_OCCU_EVENT(med_thre, 0x1),
	DDR_PMUV3_RDAT_INFO_OCCU_EVENT(high_thre, 0x2),
	DDR_PMUV3_RDAT_FIFO_OCCU_EVENT(low_thre, 0x3),
	DDR_PMUV3_RDAT_FIFO_OCCU_EVENT(med_thre, 0x4),
	DDR_PMUV3_RDAT_FIFO_OCCU_EVENT(high_thre, 0x5),
	DDR_PMUV3_WDP_BUFFER_OCCU_EVENT(low_thre, 0x6),
	DDR_PMUV3_WDP_BUFFER_OCCU_EVENT(med_thre, 0x7),
	DDR_PMUV3_WDP_BUFFER_OCCU_EVENT(high_thre, 0x8),

	DDR_PMUV3_RSP_COMP_OCCU_EVENT(low_thre, 0x0), DDR_PMUV3_RSP_COMP_OCCU_EVENT(med_thre, 0x1),
	DDR_PMUV3_RSP_COMP_OCCU_EVENT(high_thre, 0x2),
	DDR_PMUV3_RSP_DBID_OCCU_EVENT(low_thre, 0x3), DDR_PMUV3_RSP_DBID_OCCU_EVENT(med_thre, 0x4),
	DDR_PMUV3_RSP_DBID_OCCU_EVENT(high_thre, 0x5),
	DDR_PMUV3_RSP_CRQ_OCCU_EVENT(low_thre, 0x6), DDR_PMUV3_RSP_CRQ_OCCU_EVENT(med_thre, 0x7),
	DDR_PMUV3_RSP_CRQ_OCCU_EVENT(high_thre, 0x8),
	DDR_PMUV3_RSP_REQ_OCCU_EVENT(low_thre, 0x9), DDR_PMUV3_RSP_REQ_OCCU_EVENT(med_thre, 0xa),
	DDR_PMUV3_RSP_REQ_OCCU_EVENT(high_thre, 0xb),
	DDR_PMUV3_RSP_RTQ_OCCU_EVENT(low_thre, 0xc), DDR_PMUV3_RSP_RTQ_OCCU_EVENT(med_thre, 0xd),
	DDR_PMUV3_RSP_RTQ_OCCU_EVENT(high_thre, 0xe),

	DDR_PMUV3_SPECIAL_EVENT(dmu_cycles, 0x0, 64), DDR_PMUV3_SPECIAL_EVENT(mc_cq_idle, 0x1, 32),
	DDR_PMUV3_SPECIAL_EVENT(mc_exp_gpr, 0x2, 32), DDR_PMUV3_SPECIAL_EVENT(mc_exp_gpw, 0x3, 32),
	DDR_PMUV3_SPECIAL_EVENT(mc_addr_collision, 0x4, 32),
	DDR_PMUV3_SPECIAL_EVENT(mc_rdcam_critical, 0x5, 64),
	DDR_PMUV3_SPECIAL_EVENT(mc_wrcam_critical, 0x6, 64),
	DDR_PMUV3_SPECIAL_EVENT(mc_retry_cmd, 0x7, 32),
	DDR_PMUV3_SPECIAL_EVENT(port_wdat_be, 0x8, 32),
	DDR_PMUV3_SPECIAL_EVENT(port_wdat_rm_buffer_dealloc, 0x9, 32),
	DDR_PMUV3_SPECIAL_EVENT(port_rd_cmd_delay, 0xa, 64),
	DDR_PMUV3_SPECIAL_EVENT(port_wr_cmd_delay, 0xb, 64),
	NULL,
};

static const struct attribute_group phytium_ddr_pmu_v1v2_format_group = {
	.name = "format",
	.attrs = phytium_ddr_pmu_v1v2_format_attr,
};

static const struct attribute_group phytium_ddr_pmu_v3_format_group = {
	.name = "format",
	.attrs = phytium_ddr_pmu_v3_format_attr,
};

static const struct attribute_group phytium_ddr_pmu_v1_events_group = {
	.name = "events",
	.attrs = phytium_ddr_pmu_v1_events_attr,
};

static const struct attribute_group phytium_ddr_pmu_v2_events_group = {
	.name = "events",
	.attrs = phytium_ddr_pmu_v2_events_attr,
};

static const struct attribute_group phytium_ddr_pmu_v3_events_group = {
	.name = "events",
	.attrs = phytium_ddr_pmu_v3_events_attr,
};

static DEVICE_ATTR_RO(cpumask);

static struct attribute *phytium_ddr_pmu_cpumask_attrs[] = {
	&dev_attr_cpumask.attr,
	NULL,
};

static const struct attribute_group phytium_ddr_pmu_cpumask_attr_group = {
	.attrs = phytium_ddr_pmu_cpumask_attrs,
};

static const struct attribute_group *phytium_ddr_pmu_v1_attr_groups[] = {
	&phytium_ddr_pmu_v1v2_format_group,
	&phytium_ddr_pmu_v1_events_group,
	&phytium_ddr_pmu_cpumask_attr_group,
	NULL,
};

static const struct attribute_group *phytium_ddr_pmu_v2_attr_groups[] = {
	&phytium_ddr_pmu_v1v2_format_group,
	&phytium_ddr_pmu_v2_events_group,
	&phytium_ddr_pmu_cpumask_attr_group,
	NULL,
};

static const struct attribute_group *phytium_ddr_pmu_v3_attr_groups[] = {
	&phytium_ddr_pmu_v3_format_group,
	&phytium_ddr_pmu_v3_events_group,
	&phytium_ddr_pmu_cpumask_attr_group,
	NULL,
};

#if IS_ENABLED(CONFIG_ARM_PHYTIUM_DMU_DEVFREQ)
extern struct blocking_notifier_head dmu_pmu_notifier_chain;

void phytium_ddr_pmu_v2_notifier_chain_trigger(struct phytium_ddr_pmu *ddr_pmu, int event)
{
	static bool start_flag;

	if ((event == DDR_PMUV2_NOTICE_START) && (start_flag == false)) {
		blocking_notifier_call_chain(&dmu_pmu_notifier_chain, event, NULL);
		start_flag = true;
		ddr_pmu->used_flag = true;
	} else if ((event == DDR_PMUV2_NOTICE_STOP) && (start_flag == true)) {
		blocking_notifier_call_chain(&dmu_pmu_notifier_chain, event, NULL);
		start_flag = false;
		ddr_pmu->used_flag = false;
	}
}
#endif

static u64 phytium_ddr_pmu_v1_read_counter(struct phytium_ddr_pmu *ddr_pmu,
					   struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;
	u32 idx = GET_DDR_PMUV1V2_EVENTID(hwc);
	u32 cycle_l, cycle_h, w_data, ddr_data_width;
	u32 counter_offset = ddr_pmu_v1_counter_reg_offset[idx];
	u64 val64 = 0;
	int i;

	if (!EVENT_VALID_V1(idx)) {
		dev_err(ddr_pmu->dev, "Unsupported event index:%d!\n", idx);
		return 0;
	}

	switch (idx) {
	case 0:
		cycle_l = readl(ddr_pmu->base + counter_offset);
		cycle_h = readl(ddr_pmu->base + counter_offset + 4);
		val64 = (u64)cycle_h << 32 | (u64)cycle_l;
		break;
	case 7:
		ddr_data_width = readl(ddr_pmu->base + DDR_PMUV1_DATA_WIDTH);
		for (i = 0; i < (ddr_data_width / 8); i++) {
			w_data = readl(ddr_pmu->base + counter_offset + 4 * i);
			val64 += w_data;
		}
		break;
	default:
		val64 = readl(ddr_pmu->base + counter_offset);
		break;
	}

	return val64;
}

static u64 phytium_ddr_pmu_v2_read_counter(struct phytium_ddr_pmu *ddr_pmu,
					   struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;
	u32 val32_l, val32_h, idx, counter_offset;
	u64 val64;

	idx = GET_DDR_PMUV1V2_EVENTID(hwc);
	counter_offset = ddr_pmu_v2_counter_reg_offset[idx];

	if (!EVENT_VALID_V2(idx)) {
		dev_err(ddr_pmu->dev, "Unsupported event index:%d!\n", idx);
		return 0;
	}

	val32_l = readl(ddr_pmu->base + counter_offset);
	val32_h = readl(ddr_pmu->base + counter_offset + 4);
	val64 = (u64)val32_h << 32 | (u64)val32_l;

	return val64;
}

static u64 phytium_ddr_pmu_v3_read_counter(struct phytium_ddr_pmu *ddr_pmu,
					struct perf_event *event)
{
	u32 evid, evtype, cntsize;
	u32 cnt_offset, sel_offset, sel_mask;
	u32 val32_l, val32_h;
	u32 overflow_flag;
	u64 val64;
	int overflow_flag_bit;
	unsigned long overflow_mask;

	evid = get_eventid(event);
	evtype = get_type(event);
	cntsize = get_cntsize(event);
	val64 = 0;
	switch (evtype) {
	case DDR_PMUV3_SPECIAL_EVTYPE:
		cnt_offset = phytium_ddr_pmu_v3_spec_cnt_reg_offset[evid];
		overflow_flag_bit = phytium_ddr_pmu_v3_spec_cnt_overflow_bit[evid];

		overflow_mask = readl(ddr_pmu->base + DDR_PMUV3_CNT_OVERFLOW_FLAG);
		if (overflow_flag_bit > 0)
			overflow_flag = (u32)(overflow_mask & (1 << overflow_flag_bit));
		break;
	default:
		cnt_offset = phytium_ddr_pmu_v3_genc_cnt_reg_offset[evtype];
		sel_offset = phytium_ddr_pmu_v3_event_sel_reg_offset[evtype];
		sel_mask = phytium_ddr_pmu_v3_cnt_sel_mask_offset[evtype];
		overflow_flag_bit = phytium_ddr_pmu_v3_genc_cnt_overflow_bit[evtype];

		if (evtype == DDR_PMUV3_MC_BANK_SRC_EVTYPE)
			sel_mask &= (evid << 5);
		else if (evtype == DDR_PMUV3_MC_T_CMD_SCHE_EVTYPE)
			sel_mask &= (evid << 4);
		else if (evtype == DDR_PMUV3_MC_T_RDWR_SWTICH_EVTYPE)
			sel_mask &= (evid << 4);
		else
			sel_mask &= evid;

		writel(sel_mask, ddr_pmu->base + sel_offset);

		overflow_mask = readl(ddr_pmu->base + DDR_PMUV3_CNT_OVERFLOW_FLAG);
		if (overflow_flag_bit > 0)
			overflow_flag = (u32)(overflow_mask & (1 << overflow_flag_bit));

		break;
	}

	if (cntsize == 64) {
		val32_l = readl(ddr_pmu->base + cnt_offset);
		val32_h = readl(ddr_pmu->base + cnt_offset + 4);
		val64 = (u64)val32_h << 32 | (u64)val32_l;
	} else {
		val32_l = readl(ddr_pmu->base + cnt_offset);
		val64 = (u64)val32_l;
	}

	if (overflow_flag == 1) {
		dev_warn(ddr_pmu->dev,
				"The Event(type=%u,eventid=%u) counter value has overflowed.\n",
				evtype, evid);
		return 0;
	}
	return val64;
}

static void phytium_ddr_pmu_v1_enable_clk(struct phytium_ddr_pmu *ddr_pmu)
{
	u32 val;

	if (ddr_pmu->ver == DDR_PMUV1P5)
		return;

	val = readl(ddr_pmu->cfg_base);
	val |= 0xF;
	writel(val, ddr_pmu->cfg_base);
}

static void phytium_ddr_pmu_v3_enable_clk(struct phytium_ddr_pmu *ddr_pmu)
{
	writel(0x0, ddr_pmu->base + DDR_PMUV3_CLK_EN);
}

static void phytium_ddr_pmu_v1_disable_clk(struct phytium_ddr_pmu *ddr_pmu)
{
	u32 val;

	if (ddr_pmu->ver == DDR_PMUV1P5)
		return;

	val = readl(ddr_pmu->cfg_base);
	val &= ~(0xF);
	writel(val, ddr_pmu->cfg_base);
}

static void phytium_ddr_pmu_v3_disable_clk(struct phytium_ddr_pmu *ddr_pmu)
{
	writel(0x1, ddr_pmu->base + DDR_PMUV3_CLK_EN);
}

static void phytium_ddr_pmu_v3_snapshot_counters(struct phytium_ddr_pmu *ddr_pmu)
{
	writel(0x0, ddr_pmu->base + DDR_PMUV3_SNAPSHOT_PMU);
	writel(BIT(0), ddr_pmu->base + DDR_PMUV3_SNAPSHOT_PMU);
}

static void phytium_ddr_pmu_v1_clear_all_counters(struct phytium_ddr_pmu *ddr_pmu)
{
	writel(0x1, ddr_pmu->base + DDR_PMUV1_CLEAR_EVENT);
}

static void phytium_ddr_pmu_v2_clear_all_counters(struct phytium_ddr_pmu *ddr_pmu)
{
	writel(0x1, ddr_pmu->base + DDR_PMUV2_CLEAR_EVENT);
}

static void phytium_ddr_pmu_v3_clear_all_counters(struct phytium_ddr_pmu *ddr_pmu)
{
	writel(0x1, ddr_pmu->base + DDR_PMUV3_CLEAR_TIMER);
	writel(0x0, ddr_pmu->base + DDR_PMUV3_CLEAR_TIMER);
}

static void phytium_ddr_pmu_v1_start_all_counters(struct phytium_ddr_pmu *ddr_pmu)
{
	writel(0x1, ddr_pmu->base + DDR_PMUV1_START_TIMER);
}

static void phytium_ddr_pmu_v2_start_all_counters(struct phytium_ddr_pmu *ddr_pmu)
{
	writel(BIT(0), ddr_pmu->base + DDR_PMUV2_TIMER_START);
}

static void phytium_ddr_pmu_v3_start_all_counters(struct phytium_ddr_pmu *ddr_pmu)
{
	writel(BIT(0), ddr_pmu->base + DDR_PMUV3_START_TIMER);
}

static void phytium_ddr_pmu_v1_stop_all_counters(struct phytium_ddr_pmu *ddr_pmu)
{
	writel(0x1, ddr_pmu->base + DDR_PMUV1_STOP_TIMER);
}

static void phytium_ddr_pmu_v2_stop_all_counters(struct phytium_ddr_pmu *ddr_pmu)
{
	writel(BIT(0), ddr_pmu->base + DDR_PMUV2_TIMER_STOP);
}

static void phytium_ddr_pmu_v3_stop_all_counters(struct phytium_ddr_pmu *ddr_pmu)
{
	writel(0x0, ddr_pmu->base + DDR_PMUV3_START_TIMER);
}

static void phytium_ddr_pmu_v2_reset_timer(struct phytium_ddr_pmu *ddr_pmu)
{
	writel(0xFFFFFFFF, ddr_pmu->base + DDR_PMUV2_SET_TIMER_L);
	writel(0xFFFFFFFF, ddr_pmu->base + DDR_PMUV2_SET_TIMER_H);
}

static unsigned long
phytium_ddr_pmu_v1_get_stop_state(struct phytium_ddr_pmu *ddr_pmu)
{
	unsigned long val;

	val = (unsigned long)readl(ddr_pmu->base + DDR_PMUV1_STATE_STOP);
	return val;
}

static unsigned long
phytium_ddr_pmu_v1_get_irq_flag(struct phytium_ddr_pmu *ddr_pmu)
{
	unsigned long val;

	val = (unsigned long)readl(ddr_pmu->irq_reg);
	return val;
}

static void phytium_ddr_pmu_v2_enable_events(struct phytium_ddr_pmu *ddr_pmu, int idx)
{
	u8 en_bit;
	u32 en_offset, irq_offset, val;

	if (idx == 0) {
		en_bit = 0;
		en_offset = 0;
		irq_offset = DDR_PMUV2_TIMER_INT_MASK;
	} else {
		en_bit = (idx - 1) * 8;
		en_offset = DDR_PMUV2_AXI_MONITOR_EN;
		irq_offset = DDR_PMUV2_AXI_MONITOR_INT_MASK;
		}

	if (en_offset) {
		val = readl(ddr_pmu->base + en_offset);
		val |= BIT(en_bit);
		writel(val, ddr_pmu->base + en_offset);
	}

	val = readl(ddr_pmu->base + irq_offset);
	val &= ~BIT(en_bit);
	writel(val, ddr_pmu->base + irq_offset);
}

static int phytium_ddr_pmu_v3_get_event_idx(struct perf_event *event)
{
	int i, idx;
	u32 event_type, event_id;

	event_type = get_type(event);
	event_id = get_eventid(event);
	idx = 0;

	for (i = 0; i < event_type; i++)
		idx += phytium_ddr_pmu_v3_event_num[i];

	if (event_type < DDR_PMUV3_LPR_FIFO_OCCU_EVTYPE || event_type == DDR_PMUV3_SPECIAL_EVTYPE)
		idx += event_id;
	else
		idx += (event_id % 3);

	return idx;
}

static int phytium_ddr_pmu_mark_event(struct perf_event *event)
{
	struct phytium_ddr_pmu *ddr_pmu = to_phytium_ddr_pmu(event->pmu);
	unsigned long *used_mask = ddr_pmu->pmu_events.used_mask;
	struct hw_perf_event *hwc = &event->hw;
	int idx;

	if (ddr_pmu->ver == DDR_PMUV3P0)
		idx = phytium_ddr_pmu_v3_get_event_idx(event);
	else
		idx = GET_DDR_PMUV1V2_EVENTID(hwc);

	if (test_bit(idx, used_mask))
		return -EAGAIN;

	set_bit(idx, used_mask);

	return idx;
}

static void phytium_ddr_pmu_unmark_event(struct perf_event *event)
{
	struct phytium_ddr_pmu *ddr_pmu = to_phytium_ddr_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	u32 idx = GET_DDR_PMUV1V2_EVENTID(hwc);

	if (ddr_pmu->ver <= DDR_PMUV1P5) {
		if (!EVENT_VALID_V1(idx)) {
			dev_err(ddr_pmu->dev, "Unsupported event index:%d!\n", idx);
			return;
		}
	} else if (ddr_pmu->ver == DDR_PMUV2P0) {
		if (!EVENT_VALID_V2(idx)) {
			dev_err(ddr_pmu->dev, "Unsupported event index:%d!\n", idx);
			return;
		}
	} else if (ddr_pmu->ver == DDR_PMUV3P0) {
		if (!EVENT_VALID_V3(phytium_ddr_pmu_v3_get_event_idx(event))) {
			dev_err(ddr_pmu->dev, "Unsupported event index:%d!\n", idx);
			return;
		}

		int idx, i;

		u32 event_type, event_id;

		event_type = get_type(event);
		event_id = get_eventid(event);
		idx = 0;

		for (i = 0; i < event_type; i++)
			idx += phytium_ddr_pmu_v3_event_num[i];

		if (event_type < DDR_PMUV3_LPR_FIFO_OCCU_EVTYPE ||
			event_type == DDR_PMUV3_SPECIAL_EVTYPE)
			idx += event_id;
		else
			idx += (event_id % 3);
	}

	clear_bit(idx, ddr_pmu->pmu_events.used_mask);
	ddr_pmu->pmu_events.hw_events[hwc->idx] = NULL;
}

static int phytium_ddr_pmu_v3_verify_port_occu_threshold(u32 low_thre, u32 hight_thre,
						u32 *in_low_thre, u32 *in_hight_thre)
{
	if (((*in_low_thre) == 0) || ((*in_hight_thre) == 0)) {
		*in_low_thre = low_thre;
		*in_hight_thre = hight_thre;
	} else if (((*in_low_thre) != low_thre) || ((*in_hight_thre) != hight_thre)) {
		*in_low_thre = 0;
		*in_hight_thre = 0;
		return -EINVAL;
	}
	return 0;
}

static int phytium_ddr_pmu_v3_get_port_occu_threshold(struct phytium_ddr_pmu *ddr_pmu,
						struct perf_event *event)
{
	u32 low_thre, hight_thre, evtype;
	u32 thre_reg_idx, thre_max_val;
	int ret;

	low_thre = get_low_thre(event);
	hight_thre = get_hight_thre(event);
	evtype = get_type(event);

	thre_reg_idx = evtype - DDR_PMUV3_LPR_FIFO_OCCU_EVTYPE;
	thre_max_val = (1 << phytium_ddr_pmu_v3_thre_bit_size[thre_reg_idx]) - 1;

	if ((hight_thre <= low_thre) || thre_max_val < low_thre)
		return -EINVAL;
	switch (evtype) {
	case DDR_PMUV3_LPR_FIFO_OCCU_EVTYPE:
		ret = phytium_ddr_pmu_v3_verify_port_occu_threshold(low_thre, hight_thre,
			&ddr_pmu->port_occu_thre.lpr_fifo_low_thre,
			&ddr_pmu->port_occu_thre.lpr_fifo_hight_thre);
		break;
	case DDR_PMUV3_HPR_FIFO_OCCU_EVTYPE:
		ret = phytium_ddr_pmu_v3_verify_port_occu_threshold(low_thre, hight_thre,
			&ddr_pmu->port_occu_thre.hpr_fifo_low_thre,
			&ddr_pmu->port_occu_thre.hpr_fifo_hight_thre);
		break;
	case DDR_PMUV3_WR_DCQ_OCCU_EVTYPE:
		ret = phytium_ddr_pmu_v3_verify_port_occu_threshold(low_thre, hight_thre,
			&ddr_pmu->port_occu_thre.wr_dcq_low_thre,
			&ddr_pmu->port_occu_thre.wr_dcq_hight_thre);
		break;
	case DDR_PMUV3_RDAT_INFO_OCCU_EVTYPE:
		ret = phytium_ddr_pmu_v3_verify_port_occu_threshold(low_thre, hight_thre,
			&ddr_pmu->port_occu_thre.rdat_info_low_thre,
			&ddr_pmu->port_occu_thre.rdat_info_hight_thre);
		break;
	case DDR_PMUV3_RDAT_FIFO_OCCU_EVTYPE:
		ret = phytium_ddr_pmu_v3_verify_port_occu_threshold(low_thre, hight_thre,
			&ddr_pmu->port_occu_thre.rdat_fifo_low_thre,
			&ddr_pmu->port_occu_thre.rdat_fifo_hight_thre);
		break;
	case DDR_PMUV3_WDP_BUFFER_OCCU_EVTYPE:
		ret = phytium_ddr_pmu_v3_verify_port_occu_threshold(low_thre, hight_thre,
			&ddr_pmu->port_occu_thre.wdp_buffer_low_thre,
			&ddr_pmu->port_occu_thre.wdp_buffer_hight_thre);
		break;
	case DDR_PMUV3_RSP_COMP_OCCU_EVTYPE:
		ret = phytium_ddr_pmu_v3_verify_port_occu_threshold(low_thre, hight_thre,
			&ddr_pmu->port_occu_thre.rsp_comp_low_thre,
			&ddr_pmu->port_occu_thre.rsp_comp_hight_thre);
		break;
	case DDR_PMUV3_RSP_DBID_OCCU_EVTYPE:
		ret = phytium_ddr_pmu_v3_verify_port_occu_threshold(low_thre, hight_thre,
			&ddr_pmu->port_occu_thre.rsp_dbid_low_thre,
			&ddr_pmu->port_occu_thre.rsp_dbid_hight_thre);
		break;
	case DDR_PMUV3_RSP_CRQ_OCCU_EVTYPE:
		ret = phytium_ddr_pmu_v3_verify_port_occu_threshold(low_thre, hight_thre,
			&ddr_pmu->port_occu_thre.rsp_crq_low_thre,
			&ddr_pmu->port_occu_thre.rsp_crq_hight_thre);
		break;
	case DDR_PMUV3_RSP_REQ_OCCU_EVTYPE:
		ret = phytium_ddr_pmu_v3_verify_port_occu_threshold(low_thre, hight_thre,
			&ddr_pmu->port_occu_thre.rsp_req_low_thre,
			&ddr_pmu->port_occu_thre.rsp_req_hight_thre);
		break;
	case DDR_PMUV3_RSP_RTQ_OCCU_EVTYPE:
		ret = phytium_ddr_pmu_v3_verify_port_occu_threshold(low_thre, hight_thre,
			&ddr_pmu->port_occu_thre.rsp_rtq_low_thre,
			&ddr_pmu->port_occu_thre.rsp_rtq_hight_thre);
		break;
	default:
		ret = 0;
		break;
	}

	return ret;
}

static void phytium_ddr_pmu_v3_set_port_occu_threshold(struct phytium_ddr_pmu *ddr_pmu,
		u32 evtype, u32 low_thre, u32 hight_thre)
{
	u32 thre_reg_offset, thre_reg_idx, thre_bit_size;
	u32 val;

	thre_reg_idx = evtype - DDR_PMUV3_LPR_FIFO_OCCU_EVTYPE;
	thre_reg_offset = phytium_ddr_pmu_v3_thre_reg_offset[thre_reg_idx];
	thre_bit_size = phytium_ddr_pmu_v3_thre_bit_size[thre_reg_idx];

	val = low_thre | (hight_thre << thre_bit_size);

	writel(val, ddr_pmu->base + thre_reg_offset);
}

static void phytium_ddr_pmu_v3_reset_port_occu_threshold(struct phytium_ddr_pmu *ddr_pmu)
{
	u32 thre_reg_offset, thre_bit_size;
	u32 val;
	int i = 0;

	for (i = 0; i < 11; i++) {
		thre_reg_offset = phytium_ddr_pmu_v3_thre_reg_offset[i];
		thre_bit_size = phytium_ddr_pmu_v3_thre_bit_size[i];

		val = readl(ddr_pmu->base + thre_reg_offset);
		val &= ~((1 << thre_bit_size) - 1);
		writel(val, ddr_pmu->base + thre_reg_offset);
	}

	ddr_pmu->port_occu_thre.lpr_fifo_low_thre = 0;
	ddr_pmu->port_occu_thre.lpr_fifo_hight_thre = 0;
	ddr_pmu->port_occu_thre.hpr_fifo_low_thre = 0;
	ddr_pmu->port_occu_thre.hpr_fifo_hight_thre = 0;
	ddr_pmu->port_occu_thre.wr_dcq_low_thre = 0;
	ddr_pmu->port_occu_thre.wr_dcq_hight_thre = 0;

	ddr_pmu->port_occu_thre.rdat_info_low_thre = 0;
	ddr_pmu->port_occu_thre.rdat_info_hight_thre = 0;
	ddr_pmu->port_occu_thre.rdat_fifo_low_thre = 0;
	ddr_pmu->port_occu_thre.rdat_fifo_hight_thre = 0;
	ddr_pmu->port_occu_thre.wdp_buffer_low_thre = 0;
	ddr_pmu->port_occu_thre.wdp_buffer_hight_thre = 0;

	ddr_pmu->port_occu_thre.rsp_comp_low_thre = 0;
	ddr_pmu->port_occu_thre.rsp_comp_hight_thre = 0;

	ddr_pmu->port_occu_thre.rsp_dbid_low_thre = 0;
	ddr_pmu->port_occu_thre.rsp_dbid_hight_thre = 0;
	ddr_pmu->port_occu_thre.rsp_crq_low_thre = 0;
	ddr_pmu->port_occu_thre.rsp_crq_hight_thre = 0;
	ddr_pmu->port_occu_thre.rsp_req_low_thre = 0;
	ddr_pmu->port_occu_thre.rsp_req_hight_thre = 0;
	ddr_pmu->port_occu_thre.rsp_rtq_low_thre = 0;
	ddr_pmu->port_occu_thre.rsp_rtq_hight_thre = 0;
}

int phytium_ddr_pmu_event_init(struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;
	struct phytium_ddr_pmu *ddr_pmu;

	if (event->attr.type != event->pmu->type)
		return -ENOENT;

	if (is_sampling_event(event) || event->attach_state & PERF_ATTACH_TASK)
		return -EOPNOTSUPP;

	ddr_pmu = to_phytium_ddr_pmu(event->pmu);

	if (event->cpu < 0) {
		dev_warn(ddr_pmu->dev, "Can't provide per-task data!\n");
		return -EINVAL;
	}

	if (ddr_pmu->ver <= DDR_PMUV2P0) {
		if ((event->attr.config & PHYTIUM_DDR_PMUV1V2_EVENTS_MAX_MASK) > ddr_pmu->cnts_num)
			return -EINVAL;
	} else if (ddr_pmu->ver == DDR_PMUV3P0) {
		if ((event->attr.config & PHYTIUM_DDR_PMUV3_EVENTS_MAX_MASK) > ddr_pmu->cnts_num)
			return -EINVAL;
	}

	if (ddr_pmu->on_cpu == -1)
		return -EINVAL;

	hwc->idx = -1;
	hwc->config_base = event->attr.config;

	event->cpu = ddr_pmu->on_cpu;

	if (ddr_pmu->ver == DDR_PMUV2P0)
		used_event_v2 = 0;
	else if (ddr_pmu->ver == DDR_PMUV3P0) {
		int ret;
		u32 event_type, occu_flag;
		u32 low_thre, hight_thre;
		int idx;
		struct attribute *attr;
		struct device_attribute *dev_attr;
		struct phytium_ddr_pmu_v3_event_attr *eattr;

		event_type = get_type(event);
		low_thre = get_low_thre(event);
		hight_thre = get_hight_thre(event);
		idx = phytium_ddr_pmu_v3_get_event_idx(event);

		attr = phytium_ddr_pmu_v3_events_attr[idx];
		dev_attr = container_of(attr, struct device_attribute, attr);
		eattr = container_of(dev_attr, struct phytium_ddr_pmu_v3_event_attr, attr);
		occu_flag = eattr->occuflag;

		if (phytium_ddr_pmu_v3_is_port_occup_event(event_type, occu_flag)) {
			ret = phytium_ddr_pmu_v3_get_port_occu_threshold(ddr_pmu, event);
			if (ret < 0)
				return ret;
			phytium_ddr_pmu_v3_set_port_occu_threshold(ddr_pmu, event_type, low_thre,
								hight_thre);
		}
	}

	return 0;
}

void phytium_ddr_pmu_event_update(struct perf_event *event)
{
	struct phytium_ddr_pmu *ddr_pmu = to_phytium_ddr_pmu(event->pmu);
	u64 delta;

	if (ddr_pmu->ver == DDR_PMUV2P0)
		ddr_pmu->ops->stop_all_counters(ddr_pmu);

	delta = ddr_pmu->ops->read_counter(ddr_pmu, event);
	local64_add(delta, &event->count);
}

void phytium_ddr_pmu_event_start(struct perf_event *event, int flags)
{
	struct hw_perf_event *hwc = &event->hw;

	hwc->state = 0;
	perf_event_update_userpage(event);
}

void phytium_ddr_pmu_event_stop(struct perf_event *event, int flags)
{
	struct hw_perf_event *hwc = &event->hw;

	hwc->state |= PERF_HES_STOPPED;

	if (flags & PERF_EF_UPDATE)
		phytium_ddr_pmu_event_update(event);
}

int phytium_ddr_pmu_event_add(struct perf_event *event, int flags)
{
	struct phytium_ddr_pmu *ddr_pmu = to_phytium_ddr_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	int idx;

	hwc->state |= PERF_HES_STOPPED;

	idx = phytium_ddr_pmu_mark_event(event);
	if (ddr_pmu->ver <= DDR_PMUV1P5) {
		if (!EVENT_VALID_V1(idx)) {
			dev_err(ddr_pmu->dev, "Unsupported event index:%d!\n", idx);
			return idx;
		}
	} else if (ddr_pmu->ver == DDR_PMUV2P0) {
		if (!EVENT_VALID_V2(idx)) {
			dev_err(ddr_pmu->dev, "Unsupported event index:%d!\n", idx);
			return idx;
		}
		#if IS_ENABLED(CONFIG_ARM_PHYTIUM_DMU_DEVFREQ)
			phytium_ddr_pmu_v2_notifier_chain_trigger(ddr_pmu, DDR_PMUV2_NOTICE_START);
		#endif
	} else if (ddr_pmu->ver == DDR_PMUV3P0) {
		if (!EVENT_VALID_V3(phytium_ddr_pmu_v3_get_event_idx(event))) {
			dev_err(ddr_pmu->dev, "Unsupported event index:%d!\n", idx);
			return idx;
		}
	}
	event->hw.idx = idx;
	ddr_pmu->pmu_events.hw_events[idx] = event;

	if (ddr_pmu->ver == DDR_PMUV2P0) {
		phytium_ddr_pmu_v2_enable_events(ddr_pmu, idx);
		used_event_v2 += 1;
	}
	return 0;
}

void phytium_ddr_pmu_event_del(struct perf_event *event, int flags)
{
	struct phytium_ddr_pmu *ddr_pmu = to_phytium_ddr_pmu(event->pmu);

	if (ddr_pmu->ver == DDR_PMUV2P0)
		used_event_v2 -= 1;

	phytium_ddr_pmu_event_stop(event, PERF_EF_UPDATE);

	if (ddr_pmu->ver == DDR_PMUV3P0)
		phytium_ddr_pmu_v3_reset_port_occu_threshold(ddr_pmu);

	phytium_ddr_pmu_unmark_event(event);

	perf_event_update_userpage(event);

	if (ddr_pmu->ver == DDR_PMUV2P0) {
#if IS_ENABLED(CONFIG_ARM_PHYTIUM_DMU_DEVFREQ)
		if (used_event_v2 == 0)
			phytium_ddr_pmu_v2_notifier_chain_trigger(ddr_pmu, DDR_PMUV2_NOTICE_STOP);
#endif
	}
}

void phytium_ddr_pmu_v1_enable_counters(struct phytium_ddr_pmu *ddr_pmu)
{
	ddr_pmu->ops->clear_all_counters(ddr_pmu);
	ddr_pmu->ops->start_all_counters(ddr_pmu);
}

void phytium_ddr_pmu_v2_enable_counters(struct phytium_ddr_pmu *ddr_pmu)
{
	ddr_pmu->ops->stop_all_counters(ddr_pmu);
	ddr_pmu->ops->clear_all_counters(ddr_pmu);
	phytium_ddr_pmu_v2_reset_timer(ddr_pmu);
	ddr_pmu->ops->start_all_counters(ddr_pmu);
}

void phytium_ddr_pmu_v3_enable_counters(struct phytium_ddr_pmu *ddr_pmu)
{
	ddr_pmu->ops->enable_clk(ddr_pmu);
	ddr_pmu->ops->clear_all_counters(ddr_pmu);
	ddr_pmu->ops->start_all_counters(ddr_pmu);
}

void phytium_ddr_pmu_enable(struct pmu *pmu)
{
	struct phytium_ddr_pmu *ddr_pmu = to_phytium_ddr_pmu(pmu);
	int event_added = bitmap_weight(ddr_pmu->pmu_events.used_mask,
					ddr_pmu->cnts_num);

	if (event_added) {
		ddr_pmu->ops->enable_counters(ddr_pmu);
	}
}

static void phytium_ddr_pmu_v2_mask_all_irq(struct phytium_ddr_pmu *ddr_pmu)
{
	writel(DDR_PMUV2_TIMER_OPT_BIT, ddr_pmu->base + DDR_PMUV2_TIMER_INT_MASK);
	writel(DDR_PMUV2_AXI_MONITOR_OPT_BIT, ddr_pmu->base + DDR_PMUV2_AXI_MONITOR_INT_MASK);
}

static void phytium_ddr_pmu_v2_disable_axi_cmd_events(struct phytium_ddr_pmu *ddr_pmu)
{
	writel(0x0, ddr_pmu->base + DDR_PMUV2_AXI_MONITOR_EN);
}

void phytium_ddr_pmu_v1_disable_counters(struct phytium_ddr_pmu *ddr_pmu)
{
	ddr_pmu->ops->stop_all_counters(ddr_pmu);
}

void phytium_ddr_pmu_v2_disable_counters(struct phytium_ddr_pmu *ddr_pmu)
{
	if (ddr_pmu->used_flag) {
		phytium_ddr_pmu_v2_mask_all_irq(ddr_pmu);
		phytium_ddr_pmu_v2_disable_axi_cmd_events(ddr_pmu);
	}
}

void phytium_ddr_pmu_v3_disable_counters(struct phytium_ddr_pmu *ddr_pmu)
{
	ddr_pmu->ops->stop_all_counters(ddr_pmu);
	phytium_ddr_pmu_v3_snapshot_counters(ddr_pmu);
}

void phytium_ddr_pmu_disable(struct pmu *pmu)
{
	struct phytium_ddr_pmu *ddr_pmu = to_phytium_ddr_pmu(pmu);
	int event_added = bitmap_weight(ddr_pmu->pmu_events.used_mask,
					ddr_pmu->cnts_num);

	if (event_added)
		ddr_pmu->ops->disable_counters(ddr_pmu);
}

void phytium_ddr_pmu_v1_reset(struct phytium_ddr_pmu *ddr_pmu)
{
	ddr_pmu->ops->disable_clk(ddr_pmu);
	ddr_pmu->ops->clear_all_counters(ddr_pmu);
}

static const struct acpi_device_id phytium_ddr_pmu_acpi_match[] = {
	{ "PHYT0043", },
	{ "PHYT0067", },
	{ "PHYT0069", },
	{},
};
MODULE_DEVICE_TABLE(acpi, phytium_ddr_pmu_acpi_match);

static irqreturn_t phytium_ddr_pmu_v1_overflow_handler(int irq, void *dev_id)
{
	struct phytium_ddr_pmu *ddr_pmu = dev_id;
	struct perf_event *event;
	unsigned long overflown, stop_state;
	unsigned long *used_mask = ddr_pmu->pmu_events.used_mask;
	int idx;
	int event_added = bitmap_weight(used_mask, ddr_pmu->cnts_num);

	overflown = phytium_ddr_pmu_v1_get_irq_flag(ddr_pmu);

	if (!test_bit(ddr_pmu->irq_bit, &overflown))
		return IRQ_NONE;

	stop_state = phytium_ddr_pmu_v1_get_stop_state(ddr_pmu);

	if (bitmap_weight(&stop_state, 6)) {
		for_each_set_bit(idx, used_mask, ddr_pmu->cnts_num) {
			event = ddr_pmu->pmu_events.hw_events[idx];
			if (!event)
				continue;
			phytium_ddr_pmu_event_update(event);
		}

		ddr_pmu->ops->clear_all_counters(ddr_pmu);
		if ((stop_state & DDR_PMUV1_OFL_STOP_TYPE_VAL) == 0)
			ddr_pmu->ops->start_all_counters(ddr_pmu);

		return IRQ_HANDLED;
	}

	if (!event_added) {
		ddr_pmu->ops->clear_all_counters(ddr_pmu);
		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static irqreturn_t phytium_ddr_pmu_v2_overflow_handler(int irq, void *dev_id)
{
	struct phytium_ddr_pmu *ddr_pmu = dev_id;
	struct perf_event *event;
	int idx;
	unsigned long *used_mask = ddr_pmu->pmu_events.used_mask;
	u32 timer_int_sta, axi_int_sta;

	timer_int_sta = readl(ddr_pmu->base + DDR_PMUV2_TIMER_INT_STA);
	axi_int_sta = readl(ddr_pmu->base + DDR_PMUV2_AXI_MONITOR_INT_STA);

	if ((timer_int_sta + axi_int_sta) == 0)
		return IRQ_NONE;

	if (timer_int_sta)
		writel(0x1, ddr_pmu->base + DDR_PMUV2_TIMER_INT_CLEAR);

	if (axi_int_sta)
		writel(axi_int_sta, ddr_pmu->base + ddr_pmu->cnts_num);

	if (!ddr_pmu->used_flag) {
		phytium_ddr_pmu_v2_mask_all_irq(ddr_pmu);
		return IRQ_HANDLED;
	}

	for_each_set_bit(idx, used_mask, PHYTIUM_DDR_PMUV2_COUNTERS_NUM) {
		event = ddr_pmu->pmu_events.hw_events[idx];
		if (!event)
			continue;
		phytium_ddr_pmu_event_update(event);
	}
	writel(DDR_PMUV2_ALL_EVENT_CLEAR_BIT, ddr_pmu->base + DDR_PMUV2_CLEAR_EVENT);
	ddr_pmu->ops->start_all_counters(ddr_pmu);

	return IRQ_HANDLED;
}

static const struct phytium_ddr_pmu_ops phytium_ddr_pmu_v1_ops = {
	.overflow_handler = phytium_ddr_pmu_v1_overflow_handler,
	.read_counter = phytium_ddr_pmu_v1_read_counter,
	.clear_all_counters = phytium_ddr_pmu_v1_clear_all_counters,
	.start_all_counters = phytium_ddr_pmu_v1_start_all_counters,
	.stop_all_counters = phytium_ddr_pmu_v1_stop_all_counters,
	.enable_counters = phytium_ddr_pmu_v1_enable_counters,
	.disable_counters = phytium_ddr_pmu_v1_disable_counters,
	.enable_clk = phytium_ddr_pmu_v1_enable_clk,
	.disable_clk = phytium_ddr_pmu_v1_disable_clk,
};

static const struct phytium_ddr_pmu_ops phytium_ddr_pmu_v2_ops = {
	.overflow_handler = phytium_ddr_pmu_v2_overflow_handler,
	.read_counter = phytium_ddr_pmu_v2_read_counter,
	.clear_all_counters = phytium_ddr_pmu_v2_clear_all_counters,
	.start_all_counters = phytium_ddr_pmu_v2_start_all_counters,
	.stop_all_counters = phytium_ddr_pmu_v2_stop_all_counters,
	.enable_counters = phytium_ddr_pmu_v2_enable_counters,
	.disable_counters = phytium_ddr_pmu_v2_disable_counters,
};

static const struct phytium_ddr_pmu_ops phytium_ddr_pmu_v3_ops = {
	.read_counter = phytium_ddr_pmu_v3_read_counter,
	.clear_all_counters = phytium_ddr_pmu_v3_clear_all_counters,
	.start_all_counters = phytium_ddr_pmu_v3_start_all_counters,
	.stop_all_counters = phytium_ddr_pmu_v3_stop_all_counters,
	.enable_clk = phytium_ddr_pmu_v3_enable_clk,
	.disable_clk = phytium_ddr_pmu_v3_disable_clk,
	.enable_counters = phytium_ddr_pmu_v3_enable_counters,
	.disable_counters = phytium_ddr_pmu_v3_disable_counters,
};

static int phytium_ddr_pmu_v1_verify_pbf_version(struct platform_device *pdev)
{
	struct arm_smccc_res res;
	unsigned long major_ver, minor_ver;

	arm_smccc_smc(PBFVER_FUNC_ID, 0, 0, 0, 0, 0, 0, 0, &res);
	if (res.a0 <= 0) {
		dev_warn(&pdev->dev, "Can not recognize PBF Firmware version!\n");
		return -EINVAL;
	}

	minor_ver = res.a0 & 0xFFFF;
	major_ver = (res.a0 >> 16) & 0xFFFF;

	if (major_ver < 1 || (major_ver == 1 && minor_ver < 20)) {
		dev_err(&pdev->dev,
			"Driver load failed, Please upgrade PBF Firmware version to 1.20 or later!\n");
		return -EINVAL;
	}

	return 0;

}

static int phytium_ddr_pmu_version(struct platform_device *pdev,
		struct phytium_ddr_pmu *ddr_pmu)
{
	struct acpi_device *acpi_dev;

	acpi_dev = ACPI_COMPANION(&pdev->dev);
	if (!strcmp(acpi_device_hid(acpi_dev), "PHYT0043")) {
		ddr_pmu->ver = DDR_PMUV1P0;
	} else if (!strcmp(acpi_device_hid(acpi_dev), "PHYT0067")) {
		ddr_pmu->ver = DDR_PMUV1P5;
	} else if (!strcmp(acpi_device_hid(acpi_dev), "PHYT0069")) {
		ddr_pmu->ver = DDR_PMUV2P0;
	#if !IS_ENABLED(CONFIG_PHYT_DMU_PMU_PD2408)
		dev_err(&pdev->dev, "CONFIG_PHYT_DMU_PMU_PD2408 not enabled\n");
		return -ENODEV;
	#endif
	} else if (!strcmp(acpi_device_hid(acpi_dev), "PHYT3003")) {
		ddr_pmu->ver = DDR_PMUV3P0;
	} else {
		dev_err(&pdev->dev, "The current driver does not support this device.\n");
		return -ENODEV;
	}

	return 0;
}

static int phytium_ddr_pmu_init_irq(struct phytium_ddr_pmu *ddr_pmu,
				       struct platform_device *pdev)
{
	int irq, ret;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	ret = devm_request_irq(&pdev->dev, irq,
				ddr_pmu->ops->overflow_handler,
				IRQF_NOBALANCING | IRQF_NO_THREAD | IRQF_SHARED,
				dev_name(&pdev->dev), ddr_pmu);

	if (ret < 0) {
		dev_err(&pdev->dev, "Fail to request IRQ:%d ret:%d\n", irq,
			ret);
		return ret;
	}

	ddr_pmu->irq = irq;

	return 0;
}

static int phytium_ddr_pmu_v1_init_data(struct platform_device *pdev,
					struct phytium_ddr_pmu *ddr_pmu)
{
	struct resource *res, *clkres, *irqres;

	if (device_property_read_u32(&pdev->dev, "phytium,die-id",
					&ddr_pmu->die_id)) {
		dev_err(&pdev->dev, "Can not read phytium,die-id!\n");
		return -EINVAL;
	}

	if (device_property_read_u32(&pdev->dev, "phytium,ddr-id",
						&ddr_pmu->ddr_id)) {
		dev_err(&pdev->dev, "Can not read phytium,ddr-id!\n");
		return -EINVAL;
	}

	if (device_property_read_u32(&pdev->dev, "phytium,pmu-id",
					&ddr_pmu->pmu_id)) {
		dev_err(&pdev->dev, "Can not read ddr pmu-id!\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ddr_pmu->base = devm_ioremap_resource(&pdev->dev, res);

	if (IS_ERR(ddr_pmu->base)) {
		dev_err(&pdev->dev,
			"ioremap failed for ddr_pmu base resource\n");
		return PTR_ERR(ddr_pmu->base);
	}

	ddr_pmu->irq_bit = ddr_pmu->ddr_id * 2 + ddr_pmu->pmu_id;

	clkres = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!clkres) {
		dev_err(&pdev->dev, "failed for get ddr_pmu clk resource.\n");
		return -EINVAL;
	}
	ddr_pmu->cfg_base = devm_ioremap(&pdev->dev, clkres->start,
					resource_size(clkres));
	if (IS_ERR(ddr_pmu->cfg_base)) {
		dev_err(&pdev->dev, "ioremap failed for ddr_pmu clk resource\n");
		return PTR_ERR(ddr_pmu->cfg_base);
	}

	if (ddr_pmu->ver == DDR_PMUV1P5) {
		irqres = platform_get_resource(pdev, IORESOURCE_MEM, 2);
		if (!irqres) {
			dev_err(&pdev->dev, "failed for get ddr_pmu irq resource.\n");
			return -EINVAL;
		}
		ddr_pmu->irq_reg = devm_ioremap(&pdev->dev, irqres->start,
					resource_size(irqres));
		if (IS_ERR(ddr_pmu->irq_reg)) {
			dev_err(&pdev->dev, "ioremap failed for ddr_pmu irq resource\n");
			return PTR_ERR(ddr_pmu->irq_reg);
		}
	} else {
		ddr_pmu->irq_reg = ddr_pmu->cfg_base + 0x4;
	}

	ddr_pmu->cnts_num = PHYTIUM_DDR_PMUV1_COUNTERS_NUM;
	ddr_pmu->ops = &phytium_ddr_pmu_v1_ops;

	phytium_ddr_pmu_v1_reset(ddr_pmu);

	return 0;
}

static int phytium_ddr_pmu_v2_init_data(struct platform_device *pdev,
					struct phytium_ddr_pmu *ddr_pmu)
{
	struct resource *res;

	if (device_property_read_u32(&pdev->dev, "phytium,ddr-id",
						&ddr_pmu->ddr_id)) {
		dev_err(&pdev->dev, "Can not read phytium,ddr-id!\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ddr_pmu->base = devm_ioremap_resource(&pdev->dev, res);

	if (IS_ERR(ddr_pmu->base)) {
		dev_err(&pdev->dev,
			"ioremap failed for ddr_pmu base resource\n");
		return PTR_ERR(ddr_pmu->base);
	}

	ddr_pmu->used_flag = true;
	phytium_ddr_pmu_v2_mask_all_irq(ddr_pmu);

	ddr_pmu->cnts_num = PHYTIUM_DDR_PMUV2_COUNTERS_NUM;
	ddr_pmu->ops = &phytium_ddr_pmu_v2_ops;

	return 0;
}

static int phytium_ddr_pmu_v3_init_data(struct platform_device *pdev,
					struct phytium_ddr_pmu *ddr_pmu)
{
	struct resource *res;

	if (device_property_read_u32(&pdev->dev, "phytium,die-id",
					&ddr_pmu->die_id)) {
		dev_err(&pdev->dev, "Can not read phytium,die-id!\n");
		return -EINVAL;
	}

	if (device_property_read_u32(&pdev->dev, "phytium,ddr-id",
						&ddr_pmu->ddr_id)) {
		dev_err(&pdev->dev, "Can not read phytium,ddr-id!\n");
		return -EINVAL;
	}

	if (device_property_read_u32(&pdev->dev, "phytium,pmu-id",
					&ddr_pmu->pmu_id)) {
		dev_err(&pdev->dev, "Can not read ddr pmu-id!\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ddr_pmu->base = devm_ioremap_resource(&pdev->dev, res);

	if (IS_ERR(ddr_pmu->base)) {
		dev_err(&pdev->dev,
			"ioremap failed for ddr_pmu base resource\n");
		return PTR_ERR(ddr_pmu->base);
	}

	ddr_pmu->cnts_num = PHYTIUM_DDR_PMUV3_COUNTERS_NUM;
	ddr_pmu->ops = &phytium_ddr_pmu_v3_ops;

	return 0;
}

static int phytium_ddr_pmu_dev_probe(struct platform_device *pdev,
					struct phytium_ddr_pmu *ddr_pmu)
{
	int ret;

	ret = phytium_ddr_pmu_version(pdev, ddr_pmu);
	if (ret)
		return ret;

	if (ddr_pmu->ver == DDR_PMUV1P0) {
		ret = phytium_ddr_pmu_v1_verify_pbf_version(pdev);
		if (ret)
			return ret;
	}

	if (ddr_pmu->ver <= DDR_PMUV1P5)
		ret = phytium_ddr_pmu_v1_init_data(pdev, ddr_pmu);
	else if (ddr_pmu->ver == DDR_PMUV2P0)
		ret = phytium_ddr_pmu_v2_init_data(pdev, ddr_pmu);
	else if (ddr_pmu->ver == DDR_PMUV3P0)
		ret = phytium_ddr_pmu_v3_init_data(pdev, ddr_pmu);
	if (ret)
		return ret;

	if (ddr_pmu->ver <= DDR_PMUV2P0) {
		ret = phytium_ddr_pmu_init_irq(ddr_pmu, pdev);
		if (ret)
			return ret;
	}

	ddr_pmu->dev = &pdev->dev;
	ddr_pmu->on_cpu = -1;

	return 0;
}

static int phytium_ddr_pmu_probe(struct platform_device *pdev)
{
	struct phytium_ddr_pmu *ddr_pmu;
	char *name;
	int ret;

	ddr_pmu = devm_kzalloc(&pdev->dev, sizeof(*ddr_pmu), GFP_KERNEL);
	if (!ddr_pmu)
		return -ENOMEM;

	platform_set_drvdata(pdev, ddr_pmu);

	ret = phytium_ddr_pmu_dev_probe(pdev, ddr_pmu);
	if (ret)
		return ret;

	ret = cpuhp_state_add_instance(phytium_ddr_pmu_hp_state,
					&ddr_pmu->node);
	if (ret) {
		dev_err(&pdev->dev, "Error %d registering hotplug;\n", ret);
		return ret;
	}

	if (ddr_pmu->ver == DDR_PMUV2P0)
		name = devm_kasprintf(&pdev->dev, GFP_KERNEL, "phyt_ddr%u_pmu",
							ddr_pmu->ddr_id);
	else
		name = devm_kasprintf(&pdev->dev, GFP_KERNEL, "phyt%u_ddr%u_pmu%u",
							ddr_pmu->die_id, ddr_pmu->ddr_id,
							ddr_pmu->pmu_id);
	ddr_pmu->pmu = (struct pmu){
		.name = name,
		.module = THIS_MODULE,
		.task_ctx_nr = perf_invalid_context,
		.event_init = phytium_ddr_pmu_event_init,
		.pmu_enable = phytium_ddr_pmu_enable,
		.pmu_disable = phytium_ddr_pmu_disable,
		.add = phytium_ddr_pmu_event_add,
		.del = phytium_ddr_pmu_event_del,
		.start = phytium_ddr_pmu_event_start,
		.stop = phytium_ddr_pmu_event_stop,
		.read = phytium_ddr_pmu_event_update,
	};

	if (ddr_pmu->ver == DDR_PMUV2P0) {
		ddr_pmu->pmu.attr_groups = phytium_ddr_pmu_v2_attr_groups;
		dev_info(ddr_pmu->dev, "%s on cpu%d.\n",
				name, ddr_pmu->on_cpu);
	} else {
		if (ddr_pmu->ver <= DDR_PMUV1P5)
			ddr_pmu->pmu.attr_groups = phytium_ddr_pmu_v1_attr_groups;
		else if (ddr_pmu->ver == DDR_PMUV3P0)
			ddr_pmu->pmu.attr_groups = phytium_ddr_pmu_v3_attr_groups;

		ddr_pmu->ops->enable_clk(ddr_pmu);

		dev_info(ddr_pmu->dev, "die%d_ddr%d_pmu%d on cpu%d.\n", ddr_pmu->die_id,
			ddr_pmu->ddr_id, ddr_pmu->pmu_id, ddr_pmu->on_cpu);
	}

	ret = perf_pmu_register(&ddr_pmu->pmu, name, -1);
	if (ret) {
		dev_err(ddr_pmu->dev, "DDR PMU register failed!\n");
		cpuhp_state_remove_instance_nocalls(phytium_ddr_pmu_hp_state,
			&ddr_pmu->node);
	}

	return ret;
}

static int phytium_ddr_pmu_remove(struct platform_device *pdev)
{
	struct phytium_ddr_pmu *ddr_pmu = platform_get_drvdata(pdev);

	if (ddr_pmu->ver <= DDR_PMUV1P5 || ddr_pmu->ver == DDR_PMUV3P0)
		ddr_pmu->ops->disable_clk(ddr_pmu);
	else if (ddr_pmu->ver == DDR_PMUV2P0)
		phytium_ddr_pmu_v2_mask_all_irq(ddr_pmu);

	perf_pmu_unregister(&ddr_pmu->pmu);
	cpuhp_state_remove_instance_nocalls(phytium_ddr_pmu_hp_state,
					&ddr_pmu->node);

	return 0;
}

static struct platform_driver phytium_ddr_pmu_driver = {
	.driver = {
			.name = "phytium_ddr_pmu",
			.acpi_match_table = ACPI_PTR(phytium_ddr_pmu_acpi_match),
			.suppress_bind_attrs = true,
		},
	.probe = phytium_ddr_pmu_probe,
	.remove = phytium_ddr_pmu_remove,
};

int phytium_ddr_pmu_online_cpu(unsigned int cpu, struct hlist_node *node)
{
	struct phytium_ddr_pmu *ddr_pmu =
		hlist_entry_safe(node, struct phytium_ddr_pmu, node);

	if (!cpumask_test_cpu(cpu, cpumask_of_node(ddr_pmu->die_id)))
		return 0;

	if (ddr_pmu->on_cpu != -1) {
		if (!cpumask_test_cpu(ddr_pmu->on_cpu, cpumask_of_node(ddr_pmu->die_id))) {
			perf_pmu_migrate_context(&ddr_pmu->pmu, ddr_pmu->on_cpu, cpu);
			ddr_pmu->on_cpu = cpu;
			WARN_ON(irq_set_affinity_hint(ddr_pmu->irq, cpumask_of(cpu)));
		}
		return 0;
	}

	ddr_pmu->on_cpu = cpu;
	WARN_ON(irq_set_affinity_hint(ddr_pmu->irq, cpumask_of(cpu)));

	return 0;
}

int phytium_ddr_pmu_offline_cpu(unsigned int cpu, struct hlist_node *node)
{
	struct phytium_ddr_pmu *ddr_pmu =
		hlist_entry_safe(node, struct phytium_ddr_pmu, node);
	unsigned int target;
	cpumask_t available_cpus;

	if (ddr_pmu->on_cpu != cpu)
		return 0;

	if (cpumask_and(&available_cpus, cpumask_of_node(ddr_pmu->die_id), cpu_online_mask) &&
		cpumask_andnot(&available_cpus, &available_cpus, cpumask_of(cpu)))
		target = cpumask_last(&available_cpus);
	else {
		cpumask_andnot(&available_cpus, cpu_online_mask, cpumask_of(cpu));
		target = cpumask_last(&available_cpus);
	}

	if (target >= nr_cpu_ids) {
		dev_err(ddr_pmu->dev, "offline cpu%d with no target to migrate.\n",
			cpu);
		return 0;
	}

	perf_pmu_migrate_context(&ddr_pmu->pmu, cpu, target);
	WARN_ON(irq_set_affinity_hint(ddr_pmu->irq, cpumask_of(target)));
	ddr_pmu->on_cpu = target;

	return 0;
}

static int __init phytium_ddr_pmu_module_init(void)
{
	int ret;

	phytium_ddr_pmu_hp_state = cpuhp_setup_state_multi(CPUHP_AP_ONLINE_DYN,
				      "perf/phytium/ddrpmu:online",
				      phytium_ddr_pmu_online_cpu, phytium_ddr_pmu_offline_cpu);
	if (phytium_ddr_pmu_hp_state < 0) {
		pr_err("DDR PMU: setup hotplug, phytium_ddr_pmu_hp_state = %d\n",
			phytium_ddr_pmu_hp_state);
		return phytium_ddr_pmu_hp_state;
	}

	ret = platform_driver_register(&phytium_ddr_pmu_driver);
	if (ret)
		cpuhp_remove_multi_state(
			phytium_ddr_pmu_hp_state);

	return ret;
}
module_init(phytium_ddr_pmu_module_init);

static void __exit phytium_ddr_pmu_module_exit(void)
{
	platform_driver_unregister(&phytium_ddr_pmu_driver);
	cpuhp_remove_multi_state(phytium_ddr_pmu_hp_state);
}
module_exit(phytium_ddr_pmu_module_exit);

MODULE_DESCRIPTION("Phytium DDR PMU driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DDR_PERF_DRIVER_VERSION);
MODULE_AUTHOR("Hu Xianghua <huxianghua@phytium.com.cn>");
MODULE_AUTHOR("Tan Rui <tanrui2142@phytium.com.cn>");
MODULE_AUTHOR("Fu Boyi <fuboyi2150@phytium.com.cn>");
