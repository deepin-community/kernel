#ifndef __SXE_PHY_H__
#define __SXE_PHY_H__

#include <linux/types.h>
#include <linux/netdevice.h>
#include "sxe_host_cli.h"
#include "sxe_cli.h"
#ifdef SXE_PHY_CONFIGURE
#include <linux/phy.h>
#include <linux/mdio.h>
#endif

#define SXE_DEV_ID_FIBER  0
#define SXE_DEV_ID_COPPER 1

#define SXE_SFF_BASE_ADDR			0x0
#define SXE_SFF_IDENTIFIER			0x0  
#define SXE_SFF_10GBE_COMP_CODES		0x3  
#define SXE_SFF_1GBE_COMP_CODES			0x6  
#define SXE_SFF_CABLE_TECHNOLOGY		0x8  
#define SXE_SFF_VENDOR_PN			0x28 
#define SXE_SFF_8472_DIAG_MONITOR_TYPE		0x5C 
#define SXE_SFF_8472_COMPLIANCE			0x5E 

#define SXE_SFF_IDENTIFIER_SFP			0x3
#define SXE_SFF_ADDRESSING_MODE			0x4  
#define SXE_SFF_8472_UNSUP			0x0
#define SXE_SFF_DDM_IMPLEMENTED			0x40 
#define SXE_SFF_DA_PASSIVE_CABLE		0x4
#define SXE_SFF_DA_ACTIVE_CABLE			0x8
#define SXE_SFF_DA_SPEC_ACTIVE_LIMITING		0x4
#define SXE_SFF_1GBASESX_CAPABLE		0x1
#define SXE_SFF_1GBASELX_CAPABLE		0x2
#define SXE_SFF_1GBASET_CAPABLE			0x8
#define SXE_SFF_10GBASESR_CAPABLE		0x10
#define SXE_SFF_10GBASELR_CAPABLE		0x20

#define SXE_SFP_COMP_CODE_SIZE			10  
#define SXE_SFP_VENDOR_PN_SIZE			16  
#define SXE_SFP_EEPROM_SIZE_MAX			512 

#define SXE_SW_SFP_LOS_DELAY_MS		200

#define SXE_SW_SFP_MULTI_GB_MS		4000

#define SXE_PHY_ADDR_MAX			32
#define SXE_MARVELL_88X3310_PHY_ID		0x2002B

#define SXE_RATE_SEL_WAIT       (40)
#define SXE_LINK_UP_RETRY_CNT   (5)
#define SXE_LINK_UP_RETRY_ITR   (100)
#define SXE_SFP_RESET_WAIT      (100)

#define SXE_DEVAD_SHIFT         (16)
#define SXE_MII_DEV_TYPE_SHIFT  (16)

#define SXE_LINK_SPEED_MBPS_10G		10000
#define SXE_LINK_SPEED_MBPS_1G		1000
#define SXE_LINK_SPEED_MBPS_100		100
#define SXE_LINK_SPEED_MBPS_10		10

struct sxe_adapter;

enum sxe_media_type {
	SXE_MEDIA_TYPE_UNKWON = 0,
	SXE_MEDIA_TYPE_FIBER  = 1, 
	SXE_MEDIA_TYPE_COPPER = 2, 
};

enum sxe_phy_idx {
	SXE_SFP_IDX = 0,
	SXE_PHY_MARVELL_88X3310_idx,
	SXE_PHY_MAX,
};

enum sxe_phy_type {
	SXE_PHY_MARVELL_88X3310,  
	SXE_PHY_GENERIC,          
	SXE_PHY_CU_UNKNOWN,       
	SXE_PHY_UNKNOWN,          
};

enum sxe_sfp_type {
	SXE_SFP_TYPE_DA_CU       = 0, 
	SXE_SFP_TYPE_SRLR        = 1, 
	SXE_SFP_TYPE_1G_CU       = 2, 
	SXE_SFP_TYPE_1G_SXLX     = 4, 
	SXE_SFP_TYPE_NOT_PRESENT = 5, 
	SXE_SFP_TYPE_UNKNOWN     = 0xFFFF ,
};

struct sxe_phy_ops {
	s32  (*identify)(struct sxe_adapter *adapter);
	s32  (*link_configure)(struct sxe_adapter *adapter, u32 speed);
	void (*get_link_capabilities)(struct sxe_adapter *adapter, u32 *speed,
							bool *autoneg);
	s32  (*reset)(struct sxe_adapter *adapter);
	void (*sfp_tx_laser_disable)(struct sxe_adapter *adapter);
	void (*sfp_tx_laser_enable)(struct sxe_adapter *adapter);
};

#ifdef SXE_PHY_CONFIGURE
struct sxe_phy_info {
	u32			id;		
	bool			autoneg;	
	struct mii_bus		*mii_bus;	
	enum sxe_phy_type	type;		
	struct mdio_if_info	mdio;		
};
#endif 

struct sxe_sfp_info {
	enum sxe_sfp_type	type;
	bool			multispeed_fiber; 
};

struct sxe_phy_context {
	bool is_sfp;                  
	u32  speed;                   
	u32  autoneg_advertised;      
	struct sxe_phy_ops *ops;      
#ifdef SXE_PHY_CONFIGURE
	struct sxe_phy_info phy_info; 
#endif
	struct sxe_sfp_info sfp_info; 
};

s32 sxe_phy_init(struct sxe_adapter *adapter);

#ifdef SXE_PHY_CONFIGURE
int sxe_mdio_read(struct net_device *netdev, int prtad, int devad, u16 addr);

int sxe_mdio_write(struct net_device *netdev, int prtad, int devad,
			    u16 addr, u16 value);

s32 sxe_phy_identify(struct sxe_adapter *adapter);

void sxe_phy_link_capabilities_get(struct sxe_adapter *adapter, u32 *speed,
							bool *autoneg);

s32 sxe_phy_link_speed_configure(struct sxe_adapter *adapter, u32 speed);

s32 sxe_mdiobus_init(struct sxe_adapter *adapter);

void sxe_mdiobus_exit(struct sxe_adapter *adapter);

s32 sxe_phy_reset(struct sxe_adapter *adapter);
#endif 

enum sxe_media_type sxe_media_type_get(struct sxe_adapter *adapter);

static inline bool sxe_is_sfp(struct sxe_adapter *adapter)
{
	return (sxe_media_type_get(adapter) == SXE_MEDIA_TYPE_FIBER) ? \
							true : false;
}

s32 sxe_sfp_eeprom_read(struct sxe_adapter *adapter, u16 offset,
					u16 len, u8 *data);

s32 sxe_sfp_eeprom_write(struct sxe_adapter *adapter, u16 offset,
					u32 len, u8 *data);

enum sxe_media_type sxe_media_type_get(struct sxe_adapter *adapter);

void sxe_sfp_tx_laser_disable(struct sxe_adapter *adapter);

s32 sxe_sfp_vendor_pn_cmp(u8 *sfp_vendor_pn);

s32 sxe_sfp_identify(struct sxe_adapter *adapter);

s32 sxe_link_configure(struct sxe_adapter *adapter, u32 speed);

s32 sxe_sfp_reset(struct sxe_adapter *adapter);

void sxe_link_info_get(struct sxe_adapter *adapter, u32 *link_speed, bool *link_up);

s32 sxe_pcs_sds_init(struct sxe_adapter *adapter, sxe_pcs_mode_e mode, 
			u32 max_frame);

void sxe_fc_enable(struct sxe_adapter *adapter);

#endif 