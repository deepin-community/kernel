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
#include "mwv207.h"
#include "mwv207_vbios.h"

/*
 */
static inline u8 mwv207_flash_read8(struct mwv207_device *jdev, u32 offset)
{
	return ioread8(jdev->mmio + 0x100000 + offset);
}

static inline u16 mwv207_flash_read16(struct mwv207_device *jdev, u32 offset)
{
	u16 dat = ioread16(jdev->mmio + 0x100000 + offset);

	return le16_to_cpu(dat);
}

static inline u32 mwv207_flash_read32(struct mwv207_device *jdev, u32 offset)
{
	u32 dat = ioread32(jdev->mmio + 0x100000 + offset);

	return le32_to_cpu(dat);
}

static inline void mwv207_flash_cpy(struct mwv207_device *jdev, void *dst, u32 offset, u32 len)
{
	memcpy_fromio(dst, jdev->mmio + 0x100000 + offset, len);
}

static int mwv207_vbios_check_indexer(struct mwv207_device *jdev, u32 offset)
{
	u16 count, chksum, calc;

	if (mwv207_flash_read16(jdev, offset) != 0xaa55)
		return -EINVAL;

	count  = mwv207_flash_read16(jdev, offset + 2);
	chksum = mwv207_flash_read16(jdev, offset + 4);

	jdev->vbios.sector_count = count;
	for (calc = 0, offset += 8, count *= 8; count > 0; count--)
		calc += mwv207_flash_read8(jdev, offset++);

	if (calc != chksum)
		return -EINVAL;

	jdev->vbios.indexer_valid = 1;
	return 0;
}

static inline int mwv207_vbios_sector(struct mwv207_device *jdev, u32 idx, u32 *poffset)
{
	if (poffset == NULL || !jdev->vbios.indexer_valid || idx >= jdev->vbios.sector_count)
		return -EINVAL;

	*poffset = mwv207_flash_read32(jdev, 0xa000 + 0x8 + idx * 8 + 4);
	return 0;
}

static inline int mwv207_vbios_insert_vdat(struct mwv207_device *jdev, u32 key,
		struct mwv207_vdat *vdat)
{
	int ret;

	mutex_lock(&jdev->vbios.cfg_lock);
	ret = idr_alloc(&jdev->vbios.cfg_table, vdat, key, key + 1, GFP_KERNEL);
	mutex_unlock(&jdev->vbios.cfg_lock);

	return ret;
}

static void mwv207_vbios_parse_hdmi(struct mwv207_device *jdev)
{
	u32 count, offset, chksum, len;
	struct mwv207_vdat *vdat[4];
	u32 select[4];
	u8  magic[8];
	int ret, i;

	ret = mwv207_vbios_sector(jdev, 0xa, &offset);
	if (ret)
		return;

	mwv207_flash_cpy(jdev, magic, offset, 8);
	if (memcmp(magic, "JMPHYCFG", 8))
		return;

	len = mwv207_flash_read32(jdev, offset + 0x18);
	if (len > 0x1200 || (len % 1140))
		return;
	count = len / 1140;

	for (chksum = 0, i = 0; i < len; i += 4)
		chksum += mwv207_flash_read32(jdev, offset + 0x20 + i);
	if (chksum != mwv207_flash_read32(jdev, offset + 0x14))
		return;

	for (i = 0; i < 4; i++) {
		select[i] = mwv207_flash_read8(jdev, offset +  0x1c + i);
		if (select[i] > min_t(u32, 3, count - 1))
			return;
	}

	for (i = 0; i < 4; i++) {
		void *dat;

		vdat[i] = devm_kzalloc(jdev->dev, sizeof(*vdat[i]), GFP_KERNEL);
		if (!vdat[i])
			return;
		vdat[i]->len = 1140;
		dat = devm_kmalloc(jdev->dev, vdat[i]->len, GFP_KERNEL);
		if (!dat)
			return;
		mwv207_flash_cpy(jdev, dat,
				offset + 0x20 + select[i] * 1140,
				1140);

		vdat[i]->dat = dat;

		(void)mwv207_vbios_insert_vdat(jdev, 0xfff0 + i, vdat[i]);
	}
}

static void mwv207_vbios_parse_edp(struct mwv207_device *jdev)
{

}

static void mwv207_vbios_parse_cfg(struct mwv207_device *jdev)
{

}

const struct mwv207_vdat *mwv207_vbios_vdat(struct mwv207_device *jdev, u32 key)
{
	struct mwv207_vdat *dat;

	mutex_lock(&jdev->vbios.cfg_lock);
	dat = idr_find(&jdev->vbios.cfg_table, key);
	mutex_unlock(&jdev->vbios.cfg_lock);

	return dat;
}

void mwv207_vbios_init(struct mwv207_device *jdev)
{
	int ret;

	mutex_init(&jdev->vbios.vcmd_lock);
	mutex_init(&jdev->vbios.cfg_lock);
	idr_init(&jdev->vbios.cfg_table);

	ret = mwv207_vbios_check_indexer(jdev, 0xa000);
	if (ret) {
		pr_warn("mwv207: vbios indexer is not valid");
		return;
	}

	mwv207_vbios_parse_hdmi(jdev);
	mwv207_vbios_parse_edp(jdev);
	mwv207_vbios_parse_cfg(jdev);
}

void mwv207_vbios_fini(struct mwv207_device *jdev)
{
	struct mwv207_vdat *vdat;
	u32 id;

	idr_for_each_entry(&jdev->vbios.cfg_table, vdat, id) {
		devm_kfree(jdev->dev, vdat->dat);
		devm_kfree(jdev->dev, vdat);
	}
	idr_destroy(&jdev->vbios.cfg_table);
}
