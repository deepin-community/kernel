
#include "ps3_cmd_channel.h"

typedef struct ps3_nvme_scsi_status {
	U8 status;
	U8 senseKey;
	U8 asc;
	U8 ascq;
} ps3_nvme_scsi_status;

void ps3_nvme_error_to_scsi_status(PS3NvmeCmdStatus_s status, ps3_nvme_scsi_status *cpl);

void ps3_nvme_resp_to_scsi_status(struct ps3_cmd *cmd);