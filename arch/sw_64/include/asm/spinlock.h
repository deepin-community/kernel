/* SPDX-License-Identifier: GPL-2.0 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _ASM_SW64_SPINLOCK_H
#define _ASM_SW64_SPINLOCK_H

#include <asm/qspinlock.h>
#include <asm/qrwlock.h>

/* See include/linux/spinlock.h */
#define smp_mb__after_spinlock()	smp_mb()

#endif /* _ASM_SW64_SPINLOCK_H */
