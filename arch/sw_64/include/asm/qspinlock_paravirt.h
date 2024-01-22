/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_QSPINLOCK_PARAVIRT_H
#define _ASM_SW64_QSPINLOCK_PARAVIRT_H

extern void __pv_queued_spin_unlock(struct qspinlock *lock);

#endif /* _ASM_SW64_QSPINLOCK_PARAVIRT_H */
