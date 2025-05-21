/*
 * Copyright © 2021 Glenfly Tech Co., Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */
#ifndef _BIT_OPERATROR_H_
#define _BIT_OPERATROR_H_


#ifndef __GNUC__
#error "This is only for GCC"
#endif

#define VERIFY_BIT_OP 0

#if defined(__riscv)
static inline int ffs(int x)
{
    int r = 1;

    if (!x)
        return 0;
    if (!(x & 0xffff)) {
        x >>= 16;
        r += 16;
    }
    if (!(x & 0xff)) {
        x >>= 8;
        r += 8;
    }
    if (!(x & 0xf)) {
        x >>= 4;
        r += 4;
    }
    if (!(x & 3)) {
        x >>= 2;
        r += 2;
    }
    if (!(x & 1)) {
        x >>= 1;
        r += 1;
    }
    return r;
}
#endif

static __inline__ unsigned char _BitScanForward(volatile unsigned int *Index, unsigned int Mask)
{

#if VERIFY_BIT_OP
    unsigned int mask_range = Mask;
#endif
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
#elif defined(__riscv)
    Mask = ffs(Mask)-1;
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
#if VERIFY_BIT_OP
    unsigned int mask_range = Mask;
#endif
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
