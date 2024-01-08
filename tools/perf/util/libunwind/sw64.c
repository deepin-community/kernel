// SPDX-License-Identifier: GPL-2.0
/*
 * This file setups defines to compile arch specific binary from the
 * generic one.
 *
 * The function 'LIBUNWIND__ARCH_REG_ID' name is set according to arch
 * name and the defination of this function is included directly from
 * 'arch/arm64/util/unwind-libunwind.c', to make sure that this function
 * is defined no matter what arch the host is.
 *
 * Finally, the arch specific unwind methods are exported which will
 * be assigned to each arm64 thread.
 */

#define REMOTE_UNWIND_LIBUNWIND

/* Define arch specific functions & regs for libunwind, should be
 * defined before including "unwind.h"
 */
#define LIBUNWIND__ARCH_REG_ID(regnum) libunwind__sw_64_reg_id(regnum)
#define LIBUNWIND__ARCH_REG_IP PERF_REG_SW64_PC
#define LIBUNWIND__ARCH_REG_SP PERF_REG_SW64_SP

#include "unwind.h"
#include "debug.h"
#include "libunwind-sw_64.h"
#include <../../../arch/sw_64/include/uapi/asm/perf_regs.h>
#include "../../arch/sw_64/util/unwind-libunwind.c"

#include "util/unwind-libunwind-local.c"

struct unwind_libunwind_ops *
sw64_unwind_libunwind_ops = &_unwind_libunwind_ops;
