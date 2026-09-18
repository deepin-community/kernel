#ifndef _ZXDH_MSG_CHAN_VERSION_H_
#define _ZXDH_MSG_CHAN_VERSION_H_

#ifdef DRIVER_VERSION_VAL
    #define DRV_VERSION DRIVER_VERSION_VAL
#else
    #define DRV_VERSION "1.0-1"
#endif

#define DRV_RELDATE	"December 1, 2022"
#define DRV_NAME	"msg_chan"
#define DRV_DESCRIPTION	"DPU MSG Channel Driver"

#define hbond_version DRV_DESCRIPTION ": v" DRV_VERSION " (" DRV_RELDATE ")\n"

#endif  /* _ZXDH_MSG_CHAN_VERSION_H_ */