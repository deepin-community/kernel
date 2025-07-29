/* SPDX-License-Identifier: GPL-2.0 */
#define CRC32OP_L(x, y, z)		crc32l x, y, z
#define CRC32OP_W(x, y, z)		crc32w x, y, z
#define CRC32OP_H(x, y, z)		crc32h x, y, z
#define CRC32OP_B(x, y, z)		crc32b x, y, z

	.text
	.global crc32_le
	.align 4
	.ent crc32_le
crc32_le:
	.prologue 0
#include "crc32_template.S"
	.end crc32_le

#undef CRC32OP_L
#undef CRC32OP_W
#undef CRC32OP_H
#undef CRC32OP_B
