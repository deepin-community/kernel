
#ifndef _PS3_CLI_DEBUG_H_
#define _PS3_CLI_DEBUG_H_
#include "ps3_instance_manager.h"

ssize_t ps3_ioc_reg_dump(struct ps3_instance *instance, char *buf);

void ps3_cli_debug_init(void);

void ps3_io_statis_dump_cli_cb_test(U8 detail);

Bool ps3_get_wait_cli_flag(void);
#ifdef PS3_SUPPORT_INJECT

void ps3_inject_clear(void);
#endif
#endif

