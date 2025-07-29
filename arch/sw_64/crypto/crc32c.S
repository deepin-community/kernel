/* SPDX-License-Identifier: GPL-2.0 */
#define CRC32OP_L(x, y, z)		crc32cl x, y, z
#define CRC32OP_W(x, y, z)		crc32cw x, y, z
#define CRC32OP_H(x, y, z)		crc32ch x, y, z
#define CRC32OP_B(x, y, z)		crc32cb x, y, z

	.text
	.global __crc32c_le
	.align 4
	.ent __crc32c_le
__crc32c_le:
	.prologue 0
#include "crc32_template.S"
	.end __crc32c_le

#undef CRC32OP_L
#undef CRC32OP_W
#undef CRC32OP_H
#undef CRC32OP_B
