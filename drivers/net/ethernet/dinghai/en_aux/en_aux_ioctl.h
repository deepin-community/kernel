#ifndef _EN_AUX_IOCTL_H_
#define _EN_AUX_IOCTL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "../en_aux.h"

#define SIOCDEVPRIVATE_WRITE_MAC     (SIOCDEVPRIVATE + 1)
#define SIOCDEVPRIVATE_VQ_INFO       (SIOCDEVPRIVATE + 2)
#define SIOCDEVPRIVATE_SEND_FILE_PKT (SIOCDEVPRIVATE + 6)
#define SIOCDEVPRIVATE_PTP_FUNC      (SIOCDEVPRIVATE + 9)
#define SIOCDEVPRIVATE_PPS_FUNC      (SIOCDEVPRIVATE + 10)
#define SIOCDEVPRIVATE_TSN_FUNC      (SIOCDEVPRIVATE + 11)
#define SIOCDEVPRIVATE_DH_TOOLS      (SIOCDEVPRIVATE + 13)

#define PTP_SET_CLOCK_NO              (0)
#define PTP_ENABLE_PTP_ENCRYPTED_MSG  (1)
#define PTP_SET_INTR_CAPTURE_TIMER    (2)
#define PTP_SET_PP1S_SELECTION        (3)
#define PTP_SET_PHASE_DETECTION       (4)
#define PTP_GET_PD_VALUE              (5)
#define PTP_SET_L2PTP_PORT            (6)
#define PTP_SET_PTP_EC_ENABLE         (7)
#define PTP_SET_SYNCE_CLK_PORT        (8)
#define PTP_GET_SYNCE_CLK_STATS       (9)
#define PTP_SET_SPM_PORT_TSTAMP_ENABLE     (10)
#define PTP_GET_SPM_PORT_TSTAMP_ENABLE     (11)
#define PTP_SET_SPM_PORT_TSTAMP_MODE       (12)
#define PTP_GET_SPM_PORT_TSTAMP_MODE       (13)
#define PTP_SET_DELAY_STATISTICS_ENABLE    (14)
#define PTP_GET_DELAY_STATISTICS_VALUE     (15)
#define PTP_CLR_DELAY_STATISTICS_VALUE     (16)
#define PTP_SET_LOCAL_PPS_INTERRUPT_ENABLE  (17)
#define PTP_SET_EXT_PPS_INTERRUPT_ENABLE    (18)
#define PTP_SET_PD_SEL_SHIFT                (19)
#define PTP_GET_PTP_CLOCK_INDEX             (20)

#define PI_HDR_MAX_NUM 128
#define GET_LOW32 0x00000000ffffffff
#define MIN_ALIGN_BYTE 64
#define SEND_PKT_CNT_MAX 0xffffffff
#define PKT_PRINT_LINE_LEN 16
#define PKT_PRINT_LEN_MAX (16 * 1024)

#define CONFIG_RISC_PCS_LOOPB_OPCODE    13
#define CONFIG_RISC_PCS_NORMAL_OPCODE   14

#define MSG_MODULE_DEBUG_RISC   20

#define MAX_ACCESS_NUM 500
struct zxdh_en_reg
{
    uint32_t offset;
    uint32_t num;
    uint32_t data[MAX_ACCESS_NUM];
};

struct risc_config_mac_msg
{
    uint8_t op_code;
    uint8_t phyport;
    uint8_t spm_speed;
    uint8_t spm_fec;
    uint8_t loop_enable;
};

struct risc_config_userspace
{
    uint8_t op_code;
    uint8_t arg_num;
    uint8_t filestr_size;
    uint8_t file[100];
};

struct data_packet
{
    void *buf;
    uint32_t buf_size;
};

struct zxdh_en_ioctl_table
{
    int32_t cmd;
    int32_t (*func)(struct net_device *netdev, struct ifreq *ifr);
};

struct zxdh_en_ptp_ioctl_table
{
    int32_t cmd;
    int32_t (*func)(struct net_device *netdev, struct ifreq *ifr, struct zxdh_en_reg *reg);
};

int32_t print_data(uint8_t *data, uint32_t len);
int32_t zxdh_en_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd);
int32_t zxdh_queue_info_common_print(struct zxdh_en_device *en_dev, struct zxdh_en_reg *reg);

#if defined(ZXDH_ADAPT_REDHAT_9_2) || defined(USE_PRIV_IOCTL) || defined(ZXDH_ADAPT_REDHAT_9_1)
int32_t zxdh_en_private_ioctl(struct net_device *netdev, struct ifreq *ifr, void *data, int cmd);
#endif

#ifdef __cplusplus
}
#endif

#endif

