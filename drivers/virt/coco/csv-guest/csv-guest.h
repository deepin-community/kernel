/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *
 * Userspace interface for CSV guest driver
 *
 * Copyright (C) Hygon Info Technologies Ltd.
 */

#ifndef __VIRT_CSVGUEST_H__
#define __VIRT_CSVGUEST_H__

#include <linux/ioctl.h>
#include <linux/types.h>

/* Length of the user input datas used in VMMCALL */
#define CSV_REPORT_USER_DATA_LEN	64
#define CSV_REPORT_MNONCE_LEN		16
#define CSV_REPORT_HASH_LEN		32
#define CSV_REPORT_INPUT_DATA_LEN	(CSV_REPORT_USER_DATA_LEN + CSV_REPORT_MNONCE_LEN \
					+ CSV_REPORT_HASH_LEN)

/**
 * struct csv_report_req - Request struct for CSV_CMD_GET_REPORT IOCTL.
 *
 * @report_data:User buffer with REPORT_DATA to be included into CSV_REPORT, and it's also
 *		user buffer to store CSV_REPORT output from VMMCALL[KVM_HC_VM_ATTESTATION].
 * @len:	Length of the user buffer.
 */
struct csv_report_req {
	u8 *report_data;
	int len;
};

/*
 * CSV_CMD_GET_REPORT - Get CSV_REPORT using VMMCALL[KVM_HC_VM_ATTESTATION]
 *
 * Return 0 on success, -EIO on VMMCALL execution failure, and
 * standard errno on other general error cases.
 */
#define CSV_CMD_GET_REPORT	_IOWR('D', 1, struct csv_report_req)

#endif /* __VIRT_CSVGUEST_H__ */
