/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 SW64 Limited
 *
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
#ifndef _ASM_SW64_VDSO_H
#define _ASM_SW64_VDSO_H

#ifdef __KERNEL__

/*
 * Default link address for the vDSO.
 * Since we randomise the VDSO mapping, there's little point in trying
 * to prelink this.
 */
#define VDSO_LBASE	0x0

#ifndef __ASSEMBLY__

#include <asm/page.h>
#include <asm/sw64io.h>
#include <asm/processor.h>
#define VDSO_SYMBOL(base, name)						\
({									\
	extern const unsigned long __vdso_##name;			\
	((unsigned long)(base) + __vdso_##name);			\
})


struct vdso_data {
	u64 xtime_sec;
	u64 xtime_nsec;
	u64 wall_to_mono_sec;
	u64 wall_to_mono_nsec;
	u32 cs_shift;
	u32 cs_mult;
	u64 cs_cycle_last;
	u64 cs_mask;
	s32 tz_minuteswest;
	s32 tz_dsttime;
	u32 seq_count;
#ifdef CONFIG_SUBARCH_C3B
	u8 vdso_whami_to_cpu[NR_CPUS];
	u8 vdso_whami_to_node[NR_CPUS];
#endif
#ifdef CONFIG_SUBARCH_C4
	u8 vdso_cpu_to_node[NR_CPUS];
#endif
};

static inline unsigned long get_vdso_base(void)
{
	unsigned long addr, tmp;
	 __asm__ __volatile__(
	"	br	%1, 1f\n"
	"1:	ldi	%0, 0(%1)\n"
	: "=r" (addr), "=&r" (tmp)
	::);

	addr &= ~(PAGE_SIZE - 1);
	return addr;
}

static inline const struct vdso_data *get_vdso_data(void)
{
	return (const struct vdso_data *)(get_vdso_base() - PAGE_SIZE);
}

static inline u32 vdso_data_read_begin(const struct vdso_data *data)
{
	u32 seq;

	while (true) {
		seq = READ_ONCE(data->seq_count);
		if (likely(!(seq & 1))) {
			/* Paired with smp_wmb() in vdso_data_write_*(). */
			smp_rmb();
			return seq;
		}

		cpu_relax();
	}
}

static inline bool vdso_data_read_retry(const struct vdso_data *data,
		u32 start_seq)
{
	/* Paired with smp_wmb() in vdso_data_write_*(). */
	smp_rmb();
	return unlikely(data->seq_count != start_seq);
}

static inline void vdso_data_write_begin(struct vdso_data *data)
{
	++data->seq_count;

	/* Ensure sequence update is written before other data page values. */
	smp_wmb();
}

static inline void vdso_data_write_end(struct vdso_data *data)
{
	/* Ensure data values are written before updating sequence again. */
	smp_wmb();
	++data->seq_count;
}


#endif /* !__ASSEMBLY__ */

#endif /* __KERNEL__ */
#endif /* _ASM_SW64_VDSO_H */
