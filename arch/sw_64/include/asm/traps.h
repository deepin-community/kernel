/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_TRAPS_H
#define _ASM_SW64_TRAPS_H

#define ENT_IDX_INT		0
#define ENT_IDX_ARITH		1
#define ENT_IDX_MM		2
#define ENT_IDX_IF		3
#define ENT_IDX_UNA		4
#define ENT_IDX_SYS		5

#if defined(CONFIG_SUBARCH_C3B)
/* C3B does not support NMI, so invalid idx */
# define ENT_IDX_NMI		(-1)
# define ENT_IDX_SUSPEND	6
#elif defined(CONFIG_SUBARCH_C4)
# define ENT_IDX_NMI		6
# define ENT_IDX_SUSPEND	7
#endif

#define TRAP_CAUSE_NMI		(-2)
#define TRAP_CAUSE_INT		(-1)
#define TRAP_CAUSE_ARITH	1
#define TRAP_CAUSE_MM		2
#define TRAP_CAUSE_IF		3
#define TRAP_CAUSE_UNA		4

#endif /* _ASM_SW64_TRAPS_H */
