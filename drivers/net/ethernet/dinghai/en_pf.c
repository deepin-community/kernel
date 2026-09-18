#include <linux/module.h>
#include <linux/kmod.h>
#include <linux/pci.h>
#include <linux/aer.h>
#include <linux/dinghai/driver.h>
#include <linux/dinghai/lag.h>
#include <net/devlink.h>
#include <linux/dinghai/devlink.h>
#include <linux/dinghai/helper.h>
#include <linux/dinghai/dh_cmd.h>
#include <linux/namei.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fsnotify.h>
#include <linux/file.h>
#include <linux/path.h>
#include <linux/dcache.h>
#include <linux/mount.h>
#include "en_pf.h"
#include "./en_pf/en_pf_irq.h"
#include "./en_pf/en_pf_eq.h"
#include "./en_pf/en_pf_events.h"
#include "en_aux.h"
#include "en_sf.h"
#include "en_np/init/include/dpp_np_init.h"
#include "en_pf/msg_func.h"
#include "msg_common.h"
#include "slib.h"
#include "zxdh_tools/zxdh_tools_mmap_chrdev.h"
#include "en_aux/en_aux_events.h"
#include "bonding/zxdh_lag.h"

/* Special firmware version and device combination check macros */
#define SPECIAL_FW_VERSION             "2.24.40.01"
#define TARGET_VENDOR_ID               0x1cf2
#define TARGET_DEVICE_ID_1             0x8040
#define TARGET_DEVICE_ID_2             0x8047

#define IS_FW_VERSION_MATCH(fw_ver) \
    (strstr(fw_ver, SPECIAL_FW_VERSION) != NULL)

#define IS_TARGET_DEVICE(dev, vendor) \
    ((vendor) == TARGET_VENDOR_ID && \
     ((dev) == TARGET_DEVICE_ID_1 || (dev) == TARGET_DEVICE_ID_2))

#define IS_SPECIAL_VERSION_FORCE_SPLIT(fw_ver, dev, vendor, board) \
    (IS_FW_VERSION_MATCH(fw_ver) && \
     IS_TARGET_DEVICE(dev, vendor) && \
     ((board) == DH_DPUA))

#ifdef CONFIG_ZXDH_SF
#include <linux/dinghai/en_sf.h>
#endif

#ifdef DRIVER_VERSION_VAL
    #define DRV_VERSION DRIVER_VERSION_VAL
#else
    #define DRV_VERSION "1.0-1"
#endif

#define DRV_SUMMARY "ZTE(R) zxdh-net driver"

const char zxdh_pf_driver_version[] = DRV_VERSION;
static const char zxdh_pf_driver_string[] = DRV_SUMMARY;
static const char zxdh_pf_copyright[] = "Copyright (c) 2022-23, ZTE.";

MODULE_AUTHOR("ZTE");
MODULE_DESCRIPTION(DRV_SUMMARY);
MODULE_VERSION(DRV_VERSION);
MODULE_LICENSE("Dual BSD/GPL");

uint32_t dh_debug_mask;
struct slot_id_array dh_slot[DPP_PCIE_SLOT_MAX] = {0};
module_param_named(debug_mask, dh_debug_mask, uint, 0644);
MODULE_PARM_DESC(debug_mask, "debug mask: 1 = dump cmd data, 2 = dump cmd exec time, 3 = both. Default=0");
static bool probe_vf = 1;
module_param(probe_vf, bool, 0644);
MODULE_PARM_DESC(probe_vf, "probe_vf: 0 = N, 1 = Y");

#define MAX_LENTH 128
#define DEFAULT_LOG_DIR "/var/log/dinghailogs"
static char log_dir[MAX_LENTH] = DEFAULT_LOG_DIR;
module_param_string(zxdh_log_dir, log_dir, sizeof(log_dir), S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(zxdh_log_dir, "The absolute path of the Dinghai log directory, default is /var/log/dinghailogs");

const struct pci_device_id dh_pf_pci_table[] = {
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_VF_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_BSI_VENDOR_ID, ZXDH_PF_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_BSI_VENDOR_ID, ZXDH_VF_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_INICA_BOND_DEVICE_ID), 0 },   /* bond */
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_INICB_BOND_DEVICE_ID), 0 },   /* bond */
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_INICC_BOND_DEVICE_ID), 0 },   /* bond */
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_DPUA_BOND_DEVICE_ID), 0 },    /* bond */
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_INICA_UPF_BOND_DEVICE_ID), 0 },    /* bond */
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_E310_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_VF_E310_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_E310_CMCC_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_VF_E310_CMCC_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_E312_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_VF_E312_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_DPUB_NOF_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_DPUB_PF_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_DPUB_INITIATOR1_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_DPUB_INITIATOR2_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_DPUB_RDMA_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_VF_DPUB_RDMA_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_UPF_PF_I512_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_UPF_VF_I512_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_INICA_RDMA_PF_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_INICA_RDMA_VF_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_E316_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_VF_E316_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_XPU_VENDER_ID, ZXDH_PF_E316_XPU_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_XPU_VENDER_ID, ZXDH_VF_E316_XPU_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_XPU_VENDER_ID, ZXDH_PF_E316L_XPU_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_XPU_VENDER_ID, ZXDH_VF_E316L_XPU_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_XPU_VENDER_ID, ZXDH_PF_E312_XPU_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_XPU_VENDER_ID, ZXDH_VF_E312_XPU_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_E311_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_VF_E311_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_I511_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_VF_I511_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_INICD_BOND0_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_INICD_BOND1_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_INICD_NE0_PF_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_INICD_NE0_VF_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_INICD_NE1_PF_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_INICD_NE1_VF_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_INICD_NE2_PF_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_INICD_NE2_VF_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_E310_RDMA_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_VF_E310_RDMA_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_E310S_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_VF_E310S_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_E312S_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_VF_E312S_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_DPUB_SRIOV0_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_DPUB_SRIOV1_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_I510_SRIOV_SEC_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_VF_I510_SRIOV_SEC_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_E312_RDMA_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_VF_E312_RDMA_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_INICA_OFFLOAD_DEVICE_ID), 0 },
    { PCI_DEVICE(CTC_PF_VENDOR_ID, CTC_PF_B512Y_DEVICE_ID), 0},
    { PCI_DEVICE(CTC_PF_VENDOR_ID, CTC_VF_B512Y_DEVICE_ID), 0},
    { PCI_DEVICE(CTC_PF_VENDOR_ID, CTC_PF_B522Y_DEVICE_ID), 0},
    { PCI_DEVICE(CTC_PF_VENDOR_ID, CTC_VF_B522Y_DEVICE_ID), 0},
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_E312S_D_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_VF_E312S_D_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_INICE_RDMA_PF_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_INICE_RDMA_VF_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_INICF_RDMA_PF_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_INICF_RDMA_VF_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_INICG_RDMA_PF_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_INICG_RDMA_VF_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_E318_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_VF_E318_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_XPU_VENDER_ID, ZXDH_PF_E318_XPU_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_XPU_VENDER_ID, ZXDH_VF_E318_XPU_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_DPUB_ROCE_RDMA0_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_VF_DPUB_ROCE_RDMA0_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_DPUB_ROCE_RDMA1_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_VF_DPUB_ROCE_RDMA1_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_DPUB_ROCE_SRIOV3_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_DPUB_ROCE_SRIOV4_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_E200_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_VF_E200_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_SPNC52_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_VF_SPNC52_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_SPR60H4A_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_INICA_SRIOV_PF_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_INICA_SRIOV_VF_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_DS25GE52_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_VF_DS25GE52_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_E316L_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_VF_E316L_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_PF_E310_03N00_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_VF_E310_03N00_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_LNKX_VENDOR_ID, ZXDH_PF_LNKX_2X200G_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_LNKX_VENDOR_ID, ZXDH_VF_LNKX_2X200G_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_LNKX_VENDOR_ID, ZXDH_PF_LNKX_1X400G_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_LNKX_VENDOR_ID, ZXDH_VF_LNKX_1X400G_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_YUANZHI_VENDOR_ID, ZXDH_PF_YUANZHI_2X25G_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_YUANZHI_VENDOR_ID, ZXDH_VF_YUANZHI_2X25G_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_YUANZHI_VENDOR_ID, ZXDH_PF_YUANZHI_2X100G_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_YUANZHI_VENDOR_ID, ZXDH_VF_YUANZHI_2X100G_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_YUANZHI_VENDOR_ID, ZXDH_PF_YUANZHI_2X100_OCP_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_YUANZHI_VENDOR_ID, ZXDH_VF_YUANZHI_2X100_OCP_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_YUANZHI_VENDOR_ID, ZXDH_PF_YUANZHI_1X200G_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_YUANZHI_VENDOR_ID, ZXDH_VF_YUANZHI_1X200G_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_YUANZHI_VENDOR_ID, ZXDH_PF_YUANZHI_2X200G_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_YUANZHI_VENDOR_ID, ZXDH_VF_YUANZHI_2X200G_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_YUANZHI_VENDOR_ID, ZXDH_PF_YUANZHI_1X400G_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_YUANZHI_VENDOR_ID, ZXDH_VF_YUANZHI_1X400G_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_MUCSE_VENDER_ID, ZXDH_PF_MUCSE_2X200G_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_MUCSE_VENDER_ID, ZXDH_VF_MUCSE_2X200G_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_MUCSE_VENDER_ID, ZXDH_PF_MUCSE_1X400G_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_MUCSE_VENDER_ID, ZXDH_VF_MUCSE_1X400G_DEVICE_ID), 0 },
    { 0, }
};

const struct pci_device_id dh_sw_dev_id_info[] = {
    { PCI_DEVICE(ZXDH_PF_VENDOR_ID, ZXDH_SWITCH_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_XPU_VENDER_ID, ZXDH_XPU_SWITCH_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_LNKX_VENDOR_ID, ZXDH_LNKX_SWITCH_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_MUCSE_VENDER_ID, ZXDH_MUCSE_SWITCH_DEVICE_ID), 0 },
    { PCI_DEVICE(ZXDH_PF_YUANZHI_VENDOR_ID, ZXDH_YUANZHI_SWITCH_DEVICE_ID), 0 }
};

MODULE_DEVICE_TABLE(pci, dh_pf_pci_table);

extern struct devlink_ops dh_pf_devlink_ops;
extern struct dh_core_devlink_ops dh_pf_core_devlink_ops;

#ifdef PTP_DRIVER_INTERFACE_EN
int zxdh_ptp_init(struct dh_core_dev *zxdev);
void zxdh_ptp_stop(struct dh_core_dev *zxdev);
#endif

static int validate_directory_path(const char *path)
{
    const char *p = path;
    int name_len = 0;

    if (*p != '/')
    {
        LOG_ERR("Invalid directory, path must start with '/': %s\n", path);
        return -EINVAL;
    }
    p++;

    while (*p != '\0')
    {
        if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
            (*p >= '0' && *p <= '9') || *p == '-' || *p == '_')
        {
            name_len++;
            p++;
            continue;
        }

        if (*p == '/')
        {
            if (name_len == 0)
            {
                LOG_ERR("Invalid directory, consecutive '/' found: %s\n", path);
                return -EINVAL;
            }

            // deal with next directory name
            name_len = 0;
            p++;

            if (*p == '\0')
            {
                LOG_ERR("Invalid directory, path ends with '/': %s\n", path);
                return -EINVAL;
            }
            continue;
        }

        LOG_ERR("Invalid directory, invalid character '%c' (0x%02x) at position %ld: %s\n",
                *p, *p, p - path, path);
        return -EINVAL;
    }

    if (p == path + 1)
    {
        LOG_ERR("Invalid directory, path cannot be just '/': %s\n", path);
        return -EINVAL;
    }

    return 0;
}

static bool is_directory_exists(const char *path)
{
    struct path p;
    int error;

    error = kern_path(path, LOOKUP_DIRECTORY | LOOKUP_FOLLOW, &p);
    if (!error)
    {
        path_put(&p);
        return true;
    }

    return false;
}

static void __init zxdh_log_dir_init(void)
{
    int ret = 0;
    bool need_reset = false;

    if (strcmp(log_dir, DEFAULT_LOG_DIR) != 0)
    {
        if (zte_strlen_s(log_dir) == 0)
        {
            LOG_ERR("Invalid directory, path is empty\n")
            need_reset = true;
        }
        else
        {
            ret = validate_directory_path(log_dir);
            if (ret)
                need_reset = true;
        }

        if (need_reset)
        {
            LOG_INFO("Using default directory %s instead of: %s\n", DEFAULT_LOG_DIR, log_dir);
            zte_strncpy_s(log_dir, DEFAULT_LOG_DIR, sizeof(log_dir) - 1);
            log_dir[sizeof(log_dir) - 1] = '\0';
        }
        else
        {
            LOG_INFO("Using user configured directory: %s\n", log_dir);
        }
    }
    else
    {
        LOG_INFO("Using default directory: %s\n", log_dir);
    }

    if (is_directory_exists(log_dir))
        return;

    ret = create_directory_recursion(log_dir);
    if (ret && ret != -EEXIST)
        LOG_ERR("Failed to create directory %s: %d\n", log_dir, ret);

    return;
}

int get_zxdh_log_dir(char *buffer, size_t buf_size)
{
    size_t log_dir_len;

    if (buffer == NULL || buf_size == 0)
        return -1;

    log_dir_len = zte_strlen_s(log_dir) + 1;

    if (buf_size < log_dir_len)
        return -1;

    zte_strncpy_s(buffer, log_dir, buf_size - 1);
    buffer[buf_size - 1] = '\0';

    return 0;
}

/**
 * @brief Check if the PCI device is Dinghai switch device
 *
 * @param pdev:             指向pf设备的指针pci_dev
 * @return                  true表示是Dinghai switch设备，false表示不是
 */
static bool is_dh_switch_dev(struct pci_dev *pdev)
{
    u32 i = 0;

    for (i = 0; i < sizeof(dh_sw_dev_id_info) / sizeof(struct pci_device_id); i++)
    {
        if ((pdev->vendor == dh_sw_dev_id_info[i].vendor) && (pdev->device == dh_sw_dev_id_info[i].device))
        return true;
    }

    return false;
}

/**
 * @brief Find the dinghai parent device
 *
 * @param pdev:             PF的pci_dev
 * @return                  成功时返回指向dinghai父设备（pci_dev）的指针；若未找到或出错，返回NULL
 */
static struct pci_dev *pcie_find_dh_parent_dev(struct pci_dev *pdev)
{
    while (pdev)
    {
        pdev = pci_upstream_bridge(pdev);
        if (!pdev)
        {
            return NULL;
        }
        if ((pci_is_pcie(pdev)) && (!is_dh_switch_dev(pdev)))
        {
            return pdev;
        }
    }

    return NULL;
}

/**
 * @brief @brief Set the rp cpl timeout mask status object
 *
 * @param pdev:             PF的pci_dev
 * @param status:           0为unmask，其余为mask
 * @return int32_t          0为正常，其余异常
 */
static int32_t set_rp_cpl_timeout_mask_status(struct pci_dev *pdev, uint32_t status)
{
    struct pci_dev *rp_dev = NULL;
    int aer = 0;
    u32 data = 0;

    rp_dev = pcie_find_dh_parent_dev(pdev);
    if (!rp_dev)
    {
        LOG_ERR("Can not find RP\n");
        return -ENODEV;
    }

    aer = pci_find_ext_capability(rp_dev, PCI_EXT_CAP_ID_ERR);
    if (!aer)
    {
        LOG_ERR("Can not find RP AER CAP\n");
        return -ENXIO;
    }

    pci_read_config_dword(rp_dev, aer + PCI_ERR_UNCOR_MASK, &data);

    if (status)
    {
        data |= PCI_ERR_UNC_COMP_TIME;
    } else
    {
        data &= ~PCI_ERR_UNC_COMP_TIME;
    }

    pci_write_config_dword(rp_dev, aer + PCI_ERR_UNCOR_MASK, data);

    return 0;
}

static int32_t zxdh_pf_set_cpl_timeout_mask(struct dh_core_dev *dev, uint32_t mask)
{
    return set_rp_cpl_timeout_mask_status(dev->pdev, mask);
}

/**
 * @brief Get the rp cpl timeout mask status object
 *
 * @param pdev:             PF的pci_dev
 * @return int32_t          1为mask，0为unmask，其余为获取状态失败
 */
static int32_t get_rp_cpl_timeout_mask_status(struct pci_dev *pdev)
{
    struct pci_dev *rp_dev = NULL;
    int aer = 0;
    u32 data = 0;

    rp_dev = pcie_find_dh_parent_dev(pdev);
    if (!rp_dev)
    {
        LOG_ERR("Can not find RP\n");
        return -ENODEV;
    }

    aer = pci_find_ext_capability(rp_dev, PCI_EXT_CAP_ID_ERR);
    if (!aer)
    {
        LOG_ERR("Can not find RP AER CAP\n");
        return -ENXIO;
    }

    pci_read_config_dword(rp_dev, aer + PCI_ERR_UNCOR_MASK, &data);

    return (data & PCI_ERR_UNC_COMP_TIME) ? 1 : 0;
}

static int32_t zxdh_pf_get_cpl_timeout_if_mask(struct dh_core_dev *dev)
{
    return get_rp_cpl_timeout_mask_status(dev->pdev);
}

/**
 * @brief Set the rp hp irq ctrl object
 *
 * @param pdev:             PF的pci_dev
 * @param status:           0为失能，其余为使能
 * @return int32_t          0为正常，其余异常
 */
static int32_t set_rp_hp_irq_ctrl(struct pci_dev *pdev, uint32_t status)
{
    struct pci_dev *rp_dev = NULL;
    int express = 0;
    u32 data = 0;

    rp_dev = pcie_find_dh_parent_dev(pdev);
    if (!rp_dev)
    {
        LOG_ERR("Can not find RP\n");
        return -ENODEV;
    }

    express = pci_find_capability(rp_dev, PCI_CAP_ID_EXP);
    if (!express)
    {
        LOG_ERR("Can not find RP EXPRESS CAP\n");
        return -ENXIO;
    }

    pci_read_config_dword(rp_dev, express + PCI_EXP_SLTCTL, &data);

    if (status)
    {
        data |= PCI_EXP_SLTCTL_HPIE;
    } else
    {
        data &= ~PCI_EXP_SLTCTL_HPIE;
    }

    pci_write_config_dword(rp_dev, express + PCI_EXP_SLTCTL, data);

    return 0;
}

static int32_t zxdh_pf_set_hp_irq_ctrl_status(struct dh_core_dev *dev, uint32_t status)
{
    return set_rp_hp_irq_ctrl(dev->pdev, status);
}

/**
 * @brief Get the rp hp irq ctrl status object
 *
 * @param pdev:             PF的pci_dev
 * @return int32_t          1为使能，0为失能，其余为获取状态失败
 */
static int32_t get_rp_hp_irq_ctrl_status(struct pci_dev *pdev)
{
    struct pci_dev *rp_dev = NULL;
    u32 data = 0;
    int express = 0;

    rp_dev = pcie_find_dh_parent_dev(pdev);
    if (!rp_dev)
    {
        LOG_ERR("Can not find RP\n");
        return -ENODEV;
    }

    express = pci_find_capability(rp_dev, PCI_CAP_ID_EXP);
    if (!express)
    {
        LOG_ERR("Can not find RP EXPRESS CAP\n");
        return -ENXIO;
    }

    pci_read_config_dword(rp_dev, express + PCI_EXP_SLTCTL, &data);

    return (data & PCI_EXP_SLTCTL_HPIE) ? 1 : 0;
}

int32_t zxdh_pf_get_hp_irq_ctrl_status(struct dh_core_dev *dev)
{
    return get_rp_hp_irq_ctrl_status(dev->pdev);
}

int32_t zxdh_pf_rp_config_init(struct dh_core_dev *dev)
{
    int32_t err = 0;

    err = zxdh_pf_pcie_config_store(dev);
    if (err)
        return err;

    zxdh_pf_set_cpl_timeout_mask(dev, 1);

    if (zxdh_pf_is_pcie_feature_support(dev, PCIE_HP)) {
        LOG_INFO_DEV(dev, "hp_irq no change\n");
        return 0;
    }

    zxdh_pf_set_hp_irq_ctrl_status(dev, 0);
    return 0;
}

static bool zxdh_pf_get_rp_link_status(struct dh_core_dev *dev)
{
    struct pci_dev *rp_dev = NULL;
    int pcie_cap = 0;
    u16 data = 0;

    rp_dev = pcie_find_dh_parent_dev(dev->pdev);
    if (!rp_dev)
    {
        LOG_ERR_DEV(dev, "Can not find RP\n");
        return -ENODEV;
    }

    pcie_cap = pci_find_capability(rp_dev, PCI_CAP_ID_EXP);
    if (!pcie_cap)
    {
        LOG_ERR_DEV(dev, "Can not find PCI Express CAP\n");
        return -ENXIO;
    }

    pci_read_config_word(rp_dev, pcie_cap + PCI_EXP_LNKSTA, &data);
    return (data & PCI_EXP_LNKSTA_DLLLA) ? true : false;
}

static bool zxdh_pf_get_upstream_port_link_status(struct dh_core_dev *dev)
{
    struct pci_dev *up_stream_dev = NULL;
    int pcie_cap = 0;
    u16 data = 0;

    up_stream_dev = pci_upstream_bridge(dev->pdev);
    if (!up_stream_dev)
    {
        LOG_ERR_DEV(dev, "Can not find RP\n");
        return -ENODEV;
    }
    pcie_cap = pci_find_capability(up_stream_dev, PCI_CAP_ID_EXP);
    if (!pcie_cap)
    {
        LOG_ERR_DEV(dev, "Can not find PCI Express CAP\n");
        return -ENXIO;
    }
    pci_read_config_word(up_stream_dev, pcie_cap + PCI_EXP_LNKSTA, &data);
    return (data & PCI_EXP_LNKSTA_DLLLA) ? true : false;
}

static bool zxdh_pf_check_remove_state(struct dh_core_dev *dev)
{
    if (!zxdh_pf_get_rp_link_status(dev))
        return false;

    return zxdh_pf_get_upstream_port_link_status(dev);
}

static int32_t dh_pf_pci_init(struct dh_core_dev *dev)
{
    int32_t ret = 0;
    struct zxdh_pf_device *pf_dev = NULL;

    pci_set_drvdata(dev->pdev, dev);

    ret = pci_enable_device(dev->pdev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dev, "pci_enable_device failed: %d\n", ret);
        return -ENOMEM;
    }

    ret = dma_set_mask_and_coherent(dev->device, DMA_BIT_MASK(64));
    if (ret != 0)
    {
        ret = dma_set_mask_and_coherent(dev->device, DMA_BIT_MASK(32));
        if (ret != 0)
        {
            LOG_ERR_DEV(dev, "dma_set_mask_and_coherent failed: %d\n", ret);
            goto err_pci;
        }
    }

    ret = pci_request_selected_regions(dev->pdev, pci_select_bars(dev->pdev, IORESOURCE_MEM), "dh-pf");
    if (ret != 0)
    {
        LOG_ERR_DEV(dev, "pci_request_selected_regions failed: %d\n", ret);
        goto err_pci;
    }

    pci_enable_pcie_error_reporting(dev->pdev);
    pci_set_master(dev->pdev);
    ret = pci_save_state(dev->pdev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dev, "pci_save_state failed: %d\n", ret);
        goto err_pci_save_state;
    }

    pf_dev = dh_core_priv(dev);
    pf_dev->pci_ioremap_addr[0] = (uint64_t)ioremap(pci_resource_start(dev->pdev, 0), pci_resource_len(dev->pdev, 0));
    if (pf_dev->pci_ioremap_addr[0] == 0)
    {
        ret = -1;
        LOG_ERR_DEV(dev, "ioremap(0x%llx, 0x%llx) failed\n", pci_resource_start(dev->pdev, 0), pci_resource_len(dev->pdev, 0));
        goto err_pci_save_state;
    }

    if (pf_dev->num_vfs == 0)
        clear_pf_sriov_status(dev);
    return 0;

err_pci_save_state:
    pci_disable_pcie_error_reporting(dev->pdev);
    pci_release_selected_regions(dev->pdev, pci_select_bars(dev->pdev, IORESOURCE_MEM));
err_pci:
    pci_disable_device(dev->pdev);
    return ret;
}

void dh_pf_pci_close(struct dh_core_dev *dev)
{
    struct zxdh_pf_device *pf_dev = NULL;

    pf_dev = dh_core_priv(dev);
    iounmap((void *)pf_dev->pci_ioremap_addr[0]);
    pci_disable_pcie_error_reporting(dev->pdev);
    pci_release_selected_regions(dev->pdev, pci_select_bars(dev->pdev, IORESOURCE_MEM));
    pci_disable_device(dev->pdev);

    return;
}

int32_t zxdh_pf_pci_find_capability(struct pci_dev *pdev, uint8_t cfg_type, uint32_t ioresource_types, int32_t *bars)
{
    int32_t pos = 0;
    uint8_t type = 0;
    uint8_t bar = 0;

    for (pos = pci_find_capability(pdev, PCI_CAP_ID_VNDR); pos > 0; pos = pci_find_next_capability(pdev, pos, PCI_CAP_ID_VNDR))
    {
        pci_read_config_byte(pdev, pos + offsetof(struct zxdh_pf_pci_cap, cfg_type), &type);
        pci_read_config_byte(pdev, pos + offsetof(struct zxdh_pf_pci_cap, bar), &bar);

        /* ignore structures with reserved BAR values */
        if (bar > ZXDH_PF_MAX_BAR_VAL)
        {
            continue;
        }

        if (type == cfg_type)
        {
            if (pci_resource_len(pdev, bar) && pci_resource_flags(pdev, bar) & ioresource_types)
            {
                *bars |= (1 << bar);
                return pos;
            }
        }
    }

    return 0;
}

void __iomem *zxdh_pf_map_capability(struct dh_core_dev *dh_dev, int32_t off,
                                     size_t minlen, uint32_t align, uint32_t start,
                                     uint32_t size, size_t *len, resource_size_t *pa,
                                     uint32_t *bar_off)
{
    struct pci_dev *pdev = dh_dev->pdev;
    uint8_t bar = 0;
    uint32_t offset = 0;
    uint32_t length = 0;
    void __iomem *p = NULL;

    pci_read_config_byte(pdev, off + offsetof(struct zxdh_pf_pci_cap, bar), &bar);
    pci_read_config_dword(pdev, off + offsetof(struct zxdh_pf_pci_cap, offset), &offset);
    pci_read_config_dword(pdev, off + offsetof(struct zxdh_pf_pci_cap, length), &length);

    if (bar_off != NULL)
    {
        *bar_off = offset;
    }

    if (length <= start)
    {
        LOG_ERR_DEV(dh_dev, "bad capability len %u (>%u expected)\n", length, start);
        return NULL;
    }

    if (length - start < minlen)
    {
        LOG_ERR_DEV(dh_dev, "bad capability len %u (>=%zu expected)\n", length, minlen);
        return NULL;
    }

    length -= start;
    if (start + offset < offset)
    {
        LOG_ERR_DEV(dh_dev, "map wrap-around %u+%u\n", start, offset);
        return NULL;
    }

    offset += start;
    if (offset & (align - 1))
    {
        LOG_ERR_DEV(dh_dev, "offset %u not aligned to %u\n", offset, align);
        return NULL;
    }

    if (length > size)
    {
        length = size;
    }

    if (len)
    {
        *len = length;
    }

    if (minlen + offset < minlen || minlen + offset > pci_resource_len(pdev, bar))
    {
        LOG_ERR_DEV(dh_dev, "map custom queue %zu@%u " "out of range on bar %i length %lu\n",
            minlen, offset, bar, (unsigned long)pci_resource_len(pdev, bar));
        return NULL;
    }

    p = pci_iomap_range(pdev, bar, offset, length);
    if (unlikely(p == NULL))
    {
        LOG_ERR_DEV(dh_dev, "unable to map custom queue %u@%u on bar %i\n", length, offset, bar);
    }
    else if (pa)
    {
        *pa = pci_resource_start(pdev, bar) + offset;
    }

    return p;
}

int32_t zxdh_pf_common_cfg_init(struct dh_core_dev *dh_dev)
{
    int32_t common = 0;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct pci_dev *pdev = dh_dev->pdev;

    /* check for a common config: if not, use legacy mode (bar 0). */
    common = zxdh_pf_pci_find_capability(pdev, ZXDH_PCI_CAP_COMMON_CFG, IORESOURCE_IO | IORESOURCE_MEM, &pf_dev->modern_bars);
    if (common == 0)
    {
        LOG_ERR_DEV(dh_dev, "missing capabilities %i, leaving for legacy driver\n", common);
        return -ENODEV;
    }

    pf_dev->common = zxdh_pf_map_capability(dh_dev, common,
                                            sizeof(struct zxdh_pf_pci_common_cfg), ZXDH_PF_ALIGN4, 0,
                                            sizeof(struct zxdh_pf_pci_common_cfg), NULL, NULL, NULL);
    if (unlikely(pf_dev->common == NULL))
    {
        LOG_ERR_DEV(dh_dev, "pf_dev->common is null\n");
        return -EINVAL;
    }

    return 0;
}

int32_t zxdh_pf_notify_cfg_init(struct dh_core_dev *dh_dev)
{
    int32_t notify = 0;
    uint32_t notify_length = 0;
    uint32_t notify_offset = 0;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct pci_dev *pdev = dh_dev->pdev;

    /* If common is there, these should be too... */
    notify = zxdh_pf_pci_find_capability(pdev, ZXDH_PCI_CAP_NOTIFY_CFG, IORESOURCE_IO | IORESOURCE_MEM, &pf_dev->modern_bars);
    if (notify == 0)
    {
        LOG_ERR_DEV(dh_dev, "missing capabilities %i\n", notify);
        return -EINVAL;
    }

    pci_read_config_dword(pdev, notify + offsetof(struct zxdh_pf_pci_notify_cap, notify_off_multiplier), &pf_dev->notify_offset_multiplier);
    pci_read_config_dword(pdev, notify + offsetof(struct zxdh_pf_pci_notify_cap, cap.length), &notify_length);
    pci_read_config_dword(pdev, notify + offsetof(struct zxdh_pf_pci_notify_cap, cap.offset), &notify_offset);

    /* We don't know how many VQs we'll map, ahead of the time.
     * If notify length is small, map it all now. Otherwise, map each VQ individually later. */
    if ((uint64_t)notify_length + (notify_offset % PAGE_SIZE) <= PAGE_SIZE)
    {
        pf_dev->notify_base = zxdh_pf_map_capability(dh_dev, notify, ZXDH_PF_MAP_MINLEN2, ZXDH_PF_ALIGN2,
            0, notify_length, &pf_dev->notify_len, &pf_dev->notify_pa, NULL);
        if (unlikely(pf_dev->notify_base == NULL))
        {
            LOG_ERR_DEV(dh_dev, "pf_dev->notify_base is null\n");
            return -EINVAL;
        }
    }
    else
    {
        pf_dev->notify_map_cap = notify;
    }

    return 0;
}

int32_t zxdh_pf_device_cfg_init(struct dh_core_dev *dh_dev)
{
    int32_t device = 0;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct pci_dev *pdev = dh_dev->pdev;

    /* Device capability is only mandatory for devices that have device-specific configuration. */
    device = zxdh_pf_pci_find_capability(pdev, ZXDH_PCI_CAP_DEVICE_CFG, IORESOURCE_IO | IORESOURCE_MEM, &pf_dev->modern_bars);

    /* we don't know how much we should map, but PAGE_SIZE is more than enough for all existing devices. */
    if (device)
    {
        pf_dev->device = zxdh_pf_map_capability(dh_dev, device, 0, ZXDH_PF_ALIGN4,
                            0, PAGE_SIZE, &pf_dev->device_len, NULL, &pf_dev->dev_cfg_bar_off);
        if (unlikely(pf_dev->device == NULL))
        {
            LOG_ERR_DEV(dh_dev, "pf_dev->device is null\n");
            return -EINVAL;
        }
    }
    return 0;
}

void zxdh_pf_modern_cfg_uninit(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct pci_dev *pdev = dh_dev->pdev;

    if (pf_dev->device)
    {
        pci_iounmap(pdev, pf_dev->device);
    }
    if (pf_dev->notify_base)
    {
        pci_iounmap(pdev, pf_dev->notify_base);
    }
    pci_iounmap(pdev, pf_dev->common);
}

int32_t zxdh_pf_modern_cfg_init(struct dh_core_dev *dh_dev)
{
    int32_t ret = 0;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct pci_dev *pdev = dh_dev->pdev;

    ret = zxdh_pf_common_cfg_init(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_pf_common_cfg_init failed: %d\n", ret);
        return -EINVAL;
    }

    ret = zxdh_pf_notify_cfg_init(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_pf_notify_cfg_init failed: %d\n", ret);
        goto err_map_notify;
    }

    ret = zxdh_pf_device_cfg_init(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_pf_device_cfg_init failed: %d\n", ret);
        goto err_map_device;
    }

    return 0;

err_map_device:
    if (pf_dev->notify_base)
    {
        pci_iounmap(pdev, pf_dev->notify_base);
    }
err_map_notify:
    pci_iounmap(pdev, pf_dev->common);
    return -EINVAL;
}

uint16_t zxdh_pf_get_queue_notify_off(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    if (pf_dev->packed_status)
    {
        iowrite16(phy_index, &pf_dev->common->queue_select);
    }
    else
    {
        iowrite16(index, &pf_dev->common->queue_select);
    }

    return ioread16(&pf_dev->common->queue_notify_off);
}

void __iomem * zxdh_pf_map_vq_notify(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index, resource_size_t *pa)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint16_t off = 0;

    off = zxdh_pf_get_queue_notify_off(dh_dev, phy_index, index);

    if (pf_dev->notify_base)
    {
        /* offset should not wrap */
        if ((uint64_t)off * pf_dev->notify_offset_multiplier + 2 > pf_dev->notify_len)
        {
            LOG_ERR_DEV(dh_dev, "bad notification offset %u (x %u) " "for queue %u > %zd",
                    off, pf_dev->notify_offset_multiplier, phy_index, pf_dev->notify_len);
            return NULL;
        }

        if (pa)
        {
            *pa = pf_dev->notify_pa + off * pf_dev->notify_offset_multiplier;
        }

        return pf_dev->notify_base + off * pf_dev->notify_offset_multiplier;
    }
    else
    {
        return zxdh_pf_map_capability(dh_dev, pf_dev->notify_map_cap, 2, 2,
                                      off * pf_dev->notify_offset_multiplier, 2, NULL, pa, NULL);
    }
}

void zxdh_pf_unmap_vq_notify(struct dh_core_dev *dh_dev, void *priv)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    if (!pf_dev->notify_base)
    {
        pci_iounmap(dh_dev->pdev, priv);
    }
}

void zxdh_pf_set_status(struct dh_core_dev *dh_dev, uint8_t status)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    iowrite8(status, &pf_dev->common->device_status);

    return;
}

uint8_t zxdh_pf_get_status(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    return ioread8(&pf_dev->common->device_status);
}

static uint8_t zxdh_pf_get_cfg_gen(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint8_t config_generation = 0;

    config_generation = ioread8(&pf_dev->common->config_generation);
    LOG_INFO_DEV(dh_dev, "config_generation is %d\n",config_generation);

    return config_generation;
}

void zxdh_pf_get_vf_mac(struct dh_core_dev *dh_dev, uint8_t *mac, int32_t vf_id)
{
    uint32_t DEV_MAC_L = 0;
    uint16_t DEV_MAC_H = 0;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    if (pf_dev->pf_sriov_cap_base)
    {
        DEV_MAC_L = ioread32((void __iomem *)(pf_dev->pf_sriov_cap_base + (pf_dev->sriov_bar_size) * vf_id + pf_dev->dev_cfg_bar_off));
        mac[0] = DEV_MAC_L & 0xff;
        mac[1] = (DEV_MAC_L >> 8) & 0xff;
        mac[2] = (DEV_MAC_L >> 16) & 0xff;
        mac[3] = (DEV_MAC_L >> 24) & 0xff;
        DEV_MAC_H = ioread16((void __iomem *)(pf_dev->pf_sriov_cap_base + (pf_dev->sriov_bar_size) * vf_id \
                            + pf_dev->dev_cfg_bar_off + ZXDH_DEV_MAC_HIGH_OFFSET));
        mac[4] = DEV_MAC_H & 0xff;
        mac[5] = (DEV_MAC_H >> 8) & 0xff;
    }
    return;
}

void zxdh_pf_set_vf_mac_reg(struct zxdh_pf_device *pf_dev, uint8_t *mac, int32_t vf_id)
{
    uint32_t DEV_MAC_L = 0;
    uint16_t DEV_MAC_H = 0;

    if (pf_dev->pf_sriov_cap_base)
    {
        DEV_MAC_L = mac[0] | (mac[1] << 8) | (mac[2] << 16) | (mac[3] << 24);
        DEV_MAC_H = mac[4] | (mac[5] << 8);
        iowrite32(DEV_MAC_L, (void __iomem *)(pf_dev->pf_sriov_cap_base + (pf_dev->sriov_bar_size) * vf_id \
                                + pf_dev->dev_cfg_bar_off));
        iowrite16(DEV_MAC_H, (void __iomem *)(pf_dev->pf_sriov_cap_base + (pf_dev->sriov_bar_size) * vf_id \
                                + pf_dev->dev_cfg_bar_off + ZXDH_DEV_MAC_HIGH_OFFSET));
    }
    return;
}

void zxdh_pf_set_vf_mac(struct dh_core_dev *dh_dev, uint8_t *mac, int32_t vf_id)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    zxdh_pf_set_vf_mac_reg(pf_dev, mac, vf_id);
    return;
}

void zxdh_set_mac(struct dh_core_dev *dh_dev, uint8_t *mac)
{
    uint32_t DEV_MAC_L = 0;
    uint16_t DEV_MAC_H = 0;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    DEV_MAC_L = mac[0] | (mac[1] << 8) | (mac[2] << 16) | (mac[3] << 24);
    DEV_MAC_H = mac[4] | (mac[5] << 8);
    iowrite32(DEV_MAC_L, pf_dev->device);
    iowrite16(DEV_MAC_H, (void __iomem *)((uint8_t *)pf_dev->device + ZXDH_DEV_MAC_HIGH_OFFSET));
    return;
}

void zxdh_get_mac(struct dh_core_dev *dh_dev, uint8_t *mac)
{
    uint32_t DEV_MAC_L = 0;
    uint16_t DEV_MAC_H = 0;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    DEV_MAC_L = ioread32(pf_dev->device);
    mac[0] = DEV_MAC_L & 0xff;
    mac[1] = (DEV_MAC_L >> 8) & 0xff;
    mac[2] = (DEV_MAC_L >> 16) & 0xff;
    mac[3] = (DEV_MAC_L >> 24) & 0xff;
    DEV_MAC_H = ioread16((void __iomem *)((uint8_t *)pf_dev->device + ZXDH_DEV_MAC_HIGH_OFFSET));
    mac[4] = DEV_MAC_H & 0xff;
    mac[5] = (DEV_MAC_H >> 8) & 0xff;
    return;
}

uint64_t zxdh_pf_get_features(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint64_t device_feature = 0;

    iowrite32(0, &pf_dev->common->device_feature_select);
    device_feature = ioread32(&pf_dev->common->device_feature);
    iowrite32(1, &pf_dev->common->device_feature_select);
    device_feature |= ((uint64_t)ioread32(&pf_dev->common->device_feature) << 32);

    return device_feature;
}

void zxdh_pf_set_features(struct dh_core_dev *dh_dev, uint64_t features)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    iowrite32(0, &pf_dev->common->guest_feature_select);
    iowrite32((uint32_t)features, &pf_dev->common->guest_feature);
    iowrite32(1, &pf_dev->common->guest_feature_select);
    iowrite32(features >> 32, &pf_dev->common->guest_feature);

    return;
}

void zxdh_pf_set_queue_enable(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index, bool enable)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    if (pf_dev->packed_status)
    {
        iowrite16(phy_index, &pf_dev->common->queue_select);
    }
    else
    {
        iowrite16(index, &pf_dev->common->queue_select);
    }
    iowrite16(enable, &pf_dev->common->queue_enable);
}

uint16_t zxdh_pf_get_epbdf(struct dh_core_dev *dh_dev)
{
    struct pci_dev *pdev = dh_dev->pdev;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint32_t domain = 0;
    uint32_t bus = 0;
    uint32_t devid = 0;
    uint32_t function = 0;

    if (pdev == NULL)
    {
        LOG_ERR("err: pdev null, return epbdf data 0.\n");
        return 0;
    }

    if(sscanf(pci_name(pdev), "%x:%x:%x.%u", &domain, &bus, &devid, &function) != 4)
    {
        LOG_ERR_DEV(dh_dev, "failed to get pcie bus-info\n");
        return 0;
    }
    pf_dev->epbdf = BDF_ECAM(bus, devid, function);
    return pf_dev->epbdf;
}

static uint64_t zxdh_pf_get_spec_sbdf(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    return pf_dev->spec_sbdf;
}

static bool zxdh_pf_is_multi_ep(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    return pf_dev->is_multi_ep;
}

static uint32_t zxdh_pf_get_rp_sbdf(struct dh_core_dev *dh_dev)
{
    struct pci_dev *pdev = dh_dev->pdev;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct pci_dev *rp_pdev = NULL;
    struct pci_dev *uppder_pdev = NULL;
    uint32_t domain = 0;
    uint32_t bus = 0;
    uint32_t devid = 0;
    uint32_t function = 0;

    if (pdev == NULL)
    {
        LOG_ERR("err: pdev null, return epbdf data 0\n");
        return 0;
    }

    /* 找到当前设备的父节点 */
    uppder_pdev = pci_upstream_bridge(pdev);
    if (uppder_pdev == NULL)
    {
        LOG_ERR_DEV(dh_dev, "err: uppder_pdev null, return rp_sbdf data 0\n");
        return 0;
    }

    /* 判断当前是单EP还是多EP */
    if (((uppder_pdev->vendor == ZXDH_PF_VENDOR_ID) && (uppder_pdev->device == ZXDH_SWITCH_DEVICE_ID)) \
     || ((uppder_pdev->vendor == ZXDH_PF_XPU_VENDER_ID) && (uppder_pdev->device == ZXDH_XPU_SWITCH_DEVICE_ID)) \
     || ((uppder_pdev->vendor == ZXDH_PF_LNKX_VENDOR_ID) && (uppder_pdev->device == ZXDH_LNKX_SWITCH_DEVICE_ID)) \
     || ((uppder_pdev->vendor == ZXDH_PF_MUCSE_VENDER_ID) && (uppder_pdev->device == ZXDH_MUCSE_SWITCH_DEVICE_ID)) \
     || ((uppder_pdev->vendor == ZXDH_PF_YUANZHI_VENDOR_ID) && (uppder_pdev->device == ZXDH_YUANZHI_SWITCH_DEVICE_ID)))
    {
        /* switch的上节点 */
        uppder_pdev = pci_upstream_bridge(uppder_pdev);
        if (uppder_pdev == NULL)
        {
            LOG_ERR_DEV(dh_dev, "err: uppder_pdev null, return rp_sbdf data 0\n");
            return 0;
        }
        /* rp节点 */
        uppder_pdev = pci_upstream_bridge(uppder_pdev);
        if (uppder_pdev == NULL)
        {
            LOG_ERR_DEV(dh_dev, "err: uppder_pdev null, return rp_sbdf data 0\n");
            return 0;
        }
        rp_pdev = uppder_pdev;
        pf_dev->is_multi_ep = true;
    }
    else
    {
        rp_pdev = uppder_pdev;
        pf_dev->is_multi_ep = false;
    }

    /* 获取rp节点的sbdf号 */
    if (sscanf(pci_name(rp_pdev), "%x:%x:%x.%u", &domain, &bus, &devid, &function) != 4)
    {
        LOG_ERR_DEV(dh_dev, "failed to get pcie rp bus-info\n");
        return 0;
    }

    pf_dev->rp_sbdf = SBDF_ECAM(domain, bus, devid, function);
    LOG_DEBUG_DEV(dh_dev, "rp: domain %#x, bus %#x, devid %#x, function %#x, rp_sbdf %#x. is_multi_ep: %d\n",
             domain, bus, devid, function, pf_dev->rp_sbdf, pf_dev->is_multi_ep);

    return pf_dev->rp_sbdf;
}


int zxdh_pf_get_pannel_port_num(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    return pf_dev->pannel_port_num;
}

static uint16_t zxdh_pf_get_vport(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    return pf_dev->vport;
}

static enum dh_coredev_type zxdh_pf_get_coredev_type(struct dh_core_dev *dh_dev)
{
    return dh_dev->coredev_type;
}

static uint16_t zxdh_pf_get_pcie_id(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    return pf_dev->pcie_id;
}

static uint16_t zxdh_pf_get_slot_id(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint16_t slot_id = 0;
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    if (dh_dev->coredev_type == DH_COREDEV_PF)
        return pf_dev->slot_id;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(dh_dev, "failed to kzalloc\n");
        return 0;
    }

    msg->payload.hdr.op_code = ZXDH_VF_SLOT_ID_GET;

    err = zxdh_pf_msg_send_cmd(dh_dev, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "send_msg_to_pf failed, err: %d\n", err);
        kfree(msg);
        return 0;
    }
    slot_id = msg->reps.slot_info.slot_id;
    kfree(msg);

    return slot_id;
}

static bool zxdh_pf_is_special_bond(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct firmware_capability *fwcap = &pf_dev->fwcap;

    if (FW_FEATURE_GET(fwcap->fw_feature, FW_FEATURE_STD) == 1)
    {
        return true;
    }

    return false;
}

static bool zxdh_pf_is_support_bond_config(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct firmware_capability *fwcap = &pf_dev->fwcap;

    if (FW_FEATURE_GET(fwcap->fw_feature, FW_FEATURE_DHTS_BOND_CFG) == 1)
    {
        return true;
    }
    return false;
}

static bool zxdh_pf_suport_np_ext_stats(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct firmware_capability *fwcap = &pf_dev->fwcap;

    if (FW_FEATURE_GET(fwcap->fw_feature, FW_FEATURE_NPSTAT) == 1)
    {
        return true;
    }

    return false;
}

static bool zxdh_pf_is_fw_feature_support(struct dh_core_dev *dh_dev, uint32_t feature)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct firmware_capability *fwcap = &pf_dev->fwcap;

    if (FW_FEATURE_GET(fwcap->fw_feature, feature) == 1)
    {
        return true;
    }

    return false;
}

bool zxdh_pf_is_pcie_feature_support(struct dh_core_dev *dh_dev, uint32_t feature)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct firmware_capability *fwcap = &pf_dev->fwcap;

    if (FW_FEATURE_GET(fwcap->pcie_feature, feature) == 1)
    {
        return true;
    }

    return false;
}

static struct zxdh_np_ext_stats * zxdh_get_np_ext_stats(struct dh_core_dev *dh_dev, uint8_t panel_id)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint32_t err_offset = 0;
    uint32_t disc_offset = 0;
    struct zxdh_np_ext_stats *ext_stats = &pf_dev->np_ext_stats;
    void __iomem* err_addr = NULL;
    void __iomem* disc_addr = NULL;
    uint32_t err_cnt = 0;
    uint32_t disc_cnt = 0;

    err_offset = 2*pf_dev->phy_port * sizeof(uint32_t);
    disc_offset = (2*pf_dev->phy_port + 1) * sizeof(uint32_t);

    err_addr = (void __iomem*)(pf_dev->pci_ioremap_addr[0] + ZXDH_NP_EXT_STATS_OFFSET + err_offset);
    disc_addr = (void __iomem*)(pf_dev->pci_ioremap_addr[0] + ZXDH_NP_EXT_STATS_OFFSET + disc_offset);
    if ((err_addr >= (void __iomem*)(pf_dev->pci_ioremap_addr[0] + ZXDH_NP_EXT_STATS_OFFSET + ZXDH_NP_EXT_STATS_SIZE)) \
     || (disc_addr >= (void __iomem*)(pf_dev->pci_ioremap_addr[0] + ZXDH_NP_EXT_STATS_OFFSET + ZXDH_NP_EXT_STATS_SIZE)))
    {
        LOG_ERR_DEV(dh_dev, "addr out-off rang, err_addr: %llx\n", (uint64_t)err_addr);
        LOG_ERR_DEV(dh_dev, "addr out-off rang, disc_addr: %llx\n", (uint64_t)disc_addr);
        return NULL;
    }

    err_cnt = ioread32((void __iomem*)err_addr);
    disc_cnt = ioread32((void __iomem*)disc_addr);

    ext_stats->rx_vport2np_packets = err_cnt + disc_cnt;

    return ext_stats;
}

bool zxdh_pf_is_bond(struct dh_core_dev *dh_dev)
{
    bool flags = false;

    if (!dh_core_is_pf(dh_dev))
    {
        return false;
    }

    if (zxdh_pf_is_special_bond(dh_dev))
    {
        return false;
    }

    if ((dh_dev->pdev->device == ZXDH_INICA_BOND_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_INICB_BOND_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_INICC_BOND_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_INICA_UPF_BOND_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_DPUA_BOND_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_INICD_BOND0_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_INICD_BOND1_DEVICE_ID))
    {
        flags = true;
    }

    return  flags;
}

void zxdh_pf_set_bond_link_info(struct dh_core_dev *dh_dev, uint16_t bond_link_info)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    LOG_INFO_DEV(dh_dev, "%s bond_link_info: %d\n", pci_name(dh_dev->pdev), bond_link_info);
    pf_dev->bond_link_info = bond_link_info;
    return;
}

void zxdh_pf_set_bond_slave_flag(struct dh_core_dev *dh_dev, bool is_bond_slave)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    LOG_INFO_DEV(dh_dev, "%s is_bond_slave: %d\n", pci_name(dh_dev->pdev), is_bond_slave);
    pf_dev->is_bond_slave = is_bond_slave;
    return;
}

uint8_t zxdh_pf_get_bond_port_type(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    return pf_dev->bond_port_type;
}

uint8_t zxdh_pf_get_no_bondpf_panel_num(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    return pf_dev->no_bondpf_panel;
}

bool zxdh_pf_is_upf(struct dh_core_dev *dh_dev)
{
    bool flags = false;

    if ((dh_dev->pdev->device == ZXDH_UPF_PF_I512_DEVICE_ID)
        || (dh_dev->pdev->device == ZXDH_UPF_VF_I512_DEVICE_ID))
    {
        flags = true;
    }

    return  flags;
}

static bool zxdh_pf_is_nic(struct dh_core_dev *dh_dev)
{
    if ((dh_dev->pdev->device == ZXDH_PF_E312_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_E312_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_E316_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_E316_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_E316_XPU_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_E316_XPU_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_E316L_XPU_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_E316L_XPU_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_E312_XPU_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_E312_XPU_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_E310_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_E310_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_E310_CMCC_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_E310_CMCC_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_E311_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_E311_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_E310_RDMA_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_E310_RDMA_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_E310S_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_E310S_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_E312S_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_E312S_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_E312_RDMA_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_E312_RDMA_DEVICE_ID) ||
        (dh_dev->pdev->device == CTC_PF_B512Y_DEVICE_ID) ||
        (dh_dev->pdev->device == CTC_VF_B512Y_DEVICE_ID) ||
        (dh_dev->pdev->device == CTC_PF_B522Y_DEVICE_ID) ||
        (dh_dev->pdev->device == CTC_VF_B522Y_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_E312S_D_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_E312S_D_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_E318_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_E318_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_E318_XPU_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_E318_XPU_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_E200_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_E200_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_SPNC52_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_SPNC52_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_SPR60H4A_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_INICA_SRIOV_PF_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_INICA_SRIOV_VF_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_E316L_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_E316L_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_LNKX_2X200G_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_LNKX_2X200G_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_LNKX_1X400G_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_LNKX_1X400G_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_YUANZHI_2X25G_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_YUANZHI_2X25G_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_YUANZHI_2X100G_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_YUANZHI_2X100G_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_YUANZHI_2X100_OCP_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_YUANZHI_2X100_OCP_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_YUANZHI_1X200G_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_YUANZHI_1X200G_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_YUANZHI_2X200G_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_YUANZHI_2X200G_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_YUANZHI_1X400G_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_YUANZHI_1X400G_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_MUCSE_2X200G_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_MUCSE_2X200G_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_MUCSE_1X400G_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_MUCSE_1X400G_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_E310_03N00_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_E310_03N00_DEVICE_ID))
    {
        return true;
    }

    return false;
}

static bool zxdh_pf_is_rdma_enable(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct firmware_capability *fwcap = &pf_dev->fwcap;

    if (FW_FEATURE_GET(fwcap->fw_feature, FW_FEATURE_RDMA) == 1)
    {
        return true;
    }

    return false;
}

static bool zxdh_pf_is_drs_sec_enable(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct firmware_capability *fwcap = &pf_dev->fwcap;

    if (FW_FEATURE_GET(fwcap->fw_feature, FW_FEATURE_SEC) == 1)
    {
        return true;
    }

    return false;
}

uint32_t zxdh_pf_get_dev_type(struct dh_core_dev *dh_dev)
{
    if ((dh_dev->pdev->device == ZXDH_UPF_PF_I512_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_UPF_VF_I512_DEVICE_ID))
    {
        return ZXDH_DEV_UPF;
    }

    if ((dh_dev->pdev->device == ZXDH_INICD_NE0_PF_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_INICD_NE0_VF_DEVICE_ID))
    {
        return ZXDH_DEV_NE0;
    }

    if ((dh_dev->pdev->device == ZXDH_INICD_NE1_PF_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_INICD_NE1_VF_DEVICE_ID))
    {
        return ZXDH_DEV_NE1;
    }

    if ((dh_dev->pdev->device == ZXDH_PF_DPUB_ROCE_RDMA0_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_DPUB_ROCE_RDMA0_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_DPUB_ROCE_RDMA1_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_DPUB_ROCE_RDMA1_DEVICE_ID))
    {
        return ZXDH_DEV_ROCE_RDMA;
    }

    if (dh_dev->pdev->device == ZXDH_PF_DPUB_ROCE_SRIOV3_DEVICE_ID)
    {
        return ZXDH_DEV_ROCE_SRIOV;
    }

    if (dh_dev->pdev->device == ZXDH_PF_DPUB_ROCE_SRIOV4_DEVICE_ID)
    {
        return ZXDH_DEV_ROCE_SPECIAL_BOND;
    }

    return ZXDH_DEV_UNKNOW;
}

static bool zxdh_pf_is_panel_port(struct dh_core_dev *dh_dev)
{
    if ((zxdh_pf_get_dev_type(dh_dev) == ZXDH_DEV_UPF) ||
        (zxdh_pf_get_dev_type(dh_dev) == ZXDH_DEV_NE0) ||
        (zxdh_pf_get_dev_type(dh_dev) == ZXDH_DEV_NE1) ||
        (zxdh_pf_get_dev_type(dh_dev) == ZXDH_DEV_ROCE_RDMA) ||
        (zxdh_pf_get_dev_type(dh_dev) == ZXDH_DEV_ROCE_SRIOV))
    {
        return false;
    }

    return true;
}

static uint8_t zxdh_pf_get_panel_id(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    return pf_dev->panel_id;
}

static uint8_t zxdh_pf_get_queue_pairs(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    if (zxdh_pf_is_bond(dh_dev))
    {
        return ZXDH_BOND_ETH_MQ_PAIRS_NUM;
    }

    if (zxdh_pf_is_nic(dh_dev) || zxdh_pf_is_special_bond(dh_dev))
    {
        return pf_dev->vq_pairs;
    }

    return max_pairs;
}

struct zxdh_vf_item *zxdh_pf_get_vf_item(struct dh_core_dev *dh_dev, uint16_t vf_idx)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    if (dh_dev->coredev_type != DH_COREDEV_PF)
    {
        LOG_ERR_DEV(dh_dev, "Invalid device\n");
        return ERR_PTR(-EINVAL);
    }

    if (vf_idx >= ZXDH_VF_NUM_MAX)
    {
        LOG_ERR_DEV(dh_dev, "vf idx(%u) out of range(0~255)\n", vf_idx);
        return ERR_PTR(-EINVAL);
    }

    if (pf_dev->vf_item == NULL)
    {
        LOG_ERR_DEV(dh_dev, "vf_item is NULL\n");
        return ERR_PTR(-EINVAL);
    }

    if (!pf_dev->vf_item[vf_idx].enable)
    {
        LOG_ERR_DEV(dh_dev, "vf(%u) is disable\n", vf_idx);
        return ERR_PTR(-EINVAL);
    }

    return &(pf_dev->vf_item[vf_idx]);
}

static int32_t zxdh_vf_compat_check(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    uint64_t msg_idmax = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    if (dh_dev->coredev_type == DH_COREDEV_PF)
        return 0;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(dh_dev, "failed to kzalloc\n");
        return -1;
    }

    msg->payload.hdr.op_code = ZXDH_GET_K_CMPAT_VERINFO;
    msg->payload.kernel_cmpat_msg.vfid = VQM_VFID(pf_dev->vport);
    err = zxdh_pf_msg_send_cmd(dh_dev, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "send_msg_to_pf failed, err: %d\n", err);
        kfree(msg);
        return err;
    }

    msg_idmax = msg->reps.kernel_cmpat_rsp.k_msg_idmax;
    if (msg_idmax < ZXDH_MSG_TYPE_CNT_MAX) //vf驱动的消息数量比pf驱动多，做兼容性提示
    {
        LOG_INFO_DEV(dh_dev, "msg_idmax error!, msg_idmax=%lld, ZXDH_MSG_TYPE_CNT_MAX=%d\n", msg_idmax, ZXDH_MSG_TYPE_CNT_MAX);
        LOG_INFO_DEV(dh_dev, "Perhaps the version of the pf device driver is too old.\n");
    }

    kfree(msg);

    return 0;
}

static int32_t zxdh_get_mcfeature_from_pf(struct dh_core_dev *dh_dev, uint64_t *mcfeature)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    if (dh_dev->coredev_type == DH_COREDEV_PF)
        return -1;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(dh_dev, "failed to kzalloc\n");
        return -1;
    }

    msg->payload.hdr.op_code = ZXDH_MC_CMPAT_VERINFO;
    msg->payload.mcode_feature_msg.dev_id = 0;
    msg->payload.mcode_feature_msg.index = 1;
    err = zxdh_pf_msg_send_cmd(dh_dev, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "send_msg_to_pf failed, err: %d\n", err);
        kfree(msg);
        return err;
    }

    if (msg->reps.mcode_feature_rsp.len != sizeof(msg->reps.mcode_feature_rsp.feature))
    {
        LOG_ERR_DEV(dh_dev, "rsp len error!, len=%lld\n", msg->reps.mcode_feature_rsp.len);
        kfree(msg);
        return -1;
    }

    *mcfeature = msg->reps.mcode_feature_rsp.feature;

    kfree(msg);

    return 0;
}

/* 如果识别到微码兼容性问题，调整ZXDH_MCODE_FEATURE_VAL的值与微码feature值对齐 */
#define ZXDH_MCODE_FEATURE_INDEX    (2)
static int32_t dh_pf_mcode_compat_check(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    int32_t ret = 0;
    uint64_t mcode_feature = 0;
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = pf_dev->slot_id;
    pf_info.vport = pf_dev->vport;

    if (dh_core_is_pf(dh_dev))
    {
        ret = dpp_mcode_feature_get(&pf_info, ZXDH_MCODE_FEATURE_INDEX, &mcode_feature);
        if (ret > 0)
        {
            LOG_ERR_DEV(dh_dev, "mcode_feature_get failed! ret=%d\n", ret);
            return -ret;
        }
    }
    else
    {
        ret = zxdh_get_mcfeature_from_pf(dh_dev, &mcode_feature);
        if (ret != 0)
        {
            LOG_ERR_DEV(dh_dev, "get_mcfeature_from_pf failed! ret=%d\n", ret);
            return -1;
        }
    }

    LOG_DEBUG_DEV(dh_dev, "mcode_feature:0x%llx\n", mcode_feature);
    pf_dev->mcode_feature = mcode_feature;

    return ret;
}

void clear_zxdh_plcr_table(struct zxdh_plcr_table *table)
{
    int32_t i = 0;
    if (table->is_xarray_init)
    {
        for (i = 0; i <= 2; i++)
        {
            xa_destroy(&table->plcr_profiles[i]);
            xa_destroy(&table->plcr_flows[i]);
        }
        for (i = 0; i <= 1; i++)
        {
            xa_destroy(&table->plcr_maps[i]);
        }
        table->is_xarray_init = false;
    }

    table->burst_size = 0;
    table->is_init = false;

}

int32_t zxdh_pf_dpp_init(struct dh_core_dev *dh_dev, bool boot)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    int32_t ret = 0;
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = pf_dev->slot_id;
    pf_info.vport = pf_dev->vport;

    ret = dpp_vport_register(&pf_info, dh_dev->pdev);
    if (ret > 0)
    {
        return -ret;
    }

    if (dh_dev->coredev_type == DH_COREDEV_PF)
    {
        clear_zxdh_plcr_table(&(pf_dev->plcr_table));
        pf_dev->plcr_table.is_init = true;
    }

    if (boot) {
        ret = dh_pf_mcode_compat_check(dh_dev);
        if (ret != 0)
        {
            return ret;
        }
    }

    return ret;
}

static int32_t zxdh_pf_dpp_uninit(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = pf_dev->slot_id;
    pf_info.vport = pf_dev->vport;
    return dpp_vport_unregister(&pf_info);
}

static int32_t zxdh_pf_dpp_reset(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    DPP_PF_INFO_T pf_info = {0};

    if (dh_dev->coredev_type == DH_COREDEV_VF)
        return 0;

    pf_info.slot = pf_dev->slot_id;
    pf_info.vport = pf_dev->vport;

    return dpp_vport_reset(&pf_info);
}

int32_t zxdh_init_ip6mac_tbl(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct zxdh_ipv6_mac_entry *ip6mac_entry_list = NULL;
    unsigned int ip6mact_size = DEV_MULTICAST_MAX_NUM;
    int i;

#ifdef CGS_V5_693
    pf_dev->ip6mac_tbl = kvzalloc(sizeof(pf_dev->ip6mac_tbl) + sizeof(*(pf_dev->ip6mac_tbl)->hash_list) * ip6mact_size, GFP_KERNEL);
#else
    pf_dev->ip6mac_tbl = kvzalloc(struct_size(pf_dev->ip6mac_tbl, hash_list, ip6mact_size), GFP_KERNEL);
#endif
    if (!pf_dev->ip6mac_tbl)
    {
        LOG_ERR_DEV(dh_dev, "kvzalloc ip6mac_tbl failed\n");
        return -ENOMEM;
    }

    pf_dev->ip6mac_tbl->ip6mact_size = ip6mact_size;

    INIT_LIST_HEAD(&pf_dev->ip6mac_tbl->ip6mac_free_head);

    mutex_init(&pf_dev->ip6mac_tbl->mlock);

    for (i = 0; i < pf_dev->ip6mac_tbl->ip6mact_size; ++i)
        INIT_LIST_HEAD(&pf_dev->ip6mac_tbl->hash_list[i]);


#ifdef CGS_V5_693
    ip6mac_entry_list = kcalloc(pf_dev->ip6mac_tbl->ip6mact_size, sizeof(struct zxdh_ipv6_mac_entry), GFP_KERNEL);
    if (!ip6mac_entry_list)
        return -ENOMEM;
#else
    ip6mac_entry_list = kvcalloc(pf_dev->ip6mac_tbl->ip6mact_size, sizeof(struct zxdh_ipv6_mac_entry), GFP_KERNEL);
#endif
    if (!ip6mac_entry_list) {
        kvfree(pf_dev->ip6mac_tbl);
        LOG_ERR_DEV(dh_dev, "kvcalloc ip6mac_entry_list failed\n");
        return -ENOMEM;
    }
    pf_dev->ip6mac_tbl->ip6mac_entry_list = (void *)ip6mac_entry_list;

    for (i = 0; i < pf_dev->ip6mac_tbl->ip6mact_size; i++) {
        INIT_LIST_HEAD(&ip6mac_entry_list[i].list);
        list_add_tail(&ip6mac_entry_list[i].list, &pf_dev->ip6mac_tbl->ip6mac_free_head);
    }
    return 0;
}

void zxdh_cleanup_ip6mac_tbl(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct zxdh_ipv6_mac_tbl *ip6mac_tbl = pf_dev->ip6mac_tbl;

    if (ip6mac_tbl)
    {
        if (ip6mac_tbl->ip6mac_entry_list)
        {
            kvfree(ip6mac_tbl->ip6mac_entry_list);
            ip6mac_tbl->ip6mac_entry_list = NULL;
        }
        kvfree(ip6mac_tbl);
        pf_dev->ip6mac_tbl = NULL;
    }
}

struct pci_dev *zxdh_pf_get_pdev(struct dh_core_dev *dh_dev)
{
    return dh_dev->pdev;
}

uint64_t zxdh_pf_get_bar_virt_addr(struct dh_core_dev *dh_dev, uint8_t bar_num)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    return pf_dev->pci_ioremap_addr[bar_num];
}

//#define BAR_MSG_RETRY_CNT_MAX       (10)

uint64_t zxdh_pf_get_bar_phy_addr(struct dh_core_dev *dh_dev, uint8_t bar_num)
{
    return pci_resource_start(dh_dev->pdev, bar_num);
}

uint64_t zxdh_pf_get_bar_size(struct dh_core_dev *dh_dev, uint8_t bar_num)
{
    return pci_resource_len(dh_dev->pdev, bar_num);
}

int32_t zxdh_pf_msg_send_cmd(struct dh_core_dev *dh_dev, uint16_t module_id, void *msg, void *ack, \
                            struct zxdh_bar_extra_para *para)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    zxdh_reps_info *reps = (zxdh_reps_info *)(ack);
    vqm_rsp_host_data *vqm_reps = (vqm_rsp_host_data *)(ack);
    uint64_t vaddr = 0;
    int32_t err = 0;
    uint32_t i = 0;
    uint16_t ack_len = 0;

    vaddr = (uint64_t)ZXDH_BAR_MSG_BASE(pf_dev->pci_ioremap_addr[0]);

    for (i = 0; i < (para->retrycnt + 1); i++)
    {
        if (pf_dev->quick_remove)
            return 0;

        if (!pf_dev->bar_chan_valid)
            return -7;

        if (module_id == MODULE_PHYPORT_QUERY){
            ack_len = sizeof(struct port_message_recv);
        } else {
            ack_len = sizeof(union zxdh_msg);
        }
        err = zxdh_send_command(vaddr, pf_dev->pcie_id, module_id, msg, ack, ack_len, TRUE);
        if (((-err) != BAR_MSG_ERR_LOCK_FAILED) && ((-err) != BAR_MSG_ERR_TIME_OUT))
            break;

        if ((-err) == BAR_MSG_ERR_LOCK_FAILED)
        {
            LOG_WARN_DEV(dh_dev, "Get lock failed while send msg, try again ...(cnt:%u)", i);
            msleep(200);
        }
        if ((-err) == BAR_MSG_ERR_TIME_OUT)
        {
            LOG_WARN_DEV(dh_dev, "Timeout while send msg, try again ...(cnt:%u)", i);
            msleep(500);
        }
    }

    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_send_command failed, err=%d\n", err);
        return -1;
    }

    if (module_id == MODULE_CFG_VQM)
    {
        if (vqm_reps->check_result != 0xaa)
        {
            LOG_ERR_DEV(dh_dev, "failed reps->check_result: 0x%x\n", vqm_reps->check_result);
            return -1;
        }
        return 0;
    }

    if (reps->flag != ZXDH_REPS_SUCC)
    {
        if (reps->flag == ZXDH_INVALID_OP_CODE)
        {
            LOG_ERR_DEV(dh_dev, "msg to vf is invlaid op_code, reps->flag:0x%x\n", reps->flag);
            return ZXDH_INVALID_OP_CODE;
        }
        LOG_ERR_DEV(dh_dev, "failed reps->flag: 0x%x\n", reps->flag);
        return -1;
    }

    return err;
}

int32_t zxdh_pf_query_port(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct port_message_recv *recv_data = NULL;
    struct zxdh_port_msg *payload = NULL;
    struct zxdh_pannle_port *pnlport, *recvport;
    int32_t ret = 0, idx = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    payload = kzalloc(sizeof(struct zxdh_port_msg), GFP_KERNEL);
    if (unlikely(NULL == payload))
    {
        LOG_ERR_DEV(dh_dev, "failed to kzalloc\n");
        return -ENOMEM;
    }

    recv_data = kzalloc(sizeof(struct port_message_recv), GFP_KERNEL);
    if (unlikely(NULL == recv_data))
    {
        LOG_ERR_DEV(dh_dev, "failed to kzalloc\n");
        goto out;
    }

    payload->pcie_id = zxdh_pf_get_pcie_id(dh_dev);

    ret = zxdh_pf_msg_send_cmd(dh_dev, MODULE_PHYPORT_QUERY, payload, recv_data, &para);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_pf_query_port send message failed \n");
        goto free_recv;
    }
    LOG_DEBUG_DEV(dh_dev, "pcie_id: 0x%x, bond_num: %u lag_id: %u port_num: %u, no_bondpf_panel %d\n",
              payload->pcie_id, recv_data->bond_num, recv_data->bond_idx, recv_data->port_num, recv_data->no_bondpf_panel);

    if (recv_data->port_num > ZXDH_PANNEL_PORT_MAX)
    {
        LOG_ERR_DEV(dh_dev, "bond pf query port num from fw out of range.\n");
        ret = -1;
        goto free_recv;
    }

    if (recv_data->no_bondpf_panel > ZXDH_PANNEL_PORT_MAX)
    {
        LOG_ERR_DEV(dh_dev, "no_bondpf panel num from fw out of range.\n");
        ret = -1;
        goto free_recv;
    }

    pf_dev->no_bondpf_panel = recv_data->no_bondpf_panel;
    if ((recv_data->bond_num != 0) && (recv_data->bond_idx >= recv_data->bond_num))
    {
        LOG_ERR_DEV(dh_dev, "bond pf query bond idx from fw out of range.\n");
        ret = -1;
        goto free_recv;
    }
    pf_dev->port_resource.pannel_num = recv_data->port_num;
    pf_dev->port_resource.bond_num = recv_data->bond_num;
    pf_dev->port_resource.bond_idx = recv_data->bond_idx;

    for (idx = 0; idx < recv_data->port_num; idx++)
    {
        pnlport = &pf_dev->port_resource.port[idx];
        recvport = (struct zxdh_pannle_port *)((uint8_t *)&recv_data->data[idx]);

        pnlport->phyport = recvport->phyport;
        pnlport->pannel_id = recvport->pannel_id;
        pnlport->link_check_bit = recvport->link_check_bit;
        pnlport->flags = 0;

        LOG_DEBUG_DEV(dh_dev, "[%d] pannel %u, phyport %u link check bit %u\n", idx,
            (uint32_t)pnlport->pannel_id, (uint32_t)pnlport->phyport, (uint32_t)pnlport->link_check_bit);
    }

free_recv:
    kfree(recv_data);
out:
    kfree(payload);
    return ret;
}

int32_t zxdh_pf_query_fwinfo(struct dh_core_dev *dh_dev)
{
    int32_t ret = 0;

    ret = zxdh_pf_query_port(dh_dev);
    if (ret != 0)
    {
        return ret;
    }

    return 0;
}

uint16_t zxdh_pf_get_queue_num(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint16_t qnum = 0;

    qnum = ioread16(&pf_dev->common->num_queues);

    return qnum;
}

uint16_t zxdh_pf_get_queue_size(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint16_t queue_size = 0;

    if (pf_dev->packed_status)
    {
        iowrite16(phy_index, &pf_dev->common->queue_select);
    }
    else
    {
        iowrite16(index, &pf_dev->common->queue_select);
    }
    queue_size = ioread16(&pf_dev->common->queue_size);

    return queue_size;
}

uint16_t zxdh_pf_get_queue_vector(struct dh_core_dev *dh_dev, uint16_t channel, struct list_head *eqs_list, uint16_t phy_index, uint16_t index, uint16_t vq_idx)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct zxdh_pf_pci_common_cfg __iomem *cfg = pf_dev->common;
    struct dh_eq_vqs *eq_vqs = NULL;
    struct dh_eq_vqs *n;
    int32_t i = 0;
    int32_t msix_vec = ZXDH_MSI_NO_VECTOR;

    if (pf_dev->packed_status)
    {
        iowrite16(phy_index, &cfg->queue_select);
    }
    else
    {
        iowrite16(index, &cfg->queue_select);
    }

    list_for_each_entry_safe(eq_vqs, n, eqs_list, list)
    {
        if (i++ == channel)
        {
            iowrite16(eq_vqs->vq_s.core.irq->index, &cfg->queue_msix_vector);
            break;
        }
    }

    msix_vec = ioread16(&cfg->queue_msix_vector);
    LOG_DEBUG_DEV(dh_dev, "%s vq %d mapped to irqn %d\n", pci_name(dh_dev->pdev), vq_idx, eq_vqs->vq_s.core.irq->irqn);
    /* Flush the write out to device */
    return msix_vec;
}

void zxdh_pf_release_queue_vector(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct zxdh_pf_pci_common_cfg __iomem *cfg = pf_dev->common;

    if (pf_dev->packed_status)
    {
        iowrite16(phy_index, &cfg->queue_select);
    }
    else
    {
        iowrite16(index, &cfg->queue_select);
    }
    iowrite16(ZXDH_MSI_NO_VECTOR, &cfg->queue_msix_vector);
}

void zxdh_pf_set_queue_size(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index, uint16_t size)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    if (pf_dev->packed_status)
    {
        iowrite16(phy_index, &pf_dev->common->queue_select);
    }
    else
    {
        iowrite16(index, &pf_dev->common->queue_select);
    }
    iowrite16(size, &pf_dev->common->queue_size);
}

void zxdh_pf_set_queue_address(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index,
                               uint64_t desc_addr, uint64_t driver_addr, uint64_t device_addr)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    if (pf_dev->packed_status)
    {
        iowrite16(phy_index, &pf_dev->common->queue_select);
    }
    else
    {
        iowrite16(index, &pf_dev->common->queue_select);
    }
    iowrite32((uint32_t)desc_addr, &pf_dev->common->queue_desc_lo);
    iowrite32(desc_addr>>32, &pf_dev->common->queue_desc_hi);
    iowrite32((uint32_t)driver_addr, &pf_dev->common->queue_avail_lo);
    iowrite32(driver_addr>>32, &pf_dev->common->queue_avail_hi);
    iowrite32((uint32_t)device_addr, &pf_dev->common->queue_used_lo);
    iowrite32(device_addr>>32, &pf_dev->common->queue_used_hi);
}

/* Started by AICoder, pid:0afd1w375cx9bf4141b80b7cd00512118ff39939 */
int32_t zxdh_pf_get_phy_vq_info(uint32_t phy_index, uint32_t *phy_vq_reg, uint32_t *vq_bit)
{
    if (phy_index >= ZXDH_MAX_QUEUES_NUM)
    {
        LOG_ERR("Invalid phy_index:%u\n", phy_index);
        return -1;
    }

    *phy_vq_reg = phy_index / ZXDH_PHY_REG_BITS;
    *vq_bit = phy_index % ZXDH_PHY_REG_BITS;

    return 0;
}
/* Ended by AICoder, pid:0afd1w375cx9bf4141b80b7cd00512118ff39939 */

int32_t zxdh_pf_get_vq_lock(struct dh_core_dev *dh_dev)
{
    int32_t i = 0;
    int32_t val = 0;
    int32_t wait_time = ZXDH_PF_WAIT_COUNT;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    for (i = 0; i < wait_time; i++)
    {
        val = ioread32((void __iomem *)((uintptr_t)pf_dev->common + LOCK_VQ_REG_OFFSET));
        if (val & ZXDH_PF_LOCK_ENABLE_MASK)
        {
            break;
        }
        udelay(ZXDH_PF_DELAY_US);
    }

    if ((val & ZXDH_PF_LOCK_ENABLE_MASK) == 0)
    {
        LOG_INFO_DEV(dh_dev, "get phy vq_id is busy\n");
        return -1;
    }

    return 0;
}

int32_t zxdh_pf_release_vq_lock(struct dh_core_dev *dh_dev)
{
    int32_t val = 0;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    val = ioread32((void __iomem *)((uintptr_t)pf_dev->common + LOCK_VQ_REG_OFFSET));
    if (val & ZXDH_PF_LOCK_ENABLE_MASK)
    {
        iowrite32(ZXDH_PF_RELEASE_LOCK_VAL, (void __iomem *)((uintptr_t)pf_dev->common + LOCK_VQ_REG_OFFSET));
        return 0;
    }
    else
    {
        LOG_INFO_DEV(dh_dev, "no lock need to be released\n");
        return -1;
    }
}

int32_t find_valid_vqs_by_bit(struct dh_core_dev *dh_dev, uint8_t queue_type, uint16_t vq_cnt, int32_t *phy_index, uint16_t total_qp, uint16_t start_id)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint32_t phy_vq_reg = 0;
    uint32_t val = 0;
    uint32_t done = 0;
    uint32_t j = 0;
    uint16_t index = 0;
    uint16_t total_queue_num = total_qp * 2;
    uint16_t start_qp_id = start_id * 2;

    uint32_t phy_vq_reg_oft = start_qp_id / ZXDH_PHY_REG_BITS;
    uint32_t inval_bit = start_qp_id % ZXDH_PHY_REG_BITS;
    uint32_t res_bit = (total_queue_num + inval_bit) % ZXDH_PHY_REG_BITS;
    uint32_t vq_reg_num = (total_queue_num + inval_bit) / ZXDH_PHY_REG_BITS + (res_bit? 1 : 0);

    LOG_DEBUG_DEV(dh_dev, "phy_vq_reg_oft:%u, inval_bit is %u, res_bit:%u, vq_reg_num:%u\n",
        phy_vq_reg_oft, inval_bit, res_bit, vq_reg_num);

    for (phy_vq_reg = 0; phy_vq_reg < vq_reg_num; phy_vq_reg++)
    {
        val = ioread32((void __iomem *)(pf_dev->pci_ioremap_addr[0] + PHY_VQ_REG_OFFSET + (phy_vq_reg + phy_vq_reg_oft) * 4));

        if (phy_vq_reg == 0)
        {
            val = val | (((uint32_t)1 << inval_bit) - 1);
        }

        if ((phy_vq_reg == (vq_reg_num - 1)) && (res_bit != 0))
        {
            val = val | (~((uint32_t)1 << res_bit) + 1);
        }

        for (j = queue_type; (j < ZXDH_PHY_REG_BITS) && (index < vq_cnt); j += ZXDH_PF_POWER_INDEX2)
        {
            if ((val & (ZXDH_PF_GET_PHY_INDEX_BIT << j)) == 0)
            {
                phy_index[queue_type + 2*index] = (phy_vq_reg + phy_vq_reg_oft) * ZXDH_PHY_REG_BITS + j;
                //LOG_DEBUG("phy_index:%u, qp_bit is %u\n", (queue_type + 2*index), ((phy_vq_reg + phy_vq_reg_oft) * ZXDH_PHY_REG_BITS + j));
                index++;
            }
        }

        if (index == vq_cnt)
        {
            done = ZXDH_PF_GET_PHY_INDEX_DONE;
            break;
        }
    }

    if (done != ZXDH_PF_GET_PHY_INDEX_DONE)
    {
        LOG_ERR_DEV(dh_dev, "no availd phy queue, Currently can only apply %u %s queues.\n", index, queue_type?"tx":"rx");
        return -1;
    }

    return 0;
}

int32_t find_valid_vqs_by_type(struct dh_core_dev *dh_dev, uint8_t queue_type, uint16_t vq_cnt, int32_t *phy_index)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint32_t phy_vq_reg = 0;
    uint32_t vq_reg_num = ZXDH_MAX_QUEUES_NUM / ZXDH_PHY_REG_BITS;
    uint32_t val = 0;
    uint32_t done = 0;
    uint32_t j = 0;
    uint16_t index = 0;

    for (phy_vq_reg = 0; phy_vq_reg < vq_reg_num; phy_vq_reg++)
    {
        val = ioread32((void __iomem *)(pf_dev->pci_ioremap_addr[0] + PHY_VQ_REG_OFFSET + phy_vq_reg * 4));

        for (j = queue_type; (j < ZXDH_PHY_REG_BITS) && (index < vq_cnt); j += ZXDH_PF_POWER_INDEX2)
        {
            if ((val & (ZXDH_PF_GET_PHY_INDEX_BIT << j)) == 0)
            {
                phy_index[queue_type + 2*index] = phy_vq_reg * ZXDH_PHY_REG_BITS + j;
                index++;
            }
        }

        if (index == vq_cnt)
        {
            done = ZXDH_PF_GET_PHY_INDEX_DONE;
            break;
        }
    }

    if (done != ZXDH_PF_GET_PHY_INDEX_DONE)
    {
        LOG_ERR_DEV(dh_dev, "no availd phy queue, Currently can only apply %u %s queues.\n", index, queue_type?"tx":"rx");
        return -1;
    }

    return 0;
}

int32_t zxdh_pf_get_cross_region_vq_cnt(struct dh_core_dev *dh_dev, uint16_t *vq_cnt)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct zxdh_pf_queue_info *pf_qinfo = NULL;

    if (pf_dev == NULL || vq_cnt == NULL)
    {
        return -1;
    }

    pf_qinfo = (struct zxdh_pf_queue_info *)(pf_dev->pci_ioremap_addr[0] + ZXDH_PF_QUEUE_INFO_OFFSET);
    if (pf_qinfo == NULL)
    {
        return -1;
    }
    LOG_INFO_DEV(dh_dev, "pf_qp: %u\n", pf_qinfo->pf_qp);

    *vq_cnt = pf_qinfo->pf_qp * 2;
    if (*vq_cnt > ZXDH_MAX_QUEUES_NUM)
    {
        LOG_ERR_DEV(dh_dev, "cross_region_vq_cnt: %u out of range\n", *vq_cnt);
        return -1;
    }

    return 0;
}

int32_t zxdh_pf_find_valid_vqs_by_cross_alg(struct dh_core_dev *dh_dev, uint16_t vq_cnt, uint16_t cross_region_vq_cnt, int32_t *phy_index)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct zxdh_dev_queue_info *dev_qinfo = NULL;
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t val = 0;
    uint32_t phy_vq_reg = 0;
    uint32_t phy_vq_reg_bit = 0;
    uint32_t pf_region_num = 0;
    uint32_t pf_region_idx = 0;
    uint32_t cross_region_start_id = 0;
    uint32_t cross_region_end_id = 0;
    uint32_t cross_region_idx = 0;
    uint32_t cross_phy_index = 0;

    if (pf_dev == NULL || phy_index == NULL)
    {
        return -1;
    }

    for (i = 0; i < ZXDH_PF_NUM; i++)
    {
        dev_qinfo = (struct zxdh_dev_queue_info *)(pf_dev->pci_ioremap_addr[0] + ZXDH_DEV_QUEUE_INFO_OFFSET + i * 4);
        if (dev_qinfo == NULL)
        {
            return -1;
        }

        if (dev_qinfo->total_qp == 0)
        {
            continue;
        }

        if (i < GLOBAL_PF_IDX(EPID_GEN_FROM_VPORT(pf_dev->vport), pf_dev->vport))
        {
            pf_region_idx++;
        }

        pf_region_num++;
    }

    if (pf_region_num == 0)
    {
        return -1;
    }
    LOG_INFO_DEV(dh_dev, "pf_region_idx: %u pf_region_num: %u\n", pf_region_idx, pf_region_num);

    for (i = 0; i < ZXDH_PF_NUM; i++)
    {
        dev_qinfo = (struct zxdh_dev_queue_info *)(pf_dev->pci_ioremap_addr[0] + ZXDH_DEV_QUEUE_INFO_OFFSET + i * 4);
        if (dev_qinfo == NULL)
        {
            return -1;
        }

        if (dev_qinfo->total_qp == 0)
        {
            continue;
        }

        cross_region_start_id = (dev_qinfo->start_id + dev_qinfo->total_qp) * 2 - cross_region_vq_cnt;
        cross_region_end_id = (dev_qinfo->start_id + dev_qinfo->total_qp) * 2 - 1;
        LOG_INFO_DEV(dh_dev, "cross_region_idx: %u cross_region_start_id: %u cross_region_end_id: %u\n",
                                       cross_region_idx, cross_region_start_id, cross_region_end_id);

        for (; j < vq_cnt; j++)
        {
            cross_phy_index =  2 * (pf_region_idx % pf_region_num) + 2 * pf_region_num * ( j / 2) + (j % 2) +
                                             cross_region_start_id - (cross_region_idx * cross_region_vq_cnt);
            if (cross_phy_index > cross_region_end_id)
            {
                break;
            }

            if (cross_phy_index >= ZXDH_MAX_QUEUES_NUM)
            {
                LOG_ERR_DEV(dh_dev, "cross_phy_index %u is invalid.\n", cross_phy_index);
                return -1;
            }

            phy_vq_reg = cross_phy_index / ZXDH_PHY_REG_BITS;
            phy_vq_reg_bit = cross_phy_index % ZXDH_PHY_REG_BITS;

            val = ioread32((void __iomem *)(pf_dev->pci_ioremap_addr[0] + PHY_VQ_REG_OFFSET + phy_vq_reg * 4));
            if ((val & (ZXDH_PF_GET_PHY_INDEX_BIT << phy_vq_reg_bit)) != 0)
            {
                LOG_ERR_DEV(dh_dev, "phy index %u is invalid.\n", cross_phy_index);
                return -1;
            }
            phy_index[j] = cross_phy_index;
        }
        cross_region_idx++;
    }

    if (j != vq_cnt)
    {
        LOG_ERR_DEV(dh_dev, "no availd phy queue, Currently can only apply %u queues.\n", j);
        return -1;
    }

    return 0;
}

int32_t zxdh_pf_find_valid_vqs(struct dh_core_dev *dh_dev, uint16_t vq_cnt, int32_t *phy_index)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct zxdh_dev_queue_info *dev_qinfo = NULL;
    struct zxdh_fw_compat fw_compat = pf_dev->fw_compat;
    int32_t ret = 0;
    uint32_t pair_cnt = 0;
    uint16_t ep_id = 0;
    uint16_t pf_idx = 0;
    uint32_t cross_vq_enable = 0;
    uint16_t cross_region_vq_cnt = 0;

    pair_cnt = vq_cnt / 2;

    if ((fw_compat.patch >= 1) && zxdh_pf_is_nic(dh_dev))
    {
        if (dh_dev->coredev_type == DH_COREDEV_PF)
        {
            cross_vq_enable = ioread32((void __iomem *)(pf_dev->pci_ioremap_addr[0] + ZXDH_CROSS_QUEUE_OFFSET));
            if (cross_vq_enable == ZXDH_CROSS_QUEUE_ENABLE)
            {
                ret = zxdh_pf_get_cross_region_vq_cnt(dh_dev, &cross_region_vq_cnt);
                if (ret != 0)
                {
                    return ret;
                }
                LOG_INFO_DEV(dh_dev, "vq_cnt: %u cross_region_vq_cnt: %u\n", vq_cnt, cross_region_vq_cnt);

                ret = zxdh_pf_find_valid_vqs_by_cross_alg(dh_dev, vq_cnt, cross_region_vq_cnt, phy_index);
                if (ret != 0)
                {
                    return ret;
                }

                return 0;
            }
        }

        ep_id = EPID_GEN_FROM_VPORT(pf_dev->vport);
        pf_idx = GLOBAL_PF_IDX(ep_id, pf_dev->vport);

        dev_qinfo = (struct zxdh_dev_queue_info *)(pf_dev->pci_ioremap_addr[0] + ZXDH_DEV_QUEUE_INFO_OFFSET + pf_idx * 4);
        LOG_DEBUG_DEV(dh_dev, "pf(vport:0x%x) get queue config: ep_id:%u, pf_idx:%u, total_qp:%u, start_id:%u\n",
               pf_dev->vport, ep_id, pf_idx, dev_qinfo->total_qp, dev_qinfo->start_id);

        ret = find_valid_vqs_by_bit(dh_dev, ZXDH_PF_RQ_TYPE, pair_cnt, phy_index, dev_qinfo->total_qp, dev_qinfo->start_id);
        if (ret != 0)
        {
            return ret;
        }

        ret = find_valid_vqs_by_bit(dh_dev, ZXDH_PF_TQ_TYPE, pair_cnt, phy_index, dev_qinfo->total_qp, dev_qinfo->start_id);
        if (ret != 0)
        {
            return ret;
        }

        return 0;
    }


    ret = find_valid_vqs_by_type(dh_dev, ZXDH_PF_RQ_TYPE, pair_cnt, phy_index);
    if (ret != 0)
    {
        return ret;
    }

    ret = find_valid_vqs_by_type(dh_dev, ZXDH_PF_TQ_TYPE, pair_cnt, phy_index);
    if (ret != 0)
    {
        return ret;
    }

    return 0;
}

int32_t zxdh_pf_write_vqs_bit(struct dh_core_dev *dh_dev, uint16_t vq_cnt, uint32_t *phy_index)
{
    uint32_t phy_vq_reg = 0;
    uint32_t vq_bit = 0;
    uint32_t val = 0;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint16_t i = 0;

    for (i = 0; i < vq_cnt; ++i)
    {
        phy_vq_reg = phy_index[i] / ZXDH_PHY_REG_BITS;
        vq_bit = phy_index[i] % ZXDH_PHY_REG_BITS;

        val = ioread32((void __iomem *)(pf_dev->pci_ioremap_addr[0] + PHY_VQ_REG_OFFSET + phy_vq_reg * 4));
        val |= (ZXDH_PF_GET_PHY_INDEX_BIT << vq_bit);
        iowrite32(val, (void __iomem *)(pf_dev->pci_ioremap_addr[0] + PHY_VQ_REG_OFFSET + phy_vq_reg * 4));
    }

    return 0;
}

int32_t zxdh_pf_write_queue_tlb(struct dh_core_dev *dh_dev, uint16_t vq_cnt, uint32_t *phy_index, bool need_msgq)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint16_t i = 0;
    uint16_t pcieid = pf_dev->pcie_id;

    for (i = 0; i < vq_cnt; ++i)
    {
        pcieid = pf_dev->pcie_id;
        if (need_msgq && (i >= (vq_cnt -2)))
        {
            pcieid |= BIT(15);
        }
        iowrite16(pcieid, (void __iomem *)(pf_dev->pci_ioremap_addr[0] + pf_dev->qtlb_offset + phy_index[i] * 2));
    }

    return 0;
}

uint16_t zxdh_pf_get_fw_patch(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    return pf_dev->fw_compat.patch;
}

void zxdh_pf_update_link_info(struct dh_core_dev *dh_dev, struct link_info_struct *link_info_val)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    if(pf_dev->link_up && link_info_val->speed == SPEED_UNKNOWN)
    {
        LOG_INFO_DEV(dh_dev, "pf_dev->link_up is %d and link_info_val->speed is %d, can't update pf info\n",pf_dev->link_up ,link_info_val->speed);
        return;
    }
    pf_dev->speed = link_info_val->speed;
    pf_dev->autoneg_enable = link_info_val->autoneg_enable;
    pf_dev->supported_speed_modes = link_info_val->supported_speed_modes;
    pf_dev->advertising_speed_modes = link_info_val->advertising_speed_modes;
    pf_dev->duplex = link_info_val->duplex;
}

int32_t zxdh_pf_get_drv_msg(struct dh_core_dev *dh_dev, uint8_t *drv_version, uint8_t *drv_version_len)
{
    *drv_version_len = sizeof(zxdh_pf_driver_version);
    memcpy(drv_version, zxdh_pf_driver_version, *drv_version_len);
    return 0;
}

void zxdh_pf_set_vepa(struct dh_core_dev *dh_dev, bool setting)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    pf_dev->vepa = setting;
}

bool zxdh_pf_get_vepa(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    return pf_dev->vepa;
}

int32_t zxdh_pf_request_port(struct dh_core_dev *dh_dev, void *data)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint8_t port_num = pf_dev->port_resource.pannel_num;
    struct zxdh_pannle_port *port;
    struct zxdh_pannle_port *req_data = (struct zxdh_pannle_port *)data;
    int32_t idx = 0;

    for (idx = 0; idx < port_num; idx++)
    {
        port = &pf_dev->port_resource.port[idx];
        if (!(port->flags & PORT_FLAGS_ALLOC_STAT))
        {
            req_data->phyport = port->phyport;
            req_data->pannel_id = port->pannel_id;
            req_data->link_check_bit = port->link_check_bit;
            port->flags |= PORT_FLAGS_ALLOC_STAT;
            break;
        }
    }

    if (idx == port_num)
    {
        LOG_ERR_DEV(dh_dev, "failed to obtain the panel information, or this part is not released when the aux is removed\n");
        return -1;
    }

    return 0;
}

int32_t zxdh_pf_release_port(struct dh_core_dev *dh_dev, uint32_t pnl_id)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint8_t port_num = pf_dev->port_resource.pannel_num;
    struct zxdh_pannle_port *port;
    int32_t idx = 0;

    for (idx = 0; idx < port_num; idx++)
    {
        port = &pf_dev->port_resource.port[idx];
        if (pnl_id == port->pannel_id)
        {
            port->flags &= ~PORT_FLAGS_ALLOC_STAT;
            break;
        }
    }

    return 0;
}

void zxdh_pf_set_bond_num(struct dh_core_dev *dh_dev, bool add)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    if (add)
    {
        pf_dev->bond_num++;
    }
    else
    {
        pf_dev->bond_num--;
    }
}

bool zxdh_pf_if_init(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    if (pf_dev->bond_num == 0)
    {
        return true;
    }

    return false;
}

void zxdh_pf_set_init_comp_flag(struct dh_core_dev *dh_dev, uint8_t flag)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    LOG_DEBUG_DEV(dh_dev, "flag: %d, pf_dev->bond_num = %d, pf_dev->pannel_port_num = %d\n", \
                flag, pf_dev->bond_num, pf_dev->pannel_port_num);
    if (flag != ZXDH_AUX_COMP_FLAG) {
        pf_dev->aux_comp_flag = 0;
        return;
    }

    if (pf_dev->bond_num == pf_dev->pannel_port_num)
    {
        pf_dev->aux_comp_flag = ZXDH_AUX_COMP_FLAG;
    }
}

struct zxdh_ipv6_mac_tbl * zxdh_pf_get_ip6mac_tbl(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    return pf_dev->ip6mac_tbl;
}

static int32_t zxdh_pf_events_call_chain(struct dh_core_dev *dh_dev, unsigned long type, void *data)
{
    struct dh_eq_table *eq_table = &dh_dev->eq_table;

    return atomic_notifier_call_chain(&eq_table->nh[type], type, data);
}

/* Started by AICoder, pid:cb5f8h4da9v582014e6c0ae8d0bd2b1c27f6a602 */
uint16_t zxdh_pf_get_ovs_pf_vfid(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint32_t ovs_pf_vfid; // 使用 uint32_t 来存储4字节的数据

    // 读取4字节的数据
    ovs_pf_vfid = ioread32((void __iomem *)(pf_dev->pci_ioremap_addr[0] + ZXDH_OVS_PF_VFID_OFFSET));
    LOG_INFO_DEV(dh_dev, "pf(vport:0x%x) get ovs pf vfid 0x%x\n", pf_dev->vport, ovs_pf_vfid);

    // 将高16位截断以适应uint16_t
    return (uint16_t)ovs_pf_vfid;
}
/* Ended by AICoder, pid:cb5f8h4da9v582014e6c0ae8d0bd2b1c27f6a602 */

uint8_t zxdh_pf_get_board_type(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    return pf_dev->board_type;
}

bool zxdh_pf_is_hwbond(struct dh_core_dev *dh_dev, bool is_hwbond, bool update_pf)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    if (update_pf)
    {
        pf_dev->is_hwbond = is_hwbond;
    }

    return pf_dev->is_hwbond;
}

bool zxdh_pf_is_rdma_aux_plug(struct dh_core_dev *dh_dev, bool is_rdma_aux_plug, bool update_pf)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    if (update_pf)
    {
        pf_dev->is_rdma_aux_plug = is_rdma_aux_plug;
    }

    return pf_dev->is_rdma_aux_plug;
}

bool zxdh_pf_is_primary_port(struct dh_core_dev *dh_dev, bool is_primary_port, bool update_pf)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    if (update_pf)
    {
        pf_dev->is_primary_port = is_primary_port;
    }

    return pf_dev->is_primary_port;
}

int32_t zxdh_pf_update_hb_file_val(struct dh_core_dev *dh_dev, uint64_t spec_sbdf, const char *file_name, bool flag);

void zxdh_pf_update_lag_enable_in_rdma_port_cfg(DPP_PF_INFO_T* pf_info, uint8_t phy_port, bool enable)
{
    int rdma_port_start = RDMA_START_PORT_IN_MCODE;
    int rdma_port_end = RDMA_END_PORT_IN_MCODE;
    int rdma_port_idx = 0;
    for (rdma_port_idx = rdma_port_start; rdma_port_idx <= rdma_port_end; rdma_port_idx++)
    {
        dpp_pktrx_mcode_port_cfg_write(pf_info, rdma_port_idx, LAG_ENABLE_DATA_IDX, phy_port , phy_port, !!enable);
    }
}

void zxdh_pf_update_mp_enable_in_rdma_port_cfg(DPP_PF_INFO_T* pf_info, uint8_t phy_port, bool enable)
{
    int rdma_port_start = RDMA_START_PORT_IN_MCODE;
    int rdma_port_end = RDMA_END_PORT_IN_MCODE;
    int rdma_port_idx = 0;
    uint32_t start_pos = phy_port + PORT_NUM_IN_MCODE;
    uint32_t end_pos = start_pos;

    for (rdma_port_idx = rdma_port_start; rdma_port_idx <= rdma_port_end; rdma_port_idx++)
    {
        dpp_pktrx_mcode_port_cfg_write(pf_info, rdma_port_idx, LAG_ENABLE_DATA_IDX, start_pos , end_pos, !!enable);
    }
}

void zxdh_pf_optim_hardware_bond_time(struct dh_core_dev *dh_dev, bool enable)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    DPP_PF_INFO_T dpp_pf_info = {
        .slot = pf_dev->slot_id,
        .vport = pf_dev->vport,
    };
    if(pf_dev->bond_port_type == BOND_TWO_PORT_TYPE)
    {
        dpp_pktrx_mcode_glb_cfg_write(&dpp_pf_info, 29, 29, !!enable);
    }
    else if(pf_dev->bond_port_type == BOND_MULTI_PORT_TYPE)
    {
        zxdh_pf_update_lag_enable_in_rdma_port_cfg(&dpp_pf_info, pf_dev->phy_port, enable);
    }
}

void zxdh_pf_get_psn_feature_info(struct dh_core_dev *dh_dev, uint8_t *psn_version, bool *dual_tor, bool *quad_tor)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint16_t psn_feature_info;
    uint8_t second_byte;
    // 1. 计算4字节对齐的基地址偏移（将原偏移按4字节对齐）, 得到0x1014
    uint32_t aligned_offset = ZXDH_PSN_FEATURE_INFO_OFFSET & ~0x3;
    // 2. 计算目标数据在32位数据中的字节偏移（原偏移 - 对齐后的偏移）
    uint32_t offset_in_32 = ZXDH_PSN_FEATURE_INFO_OFFSET - aligned_offset;
    uint32_t data32;  // 存储32位读取结果

    // 3. 读取4字节对齐地址的32位数据
    data32 = ioread32((void __iomem *)(pf_dev->pci_ioremap_addr[0] + aligned_offset));
    // 4. 从32位数据中提取16位目标数据（偏移offset_in_32字节，即2字节）
    psn_feature_info = (uint16_t)((data32 >> (offset_in_32 * 8)) & 0xFFFF);
    LOG_INFO("pf(vport:0x%x) get psn feature info: 0x%04x\n", pf_dev->vport, psn_feature_info);
    // 第一个字节：psn_feature_info的低8位 → psn_version
    *psn_version = (uint8_t)(psn_feature_info & 0xFF);
    // 第二个字节：psn_feature_info的高8位
    second_byte = (uint8_t)((psn_feature_info >> 8) & 0xFF);
    // 第二个字节bit0 → dual_tor
    *dual_tor = (second_byte & BIT(0)) ? TRUE : FALSE;
    // 第二个字节bit1 → quad_tor
    *quad_tor = (second_byte & BIT(1)) ? TRUE : FALSE;

    LOG_INFO("psn_version: 0x%02x, dual_tor: %d, quad_tor: %d\n",
             *psn_version, *dual_tor, *quad_tor);
}

void zxdh_pf_update_mp_enable(struct dh_core_dev *dh_dev, bool enable)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    DPP_PF_INFO_T dpp_pf_info = {
        .slot = pf_dev->slot_id,
        .vport = pf_dev->vport,
    };
    if(pf_dev->bond_port_type == BOND_MULTI_PORT_TYPE)
    {
        zxdh_pf_update_mp_enable_in_rdma_port_cfg(&dpp_pf_info, pf_dev->phy_port, enable);
    }
}

bool zxdh_pf_get_ro_info_from_fwshrd(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint32_t ro_info = 0;

    if ((zxdh_pf_is_nic(dh_dev) || IS_STORAGE_BOARD(pf_dev->board_type)) &&
        zxdh_pf_is_fw_feature_support(dh_dev, FW_FEATURE_RO_CFG))
    {
        ro_info = ioread32((void __iomem *)(pf_dev->pci_ioremap_addr[0] + ZXDH_DEV_RO_OFFSET));
        LOG_DEBUG_DEV(dh_dev, "pcie_id:0x%x, ro_info = 0x%x", pf_dev->pcie_id, ro_info);
        return ro_info == 1;
    }
    return false;
}

uint32_t zxdh_pf_get_bond_config_from_fwshrd(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint32_t config_bond_info = 0;

    if (!zxdh_pf_is_support_bond_config(dh_dev)) {
        return 0xFFFFFFFF;
    }
    config_bond_info = ioread32((void __iomem *)(pf_dev->pci_ioremap_addr[0] + ZXDH_CONFIG_BOND_CONFIG_OFFSET));
    config_bond_info &= 0x000FFFFF; // 掩码过滤，仅保留低20bit
    LOG_INFO("pcie_id:0x%x, read bond_config = 0x%06x", pf_dev->pcie_id, config_bond_info);
    return config_bond_info;
}

#define BOND_LOW_20BIT_MASK    0x000FFFFF  // 低20bit掩码（bit19~bit0）
#define BOND_HIGH_12BIT_MASK   0xFFF00000  // 高12bit掩码（bit31~bit20）
bool zxdh_pf_update_active_bond_config_to_fwshrd(struct dh_core_dev *dh_dev, uint32_t active_bond_info)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint32_t original_value = 0;    // 地址原有32bit值
    uint32_t new_value = 0;         // 待写入的新值
    uint32_t verify_value = 0;
    uint32_t valid_bond_info;

    // 1. 检查是否支持bond配置
    if (!zxdh_pf_is_support_bond_config(dh_dev)) {
        return TRUE;
    }

    // 2. 读取地址原有32bit值
    original_value = ioread32((void __iomem *)(pf_dev->pci_ioremap_addr[0] + ZXDH_ACTIVE_BOND_CONFIG_OFFSET));
    LOG_INFO("pcie_id:0x%x, original IO addr value = 0x%08x", pf_dev->pcie_id, original_value);

    // 3. 过滤active_bond_info：仅保留低20bit有效位，避免脏数据
    valid_bond_info = active_bond_info & BOND_LOW_20BIT_MASK;
    LOG_INFO("pcie_id:0x%x, filtered active_bond_info (low 20bit) = 0x%05x", 
             pf_dev->pcie_id, valid_bond_info);

    // 4. 合并新值：保留原有高12bit + 写入新的低20bit
    new_value = (original_value & BOND_HIGH_12BIT_MASK) | valid_bond_info;
    LOG_INFO("pcie_id:0x%x, new IO addr value = 0x%08x (high12bit reserved, low20bit updated)", 
             pf_dev->pcie_id, new_value);

    // 5. 写入新值到目标IO地址
    iowrite32(new_value, (void __iomem *)(pf_dev->pci_ioremap_addr[0] + ZXDH_ACTIVE_BOND_CONFIG_OFFSET));

    // 6. 验证写入结果
    verify_value = ioread32((void __iomem *)(pf_dev->pci_ioremap_addr[0] + ZXDH_ACTIVE_BOND_CONFIG_OFFSET));
    if (verify_value != new_value) {
        LOG_ERR("pcie_id:0x%x, write failed! expect=0x%08x, actual=0x%08x", 
                pf_dev->pcie_id, new_value, verify_value);
        return FALSE;
    }

    LOG_INFO("pcie_id:0x%x, active_bond_info (low 20bit) update success", pf_dev->pcie_id);
    return TRUE;
}

bool zxdh_pf_is_rdma_no_panel(struct dh_core_dev *dh_dev)
{
    if ((dh_dev->pdev->device == ZXDH_PF_DPUB_ROCE_RDMA0_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_DPUB_ROCE_RDMA0_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_PF_DPUB_ROCE_RDMA1_DEVICE_ID) ||
        (dh_dev->pdev->device == ZXDH_VF_DPUB_ROCE_RDMA1_DEVICE_ID))
    {
        return true;
    }

    return false;
}

bool zxdh_pf_is_lowlatency(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct firmware_capability *fwcap = &pf_dev->fwcap;

    if (fwcap->is_lowlatency == 1)
    {
        return true;
    }

    return false;
}

/* Started by AICoder, pid:xb831u8b93k8b2b1464b09e770c06b0e39b6d8f3 */
uint64_t zxdh_pf_get_mcode_feature(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    return pf_dev->mcode_feature;
}
/* Ended by AICoder, pid:xb831u8b93k8b2b1464b09e770c06b0e39b6d8f3 */

void *zxdh_pf_get_dev_cap(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    return pf_dev->device;
}

uint16_t zxdh_pf_get_pre_bond_qidx(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    return pf_dev->pre_bond_qidx;
}

void zxdh_pf_set_pre_bond_qidx(struct dh_core_dev *dh_dev, uint16_t pre_bond_qidx)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    pf_dev->pre_bond_qidx = pre_bond_qidx;
    return;
}

void zxdh_pf_get_fw_version(struct dh_core_dev *dh_dev, uint8_t *fw_version)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    void __iomem *bar0_addr = (void __iomem *)(pf_dev->pci_ioremap_addr[0] + ZXDH_FW_VERSION_OFFSET);
    uint32_t i = 0;

    zte_memset_s(fw_version, 0, FW_VERSION_LEN);
    for (i = 0; i < FW_VERSION_LEN - 1; i++) {
        fw_version[i] = readb(bar0_addr + i);
        if (fw_version[i] == '\0') {
            break;
        }
    }
    fw_version[FW_VERSION_LEN - 1] = '\0';
    LOG_DEBUG_DEV(dh_dev, "%s fw_version: %s", pci_name(dh_dev->pdev), fw_version);
}

void zxdh_pf_get_vendor(struct dh_core_dev *dh_dev, uint8_t *vendor)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    void __iomem *bar0_addr = (void __iomem *)(pf_dev->pci_ioremap_addr[0] + ZXDH_VENDOR_OFFSET);
    uint32_t i = 0;

    zte_memset_s(vendor, 0, VENDOR_SIZE);
    for (i = 0; i < VENDOR_SIZE - 1; i++) {
        vendor[i] = readb(bar0_addr + i);
        if (vendor[i] == '\0') {
            break;
        }
    }
    vendor[VENDOR_SIZE - 1] = '\0';
    if (i != 0) {
        LOG_DEBUG_DEV(dh_dev, "%s vendor: %s", pci_name(dh_dev->pdev), vendor);
    }
}

bool zxdh_pf_get_split_packed(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct firmware_capability *fwcap = &pf_dev->fwcap;
    uint8_t fw_version[FW_VERSION_LEN] = {0};

    /* Priority check for special firmware version and device combination */
    zxdh_pf_get_fw_version(dh_dev, fw_version);

    if (IS_SPECIAL_VERSION_FORCE_SPLIT(fw_version, dh_dev->pdev->device,
                                        dh_dev->pdev->vendor, pf_dev->board_type)) {
        /* Special case: force split mode */
        pf_dev->packed_status = 0;
        LOG_INFO_DEV(dh_dev, "Special version detected, force split mode: fw=%s device=0x%x vendor=0x%x board=%d",
                     fw_version, dh_dev->pdev->device, dh_dev->pdev->vendor, pf_dev->board_type);
        return false;  // split mode
    }

    /* Standard firmware feature check */
    if (FW_FEATURE_GET(fwcap->fw_feature, FW_FEATURE_SPLIT_PACKED) == 0)
    {
        pf_dev->packed_status = 1;
        return true;
    }

    pf_dev->packed_status = 0;
    return false;
}

struct zxdh_en_sf_if en_sf_ops =
{
    .en_sf_map_vq_notify        = zxdh_pf_map_vq_notify,
    .en_sf_unmap_vq_notify      = zxdh_pf_unmap_vq_notify,
    .en_sf_set_status           = zxdh_pf_set_status,
    .en_sf_get_status           = zxdh_pf_get_status,
    .en_sf_get_cfg_gen          = zxdh_pf_get_cfg_gen,
    .en_sf_get_rp_link_status   = zxdh_pf_check_remove_state,
    .en_sf_get_features         = zxdh_pf_get_features,
    .en_sf_set_features         = zxdh_pf_set_features,
    .en_sf_set_vf_mac           = zxdh_pf_set_vf_mac,
    .en_sf_get_vf_mac           = zxdh_pf_get_vf_mac,
    .en_sf_set_mac              = zxdh_set_mac,
    .en_sf_get_mac              = zxdh_get_mac,
    .en_sf_set_queue_enable     = zxdh_pf_set_queue_enable,
    .en_sf_get_channels_num     = zxdh_pf_get_vqs_channels_num,
    .en_sf_get_queue_num        = zxdh_pf_get_queue_num,
    .en_sf_get_queue_size       = zxdh_pf_get_queue_size,
    .en_sf_get_queue_vector     = zxdh_pf_get_queue_vector,
    .en_sf_release_queue_vector = zxdh_pf_release_queue_vector,
    .en_sf_set_queue_size       = zxdh_pf_set_queue_size,
    .en_sf_set_queue_address    = zxdh_pf_set_queue_address,
    .en_sf_vq_irqs_request      = zxdh_pf_vq_irqs_request,
    .en_sf_affinity_irqs_release = zxdh_pf_affinity_irqs_release,
    .en_sf_switch_irq           = zxdh_pf_switch_irq,
    .en_sf_get_vq_lock          = zxdh_pf_get_vq_lock,
    .en_sf_release_vq_lock      = zxdh_pf_release_vq_lock,
    .en_sf_find_valid_vqs       = zxdh_pf_find_valid_vqs,
    .en_sf_write_vqs_bit        = zxdh_pf_write_vqs_bit,
    .en_sf_write_queue_tlb      = zxdh_pf_write_queue_tlb,
    .en_sf_get_fw_patch         = zxdh_pf_get_fw_patch,
    .en_sf_get_epbdf            = zxdh_pf_get_epbdf,
    .en_sf_get_spec_sbdf        = zxdh_pf_get_spec_sbdf,
    .en_sf_is_multi_ep          = zxdh_pf_is_multi_ep,
    .en_sf_get_vport            = zxdh_pf_get_vport,
    .en_sf_get_coredev_type     = zxdh_pf_get_coredev_type,
    .en_sf_get_pcie_id          = zxdh_pf_get_pcie_id,
    .en_sf_get_slot_id          = zxdh_pf_get_slot_id,
    .en_sf_is_bond              = zxdh_pf_is_bond,
    .en_sf_is_upf               = zxdh_pf_is_upf,
    .en_sf_get_pdev             = zxdh_pf_get_pdev,
    .en_sf_get_bar_virt_addr    = zxdh_pf_get_bar_virt_addr,
    .en_sf_get_bar_phy_addr     = zxdh_pf_get_bar_phy_addr,
    .en_sf_get_bar_size         = zxdh_pf_get_bar_size,
    .en_sf_msg_send_cmd         = zxdh_pf_msg_send_cmd,
    .en_sf_async_eq_enable      = zxdh_pf_async_eq_enable,
    .en_sf_nh_attach            = zxdh_pf_nh_attach,
    .en_sf_get_vf_item          = zxdh_pf_get_vf_item,
    .en_sf_set_pf_link_up       = zxdh_pf_set_pf_link_up,
    .en_sf_get_pf_link_up       = zxdh_pf_get_pf_link_up,
    .en_sf_update_pf_link_info  = zxdh_pf_update_link_info,
    .en_sf_get_drv_msg          = zxdh_pf_get_drv_msg,
    .en_sf_get_vepa             = zxdh_pf_get_vepa,
    .en_sf_set_vepa             = zxdh_pf_set_vepa,
    .en_sf_set_bond_num         = zxdh_pf_set_bond_num,
    .en_sf_if_init              = zxdh_pf_if_init,
    .en_sf_request_port_info    = zxdh_pf_request_port,
    .en_sf_release_port_info    = zxdh_pf_release_port,
    .en_sf_get_link_info_from_vqm = zxdh_pf_get_link_info_from_vqm,
    .en_sf_set_vf_link_info     = zxdh_pf_set_vf_link_info,
    .en_sf_get_vf_link_info     = zxdh_pf_get_vf_link_info,
    .en_sf_get_vf_is_probe      = zxdh_pf_get_vf_is_probe,
    .en_sf_set_pf_phy_port      = zxdh_pf_set_pf_phy_port,
    .en_sf_get_pf_phy_port      = zxdh_pf_get_pf_phy_port,
    .en_sf_set_init_comp_flag   = zxdh_pf_set_init_comp_flag,
    .en_sf_events_call_chain    = zxdh_pf_events_call_chain,
    .en_sf_get_ip6mac_tbl       = zxdh_pf_get_ip6mac_tbl,
    .en_sf_is_nic               = zxdh_pf_is_nic,
    .en_sf_is_special_bond      = zxdh_pf_is_special_bond,
    .en_sf_is_support_bond_config = zxdh_pf_is_support_bond_config,
    .en_sf_get_queue_pairs      = zxdh_pf_get_queue_pairs,
    .en_sf_get_cpl_timeout_if_mask = zxdh_pf_get_cpl_timeout_if_mask,
    .en_sf_set_cpl_timeout_mask    = zxdh_pf_set_cpl_timeout_mask,
    .en_sf_get_hp_irq_ctrl_status  = zxdh_pf_get_hp_irq_ctrl_status,
    .en_sf_set_hp_irq_ctrl_status  = zxdh_pf_set_hp_irq_ctrl_status,
    .en_sf_is_rdma_enable          = zxdh_pf_is_rdma_enable,
    .en_sf_get_dev_type            = zxdh_pf_get_dev_type,
    .en_sf_is_panel_port           = zxdh_pf_is_panel_port,
    .en_sf_get_panel_id            = zxdh_pf_get_panel_id,
    .en_sf_pf_suport_np_ext_stats  = zxdh_pf_suport_np_ext_stats,
    .en_sf_get_np_ext_stats        = zxdh_get_np_ext_stats,
    .en_sf_is_drs_sec_enable       = zxdh_pf_is_drs_sec_enable,
    .en_sf_is_fw_feature_support   = zxdh_pf_is_fw_feature_support,
    .en_sf_get_ovs_pf_vfid         = zxdh_pf_get_ovs_pf_vfid,
    .en_sf_get_board_type          = zxdh_pf_get_board_type,
    .en_sf_is_hwbond               = zxdh_pf_is_hwbond,
    .en_sf_is_rdma_aux_plug         = zxdh_pf_is_rdma_aux_plug,
    .en_sf_is_primary_port          = zxdh_pf_is_primary_port,
    .en_sf_optim_hardware_bond_time = zxdh_pf_optim_hardware_bond_time,
    .en_sf_update_mp_enable         = zxdh_pf_update_mp_enable,
    .en_sf_get_psn_feature_info     = zxdh_pf_get_psn_feature_info,
    .en_sf_update_hb_file_val       = zxdh_pf_update_hb_file_val,
    .en_sf_get_ro_info_from_fwshrd  = zxdh_pf_get_ro_info_from_fwshrd,
    .en_sf_get_bond_config_from_fwshrd = zxdh_pf_get_bond_config_from_fwshrd,
    .en_sf_update_active_bond_config_to_fwshrd = zxdh_pf_update_active_bond_config_to_fwshrd,
    .en_sf_is_rdma_no_panel         = zxdh_pf_is_rdma_no_panel,
    .en_sf_is_lowlatency            = zxdh_pf_is_lowlatency,
    .en_sf_set_bond_link_info       = zxdh_pf_set_bond_link_info,
    .en_sf_set_bond_slave_flag      = zxdh_pf_set_bond_slave_flag,
    .en_sf_get_mcode_feature        = zxdh_pf_get_mcode_feature,
    .en_sf_get_bond_port_type       = zxdh_pf_get_bond_port_type,
    .en_sf_get_no_bondpf_panel_num  = zxdh_pf_get_no_bondpf_panel_num,
    .en_sf_get_dev_cap             = zxdh_pf_get_dev_cap,
    .en_sf_get_pre_bond_qidx       = zxdh_pf_get_pre_bond_qidx,
    .en_sf_set_pre_bond_qidx       = zxdh_pf_set_pre_bond_qidx,
    .en_sf_get_fw_version           = zxdh_pf_get_fw_version,
    .en_sf_get_vendor               = zxdh_pf_get_vendor,
    .en_sf_get_split_packed         = zxdh_pf_get_split_packed,
};

void zxdh_adev_release(struct device *dev)
{
    return;
}

static DEFINE_IDA(zxdh_adev_ida);

int32_t zxdh_plug_aux_dev(struct dh_core_dev *dh_dev, int32_t idx)
{
    struct zxdh_auxiliary_device *adev = NULL;
    struct zxdh_pf_device *pf_dev = NULL;
    struct zxdh_en_sf_container *sf_con = NULL;
    struct zxdh_pf_adev *pf_adevs_table = NULL;
    int32_t ret = 0;

    pf_dev = dh_core_priv(dh_dev);

    if (idx >= pf_dev->adevs_num)
    {
        return 0;
    }

    pf_adevs_table = &pf_dev->adevs_table[idx];
    if (pf_adevs_table->adev != NULL)
    {
       return 0;
    }

    sf_con = kzalloc(sizeof(struct zxdh_en_sf_container), GFP_KERNEL);

    if (unlikely(sf_con == NULL))
    {
        LOG_ERR_DEV(dh_dev, "zxadev kzalloc is null\n");
        return -ENOMEM;
    }
    pf_adevs_table->aux_idx = ida_alloc(&zxdh_adev_ida, GFP_KERNEL);
    if (pf_adevs_table->aux_idx < 0)
    {
        LOG_ERR_DEV(dh_dev, "failed to allocate device id for aux drvs\n");
        goto free_kzalloc;
    }

    adev = &sf_con->adev;

    adev->id = pf_adevs_table->aux_idx;
    adev->dev.parent = &dh_dev->pdev->dev;
    adev->dev.release = zxdh_adev_release;
    adev->name = ZXDH_PF_EN_SF_DEV_ID_NAME;

    pf_adevs_table->adev = adev;
    sf_con->dh_dev = dh_dev;
    sf_con->ops = &en_sf_ops;

    ret = zxdh_auxiliary_device_init(adev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_auxiliary_device_init failed: %d\n", ret);
        goto free_ida_alloc;
    }

    ret = zxdh_auxiliary_device_add(adev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_auxiliary_device_add failed: %d\n", ret);
        goto release_aux_init;
    }

    return 0;

release_aux_init:
    zxdh_auxiliary_device_uninit(adev);
free_ida_alloc:
    ida_simple_remove(&zxdh_adev_ida, pf_adevs_table->aux_idx);
    pf_adevs_table->aux_idx = -1;
free_kzalloc:
    kfree(sf_con);
    sf_con = NULL;
    return ret;
}

void zxdh_unplug_aux_dev(struct dh_core_dev *dh_dev, int32_t idx)
{
    struct zxdh_pf_device *pf_dev = NULL;
    struct zxdh_en_sf_container *sf_con = NULL;
    struct zxdh_pf_adev *pf_adevs_table = NULL;

    pf_dev = dh_core_priv(dh_dev);
    if (idx >= pf_dev->adevs_num)
    {
        return;
    }

    pf_adevs_table = &pf_dev->adevs_table[idx];
    if (!pf_adevs_table->adev)
    {
        return;
    }

    sf_con = container_of(pf_adevs_table->adev, struct zxdh_en_sf_container, adev);

    zxdh_auxiliary_device_delete(pf_adevs_table->adev);
    zxdh_auxiliary_device_uninit(pf_adevs_table->adev);
    ida_simple_remove(&zxdh_adev_ida, pf_adevs_table->aux_idx);
    pf_adevs_table->aux_idx = -1;
    kfree(sf_con);
    sf_con = NULL;

    return;
}

int32_t dh_pf_vf_vport_get(struct dh_core_dev *dev, uint16_t vf_idx, uint16_t *vport)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);
    uint16_t pcie_id = 0;
    struct zxdh_pci_bar_msg in = {0};
    struct zxdh_msg_recviver_mem result = {0};
    uint8_t recv_buf[8] = {0};
    uint32_t i = 0;
    int32_t ret = 0;

    if (vport == NULL)
    {
        return BAR_MSG_ERR_NULL;
    }

    pcie_id = FIND_VF_PCIE_ID(pf_dev->pcie_id, vf_idx);

    in.virt_addr = (uint64_t)ZXDH_BAR_MSG_BASE(pf_dev->pci_ioremap_addr[0]);
    in.payload_addr = &pcie_id;
    in.payload_len = sizeof(pcie_id);
    in.src = MSG_CHAN_END_PF;
    in.dst = MSG_CHAN_END_RISC;
    in.event_id = MODULE_VPORT_GET;
    in.src_pcieid = pf_dev->pcie_id;

    result.recv_buffer = recv_buf;
    result.buffer_len = sizeof(recv_buf);

    for (i = 0; i < (BAR_MSG_RETRY_CNT_MAX + 1) ; i++)
    {
        ret = zxdh_bar_chan_sync_msg_send(&in, &result);
        if (ret == BAR_MSG_OK)
        {
            *vport = *(uint16_t *)(recv_buf + 4);
            LOG_DEBUG_DEV(dev, "pf(0x%x) get vf(%u) vport(0x%x) success\n", pf_dev->pcie_id, vf_idx, *vport);
            return ret;
        }
        else if((ret != BAR_MSG_ERR_LOCK_FAILED) && (ret != BAR_MSG_ERR_TIME_OUT))
        {
            LOG_ERR_DEV(dev, "Failed to pf(0x%x) get vf(%u) vport, ret:%d.\n", pcie_id, vf_idx, ret);
            return ret;
        }

        if (ret == BAR_MSG_ERR_LOCK_FAILED)
        {
            LOG_WARN_DEV(dev, "Get lock failed while send msg, try again ...(cnt:%u)", i);
            msleep(200);
        }

        if (ret == BAR_MSG_ERR_TIME_OUT)
        {
            LOG_WARN_DEV(dev, "Timeout while send msg, try again ...(cnt:%u)", i);
            msleep(500);
        }
    }
    return ret;
}

int32_t dh_pf_vf_item_init(struct dh_core_dev *dev, uint16_t vf_idx)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);
    struct zxdh_vf_item *vf_item = NULL;

    if (pf_dev->vf_item == NULL)
    {
        LOG_ERR_DEV(dev, "vf_item is NULL\n");
        return -EINVAL;
    }
    vf_item = &pf_dev->vf_item[vf_idx];
    vf_item->link_forced = false;
    vf_item->vport = pf_dev->vf_item[0].vport + vf_idx;
    vf_item->enable = true;
    vf_item->spoofchk = false;
    mutex_init(&vf_item->lock);
    vf_item->init_np_stats = kzalloc(sizeof(struct zxdh_en_vport_np_stats), GFP_KERNEL);
    if (vf_item->init_np_stats == NULL)
    {
        LOG_ERR_DEV(dev, "pf_dev->vf_item->init_np_stats failed\n");
        return -ENOMEM;
    }
    return 0;
}

void dh_pf_vf_item_mac_init(struct zxdh_pf_device *pf_dev, int32_t num_vfs)
{
    int32_t vf_idx = 0;
    uint8_t mac[6] = {0};

    for (vf_idx = 0; vf_idx < num_vfs; vf_idx++)
    {
        zxdh_pf_set_vf_mac_reg(pf_dev, mac, vf_idx);
    }
}

int32_t dh_pf_vf_item_uninit(struct dh_core_dev *dev, uint16_t vf_idx)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);
    struct zxdh_vf_item *vf_item = NULL;

    if (pf_dev->vf_item == NULL)
    {
        LOG_ERR_DEV(dev, "vf_item is NULL\n");
        return -EINVAL;
    }
    vf_item = &pf_dev->vf_item[vf_idx];
    eth_zero_addr(vf_item->mac);
    vf_item->pf_set_mac = false;
    vf_item->enable = false;
    vf_item->vlan = 0;
    vf_item->qos = 0;
    vf_item->vlan_proto = 0;
    vf_item->spoofchk = false;
    mutex_destroy(&vf_item->lock);
    kfree(vf_item->init_np_stats);
    return 0;
}

int32_t dh_pf_vf_enable(struct dh_core_dev *dev, int32_t num_vfs)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);
    int32_t vf_idx = 0;
    int32_t ret = 0;

    ret = dh_pf_vf_vport_get(dev, 0, &pf_dev->vf_item[0].vport);
    if (ret != 0)
    {
        return ret;
    }

    for (vf_idx = 0; vf_idx < num_vfs; vf_idx++)
    {
        ret = dh_pf_vf_item_init(dev, vf_idx);
        if (ret != 0)
        {
            LOG_ERR_DEV(dev, "Failed to init vf(%d) item\n", vf_idx);
            return ret;
        }
    }

    return ret;
}

void dh_pf_vf_disable(struct dh_core_dev *dev, int32_t num_vfs)
{
    int32_t vf_idx = 0;

    for (vf_idx = 0; vf_idx < num_vfs; vf_idx++)
    {
        dh_pf_vf_item_uninit(dev, vf_idx);
    }
}

int32_t dh_pf_sriov_enable(struct pci_dev *pdev, int32_t num_vfs)
{
    struct dh_core_dev *dev = pci_get_drvdata(pdev);
    int32_t pre_existing_vfs = pci_num_vf(pdev);
    int32_t ret = 0;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);

    if ((pre_existing_vfs != 0) && (pre_existing_vfs == num_vfs))
    {
        return 0;
    }

    ret = dh_pf_vf_enable(dev, num_vfs);
    if (ret != 0)
    {
        LOG_ERR_DEV(dev, "Failed to enable vf\n");
        return ret;
    }

#ifndef CGS_V5_693
#ifdef ZXDH_SRIOV_SYSFS_EN
    ret = zxdh_create_vfs_sysfs(dev, num_vfs);
    if (ret != 0)
    {
        LOG_ERR_DEV(dev, "zxdh_create_vfs_sysfs failed : %d\n", ret);
        goto err_create_vfs_sysfs;
    }
#endif
#endif

    ret = pci_enable_sriov(pdev, num_vfs);
    if (ret != 0)
    {
        LOG_ERR_DEV(dev, "pci_enable_sriov failed : %d\n", ret);
        goto err_pci_enable_sriov;
    }

    LOG_DEBUG_DEV(dev, "start init_vf_link_info_work");
    zxdh_events_work_enqueue(dev, &pf_dev->init_vf_link_info_work);

    /* After VF enabled, check and switch to SOFTWARE_BOND if needed */
    if (pf_dev) {
        zxdh_lag_vf_enable_to_sw_bond(pf_dev);
    }

    return ret;

err_pci_enable_sriov:
#ifndef CGS_V5_693
#ifdef ZXDH_SRIOV_SYSFS_EN
    zxdh_destroy_vfs_sysfs(dev, num_vfs);
err_create_vfs_sysfs:
#endif
#endif
    dh_pf_vf_disable(dev, num_vfs);

    return ret;
}

void dh_pf_sriov_disable(struct pci_dev *pdev)
{
    struct dh_core_dev *dev = pci_get_drvdata(pdev);
    int32_t num_vfs = pci_num_vf(pdev);
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);

    dh_pf_vf_item_mac_init(pf_dev, num_vfs);
    pci_disable_sriov(pdev);
#ifndef CGS_V5_693
#ifdef ZXDH_SRIOV_SYSFS_EN
    zxdh_destroy_vfs_sysfs(dev, num_vfs);
#endif
#endif
    dh_pf_vf_disable(dev, num_vfs);

    /* After VF disabled, check and switch to HARDWARE_BOND if needed */
    if (pf_dev) {
        zxdh_lag_vf_disable_to_hw_bond(pf_dev);
    }

}

int32_t dh_pf_vf_item_create(struct dh_core_dev *dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);

    if (dev->coredev_type == DH_COREDEV_PF)
    {
        pf_dev->vf_item = kzalloc(sizeof(struct zxdh_vf_item) * ZXDH_VF_NUM_MAX, GFP_KERNEL);
        if (pf_dev->vf_item == NULL)
        {
            LOG_ERR_DEV(dev, "pf_dev->vf_item kzalloc failed\n");
            return -ENOMEM;
        }
    }

    return 0;
}

void dh_pf_vf_item_destroy(struct dh_core_dev *dev, bool disable_vf)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);

    if (dev->coredev_type == DH_COREDEV_PF)
    {
        if (disable_vf)
        {
            pci_disable_sriov(dev->pdev);
        }
        if (pf_dev->vf_item != NULL)
        {
            kfree(pf_dev->vf_item);
            pf_dev->vf_item = NULL;
        }
    }
}

bool is_sn_invalid(uint8_t sn_code[])
{
    bool all_zero = true;
    bool all_ff = true;
    uint8_t i = 0;

    for (i = 0; i < SN_CODE_LENGTH; ++i) {
        if (sn_code[i] != 0) {
            all_zero = false;
        }
        if (sn_code[i] != 0xff) {
            all_ff = false;
        }
        if (!all_zero && !all_ff) {
            break;
        }
    }

    return all_zero || all_ff;
}

#define DH_SN_OFFSET (0x5690)
static int32_t zxdh_nic_sn_get(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct nic_sn_info sn_info = {0};

    memcpy(&sn_info, (void *)(pf_dev->pci_ioremap_addr[0] + DH_SN_OFFSET), sizeof(struct nic_sn_info));

    if ((sn_info.fixed_sn_valid != 0xaa) && (sn_info.pseudo_sn_valid != 0xaa)) {
        memcpy(sn_info.sn_code, pci_name(dh_dev->pdev),
            strlen(pci_name(dh_dev->pdev)) < SN_CODE_LENGTH ?
            strlen(pci_name(dh_dev->pdev)) : SN_CODE_LENGTH);
        sn_info.pseudo_sn_valid = 0xaa;
        memcpy((void *)(pf_dev->pci_ioremap_addr[0] + DH_SN_OFFSET), &sn_info, sizeof(struct nic_sn_info));
    }

    if (is_sn_invalid(sn_info.sn_code))
        return -2;

    memcpy(pf_dev->sn_code, sn_info.sn_code, SN_CODE_LENGTH);

    return 0;
}

static int32_t dh_pf_slot_id_get(struct zxdh_pf_device *pf_dev)
{
    uint16_t i = 0;

    for (i = 1; i < DPP_PCIE_SLOT_MAX; i++)
    {
        if (is_sn_invalid(dh_slot[i].sn_code))
        {
            memcpy(dh_slot[i].sn_code, pf_dev->sn_code, SN_CODE_LENGTH);
            dh_slot[i].pf_info.slot = i;
            dh_slot[i].pf_info.vport = pf_dev->vport;
            pf_dev->slot_id = i;
            dh_slot[i].board_type = pf_dev->board_type;
            dh_slot[i].refcnt++;
            break;
        }

        if (memcmp(pf_dev->sn_code, dh_slot[i].sn_code, SN_CODE_LENGTH) == 0)
        {
            dh_slot[i].pf_info.slot = i;
            dh_slot[i].pf_info.vport = pf_dev->vport;
            pf_dev->slot_id = i;
            dh_slot[i].board_type = pf_dev->board_type;
            dh_slot[i].refcnt++;
            break;
        }

    }

    if (i == DPP_PCIE_SLOT_MAX)
    {
        return -1;
    }

    return 0;
}

int32_t dh_pf_pcie_id_get(struct dh_core_dev *dh_dev)
{
    int32_t pos = 0;
    uint8_t type = 0;
    uint16_t padding = 0;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct pci_dev *pdev = dh_dev->pdev;

    if (dh_dev->coredev_type == DH_COREDEV_PF)
    {
        if (zxdh_nic_sn_get(dh_dev))
        {
            LOG_ERR_DEV(dh_dev, "zxdh_nic_sn_get failed\n");
            return -1;
        }

        if (dh_pf_slot_id_get(pf_dev))
        {
            LOG_ERR_DEV(dh_dev, "dh_pf_slot_id_get failed\n");
            return -1;
        }
        LOG_DEBUG_DEV(dh_dev, "lot_id: 0x%x, board_type:%d\n",
                        pf_dev->slot_id, pf_dev->board_type);
    }
    for (pos = pci_find_capability(pdev, PCI_CAP_ID_VNDR); pos > 0; pos = pci_find_next_capability(pdev, pos, PCI_CAP_ID_VNDR))
    {
        pci_read_config_byte(pdev, pos + offsetof(struct zxdh_pf_pci_cap, cfg_type), &type);

        if (type == ZXDH_PCI_CAP_PCI_CFG)
        {
            pci_read_config_word(pdev, pos + offsetof(struct zxdh_pf_pci_cap, padding[0]), &padding);
            pf_dev->pcie_id = padding;
            LOG_DEBUG_DEV(dh_dev, "pcie_id: 0x%x\n", pf_dev->pcie_id);
            return 0;
        }
    }

    LOG_INFO_DEV(dh_dev, "the pci_cap that meets the requirements is not matched\n");
    return -1;
}

static uint64_t pci_size(uint64_t base, uint64_t maxbase, uint64_t mask)
{
    uint64_t size = mask & maxbase;

    if (!size)
        return 0;
    size = size & ~(size-1);
    if (base == maxbase && ((base | (size - 1)) & mask) != mask)
        return 0;
    return size;
}

int32_t zxdh_send_pxe_status_to_riscv(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    zxdh_cfg_np_msg msg = {0};
    uint64_t vaddr = 0;
    int32_t err = 0;
    uint16_t ack_len = sizeof(zxdh_cfg_np_msg);

    if (dh_dev->coredev_type != DH_COREDEV_PF)
    {
        return 0;
    }

    msg.dev_id = 0;
    msg.type = ZXDH_CFG_NPSDK_TYPE;
    msg.operate_mode = ZXDH_STOP_PXE_MODE;

    vaddr = (uint64_t)ZXDH_BAR_MSG_BASE(pf_dev->pci_ioremap_addr[0]);

    err = zxdh_send_command(vaddr, pf_dev->pcie_id, MODULE_NPSDK, &msg, &msg, ack_len, true);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "send pxe status to config np failed: %d\n", err);
    }

    return err;
}

int32_t dh_pf_sriov_cap_cfg_init(struct dh_core_dev *dh_dev)
{
    int32_t pos = 0;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct pci_dev *pdev = dh_dev->pdev;
    uint32_t bar_address32 = 0;
    uint64_t bar_address64 = 0;
    uint64_t bar_size64 = 0;
    uint32_t bar_size32 = 0;
    uint64_t mask64 = 0;
    uint32_t mem_type = 0;
    uint16_t nr_virtfn = 0;

    if(dh_dev->coredev_type == DH_COREDEV_VF)
    {
        return 0;
    }

    pos = pci_find_ext_capability(pdev, PCI_EXT_CAP_ID_SRIOV);
    if (pos == 0)
    {
        return 0;
    }

    pci_read_config_word(pdev, pos + PCI_SRIOV_TOTAL_VF, &nr_virtfn);
    if (nr_virtfn == 0)
    {
        return 0;
    }

    pci_read_config_dword(pdev, pos + PCI_SRIOV_BAR, &bar_address32);
    pci_write_config_dword(pdev, pos + PCI_SRIOV_BAR, ~0);
    pci_read_config_dword(pdev, pos + PCI_SRIOV_BAR, &bar_size32);
    pci_write_config_dword(pdev, pos + PCI_SRIOV_BAR, bar_address32);

    bar_size64 = bar_size32 & PCI_BASE_ADDRESS_MEM_MASK;
    bar_address64 = bar_address32 & PCI_BASE_ADDRESS_MEM_MASK;
    mask64 = (uint32_t)PCI_BASE_ADDRESS_MEM_MASK;
    mem_type = bar_address32 & PCI_BASE_ADDRESS_MEM_TYPE_MASK;

    if(mem_type == PCI_BASE_ADDRESS_MEM_TYPE_64)
    {
        pci_read_config_dword(pdev, pos + PCI_SRIOV_BAR + 4, &bar_address32);
        pci_write_config_dword(pdev, pos + PCI_SRIOV_BAR + 4, ~0);
        pci_read_config_dword(pdev, pos + PCI_SRIOV_BAR + 4, &bar_size32);
        pci_write_config_dword(pdev, pos + PCI_SRIOV_BAR + 4, bar_address32);

        bar_size64 |= ((uint64_t)bar_size32 << 32);
        bar_address64 |= ((uint64_t)bar_address32 << 32);
        mask64 |= ((uint64_t)~0 << 32);
    }

    bar_size64 = pci_size(bar_address64, bar_size64, mask64);
    if (!bar_size64) {
        LOG_ERR_DEV(dh_dev, "reg 0x%x: invalid BAR (can't size)\n", pos);
    }

    if (bar_address64 == 0)
    {
        pf_dev->pf_sriov_cap_base = NULL;
        return 0;
    }

    pf_dev->pf_sriov_cap_base = (void __iomem *)ioremap(bar_address64, bar_size64 * nr_virtfn);
    if (!pf_dev->pf_sriov_cap_base)
    {
        LOG_ERR_DEV(dh_dev, "ioremap(0x%llx, 0x%llx) failed\n", bar_address64, bar_size64 * nr_virtfn);
    }
    pf_dev->sriov_bar_size = bar_size64;
    return 0;
}

static uint8_t zxdh_pf_fwcap_readb(struct dh_core_dev *dh_dev, uint32_t offset)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint64_t vaddr = (uint64_t)ZXDH_BAR_FWCAP(pf_dev->pci_ioremap_addr[0]);

    return readb((const volatile void __iomem *)(vaddr + offset));
}

static bool zxdh_pf_is_ovs(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint8_t product = pf_dev->product_type;

    if ((product == ZXDH_PRODUCT_OVS)
        || (product == ZXDH_PRODUCT_NEO)
        || (product == ZXDH_PRODUCT_EVB_EP0)
        || (product == ZXDH_PRODUCT_EVB_EP0_EP4))
    {
        return true;
    }

    return false;
}

static bool zxdh_pf_is_bond_pf_in_ovs(struct dh_core_dev *dh_dev)
{
    if (dh_core_is_pf(dh_dev) && zxdh_pf_is_bond(dh_dev) && zxdh_pf_is_ovs(dh_dev))
        return true;

    return false;
}

static int32_t zxdh_pf_lag_init(struct dh_core_dev *dh_dev, int32_t *port_num)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    pf_dev->pannel_port_num = 1;

    /* BOND PF && 非OVS场景，则产生异常 */
    if (zxdh_pf_is_bond(dh_dev) && !zxdh_pf_is_ovs(dh_dev))
    {
        LOG_ERR_DEV(dh_dev, "pf is not ovs\n");
        return -1;
    }

    if (!zxdh_pf_is_bond_pf_in_ovs(dh_dev))
        goto out;

    pf_dev->pannel_port_num = pf_dev->port_resource.pannel_num;
    zxdh_regitster_ldev(dh_dev);

out:
    *port_num = pf_dev->pannel_port_num;
    return 0;
}

static void zxdh_pf_lag_exit(struct dh_core_dev *dh_dev)
{
    if (!zxdh_pf_is_bond_pf_in_ovs(dh_dev))
        return;

    zxdh_unregitster_ldev(dh_dev);
}

int32_t dh_pf_adevs_table_init(struct dh_core_dev *dh_dev, int32_t nr)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    pf_dev->adevs_table = kzalloc(sizeof(*(pf_dev->adevs_table)) * nr, GFP_KERNEL);
    if (!pf_dev->adevs_table)
    {
        pf_dev->adevs_num = 0;
        LOG_ERR_DEV(dh_dev, "pf_dev->adevs_table kzalloc failed\n");
        return -ENOMEM;
    }

    pf_dev->adevs_num = nr;

    return 0;
}

int32_t zxdh_pf_vf_qpairs_uninit(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct zxdh_fw_compat fw_compat = pf_dev->fw_compat;
    uint8_t vf_qp_user_max = 0;
    uint16_t ep_id = 0;
    uint16_t pf_idx = 0;

    if (fw_compat.patch < 1)
    {
        return 0;
    }

    if (dh_dev->coredev_type != DH_COREDEV_PF)
    {
        return 0;
    }

    ep_id = EPID_GEN_FROM_VPORT(pf_dev->vport);
    pf_idx = GLOBAL_PF_IDX(ep_id, pf_dev->vport);

    vf_qp_user_max = ioread8((void __iomem*)(pf_dev->pci_ioremap_addr[0] + ZXDH_VF_MAX_QUEUE_USER_OFFSET));
    iowrite8(vf_qp_user_max, (void __iomem*)(pf_dev->pci_ioremap_addr[0] + ZXDH_VF_QUEUE_USER_OFFSET + pf_idx));

    return 0;
}

void dh_pf_adevs_table_destroy(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    if (pf_dev->adevs_table != NULL)
    {
        kfree(pf_dev->adevs_table);
        pf_dev->adevs_table = NULL;
        pf_dev->adevs_num = 0;
    }
}

void zxdh_unplug_aux_dev_all(struct dh_core_dev *dh_dev)
{
    int32_t idx;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    for (idx = 0; idx < pf_dev->adevs_num; idx++)
    {
        zxdh_unplug_aux_dev(dh_dev, idx);
    }
}

int32_t dh_pf_fw_compat_check(struct dh_core_dev *dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);
    struct zxdh_fw_compat *fw_compat = NULL;
    uint32_t i = 0;
    uint32_t value = 0;

    /* 固件侧bar ok启动10s，留有余量设置20秒 */
    for (i = 0; i < 20; i++) {
        value = ioread32((void __iomem*)pf_dev->pci_ioremap_addr[0] + ZXDH_FW_COMPAT_OFFSET);
        if (value != 0xffffffff) {
            LOG_INFO_DEV(dev, "wait %u s, bar_ok\n", i);
            break;
        }
        msleep(1000);
    }

    fw_compat = (struct zxdh_fw_compat *)((void __iomem*)pf_dev->pci_ioremap_addr[0] + ZXDH_FW_COMPAT_OFFSET);
    memcpy(&pf_dev->fw_compat, (uint8_t *)fw_compat, sizeof(struct zxdh_fw_compat));

    if (ZXDH_MODULE_ID != fw_compat->module_id)
    {
        LOG_INFO_DEV(dev, "The module id %u from fw version is wrong, ignore fw compat check\n", fw_compat->module_id);
        return 0;
    }

    if (ZXDH_MAJOR != fw_compat->major)
    {
        LOG_ERR_DEV(dev, "drv major:%u is not match fw:%u!\n", ZXDH_MAJOR, fw_compat->major);
        return -1;
    }

    if (ZXDH_FW_MINOR > fw_compat->fw_minor)
    {
        LOG_ERR_DEV(dev, "drv fw_minor:%u is higher than fw:%u!\n", ZXDH_FW_MINOR, fw_compat->fw_minor);
        return -1;
    }

    if (ZXDH_DRV_MINOR < fw_compat->drv_minor)
    {
        LOG_ERR_DEV(dev, "drv drv_minor:%u is lower than fw:%u!\n", ZXDH_DRV_MINOR, fw_compat->drv_minor);
        return -1;
    }

    LOG_DEBUG_DEV(dev, "%s fw_compat.patch = %d", pci_name(dev->pdev), pf_dev->fw_compat.patch);
    if (pf_dev->fw_compat.patch >= DH_NEW_QUEEU_ALLOC_PATCH)
    {
        pf_dev->qtlb_offset = ioread32((void __iomem *)(pf_dev->pci_ioremap_addr[0] + ZXDH_VQ_TLB_OFFSET + 4));
        pf_dev->qtlb_offset = (pf_dev->qtlb_offset << 32) + ioread32((void __iomem *)(pf_dev->pci_ioremap_addr[0] + ZXDH_VQ_TLB_OFFSET));
        LOG_DEBUG_DEV(dev, "qtlb_offset: 0x%llx", pf_dev->qtlb_offset);
        //防止读出来的地址值有问题，这里需要尽量做保护
        if ((pf_dev->qtlb_offset + 2*ZXDH_MAX_QUEUES_NUM) > pci_resource_len(dev->pdev, 0))
        {
            LOG_ERR_DEV(dev, "pf_dev->qtlb_offset out-off rang, (pf_dev->qtlb_offset + 2*ZXDH_MAX_QUEUES_NUM) over BAR0 size: %llx!", \
                                                                                                pci_resource_len(dev->pdev, 0));

            return -1;
        }
    }

    return 0;
}

void dh_pf_fwcap_init(struct dh_core_dev *dev)
{
#define FWCAP_BAR_READ_UNIT  (4)
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);
    uint32_t idx = 0;
    uint32_t group = 0;

    group = sizeof(struct firmware_capability) / FWCAP_BAR_READ_UNIT;

    for (idx = 0; idx < group; idx++)
    {
        *((uint32_t *)&pf_dev->fwcap + idx) = *((uint32_t *)(pf_dev->pci_ioremap_addr[0] + ZXDH_FW_CAP_OFFSET) + idx);
    }

    pf_dev->board_type = ioread8((void __iomem*)pf_dev->pci_ioremap_addr[0] + ZXDH_FW_CAP_OFFSET + 1);
    pf_dev->product_type = zxdh_pf_fwcap_readb(dev, ZXDH_PRODUCT_TYPE);
    LOG_DEBUG_DEV(dev, "%s, board_type: %d, product type: %d\n",
            pci_name(dev->pdev), pf_dev->board_type, pf_dev->product_type);

    return;
}

int dh_pf_bar_cfg_init(struct dh_core_dev *dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);
    struct firmware_capability *fwcap = &pf_dev->fwcap;

    if(dev->coredev_type == DH_COREDEV_VF)
    {
        return 0;
    }

    if (FW_FEATURE_GET(fwcap->fw_feature, FW_FEATURE_COREDUMP) == 0)
    {
        return 0;
    }

    if (FIND_PF_ID(pf_dev->pcie_id) != 0)
    {
        return 0;
    }

    pf_dev->pci_ioremap_addr[2] = (uint64_t)ioremap(pci_resource_start(dev->pdev, 2), pci_resource_len(dev->pdev, 2));
    if (pf_dev->pci_ioremap_addr[2] == 0)
    {
        LOG_ERR_DEV(dev, "ioremap(0x%llx, 0x%llx) failed\n", pci_resource_start(dev->pdev, 2), pci_resource_len(dev->pdev, 2));
        return -1;
    }

    return 0;
}

void dh_pf_bar_cfg_uninit(struct dh_core_dev *dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);
    struct firmware_capability *fwcap = &pf_dev->fwcap;
    if (FW_FEATURE_GET(fwcap->fw_feature, FW_FEATURE_COREDUMP) == 0)
    {
        return;
    }

    if (dev->coredev_type == DH_COREDEV_PF && pf_dev->pci_ioremap_addr[2] != 0)
    {
        iounmap((void *)pf_dev->pci_ioremap_addr[2]);
    }
    return;
}

#define DH_PCI_VPD_MAX_SIZE         0x100
#define DH_PCI_VPD_NEW_MAX_SIZE     0x200
size_t zxdh_pf_vpd_max_cfg_size(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct firmware_capability *fwcap = &pf_dev->fwcap;

    if (FW_FEATURE_GET(fwcap->fw_feature, FW_FEATURE_VPD_CFG) == 1)
    {
        LOG_DEBUG_DEV(dh_dev, "vpd max size is 0x200");
        return DH_PCI_VPD_NEW_MAX_SIZE;
    }

    LOG_DEBUG_DEV(dh_dev, "vpd max size is 0x100");
    return DH_PCI_VPD_MAX_SIZE;
}

void zxdh_pf_vq_pairs_config(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct zxdh_pf_queue_info *pf_qinfo = NULL;

    pf_qinfo = (struct zxdh_pf_queue_info *)(pf_dev->pci_ioremap_addr[0] + ZXDH_PF_QUEUE_INFO_OFFSET);
    pf_dev->vq_pairs = (pf_qinfo->pf_qp < ZXDH_QUEUE_PAIRS_MAX) ? pf_qinfo->pf_qp : ZXDH_QUEUE_PAIRS_MAX;
    LOG_DEBUG_DEV(dh_dev, "setup pf(vport:0x%x) queue pairs to %u\n", pf_dev->vport, pf_dev->vq_pairs);
}

void zxdh_pf_vf_vq_pairs_config(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint16_t ep_id = 0;
    uint16_t vf_idx = 0;
    uint8_t *addr = NULL;
    uint8_t val = 0;
    uint8_t power = 0;
    uint8_t vq_pairs = 0;

   /* vport bit[12:14] ep_id */
    ep_id = EPID_GEN_FROM_VPORT(pf_dev->vport);
    vf_idx = GLOBAL_VF_IDX(ep_id, pf_dev->vport);
    addr = (uint8_t*)pf_dev->pci_ioremap_addr[0] + ZXDH_VF_QUEUE_PAIRS_OFFSET + (vf_idx / 2);
    val = ioread8((void __iomem*)addr);
    if (vf_idx % 2)
    { /* 从高4位获取队列的次幂 */
        power = (val & 0xf0) >> 4;
    }
    else
    { /* 从低4位获取队列的次幂 */
        power = val & 0xf;
    }

    vq_pairs = 1 << power;
    if (vq_pairs > ZXDH_QUEUE_PAIRS_MAX)
    {
        LOG_ERR_DEV(dh_dev, "vf(vport:0x%x) get queue pairs:%u exceeds max value, using default:%u\n",
                 pf_dev->vport, vq_pairs, ZXDH_QUEUE_PAIRS_MAX);
        vq_pairs = ZXDH_QUEUE_PAIRS_MAX;
    }
    pf_dev->vq_pairs = vq_pairs;
    LOG_DEBUG_DEV(dh_dev, "setup vf(vport:0x%x) queue pairs to %u\n", pf_dev->vport, pf_dev->vq_pairs);
}

int32_t zxdh_pf_vq_pairs_init(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct zxdh_fw_compat fw_compat = pf_dev->fw_compat;

    if (zxdh_pf_is_special_bond(dh_dev) && (dh_dev->coredev_type == DH_COREDEV_PF))
    {
        zxdh_pf_vq_pairs_config(dh_dev);
        return 0;
    }

    if (zxdh_pf_is_nic(dh_dev) == false)
    {
        return 0;
    }

    if (fw_compat.patch < 1)
    {
        pf_dev->vq_pairs = ZXDH_MAX_QPS_NUM;
        return 0;
    }

    if (dh_dev->coredev_type == DH_COREDEV_PF)
        zxdh_pf_vq_pairs_config(dh_dev);
    else
        zxdh_pf_vf_vq_pairs_config(dh_dev);

    return 0;
}

static int create_directory(const char *path)
{
    struct path parent_path;
    struct dentry *dentry;
    int ret;
    char *last_slash;
    char parent[128];
    char name[128];
#if defined(HAVE_VFS_MKDIR)
    struct user_namespace *user_ns = current_user_ns();
#endif

    // 分离出父目录和最后一级目录名
    strscpy(parent, path, sizeof(parent));
    last_slash = strrchr(parent, '/');
    if (!last_slash) {
        return -EINVAL;
    }

    // 分割路径，获取父目录和最后一级目录
    *last_slash = '\0';
    strscpy(name, last_slash + 1, sizeof(name));

    // 获取父目录
    ret = kern_path(parent, LOOKUP_FOLLOW, &parent_path);
    DH_LOG_DEBUG(MODULE_PF, "check parent path %s, ret is %d\n", parent, ret);
    if (ret) {
        return ret;
    }

    // 加锁父目录的 inode，防止并发冲突
    inode_lock_nested(parent_path.dentry->d_inode, I_MUTEX_PARENT);

    // 获取最后一级的 dentry
    dentry = lookup_one_len(name, parent_path.dentry, zte_strlen_s(name));
    if (IS_ERR(dentry)) {
        ret = PTR_ERR(dentry);
        path_put(&parent_path);
        DH_LOG_DEBUG(MODULE_PF, "lookup_one_len error, ret is %d\n", ret);
        inode_unlock(parent_path.dentry->d_inode);
        return ret;
    }

    // 创建目录
#if defined(HAVE_VFS_MKDIR)
    ret = vfs_mkdir(user_ns, d_inode(parent_path.dentry), dentry, 0755);
#elif defined (HAVE_VFS_MKDIR_NO_IDMAP)
    ret = vfs_mkdir(&nop_mnt_idmap, d_inode(parent_path.dentry), dentry, 0755);
#else
    ret = vfs_mkdir(d_inode(parent_path.dentry), dentry, 0755);
#endif
    dput(dentry);

    // 解锁父目录的 inode
    inode_unlock(parent_path.dentry->d_inode);
    path_put(&parent_path);

    DH_LOG_DEBUG(MODULE_PF, "mkdir %s, ret is %d\n", path, ret);
    return ret;
}

int create_directory_recursion(const char *path)
{
    int ret = 0;
    char *temp_path = NULL;
    char *slash = NULL;

    // 创建临时路径缓冲区
    temp_path = kmalloc(zte_strlen_s(path) + 1, GFP_KERNEL);
    if (!temp_path) {
        return -ENOMEM;
    }

    // 从根路径开始逐级检查和创建
    ret = zte_snprintf_s(temp_path, zte_strlen_s(path)+1, "%s", path);
    if (ret < 0)
    {
        LOG_ERR("zte_snprintf_s %s failed, ret=%d\n", path, ret);
        kfree(temp_path);
        return ret;
    }
    slash = temp_path;

    // 循环切分路径
    while ((slash = strchr(slash + 1, '/')) != NULL) {
        *slash = '\0';

        // 检查当前路径是否存在，不存在则创建
        DH_LOG_DEBUG(MODULE_PF, "start create %s\n", temp_path);
        ret = create_directory(temp_path);
        if (ret && ret != -EEXIST) {
            kfree(temp_path);
            return ret;
        }

        // 恢复路径中的 '/'
        *slash = '/';
    }

    // 最后检查并创建完整路径
    ret = create_directory(temp_path);
    kfree(temp_path);
    return ret;
}

int32_t zxdh_pf_update_hb_file_val(struct dh_core_dev *dh_dev, uint64_t spec_sbdf, const char *file_name, bool flag)
{
    struct file *file = NULL;
    int32_t ret = 0;
    char dir_path[128];
    char xxx_file_path[128];
    const char *target_content = flag ? "1" : "0";
    loff_t pos = 0;

    zte_snprintf_s(dir_path, sizeof(dir_path), "/etc/dinghai/net/%llx", spec_sbdf);
    zte_snprintf_s(xxx_file_path, sizeof(xxx_file_path), "%s/%s", dir_path, file_name);

    file = filp_open(xxx_file_path, O_WRONLY | O_TRUNC, 0);
    if (IS_ERR(file)) {
        ret = PTR_ERR(file);
        if (ret == -ENOENT) {
            LOG_DEBUG_DEV(dh_dev, "File %s does not exist, attempting to create it.\n", xxx_file_path);
        } else {
            LOG_ERR_DEV(dh_dev, "Error opening file %s: %d\n", xxx_file_path, ret);
            return ret;
        }

        // Create directory if it doesn't exist
        ret = create_directory_recursion(dir_path);
        if (ret && ret != -EEXIST) {
            LOG_ERR_DEV(dh_dev, "Failed to create directory %s: %d\n", dir_path, ret);
            return ret;
        }

        // Reopen file after directory creation
        file = filp_open(xxx_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0640);
        if (IS_ERR(file)) {
            LOG_ERR_DEV(dh_dev, "Error creating file %s: %ld\n", xxx_file_path, PTR_ERR(file));
            return -1;
        }
    }

    // Write target content to the file
#ifdef CGS_V5_693
    ret = kernel_write(file, target_content, zte_strlen_s(target_content), pos);
#else
    ret = kernel_write(file, target_content, zte_strlen_s(target_content), &pos);
#endif
    if (ret < 0) {
        LOG_ERR_DEV(dh_dev, "Failed to write to file %s: %d\n", xxx_file_path, ret);
        filp_close(file, NULL);
        return ret;
    }

    filp_close(file, NULL);
    LOG_DEBUG_DEV(dh_dev, "Updated content %s to file %s\n", target_content, xxx_file_path);
    return 0;
}

int32_t zxdh_read_file_val(const char *xxx_file_path)
{
    struct file *file;
    ssize_t bytes_read;
    loff_t pos = 0; // 文件读取的起始位置
    char buffer[16] = {0}; // 初始化 buffer
    size_t buffer_size = sizeof(buffer);
    int result = -1; // 默认返回值，表示错误

    // 打开文件
    file = filp_open(xxx_file_path, O_RDONLY, 0);
    if (IS_ERR(file)) {
        LOG_ERR("open %s failed\n", xxx_file_path);
        return -1;
    }

    // 读取文件内容到 buffer 中
#ifdef CGS_V5_693
    bytes_read = kernel_read(file, pos, buffer, buffer_size - 1);
#else
    bytes_read = kernel_read(file, buffer, buffer_size - 1, &pos);
#endif

    if (bytes_read != 1) {
        LOG_ERR("read %s failed, bytes_read %zd\n", xxx_file_path, bytes_read);
        goto cleanup;
    }

    // 计算返回值
    result = (buffer[0] == '0') ? 0 : 1;
    LOG_DEBUG("%s buffer val: %s\n", xxx_file_path, buffer);

cleanup:
    filp_close(file, NULL); // 关闭文件
    return result;
}

void zxdh_hardware_bond_process(struct dh_core_dev *dh_dev) {
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    zxdh_pf_get_rp_sbdf(dh_dev);

    /* do nothing if vf */
    if (zxdh_pf_get_coredev_type(dh_dev) == DH_COREDEV_VF)
    {
        pf_dev->is_hwbond = false;
        pf_dev->is_rdma_aux_plug = true;
        pf_dev->is_primary_port = true;
        return ;
    }

    pf_dev->spec_sbdf = ((pf_dev->rp_sbdf) << 8) | (pf_dev->panel_id);
    LOG_DEBUG_DEV(dh_dev, "%s spec_sbdf: %#llx, rp_sbdf:0x%x, panel_id: %d\n", pci_name(dh_dev->pdev), pf_dev->spec_sbdf, pf_dev->rp_sbdf, pf_dev->panel_id);

    pf_dev->is_hwbond = true;
    pf_dev->is_primary_port = true;
    pf_dev->is_rdma_aux_plug = true;

    LOG_DEBUG_DEV(dh_dev, "is_hwbond %d, is_primary_port %d, is_rdma_aux_plug %d\n", pf_dev->is_hwbond, pf_dev->is_primary_port, pf_dev->is_rdma_aux_plug);

    return;
}

int32_t zxdh_panel_id_init(struct dh_core_dev *dh_dev)
{
    int32_t ret = 0;
    union zxdh_msg *msg = NULL;
    struct zxdh_bar_extra_para para = {0};
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    if (!zxdh_pf_is_panel_port(dh_dev) || zxdh_pf_is_bond(dh_dev))
    {
        return ret;
    }

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(dh_dev, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -1;
    }

    msg->payload.hdr_to_cmn.type = RISC_TYPE_READ;
    msg->payload.hdr_to_cmn.field = RISC_FIELD_PANEL_ID;
    msg->payload.hdr_to_cmn.pcie_id = pf_dev->pcie_id;
    msg->payload.hdr_to_cmn.write_bytes = 0;

    ret = zxdh_pf_msg_send_cmd(dh_dev, MODULE_TBL, msg, msg, &para);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "send_msg_to_tbl failed, ret: %d\n", ret);
        kfree(msg);
        return ret;
    }

    pf_dev->panel_id = msg->reps.cmn_recv_msg.value;
    if (pf_dev->panel_id > MAX_PANEL_ID)
    {
        LOG_ERR_DEV(dh_dev, "get panel_id failed, panel_id: %u\n", pf_dev->panel_id);
        kfree(msg);
        return -EINVAL;
    }
    LOG_DEBUG_DEV(dh_dev, "panel_id: %u\n", pf_dev->panel_id);

    kfree(msg);

    return ret;
}

/* Started by AICoder, pid:a9198z04ffb28621410c0840f03fdf234dd8cd39 */
static void zxdh_pf_dev_info_show(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    LOG_INFO_DEV(dh_dev, "***************** %s basic pf device info *****************\n", pci_name(dh_dev->pdev));
    LOG_INFO_DEV(dh_dev, "** slot_id: %d\n", pf_dev->slot_id);
    LOG_INFO_DEV(dh_dev, "** pcie_id: 0x%x\n", pf_dev->pcie_id);
    LOG_INFO_DEV(dh_dev, "** vport: 0x%x\n", pf_dev->vport)
    LOG_INFO_DEV(dh_dev, "** vfid: 0x%x\n", VQM_VFID(pf_dev->vport));
    LOG_INFO_DEV(dh_dev, "** board_type: %d\n", pf_dev->board_type);
    LOG_INFO_DEV(dh_dev, "** product_type: %d\n", pf_dev->product_type);
    LOG_INFO_DEV(dh_dev, "** panel_id: %d\n", pf_dev->panel_id);
    LOG_INFO_DEV(dh_dev, "** is_multi_ep: %d\n", pf_dev->is_multi_ep);
    LOG_INFO_DEV(dh_dev, "** is_special_bond: %d\n", pf_dev->is_special_bond);
    LOG_INFO_DEV(dh_dev, "** no_bondpf_panel: %d\n", pf_dev->no_bondpf_panel);
    LOG_INFO_DEV(dh_dev, "** qtlb_offset: %llu\n", pf_dev->qtlb_offset);
    LOG_INFO_DEV(dh_dev, "** rp_sbdf: %#x\n", pf_dev->rp_sbdf);
    LOG_INFO_DEV(dh_dev, "** spec_sbdf: %#llx\n", pf_dev->spec_sbdf);
    LOG_INFO_DEV(dh_dev, "** mcode_feature: 0x%llx\n", pf_dev->mcode_feature);
    LOG_INFO_DEV(dh_dev, "****************** fw_compat ***************\n");
    LOG_INFO_DEV(dh_dev, "** module_id: %d\n", pf_dev->fw_compat.module_id);
    LOG_INFO_DEV(dh_dev, "** major: %d\n", pf_dev->fw_compat.major);
    LOG_INFO_DEV(dh_dev, "** fw_minor: %d\n", pf_dev->fw_compat.fw_minor);
    LOG_INFO_DEV(dh_dev, "** drv_minor: %d\n", pf_dev->fw_compat.drv_minor);
    LOG_INFO_DEV(dh_dev, "** patch: %d\n", pf_dev->fw_compat.patch);
    LOG_INFO_DEV(dh_dev, "****************** hardware bond ***************\n");
    LOG_INFO_DEV(dh_dev, "** is_hwbond: %d\n", pf_dev->is_hwbond);
    LOG_INFO_DEV(dh_dev, "** is_bond_slave: %d\n", pf_dev->is_bond_slave);
    LOG_INFO_DEV(dh_dev, "** is_primary_port: %d\n", pf_dev->is_primary_port);
    LOG_INFO_DEV(dh_dev, "** is_rdma_aux_plug: %d\n", pf_dev->is_rdma_aux_plug);
    LOG_INFO_DEV(dh_dev, "****************** port resource ***************\n");
    LOG_INFO_DEV(dh_dev, "** pannel_num: %d\n", pf_dev->port_resource.pannel_num);
    LOG_INFO_DEV(dh_dev, "** bond_num: %d\n", pf_dev->port_resource.bond_num);
    LOG_INFO_DEV(dh_dev, "** bond_idx: %d\n", pf_dev->port_resource.bond_idx);
    LOG_INFO_DEV(dh_dev, "****************************************************\n");
}
/* Ended by AICoder, pid:a9198z04ffb28621410c0840f03fdf234dd8cd39 */

/* Started by AICoder, pid:c5c09o18b2c7e23141cc0bbce03baf017858015c */
static ssize_t dhinfo_sysfs_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "[%d] act_pf_dev_info, \
                [%d] act_aux_dev_info \n",
                act_pf_dev_info,
                act_aux_dev_info);
}
/* Ended by AICoder, pid:c5c09o18b2c7e23141cc0bbce03baf017858015c */

/* Started by AICoder, pid:39011q68fdgfd7914e030a2d90841e25377656f4 */
static ssize_t dhinfo_sysfs_store(struct kobject *kobj,
        struct kobj_attribute *attr, const char *buf, size_t count)
{
    struct dhinfo_sysfs *dhinfo = container_of(attr, struct dhinfo_sysfs, attr);
    struct zxdh_pf_device *pf_dev = container_of(dhinfo, struct zxdh_pf_device, dhinfo);
    struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);

    int action;
    int err = 0;

    err = kstrtoint(buf, 10, &action);
    if (err)
        return err;

    switch(action) {
        case act_pf_dev_info:
            zxdh_pf_dev_info_show(dh_dev);
            break;
        case act_aux_dev_info:
            zxdh_pf_call_aux_events(dh_dev, DH_EVENT_TYPE_AUX_INFO);
            break;
    }

    return count;
}
/* Ended by AICoder, pid:39011q68fdgfd7914e030a2d90841e25377656f4 */

void zxdh_pci_flag_set(struct dh_core_dev *dh_dev, bool power_on)
{
    struct pci_dev *dev = dh_dev->pdev;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    if ((pf_dev->board_type != DH_INICC) || (dh_dev->coredev_type != DH_COREDEV_PF)) {
        return;
    }

    if (power_on) {
        dev->dev_flags |= PCI_DEV_FLAGS_NO_FLR_RESET;
        dev->dev_flags |= PCI_DEV_FLAGS_NO_BUS_RESET;
        LOG_INFO_DEV(dh_dev, "clear pci dev flr flag\n");
    } else {
        dev->dev_flags &= ~PCI_DEV_FLAGS_NO_FLR_RESET;
        dev->dev_flags &= ~PCI_DEV_FLAGS_NO_BUS_RESET;
        LOG_INFO_DEV(dh_dev, "recover pci dev flag\n");
    }
}

/* Started by AICoder, pid:df0b7bc8d4d765b147900a7e50c8241228580e4c */
int32_t zxdh_dhinfo_sysfs_init(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct kobj_attribute *attr = &pf_dev->dhinfo.attr;
    int err = 0;

    attr->attr.name = "dev_info";
    attr->attr.mode = 0640;
    attr->show = dhinfo_sysfs_show;
    attr->store = dhinfo_sysfs_store;
    err = sysfs_create_file(&dh_dev->device->kobj, &attr->attr);
    if (err != 0) {
        LOG_ERR_DEV(dh_dev, "%s sysfs_create_file failed!\n", attr->attr.name);
        sysfs_remove_file(&dh_dev->device->kobj, &attr->attr);
    }
    return err;
}
/* Ended by AICoder, pid:df0b7bc8d4d765b147900a7e50c8241228580e4c */

/* Started by AICoder, pid:o124cl5fb3e6356149a30b3c60fdc5065ec8c125 */
void zxdh_dhinfo_sysfs_exit(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct kobj_attribute *attr = &pf_dev->dhinfo.attr;

    sysfs_remove_file(&dh_dev->device->kobj, &attr->attr);
}
/* Ended by AICoder, pid:o124cl5fb3e6356149a30b3c60fdc5065ec8c125 */

void zxdh_pf_l2d_clear(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    uint8_t read_headr_value = 0;
    uint8_t count = 0;
    uint8_t pf_count[MAX_EP] = {0};
    uint8_t ep_id = (pf_dev->pcie_id >> 12) & 0x7;
    uint8_t pf_id = (pf_dev->pcie_id >> 8) & 0x7;
    uint32_t i = 0;
    uint32_t offset = 0;

    LOG_INFO_DEV(dh_dev, "zxdh_pf_l2d_clear is called\n");

    if (dh_dev->coredev_type == DH_COREDEV_VF || 0 == pf_dev->pci_ioremap_addr[0]) {
        return;
    }

    read_headr_value = ioread8((void __iomem *)(pf_dev->pci_ioremap_addr[0] + ZXDH_MAC_FLAG_BAR_OFFSET + ZXDH_MAC_VERSION_OFFSET));
    if (read_headr_value == ZXDH_MAC_VERSION) {
        for (i = 0; i < MAX_EP; ++i) {
            read_headr_value = ioread8((void __iomem *)(pf_dev->pci_ioremap_addr[0] + ZXDH_MAC_FLAG_BAR_OFFSET + i));
            count = 0;
            while (read_headr_value) {
                read_headr_value &= (read_headr_value - 1);
                count++;
            }
            pf_count[i] = count;
        }
        offset = get_offset(ep_id, pf_id, pf_count);
        offset += ZXDH_MAC_FLAG_BAR_OFFSET + ZXDH_MAC_HEADER_BAR_OFFSET;
    } else {
        offset += ZXDH_MAC_FLAG_BAR_OFFSET + ZXDH_MAC_HEADER_OLD_OFFSET + ep_id * ZXDH_EP_FLAG_SIZE + pf_id * ZXDH_PF_FLAG_SIZE;
    }

    for (i = 0; i < MAX_VF_NUM; i++) {
        if (ioread8((void __iomem *)(pf_dev->pci_ioremap_addr[0] + offset + i)) == 1)
            iowrite8(0, (void __iomem *)(pf_dev->pci_ioremap_addr[0] + offset + i));
    }

    return;
}

static int32_t dh_pf_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    struct dh_core_dev *dh_dev = NULL;
    struct zxdh_pf_device *pf_dev = NULL;
    struct devlink *devlink = NULL;
    int32_t ret = 0;
    int32_t idx = 0;
    int32_t port_num = 0;

    LOG_INFO("pf level driver probe %s start\n", pci_name(pdev));
    #ifdef ZTE_SAFE_FUNC_TEST
    recording_not_safe_func();
    #endif

    if((GET_COREDEV_TYPE(pdev) != DH_COREDEV_PF) && (probe_vf == 0))
    {
        LOG_INFO("probe_vf is N, VF is not allowed to probe\n");
        return -1;
    }

    devlink = zxdh_devlink_alloc(&pdev->dev, &dh_pf_devlink_ops, sizeof(struct zxdh_pf_device));
    if (devlink == NULL)
    {
        LOG_ERR("devlink alloc failed\n");
        return -ENOMEM;
    }

    dh_dev = devlink_priv(devlink);
    dh_dev->device = &pdev->dev;
    dh_dev->pdev = pdev;
    dh_dev->devlink_ops = &dh_pf_core_devlink_ops;

    pf_dev = dh_core_priv(dh_dev);
    pf_dev->bar_chan_valid = false;
    pf_dev->vepa = false;
    pf_dev->plcr_table.is_init = false;
    pf_dev->plcr_table.is_xarray_init = false;
    mutex_init(&dh_dev->lock);
    mutex_init(&pf_dev->irq_lock);

    dh_dev->coredev_type = GET_COREDEV_TYPE(pdev);
    LOG_DEBUG_DEV(dh_dev, "%s device: %s\n", (dh_dev->coredev_type == DH_COREDEV_PF) ? "PF" : "VF", pci_name(pdev));

    ret = dh_pf_pci_init(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dh_pf_pci_init failed: %d\n", ret);
        goto err_irq_table_init;
    }

    handle_vf_bar_addr(dh_dev, &pf_dev->bar_info, VF_BAR_ADDR_STORE);

    ret = zxdh_pf_modern_cfg_init(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_pf_modern_cfg_init failed: %d\n", ret);
        goto err_cfg_init;
    }

    ret = dh_pf_fw_compat_check(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "The driver version and firmware version are incompatible\n");
        goto err_pci;
    }

    dh_pf_fwcap_init(dh_dev);

    ret = dh_pf_wait_riscv_ready(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "%s wait_riscv_ready time out\n", pci_name(dh_dev->pdev));
        goto err_pci;
    }

    ret = dh_pf_pcie_id_get(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dh_pf_pcie_id_get failed: %d\n", ret);
        goto err_pci;
    }

    ret = dh_pf_vf_item_create(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "Failed to alloc vf item\n");
        goto err_cfg_init;
    }

    ret = dh_pf_irq_table_init(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "Failed to alloc IRQs\n");
        goto err_vf_item;
    }

    ret = dh_pf_eq_table_init(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "Failed to alloc IRQs\n");
        goto err_eq_table_init;
    }

    ret = dh_pf_events_init(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "failed to initialize events\n");
        goto err_events_init;
    }

    ret = dh_pf_irq_table_create(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "Failed to alloc IRQs\n");
        goto err_irq_table_create;
    }

    ret = dh_pf_eq_table_create(dh_dev, true);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "Failed to alloc EQs\n");
        goto err_eq_table_create;
    }

    ret = dh_pf_sriov_cap_cfg_init(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dh_pf_sriov_cap_cfg_init failed: %d\n", ret);
        goto err_sriov_cap_init;
    }

    ret = dh_pf_bar_cfg_init(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dh_pf_bar_cfg_init failed: %d\n", ret);
        goto err_pci_bar_init;
    }

    ret = zxdh_send_pxe_status_to_riscv(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_send_pxe_status_to_riscv failed: %d\n", ret);
        goto err_send_pxe_status;
    }

#ifdef HAVE_DEVLINK_REGISTER_GET_1_PARAMS
    zxdh_devlink_register(devlink);
#else
    zxdh_devlink_register(devlink, &pdev->dev);
#endif

    ret = zxdh_vf_compat_check(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_vf_check_compat failed: %d\n", ret);
        goto err_vf_compat;
    }

    ret = zxdh_pf_dpp_init(dh_dev, true);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_pf_dpp_init failed: %d\n", ret);
        goto err_dpp_init;
    }

    ret = zxdh_pf_query_fwinfo(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_pf_query_fwinfo failed: %d\n", ret);
        goto err_query_fwinfo;
    }

    ret = zxdh_pf_vq_pairs_init(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_pf_vq_pairs_init failed: %d\n", ret);
        goto err_query_fwinfo;
    }

    ret = zxdh_pf_lag_init(dh_dev, &port_num);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_pf_lag_init failed: %d\n", ret);
        goto err_query_fwinfo;
    }

#ifdef PTP_DRIVER_INTERFACE_EN
    if (dh_dev->coredev_type == DH_COREDEV_PF)
    {
        ret = zxdh_ptp_init(dh_dev);
        if (ret != 0)
        {
            LOG_ERR_DEV(dh_dev, "zxdh_ptp_init failed: %d\n", ret);
            goto err_ptp_init;
        }
    }
#endif

#ifdef CONFIG_DINGHAI_TSN
    if (zxdh_pf_is_panel_port(dh_dev))
    {
        ret = zxdh_tsn_init(dh_dev);
        if (ret != 0)
        {
            LOG_ERR_DEV(dh_dev, "zxdh_tsn_init failed: %d\n", ret);
            goto err_tsn_init;
        }
    }
#endif

#ifndef CGS_V5_693
#ifdef ZXDH_SRIOV_SYSFS_EN
    ret = zxdh_sriov_sysfs_init(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_sriov_sysfs_init failed: %d, vport = %x\n", ret, pf_dev->vport);
        goto err_sriov_sysfs;
    }
#endif
#endif

    ret = zxdh_init_ip6mac_tbl(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_init_ip6mac_tbl failed: %d, vport = %x\n", ret, pf_dev->vport);
        goto err_init_ip6mac_tbl;
    }

    ret = zxdh_health_init(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_health_init failed: %d\n", ret);
        goto err_health_init;
    }

    ret = zxdh_dhinfo_sysfs_init(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_dhinfo_sysfs_init failed: %d\n", ret);
        goto err_dhinfo_sysfs_init;
    }
    zxdh_pci_flag_set(dh_dev, true);

    ret = zxdh_panel_id_init(dh_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_panel_id_init failed: %d\n", ret);
        goto err_panel_id_init;
    }

    zxdh_pf_l2d_clear(dh_dev);

    ret = dh_pf_adevs_table_init(dh_dev, port_num);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dh_pf_adevs_table_init failed: %d\n", ret);
        goto err_panel_id_init;
    }

    pf_dev->is_special_bond = zxdh_pf_is_special_bond(dh_dev);
    pf_dev->bond_port_type = (pf_dev->no_bondpf_panel <= 2) ? BOND_TWO_PORT_TYPE : BOND_MULTI_PORT_TYPE;
    if (!pf_dev->is_special_bond && !zxdh_pf_is_bond(dh_dev) && zxdh_pf_is_panel_port(dh_dev))
    {
        zxdh_hardware_bond_process(dh_dev);
    }

    for (idx = 0; idx < port_num; idx++)
    {
        zxdh_plug_aux_dev(dh_dev, idx);
    }

    LOG_INFO_DEV(dh_dev, "pf level driver probe %s completed\n", pci_name(pdev));

    return 0;

err_panel_id_init:
    zxdh_drain_health_wq(dh_dev);
    zxdh_dhinfo_sysfs_exit(dh_dev);
err_dhinfo_sysfs_init:
    zxdh_health_cleanup(dh_dev);
err_health_init:
    zxdh_cleanup_ip6mac_tbl(dh_dev);
err_init_ip6mac_tbl:
#ifndef CGS_V5_693
#ifdef ZXDH_SRIOV_SYSFS_EN
    zxdh_sriov_sysfs_exit(dh_dev);
err_sriov_sysfs:
#endif
#endif
#ifdef CONFIG_DINGHAI_TSN
    if (zxdh_pf_is_panel_port(dh_dev))
    {
        zxdh_tsn_exit(dh_dev);
    }
err_tsn_init:
#endif
#ifdef PTP_DRIVER_INTERFACE_EN
    if (dh_dev->coredev_type == DH_COREDEV_PF)
        zxdh_ptp_stop(dh_dev);
err_ptp_init:
#endif
    zxdh_pf_lag_exit(dh_dev);
err_query_fwinfo:
    zxdh_pf_dpp_uninit(dh_dev);
err_dpp_init:
err_vf_compat:
    zxdh_devlink_unregister(devlink);
err_send_pxe_status:
    dh_pf_bar_cfg_uninit(dh_dev);
err_pci_bar_init:
    dh_pf_sriov_cap_cfg_uninit(dh_dev);
err_sriov_cap_init:
    dh_pf_eq_table_destroy(dh_dev, true);
err_eq_table_create:
    dh_pf_irq_table_destroy(dh_dev);
err_irq_table_create:
    dh_pf_events_uninit(dh_dev);
err_events_init:
    dh_eq_table_cleanup(dh_dev);
err_eq_table_init:
    dh_irq_table_cleanup(dh_dev);
err_vf_item:
    dh_pf_vf_item_destroy(dh_dev, true);
err_pci:
    zxdh_pf_modern_cfg_uninit(dh_dev);
err_cfg_init:
    dh_pf_pci_close(dh_dev);
err_irq_table_init:
    mutex_destroy(&pf_dev->irq_lock);
    mutex_destroy(&dh_dev->lock);
    zxdh_devlink_free(devlink);
    pf_dev = NULL;
    return -EPERM;
}

int32_t zxdh_pf_vf_qpairs_init(struct dh_core_dev *dev, int32_t num_vfs);
int zxdh_load_one(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct zxdh_core_health *health = &pf_dev->health;
    struct dh_eq_table *eq_table = &dh_dev->eq_table;
    int ret = 0;

    mutex_lock(&dh_dev->lock);
    if (dh_dev->device_state == ZXDH_DEVICE_STATE_UP)
        goto unlock;

    if (dh_dev->driver_process == ZXDH_REMOVE) {
        ret = -1;
        goto unlock;
    }

    if (dh_dev->coredev_type == DH_COREDEV_VF) {
        ret = zxdh_vf_wait_pf_ok(dh_dev);
        if (ret != 0) {
            HEAL_ERR_DEV(dh_dev, "%s zxdh_vf_wait_pf_ok failed: %d\n", pci_name(dh_dev->pdev), ret);
            goto unlock;
        }
    }
    else if (dh_dev->coredev_type == DH_COREDEV_PF) {
        handle_vf_bar_addr(dh_dev, &pf_dev->bar_info, VF_BAR_ADDR_RELOAD);

        if (pf_dev->num_vfs > 0) {
            ret = zxdh_pf_vf_qpairs_init(dh_dev, (int32_t)(pf_dev->num_vfs));
            if (ret != 0) {
                HEAL_ERR_DEV(dh_dev, "%s Failed to recover vf queue pairs\n", pci_name(dh_dev->pdev));
                goto unlock;
            }
        } else {
            HEAL_INFO_DEV(dh_dev, "%s clear_pf_sriov_status\n", pci_name(dh_dev->pdev));
            clear_pf_sriov_status(dh_dev);
        }

        set_pci_vpd_len_to_max(dh_dev, zxdh_pf_vpd_max_cfg_size(dh_dev));
    }

    ret = dh_pf_irq_table_create(dh_dev);
    if (ret != 0) {
        HEAL_ERR_DEV(dh_dev, "%s Failed to alloc IRQs\n", pci_name(dh_dev->pdev));
        goto unlock;
    }

    /* 在此之前，bar通道还未恢复，pf_dev->bar_chan_valid还没有被重新置上，再次之前不要发送bar消息*/
    ret = dh_pf_eq_table_create(dh_dev, false);
    if (ret != 0) {
        HEAL_ERR_DEV(dh_dev, "%s Failed to alloc EQs\n", pci_name(dh_dev->pdev));
        goto irq_table_destroy;
    }

    ret = zxdh_pf_dpp_reset(dh_dev);
    if (ret != 0) {
        HEAL_ERR_DEV(dh_dev, "%s zxdh_pf_dpp_reset failed: %d\n", pci_name(dh_dev->pdev), ret);
        goto eq_table_destroy;
    }

    ret = zxdh_pf_dpp_init(dh_dev, false);
    if (ret != 0){
        HEAL_ERR_DEV(dh_dev, "%s zxdh_pf_dpp_init failed: %d\n", pci_name(dh_dev->pdev), ret);
        goto eq_table_destroy;
    }

    atomic_notifier_call_chain(&eq_table->nh[DH_EVENT_TYPE_AUX_LOAD], DH_EVENT_TYPE_AUX_LOAD, &ret);
    if (ret != 0) {
        HEAL_ERR_DEV(dh_dev, "%s DH_EVENT_TYPE_AUX_LOAD failed: %d\n", pci_name(dh_dev->pdev), ret);
        goto dpp_uninit;
    }

    pf_dev->fast_unload = false;
    dh_dev->device_state = ZXDH_DEVICE_STATE_UP;
    health->recovery_cnt++;
    HEAL_INFO_DEV(dh_dev, "%s zxdh_load_one success\n", pci_name(dh_dev->pdev));

    if (dh_dev->coredev_type == DH_COREDEV_PF) {
        zxdh_events_work_enqueue(dh_dev, &pf_dev->rdma_dev_proc_work);
        zxdh_pf_status_ok(dh_dev);
    } else {
        zxdh_events_work_enqueue(dh_dev, &pf_dev->rdma_dev_proc_work);
    }
    mutex_unlock(&dh_dev->lock);

    return 0;

dpp_uninit:
    zxdh_pf_call_aux_events(dh_dev, DH_EVENT_TYPE_AUX_UNLOAD);
    zxdh_pf_dpp_uninit(dh_dev);
eq_table_destroy:
    dh_pf_eq_table_destroy(dh_dev, false);
irq_table_destroy:
    dh_pf_irq_table_destroy(dh_dev);
unlock:
    mutex_unlock(&dh_dev->lock);
    return ret;
}

static void zxdh_reset_all_vf_item(struct dh_core_dev *dh_dev)
{
    struct zxdh_vf_item *vf_item = NULL;
    uint16_t num_vfs = 0;
    uint16_t vf_idx = 0;

    if (dh_dev->coredev_type == DH_COREDEV_VF)
        return;

    num_vfs = pci_num_vf(dh_dev->pdev);
    for (vf_idx = 0; vf_idx < num_vfs; vf_idx++)
    {
        vf_item = zxdh_pf_get_vf_item(dh_dev, vf_idx);
        vf_item->is_probed = false;
    }
}

void zxdh_unload_one(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    HEAL_INFO_DEV(dh_dev, "%s zxdh_unload_one start\n", pci_name(dh_dev->pdev));
    mutex_lock(&dh_dev->lock);
    if (dh_dev->driver_process == ZXDH_REMOVE) {
        mutex_unlock(&dh_dev->lock);
        return;
    }
    pf_dev->fast_unload = true;
    pf_dev->aux_comp_flag = 0;
    zxdh_pf_call_aux_events(dh_dev, DH_EVENT_TYPE_AUX_UNLOAD);
    dh_pf_eq_table_destroy(dh_dev, false);
    dh_pf_irq_table_destroy(dh_dev);
    zxdh_reset_all_vf_item(dh_dev);
    mutex_unlock(&dh_dev->lock);
}

/* Started by AICoder, pid:ge41e1a12dme87514ee409fcf0b7eb2c1fc938d3 */
/* if hot-swap is supported, the refcnt is 0, and the slot information is deleted. */
static void zxdh_remove_slot_info(struct zxdh_pf_device *pf_dev)
{
    uint16_t i = 0;

    for (i = 1; i < DPP_PCIE_SLOT_MAX; i++)
    {
        if (memcmp(pf_dev->sn_code, dh_slot[i].sn_code, SN_CODE_LENGTH) == 0) {
            dh_slot[i].refcnt--;
            if (dh_slot[i].refcnt == 0) {
                dh_slot[i].pf_info.slot = 0;
                dh_slot[i].pf_info.vport = 0;
                dh_slot[i].board_type = 0;
                zte_memset_s(dh_slot[i].sn_code, 0, SN_CODE_LENGTH);
            }
            break;
        }
    }
}
/* Ended by AICoder, pid:ge41e1a12dme87514ee409fcf0b7eb2c1fc938d3 */

static void dh_pf_remove(struct pci_dev *pdev)
{
    struct dh_core_dev *dh_dev = pci_get_drvdata(pdev);
    struct devlink *devlink = priv_to_devlink(dh_dev);
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    if (pf_dev == NULL)
    {
        return;
    }
    LOG_INFO_DEV(dh_dev, "pf level driver remove %s start\n", pci_name(pdev));
    if (!zxdh_pf_check_remove_state(dh_dev)) {
        pf_dev->quick_remove = true;
        LOG_INFO_DEV(dh_dev, "%s: quick_remove start\n", pci_name(pdev));
    }

    mutex_lock(&dh_dev->lock);
    zxdh_remove_slot_info(pf_dev);
    dh_dev->driver_process = ZXDH_REMOVE;
    mutex_unlock(&dh_dev->lock);

    zxdh_drain_health_wq(dh_dev);
    zxdh_health_cleanup(dh_dev);

    zxdh_unplug_aux_dev_all(dh_dev);
    dh_pf_adevs_table_destroy(dh_dev);

    zxdh_dhinfo_sysfs_exit(dh_dev);
    zxdh_pci_flag_set(dh_dev, false);

    zxdh_cleanup_ip6mac_tbl(dh_dev);
#ifndef CGS_V5_693
#ifdef ZXDH_SRIOV_SYSFS_EN
    zxdh_sriov_sysfs_exit(dh_dev);
#endif
#endif
#ifdef CONFIG_DINGHAI_TSN
    if (zxdh_pf_is_panel_port(dh_dev))
    {
        zxdh_tsn_exit(dh_dev);
    }
#endif
#ifdef PTP_DRIVER_INTERFACE_EN
    if (dh_dev->coredev_type == DH_COREDEV_PF)
    {
        zxdh_ptp_stop(dh_dev);
    }
#endif
    zxdh_pf_vf_qpairs_uninit(dh_dev);

    zxdh_pf_lag_exit(dh_dev);
    zxdh_pf_dpp_uninit(dh_dev);

    zxdh_devlink_unregister(devlink);
    dh_pf_bar_cfg_uninit(dh_dev);
    dh_pf_sriov_cap_cfg_uninit(dh_dev);
    if (!pf_dev->fast_unload) {
        dh_pf_eq_table_destroy(dh_dev, true);
        dh_pf_irq_table_destroy(dh_dev);
    }
    dh_pf_events_uninit(dh_dev);
    dh_eq_table_cleanup(dh_dev);
    dh_irq_table_cleanup(dh_dev);
    dh_pf_vf_item_destroy(dh_dev, true);
    zxdh_pf_modern_cfg_uninit(dh_dev);
    dh_pf_pci_close(dh_dev);
    mutex_destroy(&pf_dev->irq_lock);
    mutex_destroy(&dh_dev->lock);
    zxdh_devlink_free(devlink);

    pci_set_drvdata(pdev, NULL);
    LOG_INFO("pf level driver remove %s completed\n", pci_name(pdev));

    return;
}

static int32_t dh_pf_suspend(struct pci_dev *pdev, pm_message_t state)
{
    return 0;
}

static int32_t dh_pf_resume(struct pci_dev *pdev)
{
    return 0;
}

static void dh_pf_shutdown(struct pci_dev *pdev)
{
    struct dh_core_dev *dh_dev = pci_get_drvdata(pdev);
    struct devlink *devlink = priv_to_devlink(dh_dev);
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    LOG_INFO_DEV(dh_dev, "pf level driver shutdown %s start\n", pci_name(pdev));
    mutex_lock(&dh_dev->lock);
    dh_dev->driver_process = ZXDH_REMOVE;
    mutex_unlock(&dh_dev->lock);

    dh_pf_adevs_table_destroy(dh_dev);

    zxdh_dhinfo_sysfs_exit(dh_dev);

    zxdh_drain_health_wq(dh_dev);
    zxdh_health_cleanup(dh_dev);

    zxdh_cleanup_ip6mac_tbl(dh_dev);
#ifndef CGS_V5_693
#ifdef ZXDH_SRIOV_SYSFS_EN
    zxdh_sriov_sysfs_exit(dh_dev);
#endif
#endif
#ifdef CONFIG_DINGHAI_TSN
    if (!zxdh_pf_is_upf(dh_dev))
    {
        zxdh_tsn_exit(dh_dev);
    }
#endif
#ifdef PTP_DRIVER_INTERFACE_EN
    if (dh_dev->coredev_type == DH_COREDEV_PF)
    {
        zxdh_ptp_stop(dh_dev);
    }
#endif

    zxdh_pf_lag_exit(dh_dev);
    if (!pf_dev->fast_unload)
        zxdh_pf_dpp_uninit(dh_dev);

    zxdh_devlink_unregister(devlink);
    dh_pf_bar_cfg_uninit(dh_dev);
    dh_pf_sriov_cap_cfg_uninit(dh_dev);
    if (!pf_dev->fast_unload) {
        dh_pf_eq_table_destroy(dh_dev, true);
        dh_pf_irq_table_destroy(dh_dev);
    }
    dh_pf_events_uninit(dh_dev);
    dh_eq_table_cleanup(dh_dev);
    dh_irq_table_cleanup(dh_dev);
    dh_pf_vf_item_destroy(dh_dev, false);
    zxdh_pf_modern_cfg_uninit(dh_dev);

    dh_pf_pci_close(dh_dev);
    mutex_destroy(&pf_dev->irq_lock);
    mutex_destroy(&dh_dev->lock);
    zxdh_devlink_free(devlink);

    pci_set_drvdata(pdev, NULL);
    LOG_INFO("pf level driver shutdown %s completed\n", pci_name(pdev));
}

static pci_ers_result_t dh_pci_err_detected(struct pci_dev *pdev,
                                            pci_channel_state_t state)
{
    // struct dh_core_dev *dev = pci_get_drvdata(pdev);
    // struct zxdh_pf_device *pf_dev = dh_core_priv(dev);
    // bool err_detected = false;

    // LOG_INFO("PCI error detected\n");
    // if (zxdh_health_check_fatal_sensors(&pf_dev->health) &&
    //     dev->device_state == ZXDH_DEVICE_STATE_UP) {
    //     dev->device_state = ZXDH_DEVICE_STATE_INTERNAL_ERROR;
    //     err_detected = true;
    // }
    // mutex_lock(&dev->lock);
    // if (!err_detected && dev->device_state == ZXDH_DEVICE_STATE_INTERNAL_ERROR)
    // if (dev->device_state == ZXDH_DEVICE_STATE_UNINITIALIZED) {
    //     dev->device_state = ZXDH_DEVICE_STATE_INTERNAL_ERROR;
    // }
    // mutex_unlock(&dev->lock);

    // zxdh_drain_health_wq(dev);
    // pci_disable_device(pdev);
    return state == pci_channel_io_perm_failure ? PCI_ERS_RESULT_DISCONNECT : PCI_ERS_RESULT_NEED_RESET;
}

static pci_ers_result_t dh_pf_pci_slot_reset(struct pci_dev *pdev)
{
    // struct dh_core_dev *dev = pci_get_drvdata(pdev);

    // LOG_INFO("start PCI slot reset\n");
    // if (pci_enable_device(pdev) != 0) {
    //     LOG_ERR("pci_enable_device failed\n");
    //     return PCI_ERS_RESULT_DISCONNECT;
    // }
    // pci_set_master(pdev);
    // pci_restore_state(pdev);
    // pci_save_state(pdev);

    // if (wait_vital(dev)) {
    //     HEAL_ERR("%s wait_vital time out\n", pci_name(pdev));
    //     return PCI_ERS_RESULT_DISCONNECT;
    // }

    // if (zxdh_health_wait_dh_ok(dev)) {
    //     HEAL_ERR("%s zxdh_health_wait_dh_ok time out\n", pci_name(pdev));
    //     return PCI_ERS_RESULT_DISCONNECT;
    // }

    return PCI_ERS_RESULT_RECOVERED;
}

static void dh_pf_pci_resume(struct pci_dev *pdev)
{
    // struct dh_core_dev *dev = pci_get_drvdata(pdev);

    // LOG_INFO("start PCI resume\n");
    // zxdh_unload_one(dev);
    // HEAL_INFO("%s zxdh_unload_one finish\n", pci_name(pdev));
    // if (zxdh_load_one(dev)) {
    //     HEAL_ERR("%s zxdh_load_one failed\n", pci_name(pdev));
    // }
}

int32_t zxdh_user_vf_qpairs_update(struct dh_core_dev *dev, uint8_t vf_qp, uint16_t pf_idx)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);

    iowrite8(vf_qp, (void __iomem*)(pf_dev->pci_ioremap_addr[0] + ZXDH_VF_QUEUE_USER_OFFSET + pf_idx));
    return 0;
}

int32_t zxdh_pf_vf_qpairs_update(struct dh_core_dev *dev, uint8_t vf_qp, int32_t num_vfs)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);
    uint8_t power = 0;
    int32_t ret = 0;
    uint16_t vport = 0;
    uint16_t vf_idx = 0;
    uint16_t ep_id = 0;
    int32_t i = 0;
    uint8_t *addr = NULL;
    uint8_t val = 0;

    if (vf_qp == 0)
        return -1;

    /* 求不超过vf_qp的最大2次幂数的指数 */
    while ((1U << power) <= vf_qp)
    {
        power++;
    }
    power--;
    vf_qp = 1 << power;
    LOG_DEBUG_DEV(dev, "pf(vport:0x%x) setup vf queue pairs:%u, power:%u\n", pf_dev->vport, vf_qp, power);

    ret = dh_pf_vf_vport_get(dev, 0, &vport);
    if (ret != 0)
    {
        LOG_ERR_DEV(dev, "Failed to pf(vport:0x%x) get vf0 vport\n", pf_dev->vport);
        return ret;
    }

    /* vport bit[12:14] ep_id */
    ep_id = EPID_GEN_FROM_VPORT(pf_dev->vport);
    if (ep_id >= ZXDH_EP_NUM)
    {
        LOG_ERR_DEV(dev, "vf vport is err, ep_id:%u\n", ep_id);
        return -1;
    }

    for (i = 0; i < num_vfs; i++)
    {
        vf_idx = GLOBAL_VF_IDX(ep_id, vport) + i;
        addr = (uint8_t*)pf_dev->pci_ioremap_addr[0] + ZXDH_VF_QUEUE_PAIRS_OFFSET + (vf_idx / 2);
        val = ioread8((void __iomem*)addr);
        if (vf_idx % 2)
        { /* 将队列的次幂更新到高4位 */
            val = (val & 0xf) | (power << 4);
        }
        else
        { /* 将队列的次幂更新到低4位 */
            val = (val & 0xf0) | power;
        }
        iowrite8(val, (void __iomem*)addr);
    }

    return 0;
}

int32_t zxdh_pf_vf_qpairs_init(struct dh_core_dev *dev, int32_t num_vfs)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);
    struct zxdh_dev_queue_info *dev_qinfo = NULL;
    struct zxdh_pf_queue_info *pf_qinfo = NULL;
    struct zxdh_fw_compat fw_compat = pf_dev->fw_compat;
    uint16_t ep_id = 0;
    uint16_t pf_idx = 0;
    uint16_t vf_qp_flx = 0;
    uint8_t vf_qp_user_max = 0;
    uint8_t vf_qp = 0;
    int32_t ret = 0;

    if ((!zxdh_pf_is_nic(dev)) || (fw_compat.patch < 1))
        return 0;

/* 更新内核态vf分配队列数信息 */
    ep_id = EPID_GEN_FROM_VPORT(pf_dev->vport);
    pf_idx = GLOBAL_PF_IDX(ep_id, pf_dev->vport);

    dev_qinfo = (struct zxdh_dev_queue_info *)(pf_dev->pci_ioremap_addr[0] + ZXDH_DEV_QUEUE_INFO_OFFSET + pf_idx * 4);
    pf_qinfo = (struct zxdh_pf_queue_info *)(pf_dev->pci_ioremap_addr[0] + ZXDH_PF_QUEUE_INFO_OFFSET);
    LOG_DEBUG_DEV(dev, "pf(vport:0x%x) get queue config: total_qp:%u, start_qp_id:%u, pf_qp:%u, vf_qp:%u\n",
               pf_dev->vport, dev_qinfo->total_qp, dev_qinfo->start_id, pf_qinfo->pf_qp, pf_qinfo->vf_qp);

    vf_qp_flx = (dev_qinfo->total_qp - pf_dev->vq_pairs) / num_vfs;
    vf_qp = (pf_qinfo->vf_qp < vf_qp_flx) ? pf_qinfo->vf_qp : vf_qp_flx;
    ret = zxdh_pf_vf_qpairs_update(dev, vf_qp, num_vfs);
    if (ret != 0)
    {
        LOG_DEBUG_DEV(dev, "Failed to pf(vport:0x%x) setup vf queue pairs to %u\n", pf_dev->vport, vf_qp);
        return ret;
    }

/* 更新用户态vf分配队列数信息 */
    vf_qp_user_max = ioread8((void __iomem*)(pf_dev->pci_ioremap_addr[0] + ZXDH_VF_MAX_QUEUE_USER_OFFSET));
    vf_qp = (vf_qp_user_max < vf_qp_flx)? vf_qp_user_max : vf_qp_flx;

    zxdh_user_vf_qpairs_update(dev, vf_qp, pf_idx);

    return 0;
}

int32_t dh_pf_sriov_configure(struct pci_dev *pdev, int32_t num_vfs)
{
    struct dh_core_dev *dev = pci_get_drvdata(pdev);
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);
    struct zxdh_rdma_sriov_event_info rdma_sriov_info = {0};

    if (dev->coredev_type != DH_COREDEV_PF)
    {
        LOG_ERR_DEV(dev, "This device is not capable of SR-IOV\n");
        return -EOPNOTSUPP;
    }

    if (!pf_dev->pf_sriov_cap_base)
    {
        LOG_ERR_DEV(dev, "sriov not enable\n");
        return -EOPNOTSUPP;
    }

    if (num_vfs > 0)
    {
        if (dev->device_state == ZXDH_DEVICE_STATE_INTERNAL_ERROR)
        {
            LOG_ERR_DEV(dev, "device_state error\n");
            return -EOPNOTSUPP;
        }

        if (zxdh_pf_vf_qpairs_init(dev, num_vfs) != 0)
        {
            LOG_ERR_DEV(dev, "Failed to init vf queue pairs\n");
            return -1;
        }

        if (zxdh_pf_is_rdma_enable(dev))
        {
            rdma_sriov_info.pdev = pdev;
            rdma_sriov_info.bar0_virt_addr = pf_dev->pci_ioremap_addr[0];
            rdma_sriov_info.vport_id = pf_dev->vport;
            rdma_sriov_info.num_vfs = num_vfs;
            zxdh_rdma_events_call(NULL, ZXDH_RDMA_SRIOV_EVENT, &rdma_sriov_info);
        }

        if (dh_pf_sriov_enable(pdev, num_vfs) != 0)
        {
            LOG_ERR_DEV(dev, "Failed to enable sriov, num_vfs:%d\n", num_vfs);
            return -1;
        }
    }
    else
    {
        dh_pf_sriov_disable(pdev);
    }

    if (zxdh_pf_pcie_config_store(dev))
        LOG_ERR_DEV(dev, "zxdh_pf_pcie_config_store failed\n");

    pf_dev->num_vfs = (uint16_t)num_vfs;

    return num_vfs;
}

static const struct pci_error_handlers dh_pf_err_handler = {
    .error_detected = dh_pci_err_detected,
    .slot_reset     = dh_pf_pci_slot_reset,
    .resume         = dh_pf_pci_resume
};

static struct pci_driver dh_pf_driver = {
    .name            = KBUILD_MODNAME,
    .id_table        = dh_pf_pci_table,
    .probe           = dh_pf_probe,
    .remove          = dh_pf_remove,
    .suspend         = dh_pf_suspend,
    .resume          = dh_pf_resume,
    .shutdown        = dh_pf_shutdown,
    .err_handler     = &dh_pf_err_handler,
    .sriov_configure = dh_pf_sriov_configure,
};

static int32_t __init dh_pf_pci_init_module(void)
{
    int32_t ret = 0;
    int dhtool_char_init_ret = 0;

    LOG_INFO("%s - version %s %s\n", zxdh_pf_driver_string, zxdh_pf_driver_version, zxdh_pf_copyright);

    zxdh_log_dir_init();

    memset(dh_slot, 0, sizeof(dh_slot));

#ifdef NEED_XARRAY
    dh_radix_tree_init();
#endif

    ret = pci_register_driver(&dh_pf_driver);
    if (ret != 0)
    {
        LOG_ERR("pci_register_driver failed: %d\n", ret);
        goto err_register_driver;
    }

    ret = dh_pf_msg_recv_func_register();
    if (ret != 0)
    {
        LOG_ERR("dh_pf_msg_recv_func_register failed: %d\n", ret);
        goto err_msg_recv_func_registe;
    }

#ifdef CONFIG_ZXDH_SF
    ret = zxdh_en_sf_driver_register();
    if (ret != 0)
    {
        LOG_ERR("zxdh_en_sf_driver_register failed: %d\n", ret);
        goto err_sf_driver_register;
    }
#endif
    // do not let driver load failed if dhtool char device create failed because char device is low priotity
    // the failed process is in the function dhtool_char_init inner
    LOG_DEBUG("[Dhtool Char Device Called] Before char_init\n");
    dhtool_char_init_ret = dhtool_char_init();
    LOG_DEBUG(
        "[Dhtool Char Device Called] After char_init, %s, ret: %d\n",
        dhtool_char_init_ret == 0 ? "success":"failed", dhtool_char_init_ret);

    return 0;

#ifdef CONFIG_ZXDH_SF
err_sf_driver_register:
    dh_pf_msg_recv_func_unregister();
#endif
err_msg_recv_func_registe:
    pci_unregister_driver(&dh_pf_driver);
err_register_driver:
    return ret;
}

static void dh_pf_pci_exit_module(void)
{
    LOG_INFO("%s - version %s %s\n", zxdh_pf_driver_string, zxdh_pf_driver_version, zxdh_pf_copyright);

    LOG_DEBUG("[Dhtool Char Device Called] Before char_exit\n");
    dhtool_char_exit();
    LOG_DEBUG("[Dhtool Char Device Called] After char_exit\n");

    pci_unregister_driver(&dh_pf_driver);

#ifdef CONFIG_ZXDH_SF
    zxdh_en_sf_driver_unregister();
#endif
    dh_pf_msg_recv_func_unregister();
    return;
}

module_init(dh_pf_pci_init_module);
module_exit(dh_pf_pci_exit_module);
