#ifdef _WINDOWS
#include "ps3_def.h"
#endif

#include "ps3_instance_manager.h"
#include "ps3_ioc_state.h"
#include "ps3_irq.h"
#include "ps3_ioc_manager.h"
#include "ps3_scsih_cmd_parse.h"
#include "ps3_scsih.h"
#include "ps3_inner_data.h"
#include "ps3_event.h"
#include "ps3_device_update.h"
#include "ps3_pci.h"
#include "ps3_module_para.h"
#include "ps3_scsih.h"

#define  MAX_MGR_CMD_COUNT		(8)
#define  MAX_TASK_CMD_COUNT		(8)
#define  MIN_MSIX_INT_COUNT		(1)
#define  PS3_HARD_DOG_MASK       (0xF000000UL)

#define  SWITCH_MAX_MGR_CMD_COUNT		(12)
#define  SWITCH_MAX_TASK_CMD_COUNT		(2)
#define  SWITCH_MAX_MGR_CMD_TOTAL_COUNT	(14)
#define  SWITCH_MIN_MSI_INT_COUNT		(1)
#define  SWITCH_REPLY_FIFO_DEP_ADDITION	(1)
#define  PS3_HARD_DOG_MASK_SWITCH       (0x1000000UL)

struct ps3_ioc_adp_temp_entry {
	U16 device_id; 	
	struct ps3_ioc_adp_template *adp_template;
};

void ps3_ioc_resource_prepare_switch(struct ps3_instance *instance)
{
	struct PS3MgrEvent *event_req_info = &instance->event_req_info;

	memset(event_req_info, 0, sizeof(struct PS3MgrEvent));

	event_req_info->eventTypeMap = PS3_EVT_PD_COUNT;
	event_req_info->eventLevel = PS3_EVENT_LEVEL_INFO;
	event_req_info->eventTypeMapProcResult = PS3_EVT_ILLEGAL_TYPE;

	instance->max_mgr_cmd_total_count = SWITCH_MAX_MGR_CMD_TOTAL_COUNT;
	instance->max_mgr_cmd_count = SWITCH_MAX_MGR_CMD_COUNT;
	instance->max_task_cmd_count = SWITCH_MAX_TASK_CMD_COUNT;
	instance->min_intr_count = SWITCH_MIN_MSI_INT_COUNT;
	instance->reply_fifo_depth_addition = SWITCH_REPLY_FIFO_DEP_ADDITION;
	instance->is_support_jbod = PS3_TRUE;
	instance->use_clusting = PS3_TRUE;
	instance->is_use_frontend_prp = PS3_FALSE;
	instance->is_adjust_register_count = PS3_FALSE; 
	instance->is_probe_finish = PS3_FALSE;
	instance->is_probe_failed = PS3_FALSE;
	instance->is_suspend = PS3_FALSE;
	instance->is_resume = PS3_FALSE;
	instance->is_hard_reset = PS3_FALSE;
	instance->is_pci_reset = PS3_FALSE;
	instance->ioc_fw_version = 0;
	instance->hilMode = HIL_MODEL_SW;
	instance->unload_timeout = PS3_DEFAULT_MGR_CMD_TIMEOUT;
	instance->wait_ready_timeout = PS3_FW_STATE_TO_READY_TMO_LOOP_COUNT_SWITCH;
	instance ->is_half_hard_reset = PS3_FALSE;
	instance->is_need_event = PS3_FALSE;
	instance->is_raid1_direct_skip_mapblock_check = PS3_FALSE;
	instance->is_single_disk_raid0_direct_skip_strip_check = PS3_FALSE;
	instance->is_support_dump_ctrl = PS3_FALSE;
	instance->is_support_io_limit = PS3_FALSE;
	instance->is_irq_prk_support = PS3_FALSE;
	instance->is_support_irq = PS3_FALSE;
	instance->is_raid = PS3_FALSE;
	instance->hard_dog_mask = PS3_HARD_DOG_MASK_SWITCH;
	instance->task_manager_host_busy = PS3_FALSE;
	instance->is_print_special_log = PS3_FALSE;
	instance->r1x_mode = PS3_R1X_MODE_NORMAL;
	instance->smp_affinity_enable = ps3_smp_affinity_query();
	instance->page_mode_change =
		ps3_host_vendor_get() == PS3_HOST_VENDOR_INTEL ? PS3_FALSE : PS3_TRUE;
	instance->page_mode_addr_mask = PS3_PAGE_MODE_ABOVE_3_ADDR_MASK;
	ps3_mutex_init(&instance->task_mgr_reset_lock);
	ps3_mutex_init(&instance->task_abort_lock);
}

void ps3_ioc_resource_prepare_raid(struct ps3_instance *instance)
{
	struct PS3MgrEvent *event_req_info = &instance->event_req_info;

	memset(event_req_info, 0, sizeof(struct PS3MgrEvent));

	event_req_info->eventTypeMap =
		PS3_EVT_PD_COUNT |
		PS3_EVT_VD_COUNT |
		PS3_EVT_CTRL_INFO |
		PS3_EVT_PD_ATTR;

	event_req_info->eventLevel = PS3_EVENT_LEVEL_INFO;
	event_req_info->eventTypeMapProcResult = PS3_EVT_ILLEGAL_TYPE;

	instance->max_mgr_cmd_total_count = MAX_MGR_CMD_TOTAL_COUNT;
	instance->max_mgr_cmd_count = MAX_MGR_CMD_COUNT;
	instance->max_task_cmd_count = MAX_TASK_CMD_COUNT;
	instance->min_intr_count = MIN_MSIX_INT_COUNT;
	instance->is_support_jbod = PS3_TRUE;
	instance->use_clusting = (ps3_use_clustering_query() == PS3_TRUE);
	instance->is_use_frontend_prp = PS3_FALSE;
	instance->is_adjust_register_count = PS3_TRUE;
	instance->is_probe_finish = PS3_FALSE;
	instance->is_probe_failed = PS3_FALSE;
	instance->is_suspend = PS3_FALSE;
	instance->is_resume = PS3_FALSE;
	instance->is_hard_reset = PS3_FALSE;
	instance->is_pci_reset = PS3_FALSE;
	instance->ioc_fw_version = 0;
	instance->hilMode = HIL_MODEL_HW_ENHANCED;
	instance->unload_timeout = PS3_RAID_UNLOAD_MGR_CMD_TIMEOUT;
#ifdef PS3_HARDWARE_HAPS_V200
	instance->wait_ready_timeout = PS3_FW_STATE_TO_READY_TMO_LOOP_COUNT_RAID_HAPS;
#else
	instance->wait_ready_timeout = PS3_FW_STATE_TO_READY_TMO_LOOP_COUNT_RAID;
#endif
	instance ->is_half_hard_reset = PS3_FALSE;
	instance->is_need_event = PS3_TRUE;
	instance->is_raid1_direct_skip_mapblock_check = PS3_FALSE;
	instance->is_single_disk_raid0_direct_skip_strip_check = PS3_FALSE;
	instance->is_support_dump_ctrl = PS3_TRUE;
	instance->is_support_io_limit = PS3_TRUE;
	instance->is_irq_prk_support = PS3_FALSE;
	instance->is_support_irq = PS3_FALSE;
	instance->is_raid = PS3_TRUE;
	instance->hard_dog_mask = PS3_HARD_DOG_MASK;
	instance->is_print_special_log = PS3_FALSE;
	instance->r1x_mode = PS3_R1X_MODE_NORMAL;
	instance->task_manager_host_busy = PS3_FALSE;
	instance->smp_affinity_enable = ps3_smp_affinity_query();
	instance->page_mode_change =
		ps3_host_vendor_get() == PS3_HOST_VENDOR_INTEL ? PS3_FALSE : PS3_TRUE;
	instance->page_mode_addr_mask = PS3_PAGE_MODE_ABOVE_3_ADDR_MASK;
	ps3_mutex_init(&instance->task_mgr_reset_lock);
	ps3_mutex_init(&instance->task_abort_lock);
	ps3_raid_qos_prepare(instance);
}

void ps3_ioc_resource_prepare_hba(struct ps3_instance *instance)
{
	struct PS3MgrEvent *event_req_info = &instance->event_req_info;

	memset(event_req_info, 0, sizeof(struct PS3MgrEvent));

	event_req_info->eventTypeMap =
		PS3_EVT_PD_COUNT |
		PS3_EVT_VD_COUNT |
		PS3_EVT_CTRL_INFO |
		PS3_EVT_SAS_INFO |
		PS3_EVT_PD_ATTR;

	event_req_info->eventLevel = PS3_EVENT_LEVEL_INFO;
	event_req_info->eventTypeMapProcResult = PS3_EVT_ILLEGAL_TYPE;

	instance->max_mgr_cmd_total_count = MAX_MGR_CMD_TOTAL_COUNT;
	instance->max_mgr_cmd_count = MAX_MGR_CMD_COUNT;
	instance->max_task_cmd_count = MAX_TASK_CMD_COUNT;
	instance->min_intr_count = MIN_MSIX_INT_COUNT;
	instance->is_support_jbod = PS3_FALSE;
	instance->use_clusting = (ps3_use_clustering_query() == PS3_TRUE);
	instance->is_use_frontend_prp = PS3_TRUE;
	instance->is_adjust_register_count = PS3_TRUE;
	instance->is_probe_finish = PS3_FALSE;
	instance->is_probe_failed = PS3_FALSE;
	instance->is_suspend = PS3_FALSE;
	instance->is_resume = PS3_FALSE;
	instance->is_scan_host_finish = PS3_FALSE;
	instance->is_hard_reset = PS3_FALSE;
	instance->is_pci_reset = PS3_FALSE;
	instance->is_halt_support_cli = PS3_FALSE;
	ps3_atomic_set(&instance->reg_op_count, 0);
	instance->ioc_fw_version = 0;
	instance->hilMode = HIL_MODEL_HW_ENHANCED;
	instance->unload_timeout = PS3_UNLOAD_MGR_CMD_TIMEOUT;
	instance->wait_ready_timeout = PS3_FW_STATE_TO_READY_TMO_LOOP_COUNT_HBA;
	instance ->is_half_hard_reset = PS3_FALSE;
	instance->is_need_event = PS3_TRUE;
	instance->is_raid1_direct_skip_mapblock_check = PS3_TRUE;
	instance->is_single_disk_raid0_direct_skip_strip_check = PS3_TRUE;
	instance->is_support_dump_ctrl = PS3_TRUE;
	instance->is_support_io_limit = PS3_FALSE;
	instance->is_irq_prk_support = PS3_FALSE;
	instance->is_support_irq = PS3_FALSE;
	instance->is_raid = PS3_FALSE;
	instance->hard_dog_mask = PS3_HARD_DOG_MASK;
	instance->is_print_special_log = PS3_FALSE;
	instance->smp_affinity_enable = ps3_smp_affinity_query();
	ps3_atomic_set(&instance->host_reset_processing, 0);
	instance->task_manager_host_busy = PS3_FALSE;
	instance->r1x_mode = PS3_R1X_MODE_NORMAL;
	instance->page_mode_change =
		ps3_host_vendor_get() == PS3_HOST_VENDOR_INTEL ? PS3_FALSE : PS3_TRUE;
	instance->page_mode_addr_mask = PS3_PAGE_MODE_ABOVE_3_ADDR_MASK;
	ps3_mutex_init(&instance->task_mgr_reset_lock);
	ps3_mutex_init(&instance->task_abort_lock);
	ps3_hba_qos_prepare(instance);
}

static Bool ps3_replyq_count_raidhba_get(struct ps3_instance *instance,
	U32 *max_replyq_count)
{
	Bool ret = PS3_TRUE;
	if (!ps3_max_replyq_count_get(instance, max_replyq_count)) {
		LOG_ERROR("hno:%u  ps3_max_replyq_count_get NOK!\n",
			PS3_HOST(instance));
		ret = PS3_FALSE;
		goto l_out;
	}
	*max_replyq_count += PS3_RAIDHBA_NUM_ADJUST_VALUE;
	*max_replyq_count = *max_replyq_count > PS3_MAX_REPLY_QUE_COUNT ?
		PS3_MAX_REPLY_QUE_COUNT : *max_replyq_count;
l_out:
	return ret;
}

static Bool ps3_is_need_direct_to_normal_hba(const struct ps3_cmd *cmd)
{
	Bool ret = PS3_TRUE;

	if (ps3_direct_to_normal_query()) {
		ret = PS3_TRUE; 
		goto l_out;
	}

	if (cmd->io_attr.dev_type == PS3_DEV_TYPE_NVME_SSD) {
		ret = PS3_TRUE;
		goto l_out;
	}

	if (cmd->io_attr.dev_type != PS3_DEV_TYPE_VD) {
		ret = PS3_FALSE;
		goto l_out;
	}

	if (unlikely(cmd->io_attr.vd_entry == NULL)) {
		ret = PS3_FALSE;
		goto l_out;
	}

	if (cmd->io_attr.vd_entry->isNvme) {
		ret = PS3_TRUE;
		goto l_out;
	}

l_out:
	return ret;
}

static Bool ps3_is_need_direct_to_normal_raid(const struct ps3_cmd *cmd)
{
	(void)cmd;
	return PS3_TRUE;
}

static Bool ps3_is_need_direct_to_normal_switch(const struct ps3_cmd *cmd)
{
	(void)cmd;
	return PS3_FALSE;
}

struct ps3_ioc_adp_template g_ps3_template_switch = {
	.io_cmd_build              = ps3_scsih_cmd_build,
	.mgr_cmd_build             = NULL,
	.init_cmd_send             = ps3_switch_init_cmd_send,
	.cmd_send                  = ps3_switch_normal_cmd_send,
	.ioc_state_get             = ps3_ioc_state_get,
	.ioc_init_state_to_ready   = ps3_ioc_init_to_ready,
	.ioc_init_proc             = ps3_ioc_init_proc,
	.ioc_resource_prepare      = ps3_ioc_resource_prepare_switch,
	.ioc_hard_reset            = ps3_ioc_state_hard_reset,
	.ioc_shallow_soft_reset    = ps3_ioc_state_shallow_soft_reset,
	.ioc_deep_soft_reset       = ps3_ioc_state_deep_soft_reset,
	.ioc_force_to_fault        = ps3_ioc_state_force_to_fault,
	.ioc_force_to_halt         = ps3_ioc_state_force_to_halt,
	.irq_init                  = ps3_irqs_init_switch,
	.irq_enable                = ps3_irqs_enable,
	.irq_disable               = ps3_irqs_disable,
	.isr                       = ps3_irqs_service,
#ifndef _WINDOWS
	.sas_transport_get         = NULL,
#endif
	.event_filter_table_get    = ps3_event_filter_table_get_switch,
	.reg_write                 = ps3_reg_write_u64,
#ifdef _WINDOWS
	.reg_read                  = ps3_reg_read_u64,
#else
	.reg_read                  = ps3_switch_ioc_reg_read,
#endif
	.is_need_direct_to_normal  = ps3_is_need_direct_to_normal_switch,
	.max_replyq_count_get      = ps3_max_replyq_count_get,
	.check_vd_member_change    = NULL,
	.scsih_stream_is_detect    = NULL,
	.scsih_stream_is_direct    = NULL,
#ifdef PS3_HARDWARE_ASIC
	.ioc_heartbeat_detect	   = ps3_ioc_heartbeat_detect,
#else
	.ioc_heartbeat_detect	   = NULL,
#endif
	.reg_set                   = NULL,
	.ioc_security_check        = NULL,
	.io_cmd_rebuild            = ps3_scsih_direct_to_normal_req_frame_rebuild,
	.rw_cmd_is_need_split      = NULL,
	.write_direct_enable  = NULL,
};

struct ps3_ioc_adp_template g_ps3_template_hba = {
	.io_cmd_build              = ps3_scsih_cmd_build,
	.mgr_cmd_build             = NULL,


	.init_cmd_send             = ps3_switch_init_cmd_send,
	.cmd_send                  = ps3_ioc_cmd_send,
	.ioc_state_get             = ps3_ioc_state_get,
	.ioc_init_state_to_ready   = ps3_ioc_init_to_ready,
	.ioc_init_proc             = ps3_ioc_init_proc,
	.ioc_resource_prepare      = ps3_ioc_resource_prepare_hba,
	.ioc_hard_reset            = ps3_ioc_state_hard_reset,
	.ioc_shallow_soft_reset    = ps3_ioc_state_shallow_soft_reset,
	.ioc_deep_soft_reset       = ps3_ioc_state_deep_soft_reset,
	.ioc_force_to_fault        = ps3_ioc_state_force_to_fault,
	.ioc_force_to_halt         = ps3_ioc_state_force_to_halt,
	.irq_init                  = ps3_irqs_init,
	.irq_enable                = ps3_irqs_enable,
	.irq_disable               = ps3_irqs_disable,
	.isr                       = ps3_irqs_service,
#ifndef _WINDOWS
	.sas_transport_get         = ps3_sas_transport_get,
#endif
	.event_filter_table_get    = ps3_event_filter_table_get_hba,
	.reg_write                 = ps3_reg_write_u64,
	.reg_read                  = ps3_reg_read_u64,
	.is_need_direct_to_normal  = ps3_is_need_direct_to_normal_hba,
	.max_replyq_count_get      = ps3_replyq_count_raidhba_get,
	.check_vd_member_change    = ps3_check_vd_member_change,
	.scsih_stream_is_detect    = ps3_scsih_stream_is_detect,
	.scsih_stream_is_direct    = ps3_hba_scsih_stream_is_direct,
#ifdef PS3_HARDWARE_ASIC
	.ioc_heartbeat_detect = ps3_ioc_heartbeat_detect,
#else
	.ioc_heartbeat_detect = NULL,
#endif
	.reg_set                   = ps3_reg_set_ioremap,
	.ioc_security_check        = NULL,
	.io_cmd_rebuild            = ps3_scsih_direct_to_normal_req_frame_rebuild,
	.rw_cmd_is_need_split      = ps3_scsih_rw_cmd_is_need_split_hba,
	.write_direct_enable  = NULL,
};

struct ps3_ioc_adp_template g_ps3_template_raid = {
	.io_cmd_build              = ps3_scsih_cmd_build,
	.mgr_cmd_build             = NULL,

	.init_cmd_send             = ps3_switch_init_cmd_send,
	.cmd_send                  = ps3_ioc_cmd_send,
	.ioc_state_get             = ps3_ioc_state_get,
	.ioc_init_state_to_ready   = ps3_ioc_init_to_ready,
	.ioc_init_proc             = ps3_ioc_init_proc,
	.ioc_resource_prepare      = ps3_ioc_resource_prepare_raid,
	.ioc_hard_reset            = ps3_ioc_state_hard_reset,
	.ioc_shallow_soft_reset    = ps3_ioc_state_shallow_soft_reset,
	.ioc_deep_soft_reset       = ps3_ioc_state_deep_soft_reset,
	.ioc_force_to_fault        = ps3_ioc_state_force_to_fault,
	.ioc_force_to_halt         = ps3_ioc_state_force_to_halt,
	.irq_init                  = ps3_irqs_init,
	.irq_enable                = ps3_irqs_enable,
	.irq_disable               = ps3_irqs_disable,
	.isr                       = ps3_irqs_service,
#ifndef _WINDOWS
	.sas_transport_get         = NULL,
#endif
	.event_filter_table_get    = ps3_event_filter_table_get_raid,
	.reg_write                 = ps3_reg_write_u64,
	.reg_read                  = ps3_reg_read_u64,
	.is_need_direct_to_normal  = ps3_is_need_direct_to_normal_raid,
	.max_replyq_count_get      = ps3_replyq_count_raidhba_get,
	.check_vd_member_change	   = NULL,
	.scsih_stream_is_detect    = ps3_scsih_stream_is_detect,
	.scsih_stream_is_direct	   = ps3_raid_scsih_stream_is_direct,
#ifdef PS3_HARDWARE_ASIC
	.ioc_heartbeat_detect = ps3_ioc_heartbeat_detect,
#else
	.ioc_heartbeat_detect = NULL,
#endif
	.reg_set                   = ps3_reg_set_ioremap,
	.ioc_security_check        = ps3_ioc_security_state_check,
	.io_cmd_rebuild            = ps3_scsih_direct_to_normal_req_frame_rebuild,
	.rw_cmd_is_need_split      = ps3_scsih_rw_cmd_is_need_split_raid,
	.write_direct_enable       = ps3_write_direct_enable,
};

static struct ps3_ioc_adp_temp_entry ps3_ioc_adp_template_map[] = {
	{PCI_DEVICE_ID_PS3_RAID, &g_ps3_template_raid},
	{PCI_DEVICE_ID_PS3_HBA, &g_ps3_template_hba},
	{PCI_DEVICE_ID_PS3_SWITCH, &g_ps3_template_switch},
	{PCI_DEVICE_ID_PS3_SWITCH_FPGA, &g_ps3_template_switch},
	{PCI_DEVICE_ID_STARS_IOC_2020_18i, &g_ps3_template_hba},
	{PCI_DEVICE_ID_STARS_ROC_2020_10i, &g_ps3_template_raid},
	{PCI_DEVICE_ID_PS3_RAID_FPGA, &g_ps3_template_raid},
	{PCI_DEVICE_ID_STARS_IOC_2213_16i, &g_ps3_template_hba},
};

#ifndef _WINDOWS
void ps3_ioc_adp_init(struct ps3_instance *instance, const struct pci_device_id *id)
{
	U16 i = 0;
	U16 adp_num = sizeof(ps3_ioc_adp_template_map) / sizeof(struct ps3_ioc_adp_temp_entry);

	for (i = 0; i < adp_num; i++) {
		if (id->device == ps3_ioc_adp_template_map[i].device_id) {
			instance->ioc_adpter = ps3_ioc_adp_template_map[i].adp_template;
		}
	}

	instance->ioc_adpter->ioc_resource_prepare(instance);
}
#else
void ps3_ioc_adp_init(struct ps3_instance *instance)
{
	U16 i = 0;
	U16 adp_num = sizeof(ps3_ioc_adp_template_map) / sizeof(struct ps3_ioc_adp_temp_entry);

	for (i = 0; i < adp_num; i++) {
		if (instance->pci_dev_context.device_id == ps3_ioc_adp_template_map[i].device_id) {
			instance->ioc_adpter = ps3_ioc_adp_template_map[i].adp_template;
			LOG_WARN("hno:%u pci dev type is [%s]\n",
				PS3_HOST(instance),
				namePciDevType(ps3_ioc_adp_template_map[i].device_id));
#ifdef _WINDOWS
			instance->ioc_adpter->event_filter_table_get = ps3_event_filter_table_get_raid;
#endif

		}
	}

	instance->ioc_adpter->ioc_resource_prepare(instance);
}
#endif
