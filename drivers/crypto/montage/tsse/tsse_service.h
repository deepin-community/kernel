/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023 Montage Technology. All rights reserved.
 */

#ifndef __TSSE_SERVICE_H__
#define __TSSE_SERVICE_H__

#include "tsse_ipc.h"
#include "tsse_fw_service.h"

int service_rout(struct tsse_ipc *tsseipc, struct ipc_msg *msg);

#endif
