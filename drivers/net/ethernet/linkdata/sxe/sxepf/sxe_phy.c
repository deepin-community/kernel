#include "sxe.h"
#include "sxe_log.h"
#include "sxe_phy.h"
#include "sxe_msg.h"
#include "sxe_netdev.h"
#include "sxe_filter.h"
#include "sxe_version.h"
#include "sxe_host_hdc.h"
#include "sxe_errno.h"

#define SXE_COMPAT_SFP_NUM (sizeof(sfp_vendor_pn_list) / \
				sizeof(sfp_vendor_pn_list[0]))

STATIC u8 sfp_vendor_pn_list[][SXE_SFP_VENDOR_PN_SIZE] = {
	{0x58, 0x50, 0x2d, 0x33, 0x47, 0x31, 0x30, 0x2d, \
		0x31, 0x30, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20},

	{0x58, 0x50, 0x2d, 0x38, 0x47, 0x31, 0x30, 0x2d, \
		0x30, 0x31, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20},

	{0x53, 0x32, 0x31, 0x38, 0x35, 0x2d, 0x30, 0x44, \
		0x33, 0x43, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20},

	{0x59, 0x56, 0x30, 0x32, 0x2d, 0x43, 0x30, 0x31, \
		0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20},

	{0x59, 0x56, 0x30, 0x32, 0x2d, 0x43, 0x30, 0x31, \
		0x2d, 0x30, 0x31, 0x20, 0x20, 0x20, 0x20, 0x20},

	{0x50, 0x39, 0x5a, 0x31, 0x53, 0x4d, 0x41, 0x30, \
		0x31, 0x2d, 0x53, 0x44, 0x2d, 0x54, 0x20, 0x20},

	{0x41, 0x46, 0x43, 0x54, 0x2d, 0x37, 0x33, 0x39, \
		0x53, 0x4d, 0x5a, 0x20, 0x20, 0x20, 0x20, 0x20},

	{0x41, 0x46, 0x42, 0x52, 0x2d, 0x37, 0x30, 0x39, \
		0x44, 0x4d, 0x5a, 0x20, 0x20, 0x20, 0x20, 0x20},

	{0x41, 0x46, 0x42, 0x52, 0x2d, 0x37, 0x31, 0x30, \
		0x53, 0x4d, 0x5a, 0x20, 0x20, 0x20, 0x20, 0x20},

	{0x41, 0x46, 0x43, 0x54, 0x2d, 0x37, 0x33, 0x39, \
		0x44, 0x4d, 0x5a, 0x20, 0x20, 0x20, 0x20, 0x20},

	{0x52, 0x54, 0x58, 0x4d, 0x32, 0x32, 0x38, 0x2d, \
		0x34, 0x30, 0x31, 0x20, 0x20, 0x20, 0x20, 0x20},

	{0x53, 0x46, 0x50, 0x2d, 0x4d, 0x4d, 0x38, 0x35, \
		0x54, 0x47, 0x2d, 0x53, 0x33, 0x44, 0x43, 0x20},

	{0x46, 0x54, 0x4c, 0x58, 0x38, 0x35, 0x37, 0x34, \
		0x44, 0x33, 0x42, 0x43, 0x4c, 0x20, 0x20, 0x20},

	{0x46, 0x54, 0x4c, 0x58, 0x38, 0x35, 0x37, 0x34, \
		0x44, 0x33, 0x42, 0x43, 0x56, 0x20, 0x20, 0x20},

	{0x46, 0x54, 0x4c, 0x58, 0x31, 0x34, 0x37, 0x35, \
		0x44, 0x33, 0x42, 0x43, 0x4c, 0x20, 0x20, 0x20},

	{0x46, 0x43, 0x4c, 0x46, 0x38, 0x35, 0x32, 0x32, \
		0x50, 0x32, 0x42, 0x54, 0x4c, 0x20, 0x20, 0x20},

	{0x46, 0x43, 0x42, 0x47, 0x31, 0x31, 0x30, 0x53, \
		0x44, 0x31, 0x43, 0x30, 0x35, 0x20, 0x20, 0x20},

	{0x41, 0x46, 0x42, 0x52, 0x2d, 0x37, 0x30, 0x39, \
		0x44, 0x4d, 0x5a, 0x2d, 0x49, 0x4e, 0x33, 0x20},
	
	{0x46, 0x43, 0x42, 0x47, 0x31, 0x31, 0x30, 0x53, \
		0x44, 0x31, 0x43, 0x30, 0x35, 0x20, 0x20, 0x20},

	{0x58, 0x50, 0x41, 0x43, 0x2d, 0x38, 0x47, 0x31, \
		0x30, 0x2d, 0x30, 0x35, 0x20, 0x20, 0x20, 0x20},
	
	{0x58, 0x50, 0x44, 0x43, 0x2d, 0x47, 0x31, 0x30, \
		0x2d, 0x30, 0x32, 0x20, 0x20, 0x20, 0x20, 0x20},
	
	{0x52, 0x54, 0x58, 0x4d, 0x32, 0x32, 0x38, 0x2d, \
		0x35, 0x35, 0x31, 0x20, 0x20, 0x20, 0x20, 0x20},
};

#ifdef SXE_PHY_CONFIGURE
int sxe_mdio_read(struct net_device *netdev, int prtad, int devad, u16 addr)
{
	s32 ret;
	u16 value;
	s32 regnum = addr;
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_hw *hw = &adapter->hw;

	if (adapter->phy_ctxt.phy_info.mii_bus) {
		if (devad != MDIO_DEVAD_NONE) {
			regnum |= (devad << SXE_DEVAD_SHIFT) | MII_ADDR_C45;
		}

		ret = mdiobus_read(adapter->phy_ctxt.phy_info.mii_bus, prtad, regnum);
		goto l_end;
	}

	if (prtad != adapter->phy_ctxt.phy_info.mdio.prtad) {
		LOG_ERROR_BDF("not the current phy, prted=%d, current prted=%d\n",
				prtad, adapter->phy_ctxt.phy_info.mdio.prtad);
		ret = -EINVAL;
		goto l_end;
	}

	ret = hw->phy.ops->reg_read(hw, prtad, addr, devad, &value);
	ret = value;

l_end:
	return ret;
}

int sxe_mdio_write(struct net_device *netdev, int prtad, int devad,
			    u16 addr, u16 value)
{
	s32 ret;
	u32 regnum = addr;
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_hw *hw = &adapter->hw;

	if (adapter->phy_ctxt.phy_info.mii_bus) {
		if (devad != MDIO_DEVAD_NONE) {
			regnum |= (devad << SXE_DEVAD_SHIFT) | MII_ADDR_C45;
		}

		ret = mdiobus_write(adapter->phy_ctxt.phy_info.mii_bus,
					prtad, regnum, value);
		if (ret) {
			LOG_ERROR_BDF("mdio write failed, prtad=%d, ret=%d\n",
					prtad, ret);
			goto l_end;
		}
	}

	if (prtad != adapter->phy_ctxt.phy_info.mdio.prtad) {
		LOG_ERROR_BDF("not the current phy, prted=%d, current prted=%d\n",
				prtad, adapter->phy_ctxt.phy_info.mdio.prtad);
		ret = -EINVAL;
		goto l_end;
	}

	ret = hw->phy.ops->reg_write(hw, prtad, addr, devad, value);

l_end:
	return ret;
}

static s32 sxe_mii_bus_read(struct mii_bus *bus, int addr, int regnum)
{
	s32 ret;
	u16 value;
	u32 device_type, reg_addr;
	struct sxe_adapter *adapter = bus->priv;
	struct sxe_hw *hw = &adapter->hw;

	reg_addr = regnum & GENMASK(15, 0);
	device_type = (regnum & GENMASK(16,20) >> SXE_MII_DEV_TYPE_SHIFT);

	ret = hw->phy.ops->reg_read(hw, addr,
				reg_addr, device_type, &value);
	if (ret) {
		LOG_ERROR_BDF("mii read failed, reg_addr=%d, device_type=%d, prtad=%d\n",
				reg_addr, device_type, addr);
		ret = -EBUSY;
		goto l_end;
	}

	ret = value;

l_end:
	return ret;
}

static s32 sxe_mii_bus_write(struct mii_bus *bus, int addr, int regnum,
			       u16 val)
{
	s32 ret;
	u32 reg_addr;
	u32 device_type;
	struct sxe_adapter *adapter = bus->priv;
	struct sxe_hw *hw = &adapter->hw;

	reg_addr = regnum & GENMASK(15, 0);
	device_type = (regnum & GENMASK(16,20) >> SXE_MII_DEV_TYPE_SHIFT);

	ret = hw->phy.ops->reg_write(hw, addr,
				reg_addr, device_type, val);
	if (ret) {
		LOG_ERROR_BDF("mii write failed, reg_addr=%d, "
			"device_type=%d, prtad=%d, val=%u\n",
				reg_addr, device_type, addr, val);
		ret = -EBUSY;
	}

	return ret;
}

static u32 sxe_get_phy_type_from_id(u32 phy_id)
{
	u32 phy_type;

	switch (phy_id) {
	case SXE_MARVELL_88X3310_PHY_ID:
		phy_type = SXE_PHY_MARVELL_88X3310;
		break;
	default:
		phy_type = SXE_PHY_UNKNOWN;
	}

	return phy_type;
}

static bool sxe_phy_probe(struct sxe_adapter *adapter, u16 phy_addr)
{
	s32 ret;
	u16 ext_ability = 0;
	struct sxe_phy_info *phy = &adapter->phy_ctxt.phy_info;
	struct sxe_hw *hw = &adapter->hw;

	phy->mdio.prtad = phy_addr;
	if (mdio45_probe(&phy->mdio, phy_addr) != 0) {
		ret = -SXE_ERR_PHY_NOT_PERSENT;
		LOG_WARN("mdio probe failed\n");
		goto l_end;
	}

	ret = hw->phy.ops->identifier_get(hw, phy_addr, &phy->id);
	if (ret) {
		LOG_ERROR_BDF("get phy id failed, prtad=%d\n", phy_addr);
		goto l_end;
	}

	phy->type = sxe_get_phy_type_from_id(phy->id);
	if (SXE_PHY_UNKNOWN == phy->type) {
		ret = hw->phy.ops->reg_read(hw, phy_addr, MDIO_PMA_EXTABLE,
				     MDIO_MMD_PMAPMD, &ext_ability);
		if (ret) {
			LOG_ERROR_BDF("get phy extended ability failed, prtad=%d\n",
				phy_addr);
			goto l_end;
		}

		if (ext_ability & \
			(MDIO_PMA_EXTABLE_10GBT | MDIO_PMA_EXTABLE_1000BT)) {
			phy->type = SXE_PHY_CU_UNKNOWN;
		} else {
			phy->type = SXE_PHY_GENERIC;
		}
	}

l_end:
	return ret;
}

s32 sxe_phy_identify(struct sxe_adapter *adapter)
{
	s32 ret;
	u32 phy_addr;
	struct sxe_phy_info *phy = &adapter->phy_ctxt.phy_info;

	for (phy_addr = 0; phy_addr < SXE_PHY_ADDR_MAX; phy_addr++) {
		ret = sxe_phy_probe(adapter, phy_addr);
		if (!ret) {
			LOG_INFO_BDF("phy probe seccess, prtad=%d, phy_type=%d\n",
				phy->mdio.prtad, phy->type);
			goto l_end;
		}
	}

	phy->mdio.prtad = MDIO_PRTAD_NONE;
	ret = -SXE_ERR_PHY_NOT_PERSENT;

l_end:
	return ret;
}

void sxe_phy_link_capabilities_get(struct sxe_adapter *adapter, u32 *speed,
								bool *autoneg)
{
	s32 ret = 0;
	struct sxe_hw *hw = &adapter->hw;

	*autoneg = true;

	if (adapter->phy_ctxt.speed !=0) {
		*speed = adapter->phy_ctxt.speed;
		goto l_end;
	}

	ret = adapter->hw.phy.ops->link_cap_get(hw,
			adapter->phy_ctxt.phy_info.mdio.prtad, speed);
	if (ret) {
		LOG_ERROR_BDF("get link speed cap=%d\n", *speed);
		goto l_end;
	}

	LOG_INFO_BDF("phy link speed cap=%d\n", *speed);

l_end:
	return ret;
}

s32 sxe_phy_reset(struct sxe_adapter *adapter)
{
	s32 ret;
	struct sxe_hw *hw = &adapter->hw;
	struct sxe_phy_context *phy = &adapter->phy_ctxt;

	if (phy->phy_info.type == SXE_PHY_UNKNOWN) {
		ret = phy->ops->identify(adapter);
		if (ret) {
			LOG_ERROR_BDF("phy identify failed, ret=%d\n", ret);
			goto l_end;
		}
	}

	ret = hw->phy.ops->reset(hw, phy->phy_info.mdio.prtad);
	if (ret) {
		LOG_ERROR_BDF("phy reset failed, ret=%d\n", ret);
	}

l_end:
	return ret;
}

static s32 sxe_phy_link_autoneg_configure(struct sxe_adapter *adapter)
{
	s32 ret;
	u32 speed;
	u16  autoneg_reg ;
	bool autoneg = false;
	struct sxe_hw *hw = &adapter->hw;
	struct sxe_phy_info *phy = &adapter->phy_ctxt.phy_info;

	sxe_phy_link_capabilities_get(adapter, &speed, &autoneg);

	ret = hw->phy.ops->reg_read(hw, phy->mdio.prtad, MDIO_AN_10GBT_CTRL,
				MDIO_MMD_AN, &autoneg_reg);
	if (ret) {
		LOG_ERROR_BDF("get speed 10gb reg failed, ret=%d\n", ret);
	}

	autoneg_reg &= ~MDIO_AN_10GBT_CTRL_ADV10G;
	if ((adapter->phy_ctxt.autoneg_advertised & SXE_LINK_SPEED_10GB_FULL) &&
	    (speed & SXE_LINK_SPEED_10GB_FULL)) {
		autoneg_reg |= MDIO_AN_10GBT_CTRL_ADV10G;
	}

	hw->phy.ops->reg_write(hw,  phy->mdio.prtad, MDIO_AN_10GBT_CTRL,
				MDIO_MMD_AN, autoneg_reg);

	ret = hw->phy.ops->reg_read(hw, phy->mdio.prtad, MDIO_AN_ADVERTISE,
					MDIO_MMD_AN, &autoneg_reg);
	autoneg_reg &= ~(ADVERTISE_100FULL | ADVERTISE_100HALF);
	if ((adapter->phy_ctxt.autoneg_advertised & SXE_LINK_SPEED_10GB_FULL) &&
	    (speed & SXE_LINK_SPEED_10GB_FULL)) {
		autoneg_reg |= ADVERTISE_100FULL;
	}

	hw->phy.ops->reg_write(hw, phy->mdio.prtad, MDIO_AN_ADVERTISE, 			MDIO_MMD_AN, autoneg_reg);

	ret = hw->phy.ops->reg_read(hw, phy->mdio.prtad, MDIO_CTRL1,
			     MDIO_MMD_AN, &autoneg_reg);
	autoneg_reg |= MDIO_AN_CTRL1_RESTART;
	hw->phy.ops->reg_write(hw, phy->mdio.prtad, MDIO_CTRL1,
			      MDIO_MMD_AN, autoneg_reg);
l_end:
	return ret;
}

s32 sxe_phy_link_speed_configure(struct sxe_adapter *adapter, u32 speed)
{
	s32 ret;
	struct sxe_phy_context *phy = &adapter->phy_ctxt;

	phy->autoneg_advertised = 0;

	if (speed & SXE_LINK_SPEED_10GB_FULL) {
		phy->autoneg_advertised |= SXE_LINK_SPEED_10GB_FULL;
	}

	if (speed & SXE_LINK_SPEED_1GB_FULL) {
		phy->autoneg_advertised |= SXE_LINK_SPEED_1GB_FULL;
	}

	if (speed & SXE_LINK_SPEED_100_FULL) {
		phy->autoneg_advertised |= SXE_LINK_SPEED_100_FULL;
	}

	if (speed & SXE_LINK_SPEED_10_FULL) {
		phy->autoneg_advertised |= SXE_LINK_SPEED_10_FULL;
	}

	ret = sxe_phy_link_autoneg_configure(adapter);
	if (ret) {
		LOG_ERROR_BDF("phy autoneg config failed, ret=%d\n", ret);
	}

	return ret;
}

s32 sxe_mdiobus_init(struct sxe_adapter *adapter)
{
	s32 ret;
	struct mii_bus *bus;
	struct mdio_if_info *mdio = &adapter->phy_ctxt.phy_info.mdio;
	struct pci_dev *pdev = adapter->pdev;
	struct net_device *netdev = adapter->netdev;
	struct device *dev = &netdev->dev;

	mdio->prtad = MDIO_PRTAD_NONE;
	mdio->mmds = 0;
	mdio->mode_support = MDIO_SUPPORTS_C45;
	mdio->dev = netdev;
	mdio->mdio_read = sxe_mdio_read;
	mdio->mdio_write = sxe_mdio_write;

	bus = devm_mdiobus_alloc(dev);
	if (!bus) {
		LOG_ERROR_BDF("mdio bus alloc failed\n");
		ret = -ENOMEM;
		goto l_end;
	}

	bus->read = &sxe_mii_bus_read;
	bus->write = &sxe_mii_bus_write;

	snprintf(bus->id, MII_BUS_ID_SIZE, "%s-mdio-%s", SXE_DRV_NAME,
		pci_name(pdev));

	bus->name = "sxe-mdio";
	bus->priv = adapter;
	bus->parent = dev;
	bus->phy_mask = GENMASK(31, 0);

	ret = mdiobus_register(bus);
	if (ret) {
		LOG_ERROR_BDF("mdio bus register failed, ret=%d\n", ret);
		goto l_free;
	}

	LOG_INFO_BDF("phy init ok\n");
	adapter->phy_ctxt.phy_info.mii_bus = bus;
	return 0;

l_free:
	devm_mdiobus_free(dev, bus);
l_end:
	return ret;
}

void sxe_mdiobus_exit(struct sxe_adapter *adapter)
{
	if (adapter->phy_ctxt.phy_info.mii_bus) {
		mdiobus_unregister(adapter->phy_ctxt.phy_info.mii_bus);
	}

	return;
}
#endif 

s32 sxe_sfp_eeprom_read(struct sxe_adapter *adapter, u16 offset,
					u16 len, u8 *data)
{
	s32 ret;
	struct sxe_driver_cmd cmd;
	struct sxe_sfp_rw_req req;
	struct sxe_sfp_read_resp *resp;
	u16 resp_len = sizeof(struct sxe_sfp_read_resp) + len;
	struct sxe_hw *hw = &adapter->hw;

	if (!data) {
		ret = -EINVAL;
		LOG_ERROR_BDF("sfp read buff == NULL\n");
		goto l_end;
	}

	if (len > SXE_SFP_EEPROM_SIZE_MAX) {
		ret = -EINVAL;
		LOG_ERROR_BDF("sfp read size[%u] > eeprom max size[%d], ret=%d\n",
					len, SXE_SFP_EEPROM_SIZE_MAX, ret);
		goto l_end;
	}

	LOG_INFO_BDF("sfp read, offset=%u, len=%u\n", offset, len);

	req.len = len;
	req.offset = offset;

	resp = kmalloc(resp_len, GFP_KERNEL);
	if (!resp) {
		ret = -ENOMEM;
		LOG_ERROR_BDF("sfp read, alloc resp mem failed\n");
		goto l_end;
	}

	cmd.req      = &req;
	cmd.req_len  = sizeof(struct sxe_sfp_rw_req);
	cmd.resp     = resp;
	cmd.resp_len = resp_len;
	cmd.trace_id = 0;
	cmd.opcode   = SXE_CMD_SFP_READ;
	cmd.is_interruptible = true;
	ret = sxe_driver_cmd_trans(hw, &cmd);
	if (ret) {
		LOG_ERROR_BDF("sfp read, hdc failed, offset=%u, len=%u, ret=%d\n",
					offset, len, ret);
		ret = -EIO;
		goto l_free;
	}

	if (resp->len != len) {
		ret = -EIO;
		LOG_ERROR_BDF("sfp read failed, offset=%u, len=%u\n", offset, len);
		goto l_free;
	}

	memcpy(data, resp->resp, len);

l_free:
	kfree(resp);

l_end:
	return ret;
}

s32 sxe_sfp_eeprom_write(struct sxe_adapter *adapter, u16 offset,
					u32 len, u8 *data)
{
	s32 ret;
	struct sxe_driver_cmd cmd;
	struct sxe_sfp_rw_req *req;
	u16 req_len  = sizeof(struct sxe_sfp_rw_req) + len;
	struct sxe_hw *hw = &adapter->hw;

	if (!data) {
		ret = -EINVAL;
		LOG_ERROR_BDF("sfp write data == NULL\n");
		goto l_end;
	}

	if (len > SXE_SFP_EEPROM_SIZE_MAX) {
		ret = -EINVAL;
		LOG_ERROR_BDF("sfp write size[%u] > eeprom max size[%d], ret=%d\n",
					len, SXE_SFP_EEPROM_SIZE_MAX, ret);
		goto l_end;
	}

	LOG_INFO_BDF("sfp write, offset=%u, len=%u\n", offset, len);

	req = kmalloc(req_len, GFP_KERNEL);
	if (!req) {
		ret = -ENOMEM;
		LOG_ERROR_BDF("sfp write, alloc req mem failed\n");
		goto l_end;
	}

	req->len = len;
	req->offset = offset;
	memcpy(req->write_data, data, len);

	cmd.req      = req;
	cmd.req_len  = req_len;
	cmd.resp     = NULL;
	cmd.resp_len = 0;
	cmd.trace_id = 0;
	cmd.opcode   = SXE_CMD_SFP_WRITE;
	cmd.is_interruptible = true;
	ret = sxe_driver_cmd_trans(hw, &cmd);
	if (ret) {
		LOG_ERROR_BDF("sfp write failed, offset=%u, len=%u, ret=%d\n",
					offset, len, ret);
	}

	kfree(req);

l_end:
	return ret;
}

static s32 sxe_sfp_tx_laser_ctrl(struct sxe_adapter *adapter, bool is_disable)
{
	s32 ret;
	struct sxe_driver_cmd cmd;
	sxe_spp_tx_able_s laser_disable;
	struct sxe_hw *hw = &adapter->hw;

	laser_disable.isDisable = is_disable;
	LOG_INFO_BDF("sfp tx laser ctrl start, is_disable=%x\n", is_disable);

	cmd.req              = &laser_disable;
	cmd.req_len          = sizeof(laser_disable);
	cmd.resp             = NULL;
	cmd.resp_len         = 0;
	cmd.trace_id         = 0;
	cmd.opcode           = SXE_CMD_TX_DIS_CTRL;
	cmd.is_interruptible = true;
	ret = sxe_driver_cmd_trans(hw, &cmd);
	if (ret) {
		LOG_ERROR_BDF("sfp tx laser ctrl failed, ret=%d\n", ret);
		goto l_end;
	}

	LOG_INFO_BDF("sfp tx laser ctrl success, is_disable=%x\n", is_disable);

l_end:
	return ret;
}

static void sxe_sfp_tx_laser_enable(struct sxe_adapter *adapter)
{
	sxe_sfp_tx_laser_ctrl(adapter, false);

	return;
}

void sxe_sfp_tx_laser_disable(struct sxe_adapter *adapter)
{
	sxe_sfp_tx_laser_ctrl(adapter, true);

	return;
}

s32 sxe_sfp_reset(struct sxe_adapter *adapter)
{

	LOG_INFO_BDF("auto_restart:%u.\n", adapter->hw.mac.auto_restart);

	if (adapter->hw.mac.auto_restart) {
		sxe_sfp_tx_laser_disable(adapter);
		sxe_sfp_tx_laser_enable(adapter);
		adapter->hw.mac.auto_restart = false;
	}

	return 0;
}

static void sxe_sfp_link_capabilities_get(struct sxe_adapter *adapter, u32 *speed,
							bool *autoneg)
{
	struct sxe_sfp_info *sfp = &adapter->phy_ctxt.sfp_info;

	*speed = 0;

	if (sfp->type == SXE_SFP_TYPE_1G_CU ||
	    sfp->type == SXE_SFP_TYPE_1G_SXLX ) {
		*speed = SXE_LINK_SPEED_1GB_FULL;
		*autoneg = true;
		goto l_end;
	}

	*speed = SXE_LINK_SPEED_10GB_FULL;
	*autoneg = false;

	if (sfp->multispeed_fiber) {
		*speed |= SXE_LINK_SPEED_10GB_FULL | SXE_LINK_SPEED_1GB_FULL;
		*autoneg = true;
	}

l_end:
	LOG_INFO_BDF("sfp link speed cap=%d\n", *speed);
	return;
}

static s32 sxe_sfp_rate_select(struct sxe_adapter *adapter, sxe_sfp_rate_e rate)
{
	s32 ret;
	struct sxe_driver_cmd cmd;
	sxe_sfp_rate_able_s rate_able;
	struct sxe_hw *hw = &adapter->hw;

	rate_able.rate = rate;
	LOG_INFO_BDF("sfp tx rate select start, rate=%d\n", rate);

	cmd.req      = &rate_able;
	cmd.req_len  = sizeof(rate_able);
	cmd.resp     = NULL;
	cmd.resp_len = 0;
	cmd.trace_id = 0;
	cmd.opcode   = SXE_CMD_RATE_SELECT;
	cmd.is_interruptible = true;
	ret = sxe_driver_cmd_trans(hw, &cmd);
	if (ret) {
		LOG_ERROR_BDF("sfp rate select failed, ret=%d\n", ret);
	}

	LOG_INFO_BDF("sfp tx rate select end, rate=%d\n", rate);

	return ret;
}

s32 sxe_pcs_sds_init(struct sxe_adapter *adapter, sxe_pcs_mode_e mode,
			u32 max_frame)
{
	s32 ret;
	sxe_pcs_cfg_s pcs_cfg;
	struct sxe_driver_cmd cmd;
	struct sxe_hw *hw = &adapter->hw;

	pcs_cfg.mode = mode;
	pcs_cfg.mtu  = max_frame;

	cmd.req      = &pcs_cfg;
	cmd.req_len  = sizeof(pcs_cfg);
	cmd.resp     = NULL;
	cmd.resp_len = 0;
	cmd.trace_id = 0;
	cmd.opcode   = SXE_CMD_PCS_SDS_INIT;
	cmd.is_interruptible = true;
	sxe_sfp_tx_laser_disable(adapter);
	ret = sxe_driver_cmd_trans(hw, &cmd);
	sxe_sfp_tx_laser_enable(adapter);
	if (ret) {
		LOG_ERROR_BDF("hdc trans failed ret=%d, cmd:pcs init\n", ret);
		goto l_end;
	}

	sxe_fc_mac_addr_set(adapter);

	LOG_INFO_BDF("mode:%u pcs sds init done.\n", mode);

l_end:
	return ret;
}

s32 sxe_multispeed_sfp_link_configure(struct sxe_adapter *adapter, u32 speed)
{
	s32 ret = 0;
	bool autoneg, link_up;
	u32 i, speed_cap, link_speed, speedcnt = 0;
	struct sxe_hw *hw = &adapter->hw;
	u32 highest_link_speed = SXE_LINK_SPEED_UNKNOWN;
	u32 max_frame = sxe_sw_mtu_get(adapter);

	sxe_sfp_link_capabilities_get(adapter, &speed_cap, &autoneg);

	speed &= speed_cap;

	sxe_link_info_get(adapter, &link_speed, &link_up);
	if ((link_up == true) && (speed & link_speed)) {
		LOG_INFO_BDF("link cfg dont changed , dont need cfp pcs,"
			"speed=%x, mtu=%u\n", speed, max_frame);
		goto l_end;
	}

	if (speed & SXE_LINK_SPEED_10GB_FULL) {
		LOG_INFO_BDF("10G link cfg start\n");

		speedcnt++;
		highest_link_speed = SXE_LINK_SPEED_10GB_FULL;

		ret = sxe_sfp_rate_select(adapter, SXE_SFP_RATE_10G);
		if (ret) {
			LOG_ERROR_BDF("set sfp rate failed, ret=%d\n", ret);
			goto l_end;
		}

		msleep(SXE_RATE_SEL_WAIT);

		ret = sxe_pcs_sds_init(adapter, SXE_PCS_MODE_10GBASE_KR_WO,
						max_frame);
		if (ret) {
			goto l_end;
		}


		for (i = 0; i < SXE_LINK_UP_RETRY_CNT; i++) {
			msleep(SXE_LINK_UP_RETRY_ITR);

			sxe_link_info_get(adapter, &link_speed, &link_up);
			if (link_up) {
				LOG_INFO_BDF("link cfg end, link up, speed is 10G\n");
				goto l_out;
			}
		}

		LOG_WARN_BDF("10G link cfg failed, retry...\n");
	}

	if (speed & SXE_LINK_SPEED_1GB_FULL) {
		LOG_INFO_BDF("1G link cfg start\n");

		speedcnt++;
		if (highest_link_speed == SXE_LINK_SPEED_UNKNOWN) {
			highest_link_speed = SXE_LINK_SPEED_1GB_FULL;
		}

		ret = sxe_sfp_rate_select(adapter, SXE_SFP_RATE_1G);
		if (ret) {
			LOG_ERROR_BDF("set sfp rate failed, ret=%d\n", ret);
			goto l_end;
		}

		msleep(SXE_RATE_SEL_WAIT);

		ret = sxe_pcs_sds_init(adapter, SXE_PCS_MODE_1000BASE_KX_W,
						max_frame);
		if (ret) {
			goto l_end;
		}

		msleep(SXE_SFP_RESET_WAIT);

		link_up = hw->mac.ops->link_up_1g_check(hw);
		if (link_up) {
			LOG_INFO_BDF("link cfg end, link up, speed is 1G\n");
			goto l_out;
		}

		LOG_WARN_BDF("1G link cfg failed, retry...\n");
	}

	if (speedcnt > 1) {
		ret = sxe_multispeed_sfp_link_configure(adapter, highest_link_speed);
	}
l_out:

	adapter->phy_ctxt.autoneg_advertised = 0;

	if (speed & SXE_LINK_SPEED_10GB_FULL) {
		adapter->phy_ctxt.autoneg_advertised |= SXE_LINK_SPEED_10GB_FULL;
	}

	if (speed & SXE_LINK_SPEED_1GB_FULL) {
		adapter->phy_ctxt.autoneg_advertised |= SXE_LINK_SPEED_1GB_FULL;
	}

l_end:
	return ret;
}

void sxe_link_info_get(struct sxe_adapter *adapter, u32 *link_speed, bool *link_up)
{
	struct sxe_hw *hw = &adapter->hw;

	*link_up = hw->mac.ops->link_state_is_up(hw);
	if (false == *link_up) {
		LOG_INFO_BDF("link state =%d, (1=link_up, 0=link_down)\n",
								*link_up);
		*link_speed = SXE_LINK_SPEED_UNKNOWN;
	} else {
		*link_speed = hw->mac.ops->link_speed_get(hw);
	}

	return;
}

STATIC s32 sxe_an_cap_get(struct sxe_adapter *adapter, sxe_an_cap_s *an_cap)
{
	s32 ret;
	struct sxe_driver_cmd cmd;
	struct sxe_hw *hw = &adapter->hw;

	cmd.req      = NULL;
	cmd.req_len  = 0;
	cmd.resp     = an_cap;
	cmd.resp_len = sizeof(*an_cap);
	cmd.trace_id = 0;
	cmd.opcode   = SXE_CMD_AN_CAP_GET;
	cmd.is_interruptible = true;
	ret = sxe_driver_cmd_trans(hw, &cmd);
	if (ret) {
		LOG_ERROR_BDF("hdc trans failed ret=%d, cmd:negotiation cap get\n", ret);
	}

	return ret;
}

static s32 sxe_sfp_fc_autoneg(struct sxe_adapter *adapter)
{
	s32 ret;
	sxe_an_cap_s an_cap;
	struct sxe_hw *hw = &adapter->hw;

	ret = sxe_an_cap_get(adapter, &an_cap);
	if (ret) {
		LOG_ERROR_BDF("get auto negotiate capacity failed, ret=%d\n", ret);
		goto l_end;
	}

	if ((an_cap.local.pause_cap & SXE_PAUSE_CAP_SYMMETRIC_PAUSE) &&
		(an_cap.peer.pause_cap & SXE_PAUSE_CAP_SYMMETRIC_PAUSE)) {
		if (hw->fc.requested_mode == SXE_FC_FULL) {
			hw->fc.current_mode = SXE_FC_FULL;
			LOG_DEV_DEBUG("flow control = full.\n");
		} else {
			hw->fc.current_mode = SXE_FC_RX_PAUSE;
			LOG_DEV_DEBUG("flow control=RX PAUSE frames only\n");
		}
	} else if ((an_cap.local.pause_cap == SXE_PAUSE_CAP_ASYMMETRIC_PAUSE) &&
		(an_cap.peer.pause_cap == SXE_PAUSE_CAP_BOTH_PAUSE)) {
		hw->fc.current_mode = SXE_FC_TX_PAUSE;
		LOG_DEV_DEBUG("flow control = TX PAUSE frames only.\n");
	} else if ((an_cap.local.pause_cap == SXE_PAUSE_CAP_BOTH_PAUSE) &&
		(an_cap.peer.pause_cap == SXE_PAUSE_CAP_ASYMMETRIC_PAUSE)) {
		hw->fc.current_mode = SXE_FC_RX_PAUSE;
		LOG_DEV_DEBUG("flow control = RX PAUSE frames only.\n");
	} else {
		hw->fc.current_mode = SXE_FC_NONE;
		LOG_DEV_DEBUG("flow control = none.\n");
	}
	hw->fc.requested_mode = hw->fc.current_mode;

l_end:
	return ret;
}

static void sxe_fc_autoneg(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;

	s32 ret = -SXE_ERR_FC_NOT_NEGOTIATED;
	bool link_up;
	u32 link_speed;
	if (hw->fc.disable_fc_autoneg) {
		LOG_INFO_BDF("disable fc autoneg\n");
		goto l_end;
	}

	sxe_link_info_get(adapter, &link_speed, &link_up);
	if (!link_up) {
		LOG_INFO_BDF("link down, dont fc autoneg\n");
		goto l_end;
	}

	ret = sxe_sfp_fc_autoneg(adapter);
l_end:
	if (ret) {
		hw->fc.current_mode = hw->fc.requested_mode;
	}

	return;
}

void sxe_fc_enable(struct sxe_adapter *adapter)
{
	s32 ret;
	u32 i;
	struct sxe_hw *hw = &adapter->hw;

	if (!hw->fc.pause_time) {
		LOG_ERROR_BDF("link fc disabled since pause time is 0\n");
		hw->fc.requested_mode = hw->fc.current_mode;
		ret = -SXE_ERR_INVALID_LINK_SETTINGS;
		goto l_end;
	}

	for (i = 0; i < MAX_TRAFFIC_CLASS; i++) {
		if ((hw->fc.current_mode & SXE_FC_TX_PAUSE) &&
		    hw->fc.high_water[i]) {
			if (!hw->fc.low_water[i] ||
			    hw->fc.low_water[i] >= hw->fc.high_water[i]) {
				LOG_DEV_DEBUG("invalid water mark configuration, "
					"tc[%u] low_water=%u, high_water=%u\n",
					i, hw->fc.low_water[i],
					hw->fc.high_water[i]);
				hw->fc.requested_mode = hw->fc.current_mode;
				ret = -SXE_ERR_INVALID_LINK_SETTINGS;
				goto l_end;
			}
		}
	}

	sxe_fc_autoneg(adapter);

	ret = hw->mac.ops->fc_enable(hw);
	if (ret) {
		LOG_ERROR_BDF("link fc enable failed, ret=%d\n", ret);
	}

l_end:
	return;
}

static s32 sxe_sfp_link_configure(struct sxe_adapter *adapter, u32 speed)
{
	s32 ret = 0;
	bool an;
	bool link_up;
	u32 link_speed;
	u32 pcs_mode = SXE_PCS_MODE_BUTT;
	u32 max_frame = sxe_sw_mtu_get(adapter);

	sxe_sfp_link_capabilities_get(adapter, &speed, &an);

	if (SXE_LINK_SPEED_1GB_FULL == speed) {
		pcs_mode = SXE_PCS_MODE_1000BASE_KX_W;
		adapter->phy_ctxt.autoneg_advertised = SXE_LINK_SPEED_1GB_FULL;
	} else if (SXE_LINK_SPEED_10GB_FULL == speed) {
		pcs_mode = SXE_PCS_MODE_10GBASE_KR_WO;
		adapter->phy_ctxt.autoneg_advertised = SXE_LINK_SPEED_10GB_FULL;
	}

	sxe_link_info_get(adapter, &link_speed, &link_up);
	if ((link_up == true) && (link_speed == speed)) {
		LOG_INFO_BDF("link cfg dont changed , dont need cfp pcs,"
			"speed=%x, mtu=%u\n", speed, max_frame);
		goto l_end;
	}

	ret = sxe_pcs_sds_init(adapter, pcs_mode, max_frame);
	if (ret) {
		LOG_ERROR_BDF("pcs sds init failed, ret=%d\n", ret);
	}

	LOG_INFO_BDF("link :cfg speed=%x, pcs_mode=%x, atuoreg=%d, mtu=%u\n",
			speed, pcs_mode, an, max_frame);

l_end:
	return ret;
}

s32 sxe_link_configure(struct sxe_adapter *adapter, u32 speed)
{
	s32 ret;

	if (adapter->phy_ctxt.sfp_info.multispeed_fiber) {
		ret = sxe_multispeed_sfp_link_configure(adapter, speed);
	} else {
		ret = sxe_sfp_link_configure(adapter, speed);
	}

	return ret;
}

s32 sxe_sfp_vendor_pn_cmp(u8 *sfp_vendor_pn) {
	s32 ret = -EINVAL;
	u32 i;
	for (i = 0; i < SXE_COMPAT_SFP_NUM; i++) {
		ret = memcmp(sfp_vendor_pn, sfp_vendor_pn_list[i],
			SXE_SFP_VENDOR_PN_SIZE);
		if (0 == ret) {
			goto l_end;
		}
	}

l_end:
	return ret;
}

s32 sxe_sfp_identify(struct sxe_adapter *adapter)
{
	s32 ret;
	enum sxe_sfp_type sfp_type;
	u8 sfp_comp_code[SXE_SFP_COMP_CODE_SIZE];
	struct sxe_sfp_info *sfp = &adapter->phy_ctxt.sfp_info;

	LOG_INFO_BDF("sfp identify start\n");

	ret = sxe_sfp_eeprom_read(adapter, SXE_SFF_BASE_ADDR,
				SXE_SFP_COMP_CODE_SIZE, sfp_comp_code);
	if (ret) {
		sfp_type = SXE_SFP_TYPE_NOT_PRESENT;
		LOG_ERROR_BDF("get sfp identifier failed, ret=%d\n", ret);
		goto l_end;
	}

	LOG_INFO_BDF("sfp identifier=%x, cable_technology=%x, "
			"10GB_code=%x, 1GB_code=%x\n",
		sfp_comp_code[SXE_SFF_IDENTIFIER],
		sfp_comp_code[SXE_SFF_CABLE_TECHNOLOGY],
		sfp_comp_code[SXE_SFF_10GBE_COMP_CODES],
		sfp_comp_code[SXE_SFF_1GBE_COMP_CODES]);

	if (sfp_comp_code[SXE_SFF_IDENTIFIER] != SXE_SFF_IDENTIFIER_SFP) {
		LOG_WARN("sfp type get failed, offset=%d, type=%x\n",
			SXE_SFF_IDENTIFIER, sfp_comp_code[SXE_SFF_IDENTIFIER]);
		sfp_type = SXE_SFP_TYPE_UNKNOWN;
		ret = -SXE_ERR_SFF_NOT_SUPPORTED;
		goto l_end;
	}

	if (sfp_comp_code[SXE_SFF_CABLE_TECHNOLOGY] & SXE_SFF_DA_PASSIVE_CABLE) {
		sfp_type = SXE_SFP_TYPE_DA_CU;
	}  else if (sfp_comp_code[SXE_SFF_10GBE_COMP_CODES] & \
		(SXE_SFF_10GBASESR_CAPABLE | SXE_SFF_10GBASELR_CAPABLE)) {
		sfp_type = SXE_SFP_TYPE_SRLR;
	} else if (sfp_comp_code[SXE_SFF_1GBE_COMP_CODES] & \
		SXE_SFF_1GBASET_CAPABLE) {
		sfp_type = SXE_SFP_TYPE_1G_CU;
	} else if ((sfp_comp_code[SXE_SFF_1GBE_COMP_CODES] & \
		SXE_SFF_1GBASESX_CAPABLE) || \
		(sfp_comp_code[SXE_SFF_1GBE_COMP_CODES] & \
		SXE_SFF_1GBASELX_CAPABLE)) {
		sfp_type = SXE_SFP_TYPE_1G_SXLX;
	} else {
		sfp_type = SXE_SFP_TYPE_UNKNOWN;
	}

	sfp->multispeed_fiber = false;

	if (((sfp_comp_code[SXE_SFF_1GBE_COMP_CODES] & \
			SXE_SFF_1GBASESX_CAPABLE) &&
		(sfp_comp_code[SXE_SFF_10GBE_COMP_CODES] & \
			SXE_SFF_10GBASESR_CAPABLE)) ||
		((sfp_comp_code[SXE_SFF_1GBE_COMP_CODES] & \
			SXE_SFF_1GBASELX_CAPABLE) &&
		(sfp_comp_code[SXE_SFF_10GBE_COMP_CODES] & \
			SXE_SFF_10GBASELR_CAPABLE))) {
		sfp->multispeed_fiber = true;
		LOG_INFO_BDF("identify sfp, sfp is multispeed\n");
	}

	LOG_INFO_BDF("identify sfp, sfp_type=%d, is_multispeed=%x\n",
			sfp_type, sfp->multispeed_fiber);

l_end:
	adapter->phy_ctxt.sfp_info.type = sfp_type;
	return ret;
}

struct sxe_phy_ops phy_ops[SXE_PHY_MAX] =
{
	{sxe_sfp_identify,
	sxe_sfp_link_configure,
	sxe_sfp_link_capabilities_get,
	sxe_sfp_reset,
	sxe_sfp_tx_laser_disable,
	sxe_sfp_tx_laser_enable},
#ifdef SXE_PHY_CONFIGURE
	{sxe_phy_identify,
	sxe_phy_link_speed_configure,
	sxe_phy_link_capabilities_get,
	sxe_phy_reset,
	NULL,
	NULL},
#endif
};

enum sxe_media_type sxe_media_type_get(struct sxe_adapter *adapter)
{
	enum sxe_media_type type = SXE_MEDIA_TYPE_UNKWON;

	type = SXE_MEDIA_TYPE_FIBER;
	adapter->phy_ctxt.is_sfp = true;

	return type;
}

s32 sxe_phy_init(struct sxe_adapter *adapter)
{
	s32 ret;
	enum sxe_media_type media_type = sxe_media_type_get(adapter);

	switch (media_type) {
	case SXE_MEDIA_TYPE_FIBER:
		adapter->phy_ctxt.ops = &phy_ops[SXE_SFP_IDX];
		break;
#ifdef SXE_PHY_CONFIGURE
	case SXE_MEDIA_TYPE_COPPER:
		adapter->phy_ctxt.ops = &phy_ops[SXE_PHY_MARVELL_88X3310_idx];
		break;
#endif
	default:
		LOG_DEV_ERR("other media type are not adapted\n, media_type=%d",
					media_type);
		break;
	}

	ret = adapter->phy_ctxt.ops->identify(adapter);
	if (ret) {
		LOG_ERROR_BDF("phy identify failed, ret=%d\n", ret);
	}

	return ret;
}
