#ifndef DINGHAI_DEVICE_H
#define DINGHAI_DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/types.h>

/* helper macros */
#define __dh_nullp(typ) ((struct dh_ifc_##typ##_bits *)0)
#define __dh_bit_sz(typ, fld) sizeof(__dh_nullp(typ)->fld)
#define __dh_bit_off(typ, fld) (offsetof(struct dh_ifc_##typ##_bits, fld))
#define __dh_16_off(typ, fld) (__dh_bit_off(typ, fld) / 16)
#define __dh_dw_off(typ, fld) (__dh_bit_off(typ, fld) / 32)
#define __dh_64_off(typ, fld) (__dh_bit_off(typ, fld) / 64)
#define __dh_16_bit_off(typ, fld) (16 - __dh_bit_sz(typ, fld) - (__dh_bit_off(typ, fld) & 0xf))
#define __dh_dw_bit_off(typ, fld) (32 - __dh_bit_sz(typ, fld) - (__dh_bit_off(typ, fld) & 0x1f))
#define __dh_mask(typ, fld) ((u32)((1ull << __dh_bit_sz(typ, fld)) - 1))
#define __dh_dw_mask(typ, fld) (__dh_mask(typ, fld) << __dh_dw_bit_off(typ, fld))
#define __dh_mask16(typ, fld) ((u16)((1ull << __dh_bit_sz(typ, fld)) - 1))
#define __dh_16_mask(typ, fld) (__dh_mask16(typ, fld) << __dh_16_bit_off(typ, fld))
#define __dh_st_sz_bits(typ) sizeof(struct dh_ifc_##typ##_bits)

#define DH_FLD_SZ_BYTES(typ, fld) (__dh_bit_sz(typ, fld) / 8)
#define DH_ST_SZ_BYTES(typ) (sizeof(struct dh_ifc_##typ##_bits) / 8)
#define DH_ST_SZ_DW(typ) (sizeof(struct dh_ifc_##typ##_bits) / 32)
#define DH_ST_SZ_QW(typ) (sizeof(struct dh_ifc_##typ##_bits) / 64)
#define DH_UN_SZ_BYTES(typ) (sizeof(union dh_ifc_##typ##_bits) / 8)
#define DH_UN_SZ_DW(typ) (sizeof(union dh_ifc_##typ##_bits) / 32)
#define DH_BYTE_OFF(typ, fld) (__dh_bit_off(typ, fld) / 8)
#define DH_ADDR_OF(typ, p, fld) ((void *)((uint8_t *)(p) + DH_BYTE_OFF(typ, fld)))

/* insert a value to a struct */
#define DH_SET(typ, p, fld, v) do { \
    u32 _v = v; \
    BUILD_BUG_ON(__dh_st_sz_bits(typ) % 32);             \
    *((__be32 *)(p) + __dh_dw_off(typ, fld)) = \
    cpu_to_be32((be32_to_cpu(*((__be32 *)(p) + __dh_dw_off(typ, fld))) & \
            (~__dh_dw_mask(typ, fld))) | (((_v) & __dh_mask(typ, fld)) \
            << __dh_dw_bit_off(typ, fld))); \
} while (0)

#define DH_ARRAY_SET(typ, p, fld, idx, v) do { \
    BUILD_BUG_ON(__dh_bit_off(typ, fld) % 32); \
    DH_SET(typ, p, fld[idx], v); \
} while (0)

#define DH_SET_TO_ONES(typ, p, fld) do { \
    BUILD_BUG_ON(__dh_st_sz_bits(typ) % 32);             \
    *((__be32 *)(p) + __dh_dw_off(typ, fld)) = \
    cpu_to_be32((be32_to_cpu(*((__be32 *)(p) + __dh_dw_off(typ, fld))) & \
            (~__dh_dw_mask(typ, fld))) | ((__dh_mask(typ, fld)) \
            << __dh_dw_bit_off(typ, fld))); \
} while (0)

#define DH_GET(typ, p, fld) ((be32_to_cpu(*((__be32 *)(p) + \
__dh_dw_off(typ, fld))) >> __dh_dw_bit_off(typ, fld)) & \
__dh_mask(typ, fld))

#define DH_GET_PR(typ, p, fld) ({ \
    u32 ___t = DH_GET(typ, p, fld); \
    pr_debug(#fld " = 0x%x\n", ___t); \
    ___t; \
})

#define __DH_SET64(typ, p, fld, v) do { \
    BUILD_BUG_ON(__dh_bit_sz(typ, fld) != 64); \
    *((__be64 *)(p) + __dh_64_off(typ, fld)) = cpu_to_be64(v); \
} while (0)

#define DH_SET64(typ, p, fld, v) do { \
    BUILD_BUG_ON(__dh_bit_off(typ, fld) % 64); \
    __DH_SET64(typ, p, fld, v); \
} while (0)

#define DH_ARRAY_SET64(typ, p, fld, idx, v) do { \
    BUILD_BUG_ON(__dh_bit_off(typ, fld) % 64); \
    __DH_SET64(typ, p, fld[idx], v); \
} while (0)

#define DH_GET64(typ, p, fld) be64_to_cpu(*((__be64 *)(p) + __dh_64_off(typ, fld)))

#define DH_GET64_PR(typ, p, fld) ({ \
    u64 ___t = DH_GET64(typ, p, fld); \
    pr_debug(#fld " = 0x%llx\n", ___t); \
    ___t; \
})

#define DH_GET16(typ, p, fld) ((be16_to_cpu(*((__be16 *)(p) + \
__dh_16_off(typ, fld))) >> __dh_16_bit_off(typ, fld)) & \
__dh_mask16(typ, fld))

#define DH_SET16(typ, p, fld, v) do { \
    u16 _v = v; \
    BUILD_BUG_ON(__dh_st_sz_bits(typ) % 16);  \
    *((__be16 *)(p) + __dh_16_off(typ, fld)) = \
    cpu_to_be16((be16_to_cpu(*((__be16 *)(p) + __dh_16_off(typ, fld))) & \
            (~__dh_16_mask(typ, fld))) | (((_v) & __dh_mask16(typ, fld)) \
            << __dh_16_bit_off(typ, fld))); \
} while (0)

/* Big endian getters */
#define DH_GET64_BE(typ, p, fld) (*((__be64 *)(p) + \
    __dh_64_off(typ, fld)))

#define DH_GET_BE(type_t, typ, p, fld) ({                           \
        type_t tmp;                                                 \
        switch (sizeof(tmp)) {                                      \
        case sizeof(u8):                                            \
            tmp = (__force type_t)DH_GET(typ, p, fld);              \
            break;                                                  \
        case sizeof(u16):                                           \
            tmp = (__force type_t)cpu_to_be16(DH_GET(typ, p, fld)); \
            break;                                                  \
        case sizeof(u32):                                           \
            tmp = (__force type_t)cpu_to_be32(DH_GET(typ, p, fld)); \
            break;                                                  \
        case sizeof(u64):                                           \
            tmp = (__force type_t)DH_GET64_BE(typ, p, fld);         \
            break;                                                  \
            }                                                       \
        tmp;                                                        \
        })

enum dh_cap_type {
    DH_CAP_GENERAL = 0,
};
/* GET Dev Caps macros */
#define DH_CAP_GEN(mdev, cap) \
    DH_GET(cmd_hca_cap, mdev->caps.hca[DH_CAP_GENERAL]->cur, cap)

#define DH_CAP_GEN_64(mdev, cap) \
    DH_GET64(cmd_hca_cap, mdev->caps.hca[DH_CAP_GENERAL]->cur, cap)

#define DH_CAP_GEN_MAX(mdev, cap) \
    DH_GET(cmd_hca_cap, mdev->caps.hca[DH_CAP_GENERAL]->max, cap)




enum dh_event_queue_type {
    DH_EVENT_QUEUE_TYPE_SAMPLE,
    DH_EVENT_QUEUE_TYPE_RISCV
};

struct dh_mpf_priv {

};

enum dh_health_status {
    IDLE,
    HANDLING,
    PENDING,
    DONE
};

#define FW_VERISON_SIZE      (32)
struct health_buffer {
    uint32_t synd;
    uint32_t health_counter;
    uint8_t status;
    uint8_t rfr;
    uint8_t fw_exception;
    uint8_t riscv_power_on;
    uint8_t fw_version[FW_VERISON_SIZE];
    uint8_t pf_status[5];
    uint8_t health_version;
    uint8_t rsv1[30];
};

struct core_health {
    struct health_buffer __iomem *hb;
    uint32_t prev;
    uint32_t miss_counter;
    uint32_t synd;
};

#ifdef __cplusplus
}
#endif

#endif
