/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Contributed by Richard Henderson <rth@tamu.edu>
 *
 * unsigned short csum_ipv6_magic(struct in6_addr *saddr,
 *		struct in6_addr *daddr, __u32 len,
 *		unsigned short proto, unsigned int csum);
 *
 * Misalignment handling (which costs 16 instructions / 8 cycles)
 * added by Ivan Kokshaysky <ink@jurassic.park.msu.ru>
 */
#include <asm/export.h>
	.globl csum_ipv6_magic
	.align 4
	.ent csum_ipv6_magic
	.frame $30, 0, $26, 0
csum_ipv6_magic:
	.prologue 0

	ldl_u	$0, 0($16)
	zapnot	$20, 15, $20
	exthl	$18, 1, $4
	ldl_u	$21, 7($16)

	extlb	$18, 1, $5
	ldl_u	$1, 8($16)
	extlb	$18, 2, $6
	ldl_u	$22, 15($16)

	extlb	$18, 3, $18
	ldl_u	$2, 0($17)
	sra	$4, 32, $4
	ldl_u	$23, 7($17)

	extll	$0, $16, $0
	ldl_u	$3, 8($17)
	exthl	$21, $16, $21
	ldl_u	$24, 15($17)

	sll	$5, 16, $5
	or	$0, $21, $0
	extll	$1, $16, $1
	addl	$20, $0, $20

	exthl	$22, $16, $22
	cmpult	$20, $0, $0
	sll	$6, 8, $6
	or	$1, $22, $1

	extll	$2, $17, $2
	or	$4, $18, $18
	exthl	$23, $17, $23
	or	$5, $6, $5

	extll	$3, $17, $3
	or	$2, $23, $2
	exthl	$24, $17, $24
	or	$18, $5, $18

	exthh	$19, 7, $7
	or	$3, $24, $3
	extlb	$19, 1, $19
	addl	$20, $1, $20

	or	$19, $7, $19
	cmpult	$20, $1, $1
	sll	$19, 48, $19

	sra	$19, 32, $19
	addl	$20, $2, $20
	cmpult	$20, $2, $2
	addl	$20, $3, $20

	cmpult	$20, $3, $3
	addl	$20, $18, $20
	cmpult	$20, $18, $18
	addl	$20, $19, $20

	cmpult	$20, $19, $19
	addl	$0, $1, $0
	addl	$2, $3, $2
	addl	$18, $19, $18

	addl	$0, $2, $0
	addl	$20, $18, $20
	addl	$0, $20, $0
	unop

	extlh	$0, 2, $2
	zapnot	$0, 3, $3
	extlh	$0, 4, $1
	addl	$2, $3, $3

	extlh	$0, 6, $0
	addl	$3, $1, $3
	addl	$0, $3, $0
	unop

	extlh	$0, 2, $1
	zapnot	$0, 3, $0
	addl	$0, $1, $0
	unop

	extlh	$0, 2, $1
	zapnot	$0, 3, $0
	addl	$0, $1, $0
	not	$0, $0

	zapnot	$0, 3, $0
	ret

	.end csum_ipv6_magic
	EXPORT_SYMBOL(csum_ipv6_magic)
