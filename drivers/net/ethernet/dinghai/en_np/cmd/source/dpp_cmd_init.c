#include "zxic_common.h"
#include "dpp_netlink.h"
#include "dpp_cmd_init.h"

extern ZXIC_CHAR* dpp_cmd_trim(ZXIC_CHAR* line);
extern ZXIC_UINT32 dpp_cmd_exec(ZXIC_CHAR* line);

ZXIC_UINT32 dpp_cmd_msg_proc(ZXIC_VOID *msg_body, ZXIC_UINT32 msg_len, ZXIC_VOID **resp, ZXIC_UINT32 *reps_len)
{
    ZXIC_CHAR *line = NULL;
    T_MSG_CMD_SHELL *msg = (T_MSG_CMD_SHELL*)(msg_body);

    ZXIC_COMM_CHECK_POINT(msg);

    line = dpp_cmd_trim(msg->command);
    ZXIC_COMM_CHECK_POINT(line);

    if (*line)
    {
        ZXIC_COMM_PRINT("---------------------------------------------------\n");
        dpp_cmd_exec(line);
        ZXIC_COMM_PRINT("---------------------------------------------------\n");
    }

    return DPP_OK;
}

ZXIC_UINT32 dpp_cmd_init(ZXIC_VOID)
{
    dpp_netlink_regist_msg_proc_fun(MSG_ID_MSG_DPP_CMD_SHELL, dpp_cmd_msg_proc);

    return DPP_OK;
}
