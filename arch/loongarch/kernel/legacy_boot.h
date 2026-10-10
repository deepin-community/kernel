#ifndef __LEGACY_BOOT_H_
#define __LEGACY_BOOT_H_

#include <linux/efi.h>

#define LOONGARCH_BPI_GUID  EFI_GUID(0x4660f721, 0x2ec5, 0x416a,  0x89, 0x9a, 0x43, 0x18, 0x02, 0x50, 0xa0, 0xc9)

struct loongarch_bpi_info {
	unsigned long bpi;
};

extern struct loongarch_bpi_info loongarch_bpi_info;
extern void bpi_init(void);
extern void bpi_memblock_init(unsigned long *p_max_low_pfn);
extern void bpi_init_node_memblock(void (*p_add_numamem_region)(u64 start, u64 end, u32 type));
extern int loongarch_have_legacy_bpi(void);

#endif /* __LEGACY_BOOT_H_ */
