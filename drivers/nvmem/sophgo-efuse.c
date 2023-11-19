// SPDX-License-Identifier: GPL-2.0-only
/*
 * Sophgo SoC eFuse driver
 */

#include <linux/bitfield.h>
#include <linux/bits.h>
#include <linux/clk.h>
#include <linux/cleanup.h>
#include <linux/iopoll.h>
#include <linux/math.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/nvmem-provider.h>
#include <linux/platform_device.h>

#define SG2044_EFUSE_CONTENT_SIZE		0x400

#define SG2044_EFUSE_MD				0x000
#define SG2044_EFUSE_ADR			0x004
#define SG2044_EFUSE_RD_DATA			0x00c

#define SG2044_EFUSE_MODE			GENMASK(1, 0)
#define SG2044_EFUSE_MODE_READ			2

#define SG2044_EFUSE_BOOT_DONE			BIT(7)
#define SG2044_BOOT_TIMEOUT			10000

#define SG2044_EFUSE_ADR_ADDR			GENMASK(7, 0)

#define SG2044_EFUSE_ALIGN			4

struct sophgo_efuses {
	void __iomem *base;
	struct clk_bulk_data *clks;
	int num_clks;
	struct mutex mutex;
};

static int sg2044_efuse_wait_mode(struct sophgo_efuses *efuse)
{
	u32 value;

	return readl_poll_timeout(efuse->base + SG2044_EFUSE_MD, value,
				  FIELD_GET(SG2044_EFUSE_MODE, value) == 0,
				  1, SG2044_BOOT_TIMEOUT);
}

static int sg2044_efuse_set_mode(struct sophgo_efuses *efuse, int mode)
{
	u32 val = readl(efuse->base + SG2044_EFUSE_MD);

	val &= ~SG2044_EFUSE_MODE;
	val |= FIELD_PREP(SG2044_EFUSE_MODE, mode);

	writel(val, efuse->base + SG2044_EFUSE_MD);

	return sg2044_efuse_wait_mode(efuse);
}

static u32 sg2044_efuses_read_strip(struct sophgo_efuses *efuse,
				    unsigned int offset, u32 *strip)
{
	u32 val = FIELD_PREP(SG2044_EFUSE_ADR_ADDR, offset);
	int ret;

	guard(mutex)(&efuse->mutex);

	writel(val, efuse->base + SG2044_EFUSE_ADR);

	ret = sg2044_efuse_set_mode(efuse, SG2044_EFUSE_MODE_READ);
	if (ret < 0)
		return ret;

	*strip = readl(efuse->base + SG2044_EFUSE_RD_DATA);

	return 0;
}

static int sg2044_efuses_read(void *context, unsigned int offset, void *val,
			      size_t bytes)
{
	struct sophgo_efuses *efuse = context;
	unsigned int start, start_offset, end, i;
	u32 value;
	u8 *buf;
	int ret;

	start = rounddown(offset, SG2044_EFUSE_ALIGN);
	end = roundup(offset + bytes, SG2044_EFUSE_ALIGN);
	start_offset = offset - start;

	start /= SG2044_EFUSE_ALIGN;
	end /= SG2044_EFUSE_ALIGN;

	ret = readl_poll_timeout(efuse->base + SG2044_EFUSE_MD, value,
				 (value & SG2044_EFUSE_BOOT_DONE),
				 1, SG2044_BOOT_TIMEOUT);
	if (ret < 0)
		return ret;

	buf = kzalloc(end - start, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	for (i = start; i < end; i++) {
		ret = sg2044_efuses_read_strip(efuse, i, &value);
		if (ret)
			goto failed;

		memcpy(&buf[(i - start) * 4], &value, SG2044_EFUSE_ALIGN);
	}

	memcpy(val, buf + start_offset, bytes);

failed:
	kfree(buf);

	return ret;
}

static int sophgo_efuses_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sophgo_efuses *efuse;
	struct nvmem_config config = {
		.dev = &pdev->dev,
		.add_legacy_fixed_of_cells = true,
		.read_only = true,
		.reg_read = sg2044_efuses_read,
		.stride = 1,
		.word_size = 1,
		.name = "sophgo-efuse",
		.id = NVMEM_DEVID_AUTO,
		.root_only = true,
	};

	efuse = devm_kzalloc(dev, sizeof(*efuse), GFP_KERNEL);
	if (!efuse)
		return -ENOMEM;

	efuse->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(efuse->base))
		return PTR_ERR(efuse->base);

	efuse->num_clks = devm_clk_bulk_get_all_enabled(&pdev->dev, &efuse->clks);
	if (efuse->num_clks < 0)
		return dev_err_probe(dev, efuse->num_clks, "failed to get clocks\n");

	config.priv = efuse;
	config.size = SG2044_EFUSE_CONTENT_SIZE;

	return PTR_ERR_OR_ZERO(devm_nvmem_register(config.dev, &config));
}

static const struct of_device_id sophgo_efuses_of_match[] = {
	{ .compatible = "sophgo,sg2044-efuse", },
	{}
};

MODULE_DEVICE_TABLE(of, sophgo_efuses_of_match);

static struct platform_driver sophgo_efuses_driver = {
	.driver = {
		.name = "sophgo_efuse",
		.of_match_table = sophgo_efuses_of_match,
	},
	.probe = sophgo_efuses_probe,
};

module_platform_driver(sophgo_efuses_driver);

MODULE_AUTHOR("Inochi Amaoto <inochiama@gmail.com>");
MODULE_DESCRIPTION("Sophgo efuse driver");
MODULE_LICENSE("GPL");
