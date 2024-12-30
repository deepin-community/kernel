
#ifndef _PS3_IO_TRACE_H_
#define _PS3_IO_TRACE_H_

#include "ps3_cmd_channel.h"

#define PS3_IO_TRACE_PRINT_COUNT (32)
#define PS3_IO_TRACE_BUF_LEN (256)

typedef enum {
	PS3_IO_TRACE_DIRECT_SEND,
	PS3_IO_TRACE_DIRECT_RECV,
	PS3_IO_TRACE_DIRECT_COUNT,
}ps3_io_trace_dtype;

void ps3_scsih_io_trace(const struct ps3_cmd *cmd, ps3_io_trace_dtype type);
#define PS3_IO_TRACE(cmd, type) do { \
	if ((cmd)->instance->debug_context.io_trace_switch == PS3_FALSE) { \
		break; \
	} \
	ps3_scsih_io_trace((cmd),(type)); \
} while(0);

#endif
