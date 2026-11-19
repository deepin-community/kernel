/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_INSTANCE_MANAGER_H_
#define _PS3_INSTANCE_MANAGER_H_

#ifndef _WINDOWS
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
#include "ps3_qos.h"

enum PS3_INSTANCE_STATE_TYPE {
	PS3_INSTANCE_STATE_INIT = 0,
	PS3_INSTANCE_STATE_READY = 1,
	PS3_INSTANCE_STATE_PRE_OPERATIONAL = 2,
	PS3_INSTANCE_STATE_OPERATIONAL = 3,
	PS3_INSTANCE_STATE_SOFT_RECOVERY = 4,
	PS3_INSTANCE_STATE_RECOVERY = 5,
	PS3_INSTANCE_STATE_SUSPEND = 6,
	PS3_INSTANCE_STATE_DEAD = 7,
	PS3_INSTANCE_STATE_QUIT = 8,
	PS3_INSTANCE_STATE_PCIE_RECOVERY = 9,
	PS3_INSTANCE_STATE_COUNT = 10
};
enum PS3_DEVICE_ERR_HANDLE_STATE_TYPE {
	PS3_DEVICE_ERR_STATE_NORMAL = 0,
	PS3_DEVICE_ERR_STATE_CLEAN = 1,
	PS3_DEVICE_ERR_STATE_INIT = 2,
};

struct ps3_instance_state_machine {
	struct mutex lock;
	atomic_t state;
	unsigned char is_load;
	unsigned char is_suspend;
	unsigned char is_poweroff;
	unsigned char is_pci_err_recovery;
	unsigned char can_hostreset;
};

static inline const char *namePS3InstanceState(int s)
{
	static const char * const myNames[] = {
		[PS3_INSTANCE_STATE_INIT] = "PS3_INSTANCE_STATE_INIT",
		[PS3_INSTANCE_STATE_READY] = "PS3_INSTANCE_STATE_READY",
		[PS3_INSTANCE_STATE_PRE_OPERATIONAL] =
			"PS3_INSTANCE_STATE_PRE_OPERATIONAL",
		[PS3_INSTANCE_STATE_OPERATIONAL] =
			"PS3_INSTANCE_STATE_OPERATIONAL",
		[PS3_INSTANCE_STATE_SOFT_RECOVERY] =
			"PS3_INSTANCE_STATE_SOFT_RECOVERY",
		[PS3_INSTANCE_STATE_RECOVERY] = "PS3_INSTANCE_STATE_RECOVERY",
		[PS3_INSTANCE_STATE_SUSPEND] = "PS3_INSTANCE_STATE_SUSPEND",
		[PS3_INSTANCE_STATE_DEAD] = "PS3_INSTANCE_STATE_DEAD",
		[PS3_INSTANCE_STATE_QUIT] = "PS3_INSTANCE_STATE_QUIT",
		[PS3_INSTANCE_STATE_PCIE_RECOVERY] =
			"PS3_INSTANCE_STATE_PCIE_RECOVERY"
	};

	if (s >= PS3_INSTANCE_STATE_COUNT)
		return "PS3_INSTANCE_STATE_INVALID";

	return myNames[s];
}

struct ps3_recovery_function {
	int (*recovery_handle_cb)(struct ps3_instance *instance,
				  unsigned char reason);
	int (*hardreset_handle_pre_cb)(struct ps3_instance *instance);
	int (*hardreset_handle_wait_ready_cb)(struct ps3_instance *instance);
	int (*hardreset_handle_init_running_cb)(struct ps3_instance *instance);
	int (*hardreset_handle_post_cb)(struct ps3_instance *instance);
	int (*hardreset_handle_finish_cb)(struct ps3_instance *instance);
	int (*hardreset_handle_offline_cb)(struct ps3_instance *instance);
	int (*softreset_handle_pre_cb)(struct ps3_instance *instance);
	int (*softreset_handle_post_cb)(struct ps3_instance *instance);
	int (*halt_handle_cb)(struct ps3_instance *instance);
};

struct ps3_instance {
	struct list_head list_item;
	struct ps3_instance *peer_instance;
#ifndef _WINDOWS
	struct pci_dev *pdev;
	struct Scsi_Host *host;
#else
	unsigned long bus_number;
	struct ps3_pci_context pci_dev_context;
#endif
	struct Ps3Fifo __iomem *reg_set;
	atomic_t watchdog_reg_read_fail_count;

	struct ps3_cmd_context cmd_context;
	struct ps3_irq_context irq_context;
	struct ps3_dev_context dev_context;
#ifndef _WINDOWS
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

#ifdef _WINDOWS
	atomic_t ioctl_count;
#else
	struct semaphore ioctl_sem;
#endif
	struct ps3_ioc_adp_template *ioc_adpter;
	struct ps3_cmd_attr_context cmd_attr;
	struct PS3MgrEvent event_req_info;

	spinlock_t req_queue_lock;

	dma_addr_t ctrl_info_buf_h;
	dma_addr_t drv_info_buf_phys;
	unsigned char *drv_info_buf;
	dma_addr_t host_mem_info_buf_phys;
	unsigned char *host_mem_info_buf;
	unsigned int max_mgr_cmd_total_count;
	unsigned int max_mgr_cmd_count;
	unsigned int max_task_cmd_count;
	unsigned int min_intr_count;
	unsigned char reserve[4];
	unsigned int reply_fifo_depth_addition;

	unsigned long reg_bar;
	unsigned char is_support_sync_cache;
	unsigned char is_use_frontend_prp;
	unsigned char dma_mask;
	unsigned char is_support_jbod;
	unsigned char use_clusting;
	unsigned char is_adjust_register_count;
	unsigned char is_scan_host_finish;
	unsigned char is_probe_finish;
	unsigned char is_probe_failed;
	unsigned char is_suspend;
	unsigned char is_resume;
	unsigned char is_hard_reset;
	unsigned char is_pci_reset;
	unsigned char is_ioc_halt_support;
	unsigned char is_shallow_soft_recovery_support;
	unsigned char is_deep_soft_recovery_support;
	unsigned char is_hard_recovery_support;
	unsigned char is_halt_support_cli;
	unsigned char is_half_hard_reset;
	unsigned char is_host_added;
	unsigned char is_need_event;
	unsigned char is_raid1_direct_skip_mapblock_check;
	unsigned char is_single_disk_raid0_direct_skip_strip_check;
	unsigned char is_support_dump_ctrl;
	unsigned char is_support_io_limit;
	unsigned char task_manager_host_busy;
	unsigned char hilMode;
	unsigned char is_irq_prk_support;
	unsigned char is_support_irq;
	unsigned char is_raid;
	unsigned char smp_affinity_enable;
	unsigned char msix_combined;
	unsigned char reserved[2];
	unsigned short unload_timeout;
	unsigned short wait_ready_timeout;
	unsigned short dev_id;
	unsigned char dma_addr_bit_pos;
	unsigned char pci_err_handle_state;
	const char *product_model;
	long long __percpu *scsi_cmd_deliver;

	unsigned long long ioc_fw_version;
	struct mutex task_mgr_reset_lock;
#ifdef _WINDOWS
	STOR_DPC device_reset_dpc;
#endif
	unsigned char page_mode_change;
	unsigned long long page_mode_addr_mask;
	atomic_t is_err_scsi_processing;
	atomic_t reg_op_count;
	atomic_t host_reset_processing;
	atomic_t watchdog_ref;
	struct ps3_instance_state_machine state_machine;
	struct ps3_recovery_context *recovery_context;
	struct ps3_recovery_function recovery_function;
#ifndef _WINDOWS
	struct work_struct recovery_irq_work;
	struct workqueue_struct *recovery_irq_queue;
	unsigned char recovery_irq_enable;
	unsigned char is_print_special_log;
	unsigned char reserved2[2];
	unsigned int hard_dog_mask;
#endif
	atomic_t hardreset_event;
	struct ps3_qos_context qos_context;
	struct mutex task_abort_lock;
	unsigned char r1x_mode;
	unsigned long long start_pfn;
	unsigned long long end_pfn;
	unsigned long long so_start_addr;
	unsigned long long so_end_addr;
	int device_busy_threshold;
	unsigned char is_pcie_err_detected;
	unsigned char reserved3;
};

#ifndef _WINDOWS
struct ps3_mgmt_info {
	struct mutex ps3_mgmt_lock;
	struct list_head instance_list_head;
};

enum {
	PS3_HOST_MACHINE_DEFAULT,
	PS3_HOST_MACHINE_X86 = 0,
	PS3_HOST_MACHINE_ARM,
	PS3_HOST_MACHINE_MIPS,
	PS3_HOST_MACHINE_COUNT,
};

enum {
	PS3_HOST_VENDOR_DEFAULT,
	PS3_HOST_VENDOR_INTEL = 0,
	PS3_HOST_VENDOR_HYGON,
	PS3_HOST_VENDOR_AMD,
	PS3_HOST_VENDOR_COUNT,
};

enum  {
	PS3_HOST_CPU_DEFAULT = 0,
	PS3_HOST_HYGON_CPU_TYPE1,
	PS3_HOST_HYGON_CPU_TYPE2,
};

#define SYS_INFO_LEN (65)

struct ps3_host_info {
	unsigned char machine;
	unsigned char vendor;
	unsigned char model;
	unsigned short cpu_cnt;
	unsigned short core_cnt;
	unsigned short processor_cnt;
	char release[SYS_INFO_LEN + 1];
	unsigned short cpu_type;
};

struct ps3_mgmt_info *ps3_mgmt_info_get(void);

struct ps3_instance *ps3_instance_lookup(unsigned short host_no);

int ps3_instance_add(struct ps3_instance *instance);

int ps3_instance_remove(struct ps3_instance *instance);

void ps3_mgmt_info_init(void);

void ps3_mgmt_exit(void);

#endif

void ps3_instance_init(struct ps3_instance *instance);

int ps3_instance_state_transfer(struct ps3_instance *instance,
				unsigned int exp_cur_state,
				unsigned int dest_state);

int ps3_instance_no_lock_state_transfer(struct ps3_instance *instance,
					unsigned int dest_state);

void ps3_instance_state_transfer_to_dead_nolock(struct ps3_instance *instance);

void ps3_instance_state_transfer_to_dead(struct ps3_instance *instance);

void ps3_instance_state_transfer_to_pcie_recovery(struct ps3_instance *instance);

void ps3_instance_state_transfer_to_quit(struct ps3_instance *instance);

void ps3_instance_state_transfer_to_suspend(struct ps3_instance *instance);

void ps3_instance_state_transition_to_recovery(struct ps3_instance *instance);

int ps3_instance_wait_for_operational(struct ps3_instance *instance,
				      unsigned char is_hardreset);

int ps3_instance_wait_for_hard_reset_flag_done(struct ps3_instance *instance);

int ps3_instance_wait_for_dead_or_pre_operational(struct ps3_instance *instance);

static inline unsigned char
ps3_is_instance_state_normal(struct ps3_instance *instance,
			     unsigned char need_prk_err)
{
	int cur_state = ps3_atomic_read(&instance->state_machine.state);

	if (cur_state != PS3_INSTANCE_STATE_OPERATIONAL &&
	    cur_state != PS3_INSTANCE_STATE_PRE_OPERATIONAL &&
	    cur_state != PS3_INSTANCE_STATE_SOFT_RECOVERY) {
		LOG_WARN_LIM_WITH_CHECK(instance, need_prk_err,
					"hno:%u instance exception state %s\n",
					PS3_HOST(instance),
					namePS3InstanceState(cur_state));

		return PS3_FALSE;
	}

	return PS3_TRUE;
}

static inline void ps3_pci_err_recovery_set(struct ps3_instance *instance,
					    unsigned char state)
{
	instance->state_machine.is_pci_err_recovery = state;
}

static inline unsigned char
ps3_pci_err_recovery_get(struct ps3_instance *instance)
{
	return instance->state_machine.is_pci_err_recovery;
}

static inline unsigned char
ps3_need_block_hard_reset_request(struct ps3_instance *instance)
{
	int ret = PS3_SUCCESS;

	if (ps3_pci_err_recovery_get(instance)) {
		LOG_WARN_LIM(
			"hno[%u], host in pci recovery during reset request\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto end;
	}
	ps3_mutex_lock(&instance->state_machine.lock);
	if (instance->recovery_context->host_reset_state !=
	    PS3_HOST_RESET_INIT) {
		ps3_mutex_unlock(&instance->state_machine.lock);
		LOG_WARN_LIM(
			"hno[%u], host in host reset during  reset request\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto end;
	}
	ps3_mutex_unlock(&instance->state_machine.lock);
end:
	return ((ret == PS3_SUCCESS) ? PS3_FALSE : PS3_TRUE);
}

static inline void
ps3_need_wait_hard_reset_request(struct ps3_instance *instance)
{
	do {
		if (ps3_pci_err_recovery_get(instance)) {
			LOG_WARN_LIM(
				"hno[%u], host in pci recovery during reset request\n",
				PS3_HOST(instance));
			ps3_msleep(100);
			continue;
		}
		ps3_mutex_lock(&instance->state_machine.lock);
		if (instance->recovery_context->host_reset_state !=
		    PS3_HOST_RESET_INIT) {
			ps3_mutex_unlock(&instance->state_machine.lock);
			LOG_WARN_LIM(
				"hno[%u], host in host reset during  reset request\n",
				PS3_HOST(instance));
			ps3_msleep(100);
			continue;
		}
		ps3_mutex_unlock(&instance->state_machine.lock);
		break;
	} while (1);
}

static inline unsigned char ps3_state_is_normal(int cur_state)
{
	return (cur_state != PS3_INSTANCE_STATE_OPERATIONAL &&
		cur_state != PS3_INSTANCE_STATE_PRE_OPERATIONAL) ?
		       PS3_FALSE :
		       PS3_TRUE;
}

int ps3_instance_wait_for_normal(struct ps3_instance *instance);

int ps3_recovery_state_wait_for_normal(struct ps3_instance *instance);

struct ps3_ioc_adp_template {
	int (*io_cmd_build)(struct ps3_cmd *cmd);
	int (*mgr_cmd_build)(struct ps3_instance *instance,
			     struct ps3_cmd *cmd);
	void (*init_cmd_send)(struct ps3_instance *instance,
			      struct PS3CmdWord *cmd_word);
	void (*cmd_send)(struct ps3_instance *instance,
			 struct PS3CmdWord *cmd_word);
	unsigned int (*ioc_state_get)(struct ps3_instance *instance);
	int (*ioc_init_state_to_ready)(struct ps3_instance *instance);
	int (*ioc_init_proc)(struct ps3_instance *instance);
	void (*ioc_resource_prepare)(struct ps3_instance *instance);
	int (*ioc_hard_reset)(struct ps3_instance *instance);
	int (*ioc_shallow_soft_reset)(struct ps3_instance *instance);
	int (*ioc_deep_soft_reset)(struct ps3_instance *instance);
	int (*ioc_force_to_fault)(struct ps3_instance *instance);
	int (*ioc_force_to_halt)(struct ps3_instance *instance);
	int (*irq_init)(struct ps3_instance *instance);
	void (*irq_enable)(struct ps3_instance *instance);
	void (*irq_disable)(struct ps3_instance *instance);
#ifndef _WINDOWS
	irqreturn_t (*isr)(int irq_no, void *data);
	struct scsi_transport_template *(*sas_transport_get)(void);
#else
	unsigned char (*isr)(void *instance, unsigned long irq_no);
#endif
	void (*event_filter_table_get)(unsigned char *data);
	void (*reg_write)(struct ps3_instance *instance, unsigned long long val,
			  void __iomem *reg);
	unsigned long long (*reg_read)(struct ps3_instance *instance,
				       void __iomem *reg);
	unsigned char (*is_need_direct_to_normal)(const struct ps3_cmd *cmd);
	unsigned char (*max_replyq_count_get)(struct ps3_instance *instance,
					      unsigned int *max_replyq_count);
	void (*check_vd_member_change)(struct ps3_instance *instance,
				       struct ps3_pd_entry *local_entry);
	unsigned char (*scsih_stream_is_detect)(struct ps3_cmd *cmd);
	unsigned char (*scsih_stream_is_direct)(const struct ps3_cmd *cmd);
	unsigned int (*ioc_heartbeat_detect)(struct ps3_instance *instance);
	void __iomem *(*reg_set)(struct pci_dev *pdev, unsigned long reg_bar);
	unsigned char (*ioc_security_check)(struct ps3_instance *instance);
	void (*io_cmd_rebuild)(struct ps3_cmd *cmd);
	unsigned char (*rw_cmd_is_need_split)(struct ps3_cmd *cmd);
	unsigned char (*write_direct_enable)(struct ps3_cmd *cmd);
	unsigned char (*ssd_vd_qmask_calculate)(struct ps3_cmd *cmd);
	void (*pd_cmd_word_cpu_set)(struct ps3_cmd *cmd);
#ifdef PS3_NCQ_PRIO_SUPPORT
	ssize_t (*ncq_prio_enable_stor)(struct device *dev, const char *buf);
#endif
	int (*ext_sge_cnt_calc)(struct ps3_instance *instance);
	void (*max_sge_config)(struct ps3_instance *instance);
};
#define PS3_DEVICE_IS_SWITCH(id)                                               \
	((id == PCI_DEVICE_ID_PS3_SWITCH ||                                    \
	  id == PCI_DEVICE_ID_PS3_SWITCH_FPGA))

#ifndef _WINDOWS
void ps3_ioc_adp_init(struct ps3_instance *instance,
		      const struct pci_device_id *id);

void ps3_remove(struct pci_dev *pdev);

#else
void ps3_ioc_adp_init(struct ps3_instance *instance);
#endif

static inline int ps3_get_pci_function(struct pci_dev *pci)
{
	return PCI_FUNC(pci->devfn);
}

static inline int ps3_get_pci_slot(struct pci_dev *pci)
{
	return PCI_SLOT(pci->devfn);
}

static inline int ps3_get_pci_bus(struct pci_dev *pci)
{
	return pci->bus->number;
}

static inline int ps3_get_pci_domain(struct pci_dev *pci)
{
	return pci_domain_nr(pci->bus);
}

static inline bool ps3_is_latest_func(struct ps3_instance *instance)
{
	bool ret = PS3_TRUE;
	struct ps3_instance *peer_instance = NULL;

	list_for_each_entry(peer_instance,
			     &ps3_mgmt_info_get()->instance_list_head,
			     list_item) {
		if ((peer_instance != NULL) &&
		    (ps3_get_pci_domain(peer_instance->pdev) ==
		     ps3_get_pci_domain(instance->pdev)) &&
		    (PCI_BUS_NUM(peer_instance->pdev->devfn) ==
		     PCI_BUS_NUM(instance->pdev->devfn)) &&
		    (PCI_SLOT(peer_instance->pdev->devfn) ==
		     PCI_SLOT(instance->pdev->devfn)) &&
		    (PCI_FUNC(peer_instance->pdev->devfn) !=
		     PCI_FUNC(instance->pdev->devfn))) {
			ret = PS3_FALSE;
		}
	}

	return ret;
}

static inline void ps3_get_so_addr_ranger(struct ps3_instance *instance,
					  unsigned long long addr,
					  unsigned int offset)
{
	unsigned long long so_end_addr = (addr + offset) - 1;

	if (instance->so_start_addr == 0 && instance->so_end_addr == 0) {
		instance->so_start_addr = addr;
		instance->so_end_addr = so_end_addr;
		goto l_out;
	}
	instance->so_start_addr =
		((addr < instance->so_start_addr) ? addr :
						    instance->so_start_addr);
	instance->so_end_addr =
		((so_end_addr > instance->so_end_addr) ? so_end_addr :
							 instance->so_end_addr);
l_out:
	return;
}

void ps3_host_info_get(void);

unsigned short ps3_host_vendor_get(void);

char *ps3_host_release_get(void);

unsigned short ps3_host_cpu_type_get(void);

unsigned char ps3_is_last_func(struct ps3_instance *instance);
#endif
