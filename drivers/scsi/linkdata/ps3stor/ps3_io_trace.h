/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_IO_TRACE_H_
#define _PS3_IO_TRACE_H_

#include "ps3_cmd_channel.h"

#define PS3_IO_TRACE_PRINT_COUNT (32)
#define PS3_IO_TRACE_BUF_LEN (256)

enum ps3_io_trace_dtype {
	PS3_IO_TRACE_DIRECT_SEND,
	PS3_IO_TRACE_DIRECT_RECV,
	PS3_IO_TRACE_DIRECT_COUNT,
};

void ps3_scsih_io_trace(const struct ps3_cmd *cmd,
			enum ps3_io_trace_dtype type);
#define PS3_IO_TRACE(cmd, type)                                                \
	do {                                                                   \
		if ((cmd)->instance->debug_context.io_trace_switch ==          \
		    PS3_FALSE) {                                               \
			break;                                                 \
		}                                                              \
		ps3_scsih_io_trace((cmd), (type));                             \
	} while (0)

#endif
