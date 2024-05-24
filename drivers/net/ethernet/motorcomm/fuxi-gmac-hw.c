/*++

Copyright (c) 2021 Motorcomm, Inc.
Motorcomm Confidential and Proprietary.

This is Motorcomm NIC driver relevant files. Please don't copy, modify,
distribute without commercial permission.

--*/

#include "fuxi-gmac.h"
#include "fuxi-gmac-reg.h"
#include "fuxi-efuse.h"

#if defined(UEFI)
#include "nic_sw.h"
#elif defined(_WIN64) || defined(_WIN32)
#include "fuxi-mp.h"
#elif defined(PXE)
#include "fuxi_undi.h"
#endif


void fxgmac_release_phy(struct fxgmac_pdata* pdata);
static void fxgmac_pwr_clock_ungate(struct fxgmac_pdata* pdata);
static void fxgmac_pwr_clock_gate(struct fxgmac_pdata* pdata);

static int fxgmac_tx_complete(struct fxgmac_dma_desc *dma_desc)
{
#ifdef LINUX
#if (FXGMAC_DUMMY_TX_DEBUG)
    return 1;
#endif
#endif
    return !FXGMAC_GET_REG_BITS_LE(dma_desc->desc3,
                TX_NORMAL_DESC3_OWN_POS,
                TX_NORMAL_DESC3_OWN_LEN);
}

static int fxgmac_disable_rx_csum(struct fxgmac_pdata *pdata)
{
    u32 regval;

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_CR);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_IPC_POS,
                     MAC_CR_IPC_LEN, 0);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_CR);

    DPRINTK("fxgmac disable rx checksum.\n");
    return 0;
}

static int fxgmac_enable_rx_csum(struct fxgmac_pdata *pdata)
{
    u32 regval;

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_CR);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_IPC_POS,
                     MAC_CR_IPC_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_CR);

    DPRINTK("fxgmac enable rx checksum.\n");
    return 0;
}

static int fxgmac_set_mac_address(struct fxgmac_pdata *pdata, u8 *addr)
{
    unsigned int mac_addr_hi, mac_addr_lo;

    mac_addr_hi = (addr[5] <<  8) | (addr[4] <<  0);
    mac_addr_lo = (addr[3] << 24) | (addr[2] << 16) |
              (addr[1] <<  8) | (addr[0] <<  0);

    writereg(pdata->pAdapter, mac_addr_hi, pdata->mac_regs + MAC_MACA0HR);
    writereg(pdata->pAdapter, mac_addr_lo, pdata->mac_regs + MAC_MACA0LR);

    return 0;
}

#if !defined(DPDK)
static void fxgmac_set_mac_reg(struct fxgmac_pdata *pdata,
                   struct netdev_hw_addr *ha,
                   unsigned int *mac_reg)
{
    unsigned int mac_addr_hi, mac_addr_lo;
    u8 *mac_addr;

    mac_addr_lo = 0;
    mac_addr_hi = 0;

    if (ha) {
        mac_addr = (u8 *)&mac_addr_lo;
        mac_addr[0] = ha->addr[0];
        mac_addr[1] = ha->addr[1];
        mac_addr[2] = ha->addr[2];
        mac_addr[3] = ha->addr[3];
        mac_addr = (u8 *)&mac_addr_hi;
        mac_addr[0] = ha->addr[4];
        mac_addr[1] = ha->addr[5];

        netif_dbg(pdata, drv, pdata->netdev,
              "adding mac address %pM at %#x\n",
              ha->addr, *mac_reg);

        mac_addr_hi = FXGMAC_SET_REG_BITS(mac_addr_hi,
                          MAC_MACA1HR_AE_POS,
                        MAC_MACA1HR_AE_LEN,
                        1);
    }

    writereg(pdata->pAdapter, mac_addr_hi, pdata->mac_regs + *mac_reg);
    *mac_reg += MAC_MACA_INC;
    writereg(pdata->pAdapter, mac_addr_lo, pdata->mac_regs + *mac_reg);
    *mac_reg += MAC_MACA_INC;
}
#endif

static int fxgmac_enable_tx_vlan(struct fxgmac_pdata *pdata)
{
    u32 regval;

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_VLANIR);
    /* Indicate that VLAN Tx CTAGs come from mac_vlan_incl register */
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANIR_VLTI_POS,
                     MAC_VLANIR_VLTI_LEN, 0);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANIR_CSVL_POS,
                     MAC_VLANIR_CSVL_LEN, 0);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANIR_VLP_POS,
                     MAC_VLANIR_VLP_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANIR_VLC_POS,
                     MAC_VLANIR_VLC_LEN, 2);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANIR_VLT_POS,
                     MAC_VLANIR_VLT_LEN, pdata->vlan);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_VLANIR);

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_VLANTR);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANTR_VL_POS,
                     MAC_VLANTR_VL_LEN, pdata->vlan);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_VLANTR);

    return 0;
}

static int fxgmac_disable_tx_vlan(struct fxgmac_pdata *pdata)
{
    u32 regval;

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_VLANIR);

    /* Indicate that VLAN Tx CTAGs come from mac_vlan_incl register */
    //Set VLAN Tag input enable
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANIR_CSVL_POS,
                     MAC_VLANIR_CSVL_LEN, 0);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANIR_VLTI_POS,
                     MAC_VLANIR_VLTI_LEN, /*0*/1);
    //Set VLAN priority control disable
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANIR_VLP_POS,
                     MAC_VLANIR_VLP_LEN, /*1*/0);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANIR_VLC_POS,
                     MAC_VLANIR_VLC_LEN, 0);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_VLANIR);

    /* In order for the VLAN Hash Table filtering to be effective,
     * the VLAN tag identifier in the VLAN Tag Register must not
     * be zero.  Set the VLAN tag identifier to "1" to enable the
     * VLAN Hash Table filtering.  This implies that a VLAN tag of
     * 1 will always pass filtering.
     */
    //2022-04-19 xiaojiang comment
    //If it enable the following code and enable vlan filter, then the MAC only receive the packet of VLAN ID "1"
    /*regval = readreg(pdata->mac_regs + MAC_VLANTR);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANTR_VL_POS,
                     MAC_VLANTR_VL_LEN, 1);
    writereg(regval, pdata->mac_regs + MAC_VLANTR);*/

    return 0;
}

static int fxgmac_enable_rx_vlan_stripping(struct fxgmac_pdata *pdata)
{
    u32 regval;

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_VLANTR);
    /* Put the VLAN tag in the Rx descriptor */
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANTR_EVLRXS_POS,
                     MAC_VLANTR_EVLRXS_LEN, 1);
    /* Don't check the VLAN type */
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANTR_DOVLTC_POS,
                     MAC_VLANTR_DOVLTC_LEN, 1);
    /* Check only C-TAG (0x8100) packets */
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANTR_ERSVLM_POS,
                     MAC_VLANTR_ERSVLM_LEN, 0);
    /* Don't consider an S-TAG (0x88A8) packet as a VLAN packet */
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANTR_ESVL_POS,
                     MAC_VLANTR_ESVL_LEN, 0);
    /* Enable VLAN tag stripping */
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANTR_EVLS_POS,
                     MAC_VLANTR_EVLS_LEN, 0x3);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_VLANTR);
    DPRINTK("fxgmac enable MAC rx vlan stripping.\n");

    return 0;
}

static int fxgmac_disable_rx_vlan_stripping(struct fxgmac_pdata *pdata)
{
    u32 regval;

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_VLANTR);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANTR_EVLS_POS,
                     MAC_VLANTR_EVLS_LEN, 0);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_VLANTR);
    DPRINTK("fxgmac disable MAC rx vlan stripping.\n");

    return 0;
}

static int fxgmac_enable_rx_vlan_filtering(struct fxgmac_pdata *pdata)
{
    u32 regval;

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_PFR);
    /* Enable VLAN filtering */
    regval = FXGMAC_SET_REG_BITS(regval, MAC_PFR_VTFE_POS,
                     MAC_PFR_VTFE_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_PFR);

//2022-04-25 xiaojiang comment
//FXGMAC_FILTER_SINGLE_VLAN_ENABLED is used for Linux driver
//In Linux driver it uses VLAN Filter to filter packet, In windows driver it uses SW method to filter packet.
#if FXGMAC_FILTER_SINGLE_VLAN_ENABLED
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_VLANTR);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANTR_VL_POS,
                     MAC_VLANTR_VL_LEN, pdata->vlan);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_VLANTR);
#else
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_VLANTR);
    /* Enable VLAN Hash Table filtering */
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANTR_VTHM_POS,
                     MAC_VLANTR_VTHM_LEN, 1);
    /* Disable VLAN tag inverse matching */
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANTR_VTIM_POS,
                     MAC_VLANTR_VTIM_LEN, 0);
    /* Only filter on the lower 12-bits of the VLAN tag */
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANTR_ETV_POS,
                     MAC_VLANTR_ETV_LEN, 1);
#endif

    return 0;
}

static int fxgmac_disable_rx_vlan_filtering(struct fxgmac_pdata *pdata)
{
    u32 regval;

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_PFR);
    /* Disable VLAN filtering */
    regval = FXGMAC_SET_REG_BITS(regval, MAC_PFR_VTFE_POS,
                     MAC_PFR_VTFE_LEN, 0);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_PFR);

#if FXGMAC_FILTER_SINGLE_VLAN_ENABLED
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_VLANTR);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANTR_VL_POS,
                     MAC_VLANTR_VL_LEN, pdata->vlan);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_VLANTR);
#endif

    return 0;
}

#if FXGMAC_FILTER_MULTIPLE_VLAN_ENABLED
static u32 fxgmac_vid_crc32_le(__le16 vid_le)
{
    unsigned char *data = (unsigned char *)&vid_le;
    unsigned char data_byte = 0;
    u32 crc = ~0;
    u32 temp = 0;
    int i, bits;

    bits = get_bitmask_order(VLAN_VID_MASK);
    for (i = 0; i < bits; i++) {
        if ((i % 8) == 0)
            data_byte = data[i / 8];

        temp = ((crc & 1) ^ data_byte) & 1;
        crc >>= 1;
        data_byte >>= 1;

        if (temp)
            crc ^= CRC32_POLY_LE;
    }

    return crc;
}
#endif

static int fxgmac_update_vlan_hash_table(struct fxgmac_pdata *pdata)
{
    u16 vlan_hash_table = 0;
    u32 regval;
    //Linux also support multiple VLAN
#if FXGMAC_FILTER_MULTIPLE_VLAN_ENABLED
    __le16 vid_le;
    u32 crc;
    u16 vid;
    /* Generate the VLAN Hash Table value */
    for_each_set_bit(vid, pdata->active_vlans, VLAN_N_VID) {
        /* Get the CRC32 value of the VLAN ID */
        vid_le = cpu_to_le16(vid);
        crc = bitrev32(~fxgmac_vid_crc32_le(vid_le)) >> 28;

        vlan_hash_table |= (1 << crc);
    }
#endif
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_VLANHTR);
    /* Set the VLAN Hash Table filtering register */
    regval = FXGMAC_SET_REG_BITS(regval, MAC_VLANHTR_VLHT_POS,
                     MAC_VLANHTR_VLHT_LEN, vlan_hash_table);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_VLANHTR);

    DPRINTK("fxgmac_update_vlan_hash_tabl done,hash tbl=%08x.\n",vlan_hash_table);
    return 0;
}

static int fxgmac_set_promiscuous_mode(struct fxgmac_pdata *pdata,
                       unsigned int enable)
{
    unsigned int val = enable ? 1 : 0;
    u32 regval;

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_PFR);

    if (FXGMAC_GET_REG_BITS(regval, MAC_PFR_PR_POS, MAC_PFR_PR_LEN) == val) {
        return 0;
    }
        netif_dbg(pdata, drv, pdata->netdev, ""STR_FORMAT" promiscuous mode\n",
              enable ? "entering" : "leaving");

    regval = FXGMAC_SET_REG_BITS(regval, MAC_PFR_PR_POS, MAC_PFR_PR_LEN, val);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_PFR);

        DbgPrintF(MP_TRACE, ""STR_FORMAT" - promiscuous mode=%d, reg=%x.", __FUNCTION__, enable, regval);
        DbgPrintF(MP_TRACE, ""STR_FORMAT" - note, vlan filter is called when set promiscuous mode=%d.", __FUNCTION__, enable);

    /* Hardware will still perform VLAN filtering in promiscuous mode */
    if (enable) {
        fxgmac_disable_rx_vlan_filtering(pdata);
    } else {
        #ifdef LINUX
        if(pdata->netdev->features & NETIF_F_HW_VLAN_CTAG_FILTER)
        #else
        if (0)
        #endif
        {
            fxgmac_enable_rx_vlan_filtering(pdata);
        }
    }

    DPRINTK("fxgmac set promisc mode=%d\n", enable);
    return 0;
}

static int fxgmac_enable_rx_broadcast(struct fxgmac_pdata *pdata,
                       unsigned int enable)
{
    unsigned int val = enable ? 0 : 1; //mac reg bit is disable,,so invert the val.
    u32 regval;

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_PFR);

    if (FXGMAC_GET_REG_BITS(regval, MAC_PFR_DBF_POS, MAC_PFR_DBF_LEN) == val) {
        return 0;
    }

    regval = FXGMAC_SET_REG_BITS(regval, MAC_PFR_DBF_POS, MAC_PFR_DBF_LEN, val);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_PFR);

    DbgPrintF(MP_TRACE, "%s - bcast en=%d, bit-val=%d, reg=%x.", __FUNCTION__, enable, val, regval);
    return 0;
}

static int fxgmac_set_all_multicast_mode(struct fxgmac_pdata *pdata,
                     unsigned int enable)
{
    unsigned int val = enable ? 1 : 0;
    u32 regval;

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_PFR);
    if (FXGMAC_GET_REG_BITS(regval, MAC_PFR_PM_POS, MAC_PFR_PM_LEN) == val) {
        return 0;
    }
    netif_dbg(pdata, drv, pdata->netdev, ""STR_FORMAT" allmulti mode\n",
          enable ? "entering" : "leaving");

    regval = FXGMAC_SET_REG_BITS(regval, MAC_PFR_PM_POS, MAC_PFR_PM_LEN, val);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_PFR);

    DbgPrintF(MP_TRACE, ""STR_FORMAT" - Enable all Multicast=%d, regval=%#x.", __FUNCTION__, enable, regval);

    return 0;
}

static void fxgmac_set_mac_addn_addrs(struct fxgmac_pdata *pdata)
{
#ifndef DPDK
#if FXGMAC_FILTER_MULTIPLE_MAC_ADDR_ENABLED
    struct net_device *netdev = pdata->netdev;
    struct netdev_hw_addr *ha;
#endif
    unsigned int addn_macs;
    unsigned int mac_reg;

    mac_reg = MAC_MACA1HR;
    addn_macs = pdata->hw_feat.addn_mac;
#if FXGMAC_FILTER_MULTIPLE_MAC_ADDR_ENABLED
    DPRINTK("xlgamc add mac addr callin\n");
    if (netdev_uc_count(netdev) > addn_macs) {
        fxgmac_set_promiscuous_mode(pdata, 1);
    } else {
        netdev_for_each_uc_addr(ha, netdev) {
            fxgmac_set_mac_reg(pdata, ha, &mac_reg);
            addn_macs--;
        }

        if (netdev_mc_count(netdev) > addn_macs) {
            fxgmac_set_all_multicast_mode(pdata, 1);
        } else {
            netdev_for_each_mc_addr(ha, netdev) {
                fxgmac_set_mac_reg(pdata, ha, &mac_reg);
                addn_macs--;
            }
        }
    }
#endif
    /* Clear remaining additional MAC address entries */
    while (addn_macs--) {
        fxgmac_set_mac_reg(pdata, NULL, &mac_reg);
    }
#else
    (void)pdata;
#endif
}

#define GET_REG_AND_BIT_POS(reversalval, regOut, bitOut) \
    do { \
        regOut = (((reversalval) >> 5) & 0x7); \
        bitOut = ((reversalval) & 0x1f); \
    }while(0)

static u32 fxgmac_crc32(unsigned char *Data, int Length)
{
    u32 Crc = (u32)~0;  /* Initial value. 0xFFFFFFFF */

    while (--Length >= 0) {
        unsigned char	Byte = *Data++;
        int        Bit;

        for (Bit = 8; --Bit >= 0; Byte >>= 1) {
            if ((Crc ^ Byte) & 1) {
                Crc >>= 1;
                Crc ^= 0xedb88320;
            }
            else {
                Crc >>= 1;
            }
        }
    }

    return ~Crc;
}

/*
 * configure multicast hash table, reg 0x2010~202c
 * input:     pmc_mac, pointer to mcast MAC. if it is null, then clean all registers.
 *         b_add, 1 to set the bit; 0 to clear the bit.
 */
static void fxgmac_config_multicast_mac_hash_table(struct fxgmac_pdata *pdata, unsigned char *pmc_mac, int b_add)
{
    unsigned int hash_reg, reg_bit;
    unsigned int j;
    u32 crc, reversal_crc, regval;

    if(!pmc_mac)
    {
        for(j = 0; j < FXGMAC_MAC_HASH_TABLE_SIZE; j++)
        {
            hash_reg = j;
            hash_reg = (MAC_HTR0 + hash_reg * MAC_HTR_INC);
            writereg(pdata->pAdapter, 0, pdata->mac_regs + hash_reg);
            //DBGPRINT(MP_TRACE, ("fxgmac_config_mcast_mac_hash_table, reg %04x=0x%x\n", FUXI_MAC_REGS_OFFSET + hash_reg, readreg(pdata->mac_regs + hash_reg)));
        }
        DBGPRINT(MP_TRACE, ("<fxgmac_config_mcast_mac_hash_table, clear all mcast mac hash table\n"));
        return;
    }

    crc = fxgmac_crc32 (pmc_mac, ETH_ALEN);

    /* reverse the crc */
    for(j = 0, reversal_crc = 0; j < 32; j++)
    {
        if(crc & (1 << j)) reversal_crc |= 1 << (31 - j);
    }

    GET_REG_AND_BIT_POS((reversal_crc>>24), hash_reg, reg_bit);
    /* Set the MAC Hash Table registers */
    hash_reg = (MAC_HTR0 + hash_reg * MAC_HTR_INC);
    regval = readreg(pdata->pAdapter, pdata->mac_regs + hash_reg);

    regval = FXGMAC_SET_REG_BITS(regval, reg_bit, 1, (b_add ? 1 : 0));

    writereg(pdata->pAdapter, regval, pdata->mac_regs + hash_reg);

#if 0
    DBGPRINT(MP_TRACE, ("<fxgmac_config_mcast_mac_hash_table, mcaddr=%02x%02x%02x%02x%02x%02x, crc=0x%08x, rcrc=0x%08x, reg=0x%x, bit=%d\n", pmc_mac[0], pmc_mac[1], pmc_mac[2], pmc_mac[3], pmc_mac[4], pmc_mac[5],
        crc, reversal_crc, hash_reg, reg_bit));
#endif

}

static void fxgmac_set_mac_hash_table(struct fxgmac_pdata *pdata)
{
#ifndef DPDK
    //unsigned int hash_table_shift, hash_table_count;
#if FUXI_MAC_HASH_TABLE
    struct net_device *netdev = pdata->netdev;
    struct netdev_hw_addr *ha;
#if 0
    u32 hash_table[FXGMAC_MAC_HASH_TABLE_SIZE];
    unsigned int hash_reg;
    unsigned int i;
    u32 crc;
#endif
    netdev_for_each_mc_addr(ha, netdev)
    {
        fxgmac_config_multicast_mac_hash_table(pdata, ha->addr, 1);
    }
#endif

#if 0 //TODO
    hash_table_shift = 26 - (pdata->hw_feat.hash_table_size >> 7);
    hash_table_count = pdata->hw_feat.hash_table_size / 32;

    memset(hash_table, 0, sizeof(hash_table));

    /* Build the MAC Hash Table register values */
    netdev_for_each_uc_addr(ha, netdev) {
        crc = bitrev32(~crc32_le(~0, ha->addr, ETH_ALEN));
        crc >>= hash_table_shift;
        hash_table[crc >> 5] |= (1 << (crc & 0x1f));
    }

    netdev_for_each_mc_addr(ha, netdev) {
        crc = bitrev32(~crc32_le(~0, ha->addr, ETH_ALEN));
        crc >>= hash_table_shift;
        hash_table[crc >> 5] |= (1 << (crc & 0x1f));
    }

    /* Set the MAC Hash Table registers */
    hash_reg = MAC_HTR0;
    for (i = 0; i < hash_table_count; i++) {
        writereg(pdata->pAdapter, hash_table[i], pdata->mac_regs + hash_reg);
        hash_reg += MAC_HTR_INC;
    }

    //if(hash_table[i])
    //	DPRINTK("fxgmac_set_mac_hash_tabl[%d]=%08x.\n", i, hash_table[i]);
#else
    pdata = pdata;
#endif

#else
    (void)pdata;
#endif
}

static int fxgmac_add_mac_addresses(struct fxgmac_pdata *pdata)
{
    if (pdata->hw_feat.hash_table_size)
        fxgmac_set_mac_hash_table(pdata);
    else
        fxgmac_set_mac_addn_addrs(pdata);

    return 0;
}

static void fxgmac_config_mac_address(struct fxgmac_pdata *pdata)
{
    u32 regval;
    //KdBreakPoint();
    fxgmac_set_mac_address(pdata, pdata->mac_addr);
    //fxgmac_set_mac_address(pdata, (u8*)"\x00\x55\x7b\xb5\x7d\xf7");// pdata->netdev->dev_addr);
    //fxgmac_set_mac_address(pdata, (u8*)"\xf7\x7d\xb5\x7b\x55\x00");

    /* Filtering is done using perfect filtering and hash filtering */
    if (pdata->hw_feat.hash_table_size) {
        regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_PFR);
        regval = FXGMAC_SET_REG_BITS(regval, MAC_PFR_HPF_POS,
                         MAC_PFR_HPF_LEN, 1);
#if FUXI_MAC_HASH_TABLE
        regval = FXGMAC_SET_REG_BITS(regval, MAC_PFR_HUC_POS,
                         MAC_PFR_HUC_LEN, 1);
#endif
        regval = FXGMAC_SET_REG_BITS(regval, MAC_PFR_HMC_POS,
                         MAC_PFR_HMC_LEN, 1);
        writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_PFR);
    }
}

static int fxgmac_config_crc_check(struct fxgmac_pdata *pdata)
{
    u32 regval, value;

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_ECR);
    value = (pdata->crc_check) ? 0 : 1;
    regval = FXGMAC_SET_REG_BITS(regval, MAC_ECR_DCRCC_POS,
                     MAC_ECR_DCRCC_LEN, value);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_ECR);

    return 0;
}

static int fxgmac_config_jumbo(struct fxgmac_pdata *pdata)
{
    u32 regval;

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_CR);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_JE_POS,
                     MAC_CR_JE_LEN, pdata->jumbo);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_CR);
    return 0;
}

static void fxgmac_config_checksum_offload(struct fxgmac_pdata *pdata)
{
#ifdef LINUX
    if (pdata->netdev->features & NETIF_F_RXCSUM)
#else
    if (1)
#endif
        fxgmac_enable_rx_csum(pdata);
    else
        fxgmac_disable_rx_csum(pdata);
}

static void fxgmac_config_vlan_support(struct fxgmac_pdata *pdata)
{
    /*if (pdata->vlan_exist)
        fxgmac_enable_tx_vlan(pdata);
    else*/
        fxgmac_disable_tx_vlan(pdata); // configure dynamical vlanID from TX Context.

    /* Set the current VLAN Hash Table register value */
    fxgmac_update_vlan_hash_table(pdata);

    //2022-04-26 xiaojiang comment
    //Windows driver set vlan_filter disable, but in Linux driver it enable vlan_filter
    if (pdata->vlan_filter) //disable vlan rx filter by default
        fxgmac_enable_rx_vlan_filtering(pdata);
    else
        fxgmac_disable_rx_vlan_filtering(pdata);

    if (pdata->vlan_strip) //enable vlan rx strip by default
        fxgmac_enable_rx_vlan_stripping(pdata);
    else
        fxgmac_disable_rx_vlan_stripping(pdata);

}

static int fxgmac_config_rx_mode(struct fxgmac_pdata *pdata)
{
#ifdef LINUX
    struct net_device *netdev = pdata->netdev;
#endif
    unsigned int pr_mode, am_mode;

#ifdef LINUX
    pr_mode = ((netdev->flags & IFF_PROMISC) != 0);
    am_mode = ((netdev->flags & IFF_ALLMULTI) != 0);
#else
    pr_mode = 1;// ((netdev->flags & IFF_PROMISC) != 0); //TODO
    am_mode = 1;// ((netdev->flags & IFF_ALLMULTI) != 0);//TODO
#endif

    fxgmac_set_promiscuous_mode(pdata, pr_mode);
    fxgmac_set_all_multicast_mode(pdata, am_mode);

    fxgmac_add_mac_addresses(pdata);

    return 0;
}

static void fxgmac_prepare_tx_stop(struct fxgmac_pdata *pdata,
                   struct fxgmac_channel *channel)
{
#ifdef LINUX
    unsigned int tx_dsr, tx_pos, tx_qidx;
    unsigned long tx_timeout;
    unsigned int tx_status;

    pdata = pdata;

    /* Calculate the status register to read and the position within */
    if (channel->queue_index < DMA_DSRX_FIRST_QUEUE) {
        tx_dsr = DMA_DSR0;
        tx_pos = (channel->queue_index * DMA_DSR_Q_LEN) +
             DMA_DSR0_TPS_START;
    } else {
        tx_qidx = channel->queue_index - DMA_DSRX_FIRST_QUEUE;

        tx_dsr = DMA_DSR1 + ((tx_qidx / DMA_DSRX_QPR) * DMA_DSRX_INC);
        tx_pos = ((tx_qidx % DMA_DSRX_QPR) * DMA_DSR_Q_LEN) +
             DMA_DSRX_TPS_START;
    }
    //2022-04-19 xiaojiang comment
    //Windows os not have wait Tx Stop operation.
    /* The Tx engine cannot be stopped if it is actively processing
     * descriptors. Wait for the Tx engine to enter the stopped or
     * suspended state.  Don't wait forever though...
     */
#if FXGMAC_TX_HANG_TIMER_EN
    tx_timeout = jiffies + msecs_to_jiffies(100); /* 100ms */
#else
    tx_timeout = jiffies + (FXGMAC_DMA_STOP_TIMEOUT * HZ);
#endif
    while (time_before(jiffies, tx_timeout)) {
        tx_status = readreg(pdata->pAdapter, pdata->mac_regs + tx_dsr);
        tx_status = FXGMAC_GET_REG_BITS(tx_status, tx_pos,
                        DMA_DSR_TPS_LEN);
        if ((tx_status == DMA_TPS_STOPPED) ||
            (tx_status == DMA_TPS_SUSPENDED))
            break;

        usleep_range_ex(pdata->pAdapter, 500, 1000);
    }

    if (!time_before(jiffies, tx_timeout))
        netdev_info(pdata->netdev,
                "timed out waiting for Tx DMA channel %u to stop\n",
                channel->queue_index);
#else
    pdata = pdata;
    channel = channel;
#endif
}

static void fxgmac_enable_tx(struct fxgmac_pdata *pdata)
{
#ifndef DPDK
    struct fxgmac_channel *channel;
#endif
    unsigned int i;
    u32 regval;

#if FXGMAC_TX_HANG_TIMER_EN
    pdata->tx_hang_restart_queuing = 0;
#endif

    /* Enable each Tx DMA channel */
#ifndef DPDK
    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
#ifndef UBOOT //uboot unuse tx_ring
        if (!channel->tx_ring)
            break;
#endif
        regval = readreg(pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_TCR));
        regval = FXGMAC_SET_REG_BITS(regval, DMA_CH_TCR_ST_POS,
                         DMA_CH_TCR_ST_LEN, 1);
        writereg(pdata->pAdapter, regval, FXGMAC_DMA_REG(channel, DMA_CH_TCR));
    }
#else
	PMD_INIT_FUNC_TRACE();
	struct fxgmac_tx_queue *txq;
	struct rte_eth_dev *dev = pdata->expansion.eth_dev;

	for (i = 0; i < dev->data->nb_tx_queues; i++) {
		txq = dev->data->tx_queues[i];
		/* Enable Tx DMA channel */
		FXGMAC_DMA_IOWRITE_BITS(txq, DMA_CH_TCR, ST, 1);
	}
#endif

    /* Enable each Tx queue */
    for (i = 0; i < pdata->tx_q_count; i++) {
        regval = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
        regval = FXGMAC_SET_REG_BITS(regval, MTL_Q_TQOMR_TXQEN_POS,
                         MTL_Q_TQOMR_TXQEN_LEN,
                    MTL_Q_ENABLED);
        writereg(pdata->pAdapter, regval, FXGMAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
    }

    /* Enable MAC Tx */
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_CR);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_TE_POS,
                     MAC_CR_TE_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_CR);
}

static void fxgmac_disable_tx(struct fxgmac_pdata *pdata)
{
#ifndef DPDK
    struct fxgmac_channel *channel;
#endif
    unsigned int i;
    u32 regval;

    /* Prepare for Tx DMA channel stop */
#ifndef DPDK
    channel = pdata->channel_head;
    if (channel != NULL) {
        for (i = 0; i < pdata->channel_count; i++, channel++) {
            if (!channel->tx_ring)
                break;

            fxgmac_prepare_tx_stop(pdata, channel);

#if FXGMAC_TX_HANG_TIMER_EN
            pdata->tx_hang_restart_queuing = 0;
#endif
        }
    }

#else
	PMD_INIT_FUNC_TRACE();
	struct fxgmac_tx_queue *txq;
	struct rte_eth_dev *dev = pdata->expansion.eth_dev;

	for (i = 0; i < pdata->tx_q_count; i++) {
		txq = dev->data->tx_queues[i];
		fxgmac_txq_prepare_tx_stop(pdata, i);
	}
#endif

    /* Disable MAC Tx */
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_CR);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_TE_POS,
                     MAC_CR_TE_LEN, 0);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_CR);

    /* Disable each Tx queue */
    for (i = 0; i < pdata->tx_q_count; i++) {
        regval = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
        regval = FXGMAC_SET_REG_BITS(regval, MTL_Q_TQOMR_TXQEN_POS,
                         MTL_Q_TQOMR_TXQEN_LEN, 0);
        writereg(pdata->pAdapter, regval, FXGMAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
    }

    /* Disable each Tx DMA channel */
#ifndef DPDK
    channel = pdata->channel_head;
    if (channel != NULL) {
        for (i = 0; i < pdata->channel_count; i++, channel++) {
            if (!channel->tx_ring)
                break;

            regval = readreg(pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_TCR));
            regval = FXGMAC_SET_REG_BITS(regval, DMA_CH_TCR_ST_POS,
                             DMA_CH_TCR_ST_LEN, 0);
            writereg(pdata->pAdapter, regval, FXGMAC_DMA_REG(channel, DMA_CH_TCR));
        }
    }
#else
	for (i = 0; i < dev->data->nb_tx_queues; i++) {
		txq = dev->data->tx_queues[i];
		FXGMAC_DMA_IOWRITE_BITS(txq, DMA_CH_TCR, ST, 0);
	}
#endif
}

static void fxgmac_prepare_rx_stop(struct fxgmac_pdata *pdata,
                   unsigned int queue)
{
    unsigned int rx_status, prxq;
#if defined(LINUX) || defined(DPDK)
    unsigned int rxqsts;
    unsigned long rx_timeout;
#if defined(DPDK)
    unsigned long jiffies = rte_get_timer_cycles();
#endif
    /* The Rx engine cannot be stopped if it is actively processing
     * packets. Wait for the Rx queue to empty the Rx fifo.  Don't
     * wait forever though...
     */
#if FXGMAC_TX_HANG_TIMER_EN
    rx_timeout = jiffies + msecs_to_jiffies(500); /* 500ms, larger is better */
#else
    rx_timeout = jiffies + (FXGMAC_DMA_STOP_TIMEOUT * HZ);
#endif
    while (time_before(jiffies, rx_timeout)) {
        rx_status = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, queue, MTL_Q_RQDR));
        prxq = FXGMAC_GET_REG_BITS(rx_status, MTL_Q_RQDR_PRXQ_POS,
                       MTL_Q_RQDR_PRXQ_LEN);
        rxqsts = FXGMAC_GET_REG_BITS(rx_status, MTL_Q_RQDR_RXQSTS_POS,
                         MTL_Q_RQDR_RXQSTS_LEN);
        if ((prxq == 0) && (rxqsts == 0))
            break;

        usleep_range_ex(pdata->pAdapter, 500, 1000);
    }

    if (!time_before(jiffies, rx_timeout))
        netdev_info(pdata->netdev,
                "timed out waiting for Rx queue %u to empty\n",
                queue);
#else
    unsigned int busy = 100;
    do {
        rx_status = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, queue, MTL_Q_RQDR));
        prxq = FXGMAC_GET_REG_BITS(rx_status, MTL_Q_RQDR_PRXQ_POS, MTL_Q_RQDR_PRXQ_LEN);
        busy--;
        usleep_range_ex(pdata->pAdapter, 500, 1000);
    } while ((prxq) && (busy));
    if (0 == busy) {
        rx_status = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, queue, MTL_Q_RQDR));
        DbgPrintF(MP_WARN, "warning !!!timed out waiting for Rx queue %u to empty\n", queue);
    }
#endif
}

static void fxgmac_enable_rx(struct fxgmac_pdata *pdata)
{
#ifndef DPDK
    struct fxgmac_channel *channel;
#endif
    unsigned int regval, i;

    /* Enable each Rx DMA channel */
#ifndef DPDK
    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
#ifndef UBOOT //uboot unuse rx_ring
        if (!channel->rx_ring)
            break;
#endif
        regval = readreg(pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_RCR));
        regval = FXGMAC_SET_REG_BITS(regval, DMA_CH_RCR_SR_POS,
                         DMA_CH_RCR_SR_LEN, 1);
        writereg(pdata->pAdapter, regval, FXGMAC_DMA_REG(channel, DMA_CH_RCR));
    }

#else
	PMD_INIT_FUNC_TRACE();
	struct fxgmac_rx_queue *rxq;
	struct rte_eth_dev *dev = pdata->expansion.eth_dev;
	//struct fxgmac_pdata *pdata =  pdata->dev->data->dev_private;

	for (i = 0; i < dev->data->nb_rx_queues; i++) {
		rxq = dev->data->rx_queues[i];
		/* Enable Rx DMA channel */
		FXGMAC_DMA_IOWRITE_BITS(rxq, DMA_CH_RCR, SR, 1);
	}
#endif

    /* Enable each Rx queue */
    regval = 0;
    for (i = 0; i < pdata->rx_q_count; i++)
        regval |= (0x02 << (i << 1));
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_RQC0R);

#ifndef DPDK
    /* Enable MAC Rx */
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_CR);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_CST_POS,
                     MAC_CR_CST_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_ACS_POS,
                     MAC_CR_ACS_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_RE_POS,
                     MAC_CR_RE_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_CR);
#else
	/* Enable MAC Rx */
	FXGMAC_IOWRITE_BITS(pdata, MAC_ECR, DCRCC, 1);

	/* Frame is forwarded after stripping CRC to application*/
	if (pdata->expansion.crc_strip_enable) {
		FXGMAC_IOWRITE_BITS(pdata, MAC_CR, CST, 1);
		FXGMAC_IOWRITE_BITS(pdata, MAC_CR, ACS, 1);
	}
	FXGMAC_IOWRITE_BITS(pdata, MAC_CR, RE, 1);
#endif
}
static void fxgmac_enable_channel_rx(struct fxgmac_pdata* pdata, unsigned int queue)
{
    struct fxgmac_channel* channel;
    unsigned int regval;

    /* Enable Rx DMA channel */
    channel = pdata->channel_head + queue;

    if (!channel->rx_ring)
        return;
    regval = readreg(pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_RCR));
    regval = FXGMAC_SET_REG_BITS(regval, DMA_CH_RCR_SR_POS,
        DMA_CH_RCR_SR_LEN, 1);
    writereg(pdata->pAdapter, regval, FXGMAC_DMA_REG(channel, DMA_CH_RCR));
    /* Enable  Rx queue */
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_RQC0R);
    regval |= (0x02 << (queue << 1));
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_RQC0R);

    /* Enable MAC Rx */
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_CR);
    if (!(regval&((0x01<< MAC_CR_CST_POS)|(0x01 << MAC_CR_ACS_POS)|(0x01<< MAC_CR_RE_POS))))
    {
        regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_CST_POS,
            MAC_CR_CST_LEN, 1);
        regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_ACS_POS,
            MAC_CR_ACS_LEN, 1);
        regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_RE_POS,
            MAC_CR_RE_LEN, 1);
        writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_CR);
    }
}

static void fxgmac_disable_rx(struct fxgmac_pdata *pdata)
{
#ifndef DPDK
    struct fxgmac_channel *channel;
#endif
    unsigned int i;
    u32 regval;

    /* Disable MAC Rx */
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_CR);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_CST_POS,
                     MAC_CR_CST_LEN, 0);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_ACS_POS,
                     MAC_CR_ACS_LEN, 0);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_RE_POS,
                     MAC_CR_RE_LEN, 0);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_CR);

    /* Prepare for Rx DMA channel stop */
#ifndef DPDK
    for (i = 0; i < pdata->rx_q_count; i++)
        fxgmac_prepare_rx_stop(pdata, i);
#else
	PMD_INIT_FUNC_TRACE();
	struct fxgmac_rx_queue *rxq;
	struct rte_eth_dev *dev = pdata->expansion.eth_dev;

	for (i = 0; i < dev->data->nb_rx_queues; i++) {
		rxq = dev->data->rx_queues[i];
		fxgmac_prepare_rx_stop(pdata, i);
	}
#endif

    /* Disable each Rx queue */
    writereg(pdata->pAdapter, 0, pdata->mac_regs + MAC_RQC0R);

    /* Disable each Rx DMA channel */
#ifndef DPDK
    channel = pdata->channel_head;
    if (channel != NULL) {
        for (i = 0; i < pdata->channel_count; i++, channel++) {
            if (!channel->rx_ring)
                break;

            regval = readreg(pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_RCR));
            regval = FXGMAC_SET_REG_BITS(regval, DMA_CH_RCR_SR_POS,
                             DMA_CH_RCR_SR_LEN, 0);
            writereg(pdata->pAdapter, regval, FXGMAC_DMA_REG(channel, DMA_CH_RCR));
        }
    }
#else
	for (i = 0; i < dev->data->nb_rx_queues; i++) {
		rxq = dev->data->rx_queues[i];
		FXGMAC_DMA_IOWRITE_BITS(rxq, DMA_CH_RCR, SR, 0);
	}
#endif
}

#ifdef LINUX
static void fxgmac_tx_start_xmit(struct fxgmac_channel *channel,
                 struct fxgmac_ring *ring)
{
    struct fxgmac_pdata *pdata = channel->pdata;
    struct fxgmac_desc_data *desc_data;

    /* Make sure everything is written before the register write */
    wmb();

    /* Issue a poll command to Tx DMA by writing address
     * of next immediate free descriptor
     */
    desc_data = FXGMAC_GET_DESC_DATA(ring, ring->cur);
    //DPRINTK("tx_start_xmit: dump before wr reg,descs-%08x,%08x,%08x,%08x\n",desc_data->dma_desc->desc0,desc_data->dma_desc->desc1,desc_data->dma_desc->desc2,desc_data->dma_desc->desc3);
#if !(FXGMAC_DUMMY_TX_DEBUG)
    writereg(pdata->pAdapter, lower_32_bits(desc_data->dma_desc_addr),
           FXGMAC_DMA_REG(channel, DMA_CH_TDTR_LO));
#else
    DPRINTK("dummy tx, fxgmac_tx_start_xmit, tail reg=0x%lx,val=%08x\n", FXGMAC_DMA_REG(channel, DMA_CH_TDTR_LO) - pdata->mac_regs, (u32)lower_32_bits(desc_data->dma_desc_addr));
#endif
    if(netif_msg_tx_done(pdata)) DPRINTK("tx_start_xmit: dump before wr reg,dma base=0x%016llx,reg=0x%08x, tx timer usecs=%u,tx_timer_active=%u\n",
        desc_data->dma_desc_addr, readreg(pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_TDTR_LO)), pdata->tx_usecs, channel->tx_timer_active);

    ring->tx.xmit_more = 0;
}
#endif

#ifdef LINUX
static void fxgmac_dev_xmit(struct fxgmac_channel *channel)
{
    struct fxgmac_pdata *pdata = channel->pdata;
    struct fxgmac_ring *ring = channel->tx_ring;
    unsigned int tso_context, vlan_context;
    struct fxgmac_desc_data *desc_data;
    struct fxgmac_dma_desc *dma_desc;
    struct fxgmac_pkt_info *pkt_info;
    unsigned int csum, tso, vlan;
    int start_index = ring->cur;
    int cur_index = ring->cur;
    int i;

    if(netif_msg_tx_done(pdata)) DPRINTK("dev_xmit callin, desc cur=%d\n", cur_index);

    pkt_info = &ring->pkt_info;
    csum = FXGMAC_GET_REG_BITS(pkt_info->attributes,
                   TX_PACKET_ATTRIBUTES_CSUM_ENABLE_POS,
                TX_PACKET_ATTRIBUTES_CSUM_ENABLE_LEN);
    tso = FXGMAC_GET_REG_BITS(pkt_info->attributes,
                  TX_PACKET_ATTRIBUTES_TSO_ENABLE_POS,
                TX_PACKET_ATTRIBUTES_TSO_ENABLE_LEN);
    vlan = FXGMAC_GET_REG_BITS(pkt_info->attributes,
                   TX_PACKET_ATTRIBUTES_VLAN_CTAG_POS,
                TX_PACKET_ATTRIBUTES_VLAN_CTAG_LEN);

    if (tso && (pkt_info->mss != ring->tx.cur_mss))
        tso_context = 1;
    else
        tso_context = 0;

    //if(tso && (netif_msg_tx_done(pdata)))
    if((tso_context) && (netif_msg_tx_done(pdata))) {
        //tso is initialized to start...
        DPRINTK("fxgmac_dev_xmit,tso_%s tso=0x%x,pkt_mss=%d,cur_mss=%d\n",(pkt_info->mss)?"start":"stop", tso, pkt_info->mss, ring->tx.cur_mss);
    }

    if (vlan && (pkt_info->vlan_ctag != ring->tx.cur_vlan_ctag))
        vlan_context = 1;
    else
        vlan_context = 0;

    if(vlan && (netif_msg_tx_done(pdata))) DPRINTK("fxgmac_dev_xmi:pkt vlan=%d, ring vlan=%d, vlan_context=%d\n", pkt_info->vlan_ctag, ring->tx.cur_vlan_ctag, vlan_context);

    desc_data = FXGMAC_GET_DESC_DATA(ring, cur_index);
    dma_desc = desc_data->dma_desc;

    /* Create a context descriptor if this is a TSO pkt_info */
    if (tso_context || vlan_context) {
        if (tso_context) {
#if 0
            netif_dbg(pdata, tx_queued, pdata->netdev,
                  "TSO context descriptor, mss=%u\n",
                  pkt_info->mss);
#else
            if (netif_msg_tx_done(pdata)) DPRINTK("xlgamc dev xmit,construct tso context descriptor, mss=%u\n",
                  pkt_info->mss);
#endif

            /* Set the MSS size */
            dma_desc->desc2 = FXGMAC_SET_REG_BITS_LE(
                        dma_desc->desc2,
                        TX_CONTEXT_DESC2_MSS_POS,
                        TX_CONTEXT_DESC2_MSS_LEN,
                        pkt_info->mss);

            /* Mark it as a CONTEXT descriptor */
            dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                        dma_desc->desc3,
                        TX_CONTEXT_DESC3_CTXT_POS,
                        TX_CONTEXT_DESC3_CTXT_LEN,
                        1);

            /* Indicate this descriptor contains the MSS */
            dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                        dma_desc->desc3,
                        TX_CONTEXT_DESC3_TCMSSV_POS,
                        TX_CONTEXT_DESC3_TCMSSV_LEN,
                        1);

            ring->tx.cur_mss = pkt_info->mss;
        }

        if (vlan_context) {
            netif_dbg(pdata, tx_queued, pdata->netdev,
                  "VLAN context descriptor, ctag=%u\n",
                  pkt_info->vlan_ctag);

            /* Mark it as a CONTEXT descriptor */
            dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                        dma_desc->desc3,
                        TX_CONTEXT_DESC3_CTXT_POS,
                        TX_CONTEXT_DESC3_CTXT_LEN,
                        1);

            /* Set the VLAN tag */
            dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                        dma_desc->desc3,
                        TX_CONTEXT_DESC3_VT_POS,
                        TX_CONTEXT_DESC3_VT_LEN,
                        pkt_info->vlan_ctag);

            /* Indicate this descriptor contains the VLAN tag */
            dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                        dma_desc->desc3,
                        TX_CONTEXT_DESC3_VLTV_POS,
                        TX_CONTEXT_DESC3_VLTV_LEN,
                        1);

            ring->tx.cur_vlan_ctag = pkt_info->vlan_ctag;
        }

        //cur_index++;
        cur_index = FXGMAC_GET_ENTRY(cur_index, ring->dma_desc_count);
        desc_data = FXGMAC_GET_DESC_DATA(ring, cur_index);
        dma_desc = desc_data->dma_desc;
    }

    /* Update buffer address (for TSO this is the header) */
    dma_desc->desc0 =  cpu_to_le32(lower_32_bits(desc_data->skb_dma));
    dma_desc->desc1 =  cpu_to_le32(upper_32_bits(desc_data->skb_dma));

    /* Update the buffer length */
    dma_desc->desc2 = FXGMAC_SET_REG_BITS_LE(
                dma_desc->desc2,
                TX_NORMAL_DESC2_HL_B1L_POS,
                TX_NORMAL_DESC2_HL_B1L_LEN,
                desc_data->skb_dma_len);

    /* VLAN tag insertion check */
    if (vlan) {
        dma_desc->desc2 = FXGMAC_SET_REG_BITS_LE(
                    dma_desc->desc2,
                    TX_NORMAL_DESC2_VTIR_POS,
                    TX_NORMAL_DESC2_VTIR_LEN,
                    TX_NORMAL_DESC2_VLAN_INSERT);
        pdata->stats.tx_vlan_packets++;
    }

    /* Timestamp enablement check */
    if (FXGMAC_GET_REG_BITS(pkt_info->attributes,
                TX_PACKET_ATTRIBUTES_PTP_POS,
                TX_PACKET_ATTRIBUTES_PTP_LEN))
        dma_desc->desc2 = FXGMAC_SET_REG_BITS_LE(
                    dma_desc->desc2,
                    TX_NORMAL_DESC2_TTSE_POS,
                    TX_NORMAL_DESC2_TTSE_LEN,
                    1);

    /* Mark it as First Descriptor */
    dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                dma_desc->desc3,
                TX_NORMAL_DESC3_FD_POS,
                TX_NORMAL_DESC3_FD_LEN,
                1);

    /* Mark it as a NORMAL descriptor */
    dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                dma_desc->desc3,
                TX_NORMAL_DESC3_CTXT_POS,
                TX_NORMAL_DESC3_CTXT_LEN,
                0);

    /* Set OWN bit if not the first descriptor */
    if (cur_index != start_index)
        dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                    dma_desc->desc3,
                    TX_NORMAL_DESC3_OWN_POS,
                    TX_NORMAL_DESC3_OWN_LEN,
                    1);

    if (tso) {
        /* Enable TSO */
        dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                    dma_desc->desc3,
                    TX_NORMAL_DESC3_TSE_POS,
                    TX_NORMAL_DESC3_TSE_LEN, 1);
        dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                    dma_desc->desc3,
                    TX_NORMAL_DESC3_TCPPL_POS,
                    TX_NORMAL_DESC3_TCPPL_LEN,
                    pkt_info->tcp_payload_len);
        dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                    dma_desc->desc3,
                    TX_NORMAL_DESC3_TCPHDRLEN_POS,
                    TX_NORMAL_DESC3_TCPHDRLEN_LEN,
                    pkt_info->tcp_header_len / 4);

        pdata->stats.tx_tso_packets++;
    } else {
        /* Enable CRC and Pad Insertion */
        dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                    dma_desc->desc3,
                    TX_NORMAL_DESC3_CPC_POS,
                    TX_NORMAL_DESC3_CPC_LEN, 0);

        /* Enable HW CSUM */
        if (csum)
            dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                        dma_desc->desc3,
                        TX_NORMAL_DESC3_CIC_POS,
                        TX_NORMAL_DESC3_CIC_LEN,
                        0x3);

        /* Set the total length to be transmitted */
        dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                    dma_desc->desc3,
                    TX_NORMAL_DESC3_FL_POS,
                    TX_NORMAL_DESC3_FL_LEN,
                    pkt_info->length);
    }
    if(netif_msg_tx_done(pdata)) DPRINTK("dev_xmit before more descs, desc cur=%d, start=%d, desc=%#x,%#x,%#x,%#x\n",
        cur_index, start_index, dma_desc->desc0, dma_desc->desc1, dma_desc->desc2, dma_desc->desc3);

    if (start_index <= cur_index)
        i = cur_index - start_index + 1;
    else
        i = ring->dma_desc_count - start_index + cur_index;

    for (; i < pkt_info->desc_count; i++) {
        cur_index = FXGMAC_GET_ENTRY(cur_index, ring->dma_desc_count);

        desc_data = FXGMAC_GET_DESC_DATA(ring, cur_index);
        dma_desc = desc_data->dma_desc;

        /* Update buffer address */
        dma_desc->desc0 =
            cpu_to_le32(lower_32_bits(desc_data->skb_dma));
        dma_desc->desc1 =
            cpu_to_le32(upper_32_bits(desc_data->skb_dma));

        /* Update the buffer length */
        dma_desc->desc2 = FXGMAC_SET_REG_BITS_LE(
                    dma_desc->desc2,
                    TX_NORMAL_DESC2_HL_B1L_POS,
                    TX_NORMAL_DESC2_HL_B1L_LEN,
                    desc_data->skb_dma_len);

        /* Set OWN bit */
        dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                    dma_desc->desc3,
                    TX_NORMAL_DESC3_OWN_POS,
                    TX_NORMAL_DESC3_OWN_LEN, 1);

        /* Mark it as NORMAL descriptor */
        dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                    dma_desc->desc3,
                    TX_NORMAL_DESC3_CTXT_POS,
                    TX_NORMAL_DESC3_CTXT_LEN, 0);

        /* Enable HW CSUM */
        if (csum)
            dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                        dma_desc->desc3,
                        TX_NORMAL_DESC3_CIC_POS,
                        TX_NORMAL_DESC3_CIC_LEN,
                        0x3);
    }

    /* Set LAST bit for the last descriptor */
    dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                dma_desc->desc3,
                TX_NORMAL_DESC3_LD_POS,
                TX_NORMAL_DESC3_LD_LEN, 1);

    dma_desc->desc2 = FXGMAC_SET_REG_BITS_LE(
                dma_desc->desc2,
                TX_NORMAL_DESC2_IC_POS,
                TX_NORMAL_DESC2_IC_LEN, 1);

    /* Save the Tx info to report back during cleanup */
    desc_data->tx.packets = pkt_info->tx_packets;
    desc_data->tx.bytes = pkt_info->tx_bytes;

    if(netif_msg_tx_done(pdata)) DPRINTK("dev_xmit last descs, desc cur=%d, desc=%#x,%#x,%#x,%#x\n",
        cur_index, dma_desc->desc0, dma_desc->desc1, dma_desc->desc2, dma_desc->desc3);

    /* In case the Tx DMA engine is running, make sure everything
     * is written to the descriptor(s) before setting the OWN bit
     * for the first descriptor
     */
    dma_wmb();

    /* Set OWN bit for the first descriptor */
    desc_data = FXGMAC_GET_DESC_DATA(ring, start_index);
    dma_desc = desc_data->dma_desc;
    dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                dma_desc->desc3,
                TX_NORMAL_DESC3_OWN_POS,
                TX_NORMAL_DESC3_OWN_LEN, 1);

    if(netif_msg_tx_done(pdata)) DPRINTK("dev_xmit first descs, start=%d, desc=%#x,%#x,%#x,%#x\n",
        start_index, dma_desc->desc0, dma_desc->desc1, dma_desc->desc2, dma_desc->desc3);

    if (netif_msg_tx_queued(pdata))
        fxgmac_dump_tx_desc(pdata, ring, start_index,
                    pkt_info->desc_count, 1);
#if ( LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0) )
    if(netif_msg_tx_done(pdata)) DPRINTK("dev_xmit about to call tx_start_xmit, ring xmit_more=%d,txq_stopped=%x\n",ring->tx.xmit_more, netif_xmit_stopped(netdev_get_tx_queue(pdata->netdev, channel->queue_index)));
#else /* ( LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,165) )*/
    if(netif_msg_tx_done(pdata)) DPRINTK("dev_xmit about to call tx_start_xmit, pkt xmit_more=%d,txq_stopped=%x\n",pkt_info->skb->xmit_more, netif_xmit_stopped(netdev_get_tx_queue(pdata->netdev, channel->queue_index)));
#endif

    /* Make sure ownership is written to the descriptor */
    smp_wmb();

    //ring->cur = cur_index + 1;
    ring->cur = FXGMAC_GET_ENTRY(cur_index, ring->dma_desc_count);

#if 0
#if ( LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0) )
    if (!ring->tx.xmit_more || netif_xmit_stopped(netdev_get_tx_queue(pdata->netdev,
                           channel->queue_index)))
#elif ( LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,1) )
    if (!pkt_info->skb->xmit_more ||
        netif_xmit_stopped(netdev_get_tx_queue(pdata->netdev,
                           channel->queue_index)))
#else
    if (netif_xmit_stopped(netdev_get_tx_queue(pdata->netdev,
                           channel->queue_index)))
#endif
    {
        fxgmac_tx_start_xmit(channel, ring);
    }
    else
        ring->tx.xmit_more = 1;
#endif

    fxgmac_tx_start_xmit(channel, ring);

    /* yzhang for reduce debug output */
    if(netif_msg_tx_done(pdata)){
        DPRINTK("dev_xmit callout %s: descriptors %u to %u written\n",
          channel->name, start_index & (ring->dma_desc_count - 1),
          (ring->cur - 1) & (ring->dma_desc_count - 1));
    }
}
#endif
#ifdef LINUX
static void fxgmac_get_rx_tstamp(struct fxgmac_pkt_info *pkt_info,
                 struct fxgmac_dma_desc *dma_desc)
{
    //u32 tsa, tsd;
    u64 nsec;
#if 0
    tsa = FXGMAC_GET_REG_BITS_LE(dma_desc->desc3,
                     RX_CONTEXT_DESC3_TSA_POS,
                RX_CONTEXT_DESC3_TSA_LEN);
    tsd = FXGMAC_GET_REG_BITS_LE(dma_desc->desc3,
                     RX_CONTEXT_DESC3_TSD_POS,
                RX_CONTEXT_DESC3_TSD_LEN);
    if (tsa && !tsd) {
#endif
        nsec = le32_to_cpu(dma_desc->desc1);
        nsec <<= 32;
        nsec |= le32_to_cpu(dma_desc->desc0);
        if (nsec != 0xffffffffffffffffULL) {
            pkt_info->rx_tstamp = nsec;
            pkt_info->attributes = FXGMAC_SET_REG_BITS(
                    pkt_info->attributes,
                    RX_PACKET_ATTRIBUTES_RX_TSTAMP_POS,
                    RX_PACKET_ATTRIBUTES_RX_TSTAMP_LEN,
                    1);
        }
    //}
}
#endif

static void fxgmac_tx_desc_reset(struct fxgmac_desc_data *desc_data)
{
    struct fxgmac_dma_desc *dma_desc = desc_data->dma_desc;

    /* Reset the Tx descriptor
     *   Set buffer 1 (lo) address to zero
     *   Set buffer 1 (hi) address to zero
     *   Reset all other control bits (IC, TTSE, B2L & B1L)
     *   Reset all other control bits (OWN, CTXT, FD, LD, CPC, CIC, etc)
     */
    dma_desc->desc0 = 0;
    dma_desc->desc1 = 0;
    dma_desc->desc2 = 0;
    dma_desc->desc3 = 0;

    /* Make sure ownership is written to the descriptor */
    dma_wmb();
}

static void fxgmac_tx_desc_init(struct fxgmac_channel *channel)
{
    struct fxgmac_ring *ring = channel->tx_ring;
    struct fxgmac_desc_data *desc_data;
    int start_index = ring->cur;
    unsigned int i;
    start_index = start_index;
    //DEBUGPRINT(INIT, "["STR_FORMAT","STR_FORMAT",%d]:dma_desc_count=%d,0x%x\n",__FILE__, __func__,__LINE__,ring->dma_desc_count,channel->tx_ring);

    /* Initialize all descriptors */
    for (i = 0; i < ring->dma_desc_count; i++) {
        desc_data = FXGMAC_GET_DESC_DATA(ring, i);

        /* Initialize Tx descriptor */
        fxgmac_tx_desc_reset(desc_data);
    }

    ///* Update the total number of Tx descriptors */
    //writereg(ring->dma_desc_count - 1, FXGMAC_DMA_REG(channel, DMA_CH_TDRLR));
    writereg(channel->pdata->pAdapter, channel->pdata->tx_desc_count - 1, FXGMAC_DMA_REG(channel, DMA_CH_TDRLR));

/* Update the starting address of descriptor ring */
#if defined(LINUX)
    desc_data = FXGMAC_GET_DESC_DATA(ring, start_index);
    writereg(channel->pdata->pAdapter, upper_32_bits(desc_data->dma_desc_addr),
           FXGMAC_DMA_REG(channel, DMA_CH_TDLR_HI));
    writereg(channel->pdata->pAdapter, lower_32_bits(desc_data->dma_desc_addr),
           FXGMAC_DMA_REG(channel, DMA_CH_TDLR_LO));
#elif defined(UEFI)
    writereg(channel->pdata->pAdapter, GetPhyAddrHigh(((PADAPTER)channel->pdata->pAdapter)->TbdPhyAddr),//adpt->TbdPhyAddr
        FXGMAC_DMA_REG(channel, DMA_CH_TDLR_HI));
    writereg(channel->pdata->pAdapter, GetPhyAddrLow(((PADAPTER)channel->pdata->pAdapter)->TbdPhyAddr),
        FXGMAC_DMA_REG(channel, DMA_CH_TDLR_LO));
#elif defined(PXE)

#elif defined(UBOOT)

#elif defined(DPDK)

#else //netadaptercx & ndis
    writereg(channel->pdata->pAdapter, NdisGetPhysicalAddressHigh(((PMP_ADAPTER)channel->pdata->pAdapter)->HwTbdBasePa),
        FXGMAC_DMA_REG(channel, DMA_CH_TDLR_HI));
    writereg(channel->pdata->pAdapter, NdisGetPhysicalAddressLow(((PMP_ADAPTER)channel->pdata->pAdapter)->HwTbdBasePa),
        FXGMAC_DMA_REG(channel, DMA_CH_TDLR_LO));
#endif

}

static void fxgmac_rx_desc_reset(struct fxgmac_pdata *pdata,
                 struct fxgmac_desc_data *desc_data,
                 unsigned int index)
{
#ifdef LINUX
    struct fxgmac_dma_desc *dma_desc = desc_data->dma_desc;

    /* Reset the Rx descriptor
     *   Set buffer 1 (lo) address to header dma address (lo)
     *   Set buffer 1 (hi) address to header dma address (hi)
     *   Set buffer 2 (lo) address to buffer dma address (lo)
     *   Set buffer 2 (hi) address to buffer dma address (hi) and
     *     set control bits OWN and INTE
     */
    //hdr_dma = desc_data->rx.hdr.dma_base + desc_data->rx.hdr.dma_off;
    //buf_dma = desc_data->rx.buf.dma_base + desc_data->rx.buf.dma_off;
    dma_desc->desc0 = cpu_to_le32(lower_32_bits(desc_data->rx.buf.dma_base));
    dma_desc->desc1 = cpu_to_le32(upper_32_bits(desc_data->rx.buf.dma_base));
    dma_desc->desc2 = 0;//cpu_to_le32(lower_32_bits(buf_dma));
    dma_desc->desc3 = 0;//cpu_to_le32(upper_32_bits(buf_dma));
    dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                dma_desc->desc3,
                RX_NORMAL_DESC3_INTE_POS,
                RX_NORMAL_DESC3_INTE_LEN,
                1);
    dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                dma_desc->desc3,
                RX_NORMAL_DESC3_BUF2V_POS,
                RX_NORMAL_DESC3_BUF2V_LEN,
                0);
    dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                dma_desc->desc3,
                RX_NORMAL_DESC3_BUF1V_POS,
                RX_NORMAL_DESC3_BUF1V_LEN,
                1);

    /* Since the Rx DMA engine is likely running, make sure everything
     * is written to the descriptor(s) before setting the OWN bit
     * for the descriptor
     */
    dma_wmb();

    dma_desc->desc3 = FXGMAC_SET_REG_BITS_LE(
                    dma_desc->desc3,
                    RX_NORMAL_DESC3_OWN_POS,
                    RX_NORMAL_DESC3_OWN_LEN,
                    1);

    /* Make sure ownership is written to the descriptor */
    dma_wmb();
#else
    pdata = pdata;
    desc_data = desc_data;
    index = index;
#endif
}

static void fxgmac_rx_desc_init(struct fxgmac_channel *channel)
{
    struct fxgmac_pdata *pdata = channel->pdata;
    struct fxgmac_ring *ring = channel->rx_ring;
#ifdef LINUX
    unsigned int start_index = ring->cur;
#endif
    struct fxgmac_desc_data *desc_data;
    unsigned int i;
#if defined(UEFI)
    UINT64   HwRbdPa;
#elif defined(_WIN64) || defined(_WIN32)
    unsigned int Qid;
    NDIS_PHYSICAL_ADDRESS   HwRbdPa;
#elif defined(PXE)
#endif


    /* Initialize all descriptors */
    for (i = 0; i < ring->dma_desc_count; i++) {
        desc_data = FXGMAC_GET_DESC_DATA(ring, i);

        /* Initialize Rx descriptor */
        fxgmac_rx_desc_reset(pdata, desc_data, i);
    }

#if defined(LINUX)
    /* Update the total number of Rx descriptors */
    writereg(pdata->pAdapter, ring->dma_desc_count - 1, FXGMAC_DMA_REG(channel, DMA_CH_RDRLR));

    /* Update the starting address of descriptor ring */
    desc_data = FXGMAC_GET_DESC_DATA(ring, start_index);
    writereg(pdata->pAdapter, upper_32_bits(desc_data->dma_desc_addr),
        FXGMAC_DMA_REG(channel, DMA_CH_RDLR_HI));
    writereg(pdata->pAdapter, lower_32_bits(desc_data->dma_desc_addr),
        FXGMAC_DMA_REG(channel, DMA_CH_RDLR_LO));

    /* Update the Rx Descriptor Tail Pointer */
    desc_data = FXGMAC_GET_DESC_DATA(ring, start_index +
        ring->dma_desc_count - 1);
    writereg(pdata->pAdapter, lower_32_bits(desc_data->dma_desc_addr),
        FXGMAC_DMA_REG(channel, DMA_CH_RDTR_LO));
#elif defined(UEFI)
    writereg(pdata->pAdapter, pdata->rx_desc_count - 1,
        FXGMAC_DMA_REG(channel, DMA_CH_RDRLR));
    /* Update the starting address of descriptor ring */
    writereg(pdata->pAdapter, GetPhyAddrHigh(((PADAPTER)channel->pdata->pAdapter)->RbdPhyAddr),
        FXGMAC_DMA_REG(channel, DMA_CH_RDLR_HI));
    writereg(pdata->pAdapter, GetPhyAddrLow(((PADAPTER)channel->pdata->pAdapter)->RbdPhyAddr),
        FXGMAC_DMA_REG(channel, DMA_CH_RDLR_LO));


    HwRbdPa = ((PADAPTER)channel->pdata->pAdapter)->RbdPhyAddr + (pdata->rx_desc_count - 1) * sizeof(struct fxgmac_dma_desc);

    /* Update the Rx Descriptor Tail Pointer */
    writereg(pdata->pAdapter, GetPhyAddrLow(HwRbdPa), FXGMAC_DMA_REG(channel, DMA_CH_RDTR_LO));
#elif defined(PXE)
#elif defined(UBOOT)
#elif defined(DPDK)
#else //netadaptercx & ndis
    Qid = (unsigned int)(channel - pdata->channel_head);
    DbgPrintF(MP_TRACE, ""STR_FORMAT": %d, Qid =%d\n", __FUNCTION__, __LINE__, Qid);

    writereg(channel->pdata->pAdapter, ((PMP_ADAPTER)channel->pdata->pAdapter)->RxQueue[Qid].NumHwRecvBuffers - 1,
        FXGMAC_DMA_REG(channel, DMA_CH_RDRLR));

    /* Update the starting address of descriptor ring */
    writereg(channel->pdata->pAdapter, NdisGetPhysicalAddressHigh(((PMP_ADAPTER)channel->pdata->pAdapter)->RxQueue[Qid].HwRbdBasePa),
        FXGMAC_DMA_REG(channel, DMA_CH_RDLR_HI));
    writereg(channel->pdata->pAdapter, NdisGetPhysicalAddressLow(((PMP_ADAPTER)channel->pdata->pAdapter)->RxQueue[Qid].HwRbdBasePa),
        FXGMAC_DMA_REG(channel, DMA_CH_RDLR_LO));

    #if defined(NIC_NET_ADAPETERCX)
    /* Update the Rx Descriptor Tail Pointer */
    writereg(channel->pdata->pAdapter, NdisGetPhysicalAddressLow(((PMP_ADAPTER)channel->pdata->pAdapter)->RxQueue[Qid].HwRbdBasePa),
        FXGMAC_DMA_REG(channel, DMA_CH_RDTR_LO));
        HwRbdPa.QuadPart = 0;
    #else
    HwRbdPa.QuadPart = ((PMP_ADAPTER)channel->pdata->pAdapter)->RxQueue[Qid].HwRbdBasePa.QuadPart
        + (((PMP_ADAPTER)channel->pdata->pAdapter)->RxQueue[Qid].NumHwRecvBuffers - 1) * sizeof(HW_RBD);

    /* Update the Rx Descriptor Tail Pointer */
    writereg(channel->pdata->pAdapter, NdisGetPhysicalAddressLow(HwRbdPa), FXGMAC_DMA_REG(channel, DMA_CH_RDTR_LO));
    #endif
#endif
}

static int fxgmac_is_context_desc(struct fxgmac_dma_desc *dma_desc)
{
    /* Rx and Tx share CTXT bit, so check TDES3.CTXT bit */
    return FXGMAC_GET_REG_BITS_LE(dma_desc->desc3,
                TX_NORMAL_DESC3_CTXT_POS,
                TX_NORMAL_DESC3_CTXT_LEN);
}

static int fxgmac_is_last_desc(struct fxgmac_dma_desc *dma_desc)
{
    /* Rx and Tx share LD bit, so check TDES3.LD bit */
    return FXGMAC_GET_REG_BITS_LE(dma_desc->desc3,
                TX_NORMAL_DESC3_LD_POS,
                TX_NORMAL_DESC3_LD_LEN);
}

static int fxgmac_disable_tx_flow_control(struct fxgmac_pdata *pdata)
{
    unsigned int max_q_count, q_count;
    unsigned int reg, regval;
    unsigned int i;

    /* Clear MTL flow control */
    for (i = 0; i < pdata->rx_q_count; i++) {
        regval = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
        regval = FXGMAC_SET_REG_BITS(regval, MTL_Q_RQOMR_EHFC_POS,
                         MTL_Q_RQOMR_EHFC_LEN, 0);
        writereg(pdata->pAdapter, regval, FXGMAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
    }

    /* Clear MAC flow control */
    max_q_count = FXGMAC_MAX_FLOW_CONTROL_QUEUES;
    q_count = min_t(unsigned int, pdata->tx_q_count, max_q_count);
    reg = MAC_Q0TFCR;
    for (i = 0; i < q_count; i++) {
        regval = readreg(pdata->pAdapter, pdata->mac_regs + reg);
        regval = FXGMAC_SET_REG_BITS(regval,
                         MAC_Q0TFCR_TFE_POS,
                    MAC_Q0TFCR_TFE_LEN,
                    0);
        writereg(pdata->pAdapter, regval, pdata->mac_regs + reg);

        reg += MAC_QTFCR_INC;
    }

    return 0;
}

static int fxgmac_enable_tx_flow_control(struct fxgmac_pdata *pdata)
{
    unsigned int max_q_count, q_count;
    unsigned int reg, regval;
    unsigned int i;

    /* Set MTL flow control */
    for (i = 0; i < pdata->rx_q_count; i++) {
        regval = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
        regval = FXGMAC_SET_REG_BITS(regval, MTL_Q_RQOMR_EHFC_POS,
                         MTL_Q_RQOMR_EHFC_LEN, 1);
        writereg(pdata->pAdapter, regval, FXGMAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
    }

    /* Set MAC flow control */
    max_q_count = FXGMAC_MAX_FLOW_CONTROL_QUEUES;
    q_count = min_t(unsigned int, pdata->tx_q_count, max_q_count);
    reg = MAC_Q0TFCR;
    for (i = 0; i < q_count; i++) {
        regval = readreg(pdata->pAdapter, pdata->mac_regs + reg);

        /* Enable transmit flow control */
        regval = FXGMAC_SET_REG_BITS(regval, MAC_Q0TFCR_TFE_POS,
                         MAC_Q0TFCR_TFE_LEN, 1);
        /* Set pause time */
        regval = FXGMAC_SET_REG_BITS(regval, MAC_Q0TFCR_PT_POS,
                         MAC_Q0TFCR_PT_LEN, 0xffff);

        writereg(pdata->pAdapter, regval, pdata->mac_regs + reg);

        reg += MAC_QTFCR_INC;
    }

    return 0;
}

static int fxgmac_disable_rx_flow_control(struct fxgmac_pdata *pdata)
{
    u32 regval;

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_RFCR);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_RFCR_RFE_POS,
                     MAC_RFCR_RFE_LEN, 0);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_RFCR);

    return 0;
}

static int fxgmac_enable_rx_flow_control(struct fxgmac_pdata *pdata)
{
    u32 regval;

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_RFCR);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_RFCR_RFE_POS,
                     MAC_RFCR_RFE_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_RFCR);

    return 0;
}

static int fxgmac_config_tx_flow_control(struct fxgmac_pdata *pdata)
{
    if (pdata->tx_pause)
        fxgmac_enable_tx_flow_control(pdata);
    else
        fxgmac_disable_tx_flow_control(pdata);

    return 0;
}

static int fxgmac_config_rx_flow_control(struct fxgmac_pdata *pdata)
{
    if (pdata->rx_pause)
        fxgmac_enable_rx_flow_control(pdata);
    else
        fxgmac_disable_rx_flow_control(pdata);

    return 0;
}

static int fxgmac_config_rx_coalesce(struct fxgmac_pdata *pdata)
{
#ifndef DPDK
    struct fxgmac_channel *channel;
    unsigned int i;
    u32 regval;

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->rx_ring)
            break;

        regval = readreg(pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_RIWT));
        regval = FXGMAC_SET_REG_BITS(regval, DMA_CH_RIWT_RWT_POS,
                         DMA_CH_RIWT_RWT_LEN,
                         pdata->rx_riwt);
        writereg(pdata->pAdapter, regval, FXGMAC_DMA_REG(channel, DMA_CH_RIWT));
    }
#else
	struct fxgmac_rx_queue *rxq;
	unsigned int i;

	for (i = 0; i < pdata->expansion.eth_dev->data->nb_rx_queues; i++) {
		rxq = pdata->expansion.eth_dev->data->rx_queues[i];
		FXGMAC_DMA_IOWRITE_BITS(rxq, DMA_CH_RIWT, RWT,
				pdata->rx_riwt);
	}
#endif

    return 0;
}

static void fxgmac_config_rx_fep_disable(struct fxgmac_pdata *pdata)
{
    unsigned int i;
    u32 regval;

    for (i = 0; i < pdata->rx_q_count; i++) {
        regval = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
        regval = FXGMAC_SET_REG_BITS(regval, MTL_Q_RQOMR_FEP_POS,
                         MTL_Q_RQOMR_FEP_LEN, MTL_FEP_ENABLE);// 1:enable  the  rx queue forward packet with error status(crc error,gmii_er,watch dog timeout.or overflow)
        writereg(pdata->pAdapter, regval, FXGMAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
    }
}

static void fxgmac_config_rx_fup_enable(struct fxgmac_pdata *pdata)
{
    unsigned int i;
    u32 regval;

    for (i = 0; i < pdata->rx_q_count; i++) {
        regval = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
        regval = FXGMAC_SET_REG_BITS(regval, MTL_Q_RQOMR_FUP_POS,
                         MTL_Q_RQOMR_FUP_LEN, 1);
        writereg(pdata->pAdapter, regval, FXGMAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
    }
}

static int fxgmac_config_tx_coalesce(struct fxgmac_pdata *pdata)
{
    pdata = pdata;
    return 0;
}

static void fxgmac_config_rx_buffer_size(struct fxgmac_pdata *pdata)
{
#ifndef DPDK
    struct fxgmac_channel *channel;
    unsigned int i;
    u32 regval;

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->rx_ring)
            break;

        regval = readreg(pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_RCR));
        regval = FXGMAC_SET_REG_BITS(regval, DMA_CH_RCR_RBSZ_POS,
                         DMA_CH_RCR_RBSZ_LEN,
                    pdata->rx_buf_size);
        writereg(pdata->pAdapter, regval, FXGMAC_DMA_REG(channel, DMA_CH_RCR));
    }
#else
	struct fxgmac_rx_queue *rxq;
	unsigned int i;

	for (i = 0; i < pdata->expansion.eth_dev->data->nb_rx_queues; i++) {
		rxq = pdata->expansion.eth_dev->data->rx_queues[i];

		rxq->buf_size = rte_pktmbuf_data_room_size(rxq->mb_pool) -
			RTE_PKTMBUF_HEADROOM;
		rxq->buf_size = (rxq->buf_size + FXGMAC_RX_BUF_ALIGN - 1) &
			~(FXGMAC_RX_BUF_ALIGN - 1);

		if (rxq->buf_size > pdata->rx_buf_size)
			pdata->rx_buf_size = rxq->buf_size;

		FXGMAC_DMA_IOWRITE_BITS(rxq, DMA_CH_RCR, RBSZ,
					rxq->buf_size);
	}
#endif
}

static void fxgmac_config_tso_mode(struct fxgmac_pdata *pdata)
{
#ifndef DPDK
    struct fxgmac_channel *channel;
    unsigned int i;
    u32 regval;

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->tx_ring)
            break;

        if (pdata->hw_feat.tso) {
            regval = readreg(pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_TCR));
            regval = FXGMAC_SET_REG_BITS(regval, DMA_CH_TCR_TSE_POS,
                             DMA_CH_TCR_TSE_LEN, 1);
            writereg(pdata->pAdapter, regval, FXGMAC_DMA_REG(channel, DMA_CH_TCR));
        }
    }
#else
	struct fxgmac_tx_queue *txq;
	unsigned int i;
	for (i = 0; i < pdata->expansion.eth_dev->data->nb_tx_queues; i++) {
		txq = pdata->expansion.eth_dev->data->tx_queues[i];
		FXGMAC_DMA_IOWRITE_BITS(txq, DMA_CH_TCR, TSE,
				pdata->tx_pbl);
	}
#endif
}

static void fxgmac_config_sph_mode(struct fxgmac_pdata *pdata)
{
    unsigned int i;
    u32 regval;

#ifndef DPDK
    struct fxgmac_channel *channel;
    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->rx_ring)
            break;

        regval = readreg(pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_CR));
        regval = FXGMAC_SET_REG_BITS(regval, DMA_CH_CR_SPH_POS,
                         DMA_CH_CR_SPH_LEN, 0);
        writereg(pdata->pAdapter, regval, FXGMAC_DMA_REG(channel, DMA_CH_CR));
    }
#else
	struct fxgmac_rx_queue *rxq;

	for (i = 0; i < pdata->expansion.eth_dev->data->nb_rx_queues; i++) {
		rxq = pdata->expansion.eth_dev->data->rx_queues[i];
		FXGMAC_DMA_IOWRITE_BITS(rxq, DMA_CH_CR, SPH,
				pdata->rx_pbl);
	}
#endif

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_ECR);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_ECR_HDSMS_POS,
                     MAC_ECR_HDSMS_LEN,
                FXGMAC_SPH_HDSMS_SIZE);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_ECR);
}

static unsigned int fxgmac_usec_to_riwt(struct fxgmac_pdata *pdata,
                    unsigned int usec)
{
    unsigned long rate;
    unsigned int ret;

    rate = pdata->sysclk_rate;

    /* Convert the input usec value to the watchdog timer value. Each
     * watchdog timer value is equivalent to 256 clock cycles.
     * Calculate the required value as:
     *   ( usec * ( system_clock_mhz / 10^6 ) / 256
     */
    ret = (usec * (rate / 1000000)) / 256;

    return ret;
}

static unsigned int fxgmac_riwt_to_usec(struct fxgmac_pdata *pdata,
                    unsigned int riwt)
{
    unsigned long rate;
    unsigned int ret;

    rate = pdata->sysclk_rate;

    /* Convert the input watchdog timer value to the usec value. Each
     * watchdog timer value is equivalent to 256 clock cycles.
     * Calculate the required value as:
     *   ( riwt * 256 ) / ( system_clock_mhz / 10^6 )
     */
    ret = (riwt * 256) / (rate / 1000000);

    return ret;
}

static int fxgmac_config_rx_threshold(struct fxgmac_pdata *pdata,
                      unsigned int val)
{
    unsigned int i;
    u32 regval;

    for (i = 0; i < pdata->rx_q_count; i++) {
        regval = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
        regval = FXGMAC_SET_REG_BITS(regval, MTL_Q_RQOMR_RTC_POS,
                         MTL_Q_RQOMR_RTC_LEN, val);
        writereg(pdata->pAdapter, regval, FXGMAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
    }

    return 0;
}

static void fxgmac_config_mtl_mode(struct fxgmac_pdata *pdata)
{
    unsigned int i;
    u32 regval;

    /* Set Tx to weighted round robin scheduling algorithm */
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MTL_OMR);
    regval = FXGMAC_SET_REG_BITS(regval, MTL_OMR_ETSALG_POS,
                     MTL_OMR_ETSALG_LEN, MTL_ETSALG_WRR);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MTL_OMR);
#if 1
    /* Set Tx traffic classes to use WRR algorithm with equal weights */
    for (i = 0; i < pdata->tx_q_count/*hw_feat.tc_cnt*/; i++) {
        //regval = readreg(FXGMAC_MTL_REG(pdata, i, MTL_TC_ETSCR));
        //regval = FXGMAC_SET_REG_BITS(regval, MTL_TC_ETSCR_TSA_POS,
        //                MTL_TC_ETSCR_TSA_LEN, MTL_TSA_ETS);
        //writereg(regval, FXGMAC_MTL_REG(pdata, i, MTL_TC_ETSCR));

        regval = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, i, MTL_TC_QWR));
        regval = FXGMAC_SET_REG_BITS(regval, MTL_TC_QWR_QW_POS,
                         MTL_TC_QWR_QW_LEN, 1);
        writereg(pdata->pAdapter, regval, FXGMAC_MTL_REG(pdata, i, MTL_TC_QWR));
    }
#endif
    /* Set Rx to strict priority algorithm */
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MTL_OMR);
    regval = FXGMAC_SET_REG_BITS(regval, MTL_OMR_RAA_POS,
                     MTL_OMR_RAA_LEN, MTL_RAA_SP);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MTL_OMR);
}

static void fxgmac_config_queue_mapping(struct fxgmac_pdata *pdata)
{
    unsigned int ppq, ppq_extra, prio, prio_queues;
    //unsigned int qptc, qptc_extra;
    unsigned int queue;
    unsigned int reg, regval;
    unsigned int mask;
    unsigned int i, j;

    /* Map the MTL Tx Queues to Traffic Classes
     *   Note: Tx Queues >= Traffic Classes
     */
#if 0
    qptc = pdata->tx_q_count / pdata->hw_feat.tc_cnt;
    qptc_extra = pdata->tx_q_count % pdata->hw_feat.tc_cnt;
    for (i = 0, queue = 0; i < pdata->hw_feat.tc_cnt; i++) {
        for (j = 0; j < qptc; j++) {
            netif_dbg(pdata, drv, pdata->netdev,
                  "TXq%u mapped to TC%u\n", queue, i);
            regval = readreg(FXGMAC_MTL_REG(pdata, queue,
                              MTL_Q_TQOMR));
            regval = FXGMAC_SET_REG_BITS(regval,
                             MTL_Q_TQOMR_Q2TCMAP_POS,
                             MTL_Q_TQOMR_Q2TCMAP_LEN,
                             i);
            writereg(regval, FXGMAC_MTL_REG(pdata, queue,
                              MTL_Q_TQOMR));
            queue++;
        }

        if (i < qptc_extra) {
            netif_dbg(pdata, drv, pdata->netdev,
                  "TXq%u mapped to TC%u\n", queue, i);
            regval = readreg(FXGMAC_MTL_REG(pdata, queue,
                              MTL_Q_TQOMR));
            regval = FXGMAC_SET_REG_BITS(regval,
                             MTL_Q_TQOMR_Q2TCMAP_POS,
                             MTL_Q_TQOMR_Q2TCMAP_LEN,
                             i);
            writereg(regval, FXGMAC_MTL_REG(pdata, queue,
                              MTL_Q_TQOMR));
            queue++;
        }
    }
#else
    queue = 0;
    DPRINTK("need to map TXq(%u) to TC\n", queue);
#endif
    /* Map the 8 VLAN priority values to available MTL Rx queues */
    prio_queues = min_t(unsigned int, IEEE_8021QAZ_MAX_TCS,
                pdata->rx_q_count);
    ppq = IEEE_8021QAZ_MAX_TCS / prio_queues;
    ppq_extra = IEEE_8021QAZ_MAX_TCS % prio_queues;

    reg = MAC_RQC2R;
    regval = 0;
    for (i = 0, prio = 0; i < prio_queues;) {
        mask = 0;
        for (j = 0; j < ppq; j++) {
            netif_dbg(pdata, drv, pdata->netdev,
                  "PRIO%u mapped to RXq%u\n", prio, i);
            mask |= (1 << prio);
            prio++;
        }

        if (i < ppq_extra) {
            netif_dbg(pdata, drv, pdata->netdev,
                  "PRIO%u mapped to RXq%u\n", prio, i);
            mask |= (1 << prio);
            prio++;
        }

        regval |= (mask << ((i++ % MAC_RQC2_Q_PER_REG) << 3));

        if ((i % MAC_RQC2_Q_PER_REG) && (i != prio_queues))
            continue;

        writereg(pdata->pAdapter, regval, pdata->mac_regs + reg);
        reg += MAC_RQC2_INC;
        regval = 0;
    }

    /* Configure one to one, MTL Rx queue to DMA Rx channel mapping
     *  ie Q0 <--> CH0, Q1 <--> CH1 ... Q11 <--> CH11
     */
    reg = MTL_RQDCM0R;
    regval = readreg(pdata->pAdapter, pdata->mac_regs + reg);
    regval |= (MTL_RQDCM0R_Q0MDMACH | MTL_RQDCM0R_Q1MDMACH |
            MTL_RQDCM0R_Q2MDMACH | MTL_RQDCM0R_Q3MDMACH);

    if (pdata->rss)
    {
        /* in version later 0617, need to enable DA-based DMA Channel Selection to let RSS work,
         * ie, bit4,12,20,28 for Q0,1,2,3 individual
         */
        regval |= (MTL_RQDCM0R_Q0DDMACH | MTL_RQDCM0R_Q1DDMACH |
                MTL_RQDCM0R_Q2DDMACH | MTL_RQDCM0R_Q3DDMACH);
    }

    writereg(pdata->pAdapter, regval, pdata->mac_regs + reg);

    reg += MTL_RQDCM_INC;
    regval = readreg(pdata->pAdapter, pdata->mac_regs + reg);
    regval |= (MTL_RQDCM1R_Q4MDMACH | MTL_RQDCM1R_Q5MDMACH |
            MTL_RQDCM1R_Q6MDMACH | MTL_RQDCM1R_Q7MDMACH);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + reg);
#if 0
    reg += MTL_RQDCM_INC;
    regval = readreg(pdata->mac_regs + reg);
    regval |= (MTL_RQDCM2R_Q8MDMACH | MTL_RQDCM2R_Q9MDMACH |
            MTL_RQDCM2R_Q10MDMACH | MTL_RQDCM2R_Q11MDMACH);
    writereg(regval, pdata->mac_regs + reg);
#endif
}

static unsigned int fxgmac_calculate_per_queue_fifo(
                    unsigned int fifo_size,
                    unsigned int queue_count)
{
    unsigned int q_fifo_size;
    unsigned int p_fifo;

    /* Calculate the configured fifo size */
    q_fifo_size = 1 << (fifo_size + 7);

    /* The configured value may not be the actual amount of fifo RAM */
    q_fifo_size = min_t(unsigned int, FXGMAC_MAX_FIFO, q_fifo_size);

    q_fifo_size = q_fifo_size / queue_count;

    /* Each increment in the queue fifo size represents 256 bytes of
     * fifo, with 0 representing 256 bytes. Distribute the fifo equally
     * between the queues.
     */
    p_fifo = q_fifo_size / 256;
    if (p_fifo)
        p_fifo--;

    return p_fifo;
}

static void fxgmac_config_tx_fifo_size(struct fxgmac_pdata *pdata)
{
    unsigned int fifo_size;
    unsigned int i;
    u32 regval;

    fifo_size = fxgmac_calculate_per_queue_fifo(
                pdata->hw_feat.tx_fifo_size,
                pdata->tx_q_count);

    for (i = 0; i < pdata->tx_q_count; i++) {
        regval = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
        regval = FXGMAC_SET_REG_BITS(regval, MTL_Q_TQOMR_TQS_POS,
                         MTL_Q_TQOMR_TQS_LEN, fifo_size);
        writereg(pdata->pAdapter, regval, FXGMAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
    }

    netif_info(pdata, drv, pdata->netdev,
           "%d Tx hardware queues, %d byte fifo per queue\n",
           pdata->tx_q_count, ((fifo_size + 1) * 256));
}

static void fxgmac_config_rx_fifo_size(struct fxgmac_pdata *pdata)
{
    unsigned int fifo_size;
    unsigned int i;
    u32 regval;

    fifo_size = fxgmac_calculate_per_queue_fifo(
                    pdata->hw_feat.rx_fifo_size,
                    pdata->rx_q_count);

    for (i = 0; i < pdata->rx_q_count; i++) {
        regval = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
        regval = FXGMAC_SET_REG_BITS(regval, MTL_Q_RQOMR_RQS_POS,
                         MTL_Q_RQOMR_RQS_LEN, fifo_size);
        writereg(pdata->pAdapter, regval, FXGMAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
    }

    netif_info(pdata, drv, pdata->netdev,
           "%d Rx hardware queues, %d byte fifo per queue\n",
           pdata->rx_q_count, ((fifo_size + 1) * 256));
}

static void fxgmac_config_flow_control_threshold(struct fxgmac_pdata *pdata)
{
    unsigned int i;
    u32 regval;

    for (i = 0; i < pdata->rx_q_count; i++) {
        regval = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
        /* Activate flow control when less than 6k left in fifo */
        //In windows driver,it sets the value 6, but in Linux it sets the value 10. Here it uses value that exists in windows driver
        regval = FXGMAC_SET_REG_BITS(regval, MTL_Q_RQOMR_RFA_POS, MTL_Q_RQOMR_RFA_LEN, 6);
        /* De-activate flow control when more than 10k left in fifo */
        regval = FXGMAC_SET_REG_BITS(regval, MTL_Q_RQOMR_RFD_POS, MTL_Q_RQOMR_RFD_LEN, 10);
        writereg(pdata->pAdapter, regval, FXGMAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
    }
}

static int fxgmac_config_tx_threshold(struct fxgmac_pdata *pdata,
                      unsigned int val)
{
    unsigned int i;
    u32 regval;

    for (i = 0; i < pdata->tx_q_count; i++) {
        regval = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
        regval = FXGMAC_SET_REG_BITS(regval, MTL_Q_TQOMR_TTC_POS,
                         MTL_Q_TQOMR_TTC_LEN, val);
        writereg(pdata->pAdapter, regval, FXGMAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
    }

    return 0;
}

static int fxgmac_config_rsf_mode(struct fxgmac_pdata *pdata,
                  unsigned int val)
{
    unsigned int i;
    u32 regval;

    for (i = 0; i < pdata->rx_q_count; i++) {
        regval = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
        regval = FXGMAC_SET_REG_BITS(regval, MTL_Q_RQOMR_RSF_POS,
                         MTL_Q_RQOMR_RSF_LEN, val);
        writereg(pdata->pAdapter, regval, FXGMAC_MTL_REG(pdata, i, MTL_Q_RQOMR));
    }

    return 0;
}

static int fxgmac_config_tsf_mode(struct fxgmac_pdata *pdata,
                  unsigned int val)
{
    unsigned int i;
    u32 regval;

    for (i = 0; i < pdata->tx_q_count; i++) {
        regval = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
        regval = FXGMAC_SET_REG_BITS(regval, MTL_Q_TQOMR_TSF_POS,
                         MTL_Q_TQOMR_TSF_LEN, val);
        writereg(pdata->pAdapter, regval, FXGMAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
    }

    return 0;
}

static int fxgmac_config_osp_mode(struct fxgmac_pdata *pdata)
{
#ifndef DPDK
    struct fxgmac_channel *channel;
    unsigned int i;
    u32 regval;

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->tx_ring)
            break;

        regval = readreg(pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_TCR));
        regval = FXGMAC_SET_REG_BITS(regval, DMA_CH_TCR_OSP_POS,
                         DMA_CH_TCR_OSP_LEN,
                    pdata->tx_osp_mode);
        writereg(pdata->pAdapter, regval, FXGMAC_DMA_REG(channel, DMA_CH_TCR));
    }
#else
	/* Force DMA to operate on second packet before closing descriptors
	 *  of first packet
	 */
	struct fxgmac_tx_queue *txq;
	unsigned int i;

	for (i = 0; i < pdata->expansion.eth_dev->data->nb_tx_queues; i++) {
		txq = pdata->expansion.eth_dev->data->tx_queues[i];
		FXGMAC_DMA_IOWRITE_BITS(txq, DMA_CH_TCR, OSP,
					pdata->tx_osp_mode);
	}
#endif
    return 0;
}

static int fxgmac_config_pblx8(struct fxgmac_pdata *pdata)
{
#ifndef DPDK
    struct fxgmac_channel *channel;
    unsigned int i;
    u32 regval;

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        regval = readreg(pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_CR));
        regval = FXGMAC_SET_REG_BITS(regval, DMA_CH_CR_PBLX8_POS,
                         DMA_CH_CR_PBLX8_LEN,
                    pdata->pblx8);
        writereg(pdata->pAdapter, regval, FXGMAC_DMA_REG(channel, DMA_CH_CR));
    }
#else
	struct fxgmac_tx_queue *txq;
	unsigned int i;

	for (i = 0; i < pdata->expansion.eth_dev->data->nb_tx_queues; i++) {
		txq = pdata->expansion.eth_dev->data->tx_queues[i];
		FXGMAC_DMA_IOWRITE_BITS(txq, DMA_CH_CR, PBLX8,
					pdata->pblx8);
	}
#endif

    return 0;
}

static int fxgmac_get_tx_pbl_val(struct fxgmac_pdata *pdata)
{
    u32 regval;

    regval = readreg(pdata->pAdapter, FXGMAC_DMA_REG(pdata->channel_head, DMA_CH_TCR));
    regval = FXGMAC_GET_REG_BITS(regval, DMA_CH_TCR_PBL_POS,
                     DMA_CH_TCR_PBL_LEN);
    return regval;
}

static int fxgmac_config_tx_pbl_val(struct fxgmac_pdata *pdata)
{
#ifndef DPDK
    struct fxgmac_channel *channel;
    unsigned int i;
    u32 regval;

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->tx_ring)
            break;

        regval = readreg(pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_TCR));
        regval = FXGMAC_SET_REG_BITS(regval, DMA_CH_TCR_PBL_POS,
                         DMA_CH_TCR_PBL_LEN,
                    pdata->tx_pbl);
        writereg(pdata->pAdapter, regval, FXGMAC_DMA_REG(channel, DMA_CH_TCR));
    }
#else
	struct fxgmac_tx_queue *txq;
	unsigned int i;

	for (i = 0; i < pdata->expansion.eth_dev->data->nb_tx_queues; i++) {
		txq = pdata->expansion.eth_dev->data->tx_queues[i];
		FXGMAC_DMA_IOWRITE_BITS(txq, DMA_CH_TCR, PBL,
				pdata->tx_pbl);
	}
#endif

    return 0;
}

static int fxgmac_get_rx_pbl_val(struct fxgmac_pdata *pdata)
{
    u32 regval;

    regval = readreg(pdata->pAdapter, FXGMAC_DMA_REG(pdata->channel_head, DMA_CH_RCR));
    regval = FXGMAC_GET_REG_BITS(regval, DMA_CH_RCR_PBL_POS,
                     DMA_CH_RCR_PBL_LEN);
    return regval;
}

static int fxgmac_config_rx_pbl_val(struct fxgmac_pdata *pdata)
{
#ifndef DPDK
    struct fxgmac_channel *channel;
    unsigned int i;
    u32 regval;

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->rx_ring)
            break;

        regval = readreg(pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_RCR));
        regval = FXGMAC_SET_REG_BITS(regval, DMA_CH_RCR_PBL_POS,
                         DMA_CH_RCR_PBL_LEN,
                    pdata->rx_pbl);
        writereg(pdata->pAdapter, regval, FXGMAC_DMA_REG(channel, DMA_CH_RCR));
    }
#else
	struct fxgmac_rx_queue *rxq;
	unsigned int i;

	for (i = 0; i < pdata->expansion.eth_dev->data->nb_rx_queues; i++) {
		rxq = pdata->expansion.eth_dev->data->rx_queues[i];
		FXGMAC_DMA_IOWRITE_BITS(rxq, DMA_CH_RCR, PBL,
				pdata->rx_pbl);
	}
#endif

    return 0;
}

static u64 fxgmac_mmc_read(struct fxgmac_pdata *pdata, unsigned int reg_lo)
{
    /* bool read_hi; */
    u64 val;
#if 0
    switch (reg_lo) {
    /* These registers are always 64 bit */
    case MMC_TXOCTETCOUNT_GB_LO:
    case MMC_TXOCTETCOUNT_G_LO:
    case MMC_RXOCTETCOUNT_GB_LO:
    case MMC_RXOCTETCOUNT_G_LO:
        read_hi = true;
        break;

    default:
        read_hi = false;
    }
#endif
    val = (u64)readreg(pdata->pAdapter, pdata->mac_regs + reg_lo);

    /*
    if (read_hi)
        val |= ((u64)readreg(pdata->mac_regs + reg_lo + 4) << 32);
    */

    return val;
}

static void fxgmac_tx_mmc_int(struct fxgmac_pdata *pdata)
{
    unsigned int mmc_isr = readreg(pdata->pAdapter, pdata->mac_regs + MMC_TISR);
    struct fxgmac_stats *stats = &pdata->stats;

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXOCTETCOUNT_GB_POS,
                MMC_TISR_TXOCTETCOUNT_GB_LEN))
        stats->txoctetcount_gb +=
            fxgmac_mmc_read(pdata, MMC_TXOCTETCOUNT_GB_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXFRAMECOUNT_GB_POS,
                MMC_TISR_TXFRAMECOUNT_GB_LEN))
        stats->txframecount_gb +=
            fxgmac_mmc_read(pdata, MMC_TXFRAMECOUNT_GB_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXBROADCASTFRAMES_G_POS,
                MMC_TISR_TXBROADCASTFRAMES_G_LEN))
        stats->txbroadcastframes_g +=
            fxgmac_mmc_read(pdata, MMC_TXBROADCASTFRAMES_G_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXMULTICASTFRAMES_G_POS,
                MMC_TISR_TXMULTICASTFRAMES_G_LEN))
        stats->txmulticastframes_g +=
            fxgmac_mmc_read(pdata, MMC_TXMULTICASTFRAMES_G_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TX64OCTETS_GB_POS,
                MMC_TISR_TX64OCTETS_GB_LEN))
        stats->tx64octets_gb +=
            fxgmac_mmc_read(pdata, MMC_TX64OCTETS_GB_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TX65TO127OCTETS_GB_POS,
                MMC_TISR_TX65TO127OCTETS_GB_LEN))
        stats->tx65to127octets_gb +=
            fxgmac_mmc_read(pdata, MMC_TX65TO127OCTETS_GB_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TX128TO255OCTETS_GB_POS,
                MMC_TISR_TX128TO255OCTETS_GB_LEN))
        stats->tx128to255octets_gb +=
            fxgmac_mmc_read(pdata, MMC_TX128TO255OCTETS_GB_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TX256TO511OCTETS_GB_POS,
                MMC_TISR_TX256TO511OCTETS_GB_LEN))
        stats->tx256to511octets_gb +=
            fxgmac_mmc_read(pdata, MMC_TX256TO511OCTETS_GB_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TX512TO1023OCTETS_GB_POS,
                MMC_TISR_TX512TO1023OCTETS_GB_LEN))
        stats->tx512to1023octets_gb +=
            fxgmac_mmc_read(pdata, MMC_TX512TO1023OCTETS_GB_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TX1024TOMAXOCTETS_GB_POS,
                MMC_TISR_TX1024TOMAXOCTETS_GB_LEN))
        stats->tx1024tomaxoctets_gb +=
            fxgmac_mmc_read(pdata, MMC_TX1024TOMAXOCTETS_GB_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXUNICASTFRAMES_GB_POS,
                MMC_TISR_TXUNICASTFRAMES_GB_LEN))
        stats->txunicastframes_gb +=
            fxgmac_mmc_read(pdata, MMC_TXUNICASTFRAMES_GB_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXMULTICASTFRAMES_GB_POS,
                MMC_TISR_TXMULTICASTFRAMES_GB_LEN))
        stats->txmulticastframes_gb +=
            fxgmac_mmc_read(pdata, MMC_TXMULTICASTFRAMES_GB_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXBROADCASTFRAMES_GB_POS,
                MMC_TISR_TXBROADCASTFRAMES_GB_LEN))
        stats->txbroadcastframes_g +=
            fxgmac_mmc_read(pdata, MMC_TXBROADCASTFRAMES_GB_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXUNDERFLOWERROR_POS,
                MMC_TISR_TXUNDERFLOWERROR_LEN))
        stats->txunderflowerror +=
            fxgmac_mmc_read(pdata, MMC_TXUNDERFLOWERROR_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXSINGLECOLLISION_G_POS,
                MMC_TISR_TXSINGLECOLLISION_G_LEN))
        stats->txsinglecollision_g +=
            fxgmac_mmc_read(pdata, MMC_TXSINGLECOLLISION_G);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXMULTIPLECOLLISION_G_POS,
                MMC_TISR_TXMULTIPLECOLLISION_G_LEN))
        stats->txmultiplecollision_g +=
            fxgmac_mmc_read(pdata, MMC_TXMULTIPLECOLLISION_G);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXDEFERREDFRAMES_POS,
                MMC_TISR_TXDEFERREDFRAMES_LEN))
        stats->txdeferredframes +=
            fxgmac_mmc_read(pdata, MMC_TXDEFERREDFRAMES);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXLATECOLLISIONFRAMES_POS,
                MMC_TISR_TXLATECOLLISIONFRAMES_LEN))
        stats->txlatecollisionframes +=
            fxgmac_mmc_read(pdata, MMC_TXLATECOLLISIONFRAMES);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXEXCESSIVECOLLISIONFRAMES_POS,
                MMC_TISR_TXEXCESSIVECOLLISIONFRAMES_LEN))
        stats->txexcessivecollisionframes +=
            fxgmac_mmc_read(pdata, MMC_TXEXCESSIVECOLLSIONFRAMES);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXCARRIERERRORFRAMES_POS,
                MMC_TISR_TXCARRIERERRORFRAMES_LEN))
        stats->txcarriererrorframes +=
            fxgmac_mmc_read(pdata, MMC_TXCARRIERERRORFRAMES);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXOCTETCOUNT_G_POS,
                MMC_TISR_TXOCTETCOUNT_G_LEN))
        stats->txoctetcount_g +=
            fxgmac_mmc_read(pdata, MMC_TXOCTETCOUNT_G_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXFRAMECOUNT_G_POS,
                MMC_TISR_TXFRAMECOUNT_G_LEN))
        stats->txframecount_g +=
            fxgmac_mmc_read(pdata, MMC_TXFRAMECOUNT_G_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXEXCESSIVEDEFERRALFRAMES_POS,
                MMC_TISR_TXEXCESSIVEDEFERRALFRAMES_LEN))
        stats->txexcessivedeferralerror +=
            fxgmac_mmc_read(pdata, MMC_TXEXCESSIVEDEFERRALERROR);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXPAUSEFRAMES_POS,
                MMC_TISR_TXPAUSEFRAMES_LEN))
        stats->txpauseframes +=
            fxgmac_mmc_read(pdata, MMC_TXPAUSEFRAMES_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXVLANFRAMES_G_POS,
                MMC_TISR_TXVLANFRAMES_G_LEN))
        stats->txvlanframes_g +=
            fxgmac_mmc_read(pdata, MMC_TXVLANFRAMES_G_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_TISR_TXOVERSIZE_G_POS,
                MMC_TISR_TXOVERSIZE_G_LEN))
        stats->txoversize_g +=
            fxgmac_mmc_read(pdata, MMC_TXOVERSIZEFRAMES);
}

static void fxgmac_rx_mmc_int(struct fxgmac_pdata *pdata)
{
    unsigned int mmc_isr = readreg(pdata->pAdapter, pdata->mac_regs + MMC_RISR);
    struct fxgmac_stats *stats = &pdata->stats;

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXFRAMECOUNT_GB_POS,
                MMC_RISR_RXFRAMECOUNT_GB_LEN))
        stats->rxframecount_gb +=
            fxgmac_mmc_read(pdata, MMC_RXFRAMECOUNT_GB_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXOCTETCOUNT_GB_POS,
                MMC_RISR_RXOCTETCOUNT_GB_LEN))
        stats->rxoctetcount_gb +=
            fxgmac_mmc_read(pdata, MMC_RXOCTETCOUNT_GB_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXOCTETCOUNT_G_POS,
                MMC_RISR_RXOCTETCOUNT_G_LEN))
        stats->rxoctetcount_g +=
            fxgmac_mmc_read(pdata, MMC_RXOCTETCOUNT_G_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXBROADCASTFRAMES_G_POS,
                MMC_RISR_RXBROADCASTFRAMES_G_LEN))
        stats->rxbroadcastframes_g +=
            fxgmac_mmc_read(pdata, MMC_RXBROADCASTFRAMES_G_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXMULTICASTFRAMES_G_POS,
                MMC_RISR_RXMULTICASTFRAMES_G_LEN))
        stats->rxmulticastframes_g +=
            fxgmac_mmc_read(pdata, MMC_RXMULTICASTFRAMES_G_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXCRCERROR_POS,
                MMC_RISR_RXCRCERROR_LEN))
        stats->rxcrcerror +=
            fxgmac_mmc_read(pdata, MMC_RXCRCERROR_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXALIGNERROR_POS,
                MMC_RISR_RXALIGNERROR_LEN))
        stats->rxalignerror +=
            fxgmac_mmc_read(pdata, MMC_RXALIGNERROR);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXRUNTERROR_POS,
                MMC_RISR_RXRUNTERROR_LEN))
        stats->rxrunterror +=
            fxgmac_mmc_read(pdata, MMC_RXRUNTERROR);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXJABBERERROR_POS,
                MMC_RISR_RXJABBERERROR_LEN))
        stats->rxjabbererror +=
            fxgmac_mmc_read(pdata, MMC_RXJABBERERROR);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXUNDERSIZE_G_POS,
                MMC_RISR_RXUNDERSIZE_G_LEN))
        stats->rxundersize_g +=
            fxgmac_mmc_read(pdata, MMC_RXUNDERSIZE_G);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXOVERSIZE_G_POS,
                MMC_RISR_RXOVERSIZE_G_LEN))
        stats->rxoversize_g +=
            fxgmac_mmc_read(pdata, MMC_RXOVERSIZE_G);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RX64OCTETS_GB_POS,
                MMC_RISR_RX64OCTETS_GB_LEN))
        stats->rx64octets_gb +=
            fxgmac_mmc_read(pdata, MMC_RX64OCTETS_GB_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RX65TO127OCTETS_GB_POS,
                MMC_RISR_RX65TO127OCTETS_GB_LEN))
        stats->rx65to127octets_gb +=
            fxgmac_mmc_read(pdata, MMC_RX65TO127OCTETS_GB_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RX128TO255OCTETS_GB_POS,
                MMC_RISR_RX128TO255OCTETS_GB_LEN))
        stats->rx128to255octets_gb +=
            fxgmac_mmc_read(pdata, MMC_RX128TO255OCTETS_GB_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RX256TO511OCTETS_GB_POS,
                MMC_RISR_RX256TO511OCTETS_GB_LEN))
        stats->rx256to511octets_gb +=
            fxgmac_mmc_read(pdata, MMC_RX256TO511OCTETS_GB_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RX512TO1023OCTETS_GB_POS,
                MMC_RISR_RX512TO1023OCTETS_GB_LEN))
        stats->rx512to1023octets_gb +=
            fxgmac_mmc_read(pdata, MMC_RX512TO1023OCTETS_GB_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RX1024TOMAXOCTETS_GB_POS,
                MMC_RISR_RX1024TOMAXOCTETS_GB_LEN))
        stats->rx1024tomaxoctets_gb +=
            fxgmac_mmc_read(pdata, MMC_RX1024TOMAXOCTETS_GB_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXUNICASTFRAMES_G_POS,
                MMC_RISR_RXUNICASTFRAMES_G_LEN))
        stats->rxunicastframes_g +=
            fxgmac_mmc_read(pdata, MMC_RXUNICASTFRAMES_G_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXLENGTHERROR_POS,
                MMC_RISR_RXLENGTHERROR_LEN))
        stats->rxlengtherror +=
            fxgmac_mmc_read(pdata, MMC_RXLENGTHERROR_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXOUTOFRANGETYPE_POS,
                MMC_RISR_RXOUTOFRANGETYPE_LEN))
        stats->rxoutofrangetype +=
            fxgmac_mmc_read(pdata, MMC_RXOUTOFRANGETYPE_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXPAUSEFRAMES_POS,
                MMC_RISR_RXPAUSEFRAMES_LEN))
        stats->rxpauseframes +=
            fxgmac_mmc_read(pdata, MMC_RXPAUSEFRAMES_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXFIFOOVERFLOW_POS,
                MMC_RISR_RXFIFOOVERFLOW_LEN))
        stats->rxfifooverflow +=
            fxgmac_mmc_read(pdata, MMC_RXFIFOOVERFLOW_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXVLANFRAMES_GB_POS,
                MMC_RISR_RXVLANFRAMES_GB_LEN))
        stats->rxvlanframes_gb +=
            fxgmac_mmc_read(pdata, MMC_RXVLANFRAMES_GB_LO);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXWATCHDOGERROR_POS,
                MMC_RISR_RXWATCHDOGERROR_LEN))
        stats->rxwatchdogerror +=
            fxgmac_mmc_read(pdata, MMC_RXWATCHDOGERROR);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXERRORFRAMES_POS,
                MMC_RISR_RXERRORFRAMES_LEN))
        stats->rxreceiveerrorframe +=
            fxgmac_mmc_read(pdata, MMC_RXRECEIVEERRORFRAME);

    if (FXGMAC_GET_REG_BITS(mmc_isr,
                MMC_RISR_RXERRORCONTROLFRAMES_POS,
                MMC_RISR_RXERRORCONTROLFRAMES_LEN))
        stats->rxcontrolframe_g +=
            fxgmac_mmc_read(pdata, MMC_RXCONTROLFRAME_G);
}

static void fxgmac_read_mmc_stats(struct fxgmac_pdata *pdata)
{
    struct fxgmac_stats *stats = &pdata->stats;
    u32 regval;

    /* Freeze counters */
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MMC_CR);
    regval = FXGMAC_SET_REG_BITS(regval, MMC_CR_MCF_POS,
                     MMC_CR_MCF_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MMC_CR);

    stats->txoctetcount_gb +=
        fxgmac_mmc_read(pdata, MMC_TXOCTETCOUNT_GB_LO);

    stats->txframecount_gb +=
        fxgmac_mmc_read(pdata, MMC_TXFRAMECOUNT_GB_LO);

    stats->txbroadcastframes_g +=
        fxgmac_mmc_read(pdata, MMC_TXBROADCASTFRAMES_G_LO);

    stats->txmulticastframes_g +=
        fxgmac_mmc_read(pdata, MMC_TXMULTICASTFRAMES_G_LO);

    stats->tx64octets_gb +=
        fxgmac_mmc_read(pdata, MMC_TX64OCTETS_GB_LO);

    stats->tx65to127octets_gb +=
        fxgmac_mmc_read(pdata, MMC_TX65TO127OCTETS_GB_LO);

    stats->tx128to255octets_gb +=
        fxgmac_mmc_read(pdata, MMC_TX128TO255OCTETS_GB_LO);

    stats->tx256to511octets_gb +=
        fxgmac_mmc_read(pdata, MMC_TX256TO511OCTETS_GB_LO);

    stats->tx512to1023octets_gb +=
        fxgmac_mmc_read(pdata, MMC_TX512TO1023OCTETS_GB_LO);

    stats->tx1024tomaxoctets_gb +=
        fxgmac_mmc_read(pdata, MMC_TX1024TOMAXOCTETS_GB_LO);

    stats->txunicastframes_gb +=
        fxgmac_mmc_read(pdata, MMC_TXUNICASTFRAMES_GB_LO);

    stats->txmulticastframes_gb +=
        fxgmac_mmc_read(pdata, MMC_TXMULTICASTFRAMES_GB_LO);

    stats->txbroadcastframes_g +=
        fxgmac_mmc_read(pdata, MMC_TXBROADCASTFRAMES_GB_LO);

    stats->txunderflowerror +=
        fxgmac_mmc_read(pdata, MMC_TXUNDERFLOWERROR_LO);

    stats->txsinglecollision_g +=
        fxgmac_mmc_read(pdata, MMC_TXSINGLECOLLISION_G);

    stats->txmultiplecollision_g +=
        fxgmac_mmc_read(pdata, MMC_TXMULTIPLECOLLISION_G);

    stats->txdeferredframes +=
        fxgmac_mmc_read(pdata, MMC_TXDEFERREDFRAMES);

    stats->txlatecollisionframes +=
        fxgmac_mmc_read(pdata, MMC_TXLATECOLLISIONFRAMES);

    stats->txexcessivecollisionframes +=
        fxgmac_mmc_read(pdata, MMC_TXEXCESSIVECOLLSIONFRAMES);

    stats->txcarriererrorframes +=
        fxgmac_mmc_read(pdata, MMC_TXCARRIERERRORFRAMES);

    stats->txoctetcount_g +=
        fxgmac_mmc_read(pdata, MMC_TXOCTETCOUNT_G_LO);

    stats->txframecount_g +=
        fxgmac_mmc_read(pdata, MMC_TXFRAMECOUNT_G_LO);

    stats->txexcessivedeferralerror +=
        fxgmac_mmc_read(pdata, MMC_TXEXCESSIVEDEFERRALERROR);

    stats->txpauseframes +=
        fxgmac_mmc_read(pdata, MMC_TXPAUSEFRAMES_LO);

    stats->txvlanframes_g +=
        fxgmac_mmc_read(pdata, MMC_TXVLANFRAMES_G_LO);

    stats->txoversize_g +=
        fxgmac_mmc_read(pdata, MMC_TXOVERSIZEFRAMES);

    stats->rxframecount_gb +=
        fxgmac_mmc_read(pdata, MMC_RXFRAMECOUNT_GB_LO);

    stats->rxoctetcount_gb +=
        fxgmac_mmc_read(pdata, MMC_RXOCTETCOUNT_GB_LO);

    stats->rxoctetcount_g +=
        fxgmac_mmc_read(pdata, MMC_RXOCTETCOUNT_G_LO);

    stats->rxbroadcastframes_g +=
        fxgmac_mmc_read(pdata, MMC_RXBROADCASTFRAMES_G_LO);

    stats->rxmulticastframes_g +=
        fxgmac_mmc_read(pdata, MMC_RXMULTICASTFRAMES_G_LO);

    stats->rxcrcerror +=
        fxgmac_mmc_read(pdata, MMC_RXCRCERROR_LO);

    stats->rxalignerror +=
        fxgmac_mmc_read(pdata, MMC_RXALIGNERROR);

    stats->rxrunterror +=
        fxgmac_mmc_read(pdata, MMC_RXRUNTERROR);

    stats->rxjabbererror +=
        fxgmac_mmc_read(pdata, MMC_RXJABBERERROR);

    stats->rxundersize_g +=
        fxgmac_mmc_read(pdata, MMC_RXUNDERSIZE_G);

    stats->rxoversize_g +=
        fxgmac_mmc_read(pdata, MMC_RXOVERSIZE_G);

    stats->rx64octets_gb +=
        fxgmac_mmc_read(pdata, MMC_RX64OCTETS_GB_LO);

    stats->rx65to127octets_gb +=
        fxgmac_mmc_read(pdata, MMC_RX65TO127OCTETS_GB_LO);

    stats->rx128to255octets_gb +=
        fxgmac_mmc_read(pdata, MMC_RX128TO255OCTETS_GB_LO);

    stats->rx256to511octets_gb +=
        fxgmac_mmc_read(pdata, MMC_RX256TO511OCTETS_GB_LO);

    stats->rx512to1023octets_gb +=
        fxgmac_mmc_read(pdata, MMC_RX512TO1023OCTETS_GB_LO);

    stats->rx1024tomaxoctets_gb +=
        fxgmac_mmc_read(pdata, MMC_RX1024TOMAXOCTETS_GB_LO);

    stats->rxunicastframes_g +=
        fxgmac_mmc_read(pdata, MMC_RXUNICASTFRAMES_G_LO);

    stats->rxlengtherror +=
        fxgmac_mmc_read(pdata, MMC_RXLENGTHERROR_LO);

    stats->rxoutofrangetype +=
        fxgmac_mmc_read(pdata, MMC_RXOUTOFRANGETYPE_LO);

    stats->rxpauseframes +=
        fxgmac_mmc_read(pdata, MMC_RXPAUSEFRAMES_LO);

    stats->rxfifooverflow +=
        fxgmac_mmc_read(pdata, MMC_RXFIFOOVERFLOW_LO);

    stats->rxvlanframes_gb +=
        fxgmac_mmc_read(pdata, MMC_RXVLANFRAMES_GB_LO);

    stats->rxwatchdogerror +=
        fxgmac_mmc_read(pdata, MMC_RXWATCHDOGERROR);

    stats->rxreceiveerrorframe +=
        fxgmac_mmc_read(pdata, MMC_RXRECEIVEERRORFRAME);

    stats->rxcontrolframe_g +=
        fxgmac_mmc_read(pdata, MMC_RXCONTROLFRAME_G);

    /* Un-freeze counters */
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MMC_CR);
    regval = FXGMAC_SET_REG_BITS(regval, MMC_CR_MCF_POS,
                     MMC_CR_MCF_LEN, 0);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MMC_CR);
}

static void fxgmac_config_mmc(struct fxgmac_pdata *pdata)
{
    u32 regval;

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MMC_CR);
    /* Set counters to reset on read */
    regval = FXGMAC_SET_REG_BITS(regval, MMC_CR_ROR_POS,
                     MMC_CR_ROR_LEN, 1);
    /* Reset the counters */
    regval = FXGMAC_SET_REG_BITS(regval, MMC_CR_CR_POS,
                     MMC_CR_CR_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MMC_CR);

#if defined(FUXI_MISC_INT_HANDLE_FEATURE_EN) && FUXI_MISC_INT_HANDLE_FEATURE_EN
    //disable interrupts for rx Tcp and ip good pkt counter which is not read by MMC counter process.
    //regval = readreg(pdata->mac_regs + 0x800);
    writereg(pdata->pAdapter, 0xffffffff, pdata->mac_regs + MMC_IPCRXINTMASK);
#endif
}

static int fxgmac_write_rss_reg(struct fxgmac_pdata *pdata, unsigned int type,
                unsigned int index, unsigned int val)
{
    int ret = 0;
    type = type;

    writereg(pdata->pAdapter, val, (pdata->base_mem+ index));

    return ret;
}

static u32 fxgmac_read_rss_options(struct fxgmac_pdata *pdata)
{
    u32 regval;

    regval = readreg(pdata->pAdapter, pdata->base_mem + MGMT_RSS_CTRL);

    /* Get the RSS options bits */
    regval = FXGMAC_GET_REG_BITS(regval, MGMT_RSS_CTRL_OPT_POS, MGMT_RSS_CTRL_OPT_LEN);

    return regval;
}

static int fxgmac_write_rss_options(struct fxgmac_pdata *pdata)
{
    u32 regval;

    regval = readreg(pdata->pAdapter, pdata->base_mem + MGMT_RSS_CTRL);

    /* Set the RSS options */
    regval = FXGMAC_SET_REG_BITS(regval, MGMT_RSS_CTRL_OPT_POS,
        MGMT_RSS_CTRL_OPT_LEN, pdata->rss_options);

    writereg(pdata->pAdapter, regval, (pdata->base_mem + MGMT_RSS_CTRL));

    return 0;
}

#if  !defined(DPDK)
static int fxgmac_read_rss_hash_key(struct fxgmac_pdata *pdata, u8 *key_buf)
{
    //struct net_device *netdev = pdata->netdev;
    unsigned int key_regs = sizeof(pdata->rss_key) / sizeof(u32);
    u32 *key = (u32 *)key_buf;

    while (key_regs--) {
        (*key) = cpu_to_be32(readreg(pdata->pAdapter, pdata->base_mem + (MGMT_RSS_KEY0 + key_regs * MGMT_RSS_KEY_REG_INC))) ;

        DBGPRINT(MP_LOUD, ("fxgmac_read_rss_hash_key: idx=%d, reg=%x, key=0x%08x\n",
            key_regs, MGMT_RSS_KEY0 + key_regs * MGMT_RSS_KEY_REG_INC, (u32)(*key)));
        key++;
    }

    return 0;
}
#endif

static int fxgmac_write_rss_hash_key(struct fxgmac_pdata *pdata)
{
    unsigned int key_regs = sizeof(pdata->rss_key) / sizeof(u32);
    u32			*key = (u32 *)&pdata->rss_key;
    int ret;

    while (key_regs--) {
        ret = fxgmac_write_rss_reg(pdata, FXGMAC_RSS_HASH_KEY_TYPE,
                       MGMT_RSS_KEY0 + key_regs * MGMT_RSS_KEY_REG_INC, cpu_to_be32 (*key));
        if (ret)
            return ret;
        key++;
    }

    return 0;
}

static int fxgmac_write_rss_lookup_table(struct fxgmac_pdata *pdata)
{
    unsigned int i, j;
    u32 regval = 0;
    int ret;

    for (i = 0, j = 0; i < ARRAY_SIZE(pdata->rss_table); i++, j++) {
        if(j < MGMT_RSS_IDT_ENTRY_PER_REG)
        {
            regval |= ((pdata->rss_table[i] & MGMT_RSS_IDT_ENTRY_MASK) << (j * 2));
        }else{
            ret = fxgmac_write_rss_reg(pdata,
                           FXGMAC_RSS_LOOKUP_TABLE_TYPE, MGMT_RSS_IDT + (i / MGMT_RSS_IDT_ENTRY_PER_REG - 1) * MGMT_RSS_IDT_REG_INC,
                           regval);
            if (ret)
                return ret;

            regval = pdata->rss_table[i];
            j = 0;
        }
    }

    if (j == MGMT_RSS_IDT_ENTRY_PER_REG)
    {
        //last IDT
        fxgmac_write_rss_reg(pdata,
            FXGMAC_RSS_LOOKUP_TABLE_TYPE, MGMT_RSS_IDT + (i / MGMT_RSS_IDT_ENTRY_PER_REG - 1) * MGMT_RSS_IDT_REG_INC,
            regval);
    }

    return 0;
}

static int fxgmac_set_rss_hash_key(struct fxgmac_pdata *pdata, const u8 *key)
{
    memcpy(pdata->rss_key, key, sizeof(pdata->rss_key));//CopyMem

    return fxgmac_write_rss_hash_key(pdata);
}

static int fxgmac_set_rss_lookup_table(struct fxgmac_pdata *pdata,
                       const u32 *table)
{
    unsigned int i;
    u32 tval;

#if FXGMAC_MSIX_CH0RXDIS_EN
        DPRINTK("Set_rss_table, rss ctrl eth=0x%08x\n", 0);

        return 0;
#endif

    for (i = 0; i < ARRAY_SIZE(pdata->rss_table); i++) {
        tval = table[i];
        pdata->rss_table[i] = FXGMAC_SET_REG_BITS(
                        pdata->rss_table[i],
                        MAC_RSSDR_DMCH_POS,
                        MAC_RSSDR_DMCH_LEN,
                        tval);
    }

    return fxgmac_write_rss_lookup_table(pdata);
}

static u32 log2ex(u32 value)
{
    u32 i = 31;
    while (i > 0)
    {
        if (value & 0x80000000)
        {
            break;
        }
        value <<= 1;
        i--;
    }
    return i;
}

static int fxgmac_enable_rss(struct fxgmac_pdata* pdata)
{
    u32 regval;
    u32 size = 0;

#ifdef LINUX
    int ret;

    if (!pdata->hw_feat.rss) {
        return -EOPNOTSUPP;
    }

    /* Program the hash key */
    ret = fxgmac_write_rss_hash_key(pdata);
    if (ret) {
        return ret;
    }

    /* Program the lookup table */
    ret = fxgmac_write_rss_lookup_table(pdata);
    if (ret) {
        return ret;
    }
#endif

    regval = readreg(pdata->pAdapter, pdata->base_mem + MGMT_RSS_CTRL);

    //2022-04-19 xiaojiang comment
    //In Linux driver, it does not set the IDT table size, but windos implement it, so linux driver follow it.
    /* Set RSS IDT table size */
    size = log2ex(FXGMAC_RSS_MAX_TABLE_SIZE) - 1;
    regval = FXGMAC_SET_REG_BITS(regval, MGMT_RSS_CTRL_TBL_SIZE_POS,
                     MGMT_RSS_CTRL_TBL_SIZE_LEN, size);

#if FXGMAC_MSIX_CH0RXDIS_EN
    /* set default cpu id to 1 */
    regval = FXGMAC_SET_REG_BITS(regval, 8,
                     2, 1);
#endif
    /* Enable RSS */
    regval = FXGMAC_SET_REG_BITS(regval, MAC_RSSCR_RSSE_POS,
                     MAC_RSSCR_RSSE_LEN, 1);

    /* Set the RSS options */
    regval = FXGMAC_SET_REG_BITS(regval, MGMT_RSS_CTRL_OPT_POS,
                     MGMT_RSS_CTRL_OPT_LEN, pdata->rss_options);

    writereg(pdata->pAdapter, regval, (pdata->base_mem + MGMT_RSS_CTRL));
    DPRINTK("enable_rss callout, rss ctrl reg=0x%08x\n", regval);

    return 0;
}

static int fxgmac_disable_rss(struct fxgmac_pdata *pdata)
{
    u32 regval;

    if (!pdata->hw_feat.rss)
        return -EOPNOTSUPP;

#if FXGMAC_MSIX_CH0RXDIS_EN
        DPRINTK("Disable_rss, rss ctrl eth=0x%08x\n", 0);

        return 0;
#endif

    regval = readreg(pdata->pAdapter, pdata->base_mem + MGMT_RSS_CTRL);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_RSSCR_RSSE_POS,
                     MAC_RSSCR_RSSE_LEN, 0);

    writereg(pdata->pAdapter, regval, (pdata->base_mem + MGMT_RSS_CTRL));
    DPRINTK("disable_rss, rss ctrl reg=0x%08x\n", regval);

    return 0;
}

static void fxgmac_config_rss(struct fxgmac_pdata *pdata)
{
    int ret;

    if (!pdata->hw_feat.rss)
        return;

    if (pdata->rss)
        ret = fxgmac_enable_rss(pdata);
    else
        ret = fxgmac_disable_rss(pdata);

    if (ret)
        DBGPRINT(MP_ERROR, ("fxgmac_config_rss: error configuring RSS\n"));
}

#if defined(LINUX) || defined(_WIN64) || defined(_WIN32)
//
void fxgmac_update_aoe_ipv4addr(struct fxgmac_pdata* pdata, u8* ip_addr)
{
    unsigned int regval, ipval = 0;

    /* enable or disable ARP offload engine. */
    if (!pdata->hw_feat.aoe) {
        netdev_err(pdata->netdev, "error update ip addr - arp offload not supported.\n");
        return;
    }

#if 0
    if(ip_addr) {
        sscanf(ip_addr, "%x.%x.%x.%x", (unsigned int *)&tmp32[0], (unsigned int *)&tmp32[1], (unsigned int *)&tmp32[2], (unsigned int *)&tmp32[3]);
        tmp[0] = (uint8_t)tmp32[0];
        tmp[1] = (uint8_t)tmp32[1];
        tmp[2] = (uint8_t)tmp32[2];
        tmp[3] = (uint8_t)tmp32[3];
    } else
    {
        ipval = 0; //0xc0a801ca; //here just hard code to 192.168.1.202
    }
#endif

    if(ip_addr) {
        //inet_aton((const char *)ip_addr, (struct in_addr *)&ipval);
        //ipval = (unsigned int)in_aton((const char *)ip_addr);
        ipval = (ip_addr[0] << 24) | (ip_addr[1] << 16) | (ip_addr[2] << 8) | (ip_addr[3] << 0);
        DPRINTK("%s, covert IP dotted-addr %s to binary 0x%08x ok.\n", __FUNCTION__, ip_addr, cpu_to_be32(ipval));
    } else
    {
#ifdef LINUX
        /* get ipv4 addr from net device */
        //2022-04-25 xiaojiang Linux Driver behavior
        ipval = fxgmac_get_netdev_ip4addr(pdata);
        DPRINTK("%s, Get net device binary IP ok, 0x%08x\n", __FUNCTION__, cpu_to_be32(ipval));

        ipval = cpu_to_be32(ipval);
#endif
    }

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_ARP_PROTO_ADDR);
    if(regval != /*cpu_to_be32*/(ipval)) {
        writereg(pdata->pAdapter, /*cpu_to_be32*/(ipval), pdata->mac_regs + MAC_ARP_PROTO_ADDR);
        DPRINTK("%s, update arp ipaddr reg from 0x%08x to 0x%08x\n", __FUNCTION__, regval, /*cpu_to_be32*/(ipval));
    }
}

void fxgmac_config_arp_offload(struct fxgmac_pdata *pdata, int en)
{
    unsigned int regval = 0;

    /* enable or disable ARP offload engine. */
    if (!pdata->hw_feat.aoe) {
        netdev_err(pdata->netdev, "error configuring ARP offload - not supported.\n");
        return;
    }

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_CR);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_ARPEN_POS, MAC_CR_ARPEN_LEN, en ? 1 : 0);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_CR);

    DPRINTK("config_aoe, reg val=0x%08x, arp offload en=%d\n", regval, en);
}

static int fxgmac_enable_arp_offload(struct fxgmac_pdata* pdata)
{
    u32 regval;

    if (!pdata->hw_feat.aoe)
        return -EOPNOTSUPP;

    /* Enable arpoffload */
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_CR);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_ARPEN_POS,
        MAC_CR_ARPEN_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_CR);

    return 0;
}

static int fxgmac_disable_arp_offload(struct fxgmac_pdata* pdata)
{
    u32 regval;

    if (!pdata->hw_feat.aoe)
        return -EOPNOTSUPP;
    /* disable arpoffload */
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_CR);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_ARPEN_POS,
        MAC_CR_ARPEN_LEN, 0);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_CR);

    return 0;
}

/* this function config register for NS offload function
 * parameters:
 * 	index - 0~1, index to NS look up table. one entry of the lut is like this |remote|solicited|target0|target1|
 *	remote_addr - ipv6 addr where fuxi gets the NS solicitation pkt(request). in common, it is 0 to match any remote machine.
 *	solicited_addr - the solicited node multicast group address which fuxi computes and joins.
 *	target_addr1 - it is the target address in NS solicitation pkt.
 *	target_addr2 - second target address, any address (with last 6B same with target address?).
 */
static int fxgmac_set_ns_offload(struct fxgmac_pdata* pdata,unsigned int index, unsigned char* remote_addr,unsigned char* solicited_addr,unsigned char*target_addr1,unsigned char *target_addr2,unsigned char *mac_addr)
//static int fxgmac_set_ns_offload(struct nic_pdata* pdata,unsigned char index, PIPV6NSPARAMETERS PIPv6NSPara)

{
    u32 regval;
    u32 Address[4],mac_addr_hi,mac_addr_lo;
    u8  i, remote_not_zero = 0;

#if 1
    regval = readreg(pdata->pAdapter, pdata->base_mem + NS_TPID_PRO);
    regval = FXGMAC_SET_REG_BITS(regval, NS_TPID_PRO_STPID_POS,
        NS_TPID_PRO_STPID_LEN, 0X8100);
    regval = FXGMAC_SET_REG_BITS(regval, NS_TPID_PRO_CTPID_POS,
        NS_TPID_PRO_CTPID_LEN, 0X9100);
    writereg(pdata->pAdapter, regval, pdata->base_mem + NS_TPID_PRO);
    regval = readreg(pdata->pAdapter, pdata->base_mem + 0X38 * index + NS_LUT_MAC_ADDR_CTL );
    regval = FXGMAC_SET_REG_BITS(regval, NS_LUT_DST_CMP_TYPE_POS,
            NS_LUT_DST_CMP_TYPE_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, NS_LUT_DST_IGNORED_POS,
            NS_LUT_DST_IGNORED_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, NS_LUT_REMOTE_AWARED_POS,
            NS_LUT_REMOTE_AWARED_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, NS_LUT_TARGET_ISANY_POS,
            NS_LUT_TARGET_ISANY_LEN, 0);
    writereg(pdata->pAdapter, regval, pdata->base_mem + 0X38 * index + NS_LUT_MAC_ADDR_CTL);

    //AR
    for (i = 0; i < 16/4; i++)
    {
        Address[i] = (remote_addr[i * 4 + 0] << 24) | (remote_addr[i * 4 + 1] << 16) | (remote_addr[i * 4 + 2] << 8) | (remote_addr[i * 4 + 3] << 0);
        writereg(pdata->pAdapter, Address[i], pdata->base_mem + 0X38 * index + NS_LUT_ROMOTE0 + 4 * i);
        if(Address[i] )
        {
            remote_not_zero = 1;
        }
        Address[i] = (target_addr1[i * 4 + 0] << 24) | (target_addr1[i * 4 + 1] << 16) | (target_addr1[i * 4 + 2] << 8) | (target_addr1[i * 4 + 3] << 0);
        writereg(pdata->pAdapter, Address[i], pdata->base_mem + 0X38 * index + NS_LUT_TARGET0 + 4 * i);
        Address[i] = (solicited_addr[i * 4 + 0] << 24) | (solicited_addr[i * 4 + 1] << 16) | (solicited_addr[i * 4 + 2] << 8) | (solicited_addr[i * 4 + 3] << 0);
        writereg(pdata->pAdapter, Address[i], pdata->base_mem + 0X38 * index + NS_LUT_SOLICITED0 + 4 * i);
        Address[i] = (target_addr2[i * 4 + 0] << 24) | (target_addr2[i * 4 + 1] << 16) | (target_addr2[i * 4 + 2] << 8) | (target_addr2[i * 4 + 3] << 0);
        writereg(pdata->pAdapter, Address[i], pdata->base_mem + 0X10 * index + NS_LUT_TARGET4 + 4 * i);

    }
    mac_addr_hi = (mac_addr[0] <<  24) | (mac_addr[1] <<  16)|(mac_addr[2] << 8) | (mac_addr[3] << 0);
    mac_addr_lo = (mac_addr[4] <<  8) | (mac_addr[5] <<  0);

    writereg(pdata->pAdapter, mac_addr_hi, pdata->base_mem + 0X38 * index + NS_LUT_MAC_ADDR);
    if(remote_not_zero==0)
    {
        regval = readreg(pdata->pAdapter, pdata->base_mem + 0X38 * index + NS_LUT_MAC_ADDR_CTL );
        regval = FXGMAC_SET_REG_BITS(regval, NS_LUT_REMOTE_AWARED_POS,
            NS_LUT_REMOTE_AWARED_LEN, 0);
        regval = FXGMAC_SET_REG_BITS(regval, NS_LUT_MAC_ADDR_LOW_POS,
            NS_LUT_MAC_ADDR_LOW_LEN, mac_addr_lo);
        writereg(pdata->pAdapter, regval, pdata->base_mem + 0X38 * index + NS_LUT_MAC_ADDR_CTL);
    }
    else
    {
        regval = readreg(pdata->pAdapter, pdata->base_mem + 0X38 * index + NS_LUT_MAC_ADDR_CTL );
        regval = FXGMAC_SET_REG_BITS(regval, NS_LUT_REMOTE_AWARED_POS,
            NS_LUT_REMOTE_AWARED_LEN, 1);
        regval = FXGMAC_SET_REG_BITS(regval, NS_LUT_MAC_ADDR_LOW_POS,
            NS_LUT_MAC_ADDR_LOW_LEN, mac_addr_lo);
        writereg(pdata->pAdapter, regval, pdata->base_mem + 0X38 * index + NS_LUT_MAC_ADDR_CTL);
    }

#else
    regval = readreg(pdata->mac_regs -0x2000 + NS_TPID_PRO);
    regval = FXGMAC_SET_REG_BITS(regval, NS_TPID_PRO_STPID_POS,
        NS_TPID_PRO_STPID_LEN, 0X8100);
    regval = FXGMAC_SET_REG_BITS(regval, NS_TPID_PRO_CTPID_POS,
        NS_TPID_PRO_CTPID_LEN, 0X9100);
    writereg(regval, pdata->mac_regs - 0x2000 + NS_TPID_PRO);
    //AR
    writereg(0X20000000, pdata->mac_regs - 0x2000 + NS_LUT_ROMOTE0);
    writereg(0X00000000, pdata->mac_regs - 0x2000 + NS_LUT_ROMOTE1);
    writereg(0X00000000, pdata->mac_regs - 0x2000 + NS_LUT_ROMOTE2);
    writereg(0X00000001, pdata->mac_regs - 0x2000 + NS_LUT_ROMOTE3);
    writereg(0X20000000, pdata->mac_regs - 0x2000 + NS_LUT_TARGET0);
    writereg(0X00000000, pdata->mac_regs - 0x2000 + NS_LUT_TARGET1);
    writereg(0X00000000, pdata->mac_regs - 0x2000 + NS_LUT_TARGET2);
    writereg(0X00000002, pdata->mac_regs - 0x2000 + NS_LUT_TARGET3);
    writereg(0Xff020000, pdata->mac_regs - 0x2000 + NS_LUT_SOLICITED0);
    writereg(0X00000000, pdata->mac_regs - 0x2000 + NS_LUT_SOLICITED1);
    writereg(0X00000001, pdata->mac_regs - 0x2000 + NS_LUT_SOLICITED2);
    writereg(0Xff000002, pdata->mac_regs - 0x2000 + NS_LUT_SOLICITED3);
    writereg(0X00e0fc69, pdata->mac_regs - 0x2000 + NS_LUT_MAC_ADDR);
    writereg(0X00033381, pdata->mac_regs - 0x2000 + NS_LUT_MAC_ADDR_CTL);

    //NUD
    writereg(0X20000000, pdata->mac_regs - 0x2000 + 0X38 * 2 + NS_LUT_ROMOTE0);
    writereg(0X00000000, pdata->mac_regs - 0x2000 + 0X38 * 2 + NS_LUT_ROMOTE1);
    writereg(0X00000000, pdata->mac_regs - 0x2000 + 0X38 * 2 + NS_LUT_ROMOTE2);
    writereg(0X00000001, pdata->mac_regs - 0x2000 + 0X38 * 2 + NS_LUT_ROMOTE3);
    writereg(0X20000000, pdata->mac_regs - 0x2000 + 0X38 * 2 + NS_LUT_TARGET0);
    writereg(0X00000000, pdata->mac_regs - 0x2000 + 0X38 * 2 + NS_LUT_TARGET1);
    writereg(0X00000000, pdata->mac_regs - 0x2000 + 0X38 * 2 + NS_LUT_TARGET2);
    writereg(0X00000002, pdata->mac_regs - 0x2000 + 0X38 * 2 + NS_LUT_TARGET3);
    writereg(0X20000000, pdata->mac_regs - 0x2000 + 0X38 * 2 + NS_LUT_SOLICITED0);
    writereg(0X00000000, pdata->mac_regs - 0x2000 + 0X38 * 2 + NS_LUT_SOLICITED1);
    writereg(0X00000000, pdata->mac_regs - 0x2000 + 0X38 * 2 + NS_LUT_SOLICITED2);
    writereg(0X00000002, pdata->mac_regs - 0x2000 + 0X38 * 2 + NS_LUT_SOLICITED3);
    writereg(0X00e0fc69, pdata->mac_regs - 0x2000 + 0X38 * 2 + NS_LUT_MAC_ADDR);
    writereg(0X00033382, pdata->mac_regs - 0x2000 + 0X38 * 2 + NS_LUT_MAC_ADDR_CTL);

    //DAD
    writereg(0X00000000, pdata->mac_regs - 0x2000 + 0X38 * 3 + NS_LUT_ROMOTE0);
    writereg(0X00000000, pdata->mac_regs - 0x2000 + 0X38 * 3 + NS_LUT_ROMOTE1);
    writereg(0X00000000, pdata->mac_regs - 0x2000 + 0X38 * 3 + NS_LUT_ROMOTE2);
    writereg(0X00000000, pdata->mac_regs - 0x2000 + 0X38 * 3 + NS_LUT_ROMOTE3);
    writereg(0X20000000, pdata->mac_regs - 0x2000 + 0X38 * 3 + NS_LUT_TARGET0);
    writereg(0X00000000, pdata->mac_regs - 0x2000 + 0X38 * 3 + NS_LUT_TARGET1);
    writereg(0X00000000, pdata->mac_regs - 0x2000 + 0X38 * 3 + NS_LUT_TARGET2);
    writereg(0X00000002, pdata->mac_regs - 0x2000 + 0X38 * 3 + NS_LUT_TARGET3);
    writereg(0Xff020000, pdata->mac_regs - 0x2000 + 0X38 * 3 + NS_LUT_SOLICITED0);
    writereg(0X00000000, pdata->mac_regs - 0x2000 + 0X38 * 3 + NS_LUT_SOLICITED1);
    writereg(0X00000001, pdata->mac_regs - 0x2000 + 0X38 * 3 + NS_LUT_SOLICITED2);
    writereg(0Xff000002, pdata->mac_regs - 0x2000 + 0X38 * 3 + NS_LUT_SOLICITED3);
    writereg(0X00e0fc69, pdata->mac_regs - 0x2000 + 0X38 * 3 + NS_LUT_MAC_ADDR);
    writereg(0X00033381, pdata->mac_regs - 0x2000 + 0X38 * 3 + NS_LUT_MAC_ADDR_CTL);

    writereg(0X00000001, pdata->mac_regs - 0x2000 + NS_OF_GLB_CTL);
#endif
    return 0;
}

#ifdef LINUX
void fxgmac_update_ns_offload_ipv6addr(struct fxgmac_pdata *pdata, unsigned int param)
{
    struct net_device *netdev = pdata->netdev;
    unsigned char addr_buf[5][16];

    unsigned char * remote_addr = (unsigned char *)&addr_buf[0][0];
    unsigned char * solicited_addr = (unsigned char *)&addr_buf[1][0];
    unsigned char * target_addr1 = (unsigned char *)&addr_buf[2][0];
    //unsigned char * target_addr2 = (unsigned char *)&addr_buf[3][0];
    unsigned char * mac_addr = (unsigned char *)&addr_buf[4][0];

    /* get ipv6 addr from net device */
    if (NULL == fxgmac_get_netdev_ip6addr(pdata, target_addr1, solicited_addr, (FXGMAC_NS_IFA_LOCAL_LINK | FXGMAC_NS_IFA_GLOBAL_UNICAST) & param))
        {
        DPRINTK("%s, get net device ipv6 addr with err and ignore NS offload.\n", __FUNCTION__);

        return;
    }

    DPRINTK("%s, Get net device binary IPv6 ok, local-link=%pI6\n", __FUNCTION__, target_addr1);
    DPRINTK("%s, Get net device binary IPv6 ok, solicited =%pI6\n", __FUNCTION__, solicited_addr);

    memcpy(mac_addr, netdev->dev_addr, netdev->addr_len);
    DPRINTK("%s, Get net device MAC addr ok, ns_tab idx=%d, %02x:%02x:%02x:%02x:%02x:%02x\n",
        __FUNCTION__, pdata->expansion.ns_offload_tab_idx, mac_addr[0], mac_addr[1],
        mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

    memset(remote_addr, 0, 16);
    fxgmac_set_ns_offload(pdata, pdata->expansion.ns_offload_tab_idx++,
                            remote_addr, solicited_addr, target_addr1,
                            target_addr1, mac_addr);
    if(pdata->expansion.ns_offload_tab_idx >= 2) pdata->expansion.ns_offload_tab_idx = 0;

}
#endif

static int fxgmac_enable_ns_offload(struct fxgmac_pdata* pdata)
{
    writereg(pdata->pAdapter, 0X00000011, pdata->base_mem + NS_OF_GLB_CTL);
    return 0;
}


static int fxgmac_disable_ns_offload(struct fxgmac_pdata* pdata)
{
    writereg(pdata->pAdapter, 0X00000000, pdata->base_mem + NS_OF_GLB_CTL);
    return 0;
}

static int fxgmac_check_wake_pattern_fifo_pointer(struct fxgmac_pdata* pdata)
{
    u32 regval;
    int ret = 0;

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_PMT_STA);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_PMT_STA_RWKFILTERST_POS, MAC_PMT_STA_RWKFILTERST_LEN,1);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_PMT_STA);

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_PMT_STA);
    regval = FXGMAC_GET_REG_BITS(regval, MAC_PMT_STA_RWKPTR_POS, MAC_PMT_STA_RWKPTR_LEN);
    if (regval != 0) {
        DPRINTK("Remote fifo pointer  is not 0\n");
        ret = -EINVAL;
    }
    return ret;
}

static int fxgmac_set_wake_pattern_mask(struct fxgmac_pdata* pdata, u32 filter_index, u8 register_index,u32 Data)
{
    const u16 address_offset[16][3] = {
        {0x1020, 0x1024, 0x1028},
        {0x102c, 0x1030, 0x1034},
        {0x1038, 0x103c, 0x1040},
        {0x1044, 0x1050, 0x1054},
        {0x1058, 0x105c, 0x1060},
        {0x1064, 0x1068, 0x106c},
        {0x1070, 0x1074, 0x1078},
        {0x107c, 0x1080, 0x1084},
        {0x1088, 0x108c, 0x1090},
        {0x1134, 0x113c, 0x1140},
        {0x1208, 0x1200, 0x1204},
        {0x1218, 0x1210, 0x1214},
        {0x1228, 0x1220, 0x1224},
        {0x1238, 0x1230, 0x1234},
        {0x1248, 0x1240, 0x1244},
        {0x1258, 0x1250, 0x1254},
     };
     if (filter_index > 15||register_index > 2) {
         DbgPrintF(MP_TRACE, "%s - Remote mask  pointer  is  over range, filter_index:%d, register_index:0x%x\n",
            __FUNCTION__, filter_index, register_index);
        return -1;
    }
    writereg(pdata->pAdapter, Data, pdata->base_mem + address_offset[filter_index][register_index]);
    return 0;
}

static u16 wol_crc16(u8* pucframe, u16 uslen)
{
    int i;

    union type16 {
        u16 raw;
        struct {
            u16 bit_0 : 1;
            u16 bit_1 : 1;
            u16 bit_2 : 1;
            u16 bit_3 : 1;
            u16 bit_4 : 1;
            u16 bit_5 : 1;
            u16 bit_6 : 1;
            u16 bit_7 : 1;
            u16 bit_8 : 1;
            u16 bit_9 : 1;
            u16 bit_10 : 1;
            u16 bit_11 : 1;
            u16 bit_12 : 1;
            u16 bit_13 : 1;
            u16 bit_14 : 1;
            u16 bit_15 : 1;
        }bits;
    };

    union type8	{
        u16 raw;

        struct {
            u16 bit_0 : 1;
            u16 bit_1 : 1;
            u16 bit_2 : 1;
            u16 bit_3 : 1;
            u16 bit_4 : 1;
            u16 bit_5 : 1;
            u16 bit_6 : 1;
            u16 bit_7 : 1;
        }bits;
    };

    union type16 crc, crc_comb;
    union type8  next_crc, rrpe_data;
    next_crc.raw = 0;
    crc.raw = 0xffff;
    for (i = 0; i < uslen;i++) {
        rrpe_data.raw = pucframe[i];
        next_crc.bits.bit_0 = crc.bits.bit_15 ^ rrpe_data.bits.bit_0;
        next_crc.bits.bit_1 = crc.bits.bit_14 ^ next_crc.bits.bit_0 ^ rrpe_data.bits.bit_1;
        next_crc.bits.bit_2 = crc.bits.bit_13 ^ next_crc.bits.bit_1 ^ rrpe_data.bits.bit_2;
        next_crc.bits.bit_3 = crc.bits.bit_12 ^ next_crc.bits.bit_2 ^ rrpe_data.bits.bit_3;
        next_crc.bits.bit_4 = crc.bits.bit_11 ^ next_crc.bits.bit_3 ^ rrpe_data.bits.bit_4;
        next_crc.bits.bit_5 = crc.bits.bit_10 ^ next_crc.bits.bit_4 ^ rrpe_data.bits.bit_5;
        next_crc.bits.bit_6 = crc.bits.bit_9 ^ next_crc.bits.bit_5 ^ rrpe_data.bits.bit_6;
        next_crc.bits.bit_7 = crc.bits.bit_8 ^ next_crc.bits.bit_6 ^ rrpe_data.bits.bit_7;

        crc_comb.bits.bit_15 = crc.bits.bit_7 ^ next_crc.bits.bit_7;
        crc_comb.bits.bit_14 = crc.bits.bit_6;
        crc_comb.bits.bit_13 = crc.bits.bit_5;
        crc_comb.bits.bit_12 = crc.bits.bit_4;
        crc_comb.bits.bit_11 = crc.bits.bit_3;
        crc_comb.bits.bit_10 = crc.bits.bit_2;
        crc_comb.bits.bit_9 = crc.bits.bit_1 ^ next_crc.bits.bit_0;
        crc_comb.bits.bit_8 = crc.bits.bit_0 ^ next_crc.bits.bit_1;
        crc_comb.bits.bit_7 = next_crc.bits.bit_0 ^ next_crc.bits.bit_2;
        crc_comb.bits.bit_6 = next_crc.bits.bit_1 ^ next_crc.bits.bit_3;
        crc_comb.bits.bit_5 = next_crc.bits.bit_2 ^ next_crc.bits.bit_4;
        crc_comb.bits.bit_4 = next_crc.bits.bit_3 ^ next_crc.bits.bit_5;
        crc_comb.bits.bit_3 = next_crc.bits.bit_4 ^ next_crc.bits.bit_6;
        crc_comb.bits.bit_2 = next_crc.bits.bit_5 ^ next_crc.bits.bit_7;
        crc_comb.bits.bit_1 = next_crc.bits.bit_6;
        crc_comb.bits.bit_0 = next_crc.bits.bit_7;
        crc.raw = crc_comb.raw;
    }
    return crc.raw;
}

static int fxgmac_set_wake_pattern(
    struct fxgmac_pdata* pdata,
    struct wol_bitmap_pattern* wol_pattern,
    u32 pattern_cnt)
{
    u32                         i, j, kp, km, mask_index;
    int                         z;
    u16                         map_index;
    u8                          mask[MAX_PATTERN_SIZE];
    u32                         regval = 0;
    u32                         total_cnt = 0, pattern_inherited_cnt = 0;
    u8* ptdata, * ptmask;

    if (pattern_cnt > MAX_PATTERN_COUNT) {
        DbgPrintF(MP_TRACE, "%s - Error: %d patterns, exceed %d, not supported!\n",
            __FUNCTION__, pattern_cnt, MAX_PATTERN_COUNT);
        return -1;
    }

    /* Reset the FIFO head pointer. */
    if (fxgmac_check_wake_pattern_fifo_pointer(pdata)) {
        DbgPrintF(MP_TRACE, "%s - Warning: the remote pattern array pointer is not be 0\n", __FUNCTION__);
        return -1;
    }

    for (i = 0; i < pattern_cnt; i++) {
        memcpy(&pdata->pattern[i], wol_pattern + i, sizeof(wol_pattern[0]));
        if (pattern_cnt + pattern_inherited_cnt < MAX_PATTERN_COUNT)
        {
            if (wol_pattern[i].pattern_offset || !(wol_pattern[i].mask_info[0] & 0x01)) {
                memcpy(&pdata->pattern[pattern_cnt + pattern_inherited_cnt],
                    wol_pattern + i,
                    sizeof(wol_pattern[0]));
                pattern_inherited_cnt++;
            }
        }
    }
    total_cnt = pattern_cnt + pattern_inherited_cnt;

    /*
    * calculate  the crc-16 of the mask pattern
    * print the pattern and mask for debug purpose.
    */
    for (i = 0; i < total_cnt; i++) {
        /* Please program pattern[i] to NIC for pattern match wakeup.
        * pattern_size, pattern_info, mask_info
        */
        //save the mask pattern
        mask_index = 0;
        map_index = 0;
        for (j = 0; j < pdata->pattern[i].mask_size; j++) {
            for (z = 0; z < ((j == (MAX_PATTERN_SIZE / 8 - 1)) ? 7 : 8); z++) {
                if (pdata->pattern[i].mask_info[j] & (0x01 << z)) {
                    mask[map_index] = pdata->pattern[i].pattern_info[pdata->pattern[i].pattern_offset + mask_index];
                    map_index++;
                }
                mask_index++;
            }
        }
        //calculate  the crc-16 of the mask pattern
        pdata->pattern[i].pattern_crc = wol_crc16(mask, map_index);

        // Print pattern match, for debug purpose.
        DbgPrintF(MP_LOUD, "%s - Pattern[%d]:", __FUNCTION__, i);
        for (kp = 0, km = 0; kp < sizeof(pdata->pattern[i].pattern_info); kp += 16, km += 2) {
            ptdata = &pdata->pattern[i].pattern_info[kp];
            ptmask = &pdata->pattern[i].mask_info[km];
            DBGPRINT(MP_LOUD, ("\n    %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x  %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x  Mask %02x-%02x",
                ptdata[0], ptdata[1], ptdata[2], ptdata[3], ptdata[4], ptdata[5], ptdata[6], ptdata[7],
                ptdata[8], ptdata[9], ptdata[10], ptdata[11], ptdata[12], ptdata[13], ptdata[14], ptdata[15],
                ptmask[0], ptmask[1]));
        }
        //fxgmac_dump_buffer(mask, map_index, 2);
        DbgPrintF(MP_LOUD, "WritePatternToNic62 the %d patterns crc = %x mask length = %d, mask_offset=%x.\n",
            i, pdata->pattern[i].pattern_crc, map_index,
            pdata->pattern[i].pattern_offset);
        memset(mask, 0, sizeof(mask));
    }

    // Write patterns by FIFO block.
    for (i = 0; i < (total_cnt + 3) / 4; i++) {
        // 1. Write the first 4Bytes of Filter.
        writereg(pdata->pAdapter,
            ((pdata->pattern[i * 4 + 0].mask_info[3] & 0x7f) << 24) |
            (pdata->pattern[i * 4 + 0].mask_info[2] << 16) |
            (pdata->pattern[i * 4 + 0].mask_info[1] << 8) |
            (pdata->pattern[i * 4 + 0].mask_info[0] << 0),
            pdata->mac_regs + MAC_RWK_PAC);

        writereg(pdata->pAdapter,
            ((pdata->pattern[i * 4 + 1].mask_info[3] & 0x7f) << 24) |
            (pdata->pattern[i * 4 + 1].mask_info[2] << 16) |
            (pdata->pattern[i * 4 + 1].mask_info[1] << 8) |
            (pdata->pattern[i * 4 + 1].mask_info[0] << 0),
            pdata->mac_regs + MAC_RWK_PAC);

        writereg(pdata->pAdapter,
            ((pdata->pattern[i * 4 + 2].mask_info[3] & 0x7f) << 24) |
            (pdata->pattern[i * 4 + 2].mask_info[2] << 16) |
            (pdata->pattern[i * 4 + 2].mask_info[1] << 8) |
            (pdata->pattern[i * 4 + 2].mask_info[0] << 0),
            pdata->mac_regs + MAC_RWK_PAC);

        writereg(pdata->pAdapter,
            ((pdata->pattern[i * 4 + 3].mask_info[3] & 0x7f) << 24) |
            (pdata->pattern[i * 4 + 3].mask_info[2] << 16) |
            (pdata->pattern[i * 4 + 3].mask_info[1] << 8) |
            (pdata->pattern[i * 4 + 3].mask_info[0] << 0),
            pdata->mac_regs + MAC_RWK_PAC);

        // 2. Write the Filter Command.
        regval = 0;
        // Set filter enable bit.
        regval |= ((i * 4 + 0) < total_cnt) ? (0x1 << 0) : 0x0;
        regval |= ((i * 4 + 1) < total_cnt) ? (0x1 << 8) : 0x0;
        regval |= ((i * 4 + 2) < total_cnt) ? (0x1 << 16) : 0x0;
        regval |= ((i * 4 + 3) < total_cnt) ? (0x1 << 24) : 0x0;
        // Set filter address type, 0- unicast, 1 - multicast.
        regval |= (i * 4 + 0 >= total_cnt) ? 0x0 :
            (i * 4 + 0 >= pattern_cnt) ? (0x1 << (3 + 0)) :
            pdata->pattern[i * 4 + 0].pattern_offset ? 0x0 :
            !(pdata->pattern[i * 4 + 0].mask_info[0] & 0x01) ? 0x0 :
            (pdata->pattern[i * 4 + 0].pattern_info[0] & 0x01) ? (0x1 << (3 + 0)) : 0x0;
        regval |= (i * 4 + 1 >= total_cnt) ? 0x0 :
            (i * 4 + 1 >= pattern_cnt) ? (0x1 << (3 + 8)) :
            pdata->pattern[i * 4 + 1].pattern_offset ? 0x0 :
            !(pdata->pattern[i * 4 + 1].mask_info[0] & 0x01) ? 0x0 :
            (pdata->pattern[i * 4 + 1].pattern_info[0] & 0x01) ? (0x1 << (3 + 8)) : 0x0;
        regval |= (i * 4 + 2 >= total_cnt) ? 0x0 :
            (i * 4 + 2 >= pattern_cnt) ? (0x1 << (3 + 16)) :
            pdata->pattern[i * 4 + 2].pattern_offset ? 0x0 :
            !(pdata->pattern[i * 4 + 2].mask_info[0] & 0x01) ? 0x0 :
            (pdata->pattern[i * 4 + 2].pattern_info[0] & 0x01) ? (0x1 << (3 + 16)) : 0x0;
        regval |= (i * 4 + 3 >= total_cnt) ? 0x0 :
            (i * 4 + 3 >= pattern_cnt) ? (0x1 << (3 + 24)) :
            pdata->pattern[i * 4 + 3].pattern_offset ? 0x0 :
            !(pdata->pattern[i * 4 + 3].mask_info[0] & 0x01) ? 0x0 :
            (pdata->pattern[i * 4 + 3].pattern_info[0] & 0x01) ? (0x1 << (3 + 24)) : 0x0;
        writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_RWK_PAC);
        //DbgPrintF(MP_LOUD, "FilterCMD 0x%08x", regval);

        // 3. Write the mask offset.
        writereg(pdata->pAdapter,
            (pdata->pattern[i * 4 + 3].pattern_offset << 24) |
            (pdata->pattern[i * 4 + 2].pattern_offset << 16) |
            (pdata->pattern[i * 4 + 1].pattern_offset << 8) |
            (pdata->pattern[i * 4 + 0].pattern_offset << 0),
            pdata->mac_regs + MAC_RWK_PAC);

        // 4. Write the masked data CRC.
        writereg(pdata->pAdapter,
            (pdata->pattern[i * 4 + 1].pattern_crc << 16) |
            (pdata->pattern[i * 4 + 0].pattern_crc << 0),
            pdata->mac_regs + MAC_RWK_PAC);
        writereg(pdata->pAdapter,
            (pdata->pattern[i * 4 + 3].pattern_crc << 16) |
            (pdata->pattern[i * 4 + 2].pattern_crc << 0),
            pdata->mac_regs + MAC_RWK_PAC);
    }

    for (i = 0; i < total_cnt; i++) {
        fxgmac_set_wake_pattern_mask(pdata, i, 0,
            ((pdata->pattern[i].mask_info[7] & 0x7f) << (24 + 1)) |
            (pdata->pattern[i].mask_info[6] << (16 + 1)) |
            (pdata->pattern[i].mask_info[5] << (8 + 1)) |
            (pdata->pattern[i].mask_info[4] << (0 + 1)) |
            ((pdata->pattern[i].mask_info[3] & 0x80) >> 7));//global  manager  regitster  mask bit  31~62
        fxgmac_set_wake_pattern_mask(pdata, i, 1,
            ((pdata->pattern[i].mask_info[11] & 0x7f) << (24 + 1)) |
            (pdata->pattern[i].mask_info[10] << (16 + 1)) |
            (pdata->pattern[i].mask_info[9] << (8 + 1)) |
            (pdata->pattern[i].mask_info[8] << (0 + 1)) |
            ((pdata->pattern[i].mask_info[7] & 0x80) >> 7));//global  manager  regitster  mask bit  63~94
        fxgmac_set_wake_pattern_mask(pdata, i, 2, \
            ((pdata->pattern[i].mask_info[15] & 0x7f) << (24 + 1)) |
            (pdata->pattern[i].mask_info[14] << (16 + 1)) |
            (pdata->pattern[i].mask_info[13] << (8 + 1)) |
            (pdata->pattern[i].mask_info[12] << (0 + 1)) |
            ((pdata->pattern[i].mask_info[11] & 0x80) >> 7));//global  manager  regitster  mask bit  95~126
    }

    return 0;
}

static int fxgmac_enable_wake_pattern(struct fxgmac_pdata* pdata)
{
    u32 regval;
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_PMT_STA);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_PMT_STA_RWKFILTERST_POS, MAC_PMT_STA_RWKFILTERST_LEN,1);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_PMT_STA_RWKPKTEN_POS, MAC_PMT_STA_RWKPKTEN_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_PMT_STA);
    regval = readreg(pdata->pAdapter, pdata->base_mem + WOL_CTL);
    regval = FXGMAC_SET_REG_BITS(regval, WOL_PKT_EN_POS, WOL_PKT_EN_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->base_mem + WOL_CTL);
    return 0;
}

static int fxgmac_disable_wake_pattern(struct fxgmac_pdata* pdata)
{
    u32 regval;

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_PMT_STA);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_PMT_STA_RWKFILTERST_POS, MAC_PMT_STA_RWKFILTERST_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_PMT_STA_RWKPKTEN_POS, MAC_PMT_STA_RWKPKTEN_LEN, 0);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_PMT_STA);
    regval = readreg(pdata->pAdapter, pdata->base_mem + WOL_CTL);
    regval = FXGMAC_SET_REG_BITS(regval, WOL_PKT_EN_POS, WOL_PKT_EN_LEN, 0);
    writereg(pdata->pAdapter, regval, pdata->base_mem + WOL_CTL);
    return 0;
}
static int fxgmac_enable_wake_magic_pattern(struct fxgmac_pdata* pdata)
{
    u32 regval;

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_PMT_STA);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_PMT_STA_MGKPKTEN_POS, MAC_PMT_STA_MGKPKTEN_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_PMT_STA);
    regval = readreg(pdata->pAdapter, pdata->base_mem + WOL_CTL);
    regval = FXGMAC_SET_REG_BITS(regval, WOL_PKT_EN_POS, WOL_PKT_EN_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->base_mem + WOL_CTL);

    /* Enable PME Enable Bit. */
    cfg_r32(pdata, REG_PM_STATCTRL, &regval);
    regval = FXGMAC_SET_REG_BITS(regval, PM_CTRLSTAT_PME_EN_POS, PM_CTRLSTAT_PME_EN_LEN, 1);
    cfg_w32(pdata, REG_PM_STATCTRL, regval);

    return 0;
}

static int fxgmac_disable_wake_magic_pattern(struct fxgmac_pdata* pdata)
{
    u32 regval;
    regval = readreg(pdata->pAdapter, pdata->base_mem + WOL_CTL);
    regval = FXGMAC_SET_REG_BITS(regval, WOL_PKT_EN_POS, WOL_PKT_EN_LEN, 0);
    writereg(pdata->pAdapter, regval, pdata->base_mem + WOL_CTL);
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_PMT_STA);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_PMT_STA_MGKPKTEN_POS, MAC_PMT_STA_MGKPKTEN_LEN, 0);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_PMT_STA);
    return 0;
}

#if defined(FUXI_PM_WPI_READ_FEATURE_EN) && FUXI_PM_WPI_READ_FEATURE_EN
/*
 * enable Wake packet indication. called to enable before sleep/hibernation
 * and no needed to call disable for that, fxgmac_get_wake_packet_indication will clear to normal once done.
 */
static void fxgmac_enable_wake_packet_indication(struct fxgmac_pdata* pdata, int en)
{
    u32 val_wpi_crtl0;

    /* read-clear WoL event. */
    readreg(pdata->pAdapter, pdata->base_mem + MGMT_WOL_CTRL);

    /* get wake packet information */
    val_wpi_crtl0 = (u32)readreg(pdata->pAdapter, pdata->base_mem + MGMT_WPI_CTRL0);

    /* prepare to write packet data by write wpi_mode to 1 */
    val_wpi_crtl0 = FXGMAC_SET_REG_BITS(val_wpi_crtl0,
        MGMT_WPI_CTRL0_WPI_MODE_POS,
        MGMT_WPI_CTRL0_WPI_MODE_LEN, (en ? MGMT_WPI_CTRL0_WPI_MODE_WR : MGMT_WPI_CTRL0_WPI_MODE_NORMAL));
    writereg(pdata->pAdapter, val_wpi_crtl0, pdata->base_mem + MGMT_WPI_CTRL0);

    DbgPrintF(MP_TRACE, "%s - WPI pkt enable=%d, reg=%08x.\n", __FUNCTION__, en, val_wpi_crtl0);

    return;
}

/*
 * this function read Wake up packet after MDIS resume
 * input:
 * 		pdata
 *		wpi_buf		container of a packet.
 *		buf_size	size of the packet container. since HW limit to 14bits, ie 16KB all together.
 * output:
 * 		wake_reason	from HW, we can indentify 1)magic packet, or 2)pattern(remote wake packet) or WAKE_REASON_HW_ERR indicates err
 *		packet_size length of the wake packet. 0 indicates exception.
 *
 */
static void fxgmac_get_wake_packet_indication(struct fxgmac_pdata* pdata, int* wake_reason, u32* wake_pattern_number, u8* wpi_buf, u32 buf_size, u32* packet_size)
{
    u32 i, regval, val_wpi_crtl0, *dw_wpi_buf;
    u32 data_len, data_len_dw, b_need_pkt = 0;

    *wake_reason = WAKE_REASON_NONE;
    *packet_size = 0;
    fxgmac_release_phy(pdata);
#if 1
    /* try to check wake reason. GMAC reg 20c0 only tells Magic or remote-pattern
    * read from MGMT_WOL_CTRL, 1530 instead.
    */
    regval = (u32)readreg(pdata->pAdapter, pdata->base_mem + MGMT_WOL_CTRL);
    DbgPrintF(MP_TRACE, "%s - 0x1530=%x.\n", __FUNCTION__, regval);
    if (!regval) {
        DbgPrintF(MP_TRACE, "%s - nothing for WPI pkt.\n", __FUNCTION__);
        return;
    }

    if (regval & MGMT_WOL_CTRL_WPI_MGC_PKT) {
        *wake_reason = WAKE_REASON_MAGIC;
        b_need_pkt = 1;
    } else if (regval & MGMT_WOL_CTRL_WPI_RWK_PKT) {
        *wake_reason = WAKE_REASON_PATTERNMATCH;
        b_need_pkt = 1;
        *wake_pattern_number = 0;

        /*
         * wake_pattern_number, HW should tell,,tbd
         */
        for (i = 0;i < MAX_PATTERN_COUNT;i++) {
            if (regval & (MGMT_WOL_CTRL_WPI_RWK_PKT_NUMBER << i)) {
                *wake_pattern_number = i;
                break;
            }
        }
        //*wake_pattern_number = regval&MGMT_WOL_CTRL_WPI_RWK_PKT_NUMBER;

    } else if (regval & MGMT_WOL_CTRL_WPI_LINK_CHG) {
        *wake_reason = WAKE_REASON_LINK;
    }

#else
    /* for 20c0 */
    regval = (u32)readreg(pdata->mac_regs + MAC_PMT_STA);
    DbgPrintF(MP_TRACE, "%s - 0x20c0=%x.\n", __FUNCTION__, regval);
    if (FXGMAC_GET_REG_BITS(regval,
        MAC_PMT_STA_MGKPRCVD_POS,
        MAC_PMT_STA_MGKPRCVD_LEN)) {
        *wake_reason = WAKE_REASON_MAGIC;
        b_need_pkt = 1;
        DbgPrintF(MP_TRACE, "%s - waken up by Magic.\n", __FUNCTION__);
    } else if (FXGMAC_GET_REG_BITS(regval,
        MAC_PMT_STA_RWKPRCVD_POS,
        MAC_PMT_STA_RWKPRCVD_LEN)) {
        *wake_reason = WAKE_REASON_PATTERNMATCH;
        b_need_pkt = 1;
        DbgPrintF(MP_TRACE, "%s - waken up by Pattern.\n", __FUNCTION__);
    }
#endif

    if (!b_need_pkt) {
        DbgPrintF(MP_TRACE, "%s - wake by link and no WPI pkt.\n", __FUNCTION__);
        return;
    }

    /* get wake packet information */
    val_wpi_crtl0 = (u32)readreg(pdata->pAdapter, pdata->base_mem + MGMT_WPI_CTRL0);

    if (val_wpi_crtl0 & MGMT_WPI_CTRL0_WPI_FAIL) {
        *wake_reason = WAKE_REASON_HW_ERR;
        DbgPrintF(MP_TRACE, "%s - WPI pkt fail from hw.\n", __FUNCTION__);
        return;
    }

    *packet_size = FXGMAC_GET_REG_BITS(val_wpi_crtl0,
        MGMT_WPI_CTRL0_WPI_PKT_LEN_POS,
        MGMT_WPI_CTRL0_WPI_PKT_LEN_LEN);

    if (0 == *packet_size) {
        *wake_reason = WAKE_REASON_HW_ERR;
        DbgPrintF(MP_TRACE, "%s - WPI pkt len is 0 from hw.\n", __FUNCTION__);
        return;
    }


    DbgPrintF(MP_TRACE, "%s - WPI pkt len from hw, *packet_size=%u.\n", __FUNCTION__, *packet_size);

    if (buf_size < *packet_size) {
        DbgPrintF(MP_WARN, "%s - too small buf_size=%u, WPI pkt len is %u.\n", __FUNCTION__, buf_size, *packet_size);
        data_len = buf_size;
    } else {
        data_len = *packet_size;
    }

    /* prepare to read packet data by write wpi_mode to 2 */
    val_wpi_crtl0 = FXGMAC_SET_REG_BITS(val_wpi_crtl0,
        MGMT_WPI_CTRL0_WPI_MODE_POS,
        MGMT_WPI_CTRL0_WPI_MODE_LEN, MGMT_WPI_CTRL0_WPI_MODE_RD);
    writereg(pdata->pAdapter, val_wpi_crtl0, pdata->base_mem + MGMT_WPI_CTRL0);

    dw_wpi_buf = (u32*)wpi_buf;
    data_len_dw = (data_len + 3) / 4;

    i = 0;
    DbgPrintF(MP_TRACE, "%s - before retrieve, len=%d, len_dw=%d, reg_wpi_ctrl0=%08x.\n",
        __FUNCTION__, data_len, data_len_dw, val_wpi_crtl0);
    while ((0 == (val_wpi_crtl0 & MGMT_WPI_CTRL0_WPI_OP_DONE)))	{
        if (i < data_len_dw) {
            regval = (u32)readreg(pdata->pAdapter, pdata->base_mem + MGMT_WPI_CTRL1_DATA);
            /*dw_wpi_buf[i] = SWAP_BYTES_32(regval);*/
            dw_wpi_buf[i] = regval;

            //DbgPrintF(MP_TRACE, "%s - read data, reg=%x, data[%d]=%08x.\n", __FUNCTION__, MGMT_WPI_CTRL1_DATA, i, dw_wpi_buf[i]);
        } else {
            break;
        }

        val_wpi_crtl0 = (u32)readreg(pdata->pAdapter, pdata->base_mem + MGMT_WPI_CTRL0);
        i++;
    }
    if (*packet_size <= MAC_CRC_LENGTH)
    {
        DbgPrintF(MP_TRACE, "%s - Warning, WPI pkt len is less 4 from hw.\n", __FUNCTION__);
        return;
    }
    *packet_size -= MAC_CRC_LENGTH;

    /* once read data complete and write wpi_mode to 0, normal */
    val_wpi_crtl0 = FXGMAC_SET_REG_BITS(val_wpi_crtl0,
        MGMT_WPI_CTRL0_WPI_MODE_POS,
        MGMT_WPI_CTRL0_WPI_MODE_LEN, MGMT_WPI_CTRL0_WPI_MODE_NORMAL);
    writereg(pdata->pAdapter, val_wpi_crtl0, pdata->base_mem + MGMT_WPI_CTRL0);

    DbgPrintF(MP_TRACE, "%s - WPI done and back to normal mode, reg=%08x, read data=%dB.\n", __FUNCTION__, val_wpi_crtl0, i * 4);

    return;
}
#endif /* FUXI_PM_WPI_READ_FEATURE_EN */

static int fxgmac_enable_wake_link_change(struct fxgmac_pdata* pdata)
{
    u32 regval;

    regval = readreg(pdata->pAdapter, pdata->base_mem + WOL_CTL);
    regval = FXGMAC_SET_REG_BITS(regval, WOL_LINKCHG_EN_POS, WOL_LINKCHG_EN_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->base_mem + WOL_CTL);
    return 0;
}
static int fxgmac_disable_wake_link_change(struct fxgmac_pdata* pdata)
{
    u32  regval;

    regval = readreg(pdata->pAdapter, pdata->base_mem + WOL_CTL);
    regval = FXGMAC_SET_REG_BITS(regval, WOL_LINKCHG_EN_POS, WOL_LINKCHG_EN_LEN, 0);
    writereg(pdata->pAdapter, regval, pdata->base_mem + WOL_CTL);
    return 0;
}
#endif // LINUX/_WIN64/_WIN32

#ifdef LINUX
static void fxgmac_config_wol(struct fxgmac_pdata *pdata, int en)
{
    /* enable or disable WOL. this function only set wake-up type, and power related configure
     * will be in other place, see power management.
     */
    if (!pdata->hw_feat.rwk) {
        netdev_err(pdata->netdev, "error configuring WOL - not supported.\n");
        return;
    }

    fxgmac_disable_wake_magic_pattern(pdata);
    fxgmac_disable_wake_pattern(pdata);
    fxgmac_disable_wake_link_change(pdata);

    if(en) {
        /* config mac address for rx of magic or ucast */
        fxgmac_set_mac_address(pdata, (u8*)(pdata->netdev->dev_addr));

        /* Enable Magic packet */
        if (pdata->expansion.wol & WAKE_MAGIC) {
            fxgmac_enable_wake_magic_pattern(pdata);
        }

        /* Enable global unicast packet */
        if (pdata->expansion.wol & WAKE_UCAST
            || pdata->expansion.wol & WAKE_MCAST
            || pdata->expansion.wol & WAKE_BCAST
            || pdata->expansion.wol & WAKE_ARP) {
            fxgmac_enable_wake_pattern(pdata);
        }

        /* Enable ephy link change */
        if ((FXGMAC_WOL_UPON_EPHY_LINK) && (pdata->expansion.wol & WAKE_PHY)) {
            fxgmac_enable_wake_link_change(pdata);
        }
    }
    device_set_wakeup_enable(/*pci_dev_to_dev*/(pdata->dev), en);

    DPRINTK("config_wol callout\n");
}
#endif

static int  fxgmac_get_ephy_state(struct fxgmac_pdata* pdata)
{
    u32 value;
    value = readreg(pdata->pAdapter, pdata->base_mem + MGMT_EPHY_CTRL);
    return value;
}

static void fxgmac_enable_dma_interrupts(struct fxgmac_pdata *pdata)
{
#ifndef DPDK
    unsigned int dma_ch_isr, dma_ch_ier;
    struct fxgmac_channel *channel;
    unsigned int i;

#ifdef NIC_NET_ADAPETERCX
    u32 regval;
    //config interrupt to level signal
    regval = (u32)readreg(pdata->pAdapter, pdata->mac_regs + DMA_MR);
    regval = FXGMAC_SET_REG_BITS(regval, DMA_MR_INTM_POS, DMA_MR_INTM_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, DMA_MR_QUREAD_POS, DMA_MR_QUREAD_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + DMA_MR);
#endif

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        /* Clear all the interrupts which are set */
        dma_ch_isr = readreg(pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_SR));
        writereg(pdata->pAdapter, dma_ch_isr, FXGMAC_DMA_REG(channel, DMA_CH_SR));

		/* Clear all interrupt enable bits */
        dma_ch_ier = 0;

        /* Enable following interrupts
         *   NIE  - Normal Interrupt Summary Enable
         *   AIE  - Abnormal Interrupt Summary Enable
         *   FBEE - Fatal Bus Error Enable
         */
        dma_ch_ier = FXGMAC_SET_REG_BITS(dma_ch_ier,
                         DMA_CH_IER_NIE_POS,
                    DMA_CH_IER_NIE_LEN, 1);
        /*
        dma_ch_ier = FXGMAC_SET_REG_BITS(dma_ch_ier,
                         DMA_CH_IER_AIE_POS,
                    DMA_CH_IER_AIE_LEN, 1);
        */
        dma_ch_ier = FXGMAC_SET_REG_BITS(dma_ch_ier,
                         DMA_CH_IER_FBEE_POS,
                    DMA_CH_IER_FBEE_LEN, 1);

        if (channel->tx_ring) {
            /* Enable the following Tx interrupts
             *   TIE  - Transmit Interrupt Enable (unless using
             *          per channel interrupts)
             */
            if (!pdata->per_channel_irq)
                dma_ch_ier = FXGMAC_SET_REG_BITS(
                        dma_ch_ier,
                        DMA_CH_IER_TIE_POS,
                        DMA_CH_IER_TIE_LEN,
                        1);
            if (FXGMAC_IS_CHANNEL_WITH_TX_IRQ(i)) {
                if (pdata->per_channel_irq) {
                    dma_ch_ier = FXGMAC_SET_REG_BITS(
                        dma_ch_ier,
                        DMA_CH_IER_TIE_POS,
                        DMA_CH_IER_TIE_LEN,
                        1);

                    /*dma_ch_ier = FXGMAC_SET_REG_BITS(
                            dma_ch_ier,
                            DMA_CH_IER_TBUE_POS,
                            DMA_CH_IER_TBUE_LEN,
                            1);*/
                }
            }
        }
        if (channel->rx_ring) {
            /* Enable following Rx interrupts
             *   RBUE - Receive Buffer Unavailable Enable
             *   RIE  - Receive Interrupt Enable (unless using
             *          per channel interrupts)
             */
            dma_ch_ier = FXGMAC_SET_REG_BITS(
                    dma_ch_ier,
                    DMA_CH_IER_RBUE_POS,
                    DMA_CH_IER_RBUE_LEN,
                    1);
            //2022-04-20 xiaojiang comment
            //windows driver set the per_channel_irq to be zero, Linux driver also comment this, so comment it directly
            //if (!pdata->per_channel_irq)
                dma_ch_ier = FXGMAC_SET_REG_BITS(
                        dma_ch_ier,
                        DMA_CH_IER_RIE_POS,
                        DMA_CH_IER_RIE_LEN,
                        1);
        }

        writereg(pdata->pAdapter, dma_ch_ier, FXGMAC_DMA_REG(channel, DMA_CH_IER));
    }
#else
	struct fxgmac_tx_queue *txq;
	unsigned int dma_ch_isr, dma_ch_ier;
	unsigned int i;

	for (i = 0; i < pdata->expansion.eth_dev->data->nb_tx_queues; i++) {
		txq = pdata->expansion.eth_dev->data->tx_queues[i];

		/* Clear all the interrupts which are set */
		dma_ch_isr = FXGMAC_DMA_IOREAD(txq, DMA_CH_SR);
		FXGMAC_DMA_IOWRITE(txq, DMA_CH_SR, dma_ch_isr);

		/* Clear all interrupt enable bits */
		dma_ch_ier = 0;

		/* Enable following interrupts
		 *   NIE  - Normal Interrupt Summary Enable
		 *   AIE  - Abnormal Interrupt Summary Enable
		 *   FBEE - Fatal Bus Error Enable
		 */
		FXGMAC_SET_BITS(dma_ch_ier, DMA_CH_IER, NIE, 1);//0  fx 1
		FXGMAC_SET_BITS(dma_ch_ier, DMA_CH_IER, AIE, 1);
		FXGMAC_SET_BITS(dma_ch_ier, DMA_CH_IER, FBEE, 1);

		/* Enable following Rx interrupts
		 *   RBUE - Receive Buffer Unavailable Enable
		 *   RIE  - Receive Interrupt Enable (unless using
		 *          per channel interrupts in edge triggered
		 *          mode)
		 */
		FXGMAC_SET_BITS(dma_ch_ier, DMA_CH_IER, RBUE, 1);//0 fx 1
		FXGMAC_SET_BITS(dma_ch_ier, DMA_CH_IER, RIE, 0);// 0 fx 1

		FXGMAC_DMA_IOWRITE(txq, DMA_CH_IER, dma_ch_ier);
	}
#endif
}

static void fxgmac_enable_mtl_interrupts(struct fxgmac_pdata *pdata)
{
    unsigned int q_count, i;
    unsigned int mtl_q_isr;

    q_count = max(pdata->hw_feat.tx_q_cnt, pdata->hw_feat.rx_q_cnt);
    for (i = 0; i < q_count; i++) {
        /* Clear all the interrupts which are set */
        mtl_q_isr = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, i, MTL_Q_ISR));
        writereg(pdata->pAdapter, mtl_q_isr, FXGMAC_MTL_REG(pdata, i, MTL_Q_ISR));

        /* No MTL interrupts to be enabled */
        writereg(pdata->pAdapter, 0, FXGMAC_MTL_REG(pdata, i, MTL_Q_IER));
    }
}

static void fxgmac_enable_mac_interrupts(struct fxgmac_pdata *pdata)
{
    unsigned int mac_ier = 0;
    u32 regval;

    /* Enable Timestamp interrupt */
    mac_ier = FXGMAC_SET_REG_BITS(mac_ier, MAC_IER_TSIE_POS,
                      MAC_IER_TSIE_LEN, 1);

    writereg(pdata->pAdapter, mac_ier, pdata->mac_regs + MAC_IER);

    /* Enable all counter interrupts */
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MMC_RIER);
    regval = FXGMAC_SET_REG_BITS(regval, MMC_RIER_ALL_INTERRUPTS_POS,
                     MMC_RIER_ALL_INTERRUPTS_LEN, 0xffffffff);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MMC_RIER);
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MMC_TIER);
    regval = FXGMAC_SET_REG_BITS(regval, MMC_TIER_ALL_INTERRUPTS_POS,
                     MMC_TIER_ALL_INTERRUPTS_LEN, 0xffffffff);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MMC_TIER);
}

static int fxgmac_set_fxgmii_2500_speed(struct fxgmac_pdata *pdata)
{
    u32 regval;

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_CR);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_PS_POS,
                     MAC_CR_PS_LEN, 0);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_FES_POS,
                     MAC_CR_FES_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_DM_POS,
                     MAC_CR_DM_LEN , pdata->phy_duplex);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_CR);

    return 0;
}

static int fxgmac_set_fxgmii_1000_speed(struct fxgmac_pdata *pdata)
{
    u32 regval;

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_CR);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_PS_POS,
                     MAC_CR_PS_LEN, 0);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_FES_POS,
                     MAC_CR_FES_LEN, 0);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_DM_POS,
                     MAC_CR_DM_LEN , pdata->phy_duplex);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_CR);

    return 0;
}

static int fxgmac_set_fxgmii_100_speed(struct fxgmac_pdata *pdata)
{
        u32 regval;

        regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_CR);
        regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_PS_POS,
                                     MAC_CR_PS_LEN, 1);
        regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_FES_POS,
                                     MAC_CR_FES_LEN, 1);
        regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_DM_POS,
                     MAC_CR_DM_LEN , pdata->phy_duplex);
        writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_CR);

        return 0;
}

static int fxgmac_set_fxgmii_10_speed(struct fxgmac_pdata *pdata)
{
        u32 regval;

        regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_CR);
        regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_PS_POS,
                                     MAC_CR_PS_LEN, 1);
        regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_FES_POS,
                                     MAC_CR_FES_LEN, 0);
        regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_DM_POS,
                     MAC_CR_DM_LEN , pdata->phy_duplex);
        writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_CR);

        return 0;
}

 /**
 * fxgmac_check_phy_link - Get link/speed status
 * @pdata: pointer to gmac structure
 * @speed: pointer to link speed
 * @link_up: true is link is up, false otherwise
 * @link_up_wait_to_complete: bool used to wait for link up or not
 *
 * Reads the links register to determine if link is up and the current speed
 **/
static int fxgmac_check_phy_link(struct fxgmac_pdata *pdata,
                                      u32 *speed, bool *link_up,
                                      bool link_up_wait_to_complete)
{
#if defined(LINUX) || defined(DPDK)
    u16 link_reg = 0;

    //DPRINTK("fxgmac_check_phy_link callin\n");

#ifndef DPDK
    struct net_device *netdev = pdata->netdev;
    if (netdev->base_addr) {
        link_reg = (u16)(* ((u32 *)(netdev->base_addr + MGMT_EPHY_CTRL)));
#else
    (void) link_up_wait_to_complete;
    if (pdata->base_mem) {
    	link_reg = (u16)readreg(pdata->pAdapter, pdata->base_mem + MGMT_EPHY_CTRL);

    	pdata->phy_duplex = !!(link_reg&0x4);//need check
#endif
        /*
        * check register address 0x1004
        * b[6:5]       ephy_pause
        * b[4:3]       ephy_speed 0b10 1000m 0b01 100m
        * b[2]         ephy_duplex
        * b[1]         ephy_link
        * b[0]         ephy_reset. should be set to 1 before use phy.
        */
        *link_up = false;
        if (link_reg & MGMT_EPHY_CTRL_STA_EPHY_RELEASE) {
            if(link_up) {
                *link_up = (link_reg & MGMT_EPHY_CTRL_STA_EPHY_LINKUP) ? true : false;
            }
            if(speed) *speed = (link_reg & MGMT_EPHY_CTRL_STA_SPEED_MASK) >> MGMT_EPHY_CTRL_STA_SPEED_POS;
        } else {
            DPRINTK("fxgmac_check_phy_link ethernet PHY not released.\n");
            return -1;
        }
    }else {
        DPRINTK("fxgmac_check_phy_link null base addr err\n");
        return -1;
    }
    //DPRINTK("fxgmac_check_phy_link callout, reg=%#x\n", link_reg);
#else
    pdata = pdata;
    speed = speed;
    link_up = link_up;
    link_up_wait_to_complete = link_up_wait_to_complete;
#endif

        return 0;
}

static int fxgmac_config_mac_speed(struct fxgmac_pdata *pdata)
{
    switch (pdata->phy_speed) {
    case SPEED_2500:
        fxgmac_set_fxgmii_2500_speed(pdata);
        break;
    case SPEED_1000:
        fxgmac_set_fxgmii_1000_speed(pdata);
        break;
    case SPEED_100:
        fxgmac_set_fxgmii_100_speed(pdata);
        break;
    case SPEED_10:
        fxgmac_set_fxgmii_10_speed(pdata);
        break;
    }
    return 0;
}

static int fxgmac_write_ephy_reg(struct fxgmac_pdata* pdata, u32 reg_id, u32 data)
{
    u32 regval;
    u32 mdioctrl = reg_id * 0x10000 + 0x8000205;
    int busy = 15;

    writereg(pdata->pAdapter, data, pdata->mac_regs + MAC_MDIO_DATA);
    writereg(pdata->pAdapter, mdioctrl, pdata->mac_regs + MAC_MDIO_ADDRESS);
    do {
        regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_MDIO_ADDRESS);
        busy--;
    }while((regval & MAC_MDIO_ADDRESS_BUSY) && (busy));

    DPRINTK("fxgmac_write_ephy_reg id %d %s, ctrl=0x%08x, data=0x%08x\n", reg_id, (regval & 0x1)?"err" : "ok", regval, data);

    return (regval & MAC_MDIO_ADDRESS_BUSY) ? -1 : 0; //-1 indicates err
}

static int fxgmac_read_ephy_reg(struct fxgmac_pdata* pdata, u32 reg_id, u32*data)
{
    u32 regval = 0, regret;
    u32 mdioctrl = reg_id * 0x10000 + 0x800020d;
    int busy = 15;

    writereg(pdata->pAdapter, mdioctrl, pdata->mac_regs + MAC_MDIO_ADDRESS);
    do {
        regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_MDIO_ADDRESS);
        busy--;
        //DPRINTK("fxgmac_read_ephy_reg, check busy %d, ctrl=0x%08x\n", busy, regval);
    }while((regval & MAC_MDIO_ADDRESS_BUSY) && (busy));

    if (0 == (regval & MAC_MDIO_ADDRESS_BUSY)) {
        regret = readreg(pdata->pAdapter, pdata->mac_regs + MAC_MDIO_DATA);
        if(data) *data = regret;
        //DPRINTK("fxgmac_read_ephy_reg ok, reg=0x%02x, ctrl=0x%08x, data=0x%08x\n", reg_id, regval, *data);
        return regret;
    }

    DPRINTK("fxgmac_read_ephy_reg id=0x%02x err, busy=%d, ctrl=0x%08x.\n", reg_id, busy, regval);
    return -1;
}

static int fxgmac_write_ephy_mmd_reg(struct fxgmac_pdata* pdata, u32 reg_id, u32 mmd, u32 data)
{
    u32 regval;
    u32 mdioctrl = (mmd << 16) + 0x8000207;
    u32 regdata = (reg_id << 16) + data;
    //for phy mmd reg r/w operation, set more delay time than phy mii reg r/w
    int busy = 60;

    writereg(pdata->pAdapter, regdata, pdata->mac_regs + MAC_MDIO_DATA);
    writereg(pdata->pAdapter, mdioctrl, pdata->mac_regs + MAC_MDIO_ADDRESS);
    do {
        regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_MDIO_ADDRESS);
        busy--;
    } while ((regval & MAC_MDIO_ADDRESS_BUSY) && (busy));

    DPRINTK("fxgmac_write_ephy_mmd_reg id %d mmd %d %s, ctrl=0x%08x, data=0x%08x\n", reg_id, mmd, (regval & 0x1) ? "err" : "ok", regval, data);
    //DbgPrintF(MP_TRACE, "fxgmac_write_ephy_mmd_reg id %d %s, ctrl=0x%08x, data=0x%08x busy %d", reg_id, (regval & 0x1) ? "err" : "ok", regval, data, busy);

    return (regval & MAC_MDIO_ADDRESS_BUSY) ? -1 : 0; //-1 indicates err
}

#if !defined(LINUX) && !defined(DPDK)
static int fxgmac_read_ephy_mmd_reg(struct fxgmac_pdata* pdata, u32 reg_id, u32 mmd, u32* data)
{
    u32 regval = 0, regret;
    u32 mdioctrl = (mmd << 16) + 0x800020f;
    u32 regdata = (reg_id << 16);
    //for phy mmd reg r/w operation, set more delay time than phy mii reg r/w
    int busy = 60;

    writereg(pdata->pAdapter, regdata, pdata->mac_regs + MAC_MDIO_DATA);
    writereg(pdata->pAdapter, mdioctrl, pdata->mac_regs + MAC_MDIO_ADDRESS);

    do {
        regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_MDIO_ADDRESS);
        busy--;
    } while ((regval & MAC_MDIO_ADDRESS_BUSY) && (busy));

    if (0 == (regval & MAC_MDIO_ADDRESS_BUSY)) {
        regret = readreg(pdata->pAdapter, pdata->mac_regs + MAC_MDIO_DATA);
        if (data) *data = (regret & 0xffff);
        return regret;
    }

    DPRINTK("fxgmac_read_ephy_mmd_reg id=0x%02x mmd %d err, busy=%d, ctrl=0x%08x\n", reg_id, mmd, busy, regval);
    //DbgPrintF(MP_TRACE, "fxgmac_read_ephy_mmd_reg id=0x%02x err, busy=%d, ctrl=0x%08x\n", reg_id, busy, regval);
    return -1;
}
#endif

static void fxgmac_config_flow_control(struct fxgmac_pdata* pdata)
{
#ifndef UEFI
    u32 regval = 0;
#endif

    fxgmac_config_tx_flow_control(pdata);
    fxgmac_config_rx_flow_control(pdata);

#ifndef UEFI
    fxgmac_read_ephy_reg(pdata, REG_MII_ADVERTISE, &regval);
    //set auto negotiation advertisement pause ability
    if (pdata->tx_pause || pdata->rx_pause) {
        regval = FXGMAC_SET_REG_BITS(regval, PHY_MII_ADVERTISE_PAUSE_POS, PHY_MII_ADVERTISE_PAUSE_LEN, 1);
        regval = FXGMAC_SET_REG_BITS(regval, PHY_MII_ADVERTISE_ASYPAUSE_POS, PHY_MII_ADVERTISE_ASYPAUSE_LEN, 1);
    } else {
        regval = FXGMAC_SET_REG_BITS(regval, PHY_MII_ADVERTISE_PAUSE_POS, PHY_MII_ADVERTISE_PAUSE_LEN, 0);
        regval = FXGMAC_SET_REG_BITS(regval, PHY_MII_ADVERTISE_ASYPAUSE_POS, PHY_MII_ADVERTISE_ASYPAUSE_LEN, 0);
    }
    fxgmac_write_ephy_reg(pdata, REG_MII_ADVERTISE, regval);
    //after change the auto negotiation advertisement need to soft reset
    fxgmac_read_ephy_reg(pdata, REG_MII_BMCR, &regval);
    regval = FXGMAC_SET_REG_BITS(regval, PHY_CR_RESET_POS, PHY_CR_RESET_LEN, 1);
    fxgmac_write_ephy_reg(pdata, REG_MII_BMCR, regval);
#endif
}

static int fxgmac_set_ephy_autoneg_advertise(struct fxgmac_pdata* pdata, struct fxphy_ag_adv phy_ag_adv)
{
    u32 regval = 0, ret = 0;

    if (phy_ag_adv.auto_neg_en)	{
        fxgmac_read_ephy_reg(pdata, REG_MII_BMCR, &regval);
        regval = FXGMAC_SET_REG_BITS(regval, PHY_CR_AUTOENG_POS, PHY_CR_AUTOENG_LEN, 1);
        ret |= fxgmac_write_ephy_reg(pdata, REG_MII_BMCR, regval);
    } else {
        fxgmac_read_ephy_reg(pdata, REG_MII_BMCR, &regval);
        regval = FXGMAC_SET_REG_BITS(regval, PHY_CR_AUTOENG_POS, PHY_CR_AUTOENG_LEN, 0);
        ret |= fxgmac_write_ephy_reg(pdata, REG_MII_BMCR, regval);
    }

    fxgmac_read_ephy_reg(pdata, REG_MII_CTRL1000, &regval);
    if (phy_ag_adv.full_1000m) {
        regval = FXGMAC_SET_REG_BITS(regval, PHY_MII_CTRL1000_1000FULL_POS, PHY_MII_CTRL1000_1000FULL_LEN, 1);
    } else {
        regval = FXGMAC_SET_REG_BITS(regval, PHY_MII_CTRL1000_1000FULL_POS, PHY_MII_CTRL1000_1000FULL_LEN, 0);
    }
    if (phy_ag_adv.half_1000m) {
        regval = FXGMAC_SET_REG_BITS(regval, PHY_MII_CTRL1000_1000HALF_POS, PHY_MII_CTRL1000_1000HALF_LEN, 1);
    } else {
        regval = FXGMAC_SET_REG_BITS(regval, PHY_MII_CTRL1000_1000HALF_POS, PHY_MII_CTRL1000_1000HALF_LEN, 0);
    }
    ret |= fxgmac_write_ephy_reg(pdata, REG_MII_CTRL1000, regval);

    fxgmac_read_ephy_reg(pdata, REG_MII_ADVERTISE, &regval);

    if (phy_ag_adv.full_100m) {
        regval = FXGMAC_SET_REG_BITS(regval, PHY_MII_ADVERTISE_100FULL_POS, PHY_MII_ADVERTISE_100FULL_LEN, 1);
    } else {
        regval = FXGMAC_SET_REG_BITS(regval, PHY_MII_ADVERTISE_100FULL_POS, PHY_MII_ADVERTISE_100FULL_LEN, 0);
    }
    if (phy_ag_adv.half_100m) {
        regval = FXGMAC_SET_REG_BITS(regval, PHY_MII_ADVERTISE_100HALF_POS, PHY_MII_ADVERTISE_100HALF_LEN, 1);
    } else {
        regval = FXGMAC_SET_REG_BITS(regval, PHY_MII_ADVERTISE_100HALF_POS, PHY_MII_ADVERTISE_100HALF_LEN, 0);
    }
    if (phy_ag_adv.full_10m) {
        regval = FXGMAC_SET_REG_BITS(regval, PHY_MII_ADVERTISE_10FULL_POS, PHY_MII_ADVERTISE_10FULL_LEN, 1);
    } else {
        regval = FXGMAC_SET_REG_BITS(regval, PHY_MII_ADVERTISE_10FULL_POS, PHY_MII_ADVERTISE_10FULL_LEN, 0);
    }
    if (phy_ag_adv.half_10m) {
        regval = FXGMAC_SET_REG_BITS(regval, PHY_MII_ADVERTISE_10HALF_POS, PHY_MII_ADVERTISE_10HALF_LEN, 1);
    } else {
        regval = FXGMAC_SET_REG_BITS(regval, PHY_MII_ADVERTISE_10HALF_POS, PHY_MII_ADVERTISE_10HALF_LEN, 0);
    }

    ret |= fxgmac_write_ephy_reg(pdata, REG_MII_ADVERTISE, regval);
    //after change the auto negotiation advertisement need to soft reset
    fxgmac_read_ephy_reg(pdata, REG_MII_BMCR, &regval);
    regval = FXGMAC_SET_REG_BITS(regval, PHY_CR_RESET_POS, PHY_CR_RESET_LEN, 1);
    ret |= fxgmac_write_ephy_reg(pdata, REG_MII_BMCR, regval);

    return ret;
}

static int fxgmac_phy_config(struct fxgmac_pdata* pdata)
{
    struct fxphy_ag_adv phy_ag_adv;

    if (pdata->phy_autoeng) {
        phy_ag_adv.auto_neg_en = 1;
    } else {
        phy_ag_adv.auto_neg_en = 0;
    }
    switch (pdata->phy_speed)
    {
    case SPEED_1000:
        phy_ag_adv.full_1000m = 1, phy_ag_adv.half_1000m = 0, phy_ag_adv.full_100m = 1, phy_ag_adv.half_100m = 1, phy_ag_adv.full_10m = 1, phy_ag_adv.half_10m = 1;
        break;

    case SPEED_100:
        phy_ag_adv.full_1000m = 0, phy_ag_adv.half_1000m = 0;
        if (pdata->phy_duplex) {
            phy_ag_adv.full_100m = 1;
        } else {
            phy_ag_adv.full_100m = 0;
        }
        phy_ag_adv.half_100m = 1, phy_ag_adv.full_10m = 1, phy_ag_adv.half_10m = 1;
        break;

    case SPEED_10:
        phy_ag_adv.full_1000m = 0, phy_ag_adv.half_1000m = 0;
        phy_ag_adv.full_100m = 0, phy_ag_adv.half_100m = 0;
        if (pdata->phy_duplex) {
            phy_ag_adv.full_10m = 1;
        } else {
            phy_ag_adv.full_10m = 0;
        }
        phy_ag_adv.half_10m = 1;
        break;

    default:
        break;
    }
    return fxgmac_set_ephy_autoneg_advertise(pdata, phy_ag_adv);
}

static void fxgmac_phy_green_ethernet(struct fxgmac_pdata* pdata)
{
    u32 regval = 0;
    //GREEN
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_REG_PMA_DBG0_ADC);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_ENABLE_GIGA_POWER_SAVING_FOR_SHORT_CABLE);

    //CLD
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_REG_CLD_REG0);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_ENABLE_CLD_NP_WP);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_REG_CLD_REG1);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_ENABLE_CLD_GT_HT_BT);

    //after change green ethernet & CLD need to soft reset
    fxgmac_read_ephy_reg(pdata, REG_MII_BMCR, &regval);
    regval = FXGMAC_SET_REG_BITS(regval, PHY_CR_RESET_POS, PHY_CR_RESET_LEN, 1);
    fxgmac_write_ephy_reg(pdata, REG_MII_BMCR, regval);
}

static void fxgmac_phy_eee_feature(struct fxgmac_pdata* pdata)
{
    u32 regval = 0;

    regval = readreg(pdata->pAdapter, pdata->mac_regs + DMA_SBMR);
    regval = FXGMAC_SET_REG_BITS(regval, DMA_SBMR_EN_LPI_POS, DMA_SBMR_EN_LPI_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, DMA_SBMR_LPI_XIT_PKT_POS, DMA_SBMR_LPI_XIT_PKT_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, DMA_SBMR_AALE_POS, DMA_SBMR_AALE_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + DMA_SBMR);

    //regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_IER);
    //regval = FXGMAC_SET_REG_BITS(regval, MAC_LPIIE_POS, MAC_LPIIE_LEN, 1);
    //writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_IER);

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_LPI_STA);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_LPIATE_POS, MAC_LPIATE_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_LPITXA_POS, MAC_LPITXA_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_PLS_POS, MAC_PLS_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_LPIEN_POS, MAC_LPIEN_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_LPI_STA);

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_LPI_TIMER);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_LPIET_POS, MAC_LPIET_LEN, MAC_LPI_ENTRY_TIMER);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_LPI_TIMER);

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_LPI_CONTROL);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_TWT_POS, MAC_TWT_LEN, MAC_TWT_TIMER);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_LST_POS, MAC_LST_LEN, MAC_LST_TIMER);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_LPI_CONTROL);

    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_MS_TIC_COUNTER);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_MS_TIC_POS, MAC_MS_TIC_LEN, MAC_MS_TIC);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_MS_TIC_COUNTER);

    //usleep_range_ex(pdata->pAdapter, 1500, 1500);

    fxgmac_write_ephy_mmd_reg(pdata, REG_MMD_EEE_ABILITY_REG, 0x07, REG_MMD_EEE_ABILITY_VALUE);

    //after change EEE need to soft reset
    fxgmac_read_ephy_reg(pdata, REG_MII_BMCR, &regval);
    regval = FXGMAC_SET_REG_BITS(regval, PHY_CR_RESET_POS, PHY_CR_RESET_LEN, 1);
    fxgmac_write_ephy_reg(pdata, REG_MII_BMCR, regval);
}

static void fxgmac_reset_phy(struct fxgmac_pdata* pdata)
{
    u32 value = 0;

    value = FXGMAC_SET_REG_BITS(value, MGMT_EPHY_CTRL_RESET_POS, MGMT_EPHY_CTRL_RESET_LEN, MGMT_EPHY_CTRL_STA_EPHY_RESET);
    writereg(pdata->pAdapter, value, pdata->base_mem + MGMT_EPHY_CTRL);
    usleep_range_ex(pdata->pAdapter, 1500, 1500);
}

void fxgmac_release_phy(struct fxgmac_pdata* pdata)
{
    u32 value = 0;

    value = FXGMAC_SET_REG_BITS(value, MGMT_EPHY_CTRL_RESET_POS, MGMT_EPHY_CTRL_RESET_LEN, MGMT_EPHY_CTRL_STA_EPHY_RELEASE);
    writereg(pdata->pAdapter, value, pdata->base_mem + MGMT_EPHY_CTRL);
    usleep_range_ex(pdata->pAdapter, 100, 150);
    value = readreg(pdata->pAdapter, pdata->base_mem + MGMT_EPHY_CTRL);
    DBGPRINT(MP_LOUD, ("0x1004: 0x%x\n", value));
#ifdef AISC_MODE
    fxgmac_read_ephy_reg(pdata, REG_MII_SPEC_CTRL, &value);//	read  phy specific control
    value = FXGMAC_SET_REG_BITS(value, PHY_MII_SPEC_CTRL_CRS_ON_POS, PHY_MII_SPEC_CTRL_CRS_ON_LEN, 1);//set on crs on
    fxgmac_write_ephy_reg(pdata, REG_MII_SPEC_CTRL, value);// phy specific control set on crs on

    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_ANALOG_CFG3);
    fxgmac_read_ephy_reg(pdata, REG_MII_EXT_DATA, &value);
    // VGA bandwidth, default is 2 after reset. Set to 0 to mitigate unstable issue in 130m.
    value = FXGMAC_SET_REG_BITS(value, MII_EXT_ANALOG_CFG3_ADC_START_CFG_POS,
        MII_EXT_ANALOG_CFG3_ADC_START_CFG_LEN, MII_EXT_ANALOG_CFG3_ADC_START_CFG_DEFAULT);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, value);

#if 0
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_PMA_DEBUG_KCOEF);
    fxgmac_read_ephy_reg(pdata, REG_MII_EXT_DATA, &value);
    /* After reset, it's 0x10. We need change it to 0x20 to make it easier to linkup in gigabit mode with long cable. But this is has side effect.*/
    value = FXGMAC_SET_REG_BITS(value, MII_EXT_PMA_DEBUG_KCOEF_IPR_KCOEF_GE_LNG_POS,
        MII_EXT_PMA_DEBUG_KCOEF_IPR_KCOEF_GE_LNG_LEN, MII_EXT_PMA_DEBUG_KCOEF_IPR_KCOEF_GE_LNG_DEFAULT);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, value);
#endif

    fxgmac_efuse_read_data(pdata, EFUSE_LED_ADDR, &value);
    //led index use bit0~bit5
    value = FXGMAC_GET_REG_BITS(value, EFUSE_LED_POS, EFUSE_LED_LEN);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_ANALOG_CFG2);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_ANALOG_CFG2_LED_VALUE);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_ANALOG_CFG8);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_ANALOG_CFG8_LED_VALUE);

    if (EFUSE_LED_COMMON_SOLUTION != value) {
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED0_CFG);
        switch (value) {
        case EFUSE_LED_SOLUTION1:
            fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_COMMON_LED0_CFG_VALUE_SOLUTION1);
            break;
        case EFUSE_LED_SOLUTION2:
            fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_COMMON_LED0_CFG_VALUE_SOLUTION2);
            break;
        case EFUSE_LED_SOLUTION3:
        case EFUSE_LED_SOLUTION4:
            fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_COMMON_LED0_CFG_VALUE_SOLUTION3);
            break;
        default:
            //default solution
            fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_COMMON_LED0_CFG_VALUE_SOLUTION0);
            break;
        }

        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED1_CFG);
        switch (value) {
        case EFUSE_LED_SOLUTION1:
        case EFUSE_LED_SOLUTION4:
            fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_COMMON_LED1_CFG_VALUE_SOLUTION1);
            break;
        case EFUSE_LED_SOLUTION2:
        case EFUSE_LED_SOLUTION3:
            fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_COMMON_LED1_CFG_VALUE_SOLUTION2);
            break;
        default:
            //default solution
            fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_COMMON_LED1_CFG_VALUE_SOLUTION0);
            break;
        }

        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED2_CFG);
        switch (value) {
        case EFUSE_LED_SOLUTION1:
            fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_COMMON_LED2_CFG_VALUE_SOLUTION0);
            break;
        case EFUSE_LED_SOLUTION2:
            fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_COMMON_LED2_CFG_VALUE_SOLUTION2);
            break;
        case EFUSE_LED_SOLUTION3:
            fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_COMMON_LED2_CFG_VALUE_SOLUTION3);
            break;
        case EFUSE_LED_SOLUTION4:
            fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_COMMON_LED2_CFG_VALUE_SOLUTION4);
            break;
        default:
            //default solution
            fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_COMMON_LED2_CFG_VALUE_SOLUTION0);
            break;
        }

        if (EFUSE_LED_SOLUTION2 == value) {
            fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED_BLINK_CFG);
            fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_COMMON_LED_BLINK_CFG_SOLUTION2);
        }
    }
#endif
}

#if !defined(UEFI)
static void fxgmac_enable_phy_check(struct fxgmac_pdata* pdata)
{
    u32 value = 0;
    //value = 0xa8d0;

    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_PKG_CFG0);
    fxgmac_read_ephy_reg(pdata, REG_MII_EXT_DATA, &value);
    value = FXGMAC_SET_REG_BITS(value, REG_MII_EXT_PKG_CHECK_POS, REG_MII_EXT_PKG_CHECK_LEN, REG_MII_EXT_PKG_ENABLE_CHECK);

    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_PKG_CFG0);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, value);
}

static void fxgmac_disable_phy_check(struct fxgmac_pdata* pdata)
{
    u32 value = 0;
    //value = 0x68d0;

    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_PKG_CFG0);
    fxgmac_read_ephy_reg(pdata, REG_MII_EXT_DATA, &value);
    value = FXGMAC_SET_REG_BITS(value, REG_MII_EXT_PKG_CHECK_POS, REG_MII_EXT_PKG_CHECK_LEN, REG_MII_EXT_PKG_DISABLE_CHECK);

    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_PKG_CFG0);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, value);
}

static void fxgmac_setup_cable_loopback(struct fxgmac_pdata* pdata)
{
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_SLEEP_CONTROL_REG);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_SLEEP_REG_ENABLE_LOOPBACK);

    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_LPBK_REG);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_LPBK_REG_ENABLE_LOOPBACK);

    fxgmac_write_ephy_reg(pdata, REG_MII_BMCR, REG_MII_BMCR_ENABLE_LOOPBACK);
}

static void fxgmac_clean_cable_loopback(struct fxgmac_pdata* pdata)
{
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_SLEEP_CONTROL_REG);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_SLEEP_REG_CLEAN_LOOPBACK);

    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_LPBK_REG);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_LPBK_REG_CLEAN_LOOPBACK);

    fxgmac_write_ephy_reg(pdata, REG_MII_BMCR, REG_MII_BMCR_DISABLE_LOOPBACK);
}

static void fxgmac_disable_phy_sleep(struct fxgmac_pdata* pdata)
{
    u32 value = 0;
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_SLEEP_CONTROL_REG);
    fxgmac_read_ephy_reg(pdata, REG_MII_EXT_DATA, &value);

    value = FXGMAC_SET_REG_BITS(value, MII_EXT_SLEEP_CONTROL1_EN_POS, MII_EXT_SLEEP_CONTROL1_EN_LEN, 0);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_SLEEP_CONTROL_REG);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, value);
}

static void fxgmac_enable_phy_sleep(struct fxgmac_pdata* pdata)
{
    u32 value = 0;
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_SLEEP_CONTROL_REG);
    fxgmac_read_ephy_reg(pdata, REG_MII_EXT_DATA, &value);

    value = FXGMAC_SET_REG_BITS(value, MII_EXT_SLEEP_CONTROL1_EN_POS, MII_EXT_SLEEP_CONTROL1_EN_LEN, 1);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_SLEEP_CONTROL_REG);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, value);
}
#endif

static void fxgmac_close_phy_led(struct fxgmac_pdata* pdata)
{
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED0_CFG);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, 0x00);

    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED1_CFG);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, 0x00);

    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED2_CFG);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, 0x00);
}

static void fxmgac_config_led_under_active(struct fxgmac_pdata* pdata)
{
    u32 regval = 0;
    fxgmac_efuse_read_data(pdata, EFUSE_LED_ADDR, &regval);
    //led index use bit0~bit5
    regval = FXGMAC_GET_REG_BITS(regval, EFUSE_LED_POS, EFUSE_LED_LEN);
    if (EFUSE_LED_COMMON_SOLUTION == regval) {
        DbgPrintF(MP_TRACE, "%s >>>", __FUNCTION__);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED_CFG);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, pdata->led.s0_led_setting[0]);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED0_CFG);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, pdata->led.s0_led_setting[1]);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED1_CFG);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, pdata->led.s0_led_setting[2]);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED2_CFG);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, pdata->led.s0_led_setting[3]);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED_BLINK_CFG);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, pdata->led.s0_led_setting[4]);
    }
}

static void fxgmac_config_led_under_sleep(struct fxgmac_pdata* pdata)
{
    u32 regval = 0;
    fxgmac_efuse_read_data(pdata, EFUSE_LED_ADDR, &regval);
    //led index use bit0~bit5
    regval = FXGMAC_GET_REG_BITS(regval, EFUSE_LED_POS, EFUSE_LED_LEN);
    if (EFUSE_LED_COMMON_SOLUTION == regval) {
        DbgPrintF(MP_TRACE, "%s >>>", __FUNCTION__);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED_CFG);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, pdata->led.s3_led_setting[0]);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED0_CFG);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, pdata->led.s3_led_setting[1]);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED1_CFG);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, pdata->led.s3_led_setting[2]);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED2_CFG);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, pdata->led.s3_led_setting[3]);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED_BLINK_CFG);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, pdata->led.s3_led_setting[4]);
    }
}

static void fxgmac_config_led_under_shutdown(struct fxgmac_pdata* pdata)
{
    u32 regval = 0;
    fxgmac_efuse_read_data(pdata, EFUSE_LED_ADDR, &regval);
    //led index use bit0~bit5
    regval = FXGMAC_GET_REG_BITS(regval, EFUSE_LED_POS, EFUSE_LED_LEN);
    if (EFUSE_LED_COMMON_SOLUTION == regval) {
        DbgPrintF(MP_TRACE, "%s >>>", __FUNCTION__);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED_CFG);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, pdata->led.s5_led_setting[0]);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED0_CFG);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, pdata->led.s5_led_setting[1]);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED1_CFG);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, pdata->led.s5_led_setting[2]);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED2_CFG);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, pdata->led.s5_led_setting[3]);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED_BLINK_CFG);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, pdata->led.s5_led_setting[4]);
    }
}

static void fxgmac_config_led_under_disable(struct fxgmac_pdata* pdata)
{
    u32 regval = 0;
    fxgmac_efuse_read_data(pdata, EFUSE_LED_ADDR, &regval);
    //led index use bit0~bit5
    regval = FXGMAC_GET_REG_BITS(regval, EFUSE_LED_POS, EFUSE_LED_LEN);
    if (EFUSE_LED_COMMON_SOLUTION == regval) {
        DbgPrintF(MP_TRACE, "%s >>>", __FUNCTION__);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED_CFG);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, pdata->led.disable_led_setting[0]);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED0_CFG);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, pdata->led.disable_led_setting[1]);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED1_CFG);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, pdata->led.disable_led_setting[2]);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED2_CFG);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, pdata->led.disable_led_setting[3]);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED_BLINK_CFG);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, pdata->led.disable_led_setting[4]);
    }
    else {
        //http://redmine.motor-comm.com/issues/4101
        //for disable case,reset phy to close LED
        fxgmac_reset_phy(pdata);
    }
}

#ifdef LINUX
extern void fxgmac_diag_get_rx_info(struct fxgmac_channel *channel);

static int fxgmac_dev_read(struct fxgmac_channel *channel)
{
    struct fxgmac_pdata *pdata = channel->pdata;
    struct fxgmac_ring *ring = channel->rx_ring;
    struct net_device *netdev = pdata->netdev;
    struct fxgmac_desc_data *desc_data;
    struct fxgmac_dma_desc *dma_desc;
    struct fxgmac_pkt_info *pkt_info;
    unsigned int err, etlt, l34t;

    //unsigned int i;
    static unsigned int cnt_incomplete = 0;

    desc_data = FXGMAC_GET_DESC_DATA(ring, ring->cur);
    dma_desc = desc_data->dma_desc;
    pkt_info = &ring->pkt_info;

    /* Check for data availability */
    if (FXGMAC_GET_REG_BITS_LE(dma_desc->desc3,
                   RX_NORMAL_DESC3_OWN_POS,
                   RX_NORMAL_DESC3_OWN_LEN))
    {
        return 1;
    }

    /* Make sure descriptor fields are read after reading the OWN bit */
    dma_rmb();

    if (netif_msg_rx_status(pdata))
        fxgmac_dump_rx_desc(pdata, ring, ring->cur);

    if (FXGMAC_GET_REG_BITS_LE(dma_desc->desc3,
                   RX_NORMAL_DESC3_CTXT_POS,
                   RX_NORMAL_DESC3_CTXT_LEN)) {
        /* Timestamp Context Descriptor */
        fxgmac_get_rx_tstamp(pkt_info, dma_desc);

        pkt_info->attributes = FXGMAC_SET_REG_BITS(
                    pkt_info->attributes,
                    RX_PACKET_ATTRIBUTES_CONTEXT_POS,
                    RX_PACKET_ATTRIBUTES_CONTEXT_LEN,
                    1);
        pkt_info->attributes = FXGMAC_SET_REG_BITS(
                pkt_info->attributes,
                RX_PACKET_ATTRIBUTES_CONTEXT_NEXT_POS,
                RX_PACKET_ATTRIBUTES_CONTEXT_NEXT_LEN,
                0);
        if(netif_msg_rx_status(pdata)) DPRINTK("dev_read context desc,ch=%s\n",channel->name);
        return 0;
    }

    /* Normal Descriptor, be sure Context Descriptor bit is off */
    pkt_info->attributes = FXGMAC_SET_REG_BITS(
                pkt_info->attributes,
                RX_PACKET_ATTRIBUTES_CONTEXT_POS,
                RX_PACKET_ATTRIBUTES_CONTEXT_LEN,
                0);

    /* Indicate if a Context Descriptor is next */
#if 0
    if (FXGMAC_GET_REG_BITS_LE(dma_desc->desc3,
                   RX_NORMAL_DESC3_CDA_POS,
                   RX_NORMAL_DESC3_CDA_LEN))
        pkt_info->attributes = FXGMAC_SET_REG_BITS(
                pkt_info->attributes,
                RX_PACKET_ATTRIBUTES_CONTEXT_NEXT_POS,
                RX_PACKET_ATTRIBUTES_CONTEXT_NEXT_LEN,
                1);
#endif
    /* Get the header length */
    if (FXGMAC_GET_REG_BITS_LE(dma_desc->desc3,
                   RX_NORMAL_DESC3_FD_POS,
                   RX_NORMAL_DESC3_FD_LEN)) {
        desc_data->rx.hdr_len = FXGMAC_GET_REG_BITS_LE(dma_desc->desc2,
                            RX_NORMAL_DESC2_HL_POS,
                            RX_NORMAL_DESC2_HL_LEN);
        if (desc_data->rx.hdr_len)
            pdata->stats.rx_split_header_packets++;
    }
    l34t = 0;

#if 0 //(FXGMAC_RSS_FEATURE_ENABLED) //20210608
    /* Get the RSS hash */
    if (FXGMAC_GET_REG_BITS_LE(dma_desc->desc3,
                   RX_NORMAL_DESC3_RSV_POS,
                   RX_NORMAL_DESC3_RSV_LEN)) {
        pkt_info->attributes = FXGMAC_SET_REG_BITS(
                pkt_info->attributes,
                RX_PACKET_ATTRIBUTES_RSS_HASH_POS,
                RX_PACKET_ATTRIBUTES_RSS_HASH_LEN,
                1);

        pkt_info->rss_hash = le32_to_cpu(dma_desc->desc1);

        l34t = FXGMAC_GET_REG_BITS_LE(dma_desc->desc3,
                          RX_NORMAL_DESC3_L34T_POS,
                      RX_NORMAL_DESC3_L34T_LEN);
        switch (l34t) {
        case RX_DESC3_L34T_IPV4_TCP:
        case RX_DESC3_L34T_IPV4_UDP:
        case RX_DESC3_L34T_IPV6_TCP:
        case RX_DESC3_L34T_IPV6_UDP:
            pkt_info->rss_hash_type = PKT_HASH_TYPE_L4;
            break;
        default:
            pkt_info->rss_hash_type = PKT_HASH_TYPE_L3;
        }
    }
#endif
    /* Get the pkt_info length */
    desc_data->rx.len = FXGMAC_GET_REG_BITS_LE(dma_desc->desc3,
                    RX_NORMAL_DESC3_PL_POS,
                    RX_NORMAL_DESC3_PL_LEN);
    //DPRINTK("dev_read upon FD=1, pkt_len=%u\n",desc_data->rx.len);

    if (!FXGMAC_GET_REG_BITS_LE(dma_desc->desc3,
                    RX_NORMAL_DESC3_LD_POS,
                    RX_NORMAL_DESC3_LD_LEN)) {
        /* Not all the data has been transferred for this pkt_info */
        pkt_info->attributes = FXGMAC_SET_REG_BITS(
                pkt_info->attributes,
                RX_PACKET_ATTRIBUTES_INCOMPLETE_POS,
                RX_PACKET_ATTRIBUTES_INCOMPLETE_LEN,
                1);
        cnt_incomplete++;
        if ((cnt_incomplete < 2) && netif_msg_rx_status(pdata))
            DPRINTK("dev_read NOT last desc,pkt incomplete yet,%u\n", cnt_incomplete);

        return 0;
    }
    if ((cnt_incomplete) && netif_msg_rx_status(pdata))
        DPRINTK("dev_read rx back to normal and incomplete cnt=%u\n", cnt_incomplete);
    cnt_incomplete = 0; //when back to normal, reset cnt

    /* This is the last of the data for this pkt_info */
    pkt_info->attributes = FXGMAC_SET_REG_BITS(
            pkt_info->attributes,
            RX_PACKET_ATTRIBUTES_INCOMPLETE_POS,
            RX_PACKET_ATTRIBUTES_INCOMPLETE_LEN,
            0);

    /* Set checksum done indicator as appropriate */
    if (netdev->features & NETIF_F_RXCSUM)
        pkt_info->attributes = FXGMAC_SET_REG_BITS(
                pkt_info->attributes,
                RX_PACKET_ATTRIBUTES_CSUM_DONE_POS,
                RX_PACKET_ATTRIBUTES_CSUM_DONE_LEN,
                1);

    /* Check for errors (only valid in last descriptor) */
    err = FXGMAC_GET_REG_BITS_LE(dma_desc->desc3,
                     RX_NORMAL_DESC3_ES_POS,
                     RX_NORMAL_DESC3_ES_LEN);
    etlt = FXGMAC_GET_REG_BITS_LE(dma_desc->desc3,
                      RX_NORMAL_DESC3_ETLT_POS,
                      RX_NORMAL_DESC3_ETLT_LEN);
    //netif_dbg(pdata, rx_status, netdev, "err=%u, etlt=%#x\n", err, etlt);
    if((err) && netif_msg_rx_status(pdata)) {
        DPRINTK("dev_read:head_len=%u,pkt_len=%u,err=%u, etlt=%#x,desc2=0x%08x,desc3=0x%08x\n",desc_data->rx.hdr_len, desc_data->rx.len, err,
                etlt, dma_desc->desc2, dma_desc->desc3);
    }

	if (!err || !etlt) {
		/* No error if err is 0 or etlt is 0 */
		if ((etlt == 0x4 /*yzhang changed to 0x4, 0x09*/) &&
		    (netdev->features & NETIF_F_HW_VLAN_CTAG_RX)) {
			pkt_info->attributes = FXGMAC_SET_REG_BITS(
				pkt_info->attributes,
				RX_PACKET_ATTRIBUTES_VLAN_CTAG_POS,
				RX_PACKET_ATTRIBUTES_VLAN_CTAG_LEN, 1);
			pkt_info->vlan_ctag =
				FXGMAC_GET_REG_BITS_LE(dma_desc->desc0,
						       RX_NORMAL_DESC0_OVT_POS,
						       RX_NORMAL_DESC0_OVT_LEN);
			netif_dbg(pdata, rx_status, netdev, "vlan-ctag=%#06x\n",
				  pkt_info->vlan_ctag);
		}
	} else {
		if (etlt == 0x05 || etlt == 0x06)
			pkt_info->attributes = FXGMAC_SET_REG_BITS(
				pkt_info->attributes,
				RX_PACKET_ATTRIBUTES_CSUM_DONE_POS,
				RX_PACKET_ATTRIBUTES_CSUM_DONE_LEN, 0);
		else
			pkt_info->errors = FXGMAC_SET_REG_BITS(
				pkt_info->errors, RX_PACKET_ERRORS_FRAME_POS,
				RX_PACKET_ERRORS_FRAME_LEN, 1);
	}

    return 0;
}
#endif

static int fxgmac_enable_int(struct fxgmac_channel *channel,
                 enum fxgmac_int int_id)
{
	unsigned int dma_ch_ier;

    dma_ch_ier = readreg(channel->pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_IER));

    switch (int_id) {
    case FXGMAC_INT_DMA_CH_SR_TI:
        dma_ch_ier = FXGMAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_TIE_POS,
                DMA_CH_IER_TIE_LEN, 1);
        break;
    case FXGMAC_INT_DMA_CH_SR_TPS:
        dma_ch_ier = FXGMAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_TXSE_POS,
                DMA_CH_IER_TXSE_LEN, 1);
        break;
    case FXGMAC_INT_DMA_CH_SR_TBU:
        dma_ch_ier = FXGMAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_TBUE_POS,
                DMA_CH_IER_TBUE_LEN, 1);
        break;
    case FXGMAC_INT_DMA_CH_SR_RI:
        dma_ch_ier = FXGMAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_RIE_POS,
                DMA_CH_IER_RIE_LEN, 1);
        break;
    case FXGMAC_INT_DMA_CH_SR_RBU:
        dma_ch_ier = FXGMAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_RBUE_POS,
                DMA_CH_IER_RBUE_LEN, 1);
        break;
    case FXGMAC_INT_DMA_CH_SR_RPS:
        dma_ch_ier = FXGMAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_RSE_POS,
                DMA_CH_IER_RSE_LEN, 1);
        break;
    case FXGMAC_INT_DMA_CH_SR_TI_RI:
        dma_ch_ier = FXGMAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_TIE_POS,
                DMA_CH_IER_TIE_LEN, 1);
        dma_ch_ier = FXGMAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_RIE_POS,
                DMA_CH_IER_RIE_LEN, 1);
        dma_ch_ier = FXGMAC_SET_REG_BITS(
            dma_ch_ier, DMA_CH_IER_NIE_POS,
            DMA_CH_IER_NIE_LEN, 1);
        break;
    case FXGMAC_INT_DMA_CH_SR_FBE:
        dma_ch_ier = FXGMAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_FBEE_POS,
                DMA_CH_IER_FBEE_LEN, 1);
        break;
    case FXGMAC_INT_DMA_ALL:
        dma_ch_ier |= channel->saved_ier;
        break;
    default:
        return -1;
    }

    writereg(channel->pdata->pAdapter, dma_ch_ier, FXGMAC_DMA_REG(channel, DMA_CH_IER));

    return 0;
}

static int fxgmac_disable_int(struct fxgmac_channel *channel,
                  enum fxgmac_int int_id)
{
    unsigned int dma_ch_ier;

    dma_ch_ier = readreg(channel->pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_IER));

    switch (int_id) {
    case FXGMAC_INT_DMA_CH_SR_TI:
        dma_ch_ier = FXGMAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_TIE_POS,
                DMA_CH_IER_TIE_LEN, 0);
        break;
    case FXGMAC_INT_DMA_CH_SR_TPS:
        dma_ch_ier = FXGMAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_TXSE_POS,
                DMA_CH_IER_TXSE_LEN, 0);
        break;
    case FXGMAC_INT_DMA_CH_SR_TBU:
        dma_ch_ier = FXGMAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_TBUE_POS,
                DMA_CH_IER_TBUE_LEN, 0);
        break;
    case FXGMAC_INT_DMA_CH_SR_RI:
        dma_ch_ier = FXGMAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_RIE_POS,
                DMA_CH_IER_RIE_LEN, 0);
        break;
    case FXGMAC_INT_DMA_CH_SR_RBU:
        dma_ch_ier = FXGMAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_RBUE_POS,
                DMA_CH_IER_RBUE_LEN, 0);
        break;
    case FXGMAC_INT_DMA_CH_SR_RPS:
        dma_ch_ier = FXGMAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_RSE_POS,
                DMA_CH_IER_RSE_LEN, 0);
        break;
    case FXGMAC_INT_DMA_CH_SR_TI_RI:
        dma_ch_ier = FXGMAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_TIE_POS,
                DMA_CH_IER_TIE_LEN, 0);
        dma_ch_ier = FXGMAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_RIE_POS,
                DMA_CH_IER_RIE_LEN, 0);
        dma_ch_ier = FXGMAC_SET_REG_BITS(
            dma_ch_ier, DMA_CH_IER_NIE_POS,
            DMA_CH_IER_NIE_LEN, 0);
        break;
    case FXGMAC_INT_DMA_CH_SR_FBE:
        dma_ch_ier = FXGMAC_SET_REG_BITS(
                dma_ch_ier, DMA_CH_IER_FBEE_POS,
                DMA_CH_IER_FBEE_LEN, 0);
        break;
    case FXGMAC_INT_DMA_ALL:
        channel->saved_ier = dma_ch_ier & FXGMAC_DMA_INTERRUPT_MASK;
        dma_ch_ier &= ~FXGMAC_DMA_INTERRUPT_MASK;
        break;
    default:
        return -1;
    }

    writereg(channel->pdata->pAdapter, dma_ch_ier, FXGMAC_DMA_REG(channel, DMA_CH_IER));

    return 0;
}

#ifdef LINUX
static int fxgmac_dismiss_DMA_int(struct fxgmac_channel *channel, int int_id)
{
    unsigned int dma_ch_ier;

    int_id = int_id;
    dma_ch_ier = readreg(channel->pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_SR /*1160*/));
    writereg(channel->pdata->pAdapter, dma_ch_ier, FXGMAC_DMA_REG(channel, DMA_CH_SR));

    return 0;
}

static void fxgmac_dismiss_MTL_Q_int(struct fxgmac_pdata *pdata)
{
    unsigned int q_count, i;
    unsigned int mtl_q_isr;

    q_count = max(pdata->hw_feat.tx_q_cnt, pdata->hw_feat.rx_q_cnt);
    for (i = 0; i < q_count; i++) {
        /* Clear all the interrupts which are set */
        mtl_q_isr = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, i, MTL_Q_ISR));
        writereg(pdata->pAdapter, mtl_q_isr, FXGMAC_MTL_REG(pdata, i, MTL_Q_ISR));
    }
}

static int fxgmac_dismiss_MAC_int(struct fxgmac_pdata *pdata)
{
    u32 regval,regErrVal;

    /* all MAC interrupts in 0xb0 */
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_ISR);
    /* MAC tx/rx error interrupts in 0xb8 */
    regErrVal = readreg(pdata->pAdapter, pdata->mac_regs + MAC_TX_RX_STA);
#if 0 //write clear
    if(FXGMAC_GET_REG_BITS(readreg(pdata->mac_regs + MAC_CSR_SW_CTRL),
                            0/*rcwe*/,
                            1))
    {
        writereg(regval, pdata->mac_regs + MAC_ISR);
        writereg(regErrVal, pdata->mac_regs + MAC_TX_RX_STA);
    }
#endif
    return 0;
}

static int fxgmac_dismiss_MAC_PMT_int(struct fxgmac_pdata *pdata)
{
    u32 regval;

    /* MAC PMT interrupts in 0xc0 */
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_PMT_STA);
#if 0 //write clear
    if(FXGMAC_GET_REG_BITS(readreg(pdata->mac_regs + MAC_CSR_SW_CTRL),
                            0/*rcwe*/,
                            1))
    {
        writereg(regval, pdata->mac_regs + MAC_PMT_STA);
    }

#endif
    return 0;
}

static int fxgmac_dismiss_MAC_LPI_int(struct fxgmac_pdata *pdata)
{
    u32 regval;

    /* MAC PMT interrupts in 0xc0 */
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_LPI_STA);
#if 0 //write clear
    if(FXGMAC_GET_REG_BITS(readreg(pdata->mac_regs + MAC_CSR_SW_CTRL),
                            0/*rcwe*/,
                            1))
    {
        writereg(regval, pdata->mac_regs + MAC_LPI_STA);
    }

#endif
    return 0;
}

static int fxgmac_dismiss_MAC_DBG_int(struct fxgmac_pdata *pdata)
{
    u32 regval;

    /* MAC PMT interrupts in 0xc0 */
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_DBG_STA);
#if 1 //write clear
    {
        writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_DBG_STA);
    }

#endif
    return 0;
}
#endif

int fxgmac_dismiss_all_int(struct fxgmac_pdata *pdata)
{
#ifdef LINUX
    struct fxgmac_channel *channel;
    unsigned int i, regval;
    struct net_device *netdev = pdata->netdev;

    if (netif_msg_drv(pdata)) { DPRINTK("fxgmac_dismiss_all_int callin\n"); }

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        fxgmac_dismiss_DMA_int(channel, 0);
    }
    fxgmac_dismiss_MTL_Q_int(pdata);
    fxgmac_dismiss_MAC_int(pdata);
    fxgmac_dismiss_MAC_PMT_int(pdata);
    fxgmac_dismiss_MAC_LPI_int(pdata);
    fxgmac_dismiss_MAC_DBG_int(pdata);

    //control module int to PCIe slot
    if (netdev->base_addr) {
        regval = (unsigned int)(* ((u32 *)(netdev->base_addr + MGMT_INT_CTRL0)));
    }
#else
    pdata = pdata;
#endif
    return 0;
}

static void fxgmac_set_interrupt_moderation(struct fxgmac_pdata* pdata)
{
    u32 value = 0, time;
#if defined (UEFI)
    pdata->intr_mod_timer = INT_MOD_IN_US;
#elif defined (_WIN32) || defined (_WIN64)
    // the Windows driver initializes it somewhere else
#else
    pdata->intr_mod_timer = INT_MOD_IN_US;
#endif

    time = (pdata->intr_mod) ? pdata->intr_mod_timer : 0;
#ifdef LINUX
    time = (pdata->intr_mod) ? pdata->tx_usecs : 0;
#endif
    value = FXGMAC_SET_REG_BITS(value, INT_MOD_TX_POS, INT_MOD_TX_LEN, time);

#ifdef LINUX
    time = (pdata->intr_mod) ? pdata->rx_usecs : 0;
#endif

    value = FXGMAC_SET_REG_BITS(value, INT_MOD_RX_POS, INT_MOD_RX_LEN, time);
    writereg(pdata->pAdapter, value, pdata->base_mem + INT_MOD);
}
static void  fxgmac_enable_msix_rxtxinterrupt(struct fxgmac_pdata* pdata)
{
    u32   intid;

    for (intid = 0; intid < MSIX_TBL_RXTX_NUM; intid++) {
        writereg(pdata->pAdapter, 0, pdata->base_mem + MSIX_TBL_BASE_ADDR + MSIX_TBL_MASK_OFFSET + intid * 16);
    }
}
static  void  fxgmac_disable_msix_interrupt(struct fxgmac_pdata* pdata)
{
    u32   intid;

    for (intid = 0; intid < MSIX_TBL_MAX_NUM; intid++) {
        writereg(pdata->pAdapter, 0x1, pdata->base_mem + MSIX_TBL_BASE_ADDR + MSIX_TBL_MASK_OFFSET + intid * 16);
    }
}
static void  fxgmac_enable_msix_rxtxphyinterrupt(struct fxgmac_pdata* pdata)
{
    u32                       intid, regval = 0;
#if !(FUXI_EPHY_INTERRUPT_D0_OFF)
    struct fxgmac_hw_ops* hw_ops = &pdata->hw_ops;
#endif

    for (intid = 0; intid < MSIX_TBL_RXTX_NUM; intid++) {
        writereg(pdata->pAdapter, 0, pdata->base_mem + MSIX_TBL_BASE_ADDR + MSIX_TBL_MASK_OFFSET + intid * 16);
    }
    writereg(pdata->pAdapter, 0, pdata->base_mem + MSIX_TBL_BASE_ADDR + MSIX_TBL_MASK_OFFSET + MSI_ID_PHY_OTHER * 16);
#if !(FUXI_EPHY_INTERRUPT_D0_OFF)
    hw_ops->read_ephy_reg(pdata, REG_MII_INT_STATUS, NULL);//  clear  phy interrupt
    regval = FXGMAC_SET_REG_BITS(0, PHY_INT_MASK_LINK_UP_POS, PHY_INT_MASK_LINK_UP_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, PHY_INT_MASK_LINK_DOWN_POS, PHY_INT_MASK_LINK_DOWN_LEN, 1);
    hw_ops->write_ephy_reg(pdata, REG_MII_INT_MASK, regval);//enable phy interrupt    ASIC bit10 linkup  bit11 linkdown
#endif
}
static void  fxgmac_enable_msix_one_interrupt(struct fxgmac_pdata* pdata,u32 intid)
{
    writereg(pdata->pAdapter, 0, pdata->base_mem + MSIX_TBL_BASE_ADDR + MSIX_TBL_MASK_OFFSET + intid * 16);
    //DbgPrintF(MP_LOUD, "%s - Ephy, MsgId %d is enabled.", __FUNCTION__, IntId);
}

static void  fxgmac_disable_msix_one_interrupt(struct fxgmac_pdata* pdata, u32 intid)
{
    writereg(pdata->pAdapter, 0x01, pdata->base_mem + MSIX_TBL_BASE_ADDR + MSIX_TBL_MASK_OFFSET + intid * 16);
    //DbgPrintF(MP_LOUD, "%s - Ephy, MsgId %d is disabled.", __FUNCTION__, IntId);
}

static  bool fxgmac_enable_mgm_interrupt(struct fxgmac_pdata* pdata)
{
    writereg(pdata->pAdapter, 0xf0000000, pdata->base_mem + MGMT_INT_CTRL0);
    return true;
}

static  bool fxgmac_disable_mgm_interrupt(struct fxgmac_pdata* pdata)
{
    writereg(pdata->pAdapter, 0xffff0000, pdata->base_mem + MGMT_INT_CTRL0);
    return true;
}

static int fxgmac_flush_tx_queues(struct fxgmac_pdata *pdata)
{
    unsigned int i, count;
    u32 regval;

    for (i = 0; i < pdata->tx_q_count; i++) {
        regval = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
        regval = FXGMAC_SET_REG_BITS(regval, MTL_Q_TQOMR_FTQ_POS,
                         MTL_Q_TQOMR_FTQ_LEN, 1);
        writereg(pdata->pAdapter, regval, FXGMAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
        DPRINTK("fxgmac_flush_tx_queues, reg=0x%p, val=0x%08x\n", FXGMAC_MTL_REG(pdata, i, MTL_Q_TQOMR), regval);
    }

    //2022-04-20 xiaojiang comment
    //the following windows implement has some unreasonable part
    //Take Linux implement method instead
    /* Poll Until Poll Condition */
    /*for (i = 0; i < pdata->tx_q_count; i++) {
        count = 2000;
        regval = readreg(FXGMAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
        regval = FXGMAC_GET_REG_BITS(regval, MTL_Q_TQOMR_FTQ_POS,
                         MTL_Q_TQOMR_FTQ_LEN);
        while (--count && regval) {
            usleep_range(pdata->pAdapter, 500, 600);
        }

        DPRINTK("fxgmac_flush_tx_queues wait... reg=0x%p, val=0x%08x\n", FXGMAC_MTL_REG(pdata, i, MTL_Q_TQOMR), regval);

        if (!count)
            return -EBUSY;
    }*/
    for (i = 0; i < pdata->tx_q_count; i++) {
        count = 2000;
        //regval = 1; //reset is not cleared....
        do {
            usleep_range_ex(pdata->pAdapter, 40, 50);
            regval = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, i, MTL_Q_TQOMR));
            regval = FXGMAC_GET_REG_BITS(regval, MTL_Q_TQOMR_FTQ_POS,
                             MTL_Q_TQOMR_FTQ_LEN);

        } while (--count && regval);
        DPRINTK("fxgmac_flush_tx_queues wait... reg=0x%p, val=0x%08x\n", FXGMAC_MTL_REG(pdata, i, MTL_Q_TQOMR), regval);
        if (regval) {/*(!count)*/
            return -EBUSY;
        }
    }

    return 0;
}

static void fxgmac_config_dma_bus(struct fxgmac_pdata *pdata)
{
    u32 regval;
#if 1//set no fix burst length
    regval = readreg(pdata->pAdapter, pdata->mac_regs + DMA_SBMR);
    /* Set enhanced addressing mode */
    regval = FXGMAC_SET_REG_BITS(regval, DMA_SBMR_EAME_POS,
        DMA_SBMR_EAME_LEN, 1);
    /* Set the System Bus mode */
    regval = FXGMAC_SET_REG_BITS(regval, DMA_SBMR_FB_POS,
        DMA_SBMR_FB_LEN, 0);
    regval = FXGMAC_SET_REG_BITS(regval, DMA_SBMR_BLEN_4_POS,
        DMA_SBMR_BLEN_4_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, DMA_SBMR_BLEN_8_POS,
        DMA_SBMR_BLEN_8_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, DMA_SBMR_BLEN_16_POS,
        DMA_SBMR_BLEN_16_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, DMA_SBMR_BLEN_32_POS,
        DMA_SBMR_BLEN_32_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + DMA_SBMR);

#else
    regval = readreg(pdata->mac_regs + DMA_SBMR);
    /* Set enhanced addressing mode */
    regval = FXGMAC_SET_REG_BITS(regval, DMA_SBMR_EAME_POS,
                     DMA_SBMR_EAME_LEN, 1);
    /* Set the System Bus mode */
    regval = FXGMAC_SET_REG_BITS(regval, DMA_SBMR_FB_POS,
                     DMA_SBMR_FB_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, DMA_SBMR_BLEN_256_POS,
                     DMA_SBMR_BLEN_256_LEN, 1);
    writereg(regval, pdata->mac_regs + DMA_SBMR);
#endif
}

static void fxgmac_legacy_link_speed_setting(struct fxgmac_pdata* pdata)
{
    unsigned int i = 0, regval = 0;

    fxgmac_phy_config(pdata);
    for (i = 0, regval = fxgmac_get_ephy_state(pdata);
        (!(regval & MGMT_EPHY_CTRL_STA_EPHY_RELEASE) || !(regval & MGMT_EPHY_CTRL_STA_EPHY_LINKUP)) && (i < PHY_LINK_TIMEOUT);
        regval = fxgmac_get_ephy_state(pdata), i++)
    {
        usleep_range_ex(pdata->pAdapter, 2000, 2000);
    }
    fxgmac_read_ephy_reg(pdata, REG_MII_INT_STATUS, NULL); // clear  phy interrupt.
}

static void fxgmac_pre_powerdown(struct fxgmac_pdata* pdata, bool phyloopback)
{
    unsigned int regval = 0;

    fxgmac_disable_rx(pdata);

    /* HERE, WE NEED TO CONSIDER PHY CONFIG...TBD */
    DPRINTK("fxgmac_config_powerdown, phy and mac status update\n");
    //2022-11-09 xiaojiang comment
    //for phy cable loopback,it can't configure phy speed, it will cause os resume again by link change although it has finished speed setting,
    if (!phyloopback) {

#ifndef LINUX
#if defined(UEFI)
#elif defined(DPDK)
    (void) i;
#elif defined(_WIN32) || defined(_WIN64)
        LONGLONG tick_interval;
        ULONG tick_inc;
        LARGE_INTEGER tick_count;
        unsigned int i = 0;
        if ((ULONG)pdata->phy_speed != ((PMP_ADAPTER)pdata->pAdapter)->usLinkSpeed)
        {
            DbgPrintF(MP_TRACE, "%s change phy speed", __FUNCTION__);

            if (((PMP_ADAPTER)pdata->pAdapter)->RegParameter.LinkChgWol)
            {
                fxgmac_phy_config(pdata);
                //sleep fixed value(6s)
                for (i = 0; i < PHY_LINK_TIMEOUT; i++)
                {
                    usleep_range_ex(pdata->pAdapter, 2000, 2000);
                }

                fxgmac_read_ephy_reg(pdata, REG_MII_INT_STATUS, NULL); // clear  phy interrupt.
            }
            else
            {
                regval = fxgmac_get_ephy_state(pdata);
                KeQueryTickCount(&tick_count);
                tick_inc = KeQueryTimeIncrement();
                tick_interval = tick_count.QuadPart - ((PMP_ADAPTER)pdata->pAdapter)->D0_entry_tick_count.QuadPart;
                tick_interval *= tick_inc;
                tick_interval /= 10;

                /*DbgPrintF(MP_TRACE, "base tick %lld", ((PMP_ADAPTER)pdata->pAdapter)->D0_entry_tick_count.QuadPart);
                DbgPrintF(MP_TRACE, "current tick %lld", tick_count.QuadPart);
                DbgPrintF(MP_TRACE, "tick inc is %u", tick_inc);
                DbgPrintF(MP_TRACE, "tick_interval is %lld", tick_interval);*/
                if (((regval & MGMT_EPHY_CTRL_STA_EPHY_RELEASE) && (regval & MGMT_EPHY_CTRL_STA_EPHY_LINKUP))
                     || ((regval & MGMT_EPHY_CTRL_STA_EPHY_RELEASE) && !(regval & MGMT_EPHY_CTRL_STA_EPHY_LINKUP) && (tick_interval < RESUME_MAX_TIME) && (MediaConnectStateConnected == ((PMP_ADAPTER)pdata->pAdapter)->PreMediaState))
                    )
                {
                    fxgmac_legacy_link_speed_setting(pdata);
                }
            }
        }
#else
#endif
#endif

        /*
        When the Linux platform enters the s4 state, it goes through the suspend->resume->suspend process.
        The process of suspending again after resume is fast, and PHY auto-negotiation is not yet complete,
        so the auto-negotiation of PHY must be carried out again.Windows platforms and UEFI platforms do
        not need to auto-negotiate again, as they will not have such a process.

        When the Linux platform enters the s4 state, force speed to 10M.
        */
#ifdef UEFI
        fxgmac_legacy_link_speed_setting(pdata);
#elif defined(_WIN32) || defined(_WIN64)

#elif defined(LINUX)
        pdata->phy_speed = SPEED_10;
        fxgmac_legacy_link_speed_setting(pdata);
#else
        fxgmac_legacy_link_speed_setting(pdata);
#endif
    }

    fxgmac_config_mac_speed(pdata);

    /* After enable OOB_WOL from efuse, mac will loopcheck phy status, and lead to panic sometimes.
    So we should disable it from powerup, enable it from power down.*/
    regval = (u32)readreg(pdata->pAdapter, pdata->base_mem + OOB_WOL_CTRL);
    regval = FXGMAC_SET_REG_BITS(regval, OOB_WOL_CTRL_DIS_POS, OOB_WOL_CTRL_DIS_LEN, 0);
    writereg(pdata->pAdapter, regval, pdata->base_mem + OOB_WOL_CTRL);
    usleep_range_ex(pdata->pAdapter, 2000, 2000);

    //after enable OOB_WOL,recofigure mac addr again
    fxgmac_set_mac_address(pdata, pdata->mac_addr);
    //fxgmac_suspend_clock_gate(pdata);
}

#if defined(LINUX)
// only supports four patterns, and patterns will be cleared on every call
static void fxgmac_set_pattern_data(struct fxgmac_pdata *pdata)
{
    u32                             ip_addr, i = 0;
    u8                              type_offset, op_offset, tip_offset;
    struct pattern_packet           packet;
    struct wol_bitmap_pattern       pattern[4]; // for WAKE_UCAST, WAKE_BCAST, WAKE_MCAST, WAKE_ARP.

    memset(pattern, 0, sizeof(struct wol_bitmap_pattern) * 4);

    //config ucast
    if (pdata->expansion.wol & WAKE_UCAST) {
        pattern[i].mask_info[0] = 0x3F;
        pattern[i].mask_size = sizeof(pattern[0].mask_info);
        memcpy(pattern[i].pattern_info, pdata->mac_addr, ETH_ALEN);
        pattern[i].pattern_offset = 0;
        i++;
    }

    // config bcast
    if (pdata->expansion.wol & WAKE_BCAST) {
        pattern[i].mask_info[0] = 0x3F;
        pattern[i].mask_size = sizeof(pattern[0].mask_info);
        memset(pattern[i].pattern_info, 0xFF, ETH_ALEN);
        pattern[i].pattern_offset = 0;
        i++;
    }

    // config mcast
    if (pdata->expansion.wol & WAKE_MCAST) {
        pattern[i].mask_info[0] = 0x7;
        pattern[i].mask_size = sizeof(pattern[0].mask_info);
        pattern[i].pattern_info[0] = 0x1;
        pattern[i].pattern_info[1] = 0x0;
        pattern[i].pattern_info[2] = 0x5E;
        pattern[i].pattern_offset = 0;
        i++;
    }

    // config arp
    if (pdata->expansion.wol & WAKE_ARP) {
        memset(pattern[i].mask_info, 0, sizeof(pattern[0].mask_info));
        type_offset = offsetof(struct pattern_packet, ar_pro);
        pattern[i].mask_info[type_offset / 8] |= 1 << type_offset % 8;
        type_offset++;
        pattern[i].mask_info[type_offset / 8] |= 1 << type_offset % 8;
        op_offset = offsetof(struct pattern_packet, ar_op);
        pattern[i].mask_info[op_offset / 8] |= 1 << op_offset % 8;
        op_offset++;
        pattern[i].mask_info[op_offset / 8] |= 1 << op_offset % 8;
        tip_offset = offsetof(struct pattern_packet, ar_tip);
        pattern[i].mask_info[tip_offset / 8] |= 1 << tip_offset % 8;
        tip_offset++;
        pattern[i].mask_info[tip_offset / 8] |= 1 << type_offset % 8;
        tip_offset++;
        pattern[i].mask_info[tip_offset / 8] |= 1 << type_offset % 8;
        tip_offset++;
        pattern[i].mask_info[tip_offset / 8] |= 1 << type_offset % 8;

        packet.ar_pro = 0x0 << 8 | 0x08; // arp type is 0x0800, notice that ar_pro and ar_op is big endian
        packet.ar_op = 0x1 << 8; // 1 is arp request,2 is arp replay, 3 is rarp request, 4 is rarp replay
        ip_addr = fxgmac_get_netdev_ip4addr(pdata);
        packet.ar_tip[0] = ip_addr & 0xFF;
        packet.ar_tip[1] = (ip_addr >> 8) & 0xFF;
        packet.ar_tip[2] = (ip_addr >> 16) & 0xFF;
        packet.ar_tip[3] = (ip_addr >> 24) & 0xFF;
        memcpy(pattern[i].pattern_info, &packet, MAX_PATTERN_SIZE);
        pattern[i].mask_size = sizeof(pattern[0].mask_info);
        pattern[i].pattern_offset = 0;
        i++;
    }

    fxgmac_set_wake_pattern(pdata, pattern, i);
}

static void fxgmac_config_powerdown(struct fxgmac_pdata *pdata, unsigned int wol)
#else
static void fxgmac_config_powerdown(struct fxgmac_pdata* pdata, unsigned int offloadcount, bool magic_en, bool remote_pattern_en)
#endif
{
    u32 regval = 0;

    fxgmac_disable_tx(pdata);
    fxgmac_disable_rx(pdata);

    /* performs fxgmac power down sequence
     * 1. set led
     * 2. check wol.
     * 3. check arp offloading
     * 4. disable gmac rx
     * 5. set gmac power down
     */

     //Close LED when entering the S3,S4,S5 except solution3
    fxgmac_efuse_read_data(pdata, EFUSE_LED_ADDR, &regval);
    //led index use bit0~bit5
    regval = FXGMAC_GET_REG_BITS(regval, EFUSE_LED_POS, EFUSE_LED_LEN);
    if (EFUSE_LED_COMMON_SOLUTION != regval) {
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED0_CFG);
        if (EFUSE_LED_SOLUTION3 == regval) {
            fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, REG_MII_EXT_COMMON_LED0_CFG_VALUE_SLEEP_SOLUTION3);
        }
        else {
            fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, 0x00);
        }

        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED1_CFG);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, 0x00);

        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_COMMON_LED2_CFG);
        fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, 0x00);
    }

#if defined(LINUX)
    if(!test_bit(FXGMAC_POWER_STATE_DOWN, &pdata->expansion.powerstate)) {
        netdev_err(pdata->netdev,
            "fxgmac powerstate is %lu when config power to down.\n", pdata->expansion.powerstate);
    }

#if FXGMAC_WOL_FEATURE_ENABLED
    fxgmac_config_wol(pdata, wol);
#endif
#if FXGMAC_AOE_FEATURE_ENABLED
    /* use default arp offloading feature */
    fxgmac_update_aoe_ipv4addr(pdata, (u8 *)NULL);
    fxgmac_enable_arp_offload(pdata);
#endif

#if FXGMAC_NS_OFFLOAD_ENABLED
    /* pls do not change the seq below */
    fxgmac_update_ns_offload_ipv6addr(pdata, FXGMAC_NS_IFA_GLOBAL_UNICAST);
    fxgmac_update_ns_offload_ipv6addr(pdata, FXGMAC_NS_IFA_LOCAL_LINK);
    fxgmac_enable_ns_offload(pdata);
#endif
#endif // LINUX.

#if defined(_WIN32) || defined(_WIN64)
    fxgmac_enable_wake_packet_indication(pdata, 1);
#endif
    /* Enable MAC Rx TX */
#if defined(LINUX)
    if (1) {
#else
    if (magic_en || remote_pattern_en || offloadcount) {
#endif
        regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_CR);
        regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_RE_POS, MAC_CR_RE_LEN, 1);
#if defined(LINUX)
        if(pdata->hw_feat.aoe) {
#else
        if (offloadcount) {
#endif
            regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_TE_POS, MAC_CR_TE_LEN, 1);
        }
        writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_CR);
    }

    /* Enable fast link mode. - ECO to fix it.*/
#if 0
    cfg_r32(pdata, REG_POWER_EIOS, &regval);
    regval = FXGMAC_SET_REG_BITS(regval, POWER_EIOS_POS, POWER_EIOS_LEN, 1);
    cfg_w32(pdata, REG_POWER_EIOS, regval);
#endif

    regval = readreg(pdata->pAdapter, pdata->base_mem + LPW_CTRL);
    regval = FXGMAC_SET_REG_BITS(regval, LPW_CTRL_ASPM_LPW_EN_POS, LPW_CTRL_ASPM_LPW_EN_LEN, 1); // Enable PCIE PM_L23.
#if 0
    regval = FXGMAC_SET_REG_BITS(regval, LPW_CTRL_ASPM_L0S_EN_POS, LPW_CTRL_ASPM_L0S_EN_LEN, 0);
    regval = FXGMAC_SET_REG_BITS(regval, LPW_CTRL_ASPM_L1_EN_POS, LPW_CTRL_ASPM_L1_EN_LEN, 0);
    regval = FXGMAC_SET_REG_BITS(regval, LPW_CTRL_L1SS_EN_POS, LPW_CTRL_L1SS_EN_LEN, 0);
#endif
    writereg(pdata->pAdapter, regval, pdata->base_mem + LPW_CTRL);

    /* set gmac power down */
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_PMT_STA);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_PMT_STA_PWRDWN_POS, MAC_PMT_STA_PWRDWN_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_PMT_STA);

#ifdef LINUX
    /*adjust sigdet threshold*/
    //redmine.motor-comm.com/issues/5093
    // fix issue can not wake up os on some FT-D2000 platform, this modification is only temporary
    // if it is 55mv, wol maybe failed.

    regval = readreg(pdata->pAdapter, pdata->base_mem + MGMT_SIGDET);
    regval = FXGMAC_SET_REG_BITS(regval, MGMT_SIGDET_POS, MGMT_SIGDET_LEN, MGMT_SIGDET_40MV);
    writereg(pdata->pAdapter, regval, pdata->base_mem + MGMT_SIGDET);
#endif
    DPRINTK("fxgmac_config_powerdown callout, reg=0x%08x\n", regval);
}

static void fxgmac_config_powerup(struct fxgmac_pdata* pdata)
{
    u32 regval = 0;

#if defined(LINUX)
    if (test_bit(FXGMAC_POWER_STATE_DOWN, &pdata->expansion.powerstate)) {
        netdev_err(pdata->netdev, "fxgmac powerstate is %lu when config power to up.\n", pdata->expansion.powerstate);
    }
#endif

    /* After enable OOB_WOL from efuse, mac will loopcheck phy status, and lead to panic sometimes.
        So we should disable it from powerup, enable it from power down.*/
    regval = (u32)readreg(pdata->pAdapter, pdata->base_mem + OOB_WOL_CTRL);
    regval = FXGMAC_SET_REG_BITS(regval, OOB_WOL_CTRL_DIS_POS, OOB_WOL_CTRL_DIS_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->base_mem + OOB_WOL_CTRL);

    /* clear wpi mode whether or not waked by WOL, write reset value */
    regval = (u32)readreg(pdata->pAdapter, pdata->base_mem + MGMT_WPI_CTRL0);

    regval = FXGMAC_SET_REG_BITS(regval,
        MGMT_WPI_CTRL0_WPI_MODE_POS,
        MGMT_WPI_CTRL0_WPI_MODE_LEN, 0);
    writereg(pdata->pAdapter, regval, pdata->base_mem + MGMT_WPI_CTRL0);
    /* read pmt_status register to De-assert the pmt_intr_o */
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_PMT_STA);
    /* wether or not waked up by WOL, write reset value */
    regval = FXGMAC_SET_REG_BITS(regval, MAC_PMT_STA_PWRDWN_POS, MAC_PMT_STA_PWRDWN_LEN, 0);
    /* write register to synchronized always-on block */
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_PMT_STA);

    /* Disable fast link mode*/
    cfg_r32(pdata, REG_POWER_EIOS, &regval);
    regval = FXGMAC_SET_REG_BITS(regval, POWER_EIOS_POS, POWER_EIOS_LEN, 0);
    cfg_w32(pdata, REG_POWER_EIOS, regval);

    fxgmac_pwr_clock_gate(pdata);
}

#if FXGMAC_SANITY_CHECK_ENABLED
/*
 * fxgmac_diag_sanity_check
 * check if there is any error like tx q hang
 * return: 0 normal and other fatal error
 */
static int fxgmac_diag_sanity_check(struct fxgmac_pdata *pdata)
{
    u32 reg_q_val, reg_tail_val;
    static u32 reg_tail_pre = 0;
    static int cnt = 0;

    reg_q_val = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, 0/* tx channe 0 */, 0x8/*  0x2d08 */));
    if (!(reg_q_val & 0x10)) { //tx q is empty
        return 0;
    }
    reg_tail_val = readreg(pdata->pAdapter, FXGMAC_DMA_REG(pdata->channel_head, DMA_CH_TDTR_LO));
    if(reg_tail_pre != reg_tail_val) {
        reg_tail_pre = reg_tail_val;
        cnt = 0;
    } else {
        cnt++;
    }

    if(cnt > 10) {
        reg_q_val = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, 0/* tx channe 0 */, 0x8/*  0x2d08 */));
        if(reg_q_val & 0x10) { //double check
            DPRINTK("fxgmac, WARNing, tx Q status is 0x%x and tail keeps unchanged for %d times, 0x%x\n", reg_q_val, cnt, reg_tail_val);
            return 1;
        }
    }

    return 0;
}
#endif
static void fxgmac_pwr_clock_gate(struct fxgmac_pdata* pdata)
{
    u32    regval = 0;

    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_SLEEP_CONTROL1);
    fxgmac_read_ephy_reg(pdata, REG_MII_EXT_DATA, &regval);
    // close pll in sleep mode
    regval = FXGMAC_SET_REG_BITS(regval, MII_EXT_SLEEP_CONTROL1_PLLON_IN_SLP_POS,
        MII_EXT_SLEEP_CONTROL1_PLLON_IN_SLP_LEN, 0);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, regval);

    /*regval = readreg(pdata->pAdapter, pdata->base_mem + MGMT_XST_OSC_CTRL);
    regval = FXGMAC_SET_REG_BITS(regval, MGMT_XST_OSC_CTRL_XST_OSC_SEL_POS,
        MGMT_XST_OSC_CTRL_XST_OSC_SEL_LEN, 0);
    writereg(pdata->pAdapter, regval, pdata->base_mem + MGMT_XST_OSC_CTRL);
    regval = readreg(pdata->pAdapter, pdata->base_mem + MGMT_XST_OSC_CTRL);
    regval = FXGMAC_SET_REG_BITS(regval, MGMT_XST_OSC_CTRL_EN_XST_POS,
        MGMT_XST_OSC_CTRL_EN_XST_LEN, 0);
    writereg(pdata->pAdapter, regval, pdata->base_mem + MGMT_XST_OSC_CTRL);*/
}
static void fxgmac_pwr_clock_ungate(struct fxgmac_pdata* pdata)
{
    u32 regval = 0;

    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_ADDR, REG_MII_EXT_SLEEP_CONTROL1);
    fxgmac_read_ephy_reg(pdata, REG_MII_EXT_DATA, &regval);
    // keep pll in sleep mode
    regval = FXGMAC_SET_REG_BITS(regval, MII_EXT_SLEEP_CONTROL1_PLLON_IN_SLP_POS,
        MII_EXT_SLEEP_CONTROL1_PLLON_IN_SLP_LEN, 1);
    fxgmac_write_ephy_reg(pdata, REG_MII_EXT_DATA, regval);

    /*regval = readreg(pdata->pAdapter, pdata->base_mem + MGMT_XST_OSC_CTRL);
    regval = FXGMAC_SET_REG_BITS(regval, MGMT_XST_OSC_CTRL_EN_XST_POS,
        MGMT_XST_OSC_CTRL_EN_XST_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->base_mem + MGMT_XST_OSC_CTRL);
    regval = readreg(pdata->pAdapter, pdata->base_mem + MGMT_XST_OSC_CTRL);
    regval = FXGMAC_SET_REG_BITS(regval, MGMT_XST_OSC_CTRL_XST_OSC_SEL_POS,
        MGMT_XST_OSC_CTRL_XST_OSC_SEL_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->base_mem + MGMT_XST_OSC_CTRL);*/
}
static unsigned char fxgmac_suspend_int(void* context)
// context - pointer to struct nic_pdata.
{
    //ULONG_PTR addr;
    u32   intid;
#if FUXI_EPHY_INTERRUPT_D0_OFF
    u32   regval = 0;
#endif
    u32 val_mgmt_intcrtl0;
    struct fxgmac_pdata* pdata = (struct fxgmac_pdata*)context;

    val_mgmt_intcrtl0 = (u32)readreg(pdata->pAdapter, pdata->base_mem + MGMT_INT_CTRL0);
    //disable  management interrupts. enable only pmt interrupts.
    val_mgmt_intcrtl0 = FXGMAC_SET_REG_BITS(val_mgmt_intcrtl0, MGMT_INT_CTRL0_INT_MASK_POS,
        MGMT_INT_CTRL0_INT_MASK_LEN,
        MGMT_INT_CTRL0_INT_MASK_EX_PMT);
    writereg(pdata->pAdapter, val_mgmt_intcrtl0, pdata->base_mem + MGMT_INT_CTRL0);

    for (intid = 0; intid < MSIX_TBL_MAX_NUM; intid++) { // disable  all  msix
        writereg(pdata->pAdapter, 0x1, pdata->base_mem + MSIX_TBL_BASE_ADDR + MSIX_TBL_MASK_OFFSET + intid * 16);
    }

    //enable  pmt  msix
    writereg(pdata->pAdapter, 0x0, pdata->base_mem + MSIX_TBL_BASE_ADDR + MSIX_TBL_MASK_OFFSET + MSI_ID_PHY_OTHER * 16);
    readreg(pdata->pAdapter, pdata->base_mem + MGMT_WOL_CTRL);//  read  clear  wake up  reason
    //since Msix interrupt masked now, enable EPHY interrupt for case of link change wakeup
    fxgmac_read_ephy_reg(pdata, REG_MII_INT_STATUS, NULL);	//	clear  phy interrupt
#if FUXI_EPHY_INTERRUPT_D0_OFF
    regval = FXGMAC_SET_REG_BITS(0, PHY_INT_MASK_LINK_UP_POS, PHY_INT_MASK_LINK_UP_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, PHY_INT_MASK_LINK_DOWN_POS, PHY_INT_MASK_LINK_DOWN_LEN, 1);
    fxgmac_write_ephy_reg(pdata, REG_MII_INT_MASK, regval);//enable phy interrupt
#endif

    return true;
}
static int fxgmac_suspend_txrx(struct fxgmac_pdata* pdata)
{
    struct fxgmac_channel* channel;
    unsigned int i;
    u32 regval;
    int busy = 15;
    /* Prepare for Tx DMA channel stop */
    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->tx_ring) {
            break;
        }
        fxgmac_prepare_tx_stop(pdata, channel);
    }

    /* Disable each Tx DMA channel */
    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->tx_ring) {
            break;
        }

        regval = readreg(pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_TCR));
        regval = FXGMAC_SET_REG_BITS(regval, DMA_CH_TCR_ST_POS,
            DMA_CH_TCR_ST_LEN, 0);
        writereg(pdata->pAdapter, regval, FXGMAC_DMA_REG(channel, DMA_CH_TCR));
        DBGPRINT(MP_TRACE, (" %s disable tx  dma", __FUNCTION__));
    }

    do {
        regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_DBG_STA);
        busy--;
    } while ((regval & MAC_DBG_STA_TX_BUSY) && (busy));

    if (0 != (regval & MAC_DBG_STA_TX_BUSY)) {
        regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_DBG_STA);
        DbgPrintF(MP_WARN, "warning !!!timed out waiting for Tx MAC to stop\n");
        return -1;
    }
    /* wait  empty Tx queue */
    for (i = 0; i < pdata->tx_q_count; i++) {
        do {

            regval = readreg(pdata->pAdapter, FXGMAC_MTL_REG(pdata, i, MTL_TXQ_DEG));
            busy--;
        } while ((regval & MTL_TXQ_DEG_TX_BUSY) && (busy));
        if (0 != (regval & MTL_TXQ_DEG_TX_BUSY)) {
            regval = readreg(pdata->pAdapter, pdata->mac_regs + MTL_TXQ_DEG);
            DbgPrintF(MP_WARN, "warning !!!timed out waiting for tx queue %u to empty\n",
                i);
            return -1;
        }
    }

    /* Disable MAC TxRx */
    regval = readreg(pdata->pAdapter, pdata->mac_regs + MAC_CR);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_TE_POS,
        MAC_CR_TE_LEN, 0);
    regval = FXGMAC_SET_REG_BITS(regval, MAC_CR_RE_POS,
        MAC_CR_RE_LEN, 0);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + MAC_CR);

    /* Prepare for Rx DMA channel stop */
    for (i = 0; i < pdata->rx_q_count; i++) {
        fxgmac_prepare_rx_stop(pdata, i);
    }
        /* Disable each Rx DMA channel */
    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
        if (!channel->rx_ring) {
            break;
        }

        regval = readreg(pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_RCR));
        regval = FXGMAC_SET_REG_BITS(regval, DMA_CH_RCR_SR_POS,
                         DMA_CH_RCR_SR_LEN, 0);
        writereg(pdata->pAdapter, regval, FXGMAC_DMA_REG(channel, DMA_CH_RCR));
        DBGPRINT(MP_TRACE, (" %s disable rx  dma", __FUNCTION__));
    }
    return 0;
}
static void fxgmac_resume_int(struct fxgmac_pdata* pdata)
{
    u32   intid, regval = 0;
    u32 val_mgmt_intcrtl0;

    val_mgmt_intcrtl0 = (u32)readreg(pdata->pAdapter, pdata->base_mem + MGMT_INT_CTRL0);
    //disable  management interrupts. enable only pmt interrupts.
    val_mgmt_intcrtl0 = FXGMAC_SET_REG_BITS(val_mgmt_intcrtl0, MGMT_INT_CTRL0_INT_MASK_POS,
        MGMT_INT_CTRL0_INT_MASK_LEN,
        MGMT_INT_CTRL0_INT_MASK_DISABLE);
    writereg(pdata->pAdapter, val_mgmt_intcrtl0, pdata->base_mem + MGMT_INT_CTRL0);

    for (intid = 0; intid < MSIX_TBL_RXTX_NUM; intid++) {
        writereg(pdata->pAdapter, 0, pdata->base_mem + MSIX_TBL_BASE_ADDR + MSIX_TBL_MASK_OFFSET + intid * 16);
    }

    for (intid = MSIX_TBL_RXTX_NUM; intid < MSIX_TBL_MAX_NUM; intid++) { // disable  some  msix
        writereg(pdata->pAdapter, 0, pdata->base_mem + MSIX_TBL_BASE_ADDR + MSIX_TBL_MASK_OFFSET + intid * 16);
    }

#if FUXI_EPHY_INTERRUPT_D0_OFF
    fxgmac_write_ephy_reg(pdata, REG_MII_INT_MASK,0x0);     //disable phy interrupt
    fxgmac_read_ephy_reg(pdata, REG_MII_INT_STATUS, NULL);      //  clear  phy interrupt
#else
    //hw_ops->read_ephy_reg(pdata, REG_MII_INT_STATUS, NULL);//     clear  phy interrupt
    regval = FXGMAC_SET_REG_BITS(0, PHY_INT_MASK_LINK_UP_POS, PHY_INT_MASK_LINK_UP_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, PHY_INT_MASK_LINK_DOWN_POS, PHY_INT_MASK_LINK_DOWN_LEN, 1);
    fxgmac_write_ephy_reg(pdata, REG_MII_INT_MASK, regval);//enable phy interrupt
#endif
}

static int fxgmac_hw_init(struct fxgmac_pdata *pdata)
{
    struct fxgmac_desc_ops *desc_ops = &pdata->desc_ops;
    int ret;
    u32 regval = 0;

    if (netif_msg_drv(pdata)) { DPRINTK("fxgmac hw init call in\n"); }

    /* Flush Tx queues */
    ret = fxgmac_flush_tx_queues(pdata);
    if (ret) {
        if (netif_msg_drv(pdata)) { DPRINTK("fxgmac_hw_init call flush tx queue err.\n"); }
#ifdef LINUX
        return ret;
#endif
    }

    /* Initialize DMA related features */
    fxgmac_config_dma_bus(pdata);
    fxgmac_config_osp_mode(pdata);
    fxgmac_config_pblx8(pdata);
    fxgmac_config_tx_pbl_val(pdata);
    fxgmac_config_rx_pbl_val(pdata);
    fxgmac_config_rx_coalesce(pdata);
    fxgmac_config_tx_coalesce(pdata);
    fxgmac_config_rx_buffer_size(pdata);
    fxgmac_config_tso_mode(pdata);
    fxgmac_config_sph_mode(pdata);
    fxgmac_config_rss(pdata);

#ifdef LINUX
    fxgmac_config_wol(pdata, pdata->expansion.wol);
#endif

    desc_ops->tx_desc_init(pdata);
    desc_ops->rx_desc_init(pdata);
    fxgmac_enable_dma_interrupts(pdata);

    /* Initialize MTL related features */
    fxgmac_config_mtl_mode(pdata);
    fxgmac_config_queue_mapping(pdata);
    fxgmac_config_tsf_mode(pdata, pdata->tx_sf_mode);
    fxgmac_config_rsf_mode(pdata, pdata->rx_sf_mode);
    fxgmac_config_tx_threshold(pdata, pdata->tx_threshold);
    fxgmac_config_rx_threshold(pdata, pdata->rx_threshold);
    fxgmac_config_tx_fifo_size(pdata);
    fxgmac_config_rx_fifo_size(pdata);
    fxgmac_config_flow_control_threshold(pdata);
    fxgmac_config_rx_fep_disable(pdata);
    fxgmac_config_rx_fup_enable(pdata);
    fxgmac_enable_mtl_interrupts(pdata);

    /* Initialize MAC related features */
    fxgmac_config_mac_address(pdata);
    fxgmac_config_crc_check(pdata);
    fxgmac_config_rx_mode(pdata);
    fxgmac_config_jumbo(pdata);
    fxgmac_config_flow_control(pdata);
    fxgmac_config_mac_speed(pdata);
    fxgmac_config_checksum_offload(pdata);
    fxgmac_config_vlan_support(pdata);
    fxgmac_config_mmc(pdata);
    fxgmac_enable_mac_interrupts(pdata);

    /* enable EPhy link change interrupt */
    fxgmac_read_ephy_reg(pdata, REG_MII_INT_STATUS, NULL);//  clear  phy interrupt
    regval = FXGMAC_SET_REG_BITS(0, PHY_INT_MASK_LINK_UP_POS, PHY_INT_MASK_LINK_UP_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, PHY_INT_MASK_LINK_DOWN_POS, PHY_INT_MASK_LINK_DOWN_LEN, 1);
    fxgmac_write_ephy_reg(pdata, REG_MII_INT_MASK, regval);//enable phy interrupt

    if (netif_msg_drv(pdata)) { DPRINTK("fxgmac hw init callout\n"); }
    return 0;
}

static void fxgmac_save_nonstick_reg(struct fxgmac_pdata* pdata)
{
    u32     i;
    for (i = REG_PCIE_TRIGGER; i < MSI_PBA_REG; i += 4) {
        pdata->reg_nonstick[(i - REG_PCIE_TRIGGER) >> 2] = readreg(pdata->pAdapter, pdata->base_mem + i);
    }

#if defined(UBOOT)
    /* PCI config space info */
    dm_pci_read_config16(pdata->pdev, PCI_VENDOR_ID, &pdata->expansion.pci_venid);
    dm_pci_read_config16(pdata->pdev, PCI_DEVICE_ID, &pdata->expansion.pci_devid);
    dm_pci_read_config16(pdata->pdev, PCI_SUBSYSTEM_VENDOR_ID, &pdata->expansion.SubVendorID);
    dm_pci_read_config16(pdata->pdev, PCI_SUBSYSTEM_ID, &pdata->expansion.SubSystemID);

    dm_pci_read_config8(pdata->pdev, PCI_REVISION_ID, &pdata->expansion.pci_revid);
    //dm_pci_read_config16(pdata->pdev, PCI_COMMAND, &pdata->pci_cmd_word);

    DbgPrintF(MP_TRACE, "VenId is %x, Devid is %x, SubId is %x, SubSysId is %x, Revid is %x.\n",
             pdata->expansion.pci_venid, pdata->expansion.pci_devid, pdata->expansion.SubVendorID,
             pdata->expansion.SubSystemID, pdata->expansion.pci_revid);

#elif !defined(UEFI) && !defined(PXE) && !defined(DPDK)
    cfg_r32(pdata, REG_PCI_COMMAND, &pdata->expansion.cfg_pci_cmd);
    cfg_r32(pdata, REG_CACHE_LINE_SIZE, &pdata->expansion.cfg_cache_line_size);
    cfg_r32(pdata, REG_MEM_BASE, &pdata->expansion.cfg_mem_base);
    cfg_r32(pdata, REG_MEM_BASE_HI, &pdata->expansion.cfg_mem_base_hi);
    cfg_r32(pdata, REG_IO_BASE, &pdata->expansion.cfg_io_base);
    cfg_r32(pdata, REG_INT_LINE, &pdata->expansion.cfg_int_line);
    cfg_r32(pdata, REG_DEVICE_CTRL1, &pdata->expansion.cfg_device_ctrl1);
    cfg_r32(pdata, REG_PCI_LINK_CTRL, &pdata->expansion.cfg_pci_link_ctrl);
    cfg_r32(pdata, REG_DEVICE_CTRL2, &pdata->expansion.cfg_device_ctrl2);
    cfg_r32(pdata, REG_MSIX_CAPABILITY, &pdata->expansion.cfg_msix_capability);

    DbgPrintF(MP_TRACE, "%s:\nCFG%02x-%02x\nCFG%02x-%02x\nCFG%02x-%02x\nCFG%02x-%02x\nCFG%02x-%02x\nCFG%02x-%02x\nCFG%02x-%02x\nCFG%02x-%02x\nCFG%02x-%02x\nCFG%02x-%02x\n",
        __FUNCTION__,
        REG_PCI_COMMAND, pdata->expansion.cfg_pci_cmd,
        REG_CACHE_LINE_SIZE, pdata->expansion.cfg_cache_line_size,
        REG_MEM_BASE, pdata->expansion.cfg_mem_base,
        REG_MEM_BASE_HI, pdata->expansion.cfg_mem_base_hi,
        REG_IO_BASE, pdata->expansion.cfg_io_base,
        REG_INT_LINE, pdata->expansion.cfg_int_line,
        REG_DEVICE_CTRL1, pdata->expansion.cfg_device_ctrl1,
        REG_PCI_LINK_CTRL, pdata->expansion.cfg_pci_link_ctrl,
        REG_DEVICE_CTRL2, pdata->expansion.cfg_device_ctrl2,
        REG_MSIX_CAPABILITY, pdata->expansion.cfg_msix_capability);
#endif
}

static void fxgmac_restore_nonstick_reg(struct fxgmac_pdata* pdata)
{
    u32     i;
    for (i = REG_PCIE_TRIGGER; i < MSI_PBA_REG; i += 4) {
        writereg(pdata->pAdapter, pdata->reg_nonstick[(i - REG_PCIE_TRIGGER) >> 2], pdata->base_mem + i);
    }
}

#if !defined(UEFI) && !defined(PXE) && !defined(DPDK)
static void fxgmac_esd_restore_pcie_cfg(struct fxgmac_pdata* pdata)
{
    cfg_w32(pdata, REG_PCI_COMMAND, pdata->expansion.cfg_pci_cmd);
    cfg_w32(pdata, REG_CACHE_LINE_SIZE, pdata->expansion.cfg_cache_line_size);
    cfg_w32(pdata, REG_MEM_BASE, pdata->expansion.cfg_mem_base);
    cfg_w32(pdata, REG_MEM_BASE_HI, pdata->expansion.cfg_mem_base_hi);
    cfg_w32(pdata, REG_IO_BASE, pdata->expansion.cfg_io_base);
    cfg_w32(pdata, REG_INT_LINE, pdata->expansion.cfg_int_line);
    cfg_w32(pdata, REG_DEVICE_CTRL1, pdata->expansion.cfg_device_ctrl1);
    cfg_w32(pdata, REG_PCI_LINK_CTRL, pdata->expansion.cfg_pci_link_ctrl);
    cfg_w32(pdata, REG_DEVICE_CTRL2, pdata->expansion.cfg_device_ctrl2);
    cfg_w32(pdata, REG_MSIX_CAPABILITY, pdata->expansion.cfg_msix_capability);
}
#endif

static int fxgmac_hw_exit(struct fxgmac_pdata *pdata)
{
    //unsigned int count = 2000;
    u32     regval;
    u32 value = 0;
#if 1
    cfg_r32(pdata, REG_PCI_LINK_CTRL, &regval);
    pdata->pcie_link_status = FXGMAC_GET_REG_BITS(regval, PCI_LINK_CTRL_ASPM_CONTROL_POS, PCI_LINK_CTRL_ASPM_CONTROL_LEN);
    if (PCI_LINK_CTRL_L1_STATUS == (pdata->pcie_link_status & 0x02))
    {
        regval = FXGMAC_SET_REG_BITS(regval, PCI_LINK_CTRL_ASPM_CONTROL_POS, PCI_LINK_CTRL_ASPM_CONTROL_LEN, 0);
        cfg_w32(pdata, REG_PCI_LINK_CTRL, regval);
    }

    /* Issue a CHIP reset */
    regval = readreg(pdata->pAdapter, pdata->base_mem + SYS_RESET_REG);
    DPRINTK("CHIP_RESET 0x%x\n", regval);
	/* reg152c  bit31 1->reset, self-clear, if read it again, it still set 1. */
    regval = FXGMAC_SET_REG_BITS(regval, SYS_RESET_POS, SYS_RESET_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->base_mem + SYS_RESET_REG);

    usleep_range_ex(pdata->pAdapter, 9000, 10000); //usleep_range_ex(pdata->pAdapter, 10, 15);

	/* reg152c reset will reset trigger circuit and reload efuse patch 0x1004=0x16,need to release ephy reset again */
    value = FXGMAC_SET_REG_BITS(value, MGMT_EPHY_CTRL_RESET_POS, MGMT_EPHY_CTRL_RESET_LEN, MGMT_EPHY_CTRL_STA_EPHY_RELEASE);
    writereg(pdata->pAdapter, value, pdata->base_mem + MGMT_EPHY_CTRL);
    usleep_range_ex(pdata->pAdapter, 100, 150);

    fxgmac_restore_nonstick_reg(pdata); // reset will clear nonstick registers.
#else
    /* Issue a software reset */
    regval = readreg(pdata->pAdapter, pdata->mac_regs + DMA_MR); DPRINTK("DMA_MR 0x%x\n", regval);
    regval = FXGMAC_SET_REG_BITS(regval, DMA_MR_SWR_POS,
        DMA_MR_SWR_LEN, 1);
    regval = FXGMAC_SET_REG_BITS(regval, DMA_MR_INTM_POS,
        DMA_MR_INTM_LEN, 0);
    writereg(pdata->pAdapter, regval, pdata->mac_regs + DMA_MR);

    usleep_range_ex(pdata->pAdapter, 10, 15);

    /* Poll Until Poll Condition */
    while (--count &&
        FXGMAC_GET_REG_BITS(readreg(pdata->pAdapter, pdata->mac_regs + DMA_MR),
            DMA_MR_SWR_POS, DMA_MR_SWR_LEN)) {
        usleep_range_ex(pdata->pAdapter, 500, 600);
    }

    DPRINTK("Soft reset count %d\n", count);
    if (!count) {
        return -EBUSY;
    }
#endif
    return 0;
}

#if defined(UEFI) || defined(PXE)
static int fxgmac_set_gmac_register(struct fxgmac_pdata* pdata, u32 address, unsigned int data)
#else
static int fxgmac_set_gmac_register(struct fxgmac_pdata* pdata, u8* address, unsigned int data)
#endif
{
#if defined(UEFI) || defined(PXE)
    if (address < (u32)(pdata->base_mem))
#else
    if (address < (u8*)(pdata->base_mem))
#endif
    {
        return -1;
    }
    writereg(pdata->pAdapter, data, address);
    return 0;
}

#if defined(UEFI) || defined(PXE)
static u32 fxgmac_get_gmac_register(struct fxgmac_pdata* pdata, u32 address)
#else
static u32 fxgmac_get_gmac_register(struct fxgmac_pdata* pdata, u8* address)
#endif
{
    u32 regval = 0;

#if defined(UEFI) || defined(PXE)
    if (address > (u32)(pdata->base_mem))
#else
    if (address > (u8*)(pdata->base_mem))
#endif
    {
        regval = readreg(pdata->pAdapter, address);
    }
    return regval;
}

static int fxgmac_pcie_init(struct fxgmac_pdata* pdata, bool ltr_en, bool aspm_l1ss_en, bool aspm_l1_en, bool aspm_l0s_en)
{
    //DbgPrintF(MP_TRACE, "%s ltr_en %d aspm_l1ss_en %d aspm_l1_en %d aspm_l0s_en %d", __FUNCTION__, ltr_en, aspm_l1ss_en, aspm_l1_en, aspm_l0s_en);
    u32 regval = 0;
    u32 deviceid = 0;

    cfg_r32(pdata, REG_PCI_LINK_CTRL, &regval);
    if (PCI_LINK_CTRL_L1_STATUS == (pdata->pcie_link_status & 0x02)
        && 0x00 == FXGMAC_GET_REG_BITS(regval, PCI_LINK_CTRL_ASPM_CONTROL_POS, PCI_LINK_CTRL_ASPM_CONTROL_LEN)
        )
    {
        regval = FXGMAC_SET_REG_BITS(regval, PCI_LINK_CTRL_ASPM_CONTROL_POS, PCI_LINK_CTRL_ASPM_CONTROL_LEN, pdata->pcie_link_status);
        cfg_w32(pdata, REG_PCI_LINK_CTRL, regval);
    }

    regval = FXGMAC_SET_REG_BITS(0, LTR_IDLE_ENTER_REQUIRE_POS, LTR_IDLE_ENTER_REQUIRE_LEN, LTR_IDLE_ENTER_REQUIRE);
    regval = FXGMAC_SET_REG_BITS(regval, LTR_IDLE_ENTER_SCALE_POS, LTR_IDLE_ENTER_SCALE_LEN, LTR_IDLE_ENTER_SCALE);
    regval = FXGMAC_SET_REG_BITS(regval, LTR_IDLE_ENTER_POS, LTR_IDLE_ENTER_LEN, LTR_IDLE_ENTER_USVAL);
    regval = (regval << 16) + regval; /* snoopy + non-snoopy */
    writereg(pdata->pAdapter, regval, pdata->base_mem + LTR_IDLE_ENTER);

    regval = 0;
    regval = FXGMAC_SET_REG_BITS(0, LTR_IDLE_EXIT_REQUIRE_POS, LTR_IDLE_EXIT_REQUIRE_LEN, LTR_IDLE_EXIT_REQUIRE);
    regval = FXGMAC_SET_REG_BITS(regval, LTR_IDLE_EXIT_SCALE_POS, LTR_IDLE_EXIT_SCALE_LEN, LTR_IDLE_EXIT_SCALE);
    regval = FXGMAC_SET_REG_BITS(regval, LTR_IDLE_EXIT_POS, LTR_IDLE_EXIT_LEN, LTR_IDLE_EXIT_USVAL);
    regval = (regval << 16) + regval; /* snoopy + non-snoopy */
    writereg(pdata->pAdapter, regval, pdata->base_mem + LTR_IDLE_EXIT);

    regval = readreg(pdata->pAdapter, pdata->base_mem + LTR_CTRL);
    if (ltr_en) {
        regval = FXGMAC_SET_REG_BITS(regval, LTR_CTRL_EN_POS, LTR_CTRL_EN_LEN, 1);
        regval = FXGMAC_SET_REG_BITS(regval, LTR_CTRL_IDLE_THRE_TIMER_POS, LTR_CTRL_IDLE_THRE_TIMER_LEN, LTR_CTRL_IDLE_THRE_TIMER_VAL);
    } else {
        regval = FXGMAC_SET_REG_BITS(regval, LTR_CTRL_EN_POS, LTR_CTRL_EN_LEN, 0);
    }
    writereg(pdata->pAdapter, regval, pdata->base_mem + LTR_CTRL);

    regval = readreg(pdata->pAdapter, pdata->base_mem + LPW_CTRL);
    regval = FXGMAC_SET_REG_BITS(regval, LPW_CTRL_ASPM_L0S_EN_POS, LPW_CTRL_ASPM_L0S_EN_LEN, aspm_l0s_en ? 1 : 0);
    regval = FXGMAC_SET_REG_BITS(regval, LPW_CTRL_ASPM_L1_EN_POS, LPW_CTRL_ASPM_L1_EN_LEN, aspm_l1_en ? 1 : 0);
    regval = FXGMAC_SET_REG_BITS(regval, LPW_CTRL_L1SS_EN_POS, LPW_CTRL_L1SS_EN_LEN, aspm_l1ss_en ? 1 : 0);
    writereg(pdata->pAdapter, regval, pdata->base_mem + LPW_CTRL);

    cfg_r32(pdata, REG_ASPM_CONTROL, &regval);
    regval = FXGMAC_SET_REG_BITS(regval, ASPM_L1_IDLE_THRESHOLD_POS, ASPM_L1_IDLE_THRESHOLD_LEN, ASPM_L1_IDLE_THRESHOLD_1US);
    cfg_w32(pdata, REG_ASPM_CONTROL, regval);

    regval = 0;
    regval = FXGMAC_SET_REG_BITS(regval, PCIE_SERDES_PLL_AUTOOFF_POS, PCIE_SERDES_PLL_AUTOOFF_LEN, 1);
    writereg(pdata->pAdapter, regval, pdata->base_mem + REG_PCIE_SERDES_PLL);

    /*fuxi nto adjust sigdet threshold*/
    cfg_r8(pdata, REG_PCI_REVID, &regval);
    cfg_r16(pdata, REG_PCI_DEVICE_ID, &deviceid);
    if (FUXI_REV_01 == regval && PCI_DEVICE_ID_FUXI == deviceid)
    {
        regval = readreg(pdata->pAdapter, pdata->base_mem + MGMT_SIGDET);
        regval = FXGMAC_SET_REG_BITS(regval, MGMT_SIGDET_POS, MGMT_SIGDET_LEN, MGMT_SIGDET_55MV);
        writereg(pdata->pAdapter, regval, pdata->base_mem + MGMT_SIGDET);
    }

    return 0;
}


static void fxgmac_trigger_pcie(struct fxgmac_pdata* pdata, u32 code)
{
    writereg(pdata->pAdapter, code, pdata->base_mem + REG_PCIE_TRIGGER);
}

void fxgmac_init_hw_ops(struct fxgmac_hw_ops *hw_ops)
{
    hw_ops->init = fxgmac_hw_init;
    hw_ops->exit = fxgmac_hw_exit;
    hw_ops->save_nonstick_reg = fxgmac_save_nonstick_reg;
    hw_ops->restore_nonstick_reg = fxgmac_restore_nonstick_reg;
#if !defined(UEFI) && !defined(PXE) && !defined(DPDK)
    hw_ops->esd_restore_pcie_cfg = fxgmac_esd_restore_pcie_cfg;
#endif

    hw_ops->set_gmac_register = fxgmac_set_gmac_register;
    hw_ops->get_gmac_register = fxgmac_get_gmac_register;

    hw_ops->tx_complete = fxgmac_tx_complete;
    hw_ops->enable_tx = fxgmac_enable_tx;
    hw_ops->disable_tx = fxgmac_disable_tx;
    hw_ops->enable_rx = fxgmac_enable_rx;
    hw_ops->disable_rx = fxgmac_disable_rx;
    hw_ops->enable_channel_rx = fxgmac_enable_channel_rx;

#ifdef LINUX
    hw_ops->dev_xmit = fxgmac_dev_xmit;
    hw_ops->dev_read = fxgmac_dev_read;
    hw_ops->config_tso = fxgmac_config_tso_mode;
#endif
    hw_ops->enable_int = fxgmac_enable_int;
    hw_ops->disable_int = fxgmac_disable_int;
    hw_ops->set_interrupt_moderation = fxgmac_set_interrupt_moderation;
    hw_ops->enable_msix_rxtxinterrupt = fxgmac_enable_msix_rxtxinterrupt;
    hw_ops->disable_msix_interrupt = fxgmac_disable_msix_interrupt;
    hw_ops->enable_msix_rxtxphyinterrupt = fxgmac_enable_msix_rxtxphyinterrupt;
    hw_ops->enable_msix_one_interrupt = fxgmac_enable_msix_one_interrupt;
    hw_ops->disable_msix_one_interrupt = fxgmac_disable_msix_one_interrupt;
    hw_ops->enable_mgm_interrupt = fxgmac_enable_mgm_interrupt;
    hw_ops->disable_mgm_interrupt = fxgmac_disable_mgm_interrupt;

    hw_ops->set_mac_address = fxgmac_set_mac_address;
    hw_ops->set_mac_hash = fxgmac_add_mac_addresses;
    hw_ops->config_rx_mode = fxgmac_config_rx_mode;
    hw_ops->enable_rx_csum = fxgmac_enable_rx_csum;
    hw_ops->disable_rx_csum = fxgmac_disable_rx_csum;

    /* For MII speed configuration */
    hw_ops->config_mac_speed = fxgmac_config_mac_speed;
    hw_ops->get_xlgmii_phy_status = fxgmac_check_phy_link;

    /* For descriptor related operation */
    hw_ops->tx_desc_init = fxgmac_tx_desc_init;
    hw_ops->rx_desc_init = fxgmac_rx_desc_init;
    hw_ops->tx_desc_reset = fxgmac_tx_desc_reset;
    hw_ops->rx_desc_reset = fxgmac_rx_desc_reset;
    hw_ops->is_last_desc = fxgmac_is_last_desc;
    hw_ops->is_context_desc = fxgmac_is_context_desc;
#ifdef LINUX
    hw_ops->tx_start_xmit = fxgmac_tx_start_xmit;
    hw_ops->set_pattern_data = fxgmac_set_pattern_data;
    hw_ops->config_wol = fxgmac_config_wol;
    hw_ops->get_rss_hash_key = fxgmac_read_rss_hash_key;
    hw_ops->write_rss_lookup_table = fxgmac_write_rss_lookup_table;
#if FXGMAC_SANITY_CHECK_ENABLED
    hw_ops->diag_sanity_check = fxgmac_diag_sanity_check;
#endif
#endif

    /* For Flow Control */
    hw_ops->config_tx_flow_control = fxgmac_config_tx_flow_control;
    hw_ops->config_rx_flow_control = fxgmac_config_rx_flow_control;

    /*For Jumbo Frames*/
    hw_ops->enable_jumbo = fxgmac_config_jumbo;

    /* For Vlan related config */
    hw_ops->enable_tx_vlan = fxgmac_enable_tx_vlan;
    hw_ops->disable_tx_vlan = fxgmac_disable_tx_vlan;
    hw_ops->enable_rx_vlan_stripping = fxgmac_enable_rx_vlan_stripping;
    hw_ops->disable_rx_vlan_stripping = fxgmac_disable_rx_vlan_stripping;
    hw_ops->enable_rx_vlan_filtering = fxgmac_enable_rx_vlan_filtering;
    hw_ops->disable_rx_vlan_filtering = fxgmac_disable_rx_vlan_filtering;
    hw_ops->update_vlan_hash_table = fxgmac_update_vlan_hash_table;

    /* For RX coalescing */
    hw_ops->config_rx_coalesce = fxgmac_config_rx_coalesce;
    hw_ops->config_tx_coalesce = fxgmac_config_tx_coalesce;
    hw_ops->usec_to_riwt = fxgmac_usec_to_riwt;
    hw_ops->riwt_to_usec = fxgmac_riwt_to_usec;

    /* For RX and TX threshold config */
    hw_ops->config_rx_threshold = fxgmac_config_rx_threshold;
    hw_ops->config_tx_threshold = fxgmac_config_tx_threshold;

    /* For RX and TX Store and Forward Mode config */
    hw_ops->config_rsf_mode = fxgmac_config_rsf_mode;
    hw_ops->config_tsf_mode = fxgmac_config_tsf_mode;

    /* For TX DMA Operating on Second Frame config */
    hw_ops->config_osp_mode = fxgmac_config_osp_mode;

    /* For RX and TX PBL config */
    hw_ops->config_rx_pbl_val = fxgmac_config_rx_pbl_val;
    hw_ops->get_rx_pbl_val = fxgmac_get_rx_pbl_val;
    hw_ops->config_tx_pbl_val = fxgmac_config_tx_pbl_val;
    hw_ops->get_tx_pbl_val = fxgmac_get_tx_pbl_val;
    hw_ops->config_pblx8 = fxgmac_config_pblx8;

    /* For MMC statistics support */
    hw_ops->tx_mmc_int = fxgmac_tx_mmc_int;
    hw_ops->rx_mmc_int = fxgmac_rx_mmc_int;
    hw_ops->read_mmc_stats = fxgmac_read_mmc_stats;
#if defined(UEFI)
#elif defined(_WIN32) || defined(_WIN64)
    hw_ops->update_stats_counters = fxgmac_update_stats_counters;
#endif

    /* For Receive Side Scaling */
    hw_ops->enable_rss = fxgmac_enable_rss;
    hw_ops->disable_rss = fxgmac_disable_rss;
    hw_ops->get_rss_options = fxgmac_read_rss_options;
    hw_ops->set_rss_options = fxgmac_write_rss_options;
    hw_ops->set_rss_hash_key = fxgmac_set_rss_hash_key;
    hw_ops->set_rss_lookup_table = fxgmac_set_rss_lookup_table;

    /*For Offload*/
#if defined(LINUX) || defined(_WIN64) || defined(_WIN32)
    hw_ops->set_arp_offload = fxgmac_update_aoe_ipv4addr;
    hw_ops->enable_arp_offload = fxgmac_enable_arp_offload;
    hw_ops->disable_arp_offload = fxgmac_disable_arp_offload;

    hw_ops->set_ns_offload = fxgmac_set_ns_offload;
    hw_ops->enable_ns_offload = fxgmac_enable_ns_offload;
    hw_ops->disable_ns_offload = fxgmac_disable_ns_offload;

    hw_ops->enable_wake_magic_pattern = fxgmac_enable_wake_magic_pattern;
    hw_ops->disable_wake_magic_pattern = fxgmac_disable_wake_magic_pattern;

    hw_ops->enable_wake_link_change = fxgmac_enable_wake_link_change;
    hw_ops->disable_wake_link_change = fxgmac_disable_wake_link_change;

    hw_ops->check_wake_pattern_fifo_pointer = fxgmac_check_wake_pattern_fifo_pointer;
    hw_ops->set_wake_pattern = fxgmac_set_wake_pattern;
    hw_ops->enable_wake_pattern = fxgmac_enable_wake_pattern;
    hw_ops->disable_wake_pattern = fxgmac_disable_wake_pattern;
    hw_ops->set_wake_pattern_mask = fxgmac_set_wake_pattern_mask;
#if defined(FUXI_PM_WPI_READ_FEATURE_EN) && FUXI_PM_WPI_READ_FEATURE_EN
    hw_ops->enable_wake_packet_indication = fxgmac_enable_wake_packet_indication;
    hw_ops->get_wake_packet_indication = fxgmac_get_wake_packet_indication;
#endif
#endif

    /*For phy write /read*/
    hw_ops->reset_phy = fxgmac_reset_phy;
    hw_ops->release_phy = fxgmac_release_phy;
    hw_ops->get_ephy_state = fxgmac_get_ephy_state;
    hw_ops->write_ephy_reg = fxgmac_write_ephy_reg;
    hw_ops->read_ephy_reg = fxgmac_read_ephy_reg;
    hw_ops->set_ephy_autoneg_advertise = fxgmac_set_ephy_autoneg_advertise;
    hw_ops->phy_config = fxgmac_phy_config;
    hw_ops->close_phy_led = fxgmac_close_phy_led;
    hw_ops->led_under_active = fxmgac_config_led_under_active;
    hw_ops->led_under_sleep = fxgmac_config_led_under_sleep;
    hw_ops->led_under_shutdown = fxgmac_config_led_under_shutdown;
    hw_ops->led_under_disable = fxgmac_config_led_under_disable;
#if !defined(UEFI)
    hw_ops->enable_phy_check = fxgmac_enable_phy_check;
    hw_ops->disable_phy_check = fxgmac_disable_phy_check;
    hw_ops->setup_cable_loopback = fxgmac_setup_cable_loopback;
    hw_ops->clean_cable_loopback = fxgmac_clean_cable_loopback;
    hw_ops->disable_phy_sleep = fxgmac_disable_phy_sleep;
    hw_ops->enable_phy_sleep = fxgmac_enable_phy_sleep;
    hw_ops->phy_green_ethernet = fxgmac_phy_green_ethernet;
    hw_ops->phy_eee_feature = fxgmac_phy_eee_feature;
#endif

    /* For power management */
    hw_ops->pre_power_down = fxgmac_pre_powerdown;
    hw_ops->config_power_down = fxgmac_config_powerdown;
    hw_ops->config_power_up = fxgmac_config_powerup;
    hw_ops->set_suspend_int = fxgmac_suspend_int;
    hw_ops->set_resume_int = fxgmac_resume_int;
    hw_ops->set_suspend_txrx = fxgmac_suspend_txrx;
    hw_ops->set_pwr_clock_gate = fxgmac_pwr_clock_gate;
    hw_ops->set_pwr_clock_ungate = fxgmac_pwr_clock_ungate;

    hw_ops->set_all_multicast_mode = fxgmac_set_all_multicast_mode;
    hw_ops->config_multicast_mac_hash_table = fxgmac_config_multicast_mac_hash_table;
    hw_ops->set_promiscuous_mode = fxgmac_set_promiscuous_mode;
    hw_ops->enable_rx_broadcast = fxgmac_enable_rx_broadcast;

    /* efuse relevant operation. */
    hw_ops->read_patch_from_efuse = fxgmac_read_patch_from_efuse; /* read patch per register. */
    hw_ops->read_patch_from_efuse_per_index = fxgmac_read_patch_from_efuse_per_index; /* read patch per index. */
    hw_ops->write_patch_to_efuse = fxgmac_write_patch_to_efuse;
    hw_ops->write_patch_to_efuse_per_index = fxgmac_write_patch_to_efuse_per_index;
    hw_ops->read_mac_subsys_from_efuse = fxgmac_read_mac_subsys_from_efuse;
    hw_ops->write_mac_subsys_to_efuse = fxgmac_write_mac_subsys_to_efuse;
    hw_ops->efuse_load = fxgmac_efuse_load;
    hw_ops->read_efuse_data = fxgmac_efuse_read_data;
    hw_ops->write_oob = fxgmac_efuse_write_oob;
    hw_ops->write_led = fxgmac_efuse_write_led;
    hw_ops->write_led_config = fxgmac_write_led_setting_to_efuse;
    hw_ops->read_led_config = fxgmac_read_led_setting_from_efuse;

    /*  */
    hw_ops->pcie_init = fxgmac_pcie_init;
    hw_ops->trigger_pcie = fxgmac_trigger_pcie;
}
