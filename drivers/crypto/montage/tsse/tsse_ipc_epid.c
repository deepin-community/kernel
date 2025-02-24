// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023-2024 Montage Technology. All rights reserved.
 */
#include <linux/errno.h>
#include <linux/string.h>
#include "tsse_ipc_epid.h"
#include "tsse_ipc_hash.h"
#include "tsse_ipc_service.h"

/* service max ids: 2^20 */
#define SERVICE_RESERVED_NUM 16
#define SERVICE_MAX_IDS (1 << 20)
#define SERVICE_BITMAP_SIZE (SERVICE_MAX_IDS / 8)

#define IS_BIT_SET(bitmap, bit) ((bitmap[(bit) / 8] & (1 << ((bit) % 8))) != 0)
#define SET_BIT(bitmap, bit) (bitmap[(bit) / 8] |= (1 << ((bit) % 8)))
#define CLEAR_BIT(bitmap, bit) (bitmap[(bit) / 8] &= ~(1 << ((bit) % 8)))

static u8 service_id_bitmap[SERVICE_BITMAP_SIZE] = {0};
static u32 current_max_service_id = SERVICE_RESERVED_NUM;

static int tsse_available_service_id(void)
{
	int i = current_max_service_id;

	if (i == SERVICE_MAX_IDS)
		i = current_max_service_id = SERVICE_RESERVED_NUM;
	for (; i < SERVICE_MAX_IDS; i++) {
		if (!IS_BIT_SET(service_id_bitmap, i)) {
			SET_BIT(service_id_bitmap, i);
			if (i > current_max_service_id)
				current_max_service_id = i;
			return i;
		}
	}
	return -1;
}

static void fill_epid(struct tsse_service_instance *service_instance, int service_id)
{
	struct tsse_epid epid_data = {0};

	epid_data.service_id = service_id;
	epid_data.pasid_en = 0;
	epid_data.vf_id = 0;
	epid_data.is_pf = 1;
	epid_data.device_id = service_instance->device_handle;
	service_instance->service_epid = EPID_TO_UINT64(&epid_data);
}

int tsse_alloc_service_epid(tsse_im_service_handle handle)
{
	int service_id;
	struct tsse_service_instance *service_instance = (struct tsse_service_instance *)handle;

	if (strcmp(service_instance->service_name, TSSE_MANAGE_SERVICE_NAME) == 0)
		service_id = EPID_MANAGE_SERVICE_ID;
	else
		service_id = tsse_available_service_id();
	if (service_id < 0)
		return -EFAULT;
	fill_epid(service_instance, service_id);
	return 0;
}

void tsse_free_service_epid(tsse_im_service_handle handle)
{
	struct tsse_service_instance *service_instance = (struct tsse_service_instance *)handle;
	u32 service_id = GET_SERVICE_ID(service_instance->service_epid);

	if (service_id < SERVICE_MAX_IDS)
		CLEAR_BIT(service_id_bitmap, service_id);
}
