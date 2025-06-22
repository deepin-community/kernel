// SPDX-License-Identifier: GPL-2.0
/*
 * SAS Transport Layer for MPT (Message Passing Technology) based controllers
 *
 * Copyright (C) 2013-2018  LSI Corporation
 * Copyright (C) 2013-2018  Avago Technologies
 * Copyright (C) 2013-2018  Broadcom Inc.
 *  (mailto:MPT-FusionLinux.pdl@broadcom.com)
 *
 * Copyright (C) 2024 LeapIO Tech Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * NO WARRANTY
 * THE PROGRAM IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT
 * LIMITATION, ANY WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Each Recipient is
 * solely responsible for determining the appropriateness of using and
 * distributing the Program and assumes all risks associated with its
 * exercise of rights under this Agreement, including but not limited to
 * the risks and costs of program errors, damage to or loss of data,
 * programs or equipment, and unavailability or interruption of operations.

 * DISCLAIMER OF LIABILITY
 * NEITHER RECIPIENT NOR ANY CONTRIBUTORS SHALL HAVE ANY LIABILITY FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING WITHOUT LIMITATION LOST PROFITS), HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OR DISTRIBUTION OF THE PROGRAM OR THE EXERCISE OF ANY RIGHTS GRANTED
 * HEREUNDER, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_transport_sas.h>
#include <scsi/scsi_dbg.h>
#include "leapioraid_func.h"

static
struct leapioraid_raid_sas_node *leapioraid_transport_sas_node_find_by_sas_address(
			struct LEAPIORAID_ADAPTER *ioc,
			u64 sas_address, struct leapioraid_hba_port *port)
{
	if (ioc->sas_hba.sas_address == sas_address)
		return &ioc->sas_hba;
	else
		return leapioraid_scsihost_expander_find_by_sas_address(ioc,
									sas_address,
									port);
}

static inline u8
leapioraid_transport_get_port_id_by_sas_phy(struct sas_phy *phy)
{
	u8 port_id = 0xFF;
	struct leapioraid_hba_port *port = phy->hostdata;

	if (port)
		port_id = port->port_id;
	else
		BUG();
	return port_id;
}

static int
leapioraid_transport_find_parent_node(
	struct LEAPIORAID_ADAPTER *ioc, struct sas_phy *phy)
{
	unsigned long flags;
	struct leapioraid_hba_port *port = phy->hostdata;

	spin_lock_irqsave(&ioc->sas_node_lock, flags);
	if (leapioraid_transport_sas_node_find_by_sas_address(ioc,
						    phy->identify.sas_address,
						    port) == NULL) {
		spin_unlock_irqrestore(&ioc->sas_node_lock, flags);
		return -EINVAL;
	}
	spin_unlock_irqrestore(&ioc->sas_node_lock, flags);
	return 0;
}

static u8
leapioraid_transport_get_port_id_by_rphy(struct LEAPIORAID_ADAPTER *ioc,
			       struct sas_rphy *rphy)
{
	struct leapioraid_raid_sas_node *sas_expander;
	struct leapioraid_sas_device *sas_device;
	unsigned long flags;
	u8 port_id = 0xFF;

	if (!rphy)
		return port_id;
	if (rphy->identify.device_type == SAS_EDGE_EXPANDER_DEVICE ||
	    rphy->identify.device_type == SAS_FANOUT_EXPANDER_DEVICE) {
		spin_lock_irqsave(&ioc->sas_node_lock, flags);
		list_for_each_entry(sas_expander, &ioc->sas_expander_list, list) {
			if (sas_expander->rphy == rphy) {
				port_id = sas_expander->port->port_id;
				break;
			}
		}
		spin_unlock_irqrestore(&ioc->sas_node_lock, flags);
	} else if (rphy->identify.device_type == SAS_END_DEVICE) {
		spin_lock_irqsave(&ioc->sas_device_lock, flags);
		sas_device = __leapioraid_get_sdev_by_addr_and_rphy(
			ioc, rphy->identify.sas_address, rphy);
		if (sas_device) {
			port_id = sas_device->port->port_id;
			leapioraid_sas_device_put(sas_device);
		}
		spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	}
	return port_id;
}

static enum sas_linkrate
leapioraid_transport_convert_phy_link_rate(u8 link_rate)
{
	enum sas_linkrate rc;

	switch (link_rate) {
	case LEAPIORAID_SAS_NEG_LINK_RATE_1_5:
		rc = SAS_LINK_RATE_1_5_GBPS;
		break;
	case LEAPIORAID_SAS_NEG_LINK_RATE_3_0:
		rc = SAS_LINK_RATE_3_0_GBPS;
		break;
	case LEAPIORAID_SAS_NEG_LINK_RATE_6_0:
		rc = SAS_LINK_RATE_6_0_GBPS;
		break;
	case LEAPIORAID_SAS_NEG_LINK_RATE_12_0:
		rc = SAS_LINK_RATE_12_0_GBPS;
		break;
	case LEAPIORAID_SAS_NEG_LINK_RATE_PHY_DISABLED:
		rc = SAS_PHY_DISABLED;
		break;
	case LEAPIORAID_SAS_NEG_LINK_RATE_NEGOTIATION_FAILED:
		rc = SAS_LINK_RATE_FAILED;
		break;
	case LEAPIORAID_SAS_NEG_LINK_RATE_PORT_SELECTOR:
		rc = SAS_SATA_PORT_SELECTOR;
		break;
	case LEAPIORAID_SAS_NEG_LINK_RATE_SMP_RESET_IN_PROGRESS:
	default:
	case LEAPIORAID_SAS_NEG_LINK_RATE_SATA_OOB_COMPLETE:
	case LEAPIORAID_SAS_NEG_LINK_RATE_UNKNOWN_LINK_RATE:
		rc = SAS_LINK_RATE_UNKNOWN;
		break;
	}
	return rc;
}

static int
leapioraid_transport_set_identify(
	struct LEAPIORAID_ADAPTER *ioc, u16 handle,
	struct sas_identify *identify)
{
	struct LeapioraidSasDevP0_t sas_device_pg0;
	struct LeapioraidCfgRep_t mpi_reply;
	u32 device_info;
	u32 ioc_status;

	if ((ioc->shost_recovery && !ioc->is_driver_loading)
	    || ioc->pci_error_recovery) {
		pr_info("%s %s: host reset in progress!\n",
		       __func__, ioc->name);
		return -EFAULT;
	}
	if ((leapioraid_config_get_sas_device_pg0
	     (ioc, &mpi_reply, &sas_device_pg0,
	      LEAPIORAID_SAS_DEVICE_PGAD_FORM_HANDLE, handle))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return -ENXIO;
	}
	ioc_status = le16_to_cpu(mpi_reply.IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
	if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
		pr_err("%s handle(0x%04x), ioc_status(0x%04x)\nfailure at %s:%d/%s()!\n",
			ioc->name, handle,
			ioc_status, __FILE__, __LINE__, __func__);
		return -EIO;
	}
	memset(identify, 0, sizeof(struct sas_identify));
	device_info = le32_to_cpu(sas_device_pg0.DeviceInfo);
	identify->sas_address = le64_to_cpu(sas_device_pg0.SASAddress);
	identify->phy_identifier = sas_device_pg0.PhyNum;
	switch (device_info & LEAPIORAID_SAS_DEVICE_INFO_MASK_DEVICE_TYPE) {
	case LEAPIORAID_SAS_DEVICE_INFO_NO_DEVICE:
		identify->device_type = SAS_PHY_UNUSED;
		break;
	case LEAPIORAID_SAS_DEVICE_INFO_END_DEVICE:
		identify->device_type = SAS_END_DEVICE;
		break;
	case LEAPIORAID_SAS_DEVICE_INFO_EDGE_EXPANDER:
		identify->device_type = SAS_EDGE_EXPANDER_DEVICE;
		break;
	case LEAPIORAID_SAS_DEVICE_INFO_FANOUT_EXPANDER:
		identify->device_type = SAS_FANOUT_EXPANDER_DEVICE;
		break;
	}
	if (device_info & LEAPIORAID_SAS_DEVICE_INFO_SSP_INITIATOR)
		identify->initiator_port_protocols |= SAS_PROTOCOL_SSP;
	if (device_info & LEAPIORAID_SAS_DEVICE_INFO_STP_INITIATOR)
		identify->initiator_port_protocols |= SAS_PROTOCOL_STP;
	if (device_info & LEAPIORAID_SAS_DEVICE_INFO_SMP_INITIATOR)
		identify->initiator_port_protocols |= SAS_PROTOCOL_SMP;
	if (device_info & LEAPIORAID_SAS_DEVICE_INFO_SATA_HOST)
		identify->initiator_port_protocols |= SAS_PROTOCOL_SATA;
	if (device_info & LEAPIORAID_SAS_DEVICE_INFO_SSP_TARGET)
		identify->target_port_protocols |= SAS_PROTOCOL_SSP;
	if (device_info & LEAPIORAID_SAS_DEVICE_INFO_STP_TARGET)
		identify->target_port_protocols |= SAS_PROTOCOL_STP;
	if (device_info & LEAPIORAID_SAS_DEVICE_INFO_SMP_TARGET)
		identify->target_port_protocols |= SAS_PROTOCOL_SMP;
	if (device_info & LEAPIORAID_SAS_DEVICE_INFO_SATA_DEVICE)
		identify->target_port_protocols |= SAS_PROTOCOL_SATA;
	return 0;
}

u8
leapioraid_transport_done(struct LEAPIORAID_ADAPTER *ioc, u16 smid,
			  u8 msix_index, u32 reply)
{
	struct LeapioraidDefaultRep_t *mpi_reply;

	mpi_reply = leapioraid_base_get_reply_virt_addr(ioc, reply);
	if (ioc->transport_cmds.status == LEAPIORAID_CMD_NOT_USED)
		return 1;
	if (ioc->transport_cmds.smid != smid)
		return 1;
	ioc->transport_cmds.status |= LEAPIORAID_CMD_COMPLETE;
	if (mpi_reply) {
		memcpy(ioc->transport_cmds.reply, mpi_reply,
		       mpi_reply->MsgLength * 4);
		ioc->transport_cmds.status |= LEAPIORAID_CMD_REPLY_VALID;
	}
	ioc->transport_cmds.status &= ~LEAPIORAID_CMD_PENDING;
	complete(&ioc->transport_cmds.done);
	return 1;
}

#if defined(LEAPIORAID_WIDE_PORT_API)
struct leapioraid_rep_manu_request {
	u8 smp_frame_type;
	u8 function;
	u8 reserved;
	u8 request_length;
};

struct leapioraid_rep_manu_reply {
	u8 smp_frame_type;
	u8 function;
	u8 function_result;
	u8 response_length;
	u16 expander_change_count;
	u8 reserved0[2];
	u8 sas_format;
	u8 reserved2[3];
	u8 vendor_id[SAS_EXPANDER_VENDOR_ID_LEN];
	u8 product_id[SAS_EXPANDER_PRODUCT_ID_LEN];
	u8 product_rev[SAS_EXPANDER_PRODUCT_REV_LEN];
	u8 component_vendor_id[SAS_EXPANDER_COMPONENT_VENDOR_ID_LEN];
	u16 component_id;
	u8 component_revision_id;
	u8 reserved3;
	u8 vendor_specific[8];
};

static int
leapioraid_transport_expander_report_manufacture(
		struct LEAPIORAID_ADAPTER *ioc,
		u64 sas_address,
		struct sas_expander_device *edev,
		u8 port_id)
{
	struct LeapioraidSmpPassthroughReq_t *mpi_request;
	struct LeapioraidSmpPassthroughRep_t *mpi_reply;
	struct leapioraid_rep_manu_reply *manufacture_reply;
	struct leapioraid_rep_manu_request *manufacture_request;
	int rc;
	u16 smid;
	void *psge;
	u8 issue_reset = 0;
	void *data_out = NULL;
	dma_addr_t data_out_dma;
	dma_addr_t data_in_dma;
	size_t data_in_sz;
	size_t data_out_sz;

	if (ioc->shost_recovery || ioc->pci_error_recovery) {
		pr_info("%s %s: host reset in progress!\n",
		       __func__, ioc->name);
		return -EFAULT;
	}
	mutex_lock(&ioc->transport_cmds.mutex);
	if (ioc->transport_cmds.status != LEAPIORAID_CMD_NOT_USED) {
		pr_err("%s %s: transport_cmds in use\n",
		       ioc->name, __func__);
		mutex_unlock(&ioc->transport_cmds.mutex);
		return -EAGAIN;
	}
	ioc->transport_cmds.status = LEAPIORAID_CMD_PENDING;
	rc = leapioraid_wait_for_ioc_to_operational(ioc, 10);
	if (rc)
		goto out;
	smid = leapioraid_base_get_smid(ioc, ioc->transport_cb_idx);
	if (!smid) {
		pr_err("%s %s: failed obtaining a smid\n",
		       ioc->name, __func__);
		rc = -EAGAIN;
		goto out;
	}
	rc = 0;
	mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
	ioc->transport_cmds.smid = smid;
	data_out_sz = sizeof(struct leapioraid_rep_manu_request);
	data_in_sz = sizeof(struct leapioraid_rep_manu_reply);
	data_out = dma_alloc_coherent(&ioc->pdev->dev, data_out_sz + data_in_sz,
				      &data_out_dma, GFP_ATOMIC);
	if (!data_out) {
		rc = -ENOMEM;
		leapioraid_base_free_smid(ioc, smid);
		goto out;
	}
	data_in_dma = data_out_dma + sizeof(struct leapioraid_rep_manu_request);
	manufacture_request = data_out;
	manufacture_request->smp_frame_type = 0x40;
	manufacture_request->function = 1;
	manufacture_request->reserved = 0;
	manufacture_request->request_length = 0;
	memset(mpi_request, 0, sizeof(struct LeapioraidSmpPassthroughReq_t));
	mpi_request->Function = LEAPIORAID_FUNC_SMP_PASSTHROUGH;
	mpi_request->PhysicalPort = port_id;
	mpi_request->SASAddress = cpu_to_le64(sas_address);
	mpi_request->RequestDataLength = cpu_to_le16(data_out_sz);
	psge = &mpi_request->SGL;
	ioc->build_sg(ioc, psge, data_out_dma, data_out_sz, data_in_dma,
		      data_in_sz);
	dtransportprintk(ioc,
			 pr_info("%s report_manufacture - send to sas_addr(0x%016llx)\n",
				ioc->name,
				(unsigned long long)sas_address));
	init_completion(&ioc->transport_cmds.done);
	ioc->put_smid_default(ioc, smid);
	wait_for_completion_timeout(&ioc->transport_cmds.done, 10 * HZ);
	if (!(ioc->transport_cmds.status & LEAPIORAID_CMD_COMPLETE)) {
		pr_err("%s %s: timeout\n",
		       ioc->name, __func__);
		leapioraid_debug_dump_mf(mpi_request,
			       sizeof(struct LeapioraidSmpPassthroughReq_t) / 4);
		if (!(ioc->transport_cmds.status & LEAPIORAID_CMD_RESET))
			issue_reset = 1;
		goto issue_host_reset;
	}
	dtransportprintk(ioc,
			 pr_info("%s report_manufacture - complete\n", ioc->name));
	if (ioc->transport_cmds.status & LEAPIORAID_CMD_REPLY_VALID) {
		u8 *tmp;

		mpi_reply = ioc->transport_cmds.reply;
		dtransportprintk(ioc, pr_err(
					     "%s report_manufacture - reply data transfer size(%d)\n",
					     ioc->name,
					     le16_to_cpu(mpi_reply->ResponseDataLength)));
		if (le16_to_cpu(mpi_reply->ResponseDataLength) !=
		    sizeof(struct leapioraid_rep_manu_reply))
			goto out;
		manufacture_reply = data_out + sizeof(struct leapioraid_rep_manu_request);
		strscpy(edev->vendor_id, manufacture_reply->vendor_id,
			sizeof(edev->vendor_id));
		strscpy(edev->product_id, manufacture_reply->product_id,
			sizeof(edev->product_id));
		strscpy(edev->product_rev, manufacture_reply->product_rev,
			sizeof(edev->product_rev));
		edev->level = manufacture_reply->sas_format & 1;
		if (edev->level) {
			strscpy(edev->component_vendor_id,
				manufacture_reply->component_vendor_id,
				sizeof(edev->component_vendor_id));
			tmp = (u8 *) &manufacture_reply->component_id;
			edev->component_id = tmp[0] << 8 | tmp[1];
			edev->component_revision_id =
			    manufacture_reply->component_revision_id;
		}
	} else
		dtransportprintk(ioc, pr_err(
					     "%s report_manufacture - no reply\n",
					     ioc->name));
issue_host_reset:
	if (issue_reset)
		leapioraid_base_hard_reset_handler(ioc, FORCE_BIG_HAMMER);
out:
	ioc->transport_cmds.status = LEAPIORAID_CMD_NOT_USED;
	if (data_out)
		dma_free_coherent(&ioc->pdev->dev, data_out_sz + data_in_sz,
				  data_out, data_out_dma);
	mutex_unlock(&ioc->transport_cmds.mutex);
	return rc;
}
#endif

static void
leapioraid_transport_delete_port(struct LEAPIORAID_ADAPTER *ioc,
		       struct leapioraid_sas_port *leapioraid_port)
{
	u64 sas_address = leapioraid_port->remote_identify.sas_address;
	struct leapioraid_hba_port *port = leapioraid_port->hba_port;
	enum sas_device_type device_type =
	    leapioraid_port->remote_identify.device_type;

#if defined(LEAPIORAID_WIDE_PORT_API)
	dev_info(&leapioraid_port->port->dev,
		   "remove: sas_addr(0x%016llx)\n",
		   (unsigned long long)sas_address);
#endif
	ioc->logging_level |= LEAPIORAID_DEBUG_TRANSPORT;
	if (device_type == SAS_END_DEVICE)
		leapioraid_device_remove_by_sas_address(ioc, sas_address, port);
	else if (device_type == SAS_EDGE_EXPANDER_DEVICE ||
		 device_type == SAS_FANOUT_EXPANDER_DEVICE)
		leapioraid_expander_remove(ioc, sas_address, port);
	ioc->logging_level &= ~LEAPIORAID_DEBUG_TRANSPORT;
}

#if defined(LEAPIORAID_WIDE_PORT_API)
static void
leapioraid_transport_delete_phy(struct LEAPIORAID_ADAPTER *ioc,
		      struct leapioraid_sas_port *leapioraid_port,
		      struct leapioraid_sas_phy *leapioraid_phy)
{
	u64 sas_address = leapioraid_port->remote_identify.sas_address;

	dev_info(&leapioraid_phy->phy->dev,
		   "remove: sas_addr(0x%016llx), phy(%d)\n",
		   (unsigned long long)sas_address, leapioraid_phy->phy_id);
	list_del(&leapioraid_phy->port_siblings);
	leapioraid_port->num_phys--;
	sas_port_delete_phy(leapioraid_port->port, leapioraid_phy->phy);
	leapioraid_phy->phy_belongs_to_port = 0;
}

static void
leapioraid_transport_add_phy(struct LEAPIORAID_ADAPTER *ioc,
		   struct leapioraid_sas_port *leapioraid_port,
		   struct leapioraid_sas_phy *leapioraid_phy)
{
	u64 sas_address = leapioraid_port->remote_identify.sas_address;

	dev_info(&leapioraid_phy->phy->dev,
		   "add: sas_addr(0x%016llx), phy(%d)\n", (unsigned long long)
		   sas_address, leapioraid_phy->phy_id);
	list_add_tail(&leapioraid_phy->port_siblings,
		      &leapioraid_port->phy_list);
	leapioraid_port->num_phys++;
	sas_port_add_phy(leapioraid_port->port, leapioraid_phy->phy);
	leapioraid_phy->phy_belongs_to_port = 1;
}

void
leapioraid_transport_add_phy_to_an_existing_port(
			struct LEAPIORAID_ADAPTER *ioc,
			struct leapioraid_raid_sas_node *sas_node,
			struct leapioraid_sas_phy *leapioraid_phy,
			u64 sas_address,
			struct leapioraid_hba_port *port)
{
	struct leapioraid_sas_port *leapioraid_port;
	struct leapioraid_sas_phy *phy_srch;

	if (leapioraid_phy->phy_belongs_to_port == 1)
		return;
	if (!port)
		return;
	list_for_each_entry(leapioraid_port, &sas_node->sas_port_list,
			    port_list) {
		if (leapioraid_port->remote_identify.sas_address != sas_address)
			continue;
		if (leapioraid_port->hba_port != port)
			continue;
		list_for_each_entry(phy_srch, &leapioraid_port->phy_list,
				    port_siblings) {
			if (phy_srch == leapioraid_phy)
				return;
		}
		leapioraid_transport_add_phy(ioc, leapioraid_port, leapioraid_phy);
		return;
	}
}
#endif

void
leapioraid_transport_del_phy_from_an_existing_port(
			struct LEAPIORAID_ADAPTER *ioc,
			struct leapioraid_raid_sas_node *sas_node,
			struct leapioraid_sas_phy *leapioraid_phy)
{
	struct leapioraid_sas_port *leapioraid_port, *next;
	struct leapioraid_sas_phy *phy_srch;

	if (leapioraid_phy->phy_belongs_to_port == 0)
		return;
	list_for_each_entry_safe(leapioraid_port, next,
				 &sas_node->sas_port_list, port_list) {
		list_for_each_entry(phy_srch, &leapioraid_port->phy_list,
				    port_siblings) {
			if (phy_srch != leapioraid_phy)
				continue;
#if defined(LEAPIORAID_WIDE_PORT_API)
			if (leapioraid_port->num_phys == 1
			    && !ioc->shost_recovery)
				leapioraid_transport_delete_port(ioc, leapioraid_port);
			else
				leapioraid_transport_delete_phy(ioc, leapioraid_port,
						      leapioraid_phy);
#else
			leapioraid_transport_delete_port(ioc, leapioraid_port);
#endif
			return;
		}
	}
}

static void
leapioraid_transport_sanity_check(
		struct LEAPIORAID_ADAPTER *ioc,
		struct leapioraid_raid_sas_node *sas_node, u64 sas_address,
		struct leapioraid_hba_port *port)
{
	int i;

	for (i = 0; i < sas_node->num_phys; i++) {
		if (sas_node->phy[i].remote_identify.sas_address != sas_address
		    || sas_node->phy[i].port != port)
			continue;
		if (sas_node->phy[i].phy_belongs_to_port == 1)
			leapioraid_transport_del_phy_from_an_existing_port(ioc,
									   sas_node,
									   &sas_node->phy
									   [i]);
	}
}

struct leapioraid_sas_port *leapioraid_transport_port_add(
	struct LEAPIORAID_ADAPTER *ioc,
	u16 handle, u64 sas_address,
	struct leapioraid_hba_port *hba_port)
{
	struct leapioraid_sas_phy *leapioraid_phy, *next;
	struct leapioraid_sas_port *leapioraid_port;
	unsigned long flags;
	struct leapioraid_raid_sas_node *sas_node;
	struct sas_rphy *rphy;
	struct leapioraid_sas_device *sas_device = NULL;
	int i;
#if defined(LEAPIORAID_WIDE_PORT_API)
	struct sas_port *port;
#endif
	struct leapioraid_virtual_phy *vphy = NULL;

	if (!hba_port) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return NULL;
	}
	leapioraid_port = kzalloc(sizeof(struct leapioraid_sas_port), GFP_KERNEL);
	if (!leapioraid_port)
		return NULL;
	INIT_LIST_HEAD(&leapioraid_port->port_list);
	INIT_LIST_HEAD(&leapioraid_port->phy_list);
	spin_lock_irqsave(&ioc->sas_node_lock, flags);
	sas_node = leapioraid_transport_sas_node_find_by_sas_address(
		ioc,
		sas_address,
		hba_port);
	spin_unlock_irqrestore(&ioc->sas_node_lock, flags);
	if (!sas_node) {
		pr_err("%s %s: Could not find parent sas_address(0x%016llx)!\n",
			ioc->name,
			__func__, (unsigned long long)sas_address);
		goto out_fail;
	}
	if ((leapioraid_transport_set_identify(ioc, handle,
				     &leapioraid_port->remote_identify))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		goto out_fail;
	}
	if (leapioraid_port->remote_identify.device_type == SAS_PHY_UNUSED) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		goto out_fail;
	}
	leapioraid_port->hba_port = hba_port;
	leapioraid_transport_sanity_check(ioc, sas_node,
		leapioraid_port->remote_identify.sas_address,
		hba_port);
	for (i = 0; i < sas_node->num_phys; i++) {
		if (sas_node->phy[i].remote_identify.sas_address !=
		    leapioraid_port->remote_identify.sas_address ||
		    sas_node->phy[i].port != hba_port)
			continue;
		list_add_tail(&sas_node->phy[i].port_siblings,
			      &leapioraid_port->phy_list);
		leapioraid_port->num_phys++;
		if (sas_node->handle <= ioc->sas_hba.num_phys) {
			if (!sas_node->phy[i].hba_vphy) {
				hba_port->phy_mask |= (1 << i);
				continue;
			}
			vphy = leapioraid_get_vphy_by_phy(ioc, hba_port, i);
			if (!vphy) {
				pr_err("%s failure at %s:%d/%s()!\n",
				    ioc->name, __FILE__, __LINE__, __func__);
				goto out_fail;
			}
		}
	}
	if (!leapioraid_port->num_phys) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		goto out_fail;
	}
	if (leapioraid_port->remote_identify.device_type == SAS_END_DEVICE) {
		sas_device = leapioraid_get_sdev_by_addr(ioc,
			leapioraid_port->remote_identify.sas_address,
			leapioraid_port->hba_port);
		if (!sas_device) {
			pr_err("%s failure at %s:%d/%s()!\n",
			       ioc->name, __FILE__, __LINE__, __func__);
			goto out_fail;
		}
		sas_device->pend_sas_rphy_add = 1;
	}
#if defined(LEAPIORAID_WIDE_PORT_API)
	if (!sas_node->parent_dev) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		goto out_fail;
	}
	port = sas_port_alloc_num(sas_node->parent_dev);
	if ((sas_port_add(port))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		goto out_fail;
	}
	list_for_each_entry(leapioraid_phy, &leapioraid_port->phy_list,
			    port_siblings) {
		if ((ioc->logging_level & LEAPIORAID_DEBUG_TRANSPORT))
			dev_info(&port->dev,
				"add: handle(0x%04x), sas_addr(0x%016llx), phy(%d)\n",
				handle,
				(unsigned long long)
				leapioraid_port->remote_identify.sas_address,
				leapioraid_phy->phy_id);
		sas_port_add_phy(port, leapioraid_phy->phy);
		leapioraid_phy->phy_belongs_to_port = 1;
		leapioraid_phy->port = hba_port;
	}
	leapioraid_port->port = port;
	if (leapioraid_port->remote_identify.device_type == SAS_END_DEVICE) {
		rphy = sas_end_device_alloc(port);
		sas_device->rphy = rphy;
		if (sas_node->handle <= ioc->sas_hba.num_phys) {
			if (!vphy)
				hba_port->sas_address = sas_device->sas_address;
			else
				vphy->sas_address = sas_device->sas_address;
		}
	} else {
		rphy = sas_expander_alloc(port,
					  leapioraid_port->remote_identify.device_type);
		if (sas_node->handle <= ioc->sas_hba.num_phys)
			hba_port->sas_address =
			    leapioraid_port->remote_identify.sas_address;
	}
#else
	leapioraid_phy =
	    list_entry(leapioraid_port->phy_list.next, struct leapioraid_sas_phy,
		       port_siblings);
	if (leapioraid_port->remote_identify.device_type == SAS_END_DEVICE) {
		rphy = sas_end_device_alloc(leapioraid_phy->phy);
		sas_device->rphy = rphy;
	} else
		rphy = sas_expander_alloc(leapioraid_phy->phy,
					  leapioraid_port->remote_identify.device_type);
#endif
	rphy->identify = leapioraid_port->remote_identify;
	if ((sas_rphy_add(rphy))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
	}
	if (leapioraid_port->remote_identify.device_type == SAS_END_DEVICE) {
		sas_device->pend_sas_rphy_add = 0;
		leapioraid_sas_device_put(sas_device);
	}
	dev_info(&rphy->dev,
		   "%s: added: handle(0x%04x), sas_addr(0x%016llx)\n",
		   __func__, handle, (unsigned long long)
		   leapioraid_port->remote_identify.sas_address);
	leapioraid_port->rphy = rphy;
	spin_lock_irqsave(&ioc->sas_node_lock, flags);
	list_add_tail(&leapioraid_port->port_list, &sas_node->sas_port_list);
	spin_unlock_irqrestore(&ioc->sas_node_lock, flags);
#if defined(LEAPIORAID_WIDE_PORT_API)
	if (leapioraid_port->remote_identify.device_type ==
	    LEAPIORAID_SAS_DEVICE_INFO_EDGE_EXPANDER ||
	    leapioraid_port->remote_identify.device_type ==
	    LEAPIORAID_SAS_DEVICE_INFO_FANOUT_EXPANDER)
		leapioraid_transport_expander_report_manufacture(ioc,
			leapioraid_port->remote_identify.sas_address,
			rphy_to_expander_device
			(rphy),
			hba_port->port_id);
#endif
	return leapioraid_port;
out_fail:
	list_for_each_entry_safe(leapioraid_phy, next,
				 &leapioraid_port->phy_list, port_siblings)
		list_del(&leapioraid_phy->port_siblings);
	kfree(leapioraid_port);
	return NULL;
}

void
leapioraid_transport_port_remove(struct LEAPIORAID_ADAPTER *ioc,
				 u64 sas_address, u64 sas_address_parent,
				 struct leapioraid_hba_port *port)
{
	int i;
	unsigned long flags;
	struct leapioraid_sas_port *leapioraid_port, *next;
	struct leapioraid_raid_sas_node *sas_node;
	u8 found = 0;
#if defined(LEAPIORAID_WIDE_PORT_API)
	struct leapioraid_sas_phy *leapioraid_phy, *next_phy;
#endif
	struct leapioraid_hba_port *hba_port, *hba_port_next = NULL;
	struct leapioraid_virtual_phy *vphy, *vphy_next = NULL;

	if (!port)
		return;
	spin_lock_irqsave(&ioc->sas_node_lock, flags);
	sas_node = leapioraid_transport_sas_node_find_by_sas_address(
										ioc,
										sas_address_parent,
										port);
	if (!sas_node) {
		spin_unlock_irqrestore(&ioc->sas_node_lock, flags);
		return;
	}
	list_for_each_entry_safe(leapioraid_port, next,
				 &sas_node->sas_port_list, port_list) {
		if (leapioraid_port->remote_identify.sas_address != sas_address)
			continue;
		if (leapioraid_port->hba_port != port)
			continue;
		found = 1;
		list_del(&leapioraid_port->port_list);
		goto out;
	}
out:
	if (!found) {
		spin_unlock_irqrestore(&ioc->sas_node_lock, flags);
		return;
	}
	if ((sas_node->handle <= ioc->sas_hba.num_phys) &&
	    (ioc->multipath_on_hba)) {
		if (port->vphys_mask) {
			list_for_each_entry_safe(vphy, vphy_next,
						 &port->vphys_list, list) {
				if (vphy->sas_address != sas_address)
					continue;
				pr_err(
					"%s remove vphy entry: %p of port:%p,\n\t\t"
					"from %d port's vphys list\n",
						ioc->name,
						vphy,
						port,
						port->port_id);
				port->vphys_mask &= ~vphy->phy_mask;
				list_del(&vphy->list);
				kfree(vphy);
			}
			if (!port->vphys_mask && !port->sas_address) {
				pr_err(
					"%s remove hba_port entry: %p port: %d\n\t\t"
					"from hba_port list\n",
						ioc->name,
						port,
						port->port_id);
				list_del(&port->list);
				kfree(port);
			}
		}
		list_for_each_entry_safe(hba_port, hba_port_next,
					 &ioc->port_table_list, list) {
			if (hba_port != port)
				continue;
			if (hba_port->sas_address != sas_address)
				continue;
			if (!port->vphys_mask) {
				pr_err(
					"%s remove hba_port entry: %p port: %d\n\t\t"
					"from hba_port list\n",
						ioc->name,
						hba_port,
						hba_port->port_id);
				list_del(&hba_port->list);
				kfree(hba_port);
			} else {
				pr_err(
					"%s clearing sas_address from hba_port entry: %p\n\t\t"
					"port: %d from hba_port list\n",
						ioc->name,
						hba_port,
						hba_port->port_id);
				port->sas_address = 0;
			}
			break;
		}
	}
	for (i = 0; i < sas_node->num_phys; i++) {
		if (sas_node->phy[i].remote_identify.sas_address == sas_address) {
			memset(&sas_node->phy[i].remote_identify, 0,
			       sizeof(struct sas_identify));
			sas_node->phy[i].hba_vphy = 0;
		}
	}
	spin_unlock_irqrestore(&ioc->sas_node_lock, flags);
#if defined(LEAPIORAID_WIDE_PORT_API)
	list_for_each_entry_safe(leapioraid_phy, next_phy,
				 &leapioraid_port->phy_list, port_siblings) {
		if ((ioc->logging_level & LEAPIORAID_DEBUG_TRANSPORT))
			pr_info("%s %s: remove: sas_addr(0x%016llx), phy(%d)\n",
				ioc->name, __func__,
				   (unsigned long long)
				   leapioraid_port->remote_identify.sas_address,
				   leapioraid_phy->phy_id);
		leapioraid_phy->phy_belongs_to_port = 0;
		if (!ioc->remove_host)
			sas_port_delete_phy(leapioraid_port->port,
					    leapioraid_phy->phy);
		list_del(&leapioraid_phy->port_siblings);
	}
	if (!ioc->remove_host)
		sas_port_delete(leapioraid_port->port);
	pr_info("%s %s: removed: sas_addr(0x%016llx)\n",
	       ioc->name, __func__, (unsigned long long)sas_address);
#else
	if ((ioc->logging_level & LEAPIORAID_DEBUG_TRANSPORT))
		pr_info("%s %s: remove: sas_addr(0x%016llx)\n",
			ioc->name, __func__,
			   (unsigned long long)sas_address);
	if (!ioc->remove_host)
		sas_rphy_delete(leapioraid_port->rphy);
	pr_info("%s %s: removed: sas_addr(0x%016llx)\n",
	       ioc->name, __func__, (unsigned long long)sas_address);
#endif
	kfree(leapioraid_port);
}

int
leapioraid_transport_add_host_phy(
	struct LEAPIORAID_ADAPTER *ioc,
	struct leapioraid_sas_phy *leapioraid_phy,
	struct LeapioraidSasPhyP0_t phy_pg0,
	struct device *parent_dev)
{
	struct sas_phy *phy;
	int phy_index = leapioraid_phy->phy_id;

	INIT_LIST_HEAD(&leapioraid_phy->port_siblings);
	phy = sas_phy_alloc(parent_dev, phy_index);
	if (!phy) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return -1;
	}
	if ((leapioraid_transport_set_identify(ioc, leapioraid_phy->handle,
				     &leapioraid_phy->identify))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		sas_phy_free(phy);
		return -1;
	}
	phy->identify = leapioraid_phy->identify;
	leapioraid_phy->attached_handle =
	    le16_to_cpu(phy_pg0.AttachedDevHandle);
	if (leapioraid_phy->attached_handle)
		leapioraid_transport_set_identify(
				ioc, leapioraid_phy->attached_handle,
				&leapioraid_phy->remote_identify);
	phy->identify.phy_identifier = leapioraid_phy->phy_id;
	phy->negotiated_linkrate =
	    leapioraid_transport_convert_phy_link_rate(
			phy_pg0.NegotiatedLinkRate &
					     LEAPIORAID_SAS_NEG_LINK_RATE_MASK_PHYSICAL);
	phy->minimum_linkrate_hw =
	    leapioraid_transport_convert_phy_link_rate(
			phy_pg0.HwLinkRate &
					     LEAPIORAID_SAS_HWRATE_MIN_RATE_MASK);
	phy->maximum_linkrate_hw =
	    leapioraid_transport_convert_phy_link_rate(
			phy_pg0.HwLinkRate >> 4);
	phy->minimum_linkrate =
	    leapioraid_transport_convert_phy_link_rate(
			phy_pg0.ProgrammedLinkRate &
					     LEAPIORAID_SAS_PRATE_MIN_RATE_MASK);
	phy->maximum_linkrate =
	    leapioraid_transport_convert_phy_link_rate(
			phy_pg0.ProgrammedLinkRate >> 4);
	phy->hostdata = leapioraid_phy->port;
#if !defined(LEAPIORAID_WIDE_PORT_API_PLUS)
	phy->local_attached = 1;
#endif
#if !defined(LEAPIORAID_WIDE_PORT_API)
	phy->port_identifier = phy_index;
#endif
	if ((sas_phy_add(phy))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		sas_phy_free(phy);
		return -1;
	}
	if ((ioc->logging_level & LEAPIORAID_DEBUG_TRANSPORT))
		dev_info(&phy->dev,
			   "add: handle(0x%04x), sas_addr(0x%016llx)\n"
			   "\tattached_handle(0x%04x), sas_addr(0x%016llx)\n",
			   leapioraid_phy->handle, (unsigned long long)
			   leapioraid_phy->identify.sas_address,
			   leapioraid_phy->attached_handle, (unsigned long long)
			   leapioraid_phy->remote_identify.sas_address);
	leapioraid_phy->phy = phy;
	return 0;
}

int
leapioraid_transport_add_expander_phy(
		struct LEAPIORAID_ADAPTER *ioc,
		struct leapioraid_sas_phy *leapioraid_phy,
		struct LeapioraidExpanderP1_t expander_pg1,
		struct device *parent_dev)
{
	struct sas_phy *phy;
	int phy_index = leapioraid_phy->phy_id;

	INIT_LIST_HEAD(&leapioraid_phy->port_siblings);
	phy = sas_phy_alloc(parent_dev, phy_index);
	if (!phy) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return -1;
	}
	if ((leapioraid_transport_set_identify(ioc, leapioraid_phy->handle,
				     &leapioraid_phy->identify))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		sas_phy_free(phy);
		return -1;
	}
	phy->identify = leapioraid_phy->identify;
	leapioraid_phy->attached_handle =
	    le16_to_cpu(expander_pg1.AttachedDevHandle);
	if (leapioraid_phy->attached_handle)
		leapioraid_transport_set_identify(
			ioc, leapioraid_phy->attached_handle,
					&leapioraid_phy->remote_identify);
	phy->identify.phy_identifier = leapioraid_phy->phy_id;
	phy->negotiated_linkrate =
	    leapioraid_transport_convert_phy_link_rate(
			expander_pg1.NegotiatedLinkRate &
					     LEAPIORAID_SAS_NEG_LINK_RATE_MASK_PHYSICAL);
	phy->minimum_linkrate_hw =
	    leapioraid_transport_convert_phy_link_rate(
			expander_pg1.HwLinkRate &
					     LEAPIORAID_SAS_HWRATE_MIN_RATE_MASK);
	phy->maximum_linkrate_hw =
	    leapioraid_transport_convert_phy_link_rate(
			expander_pg1.HwLinkRate >> 4);
	phy->minimum_linkrate =
	    leapioraid_transport_convert_phy_link_rate(
			expander_pg1.ProgrammedLinkRate &
					     LEAPIORAID_SAS_PRATE_MIN_RATE_MASK);
	phy->maximum_linkrate =
	    leapioraid_transport_convert_phy_link_rate(
			expander_pg1.ProgrammedLinkRate >> 4);
	phy->hostdata = leapioraid_phy->port;
#if !defined(LEAPIORAID_WIDE_PORT_API)
	phy->port_identifier = phy_index;
#endif
	if ((sas_phy_add(phy))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		sas_phy_free(phy);
		return -1;
	}
	if ((ioc->logging_level & LEAPIORAID_DEBUG_TRANSPORT))
		dev_info(&phy->dev,
			   "add: handle(0x%04x), sas_addr(0x%016llx)\n"
			   "\tattached_handle(0x%04x), sas_addr(0x%016llx)\n",
			   leapioraid_phy->handle, (unsigned long long)
			   leapioraid_phy->identify.sas_address,
			   leapioraid_phy->attached_handle, (unsigned long long)
			   leapioraid_phy->remote_identify.sas_address);
	leapioraid_phy->phy = phy;
	return 0;
}

void
leapioraid_transport_update_links(struct LEAPIORAID_ADAPTER *ioc,
				  u64 sas_address, u16 handle, u8 phy_number,
				  u8 link_rate, struct leapioraid_hba_port *port)
{
	unsigned long flags;
	struct leapioraid_raid_sas_node *sas_node;
	struct leapioraid_sas_phy *leapioraid_phy;
	struct leapioraid_hba_port *hba_port = NULL;

	if (ioc->shost_recovery || ioc->pci_error_recovery)
		return;
	spin_lock_irqsave(&ioc->sas_node_lock, flags);
	sas_node = leapioraid_transport_sas_node_find_by_sas_address(ioc,
							   sas_address, port);
	if (!sas_node) {
		spin_unlock_irqrestore(&ioc->sas_node_lock, flags);
		return;
	}
	leapioraid_phy = &sas_node->phy[phy_number];
	leapioraid_phy->attached_handle = handle;
	spin_unlock_irqrestore(&ioc->sas_node_lock, flags);
	if (handle && (link_rate >= LEAPIORAID_SAS_NEG_LINK_RATE_1_5)) {
		leapioraid_transport_set_identify(ioc, handle,
					&leapioraid_phy->remote_identify);
#if defined(LEAPIORAID_WIDE_PORT_API)
		if ((sas_node->handle <= ioc->sas_hba.num_phys) &&
		    (ioc->multipath_on_hba)) {
			list_for_each_entry(hba_port,
					    &ioc->port_table_list, list) {
				if (hba_port->sas_address == sas_address &&
				    hba_port == port)
					hba_port->phy_mask |=
					    (1 << leapioraid_phy->phy_id);
			}
		}
		leapioraid_transport_add_phy_to_an_existing_port(ioc, sas_node,
				leapioraid_phy,
				leapioraid_phy->remote_identify.sas_address,
				port);
#endif
	} else
		memset(&leapioraid_phy->remote_identify, 0, sizeof(struct
								   sas_identify));
	if (leapioraid_phy->phy)
		leapioraid_phy->phy->negotiated_linkrate =
		    leapioraid_transport_convert_phy_link_rate(link_rate);
	if ((ioc->logging_level & LEAPIORAID_DEBUG_TRANSPORT))
		dev_info(&leapioraid_phy->phy->dev,
			   "refresh: parent sas_addr(0x%016llx),\n"
			   "\tlink_rate(0x%02x), phy(%d)\n"
			   "\tattached_handle(0x%04x), sas_addr(0x%016llx)\n",
			   (unsigned long long)sas_address,
			   link_rate, phy_number, handle, (unsigned long long)
			   leapioraid_phy->remote_identify.sas_address);
}

static inline void *phy_to_ioc(struct sas_phy *phy)
{
	struct Scsi_Host *shost = dev_to_shost(phy->dev.parent);

	return leapioraid_shost_private(shost);
}

static inline void *rphy_to_ioc(struct sas_rphy *rphy)
{
	struct Scsi_Host *shost = dev_to_shost(rphy->dev.parent->parent);

	return leapioraid_shost_private(shost);
}

struct leapioraid_phy_error_log_request {
	u8 smp_frame_type;
	u8 function;
	u8 allocated_response_length;
	u8 request_length;
	u8 reserved_1[5];
	u8 phy_identifier;
	u8 reserved_2[2];
};

struct leapioraid_phy_error_log_reply {
	u8 smp_frame_type;
	u8 function;
	u8 function_result;
	u8 response_length;
	__be16 expander_change_count;
	u8 reserved_1[3];
	u8 phy_identifier;
	u8 reserved_2[2];
	__be32 invalid_dword;
	__be32 running_disparity_error;
	__be32 loss_of_dword_sync;
	__be32 phy_reset_problem;
};

static int
leapioraid_transport_get_expander_phy_error_log(
	struct LEAPIORAID_ADAPTER *ioc, struct sas_phy *phy)
{
	struct LeapioraidSmpPassthroughReq_t *mpi_request;
	struct LeapioraidSmpPassthroughRep_t *mpi_reply;
	struct leapioraid_phy_error_log_request *phy_error_log_request;
	struct leapioraid_phy_error_log_reply *phy_error_log_reply;
	int rc;
	u16 smid;
	void *psge;
	u8 issue_reset = 0;
	void *data_out = NULL;
	dma_addr_t data_out_dma;
	u32 sz;

	if (ioc->shost_recovery || ioc->pci_error_recovery) {
		pr_info("%s %s: host reset in progress!\n",
		       __func__, ioc->name);
		return -EFAULT;
	}
	mutex_lock(&ioc->transport_cmds.mutex);
	if (ioc->transport_cmds.status != LEAPIORAID_CMD_NOT_USED) {
		pr_err("%s %s: transport_cmds in use\n",
		       ioc->name, __func__);
		mutex_unlock(&ioc->transport_cmds.mutex);
		return -EAGAIN;
	}
	ioc->transport_cmds.status = LEAPIORAID_CMD_PENDING;
	rc = leapioraid_wait_for_ioc_to_operational(ioc, 10);
	if (rc)
		goto out;
	smid = leapioraid_base_get_smid(ioc, ioc->transport_cb_idx);
	if (!smid) {
		pr_err("%s %s: failed obtaining a smid\n",
		       ioc->name, __func__);
		rc = -EAGAIN;
		goto out;
	}
	mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
	ioc->transport_cmds.smid = smid;
	sz = sizeof(struct leapioraid_phy_error_log_request) +
	    sizeof(struct leapioraid_phy_error_log_reply);
	data_out =
	    dma_alloc_coherent(&ioc->pdev->dev, sz, &data_out_dma,
			GFP_ATOMIC);
	if (!data_out) {
		pr_err("failure at %s:%d/%s()!\n", __FILE__,
		       __LINE__, __func__);
		rc = -ENOMEM;
		leapioraid_base_free_smid(ioc, smid);
		goto out;
	}
	rc = -EINVAL;
	memset(data_out, 0, sz);
	phy_error_log_request = data_out;
	phy_error_log_request->smp_frame_type = 0x40;
	phy_error_log_request->function = 0x11;
	phy_error_log_request->request_length = 2;
	phy_error_log_request->allocated_response_length = 0;
	phy_error_log_request->phy_identifier = phy->number;
	memset(mpi_request, 0, sizeof(struct LeapioraidSmpPassthroughReq_t));
	mpi_request->Function = LEAPIORAID_FUNC_SMP_PASSTHROUGH;
	mpi_request->PhysicalPort = leapioraid_transport_get_port_id_by_sas_phy(phy);
	mpi_request->VF_ID = 0;
	mpi_request->VP_ID = 0;
	mpi_request->SASAddress = cpu_to_le64(phy->identify.sas_address);
	mpi_request->RequestDataLength =
	    cpu_to_le16(sizeof(struct leapioraid_phy_error_log_request));
	psge = &mpi_request->SGL;
	ioc->build_sg(ioc, psge, data_out_dma,
		      sizeof(struct leapioraid_phy_error_log_request),
		      data_out_dma + sizeof(struct leapioraid_phy_error_log_request),
		      sizeof(struct leapioraid_phy_error_log_reply));
	dtransportprintk(ioc, pr_info(
		"%s phy_error_log - send to sas_addr(0x%016llx), phy(%d)\n",
		ioc->name,
		(unsigned long long)phy->identify.sas_address,
		phy->number));
	init_completion(&ioc->transport_cmds.done);
	ioc->put_smid_default(ioc, smid);
	wait_for_completion_timeout(&ioc->transport_cmds.done, 10 * HZ);
	if (!(ioc->transport_cmds.status & LEAPIORAID_CMD_COMPLETE)) {
		pr_err("%s %s: timeout\n",
		       ioc->name, __func__);
		leapioraid_debug_dump_mf(mpi_request,
			       sizeof(struct LeapioraidSmpPassthroughReq_t) / 4);
		if (!(ioc->transport_cmds.status & LEAPIORAID_CMD_RESET))
			issue_reset = 1;
		goto issue_host_reset;
	}
	dtransportprintk(ioc, pr_info("%s phy_error_log - complete\n", ioc->name));
	if (ioc->transport_cmds.status & LEAPIORAID_CMD_REPLY_VALID) {
		mpi_reply = ioc->transport_cmds.reply;
		dtransportprintk(ioc, pr_err(
					     "%s phy_error_log - reply data transfer size(%d)\n",
					     ioc->name,
					     le16_to_cpu(mpi_reply->ResponseDataLength)));
		if (le16_to_cpu(mpi_reply->ResponseDataLength) !=
		    sizeof(struct leapioraid_phy_error_log_reply))
			goto out;
		phy_error_log_reply = data_out +
		    sizeof(struct leapioraid_phy_error_log_request);
		dtransportprintk(ioc, pr_err(
					     "%s phy_error_log - function_result(%d)\n",
					     ioc->name,
					     phy_error_log_reply->function_result));
		phy->invalid_dword_count =
		    be32_to_cpu(phy_error_log_reply->invalid_dword);
		phy->running_disparity_error_count =
		    be32_to_cpu(phy_error_log_reply->running_disparity_error);
		phy->loss_of_dword_sync_count =
		    be32_to_cpu(phy_error_log_reply->loss_of_dword_sync);
		phy->phy_reset_problem_count =
		    be32_to_cpu(phy_error_log_reply->phy_reset_problem);
		rc = 0;
	} else
		dtransportprintk(ioc, pr_err(
					     "%s phy_error_log - no reply\n",
					     ioc->name));
issue_host_reset:
	if (issue_reset)
		leapioraid_base_hard_reset_handler(ioc, FORCE_BIG_HAMMER);
out:
	ioc->transport_cmds.status = LEAPIORAID_CMD_NOT_USED;
	if (data_out)
		dma_free_coherent(&ioc->pdev->dev, sz, data_out, data_out_dma);
	mutex_unlock(&ioc->transport_cmds.mutex);
	return rc;
}

static int
leapioraid_transport_get_linkerrors(struct sas_phy *phy)
{
	struct LEAPIORAID_ADAPTER *ioc = phy_to_ioc(phy);
	struct LeapioraidCfgRep_t mpi_reply;
	struct LeapioraidSasPhyP1_t phy_pg1;
	int rc = 0;

	rc = leapioraid_transport_find_parent_node(ioc, phy);
	if (rc)
		return rc;
	if (phy->identify.sas_address != ioc->sas_hba.sas_address)
		return leapioraid_transport_get_expander_phy_error_log(ioc, phy);
	if ((leapioraid_config_get_phy_pg1(ioc, &mpi_reply, &phy_pg1,
					   phy->number))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return -ENXIO;
	}
	if (mpi_reply.IOCStatus || mpi_reply.IOCLogInfo)
		pr_info("%s phy(%d), ioc_status(0x%04x), loginfo(0x%08x)\n",
				ioc->name,
				phy->number,
				le16_to_cpu(mpi_reply.IOCStatus),
				le32_to_cpu(mpi_reply.IOCLogInfo));
	phy->invalid_dword_count = le32_to_cpu(phy_pg1.InvalidDwordCount);
	phy->running_disparity_error_count =
	    le32_to_cpu(phy_pg1.RunningDisparityErrorCount);
	phy->loss_of_dword_sync_count =
	    le32_to_cpu(phy_pg1.LossDwordSynchCount);
	phy->phy_reset_problem_count =
	    le32_to_cpu(phy_pg1.PhyResetProblemCount);
	return 0;
}

static int
leapioraid_transport_get_enclosure_identifier(
	struct sas_rphy *rphy, u64 *identifier)
{
	struct LEAPIORAID_ADAPTER *ioc = rphy_to_ioc(rphy);
	struct leapioraid_sas_device *sas_device;
	unsigned long flags;
	int rc;

	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	sas_device = __leapioraid_get_sdev_by_addr_and_rphy(ioc,
							    rphy->identify.sas_address, rphy);
	if (sas_device) {
		*identifier = sas_device->enclosure_logical_id;
		rc = 0;
		leapioraid_sas_device_put(sas_device);
	} else {
		*identifier = 0;
		rc = -ENXIO;
	}
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	return rc;
}

static int
leapioraid_transport_get_bay_identifier(struct sas_rphy *rphy)
{
	struct LEAPIORAID_ADAPTER *ioc = rphy_to_ioc(rphy);
	struct leapioraid_sas_device *sas_device;
	unsigned long flags;
	int rc;

	spin_lock_irqsave(&ioc->sas_device_lock, flags);
	sas_device = __leapioraid_get_sdev_by_addr_and_rphy(ioc,
							    rphy->identify.sas_address, rphy);
	if (sas_device) {
		rc = sas_device->slot;
		leapioraid_sas_device_put(sas_device);
	} else {
		rc = -ENXIO;
	}
	spin_unlock_irqrestore(&ioc->sas_device_lock, flags);
	return rc;
}

struct leapioraid_phy_control_request {
	u8 smp_frame_type;
	u8 function;
	u8 allocated_response_length;
	u8 request_length;
	u16 expander_change_count;
	u8 reserved_1[3];
	u8 phy_identifier;
	u8 phy_operation;
	u8 reserved_2[13];
	u64 attached_device_name;
	u8 programmed_min_physical_link_rate;
	u8 programmed_max_physical_link_rate;
	u8 reserved_3[6];
};

struct leapioraid_phy_control_reply {
	u8 smp_frame_type;
	u8 function;
	u8 function_result;
	u8 response_length;
};

#define LEAPIORAID_SMP_PHY_CONTROL_LINK_RESET	(0x01)
#define LEAPIORAID_SMP_PHY_CONTROL_HARD_RESET	(0x02)
#define LEAPIORAID_SMP_PHY_CONTROL_DISABLE		(0x03)
static int
leapioraid_transport_expander_phy_control(
	struct LEAPIORAID_ADAPTER *ioc,
	struct sas_phy *phy, u8 phy_operation)
{
	struct LeapioraidSmpPassthroughReq_t *mpi_request;
	struct LeapioraidSmpPassthroughRep_t *mpi_reply;
	struct leapioraid_phy_control_request *phy_control_request;
	struct leapioraid_phy_control_reply *phy_control_reply;
	int rc;
	u16 smid;
	void *psge;
	u8 issue_reset = 0;
	void *data_out = NULL;
	dma_addr_t data_out_dma;
	u32 sz;

	if (ioc->shost_recovery || ioc->pci_error_recovery) {
		pr_info("%s %s: host reset in progress!\n",
		       __func__, ioc->name);
		return -EFAULT;
	}
	mutex_lock(&ioc->transport_cmds.mutex);
	if (ioc->transport_cmds.status != LEAPIORAID_CMD_NOT_USED) {
		pr_err("%s %s: transport_cmds in use\n",
		       ioc->name, __func__);
		mutex_unlock(&ioc->transport_cmds.mutex);
		return -EAGAIN;
	}
	ioc->transport_cmds.status = LEAPIORAID_CMD_PENDING;
	rc = leapioraid_wait_for_ioc_to_operational(ioc, 10);
	if (rc)
		goto out;
	smid = leapioraid_base_get_smid(ioc, ioc->transport_cb_idx);
	if (!smid) {
		pr_err("%s %s: failed obtaining a smid\n",
		       ioc->name, __func__);
		rc = -EAGAIN;
		goto out;
	}
	mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
	ioc->transport_cmds.smid = smid;
	sz = sizeof(struct leapioraid_phy_control_request) +
	    sizeof(struct leapioraid_phy_control_reply);
	data_out =
	    dma_alloc_coherent(&ioc->pdev->dev, sz, &data_out_dma,
			GFP_ATOMIC);
	if (!data_out) {
		pr_err("failure at %s:%d/%s()!\n", __FILE__,
		       __LINE__, __func__);
		rc = -ENOMEM;
		leapioraid_base_free_smid(ioc, smid);
		goto out;
	}
	rc = -EINVAL;
	memset(data_out, 0, sz);
	phy_control_request = data_out;
	phy_control_request->smp_frame_type = 0x40;
	phy_control_request->function = 0x91;
	phy_control_request->request_length = 9;
	phy_control_request->allocated_response_length = 0;
	phy_control_request->phy_identifier = phy->number;
	phy_control_request->phy_operation = phy_operation;
	phy_control_request->programmed_min_physical_link_rate =
	    phy->minimum_linkrate << 4;
	phy_control_request->programmed_max_physical_link_rate =
	    phy->maximum_linkrate << 4;
	memset(mpi_request, 0, sizeof(struct LeapioraidSmpPassthroughReq_t));
	mpi_request->Function = LEAPIORAID_FUNC_SMP_PASSTHROUGH;
	mpi_request->PhysicalPort = leapioraid_transport_get_port_id_by_sas_phy(phy);
	mpi_request->VF_ID = 0;
	mpi_request->VP_ID = 0;
	mpi_request->SASAddress = cpu_to_le64(phy->identify.sas_address);
	mpi_request->RequestDataLength =
	    cpu_to_le16(sizeof(struct leapioraid_phy_error_log_request));
	psge = &mpi_request->SGL;
	ioc->build_sg(ioc, psge, data_out_dma,
		      sizeof(struct leapioraid_phy_control_request),
		      data_out_dma + sizeof(struct leapioraid_phy_control_request),
		      sizeof(struct leapioraid_phy_control_reply));
	dtransportprintk(ioc, pr_info(
		"%s phy_control - send to sas_addr(0x%016llx), phy(%d), opcode(%d)\n",
		ioc->name,
		(unsigned long long)phy->identify.sas_address,
		phy->number, phy_operation));
	init_completion(&ioc->transport_cmds.done);
	ioc->put_smid_default(ioc, smid);
	wait_for_completion_timeout(&ioc->transport_cmds.done, 10 * HZ);
	if (!(ioc->transport_cmds.status & LEAPIORAID_CMD_COMPLETE)) {
		pr_err("%s %s: timeout\n",
		       ioc->name, __func__);
		leapioraid_debug_dump_mf(mpi_request,
			       sizeof(struct LeapioraidSmpPassthroughReq_t) / 4);
		if (!(ioc->transport_cmds.status & LEAPIORAID_CMD_RESET))
			issue_reset = 1;
		goto issue_host_reset;
	}
	dtransportprintk(ioc, pr_info(
		"%s phy_control - complete\n", ioc->name));
	if (ioc->transport_cmds.status & LEAPIORAID_CMD_REPLY_VALID) {
		mpi_reply = ioc->transport_cmds.reply;
		dtransportprintk(ioc, pr_err(
					"%s phy_control - reply data transfer size(%d)\n",
					ioc->name,
					le16_to_cpu(mpi_reply->ResponseDataLength)));
		if (le16_to_cpu(mpi_reply->ResponseDataLength) !=
		    sizeof(struct leapioraid_phy_control_reply))
			goto out;
		phy_control_reply = data_out +
		    sizeof(struct leapioraid_phy_control_request);
		dtransportprintk(ioc, pr_err(
					"%s phy_control - function_result(%d)\n",
					ioc->name,
					phy_control_reply->function_result));
		rc = 0;
	} else
		dtransportprintk(ioc, pr_err(
					"%s phy_control - no reply\n",
					ioc->name));
issue_host_reset:
	if (issue_reset)
		leapioraid_base_hard_reset_handler(ioc, FORCE_BIG_HAMMER);
out:
	ioc->transport_cmds.status = LEAPIORAID_CMD_NOT_USED;
	if (data_out)
		dma_free_coherent(&ioc->pdev->dev, sz, data_out, data_out_dma);
	mutex_unlock(&ioc->transport_cmds.mutex);
	return rc;
}

static int
leapioraid_transport_phy_reset(struct sas_phy *phy, int hard_reset)
{
	struct LEAPIORAID_ADAPTER *ioc = phy_to_ioc(phy);
	struct LeapioraidSasIoUnitControlRep_t mpi_reply;
	struct LeapioraidSasIoUnitControlReq_t mpi_request;
	int rc = 0;

	rc = leapioraid_transport_find_parent_node(ioc, phy);
	if (rc)
		return rc;
	if (phy->identify.sas_address != ioc->sas_hba.sas_address)
		return leapioraid_transport_expander_phy_control(ioc, phy,
						       (hard_reset ==
							1) ?
						       LEAPIORAID_SMP_PHY_CONTROL_HARD_RESET
						       :
						       LEAPIORAID_SMP_PHY_CONTROL_LINK_RESET);
	memset(&mpi_request, 0, sizeof(struct LeapioraidSasIoUnitControlReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_SAS_IO_UNIT_CONTROL;
	mpi_request.Operation = hard_reset ?
	    LEAPIORAID_SAS_OP_PHY_HARD_RESET : LEAPIORAID_SAS_OP_PHY_LINK_RESET;
	mpi_request.PhyNum = phy->number;
	if ((leapioraid_base_sas_iounit_control(ioc, &mpi_reply, &mpi_request))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return -ENXIO;
	}
	if (mpi_reply.IOCStatus || mpi_reply.IOCLogInfo)
		pr_info("%s phy(%d), ioc_status(0x%04x), loginfo(0x%08x)\n",
				ioc->name,
				phy->number,
				le16_to_cpu(mpi_reply.IOCStatus),
				le32_to_cpu(mpi_reply.IOCLogInfo));
	return 0;
}

static int
leapioraid_transport_phy_enable(struct sas_phy *phy, int enable)
{
	struct LEAPIORAID_ADAPTER *ioc = phy_to_ioc(phy);
	struct LeapioraidSasIOUnitP1_t *sas_iounit_pg1 = NULL;
	struct LeapioraidSasIOUnitP0_t *sas_iounit_pg0 = NULL;
	struct LeapioraidCfgRep_t mpi_reply;
	u16 ioc_status;
	u16 sz;
	int rc = 0;
	int i, discovery_active;

	rc = leapioraid_transport_find_parent_node(ioc, phy);
	if (rc)
		return rc;
	if (phy->identify.sas_address != ioc->sas_hba.sas_address)
		return leapioraid_transport_expander_phy_control(ioc, phy,
						       (enable ==
							1) ?
						       LEAPIORAID_SMP_PHY_CONTROL_LINK_RESET
						       :
						       LEAPIORAID_SMP_PHY_CONTROL_DISABLE);
	sz = offsetof(struct LeapioraidSasIOUnitP0_t,
		      PhyData) +
	    (ioc->sas_hba.num_phys * sizeof(struct LEAPIORAID_SAS_IO_UNIT0_PHY_DATA));
	sas_iounit_pg0 = kzalloc(sz, GFP_KERNEL);
	if (!sas_iounit_pg0) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		rc = -ENOMEM;
		goto out;
	}
	if ((leapioraid_config_get_sas_iounit_pg0(ioc, &mpi_reply,
						  sas_iounit_pg0, sz))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		rc = -ENXIO;
		goto out;
	}
	ioc_status = le16_to_cpu(mpi_reply.IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
	if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		rc = -EIO;
		goto out;
	}
	for (i = 0, discovery_active = 0; i < ioc->sas_hba.num_phys; i++) {
		if (sas_iounit_pg0->PhyData[i].PortFlags &
		    LEAPIORAID_SASIOUNIT0_PORTFLAGS_DISCOVERY_IN_PROGRESS) {
			pr_err(
				"%s discovery is active on port = %d, phy = %d:\n\t\t"
					"unable to enable/disable phys, try again later!\n",
					ioc->name,
					sas_iounit_pg0->PhyData[i].Port,
					i);
			discovery_active = 1;
		}
	}
	if (discovery_active) {
		rc = -EAGAIN;
		goto out;
	}
	sz = offsetof(struct LeapioraidSasIOUnitP1_t,
		      PhyData) +
	    (ioc->sas_hba.num_phys * sizeof(struct LEAPIORAID_SAS_IO_UNIT1_PHY_DATA));
	sas_iounit_pg1 = kzalloc(sz, GFP_KERNEL);
	if (!sas_iounit_pg1) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		rc = -ENOMEM;
		goto out;
	}
	if ((leapioraid_config_get_sas_iounit_pg1(ioc, &mpi_reply,
						  sas_iounit_pg1, sz))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		rc = -ENXIO;
		goto out;
	}
	ioc_status = le16_to_cpu(mpi_reply.IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
	if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		rc = -EIO;
		goto out;
	}
	for (i = 0; i < ioc->sas_hba.num_phys; i++) {
		sas_iounit_pg1->PhyData[i].Port =
		    sas_iounit_pg0->PhyData[i].Port;
		sas_iounit_pg1->PhyData[i].PortFlags =
		    (sas_iounit_pg0->PhyData[i].PortFlags &
		     LEAPIORAID_SASIOUNIT0_PORTFLAGS_AUTO_PORT_CONFIG);
		sas_iounit_pg1->PhyData[i].PhyFlags =
		    (sas_iounit_pg0->PhyData[i].PhyFlags &
		     (LEAPIORAID_SASIOUNIT0_PHYFLAGS_ZONING_ENABLED +
		      LEAPIORAID_SASIOUNIT0_PHYFLAGS_PHY_DISABLED));
	}
	if (enable)
		sas_iounit_pg1->PhyData[phy->number].PhyFlags
		    &= ~LEAPIORAID_SASIOUNIT1_PHYFLAGS_PHY_DISABLE;
	else
		sas_iounit_pg1->PhyData[phy->number].PhyFlags
		    |= LEAPIORAID_SASIOUNIT1_PHYFLAGS_PHY_DISABLE;
	leapioraid_config_set_sas_iounit_pg1(ioc, &mpi_reply, sas_iounit_pg1,
					     sz);
	if (enable)
		leapioraid_transport_phy_reset(phy, 0);
out:
	kfree(sas_iounit_pg1);
	kfree(sas_iounit_pg0);
	return rc;
}

static int
leapioraid_transport_phy_speed(
	struct sas_phy *phy, struct sas_phy_linkrates *rates)
{
	struct LEAPIORAID_ADAPTER *ioc = phy_to_ioc(phy);
	struct LeapioraidSasIOUnitP1_t *sas_iounit_pg1 = NULL;
	struct LeapioraidSasPhyP0_t phy_pg0;
	struct LeapioraidCfgRep_t mpi_reply;
	u16 ioc_status;
	u16 sz;
	int i;
	int rc = 0;

	rc = leapioraid_transport_find_parent_node(ioc, phy);
	if (rc)
		return rc;
	if (!rates->minimum_linkrate)
		rates->minimum_linkrate = phy->minimum_linkrate;
	else if (rates->minimum_linkrate < phy->minimum_linkrate_hw)
		rates->minimum_linkrate = phy->minimum_linkrate_hw;
	if (!rates->maximum_linkrate)
		rates->maximum_linkrate = phy->maximum_linkrate;
	else if (rates->maximum_linkrate > phy->maximum_linkrate_hw)
		rates->maximum_linkrate = phy->maximum_linkrate_hw;
	if (phy->identify.sas_address != ioc->sas_hba.sas_address) {
		phy->minimum_linkrate = rates->minimum_linkrate;
		phy->maximum_linkrate = rates->maximum_linkrate;
		return leapioraid_transport_expander_phy_control(ioc, phy,
						       LEAPIORAID_SMP_PHY_CONTROL_LINK_RESET);
	}
	sz = offsetof(struct LeapioraidSasIOUnitP1_t,
		      PhyData) +
	    (ioc->sas_hba.num_phys * sizeof(struct LEAPIORAID_SAS_IO_UNIT1_PHY_DATA));
	sas_iounit_pg1 = kzalloc(sz, GFP_KERNEL);
	if (!sas_iounit_pg1) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		rc = -ENOMEM;
		goto out;
	}
	if ((leapioraid_config_get_sas_iounit_pg1(ioc, &mpi_reply,
						  sas_iounit_pg1, sz))) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		rc = -ENXIO;
		goto out;
	}
	ioc_status = le16_to_cpu(mpi_reply.IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
	if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		rc = -EIO;
		goto out;
	}
	for (i = 0; i < ioc->sas_hba.num_phys; i++) {
		if (phy->number != i) {
			sas_iounit_pg1->PhyData[i].MaxMinLinkRate =
			    (ioc->sas_hba.phy[i].phy->minimum_linkrate +
			     (ioc->sas_hba.phy[i].phy->maximum_linkrate << 4));
		} else {
			sas_iounit_pg1->PhyData[i].MaxMinLinkRate =
			    (rates->minimum_linkrate +
			     (rates->maximum_linkrate << 4));
		}
	}
	if (leapioraid_config_set_sas_iounit_pg1
	    (ioc, &mpi_reply, sas_iounit_pg1, sz)) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		rc = -ENXIO;
		goto out;
	}
	leapioraid_transport_phy_reset(phy, 0);
	if (!leapioraid_config_get_phy_pg0(ioc, &mpi_reply, &phy_pg0,
					   phy->number)) {
		phy->minimum_linkrate =
		    leapioraid_transport_convert_phy_link_rate(
				phy_pg0.ProgrammedLinkRate &
						     LEAPIORAID_SAS_PRATE_MIN_RATE_MASK);
		phy->maximum_linkrate =
		    leapioraid_transport_convert_phy_link_rate(
				phy_pg0.ProgrammedLinkRate >> 4);
		phy->negotiated_linkrate =
		    leapioraid_transport_convert_phy_link_rate(
				phy_pg0.NegotiatedLinkRate &
						     LEAPIORAID_SAS_NEG_LINK_RATE_MASK_PHYSICAL);
	}
out:
	kfree(sas_iounit_pg1);
	return rc;
}

static int
leapioraid_transport_map_smp_buffer(
	struct device *dev, struct bsg_buffer *buf,
	dma_addr_t *dma_addr, size_t *dma_len, void **p)
{
	if (buf->sg_cnt > 1) {
		*p = dma_alloc_coherent(dev, buf->payload_len, dma_addr,
					GFP_KERNEL);
		if (!*p)
			return -ENOMEM;
		*dma_len = buf->payload_len;
	} else {
		if (!dma_map_sg(dev, buf->sg_list, 1, DMA_BIDIRECTIONAL))
			return -ENOMEM;
		*dma_addr = sg_dma_address(buf->sg_list);
		*dma_len = sg_dma_len(buf->sg_list);
		*p = NULL;
	}
	return 0;
}

static void
leapioraid_transport_unmap_smp_buffer(
	struct device *dev, struct bsg_buffer *buf,
	dma_addr_t dma_addr, void *p)
{
	if (p)
		dma_free_coherent(dev, buf->payload_len, p, dma_addr);
	else
		dma_unmap_sg(dev, buf->sg_list, 1, DMA_BIDIRECTIONAL);
}

static void
leapioraid_transport_smp_handler(
	struct bsg_job *job, struct Scsi_Host *shost,
	struct sas_rphy *rphy)
{
	struct LEAPIORAID_ADAPTER *ioc = shost_priv(shost);
	struct LeapioraidSmpPassthroughReq_t *mpi_request;
	struct LeapioraidSmpPassthroughRep_t *mpi_reply;
	int rc;
	u16 smid;
	u32 ioc_state;
	void *psge;
	dma_addr_t dma_addr_in;
	dma_addr_t dma_addr_out;
	void *addr_in = NULL;
	void *addr_out = NULL;
	size_t dma_len_in;
	size_t dma_len_out;
	u16 wait_state_count;
	unsigned int reslen = 0;

	if (ioc->shost_recovery || ioc->pci_error_recovery) {
		pr_info("%s %s: host reset in progress!\n",
			__func__, ioc->name);
		rc = -EFAULT;
		goto job_done;
	}
	rc = mutex_lock_interruptible(&ioc->transport_cmds.mutex);
	if (rc)
		goto job_done;
	if (ioc->transport_cmds.status != LEAPIORAID_CMD_NOT_USED) {
		pr_err("%s %s: transport_cmds in use\n",
		       ioc->name, __func__);
		mutex_unlock(&ioc->transport_cmds.mutex);
		rc = -EAGAIN;
		goto job_done;
	}
	ioc->transport_cmds.status = LEAPIORAID_CMD_PENDING;
	rc = leapioraid_transport_map_smp_buffer(
		&ioc->pdev->dev, &job->request_payload,
		&dma_addr_out, &dma_len_out, &addr_out);
	if (rc)
		goto out;
	if (addr_out) {
		sg_copy_to_buffer(job->request_payload.sg_list,
				  job->request_payload.sg_cnt, addr_out,
				  job->request_payload.payload_len);
	}
	rc = leapioraid_transport_map_smp_buffer(
		&ioc->pdev->dev, &job->reply_payload,
		&dma_addr_in, &dma_len_in, &addr_in);
	if (rc)
		goto unmap_out;
	wait_state_count = 0;
	ioc_state = leapioraid_base_get_iocstate(ioc, 1);
	while (ioc_state != LEAPIORAID_IOC_STATE_OPERATIONAL) {
		if (wait_state_count++ == 10) {
			pr_err(
			       "%s %s: failed due to ioc not operational\n",
			       ioc->name, __func__);
			rc = -EFAULT;
			goto unmap_in;
		}
		ssleep(1);
		ioc_state = leapioraid_base_get_iocstate(ioc, 1);
		pr_info(
			"%s %s: waiting for operational state(count=%d)\n",
			ioc->name, __func__, wait_state_count);
	}
	if (wait_state_count)
		pr_info("%s %s: ioc is operational\n",
			ioc->name, __func__);
	smid = leapioraid_base_get_smid(ioc, ioc->transport_cb_idx);
	if (!smid) {
		pr_err("%s %s: failed obtaining a smid\n",
		       ioc->name, __func__);
		rc = -EAGAIN;
		goto unmap_in;
	}
	rc = 0;
	mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
	ioc->transport_cmds.smid = smid;
	memset(mpi_request, 0, sizeof(struct LeapioraidSmpPassthroughReq_t));
	mpi_request->Function = LEAPIORAID_FUNC_SMP_PASSTHROUGH;
	mpi_request->PhysicalPort = leapioraid_transport_get_port_id_by_rphy(
		ioc, rphy);
	mpi_request->SASAddress = (rphy) ?
	    cpu_to_le64(rphy->identify.sas_address) :
	    cpu_to_le64(ioc->sas_hba.sas_address);
	mpi_request->RequestDataLength = cpu_to_le16(dma_len_out - 4);
	psge = &mpi_request->SGL;
	ioc->build_sg(ioc, psge, dma_addr_out, dma_len_out - 4, dma_addr_in,
		      dma_len_in - 4);
	dtransportprintk(ioc, pr_info(
				"%s %s - sending smp request\n", ioc->name,
				__func__));
	init_completion(&ioc->transport_cmds.done);
	ioc->put_smid_default(ioc, smid);
	wait_for_completion_timeout(&ioc->transport_cmds.done, 10 * HZ);
	if (!(ioc->transport_cmds.status & LEAPIORAID_CMD_COMPLETE)) {
		pr_err("%s %s : timeout\n", __func__, ioc->name);
		leapioraid_debug_dump_mf(mpi_request,
			       sizeof(struct LeapioraidSmpPassthroughReq_t) / 4);
		if (!(ioc->transport_cmds.status & LEAPIORAID_CMD_RESET)) {
			leapioraid_base_hard_reset_handler(ioc,
							   FORCE_BIG_HAMMER);
			rc = -ETIMEDOUT;
			goto unmap_in;
		}
	}
	dtransportprintk(ioc, pr_info(
				      "%s %s - complete\n", ioc->name, __func__));
	if (!(ioc->transport_cmds.status & LEAPIORAID_CMD_REPLY_VALID)) {
		dtransportprintk(ioc, pr_info(
					      "%s %s - no reply\n", ioc->name,
					      __func__));
		rc = -ENXIO;
		goto unmap_in;
	}
	mpi_reply = ioc->transport_cmds.reply;
	dtransportprintk(ioc,
			 pr_info(
				 "%s %s - reply data transfer size(%d)\n",
				 ioc->name, __func__,
				 le16_to_cpu(mpi_reply->ResponseDataLength)));
	memcpy(job->reply, mpi_reply, sizeof(*mpi_reply));
	job->reply_len = sizeof(*mpi_reply);
	reslen = le16_to_cpu(mpi_reply->ResponseDataLength);
	if (addr_in) {
		sg_copy_from_buffer(job->reply_payload.sg_list,
				    job->reply_payload.sg_cnt, addr_in,
				    job->reply_payload.payload_len);
	}
	rc = 0;
unmap_in:
	leapioraid_transport_unmap_smp_buffer(
		&ioc->pdev->dev, &job->reply_payload,
				    dma_addr_in, addr_in);
unmap_out:
	leapioraid_transport_unmap_smp_buffer(
		&ioc->pdev->dev, &job->request_payload,
				    dma_addr_out, addr_out);
out:
	ioc->transport_cmds.status = LEAPIORAID_CMD_NOT_USED;
	mutex_unlock(&ioc->transport_cmds.mutex);
job_done:
	bsg_job_done(job, rc, reslen);
}

struct sas_function_template leapioraid_transport_functions = {
	.get_linkerrors = leapioraid_transport_get_linkerrors,
	.get_enclosure_identifier = leapioraid_transport_get_enclosure_identifier,
	.get_bay_identifier = leapioraid_transport_get_bay_identifier,
	.phy_reset = leapioraid_transport_phy_reset,
	.phy_enable = leapioraid_transport_phy_enable,
	.set_phy_speed = leapioraid_transport_phy_speed,
	.smp_handler = leapioraid_transport_smp_handler,
};

struct scsi_transport_template *leapioraid_transport_template;