// SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#include "tc_hmcdma.h"
#include "icrdma_hw.h"
#include "type.h"
#include "protos.h"

#define L2D_BASE_PA 0x6200900000

int host_test_dma_write32(struct zxdh_pci_f *rf)
{
	int i = 0, status = 0;
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_path_index dpath_index = {};
	struct zxdh_dma_mem ddrsrc = {};
	struct zxdh_dma_write32_date dma_data = {};
	struct zxdh_sc_cqp *cqp = rf->sc_dev.cqp;
	u64 hmcreg = 0;

	if (rf->sc_dev.hmc_use_dpu_ddr == true) {
		pr_info("[zxdh_rdma] This is use DPU DDR!!!\n");
		return -EPERM;
	}

	ddrsrc.size = 100;
	ddrsrc.va = dma_alloc_coherent(rf->hw.device, ddrsrc.size, &ddrsrc.pa,
				       GFP_KERNEL);

	if (!ddrsrc.va) {
		status = -ENOMEM;
		return status;
	}
	memset(ddrsrc.va, 0, ddrsrc.size);

	dpath_index.inter_select = ZXDH_INTERFACE_NOTCACHE;
	dpath_index.path_select = ZXDH_INDICATE_HOST_NOSMMU;
	dpath_index.obj_id = ZXDH_DMA_OBJ_ID;
	dpath_index.vhca_id = dev->vhca_id;

	dma_data.num = 4;
	for (i = 0; i < dma_data.num; i++) {
		dma_data.addrbuf[i] = ddrsrc.pa + 0x04 * i;
		dma_data.databuf[i] = 0x55 + i;
	}

	zxdh_sc_dma_write32(cqp, 0, &dpath_index, &dma_data, true);

	dpath_index.inter_select = ZXDH_INTERFACE_NOTCACHE;
	dpath_index.path_select = ZXDH_INDICATE_REGISTER;
	dpath_index.obj_id = ZXDH_REG_OBJ_ID;
	dpath_index.vhca_id = dev->vhca_id;

	dma_data.num = 4;

	hmcreg = 0x6204c00010;

	dma_data.addrbuf[0] = hmcreg;
	dma_data.databuf[0] = 0x55;

	hmcreg = 0x6204c00010 + 4096 * 1;
	dma_data.addrbuf[1] = hmcreg;
	dma_data.databuf[1] = 0x56;

	hmcreg = 0x6204c00010 + 4096 * 2;
	dma_data.addrbuf[2] = hmcreg;
	dma_data.databuf[2] = 0x57;

	hmcreg = 0x6204c00010 + 4096 * 3;
	dma_data.addrbuf[3] = hmcreg;
	dma_data.databuf[3] = 0x58;

	zxdh_sc_dma_write32(cqp, 0, &dpath_index, &dma_data, true);

	return status;
}

int host_test_dma_write64(struct zxdh_pci_f *rf)
{
	int i = 0, status = 0;
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_path_index dpath_index = {};
	struct zxdh_dma_mem ddrsrc = {}; // 外挂DDR
	struct zxdh_dma_write64_date dma_data = {};
	struct zxdh_sc_cqp *cqp = rf->sc_dev.cqp;
	u64 hmcreg = 0;

	if (rf->sc_dev.hmc_use_dpu_ddr == true) {
		pr_info("[zxdh_rdma] This is use DPU DDR!!!\n");
		return -EPERM;
	}

	ddrsrc.size = 100;
	ddrsrc.va = dma_alloc_coherent(rf->hw.device, ddrsrc.size, &ddrsrc.pa,
				       GFP_KERNEL);

	if (!ddrsrc.va) {
		status = -ENOMEM;
		return status;
	}
	memset(ddrsrc.va, 0, ddrsrc.size);

	dpath_index.inter_select = ZXDH_INTERFACE_NOTCACHE; // not pass cache
	dpath_index.path_select = ZXDH_INDICATE_HOST_NOSMMU;
	dpath_index.obj_id = ZXDH_DMA_OBJ_ID;
	dpath_index.vhca_id = dev->vhca_id;

	dma_data.num = 3;
	for (i = 0; i < dma_data.num; i++) {
		dma_data.addrbuf[i] = ddrsrc.pa + 0x08 * i;
		dma_data.databuf[i] = 0x66 + i;
	}

	zxdh_sc_dma_write64(cqp, 0, &dpath_index, &dma_data, true);

	dpath_index.inter_select = ZXDH_INTERFACE_NOTCACHE; // not pass cache
	dpath_index.path_select = ZXDH_INDICATE_REGISTER; // L2D
	dpath_index.obj_id = ZXDH_REG_OBJ_ID; // L2D
	dpath_index.vhca_id = dev->vhca_id;

	dma_data.num = 3;
	hmcreg = 0x6204c00008;

	dma_data.addrbuf[0] = hmcreg;
	dma_data.databuf[0] = 0x155;

	hmcreg = 0x6204c00008 + 4096 * 1;
	dma_data.addrbuf[1] = hmcreg;
	dma_data.databuf[1] = 0x156;

	hmcreg = 0x6204c00008 + 4096 * 2;
	dma_data.addrbuf[2] = hmcreg;
	dma_data.databuf[2] = 0x157;
	zxdh_sc_dma_write64(cqp, 0, &dpath_index, &dma_data, true);
	return status;
}

int host_test_dma_write(struct zxdh_pci_f *rf)
{
	int status = 0;
	u16 i = 0;
	u32 val = 0xff;
	u8 *addr;

	struct zxdh_dma_mem ddrsrc = {};
	struct zxdh_dma_mem ddrdest = {};

	struct zxdh_src_copy_dest src_dest = {};

	struct zxdh_path_index spath_index = {};
	struct zxdh_path_index dpath_index = {};
	struct zxdh_sc_cqp *cqp = rf->sc_dev.cqp;

	if (rf->sc_dev.hmc_use_dpu_ddr == true) {
		pr_info("[zxdh_rdma] This is use DPU DDR!!!\n");
		return -EPERM;
	}

	ddrsrc.size = 1024;
	ddrsrc.va = dma_alloc_coherent(rf->hw.device, ddrsrc.size, &ddrsrc.pa,
				       GFP_KERNEL);

	if (!ddrsrc.va) {
		status = -ENOMEM;
		return status;
	}

	memset(ddrsrc.va, 0x00, ddrsrc.size);

	ddrdest.size = 1024;
	ddrdest.va = dma_alloc_coherent(rf->hw.device, ddrdest.size,
					&ddrdest.pa, GFP_KERNEL);

	if (!ddrdest.va) {
		status = -ENOMEM;
		return status;
	}
	memset(ddrdest.va, 0, ddrdest.size);

	addr = (u8 *)(uintptr_t)ddrsrc.va;

	for (i = 0; i < 200; i++) {
		*addr = val + i;
		addr = addr + sizeof(val);
	}

	src_dest.src = ddrsrc.pa;
	src_dest.dest = ddrdest.pa;
	src_dest.len = 5 * 4;

	spath_index.inter_select = ZXDH_INTERFACE_NOTCACHE;
	spath_index.path_select = ZXDH_INDICATE_HOST_NOSMMU;
	spath_index.obj_id = ZXDH_DMA_OBJ_ID;
	spath_index.vhca_id = rf->sc_dev.vhca_id;

	if (rf->sc_dev.cache_id != 0) {
		dpath_index.inter_select = ZXDH_INTERFACE_CACHE;
		dpath_index.path_select = rf->sc_dev.cache_id;
		dpath_index.obj_id = ZXDH_DMA_OBJ_ID;
		dpath_index.vhca_id = rf->sc_dev.vhca_id;
	} else {
		dpath_index.inter_select = ZXDH_INTERFACE_NOTCACHE;
		dpath_index.path_select = ZXDH_INDICATE_HOST_NOSMMU;
		dpath_index.obj_id = ZXDH_DMA_OBJ_ID;
		dpath_index.vhca_id = rf->sc_dev.vhca_id;
	}

	zxdh_sc_dma_write(cqp, 0, &src_dest, &spath_index, &dpath_index, true);

	src_dest.src = ddrsrc.pa;
	src_dest.dest = L2D_BASE_PA;
	src_dest.len = 5 * 4;

	spath_index.inter_select = ZXDH_INTERFACE_NOTCACHE;
	spath_index.path_select = ZXDH_INDICATE_HOST_NOSMMU;
	spath_index.obj_id = ZXDH_DMA_OBJ_ID;
	spath_index.vhca_id = rf->sc_dev.vhca_id;

	dpath_index.inter_select = ZXDH_INTERFACE_NOTCACHE;
	dpath_index.path_select = ZXDH_INDICATE_L2D;
	dpath_index.obj_id = ZXDH_L2D_OBJ_ID;
	dpath_index.vhca_id = rf->sc_dev.vhca_id;
	zxdh_sc_dma_write(cqp, 0, &src_dest, &spath_index, &dpath_index, true);

	return status;
}

int host_test_dma_write_bysmmu(struct zxdh_pci_f *rf)
{
	int status = 0;
	u16 i = 0;
	u32 val = 0xff;
	u8 *addr;

	struct zxdh_dma_mem ddrsrc = {};
	struct zxdh_src_copy_dest src_dest = {};

	struct zxdh_path_index spath_index = {};
	struct zxdh_path_index dpath_index = {};
	struct zxdh_sc_cqp *cqp = rf->sc_dev.cqp;

	if (rf->sc_dev.hmc_use_dpu_ddr == true) {
		pr_info("[zxdh_rdma] This is use DPU DDR!!!\n");
		return -EPERM;
	}

	ddrsrc.size = 1024;
	ddrsrc.va = dma_alloc_coherent(rf->hw.device, ddrsrc.size, &ddrsrc.pa,
				       GFP_KERNEL);

	if (!ddrsrc.va) {
		status = -ENOMEM;
		return status;
	}
	memset(ddrsrc.va, 0x00, ddrsrc.size);

	addr = (u8 *)(uintptr_t)ddrsrc.va;

	for (i = 0; i < 200; i++) {
		*addr = val + i;
		addr = addr + sizeof(val);
	}

	src_dest.src = ddrsrc.pa;
	src_dest.dest = rf->sc_dev.hmc_info->hmc_obj[ZXDH_HMC_IW_QP].base;
	src_dest.len = 512;

	spath_index.inter_select = ZXDH_INTERFACE_NOTCACHE;
	spath_index.path_select = ZXDH_INDICATE_HOST_NOSMMU;
	spath_index.obj_id = ZXDH_DMA_OBJ_ID;
	spath_index.vhca_id = rf->sc_dev.vhca_id;

	dpath_index.inter_select = ZXDH_INTERFACE_NOTCACHE;
	dpath_index.path_select = ZXDH_INDICATE_HOST_SMMU;
	dpath_index.obj_id = ZXDH_QPC_OBJ_ID;
	dpath_index.vhca_id = rf->sc_dev.vhca_id;

	status = zxdh_sc_dma_write(cqp, 0, &src_dest, &spath_index,
				   &dpath_index, true);
	return status;
}

int zxdh_sc_dma_wr32_auto(struct zxdh_pci_f *rf)
{
	int status = 0;
	u16 i = 0, len = 0x20;
	u32 val = 0xff;
	u8 *addr;

	struct zxdh_dma_mem ddr1 = {};
	struct zxdh_dma_mem ddr2 = {};
	struct zxdh_dma_mem ddr3 = {};

	struct zxdh_src_copy_dest src_dest = {};

	if (rf->sc_dev.hmc_use_dpu_ddr == true) {
		pr_info("[zxdh_rdma] This is use DPU DDR!!!\n");
		return -EPERM;
	}

	ddr1.size = 1024;
	ddr1.va = dma_alloc_coherent(rf->hw.device, ddr1.size, &ddr1.pa,
				     GFP_KERNEL);

	if (!ddr1.va) {
		status = -ENOMEM;
		return status;
	}

	ddr2.size = 1024;
	ddr2.va = dma_alloc_coherent(rf->hw.device, ddr2.size, &ddr2.pa,
				     GFP_KERNEL);

	if (!ddr2.va) {
		status = -ENOMEM;
		return status;
	}
	memset(ddr2.va, 0x00, ddr2.size);

	ddr3.size = 1024;
	ddr3.va = dma_alloc_coherent(rf->hw.device, ddr3.size, &ddr3.pa,
				     GFP_KERNEL);

	if (!ddr3.va) {
		status = -ENOMEM;
		return status;
	}
	memset(ddr3.va, 0x00, ddr3.size);

	addr = (u8 *)(uintptr_t)ddr1.va;

	for (i = 0; i < 200; i++) {
		*addr = val + i;
		addr = addr + sizeof(val);
	}

	src_dest.src = ddr1.pa;
	src_dest.dest = ddr2.pa;
	src_dest.len = len;
	zxdh_cqp_rdma_write_cmd(&rf->sc_dev, &src_dest,
				ZXDH_INDICATE_HOST_NOSMMU,
				ZXDH_INDICATE_HOST_NOSMMU);
	src_dest.src = ddr2.pa;
	src_dest.dest = ddr3.pa;
	src_dest.len = len;
	zxdh_cqp_rdma_read_cmd(&rf->sc_dev, &src_dest,
			       ZXDH_INDICATE_HOST_NOSMMU,
			       ZXDH_INDICATE_HOST_NOSMMU);

	if (!memcmp(ddr1.va, ddr3.va, len)) {
		status = 0;
		pr_info("[zxdh_rdma] CQP Write Read is normal!!!\n");
	}
	return status;
}

int zxdh_sc_dma_w32r32_auto(struct zxdh_pci_f *rf)
{
	int status = 0;
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_path_index dpath_index = {};
	struct zxdh_dma_write32_date dma_data = {};
	u64 rarry[5], hmcreg = 0;
	struct zxdh_dam_read_bycqe rdmadata = {};

	if (rf->sc_dev.hmc_use_dpu_ddr == true) {
		pr_info("[zxdh_rdma] This is use DPU DDR!!!\n");
		return -EPERM;
	}

	rdmadata.num = 4;
	rdmadata.bitwidth = 1;
	rdmadata.valuetype = 1;
	rdmadata.addrbuf[0] = 0x6204c00010;
	rdmadata.addrbuf[1] = 0x6204c00010 + 4096 * 1;
	rdmadata.addrbuf[2] = 0x6204c00010 + 4096 * 2;
	rdmadata.addrbuf[3] = 0x6204c00010 + 4096 * 3;

	dma_data.num = 4;

	hmcreg = 0x6204c00010;

	dma_data.addrbuf[0] = hmcreg;
	dma_data.databuf[0] = 0x55;

	hmcreg = 0x6204c00010 + 4096 * 1;
	dma_data.addrbuf[1] = hmcreg;
	dma_data.databuf[1] = 0x56;

	hmcreg = 0x6204c00010 + 4096 * 2;
	dma_data.addrbuf[2] = hmcreg;
	dma_data.databuf[2] = 0x57;

	hmcreg = 0x6204c00010 + 4096 * 3;
	dma_data.addrbuf[3] = hmcreg;
	dma_data.databuf[3] = 0x58;

	dpath_index.vhca_id = dev->vhca_id;
	dpath_index.obj_id = ZXDH_REG_OBJ_ID;
	dpath_index.path_select = ZXDH_INDICATE_REGISTER;
	dpath_index.inter_select = ZXDH_INTERFACE_NOTCACHE;
	zxdh_cqp_rdma_write32_cmd(dev, &dma_data);

	zxdh_cqp_damreadbycqe_cmd(dev, &rdmadata, &dpath_index, rarry);

	if (rarry[0] == 0x55 && rarry[1] == 0x56 && rarry[2] == 0x57 &&
	    rarry[3] == 0x58) {
		pr_info("[zxdh_rdma] CQP Write32 ReadbyCqe is normal!!!\n");
		status = 0;
	}

	return status;
}
