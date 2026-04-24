// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2019 - 2022 Beijing WangXun Technology Co., Ltd. */

#include "txgbe.h"
#include "txgbe_mtd.h"

u32 txgbe_mtd_xmdio_wr(struct mtd_dev *dev_ptr,
		       u16 port,
						u16 dev,
						u16 reg,
						u16 value)
{
	u32 result = MTD_OK;

	if (!dev_ptr->mtd_write_mdio) {
		if (dev_ptr->mtd_write_mdio(dev_ptr, port, dev, reg, value) == MTD_FAIL) {
			result = MTD_FAIL;
			MTD_DBG_INFO("mtd_write_mdio 0x%04X failed to port=%d, dev=%d, reg=0x%04X\n",
				     value, port, dev, reg);
		}
	} else {
		result = MTD_FAIL;
	}

	return result;
}

u32 txgbe_mtd_hw_xmdio_rd(struct mtd_dev *dev_ptr,
			  u16 port,
							u16 dev,
							u16 reg,
							u16 *data)
{
	u32 result = MTD_OK;

	if (!dev_ptr->fmtd_read_mdio) {
		if (dev_ptr->fmtd_read_mdio(dev_ptr, port, dev, reg, data) == MTD_FAIL) {
			result = MTD_FAIL;
			MTD_DBG_INFO("fmtd_read_mdio failed from port=%d, dev=%d, reg=0x%04X\n",
				     port, dev, reg);
		}
	} else {
		result = MTD_FAIL;
	}
	return result;
}

#define MTD_CALC_MASK(field_offset, field_len, mask)	do {\
			if (((field_len) + (field_offset)) >= 16)	  \
				mask = (0 - (1 << (field_offset)));	\
			else									\
				mask = (((1 << ((field_len) + (field_offset)))) - (1 << (field_offset)));\
		} while (0)

u32 txgbe_mtd_get_phy_reg_filed(struct mtd_dev *dev_ptr,
				u16 port,
								u16 dev,
								u16 reg_addr,
								u8 field_offset,
								u8 field_length,
								u16 *data)
{
	u16 tmp_data;
	u32   ret_val;

	ret_val = txgbe_mtd_hw_xmdio_rd(dev_ptr, port, dev, reg_addr, &tmp_data);

	if (ret_val != MTD_OK) {
		MTD_DBG_ERROR("Failed to read register\n");
		return MTD_FAIL;
	}

	txgbe_mtd_get_reg_filed(tmp_data, field_offset, field_length, data);

	return MTD_OK;
}

u32 txgbe_mtd_set_phy_feild(struct mtd_dev *dev_ptr,
			    u16 port,
									u16 dev,
									u16 reg_addr,
									u8 field_offset,
									u8 field_length,
									u16 data)
{
	u16 tmp_data, new_data;
	u32   ret_val;

	ret_val = txgbe_mtd_hw_xmdio_rd(dev_ptr, port, dev, reg_addr, &tmp_data);
	if (ret_val != MTD_OK)
		return MTD_FAIL;

	txgbe_mtd_set_reg_field_word(tmp_data,
				     data, field_offset, field_length, &new_data);

	ret_val = txgbe_mtd_xmdio_wr(dev_ptr, port, dev, reg_addr, new_data);

	if (ret_val != MTD_OK)
		return MTD_FAIL;

	return MTD_OK;
}

u32 txgbe_mtd_get_reg_filed(u16 reg_data,
			    u8 field_offset,
							u8 field_length,
							u16 *data)
{
	/* Bits mask to be read */
	u16 mask;

	MTD_CALC_MASK(field_offset, field_length, mask);

	*data = (reg_data & mask) >> field_offset;

	return MTD_OK;
}

u32 txgbe_mtd_set_reg_field_word(u16 reg_data,
				 u16 bit_field_data,
								u8 field_offset,
								u8 field_length,
								u16 *data)
{
	/* Bits mask to be read */
	u16 mask;

	MTD_CALC_MASK(field_offset, field_length, mask);

	/* Set the desired bits to 0. */
	reg_data &= ~mask;
	/* Set the given data into the above reset bits.*/
	reg_data |= ((bit_field_data << field_offset) & mask);

	*data = reg_data;

	return MTD_OK;
}

u32 txgbe_mtd_wait(u32 x)
{
	msleep(x);
	return MTD_OK;
}

static u32 txgbe_mtd_check_dev_cap(struct mtd_dev *dev_ptr,
				   u16 port,
									bool *phy_has_macsec,
									bool *phy_has_copper_intf,
									bool *is_e20x0_dev)
{
	u8 major, minor, inc, test;
	u16 abilities;

	*phy_has_macsec = true;
	*phy_has_copper_intf = true;
	*is_e20x0_dev = false;

	if (txgbe_mtd_get_firmver(dev_ptr, port,
				  &major, &minor, &inc, &test) == MTD_FAIL) {
		/* firmware not running will produce this case */
		major = 0;
		minor = 0;
		inc = 0;
		test = 0;
	}

	if (major == 0 && minor == 0 && inc == 0 && test == 0) {
		u16 reg2, reg3;
		u16 index, index2;
		u16 temp;
		u16 bit16thru23[8];

		/* save these registers */
		txgbe_attempt(txgbe_mtd_hw_xmdio_rd(dev_ptr, port,
						    31, MTD_REG_SCR, &reg2));
		txgbe_attempt(txgbe_mtd_hw_xmdio_rd(dev_ptr, port,
						    31, MTD_REG_ECSR, &reg3));

		/* clear these bit indications */
		for (index = 0; index < 8; index++)
			bit16thru23[index] = 0;

		txgbe_attempt(txgbe_mtd_xmdio_wr(dev_ptr, port,
						 31, MTD_REG_CCCR9, 0x0300));
		txgbe_mtd_wait(1);
		txgbe_attempt(txgbe_mtd_xmdio_wr(dev_ptr, port,
						 31, MTD_REG_SCR, 0x0102));
		txgbe_mtd_wait(1);

		txgbe_attempt(txgbe_mtd_xmdio_wr(dev_ptr, port,
						 31, MTD_REG_ECSR, 0x06D3));
		txgbe_mtd_wait(1);
		txgbe_attempt(txgbe_mtd_xmdio_wr(dev_ptr, port,
						 31, MTD_REG_ECSR, 0x0593));
		txgbe_mtd_wait(1);
		txgbe_attempt(txgbe_mtd_xmdio_wr(dev_ptr, port,
						 31, MTD_REG_ECSR, 0x0513));
		txgbe_mtd_wait(1);

		index = 0;
		index2 = 0;
		while (index < 24) {
			txgbe_attempt(txgbe_mtd_xmdio_wr(dev_ptr, port,
							 31, MTD_REG_ECSR, 0x0413));
			txgbe_mtd_wait(1);
			txgbe_attempt(txgbe_mtd_xmdio_wr(dev_ptr, port,
							 31, MTD_REG_ECSR, 0x0513));
			txgbe_mtd_wait(1);

			if (index >= 16)
				txgbe_attempt(txgbe_mtd_hw_xmdio_rd(dev_ptr, port, 31,
								    MTD_REG_ECSR, &bit16thru23[index2++]));
			else
				txgbe_attempt(txgbe_mtd_hw_xmdio_rd(dev_ptr, port, 31,
								    MTD_REG_ECSR, &temp));

			txgbe_mtd_wait(1);
			index++;
		}

		if (((bit16thru23[0] >> 11) & 1) | ((bit16thru23[1] >> 11) & 1))
			*phy_has_macsec = false;

		if (((bit16thru23[4] >> 11) & 1) | ((bit16thru23[5] >> 11) & 1))
			*phy_has_copper_intf = false;

		if (((bit16thru23[6] >> 11) & 1) | ((bit16thru23[7] >> 11) & 1))
			*is_e20x0_dev = true;

		txgbe_attempt(txgbe_mtd_xmdio_wr(dev_ptr, port,
						 31, MTD_REG_ECSR, 0x0413));
		txgbe_mtd_wait(1);
		txgbe_attempt(txgbe_mtd_xmdio_wr(dev_ptr, port,
						 31, MTD_REG_ECSR, 0x0493));
		txgbe_mtd_wait(1);
		txgbe_attempt(txgbe_mtd_xmdio_wr(dev_ptr, port,
						 31, MTD_REG_ECSR, 0x0413));
		txgbe_mtd_wait(1);
		txgbe_attempt(txgbe_mtd_xmdio_wr(dev_ptr, port,
						 31, MTD_REG_ECSR, 0x0513));
		txgbe_mtd_wait(1);

		txgbe_attempt(txgbe_mtd_xmdio_wr(dev_ptr, port,
						 31, MTD_REG_CCCR9, 0x5440));
		txgbe_attempt(txgbe_mtd_xmdio_wr(dev_ptr, port,
						 31, MTD_REG_SCR, reg2));
		txgbe_attempt(txgbe_mtd_xmdio_wr(dev_ptr, port,
						 31, MTD_REG_ECSR, reg3));

	} else {
		/* should just read it from the firmware status register */
		txgbe_attempt(txgbe_mtd_hw_xmdio_rd(dev_ptr, port,
						    MTD_T_UNIT_PMA_PMD,
						    MTD_TUNIT_XG_EXT_STATUS, &abilities));
		if (abilities & (1 << 12))
			*phy_has_macsec = false;

		if (abilities & (1 << 13))
			*phy_has_copper_intf = false;

		if (abilities & (1 << 14))
			*is_e20x0_dev = true;
	}

	return MTD_OK;
}

static u32 txgbe_mtd_ready_after_rst(struct mtd_dev *dev_ptr, u16 port, bool *phy_ready)
{
	u16 val;

	*phy_ready = false;

	txgbe_attempt(txgbe_mtd_get_phy_reg_filed(dev_ptr, port,
						  MTD_T_UNIT_PMA_PMD, MTD_TUNIT_IEEE_PMA_CTRL1,
		15, 1, &val));

	if (val)
		*phy_ready = false;
	else
		*phy_ready = true;

	return MTD_OK;
}

u32 txgbe_mtd_sw_rst(struct mtd_dev *dev_ptr,
		     u16 port,
						u16 timeout)
{
	u16 counter;
	bool phy_ready;
	/* bit self clears when done */
	txgbe_attempt(txgbe_mtd_set_phy_feild(dev_ptr, port,
					      MTD_T_UNIT_PMA_PMD, MTD_TUNIT_IEEE_PMA_CTRL1,
		15, 1, 1));

	if (timeout) {
		counter = 0;
		txgbe_attempt(txgbe_mtd_ready_after_rst(dev_ptr,
							port, &phy_ready));
		while (!phy_ready && counter <= timeout) {
			txgbe_attempt(txgbe_mtd_wait(1));
			txgbe_attempt(txgbe_mtd_ready_after_rst(dev_ptr,
								port, &phy_ready));
			counter++;
		}

		if (counter < timeout)
			return MTD_OK;
		else
			return MTD_FAIL;
	} else {
		return MTD_OK;
	}
}

static u32 txgbe_mtd_ready_after_hw_rst(struct mtd_dev *dev_ptr, u16 port, bool *phy_ready)
{
	u16 val;

	*phy_ready = false;

	txgbe_attempt(txgbe_mtd_get_phy_reg_filed(dev_ptr, port,
						  MTD_C_UNIT_GENERAL, MTD_CUNIT_PORT_CTRL,
		14, 1, &val));

	if (val)
		*phy_ready = false;
	else
		*phy_ready = true;

	return MTD_OK;
}

u32 txgbe_mtd_hw_rst(struct mtd_dev *dev_ptr,
		     u16 port,
						u16 timeout)
{
	u16 counter;
	bool phy_ready;

	/* bit self clears when done */
	txgbe_attempt(txgbe_mtd_set_phy_feild(dev_ptr, port,
					      MTD_C_UNIT_GENERAL, MTD_CUNIT_PORT_CTRL,
		14, 1, 1));

	if (timeout) {
		counter = 0;
		txgbe_attempt(txgbe_mtd_ready_after_hw_rst(dev_ptr, port, &phy_ready));
		while (!phy_ready && counter <= timeout) {
			txgbe_attempt(txgbe_mtd_wait(1));
			txgbe_attempt(txgbe_mtd_ready_after_hw_rst(dev_ptr, port, &phy_ready));
			counter++;
		}
		if (counter < timeout)
			return MTD_OK;
		else
			return MTD_FAIL; /* timed out without becoming ready */
	} else {
		return MTD_OK;
	}
}

u32 txgbe_mtd_enable_speeds(struct mtd_dev *dev_ptr,
			    u16 port,
					u16 speed_bits,
					bool an_restart)
{
	bool speed_forced;
	u16 dummy;
	u16 temp_reg_value;

	if (speed_bits & MTD_FORCED_SPEEDS_BIT_MASK) {
		/* tried to force the speed, this function is for autonegotiation control */
		return MTD_FAIL;
	}

	if (MTD_IS_X32X0_BASE(dev_ptr->device_id) && ((speed_bits & MTD_SPEED_2P5GIG_FD) ||
						      (speed_bits & MTD_SPEED_5GIG_FD))) {
		return MTD_FAIL; /* tried to advertise 2.5G/5G on a 88X32X0 chipset */
	}

	if (MTD_IS_X33X0_BASE(dev_ptr->device_id)) {
		const u16 chip_rev = (dev_ptr->device_id & 0xf); /* get the chip revision */

		if (chip_rev == 9 || chip_rev == 5 || chip_rev == 1 ||
		    chip_rev == 8 || chip_rev == 4 || chip_rev == 0)
			return MTD_FAIL;
	}

	/* Enable AN and set speed back to power-on default in case previously forced
	 *Only do it if forced, to avoid an extra/unnecessary soft reset
	 */
	txgbe_attempt(txgbe_mtd_get_forced_speed(dev_ptr, port,
						 &speed_forced, &dummy));
	if (speed_forced)
		txgbe_attempt(txgbe_mtd_undo_forced_speed(dev_ptr, port, false));

	if (speed_bits == MTD_ADV_NONE) {
		txgbe_attempt(txgbe_mtd_set_phy_feild(dev_ptr, port, 7, 0x0010,
						      MTD_7_0010_SPEED_BIT_POS,
							     MTD_7_0010_SPEED_BIT_LENGTH,
				0));

		/* Take care of speed bits in 7.8000 (1000BASE-T speed bits) */
		txgbe_attempt(txgbe_mtd_set_phy_feild(dev_ptr, port, 7, 0x8000,
						      MTD_7_8000_SPEED_BIT_POS,
							     MTD_7_8000_SPEED_BIT_LENGTH,
				0));

		/* Now take care of bit in 7.0020 (10GBASE-T) */
		txgbe_attempt(txgbe_mtd_set_phy_feild(dev_ptr, port, 7, 0x0020,
						      MTD_7_0020_SPEED_BIT_POS,
				MTD_7_0020_SPEED_BIT_LENGTH, 0));

		if (MTD_IS_X33X0_BASE(dev_ptr->device_id)) {
			/* Now take care of bits in 7.0020 (2.5G, 5G speed bits) */
			txgbe_attempt(txgbe_mtd_set_phy_feild(dev_ptr, port,
							      7, 0x0020,
								     MTD_7_0020_SPEED_BIT_POS2,
					 MTD_7_0020_SPEED_BIT_LENGTH2, 0));
		}
	} else {
		/* Take care of bits in 7.0010 (advertisement register, 10BT and 100BT bits) */
		txgbe_attempt(txgbe_mtd_set_phy_feild(dev_ptr, port, 7, 0x0010,
						      MTD_7_0010_SPEED_BIT_POS,
							     MTD_7_0010_SPEED_BIT_LENGTH,
				(speed_bits & MTD_LOWER_BITS_MASK)));

		/* Take care of speed bits in 7.8000 (1000BASE-T speed bits) */
		txgbe_attempt(txgbe_mtd_set_phy_feild(dev_ptr, port, 7, 0x8000,
						      MTD_7_8000_SPEED_BIT_POS,
							     MTD_7_8000_SPEED_BIT_LENGTH,
				MTD_GET_1000BT_BITS(speed_bits)));

		/* Now take care of bits in 7.0020 (10GBASE-T first) */
		txgbe_attempt(txgbe_mtd_hw_xmdio_rd(dev_ptr, port,
						    7, 0x0020, &temp_reg_value));
		txgbe_attempt(txgbe_mtd_set_reg_field_word(temp_reg_value,
							   MTD_GET_10GBT_BIT(speed_bits),
				MTD_7_0020_SPEED_BIT_POS,
				MTD_7_0020_SPEED_BIT_LENGTH,
						&temp_reg_value));

		if (MTD_IS_X33X0_BASE(dev_ptr->device_id)) {
			/* Now take care of 2.5G bit in 7.0020 */
			txgbe_attempt(txgbe_mtd_set_reg_field_word(temp_reg_value,
								   MTD_GET_2P5GBT_BIT(speed_bits),
					7, 1,
					&temp_reg_value));

			/* Now take care of 5G bit in 7.0020 */
			txgbe_attempt(txgbe_mtd_set_reg_field_word(temp_reg_value,
								   MTD_GET_5GBT_BIT(speed_bits),
					8, 1,
					&temp_reg_value));
		}

		/* Now write result back to 7.0020 */
		txgbe_attempt(txgbe_mtd_xmdio_wr(dev_ptr, port, 7, 0x0020, temp_reg_value));

		if (MTD_GET_10GBT_BIT(speed_bits) ||
		    MTD_GET_2P5GBT_BIT(speed_bits) ||
			MTD_GET_5GBT_BIT(speed_bits))
			/* Set XNP on if any bit that required it was set */
			txgbe_attempt(txgbe_mtd_set_phy_feild(dev_ptr, port,
							      7, 0, 13, 1, 1));
	}

	if (an_restart) {
		return ((u32)(txgbe_mtd_autoneg_enable(dev_ptr, port) ||
			      txgbe_mtd_autoneg_restart(dev_ptr, port)));
	}

	return MTD_OK;
}

u32 txgbe_mtd_undo_forced_speed(struct mtd_dev *dev_ptr,
				u16 port,
								bool an_restart)
{
	txgbe_attempt(txgbe_mtd_set_phy_feild(dev_ptr, port,
					      MTD_T_UNIT_PMA_PMD, MTD_TUNIT_IEEE_PMA_CTRL1,
		13, 1, 1));
	txgbe_attempt(txgbe_mtd_set_phy_feild(dev_ptr, port,
					      MTD_T_UNIT_PMA_PMD, MTD_TUNIT_IEEE_PMA_CTRL1,
		6, 1, 1));

	/* when speed bits are changed, T unit sw reset is required, wait until phy is ready */
	txgbe_attempt(txgbe_mtd_sw_rst(dev_ptr, port, 1000));

	if (an_restart) {
		return ((u32)(txgbe_mtd_autoneg_enable(dev_ptr, port) ||
			      txgbe_mtd_autoneg_restart(dev_ptr, port)));
	}

	return MTD_OK;
}

u32 txgbe_mtd_get_forced_speed(struct mtd_dev *dev_ptr,
			       u16 port,
								bool *speed_is_forced,
								u16 *force_speed)
{
	u16 val, bit0, bit1, force_speed_bits, duplex_bit;
	bool an_disabled;

	*speed_is_forced = false;
	*force_speed = MTD_ADV_NONE;

	/* check if 7.0.12 is 0 or 1 (disabled or enabled) */
	txgbe_attempt(txgbe_mtd_get_phy_reg_filed(dev_ptr, port,
						  7, 0, 12, 1, &val));

	(val) ? (an_disabled = false) : (an_disabled = true);

	if (an_disabled) {
		txgbe_attempt(txgbe_mtd_get_phy_reg_filed(dev_ptr, port,
							  MTD_T_UNIT_PMA_PMD, MTD_TUNIT_IEEE_PMA_CTRL1,
			6, 1, &bit0));
		txgbe_attempt(txgbe_mtd_get_phy_reg_filed(dev_ptr, port,
							  MTD_T_UNIT_PMA_PMD, MTD_TUNIT_IEEE_PMA_CTRL1,
			13, 1, &bit1));

		/* now read the duplex bit setting */
		txgbe_attempt(txgbe_mtd_get_phy_reg_filed(dev_ptr, port,
							  7, 0x8000,
							  4, 1, &duplex_bit));

		force_speed_bits = 0;
		force_speed_bits = bit0 | (bit1 << 1);

		if (force_speed_bits == 0) {
			/* it's set to 10BT */
			if (duplex_bit) {
				*speed_is_forced = true;
				*force_speed = MTD_SPEED_10M_FD_AN_DIS;
			} else {
				*speed_is_forced = true;
				*force_speed = MTD_SPEED_10M_HD_AN_DIS;
			}
		} else if (force_speed_bits == 2) {
			/* it's set to 100BT */
			if (duplex_bit) {
				*speed_is_forced = true;
				*force_speed = MTD_SPEED_100M_FD_AN_DIS;
			} else {
				*speed_is_forced = true;
				*force_speed = MTD_SPEED_100M_HD_AN_DIS;
			}
		}
		/* else it's set to 1000BT or 10GBT which require AN to work */
	}

	return MTD_OK;
}

u32 txgbe_mtd_autoneg_restart(struct mtd_dev *dev_ptr, u16 port)
{
	/* set 7.0.9, restart AN */
	return (txgbe_mtd_set_phy_feild(dev_ptr, port, 7, 0,
					9, 1, 1));
}

u32 txgbe_mtd_autoneg_enable(struct mtd_dev *dev_ptr, u16 port)
{
	/* set 7.0.12=1, enable AN */
	return (txgbe_mtd_set_phy_feild(dev_ptr, port, 7, 0,
					12, 1, 1));
}

u32 txgbe_mtd_autoneg_done(struct mtd_dev *dev_ptr, u16 port, bool *an_speed_res_done)
{
	u16 val;

	/* read speed/duplex resolution done bit in 3.8008 bit 11 */
	if (txgbe_mtd_get_phy_reg_filed(dev_ptr, port,
					3, 0x8008,
					11, 1, &val) == MTD_FAIL) {
		*an_speed_res_done = false;
		return MTD_FAIL;
	}

	(val) ? (*an_speed_res_done = true) : (*an_speed_res_done = false);

	return MTD_OK;
}

u32 txgbe_mtd_get_autoneg_res(struct mtd_dev *dev_ptr, u16 port, u16 *speed_resolution)
{
	u16 val, speed, speed2, duplex;
	bool res_done;

	*speed_resolution = MTD_ADV_NONE;

	/* check if AN is enabled */
	txgbe_attempt(txgbe_mtd_get_phy_reg_filed(dev_ptr, port,
						  7, 0, 12, 1, &val));

	if (val) {
		/* an is enabled, check if speed is resolved */
		txgbe_attempt(txgbe_mtd_autoneg_done(dev_ptr, port, &res_done));

		if (res_done) {
			txgbe_attempt(txgbe_mtd_get_phy_reg_filed(dev_ptr, port,
								  3, 0x8008, 14, 2, &speed));

			txgbe_attempt(txgbe_mtd_get_phy_reg_filed(dev_ptr, port,
								  3, 0x8008, 13, 1, &duplex));

			switch (speed) {
			case MTD_CU_SPEED_10_MBPS:
				if (duplex)
					*speed_resolution = MTD_SPEED_10M_FD;
				else
					*speed_resolution = MTD_SPEED_10M_HD;
				break;
			case MTD_CU_SPEED_100_MBPS:
				if (duplex)
					*speed_resolution = MTD_SPEED_100M_FD;
				else
					*speed_resolution = MTD_SPEED_100M_HD;
				break;
			case MTD_CU_SPEED_1000_MBPS:
				if (duplex)
					*speed_resolution = MTD_SPEED_1GIG_FD;
				else
					*speed_resolution = MTD_SPEED_1GIG_HD;
				break;
			case MTD_CU_SPEED_10_GBPS: /* also MTD_CU_SPEED_NBT */
				if (MTD_IS_X32X0_BASE(dev_ptr->device_id)) {
					*speed_resolution = MTD_SPEED_10GIG_FD;
				} else {
					txgbe_attempt(txgbe_mtd_get_phy_reg_filed(dev_ptr, port,
										  3, 0x8008,
							2, 2, &speed2));

					switch (speed2) {
					case MTD_CU_SPEED_NBT_10G:
						*speed_resolution = MTD_SPEED_10GIG_FD;
						break;

					case MTD_CU_SPEED_NBT_5G:
						*speed_resolution = MTD_SPEED_5GIG_FD;
						break;

					case MTD_CU_SPEED_NBT_2P5G:
						*speed_resolution = MTD_SPEED_2P5GIG_FD;
						break;

					default:
						/* this is an error */
						return MTD_FAIL;
					}
				}
				break;
			default:
				/* this is an error */
				return MTD_FAIL;
			}
		}
	}

	return MTD_OK;
}

/****************************************************************************/
u32 txgbe_mtd_is_baset_up(struct mtd_dev *dev_ptr, u16 port,
			  u16 *speed,
							bool *link_up)
{
	bool speed_is_forced;
	u16 force_speed, cu_speed, cu_link_status;

	*link_up = false;
	*speed = MTD_ADV_NONE;

	/* first check if speed is forced to one of the speeds not requiring AN to train */
	txgbe_attempt(txgbe_mtd_get_forced_speed(dev_ptr, port, &speed_is_forced, &force_speed));

	if (speed_is_forced) {
		/* check if the link is up at the speed it's forced to */
		txgbe_attempt(txgbe_mtd_get_phy_reg_filed(dev_ptr, port,
							  3, 0x8008, 14,
			2, &cu_speed));
		txgbe_attempt(txgbe_mtd_get_phy_reg_filed(dev_ptr, port,
							  3, 0x8008, 10,
			1, &cu_link_status));

		switch (force_speed) {
		case MTD_SPEED_10M_HD_AN_DIS:
		case MTD_SPEED_10M_FD_AN_DIS:
		/* might want to add checking the duplex to make sure there
		 * is no duplex mismatch
		 */
			if (cu_speed == MTD_CU_SPEED_10_MBPS)
				*speed = force_speed;
			else
				*speed = MTD_SPEED_MISMATCH;
			if (cu_link_status)
				*link_up = true;

			break;

		case MTD_SPEED_100M_HD_AN_DIS:
		case MTD_SPEED_100M_FD_AN_DIS:
		/* might want to add checking the duplex to make sure there
		 * is no duplex mismatch
		 */
			if (cu_speed == MTD_CU_SPEED_100_MBPS)
				*speed = force_speed;
			else
				*speed = MTD_SPEED_MISMATCH;

			if (cu_link_status)
				*link_up = true;
			break;

		default:
			return MTD_FAIL;
		}
	} else {
		/* must be going through AN */
		txgbe_attempt(txgbe_mtd_get_autoneg_res(dev_ptr, port, speed));

		if (*speed != MTD_ADV_NONE) {
			/* check if the link is up at the speed it's AN to */
			txgbe_attempt(txgbe_mtd_get_phy_reg_filed(dev_ptr, port,
								  3, 0x8008, 10,
				1, &cu_link_status));

			switch (*speed) {
			case MTD_SPEED_10M_HD:
			case MTD_SPEED_10M_FD:
			case MTD_SPEED_100M_HD:
			case MTD_SPEED_100M_FD:
			case MTD_SPEED_1GIG_HD:
			case MTD_SPEED_1GIG_FD:
			case MTD_SPEED_10GIG_FD:
			case MTD_SPEED_2P5GIG_FD:
			case MTD_SPEED_5GIG_FD:
				if (cu_link_status)
					*link_up = true;
				break;
			default:
				return MTD_FAIL;
			}
		}
		/* else link is down, and AN is in progress, */
	}

	if (*speed == MTD_SPEED_MISMATCH)
		return MTD_FAIL;
	else
		return MTD_OK;
}

u32 txgbe_mtd_set_pause_adver(struct mtd_dev *dev_ptr, u16 port,
			      u32 pause_type,
								bool an_restart)
{
	/* sets/clears bits 11, 10 (A6,A5 in the tech bit field of 7.16) */
	txgbe_attempt(txgbe_mtd_set_phy_feild(dev_ptr, port, 7, 0x0010,
					      10, 2, (u16)pause_type));

	if (an_restart) {
		return ((u32)(txgbe_mtd_autoneg_enable(dev_ptr, port) ||
			      txgbe_mtd_autoneg_restart(dev_ptr, port)));
	}

	return MTD_OK;
}

static u32 txgbe_mtd_autoneg_is_cmplt(struct mtd_dev *dev_ptr, u16 port, bool *an_status_ready)
{
	u16 val;

	/* read an completed, 7.1.5 bit */
	if (txgbe_mtd_get_phy_reg_filed(dev_ptr, port,
					7, 1, 5,
			1, &val) == MTD_FAIL) {
		*an_status_ready = false;
		return MTD_FAIL;
	}

	(val) ? (*an_status_ready = true) : (*an_status_ready = false);

	return MTD_OK;
}

u32 txgbe_mtd_get_lp_adver_pause(struct mtd_dev *dev_ptr,
				 u16 port,
									u8 *pause_bits)
{
	u16 val;
	bool an_status_ready;

	/* Make sure AN is complete */
	txgbe_attempt(txgbe_mtd_autoneg_is_cmplt(dev_ptr, port,
						 &an_status_ready));

	if (!an_status_ready) {
		*pause_bits = MTD_CLEAR_PAUSE;
		return MTD_FAIL;
	}

	/* get bits 11, 10 (A6,A5 in the tech bit field of 7.19) */
	if (txgbe_mtd_get_phy_reg_filed(dev_ptr, port, 7, 19,
					10, 2, &val) == MTD_FAIL) {
		*pause_bits = MTD_CLEAR_PAUSE;
		return MTD_FAIL;
	}

	*pause_bits = (u8)val;

	return MTD_OK;
}

u32 txgbe_mtd_get_firmver(struct mtd_dev *dev_ptr,
			  u16 port,
							u8 *major,
							u8 *minor,
							u8 *inc,
							u8 *test)
{
	u16 reg_49169, reg_49170;

	txgbe_attempt(txgbe_mtd_hw_xmdio_rd(dev_ptr, port, 1, 49169, &reg_49169));

	*major = (reg_49169 & 0xFF00) >> 8;
	*minor = (reg_49169 & 0x00FF);

	txgbe_attempt(txgbe_mtd_hw_xmdio_rd(dev_ptr, port, 1, 49170, &reg_49170));

	*inc = (reg_49170 & 0xFF00) >> 8;
	*test = (reg_49170 & 0x00FF);

	/* firmware is not running if all 0's */
	if (!(*major || *minor || *inc || *test))
		return MTD_FAIL;

	return MTD_OK;
}

u32 txgbe_mtd_get_phy_revision(struct mtd_dev *dev_ptr,
			       u16 port,
								enum txgbe_mtd_dev_id *phy_rev,
								u8 *num_ports,
								u8 *this_port)
{
	u16 temp = 0, try_counter, temp2, base_type, reported_hw_rev;
	u16 revision = 0, numports, thisport, ready_bit, fw_numports, fw_thisport;
	bool register_exists, reg_ready, has_macsec, has_copper, is_e20x0_dev;
	u8 major, minor, inc, test;

	*phy_rev = MTD_REV_UNKNOWN;
	*num_ports = 0;
	*this_port = 0;

	/* first check base type of device, get reported rev and port info */
	txgbe_attempt(txgbe_mtd_hw_xmdio_rd(dev_ptr, port, 3, 0xD00D, &temp));
	base_type = ((temp & 0xFC00) >> 6);
	reported_hw_rev = (temp & 0x000F);
	numports = ((temp & 0x0380) >> 7) + 1;
	thisport = ((temp & 0x0070) >> 4);

	/* find out if device has macsec/ptp, copper unit or is an E20X0-type device */
	txgbe_attempt(txgbe_mtd_check_dev_cap(dev_ptr, port,
					      &has_macsec, &has_copper, &is_e20x0_dev));

	/* check if internal processor firmware is up and running, and if so, easier to get info */
	if (txgbe_mtd_get_firmver(dev_ptr, port,
				  &major, &minor, &inc, &test) == MTD_FAIL) {
		major = 0;
		minor = 0;
		inc = 0;
		test = 0;
	}

	if (major == 0 && minor == 0 && inc == 0 && test == 0) {
		/* no firmware running, have to verify device revision */
		if (MTD_IS_X32X0_BASE(base_type)) {
			/* A0 and Z2 report the same revision, need to check which is which */
			if (reported_hw_rev == 1) {
				/* need to figure out if it's A0 or Z2 */
				/* remove internal reset */
				txgbe_attempt(txgbe_mtd_set_phy_feild(dev_ptr, port,
								      3, 0xD801, 5, 1, 1));

				/* wait until it's ready */
				reg_ready = false;
				try_counter = 0;
				while (!reg_ready && try_counter++ < 10) {
					txgbe_attempt(txgbe_mtd_wait(1));
					txgbe_attempt(txgbe_mtd_get_phy_reg_filed(dev_ptr, port,
										  3, 0xD007, 6,
						1, &ready_bit));
					if (ready_bit == 1)
						reg_ready = true;
				}

				if (!reg_ready) {
					/* timed out, can't tell for sure what rev this is */
					*num_ports = 0;
					*this_port = 0;
					*phy_rev = MTD_REV_UNKNOWN;
					return MTD_FAIL;
				}

				/* perform test */
				register_exists = false;
				txgbe_attempt(txgbe_mtd_hw_xmdio_rd(dev_ptr,
								    port, 3, 0x8EC6, &temp));
				txgbe_attempt(txgbe_mtd_xmdio_wr(dev_ptr, port,
								 3, 0x8EC6, 0xA5A5));
				txgbe_attempt(txgbe_mtd_hw_xmdio_rd(dev_ptr, port,
								    3, 0x8EC6, &temp2));

				/* put back internal reset */
				txgbe_attempt(txgbe_mtd_set_phy_feild(dev_ptr, port,
								      3, 0xD801,
									     5, 1, 0));

				if (temp == 0 && temp2 == 0xA5A5)
					register_exists = true;

				if (register_exists)
					revision = 2; /* this is actually QA0 */
				else
					revision = reported_hw_rev; /* this is a QZ2 */
			} else {
				/* it's not A0 or Z2, use what's reported by the hardware */
				revision = reported_hw_rev;
			}
		} else if (MTD_IS_X33X0_BASE(base_type)) {
			/* all 33X0 devices report correct revision */
			revision = reported_hw_rev;
		}

		/* have to use what's reported by the hardware */
		*num_ports = (u8)numports;
		*this_port = (u8)thisport;
	} else {
		/* there is firmware loaded/running in internal processor */
		/* can get device revision reported by firmware */
		txgbe_attempt(txgbe_mtd_hw_xmdio_rd(dev_ptr, port,
						    MTD_T_UNIT_PMA_PMD,
							MTD_TUNIT_PHY_REV_INFO_REG,
							&temp));
		txgbe_attempt(txgbe_mtd_get_reg_filed(temp,
						      0, 4, &revision));
		txgbe_attempt(txgbe_mtd_get_reg_filed(temp,
						      4, 3, &fw_numports));
		txgbe_attempt(txgbe_mtd_get_reg_filed(temp,
						      7, 3, &fw_thisport));
		if (fw_numports == numports && fw_thisport == thisport) {
			*num_ports = (u8)numports;
			*this_port = (u8)thisport;
		} else {
			*phy_rev = MTD_REV_UNKNOWN;
			*num_ports = 0;
			*this_port = 0;
			return MTD_FAIL; /* firmware and hardware are reporting different values */
		}
	}

	/* now have correct information to build up the enum txgbe_mtd_dev_id */
	if (MTD_IS_X32X0_BASE(base_type)) {
		temp =  MTD_X32X0_BASE;
	} else if (MTD_IS_X33X0_BASE(base_type)) {
		temp = MTD_X33X0_BASE;
	} else {
		*phy_rev = MTD_REV_UNKNOWN;
		*num_ports = 0;
		*this_port = 0;
		return MTD_FAIL;
	}

	if (has_macsec)
		temp |= MTD_MACSEC_CAPABLE;

	if (has_copper)
		temp |= MTD_COPPER_CAPABLE;

	if (MTD_IS_X33X0_BASE(base_type) && is_e20x0_dev)
		temp |= MTD_E20X0_DEVICE;

	temp |= (revision & 0xF);

	*phy_rev = (enum txgbe_mtd_dev_id)temp;

	/* make sure we got a good one */
	if (txgbe_mtd_phy_rev_vaild(*phy_rev) == MTD_OK)
		return MTD_OK;
	else
		return MTD_FAIL;
}

u32 txgbe_mtd_phy_rev_vaild(enum txgbe_mtd_dev_id phy_rev)
{
	switch (phy_rev) {
	/* list must match enum txgbe_mtd_dev_id */
	case MTD_REV_3240P_Z2:
	case MTD_REV_3240P_A0:
	case MTD_REV_3240P_A1:
	case MTD_REV_3220P_Z2:
	case MTD_REV_3220P_A0:

	case MTD_REV_3240_Z2:
	case MTD_REV_3240_A0:
	case MTD_REV_3240_A1:
	case MTD_REV_3220_Z2:
	case MTD_REV_3220_A0:

	case MTD_REV_3310P_A0:
	case MTD_REV_3320P_A0:
	case MTD_REV_3340P_A0:
	case MTD_REV_3310_A0:
	case MTD_REV_3320_A0:
	case MTD_REV_3340_A0:

	case MTD_REV_E2010P_A0:
	case MTD_REV_E2020P_A0:
	case MTD_REV_E2040P_A0:
	case MTD_REV_E2010_A0:
	case MTD_REV_E2020_A0:
	case MTD_REV_E2040_A0:

	case MTD_REV_2340P_A1:
	case MTD_REV_2320P_A0:
	case MTD_REV_2340_A1:
	case MTD_REV_2320_A0:
		return MTD_OK;
	/* unsupported PHYs */
	case MTD_REV_3310P_Z1:
	case MTD_REV_3320P_Z1:
	case MTD_REV_3340P_Z1:
	case MTD_REV_3310_Z1:
	case MTD_REV_3320_Z1:
	case MTD_REV_3340_Z1:
	case MTD_REV_3310P_Z2:
	case MTD_REV_3320P_Z2:
	case MTD_REV_3340P_Z2:
	case MTD_REV_3310_Z2:
	case MTD_REV_3320_Z2:
	case MTD_REV_3340_Z2:
	case MTD_REV_E2010P_Z2:
	case MTD_REV_E2020P_Z2:
	case MTD_REV_E2040P_Z2:
	case MTD_REV_E2010_Z2:
	case MTD_REV_E2020_Z2:
	case MTD_REV_E2040_Z2:
	default:
		return MTD_FAIL; /* is either MTD_REV_UNKNOWN or not in the above list */
	}
}

/* mtdCunit.c */
static u32 txgbe_mtd_cunit_sw_rst(struct mtd_dev *dev_ptr, u16 port)
{
	return txgbe_mtd_set_phy_feild(dev_ptr, port,
		MTD_C_UNIT_GENERAL, MTD_CUNIT_PORT_CTRL,
		15, 1, 1);
}

/* mtdHxunit.c */
static u32 txgbe_mtd_rerun_serdes_autoneg_init_automode(struct mtd_dev *dev_ptr, u16 port)
{
	u16 temp, temp2, temp3;
	u16 wait_counter;

	txgbe_attempt(txgbe_mtd_hw_xmdio_rd(dev_ptr, port,
					    MTD_T_UNIT_AN, MTD_SERDES_CTRL_STATUS, &temp));

	txgbe_attempt(txgbe_mtd_set_reg_field_word(temp,
						   3, 14, 2, &temp2));

	txgbe_attempt(txgbe_mtd_xmdio_wr(dev_ptr, port,
					 MTD_T_UNIT_AN, MTD_SERDES_CTRL_STATUS, temp2));

	/* wait for it to be done */
	wait_counter = 0;
	txgbe_attempt(txgbe_mtd_hw_xmdio_rd(dev_ptr, port,
					    MTD_T_UNIT_AN, MTD_SERDES_CTRL_STATUS, &temp3));
	while ((temp3 & 0x8000) && (wait_counter < 100)) {
		txgbe_attempt(txgbe_mtd_wait(1));
		txgbe_attempt(txgbe_mtd_hw_xmdio_rd(dev_ptr, port,
						    MTD_T_UNIT_AN, MTD_SERDES_CTRL_STATUS, &temp3));
		wait_counter++;
	}

	if (wait_counter >= 100)
		return MTD_FAIL; /* execute timed out */

	return MTD_OK;
}

u32 txgbe_mtd_set_mac_intf_ctrl(struct mtd_dev *dev_ptr,
				u16 port,
									u16 mac_type,
									bool mac_powerdown,
									u16 mac_snoop_sel,
									u16 mac_active_lane_sel,
									u16 mac_link_down_speed,
									u16 mac_max_speed,
									bool do_sw_rst,
									bool rerun_serdes_init)
{
	u16 cunit_port_ctrl, cunit_mode_cfg;

	/* do range checking on parameters */
	if (mac_type > MTD_MAC_LEAVE_UNCHANGED)
		return MTD_FAIL;

	if (mac_snoop_sel > MTD_MAC_SNOOP_LEAVE_UNCHANGED ||
	    mac_snoop_sel == 1)
		return MTD_FAIL;

	if (mac_active_lane_sel > 1)
		return MTD_FAIL;

	if (mac_link_down_speed > MTD_MAC_SPEED_LEAVE_UNCHANGED)
		return MTD_FAIL;

	if (!(mac_max_speed == MTD_MAX_MAC_SPEED_10G ||
	      mac_max_speed == MTD_MAX_MAC_SPEED_5G ||
			mac_max_speed == MTD_MAX_MAC_SPEED_2P5G ||
			mac_max_speed == MTD_MAX_MAC_SPEED_LEAVE_UNCHANGED ||
			mac_max_speed == MTD_MAX_MAC_SPEED_NOT_APPLICABLE))
		return MTD_FAIL;

	txgbe_attempt(txgbe_mtd_hw_xmdio_rd(dev_ptr, port,
					    MTD_C_UNIT_GENERAL,
						MTD_CUNIT_PORT_CTRL,
						&cunit_port_ctrl));
	txgbe_attempt(txgbe_mtd_hw_xmdio_rd(dev_ptr, port,
					    MTD_C_UNIT_GENERAL,
						MTD_CUNIT_MODE_CONFIG,
						&cunit_mode_cfg));

	/* Because writes of some of these bits don't show up in the register on a read
	 * until after the software reset, we can't do repeated read-modify-writes
	 * to the same register or we will lose those changes.
	 * This approach also cuts down on IO and speeds up the code
	 */

	if (mac_type < MTD_MAC_LEAVE_UNCHANGED)
		txgbe_attempt(txgbe_mtd_set_reg_field_word(cunit_port_ctrl,
							   mac_type,
							   0, 3,
							   &cunit_port_ctrl));

	txgbe_attempt(txgbe_mtd_set_reg_field_word(cunit_mode_cfg,
						   (u16)mac_powerdown,
						   3, 1,
						   &cunit_mode_cfg));

	if (mac_snoop_sel < MTD_MAC_SNOOP_LEAVE_UNCHANGED)
		txgbe_attempt(txgbe_mtd_set_reg_field_word(cunit_mode_cfg,
							   mac_snoop_sel,
							   8, 2,
							   &cunit_mode_cfg));

	txgbe_attempt(txgbe_mtd_set_reg_field_word(cunit_mode_cfg,
						   mac_active_lane_sel, 10,
		1, &cunit_mode_cfg));

	if (mac_link_down_speed < MTD_MAC_SPEED_LEAVE_UNCHANGED)
		txgbe_attempt(txgbe_mtd_set_reg_field_word(cunit_mode_cfg,
							   mac_link_down_speed,
	6, 2, &cunit_mode_cfg));

	/* Now write changed values */
	txgbe_attempt(txgbe_mtd_xmdio_wr(dev_ptr, port,
					 MTD_C_UNIT_GENERAL,
					 MTD_CUNIT_PORT_CTRL,
					 cunit_port_ctrl));
	txgbe_attempt(txgbe_mtd_xmdio_wr(dev_ptr, port,
					 MTD_C_UNIT_GENERAL,
					 MTD_CUNIT_MODE_CONFIG,
					 cunit_mode_cfg));

	if (MTD_IS_X33X0_BASE(dev_ptr->device_id))
		if (mac_max_speed != MTD_MAX_MAC_SPEED_LEAVE_UNCHANGED)
			txgbe_attempt(txgbe_mtd_set_phy_feild(dev_ptr, port,
							      31, 0xF0A8, 0,
								     2, mac_max_speed));

	if (do_sw_rst) {
		txgbe_attempt(txgbe_mtd_cunit_sw_rst(dev_ptr, port));

		if (mac_link_down_speed < MTD_MAC_SPEED_LEAVE_UNCHANGED)
			txgbe_attempt(txgbe_mtd_cunit_sw_rst(dev_ptr, port));

		if (rerun_serdes_init)
			txgbe_attempt(txgbe_mtd_rerun_serdes_autoneg_init_automode(dev_ptr, port));
	}

	return MTD_OK;
}

static u32 txgbe_mtd_sem_create(struct mtd_dev *dev, enum begin_state state)
{
	if (dev->sem_create)
		return dev->sem_create(state);

	return 1;
}

static u32 txgbe_mtd_sem_delete(struct mtd_dev *dev, u32 smid)
{
	if ((dev->sem_delete) && smid)
		if (dev->sem_delete(smid))
			return MTD_FAIL;

	return MTD_OK;
}

u32 txgbe_mtd_load_driver(fmtd_read_mdio read_mdio,
			  fmtd_write_mdio write_mdio,
							bool macsec_indirect_access,
							f_create sem_create,
							f_delete sem_delete,
							f_take sem_take,
							f_give sem_give,
							u16 any_port,
							struct mtd_dev *dev)
{
	u16 data;

	/* Check for parameters validity */
	if (!dev)
		return MTD_API_ERR_DEV;

	/* The initialization was already done. */
	if (dev->dev_enabled)
		return MTD_API_ERR_DEV_ALREADY_EXIST;

	/* Make sure txgbe_mtd_wait() was implemented */
	if (txgbe_mtd_wait(1) == MTD_FAIL)
		return MTD_FAIL;

	dev->fmtd_read_mdio =  read_mdio;
	dev->mtd_write_mdio = write_mdio;

	dev->sem_create = sem_create;
	dev->sem_delete = sem_delete;
	dev->sem_take   = sem_take;
	dev->sem_give   = sem_give;
	dev->macsec_indirect_access = macsec_indirect_access;

	/* try to read 1.0 */
	if ((txgbe_mtd_hw_xmdio_rd(dev,
				   any_port, 1, 0, &data)) != MTD_OK)
		return MTD_API_FAIL_READ_REG;

	/* Initialize the MACsec Register Access semaphore. */
	dev->multi_addr_sem = txgbe_mtd_sem_create(dev, FULL);
	if (dev->multi_addr_sem == 0)
		return MTD_API_FAIL_SEM_CREATE;

	if (dev->msec_ctrl.msec_rev == MTD_MSEC_REV_FPGA) {
		dev->device_id = MTD_REV_3310P_Z2; /* verification: change if needed */
		dev->num_ports = 1; /* verification: change if needed */
		dev->this_port = 0;
	} else {
		/* After everything else is done, can fill in the device id */
		if ((txgbe_mtd_get_phy_revision(dev, any_port,
						&dev->device_id,
							   &dev->num_ports,
							   &dev->this_port)) != MTD_OK)
			return MTD_FAIL;
	}

	if (MTD_IS_X33X0_BASE(dev->device_id))
		dev->macsec_indirect_access = false;

	dev->dev_enabled = true;

	return MTD_OK;
}

u32 txgbe_mtd_unload_driver(struct mtd_dev *dev)
{
	/* Delete the MACsec register access semaphore.	*/
	if (txgbe_mtd_sem_delete(dev, dev->multi_addr_sem) != MTD_OK)
		return MTD_API_FAIL_SEM_DELETE;

	dev->fmtd_read_mdio =  NULL;
	dev->mtd_write_mdio = NULL;

	dev->sem_create = NULL;
	dev->sem_delete = NULL;
	dev->sem_take   = NULL;
	dev->sem_give   = NULL;

	dev->dev_enabled = false;

	return MTD_OK;
}
