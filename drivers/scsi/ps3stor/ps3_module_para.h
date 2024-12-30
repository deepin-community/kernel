
#ifndef _PS3_MODULE_PARA_H_
#define _PS3_MODULE_PARA_H_

#include "ps3_types.h"

#define MB_TO_BYTE(MB) ((MB) << 20)
#define PS3_MAX_FUNC_ID		(2)

U32 ps3_throttle_qdepth_query(void);

void ps3_debug_mem_size_modify(U32 size);

U32 ps3_debug_mem_size_query(void);

U32 ps3_sata_direct_query(void);

void ps3_sata_direct_modify(U32 val);

U16 ps3_use_clustering_query(void);

void ps3_scsi_cmd_timeout_modify(U32 val);

U32 ps3_scsi_cmd_timeout_query(void);

void ps3_scsi_cmd_timeout_adjust(void);

U32 ps3_r1x_lock_flag_quiry(void);

void ps3_r1x_lock_flag_modify(U32 val);

U32 ps3_direct_to_normal_query(void);

void ps3_direct_to_normal_modify(U32 val);

U32 ps3_hba_check_time_query(void);

U32 ps3_task_reset_delay_time_query(void);

U64 ps3_r1x_ring_size_query(void);

void ps3_r1x_ring_size_modify(U32 size);

U32 ps3_direct_check_stream_query(void);

S32 ps3_device_busy_threshold_query(void);

void ps3_device_busy_threshold_modify(S32 busy);

void ps3_log_level_modify(U32 level);

char* ps3_log_path_query(void);

U32 ps3_log_space_size_query(void);

U32 ps3_log_file_size_query(void);

void ps3_log_file_size_modify(U32 size);

U32 ps3_log_level_query(void);

U32 ps3_log_tty_query(void);

void ps3_log_tty_modify(U32 enable);

void ps3_hard_reset_enable_modify(U32 val);
void ps3_deep_soft_reset_enable_modify(U32 val);

U32 ps3_hard_reset_enable_query(void);
U32 ps3_deep_soft_reset_enable_query(void);

U32 ps3_log_level_query(void);

U32 ps3_aer_handle_support_query(void);

void ps3_aer_handle_support_set(U32 aer_handle_support);

void ps3_version_verbose_fill(void);

U32 ps3_hard_reset_waiting_query(void);

U32 ps3_use_hard_reset_reg_query(void);
U32 ps3_use_hard_reset_max_retry(void);

U32 ps3_enable_heartbeat_query(void);

U32 ps3_enable_heartbeat_set(U32 val);

U32 ps3_hil_mode_query(void);

void ps3_hil_mode_modify(U32 val);

U32 ps3_avaliable_func_id_query(void);

void ps3_avaliable_func_id_modify(U32 val);

void  ps3_direct_check_stream_modify(U32 val);

U32 ps3_r1x_tmo_query(void);

U32 ps3_r1x_conflict_queue_support_query(void);

U32 ps3_pci_irq_mode_query(void);

#if defined(PS3_TAGSET_SUPPORT)

Bool ps3_tagset_enable_query(void);
#endif

Bool ps3_smp_affinity_query(void);

#ifndef _WINDOWS
#ifdef PS3_SUPPORT_INJECT
U32 ps3_err_inject_num_query(void);
U32* ps3_err_inject_array_query(void);
U32 ps3_err_inject_state_query(void);

#endif
#endif

#endif
