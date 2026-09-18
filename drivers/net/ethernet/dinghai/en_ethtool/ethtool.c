#include <linux/dinghai/driver.h>
#ifndef CGS_V5_693
#include <linux/compiler_types.h>
#endif
#include <linux/netdevice.h>
#include <linux/ethtool.h>
#include <linux/types.h>
#include <linux/bitmap.h>
#include "../slib.h"
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/file.h>
#include <linux/path.h>
#include <linux/dcache.h>
#include <linux/namei.h>
#include <linux/mount.h>

#if defined(HAVE_VFS_MKDIR_NO_IDMAP)
#include <linux/mnt_idmapping.h>
#endif

#include "../en_aux/queue.h"
#include "../en_aux.h"
#include "../en_aux/en_aux_cmd.h"
#include "../en_np/table/include/dpp_tbl_api.h"
#include "ethtool.h"
#include "linux/dinghai/dh_cmd.h"
#include "../msg_common.h"
#include "../bonding/rdma_ops.h"
#include "../bonding/zxdh_lag.h"
#include "../en_aux/dcbnl/en_dcbnl_api.h"
#include "../en_aux/dcbnl/en_dcbnl.h"
#include "../en_aux/queue.h"
#include "../en_pf/msg_func.h"
#include "en_aux/en_aux_events.h"

MODULE_LICENSE("Dual BSD/GPL");

#define DRV_NAME "dinghai10e"
#define ETHTOOL_LINK_MODE_MASK_MAX_KERNEL_NBITS 32
#define VERSION_PART_LEN 15
#define BUILD_PART_LEN 5
#define MAX_DRV_NAME_LEN 32
#define MAX_DRV_VERSION_LEN 32
#define ZXDH_DEFAULT_MAX_COALESCED_FRAMES 32
#define PCI_BUS(PCI_BDF) ((PCI_BDF >> 8) & 0xff)

#define ZXDH_EN_LINK_MODE_ADD(ks, name, sup)                             \
do                                                                       \
{                                                                        \
    if (sup)                                                             \
    {                                                                    \
        ethtool_link_ksettings_add_link_mode((ks), supported, name);     \
    }                                                                    \
    else                                                                 \
    {                                                                    \
        ethtool_link_ksettings_add_link_mode((ks), advertising, name);   \
    }                                                                    \
} while (0)

#define ZXDH_EN_SPEED_MODE_TO_ETHTOOL(en_dev, bit, sup)              \
    sup ? ((en_dev->supported_speed_modes) & BIT(bit)) == BIT(bit) : \
    ((en_dev->advertising_speed_modes) & BIT(bit)) == BIT(bit)

bool enable_1588_debug = false;
#ifdef PTP_DRIVER_INTERFACE_EN
extern int32_t zxdh_get_ptp_clock_index(struct zxdh_en_device *en_dev, uint32_t *ptp_clock_idx);
#endif /* PTP_DRIVER_INTERFACE_EN */

#ifndef CGS_V5_693

#define GET_FEC_LINK_FLAG  (0)
#define GET_FEC_CFG_FLAG   (1)
#define GET_FEC_CAP_FLAG   (2)

extern int32_t zxdh_get_ptp_clock_index(struct zxdh_en_device *en_dev, uint32_t *ptp_clock_idx);
static const uint32_t fec_2_ethtool_fecparam[] =
{
    [SPM_FEC_NONE] = ETHTOOL_FEC_OFF,
    [SPM_FEC_BASER] = ETHTOOL_FEC_BASER,
    [SPM_FEC_RS528] = ETHTOOL_FEC_RS,
    [SPM_FEC_RS544] = ETHTOOL_FEC_RS,
};

static uint32_t zxdh_en_fec_to_ethtool_fecparam(uint32_t fec_mode, uint32_t flag)
{
    int32_t i;
    uint32_t fecparam_cap = 0;

    if(!fec_mode)
    {
        if(flag == GET_FEC_LINK_FLAG)
            return ETHTOOL_FEC_NONE;
        else if(flag == GET_FEC_CFG_FLAG)
            return ETHTOOL_FEC_AUTO;
    }

    for(i = 0; i < ARRAY_SIZE(fec_2_ethtool_fecparam); i++)
    {
        if(fec_mode & BIT(i))
        {
            fecparam_cap |= fec_2_ethtool_fecparam[i];
        }
    }

    if(flag == GET_FEC_CAP_FLAG)
        fecparam_cap |= ETHTOOL_FEC_AUTO;

    return fecparam_cap;
}

static void zxdh_en_fec_to_link_ksettings(uint32_t fec_mode,
                                          struct ethtool_link_ksettings *ks,
                                          bool sup)
{
    if(fec_mode & BIT(SPM_FEC_NONE))
        ZXDH_EN_LINK_MODE_ADD(ks, FEC_NONE, sup);
    if(fec_mode & BIT(SPM_FEC_BASER))
        ZXDH_EN_LINK_MODE_ADD(ks, FEC_BASER, sup);
    if(fec_mode & BIT(SPM_FEC_RS528) ||
       fec_mode & BIT(SPM_FEC_RS544))
        ZXDH_EN_LINK_MODE_ADD(ks, FEC_RS, sup);
}

static void zxdh_en_fec_link_ksettings_get(struct zxdh_en_device *en_dev,
                                              struct ethtool_link_ksettings *ks)
{
    int32_t ret;
    uint32_t fec_cap;
    uint32_t fec_active;

    ret = zxdh_en_fec_mode_get(en_dev, &fec_cap, NULL, &fec_active);
    if(ret)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_en_fec_mode_get failed!\n");
        return;
    }
    //LOG_INFO("fec_cap=0x%x, fec_active=0x%x\n", fec_cap, fec_active);

    zxdh_en_fec_to_link_ksettings(fec_cap, ks, true);
    zxdh_en_fec_to_link_ksettings(fec_active, ks, false);

    return;
}
#endif

static void zxdh_en_pause_link_ksettings_get(struct zxdh_en_device *en_dev,
                                             struct ethtool_link_ksettings *ks)
{
    int32_t err;
    uint32_t fc_mode;

    err = zxdh_en_fc_mode_get(en_dev, &fc_mode);
    if(err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_en_fc_mode_get failed!\n");
        return;
    }

    ZXDH_EN_LINK_MODE_ADD(ks, Pause, true);

    if(fc_mode == BIT(SPM_FC_PAUSE_FULL))
        ZXDH_EN_LINK_MODE_ADD(ks, Pause, false);
    else if(fc_mode == BIT(SPM_FC_PAUSE_RX) || fc_mode == BIT(SPM_FC_PAUSE_TX))
        ZXDH_EN_LINK_MODE_ADD(ks, Asym_Pause, false);

    return;
}

static void zxdh_en_phytype_to_ethtool(struct zxdh_en_device *en_dev, struct ethtool_link_ksettings *ks, bool sup)
{
    //0x20000020020
    if (ZXDH_EN_SPEED_MODE_TO_ETHTOOL(en_dev, SPM_SPEED_1X_1G, sup))
    {
        ZXDH_EN_LINK_MODE_ADD(ks, 1000baseT_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 1000baseKX_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 1000baseX_Full, sup);
    }

    //0x5C0000081000
    if (ZXDH_EN_SPEED_MODE_TO_ETHTOOL(en_dev, SPM_SPEED_1X_10G, sup))
    {
        ZXDH_EN_LINK_MODE_ADD(ks, 10000baseT_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 10000baseKR_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 10000baseCR_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 10000baseSR_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 10000baseLR_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 10000baseER_Full, sup);
    }

    //0x380000000
    if (ZXDH_EN_SPEED_MODE_TO_ETHTOOL(en_dev, SPM_SPEED_1X_25G, sup))
    {
        ZXDH_EN_LINK_MODE_ADD(ks, 25000baseCR_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 25000baseKR_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 25000baseSR_Full, sup);
    }

    //0x10C00000000
    if (ZXDH_EN_SPEED_MODE_TO_ETHTOOL(en_dev, SPM_SPEED_1X_50G, sup))
    {
        ZXDH_EN_LINK_MODE_ADD(ks, 50000baseCR2_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 50000baseKR2_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 50000baseSR2_Full, sup);
    }

    //0x7800000
    if (ZXDH_EN_SPEED_MODE_TO_ETHTOOL(en_dev, SPM_SPEED_4X_40G, sup))
    {
        ZXDH_EN_LINK_MODE_ADD(ks, 40000baseKR4_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 40000baseCR4_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 40000baseSR4_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 40000baseLR4_Full, sup);
    }

    //0xF000000000
    if (ZXDH_EN_SPEED_MODE_TO_ETHTOOL(en_dev, SPM_SPEED_4X_100G, sup))
    {
        ZXDH_EN_LINK_MODE_ADD(ks, 100000baseKR4_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 100000baseSR4_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 100000baseCR4_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 100000baseLR4_ER4_Full, sup);
    }

#ifndef NEED_XARRAY
    //0x1E00000000000000
    if (ZXDH_EN_SPEED_MODE_TO_ETHTOOL(en_dev, SPM_SPEED_2X_100G, sup))
    {
        ZXDH_EN_LINK_MODE_ADD(ks, 100000baseKR2_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 100000baseSR2_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 100000baseCR2_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 100000baseLR2_ER2_FR2_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 100000baseCR2_Full, sup);
    }

    //0x5C000000000000000
    if (ZXDH_EN_SPEED_MODE_TO_ETHTOOL(en_dev, SPM_SPEED_4X_200G, sup))
    {
        ZXDH_EN_LINK_MODE_ADD(ks, 200000baseKR4_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 200000baseSR4_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 200000baseCR4_Full, sup);
        ZXDH_EN_LINK_MODE_ADD(ks, 200000baseLR4_ER4_FR4_Full, sup);
    }
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,5,0))
    if (ZXDH_EN_SPEED_MODE_TO_ETHTOOL(en_dev, SPM_SPEED_8X_400G, sup))
    {
        ZXDH_EN_LINK_MODE_ADD(ks, 400000baseSR8_Full, sup);
    }
#endif
    return;
}

static void zxdh_en_ethtool_to_phytype(struct ethtool_link_ksettings *ks, uint32_t *speed_modes)
{
    if (ethtool_link_ksettings_test_link_mode(ks, advertising, 1000baseT_Full))
    {
        *speed_modes |= BIT(SPM_SPEED_1X_1G);
    }

    if (ethtool_link_ksettings_test_link_mode(ks, advertising, 10000baseT_Full))
    {
        *speed_modes |= BIT(SPM_SPEED_1X_10G);
    }

    if (ethtool_link_ksettings_test_link_mode(ks, advertising, 25000baseCR_Full))
    {
        *speed_modes |= BIT(SPM_SPEED_1X_25G);
    }

    if (ethtool_link_ksettings_test_link_mode(ks, advertising, 50000baseCR2_Full))
    {
        *speed_modes |= BIT(SPM_SPEED_1X_50G);
    }

    if (ethtool_link_ksettings_test_link_mode(ks, advertising, 40000baseKR4_Full))
    {
        *speed_modes |= BIT(SPM_SPEED_4X_40G);
    }

    if (ethtool_link_ksettings_test_link_mode(ks, advertising, 100000baseKR4_Full))
    {
        *speed_modes |= BIT(SPM_SPEED_4X_100G);
    }

#ifndef NEED_XARRAY
    if (ethtool_link_ksettings_test_link_mode(ks, advertising, 100000baseKR2_Full))
    {
        *speed_modes |= BIT(SPM_SPEED_2X_100G);
    }

    if (ethtool_link_ksettings_test_link_mode(ks, advertising, 200000baseKR4_Full))
    {
        *speed_modes |= BIT(SPM_SPEED_4X_200G);
    }
#endif

    return;
}

static int32_t zxdh_en_speed_to_speed_modes(uint32_t speed, uint32_t *speed_modes, uint32_t sup_modes)
{
    switch (speed)
    {
        case SPEED_1000:
        {
            *speed_modes |= BIT(SPM_SPEED_1X_1G);
            break;
        }
        case SPEED_10000:
        {
            *speed_modes |= BIT(SPM_SPEED_1X_10G);
            break;
        }
        case SPEED_25000:
        {
            *speed_modes |= BIT(SPM_SPEED_1X_25G);
            break;
        }
        case SPEED_40000:
        {
            *speed_modes |= BIT(SPM_SPEED_4X_40G);
            break;
        }
        case SPEED_50000:
        {
            *speed_modes |= BIT(SPM_SPEED_1X_50G);
            break;
        }
        case SPEED_100000:
        {
            *speed_modes |= BIT(SPM_SPEED_2X_100G);
            *speed_modes |= BIT(SPM_SPEED_4X_100G);
            break;
        }
        case SPEED_200000:
        {
            *speed_modes |= BIT(SPM_SPEED_4X_200G);
            break;
        }
        default:
        {
            return -EINVAL;
        }
    }

    *speed_modes &= sup_modes;
    if (*speed_modes == 0)
    {
        return -EINVAL;
    }

    return 0;
}

static uint8_t zxdh_en_get_module_eeprom_contype(struct zxdh_en_device *en_dev)
{
    uint32_t read_bytes;
    uint8_t data;
    uint8_t identifier;
    struct zxdh_en_module_eeprom_param query = {0};

    if((en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF))
    {
        return PORT_OTHER;
    }

    query.i2c_addr = SFF_I2C_ADDRESS_LOW;
    query.bank = 0;
    query.page = 0;
    query.offset = SFF_I2C_ETH_IDENTIFIER;
    query.length = 1;
    read_bytes = zxdh_en_module_eeprom_read(en_dev, &query, &identifier);
    if(read_bytes != query.length)
    {
        return PORT_OTHER;
    }
    if (identifier == ZXDH_MODULE_ID_QSFP ||
        identifier == ZXDH_MODULE_ID_QSFP_PLUS ||
        identifier == ZXDH_MODULE_ID_QSFP28) {
        query.offset = SFF8636_I2C_ETH_COMPLIANCE;
        read_bytes = zxdh_en_module_eeprom_read(en_dev, &query, &data);
        if(read_bytes != query.length)
        {
            return PORT_OTHER;
        }
        if (data & SFF8636_ETHERNET_40G_CR4) {
            return PORT_DA;
        }
        if (data & SFF8636_ETHERNET_RSRVD) {
            query.offset = SFF8636_I2C_ETH_COMPLIANCE_EXTEND;
            read_bytes = zxdh_en_module_eeprom_read(en_dev, &query, &data);
            if(read_bytes != query.length)
            {
                return PORT_OTHER;
            }
            switch (data) {
            case SFF8636_ETHERNET_100G_CR4:
            case SFF8636_ETHERNET_25G_CR_CA_S:
            case SFF8636_ETHERNET_25G_CR_CA_N:
            case SFF8636_ETHERNET_200G_CR4:
                return PORT_DA;
            }
        }
    } else if (identifier == ZXDH_MODULE_ID_SFP) {
        query.offset = SFF8472_I2C_ETH_TRANSCEIVER;
        read_bytes = zxdh_en_module_eeprom_read(en_dev, &query, &data);
        if(read_bytes != query.length)
        {
            return PORT_OTHER;
        }

        if (data == SFF8472_ETHERNET_25G_CR) {
            return PORT_DA;
        }
    } else if (identifier == ZXDH_MODULE_ID_QSFP_PLUS_WITH_CMIS) {
        query.offset = CMIS_MEDIA_INTF_TECH_OFFSET;
        read_bytes = zxdh_en_module_eeprom_read(en_dev, &query, &data);
        if(read_bytes != query.length)
        {
            return PORT_OTHER;
        }
        switch (data) {
            case CMIS_COPPER_UNEQUAL:
            case CMIS_COPPER_PASS_EQUAL:
            case CMIS_COPPER_NF_EQUAL:
            case CMIS_COPPER_F_EQUA:
            case CMIS_COPPER_N_EQUAL:
            case CMIS_COPPER_LINEAR_EQUAL:
                return PORT_DA;
        }
    }

    return PORT_FIBRE;
}

static int32_t zxdh_en_get_link_ksettings(struct net_device *netdev,
                                struct ethtool_link_ksettings *ks)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint8_t contype;

    ethtool_link_ksettings_zero_link_mode(ks, supported);
    ethtool_link_ksettings_zero_link_mode(ks, advertising);

    contype = zxdh_en_get_module_eeprom_contype(en_dev);
    ks->base.port = contype;
    ks->base.autoneg = en_dev->autoneg_enable;
    ethtool_link_ksettings_add_link_mode(ks, supported, FIBRE);
    ethtool_link_ksettings_add_link_mode(ks, supported, Autoneg);

    if (en_dev->autoneg_enable == AUTONEG_ENABLE)
    {
        ethtool_link_ksettings_add_link_mode(ks, advertising, Autoneg);
    }

    if (en_dev->speed == 0 &&
        en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_ROCE_RDMA) {
        ks->base.speed = SPEED_200000;
    } else {
        ks->base.speed = en_dev->speed;
    }

    if ((!netif_running(netdev)) || (!netif_carrier_ok(netdev)))
    {
        ks->base.speed = SPEED_UNKNOWN;
    }
    ks->base.duplex = ks->base.speed == SPEED_UNKNOWN ? DUPLEX_UNKNOWN : DUPLEX_FULL;

    zxdh_en_phytype_to_ethtool(en_dev, ks, true);
    zxdh_en_phytype_to_ethtool(en_dev, ks, false);

    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF && zxdh_en_is_panel_port(en_dev))
    {
#ifndef CGS_V5_693
        zxdh_en_fec_link_ksettings_get(en_dev, ks);
#endif
        zxdh_en_pause_link_ksettings_get(en_dev, ks);
    }

    return 0;
}

static int32_t zxdh_en_set_link_ksettings(struct net_device *netdev,
                                    const struct ethtool_link_ksettings *ks)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct ethtool_link_ksettings safe_ks;
    uint32_t advertising_link_modes = 0;
    uint32_t off_speed_modes = 0;
    uint32_t on_speed_modes = 0;
    int32_t err = 0;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    if((en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF) ||
       (!zxdh_en_is_panel_port(en_dev)))
    {
        return -EOPNOTSUPP;
    }

    if (ks->base.duplex == DUPLEX_HALF)
    {
        return -ENAVAIL;
    }

    memset(&safe_ks, 0, sizeof(safe_ks));
    ethtool_link_ksettings_zero_link_mode(&safe_ks, supported);
    ethtool_link_ksettings_zero_link_mode(&safe_ks, advertising);

    if (ks->base.autoneg == AUTONEG_DISABLE)
    {
        err = zxdh_en_speed_to_speed_modes(ks->base.speed, &off_speed_modes,
                                        en_dev->supported_speed_modes);
        LOG_DEBUG_DEV(en_dev->parent, "set speed: %d, off_speed_modes: 0x%x\n", ks->base.speed, off_speed_modes);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_en_speed_to_speed_mode failed: %d\n", err);
            return -EOPNOTSUPP;
        }

        advertising_link_modes = off_speed_modes;
    }
    else
    {
        zxdh_en_phytype_to_ethtool(en_dev, &safe_ks, true);
        if (!bitmap_intersects(ks->link_modes.advertising,
                    safe_ks.link_modes.supported, __ETHTOOL_LINK_MODE_MASK_NBITS))
        {
            LOG_ERR_DEV(en_dev->parent, "link_mode not supported\n");
            return -EOPNOTSUPP;
        }

        bitmap_and(safe_ks.link_modes.advertising, ks->link_modes.advertising,
                    safe_ks.link_modes.supported, __ETHTOOL_LINK_MODE_MASK_NBITS);
        zxdh_en_ethtool_to_phytype(&safe_ks, &on_speed_modes);
        LOG_DEBUG_DEV(en_dev->parent, "on_speed_modes: 0x%x\n", on_speed_modes);
        advertising_link_modes = on_speed_modes;
    }

    if ((advertising_link_modes == en_dev->advertising_speed_modes) &&
        (ks->base.autoneg == en_dev->autoneg_enable))
    {
        LOG_DEBUG_DEV(en_dev->parent, "nothing changed\n");
        return 0;
    }

    safe_ks.base.speed = en_dev->speed;
    en_dev->speed = SPEED_UNKNOWN;
    LOG_INFO_DEV(en_dev->parent, "autoneg %d, link_modes: 0x%x\n", ks->base.autoneg, advertising_link_modes);
    err = zxdh_en_autoneg_set(en_dev, ks->base.autoneg, advertising_link_modes);
    if (err != 0)
    {
        en_dev->speed = safe_ks.base.speed;
        LOG_ERR_DEV(en_dev->parent, "zxdh_en_autoneg_set failed: %d\n", err);
        return err;
    }
    else
    {
        en_dev->autoneg_enable = ks->base.autoneg;
        en_dev->advertising_speed_modes = advertising_link_modes;
        en_dev->link_up = false;
        netif_carrier_off(netdev);
        en_dev->ops->set_pf_link_up(en_dev->parent, FALSE); //TODO:是否需要更新pf信息？
        queue_work(en_priv->events->wq, &en_priv->edev.vf_link_info_update_work);
        queue_work(en_priv->events->wq, &en_priv->edev.link_info_irq_update_np_work);
    }

    return err;
}

static uint32_t zxdh_en_get_link(struct net_device *netdev)
{
    return netif_carrier_ok(netdev) ? 1 : 0;
}

static int zxdh_en_get_eeprom_len(struct net_device *netdev)
{
    return 0;
}

static int zxdh_en_get_eeprom(struct net_device *netdev, struct ethtool_eeprom *eeprom, u8 *bytes)
{
    return 0;
}

static int zxdh_en_set_eeprom(struct net_device *netdev, struct ethtool_eeprom *eeprom, u8 *bytes)
{
    return 0;
}

#ifdef HAVE_ETHTOOL_RING_PARAM
static void zxdh_en_get_ringparam(struct net_device *netdev, struct ethtool_ringparam *param, struct kernel_ethtool_ringparam *kernel_ring, struct netlink_ext_ack *ack)
#else
static void zxdh_en_get_ringparam(struct net_device *netdev, struct ethtool_ringparam *param)
#endif
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    param->rx_max_pending = ZXDH_PF_MAX_DESC_NUM(en_dev);
    param->tx_max_pending = ZXDH_PF_MAX_DESC_NUM(en_dev);
    param->rx_pending = en_dev->eth_config.rx_queue_size;
    param->tx_pending = en_dev->eth_config.tx_queue_size;

    return;
}

static int32_t zxdh_phy_vq_reset(struct zxdh_en_device *en_dev)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    int32_t i = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    msg->vqm_msg.opcode = OPCODE_SET;
    msg->vqm_msg.cmd = OVS_VQM_CTRL_RESET_QIDS;
    msg->vqm_msg.qid_reset_msg.version = ZXDH_VNET_ZTE;
    msg->vqm_msg.qid_reset_msg.qnum = en_dev->max_queue_pairs * 2;
    for (i = 0; i < en_dev->max_queue_pairs * 2; ++i) {
        msg->vqm_msg.qid_reset_msg.qid[i] = en_dev->phy_index[i];
    }
    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_CFG_VQM, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "send cfg msix mode msg to riscv failed\n");
    }
    kfree(msg);
    return err;
}

#ifdef HAVE_ETHTOOL_RING_PARAM
static int zxdh_en_set_ringparam(struct net_device *netdev, struct ethtool_ringparam *param, struct kernel_ethtool_ringparam *kernel_ring, struct netlink_ext_ack *ack)
#else
static int zxdh_en_set_ringparam(struct net_device *netdev, struct ethtool_ringparam *param)
#endif
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t carrier_ok;
    int32_t err = 0;
    int32_t i = 0;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    if (!en_dev->ops->is_fw_feature_support(en_dev->parent, FW_FEATURE_QUEUE_RESET)) {
        LOG_ERR_DEV(en_dev->parent, "fw feature not supported\n");
        return -EINVAL;
    }

    if (en_dev->ops->is_bond(en_dev->parent))
        return -EINVAL;

    if (param->rx_jumbo_pending) {
        LOG_ERR_DEV(en_dev->parent, "rx_jumbo_pending not supported\n");
        return -EINVAL;
    }
    if (param->rx_mini_pending) {
        LOG_ERR_DEV(en_dev->parent, "rx_mini_pending not supported\n");
        return -EINVAL;
    }

    if ((param->rx_pending < ZXDH_PF_MIN_DESC_NUM) || (param->rx_pending > ZXDH_PF_MAX_DESC_NUM(en_dev))) {
        LOG_ERR_DEV(en_dev->parent, "rx_pending (%d) out of range\n", param->rx_pending);
        return -EINVAL;
    }

    if ((param->tx_pending < ZXDH_PF_MIN_DESC_NUM) || (param->tx_pending > ZXDH_PF_MAX_DESC_NUM(en_dev))) {
        LOG_ERR_DEV(en_dev->parent, "tx_pending (%d) out of range\n", param->tx_pending);
        return -EINVAL;
    }

    if (param->rx_pending == en_dev->eth_config.rx_queue_size &&
        param->tx_pending == en_dev->eth_config.tx_queue_size) {
        LOG_DEBUG_DEV(en_dev->parent, "no need to set ring param\n");
        return 0;
    }

    //1、关端口
    carrier_ok = netif_carrier_ok(netdev);
    netif_carrier_off(netdev);
    mutex_lock(&en_priv->lock);
    if (netif_running(netdev)) {
        zxdh_port_enable(en_dev, false);
    }

    //2、确保接收方向停流
    if (carrier_ok) {
        msleep(80); //等待vqm清空缓存报文
        if(!is_flow_stopped(en_dev)) {
            LOG_ERR_DEV(en_dev->parent, "rx flow stopped failed\n");
            err = -EINVAL;
            goto out;
        }
    }

    if (netif_running(netdev)) {
        if (!test_bit(ZXDH_DEVICE_STATE_OPENED, &en_dev->state)) {
            goto free_bufs;
        }

        cancel_delayed_work_sync(&en_dev->refill);
        for (i = 0; i < en_dev->max_vq_pairs; i++) {
            virtnet_napi_tx_disable(&en_dev->sq[i].napi);
            napi_disable(&en_dev->rq[i].napi);
        }
        netif_tx_stop_all_queues(netdev);
        netif_tx_disable(netdev);
        clear_bit(ZXDH_DEVICE_STATE_OPENED, &en_dev->state);
    }

free_bufs:
    //3、改配队列深度
    en_dev->eth_config.rx_queue_size = roundup_pow_of_two(param->rx_pending);
    en_dev->eth_config.tx_queue_size = roundup_pow_of_two(param->tx_pending);
    LOG_DEBUG_DEV(en_dev->parent, "rx_size: %d, tx_size: %d\n", en_dev->eth_config.rx_queue_size, en_dev->eth_config.tx_queue_size);
    mutex_lock(&en_dev->parent->lock);
    zxdh_free_unused_bufs(netdev);
    usleep_range(70, 100);
    zxdh_vvq_reset(en_dev);
    mutex_unlock(&en_dev->parent->lock);

    //4、队列reset
    err = zxdh_phy_vq_reset(en_dev);
    if (err != 0) {
        LOG_ERR_DEV(en_dev->parent, "zxdh_phy_vq_reset failed\n");
        err = -EINVAL;
    }

    //5、回填描述符
    if (netif_running(netdev)) {
        if (test_bit(ZXDH_DEVICE_STATE_OPENED, &en_dev->state)) {
            goto out;
        }

        for (i = 0; i < en_dev->max_vq_pairs; i++) {
            if (i < en_dev->eth_config.num_rxq){
                if (!try_fill_recv(&en_dev->rq[i], GFP_KERNEL))
                    schedule_delayed_work(&en_dev->refill, 0);
            }
            virtnet_napi_enable(en_dev->rq[i].vq, &en_dev->rq[i].napi);
            virtnet_napi_tx_enable(netdev, en_dev->sq[i].vq, &en_dev->sq[i].napi);
        }
        set_bit(ZXDH_DEVICE_STATE_OPENED, &en_dev->state);
    }

out:
    //6、开端口
    if (netif_running(netdev)) {
        zxdh_port_enable(en_dev, true);
        netif_tx_wake_all_queues(netdev);
    }
    if (carrier_ok && en_dev->link_up)
        netif_carrier_on(netdev);
    mutex_unlock(&en_priv->lock);

    return err;
}

static void zxdh_en_get_pauseparam(struct net_device *netdev, struct ethtool_pauseparam *pause)
{
    int32_t err;
    uint32_t fc_mode;
    struct zxdh_en_device *en_dev = netdev_priv(netdev);

    if (en_dev->device_state == ZXDH_DEVICE_STATE_INTERNAL_ERROR)
        return;

    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF)
    {
        return;
    }

    err = zxdh_en_fc_mode_get(en_dev, &fc_mode);
    if(err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_en_fc_mode_get failed!\n");
        return;
    }

    pause->autoneg = 0;

    switch(fc_mode)
    {
        case BIT(SPM_FC_PAUSE_FULL):
        {
            pause->rx_pause = 1;
            pause->tx_pause = 1;
            break;
        }
        case BIT(SPM_FC_PAUSE_RX):
        {
            pause->rx_pause = 1;
            pause->tx_pause = 0;
            break;
        }
        case BIT(SPM_FC_PAUSE_TX):
        {
            pause->rx_pause = 0;
            pause->tx_pause = 1;
            break;
        }
        default:
        {
            pause->rx_pause = 0;
            pause->tx_pause = 0;
            break;
        }
    }

    return;
}

static int32_t zxdh_en_set_pauseparam(struct net_device *netdev, struct ethtool_pauseparam *pause)
{
    int32_t err;
    uint32_t fc_mode_cur;
    uint32_t fc_mode_cfg;
    DPP_PF_INFO_T pf_info = {0};
    struct zxdh_en_device *en_dev = netdev_priv(netdev);

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    if((en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF) ||
       !(zxdh_en_is_panel_port(en_dev)))
    {
        return -EOPNOTSUPP;
    }

    if(pause->autoneg)
    {
        LOG_ERR_DEV(en_dev->parent, "not support pause autoneg!\n");
        return -EOPNOTSUPP;
    }

    err = zxdh_en_fc_mode_get(en_dev, &fc_mode_cur);
    if(err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_en_fc_mode_get failed!\n");
        return err;
    }

    if((pause->rx_pause || pause->tx_pause) && (fc_mode_cur == BIT(SPM_FC_PFC_FULL)))
    {
        LOG_ERR_DEV(en_dev->parent, "warning, ethtool cfg pause on, this will lead to pfc off!\n");
    }

    if(pause->rx_pause && pause->tx_pause)
    {
        fc_mode_cfg = BIT(SPM_FC_PAUSE_FULL);
    }
    else if(pause->rx_pause)
    {
        fc_mode_cfg = BIT(SPM_FC_PAUSE_RX);
    }
    else if(pause->tx_pause)
    {
        fc_mode_cfg = BIT(SPM_FC_PAUSE_TX);
    }
    else
    {
        if(fc_mode_cur == BIT(SPM_FC_PFC_FULL))
            fc_mode_cfg = BIT(SPM_FC_PFC_FULL);
        else
            fc_mode_cfg = BIT(SPM_FC_NONE);
    }

    if(fc_mode_cfg != fc_mode_cur)
    {
        err = zxdh_en_fc_mode_set(en_dev, fc_mode_cfg);
        if(err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_en_fc_mode_set failed!\n");
            return err;
        }

        if ((en_dev->board_type == DH_STD_E318) &&
            en_dev->ops->is_fw_feature_support(en_dev->parent, FW_FEATURE_NP_NPPU_TCAM_PFC_MAP) &&
            fc_mode_cur == BIT(SPM_FC_PFC_FULL) &&
            zxdh_en_is_panel_port(en_dev))
        {
            pf_info.slot = en_dev->slot_id;
            pf_info.vport = en_dev->vport;
            err = (int32_t)dpp_pktrx_tcam_pfc_set(&pf_info, 0);
            if (err != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "dpp_pktrx_tcam_pfc_set failed!\n");
                return err;
            }
        }
    }

    return 0;
}

#ifndef CGS_V5_693
static int32_t zxdh_en_get_fecparam(struct net_device *netdev, struct ethtool_fecparam *fecparam)
{
    int32_t err;
    uint32_t fec_cfg;
    uint32_t fec_active;
    struct zxdh_en_device *en_dev = netdev_priv(netdev);

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    if((en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF) ||
       (!zxdh_en_is_panel_port(en_dev)))
    {
        return -EOPNOTSUPP;
    }

    err = zxdh_en_fec_mode_get(en_dev, NULL, &fec_cfg, &fec_active);
    if(err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_en_fec_mode_get failed!\n");
        return err;
    }

    fecparam->fec = zxdh_en_fec_to_ethtool_fecparam(fec_cfg, GET_FEC_CFG_FLAG);
    fecparam->active_fec = zxdh_en_fec_to_ethtool_fecparam(fec_active, GET_FEC_LINK_FLAG);

    //LOG_INFO("fec_cfg=0x%x, fecparam->fec=0x%x, fec_active=0x%x, fecparam->active_fec=0x%x\n",
    //          fec_cfg, fecparam->fec, fec_active, fecparam->active_fec);

    return 0;
}

static int32_t zxdh_en_set_fecparam(struct net_device *netdev, struct ethtool_fecparam *fecparam)
{
    int32_t i;
    int32_t err;
    uint32_t fec_cap;
    uint32_t fec_cfg = 0;
    uint32_t fecparam_cap;
    struct zxdh_en_device *en_dev = netdev_priv(netdev);

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    if((en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF) ||
       (!zxdh_en_is_panel_port(en_dev)))
    {
        return -EOPNOTSUPP;
    }

    err = zxdh_en_fec_mode_get(en_dev, &fec_cap, NULL, NULL);
    if(err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_en_fec_mode_get failed!\n");
        return err;
    }
    fecparam_cap = zxdh_en_fec_to_ethtool_fecparam(fec_cap, GET_FEC_CAP_FLAG);

    if((fecparam->fec | fecparam_cap) != fecparam_cap)
    {
        LOG_ERR_DEV(en_dev->parent, "fecparam->fec 0x%x unsupport !\n", fecparam->fec);
        return -EOPNOTSUPP;
    }

    for(i = 0; i < ARRAY_SIZE(fec_2_ethtool_fecparam); i++)
    {
        if(fecparam->fec == fec_2_ethtool_fecparam[i])
        {
            fec_cfg |= BIT(i);
        }
    }

    if(!fec_cfg && (fecparam->fec != ETHTOOL_FEC_AUTO))
    {
        LOG_ERR_DEV(en_dev->parent, "fecparam->fec 0x%x unsupport !\n", fecparam->fec);
        return -EOPNOTSUPP;
    }

    //LOG_INFO("fecparam_cap=0x%x, fec_cap=0x%x, fecparam->fec=0x%x, fec_cfg=0x%x\n",
    //          fecparam_cap, fec_cap, fecparam->fec, fec_cfg);

    err = zxdh_en_fec_mode_set(en_dev, fec_cfg);
    if(err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_en_fec_mode_set failed!\n");
        return err;
    }

    return 0;
}
#endif

static int32_t zxdh_en_get_module_info(struct net_device *netdev, struct ethtool_modinfo *modinfo)
{
    uint32_t read_bytes;
    uint8_t data[2] = {0};
    uint8_t transceiver = 0;
    struct zxdh_en_module_eeprom_param query = {0};
    struct zxdh_en_device *en_dev = netdev_priv(netdev);

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    if((en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF) ||
       (!zxdh_en_is_panel_port(en_dev)))
    {
        return -EOPNOTSUPP;
    }

    query.i2c_addr = SFF_I2C_ADDRESS_LOW;
    query.page = 0;
    query.offset = 0;
    query.length = 2;
    read_bytes = zxdh_en_module_eeprom_read(en_dev, &query, data);
    if(read_bytes != query.length)
    {
        return -EIO;
    }

    switch(data[0])
    {
        case ZXDH_MODULE_ID_SFP:
            query.offset = SFF8472_I2C_ETH_TRANSCEIVER;
            query.length = 1;
            read_bytes = zxdh_en_module_eeprom_read(en_dev, &query, &transceiver);
            if(read_bytes != query.length) {
                return -EIO;
            }

            modinfo->type = ETH_MODULE_SFF_8472;
            if (transceiver == SFF8472_ETHERNET_25G_CR) {
                modinfo->eeprom_len = ETH_MODULE_SFF_8472_DAC_LEN;
            } else {
                modinfo->eeprom_len = ETH_MODULE_SFF_8472_LEN;
            }
            break;
        case ZXDH_MODULE_ID_QSFP:
            modinfo->type       = ETH_MODULE_SFF_8436;
            modinfo->eeprom_len = ETH_MODULE_SFF_8436_MAX_LEN;
            break;
        case ZXDH_MODULE_ID_QSFP_PLUS:
        case ZXDH_MODULE_ID_QSFP28:
            if(data[1] < 3)
            {
                modinfo->type       = ETH_MODULE_SFF_8436;
                modinfo->eeprom_len = ETH_MODULE_SFF_8436_MAX_LEN;
            }
            else
            {
                modinfo->type       = ETH_MODULE_SFF_8636;
                modinfo->eeprom_len = ETH_MODULE_SFF_8636_MAX_LEN;
            }
            break;
        /*ZXDH_MODULE_ID_QSFP_DD  ZXDH_MODULE_ID_OSFP在长度上不太对,其他类型的光模块开源代码没有*/
        case ZXDH_MODULE_ID_QSFP_DD:
        case ZXDH_MODULE_ID_OSFP:
        case ZXDH_MODULE_ID_DSFP:
        case ZXDH_MODULE_ID_QSFP_PLUS_WITH_CMIS:
        case ZXDH_MODULE_ID_SFP_DD_WITH_CMIS:
        case ZXDH_MODULE_ID_SFP_PLUS_WITH_CMIS:
            modinfo->type       = ETH_MODULE_SFF_8636;
            modinfo->eeprom_len = ETH_MODULE_SFF_8636_MAX_LEN;
            break;
        default:
            LOG_ERR_DEV(en_dev->parent, "can not recognize module identifier 0x%x!\n", data[0]);
            return -EINVAL;
    }

    return 0;
}

static int32_t zxdh_en_get_module_eeprom(struct net_device *netdev, struct ethtool_eeprom *ee, u8 *data)
{
    struct zxdh_en_module_eeprom_param query = {0};
    struct zxdh_en_device *en_dev = netdev_priv(netdev);
    uint32_t offset = ee->offset;
    uint32_t length = ee->len;
    uint8_t identifier;
    uint32_t offset_boundary = 0;
    uint32_t total_read_bytes = 0;
    uint32_t read_bytes = 0;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);

    if((en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF) ||
       (!zxdh_en_is_panel_port(en_dev)))
    {
        return -EOPNOTSUPP;
    }

    //LOG_INFO("offset %u, len %u\n", ee->offset, ee->len);

    if(!ee->len)
        return -EINVAL;

    memset(data, 0, ee->len);

    query.i2c_addr = SFF_I2C_ADDRESS_LOW;
    query.bank = 0;
    query.page = 0;
    query.offset = 0;
    query.length = 1;
    read_bytes = zxdh_en_module_eeprom_read(en_dev, &query, &identifier);
    if(read_bytes != query.length)
    {
        return -EIO;
    }

    while(total_read_bytes < ee->len)
    {
        if(identifier == ZXDH_MODULE_ID_SFP)
        {
            if(offset < 256)
            {
                query.i2c_addr = SFF_I2C_ADDRESS_LOW;
                query.page = 0;
                query.offset = offset;
            }
            else
            {
                query.i2c_addr = SFF_I2C_ADDRESS_HIGH;
                query.page = 0;
                query.offset = offset - 256;
            }
            offset_boundary = (query.offset < 128) ? 128 : 256;
            query.length = ((query.offset + length) > offset_boundary) ? (offset_boundary - query.offset) : length;
        }
        else if(identifier == ZXDH_MODULE_ID_QSFP ||
                identifier == ZXDH_MODULE_ID_QSFP_PLUS ||
                identifier == ZXDH_MODULE_ID_QSFP28 ||
                identifier == ZXDH_MODULE_ID_QSFP_DD ||
                identifier == ZXDH_MODULE_ID_OSFP ||
                identifier == ZXDH_MODULE_ID_DSFP ||
                identifier == ZXDH_MODULE_ID_QSFP_PLUS_WITH_CMIS ||
                identifier == ZXDH_MODULE_ID_SFP_DD_WITH_CMIS ||
                identifier == ZXDH_MODULE_ID_SFP_PLUS_WITH_CMIS)
        {
            query.i2c_addr = SFF_I2C_ADDRESS_LOW;
            if(offset < 256)
            {
                query.page = 0;
                query.offset = offset;
            }
            else
            {
                query.page = (offset - 256) / 128 + 1;
                query.offset = offset - 128 * query.page;
            }
            offset_boundary = (query.offset < 128) ? 128 : 256;
            query.length = ((query.offset + length) > offset_boundary) ? (offset_boundary - query.offset) : length;
        }
        else
        {
            LOG_ERR_DEV(en_dev->parent, "can not recognize module identifier 0x%x!\n", identifier);
            return -EINVAL;
        }

        read_bytes = zxdh_en_module_eeprom_read(en_dev, &query, data + total_read_bytes);
        if(read_bytes != query.length)
        {
            return -EIO;
        }

        total_read_bytes += read_bytes;
        offset += read_bytes;
        length -= read_bytes;
    }

    return 0;
}

#ifdef HAVE_ETHTOOL_GET_MODULE_EEPROM_BY_PAGE
static int32_t zxdh_en_get_module_eeprom_by_page(struct net_device *netdev,
                                                 const struct ethtool_module_eeprom *page_data,
                                                 struct netlink_ext_ack *extack)
{
    struct zxdh_en_module_eeprom_param query = {0};
    struct zxdh_en_device *en_dev = netdev_priv(netdev);
    uint32_t read_bytes = 0;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);

    if((en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF) ||
       (!zxdh_en_is_panel_port(en_dev)))
    {
        return -EOPNOTSUPP;
    }

    //LOG_INFO("offset %u, length %u, page %u, bank %u, i2c_address %u\n",
    //          page_data->offset, page_data->length, page_data->page, page_data->bank, page_data->i2c_address);

    if(!page_data->length)
        return -EINVAL;

    zte_memset_s(page_data->data, 0, page_data->length);

    query.i2c_addr = page_data->i2c_address;
    query.bank     = page_data->bank;
    query.page     = page_data->page;
    query.offset   = page_data->offset;
    query.length   = page_data->length;

    /* 优先命中 cache：按 (i2c, bank, page, offset, length) 在 g_module_regions 里匹配 */
    read_bytes = zxdh_module_region_copy_from_cache(en_dev, &query, page_data->data);
    if (read_bytes == query.length)
        return read_bytes;

    /* cache miss，回退到硬件直读 */
    read_bytes = zxdh_en_module_eeprom_read(en_dev, &query, page_data->data);
    if(read_bytes != query.length)
    {
        return -EIO;
    }

    return read_bytes;
}
#endif

static int32_t zxdh_test_health_info(struct zxdh_en_priv *en_priv)
{
    struct zxdh_en_device *en_dev  = &en_priv->edev;
    struct dh_core_dev    *dh_dev  = en_dev->parent;
    struct zxdh_pf_device *pf_dev  = dh_core_priv(dh_dev->parent);
    struct zxdh_core_health *health = &pf_dev->health;

    return health->fatal ? 1 : 0;
}

static int32_t zxdh_test_link_speed(struct zxdh_en_priv *en_priv)
{
    struct zxdh_en_device *en_dev  = &en_priv->edev;

    if (!test_bit(ZXDH_DEVICE_STATE_OPENED, &en_dev->state))
    {
        return 1;
    }

    if (en_dev->speed == SPEED_UNKNOWN)
    {
        LOG_ERR_DEV(en_dev->parent, "get link speed error\n");
        return 1;
    }
    return 0;
}

static int32_t zxdh_test_link_state(struct zxdh_en_priv *en_priv)
{
    struct zxdh_en_device *en_dev = &en_priv->edev;

    if (!test_bit(ZXDH_DEVICE_STATE_OPENED, &en_dev->state))
    {
        return 1;
    }

    if ((!en_dev->link_up) && (!netif_carrier_ok(en_dev->netdev)))
    {
        LOG_ERR_DEV(en_dev->parent, "curr state is link down\n");
        return 1;
    }

    return 0;
}

#ifdef CONFIG_INET
static int32_t zxdh_test_loopback_validate(struct sk_buff *skb,
                                           struct net_device *ndev,
                                           struct packet_type *pt,
                                           struct net_device *orig_ndev)
{
    struct zxdh_lbt_priv *lbtp = pt->af_packet_priv;
    struct zxdh_ehdr *zxdhh = NULL;
    struct ethhdr *ethh = NULL;
    struct udphdr *udph = NULL;
    struct iphdr *iph= NULL;

    ethh = (struct ethhdr *)skb_mac_header(skb);
    if (!ether_addr_equal(ethh->h_dest, orig_ndev->dev_addr))
    {
        goto out;
    }

    iph = ip_hdr(skb);
    if (iph->protocol != IPPROTO_UDP)
    {
        goto out;
    }

    /* Don't assume skb_transport_header() was set */
    udph = (struct udphdr *)((uint8_t *)iph + 4 * iph->ihl);
    if (udph->dest != htons(9))
    {
        goto out;
    }

    zxdhh = (struct zxdh_ehdr *)((int8_t *)udph + sizeof(*udph));
    if (zxdhh->magic != cpu_to_be64(ZXDH_TEST_MAGIC))
    {
        goto out; /* so close ! */
    }
    lbtp->loopback_ok = true;
    complete(&lbtp->comp);
out:
    kfree_skb(skb);
    return 0;
}

int32_t zxdh_test_loopback_setup(struct zxdh_en_priv *en_priv, struct zxdh_lbt_priv *lbtp)
{
    struct zxdh_en_device *en_dev = &en_priv->edev;

    en_dev->local_lb_enable = true;/* 使能环回标识 */
    lbtp->loopback_ok = false;
    init_completion(&lbtp->comp); /* 初始化完成量 */

    lbtp->pt.type = htons(ETH_P_IP);
    lbtp->pt.func = zxdh_test_loopback_validate;
    lbtp->pt.dev = en_dev->netdev;
    lbtp->pt.af_packet_priv = lbtp;
    dev_add_pack(&lbtp->pt); /* 注册回调函数 */
    return 0;
}

static void zxdh_test_loopback_cleanup(struct zxdh_en_priv *en_priv, struct zxdh_lbt_priv *lbtp)
{
    struct zxdh_en_device *en_dev = &en_priv->edev;

    en_dev->local_lb_enable = false; /* 去使能环回标识 */
    dev_remove_pack(&lbtp->pt);
}

/* 构造udp报文 */
static struct sk_buff *zxdh_test_get_udp_skb(struct zxdh_en_priv *en_priv)
{
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct sk_buff *skb = NULL;
    struct zxdh_ehdr *zxdhh = NULL;
    struct ethhdr *ethh = NULL; /* 报文的L2头*/
    struct udphdr *udph = NULL; /* 报文的L4头*/
    struct iphdr *iph = NULL;   /* 报文的L3头*/
    int32_t iplen = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    skb = netdev_alloc_skb(en_dev->netdev, ZXDH_TEST_PKT_SIZE);
    if (skb == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "Failed to alloc loopback skb\n");
        return NULL;
    }

    /* Reserve for ethernet and IP header  */
#ifndef CGS_V5_693
    ethh = skb_push(skb, ETH_HLEN); /* 插入L2头*/
#else
    ethh = (struct ethhdr *)skb_push(skb, ETH_HLEN); /* 插入L2头*/
#endif
    skb_reset_mac_header(skb);

    skb_set_network_header(skb, skb->len);
#ifndef CGS_V5_693
    iph = skb_put(skb, sizeof(struct iphdr));  /* 插入ip头 */
#else
    iph = (struct iphdr *)skb_put(skb, sizeof(struct iphdr));  /* 插入ip头 */
#endif

    skb_set_transport_header(skb, skb->len);
#ifndef CGS_V5_693
    udph = skb_put(skb, sizeof(struct udphdr)); /* 插入udp头 */
#else
    udph = (struct udphdr *)skb_put(skb, sizeof(struct udphdr)); /* 插入udp头 */
#endif
    /* Fill ETH header */
    ether_addr_copy(ethh->h_dest, en_dev->netdev->dev_addr);
    eth_zero_addr(ethh->h_source);
    ethh->h_proto = htons(ETH_P_IP); /* ipv4 */

    /* Fill UDP header */
    udph->source = htons(9);
    udph->dest = htons(9); /* Discard服务:测试网络连接，到达此端口的包会被drop*/
    udph->len = htons(sizeof(struct zxdh_ehdr) + sizeof(struct udphdr));
    udph->check = 0;

    /* Fill IP header */
    iph->ihl = 5;
    iph->ttl = 32;
    iph->version = 4;
    iph->protocol = IPPROTO_UDP;
    iplen = sizeof(struct iphdr) + sizeof(struct udphdr) + sizeof(struct zxdh_ehdr); /* ip数据包的总长度 */
    iph->tot_len = htons(iplen);
    iph->frag_off = 0;
    iph->saddr = 0;
    iph->daddr = 0;
    iph->tos = 0;
    iph->id = 0;
    ip_send_check(iph);

    /* Fill test header and data */
#ifndef CGS_V5_693
    zxdhh = skb_put(skb, sizeof(*zxdhh));
#else
    zxdhh = (struct zxdh_ehdr *)skb_put(skb, sizeof(*zxdhh));
#endif
    zxdhh->magic = cpu_to_be64(ZXDH_TEST_MAGIC);

    skb->csum = 0;
    skb->ip_summed = CHECKSUM_PARTIAL;
    udp4_hwcsum(skb, iph->saddr, iph->daddr); /* udp校验*/

    skb->protocol = htons(ETH_P_IP);
    skb->pkt_type = PACKET_HOST;
    skb->dev = en_dev->netdev;

    return skb;
}

static int32_t zxdh_test_loopback(struct zxdh_en_priv *en_priv)
{
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct zxdh_lbt_priv *lbtp = NULL;
    struct sk_buff *skb = NULL;
    int32_t err = 0;

    if (!test_bit(ZXDH_DEVICE_STATE_OPENED, &en_dev->state))
    {
        LOG_ERR_DEV(en_dev->parent, "Can't perform loopback test while device is down\n");
        return -ENODEV;
    }

    lbtp = kzalloc(sizeof(*lbtp), GFP_KERNEL);
    if (lbtp == NULL)
    {
        return -ENOMEM;
    }
    lbtp->loopback_ok = false;

    err = zxdh_test_loopback_setup(en_priv, lbtp);
    if (err != 0)
    {
        goto out;
    }

    skb = zxdh_test_get_udp_skb(en_priv);
    if (skb == NULL)
    {
        err = -ENOMEM;
        goto cleanup;
    }

    skb_set_queue_mapping(skb, 0);
    err = dev_queue_xmit(skb);
    if (err)
    {
        LOG_ERR_DEV(en_dev->parent, "Failed to xmit loopback packet err(%d)\n", err);
        goto cleanup;
    }

    wait_for_completion_timeout(&lbtp->comp, ZXDH_LB_VERIFY_TIMEOUT);
    err = !lbtp->loopback_ok;

cleanup:
    zxdh_test_loopback_cleanup(en_priv, lbtp);
out:
    kfree(lbtp);
    return err;
}
#endif /* CONFIG_INET */

static int32_t (*zxdh_st_func[ZXDH_ST_NUM])(struct zxdh_en_priv *) = {
    zxdh_test_link_state,
    zxdh_test_link_speed,
    zxdh_test_health_info,
#ifdef CONFIG_INET
    zxdh_test_loopback,
#endif
};

int32_t zxdh_en_self_test_num(void)
{
    return ARRAY_SIZE(zxdh_self_tests);
}

static void zxdh_en_diag_test(struct net_device *netdev, struct ethtool_test *etest, u64 *buf)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    int32_t i = 0;

    if (en_priv->edev.device_state == ZXDH_DEVICE_STATE_INTERNAL_ERROR)
        return;

    memset(buf, 0, sizeof(uint64_t) * ZXDH_ST_NUM);

    mutex_lock(&en_priv->lock);

    LOG_INFO_DEV(en_priv->edev.parent, "Self test begin...\n");

    for (i = 0; i < ZXDH_ST_NUM; i++)
    {
        LOG_INFO_DEV(en_priv->edev.parent, "[%d] %s start..\n", i, zxdh_self_tests[i]);
        buf[i] = zxdh_st_func[i](en_priv);
        LOG_INFO_DEV(en_priv->edev.parent, "[%d] %s end: result(%lld)\n", i, zxdh_self_tests[i], buf[i]);
    }

    mutex_unlock(&en_priv->lock);

    for (i = 0; i < ZXDH_ST_NUM; i++)
    {
        if (buf[i])
        {
            etest->flags |= ETH_TEST_FL_FAILED;
            break;
        }
    }

    LOG_INFO_DEV(en_priv->edev.parent, "Self test out: status flags(0x%x)\n", etest->flags);
}

static int32_t zxdh_hardware_bond_enable_proc(struct net_device *netdev, bool enable)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct zxdh_lag_tracker *tracker = NULL;
    struct zxdh_lag_dev *ldev = NULL;
    int32_t ret = 0;

    if ((en_dev->ops->is_bond(en_dev->parent)) || (en_dev->ops->is_special_bond(en_dev->parent)) ||
        (!zxdh_en_is_panel_port(en_dev)) || (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF))
        return -EPERM;

    if (!netif_is_lag_port(netdev)) //若网口不位于内核bond组中
    {
        en_dev->is_hwbond = enable;
        en_dev->ops->is_hwbond(en_dev->parent, en_dev->is_hwbond, TRUE);
        return 0;
    }

    if(!en_dev->is_hwbond && enable) //hardware-bond变量希望从off更新为on, 退出
    {
        LOG_INFO_DEV(en_dev->parent, "Operation failed: Cannot transition %s's hardware bonding state from off to on as the device is a bond slave, exit\n", netdev_name(netdev));
        return -EPERM;
    }

    // 判断是否处于硬bond场景
    ldev = en_dev->ldev;
    if (!ldev)
    {
        return -EPERM;
    }
    tracker = &ldev->tracker;
    if (!tracker)
    {
        return -EPERM;
    }

        if(ldev->state == LAG_DEV_ACTIVE && ldev->is_active && ldev->upper_netdev && tracker->bond_type == HARDWARE_BOND)
        {
            LOG_INFO_DEV(en_dev->parent, "%s lag_dev[%d] bond_type is HARDWARE_BOND, current hardware-bond state is %d", netdev_name(netdev), ldev->idx, en_dev->is_hwbond);
            if (en_dev->is_hwbond && !enable) //hardware-bond 变量希望从 on 更新为 off
            {
                LOG_INFO_DEV(en_dev->parent, "set %s hardware-bond from on to off , %s become to SOFTWARE_BOND, exit\n", netdev_name(netdev), ldev->upper_name);
                ret = zxdh_lag_change_to_software_bond(ldev, tracker, en_dev);
                if (ret == 0)
                {
                    en_dev->is_hwbond = enable;
                    en_dev->ops->is_hwbond(en_dev->parent, en_dev->is_hwbond, TRUE);
                }
            }
        }
    else // 非HARDWARE_BOND场景下，可以直接修改en_dev->is_hwbond的值
    {
        en_dev->is_hwbond = enable;
        en_dev->ops->is_hwbond(en_dev->parent, en_dev->is_hwbond, TRUE);
    }

    return ret;
}

static int32_t zxdh_hardware_bond_primary_enable_proc(struct net_device *netdev, bool enable)
{
    return 0;
}

static int32_t zxdh_lldp_enable_proc(struct net_device *netdev, bool enable)
{
    int32_t ret = 0;
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);

    ret = zxdh_lldp_enable_set(&en_priv->edev, enable);
    if (0 != ret)
    {
        LOG_ERR_DEV(en_priv->edev.parent, "%s lldp failed!\n", enable ? "enable" : "disable");
        return ret;
    }

    return ret;
}

static int32_t zxdh_dual_tor_switch_proc(struct net_device *netdev, bool enable)
{
    int32_t ret = 0;
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint8_t psn_version = 0;
    bool dual_tor = FALSE;
    bool quad_tor = FALSE;

    en_dev->ops->get_psn_feature_info(en_dev->parent, &psn_version, &dual_tor, &quad_tor);
    if (psn_version >= 1 && !dual_tor)
    {
        LOG_INFO("%s: unsupported dual_tor config\n", netdev_name(netdev));
        return -EPERM;
    }

    ret = zxdh_dual_tor_switch(&en_priv->edev, enable);
    if (0 != ret)
    {
        LOG_ERR_DEV(en_priv->edev.parent, "%s zxdh_dual_tor_switch failed!\n", enable ? "enable" : "disable");
        return ret;
    }

    return ret;
}

static int32_t zxdh_1588_debug_enable_proc(struct net_device *netdev, bool enable)
{
    enable_1588_debug = enable;
    return 0;
}

static int32_t zxdh_1588_enable_proc(struct net_device *netdev, bool enable)
{
    union zxdh_msg *msg = NULL;
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t ret = 0;
    struct zxdh_bar_extra_para para = {0};
    DPP_PF_INFO_T dpp_pf_info = {
        .slot = en_dev->slot_id,
        .vport = en_dev->vport,
    };

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;
    if (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
        if (msg == NULL)
        {
            LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
            return -ENOMEM;
        }

        msg->payload.vf_1588_enable.proc_cmd = ZXDH_VF_1588_ENABLE_SET;
        msg->payload.hdr.op_code = ZXDH_VF_1588_ENABLE;
        msg->payload.hdr.vport = en_dev->vport;
        msg->payload.hdr.pcie_id = en_dev->pcie_id;
        msg->payload.vf_1588_enable.enable_1588_vf = (uint32_t)enable;
        ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
        if(ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_send_command_to_pf failed: %d\n", ret);
            kfree(msg);
            return ret;
        }

        kfree(msg);
        en_dev->enable_1588 = enable;
        return ret;
    }

    ret = dpp_vport_attr_set(&dpp_pf_info, SRIOV_VPORT_1588_EN, (uint32_t)enable);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_vport_attr_set SRIOV_VPORT_1588_EN failed, ret:%d\n", ret);
        return ret;
    }

    en_dev->enable_1588 = enable;
    return ret;
}

static int32_t zxdh_rsskey_ipid_proc(struct net_device *netdev, bool enable)
{
    union zxdh_msg *msg = NULL;
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t ret = 0;
    struct zxdh_bar_extra_para para = {0};
    DPP_PF_INFO_T dpp_pf_info = {
        .slot = en_dev->slot_id,
        .vport = en_dev->vport,
    };

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;
    if (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
        if (msg == NULL)
        {
            LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
            return -ENOMEM;
        }

        msg->payload.vf_rsskey_ipid.proc_cmd = ZXDH_VF_RSSKEY_IPID_SET;
        msg->payload.hdr.op_code = ZXDH_VF_RSSKEY_IPID_ENABLE;
        msg->payload.hdr.vport = en_dev->vport;
        msg->payload.hdr.pcie_id = en_dev->pcie_id;
        msg->payload.vf_rsskey_ipid.enable_vf_rsskey_ipid = (uint32_t)enable;
        ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
        if(ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_send_command_to_pf failed: %d\n", ret);
        }

        kfree(msg);
        return ret;
    }

    ret = dpp_vport_attr_set(&dpp_pf_info, SRIOV_VPORT_FRAG_PKT_USE_IPID, (uint32_t)enable);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_vport_attr_set SRIOV_VPORT_FRAG_PKT_USE_IPID failed, ret:%d\n", ret);
    }

    return ret;
}

static int32_t zxdh_quad_tor_proc(struct net_device *netdev, bool enable)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint8_t psn_version = 0;
    bool dual_tor = FALSE;
    bool quad_tor = FALSE;
    int32_t ret = 0;
    DPP_PF_INFO_T dpp_pf_info = {
        .slot = en_dev->slot_id,
        .vport = en_dev->vport,
    };

    if ((en_dev->ops->is_bond(en_dev->parent)) || (en_dev->ops->is_special_bond(en_dev->parent)) ||
        (!zxdh_en_is_panel_port(en_dev)) || (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF) || 
        (en_dev->ops->get_bond_port_type(en_dev->parent) == BOND_TWO_PORT_TYPE))
    {
        LOG_INFO("%s: unsupported quad_tor config\n", netdev_name(netdev));
        return -EPERM;
    }

    en_dev->ops->get_psn_feature_info(en_dev->parent, &psn_version, &dual_tor, &quad_tor);
    if (psn_version < 1 || (psn_version >= 1 && !quad_tor))
    {
        LOG_INFO("%s: unsupported quad_tor config\n", netdev_name(netdev));
        return -EPERM;
    }
    en_dev->ops->update_mp_enable(en_dev->parent, enable);
    ret = dpp_vport_attr_set(&dpp_pf_info, SRIOV_VPORT_MULTI_PLANE_EN, (uint32_t)enable);
    if (ret != 0)
    {
        LOG_ERR("dpp_vport_attr_set SRIOV_VPORT_MULTI_PLANE_EN failed, ret:%d\n", ret);
    }

    return ret;
}

static int32_t zxdh_link_down_on_close_proc(struct net_device *netdev, bool enable)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    int32_t ret = 0;

    en_priv->edev.link_down_on_close = enable;

    return ret;
}

static int32_t zxdh_ets_info_update(struct zxdh_en_priv *en_priv, uint32_t mode, uint32_t *cur_mode, uint32_t *tc_td_th)
{
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t ret = 0;
    uint32_t ets_tc_td_th[ZXDH_DCBNL_MAX_TRAFFIC_CLASS] = {0};
    DPP_PF_INFO_T pf_info = {0};
    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    ret = zxdh_dcbnl_get_flow_td_th(en_priv, ets_tc_td_th);
    if(ret)
    {
        LOG_ERR_DEV(en_dev->parent, "get td_th fail\n");
        return ret;
    }

    *cur_mode = en_dev->ets_info.cur_ets; // 传出当前开关状态
    if (!en_dev->ets_info.switch_flag)  // 第一次从关到开
    {
        memcpy(en_dev->ets_info.tc_td_th, ets_tc_td_th, sizeof(uint32_t) * ZXDH_DCBNL_MAX_TRAFFIC_CLASS);
    }
    else if(en_dev->ets_info.cur_ets) // 只有当前开关状态是开的状态下才更新tc_td_th
    {
        memcpy(en_dev->ets_info.tc_td_th, ets_tc_td_th, sizeof(uint32_t) * ZXDH_DCBNL_MAX_TRAFFIC_CLASS);
        LOG_INFO_DEV(en_dev->parent, "Updated PF ets_info: slot=%d, vport=0x%x, ets_mode=%u\n", pf_info.slot, pf_info.vport, mode);
        LOG_INFO_DEV(en_dev->parent, "td_th -> tc_td_th[0]:%d tc_td_th[1]:%d tc_td_th[2]:%d tc_td_th[3]:%d tc_td_th[4]:%d tc_td_th[5]:%d tc_td_th[6]:%d tc_td_th[7]:%d\n",
                                            tc_td_th[0], tc_td_th[1], tc_td_th[2], tc_td_th[3], tc_td_th[4], tc_td_th[5], tc_td_th[6], tc_td_th[7]);
    }
    en_dev->ets_info.cur_ets = mode;
    en_dev->ets_info.switch_flag = 1;

    memcpy(tc_td_th, en_dev->ets_info.tc_td_th, sizeof(uint32_t) * ZXDH_DCBNL_MAX_TRAFFIC_CLASS); // 关->开，传出存储的td配置

    return ret;
}

/* Started by AICoder, pid:t3176w5cf0n5e57140e90a54e0c0e732a8f01a25 */
static int32_t zxdh_ets_switch_proc(struct net_device *netdev, bool enable)
{
    int32_t ret = 0;
    uint32_t mode = enable == true ? 1 : 0;
    uint32_t old_mode = 0;
    uint32_t tc_td_th[ZXDH_DCBNL_MAX_TRAFFIC_CLASS] = {0};
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);

    ret = zxdh_dcbnl_enable_debug(en_priv);
    if(ret)
    {
        LOG_ERR_DEV(en_priv->edev.parent, "clean flow fail %s\n", netdev->name);
        return ret;
    }

    ret = zxdh_ets_info_update(en_priv, mode, &old_mode, tc_td_th);
    if(ret)
    {
        LOG_ERR_DEV(en_priv->edev.parent, "ets_info_update fail %s\n", netdev->name);
        return ret;
    }

    if((old_mode ^ mode) && mode) // ets开关状态发生改变，ets 关->开
    {
        ret = zxdh_dcbnl_set_flow_td_th(en_priv, tc_td_th);
        LOG_INFO_DEV(en_priv->edev.parent, "set_flow_td_th -> tc_td_th[0]:%d tc_td_th[1]:%d tc_td_th[2]:%d tc_td_th[3]:%d tc_td_th[4]:%d tc_td_th[5]:%d tc_td_th[6]:%d tc_td_th[7]:%d\n",
                                                    tc_td_th[0], tc_td_th[1], tc_td_th[2], tc_td_th[3], tc_td_th[4], tc_td_th[5], tc_td_th[6], tc_td_th[7]);
        if(ret)
        {
            LOG_ERR_DEV(en_priv->edev.parent, "set td_th fail %s\n", netdev->name);
            return ret;
        }

        ret = zxdh_dcbnl_set_tm_gate(en_priv, mode);
        if(ret)
        {
            LOG_ERR_DEV(en_priv->edev.parent, "ets switch fail %s %u\n", netdev->name, mode);
            return ret;
        }
    }

    if((old_mode ^ mode) && !mode) // ets开关状态发生改变，ets 开->关
    {
        ret = zxdh_dcbnl_set_tm_gate(en_priv, mode);
        if(ret)
        {
            LOG_ERR_DEV(en_priv->edev.parent, "ets switch fail %s %u\n", netdev->name, mode);
            return ret;
        }

        ret = zxdh_dcbnl_clear_flow_td_th(en_priv);
        if(ret)
        {
            LOG_ERR_DEV(en_priv->edev.parent, "clear td_th fail %s\n", netdev->name);
            return ret;
        }
    }

    ret = zxdh_dcbnl_disable_debug(en_priv);
    if(ret)
    {
        LOG_ERR_DEV(en_priv->edev.parent, "disable debug %s\n", netdev->name);
        return ret;
    }

    LOG_INFO_DEV(en_priv->edev.parent, "ets switch success %s %u\n", netdev->name, mode);
    return ret;
}
/* Ended by AICoder, pid:t3176w5cf0n5e57140e90a54e0c0e732a8f01a25 */

static int32_t zxdh_rdma_ets_switch_proc(struct net_device *netdev, bool enable)
{
    int32_t ret = 0;
    uint32_t mode = enable == true ? 1 : 0;
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct zxdh_rdma_status rdma_status = {0};

    en_dev->rdma_ets_flag = mode;

    rdma_status.port = en_dev->phy_port;
    rdma_status.mode = mode;

    ret = zxdh_rdma_events_call(en_dev->netdev, ZXDH_RDMA_ETS_SWITCH_EVENT, &rdma_status);
    if (ret) 
    {
        LOG_ERR_DEV(en_dev->parent, "Failed to set RDMA ETS flag: %u\n", ret);
    }

    LOG_INFO_DEV(en_dev->parent, "rdma ets switch success %s %u\n", netdev->name, mode);
    return ret;
}

static int32_t zxdh_prio_stat_switch_proc(struct net_device *netdev, bool enable)
{
    int32_t ret = 0;
    uint32_t mode = enable == true ? 1 : 0;
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    ret = zxdh_prio_stat_switch(en_dev, mode);
    if (ret) 
    {
        LOG_ERR("Failed to set PRIO STAT flag ret: %u\n", ret);
        return ret;
    }

    LOG_INFO("prio stat switch success %s %u\n", netdev->name, mode);
    return ret;
}

static int32_t zxdh_pcie_rp_cpl_timeout(struct net_device *netdev, bool mask)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    return en_dev->ops->set_cpl_timeout_mask(en_dev->parent, mask);
}

static int32_t zxdh_pcie_rp_hp_irq_ctl(struct net_device *netdev, bool status)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    return en_dev->ops->set_hp_irq_ctrl_status(en_dev->parent, status);
}

typedef int32_t (*zxdh_pflag_handler)(struct net_device *netdev, bool enable);

struct flag_desc
{
    uint8_t name[ETH_GSTRING_LEN];
    uint32_t bitno;
    zxdh_pflag_handler handler;
};

#define ZXDH_PRIV_DESC(_name, _bitno, _handler) \
{                                               \
    .name = _name,                              \
    .bitno = _bitno,                            \
    .handler = _handler,                        \
}

static const struct flag_desc zxdh_gstrings_priv_flags[] =
{
    ZXDH_PRIV_DESC("enable_lldp", ZXDH_PFLAG_ENABLE_LLDP, zxdh_lldp_enable_proc),
    ZXDH_PRIV_DESC("1588_debug", ZXDH_PFLAG_1588_DEBUG, zxdh_1588_debug_enable_proc),
    ZXDH_PRIV_DESC("hardware-bond", ZXDH_PFLAG_HARDWARE_BOND, zxdh_hardware_bond_enable_proc),
    ZXDH_PRIV_DESC("hardware-bond-primary", ZXDH_PFLAG_HARDWARE_BOND_PRIMARY, zxdh_hardware_bond_primary_enable_proc),
    ZXDH_PRIV_DESC("link-down-on-close", ZXDH_PFLAG_LINK_DOWN_ON_CLOSE, zxdh_link_down_on_close_proc),
    ZXDH_PRIV_DESC("ets-switch", ZXDH_PFLAG_ETS_SWITCH, zxdh_ets_switch_proc),
    ZXDH_PRIV_DESC("pcie_aer_cpl_timeout", ZXDH_PFLAG_PCIE_AER_CPL_TIMEOUT, zxdh_pcie_rp_cpl_timeout),
    ZXDH_PRIV_DESC("pcie_rp_hp_irq_ctl", ZXDH_PFLAG_PCIE_HP_IRQ_CTRL, zxdh_pcie_rp_hp_irq_ctl),
    ZXDH_PRIV_DESC("dual_tor", ZXDH_PFLAG_DUAL_TOR_CTRL, zxdh_dual_tor_switch_proc),
    ZXDH_PRIV_DESC("1588_enable", ZXDH_PFLAG_1588_ENABLE, zxdh_1588_enable_proc),
    ZXDH_PRIV_DESC("rsskey_ipid", ZXDH_PFLAG_RSSKEY_IPID, zxdh_rsskey_ipid_proc),
    ZXDH_PRIV_DESC("rdma-ets-switch", ZXDH_PFLAG_RDMA_ETS_SWITCH, zxdh_rdma_ets_switch_proc),
    ZXDH_PRIV_DESC("prio-stat-switch", ZXDH_PFLAG_RRIO_STAT_SWITCH, zxdh_prio_stat_switch_proc),
    ZXDH_PRIV_DESC("quad_tor", ZXDH_PFLAG_QUAD_TOR, zxdh_quad_tor_proc),
};

#define ZXDH_PRIV_FALG_ARRAY_SIZE ARRAY_SIZE(zxdh_gstrings_priv_flags)

static void zxdh_en_get_strings(struct net_device *netdev, u32 stringset, u8 *data)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint16_t i = 0;

    switch (stringset)
    {
        case ETH_SS_STATS:
        {
            snprintf(data, ETH_GSTRING_LEN, "rx_packets");//get stat from netdev->stats
            ZXDH_ADD_STRING(data, "tx_packets");
            ZXDH_ADD_STRING(data, "rx_bytes");
            ZXDH_ADD_STRING(data, "tx_bytes");
            ZXDH_ADD_STRING(data, "tx_queue_wake");
            ZXDH_ADD_STRING(data, "tx_queue_stopped");
            ZXDH_ADD_STRING(data, "tx_queue_dropped");
            ZXDH_ADD_STRING(data, "rx_removed_vlan_packets");//get stat from xmit-func
            ZXDH_ADD_STRING(data, "tx_added_vlan_packets");
            ZXDH_ADD_STRING(data, "rx_csum_unnecessary");
            ZXDH_ADD_STRING(data, "rx_csum_none");
            ZXDH_ADD_STRING(data, "rx_csum_unnecessary_tunnel");

            ZXDH_ADD_STRING(data, "rx_vport_packets");//get stat from vqm
            ZXDH_ADD_STRING(data, "tx_vport_packets");
            ZXDH_ADD_STRING(data, "rx_vport_bytes");
            ZXDH_ADD_STRING(data, "tx_vport_bytes");
            ZXDH_ADD_STRING(data, "rx_vport_dropped");
            ZXDH_ADD_STRING(data, "rx_vport_unicast_packets");//get stat from np
            ZXDH_ADD_STRING(data, "tx_vport_unicast_packets");
            ZXDH_ADD_STRING(data, "rx_vport_unicast_bytes");
            ZXDH_ADD_STRING(data, "tx_vport_unicast_bytes");
            ZXDH_ADD_STRING(data, "rx_vport_multicast_packets");
            ZXDH_ADD_STRING(data, "tx_vport_multicast_packets");
            ZXDH_ADD_STRING(data, "rx_vport_multicast_bytes");
            ZXDH_ADD_STRING(data, "tx_vport_multicast_bytes");
            ZXDH_ADD_STRING(data, "rx_vport_broadcast_packets");
            ZXDH_ADD_STRING(data, "tx_vport_broadcast_packets");
            ZXDH_ADD_STRING(data, "rx_vport_broadcast_bytes");
            ZXDH_ADD_STRING(data, "tx_vport_broadcast_bytes");
            ZXDH_ADD_STRING(data, "rx_vport_mtu_drop_packets");
            ZXDH_ADD_STRING(data, "tx_vport_mtu_drop_packets");
            ZXDH_ADD_STRING(data, "rx_vport_mtu_drop_bytes");
            ZXDH_ADD_STRING(data, "tx_vport_mtu_drop_bytes");
            ZXDH_ADD_STRING(data, "rx_vport_plcr_drop_packets");
            ZXDH_ADD_STRING(data, "tx_vport_plcr_drop_packets");
            ZXDH_ADD_STRING(data, "rx_vport_plcr_drop_bytes");
            ZXDH_ADD_STRING(data, "tx_vport_plcr_drop_bytes");
            ZXDH_ADD_STRING(data, "tx_vport_ssvpc_packets");
            ZXDH_ADD_STRING(data, "rx_vport_idma_drop_packets");
            ZXDH_ADD_STRING(data, "rx_vport_fdir_hits_packets");
            ZXDH_ADD_STRING(data, "rx_vport_fdir_hits_bytes");
            ZXDH_ADD_STRING(data, "rx_vport_fdir_drop_packets");
            ZXDH_ADD_STRING(data, "rx_vport_fdir_drop_bytes");

            ZXDH_ADD_STRING(data, "rx_lro_packets");
            ZXDH_ADD_STRING(data, "rx_udp_csum_fail_packets");
            ZXDH_ADD_STRING(data, "tx_udp_csum_fail_packets");
            ZXDH_ADD_STRING(data, "rx_tcp_csum_fail_packets");
            ZXDH_ADD_STRING(data, "tx_tcp_csum_fail_packets");
            ZXDH_ADD_STRING(data, "rx_ipv4_csum_fail_packets");
            ZXDH_ADD_STRING(data, "tx_ipv4_csum_fail_packets");

            ZXDH_ADD_STRING(data, "rx_packets_phy");//get stat from mac
            ZXDH_ADD_STRING(data, "tx_packets_phy");
            ZXDH_ADD_STRING(data, "rx_bytes_phy");
            ZXDH_ADD_STRING(data, "tx_bytes_phy");
            ZXDH_ADD_STRING(data, "rx_error_phy");
            ZXDH_ADD_STRING(data, "tx_error_phy");
            ZXDH_ADD_STRING(data, "rx_drop_phy");
            ZXDH_ADD_STRING(data, "tx_drop_phy");
            ZXDH_ADD_STRING(data, "rx_good_bytes_phy");
            ZXDH_ADD_STRING(data, "tx_good_bytes_phy");
            ZXDH_ADD_STRING(data, "rx_unicast_phy");
            ZXDH_ADD_STRING(data, "tx_unicast_phy");
            ZXDH_ADD_STRING(data, "rx_multicast_phy");
            ZXDH_ADD_STRING(data, "tx_multicast_phy");
            ZXDH_ADD_STRING(data, "rx_broadcast_phy");
            ZXDH_ADD_STRING(data, "tx_broadcast_phy");
            ZXDH_ADD_STRING(data, "rx_under64_drop");
            ZXDH_ADD_STRING(data, "rx_undersize_phy");
            ZXDH_ADD_STRING(data, "rx_size_64_phy");
            ZXDH_ADD_STRING(data, "rx_size_65_127");
            ZXDH_ADD_STRING(data, "rx_size_128_255");
            ZXDH_ADD_STRING(data, "rx_size_256_511");
            ZXDH_ADD_STRING(data, "rx_size_512_1023");
            ZXDH_ADD_STRING(data, "rx_size_1024_1518");
            ZXDH_ADD_STRING(data, "rx_size_1519_mru");
            ZXDH_ADD_STRING(data, "rx_oversize_phy");
            ZXDH_ADD_STRING(data, "tx_undersize_phy");
            ZXDH_ADD_STRING(data, "tx_size_64_phy");
            ZXDH_ADD_STRING(data, "tx_size_65_127");
            ZXDH_ADD_STRING(data, "tx_size_128_255");
            ZXDH_ADD_STRING(data, "tx_size_256_511");
            ZXDH_ADD_STRING(data, "tx_size_512_1023");
            ZXDH_ADD_STRING(data, "tx_size_1024_1518");
            ZXDH_ADD_STRING(data, "tx_size_1519_mtu");
            ZXDH_ADD_STRING(data, "tx_oversize_phy");
            ZXDH_ADD_STRING(data, "rx_pause_phy");
            ZXDH_ADD_STRING(data, "tx_pause_phy");
            ZXDH_ADD_STRING(data, "rx_crc_errors");
            ZXDH_ADD_STRING(data, "tx_crc_errors");
            ZXDH_ADD_STRING(data, "rx_mac_control_phy");
            ZXDH_ADD_STRING(data, "tx_mac_control_phy");
            ZXDH_ADD_STRING(data, "rx_fragment_phy");
            ZXDH_ADD_STRING(data, "tx_fragment_phy");
            ZXDH_ADD_STRING(data, "rx_jabber_phy");
            ZXDH_ADD_STRING(data, "tx_jabber_phy");
            ZXDH_ADD_STRING(data, "rx_vlan_phy");
            ZXDH_ADD_STRING(data, "tx_vlan_phy");
            ZXDH_ADD_STRING(data, "rx_eee_phy");
            ZXDH_ADD_STRING(data, "tx_eee_phy");
            ZXDH_ADD_STRING(data, "rx_arn_phy");
            ZXDH_ADD_STRING(data, "tx_psn_phy");
            ZXDH_ADD_STRING(data, "rx_psn_phy");
            ZXDH_ADD_STRING(data, "tx_psn_ack_phy");
            ZXDH_ADD_STRING(data, "rx_psn_ack_phy");

            for (i = 0; i < en_dev->max_vq_pairs; i++)
            {
                ZXDH_ADD_QUEUE_STRING(data, "rx_pkts", i);
                ZXDH_ADD_QUEUE_STRING(data, "tx_pkts", i);
                ZXDH_ADD_QUEUE_STRING(data, "rx_bytes", i);
                ZXDH_ADD_QUEUE_STRING(data, "tx_bytes", i);
                ZXDH_ADD_QUEUE_STRING(data, "tx_stopped", i);
                ZXDH_ADD_QUEUE_STRING(data, "tx_wake", i);
                ZXDH_ADD_QUEUE_STRING(data, "tx_dropped", i);
            }

            for(i = 0; i < 8; i++)
            {
                ZXDH_ADD_PRIO_STRING(data, "rx_pkts", i);
                ZXDH_ADD_PRIO_STRING(data, "rx_bytes", i);
            }

            for(i = 0; i < 8; i++)
            {
                ZXDH_ADD_PRIO_STRING(data, "tx_pkts", i);
                ZXDH_ADD_PRIO_STRING(data, "tx_bytes", i);
            }
            break;
        }
        case ETH_SS_PRIV_FLAGS:
        {
            for (i = 0; i < ZXDH_NUM_PFLAGS; i++)
            {
                strncpy(data + i * ETH_GSTRING_LEN, zxdh_gstrings_priv_flags[i].name, ETH_GSTRING_LEN);
            }
            break;
        }
        case ETH_SS_TEST:
            for (i = 0; i < zxdh_en_self_test_num(); i++)
            {
                strcpy(data + i * ETH_GSTRING_LEN, zxdh_self_tests[i]);
            }
            break;
        default:
        {
            LOG_ERR_DEV(en_dev->parent, "invalid para\n");
            break;
        }
    }

    return;
}

int32_t zxdh_pflags_update(struct net_device *netdev, uint8_t flag, bool enable)
{
    if (!zxdh_gstrings_priv_flags[flag].handler)
        return 0;
    return zxdh_gstrings_priv_flags[flag].handler(netdev, enable);
}

static int32_t zxdh_handle_pflag(struct net_device *netdev,
                                uint32_t wanted_flags,
                                enum zxdh_priv_flag flag)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    bool enable = !!(wanted_flags & BIT(flag));
    uint32_t changes = wanted_flags ^ en_priv->edev.pflags;
    int32_t err = 0;

    /* 判断设置的值是否改变&改变的位是否为flag位 */
    if (!(changes & BIT(flag)))
    {
        return 0;
    }

    err = zxdh_pflags_update(netdev, flag, enable);
    if (0 != err) {
        LOG_ERR_DEV(en_priv->edev.parent, "%s private flag '%s' failed err %d\n", \
               enable ? "Enable" : "Disable", zxdh_gstrings_priv_flags[flag].name, err);
        return err;
    }

    ZXDH_SET_PFLAG(en_priv->edev.pflags, flag, enable);

    return 0;
}


static int32_t zxdh_en_set_priv_flags(struct net_device *netdev, uint32_t pflags)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    enum zxdh_priv_flag pflag = 0;
    int32_t err = 0;
    uint32_t changes = pflags ^ en_priv->edev.pflags;
    bool hardware_bond_change = changes & BIT(ZXDH_PFLAG_HARDWARE_BOND);
    bool hardware_bond_prima_change = changes & BIT(ZXDH_PFLAG_HARDWARE_BOND_PRIMARY);
    LOG_INFO_DEV(en_dev->parent, "hardware_bond_change %d, hardware_bond_prima_change %d\n",
                hardware_bond_change, hardware_bond_prima_change);
    if (!(hardware_bond_change || hardware_bond_prima_change))
    {
        ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    }
    for (pflag = 0; pflag < ZXDH_NUM_PFLAGS; pflag++)
    {
        err = zxdh_handle_pflag(netdev, pflags, pflag);
        if (0 != err)
        {
            break;
        }
    }

    return err;
}

static void flag_enable_1588_get(struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;
    union zxdh_msg *msg = NULL;
    ZXDH_SRIOV_VPORT_T port_attr_entry = {0};
    struct zxdh_bar_extra_para para = {0};
    DPP_PF_INFO_T dpp_pf_info = {
        .slot = en_dev->slot_id,
        .vport = en_dev->vport,
    };

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    if (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
        if (msg == NULL)
        {
            LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
            return ;
        }

        msg->payload.vf_1588_enable.proc_cmd = ZXDH_VF_1588_ENABLE_GET;
        msg->payload.hdr.op_code = ZXDH_VF_1588_ENABLE;
        msg->payload.hdr.vport = en_dev->vport;
        msg->payload.hdr.pcie_id = en_dev->pcie_id;
        ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
        if(ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_send_command_to_pf failed: %d\n", ret);
            kfree(msg);
            return;
        }

        en_dev->enable_1588 = msg->reps.vf_1588_enable_rsp.enable_1588_vf_rsp;
        kfree(msg);
    }
    else
    {
        ret = dpp_vport_attr_get(&dpp_pf_info, &port_attr_entry);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_vport_attr_get SRIOV_VPORT_1588_EN failed, ret:%d\n", ret);
            return;
        }

        en_dev->enable_1588 = port_attr_entry.flag_1588_enable;
    }

    if (en_dev->enable_1588 == 0)
    {
        en_dev->pflags &= ~BIT(ZXDH_PFLAG_1588_ENABLE);
    }
    else
    {
        en_dev->pflags |= BIT(ZXDH_PFLAG_1588_ENABLE);
    }
}

static void flag_rsskey_ipid_get(struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;
    union zxdh_msg *msg = NULL;
    ZXDH_SRIOV_VPORT_T port_attr_entry = {0};
    struct zxdh_bar_extra_para para = {0};
    uint32_t enable_rsskey_ipid = 0;
    DPP_PF_INFO_T dpp_pf_info = {
        .slot = en_dev->slot_id,
        .vport = en_dev->vport,
    };

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;
    if (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
        if (msg == NULL)
        {
            LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
            return ;
        }

        msg->payload.vf_rsskey_ipid.proc_cmd = ZXDH_VF_RSSKEY_IPID_GET;
        msg->payload.hdr.op_code = ZXDH_VF_RSSKEY_IPID_ENABLE;
        msg->payload.hdr.vport = en_dev->vport;
        msg->payload.hdr.pcie_id = en_dev->pcie_id;
        ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
        if(ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_send_command_to_pf failed: %d\n", ret);
            kfree(msg);
            return;
        }

        enable_rsskey_ipid = msg->reps.vf_rsskey_ipid_rsp.enable_rsskey_ipid_rsp;
        kfree(msg);
    }
    else
    {
        ret = dpp_vport_attr_get(&dpp_pf_info, &port_attr_entry);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_vport_attr_get SRIOV_VPORT_FRAG_PKT_USE_IPID failed, ret:%d\n", ret);
            return;
        }

        enable_rsskey_ipid = port_attr_entry.frag_pkt_use_ipid;
    }

    if (enable_rsskey_ipid == 0)
    {
        en_dev->pflags &= ~BIT(ZXDH_PFLAG_RSSKEY_IPID);
    }
    else
    {
        en_dev->pflags |= BIT(ZXDH_PFLAG_RSSKEY_IPID);
    }
}

static uint32_t zxdh_en_get_priv_flags(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint32_t flag_lldp = 0;
    uint32_t lldp_mask = 0;
    uint32_t cpl_timeout_mask = 0;
    uint32_t hp_irq_ctrl_status = 0;
    int32_t ret = 0;

    ret = zxdh_lldp_enable_get(&en_priv->edev, &flag_lldp);
    if (ret != 0)
    {
        flag_lldp = flag_lldp << ZXDH_PFLAG_ENABLE_LLDP;

        lldp_mask = 0xFFFFFFFF ^ BIT(ZXDH_PFLAG_ENABLE_LLDP);
        en_priv->edev.pflags = (en_priv->edev.pflags & lldp_mask) | flag_lldp;
    }

    if(en_dev->is_hwbond)
    {
        en_priv->edev.pflags |= (1 << ZXDH_PFLAG_HARDWARE_BOND);
    }
    else
    {
        en_priv->edev.pflags &= ~(1 << ZXDH_PFLAG_HARDWARE_BOND);
    }
    if(en_dev->is_primary_port)
    {
        en_priv->edev.pflags |= (1 << ZXDH_PFLAG_HARDWARE_BOND_PRIMARY);
    }
    else
    {
        en_priv->edev.pflags &= ~(1 << ZXDH_PFLAG_HARDWARE_BOND_PRIMARY);
    }

    cpl_timeout_mask = en_dev->ops->get_cpl_timeout_if_mask(en_dev->parent);
    LOG_DEBUG_DEV(en_dev->parent, "cpl_timeout_mask: %d\n", cpl_timeout_mask);
    if (cpl_timeout_mask == 1)
        en_dev->pflags |= BIT(ZXDH_PFLAG_PCIE_AER_CPL_TIMEOUT);
    else
        en_dev->pflags &= ~BIT(ZXDH_PFLAG_PCIE_AER_CPL_TIMEOUT);

    hp_irq_ctrl_status = en_dev->ops->get_hp_irq_ctrl_status(en_dev->parent);
    LOG_DEBUG_DEV(en_dev->parent, "hp_irq_ctrl_status: %d\n", hp_irq_ctrl_status);
    if (hp_irq_ctrl_status == 1)
        en_dev->pflags |= BIT(ZXDH_PFLAG_PCIE_HP_IRQ_CTRL);
    else
        en_dev->pflags &= ~BIT(ZXDH_PFLAG_PCIE_HP_IRQ_CTRL);

    ret = zxdh_dual_tor_label_get(en_dev);
    if (ret == 1)
        en_dev->pflags |= BIT(ZXDH_PFLAG_DUAL_TOR_CTRL);
    else if (ret == 0)
        en_dev->pflags &= ~BIT(ZXDH_PFLAG_DUAL_TOR_CTRL);

    flag_enable_1588_get(en_dev);
    flag_rsskey_ipid_get(en_dev);

    return en_priv->edev.pflags;
}

static int zxdh_en_get_regs_len(struct net_device *netdev)
{
#define ZXDH_REGS_LEN (128 * 1024)
    return ZXDH_REGS_LEN * sizeof(uint32_t);
}

static void zxdh_en_get_regs(struct net_device *netdev, struct ethtool_regs *regs, void *p)
{

}

static void zxdh_en_get_wol(struct net_device *netdev, struct ethtool_wolinfo *wol)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    wol->supported = en_dev->wol_support;
    if (wol->supported == 0)
    {
        return;
    }
    wol->wolopts = en_dev->wolopts;
}

static int zxdh_en_set_wol(struct net_device *netdev, struct ethtool_wolinfo *wol)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    DPP_PF_INFO_T pf_info = {0};
    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    if((en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF) ||
       !zxdh_en_is_panel_port(en_dev))
    {
        return -EOPNOTSUPP;
    }

    LOG_INFO_DEV(en_dev->parent, "wol mode=0x%x, en_dev->phy_port=0x%x\n", wol->wolopts, en_dev->phy_port);
    if(en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        return -EOPNOTSUPP;
    }

    if (wol->wolopts & WAKE_MAGIC)
    {
        en_dev->wolopts = WAKE_MAGIC;
        dpp_uplink_phy_attr_set(&pf_info, en_dev->phy_port, UPLINK_PHY_PORT_MAGIC_PACKET_ENABLE, 1);
    }
    else if (wol->wolopts == 0)
    {
        en_dev->wolopts = 0;
        dpp_uplink_phy_attr_set(&pf_info, en_dev->phy_port, UPLINK_PHY_PORT_MAGIC_PACKET_ENABLE, 0);
    }
    else
    {
        return -EOPNOTSUPP;
    }

    return 0;
}

static uint32_t zxdh_en_get_msglevel(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    return en_dev->msglevel;
}

static void zxdh_en_set_msglevel(struct net_device *netdev, uint32_t data)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    en_dev->msglevel = data;
}

static int zxdh_en_nway_reset(struct net_device *netdev)
{
    return 0;
}

#ifdef HAVE_ETHTOOL_SET_PHYS_ID
static int zxdh_en_set_phys_id(struct net_device *netdev, enum ethtool_phys_id_state state)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int ret = 0;
    union zxdh_msg *msg = NULL;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    if (en_dev->phy_port == INVALID_PHY_PORT)
    {
        LOG_ERR_DEV(en_dev->parent, "phyport is invalid!");
        return -EOPNOTSUPP;
    }

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    switch (state)
    {
        case ETHTOOL_ID_ACTIVE:
        {
            msg->payload.mac_set_msg.blink_enable = 1;
            break;
        }
        case ETHTOOL_ID_INACTIVE:
        {
            msg->payload.mac_set_msg.blink_enable = 0;
            break;
        }
        default:
        {
            kfree(msg);
            return -EOPNOTSUPP;
        }
    }
    msg->payload.hdr_to_agt.op_code = AGENT_MAC_LED_BLINK;
    msg->payload.hdr_to_agt.phyport = en_dev->phy_port;
    LOG_DEBUG_DEV(en_dev->parent, "send phyport %d, blink_enable=%d\n", en_dev->phy_port, msg->payload.mac_set_msg.blink_enable);
    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_MAC, msg, msg, &para);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_send_command_to_riscv_mac failed, err: %d\n", ret);
    }
    kfree(msg);
    return ret;
}
#else
static int zxdh_en_phys_id(struct net_device *netdev, u32 data)
{
    return 0;
}
#endif /* HAVE_ETHTOOL_SET_PHYS_ID */

#ifdef HAVE_ETHTOOL_GET_SSET_COUNT
static int32_t zxdh_en_get_sset_count(struct net_device *netdev, int sset)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    switch (sset)
    {
        case ETH_SS_STATS:
        {
            return ZXDH_NET_PF_STATS_NUM(en_dev);
        }
        case ETH_SS_PRIV_FLAGS:
        {
            return ZXDH_NUM_PFLAGS;
        }
        case ETH_SS_TEST:
        {
            return zxdh_en_self_test_num();
        }
        default:
        {
            return -EOPNOTSUPP;
        }
    }

    return 0;
}
#endif


/* Started by AICoder, pid:n9b3e3b5e473d42143ee0ab66051003afbb63ce8 */
bool is_all_ff(const uint8_t *str)
{
    if (str == NULL) {
        return false;
    }

    while (*str != '\0') {
        if (*str != 0xff) {
            return false;
        }
        str++;
    }

    return true;
}
/* Ended by AICoder, pid:n9b3e3b5e473d42143ee0ab66051003afbb63ce8 */

void zxdh_format_firmware_version(uint8_t *vendor, uint8_t *input)
{
    uint8_t *v_pos;
    uint8_t *b_pos;
    uint8_t *last_dot;
    uint8_t version_len = 0;
    uint8_t version_part[VERSION_PART_LEN + 1] = {0};
    uint8_t build_part[BUILD_PART_LEN * 2] = {0};
    const uint8_t *vendor_str = NULL;
    int ret = 0;

    if (input == NULL || zte_strlen_s(input) == 0) {
        LOG_ERR("Invalid input");
        return;
    }

    vendor_str = vendor;

    v_pos = (uint8_t *)strstr(input, "-V");
    if (v_pos == NULL) {
        LOG_ERR("Invalid: No '-V' found in input:%s", input);
        return;
    }

    v_pos += 2;
    b_pos = (uint8_t *)strchr(v_pos, 'B');

    if (b_pos == NULL) {
        last_dot = (uint8_t *)strrchr(v_pos, '.');
        if (last_dot == NULL) {
            LOG_ERR("Invalid: No '.' found in input:%s", input);
            return;
        }
        version_len = last_dot - v_pos;
    } else {
        last_dot = (uint8_t *)strrchr(b_pos, '.');
        if (last_dot == NULL) {
            LOG_ERR("Invalid: No '.' found in input:%s", input);
            return;
        }
        version_len = b_pos - v_pos;
    }

    if (version_len >= VERSION_PART_LEN) {
        version_len = VERSION_PART_LEN;
    }

    zte_memcpy_s(version_part, v_pos, version_len);
    version_part[version_len] = '\0';

    if (b_pos == NULL) {
        uint8_t *build_start = last_dot + 1;
        uint8_t build_len = zte_strlen_s(build_start);

        if (build_len >= BUILD_PART_LEN) {
            build_len = BUILD_PART_LEN;
        }
        zte_memset_s(build_part, '0', BUILD_PART_LEN - build_len);
        zte_memcpy_s(build_part + BUILD_PART_LEN - build_len, build_start, build_len);
        build_part[BUILD_PART_LEN] = '\0';
    } else {
        uint8_t b_num_len = last_dot - (b_pos + 1);
        uint8_t *last_number = last_dot + 1;
        uint8_t last_num_len = zte_strlen_s(last_number);

        if (b_num_len >= BUILD_PART_LEN - 1) {
            b_num_len = BUILD_PART_LEN - 1;
        }
        if (last_num_len >= BUILD_PART_LEN) {
            last_num_len = BUILD_PART_LEN;
        }
        zte_memcpy_s(build_part, b_pos + 1, b_num_len);
        zte_memset_s(build_part + b_num_len, '0', BUILD_PART_LEN - last_num_len);
        zte_memcpy_s(build_part + b_num_len + (BUILD_PART_LEN - last_num_len), last_number, last_num_len);
        build_part[b_num_len + BUILD_PART_LEN] = '\0';
    }

    ret = zte_snprintf_s(input,  FW_VERSION_LEN - 1, "%s (%s_%s)", version_part, vendor_str, build_part);
    if (ret < 0) {
        LOG_ERR("format_version zte_snprintf_s %s failed, ret=%d\n", input, ret);
    }
    input[FW_VERSION_LEN - 1] = '\0';
}

static void zxdh_en_get_drvinfo(struct net_device *netdev, struct ethtool_drvinfo *drvinfo)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t ret = 0;
    uint8_t drv_name_len = 0;
    uint8_t drv_version[MAX_DRV_VERSION_LEN] = {0};
    uint8_t drv_version_len = 0;
    uint8_t vendor[VENDOR_SIZE] = {0};
    uint8_t fw_version[FW_VERSION_LEN] = {0};

    ret = en_dev->ops->get_pf_drv_msg(en_dev->parent, drv_version, &drv_version_len);
    if (drv_version_len > MAX_DRV_NAME_LEN)
    {
        LOG_ERR_DEV(en_dev->parent, "drv_version_len(%hhu) greater than %u", drv_version_len, MAX_DRV_NAME_LEN);
        drv_version_len = MAX_DRV_NAME_LEN;
    }

    drv_name_len = strlen(DRV_NAME);

    if (drv_name_len > MAX_DRV_NAME_LEN)
    {
        LOG_ERR_DEV(en_dev->parent, "drv_name_len(%hhu) greater than %u", drv_name_len, MAX_DRV_NAME_LEN);
        drv_name_len = MAX_DRV_NAME_LEN;
    }

    memcpy(drvinfo->driver, DRV_NAME, drv_name_len);
    memcpy(drvinfo->version, drv_version, drv_version_len);

    zte_strncpy_s(drvinfo->bus_info, dev_name(en_dev->parent->parent->device), sizeof(drvinfo->bus_info) -1);

    drvinfo->n_priv_flags = ZXDH_NUM_PFLAGS;
    drvinfo->n_stats = ZXDH_NET_PF_STATS_NUM(en_dev);
    drvinfo->eedump_len = zxdh_en_get_eeprom_len(netdev);
    drvinfo->regdump_len = zxdh_en_get_regs_len(netdev);
    drvinfo->testinfo_len = zxdh_en_self_test_num();

    zte_memset_s(fw_version, 0, FW_VERSION_LEN);
    en_dev->ops->get_vendor(en_dev->parent, vendor);

    if (is_all_ff(vendor)) {
        LOG_DEBUG_DEV(en_dev->parent, "use cached fw_version: %s", en_dev->fw_version);
        zte_strncpy_s(drvinfo->fw_version, en_dev->fw_version, FW_VERSION_LEN - 1);
        drvinfo->fw_version[FW_VERSION_LEN - 1] = '\0';
        return;
    }

    if (zte_strlen_s(vendor) == 0) {
        ret = zxdh_en_firmware_version_get(en_dev, fw_version);
        if (ret != 0) {
            LOG_ERR_DEV(en_dev->parent, "zxdh_en_firmware_version_get failed: %d\n", ret);
        }
    } else {
        en_dev->ops->get_fw_version(en_dev->parent, fw_version);
        zxdh_format_firmware_version(vendor, fw_version);
    }
    LOG_DEBUG_DEV(en_dev->parent, "netdev %s processed fw_version: %s", en_dev->netdev->name, fw_version);
    zte_strncpy_s(drvinfo->fw_version, fw_version, FW_VERSION_LEN - 1);
    drvinfo->fw_version[FW_VERSION_LEN - 1] = '\0';
    return;
}

int32_t zxdh_stats_update(struct zxdh_en_device *en_dev)
{
    uint16_t i = 0;
    int32_t ret = 0;

    ret = zxdh_vport_stats_get(en_dev, FALSE);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_vport_stats_get failed, ret: %d\n", ret);
        return -1;
    }

    ret = zxdh_mac_stats_get(en_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_mac_stats_get failed, ret: %d\n", ret);
        return -1;
    }

    ret = zxdh_en_udp_pkt_stats_get(en_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_en_udp_pkt_stats_get failed, ret: %d\n", ret);
        return -1;
    }

    en_dev->hw_stats.netdev_stats.rx_packets = 0;
    en_dev->hw_stats.netdev_stats.tx_packets = 0;
    en_dev->hw_stats.netdev_stats.rx_bytes = 0;
    en_dev->hw_stats.netdev_stats.tx_bytes = 0;
    en_dev->hw_stats.netdev_stats.tx_queue_wake = 0;
    en_dev->hw_stats.netdev_stats.tx_queue_stopped = 0;
    en_dev->hw_stats.netdev_stats.tx_queue_dropped = 0;
    en_dev->hw_stats.netdev_stats.rx_csum_unnecessary = 0;
    en_dev->hw_stats.netdev_stats.rx_removed_vlan_packets = 0;
    en_dev->hw_stats.netdev_stats.rx_csum_unnecessary_tunnel = 0;
    for (i = 0; i < en_dev->max_vq_pairs; i++)
    {
        /* queue software statistics */
        en_dev->hw_stats.q_stats[i].q_rx_pkts = en_dev->rq[i].stats.packets;
        en_dev->hw_stats.q_stats[i].q_tx_pkts = en_dev->sq[i].stats.packets;
        en_dev->hw_stats.q_stats[i].q_rx_bytes = en_dev->rq[i].stats.bytes;
        en_dev->hw_stats.q_stats[i].q_tx_bytes = en_dev->sq[i].stats.bytes;

        en_dev->hw_stats.netdev_stats.rx_packets += en_dev->rq[i].stats.packets;
        en_dev->hw_stats.netdev_stats.tx_packets += en_dev->sq[i].stats.packets;
        en_dev->hw_stats.netdev_stats.rx_bytes += en_dev->rq[i].stats.bytes;
        en_dev->hw_stats.netdev_stats.tx_bytes += en_dev->sq[i].stats.bytes;
        en_dev->hw_stats.netdev_stats.rx_csum_unnecessary += en_dev->rq[i].stats.rx_csum_unnecessary;
        en_dev->hw_stats.netdev_stats.rx_removed_vlan_packets += en_dev->rq[i].stats.rx_removed_vlan_packets;
        en_dev->hw_stats.netdev_stats.rx_csum_unnecessary_tunnel += en_dev->rq[i].stats.rx_csum_unnecessary_tunnel;
        en_dev->hw_stats.netdev_stats.tx_queue_wake += en_dev->hw_stats.q_stats[i].q_tx_wake;
        en_dev->hw_stats.netdev_stats.tx_queue_stopped += en_dev->hw_stats.q_stats[i].q_tx_stopped;
        en_dev->hw_stats.netdev_stats.tx_queue_dropped += en_dev->hw_stats.q_stats[i].q_tx_dropped;
    }

    return ret;
}

static void zxdh_en_get_ethtool_stats(struct net_device *netdev, struct ethtool_stats *stats, u64 *data)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint32_t offset_queue_stats = ZXDH_NETDEV_STATS_NUM + ZXDH_MAC_STATS_NUM + ZXDH_VPORT_STATS_NUM + ZXDH_UDP_STATS_NUM;
    uint32_t offset_idma_stats = offset_queue_stats + en_dev->max_vq_pairs * ZXDH_QUEUE_STATS_NUM;
    uint32_t offset_tm_odma_stats = offset_idma_stats + 8 * ZXDH_IDMA_STATS_NUM;

    if (en_dev->device_state == ZXDH_DEVICE_STATE_INTERNAL_ERROR)
        return;

    if (zxdh_stats_update(en_dev) != 0)
        return;

    memcpy(data, &en_dev->hw_stats, ZXDH_NET_PF_STATS_NUM(en_dev) * sizeof(uint64_t));
    memcpy(data + offset_queue_stats, en_dev->hw_stats.q_stats, (en_dev->max_vq_pairs * ZXDH_QUEUE_STATS_NUM) * sizeof(uint64_t));
    memcpy(data + offset_idma_stats, en_dev->hw_stats.idma_stats, 8 * ZXDH_IDMA_STATS_NUM * sizeof(uint64_t));
    memcpy(data + offset_tm_odma_stats, en_dev->hw_stats.tm_odma_stats, 8 * ZXDH_TM_ODMA_STATS_NUM * sizeof(uint64_t));

    return;
}

#if defined(ZXDH_ADAPT_REDHAT_9_6) || defined (HAVE_KERNEL_ETHTOOL_TS_INFO)
static int zxdh_en_get_ts_info(struct net_device *netdev, struct kernel_ethtool_ts_info *info)
{
    return 0;
}
#else
static int zxdh_en_get_ts_info(struct net_device *netdev, struct ethtool_ts_info *info)
{
    /* Started by AICoder, pid:3de164760c033b9146ed0b5d90d1440b13548a62 */
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    uint32_t ptp_clock_index;

    CHECK_EQUAL_ERR(netdev, NULL, -EADDRNOTAVAIL, "netdev is null!\n");

    en_priv = netdev_priv(netdev);
    CHECK_EQUAL_ERR(en_priv, NULL, -EADDRNOTAVAIL, "netdev priv is null!\n");
    en_dev = &en_priv->edev;
    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);

    /* Started by AICoder, pid:4a311a2f56c7b6e1476f0b5d90d1440b13548a62 */
    if(en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        return 0;
    }
    /* Ended by AICoder, pid:4a311a2f56c7b6e1476f0b5d90d1440b13548a62 */
#ifdef PTP_DRIVER_INTERFACE_EN
    {
        int32_t ret = 0;
        ret = zxdh_get_ptp_clock_index(en_dev, &ptp_clock_index);
        if(ret !=0)
        {
            return 0;
        }
    }
#else
    ptp_clock_index = 0; /* 3.10 内核未启用 PTP 时设置为 0 */
#endif /* PTP_DRIVER_INTERFACE_EN */

    info->so_timestamping = SOF_TIMESTAMPING_TX_HARDWARE |
                            SOF_TIMESTAMPING_RX_HARDWARE |
                            SOF_TIMESTAMPING_RAW_HARDWARE;

    info->phc_index = ptp_clock_index;

    info->tx_types = (1 << HWTSTAMP_TX_OFF) |
                    (1 << HWTSTAMP_TX_ON);

    info->rx_filters = (1 << HWTSTAMP_FILTER_NONE) |
                    (1 << HWTSTAMP_FILTER_ALL);
    /* Ended by AICoder, pid:3de164760c033b9146ed0b10407e2720681505ce */
    return 0;
}
#endif

#ifdef CONFIG_PM_RUNTIME
static int zxdh_en_ethtool_begin(struct net_device *netdev)
{
    return 0;
}

static void zxdh_en_ethtool_complete(struct net_device *netdev)
{

}
#endif

#ifndef HAVE_NDO_SET_FEATURES
static int zxdh_en_get_rx_csum(struct net_device *netdev)
{
    return 0;
}

static int zxdh_en_set_rx_csum(struct net_device *netdev, u32 data)
{
    return 0;
}

static int zxdh_en_set_tx_csum(struct net_device *netdev, u32 data)
{
    return 0;
}

#ifdef NETIF_F_TSO
static int zxdh_en_set_tso(struct net_device *netdev, u32 data)
{
    return 0;
}
#endif /* NETIF_F_TSO */

#ifdef ETHTOOL_GFLAGS
static int zxdh_en_set_flags(struct net_device *netdev, u32 data)
{
    return 0;
}
#endif /* ETHTOOL_GFLAGS */
#endif /* HAVE_NDO_SET_FEATURES */

#if defined(ZXDH_ADAPT_REDHAT_9_6) || defined(HAVE_ETHTOOL_KEEE)
static int zxdh_en_get_eee(struct net_device *netdev, struct ethtool_keee *edata)
#else
static int zxdh_en_get_eee(struct net_device *netdev, struct ethtool_eee *edata)
#endif
{
    return 0;
}

#if defined(ZXDH_ADAPT_REDHAT_9_6) || defined(HAVE_ETHTOOL_KEEE)
static int zxdh_en_set_eee(struct net_device *netdev, struct ethtool_keee *edata)
#else
static int zxdh_en_set_eee(struct net_device *netdev, struct ethtool_eee *edata)
#endif
{
    return 0;
}
#ifdef ETHTOOL_GRXFHINDIR
#ifdef HAVE_ETHTOOL_GRXFHINDIR_SIZE
static u32 zxdh_en_get_rxfh_indir_size(struct net_device *netdev)
{
    return ZXDH_INDIR_RQT_SIZE;
}

static u32 zxdh_en_get_rxfh_key_size(struct net_device *netdev)
{
    return ZXDH_NET_HASH_KEY_SIZE;
}

int zxdh_en_hash_key_get(struct zxdh_en_device *en_dev, uint8_t *key)
{
    union zxdh_msg *msg = NULL;
    int32_t ret = 0;
    DPP_PF_INFO_T pf_info = {0};
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL) {
        LOG_ERR_DEV(en_dev->parent, "malloc(%lu) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF) {
        pf_info.slot = en_dev->slot_id;
        pf_info.vport = en_dev->vport;
        ret = dpp_thash_key_get(&pf_info, key, ZXDH_NET_HASH_KEY_SIZE);
    } else {
        msg->payload.hdr.op_code = ZXDH_THASH_KEY_GET;
        msg->payload.hdr.vport = en_dev->vport;
        msg->payload.hdr.pcie_id = en_dev->pcie_id;
        ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
        if (ret == 0)
            zte_memcpy_s(key, msg->reps.thash_key_set_msg.key_map, ZXDH_NET_HASH_KEY_SIZE);
    }

    if (ret != 0 )
        LOG_ERR_DEV(en_dev->parent, "get hash key failed !\n");

    kfree(msg);
    return ret;
}

#ifdef ZXDH_ADAPT_REDHAT_9_6
static int zxdh_en_get_rxfh_by_param(struct net_device *netdev, struct ethtool_rxfh_param *rxfh)
{
    return 0;
}
#else
#if (defined(ETHTOOL_GRSSH) && !defined(HAVE_ETHTOOL_GSRSSH))
/* Started by AICoder, pid:n5fa5x7d0f67f981485e0b6a5000650937b8f1e7 */
#if (defined(ETHTOOL_USE_RXFH_PARAM))
static int zxdh_en_get_rxfh(struct net_device *netdev, struct ethtool_rxfh_param *rxfh_param)
{
    u32 *indir = rxfh_param->indir;
    u8 *key =  rxfh_param->key;
    u8 *hfunc = &rxfh_param->hfunc;
#else
/* Ended by AICoder, pid:n5fa5x7d0f67f981485e0b6a5000650937b8f1e7 */
#ifdef HAVE_RXFH_HASHFUNC
static int zxdh_en_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key, u8 *hfunc)
{
#else
static int zxdh_en_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key)
{
#endif /* HAVE_RXFH_HASHFUNC */
#endif
#else
static int zxdh_en_get_rxfh_indir(struct net_device *netdev, u32 *indir)
{
#endif /* HAVE_ETHTOOL_GSRSSH */

    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t ret = 0;
    uint8_t func = 0;

    LOG_DEBUG_DEV(en_dev->parent, "zxdh_en_get_rxfh start\n");
    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);

    if (indir != NULL)
    {
        memcpy(indir, en_dev->indir_rqt, sizeof(uint32_t) * ZXDH_INDIR_RQT_SIZE);
    }

    if (key != NULL)
    {
        LOG_DEBUG_DEV(en_dev->parent, "get key is called\n");
        ret = zxdh_en_hash_key_get(en_dev, key);
        if (ret)
            return -EOPNOTSUPP;
    }

    if (hfunc != NULL)
    {
        func = en_dev->eth_config.hash_func;
        switch (func)
        {
            case ZXDH_FUNC_TOP:
            {
                *hfunc = ETH_RSS_HASH_TOP;
                break;
            }
            case ZXDH_FUNC_XOR:
            {
                *hfunc = ETH_RSS_HASH_XOR;
                break;
            }
#ifndef CGS_V5_693
            case ZXDH_FUNC_CRC32:
            {
                *hfunc = ETH_RSS_HASH_CRC32;
                break;
            }
#endif
            default:
            {
                return -EOPNOTSUPP;
            }
        }
    }

    return 0;
}
#endif
#else
#ifndef ZXDH_ADAPT_REDHAT_9_6
static int zxdh_en_get_rxfh_indir(struct net_device *netdev, struct ethtool_rxfh_indir *indir)
{
    return 0;
}
#endif
#endif /* HAVE_ETHTOOL_GRXFHINDIR_SIZE */
#endif /* ETHTOOL_GRXFHINDIR */

int32_t zxdh_indir_to_queue_map(struct zxdh_en_device *en_dev, const uint32_t *indir)
{
    uint32_t *queue_map = NULL;
    int32_t err = 0;
    uint16_t i = 0;
    uint16_t j = 0;

    queue_map = kzalloc(ZXDH_INDIR_RQT_SIZE * sizeof(uint32_t), GFP_KERNEL);
    if (queue_map == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "queue_map is NULL\n");
        return -ENOMEM;
    }
    for (i = 0; i < ZXDH_INDIR_RQT_SIZE; i++)
    {
        j = indir[i];
        queue_map[i] = en_dev->phy_index[2 * j];
    }

    memcpy(en_dev->eth_config.queue_map, queue_map, ZXDH_INDIR_RQT_SIZE * sizeof(uint32_t));

    kfree(queue_map);
    return err;
}

int zxdh_en_hash_func_set(struct zxdh_en_device *en_dev, uint8_t func)
{
    union zxdh_msg *msg = NULL;
    int32_t ret = 0;
    DPP_PF_INFO_T pf_info = {0};
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL) {
        LOG_ERR_DEV(en_dev->parent, "malloc(%lu) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF) {
        pf_info.slot = en_dev->slot_id;
        pf_info.vport = en_dev->vport;
        ret = dpp_vport_hash_funcs_set(&pf_info, func);
    } else {
        msg->payload.hdr.op_code = ZXDH_HASH_FUNC_SET;
        msg->payload.hdr.vport = en_dev->vport;
        msg->payload.hdr.pcie_id = en_dev->pcie_id;
        msg->payload.hfunc_set_msg.func = func;
        ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    }

    if (ret == 0)
        en_dev->eth_config.hash_func = func;

    kfree(msg);
    return ret;
}

static int zxdh_en_hash_key_set(struct zxdh_en_device *en_dev, uint8_t *key)
{
    union zxdh_msg *msg = NULL;
    int32_t ret = 0;
    DPP_PF_INFO_T pf_info = {0};
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL) {
        LOG_ERR_DEV(en_dev->parent, "malloc(%lu) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF) {
        pf_info.slot = en_dev->slot_id;
        pf_info.vport = en_dev->vport;
        ret = dpp_thash_key_set(&pf_info, key, ZXDH_NET_HASH_KEY_SIZE);
    } else {
        msg->payload.hdr.op_code = ZXDH_THASH_KEY_SET;
        msg->payload.hdr.vport = en_dev->vport;
        msg->payload.hdr.pcie_id = en_dev->pcie_id;
        zte_memcpy_s(msg->payload.thash_key_set_msg.key_map, key, ZXDH_NET_HASH_KEY_SIZE);
        ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    }

    kfree(msg);
    return ret;
}

int32_t zxdh_en_hash_key_recover(struct zxdh_en_device *en_dev)
{
    uint8_t key[ZXDH_NET_HASH_KEY_SIZE] = {0};
    int32_t err = 0;

    err = zxdh_en_hash_key_get(en_dev, key);
    ZXDH_CHECK_RET_RETURN(err, "zxdh_en_hash_key_get failed: %d\n", err);

    err = zxdh_en_hash_key_set(en_dev, key);
    ZXDH_CHECK_RET_RETURN(err, "zxdh_en_hash_key_set failed: %d\n", err);

    return err;
}

#ifdef HAVE_ETHTOOL_GRXFHINDIR_SIZE
#ifdef ZXDH_ADAPT_REDHAT_9_6
static int zxdh_en_set_rxfh_by_param(struct net_device *netdev, struct ethtool_rxfh_param *rxfh, struct netlink_ext_ack * ack)
{
    return 0;
}
#else
#if (defined(ETHTOOL_GRSSH) && !defined(HAVE_ETHTOOL_GSRSSH))
#if (defined(ETHTOOL_USE_RXFH_PARAM))
static int zxdh_en_set_rxfh(struct net_device *netdev, struct ethtool_rxfh_param *rxfh_param, struct netlink_ext_ack *ack)
{
    u32 *indir = rxfh_param->indir;
    u8 *key =  rxfh_param->key;
    u8 hfunc = rxfh_param->hfunc;
#else
#ifdef HAVE_RXFH_HASHFUNC
static int zxdh_en_set_rxfh(struct net_device *netdev, const u32 *indir, const u8 *key, const u8 hfunc)
{
#else
static int zxdh_en_set_rxfh(struct net_device *netdev, const u32 *indir, const u8 *key)
{
#endif /* HAVE_RXFH_HASHFUNC */
#endif
#else
static int zxdh_en_set_rxfh_indir(struct net_device *netdev, const u32 *indir)
{
#endif /* HAVE_ETHTOOL_GSRSSH */
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t ret = 0;
    uint8_t func = 0;

    LOG_DEBUG_DEV(en_dev->parent, "zxdh_en_set_rxfh_indir start\n");
    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);

    switch (hfunc)
    {
        case ETH_RSS_HASH_NO_CHANGE:
        {
            break;
        }
        case ETH_RSS_HASH_TOP:
        {
            func = ZXDH_FUNC_TOP;
            break;
        }
        case ETH_RSS_HASH_XOR:
        {
            func = ZXDH_FUNC_XOR;
            break;
        }
#ifndef CGS_V5_693
        case ETH_RSS_HASH_CRC32:
        {
            func = ZXDH_FUNC_CRC32;
            break;
        }
#endif
        default:
        {
            return -EOPNOTSUPP;
        }
    }

    if ((hfunc != ETH_RSS_HASH_NO_CHANGE) && (func != en_dev->eth_config.hash_func))
    {
        LOG_DEBUG_DEV(en_dev->parent, "func: %u\n", func);
        ret = zxdh_en_hash_func_set(en_dev, func);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "hunc set failed: %d", ret);
            return -EOPNOTSUPP;
        }
    }

    if (indir != NULL)
    {
        LOG_DEBUG_DEV(en_dev->parent, "set indir is called\n");
        ret = zxdh_indir_to_queue_map(en_dev, indir);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "indir set failed: %d", ret);
            return -EOPNOTSUPP;
        }

        memcpy(en_dev->indir_rqt, indir, ZXDH_INDIR_RQT_SIZE * sizeof(uint32_t));
        ret = zxdh_rxfh_set(en_dev, en_dev->eth_config.queue_map);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_rxfh_set failed: %d\n", ret);
            return -EOPNOTSUPP;
        }
    }

    if (key != NULL) {
        LOG_DEBUG_DEV(en_dev->parent, "set thash key is called\n");
        ret = zxdh_en_hash_key_set(en_dev, (uint8_t *)key);
    }

    return ret;
}
#endif
#else
#ifndef ZXDH_ADAPT_REDHAT_9_6
static int zxdh_en_set_rxfh_indir(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
    return 0;
}
#endif
#endif
#ifdef ETHTOOL_GCHANNELS
static void zxdh_en_get_channels(struct net_device *netdev, struct ethtool_channels *ch)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    DPP_PF_INFO_T pf_info = {0};
    struct zxdh_bar_extra_para para ={0};

    LOG_DEBUG_DEV(en_dev->parent, "start\n");
    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    ch->max_rx = en_dev->max_vq_pairs;
    ch->max_tx = en_dev->max_vq_pairs;
    ch->max_combined = min_t(int, ch->max_rx, ch->max_tx);
    ch->combined_count = min_t(int, en_dev->eth_config.num_rxq, en_dev->eth_config.num_txq);
    ch->rx_count = en_dev->eth_config.num_rxq - ch->combined_count;
    ch->tx_count = en_dev->eth_config.num_txq - ch->combined_count;

    if (en_dev->device_state == ZXDH_DEVICE_STATE_INTERNAL_ERROR)
        return;

    if (en_dev->ops->is_bond(en_dev->parent))
        return;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return;
    }
    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        pf_info.slot = en_dev->slot_id;
        pf_info.vport = en_dev->vport;
        err = dpp_rxfh_get(&pf_info, msg->payload.rxfh_set_msg.queue_map, ZXDH_INDIR_RQT_SIZE);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_rxfh_get failed: %d\n", err);
            goto free_msg;
        }

        LOG_DEBUG_DEV(en_dev->parent, "*******pf_queue_map*******\n");
        zxdh_u32_array_print(msg->payload.rxfh_set_msg.queue_map, ZXDH_INDIR_RQT_SIZE);
    }
    else
    {
        msg->payload.hdr.op_code = ZXDH_RXFH_GET;
        msg->payload.hdr.vport = en_dev->vport;
        msg->payload.hdr.pcie_id = en_dev->pcie_id;
        err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_rxfh_get failed: %d\n", err);
            goto free_msg;
        }

        LOG_DEBUG_DEV(en_dev->parent, "*******vf_queue_map*******\n");
        zxdh_u32_array_print(msg->reps.rxfh_get_msg.queue_map, ZXDH_INDIR_RQT_SIZE);
    }
free_msg:
    kfree(msg);
}
#endif /* ETHTOOL_GCHANNELS */

int32_t zxdh_get_max_rxfh_channel(struct net_device *netdev, uint32_t *max)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint32_t dev_size = ZXDH_INDIR_RQT_SIZE;
    uint32_t current_max = 0;

    while (dev_size--)
        current_max = max(current_max, en_dev->indir_rqt[dev_size]);

    *max = current_max;
    return 0;
}

int32_t zxdh_num_channels_changed(struct zxdh_en_device *en_dev, uint16_t rxq_num)
{
    uint32_t *indir = NULL;
    int32_t err = 0;
    uint16_t i = 0;

    if (rxq_num == 0)
    {
        LOG_ERR_DEV(en_dev->parent, "rxq_num cannot be zero\n");
        return -1;
    }

    indir = kzalloc(sizeof(uint32_t) * ZXDH_INDIR_RQT_SIZE, GFP_KERNEL);
    if (unlikely(NULL == indir))
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    if (!netif_is_rxfh_configured(en_dev->netdev))
    {
        LOG_DEBUG_DEV(en_dev->parent, "indir_is_default\n");
        for (i = 0; i < ZXDH_INDIR_RQT_SIZE; ++i)
        {
            indir[i] = i % rxq_num;
        }

        err = zxdh_indir_to_queue_map(en_dev, indir);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_indir_to_queue_map failed: %d\n", err);
            kfree(indir);
            return err;
        }

        zte_memcpy_s(en_dev->indir_rqt, indir, ZXDH_INDIR_RQT_SIZE * sizeof(uint32_t));
        err = zxdh_rxfh_set(en_dev, en_dev->eth_config.queue_map);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_rxfh_set failed: %d\n", err);
            kfree(indir);
            return -EOPNOTSUPP;
        }
    }

    kfree(indir);

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF) {
        err = zxdh_vf_set_rx_num(en_dev, rxq_num);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_vf_set_rx_num failed %d\n", err);
        }
    }

    return set_feature_rxhash(en_dev, rxq_num != 1 ? true : false);
}

#ifdef ETHTOOL_SCHANNELS
static int zxdh_en_set_channels(struct net_device *netdev, struct ethtool_channels *ch)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint32_t curr_combined = 0;
    uint16_t curr_num_rxq = 0;
    uint16_t curr_num_txq = 0;
    uint16_t pre_num_rxq = 0;
    uint16_t pre_num_txq = 0;
    uint32_t max_rx_in_use = 0;
    int32_t ret = 0;

    LOG_DEBUG_DEV(en_dev->parent, "start\n");
    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    /* verify that the number of channels does not invalidate any current
     * flow director rules
     */
    //TODO

#ifndef CGS_V5_693
    if (en_dev->xdp_enabled)
    {
        LOG_ERR_DEV(en_dev->parent, "XDP is enabled, not support set channels\n");
        return -EOPNOTSUPP;
    }
#endif

    if (ch->other_count)
    {
        LOG_ERR_DEV(en_dev->parent, "not supported to config other_count\n");
        return -EINVAL;
    }

    LOG_DEBUG_DEV(en_dev->parent, "ch->combined_count=%d, ch->rx_count=%d, ch->tx_count=%d, num_rxq=%d, num_txq=%d, old_queue_pairs=%d, curr_queue_pairs=%d\n",
              ch->combined_count, ch->rx_count, ch->tx_count, en_dev->eth_config.num_rxq, en_dev->eth_config.num_txq,
              en_dev->old_queue_pairs, en_dev->curr_queue_pairs);

    curr_combined = min_t(int, en_dev->eth_config.num_rxq, en_dev->eth_config.num_txq);
    /* these checks are for cases where user didn't specify a particular
     * value on cmd line but we get non-zero value anyway via get_channels() */
    if (ch->rx_count == (en_dev->eth_config.num_rxq - curr_combined))
    {
        LOG_DEBUG_DEV(en_dev->parent, "ch->rx_count=0 enter\n");
        ch->rx_count = 0;
    }
    if (ch->tx_count == (en_dev->eth_config.num_txq - curr_combined))
    {
        LOG_DEBUG_DEV(en_dev->parent, "ch->tx_count=0 enter\n");
        ch->tx_count = 0;
    }
    if (ch->combined_count == curr_combined)
    {
        LOG_DEBUG_DEV(en_dev->parent, "ch->combined_count=0 enter\n");
        ch->combined_count = 0;
    }

    if (!(ch->combined_count || (ch->rx_count && ch->tx_count)))
    {
        LOG_ERR_DEV(en_dev->parent, "invalid para, please specify at least 1 rx and 1 tx queue\n");
        return -EINVAL;
    }

    if (netif_is_rxfh_configured(netdev) && !zxdh_get_max_rxfh_channel(netdev, &max_rx_in_use) &&
        ((ch->combined_count + ch->rx_count) <= max_rx_in_use))
    {
        LOG_ERR_DEV(en_dev->parent, "config rx qnum %d are too low for indir table settings %d\n", (ch->combined_count + ch->rx_count), max_rx_in_use);
        return -EINVAL;
    }

    curr_num_rxq = ch->rx_count + ch->combined_count;
    curr_num_txq = ch->tx_count + ch->combined_count;
    if ((ch->combined_count > ch->max_combined) || (curr_num_rxq > ch->max_rx) || (curr_num_txq > ch->max_tx))
    {
        LOG_ERR_DEV(en_dev->parent, "maximum allowed max_combined=%d, max_rx=%d, max_tx=%d. current config combined_count=%d, num_rxq=%d, num_txq=%d\n",
                ch->max_combined, ch->max_rx, ch->max_tx, ch->combined_count, curr_num_rxq, curr_num_txq);
        return -EINVAL;
    }

    pre_num_rxq = en_dev->eth_config.num_rxq;
    pre_num_txq = en_dev->eth_config.num_txq;
    en_dev->eth_config.num_rxq = ch->rx_count + ch->combined_count;
    en_dev->eth_config.num_txq = ch->tx_count + ch->combined_count;
    if ((ch->combined_count == en_dev->curr_queue_pairs) && (en_dev->eth_config.num_rxq == pre_num_rxq) && (en_dev->eth_config.num_txq == pre_num_txq))
    {
        LOG_INFO_DEV(en_dev->parent, "combined, rx and tx are exactly the same as last time\n");
        return 0;
    }

    ret = zxdh_num_channels_changed(en_dev, en_dev->eth_config.num_rxq);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_num_channels_changed failed: %d\n", ret);
        return -1;
    }

    en_dev->old_queue_pairs = en_dev->curr_queue_pairs;
    en_dev->curr_queue_pairs = min_t(int, en_dev->eth_config.num_rxq, en_dev->eth_config.num_txq);
    en_dev->eth_config.curr_combined = en_dev->curr_queue_pairs;
    LOG_INFO_DEV(en_dev->parent, "config ch->combined_count=%d, ch->rx_count=%d, ch->tx_count=%d, num_rxq=%d, num_txq=%d, old_queue_pairs=%d, curr_queue_pairs=%d\n",
             ch->combined_count, ch->rx_count, ch->tx_count, en_dev->eth_config.num_rxq, en_dev->eth_config.num_txq,
             en_dev->old_queue_pairs, en_dev->curr_queue_pairs);

    zxdh_set_default_xps_cpumasks(en_dev);
    netif_set_real_num_tx_queues(netdev, en_dev->eth_config.num_txq);
    netif_set_real_num_rx_queues(netdev, en_dev->eth_config.num_rxq);

    if (test_bit(ZXDH_DEVICE_STATE_OPENED, &en_dev->state))
    {
        zxdh_flow_map_update_sysfs(netdev, pre_num_txq);
    }

    return 0;
}
#endif

static int32_t zxdh_get_rss_hash(struct ethtool_rxnfc *cmd, struct zxdh_en_device *en_dev)
{
    union zxdh_msg *msg = NULL;
    uint32_t hash_mode = 0;
    int32_t ret = 0;
    DPP_PF_INFO_T pf_info = {0};

    struct zxdh_bar_extra_para para = {0};
    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        ret = dpp_vport_rx_flow_hash_get(&pf_info, &hash_mode);
    }
    else
    {
        msg->payload.hdr.op_code = ZXDH_RX_FLOW_HASH_GET;
        msg->payload.hdr.vport = en_dev->vport;
        msg->payload.hdr.pcie_id = en_dev->pcie_id;
        ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
        hash_mode = msg->reps.rx_flow_hash_set_msg.hash_mode;
    }
    if (ret != 0)
    {
        kfree(msg);
        return ret;
    }

    LOG_INFO_DEV(en_dev->parent, "hash_mode: %u\n", hash_mode);
    switch (hash_mode)
    {
        case ZXDH_NET_RX_FLOW_HASH_MV:
        {
            cmd->data = RXH_L2DA + RXH_VLAN;
            break;
        }
        case ZXDH_NET_RX_FLOW_HASH_SD:
        {
            cmd->data = RXH_IP_SRC + RXH_IP_DST;
            break;
        }
        case ZXDH_NET_RX_FLOW_HASH_SDT:
        {
            cmd->data = RXH_L3_PROTO + RXH_IP_SRC + RXH_IP_DST;
            break;
        }
        case ZXDH_NET_RX_FLOW_HASH_SDFN:
        {
            cmd->data = RXH_IP_SRC + RXH_IP_DST + RXH_L4_B_0_1 + RXH_L4_B_2_3;
            break;
        }
        case ZXDH_NET_RX_FLOW_HASH_SDFNT:
        {
            cmd->data = RXH_L3_PROTO + RXH_IP_SRC + RXH_IP_DST + RXH_L4_B_0_1 + RXH_L4_B_2_3;
            break;
        }
        default:
        {
            LOG_ERR_DEV(en_dev->parent, "invalid hash_mode\n");
            kfree(msg);
            return -1;
        }
    }
    kfree(msg);
    return 0;
}

static int32_t zxdh_ethtool_get_flow(struct zxdh_en_device *en_dev, struct ethtool_rxnfc *info, int32_t location)
{

    if (location < 0 || location >=  ETHTOOL_FD_MAX_NUM)
        return -EINVAL;

    if (!en_dev->fs.ethtool_fs[location].is_used) {
        return -ENOENT;
    } else {
        zte_memcpy_s(&info->fs, &en_dev->fs.ethtool_fs[location].rfs, sizeof(struct ethtool_rx_flow_spec));
    }

    return 0;
}

/* @rule_cnt: Number of rules to be affected
 * @rule_locs: Array of used rule locations
 */
static int32_t zxdh_ethtool_get_all_flows(struct zxdh_en_device *en_dev, struct ethtool_rxnfc *info, uint32_t *rule_locs)
{
    int32_t location = 0;
    int32_t idx = 0;
    int32_t err = 0;

    info->data = ETHTOOL_FD_MAX_NUM;

    LOG_INFO_DEV(en_dev->parent, "zxdh_ethtool_get_all_flows rule_cnt:%d\n", info->rule_cnt);

    while ((!err || err == -ENOENT) && idx < info->rule_cnt)
    {
        err = zxdh_ethtool_get_flow(en_dev, info, location);
        if (!err)
            rule_locs[idx++] = location; /* 成功找到流表，存入rule_locs */
        location++; /* 继续找下一条 */
    }
    if (info->rule_cnt > idx)
    {
        LOG_INFO_DEV(en_dev->parent, "zxdh_ethtool_get_all_flows idx:%d less than %d\n", idx, info->rule_cnt);
    }
    return err;
}

static int zxdh_en_get_rxnfc(struct net_device *netdev, struct ethtool_rxnfc *info,
#ifdef HAVE_ETHTOOL_GET_RXNFC_VOID_RULE_LOCS
                            void *rule_locs)
#else
                            u32 *rule_locs)
#endif
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t err = 0;

    switch (info->cmd)
    {
        case ETHTOOL_GRXRINGS:
            info->data = en_dev->eth_config.num_rxq;
            break;
        case ETHTOOL_GRXFH:
            err = zxdh_get_rss_hash(info, en_dev);
            break;
        case ETHTOOL_GRXCLSRLCNT:
            info->rule_cnt = en_dev->fs.tot_num_rules;
            break;
        case ETHTOOL_GRXCLSRULE:
            err = zxdh_ethtool_get_flow(en_dev, info, info->fs.location);
            break;
        case ETHTOOL_GRXCLSRLALL:
            err = zxdh_ethtool_get_all_flows(en_dev, info, rule_locs);
            break;
        default:
            err = -EOPNOTSUPP;
            break;
    }

    return err;
}

static int32_t validate_ethter(struct ethtool_rx_flow_spec *fs)
{
    struct ethhdr *eth_mask = &fs->m_u.ether_spec;
    int32_t ntuples = 0;

    if (!is_zero_ether_addr(eth_mask->h_dest))
        ntuples++;
    if (!is_zero_ether_addr(eth_mask->h_source))
        ntuples++;
    if (eth_mask->h_proto)
        ntuples++;

    LOG_INFO("current ethet_addr num is %d\n", ntuples);
    return ntuples;
}

static int32_t validate_tcpudp4(struct ethtool_rx_flow_spec *fs)
{
    struct ethtool_tcpip4_spec *l4_mask = &fs->m_u.tcp_ip4_spec;
    int ntuples = 0;

    if (l4_mask->tos) /* 目前还不支支持tos */
        return -EINVAL;
    if (l4_mask->ip4src)
        ntuples++;
    if (l4_mask->ip4dst)
        ntuples++;
    if (l4_mask->psrc)
        ntuples++;
    if (l4_mask->pdst)
        ntuples++;

    /* tcp4/udp4 flow: proto and ethtype is masked */
    ntuples += 2;

    LOG_INFO("current TCP/UDP4 num is %d\n", ntuples);
    return ntuples;
}

static int32_t validate_ip4(struct ethtool_rx_flow_spec *fs)
{
    struct ethtool_usrip4_spec *l3_mask = &fs->m_u.usr_ip4_spec;
    int32_t ntuples = 0;

    if (l3_mask->l4_4_bytes || l3_mask->tos ||
        fs->h_u.usr_ip4_spec.ip_ver != ETH_RX_NFC_IP4)
        return -EINVAL;
    if (l3_mask->ip4src)
        ntuples++;
    if (l3_mask->ip4dst)
        ntuples++;
    if (l3_mask->proto)
        ntuples++;

    /* ip4 flow: ethtype is masked */
    ntuples++;
    LOG_INFO("current Ipv4 num is %d\n", ntuples);
    return ntuples;
}

static int32_t validate_ip6(struct ethtool_rx_flow_spec *fs)
{
    struct ethtool_usrip6_spec *l3_mask = &fs->m_u.usr_ip6_spec;
    int32_t ntuples = 0;

    if (l3_mask->l4_4_bytes || l3_mask->tclass)
        return -EINVAL;
    if (!ipv6_addr_any((struct in6_addr *)l3_mask->ip6src))
        ntuples++;
    if (!ipv6_addr_any((struct in6_addr *)l3_mask->ip6dst))
        ntuples++;
    if (l3_mask->l4_proto)
        ntuples++;

    /* ip6 flow: ethtype is masked */
    ntuples++;
    LOG_INFO("current IPv6 flow-type num is %d\n", ntuples);
    return ntuples;
}

static int32_t validate_tcpudp6(struct ethtool_rx_flow_spec *fs)
{
    struct ethtool_tcpip6_spec *l4_mask = &fs->m_u.tcp_ip6_spec;
    int32_t ntuples = 0;

    if (l4_mask->tclass)
        return -EINVAL;
    if (!ipv6_addr_any((struct in6_addr *)l4_mask->ip6src))
        ntuples++;
    if (!ipv6_addr_any((struct in6_addr *)l4_mask->ip6dst))
        ntuples++;
    if (l4_mask->psrc)
        ntuples++;
    if (l4_mask->pdst)
        ntuples++;

    /* tcp6/udp6 flow: proto and ethtype is masked */
    ntuples += 2;
    LOG_INFO("current TCP/UDP6 flow-type num is %d\n", ntuples);
    return ntuples;
}

static int32_t validate_vlan(struct ethtool_rx_flow_spec *fs)
{
    int ntuples = 0;

    /* vlan_etype非0， 设置了Vlan */
    if (fs->m_ext.vlan_etype && ntohs(fs->h_ext.vlan_etype) != ETH_TYPE_VLAN)
        return -EINVAL;
    /* vlan_tci非0， 设置了vlan_tci匹配，下面判断Vlan id是否在有效范围  */
    if (fs->m_ext.vlan_tci) {
        if (ntohs(fs->h_ext.vlan_tci) >= VLAN_N_VID) {
            return -EINVAL;
        } else {
            ntuples++;
        }
    }
    LOG_INFO("current extra vlan flow num is %d\n", ntuples);
    return ntuples;
}

/* 假设 ethtool_flow_union 和 ethtool_flow_ext 已定义 */
void print_ethtool_rx_flow_spec(const struct ethtool_rx_flow_spec *fs)
{
    LOG_DEBUG("struct ethtool_rx_flow_spec:\n");
    LOG_DEBUG("  flow_type: 0x%08x\n", fs->flow_type);

    /* 解析 flow_type 的基本类型（屏蔽 FLOW_EXT 和 FLOW_MAC_EXT） */
    switch (fs->flow_type & ~(FLOW_EXT | FLOW_MAC_EXT)) {
    case TCP_V4_FLOW:
        LOG_DEBUG("TCP_V4_FLOW\n");
        break;
    case UDP_V4_FLOW:
        LOG_DEBUG("UDP_V4_FLOW\n");
        break;
    case TCP_V6_FLOW:
        LOG_DEBUG("TCP_V6_FLOW\n");
        break;
    case UDP_V6_FLOW:
        LOG_DEBUG("UDP_V6_FLOW\n");
        break;
    case IP_USER_FLOW:
        LOG_DEBUG("IP_USER_FLOW\n");
        break;
    case IPV6_USER_FLOW:
        LOG_DEBUG("IPV6_USER_FLOW\n");
        break;
    case ETHER_FLOW:
        LOG_DEBUG("ETHER_FLOW\n");
        break;
    default:
        LOG_DEBUG("UNKNOWN\n");
        break;
    }
    if (fs->flow_type & FLOW_EXT)
        LOG_DEBUG(" | FLOW_EXT\n");
    if (fs->flow_type & FLOW_MAC_EXT)
        LOG_DEBUG(" | FLOW_MAC_EXT\n");

    /* 打印 h_u 和 m_u，根据 flow_type 选择联合体成员 */
    if ((fs->flow_type & ~(FLOW_EXT | FLOW_MAC_EXT)) == TCP_V4_FLOW ||
        (fs->flow_type & ~(FLOW_EXT | FLOW_MAC_EXT)) == UDP_V4_FLOW ||
        (fs->flow_type & ~(FLOW_EXT | FLOW_MAC_EXT)) == IP_USER_FLOW) {
        LOG_DEBUG("  h_u.tcp_ip4_spec:\n");
        LOG_DEBUG("    ip4src: %pI4\n", &fs->h_u.tcp_ip4_spec.ip4src);
        LOG_DEBUG("    ip4dst: %pI4\n", &fs->h_u.tcp_ip4_spec.ip4dst);
        LOG_DEBUG("    psrc: %u\n", ntohs(fs->h_u.tcp_ip4_spec.psrc));
        LOG_DEBUG("    pdst: %u\n", ntohs(fs->h_u.tcp_ip4_spec.pdst));
        LOG_DEBUG("    tos: 0x%02x\n", fs->h_u.tcp_ip4_spec.tos);

        LOG_DEBUG("  m_u.tcp_ip4_spec:\n");
        LOG_DEBUG("    ip4src: 0x%08x\n", ntohl(fs->m_u.tcp_ip4_spec.ip4src));
        LOG_DEBUG("    ip4dst: 0x%08x\n", ntohl(fs->m_u.tcp_ip4_spec.ip4dst));
        LOG_DEBUG("    psrc: 0x%04x\n", ntohs(fs->m_u.tcp_ip4_spec.psrc));
        LOG_DEBUG("    pdst: 0x%04x\n", ntohs(fs->m_u.tcp_ip4_spec.pdst));
        LOG_DEBUG("    tos: 0x%02x\n", fs->m_u.tcp_ip4_spec.tos);
    } else if ((fs->flow_type & ~(FLOW_EXT | FLOW_MAC_EXT)) == ETHER_FLOW) {
        LOG_DEBUG("  h_u.ether_spec:\n");
        LOG_DEBUG("    h_dest: %02x:%02x:%02x:%02x:%02x:%02x\n",
               fs->h_u.ether_spec.h_dest[0], fs->h_u.ether_spec.h_dest[1],
               fs->h_u.ether_spec.h_dest[2], fs->h_u.ether_spec.h_dest[3],
               fs->h_u.ether_spec.h_dest[4], fs->h_u.ether_spec.h_dest[5]);
        LOG_DEBUG("    h_source: %02x:%02x:%02x:%02x:%02x:%02x\n",
               fs->h_u.ether_spec.h_source[0], fs->h_u.ether_spec.h_source[1],
               fs->h_u.ether_spec.h_source[2], fs->h_u.ether_spec.h_source[3],
               fs->h_u.ether_spec.h_source[4], fs->h_u.ether_spec.h_source[5]);
        LOG_DEBUG("    h_proto: 0x%04x\n", ntohs(fs->h_u.ether_spec.h_proto));

        LOG_DEBUG("  m_u.ether_spec:\n");
        LOG_DEBUG("    h_dest: %02x:%02x:%02x:%02x:%02x:%02x\n",
               fs->m_u.ether_spec.h_dest[0], fs->m_u.ether_spec.h_dest[1],
               fs->m_u.ether_spec.h_dest[2], fs->m_u.ether_spec.h_dest[3],
               fs->m_u.ether_spec.h_dest[4], fs->m_u.ether_spec.h_dest[5]);
        LOG_DEBUG("    h_source: %02x:%02x:%02x:%02x:%02x:%02x\n",
               fs->m_u.ether_spec.h_source[0], fs->m_u.ether_spec.h_source[1],
               fs->m_u.ether_spec.h_source[2], fs->m_u.ether_spec.h_source[3],
               fs->m_u.ether_spec.h_source[4], fs->m_u.ether_spec.h_source[5]);
        LOG_DEBUG("    h_proto: 0x%04x\n", ntohs(fs->m_u.ether_spec.h_proto));
    } else {
        LOG_DEBUG("  h_u/m_u: <Unsupported flow type>\n");
    }

    /* 打印 h_ext 和 m_ext */
    LOG_DEBUG("  h_ext:\n");
    if (fs->flow_type & FLOW_MAC_EXT) {
        LOG_DEBUG("    h_dest: %02x:%02x:%02x:%02x:%02x:%02x\n",
               fs->h_ext.h_dest[0], fs->h_ext.h_dest[1],
               fs->h_ext.h_dest[2], fs->h_ext.h_dest[3],
               fs->h_ext.h_dest[4], fs->h_ext.h_dest[5]);
    } else {
        LOG_DEBUG("    h_dest: <Disabled, no FLOW_MAC_EXT>\n");
    }
    if (fs->flow_type & FLOW_EXT) {
        LOG_DEBUG("    vlan_etype: 0x%04x\n", ntohs(fs->h_ext.vlan_etype));
        LOG_DEBUG("    vlan_tci: 0x%04x (VLAN ID: %u, Priority: %u)\n",
                ntohs(fs->h_ext.vlan_tci),
                ntohs(fs->h_ext.vlan_tci) & 0x0FFF,
                (ntohs(fs->h_ext.vlan_tci) >> 13) & 0x7);
        LOG_DEBUG("    data: 0x%08x 0x%08x\n",
               ntohl(fs->h_ext.data[0]), ntohl(fs->h_ext.data[1]));
    } else {
        LOG_DEBUG("    vlan_etype, vlan_tci, data: <Disabled, no FLOW_EXT>\n");
    }

    LOG_DEBUG("  m_ext:\n");
    if (fs->flow_type & FLOW_MAC_EXT) {
        LOG_DEBUG("h_dest: %02x:%02x:%02x:%02x:%02x:%02x\n",
               fs->m_ext.h_dest[0], fs->m_ext.h_dest[1],
               fs->m_ext.h_dest[2], fs->m_ext.h_dest[3],
               fs->m_ext.h_dest[4], fs->m_ext.h_dest[5]);
    } else {
        LOG_DEBUG("    h_dest: <Disabled, no FLOW_MAC_EXT>\n");
    }

    if (fs->flow_type & FLOW_EXT) {
        LOG_DEBUG("    vlan_etype: 0x%04x\n", ntohs(fs->m_ext.vlan_etype));
        LOG_DEBUG("    vlan_tci: 0x%04x\n", ntohs(fs->m_ext.vlan_tci));
        LOG_DEBUG("    data: 0x%08x 0x%08x\n",
               ntohl(fs->m_ext.data[0]), ntohl(fs->m_ext.data[1]));
    } else {
        LOG_DEBUG("    vlan_etype, vlan_tci, data: <Disabled, no FLOW_EXT>\n");
    }

    /* 打印 ring_cookie */
    LOG_DEBUG("  ring_cookie: 0x%llx\n", (unsigned long long)fs->ring_cookie);
    if (fs->ring_cookie == RX_CLS_FLOW_DISC) {
        LOG_DEBUG("DISCARD\n");
    } else {
        uint8_t vf = ethtool_get_flow_spec_ring_vf(fs->ring_cookie);
        uint32_t queue = ethtool_get_flow_spec_ring(fs->ring_cookie);

        if (vf) {
            LOG_DEBUG("Action: Direct to VF %u queue %u\n", vf - 1, queue);
        } else {
            LOG_DEBUG("Action: Direct to queue %u\n", queue);
        }
    }

    /* 打印 location */
    LOG_DEBUG("  location: %u\n", fs->location);

}

static int32_t validate_ring_cookie(struct ethtool_rx_flow_spec *fs,
                                        struct zxdh_en_device *en_dev)
{
    uint64_t ring_cookie = fs->ring_cookie;
    uint8_t vf_id = 0;
    uint32_t queue_id = 0;
    uint16_t max_rx_num = 0;
    struct zxdh_vf_item *vf_item = NULL;

    if (ring_cookie == RX_CLS_FLOW_DISC) {
        LOG_INFO_DEV(en_dev->parent, "fs->ring_cookie is 0x%llx, action is DISCARD\n", fs->ring_cookie);
        return 0;
    }
    LOG_INFO_DEV(en_dev->parent, "fs->ring_cookie is 0x%llx", fs->ring_cookie);

    vf_id = ethtool_get_flow_spec_ring_vf(fs->ring_cookie);
    queue_id = ethtool_get_flow_spec_ring(fs->ring_cookie);
    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF) {
        if (vf_id > 0) {
            LOG_ERR_DEV(en_dev->parent, "vf fd action do not support specific vf");
            return -EINVAL;
        }
        max_rx_num = en_dev->eth_config.num_rxq;
    } else {
        if (vf_id > 0) {
            vf_id --;
            LOG_INFO_DEV(en_dev->parent, "vf_id is %u\n", vf_id);
            vf_item = en_dev->ops->get_vf_item(en_dev->parent, vf_id);
            if (IS_ERR_OR_NULL(vf_item)) {
                LOG_ERR_DEV(en_dev->parent, "vif_item %d get failed", vf_id);
                return -EINVAL;
            }
            max_rx_num = vf_item->rx_num;
        } else {
            max_rx_num = en_dev->eth_config.num_rxq;
        }
    }
    if (queue_id == QUEUE_RSS) {
        LOG_INFO_DEV(en_dev->parent, "queue_id is 0xffff, use rss to distribute packets\n");
    } else if (queue_id >= max_rx_num && queue_id != QUEUE_RSS) {
        LOG_ERR_DEV(en_dev->parent, "queue_id is out of range %d\n", max_rx_num - 1);
        return -EINVAL;
    } else {
        LOG_INFO_DEV(en_dev->parent, "queue_id is %u\n", queue_id);
    }
    return 0;
}
/* 流规则校验 */
static int32_t validate_flow(struct zxdh_en_device *en_dev, struct ethtool_rx_flow_spec *fs)
{
    int32_t num_tuples = 0; /* 记录有效元组数量 */
    int32_t ret = 0;

    /* 调试打印，显示流类型 */
    print_ethtool_rx_flow_spec(fs);

    if (fs->location >=  ETHTOOL_FD_MAX_NUM)
        return -EINVAL;

    if (validate_ring_cookie(fs, en_dev)) {
        return -EINVAL;
    }

    /* 对流类型进行验证 */
    switch (fs->flow_type & ~(FLOW_EXT | FLOW_MAC_EXT)) /* 判断流基本类型，屏蔽扩展类型 */
    {
        case ETHER_FLOW:
            num_tuples += validate_ethter(fs);
            break;
        case TCP_V4_FLOW:
        case UDP_V4_FLOW:
            ret = validate_tcpudp4(fs);
            if (ret < 0)
                return ret;
            num_tuples += ret;
            break;
        case IP_USER_FLOW:
            ret = validate_ip4(fs);
            if (ret < 0)
                return ret;
            num_tuples += ret;
            break;
        case TCP_V6_FLOW:
        case UDP_V6_FLOW:
            ret = validate_tcpudp6(fs);
            if (ret < 0)
                return ret;
            num_tuples += ret;
            break;
        case IPV6_USER_FLOW:
            ret = validate_ip6(fs);
            if (ret < 0)
                return ret;
            num_tuples += ret;
            break;
        default:
            return -ENOTSUPP;
    }

    /* 校验Vlan扩展字段 */
    if ((fs->flow_type & FLOW_EXT))
    {
        ret = validate_vlan(fs);
        if (ret < 0)
            return ret;
        num_tuples += ret;
    }

    /* 校验mac扩展字段 */
    if ((fs->flow_type & FLOW_MAC_EXT) && (!is_zero_ether_addr(fs->m_ext.h_dest)))
        num_tuples++;

    /* For coverity */
    if (num_tuples > 0) {
        num_tuples = MAX_NUM_TUPLES;
    } else {
        num_tuples = 0;
    }
    return num_tuples;
}

int32_t zxdh_flow_table_pf_action_add(struct zxdh_en_device *en_dev,
                            struct ethtool_rx_flow_spec *fs, ZXDH_FD_CFG_T *p_fd_cfg)
{
    uint8_t vf_id = 0;
    uint32_t queue_id = 0;
    struct zxdh_vf_item *vf_item = NULL;
    DPP_PF_INFO_T pf_info = {0};
    uint32_t base_qid = 0;
    int32_t ret = 0;

    /* 默认打开cnt统计功能 */
    p_fd_cfg->as_rlt.action_index |= ACTION_TYPE_COUNT;

    if (fs->ring_cookie == RX_CLS_FLOW_DISC) {
        p_fd_cfg->as_rlt.action_index |= ACTION_TYPE_DROP;
        return 0;
    }

    vf_id = ethtool_get_flow_spec_ring_vf(fs->ring_cookie);
    queue_id = ethtool_get_flow_spec_ring(fs->ring_cookie);

    if (vf_id > 0) {
        vf_id--;
        vf_item = en_dev->ops->get_vf_item(en_dev->parent, vf_id);
        if (IS_ERR_OR_NULL(vf_item)) {
            LOG_ERR_DEV(en_dev->parent, "vif_item %d get failed", vf_id);
            return -EINVAL;
        }
        p_fd_cfg->as_rlt.action_index2 |= ACTION_TYPE_SPEC_PORT;
        p_fd_cfg->as_rlt.spec_port_vfid = EPID(vf_item->vport) * 256 + VFUNC_NUM(vf_item->vport);
        LOG_INFO_DEV(en_dev->parent, "zxdh_cfg_fd_add, vf_id %u, physical vf_id is %u",
                    vf_id, p_fd_cfg->as_rlt.spec_port_vfid);
        if (queue_id == QUEUE_RSS) {
            p_fd_cfg->as_rlt.action_index |= ACTION_TYPE_RSS;
            return 0;
        }

        pf_info.slot = en_dev->slot_id;
        pf_info.vport = vf_item->vport;

        ret = dpp_vport_base_qid_get(&pf_info, &base_qid);
        if (ret) {
            LOG_ERR_DEV(en_dev->parent, "zxdh_cfg_fd_add: get vf %u base qid failed", vf_id);
            return ret;
        }
        p_fd_cfg->as_rlt.v_qid = queue_id * 2 + base_qid;
        LOG_INFO_DEV(en_dev->parent, "zxdh_cfg_fd_add, vf %u, phy base qid is %u", vf_id, base_qid);
    } else {
        if (queue_id == QUEUE_RSS) {
            p_fd_cfg->as_rlt.action_index |= ACTION_TYPE_RSS;
            return 0;
        }
        p_fd_cfg->as_rlt.v_qid = en_dev->phy_index[queue_id * 2];
    }
    p_fd_cfg->as_rlt.action_index |= ACTION_TYPE_QUEUE;
    LOG_INFO_DEV(en_dev->parent, "zxdh_cfg_fd_add, phy queue id is %u", p_fd_cfg->as_rlt.v_qid);
    return 0;
}

static int32_t zxdh_cfg_np_fd(struct zxdh_en_device *en_dev, struct ethtool_rx_flow_spec *fs,
                                uint32_t *index)
{
    DPP_PF_INFO_T pf_info = {0};
    ZXDH_FD_CFG_T p_fd_cfg = {0};
    uint32_t handle = 0;
    uint32_t err = 0;

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    /* vf添加fd流表*/
    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF) {
        err = zxdh_vf_add_fd(en_dev, fs, index);
        if (err) {
            LOG_ERR_DEV(en_dev->parent, "zxdh_vf_add_fd failed\n");
            return -1;
        }
        return 0;
    }

    /* 填写fd表 */
    zxdh_flow_table_add(fs, &p_fd_cfg, &pf_info);
    err = zxdh_flow_table_pf_action_add(en_dev, fs, &p_fd_cfg);
    if (err) {
        LOG_ERR_DEV(en_dev->parent, "zxdh_cfg_fd_add_action failed");
        return -EINVAL;
    }

    /* 获取index */
    if (!en_dev->fs.ethtool_fs[fs->location].is_used) {
        /* 申请新的index */
        err = dpp_fd_acl_index_request(&pf_info, &handle);
        if (err) {
            LOG_ERR_DEV(en_dev->parent, "failed to request index!\n");
            return -ENOSPC;
        }
    } else {
        handle = en_dev->fs.ethtool_fs[fs->location].index;   /* 使用旧的index */
    }

    /* 统计功能使用的count_id为index */
    p_fd_cfg.as_rlt.count_id = handle;

    *index = handle;
    LOG_INFO_DEV(en_dev->parent, "fd index is %d\n", *index);

    /* 配置到np */
    err = dpp_tbl_fd_cfg_add(&pf_info, ZXDH_SDT_FD_CFG_TABLE, handle, &p_fd_cfg);
    if (err != 0) {
        LOG_ERR_DEV(en_dev->parent, "failed to add fd in np!\n");
        return -1;
    }

    return 0;
}

static void set_flow_table(struct zxdh_en_device *en_dev, struct ethtool_rx_flow_spec *fs, \
                                uint32_t index)
{
    /* 将流标规则存放在私有结构体中 */
    zte_memcpy_s(&en_dev->fs.ethtool_fs[fs->location].rfs, fs, sizeof(struct ethtool_rx_flow_spec));
    if (!en_dev->fs.ethtool_fs[fs->location].is_used) /* add */
    {
        en_dev->fs.ethtool_fs[fs->location].loc = fs->location;
        en_dev->fs.ethtool_fs[fs->location].index = index;
        en_dev->fs.ethtool_fs[fs->location].is_used = true;
        en_dev->fs.tot_num_rules++;
    }

    LOG_INFO_DEV(en_dev->parent, "set_flow_table: location is %u, index is %u\n", fs->location, index);
    return ;
}

static int32_t zxdh_ethtool_flow_replace(struct zxdh_en_device *en_dev, struct ethtool_rx_flow_spec *fs)
{
    int32_t num_tuples = 0;
    int32_t err = 0;
    uint32_t index = 0;

    /* 检查流规则是否有效 */
    num_tuples = validate_flow(en_dev, fs);
    if (num_tuples <= 0)
    {
        LOG_ERR_DEV(en_dev->parent, "flow is not valid %d\n", num_tuples);
        return -EINVAL;
    }

    /* 将流标规则配置到np中 */
    err = zxdh_cfg_np_fd(en_dev, fs, &index);
    if (err != 0) {
        LOG_ERR_DEV(en_dev->parent, "zxdh_cfg_np_fd failed!\n");
        return -EINVAL;
    }

    /* 将此fd流表存储在私有结构体中 */
    set_flow_table(en_dev, fs, index);
    return 0;
}

static int32_t zxdh_ethtool_flow_remove(struct zxdh_en_device *en_dev, int32_t location)
{
    uint32_t err = 0;
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    if (location < 0 || location >=  ETHTOOL_FD_MAX_NUM)
        return -EINVAL;

    if (!en_dev->fs.ethtool_fs[location].is_used)
    {
        LOG_ERR_DEV(en_dev->parent, "location %d is not used!!!\n", location);
        return -EINVAL;
    }

    /* 清除vf的fd表 */
    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF)
    {
        err = zxdh_vf_del_fd(en_dev, en_dev->fs.ethtool_fs[location].index);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_vf_del_fd failed!\n");
            return -EINVAL;
        }
        goto free_flow_table;
    }

    /* 清除np表项 */
    err = dpp_tbl_fd_cfg_del(&pf_info, ZXDH_SDT_FD_CFG_TABLE, en_dev->fs.ethtool_fs[location].index);
    if (err != 0) {
        LOG_ERR_DEV(en_dev->parent, "dpp_tbl_fd_cfg_del failed!\n");
        return -EINVAL;
    }

    /* 释放index */
    err = dpp_fd_acl_index_release(&pf_info, en_dev->fs.ethtool_fs[location].index);
    if (err)
    {
        LOG_ERR_DEV(en_dev->parent, "failed to release index!!!\n");
        return -EINVAL;
    }

free_flow_table:
    /* 清除私有结构体保存的信息*/
    zte_memset_s(&en_dev->fs.ethtool_fs[location], 0, sizeof(struct zxdh_ethtool_table));
    en_dev->fs.tot_num_rules--;
    return 0;
}

bool fw_has_unsupport_protocol_flag(struct zxdh_en_device *en_dev)
{
    struct dh_core_dev *dh_dev = en_dev->parent;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev->parent);
    return (pf_dev->mcode_feature & RSS_HASH_UNSUPPORT_PROTOCOL_FLAG) != 0;
}

uint32_t zxdh_get_default_hash_mode(struct zxdh_en_device *en_dev)
{
    struct dh_core_dev *dh_dev = en_dev->parent;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev->parent);
    return ZXDH_HASH_MODE_BY_MCODE_FLAG(pf_dev->mcode_feature);
}

int32_t zxdh_ethtool_rss_set(struct zxdh_en_device *en_dev, struct ethtool_rxnfc *cmd)
{
    union zxdh_msg *msg = NULL;
    uint32_t hash_mode = 0;
    int32_t ret = 0;

    DPP_PF_INFO_T pf_info = {0};
    struct zxdh_bar_extra_para para = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }

    switch (cmd->data)
    {
        case (RXH_L2DA + RXH_VLAN):
            hash_mode = ZXDH_NET_RX_FLOW_HASH_MV;
            break;
        case (RXH_L3_PROTO + RXH_IP_SRC + RXH_IP_DST):
            hash_mode = ZXDH_NET_RX_FLOW_HASH_SDT;
            break;
        case (RXH_L3_PROTO + RXH_IP_SRC + RXH_IP_DST + RXH_L4_B_0_1 + RXH_L4_B_2_3):
            hash_mode = ZXDH_NET_RX_FLOW_HASH_SDFNT;
            break;
        case (RXH_IP_SRC + RXH_IP_DST):
            if (!fw_has_unsupport_protocol_flag(en_dev))
                goto invalid_param;
            hash_mode = ZXDH_NET_RX_FLOW_HASH_SD;
            break;
        case (RXH_IP_SRC + RXH_IP_DST + RXH_L4_B_0_1 + RXH_L4_B_2_3):
            if (!fw_has_unsupport_protocol_flag(en_dev))
                goto invalid_param;
            hash_mode = ZXDH_NET_RX_FLOW_HASH_SDFN;
            break;
        default:
invalid_param:
            LOG_ERR_DEV(en_dev->parent,
                "invalid para, support %s\n",
                fw_has_unsupport_protocol_flag(en_dev) ? "mv, sd, sdt, sdfn, sdfnt" : "mv, sdt, sdfnt");
            ret = -EOPNOTSUPP;
            goto free_msg;
    }
    LOG_INFO_DEV(en_dev->parent, "hash_mode: %u\n", hash_mode);

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        ret = dpp_vport_rx_flow_hash_set(&pf_info, hash_mode);
    }
    else
    {
        msg->payload.hdr.op_code = ZXDH_RX_FLOW_HASH_SET;
        msg->payload.hdr.vport = en_dev->vport;
        msg->payload.hdr.pcie_id = en_dev->pcie_id;
        msg->payload.rx_flow_hash_set_msg.hash_mode = hash_mode;
        ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    }
    if (ret == 0)
        en_dev->eth_config.hash_mode = hash_mode;

free_msg:
    kfree(msg);
    return ret;
}

static int zxdh_en_set_rxnfc(struct net_device *netdev, struct ethtool_rxnfc *cmd)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t ret = 0;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);

    switch (cmd->cmd) {
    case ETHTOOL_SRXCLSRLINS:
        ret = zxdh_ethtool_flow_replace(en_dev, &cmd->fs);
        break;
    case ETHTOOL_SRXCLSRLDEL:
        ret = zxdh_ethtool_flow_remove(en_dev, cmd->fs.location);
        break;
    case ETHTOOL_SRXFH:
        ret = zxdh_ethtool_rss_set(en_dev, cmd);
        break;
    default:
        ret = -EOPNOTSUPP;
        break;
    }

    return ret;
}

/**
 * zxdh_en_fw_supports_new_coalesce - Check if firmware version supports
 *                                     new coalesce interface
 * @en_dev: Ethernet device
 *
 * Return: true if firmware patch >= DH_NEW_COALESCE_INTERFACE_PATCH,
 *         supports new interface; false otherwise
 */
static bool zxdh_en_fw_supports_new_coalesce(struct zxdh_en_device *en_dev)
{
    uint16_t fw_patch = 0;
    if (en_dev->ops) {
        fw_patch = en_dev->ops->get_fw_patch(en_dev->parent);
    }

    return (fw_patch >= DH_NEW_COALESCE_INTERFACE_PATCH);
}

/**
 * zxdh_en_get_coalesce_new - Get coalesce parameters from new interface
 * @en_dev: Ethernet device
 * @coal: ethtool coalesce structure
 * @rx_coalesce_usecs: Output parameter for RX coalesce usecs
 * @tx_coalesce_usecs: Output parameter for TX coalesce usecs
 * @rx_coalesced_frames: Output parameter for RX coalesced frames
 * @tx_coalesced_frames: Output parameter for TX coalesced frames
 *
 * Return: 0 on success, negative error code on failure
 */
static int32_t zxdh_en_get_coalesce_new(struct zxdh_en_device *en_dev,
                                        struct ethtool_coalesce *coal,
                                        uint32_t *rx_coalesce_usecs,
                                        uint32_t *tx_coalesce_usecs,
                                        uint16_t *rx_coalesced_frames,
                                        uint16_t *tx_coalesced_frames)
{
    int32_t err = 0;

    err = zxdh_get_coalesce_params(en_dev, rx_coalesce_usecs, tx_coalesce_usecs,
                                    rx_coalesced_frames, tx_coalesced_frames);
    if (err != 0)
    {
        LOG_ERR("zxdh_get_coalesce_params failed\n");
        return -1;
    }

    coal->rx_max_coalesced_frames = *rx_coalesced_frames;
    coal->tx_max_coalesced_frames = *tx_coalesced_frames;

    LOG_DEBUG("get rx_coalesced_frames is:%d\n", *rx_coalesced_frames);
    LOG_DEBUG("get tx_coalesced_frames is:%d\n", *tx_coalesced_frames);
    return 0;
}

/**
 * zxdh_en_get_coalesce_old - Get coalesce parameters from old interface
 * @en_dev: Ethernet device
 * @rx_coalesce_usecs: Output parameter for RX coalesce usecs
 * @tx_coalesce_usecs: Output parameter for TX coalesce usecs
 *
 * Return: 0 on success, negative error code on failure
 */
static int32_t zxdh_en_get_coalesce_old(struct zxdh_en_device *en_dev,
                                        uint32_t *rx_coalesce_usecs,
                                        uint32_t *tx_coalesce_usecs)
{
    int32_t err = 0;
    err = zxdh_get_coalesce_usecs(en_dev, rx_coalesce_usecs, tx_coalesce_usecs);
    if (err != 0)
    {
        LOG_ERR("zxdh_get_coalesce_usecs failed\n");
        return -1;
    }
    return 0;
}

#ifdef ETHTOOL_COALESCE_CFG
static int32_t zxdh_en_get_coalesce(struct net_device *netdev, struct ethtool_coalesce *coal,
                struct kernel_ethtool_coalesce *kec, struct netlink_ext_ack *ack)
#else
static int32_t zxdh_en_get_coalesce(struct net_device *netdev, struct ethtool_coalesce *coal)
#endif /* ETHTOOL_COALESCE_CFG */
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint32_t intr_adaptataion_flag = 0;
    uint32_t rx_coalesce_usecs = 0;
    uint32_t tx_coalesce_usecs = 0;
    int32_t err = 0;
    uint16_t rx_coalesced_frames = 0;
    uint16_t tx_coalesced_frames = 0;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    LOG_DEBUG_DEV(en_dev->parent, "zxdh_en_get_coalesce start\n");

    /* Only set adaptive parameters in non-low-latency scenarios; leave unset in low-latency scenarios to display N/A */
    if (!en_dev->is_lowlatency)
    {
        err = zxdh_get_misx_mode(en_dev, &intr_adaptataion_flag);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_get_misx_mode failed\n");
            return -1;
        }

        LOG_DEBUG_DEV(en_dev->parent, "get intr_adaptataion_flag is %d\n", intr_adaptataion_flag);
        coal->use_adaptive_rx_coalesce = intr_adaptataion_flag;
    }

    if (zxdh_en_fw_supports_new_coalesce(en_dev))
    {
        err = zxdh_en_get_coalesce_new(en_dev, coal, &rx_coalesce_usecs,
                                       &tx_coalesce_usecs, &rx_coalesced_frames,
                                       &tx_coalesced_frames);
    }
    else
    {
        err = zxdh_en_get_coalesce_old(en_dev, &rx_coalesce_usecs, &tx_coalesce_usecs);
    }
    if (err != 0)
    {
        return err;
    }

    LOG_DEBUG_DEV(en_dev->parent, "get rx_coalesce_usecs is:%d\n",  rx_coalesce_usecs);
    LOG_DEBUG_DEV(en_dev->parent, "get tx_coalesce_usecs is:%d\n",  tx_coalesce_usecs);

    coal->rx_coalesce_usecs = rx_coalesce_usecs;
    coal->tx_coalesce_usecs = tx_coalesce_usecs;

    LOG_DEBUG_DEV(en_dev->parent, "zxdh_en_get_coalesce end\n");
    return 0;
}

#ifdef ETHTOOL_COALESCE_CFG
static int32_t zxdh_en_set_coalesce(struct net_device *netdev, struct ethtool_coalesce *coal,
                struct kernel_ethtool_coalesce *kec, struct netlink_ext_ack *ack)
#else
static int32_t zxdh_en_set_coalesce(struct net_device *netdev, struct ethtool_coalesce *coal)
#endif /* ETHTOOL_COALESCE_CFG */
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint32_t rx_coalesce_usecs = coal->rx_coalesce_usecs;
    int32_t err = 0;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    LOG_DEBUG_DEV(en_dev->parent, "zxdh_en_set_coalesce start\n");

    /* Block configuration operations in low-latency firmware scenarios */
    if (en_dev->is_lowlatency)
    {
        LOG_DEBUG_DEV(en_dev->parent, "low latency firmware, coalesce config not supported\n");
        return -EOPNOTSUPP;
    }

    if (!ZXDH_COAL_USECS_VALID(coal->tx_coalesce_usecs) || !ZXDH_COAL_USECS_VALID(coal->rx_coalesce_usecs))
    {
        LOG_ERR_DEV(en_dev->parent, "tx_coalesce_usecs and rx_coalesce_usecs must be in [1, %u] usecs\n",
                    ZXDH_MAX_COAL_TIME);
        return -ERANGE;
    }

    /* Check max_coalesced_frames parameter when firmware version >= 5 */
    if (zxdh_en_fw_supports_new_coalesce(en_dev))
    {
        if (coal->rx_max_coalesced_frames != ZXDH_DEFAULT_MAX_COALESCED_FRAMES ||
            coal->tx_max_coalesced_frames != ZXDH_DEFAULT_MAX_COALESCED_FRAMES)
        {
            LOG_ERR_DEV(en_dev->parent, "max_coalesced_frames must be %u\n", ZXDH_DEFAULT_MAX_COALESCED_FRAMES);
            return -ERANGE;
        }
    }

    LOG_DEBUG_DEV(en_dev->parent, "cfg intr_adaptataion_flag is %d\n",  coal->use_adaptive_rx_coalesce);
    LOG_DEBUG_DEV(en_dev->parent, "cfg rx_coalesce_usecs is %d\n", coal->rx_coalesce_usecs);
    LOG_DEBUG_DEV(en_dev->parent, "cfg tx_coalesce_usecs is %d\n", coal->tx_coalesce_usecs);

    err = zxdh_cfg_misx_mode(en_dev, coal->use_adaptive_rx_coalesce);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_cfg_misx_mode failed\n");
        return err;
    }

    err = zxdh_cfg_coalesce_usecs(en_dev, rx_coalesce_usecs, coal->tx_coalesce_usecs);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_cfg_coalesce_usecs failed\n");
        return err;
    }

    return 0;
}

static const struct ethtool_ops zxdh_en_ethtool_ops =
{
#ifdef HAVE_ETHTOOL_COALESCE_PARAMS_SUPPORT
    .supported_coalesce_params = ETHTOOL_COALESCE_USECS |
                                ETHTOOL_COALESCE_RX_USECS |
                                ETHTOOL_COALESCE_TX_USECS |
                                ETHTOOL_COALESCE_USE_ADAPTIVE_RX,
#endif
    .get_coalesce = zxdh_en_get_coalesce,
    .set_coalesce = zxdh_en_set_coalesce,
    .get_drvinfo = zxdh_en_get_drvinfo,
    .get_link_ksettings = zxdh_en_get_link_ksettings,
    .set_link_ksettings = zxdh_en_set_link_ksettings,
    .get_regs_len = zxdh_en_get_regs_len,
    .get_regs = zxdh_en_get_regs,
    .get_wol = zxdh_en_get_wol,
    .set_wol = zxdh_en_set_wol,
    .get_msglevel = zxdh_en_get_msglevel,
    .set_msglevel = zxdh_en_set_msglevel,
    .nway_reset = zxdh_en_nway_reset,
    .get_link = zxdh_en_get_link,
    .get_eeprom_len = zxdh_en_get_eeprom_len,
    .get_eeprom = zxdh_en_get_eeprom,
    .set_eeprom = zxdh_en_set_eeprom,
    .get_ringparam = zxdh_en_get_ringparam,
    .set_ringparam = zxdh_en_set_ringparam,
    .get_pauseparam = zxdh_en_get_pauseparam,
    .set_pauseparam = zxdh_en_set_pauseparam,
#ifndef CGS_V5_693
    .get_fecparam = zxdh_en_get_fecparam,
    .set_fecparam = zxdh_en_set_fecparam,
#endif
    .get_module_info   = zxdh_en_get_module_info,
    .get_module_eeprom = zxdh_en_get_module_eeprom,
#ifdef HAVE_ETHTOOL_GET_MODULE_EEPROM_BY_PAGE
    .get_module_eeprom_by_page = zxdh_en_get_module_eeprom_by_page,
#endif
    .self_test = zxdh_en_diag_test,
    .get_strings = zxdh_en_get_strings,
    .get_priv_flags = zxdh_en_get_priv_flags,
    .set_priv_flags = zxdh_en_set_priv_flags,
#ifndef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
#ifdef HAVE_ETHTOOL_SET_PHYS_ID
    .set_phys_id = zxdh_en_set_phys_id,
#else
    .phys_id = zxdh_en_phys_id,
#endif /* HAVE_ETHTOOL_SET_PHYS_ID */
#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */

#ifdef HAVE_ETHTOOL_GET_SSET_COUNT
    .get_sset_count = zxdh_en_get_sset_count,
#endif
    .get_ethtool_stats = zxdh_en_get_ethtool_stats,

#ifndef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
#ifdef HAVE_ETHTOOL_GET_TS_INFO
    .get_ts_info = zxdh_en_get_ts_info,
#endif /* HAVE_ETHTOOL_GET_TS_INFO */
#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */
#ifdef CONFIG_PM_RUNTIME
    .begin = zxdh_en_ethtool_begin,
    .complete = zxdh_en_ethtool_complete,
#endif /* CONFIG_PM_RUNTIME */
#ifndef HAVE_NDO_SET_FEATURES
    .get_rx_csum = zxdh_en_get_rx_csum,
    .set_rx_csum = zxdh_en_set_rx_csum,
    .set_tx_csum = zxdh_en_set_tx_csum,
#ifdef NETIF_F_TSO
    .set_tso = zxdh_en_set_tso,
#endif
#ifdef ETHTOOL_GFLAGS
    .set_flags = zxdh_en_set_flags,
#endif /* ETHTOOL_GFLAGS */
#endif /* HAVE_NDO_SET_FEATURES */

#ifndef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
#ifdef ETHTOOL_GEEE
    .get_eee = zxdh_en_get_eee,
#endif
#ifdef ETHTOOL_SEEE
    .set_eee = zxdh_en_set_eee,
#endif
#ifdef ETHTOOL_GRXFHINDIR
#ifdef HAVE_ETHTOOL_GRXFHINDIR_SIZE
    .get_rxfh_indir_size = zxdh_en_get_rxfh_indir_size,
    .get_rxfh_key_size = zxdh_en_get_rxfh_key_size,
#endif /* HAVE_ETHTOOL_GRSFHINDIR_SIZE */
#if (defined(ETHTOOL_GRSSH) && !defined(HAVE_ETHTOOL_GSRSSH))
    .get_rxfh = zxdh_en_get_rxfh,
#else
#ifdef ZXDH_ADAPT_REDHAT_9_6
    .get_rxfh = zxdh_en_get_rxfh_by_param,
#else
    .get_rxfh_indir = zxdh_en_get_rxfh_indir,
#endif
#endif /* HAVE_ETHTOOL_GSRSSH */
#endif /* ETHTOOL_GRXFHINDIR */
#ifdef ETHTOOL_SRXFHINDIR
#if (defined(ETHTOOL_GRSSH) && !defined(HAVE_ETHTOOL_GSRSSH))
    .set_rxfh = zxdh_en_set_rxfh,
#else
#ifdef ZXDH_ADAPT_REDHAT_9_6
    .set_rxfh = zxdh_en_set_rxfh_by_param,
#else
    .set_rxfh_indir = zxdh_en_set_rxfh_indir,
#endif
#endif /* HAVE_ETHTOOL_GSRSSH */
#endif /* ETHTOOL_SRXFHINDIR */
#ifdef ETHTOOL_GCHANNELS
    .get_channels = zxdh_en_get_channels,
#endif /* ETHTOOL_GCHANNELS */
#ifdef ETHTOOL_SCHANNELS
    .set_channels = zxdh_en_set_channels,
#endif /* ETHTOOL_SCHANNELS */
#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */
#ifdef ETHTOOL_GRXFH
    .get_rxnfc = zxdh_en_get_rxnfc,
    .set_rxnfc = zxdh_en_set_rxnfc,
#endif
};

#ifdef HAVE_ETHTOOL_COALESCE_PARAMS_SUPPORT
/**
 * zxdh_en_adjust_ethtool_ops - Adjust ethtool_ops based on low-latency scenario and firmware version
 * @netdev: Network device
 *
 * If in low-latency scenario, remove ETHTOOL_COALESCE_USE_ADAPTIVE_RX flag.
 * Add ETHTOOL_COALESCE_RX_MAX_FRAMES and ETHTOOL_COALESCE_TX_MAX_FRAMES flags
 * based on firmware version.
 *
 * Note: Each net_device has its own ethtool_ops storage in zxdh_en_priv,
 * avoiding concurrency issues when multiple devices call this function.
 */
static void zxdh_en_adjust_ethtool_ops(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    /* Copy from default ops to per-device storage */
    zte_memcpy_s(&en_priv->ethtool_ops_adjusted, &zxdh_en_ethtool_ops, sizeof(struct ethtool_ops));

    /* First check if it's a low-latency scenario */
    if (en_dev->ops != NULL && en_dev->ops->is_lowlatency != NULL &&
        en_dev->ops->is_lowlatency(en_dev->parent))
    {
        en_priv->ethtool_ops_adjusted.supported_coalesce_params &= ~ETHTOOL_COALESCE_USE_ADAPTIVE_RX;
    }

    /* Then check if firmware version supports new interface */
    if (zxdh_en_fw_supports_new_coalesce(en_dev))
    {
        en_priv->ethtool_ops_adjusted.supported_coalesce_params |= ETHTOOL_COALESCE_RX_MAX_FRAMES;
        en_priv->ethtool_ops_adjusted.supported_coalesce_params |= ETHTOOL_COALESCE_TX_MAX_FRAMES;
    }

    netdev->ethtool_ops = &en_priv->ethtool_ops_adjusted;
}
#endif

#ifdef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
static const struct ethtool_ops_ext zxdh_en_ethtool_ops_ext =
{
    .size = sizeof(struct ethtool_ops_ext),
    .get_ts_info = zxdh_en_get_ts_info,
    .set_phys_id = zxdh_en_set_phys_id,
    .get_eee = zxdh_en_get_eee,
    .set_eee = zxdh_en_set_eee,
#ifdef HAVE_ETHTOOL_GRXFHINDIR_SIZE
    .get_rxfh_indir_size = zxdh_en_get_rxfh_indir_size,
#endif /* HAVE_ETHTOOL_GRSFHINDIR_SIZE */
    .get_rxfh_indir = zxdh_en_get_rxfh_indir,
    .set_rxfh_indir = zxdh_en_set_rxfh_indir,
    .get_channels = zxdh_en_get_channels,
    .set_channels = zxdh_en_set_channels,
};

void zxdh_en_set_ethtool_ops_ext(struct net_device *netdev)
{
    /* Set default first */
    netdev->ethtool_ops = &zxdh_en_ethtool_ops;

#ifdef HAVE_ETHTOOL_COALESCE_PARAMS_SUPPORT
    /* Adjust ethtool_ops for low-latency scenario and firmware version */
    zxdh_en_adjust_ethtool_ops(netdev);
#endif
    set_ethtool_ops_ext(netdev, &zxdh_en_ethtool_ops_ext);
}
#else
void zxdh_en_set_ethtool_ops(struct net_device *netdev)
{
    /* Set default first */
    netdev->ethtool_ops = &zxdh_en_ethtool_ops;

#ifdef HAVE_ETHTOOL_COALESCE_PARAMS_SUPPORT
    /* Adjust ethtool_ops for low-latency scenario and firmware version */
    zxdh_en_adjust_ethtool_ops(netdev);
#endif
}
#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */

