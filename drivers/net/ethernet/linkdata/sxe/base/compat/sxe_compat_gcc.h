#ifndef __SXE_COMPAT_GCC_H__
#define __SXE_COMPAT_GCC_H__

#ifdef __has_attribute
#if __has_attribute(__fallthrough__)
# define fallthrough __attribute__((__fallthrough__))
#else
# define fallthrough do {} while (0)  
#endif 
#else
# define fallthrough do {} while (0)  
#endif 

#endif 