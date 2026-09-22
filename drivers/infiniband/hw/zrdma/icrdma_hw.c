// SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
/* Copyright (c) 2023 - 2024 ZTE Corporation */
#include "osdep.h"
#include "type.h"
#include "icrdma_hw.h"
#include "main.h"

static u64 icrdma_masks[ZXDH_MAX_MASKS] = {
	ICRDMA_CCQPSTATUS_CCQP_DONE, ICRDMA_CCQPSTATUS_CCQP_ERR,
	ICRDMA_CQPSQ_STAG_PDID,	     ICRDMA_CQPSQ_CQ_CEQID,
	ICRDMA_CQPSQ_CQ_CQID,	     ICRDMA_COMMIT_FPM_CQCNT,
};

static u8 icrdma_shifts[ZXDH_MAX_SHIFTS] = {
	ICRDMA_CCQPSTATUS_CCQP_DONE_S, ICRDMA_CCQPSTATUS_CCQP_ERR_S,
	ICRDMA_CQPSQ_STAG_PDID_S,      ICRDMA_CQPSQ_CQ_CEQID_S,
	ICRDMA_CQPSQ_CQ_CQID_S,	       ICRDMA_COMMIT_FPM_CQCNT_S,
};

static unsigned int zxdh_dbi_en = 1;
module_param(zxdh_dbi_en, uint, 0444);
MODULE_PARM_DESC(zxdh_dbi_en, "zxdh_dbi_en =1, enable dbi module");

static unsigned int zxdh_ep_addr = 0x948;
module_param(zxdh_ep_addr, uint, 0444);
MODULE_PARM_DESC(zxdh_ep_addr,
		 "zxdh_ep_addr = 0x948, dbi model ,0x948 is register addr");

static unsigned int zxdh_ep_id;
module_param(zxdh_ep_id, uint, 0444);
MODULE_PARM_DESC(zxdh_ep_id,
		 "zxdh_ep_id 0 is 5, 1 is 6, 2 is 7, 3 is 8, 4 is 9");

/**
 * zxdh_rdma_ena_ceq_irq - Enable ceq interrupt
 * @dev: pointer to the device structure
 * @ceq_id: ceq id
 */
static void zxdh_rdma_ena_ceq_irq(struct zxdh_sc_dev *dev, u32 ceq_id)
{
	u32 hdr;

	hdr = FIELD_PREP(ZXDH_CEQ_ARM_VHCA_ID, dev->vhca_id) |
	      FIELD_PREP(ZXDH_CEQ_ARM_CEQ_ID, ceq_id);
	dma_wmb(); /* make sure WQE is populated before valid bit is set */
	writel(hdr, dev->ceq_itr_enable);
	// printk("%s hdr:0x%x\n",__func__,hdr);
}

/**
 * zxdh_rdma_ena_aeq_irq - Enable aeq interrupt
 * @dev: pointer to the device structure
 * @enable: enable value
 */
static void zxdh_rdma_ena_aeq_irq(struct zxdh_sc_dev *dev, bool enable)
{
	writel(enable, dev->aeq_itr_enable);
}

/**
 * zxdh_rdma_ena_ext_aeq_irq - Enable ext aeq interrupt
 * @ext_dev: pointer to the device structure
 * @enable: enable value
 */
static void zxdh_rdma_ena_ext_aeq_irq(struct zxdh_sc_dev *ext_dev, bool enable)
{
	writel(enable, ext_dev->aeq_itr_enable);
}

static const struct zxdh_irq_ops zxdh_rdma_irq_ops = {
	.zxdh_cfg_aeq = zxdh_cfg_aeq,
	.zxdh_ceq_en_irq = zxdh_rdma_ena_ceq_irq,
	.zxdh_aeq_en_irq = zxdh_rdma_ena_aeq_irq,
};

static const struct zxdh_irq_ops zxdh_rdma_ext_irq_ops = {
	.zxdh_cfg_aeq = zxdh_cfg_ext_aeq,
	.zxdh_aeq_en_irq = zxdh_rdma_ena_ext_aeq_irq,
};

static void zxdh_init_ceq_hw(struct zxdh_sc_dev *dev)
{
	struct zxdh_pci_f *rf;
	u32 hdr;
	u8 __iomem *hw_addr;

	hw_addr = dev->hw->hw_addr;
	rf = container_of(dev, struct zxdh_pci_f, sc_dev);

	dev->ceq_itr_enable = (u32 __iomem *)(hw_addr + C_CEQ_EQARM_RAM(dev->ep_id));
	dev->ceq_axi.ceqe_axi_info =
		(u32 __iomem *)(hw_addr + C_CEQ_CEQE_AXI_INFO_RAM(dev->ep_id));
	dev->ceq_axi.rpble_axi_info =
		(u32 __iomem *)(hw_addr + C_CEQ_RPBLE_AXI_INFO_RAM(dev->ep_id));
	dev->ceq_axi.lpble_axi_info =
		(u32 __iomem *)(hw_addr + C_CEQ_LPBLE_AXI_INFO_RAM(dev->ep_id));
	dev->ceq_axi.int_info = (u32 __iomem *)(hw_addr + C_CEQ_INT_INFO_RAM(dev->ep_id));

	hdr = FIELD_PREP(ZXDH_CEQ_CEQE_AXI_INFO_INDICATE_ID,
			 dev->soc_tx_rx_cqp_ind) | //�1�7�1�7�1�7�1�7smmu
	      FIELD_PREP(ZXDH_CEQ_CEQE_AXI_INFO_AXI_ID,
			 dev->soc_tx_rx_cqp_axid); //ep5
	dma_wmb(); /* make sure WQE is populated before valid bit is set */
	writel(hdr, dev->ceq_axi.ceqe_axi_info);

	hdr = FIELD_PREP(ZXDH_CEQ_PBLE_AXI_INFO_CACHE_ID, dev->cache_id) |
	      FIELD_PREP(ZXDH_CEQ_CEQE_AXI_INFO_AXI_ID,
			 dev->soc_tx_rx_cqp_axid); //ep5
	dma_wmb(); /* make sure WQE is populated before valid bit is set */
	writel(hdr, dev->ceq_axi.rpble_axi_info);

	hdr = FIELD_PREP(ZXDH_CEQ_PBLE_AXI_INFO_CACHE_ID, dev->cache_id) |
	      FIELD_PREP(ZXDH_CEQ_CEQE_AXI_INFO_AXI_ID,
			 dev->soc_tx_rx_cqp_axid); //ep5
	dma_wmb(); /* make sure WQE is populated before valid bit is set */
	writel(hdr, dev->ceq_axi.lpble_axi_info);

	hdr = FIELD_PREP(ZXDH_CEQ_INT_PCIE_DBI_EN, zxdh_dbi_en) |
	      FIELD_PREP(ZXDH_CEQ_INT_EP_ID, rf->ep_id) |
	      FIELD_PREP(ZXDH_CEQ_INT_PF_NUM, rf->pf_id) |
	      FIELD_PREP(ZXDH_CEQ_INT_VF_NUM, rf->vf_id) |
	      FIELD_PREP(ZXDH_CEQ_INT_VF_ACTIVE, rf->ftype);

	dma_wmb(); /* make sure WQE is populated before valid bit is set */
	writel(hdr, dev->ceq_axi.int_info);
}

static void zxdh_init_aeq_hw(struct zxdh_sc_dev *dev)
{
	u8 __iomem *hw_addr;
	u32 hdr;

	hw_addr = dev->hw->hw_addr;

	dev->aeq_itr_enable = (u32 __iomem *)(hw_addr + C_RDMA_CPU_AEQ_ARM(dev->ep_id));
	dev->aeq_tail_pointer =
		(u32 __iomem *)(hw_addr + C_RDMA_CPU_SOFTWARE_TAIL(dev->ep_id));
	dev->aeq_vhca_pfvf.aeq_msix_data =
		(u32 __iomem *)(hw_addr + RDMA_CPU_MSIX_DATA(dev->ep_id));
	dev->aeq_vhca_pfvf.aeq_msix_config =
		(u32 __iomem *)(hw_addr + RDMA_CPU_MSIX_CONFIG(dev->ep_id));
	dev->aeq_vhca_pfvf.aeq_root_axi_data =
		(u32 __iomem *)(hw_addr + AEQ_REPORT_ROOT_AXI_DATA(dev->ep_id));
	dev->aeq_vhca_pfvf.aeq_leaf_axi_data =
		(u32 __iomem *)(hw_addr + AEQ_REPORT_LEAF_AXI_DATA(dev->ep_id));
	dev->aeq_vhca_pfvf.aeq_wr_axi_data =
		(u32 __iomem *)(hw_addr + AEQ_REPORT_WR_AXI_DATA(dev->ep_id));
	dev->aeq_vhca_pfvf.aeq_aee_flag =
		(u32 __iomem *)(hw_addr + AEQ_AEQC_AEE_FLAG(dev->ep_id));

	writel(0, dev->aeq_tail_pointer);
	//soc hmc config
	hdr = FIELD_PREP(ZXDH_AEQ_CACHE_ID, dev->cache_id) |
	      FIELD_PREP(ZXDH_AEQ_AXI_ID, dev->soc_tx_rx_cqp_axid) |
	      FIELD_PREP(ZXDH_AEQ_WAY_PATITION, 0);
	dma_wmb(); /* make sure WQE is populated before valid bit is set */
	writel(hdr, dev->aeq_vhca_pfvf.aeq_root_axi_data);

	hdr = FIELD_PREP(ZXDH_AEQ_CACHE_ID, dev->cache_id) |
	      FIELD_PREP(ZXDH_AEQ_AXI_ID, dev->soc_tx_rx_cqp_axid) |
	      FIELD_PREP(ZXDH_AEQ_WAY_PATITION, 0);
	dma_wmb(); /* make sure WQE is populated before valid bit is set */
	writel(hdr, dev->aeq_vhca_pfvf.aeq_leaf_axi_data);
	//soc data config
	hdr = FIELD_PREP(ZXDH_AEQ_INDICIATE_ID, dev->soc_tx_rx_cqp_ind) |
	      FIELD_PREP(ZXDH_AEQ_AXI_ID, dev->soc_tx_rx_cqp_axid) |
	      FIELD_PREP(ZXDH_AEQ_WAY_PATITION, 0);
	dma_wmb(); /* make sure WQE is populated before valid bit is set */
	writel(hdr, dev->aeq_vhca_pfvf.aeq_wr_axi_data);
	//clear 0 after reading values during maintenance
	writel(0, dev->aeq_vhca_pfvf.aeq_aee_flag);
}

static void zxdh_init_ext_aeq_hw(struct zxdh_sc_dev *ext_dev)
{
	u8 __iomem *hw_addr;
	u32 hdr;

	hw_addr = ext_dev->hw->ext_hw_addr;

	ext_dev->aeq_itr_enable = (u32 __iomem *)(hw_addr + C_RDMA_EXT_CPU_AEQ_ARM(ext_dev->ep_id));
	ext_dev->aeq_tail_pointer =
		(u32 __iomem *)(hw_addr + C_RDMA_EXT_CPU_SOFTWARE_TAIL(ext_dev->ep_id));
	ext_dev->aeq_vhca_pfvf.aeq_msix_data =
		(u32 __iomem *)(hw_addr + RDMA_EXT_CPU_MSIX_DATA(ext_dev->ep_id));
	ext_dev->aeq_vhca_pfvf.aeq_msix_config =
		(u32 __iomem *)(hw_addr + RDMA_EXT_CPU_MSIX_CONFIG(ext_dev->ep_id));
	ext_dev->aeq_vhca_pfvf.aeq_root_axi_data =
		(u32 __iomem *)(hw_addr + EXT_AEQ_REPORT_ROOT_AXI_DATA(ext_dev->ep_id));
	ext_dev->aeq_vhca_pfvf.aeq_leaf_axi_data =
		(u32 __iomem *)(hw_addr + EXT_AEQ_REPORT_LEAF_AXI_DATA(ext_dev->ep_id));
	ext_dev->aeq_vhca_pfvf.aeq_wr_axi_data =
		(u32 __iomem *)(hw_addr + EXT_AEQ_REPORT_WR_AXI_DATA(ext_dev->ep_id));
	ext_dev->aeq_vhca_pfvf.aeq_aee_flag =
		(u32 __iomem *)(hw_addr + EXT_AEQ_AEQC_AEE_FLAG(ext_dev->ep_id));

	writel(0, ext_dev->aeq_tail_pointer);
	//soc hmc config
	hdr = FIELD_PREP(ZXDH_AEQ_CACHE_ID, ext_dev->cache_id) |
	      FIELD_PREP(ZXDH_AEQ_AXI_ID, ext_dev->soc_tx_rx_cqp_axid) |
	      FIELD_PREP(ZXDH_AEQ_WAY_PATITION, 0);
	dma_wmb(); /* make sure WQE is populated before valid bit is set */
	writel(hdr, ext_dev->aeq_vhca_pfvf.aeq_root_axi_data);

	hdr = FIELD_PREP(ZXDH_AEQ_CACHE_ID, ext_dev->cache_id) |
	      FIELD_PREP(ZXDH_AEQ_AXI_ID, ext_dev->soc_tx_rx_cqp_axid) |
	      FIELD_PREP(ZXDH_AEQ_WAY_PATITION, 0);
	dma_wmb(); /* make sure WQE is populated before valid bit is set */
	writel(hdr, ext_dev->aeq_vhca_pfvf.aeq_leaf_axi_data);
	//soc data config
	hdr = FIELD_PREP(ZXDH_AEQ_INDICIATE_ID, ext_dev->soc_tx_rx_cqp_ind) |
	      FIELD_PREP(ZXDH_AEQ_AXI_ID, ext_dev->soc_tx_rx_cqp_axid) |
	      FIELD_PREP(ZXDH_AEQ_WAY_PATITION, 0);
	dma_wmb(); /* make sure WQE is populated before valid bit is set */
	writel(hdr, ext_dev->aeq_vhca_pfvf.aeq_wr_axi_data);
	//clear 0 after reading values during maintenance
	writel(0, ext_dev->aeq_vhca_pfvf.aeq_aee_flag);
}

void zxdh_init_hw(struct zxdh_sc_dev *dev)
{
	int i;
	u32 hdr;
	u8 __iomem *hw_addr;
	u8 __iomem *ext_hw_addr;
    struct zxdh_pci_f *rf = dev_to_rf(dev);
	struct zxdh_sc_dev *ext_dev = &rf->ext_sc_dev;

	dev->ceq_0_ok = false;
	dev->soc_tx_rx_cqp_ind = ZXDH_SOC_TXRXCQP_IND_ACC_HOST_NOT_THROUGH_SMMU;
	dev->soc_tx_rx_cqp_axid = ZXDH_SOC_TXRXCQP_AXID_DEST_EP5;
	dev->soc_rdma_io_ind = ZXDH_SOC_RDMAIO_IND_ACC_HOST_NOT_THROUGH_SMMU;

	hw_addr = dev->hw->hw_addr;

	dev->wqe_alloc_db = (u32 __iomem *)(hw_addr + C_RDMA_SQ_DBINFO_LOW_DIN(dev->ep_id));
	dev->cq_arm_db = (u32 __iomem *)(hw_addr + RDMARX_CQ_CQARM(dev->ep_id));
	dev->cqp_db = (u32 __iomem *)(hw_addr + C_RDMA_CQP_DB(dev->ep_id));

	zxdh_init_ceq_hw(dev);
	zxdh_init_aeq_hw(dev);

	if(rf->use_blue_flame_flag && (!rf->ftype))
	{
		ext_dev->ceq_0_ok = false;
		ext_dev->soc_tx_rx_cqp_ind = ZXDH_SOC_TXRXCQP_IND_ACC_HOST_NOT_THROUGH_SMMU;
		ext_dev->soc_tx_rx_cqp_axid = ZXDH_SOC_TXRXCQP_AXID_DEST_EP5;
		ext_dev->soc_rdma_io_ind = ZXDH_SOC_RDMAIO_IND_ACC_HOST_NOT_THROUGH_SMMU;

		ext_hw_addr = ext_dev->hw->ext_hw_addr;

		ext_dev->wqe_alloc_db = (u32 __iomem *)(ext_hw_addr + C_RDMA_EXT_SQ_DBINFO_LOW_DIN(ext_dev->ep_id));
		ext_dev->cqp_db = (u32 __iomem *)(ext_hw_addr + C_RDMA_EXT_CQP_DB(ext_dev->ep_id));

		for (i = 0; i < ZXDH_MAX_SHIFTS; ++i)
			ext_dev->hw_shifts[i] = icrdma_shifts[i];

		for (i = 0; i < ZXDH_MAX_MASKS; ++i)
			ext_dev->hw_masks[i] = icrdma_masks[i];

		zxdh_init_ext_aeq_hw(ext_dev);

		ext_dev->irq_ops = &zxdh_rdma_ext_irq_ops;

		ext_dev->hw_attrs.max_hw_ird = rf->iwdev->max_hw_ord;
		ext_dev->hw_attrs.max_hw_ord = rf->iwdev->max_hw_ord;
		ext_dev->hw_attrs.max_stat_inst = ICRDMA_MAX_STATS_COUNT;
		ext_dev->hw_attrs.max_stat_idx = ZXDH_HW_STAT_INDEX_MAX;

		ext_dev->hw_attrs.uk_attrs.max_hw_sq_chunk = ZXDH_MAX_QUANTA_PER_WR;
		ext_dev->hw_attrs.uk_attrs.feature_flags |=
			ZXDH_FEATURE_RTS_AE | ZXDH_FEATURE_CQ_RESIZE |
			ZXDH_FEATURE_64_BYTE_CQE; /* RC UD both set to 64 Bytes*/
	}

	dev->hw_attrs.max_hw_vf_fpm_id = ZXDH_MAX_VF_FPM_ID;
	dev->hw_attrs.first_hw_vf_fpm_id = ZXDH_FIRST_VF_FPM_ID;

	for (i = 0; i < ZXDH_MAX_SHIFTS; ++i)
		dev->hw_shifts[i] = icrdma_shifts[i];

	for (i = 0; i < ZXDH_MAX_MASKS; ++i)
		dev->hw_masks[i] = icrdma_masks[i];

	dev->srq_axi_ram.db = (u32 __iomem *)(hw_addr + C_DB_AXI_RAM(dev->ep_id));
	dev->srq_axi_ram.srql = (u32 __iomem *)(hw_addr + C_SRQL_AXI_RAM(dev->ep_id));

	dev->irq_ops = &zxdh_rdma_irq_ops;

	dev->hw_attrs.max_hw_ird = rf->iwdev->max_hw_ord;
	dev->hw_attrs.max_hw_ord = rf->iwdev->max_hw_ord;
	dev->hw_attrs.max_stat_inst = ICRDMA_MAX_STATS_COUNT;
	dev->hw_attrs.max_stat_idx = ZXDH_HW_STAT_INDEX_MAX;

	dev->hw_attrs.uk_attrs.max_hw_sq_chunk = ZXDH_MAX_QUANTA_PER_WR;
	
	if (rf->use_ext_mem_flag)
		dev->hw_attrs.uk_attrs.feature_flags |=
			ZXDH_FEATURE_RTS_AE | ZXDH_FEATURE_64_BYTE_CQE; /* RC UD both set to 64 Bytes*/
	else
		dev->hw_attrs.uk_attrs.feature_flags |=
			ZXDH_FEATURE_RTS_AE | ZXDH_FEATURE_CQ_RESIZE |
			ZXDH_FEATURE_64_BYTE_CQE; /* RC UD both set to 64 Bytes*/

    if (rf->chip_srq_base_paddr != 0 && rf->srq_mem_size != 0) {
	    hdr = FIELD_PREP(ZXDH_SRQ_DB_CACHE_ID, dev->cache_id) |
	        FIELD_PREP(ZXDH_SRQ_DB_INDICATE_ID, ZXDH_INDICATE_L2D) |
	        FIELD_PREP(ZXDH_SRQ_DB_AXI_ID, ZXDH_AXID_L2D) |
	        FIELD_PREP(ZXDH_SRQ_DB_WAY_PATION, 0);
    } else {
	    hdr = FIELD_PREP(ZXDH_SRQ_DB_CACHE_ID, dev->cache_id) |
	        FIELD_PREP(ZXDH_SRQ_DB_INDICATE_ID, dev->soc_tx_rx_cqp_ind) |
	        FIELD_PREP(ZXDH_SRQ_DB_AXI_ID, dev->soc_tx_rx_cqp_axid) |
	        FIELD_PREP(ZXDH_SRQ_DB_WAY_PATION, 0);
    }
	wmb(); /* make sure WQE is populated before valid bit is set */
	writel(hdr, dev->srq_axi_ram.db);

	hdr = FIELD_PREP(ZXDH_SRQ_DSRQL_CACHE_ID, dev->cache_id) |
	      FIELD_PREP(ZXDH_SRQ_SRQL_INDICATE_ID, dev->soc_tx_rx_cqp_ind) |
	      FIELD_PREP(ZXDH_SRQ_SRQL_AXI_ID, dev->soc_tx_rx_cqp_axid) |
	      FIELD_PREP(ZXDH_SRQ_SRQL_WAY_PATION, 0);
	wmb(); /* make sure WQE is populated before valid bit is set */
	writel(hdr, dev->srq_axi_ram.srql);

	writel(IRDMARX_RD_TIME_LIMIT_VALUE,
	       (u32 __iomem *)(hw_addr + RDMATX_RD_TIME_LIMIT(dev->ep_id)));
	writel(IRDMARX_RD_TIME_LIMIT_VALUE,
	       (u32 __iomem *)(hw_addr + RDMARX_RD_TIME_LIMIT(dev->ep_id)));
}

void zxdh_init_config_check(struct zxdh_config_check *cc, u8 traffic_class,
			    u16 qs_handle)
{
	cc->config_ok = false;
	cc->traffic_class = traffic_class;
	cc->qs_handle = qs_handle;
	cc->lfc_set = 0;
	cc->pfc_set = 0;
}

static bool zxdh_is_lfc_set(struct zxdh_config_check *cc,
			    struct zxdh_sc_vsi *vsi)
{
	u32 lfc = 1;
	u8 fn_id = vsi->dev->hmc_fn_id;

	lfc &= (rd32(vsi->dev->hw,
		     PRTMAC_HSEC_CTL_RX_PAUSE_ENABLE_0 + 4 * fn_id) >>
		8);
	lfc &= (rd32(vsi->dev->hw,
		     PRTMAC_HSEC_CTL_TX_PAUSE_ENABLE_0 + 4 * fn_id) >>
		8);
	lfc &= rd32(vsi->dev->hw,
		    PRTMAC_HSEC_CTL_RX_ENABLE_GPP_0 + 4 * vsi->dev->hmc_fn_id);

	if (lfc)
		return true;
	return false;
}

static bool zxdh_check_tc_has_pfc(struct zxdh_sc_vsi *vsi, u64 reg_offset,
				  u16 traffic_class)
{
	u32 value, pfc = 0;
	u32 i;

	value = rd32(vsi->dev->hw, reg_offset);
	for (i = 0; i < 4; i++)
		pfc |= (value >> (8 * i + traffic_class)) & 0x1;

	if (pfc)
		return true;
	return false;
}

static bool zxdh_is_pfc_set(struct zxdh_config_check *cc,
			    struct zxdh_sc_vsi *vsi)
{
	u32 pause;
	u8 fn_id = vsi->dev->hmc_fn_id;

	pause = (rd32(vsi->dev->hw,
		      PRTMAC_HSEC_CTL_RX_PAUSE_ENABLE_0 + 4 * fn_id) >>
		 cc->traffic_class) &
		BIT(0);
	pause &= (rd32(vsi->dev->hw,
		       PRTMAC_HSEC_CTL_TX_PAUSE_ENABLE_0 + 4 * fn_id) >>
		  cc->traffic_class) &
		 BIT(0);

	return zxdh_check_tc_has_pfc(vsi, GLDCB_TC2PFC, cc->traffic_class) &&
	       pause;
}

bool zxdh_is_config_ok(struct zxdh_config_check *cc, struct zxdh_sc_vsi *vsi)
{
	cc->lfc_set = zxdh_is_lfc_set(cc, vsi);
	cc->pfc_set = zxdh_is_pfc_set(cc, vsi);

	cc->config_ok = cc->lfc_set || cc->pfc_set;

	return cc->config_ok;
}

#define ZXDH_RCV_WND_NO_FC 0x1FFFC
#define ZXDH_RCV_WND_FC 0x3FFFC

#define ZXDH_CWND_NO_FC 0x20
#define ZXDH_CWND_FC 0x400

#define ZXDH_RTOMIN_NO_FC 0x5
#define ZXDH_RTOMIN_FC 0x32

#define ZXDH_ACKCREDS_NO_FC 0x02
#define ZXDH_ACKCREDS_FC 0x1E

static void zxdh_check_flow_ctrl(struct zxdh_sc_vsi *vsi, u8 user_prio,
				 u8 traffic_class)
{
#if IS_ENABLED(CONFIG_CONFIGFS_FS)
	struct zxdh_config_check *cfg_chk = &vsi->cfg_check[user_prio];
	struct zxdh_device *iwdev = vsi->back_vsi;

	if (!zxdh_is_config_ok(cfg_chk, vsi)) {
		if (!iwdev->override_rcv_wnd)
			iwdev->rcv_wnd = ZXDH_RCV_WND_NO_FC;
		if (!iwdev->override_cwnd)
			iwdev->roce_cwnd = ZXDH_CWND_NO_FC;
		if (!iwdev->override_rtomin)
			iwdev->roce_rtomin = ZXDH_RTOMIN_NO_FC;
		if (!iwdev->override_ackcreds)
			iwdev->roce_ackcreds = ZXDH_ACKCREDS_NO_FC;
#define ZXDH_READ_FENCE_RATE_NO_FC 4
		if (iwdev->roce_mode && !iwdev->override_rd_fence_rate)
			iwdev->rd_fence_rate = ZXDH_READ_FENCE_RATE_NO_FC;
		if (vsi->tc_print_warning[traffic_class]) {
			pr_info("[zxdh_rdma] INFO: Flow control is disabled for this traffic class (%d) on this vsi.\n",
				traffic_class);
			vsi->tc_print_warning[traffic_class] = false;
		}
	} else {
		if (!iwdev->override_rcv_wnd)
			iwdev->rcv_wnd = ZXDH_RCV_WND_FC;
		if (!iwdev->override_cwnd)
			iwdev->roce_cwnd = ZXDH_CWND_FC;
		if (!iwdev->override_rtomin)
			iwdev->roce_rtomin = ZXDH_RTOMIN_FC;
		if (!iwdev->override_ackcreds)
			iwdev->roce_ackcreds = ZXDH_ACKCREDS_FC;
#define ZXDH_READ_FENCE_RATE_FC 0
		if (!iwdev->override_rd_fence_rate)
			iwdev->rd_fence_rate = ZXDH_READ_FENCE_RATE_FC;
		if (vsi->tc_print_warning[traffic_class]) {
			pr_info("[zxdh_rdma] INFO: Flow control is enabled for this traffic class (%d) on this vsi.\n",
				traffic_class);
			vsi->tc_print_warning[traffic_class] = false;
		}
	}
#endif
}

void zxdh_check_fc_for_tc_update(struct zxdh_sc_vsi *vsi,
				 struct zxdh_l2params *l2params)
{
	u8 i;

	if (!vsi->dev->privileged)
		return;
	for (i = 0; i < IEEE_8021QAZ_MAX_TCS; i++)
		vsi->tc_print_warning[i] = true;

	for (i = 0; i < ZXDH_MAX_USER_PRIORITY; i++) {
		struct zxdh_config_check *cfg_chk = &vsi->cfg_check[i];
		u8 tc = l2params->up2tc[i];

		cfg_chk->traffic_class = tc;
		cfg_chk->qs_handle = vsi->qos[i].qs_handle;
		zxdh_check_flow_ctrl(vsi, i, tc);
	}
}

void zxdh_check_fc_for_qp(struct zxdh_sc_vsi *vsi, struct zxdh_sc_qp *sc_qp)
{
	u8 i;

	if (!vsi->dev->privileged)
		return;
	for (i = 0; i < ZXDH_MAX_USER_PRIORITY; i++) {
		struct zxdh_config_check *cfg_chk = &vsi->cfg_check[i];

		zxdh_init_config_check(cfg_chk, vsi->qos[i].traffic_class,
				       vsi->qos[i].qs_handle);
		if (sc_qp->qs_handle == cfg_chk->qs_handle)
			zxdh_check_flow_ctrl(vsi, i, cfg_chk->traffic_class);
	}
}
