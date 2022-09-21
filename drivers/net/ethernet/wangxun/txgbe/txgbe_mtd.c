/*
 * WangXun 10 Gigabit PCI Express Linux driver
 * Copyright (c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 */


#include "txgbe_mtd.h"
#include "kcompat.h"

MTD_STATUS mtdHwXmdioWrite(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	IN MTD_U16 dev,
	IN MTD_U16 reg,
	IN MTD_U16 value)
{
	MTD_STATUS result = MTD_OK;


	if (devPtr->fmtdWriteMdio != NULL) {
		if (devPtr->fmtdWriteMdio(devPtr, port, dev, reg, value) == MTD_FAIL) {
			result = MTD_FAIL;
			MTD_DBG_INFO("fmtdWriteMdio 0x%04X failed to port=%d, dev=%d, reg=0x%04X\n",
							(unsigned)(value), (unsigned)port, (unsigned)dev, (unsigned)reg);
		}
	} else
		result = MTD_FAIL;

	return result;
}

MTD_STATUS mtdHwXmdioRead(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	IN MTD_U16 dev,
	IN MTD_U16 reg,
	OUT MTD_U16 * data)
{
	MTD_STATUS result = MTD_OK;


	if (devPtr->fmtdReadMdio != NULL) {
		if (devPtr->fmtdReadMdio(devPtr, port, dev, reg, data) == MTD_FAIL) {
			result = MTD_FAIL;
			MTD_DBG_INFO("fmtdReadMdio failed from port=%d, dev=%d, reg=0x%04X\n",
						(unsigned)port, (unsigned)dev, (unsigned)reg);
		}
	} else
		result = MTD_FAIL;

	return result;
}

/*
	This macro calculates the mask for partial read/write of register's data.
*/
#define MTD_CALC_MASK(fieldOffset, fieldLen, mask)	do {\
			if ((fieldLen + fieldOffset) >= 16)	  \
				mask = (0 - (1 << fieldOffset));	\
			else									\
				mask = (((1 << (fieldLen + fieldOffset))) - (1 << fieldOffset));\
		} while (0)

MTD_STATUS mtdHwGetPhyRegField(
	IN  MTD_DEV_PTR devPtr,
	IN  MTD_U16	 port,
	IN  MTD_U16	 dev,
	IN  MTD_U16	 regAddr,
	IN  MTD_U8	 fieldOffset,
	IN  MTD_U8	 fieldLength,
	OUT MTD_U16	 * data)
{
	MTD_U16 tmpData;
	MTD_STATUS   retVal;

	retVal = mtdHwXmdioRead(devPtr, port, dev, regAddr, &tmpData);

	if (retVal != MTD_OK) {
		MTD_DBG_ERROR("Failed to read register \n");
		return MTD_FAIL;
	}

	mtdHwGetRegFieldFromWord(tmpData, fieldOffset, fieldLength, data);

	MTD_DBG_INFO("fOff %d, fLen %d, data 0x%04X.\n", (int)fieldOffset,
										(int)fieldLength, (int)*data);

	return MTD_OK;
}


MTD_STATUS mtdHwSetPhyRegField(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16	  port,
	IN MTD_U16	  dev,
	IN MTD_U16	  regAddr,
	IN MTD_U8	   fieldOffset,
	IN MTD_U8	   fieldLength,
	IN MTD_U16	  data)
{
	MTD_U16 tmpData, newData;
	MTD_STATUS   retVal;

	retVal = mtdHwXmdioRead(devPtr, port, dev, regAddr, &tmpData);
	if (retVal != MTD_OK) {
		MTD_DBG_ERROR("Failed to read register \n");
		return MTD_FAIL;
	}

	mtdHwSetRegFieldToWord(tmpData, data, fieldOffset, fieldLength, &newData);

	retVal = mtdHwXmdioWrite(devPtr, port, dev, regAddr, newData);

	if (retVal != MTD_OK) {
		MTD_DBG_ERROR("Failed to write register \n");
		return MTD_FAIL;
	}

	MTD_DBG_INFO("fieldOff %d, fieldLen %d, data 0x%x.\n", fieldOffset,
				  fieldLength, data);

	return MTD_OK;
}


MTD_STATUS mtdHwGetRegFieldFromWord(
	IN  MTD_U16	  regData,
	IN  MTD_U8	   fieldOffset,
	IN  MTD_U8	   fieldLength,
	OUT MTD_U16	  *data)
{
	/* Bits mask to be read */
	MTD_U16 mask;

	MTD_CALC_MASK(fieldOffset, fieldLength, mask);

	*data = (regData & mask) >> fieldOffset;

	return MTD_OK;
}


MTD_STATUS mtdHwSetRegFieldToWord(
	IN  MTD_U16	  regData,
	IN  MTD_U16	  bitFieldData,
	IN  MTD_U8	   fieldOffset,
	IN  MTD_U8	   fieldLength,
	OUT MTD_U16	  *data)
{
	/* Bits mask to be read */
	MTD_U16 mask;

	MTD_CALC_MASK(fieldOffset, fieldLength, mask);

	/* Set the desired bits to 0. */
	regData &= ~mask;
	/* Set the given data into the above reset bits.*/
	regData |= ((bitFieldData << fieldOffset) & mask);

	*data = regData;

	return MTD_OK;
}


MTD_STATUS mtdWait(IN MTD_UINT x)
{
	msleep(x);
	return MTD_OK;
}


/* internal device registers */
#define MTD_REG_CCCR9 31, 0xF05E	/* do not enclose in parentheses */
#define MTD_REG_SCR   31, 0xF0F0	/* do not enclose in parentheses */
#define MTD_REG_ECSR  31, 0xF0F5	/* do not enclose in parentheses */
MTD_STATUS mtdCheckDeviceCapabilities(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	OUT MTD_BOOL * phyHasMacsec,
	OUT MTD_BOOL * phyHasCopperInterface,
	OUT MTD_BOOL * isE20X0Device)
{
	MTD_U8 major, minor, inc, test;
	MTD_U16 abilities;

	*phyHasMacsec = MTD_TRUE;
	*phyHasCopperInterface = MTD_TRUE;
	*isE20X0Device = MTD_FALSE;

	if (mtdGetFirmwareVersion(devPtr, port, &major, &minor, &inc, &test) == MTD_FAIL) {
		/* firmware not running will produce this case */
		major = minor = inc = test = 0;
	}

	if (major == 0 && minor == 0 && inc == 0 && test == 0) {
		/* no code loaded into internal processor */
		/* have to read it from the device itself the hard way */
		MTD_U16 reg2, reg3; 
		MTD_U16 index, index2;
		MTD_U16 temp;
		MTD_U16 bit16thru23[8];

		/* save these registers */
		/* ATTEMPT(mtdHwXmdioRead(devPtr,port,MTD_REG_CCCR9,&reg1)); some revs can't read this register reliably */
		ATTEMPT(mtdHwXmdioRead(devPtr, port, MTD_REG_SCR, &reg2));
		ATTEMPT(mtdHwXmdioRead(devPtr, port, MTD_REG_ECSR, &reg3));

		/* clear these bit indications */
		for (index = 0; index < 8; index++) {
			bit16thru23[index] = 0;
		}

		ATTEMPT(mtdHwXmdioWrite(devPtr, port, MTD_REG_CCCR9, 0x0300)); /* force clock on */
		mtdWait(1);
		ATTEMPT(mtdHwXmdioWrite(devPtr, port, MTD_REG_SCR, 0x0102)); /* set access */
		mtdWait(1);

		ATTEMPT(mtdHwXmdioWrite(devPtr, port, MTD_REG_ECSR, 0x06D3)); /* sequence needed */
		mtdWait(1);
		ATTEMPT(mtdHwXmdioWrite(devPtr, port, MTD_REG_ECSR, 0x0593));
		mtdWait(1);
		ATTEMPT(mtdHwXmdioWrite(devPtr, port, MTD_REG_ECSR, 0x0513));
		mtdWait(1);

		index = 0;
		index2 = 0;
		while (index < 24) {
			ATTEMPT(mtdHwXmdioWrite(devPtr, port, MTD_REG_ECSR, 0x0413));
			mtdWait(1);
			ATTEMPT(mtdHwXmdioWrite(devPtr, port, MTD_REG_ECSR, 0x0513));
			mtdWait(1);

			if (index >= 16) {
				ATTEMPT(mtdHwXmdioRead(devPtr, port, MTD_REG_ECSR, &bit16thru23[index2++]));
			} else {
				ATTEMPT(mtdHwXmdioRead(devPtr, port, MTD_REG_ECSR, &temp));
			}
			mtdWait(1);
			index++;
		}

		if (((bit16thru23[0] >> 11) & 1) | ((bit16thru23[1] >> 11) & 1)) {
			*phyHasMacsec = MTD_FALSE;
		}
		if (((bit16thru23[4] >> 11) & 1) | ((bit16thru23[5] >> 11) & 1)) {
			*phyHasCopperInterface = MTD_FALSE;
		}

		if (((bit16thru23[6] >> 11) & 1) | ((bit16thru23[7] >> 11) & 1)) {
			*isE20X0Device = MTD_TRUE;
		}

		ATTEMPT(mtdHwXmdioWrite(devPtr, port, MTD_REG_ECSR, 0x0413));
		mtdWait(1);
		ATTEMPT(mtdHwXmdioWrite(devPtr, port, MTD_REG_ECSR, 0x0493));
		mtdWait(1);
		ATTEMPT(mtdHwXmdioWrite(devPtr, port, MTD_REG_ECSR, 0x0413));
		mtdWait(1);
		ATTEMPT(mtdHwXmdioWrite(devPtr, port, MTD_REG_ECSR, 0x0513));
		mtdWait(1);

		/* restore the registers */
		/* ATTEMPT(mtdHwXmdioWrite(devPtr,port,MTD_REG_CCCR9,reg1)); Some revs can't read this register reliably */
		ATTEMPT(mtdHwXmdioWrite(devPtr, port, MTD_REG_CCCR9, 0x5440)); /* set back to reset value */
		ATTEMPT(mtdHwXmdioWrite(devPtr, port, MTD_REG_SCR, reg2));
		ATTEMPT(mtdHwXmdioWrite(devPtr, port, MTD_REG_ECSR, reg3));

	} else {
		/* should just read it from the firmware status register */
		ATTEMPT(mtdHwXmdioRead(devPtr, port, MTD_TUNIT_XG_EXT_STATUS, &abilities));
		if (abilities & (1 << 12)) {
			*phyHasMacsec = MTD_FALSE;
		}

		if (abilities & (1 << 13)) {
			*phyHasCopperInterface = MTD_FALSE;
		}

		if (abilities & (1 << 14)) {
			*isE20X0Device = MTD_TRUE;
		}

	}

	return MTD_OK;
}

MTD_STATUS mtdIsPhyReadyAfterReset(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	OUT MTD_BOOL * phyReady)
{
	MTD_U16 val;

	*phyReady = MTD_FALSE;

	ATTEMPT(mtdHwGetPhyRegField(devPtr, port, MTD_TUNIT_IEEE_PMA_CTRL1, 15, 1, &val));

	if (val) {
		/* if still in reset return '0' (could be coming up, or disabled by download mode) */
		*phyReady = MTD_FALSE;
	} else {
		/* if Phy is in normal operation */
		*phyReady = MTD_TRUE;
	}

	return MTD_OK;
}


MTD_STATUS mtdSoftwareReset(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	IN MTD_U16 timeoutMs)
{
	MTD_U16 counter;
	MTD_BOOL phyReady;
	/* bit self clears when done */
	ATTEMPT(mtdHwSetPhyRegField(devPtr, port, MTD_TUNIT_IEEE_PMA_CTRL1, 15, 1, 1));

	if (timeoutMs) {
		counter = 0;
		ATTEMPT(mtdIsPhyReadyAfterReset(devPtr, port, &phyReady));
		while (phyReady == MTD_FALSE && counter <= timeoutMs) {
			ATTEMPT(mtdWait(1));
			ATTEMPT(mtdIsPhyReadyAfterReset(devPtr, port, &phyReady));
			counter++;
		}

		if (counter < timeoutMs) {
			return MTD_OK;
		} else {
			/* timed out without becoming ready */
			return MTD_FAIL;
		}
	} else {
		return MTD_OK;
	}
}

MTD_STATUS mtdIsPhyReadyAfterHardwareReset(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	OUT MTD_BOOL *phyReady)
{
	MTD_U16 val;

	*phyReady = MTD_FALSE;

	ATTEMPT(mtdHwGetPhyRegField(devPtr, port, MTD_CUNIT_PORT_CTRL, 14, 1, &val));

	if (val) {
		/* if still in reset return '0' (could be coming up, or disabled by download mode) */
		*phyReady = MTD_FALSE;
	} else {
		/* if Phy is in normal operation */
		*phyReady = MTD_TRUE;
	}
	return MTD_OK;
}


MTD_STATUS mtdHardwareReset(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	IN MTD_U16 timeoutMs)
{
	MTD_U16 counter;
	MTD_BOOL phyReady;

	/* bit self clears when done */
	ATTEMPT(mtdHwSetPhyRegField(devPtr, port, MTD_CUNIT_PORT_CTRL, 14, 1, 1));

	if (timeoutMs) {
		counter = 0;
		ATTEMPT(mtdIsPhyReadyAfterHardwareReset(devPtr, port, &phyReady));
		while (phyReady == MTD_FALSE && counter <= timeoutMs) {
			ATTEMPT(mtdWait(1));
			ATTEMPT(mtdIsPhyReadyAfterHardwareReset(devPtr, port, &phyReady));
			counter++;
		}
		if (counter < timeoutMs)
			return MTD_OK;
		else
			return MTD_FAIL; /* timed out without becoming ready */
	} else {
		return MTD_OK;
	}
}

/****************************************************************************/

/****************************************************************************/
/*******************************************************************
   802.3 Clause 28 and Clause 45
   Autoneg Related Control & Status
 *******************************************************************/
/*******************************************************************
  Enabling speeds for autonegotiation
  Reading speeds enabled for autonegotation
  Set/get pause advertisement for autonegotiation
  Other Autoneg-related Control and Status (restart,disable/enable,
  force master/slave/auto, checking for autoneg resolution, etc.)
 *******************************************************************/

#define MTD_7_0010_SPEED_BIT_LENGTH 4
#define MTD_7_0010_SPEED_BIT_POS	5
#define MTD_7_8000_SPEED_BIT_LENGTH 2
#define MTD_7_8000_SPEED_BIT_POS	8
#define MTD_7_0020_SPEED_BIT_LENGTH 1   /* for 88X32X0 family and 88X33X0 family */
#define MTD_7_0020_SPEED_BIT_POS	12
#define MTD_7_0020_SPEED_BIT_LENGTH2 2   /* for 88X33X0 family A0 revision 2.5/5G */
#define MTD_7_0020_SPEED_BIT_POS2	7

/* Bit defines for speed bits */
#define MTD_FORCED_SPEEDS_BIT_MASK  (MTD_SPEED_10M_HD_AN_DIS | MTD_SPEED_10M_FD_AN_DIS | \
									 MTD_SPEED_100M_HD_AN_DIS | MTD_SPEED_100M_FD_AN_DIS)
#define MTD_LOWER_BITS_MASK			0x000F /* bits in base page */
#define MTD_GIG_SPEED_POS			4
#define MTD_XGIG_SPEED_POS			6
#define MTD_2P5G_SPEED_POS			11
#define MTD_5G_SPEED_POS			12
#define MTD_GET_1000BT_BITS(_speedBits) ((_speedBits & (MTD_SPEED_1GIG_HD | MTD_SPEED_1GIG_FD)) \
										>> MTD_GIG_SPEED_POS) /* 1000BT bits */
#define MTD_GET_10GBT_BIT(_speedBits) ((_speedBits & MTD_SPEED_10GIG_FD) \
										>> MTD_XGIG_SPEED_POS) /* 10GBT bit setting */
#define MTD_GET_2P5GBT_BIT(_speedBits) ((_speedBits & MTD_SPEED_2P5GIG_FD) \
										>> MTD_2P5G_SPEED_POS) /* 2.5GBT bit setting */
#define MTD_GET_5GBT_BIT(_speedBits) ((_speedBits & MTD_SPEED_5GIG_FD) \
										>> MTD_5G_SPEED_POS) /* 5GBT bit setting */


MTD_STATUS mtdEnableSpeeds(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	IN MTD_U16 speed_bits,
	IN MTD_BOOL anRestart)
{
	MTD_BOOL speedForced;
	MTD_U16 dummy;
	MTD_U16 tempRegValue;

	if (speed_bits & MTD_FORCED_SPEEDS_BIT_MASK) {
		/* tried to force the speed, this function is for autonegotiation control */
		return MTD_FAIL;
	}

	if (MTD_IS_X32X0_BASE(devPtr->deviceId) && ((speed_bits & MTD_SPEED_2P5GIG_FD) ||
												(speed_bits & MTD_SPEED_5GIG_FD))) {
		return MTD_FAIL; /* tried to advertise 2.5G/5G on a 88X32X0 chipset */
	}

	if (MTD_IS_X33X0_BASE(devPtr->deviceId)) {
		const MTD_U16 chipRev = (devPtr->deviceId & 0xf); /* get the chip revision */

		if (chipRev == 9 || chipRev == 5 || chipRev == 1 || /* Z2 chip revisions */
			chipRev == 8 || chipRev == 4 || chipRev == 0)   /* Z1 chip revisions */ {
			/* this is an X33X0 or E20X0 Z2/Z1 device and not supported (not compatible with A0) */
			return MTD_FAIL;
		}
	}

	/* Enable AN and set speed back to power-on default in case previously forced
	   Only do it if forced, to avoid an extra/unnecessary soft reset */
	ATTEMPT(mtdGetForcedSpeed(devPtr, port, &speedForced, &dummy));
	if (speedForced) {
		ATTEMPT(mtdUndoForcedSpeed(devPtr, port, MTD_FALSE));
	}

	if (speed_bits == MTD_ADV_NONE) {
		/* Set all speeds to be disabled
		 Take care of bits in 7.0010 (advertisement register, 10BT and 100BT bits) */
		ATTEMPT(mtdHwSetPhyRegField(devPtr, port, 7, 0x0010,\
				MTD_7_0010_SPEED_BIT_POS, MTD_7_0010_SPEED_BIT_LENGTH, \
				0));

		/* Take care of speed bits in 7.8000 (1000BASE-T speed bits) */
		ATTEMPT(mtdHwSetPhyRegField(devPtr, port, 7, 0x8000,\
				MTD_7_8000_SPEED_BIT_POS, MTD_7_8000_SPEED_BIT_LENGTH, \
				0));

		/* Now take care of bit in 7.0020 (10GBASE-T) */
		ATTEMPT(mtdHwSetPhyRegField(devPtr, port, 7, 0x0020,\
				MTD_7_0020_SPEED_BIT_POS, MTD_7_0020_SPEED_BIT_LENGTH, 0));

		if (MTD_IS_X33X0_BASE(devPtr->deviceId)) {
			/* Now take care of bits in 7.0020 (2.5G, 5G speed bits) */
			ATTEMPT(mtdHwSetPhyRegField(devPtr, port, 7, 0x0020,\
					MTD_7_0020_SPEED_BIT_POS2, MTD_7_0020_SPEED_BIT_LENGTH2, 0));
		}
	} else {
		/* Take care of bits in 7.0010 (advertisement register, 10BT and 100BT bits) */
		ATTEMPT(mtdHwSetPhyRegField(devPtr, port, 7, 0x0010,\
				MTD_7_0010_SPEED_BIT_POS, MTD_7_0010_SPEED_BIT_LENGTH, \
				(speed_bits & MTD_LOWER_BITS_MASK)));

		/* Take care of speed bits in 7.8000 (1000BASE-T speed bits) */
		ATTEMPT(mtdHwSetPhyRegField(devPtr, port, 7, 0x8000,\
				MTD_7_8000_SPEED_BIT_POS, MTD_7_8000_SPEED_BIT_LENGTH, \
				MTD_GET_1000BT_BITS(speed_bits)));


		/* Now take care of bits in 7.0020 (10GBASE-T first) */
		ATTEMPT(mtdHwXmdioRead(devPtr, port, 7, 0x0020, &tempRegValue));
		ATTEMPT(mtdHwSetRegFieldToWord(tempRegValue, MTD_GET_10GBT_BIT(speed_bits),\
				MTD_7_0020_SPEED_BIT_POS, MTD_7_0020_SPEED_BIT_LENGTH, \
				&tempRegValue));

		if (MTD_IS_X33X0_BASE(devPtr->deviceId)) {
			/* Now take care of 2.5G bit in 7.0020 */
			ATTEMPT(mtdHwSetRegFieldToWord(tempRegValue, MTD_GET_2P5GBT_BIT(speed_bits),\
					7, 1, \
					&tempRegValue));

			/* Now take care of 5G bit in 7.0020 */
			ATTEMPT(mtdHwSetRegFieldToWord(tempRegValue, MTD_GET_5GBT_BIT(speed_bits),\
					8, 1, \
					&tempRegValue));
		}

		/* Now write result back to 7.0020 */
		ATTEMPT(mtdHwXmdioWrite(devPtr, port, 7, 0x0020, tempRegValue));

		if (MTD_GET_10GBT_BIT(speed_bits) ||
			MTD_GET_2P5GBT_BIT(speed_bits) ||
			MTD_GET_5GBT_BIT(speed_bits)) {
			/* Set XNP on if any bit that required it was set */
			ATTEMPT(mtdHwSetPhyRegField(devPtr, port, 7, 0, 13, 1, 1));
		}
	}

	if (anRestart) {
		return ((MTD_STATUS)(mtdAutonegEnable(devPtr, port) ||
							 mtdAutonegRestart(devPtr, port)));
	}

	return MTD_OK;
}

MTD_STATUS mtdUndoForcedSpeed(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	IN MTD_BOOL anRestart)
{

	ATTEMPT(mtdHwSetPhyRegField(devPtr, port, MTD_TUNIT_IEEE_PMA_CTRL1, 13, 1, 1));
	ATTEMPT(mtdHwSetPhyRegField(devPtr, port, MTD_TUNIT_IEEE_PMA_CTRL1, 6, 1, 1));

	/* when speed bits are changed, T unit sw reset is required, wait until phy is ready */
	ATTEMPT(mtdSoftwareReset(devPtr, port, 1000));

	if (anRestart) {
		return ((MTD_STATUS)(mtdAutonegEnable(devPtr, port) ||
							 mtdAutonegRestart(devPtr, port)));
	}

	return MTD_OK;
}



MTD_STATUS mtdGetForcedSpeed(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	OUT MTD_BOOL *speedIsForced,
	OUT MTD_U16 *forcedSpeed)
{
	MTD_U16 val, bit0, bit1, forcedSpeedBits, duplexBit;
	MTD_BOOL anDisabled;

	*speedIsForced = MTD_FALSE;
	*forcedSpeed = MTD_ADV_NONE;

	/* check if 7.0.12 is 0 or 1 (disabled or enabled) */
	ATTEMPT(mtdHwGetPhyRegField(devPtr, port, 7, 0, 12, 1, &val));

	(val) ? (anDisabled = MTD_FALSE) : (anDisabled = MTD_TRUE);

	if (anDisabled) {
		/* autoneg is disabled, see if it's forced to one of the speeds that work without AN */
		ATTEMPT(mtdHwGetPhyRegField(devPtr, port, MTD_TUNIT_IEEE_PMA_CTRL1, 6, 1, &bit0));
		ATTEMPT(mtdHwGetPhyRegField(devPtr, port, MTD_TUNIT_IEEE_PMA_CTRL1, 13, 1, &bit1));

		/* now read the duplex bit setting */
		ATTEMPT(mtdHwGetPhyRegField(devPtr, port, 7, 0x8000, 4, 1, &duplexBit));

		forcedSpeedBits = 0;
		forcedSpeedBits = bit0 | (bit1 << 1);

		if (forcedSpeedBits == 0) {
			/* it's set to 10BT */
			if (duplexBit) {
				*speedIsForced = MTD_TRUE;
				*forcedSpeed = MTD_SPEED_10M_FD_AN_DIS;
			} else {
				*speedIsForced = MTD_TRUE;
				*forcedSpeed = MTD_SPEED_10M_HD_AN_DIS;
			}
		} else if (forcedSpeedBits == 2) {
			/* it's set to 100BT */
			if (duplexBit) {
				*speedIsForced = MTD_TRUE;
				*forcedSpeed = MTD_SPEED_100M_FD_AN_DIS;
			} else {
				*speedIsForced = MTD_TRUE;
				*forcedSpeed = MTD_SPEED_100M_HD_AN_DIS;
			}
		}
		/* else it's set to 1000BT or 10GBT which require AN to work */
	}

	return MTD_OK;
}

MTD_STATUS mtdAutonegRestart(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port)
{
	/* set 7.0.9, restart AN */
	return (mtdHwSetPhyRegField(devPtr, port, 7, 0,
			 9, 1, 1));
}


MTD_STATUS mtdAutonegEnable(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port)
{
	/* set 7.0.12=1, enable AN */
	return (mtdHwSetPhyRegField(devPtr, port, 7, 0,
			12, 1, 1));
}

/******************************************************************************
 MTD_STATUS mtdAutonegIsSpeedDuplexResolutionDone
 (
	 IN MTD_DEV_PTR devPtr,
	 IN MTD_U16 port,
	 OUT MTD_BOOL *anSpeedResolutionDone
 );

 Inputs:
	devPtr - pointer to MTD_DEV initialized by mtdLoadDriver() call
	port - MDIO port address, 0-31

 Outputs:
	anSpeedResolutionDone - one of the following
		 MTD_TRUE if speed/duplex is resolved
		 MTD_FALSE if speed/duplex is not resolved

 Returns:
	MTD_OK or MTD_FAIL, if query was successful or not

 Description:
	Queries register 3.8008.11 Speed/Duplex resolved to see if autonegotiation
	is resolved or in progress. See note below. This function is only to be
	called if autonegotation is enabled and speed is not forced.

	anSpeedResolutionDone being MTD_TRUE, only indicates if AN has determined
	the speed and duplex bits in 3.8008, which will indicate what registers
	to read later for AN resolution after AN has completed.

 Side effects:
	None

 Notes/Warnings:
	If autonegotiation is disabled or speed is forced, this function returns
	MTD_TRUE.

******************************************************************************/
MTD_STATUS mtdAutonegIsSpeedDuplexResolutionDone(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	OUT MTD_BOOL *anSpeedResolutionDone)
{
	MTD_U16 val;

	/* read speed/duplex resolution done bit in 3.8008 bit 11 */
	if (mtdHwGetPhyRegField(devPtr, port,
			3, 0x8008, 11, 1, &val) == MTD_FAIL) {
		*anSpeedResolutionDone = MTD_FALSE;
		return MTD_FAIL;
	}

	(val) ? (*anSpeedResolutionDone = MTD_TRUE) : (*anSpeedResolutionDone = MTD_FALSE);

	return MTD_OK;
}


MTD_STATUS mtdGetAutonegSpeedDuplexResolution(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	OUT MTD_U16 *speedResolution)
{
	MTD_U16 val, speed, speed2, duplex;
	MTD_BOOL resDone;

	*speedResolution = MTD_ADV_NONE;

	/* check if AN is enabled */
	ATTEMPT(mtdHwGetPhyRegField(devPtr, port, \
			7, 0, 12, 1, &val));

	if (val) {
		/* an is enabled, check if speed is resolved */
		ATTEMPT(mtdAutonegIsSpeedDuplexResolutionDone(devPtr, port, &resDone));

		if (resDone) {
			ATTEMPT(mtdHwGetPhyRegField(devPtr, port, \
					3, 0x8008, 14, 2, &speed));

			ATTEMPT(mtdHwGetPhyRegField(devPtr, port, \
					3, 0x8008, 13, 1, &duplex));

			switch (speed) {
			case MTD_CU_SPEED_10_MBPS:
				if (duplex) {
					*speedResolution = MTD_SPEED_10M_FD;
				} else {
					*speedResolution = MTD_SPEED_10M_HD;
				}
				break;
			case MTD_CU_SPEED_100_MBPS:
				if (duplex) {
					*speedResolution = MTD_SPEED_100M_FD;
				} else {
					*speedResolution = MTD_SPEED_100M_HD;
				}
				break;
			case MTD_CU_SPEED_1000_MBPS:
				if (duplex) {
					*speedResolution = MTD_SPEED_1GIG_FD;
				} else {
					*speedResolution = MTD_SPEED_1GIG_HD;
				}
				break;
			case MTD_CU_SPEED_10_GBPS: /* also MTD_CU_SPEED_NBT */
				if (MTD_IS_X32X0_BASE(devPtr->deviceId)) {
					*speedResolution = MTD_SPEED_10GIG_FD; /* 10G has only full duplex, ignore duplex bit */
				} else {
					ATTEMPT(mtdHwGetPhyRegField(devPtr, port, \
							3, 0x8008, 2, 2, &speed2));

					switch (speed2) {
					case MTD_CU_SPEED_NBT_10G:
						*speedResolution = MTD_SPEED_10GIG_FD;
						break;

					case MTD_CU_SPEED_NBT_5G:
						*speedResolution = MTD_SPEED_5GIG_FD;
						break;

					case MTD_CU_SPEED_NBT_2P5G:
						*speedResolution = MTD_SPEED_2P5GIG_FD;
						break;

					default:
						/* this is an error */
						return MTD_FAIL;
						break;
					}
				}
				break;
			default:
				/* this is an error */
				return MTD_FAIL;
				break;
			}

		}

	}

	return MTD_OK;
}


/****************************************************************************/
MTD_STATUS mtdIsBaseTUp(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	OUT MTD_U16 *speed,
	OUT MTD_BOOL *linkUp)
{
	MTD_BOOL speedIsForced;
	MTD_U16 forcedSpeed, cuSpeed, cuLinkStatus;

	*linkUp = MTD_FALSE;
	*speed = MTD_ADV_NONE;

	/* first check if speed is forced to one of the speeds not requiring AN to train */
	ATTEMPT(mtdGetForcedSpeed(devPtr, port, &speedIsForced, &forcedSpeed));

	if (speedIsForced) {
		/* check if the link is up at the speed it's forced to */
		ATTEMPT(mtdHwGetPhyRegField(devPtr, port, 3, 0x8008, 14, 2, &cuSpeed));
		ATTEMPT(mtdHwGetPhyRegField(devPtr, port, 3, 0x8008, 10, 1, &cuLinkStatus));

		switch (forcedSpeed) {
		case MTD_SPEED_10M_HD_AN_DIS:
		case MTD_SPEED_10M_FD_AN_DIS:
		/* might want to add checking the duplex to make sure there
		 * is no duplex mismatch */
			if (cuSpeed == MTD_CU_SPEED_10_MBPS) {
				*speed = forcedSpeed;
			} else {
				*speed = MTD_SPEED_MISMATCH;
			}
			if (cuLinkStatus) {
				*linkUp = MTD_TRUE;
			}
			break;

		case MTD_SPEED_100M_HD_AN_DIS:
		case MTD_SPEED_100M_FD_AN_DIS:
		/* might want to add checking the duplex to make sure there
		 * is no duplex mismatch */
			if (cuSpeed == MTD_CU_SPEED_100_MBPS) {
				*speed = forcedSpeed;
			} else {
				*speed = MTD_SPEED_MISMATCH;
			}
			if (cuLinkStatus) {
				*linkUp = MTD_TRUE;
			}
			break;

		default:
			return MTD_FAIL;
			break;
		}
	} else {
		/* must be going through AN */
		ATTEMPT(mtdGetAutonegSpeedDuplexResolution(devPtr, port, speed));

		if (*speed != MTD_ADV_NONE) {
			/* check if the link is up at the speed it's AN to */
			ATTEMPT(mtdHwGetPhyRegField(devPtr, port, 3, 0x8008, 10, 1, &cuLinkStatus));

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
				if (cuLinkStatus) {
					*linkUp = MTD_TRUE;
				}
				break;
			default:
				return MTD_FAIL;
				break;
			}

		}
		/* else link is down, and AN is in progress, */
	}

	if (*speed == MTD_SPEED_MISMATCH) {
		return MTD_FAIL;
	} else {
		return MTD_OK;
	}

}

MTD_STATUS mtdSetPauseAdvertisement(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	IN MTD_U32 pauseType,
	IN MTD_BOOL anRestart)
{
	/* sets/clears bits 11, 10 (A6,A5 in the tech bit field of 7.16) */
	ATTEMPT(mtdHwSetPhyRegField(devPtr, port, 7, 0x0010, \
						10, 2, (MTD_U16)pauseType));

	if (anRestart) {
		return ((MTD_STATUS)(mtdAutonegEnable(devPtr, port) ||
							 mtdAutonegRestart(devPtr, port)));
	}

	return MTD_OK;
}


/******************************************************************************
 MTD_STATUS mtdAutonegIsCompleted
 (
	 IN MTD_DEV_PTR devPtr,
	 IN MTD_U16 port,
	 OUT MTD_BOOL *anStatusReady
 );

 Inputs:
	devPtr - pointer to MTD_DEV initialized by mtdLoadDriver() call
	port - MDIO port address, 0-31

 Outputs:
	anStatusReady - one of the following
		 MTD_TRUE if AN status registers are available to be read (7.1, 7.33, 7.32769, etc.)
		 MTD_FALSE if AN is not completed and AN status registers may contain old data

 Returns:
	MTD_OK or MTD_FAIL, if query was successful or not

 Description:
	Checks 7.1.5 for 1. If 1, returns MTD_TRUE. If not, returns MTD_FALSE. Many
	autonegotiation status registers are not valid unless AN has completed
	meaning 7.1.5 = 1.

 Side effects:
	None

 Notes/Warnings:
	Call this function before reading 7.33 or 7.32769 to check for master/slave
	resolution or other negotiated parameters which are negotiated during
	autonegotiation like fast retrain, fast retrain type, etc.

******************************************************************************/
MTD_STATUS mtdAutonegIsCompleted(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	OUT MTD_BOOL *anStatusReady)
{
	MTD_U16 val;

	/* read an completed, 7.1.5 bit */
	if (mtdHwGetPhyRegField(devPtr, port,
			7, 1, 5, 1, &val) == MTD_FAIL) {
		*anStatusReady = MTD_FALSE;
		return MTD_FAIL;
	}

	(val) ? (*anStatusReady = MTD_TRUE) : (*anStatusReady = MTD_FALSE);

	return MTD_OK;
}


MTD_STATUS mtdGetLPAdvertisedPause(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	OUT MTD_U8 *pauseBits)
{
	MTD_U16 val;
	MTD_BOOL anStatusReady;

	/* Make sure AN is complete */
	ATTEMPT(mtdAutonegIsCompleted(devPtr, port, &anStatusReady));

	if (anStatusReady == MTD_FALSE) {
		*pauseBits = MTD_CLEAR_PAUSE;
		return MTD_FAIL;
	}

	/* get bits 11, 10 (A6,A5 in the tech bit field of 7.19) */
	if (mtdHwGetPhyRegField(devPtr, port, 7, 19,
		 10, 2, &val) == MTD_FAIL) {
		*pauseBits = MTD_CLEAR_PAUSE;
		return MTD_FAIL;
	}

	*pauseBits = (MTD_U8)val;

	return MTD_OK;
}

/*******************************************************************
 Firmware Version
 *******************************************************************/
/****************************************************************************/
MTD_STATUS mtdGetFirmwareVersion(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	OUT MTD_U8 *major,
	OUT MTD_U8 *minor,
	OUT MTD_U8 *inc,
	OUT MTD_U8 *test)
{
	MTD_U16 reg_49169, reg_49170;

	ATTEMPT(mtdHwXmdioRead(devPtr, port, 1, 49169, &reg_49169));

	*major = (reg_49169 & 0xFF00) >> 8;
	*minor = (reg_49169 & 0x00FF);

	ATTEMPT(mtdHwXmdioRead(devPtr, port, 1, 49170, &reg_49170));

	*inc = (reg_49170 & 0xFF00) >> 8;
	*test = (reg_49170 & 0x00FF);

	/* firmware is not running if all 0's */
	if (!(*major || *minor || *inc || *test)) {
		return MTD_FAIL;
	}
	return MTD_OK;
}


MTD_STATUS mtdGetPhyRevision(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	OUT MTD_DEVICE_ID * phyRev,
	OUT MTD_U8 *numPorts,
	OUT MTD_U8 *thisPort)
{
	MTD_U16 temp = 0, tryCounter, temp2, baseType, reportedHwRev;
	MTD_U16 revision = 0, numports, thisport, readyBit, fwNumports, fwThisport;
	MTD_BOOL registerExists, regReady, hasMacsec, hasCopper, isE20X0Device;
	MTD_U8 major, minor, inc, test;

	*phyRev = MTD_REV_UNKNOWN; /* in case we have any failed ATTEMPT below, will return unknown */
	*numPorts = 0;
	*thisPort = 0;

	/* first check base type of device, get reported rev and port info */
	ATTEMPT(mtdHwXmdioRead(devPtr, port, 3, 0xD00D, &temp));
	baseType = ((temp & 0xFC00) >> 6);
	reportedHwRev = (temp & 0x000F);
	numports = ((temp & 0x0380) >> 7) + 1;
	thisport = ((temp & 0x0070) >> 4);

	/* find out if device has macsec/ptp, copper unit or is an E20X0-type device */
	ATTEMPT(mtdCheckDeviceCapabilities(devPtr, port, &hasMacsec, &hasCopper, &isE20X0Device));

	/* check if internal processor firmware is up and running, and if so, easier to get info */
	if (mtdGetFirmwareVersion(devPtr, port, &major, &minor, &inc, &test) == MTD_FAIL) {
		major = minor = inc = test = 0; /* this is expected if firmware is not loaded/running */
	}

	if (major == 0 && minor == 0 && inc == 0 && test == 0) {
		/* no firmware running, have to verify device revision */
		if (MTD_IS_X32X0_BASE(baseType)) {
			/* A0 and Z2 report the same revision, need to check which is which */
			if (reportedHwRev == 1) {
				/* need to figure out if it's A0 or Z2 */
				/* remove internal reset */
				ATTEMPT(mtdHwSetPhyRegField(devPtr, port, 3, 0xD801, 5, 1, 1));

				/* wait until it's ready */
				regReady = MTD_FALSE;
				tryCounter = 0;
				while (regReady == MTD_FALSE && tryCounter++ < 10) {
					ATTEMPT(mtdWait(1)); /* timeout is set to 10 ms */
					ATTEMPT(mtdHwGetPhyRegField(devPtr, port, 3, 0xD007, 6, 1, &readyBit));
					if (readyBit == 1) {
						regReady = MTD_TRUE;
					}
				}

				if (regReady == MTD_FALSE) {
					/* timed out, can't tell for sure what rev this is */
					*numPorts = 0;
					*thisPort = 0;
					*phyRev = MTD_REV_UNKNOWN;
					return MTD_FAIL;
				}

				/* perform test */
				registerExists = MTD_FALSE;
				ATTEMPT(mtdHwXmdioRead(devPtr, port, 3, 0x8EC6, &temp));
				ATTEMPT(mtdHwXmdioWrite(devPtr, port, 3, 0x8EC6, 0xA5A5));
				ATTEMPT(mtdHwXmdioRead(devPtr, port, 3, 0x8EC6, &temp2));

				/* put back internal reset */
				ATTEMPT(mtdHwSetPhyRegField(devPtr, port, 3, 0xD801, 5, 1, 0));

				if (temp == 0 && temp2 == 0xA5A5) {
					registerExists = MTD_TRUE;
				}

				if (registerExists == MTD_TRUE) {
					revision = 2; /* this is actually QA0 */
				} else {
					revision = reportedHwRev; /* this is a QZ2 */
				}

			} else {
				/* it's not A0 or Z2, use what's reported by the hardware */
				revision = reportedHwRev;
			}
		} else if (MTD_IS_X33X0_BASE(baseType)) {
			/* all 33X0 devices report correct revision */
			revision = reportedHwRev;
		}

		/* have to use what's reported by the hardware */
		*numPorts = (MTD_U8)numports;
		*thisPort = (MTD_U8)thisport;
	} else {
		/* there is firmware loaded/running in internal processor */
		/* can get device revision reported by firmware */
		ATTEMPT(mtdHwXmdioRead(devPtr, port, MTD_TUNIT_PHY_REV_INFO_REG, &temp));
		ATTEMPT(mtdHwGetRegFieldFromWord(temp, 0, 4, &revision));
		ATTEMPT(mtdHwGetRegFieldFromWord(temp, 4, 3, &fwNumports));
		ATTEMPT(mtdHwGetRegFieldFromWord(temp, 7, 3, &fwThisport));
		if (fwNumports == numports && fwThisport == thisport) {
			*numPorts = (MTD_U8)numports;
			*thisPort = (MTD_U8)thisport;
		} else {
			*phyRev = MTD_REV_UNKNOWN;
			*numPorts = 0;
			*thisPort = 0;
			return MTD_FAIL; /* firmware and hardware are reporting different values */
		}
	}

	/* now have correct information to build up the MTD_DEVICE_ID */
	if (MTD_IS_X32X0_BASE(baseType)) {
		temp =  MTD_X32X0_BASE;
	} else if (MTD_IS_X33X0_BASE(baseType)) {
		temp = MTD_X33X0_BASE;
	} else {
		*phyRev = MTD_REV_UNKNOWN;
		*numPorts = 0;
		*thisPort = 0;
		return MTD_FAIL;
	}

	if (hasMacsec) {
		temp |= MTD_MACSEC_CAPABLE;
	}

	if (hasCopper) {
		temp |= MTD_COPPER_CAPABLE;
	}

	if (MTD_IS_X33X0_BASE(baseType) && isE20X0Device) {
		temp |= MTD_E20X0_DEVICE;
	}

	temp |= (revision & 0xF);

	*phyRev = (MTD_DEVICE_ID)temp;

	/* make sure we got a good one */
	if (mtdIsPhyRevisionValid(*phyRev) == MTD_OK) {
		return MTD_OK;
	} else {
		return MTD_FAIL; /* unknown or unsupported, if recognized but unsupported, value is still valid */
	}
}

MTD_STATUS mtdIsPhyRevisionValid(IN MTD_DEVICE_ID phyRev)
{
	switch (phyRev) {
	/* list must match MTD_DEVICE_ID */
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
		break;

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
		break;
	}
}

/* mtdCunit.c */
MTD_STATUS mtdCunitSwReset(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port)
{
	return mtdHwSetPhyRegField(devPtr, port, MTD_CUNIT_PORT_CTRL, 15, 1, 1);
}

/* mtdHxunit.c */
MTD_STATUS mtdRerunSerdesAutoInitializationUseAutoMode(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port)
{
	MTD_U16 temp, temp2, temp3;
	MTD_U16 waitCounter;

	ATTEMPT(mtdHwXmdioRead(devPtr, port, MTD_SERDES_CTRL_STATUS, &temp));

	ATTEMPT(mtdHwSetRegFieldToWord(temp, 3, 14, 2, &temp2));  /* execute bits and disable bits set */

	ATTEMPT(mtdHwXmdioWrite(devPtr, port, MTD_SERDES_CTRL_STATUS, temp2));

	/* wait for it to be done */
	waitCounter = 0;
	ATTEMPT(mtdHwXmdioRead(devPtr, port, MTD_SERDES_CTRL_STATUS, &temp3));
	while ((temp3 & 0x8000) && (waitCounter < 100)) {
		ATTEMPT(mtdWait(1));
		ATTEMPT(mtdHwXmdioRead(devPtr, port, MTD_SERDES_CTRL_STATUS, &temp3));
		waitCounter++;
	}

	/* if speed changed, let it stay. that's the speed that it ended up changing to/serdes was initialied to */
	if (waitCounter >= 100) {
		return MTD_FAIL; /* execute timed out */
	}

	return MTD_OK;
}


/* mtdHunit.c */
/******************************************************************************
 Mac Interface functions
******************************************************************************/

MTD_STATUS mtdSetMacInterfaceControl(
	IN MTD_DEV_PTR devPtr,
	IN MTD_U16 port,
	IN MTD_U16 macType,
	IN MTD_BOOL macIfPowerDown,
	IN MTD_U16 macIfSnoopSel,
	IN MTD_U16 macIfActiveLaneSelect,
	IN MTD_U16 macLinkDownSpeed,
	IN MTD_U16 macMaxIfSpeed, /* 33X0/E20X0 devices only */
	IN MTD_BOOL doSwReset,
	IN MTD_BOOL rerunSerdesInitialization)
{
	MTD_U16 cunitPortCtrl, cunitModeConfig;

	/* do range checking on parameters */
	if ((macType > MTD_MAC_LEAVE_UNCHANGED)) {
		return MTD_FAIL;
	}

	if ((macIfSnoopSel > MTD_MAC_SNOOP_LEAVE_UNCHANGED) ||
		(macIfSnoopSel == 1)) {
		return MTD_FAIL;
	}

	if (macIfActiveLaneSelect > 1) {
		return MTD_FAIL;
	}

	if (macLinkDownSpeed > MTD_MAC_SPEED_LEAVE_UNCHANGED) {
		return MTD_FAIL;
	}

	if (!(macMaxIfSpeed == MTD_MAX_MAC_SPEED_10G ||
			macMaxIfSpeed == MTD_MAX_MAC_SPEED_5G ||
			macMaxIfSpeed == MTD_MAX_MAC_SPEED_2P5G ||
			macMaxIfSpeed == MTD_MAX_MAC_SPEED_LEAVE_UNCHANGED ||
			macMaxIfSpeed == MTD_MAX_MAC_SPEED_NOT_APPLICABLE)) {
		return MTD_FAIL;
	}


	ATTEMPT(mtdHwXmdioRead(devPtr, port, MTD_CUNIT_PORT_CTRL, &cunitPortCtrl));
	ATTEMPT(mtdHwXmdioRead(devPtr, port, MTD_CUNIT_MODE_CONFIG, &cunitModeConfig));

	/* Because writes of some of these bits don't show up in the register on a read
	 * until after the software reset, we can't do repeated read-modify-writes
	 * to the same register or we will lose those changes.

	 * This approach also cuts down on IO and speeds up the code
	 */

	if (macType < MTD_MAC_LEAVE_UNCHANGED) {
		ATTEMPT(mtdHwSetRegFieldToWord(cunitPortCtrl, macType, 0, 3, &cunitPortCtrl));
	}

	ATTEMPT(mtdHwSetRegFieldToWord(cunitModeConfig, (MTD_U16)macIfPowerDown, 3, 1, &cunitModeConfig));

	if (macIfSnoopSel < MTD_MAC_SNOOP_LEAVE_UNCHANGED) {
		ATTEMPT(mtdHwSetRegFieldToWord(cunitModeConfig, macIfSnoopSel, 8, 2, &cunitModeConfig));
	}

	ATTEMPT(mtdHwSetRegFieldToWord(cunitModeConfig, macIfActiveLaneSelect, 10, 1, &cunitModeConfig));

	if (macLinkDownSpeed < MTD_MAC_SPEED_LEAVE_UNCHANGED) {
		ATTEMPT(mtdHwSetRegFieldToWord(cunitModeConfig, macLinkDownSpeed, 6, 2, &cunitModeConfig));
	}

	/* Now write changed values */
	ATTEMPT(mtdHwXmdioWrite(devPtr, port, MTD_CUNIT_PORT_CTRL, cunitPortCtrl));
	ATTEMPT(mtdHwXmdioWrite(devPtr, port, MTD_CUNIT_MODE_CONFIG, cunitModeConfig));

	if (MTD_IS_X33X0_BASE(devPtr->deviceId)) {
		if (macMaxIfSpeed != MTD_MAX_MAC_SPEED_LEAVE_UNCHANGED) {
			ATTEMPT(mtdHwSetPhyRegField(devPtr, port, 31, 0xF0A8, 0, 2, macMaxIfSpeed));
		}
	}

	if (doSwReset == MTD_TRUE) {
		ATTEMPT(mtdCunitSwReset(devPtr, port));

		if (macLinkDownSpeed < MTD_MAC_SPEED_LEAVE_UNCHANGED) {
			ATTEMPT(mtdCunitSwReset(devPtr, port)); /* need 2x for changes to macLinkDownSpeed */
		}

		if (rerunSerdesInitialization == MTD_TRUE) {
			ATTEMPT(mtdRerunSerdesAutoInitializationUseAutoMode(devPtr, port));
		}
	}

	return MTD_OK;
}


/*******************************************************************************
* mtdSemCreate
*
* DESCRIPTION:
*	   Create semaphore.
*
* INPUTS:
*		state - beginning state of the semaphore, either MTD_SEM_EMPTY or MTD_SEM_FULL
*
* OUTPUTS:
*	   None
*
* RETURNS:
*	   MTD_SEM if success. Otherwise, NULL
*
* COMMENTS:
*	   None
*
*******************************************************************************/
MTD_SEM mtdSemCreate(
	IN MTD_DEV * dev,
	IN MTD_SEM_BEGIN_STATE state)
{
	if (dev->semCreate)
		return dev->semCreate(state);

	return 1; /* should return any value other than 0 to let it keep going */
}

/*******************************************************************************
* mtdSemDelete
*
* DESCRIPTION:
*	   Delete semaphore.
*
* INPUTS:
*	   smid - semaphore Id
*
* OUTPUTS:
*	   None
*
* RETURNS:
*	   MTD_OK   - on success
*	   MTD_FAIL - on error
*
* COMMENTS:
*	   None
*
*******************************************************************************/
MTD_STATUS mtdSemDelete(
	IN MTD_DEV * dev,
	IN MTD_SEM smid)
{
	if ((dev->semDelete) && (smid))
		if (dev->semDelete(smid))
		  return MTD_FAIL;

	return MTD_OK;
}



MTD_STATUS mtdLoadDriver(
	IN FMTD_READ_MDIO	 readMdio,
	IN FMTD_WRITE_MDIO	writeMdio,
	IN MTD_BOOL		   macsecIndirectAccess,
	IN FMTD_SEM_CREATE	semCreate,
	IN FMTD_SEM_DELETE	semDelete,
	IN FMTD_SEM_TAKE	  semTake,
	IN FMTD_SEM_GIVE	  semGive,
	IN MTD_U16			anyPort,
	OUT MTD_DEV		  * dev)
{
	MTD_U16 data;

	MTD_DBG_INFO("mtdLoadDriver Called.\n");

	/* Check for parameters validity */
	if (dev == NULL) {
		MTD_DBG_ERROR("MTD_DEV pointer is NULL.\n");
		return MTD_API_ERR_DEV;
	}

	/* The initialization was already done. */
	if (dev->devEnabled) {
		MTD_DBG_ERROR("Device Driver already loaded.\n");
		return MTD_API_ERR_DEV_ALREADY_EXIST;
	}

	/* Make sure mtdWait() was implemented */
	if (mtdWait(1) == MTD_FAIL) {
		MTD_DBG_ERROR("mtdWait() not implemented.\n");
		return MTD_FAIL;
	}

	dev->fmtdReadMdio =  readMdio;
	dev->fmtdWriteMdio = writeMdio;

	dev->semCreate = semCreate;
	dev->semDelete = semDelete;
	dev->semTake   = semTake;
	dev->semGive   = semGive;
	dev->macsecIndirectAccess = macsecIndirectAccess;  /* 88X33X0 and later force direct access */

	/* try to read 1.0 */
	if ((mtdHwXmdioRead(dev, anyPort, 1, 0, &data)) != MTD_OK) {
		MTD_DBG_ERROR("Reading to reg %x failed.\n", 0);
		return MTD_API_FAIL_READ_REG;
	}

	MTD_DBG_INFO("mtdLoadDriver successful.\n");

	/* Initialize the MACsec Register Access semaphore.	*/
	if ((dev->multiAddrSem = mtdSemCreate(dev, MTD_SEM_FULL)) == 0) {
		MTD_DBG_ERROR("semCreate Failed.\n");
		return MTD_API_FAIL_SEM_CREATE;
	}

	if (dev->msec_ctrl.msec_rev == MTD_MSEC_REV_FPGA) {
		dev->deviceId = MTD_REV_3310P_Z2; /* verification: change if needed */
		dev->numPorts = 1; /* verification: change if needed */
		dev->thisPort = 0;
	} else {
		/* After everything else is done, can fill in the device id */
		if ((mtdGetPhyRevision(dev, anyPort,
							   &(dev->deviceId),
							   &(dev->numPorts),
							   &(dev->thisPort))) != MTD_OK) {
			MTD_DBG_ERROR("mtdGetPhyRevision Failed.\n");
			return MTD_FAIL;
		}
	}


	if (MTD_IS_X33X0_BASE(dev->deviceId)) {
		dev->macsecIndirectAccess = MTD_FALSE; /* bug was fixed in 88X33X0 and later revisions, go direct */
	}

	dev->devEnabled = MTD_TRUE;

	MTD_DBG_INFO("mtdLoadDriver successful !!!.\n");

	return MTD_OK;
}

/*******************************************************************************
* mtdUnloadDriver
*
* DESCRIPTION:
*	   This function clears MTD_DEV structure.
*
* INPUTS:
*	   None.
*
* OUTPUTS:
*	   None.
*
* RETURNS:
*	   MTD_OK		   - on success
*	   MTD_FAIL		 - on error
*
* COMMENTS:
*	   1.  This function should be called only after successful execution of
*		   mtdLoadDriver().
*
*******************************************************************************/
MTD_STATUS mtdUnloadDriver(IN MTD_DEV * dev)
{
	MTD_DBG_INFO("mtdUnloadDriver Called.\n");

	/* Delete the MACsec register access semaphore.	*/
	if (mtdSemDelete(dev, dev->multiAddrSem) != MTD_OK) {
		MTD_DBG_ERROR("SemDelete Failed.\n");
		return MTD_API_FAIL_SEM_DELETE;
	}

	dev->fmtdReadMdio =  NULL;
	dev->fmtdWriteMdio = NULL;

	dev->semCreate = NULL;
	dev->semDelete = NULL;
	dev->semTake   = NULL;
	dev->semGive   = NULL;

	dev->devEnabled = MTD_FALSE;

	return MTD_OK;
}

