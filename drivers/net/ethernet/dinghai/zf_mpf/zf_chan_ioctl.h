#ifndef _ZXDH_MSG_CHAN_IOCTL_H_
#define _ZXDH_MSG_CHAN_IOCTL_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "zf_mpf.h"

#define DEVICE_NAME                    "bar_ioctl_dev"
#define BAR_CHAN_SIZE                  (1024 * 2)
#define BAR_CHAN_PLOAD_SIZE            (BAR_CHAN_SIZE - 12)
#define BAR_REV_HDR_LEN                (4)
#define ZXDH_PF_DEV_NUM                (40)

#define PCI_QUERY_TYPE_SINGLE          (1)
#define PCI_QUERY_TYPE_ALL             (2)

#define MSG_LIST_MAX_LEN               (10)

enum {
    IOCTRL_OK,
    IOCTRL_ERR_MALLOC,
    IOCTRL_ERR_MSG_GET,
    IOCTRL_ERR_COPY_FROM_USR,
    IOCTRL_ERR_SEND_LENGTH_EXCCED,
    IOCTRL_ERR_SEND_EVENTID_EXCCED,
    IOCTRL_ERR_RECV_NOT_REGISTER,
    IOCTRL_ERR_RECV_REPEAT_REGISTER,
    IOCTRL_ERR_SEND_EXCEED_QUEUE_SIZE,
};
struct zxdh_ioctl_send_in
{
    uint16_t pload_len;
    uint16_t src;
    uint16_t dst;
    uint16_t event_id;
};

struct zxdh_ioctl_send_out
{
    int ioctl_state;    //ioctrl级别返回值
    int bar_state;      //bar通道接口级别返回值
};

struct zxdh_ioctl_recv_in
{
    uint16_t event_id;
    uint16_t rsv1;
    uint32_t rsv2;
};

struct zxdh_ioctl_recv_out
{
    uint16_t event_id;
    uint16_t state;
    uint32_t rsv2;
};

typedef struct normal_msg_entity
{
    union ioctl_ctrl_hdr                          //私有消息控制头 8 bytes
    {
        struct zxdh_ioctl_send_in send_hdr_in;          //send消息入参
        struct zxdh_ioctl_send_out send_hdr_out;        //send消息出参
        struct zxdh_ioctl_recv_in recv_hdr_in;         //recv消息入参
        struct zxdh_ioctl_recv_out recv_hdr_out;        //recv
        uint8_t std[8];
    }hdr;
    uint8_t pload[BAR_CHAN_PLOAD_SIZE];
}msg_entity_t;

struct zxdh_mpf_pci_res_item
{
    uint16_t device_id;
    uint16_t pcie_id;
    uint16_t bdf;
    uint8_t  link_state;
    uint8_t  dev_type;
    uint16_t  total_vfs;
    uint16_t  initial_vfs;
    uint16_t  num_vfs;
    uint8_t  vf_stride;
    uint8_t  first_vf_offset;
    uint8_t  pad[8];   //预留字段
};

/* zf内核态和risc通信的约定结构体*/
struct zxdh_mpf_pci_res_list
{
    uint16_t num;
    uint16_t verno;   //版本号
    int res;       //0表示返回成功， 其他表示失败， 包括消息发送失败
    struct zxdh_mpf_pci_res_item  pci_res_lis[ZXDH_PF_DEV_NUM];
};

struct zxdh_mpf_query_pci_res_msg
{
    uint16_t pcie_id;
    uint8_t  dev_type;
    uint8_t  pad[5];
    struct zxdh_mpf_pci_res_list reply;
};

typedef struct zxdh_mpf_query_bar_msg
{
    int ioctl_state;
    int bar_state;
    struct zxdh_mpf_query_pci_res_msg pci_res_msg;
}pci_res_st;

struct zxdh_pci_query_hdr
{
    uint16_t mode;
    uint16_t pcie_id;
};

int zxdh_bar_ioctl_msg_mdl_init(struct dh_core_dev *core_dev);
void zxdh_bar_ioctl_msg_mdl_exit(struct dh_core_dev *core_dev);

#ifdef __cplusplus
}
#endif

#endif /* _ZXDH_MSG_CHAN_IOCTL_H_  */
