/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023-2024 Montage Technology. All rights reserved.
 */
#ifndef __TSSE_IPC_EPID_H__
#define __TSSE_IPC_EPID_H__

#include <linux/types.h>
#include "tsse_ipc_msg.h"

#define EPID_MANAGE_SERVICE_ID 0
struct tsse_epid {
	uint64_t app_id : 8;
	uint64_t service_id : 20;
	uint64_t pasid : 20;
	uint64_t pasid_en : 2;
	uint64_t vf_id : 4;
	uint64_t is_pf : 2;
	uint64_t device_id : 8;
};

#define GET_DEVICE_ID(epid) (((epid) >> 56) & 0xFF)
#define GET_SERVICE_ID(epid) (((epid) >> 8) & 0xFFFFF)
#define GET_APP_SPECIFIC_ID(epid) ((epid) & 0xFF)
#define EPID_SET_PF(epid, is_pf) (((epid) & 0xFF3FFFFFFFFFFFFF) | ((uint64_t)(is_pf)) << 54)
#define SERVICE_LEVEL_EPID(epid) ((epid) & 0xFFFFFFFFFFFFFF00)

#define EPID_TO_UINT64(epid_data) \
	(((uint64_t)(epid_data)->app_id) | \
	((uint64_t)(epid_data)->service_id << 8) | \
	((uint64_t)(epid_data)->pasid << 28) | \
	((uint64_t)(epid_data)->pasid_en << 48) | \
	((uint64_t)(epid_data)->vf_id << 50) | \
	((uint64_t)(epid_data)->is_pf << 54) | \
	((uint64_t)(epid_data)->device_id << 56))

/* used to parse from response epid,
 * contains device_id, service_id and app_id
 */
#define GET_BASIC_EPID(epid) ((epid) & 0xFF0000000FFFFFFF)

#define APPEND_APP_ID_TO_EPID(epid, app_id) \
	(((epid) & 0xFFFFFFFFFFFFFF00) | ((app_id) & 0xFF))


int tsse_alloc_service_epid(tsse_im_service_handle handle);
void tsse_free_service_epid(tsse_im_service_handle handle);
#endif
