/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_MODULE_PARA_H_
#define _PS3_MODULE_PARA_H_

#define MB_TO_BYTE(MB) ((MB) << 20)
#define PS3_MAX_FUNC_ID (2)

unsigned int ps3_throttle_qdepth_query(void);

void ps3_debug_mem_size_modify(unsigned int size);

unsigned int ps3_debug_mem_size_query(void);

unsigned int ps3_sata_direct_query(void);

void ps3_sata_direct_modify(unsigned int val);

unsigned short ps3_use_clustering_query(void);

void ps3_scsi_cmd_timeout_modify(unsigned int val);

unsigned int ps3_scsi_cmd_timeout_query(void);

void ps3_scsi_cmd_timeout_adjust(void);

unsigned int ps3_r1x_lock_flag_quiry(void);

void ps3_r1x_lock_flag_modify(unsigned int val);

unsigned int ps3_direct_to_normal_query(void);

void ps3_direct_to_normal_modify(unsigned int val);

unsigned int ps3_hba_check_time_query(void);

unsigned int ps3_task_reset_delay_time_query(void);

unsigned long long ps3_r1x_ring_size_query(void);

void ps3_r1x_ring_size_modify(unsigned int size);

unsigned int ps3_direct_check_stream_query(void);

int ps3_device_busy_threshold_query(void);

void ps3_device_busy_threshold_modify(int busy);

void ps3_log_level_modify(unsigned int level);

char *ps3_log_path_query(void);

unsigned int ps3_log_space_size_query(void);

unsigned int ps3_log_file_size_query(void);

void ps3_log_file_size_modify(unsigned int size);

unsigned int ps3_log_level_query(void);

unsigned int ps3_log_tty_query(void);

void ps3_log_tty_modify(unsigned int enable);

void ps3_hard_reset_enable_modify(unsigned int val);
void ps3_deep_soft_reset_enable_modify(unsigned int val);

unsigned int ps3_hard_reset_enable_query(void);
unsigned int ps3_deep_soft_reset_enable_query(void);

unsigned int ps3_log_level_query(void);

unsigned int ps3_aer_handle_support_query(void);
void ps3_aer_handle_support_set(unsigned int aer_handle_support);

void ps3_version_verbose_fill(void);

unsigned int ps3_hard_reset_waiting_query(void);
unsigned int ps3_use_hard_reset_reg_query(void);
unsigned int ps3_use_hard_reset_max_retry(void);
unsigned int ps3_enable_heartbeat_query(void);
unsigned int ps3_enable_heartbeat_set(unsigned int val);
unsigned int ps3_hil_mode_query(void);
void ps3_hil_mode_modify(unsigned int val);
unsigned int ps3_available_func_id_query(void);
void ps3_available_func_id_modify(unsigned int val);
void ps3_direct_check_stream_modify(unsigned int val);
unsigned int ps3_r1x_tmo_query(void);
unsigned int ps3_r1x_conflict_queue_support_query(void);
unsigned int ps3_pci_irq_mode_query(void);

#if defined(PS3_TAGSET_SUPPORT)

void ps3_tagset_enable_modify(unsigned char enable);

unsigned char ps3_tagset_enable_query(void);
#endif
unsigned char ps3_smp_affinity_query(void);

unsigned char ps3_latency_perf_mode_query(void);

void ps3_latency_perf_mode_modify(unsigned char enable);

int ps3_max_sge_cnt_query(void);

void ps3_max_sge_cnt_modify(int value);

#ifndef _WINDOWS
#endif

#endif
