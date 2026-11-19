/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_CLI_H_
#define _PS3_CLI_H_

#define PS3_CLI_INPUT_LEN 2048
#define PS3_CLI_OUTPUT_MAX (16 * 1024 * 1024)
#define PS3_CLI_OUTLINE_LEN 4096
#define PS3_CLI_HELP_LEN 256
#define PS3_CLI_CMD_MAXLEN 32
#define PS3_MAX_ARGV 64

int ps3stor_cli_register(void (*func)(int argc, char *argv[]), const char *cmd_str,
			 const char *help);

int ps3stor_cli_printf(const char *fmt, ...);

int ps3cmd_init(void);

void ps3cmd_exit(void);


#endif

