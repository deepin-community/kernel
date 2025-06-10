/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2022 - 2025 Mucse Corporation. */

#ifndef __RNMP_MPE_H__
#define __RNMP_MPE_H__

#include "rnpm.h"

extern unsigned int mpe_src_port;
extern unsigned int mpe_pkt_version;

struct mpe_shm_comm {
	int valid_magic;
#define DRV_MPE_SHM_MAGIC 0xA5A6
	int ablity; /* set by mpe. driver readonly */
	int ctrl_flags; /* set by driver.mpe readonly */
	int nic_mode; /* (0|1): 1port 2:2port 3:4port for one pf */
	int lane_rx_pkts[10];
	int lane_drop_pkts[10];
	int lane_drop_as_busy_pkts[10];
	int lane_drop_as_err_pkts[10];
	int lane_to_rpu_pkts[10];
	int lane_to_port_pkts[10]; /*  to phy port */
	int lane_to_drv_pkts[10];
	int invliad_lane_pkts;
	int rev;
};

struct rss_queue_info {
	int valid;
#define INFO_VALID 0xa8

	int queue_start;
	int queue_end;
};

struct ipsec_rss_shm {
	struct mpe_shm_comm comm;

#define MAX_PORT_CNT 8
	struct rss_queue_info irss[MAX_PORT_CNT];
} __attribute__((packed, aligned(4)));

int rnpm_rpu_mpe_start(struct rnpm_pf_adapter *pf_adapter);
void rnpm_rpu_mpe_stop(struct rnpm_pf_adapter *pf_adapter);

void rnpm_mpe_shm_write32(struct rnpm_pf_adapter *pf_adapter, int nr_mpe,
			  int offset_bytes, unsigned int value);
int rnpm_mpe_shm_read32(struct rnpm_pf_adapter *pf_adapter, int nr_mpe,
			int offset);
void rnpm_mpe_shm_write32_array(struct rnpm_pf_adapter *pf_adapter,
				int nr_mpe, int offset_bytes, int *values,
				int bytes);
void rnpm_mpe_shm_read32_array(struct rnpm_pf_adapter *pf_adapter,
			       int nr_mpe, int offset_bytes, int *values,
			       int bytes);
#endif /* __RNMP_MPE_H__ */
