// SPDX-License-Identifier: GPL-2.0
/*
 * Support for kernel relocation at boot time.
 *
 * Based on arch/mips/kernel/relocate.c
 *
 * Copyright (C) 2019 He Sheng
 * Authors: He Sheng (hesheng05@gmail.com)
 */
#include <linux/elf.h>
#include <linux/notifier.h>
#include <linux/panic_notifier.h>
#include <linux/mm.h>

#include <asm/sections.h>

#define KTEXT_MAX    0xffffffffa0000000UL
#define RELOCATED(x) ((void *)((unsigned long)x + offset))

extern unsigned long _got_start[];
extern unsigned long _got_end[];
extern char pre_start_kernel[];

extern unsigned int _relocation_start[];	/* End kernel image / start relocation table */
extern unsigned int _relocation_end[];	/* End relocation table */

extern unsigned long __start___ex_table;	/* Start exception table */
extern unsigned long __stop___ex_table;	/* End exception table */
extern union thread_union init_thread_union;

/*
 * This function may be defined for a platform to perform any post-relocation
 * fixup necessary.
 * Return non-zero to abort relocation
 */
int __weak plat_post_relocation(long offset)
{
	return 0;
}

static int __init apply_r_sw64_refquad(unsigned long *loc_orig, unsigned long *loc_new, unsigned int offset)
{
	*(unsigned long *)loc_new += offset;

	return 0;
}

static int (*reloc_handlers_rel[]) (unsigned long *, unsigned long *, unsigned int) __initdata = {
	[R_SW64_REFQUAD]		= apply_r_sw64_refquad,
};

int __init do_relocations(void *kbase_old, void *kbase_new, unsigned int offset)
{
	unsigned int *r;
	unsigned long *loc_orig;
	unsigned long *loc_new;
	int type;
	int res;

	for (r = _relocation_start; r < _relocation_end; r++) {
		/* Sentinel for last relocation */
		if (*r == 0)
			break;

		type = (*r >> 24) & 0xff;
		loc_orig = kbase_old + ((*r & 0x00ffffff) << 2);
		loc_new = RELOCATED(loc_orig);

		if (reloc_handlers_rel[type] == NULL) {
			/* Unsupported relocation */
			pr_err("Unhandled relocation type %d at 0x%pK\n",
			       type, loc_orig);
			return -ENOEXEC;
		}

		res = reloc_handlers_rel[type](loc_orig, loc_new, offset);
		if (res)
			return res;
	}

	return 0;
}

static int __init relocate_got(unsigned int offset)
{
	unsigned long *got_start, *got_end, *e;

	got_start = RELOCATED(&_got_start);
	got_end = RELOCATED(&_got_end);

	for (e = got_start; e < got_end; e++)
		*e += offset;

	return 0;
}

#ifdef CONFIG_RANDOMIZE_BASE

static inline __init unsigned long rotate_xor(unsigned long hash,
					      const void *area, size_t size)
{
	size_t i;
	unsigned long start, *ptr;
	/* Make sure start is 8 byte aligned */
	start = ALIGN((unsigned long)area, 8);
	size -= (start - (unsigned long)area);
	ptr = (unsigned long *) start;
	for (i = 0; i < size / sizeof(hash); i++) {
		/* Rotate by odd number of bits and XOR. */
		hash = (hash << ((sizeof(hash) * 8) - 7)) | (hash >> 7);
		hash ^= ptr[i];
	}
	return hash;
}

static inline __init unsigned long get_random_boot(void)
{
	unsigned long entropy = random_get_entropy();
	unsigned long hash = 0;

	/* Attempt to create a simple but unpredictable starting entropy. */
	hash = rotate_xor(hash, linux_banner, strlen(linux_banner));

	/* Add in any runtime entropy we can get */
	hash = rotate_xor(hash, &entropy, sizeof(entropy));

	return hash;
}

static inline __init bool kaslr_disabled(void)
{
	char *str;

	str = strstr(COMMAND_LINE, "nokaslr");
	if (str == COMMAND_LINE || (str > COMMAND_LINE && *(str - 1) == ' '))
		return true;

	return false;
}

static unsigned long __init determine_relocation_offset(void)
{
	/* Choose a new address for the kernel */
	unsigned long kernel_length;
	unsigned long offset;

	if (kaslr_disabled())
		return 0;

	kernel_length = (unsigned long)_end - (unsigned long)(&_text);

	/* TODO: offset is 64K align. maybe 8KB align is okay.  */
	offset = get_random_boot() << 16;
	offset &= (CONFIG_RANDOMIZE_BASE_MAX_OFFSET - 1);
	if (offset < kernel_length)
		offset += ALIGN(kernel_length, 0x10000);

	/*
	 * TODO:new location should not overlaps initrd, dtb, acpi
	 * tables, etc.
	 */

	if ((KTEXT_MAX - (unsigned long)_end) < offset)
		offset = 0;

	return offset;
}

#else

static inline unsigned long __init determine_relocation_offset(void)
{
	/*
	 * Choose a new address for the kernel
	 * For now we'll hard code the destination offset.
	 */
	return 0;
}

#endif

static inline int __init relocation_offset_valid(unsigned long offset)
{
	unsigned long loc_new = (unsigned long)_text + offset;

	if (loc_new & 0x0000ffff) {
		/* Inappropriately aligned new location */
		return 0;
	}
	if (loc_new < (unsigned long)&_end) {
		/* New location overlaps original kernel */
		return 0;
	}
	return 1;
}

unsigned int __init relocate_kernel(void)
{
	void *loc_new;
	unsigned long kernel_length;
	unsigned long bss_length;
	unsigned int offset = 0;
	int res = 1;

	kernel_length = (unsigned long)(&_relocation_start) - (long)(&_text);
	bss_length = (unsigned long)&__bss_stop - (long)&__bss_start;

	offset = determine_relocation_offset();
	/* Reset the command line now so we don't end up with a duplicate */

	/* Sanity check relocation address */
	if (offset && relocation_offset_valid(offset)) {

		loc_new = RELOCATED(&_text);
		/* Copy the kernel to it's new location */
		memcpy(loc_new, &_text, kernel_length);

		/* Perform relocations on the new kernel */
		res = do_relocations(&_text, loc_new, offset);
		if (res < 0)
			goto out;

		res = relocate_got(offset);
		if (res < 0)
			goto out;

		/*
		 * The original .bss has already been cleared, and
		 * some variables such as command line parameters
		 * stored to it so make a copy in the new location.
		 */
		memcpy(RELOCATED(&__bss_start), &__bss_start, bss_length);

		/*
		 * Last chance for the platform to abort relocation.
		 * This may also be used by the platform to perform any
		 * initialisation required now that the new kernel is
		 * resident in memory and ready to be executed.
		 */
		if (plat_post_relocation(offset))
			goto out;

		/* Return the new kernel's offset */
		return offset;
	}
out:
	return 0;
}

/*
 * Show relocation information on panic.
 */
void show_kernel_relocation(const char *level)
{
	unsigned long offset;

	offset = __pa_symbol(_text) - __pa_symbol(_TEXT_START);

	if (IS_ENABLED(CONFIG_RELOCATABLE) && offset > 0) {
		printk(level);
		pr_cont("Kernel relocated by 0x%pK\n", (void *)offset);
		pr_cont(" .text @ 0x%pK\n", _text);
		pr_cont(" .data @ 0x%pK\n", _sdata);
		pr_cont(" .bss  @ 0x%pK\n", __bss_start);
	}
}

static int kernel_location_notifier_fn(struct notifier_block *self,
				       unsigned long v, void *p)
{
	show_kernel_relocation(KERN_EMERG);
	return NOTIFY_DONE;
}

static struct notifier_block kernel_location_notifier = {
	.notifier_call = kernel_location_notifier_fn
};

static int __init register_kernel_offset_dumper(void)
{
	atomic_notifier_chain_register(&panic_notifier_list,
				       &kernel_location_notifier);
	return 0;
}
device_initcall(register_kernel_offset_dumper);
