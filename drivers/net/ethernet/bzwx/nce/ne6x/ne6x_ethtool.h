/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#ifndef _NE6X_ETHTOOL_H
#define _NE6X_ETHTOOL_H

#define NE6X_STAT(_type, _name, _stat)			\
{							\
	.stat_string = _name,				\
	.sizeof_stat = sizeof_field(_type, _stat),	\
	.stat_offset = offsetof(_type, _stat)		\
}

enum ne6x_ethtool_test_id {
	NE6X_ETH_TEST_LINK,
	NE6X_ETH_TEST_LOOPBACK,
	NE6X_ETH_TEST_REG,
	NE6X_ETH_TEST_INT,
	NE6X_ETH_TEST_CHIP_TEMPERATUR,
	NE6X_ETH_TEST_BOARD_TEMPERATUR,
	NE6X_ETH_TEST_CURRENT,
	NE6X_ETH_TEST_VOLTAGE,
	NE6X_ETH_TEST_POWER,
	NE6X_ETH_TEST_I2C3,
};

void ne6x_set_ethtool_ops(struct net_device *netdev);

#endif
