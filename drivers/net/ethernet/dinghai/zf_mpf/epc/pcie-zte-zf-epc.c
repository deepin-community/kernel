#include "pcie-zte-zf-epc.h"
#include "pcie-zte-zf-hdma.h"
#include "../../slib.h"

struct pcie_zf_ep *zf_ep = NULL;
static unsigned int epc_init_flag[4] = {0};

static int pcie_epc_cdev_major = PCIE_EPC_DRIVER_MAJOR;
static struct class *pcie_epc_cdev_class = NULL;
static struct cdev pcie_epc_cdev;
static dev_t pcie_epc_cdev_num;
static DEFINE_MUTEX(pcie_epc_cdev_mutex); /* 保护配置数据的互斥锁 */

static int pcie_epc_get_config(u64 arg);
static int pcie_epc_set_config(u64 arg);
static int pcie_epc_iatu_send_msg(struct pcie_epc_iatu_cfg *iatu_cfg);
struct pcie_epc_ioctl_sel ioctl_func_sel[] = {
    {PCIE_EPC_GET_CONFIG, pcie_epc_get_config},
    {PCIE_EPC_SET_CONFIG, pcie_epc_set_config},
};

#if CFG_FILE_MODE_ENABLE
extern int pcie_zte_zf_cfg_file_init(struct dh_core_dev *core_dev);
extern void pcie_zte_zf_cfg_file_exit(void);
#endif

extern int zxdh_bar_chan_sync_msg_send(struct zxdh_pci_bar_msg *in, struct zxdh_msg_recviver_mem *result);

/*======================================================base func of epc======================================================*/
#if 1
int pcie_zf_read(void __iomem *addr, int size, u32 *val)
{
    *val = 0;
    if (!IS_ALIGNED((unsigned long)addr, size))
        return PCIBIOS_BAD_REGISTER_NUMBER;

    if (size == 4)
        *val = readl(addr);
    else if (size == 2)
        *val = readw(addr);
    else if (size == 1)
        *val = readb(addr);
    else
        return PCIBIOS_BAD_REGISTER_NUMBER;

    return PCIBIOS_SUCCESSFUL;
}
int pcie_zf_write(void __iomem *addr, int size, u32 val)
{
    if (!IS_ALIGNED((unsigned long)addr, size))
        return PCIBIOS_BAD_REGISTER_NUMBER;

    if (size == 4)
        writel(val, addr);
    else if (size == 2)
        writew(val, addr);
    else if (size == 1)
        writeb(val, addr);
    else
        return PCIBIOS_BAD_REGISTER_NUMBER;

    return PCIBIOS_SUCCESSFUL;
}
u32 cfg_phy_rmw(u64 phy_addr, u32 value, u32 mask)
{
    u32 reg_val = 0;
    void __iomem *virt_addr = NULL;
    u64 tmp_addr = 0;
    u64 offset = 0;
    u64 size = 0;
    int ret = 0;

    offset = phy_addr % PAGE_SIZE;
    if (phy_addr < offset)
    {
        DH_LOG_ERR(MODULE_MPF, "data overflow! phy_addr=0x%llx, offset=0x%llx\n", phy_addr, offset);
        return PCIBIOS_BAD_REGISTER_NUMBER;
    }
    else
    {
        tmp_addr = phy_addr - offset;
    }
    if (offset <= (PAGE_SIZE - 4))
    {
        size = PAGE_SIZE;
    }
    else
    {
        size = 2 * PAGE_SIZE;
    }

    virt_addr = ioremap(tmp_addr, size);
    if (NULL == virt_addr)
    {
        DH_LOG_ERR(MODULE_MPF, "cfg_phy_rmw ioremap failed!\n");
        return PCIBIOS_BAD_REGISTER_NUMBER;
    }

    ret = pcie_zf_read((virt_addr + offset), 4, &reg_val);
    if (ret)
        goto err;

    reg_val &= (~mask);
    reg_val |= (value & mask);

    ret = pcie_zf_write(virt_addr + offset, 4, reg_val);
err:
    iounmap(virt_addr);
    return ret;
}

u8 pcie_zf_readb_dbi(struct pcie_dpu_ep *ep, u32 reg)
{
    u32 val = 0;

    pcie_zf_read(ep->dbi_base + reg, 0x1, &val);

    return val;
}

u16 pcie_zf_readw_dbi(struct pcie_dpu_ep *ep, u32 reg)
{
    int ret = 0;
    u32 val;

    ret = pcie_zf_read(ep->dbi_base + reg, 0x2, &val);
    if (ret)
        DH_LOG_ERR(MODULE_MPF, "Read DBIw address failed\r\n");

    return val;
}
u32 pcie_zf_readl_dbi(struct pcie_dpu_ep *ep, u32 reg)
{
    int ret = 0;
    u32 val = 0;

    ret = pcie_zf_read(ep->dbi_base + reg, 0x4, &val);
    if (ret)
        DH_LOG_ERR(MODULE_MPF, "Read DBIl address failed\r\n");

    return val;
}

void pcie_zf_writeb_dbi(struct pcie_dpu_ep *ep, u32 reg, u32 val)
{
    pcie_zf_write(ep->dbi_base + reg, 0x1, val);
}

void pcie_zf_writew_dbi(struct pcie_dpu_ep *ep, u32 reg, u32 val)
{
    int ret = 0;

    ret = pcie_zf_write(ep->dbi_base + reg, 0x2, val);
    if (ret)
        DH_LOG_ERR(MODULE_MPF, "Write DBI address failed\r\n");
}
void pcie_zf_writel_dbi(struct pcie_dpu_ep *ep, u32 reg, u32 val)
{
    int ret = 0;

    ret = pcie_zf_write(ep->dbi_base + reg, 0x4, val);
    if (ret)
        DH_LOG_ERR(MODULE_MPF, "Write DBI address failed\r\n");
}

void pcie_zf_writeb_dbi2(struct pcie_dpu_ep *ep, u32 reg, u32 val)
{
    pcie_zf_write(ep->dbi_base + PCIE_DPU_EP_DBI2_OFFSET + reg, 0x1, val);
}

void pcie_zf_writew_dbi2(struct pcie_dpu_ep *ep, u32 reg, u32 val)
{
    int ret = 0;

    ret = pcie_zf_write(ep->dbi_base + PCIE_DPU_EP_DBI2_OFFSET + reg, 0x2, val);
    if (ret)
        DH_LOG_ERR(MODULE_MPF, "Write DBI address failed\r\n");
}
void pcie_zf_writel_dbi2(struct pcie_dpu_ep *ep, u32 reg, u32 val)
{
    int ret = 0;

    ret = pcie_zf_write(ep->dbi_base + PCIE_DPU_EP_DBI2_OFFSET + reg, 0x4, val);
    if (ret)
        DH_LOG_ERR(MODULE_MPF, "Write DBI address failed\r\n");
}
void pcie_zf_writel_atu(struct pcie_dpu_ep *ep, u32 reg, u32 val)
{
    int ret = 0;

    ret = pcie_zf_write(ep->atu_base + reg, 0x4, val);
    if (ret)
        DH_LOG_ERR(MODULE_MPF, "Write ATU address failed\r\n");
}
static u32 pcie_zf_readl_atu(struct pcie_dpu_ep *ep, u32 reg)
{
    int ret = 0;
    u32 val = 0;

    ret = pcie_zf_read(ep->atu_base + reg, 4, &val);
    if (ret)
        DH_LOG_ERR(MODULE_MPF, "Read ATU address failed\r\n");

    return val;
}
static u64 zte_pcie_dma_atu_addr_remapping(u64 addr_input)
{
    u64 addr_output = 0;
    // DMA and ATU地址重映射，将地址的bit12~18左移4位，设置bit15 = 0
    addr_output = (((addr_input & (0x7F << 12)) << 4) | (addr_input & 0xFFF)) & (~(1 << 15));

    return addr_output;
}
static u32 pcie_zf_readl_ib_unroll(struct pcie_dpu_ep *ep, u32 index, u32 reg)
{
    u32 offset = zte_pcie_dma_atu_addr_remapping(PCIE_GET_ATU_INB_UNR_REG_OFFSET(index));

    return pcie_zf_readl_atu(ep, offset + reg);
}
static void pcie_zf_writel_ib_unroll(struct pcie_dpu_ep *ep, u32 index, u32 reg, u32 val)
{
    u32 offset = zte_pcie_dma_atu_addr_remapping(PCIE_GET_ATU_INB_UNR_REG_OFFSET(index));

    pcie_zf_writel_atu(ep, offset + reg, val);
}
static u32 pcie_zf_readl_ob_unroll(struct pcie_dpu_ep *ep, u32 index, u32 reg)
{
    u32 offset = zte_pcie_dma_atu_addr_remapping(PCIE_GET_ATU_OUTB_UNR_REG_OFFSET(index));

    return pcie_zf_readl_atu(ep, offset + reg);
}
static void pcie_zf_writel_ob_unroll(struct pcie_dpu_ep *ep, u32 index, u32 reg, u32 val)
{
    u32 offset = zte_pcie_dma_atu_addr_remapping(PCIE_GET_ATU_OUTB_UNR_REG_OFFSET(index));

    pcie_zf_writel_atu(ep, offset + reg, val);
}
static void pcie_zf_dbi_ro_wr_en(struct pcie_dpu_ep *ep)
{
    u64 reg = 0;
    u32 val = 0;

    reg = PCIE_MISC_CONTROL_1_OFF;
    val = pcie_zf_readl_dbi(ep, reg);
    val |= PCIE_DBI_RO_WR_EN;
    pcie_zf_writel_dbi(ep, reg, val);
}
static void pcie_zf_dbi_ro_wr_dis(struct pcie_dpu_ep *ep)
{
    u64 reg = 0;
    u32 val = 0;

    reg = PCIE_MISC_CONTROL_1_OFF;
    val = pcie_zf_readl_dbi(ep, reg);
    val &= ~PCIE_DBI_RO_WR_EN;
    pcie_zf_writel_dbi(ep, reg, val);
}
static void pcie_dpu_ep_sriov_enable(struct pcie_dpu_ep *ep, u64 sriov_ecap_offset)
{
    u32 val = pcie_zf_readl_dbi(ep, sriov_ecap_offset + PCIE_SRIOV_CTRL);
    val |= PCIE_SRIOV_CTRL_VFE;
    pcie_zf_writel_dbi(ep, sriov_ecap_offset + PCIE_SRIOV_CTRL, val);
}
static void pcie_dpu_ep_sriov_disable(struct pcie_dpu_ep *ep, u64 sriov_ecap_offset)
{
    u32 val = pcie_zf_readl_dbi(ep, sriov_ecap_offset + PCIE_SRIOV_CTRL);
    val &= ~PCIE_SRIOV_CTRL_VFE;
    pcie_zf_writel_dbi(ep, sriov_ecap_offset + PCIE_SRIOV_CTRL, val);
}
struct pcie_dpu_ep_func *pcie_dpu_ep_get_func_from_ep(struct pcie_dpu_ep *ep, u8 func_no, u8 vfunc_no)
{
    struct pcie_dpu_ep_func *ep_func = NULL;

    list_for_each_entry(ep_func, &ep->func_list, list)
    {
        if (ep_func->func_no == func_no && ep_func->vfunc_no == vfunc_no)
            return ep_func;
    }

    return NULL;
}
static u32 pcie_dpu_ep_func_select(u8 func_no, u8 vfunc_no)
{
    u32 func_offset = 0;

    if (isPF(func_no))
    {
        func_offset = func_no & PCIE_DPU_EP_GET_PF_NO;
    }
    else
    {
        func_offset = (func_no & PCIE_DPU_EP_GET_PF_NO) + (vfunc_no << DBI_VF_CFG_OFFSET_BIT) + VF_ACT_BIT;
    }

    return func_offset * PCIE_DPU_EP_FUNC_CFG_SIZE;
}
static u8 pcie_dpu_ep_find_next_cap(struct pcie_dpu_ep *ep,
                                    u32 func_offset, u8 cap_ptr, u8 capid)
{
    u8 now_cap_id = 0, next_cap_ptr = 0;
    u16 reg = 0;

    if (!cap_ptr)
        return 0;

    reg = pcie_zf_readl_dbi(ep, func_offset + cap_ptr);
    now_cap_id = (reg & 0x00ff);

    if (now_cap_id > PCI_CAP_ID_MAX)
        return 0;

    if (now_cap_id == capid)
        return cap_ptr;

    next_cap_ptr = (reg & 0xff00) >> 8;
    return pcie_dpu_ep_find_next_cap(ep, func_offset, next_cap_ptr, capid);
}
static u8 pcie_dpu_ep_find_cap(struct pcie_dpu_ep *ep, u32 func_offset, u8 capid)
{
    u8 next_cap_ptr = 0;
    u16 reg = 0;

    reg = pcie_zf_readl_dbi(ep, func_offset + PCI_CAPABILITY_LIST);
    next_cap_ptr = (reg & 0x00ff);

    return pcie_dpu_ep_find_next_cap(ep, func_offset, next_cap_ptr, capid);
}
static int pcie_dpu_ep_find_extcap(struct pcie_dpu_ep *ep, u32 func_offset, u8 ext_cap_id, u8 vsecid)
{
    u32 now_cap_id = 0;
    u32 vsec_id = 0;
    u32 ext_cap_offset = PCIE_ECAP_POINTER_OFF;

    now_cap_id = pcie_zf_readl_dbi(ep, func_offset + ext_cap_offset);
    if (now_cap_id == 0x0 || now_cap_id == 0xFFFF)
    {
        DH_LOG_ERR(MODULE_MPF, "pcie_zf_ep get extcap0 failed!\n");
        return -ENXIO;
    }

    while (1)
    {
        if ((now_cap_id & 0xFFFF) == ext_cap_id)
        {
            if (ext_cap_id == PCIE_ECAP_VSEC_ID)
            {
                vsec_id = pcie_zf_readl_dbi(ep, func_offset + ext_cap_offset + 4);
                if (vsec_id == vsecid)
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }

        ext_cap_offset = (now_cap_id >> 20) & 0xFFF;
        if (0x0 == ext_cap_offset)
        {
            DH_LOG_ERR(MODULE_MPF, "pcie_zf_ep find extcap failed\n");
            return -ENXIO;
        }

        now_cap_id = pcie_zf_readl_dbi(ep, func_offset + ext_cap_offset);
    }

    return ext_cap_offset;
}

static int zf_atu_is_used(struct pcie_dpu_ep *ep, int ib_no)
{
    return (pcie_zf_readl_ib_unroll(ep, ib_no, PCIE_ATU_UNR_REGION_CTRL2) & PCIE_ATU_ENABLE) ? 1 : 0;
}

static int zf_func_is_set_ib(struct pcie_dpu_ep *ep, u8 func_no, enum pci_barno bar, int ib_no)
{
    int func_val = 0, bar_val = 0;
    int ctl1_val = pcie_zf_readl_ib_unroll(ep, ib_no, PCIE_ATU_UNR_REGION_CTRL1);
    int ctl2_val = pcie_zf_readl_ib_unroll(ep, ib_no, PCIE_ATU_UNR_REGION_CTRL2);

    func_val = PCIE_ATU_FUNC_NUM(func_no & PCIE_DPU_EP_GET_PF_NO);
    if ((ctl1_val & PCIE_ATU_FUNC_NUM_MASK) != func_val)
        return 0;

    if (isPF(func_no))
    {
        if (ctl2_val & PCIE_ATU_VFBAR_MATCH_MODE_ENABLE)
            return 0;
    }
    else
    {
        if (!(ctl2_val & PCIE_ATU_VFBAR_MATCH_MODE_ENABLE))
            return 0;
    }
    bar_val = (bar << 8);
    return ((ctl2_val & PCIE_ATU_BAR_NUM_MASK) == bar_val);
}

static int pcie_zf_prog_inbound_atu(struct pcie_dpu_ep *ep, u8 func_no, int index, int bar, dma_addr_t dpu_addr, enum pcie_dpu_as_type as_type)
{
    int type = 0;
    u32 retries = 0, val = 0;
    int vf_flag = 0;

    dpu_addr = dpu_addr | ZF_PREFIX_ADDR; // dpu addr route

    if (!isPF(func_no))
    {
        vf_flag = 1;
    }

    pcie_zf_writel_ib_unroll(ep, index, PCIE_ATU_UNR_LOWER_TARGET, lower_32_bits(dpu_addr));
    pcie_zf_writel_ib_unroll(ep, index, PCIE_ATU_UNR_UPPER_TARGET, upper_32_bits(dpu_addr));

    switch (as_type)
    {
        case PCIE_DPU_AS_MEM:
            type = PCIE_ATU_TYPE_MEM;
            break;
        case PCIE_DPU_AS_IO:
            if (vf_flag == 1)
            {
                return -EINVAL;
            }
            type = PCIE_ATU_TYPE_IO;
            break;
        default:
            return -EINVAL;
    }

    pcie_zf_writel_ib_unroll(ep, index, PCIE_ATU_UNR_REGION_CTRL1, type | PCIE_ATU_INCREASE_REGION_SIZE | PCIE_ATU_FUNC_NUM(func_no & PCIE_DPU_EP_GET_PF_NO));
    if (vf_flag)
    {
        pcie_zf_writel_ib_unroll(ep, index, PCIE_ATU_UNR_REGION_CTRL2,
                                 PCIE_ATU_FUNC_NUM_MATCH_EN |
                                     PCIE_ATU_ENABLE |
                                     PCIE_ATU_VFBAR_MATCH_MODE_ENABLE |
                                     PCIE_ATU_BAR_MODE_ENABLE | (bar << 8));
    }
    else
    {
        pcie_zf_writel_ib_unroll(ep, index, PCIE_ATU_UNR_REGION_CTRL2,
                                 PCIE_ATU_FUNC_NUM_MATCH_EN |
                                     PCIE_ATU_ENABLE |
                                     PCIE_ATU_BAR_MODE_ENABLE | (bar << 8));
    }

    /*
     * Make sure ATU enable takes effect before any subsequent config
     * and I/O accesses.
     */
    for (retries = 0; retries < LINK_WAIT_MAX_IATU_RETRIES; retries++)
    {
        val = pcie_zf_readl_ib_unroll(ep, index, PCIE_ATU_UNR_REGION_CTRL2);
        if (val & PCIE_ATU_ENABLE)
            return 0;

        mdelay((u32)LINK_WAIT_IATU);
    }
    DH_LOG_ERR(MODULE_MPF, "Inbound iATU is not being enabled\r\n");

    return -EBUSY;
}

static int send_pcie_dpu_ep_inbound_info(struct pcie_dpu_ep *ep, u8 func_no, u32 atu_win, enum pci_barno bar, dma_addr_t dpu_addr)
{
    struct pcie_epc_iatu_cfg iatu_cfg = {0};

    iatu_cfg.cmd_id = CMD_SET_IATU;
    iatu_cfg.ep_id = ep->ep_id;
    iatu_cfg.pf_id = (func_no & PCIE_DPU_EP_GET_PF_NO);
    iatu_cfg.is_pf = isPF(func_no);
    iatu_cfg.bar_index = bar;
    iatu_cfg.iatu_win = atu_win;
    iatu_cfg.target_addr = (u64)dpu_addr;

    DH_LOG_DEBUG(MODULE_MPF, "atu set target_addr:0x%llx\n", iatu_cfg.target_addr);

    return pcie_epc_iatu_send_msg(&iatu_cfg);
}

static int pcie_dpu_ep_inbound_atu(struct pcie_dpu_ep *ep, u8 func_no, enum pci_barno bar, dma_addr_t dpu_addr, enum pcie_dpu_as_type as_type)
{
    int ret = 0, free_win = -1, atu_id = 0;
    u32 vf_bar_off = 0;
    u32 is_pf = 0;
    u32 bar_to_atu_index = 0;

    if ((func_no & PCIE_DPU_EP_GET_PF_NO) >= PCIE_DPU_PF_NUMS)
    {
        DH_LOG_ERR(MODULE_MPF, "func_no is err!\n");
        return -EINVAL;
    }

    is_pf = isPF(func_no);
    if (!is_pf)
    {
        vf_bar_off = PCIE_VF_BARS_OFF;
    }

    spin_lock(&ep->ib_window_lock);
    for (atu_id = 0; atu_id < PCIE_DPU_IATU_NUMS; atu_id++)
    {
        if (!zf_atu_is_used(ep, atu_id))
        {
            clear_bit(atu_id, ep->ib_window_map);
        }
        else if (zf_func_is_set_ib(ep, func_no, bar, atu_id))
        {
            free_win = atu_id;
            break;
        }
    }

    if (-1 == free_win)
    {
        free_win = find_first_zero_bit(ep->ib_window_map, ep->num_ib_windows);
        if (free_win >= ep->num_ib_windows)
        {
            spin_unlock(&ep->ib_window_lock);
            DH_LOG_ERR(MODULE_MPF, "No free inbound window\r\n");
            return -EINVAL;
        }
    }
    set_bit(free_win, ep->ib_window_map);

    spin_unlock(&ep->ib_window_lock);
    ret = pcie_zf_prog_inbound_atu(ep, func_no, free_win, bar, dpu_addr, as_type);
    if (ret < 0)
    {
        DH_LOG_ERR(MODULE_MPF, "Failed to program IB window\r\n");
        return ret;
    }
    DH_LOG_INFO(MODULE_MPF, "ep%d func%d bar%d set aitu%d\n", ep->ep_id, func_no, bar, free_win);
    send_pcie_dpu_ep_inbound_info(ep, func_no, free_win, bar, dpu_addr);

    bar_to_atu_index = (u32)bar + vf_bar_off;
    if (bar_to_atu_index < (PCI_STD_NUM_BARS * 2 + 1))
    {
        ep->bar_to_atu[func_no & PCIE_DPU_EP_GET_PF_NO][bar_to_atu_index] = free_win;
    }
    else
    {
        DH_LOG_ERR(MODULE_MPF, "error bar_to_atu index %d\r\n", bar_to_atu_index);
        return -EINVAL;
    }

    return 0;
}

static void pcie_zf_prog_outbound_atu(struct pcie_dpu_ep *ep, u8 func_no, u8 vfunc_no, int index, int type, u64 dpu_addr, u64 host_addr, size_t size)
{
    u32 retries = 0, val = 0;
    u64 limit_addr = 0;
    u64 limit_addr_tmp = 0;

    if (size == 0)
    {
        DH_LOG_ERR(MODULE_MPF, "data error! dpu_addr=0x%llx, size=0x%lx\r\n", dpu_addr, size);
        return;
    }
    else
    {
        limit_addr_tmp = ULLONG_MAX - size + 1;
        if (dpu_addr > limit_addr_tmp)
        {
            DH_LOG_ERR(MODULE_MPF, "data overflow!\r\n");
            return;
        }
        limit_addr = dpu_addr + size - 1;
    }

    DH_LOG_INFO(MODULE_MPF, "ep_id[0x%x] func_no:0x%x, vfunc_no:0x%x, index: %d, dpu_addr:0x%llx, host_addr:0x%llx, size:0x%lx\n",
                ep->ep_id, func_no, vfunc_no, index, dpu_addr, host_addr, size);

    pcie_zf_writel_ob_unroll(ep, index, PCIE_ATU_UNR_LOWER_BASE, lower_32_bits(dpu_addr));
    pcie_zf_writel_ob_unroll(ep, index, PCIE_ATU_UNR_UPPER_BASE, upper_32_bits(dpu_addr));
    pcie_zf_writel_ob_unroll(ep, index, PCIE_ATU_UNR_LOWER_LIMIT, lower_32_bits(limit_addr));
    pcie_zf_writel_ob_unroll(ep, index, PCIE_ATU_UNR_UPPER_LIMIT, upper_32_bits(limit_addr));
    pcie_zf_writel_ob_unroll(ep, index, PCIE_ATU_UNR_LOWER_TARGET, lower_32_bits(host_addr));
    pcie_zf_writel_ob_unroll(ep, index, PCIE_ATU_UNR_UPPER_TARGET, upper_32_bits(host_addr));
    pcie_zf_writel_ob_unroll(ep, index, PCIE_ATU_UNR_REGION_CTRL1,
                             type | PCIE_ATU_FUNC_NUM(func_no & PCIE_DPU_EP_GET_PF_NO));

    if (type == 4 || type == 5)
    {
        pcie_zf_writel_ob_unroll(ep, index, PCIE_ATU_UNR_REGION_CTRL2, PCIE_ATU_ENABLE | PCIE_ATU_CFG_SHIFT_MODE | PCIE_ATU_DMA_BYPSS);
    }
    else
    {
        pcie_zf_writel_ob_unroll(ep, index, PCIE_ATU_UNR_REGION_CTRL2, PCIE_ATU_ENABLE | PCIE_ATU_DMA_BYPSS);
    }

    if (isPF(func_no))
    {
        pcie_zf_writel_ob_unroll(ep, index, PCIE_ATU_UNR_REGION_CTRL3, 0x0);
    }
    else
    {
        pcie_zf_writel_ob_unroll(ep, index, PCIE_ATU_UNR_REGION_CTRL3, PCIE_ATU_OB_VF_ACTIVE | vfunc_no);
    }

    /*
     * Make sure ATU enable takes effect before any subsequent config
     * and I/O accesses.
     */
    for (retries = 0; retries < LINK_WAIT_MAX_IATU_RETRIES; retries++)
    {
        val = pcie_zf_readl_ob_unroll(ep, index, PCIE_ATU_UNR_REGION_CTRL2);
        if (val & PCIE_ATU_ENABLE)
            return;

        mdelay((u32)LINK_WAIT_IATU);
    }
    DH_LOG_ERR(MODULE_MPF, "Outbound iATU is not being enabled\r\n");
}

static int pcie_dpu_ep_outbound_atu(struct pcie_dpu_ep *ep, u8 func_no, u8 vfunc_no, phys_addr_t dpu_offset, u64 host_addr, size_t size)
{
    u32 free_win = 0;

    free_win = find_first_zero_bit(ep->ob_window_map, ep->num_ob_windows);
    if (free_win >= ep->num_ob_windows)
    {
        DH_LOG_ERR(MODULE_MPF, "No free outbound window\r\n");
        return -EINVAL;
    }

    pcie_zf_prog_outbound_atu(ep, func_no, vfunc_no, free_win, PCIE_ATU_TYPE_MEM,
                              dpu_offset, host_addr, size);

    set_bit(free_win, ep->ob_window_map);
    ep->ob_src_addr[free_win] = dpu_offset;

    return 0;
}

static void pcie_dpu_ep_reset_bar(struct pcie_dpu_ep *ep, u8 func_no, u8 vfunc_no,
                                  enum pci_barno bar, int flags)
{
    int sriov_cap_offset = 0;
    u32 reg = 0;
    u32 func_offset = pcie_dpu_ep_func_select(func_no & PCIE_DPU_EP_GET_PF_NO, vfunc_no);

    if (isPF(func_no))
    {
        reg = func_offset + (u32)(PCI_BASE_ADDRESS_0) + (u32)(PCIE_NEXT_BAR_OFFSET * bar);
        pcie_zf_dbi_ro_wr_en(ep);
        // pcie_zf_writel_dbi2(ep, reg, 0x0);
        pcie_zf_writel_dbi(ep, reg, 0xc);
        // pcie_zf_writel_dbi2(ep, reg + PCIE_NEXT_BAR_OFFSET, 0x0);
        // pcie_zf_writel_dbi(ep, reg + PCIE_NEXT_BAR_OFFSET, 0x0);
        pcie_zf_dbi_ro_wr_dis(ep);
    }
    else
    {
        sriov_cap_offset = pcie_dpu_ep_find_extcap(ep, func_offset, PCI_EXT_CAP_ID_SRIOV, 0);
        if (sriov_cap_offset < 0)
        {
            DH_LOG_ERR(MODULE_MPF, "find_extcap failed!!\n");
        }

        reg = func_offset + (u32)sriov_cap_offset + (u32)(PCIE_SRIOV_ECAP_BAR0_OFFSET) + (u32)(bar * PCIE_NEXT_BAR_OFFSET);
        pcie_zf_writel_dbi(ep, reg, 0xc);
        pcie_zf_writel_dbi(ep, reg + PCIE_NEXT_BAR_OFFSET, 0x0);

        // reg = func_offset + sriov_cap_offset;
        // pcie_dpu_ep_sriov_disable(ep, reg);
        // pcie_zf_writel_dbi2(ep, reg + PCIE_SRIOV_ECAP_BAR0_OFFSET + bar * PCIE_NEXT_BAR_OFFSET, 0x0);
        // pcie_zf_writel_dbi2(ep, reg + PCIE_SRIOV_ECAP_BAR0_OFFSET + (bar + 1) * PCIE_NEXT_BAR_OFFSET, 0x0);
        // pcie_dpu_ep_sriov_enable(ep, reg);
    }
}

void pcie_zf_disable_atu(struct pcie_dpu_ep *ep, int index,
                         enum pcie_dpu_region_type type)
{
    u32 val = 0;
    switch (type)
    {
        case PCIE_DPU_REGION_INBOUND:
            val = pcie_zf_readl_ib_unroll(ep, index, PCIE_ATU_UNR_REGION_CTRL2);
            val &= (u32)(~PCIE_ATU_ENABLE);
            pcie_zf_writel_ib_unroll(ep, index, PCIE_ATU_UNR_REGION_CTRL2, val);
            break;
        case PCIE_DPU_REGION_OUTBOUND:
            val = pcie_zf_readl_ob_unroll(ep, index, PCIE_ATU_UNR_REGION_CTRL2);
            val &= (u32)(~PCIE_ATU_ENABLE);
            pcie_zf_writel_ob_unroll(ep, index, PCIE_ATU_UNR_REGION_CTRL2, val);
            break;
        default:
            return;
    }
}

static int pcie_zf_find_index(struct pcie_dpu_ep *ep, phys_addr_t dpu_offset,
                              u32 *atu_index)
{
    u32 index = 0;

    for (index = 0; index < ep->num_ob_windows; index++)
    {
        if (ep->ob_src_addr[index] != dpu_offset)
            continue;
        *atu_index = index;
        return 0;
    }

    return -EINVAL;
}
#endif

/*======================================================ioctl func of epf======================================================*/
#if 1
// if ep is link up, return 1
int is_pcie_ep_link(int ep_id)
{
    u32 val = 0;
    void __iomem *csr_base_addr = 0;

    if ((ep_id < 0) || (ep_id >= PCIE_DPU_EP_NUMS))
    {
        DH_LOG_ERR(MODULE_MPF, "is_pcie_ep_link:err ep_id!\n");
        return -1;
    }

    csr_base_addr = zf_ep->mpf_vaddr + PCIE_DPU_MPF_CSR_ADDR(PCIE_DPU_EP_CSR_SIZE * ep_id);
    pcie_zf_read(csr_base_addr + PCIE_DPU_EP_CSR_LTSSM_ADDR, 4, &val);
    if (LTSSM_EN_VAL != val)
    {
        DH_LOG_ERR(MODULE_MPF, "read off:0x150 val:0x%x\n", val);
    }

    return (LTSSM_EN_VAL == val);
}
EXPORT_SYMBOL_GPL(is_pcie_ep_link);

void ep_power_reset(int ep_id)
{
    u64 csr_base_addr = 0;

    csr_base_addr = zf_ep->mpf_paddr + PCIE_DPU_MPF_CSR_ADDR(PCIE_DPU_EP_CSR_SIZE * ep_id);
    cfg_phy_rmw(csr_base_addr + PCIE_DPU_EP_CSR_PRST_ADDR, 0x0, 0x2);
    cfg_phy_rmw(csr_base_addr + PCIE_DPU_EP_CSR_PRST_ADDR, 0x2, 0x2);
}
EXPORT_SYMBOL_GPL(ep_power_reset);

int ep_virtio_module_set(int ep_id, int pf_idx, int en)
{
    u64 csr_base_addr = 0;

    csr_base_addr = zf_ep->mpf_paddr + PCIE_DPU_MPF_CSR_ADDR(PCIE_DPU_EP_CSR_SIZE * ep_id);
    if ((0 != en) & (1 != en))
    {
        DH_LOG_ERR(MODULE_MPF, "err module!\n");
        return -EINVAL;
    }
    cfg_phy_rmw(csr_base_addr + PCIE_DPU_EP_CSR_VIRT_ADDR, en << pf_idx, 0x1 << pf_idx);
    return 0;
}
EXPORT_SYMBOL_GPL(ep_virtio_module_set);

static phys_addr_t ob_addr_set(int ep_id, phys_addr_t phys_addr)
{
    phys_addr_t rel_addr;
    u64 addr_mask = 0xffff;

    rel_addr = (phys_addr & addr_mask) | ep_id << 16 | ((phys_addr & ~addr_mask) << EP_ID_LEN);
    return rel_addr;
}

int pcie_zte_epc_ob_read(struct pci_epc *epc, phys_addr_t phys_addr, unsigned int size, unsigned int *val)
{
    void __iomem *vaddr = NULL;
    struct pcie_dpu_ep *dpu_dev = NULL;

    if (!epc)
    {
        DH_LOG_ERR(MODULE_MPF, "epc is NULL!\n");
        return -ENOMEM;
    }

    if (phys_addr < zf_ep->vsock_paddr || phys_addr >= zf_ep->vsock_paddr + zf_ep->ob_size)
    {
        DH_LOG_ERR(MODULE_MPF, "err:phys_addr out of range!\n");
        return -ENOMEM;
    }

    dpu_dev = epc_get_drvdata(epc);
    vaddr = zf_ep->vsock_vaddr + ob_addr_set(dpu_dev->ep_id + 5, phys_addr - zf_ep->vsock_paddr);

    return pcie_zf_read(vaddr, size, val);
}
EXPORT_SYMBOL_GPL(pcie_zte_epc_ob_read);

int pcie_zte_epc_ob_write(struct pci_epc *epc, phys_addr_t phys_addr, int size, int val)
{
    void __iomem *vaddr = NULL;
    struct pcie_dpu_ep *dpu_dev = NULL;

    if (!epc)
    {
        DH_LOG_ERR(MODULE_MPF, "epc is NULL\n");
        return -ENOMEM;
    }

    if (phys_addr < zf_ep->vsock_paddr || phys_addr >= zf_ep->vsock_paddr + zf_ep->ob_size)
    {
        DH_LOG_ERR(MODULE_MPF, "err:phys_addr out of range\n");
        return -ENOMEM;
    }

    dpu_dev = epc_get_drvdata(epc);

    vaddr = zf_ep->vsock_vaddr + ob_addr_set(dpu_dev->ep_id + 5, phys_addr - zf_ep->vsock_paddr);

    return pcie_zf_write(vaddr, size, val);
}
EXPORT_SYMBOL_GPL(pcie_zte_epc_ob_write);
#endif

/*======================================================the ops of epc======================================================*/
#if 1
static int pcie_dpu_ep_write_header(struct pci_epc *epc, u8 func_no, u8 vfunc_no,
                                    struct pci_epf_header *hdr)
{
    struct pcie_dpu_ep *ep = epc_get_drvdata(epc);
    u32 func_offset = 0, sriov_offset = 0;
    int sriov_cap_offset = 0;
    int pf_no = func_no & PCIE_DPU_EP_GET_PF_NO;

    if (!is_pcie_ep_link(ep->ep_id))
    {
        DH_LOG_ERR(MODULE_MPF, "err: ep%d not link\n", ep->ep_id);
        return -ENODEV;
    }

    DH_LOG_INFO(MODULE_MPF, "func_no = 0x%x, vfunc_no = 0x%x\n", func_no, vfunc_no);

    pcie_zf_dbi_ro_wr_en(ep);
    if (isPF(func_no))
    {
        func_offset = pcie_dpu_ep_func_select(func_no, 0);
        pcie_zf_writew_dbi(ep, func_offset + PCI_VENDOR_ID, hdr->vendorid);
        pcie_zf_writew_dbi(ep, func_offset + PCI_DEVICE_ID, hdr->deviceid);
        pcie_zf_writel_dbi(ep, func_offset + PCI_CLASS_REVISION, hdr->revid | hdr->progif_code << 8 | hdr->subclass_code << 16 | hdr->baseclass_code << 24);
        pcie_zf_writew_dbi(ep, func_offset + PCI_SUBSYSTEM_VENDOR_ID, hdr->subsys_vendor_id);
        pcie_zf_writew_dbi(ep, func_offset + PCI_SUBSYSTEM_ID, hdr->subsys_id);
    }
    else
    {
        func_offset = pcie_dpu_ep_func_select(pf_no, 0);
        sriov_cap_offset = pcie_dpu_ep_find_extcap(ep, func_offset, PCI_EXT_CAP_ID_SRIOV, 0);
        if (sriov_cap_offset < 0)
        {
            DH_LOG_ERR(MODULE_MPF, "find_extcap failed!!\n");
            return -ENODEV;
        }
        sriov_offset = func_offset + (u32)sriov_cap_offset;
        pcie_zf_writew_dbi(ep, sriov_offset + PCIE_SRIOV_ECAP_DEVICE_ID, hdr->deviceid);
    }
    pcie_zf_dbi_ro_wr_dis(ep);

    return 0;
}

static int pcie_dpu_ep_set_pf_bar(struct pci_epc *epc, u8 func_no, u8 vfunc_no,
                                  struct pci_epf_bar *epf_bar)
{
    int ret = 0;
    struct pcie_dpu_ep *ep = epc_get_drvdata(epc);
    enum pci_barno barno = epf_bar->barno;
    size_t size = epf_bar->size;
    int flags = epf_bar->flags;
    enum pcie_dpu_as_type as_type;
    u32 reg = 0;
    u32 func_offset = 0;
    u64 dpu_addr = 0;

    if (!is_pcie_ep_link(ep->ep_id))
    {
        DH_LOG_ERR(MODULE_MPF, "err: ep%d not link\n", ep->ep_id);
        return -ENODEV;
    }

    dpu_addr = epf_bar->phys_addr | ZF_PREFIX_ADDR;

    if (!(flags & PCI_BASE_ADDRESS_SPACE))
        as_type = PCIE_DPU_AS_MEM;
    else
        as_type = PCIE_DPU_AS_IO;

    if (barno != BAR_4)
    {
        ret = pcie_dpu_ep_inbound_atu(ep, func_no, barno, dpu_addr, as_type);
        if (ret)
            return ret;
    }
    else
    {
        return 0;
    }

    func_offset = pcie_dpu_ep_func_select(func_no, vfunc_no);

    if (barno != BAR_ROM)
    {
        reg = PCI_BASE_ADDRESS_0 + (4 * barno) + func_offset;
    }
    else
    {
        reg = PCI_ROM_ADDRESS + func_offset;
    }

    if (size)
    {
        pcie_zf_dbi_ro_wr_en(ep);

        pcie_zf_writel_dbi2(ep, reg, 1);
        pcie_zf_writel_dbi2(ep, reg, lower_32_bits(size - 1));
        pcie_zf_writel_dbi(ep, reg, flags | BIT(3));
        if (barno != BAR_ROM)
        {
            pcie_zf_writel_dbi2(ep, reg + PCIE_NEXT_BAR_OFFSET, upper_32_bits(size - 1));
            pcie_zf_writel_dbi(ep, reg + PCIE_NEXT_BAR_OFFSET, 0);
        }
        pcie_zf_dbi_ro_wr_dis(ep);
    }

    return 0;
}

static int pcie_dpu_ep_set_vf_bar(struct pci_epc *epc, u8 func_no, u8 vfunc_no,
                                  struct pci_epf_bar *epf_bar)
{
    int ret = 0;
    struct pcie_dpu_ep *ep = epc_get_drvdata(epc);
    enum pci_barno barno = epf_bar->barno;
    int flags = epf_bar->flags;
    u32 reg = 0;
    u32 func_offset = 0;
    int sriov_cap_offset = 0;
    u32 pf_func_no = func_no & PCIE_DPU_EP_GET_PF_NO;
    u64 dpu_addr = epf_bar->phys_addr | ZF_PREFIX_ADDR;

    if (!is_pcie_ep_link(ep->ep_id))
    {
        DH_LOG_ERR(MODULE_MPF, "err: ep%d not link\n", ep->ep_id);
        return -ENODEV;
    }

    if (flags & PCI_BASE_ADDRESS_SPACE)
    {
        DH_LOG_ERR(MODULE_MPF, "error:vf bar must be mem\n");
        return -EINVAL;
    }

    DH_LOG_DEBUG(MODULE_MPF, "pf%x vf%x bar->flags:%d\n", pf_func_no, vfunc_no, flags);

    if (barno != BAR_4)
    {
        ret = pcie_dpu_ep_inbound_atu(ep, func_no, barno,
                                      dpu_addr, PCIE_DPU_AS_MEM);
        if (ret)
            return ret;
    }
    else
    {
        return 0;
    }

    func_offset = pcie_dpu_ep_func_select(pf_func_no, 0);
    sriov_cap_offset = pcie_dpu_ep_find_extcap(ep, func_offset, PCI_EXT_CAP_ID_SRIOV, 0);
    if (sriov_cap_offset < 0)
    {
        DH_LOG_ERR(MODULE_MPF, "find_extcap failed!!\n");
        return -ENXIO;
    }

    reg = func_offset + sriov_cap_offset;
    pcie_dpu_ep_sriov_disable(ep, reg);
    if (epf_bar->size)
    {
        pcie_zf_writel_dbi2(ep, reg + PCIE_SRIOV_ECAP_BAR0_OFFSET + barno * PCIE_NEXT_BAR_OFFSET, 1);
        pcie_zf_writel_dbi2(ep, reg + PCIE_SRIOV_ECAP_BAR0_OFFSET + barno * PCIE_NEXT_BAR_OFFSET, lower_32_bits(epf_bar->size - 1));
        pcie_zf_writel_dbi2(ep, reg + PCIE_SRIOV_ECAP_BAR0_OFFSET + (barno + 1) * PCIE_NEXT_BAR_OFFSET, upper_32_bits(epf_bar->size - 1));
    }
    pcie_zf_dbi_ro_wr_en(ep);
    pcie_zf_writel_dbi(ep, reg + PCIE_SRIOV_ECAP_BAR0_OFFSET + barno * PCIE_NEXT_BAR_OFFSET, epf_bar->flags | BIT(3));
    pcie_zf_dbi_ro_wr_dis(ep);

    pcie_dpu_ep_sriov_enable(ep, reg);

    return 0;
}

static int pcie_dpu_ep_set_bar(struct pci_epc *epc, u8 func_no, u8 vfunc_no, struct pci_epf_bar *epf_bar)
{
    DH_LOG_DEBUG(MODULE_MPF, "func:0x%x vfunc:0x%x\n", func_no, vfunc_no);
    if (isPF(func_no))
    {
        return pcie_dpu_ep_set_pf_bar(epc, func_no, vfunc_no, epf_bar);
    }
    else if (vfunc_no == 0)
    {
        return pcie_dpu_ep_set_vf_bar(epc, func_no, vfunc_no, epf_bar);
    }

    return 0;
}

static void pcie_dpu_ep_clear_bar(struct pci_epc *epc, u8 func_no, u8 vfunc_no,
                                  bool clear_vf, struct pci_epf_bar *epf_bar)
{
    u32 vf_bar_off = 0;
    int atu_index = 0;
    u32 bar_to_atu_index = 0;
    enum pci_barno barno = epf_bar->barno;
    struct pcie_dpu_ep *ep = epc_get_drvdata(epc);

    if (ep == NULL)
    {
        DH_LOG_ERR(MODULE_MPF, "ep is NULL\r\n");
        return;
    }

    if ((func_no & PCIE_DPU_EP_GET_PF_NO) >= PCIE_DPU_PF_NUMS)
    {
        DH_LOG_ERR(MODULE_MPF, "func_no is err!\n");
        return;
    }

    if ((barno == BAR_4) || (!isPF(func_no) && !vfunc_no))
    {
        return;
    }

    if (!isPF(func_no))
    {
        vf_bar_off = PCIE_VF_BARS_OFF;
    }

    bar_to_atu_index = (u32)barno + vf_bar_off;
    if (bar_to_atu_index < (PCI_STD_NUM_BARS * 2 + 1))
    {
        atu_index = ep->bar_to_atu[func_no & PCIE_DPU_EP_GET_PF_NO][bar_to_atu_index];
    }
    else
    {
        DH_LOG_ERR(MODULE_MPF, "error bar_to_atu index %d\r\n", bar_to_atu_index);
        return;
    }

    spin_lock(&ep->ib_window_lock);
    clear_bit(atu_index, ep->ib_window_map);
    spin_unlock(&ep->ib_window_lock);

    if(!is_pcie_ep_link(ep->ep_id))
    {
        DH_LOG_ERR(MODULE_MPF, "err: ep%d not link\n", ep->ep_id);
        return;
    }

    pcie_dpu_ep_reset_bar(ep, func_no, vfunc_no, barno, epf_bar->flags);

    pcie_zf_disable_atu(ep, atu_index, PCIE_DPU_REGION_INBOUND);
}

static int pcie_dpu_ep_map_addr(struct pci_epc *epc, u8 func_no, u8 vfunc_no,
                                phys_addr_t dpu_offset, u64 host_addr, size_t size)
{
    int ret = 0;
    struct pcie_dpu_ep *ep = epc_get_drvdata(epc);

    if (!is_pcie_ep_link(ep->ep_id))
    {
        DH_LOG_ERR(MODULE_MPF, "err: ep%d not link\n", ep->ep_id);
        return -ENODEV;
    }

    ret = pcie_dpu_ep_outbound_atu(ep, func_no, vfunc_no, dpu_offset - zf_ep->vsock_paddr, host_addr, size);
    if (ret)
    {
        DH_LOG_ERR(MODULE_MPF, "Failed to enable address\r\n");
        return ret;
    }

    return 0;
}

static void pcie_dpu_ep_unmap_addr(struct pci_epc *epc, u8 func_no,
                                   u8 vfunc_no, phys_addr_t dpu_offset)
{
    int ret = 0;
    u32 atu_index = 0;
    struct pcie_dpu_ep *ep = epc_get_drvdata(epc);

    if (ep == NULL)
    {
        DH_LOG_ERR(MODULE_MPF, "ep is NULL\r\n");
        return;
    }

    ret = pcie_zf_find_index(ep, dpu_offset - zf_ep->vsock_paddr, &atu_index);
    if (ret < 0)
    {
        return;
    }
    clear_bit(atu_index, ep->ob_window_map);

    if(!is_pcie_ep_link(ep->ep_id))
    {
        DH_LOG_ERR(MODULE_MPF, "err: ep%d not link\n", ep->ep_id);
        return;
    }

    pcie_zf_disable_atu(ep, atu_index, PCIE_DPU_REGION_OUTBOUND);
}

static int pcie_dpu_ep_set_msi(struct pci_epc *epc, u8 func_no, u8 vfunc_no, u8 interrupts)
{
    DH_LOG_ERR(MODULE_MPF, "error:pcie_dpu_ep can't set msi#\n");
    return -ESRCH;
}

static int pcie_dpu_ep_get_msi(struct pci_epc *epc, u8 func_no, u8 vfunc_no)
{
    DH_LOG_ERR(MODULE_MPF, "error:pcie_dpu_ep can't get msi#\n");
    return -ESRCH;
}

static int pcie_dpu_ep_set_msix(struct pci_epc *epc, u8 func_no, u8 vfunc_no,
                                u16 interrupts, enum pci_barno bir, u32 bar_offset)
{
    /*
     * MSIX tables are fixed in BAR4 (mapped to PCIe IP), other configs
     * will make MSIX unable to function.
     */
    dev_warn(&epc->dev, "MSIX config is not supported by ZF epc\n");

    return 0;
}

int pcie_dpu_ep_get_msix(struct pci_epc *epc, u8 func_no, u8 vfunc_no)
{
    struct pcie_dpu_ep *dpu_dev = epc_get_drvdata(epc);
    u32 val, reg;
    u32 func_offset = 0;
    struct pcie_dpu_ep_func *ep_func;

    if (!is_pcie_ep_link(dpu_dev->ep_id))
    {
        DH_LOG_ERR(MODULE_MPF, "err: ep%d not link\n", dpu_dev->ep_id);
        return -ENODEV;
    }

    ep_func = pcie_dpu_ep_get_func_from_ep(dpu_dev, func_no, vfunc_no);
    if (!ep_func || !ep_func->msix_cap)
        return -EINVAL;

    func_offset = pcie_dpu_ep_func_select(func_no, vfunc_no);

    reg = func_offset + ep_func->msix_cap + PCI_MSIX_FLAGS;
    val = pcie_zf_readw_dbi(dpu_dev, reg);
    if (!(val & PCI_MSIX_FLAGS_ENABLE))
        return -EINVAL;

    val &= PCI_MSIX_FLAGS_QSIZE;

    return val;
}

int pcie_dpu_ep_raise_legacy_irq(struct pci_epc *epc, u8 func_no, u8 vfunc_no)
{
    DH_LOG_ERR(MODULE_MPF, "EP cannot trigger legacy IRQs\r\n");

    return -EINVAL;
}

int pcie_dpu_ep_raise_msi_irq(struct pci_epc *epc, u8 func_no, u8 vfunc_no, u8 interrupt_num)
{
    DH_LOG_ERR(MODULE_MPF, "EP cannot trigger msi IRQs\r\n");

    return -EINVAL;
}

int pcie_dpu_ep_raise_msix_irq(struct pci_epc *epc, u8 func_no, u8 vfunc_no, u8 interrupt_num)
{
    struct pcie_dpu_ep *dpu_dev = epc_get_drvdata(epc);
    u32 msg_data = 0;

    if (!is_pcie_ep_link(dpu_dev->ep_id))
    {
        DH_LOG_ERR(MODULE_MPF, "err: ep%d not link\n", dpu_dev->ep_id);
        return -ENODEV;
    }

    // DH_LOG_INFO(MODULE_MPF, "func_no = 0x%x, vfunc_no = 0x%x, interrupt_num = 0x%x\n", func_no, vfunc_no, interrupt_num);

    if (isPF(func_no))
    {
        msg_data = (func_no << PCIE_MSIX_DOORBELL_PF_SHIFT) | (interrupt_num);
    }
    else
    {
        msg_data = ((func_no & PCIE_DPU_EP_GET_PF_NO) << PCIE_MSIX_DOORBELL_PF_SHIFT) |
                   MSIX_DOORBELL_VF_ACTIVE | (vfunc_no << PCIE_MSIX_DOORBELL_VF_SHIFT) |
                   (interrupt_num);
    }

    pcie_zf_writel_dbi(dpu_dev, PCIE_MSIX_DOORBELL, msg_data);

    return 0;
}

static int pcie_dpu_ep_raise_irq(struct pci_epc *epc, u8 func_no, u8 vfunc_no,
                                 enum pci_epc_irq_type type, u16 interrupt_num)
{
    switch (type)
    {
        case PCI_EPC_IRQ_LEGACY:
            return pcie_dpu_ep_raise_legacy_irq(epc, func_no, vfunc_no);
        case PCI_EPC_IRQ_MSI:
            return pcie_dpu_ep_raise_msi_irq(epc, func_no, vfunc_no, interrupt_num - 1);
        case PCI_EPC_IRQ_MSIX:
            return pcie_dpu_ep_raise_msix_irq(epc, func_no, vfunc_no, interrupt_num - 1);
        default:
            DH_LOG_ERR(MODULE_MPF, "UNKNOWN IRQ type\r\n");
    }
    return 0;
}

static int pcie_dpu_ep_get_max_vfs(struct pci_epc *epc, u8 func_no)
{
    struct pcie_dpu_ep *dpu_dev = NULL;
    u32 vf_total_num = 0;

    if (epc == NULL)
    {
        DH_LOG_ERR(MODULE_MPF, "epc is NULL!!!\n");
        return -EINVAL;
    }
    dpu_dev = epc_get_drvdata(epc);

    vf_total_num = func_no & PCIE_DPU_EP_GET_PF_NO;
    if (vf_total_num >= PCIE_DPU_PF_NUMS)
    {
        DH_LOG_ERR(MODULE_MPF, "error vf_total_num=%d\n", vf_total_num);
        return -EINVAL;
    }

    DH_LOG_INFO(MODULE_MPF, "get vf max_num:%d\n", dpu_dev->vf_total_num[vf_total_num]);
    return dpu_dev->vf_total_num[vf_total_num];
}

static void pcie_dpu_ep_stop(struct pci_epc *epc)
{
    DH_LOG_INFO(MODULE_MPF, "%s\n", __func__);
    return;
}

static int pcie_dpu_ep_start(struct pci_epc *epc)
{
    DH_LOG_INFO(MODULE_MPF, "%s\n", __func__);
    return 0;
}

static const struct pci_epc_features pcie_zf_epc_features = {
    .linkup_notifier = false,
    .msi_capable = false,
    .msix_capable = true,
    .reserved_bar = PCIE_DPU_EP_REAERVED_BAR,
    .bar_fixed_64bit = PCIE_DPU_EP_BAR_FIXED_64BIT,
    .align = PCIE_DPU_EP_ALIGN,
};

static const struct pci_epc_features *
pcie_dpu_ep_get_features(struct pci_epc *epc, u8 func_no, u8 vfunc_no)
{
    return &pcie_zf_epc_features;
}

static int pci_dpu_ep_get_port_id(struct pci_epc *epc,
                                  enum pci_epc_port_id_type *id)
{
    struct pcie_dpu_ep *dpu_dev = epc_get_drvdata(epc);

    *id = dpu_dev->ep_id + 5;
    return 0;
}

static int pci_dpu_ep_calc_pfns(struct pci_epc *epc, phys_addr_t phys,
                                size_t n_pfns, unsigned long *pfn)
{
    enum pci_epc_port_id_type port = 0;
    phys_addr_t offset = 0, base = zf_ep->vsock_paddr;
    size_t i = 0;

    if (phys < base ||
        phys - base + (n_pfns << PAGE_SHIFT) > zf_ep->ob_size)
        return -EINVAL;

    pci_dpu_ep_get_port_id(epc, &port);

    for (i = 0; i < n_pfns; i++)
    {
        offset = phys - base + (i << PAGE_SHIFT);
        pfn[i] = (base + EP_DPU_PA(offset, port)) >> PAGE_SHIFT;
    }
    return 0;
}

#endif

static const struct pci_epc_ops epc_ops = {
    .write_header = pcie_dpu_ep_write_header,
    .set_bar = pcie_dpu_ep_set_bar,
    .clear_bar = pcie_dpu_ep_clear_bar,
    .map_addr = pcie_dpu_ep_map_addr,
    .unmap_addr = pcie_dpu_ep_unmap_addr,
    .set_msi = pcie_dpu_ep_set_msi,
    .get_msi = pcie_dpu_ep_get_msi,
    .set_msix = pcie_dpu_ep_set_msix,
    .get_msix = pcie_dpu_ep_get_msix,
    .raise_irq = pcie_dpu_ep_raise_irq,
    .get_max_vfs = pcie_dpu_ep_get_max_vfs,
    .start = pcie_dpu_ep_start,
    .stop = pcie_dpu_ep_stop,
    .get_features = pcie_dpu_ep_get_features,
    .get_port_id = pci_dpu_ep_get_port_id,
    .calc_pfns = pci_dpu_ep_calc_pfns,
    .get_xdma_chan = zf_pcie_get_hdma_chan,
};

/*======================================================cdev of epc======================================================*/
#if 1
static int pcie_epc_cdev_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int pcie_epc_cdev_release(struct inode *inode, struct file *file)
{
    return 0;
}

static int pcie_epc_iatu_send_msg(struct pcie_epc_iatu_cfg *iatu_cfg)
{
    int ret = 0;
    char recv_buffer[RECV_BUFFER_SIZE] = {0};
    struct zxdh_pci_bar_msg in = {0};
    struct zxdh_msg_recviver_mem result = {0};

    if (RECV_BUFFER_SIZE < (sizeof(struct pcie_epc_iatu_cfg) + BAR_MSG_HEADER_SIZE))
    {
        DH_LOG_ERR(MODULE_MPF, "recv_buffer size is smaller than sizeof struct iatu_cfg\n");
        return -ENOMEM;
    }

    in.virt_addr = (u64)zf_ep->mpf_vaddr + ZXDH_BAR1_CHAN_OFFSET;
    in.payload_addr = iatu_cfg;
    in.payload_len = sizeof(struct pcie_epc_iatu_cfg);
    in.src = MSG_CHAN_END_PF;
    in.dst = MSG_CHAN_END_RISC;
    in.event_id = MODULE_DPU_PCIE_PF_INFO; /* 事件号 */
    in.src_pcieid = 0;

    result.recv_buffer = recv_buffer;
    result.buffer_len = sizeof(struct pcie_epc_iatu_cfg) + BAR_MSG_HEADER_SIZE;

    DH_LOG_DEBUG(MODULE_MPF, "send msg   cmd:%d, ep%d pf%d\n", iatu_cfg->cmd_id, iatu_cfg->ep_id, iatu_cfg->pf_id);
    ret = zxdh_bar_chan_sync_msg_send(&in, &result);
    if (ret)
    {
        DH_LOG_ERR(MODULE_MPF, "%s zxdh_bar_chan_sync_msg_send failed.\n", __func__);
        return ret;
    }

    memcpy(iatu_cfg, (char *)(&recv_buffer) + BAR_MSG_HEADER_SIZE, sizeof(struct pcie_epc_iatu_cfg));

    return ret;
}

static int pcie_epc_ioctl_send_msg(struct pcie_epc_ioctl_pf_cfg *pf_cfg)
{
    int ret = 0;
    char recv_buffer[RECV_BUFFER_SIZE] = {0};
    struct zxdh_pci_bar_msg in = {0};
    struct zxdh_msg_recviver_mem result = {0};

    if (RECV_BUFFER_SIZE < (sizeof(struct pcie_epc_ioctl_pf_cfg) + BAR_MSG_HEADER_SIZE))
    {
        DH_LOG_ERR(MODULE_MPF, "recv_buffer size is smaller than sizeof struct dpu_pf_cfg\n");
        return -ENOMEM;
    }

    in.virt_addr = (u64)zf_ep->mpf_vaddr + ZXDH_BAR1_CHAN_OFFSET;
    in.payload_addr = pf_cfg;
    in.payload_len = sizeof(struct pcie_epc_ioctl_pf_cfg);
    in.src = MSG_CHAN_END_PF;
    in.dst = MSG_CHAN_END_RISC;
    in.event_id = MODULE_DPU_PCIE_PF_INFO; /* 事件号 */
    in.src_pcieid = 0;

    result.recv_buffer = recv_buffer;
    result.buffer_len = sizeof(struct pcie_epc_ioctl_pf_cfg) + BAR_MSG_HEADER_SIZE;

    ret = zxdh_bar_chan_sync_msg_send(&in, &result);
    if (ret)
    {
        DH_LOG_ERR(MODULE_MPF, "%s zxdh_bar_chan_sync_msg_send failed.\n", __func__);
        return ret;
    }

    memcpy(pf_cfg, (char *)(&recv_buffer) + BAR_MSG_HEADER_SIZE, sizeof(struct pcie_epc_ioctl_pf_cfg));

    return ret;
}

static int copy_pf_cfg_to_user(u64 arg, enum pcie_epcbar_msg_cmd cmd_id, struct pcie_epc_ioctl_pf_cfg *pf_cfg)
{
    if (CMD_GET_CONFIG == cmd_id)
    {
        if (copy_to_user((void __user *)arg, pf_cfg, sizeof(struct pcie_epc_ioctl_pf_cfg)))
        {
            DH_LOG_ERR(MODULE_MPF, "failed to copy config to user space\n");
            return -EFAULT;
        }
    }

    return 0;
}

static int pcie_epc_ioctl_config_process(u64 arg, enum pcie_epcbar_msg_cmd cmd_id)
{
    int ret = 0;
    struct pcie_epc_ioctl_pf_cfg pf_cfg = {0};
    struct pcie_epc_ioctl_pf_cfg zero_cfg = {0};

    if (copy_from_user(&pf_cfg, (void __user *)arg, sizeof(struct pcie_epc_ioctl_pf_cfg)))
    {
        DH_LOG_ERR(MODULE_MPF, "failed to copy config from user space\n");
        ret = -EFAULT;
        goto copy_zero_to_user;
    }

    if ((pf_cfg.ep_id >= PCIE_DPU_EP_NUMS) || (pf_cfg.pf_id >= PCIE_DPU_PF_NUMS))
    {
        DH_LOG_ERR(MODULE_MPF, "invalid EP_ID or PF_ID\n");
        ret = -EINVAL;
        goto copy_zero_to_user;
    }

    DH_LOG_DEBUG(MODULE_MPF, "configuration - EP_ID: 0x%x, PF_ID: 0x%x cmd:%d\n",
                 pf_cfg.ep_id, pf_cfg.pf_id, cmd_id);

    /* 加锁保护全局配置数据 */
    mutex_lock(&pcie_epc_cdev_mutex);

    pf_cfg.cmd_id = cmd_id;
    ret = pcie_epc_ioctl_send_msg(&pf_cfg);
    if (ret)
    {
        mutex_unlock(&pcie_epc_cdev_mutex);
        goto copy_zero_to_user;
    }
    mutex_unlock(&pcie_epc_cdev_mutex);

    ret = copy_pf_cfg_to_user(arg, cmd_id, &pf_cfg);
    return ret;

copy_zero_to_user:
    copy_pf_cfg_to_user(arg, cmd_id, &zero_cfg);
    return ret;
}

static int pcie_epc_set_config(u64 arg)
{
    return pcie_epc_ioctl_config_process(arg, CMD_SET_CONFIG);
}

static int pcie_epc_get_config(u64 arg)
{
    return pcie_epc_ioctl_config_process(arg, CMD_GET_CONFIG);
}

static long pcie_epc_cdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    u32 i = 0;
    u32 ioctl_func_nums = sizeof(ioctl_func_sel) / sizeof(struct pcie_epc_ioctl_sel);

    for (i = 0; i < ioctl_func_nums; i++)
    {
        if (ioctl_func_sel[i].ioctl_cmd == cmd)
        {
            return ioctl_func_sel[i].ioctl_func(arg);
        }
    }

    return -EINVAL;
}

static struct file_operations pcie_epc_cdev_fops = {
    .owner = THIS_MODULE,
    .open = pcie_epc_cdev_open,
    .release = pcie_epc_cdev_release,
    .unlocked_ioctl = pcie_epc_cdev_ioctl,
};

static int pci_zte_epc_cdev_init(void)
{
    int ret = 0;

    if (pcie_epc_cdev_major)
    {
        pcie_epc_cdev_num = MKDEV(pcie_epc_cdev_major, 0);
        ret = register_chrdev_region(pcie_epc_cdev_num, 1, PCIE_EPC_DRIVER_NAME);
    }
    else
    {
        ret = alloc_chrdev_region(&pcie_epc_cdev_num, 0, 1, PCIE_EPC_DRIVER_NAME);
        pcie_epc_cdev_major = MAJOR(pcie_epc_cdev_num);
    }

    if (ret < 0)
    {
        DH_LOG_ERR(MODULE_MPF, "Failed to register pcie epc cdev region\n");
        return ret;
    }

    cdev_init(&pcie_epc_cdev, &pcie_epc_cdev_fops);
    pcie_epc_cdev.owner = THIS_MODULE;

    ret = cdev_add(&pcie_epc_cdev, pcie_epc_cdev_num, 1);
    if (ret < 0)
    {
        DH_LOG_ERR(MODULE_MPF, "PCIe driver: failed to add cdev\n");
        goto err_cdev;
    }

    /* 创建设备类 */
    pcie_epc_cdev_class = class_create(THIS_MODULE, PCIE_EPC_DRIVER_CLASS);
    if (IS_ERR(pcie_epc_cdev_class))
    {
        DH_LOG_ERR(MODULE_MPF, "PCIe driver: failed to create device class\n");
        ret = -ENOMEM;
        goto err_class;
    }

    /* 创建设备节点 */
    if (NULL == device_create(pcie_epc_cdev_class, NULL, pcie_epc_cdev_num, NULL, PCIE_EPC_DRIVER_NAME))
    {
        DH_LOG_ERR(MODULE_MPF, "PCIe driver: failed to create device\n");
        ret = -ENOMEM;
        goto err_device;
    }

    return 0;

err_device:
    class_destroy(pcie_epc_cdev_class);
err_class:
    cdev_del(&pcie_epc_cdev);
err_cdev:
    unregister_chrdev_region(pcie_epc_cdev_num, 1);
    return ret;
}

static void pci_zte_epc_cdev_exit(void)
{
    device_destroy(pcie_epc_cdev_class, pcie_epc_cdev_num);
    class_destroy(pcie_epc_cdev_class);
    cdev_del(&pcie_epc_cdev);
    unregister_chrdev_region(pcie_epc_cdev_num, 1);
}
#endif

/*======================================================init of epc======================================================*/
#if 1
static void dpu_ep_default_set(int ep_id)
{
    u32 pf_idx = 0;
    u32 func_offset = 0;
    int sriov_cap_offset = 0;
    u32 reg = 0;
    struct pcie_dpu_ep *ep = zf_ep->dpu_ep_array[ep_id];

    pcie_zf_dbi_ro_wr_en(ep);
    for (pf_idx = 0; pf_idx < PCIE_DPU_PF_NUMS; ++pf_idx)
    {
        if (!(ep->permissible_pf_map & (0x1 << pf_idx)))
        {
            continue;
        }
        pcie_zf_writel_dbi(ep, pf_idx * PCIE_DPU_EP_FUNC_CFG_SIZE, PCIE_DPU_PF_INITIAL_ID);
        pcie_zf_writel_dbi(ep, pf_idx * PCIE_DPU_EP_FUNC_CFG_SIZE + PCI_CLASS_REVISION, PCIE_DPU_PF_DEFAUTL_CLASSCODE);
        pcie_zf_writel_dbi2(ep, pf_idx * PCIE_DPU_EP_FUNC_CFG_SIZE + PCI_ROM_ADDRESS, ZF_DISABLE);

        pcie_zf_writel_dbi2(ep, pf_idx * PCIE_DPU_EP_FUNC_CFG_SIZE + PCI_BASE_ADDRESS_4, ZF_ENABLE);
        pcie_zf_writel_dbi2(ep, pf_idx * PCIE_DPU_EP_FUNC_CFG_SIZE + PCI_BASE_ADDRESS_4, lower_32_bits(BAR4_DEFAULT_SIZE - 1));
        pcie_zf_writel_dbi2(ep, pf_idx * PCIE_DPU_EP_FUNC_CFG_SIZE + PCI_BASE_ADDRESS_4 + PCIE_NEXT_BAR_OFFSET, upper_32_bits(BAR4_DEFAULT_SIZE - 1));
        pcie_zf_writel_dbi(ep, pf_idx * PCIE_DPU_EP_FUNC_CFG_SIZE + PCI_BASE_ADDRESS_4, PCIE_DEFAULT_BAR_FLAG);

        func_offset = pcie_dpu_ep_func_select(pf_idx, 0);
        sriov_cap_offset = pcie_dpu_ep_find_extcap(ep, func_offset, PCI_EXT_CAP_ID_SRIOV, 0);
        if (sriov_cap_offset < 0)
        {
            DH_LOG_ERR(MODULE_MPF, "find_extcap failed!!\n");
            return;
        }

        reg = func_offset + sriov_cap_offset;
        pcie_dpu_ep_sriov_disable(ep, reg);
        pcie_zf_writel_dbi(ep, reg + PCIE_SRIOV_ECAP_BAR4_OFFSET, PCIE_DEFAULT_BAR_FLAG);
        pcie_zf_writel_dbi2(ep, reg + PCIE_SRIOV_ECAP_BAR4_OFFSET, 0x1);
        pcie_zf_writel_dbi2(ep, reg + PCIE_SRIOV_ECAP_BAR4_OFFSET, lower_32_bits(BAR4_DEFAULT_SIZE - 1));
        pcie_zf_writel_dbi2(ep, reg + PCIE_SRIOV_ECAP_BAR4_OFFSET + PCIE_NEXT_BAR_OFFSET, upper_32_bits(BAR4_DEFAULT_SIZE - 1));
        pcie_dpu_ep_sriov_enable(ep, reg);
    }
    pcie_zf_dbi_ro_wr_dis(ep);
}

static int dpu_ep_iatu_init(struct device *dev, int id)
{
    int iatu_no = 0;

    zf_ep->dpu_ep_array[id]->num_ib_windows = PCIE_DPU_IATU_NUMS;
    zf_ep->dpu_ep_array[id]->num_ob_windows = PCIE_DPU_IATU_NUMS;
    zf_ep->dpu_ep_array[id]->ib_window_map = devm_kcalloc(dev, BITS_TO_LONGS(zf_ep->dpu_ep_array[id]->num_ib_windows),
                                                          sizeof(long), GFP_KERNEL);
    if (!zf_ep->dpu_ep_array[id]->ib_window_map)
    {
        DH_LOG_ERR(MODULE_MPF, "get ib_map err\n");
        return -ENOMEM;
    }

    zf_ep->dpu_ep_array[id]->ob_window_map = devm_kcalloc(dev, BITS_TO_LONGS(zf_ep->dpu_ep_array[id]->num_ob_windows),
                                                          sizeof(long), GFP_KERNEL);
    if (!zf_ep->dpu_ep_array[id]->ob_window_map)
    {
        DH_LOG_ERR(MODULE_MPF, "get ob_map err\n");
        goto free_ib_map;
    }

    zf_ep->dpu_ep_array[id]->ob_src_addr = devm_kcalloc(dev, zf_ep->dpu_ep_array[id]->num_ob_windows, sizeof(phys_addr_t),
                                                        GFP_KERNEL);
    if (!zf_ep->dpu_ep_array[id]->ob_src_addr)
    {
        DH_LOG_ERR(MODULE_MPF, "get ob_src_addr err\n");
        goto free_ob_map;
    }

    // 确定哪些inbound已经被使用
    for (iatu_no = 0; iatu_no < PCIE_DPU_IATU_NUMS; iatu_no++)
    {
        if (zf_atu_is_used(zf_ep->dpu_ep_array[id], iatu_no))
        {
            set_bit(iatu_no, zf_ep->dpu_ep_array[id]->ib_window_map);
        }
    }

    spin_lock_init(&zf_ep->dpu_ep_array[id]->ib_window_lock);

    return 0;

free_ob_map:
    devm_kfree(dev, zf_ep->dpu_ep_array[id]->ob_window_map);
free_ib_map:
    devm_kfree(dev, zf_ep->dpu_ep_array[id]->ib_window_map);
    return -ENOMEM;
}

static int dpu_ep_func_list_init(struct device *dev, int id)
{
    u8 func_no = 0, vfunc_no = 0;
    u8 vf_total_num = 0;
    u32 func_offset = 0;
    struct pcie_dpu_ep_func *ep_func = NULL;

    INIT_LIST_HEAD(&zf_ep->dpu_ep_array[id]->func_list);
    for (func_no = 0; func_no < PCIE_DPU_PF_NUMS; ++func_no)
    {
        if (!(zf_ep->dpu_ep_array[id]->permissible_pf_map & (0x1 << func_no)))
        {
            continue;
        }
        ep_func = devm_kzalloc(dev, sizeof(*ep_func), GFP_KERNEL);
        if (!ep_func)
        {
            return -ENOMEM;
        }

        ep_func->func_no = func_no;
        func_offset = pcie_dpu_ep_func_select(func_no, vfunc_no);
        ep_func->msix_cap = pcie_dpu_ep_find_cap(zf_ep->dpu_ep_array[id], func_offset, PCI_CAP_ID_MSIX);

        list_add_tail(&ep_func->list, &zf_ep->dpu_ep_array[id]->func_list);

        vf_total_num = (u8)zf_ep->dpu_ep_array[id]->vf_total_num[func_no];
        for (vfunc_no = 0; vfunc_no < vf_total_num; ++vfunc_no)
        {
            ep_func = devm_kzalloc(dev, sizeof(*ep_func), GFP_KERNEL);
            if (!ep_func)
            {
                return -ENOMEM;
            }

            ep_func->func_no = PCIE_DPU_EP_FUNC_IS_VF | func_no;
            ep_func->vfunc_no = vfunc_no;
            func_offset = pcie_dpu_ep_func_select(ep_func->func_no, vfunc_no);
            ep_func->msix_cap = pcie_dpu_ep_find_cap(zf_ep->dpu_ep_array[id],
                                                     func_offset, PCI_CAP_ID_MSIX);

            list_add_tail(&ep_func->list, &zf_ep->dpu_ep_array[id]->func_list);
        }
    }

    return 0;
}

static int dpu_ep_vf_total_num_get(int id)
{
    int func_no = 0, func_offset = 0, sriov_cap_offset = 0, vf_total_num_addr = 0;

    for (func_no = 0; func_no < PCIE_DPU_PF_NUMS; ++func_no)
    {
        func_offset = pcie_dpu_ep_func_select(func_no, 0);
        sriov_cap_offset = pcie_dpu_ep_find_extcap(zf_ep->dpu_ep_array[id], func_offset, PCI_EXT_CAP_ID_SRIOV, 0);
        if (sriov_cap_offset < 0)
        {
            DH_LOG_ERR(MODULE_MPF, "find_extcap failed!!\n");
            return -ENXIO;
        }
        vf_total_num_addr = func_offset + sriov_cap_offset + PCIE_SRIOV_TOTAL_VFS;
        zf_ep->dpu_ep_array[id]->vf_total_num[func_no] = pcie_zf_readw_dbi(zf_ep->dpu_ep_array[id], vf_total_num_addr);
    }

    return 0;
}

static int dpu_ep_get_permissible_pf(int id)
{
    int func_no = 0, func_offset = 0;
    u32 func_id = 0;

    for (func_no = 0; func_no < PCIE_DPU_PF_NUMS; ++func_no)
    {
        func_offset = pcie_dpu_ep_func_select(func_no, 0);
        func_id = pcie_zf_readl_dbi(zf_ep->dpu_ep_array[id], func_offset);
        if (PCIE_DPU_PF_DEFAUTL_ID1 == func_id || PCIE_DPU_PF_DEFAUTL_ID2 == func_id ||
            PCIE_DPU_PF_DEFAUTL_ID3 == func_id || PCIE_DPU_PF_DEFAUTL_ID4 == func_id)
        {
            zf_ep->dpu_ep_array[id]->permissible_pf_map |= (0x1 << func_no);
        }
    }

    return 0;
}

static void epc_dev_release(struct device *dev)
{
}

static int pci_zte_epc_dev_init_one(struct pci_dev *pdev, int id)
{
    struct pci_epc *epc = NULL;
    struct device *dev = NULL;
    struct device_node *np = NULL;
    struct platform_device *zf_pdev = NULL;
#if ZF_MPF_HDMA_MODE_ENABLE
    struct platform_device *zf_pdev_dma = NULL;
#endif
    char class_name[PCIE_DPU_EP_CLASS_NAME] = {0};
    int node = 0, i = 0;
    int ret = -ENOMEM;

    ret = snprintf(class_name, sizeof(class_name) - 1, "zf_epc_class%d", id);
    if ((ret < 0) || (ret >= PCIE_DPU_EP_CLASS_NAME))
    {
        DH_LOG_ERR(MODULE_MPF, "get ep name failed\n");
        return -ENOMEM;
    }

    ret = -ENOMEM;

    zf_pdev = platform_device_register_simple(class_name, -1, NULL, 0);
    if (!zf_pdev)
    {
        DH_LOG_ERR(MODULE_MPF, "Error platform_device_register zf_pdev failed\n");
        return ret;
    }

    ret = snprintf(class_name, sizeof(class_name) - 1, "zf_epc_dma_rd%d", id);
    if (ret < 0 || ret > sizeof(class_name))
    {
        platform_device_unregister(zf_pdev);
        return ret;
    }

#if ZF_MPF_HDMA_MODE_ENABLE
    zf_pdev_dma = platform_device_register_simple(class_name, -1, NULL, 0);
    if (!zf_pdev_dma)
    {
        DH_LOG_ERR(MODULE_MPF, "Error platform_device_register zf_pdev_dma failed\n");
        platform_device_unregister(zf_pdev);
        return ret;
    }
    zf_pdev->dev.driver = pdev->dev.driver;
    zf_pdev_dma->dev.driver = pdev->dev.driver;
#endif

    dev = &zf_pdev->dev;
    np = dev->of_node;
    epc = pci_epc_create(dev, &epc_ops);
    if (IS_ERR_OR_NULL(epc))
    {
        DH_LOG_ERR(MODULE_MPF, "Failed %ld to create epc device\n", PTR_ERR(epc));
        ret = -EPERM;
        goto free_pdev;
    }

    epc->dev.release = epc_dev_release;
    epc->max_functions = PCIE_DPU_PF_NUMS;
    epc->is_dpu_epc = 1;

    ret = pci_epc_mem_init(epc, zf_ep->vsock_paddr, zf_ep->ob_size, PAGE_SIZE);
    if (ret < 0)
    {
        DH_LOG_ERR(MODULE_MPF, "ep%d failed to initialize the memory space\n", id);
        goto free_epc;
    }

    node = dev_to_node(dev); // 多核
    if (node == NUMA_NO_NODE)
        set_dev_node(dev, first_memory_node);

    /*##################zf_dev init##################*/
    zf_ep->dpu_ep_array[id] = kzalloc_node(sizeof(struct pcie_dpu_ep), GFP_KERNEL, node);
    if (!zf_ep->dpu_ep_array[id])
    {
        DH_LOG_ERR(MODULE_MPF, "Error kzalloc node\n");
        ret = -ENOMEM;
        goto free_epc_mem;
    }

    zf_ep->dpu_ep_array[id]->ep_id = id;
    zf_ep->dpu_ep_array[id]->epc = epc;
    zf_ep->dpu_ep_array[id]->dbi_base = zf_ep->dbi_vaddr + PCIE_DPU_EP_DBI_SIZE * id;
    zf_ep->dpu_ep_array[id]->atu_base = zf_ep->dpu_ep_array[id]->dbi_base + DEFAULT_DBI_ATU_OFFSET;
    zf_ep->dpu_ep_array[id]->zf_pdev = zf_pdev;
#if ZF_MPF_HDMA_MODE_ENABLE
    zf_ep->dpu_ep_array[id]->zf_pdev_dma = zf_pdev_dma;
#endif

    dpu_ep_default_set(id);

    epc_set_drvdata(epc, zf_ep->dpu_ep_array[id]);

    dpu_ep_get_permissible_pf(id);
    for (i = 0; i < PCIE_DPU_PF_NUMS; i++)
    {
        if (!(zf_ep->dpu_ep_array[id]->permissible_pf_map & (0x1 << i)))
        {
            DH_LOG_INFO(MODULE_MPF, "ep%d pf%d can't uesd\n", id, i);
            set_bit(i, &epc->function_num_map);
        }
    }

    ret |= dpu_ep_iatu_init(&pdev->dev, id);
    ret |= dpu_ep_vf_total_num_get(id);
    ret |= dpu_ep_func_list_init(&pdev->dev, id);

    if (ret)
        goto free_dpu_ep;

    return 0;

free_dpu_ep:
    kfree(zf_ep->dpu_ep_array[id]);
free_epc_mem:
    pci_epc_mem_exit(epc);
free_epc:
    pci_epc_destroy(epc);
free_pdev:
    platform_device_unregister(zf_pdev);
#if ZF_MPF_HDMA_MODE_ENABLE
    platform_device_unregister(zf_pdev_dma);
#endif
    return ret;
}

static void pci_zte_epc_dev_free_one(struct pci_dev *pdev, int ep_idx)
{
    if (epc_init_flag[ep_idx])
    {
        pcie_zf_dma_free(zf_ep->dpu_ep_array[ep_idx], pdev);
        pci_epc_destroy(zf_ep->dpu_ep_array[ep_idx]->epc);
#if ZF_MPF_HDMA_MODE_ENABLE
        zf_ep->dpu_ep_array[ep_idx]->zf_pdev->dev.driver = NULL;
        zf_ep->dpu_ep_array[ep_idx]->zf_pdev_dma->dev.driver = NULL;
        platform_device_unregister(zf_ep->dpu_ep_array[ep_idx]->zf_pdev_dma);
#endif
        platform_device_unregister(zf_ep->dpu_ep_array[ep_idx]->zf_pdev);
        kfree(zf_ep->dpu_ep_array[ep_idx]);
        epc_init_flag[ep_idx] = 0;
    }
    else
    {
        DH_LOG_INFO(MODULE_MPF, "ep%d is not initialized, skip free\n", ep_idx);
    }
}

static void pci_zte_epc_dev_free(struct pci_dev *pdev)
{
    u8 ep_idx = 0;

    for (ep_idx = 0; ep_idx < PCIE_DPU_EP_NUMS; ep_idx++)
    {
        pci_zte_epc_dev_free_one(pdev, ep_idx);
    }
}

static int zf_dev_map(struct pci_dev *pdev)
{
    zf_ep->dbi_vaddr = ioremap(zf_ep->dbi_paddr, PCIE_DPU_EP_DBI_SIZE * PCIE_DPU_EP_NUMS);
    if (!zf_ep->dbi_vaddr)
    {
        pci_release_mem_regions(pdev);
        return -ENODEV;
    }

    zf_ep->vsock_vaddr = ioremap(zf_ep->vsock_paddr, pci_resource_len(pdev, BAR_0));
    if (!zf_ep->vsock_vaddr)
    {
        iounmap(zf_ep->dbi_vaddr);
        pci_release_mem_regions(pdev);
        return -ENODEV;
    }

    zf_ep->mpf_vaddr = ioremap(zf_ep->mpf_paddr, pci_resource_len(pdev, BAR_0));
    if (!zf_ep->mpf_vaddr)
    {
        iounmap(zf_ep->dbi_vaddr);
        pci_release_mem_regions(pdev);
        return -ENODEV;
    }

    return 0;
}

static void zf_dev_unmap(struct pci_dev *pdev)
{
    if (zf_ep->dbi_vaddr)
        iounmap(zf_ep->dbi_vaddr);
    if (zf_ep->vsock_vaddr)
        iounmap(zf_ep->vsock_vaddr);
    pci_release_mem_regions(pdev);
}

int pcie_zte_zf_signal_epc_dev_init(uint32_t ep_idx)
{
    int ret = 0;
    struct pci_dev *pdev = zf_ep->mpf_pdev;

    if (epc_init_flag[ep_idx])
    {
        DH_LOG_DEBUG(MODULE_MPF, "zf_mpf ep%d is already init\n", ep_idx);
        return ret;
    }

    ret = pci_zte_epc_dev_init_one(pdev, ep_idx);
    if (zf_ep->dpu_ep_array[ep_idx] == NULL)
    {
        DH_LOG_ERR(MODULE_MPF, "pci_zte_epc_dev_init_one ep%d failed\n", ep_idx);
        return -ENODEV;
    }
    DH_LOG_INFO(MODULE_MPF, "pci_zte_epc_dev_init ep%d success!\n", ep_idx);

    ret = pcie_zf_dma_init(zf_ep->dpu_ep_array[ep_idx], pdev);
    if (ret)
    {
        DH_LOG_ERR(MODULE_MPF, "pcie_zf_dma_init failed\n");
        return ret;
    }
    epc_init_flag[ep_idx] = 1;

    return ret;
}

static bool is_pcie_zte_zf_device_support_vf(struct pci_dev *pdev)
{
    int total_vfs = 0;

    total_vfs = pci_sriov_get_totalvfs(pdev);
    return total_vfs > 0;
}

static int pci_zte_zf_epc_dev_init(void)
{
    int ret = 0;
    int ep_idx = 0;

    for (ep_idx = 0; ep_idx < PCIE_DPU_EP_NUMS; ep_idx++)
    {
        if (epc_init_flag[ep_idx])
        {
            DH_LOG_ERR(MODULE_MPF, "ep%d is already init\n", ep_idx);
            continue;
        }

        if (!is_pcie_ep_link(ep_idx))
        {
            DH_LOG_ERR(MODULE_MPF, "ep%d is not link\n", ep_idx);
            continue;
        }

        pcie_zte_zf_signal_epc_dev_init(ep_idx);
    }

    return ret;
}
#endif

int pcie_zte_zf_epc_module_init(struct dh_core_dev *dh_dev, const struct pci_device_id *id)
{
    int ret = -ENXIO;
    struct pci_dev *vsock_pdev = NULL;

    if (IS_ERR_OR_NULL(dh_dev) || IS_ERR_OR_NULL(id))
    {
        DH_LOG_ERR(MODULE_MPF, "dh_dev or id is NULL\n");
        return -EINVAL;
    }

    dh_dev->zf_ep = (struct pcie_zf_ep *)kzalloc(sizeof(struct pcie_zf_ep), GFP_KERNEL);
    if (!dh_dev->zf_ep)
    {
        DH_LOG_ERR_DEV(MODULE_MPF, dh_dev, "kzalloc zf_ep err\n");
        return -ENODEV;
    }
    zf_ep = dh_dev->zf_ep;

    dh_dev->zf_ep->dpu_ep_array = kzalloc(PCIE_DPU_EP_NUMS * sizeof(struct pcie_dpu_ep *), GFP_KERNEL);
    if (!dh_dev->zf_ep->dpu_ep_array)
    {
        DH_LOG_ERR_DEV(MODULE_MPF, dh_dev, "kzalloc dpu_ep_array err\n");
        ret = -ENODEV;
        goto free_zf_ep;
    }

    if (!is_pcie_zte_zf_device_support_vf(dh_dev->pdev))
    {
        DH_LOG_INFO_DEV(MODULE_MPF, dh_dev, "Firmware does not support SR-IOV (total_vfs=0) and use PF for vsock\n");
        vsock_pdev = dh_dev->pdev;
    }
    else
    {
        ret = pci_enable_sriov(dh_dev->pdev, 1); // 启用mpf的vf0
        if (ret)
        {
            DH_LOG_ERR_DEV(MODULE_MPF, dh_dev, "Failed to enable SR-IOV: %d\n", ret);
            goto free_dpu_ep_array;
        }

        vsock_pdev = pci_get_device(PCI_VENDOR_ID_ZTE, PCI_DID_DPUA_VSOCK_VF, NULL);
        if (!vsock_pdev)
        {
            vsock_pdev = pci_get_device(PCI_VENDOR_ID_ZTE, PCI_DID_DPUB_VSOCK_VF, NULL);
            if (!vsock_pdev)
            {
                DH_LOG_ERR_DEV(MODULE_MPF, dh_dev, "Failed to find vsock_vf_dev %d\n", ret);
                goto disable_sriov;
            }
        }
        DH_LOG_INFO_DEV(MODULE_MPF, dh_dev, "Firmware support SR-IOV and use VF for vsock\n");
    }

    dh_dev->zf_ep->mpf_pdev = dh_dev->pdev;
    dh_dev->zf_ep->dbi_paddr = pci_resource_start(dh_dev->pdev, BAR_2);
    dh_dev->zf_ep->mpf_paddr = pci_resource_start(dh_dev->pdev, BAR_0);
    dh_dev->zf_ep->vsock_paddr = pci_resource_start(vsock_pdev, BAR_0);
    dh_dev->zf_ep->ob_size = pci_resource_len(vsock_pdev, BAR_0) >> EP_ID_LEN;
    dh_dev->zf_ep->dpu_ep_num = PCIE_DPU_EP_NUMS;

    ret = zf_dev_map(dh_dev->pdev);
    if (ret)
    {
        DH_LOG_ERR_DEV(MODULE_MPF, dh_dev, "zf_dev_map err\n");
        goto disable_sriov;
    }

    ret = pci_zte_zf_epc_dev_init();
    if (ret)
    {
        goto unmap;
    }

    ret = pci_zte_epc_cdev_init();
    if (ret)
    {
        goto unmap;
    }

#if CFG_FILE_MODE_ENABLE
    if (pcie_zte_zf_cfg_file_init(dh_dev))
    {
        DH_LOG_ERR_DEV(MODULE_MPF, dh_dev, "pcie_zte_zf_cfg_file_init is err!\n");
    }
#endif

    DH_LOG_INFO_DEV(MODULE_MPF, dh_dev, "INFO:the EP0~3 pci_dev created successed!\n");
    return ret;
unmap:
    zf_dev_unmap(dh_dev->pdev);
disable_sriov:
    pci_disable_sriov(dh_dev->pdev);
free_dpu_ep_array:
    kfree(dh_dev->zf_ep->dpu_ep_array);
free_zf_ep:
    kfree(dh_dev->zf_ep);
    return ret;
}

void pcie_zte_zf_epc_free(struct dh_core_dev *dh_dev)
{
    // TODO:removed lose some step
#if CFG_FILE_MODE_ENABLE
    pcie_zte_zf_cfg_file_exit();
#endif

    if (IS_ERR_OR_NULL(dh_dev))
    {
        DH_LOG_ERR(MODULE_MPF, "dh_dev or id is NULL\n");
        return;
    }

    pci_disable_sriov(dh_dev->pdev);
    zf_dev_unmap(dh_dev->pdev);
    pci_zte_epc_dev_free(dh_dev->pdev);
    kfree(dh_dev->zf_ep->dpu_ep_array);
    kfree(dh_dev->zf_ep);
    pci_zte_epc_cdev_exit();
    DH_LOG_INFO_DEV(MODULE_MPF, dh_dev, "the EP0~3 pci_dev removed successed!\n");
}
