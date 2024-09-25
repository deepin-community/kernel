
#ifndef _WINDOWS
#include "linux/moduleparam.h"
#endif
#include "ps3_inner_data.h"
#include "ps3_module_para.h"
#include "ps3_driver_log.h"
#include "ps3_drv_ver.h"
#include "ps3_ioc_state.h"

#ifndef _WINDOWS
static U32 cli_ver = PS3_IOCTL_VERSION;
module_param(cli_ver, uint, S_IRUGO | S_IRUSR);
MODULE_PARM_DESC(cli_ver,
	"The version for communication between driver and cli.");
#endif

static U32 g_throttle_que_depth = PS3_DEVICE_QDEPTH_DEFAULT_VALUE;
#ifndef _WINDOWS
module_param(g_throttle_que_depth, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_throttle_que_depth,
	"IOC queue depth when throttled due to SCSI cmd timeout. Default: 16");
#endif
static U32 g_debug_mem_size = 0;
#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
module_param(g_debug_mem_size, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_debug_mem_size,
	"Allocate DMA memory for IOC debugging little than 65535KB. Default: 0KB");
#endif
#endif

static U32 g_use_clustering = 1;
#ifndef _WINDOWS
module_param(g_use_clustering, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_use_clustering,
	"Make SCSI mid layer bio merge feature enable/disable. Default: enable(1)");
#endif
static U32 g_scsi_cmd_timeout = 0;
#ifndef _WINDOWS
module_param(g_scsi_cmd_timeout, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_scsi_cmd_timeout,
	"SCSI cmd timeout (10-255s). Default: 0(No specify default 90s)");
#endif

extern U32 g_ps3_r1x_lock_flag;
extern U32 g_ps3_r1x_lock_enable;
#ifndef _WINDOWS

#if 0
module_param(g_ps3_r1x_lock_flag, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_ps3_r1x_lock_flag,
	"R1X write conflict check func. 0 - hash+bitmap, 1 - "
	"hash+rangetree. Default: 0");
#endif
module_param(g_ps3_r1x_lock_enable, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_ps3_r1x_lock_enable,
	"Enable/disable r1x write conflict check feature enable/disable. Default: enable(1)");
#endif

#if defined(PS3_SUPPORT_DEBUG) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
extern U32 g_ps3_qos_hdd_pd_quota;
extern U32 g_ps3_qos_nvme_pd_quota;
#ifndef _WINDOWS
module_param(g_ps3_qos_hdd_pd_quota, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_ps3_qos_hdd_pd_quota,
	"Qos pd quota. Default: 40");
module_param(g_ps3_qos_nvme_pd_quota, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_ps3_qos_nvme_pd_quota,
	"Qos nvme pd quota. Default: 127");
#endif

extern U32 g_ps3_r1x_rb_diff_cmds;
#ifndef _WINDOWS
module_param(g_ps3_r1x_rb_diff_cmds, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_ps3_r1x_rb_diff_cmds,
		"Raid1x read balancing outstanding threshold. Default: 4");
#endif

#endif

static U32 g_direct_to_normal_enable = 1;
#ifndef _WINDOWS
module_param(g_direct_to_normal_enable, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_direct_to_normal_enable,
	"Direct to normal feature enable/disable. Default: enable(1)");
#endif

static U32 g_hba_check_time = 10;
#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
module_param(g_hba_check_time, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_hba_check_time, "HBA device id check time out. Default: 10s");
#endif
#endif

static U32 g_task_reset_delay_time = 50;
#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
module_param(g_task_reset_delay_time, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_task_reset_delay_time,
	"Task reset delay time (0-1000ms). Default: 50ms");
#endif
#endif

static U32 g_r1x_ring_size = 16;
#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
module_param(g_r1x_ring_size, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_r1x_ring_size,
	"If R1X direct read more than g_r1x_ring_size, read from spare. Default: 16MB");
#endif
#endif

static U32 g_direct_check_stream_enable = PS3_TRUE;
#ifndef _WINDOWS
module_param(g_direct_check_stream_enable, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_direct_check_stream_enable,
	"Direct detect stream or not feature enable/disable. Default: enable(1)");
#endif

static S32 g_device_busy_threshold = PS3_DEVICE_IO_BUSY_THRESHOLD;
#ifndef _WINDOWS
module_param(g_device_busy_threshold, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_device_busy_threshold,
	"Device busy threshold value. Default: 8.");
#endif

#ifndef __cplusplus
static char g_log_path[80] = {0};
#ifndef _WINDOWS
module_param_string(g_log_path, g_log_path, 80, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_log_path, "The path host driver will be saved little than 80 chars. Default: /var/log");
#endif
#endif 

static U32 g_log_file_size = 200;
#ifdef PS3_CFG_RELEASE
static U32 g_log_level = LEVEL_INFO;
#else
static U32 g_log_level = LEVEL_DEBUG;
#endif

#if defined(PS3_SUPPORT_DEBUG) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
#ifndef _WINDOWS
module_param(g_log_file_size, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_log_file_size,
	"Single driver log file size (10-200MB). Default: 200MB");

module_param(g_log_level, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_log_level,
	"specify driver log level."
#ifdef PS3_CFG_RELEASE
	"0 - error, 1 - warn, 2 - info, 3 - debug. Default: 2");
#else
	"0 - error, 1 - warn, 2 - info, 3 - debug. Default: 3");
#endif
#endif
#endif

static U32 g_r1x_time_out = 3000;
#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))

module_param(g_r1x_time_out, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_r1x_time_out,
	"R1X conflict in queue after time. Default: 3000ms");
#endif
#endif

static U32 g_r1x_conflict_queue_enable = PS3_TRUE;
#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
module_param(g_r1x_conflict_queue_enable, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_r1x_conflict_queue_enable,
	"R1X conflict queue function is feature enable enable/disable. Default: enable(1)");
#endif
#endif

#ifndef _WINDOWS
#ifdef PS3_SUPPORT_INJECT
#define PS3_MAX_ERR_INJECT	(16)
static U32 g_err_inject[PS3_MAX_ERR_INJECT];
static U32 num_inject;
static U32 g_enable_inject = 0;
module_param_array(g_err_inject, uint, &num_inject, 0);
MODULE_PARM_DESC(g_err_inject,
	"Err inject. Default: 0");
module_param(g_enable_inject, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_enable_inject,
	"Enable inject feature enable/disable. Default: disable(0)");

U32 ps3_err_inject_num_query(void)
{
	return num_inject;
}
U32* ps3_err_inject_array_query(void)
{
	return &g_err_inject[0];
}
U32 ps3_err_inject_state_query(void)
{
	return g_enable_inject;
}

#endif
#endif
static U32 g_log_space_size = 0;

static U32 g_log_tty = 0;
#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
module_param(g_log_tty, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_log_tty,
	"Allow driver log output to tty console feature enable/disable. Default: disable(0)");
#endif
#endif

#if defined PS3_HARDWARE_ASIC
static U32 g_hard_reset_enable = 1;
static U32 g_deep_soft_reset_enable = 0;
#elif (((defined PS3_HARDWARE_FPGA || defined PS3_HARDWARE_ASIC) \
	&& defined PS3_MODEL_V200)\
	|| defined(PS3_HARDWARE_HAPS_V200))
static U32 g_hard_reset_enable = 1;
static U32 g_deep_soft_reset_enable = 0;
#else
static U32 g_hard_reset_enable = 0;
static U32 g_deep_soft_reset_enable = 0;
#endif
#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
module_param(g_hard_reset_enable, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_hard_reset_enable,
	"Hard reset feature enable/disable. Default: enable(1)");
module_param(g_deep_soft_reset_enable, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_deep_soft_reset_enable,
	"Deep soft reset feature enable/disable. Default: disable(0)");
#endif
#endif

static U32 g_aer_handle_support = 1;
#ifndef _WINDOWS
module_param(g_aer_handle_support, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_aer_handle_support,
	"Modify driver aer handle support feature enable/disable. Default: enable(1)");
#endif

#ifndef __cplusplus
static char g_version_verbose[512] = {0};
#ifndef _WINDOWS
module_param_string(g_version_verbose, g_version_verbose, 511, S_IRUGO);
MODULE_PARM_DESC(g_version_verbose, "Display detailed version information about ps3stor driver.");
#endif 
#endif 

static U32 g_hard_reset_waiting = 0;
static U32 g_use_hard_reset_reg = 1;
static U32 g_use_hard_reset_max_retry = PS3_HARD_RESET_MAX_RETRY;

#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
module_param(g_hard_reset_waiting, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_hard_reset_waiting,
	"Allow to access PCIe Config/BAR after xxx ms. Default: 0");
module_param(g_use_hard_reset_reg, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_use_hard_reset_reg,
	"Write hard reset reg triggered feature enable/disable. Default: enable(1)");
module_param(g_use_hard_reset_max_retry, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_use_hard_reset_max_retry,
	"Hard reset retry max count. Default: 2");
#endif
#endif 

#if ((defined PS3_HARDWARE_FPGA && defined PS3_MODEL_V200) \
	|| defined(PS3_HARDWARE_HAPS_V200) || defined(PS3_HARDWARE_ASIC))
static U32 g_enable_heartbeat = 1;
#else  
static U32 g_enable_heartbeat = 0;
#endif 
#ifndef _WINDOWS
module_param(g_enable_heartbeat, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_enable_heartbeat,
	"Enable heartbeat feature enable/disable. Default: enable(1)");
#endif 

static U32 g_hil_mode = 0xFFFF;
#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
module_param(g_hil_mode, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_hil_mode,
	"Set HIL operational mode."
	"0 - SW mode, 1 - HW mode, 2 - Enhanced HW mode, 3 - SW assist mode. Default: 65535(0xFFFF)");
#endif
#endif

static U32 g_avaliable_func_id = 0xFF;
#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
module_param(g_avaliable_func_id, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_avaliable_func_id,
	"Set function id."
	"0 - Func0, 1 - Func1, 2 - Unlimited. Default: 0xFF");
#endif
#endif

static U32 g_pci_irq_mode = PS3_PCI_IRQ_MODE_NONE_SPE;
#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
module_param(g_pci_irq_mode, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_pci_irq_mode,
	"specify pci irq mode."
	"0 - none specify, 1 - legacy, 2 - msi, 3 - msix. Default: 0");
#endif
#endif

#if defined(PS3_TAGSET_SUPPORT)

#if ((defined(RHEL_MAJOR) && (RHEL_MAJOR == 8) && (RHEL_MINOR > 5)) || \
	(defined(RHEL_MAJOR) && (RHEL_MAJOR == 9) && (RHEL_MINOR >= 1)) || \
	(LINUX_VERSION_CODE >= KERNEL_VERSION(5,16,0)))
S32 g_ps3_tagset_enable = 1;
module_param(g_ps3_tagset_enable, int, 0444);
MODULE_PARM_DESC(g_ps3_tagset_enable, "Shared host tagset enable/disable. Default: enable(1)");
#else
S32 g_ps3_tagset_enable = 0;
module_param(g_ps3_tagset_enable, int, 0444);
MODULE_PARM_DESC(g_ps3_tagset_enable, "Shared host tagset enable/disable. Default: disable(0)");
#endif

#endif

static U32 g_smp_affinity_enable = 1;
module_param(g_smp_affinity_enable, int, 0444);
MODULE_PARM_DESC(g_smp_affinity_enable, "SMP affinity feature enable/disable. Default: enable(1)");

void ps3_version_verbose_fill(void)
{
#ifndef __cplusplus
	S32 len = 0;
	S32 total_len = sizeof(g_version_verbose) - 1;

	memset(g_version_verbose, 0, sizeof(g_version_verbose));
	len += snprintf(g_version_verbose + len, total_len - len, "%-20s:%s\n",
		"version", PS3_DRV_VERSION);

	len += snprintf(g_version_verbose + len, total_len - len, "%-20s:%s\n",
		"commit_id", PS3_DRV_COMMIT_ID);

	len += snprintf(g_version_verbose + len, total_len - len, "%-20s:%s\n",
		"toolchain_id", PS3_DRV_TOOLCHAIN_ID);

	len += snprintf(g_version_verbose + len, total_len - len, "%-20s:%s\n",
		"build_time", PS3_DRV_BUILD_TIME);

	len += snprintf(g_version_verbose + len, total_len - len, "%-20s:%s\n",
		"product_support", PS3_DRV_PRODUCT_SUPPORT);
#endif
}

U32 ps3_throttle_qdepth_query(void)
{
	return g_throttle_que_depth;
}

void ps3_debug_mem_size_modify(U32 size)
{
	g_debug_mem_size = size;
}

U32 ps3_debug_mem_size_query(void)
{
	return g_debug_mem_size;
}

U16 ps3_use_clustering_query(void)
{
	return (U16)g_use_clustering;
}

void ps3_scsi_cmd_timeout_modify(U32 val)
{
	g_scsi_cmd_timeout = val;
}

void ps3_scsi_cmd_timeout_adjust(void)
{

	if (g_scsi_cmd_timeout != 0 && (
		g_scsi_cmd_timeout < PS3_SCSI_CMD_TIMEOUT_MIN ||
		g_scsi_cmd_timeout > PS3_SCSI_CMD_TIMEOUT_MAX)) {
		g_scsi_cmd_timeout = PS3_SCSI_CMD_TIMEOUT_DEFAULT;
	}
}

U32 ps3_scsi_cmd_timeout_query(void)
{
	return g_scsi_cmd_timeout;
}

U32 ps3_r1x_lock_flag_quiry(void)
{
	return g_ps3_r1x_lock_flag;
}

void ps3_r1x_lock_flag_modify(U32 val)
{
	g_ps3_r1x_lock_flag = val;
}

U32 ps3_direct_to_normal_query(void)
{
	return g_direct_to_normal_enable;
}

void ps3_direct_to_normal_modify(U32 val)
{
	g_direct_to_normal_enable = val;
}

U32 ps3_hba_check_time_query(void)
{
	return g_hba_check_time;
}

U32 ps3_task_reset_delay_time_query(void)
{
	return g_task_reset_delay_time;
}

U64 ps3_r1x_ring_size_query(void)
{
	U64 ring_size_byte = (U64)g_r1x_ring_size;
	ring_size_byte = MB_TO_BYTE(ring_size_byte);
	return ring_size_byte;
}

void ps3_r1x_ring_size_modify(U32 size)
{
	g_r1x_ring_size = size;
}

U32 ps3_direct_check_stream_query(void)
{
	return g_direct_check_stream_enable;
}

void  ps3_direct_check_stream_modify(U32 val)
{
	g_direct_check_stream_enable = val;
}

S32 ps3_device_busy_threshold_query(void)
{
	return g_device_busy_threshold;
}

void ps3_device_busy_threshold_modify(S32 busy)
{
	g_device_busy_threshold = busy;
}

void ps3_log_level_modify(U32 level)
{
    ps3_level_set(level);
}

char* ps3_log_path_query(void)
{
#ifndef __cplusplus
    return g_log_path;
#else
    return NULL;
#endif
}

U32 ps3_log_space_size_query(void)
{
    return g_log_space_size;
}

U32 ps3_log_file_size_query(void)
{
    return g_log_file_size;
}

U32 ps3_log_level_query(void)
{
	return g_log_level;
}

void ps3_log_file_size_modify(U32 size)
{
    g_log_file_size = size;
}

U32 ps3_log_tty_query(void)
{
    return g_log_tty;
}

void ps3_log_tty_modify(U32 enable)
{
    g_log_tty = enable;
}

void ps3_hard_reset_enable_modify(U32 val)
{
	g_hard_reset_enable = val;
}

U32 ps3_hard_reset_enable_query(void)
{
	return g_hard_reset_enable;
}
U32 ps3_deep_soft_reset_enable_query(void)
{
	return g_deep_soft_reset_enable;
}
void ps3_deep_soft_reset_enable_modify(U32 val)
{
	g_deep_soft_reset_enable = val;
}

U32 ps3_aer_handle_support_query(void)
{
	return g_aer_handle_support;
}

void ps3_aer_handle_support_set(U32 aer_handle_support)
{
	g_aer_handle_support = aer_handle_support;
}

U32 ps3_hard_reset_waiting_query(void)
{
	return g_hard_reset_waiting;
}

U32 ps3_use_hard_reset_reg_query(void)
{
	return g_use_hard_reset_reg;
}
U32 ps3_use_hard_reset_max_retry(void)
{
	return g_use_hard_reset_max_retry;
}

U32 ps3_enable_heartbeat_query(void)
{
	return g_enable_heartbeat;
}

U32 ps3_enable_heartbeat_set(U32 val)
{
	return g_enable_heartbeat = val;
}

U32 ps3_hil_mode_query(void)
{
	return g_hil_mode;
}

void ps3_hil_mode_modify(U32 val)
{
	g_hil_mode = val;
	return;
}

U32 ps3_avaliable_func_id_query(void)
{
	return g_avaliable_func_id;
}

void ps3_avaliable_func_id_modify(U32 val)
{
	g_avaliable_func_id = val;
	return;
}

U32 ps3_r1x_tmo_query(void)
{
	if (unlikely(g_r1x_time_out > 360000)) {
		g_r1x_time_out = 360000;
	}
	return g_r1x_time_out * HZ / 1000;
}

U32 ps3_r1x_conflict_queue_support_query(void)
{
	return g_r1x_conflict_queue_enable;
}

U32 ps3_pci_irq_mode_query(void)
{
	return g_pci_irq_mode;
}

U32 ps3_cli_ver_query(void)
{
	return cli_ver;
}
#if defined(PS3_TAGSET_SUPPORT)

Bool ps3_tagset_enable_query(void)
{
	return (g_ps3_tagset_enable == 1) ? PS3_TRUE: PS3_FALSE;
}
#endif

Bool ps3_smp_affinity_query(void)
{
	return (g_smp_affinity_enable == 1) ? PS3_TRUE: PS3_FALSE;
}

