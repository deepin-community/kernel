/*******************************************************************************

  WangXun(R) 10GbE PCI Express Virtual Function Linux Network Driver
  Copyright(c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:
  Software Team <linux.nic@trustnetic.com>
  WangXun Technology, HuaXing Times Square A507, Hangzhou, China.

*******************************************************************************/

#include "txgbe.h"
#include "txgbe_diag.h"

/* ethtool register test data */
struct txgbe_reg_test {
	u16 reg;
	u8  array_len;
	u8  test_type;
	u32 mask;
	u32 write;
};

/* In the hardware, registers are laid out either singly, in arrays
 * spaced 0x40 bytes apart, or in contiguous tables.  We assume
 * most tests take place on arrays or single registers (handled
 * as a single-element array) and special-case the tables.
 * Table tests are always pattern tests.
 *
 * We also make provision for some required setup steps by specifying
 * registers to be written without any read-back testing.
 */

#define PATTERN_TEST    1
#define SET_READ_TEST   2
#define WRITE_NO_TEST   3
#define TABLE32_TEST    4
#define TABLE64_TEST_LO 5
#define TABLE64_TEST_HI 6

/* default VF register test */
static struct txgbe_reg_test reg_test_vf[] = {
	{ TXGBE_VXRDBAL(0), 2, PATTERN_TEST, 0xFFFFFF80, 0xFFFFFF80 },
	{ TXGBE_VXRDBAH(0), 2, PATTERN_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	{ TXGBE_VXRXDCTL(0), 2, WRITE_NO_TEST, 0, TXGBE_VXRXDCTL_ENABLE },
	{ TXGBE_VXRDT(0), 2, PATTERN_TEST, 0x0000FFFF, 0x0000FFFF },
	{ TXGBE_VXRXDCTL(0), 2, WRITE_NO_TEST, 0, 0 },
	{ TXGBE_VXTDBAL(0), 2, PATTERN_TEST, 0xFFFFFF80, 0xFFFFFFFF },
	{ TXGBE_VXTDBAH(0), 2, PATTERN_TEST, 0xFFFFFFFF, 0xFFFFFFFF },
	{ 0, 0, 0, 0, 0 }
};

static int
reg_pattern_test(struct txgbe_hw *hw, u32 r, u32 m, u32 w)
{
	static const u32 _test[] = {
		0x5A5A5A5A, 0xA5A5A5A5, 0x00000000, 0xFFFFFFFF
	};
	u32 pat, val, before;

	if (TXGBE_REMOVED(hw->hw_addr)) {
		return 1;
	}
	for (pat = 0; pat < ARRAY_SIZE(_test); pat++) {
		before = rd32(hw, r);
		wr32(hw, r, _test[pat] & w);
		val = rd32(hw, r);
		if (val != (_test[pat] & w & m)) {
			HWPRINTK(DRV, ERR,
			      "pattern test reg %04X failed: got 0x%08X expected 0x%08X\n",
			      r, val, _test[pat] & w & m);
			wr32(hw, r, before);
			return 1;
		}
		wr32(hw, r, before);
	}
	return 0;
}

static int
reg_set_and_check(struct txgbe_hw *hw, u32 r, u32 m, u32 w)
{
	u32 val, before;

	if (TXGBE_REMOVED(hw->hw_addr)) {
		return 1;
	}
	before = rd32(hw, r);
	wr32(hw, r, w & m);
	val = rd32(hw, r);
	if ((w & m) != (val & m)) {
		HWPRINTK(DRV, ERR,
		      "set/check reg %04X test failed: got 0x%08X expected 0x%08X\n",
		      r, (val & m), (w & m));
		wr32(hw, r, before);
		return 1;
	}
	wr32(hw, r, before);
	return 0;
}

int txgbe_diag_reg_test(struct txgbe_hw *hw)
{
	struct txgbe_reg_test *test;
	int rc;
	u32 i;

	if (TXGBE_REMOVED(hw->hw_addr)) {
		HWPRINTK(DRV, ERR, "Adapter removed - register test blocked\n");
		return 1;
	}
	test = reg_test_vf;

	/*
	 * Perform the register test, looping through the test table
	 * until we either fail or reach the null entry.
	 */
	while (test->reg) {
		for (i = 0; i < test->array_len; i++) {
			rc = 0;
			switch (test->test_type) {
			case PATTERN_TEST:
				rc = reg_pattern_test(hw,
						      test->reg + (i * 0x40),
						      test->mask,
						      test->write);
				break;
			case SET_READ_TEST:
				rc = reg_set_and_check(hw,
						       test->reg + (i * 0x40),
						       test->mask,
						       test->write);
				break;
			case WRITE_NO_TEST:
				wr32(hw, test->reg + (i * 0x40),
						test->write);
				break;
			case TABLE32_TEST:
				rc = reg_pattern_test(hw,
						      test->reg + (i * 4),
						      test->mask,
						      test->write);
				break;
			case TABLE64_TEST_LO:
				rc = reg_pattern_test(hw,
						      test->reg + (i * 8),
						      test->mask,
						      test->write);
				break;
			case TABLE64_TEST_HI:
				rc = reg_pattern_test(hw,
						      test->reg + 4 + (i * 8),
						      test->mask,
						      test->write);
				break;
			}
			if (rc)
				return rc;
		}
		test++;
	}

	return 0;
}

