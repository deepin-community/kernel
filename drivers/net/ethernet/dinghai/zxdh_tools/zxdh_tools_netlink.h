#ifndef ZXDH_TOOLS_NETLINK_H_
#define ZXDH_TOOLS_NETLINK_H_


#ifdef __cplusplus
extern "C" {
#endif

#define NLA_DATA(na)                   ((void *)((char *)(na) + NLA_HDRLEN))
#define ZXDH_TOOLS_NETLINK_NAME         "tools_family"

typedef enum
{
    EVENT_OP_CODE_DEV_PCIEID_TO_H = 0,
    EVENT_OP_CODE_LOG_GET_TO_H = 1,
    EVENT_OP_CODE_DIAG_TO_H = 2,
    EVENT_OP_CODE_STAT_TO_H = 3,
    EVENT_OP_CODE_REGSDUMP_TO_H = 4,
    EVENT_OP_CODE_REGSMEM_TO_H = 5,
    EVENT_OP_CODE_SN_MAC_SEND_TO_H = 6,
    EVENT_OP_CODE_FWUPDATE_TO_H = 7,
    EVENT_OP_CODE_DINGHAI_RESET_TO_H = 8,
    EVENT_OP_CODE_FPUT_TO_H = 10,
    EVENT_OP_CODE_LOG_GET_FINISH_TO_H     = 11,
    EVENT_OP_CODE_FPUT_FLASH_TO_H = 14,
    EVENT_OP_CODE_NUM_TO_H=100,
}EVENT_OP_CODE_TO_H;

/* 属性类型*/
enum {
    ZXDH_TOOLS_A_UNSPEC,
    ZXDH_TOOLS_A_MSG,
    __ZXDH_TOOLS_A_MAX,
};
#define ZXDH_TOOLS_A_MAX (__ZXDH_TOOLS_A_MAX - 1)

/* 操作码*/
enum {
    ZXDH_TOOLS_C_UNSPEC,
    ZXDH_TOOLS_C_ECHO,
    __ZXDH_TOOLS_C_ECHO,
};
#define ZXDH_TOOLS_C_MAX (__ZXDH_TOOLS_C_MAX - 1)

typedef enum
{
    FWUPDATE = 27,
} event_op_code;

struct zxdh_tools_recv_msg {
    event_op_code op_code;
    uint8_t status;
};


int32_t zxdh_tools_sendto_user_netlink(void *pay_load, uint16_t len, void *reps_buffer, uint16_t *reps_len, void *dev);
int zxdh_tools_netlink_register(void);
void zxdh_tools_netlink_unregister(void);


#ifdef __cplusplus
}
#endif


#endif /* ZXDH_TOOLS_NETLINK_H_  */




