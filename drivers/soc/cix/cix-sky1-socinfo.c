// SPDX-License-Identifier: GPL-2.0+
/*
 * Add CIX SKY1 SoC Version driver
 */

#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sys_soc.h>
#include <linux/bitfield.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/nvmem-consumer.h>


#define	SKY1_OPN_OFFSET		0x12c

#define REVISION_MASK			0xFF
#define SOC_ID_MASK			0xFFF00
#define SOC_ID_SHIFT_BITS		0x8
#define SERIAL_NUMBER_MASK		0x1F00000
#define SERIAL_SHIFT_BITS		0x14

/* Part definition ID: Reversion */
#define A0_GENEARL			0x0
#define A0_MINI_DESK			0x1
#define A0_COCKPIT			0x2
#define A1_CLOUND_BOOK			0x3
#define B0_GENERAL			0x4

/* ROAD MAP definition ID: SoC ID */
#define ROAD_MAP_SKY1			0x111
#define ROAD_MAP_SKY1_PLUS		0x112
#define ROAD_MAP_SKY2			0x121
#define ROAD_MAP_STAR			0x211
#define ROAD_MAP_SEA			0x311

/* Serial Number ID */
#define AUTOMOTIVE			0x1
#define PC_DESKTOP			0x2
#define EMBEDDED_SERVER			0x3
#define EMBEDDED_DESKTOP		0x4
#define EMBEDDED_MOBILE			0x5
#define GAMING				0x6
#define PC_LAPTOP			0x7
#define SERVER				0x8
#define ULP				0x9
#define WORKSTATION			0x10

/* Firmware Offset */
#define SE_FIRMWARE_OFFSET		0x0
#define PBL_FIRMWARE_OFFSET		0x80
#define ATF_FIRMWARE_OFFSET		0x100
#define PM_FIRMWARE_OFFSET		0x180
#define TEE_FIRMWARE_OFFSET		0x200
#define UEFI_FIRMWARE_OFFSET		0x280
#define EC_FIRMWARE_OFFSET		0x300
#define BOARD_ID_OFFSET			0x380

#define MAX_BYTES			0x7f

static const struct of_device_id cix_sky1_top_ids[] = {
	{ .compatible = "cix,sky1-top", },
	{ /* sentinel */ }
};

struct sky1_firmware_version {
	const char *se_version;
	const char *pbl_version;
	const char *atf_version;
	const char *pm_version;
	const char *tee_version;
	const char *uefi_version;
	const char *ec_version;
	const char *board_id;
};

static struct sky1_firmware_version *firmware_version;

static void *cix_sky1_firmware_parse(const void __iomem *base)
{
	unsigned int len = 0;
	unsigned char *buf, *tmp0, *tmp1;

	tmp0 = (char *)base;
	tmp1 = (char *)base;

	while (*tmp0++ && len <= MAX_BYTES)
		len++;

	if (len > 0 && len <= MAX_BYTES)
		buf = kasprintf(GFP_KERNEL, "Version %s", tmp1);
	else
		buf = kasprintf(GFP_KERNEL, "firmware_version_parse unknown");

	return buf;
}

static const char *cix_sky1_socinfo_revision(unsigned int opn)
{
	unsigned int revision = opn & REVISION_MASK;
	char *socinfo_revision;

	switch (revision) {
	case A0_GENEARL:
		socinfo_revision = kasprintf(GFP_KERNEL, "Rev %s",
					     "A0_GENEARL");
		break;

	case A0_MINI_DESK:
		socinfo_revision = kasprintf(GFP_KERNEL, "Rev %s",
					     "A0_MINI_DESK");
		break;

	case A0_COCKPIT:
		socinfo_revision = kasprintf(GFP_KERNEL, "Rev %s",
					     "A0_COCKPIT");
		break;

	case A1_CLOUND_BOOK:
		socinfo_revision = kasprintf(GFP_KERNEL, "Rev %s",
					     "A1_CLOUND_BOOK");
		break;

	case B0_GENERAL:
		socinfo_revision = kasprintf(GFP_KERNEL, "Rev %s",
					     "B0_MINI_DESK");
		break;

	default:
		socinfo_revision = kasprintf(GFP_KERNEL, "Rev %s",
					     "ERROR_REVISION_NUMBER");
		break;
	}

	return socinfo_revision;
}

static const char *cix_sky1_socinfo_soc_id(unsigned int opn)
{
	unsigned int soc_id = (opn & SOC_ID_MASK) >> SOC_ID_SHIFT_BITS;
	char *socinfo_soc_id;

	switch (soc_id) {
	case ROAD_MAP_SKY1:
		socinfo_soc_id = kasprintf(GFP_KERNEL, "SOC_ID %s",
					     "ROAD_MAP_SKY1");
		break;

	case ROAD_MAP_SKY1_PLUS:
		socinfo_soc_id = kasprintf(GFP_KERNEL, "SOC_ID %s",
					     "ROAD_MAP_SKY1_PLUS");
		break;

	case ROAD_MAP_SKY2:
		socinfo_soc_id = kasprintf(GFP_KERNEL, "SOC_ID %s",
					     "ROAD_MAP_SKY2");
		break;

	case ROAD_MAP_STAR:
		socinfo_soc_id = kasprintf(GFP_KERNEL, "SOC_ID %s",
					     "ROAD_MAP_STAR");
		break;

	case ROAD_MAP_SEA:
		socinfo_soc_id = kasprintf(GFP_KERNEL, "SOC_ID %s",
					     "ROAD_MAP_SEA");
		break;

	default:
		socinfo_soc_id = kasprintf(GFP_KERNEL, "SOC_ID %s",
					     "ERROR_SOC_ID_NUMBER");
		break;

	}

	return socinfo_soc_id;
}

static const char *cix_sky1_socinfo_serial_number(unsigned int opn)
{
	unsigned int serial_number = (opn & SERIAL_NUMBER_MASK) >> SERIAL_SHIFT_BITS;
	char *socinfo_serial_number;

	switch (serial_number) {
	case(AUTOMOTIVE):
		socinfo_serial_number = kasprintf(GFP_KERNEL, "Serail %s",
					     "AUTOMOTIVE");
		break;

	case(PC_DESKTOP):
		socinfo_serial_number = kasprintf(GFP_KERNEL, "Serail %s",
					     "DESKTOP");
		break;

	case(EMBEDDED_SERVER):
		socinfo_serial_number = kasprintf(GFP_KERNEL, "Serail %s",
					     "EMBEDDED_SERVER");
		break;

	case(EMBEDDED_DESKTOP):
		socinfo_serial_number = kasprintf(GFP_KERNEL, "Serail %s",
					     "EMBEDDED_DESKTOP");
		break;

	case(EMBEDDED_MOBILE):
		socinfo_serial_number = kasprintf(GFP_KERNEL, "Serail %s",
					     "EMBEDDED_MOBILE");
		break;

	case(GAMING):
		socinfo_serial_number = kasprintf(GFP_KERNEL, "Serail %s",
					     "EMBEDDED_GAMING");
		break;

	case(PC_LAPTOP):
		socinfo_serial_number = kasprintf(GFP_KERNEL, "Serail %s",
					     "MOBILE");
		break;

	case(SERVER):
		socinfo_serial_number = kasprintf(GFP_KERNEL, "Serail %s",
					     "SERVER");
		break;

	case(ULP):
		socinfo_serial_number = kasprintf(GFP_KERNEL, "Serail %s",
					     "ULP");
		break;

	case(WORKSTATION):
		socinfo_serial_number = kasprintf(GFP_KERNEL, "Serail %s",
					     "WORKSTATION_GRAPHICS");
		break;

	default:
		socinfo_serial_number = kasprintf(GFP_KERNEL, "Serail %s",
					     "ERROR SERIAL NUMBER");
		break;
	}

	return socinfo_serial_number;
}

static ssize_t firmware_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "SE Version: %s\n PBL Version: %s\n"
			"ATF Version: %s\n PM version: %s\n TEE Version: %s\n"
			"UEFI Version: %s\n EC Version:%s\n Board ID: %s\n",
			firmware_version->se_version, firmware_version->pbl_version,
			firmware_version->atf_version, firmware_version->pm_version,
			firmware_version->tee_version, firmware_version->uefi_version,
			firmware_version->ec_version, firmware_version->board_id);
}

static DEVICE_ATTR_RO(firmware_version);

static struct attribute *firmware_attrs[] = {
	&dev_attr_firmware_version.attr,
	NULL,
};

ATTRIBUTE_GROUPS(firmware);

static int __init cix_sky1_socinfo_init(void)
{
	struct soc_device_attribute *soc_dev_attr;
	struct soc_device *soc_dev;
	struct device_node *np;
	void __iomem *soc_info_base;
	void __iomem *firmware_version_base;
	unsigned int cix_sky1_opn;

	np = of_find_matching_node(NULL, cix_sky1_top_ids);
	if (!np)
		return -ENODEV;

	soc_info_base = of_iomap(np, 0);
	if (!soc_info_base)
		return -ENXIO;

	firmware_version_base = of_iomap(np, 1);
	if (!firmware_version_base)
		return -ENXIO;

	firmware_version = kzalloc(sizeof(*firmware_version), GFP_KERNEL);
	if (!firmware_version)
		goto alloc_err;

	firmware_version->se_version = cix_sky1_firmware_parse(firmware_version_base +
								SE_FIRMWARE_OFFSET);
	if (!firmware_version->se_version)
		goto alloc_err;

	firmware_version->pbl_version = cix_sky1_firmware_parse(firmware_version_base +
								PBL_FIRMWARE_OFFSET);
	if (!firmware_version->pbl_version)
		goto alloc_err;

	firmware_version->atf_version = cix_sky1_firmware_parse(firmware_version_base +
								ATF_FIRMWARE_OFFSET);
	if (!firmware_version->atf_version)
		goto alloc_err;

	firmware_version->pm_version = cix_sky1_firmware_parse(firmware_version_base +
								PM_FIRMWARE_OFFSET);
	if (!firmware_version->pm_version)
		goto alloc_err;

	firmware_version->tee_version = cix_sky1_firmware_parse(firmware_version_base +
								TEE_FIRMWARE_OFFSET);
	if (!firmware_version->tee_version)
		goto alloc_err;

	firmware_version->uefi_version = cix_sky1_firmware_parse(firmware_version_base +
								UEFI_FIRMWARE_OFFSET);
	if (!firmware_version->uefi_version)
		goto alloc_err;

	firmware_version->ec_version = cix_sky1_firmware_parse(firmware_version_base +
								EC_FIRMWARE_OFFSET);
	if (!firmware_version->ec_version)
		goto alloc_err;

	firmware_version->board_id = cix_sky1_firmware_parse(firmware_version_base +
								BOARD_ID_OFFSET);
	if (!firmware_version->board_id)
		goto alloc_err;

	cix_sky1_opn = readl_relaxed(soc_info_base + SKY1_OPN_OFFSET);

	soc_dev_attr = kzalloc(sizeof(*soc_dev_attr), GFP_KERNEL);
	if (!soc_dev_attr)
		return -ENODEV;

	soc_dev_attr->family = "Cix SoC";
	np = of_find_node_by_path("/");

	cix_sky1_opn = readl_relaxed(soc_info_base + SKY1_OPN_OFFSET);

	soc_dev_attr = kzalloc(sizeof(*soc_dev_attr), GFP_KERNEL);
	if (!soc_dev_attr)
		return -ENODEV;

	soc_dev_attr->family = "Cix SoC";
	np = of_find_node_by_path("/");
	of_property_read_string(np, "model", &soc_dev_attr->machine);
	of_node_put(np);

	soc_dev_attr->revision = cix_sky1_socinfo_revision(cix_sky1_opn);

	soc_dev_attr->soc_id = cix_sky1_socinfo_soc_id(cix_sky1_opn);

	soc_dev_attr->serial_number = cix_sky1_socinfo_serial_number(cix_sky1_opn);

	soc_dev_attr->custom_attr_group = firmware_groups[0];

	soc_dev = soc_device_register(soc_dev_attr);
	if (IS_ERR(soc_dev)) {
		kfree_const(soc_dev_attr->revision);
		kfree_const(soc_dev_attr->soc_id);
		kfree_const(soc_dev_attr->serial_number);
		kfree(soc_dev_attr);
		kfree_const(firmware_version->se_version);
		kfree_const(firmware_version->pbl_version);
		kfree_const(firmware_version->atf_version);
		kfree_const(firmware_version->pm_version);
		kfree_const(firmware_version->tee_version);
		kfree_const(firmware_version->uefi_version);
		kfree_const(firmware_version->ec_version);
		kfree_const(firmware_version->board_id);
		kfree(firmware_version);
		return PTR_ERR(soc_dev);
	}

	dev_info(soc_device_to_device(soc_dev), "CIX SKY1 %s %s %s detected\n",
		 soc_dev_attr->revision, soc_dev_attr->serial_number, soc_dev_attr->soc_id);

	return 0;

alloc_err:
	kfree_const(firmware_version->se_version);
	kfree_const(firmware_version->pbl_version);
	kfree_const(firmware_version->atf_version);
	kfree_const(firmware_version->pm_version);
	kfree_const(firmware_version->tee_version);
	kfree_const(firmware_version->uefi_version);
	kfree_const(firmware_version->ec_version);
	kfree_const(firmware_version->board_id);
	kfree(firmware_version);
	return -ENODEV;
}
device_initcall(cix_sky1_socinfo_init);
