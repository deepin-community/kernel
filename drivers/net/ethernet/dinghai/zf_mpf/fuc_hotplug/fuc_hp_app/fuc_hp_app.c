#include "fuc_hp_app.h"
#include "../fuc_hotplug_commom.h"

static int printf_usage(int argc, char *argv[]);
static int fuc_hp(int argc, char *argv[]);
static int ep_hp(int argc, char *argv[]);

static int printf_usage(int argc, char *argv[])
{
    printf("** dpu_hotplug -s fuc -e epid -p pfid -v vfid -o add/del -t time  \n");
    printf("** -s 指定拔插场景,fuction级热插拔固定输入:fuc                         \n");
    printf("** -e 指定epid                                                     \n");
    printf("** -p 指定pfid                                                     \n");
    printf("** -v 指定vfid, vfid为0,表示pf                                      \n");
    printf("**             vfid > 0,表示vf,vfid_real = vfid - 1                \n");
    printf("** -o 指定插拔操作, add 插入,del 拔出                                 \n");
    printf("** -t 指定超时时间，单位:s                                            \n");
    printf("** \n");
    printf("** dpu_hotplug -s ep -e epid -o add/del                           \n");
    printf("** -s 指定拔插场景,ep级热插拔固定输入:ep                               \n");
    printf("** -e 指定epid                                                     \n");
    printf("** -o 指定插拔操作, add 插入,del 拔出                                 \n");
    return FUC_HP_OK;
};

struct fuc_hp_app_input fuc_hp_app_input[] = {
    /* ops_type ops_value*/
    {"-e", 0},
    {"-p", 0},
    {"-v", 0},
    {"-o", 0},
    {"-t", 0}
};

struct fuc_hp_app_input ep_hp_app_input[] = {
    /* ops_type ops_value*/
    {"-e", 0},
    {"-o", 0},
};

static int parse_fuc_hp_input(struct fuc_hotplug_bar_msg *fuc_hotplug_bar_msg)
{
    fuc_hotplug_bar_msg->fuc_hotplug_info = ((fuc_hp_app_input[EP_ID].input_value + MIN_EP_ID) << FUNC_HP_EP_ID_START_BIT)
                                            | (fuc_hp_app_input[PF_ID].input_value << FUNC_HP_PF_ID_START_BIT);

    if (fuc_hp_app_input[VF_ID].input_value != 0)
    {
        fuc_hotplug_bar_msg->fuc_hotplug_info |= ((fuc_hp_app_input[2].input_value - 1) << FUNC_HP_VF_ID_START_BIT);
        fuc_hotplug_bar_msg->fuc_hotplug_info |= (1 << FUNC_HP_FUNC_TYPE_START_BIT);
    }

    if (fuc_hp_app_input[OPS_ID].input_value != 0)
    {
        fuc_hotplug_bar_msg->fuc_hotplug_info |= (fuc_hp_app_input[OPS_ID].input_value << FUNC_HP_SCENE_CODE_START_BIT);
    }
    else
    {
        return FUC_HP_FAILED;
    }

    fuc_hotplug_bar_msg->timeout = fuc_hp_app_input[TIMEOUT_ID].input_value;

    printf("value:0x%x timeout:%ds\n", fuc_hotplug_bar_msg->fuc_hotplug_info, fuc_hotplug_bar_msg->timeout);

    return FUC_HP_OK;
}

static int fuc_hp(int argc, char *argv[])
{
    int ret = FUC_HP_OK;
    int fd = 0;
    int arg_no = ARG_START_NO;
    int input_no = 0;
    char *stop_at = NULL;
    struct fuc_hotplug_bar_msg *fuc_hotplug_bar_msg = NULL;

    fuc_hotplug_bar_msg = malloc(sizeof(struct fuc_hotplug_bar_msg));
    if (fuc_hotplug_bar_msg == NULL)
    {
        printf("malloc fuc_hotplug_bar_msg failed!\n");
        return FUC_HP_FAILED;
    }
    fuc_hotplug_bar_msg->fuc_hotplug_info = 0;

    if (argc != FUC_HOTPLUG_MEMBER_NUMS + ARG_START_NO)
    {
        printf("[%s]: Invalid argument count %d\n", __func__, argc);
        printf_usage(argc, argv);
        goto failed;
    }

    while (arg_no < argc)
    {
        if (strcmp(argv[arg_no++], fuc_hp_app_input[input_no].input_type) == 0)
        {
            if (input_no == OPS_ID)
            {
                fuc_hp_app_input[input_no++].input_value = strcmp(argv[arg_no], "add") == 0 ? FUNCTION_INSERT :
                                                           strcmp(argv[arg_no], "del") == 0  ? FUNCTION_REMOVE : 0;
                arg_no++;
            }
            else
            {
                fuc_hp_app_input[input_no++].input_value = strtoul(argv[arg_no++], &stop_at, 0);
            }
        }
        else
        {
            printf("[%s]: Invalid input %s\n", __func__, fuc_hp_app_input[input_no].input_type);
            printf_usage(argc, argv);
            goto failed;
        }
    }

    ret = parse_fuc_hp_input(fuc_hotplug_bar_msg);
    if(ret != FUC_HP_OK)
    {
        printf_usage(argc, argv);
        goto failed;
    }

    fuc_hotplug_bar_msg->cmd = FUC_HP_BAR_MSG_CMD;

    fd = open(FUC_HP_IOCTRL_DEV_NAME, O_RDWR, 0);
    if (fd < 0)
    {
        printf("[%s]: Can not open %s \n", __func__, FUC_HP_IOCTRL_DEV_NAME);
        goto failed;
    }

    ret = ioctl(fd, FUC_HP_IOCTL_CMD0, fuc_hotplug_bar_msg);
    if (ret)
    {
        printf("[%s]: ERR --> %d\n", __func__, ret);
        goto finish;
    }

    if (fuc_hotplug_bar_msg->cpl_chk == FUC_HP_RET_FINISH)
    {
        printf("[%s] fuction hotplug finish!!\n", __func__);
    }
    else if (fuc_hotplug_bar_msg->cpl_chk == FUC_HP_RET_FAILED)
    {
        printf("[%s] fuction hotplug failed!\n", __func__);
    }
    else
    {
        printf("[%s] fuction hotplug timeout!!!\n", __func__);
    }

finish:
    free(fuc_hotplug_bar_msg);
    fuc_hotplug_bar_msg = NULL;
    close(fd);
    return ret;

failed:
    free(fuc_hotplug_bar_msg);
    fuc_hotplug_bar_msg = NULL;
    return ret;
}

static int ep_hp(int argc, char *argv[])
{
    int ret = FUC_HP_OK;
    int fd = 0;
    int arg_no = ARG_START_NO;
    int input_no = 0;
    char *stop_at = NULL;
    struct ep_hotplug_info *ep_hotplug_info = NULL;

    ep_hotplug_info = malloc(sizeof(struct ep_hotplug_info));
    if (ep_hotplug_info == NULL)
    {
        printf("malloc ep_hotplug_info failed!\n");
        return FUC_HP_FAILED;
    }

    if (argc != EP_HOTPLUG_MEMBER_NUMS + ARG_START_NO)
    {
        printf("[%s]: Invalid argument count %d\n", __func__, argc);
        printf_usage(argc, argv);
        goto failed;
    }

    while (arg_no < argc)
    {
        if (strcmp(argv[arg_no++], ep_hp_app_input[input_no].input_type) == 0)
        {
            if (input_no == E_OPS_ID)
            {
                ep_hp_app_input[input_no++].input_value = strcmp(argv[arg_no], "add") == 0 ? FUNCTION_INSERT :
                                                          strcmp(argv[arg_no], "del") == 0  ? FUNCTION_REMOVE : 0;
                arg_no++;
            }
            else
            {
                ep_hp_app_input[input_no++].input_value = strtoul(argv[arg_no++], &stop_at, 0);
            }
        }
        else
        {
            printf("[%s]: Invalid input %s\n", __func__, ep_hp_app_input[input_no].input_type);
            printf_usage(argc, argv);
            goto failed;
        }
    }

    ep_hotplug_info->cmd = EP_HP_BAR_MSG_CMD;
    ep_hotplug_info->ep_no = ep_hp_app_input[E_EP_ID].input_value;
    ep_hotplug_info->ops_type = ep_hp_app_input[E_OPS_ID].input_value;

    fd = open(FUC_HP_IOCTRL_DEV_NAME, O_RDWR, 0);
    if (fd < 0)
    {
        printf("[%s]: Can not open %s \n", __func__, FUC_HP_IOCTRL_DEV_NAME);
        goto failed;
    }

    ret = ioctl(fd, FUC_HP_IOCTL_CMD2, ep_hotplug_info);
    if (ret)
    {
        printf("[%s]: ERR --> %d\n", __func__, ret);
        goto finish;
    }

    if (ep_hotplug_info->cpl_chk == FUC_HP_RET_FINISH)
    {
        printf("[%s] ep hotplug finish!!\n", __func__);
    }
    else if (ep_hotplug_info->cpl_chk == FUC_HP_RET_FAILED)
    {
        printf("[%s] ep hotplug failed!\n", __func__);
    }
    else
    {
        printf("[%s] ep hotplug timeout!!!\n", __func__);
    }

finish:
    free(ep_hotplug_info);
    ep_hotplug_info = NULL;
    close(fd);
    return ret;

failed:
    free(ep_hotplug_info);
    ep_hotplug_info = NULL;
    return ret;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    int arg_no = ARG_TYPE_NO;

    if (argc < ARG_START_NO)
    {
        printf("[%s]: Invalid argument count %d\n", __func__, argc);
        printf_usage(argc, argv);
        ret = FUC_HP_FAILED;
        goto failed;
    }

    printf("start\n");
    if (strcmp(argv[arg_no++], "-s") == 0)
    {
        if (strcmp(argv[arg_no], "ep") == 0)
        {
            ret = ep_hp(argc, argv);
        }
        else if(strcmp(argv[arg_no], "fuc") == 0)
        {
            ret = fuc_hp(argc, argv);
        }
    }

failed:
    return ret;
}
