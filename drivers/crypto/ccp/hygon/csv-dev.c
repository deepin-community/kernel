// SPDX-License-Identifier: GPL-2.0-only
/*
 * HYGON CSV interface
 *
 * Copyright (C) 2024 Hygon Info Technologies Ltd.
 *
 * Author: Liyang Han <hanliyang@hygon.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/psp.h>
#include <linux/psp-hygon.h>
#include <uapi/linux/psp-hygon.h>

#include "csv-dev.h"
#include "psp-dev.h"

/*
 * Hygon CSV build info:
 *    Hygon CSV build info is 32-bit in length other than 8-bit as that
 *    in AMD SEV.
 */
u32 hygon_csv_build;

/*
 * csv_update_api_version used to update the api version of HYGON CSV
 * firmwareat driver side.
 * Currently, we only need to update @hygon_csv_build.
 */
void csv_update_api_version(struct sev_user_data_status *status)
{
	if (status) {
		hygon_csv_build = (status->flags >> 9) |
				   ((u32)status->build << 23);
	}
}

int csv_cmd_buffer_len(int cmd)
{
	switch (cmd) {
	case CSV_CMD_HGSC_CERT_IMPORT:		return sizeof(struct csv_data_hgsc_cert_import);
	default:				return 0;
	}
}

static int csv_ioctl_do_hgsc_import(struct sev_issue_cmd *argp)
{
	struct csv_user_data_hgsc_cert_import input;
	struct csv_data_hgsc_cert_import *data;
	void *hgscsk_blob, *hgsc_blob;
	int ret;

	if (copy_from_user(&input, (void __user *)argp->data, sizeof(input)))
		return -EFAULT;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	/* copy HGSCSK certificate blobs from userspace */
	hgscsk_blob = psp_copy_user_blob(input.hgscsk_cert_address, input.hgscsk_cert_len);
	if (IS_ERR(hgscsk_blob)) {
		ret = PTR_ERR(hgscsk_blob);
		goto e_free;
	}

	data->hgscsk_cert_address = __psp_pa(hgscsk_blob);
	data->hgscsk_cert_len = input.hgscsk_cert_len;

	/* copy HGSC certificate blobs from userspace */
	hgsc_blob = psp_copy_user_blob(input.hgsc_cert_address, input.hgsc_cert_len);
	if (IS_ERR(hgsc_blob)) {
		ret = PTR_ERR(hgsc_blob);
		goto e_free_hgscsk;
	}

	data->hgsc_cert_address = __psp_pa(hgsc_blob);
	data->hgsc_cert_len = input.hgsc_cert_len;

	ret = hygon_psp_hooks.__sev_do_cmd_locked(CSV_CMD_HGSC_CERT_IMPORT,
						  data, &argp->error);

	kfree(hgsc_blob);
e_free_hgscsk:
	kfree(hgscsk_blob);
e_free:
	kfree(data);
	return ret;
}

static int csv_ioctl_do_download_firmware(struct sev_issue_cmd *argp)
{
	struct sev_data_download_firmware *data = NULL;
	struct csv_user_data_download_firmware input;
	int ret, order;
	struct page *p;
	u64 data_size;

	/* Only support DOWNLOAD_FIRMWARE if build greater or equal 1667 */
	if (!csv_version_greater_or_equal(1667)) {
		pr_err("DOWNLOAD_FIRMWARE not supported\n");
		return -EIO;
	}

	if (copy_from_user(&input, (void __user *)argp->data, sizeof(input)))
		return -EFAULT;

	if (!input.address) {
		argp->error = SEV_RET_INVALID_ADDRESS;
		return -EINVAL;
	}

	if (!input.length || input.length > CSV_FW_MAX_SIZE) {
		argp->error = SEV_RET_INVALID_LEN;
		return -EINVAL;
	}

	/*
	 * CSV FW expects the physical address given to it to be 32
	 * byte aligned. Memory allocated has structure placed at the
	 * beginning followed by the firmware being passed to the CSV
	 * FW. Allocate enough memory for data structure + alignment
	 * padding + CSV FW.
	 */
	data_size = ALIGN(sizeof(struct sev_data_download_firmware), 32);

	order = get_order(input.length + data_size);
	p = alloc_pages(GFP_KERNEL, order);
	if (!p)
		return -ENOMEM;

	/*
	 * Copy firmware data to a kernel allocated contiguous
	 * memory region.
	 */
	data = page_address(p);
	if (copy_from_user((void *)(page_address(p) + data_size),
			   (void *)input.address, input.length)) {
		ret = -EFAULT;
		goto err_free_page;
	}

	data->address = __psp_pa(page_address(p) + data_size);
	data->len = input.length;

	ret = hygon_psp_hooks.__sev_do_cmd_locked(SEV_CMD_DOWNLOAD_FIRMWARE,
						  data, &argp->error);
	if (ret)
		pr_err("Failed to update CSV firmware: %#x\n", argp->error);
	else
		pr_info("CSV firmware update successful\n");

err_free_page:
	__free_pages(p, order);

	return ret;
}

static long csv_ioctl(struct file *file, unsigned int ioctl, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct sev_issue_cmd input;
	int ret = -EFAULT;

	if (!hygon_psp_hooks.sev_dev_hooks_installed)
		return -ENODEV;

	if (!psp_master || !psp_master->sev_data)
		return -ENODEV;

	if (ioctl != SEV_ISSUE_CMD)
		return -EINVAL;

	if (copy_from_user(&input, argp, sizeof(struct sev_issue_cmd)))
		return -EFAULT;

	if (input.cmd > CSV_MAX)
		return -EINVAL;

	mutex_lock(hygon_psp_hooks.sev_cmd_mutex);

	switch (input.cmd) {
	case CSV_HGSC_CERT_IMPORT:
		ret = csv_ioctl_do_hgsc_import(&input);
		break;
	case CSV_PLATFORM_INIT:
		ret = hygon_psp_hooks.__sev_platform_init_locked(&input.error);
		break;
	case CSV_PLATFORM_SHUTDOWN:
		ret = hygon_psp_hooks.__sev_platform_shutdown_locked(&input.error);
		break;
	case CSV_DOWNLOAD_FIRMWARE:
		ret = csv_ioctl_do_download_firmware(&input);
		break;
	default:
		/*
		 * If the command is compatible between CSV and SEV, the
		 * native implementation of the driver is invoked.
		 * Release the mutex before calling the native ioctl function
		 * because it will acquires the mutex.
		 */
		mutex_unlock(hygon_psp_hooks.sev_cmd_mutex);
		return hygon_psp_hooks.sev_ioctl(file, ioctl, arg);
	}

	if (copy_to_user(argp, &input, sizeof(struct sev_issue_cmd)))
		ret = -EFAULT;

	mutex_unlock(hygon_psp_hooks.sev_cmd_mutex);

	return ret;
}

const struct file_operations csv_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = csv_ioctl,
};
