/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_IOC_STATE_H_
#define _PS3_IOC_STATE_H_

#include "ps3_instance_manager.h"

enum {
	PS3_FW_STATE_TO_READY_TMO_LOOP_COUNT_RAID = 18000,
	PS3_FW_STATE_TO_READY_TMO_LOOP_COUNT_HBA = 18000,
	PS3_WAIT_FOR_OUTSTANDING_IO_COMPLETE = 9000,
	PS3_FW_STATE_TO_READY_TMO_LOOP_COUNT_SWITCH = 15000,
	PS3_FW_STATE_TO_READY_TMO_LOOP_COUNT_RAID_HAPS = 3600,
	PS3_FW_STATE_TO_RUNNING_TMO_LOOP_COUNT = 9000,
	PS3_FW_STATE_TO_FAULT_TMO_LOOP_COUNT = 1500,
	PS3_FW_STATE_TO_HALT_TMO_LOOP_COUNT = 9000,
	PS3_FW_STATE_TO_UNLOAD_TMO_LOOP_COUNT = 500,
	PS3_WAIT_EVENT_CMD_LOOP_COUNT = 100,
	PS3_LOOP_TIME_INTERVAL_1MS = 1,
	PS3_LOOP_TIME_INTERVAL_20MS = 20,
	PS3_LOOP_TIME_INTERVAL_50MS = 50,
	PS3_LOOP_TIME_INTERVAL_100MS = 100,
	PS3_LOOP_TIME_INTERVAL_3000MS = 3000,
	PS3_WRITE_HARD_RESET_WAIT_TIME_500MS = 500,
	PS3_RECOVERY_WAIT_PROBE_FINISH_LOOP_COUNT = 9000,
	PS3_RECOVERY_WAIT_LOOP_TIME_INTERVAL_20MS = 20,
	PS3_PS3_LOOP_TIME_INTERVAL_1000MS = 1000,
	PS3_TRANS_DEAD_IOC_FAILED_MAX_COUNT = 10,
};
enum ps3_reset_time_type {
	PS3_START_WRITE_KEY_REG = 0,
	PS3_END_WRITE_KEY_REG = 1,
	PS3_START_WAIT_KEY_READY_REG = 2,
	PS3_END_WAIT_KEY_READY_REG = 3,
	PS3_START_WRITE_HARDRESET_REG = 4,
	PS3_END_WRITE_HARDRESET_REG = 5,
	PS3_RESET_MAX_COUNT = 6,
};

#define PS3_HARD_RESET_FORCE_STOP_MAX_TIME_FIXED                               \
	(PS3_RECOVERY_WAIT_PROBE_FINISH_LOOP_COUNT *                           \
		 PS3_RECOVERY_WAIT_LOOP_TIME_INTERVAL_20MS / HZ +              \
	 PS3_FW_STATE_TO_FAULT_TMO_LOOP_COUNT * PS3_LOOP_TIME_INTERVAL_20MS /  \
		 HZ +                                                          \
	 PS3_LOOP_TIME_INTERVAL_100MS * PS3_LOOP_TIME_INTERVAL_100MS / HZ +    \
	 PS3_WRITE_HARD_RESET_WAIT_TIME_500MS / HZ +                           \
	 PS3_SLEEP_TOLERATE_TIMEOUT)

#ifdef PS3_HARDWARE_HAPS_V200
#define PS3_HARD_RESET_FORCE_STOP_MAX_TIME(ins)                                \
	(PS3_HARD_RESET_FORCE_STOP_MAX_TIME_FIXED +                            \
	 (ins)->wait_ready_timeout * PS3_LOOP_TIME_INTERVAL_3000MS / HZ)

#else
#define PS3_HARD_RESET_FORCE_STOP_MAX_TIME(ins)                                \
	(PS3_HARD_RESET_FORCE_STOP_MAX_TIME_FIXED +                            \
	 (ins)->wait_ready_timeout * PS3_LOOP_TIME_INTERVAL_20MS / HZ)
#endif

int ps3_ioc_state_transfer_to_ready(struct ps3_instance *instance);

int ps3_ioc_state_transfer_wait_to_running(struct ps3_instance *instance);

int ps3_ioc_state_hard_reset(struct ps3_instance *instance);

int ps3_ioc_state_shallow_soft_reset(struct ps3_instance *instance);

int ps3_ioc_state_deep_soft_reset(struct ps3_instance *instance);

int ps3_ioc_state_force_to_fault(struct ps3_instance *instance);

int ps3_ioc_state_force_to_halt(struct ps3_instance *instance);

int ps3_ioc_notify_unload(struct ps3_instance *instance);

#ifdef PS3_HARDWARE_ASIC
unsigned int ps3_ioc_heartbeat_detect(struct ps3_instance *instance);
#endif

int ps3_ioc_state_ready_wait(struct ps3_instance *instance);

int ps3_ioc_state_fault_wait(struct ps3_instance *instance);

const char *ps3_ioc_state_print(unsigned int state);

#endif
