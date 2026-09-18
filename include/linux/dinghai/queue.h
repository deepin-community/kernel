#ifndef __DINGHAI_QUEUE_H__
#define __DINGHAI_QUEUE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Vector value used to disable MSI for queue */
#define ZXDH_MSI_NO_VECTOR      0xff

/* Status byte for guest to report progress, and synchronize features */
/* We have seen device and processed generic fields */
#define ZXDH_CONFIG_S_ACKNOWLEDGE 1
/* We have found a driver for the device. */
#define ZXDH_CONFIG_S_DRIVER      2
/* Driver has used its parts of the config, and is happy */
#define ZXDH_CONFIG_S_DRIVER_OK   4
/* Driver has finished configuring features */
#define ZXDH_CONFIG_S_FEATURES_OK 8
/* Device entered invalid state, driver must reset it */
#define ZXDH_CONFIG_S_NEEDS_RESET 0x40
/* We've given up on this device */
#define ZXDH_CONFIG_S_FAILED      0x80


/* This is the PCI capability header: */
struct zxdh_pf_pci_cap
{
    __u8 cap_vndr;        /* Generic PCI field: PCI_CAP_ID_VNDR */
    __u8 cap_next;        /* Generic PCI field: next ptr. */
    __u8 cap_len;         /* Generic PCI field: capability length */
    __u8 cfg_type;        /* Identifies the structure. */
    __u8 bar;             /* Where to find it. */
    __u8 id;              /* Multiple capabilities of the same type */
    __u8 padding[2];      /* Pad to full dword. */
    __le32 offset;        /* Offset within bar. */
    __le32 length;        /* Length of the structure, in bytes. */
};

/* Fields in ZXDH_PF_PCI_CAP_COMMON_CFG: */
struct zxdh_pf_pci_common_cfg
{
    /* About the whole device. */
    __le32 device_feature_select; /* read-write */
    __le32 device_feature;        /* read-only */
    __le32 guest_feature_select;  /* read-write */
    __le32 guest_feature;         /* read-write */
    __le16 msix_config;           /* read-write */
    __le16 num_queues;            /* read-only */
    __u8 device_status;           /* read-write */
    __u8 config_generation;       /* read-only */

    /* About a specific virtqueue. */
    __le16 queue_select;          /* read-write */
    __le16 queue_size;            /* read-write, power of 2. */
    __le16 queue_msix_vector;     /* read-write */
    __le16 queue_enable;          /* read-write */
    __le16 queue_notify_off;      /* read-only */
    __le32 queue_desc_lo;         /* read-write */
    __le32 queue_desc_hi;         /* read-write */
    __le32 queue_avail_lo;        /* read-write */
    __le32 queue_avail_hi;        /* read-write */
    __le32 queue_used_lo;         /* read-write */
    __le32 queue_used_hi;         /* read-write */
};

struct zxdh_pf_pci_notify_cap
{
    struct zxdh_pf_pci_cap cap;
    __le32 notify_off_multiplier; /* Multiplier for queue_notify_off. */
};


#ifdef __cplusplus
}
#endif

#endif
