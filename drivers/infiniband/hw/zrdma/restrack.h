/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef ZXDH_ZRDMA_H
#define ZXDH_ZRDMA_H

#ifndef ZXDH_UAPI_DEF
#include <rdma/ib_verbs.h>
#endif

int zxdh_set_restrack_ops(struct ib_device *ibdev);
#endif
