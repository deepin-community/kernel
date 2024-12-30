#ifndef _WINDOWS

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/uio.h>
#include <linux/irq_poll.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/compiler.h>

#include <scsi/scsi_host.h>

#include "ps3_device_manager_sas.h"
#include "ps3_sas_transport.h"
#include "ps3_util.h"
#include "ps3_mgr_cmd.h"

static S32 ps3_sas_expander_event_update(struct ps3_instance *instance, U8 encl_id);

Bool ps3_sas_is_support_smp(struct ps3_instance *instance)
{
	return instance->sas_dev_context.is_support_smp;
}

static inline Bool ps3_is_sata_end_device(struct ps3_pd_entry *pd_entry)
{
	return (pd_entry->dev_type == PS3_DEV_TYPE_SATA_HDD ||
			pd_entry->dev_type == PS3_DEV_TYPE_SATA_SSD);
}

S32 ps3_sas_rphy_slot_get(struct ps3_instance *instance, U64 sas_addr, U32 *slot_id)
{
	S32 ret = -PS3_FAILED;
	struct ps3_sas_node *ps3_sas_node = &instance->sas_dev_context.ps3_hba_sas;
	struct ps3_sas_port *ps3_sas_port = NULL;
	ULong flags = 0;

	spin_lock_irqsave(&instance->sas_dev_context.ps3_sas_node_lock, flags);

	list_for_each_entry(ps3_sas_port, &ps3_sas_node->sas_port_list, list) {
		if (ps3_sas_port->remote_identify.sas_address ==
				sas_addr) {
			*slot_id = ps3_sas_node->phys[ps3_sas_port->
				remote_identify.phy_identifier].slot_id;
			ret = PS3_SUCCESS;
			INJECT_START(PS3_ERR_IJ_SAS_RPHY_SLOT_GET_FAILED, &ret)
			goto l_out;
		}
	}

	list_for_each_entry(ps3_sas_node,
			&instance->sas_dev_context.ps3_sas_node_list, list) {
		list_for_each_entry(ps3_sas_port,
				&ps3_sas_node->sas_port_list, list) {
			if (ps3_sas_port->remote_identify.sas_address ==
					sas_addr) {
				*slot_id = ps3_sas_node->phys[ps3_sas_port->
					remote_identify.phy_identifier].slot_id;
				ret = PS3_SUCCESS;
				INJECT_START(PS3_ERR_IJ_SAS_EXP_RPHY_SLOT_GET_FAILED, &ret)
				goto l_out;
			}
		}
	}
l_out:
	spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);

	return ret;
}

U64 ps3_sas_rphy_parent_sas_addr_get(struct ps3_instance *instance, U64 sas_addr)
{
	struct ps3_sas_node *ps3_sas_node = NULL;
	struct ps3_sas_port *ps3_sas_port = NULL;
	struct ps3_sas_phy *ps3_sas_phy = NULL;

	U64 encl_id = PS3_SAS_INVALID_SAS_ADDR;
	ULong flags = 0;

	spin_lock_irqsave(&instance->sas_dev_context.ps3_sas_node_lock, flags);

	list_for_each_entry(ps3_sas_port,
			&instance->sas_dev_context.ps3_hba_sas.sas_port_list, list) {
		if (ps3_sas_port->remote_identify.sas_address !=
				sas_addr) {
			continue;
		}

		list_for_each_entry(ps3_sas_phy,
				&ps3_sas_port->phy_list, port_siblings) {
			if (ps3_sas_phy->remote_identify.sas_address ==
					sas_addr) {
				encl_id = ps3_sas_phy->identify.sas_address;
				goto l_out;
			}
		}
	}

	list_for_each_entry(ps3_sas_node,
			&instance->sas_dev_context.ps3_sas_node_list, list) {
		list_for_each_entry(ps3_sas_port,
				&ps3_sas_node->sas_port_list, list) {
			if (ps3_sas_port->remote_identify.sas_address ==
					sas_addr) {
				encl_id = ps3_sas_node->sas_address;
				goto l_out;
			}
		}
	}
l_out:
	spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);

	return encl_id;
}

U8 ps3_sas_encl_id_get(struct ps3_instance *instance, U64 sas_addr)
{
	U8 encl_id = PS3_SAS_INVALID_ID;
	struct ps3_sas_node *ps3_sas_node = NULL;
	ULong flags = 0;
	U8 i = 0;

	for (i = 0; i < PS3_SAS_HBA_MAX_SAS_NUM; i++) {
		if (instance->sas_dev_context.ps3_hba_sas_addr[i] == sas_addr) {
			return instance->sas_dev_context.ps3_hba_sas.encl_id;
		}
	}

	spin_lock_irqsave(&instance->sas_dev_context.ps3_sas_node_lock, flags);
	list_for_each_entry(ps3_sas_node,
			&instance->sas_dev_context.ps3_sas_node_list, list) {
		if (ps3_sas_node->sas_address == sas_addr) {
			encl_id = ps3_sas_node->encl_id;
			goto l_out;
		}
	}
l_out:
	spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);

	return encl_id;
}

static void ps3_sas_node_phy_init(struct ps3_sas_phy *ps3_phy,
	struct ps3_sas_node *sas_node, struct PS3PhyInfo *phy_info)
{
	ps3_phy->phy_id = phy_info->phyId;
	ps3_phy->encl_id = sas_node->encl_id;
	ps3_phy->slot_id = phy_info->slotId;

	ps3_phy->identify.device_type = sas_node->dev_type;
	ps3_phy->identify.sas_address = le64_to_cpu(phy_info->sasAddr);
	ps3_phy->identify.initiator_port_protocols =
		phy_info->initiatorPortProtocol;
	ps3_phy->identify.target_port_protocols =
		phy_info->targetPortProtocols;
	ps3_phy->identify.phy_identifier = phy_info->phyId;

	if (phy_info->attachedSasAddr != PS3_SAS_INVALID_SAS_ADDR) {
		ps3_phy->remote_identify.device_type = phy_info->attachDevType;
		ps3_phy->remote_identify.sas_address = le64_to_cpu(phy_info->attachedSasAddr);
		ps3_phy->remote_identify.initiator_port_protocols =
			phy_info->attachInitiatorPortProtocol;
		ps3_phy->remote_identify.target_port_protocols =
			phy_info->attachTargetPortProtocols;
		ps3_phy->remote_identify.phy_identifier = phy_info->phyId;
	}
}

static S32 ps3_sas_node_phy_add(struct ps3_instance *instance,
	struct ps3_sas_phy *ps3_phy, struct ps3_sas_node *sas_node,
	struct PS3PhyInfo *phy_info)
{
	S32 ret = PS3_SUCCESS;
	struct sas_phy *sas_phy = NULL;

	INIT_LIST_HEAD(&ps3_phy->port_siblings);
	sas_phy = sas_phy_alloc(sas_node->dev, phy_info->phyId);
	INJECT_START(PS3_ERR_IJ_SAS_ALLOC_PHY_NULL1, &sas_phy);
	if (sas_phy == NULL) {
		LOG_ERROR("hno:%u alloc node[%d] phys[%d] buffer failed !\n",
			PS3_HOST(instance), sas_node->encl_id,
			phy_info->phyId);
		ret = -PS3_ENOMEM;
		goto l_out;
	}

	ps3_sas_node_phy_init(ps3_phy, sas_node, phy_info);
	sas_phy->identify = ps3_phy->identify;

	sas_phy->negotiated_linkrate = phy_info->negLinkRate;
	sas_phy->minimum_linkrate = phy_info->minLinkRate;
	sas_phy->maximum_linkrate = phy_info->maxLinkRate;
	sas_phy->minimum_linkrate_hw = phy_info->minLinkRateHw;
	sas_phy->maximum_linkrate_hw = phy_info->maxLinkRateHw;

	LOG_INFO("hno:%u phy %d in encl[%d], dev_type[%d],"
		"sas_addr[%016llx], n_linkrate[%d], slot_id[%d], i_prol[%d], t_prol[%d], "
		"remote_dev_type[%d], remote_sas_addr[%016llx], i_prol[%d], t_prol[%d]"
		" min_linkr[%u:%u], max_linikr[%u:%u] !\n",
		PS3_HOST(instance), sas_phy->identify.phy_identifier,
		sas_node->encl_id, sas_phy->identify.device_type,
		sas_phy->identify.sas_address,
		sas_phy->negotiated_linkrate, phy_info->slotId,
		sas_phy->identify.initiator_port_protocols,
		sas_phy->identify.target_port_protocols,
		ps3_phy->remote_identify.device_type,
		ps3_phy->remote_identify.sas_address,
		ps3_phy->remote_identify.initiator_port_protocols,
		ps3_phy->remote_identify.target_port_protocols,
		sas_phy->minimum_linkrate, sas_phy->minimum_linkrate_hw,
		sas_phy->maximum_linkrate, sas_phy->maximum_linkrate_hw);

	ret = sas_phy_add(sas_phy);
	INJECT_START(PS3_ERR_IJ_SAS_PHY_ADD_FAILED1, &ret);
	if (ret != 0) {
		LOG_ERROR("hno:%u add node[%d]-phys[%d] failed ret[%d] !\n",
			PS3_HOST(instance), sas_node->encl_id,
			phy_info->phyId, ret);
		sas_phy_free(sas_phy);
		goto l_out;
	}

	ps3_phy->phy = sas_phy;
l_out:
	return ret;
}

void ps3_sas_node_phy_update(struct ps3_instance *instance, struct ps3_sas_phy *ps3_phy,
	struct PS3PhyInfo *phy_info)
{
	ps3_phy->identify.initiator_port_protocols =
		phy_info->initiatorPortProtocol;
	ps3_phy->identify.target_port_protocols =
		phy_info->targetPortProtocols;
#if 0 
	if (phy_info->attachedSasAddr != PS3_SAS_INVALID_SAS_ADDR) {
#endif
		ps3_phy->remote_identify.device_type = phy_info->attachDevType;
		ps3_phy->remote_identify.sas_address = phy_info->attachedSasAddr;
		ps3_phy->remote_identify.initiator_port_protocols =
			phy_info->attachInitiatorPortProtocol;
		ps3_phy->remote_identify.target_port_protocols =
			phy_info->attachTargetPortProtocols;
		ps3_phy->remote_identify.phy_identifier = phy_info->phyId;
#if 0
	}
#endif
	ps3_phy->phy->identify = ps3_phy->identify;

	ps3_phy->phy->negotiated_linkrate = phy_info->negLinkRate;
	ps3_phy->phy->enabled = phy_info->enable;
	ps3_phy->phy->minimum_linkrate = phy_info->minLinkRate;
	ps3_phy->phy->maximum_linkrate = phy_info->maxLinkRate;
	ps3_phy->phy->minimum_linkrate_hw = phy_info->minLinkRateHw;
	ps3_phy->phy->maximum_linkrate_hw = phy_info->maxLinkRateHw;

	LOG_INFO_IN_IRQ(instance, "update phy %d, dev_type[%d], enable[%d]"
		"sas_addr[%016llx], n_linkrate[%d], slot_id[%d], i_prol[%d], t_prol[%d], "
		"remote_dev_type[%d], remote_sas_addr[%016llx], i_prol[%d], t_prol[%d]"
		" min_linkr[%u:%u], max_linikr[%u:%u] !\n",
		ps3_phy->phy->identify.phy_identifier,
		ps3_phy->phy->identify.device_type,
		ps3_phy->phy->enabled,
		ps3_phy->phy->identify.sas_address,
		ps3_phy->phy->negotiated_linkrate, phy_info->slotId,
		ps3_phy->phy->identify.initiator_port_protocols,
		ps3_phy->phy->identify.target_port_protocols,
		ps3_phy->remote_identify.device_type,
		ps3_phy->remote_identify.sas_address,
		ps3_phy->remote_identify.initiator_port_protocols,
		ps3_phy->remote_identify.target_port_protocols,
		ps3_phy->phy->minimum_linkrate,
		ps3_phy->phy->minimum_linkrate_hw,
		ps3_phy->phy->maximum_linkrate,
		ps3_phy->phy->maximum_linkrate_hw);
}

static void ps3_sas_port_phy_update(struct ps3_instance *instance,
	struct ps3_sas_node *sas_node)
{
	struct ps3_sas_port *ps3_sas_port = NULL;
	struct ps3_sas_port *ps3_sas_port_next = NULL;
	U8 i = 0;
	ULong flags = 0;

	for (i = 0; i < sas_node->phy_count; i++) {
		if (sas_node->phys[i].remote_identify.sas_address == 0 ||
			sas_node->phys[i].remote_identify.device_type == SAS_END_DEVICE) {
			continue;
		}

		if (sas_node->phys[i].attach_port != NULL) {
			continue;
		}

		list_for_each_entry_safe(ps3_sas_port, ps3_sas_port_next,
			&sas_node->sas_port_list, list) {
			if (sas_node->phys[i].remote_identify.sas_address ==
				ps3_sas_port->remote_identify.sas_address) {
				spin_lock_irqsave(&instance->sas_dev_context.ps3_sas_node_lock, flags);
				list_add_tail(&sas_node->phys[i].port_siblings, &ps3_sas_port->phy_list);
				sas_node->phys[i].attach_port = ps3_sas_port;
				spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);
				ps3_sas_port->phy_count++;
				sas_port_add_phy(ps3_sas_port->port, sas_node->phys[i].phy);
			}
		}
	}
}

static struct ps3_sas_port* ps3_sas_port_find(struct ps3_instance *instance,
	struct ps3_sas_node *exp_node, U64 sas_addr)
{
	struct ps3_sas_port *ps3_sas_port = NULL;
	ULong flags = 0;

	spin_lock_irqsave(&instance->sas_dev_context.ps3_sas_node_lock, flags);
	list_for_each_entry(ps3_sas_port, &exp_node->sas_port_list, list) {
		if (ps3_sas_port->remote_identify.sas_address == sas_addr) {
			spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);
			return ps3_sas_port;
		}
	}
	spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);

	return NULL;
}

static void ps3_sas_port_phy_add(struct ps3_instance *instance,
	struct ps3_sas_port *ps3_sas_port, struct ps3_sas_phy *ps3_phy)
{
	struct ps3_sas_phy *tmp_phy = NULL;
	ULong flags = 0;

	spin_lock_irqsave(&instance->sas_dev_context.ps3_sas_node_lock, flags);
	list_for_each_entry(tmp_phy, &ps3_sas_port->phy_list, port_siblings) {
		if (tmp_phy == ps3_phy) {
			spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);
			return;
		}
	}
	list_add_tail(&ps3_phy->port_siblings, &ps3_sas_port->phy_list);
	ps3_phy->attach_port = ps3_sas_port;
	spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);

	ps3_sas_port->phy_count++;

	sas_port_add_phy(ps3_sas_port->port, ps3_phy->phy);
}

static struct sas_port* ps3_sas_sas_port_create(struct ps3_instance *instance,
	struct ps3_sas_node *parent_node, struct list_head *phy_list)
{
	S32 ret = PS3_SUCCESS;
	struct sas_port *sas_port = NULL;
	struct ps3_sas_phy *ps3_phy = NULL;

	sas_port = sas_port_alloc_num(parent_node->dev);
	INJECT_START(PS3_ERR_IJ_SAS_ALLOC_NUM_NULL, &sas_port);
	if (sas_port == NULL) {
		LOG_ERROR("hno:%u alloc sas_port on node[%d] failed !\n",
			PS3_HOST(instance), parent_node->encl_id);
		goto l_out;
	}

	ret = sas_port_add(sas_port);
	INJECT_START(PS3_ERR_IJ_SAS_PORT_ADD_FAILED, &ret);
	if (ret != 0) {
		LOG_ERROR("hno:%u add sas_port on node[%d] failed !\n",
			PS3_HOST(instance), parent_node->encl_id);
		goto l_failed;
	}

	list_for_each_entry(ps3_phy, phy_list, port_siblings) {
		LOG_DEBUG("hno:%u add phy[%d] in sas_port[%016llx] on node[%d] !\n",
			PS3_HOST(instance), ps3_phy->phy_id,
			ps3_phy->remote_identify.sas_address,
			parent_node->encl_id);
		sas_port_add_phy(sas_port, ps3_phy->phy);
	}

	return sas_port;
l_failed:
	sas_port_delete(sas_port);
l_out:
	return NULL;
}

static S32 ps3_sas_end_device_add_past(struct ps3_instance *instance,
	struct ps3_sas_port *ps3_sas_port, struct ps3_pd_entry *pd_entry,
	struct sas_rphy *sas_rphy)
{
	S32 ret = PS3_SUCCESS;

	if (ps3_sas_port->remote_identify.device_type != SAS_END_DEVICE) {
		ret = PS3_SUCCESS;
		goto l_out;
	}

	sas_rphy->identify = ps3_sas_port->remote_identify;
	ret = ps3_scsi_add_device_ack(instance, &pd_entry->disk_pos,
		PS3_DISK_TYPE_PD);
	INJECT_START(PS3_ERR_IJ_SAS_ADD_ACK_FAILED, &ret);
	if (unlikely(ret != PS3_SUCCESS)) {
		LOG_ERROR("hno:%u rphy sas_addr[%016llx] end-device[%u:%u:%u] "
			"magic[%#x] add scsi device ack NOK, ret %d\n",
			PS3_HOST(instance), ps3_sas_port->remote_identify.sas_address,
			PS3_CHANNEL(&pd_entry->disk_pos), PS3_TARGET(&pd_entry->disk_pos),
			PS3_PDID(&pd_entry->disk_pos),
			pd_entry->disk_pos.diskMagicNum, ret);
		ret = -PS3_ACTIVE_ERR;
	} else {
		LOG_WARN("hno:%u rphy sas_addr[%016llx] end-device[%u:%u:%u] add begin\n",
			PS3_HOST(instance), ps3_sas_port->remote_identify.sas_address,
			PS3_CHANNEL(&pd_entry->disk_pos), PS3_TARGET(&pd_entry->disk_pos),
			PS3_PDID(&pd_entry->disk_pos));
		scsi_scan_target(&sas_rphy->dev, PS3_CHANNEL(&pd_entry->disk_pos),
			PS3_TARGET(&pd_entry->disk_pos), 0, SCSI_SCAN_INITIAL);
		LOG_WARN("hno:%u rphy sas_addr[%016llx] end-device[%u:%u:%u] add end\n",
			PS3_HOST(instance), ps3_sas_port->remote_identify.sas_address,
			PS3_CHANNEL(&pd_entry->disk_pos), PS3_TARGET(&pd_entry->disk_pos),
			PS3_PDID(&pd_entry->disk_pos));
	}
l_out:
	return ret;
}

static S32 ps3_sas_port_rphy_create(struct ps3_instance *instance,
	struct ps3_sas_port *ps3_sas_port)
{
	S32 ret = -PS3_FAILED;
	struct sas_rphy *sas_rphy = NULL;
	struct ps3_pd_entry *pd_entry = NULL;

	LOG_DEBUG("hno:%u enter port rphy create, type[%d], pdflatid[%d], sas_addr[%016llx]\n",
		PS3_HOST(instance), ps3_sas_port->remote_identify.device_type,
		ps3_sas_port->pd_flat_id,
		ps3_sas_port->remote_identify.sas_address);

	if (ps3_sas_port->remote_identify.device_type == SAS_END_DEVICE) {
		sas_rphy = sas_end_device_alloc(ps3_sas_port->port);
	} else {
		sas_rphy = sas_expander_alloc(ps3_sas_port->port,
			(enum sas_device_type)ps3_sas_port->remote_identify.device_type);
	}
	INJECT_START(PS3_ERR_IJ_SAS_ALLOC_PHY_NULL, &sas_rphy);
	if (unlikely(sas_rphy == NULL)) {
		LOG_ERROR("hno:%u alloc SAS rphy for sas_addr[%016llx] failed !\n",
			PS3_HOST(instance), ps3_sas_port->remote_identify.sas_address);
		goto l_out;
	}

	sas_rphy->identify = ps3_sas_port->remote_identify;

	if (ps3_sas_port->remote_identify.device_type == SAS_END_DEVICE) {
		sas_rphy->identify.target_port_protocols = SAS_PROTOCOL_NONE;
		pd_entry = ps3_dev_mgr_lookup_pd_info_by_id(instance,
			ps3_sas_port->pd_flat_id);
		if (unlikely(pd_entry == NULL)) {
			LOG_ERROR("hno:%u cannot find pd entry by pd_flat_id[%d] !\n",
				PS3_HOST(instance), ps3_sas_port->pd_flat_id);
			goto l_out;
		}
	}

	ret = sas_rphy_add(sas_rphy);
	INJECT_START(PS3_ERR_IJ_SAS_PHY_ADD_FAILED, &ret);
	if (unlikely(ret != 0)) {
		LOG_ERROR("hno:%u add SAS rphy for sas_addr[%016llx] failed !\n",
			PS3_HOST(instance), ps3_sas_port->remote_identify.sas_address);
		goto l_out;
	}

	ret = ps3_sas_end_device_add_past(instance, ps3_sas_port, pd_entry, sas_rphy);

l_out:
	ps3_sas_port->rphy = sas_rphy;
	LOG_DEBUG("hno:%u quit port rphy create, type[%d], pdflatid[%d], sas_addr[%016llx]\n",
		PS3_HOST(instance), ps3_sas_port->remote_identify.device_type,
		ps3_sas_port->pd_flat_id, ps3_sas_port->remote_identify.sas_address);
	return ret;
}

struct ps3_sas_node* ps3_sas_find_node_by_sas_addr(
	struct ps3_instance *instance, U64 sas_addr)
{
	struct ps3_sas_node *ps3_sas_node = NULL;
	struct ps3_sas_node *ret_node = NULL;
	ULong flags = 0;
	U8 i = 0;

	for (i = 0; i < PS3_SAS_HBA_MAX_SAS_NUM; i++) {
		if (instance->sas_dev_context.ps3_hba_sas_addr[i] == sas_addr) {
			ret_node = &instance->sas_dev_context.ps3_hba_sas;
			goto l_out;
		}
	}

	spin_lock_irqsave(&instance->sas_dev_context.ps3_sas_node_lock, flags);
	list_for_each_entry(ps3_sas_node,
			&instance->sas_dev_context.ps3_sas_node_list, list) {
		if (ps3_sas_node->sas_address == sas_addr) {
			ret_node = ps3_sas_node;
			break;
		}
	}
	spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);
l_out:
	return ret_node;
}

static void ps3_sanity_check_clean(struct ps3_instance *instance,
	struct ps3_sas_node *sas_node, U64 sas_address, struct ps3_sas_port *sas_port)
{
	struct ps3_sas_port *ps3_sas_port = NULL;
	struct ps3_sas_port *ps3_sas_port_next = NULL;
	struct ps3_sas_phy *ps3_sas_phy = NULL;
	struct ps3_sas_phy *ps3_sas_phy_next = NULL;
	struct ps3_sas_node *exp_node = NULL;
	ULong flags = 0;
	struct ps3_pd_entry *pd_entry = NULL;

	list_for_each_entry_safe(ps3_sas_port, ps3_sas_port_next,
			&sas_node->sas_port_list, list) {
		if (ps3_sas_port == sas_port) {
			continue;
		}
		list_for_each_entry_safe(ps3_sas_phy, ps3_sas_phy_next,
				&ps3_sas_port->phy_list, port_siblings) {
			if (ps3_sas_phy->remote_identify.sas_address !=
				sas_address) {
				continue;
			}

			LOG_WARN("hno:%u phy[%d] sas_addr[%016llx] == new device SAS addr\n",
				PS3_HOST(instance), ps3_sas_phy->phy_id,
				ps3_sas_port->remote_identify.sas_address);

			spin_lock_irqsave(&instance->sas_dev_context.ps3_sas_node_lock, flags);
			list_del(&ps3_sas_phy->port_siblings);
			ps3_sas_phy->attach_port = NULL;
			spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);
			ps3_sas_port->phy_count--;
			sas_port_delete_phy(ps3_sas_port->port, ps3_sas_phy->phy);
		}

		if (ps3_sas_port->phy_count != 0){
			continue;
		}

		if (ps3_sas_port->remote_identify.device_type ==
			SAS_EDGE_EXPANDER_DEVICE ||
			ps3_sas_port->remote_identify.device_type ==
			SAS_FANOUT_EXPANDER_DEVICE) {

			exp_node = ps3_sas_find_node_by_sas_addr(instance,
				ps3_sas_port->remote_identify.sas_address);
			if (exp_node == NULL) {
				LOG_ERROR("hno:%u cannot find node sas_addr[%016llx] !\n",
					PS3_HOST(instance), ps3_sas_port->remote_identify.sas_address);
				PS3_BUG();
				continue;
			}

			ps3_sas_expander_node_del(instance, exp_node);

		}else if (ps3_sas_port->remote_identify.device_type ==
			SAS_END_DEVICE) {

			spin_lock_irqsave(&instance->sas_dev_context.ps3_sas_node_lock, flags);
			list_del(&ps3_sas_port->list);
			spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);

			LOG_INFO("hno:%u sas_port pdid[%u] delete start, by r SAS addr[0x%llx], change\n",
				PS3_HOST(instance), ps3_sas_port->pd_flat_id, sas_address);

			sas_port_delete(ps3_sas_port->port);

			pd_entry = ps3_dev_mgr_lookup_pd_info_by_id(instance, ps3_sas_port->pd_flat_id);
			if (pd_entry != NULL) {
				pd_entry->sas_rphy = NULL;
			}

			LOG_INFO("hno:%u sas_port pdid[%u] delete end, by r SAS addr[0x%llx], change\n",
				PS3_HOST(instance), ps3_sas_port->pd_flat_id, sas_address);
			kfree(ps3_sas_port);
		}
	}

}

static S32 ps3_sas_port_create(struct ps3_instance *instance,
	struct ps3_sas_node *sas_node, struct ps3_sas_port *ps3_sas_port)
{
	S32 ret = -PS3_FAILED;
	ULong flags = 0;

	ps3_sas_port->port = ps3_sas_sas_port_create(instance, sas_node,
		&ps3_sas_port->phy_list);
	if (unlikely(ps3_sas_port->port == NULL)) {
		LOG_ERROR("hno:%u cannot add port on parent[%d] !\n",
			PS3_HOST(instance), sas_node->encl_id);

		goto l_out;
	}

	ret = ps3_sas_port_rphy_create(instance, ps3_sas_port);
	if (unlikely(ret != PS3_SUCCESS)) {
		LOG_ERROR("hno:%u create rphy[%016llx] on parent[%d] NOK!\n",
			PS3_HOST(instance), ps3_sas_port->remote_identify.sas_address,
			sas_node->encl_id);
		goto l_failed;
	}

	spin_lock_irqsave(&instance->sas_dev_context.ps3_sas_node_lock, flags);
	list_add_tail(&ps3_sas_port->list, &sas_node->sas_port_list);
	spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);

	goto l_out;
l_failed:
	if (ps3_sas_port->port != NULL) {
		sas_port_delete(ps3_sas_port->port);
		ps3_sas_port->port = NULL;
	}
l_out:
	return ret;
}

static void ps3_sas_port_del(struct ps3_instance *instance,
	struct ps3_sas_port *ps3_sas_port)
{
	struct ps3_sas_phy *ps3_sas_phy = NULL;
	struct ps3_sas_phy *ps3_sas_phy_next = NULL;
	ULong flags = 0;
	struct ps3_pd_entry *pd_entry = NULL;

	(void)instance;

	spin_lock_irqsave(&instance->sas_dev_context.ps3_sas_node_lock, flags);
	list_del(&ps3_sas_port->list);
	spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);

	list_for_each_entry_safe(ps3_sas_phy, ps3_sas_phy_next,
			&ps3_sas_port->phy_list, port_siblings) {

		sas_port_delete_phy(ps3_sas_port->port, ps3_sas_phy->phy);

		list_del(&ps3_sas_phy->port_siblings);
		ps3_sas_phy->attach_port = NULL;
		ps3_sas_port->phy_count--;
	}

	LOG_INFO("hno:%u sas_port pdid[%u] phy_count[%u] delete start\n",
		PS3_HOST(instance), ps3_sas_port->pd_flat_id, ps3_sas_port->phy_count);
	sas_port_delete(ps3_sas_port->port);

	pd_entry = ps3_dev_mgr_lookup_pd_info_by_id(instance, ps3_sas_port->pd_flat_id);
	if (pd_entry != NULL) {
		pd_entry->sas_rphy = NULL;
	}

	LOG_WARN("hno:%u sas_port pdid[%u] phy_count[%u] delete end\n",
		PS3_HOST(instance), ps3_sas_port->pd_flat_id, ps3_sas_port->phy_count);

	kfree(ps3_sas_port);

}

static S32 ps3_sas_end_device_try_add(struct ps3_instance *instance,
	struct ps3_sas_node *sas_node, struct ps3_sas_phy *ps3_phy,
	struct ps3_pd_entry *pd_entry)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_sas_port *ps3_sas_port = NULL;

	LOG_DEBUG("hno:%u ready add end device[%016llx] on node[%d]\n",
		PS3_HOST(instance), ps3_phy->remote_identify.sas_address,
		sas_node->encl_id);

	ps3_sas_port = ps3_sas_port_find(instance, sas_node,
		ps3_phy->remote_identify.sas_address);

	ps3_sanity_check_clean(instance, sas_node, ps3_phy->remote_identify.sas_address, ps3_sas_port);

	if (ps3_sas_port != NULL) {
		LOG_DEBUG("hno:%u find exist port[%016llx] on node[%d]\n",
			PS3_HOST(instance), ps3_phy->remote_identify.sas_address,
			sas_node->encl_id);
		if (ps3_is_sata_end_device(pd_entry) &&
			ps3_sas_port->pd_flat_id != PS3_PDID(&pd_entry->disk_pos)) {
			LOG_INFO("hno:%u sata end device [%u:%u] not exist but in port\n",
				PS3_HOST(instance), PS3_CHANNEL(&pd_entry->disk_pos),
				PS3_TARGET(&pd_entry->disk_pos));
			ps3_sas_port_del(instance, ps3_sas_port);
			ps3_sas_port = NULL;
		} else {
			ps3_sas_port_phy_add(instance, ps3_sas_port, ps3_phy);
			goto l_out;
		}
	}

	ps3_sas_port = (struct ps3_sas_port *)ps3_kzalloc(instance, sizeof(struct ps3_sas_port));
	INJECT_START(PS3_ERR_IJ_SAS_PORT_ALLOC_NULL, &ps3_sas_port);
	if (ps3_sas_port == NULL) {
		LOG_ERROR("hno:%u alloc PS3 port on node[%d] failed !\n",
			PS3_HOST(instance), sas_node->encl_id);
		ret = -PS3_FAILED;
		goto l_out;
	}

	INIT_LIST_HEAD(&ps3_sas_port->phy_list);
	ps3_sas_port->remote_identify = ps3_phy->remote_identify;
	list_add_tail(&ps3_phy->port_siblings, &ps3_sas_port->phy_list);
	ps3_phy->attach_port = ps3_sas_port;
	ps3_sas_port->phy_count++;

	ps3_sas_port->pd_flat_id = pd_entry->disk_pos.diskDev.ps3Dev.phyDiskID;
	ret = ps3_sas_port_create(instance, sas_node, ps3_sas_port);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u create end device rphy[%016llx] on parent[%d] NOK!\n",
			PS3_HOST(instance), ps3_phy->remote_identify.sas_address, sas_node->encl_id);
		goto l_failed;
	}
	pd_entry->sas_rphy = ps3_sas_port->rphy;

	goto l_out;
l_failed:
	kfree(ps3_sas_port);

l_out:
	LOG_DEBUG("hno:%u add end device[%016llx] on node[%d] end\n",
		PS3_HOST(instance), ps3_phy->remote_identify.sas_address,
		sas_node->encl_id);

	return ret;
}

static S32 ps3_sas_node_all_phys_add(struct ps3_instance *instance,
	struct ps3_sas_node *sas_node)
{
	S32 ret = PS3_SUCCESS;
	struct PS3SasMgr sas_req_param;
	struct PS3PhyInfo *phy_info = instance->sas_dev_context.ps3_sas_phy_buff;
	U8 i = 0;

	memset(&sas_req_param, 0, sizeof(sas_req_param));
	sas_req_param.enclID = sas_node->encl_id;
	sas_req_param.startPhyID = 0;
	sas_req_param.phyCount = sas_node->phy_count;
	sas_req_param.sasAddr = sas_node->sas_address;

	memset(phy_info, 0, PS3_SAS_REQ_BUFF_LEN);
	ret = ps3_sas_phy_get(instance, &sas_req_param);
	INJECT_START(PS3_ERR_IJ_SAS_PHY_GET_ERR1, &ret);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u get encl[%d] all phys info NOK\n",
			PS3_HOST(instance), sas_req_param.enclID);
		goto l_out;
	}

	for (i = 0; i < sas_node->phy_count; i++) {
		ret = ps3_sas_node_phy_add(instance, &sas_node->phys[i],
			sas_node, &phy_info[i]);
		if (ret != PS3_SUCCESS) {
			LOG_ERROR("hno:%u add node[%d]-phys[%d] NOK !\n",
				PS3_HOST(instance), sas_node->encl_id, i);
			goto l_out;
		}
#if 0
		不能再此添加，需要通过scsi那边进行添加，否则无法定序，分配正确的channel
		if ((phy_info[i].attachedSasAddr != PS3_SAS_INVALID_SAS_ADDR &&
				phy_info[i].attachDevType == SAS_END_DEVICE) &&
				phy_info[i].attachedSasAddr !=
				sas_node->parent_sas_address) {
			ret = ps3_sas_end_device_try_add(instance, sas_node,
				&sas_node->phys[i]);
			if (ret != PS3_SUCCESS) {
				LOG_ERROR("hno:%u add end device[%016llx] on node[%d]-phys[%d] failed !\n",
					PS3_HOST(instance),
					phy_info[i].attachedSasAddr,
					sas_node->encl_id, i);
				goto l_out;
			}
		}
#endif
	}
l_out:
	return ret;
}

static S32 ps3_sas_hba_node_init(struct ps3_instance *instance,
	struct PS3ExpanderInfo *exp_info, U64 *hba_sas_addr)
{
	S32 ret = PS3_SUCCESS;
	U8 i = 0;
	struct ps3_sas_node *hba_node =
		&instance->sas_dev_context.ps3_hba_sas;

	LOG_DEBUG("hno:%u enter !\n", PS3_HOST(instance));

	for (i = 0; i < PS3_SAS_HBA_MAX_SAS_NUM; i++) {
		instance->sas_dev_context.ps3_hba_sas_addr[i] =
			le64_to_cpu(hba_sas_addr[i]);
		LOG_INFO("hno:%u hba SAS addr[%d] is [%016llx] !\n",
			PS3_HOST(instance), i,
			instance->sas_dev_context.ps3_hba_sas_addr[i]);
	}

	hba_node->encl_id = exp_info->enclID;
	hba_node->phy_count = exp_info->phyCount;
	hba_node->dev_type = exp_info->devType;

	LOG_INFO("hno:%u hba encl_id[%d], phy_count[%d], dev_type[%d] !\n",
		PS3_HOST(instance), hba_node->encl_id, hba_node->phy_count,
		hba_node->dev_type);

	if (hba_node->phys == NULL) {
		hba_node->phys = (struct ps3_sas_phy*)ps3_kcalloc(instance, hba_node->phy_count,
			sizeof(struct ps3_sas_phy));
	}
	if (hba_node->phys == NULL) {
		LOG_ERROR("hno:%u alloc hba phys buffer failed !\n",
			PS3_HOST(instance));
		goto l_out;
	}

	ret = ps3_sas_node_all_phys_add(instance, hba_node);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u hba add phys NOK !\n",
			PS3_HOST(instance));
		goto l_out;
	}

l_out:
	LOG_DEBUG("hno:%u quit !\n", PS3_HOST(instance));
	return ret;
}

static struct ps3_sas_node* ps3_sas_find_node_by_id(struct ps3_instance *instance,
	U8 encl_id)
{
	struct ps3_sas_node *ps3_sas_node = NULL;
	ULong flags = 0;

	if (instance->sas_dev_context.ps3_hba_sas.encl_id == encl_id) {
		return &instance->sas_dev_context.ps3_hba_sas;
	}

	spin_lock_irqsave(&instance->sas_dev_context.ps3_sas_node_lock, flags);
	list_for_each_entry(ps3_sas_node,
			&instance->sas_dev_context.ps3_sas_node_list, list) {
		if (ps3_sas_node->encl_id == encl_id) {
			spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);
			return ps3_sas_node;
		}
	}
	spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);

	return NULL;
}

static struct ps3_sas_port* ps3_sas_expander_parent_attach(struct ps3_instance *instance,
	struct ps3_sas_node *exp_node)
{
	S32 ret = -PS3_FAILED;
	struct ps3_sas_node *parent_node = NULL;
	struct ps3_sas_port *ps3_sas_port = NULL;
	U8 i = 0;
	ULong flags = 0;
	Bool is_exist_port = PS3_TRUE;

	parent_node = ps3_sas_find_node_by_id(instance,
		exp_node->parent_encl_id);
	if (parent_node == NULL) {
		LOG_ERROR("hno:%u cannot find parent node[%d] !\n",
			PS3_HOST(instance), exp_node->parent_encl_id);
		goto l_out;
	}

	ps3_sas_port = ps3_sas_port_find(instance, parent_node, exp_node->sas_address);
	if (ps3_sas_port == NULL) {
		ps3_sas_port = (struct ps3_sas_port*)ps3_kzalloc(instance, sizeof(struct ps3_sas_port));
		INJECT_START(PS3_ERR_IJ_SAS_PORT_ALLOC_NULL1, &ps3_sas_port);
		if (ps3_sas_port == NULL) {
			LOG_ERROR("hno:%u alloc PS3 port on node[%d] failed !\n",
				PS3_HOST(instance), parent_node->encl_id);
			goto l_out;
		}
		INIT_LIST_HEAD(&ps3_sas_port->phy_list);
		is_exist_port = PS3_FALSE;
	}

	ps3_sanity_check_clean(instance, parent_node, exp_node->sas_address, ps3_sas_port);

	spin_lock_irqsave(&instance->sas_dev_context.ps3_sas_node_lock, flags);
	for (i = 0; i < parent_node->phy_count; i++) {
		if ((parent_node->phys[i].remote_identify.sas_address ==
				exp_node->sas_address) &&
			(parent_node->phys[i].attach_port == NULL)) {
			if (ps3_sas_port->phy_count == 0) {
				ps3_sas_port->remote_identify =
					parent_node->phys[i].remote_identify;
			}
			list_add_tail(&parent_node->phys[i].port_siblings,
				&ps3_sas_port->phy_list);
			parent_node->phys[i].attach_port = ps3_sas_port;
			ps3_sas_port->phy_count++;
			if (is_exist_port && ps3_sas_port->port != NULL) {
				sas_port_add_phy(ps3_sas_port->port, parent_node->phys[i].phy);
			}
		}
	}
	spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);

	if (!is_exist_port) {
		if (ps3_sas_port->phy_count == 0) {
			LOG_ERROR("hno:%u cannot find phy in parent[%d] !\n",
				PS3_HOST(instance), parent_node->encl_id);
			goto l_failed;
		}

		ret = ps3_sas_port_create(instance, parent_node, ps3_sas_port);
		if (ret != PS3_SUCCESS) {
			LOG_ERROR("hno:%u create rphy on parent[%d] for exp[%d] !\n",
				PS3_HOST(instance), parent_node->encl_id, exp_node->encl_id);
			goto l_failed;
		}

		exp_node->dev = &ps3_sas_port->rphy->dev;
	}
l_out:
	return ps3_sas_port;

l_failed:
	kfree(ps3_sas_port);

	return NULL;
}

static S32 ps3_sas_expander_node_add(struct ps3_instance *instance,
	struct PS3ExpanderInfo *exp_info)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_sas_node *exp_node = NULL;
	struct ps3_sas_port *ps3_sas_port = NULL;
	Bool is_exist_expander = PS3_TRUE;
	ULong flags = 0;
	U8 old_parent_id = PS3_SAS_INVALID_ID;

	LOG_DEBUG("hno:%u enter !\n", PS3_HOST(instance));

	if ((exp_node = ps3_sas_find_node_by_id(instance, exp_info->enclID)) != NULL &&
		unlikely(exp_node->parent_sas_address != exp_info->parentSasAddr ||
			exp_node->parent_encl_id != exp_info->parentId)) {
		LOG_WARN("hno:%u SAS node encl_id[%d], change place, "
			"parent sas_addr old[0x%016llx]-new[0x%016llx], parent_encl_id old[%d]=new[%d]\n",
			PS3_HOST(instance), exp_info->enclID,
			exp_node->parent_sas_address,
			exp_info->parentSasAddr, exp_node->parent_encl_id,
			exp_info->parentId);
		old_parent_id = exp_node->parent_encl_id;
		ps3_sas_expander_node_del(instance, exp_node);
		ps3_sas_expander_event_update(instance, old_parent_id);
		exp_node = NULL;
	}

	if (exp_node == NULL) {
		exp_node = (struct ps3_sas_node*)ps3_kzalloc(instance, sizeof(struct ps3_sas_node));
		INJECT_START(PS3_ERR_IJ_ALLOC_EXP_NODE_NULL, &exp_node);
		if (exp_node == NULL) {
			LOG_ERROR("hno:%u alloc node[%d] failed !\n",
				PS3_HOST(instance), exp_info->enclID);
			goto l_out;
		}

		exp_node->sas_address = le64_to_cpu(exp_info->sasAddr);
		exp_node->encl_id = exp_info->enclID;
		exp_node->phy_count = exp_info->phyCount;
		exp_node->dev_type = exp_info->devType;
		exp_node->parent_encl_id = exp_info->parentId;
		exp_node->parent_sas_address = le64_to_cpu(exp_info->parentSasAddr);

		exp_node->phys = (struct ps3_sas_phy*)ps3_kcalloc(instance, exp_node->phy_count,
			sizeof(struct ps3_sas_phy));
		INJECT_START(PS3_ERR_IJ_ALLOC_EXP_PHYS_NULL, &exp_node->phys);
		if (exp_node->phys == NULL) {
			LOG_ERROR("hno:%u alloc exp[%d] phys buffer failed !\n",
				PS3_HOST(instance), exp_node->encl_id);
			goto l_failed;
		}
		INIT_LIST_HEAD(&exp_node->sas_port_list);
		is_exist_expander = PS3_FALSE;
	}

	LOG_INFO("hno:%u ready add exp_node sas_address[0x%016llx],"
		" encl_id[%d], phy_count[%d], parent_encl_id[%d],"
		" parent_sas_address[0x%016llx] !\n",
		PS3_HOST(instance), exp_node->sas_address, exp_node->encl_id,
		exp_node->phy_count, exp_node->parent_encl_id,
		exp_node->parent_sas_address);

	ps3_sas_port = ps3_sas_expander_parent_attach(instance, exp_node);
	if (ps3_sas_port == NULL) {
		LOG_ERROR("hno:%u attch exp[%d] on parent NOK !\n",
			PS3_HOST(instance), exp_node->encl_id);
		ret = -PS3_FAILED;
		goto l_failed;
	}
	if (!is_exist_expander) {
		ret = ps3_sas_node_all_phys_add(instance, exp_node);
		if (ret != PS3_SUCCESS) {
			LOG_ERROR("hno:%u expander[%d] add phys NOK !\n",
				PS3_HOST(instance), exp_node->encl_id);
			goto l_failed;
		}

		spin_lock_irqsave(&instance->sas_dev_context.ps3_sas_node_lock, flags);
		list_add_tail(&exp_node->list,&instance->sas_dev_context.ps3_sas_node_list);
		spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);
		LOG_WARN("hno:%u add exp_node sas_address[0x%016llx],"
			" encl_id[%d], phy_count[%d], parent_encl_id[%d],"
			" parent_sas_address[0x%016llx] end!\n",
			PS3_HOST(instance), exp_node->sas_address, exp_node->encl_id,
			exp_node->phy_count, exp_node->parent_encl_id,
			exp_node->parent_sas_address);
	}
	goto l_out;

l_failed:
	if (ps3_sas_port != NULL) {
		ps3_sas_port_del(instance, ps3_sas_port);
		ps3_sas_port = NULL;
	}

	if (exp_node->phys != NULL) {
		kfree(exp_node->phys);
		exp_node->phys = NULL;
	}

	if (exp_node != NULL) {
		kfree(exp_node);
		exp_node = NULL;
	}
l_out:
	LOG_DEBUG("hno:%u quit !\n", PS3_HOST(instance));
	return ret;
}

static void ps3_sas_expander_del_list_build(struct ps3_instance *instance,
	struct list_head *node_del_list)
{
	struct ps3_sas_node *exp_node = NULL;
	struct ps3_sas_node *child_exp_node = NULL;
	struct ps3_sas_port *ps3_sas_port = NULL;
	ULong flags = 0;

	list_for_each_entry(exp_node, node_del_list, list) {
		list_for_each_entry(ps3_sas_port, &exp_node->sas_port_list, list) {
			if ((ps3_sas_port->remote_identify.device_type !=
				SAS_EDGE_EXPANDER_DEVICE) &&
				(ps3_sas_port->remote_identify.device_type !=
					SAS_FANOUT_EXPANDER_DEVICE)) {
				continue;
			}

			child_exp_node = ps3_sas_find_node_by_sas_addr(
				instance,
				ps3_sas_port->remote_identify.sas_address);
			if (child_exp_node != NULL) {
				spin_lock_irqsave(&instance->sas_dev_context.
					ps3_sas_node_lock, flags);
				list_move_tail(&child_exp_node->list,
					node_del_list);
				spin_unlock_irqrestore(&instance->sas_dev_context.
					ps3_sas_node_lock, flags);
			}
		}
	}
}

static void ps3_sas_node_del(struct ps3_instance *instance,
	struct ps3_sas_node *exp_node)
{
	struct ps3_sas_port *ps3_sas_port = NULL;
	struct ps3_sas_port *tmp_port = NULL;

	list_for_each_entry_safe(ps3_sas_port, tmp_port,
			&exp_node->sas_port_list, list) {

		ps3_sas_port_del(instance, ps3_sas_port);
		ps3_sas_port = NULL;
	}

	kfree(exp_node->phys);
}

static S32 ps3_sas_node_port_del(struct ps3_instance *instance,
	struct ps3_sas_node *parent_node, U64 sas_addr)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_sas_port *ps3_port = NULL;
	ps3_port = ps3_sas_port_find(instance, parent_node, sas_addr);
	if (ps3_port == NULL) {
		LOG_ERROR("hno:%u cannot find port[%016llx] in node[%d] !\n",
			PS3_HOST(instance), sas_addr, parent_node->encl_id);
		ret = -PS3_FAILED;
		goto l_out;
	}

	ps3_sas_port_del(instance, ps3_port);
	ps3_port = NULL;
l_out:
	return ret;
}

void ps3_sas_expander_node_del(struct ps3_instance *instance,
	struct ps3_sas_node *exp_node)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_sas_node *parent_node = NULL;
	struct ps3_sas_node *del_exp_node = NULL;
	struct ps3_sas_node *tmp_exp_node = NULL;
	struct list_head node_del_list = {NULL, NULL};
	U64 sas_address = exp_node->sas_address;
	ULong flags = 0;

	LOG_INFO("hno:%u enter !, encl_id[%d], parent_id[%d], sas_addr[%016llx]\n", PS3_HOST(instance),
		exp_node->encl_id, exp_node->parent_encl_id, exp_node->sas_address);

	parent_node = ps3_sas_find_node_by_id(instance,
		exp_node->parent_encl_id);
	if (parent_node == NULL) {
		LOG_ERROR("hno:%u cannot find parent node[%d] !\n",
			PS3_HOST(instance), exp_node->parent_encl_id);
		BUG();
		goto l_out;
	}

	INIT_LIST_HEAD(&node_del_list);
	spin_lock_irqsave(&instance->sas_dev_context.ps3_sas_node_lock, flags);
	list_move_tail(&exp_node->list, &node_del_list);
	spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);
	ps3_sas_expander_del_list_build(instance, &node_del_list);

	list_for_each_entry_safe_reverse(del_exp_node, tmp_exp_node,
			&node_del_list, list) {
		LOG_DEBUG("hno:%u remove encl_id[%d], sas_addr[%016llx]\n", PS3_HOST(instance),
			del_exp_node->encl_id, del_exp_node->sas_address);
		ps3_sas_node_del(instance, del_exp_node);
		list_del(&del_exp_node->list);
		kfree(del_exp_node);
	}

	LOG_WARN("hno:%u remove sas_addr[%016llx] from parent[%d]\n", PS3_HOST(instance),
		sas_address, parent_node->encl_id);

	ret = ps3_sas_node_port_del(instance, parent_node,
		sas_address);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u cannot delete expander[%016llx] from parent[%d] !\n",
			PS3_HOST(instance), sas_address,
			parent_node->encl_id);
		BUG();
		goto l_out;
	}
l_out:
	LOG_DEBUG("hno:%u quit !\n", PS3_HOST(instance));
}

S32 ps3_sas_device_data_init(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct PS3Expanders *p_expanders =
		(struct PS3Expanders *)instance->sas_dev_context.ps3_sas_buff;
	struct PS3ExpanderInfo *exp_info = p_expanders->expanders;
	U8 i = 0;

	if (!ps3_sas_is_support_smp(instance)) {
		goto l_out;
	}

	LOG_DEBUG("hno:%u ready get init expander enter\n",
		PS3_HOST(instance));



	memset(p_expanders, 0, PS3_SAS_REQ_BUFF_LEN);
	ret = ps3_sas_expander_all_get(instance);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u init get expander NOK\n",
			PS3_HOST(instance));
		goto l_out;
	}

	LOG_INFO("hno:%u get expander list count[%d]\n",
		PS3_HOST(instance), p_expanders->count);

	INJECT_START(PS3_ERR_HBA_PHY_COUNT_ZERO, &p_expanders->count);
	if (p_expanders->count == 0) {
		LOG_ERROR("hno:%u expander init info count = 0\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;;
	}

	ret = ps3_sas_hba_node_init(instance, exp_info, p_expanders->hbaSasAddr);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u init hba info NOK\n",
			PS3_HOST(instance));
		goto l_out;
	}

	for (i = 1; i < p_expanders->count; i++) {
		ret = ps3_sas_expander_node_add(instance, &exp_info[i]);
		if (ret != PS3_SUCCESS) {
			LOG_ERROR("hno:%u init add expander[%d] info NOK\n",
				PS3_HOST(instance), exp_info[i].enclID);
			goto l_out;
		}
	}

	LOG_DEBUG("hno:%u SAS init end\n",
		PS3_HOST(instance));
l_out:
	return ret ;
}

S32 ps3_sas_device_data_exit(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;

	if (!ps3_sas_is_support_smp(instance)) {
		goto l_out;
	}

	LOG_INFO("hno:%u ps3_sas_device_data_exit\n",
		PS3_HOST(instance));


	(void)instance;
l_out:
	return ret;
}

static S32 ps3_sas_expander_dma_buf_alloc(struct ps3_instance *instance)
{
	struct ps3_sas_dev_context *ps3_sas_ctx = &instance->sas_dev_context;

	ps3_sas_ctx->ps3_sas_buff = ps3_dma_alloc_coherent(instance,
		PS3_SAS_REQ_BUFF_LEN,
		&ps3_sas_ctx->ps3_sas_buff_dma_addr);

	if (ps3_sas_ctx->ps3_sas_buff == NULL) {
		LOG_ERROR("hno:%u alloc SAS req buffer failed !\n",
			PS3_HOST(instance));
		goto l_fail;
	}
	return PS3_SUCCESS;
l_fail:
	return -PS3_ENOMEM;
}

static void ps3_sas_expander_dma_buf_free(struct ps3_instance *instance)
{
	struct ps3_sas_dev_context *ps3_sas_ctx = &instance->sas_dev_context;

	if (ps3_sas_ctx->ps3_sas_buff != NULL) {
		ps3_dma_free_coherent(instance, PS3_SAS_REQ_BUFF_LEN,
			ps3_sas_ctx->ps3_sas_buff,
			ps3_sas_ctx->ps3_sas_buff_dma_addr);

		ps3_sas_ctx->ps3_sas_buff = NULL;
	}
}

static S32 ps3_sas_phy_dma_buf_alloc(struct ps3_instance *instance)
{
	struct ps3_sas_dev_context *ps3_sas_ctx = &instance->sas_dev_context;

	ps3_sas_ctx->ps3_sas_phy_buff =
		(struct PS3PhyInfo *)ps3_dma_alloc_coherent(instance,
		PS3_SAS_REQ_BUFF_LEN,
		&ps3_sas_ctx->ps3_sas_phy_buff_dma_addr);

	if (ps3_sas_ctx->ps3_sas_phy_buff == NULL) {
		LOG_ERROR("hno:%u alloc SAS req buffer failed !\n",
			PS3_HOST(instance));
		goto l_fail;
	}
	return PS3_SUCCESS;
l_fail:
	return -PS3_ENOMEM;
}

static void ps3_sas_phy_dma_buf_free(struct ps3_instance *instance)
{
	struct ps3_sas_dev_context *ps3_sas_ctx = &instance->sas_dev_context;

	if (ps3_sas_ctx->ps3_sas_phy_buff != NULL) {
		ps3_dma_free_coherent(instance, PS3_SAS_REQ_BUFF_LEN,
			ps3_sas_ctx->ps3_sas_phy_buff,
			ps3_sas_ctx->ps3_sas_phy_buff_dma_addr);

		ps3_sas_ctx->ps3_sas_phy_buff = NULL;
	}
}

S32 ps3_sas_device_mgr_init(struct ps3_instance *instance)
{
	S32 ret = -PS3_FAILED;
	U8 i = 0;

	if (instance->ioc_adpter->sas_transport_get != NULL) {
		instance->sas_dev_context.is_support_smp = PS3_TRUE;
	} else {
		instance->sas_dev_context.is_support_smp = PS3_FALSE;
		ret = PS3_SUCCESS;
		LOG_INFO("hno:%u the IOC is not support SAS expander !\n",
			PS3_HOST(instance));
		goto l_out;
	}

	sema_init(&instance->sas_dev_context.ps3_sas_smp_semaphore, 1);

	ret = ps3_sas_expander_dma_buf_alloc(instance);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u alloc SAS expander dma buffer failed !\n",
			PS3_HOST(instance));
		goto l_out;
	}

	ret = ps3_sas_phy_dma_buf_alloc(instance);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u alloc SAS phy dma buffer failed !\n",
			PS3_HOST(instance));
		goto l_out;
	}

	INIT_LIST_HEAD(&instance->sas_dev_context.ps3_sas_node_list);
	spin_lock_init(&instance->sas_dev_context.ps3_sas_node_lock);

	memset(&instance->sas_dev_context.ps3_hba_sas, 0,
		sizeof(struct ps3_sas_node));

	for (i = 0; i < PS3_SAS_HBA_MAX_SAS_NUM; i++) {
		instance->sas_dev_context.ps3_hba_sas_addr[i] =
			PS3_SAS_INVALID_SAS_ADDR;
	}

	INIT_LIST_HEAD(&instance->sas_dev_context.ps3_hba_sas.sas_port_list);
	instance->sas_dev_context.ps3_hba_sas.parent_encl_id =
		PS3_SAS_INVALID_ID;
	instance->sas_dev_context.ps3_hba_sas.parent_sas_address =
		PS3_SAS_INVALID_SAS_ADDR;
	instance->sas_dev_context.ps3_hba_sas.dev =
		&instance->host->shost_gendev;
l_out:
	return ret ;
}

S32 ps3_sas_device_mgr_exit(struct ps3_instance *instance)
{
	if (ps3_sas_is_support_smp(instance)) {
		ps3_sas_phy_dma_buf_free(instance);
		ps3_sas_expander_dma_buf_free(instance);
	}

	return PS3_SUCCESS;
}


static void ps3_sas_end_dev_del(struct ps3_instance *instance,
	struct ps3_sas_node* parent_node, U8 phy_id)
{
	struct ps3_sas_port *ps3_sas_port = NULL;
	struct ps3_sas_port *ps3_sas_port_next = NULL;
	struct ps3_sas_phy *ps3_sas_phy = NULL;
	struct ps3_sas_phy *ps3_sas_phy_next = NULL;
	Bool del_finish = PS3_FALSE;
	ULong flags = 0;
	struct ps3_pd_entry *pd_entry = NULL;

	(void)instance;

	list_for_each_entry_safe(ps3_sas_port, ps3_sas_port_next, &parent_node->sas_port_list, list) {
		list_for_each_entry_safe(ps3_sas_phy, ps3_sas_phy_next, &ps3_sas_port->phy_list, port_siblings) {
			if (likely(ps3_sas_phy->phy_id == phy_id)) {
				spin_lock_irqsave(&instance->sas_dev_context.ps3_sas_node_lock, flags);
				list_del(&ps3_sas_phy->port_siblings);
				ps3_sas_phy->attach_port = NULL;
				spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);

				sas_port_delete_phy(ps3_sas_port->port, ps3_sas_phy->phy);

				ps3_sas_port->phy_count--;

				LOG_DEBUG("hno:%u sas_port pdid[%u] phy_count[%u] -> "
					"encl[%u] phy_id[%u] sas_addr[%016llx] sas_remote_addr[%016llx] sas_phy delete\n",
					PS3_HOST(instance), ps3_sas_port->pd_flat_id, ps3_sas_port->phy_count,
					ps3_sas_phy->encl_id, ps3_sas_phy->phy_id,
					ps3_sas_phy->identify.sas_address,
					ps3_sas_phy->remote_identify.sas_address);

				memset(&ps3_sas_phy->remote_identify, 0,
					sizeof(struct sas_identify));
				del_finish = PS3_TRUE;
				break;
			}
		}

		if (del_finish == PS3_TRUE) {
			if (ps3_sas_port->phy_count == 0) {
				spin_lock_irqsave(&instance->sas_dev_context.ps3_sas_node_lock, flags);
				list_del(&ps3_sas_port->list);
				spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);

				LOG_INFO("hno:%u sas_port pdid[%u] phy_count[%u] delete start\n",
					PS3_HOST(instance), ps3_sas_port->pd_flat_id, ps3_sas_port->phy_count);

				sas_port_delete(ps3_sas_port->port);

				pd_entry = ps3_dev_mgr_lookup_pd_info_by_id(instance, ps3_sas_port->pd_flat_id);
				if (pd_entry != NULL) {
					pd_entry->sas_rphy = NULL;
				}

				LOG_INFO("hno:%u sas_port pdid[%u] phy_count[%u] delete end\n",
					PS3_HOST(instance), ps3_sas_port->pd_flat_id, ps3_sas_port->phy_count);
				kfree(ps3_sas_port);
			}

			break;
		}
	}
}

static Bool ps3_sas_addr_is_exist(
	struct PS3Expanders *p_expanders, U64 sas_addr)
{
	Bool ret = PS3_FALSE;
	U8 i = 0;

	if (p_expanders == NULL) {
		goto l_out;
	}
	for (i = 1; i < p_expanders->count; i++) {
		if (p_expanders->expanders[i].sasAddr == sas_addr) {
			ret = PS3_TRUE;
			break;
		}
	}
l_out:
	return ret;
}

static void ps3_sas_expander_port_clean(struct ps3_instance *instance,
							struct ps3_sas_node *sas_node, struct PS3Expanders *p_expanders)
{
	struct ps3_sas_port *ps3_sas_port = NULL;
	struct ps3_sas_port *ps3_sas_port_next = NULL;
	struct ps3_sas_phy *ps3_sas_phy = NULL;
	struct ps3_sas_phy *ps3_sas_phy_next = NULL;
	struct ps3_sas_node *exp_node = NULL;
	Bool is_exist = PS3_FALSE;
	U8 diff_phy_num = 0;

	list_for_each_entry_safe(ps3_sas_port, ps3_sas_port_next,
			&sas_node->sas_port_list, list) {
		if (ps3_sas_port->remote_identify.device_type !=
				SAS_EDGE_EXPANDER_DEVICE &&
			ps3_sas_port->remote_identify.device_type !=
				SAS_FANOUT_EXPANDER_DEVICE) {
			continue;
		}
		is_exist = ps3_sas_addr_is_exist(p_expanders, ps3_sas_port->remote_identify.sas_address);
		diff_phy_num = ps3_sas_port->phy_count;
		list_for_each_entry_safe(ps3_sas_phy, ps3_sas_phy_next,
				&ps3_sas_port->phy_list, port_siblings) {

			if ((ps3_sas_phy->remote_identify.sas_address ==
				ps3_sas_port->remote_identify.sas_address) ||
				(ps3_sas_phy->remote_identify.sas_address == 0 && is_exist)) {
				continue;
			}

			diff_phy_num--;
			LOG_WARN("hno:%u phy[%d]'s remote "
				"addr[%016llx] != expander "
				"port's rphy addr[0x%016llx]!\n",
				PS3_HOST(instance), ps3_sas_phy->phy_id,
				ps3_sas_phy->remote_identify.sas_address,
				ps3_sas_port->remote_identify.sas_address);
		}
		if (diff_phy_num != 0) {
			continue;
		}

		exp_node = ps3_sas_find_node_by_sas_addr(instance,
			ps3_sas_port->remote_identify.sas_address);
		if (exp_node == NULL) {
			LOG_ERROR("hno:%u cannot find node sas_addr[%016llx] !\n",
				PS3_HOST(instance), ps3_sas_port->remote_identify.sas_address);
			PS3_BUG();
		} else {
			LOG_INFO("hno:%u del expander in sas port clean. encl_id[%d], sas_addr[%016llx]\n",
					PS3_HOST(instance), exp_node->encl_id, exp_node->sas_address);
			ps3_sas_expander_node_del(instance, exp_node);
		}
	}
}

S32 ps3_sas_expander_phys_refresh(struct ps3_instance *instance,
	struct ps3_sas_node *sas_node)
{
	S32 ret = -PS3_FAILED;
	struct PS3SasMgr sas_req_param;
	struct PS3PhyInfo *phy_info = instance->sas_dev_context.ps3_sas_phy_buff;
	U8 i = 0;
	ULong flags = 0;

	memset(&sas_req_param, 0, sizeof(sas_req_param));
	sas_req_param.enclID = sas_node->encl_id;
	sas_req_param.sasAddr =cpu_to_le64(sas_node->sas_address);
	sas_req_param.startPhyID = 0;
	sas_req_param.phyCount = sas_node->phy_count;

	LOG_DEBUG("hno:%u ready get phys[%d] of encl_id[%d] !\n",
		PS3_HOST(instance), sas_req_param.phyCount, sas_req_param.enclID);

	memset(phy_info, 0, PS3_SAS_REQ_BUFF_LEN);
	ret = ps3_sas_phy_get(instance, &sas_req_param);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u init get expander NOK\n",
			PS3_HOST(instance));
		goto l_out;
	}

	spin_lock_irqsave(&instance->sas_dev_context.ps3_sas_node_lock, flags);
	LOG_INFO_IN_IRQ(instance, "hno:%u ready update %d phys of encl_id[%d]!\n",
		PS3_HOST(instance), sas_req_param.phyCount, sas_req_param.enclID);
	for (i = 0; i < sas_node->phy_count; i++) {
		ps3_sas_node_phy_update(instance, &sas_node->phys[i], &phy_info[i]);
#if 0
		所有的添加都应该通过scsi来触发
		if (phy_info[i].attachedSasAddr != PS3_SAS_INVALID_SAS_ADDR &&
				phy_info[i].attachDevType == SAS_END_DEVICE) {
			ret = ps3_sas_end_device_try_add(instance, sas_node,
				&sas_node->phys[i]);
			if (ret != PS3_SUCCESS) {
				LOG_ERROR("hno:%u add end device[%016llx] on node[%d]-phys[%d] failed !\n",
					PS3_HOST(instance),
					phy_info[i].attachedSasAddr,
					sas_node->encl_id, i);
				goto l_out;
			}
		}
#endif
	}
	spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);
	ps3_sas_port_phy_update(instance, sas_node);
l_out:
	return ret;
}

static S32 ps3_sas_device_date_refresh(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct PS3Expanders *p_expanders = (struct PS3Expanders*)instance->sas_dev_context.ps3_sas_buff;
	struct PS3ExpanderInfo *exp_info = p_expanders->expanders;
	struct ps3_sas_node *exp_node = NULL;
	U8 i = 0;

	LOG_DEBUG("hno:%u SAS dev refresh enter\n",
		PS3_HOST(instance));

	memset(p_expanders, 0, PS3_SAS_REQ_BUFF_LEN);
	ret = ps3_sas_expander_all_get(instance);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u init get expander NOK\n",
			PS3_HOST(instance));
		goto l_out;
	}

	LOG_DEBUG("hno:%u ready refresh expander count[%d]\n",
		PS3_HOST(instance), p_expanders->count);
	if (p_expanders->count == 0) {
		LOG_ERROR("hno:%u expander init info count = 0\n",
			PS3_HOST(instance));
		BUG();
	}

	LOG_DEBUG("hno:%u ready refresh HBA\n", PS3_HOST(instance));

	ret = ps3_sas_expander_phys_refresh(instance,
		&instance->sas_dev_context.ps3_hba_sas);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u refresh phys on HBA NOK!\n",
			PS3_HOST(instance));
		goto l_out;
	}
	ps3_sas_expander_port_clean(instance, &instance->sas_dev_context.ps3_hba_sas, p_expanders);

	LOG_DEBUG("hno:%u end refresh HBA\n", PS3_HOST(instance));

	for (i = 1; i < p_expanders->count; i++) {
		exp_node = ps3_sas_find_node_by_id(instance, exp_info[i].enclID);
		if (exp_node != NULL) {
			if (likely(exp_node->parent_sas_address == exp_info[i].parentSasAddr &&
				exp_node->parent_encl_id == exp_info[i].parentId)) {
				LOG_DEBUG("hno:%u ready refresh expander[%d]\n",
					PS3_HOST(instance), exp_info[i].enclID);
				ret = ps3_sas_expander_phys_refresh(instance, exp_node);
				if (ret != PS3_SUCCESS) {
					LOG_ERROR("hno:%u refresh phys on expander[%d] NOK!\n",
						PS3_HOST(instance), exp_info[i].enclID);
					goto l_out;
				}
				ps3_sas_expander_port_clean(instance, exp_node, p_expanders);
				LOG_DEBUG("hno:%u end refresh expander[%d]\n",
					PS3_HOST(instance), exp_info[i].enclID);
			}
		}

		LOG_DEBUG("hno:%u ready add expander[%d]\n",
			PS3_HOST(instance), exp_info[i].enclID);
		ret = ps3_sas_expander_node_add(instance, &exp_info[i]);
		LOG_DEBUG("hno:%u end add expander[%d], ret[%d]\n",
			PS3_HOST(instance), exp_info[i].enclID, ret);
		if (ret != PS3_SUCCESS) {
			LOG_ERROR("hno:%u init add expander[%d] info NOK\n",
				PS3_HOST(instance), exp_info[i].enclID);
			goto l_out;
		}
	}

	LOG_DEBUG("hno:%u SAS refresh end\n",
		PS3_HOST(instance));
l_out:
	return ret ;
}

static S32 ps3_sas_expander_event_update(struct ps3_instance *instance, U8 encl_id)
{
	S32 ret = -PS3_FAILED;
	struct ps3_sas_node *sas_node = NULL;

	LOG_DEBUG("hno:%u enter SAS expander update encl_id[%d] !\n",
		PS3_HOST(instance), encl_id);
	sas_node = ps3_sas_find_node_by_id(instance, encl_id);
	if (sas_node == NULL) {
		LOG_ERROR("hno:%u cannot find PS3 node[%d] !\n",
			PS3_HOST(instance), encl_id);
		goto l_out;
	}

	ret = ps3_sas_expander_phys_refresh(instance, sas_node);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u refresh phys on node[%d] NOK!\n",
			PS3_HOST(instance), encl_id);
		goto l_out;
	}
l_out:
	LOG_DEBUG("hno:%u quit SAS expander update encl_id[%d] !\n",
		PS3_HOST(instance), encl_id);
	return ret;
}

static S32 ps3_sas_expander_phy_update(struct ps3_instance *instance, U8 encl_id, U8 phy_id)
{
	S32 ret = -PS3_FAILED;
	struct ps3_sas_node *sas_node = NULL;
	struct PS3SasMgr sas_req_param;
	struct PS3PhyInfo *phy_info = instance->sas_dev_context.ps3_sas_phy_buff;
	ULong flags = 0;

	LOG_DEBUG("hno:%u enter SAS expander update encl_id[%d] !\n",
		PS3_HOST(instance), encl_id);
	sas_node = ps3_sas_find_node_by_id(instance, encl_id);
	if (sas_node == NULL) {
		LOG_ERROR("hno:%u cannot find PS3 node[%d] !\n",
			PS3_HOST(instance), encl_id);
		goto l_out;
	}

	memset(&sas_req_param, 0, sizeof(sas_req_param));
	sas_req_param.enclID = sas_node->encl_id;
	sas_req_param.sasAddr =cpu_to_le64(sas_node->sas_address);
	sas_req_param.startPhyID = phy_id;
	sas_req_param.phyCount = 1;

	memset(phy_info, 0, PS3_SAS_REQ_BUFF_LEN);
	ret = ps3_sas_phy_get(instance, &sas_req_param);
	INJECT_START(PS3_ERR_IJ_SAS_PHY_GET_ERR, &ret);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u init get expander NOK\n",
			PS3_HOST(instance));
		goto l_out;
	}

	spin_lock_irqsave(&instance->sas_dev_context.ps3_sas_node_lock, flags);
	ps3_sas_node_phy_update(instance, &sas_node->phys[phy_id], phy_info);
	spin_unlock_irqrestore(&instance->sas_dev_context.ps3_sas_node_lock, flags);

l_out:
	LOG_DEBUG("hno:%u quit sas expander update encl_id[%d] !\n",
		PS3_HOST(instance), encl_id);
	return ret;
}

static S32 ps3_sas_expander_event_add(struct ps3_instance *instance, U8 encl_id)
{
	S32 ret = PS3_SUCCESS;
	struct PS3ExpanderInfo *exp_info =
		(struct PS3ExpanderInfo *)instance->sas_dev_context.ps3_sas_buff;
	struct PS3SasMgr sas_req_param;

	memset(&sas_req_param, 0, sizeof(sas_req_param));
	sas_req_param.enclID = encl_id;

	LOG_WARN("hno:%u enter sas expander add encl_id[%d] !\n",
		PS3_HOST(instance), encl_id);

	memset(exp_info, 0, PS3_SAS_REQ_BUFF_LEN);
	ret = ps3_sas_expander_get(instance, &sas_req_param);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u get expander[%d] NOK\n",
			PS3_HOST(instance), encl_id);
		goto l_out;
	}

	if (exp_info->phyCount == 0) {
		LOG_ERROR("hno:%u phy_count Invalid!\n", PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}
	INJECT_START(PS3_ERR_EXP_ID_ERR, &encl_id);
	if (exp_info->enclID != encl_id) {
		LOG_ERROR("hno:%u got expander[%d] is not [%d]\n",
			PS3_HOST(instance), exp_info->enclID, encl_id);
		PS3_BUG();
		ret = -PS3_FAILED;
		goto l_out;
	}

	ret = ps3_sas_expander_event_update(instance, exp_info->parentId);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u update parent encl[%d] NOK\n",
			PS3_HOST(instance), exp_info->parentId);
		goto l_out;
	}

	ret = ps3_sas_expander_node_add(instance, exp_info);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u add expander[%d] NOK\n",
			PS3_HOST(instance), encl_id);
		goto l_out;
	}
l_out:
	LOG_WARN("hno:%u quit sas expander add encl_id[%d] !\n",
		PS3_HOST(instance), encl_id);
	return ret;
}

static S32 ps3_sas_expander_event_del(struct ps3_instance *instance, U8 encl_id)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_sas_node *exp_node = NULL;
	U8 parentId = 0;

	LOG_WARN("hno:%u enter sas expander del encl_id[%d] !\n",
		PS3_HOST(instance), encl_id);

	exp_node = ps3_sas_find_node_by_id(instance, encl_id);
	if (exp_node == NULL) {
		LOG_ERROR("hno:%u cannot find node[%d] !\n",
			PS3_HOST(instance), encl_id);
		ret = -PS3_FAILED;
		goto l_out;
	}

	parentId = exp_node->parent_encl_id;
	ps3_sas_expander_node_del(instance, exp_node);

	ret = ps3_sas_expander_event_update(instance, parentId);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u update parent encl[%d] NOK\n",
			PS3_HOST(instance), parentId);
		goto l_out;
	}

l_out:
	LOG_WARN("hno:%u quit sas expander del encl_id[%d] !\n",
		PS3_HOST(instance), encl_id);
	return ret;
}

S32 ps3_sas_expander_event_refresh(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	ret = ps3_sas_device_date_refresh(instance);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u refresh all SAS NOK\n",
			PS3_HOST(instance));
		goto l_out;
	}

l_out:
	return ret;
}

S32 ps3_sas_update_detail_proc(struct ps3_instance *instance,
	struct PS3EventDetail *event_detail, U32 event_cnt)
{
	S32 ret = PS3_SUCCESS;
	S32 ret_map = PS3_SUCCESS;
	U32 i = 0;

	LOG_DEBUG("hno:%u, event detail count[%d]\n",
		PS3_HOST(instance), event_cnt);

	for (i = 0; i < event_cnt; i++) {
		LOG_INFO("hno:%u, event detail %d eventCode is [%s], encl_Id[%d]\n",
			PS3_HOST(instance), i, mgrEvtCodeTrans(event_detail[i].eventCode),
			event_detail[i].EnclId);

		switch (event_detail[i].eventCode) {
		case PS3_EVT_CODE(MGR_EVT_SAS_EXPANDER_CHANGE):
		case PS3_EVT_CODE(MGR_EVT_SAS_EXPANDER_IN):
			ret = ps3_sas_expander_event_add(instance,
				event_detail[i].EnclId);
			break;
		case PS3_EVT_CODE(MGR_EVT_SAS_EXPANDER_OUT):
			ret = ps3_sas_expander_event_del(instance,
				event_detail[i].EnclId);
			break;
		case PS3_EVT_CODE(MGR_EVT_SAS_EXPANDER_UPDATE):
			ret = ps3_sas_expander_event_update(instance,
				event_detail[i].EnclId);
			break;
		default:
			break;
		}

		if (ret != PS3_SUCCESS) {
			ret_map |= event_detail[i].eventType;
		}
	}
	return ret_map;
}

S32 ps3_sas_add_device(struct ps3_instance *instance, struct ps3_pd_entry *pd_entry)
{
	S32 ret = -PS3_FAILED;
	struct ps3_sas_node* parent_node = NULL;
	struct sas_identify *remote_id = NULL;

	U32 softChan  = PS3_CHANNEL(&pd_entry->disk_pos);
	U32 devID     = PS3_TARGET(&pd_entry->disk_pos);
	U32 phyDiskID = PS3_PDID(&pd_entry->disk_pos);

	LOG_DEBUG("hno:%u enter ps3_sas_add_device pd[%u:%u:%u] on encl[%u], phy[%u]!\n",
		PS3_HOST(instance), softChan, devID, phyDiskID,
		pd_entry->encl_id, pd_entry->phy_id);

	parent_node = ps3_sas_find_node_by_id(instance, pd_entry->encl_id);
	if (parent_node == NULL) {
		LOG_ERROR("hno:%u add end device channel[%u:%u:%u] cannot found node[%d] !\n",
			PS3_HOST(instance), softChan, devID, phyDiskID,
			pd_entry->encl_id);
		goto l_out;
	}
	if (pd_entry->phy_id >= parent_node->phy_count) {
		LOG_ERROR("hno:%u add end device channel[%u:%u:%u] phyid[%u] exceed node phy_count[%u] !\n",
			PS3_HOST(instance), softChan, devID, phyDiskID,
			pd_entry->phy_id, parent_node->phy_count);
		goto l_out;
	}

	ret = ps3_sas_expander_phy_update(instance, pd_entry->encl_id, pd_entry->phy_id);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u update phy in add pd channel[%u:%u:%u] phyid[%u] on node[%u] NOK !\n",
			PS3_HOST(instance), softChan, devID, phyDiskID,
			pd_entry->phy_id, parent_node->encl_id);
		goto l_out;
	}

	remote_id = &parent_node->phys[pd_entry->phy_id].remote_identify;
	if (remote_id->sas_address == PS3_SAS_INVALID_SAS_ADDR ||
		remote_id->device_type != SAS_END_DEVICE ||
		remote_id->sas_address == parent_node->parent_sas_address) {
		LOG_ERROR("hno:%u add end device channel[%u:%u:%u] phyid[%u] "
			"invalid SAS addr[%016llx] or dev_type[%d] != SAS_END_DEVICE or "
			" SAS addr = parent SAS addr[%016llx]!\n",
			PS3_HOST(instance), softChan, devID, phyDiskID,
			pd_entry->phy_id, remote_id->sas_address,
			remote_id->device_type,
			parent_node->parent_sas_address);
		goto l_out;
	}

	ret = ps3_sas_end_device_try_add(instance, parent_node,
		&parent_node->phys[pd_entry->phy_id], pd_entry);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u add end device channel[%u:%u:%u] phyid[%u] on node[%u] NOK !\n",
			PS3_HOST(instance), softChan, devID, phyDiskID,
			pd_entry->phy_id, parent_node->encl_id);
		goto l_out;
	}

l_out:
	return ret;
}

S32 ps3_sas_remove_device(struct ps3_instance *instance,
	struct PS3DiskDevPos *diskPos, U8 encl_id, U8 phy_id)
{
	S32 ret = -PS3_FAILED;
	struct ps3_sas_node* parent_node = NULL;
	struct sas_identify *remote_id = NULL;
	U32 softChan  = PS3_CHANNEL(diskPos);
	U32 devID     = PS3_TARGET(diskPos);
	U32 phyDiskID = PS3_PDID(diskPos);

	LOG_WARN("hno:%u enter ps3_sas_remove_device pd[%u:%u:%u] on encl[%u], phy[%u]!\n",
		PS3_HOST(instance), softChan,
		devID, phyDiskID,  encl_id, phy_id);

	parent_node = ps3_sas_find_node_by_id(instance, encl_id);
	if (parent_node == NULL) {
		LOG_ERROR("hno:%u add end device channel[%u:%u:%u] cannot found node[%u] !\n",
 			PS3_HOST(instance),softChan, devID, phyDiskID,
			encl_id);
		goto l_out;
	}

	if (phy_id > parent_node->phy_count) {
		LOG_ERROR("hno:%u add end device channel[%u:%u:%u] phyid[%u] exceed node phy_count[%u] !\n",
			PS3_HOST(instance), softChan, devID, phyDiskID,
			phy_id, parent_node->phy_count);
		goto l_out;
	}

	remote_id = &parent_node->phys[phy_id].remote_identify;
	if (remote_id->sas_address != PS3_SAS_INVALID_SAS_ADDR &&
		remote_id->sas_address == parent_node->parent_sas_address) {
		LOG_ERROR("hno:%u add end device channel[%u:%u:%u] phyid[%u] "
			"SAS addr[%016llx] dev_type[%d] parent SAS addr[%016llx]!\n",
			PS3_HOST(instance), softChan, devID, phyDiskID,
			phy_id, remote_id->sas_address,
			remote_id->device_type,
			parent_node->parent_sas_address);
		goto l_out;
	}

	ps3_sas_end_dev_del(instance, parent_node, phy_id);

	ret = ps3_sas_expander_phy_update(instance, encl_id, phy_id);
	if (ret != PS3_SUCCESS) {
		LOG_ERROR("hno:%u update phy in add pd channel[%u:%u:%u] phyid[%u] on node[%u] NOK !\n",
			PS3_HOST(instance), softChan, devID, phyDiskID, phy_id, encl_id);
		goto l_out;
	}
l_out:
	LOG_WARN("hno:%u ps3_sas_remove_device pd[%u:%u:%u] on encl[%u], phy[%u] end, ret[%d]!\n",
		PS3_HOST(instance), softChan, devID, phyDiskID,
		encl_id, phy_id, ret);
	return ret;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,17,0)
	S32 ps3_sas_user_scan(struct Scsi_Host *host, U32 channel, U32 id, U32 lun)
#else
	S32 ps3_sas_user_scan(struct Scsi_Host *host, U32 channel, U32 id, U64 lun)
#endif
{
	struct PS3ChannelInfo *channel_info = NULL;
	struct ps3_instance *instance = NULL;
	struct ps3_pd_entry *pd_entry = NULL;
	U16 max_dev_num = 0;
	U8 i = 0;
	U16 j = 0;

	if ((lun != 0) && (lun != SCAN_WILD_CARD)) {
		goto l_out;
	}

	instance = (struct ps3_instance *)shost_priv(host);
	channel_info = &instance->ctrl_info.channelInfo;
	
	ps3_mutex_lock(&instance->dev_context.dev_scan_lock);
	for (i = 0; i < channel_info->channelNum; i++) {
		if ((channel_info->channels[i].channelType == PS3_CHAN_TYPE_PD) &&
			(channel == SCAN_WILD_CARD || channel == i)) {
			max_dev_num = channel_info->channels[i].maxDevNum;
			for (j = 0; j < max_dev_num; j++) {
				pd_entry = ps3_dev_mgr_lookup_pd_info(instance, i, j);
				if ((pd_entry != NULL) && (pd_entry->sas_rphy != NULL)) {
					if (id == SCAN_WILD_CARD || id == j) {
						scsi_scan_target(&pd_entry->sas_rphy->dev, i, j, 0, SCSI_SCAN_MANUAL);
						LOG_DEBUG("hno:%u channel:%u target:%u scan\n", PS3_HOST(instance), i, j);
					}
				}
			}
		}
	}
	ps3_mutex_unlock(&instance->dev_context.dev_scan_lock);

l_out:
	return 0;
}

#endif
