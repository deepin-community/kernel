#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BAR_CHAN_PLOAD_SIZE      (2036)
#define BAR_REPS_HDR_LEN         (4)
#define DEVICE_FILE              "/dev/bar_ioctl_dev"
#define BAR_IOCTL_CMD_NORMAL     _IOW('a', 1, msg_entity_t)
struct zxdh_ioctl_send_in
{
    uint16_t pload_len;
    uint16_t src;
    uint16_t dst;
    uint16_t event_id;
};

struct zxdh_ioctl_send_out
{
    int ioctl_state;    //消息返回值ioctl状态 - 用户读
    int bar_state;      //bar通道接口级别返回值 - 用户读
};

struct zxdh_ioctl_recv_in
{
    uint16_t event_id;
    uint16_t rsv1;
    uint32_t rsv2;
};

struct zxdh_ioctl_recv_out
{
    uint16_t msg_len;
    uint16_t rsv1;
    uint32_t rsv2;
};

typedef struct normal_msg_entity
{
    union ioctl_ctrl_hdr                          //私有消息控制头
    {
        struct zxdh_ioctl_send_in send_hdr_in;          //普通消息发送头
        struct zxdh_ioctl_send_out send_hdr_out;          //普通消息发送头
    }hdr;
    uint8_t pload[BAR_CHAN_PLOAD_SIZE];
}msg_entity_t;

typedef enum BAR_DRIVER_TYPE {
    MSG_CHAN_END_MPF = 0,
    MSG_CHAN_END_PF,
    MSG_CHAN_END_VF,
    MSG_CHAN_END_RISC,
    MSG_CHAN_END_ERR,
} BAR_DRIVER_TYPE;

int main()
{
    int fd, ret;
    struct normal_msg_entity entity = {0};
    uint8_t data[5] = {0x12, 0x34, 0x53, 0x32, 0xaa};

    /* 1 填写消息发送头， 主要包括消息长度， 消息源、消息目的、 事件id(调用到哪一个消息处理函数)*/
    entity.hdr.send_hdr_in.pload_len = sizeof(data);
    entity.hdr.send_hdr_in.src = MSG_CHAN_END_PF;
    entity.hdr.send_hdr_in.dst = MSG_CHAN_END_RISC;
    entity.hdr.send_hdr_in.event_id = 5;

    /* 2 拷贝消息到pload, 这部分数据会传送到消息处理函数中去*/
    memcpy(entity.pload, data, sizeof(data));

    /* 3 打开ioctl字符设备， ioctl命令码和设备名称看代码*/
    fd = open(DEVICE_FILE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open the device.");
        return 1;
    }
    printf("reps: 0x%llx.\n", *(uint64_t*)entity.pload);

    /* 4 发起ioctl请求发送消息， 消息传送过去后会根据event_id, 自动调用消息处理函数， 并且返回结果到pload字段*/
    ret = ioctl(fd, BAR_IOCTL_CMD_NORMAL, &entity);
    if (ret < 0) {
        perror("IOCTL command failed.");
        ret = 1;
        goto out;
    }

    /* 5 根据前4字节校验ioctl是否正常*/
    /* 判断ioctl通信是否异常*/
    if (entity.hdr.send_hdr_out.ioctl_state != 0)
    {
        printf("ioctl failed, state: %d\n", entity.hdr.send_hdr_out.ioctl_state);
        ret = -1;
        goto out;
    }

    /*6 根据后续4字节校验内核态接口是否调用正常*/
    /* 判断调用bar通道内核态接口是否错误*/
    if (entity.hdr.send_hdr_out.bar_state != 0)
    {
        printf("bar send err, state: %d\n", entity.hdr.send_hdr_out.bar_state);
        ret = entity.hdr.send_hdr_out.bar_state;
        goto out;
    }

    /* 7 调用接口完毕， 从entity.pload中取消息处理函数的回复结果*/
    printf("the sum 2byes of data is 0x%x.\n", *(uint16_t*)entity.pload);

out:
    close(fd);
    return 0;
}