#ifndef __ZXDH_QOS_H__
#define __ZXDH_QOS_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <linux/dinghai/driver.h>
#include <linux/dinghai/en_aux.h>
#include <linux/dinghai/eq.h>
#ifndef CGS_V5_693
#include <linux/compiler_types.h>
#endif
#include <linux/types.h>
#include "dpp_drv_qos.h"
#include "en_sf.h"
// #include "./en_aux/queue.h"

#define ZXDH_PLCR_OPEN
#define ZXDH_SRIOV_SYSFS_EN
#ifdef ZXDH_PLCR_OPEN
    #define ZXDH_PLCR_DEBUG
#endif

#define DROP_ENABLE       1
#define DROP_DISABLE      0
#define PLCR_ENABLE       1
#define PLCR_DISABLE      0

#define PLCR_STEP_SIZE    1u
#define PLCR_MIN_RATE     1u
#define PLCR_MAX_RATE      400 * (1 << 20)
#define PLCR_MAX_PKT_RATE  DPP_CAR_MAX_PKT_CIR_VALUE

#define USER_MAX_BYTE_RATE ((PLCR_MAX_RATE * PLCR_STEP_SIZE) >> 10)
#define USER_MAX_PKT_RATE   PLCR_MAX_PKT_RATE

#define CRED_ID(vport, car_type, profile_id) ((vport & 0xffff) << 20) | ((car_type & 0xf) << 16) | (profile_id & 0x1FF);
#define PROFILE_ID(cred_id)  ((cred_id & 0x1FF))
#define VQM_QUEUE_PAIRS_MAX_NUM 2048

#define PLCR_CAR_A_PROFILE_RES_NUM 512
#define PLCR_CAR_B_PROFILE_RES_NUM 128
#define PLCR_CAR_C_PROFILE_RES_NUM 32

#define PLCR_CAR_A_FLOWID_RES_NUM 10240
#define PLCR_CAR_B_FLOWID_RES_NUM 2368 // 2304 + 64
#define PLCR_CAR_C_FLOWID_RES_NUM 1024
#define PLCR_CAR_B_PF_DEFAULT_FLOWID 2048

#define PLCR_CAR_C_FLOWIDS_PER_EP 256
#define PLCR_CAR_C_FLOWIDS_PER_PF 32

/********************************** definition of types **********************************/
#define FLOWID_2_XARRAY(flowid)       ((flowid + 1)*8)
#define XARRAY_2_FLOWID(flowid)       ((flowid / 8)-1)

#define PLCR_INVALID_PARAM            0xffffffff
#define PLCR_CAR_A_DPDK_FLOWID_OFFSET  8192
#define PLCR_MAX_QUEUE_PAIRS           128 //ZXDH_MAX_PAIRS_NUM
#define PLCR_DEBUG

#ifdef PLCR_DEBUG
#define PLCR_FUNC_DBG_ENTER()\
        LOG_INFO("%s-%d:enter !\n", __FUNCTION__, __LINE__)

#define PLCR_LOG_INFO(fmt, arg...)\
        DH_LOG_INFO(MODULE_PF, fmt, ##arg)

#define PLCR_LOG_ERR(fmt, arg...)\
        DH_LOG_ERR(MODULE_PF, fmt, ##arg)

#define PLCR_LOG_DEBUG_DEV(dev, fmt, arg...)\
        DH_LOG_DEBUG_DEV(MODULE_PF, dev, fmt, ##arg)

#define PLCR_LOG_INFO_DEV(dev, fmt, arg...)\
        DH_LOG_INFO_DEV(MODULE_PF, dev, fmt, ##arg)

#define PLCR_LOG_ERR_DEV(dev, fmt, arg...)\
        DH_LOG_ERR_DEV(MODULE_PF, dev, fmt, ##arg)

#define PLCR_COMM_ASSERT(rtn)\
    do{\
        if(0 != rtn)\
        {\
            PLCR_LOG_ERR("failed and rtn=%d\n", rtn)\
            return rtn;\
        }\
    }while(0)
#else
#define PLCR_FUNC_DBG_ENTER()

#define PLCR_LOG_INFO(fmt, arg...)

#define PLCR_LOG_INFO_DEV(dev, fmt, arg...)

#define PLCR_LOG_ERR(fmt, arg...)\
        LOG_INFO("%s-%d: ", __FUNCTION__, __LINE__);\
        DH_LOG_ERR(MODULE_PF, fmt, ##arg)

#define PLCR_LOG_ERR_DEV(dev, fmt, arg...)\
        LOG_INFO_DEV(dev, "%s-%d: ", __FUNCTION__, __LINE__);\
        DH_LOG_ERR_DEV(MODULE_PF, dev, fmt, ##arg)

#define PLCR_COMM_ASSERT(rtn)\
    do{\
        if(0 != rtn)\
        {\
            return rtn;\
        }\
    }while(0)
#endif

typedef enum
{
    PLCR_DEV_ALL_QID_2_FLOWID_QUEUE_PAIRS_OVERFLOW = 1,
    PLCR_GET_REQ_TYPE_INVALID_ERR,
    PLCR_DUPLICATE_RATE,
    PLCR_REMOVE_RATE_LIMIT,

    PLCR_ERROR_NUM
} E_PLCR_ERR_CODE;

// Define the type of PLCR card
typedef enum{
    E_PLCR_CAR_A = 0,                  // PLCR card type A
    E_PLCR_CAR_B = 1,                  // PLCR card type B
    E_PLCR_CAR_C = 2,                  // PLCR card type C
    E_PLCR_CAR_NUM = 3,                // Number of PLCR card types
} E_PLCR_CAR_TYPE;

// Define the union for PLCR profile configuration
union zxdh_plcr_profile_cfg
{
    DPP_STAT_CAR_PROFILE_CFG_T      byte_profile_cfg;  // Byte rate limit profile configuration
    DPP_STAT_CAR_PKT_PROFILE_CFG_T  pkt_profile_cfg;   // Packet rate limit profile configuration
};

typedef enum {
    E_RATE_LIMIT_MODE0 = 0,            // mode 0:no limit
    E_RATE_LIMIT_MODE1,                // mode 1:queue is mapped to car A flowid
    E_RATE_LIMIT_MODE2,                // mode 2:vfid is mapped to car A flowid
    E_RATE_LIMIT_MODE3,                // mode 3:vf is uninstalled and vport table does not exist.
} E_RATE_LIMIT_MODE;

typedef enum {
    E_RATE_LIMIT_REQ_QUEUE_BYTE = 0,   // queue    byte   rate limit
    E_RATE_LIMIT_REQ_VF_BYTE,          // vf       byte   rate limit
    E_RATE_LIMIT_REQ_VF_GROUP_BYTE,    // vf group byte   rate limit
    E_RATE_LIMIT_REQ_VF_PKT,           // vf       packet rate limit
    E_RATE_LIMIT_REQ_MOVE_VF_GROUP,    //just to move vf to other group
    E_RATE_LIMIT_REQ_TYPE_NUM,         //
} E_RATE_LIMIT_REQ_TYPE;

/* Define rate limit direction */
typedef enum {
    E_RATE_LIMIT_RX = 0,                   // Receive direction
    E_RATE_LIMIT_TX,               // Send direction
} E_RATE_LIMIT_DIRECTION;

/* Define rate limit type */
typedef enum {
    E_RATE_LIMIT_BYTE = 0,             // Byte rate limit
    E_RATE_LIMIT_PACKET,               // Packet rate limit
} E_RATE_LIMIT_PKT_BYTE;

/* Define the structure for rate limit parameters */
typedef struct {
    E_RATE_LIMIT_REQ_TYPE  req_type;   // Limit scope
    E_RATE_LIMIT_DIRECTION direction;  // Limit direction
    E_RATE_LIMIT_PKT_BYTE  mode;       // Limit mode
    uint32_t max_rate;                 // Maximum rate limit
    uint32_t min_rate;                 // Minimum rate limit
    uint32_t queue_id;                 // Queue id
    uint32_t vf_idx;                   // VF index in PF
    uint32_t vfid;                     // VF id is global
    uint32_t vport;                    // VF id is global
    uint32_t group_id;                 // Group id
} zxdh_plcr_rate_limit_paras;

typedef struct {
    uint16_t queue_pairs;
    uint16_t flowids_A[2][PLCR_MAX_QUEUE_PAIRS];        // flowid in car A
    uint16_t flowid_A[2];              // flowid in car A
    uint16_t flowid_B[2];              // flowid in car B
    uint16_t flowid_C[2];              // flowid in car C
} zxdh_plcr_flowids;

// Define the structure for PLCR profile
struct zxdh_plcr_profile
{
    uint16_t ref_cnt;                  // Reference count
    uint16_t profile_id;               // Profile ID
    uint16_t vport;                    // Virtual port
    uint32_t max_rate;                 // Maximum rate
    uint32_t min_rate;                 // Minimum rate
    uint64_t cred_id;                  // Credit ID
    DPP_STAT_CAR_PROFILE_CFG_T profile_cfg; // Profile configuration
};

// Define the structure for PLCR flow
struct zxdh_plcr_flow
{
    uint16_t vport;                    // Virtual port
    uint16_t vf_id;                    // VF ID
    uint16_t profile_id;               // Profile ID
    uint16_t flowid;                   // flowid
    uint16_t map_flowid;               // net car's flowid
    uint16_t next_flowid;              // next car's flowid
    uint32_t max_rate;                 // Maximum rate
    uint32_t min_rate;                 // Minimum rate
};

struct dh_core_dev;
struct zxdh_pf_device;
struct zxdh_en_priv;

typedef enum{
    ZXDH_GROUP_RX_RATE = 0,
    ZXDH_GROUP_TX_RATE = 1,
}ZXDH_GROUP_DATA_TYPE;

struct zxdh_group_obj
{
    struct zxdh_pf_device *pf_dev;
    struct kobject kobj;
    struct completion free_group_comp;

    struct list_head list;
    int32_t group_id;
    int32_t num_vfs;
    uint32_t max_tx_rate;
    uint32_t max_rx_rate;
};

struct zxdh_group_work {
	struct work_struct work;
	struct zxdh_group_obj *group_obj;
};

typedef enum{
    ZXDH_VF_MIN_RATE = 0,
    ZXDH_VF_MAX_RATE = 1,
}ZXDH_VF_METER_DATA_TYPE;

typedef enum{
    VF_METER_RX_BPS = 0,
    VF_METER_RX_PPS = 1,
    VF_METER_TX_BPS = 2,
    VF_METER_TX_PPS = 3,
    VF_METER_TYPE_NUM,
}ZXDH_VF_METER_TYPE;

#define IS_TX_METER(meter_type) (meter_type == VF_METER_TX_BPS || meter_type == VF_METER_TX_PPS)
#define IS_PPS_METER(meter_type) (meter_type == VF_METER_RX_PPS || meter_type == VF_METER_TX_PPS)

struct zxdh_vf_meter_obj
{
    struct zxdh_pf_device *pf_dev;
    struct zxdh_vf_obj *vf_obj;
    struct kobject kobj;
    uint32_t meter_type;
    uint32_t min_rate;
    uint32_t max_rate;
};

struct zxdh_vf_meters
{
    struct kobject *kobj;
    struct kobject *rx_obj;
    struct kobject *tx_obj;
    struct zxdh_vf_meter_obj xps[4];
};

struct zxdh_vf_file_stats
{
    uint64_t tx_packets;
    uint64_t tx_bytes;
    uint64_t rx_packets;
    uint64_t rx_bytes;
    uint64_t rx_broadcast;
    uint64_t rx_multicast;
    uint64_t tx_broadcast;
    uint64_t tx_multicast;
    uint64_t rx_dropped;
    uint64_t tx_error;
    uint64_t rx_error;
};

struct zxdh_vf_obj
{
    struct zxdh_pf_device *pf_dev;
    struct kobject kobj;
    uint16_t vport;
    uint16_t vf_idx;
    struct zxdh_group_obj *group;
    struct zxdh_vf_meters *meters;
};

struct zxdh_sriov_sysfs
{
    struct kobject *sriov_obj;
#ifdef ZXDH_PLCR_DEBUG
    struct kobj_attribute burst_attr;
    struct kobj_attribute profile_attr;
    struct kobj_attribute all_vf_stats_attr;
#endif
    struct kobject *groups_obj;
    struct zxdh_group_obj *group_0;
    struct list_head groups_head;
    struct zxdh_vf_obj *vfs;
};

struct zxdh_plcr_table {
    struct xarray plcr_profiles[E_PLCR_CAR_NUM];          // Array of PLCR profiles(index = prfile id)
    struct xarray plcr_flows[E_PLCR_CAR_NUM];             // Array of PLCR flows(index = flowid)
    struct xarray plcr_maps[E_PLCR_CAR_NUM];              // Array of PLCR flows mapping relationship
    uint32_t burst_size;
    bool is_init;
    bool is_xarray_init;
};

struct zxdh_plcr_cbs
{
    uint32_t min_rate;
    uint32_t max_rate;
    uint32_t cbs;
};

struct vqm_rate {
    uint32_t pack_rate;  //pps
    uint32_t rate;       //kbps
} __attribute__((packed));

struct vqm_poll {
    uint16_t poll_mode; /* bit0:rx, bit1:tx, 1:poll, 0:kick */
    uint16_t poll_time;    /* 0:not cfg, other:cfg, unit:ms */
} __attribute__((packed));

struct vqm_global_feature {
    uint16_t version;
    uint64_t features;
} __attribute__((packed));

struct zxdh_vqm_param
{
    uint16_t vqm_vfid;
    uint16_t opcode; // 0: get, 1: set
    uint16_t cmd;
    union {
        uint8_t mac[6];
        uint8_t enable_flag;
        struct vqm_rate vqm_rate;
        struct vqm_poll vqm_poll;
        struct vqm_global_feature vqm_global_feature;
    };
} __attribute__((packed));

extern const uint32_t gaudPlcrCarxProfileNum[E_PLCR_CAR_NUM];
extern const uint32_t gaudPlcrCarxFlowIdNum[E_PLCR_CAR_NUM];
int zxdh_plcr_remove_rate_limit(struct zxdh_pf_device *pf_dev, E_PLCR_CAR_TYPE car_type, uint32_t flowid, uint32_t flag);
void zxdh_plcr_count_profiles(struct zxdh_pf_device *pf_dev);
int zxdh_plcr_set_rate_limit(struct zxdh_pf_device *pf_dev, E_RATE_LIMIT_PKT_BYTE is_pkt_mode, E_PLCR_CAR_TYPE car_type, uint16_t vport, uint32_t flowid, uint32_t max_rate, uint32_t min_rate);
int32_t zxdh_plcr_get_next_map(struct zxdh_pf_device *pf_dev, E_PLCR_CAR_TYPE car_type, uint32_t  flowid, uint32_t *map_flowid);
int32_t zxdh_plcr_init(struct zxdh_en_priv *en_priv);
int32_t zxdh_plcr_uninit(struct zxdh_en_priv *en_priv);
int zxdh_plcr_get_vport_vfid(struct zxdh_pf_device *pf_dev, uint32_t vf_idx, uint32_t *vport, uint32_t *vfid);
int zxdh_plcr_req_profile(struct zxdh_pf_device *pf_dev, E_PLCR_CAR_TYPE car_type, uint16_t *profile_id_out);
int zxdh_plcr_release_profile(struct zxdh_pf_device *pf_dev, E_PLCR_CAR_TYPE car_type, uint16_t profile_id, uint32_t flag);
int zxdh_plcr_count_up_profile(struct zxdh_pf_device *pf_dev, E_PLCR_CAR_TYPE car_type, uint16_t profile_id);
int zxdh_plcr_count_down_profile(struct zxdh_pf_device *pf_dev, E_PLCR_CAR_TYPE car_type, uint16_t profile_id);
int zxdh_plcr_cfg_profile(struct zxdh_pf_device *pf_dev, E_PLCR_CAR_TYPE car_type, DPP_STAT_CAR_PROFILE_CFG_T *profile_cfg);
int zxdh_plcr_get_profile(struct zxdh_pf_device *pf_dev, E_PLCR_CAR_TYPE car_type, uint32_t pkt_sign, uint16_t profile_id, DPP_STAT_CAR_PROFILE_CFG_T *profile_cfg);
uint32_t zxdh_plcr_reg_maxrate_user(uint32_t reg_maxrate);
int32_t zxdh_plcr_req_flow(struct zxdh_pf_device *pf_dev, E_PLCR_CAR_TYPE car_type, uint16_t flow_id, struct zxdh_plcr_flow **flow);
int32_t zxdh_plcr_release_flow(struct zxdh_pf_device *pf_dev, E_PLCR_CAR_TYPE car_type, uint16_t flow_id);
void zxdh_plcr_update_flow(struct zxdh_plcr_flow *flow, uint16_t vport, uint32_t max_rate, uint32_t min_rate);
int zxdh_plcr_store_profile(struct zxdh_pf_device *pf_dev, E_PLCR_CAR_TYPE car_type, uint32_t user_max_rate, uint32_t user_min_rate, DPP_STAT_CAR_PROFILE_CFG_T *profile_cfg);
int32_t zxdh_plcr_stroe_map(struct zxdh_pf_device *pf_dev, E_PLCR_CAR_TYPE car_type, uint32_t flowid, uint32_t map_flowid);
int32_t zxdh_plcr_clear_map(struct zxdh_pf_device *pf_dev, E_PLCR_CAR_TYPE car_type, uint32_t flowid);
int zxdh_pf_plcr_get_mode(struct zxdh_pf_device *pf_dev, uint16_t vport, E_RATE_LIMIT_MODE *p_mode);
int zxdh_pf_plcr_set_mode(struct zxdh_pf_device *pf_dev, uint16_t vport, E_RATE_LIMIT_MODE mode);
int zxdh_plcr_get_mode(struct zxdh_pf_device *pf_dev, uint16_t vport, E_RATE_LIMIT_MODE *mode);
int zxdh_plcr_set_mode(struct zxdh_pf_device *pf_dev, uint16_t vport, E_RATE_LIMIT_MODE mode);
int zxdh_plcr_unified_set_rate_limit(struct zxdh_pf_device *pf_dev, zxdh_plcr_rate_limit_paras *rate_limit_paras);
int32_t zxdh_plcr_recover_cfg(struct zxdh_vf_item *vf_item,struct zxdh_pf_device *pf_dev,int32_t vf_idx);
int zxdh_vqm_vf_set_rate_limit(struct zxdh_pf_device *pf_dev, uint16_t vqm_vfid, uint32_t vf_rate);

int zxdh_vf_update_sysfs_group(struct zxdh_pf_device *pf_dev, struct zxdh_vf_obj *vf, int32_t group_id);
#ifndef CGS_V5_693
int zxdh_create_vfs_sysfs(struct dh_core_dev *dev, int32_t num_vfs);
void zxdh_destroy_vfs_sysfs(struct dh_core_dev *dev, int32_t num_vfs);
int zxdh_sriov_sysfs_init(struct dh_core_dev *dev);
void zxdh_sriov_sysfs_exit(struct dh_core_dev *dev);
#endif
struct zxdh_en_device *pf_dev_get_edev(struct zxdh_pf_device *pf_dev);

#ifdef __cplusplus
}
#endif

#endif
