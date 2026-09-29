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

#define DEVICE_FILE                 "/dev/bar_ioctl_dev"
#define ZXDH_PF_DEV_NUM             (40)
#define BAR_IOCTL_CMD_SINGLE_DEV    _IOW('a', 2, pci_res_st)
#define BAR_IOCTL_CMD_ALL_DEV       _IOW('a', 3, pci_res_st)
#define PF_PCIE_ID(pf_idx)          (((pf_idx % 8) << 8) | (1 << 11) | ((pf_idx / 8) << 12))

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
    int res;
    uint8_t  pad[4];   //预留字段
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

void print_pf_pci_res(struct zxdh_mpf_pci_res_item *item)
{
    printf("device_id: 0x%x.\n", item->device_id);
    printf("link_state: %d.\n",item->link_state);
    printf("pcie_id: 0x%x.\n",item->pcie_id);
    printf("bdf: 0x%x.\n",item->bdf);
    printf("dev_type: %d.\n",item->dev_type);
    printf("total_vfs: %d.\n",item->total_vfs);
    printf("initial_vfs: %d.\n",item->initial_vfs);
    printf("num_vfs: %d.\n",item->num_vfs);
    printf("vf_stride: %d.\n",item->vf_stride);
    printf("first_vf_offset: %d.\n",item->first_vf_offset);
    return;
}

static int zxdh_get_dev_pci_resource(uint16_t pcie_id, struct zxdh_mpf_query_bar_msg *data, int cmd)
{
    int fd, ret;
    struct zxdh_mpf_query_bar_msg msg = {0};

    msg.pci_res_msg.pcie_id = pcie_id;
    msg.pci_res_msg.dev_type = 8;

    fd = open(DEVICE_FILE, O_RDWR);
    if (fd < 0)
    {
        perror("Failed to open the device.");
        return 1;
    }

    ret = ioctl(fd, cmd, &msg);
    if (ret < 0)
    {
        perror("IOCTL command failed.");
        ret = 1;
        goto out;
    }

    /* ioctl通信异常*/
    if (msg.ioctl_state != 0)
    {
        //return IOCTL_ERR
        printf("ioctl failed, state: %d\n", msg.ioctl_state);
        ret = -1;
        goto out;
    }

    /* 调用bar通道内核态接口错误*/
    if (msg.bar_state != 0)
    {
        //return IOCTL_ERR
        printf("bar send err, state: %d\n", msg.bar_state);
        ret = msg.bar_state;
        goto out;
    }

    *data = msg;
out:
    close(fd);
    return ret;
}

int zxdh_get_dev_pci_resource_single(uint16_t pcie_id, struct zxdh_mpf_query_bar_msg *data)
{
    return zxdh_get_dev_pci_resource(pcie_id, data, BAR_IOCTL_CMD_SINGLE_DEV);
}

int zxdh_get_dev_pci_resource_all(struct zxdh_mpf_query_bar_msg *data)
{
    return zxdh_get_dev_pci_resource(0, data, BAR_IOCTL_CMD_ALL_DEV);
}

void test_pci_res_query_single(uint16_t pcie_id)
{
    int ret = 0;
    struct zxdh_mpf_query_bar_msg data = {0};

    printf("**************pcie_id: 0x%x**************.\n", pcie_id);
    ret = zxdh_get_dev_pci_resource_single(pcie_id, &data);
    if (ret != 0)
    {
        printf("ioctl msg failed, ret: %d.\n", ret);
        return;
    }
    if (data.pci_res_msg.reply.res != 0)
    {
        printf("data.pci_res_msg.reply.res is %d.\n", data.pci_res_msg.reply.res);
        return;
    }
    print_pf_pci_res(&data.pci_res_msg.reply.pci_res_lis[0]);
    return;
}

void test_pci_res_query_all()
{
    int pf_idx = 0;
    int ret = 0;
    uint16_t pcie_id = 0;
    struct zxdh_mpf_query_bar_msg data = {0};

    ret = zxdh_get_dev_pci_resource_all(&data);
    if (ret != 0)
    {
        printf("ioctl msg failed, ret:%d.\n", ret);
    }

    printf("res: %d.\n", data.pci_res_msg.reply.res);
    printf("num: %d.\n", data.pci_res_msg.reply.num);

    for (pf_idx = 0; pf_idx < 40; pf_idx++)
   {
       printf("********%dth dev, ep:%d, pf: %d**********.\n", pf_idx, pf_idx / 8, pf_idx % 8);

       if (data.pci_res_msg.reply.pci_res_lis[pf_idx].res != 0)
       {
           printf("invalid res.\n");
           continue;
       }
           print_pf_pci_res(&data.pci_res_msg.reply.pci_res_lis[pf_idx]);
    }
}

int main(int argc, char *argv[])
{
    uint16_t pcie_id = 0;

    if (argc < 2)
    {
        goto help;
    }
    if (strcmp(argv[1], "all") == 0)
    {
        test_pci_res_query_all();
        goto out;
    }
    else if(strcmp(argv[1], "dev") == 0)
    {
        if (argc < 3)
        {
            goto help;
        }
        pcie_id = strtol(argv[2],NULL, 16);
        test_pci_res_query_single(pcie_id);
        goto out;
    }
    else
    {
        goto help;
    }

help:
    printf("./test all ------------------print all pci_dev resources.\n");
    printf("./test dev [pcie_id] --------print pci_dev resource.\n");
out:
    return 0;
}
