
#ifndef __SXE_LOG_TYPES_H__
#define __SXE_LOG_TYPES_H__

#ifdef __cplusplus
extern "C"{
#endif

#include <linux/stddef.h>

typedef unsigned char       U8;
typedef unsigned short      U16;
typedef unsigned int        U32;
typedef unsigned long       ULong;
typedef unsigned long long  U64;

typedef char                S8;
typedef short               S16;
typedef int                 S32;
typedef long                Long;
typedef long long           S64;

#define SXE_FALSE 0
#define SXE_TRUE  1

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#ifndef likely
#define likely(x)    __builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
#define unlikely(x)  __builtin_expect(!!(x), 0)
#endif

#ifndef offsetof
#define offsetof(TYPE, MEMBER)    ((size_t)(&((TYPE *)0)->MEMBER))
#endif


#ifndef SXE_MIN
#define SXE_MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif


#ifndef SXE_MAX
#define SXE_MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif


#ifndef SXE_MIN_NON_ZERO
#define SXE_MIN_NON_ZERO(a, b) ((a) == 0 ? (b) : \
                                    ((b) == 0 ? (a) : (SXE_MIN(a, b))))
#endif

#ifndef TYPEOF
#ifdef __cplusplus
#define TYPEOF decltype
#else
#define TYPEOF typeof
#endif
#endif

#ifndef container_of
#ifndef PCLINT
#define container_of(ptr, type, member) ({ \
    const TYPEOF( ((type *)0)->member ) *__mptr = (ptr); \
    (type *)( (char *)__mptr - offsetof(type,member) );})
#else
#define container_of(ptr, type, member) \
    ((type *)(void *)(char *)ptr)
#endif
#endif

#ifndef SXE_DESC
#define SXE_DESC(a) 1
#endif


#ifndef SXE_IN
#define SXE_IN
#endif

#ifndef SXE_OUT
#define SXE_OUT
#endif

#ifdef __cplusplus
}
#endif

#endif
