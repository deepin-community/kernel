#ifndef __ASM_HWCAP_H
#define __ASM_HWCAP_H

#include <uapi/asm/hwcap.h>
#include <asm/cpufeature.h>

#ifndef __ASSEMBLY__
#include <linux/log2.h>
#define __khwcap_feature(x)				const_ilog2(HWCAP_SW64_ ## x)

#define KERNEL_HWCAP_SW64_HWUNA			__khwcap_feature(HWUNA)

#endif
#endif
