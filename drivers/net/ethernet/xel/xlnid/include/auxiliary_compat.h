/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 1999 - 2024 Xel Technology. */

#ifndef _AUXILIARY_COMPAT_H_
#define _AUXILIARY_COMPAT_H_

/*  This file contains only the minimal set of kernel compatibility backports
		required by auxiliary.c to build. It is similar to the kcompat.h file, but
		reduced to an absolute minimum in order to reduce the risk of generating
		different kernel symbol CRC values at build time.

		For a detailed discussion of kernel symbol CRCs, please read:

			Documentation/kernel-symbol-crc.rst

		Include only the minimum required kernel compatibility implementations from
		kcompat_generated_defs.h and kcompat_impl.h. If a new fix is required,
		please first implement it as part of the kcompat project before porting it
		to this file.

		The current list of required implementations is:

		NEED_BUS_FIND_DEVICE_CONST_DATA
		NEED_DEV_PM_DOMAIN_ATTACH
		NEED_DEV_PM_DOMAIN_DETACH

		Note that kernels since v5.11 support auxiliary as a built-in config
		option. Using this is always preferred to using an out-of-tree module when
		available.
*/

#include "kcompat_generated_defs.h"

/****************************
		Backport implementations
	****************************/

#ifdef NEED_BUS_FIND_DEVICE_CONST_DATA
/*  NEED_BUS_FIND_DEVICE_CONST_DATA

		bus_find_device() was updated in upstream commit 418e3ea157ef
		("bus_find_device: Unify the match callback with class_find_device")
		to take a const void *data parameter and also have the match() function
		passed in take a const void *data parameter.

		all of the kcompat below makes it so the caller can always just call
		bus_find_device() according to the upstream kernel without having to worry
		about const vs. non-const arguments.
*/
struct _kc_bus_find_device_custom_data {
	const void *real_data;
	int (*real_match)(struct device *dev, const void *data);
};

static inline int _kc_bus_find_device_wrapped_match(struct device *dev,
		void *data)
{
	struct _kc_bus_find_device_custom_data *custom_data = data;

	return custom_data->real_match(dev, custom_data->real_data);
}

static inline struct device *
_kc_bus_find_device(struct bus_type *type, struct device *start,
					const void *data,
					int (*match)(struct device *dev, const void *data))
{
	struct _kc_bus_find_device_custom_data custom_data = {};

	custom_data.real_data = data;
	custom_data.real_match = match;

	return bus_find_device(type, start, &custom_data,
							_kc_bus_find_device_wrapped_match);
}

/*  force callers of bus_find_device() to call _kc_bus_find_device() on kernels
		where NEED_BUS_FIND_DEVICE_CONST_DATA is defined
*/
#define bus_find_device(type, start, data, match) \
	_kc_bus_find_device(type, start, data, match)
#endif /* NEED_BUS_FIND_DEVICE_CONST_DATA */

#endif /* _AUXILIARY_COMPAT_H_ */
