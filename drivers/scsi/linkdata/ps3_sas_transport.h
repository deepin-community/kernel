#ifndef _WINDOWS

#ifndef _PS3_SAS_TRANSPORT_H_
#define _PS3_SAS_TRANSPORT_H_

#include "ps3_instance_manager.h"
#include <scsi/scsi_transport_sas.h>
#include <linux/version.h>

#include "ps3_htp.h"

#define PS3_SAS_TIMEOUT_SEC (40)
#define PS3_SMP_CRC_LEN (4)

struct scsi_transport_template *ps3_sas_transport_get(void);

S32 ps3_sas_attach_transport(void);

void ps3_sas_release_transport(void);

S32 ps3_sas_linkerrors_get(struct sas_phy *phy);

S32 ps3_sas_enclosure_identifier_get(struct sas_rphy *rphy, u64 *identifier);

S32 ps3_sas_bay_identifier_get(struct sas_rphy *rphy);

S32 ps3_sas_phy_reset(struct sas_phy *phy, int hard_reset);

S32 ps3_sas_phy_enable(struct sas_phy *phy, int enable);

S32 ps3_sas_linkrates_set(struct sas_phy *phy, struct sas_phy_linkrates *rates);

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,14,0)
S32 ps3_sas_smp_handler(struct Scsi_Host *shost, struct sas_rphy *rphy,
	struct request *req);
#else
void ps3_sas_smp_handler(struct bsg_job *job, struct Scsi_Host *shost,
	struct sas_rphy *rphy);
#endif

#endif
#endif
