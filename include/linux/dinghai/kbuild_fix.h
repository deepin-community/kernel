#ifndef _ZXDH_KBUILD_FIX_H_
#define _ZXDH_KBUILD_FIX_H_

/* CGS_V5_693: some external builds may miss KBUILD_MODNAME when
 * including kernel headers that rely on it (dynamic_debug, pr_debug).
 * If the kernel build system already defines KBUILD_MODNAME via
 * compiler flags, this header will not change it.
 */
#ifdef CGS_V5_693
#ifndef KBUILD_MODNAME
#define KBUILD_MODNAME "zxdh_dinghai"
#endif
#endif

#endif /* _ZXDH_KBUILD_FIX_H_ */

