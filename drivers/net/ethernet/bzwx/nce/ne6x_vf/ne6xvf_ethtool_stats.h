/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#ifndef _NE6XVF_ETHTOOL_H
#define _NE6XVF_ETHTOOL_H

#include "ne6xvf.h"

#define NE6XVF_STAT(_type, _name, _stat)           \
{						   \
	.stat_string = _name,			   \
	.sizeof_stat = sizeof_field(_type, _stat), \
	.stat_offset = offsetof(_type, _stat)	   \
}

enum ne6xvf_ethtool_test_id {
	NE6XVF_ETH_TEST_REG = 0,
	NE6XVF_ETH_TEST_EEPROM,
	NE6XVF_ETH_TEST_INTR,
	NE6XVF_ETH_TEST_LINK,
};

#endif
