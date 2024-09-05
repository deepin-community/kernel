#ifndef __SXE_COMPAT_H__
#define __SXE_COMPAT_H__

#include "sxe_compat_gcc.h"

#include "sxe_compat_inc.h"

#include "sxe_compat_vercode.h"

#ifdef SPECIFIC_LINUX                  
#include "sxe_compat_spec.h"
#elif RHEL_RELEASE_CODE                
#include "sxe_compat_rhel.h"
#elif UBUNTU_VERSION_CODE              
#include "sxe_compat_ubuntu.h"
#elif OPENEULER_VERSION_CODE           
#include "sxe_compat_euler.h"
#elif KYLIN_RELEASE_CODE               
#include "sxe_compat_kylin.h"
#elif SUSE_PRODUCT_CODE                
#include "sxe_compat_suse.h"
#endif 

#ifndef SXE_KERNEL_MATCHED
#include "sxe_compat_std.h"
#endif 

#endif 
