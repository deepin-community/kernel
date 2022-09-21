//*****************************************************************************
//  Copyright (c) 2021 Glenfly Tech Co., Ltd.. 
//  All Rights Reserved.
//
//  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Glenfly Tech Co., Ltd..;
//  the contents of this file may not be disclosed to third parties, copied or
//  duplicated in any form, in whole or in part, without the prior written
//  permission of Glenfly Tech Co., Ltd..
//
//  The copyright of the source code is protected by the copyright laws of the People's
//  Republic of China and the related laws promulgated by the People's Republic of China
//  and the international covenant(s) ratified by the People's Republic of China. 
//*****************************************************************************

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <libudev.h>
#include <sys/select.h>

#define USE_ATOMIC_IN_TRACE_H 0
#include "gf_trace.h"

/**
 * Maximum size of a message that can be logged to the trace buffer.
 * Note this message includes a tag, the pid, and the string given as the name.
 * Names should be kept short to get the most use of the trace buffer.
 */
#define GF_TRACE_MESSAGE_LENGTH 1024

__attribute__((visibility("default")))
atomic_bool             gf_trace_is_ready      = ATOMIC_VAR_INIT(false);
int                     gf_trace_marker_fd     = -1;
__attribute__((visibility("default")))
uint64_t                gf_trace_enabled_tags  = GF_TRACE_TAG_NOT_READY;
static pthread_once_t   gf_trace_once_control  = PTHREAD_ONCE_INIT;
static pthread_mutex_t  gf_trace_tags_mutex    = PTHREAD_MUTEX_INITIALIZER;

// Read the sysprop and return the value tags should be set to
static uint64_t gf_trace_get_property()
{
    uint64_t tags = 0;

    // open /sys/kernel/debug/dri/0/gf/umd_trace
    FILE * file = fopen("/sys/kernel/debug/dri/0/gf/umd_trace", "re");
    if (file) {
        char value[32];
        char *endptr;
        if (fgets(value, sizeof(value), file)) {
            tags = strtoull(value, &endptr, 0);
            if (value[0] == '\0' || (*endptr != '\0' && *endptr != '\n')) {
                fprintf(stderr, "Error parsing trace property: Not a number: %s", value);
                tags = 0;
            } else if (errno == ERANGE || tags == ULLONG_MAX) {
                fprintf(stderr, "Error parsing trace property: Number too large: %s\n", value);
                tags = 0;
            }
        } else {
            fprintf(stderr, "Error reading umd_trace_tags: %s (%d)\n", strerror(errno), errno);
        }
        fclose(file);
    } else {
        fprintf(stderr, "Error opening /sys/kernel/debug/dri/0/gf/umd_trace: %s (%d)\n", strerror(errno), errno);
    }

    return (tags | GF_TRACE_TAG_ALWAYS) & GF_TRACE_TAG_VALID_MASK;
}

// Update tags if tracing is ready. Useful as a sysprop change callback.
__attribute__((visibility("default")))
void gf_trace_update_tags()
{
    uint64_t tags;
    if (CC_UNLIKELY(atomic_load_explicit(&gf_trace_is_ready, memory_order_acquire))) {
        tags = gf_trace_get_property();
        pthread_mutex_lock(&gf_trace_tags_mutex);
        gf_trace_enabled_tags = tags;
        pthread_mutex_unlock(&gf_trace_tags_mutex);
    }
}

static void* trace_event_thread(void *data)
{
    struct udev *udev;
    struct udev_monitor *mon;
    struct udev_device *dev;
    int fd;

    udev = udev_new();
    if (!udev)
        return NULL;

    mon = udev_monitor_new_from_netlink(udev, "udev");
    if (!mon) {
        udev_unref(udev);
        return NULL;
    }

    if (udev_monitor_filter_add_match_subsystem_devtype(mon, "pci", NULL) < 0
        || udev_monitor_enable_receiving(mon) < 0) {
        fprintf(stderr, "udev_monitor failed: %s (%d)\n", strerror(errno), errno);

        udev_monitor_unref(mon);
        udev_unref(udev);
        return NULL;
    }

    fd = udev_monitor_get_fd(mon);
    if (fd == -1)
        return NULL;

    while (1) {
        fd_set fds;
        int ret;
        const char *gf_trace_tags_prop;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        ret = select(fd + 1, &fds, NULL, NULL, NULL);
        if (ret > 0 && FD_ISSET(fd, &fds)) {
            dev = udev_monitor_receive_device(mon);
            if (!dev)
                continue;

            gf_trace_tags_prop = udev_device_get_property_value(dev, "GF_TRACE_TAGS");
            if (!gf_trace_tags_prop)
                continue;

            if (CC_UNLIKELY(atomic_load_explicit(&gf_trace_is_ready, memory_order_acquire))) {
                uint64_t trace_tags = strtoull(gf_trace_tags_prop, NULL, 0);
                if (trace_tags > 0) {
                    if (gf_trace_marker_fd == -1) {
                        gf_trace_marker_fd = open("/sys/kernel/debug/tracing/trace_marker", O_WRONLY | O_CLOEXEC);
                        if (gf_trace_marker_fd == -1) {
                            fprintf(stderr, "Error opening trace file: %s (%d)\n", strerror(errno), errno);
                            trace_tags = 0;
                        }
                    }
                } else {
                    trace_tags = 0;
                }

                pthread_mutex_lock(&gf_trace_tags_mutex);
                gf_trace_enabled_tags = (trace_tags | GF_TRACE_TAG_ALWAYS) & GF_TRACE_TAG_VALID_MASK;
                pthread_mutex_unlock(&gf_trace_tags_mutex);
            }
            udev_device_unref(dev);
        }
    }

    close(fd);
    udev_monitor_unref(mon);
    udev_unref(udev);

    return NULL;
}

static void gf_trace_init_once()
{
    static pthread_t event_thread;
    if (pthread_create(&event_thread, NULL, trace_event_thread, NULL)) {
        fprintf(stderr, "Error create trace event thread\n");
        gf_trace_enabled_tags = 0;
        goto done;
    }

    gf_trace_marker_fd = open("/sys/kernel/debug/tracing/trace_marker", O_WRONLY | O_CLOEXEC);
    if (gf_trace_marker_fd == -1) {
        // Silently ignore the error, will try to open later
        // fprintf(stderr, "Error opening trace file: %s (%d)\n", strerror(errno), errno);
        gf_trace_enabled_tags = 0;
        goto done;
    }

    gf_trace_enabled_tags = gf_trace_get_property();

done:
    atomic_store_explicit(&gf_trace_is_ready, true, memory_order_release);
}

__attribute__((visibility("default")))
void gf_trace_setup()
{
    pthread_once(&gf_trace_once_control, gf_trace_init_once);
}

__attribute__((visibility("default")))
void gf_trace_init()
{
    if (CC_UNLIKELY(!atomic_load_explicit(&gf_trace_is_ready, memory_order_acquire))) {
        gf_trace_setup();
    }
}

static inline void gf_trace_with_args_body(const char* prefix, unsigned int prefix_len, const char* format, va_list args)
{
    char buf[GF_TRACE_MESSAGE_LENGTH];
    int len;

    strncpy(buf, prefix, prefix_len);

    len = vsnprintf(buf + prefix_len, GF_TRACE_MESSAGE_LENGTH - prefix_len, format, args);

    if (len < 0) {
        fprintf(stderr, "Error in args in %s: %s\n", __FUNCTION__, format);
        len = prefix_len;
    } else {
        len += prefix_len;
    }

    if (len >= (int) sizeof(buf)) {
        fprintf(stderr, "Truncated args in %s: %s\n", __FUNCTION__, format);
        len = sizeof(buf) - 1;
    }
    write(gf_trace_marker_fd, buf, len);
}

__attribute__((visibility("default")))
void gf_trace_begin_body(const char* name, va_list args)
{
    gf_trace_with_args_body("B|0|", 4, name, args);
}

__attribute__((visibility("default")))
void gf_trace_args_body(const char* format, va_list args)
{
    gf_trace_with_args_body("A|0||", 5, format, args);
}

__attribute__((visibility("default")))
void gf_trace_end_body()
{
    char c = 'E';
    write(gf_trace_marker_fd, &c, 1);
}

__attribute__((visibility("default")))
void gf_trace_end2_body(const char* format, va_list args)
{
    gf_trace_with_args_body("E|0||", 5, format, args);
}

#define WRITE_MSG(format_begin, format_end, name, value) { \
    char buf[GF_TRACE_MESSAGE_LENGTH]; \
    int len = snprintf(buf, sizeof(buf), format_begin "%s" format_end, \
        name, value); \
    if (len >= (int) sizeof(buf)) { \
        /* Given the sizeof(buf), and all of the current format buffers, \
         * it is impossible for name_len to be < 0 if len >= sizeof(buf). */ \
        int name_len = strlen(name) - (len - sizeof(buf)) - 1; \
        /* Truncate the name to make the message fit. */ \
        fprintf(stderr, "Truncated name in %s: %s\n", __FUNCTION__, name); \
        len = snprintf(buf, sizeof(buf), format_begin "%.*s" format_end, \
            name_len, name, value); \
    } \
    write(gf_trace_marker_fd, buf, len); \
}

__attribute__((visibility("default")))
void gf_trace_async_begin_body(const char* name, int32_t cookie)
{
    WRITE_MSG("S|0|", "|%" PRId32, name, cookie);
}

__attribute__((visibility("default")))
void gf_trace_async_end_body(const char* name, int32_t cookie)
{
    WRITE_MSG("F|0|", "|%" PRId32, name, cookie);
}

__attribute__((visibility("default")))
void gf_trace_int_body(const char* name, int32_t value)
{
    WRITE_MSG("C|0|", "|%" PRId32, name, value);
}

__attribute__((visibility("default")))
void gf_trace_int64_body(const char* name, int64_t value)
{
    WRITE_MSG("C|0|", "|%" PRId64, name, value);
}
