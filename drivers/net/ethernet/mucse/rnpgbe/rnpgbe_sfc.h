/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#ifndef _RNPGBE_SFC_H
#define _RNPGBE_SFC_H

/* Return value */
#define HAL_OK 0
#define HAL_EINVAL (-3) /* Invalid argument */
#define HAL_ETIME (-6) /* Timer expired */
#define RSP_FLASH_HIGH_16M_OFFSET 0x1000000
#define SFC_MEM_BASE 0x28000000
#define RSP_FLASH_SIZE 0x1000000
#define CMD_WRITE_DISABLE 0x04000000
#define CMD_READ_STATUS 0x05000000
#define CMD_WRITE_ENABLE 0x06000000
#define CMD_SECTOR_ERASE 0x20000000
#define CMD_BLOCK_ERASE_64K 0xd8000000
#define SFCADDR(a) ((a) << 8)
#define CMD_CYCLE(c) (((c) & 0xff) << 0)
#define RD_DATA_CYCLE(c) (((c) & 0xff) << 8)
#define WR_DATA_CYCLE(c) (((c) & 0xff) << 0)
#define ADDR_CYCLE(c) (((c) & 0xff) << 16)

#endif /* _RNPGBE_SFC_H */
