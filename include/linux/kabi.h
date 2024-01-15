/* SPDX-License-Identifier: GPL-2.0 */
/*
 * kabi.h - Deepin kABI abstraction header
 *
 *Copyright (c) 2014 Don Zickus
 *Copyright (c) 2015-2018 Jiri Benc
 *Copyright (c) 2015 Sabrina Dubroca, Hannes Frederic Sowa
 *Copyright (c) 2016-2018 Prarit Bhargava
 *Copyright (c) 2017 Paolo Abeni, Larry Woodman
 *Copyright (c) 2023 HeLugang <helugang@uniontech.com>
 *
 *
 * These macros are to be used to try to help alleviate future kernel abi
 * changes that will occur as LTS and other kernel patches are merged into the
 * tree during a period in which the kernel abi is wishing to not be disturbed.
 *
 * There are two times these macros should be used:
 *  - Before the kernel abi is "frozen"
 *    Padding can be added to various kernel structures that have in the past
 *    been known to change over time.  That will give "room" in the structure
 *    that can then be used when fields are added so that the structure size
 *    will not change.
 *
 *  - After the kernel abi is "frozen"
 *    If a structure's field is changed to a type that is identical in size to
 *    the previous type, it can be changed with a union macro
 *    If a field is added to a structure, the padding fields can be used to add
 *    the new field in a "safe" way.
 *
 * The macro helpers are derived from RHEL "include/linux/rh_kabi.h"
 * Mostly debrand from RHEL.
 */
#ifndef _LINUX_KABI_H
#define _LINUX_KABI_H

#include <linux/compiler.h>
#include <linux/stringify.h>


#ifdef __GENKSYMS__
# define _KABI_REPLACE(_orig, _new) _orig

#else
# define KABI_ALIGN_WARNING ".  Disable CONFIG_KABI_SIZE_ALIGN_CHECKS if debugging."
# define __KABI_CHECK_SIZE_ALIGN(_orig, _new)			\
	union {								\
		_Static_assert(sizeof(struct{_new;}) <= sizeof(struct{_orig;}), \
			       __FILE__ ":" __stringify(__LINE__) ": "  __stringify(_new) " is larger than " __stringify(_orig) KABI_ALIGN_WARNING); \
		_Static_assert(__alignof__(struct{_new;}) <= __alignof__(struct{_orig;}), \
			       __FILE__ ":" __stringify(__LINE__) ": "  __stringify(_orig) " is not aligned the same as " __stringify(_new) KABI_ALIGN_WARNING); \
	}
# define _KABI_REPLACE(_orig, _new)			  \
	union {						  \
		_new;					  \
		struct {				  \
			_orig;				  \
		} KABI_UNIQUE_ID;			  \
		__KABI_CHECK_SIZE_ALIGN(_orig, _new);  \
	}
#endif /* __GENKSYMS__ */

/* semicolon added wrappers for the KABI_REPLACE macros */
# define KABI_REPLACE(_orig, _new)		_KABI_REPLACE(_orig, _new);
/*
 * Macro for breaking up a random element into two smaller chunks using an
 * anonymous struct inside an anonymous union.
 */
# define KABI_REPLACE2(orig, _new1, _new2)	KABI_REPLACE(orig, struct{ _new1; _new2;})

/*
 * Macros to use _before_ the ABI is frozen
 */

/*
 *   KABI_RESERVE
 *   Reserve some "padding" in a structure for potential future use.
 *   This normally placed at the end of a structure.
 *   n: the "number" of the padding variable in the structure.  Start with
 *   1 and go up.
 */
#ifdef CONFIG_KABI_RESERVE
# define _KABI_RESERVE(n)		u64 kabi_reserved##n
#else
# define _KABI_RESERVE(n)
#endif
# define KABI_RESERVE(n)		_KABI_RESERVE(n);
/*
 * Macros to use _after_ the ABI is frozen
 */

/*
 * KABI_USE(number, _new)
 *   Use a previous padding entry that was defined with KABI_RESERVE
 *   n: the previous "number" of the padding variable
 *   _new: the variable to use now instead of the padding variable
 */
# define KABI_USE(n, _new)	KABI_REPLACE(_KABI_RESERVE(n), _new)

/*
 * KABI_USE2(number, _new1, _new2)
 *   Use a previous padding entry that was defined with KABI_RESERVE for
 *   two new variables that fit into 64 bits.  This is good for when you do not
 *   want to "burn" a 64bit padding variable for a smaller variable size if not
 *   needed.
 */
# define KABI_USE2(n, _new1, _new2)	KABI_REPLACE(_KABI_RESERVE(n), struct{ _new1; _new2; })

#endif /* _LINUX_KABI_H */
