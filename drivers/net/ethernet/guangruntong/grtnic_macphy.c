#include "grtnic.h"
#include "grtnic_macphy.h"

void grtnic_SetSpeed(struct net_device *netdev, int speed)
{
	struct grtnic_adapter *adapter  = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;

	u16 BaseAddr = XXGE_PORT_ADDRBASE;
	u32 Speed_reg = speed << 30;
	GRTNIC_WRITE_REG(hw, (BaseAddr + XXGE_SPEED_OFFSET), Speed_reg, 0);
}


void grtnic_SetFc(struct net_device *netdev, int onoff) //flow control no use, use setpause blow
{
	struct grtnic_adapter *adapter  = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;

	u16 BaseAddr = XXGE_PORT_ADDRBASE;
	u32 Reg;
	
	Reg = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_FCC_OFFSET), 0);
	if(onoff)
	{
		Reg |= XXGE_FCC_FCRX_MASK;
	}
	else
	{
		Reg &= ~XXGE_FCC_FCRX_MASK;
	}

	GRTNIC_WRITE_REG(hw, (BaseAddr + XXGE_FCC_OFFSET), Reg, 0);
}

int grtnic_ResetTx(struct net_device *netdev)
{
	struct grtnic_adapter *adapter  = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;

	u16 BaseAddr = XXGE_PORT_ADDRBASE;
	u32 RegTc;
	u32 TimeoutLoops;
	
	RegTc = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_TC_OFFSET), 0);
	RegTc |= XXGE_TC_RST_MASK;
	GRTNIC_WRITE_REG(hw, (BaseAddr + XXGE_TC_OFFSET), RegTc, 0);
	TimeoutLoops  = XXGE_RST_DELAY_LOOPCNT_VAL;
	/* Poll until the reset is done */
	while (TimeoutLoops  && (RegTc & XXGE_TC_RST_MASK)) {
		RegTc = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_TC_OFFSET), 0);
		TimeoutLoops --;
	} //return somthing

	if(0 == TimeoutLoops ) {
		return 1;
	}
	return 0;
}

int grtnic_ResetRx(struct net_device *netdev)
{
	struct grtnic_adapter *adapter  = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;

	u16 BaseAddr = XXGE_PORT_ADDRBASE;
	u32 RegRcw1;
	u32 TimeoutLoops;
	
	RegRcw1 = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_RCW1_OFFSET), 0);
	RegRcw1 |= XXGE_RCW1_RST_MASK;
	GRTNIC_WRITE_REG(hw, (BaseAddr + XXGE_RCW1_OFFSET), RegRcw1, 0);

	TimeoutLoops  = XXGE_RST_DELAY_LOOPCNT_VAL;
	/* Poll until the reset is done */
	while (TimeoutLoops  && (RegRcw1 & XXGE_RCW1_RST_MASK)) {
		RegRcw1 = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_RCW1_OFFSET), 0);
		TimeoutLoops --;
	} //return somthing

	if(0 == TimeoutLoops ) {
		return 1;
	}
	return 0;
}


void grtnic_SetTx(struct net_device *netdev, int onoff)
{
	struct grtnic_adapter *adapter  = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;

	u16 BaseAddr = XXGE_PORT_ADDRBASE;
	u32 RegTc;

	RegTc = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_TC_OFFSET), 0);
	if(onoff)
	{
		RegTc |= XXGE_TC_TX_MASK;
	}
	else
	{
		RegTc &= ~XXGE_TC_TX_MASK;
	}

	RegTc |= XXGE_TC_DIC_MASK; //Deficit Idle Count Enable

	GRTNIC_WRITE_REG(hw, (BaseAddr + XXGE_TC_OFFSET), RegTc, 0);
	RegTc = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_TC_OFFSET), 0);
}

void grtnic_SetRx(struct net_device *netdev, int onoff)
{
	struct grtnic_adapter *adapter  = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;

	u16 BaseAddr = XXGE_PORT_ADDRBASE;
	u32 RegRcw1;
	
	RegRcw1 = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_RCW1_OFFSET), 0);

	if(onoff)
	{
		RegRcw1 |= XXGE_RCW1_RX_MASK;
	}
	else
	{
		RegRcw1 &= ~XXGE_RCW1_RX_MASK;
	}

	GRTNIC_WRITE_REG(hw, (BaseAddr + XXGE_RCW1_OFFSET), RegRcw1, 0);
	RegRcw1 = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_RCW1_OFFSET), 0);
//	printk("after RegRcw1 = %08x\n", RegRcw1);
}

void grtnic_GetRx(struct net_device *netdev, u32 *status)
{
	struct grtnic_adapter *adapter  = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;

	u16 BaseAddr = XXGE_PORT_ADDRBASE;
	*status = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_RCW1_OFFSET), 0);
//	printk("read RegRcw1 = %08x\n", *status);
}

void grtnic_SetMaxFrameLen(struct net_device *netdev, int len)
{
	struct grtnic_adapter *adapter  = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;

	u32 value;
	u16 BaseAddr = XXGE_PORT_ADDRBASE;

	value = ((len & 0x7FFF) | (1<<16));

	GRTNIC_WRITE_REG(hw, (BaseAddr + XXGE_TMTU_OFFSET), value, 0);
	GRTNIC_WRITE_REG(hw, (BaseAddr + XXGE_RMTU_OFFSET), value, 0);
}

void grtnic_SetJumbo(struct net_device *netdev, int onoff)
{
	struct grtnic_adapter *adapter  = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;

	u32 RegRcw1;
	u32 RegTc;
	u16 BaseAddr = XXGE_PORT_ADDRBASE;

	RegRcw1 = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_RCW1_OFFSET), 0);
	RegTc = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_TC_OFFSET), 0);

	if(onoff)
	{
		RegRcw1 |= XXGE_RCW1_JUM_MASK;
		RegTc 	|= XXGE_TC_JUM_MASK;
	}
	else
	{
		RegRcw1 &= ~XXGE_RCW1_JUM_MASK;
		RegTc 	&= ~XXGE_TC_JUM_MASK;
	}
	GRTNIC_WRITE_REG(hw, (BaseAddr + XXGE_RCW1_OFFSET), RegRcw1, 0);
	GRTNIC_WRITE_REG(hw, (BaseAddr + XXGE_TC_OFFSET), RegTc, 0);
}

void grtnic_SetAdrsFilter(struct net_device *netdev, int filter)
{
	struct grtnic_adapter *adapter  = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;

	GRTNIC_WRITE_REG(hw, MAC_ADRS_FILTER, filter, 0);
}

int grtnic_GetAdrsFilter(struct net_device *netdev)
{
	struct grtnic_adapter *adapter  = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;
	int filter;

	filter = GRTNIC_READ_REG(hw, MAC_ADRS_FILTER, 0);
	return filter;
}

void grtnic_SetMacAddress(struct net_device *netdev, const u8 *AddressPtr)
{
	struct grtnic_adapter *adapter  = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;

	u32 MacAddr;
	u8 *Aptr = (u8 *) AddressPtr;

  MacAddr = Aptr[0];
  MacAddr |= Aptr[1] << 8;
  MacAddr |= Aptr[2] << 16;
  MacAddr |= Aptr[3] << 24;
	GRTNIC_WRITE_REG(hw, MAC_ADRS_LOW, MacAddr, 0); //addr l

  MacAddr  = 0;
  MacAddr |= Aptr[4];
  MacAddr |= Aptr[5] << 8;
	GRTNIC_WRITE_REG(hw, MAC_ADRS_HIGH, MacAddr, 0); //addr h
}


void grtnic_GetMacAddress(struct net_device *netdev, void *AddressPtr)
{
	struct grtnic_adapter *adapter  = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;

	u32 MacAddr;
	u8 *Aptr = (u8 *) AddressPtr;

	MacAddr = GRTNIC_READ_REG(hw, MAC_ADRS_LOW, 0);
  Aptr[0] = (u8) MacAddr;
  Aptr[1] = (u8) (MacAddr >> 8);
  Aptr[2] = (u8) (MacAddr >> 16);
  Aptr[3] = (u8) (MacAddr >> 24);

	MacAddr = GRTNIC_READ_REG(hw, MAC_ADRS_HIGH, 0);
  Aptr[4] = (u8) MacAddr;
  Aptr[5] = (u8) (MacAddr >> 8);
}

void grtnic_PhySetMdioDivisor(struct net_device *netdev, u8 Divisor)
{
	struct grtnic_adapter *adapter  = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;

	u16 BaseAddr = XXGE_PORT_ADDRBASE;
	GRTNIC_WRITE_REG(hw, (BaseAddr + XXGE_MDIO_CFG0_OFFSET), ((u32) Divisor | XXGE_MDIO_CFG0_MDIOEN_MASK), 0);
}

void grtnic_SetPhyAddr(struct net_device *netdev, u32 Prtad, u32 Devad, u32 RegisterNum) //only for 10G phy
{
	struct grtnic_adapter *adapter  = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;

	u16 BaseAddr = XXGE_PORT_ADDRBASE;
	u32 Address;
	u32 MdioCtrlReg = 0;
	u32 TimeoutLoops;
    /* Sequence of steps is:
     * - Set MDIO REG (TX Data)
     * - TX Data opcode (CFG1) 0x00 and PRTAD, DEVAD be written (TX Data)
     * - Check for MDIO ready at every step
     */

	/*
	 * Wait till the MDIO interface is ready to accept a new transaction.
	 */
//////////////////////////////////////////////////////////////////////////////////////////////
	MdioCtrlReg = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_MDIO_CFG1_OFFSET), 0);

	TimeoutLoops  = XXGE_MDIO_RDY_LOOPCNT_VAL;
	/* Poll until the reset is done */
	while (TimeoutLoops  && !(MdioCtrlReg & XXGE_MDIO_CFG1_READY_MASK)) {
		MdioCtrlReg = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_MDIO_CFG1_OFFSET), 0);
		TimeoutLoops --;
	} //return somthing

//	if(0 == TimeoutLoops ) printk("Timeout 1\n");
//////////////////////////////////////////////////////////////////////////////////////////////

	GRTNIC_WRITE_REG(hw, (BaseAddr + XXGE_MDIO_TX_DATA_OFFSET), RegisterNum, 0);

    /* Now initiate the set PHY register address operation */
	Address = ((Prtad << 24) | (Devad << 16));
	MdioCtrlReg = Address | XXGE_MDIO_CFG1_INITIATE_MASK;

	GRTNIC_WRITE_REG(hw, (BaseAddr + XXGE_MDIO_CFG1_OFFSET), MdioCtrlReg, 0);
	/*
	 * Wait till MDIO transaction is completed.
	 */
//////////////////////////////////////////////////////////////////////////////////////////////
	MdioCtrlReg = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_MDIO_CFG1_OFFSET), 0);

	TimeoutLoops  = XXGE_MDIO_RDY_LOOPCNT_VAL;
	/* Poll until the reset is done */
	while (TimeoutLoops  && !(MdioCtrlReg & XXGE_MDIO_CFG1_READY_MASK)) {
		MdioCtrlReg = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_MDIO_CFG1_OFFSET), 0);
		TimeoutLoops --;
	} //return somthing

//	if(0 == TimeoutLoops ) printk("Timeout 2\n");
//////////////////////////////////////////////////////////////////////////////////////////////
}


void grtnic_PhyRead(struct net_device *netdev, u32 PhyAddress, u32 RegisterNum, u16 *PhyDataPtr) //if 10G, RegisterNum is devad
{
	struct grtnic_adapter *adapter  = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;
	int max_speed = adapter->speed;
	u32 mdio_cfg1_op_read_mask;
	u32 TimeoutLoops;


	u32 Address;
	u32 MdioCtrlReg = 0;
	u16 BaseAddr = XXGE_PORT_ADDRBASE;

    /* Sequence of steps is:
     * - Set Address opcode (CFG1) and actual address (TX Data)
     * - RX Data opcode (CFG1) and actual data read (RX Data)
     * - Check for MDIO ready at every step
     */

	/*
	 * Wait till MDIO interface is ready to accept a new transaction.
	 */
//////////////////////////////////////////////////////////////////////////////////////////////
	MdioCtrlReg = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_MDIO_CFG1_OFFSET), 0);

	TimeoutLoops  = XXGE_MDIO_RDY_LOOPCNT_VAL;
	/* Poll until the reset is done */
	while (TimeoutLoops  && !(MdioCtrlReg & XXGE_MDIO_CFG1_READY_MASK)) {
		MdioCtrlReg = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_MDIO_CFG1_OFFSET), 0);
		TimeoutLoops --;
	} //return somthing

//	if(0 == TimeoutLoops ) printk("Timeout\n");
//////////////////////////////////////////////////////////////////////////////////////////////


	mdio_cfg1_op_read_mask = (max_speed==1) ? XXGE_MDIO_CFG1_OP_READ_MASK_10G : XXGE_MDIO_CFG1_OP_READ_MASK;


    /* Now initiate the set PHY register address operation */

	Address = ((PhyAddress << 24) | (RegisterNum << 16));
	MdioCtrlReg = Address | XXGE_MDIO_CFG1_INITIATE_MASK | mdio_cfg1_op_read_mask;

	GRTNIC_WRITE_REG(hw, (BaseAddr + XXGE_MDIO_CFG1_OFFSET), MdioCtrlReg, 0);
	/*
	 * Wait till MDIO transaction is completed.
	 */
//////////////////////////////////////////////////////////////////////////////////////////////
	MdioCtrlReg = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_MDIO_CFG1_OFFSET), 0);

	TimeoutLoops  = XXGE_MDIO_RDY_LOOPCNT_VAL;
	/* Poll until the reset is done */
	while (TimeoutLoops  && !(MdioCtrlReg & XXGE_MDIO_CFG1_READY_MASK)) {
		MdioCtrlReg = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_MDIO_CFG1_OFFSET), 0);
		TimeoutLoops --;
	} //return somthing

//	if(0 == TimeoutLoops ) printk("Timeout\n");
//////////////////////////////////////////////////////////////////////////////////////////////

	*PhyDataPtr = (u16) GRTNIC_READ_REG(&adapter->hw, (BaseAddr + XXGE_MDIO_RX_DATA_OFFSET), 0);
}


void grtnic_PhyWrite(struct net_device *netdev, u32 PhyAddress, u32 RegisterNum, u16 PhyData)
{
	struct grtnic_adapter *adapter  = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;

	u16 BaseAddr = XXGE_PORT_ADDRBASE;
	u32 Address;
	u32 TimeoutLoops;

	u32 MdioCtrlReg = 0;
    /* Sequence of steps is:
     * - Set Address opcode (CFG1) and actual address (TX Data)
     * - TX Data opcode (CFG1) and actual data to be written (TX Data)
     * - Check for MDIO ready at every step
     */

	/*
	 * Wait till the MDIO interface is ready to accept a new transaction.
	 */
//////////////////////////////////////////////////////////////////////////////////////////////
	MdioCtrlReg = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_MDIO_CFG1_OFFSET), 0);

	TimeoutLoops  = XXGE_MDIO_RDY_LOOPCNT_VAL;
	/* Poll until the reset is done */
	while (TimeoutLoops  && !(MdioCtrlReg & XXGE_MDIO_CFG1_READY_MASK)) {
		MdioCtrlReg = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_MDIO_CFG1_OFFSET), 0);
		TimeoutLoops --;
	} //return somthing

//	if(0 == TimeoutLoops ) printk("Timeout\n");
//////////////////////////////////////////////////////////////////////////////////////////////

    /* Now initiate the set PHY register address operation */
	Address = ((PhyAddress << 24) | (RegisterNum << 16));
	MdioCtrlReg = Address | XXGE_MDIO_CFG1_INITIATE_MASK | XXGE_MDIO_CFG1_OP_WRITE_MASK;

	GRTNIC_WRITE_REG(hw, (BaseAddr + XXGE_MDIO_TX_DATA_OFFSET), (PhyData & XXGE_MDIO_TX_DATA_MASK), 0);
	GRTNIC_WRITE_REG(hw, (BaseAddr + XXGE_MDIO_CFG1_OFFSET), MdioCtrlReg, 0);
	/*
	 * Wait till MDIO transaction is completed.
	 */
//////////////////////////////////////////////////////////////////////////////////////////////
	MdioCtrlReg = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_MDIO_CFG1_OFFSET), 0);

	TimeoutLoops  = XXGE_MDIO_RDY_LOOPCNT_VAL;
	/* Poll until the reset is done */
	while (TimeoutLoops  && !(MdioCtrlReg & XXGE_MDIO_CFG1_READY_MASK)) {
		MdioCtrlReg = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_MDIO_CFG1_OFFSET), 0);
		TimeoutLoops --;
	} //return somthing

//	if(0 == TimeoutLoops ) printk("Timeout\n");
//////////////////////////////////////////////////////////////////////////////////////////////

}

void grtnic_SetPause (struct net_device *netdev, u8 flowctl)
{
	struct grtnic_adapter *adapter  = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;

	u32 RegFc;
	u16 BaseAddr = XXGE_PORT_ADDRBASE;


	RegFc = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_FCC_OFFSET), 0);

	printk("RegFc = %08x, flowctl=%x\n",RegFc, flowctl);
	RegFc &= ~(XXGE_FCC_FCRX_MASK | XXGE_FCC_FCTX_MASK);
	RegFc |= flowctl<<29;

	GRTNIC_WRITE_REG(hw, (BaseAddr + XXGE_FCC_OFFSET), RegFc, 0);
}

void grtnic_set_fc_watermarks (struct net_device *netdev)
{
	struct grtnic_adapter *adapter  = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;

	u32 value = ((ETH_HIGH_MARK << 16) | ETH_LOW_MARK);
	GRTNIC_WRITE_REG(hw, FC_WATERMARK, value, 0);
}

void grtnic_SetMacPauseAddress(struct net_device *netdev, const u8 *AddressPtr)
{
	struct grtnic_adapter *adapter  = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;

	u16 BaseAddr = XXGE_PORT_ADDRBASE;

	u32 MacAddr;
	u8 *Aptr = (u8 *) AddressPtr;

	/* Set the MAC bits [31:0] in RCW0 register */
  MacAddr = Aptr[0];
  MacAddr |= Aptr[1] << 8;
  MacAddr |= Aptr[2] << 16;
  MacAddr |= Aptr[3] << 24;
	GRTNIC_WRITE_REG(hw, (BaseAddr + XXGE_RCW0_OFFSET), MacAddr, 0);

	/* RCW1 contains other info that must be preserved */
	MacAddr = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_RCW1_OFFSET), 0);
	MacAddr &= ~XXGE_RCW1_PAUSEADDR_MASK;
	/* Set MAC bits [47:32] */
	MacAddr |= Aptr[4];
	MacAddr |= Aptr[5] << 8;
	GRTNIC_WRITE_REG(hw, (BaseAddr + XXGE_RCW1_OFFSET), MacAddr, 0);
}

void grtnic_GetMacPauseAddress(struct net_device *netdev, void *AddressPtr)
{
	struct grtnic_adapter *adapter  = netdev_priv(netdev);
	struct grtnic_hw *hw = &adapter->hw;

	u16 BaseAddr = XXGE_PORT_ADDRBASE;

	u32 MacAddr;
	u8 *Aptr = (u8 *) AddressPtr;

	/* Read MAC bits [31:0] in ERXC0 */
	MacAddr = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_RCW0_OFFSET), 0);
	Aptr[0] = (u8) MacAddr;
	Aptr[1] = (u8) (MacAddr >> 8);
	Aptr[2] = (u8) (MacAddr >> 16);
	Aptr[3] = (u8) (MacAddr >> 24);

	/* Read MAC bits [47:32] in RCW1 */
	MacAddr = GRTNIC_READ_REG(hw, (BaseAddr + XXGE_RCW1_OFFSET), 0);
	Aptr[4] = (u8) MacAddr;
	Aptr[5] = (u8) (MacAddr >> 8);
}

u64 grtnic_get_statistics_cnt(struct grtnic_adapter *adapter, u32 reg, u32 old_cnt)
{
	struct grtnic_hw *hw = &adapter->hw;
  u64 new_val;
  u32 temp_val0 = 0; 
  u64 temp_val1 = 0;
  u16 BaseAddr = XXGE_PORT_ADDRBASE;

	temp_val0 = GRTNIC_READ_REG(hw, (BaseAddr + reg), 0);				//low
	temp_val1 = GRTNIC_READ_REG(hw, (BaseAddr + reg + 4), 0);		//hi
	new_val = (temp_val1 << 32) | temp_val0;

  return new_val;
}

/////////////////////////////////////////////////////////////////////////////////
/**
 * grtnic_lower_i2c_clk - Lowers the I2C SCL clock
 * @hw: pointer to hardware structure
 * @i2cctl: Current value of I2CCTL register
 *
 * Lowers the I2C clock line '1'->'0'
 * Asserts the I2C clock output enable on X550 hardware.
 **/
static void grtnic_lower_i2c_clk(struct grtnic_hw *hw, u32 *i2cctl)
{
  DEBUGFUNC("grtnic_lower_i2c_clk");

  *i2cctl &= ~(GRTNIC_I2C_CLK_OUT);

  GRTNIC_WRITE_REG(hw, I2CCTL, *i2cctl, 0);
  GRTNIC_WRITE_FLUSH(hw);

  /* SCL fall time (300ns) */
  usec_delay(GRTNIC_I2C_T_FALL);
}

/**
 * grtnic_raise_i2c_clk - Raises the I2C SCL clock
 * @hw: pointer to hardware structure
 * @i2cctl: Current value of I2CCTL register
 *
 * Raises the I2C clock line '0'->'1'
 * Negates the I2C clock output enable on X550 hardware.
 **/
static void grtnic_raise_i2c_clk(struct grtnic_hw *hw, u32 *i2cctl)
{
  u32 i = 0;
  u32 timeout = GRTNIC_I2C_CLOCK_STRETCHING_TIMEOUT;
  u32 i2cctl_r = 0;

  DEBUGFUNC("grtnic_raise_i2c_clk");

  for (i = 0; i < timeout; i++) {
    *i2cctl |= GRTNIC_I2C_CLK_OUT;

    GRTNIC_WRITE_REG(hw, I2CCTL, *i2cctl, 0);
    GRTNIC_WRITE_FLUSH(hw);
    /* SCL rise time (1000ns) */
    usec_delay(GRTNIC_I2C_T_RISE);

    i2cctl_r = GRTNIC_READ_REG(hw, I2CCTL, 0);
    if (i2cctl_r & GRTNIC_I2C_CLK_IN)
      break;
  }
}

/**
 * grtnic_get_i2c_data - Reads the I2C SDA data bit
 * @hw: pointer to hardware structure
 * @i2cctl: Current value of I2CCTL register
 *
 * Returns the I2C data bit value
 * Negates the I2C data output enable on X550 hardware.
 **/
static bool grtnic_get_i2c_data(struct grtnic_hw *hw, u32 *i2cctl)
{
  bool data;

  DEBUGFUNC("grtnic_get_i2c_data");

  if (*i2cctl & GRTNIC_I2C_DATA_IN)
    data = 1;
  else
    data = 0;

  return data;
}

/**
 * grtnic_set_i2c_data - Sets the I2C data bit
 * @hw: pointer to hardware structure
 * @i2cctl: Current value of I2CCTL register
 * @data: I2C data value (0 or 1) to set
 *
 * Sets the I2C data bit
 * Asserts the I2C data output enable on X550 hardware.
 **/
static s32 grtnic_set_i2c_data(struct grtnic_hw *hw, u32 *i2cctl, bool data)
{
  s32 status = GRTNIC_SUCCESS;

  DEBUGFUNC("grtnic_set_i2c_data");

  if (data)
    *i2cctl |= GRTNIC_I2C_DATA_OUT;
  else
    *i2cctl &= ~(GRTNIC_I2C_DATA_OUT);

  GRTNIC_WRITE_REG(hw, I2CCTL, *i2cctl, 0);
  GRTNIC_WRITE_FLUSH(hw);

  /* Data rise/fall (1000ns/300ns) and set-up time (250ns) */
  usec_delay(GRTNIC_I2C_T_RISE + GRTNIC_I2C_T_FALL + GRTNIC_I2C_T_SU_DATA);

  if (!data)  /* Can't verify data in this case */
    return GRTNIC_SUCCESS;

  /* Verify data was set correctly */
  *i2cctl = GRTNIC_READ_REG(hw, I2CCTL, 0);
  if (data != grtnic_get_i2c_data(hw, i2cctl)) {
    status = GRTNIC_ERR_I2C;
    printk("Error - I2C data was not set to %X.\n", data);
  }

  return status;
}

/**
 * grtnic_clock_in_i2c_bit - Clocks in one bit via I2C data/clock
 * @hw: pointer to hardware structure
 * @data: read data value
 *
 * Clocks in one bit via I2C data/clock
 **/
static void grtnic_clock_in_i2c_bit(struct grtnic_hw *hw, bool *data)
{
  u32 i2cctl = GRTNIC_READ_REG(hw, I2CCTL, 0);

  DEBUGFUNC("grtnic_clock_in_i2c_bit");

  grtnic_raise_i2c_clk(hw, &i2cctl);

  /* Minimum high period of clock is 4us */
  usec_delay(GRTNIC_I2C_T_HIGH);

  i2cctl = GRTNIC_READ_REG(hw, I2CCTL, 0);
  *data = grtnic_get_i2c_data(hw, &i2cctl);

  grtnic_lower_i2c_clk(hw, &i2cctl);

  /* Minimum low period of clock is 4.7 us */
  usec_delay(GRTNIC_I2C_T_LOW);
}

/**
 * grtnic_clock_out_i2c_bit - Clocks in/out one bit via I2C data/clock
 * @hw: pointer to hardware structure
 * @data: data value to write
 *
 * Clocks out one bit via I2C data/clock
 **/
static s32 grtnic_clock_out_i2c_bit(struct grtnic_hw *hw, bool data)
{
  s32 status;
  u32 i2cctl = GRTNIC_READ_REG(hw, I2CCTL, 0);

  DEBUGFUNC("grtnic_clock_out_i2c_bit");

  status = grtnic_set_i2c_data(hw, &i2cctl, data);
  if (status == GRTNIC_SUCCESS) {
    grtnic_raise_i2c_clk(hw, &i2cctl);

    /* Minimum high period of clock is 4us */
    usec_delay(GRTNIC_I2C_T_HIGH);

    grtnic_lower_i2c_clk(hw, &i2cctl);

    /* Minimum low period of clock is 4.7 us.
     * This also takes care of the data hold time.
     */
    usec_delay(GRTNIC_I2C_T_LOW);
  } else {
    status = GRTNIC_ERR_I2C;
    printk("I2C data was not set to %X\n", data);
  }

  return status;
}

/**
 * grtnic_i2c_start - Sets I2C start condition
 * @hw: pointer to hardware structure
 *
 * Sets I2C start condition (High -> Low on SDA while SCL is High)
 * Set bit-bang mode on X550 hardware.
 **/
static void grtnic_i2c_start(struct grtnic_hw *hw)
{
  u32 i2cctl = GRTNIC_READ_REG(hw, I2CCTL, 0);

  DEBUGFUNC("grtnic_i2c_start");

  /* Start condition must begin with data and clock high */
  grtnic_set_i2c_data(hw, &i2cctl, 1);
  grtnic_raise_i2c_clk(hw, &i2cctl);

  /* Setup time for start condition (4.7us) */
  usec_delay(GRTNIC_I2C_T_SU_STA);

  grtnic_set_i2c_data(hw, &i2cctl, 0);

  /* Hold time for start condition (4us) */
  usec_delay(GRTNIC_I2C_T_HD_STA);

  grtnic_lower_i2c_clk(hw, &i2cctl);

  /* Minimum low period of clock is 4.7 us */
  usec_delay(GRTNIC_I2C_T_LOW);

}

/**
 * grtnic_i2c_stop - Sets I2C stop condition
 * @hw: pointer to hardware structure
 *
 * Sets I2C stop condition (Low -> High on SDA while SCL is High)
 * Disables bit-bang mode and negates data output enable on X550
 * hardware.
 **/
static void grtnic_i2c_stop(struct grtnic_hw *hw)
{
  u32 i2cctl = GRTNIC_READ_REG(hw, I2CCTL, 0);

  DEBUGFUNC("grtnic_i2c_stop");

  /* Stop condition must begin with data low and clock high */
  grtnic_set_i2c_data(hw, &i2cctl, 0);
  grtnic_raise_i2c_clk(hw, &i2cctl);

  /* Setup time for stop condition (4us) */
  usec_delay(GRTNIC_I2C_T_SU_STO);

  grtnic_set_i2c_data(hw, &i2cctl, 1);

  /* bus free time between stop and start (4.7us)*/
  usec_delay(GRTNIC_I2C_T_BUF);
}

/**
 * grtnic_clock_in_i2c_byte - Clocks in one byte via I2C
 * @hw: pointer to hardware structure
 * @data: data byte to clock in
 *
 * Clocks in one byte data via I2C data/clock
 **/
static void grtnic_clock_in_i2c_byte(struct grtnic_hw *hw, u8 *data)
{
  s32 i;
  bool bit = 0;

  DEBUGFUNC("grtnic_clock_in_i2c_byte");

  *data = 0;
  for (i = 7; i >= 0; i--) {
    grtnic_clock_in_i2c_bit(hw, &bit);
    *data |= bit << i;
  }
}

/**
 * grtnic_clock_out_i2c_byte - Clocks out one byte via I2C
 * @hw: pointer to hardware structure
 * @data: data byte clocked out
 *
 * Clocks out one byte data via I2C data/clock
 **/
static s32 grtnic_clock_out_i2c_byte(struct grtnic_hw *hw, u8 data)
{
  s32 status = GRTNIC_SUCCESS;
  s32 i;
  u32 i2cctl;
  bool bit;

  DEBUGFUNC("grtnic_clock_out_i2c_byte");

  for (i = 7; i >= 0; i--) {
    bit = (data >> i) & 0x1;
    status = grtnic_clock_out_i2c_bit(hw, bit);

    if (status != GRTNIC_SUCCESS)
      break;
  }

  /* Release SDA line (set high) */
  i2cctl = GRTNIC_READ_REG(hw, I2CCTL, 0);
  i2cctl |= GRTNIC_I2C_DATA_OUT;
  GRTNIC_WRITE_REG(hw, I2CCTL, i2cctl, 0);
  GRTNIC_WRITE_FLUSH(hw);

  return status;
}

/**
 * grtnic_get_i2c_ack - Polls for I2C ACK
 * @hw: pointer to hardware structure
 *
 * Clocks in/out one bit via I2C data/clock
 **/
static s32 grtnic_get_i2c_ack(struct grtnic_hw *hw)
{
  s32 status = GRTNIC_SUCCESS;
  u32 i = 0;
  u32 i2cctl = GRTNIC_READ_REG(hw, I2CCTL, 0);
  u32 timeout = 10;
  bool ack = 1;

  DEBUGFUNC("grtnic_get_i2c_ack");

  grtnic_raise_i2c_clk(hw, &i2cctl);

  /* Minimum high period of clock is 4us */
  usec_delay(GRTNIC_I2C_T_HIGH);

  /* Poll for ACK.  Note that ACK in I2C spec is
   * transition from 1 to 0 */
  for (i = 0; i < timeout; i++) {
    i2cctl = GRTNIC_READ_REG(hw, I2CCTL, 0);
    ack = grtnic_get_i2c_data(hw, &i2cctl);

    usec_delay(1);
    if (!ack)
      break;
  }

  if (ack) {
    printk("I2C ack was not received.\n");
    status = GRTNIC_ERR_I2C;
  }

  grtnic_lower_i2c_clk(hw, &i2cctl);

  /* Minimum low period of clock is 4.7 us */
  usec_delay(GRTNIC_I2C_T_LOW);

  return status;
}

/**
 * grtnic_i2c_bus_clear - Clears the I2C bus
 * @hw: pointer to hardware structure
 *
 * Clears the I2C bus by sending nine clock pulses.
 * Used when data line is stuck low.
 **/
void grtnic_i2c_bus_clear(struct grtnic_hw *hw)
{
  u32 i2cctl;
  u32 i;

  DEBUGFUNC("grtnic_i2c_bus_clear");

  grtnic_i2c_start(hw);
  i2cctl = GRTNIC_READ_REG(hw, I2CCTL, 0);

  grtnic_set_i2c_data(hw, &i2cctl, 1);

  for (i = 0; i < 9; i++) {
    grtnic_raise_i2c_clk(hw, &i2cctl);

    /* Min high period of clock is 4us */
    usec_delay(GRTNIC_I2C_T_HIGH);

    grtnic_lower_i2c_clk(hw, &i2cctl);

    /* Min low period of clock is 4.7us*/
    usec_delay(GRTNIC_I2C_T_LOW);
  }

  grtnic_i2c_start(hw);

  /* Put the i2c bus back to default state */
  grtnic_i2c_stop(hw);
}

/**
 * grtnic_read_i2c_byte_generic_int - Reads 8 bit word over I2C
 * @hw: pointer to hardware structure
 * @byte_offset: byte offset to read
 * @dev_addr: address to read from
 * @data: value read
 * @lock: true if to take and release semaphore
 *
 * Performs byte read operation to SFP module's EEPROM over I2C interface at
 * a specified device address.
 **/
static s32 grtnic_read_i2c_byte_generic_int(struct grtnic_hw *hw, u8 byte_offset,
             u8 dev_addr, u8 *data, bool lock)
{
  s32 status;
  u32 max_retry = 10;
  u32 retry = 0;
  bool nack = 1;
  *data = 0;

  DEBUGFUNC("grtnic_read_i2c_byte_generic");

  do {
    grtnic_i2c_start(hw);

    /* Device Address and write indication */
    status = grtnic_clock_out_i2c_byte(hw, dev_addr);
    if (status != GRTNIC_SUCCESS)
      goto fail;

    status = grtnic_get_i2c_ack(hw);
    if (status != GRTNIC_SUCCESS)
      goto fail;

    status = grtnic_clock_out_i2c_byte(hw, byte_offset);
    if (status != GRTNIC_SUCCESS)
      goto fail;

    status = grtnic_get_i2c_ack(hw);
    if (status != GRTNIC_SUCCESS)
      goto fail;

    grtnic_i2c_start(hw);

    /* Device Address and read indication */
    status = grtnic_clock_out_i2c_byte(hw, (dev_addr | 0x1));
    if (status != GRTNIC_SUCCESS)
      goto fail;

    status = grtnic_get_i2c_ack(hw);
    if (status != GRTNIC_SUCCESS)
      goto fail;

    grtnic_clock_in_i2c_byte(hw, data);

    status = grtnic_clock_out_i2c_bit(hw, nack);
    if (status != GRTNIC_SUCCESS)
      goto fail;

    grtnic_i2c_stop(hw);

    return GRTNIC_SUCCESS;

fail:
    grtnic_i2c_bus_clear(hw);

    if (retry < max_retry)
      printk("I2C byte read error - Retrying.\n");
    else
      printk("I2C byte read error.\n");
    retry++;

  } while (retry <= max_retry);

  return status;
}

/**
 * grtnic_read_i2c_byte_generic - Reads 8 bit word over I2C
 * @hw: pointer to hardware structure
 * @byte_offset: byte offset to read
 * @dev_addr: address to read from
 * @data: value read
 *
 * Performs byte read operation to SFP module's EEPROM over I2C interface at
 * a specified device address.
 **/
s32 grtnic_read_i2c_byte(struct grtnic_hw *hw, u8 byte_offset, u8 dev_addr, u8 *data)
{
  return grtnic_read_i2c_byte_generic_int(hw, byte_offset, dev_addr, data, true);
}

/**
 * grtnic_read_i2c_eeprom_generic - Reads 8 bit EEPROM word over I2C interface
 * @hw: pointer to hardware structure
 * @byte_offset: EEPROM byte offset to read
 * @eeprom_data: value read
 *
 * Performs byte read operation to SFP module's EEPROM over I2C interface.
 **/
s32 grtnic_read_i2c_eeprom(struct grtnic_hw *hw, u8 byte_offset, u8 *eeprom_data)
{
	DEBUGFUNC("grtnic_read_i2c_eeprom_generic");

	return grtnic_read_i2c_byte(hw, byte_offset, GRTNIC_I2C_EEPROM_DEV_ADDR, eeprom_data);
}

/**
 * grtnic_read_i2c_sff8472_generic - Reads 8 bit word over I2C interface
 * @hw: pointer to hardware structure
 * @byte_offset: byte offset at address 0xA2
 * @sff8472_data: value read
 *
 * Performs byte read operation to SFP module's SFF-8472 data over I2C
 **/
s32 grtnic_read_i2c_sff8472(struct grtnic_hw *hw, u8 byte_offset, u8 *sff8472_data)
{
	return grtnic_read_i2c_byte(hw, byte_offset, GRTNIC_I2C_EEPROM_DEV_ADDR2, sff8472_data);
}
