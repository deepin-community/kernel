/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_SCSIH_RAID_ENGINE_H_
#define _PS3_SCSIH_RAID_ENGINE_H_

#include "ps3_htp.h"
#include "ps3_err_def.h"
#include "ps3_cmd_channel.h"

enum ps3_odd_r1x_vd_judge_type {
	PS3_IS_HDD_R1X_VD,
	PS3_IS_SSD_ODD_R1X_VD,
	PS3_IS_SSD_EVEN_R1X_VD,
	PS3_IS_VALID_R1X_VD
};

unsigned char ps3_scsih_is_same_strip(const struct PS3VDEntry *vd_entry,
				      unsigned int vlba_lo,
				      unsigned int lba_length);

int ps3_scsih_vd_rw_io_to_pd_calc(struct ps3_cmd *cmd);

unsigned char ps3_scsih_vd_acc_att_build(struct ps3_cmd *cmd);

int ps3_scsih_vlba_to_pd_calc(struct ps3_cmd *cmd);

void ps3_qos_cmd_member_pd_calc(struct ps3_cmd *cmd);

unsigned short ps3_odd_r1x_judge(struct PS3VDEntry *vd_entry);

#endif
