// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#include <linux/types.h>
#include <linux/module.h>
#include <linux/firmware.h>

#include "rnp_common.h"
#include "rnp_mbx.h"
#include "rnp_mpe.h"
#define MPE_FW_BIN "n10c/n10-mpe.bin"
#define MPE_FW_DATA "n10c/n10-mpe-data.bin"
#define MPE_RPU_BIN "n10c/n10-rpu.bin"

extern unsigned int mpe_src_port;
extern unsigned int mpe_pkt_version;
#define CFG_RPU_OFFSET 0x100000
/* 4010_0000 broadcast addr */
#define START_MPE_REG 0x00198700
/* 4019_8700 start all mpe */

/* RV_CORE_STATUS: 4000_6000 */
#define RV_CORE0_WORING_REG 0x6000
#define RPU_ID 0x6060
/* read-only rpu id */

/* RPU_REG */
#define RV_BROADCAST_START_REG (0x106000)
#define RPU_DMA_START_REG (0x110000)
#define RPU_ENDIAN_REG (0x110010)
#define N10_START_REG (0x106000)

/* MPE0_ICCM:	4020_0000H */
#define CFG_MPE_ICCM(nr) (0x200000 + (nr) * 0x80000)
#define CFG_MPE_DCCM(nr) (0x220000 + (nr) * 0x80000)

#define RPU_CM3_BASE 0x40000000
#define RPU_SDRAM_BASE (0x60000000)
#define SDRAM_DEFAULT_VAL (0x88481c00)

#define iowrite32_arrary(rpubase, offset, array, size)                     \
	do {                                                               \
		int i;                                                     \
		for (i = 0; i < size; i++) {                               \
			rnp_wr_reg(((char *)(rpubase)) + (offset) + i * 4, \
				   (array)[i]);                            \
		}                                                          \
	} while (0)

static void rnp_reset_mpe_and_rpu(struct rnp_hw *hw)
{
#define SYSCTL_CRG_CTRL12 0x30007030
#define RPU_RESET_BIT 9

	/* reset rpu/mpe/pub */
	cm3_reg_write32(hw, SYSCTL_CRG_CTRL12, BIT(RPU_RESET_BIT + 16) | 0);
	smp_mb();
	mdelay(150);
	cm3_reg_write32(hw, SYSCTL_CRG_CTRL12,
			BIT(RPU_RESET_BIT + 16) | BIT(RPU_RESET_BIT));
	smp_mb();
	mdelay(100);
}

static void rnp_start_rpu(char *rpu_base, int do_start)
{
	int mpe_start_v = 0xff, rpu_start_v = 0x1;

	if (do_start == 0) {
		mpe_start_v = 0;
		rpu_start_v = 0;
	}
	rnp_wr_reg(rpu_base + START_MPE_REG, mpe_start_v);

	/* start all rpu-rv-core */
	rnp_wr_reg(rpu_base + RV_BROADCAST_START_REG, rpu_start_v);
	/* start rpu */
	rnp_wr_reg(rpu_base + RPU_DMA_START_REG, rpu_start_v);

	smp_mb();
}

/*
	@rpu_base: mapped(0x4000_0000)
	@mpe_bin : required
	@mpe_data: optional
	@rpu_bin : optional
*/
static int
rnp_download_and_start_rpu(struct rnp_hw *hw, char *rpu_base,
			   const unsigned int *mpe_bin, const int mpe_bin_sz,
			   const unsigned int *mpe_data, const int mpe_data_sz,
			   const unsigned int *rpu_bin, const int rpu_sz)
{
	int nr = 0;

	rnp_info("MPE: rpu:%d mpe:%d mpe-data:%d. Downloading...\n", rpu_sz,
		 mpe_bin_sz, mpe_data_sz);

	rnp_reset_mpe_and_rpu(hw);

	/* download rpu firmeware */
	if (rpu_sz) {
		iowrite32_arrary(rpu_base, CFG_RPU_OFFSET + 0x4000, rpu_bin,
				 rpu_sz / 4);
	}

	/* download firmware to 4 mpe-core: mpe0,mpe1,mpe2,mpe3 */
	for (nr = 0; nr < 4; nr++) {
		iowrite32_arrary(rpu_base, CFG_MPE_ICCM(nr), mpe_bin,
				 mpe_bin_sz / 4);
		if (mpe_data_sz)
			iowrite32_arrary(rpu_base, CFG_MPE_DCCM(nr), mpe_data,
					 mpe_data_sz / 4);
	}
	smp_mb();

	/* Enable MPE */
	if (mpe_src_port != 0) {
		printk("%s %d\n", __func__, __LINE__);
		rnp_wr_reg(rpu_base + 0x100000, mpe_pkt_version);
		rnp_wr_reg(rpu_base + 0x100004, mpe_src_port);
	}

	/* start mpe */
	rnp_wr_reg(rpu_base + RPU_ENDIAN_REG, 0xf);
	smp_mb();
	rnp_start_rpu(rpu_base, 1);

	return 0;
}

/*
 *load fw bin from: /lib/firmware/ directory
 */
static const struct firmware *rnp_load_fw(struct device *dev,
					  const char *fw_name)
{
	const struct firmware *fw;
	int rc;

	rc = request_firmware(&fw, fw_name, dev);
	if (rc != 0) {
		// dev_warn( dev, "Faild to requesting firmware file: %s, %d\n",
		// fw_name, rc);
		return NULL;
	}

	return fw;
}

int rnp_rpu_mpe_start(struct rnp_adapter *adapter)
{
	const struct firmware *mpe_bin = NULL, *mpe_data = NULL,
			      *rpu_bin = NULL;
	struct rnp_hw *hw = &adapter->hw;
	int rpu_version, err = 0;
	// u32 val = 0;

	rpu_version = cm3_reg_read32(hw, RPU_CM3_BASE + RPU_ID);
	dev_info(&adapter->pdev->dev, "rpu_version:0x%x\n", rpu_version);

	if (rpu_version != 0x20201125) {
		dev_info(&adapter->pdev->dev, "rpu not enabled!\n");
		return -1;
	}

	dev_info(&adapter->pdev->dev, "rpu_addr=%p\n", hw->rpu_addr);
	if (hw->rpu_addr == NULL) {
		return -EINVAL;
	}

	mpe_bin = rnp_load_fw(&adapter->pdev->dev, MPE_FW_BIN);
	if (!mpe_bin) {
		dev_warn(&adapter->pdev->dev, "can't load mpe fw:%s\n",
			 MPE_FW_BIN);
		goto quit;
	}
	mpe_data = rnp_load_fw(&adapter->pdev->dev, MPE_FW_DATA);
	if (!mpe_data) {
		dev_warn(&adapter->pdev->dev, "no %s, ignored\n", MPE_FW_DATA);
	}
	rpu_bin = rnp_load_fw(&adapter->pdev->dev, MPE_RPU_BIN);
	if (!rpu_bin) {
		dev_warn(&adapter->pdev->dev, "no %s, ignored\n", MPE_RPU_BIN);
	}

	err = rnp_download_and_start_rpu(
		hw, hw->rpu_addr, (unsigned int *)mpe_bin->data, mpe_bin->size,
		mpe_data ? (unsigned int *)mpe_data->data : NULL,
		mpe_data ? mpe_data->size : 0,
		rpu_bin ? (unsigned int *)rpu_bin->data : NULL,
		rpu_bin ? rpu_bin->size : 0);
	if (err != 0) {
		dev_warn(&adapter->pdev->dev, "can't start mpe and rpu\n");
		goto quit;
	}

	adapter->rpu_inited = 1;

quit:
	if (rpu_bin) {
		release_firmware(rpu_bin);
	}
	if (mpe_data)
		release_firmware(mpe_data);
	if (mpe_bin)
		release_firmware(mpe_bin);
	return 0;
}

void rnp_rpu_mpe_stop(struct rnp_adapter *adapter)
{
	if (adapter->rpu_inited) {
		rnp_start_rpu(adapter->hw.rpu_addr, 0);
		rnp_reset_mpe_and_rpu(&adapter->hw);
	}

	adapter->rpu_inited = 0;
}
