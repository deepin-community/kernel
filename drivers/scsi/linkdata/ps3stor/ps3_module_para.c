// SPDX-License-Identifier: GPL-2.0
#ifndef _WINDOWS
#include "linux/moduleparam.h"
#endif
#include "ps3_inner_data.h"
#include "ps3_module_para.h"
#include "ps3_driver_log.h"
#include "ps3_drv_ver.h"
#include "ps3_ioc_state.h"
#include "ps3_kernel_version.h"

#ifndef _WINDOWS
static unsigned int cli_ver = PS3_IOCTL_VERSION;
module_param(cli_ver, uint, 0444);
MODULE_PARM_DESC(cli_ver,
		 "The version for communication between driver and CLI");
#endif

static unsigned int g_throttle_que_depth = PS3_DEVICE_QDEPTH_DEFAULT_VALUE;
#ifndef _WINDOWS
module_param(g_throttle_que_depth, uint, 0644);
MODULE_PARM_DESC(
	g_throttle_que_depth,
	"IOC queue depth when throttled due to SCSI cmd timeout. Default: 16");
#endif
static unsigned int g_debug_mem_size;
#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) ||                                              \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) ||           \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
module_param(g_debug_mem_size, uint, 0644);
MODULE_PARM_DESC(
	g_debug_mem_size,
	"Allocate DMA memory for IOC debugging little than 65535KB. Default: 0KB");
#endif
#endif

static unsigned int g_use_clustering = 1;
#ifndef _WINDOWS
module_param(g_use_clustering, uint, 0644);
MODULE_PARM_DESC(
	g_use_clustering,
	"SCSI mid-layer bio merge feature enable/disable. Default: enable(1)");
#endif
static unsigned int g_scsi_cmd_timeout;
#ifndef _WINDOWS
module_param(g_scsi_cmd_timeout, uint, 0644);
MODULE_PARM_DESC(
	g_scsi_cmd_timeout,
	"SCSI cmd timeout (10-255s). Default: 0(No specify default 90s)");
#endif

extern unsigned int g_ps3_r1x_lock_flag;
extern unsigned int g_ps3_r1x_lock_enable;
#ifndef _WINDOWS

module_param(g_ps3_r1x_lock_enable, uint, 0444);
MODULE_PARM_DESC(
	g_ps3_r1x_lock_enable,
	"R1x write conflict check feature enable/disable. Default: disable(0)");
#endif

#if defined(PS3_SUPPORT_DEBUG) ||                                              \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) ||           \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
extern unsigned int g_ps3_qos_hdd_pd_quota;
extern unsigned int g_ps3_qos_nvme_pd_quota;
#ifndef _WINDOWS
module_param(g_ps3_qos_hdd_pd_quota, uint, 0644);
MODULE_PARM_DESC(g_ps3_qos_hdd_pd_quota, "Qos pd quota. Default: 40");
module_param(g_ps3_qos_nvme_pd_quota, uint, 0644);
MODULE_PARM_DESC(g_ps3_qos_nvme_pd_quota, "Qos nvme pd quota. Default: 127");
#endif

extern unsigned int g_ps3_r1x_rb_diff_cmds;
#ifndef _WINDOWS
module_param(g_ps3_r1x_rb_diff_cmds, uint, 0644);
MODULE_PARM_DESC(g_ps3_r1x_rb_diff_cmds,
		 "R1x read balancing outstanding threshold. Default: 4");
#endif

#endif

static unsigned int g_direct_to_normal_enable = 1;
#ifndef _WINDOWS
module_param(g_direct_to_normal_enable, uint, 0644);
MODULE_PARM_DESC(g_direct_to_normal_enable,
		 "Direct to normal feature enable/disable. Default: enable(1)");
#endif

static unsigned int g_hba_check_time = 10;
#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) ||                                              \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) ||           \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
module_param(g_hba_check_time, uint, 0644);
MODULE_PARM_DESC(g_hba_check_time,
		 "HBA device id check time out. Default: 10s");
#endif
#endif

static unsigned int g_task_reset_delay_time = 50;
#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) ||                                              \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) ||           \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
module_param(g_task_reset_delay_time, uint, 0644);
MODULE_PARM_DESC(g_task_reset_delay_time,
		 "Task reset delay time (0-1000ms). Default: 50ms");
#endif
#endif

static unsigned int g_r1x_ring_size = 16;
#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) ||                                              \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) ||           \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
module_param(g_r1x_ring_size, uint, 0644);
MODULE_PARM_DESC(
	g_r1x_ring_size,
	"If R1X direct read more than g_r1x_ring_size, read from spare. Default: 16MB");
#endif
#endif

static unsigned int g_direct_check_stream_enable = PS3_TRUE;
#ifndef _WINDOWS
module_param(g_direct_check_stream_enable, uint, 0644);
MODULE_PARM_DESC(
	g_direct_check_stream_enable,
	"Direct detect stream or not feature enable/disable. Default: enable(1)");
#endif

static int g_device_busy_threshold = PS3_DEVICE_IO_BUSY_THRESHOLD;
#ifndef _WINDOWS
module_param(g_device_busy_threshold, int, 0644);
MODULE_PARM_DESC(g_device_busy_threshold,
		 "Device busy threshold value. Default: 8");
#endif

#ifndef __cplusplus
static char g_log_path[80] = { 0 };
#ifndef _WINDOWS
module_param_string(g_log_path, g_log_path, 80, 0644);
MODULE_PARM_DESC(
	g_log_path,
	"The log path of host driver will be saved little than 80 chars. Default: /var/log");
#endif
#endif

static unsigned int g_log_file_size = 200;
#ifdef PS3_CFG_RELEASE
static unsigned int g_log_level = LEVEL_INFO;
#else
static unsigned int g_log_level = LEVEL_DEBUG;
#endif

#if defined(PS3_SUPPORT_DEBUG) ||                                              \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) ||           \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
#ifndef _WINDOWS
module_param(g_log_file_size, uint, 0644);
MODULE_PARM_DESC(g_log_file_size,
		 "Single driver log file size (10-200MB). Default: 200MB");

module_param(g_log_level, uint, 0644);
MODULE_PARM_DESC(g_log_level,
		 "Specify driver log level."
#ifdef PS3_CFG_RELEASE
		 "0 - error, 1 - warn, 2 - info, 3 - debug. Default: 2");
#else
		 "0 - error, 1 - warn, 2 - info, 3 - debug. Default: 3");
#endif
#endif
#endif

static unsigned int g_r1x_time_out = 3000;
#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) ||                                              \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) ||           \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))

module_param(g_r1x_time_out, uint, 0644);
MODULE_PARM_DESC(g_r1x_time_out,
		 "R1X conflict in queue after time. Default: 3000ms");
#endif
#endif

static unsigned int g_r1x_conflict_queue_enable = PS3_TRUE;
#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) ||                                              \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) ||           \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
module_param(g_r1x_conflict_queue_enable, uint, 0644);
MODULE_PARM_DESC(
	g_r1x_conflict_queue_enable,
	"R1X conflict queue function feature enable/disable. Default: enable(1)");
#endif
#endif

static unsigned int g_latency_perf_mode_enable;
module_param(g_latency_perf_mode_enable, uint, 0444);
MODULE_PARM_DESC(g_latency_perf_mode_enable,
	"Latency perf mode enable/disable. Default: disable(0)");

#ifndef _WINDOWS
#endif
static unsigned int g_log_space_size;

static unsigned int g_log_tty;
#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) ||                                              \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) ||           \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
module_param(g_log_tty, uint, 0644);
MODULE_PARM_DESC(
	g_log_tty,
	"Allow driver log output to tty console feature enable/disable. Default: disable(0)");
#endif
#endif

#if defined PS3_HARDWARE_ASIC
static unsigned int g_hard_reset_enable = 1;
static unsigned int g_deep_soft_reset_enable;
#elif (((defined PS3_HARDWARE_FPGA || defined PS3_HARDWARE_ASIC) &&            \
	defined PS3_MODEL_V200) ||                                             \
	defined(PS3_HARDWARE_HAPS_V200))
static unsigned int g_hard_reset_enable = 1;
static unsigned int g_deep_soft_reset_enable;
#else
static unsigned int g_hard_reset_enable;
static unsigned int g_deep_soft_reset_enable;
#endif
#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) ||                                              \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) ||           \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
module_param(g_hard_reset_enable, uint, 0644);
MODULE_PARM_DESC(g_hard_reset_enable,
		 "Hard reset feature enable/disable. Default: enable(1)");
module_param(g_deep_soft_reset_enable, uint, 0644);
MODULE_PARM_DESC(g_deep_soft_reset_enable,
		 "Deep soft reset feature enable/disable. Default: disable(0)");
#endif
#endif

static unsigned int g_aer_handle_support = 1;
#ifndef _WINDOWS
module_param(g_aer_handle_support, uint, 0644);
MODULE_PARM_DESC(
	g_aer_handle_support,
	"Driver aer handle support feature enable/disable. Default: enable(1)");
#endif

#ifndef __cplusplus
static char g_version_verbose[512] = { 0 };
#ifndef _WINDOWS
module_param_string(g_version_verbose, g_version_verbose, 511, 0444);
MODULE_PARM_DESC(g_version_verbose,
		 "Display detailed version information about ps3stor driver");
#endif
#endif

static unsigned int g_hard_reset_waiting;
static unsigned int g_use_hard_reset_reg = 1;
static unsigned int g_use_hard_reset_max_retry = PS3_HARD_RESET_MAX_RETRY;

#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) ||                                              \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) ||           \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
module_param(g_hard_reset_waiting, uint, 0644);
MODULE_PARM_DESC(g_hard_reset_waiting,
		 "Allow to access PCIe Config/BAR after xxx ms. Default: 0");
module_param(g_use_hard_reset_reg, uint, 0644);
MODULE_PARM_DESC(
	g_use_hard_reset_reg,
	"Write hard reset reg triggered feature enable/disable. Default: enable(1)");
module_param(g_use_hard_reset_max_retry, uint, 0644);
MODULE_PARM_DESC(g_use_hard_reset_max_retry,
		 "Hard reset retry max count. Default: 2");
#endif
#endif

#if ((defined PS3_HARDWARE_FPGA && defined PS3_MODEL_V200) ||                  \
	defined(PS3_HARDWARE_HAPS_V200) || defined(PS3_HARDWARE_ASIC))
static unsigned int g_enable_heartbeat = 1;
module_param(g_enable_heartbeat, uint, 0644);
MODULE_PARM_DESC(g_enable_heartbeat,
		 "Heartbeat query feature enable/disable. Default: enable(1)");
#else
static unsigned int g_enable_heartbeat;
module_param(g_enable_heartbeat, uint, 0644);
MODULE_PARM_DESC(g_enable_heartbeat,
		 "Heartbeat query feature enable/disable. Default: disable(0)");
#endif

static unsigned int g_hil_mode = 0xFFFF;
#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) ||                                              \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) ||           \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
module_param(g_hil_mode, uint, 0644);
MODULE_PARM_DESC(
g_hil_mode,
"Set HIL operational mode.\n"
"\t0 - SW mode, 1 - HW mode, 2 - Enhanced HW mode, 3 - SW assist mode. Default: 65535(0xFFFF)");
#endif
#endif

static unsigned int g_available_func_id = 0xFF;
#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) ||                                              \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) ||           \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
module_param(g_available_func_id, uint, 0644);
MODULE_PARM_DESC(g_available_func_id,
		 "Set function id.\n"
		 "\t0 - Func0, 1 - Func1, 2 - Unlimited. Default: 255(0xFF)");
#endif
#endif

static unsigned int g_pci_irq_mode = PS3_PCI_IRQ_MODE_NONE_SPE;
#ifndef _WINDOWS
#if defined(PS3_SUPPORT_DEBUG) ||                                              \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) ||           \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
module_param(g_pci_irq_mode, uint, 0644);
MODULE_PARM_DESC(g_pci_irq_mode,
		 "Specify pci irq mode.\n"
		 "\t0 - none specify, 1 - legacy, 2 - msi, 3 - msix. Default: 0");
#endif
#endif

#if defined(PS3_TAGSET_SUPPORT)

#if defined(PS3_SUPPORT_TAGSET)
static int g_ps3_tagset_enable = 1;
module_param(g_ps3_tagset_enable, int, 0444);
MODULE_PARM_DESC(g_ps3_tagset_enable,
		 "Shared host tagset enable/disable. Default: enable(1)");
#else
static int g_ps3_tagset_enable;
module_param(g_ps3_tagset_enable, int, 0444);
MODULE_PARM_DESC(g_ps3_tagset_enable,
		 "Shared host tagset enable/disable. Default: disable(0)");
#endif

#endif

static unsigned int g_smp_affinity_enable = 1;
module_param(g_smp_affinity_enable, int, 0444);
MODULE_PARM_DESC(g_smp_affinity_enable,
		 "SMP affinity feature enable/disable. Default: enable(1)");

static int max_sgl_entries = PS3_DEFAULT_SGE_CNT;
module_param(max_sgl_entries, int, 0444);
MODULE_PARM_DESC(max_sgl_entries,
	"Preferred max number of SGE to be used for a single I/O\n\t\t"
	"The actual value will be determined by the driver\n\t\t"
	"This parameter can be [128,256,262,512]. Default: 262");

void ps3_version_verbose_fill(void)
{
#ifndef __cplusplus
	int len = 0;
	int total_len = sizeof(g_version_verbose) - 1;

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

unsigned int ps3_throttle_qdepth_query(void)
{
	return g_throttle_que_depth;
}

void ps3_debug_mem_size_modify(unsigned int size)
{
	g_debug_mem_size = size;
}

unsigned int ps3_debug_mem_size_query(void)
{
	return g_debug_mem_size;
}

unsigned short ps3_use_clustering_query(void)
{
	return (unsigned short)g_use_clustering;
}

void ps3_scsi_cmd_timeout_modify(unsigned int val)
{
	g_scsi_cmd_timeout = val;
}

void ps3_scsi_cmd_timeout_adjust(void)
{
	if (g_scsi_cmd_timeout != 0 &&
	    (g_scsi_cmd_timeout < PS3_SCSI_CMD_TIMEOUT_MIN ||
	     g_scsi_cmd_timeout > PS3_SCSI_CMD_TIMEOUT_MAX)) {
		g_scsi_cmd_timeout = PS3_SCSI_CMD_TIMEOUT_DEFAULT;
	}
}

unsigned int ps3_scsi_cmd_timeout_query(void)
{
	return g_scsi_cmd_timeout;
}

unsigned int ps3_r1x_lock_flag_quiry(void)
{
	return g_ps3_r1x_lock_flag;
}

void ps3_r1x_lock_flag_modify(unsigned int val)
{
	g_ps3_r1x_lock_flag = val;
}

unsigned int ps3_direct_to_normal_query(void)
{
	return g_direct_to_normal_enable;
}

void ps3_direct_to_normal_modify(unsigned int val)
{
	g_direct_to_normal_enable = val;
}

unsigned int ps3_hba_check_time_query(void)
{
	return g_hba_check_time;
}

unsigned int ps3_task_reset_delay_time_query(void)
{
	return g_task_reset_delay_time;
}

unsigned long long ps3_r1x_ring_size_query(void)
{
	unsigned long long ring_size_byte = (unsigned long long)g_r1x_ring_size;

	ring_size_byte = MB_TO_BYTE(ring_size_byte);
	return ring_size_byte;
}

void ps3_r1x_ring_size_modify(unsigned int size)
{
	g_r1x_ring_size = size;
}

unsigned int ps3_direct_check_stream_query(void)
{
	return g_direct_check_stream_enable;
}

void ps3_direct_check_stream_modify(unsigned int val)
{
	g_direct_check_stream_enable = val;
}

int ps3_device_busy_threshold_query(void)
{
	return g_device_busy_threshold;
}

void ps3_device_busy_threshold_modify(int busy)
{
	g_device_busy_threshold = busy;
}

void ps3_log_level_modify(unsigned int level)
{
	ps3_level_set(level);
}

char *ps3_log_path_query(void)
{
#ifndef __cplusplus
	return g_log_path;
#else
	return NULL;
#endif
}

unsigned int ps3_log_space_size_query(void)
{
	return g_log_space_size;
}

unsigned int ps3_log_file_size_query(void)
{
	return g_log_file_size;
}

unsigned int ps3_log_level_query(void)
{
	return g_log_level;
}

void ps3_log_file_size_modify(unsigned int size)
{
	g_log_file_size = size;
}

unsigned int ps3_log_tty_query(void)
{
	return g_log_tty;
}

void ps3_log_tty_modify(unsigned int enable)
{
	g_log_tty = enable;
}

void ps3_hard_reset_enable_modify(unsigned int val)
{
	g_hard_reset_enable = val;
}

unsigned int ps3_hard_reset_enable_query(void)
{
	return g_hard_reset_enable;
}
unsigned int ps3_deep_soft_reset_enable_query(void)
{
	return g_deep_soft_reset_enable;
}
void ps3_deep_soft_reset_enable_modify(unsigned int val)
{
	g_deep_soft_reset_enable = val;
}

unsigned int ps3_aer_handle_support_query(void)
{
	return g_aer_handle_support;
}

void ps3_aer_handle_support_set(unsigned int aer_handle_support)
{
	g_aer_handle_support = aer_handle_support;
}

unsigned int ps3_hard_reset_waiting_query(void)
{
	return g_hard_reset_waiting;
}

unsigned int ps3_use_hard_reset_reg_query(void)
{
	return g_use_hard_reset_reg;
}
unsigned int ps3_use_hard_reset_max_retry(void)
{
	return g_use_hard_reset_max_retry;
}

unsigned int ps3_enable_heartbeat_query(void)
{
	return g_enable_heartbeat;
}

unsigned int ps3_enable_heartbeat_set(unsigned int val)
{
	return g_enable_heartbeat = val;
}

unsigned int ps3_hil_mode_query(void)
{
	return g_hil_mode;
}

void ps3_hil_mode_modify(unsigned int val)
{
	g_hil_mode = val;
}

unsigned int ps3_available_func_id_query(void)
{
	return g_available_func_id;
}

void ps3_available_func_id_modify(unsigned int val)
{
	g_available_func_id = val;
}

unsigned int ps3_r1x_tmo_query(void)
{
	if (unlikely(g_r1x_time_out > 360000))
		g_r1x_time_out = 360000;
	return g_r1x_time_out * HZ / 1000;
}

unsigned int ps3_r1x_conflict_queue_support_query(void)
{
	return g_r1x_conflict_queue_enable;
}

unsigned int ps3_pci_irq_mode_query(void)
{
	return g_pci_irq_mode;
}

#ifdef PS3_UT
unsigned int ps3_cli_ver_query(void)
{
	return cli_ver;
}
#endif
#if defined(PS3_TAGSET_SUPPORT)

void ps3_tagset_enable_modify(unsigned char enable)
{
	g_ps3_tagset_enable = enable;
}

unsigned char ps3_tagset_enable_query(void)
{
	return (g_ps3_tagset_enable == 1) ? PS3_TRUE : PS3_FALSE;
}
#endif

unsigned char ps3_smp_affinity_query(void)
{
	return (g_smp_affinity_enable == 1) ? PS3_TRUE : PS3_FALSE;
}

unsigned char ps3_latency_perf_mode_query(void)
{
	return (g_latency_perf_mode_enable == 1);
}

void ps3_latency_perf_mode_modify(unsigned char enable)
{
	g_latency_perf_mode_enable = enable;
}

int ps3_max_sge_cnt_query(void)
{
	return max_sgl_entries;
}

void ps3_max_sge_cnt_modify(int value)
{
	max_sgl_entries = value;
}
