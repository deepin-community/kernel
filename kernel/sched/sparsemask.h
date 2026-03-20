/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sparsemask.h - sparse bitmap operations
 *
 * Copyright (c) 2018 Oracle Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __LINUX_SPARSEMASK_H
#define __LINUX_SPARSEMASK_H

#include <linux/kernel.h>
#include <linux/bitmap.h>
#include <linux/bug.h>

/*
 * A sparsemask is a sparse bitmap.  It reduces cache contention vs the usual
 * bitmap when many threads concurrently set, clear, and visit elements.  For
 * each cacheline chunk of the mask, only the first K bits of the first word are
 * used, and the remaining bits are ignored, where K is a creation time
 * parameter.  Thus a sparsemask that can represent a set of N elements is
 * approximately (N/K * CACHELINE) bytes in size.
 *
 * Clients pass and receive element numbers in the public API, and the
 * implementation translates them to bit numbers to perform the bitmap
 * operations.
 */

struct sparsemask_chunk {
	unsigned long word;	/* the significant bits */
} ____cacheline_aligned_in_smp;

struct sparsemask {
	short nelems;		/* current number of elements */
	short density;		/* store 2^density elements per chunk */
	struct sparsemask_chunk chunks[0];  /* embedded array of chunks */
};

#define _SMASK_INDEX(density, elem)	((elem) >> (density))
#define _SMASK_BIT(density, elem)	((elem) & ((1U << (density)) - 1U))
#define SMASK_INDEX(mask, elem)		_SMASK_INDEX((mask)->density, elem)
#define SMASK_BIT(mask, elem)		_SMASK_BIT((mask)->density, elem)
#define SMASK_WORD(mask, elem)		\
	(&(mask)->chunks[SMASK_INDEX((mask), (elem))].word)

/*
 * sparsemask_next() - Return the next one bit in a bitmap, starting at a
 * specified position and wrapping from the last bit to the first, up to but
 * not including a specified origin.  This is a helper, so do not call it
 * directly.
 *
 * @mask: Bitmap to search.
 * @origin: Origin.
 * @prev: Previous bit. Start search after this bit number.
 *	  If -1, start search at @origin.
 *
 * Return: the bit number, else mask->nelems if no bits are set in the range.
 */
static inline int
sparsemask_next(const struct sparsemask *mask, int origin, int prev)
{
	int density = mask->density;
	int bits_per_word = 1U << density;
	const struct sparsemask_chunk *chunk;
	int nelems = mask->nelems;
	int next, bit, nbits;
	unsigned long word;

	/* Calculate number of bits to be searched. */
	if (prev == -1) {
		nbits = nelems;
		next = origin;
	} else if (prev < origin) {
		nbits = origin - prev;
		next = prev + 1;
	} else {
		nbits = nelems - prev + origin - 1;
		next = prev + 1;
	}

	if (unlikely(next >= nelems))
		return nelems;

	/*
	 * Fetch and adjust first word.  Clear word bits below @next, and round
	 * @next down to @bits_per_word boundary because later ffs will add
	 * those bits back.
	 */
	chunk = &mask->chunks[_SMASK_INDEX(density, next)];
	bit = _SMASK_BIT(density, next);
	word = chunk->word & (~0UL << bit);
	next -= bit;
	nbits += bit;

	while (!word) {
		next += bits_per_word;
		nbits -= bits_per_word;
		if (nbits <= 0)
			return nelems;

		if (next >= nelems) {
			chunk = mask->chunks;
			nbits -= (next - nelems);
			next = 0;
		} else {
			chunk++;
		}
		word = chunk->word;
	}

	next += __ffs(word);
	if (next >= origin && prev != -1)
		return nelems;
	return next;
}

/****************** The public API ********************/

/*
 * Max value for the density parameter, limited by 64 bits in the chunk word.
 */
#define SMASK_DENSITY_MAX		6

/*
 * Return bytes to allocate for a sparsemask, for custom allocators.
 */
static inline size_t sparsemask_size(int nelems, int density)
{
	int index = _SMASK_INDEX(density, nelems) + 1;

	return offsetof(struct sparsemask, chunks[index]);
}

/*
 * Initialize an allocated sparsemask, for custom allocators.
 */
static inline void
sparsemask_init(struct sparsemask *mask, int nelems, int density)
{
	WARN_ON(density < 0 || density > SMASK_DENSITY_MAX || nelems < 0);
	mask->nelems = nelems;
	mask->density = density;
}

/*
 * sparsemask_alloc_node() - Allocate, initialize, and return a sparsemask.
 *
 * @nelems - maximum number of elements.
 * @density - store 2^density elements per cacheline chunk.
 *	      values from 0 to SMASK_DENSITY_MAX inclusive.
 * @flags - kmalloc allocation flags
 * @node - numa node
 */
static inline struct sparsemask *
sparsemask_alloc_node(int nelems, int density, gfp_t flags, int node)
{
	int nbytes = sparsemask_size(nelems, density);
	struct sparsemask *mask = kmalloc_node(nbytes, flags, node);

	if (mask)
		sparsemask_init(mask, nelems, density);
	return mask;
}

static inline void sparsemask_free(struct sparsemask *mask)
{
	kfree(mask);
}

static inline void sparsemask_set_elem(struct sparsemask *dst, int elem)
{
	set_bit(SMASK_BIT(dst, elem), SMASK_WORD(dst, elem));
}

static inline void sparsemask_clear_elem(struct sparsemask *dst, int elem)
{
	clear_bit(SMASK_BIT(dst, elem), SMASK_WORD(dst, elem));
}

static inline int sparsemask_test_elem(const struct sparsemask *mask, int elem)
{
	return test_bit(SMASK_BIT(mask, elem), SMASK_WORD(mask, elem));
}

/*
 * sparsemask_for_each() - iterate over each set bit in a bitmap, starting at a
 *   specified position, and wrapping from the last bit to the first.
 *
 * @mask: Bitmap to iterate over.
 * @origin: Bit number at which to start searching.
 * @elem: Iterator.  Can be signed or unsigned integer.
 *
 * The implementation does not assume any bit in @mask is set, including
 * @origin.  After the loop, @elem = @mask->nelems.
 */
#define sparsemask_for_each(mask, origin, elem)				\
	for ((elem) = -1;						\
	     (elem) = sparsemask_next((mask), (origin), (elem)),	\
		(elem) < (mask)->nelems;)

#endif /* __LINUX_SPARSEMASK_H */
