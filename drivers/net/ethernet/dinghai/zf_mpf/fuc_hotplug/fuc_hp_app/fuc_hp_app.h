#ifndef __DPMT_APP_H
#define __DPMT_APP_H

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
#include "sys/ioctl.h"

#define FUC_HP_IOCTRL_DEV_NAME             "/dev/fuc_hp_ioctl"


#define FUNC_HP_SCENE_CODE_START_BIT                21
#define FUNC_HP_FUNC_TYPE_START_BIT                 20
#define FUNC_HP_EP_ID_START_BIT                     16
#define FUNC_HP_PF_ID_START_BIT                     12
#define FUNC_HP_VF_ID_START_BIT                     0

struct hp_app_func {
    char *name;
    int (*func)(int argc, char *argv[]);
};

struct fuc_hp_app_input {
    char         *input_type;
    unsigned int input_value;
};

typedef enum {
    EP_ID = 0,
    PF_ID,
    VF_ID,
    OPS_ID,
    TIMEOUT_ID,
    INVALID_FUC_HP_INPUT
} FUC_HP_INPUT;

typedef enum {
    E_EP_ID = 0,
    E_OPS_ID,
    E_INVALID_FUC_HP_INPUT
} EP_HP_INPUT;

#endif