/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_TRACE_ID_ALLOC_H_
#define _PS3_TRACE_ID_ALLOC_H_

#include "ps3_htp_def.h"

enum {
	ps3_trace_id_flag_host_driver = 0,
};

enum {
	ps3_trace_id_switch_open,
	ps3_trace_id_switch_close,
};

union ps3_trace_id {
	unsigned long long trace_id;
	struct {
		unsigned long long count : 52;
		unsigned long long cpu_id : 11;
		unsigned long long flag : 1;
	} ps3_trace_id;
};

void ps3_trace_id_alloc(unsigned long long *trace_id);

void ps3_trace_id_init(void);

int ps3_trace_id_switch_store(unsigned char value);

unsigned char ps3_trace_id_switch_show(void);

#endif
