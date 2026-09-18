#ifndef __DEVLINK_COMPAT_H__
#define __DEVLINK_COMPAT_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Compatibility definitions for devlink parameters
 * For CGSL V5 SP693 (RHEL 7.4 / 3.10.0-693) kernel which lacks these definitions
 */

#ifdef CGS_V5_693

/*
 * For CGSL V5 693 kernel, devlink parameter related types are not defined.
 * Define them here for compatibility.
 */

/* Maximum string value for devlink parameters */
#define __DEVLINK_PARAM_MAX_STRING_VALUE 32

/* Devlink parameter type enumeration */
enum devlink_param_type {
    DEVLINK_PARAM_TYPE_U8,
    DEVLINK_PARAM_TYPE_U16,
    DEVLINK_PARAM_TYPE_U32,
    DEVLINK_PARAM_TYPE_STRING,
    DEVLINK_PARAM_TYPE_BOOL,
};

/* Devlink parameter value union */
union devlink_param_value {
    u8 vu8;
    u16 vu16;
    u32 vu32;
    char vstr[__DEVLINK_PARAM_MAX_STRING_VALUE];
    bool vbool;
};

/* Devlink parameter configuration mode enumeration */
enum devlink_param_cmode {
    DEVLINK_PARAM_CMODE_RUNTIME,
    DEVLINK_PARAM_CMODE_DRIVERINIT,
    DEVLINK_PARAM_CMODE_PERMANENT,
    /* Add new configuration modes above */
    __DEVLINK_PARAM_CMODE_MAX,
    DEVLINK_PARAM_CMODE_MAX = __DEVLINK_PARAM_CMODE_MAX - 1
};

/* Devlink parameter get/set context structure */
struct devlink_param_gset_ctx {
    union devlink_param_value val;
    enum devlink_param_cmode cmode;
};

/*
 * Generic parameter IDs for devlink
 * These are used to ensure parameter ID uniqueness across different drivers
 */
enum devlink_param_generic_id {
    DEVLINK_PARAM_GENERIC_ID_INT_ERR_RESET = 0,
    DEVLINK_PARAM_GENERIC_ID_MAX_MACS,
    DEVLINK_PARAM_GENERIC_ID_ENABLE_SRIOV,
    DEVLINK_PARAM_GENERIC_ID_REGION_SNAPSHOT,
    DEVLINK_PARAM_GENERIC_ID_IGNORE_ARI,
    DEVLINK_PARAM_GENERIC_ID_MSIX_VEC_PER_PF_MAX,
    DEVLINK_PARAM_GENERIC_ID_MSIX_VEC_PER_PF_MIN,
    DEVLINK_PARAM_GENERIC_ID_FW_LOAD_POLICY,
    /* add new param generic ids above here*/
    __DEVLINK_PARAM_GENERIC_ID_MAX,
    DEVLINK_PARAM_GENERIC_ID_MAX = __DEVLINK_PARAM_GENERIC_ID_MAX - 1,
};

/* Generic parameter names and types */
#define DEVLINK_PARAM_GENERIC_INT_ERR_RESET_NAME "internal_error_reset"
#define DEVLINK_PARAM_GENERIC_INT_ERR_RESET_TYPE DEVLINK_PARAM_TYPE_BOOL
#define DEVLINK_PARAM_GENERIC_MAX_MACS_NAME "max_macs"
#define DEVLINK_PARAM_GENERIC_MAX_MACS_TYPE DEVLINK_PARAM_TYPE_U32
#define DEVLINK_PARAM_GENERIC_ENABLE_SRIOV_NAME "enable_sriov"
#define DEVLINK_PARAM_GENERIC_ENABLE_SRIOV_TYPE DEVLINK_PARAM_TYPE_BOOL
#define DEVLINK_PARAM_GENERIC_REGION_SNAPSHOT_NAME "region_snapshot"
#define DEVLINK_PARAM_GENERIC_REGION_SNAPSHOT_TYPE DEVLINK_PARAM_TYPE_BOOL
#define DEVLINK_PARAM_GENERIC_IGNORE_ARI_NAME "ignore_ari"
#define DEVLINK_PARAM_GENERIC_IGNORE_ARI_TYPE DEVLINK_PARAM_TYPE_BOOL
#define DEVLINK_PARAM_GENERIC_MSIX_VEC_PER_PF_MAX_NAME "msix_vec_per_pf_max"
#define DEVLINK_PARAM_GENERIC_MSIX_VEC_PER_PF_MAX_TYPE DEVLINK_PARAM_TYPE_U32
#define DEVLINK_PARAM_GENERIC_MSIX_VEC_PER_PF_MIN_NAME "msix_vec_per_pf_min"
#define DEVLINK_PARAM_GENERIC_MSIX_VEC_PER_PF_MIN_TYPE DEVLINK_PARAM_TYPE_U32
#define DEVLINK_PARAM_GENERIC_FW_LOAD_POLICY_NAME "fw_load_policy"
#define DEVLINK_PARAM_GENERIC_FW_LOAD_POLICY_TYPE DEVLINK_PARAM_TYPE_U8

/* Helper macro for creating generic parameter definitions */
#define DEVLINK_PARAM_GENERIC(_id) \
    DEVLINK_PARAM_DRIVER(DEVLINK_PARAM_GENERIC_ID_##_id, \
                         DEVLINK_PARAM_GENERIC_##_id##_NAME, \
                         DEVLINK_PARAM_GENERIC_##_id##_TYPE, \
                         BIT(DEVLINK_PARAM_CMODE_RUNTIME), \
                         NULL, NULL, NULL)

#else
/* For non-CGS_V5_693 kernels, use kernel-provided definitions */
/* No additional definitions needed */
#endif

#ifdef __cplusplus
}
#endif

#endif /* __DEVLINK_COMPAT_H__ */
