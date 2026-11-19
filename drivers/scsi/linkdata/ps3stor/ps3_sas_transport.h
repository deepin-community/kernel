/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _WINDOWS

#ifndef _PS3_SAS_TRANSPORT_H_
#define _PS3_SAS_TRANSPORT_H_

#include "ps3_instance_manager.h"
#include <scsi/scsi_transport_sas.h>
#include <linux/version.h>

#include "ps3_htp.h"
#include "ps3_kernel_version.h"

#define PS3_SAS_TIMEOUT_SEC (40)
#define PS3_SMP_CRC_LEN (4)

struct scsi_transport_template *ps3_sas_transport_get(void);

int ps3_sas_attach_transport(void);

void ps3_sas_release_transport(void);

int ps3_sas_linkerrors_get(struct sas_phy *phy);

int ps3_sas_enclosure_identifier_get(struct sas_rphy *rphy, u64 *identifier);

int ps3_sas_bay_identifier_get(struct sas_rphy *rphy);

int ps3_sas_phy_reset(struct sas_phy *phy, int hard_reset);

int ps3_sas_phy_enable(struct sas_phy *phy, int enable);

int ps3_sas_linkrates_set(struct sas_phy *phy, struct sas_phy_linkrates *rates);

#if defined(PS3_SAS_SMP_RETURN_VOID)
void ps3_sas_smp_handler(struct bsg_job *job, struct Scsi_Host *shost,
			 struct sas_rphy *rphy);
#else
int ps3_sas_smp_handler(struct Scsi_Host *shost, struct sas_rphy *rphy,
			struct request *req);
#endif

#endif
#endif
