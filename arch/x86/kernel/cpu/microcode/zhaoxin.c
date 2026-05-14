// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Zhaoxin CPU Microcode Update Driver for Linux
 *
 * Author: Lyle Li <LyleLi@zhaoxin.com>
 *
 */
#define pr_fmt(fmt) "microcode: " fmt
#include <linux/earlycpio.h>
#include <linux/firmware.h>
#include <linux/uaccess.h>
#include <linux/initrd.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/uio.h>
#include <linux/mm.h>
#include <linux/io.h>

#include <asm/cpu_device_id.h>
#include <asm/processor.h>
#include <asm/tlbflush.h>
#include <asm/setup.h>
#include <asm/msr.h>

#include "internal.h"

struct microcode_header_zhaoxin {
	u32 signature;
	u32 reserved1;
	u32 year:16;
	u32 day:8;
	u32 month:8;
	u32 applicable_processor;
	u32 checksum;
	u32 ldr_rev;
	u8 chip_pf;
	u8 sku_flag;
	u16 update_rev_small_low;
	u32 data_size;
	u32 total_size;
	u16 reserved2;
	u32 update_rev;
	u16 reserved3;
	u16 signed_flag;
	u16 update_rev_small_high;
} __packed;

struct microcode_zhaoxin {
	struct microcode_header_zhaoxin hdr;
	unsigned int data[];
};

static pgd_t microcode_pgd_entry;
static pud_t *microcode_pud_page;
static p4d_t *microcode_p4d_page;
static const char ucode_path[] = "kernel/x86/microcode/Zhaoxinucode.bin";

#define ZHAOXIN_MICROCODE_HEADER 0x53415252
#define ZHAOXIN_MC_HEADER_SIZE sizeof(struct microcode_header_zhaoxin)
#define UCODE_BSP_LOADED ((struct microcode_zhaoxin *)0x1UL)
#define ZHAOXIN_MSR_PF 0x1631
#define ZHAOXIN_MSR_FCR5_PATCH_ERROR_CODE 0x1205
#define IS_HEX(c) \
	(((c) >= '0' && (c) <= '9') || ((c) >= 'A' && (c) <= 'F') || ((c) >= 'a' && (c) <= 'f'))

/*
 * Current microcode patch used on both the BSP and APs
 * during the resume phase
 */
static struct microcode_zhaoxin *zhaoxin_ucode_patch __read_mostly;

static inline u32 get_microcode_revision(bool small)
{
	u32 rev, rev_small;

	native_wrmsrl(MSR_IA32_UCODE_REV, 0);

	/* As documented in the SDM: Do a CPUID 1 here */
	native_cpuid_eax(1);

	/* get the current revision from MSR 0x8B */
	native_rdmsr(MSR_IA32_UCODE_REV, rev_small, rev);

	return small ? rev_small : rev;
}

static int zhaoxin_collect_cpu_info(int cpu_num, struct cpu_signature *sig)
{
	sig->sig = cpuid_eax(1);
	sig->rev = get_microcode_revision(true);

	sig->pf = get_microcode_revision(false);

	return 0;
}

static inline u32 get_microcode_header_update_revision(struct microcode_header_zhaoxin *mc_hdr)
{
	const u8 *bytes = (const u8 *)&mc_hdr->update_rev;
	u32 byte0, byte1, byte2, byte3;

	if (!IS_HEX(bytes[0]) || !IS_HEX(bytes[1]) || !IS_HEX(bytes[2]) || !IS_HEX(bytes[3]))
		return 0;

	byte0 = (bytes[0] <= '9') ? (bytes[0] - '0') : ((bytes[0] & 0xDF) - 'A' + 0xA);
	byte1 = (bytes[1] <= '9') ? (bytes[1] - '0') : ((bytes[1] & 0xDF) - 'A' + 0xA);
	byte2 = (bytes[2] <= '9') ? (bytes[2] - '0') : ((bytes[2] & 0xDF) - 'A' + 0xA);
	byte3 = (bytes[3] <= '9') ? (bytes[3] - '0') : ((bytes[3] & 0xDF) - 'A' + 0xA);

	return (byte0 << 12) | (byte1 << 8) | (byte2 << 4) | byte3;
}

static inline bool cpu_signatures_match(struct cpu_signature *s1, unsigned int sig2,
					unsigned int pf2)
{
	return s1->sig == sig2 && s1->pf == pf2;
}

static bool zhaoxin_find_matching_signature(void *mc, struct cpu_signature *sig)
{
	struct microcode_header_zhaoxin *mc_hdr = mc;
	u32 chip_pf, dummy, sku_flag;

	/* verfiy cpu signature and revision */
	if (!cpu_signatures_match(sig, mc_hdr->applicable_processor,
				  get_microcode_header_update_revision(mc_hdr)))
		return false;

	native_rdmsr(ZHAOXIN_MSR_PF, dummy, chip_pf);
	chip_pf &= ((0x1 << 8) - 1);
	if (mc_hdr->chip_pf != chip_pf)
		return false;

	native_rdmsr(ZHAOXIN_MSR_FCR5_PATCH_ERROR_CODE, sku_flag, dummy);
	sku_flag = (sku_flag & ((0x1 << 21) - 1)) >> 13;

	return mc_hdr->sku_flag == sku_flag;
}

static int zhaoxin_microcode_sanity_check(void *mc, bool print_err, int hdr_type)
{
	struct microcode_header_zhaoxin *mc_header = mc;
	u32 check_sum = 0, i;

	/* verify microcode data size */
	if (mc_header->data_size + ZHAOXIN_MC_HEADER_SIZE > mc_header->total_size) {
		if (print_err)
			pr_err("Error: bad microcode data file size.\n");
		return -EINVAL;
	}

	/* verify loader_version and header signature */
	if (mc_header->ldr_rev != 1 || mc_header->signature != hdr_type) {
		if (print_err)
			pr_err("Error: invalid/unknown microcode update format. Header type %d\n",
			       mc_header->signature);
		return -EINVAL;
	}

	/* Calculate the checksum of update data and header. */
	check_sum = 0;
	i = mc_header->total_size / sizeof(u32);
	while (i--)
		check_sum += ((u32 *)mc)[i];

	if (check_sum) {
		if (print_err)
			pr_err("Bad microcode data checksum, aborting.\n");
		return -EINVAL;
	}

	return 0;
}

static void save_microcode_patch(struct microcode_zhaoxin *patch)
{
	unsigned int size = patch->hdr.total_size;
	struct page *pg = NULL;
	void *dst = NULL;

	/*
	 * Due to hardware limitations, the ucode must reside within the 4G
	 * address space. Therefore, the GFP_DMA32 flag is used to restrict
	 * the memory allocation to this range.
	 */
	pg = alloc_pages(GFP_DMA32 | GFP_KERNEL, get_order(size));
	if (!pg) {
		pr_err("Unable to allocate microcode memory size: %u\n", size);
		return;
	}

	dst = page_address(pg);
	memcpy(dst, patch, size);
	zhaoxin_ucode_patch = dst;
}

static inline u32
get_microcode_header_update_revision_small(struct microcode_header_zhaoxin *mc_hdr)
{
	const u8 *bytes_low = (const u8 *)&mc_hdr->update_rev_small_low;
	const u8 *bytes_high = (const u8 *)&mc_hdr->update_rev_small_high;
	u32 byte0, byte1, byte2, byte3;

	if (!IS_HEX(bytes_low[0]) || !IS_HEX(bytes_low[1]) || !IS_HEX(bytes_high[0]) ||
	    !IS_HEX(bytes_high[1]))
		return 0;

	byte0 = (bytes_low[0] <= '9') ? (bytes_low[0] - '0') : ((bytes_low[0] & 0xDF) - 'A' + 0xA);
	byte1 = (bytes_low[1] <= '9') ? (bytes_low[1] - '0') : ((bytes_low[1] & 0xDF) - 'A' + 0xA);
	byte2 = (bytes_high[0] <= '9') ? (bytes_high[0] - '0') :
					 ((bytes_high[0] & 0xDF) - 'A' + 0xA);
	byte3 = (bytes_high[1] <= '9') ? (bytes_high[1] - '0') :
					 ((bytes_high[1] & 0xDF) - 'A' + 0xA);

	return (byte0 << 12) | (byte1 << 8) | (byte2 << 4) | byte3;
}

/* Scan blob for microcode matching the boot CPUs family, model, stepping */
static struct microcode_zhaoxin *scan_microcode(void *data, size_t size, struct ucode_cpu_info *uci,
						bool save)
{
	struct microcode_header_zhaoxin *mc_header;
	struct microcode_zhaoxin *patch = NULL;
	unsigned int mc_size;
	u32 cur_rev = uci->cpu_sig.rev;
	u32 update_rev;

	for (; size >= sizeof(struct microcode_header_zhaoxin); size -= mc_size, data += mc_size) {
		mc_header = (struct microcode_header_zhaoxin *)data;

		mc_size = mc_header->total_size;
		if (!mc_size || mc_size > size ||
		    zhaoxin_microcode_sanity_check(data, false, ZHAOXIN_MICROCODE_HEADER) < 0)
			break;

		if (!zhaoxin_find_matching_signature(data, &uci->cpu_sig))
			continue;

		update_rev = get_microcode_header_update_revision_small(mc_header);
		if (!update_rev)
			continue;
		/*
		 * For saving the early microcode, find the matching revision
		 * which was loaded on both the BSP and APs.
		 *
		 * On both the BSP and APs during early boot, find a newer
		 * revision than actually loaded in the CPU.
		 */
		if (save) {
			if (cur_rev != update_rev)
				continue;
		} else if (cur_rev >= update_rev) {
			continue;
		}

		patch = data;
		cur_rev = update_rev;
	}

	return size ? NULL : patch;
}

static bool verify_patch_load(void)
{
	u32 err_code, dummy;

	/* get ucode update return code from msr 0x1205 */
	native_rdmsr(ZHAOXIN_MSR_FCR5_PATCH_ERROR_CODE, err_code, dummy);

	err_code &= (1 << 8) - 1;

	switch (err_code) {
	case 0:
		pr_err("no update has been attempted since reset\n");
		break;
	case 1:
		return true;
	case 2:
		pr_err("patch mechanism disable\n");
		break;
	case 3:
		pr_err("bad patch header data\n");
		break;
	case 4:
		pr_err("bad patch header checksum\n");
		break;
	case 5:
		pr_err("bad immediate patch data checksum\n");
		break;
	case 6:
		pr_err("bad main patch data checksum\n");
		break;
	case 7:
		pr_err("bad overlay patch data checksum\n");
		break;
	case 8:
		pr_err("patch too big for pram\n");
		break;
	}

	return false;
}

static inline void clear_zhaoxin_err_code_reg(void)
{
	u32 err_code, dummy;

	native_rdmsr(ZHAOXIN_MSR_FCR5_PATCH_ERROR_CODE, err_code, dummy);

	err_code &= ~((1 << 8) - 1);
	native_wrmsr(ZHAOXIN_MSR_FCR5_PATCH_ERROR_CODE, err_code, dummy);
}

static enum ucode_state apply_microcode_early(struct ucode_cpu_info *uci)
{
	struct microcode_zhaoxin *mc = uci->mc;
	u32 rev;

	if (!mc)
		return UCODE_NFOUND;

	clear_zhaoxin_err_code_reg();

	/* write microcode via MSR 0x79 */
	native_wrmsrl(MSR_IA32_UCODE_WRITE, __pa((unsigned long)mc->data));

	if (!verify_patch_load())
		return UCODE_ERROR;

	rev = get_microcode_revision(true);
	if (rev != get_microcode_header_update_revision_small(&mc->hdr))
		return UCODE_ERROR;

	uci->cpu_sig.rev = rev;

	return UCODE_UPDATED;
}

static __init struct microcode_zhaoxin *get_microcode_blob(struct ucode_cpu_info *uci, bool save)
{
	struct cpio_data cp;

	zhaoxin_collect_cpu_info(smp_processor_id(), &uci->cpu_sig);

	cp = find_microcode_in_initrd(ucode_path);

	if (!(cp.data && cp.size))
		return NULL;

	return scan_microcode(cp.data, cp.size, uci, save);
}

static __init pud_t *find_pud_entry(unsigned long addr)
{
	pgd_t *pgd;
	p4d_t *p4d;

	pgd = (pgd_t *)__va(read_cr3_pa());

	pgd = pgd_offset_pgd(pgd, addr);
	if (pgd_none(*pgd) || pgd_bad(*pgd)) {
		pr_err("Invalid PGD entry for 0x%lx\n", addr);
		return NULL;
	}

	p4d = p4d_offset(pgd, addr);
	if (p4d_none(*p4d) || p4d_bad(*p4d)) {
		pr_err("Invalid P4D entry for 0x%lx\n", addr);
		return NULL;
	}

	return pud_offset(p4d, addr);
}

/*
 * Due to hardware limitations, when loading the microcode,
 * it is necessary to ensure that the physical address is
 * equal to the virtual address. When the APs core loads
 * the ucode, the PAGE_OFFSET has already been randomized.
 * Therefore, this function is called during early initialization
 * to set up a 4G identity-mapped pgd entry.
 */
static __init void init_microcode_pages(void)
{
	pud_t *pud;
	unsigned int i;
	pgdval_t pgd_flags = _PAGE_PRESENT | _PAGE_RW | _PAGE_USER;

	microcode_pud_page = (pud_t *)__get_free_page(GFP_KERNEL_ACCOUNT);
	if (!microcode_pud_page) {
		pr_err("Failed to allocate PUD page\n");
		goto exit;
	}
	memset(microcode_pud_page, 0, PAGE_SIZE);

	if (pgtable_l5_enabled()) {
		microcode_p4d_page = (p4d_t *)__get_free_page(GFP_KERNEL_ACCOUNT);
		if (!microcode_pud_page) {
			pr_err("Failed to allocate P4D page\n");
			goto fail_pud;
		}
		memset(microcode_p4d_page, 0, PAGE_SIZE);
	}

	for (i = 0; i < 4; i++) {
		pud = find_pud_entry(__PAGE_OFFSET + (unsigned long)(i << 30));
		if (pud == NULL)
			goto fail_p4d;
		microcode_pud_page[i] = *pud;
	}

	if (pgtable_l5_enabled()) {
		microcode_p4d_page[0] = __p4d(__pa(microcode_pud_page) | pgd_flags);
		microcode_pgd_entry = __pgd(__pa(microcode_p4d_page) | pgd_flags);
	} else
		microcode_pgd_entry = __pgd(__pa(microcode_pud_page) | pgd_flags);

	goto exit;

fail_p4d:
	if (pgtable_l5_enabled())
		free_page((unsigned long)microcode_p4d_page);

fail_pud:
	free_page((unsigned long)microcode_pud_page);
	pr_err("Microcode PUD initialized failed.\n");
	microcode_pgd_entry.pgd = 0;
exit:
	return;
}

/*
 * Invoked from an early init call to save the microcode blob which was
 * selected during early boot when mm was not usable. The microcode must be
 * saved because initrd is going away. It's an early init call so the APs
 * just can use the pointer and do not have to scan initrd/builtin firmware
 * again.
 */
static __init int save_builtin_microcode(void)
{
	struct ucode_cpu_info uci;

	if (IS_ENABLED(CONFIG_X86_32))
		return 0;

	if (xchg(&zhaoxin_ucode_patch, NULL) != UCODE_BSP_LOADED)
		return 0;

	if (boot_cpu_data.x86_vendor != X86_VENDOR_ZHAOXIN &&
	    boot_cpu_data.x86_vendor != X86_VENDOR_CENTAUR)
		return 0;

	if (dis_ucode_ldr)
		return 0;

	uci.mc = get_microcode_blob(&uci, true);
	if (uci.mc) {
		save_microcode_patch(uci.mc);
		if (zhaoxin_ucode_patch) {
			init_microcode_pages();
			return 0;
		}
		pr_err("%s: CPU %d save microcode patch failed due to page allocation failure\n",
		       __func__, smp_processor_id());
	}
	return 0;
}
early_initcall(save_builtin_microcode);

/* Load microcode on BSP from initrd */
void __init load_ucode_zhaoxin_bsp(struct early_load_data *ed)
{
	struct ucode_cpu_info uci;

	if (IS_ENABLED(CONFIG_X86_32))
		return;

	uci.mc = get_microcode_blob(&uci, false);
	ed->old_rev = uci.cpu_sig.rev;

	/*
	 * Due to hardware limitations, when loading the microcode,
	 * it is necessary to ensure that the physical address is
	 * equal to the virtual address. Therefore, an identity mapping
	 * is created for the physical address where the ucode data resides.
	 */
	early_top_pgt[0] = early_top_pgt[pgd_index(__PAGE_OFFSET)];
	__native_tlb_flush_global(this_cpu_read(cpu_tlbstate.cr4));

	if (uci.mc && apply_microcode_early(&uci) == UCODE_UPDATED) {
		zhaoxin_ucode_patch = UCODE_BSP_LOADED;
		x86_cpuinit.parallel_bringup = false;
		ed->new_rev = uci.cpu_sig.rev;
	} else if (uci.mc) {
		pr_debug("%s: BSP CPU %d early update failed due to application failure\n",
			__func__, smp_processor_id());
	}
}

/* Load microcode on AP cores */
void load_ucode_zhaoxin_ap(void)
{
	struct ucode_cpu_info uci;
	pgd_t *pgd_pgt;

	if (IS_ENABLED(CONFIG_X86_32))
		return;

	if (!zhaoxin_ucode_patch)
		return;

	if (!microcode_pgd_entry.pgd) {
		pr_err("%s: CPU %d early update failed due to page initialization failure\n",
		       __func__, smp_processor_id());
		return;
	}

	uci.mc = zhaoxin_ucode_patch;

	/*
	 * Due to hardware limitations, the ucode loading
	 * must use identity mapping.
	 */
	pgd_pgt = __va(read_cr3_pa());
	pgd_pgt[0] = microcode_pgd_entry;
	__flush_tlb_all();

	if (apply_microcode_early(&uci) != UCODE_UPDATED)
		pr_debug("%s: CPU %d early update failed due to microcode application failure\n",
			__func__, smp_processor_id());
}

/* Reload microcode on resume */
void reload_ucode_zhaoxin(void)
{
	struct ucode_cpu_info uci = {
		.mc = zhaoxin_ucode_patch,
	};
	pgd_t old_pgd, *pgd_pgt;

	if (!zhaoxin_ucode_patch)
		return;

	if (!microcode_pgd_entry.pgd) {
		pr_err("%s: BSP CPU %d reload update failed due to page initialization failure\n",
		       __func__, smp_processor_id());
		return;
	}

	/*
	 * Due to hardware limitations, the ucode loading must
	 * use identity mapping.
	 */
	pgd_pgt = __va(read_cr3_pa());
	old_pgd = pgd_pgt[0];
	pgd_pgt[0] = microcode_pgd_entry;
	__flush_tlb_all();

	if (uci.mc)
		if (apply_microcode_early(&uci) != UCODE_UPDATED)
			pr_debug("%s: BSP CPU %d reload update failed due to application failure\n",
				__func__, smp_processor_id());

	/*
	 * Since the page table in use at this point might be that of
	 * a user-space process, it needs to be restored.
	 */
	pgd_pgt[0] = old_pgd;
	__flush_tlb_all();
}

static enum ucode_state apply_microcode_late(int cpu)
{
	/* Zhaoxin CPUs currently do not support runtime microcode updates. */
	return UCODE_NFOUND;
}

static enum ucode_state request_microcode_fw(int cpu, struct device *device)
{
	/* Zhaoxin CPUs currently do not support runtime microcode updates. */
	return UCODE_NFOUND;
}

static struct microcode_ops microcode_zhaoxin_ops = {
	.request_microcode_fw = request_microcode_fw,
	.collect_cpu_info = zhaoxin_collect_cpu_info,
	.apply_microcode = apply_microcode_late,
};

struct microcode_ops *__init init_zhaoxin_microcode(void)
{
	struct cpuinfo_x86 *c = &boot_cpu_data;

	if (IS_ENABLED(CONFIG_X86_32))
		return NULL;

	if ((cpuid_eax(0xC0000000) < 0xC0000004) || !(cpuid_edx(0xC0000004) & 0x1)) {
		pr_info("Zhaoxin CPU family 0x%x model 0x%x not supported\n", c->x86, c->x86_model);
		return NULL;
	}

	return &microcode_zhaoxin_ops;
}
