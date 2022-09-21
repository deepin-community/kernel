//*****************************************************************************
//  Copyright (c) 2021 Glenfly Tech Co., Ltd.. 
//  All Rights Reserved.
//
//  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Glenfly Tech Co., Ltd..;
//  the contents of this file may not be disclosed to third parties, copied or
//  duplicated in any form, in whole or in part, without the prior written
//  permission of Glenfly Tech Co., Ltd..
//
//  The copyright of the source code is protected by the copyright laws of the People's
//  Republic of China and the related laws promulgated by the People's Republic of China
//  and the international covenant(s) ratified by the People's Republic of China. 
//*****************************************************************************


#ifndef _BIT_OPERATROR_H_
#define _BIT_OPERATROR_H_


#ifndef __GNUC__
#error "This is only for GCC"
#endif

#define VERIFY_BIT_OP 0


static __inline__ unsigned char _BitScanForward(volatile unsigned int *Index, unsigned int Mask)
{
    unsigned int mask_range = Mask;

    if(Mask==0)
    {
        return 0;
    }
#if defined(__i386__) || defined(__x86_64__)
    __asm__ __volatile__( 
      "bsfl %1, %0"
       :"=r"(Mask)     
       :"r" (Mask)  
       );
#else
    Mask = __builtin_ffs(Mask)-1;
#endif  
    *Index = Mask;


    
#if VERIFY_BIT_OP
        {
            unsigned int i = 32;    
        //find the set bit from lsb
#if __BIG_ENDIAN__
            for(i=31; i>=0; i--)
            {
                if(mask_range & (1<<i)) break;
                 
            }
#else
            for(i=0; i<32; i++)
            {
                if(mask_range & (1<<i)) break;
                 
            }
#endif
    
            if(i != Mask)
            {
                gf_info("bitscanReverse range is %d while should be %d.\n", Mask, i);
            }
    
        }
#endif //VERIFY_BIT_OP


    return 1;
}

static __inline__ unsigned char _BitScanReverse(volatile unsigned int *Index, unsigned int Mask)
{
    unsigned int mask_range = Mask;

    if(Mask==0)
    {
        return 0;
    }
#if defined(__i386__) || defined(__x86_64__)
    __asm__ __volatile__( 
       "bsrl %1, %0"
       :"=r"(Mask)     
       :"r" (Mask)  
       );
#else
    Mask = 31 - __builtin_clz(Mask);
#endif
    *Index = Mask;


#if VERIFY_BIT_OP
    {
        unsigned int i = 32;    
    //find the set bit from msb
#if __BIG_ENDIAN__
        for(i=0; i<32; i++)
        {
            if(mask_range & (1<<i)) break;
             
        }
#else
        for(i=31; i>=0; i--)
        {
            if(mask_range & (1<<i)) break;
             
        }
#endif

        if(i != Mask)
        {
            gf_info("bitscanForward range is %d while should be %d.\n", Mask, i);
        }

    }
#endif //VERIFY_BIT_OP



    return 1;
}

static __inline__ unsigned char _bittestandreset(volatile int *a, int b)
{
    register unsigned char ret;
#if defined(__i386__) || defined(__x86_64__)
    ret = (((*a) >> b) & 1);

    __asm__ __volatile__(
      "btrl %1,%0"
      :"=m"(*(a))
      :"r"(b)
      );

    return ret;
#else
    int c = *a;
    (*a) &= ~(1<<b);
    return (c!=*a);
#endif
}


typedef union Mask64 {
    struct {
#ifdef  __BIG_ENDIAN__
    int mask_1;
    int mask_0;
#else
    int mask_0;
    int mask_1;
#endif
    };
    unsigned long long value;
} Mask64;


static __inline__ unsigned char _BitScanForward_64(unsigned int * Index, unsigned long long Mask)
{
    Mask64 mask64;

    mask64.value = Mask;

    if(mask64.mask_0)
    {
        if(_BitScanForward((unsigned int*)Index, mask64.mask_0))
        {
            return 1;
        }
    }
    else
    {
        if(_BitScanForward((unsigned int*)Index, mask64.mask_1))
        {
            *Index += sizeof(int) * 8;
            return 1;
        }        
    }

    return 0;
}

static __inline__ unsigned char _BitScanReverse_64(unsigned int * Index, unsigned long long Mask)
{
    Mask64 mask64;

    mask64.value = Mask;
    if(mask64.mask_1)
    {
        if(_BitScanReverse((unsigned int*)Index, mask64.mask_1))
        {
            *Index += sizeof(int)*8;
            return 1;
        }
    }
    else
    {
        if(_BitScanReverse((unsigned int*)Index, mask64.mask_0))
        {
            return 1;
        }
    }

    return 0;
}

static __inline__ unsigned char _bittestandreset_64(unsigned long long *a, int b)
{
    unsigned char c;
    Mask64 mask64;

    mask64.value = *a;    
    if (b < (int)(sizeof(int) * 8))
        c = _bittestandreset((int*)&(mask64.mask_0), b);
    else
        c = _bittestandreset((int*)&(mask64.mask_1), b - sizeof(int) * 8);
    *a = mask64.value;

    return c;
}



#ifndef KERNEL_BUILD

//here code are copied from the kernel source code <asm-i386/bitops.h>
static __inline__ int constant_test_bit(int nr, const volatile unsigned int *addr)
{
    return ((1UL << (nr & 31)) & (addr[nr >> 5])) != 0;
}

static __inline__ int variable_test_bit(int nr, const volatile unsigned int * addr)
{
    int oldbit;
#if defined(__i386__) || defined(__x86_64__)
    __asm__ __volatile__(
        "btl %2,%1\n\tsbbl %0,%0"
        :"=r" (oldbit)
        :"m" (*(addr)),"Ir" (nr));
#else
    if((*addr) & ( 1 << (nr&31)))
    {
        oldbit = 1;
    }
    else
    {
        oldbit = 0;
    }

#endif
    return oldbit;
}

#define test_bit(nr,addr) \
(__builtin_constant_p(nr) ? \
 constant_test_bit((nr),(addr)) : \
 variable_test_bit((nr),(addr)))


static __inline__ int __test_and_set_bit(int nr, volatile unsigned int * addr)
{
    int oldbit;
#if defined(__i386__) || defined(__x86_64__)
    __asm__ __volatile__(
        "btsl %2,%1\n\tsbbl %0,%0"
        :"=r" (oldbit),"+m" (*(addr))
        :"Ir" (nr));
#else
    if( (*addr) & (1 <<(nr&31)))
    {
        oldbit = 1;
    }
    else
    {
        (*addr) |= (1 <<(nr&31));
        oldbit = 0;
    }

#endif
    return oldbit;
}

static __inline__ unsigned char _bittest(int *a, int b)
{

    return (unsigned char)test_bit(b, (volatile unsigned int *)a);
}

static __inline__ unsigned char _bittestandset(int *a, int b)
{
 
    return (unsigned char)__test_and_set_bit(b, (volatile unsigned int *)a);
}

#endif

#endif
