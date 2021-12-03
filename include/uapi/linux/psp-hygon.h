/* SPDX-License-Identifier: GPL-2.0-only WITH Linux-syscall-note */
/*
 * Userspace interface for HYGON Platform Security Processor (PSP)
 * commands.
 *
 * Copyright (C) 2024 Hygon Info Technologies Ltd.
 *
 * Author: Liyang Han <hanliyang@hygon.cn>
 */

#ifndef __PSP_HYGON_USER_H__
#define __PSP_HYGON_USER_H__

#include <linux/types.h>

/*****************************************************************************/
/***************************** CSV interface *********************************/
/*****************************************************************************/

/**
 * CSV guest/platform commands
 */
enum {
	CSV_PLATFORM_INIT = 101,
	CSV_HGSC_CERT_IMPORT = 201,

	CSV_MAX,
};

/**
 * struct csv_user_data_hgsc_cert_import - HGSC_CERT_IMPORT command parameters
 *
 * @hgscsk_cert_address: HGSCSK certificate chain
 * @hgscsk_cert_len: length of HGSCSK certificate
 * @hgsc_cert_address: HGSC certificate chain
 * @hgsc_cert_len: length of HGSC certificate
 */
struct csv_user_data_hgsc_cert_import {
	__u64 hgscsk_cert_address;              /* In */
	__u32 hgscsk_cert_len;                  /* In */
	__u64 hgsc_cert_address;                /* In */
	__u32 hgsc_cert_len;                    /* In */
} __packed;

#endif	/* __PSP_HYGON_USER_H__ */
