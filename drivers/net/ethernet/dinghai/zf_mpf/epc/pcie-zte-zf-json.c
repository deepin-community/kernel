#include "pcie-zte-zf-json.h"

static struct kobject *kobj_zxdh_cfg = NULL;
static char zxdh_pcie_cfg[PAGE_SIZE];
static struct dh_core_dev *dh_core_dev = NULL;
static int pcie_zte_zf_get_dev_cfg(struct dh_core_dev *core_dev);

extern int zxdh_bar_chan_sync_msg_send(struct zxdh_pci_bar_msg *in, struct zxdh_msg_recviver_mem *result);

static ssize_t read_pcie_cfg(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int ret = 0;

    ret = pcie_zte_zf_get_dev_cfg(dh_core_dev);
    if (ret)
    {
        DH_LOG_ERR(MODULE_MPF, "pcie_zte_zf_get_dev_cfg failed.\n");
    }

    return snprintf(buf, PAGE_SIZE, "%s", zxdh_pcie_cfg);
}

/*使用__ATTR宏初始化zxdh_cfg结构体，该宏定义在include/linux/sysfs.h*/
struct kobj_attribute zxdh_cfg = __ATTR(zxdh_pcie_cfg, 0664, read_pcie_cfg, NULL);

static int pcie_zte_zf_get_zxdh_info(struct dh_core_dev *core_dev, struct dpu_pf_cfg *pf_cfg)
{
    int ret = 0;
    char recv_buffer[RECV_BUFFER_SIZE] = {0};
    struct zxdh_pci_bar_msg in = {0};
    struct zxdh_msg_recviver_mem result = {0};
    struct dh_en_mpf_dev *mpf_dev = dh_core_priv(core_dev);

    if (RECV_BUFFER_SIZE < (sizeof(struct dpu_pf_cfg) + BAR_MSG_HEADER_SIZE))
    {
        DH_LOG_ERR_DEV(MODULE_MPF, core_dev, "recv_buffer size is smaller than sizeof struct dpu_pf_cfg \n");
        return -ENOMEM;
    }

    in.virt_addr = mpf_dev->pci_ioremap_addr + ZXDH_BAR1_CHAN_OFFSET;
    in.payload_addr = pf_cfg;
    in.payload_len = sizeof(struct dpu_pf_cfg);
    in.src = MSG_CHAN_END_PF;
    in.dst = MSG_CHAN_END_RISC;
    in.event_id = MODULE_DPU_PCIE_PF_INFO; /* 事件号 */
    in.src_pcieid = mpf_dev->pcie_id;

    result.recv_buffer = recv_buffer;
    result.buffer_len = sizeof(struct dpu_pf_cfg) + BAR_MSG_HEADER_SIZE;

    ret = zxdh_bar_chan_sync_msg_send(&in, &result);
    if (ret)
    {
        DH_LOG_ERR_DEV(MODULE_MPF, core_dev, "zxdh_bar_chan_sync_msg_send failed.\n");
        return ret;
    }

    memcpy(pf_cfg, (char *)(&recv_buffer) + BAR_MSG_HEADER_SIZE, sizeof(struct dpu_pf_cfg));

    return ret;
}

static void pcie_zte_zf_write_pf_info(struct dpu_pf_cfg *pf_cfg, char flag)
{
    char pf_info[100] = {0};
    size_t json_cfg_len = 0;
    int ret = 0;

    if (flag)
    {
        ret = snprintf(pf_info, sizeof(pf_info), "ep:%d pf:%d pf_enable:%d dev_type:%d vid:0x%x did:0x%x max_vf:%d\n",
                       pf_cfg->ep_id, pf_cfg->pf_id, pf_cfg->pf_enable, pf_cfg->dev_type, pf_cfg->vendor_id,
                       pf_cfg->device_id, pf_cfg->max_vf);
        if (ret < 0 || ret > sizeof(pf_info))
        {
            return;
        }
    }
    else
    {
        ret = snprintf(pf_info, sizeof(pf_info), "get ep:%d pf:%d info failed\n", pf_cfg->ep_id, pf_cfg->pf_id);
        if (ret < 0 || ret > sizeof(pf_info))
        {
            return;
        }
    }

    json_cfg_len = strlen(zxdh_pcie_cfg);

    if (json_cfg_len + strlen(pf_info) < PAGE_SIZE - 1)
    {
        strcat(zxdh_pcie_cfg, pf_info);
    }
    else
    {
        DH_LOG_ERR(MODULE_MPF, "error: zxdh_pcie_cfg len not enough\n");
    }
}

static int pcie_zte_zf_get_dev_cfg(struct dh_core_dev *core_dev)
{
    int ret = 0;
    u8 ep_idx = 0, pf_idx = 0;
    struct dpu_pf_cfg dpu_pf[PCIE_DPU_EP_NUMS][PCIE_DPU_PF_NUMS] = {0};

    memset(zxdh_pcie_cfg, 0, sizeof(zxdh_pcie_cfg));

    for (ep_idx = 0; ep_idx < PCIE_DPU_EP_NUMS; ep_idx++)
    {
        for (pf_idx = 0; pf_idx < PCIE_DPU_PF_NUMS; pf_idx++)
        {
            // get info
            dpu_pf[ep_idx][pf_idx].ep_id = ep_idx;
            dpu_pf[ep_idx][pf_idx].pf_id = pf_idx;
            dpu_pf[ep_idx][pf_idx].cmd_id = CMD_GET_PF_INFO_BY_FILE;
            ret = pcie_zte_zf_get_zxdh_info(core_dev, &dpu_pf[ep_idx][pf_idx]);
            if (ret)
            {
                DH_LOG_ERR_DEV(MODULE_MPF, core_dev, "ep%d pf%d get info err!!!\n", ep_idx, pf_idx);
                pcie_zte_zf_write_pf_info(&dpu_pf[ep_idx][pf_idx], ZF_DISABLE);
                return ret;
            }
            // write zxdh_pcie_cfg
            pcie_zte_zf_write_pf_info(&dpu_pf[ep_idx][pf_idx], ZF_ENABLE);
        }
    }

    return ret;
}

void delete_folder(char *path)
{
    int ret = 0;
    struct path lookup_path;
    struct dentry *dentry = NULL;
    struct inode *dir = NULL;

    ret = kern_path(path, LOOKUP_DIRECTORY, &lookup_path);
    if (ret)
    {
        return;
    }

    dentry = lookup_path.dentry;
    dir = dentry->d_inode;

    ret = vfs_rmdir(dir, dentry);
    if (ret)
    {
        DH_LOG_ERR(MODULE_MPF, "Failed to remove directory: %s\n", path);
    }

    dput(dentry);
}

int pcie_zte_zf_cfg_file_init(struct dh_core_dev *core_dev)
{
    int ret = 0;
    char path[] = ZXDH_SYSFS_PATH;

    delete_folder(path);

    kobj_zxdh_cfg = kobject_create_and_add(ZXDH_SYSFS_DIR, NULL);
    if (kobj_zxdh_cfg == NULL)
    {
        DH_LOG_ERR_DEV(MODULE_MPF, core_dev, "zxdh_cfg sysfs create failed\n");
        return -ENOMEM;
    }

    if (sysfs_create_file(kobj_zxdh_cfg, &zxdh_cfg.attr))
    {
        DH_LOG_ERR_DEV(MODULE_MPF, core_dev, "zxdh_cfg file create failed.\n");
        goto error_sysfs;
    }

    dh_core_dev = core_dev;
    if (pcie_zte_zf_get_dev_cfg(dh_core_dev))
    {
        DH_LOG_ERR_DEV(MODULE_MPF, core_dev, "pcie_zte_zf_get_dev_cfg failed.\n");
    }

    return ret;
error_sysfs:
    kobject_put(kobj_zxdh_cfg);
    kobj_zxdh_cfg = NULL;
    return -ENOENT;
}

void pcie_zte_zf_cfg_file_exit(void)
{
    sysfs_remove_file(kernel_kobj, &zxdh_cfg.attr);
    kobject_put(kobj_zxdh_cfg);
    dh_core_dev = NULL;
    DH_LOG_INFO(MODULE_MPF, "PCIe device to JSON driver cleanup\n");
}