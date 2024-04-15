// SPDX-License-Identifier: GPL-2.0

#include <linux/suspend.h>
#include <asm/hmcall.h>
#include <asm/suspend.h>

struct processor_state hibernate_state;
/* Defined in hibernate_asm.S */
extern int restore_image(void);

void save_processor_state(void)
{
	struct vcpucb *vcb = &(hibernate_state.vcb);

	vcb->ksp = rdksp();
	vcb->usp = rdusp();
	vcb->soft_tid = rtid();
#if defined(CONFIG_SUBARCH_C3B)
	vcb->ptbr = rdptbr();
#elif defined(CONFIG_SUBARCH_C4)
	vcb->ptbr_usr = sw64_read_csr(CSR_PTBR_USR);
	vcb->ptbr_sys = sw64_read_csr(CSR_PTBR_SYS);
#endif
}

void restore_processor_state(void)
{
	struct vcpucb *vcb = &(hibernate_state.vcb);

	wrksp(vcb->ksp);
	wrusp(vcb->usp);
	wrtp(vcb->soft_tid);
#if defined(CONFIG_SUBARCH_C3B)
	wrptbr(vcb->ptbr);
#elif defined(CONFIG_SUBARCH_C4)
	sw64_write_csr_imb(vcb->ptbr_usr, CSR_PTBR_USR);
	sw64_write_csr_imb(vcb->ptbr_sys, CSR_PTBR_SYS);
#endif
	sflush();
	tbiv();
}

int swsusp_arch_resume(void)
{
	restore_image();
	return 0;
}
/* References to section boundaries */
extern const void __nosave_begin, __nosave_end;
int pfn_is_nosave(unsigned long pfn)
{
	unsigned long nosave_begin_pfn = PFN_DOWN(__pa(&__nosave_begin));
	unsigned long nosave_end_pfn = PFN_UP(__pa(&__nosave_end));

	return	(pfn >= nosave_begin_pfn) && (pfn < nosave_end_pfn);
}

struct restore_data_record {
	unsigned long magic;
};

#define RESTORE_MAGIC	0x0123456789ABCDEFUL

/**
 *	arch_hibernation_header_save - populate the architecture specific part
 *		of a hibernation image header
 *	@addr: address to save the data at
 */
int arch_hibernation_header_save(void *addr, unsigned int max_size)
{
	struct restore_data_record *rdr = addr;

	if (max_size < sizeof(struct restore_data_record))
		return -EOVERFLOW;
	rdr->magic = RESTORE_MAGIC;
	return 0;
}

/**
 *	arch_hibernation_header_restore - read the architecture specific data
 *		from the hibernation image header
 *	@addr: address to read the data from
 */
int arch_hibernation_header_restore(void *addr)
{
	struct restore_data_record *rdr = addr;

	return (rdr->magic == RESTORE_MAGIC) ? 0 : -EINVAL;
}
