/* SPDX-License-Identifier: GPL-2.0 */
#include "ps3_cmd_channel.h"

struct ps3_nvme_scsi_status {
	unsigned char status;
	unsigned char senseKey;
	unsigned char asc;
	unsigned char ascq;
};

void ps3_nvme_error_to_scsi_status(struct PS3NvmeCmdStatus status,
				   struct ps3_nvme_scsi_status *cpl);
void ps3_nvme_resp_to_scsi_status(struct ps3_cmd *cmd);
