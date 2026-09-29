#ifndef _TOD_DRIVER_H_
#define _TOD_DRIVER_H_

#include <linux/types.h>

#define TOD_DEVICE_MSG_OPEN         ((uint32_t)(0))
#define TOD_DEVICE_MSG_CLOSE        ((uint32_t)(1))
#define TOD_DEVICE_MSG_READ         ((uint32_t)(2))
#define TOD_DEVICE_MSG_WRITE        ((uint32_t)(3))
#define TOD_DEVICE_MSG_POLL         ((uint32_t)(4))
#define TOD_DEVICE_MSG_IOCTL        ((uint32_t)(5))

#define TOD_DEVICE_DATA_LEN         ((uint32_t)(512))

struct tod_device_msg
{
    uint32_t type;
    uint32_t command;
    size_t   count;
    void*    file;
    uint8_t  data[TOD_DEVICE_DATA_LEN];
};

#endif
