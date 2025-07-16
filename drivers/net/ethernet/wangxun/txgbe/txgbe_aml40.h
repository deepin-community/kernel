/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2015 - 2022 Beijing WangXun Technology Co., Ltd. */

#ifndef _TXGBE_AML40_H_
#define _TXGBE_AML40_H_

enum txgbe_media_type txgbe_get_media_type_aml40(struct txgbe_hw *hw);
s32 txgbe_init_ops_aml40(struct txgbe_hw *hw);
s32 txgbe_init_phy_ops_aml40(struct txgbe_hw *hw);
#endif /* _TXGBE_AML40_H_ */
