/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_DELAY_H
#define _ASM_SW64_DELAY_H

extern void __delay(unsigned long loops);
extern void udelay(unsigned long usecs);

extern void ndelay(unsigned long nsecs);
#define ndelay ndelay

#endif /* _ASM_SW64_DELAY_H */
