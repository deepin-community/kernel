// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2025 Mucse Corporation. */

#include <linux/types.h>
#include <linux/module.h>
#include <linux/firmware.h>
#include "rnpm_common.h"
#include "rnpm_mbx.h"
#include "rnpm_mbx_fw.h"
#include "rnpm_mpe.h"

#define MPE_FW_BIN "n10c/n10-mpe.bin"
#define MPE_FW_DATA "n10c/n10-mpe-data.bin"
#define MPE_RPU_BIN "n10c/n10-rpu.bin"

#define CFG_RPU_OFFSET (0x100000) /* 4010_0000 broadcast addr */
#define START_MPE_REG (0x00198700) /* 4019_8700 start all mpe */

/* RV_CORE_STATUS: 4000_6000 */
#define RV_CORE0_WORING_REG (0x6000)
#define RPU_ID (0x6060)

/* RPU_REG */
#define RV_BROADCASE_START_REG (0x106000) /* broadcast to 0x400X_6000 */
#define RPU_DMA_START_REG (0x110000)
#define RPU_ENDIAN_REG (0x110010)
#define N10_START_REG (0x106000)

#define MPE_OFF 0x80000
#define CFG_MPE_ICCM(nr) (0x200000 + (nr) * 0x80000)
#define CFG_MPE_DCCM(nr) (0x220000 + (nr) * 0x80000)

#define RPU_CM3_BASE (0x40000000)
#define RPU_SDRAM_BASE (0x60000000)
#define SDRAM_DEFAULT_VAL (0x88481c00)

#define iowrite32_arrary(rpubase, offset, array, size)               \
	do {                                                         \
		int i;                                               \
		for (i = 0; i < size; i++) {                         \
			rnpm_wr_reg(((char *)(rpubase)) + (offset) + \
					    i * 4,                   \
				    (array)[i]);                     \
		}                                                    \
	} while (0)

#define MPE_LOOP_INST 0x6f

static void rnpm_reset_mpe_and_rpu(struct rnpm_hw *hw)
{
#define SYSCTL_CRG_CTRL12 0x30007030
#define RPU_RESET_BIT 9
	/* reset rpu/mpe/pub */
	cm3_reg_write32(hw, SYSCTL_CRG_CTRL12,
			BIT(RPU_RESET_BIT + 16) | 0);
	/* memory barrior */
	smp_mb();
	mdelay(150);
	cm3_reg_write32(hw, SYSCTL_CRG_CTRL12,
			BIT(RPU_RESET_BIT + 16) | BIT(RPU_RESET_BIT));
	/* memory barrior */
	smp_mb();
	mdelay(100);
}

static void rnpm_start_rpu(char *rpu_base, int do_start)
{
	int mpe_start_v = 0xff, rpu_start_v = 0x1;

	if (do_start == 0) {
		mpe_start_v = 0;
		rpu_start_v = 0;
	}
	rnpm_wr_reg(rpu_base + START_MPE_REG, mpe_start_v);

	/* start all rpu-rv-core */
	rnpm_wr_reg(rpu_base + RV_BROADCASE_START_REG, rpu_start_v);
	/* start rpu */
	rnpm_wr_reg(rpu_base + RPU_DMA_START_REG, rpu_start_v);
	/* memory barrior */
	smp_mb();
}

/*
 * @rpu_base: mapped(0x4000_0000)
 * @mpe_bin : required
 * @mpe_data: optional
 * @rpu_bin : optional
 */
static int rnpm_download_and_start_rpu(
	struct rnpm_pf_adapter *pf_adapter, struct rnpm_hw *hw,
	char *rpu_base, const unsigned int *mpe_bin, const int mpe_bin_sz,
	const unsigned int *mpe_data, const int mpe_data_sz,
	const unsigned int *rpu_bin, const int rpu_sz)
{
	int nr = 0;

	dev_info(&pf_adapter->pdev->dev,
		 "MPE: rpu:%d mpe:%d mpe-data:%d. Downloading...\n",
		 rpu_sz, mpe_bin_sz, mpe_data_sz);
	rnpm_reset_mpe_and_rpu(hw);
	/* download rpu firmeware */
	if (rpu_sz)
		iowrite32_arrary(rpu_base, CFG_RPU_OFFSET + 0x4000,
				 rpu_bin, rpu_sz / 4);

	/* download firmware to 4 mpe-core: mpe0,mpe1,mpe2,mpe3 */
	for (nr = 0; nr < 4; nr++) {
		iowrite32_arrary(rpu_base, CFG_MPE_ICCM(nr), mpe_bin,
				 mpe_bin_sz / 4);
		if (mpe_data_sz)
			iowrite32_arrary(rpu_base, CFG_MPE_DCCM(nr),
					 mpe_data, mpe_data_sz / 4);
	}
	/* memory barrior */
	smp_mb();
	/* enable MPE */
	if (mpe_src_port != 0) {
		rnpm_wr_reg(rpu_base + 0x100000, mpe_pkt_version);
		rnpm_wr_reg(rpu_base + 0x100004, mpe_src_port);
	}

	/* start mpe */
	rnpm_wr_reg(rpu_base + RPU_ENDIAN_REG, 0xf);
	/* memory barrior */
	smp_mb();
	rnpm_start_rpu(rpu_base, 1);
	dev_info(&pf_adapter->pdev->dev, "start mpe/rpu\n");

	return 0;
}

/* load fw bin from: /lib/firmware/ directory */
static const struct firmware *rnpm_load_fw(struct device *dev,
					   const char *fw_name)
{
	const struct firmware *fw;
	int rc;

	rc = request_firmware(&fw, fw_name, dev);
	if (rc != 0)
		return NULL;
	return fw;
}

int rnpm_rpu_mpe_start(struct rnpm_pf_adapter *pf_adapter)
{
	const struct firmware *mpe_bin = NULL, *mpe_data = NULL,
			      *rpu_bin = NULL;
	struct rnpm_hw *hw = &pf_adapter->hw;
	int rpu_version, err = 0;

	rpu_version = cm3_reg_read32(hw, RPU_CM3_BASE + RPU_ID);
	dev_info(&pf_adapter->pdev->dev, "rpu_version:0x%x\n",
		 rpu_version);

	if (rpu_version != 0x20201125) {
		dev_info(&pf_adapter->pdev->dev, "rpu not enabled!\n");
		return -1;
	}

	dev_info(&pf_adapter->pdev->dev, "rpu_addr=%p\n", hw->rpu_addr);
	if (hw->rpu_addr == NULL)
		return -EINVAL;

	mpe_bin = rnpm_load_fw(&pf_adapter->pdev->dev, MPE_FW_BIN);
	if (!mpe_bin) {
		dev_warn(&pf_adapter->pdev->dev, "can't load mpe fw:%s\n",
			 MPE_FW_BIN);
		goto quit;
	}
	mpe_data = rnpm_load_fw(&pf_adapter->pdev->dev, MPE_FW_DATA);
	if (!mpe_data) {
		dev_warn(&pf_adapter->pdev->dev, "no %s, ignored\n",
			 MPE_FW_DATA);
	}
	rpu_bin = rnpm_load_fw(&pf_adapter->pdev->dev, MPE_RPU_BIN);
	if (!rpu_bin)
		dev_warn(&pf_adapter->pdev->dev, "no %s, ignored\n",
			 MPE_RPU_BIN);

	if (((unsigned int *)mpe_bin->data)[4] == 0xA5) {
		pf_adapter->mpe_shm = ((unsigned int *)mpe_bin->data)[2];
	} else {
		dev_err(&pf_adapter->pdev->dev,
			"imvalid mpe bin. magic-code error\n");
		return -EINVAL;
	}
	dev_info(&pf_adapter->pdev->dev, "mpe shm:0x%x nic-mode:%x\n",
		 pf_adapter->mpe_shm, pf_adapter->hw.mode);

	/* only pf0 download mpe/rpu firmware */
	if (pf_adapter->hw.pfvfnum == PF_NUM(0)) {
		err = rnpm_download_and_start_rpu(
			pf_adapter, hw, hw->rpu_addr,
			(unsigned int *)mpe_bin->data, mpe_bin->size,
			mpe_data ? (unsigned int *)mpe_data->data : NULL,
			mpe_data ? mpe_data->size : 0,
			rpu_bin ? (unsigned int *)rpu_bin->data : NULL,
			rpu_bin ? rpu_bin->size : 0);
		if (err != 0) {
			dev_warn(&pf_adapter->pdev->dev,
				 "can't start mpe and rpu\n");
			goto quit;
		}
		pf_adapter->rpu_need_stop = 1;
	}
	pf_adapter->rpu_inited = 1;

quit:
	if (rpu_bin)
		release_firmware(rpu_bin);
	if (mpe_data)
		release_firmware(mpe_data);
	if (mpe_bin)
		release_firmware(mpe_bin);
	return 0;
}

void rnpm_mpe_shm_write32(struct rnpm_pf_adapter *pf_adapter, int nr_mpe,
			  int offset_bytes, unsigned int value)
{
	rnpm_wr_reg(pf_adapter->hw.rpu_addr + CFG_MPE_DCCM(nr_mpe) +
			    pf_adapter->mpe_shm + offset_bytes,
		    value);
}

void rnpm_mpe_shm_write32_array(struct rnpm_pf_adapter *pf_adapter,
				int nr_mpe, int offset_bytes, int *values,
				int bytes)
{
	int i;

	for (i = 0; i < bytes / 4; i++)
		rnpm_mpe_shm_write32(pf_adapter, nr_mpe,
				     offset_bytes + i * 4, values[i]);
}

int rnpm_mpe_shm_read32(struct rnpm_pf_adapter *pf_adapter, int nr_mpe,
			int offset)
{
	int v;

	v = rnpm_rd_reg(pf_adapter->hw.rpu_addr + CFG_MPE_DCCM(nr_mpe) +
			pf_adapter->mpe_shm + offset);
	return v;
}

void rnpm_mpe_shm_read32_array(struct rnpm_pf_adapter *pf_adapter,
			       int nr_mpe, int offset_bytes, int *values,
			       int bytes)
{
	int i;

	for (i = 0; i < bytes / 4; i++)
		values[i] = rnpm_mpe_shm_read32(pf_adapter, nr_mpe,
						offset_bytes + i * 4);
}

void rnpm_rpu_mpe_stop(struct rnpm_pf_adapter *pf_adapter)
{
	if (pf_adapter->rpu_need_stop &&
	    pci_channel_offline(pf_adapter->pdev) == false) {
		rnpm_start_rpu(pf_adapter->hw.rpu_addr, 0);
		rnpm_reset_mpe_and_rpu(&pf_adapter->hw);
	}

	pf_adapter->rpu_need_stop = 0;
}

/*
 * @lane: 0~7 . serdes_lane not port_id
 *@mpe_id_bitmask: [0]:mpe0 [1]:mpe1 [2]:mpe2 [3]:mpe3
 */
int rnpm_mpe_set_queue_info(struct rnpm_pf_adapter *pf_adapter, int lane,
			    int mpe_id_bitmask, int queue_start,
			    int queue_end)
{
	int irss_off = offsetof(struct ipsec_rss_shm, irss);
	int nr_mpe;

	for (nr_mpe = 0; nr_mpe < 4; nr_mpe++) {
		if (BIT(nr_mpe) & mpe_id_bitmask) {
			rnpm_mpe_shm_write32(
				pf_adapter, nr_mpe,
				irss_off +
					lane * sizeof(struct rss_queue_info) +
					offsetof(struct rss_queue_info,
						 valid),
				0);
			rnpm_mpe_shm_write32(
				pf_adapter, nr_mpe,
				irss_off +
					lane * sizeof(struct rss_queue_info) +
					offsetof(struct rss_queue_info,
						 queue_start),
				queue_start);
			rnpm_mpe_shm_write32(
				pf_adapter, nr_mpe,
				irss_off +
					lane * sizeof(struct rss_queue_info) +
					offsetof(struct rss_queue_info,
						 queue_end),
				queue_end);

			rnpm_mpe_shm_write32(
				pf_adapter, nr_mpe,
				irss_off +
					lane * sizeof(struct rss_queue_info) +
					offsetof(struct rss_queue_info,
						 valid),
				INFO_VALID);

			rnpm_mpe_shm_write32(pf_adapter, nr_mpe,
					     offsetof(struct mpe_shm_comm,
						      nic_mode),
					     pf_adapter->hw.mode);
			rnpm_mpe_shm_write32(pf_adapter, nr_mpe,
					     offsetof(struct mpe_shm_comm,
						      valid_magic),
					     DRV_MPE_SHM_MAGIC);
		}
	}

	return 0;
}

int rnpm_ipsec_rss_update_args(struct rnpm_adapter *adapter)
{
	int start, end, lane_gap;
	struct rnpm_ring *ring = NULL;

	if (!adapter->pf_adapter->rpu_inited ||
	    adapter->num_rx_queues <= 0)
		return -EINVAL;
	ring = adapter->rx_ring[0];
	if (!ring)
		return -EINVAL;
	start = ring->rnpm_queue_idx;
	ring = adapter->rx_ring[adapter->num_rx_queues - 1];
	if (!ring)
		return -EINVAL;
	end = ring->rnpm_queue_idx;
	lane_gap = (adapter->hw.pfvfnum == PF_NUM(1)) ? 4 : 0;

	rnpm_mpe_set_queue_info(adapter->pf_adapter,
				adapter->hw.nr_lane + lane_gap, 0b1111,
				start, end);

	return 0;
}
