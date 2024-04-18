/*
* SPDX-License-Identifier: GPL
*
* Copyright (c) 2020 ChangSha JingJiaMicro Electronics Co., Ltd.
* All rights reserved.
*
* Author:
*      shanjinkui <shanjinkui@jingjiamicro.com>
*
* The software and information contained herein is proprietary and
* confidential to JingJiaMicro Electronics. This software can only be
* used by JingJiaMicro Electronics Corporation. Any use, reproduction,
* or disclosure without the written permission of JingJiaMicro
* Electronics Corporation is strictly prohibited.
*/
#include <linux/sort.h>
#include <linux/bsearch.h>
#include <linux/uaccess.h>
#include "mwv207_db.h"
#include "mwv207.h"
#include "mwv207_drm.h"

struct mwv207_db {
	u32 nr;
	u32 max_nr;
	u32 sorted;
	u32 pad;
	struct drm_mwv207_info_rec *db;
};

int mwv207_db_init(struct mwv207_device *jdev)
{
	struct mwv207_db *db;
	u32 size;

	size = sizeof(struct mwv207_db);
	size += 2048 * sizeof(struct drm_mwv207_info_rec);
	db = devm_kmalloc(jdev->dev, size, GFP_KERNEL);
	if (!db)
		return -ENOMEM;

	db->db = (struct drm_mwv207_info_rec *)(db + 1);
	db->max_nr = 2048;
	db->nr = 0;
	db->sorted = 0;
	jdev->db = db;
	return 0;
}

void mwv207_db_fini(struct mwv207_device *jdev)
{
	devm_kfree(jdev->dev, jdev->db);
}

int mwv207_db_add(struct mwv207_device *jdev, u32 key, u32 val)
{
	struct mwv207_db *db = jdev->db;

	if (db->nr >= db->max_nr) {
		WARN_ONCE(1, "mwv207: maximum key count reached");
		return -ENOMEM;
	}

	db->db[db->nr].key = key;
	db->db[db->nr].val = val;
	db->nr++;

	if (db->sorted)
		mwv207_db_sort(jdev);

	return 0;
}

static int cmp_key(const void *a, const void *b)
{
	struct drm_mwv207_info_rec const *rec_a = a, *rec_b = b;

	if (rec_a->key < rec_b->key)
		return -1;
	else if (rec_a->key > rec_b->key)
		return 1;
	else
		return 0;
}

void mwv207_db_sort(struct mwv207_device *jdev)
{
	struct mwv207_db *db = jdev->db;
	int i;

	db->sorted = 1;
	if (db->nr < 2)
		return;

	sort(db->db, db->nr, 8, cmp_key, NULL);

	for (i = 0; i < db->nr - 1; ++i) {
		if (db->db[i].key == db->db[i + 1].key) {
			WARN_ONCE(1, "mwv207 key: %d is duplicated", db->db[i].key);
			break;
		}
	}
}

int mwv207_db_ioctl(struct drm_device *dev, void *data,
			    struct drm_file *filp)
{
	struct mwv207_device *jdev = dev->dev_private;
	struct drm_mwv207_info_rec rec, *result;
	struct drm_mwv207_info *args = data;
	struct mwv207_db *db = jdev->db;

	switch (args->op) {
	case 1:
		rec.key = args->nr_recs;
		result = bsearch(&rec, db->db, db->nr, 8, cmp_key);
		if (!result) {
			args->recs = 0;
			return -ENOENT;
		}
		args->recs = result->val;

		break;
	case 0:
		if (args->recs)
			return copy_to_user((void __user *)args->recs, db->db,
					db->nr * sizeof(struct drm_mwv207_info_rec));
		args->nr_recs = db->nr;

		break;
	default:
		return -EINVAL;
	}

	return 0;
}
