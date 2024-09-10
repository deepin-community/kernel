// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#include "rnpgbe_sfc.h"
#include "rnpgbe.h"

static inline void rsp_hal_sfc_command(u8 __iomem *hw_addr, u32 cmd)
{
	iowrite32(cmd, (hw_addr + 0x8));
	iowrite32(1, (hw_addr + 0x0));
	while (ioread32(hw_addr) != 0)
		;
}

static inline void rsp_hal_sfc_flash_write_disable(u8 __iomem *hw_addr)
{
	iowrite32(CMD_CYCLE(8), (hw_addr + 0x10));
	iowrite32(WR_DATA_CYCLE(0), (hw_addr + 0x14));

	rsp_hal_sfc_command(hw_addr, CMD_WRITE_DISABLE);
}

static int32_t rsp_hal_sfc_flash_wait_idle(u8 __iomem *hw_addr)
{
	iowrite32(CMD_CYCLE(8), (hw_addr + 0x10));
	iowrite32(RD_DATA_CYCLE(8), (hw_addr + 0x14));

	while (1) {
		rsp_hal_sfc_command(hw_addr, CMD_READ_STATUS);
		if ((ioread32(hw_addr + 0x4) & 0x1) == 0)
			break;
	}
	return HAL_OK;
}

static inline void rsp_hal_sfc_flash_write_enable(u8 __iomem *hw_addr)
{
	iowrite32(CMD_CYCLE(8), (hw_addr + 0x10));
	iowrite32(0x1f, (hw_addr + 0x18));
	iowrite32(0x100000, (hw_addr + 0x14));

	rsp_hal_sfc_command(hw_addr, CMD_WRITE_ENABLE);
}

static int rsp_hal_sfc_flash_erase_sector_internal(u8 __iomem *hw_addr,
						   u32 address)
{
	if (address >= RSP_FLASH_HIGH_16M_OFFSET)
		return HAL_EINVAL;

	if (address % 4096)
		return HAL_EINVAL;

	rsp_hal_sfc_flash_write_enable(hw_addr);

	iowrite32((CMD_CYCLE(8) | ADDR_CYCLE(24)), (hw_addr + 0x10));
	iowrite32((RD_DATA_CYCLE(0) | WR_DATA_CYCLE(0)), (hw_addr + 0x14));
	iowrite32(SFCADDR(address), (hw_addr + 0xc));
	rsp_hal_sfc_command(hw_addr, CMD_SECTOR_ERASE);
	rsp_hal_sfc_flash_wait_idle(hw_addr);
	rsp_hal_sfc_flash_write_disable(hw_addr);

	return HAL_OK;
}

int rsp_hal_sfc_flash_erase(struct rnpgbe_hw *hw, u32 size)
{
	u32 addr = SFC_MEM_BASE;
	u32 i = 0;
	u32 page_size = 0x1000;

	size = ((size + (page_size - 1)) / page_size) * page_size;

	addr = addr - SFC_MEM_BASE;

	if (size == 0)
		return HAL_EINVAL;

	if ((addr + size) > RSP_FLASH_HIGH_16M_OFFSET)
		return HAL_EINVAL;

	if (addr % page_size)
		return HAL_EINVAL;

	if (size % page_size)
		return HAL_EINVAL;

	for (i = 0; i < size; i += page_size) {
		if ((i >= 0x1f000) && (i < 0x20000))
			continue;

		rsp_hal_sfc_flash_erase_sector_internal(hw->hw_addr,
							(addr + i));
	}

	return HAL_OK;
}
