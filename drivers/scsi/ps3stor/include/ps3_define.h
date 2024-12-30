
#ifndef __PS3_DEFINE_H__
#define __PS3_DEFINE_H__

#include "ps3_types.h"

#define PS3_ASM                     __asm__ volatile

#ifndef NULL
#define NULL 0
#endif

#ifndef INVALID_U4
#define INVALID_U4 0xF
#endif

#ifndef INVALID_U8
#define INVALID_U8 0xFF
#endif

#ifndef INVALID_U16
#define INVALID_U16 0xFFFF
#endif

#ifndef INVALID_U16_1
#define INVALID_U16_1 0xFFFE
#endif

#ifndef INVALID_U16_2
#define INVALID_U16_2 0xFFFD
#endif

#ifndef INVALID_U32
#define INVALID_U32 0xFFFFFFFF
#endif

#ifndef INVALID_U64
#define INVALID_U64 0xFFFFFFFFFFFFFFFF
#endif

#ifdef __aarch64__
#define __arm64__
#endif


#ifndef INVALID_ULONG
#if defined (__arm64__) || defined (__x86_64__)
#define INVALID_ULONG 0xFFFFFFFFFFFFFFFF
#elif defined(__arm__) || defined (__i386__)
#define INVALID_ULONG 0xFFFFFFFF
#endif
#endif

#ifndef INVALID_S8
#define INVALID_S8 0xFF
#endif

#ifndef INVALID_S16
#define INVALID_S16 0xFFFF
#endif

#ifndef INVALID_S32
#define INVALID_S32 ((S32)(-1))
#endif

#ifndef INVALID_S64
#define INVALID_S64 0xFFFFFFFFFFFFFFFF
#endif

#ifndef INVALID_LONG
#if defined (__arm64__) || defined (__x86_64__)
#define INVALID_LONG 0xFFFFFFFFFFFFFFFF
#elif defined(__arm__) || defined (__i386__)
#define INVALID_LONG 0xFFFFFFFF
#endif
#endif


#ifndef MACRO_64_BIT
#if defined (__arm64__) || defined (__x86_64__)
#define MACRO_64_BIT
#endif
#endif

#ifndef MACRO_32_BIT
#if defined (__arm__) || defined (__i386__)
#define MACRO_32_BIT
#endif
#endif

#ifndef MACRO_ARM
#if defined (__arm64__) || defined (__arm__)
#define MACRO_ARM
#endif
#endif

#ifndef MACRO_X86
#if defined (__x86_64__) || defined (__i386__)
#define MACRO_X86
#endif
#endif

#ifndef LOCK_PREFIX
#define LOCK_PREFIX "lock ; "
#endif

#ifndef BITS_PER_LONG
#ifdef MACRO_64_BIT
#define BITS_PER_LONG 64
#else
#define BITS_PER_LONG 32
#endif
#endif

#ifdef MACRO_64_BIT
#define BITS_PER_LONG_SHIFT 6
#else
#define BITS_PER_LONG_SHIFT 5
#endif

#ifndef BIT
#define BIT(nr)             (1UL << (nr))
#endif

#ifndef BIT_MASK
#define BIT_MASK(nr)        (1UL << ((nr) & ((BITS_PER_LONG) - 1)))
#endif
#ifndef BIT_WORD
#define BIT_WORD(nr)        ((nr) >> (BITS_PER_LONG_SHIFT))
#endif

#define BIT_MASK_32(nr)     (1UL << ((nr) & (32 - 1)))
#define BIT_WORD_32(nr)     ((nr) >> 5)

#ifndef BITS_PER_BYTE
#define BITS_PER_BYTE       8
#endif


#ifndef PS3_BITIDX_INVALID
#define PS3_BITIDX_INVALID          (-1)
#endif


#define SECTOR_SIZE_512                 (512)      
#define SECTOR_SIZE_4096                (4096)     
#define SECTOR_SIZE_512_BYTES_SHIFT     (9)        
#define SECTOR_SIZE_4096_BYTES_SHIFT    (12)       
#define SECTOR_512_TO_BYTES(lba)        ((lba) << SECTOR_SIZE_512_BYTES_SHIFT)
#define BYTES_TO_SECTOR_512(byte)       ((byte) >> SECTOR_SIZE_512_BYTES_SHIFT)
#define SECTOR_512_TO_4K_SHIFT          (3)
#define SECTOR4K_512COUNT               (8)

#define SECTOR4K_INDEX(offset)  ((offset) >> SECTOR_512_TO_4K_SHIFT)
#define SECTOR4K_ALIGN(offset)  ((offset) & ~(SECTOR4K_512COUNT - 1))
#define SECTOR4K_MOD(offset)    ((offset) & (SECTOR4K_512COUNT - 1))

#ifndef CACHE_ALIGNED
#define CACHE_ALIGNED __attribute__((__aligned__(64)))
#endif

#ifndef ALIGNED
#define ALIGNED(a) __attribute__((__aligned__(a)))
#endif



#ifndef RING_NAMESIZE
#define RING_NAMESIZE (32)
#endif

#ifndef RING_F_SP_ENQ
#define RING_F_SP_ENQ 0x0001
#endif
#ifndef RING_F_SC_DEQ
#define RING_F_SC_DEQ 0x0002
#endif
#ifndef RING_F_EXACT_SZ
#define RING_F_EXACT_SZ 0x0004
#endif
#ifndef RING_SZ_MASK
#define RING_SZ_MASK  (0x7fffffffU)
#endif


#ifndef RING_F_MASK
#define RING_F_MASK (RING_F_SP_ENQ | RING_F_SC_DEQ | RING_F_EXACT_SZ)
#endif

#ifndef RING_CACHE_LINE_SIZE
#define RING_CACHE_LINE_SIZE (64)
#endif
#ifndef RING_CACHE_LINE_MASK
#define RING_CACHE_LINE_MASK (RING_CACHE_LINE_SIZE - 1)
#endif

#ifndef ALIGN_FLOOR
#define ALIGN_FLOOR(val, cacheLineSize) \
    val & (~(cacheLineSize - 1))
#endif
#ifndef ALIGN
#define ALIGN(val, cacheLineSize) \
    ALIGN_FLOOR((val + cacheLineSize - 1), (cacheLineSize))
#endif

#if !defined(PCLINT)


#define PS3_SIZE_OF_TYPE_IS_ALIGN_OF(type, alignSize) \
static inline char size_of_##type##_is_align_of_##alignSize() \
{ \
    char __dummy1[0 - (sizeof(type) & (alignSize - 1))]; \
    return __dummy1[-1]; \
}


#define PS3_SIZE_OF_TYPE_NOT_LARGER_THAN(type, size) \
static inline char size_of_##type##_not_larger_than_##size() \
{ \
    char __dummy1[size - sizeof(type)]; \
    return __dummy1[-1]; \
}

#define PS3_SIZE_OF_TYPE_EQUAL_WITH(type, size) \
static inline char size_of_##type##_equal_with_##size1() \
{ \
    char __dummy1[size - sizeof(type)]; \
    return __dummy1[-1]; \
} \
static inline char size_of_##type##_equal_with_##size2() \
{ \
    char __dummy2[sizeof(type) - size]; \
    return __dummy2[-1]; \
}
#else

#define PS3_SIZE_OF_TYPE_IS_ALIGN_OF(type, alignSize) 
#define PS3_SIZE_OF_TYPE_NOT_LARGER_THAN(type, size) 
#define PS3_SIZE_OF_TYPE_EQUAL_WITH(type, size) 

#endif

#endif

