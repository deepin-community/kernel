/* SPDX-License-Identifier: GPL-2.0 */
/*
 * $16: crc
 * $17: buf
 * $18: len
 */

	beq	$18, $return_crc
	srl	$18, 4, $19
	beq	$19, $check_8B

	.align 4
$loop_16B:
	ldl_a	$1, 8($17)
	ldl_a	$2, 8($17)
	CRC32OP_L($16, $1, $16)
	CRC32OP_L($16, $2, $16)
	subl	$19, 1, $19
	bne	$19, $loop_16B

$check_8B:
	and	$18, 8, $3
	beq	$3, $check_4B
	ldl_a	$1, 8($17)
	CRC32OP_L($16, $1, $16)
$check_4B:
	and	$18, 4, $3
	beq	$3, $check_2B
	ldw_a	$1, 4($17)
	CRC32OP_W($16, $1, $16)
$check_2B:
	and	$18, 2, $3
	beq	$3, $check_1B
	ldhu_a	$1, 2($17)
	CRC32OP_H($16, $1, $16)
$check_1B:
	and	$18, 1, $3
	beq	$3, $return_crc
	ldbu_a	$1, 1($17)
	CRC32OP_B($16, $1, $16)

$return_crc:
	mov	$16, $0
	ret
