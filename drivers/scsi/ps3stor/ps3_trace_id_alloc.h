
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

typedef union {
	U64 trace_id;
	struct {
		U64 count  : 52; 
		U64 cpu_id : 11; 
		U64 flag   : 1;
	} ps3_trace_id;
} ps3_trace_id_u;

void ps3_trace_id_alloc(U64 *trace_id);

void ps3_trace_id_init(void);

S32 ps3_trace_id_switch_store(U8 value);

U8 ps3_trace_id_switch_show(void);

#endif