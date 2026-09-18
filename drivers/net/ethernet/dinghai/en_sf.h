#ifndef __ZXDH_PF_EN_SF_H__
#define __ZXDH_PF_EN_SF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/dinghai/driver.h>
#include <linux/dinghai/zxdh_auxiliary_bus.h>
#include <linux/dinghai/en_sf.h>
#include <linux/types.h>
#include <linux/dinghai/queue.h>
#include <linux/dinghai/pci_irq.h>
#include "dpp_np_init.h"
#include "dpp_tbl_pmtu_info.h"
#include "dpp_tbl_qp_info_ex.h"

#define ZXDH_MAJOR_VER      10
#define ZXDH_MINOR_VER      1
#define ZXDH_NET_MAJOR_VER  0  //0-255, bit0-7
#define ZXDH_NET_MINOR_VER  0  //0-255, bit8-15
#define ZXDH_RDMA_MINOR_VER 0  //0-255, bit16-23
#define ZXDH_RDMA_COMM_FUNC 1  //0-1  , bit24
#define ZXDH_HIGH_8BIT      8
#define ZXDH_HIGH_16BIT     16
#define ZXDH_HIGH_24BIT     24
#define ZXDH_SF_ADEV_NUM    32


struct zxdh_rdma_dev_info;

enum zxdh_comm_func_type
{
    ZXDH_COMM_FUNC_NUM_REQUIRE = 0,
    ZXDH_COMM_FUNC_NP_MAC = 1,
    ZXDH_COMM_FUNC_IRQ_REQUEST = 2,
    ZXDH_COMM_FUNC_IRQ_FREE = 3,
    ZXDH_COMM_FUNC_PMTU_INFO_ADD = 4,
    ZXDH_COMM_FUNC_PMTU_INFO_DEL = 5,
    ZXDH_COMM_FUNC_QP_INFO_DEL_EX = 6,
    ZXDH_COMM_FUNC_NETDEV_SPEED_GET = 7,
    ZXDH_COMM_FUNC_SEARCH_MAC_FROM_FW = 8,
    ZXDH_COMM_FUNC_NETDEV_TRUST = 9,
    ZXDH_COMM_FUNC_VF_VLAN_GET = 10,
    ZXDH_COMM_FUNC_NUM_MAX,
};

typedef struct zxdh_dpp_pf_info
{
    uint16_t slot;
    uint16_t vport;
} zxdh_dpp_pf_info_t;

typedef struct zxdh_dpp_pf_mac_info
{
    zxdh_dpp_pf_info_t pf_info;
    const void *mac;
} zxdh_dpp_pf_mac_info_t;

typedef struct zxdh_rdma_msix_info
{
    struct dh_core_dev *dh_dev;
    unsigned int irq;
    irq_handler_t handler;
    unsigned long flags;
    char name[DH_MAX_IRQ_NAME];
    void *data;
    cpumask_var_t mask;
} zxdh_rdma_msix_info_t;

typedef struct zxdh_dpp_pmtu_info 
{
    zxdh_dpp_pf_info_t pf_info;
    ZXDH_PMTU_INFO_T pmtu_info;
} zxdh_dpp_pmtu_info_t;

typedef struct zxdh_dpp_qp_info 
{
    zxdh_dpp_pf_info_t pf_info;
    ZXDH_QP_INFO_T qp_info;
} zxdh_dpp_qp_info_t;

enum AUX_DEVICE_TYPE
{
    NET_AUX_DEVICE,
    RDMA_AUX_DEVICE,
    SEC_AUX_DEVICE,
};

struct zxdh_adev_handle_table
{
    enum AUX_DEVICE_TYPE adev_type;
    int32_t (*cb_fn)(struct dh_core_dev *dh_dev, struct zxdh_auxiliary_device *adev);
};

struct zxdh_ver_info
{
    uint16_t major;
    uint16_t minor;
    uint64_t support;
};

enum zxdh_function_type
{
    ZXDH_FUNCTION_TYPE_PF,
    ZXDH_FUNCTION_TYPE_VF,
};

enum zxdh_rdma_protocol
{
    ZXDH_RDMA_PROTOCOL_IWARP = BIT(0),
    ZXDH_RDMA_PROTOCOL_ROCEV2 = BIT(1),
};

struct zxdh_rdma_qos_params
{
    uint8_t reserve;
};

enum zxdh_rdma_reset_type
{
    ZXDH_RESET_MTU_CHANGE,
    ZXDH_RESET_HW_ERROR,
};

struct zxdh_rdma_dev_ops
{
    int32_t (*request_reset)(struct zxdh_rdma_dev_info *rdma_infos, enum zxdh_rdma_reset_type reset_type);
    int32_t (*zxdh_common_func)(void *in_param,  void *out_param,  uint32_t opcode);
};

/* auxiliary driver tailored information about the core PCI dev */
struct zxdh_rdma_dev_info
{
    struct pci_dev *pdev;
    struct zxdh_auxiliary_device *adev;

    uint8_t __iomem *hw_addr;
    int32_t adev_info_id;
    struct zxdh_ver_info ver;

    void *auxiliary_priv;

    enum zxdh_function_type ftype;
    uint16_t vport_id;
    uint16_t slot_id;
    /* Current active RDMA protocol */
    enum zxdh_rdma_protocol rdma_protocol;

    struct zxdh_rdma_qos_params qos_info;

    struct msix_entry msix_entries;
    /* How many vectors are reserved for this device */
    uint16_t msix_count;
    /* function pointers to be initialized by core PCI driver and called by auxiliary driver */
    struct zxdh_rdma_dev_ops *ops;
    struct dh_core_dev *dh_dev;
};

struct zxdh_en_sf_device {
    int32_t max_channels;
    struct zxdh_en_sf_if *sf_ops;
    void *netdev;
    void *sec_info;

    struct zxdh_auxiliary_device *adev[ZXDH_SF_ADEV_NUM];
    int32_t aux_idx;
    uint32_t speed;
};

int32_t zxdh_irq_request(unsigned int irq, irq_handler_t handler, \
                        unsigned long flags, char *name, \
                        void *data, cpumask_var_t mask);
void zxdh_irq_free(unsigned int irq, void *data);

#ifdef __cplusplus
}
#endif

#endif