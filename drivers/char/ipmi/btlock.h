/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __BTLOCK_H__
#define __BTLOCK_H__

#include <linux/delay.h>
#include <asm/timex.h>

union btlock {
	char b[2];
	unsigned int u;
};

/*
 *wait delay us if lock failed.
 *lock fail if another one get lock or both try get lock.
 *c must compile b with byte access.
 */
static inline int btlock_lock(volatile union btlock *p, int n, unsigned char delay)
{
	union btlock t, t1;
	unsigned long flags;
	unsigned long c0 = get_cycles(), c1;

	if (n > 1)
		return -1;
	delay |= 0x80;
	t1.u = 0;
	t1.b[n] = delay;

	while (1) {
		local_irq_save(flags);
		p->b[n] = delay;
		t.u = p->u;
		if (t.u == t1.u) {
			wmb(); /* flush write out immediately */
			local_irq_restore(flags);
			return 0;
		}
		p->b[n] = 0;
		t.u = p->u;
		wmb(); /* flush write out immediately */
		local_irq_restore(flags);
		c1 = get_cycles();
		if (c1 - c0 > *mscycles * 1000)
			return -1;
		ndelay(((t.b[1 - n] & 0x7f) + (c1 & 1)) * 100);
	}
	return 0;
}

static inline int btlock_trylock(volatile union btlock *p, int n, unsigned char delay)
{
	union btlock t, t1;
	unsigned long flags;

	if (n > 1)
		return -1;
	delay |= 0x80;
	t1.u = 0;
	t1.b[n] = delay;

	local_irq_save(flags);
	p->b[n] = delay;
	t.u = p->u;
	if (t.u == t1.u) {
		wmb(); /* flush write out immediately */
		local_irq_restore(flags);
		return 0;
	}
	p->b[n] = 0;
	t.u = p->u;
	wmb(); /* flush write out immediately */
	local_irq_restore(flags);
	ndelay(((t.b[1 - n] & 0x7f) + (get_cycles() & 1)) * 100);
	return -1;
}

static inline int btlock_unlock(volatile union btlock *p, int n)
{
		p->b[n] = 0;
		wmb(); /* flush write out immediately */
		return p->u;
}

static inline int btlock_islocked(volatile union btlock *p, int n)
{
	union btlock t;

	t.u = p->u;
	return t.b[n] && !t.b[1 - n];
}
#endif
