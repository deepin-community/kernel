/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Reset a DECstation machine.
 *
 * File created to eliminate warnings; copyright from reset.c.
 *
 * Copyright (C) 199x  the Anonymous
 * Copyright (C) 2001, 2002, 2003  Maciej W. Rozycki
 */

#ifndef __ASM_DEC_RESET_H

#include <linux/interrupt.h>

extern void __noreturn dec_machine_restart(char *command);
extern void __noreturn dec_machine_halt(void);
extern void __noreturn dec_machine_power_off(void);
extern irqreturn_t dec_intr_halt(int irq, void *dev_id);

#endif /* __ASM_DEC_RESET_H */
