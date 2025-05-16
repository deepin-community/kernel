/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2024 Huawei Technologies Co., Ltd */

#ifndef _OSSL_TYPES_H
#define _OSSL_TYPES_H

#undef NULL
#if defined(__cplusplus)
#define NULL 0
#else
#define NULL ((void *)0)
#endif

#if defined(__LINUX__)
#ifdef __USER__ /* linux user */
#if defined(__ia64__) || defined(__x86_64__) || defined(__aarch64__)
#define s64 long
#define u64 unsigned long
#else
#define s64 long long
#define u64 unsigned long long
#endif
#define s32 int
#define u32 unsigned int
#define s16 short
#define u16 unsigned short

#ifdef __hinic_arm__
#define s8 signed char
#else
#define s8 char
#endif

#ifndef dma_addr_t
typedef u64 dma_addr_t;
#endif

#define u8 unsigned char
#define ulong unsigned long
#define uint unsigned int

#define ushort unsigned short

#endif
#endif

#define uda_handle void *

#define UDA_TRUE 1
#define UDA_FALSE 0

#if defined(__USER__) || defined(USER)
#ifndef F_OK
#define F_OK 0
#endif
#ifndef F_FAILED
#define F_FAILED (-1)
#endif

#define uda_status int
#define TOOL_REAL_PATH_MAX_LEN 512
#define SAFE_FUNCTION_ERR (-1)

enum {
	UDA_SUCCESS = 0x0,	// run success
	UDA_FAIL,		// run failed
	UDA_ENXIO,		// no device
	UDA_ENONMEM,		// alloc memory failed
	UDA_EBUSY,		// card busy or restart
	UDA_ECRC,		// CRC check error
	UDA_EINVAL,		// invalid parameter
	UDA_EFAULT,		// invalid address
	UDA_ELEN,		// invalid length
	UDA_ECMD,		// error occurs when execute the cmd
	UDA_ENODRIVER,		// driver is not installed
	UDA_EXIST,		// has existed
	UDA_EOVERSTEP,		// over step
	UDA_ENOOBJ,		// have no object
	UDA_EOBJ,		// error object
	UDA_ENOMATCH,		// driver does not match to firmware
	UDA_ETIMEOUT,		// timeout

	UDA_CONTOP,

	UDA_REBOOT = 0xFD,
	UDA_CANCEL = 0xFE,
	UDA_KILLED = 0xFF,
};

enum {
	UDA_FLOCK_NOBLOCK = 0,
	UDA_FLOCK_BLOCK = 1,
};

/* array index */
#define ARRAY_INDEX_0 0
#define ARRAY_INDEX_1 1
#define ARRAY_INDEX_2 2
#define ARRAY_INDEX_3 3
#define ARRAY_INDEX_4 4
#define ARRAY_INDEX_5 5
#define ARRAY_INDEX_6 6
#define ARRAY_INDEX_7 7
#define ARRAY_INDEX_8 8
#define ARRAY_INDEX_12 12
#define ARRAY_INDEX_13 13

/* define shift bits */
#define SHIFT_BIT_1 1
#define SHIFT_BIT_2 2
#define SHIFT_BIT_3 3
#define SHIFT_BIT_4 4
#define SHIFT_BIT_6 6
#define SHIFT_BIT_7 7
#define SHIFT_BIT_8 8
#define SHIFT_BIT_11 11
#define SHIFT_BIT_12 12
#define SHIFT_BIT_15 15
#define SHIFT_BIT_16 16
#define SHIFT_BIT_17 17
#define SHIFT_BIT_19 19
#define SHIFT_BIT_20 20
#define SHIFT_BIT_23 23
#define SHIFT_BIT_24 24
#define SHIFT_BIT_25 25
#define SHIFT_BIT_26 26
#define SHIFT_BIT_28 28
#define SHIFT_BIT_29 29
#define SHIFT_BIT_32 32
#define SHIFT_BIT_35 35
#define SHIFT_BIT_37 37
#define SHIFT_BIT_39 39
#define SHIFT_BIT_40 40
#define SHIFT_BIT_43 43
#define SHIFT_BIT_48 48
#define SHIFT_BIT_51 51
#define SHIFT_BIT_56 56
#define SHIFT_BIT_57 57
#define SHIFT_BIT_59 59
#define SHIFT_BIT_60 60
#define SHIFT_BIT_61 61

#endif
#endif /* OSSL_TYPES_H */
