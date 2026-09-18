#ifndef __FUC_HOTPLUG_COMMOM_H
#define __FUC_HOTPLUG_COMMOM_H

#define ARG_START_NO                     3
#define ARG_TYPE_NO                      1
#define FUC_HP_OK                        0
#define FUC_HP_FAILED                    -1


#define FUC_HP_IOCTL_TYPE                '>'
#define FUC_HP_IOCTL_MAGIC               117 /* random */
#define FUC_HP_IOCTL_CMD0                (_IO(FUC_HP_IOCTL_TYPE, FUC_HP_IOCTL_MAGIC + 0))
#define FUC_HP_IOCTL_CMD1                (_IO(FUC_HP_IOCTL_TYPE, FUC_HP_IOCTL_MAGIC + 1))
#define FUC_HP_IOCTL_CMD2                (_IO(FUC_HP_IOCTL_TYPE, FUC_HP_IOCTL_MAGIC + 2))

#define MIN_EP_ID                        5
#define MAX_FUCTION_HOTPLUG_EP_NUMS      4

#define FUC_HOTPLUG_MEMBER_NUMS          10
struct fuc_hotplug_bar_msg {
    unsigned int  cmd;
    unsigned int  fuc_hotplug_info;
    unsigned int  timeout;
    unsigned int  cpl_chk;
};

#define FUC_HOTPLUG_TIMEOUT_NUMS          1
struct fuc_hotplug_set_timeout {
    unsigned int  timeout;
    unsigned int  cpl_chk;
};

struct get_pf_state_info {
    unsigned int cmd;
    unsigned int ep_no;
    unsigned int pf_no;
    unsigned int cpl_chk;
};

struct get_pf_state_resp {
    uint8_t check_cpl;
    uint8_t pf_state_of_ep[MAX_FUCTION_HOTPLUG_EP_NUMS];
};

#define EP_HOTPLUG_MEMBER_NUMS          4
struct ep_hotplug_info {
    unsigned int cmd;
    unsigned int ops_type;
    unsigned int ep_no;
    unsigned int cpl_chk;
};

struct ep_hotplug_resp {
    uint8_t check_cpl;
};


typedef enum {
    FUC_HP_RET_TIMEOUT = 0,
    FUC_HP_RET_FINISH,
    FUC_HP_RET_FAILED,
    INVALID_FUC_HP_RETURE
} FUC_HP_RETURE;

typedef enum {
    FUNCTION_REMOVE = 1,
    FUNCTION_INSERT,
    FUNCTION_INVALID_TYPE,
} FUNCTION_HP_TYPE;

typedef enum {
    FUC_HP_BAR_MSG_CMD = 1,
    USED_BY_HOST_HP,
    GET_STATE_BAR_MSG_CMD,
    EP_HP_BAR_MSG_CMD,
    INVALID_CMD,
} HOTPLUG_CMD;

#endif