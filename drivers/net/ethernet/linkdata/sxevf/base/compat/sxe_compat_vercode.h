#ifndef __SXE_COMPAT_VERCODE_H__
#define __SXE_COMPAT_VERCODE_H__


#ifndef LINUX_VERSION_CODE
#include <linux/version.h>
#else
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#endif

#ifndef UTS_RELEASE
#include <generated/utsrelease.h>
#endif

#ifndef RHEL_RELEASE_CODE
#define RHEL_RELEASE_CODE 0
#endif
#ifndef RHEL_RELEASE_VERSION
#define RHEL_RELEASE_VERSION(a,b) (((a) << 8) + (b))
#endif

#define UBUNTU_VERSION(a,b,c,d) (((a) << 24) + ((b) << 16) + (d))

#ifndef UTS_UBUNTU_RELEASE_ABI
#define UTS_UBUNTU_RELEASE_ABI 0
#define UBUNTU_VERSION_CODE 0
#else
#define UBUNTU_VERSION_CODE (((~0xFF & LINUX_VERSION_CODE) << 8) + \
			     UTS_UBUNTU_RELEASE_ABI)
#if UTS_UBUNTU_RELEASE_ABI > 65535
#error UTS_UBUNTU_RELEASE_ABI is larger than 65535...
#endif 
#endif 

#ifndef OPENEULER_VERSION_CODE
#define OPENEULER_VERSION_CODE 0
#endif
#ifndef OPENEULER_VERSION
#define OPENEULER_VERSION(a,b) (((a) << 8) + (b))
#endif

#ifndef KYLIN_RELEASE_CODE 
#define KYLIN_RELEASE_CODE  0
#endif
#ifndef KYLIN_RELEASE_VERSION
#define KYLIN_RELEASE_VERSION(a,b) ((a << 8) + b)
#endif

#ifdef CONFIG_SUSE_KERNEL
#include <generated/uapi/linux/suse_version.h>
#endif
#ifndef SUSE_PRODUCT_CODE 
#define SUSE_PRODUCT_CODE  0
#endif
#ifndef SUSE_PRODUCT
#define SUSE_PRODUCT(product, version, patchlevel, auxrelease)		\
	(((product) << 24) + ((version) << 16) +			\
	 ((patchlevel) << 8) + (auxrelease))
#endif

#endif 