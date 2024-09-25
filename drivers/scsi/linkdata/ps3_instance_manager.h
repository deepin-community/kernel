
#ifndef _PS3_INSTANCE_MANAGER_H_
#define _PS3_INSTANCE_MANAGER_H_

#ifndef  _WINDOWS
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/irqreturn.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include "ps3_sas_transport.h"
#include "ps3_device_manager_sas.h"
#else
#include "ps3_pci.h"

#endif

#include "ps3_inner_data.h"
#include "ps3_irq.h"
#include "ps3_cmd_channel.h"
#include "ps3_platform_utils.h"
#include "ps3_cmd_channel.h"
#include "ps3_inner_data.h"
#include "ps3_debug.h"
#include "ps3_irq.h"
#include "ps3_recovery.h"
#include "ps3_dump.h"
#include "ps3_cmd_stat_def.h"
#include "ps3_watchdog.h"
#include "ps3_err_inject.h"
#include "ps3_qos.h"

enum PS3_INSTANCE_STATE_TYPE {
	PS3_INSTANCE_STATE_INIT			= 0,
	PS3_INSTANCE_STATE_READY		= 1,
	PS3_INSTANCE_STATE_PRE_OPERATIONAL	= 2,
	PS3_INSTANCE_STATE_OPERATIONAL		= 3,
	PS3_INSTANCE_STATE_SOFT_RECOVERY	= 4,
	PS3_INSTANCE_STATE_RECOVERY		= 5,
	PS3_INSTANCE_STATE_SUSPEND		= 6,
	PS3_INSTANCE_STATE_DEAD			= 7,
	PS3_INSTANCE_STATE_QUIT			= 8,
	PS3_INSTANCE_STATE_PCIE_RECOVERY = 9,
	PS3_INSTANCE_STATE_COUNT		= 10
};
enum PS3_DEVICE_ERR_HANDLE_STATE_TYPE {
	PS3_DEVICE_ERR_STATE_NORMAL			= 0,
	PS3_DEVICE_ERR_STATE_CLEAN			= 1,
	PS3_DEVICE_ERR_STATE_INIT			= 2,
};

struct ps3_instance_state_machine {
	ps3_mutex lock;
	ps3_atomic32 state;
	Bool is_load;	
	Bool is_suspend; 
	Bool is_poweroff; 
	Bool is_pci_err_recovery; 
	Bool can_hostreset; 
};

static inline const S8 *namePS3InstanceState(S32 s)
{
	static const S8 *myNames[] = {
		[PS3_INSTANCE_STATE_INIT]		= "PS3_INSTANCE_STATE_INIT",
		[PS3_INSTANCE_STATE_READY]		= "PS3_INSTANCE_STATE_READY",
		[PS3_INSTANCE_STATE_PRE_OPERATIONAL]	= "PS3_INSTANCE_STATE_PRE_OPERATIONAL",
		[PS3_INSTANCE_STATE_OPERATIONAL]	= "PS3_INSTANCE_STATE_OPERATIONAL",
		[PS3_INSTANCE_STATE_SOFT_RECOVERY]	= "PS3_INSTANCE_STATE_SOFT_RECOVERY",
		[PS3_INSTANCE_STATE_RECOVERY]		= "PS3_INSTANCE_STATE_RECOVERY",
		[PS3_INSTANCE_STATE_SUSPEND]		= "PS3_INSTANCE_STATE_SUSPEND",
		[PS3_INSTANCE_STATE_DEAD]		= "PS3_INSTANCE_STATE_DEAD",
		[PS3_INSTANCE_STATE_QUIT]		= "PS3_INSTANCE_STATE_QUIT",
		[PS3_INSTANCE_STATE_PCIE_RECOVERY] = "PS3_INSTANCE_STATE_PCIE_RECOVERY"
	};

	if (s >= PS3_INSTANCE_STATE_COUNT) {
		return "PS3_INSTANCE_STATE_INVALID";
	}

	return myNames[s];
}

struct ps3_recovery_function {
	S32 (*recovery_handle_cb)(struct ps3_instance *instance, U8 reason);
	S32 (*hardreset_handle_pre_cb)(struct ps3_instance *instance);
	S32 (*hardreset_handle_wait_ready_cb)(struct ps3_instance *instance);
	S32 (*hardreset_handle_init_running_cb)(struct ps3_instance *instance);
	S32 (*hardreset_handle_post_cb)(struct ps3_instance *instance);
	S32 (*hardreset_handle_finish_cb)(struct ps3_instance *instance);
	S32 (*hardreset_handle_offline_cb)(struct ps3_instance *instance);
	S32 (*softreset_handle_pre_cb)(struct ps3_instance *instance);
	S32 (*softreset_handle_post_cb)(struct ps3_instance *instance);
	S32 (*halt_handle_cb)(struct ps3_instance *instance);
};

struct ps3_instance {
	ps3_list_head list_item;
	struct ps3_instance *peer_instance;
#ifndef  _WINDOWS
	struct pci_dev *pdev;
	struct Scsi_Host *host;
#else
	ULong bus_number;
	struct ps3_pci_context pci_dev_context;
#endif
	Ps3Fifo_s __iomem *reg_set;
	ps3_atomic32 watchdog_reg_read_fail_count; 

	struct ps3_cmd_context cmd_context;
	struct ps3_irq_context irq_context;
	struct ps3_dev_context dev_context;
#ifndef  _WINDOWS
	struct ps3_sas_dev_context sas_dev_context;
#endif
	struct ps3_event_context event_context;
    struct ps3_webSubscribe_context webSubscribe_context;

	struct ps3_fault_context fault_context;

	struct ps3_watchdog_context watchdog_context;
	struct ps3_dump_context dump_context;
	struct ps3_debug_context debug_context; 
	struct ps3_cmd_statistics_context cmd_statistics; 
	struct PS3IocCtrlInfo ctrl_info;
	struct PS3IocCtrlInfo *ctrl_info_buf;

#ifdef  _WINDOWS
	ps3_atomic32 ioctl_count;
#else
	struct semaphore ioctl_sem;
#endif
	struct ps3_ioc_adp_template *ioc_adpter; 
	struct ps3_cmd_attr_context cmd_attr; 
	struct PS3MgrEvent event_req_info;  

	ps3_spinlock req_queue_lock;   

	dma_addr_t ctrl_info_buf_h;
	dma_addr_t drv_info_buf_phys; 
	U8 *drv_info_buf;              
	dma_addr_t host_mem_info_buf_phys; 
	U8 *host_mem_info_buf;         
	U32 max_mgr_cmd_total_count;   
	U32 max_mgr_cmd_count;         
	U32 max_task_cmd_count;        
	U32 min_intr_count;            
	U8 reserve[4];
	U32 reply_fifo_depth_addition; 

	ULong reg_bar;
	Bool is_support_sync_cache; 
	Bool is_use_frontend_prp;   
	U8 dma_mask;
	Bool is_support_jbod;
	Bool use_clusting; 
	Bool is_adjust_register_count;
	Bool is_scan_host_finish;
	Bool is_probe_finish;
	Bool is_probe_failed;
	Bool is_suspend;	 
	Bool is_resume;
	Bool is_hard_reset;
	Bool is_pci_reset;
	Bool is_ioc_halt_support;
	Bool is_shallow_soft_recovery_support;
	Bool is_deep_soft_recovery_support;
	Bool is_hard_recovery_support;
	Bool is_halt_support_cli;
	Bool is_half_hard_reset;
	Bool is_host_added;
	Bool is_need_event;
	Bool is_raid1_direct_skip_mapblock_check;
	Bool is_single_disk_raid0_direct_skip_strip_check;
	Bool is_support_dump_ctrl;
	Bool is_support_io_limit;
	Bool task_manager_host_busy;
	U8 hilMode;      
	Bool is_irq_prk_support;
	Bool is_support_irq;
	Bool is_raid;
	Bool smp_affinity_enable;
	Bool msix_combined;
	U8 reserved[2];
	U16 unload_timeout;
	U16 wait_ready_timeout;
	U16 dev_id;
	U8 dma_addr_bit_pos;
	U8 pci_err_handle_state;
	const char *product_model;
	S64 __percpu *scsi_cmd_deliver; 

	U64 ioc_fw_version;
	ps3_mutex task_mgr_reset_lock;
#ifdef _WINDOWS
	STOR_DPC device_reset_dpc;
#endif
	Bool page_mode_change;
	U64 page_mode_addr_mask;
	ps3_atomic32 is_err_scsi_processing;
	ps3_atomic32 reg_op_count;
	ps3_atomic32 host_reset_processing;
	ps3_atomic32 watchdog_ref;
	struct ps3_instance_state_machine state_machine;
	struct ps3_recovery_context *recovery_context;	
	struct ps3_recovery_function recovery_function; 
#ifndef _WINDOWS
	struct work_struct recovery_irq_work;		
	struct workqueue_struct *recovery_irq_queue;	
	U8 recovery_irq_enable;	
	Bool is_print_special_log;
	U8 reserved2[2];
	U32 hard_dog_mask;
#endif
	volatile U32 hardreset_event;
	struct ps3_qos_context qos_context;
	ps3_mutex task_abort_lock;
	U8 r1x_mode; 
	U64 start_pfn; 
	U64 end_pfn; 
	U64 so_start_addr; 
	U64 so_end_addr;   
	S32 device_busy_threshold;
};

#ifndef _WINDOWS
struct ps3_mgmt_info {
	ps3_spinlock spin_lock_obj;
	ps3_list_head instance_list_head;
};

enum  {
	PS3_HOST_MACHINE_DEFAULT,
	PS3_HOST_MACHINE_X86 = 0,
	PS3_HOST_MACHINE_ARM,
	PS3_HOST_MACHINE_MIPS,
	PS3_HOST_MACHINE_COUNT,
};

enum  {
	PS3_HOST_VENDOR_DEFAULT,
	PS3_HOST_VENDOR_INTEL = 0,
	PS3_HOST_VENDOR_HYGON,
	PS3_HOST_VENDOR_AMD,
	PS3_HOST_VENDOR_COUNT,
};

struct ps3_host_info {
	U8 machine;
	U8 vendor;
	U16 cpu_cnt;
	U16 core_cnt;
	U16 processor_cnt;
};

struct ps3_mgmt_info* ps3_mgmt_info_get(void);

struct ps3_instance *ps3_instance_lookup(U16 host_no);

S32 ps3_instance_add(struct ps3_instance *instance);

S32 ps3_instance_remove(struct ps3_instance *instance);

void ps3_mgmt_info_init(void);

#endif

void ps3_instance_init(struct ps3_instance *instance);

S32 ps3_instance_state_transfer(struct ps3_instance *instance,
	U32 exp_cur_state, U32 dest_state);
S32 ps3_instance_no_lock_state_transfer(struct ps3_instance *instance,
	U32 dest_state);

void ps3_instance_state_transfer_to_dead(struct ps3_instance *instance);

void ps3_instance_state_transfer_to_pcie_recovery(struct ps3_instance *instance);

void ps3_instance_state_transfer_to_quit(struct ps3_instance *instance);

void ps3_instance_state_transfer_to_suspend(struct ps3_instance *instance);

void ps3_instance_state_transition_to_recovery(struct ps3_instance *instance);

S32 ps3_instance_wait_for_operational(struct ps3_instance *instance, Bool is_hardreset);
S32 ps3_instance_wait_for_hard_reset_flag_done(struct ps3_instance *instance);

S32 ps3_instance_wait_for_dead_or_pre_operational(struct ps3_instance *instance);

static inline Bool ps3_is_instance_state_normal(struct ps3_instance *instance)
{
	S32 cur_state = ps3_atomic_read(&instance->state_machine.state);
	INJECT_START(PS3_ERR_IJ_SAS_REQ_PRE_CHECK, &cur_state)
	if (cur_state != PS3_INSTANCE_STATE_OPERATIONAL &&
		cur_state != PS3_INSTANCE_STATE_PRE_OPERATIONAL &&
		cur_state != PS3_INSTANCE_STATE_SOFT_RECOVERY) {
		LOG_WARN_LIM("hno:%u instance exception state %s\n",
			PS3_HOST(instance), namePS3InstanceState(cur_state));
		return PS3_FALSE;
	}

	return PS3_TRUE;
}

static inline void ps3_pci_err_recovery_set(struct ps3_instance *instance, Bool state)
{
	instance->state_machine.is_pci_err_recovery = state;
}

static inline Bool ps3_pci_err_recovery_get(struct ps3_instance *instance)
{
	return instance->state_machine.is_pci_err_recovery;
}
static inline Bool ps3_need_block_hard_reset_request(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	if (ps3_pci_err_recovery_get(instance)) {
		LOG_WARN_LIM("hno[%u], host in pci recovery during reset request\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto end;
	}
	ps3_mutex_lock(&instance->state_machine.lock);
	if(instance->recovery_context->host_reset_state != PS3_HOST_RESET_INIT) {
		ps3_mutex_unlock(&instance->state_machine.lock);
		LOG_WARN_LIM("hno[%u], host in host reset during  reset request\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto end;
	}
	ps3_mutex_unlock(&instance->state_machine.lock);
end:
	return ((ret == PS3_SUCCESS) ? PS3_FALSE:PS3_TRUE);
}
static inline void ps3_need_wait_hard_reset_request(struct ps3_instance *instance)
{
	do {
		if (ps3_pci_err_recovery_get(instance)) {
			LOG_WARN_LIM("hno[%u], host in pci recovery during reset request\n",
				PS3_HOST(instance));
			ps3_msleep(100);
			continue;
		}
		ps3_mutex_lock(&instance->state_machine.lock);
		if(instance->recovery_context->host_reset_state != PS3_HOST_RESET_INIT) {
			ps3_mutex_unlock(&instance->state_machine.lock);
			LOG_WARN_LIM("hno[%u], host in host reset during  reset request\n",
				PS3_HOST(instance));
			ps3_msleep(100);
			continue;
		}
		ps3_mutex_unlock(&instance->state_machine.lock);
		break;
	} while (1);
}

static inline Bool ps3_state_is_normal(S32 cur_state)
{
	return (cur_state != PS3_INSTANCE_STATE_OPERATIONAL &&
		cur_state != PS3_INSTANCE_STATE_PRE_OPERATIONAL) ?
		PS3_FALSE : PS3_TRUE;
}

S32 ps3_instance_wait_for_normal(struct ps3_instance *instance);

S32 ps3_recovery_state_wait_for_normal(struct ps3_instance *instance);

struct ps3_ioc_adp_template {
	S32 (*io_cmd_build)(struct ps3_cmd *cmd); 
	S32 (*mgr_cmd_build)(struct ps3_instance *instance,
		struct ps3_cmd *cmd); 
	void (*init_cmd_send)(struct ps3_instance *instance,
		struct PS3CmdWord *cmd_word); 
	void (*cmd_send)(struct ps3_instance *instance,
		struct PS3CmdWord *cmd_word); 
	U32 (*ioc_state_get)(struct ps3_instance *instance);
	S32 (*ioc_init_state_to_ready)(struct ps3_instance *instance);
	S32 (*ioc_init_proc)(struct ps3_instance *instance);
	void (*ioc_resource_prepare)(struct ps3_instance *instance);
	S32 (*ioc_hard_reset)(struct ps3_instance *instance);
	S32 (*ioc_shallow_soft_reset)(struct ps3_instance *instance);
	S32 (*ioc_deep_soft_reset)(struct ps3_instance *instance);
	S32 (*ioc_force_to_fault)(struct ps3_instance *instance);
	S32 (*ioc_force_to_halt)(struct ps3_instance *instance);
	S32 (*irq_init)(struct ps3_instance *instance);
	void (*irq_enable)(struct ps3_instance *instance);
	void (*irq_disable)(struct ps3_instance *instance);
#ifndef _WINDOWS
	irqreturn_t (*isr)(int irq_no, void *data); 
	struct scsi_transport_template *(*sas_transport_get)(void);
#else
	U8(*isr)(void *instance, ULong irq_no); 
#endif
	void (*event_filter_table_get)(U8 *data);
	void (*reg_write)(struct ps3_instance *instance, U64 val, void __iomem *reg);
	U64 (*reg_read)(struct ps3_instance *instance, void __iomem *reg);
	Bool (*is_need_direct_to_normal)(const struct ps3_cmd *cmd);
	Bool (*max_replyq_count_get)(struct ps3_instance *instance,
		U32 *max_replyq_count);
	void (*check_vd_member_change)(struct ps3_instance *instance,
		struct ps3_pd_entry *local_entry);
	Bool (*scsih_stream_is_detect)(struct ps3_cmd *cmd);
	Bool (*scsih_stream_is_direct)(const struct ps3_cmd *cmd);
	U32 (*ioc_heartbeat_detect)(struct ps3_instance *instance);
	void __iomem * (*reg_set)(struct pci_dev *pdev, ULong reg_bar);
	Bool (*ioc_security_check)(struct ps3_instance *instance);
	void (*io_cmd_rebuild)(struct ps3_cmd *cmd); 
	Bool (*rw_cmd_is_need_split)(struct ps3_cmd *cmd);
	Bool (*write_direct_enable)(struct ps3_cmd *cmd);
};
#define PS3_DEVICE_IS_SWITCH(id) ((id == PCI_DEVICE_ID_PS3_SWITCH || \
        id == PCI_DEVICE_ID_PS3_SWITCH_FPGA))

#ifndef _WINDOWS
void ps3_ioc_adp_init(struct ps3_instance *instance, const struct pci_device_id *id);

void ps3_remove(struct pci_dev *pdev);

#else
void ps3_ioc_adp_init(struct ps3_instance *instance);
#endif

static inline S32 ps3_get_pci_function(struct pci_dev *pci)
{
	return (PCI_FUNC(pci->devfn));
}

static inline S32 ps3_get_pci_slot(struct pci_dev * pci)
{
	return (PCI_SLOT(pci->devfn));
}

static inline S32 ps3_get_pci_bus(struct pci_dev * pci)
{
	return (pci->bus->number);
}

static inline S32 ps3_get_pci_domain(struct pci_dev * pci)
{
	return pci_domain_nr(pci->bus);
}

static inline bool ps3_is_latest_func(struct ps3_instance *instance)
{
	bool ret = PS3_TRUE;
	struct ps3_instance *peer_instance = NULL;

	list_for_each_entry(peer_instance, &ps3_mgmt_info_get()->instance_list_head, list_item) {
		if ((peer_instance != NULL) &&
			(ps3_get_pci_domain(peer_instance->pdev) == ps3_get_pci_domain(instance->pdev)) &&
			(PCI_BUS_NUM(peer_instance->pdev->devfn) == PCI_BUS_NUM(instance->pdev->devfn)) &&
			(PCI_SLOT(peer_instance->pdev->devfn) == PCI_SLOT(instance->pdev->devfn)) &&
			(PCI_FUNC(peer_instance->pdev->devfn) != PCI_FUNC(instance->pdev->devfn))) {
				ret = PS3_FALSE;
		}
	}

	return ret;
}

static inline void ps3_get_so_addr_ranger(struct ps3_instance *instance,
	U64 addr, U32 offset)
{
	U64 so_end_addr = (addr + offset) - 1;
	if (instance->so_start_addr == 0 && instance->so_end_addr == 0) {
		instance->so_start_addr = addr;
		instance->so_end_addr = so_end_addr;
		goto l_out;
	}
	instance->so_start_addr = ((addr < instance->so_start_addr) ? addr : instance->so_start_addr);
	instance->so_end_addr = ((so_end_addr > instance->so_end_addr) ? so_end_addr : instance->so_end_addr);
l_out:
	return;
}

void ps3_host_info_get(void);

U16 ps3_host_vendor_get(void);

#endif
