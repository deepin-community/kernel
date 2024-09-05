#ifndef _SXE_IOCTL_H_
#define _SXE_IOCTL_H_

#ifdef SXE_HOST_DRIVER
#include "sxe_drv_type.h"
#endif

struct SxeIoctlSyncCmd {
    U64   traceid;
    void *inData;
    U32   inLen;
    void *outData;
    U32   outLen;
};

#define SXE_CMD_IOCTL_SYNC_CMD _IOWR('M', 1, struct SxeIoctlSyncCmd)

#endif
