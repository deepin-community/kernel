#ifndef __ZXDH_EN_DCBNL_H__
#define __ZXDH_EN_DCBNL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/device.h>
#include <linux/dinghai/kcompat.h>
/* 启用dcb会大幅度增加初始化时间，暂时先注释 */
#if defined(CHECK_DCB_ENABLED)
#ifdef CONFIG_DCB
#define ZXDH_DCBNL_OPEN
#endif
#else
#define ZXDH_DCBNL_OPEN
#endif

/* CEE not support */
//#define ZXDH_DCBNL_CEE_SUPPORT

#define ZXDH_DCBNL_INIT_FLAG (0x5a5a5a5a)
#define ZXDH_DCBNL_NULL_ID (0xffffffff)

#define ZXDH_DCBNL_MAX_PRIORITY (8)
#define ZXDH_DCBNL_MAX_TRAFFIC_CLASS (8)

#define ZXDH_DCBNL_MAX_DSCP (64)

#define ZXDH_DCBNL_MAX_BW_ALLOC  (100)
#define ZXDH_DCBNL_MAX_WEIGHT    (512)

#define ZXDH_DCBNL_RATEUNIT_K    (1000)
#define ZXDH_DCBNL_RATEUNIT_M    (1000000)
#define ZXDH_DCBNL_RATEUNIT_G    (1000000000)
#define ZXDH_DCBNL_MAXRATE_KBITPS (400*1000000)
#define ZXDH_DCBNL_MINRATE_KBITPS (64)

#define ZXDH_DCBNL_INITRATE_KBITPS (400*1000000)

#define ZXDH_DCBNL_FLOW_RATE_CIR (0)

#define ZXDH_DCBNL_FLOW_RATE_CBS (2000)
#define ZXDH_DCBNL_FLOW_RATE_EBS (4000)
#define ZXDH_DCBNL_PORT_RATE_CBS (4000)

#define ZXDH_DCBNL_FLOW_RATE_CBS_REFRESH (0)
#define ZXDH_DCBNL_FLOW_RATE_EBS_REFRESH (0)

#define ZXDH_DCBNL_FLOW_TDTH (200)
#define ZXDH_DCBNL_FLOW_TDTH_UPF (900)  //待更改成最优值
#define ZXDH_DCBNL_FLOW_TDTH_OPT (500)  
#define ZXDH_DCBNL_FLOW_TDTH_DEFAULT (100)
#define ZXDH_DCBNL_FLOW_TDTH_BD (2000)

#define ZXDH_DCBNL_CEE_STATE_UP  (1)

#define ZXDH_DCBNL_MAX_SE_NODE_NUM      (12)
#define ZXDH_DCBNL_MAX_TREE_LEVEL       (7)
#define ZXDH_DCBNL_ETS_TREE_ROOT_LEVEL  (4)
#define ZXDH_DCBNL_ETS_TREE_FLOW_LEVEL  (0)

#define ZXDH_DCBNL_GSCHID_ID_MASK   (0xFFFF)
#define ZXDH_DCBNL_GSCHID_ID_SHIFT  (0)

#define ZXDH_DCBNL_GET_GSCHID_MSG(val,mask,shift)  ((val >> shift)&mask)

#define ZXDH_DCBNL_INVALID_PARA  (0xffffffff)

#define ZXDH_DCBNL_CHECK_MAX_WITH_RETURN(val, max, ret) \
    do { \
        if(val >= max) \
            return ret; \
    }while(0)

#define ZXDH_DCBNL_CHECK_RANGE_WITH_RETURN(val, min, max, ret) \
    do { \
        if(!(min <= val && val <= max)) \
            return ret; \
    }while(0)

#define ZXDH_DCBNL_CHECK_POINT_RET(point,ret) \
    do{\
       if(NULL == point) \
       {\
          LOG_ERR("\n %s:%d[Error:POINT NULL] ! FUNCTION : %s!\n",__FILE__,__LINE__,__FUNCTION__);\
          return ret;\
       }\
    }while(0)

#define ZXDH_DCBNL_CHECK_POINT(point) \
    do{\
       if(NULL == point) \
       {\
          LOG_ERR("\n %s:%d[Error:POINT NULL] ! FUNCTION : %s!\n",__FILE__,__LINE__,__FUNCTION__);\
          return;\
       }\
    }while(0)

#define ZXDH_DCBNL_CHECK_RET_RETURN(ret) \
    do{\
       if(0 != ret) \
       {\
          LOG_ERR("\n %s:%d error FUNCTION : %s!\n",__FILE__,__LINE__,__FUNCTION__);\
          return ret;\
       }\
    }while(0)

#define MAX_RATE_LIMITED_NUM (8)
enum zxdh_dcbnl_ets_trust {
    ZXDH_DCBNL_ETS_TRUST_PCP  = 0,
    ZXDH_DCBNL_ETS_TRUST_DSCP = 1,
};

enum zxdh_dcbnl_ets_tc_tsa {
    ZXDH_DCBNL_VENDOR_TC     = 0,
    ZXDH_DCBNL_STRICT_TC     = 1,
    ZXDH_DCBNL_ETS_TC        = 2,
    ZXDH_DCBNL_ZEROBW_ETS_TC = 3,
};

enum zxdh_dcbnl_ets_node_link_point {
    ZXDH_DCBNL_ETS_NODE_NULL         = 0,
    ZXDH_DCBNL_ETS_NODE_VENDOR_C     = 1,
    ZXDH_DCBNL_ETS_NODE_STRICT_C     = 2,
    ZXDH_DCBNL_ETS_NODE_ETS_C        = 3,
    ZXDH_DCBNL_ETS_NODE_ZEROBW_ETS_C = 4,
    ZXDH_DCBNL_ETS_NODE_VENDOR_E     = 5,
    ZXDH_DCBNL_ETS_NODE_STRICT_E     = 6,
    ZXDH_DCBNL_ETS_NODE_ETS_E        = 7,
    ZXDH_DCBNL_ETS_NODE_ZEROBW_ETS_E = 8,
};

enum zxdh_dcbnl_se_flow_node_type {
    ZXDH_DCBNL_ETS_NODE_FQ   = 0,
    ZXDH_DCBNL_ETS_NODE_FQ2  = 1,
    ZXDH_DCBNL_ETS_NODE_FQ4  = 2,
    ZXDH_DCBNL_ETS_NODE_FQ8  = 3,
    ZXDH_DCBNL_ETS_NODE_SP   = 4,
    ZXDH_DCBNL_ETS_NODE_WFQ  = 5,
    ZXDH_DCBNL_ETS_NODE_WFQ2 = 6,
    ZXDH_DCBNL_ETS_NODE_WFQ4 = 7,
    ZXDH_DCBNL_ETS_NODE_WFQ8 = 8,
    ZXDH_DCBNL_ETS_NODE_FLOW = 9,
};

struct zxdh_dcbnl_ets_se_node{
    struct zxdh_dcbnl_ets_se_node *se_next;
    uint64_t  gsch_id;
    uint32_t  node_idx;
    uint32_t  node_type;
    uint32_t  se_id;
    uint32_t  se_link_id;
    uint32_t  se_link_weight;
    uint32_t  se_link_sp;
    uint32_t  link_point;
};

struct zxdh_dcbnl_ets_flow_node{
    struct zxdh_dcbnl_ets_flow_node *flow_next;
    uint64_t  gsch_id;
    uint32_t  flow_id;
    uint32_t  tc_id;
    uint32_t  tc_type;
    uint32_t  tc_tx_bw;
    uint32_t  td_th;
    uint32_t  c_linkid;
    uint32_t  c_weight;
    uint32_t  c_sp;
    uint32_t  c_rate;
    uint32_t  mode;
    uint32_t  e_linkid;
    uint32_t  e_weight;
    uint32_t  e_sp;
    uint32_t  e_rate;
};

struct zxdh_dcbnl_ets_node_list_head{
    struct zxdh_dcbnl_ets_se_node *se_next;
    struct zxdh_dcbnl_ets_flow_node *flow_next;
    uint32_t node_num;
};

struct zxdh_dcbnl_ets_se_flow_resource{
    uint32_t  numq;
    uint32_t  level;
    uint32_t  flags;
    uint32_t  resource_id;
    uint64_t  gsch_id;
};

struct zxdh_dcbnl_se_tree_config{
    uint32_t  level;
    uint32_t  idx;
    uint32_t  type;
    uint32_t  link_level;
    uint32_t  link_idx;
    uint32_t  link_weight;
    uint32_t  link_sp;
    uint32_t  link_point;
};

struct zxdh_dcbnl_tc_flow_config{
    uint32_t  link_level;
    uint32_t  tc_type;
    uint32_t  tc_tx_bw;
    uint32_t  c_rate;
    uint32_t  e_rate;
    uint32_t  td_th;
};

struct zxdh_dcbnl_tc_flow_shape_para{
    uint32_t  cir;
    uint32_t  cbs;
    uint32_t  db_en;
    uint32_t  eir;
    uint32_t  ebs;
};

struct zxdh_dcbnl_ieee_ets {
    uint8_t    willing;
    uint8_t    ets_cap;
    uint8_t    cbs;
    uint8_t    tc_tx_bw[ZXDH_DCBNL_MAX_TRAFFIC_CLASS];
    uint8_t    tc_tsa[ZXDH_DCBNL_MAX_TRAFFIC_CLASS];
    uint8_t    prio_tc[ZXDH_DCBNL_MAX_PRIORITY];
};

struct zxdh_dcbnl_cee_ets {
    uint8_t    tc_tx_bw[ZXDH_DCBNL_MAX_TRAFFIC_CLASS];
    uint8_t    tc_tsa[ZXDH_DCBNL_MAX_TRAFFIC_CLASS];
    uint8_t    prio_tc[ZXDH_DCBNL_MAX_PRIORITY];
};

struct zxdh_dcbnl_para {
    uint32_t   init_flag;
    uint32_t   trust;
    uint32_t   dscp_app_num;
    uint8_t    dscp2prio[ZXDH_DCBNL_MAX_DSCP];
    uint64_t   tc_maxrate[ZXDH_DCBNL_MAX_TRAFFIC_CLASS];
    struct zxdh_dcbnl_ieee_ets ets_cfg;
    struct zxdh_dcbnl_cee_ets  cee_ets_cfg;
    struct zxdh_dcbnl_ets_node_list_head ets_node_list_head[ZXDH_DCBNL_MAX_TREE_LEVEL];
};

// 只在切换ets状态时维护
struct zxdh_dcbnl_ets_switch_info {
    uint32_t  cur_ets;
    uint32_t  tc_td_th[ZXDH_DCBNL_MAX_TRAFFIC_CLASS];
    uint32_t  switch_flag;
};

struct zxdh_dcbnl_rdma_trust {
    uint32_t trust_mode;
    uint32_t port;
};

struct zxdh_dcbnl_ieee_rdma_ets {
    uint32_t    port;
    uint32_t    mode;
    uint8_t    tc_tx_bw[ZXDH_DCBNL_MAX_TRAFFIC_CLASS];
    uint8_t    tc_tsa[ZXDH_DCBNL_MAX_TRAFFIC_CLASS];
    uint8_t    prio_tc[ZXDH_DCBNL_MAX_PRIORITY];
};

struct  zxdh_dcbnl_ieee_rdma_maxrate {
    uint32_t    port;
    uint32_t    mode;
    uint64_t   tc_maxrate[ZXDH_DCBNL_MAX_TRAFFIC_CLASS];
};

struct  zxdh_rdma_status {
    uint32_t    port;
    uint32_t    mode;
};

uint32_t zxdh_dcbnl_initialize(struct net_device *netdev);
uint32_t zxdh_dcbnl_ets_uninit(struct net_device *netdev);
uint32_t zxdh_dcbnl_set_tm_pport_mcode_gate_open(struct net_device *netdev);
uint32_t zxdh_dcbnl_set_tm_pport_mcode_gate_close(struct net_device *netdev);
uint32_t zxdh_dcbnl_set_np_flow_monitor_flag(struct net_device *netdev, uint32_t flag);

#ifdef __cplusplus
}
#endif

#endif
