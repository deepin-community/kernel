//generate function cmdlist from symbol file

#ifndef DPP_CMD_INIT_H
#define DPP_CMD_INIT_H

#include "zxic_common.h"

#define MSG_ID_MSG_DPP_CMD_SHELL    ((ZXIC_UINT32)(100))
typedef struct {
    ZXIC_UINT32 msgId;
    ZXIC_UINT8 command[256];
} T_MSG_CMD_SHELL;

ZXIC_UINT32 dpp_cmd_init(ZXIC_VOID);

#endif
