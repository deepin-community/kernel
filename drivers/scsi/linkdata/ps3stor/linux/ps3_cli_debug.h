/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_CLI_DEBUG_H_
#define _PS3_CLI_DEBUG_H_
#include "ps3_instance_manager.h"

ssize_t ps3_ioc_reg_dump(struct ps3_instance *instance, char *buf);

void ps3_cli_debug_init(void);

void ps3_io_statis_dump_cli_cb_test(unsigned char detail);

unsigned char ps3_get_wait_cli_flag(void);
#endif
