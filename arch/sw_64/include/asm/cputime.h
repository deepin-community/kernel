/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_CPUTIME_H
#define _ASM_SW64_CPUTIME_H

typedef u64 __nocast cputime64_t;

#define jiffies64_to_cputime64(__jif)  ((__force cputime64_t)(__jif))

#endif /* _ASM_SW64_CPUTIME_H */
