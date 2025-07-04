/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_IRQFLAGS_H
#define _ASM_SW64_IRQFLAGS_H

#include <asm/csr.h>
#include <asm/hmcall.h>
#include <asm/hw_init.h>

#define IPL_MIN		0	// enabled
#define IPL_MAX		7	// disabled

#define getipl()		(rdps() & 7)
#define setipl(ipl)		((void) swpipl(ipl))

#define CSR_IRQ_ENABLED		(-1UL)

#ifdef MODULE
#undef is_in_host
#define is_in_host()		0
#endif

#ifndef CONFIG_SUBARCH_C3B

static inline unsigned long host_save_flags_fast(void)
{
	return sw64_read_csr(CSR_INT_EN) == CSR_IRQ_ENABLED ?
		IPL_MIN : IPL_MAX;
}

static inline void host_irq_enable_fast(void)
{
	sw64_write_csr(CSR_IRQ_ENABLED, CSR_INT_EN);
	if (unlikely(!sw64_read_csr(CSR_NO_IRQ_PENDING)))
		imemb();
}

static inline void host_irq_disable_fast(void)
{
	sw64_write_csr(sw64_read_csr(CSR_NMI_MASK), CSR_INT_EN);
}

static inline unsigned long host_irq_save_fast(void)
{
	unsigned long flags = host_save_flags_fast();

	host_irq_disable_fast();
	return flags;
}

static inline void host_irq_restore_fast(unsigned long flags)
{
	flags == IPL_MIN ?
		host_irq_enable_fast() : host_irq_disable_fast();
}

static inline bool host_irqs_disabled_fast(void)
{
	return sw64_read_csr(CSR_INT_EN) == CSR_IRQ_ENABLED ? false : true;
}

#else /* CONFIG_SUBARCH_C3B */

/*
 * Ensure calls are optimized out by the compiler,
 * otherwise a linker error is raised.
 */
extern void irqflags_linker_error(void);

#define host_save_flags_fast()				\
({							\
	unsigned long __x = 0;				\
	irqflags_linker_error();			\
	__x;						\
})

#define host_irq_enable_fast()				\
({							\
	irqflags_linker_error();			\
})

#define host_irq_disable_fast()				\
({							\
	irqflags_linker_error();			\
})

#define host_irq_save_fast()				\
({							\
	unsigned long __x = 0;				\
	irqflags_linker_error();			\
	__x;						\
})

#define host_irq_restore_fast(flags)			\
({							\
	irqflags_linker_error();			\
})

#define host_irqs_disabled_fast()			\
({							\
	bool __x = 0;					\
	irqflags_linker_error();			\
	__x;						\
})

#endif /* !CONFIG_SUBARCH_C3B */

static inline unsigned long arch_local_save_flags(void)
{
	if (is_in_host() && !IS_ENABLED(CONFIG_SUBARCH_C3B))
		return host_save_flags_fast();

	return getipl();
}

static inline void arch_local_irq_disable(void)
{
	if (is_in_host() && !IS_ENABLED(CONFIG_SUBARCH_C3B))
		return host_irq_disable_fast();

	setipl(IPL_MAX);
	barrier();
}

static inline unsigned long arch_local_irq_save(void)
{
	unsigned long flags;

	if (is_in_host() && !IS_ENABLED(CONFIG_SUBARCH_C3B))
		return host_irq_save_fast();

	flags = swpipl(IPL_MAX);
	barrier();
	return flags;
}

static inline void arch_local_irq_enable(void)
{
	if (is_in_host() && !IS_ENABLED(CONFIG_SUBARCH_C3B))
		return host_irq_enable_fast();

	barrier();
	setipl(IPL_MIN);
}

static inline void arch_local_irq_restore(unsigned long flags)
{
	if (is_in_host() && !IS_ENABLED(CONFIG_SUBARCH_C3B))
		return host_irq_restore_fast(flags);

	barrier();
	setipl(flags);
	barrier();
}

static inline bool arch_irqs_disabled_flags(unsigned long flags)
{
	return flags > IPL_MIN;
}

static inline bool arch_irqs_disabled(void)
{
	if (is_in_host() && !IS_ENABLED(CONFIG_SUBARCH_C3B))
		return host_irqs_disabled_fast();

	return arch_irqs_disabled_flags(arch_local_save_flags());
}

#endif /* _ASM_SW64_IRQFLAGS_H */
